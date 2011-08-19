/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2005-2008  Marcel Holtmann <marcel@holtmann.org>
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

#include <math.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <dbus/dbus-glib.h>
#include <unique/uniqueapp.h>

#include <bluetooth-client.h>
#include <bluetooth-client-private.h>
#include <bluetooth-chooser.h>
#include <bluetooth-agent.h>
#include <bluetooth-plugin-manager.h>

#include "pin.h"

#define AGENT_PATH "/org/bluez/agent/wizard"

/* We'll try to connect to the device repeatedly for that
 * amount of time before we bail out */
#define CONNECT_TIMEOUT 3.0

#define W(x) GTK_WIDGET(gtk_builder_get_object(builder, x))

enum {
	PAGE_INTRO,
	PAGE_SEARCH,
	PAGE_CONNECTING,
	PAGE_SETUP,
	PAGE_SSP_SETUP,
	PAGE_FAILURE,
	PAGE_FINISHING,
	PAGE_SUMMARY
};

static gboolean set_page_search_complete(void);

static BluetoothClient *client;
static BluetoothAgent *agent;

static gchar *target_address = NULL;
static gchar *target_name = NULL;
static gchar *target_pincode = NULL;
static guint target_type = BLUETOOTH_TYPE_ANY;
static gboolean target_ssp = FALSE;
static gboolean create_started = FALSE;
static gboolean display_called = FALSE;

/* NULL means automatic, anything else is a pincode specified by the user */
static gchar *user_pincode = NULL;
/* If TRUE, then we won't display the PIN code to the user when pairing */
static gboolean automatic_pincode = FALSE;
static char *pincode = NULL;

static GtkBuilder *builder = NULL;

static GtkAssistant *window_assistant = NULL;
static GtkWidget *page_search = NULL;
static GtkWidget *page_connecting = NULL;
static GtkWidget *page_setup = NULL;
static GtkWidget *page_ssp_setup = NULL;
static GtkWidget *page_failure = NULL;
static GtkWidget *page_finishing = NULL;
static GtkWidget *page_summary = NULL;

static GtkWidget *label_connecting = NULL;
static GtkWidget *spinner_connecting = NULL;

static GtkWidget *label_pin = NULL;
static GtkWidget *label_pin_help = NULL;

static GtkWidget *label_ssp_pin_help = NULL;
static GtkWidget *label_ssp_pin = NULL;
static GtkWidget *does_not_match_button = NULL;
static GtkWidget *matches_button = NULL;

static GtkWidget *label_failure = NULL;

static GtkWidget *label_finishing = NULL;
static GtkWidget *spinner_finishing = NULL;

static GtkWidget *label_summary = NULL;
static GtkWidget *extra_config_vbox = NULL;

static BluetoothChooser *selector = NULL;

static GtkWidget *pin_dialog = NULL;
static GtkWidget *radio_auto = NULL;
static GtkWidget *radio_0000 = NULL;
static GtkWidget *radio_1111 = NULL;
static GtkWidget *radio_1234 = NULL;
static GtkWidget *radio_none = NULL;
static GtkWidget *radio_custom = NULL;
static GtkWidget *entry_custom = NULL;

/* Signals */
void close_callback(GtkWidget *assistant, gpointer data);
void prepare_callback(GtkWidget *assistant, GtkWidget *page, gpointer data);
void select_device_changed(BluetoothChooser *selector, const char *address, gpointer user_data);
gboolean entry_custom_event(GtkWidget *entry, GdkEventKey *event);
void set_user_pincode(GtkWidget *button);
void toggle_set_sensitive(GtkWidget *button, gpointer data);
void pin_option_button_clicked (GtkButton *button, gpointer data);
void entry_custom_changed(GtkWidget *entry);
void restart_button_clicked (GtkButton *button, gpointer user_data);
void does_not_match_cb (GtkButton *button, gpointer user_data);
void matches_cb (GtkButton *button, gpointer user_data);

static void
set_large_label (GtkLabel *label, const char *text)
{
	char *str;

	str = g_strdup_printf("<span font_desc=\"50\" color=\"black\" bgcolor=\"white\">  %s  </span>", text);
	gtk_label_set_markup(GTK_LABEL(label), str);
	g_free(str);
}

static void
update_random_pincode (void)
{
	target_pincode = g_strdup_printf ("%d", g_random_int_range (pow (10, PIN_NUM_DIGITS - 1),
								    pow (10, PIN_NUM_DIGITS) - 1));
	automatic_pincode = FALSE;
	g_free (user_pincode);
	user_pincode = NULL;
}

static gboolean
pincode_callback (DBusGMethodInvocation *context,
		  DBusGProxy *device,
		  gpointer user_data)
{
	target_ssp = FALSE;

	/* Only show the pincode page if the pincode isn't automatic */
	if (automatic_pincode == FALSE)
		gtk_assistant_set_current_page (window_assistant, PAGE_SETUP);
	dbus_g_method_return(context, pincode);

	return TRUE;
}

void
restart_button_clicked (GtkButton *button,
			gpointer user_data)
{
	/* Clean up old state */
	update_random_pincode ();
	target_ssp = FALSE;
	target_type = BLUETOOTH_TYPE_ANY;
	display_called = FALSE;
	g_free (target_address);
	target_address = NULL;
	g_free (target_name);
	target_name = NULL;

	g_object_set (selector,
		      "device-category-filter", BLUETOOTH_CATEGORY_NOT_PAIRED_OR_TRUSTED,
		      NULL);

	gtk_assistant_set_current_page (window_assistant, PAGE_SEARCH);
}

void
does_not_match_cb (GtkButton *button,
		   gpointer user_data)
{
	DBusGMethodInvocation *context;
	GError *error = NULL;
	char *text;

	gtk_assistant_set_current_page (window_assistant, PAGE_FAILURE);

	/* translators:
	 * The '%s' is the device name, for example:
	 * Pairing with 'Sony Bluetooth Headset' cancelled
	 */
	text = g_strdup_printf(_("Pairing with '%s' cancelled"), target_name);
	gtk_label_set_text(GTK_LABEL(label_failure), text);
	g_free(text);

	context = g_object_get_data (G_OBJECT (button), "context");
	g_error_new(AGENT_ERROR, AGENT_ERROR_REJECT,
		    "Agent callback cancelled");
	dbus_g_method_return(context, error);

	g_object_set_data (G_OBJECT(does_not_match_button), "context", NULL);
	g_object_set_data (G_OBJECT(matches_button), "context", NULL);
}

void
matches_cb (GtkButton *button,
	    gpointer user_data)
{
	DBusGMethodInvocation *context;

	context = g_object_get_data (G_OBJECT (button), "context");
	gtk_widget_set_sensitive (does_not_match_button, FALSE);
	gtk_widget_set_sensitive (matches_button, FALSE);
	dbus_g_method_return(context, "");

	g_object_set_data (G_OBJECT(does_not_match_button), "context", NULL);
	g_object_set_data (G_OBJECT(matches_button), "context", NULL);
}

static gboolean
confirm_callback (DBusGMethodInvocation *context,
		  DBusGProxy *device,
		  guint pin,
		  gpointer user_data)
{
	char *str, *label;

	target_ssp = TRUE;
	gtk_assistant_set_current_page (window_assistant, PAGE_SSP_SETUP);

	gtk_widget_show (label_ssp_pin_help);
	label = g_strdup_printf (_("Please confirm that the PIN displayed on '%s' matches this one."),
				 target_name);
	gtk_label_set_markup(GTK_LABEL(label_ssp_pin_help), label);
	g_free (label);

	gtk_widget_show (label_ssp_pin);
	str = g_strdup_printf ("%d", pin);
	set_large_label (GTK_LABEL (label_ssp_pin), str);
	g_free (str);

	g_object_set_data (G_OBJECT(does_not_match_button), "context", context);
	g_object_set_data (G_OBJECT(matches_button), "context", context);

	return TRUE;
}

static gboolean
display_callback (DBusGMethodInvocation *context,
		  DBusGProxy *device,
		  guint pin,
		  guint entered,
		  gpointer user_data)
{
	gchar *text, *done, *code;

	display_called = TRUE;
	target_ssp = TRUE;
	gtk_assistant_set_current_page (window_assistant, PAGE_SSP_SETUP);

	code = g_strdup_printf("%d", pin);

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

	gtk_widget_show (label_pin_help);

	gtk_label_set_markup(GTK_LABEL(label_ssp_pin_help), _("Please enter the following PIN:"));
	text = g_strdup_printf("%s%s", done, code + entered);
	set_large_label (GTK_LABEL (label_ssp_pin), text);
	g_free(text);

	g_free(done);
	g_free(code);

	dbus_g_method_return(context);

	return TRUE;
}

static gboolean
cancel_callback (DBusGMethodInvocation *context,
		 gpointer user_data)
{
	gchar *text;

	create_started = FALSE;

	gtk_assistant_set_current_page (window_assistant, PAGE_FAILURE);

	/* translators:
	 * The '%s' is the device name, for example:
	 * Pairing with 'Sony Bluetooth Headset' cancelled
	 */
	text = g_strdup_printf(_("Pairing with '%s' cancelled"), target_name);
	gtk_label_set_text(GTK_LABEL(label_failure), text);
	g_free(text);

	dbus_g_method_return(context);

	return TRUE;
}

typedef struct {
	char *path;
	GTimer *timer;
} ConnectData;

static void
connect_callback (BluetoothClient *_client,
		  gboolean success,
		  gpointer user_data)
{
	ConnectData *data = (ConnectData *) user_data;

	if (success == FALSE && g_timer_elapsed (data->timer, NULL) < CONNECT_TIMEOUT) {
		if (bluetooth_client_connect_service(client, data->path, connect_callback, data) != FALSE)
			return;
	}

	if (success == FALSE)
		g_message ("Failed to connect to device %s", data->path);

	g_timer_destroy (data->timer);
	g_free (data->path);
	g_free (data);

	gtk_assistant_set_current_page (window_assistant, PAGE_SUMMARY);
}

static void
create_callback (BluetoothClient *_client,
		 const char *path,
		 const GError *error,
		 gpointer user_data)
{
	ConnectData *data;

	create_started = FALSE;

	/* Create failed */
	if (path == NULL) {
		char *text;

		gtk_assistant_set_current_page (window_assistant, PAGE_FAILURE);

		/* translators:
		 * The '%s' is the device name, for example:
		 * Setting up 'Sony Bluetooth Headset' failed
		 */
		text = g_strdup_printf(_("Setting up '%s' failed"), target_name);

		gtk_label_set_markup(GTK_LABEL(label_failure), text);
		return;
	}

	bluetooth_client_set_trusted(client, path, TRUE);

	data = g_new0 (ConnectData, 1);
	data->path = g_strdup (path);
	data->timer = g_timer_new ();

	if (bluetooth_client_connect_service(client, path, connect_callback, data) != FALSE) {
		gtk_assistant_set_current_page (window_assistant, PAGE_FINISHING);
	} else {
		gtk_assistant_set_current_page (window_assistant, PAGE_SUMMARY);
		g_timer_destroy (data->timer);
		g_free (data->path);
		g_free (data);
	}
}

void
close_callback (GtkWidget *assistant,
		gpointer data)
{
	gtk_widget_destroy(assistant);

	gtk_main_quit();
}

/* HACK, to access the GtkAssistant buttons */
struct RealGtkAssistant
{
	GtkWindow  parent;

	GtkWidget *cancel;
	GtkWidget *forward;
	GtkWidget *back;
	GtkWidget *apply;
	GtkWidget *close;
	GtkWidget *last;

	/*< private >*/
	GtkAssistantPrivate *priv;
};
typedef struct RealGtkAssistant RealGtkAssistant;

static gboolean
prepare_idle_cb (gpointer data)
{
	RealGtkAssistant *assistant = (RealGtkAssistant *) window_assistant;
	gint page;

	page = gtk_assistant_get_current_page (GTK_ASSISTANT (window_assistant));
	if (page == PAGE_FAILURE) {
		gtk_widget_hide (assistant->cancel);
		gtk_widget_hide (assistant->forward);
		gtk_widget_hide (assistant->back);
		gtk_widget_hide (assistant->apply);
		gtk_widget_hide (assistant->last);
		gtk_widget_show (assistant->close);
		gtk_widget_set_sensitive (assistant->close, TRUE);
	}
	if (page == PAGE_CONNECTING) {
		gtk_widget_hide (assistant->forward);
		gtk_widget_hide (assistant->back);
		gtk_widget_hide (assistant->apply);
		gtk_widget_hide (assistant->last);
		gtk_widget_hide (assistant->close);
		gtk_widget_show (assistant->cancel);
		gtk_widget_set_sensitive (assistant->cancel, TRUE);
	}
	if (page == PAGE_SETUP) {
		gtk_widget_hide (assistant->forward);
		gtk_widget_hide (assistant->back);
		gtk_widget_hide (assistant->apply);
		gtk_widget_hide (assistant->last);
		gtk_widget_hide (assistant->close);
		gtk_widget_show (assistant->cancel);
		gtk_widget_set_sensitive (assistant->cancel, TRUE);
	}
	if (page == PAGE_SSP_SETUP) {
		gtk_widget_hide (assistant->forward);
		gtk_widget_hide (assistant->back);
		gtk_widget_hide (assistant->apply);
		gtk_widget_hide (assistant->last);
		gtk_widget_hide (assistant->close);
		if (display_called == FALSE) {
			gtk_widget_hide (assistant->cancel);
		} else {
			gtk_widget_show (assistant->cancel);
			gtk_widget_set_sensitive (assistant->cancel, TRUE);
		}
	}
	if (page == PAGE_FINISHING) {
		gtk_widget_hide (assistant->forward);
		gtk_widget_hide (assistant->back);
		gtk_widget_hide (assistant->apply);
		gtk_widget_hide (assistant->last);
		gtk_widget_hide (assistant->cancel);
		gtk_widget_show (assistant->close);
		gtk_widget_set_sensitive (assistant->close, FALSE);
	}
	return FALSE;
}

void prepare_callback (GtkWidget *assistant,
		       GtkWidget *page,
		       gpointer data)
{
	gboolean complete = TRUE;

	if (page == page_search) {
		complete = set_page_search_complete ();
		bluetooth_chooser_start_discovery(selector);
	} else {
		bluetooth_chooser_stop_discovery(selector);
	}

	if (page == page_connecting) {
		char *text;

		complete = FALSE;

		gtk_spinner_start (GTK_SPINNER (spinner_connecting));

		/* translators:
		 * The '%s' is the device name, for example:
		 * Connecting to 'Sony Bluetooth Headset' now...
		 */
		text = g_strdup_printf (_("Connecting to '%s'..."), target_name);
		gtk_label_set_text (GTK_LABEL (label_connecting), text);
		g_free (text);
	} else {
		gtk_spinner_stop (GTK_SPINNER (spinner_connecting));
	}

	if ((page == page_setup || page == page_connecting) && (create_started == FALSE)) {
		const char *path = AGENT_PATH;
		char *pin_ret;

		/* Set the filter on the selector, so we can use it to get more
		 * info later, in page_summary */
		g_object_set (selector,
			      "device-category-filter", BLUETOOTH_CATEGORY_ALL,
			      NULL);

		/* Do we pair, or don't we? */
		pin_ret = get_pincode_for_device (target_type, target_address, target_name, NULL);
		if (pin_ret != NULL && g_str_equal (pin_ret, "NULL"))
			path = NULL;
		g_free (pin_ret);

		g_object_ref(agent);
		bluetooth_client_create_device (client, target_address,
						path, create_callback, assistant);
		create_started = TRUE;
	}

	if (page == page_setup) {
		complete = FALSE;

		if (automatic_pincode == FALSE && target_ssp == FALSE) {
			char *text;

			if (target_type == BLUETOOTH_TYPE_KEYBOARD) {
				text = g_strdup_printf (_("Please enter the following PIN on '%s' and press “Enter” on the keyboard:"), target_name);
			} else {
				text = g_strdup_printf (_("Please enter the following PIN on '%s':"), target_name);
			}
			gtk_label_set_markup(GTK_LABEL(label_pin_help), text);
			g_free (text);
			set_large_label (GTK_LABEL (label_pin), pincode);
		} else {
			g_assert_not_reached ();
		}
	}

	if (page == page_finishing) {
		char *text;

		complete = FALSE;

		gtk_spinner_start (GTK_SPINNER (spinner_finishing));

		/* translators:
		 * The '%s' is the device name, for example:
		 * Please wait while finishing setup on 'Sony Bluetooth Headset'...
		 */
		text = g_strdup_printf (_("Please wait while finishing setup on device '%s'..."), target_name);
		gtk_label_set_text (GTK_LABEL (label_finishing), text);
		g_free (text);
	} else {
		gtk_spinner_stop (GTK_SPINNER (spinner_finishing));
	}

	if (page == page_summary) {
		GList *widgets = NULL;
		GValue value = { 0, };
		char **uuids, *text;

		/* FIXME remove this code when bluetoothd has pair/unpair code */
		g_object_set (G_OBJECT (selector), "device-selected", target_address, NULL);

		bluetooth_chooser_get_selected_device_info (selector, "name", &value);
		text = g_strdup_printf (_("Successfully set up new device '%s'"), g_value_get_string (&value));
		g_value_unset (&value);
		gtk_label_set_text (GTK_LABEL (label_summary), text);
		g_free (text);

		if (bluetooth_chooser_get_selected_device_info (selector, "uuids", &value) != FALSE) {
			uuids = g_value_get_boxed (&value);
			widgets = bluetooth_plugin_manager_get_widgets (target_address,
									(const char **) uuids);
			g_value_unset (&value);
		}
		if (widgets != NULL) {
			GList *l;

			for (l = widgets; l != NULL; l = l->next) {
				GtkWidget *widget = l->data;
				gtk_box_pack_start (GTK_BOX (extra_config_vbox),
						    widget,
						    FALSE,
						    TRUE,
						    0);
			}
			g_list_free (widgets);
			gtk_widget_show_all (extra_config_vbox);
		}
	}

	/* Setup the buttons some */
	if (page == page_failure) {
		complete = FALSE;
		gtk_assistant_add_action_widget (GTK_ASSISTANT (assistant), W("restart_button"));
	} else {
		if (gtk_widget_get_parent (W("restart_button")) != NULL)
			gtk_assistant_remove_action_widget (GTK_ASSISTANT (assistant), W("restart_button"));
	}

	if (page == page_ssp_setup && display_called == FALSE) {
		complete = FALSE;
		gtk_assistant_add_action_widget (GTK_ASSISTANT (assistant), W("matches_button"));
		gtk_assistant_add_action_widget (GTK_ASSISTANT (assistant), W("does_not_match_button"));
	} else {
		if (gtk_widget_get_parent (W("does_not_match_button")) != NULL)
			gtk_assistant_remove_action_widget (GTK_ASSISTANT (assistant), W("does_not_match_button"));
		if (gtk_widget_get_parent (W("matches_button")) != NULL)
			gtk_assistant_remove_action_widget (GTK_ASSISTANT (assistant), W("matches_button"));
	}

	gtk_assistant_set_page_complete (GTK_ASSISTANT(assistant),
					 page, complete);

	/* HACK to allow hiding/showing the buttons
	 * instead of relying on the GtkAssistant doing that
	 * for us */
	g_idle_add (prepare_idle_cb, NULL);
}

static gboolean
set_page_search_complete (void)
{
	char *name, *address;
	gboolean complete = FALSE;

	address = bluetooth_chooser_get_selected_device (selector);
	name = bluetooth_chooser_get_selected_device_name (selector);

	if (address == NULL)
		complete = FALSE;
	else if (name == NULL)
		complete = (user_pincode != NULL && strlen(user_pincode) >= 4);
	else
		complete = (user_pincode == NULL || strlen(user_pincode) >= 4);

	g_free (address);
	g_free (name);

	gtk_assistant_set_page_complete (GTK_ASSISTANT (window_assistant),
					 page_search, complete);

	return complete;
}

gboolean
entry_custom_event (GtkWidget *entry, GdkEventKey *event)
{
	if (event->length == 0)
		return FALSE;

	if ((event->keyval >= GDK_0 && event->keyval <= GDK_9) ||
	    (event->keyval >= GDK_KP_0 && event->keyval <= GDK_KP_9))
		return FALSE;

	return TRUE;
}

void
entry_custom_changed (GtkWidget *entry)
{
	user_pincode = g_strdup (gtk_entry_get_text(GTK_ENTRY(entry)));
	gtk_dialog_set_response_sensitive (GTK_DIALOG (pin_dialog),
					   GTK_RESPONSE_ACCEPT,
					   gtk_entry_get_text_length (GTK_ENTRY (entry)) >= 1);
}

void
toggle_set_sensitive (GtkWidget *button,
		      gpointer data)
{
	gboolean active;

	active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
	gtk_widget_set_sensitive(entry_custom, active);
	/* When selecting another PIN, make sure the "Close" button is sensitive */
	if (!active)
		gtk_dialog_set_response_sensitive (GTK_DIALOG (pin_dialog),
						   GTK_RESPONSE_ACCEPT, TRUE);
}

void
set_user_pincode (GtkWidget *button)
{
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
			set_user_pincode (radio);
			return;
		}

		pin = g_object_get_data (G_OBJECT (button), "pin");
		entry = g_object_get_data (G_OBJECT (button), "entry");

		if (entry != NULL) {
			g_free (user_pincode);
			user_pincode = g_strdup (gtk_entry_get_text(entry));
			gtk_dialog_set_response_sensitive (GTK_DIALOG (pin_dialog),
							   GTK_RESPONSE_ACCEPT,
							   gtk_entry_get_text_length (entry) >= 1);
		} else if (pin != NULL) {
			g_free (user_pincode);
			if (*pin == '\0')
				user_pincode = NULL;
			else
				user_pincode = g_strdup (pin);
		}

		break;
	}
}

void
select_device_changed (BluetoothChooser *selector,
		       const char *address,
		       gpointer user_data)
{
	GValue value = { 0, };
	char *name;
	int legacypairing;
	BluetoothType type;

	if (gtk_assistant_get_current_page (GTK_ASSISTANT (window_assistant)) != PAGE_SEARCH)
		return;

	set_page_search_complete ();

	name = bluetooth_chooser_get_selected_device_name (selector);
	type = bluetooth_chooser_get_selected_device_type (selector);
	if (bluetooth_chooser_get_selected_device_info (selector, "legacypairing", &value) != FALSE) {
		legacypairing = g_value_get_int (&value);
		if (legacypairing == -1)
			legacypairing = TRUE;
	} else {
		legacypairing = TRUE;
	}

	g_free(target_address);
	target_address = g_strdup (address);

	g_free(target_name);
	target_name = name;

	target_type = type;
	target_ssp = !legacypairing;
	automatic_pincode = FALSE;

	g_free (pincode);
	pincode = NULL;

	if (user_pincode != NULL && *user_pincode != '\0') {
		pincode = g_strdup (user_pincode);
		automatic_pincode = TRUE;
	} else if (address != NULL) {
		guint max_digits;

		pincode = get_pincode_for_device(target_type, target_address, target_name, &max_digits);
		if (pincode == NULL) {
			/* Truncate the default pincode if the device doesn't like long
			 * PIN codes */
			if (max_digits != PIN_NUM_DIGITS && max_digits > 0)
				pincode = g_strndup(target_pincode, max_digits);
			else
				pincode = g_strdup(target_pincode);
		} else if (target_ssp == FALSE) {
			automatic_pincode = TRUE;
		}
	}
}

void
pin_option_button_clicked (GtkButton *button,
			       gpointer data)
{
	GtkWidget *radio;
	char *oldpin;

	oldpin = user_pincode;
	user_pincode = NULL;

	gtk_window_set_transient_for (GTK_WINDOW (pin_dialog),
				      GTK_WINDOW (window_assistant));
	gtk_window_present (GTK_WINDOW (pin_dialog));

	/* When reopening, try to guess where the pincode was set */
	if (oldpin == NULL)
		radio = radio_auto;
	else if (g_str_equal (oldpin, "0000"))
		radio = radio_0000;
	else if (g_str_equal (oldpin, "1111"))
		radio = radio_1111;
	else if (g_str_equal (oldpin, "1234"))
		radio = radio_1234;
	else
		radio = radio_custom;

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio), TRUE);
	if (radio == radio_custom)
		gtk_entry_set_text (GTK_ENTRY (entry_custom), oldpin);

	if (gtk_dialog_run (GTK_DIALOG (pin_dialog)) != GTK_RESPONSE_ACCEPT) {
		g_free (user_pincode);
		user_pincode = oldpin;
	} else {
		g_free (oldpin);
	}

	gtk_widget_hide(pin_dialog);
}

static int
page_func (gint current_page,
	   gpointer data)
{
	if (current_page == PAGE_SEARCH) {
		if (target_ssp != FALSE || automatic_pincode != FALSE)
			return PAGE_CONNECTING;
		else
			return PAGE_SETUP;
	}
	if (current_page == PAGE_SETUP)
		return PAGE_SUMMARY;
	return current_page + 1;
}

static GtkAssistant *
create_wizard (void)
{
	const char *pages[] = {
		"page_intro",
		"page_search",
		"page_connecting",
		"page_setup",
		"page_ssp_setup",
		"page_failure",
		"page_finishing",
		"page_summary",
	};
	guint i;

	GtkAssistant *assistant;
	GError *err = NULL;
	GtkWidget *combo, *page_intro;
	GtkTreeModel *model;
	GtkCellRenderer *renderer;
	GdkPixbuf *pixbuf;
	int height;

	builder = gtk_builder_new ();
	if (gtk_builder_add_from_file (builder, "wizard.ui", NULL) == 0) {
		if (gtk_builder_add_from_file (builder, PKGDATADIR "/wizard.ui", &err) == 0) {
			g_warning ("Could not load UI from %s: %s", PKGDATADIR "/wizard.ui", err->message);
			g_error_free(err);
			return NULL;
		}
	}

	assistant = GTK_ASSISTANT(gtk_builder_get_object(builder, "assistant"));

	gtk_assistant_set_forward_page_func (assistant, page_func, NULL, NULL);

	/* Intro page */
	combo = gtk_combo_box_new();

	model = bluetooth_client_get_adapter_model(client);
	gtk_combo_box_set_model(GTK_COMBO_BOX(combo), model);
	g_object_unref(model);

	gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
	gtk_cell_layout_clear(GTK_CELL_LAYOUT(combo));

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), renderer,
					"text", BLUETOOTH_COLUMN_NAME, NULL);

	page_intro = W("page_intro");
	if (gtk_tree_model_iter_n_children(model, NULL) > 1)
		gtk_box_pack_start(GTK_BOX(page_intro), combo, FALSE, FALSE, 0);

	/* Search page */
	page_search = W("page_search");
	selector = BLUETOOTH_CHOOSER (gtk_builder_get_object (builder, "selector"));

	/* Connecting page */
	page_connecting = W("page_connecting");
	label_connecting = W("label_connecting");
	spinner_connecting = W("spinner_connecting");

	/* Setup page */
	page_setup = W("page_setup");
	label_pin_help = W("label_pin_help");
	label_pin = W("label_pin");

	/* SSP Setup page */
	page_ssp_setup = W("page_ssp_setup");
	gtk_assistant_set_page_complete(assistant, page_ssp_setup, FALSE);
	label_ssp_pin_help = W("label_ssp_pin_help");
	label_ssp_pin = W("label_ssp_pin");
	does_not_match_button = W("does_not_match_button");
	matches_button = W("matches_button");

	/* Failure page */
	page_failure = W("page_failure");
	gtk_assistant_set_page_complete(assistant, page_failure, FALSE);
	label_failure = W("label_failure");

	/* Finishing page */
	page_finishing = W("page_finishing");
	label_finishing = W("label_finishing");
	spinner_finishing = W("spinner_finishing");

	/* Summary page */
	page_summary = W("page_summary");
	label_summary = W("label_summary");
	extra_config_vbox = W("extra_config_vbox");

	/* Set page icons */
	gtk_icon_size_lookup (GTK_ICON_SIZE_DIALOG, NULL, &height);
	pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
					   "bluetooth", height, 0, NULL);
	for (i = 0; i < G_N_ELEMENTS (pages); i++)
		gtk_assistant_set_page_header_image (assistant, W(pages[i]), pixbuf);
	if (pixbuf != NULL)
		g_object_unref (pixbuf);

	/* PIN dialog */
	pin_dialog = W("pin_dialog");
	radio_auto = W("radio_auto");
	radio_0000 = W("radio_0000");
	radio_1111 = W("radio_1111");
	radio_1234 = W("radio_1234");
	radio_none = W("radio_none");
	radio_custom = W("radio_custom");
	entry_custom = W("entry_custom");

	g_object_set_data (G_OBJECT (radio_auto), "pin", "");
	g_object_set_data (G_OBJECT (radio_0000), "pin", "0000");
	g_object_set_data (G_OBJECT (radio_1111), "pin", "1111");
	g_object_set_data (G_OBJECT (radio_1234), "pin", "1234");
	g_object_set_data (G_OBJECT (radio_none), "pin", "NULL");
	g_object_set_data (G_OBJECT (radio_custom), "entry", entry_custom);

	gtk_builder_connect_signals(builder, NULL);

	gtk_widget_show (GTK_WIDGET(assistant));

	gtk_assistant_update_buttons_state(GTK_ASSISTANT(assistant));

	return assistant;
}

static UniqueResponse
message_received_cb (UniqueApp         *app,
                     int                command,
                     UniqueMessageData *message_data,
                     guint              time_,
                     gpointer           user_data)
{
        gtk_window_present (GTK_WINDOW (user_data));

        return UNIQUE_RESPONSE_OK;
}

static GOptionEntry options[] = {
	{ NULL },
};

int main (int argc, char **argv)
{
	UniqueApp *app;
	GError *error = NULL;

	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

	if (gtk_init_with_args(&argc, &argv, NULL,
				options, GETTEXT_PACKAGE, &error) == FALSE) {
		if (error) {
			g_print("%s\n", error->message);
			g_error_free(error);
		} else
			g_print("An unknown error occurred\n");

		return 1;
	}

	app = unique_app_new ("org.mate.Bluetooth.wizard", NULL);
	if (unique_app_is_running (app)) {
		gdk_notify_startup_complete ();
		unique_app_send_message (app, UNIQUE_ACTIVATE, NULL);
		return 0;
	}

	gtk_window_set_default_icon_name("bluetooth");

	update_random_pincode ();

	client = bluetooth_client_new();

	agent = bluetooth_agent_new();

	bluetooth_agent_set_pincode_func(agent, pincode_callback, NULL);
	bluetooth_agent_set_display_func(agent, display_callback, NULL);
	bluetooth_agent_set_cancel_func(agent, cancel_callback, NULL);
	bluetooth_agent_set_confirm_func(agent, confirm_callback, NULL);

	bluetooth_agent_setup(agent, AGENT_PATH);

	bluetooth_plugin_manager_init ();

	window_assistant = create_wizard();
	if (window_assistant == NULL)
		return 1;

	g_signal_connect (app, "message-received",
			  G_CALLBACK (message_received_cb), window_assistant);

	gtk_main();

	bluetooth_plugin_manager_cleanup ();

	g_object_unref(agent);

	g_object_unref(client);

	g_object_unref(app);

	return 0;
}

