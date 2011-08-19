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
#include <gdk/gdkkeysyms.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <unique/uniqueapp.h>

#include "bluetooth-plugin-manager.h"
#include "bluetooth-client.h"
#include "bluetooth-client-private.h"
#include "general.h"
#include "adapter.h"

#define SCHEMA_NAME		"org.mate.Bluetooth"
#define PREF_SHOW_ICON		"show-icon"

static gboolean delete_callback(GtkWidget *window, GdkEvent *event,
							gpointer user_data)
{
	gtk_widget_destroy(GTK_WIDGET(window));

	gtk_main_quit();

	return FALSE;
}

static gboolean
keypress_callback (GtkWidget *window,
		   GdkEventKey *key,
		   gpointer user_data)
{
	if (key->keyval == GDK_Escape) {
		gtk_widget_destroy(GTK_WIDGET(window));

		gtk_main_quit();
	}

	return FALSE;
}

static void close_callback(GtkWidget *button, gpointer user_data)
{
	GtkWidget *window = user_data;

	gtk_widget_destroy(GTK_WIDGET(window));

	gtk_main_quit();
}

static void
receive_callback (GtkWidget *item, GtkWindow *window)
{
	GtkWidget *dialog;
	const char *command = "mate-file-share-properties";

	if (!g_spawn_command_line_async(command, NULL)) {
		/* translators:
		 * This is the name of the preferences dialogue for mate-user-share */
		dialog = gtk_message_dialog_new (window,
						 GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_CLOSE,
						 _("Cannot start \"Personal File Sharing\" Preferences"));

		/* translators:
		 * This is the name of the preferences dialogue for mate-user-share */
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
			_("Please verify that the \"Personal File Sharing\" program is correctly installed."));

		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
	}
}

static void help_callback(GtkWidget *item)
{
	GError *error = NULL;

	if(!gtk_show_uri (gtk_widget_get_screen (GTK_WIDGET(item)),
		"ghelp:mate-bluetooth",  gtk_get_current_event_time (), &error)) {

		g_printerr("Unable to launch help: %s", error->message);
		g_error_free(error);
	}
}

static GtkWidget *create_window(GtkWidget *notebook)
{
	GtkWidget *window;
	GtkWidget *vbox;
	GtkWidget *buttonbox;
	GtkWidget *button;
	GtkWidget *image;
	GSettings *settings;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), _("Bluetooth Preferences"));
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 600, 420);
	g_signal_connect(G_OBJECT(window), "delete-event",
					G_CALLBACK(delete_callback), NULL);

	g_signal_connect(G_OBJECT(window), "key-press-event",
			 G_CALLBACK(keypress_callback), NULL);

	vbox = gtk_vbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

	button = gtk_check_button_new_with_mnemonic (_("_Show Bluetooth icon"));
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
	settings = g_settings_new (SCHEMA_NAME);
	g_settings_bind (settings, PREF_SHOW_ICON, G_OBJECT (button), "active",
			 G_SETTINGS_BIND_DEFAULT);

	buttonbox = gtk_hbutton_box_new();
	gtk_button_box_set_layout(GTK_BUTTON_BOX(buttonbox), GTK_BUTTONBOX_END);
	gtk_box_pack_start(GTK_BOX(vbox), buttonbox, FALSE, FALSE, 0);

	button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	gtk_container_add(GTK_CONTAINER(buttonbox), button);
	g_signal_connect(G_OBJECT(button), "clicked",
					G_CALLBACK(close_callback), window);

	button = gtk_button_new_from_stock(GTK_STOCK_HELP);
	gtk_container_add(GTK_CONTAINER(buttonbox), button);
	gtk_button_box_set_child_secondary(GTK_BUTTON_BOX(buttonbox),
								button, TRUE);
	g_signal_connect(G_OBJECT(button), "clicked",
					G_CALLBACK(help_callback), window);

	image = gtk_image_new_from_stock (GTK_STOCK_JUMP_TO,
					  GTK_ICON_SIZE_BUTTON);
	button = gtk_button_new_with_label (_("Receive Files"));
	gtk_button_set_image (GTK_BUTTON (button), image);
	gtk_container_add(GTK_CONTAINER(buttonbox), button);
	gtk_button_box_set_child_secondary(GTK_BUTTON_BOX(buttonbox),
								button, TRUE);
	g_signal_connect(G_OBJECT(button), "clicked",
					G_CALLBACK(receive_callback), window);

	gtk_widget_show_all(window);

	return window;
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

static void
dump_devices (void)
{
	BluetoothClient *client;
	GtkTreeModel *model;
	GtkTreeIter iter;

	client = bluetooth_client_new ();
	model = bluetooth_client_get_model (client);

	if (gtk_tree_model_get_iter_first (model, &iter) == FALSE) {
		g_print ("No known devices\n");
		g_print ("Is bluetoothd running, and a Bluetooth adapter enabled?\n");
		return;
	}
	bluetooth_client_dump_device (model, &iter, TRUE);
	while (gtk_tree_model_iter_next (model, &iter))
		bluetooth_client_dump_device (model, &iter, TRUE);
}

static gboolean option_dump = FALSE;

static GOptionEntry options[] = {
	{ "dump-devices", 'd', 0, G_OPTION_ARG_NONE, &option_dump, N_("Output a list of currently known devices"), NULL },
	{ NULL },
};

int main(int argc, char *argv[])
{
	UniqueApp *app;
	GtkWidget *window;
	GtkWidget *notebook;
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

	if (option_dump != FALSE) {
		dump_devices ();
		return 0;
	}

	app = unique_app_new ("org.mate.Bluetooth.properties", NULL);
	if (unique_app_is_running (app)) {
		gdk_notify_startup_complete ();
		unique_app_send_message (app, UNIQUE_ACTIVATE, NULL);
		return 0;
	}

	g_set_application_name(_("Bluetooth Properties"));

	gtk_window_set_default_icon_name("bluetooth");

	bluetooth_plugin_manager_init ();

	notebook = gtk_notebook_new();
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);

	setup_adapter(GTK_NOTEBOOK(notebook));

	window = create_window(notebook);

	g_signal_connect (app, "message-received",
			  G_CALLBACK (message_received_cb), window);

	gtk_main();

	bluetooth_plugin_manager_cleanup ();

	cleanup_adapter();

	g_object_unref(app);

	return 0;
}
