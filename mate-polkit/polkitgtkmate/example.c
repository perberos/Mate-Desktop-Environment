/*
 * Copyright (C) 2009 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: David Zeuthen <davidz@redhat.com>
 */

#include <polkitgtkmate/polkitgtkmate.h>

static PolkitAuthority* authority = NULL;
const gchar* action_id = NULL;
static PolkitSubject* system_bus_name_subject = NULL;
static PolkitSubject* unix_process_subject = NULL;
static GtkWidget* system_bus_name_authorized_label = NULL;
static GtkWidget* unix_process_authorized_label = NULL;

static void update_one(PolkitSubject* subject, GtkWidget* label)
{
	PolkitAuthorizationResult* result;
	GError* error;
	GString* s;
	gchar* subject_str;

	s = g_string_new(NULL);
	subject_str = polkit_subject_to_string(subject);
	g_string_append_printf(s, "Result for subject `%s': ", subject_str);
	g_free(subject_str);

	error = NULL;
	result = polkit_authority_check_authorization_sync(authority, subject, action_id, NULL, POLKIT_CHECK_AUTHORIZATION_FLAGS_NONE, NULL, &error);

	if (result == NULL)
	{
		g_string_append_printf(s, "failed: %s", error->message);
		g_error_free(error);
	}
	else
	{
		g_string_append_printf(s, "authorized=%d challenge=%d retains=%d temporary=%d", polkit_authorization_result_get_is_authorized(result), polkit_authorization_result_get_is_challenge(result), polkit_authorization_result_get_retains_authorization(result), polkit_authorization_result_get_temporary_authorization_id(result) != NULL);
		g_object_unref(result);
	}

	gtk_label_set_text(GTK_LABEL(label), s->str);
	g_string_free(s, TRUE);
}

static void update_labels(void)
{
	update_one(system_bus_name_subject, system_bus_name_authorized_label);
	update_one(unix_process_subject, unix_process_authorized_label);
}

static void on_authority_changed(PolkitAuthority* authority, gpointer user_data)
{
	update_labels();
}

static void on_close(PolkitLockButton* button, gpointer user_data)
{
	gtk_main_quit();
}

static void on_button_changed(PolkitLockButton* button, gpointer user_data)
{
	GtkWidget* entry = GTK_WIDGET(user_data);

	gtk_widget_set_sensitive(entry, polkit_lock_button_get_is_authorized(button));
}

int main(int argc, char* argv[])
{
	GDBusConnection* bus;
	GtkWidget* window;
	GtkWidget* label;
	GtkWidget* button;
	GtkWidget* entry;
	GtkWidget* vbox;
	GError* error;
	gchar* s;

	gtk_init(&argc, &argv);

	if (argc != 2)
	{
		g_printerr("usage: %s <action_id>\n", argv[0]);
		goto out;
	}

	action_id = argv[1];

	error = NULL;
	bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL /* GCancellable* */, &error);

	if (bus == NULL)
	{
		g_printerr("Failed connecting to system bus: %s\n", error->message);
		g_error_free(error);
		goto out;
	}

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_resizable(GTK_WINDOW(window), TRUE);

	vbox = gtk_vbox_new(FALSE, 12);
	gtk_container_set_border_width(GTK_CONTAINER(window), 12);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	s = g_strdup_printf("Showing PolkitLockButton for action id: %s", action_id);
	label = gtk_label_new(s);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	g_free(s);

	label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	system_bus_name_authorized_label = label;

	label = gtk_label_new(NULL);
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	unix_process_authorized_label = label;

	entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);

	button = polkit_lock_button_new(action_id);
	g_signal_connect(button, "changed", G_CALLBACK(on_button_changed), entry);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);


	system_bus_name_subject = polkit_system_bus_name_new(g_dbus_connection_get_unique_name(bus));
	unix_process_subject = polkit_unix_process_new(getpid());

	error = NULL;
	authority = polkit_authority_get_sync(NULL /* GCancellable* */, &error);

	if (authority == NULL)
	{
		g_printerr("Failed getting the authority: %s\n", error->message);
		g_error_free(error);
		goto out;
	}

	g_debug("backend: name=`%s' version=`%s' features=%d", polkit_authority_get_backend_name(authority), polkit_authority_get_backend_version(authority), polkit_authority_get_backend_features(authority));
	g_signal_connect(authority, "changed", G_CALLBACK(on_authority_changed), NULL);

	update_labels();

	gtk_widget_set_sensitive(entry, polkit_lock_button_get_is_authorized(POLKIT_LOCK_BUTTON(button)));

	gtk_widget_show_all(window);
	gtk_window_present(GTK_WINDOW(window));

	g_signal_connect(button, "destroy", G_CALLBACK(on_close), NULL);

	gtk_main();

	out:
	return 0;
}
