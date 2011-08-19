/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2009  Bastien Nocera <hadess@hadess.net>
 *  Copyright (C) 2009, Intel Corporation.
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Written by: Joshua Lock <josh@linux.intel.com>
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <math.h>
#include <gdk/gdkkeysyms.h>
#include <mx/mx-gtk.h>

#include "bluetooth-client.h"
#include "bluetooth-client-private.h"
#include "bluetooth-chooser.h"
#include "bluetooth-chooser-private.h"
#include "bluetooth-killswitch.h"
#include "bluetooth-plugin-manager.h"
#include "bluetooth-filter-widget.h"
#include "bluetooth-agent.h"
#include "mate-bluetooth-enum-types.h"

#include "pin.h"

#include "mux-cell-renderer-text.h"
#include "koto-cell-renderer-pixbuf.h"

#include "moblin-panel.h"

G_DEFINE_TYPE (MoblinPanel, moblin_panel, GTK_TYPE_HBOX)

#define MOBLIN_PANEL_GET_PRIVATE(o)                                         \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), MOBLIN_TYPE_PANEL, MoblinPanelPrivate))

struct _MoblinPanelPrivate
{
	/* Bluetooth objects for interacting with BlueZ */
	BluetoothKillswitch *killswitch;
	BluetoothClient *client;
	BluetoothAgent *agent;
	GtkTreeModel *chooser_model;

	/* Page widgets that need to be "globally" accessible */
	GtkWidget *power_switch;
	GtkWidget *notebook;
	GtkWidget *label_pin_help;
	GtkWidget *label_pin;
	GtkWidget *label_ssp_pin_help;
	GtkWidget *label_ssp_pin;
	GtkWidget *summary_frame;
	GtkWidget *extra_config_vbox;
	GtkWidget *label_failure;
	GtkWidget *chooser;
	GtkWidget *display;
	GtkWidget *send_button;
	GtkWidget *add_new_button;
	/* Widgets for use in lazy-loading the pin options dialog from the GtkBuilder UI file */
	GtkWidget *matches_button;
	GtkWidget *does_not_match_button;
	GtkWidget *spinner;

	/* Widgets for use in lazy-loading the pin options dialog from the GtkBuilder UI file */
	GtkWidget *pin_dialog;
	GtkWidget *entry_custom;
	GtkWidget *radio_auto;
	GtkWidget *radio_0000;
	GtkWidget *radio_1111;
	GtkWidget *radio_1234;
	GtkWidget *radio_custom;

	/* State tracking variables */
	gchar *pincode;
	gchar *user_pincode;
	gchar *target_pincode;
	gchar *target_name;
	gchar *target_address;
	gboolean target_ssp;
	gboolean automatic_pincode;
	gboolean create_started;
	gboolean complete;
	BluetoothType target_type;
	gboolean connecting;
	gboolean display_called;
};

#define CONNECT_TIMEOUT 3.0
#define AGENT_PATH "/org/bluez/agent/moblin"

typedef struct {
	char *path;
	GTimer *timer;
	MoblinPanel *self;
} ConnectData;

typedef enum {
	PAGE_DEVICES,
	PAGE_ADD,
	PAGE_SETUP,
	PAGE_SSP_SETUP,
	PAGE_CONNECTING,
	PAGE_SUMMARY,
	PAGE_FAILURE
} MoblinPages;

enum {
	STATUS_CONNECTING,
        REQUEST_FOCUS,
	LAST_SIGNAL
};

static guint _signals[LAST_SIGNAL] = {0, };

/* Forward declarations of methods */
static void set_current_page (MoblinPanel *self, MoblinPages page);
static void create_callback (BluetoothClient *client, const gchar *path, const GError *error, gpointer user_data);
static void update_random_pincode (MoblinPanel *self);
static void create_selected_device (MoblinPanel *self);

static void
power_switch_toggled_cb (MxGtkLightSwitch *light_switch,
			 gboolean 	     state,
                         gpointer            user_data)
{
	MoblinPanelPrivate *priv = MOBLIN_PANEL_GET_PRIVATE(user_data);

	if (state == TRUE) {
		bluetooth_killswitch_set_state (priv->killswitch, KILLSWITCH_STATE_UNBLOCKED);
	} else {
		bluetooth_killswitch_set_state (priv->killswitch, KILLSWITCH_STATE_SOFT_BLOCKED);
	}
}

static void
enable_send_file (MoblinPanel *self)
{
	MoblinPanelPrivate *priv = MOBLIN_PANEL_GET_PRIVATE (self);
	BluetoothChooser *chooser = BLUETOOTH_CHOOSER (priv->display);
	GValue value = {0, };
	gchar *name = NULL;
	gboolean enabled = FALSE;

	name = bluetooth_chooser_get_selected_device_name (chooser);
	if (name != NULL) {
		guint i;
		const char **uuids;

		g_free (name);
		bluetooth_chooser_get_selected_device_info (chooser, "uuids", &value);

		uuids = (const char **) g_value_get_boxed (&value);
		if (uuids != NULL) {
			for (i = 0; uuids[i] != NULL; i++)
				if (g_str_equal (uuids[i], "OBEXObjectPush")) {
					enabled = TRUE;
					break;
				}
			g_value_unset (&value);
		}
	}
	gtk_widget_set_sensitive (priv->send_button, enabled);
}

static void
killswitch_state_changed_cb (BluetoothKillswitch *killswitch,
                             KillswitchState      state,
                             gpointer             user_data)
{
	MoblinPanel *self = MOBLIN_PANEL (user_data);
	MoblinPanelPrivate *priv = MOBLIN_PANEL_GET_PRIVATE (self);

	g_signal_handlers_block_by_func (priv->power_switch, power_switch_toggled_cb, user_data);

	if (state == KILLSWITCH_STATE_SOFT_BLOCKED) {
		mx_gtk_light_switch_set_active (MX_GTK_LIGHT_SWITCH (priv->power_switch),
	                                      FALSE);
		gtk_widget_set_sensitive (priv->power_switch, TRUE);
		gtk_widget_set_sensitive (priv->add_new_button, FALSE);
		gtk_widget_set_sensitive (priv->send_button, FALSE);
	} else if (state == KILLSWITCH_STATE_UNBLOCKED) {
		mx_gtk_light_switch_set_active (MX_GTK_LIGHT_SWITCH (priv->power_switch), TRUE);
		gtk_widget_set_sensitive (priv->power_switch, TRUE);
		gtk_widget_set_sensitive (priv->add_new_button, TRUE);
		enable_send_file (self);
	} else if (state == KILLSWITCH_STATE_HARD_BLOCKED || state == KILLSWITCH_STATE_NO_ADAPTER) {
		gtk_widget_set_sensitive (priv->power_switch, FALSE);
		gtk_widget_set_sensitive (priv->add_new_button, FALSE);
		gtk_widget_set_sensitive (priv->send_button, FALSE);
	} else {
		g_assert_not_reached ();
	}

	g_signal_handlers_unblock_by_func (priv->power_switch, power_switch_toggled_cb, user_data);
}

static void
selected_device_changed_cb (BluetoothChooser *chooser, const char *address, gpointer user_data)
{
	MoblinPanel *self = MOBLIN_PANEL (user_data);
	enable_send_file (MOBLIN_PANEL (self));
}

static void
set_frame_title (GtkFrame *frame, gchar *title)
{
	GtkWidget *frame_title;
	gchar *label = NULL;

	label = g_strdup_printf ("<span font_desc=\"Liberation Sans Bold 18px\""
	                           "foreground=\"#3e3e3e\">%s</span>", title);
	frame_title = gtk_frame_get_label_widget (frame);
	gtk_label_set_markup (GTK_LABEL (frame_title), label);
	g_free (label);
	gtk_widget_show (frame_title);
}

static void
send_file_button_clicked_cb (GtkButton *button,
                             gpointer   user_data)
{
	MoblinPanelPrivate *priv = MOBLIN_PANEL_GET_PRIVATE (user_data);
	BluetoothChooser *chooser = BLUETOOTH_CHOOSER (priv->display);
	GPtrArray *a;
	GError *err = NULL;
	guint i;
	char *address, *name;

	address = bluetooth_chooser_get_selected_device (chooser);
	name = bluetooth_chooser_get_selected_device_name (chooser);

	a = g_ptr_array_new ();
	g_ptr_array_add (a, "bluetooth-sendto");
	if (address != NULL) {
		char *s;

		s = g_strdup_printf ("--device=\"%s\"", address);
		g_ptr_array_add (a, s);
	}
	if (address != NULL && name != NULL) {
		char *s;

		s = g_strdup_printf ("--name=\"%s\"", name);
		g_ptr_array_add (a, s);
	}
	g_ptr_array_add (a, NULL);

	g_free (address);
	g_free (name);

	if (g_spawn_async(NULL, (char **) a->pdata, NULL,
			  G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &err) == FALSE) {
		g_printerr("Couldn't execute command: %s\n", err->message);
		g_error_free (err);
	}

	for (i = 1; a->pdata[i] != NULL; i++)
		g_free (a->pdata[i]);

	g_ptr_array_free (a, TRUE);
}

static gboolean
entry_custom_event (GtkWidget *entry, GdkEventKey *event)
{
	if (event->length == 0)
		return FALSE;

	if ((event->keyval >= GDK_0 && event->keyval <= GDK_9) ||
	    (event->keyval >= GDK_KP_0 && event->keyval <= GDK_KP_9))
		return FALSE;

	return TRUE;
}

static void
entry_custom_changed (GtkWidget *entry, gpointer user_data)
{
	MoblinPanelPrivate *priv = MOBLIN_PANEL_GET_PRIVATE (user_data);
	priv->user_pincode = g_strdup (gtk_entry_get_text(GTK_ENTRY(entry)));
	gtk_dialog_set_response_sensitive (GTK_DIALOG (priv->pin_dialog),
					   GTK_RESPONSE_ACCEPT,
					   gtk_entry_get_text_length (GTK_ENTRY (entry)) >= 1);
}

static void
toggle_set_sensitive (GtkWidget *button, gpointer user_data)
{
	MoblinPanelPrivate *priv = MOBLIN_PANEL_GET_PRIVATE (user_data);
	gboolean active;

	active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
	gtk_widget_set_sensitive(priv->entry_custom, active);
	/* When selecting another PIN, make sure the "Close" button is sensitive */
	if (!active)
		gtk_dialog_set_response_sensitive (GTK_DIALOG (priv->pin_dialog),
						   GTK_RESPONSE_ACCEPT, TRUE);
}

static void
set_user_pincode (GtkWidget *button, gpointer user_data)
{
	MoblinPanelPrivate *priv = MOBLIN_PANEL_GET_PRIVATE (user_data);
	GSList *list, *l;

	list = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
	for (l = list; l ; l = l->next) {
		GtkEntry *entry;
		GtkWidget *radio;
		const char *pin;

		if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)))
			continue;

		/* Is it radio_fixed that changed? */
		radio = g_object_get_data (G_OBJECT (button), "button");
		if (radio != NULL) {
			set_user_pincode (radio, user_data);
			return;
		}

		pin = g_object_get_data (G_OBJECT (button), "pin");
		entry = g_object_get_data (G_OBJECT (button), "entry");

		if (entry != NULL) {
			g_free (priv->user_pincode);
			priv->user_pincode = g_strdup (gtk_entry_get_text(entry));
			gtk_dialog_set_response_sensitive (GTK_DIALOG (priv->pin_dialog),
							   GTK_RESPONSE_ACCEPT,
							   gtk_entry_get_text_length (entry) >= 1);
		} else if (pin != NULL) {
			g_free (priv->user_pincode);
			if (*pin == '\0')
				priv->user_pincode = NULL;
			else
				priv->user_pincode = g_strdup (pin);
		}

		break;
	}
}

static void
pin_options_button_clicked_cb (GtkButton *button,
                             gpointer   user_data)
{
	MoblinPanel *self = MOBLIN_PANEL (user_data);
	MoblinPanelPrivate *priv = MOBLIN_PANEL_GET_PRIVATE (self);
	GtkWidget *radio;
	gchar *oldpin;
	GtkBuilder *builder;
	GError *err = NULL;

	if (priv->pin_dialog == NULL) {
		builder = gtk_builder_new ();
		if (gtk_builder_add_from_file (builder, "pin.ui", NULL) == 0) {
			if (gtk_builder_add_from_file (builder, PKGDATADIR "/pin.ui", &err) == 0) {
				g_warning ("Could not load PIN dialog UI form %s: %s",
					PKGDATADIR "/pin.ui", err->message);
				g_error_free (err);
			}
		}

		priv->pin_dialog = GTK_WIDGET (gtk_builder_get_object (builder, "pin_dialog"));
		priv->radio_auto  = GTK_WIDGET (gtk_builder_get_object (builder, "radio_auto"));
		g_signal_connect (priv->radio_auto, "toggled", G_CALLBACK (set_user_pincode), self);
		priv->radio_0000 = GTK_WIDGET (gtk_builder_get_object (builder, "radio_0000"));
		g_signal_connect (priv->radio_0000, "toggled", G_CALLBACK (set_user_pincode), self);
		priv->radio_1111 = GTK_WIDGET (gtk_builder_get_object (builder, "radio_1111"));
		g_signal_connect (priv->radio_1111, "toggled", G_CALLBACK (set_user_pincode), self);
		priv->radio_1234 = GTK_WIDGET (gtk_builder_get_object (builder, "radio_1234"));
		g_signal_connect (priv->radio_1234, "toggled", G_CALLBACK (set_user_pincode), self);
		priv->radio_custom = GTK_WIDGET (gtk_builder_get_object (builder, "radio_custom"));
		g_signal_connect (priv->radio_custom, "toggled", G_CALLBACK (set_user_pincode), self);
		g_signal_connect (priv->radio_custom, "toggled", G_CALLBACK (toggle_set_sensitive), self);
		priv->entry_custom = GTK_WIDGET (gtk_builder_get_object (builder, "entry_custom"));
		g_signal_connect (priv->entry_custom, "key-press-event", G_CALLBACK (entry_custom_event), self);
		g_signal_connect (priv->entry_custom, "changed", G_CALLBACK (entry_custom_changed), self);

		g_object_set_data (G_OBJECT (priv->radio_auto), "pin", "");
		g_object_set_data (G_OBJECT (priv->radio_0000), "pin", "0000");
		g_object_set_data (G_OBJECT (priv->radio_1111), "pin", "1111");
		g_object_set_data (G_OBJECT (priv->radio_1234), "pin", "1234");
		g_object_set_data (G_OBJECT (priv->radio_custom), "entry", priv->entry_custom);

		gtk_dialog_add_button (GTK_DIALOG (priv->pin_dialog), GTK_STOCK_OK, GTK_RESPONSE_ACCEPT);
	}

	oldpin = priv->user_pincode;
	priv->user_pincode = NULL;

	gtk_window_present (GTK_WINDOW (priv->pin_dialog));

	if (oldpin == NULL)
		radio = priv->radio_auto;
	else if (g_str_equal (oldpin, "0000"))
		radio = priv->radio_0000;
	else if (g_str_equal (oldpin, "1111"))
		radio = priv->radio_1111;
	else if (g_str_equal (oldpin, "1234"))
		radio = priv->radio_1234;
	else
		radio = priv->radio_custom;

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio), TRUE);
	if (radio == priv->radio_custom)
		gtk_entry_set_text (GTK_ENTRY (priv->entry_custom), oldpin);

	if (gtk_dialog_run (GTK_DIALOG (priv->pin_dialog)) != GTK_RESPONSE_ACCEPT) {
		g_free (priv->user_pincode);
		priv->user_pincode = oldpin;
	} else {
		g_free (oldpin);
	}

	gtk_widget_hide (priv->pin_dialog);

        g_signal_emit (self, _signals[REQUEST_FOCUS], 0);
}

/*
 * This helper function forces the currently selected row in the chooser to be
 * the same row which contains the activated cell.
 */
static void
ensure_selection (BluetoothChooser *chooser, const gchar *path)
{
	GtkTreeView *view;
	GtkTreePath *tree_path;
	view = GTK_TREE_VIEW (bluetooth_chooser_get_treeview (chooser));

	/* Set selection */
	tree_path = gtk_tree_path_new_from_string (path);
	gtk_tree_view_set_cursor (view, tree_path, NULL, FALSE);
	gtk_tree_path_free (tree_path);
}

static void
remove_clicked_cb (GtkCellRenderer *cell, const gchar *path, gpointer user_data)
{
	MoblinPanelPrivate *priv = MOBLIN_PANEL_GET_PRIVATE (user_data);
	BluetoothChooser *chooser = BLUETOOTH_CHOOSER (priv->display);
	const gchar *address = NULL;
	GValue value = { 0, };

	ensure_selection (chooser, path);

	/* Get address */
	g_assert (bluetooth_chooser_get_selected_device_info (chooser, "address", &value));
	address = g_value_get_string (&value);

	if (bluetooth_chooser_remove_selected_device (chooser) != FALSE && address) {
		bluetooth_plugin_manager_device_deleted (address);
	}

	g_value_unset (&value);
}

static void
browse_clicked (GtkCellRenderer *renderer, const gchar *path, gpointer user_data)
{
	MoblinPanelPrivate *priv = MOBLIN_PANEL_GET_PRIVATE (user_data);
	BluetoothChooser *chooser = BLUETOOTH_CHOOSER (priv->display);
	const gchar *address = NULL;
	GValue value = { 0, };
	gchar *cmd;

	ensure_selection (chooser, path);

	/* Get address */
	if (bluetooth_chooser_get_selected_device_info (chooser, "address", &value)) {
		address = g_value_get_string (&value);
	}

	if (address == NULL) {
		cmd = g_strdup_printf ("%s --no-default-window \"obex://[%s]\"",
				       "caja", address);
		if (!g_spawn_command_line_async (cmd, NULL))
			g_printerr("Couldn't execute command: %s\n", cmd);
		g_free (cmd);
	}

	g_value_unset (&value);
}

static void
connect_callback (BluetoothClient *client, gboolean success, gpointer user_data)
{
	ConnectData *data = (ConnectData *)user_data;

	if (success == FALSE && g_timer_elapsed (data->timer, NULL) < CONNECT_TIMEOUT) {
		if (bluetooth_client_connect_service (client, data->path, connect_callback, data) != FALSE)
			return;
	}

	if (success == FALSE)
		g_message ("Failed to connect to device %s", data->path);

	g_timer_destroy (data->timer);
	g_free (data->path);
	g_object_unref (data->self);
	g_free (data);
}

static void
set_failure_message (MoblinPanel *self, gchar *device)
{
	MoblinPanelPrivate *priv = MOBLIN_PANEL_GET_PRIVATE (self);
	gchar *str;

	str = g_strdup_printf (_("Pairing with %s failed."), device);
	gtk_label_set_text (GTK_LABEL (priv->label_failure), str);
}

static void
set_ssp_pin_message (MoblinPanel *self, gchar *msg)
{
	MoblinPanelPrivate *priv = MOBLIN_PANEL_GET_PRIVATE (self);

	gtk_label_set_text (GTK_LABEL (priv->label_ssp_pin_help), msg);
}

static void
set_ssp_pin_label (MoblinPanel *self, gchar *pin)
{
	MoblinPanelPrivate *priv = MOBLIN_PANEL_GET_PRIVATE (self);
	gchar *str;

	str = g_strdup_printf ("<span font_desc=\"50\" color=\"black\" bgcolor=\"white\">  %s  </span>", pin);

	gtk_label_set_markup (GTK_LABEL (priv->label_ssp_pin), str);

	g_free (str);
}

static void
set_pin_message (MoblinPanel *self, gchar *msg)
{
	MoblinPanelPrivate *priv = MOBLIN_PANEL_GET_PRIVATE (self);

	gtk_label_set_text (GTK_LABEL (priv->label_pin_help), msg);
}

static void
set_pin_label (MoblinPanel *self, gchar *pin)
{
	MoblinPanelPrivate *priv = MOBLIN_PANEL_GET_PRIVATE (self);
	gchar *str;

	str = g_strdup_printf ("<span font_desc=\"50\" color=\"black\" bgcolor=\"white\">  %s  </span>", pin);

	gtk_label_set_markup (GTK_LABEL (priv->label_pin), str);

	g_free (str);
}

static gboolean
cancel_callback (DBusGMethodInvocation *context, gpointer user_data)
{
	MoblinPanel *self = MOBLIN_PANEL (user_data);
	MoblinPanelPrivate *priv = MOBLIN_PANEL_GET_PRIVATE (self);

	priv->create_started  = FALSE;
	set_current_page (self, PAGE_FAILURE);

	set_failure_message (self, priv->target_name);

	dbus_g_method_return(context);

	return TRUE;
}

static void
connect_device (const gchar *device_path, MoblinPanel *self)
{
	MoblinPanelPrivate *priv = MOBLIN_PANEL_GET_PRIVATE (self);
	ConnectData *data;

	data = g_new0 (ConnectData, 1);
	data->path = g_strdup (device_path);
	data->timer = g_timer_new ();
	data->self = g_object_ref (self);

	if (bluetooth_client_connect_service (priv->client, device_path, connect_callback, data) == FALSE) {
		g_timer_destroy (data->timer);
		g_free (data->path);
		g_object_unref (data->self);
		g_free (data);
	}
}

static void
container_remove_all (GtkContainer *container)
{
	GList *list;
	GList *iter;

	list = gtk_container_get_children (container);
	for (iter = list; iter; iter = iter->next)
		gtk_container_remove (container, GTK_WIDGET (iter->data));

	g_list_free (list);
}

static void
set_current_page (MoblinPanel *self, MoblinPages page)
{
	MoblinPanelPrivate *priv = MOBLIN_PANEL_GET_PRIVATE (self);

	if (page == PAGE_ADD) {
		g_object_set (BLUETOOTH_CHOOSER (priv->chooser),
			      "device-category-filter", BLUETOOTH_CATEGORY_NOT_PAIRED_OR_TRUSTED,
			      NULL);

		bluetooth_chooser_start_discovery (BLUETOOTH_CHOOSER (priv->chooser));
	} else {
		bluetooth_chooser_stop_discovery (BLUETOOTH_CHOOSER (priv->chooser));
	}

	if (page == PAGE_CONNECTING)
		gtk_spinner_start (GTK_SPINNER (priv->spinner));
	else
		gtk_spinner_stop (GTK_SPINNER (priv->spinner));

	if ((page == PAGE_SETUP || page == PAGE_SSP_SETUP || page == PAGE_CONNECTING) && (priv->create_started == FALSE)) {
		create_selected_device (self);
	}

	if (page == PAGE_SETUP) {
		priv->complete = FALSE;

		if (priv->automatic_pincode == FALSE && priv->target_ssp == FALSE) {
			gchar *text;

			if (priv->target_type == BLUETOOTH_TYPE_KEYBOARD) {
				text = g_strdup_printf (_("Please enter the following PIN on '%s' and press “Enter” on the keyboard:"), priv->target_name);
			} else {
				text = g_strdup_printf (_("Please enter the following PIN on '%s':"), priv->target_name);
			}

			set_pin_message (self, text);
			set_pin_label (self, priv->pincode);
		} else {
			g_assert_not_reached ();
		}
	}

	if (page == PAGE_SSP_SETUP && priv->display_called == FALSE) {
		priv->complete = FALSE;
		gtk_widget_show (priv->matches_button);
		gtk_widget_show (priv->does_not_match_button);
	} else {
		gtk_widget_hide (priv->matches_button);
		gtk_widget_hide (priv->does_not_match_button);
	}

	if (page == PAGE_SUMMARY) {
		GValue value = { 0, };
		const char **uuids;
		GList *widgets;
		char *title;
		gboolean have_additional_widgets = FALSE;

		title = g_strdup_printf (_("Successfully set up new device '%s'"), priv->target_name);
		set_frame_title (GTK_FRAME (priv->summary_frame), title);
		g_free (title);

		if (bluetooth_chooser_get_selected_device_info (BLUETOOTH_CHOOSER (priv->chooser), "uuids", &value))
			uuids = g_value_get_boxed (&value);
		else
			uuids = NULL;

		widgets = bluetooth_plugin_manager_get_widgets (priv->target_address, uuids);
		if (widgets) {
			GList *iter;

			for (iter = widgets; iter; iter = iter->next)
				gtk_box_pack_start (GTK_BOX (priv->extra_config_vbox),
						    GTK_WIDGET (iter->data),
						    FALSE,
						    TRUE,
						    0);

			g_list_free (widgets);
			gtk_widget_show_all (priv->extra_config_vbox);
			have_additional_widgets = TRUE;
		}

		g_value_unset (&value);

		if (!have_additional_widgets)
			/* Show the devices page if we don't have any extra information to present. */
			page = PAGE_DEVICES;
	}

	if (page == PAGE_DEVICES) {
		/* Clean up old state */
		update_random_pincode (self);
		priv->target_ssp = FALSE;
		priv->target_type = BLUETOOTH_TYPE_ANY;
		priv->display_called = FALSE;
		g_free (priv->target_address);
		priv->target_address = NULL;
		g_free (priv->target_name);
		priv->target_name = NULL;
		container_remove_all (GTK_CONTAINER (priv->extra_config_vbox));
		g_object_set (priv->chooser, "device-type-filter", BLUETOOTH_TYPE_ANY, NULL);
	}

	gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook), page);
}

static void
create_callback (BluetoothClient *client, const gchar *path, const GError *error, gpointer user_data)
{
	MoblinPanel *self = MOBLIN_PANEL (user_data);
	MoblinPanelPrivate *priv = MOBLIN_PANEL_GET_PRIVATE (self);
	gchar *device_name;

	priv->create_started = FALSE;

	if (path == NULL) {
		device_name = bluetooth_chooser_get_selected_device_name (BLUETOOTH_CHOOSER (priv->chooser));
		set_failure_message (self, device_name);
		g_free (device_name);
		set_current_page (self, PAGE_FAILURE);
		return;
	}

	bluetooth_client_set_trusted (client, path, TRUE);

	connect_device (path, self);

	set_current_page (self, PAGE_SUMMARY);
}

static void
create_selected_device (MoblinPanel *self)
{
	MoblinPanelPrivate *priv = MOBLIN_PANEL_GET_PRIVATE (self);
	const gchar *path = AGENT_PATH;
	gchar *pin_ret;

	/* Set the filter on the selector, so we can use it to get more
	 * info later, in page_summary */
	g_object_set (priv->chooser,
		      "device-category-filter", BLUETOOTH_CATEGORY_ALL,
		      NULL);

	pin_ret = get_pincode_for_device (priv->target_type, priv->target_address,
					priv->target_name, NULL);
	if (pin_ret != NULL && g_str_equal (pin_ret, "NULL"))
		path = NULL;
	g_free (pin_ret);

	g_object_ref (priv->agent);
	bluetooth_client_create_device (priv->client, priv->target_address, path,
					create_callback, self);
	priv->create_started = TRUE;
}

static void
pair_clicked (GtkCellRenderer *renderer, const gchar *path, gpointer user_data)
{
	MoblinPanel *self = MOBLIN_PANEL (user_data);
	MoblinPanelPrivate *priv = MOBLIN_PANEL_GET_PRIVATE (self);
	GValue value = { 0, };
	gchar *name;
	gchar *address = NULL;
	guint legacy_pairing;
	BluetoothType type;
	BluetoothChooser *chooser = BLUETOOTH_CHOOSER (priv->chooser);

	ensure_selection (BLUETOOTH_CHOOSER (priv->chooser), path);

	name = bluetooth_chooser_get_selected_device_name (chooser);
	type = bluetooth_chooser_get_selected_device_type (chooser);
	if (bluetooth_chooser_get_selected_device_info (chooser, "address", &value) != FALSE) {
		address = g_value_dup_string (&value);
		g_value_unset (&value);
	}
	if (bluetooth_chooser_get_selected_device_info (chooser, "legacypairing", &value) != FALSE) {
		legacy_pairing = g_value_get_int (&value);
		g_value_unset (&value);
			if (legacy_pairing == -1)
				legacy_pairing = TRUE;
	} else {
		legacy_pairing = TRUE;
	}

	g_free (priv->target_address);
	priv->target_address = address;

	g_free (priv->target_name);
	priv->target_name = name;

	priv->target_type = type;
	priv->target_ssp = !legacy_pairing;
	priv->automatic_pincode = FALSE;

	g_free (priv->pincode);
	priv->pincode = NULL;

	if (priv->user_pincode != NULL && *priv->user_pincode != '\0') {
		priv->pincode = g_strdup (priv->user_pincode);
		priv->automatic_pincode = TRUE;
	} else if (address != NULL) {
		guint max_digits;

		priv->pincode = get_pincode_for_device (priv->target_type, priv->target_address,
							priv->target_name, &max_digits);
		if (priv->pincode == NULL) {
			/* Truncate the default pincode if the device doesn't like long
			 * PIN codes */
			if (max_digits != PIN_NUM_DIGITS && max_digits > 0)
				priv->pincode = g_strndup(priv->target_pincode, max_digits);
			else
				priv->pincode = g_strdup(priv->target_pincode);
		} else if (priv->target_ssp == FALSE) {
			priv->automatic_pincode = TRUE;
		}
	}

	if (priv->target_ssp != FALSE || priv->automatic_pincode != FALSE) {
		set_current_page (self, PAGE_CONNECTING);
	} else {
		set_current_page (self, PAGE_SETUP);
	}
}

static void
connect_clicked (GtkCellRenderer *renderer, const gchar *path, gpointer user_data)
{
	MoblinPanel *self = MOBLIN_PANEL (user_data);
	MoblinPanelPrivate *priv = MOBLIN_PANEL_GET_PRIVATE (self);
	GValue value = { 0, };
	DBusGProxy *device;
	const gchar *device_path = NULL;

	ensure_selection (BLUETOOTH_CHOOSER (priv->display), path);

	bluetooth_chooser_get_selected_device_info (BLUETOOTH_CHOOSER (priv->display), "proxy", &value);
	device = g_value_get_object (&value);
	device_path = dbus_g_proxy_get_path (device);
	g_value_unset (&value);

	connect_device (device_path, self);
}

static void
remove_to_icon (GtkTreeViewColumn *column, GtkCellRenderer *cell,
		GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	gboolean paired, trusted;
	gtk_tree_model_get (model, iter, BLUETOOTH_COLUMN_PAIRED, &paired,
			BLUETOOTH_COLUMN_TRUSTED, &trusted, -1);
	if (paired || trusted) {
		g_object_set (cell, "icon-name", GTK_STOCK_CLEAR, NULL);
	} else {
		g_object_set (cell, "icon-name", NULL, NULL);
	}
}

static void
pair_to_text (GtkTreeViewColumn *column, GtkCellRenderer *cell,
		GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	gboolean paired, trusted;
	gtk_tree_model_get (model, iter, BLUETOOTH_COLUMN_PAIRED, &paired,
			BLUETOOTH_COLUMN_TRUSTED, &trusted, -1);

	if (!paired && !trusted) {
		g_object_set (cell, "markup", _("<u>Pair</u>"), NULL);
	}
}

static void
connect_to_text (GtkTreeViewColumn *column, GtkCellRenderer *cell,
		GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	gboolean paired, trusted, connected;
	gtk_tree_model_get (model, iter, BLUETOOTH_COLUMN_PAIRED, &paired,
			BLUETOOTH_COLUMN_CONNECTED, &connected,
			BLUETOOTH_COLUMN_TRUSTED, &trusted, -1);

	if ((paired || trusted) && connected == FALSE) {
		g_object_set (cell, "markup", _("<u>Connect</u>"), NULL);
	} else {
		g_object_set (cell, "markup", "", NULL);
	}
}

static void
browse_to_text (GtkTreeViewColumn *column, GtkCellRenderer *cell,
		GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
	gboolean found = FALSE;
	guint i;
	char **uuids = NULL;

	gtk_tree_model_get (model, iter, BLUETOOTH_COLUMN_UUIDS, &uuids, -1);

	if (uuids != NULL) {
		for (i = 0; uuids[i] != NULL; i++)
			if (g_str_equal (uuids[i], "OBEXFileTransfer")) {
				g_object_set (cell, "markup", _("<u>Browse</u>"), NULL);
				found = TRUE;
				break;
			}
		g_strfreev (uuids);
	}

	if (found == FALSE)
		g_object_set (cell, "markup", "", NULL);
}

static void
set_scanning_view (GtkButton *button, MoblinPanel *self)
{
	set_current_page (self, PAGE_ADD);
}

static void
set_device_view (GtkButton *button, MoblinPanel *self)
{
	set_current_page (self, PAGE_DEVICES);
}

static void
determine_connecting (const gchar *key, gpointer value, gpointer user_data)
{
	BluetoothStatus status = GPOINTER_TO_INT (value);
	BluetoothStatus *other_status = user_data;

	if (status == BLUETOOTH_STATUS_CONNECTING)
		(*other_status) = status;
}

static void
have_connecting_device (MoblinPanel *self)
{
	MoblinPanelPrivate *priv = MOBLIN_PANEL_GET_PRIVATE (self);
	GHashTable *states = NULL;
	GtkTreeIter iter;
	BluetoothStatus status = BLUETOOTH_STATUS_INVALID;
	gboolean connecting = FALSE;

	if (gtk_tree_model_get_iter_first (priv->chooser_model, &iter) == TRUE)
	{
		gtk_tree_model_get(priv->chooser_model, &iter, BLUETOOTH_COLUMN_SERVICES, &states, -1);

		if (states) {
			g_hash_table_foreach (states, (GHFunc) determine_connecting, &status);

			g_hash_table_unref (states);
		}

		if (status == BLUETOOTH_STATUS_CONNECTING)
			connecting = TRUE;

		if (connecting != priv->connecting) {
			priv->connecting = connecting;
			g_signal_emit (self, _signals[STATUS_CONNECTING], 0,
				priv->connecting);
		}
	}
}

static void
model_changed_cb (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer user_data)
{
	have_connecting_device (MOBLIN_PANEL (user_data));
}

static void
row_deleted_cb (GtkTreeModel *model, GtkTreePath *path, gpointer user_data)
{
	have_connecting_device (MOBLIN_PANEL (user_data));
}

static void
does_not_match_cb (GtkButton *button, gpointer user_data)
{
	MoblinPanel *self = MOBLIN_PANEL (user_data);
	MoblinPanelPrivate *priv = MOBLIN_PANEL_GET_PRIVATE (self);
	DBusGMethodInvocation *context;
	GError *error = NULL;

	set_current_page (self, PAGE_FAILURE);

	set_failure_message (self, priv->target_name);

	context = g_object_get_data (G_OBJECT (button), "context");
	g_error_new (AGENT_ERROR, AGENT_ERROR_REJECT, "Agent callback cancelled");
	dbus_g_method_return (context, error);

	g_object_set_data (G_OBJECT (priv->does_not_match_button), "context", NULL);
	g_object_set_data (G_OBJECT (priv->matches_button), "context", NULL);
}

static void
matches_cb (GtkButton *button, gpointer user_data)
{
	MoblinPanelPrivate *priv = MOBLIN_PANEL_GET_PRIVATE (user_data);
	DBusGMethodInvocation *context;

	context = g_object_get_data (G_OBJECT (button), "context");
	gtk_widget_set_sensitive (priv->does_not_match_button, FALSE);
	gtk_widget_set_sensitive (priv->matches_button, FALSE);
	dbus_g_method_return (context, "");

	g_object_set_data (G_OBJECT (priv->does_not_match_button), "context", NULL);
	g_object_set_data (G_OBJECT (priv->matches_button), "context", NULL);
}

static gboolean
confirm_callback (DBusGMethodInvocation *context, DBusGProxy *device, guint pin, gpointer user_data)
{
	MoblinPanel *self = MOBLIN_PANEL (user_data);
	MoblinPanelPrivate *priv = MOBLIN_PANEL_GET_PRIVATE (self);
	gchar *text, *label;

	priv->target_ssp = TRUE;
	set_current_page (self, PAGE_SSP_SETUP);

	text = g_strdup_printf ("%d", pin);
	label = g_strdup_printf (_("Please confirm that the PIN displayed on '%s' matches this one."),
				priv->target_name);
	set_ssp_pin_message (self, label);
	set_ssp_pin_label (self, text);

	gtk_widget_set_sensitive (priv->does_not_match_button, TRUE);
	gtk_widget_set_sensitive (priv->matches_button, TRUE);

	g_object_set_data (G_OBJECT (priv->does_not_match_button), "context", context);
	g_object_set_data (G_OBJECT (priv->matches_button), "context", context);

	return TRUE;
}

static gboolean
display_callback (DBusGMethodInvocation *context, DBusGProxy *device, guint pin, guint entered,
		gpointer user_data)
{
	MoblinPanel *self = MOBLIN_PANEL (user_data);
	MoblinPanelPrivate *priv = MOBLIN_PANEL_GET_PRIVATE (self);
	gchar *text, *code, *done;

	priv->display_called = TRUE;
	priv->target_ssp = TRUE;

	set_current_page (self, PAGE_SSP_SETUP);

	code = g_strdup_printf ("%d", pin);

	if (entered > 0) {
		GtkEntry *entry;
		gunichar invisible;
		GString *str;
		guint i;

		entry = GTK_ENTRY (gtk_entry_new ());
		invisible = gtk_entry_get_invisible_char (entry);
		g_object_unref (entry);

		str = g_string_new (NULL);
		for (i = 0; i < entered; i++)
			g_string_append_unichar (str, invisible);
		if (entered < strlen (code))
			g_string_append (str, code + entered);

		done = g_string_free (str, FALSE);
	} else {
		done = g_strdup ("");
	}

	text = g_strdup_printf("%s%s", done, code + entered);
	set_ssp_pin_message (self, _("Please enter the following PIN:"));
	set_ssp_pin_label (self, text);

	dbus_g_method_return (context);

	return TRUE;
}

static GtkWidget *
create_failure_page (MoblinPanel *self)
{
	MoblinPanelPrivate *priv;
	GtkWidget *page;
	GtkWidget *page_title;
	GtkWidget *vbox, *hbox;
	GtkWidget *back_button;

	priv = MOBLIN_PANEL_GET_PRIVATE (self);

	page = mx_gtk_frame_new ();
	page_title = gtk_label_new ("");
	gtk_frame_set_label_widget (GTK_FRAME (page), page_title);
	set_frame_title (GTK_FRAME (page), _("Device setup failed"));
	gtk_widget_show (page);

	vbox = gtk_vbox_new (FALSE, 6);
	gtk_widget_show (vbox);
	gtk_container_add (GTK_CONTAINER (page), vbox);
	priv->label_failure = gtk_label_new ("");
	gtk_misc_set_alignment (GTK_MISC (priv->label_failure), 0.0, 0.5);
	gtk_widget_show (priv->label_failure);
	gtk_box_pack_start (GTK_BOX (vbox), priv->label_failure, FALSE, FALSE, 6);

	hbox = gtk_hbox_new (FALSE, 6);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 6);
	back_button = gtk_button_new_with_label (_("Back to devices"));
	gtk_widget_show (back_button);
	gtk_box_pack_start (GTK_BOX (hbox), back_button, FALSE, FALSE, 6);
	g_signal_connect (back_button, "clicked", G_CALLBACK (set_device_view), self);

	return page;
}

static GtkWidget *
create_summary_page (MoblinPanel *self)
{
	MoblinPanelPrivate *priv;
	GtkWidget *page;
	GtkWidget *vbox, *hbox;
	GtkWidget *w;

	priv = MOBLIN_PANEL_GET_PRIVATE (self);

	page = mx_gtk_frame_new ();
	w = gtk_label_new (NULL);
	gtk_frame_set_label_widget (GTK_FRAME (page), w);
	gtk_widget_show (page);
	priv->summary_frame = page;

	vbox = gtk_vbox_new (FALSE, 12);
	gtk_widget_show (vbox);
	gtk_container_add (GTK_CONTAINER (page), vbox);
	w = gtk_label_new (_("Select the additional services you want to use with your device:"));
	gtk_misc_set_alignment (GTK_MISC (w), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 6);
	gtk_widget_show (w);

	priv->extra_config_vbox = gtk_vbox_new (FALSE, 6);
	gtk_widget_show (priv->extra_config_vbox);
	gtk_box_pack_start (GTK_BOX (vbox), priv->extra_config_vbox, TRUE, TRUE, 6);

	hbox = gtk_hbox_new (FALSE, 6);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 6);
	w = gtk_button_new_with_label (_("Done"));
	gtk_widget_show (w);
	gtk_box_pack_start (GTK_BOX (hbox), w, FALSE, FALSE, 6);
	g_signal_connect (w, "clicked", G_CALLBACK (set_device_view), self);

	return page;
}

static GtkWidget *
create_setup_page (MoblinPanel *self)
{
	MoblinPanelPrivate *priv;
	GtkWidget *page, *page_title;
	GtkWidget *vbox, *hbox;
	GtkWidget *back_button;

	priv = MOBLIN_PANEL_GET_PRIVATE (self);

	page = mx_gtk_frame_new ();
	page_title = gtk_label_new ("");
	gtk_frame_set_label_widget (GTK_FRAME (page), page_title);
	set_frame_title (GTK_FRAME (page), _("Device setup"));
	gtk_widget_show (page);

	vbox = gtk_vbox_new (FALSE, 6);
	gtk_widget_show (vbox);
	gtk_container_add (GTK_CONTAINER (page), vbox);
	priv->label_pin_help = gtk_label_new ("");
	gtk_widget_show (priv->label_pin_help);
	gtk_box_pack_start (GTK_BOX (vbox), priv->label_pin_help, FALSE, FALSE, 6);
	priv->label_pin = gtk_label_new ("");
	gtk_widget_show (priv->label_pin);
	gtk_box_pack_start (GTK_BOX (vbox), priv->label_pin, FALSE, FALSE, 6);

	hbox = gtk_hbox_new (FALSE, 6);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 6);
	back_button = gtk_button_new_with_label (_("Back to devices"));
	gtk_widget_show (back_button);
	gtk_box_pack_start (GTK_BOX (hbox), back_button, FALSE, FALSE, 6);
	g_signal_connect (back_button, "clicked", G_CALLBACK (set_device_view), self);

	return page;
}

static GtkWidget *
create_ssp_setup_page (MoblinPanel *self)
{
	MoblinPanelPrivate *priv;
	GtkWidget *page, *page_title;
	GtkWidget *vbox, *hbox;
	GtkWidget *back_button;

	priv = MOBLIN_PANEL_GET_PRIVATE (self);

	page = mx_gtk_frame_new ();
	page_title = gtk_label_new ("");
	gtk_frame_set_label_widget (GTK_FRAME (page), page_title);
	set_frame_title (GTK_FRAME (page), _("Device setup"));
	gtk_widget_show (page);

	vbox = gtk_vbox_new (FALSE, 6);
	gtk_widget_show (vbox);
	gtk_container_add (GTK_CONTAINER (page), vbox);
	priv->label_ssp_pin_help = gtk_label_new ("");
	gtk_widget_show (priv->label_ssp_pin_help);
	gtk_box_pack_start (GTK_BOX (vbox), priv->label_ssp_pin_help, FALSE, FALSE, 6);
	priv->label_ssp_pin = gtk_label_new ("");
	gtk_widget_show (priv->label_ssp_pin);
	gtk_box_pack_start (GTK_BOX (vbox), priv->label_ssp_pin, FALSE, FALSE, 6);

	hbox = gtk_hbox_new (FALSE, 6);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 6);
	priv->matches_button = gtk_button_new_with_label (_("Matches"));
	gtk_widget_show (priv->matches_button);
	g_signal_connect (priv->matches_button, "clicked", G_CALLBACK (matches_cb), self);
	gtk_box_pack_start (GTK_BOX (hbox), priv->matches_button, FALSE, FALSE, 6);
	priv->does_not_match_button = gtk_button_new_with_label (_("Does not match"));
	gtk_widget_show (priv->does_not_match_button);
	g_signal_connect (priv->does_not_match_button, "clicked", G_CALLBACK (does_not_match_cb), self);
	gtk_box_pack_start (GTK_BOX (hbox), priv->does_not_match_button, FALSE, FALSE, 6);

	hbox = gtk_hbox_new (FALSE, 6);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 6);
	back_button = gtk_button_new_with_label (_("Back to devices"));
	gtk_widget_show (back_button);
	gtk_box_pack_start (GTK_BOX (hbox), back_button, FALSE, FALSE, 6);
	g_signal_connect (back_button, "clicked", G_CALLBACK (set_device_view), self);

	return page;
}

static GtkWidget *
create_add_page (MoblinPanel *self)
{
	MoblinPanelPrivate *priv;
	GtkWidget *page;
	GtkWidget *vbox, *hbox;
	GtkWidget *filter;
	GtkWidget *frame_title;
	GtkWidget *frame;
	GtkWidget *back_button;
	GtkWidget *pin_button;
	GtkWidget *tree_view;
	GtkTreeViewColumn *type_column;
	GtkCellRenderer *cell;

	priv = MOBLIN_PANEL_GET_PRIVATE (self);

	page = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (page);

	/* Add child widgetry */
	vbox = gtk_vbox_new (FALSE, 4);
	gtk_widget_show (vbox);
	gtk_box_pack_start (GTK_BOX (page), vbox, TRUE, TRUE, 4);

	frame = mx_gtk_frame_new ();
	frame_title = gtk_label_new ("");
	gtk_frame_set_label_widget (GTK_FRAME (frame), frame_title);
	set_frame_title (GTK_FRAME (frame), _("Devices"));

	/* Device list */
	priv->chooser = g_object_new (BLUETOOTH_TYPE_CHOOSER,
				"has-internal-device-filter", FALSE,
				"show-device-category", FALSE,
				"show-searching", TRUE,
				"device-category-filter", BLUETOOTH_CATEGORY_NOT_PAIRED_OR_TRUSTED,
				NULL);
	tree_view = bluetooth_chooser_get_treeview (BLUETOOTH_CHOOSER (priv->chooser));
	g_object_set (tree_view, "enable-grid-lines", TRUE, "headers-visible", FALSE, NULL);
	type_column = bluetooth_chooser_get_type_column (BLUETOOTH_CHOOSER (priv->chooser));
	/* Add the pair button */
	cell = mux_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (type_column, cell, FALSE);

	gtk_tree_view_column_set_cell_data_func (type_column, cell,
						 pair_to_text, self, NULL);
	g_signal_connect (cell, "activated", G_CALLBACK (pair_clicked), self);

	gtk_widget_show (priv->chooser);
	gtk_container_add (GTK_CONTAINER (frame), priv->chooser);
	gtk_widget_show (frame);
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 4);

	/* Back button */
	back_button = gtk_button_new_with_label (_("Back to devices"));
	gtk_widget_show (back_button);
	g_signal_connect (back_button, "clicked",
			G_CALLBACK (set_device_view), self);
	gtk_box_pack_start (GTK_BOX (vbox), back_button, FALSE, FALSE, 4);

	/* Right column */
	frame = mx_gtk_frame_new ();
	gtk_widget_show (frame);
	vbox = gtk_vbox_new (FALSE, 4);
	gtk_widget_show (vbox);
	gtk_container_add (GTK_CONTAINER (frame), vbox);
	gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 4);

	hbox = gtk_hbox_new (FALSE, 4);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 4);

	/* Filter combo */
	filter = bluetooth_filter_widget_new ();
	gtk_widget_show (filter);
	bluetooth_filter_widget_set_title (BLUETOOTH_FILTER_WIDGET (filter), _("Only show:"));
	bluetooth_filter_widget_bind_filter (BLUETOOTH_FILTER_WIDGET (filter),
					     BLUETOOTH_CHOOSER (priv->chooser));
	gtk_box_pack_start (GTK_BOX (vbox), filter, FALSE, FALSE, 4);

	/* Button for PIN options file */
	pin_button = gtk_button_new_with_label (_("PIN options"));
	gtk_widget_show (pin_button);
	g_signal_connect (pin_button, "clicked",
                    G_CALLBACK (pin_options_button_clicked_cb), self);
	gtk_box_pack_start (GTK_BOX (vbox), pin_button, FALSE, FALSE, 4);

	return page;
}

static GtkWidget *
create_devices_page (MoblinPanel *self)
{
	MoblinPanelPrivate *priv;
	GtkWidget *page;
	GtkWidget *frame_title;
	GtkWidget *vbox, *hbox;
	GtkWidget *frame;
	GtkWidget *power_label;
	GtkTreeViewColumn *type_column;
	GtkCellRenderer *cell;
	KillswitchState switch_state;
	GtkWidget *tree_view;

	priv = MOBLIN_PANEL_GET_PRIVATE (self);

	page = gtk_hbox_new (FALSE, 0);
	gtk_widget_show (page);
	/* Add child widgetry */
	vbox = gtk_vbox_new (FALSE, 4);
	gtk_widget_show (vbox);
	gtk_box_pack_start (GTK_BOX (page), vbox, TRUE, TRUE, 4);

	frame = mx_gtk_frame_new ();
	frame_title = gtk_label_new ("");
	gtk_frame_set_label_widget (GTK_FRAME (frame), frame_title);
	set_frame_title (GTK_FRAME (frame), _("Devices"));

	/* Device list */
	priv->display = g_object_new (BLUETOOTH_TYPE_CHOOSER,
			        "has-internal-device-filter", FALSE,
				"show-device-category", FALSE,
				"show-searching", FALSE,
				"device-category-filter", BLUETOOTH_CATEGORY_PAIRED_OR_TRUSTED,
				NULL);
	type_column = bluetooth_chooser_get_type_column (BLUETOOTH_CHOOSER (priv->display));
	if (!priv->chooser_model) {
		priv->chooser_model = bluetooth_chooser_get_model (BLUETOOTH_CHOOSER (priv->display));
		g_signal_connect (priv->chooser_model, "row-changed", G_CALLBACK (model_changed_cb), self);
		g_signal_connect (priv->chooser_model, "row-deleted", G_CALLBACK (row_deleted_cb), self);
		g_signal_connect (priv->chooser_model, "row-inserted", G_CALLBACK (model_changed_cb), self);
	}

	tree_view = bluetooth_chooser_get_treeview (BLUETOOTH_CHOOSER (priv->display));
	g_object_set (tree_view, "enable-grid-lines", TRUE, "headers-visible", FALSE, NULL);

	/* Add the browse button */
	cell = mux_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (type_column, cell, FALSE);

	gtk_tree_view_column_set_cell_data_func (type_column, cell,
						 browse_to_text, self, NULL);
	g_signal_connect (cell, "activated", G_CALLBACK (browse_clicked), self);

	/* Add the connect button */
	cell = mux_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (type_column, cell, FALSE);

	gtk_tree_view_column_set_cell_data_func (type_column, cell,
						 connect_to_text, self, NULL);
	g_signal_connect (cell, "activated", G_CALLBACK (connect_clicked), self);
	/* Add the remove button */
	cell = koto_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_end (type_column, cell, FALSE);
	gtk_tree_view_column_set_cell_data_func (type_column, cell,
						remove_to_icon, self, NULL);
	g_signal_connect (cell, "activated", G_CALLBACK (remove_clicked_cb), self);

	gtk_widget_show (priv->display);
	gtk_container_add (GTK_CONTAINER (frame), priv->display);
	gtk_widget_show (frame);
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 4);

	/* Add new button */
	priv->add_new_button = gtk_button_new_with_label (_("Add a new device"));
	gtk_widget_show (priv->add_new_button);
	g_signal_connect (priv->add_new_button, "clicked",
			G_CALLBACK (set_scanning_view), self);
	gtk_box_pack_start (GTK_BOX (vbox), priv->add_new_button, FALSE, FALSE, 4);

	/* Right column */
	frame = mx_gtk_frame_new ();
	gtk_widget_show (frame);
	vbox = gtk_vbox_new (FALSE, 4);
	gtk_widget_show (vbox);
	gtk_container_add (GTK_CONTAINER (frame), vbox);
	gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 4);

	hbox = gtk_hbox_new (FALSE, 4);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 4);

	/* Power switch */
	/* Translators: This string appears next to a toggle switch which controls
	 * enabling/disabling Bluetooth radio's on the device. Akin to the power
	 * switches in the Network UI of Moblin */
	power_label = gtk_label_new (_("Bluetooth"));
	gtk_widget_show (power_label);
	gtk_box_pack_start (GTK_BOX (hbox), power_label, FALSE, FALSE, 4);

	priv->power_switch = mx_gtk_light_switch_new ();
	if (priv->killswitch != NULL) {
		if (bluetooth_killswitch_has_killswitches (priv->killswitch) == FALSE) {
			gtk_widget_set_sensitive (priv->power_switch, FALSE);
			gtk_widget_set_sensitive (priv->add_new_button, FALSE);
		} else {
			switch_state = bluetooth_killswitch_get_state (priv->killswitch);

			switch (switch_state) {
				case KILLSWITCH_STATE_UNBLOCKED:
					mx_gtk_light_switch_set_active
						(MX_GTK_LIGHT_SWITCH (priv->power_switch),
						 TRUE);
					gtk_widget_set_sensitive (priv->add_new_button, TRUE);
					gtk_widget_set_sensitive (priv->power_switch, TRUE);
				break;
				case KILLSWITCH_STATE_SOFT_BLOCKED:
					mx_gtk_light_switch_set_active
						(MX_GTK_LIGHT_SWITCH (priv->power_switch),
							FALSE);
				break;
				case KILLSWITCH_STATE_HARD_BLOCKED:
				case KILLSWITCH_STATE_NO_ADAPTER:
				default:
					gtk_widget_set_sensitive (priv->power_switch, FALSE);
					gtk_widget_set_sensitive (priv->add_new_button, FALSE);
				break;
			}
		}
	} else {
		gtk_widget_set_sensitive (priv->power_switch, FALSE);
		gtk_widget_set_sensitive (priv->add_new_button, FALSE);
	}
	g_signal_connect  (priv->killswitch, "state-changed",
			G_CALLBACK (killswitch_state_changed_cb), self);
	g_signal_connect (priv->power_switch, "switch-flipped",
			G_CALLBACK (power_switch_toggled_cb), self);
	gtk_widget_show (priv->power_switch);
	gtk_box_pack_start (GTK_BOX (hbox), priv->power_switch, FALSE, FALSE, 4);

	/* Button for Send file */
	priv->send_button = gtk_button_new_with_label (_("Send file from your computer"));
	gtk_widget_show (priv->send_button);
	g_signal_connect (priv->send_button, "clicked",
                    G_CALLBACK (send_file_button_clicked_cb), self);
	g_signal_connect (priv->display, "selected-device-changed",
			G_CALLBACK (selected_device_changed_cb), self);
	gtk_box_pack_start (GTK_BOX (vbox), priv->send_button, FALSE, FALSE, 4);

	enable_send_file (self);

	return page;
}

static GtkWidget *
create_connecting_page (MoblinPanel *self)
{
	MoblinPanelPrivate *priv;
	GtkWidget *page, *page_title;
	GtkWidget *vbox, *hbox;
	GtkWidget *back_button;

	priv = MOBLIN_PANEL_GET_PRIVATE (self);

	page = mx_gtk_frame_new ();
	page_title = gtk_label_new ("");
	gtk_frame_set_label_widget (GTK_FRAME (page), page_title);
	//set_frame_title (GTK_FRAME (page), _("Connecting"));
	gtk_widget_show (page);

	vbox = gtk_vbox_new (FALSE, 6);
	gtk_widget_show (vbox);
	gtk_container_add (GTK_CONTAINER (page), vbox);
	priv->spinner = gtk_spinner_new ();
	gtk_widget_set_size_request (priv->spinner, 150, 150);
	gtk_widget_show (priv->spinner);
	gtk_box_pack_start (GTK_BOX (vbox), priv->spinner, FALSE, FALSE, 6);

	hbox = gtk_hbox_new (FALSE, 6);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 6);
	back_button = gtk_button_new_with_label (_("Back to devices"));
	gtk_widget_show (back_button);
	gtk_box_pack_start (GTK_BOX (hbox), back_button, FALSE, FALSE, 6);
	g_signal_connect (back_button, "clicked", G_CALLBACK (set_device_view), self);

	return page;
}

static gboolean
pincode_callback (DBusGMethodInvocation *context,
		DBusGProxy *device,
		gpointer user_data)
{
	MoblinPanel *self = MOBLIN_PANEL (user_data);
	MoblinPanelPrivate *priv = MOBLIN_PANEL_GET_PRIVATE (self);

	priv->target_ssp = FALSE;

	/* Only show the pincode page if the pincode isn't automatic */
	if (priv->automatic_pincode == FALSE)
		set_current_page (self, PAGE_SETUP);

	dbus_g_method_return (context, priv->pincode);

	return TRUE;
}

static void
update_random_pincode (MoblinPanel *self)
{
	MoblinPanelPrivate *priv = MOBLIN_PANEL_GET_PRIVATE (self);

	priv->target_pincode = g_strdup_printf ("%d", g_random_int_range (pow (10, PIN_NUM_DIGITS - 1),
						pow (10, PIN_NUM_DIGITS) - 1));
	priv->automatic_pincode = FALSE;
}

static void
moblin_panel_init (MoblinPanel *self)
{
	MoblinPanelPrivate *priv;
	GtkWidget *devices_page, *add_page, *setup_page, *ssp_setup_page, *failure_page, *connecting_page;
	GtkWidget *page;

	priv = MOBLIN_PANEL_GET_PRIVATE (self);
	priv->pin_dialog = NULL;
	priv->target_address = NULL;
	priv->target_name = NULL;
	priv->target_pincode = NULL;
	priv->target_type = BLUETOOTH_TYPE_ANY;
	priv->target_ssp = FALSE;
	priv->create_started = FALSE;
	priv->display_called = FALSE;
	priv->user_pincode = NULL;
	priv->automatic_pincode = FALSE;
	priv->pincode = NULL;
	priv->connecting = FALSE;

	update_random_pincode (self);

	priv->client = bluetooth_client_new ();
	priv->agent = bluetooth_agent_new ();

	bluetooth_agent_set_pincode_func (priv->agent, pincode_callback, self);
	bluetooth_agent_set_display_func (priv->agent, display_callback, self);
	bluetooth_agent_set_cancel_func (priv->agent, cancel_callback, self);
	bluetooth_agent_set_confirm_func (priv->agent, confirm_callback, self);

	bluetooth_agent_setup (priv->agent, AGENT_PATH);

	priv->killswitch = bluetooth_killswitch_new ();
	bluetooth_plugin_manager_init ();

	gtk_box_set_homogeneous (GTK_BOX (self), FALSE);
	gtk_box_set_spacing (GTK_BOX (self), 0);

	priv->notebook = gtk_notebook_new ();
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (priv->notebook), FALSE);
	gtk_notebook_set_show_border (GTK_NOTEBOOK (priv->notebook), FALSE);

	devices_page = create_devices_page (self);
	gtk_widget_show (devices_page);
	gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook), devices_page, NULL);

	add_page = create_add_page (self);
	gtk_widget_show (add_page);
	gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook), add_page, NULL);

	setup_page = create_setup_page (self);
	gtk_widget_show (setup_page);
	gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook), setup_page, NULL);

	ssp_setup_page = create_ssp_setup_page (self);
	gtk_widget_show (ssp_setup_page);
	gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook), ssp_setup_page, NULL);

	connecting_page = create_connecting_page (self);
	gtk_widget_show (connecting_page);
	gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook), connecting_page, NULL);

	page = create_summary_page (self);
	gtk_widget_show (page);
	gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook), page, NULL);

	failure_page = create_failure_page (self);
	gtk_widget_show (failure_page);
	gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook), failure_page, NULL);

	set_current_page (self, PAGE_DEVICES);
	gtk_widget_show (priv->notebook);
	gtk_box_pack_start (GTK_BOX (self), priv->notebook, TRUE, TRUE, 0);
}

static void
moblin_panel_dispose (GObject *object)
{
	MoblinPanelPrivate *priv = MOBLIN_PANEL_GET_PRIVATE (object);

	bluetooth_plugin_manager_cleanup ();

	if (priv->killswitch != NULL) {
		g_object_unref (priv->killswitch);
		priv->killswitch = NULL;
	}

	if (priv->agent != NULL) {
		g_object_unref (priv->agent);
		priv->agent = NULL;
	}

	if (priv->client != NULL) {
		g_object_unref (priv->client);
		priv->client = NULL;
	}

	G_OBJECT_CLASS (moblin_panel_parent_class)->dispose (object);
}

static void
moblin_panel_class_init (MoblinPanelClass *klass)
{
	GObjectClass *obj_class = G_OBJECT_CLASS (klass);
	g_type_class_add_private (klass, sizeof (MoblinPanelPrivate));
	obj_class->dispose = moblin_panel_dispose;

	_signals[STATUS_CONNECTING] = g_signal_new ("status-connecting", MOBLIN_TYPE_PANEL,
						G_SIGNAL_RUN_FIRST,
						G_STRUCT_OFFSET (MoblinPanelClass, status_connecting),
						NULL, NULL, g_cclosure_marshal_VOID__BOOLEAN,
						G_TYPE_NONE, 1, G_TYPE_BOOLEAN);

	_signals[REQUEST_FOCUS] = g_signal_new ("request-focus", MOBLIN_TYPE_PANEL,
						G_SIGNAL_RUN_FIRST,
                                                0,
						NULL, NULL, g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE, 0);
}

/**
 * moblin_panel_new:
 *
 * Return value: A #MoblinPanel widget
 **/
GtkWidget *
moblin_panel_new (void)
{
	return g_object_new (MOBLIN_TYPE_PANEL, NULL);
}

/**
 *
 * moblin_panel_reset_view:
 *
 * @self: A #MoblinPanel widget
 **/
void
moblin_panel_reset_view (MoblinPanel *self)
{
	set_current_page (self, PAGE_DEVICES);
}
