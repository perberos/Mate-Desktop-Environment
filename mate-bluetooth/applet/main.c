/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2005-2008  Marcel Holtmann <marcel@holtmann.org>
 *  Copyright (C) 2006-2007  Bastien Nocera <hadess@hadess.net>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <unique/uniqueapp.h>

#include <bluetooth-client.h>
#include <bluetooth-client-private.h>
#include <bluetooth-chooser.h>
#include <bluetooth-killswitch.h>

#include "notify.h"
#include "agent.h"

static gboolean option_debug = FALSE;
static BluetoothClient *client;
static GtkTreeModel *devices_model;
static guint num_adapters_present = 0;
static guint num_adapters_powered = 0;
static gboolean show_icon_pref = TRUE;
static gboolean discover_lock = FALSE;

#define SCHEMA_NAME		"org.mate.Bluetooth"
#define PREF_SHOW_ICON		"show-icon"

#define KEYBOARD_PREFS		"mate-keyboard-properties"
#define MOUSE_PREFS		"mate-mouse-properties"
#define SOUND_PREFS		"mate-volume-control"
#define SOUND_PREFS_CMDLINE	SOUND_PREFS " -p hardware"

enum {
	CONNECTED,
	DISCONNECTED,
	CONNECTING,
	DISCONNECTING
};

static GSettings *settings;
static BluetoothKillswitch *killswitch = NULL;

static GtkBuilder *xml = NULL;
static GtkActionGroup *devices_action_group = NULL;

/* Signal callbacks */
void settings_callback(GObject *widget, gpointer user_data);
void quit_callback(GObject *widget, gpointer user_data);
void browse_callback(GObject *widget, gpointer user_data);
void bluetooth_status_callback (GObject *widget, gpointer user_data);
void bluetooth_discoverable_callback (GtkToggleAction *toggleaction, gpointer user_data);
void wizard_callback(GObject *widget, gpointer user_data);
void sendto_callback(GObject *widget, gpointer user_data);

static void action_set_bold (GtkUIManager *manager, GtkAction *action, const char *path);
static void update_icon_visibility (void);

void quit_callback(GObject *widget, gpointer user_data)
{
	gtk_main_quit ();
}

void settings_callback(GObject *widget, gpointer user_data)
{
	const char *command = "bluetooth-properties";

	if (!g_spawn_command_line_async(command, NULL))
		g_printerr("Couldn't execute command: %s\n", command);
}

static void
select_device_changed(BluetoothChooser *sel,
		      char *address,
		      gpointer user_data)
{
	GtkDialog *dialog = user_data;

	gtk_dialog_set_response_sensitive(dialog,
				GTK_RESPONSE_ACCEPT, address != NULL);
}

static void
mount_finish_cb (GObject *source_object,
		 GAsyncResult *res,
		 gpointer user_data)
{
	GError *error = NULL;
	char *uri;

	if (g_file_mount_enclosing_volume_finish (G_FILE (source_object),
						  res, &error) == FALSE) {
		/* Ignore "already mounted" error */
		if (error->domain == G_IO_ERROR &&
		    error->code == G_IO_ERROR_ALREADY_MOUNTED) {
			g_error_free (error);
			error = NULL;
		} else {
			g_printerr ("Failed to mount OBEX volume: %s", error->message);
			g_error_free (error);
			return;
		}
	}

	uri = g_file_get_uri (G_FILE (source_object));
	if (gtk_show_uri (NULL, uri, GDK_CURRENT_TIME, &error) == FALSE) {
		g_printerr ("Failed to open %s: %s", uri, error->message);
		g_error_free (error);
	}
	g_free (uri);
}

void browse_callback(GObject *widget, gpointer user_data)
{
	GFile *file;
	char *address, *uri;

	address = g_strdup (g_object_get_data (widget, "address"));
	if (address == NULL) {
		GtkWidget *dialog, *selector, *content_area;
		int response_id;

		dialog = gtk_dialog_new_with_buttons(_("Select Device to Browse"), NULL,
						     GTK_DIALOG_NO_SEPARATOR,
						     GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
						     NULL);
		gtk_dialog_add_button (GTK_DIALOG (dialog), _("_Browse"), GTK_RESPONSE_ACCEPT);
		gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog),
						  GTK_RESPONSE_ACCEPT, FALSE);
		gtk_window_set_default_size(GTK_WINDOW(dialog), 480, 400);

		gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
		content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
		gtk_box_set_spacing (GTK_BOX (content_area), 2);

		selector = bluetooth_chooser_new(_("Select device to browse"));
		gtk_container_set_border_width(GTK_CONTAINER(selector), 5);
		gtk_widget_show(selector);
		g_object_set(selector,
			     "show-searching", FALSE,
			     "show-device-category", FALSE,
			     "show-device-type", TRUE,
			     "device-category-filter", BLUETOOTH_CATEGORY_PAIRED_OR_TRUSTED,
			     "device-service-filter", "OBEXFileTransfer",
			     NULL);
		g_signal_connect(selector, "selected-device-changed",
				 G_CALLBACK(select_device_changed), dialog);
		gtk_container_add (GTK_CONTAINER (content_area), selector);

		address = NULL;
		response_id = gtk_dialog_run (GTK_DIALOG (dialog));
		if (response_id == GTK_RESPONSE_ACCEPT)
			g_object_get (G_OBJECT (selector), "device-selected", &address, NULL);

		gtk_widget_destroy (dialog);

		if (response_id != GTK_RESPONSE_ACCEPT)
			return;
	}

	uri = g_strdup_printf ("obex://[%s]/", address);
	g_free (address);

	file = g_file_new_for_uri (uri);
	g_free (uri);

	g_file_mount_enclosing_volume (file, G_MOUNT_MOUNT_NONE, NULL, NULL, mount_finish_cb, NULL);
	g_object_unref (file);
}

void sendto_callback(GObject *widget, gpointer user_data)
{
	GPtrArray *a;
	GError *err = NULL;
	guint i;
	const char *address, *alias;

	address = g_object_get_data (widget, "address");
	alias = g_object_get_data (widget, "alias");

	a = g_ptr_array_new ();
	g_ptr_array_add (a, "bluetooth-sendto");
	if (address != NULL) {
		char *s;

		s = g_strdup_printf ("--device=\"%s\"", address);
		g_ptr_array_add (a, s);
	}
	if (address != NULL && alias != NULL) {
		char *s;

		s = g_strdup_printf ("--name=\"%s\"", alias);
		g_ptr_array_add (a, s);
	}
	g_ptr_array_add (a, NULL);

	if (g_spawn_async(NULL, (char **) a->pdata, NULL,
			  G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &err) == FALSE) {
		g_printerr("Couldn't execute command: %s\n", err->message);
		g_error_free (err);
	}

	for (i = 1; a->pdata[i] != NULL; i++)
		g_free (a->pdata[i]);

	g_ptr_array_free (a, TRUE);
}

static void keyboard_callback(GObject *widget, gpointer user_data)
{
	const char *command = KEYBOARD_PREFS;

	if (!g_spawn_command_line_async(command, NULL))
		g_printerr("Couldn't execute command: %s\n", command);
}

static void mouse_callback(GObject *widget, gpointer user_data)
{
	const char *command = MOUSE_PREFS;

	if (!g_spawn_command_line_async(command, NULL))
		g_printerr("Couldn't execute command: %s\n", command);
}

static void sound_callback(GObject *widget, gpointer user_data)
{
	const char *command = SOUND_PREFS_CMDLINE;

	if (!g_spawn_command_line_async(command, NULL))
		g_printerr("Couldn't execute command: %s\n", command);
}

void wizard_callback(GObject *widget, gpointer user_data)
{
	const char *command = "bluetooth-wizard";

	if (!g_spawn_command_line_async(command, NULL))
		g_printerr("Couldn't execute command: %s\n", command);
}

static gboolean
set_powered_foreach (GtkTreeModel *model,
		     GtkTreePath  *path,
		     GtkTreeIter  *iter,
		     gpointer      data)
{
	DBusGProxy *proxy = NULL;
	GValue value = { 0, };

	gtk_tree_model_get (model, iter,
			    BLUETOOTH_COLUMN_PROXY, &proxy, -1);
	if (proxy == NULL)
		return FALSE;

	g_value_init (&value, G_TYPE_BOOLEAN);
	g_value_set_boolean (&value, TRUE);

	dbus_g_proxy_call_no_reply (proxy, "SetProperty",
				    G_TYPE_STRING, "Powered",
				    G_TYPE_VALUE, &value,
				    G_TYPE_INVALID,
				    G_TYPE_INVALID);

	g_value_unset (&value);
	g_object_unref (proxy);

	return FALSE;
}

static void
bluetooth_set_adapter_powered (void)
{
	GtkTreeModel *adapters;

	adapters = bluetooth_client_get_adapter_model (client);
	gtk_tree_model_foreach (adapters, set_powered_foreach, NULL);
	g_object_unref (adapters);
}

void bluetooth_status_callback (GObject *widget, gpointer user_data)
{
	GObject *object;
	gboolean active;

	object = gtk_builder_get_object (xml, "killswitch");
	active = GPOINTER_TO_INT (g_object_get_data (object, "bt-active"));
	active = !active;
	bluetooth_killswitch_set_state (killswitch,
					active ? KILLSWITCH_STATE_UNBLOCKED : KILLSWITCH_STATE_SOFT_BLOCKED);
	g_object_set_data (object, "bt-active", GINT_TO_POINTER (active));
	bluetooth_set_adapter_powered ();
}

void
bluetooth_discoverable_callback (GtkToggleAction *toggleaction, gpointer user_data)
{
	gboolean discoverable;

	discoverable = gtk_toggle_action_get_active (toggleaction);
	discover_lock = TRUE;
	bluetooth_client_set_discoverable (client, discoverable);
	discover_lock = FALSE;
}

static gboolean program_available(const char *program)
{
	gchar *path;

	path = g_find_program_in_path(program);
	if (path == NULL)
		return FALSE;

	g_free(path);

	return TRUE;
}

static void popup_callback(GObject *widget, guint button,
				guint activate_time, gpointer user_data)
{
	GtkWidget *menu = user_data;

	gtk_menu_popup(GTK_MENU(menu), NULL, NULL,
			gtk_status_icon_position_menu,
			GTK_STATUS_ICON(widget), button, activate_time);
}

static void activate_callback(GObject *widget, gpointer user_data)
{
	guint32 activate_time = gtk_get_current_event_time();

	if (query_blinking() == TRUE) {
		show_agents();
		return;
	}

	popup_callback(widget, 0, activate_time, user_data);
}

static void
killswitch_state_changed (BluetoothKillswitch *killswitch, KillswitchState state)
{
	GObject *object;
	gboolean sensitive = TRUE;
	gboolean bstate = FALSE;
	const char *label, *status_label;

	if (state == KILLSWITCH_STATE_NO_ADAPTER) {
		object = gtk_builder_get_object (xml, "bluetooth-applet-popup");
		gtk_menu_popdown (GTK_MENU (object));
		update_icon_visibility ();
		return;
	}

	if (state == KILLSWITCH_STATE_SOFT_BLOCKED) {
		label = N_("Turn On Bluetooth");
		status_label = N_("Bluetooth: Off");
		bstate = FALSE;
	} else if (state == KILLSWITCH_STATE_UNBLOCKED) {
		label = N_("Turn Off Bluetooth");
		status_label = N_("Bluetooth: On");
		bstate = TRUE;
	} else if (state == KILLSWITCH_STATE_HARD_BLOCKED) {
		sensitive = FALSE;
		label = NULL;
		status_label = N_("Bluetooth: Disabled");
	} else {
		g_assert_not_reached ();
	}

	object = gtk_builder_get_object (xml, "killswitch-label");
	gtk_action_set_label (GTK_ACTION (object), _(status_label));
	gtk_action_set_visible (GTK_ACTION (object), TRUE);

	object = gtk_builder_get_object (xml, "killswitch");
	gtk_action_set_visible (GTK_ACTION (object), sensitive);
	gtk_action_set_label (GTK_ACTION (object), _(label));

	if (sensitive != FALSE) {
		gtk_action_set_label (GTK_ACTION (object), _(label));
		g_object_set_data (object, "bt-active", GINT_TO_POINTER (bstate));
	}

	object = gtk_builder_get_object (xml, "bluetooth-applet-ui-manager");
	gtk_ui_manager_ensure_update (GTK_UI_MANAGER (object));

	update_icon_visibility ();
}

static GtkWidget *create_popupmenu(void)
{
	GObject *object;
	GError *error = NULL;

	xml = gtk_builder_new ();
	if (gtk_builder_add_from_file (xml, "popup-menu.ui", &error) == 0) {
		if (error->domain == GTK_BUILDER_ERROR) {
			g_warning ("Failed to load popup-menu.ui: %s", error->message);
			g_error_free (error);
			return NULL;
		}
		g_error_free (error);
		error = NULL;
		if (gtk_builder_add_from_file (xml, PKGDATADIR "/popup-menu.ui", &error) == 0) {
			g_warning ("Failed to load popup-menu.ui: %s", error->message);
			g_error_free (error);
			return NULL;
		}
	}

	gtk_builder_connect_signals (xml, NULL);

	if (bluetooth_killswitch_has_killswitches (killswitch) != FALSE) {
		GObject *object;

		object = gtk_builder_get_object (xml, "killswitch-label");
		gtk_action_set_visible (GTK_ACTION (object), TRUE);
	}

	if (option_debug != FALSE) {
		GObject *object;

		object = gtk_builder_get_object (xml, "quit");
		gtk_action_set_visible (GTK_ACTION (object), TRUE);
	}

	object = gtk_builder_get_object (xml, "bluetooth-applet-ui-manager");
	devices_action_group = gtk_action_group_new ("devices-action-group");
	gtk_ui_manager_insert_action_group (GTK_UI_MANAGER (object),
					    devices_action_group, -1);

	action_set_bold (GTK_UI_MANAGER (object),
			 GTK_ACTION (gtk_builder_get_object (xml, "devices-label")),
			 "/bluetooth-applet-popup/devices-label");

	return GTK_WIDGET (gtk_builder_get_object (xml, "bluetooth-applet-popup"));
}

static void
update_menu_items (void)
{
	gboolean enabled;
	GObject *object;

	if (num_adapters_present == 0)
		enabled = FALSE;
	else
		enabled = (num_adapters_present - num_adapters_powered) <= 0;

	object = gtk_builder_get_object (xml, "adapter-action-group");
	gtk_action_group_set_visible (GTK_ACTION_GROUP (object), enabled);
	gtk_action_group_set_visible (devices_action_group, enabled);

	if (enabled == FALSE)
		return;

	gtk_action_group_set_sensitive (GTK_ACTION_GROUP (object), TRUE);
}

static void
update_icon_visibility (void)
{
	if (num_adapters_powered == 0)
		set_icon (FALSE);
	else
		set_icon (TRUE);

	if (show_icon_pref != FALSE) {
		if (num_adapters_present > 0 ||
		    bluetooth_killswitch_has_killswitches (killswitch) != FALSE) {
			show_icon ();
			return;
		}
	}
	hide_icon ();
}

static void
update_discoverability (GtkTreeIter *iter)
{
	gboolean discoverable;
	GObject *object;

	/* Avoid loops from changing the UI */
	if (discover_lock != FALSE)
		return;

	discover_lock = TRUE;

	object = gtk_builder_get_object (xml, "discoverable");

	if (iter == NULL) {
		discover_lock = FALSE;
		gtk_action_set_visible (GTK_ACTION (object), FALSE);
		return;
	}

	gtk_tree_model_get (devices_model, iter,
			    BLUETOOTH_COLUMN_DISCOVERABLE, &discoverable,
			    -1);

	gtk_action_set_visible (GTK_ACTION (object), TRUE);
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (object), discoverable);

	discover_lock = FALSE;
}

static void
set_device_status_label (const char *address, int connected)
{
	char *action_name, *action_path;
	GtkAction *status;
	GObject *object;
	const char *label;

	g_return_if_fail (address != NULL);

	switch (connected) {
	case DISCONNECTING:
		label = N_("Disconnecting...");
		break;
	case CONNECTING:
		label = N_("Connecting...");
		break;
	case CONNECTED:
		label = N_("Connected");
		break;
	case DISCONNECTED:
		label = N_("Disconnected");
		break;
	default:
		g_assert_not_reached ();
	}

	action_name = g_strdup_printf ("%s-status", address);
	status = gtk_action_group_get_action (devices_action_group, action_name);
	g_free (action_name);

	g_assert (status != NULL);
	gtk_action_set_label (status, _(label));

	object = gtk_builder_get_object (xml, "bluetooth-applet-ui-manager");
	action_path = g_strdup_printf ("/bluetooth-applet-popup/devices-placeholder/%s/%s-status",
				       address, address);
	action_set_bold (GTK_UI_MANAGER (object), status, action_path);
	g_free (action_path);
}

static void
connection_action_callback (BluetoothClient *_client,
			    gboolean success,
			    gpointer user_data)
{
	GtkAction *action = (GtkAction *) user_data;
	const char *address;
	int connected;

	/* Revert to the previous state and wait for the
	 * BluetoothClient to catch up */
	connected = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (action), "connected"));
	if (connected == DISCONNECTING)
		connected = CONNECTED;
	else if (connected == CONNECTING)
		connected = DISCONNECTED;
	else
		return;

	g_object_set_data_full (G_OBJECT (action),
				"connected", GINT_TO_POINTER (connected), NULL);
	address = g_object_get_data (G_OBJECT (action), "address");
	set_device_status_label (address, connected);
}

static void
on_connect_activate (GtkAction *action, gpointer data)
{
	int connected;
	const char *path;
	const char *address;

	connected = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (action), "connected"));
	/* Don't try connecting if something is already in progress */
	if (connected == CONNECTING || connected == DISCONNECTING)
		return;

	path = g_object_get_data (G_OBJECT (action), "device-path");

	if (connected == DISCONNECTED) {
		if (bluetooth_client_connect_service (client, path, connection_action_callback, action) != FALSE)
			connected = DISCONNECTING;
	} else if (connected == CONNECTED) {
		if (bluetooth_client_disconnect_service (client, path, connection_action_callback, action) != FALSE)
			connected = CONNECTING;
	} else {
		g_assert_not_reached ();
	}

	g_object_set_data_full (G_OBJECT (action),
				"connected", GINT_TO_POINTER (connected), NULL);

	address = g_object_get_data (G_OBJECT (action), "address");
	set_device_status_label (address, connected);
}

static void
action_set_bold (GtkUIManager *manager, GtkAction *action, const char *path)
{
	GtkWidget *widget;
	char *str;
	const char *label;

	/* That sucks, but otherwise the widget might not exist */
	gtk_ui_manager_ensure_update (manager);

	widget = gtk_ui_manager_get_widget (manager, path);
	g_assert (widget);

	label = gtk_action_get_label (action);
	str = g_strdup_printf ("<b>%s</b>", label);
	gtk_label_set_markup (GTK_LABEL (gtk_bin_get_child (GTK_BIN (widget))), str);
	g_free (str);
}

static void
remove_action_item (GtkAction *action, GtkUIManager *manager)
{
	guint menu_merge_id;
	GList *actions, *l;

	menu_merge_id = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (action), "merge-id"));
	gtk_ui_manager_remove_ui (GTK_UI_MANAGER (manager), menu_merge_id);
	actions = gtk_action_group_list_actions (devices_action_group);
	for (l = actions; l != NULL; l = l->next) {
		GtkAction *a = l->data;
		/* Don't remove the top-level action straight away */
		if (g_str_equal (gtk_action_get_name (a), gtk_action_get_name (action)) != FALSE)
			continue;
		/* But remove all the sub-actions for it */
		if (g_str_has_prefix (gtk_action_get_name (a), gtk_action_get_name (action)) != FALSE)
			gtk_action_group_remove_action (devices_action_group, a);
	}
	g_list_free (actions);
	gtk_action_group_remove_action (devices_action_group, action);
}

static char *
escape_label_for_action (const char *alias)
{
	char **tokens, *name;

	if (strchr (alias, '_') == NULL)
		return g_strdup (alias);

	tokens = g_strsplit (alias, "_", -1);
	name = g_strjoinv ("__", tokens);
	g_strfreev (tokens);

	return name;
}

static gboolean
device_has_uuid (const char **uuids, const char *uuid)
{
	guint i;

	if (uuids == NULL)
		return FALSE;

	for (i = 0; uuids[i] != NULL; i++) {
		if (g_str_equal (uuid, uuids[i]) != FALSE)
			return TRUE;
	}
	return FALSE;
}

static gboolean
device_has_submenu (const char **uuids, GHashTable *services, BluetoothType type)
{
	if (services != NULL)
		return TRUE;
	if (device_has_uuid (uuids, "OBEXObjectPush") != FALSE)
		return TRUE;
	if (device_has_uuid (uuids, "OBEXFileTransfer") != FALSE)
		return TRUE;
	if (type == BLUETOOTH_TYPE_KEYBOARD ||
	    type == BLUETOOTH_TYPE_MOUSE)
	    	return TRUE;
	return FALSE;
}

static GtkAction *
add_menu_item (const char *address,
	       const char *suffix,
	       const char *label,
	       GtkUIManager *uimanager,
	       guint merge_id,
	       GCallback callback)
{
	GtkAction *action;
	char *action_path, *action_name;

	g_return_val_if_fail (address != NULL, NULL);
	g_return_val_if_fail (suffix != NULL, NULL);
	g_return_val_if_fail (label != NULL, NULL);

	action_name = g_strdup_printf ("%s-%s", address, suffix);
	action = gtk_action_new (action_name, label, NULL, NULL);

	gtk_action_group_add_action (devices_action_group, action);
	g_object_unref (action);

	action_path = g_strdup_printf ("/bluetooth-applet-popup/devices-placeholder/%s", address);
	gtk_ui_manager_add_ui (uimanager, merge_id,
			       action_path, action_name, action_name,
			       GTK_UI_MANAGER_MENUITEM, FALSE);
	g_free (action_path);

	g_free (action_name);

	if (callback != NULL)
		g_signal_connect (G_OBJECT (action), "activate", callback, NULL);

	g_object_set_data_full (G_OBJECT (action),
				"address", g_strdup (address), g_free);

	return action;
}

static void
add_separator_item (const char *address,
		    const char *suffix,
		    GtkUIManager *uimanager,
		    guint merge_id)
{
	char *action_path, *action_name;

	g_return_if_fail (address != NULL);
	g_return_if_fail (suffix != NULL);

	action_name = g_strdup_printf ("%s-%s", address, suffix);
	action_path = g_strdup_printf ("/bluetooth-applet-popup/devices-placeholder/%s", address);

	gtk_ui_manager_add_ui (uimanager, merge_id,
			       action_path, action_name, NULL,
			       GTK_UI_MANAGER_SEPARATOR, FALSE);
	g_free (action_path);
	g_free (action_name);
}

static void
update_device_list (GtkTreeIter *parent)
{
	GtkUIManager *uimanager;
	GtkTreeIter iter;
	gboolean cont;
	guint num_devices;
	GList *actions, *l;

	num_devices = 0;

	uimanager = GTK_UI_MANAGER (gtk_builder_get_object (xml, "bluetooth-applet-ui-manager"));

	if (parent == NULL) {
		/* No default adapter? Remove everything */
		actions = gtk_action_group_list_actions (devices_action_group);
		g_list_foreach (actions, (GFunc) remove_action_item, uimanager);
		g_list_free (actions);
		goto done;
	}

	/* Get a list of actions, and we'll remove the ones with a
	 * device in the list. We remove the submenu items first */
	actions = gtk_action_group_list_actions (devices_action_group);
	for (l = actions; l != NULL; l = l->next) {
		if (bluetooth_verify_address (gtk_action_get_name (l->data)) == FALSE)
			l->data = NULL;
	}
	actions = g_list_remove_all (actions, NULL);

	cont = gtk_tree_model_iter_children (devices_model, &iter, parent);
	while (cont) {
		GHashTable *services;
		DBusGProxy *proxy;
		char *alias, *address, **uuids, *name;
		gboolean is_connected;
		BluetoothType type;
		GtkAction *action, *status, *oper;

		gtk_tree_model_get (devices_model, &iter,
				    BLUETOOTH_COLUMN_PROXY, &proxy,
				    BLUETOOTH_COLUMN_ADDRESS, &address,
				    BLUETOOTH_COLUMN_SERVICES, &services,
				    BLUETOOTH_COLUMN_ALIAS, &alias,
				    BLUETOOTH_COLUMN_UUIDS, &uuids,
				    BLUETOOTH_COLUMN_TYPE, &type,
				    -1);

		if (device_has_submenu ((const char **) uuids, services, type) == FALSE ||
		    address == NULL || proxy == NULL || alias == NULL) {
			if (proxy != NULL)
				g_object_unref (proxy);

			if (services != NULL)
				g_hash_table_unref (services);
			g_strfreev (uuids);
			g_free (alias);
			g_free (address);
			cont = gtk_tree_model_iter_next (devices_model, &iter);
			continue;
		}

		action = gtk_action_group_get_action (devices_action_group, address);
		oper = NULL;
		status = NULL;
		if (action) {
			char *action_name;

			actions = g_list_remove (actions, action);

			action_name = g_strdup_printf ("%s-status", address);
			status = gtk_action_group_get_action (devices_action_group, action_name);
			g_free (action_name);

			action_name = g_strdup_printf ("%s-action", address);
			oper = gtk_action_group_get_action (devices_action_group, action_name);
			g_free (action_name);
		}

		/* If one service is connected, then we're connected */
		is_connected = FALSE;
		if (services != NULL) {
			GList *list, *l;

			list = g_hash_table_get_values (services);
			for (l = list; l != NULL; l = l->next) {
				BluetoothStatus val = GPOINTER_TO_INT (l->data);
				if (val == BLUETOOTH_STATUS_CONNECTED ||
				    val == BLUETOOTH_STATUS_PLAYING) {
					is_connected = TRUE;
					break;
				}
			}
			g_list_free (list);
		}

		name = escape_label_for_action (alias);

		if (action == NULL) {
			guint menu_merge_id;
			char *action_path;

			/* The menu item with descendants */
			action = gtk_action_new (address, name, NULL, NULL);

			gtk_action_group_add_action (devices_action_group, action);
			g_object_unref (action);
			menu_merge_id = gtk_ui_manager_new_merge_id (uimanager);
			gtk_ui_manager_add_ui (uimanager, menu_merge_id,
					       "/bluetooth-applet-popup/devices-placeholder", address, address,
					       GTK_UI_MANAGER_MENU, FALSE);
			g_object_set_data_full (G_OBJECT (action),
						"merge-id", GUINT_TO_POINTER (menu_merge_id), NULL);

			/* The status menu item */
			status = add_menu_item (address,
						"status",
						is_connected ? _("Connected") : _("Disconnected"),
						uimanager,
						menu_merge_id,
						NULL);
			gtk_action_set_sensitive (status, FALSE);

			if (services != NULL) {
				action_path = g_strdup_printf ("/bluetooth-applet-popup/devices-placeholder/%s/%s-status",
							       address, address);
				action_set_bold (uimanager, status, action_path);
				g_free (action_path);
			} else {
				gtk_action_set_visible (status, FALSE);
			}

			/* The connect button */
			oper = add_menu_item (address,
					      "action",
					      is_connected ? _("Disconnect") : _("Connect"),
					      uimanager,
					      menu_merge_id,
					      G_CALLBACK (on_connect_activate));
			if (services == NULL)
				gtk_action_set_visible (oper, FALSE);

			add_separator_item (address, "connect-sep", uimanager, menu_merge_id);

			/* The Send to... button */
			if (device_has_uuid ((const char **) uuids, "OBEXObjectPush") != FALSE) {
				add_menu_item (address,
					       "sendto",
					       _("Send files..."),
					       uimanager,
					       menu_merge_id,
					       G_CALLBACK (sendto_callback));
				g_object_set_data_full (G_OBJECT (action),
							"alias", g_strdup (alias), g_free);
			}
			if (device_has_uuid ((const char **) uuids, "OBEXFileTransfer") != FALSE) {
				add_menu_item (address,
					       "browse",
					       _("Browse files..."),
					       uimanager,
					       menu_merge_id,
					       G_CALLBACK (browse_callback));
			}

			add_separator_item (address, "files-sep", uimanager, menu_merge_id);

			if (type == BLUETOOTH_TYPE_KEYBOARD && program_available (KEYBOARD_PREFS)) {
				add_menu_item (address,
					       "keyboard",
					       _("Open Keyboard Preferences..."),
					       uimanager,
					       menu_merge_id,
					       G_CALLBACK (keyboard_callback));
			}
			if (type == BLUETOOTH_TYPE_MOUSE && program_available (MOUSE_PREFS)) {
				add_menu_item (address,
					       "mouse",
					       _("Open Mouse Preferences..."),
					       uimanager,
					       menu_merge_id,
					       G_CALLBACK (mouse_callback));
			}
			if ((type == BLUETOOTH_TYPE_HEADSET ||
			     type == BLUETOOTH_TYPE_HEADPHONES ||
			     type == BLUETOOTH_TYPE_OTHER_AUDIO) && program_available (SOUND_PREFS)) {
				add_menu_item (address,
					       "sound",
					       _("Open Sound Preferences..."),
					       uimanager,
					       menu_merge_id,
					       G_CALLBACK (sound_callback));
			}
		} else {
			gtk_action_set_label (action, name);

			gtk_action_set_visible (status, services != NULL);
			gtk_action_set_visible (oper, services != NULL);
			if (services != NULL) {
				set_device_status_label (address, is_connected ? CONNECTED : DISCONNECTED);
				gtk_action_set_label (oper, is_connected ? _("Disconnect") : _("Connect"));
			}
		}
		g_free (name);

		if (oper != NULL) {
			g_object_set_data_full (G_OBJECT (oper),
						"connected", GINT_TO_POINTER (is_connected ? CONNECTED : DISCONNECTED), NULL);
			g_object_set_data_full (G_OBJECT (oper),
						"device-path", g_strdup (dbus_g_proxy_get_path (proxy)), g_free);
		}

		/* And now for the trick of the day */
		if (is_connected != FALSE) {
			char *path;

			path = g_strdup_printf ("/bluetooth-applet-popup/devices-placeholder/%s", address);
			action_set_bold (uimanager, action, path);
			g_free (path);
		}

		num_devices++;

		if (proxy != NULL)
			g_object_unref (proxy);

		if (services != NULL)
			g_hash_table_unref (services);
		g_strfreev (uuids);
		g_free (alias);
		g_free (address);
		cont = gtk_tree_model_iter_next (devices_model, &iter);
	}

	/* Remove the left-over devices */
	g_list_foreach (actions, (GFunc) remove_action_item, uimanager);
	g_list_free (actions);

done:
	gtk_ui_manager_ensure_update (uimanager);

	gtk_action_set_visible (GTK_ACTION (gtk_builder_get_object (xml, "devices-label")),
				num_devices > 0);
}

static void device_changed (GtkTreeModel *model,
			     GtkTreePath  *path,
			     GtkTreeIter  *_iter,
			     gpointer      data)
{
	GtkTreeIter iter, *default_iter;
	gboolean powered, cont;

	default_iter = NULL;
	num_adapters_present = num_adapters_powered = 0;

	cont = gtk_tree_model_get_iter_first (model, &iter);
	while (cont) {
		gboolean is_default;

		num_adapters_present++;

		gtk_tree_model_get (model, &iter,
				    BLUETOOTH_COLUMN_DEFAULT, &is_default,
				    BLUETOOTH_COLUMN_POWERED, &powered,
				    -1);
		if (powered)
			num_adapters_powered++;
		if (is_default && powered)
			default_iter = gtk_tree_iter_copy (&iter);

		cont = gtk_tree_model_iter_next (model, &iter);
	}

	update_discoverability (default_iter);
	update_icon_visibility ();
	update_menu_items ();
	update_device_list (default_iter);

	if (default_iter != NULL)
		gtk_tree_iter_free (default_iter);
}

static void device_added(GtkTreeModel *model,
			  GtkTreePath *path,
			  GtkTreeIter *iter,
			  gpointer user_data)
{
	device_changed (model, path, iter, user_data);
}

static void device_removed(GtkTreeModel *model,
			    GtkTreePath *path,
			    gpointer user_data)
{
	device_changed (model, path, NULL, user_data);
}

static void
show_icon_changed (GSettings *settings,
		   const char *key,
		   gpointer   user_data)
{
	show_icon_pref = g_settings_get_boolean (settings, PREF_SHOW_ICON);
	update_icon_visibility();
}

static GOptionEntry options[] = {
	{ "debug", 'd', 0, G_OPTION_ARG_NONE, &option_debug, N_("Debug"), NULL },
	{ NULL },
};

int main(int argc, char *argv[])
{
	UniqueApp *app;
	GtkStatusIcon *statusicon;
	GtkWidget *menu;
	GOptionContext *context;
	GError *error = NULL;

	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

	g_type_init ();

	/* Parse command-line options */
	context = g_option_context_new (N_("- Bluetooth applet"));
	g_option_context_set_translation_domain (context, GETTEXT_PACKAGE);
	g_option_context_add_main_entries (context, options, GETTEXT_PACKAGE);
	g_option_context_add_group (context, gtk_get_option_group (TRUE));
	if (g_option_context_parse (context, &argc, &argv, &error) == FALSE) {
		g_print (_("%s\nRun '%s --help' to see a full list of available command line options.\n"),
			 error->message, argv[0]);
		g_error_free (error);
		return 1;
	}

	if (option_debug == FALSE) {
		app = unique_app_new ("org.mate.Bluetooth.applet", NULL);
		if (unique_app_is_running (app)) {
			gdk_notify_startup_complete ();
			g_warning ("Applet is already running, exiting");
			return 0;
		}
	} else {
		app = NULL;
	}

	g_set_application_name(_("Bluetooth Applet"));

	gtk_window_set_default_icon_name("bluetooth");

	killswitch = bluetooth_killswitch_new ();
	g_signal_connect (G_OBJECT (killswitch), "state-changed",
			  G_CALLBACK (killswitch_state_changed), NULL);

	menu = create_popupmenu();

	client = bluetooth_client_new();

	devices_model = bluetooth_client_get_model(client);

	g_signal_connect(G_OBJECT(devices_model), "row-inserted",
			 G_CALLBACK(device_added), NULL);
	g_signal_connect(G_OBJECT(devices_model), "row-deleted",
			 G_CALLBACK(device_removed), NULL);
	g_signal_connect (G_OBJECT (devices_model), "row-changed",
			  G_CALLBACK (device_changed), NULL);

	/* Set the default adapter */
	device_changed (devices_model, NULL, NULL, NULL);
	if (bluetooth_killswitch_has_killswitches (killswitch) != FALSE) {
		killswitch_state_changed (killswitch,
					  bluetooth_killswitch_get_state (killswitch));
	}

	/* Make sure all the unblocked adapters are powered,
	 * so as to avoid seeing unpowered, but unblocked
	 * devices */
	bluetooth_set_adapter_powered ();

	settings = g_settings_new (SCHEMA_NAME);
	show_icon_pref = g_settings_get_boolean (settings, PREF_SHOW_ICON);

	g_signal_connect (G_OBJECT (settings), "changed::" PREF_SHOW_ICON,
			  G_CALLBACK (show_icon_changed), NULL);

	statusicon = init_notification();

	update_icon_visibility();

	g_signal_connect(statusicon, "activate",
				G_CALLBACK(activate_callback), menu);
	g_signal_connect(statusicon, "popup-menu",
				G_CALLBACK(popup_callback), menu);

	setup_agents();

	gtk_main();

	gtk_widget_destroy(menu);

	g_object_unref(settings);

	cleanup_agents();

	cleanup_notification();

	g_object_unref(devices_model);

	g_object_unref(client);

	if (app != NULL)
		g_object_unref (app);

	return 0;
}
