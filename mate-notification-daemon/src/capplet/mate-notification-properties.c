/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2007 Jonh Wendell <wendell@bani.com.br>
 * Copyright (C) 2011 Perberos <perberos@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include "config.h"

#include <glib/gi18n.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <mateconf/mateconf-client.h>
#include <string.h>
#include <libmatenotify/notify.h>

#include "stack.h"

#define MATECONF_KEY_DAEMON "/apps/mate-notification-daemon"
#define MATECONF_KEY_THEME MATECONF_KEY_DAEMON "/theme"
#define MATECONF_KEY_POPUP_LOCATION MATECONF_KEY_DAEMON "/popup_location"

#define N_LISTENERS 2

#define NOTIFICATION_UI_FILE "mate-notification-properties.ui"

typedef struct {
	MateConfClient* client;

	GtkWidget* dialog;
	GtkWidget* position_combo;
	GtkWidget* theme_combo;
	GtkWidget* preview_button;

	NotifyNotification* preview;

	guint listeners[N_LISTENERS];
	int n_listeners;
	int expected_listeners;
} NotificationAppletDialog;

enum {
	NOTIFY_POSITION_LABEL,
	NOTIFY_POSITION_NAME,
	N_COLUMNS_POSITION
};

enum {
	NOTIFY_THEME_LABEL,
	NOTIFY_THEME_NAME,
	NOTIFY_THEME_FILENAME,
	N_COLUMNS_THEME
};

static void notification_properties_location_notify(MateConfClient* client, guint cnx_id, MateConfEntry* entry, NotificationAppletDialog* dialog)
{
	GtkTreeModel* model;
	GtkTreeIter iter;
	const char* location;
	gboolean valid;

	if (!entry->value || entry->value->type != MATECONF_VALUE_STRING)
	{
		return;
	}

	location = mateconf_value_get_string(entry->value);

	model = gtk_combo_box_get_model(GTK_COMBO_BOX(dialog->position_combo));
	valid = gtk_tree_model_get_iter_first(model, &iter);

	for (valid = gtk_tree_model_get_iter_first(model, &iter); valid; valid = gtk_tree_model_iter_next(model, &iter))
	{
		gchar* key;

		gtk_tree_model_get(model, &iter, NOTIFY_POSITION_NAME, &key, -1);

		if (g_str_equal(key, location))
		{
			gtk_combo_box_set_active_iter(GTK_COMBO_BOX(dialog->position_combo), &iter);
			g_free(key);
			break;
		}

		g_free(key);
	}
}

static void notification_properties_location_changed(GtkComboBox* widget, NotificationAppletDialog* dialog)
{
	char* location;
	GtkTreeIter iter;

	GtkTreeModel* model = gtk_combo_box_get_model(GTK_COMBO_BOX(dialog->position_combo));

	if (!gtk_combo_box_get_active_iter(GTK_COMBO_BOX(dialog->position_combo), &iter))
	{
		return;
	}

	gtk_tree_model_get(model, &iter, NOTIFY_POSITION_NAME, &location, -1);

	mateconf_client_set_string(dialog->client, MATECONF_KEY_POPUP_LOCATION, location, NULL);
	g_free(location);
}

static void notification_properties_dialog_setup_positions(NotificationAppletDialog* dialog)
{
	char* location;
	gboolean valid;
	GtkTreeModel* model;
	GtkTreeIter iter;

	model = gtk_combo_box_get_model(GTK_COMBO_BOX(dialog->position_combo));
	g_signal_connect(dialog->position_combo, "changed", G_CALLBACK(notification_properties_location_changed), dialog);

	location = mateconf_client_get_string(dialog->client, MATECONF_KEY_POPUP_LOCATION, NULL);

	for (valid = gtk_tree_model_get_iter_first(model, &iter); valid; valid = gtk_tree_model_iter_next(model, &iter))
	{
		gchar* key;

		gtk_tree_model_get(model, &iter, NOTIFY_POSITION_NAME, &key, -1);

		if (g_str_equal(key, location))
		{
			gtk_combo_box_set_active_iter(GTK_COMBO_BOX(dialog->position_combo), &iter);
			g_free(key);
			break;
		}

		g_free(key);
	}

	dialog->listeners[dialog->n_listeners] = mateconf_client_notify_add(dialog->client, MATECONF_KEY_POPUP_LOCATION, (MateConfClientNotifyFunc) notification_properties_location_notify, dialog, NULL, NULL);
	dialog->n_listeners++;
	g_free(location);
}

static void notification_properties_theme_notify(MateConfClient* client, guint cnx_id, MateConfEntry* entry, NotificationAppletDialog* dialog)
{
	if (!entry->value || entry->value->type != MATECONF_VALUE_STRING)
	{
		return;
	}

	const char* theme = mateconf_value_get_string(entry->value);

	GtkTreeModel* model = gtk_combo_box_get_model(GTK_COMBO_BOX(dialog->theme_combo));

	GtkTreeIter iter;
	gboolean valid;

	for (valid = gtk_tree_model_get_iter_first(model, &iter); valid; valid = gtk_tree_model_iter_next(model, &iter))
	{
		gchar* theme_name;

		gtk_tree_model_get(model, &iter, NOTIFY_THEME_NAME, &theme_name, -1);

		if (g_str_equal(theme_name, theme))
		{
			gtk_combo_box_set_active_iter(GTK_COMBO_BOX(dialog->theme_combo), &iter);
			g_free(theme_name);
			break;
		}

		g_free(theme_name);
	}
}

static void notification_properties_theme_changed(GtkComboBox* widget, NotificationAppletDialog* dialog)
{
	char* theme;
	GtkTreeIter iter;

	GtkTreeModel* model = gtk_combo_box_get_model(GTK_COMBO_BOX(dialog->theme_combo));

	if (!gtk_combo_box_get_active_iter(GTK_COMBO_BOX(dialog->theme_combo), &iter))
	{
		return;
	}

	gtk_tree_model_get(model, &iter, NOTIFY_THEME_NAME, &theme, -1);
	mateconf_client_set_string(dialog->client, MATECONF_KEY_THEME, theme, NULL);
	g_free(theme);
}

static gchar* get_theme_name(const gchar* filename)
{
	/* TODO: Remove magic numbers. Strip "lib" and ".so" */
	gchar* result = g_strdup(filename + 3);
	result[strlen(result) - 3] = '\0';
	return result;
}

static void notification_properties_dialog_setup_themes(NotificationAppletDialog* dialog)
{
	GDir* dir;
	const gchar* filename;
	char* theme;
	char* theme_name;
	char* theme_label;
	gboolean valid;
	GtkListStore* store;
	GtkTreeIter iter;

	store = gtk_list_store_new(N_COLUMNS_THEME, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

	gtk_combo_box_set_model(GTK_COMBO_BOX(dialog->theme_combo), GTK_TREE_MODEL(store));
	g_signal_connect(dialog->theme_combo, "changed", G_CALLBACK(notification_properties_theme_changed), dialog);

	if ((dir = g_dir_open(ENGINES_DIR, 0, NULL)))
	{
		while ((filename = g_dir_read_name(dir)))
		{
			if (g_str_has_prefix(filename, "lib") && g_str_has_suffix(filename, ".so"))
			{
				theme_name = get_theme_name(filename);

				/* FIXME: other solution than hardcode? */
				if (g_str_equal(theme_name, "slider"))
				{
					theme_label = g_strdup(_("Slider"));
				}
				else if (g_str_equal(theme_name, "standard"))
				{
					theme_label = g_strdup(_("Standard theme"));
				}
				else
				{
					theme_label = g_strdup(theme_name);
				}

				gtk_list_store_append(store, &iter);
				gtk_list_store_set(store, &iter, NOTIFY_THEME_LABEL, theme_label, NOTIFY_THEME_NAME, theme_name, NOTIFY_THEME_FILENAME, filename, -1);
				g_free(theme_name);
				g_free(theme_label);
			}
		}

		g_dir_close(dir);
	}
	else
	{
		g_warning("Error opening themes dir");
	}


	theme = mateconf_client_get_string(dialog->client, MATECONF_KEY_THEME, NULL);

	for (valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter); valid; valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter))
	{
		gchar* key;

		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, NOTIFY_THEME_NAME, &key, -1);

		if (g_str_equal(key, theme))
		{
			gtk_combo_box_set_active_iter(GTK_COMBO_BOX(dialog->theme_combo), &iter);
			g_free(key);
			break;
		}

		g_free(key);
	}

	dialog->listeners[dialog->n_listeners] = mateconf_client_notify_add(dialog->client, MATECONF_KEY_THEME, (MateConfClientNotifyFunc) notification_properties_theme_notify, dialog, NULL, NULL);
	dialog->n_listeners++;
	g_free(theme);
}

static void notification_properties_dialog_help(void)
{
	/* Do nothing */
}

static void show_message(NotificationAppletDialog* dialog, const gchar* message)
{
	GtkWidget* d = gtk_message_dialog_new(GTK_WINDOW(dialog->dialog), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s", message);
	gtk_dialog_run(GTK_DIALOG(d));
	gtk_widget_destroy(d);
}

static void notification_properties_dialog_preview_closed(NotifyNotification* preview, NotificationAppletDialog* dialog)
{
	if (preview == dialog->preview)
	{
		dialog->preview = NULL;
	}

	g_object_unref(preview);
}

static void notification_properties_dialog_preview(NotificationAppletDialog* dialog)
{
	if (!notify_is_initted() && !notify_init("n-d"))
	{
		show_message(dialog, _("Error initializing libmatenotify"));
		return;
	}

	GError* error = NULL;

	if (dialog->preview)
	{
		notify_notification_close(dialog->preview, NULL);
		g_object_unref(dialog->preview);
		dialog->preview = NULL;
	}

	dialog->preview = notify_notification_new(_("Notification Test"), _("Just a test"), "mate-util", NULL);

	if (!notify_notification_show(dialog->preview, &error))
	{
		char* message = g_strdup_printf(_("Error while displaying notification: %s"), error->message);
		show_message(dialog, message);
		g_error_free(error);
		g_free(message);
	}

	g_signal_connect(dialog->preview, "closed", G_CALLBACK(notification_properties_dialog_preview_closed), dialog);
}

static void notification_properties_dialog_response(GtkWidget* widget, int response, NotificationAppletDialog* dialog)
{
	switch (response)
	{
		case GTK_RESPONSE_HELP:
			notification_properties_dialog_help();
			break;

		case GTK_RESPONSE_ACCEPT:
			notification_properties_dialog_preview(dialog);
			break;

		case GTK_RESPONSE_CLOSE:
		default:
			gtk_widget_destroy(widget);
			break;
	}
}

static void notification_properties_dialog_destroyed(GtkWidget* widget, NotificationAppletDialog* dialog)
{
	dialog->dialog = NULL;

	gtk_main_quit();
}

static gboolean notification_properties_dialog_init(NotificationAppletDialog* dialog)
{
	const char* ui_file;

	if (g_file_test(NOTIFICATION_UI_FILE, G_FILE_TEST_EXISTS))
	{
		ui_file = NOTIFICATION_UI_FILE;
	}
	else
	{
		ui_file = NOTIFICATION_UIDIR "/" NOTIFICATION_UI_FILE;
	}

	GtkBuilder* builder = gtk_builder_new();
	GError* error = NULL;

	gtk_builder_add_from_file(builder, ui_file, &error);

	if (error != NULL)
	{
		g_warning(_("Could not load user interface file: %s"), error->message);
		g_error_free(error);
		return FALSE;
	}

	dialog->dialog = GTK_WIDGET(gtk_builder_get_object(builder, "dialog"));
	g_assert(dialog->dialog != NULL);

	dialog->position_combo = GTK_WIDGET(gtk_builder_get_object(builder, "position_combo"));
	g_assert(dialog->position_combo != NULL);

	dialog->theme_combo = GTK_WIDGET(gtk_builder_get_object(builder, "theme_combo"));
	g_assert(dialog->theme_combo != NULL);

	g_object_unref(builder);

	g_signal_connect(dialog->dialog, "response", G_CALLBACK(notification_properties_dialog_response), dialog);
	g_signal_connect(dialog->dialog, "destroy", G_CALLBACK(notification_properties_dialog_destroyed), dialog);

	dialog->client = mateconf_client_get_default();
	mateconf_client_add_dir(dialog->client, MATECONF_KEY_DAEMON, MATECONF_CLIENT_PRELOAD_ONELEVEL, NULL);

	dialog->expected_listeners = N_LISTENERS;
	dialog->n_listeners = 0;

	notification_properties_dialog_setup_themes(dialog);
	notification_properties_dialog_setup_positions(dialog);

	g_assert(dialog->n_listeners == dialog->expected_listeners);
	gtk_widget_show(dialog->dialog);

	dialog->preview = NULL;

	return TRUE;
}

static void notification_properties_dialog_finalize(NotificationAppletDialog* dialog)
{
	if (dialog->dialog != NULL)
	{
		gtk_widget_destroy(dialog->dialog);
		dialog->dialog = NULL;
	}

	if (dialog->client != NULL)
	{
		int i;

		for (i = 0; i < dialog->n_listeners; i++)
		{
			if (dialog->listeners[i])
			{
				mateconf_client_notify_remove(dialog->client, dialog->listeners[i]);
				dialog->listeners[i] = 0;
			}
		}

		dialog->n_listeners = 0;
		mateconf_client_remove_dir(dialog->client, MATECONF_KEY_DAEMON, NULL);
		g_object_unref(dialog->client);
		dialog->client = NULL;
	}

	if (dialog->preview)
	{
		notify_notification_close(dialog->preview, NULL);
		dialog->preview = NULL;
	}
}

int main(int argc, char** argv)
{
	NotificationAppletDialog dialog = {NULL, }; /* <- ? */

	bindtextdomain(GETTEXT_PACKAGE, NOTIFICATION_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

	gtk_init(&argc, &argv);

	notify_init("mate-notification-properties");

	if (!notification_properties_dialog_init(&dialog))
	{
		notification_properties_dialog_finalize(&dialog);
		return 1;
	}

	gtk_main();

	notification_properties_dialog_finalize(&dialog);

	return 0;
}
