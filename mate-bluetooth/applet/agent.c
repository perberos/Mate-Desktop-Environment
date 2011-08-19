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

#include <stdlib.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <dbus/dbus-glib.h>

#include <bluetooth-client.h>
#include <bluetooth-agent.h>

#include "notify.h"
#include "agent.h"

static BluetoothClient *client;
static BluetoothAgent *agent = NULL;

static GList *input_list = NULL;

typedef struct input_data input_data;
struct input_data {
	char *path;
	char *uuid;
	gboolean numeric;
	DBusGProxy *device;
	DBusGMethodInvocation *context;
	GtkWidget *dialog;
	GtkWidget *button;
	GtkWidget *entry;
};

static gint input_compare(gconstpointer a, gconstpointer b)
{
	input_data *a_data = (input_data *) a;
	input_data *b_data = (input_data *) b;

	return g_ascii_strcasecmp(a_data->path, b_data->path);
}

static void input_free(input_data *input)
{
	gtk_widget_destroy(input->dialog);

	input_list = g_list_remove(input_list, input);

	if (input->device != NULL)
		g_object_unref(input->device);

	g_free(input->uuid);
	g_free(input->path);
	g_free(input);

	if (g_list_length(input_list) == 0)
		disable_blinking();
}

static void pin_callback(GtkWidget *dialog,
				gint response, gpointer user_data)
{
	input_data *input = user_data;

	if (response == GTK_RESPONSE_OK) {
		const char *text;

		text = gtk_entry_get_text(GTK_ENTRY(input->entry));

		if (input->numeric == TRUE) {
			guint pin = atoi(text);
			dbus_g_method_return(input->context, pin);
		} else {
			dbus_g_method_return(input->context, text);
		}
	} else {
		GError *error;
		error = g_error_new(AGENT_ERROR, AGENT_ERROR_REJECT,
						"Pairing request rejected");
		dbus_g_method_return_error(input->context, error);
	}

	input_free(input);
}

static void
confirm_callback (GtkWidget *dialog,
		  gint response,
		  gpointer user_data)
{
	input_data *input = user_data;

	if (response != GTK_RESPONSE_ACCEPT) {
		GError *error;
		error = g_error_new(AGENT_ERROR, AGENT_ERROR_REJECT,
					"Confirmation request rejected");
		dbus_g_method_return_error(input->context, error);
	} else
		dbus_g_method_return(input->context);

	input_free(input);
}

static void set_trusted(input_data *input)
{
	GValue value = { 0 };
	gboolean active;

	if (input->device == NULL)
		return;

	active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (input->button));
	if (active == FALSE)
		return;

	g_value_init (&value, G_TYPE_BOOLEAN);
	g_value_set_boolean (&value, TRUE);

	dbus_g_proxy_call (input->device, "SetProperty", NULL,
			   G_TYPE_STRING, "Trusted",
			   G_TYPE_VALUE, &value, G_TYPE_INVALID,
			   G_TYPE_INVALID);

	g_value_unset (&value);
}

static void
auth_callback (GtkWidget *dialog,
	       gint response,
	       gpointer user_data)
{
	input_data *input = user_data;

	if (response == GTK_RESPONSE_ACCEPT) {
		set_trusted (input);
		dbus_g_method_return(input->context);
	} else {
		GError *error;
		error = g_error_new (AGENT_ERROR, AGENT_ERROR_REJECT,
				     "Authorization request rejected");
		dbus_g_method_return_error (input->context, error);
	}

	input_free(input);
}

static void insert_callback(GtkEditable *editable, const gchar *text,
			gint length, gint *position, gpointer user_data)
{
	input_data *input = user_data;
	gint i;

	if (input->numeric == FALSE)
		return;

	for (i = 0; i < length; i++) {
		if (g_ascii_isdigit(text[i]) == FALSE) {
			g_signal_stop_emission_by_name(editable,
							"insert-text");
		}
	}
}

static void changed_callback(GtkWidget *editable, gpointer user_data)
{
	input_data *input = user_data;
	const gchar *text;

	text = gtk_entry_get_text(GTK_ENTRY(input->entry));

	gtk_dialog_set_response_sensitive (GTK_DIALOG (input->dialog),
					   GTK_RESPONSE_OK,
					   *text != '\0' ? TRUE : FALSE);
}

static void toggled_callback(GtkWidget *button, gpointer user_data)
{
	input_data *input = user_data;
	gboolean mode;

	mode = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

	gtk_entry_set_visibility (GTK_ENTRY (input->entry), mode);
}

static void
pin_dialog (DBusGProxy *adapter,
		DBusGProxy *device,
		const char *name,
		const char *long_name,
		gboolean numeric,
		DBusGMethodInvocation *context)
{
	GtkWidget *dialog;
	GtkWidget *button;
	GtkWidget *entry;
	GtkBuilder *xml;
	char *str;
	input_data *input;

	xml = gtk_builder_new ();
	if (gtk_builder_add_from_file (xml, "passkey-dialogue.ui", NULL) == 0)
		gtk_builder_add_from_file (xml, PKGDATADIR "/passkey-dialogue.ui", NULL);

	input = g_new0 (input_data, 1);
	input->path = g_strdup (dbus_g_proxy_get_path(adapter));
	input->numeric = numeric;
	input->context = context;
	input->device = g_object_ref (device);

	dialog = GTK_WIDGET (gtk_builder_get_object (xml, "dialog"));

	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
	if (notification_supports_actions () != FALSE)
		gtk_window_set_keep_above (GTK_WINDOW (dialog), TRUE);
	else
		gtk_window_set_focus_on_map (GTK_WINDOW (dialog), FALSE);
	gtk_window_set_urgency_hint (GTK_WINDOW (dialog), TRUE);
	input->dialog = dialog;

	gtk_dialog_set_default_response (GTK_DIALOG (dialog),
					 GTK_RESPONSE_OK);
	gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
					   GTK_RESPONSE_OK,
					   FALSE);

	str = g_strdup_printf (_("Device '%s' wants to pair with this computer"),
			       name);
	g_object_set (G_OBJECT (dialog), "text", str, NULL);
	g_free (str);

	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
						  _("Please enter the PIN mentioned on device %s."),
						  long_name);

	entry = GTK_WIDGET (gtk_builder_get_object (xml, "entry"));
	if (numeric == TRUE) {
		gtk_entry_set_max_length (GTK_ENTRY (entry), 6);
		gtk_entry_set_width_chars (GTK_ENTRY (entry), 6);
		g_signal_connect (G_OBJECT (entry), "insert-text",
				  G_CALLBACK (insert_callback), input);
	} else {
		gtk_entry_set_max_length (GTK_ENTRY (entry), 16);
		gtk_entry_set_width_chars (GTK_ENTRY (entry), 16);
		gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
	}
	gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
	input->entry = entry;
	g_signal_connect (G_OBJECT (entry), "changed",
			  G_CALLBACK (changed_callback), input);

	button = GTK_WIDGET (gtk_builder_get_object (xml, "showinput_button"));
	if (numeric == FALSE) {
		g_signal_connect (G_OBJECT (button), "toggled",
				  G_CALLBACK (toggled_callback), input);
	} else {
		gtk_widget_set_no_show_all (button, TRUE);
		gtk_widget_hide (button);
	}

	input_list = g_list_append(input_list, input);

	g_signal_connect (G_OBJECT (dialog), "response",
			  G_CALLBACK (pin_callback), input);

	enable_blinking();
}

#if 0
static void display_dialog(DBusGProxy *adapter, DBusGProxy *device,
		const char *address, const char *name, const char *value,
				guint entered, DBusGMethodInvocation *context)
{
}
#endif

static void
confirm_dialog (DBusGProxy *adapter,
		DBusGProxy *device,
		const char *name,
		const char *long_name,
		const char *value,
		DBusGMethodInvocation *context)
{
	GtkWidget *dialog;
	GtkBuilder *xml;
	char *str;
	input_data *input;

	input = g_new0 (input_data, 1);
	input->path = g_strdup (dbus_g_proxy_get_path(adapter));
	input->device = g_object_ref (device);
	input->context = context;

	xml = gtk_builder_new ();
	if (gtk_builder_add_from_file (xml, "confirm-dialogue.ui", NULL) == 0)
		gtk_builder_add_from_file (xml, PKGDATADIR "/confirm-dialogue.ui", NULL);

	dialog = GTK_WIDGET (gtk_builder_get_object (xml, "dialog"));
	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
	if (notification_supports_actions () != FALSE)
		gtk_window_set_keep_above (GTK_WINDOW (dialog), TRUE);
	else
		gtk_window_set_focus_on_map (GTK_WINDOW (dialog), FALSE);
	gtk_window_set_urgency_hint (GTK_WINDOW (dialog), TRUE);
	input->dialog = dialog;

	str = g_strdup_printf (_("Device '%s' wants to pair with this computer"),
			       name);
	g_object_set (G_OBJECT (dialog), "text", str, NULL);
	g_free (str);

	str = g_strdup_printf ("<b>%s</b>", value);
	gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (dialog),
						    _("Please confirm whether the PIN '%s' matches the one on device %s."),
						    str, long_name);
	g_free (str);

	input_list = g_list_append (input_list, input);

	g_signal_connect (G_OBJECT (dialog), "response",
			  G_CALLBACK (confirm_callback), input);

	enable_blinking ();
}

static void
auth_dialog (DBusGProxy *adapter,
	     DBusGProxy *device,
	     const char *name,
	     const char *long_name,
	     const char *uuid,
	     DBusGMethodInvocation *context)
{
	GtkBuilder *xml;
	GtkWidget *dialog;
	GtkWidget *button;
	char *str;
	input_data *input;

	input = g_new0 (input_data, 1);
	input->path = g_strdup (dbus_g_proxy_get_path (adapter));
	input->uuid = g_strdup (uuid);
	input->device = g_object_ref (device);
	input->context = context;

	xml = gtk_builder_new ();
	if (gtk_builder_add_from_file (xml, "authorisation-dialogue.ui", NULL) == 0)
		gtk_builder_add_from_file (xml, PKGDATADIR "/authorisation-dialogue.ui", NULL);

	dialog = GTK_WIDGET (gtk_builder_get_object (xml, "dialog"));
	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
	if (notification_supports_actions () != FALSE)
		gtk_window_set_keep_above (GTK_WINDOW (dialog), TRUE);
	else
		gtk_window_set_focus_on_map (GTK_WINDOW (dialog), FALSE);
	gtk_window_set_urgency_hint (GTK_WINDOW (dialog), TRUE);
	input->dialog = dialog;

	/* translators: Whether to grant access to a particular service */
	str = g_strdup_printf (_("Grant access to '%s'"), uuid);
	g_object_set (G_OBJECT (dialog), "text", str, NULL);
	g_free (str);

	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
						  _("Device %s wants access to the service '%s'."),
						  long_name, uuid);

	button = GTK_WIDGET (gtk_builder_get_object (xml, "always_button"));
	input->button = button;

	input_list = g_list_append (input_list, input);

	g_signal_connect (G_OBJECT (dialog), "response",
			  G_CALLBACK (auth_callback), input);

	enable_blinking ();
}

static void show_dialog(gpointer data, gpointer user_data)
{
	input_data *input = data;

	gtk_widget_show_all(input->dialog);

	gtk_window_present(GTK_WINDOW(input->dialog));
}

static void present_notification_dialogs (void)
{
	g_list_foreach(input_list, show_dialog, NULL);

	disable_blinking();
}

static void notification_closed(GObject *object, gpointer user_data)
{
	present_notification_dialogs ();
}

#ifndef DBUS_TYPE_G_DICTIONARY
#define DBUS_TYPE_G_DICTIONARY \
	(dbus_g_type_get_map("GHashTable", G_TYPE_STRING, G_TYPE_VALUE))
#endif

static char *
device_get_name(DBusGProxy *proxy, char **long_name)
{
	GHashTable *hash;
	GValue *value;
	char *alias, *address;

	if (long_name != NULL)
		*long_name = NULL;

	if (dbus_g_proxy_call (proxy, "GetProperties",  NULL,
			       G_TYPE_INVALID,
			       DBUS_TYPE_G_DICTIONARY, &hash,
			       G_TYPE_INVALID) == FALSE) {
		return NULL;
	}

	value = g_hash_table_lookup(hash, "Address");
	if (value == NULL) {
		g_hash_table_destroy (hash);
		return NULL;
	}
	address = g_value_dup_string(value);

	value = g_hash_table_lookup(hash, "Alias");
	alias = value ? g_value_dup_string(value) : NULL;

	g_hash_table_destroy (hash);

	if (g_strcmp0 (alias, address) == 0) {
		g_free (alias);
		if (long_name != NULL)
			*long_name = g_strdup_printf ("'%s'", address);
		return address;
	}

	if (long_name != NULL)
		*long_name = g_strdup_printf ("'%s' (%s)", alias, address);

	g_free (address);

	return alias;
}

static gboolean pincode_request(DBusGMethodInvocation *context,
					DBusGProxy *device, gpointer user_data)
{
	DBusGProxy *adapter = user_data;
	char *name, *long_name, *line;

	name = device_get_name (device, &long_name);
	if (name == NULL)
		return FALSE;

	pin_dialog(adapter, device, name, long_name, FALSE, context);

	g_free (long_name);

	if (notification_supports_actions () == FALSE) {
		g_free (name);
		present_notification_dialogs ();
		return TRUE;
	}

	/* translators: this is a popup telling you a particular device
	 * has asked for pairing */
	line = g_strdup_printf(_("Pairing request for '%s'"), name);

	g_free(name);

	show_notification(_("Bluetooth device"),
			  line, _("Enter PIN"), 0,
			  G_CALLBACK(notification_closed));

	g_free(line);

	return TRUE;
}

static gboolean pin_request(DBusGMethodInvocation *context,
					DBusGProxy *device, gpointer user_data)
{
	DBusGProxy *adapter = user_data;
	char *name, *long_name, *line;

	name = device_get_name (device, &long_name);
	if (name == NULL)
		return FALSE;

	pin_dialog(adapter, device, name, long_name, TRUE, context);

	g_free (long_name);

	if (notification_supports_actions () == FALSE) {
		g_free (name);
		present_notification_dialogs ();
		return TRUE;
	}

	/* translators: this is a popup telling you a particular device
	 * has asked for pairing */
	line = g_strdup_printf(_("Pairing request for '%s'"), name);

	g_free(name);

	show_notification(_("Bluetooth device"),
			  line, _("Enter PIN"), 0,
			  G_CALLBACK(notification_closed));

	g_free(line);

	return TRUE;
}

static gboolean
display_request (DBusGMethodInvocation *context,
		 DBusGProxy *device,
		 guint pin,
		 guint entered,
		 gpointer user_data)
{
	g_warning ("Not implemented, please file a bug at how this happened");
	return FALSE;
#if 0
	DBusGProxy *adapter = user_data;
	char *name, *text;

	name = get_name_for_display (device);
	if (name == NULL)
		return FALSE;

	text = g_strdup_printf("%d", pin);
	display_dialog(adapter, device, address, name, text, entered, context);
	g_free(text);

	if (notification_supports_actions () == FALSE) {
		g_free (name);
		present_notification_dialogs ();
		return TRUE;
	}

	/* translators: this is a popup telling you a particular device
	 * has asked for pairing */
	line = g_strdup_printf(_("Pairing request for '%s'"), name);

	g_free(name);

	show_notification(_("Bluetooth device"),
			  line, _("Enter PIN"), 0,
			  G_CALLBACK(notification_closed));

	g_free(line);

	return TRUE;
#endif
}

static gboolean
confirm_request (DBusGMethodInvocation *context,
		 DBusGProxy *device,
		 guint pin,
		 gpointer user_data)
{
	DBusGProxy *adapter = user_data;
	char *name, *long_name, *line, *text;

	name = device_get_name (device, &long_name);
	if (name == NULL)
		return FALSE;

	text = g_strdup_printf("%d", pin);
	confirm_dialog(adapter, device, name, long_name, text, context);
	g_free(text);

	g_free (long_name);

	/* translators: this is a popup telling you a particular device
	 * has asked for pairing */
	line = g_strdup_printf(_("Pairing confirmation for '%s'"), name);

	g_free(name);

	/* translators:
	 * This message is for Bluetooth 2.1 support, when the
	 * action is clicked in the notification popup, the user
	 * will get to check whether the PIN matches the one
	 * showing on the Bluetooth device */
	if (notification_supports_actions () != FALSE)
		show_notification(_("Bluetooth device"),
				    line, _("Verify PIN"), 0,
				    G_CALLBACK(notification_closed));
	else
		present_notification_dialogs ();

	g_free(line);

	return TRUE;
}

static gboolean
authorize_request (DBusGMethodInvocation *context,
		   DBusGProxy *device,
		   const char *uuid,
		   gpointer user_data)
{
	DBusGProxy *adapter = user_data;
	char *name, *long_name, *line;

	name = device_get_name (device, &long_name);
	if (name == NULL)
		return FALSE;

	auth_dialog (adapter, device, name, long_name, uuid, context);

	g_free (long_name);

	if (notification_supports_actions () == FALSE) {
		g_free (name);
		present_notification_dialogs ();
		return TRUE;
	}

	line = g_strdup_printf (_("Authorization request from '%s'"), name);

	g_free (name);

	show_notification (_("Bluetooth device"),
			   line, _("Check authorization"), 0,
			   G_CALLBACK (notification_closed));

	g_free (line);

	return TRUE;
}

static gboolean cancel_request(DBusGMethodInvocation *context,
							gpointer user_data)
{
	DBusGProxy *adapter = user_data;
	GList *list;
	GError *result;
	input_data *input;

	input = g_new0(input_data, 1);
	input->path = g_strdup(dbus_g_proxy_get_path(adapter));

	list = g_list_find_custom(input_list, input, input_compare);

	g_free(input->path);
	g_free(input);

	if (!list || !list->data)
		return FALSE;

	input = list->data;

	close_notification();

	result = g_error_new(AGENT_ERROR, AGENT_ERROR_REJECT,
						"Agent callback cancelled");

	dbus_g_method_return_error(input->context, result);

	input_free(input);

	return TRUE;
}

static void
default_adapter_changed (GObject    *gobject,
			 GParamSpec *pspec,
			 gpointer    user_data)
{
	char *adapter_str;

	g_object_get (G_OBJECT (gobject), "default-adapter", &adapter_str, NULL);
	if (agent != NULL) {
		bluetooth_agent_unregister (agent);
		g_object_unref (agent);
		agent = NULL;
	}
	if (adapter_str != NULL) {
		DBusGConnection *connection;
		DBusGProxy *adapter;

		connection = dbus_g_bus_get(DBUS_BUS_SYSTEM, NULL);
		adapter = dbus_g_proxy_new_for_name (connection,
						     "org.bluez",
						     adapter_str,
						     "org.bluez.Adapter");
		dbus_g_connection_unref (connection);

		agent = bluetooth_agent_new();

		bluetooth_agent_set_pincode_func(agent, pincode_request, adapter);
		bluetooth_agent_set_passkey_func(agent, pin_request, adapter);
		bluetooth_agent_set_display_func(agent, display_request, adapter);
		bluetooth_agent_set_confirm_func(agent, confirm_request, adapter);
		bluetooth_agent_set_authorize_func(agent, authorize_request, adapter);
		bluetooth_agent_set_cancel_func(agent, cancel_request, adapter);

		bluetooth_agent_register(agent, adapter);

		g_object_unref (adapter);
	}
	g_free (adapter_str);
}

int setup_agents(void)
{
	dbus_g_error_domain_register(AGENT_ERROR, "org.bluez.Error",
							AGENT_ERROR_TYPE);

	client = bluetooth_client_new();
	g_signal_connect (G_OBJECT (client), "notify::default-adapter",
			  G_CALLBACK (default_adapter_changed), NULL);
	default_adapter_changed (G_OBJECT (client), NULL, NULL);

	return 0;
}

void cleanup_agents(void)
{
	if (agent != NULL) {
		bluetooth_agent_unregister (agent);
		g_object_unref (agent);
	}
	g_object_unref (client);
}

void show_agents(void)
{
	close_notification();

	g_list_foreach(input_list, show_dialog, NULL);

	disable_blinking();
}
