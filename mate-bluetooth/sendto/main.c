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

#include <sys/time.h>

#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <obex-agent.h>
#include <bluetooth-client.h>
#include <bluetooth-chooser.h>
#include "marshal.h"

#define AGENT_PATH "/org/bluez/agent/sendto"

#define RESPONSE_RETRY 1

static DBusGConnection *conn = NULL;
static ObexAgent *agent = NULL;
static DBusGProxy *client_proxy = NULL;

static GtkWidget *dialog;
static GtkWidget *label_from;
static GtkWidget *image_status;
static GtkWidget *label_status;
static GtkWidget *progress;

static gchar *option_device = NULL;
static gchar *option_device_name = NULL;
static gchar **option_files = NULL;

static DBusGProxy *current_transfer = NULL;
static guint64 current_size = 0;
static guint64 total_size = 0;
static guint64 total_sent = 0;

static int file_count = 0;
static int file_index = 0;

static gint64 first_update = 0;
static gint64 last_update = 0;

static void send_notify(DBusGProxy *proxy, DBusGProxyCall *call, void *user_data);

/* Agent callbacks */
static gboolean release_callback(DBusGMethodInvocation *context, gpointer user_data);
static gboolean request_callback(DBusGMethodInvocation *context, DBusGProxy *transfer, gpointer user_data);
static gboolean progress_callback(DBusGMethodInvocation *context,
				  DBusGProxy *transfer,
				  guint64 transferred,
				  gpointer user_data);
static gboolean complete_callback(DBusGMethodInvocation *context, DBusGProxy *transfer, gpointer user_data);
static gboolean error_callback(DBusGMethodInvocation *context,
			       DBusGProxy *transfer,
			       const char *message,
			       gpointer user_data);

static void value_free(GValue *value)
{
	g_value_unset(value);
	g_free(value);
}

static GHashTable *
send_files (void)
{
	GHashTable *hash;
	GValue *value;

	hash = g_hash_table_new_full(g_str_hash, g_str_equal,
				     g_free, (GDestroyNotify) value_free);

	value = g_new0(GValue, 1);
	g_value_init(value, G_TYPE_STRING);
	g_value_set_string(value, option_device);
	g_hash_table_insert(hash, g_strdup("Destination"), value);

	dbus_g_proxy_begin_call(client_proxy, "SendFiles",
				send_notify, NULL, NULL,
				dbus_g_type_get_map("GHashTable", G_TYPE_STRING, G_TYPE_VALUE), hash,
				G_TYPE_STRV, option_files,
				DBUS_TYPE_G_OBJECT_PATH, AGENT_PATH,
				G_TYPE_INVALID);

	return hash;
}

static void
setup_agent (void)
{
	if (agent == NULL)
		agent = obex_agent_new();

	obex_agent_set_release_func(agent, release_callback, NULL);
	obex_agent_set_request_func(agent, request_callback, NULL);
	obex_agent_set_progress_func(agent, progress_callback, NULL);
	obex_agent_set_complete_func(agent, complete_callback, NULL);
	obex_agent_set_error_func(agent, error_callback, NULL);

	obex_agent_setup(agent, AGENT_PATH);
}

static gchar *filename_to_path(const gchar *filename)
{
	GFile *file;
	gchar *path;

	file = g_file_new_for_commandline_arg(filename);
	path = g_file_get_path(file);
	g_object_unref(file);

	return path;
}

static gint64 get_system_time(void)
{
	struct timeval tmp;

	gettimeofday(&tmp, NULL);

	return (gint64) tmp.tv_usec +
			(gint64) tmp.tv_sec * G_GINT64_CONSTANT(1000000);
}

static gchar *format_time(gint seconds)
{
	gint hours, minutes;

	if (seconds < 0)
		seconds = 0;

	if (seconds < 60)
		return g_strdup_printf(ngettext("%'d second",
					"%'d seconds", seconds), seconds);

	if (seconds < 60 * 60) {
		minutes = (seconds + 30) / 60;
		return g_strdup_printf(ngettext("%'d minute",
					"%'d minutes", minutes), minutes);
	}

	hours = seconds / (60 * 60);

	if (seconds < 60 * 60 * 4) {
		gchar *res, *h, *m;

		minutes = (seconds - hours * 60 * 60 + 30) / 60;

		h = g_strdup_printf(ngettext("%'d hour",
					"%'d hours", hours), hours);
		m = g_strdup_printf(ngettext("%'d minute",
					"%'d minutes", minutes), minutes);
		res = g_strconcat(h, ", ", m, NULL);
		g_free(h);
		g_free(m);
		return res;
	}

	return g_strdup_printf(ngettext("approximately %'d hour",
				"approximately %'d hours", hours), hours);
}

static void
set_response_visible (GtkDialog *dialog,
		      int response_id,
		      gboolean visible)
{
	GtkWidget *widget;

	widget = gtk_dialog_get_widget_for_response (dialog, response_id);
	gtk_widget_set_no_show_all (widget, TRUE);
	gtk_widget_set_visible (widget, visible);
}

static void response_callback(GtkWidget *dialog,
					gint response, gpointer user_data)
{
	if (response == RESPONSE_RETRY) {
		setup_agent ();

		/* Reset buttons */
		set_response_visible (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE, FALSE);
		set_response_visible (GTK_DIALOG (dialog), RESPONSE_RETRY, FALSE);
		set_response_visible (GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL, TRUE);

		/* Reset status and progress bar */
		gtk_progress_bar_set_text (GTK_PROGRESS_BAR (progress),
					  _("Connecting..."));
		gtk_label_set_text (GTK_LABEL (label_status), "");
		gtk_widget_hide (image_status);
		send_files ();
		return;
	}

	if (current_transfer != NULL) {
		obex_agent_set_error_func(agent, NULL, NULL);
		dbus_g_proxy_call_no_reply (current_transfer, "Cancel", G_TYPE_INVALID);
		g_object_unref (current_transfer);
		current_transfer = NULL;
	}

	gtk_widget_destroy(dialog);
	gtk_main_quit();
}

static gboolean is_palm_device(const gchar *bdaddr)
{
	return (g_str_has_prefix(bdaddr, "00:04:6B") ||
			g_str_has_prefix(bdaddr, "00:07:E0") ||
				g_str_has_prefix(bdaddr, "00:0E:20"));
}

static void create_window(void)
{
	GtkWidget *vbox, *hbox;
	GtkWidget *table;
	GtkWidget *label;
	gchar *text;

	dialog = gtk_dialog_new_with_buttons(_("File Transfer"), NULL,
				GTK_DIALOG_NO_SEPARATOR,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
				_("_Retry"), RESPONSE_RETRY,
				NULL);
	set_response_visible (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE, FALSE);
	set_response_visible (GTK_DIALOG (dialog), RESPONSE_RETRY, FALSE);
	gtk_window_set_type_hint(GTK_WINDOW(dialog),
						GDK_WINDOW_TYPE_HINT_NORMAL);
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(dialog), 400, -1);
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 6);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_set_spacing(GTK_BOX(vbox), 6);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
	                   vbox);

	label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	text = g_markup_printf_escaped("<span size=\"larger\"><b>%s</b></span>",
	/* translators: This is the heading for the progress dialogue */
					_("Sending files via Bluetooth"));
	gtk_label_set_markup(GTK_LABEL(label), text);
	g_free(text);
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);

	table = gtk_table_new(2, 2, FALSE);
	gtk_table_set_col_spacings(GTK_TABLE(table), 4);
	gtk_table_set_row_spacings(GTK_TABLE(table), 4);
	gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 9);

	label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
	text = g_markup_printf_escaped("<b>%s</b>", _("From:"));
	gtk_label_set_markup(GTK_LABEL(label), text);
	g_free(text);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
							GTK_FILL, 0, 0, 0);

	label_from = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(label_from), 0, 0.5);
	gtk_label_set_ellipsize(GTK_LABEL(label_from), PANGO_ELLIPSIZE_MIDDLE);
	gtk_table_attach_defaults(GTK_TABLE(table), label_from, 1, 2, 0, 1);

	label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
	text = g_markup_printf_escaped("<b>%s</b>", _("To:"));
	gtk_label_set_markup(GTK_LABEL(label), text);
	g_free(text);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
							GTK_FILL, 0, 0, 0);

	label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
	gtk_label_set_text(GTK_LABEL(label), option_device_name);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 1, 2, 1, 2);

	progress = gtk_progress_bar_new();
	gtk_progress_bar_set_ellipsize(GTK_PROGRESS_BAR(progress),
							PANGO_ELLIPSIZE_END);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress),
							_("Connecting..."));
	gtk_box_pack_start(GTK_BOX(vbox), progress, TRUE, TRUE, 0);

	hbox = gtk_hbox_new (FALSE, 4);

	image_status = gtk_image_new_from_stock (GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_MENU);
	gtk_widget_set_no_show_all (image_status, TRUE);
	gtk_box_pack_start(GTK_BOX (hbox), image_status, FALSE, FALSE, 4);

	label_status = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(label_status), 0, 0.5);
	gtk_label_set_line_wrap(GTK_LABEL(label_status), TRUE);
	gtk_box_pack_start(GTK_BOX (hbox), label_status, TRUE, TRUE, 4);

	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 2);

	g_signal_connect(G_OBJECT(dialog), "response",
				G_CALLBACK(response_callback), NULL);

	gtk_widget_show_all(dialog);
}

#define OPENOBEX_CONNECTION_FAILED "org.openobex.Error.ConnectionAttemptFailed"

static gchar *get_error_message(GError *error)
{
	char *message;

	if (error == NULL)
		return g_strdup(_("An unknown error occurred"));

	if (error->code != DBUS_GERROR_REMOTE_EXCEPTION) {
		message = g_strdup(error->message);
		goto done;
	}

	if (dbus_g_error_has_name(error, OPENOBEX_CONNECTION_FAILED) == TRUE &&
					is_palm_device(option_device)) {
		message = g_strdup(_("Make sure that remote device "
					"is switched on and that it "
					"accepts Bluetooth connections"));
		goto done;
	}

	if (*error->message == '\0')
		message = g_strdup(_("An unknown error occurred"));
	else
		message = g_strdup(error->message);

done:
	g_error_free(error);

	return message;
}

static gchar *get_device_name(const gchar *address)
{
	BluetoothClient *client;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean cont;
	char *found_name;

	found_name = NULL;
	client = bluetooth_client_new ();
	model = bluetooth_client_get_model (client);
	if (model == NULL) {
		g_object_unref (client);
		return NULL;
	}

	cont = gtk_tree_model_get_iter_first(model, &iter);
	while (cont != FALSE) {
		char *bdaddr, *name;

		gtk_tree_model_get(model, &iter,
				   BLUETOOTH_COLUMN_ADDRESS, &bdaddr,
				   BLUETOOTH_COLUMN_ALIAS, &name,
				   -1);
		if (g_strcmp0 (bdaddr, address) == 0) {
			g_free (bdaddr);
			found_name = name;
			break;
		}
		g_free (bdaddr);
		g_free (name);

		cont = gtk_tree_model_iter_next(model, &iter);
	}

	g_object_unref (model);
	g_object_unref (client);

	return found_name;
}

static void get_properties_callback (DBusGProxy *proxy,
				     DBusGProxyCall *call,
				     void *user_data)
{
	GError *error = NULL;
	gchar *filename = option_files[file_index];
	GFile *file, *dir;
	gchar *basename, *text, *markup;
	GHashTable *hash;

	if (dbus_g_proxy_end_call (proxy, call, &error,
				   dbus_g_type_get_map("GHashTable", G_TYPE_STRING, G_TYPE_VALUE), &hash,
				   G_TYPE_INVALID) != FALSE) {
		GValue *value;

		value = g_hash_table_lookup(hash, "Size");
		if (value) {
			current_size = g_value_get_uint64(value);

			last_update = get_system_time();
		}

		g_hash_table_destroy(hash);
	}

	file = g_file_new_for_path (filename);
	dir = g_file_get_parent (file);
	g_object_unref (file);
	if (g_file_has_uri_scheme (dir, "file") != FALSE) {
		text = g_file_get_path (dir);
	} else {
		text = g_file_get_uri (dir);
	}
	markup = g_markup_escape_text (text, -1);
	g_free (text);
	g_object_unref (dir);
	gtk_label_set_markup(GTK_LABEL(label_from), markup);
	g_free(markup);

	basename = g_path_get_basename(filename);
	text = g_strdup_printf(_("Sending %s"), basename);
	g_free(basename);
	markup = g_markup_printf_escaped("<i>%s</i>", text);
	gtk_label_set_markup(GTK_LABEL(label_status), markup);
	g_free(markup);
	g_free(text);

	text = g_strdup_printf(_("Sending file %d of %d"),
						file_index + 1, file_count);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress), text);
	g_free(text);
}

static gboolean request_callback(DBusGMethodInvocation *context,
				DBusGProxy *transfer, gpointer user_data)
{
	g_assert (current_transfer == NULL);

	dbus_g_proxy_begin_call(transfer, "GetProperties",
				get_properties_callback, NULL, NULL,
				G_TYPE_INVALID);

	current_transfer = g_object_ref (transfer);

	dbus_g_method_return(context, "");

	return TRUE;
}

static gboolean progress_callback(DBusGMethodInvocation *context,
				DBusGProxy *transfer, guint64 transferred,
							gpointer user_data)
{
	gint64 current_time;
	gint elapsed_time;
	gint remaining_time;
	gint transfer_rate;
	guint64 current_sent;
	gdouble fraction;
	gchar *time, *rate, *file, *text;

	current_sent = total_sent + transferred;
	if (total_size == 0)
		fraction = 0.0;
	else
		fraction = (gdouble) current_sent / (gdouble) total_size;
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), fraction);

	current_time = get_system_time();
	elapsed_time = (current_time - first_update) / 1000000;

	if (current_time < last_update + 1000000)
		goto done;

	last_update = current_time;

	if (elapsed_time == 0)
		goto done;

	transfer_rate = current_sent / elapsed_time;

	if (transfer_rate == 0)
		goto done;

	remaining_time = (total_size - current_sent) / transfer_rate;

	time = format_time(remaining_time);

	if (transfer_rate >= 3000)
		rate = g_strdup_printf(_("%d KB/s"), transfer_rate / 1000);
	else
		rate = g_strdup_printf(_("%d B/s"), transfer_rate);

	file = g_strdup_printf(_("Sending file %d of %d"),
						file_index + 1, file_count);
	text = g_strdup_printf("%s (%s, %s)", file, rate, time);
	g_free(file);
	g_free(rate);
	g_free(time);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress), text);
	g_free(text);

done:
	dbus_g_method_return(context);

	return TRUE;
}

static gboolean complete_callback(DBusGMethodInvocation *context,
				DBusGProxy *transfer, gpointer user_data)
{
	total_sent += current_size;

	file_index++;

	/* And we're done with the transfer */
	g_object_unref (current_transfer);
	current_transfer = NULL;

	if (file_index == file_count)
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 1.0);

	dbus_g_method_return(context);

	return TRUE;
}

static gboolean release_callback(DBusGMethodInvocation *context,
							gpointer user_data)
{
	dbus_g_method_return(context);

	agent = NULL;

	gtk_label_set_markup(GTK_LABEL(label_status), NULL);

	gtk_widget_destroy(dialog);

	gtk_main_quit();

	return TRUE;
}

static gboolean error_callback(DBusGMethodInvocation *context,
			       DBusGProxy *transfer,
			       const char *message,
			       gpointer user_data)
{
	gtk_widget_show (image_status);
	gtk_label_set_markup(GTK_LABEL(label_status), message);

	set_response_visible (GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL, TRUE);
	set_response_visible (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE, FALSE);
	set_response_visible (GTK_DIALOG (dialog), RESPONSE_RETRY, TRUE);

	g_object_unref (current_transfer);
	current_transfer = NULL;

	if (agent != NULL) {
		obex_agent_set_release_func(agent, NULL, NULL);
		agent = NULL;
	}

	dbus_g_method_return(context);

	return TRUE;
}

static void send_notify(DBusGProxy *proxy,
				DBusGProxyCall *call, void *user_data)
{
	GError *error = NULL;

	if (dbus_g_proxy_end_call(proxy, call, &error,
						G_TYPE_INVALID) == FALSE) {
		char *message;

		message = get_error_message(error);
		gtk_widget_show (image_status);
		gtk_label_set_markup(GTK_LABEL(label_status), message);
		g_free (message);

		set_response_visible (GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL, TRUE);
		set_response_visible (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE, FALSE);
		set_response_visible (GTK_DIALOG (dialog), RESPONSE_RETRY, TRUE);
		return;
	}

	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress), NULL);

	first_update = get_system_time();
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

static char *
show_browse_dialog (char **device_name)
{
	GtkWidget *dialog, *selector, *send_button, *image, *content_area;
	char *bdaddr;
	int response_id;

	dialog = gtk_dialog_new_with_buttons(_("Select Device to Send To"), NULL,
					     GTK_DIALOG_NO_SEPARATOR,
					     GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
					     NULL);
	send_button = gtk_dialog_add_button (GTK_DIALOG (dialog), _("Send _To"), GTK_RESPONSE_ACCEPT);
	image = gtk_image_new_from_icon_name ("document-send", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image (GTK_BUTTON (send_button), image);
	gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog),
					  GTK_RESPONSE_ACCEPT, FALSE);
	gtk_window_set_default_size(GTK_WINDOW(dialog), 480, 400);

	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
	content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
	gtk_box_set_spacing (GTK_BOX (content_area), 2);

	selector = bluetooth_chooser_new("Select device to send files to");
	gtk_container_set_border_width(GTK_CONTAINER(selector), 5);
	gtk_widget_show(selector);
	g_object_set(selector,
		     "show-searching", TRUE,
		     "show-device-category", TRUE,
		     "show-device-type", TRUE,
		     NULL);
	g_signal_connect(selector, "selected-device-changed",
			 G_CALLBACK(select_device_changed), dialog);
	gtk_container_add (GTK_CONTAINER (content_area), selector);
	bluetooth_chooser_start_discovery (BLUETOOTH_CHOOSER (selector));

	bdaddr = NULL;
	response_id = gtk_dialog_run (GTK_DIALOG (dialog));
	if (response_id == GTK_RESPONSE_ACCEPT) {
		bdaddr = bluetooth_chooser_get_selected_device (BLUETOOTH_CHOOSER (selector));
		*device_name = bluetooth_chooser_get_selected_device_name (BLUETOOTH_CHOOSER (selector));
	}

	gtk_widget_destroy (dialog);

	return bdaddr;
}

static char **
show_select_dialog(void)
{
	GtkWidget *dialog;
	gchar **files = NULL;

	dialog = gtk_file_chooser_dialog_new(_("Choose files to send"), NULL,
				GTK_FILE_CHOOSER_ACTION_OPEN,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		GSList *list, *filenames;
		int i;

		filenames = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));

		files = g_new(gchar *, g_slist_length(filenames) + 1);

		for (list = filenames, i = 0; list; list = list->next, i++)
			files[i] = list->data;
		files[i] = NULL;

		g_slist_free(filenames);
	}

	gtk_widget_destroy(dialog);

	return files;
}

static GOptionEntry options[] = {
	{ "device", 0, 0, G_OPTION_ARG_STRING, &option_device,
				N_("Remote device to use"), "ADDRESS" },
	{ "name", 0, 0, G_OPTION_ARG_STRING, &option_device_name,
				N_("Remote device's name"), NULL },
	{ "dest", 0, G_OPTION_FLAG_HIDDEN,
			G_OPTION_ARG_STRING, &option_device, NULL, NULL },
	{ G_OPTION_REMAINING, 0, 0,
			G_OPTION_ARG_FILENAME_ARRAY, &option_files },
	{ NULL },
};

int main(int argc, char *argv[])
{
	GError *error = NULL;
	int i;

	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

	error = NULL;

	if (gtk_init_with_args(&argc, &argv, "[FILE...]",
				options, GETTEXT_PACKAGE, &error) == FALSE) {
		if (error != NULL) {
			g_printerr("%s\n", error->message);
			g_error_free(error);
		} else
			g_printerr("An unknown error occurred\n");

		return 1;
	}

	gtk_window_set_default_icon_name("bluetooth");

	/* A device name, but no device? */
	if (option_device == NULL && option_device_name != NULL) {
		if (option_files != NULL)
			g_strfreev(option_files);
		g_free (option_device_name);
		return 1;
	}

	if (option_files == NULL) {
		option_files = show_select_dialog();
		if (option_files == NULL)
			return 1;
	}

	if (option_device == NULL) {
		option_device = show_browse_dialog(&option_device_name);
		if (option_device == NULL) {
			g_strfreev(option_files);
			return 1;
		}
	}

	file_count = g_strv_length(option_files);

	for (i = 0; i < file_count; i++) {
		gchar *filename;
		struct stat st;

		filename = filename_to_path(option_files[i]);

		if (filename != NULL) {
			g_free(option_files[i]);
			option_files[i] = filename;
		}

		if (g_file_test(option_files[i],
					G_FILE_TEST_IS_REGULAR) == FALSE) {
			option_files[i][0] = '\0';
			continue;
		}

		if (g_stat(option_files[i], &st) < 0)
			option_files[i][0] = '\0';
		else
			total_size += st.st_size;
	}

	conn = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
	if (conn == NULL) {
		if (error != NULL) {
			g_printerr("Connecting to session bus failed: %s\n",
							error->message);
			g_error_free(error);
		} else
			g_print("An unknown error occured\n");

		return 1;
	}

	dbus_g_object_register_marshaller(marshal_VOID__STRING_STRING,
					G_TYPE_NONE, G_TYPE_STRING,
					G_TYPE_STRING, G_TYPE_INVALID);

	dbus_g_object_register_marshaller(marshal_VOID__STRING_STRING_UINT64,
				G_TYPE_NONE, G_TYPE_STRING,
				G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_INVALID);

	dbus_g_object_register_marshaller(marshal_VOID__UINT64,
				G_TYPE_NONE, G_TYPE_UINT64, G_TYPE_INVALID);

	dbus_g_object_register_marshaller(marshal_VOID__STRING_STRING_STRING,
					  G_TYPE_NONE,
					  DBUS_TYPE_G_OBJECT_PATH,
					  G_TYPE_STRING,
					  G_TYPE_STRING,
					  G_TYPE_INVALID);

	if (option_device_name == NULL)
		option_device_name = get_device_name(option_device);
	if (option_device_name == NULL)
		option_device_name = g_strdup(option_device);

	create_window();

	client_proxy = dbus_g_proxy_new_for_name(conn, "org.openobex.client",
					"/", "org.openobex.Client");

	setup_agent ();

	send_files ();

	gtk_main();

	g_object_unref(client_proxy);

	dbus_g_connection_unref(conn);

	g_strfreev(option_files);
	g_free(option_device);
	g_free(option_device_name);

	return 0;
}
