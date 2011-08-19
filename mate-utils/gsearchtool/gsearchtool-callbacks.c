/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * MATE Search Tool
 *
 *  File:  gsearchtool-callbacks.c
 *
 *  (C) 2002 the Free Software Foundation
 *
 *  Authors:    Dennis Cranston  <dennis_cranston@yahoo.com>
 *              George Lebl
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
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

#include <gdk/gdkkeysyms.h>

#include "gsearchtool.h"
#include "gsearchtool-callbacks.h"
#include "gsearchtool-support.h"

#define SILENT_WINDOW_OPEN_LIMIT 5

#ifdef HAVE_GETPGID
extern pid_t getpgid (pid_t);
#endif

gboolean row_selected_by_button_press_event;

static void
store_window_state_and_geometry (GSearchWindow *gsearch)
{
	gsearch->window_width = MAX (gsearch->window_width, MINIMUM_WINDOW_WIDTH);
	gsearch->window_height = MAX (gsearch->window_height, MINIMUM_WINDOW_HEIGHT);

	gsearchtool_mateconf_set_int ("/apps/mate-search-tool/default_window_width",
	                           gsearch->window_width);
	gsearchtool_mateconf_set_int ("/apps/mate-search-tool/default_window_height",
		                   gsearch->window_height);
	gsearchtool_mateconf_set_boolean ("/apps/mate-search-tool/default_window_maximized",
	                               gsearch->is_window_maximized);
}

static void
quit_application (GSearchWindow * gsearch)
{
	GSearchCommandDetails * command_details = gsearch->command_details;

	if (command_details->command_status == RUNNING) {
#ifdef HAVE_GETPGID
		pid_t pgid;
#endif
		command_details->command_status = MAKE_IT_QUIT;
#ifdef HAVE_GETPGID
		pgid = getpgid (command_details->command_pid);

		if ((pgid > 1) && (pgid != getpid ())) {
			kill (-(getpgid (command_details->command_pid)), SIGKILL);
		}
		else {
			kill (command_details->command_pid, SIGKILL);
		}
#else
		kill (command_details->command_pid, SIGKILL);
#endif
		wait (NULL);
	}
	store_window_state_and_geometry (gsearch);
	gtk_main_quit ();
}

void
version_cb (const gchar * option_name,
            const gchar * value,
            gpointer data,
            GError ** error)
{
	g_print ("%s %s\n", g_get_application_name (), VERSION);
	exit (0);
}

void
quit_session_cb (EggSMClient * client,
                 gpointer data)
{
	quit_application ((GSearchWindow *) data);
}

void
quit_cb (GtkWidget * widget,
         GdkEvent * event,
	 gpointer data)
{
	quit_application ((GSearchWindow *) data);
}

void
click_close_cb (GtkWidget * widget,
                gpointer data)
{
	quit_application ((GSearchWindow *) data);
}

void
click_find_cb (GtkWidget * widget,
               gpointer data)
{
	GSearchWindow * gsearch = data;
	gchar * command;

	if (gsearch->command_details->is_command_timeout_enabled == TRUE) {
		return;
	}

	if ((gsearch->command_details->command_status == STOPPED) ||
	    (gsearch->command_details->command_status == ABORTED)) {
	    	command = build_search_command (gsearch, TRUE);
		if (command != NULL) {
			spawn_search_command (gsearch, command);
			g_free (command);
		}
	}
}

void
click_stop_cb (GtkWidget * widget,
               gpointer data)
{
	GSearchWindow * gsearch = data;

	if (gsearch->command_details->command_status == RUNNING) {
#ifdef HAVE_GETPGID
		pid_t pgid;
#endif
		gtk_widget_set_sensitive (gsearch->stop_button, FALSE);
		gsearch->command_details->command_status = MAKE_IT_STOP;
#ifdef HAVE_GETPGID
		pgid = getpgid (gsearch->command_details->command_pid);

		if ((pgid > 1) && (pgid != getpid ())) {
			kill (-(getpgid (gsearch->command_details->command_pid)), SIGKILL);
		}
		else {
			kill (gsearch->command_details->command_pid, SIGKILL);
		}
#else
		kill (gsearch->command_details->command_pid, SIGKILL);
#endif
		wait (NULL);
	}
}

void
click_help_cb (GtkWidget * widget,
               gpointer data)
{
	GtkWidget * window = data;
	GError * error = NULL;

	gtk_show_uri (gtk_widget_get_screen (widget), "ghelp:mate-search-tool",  
	              gtk_get_current_event_time (), &error);
	if (error) {
		GtkWidget * dialog;

		dialog = gtk_message_dialog_new (GTK_WINDOW (window),
		                                 GTK_DIALOG_DESTROY_WITH_PARENT,
		                                 GTK_MESSAGE_ERROR,
		                                 GTK_BUTTONS_OK,
		                                 _("Could not open help document."));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
		                                          error->message, NULL);

		gtk_window_set_title (GTK_WINDOW (dialog), "");
		gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
		gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 14);

		g_signal_connect (G_OBJECT (dialog),
                                  "response",
                                  G_CALLBACK (gtk_widget_destroy), NULL);

                gtk_widget_show (dialog);
                g_error_free (error);
        }
}

void
click_expander_cb (GObject * object,
                   GParamSpec * param_spec,
                   gpointer data)
{
	GSearchWindow * gsearch = data;

	if (gtk_expander_get_expanded (GTK_EXPANDER (object)) == TRUE) {
		gtk_widget_show (gsearch->available_options_vbox);
		gtk_window_set_geometry_hints (GTK_WINDOW (gsearch->window),
		                               GTK_WIDGET (gsearch->window),
		                               &gsearch->window_geometry,
		                               GDK_HINT_MIN_SIZE);
	}
	else {
		GdkGeometry default_geometry = {MINIMUM_WINDOW_WIDTH, MINIMUM_WINDOW_HEIGHT};

		gtk_widget_hide (gsearch->available_options_vbox);
		gtk_window_set_geometry_hints (GTK_WINDOW (gsearch->window),
		                               GTK_WIDGET (gsearch->window),
		                               &default_geometry,
		                               GDK_HINT_MIN_SIZE);
	}
}

void
size_allocate_cb (GtkWidget * widget,
                  GtkAllocation * allocation,
                  gpointer data)
{
	GtkWidget * button = data;

 	gtk_widget_set_size_request (button, allocation->width, -1);
}

void
add_constraint_cb (GtkWidget * widget,
                   gpointer data)
{
	GSearchWindow * gsearch = data;
	gint idx;

	idx = gtk_combo_box_get_active (GTK_COMBO_BOX (gsearch->available_options_combo_box));
	add_constraint (gsearch, idx, NULL, FALSE);
}

void
remove_constraint_cb (GtkWidget * widget,
                      gpointer data)
{
	GList * list = data;

	GSearchWindow * gsearch = g_list_first (list)->data;
	GSearchConstraint * constraint = g_list_last (list)->data;

      	gsearch->window_geometry.min_height -= WINDOW_HEIGHT_STEP;

	gtk_window_set_geometry_hints (GTK_WINDOW (gsearch->window),
	                               GTK_WIDGET (gsearch->window),
	                               &gsearch->window_geometry,
	                               GDK_HINT_MIN_SIZE);

	gtk_container_remove (GTK_CONTAINER (gsearch->available_options_vbox), widget->parent);

	gsearch->available_options_selected_list =
	    g_list_remove (gsearch->available_options_selected_list, constraint);

	set_constraint_selected_state (gsearch, constraint->constraint_id, FALSE);
	set_constraint_mateconf_boolean (constraint->constraint_id, FALSE);
	g_slice_free (GSearchConstraint, constraint);
	g_list_free (list);
}

void
constraint_activate_cb (GtkWidget * widget,
                        gpointer data)
{
	GSearchWindow * gsearch = data;

	if ((gtk_widget_get_visible (gsearch->find_button)) &&
	    (gtk_widget_get_sensitive (gsearch->find_button))) {
		click_find_cb (gsearch->find_button, data);
	}
}

void
constraint_update_info_cb (GtkWidget * widget,
                           gpointer data)
{
	static gchar * string;
	GSearchConstraint * opt = data;

	string = (gchar *) gtk_entry_get_text (GTK_ENTRY (widget));
	update_constraint_info (opt, string);
}

void
name_contains_activate_cb (GtkWidget * widget,
                           gpointer data)
{
	GSearchWindow * gsearch = data;

	if ((gtk_widget_get_visible (gsearch->find_button)) &&
	    (gtk_widget_get_sensitive (gsearch->find_button))) {
		click_find_cb (gsearch->find_button, data);
	}
}

void
look_in_folder_changed_cb (GtkWidget * widget,
                           gpointer data)
{
	GSearchWindow * gsearch = data;
	gchar * value;

	value = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (gsearch->look_in_folder_button));

	if (value != NULL) {
		gsearchtool_mateconf_set_string ("/apps/mate-search-tool/look_in_folder", value);
	}
	g_free (value);
}


static gint
display_dialog_file_open_limit (GtkWidget * window,
                                  gint count)
{
	GtkWidget * dialog;
	GtkWidget * button;
	gchar * primary;
	gchar * secondary;
	gint response;

	primary = g_strdup_printf (ngettext ("Are you sure you want to open %d document?",
	                                     "Are you sure you want to open %d documents?",
	                                     count),
	                           count);

	secondary = g_strdup_printf (ngettext ("This will open %d separate window.",
	                                       "This will open %d separate windows.",
	                                       count),
	                             count);

	dialog = gtk_message_dialog_new (GTK_WINDOW (window),
	                                 GTK_DIALOG_DESTROY_WITH_PARENT,
	                                 GTK_MESSAGE_QUESTION,
	                                 GTK_BUTTONS_CANCEL,
	                                 primary, NULL);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
	                                          secondary, NULL);

	gtk_window_set_title (GTK_WINDOW (dialog), "");
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 14);

	button = gtk_button_new_from_stock ("gtk-open");
	gtk_widget_set_can_default (button, TRUE);
	gtk_widget_show (button);

	gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button, GTK_RESPONSE_OK);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

	response = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	g_free (primary);
	g_free (secondary);

	return response;
}

static void
display_dialog_could_not_open_file (GtkWidget * window,
                                    const gchar * file,
                                    const gchar * message)
{
	GtkWidget * dialog;
	gchar * primary;

	primary = g_strdup_printf (_("Could not open document \"%s\"."), file);

	dialog = gtk_message_dialog_new (GTK_WINDOW (window),
	                                 GTK_DIALOG_DESTROY_WITH_PARENT,
	                                 GTK_MESSAGE_ERROR,
	                                 GTK_BUTTONS_OK,
	                                 primary, NULL);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
	                                          message, NULL);

	gtk_window_set_title (GTK_WINDOW (dialog), "");
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 14);

	g_signal_connect (G_OBJECT (dialog),
               		  "response",
               		  G_CALLBACK (gtk_widget_destroy), NULL);

	gtk_widget_show (dialog);
	g_free (primary);
}

static void
display_dialog_could_not_open_folder (GtkWidget * window,
                                      const gchar * folder)
{
	GtkWidget * dialog;
	gchar * primary;

	primary = g_strdup_printf (_("Could not open folder \"%s\"."), folder);

	dialog = gtk_message_dialog_new (GTK_WINDOW (window),
	                                 GTK_DIALOG_DESTROY_WITH_PARENT,
	                                 GTK_MESSAGE_ERROR,
	                                 GTK_BUTTONS_OK,
	                                 primary, NULL);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
	                                          _("The caja file manager is not running."));

	gtk_window_set_title (GTK_WINDOW (dialog), "");
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 14);

	g_signal_connect (G_OBJECT (dialog),
               		  "response",
               		  G_CALLBACK (gtk_widget_destroy), NULL);

	gtk_widget_show (dialog);
	g_free (primary);
}

void
open_file_event_cb (GtkWidget * widget,
                    GdkEventButton * event,
                    gpointer data)
{
	open_file_cb ((GtkMenuItem *) widget, data);
}

void
open_file_cb (GtkMenuItem * action,
              gpointer data)
{
	GSearchWindow * gsearch = data;
	GtkTreeModel * model;
	GList * list;
	guint idx;

	if (gtk_tree_selection_count_selected_rows (GTK_TREE_SELECTION (gsearch->search_results_selection)) == 0) {
		return;
	}

	list = gtk_tree_selection_get_selected_rows (GTK_TREE_SELECTION (gsearch->search_results_selection),
						     &model);

	if (g_list_length (list) > SILENT_WINDOW_OPEN_LIMIT) {
		gint response;

		response = display_dialog_file_open_limit (gsearch->window, g_list_length (list));

		if (response == GTK_RESPONSE_CANCEL) {
			g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
			g_list_free (list);
			return;
		}
	}

	for (idx = 0; idx < g_list_length (list); idx++) {

		gboolean no_files_found = FALSE;
		gchar * utf8_name;
		gchar * locale_file;
		GtkTreeIter iter;

		gtk_tree_model_get_iter (GTK_TREE_MODEL (gsearch->search_results_list_store), &iter,
		                         g_list_nth (list, idx)->data);

		gtk_tree_model_get (GTK_TREE_MODEL (gsearch->search_results_list_store), &iter,
    		                    COLUMN_NAME, &utf8_name,
				    COLUMN_LOCALE_FILE, &locale_file,
		                    COLUMN_NO_FILES_FOUND, &no_files_found,
		                    -1);

		if (!no_files_found) {
			GAppInfo * app = NULL;

			if (GTK_IS_OBJECT (action)) {
				app = g_object_get_data (G_OBJECT (action), "app");
			}

			if (!g_file_test (locale_file, G_FILE_TEST_EXISTS)) {
				gtk_tree_selection_unselect_iter (GTK_TREE_SELECTION (gsearch->search_results_selection),
				                                  &iter);
				display_dialog_could_not_open_file (gsearch->window, utf8_name,
				                                    _("The document does not exist."));

			}
			else if (open_file_with_application (gsearch->window, locale_file, app) == FALSE) {

				if (launch_file (locale_file) == FALSE) {

					if (g_file_test (locale_file, G_FILE_TEST_IS_DIR)) {

						if (open_file_with_filemanager (gsearch->window, locale_file) == FALSE) {
							display_dialog_could_not_open_folder (gsearch->window, utf8_name);
						}
					}
					else {
						display_dialog_could_not_open_file (gsearch->window, utf8_name,
						                                    _("There is no installed viewer capable "
						                                      "of displaying the document."));
					}
				}
			}
		}
		g_free (utf8_name);
		g_free (locale_file);
	}
	g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (list);
}

static gint
display_dialog_folder_open_limit (GtkWidget * window,
                                  gint count)
{
	GtkWidget * dialog;
	GtkWidget * button;
	gchar * primary;
	gchar * secondary;
	gint response;

	primary = g_strdup_printf (ngettext ("Are you sure you want to open %d folder?",
	                                     "Are you sure you want to open %d folders?",
	                                     count),
	                           count);

	secondary = g_strdup_printf (ngettext ("This will open %d separate window.",
					       "This will open %d separate windows.",
					       count),
	                             count);

	dialog = gtk_message_dialog_new (GTK_WINDOW (window),
	                                 GTK_DIALOG_DESTROY_WITH_PARENT,
	                                 GTK_MESSAGE_QUESTION,
	                                 GTK_BUTTONS_CANCEL,
	                                 primary, NULL);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
	                                          secondary, NULL);

	gtk_window_set_title (GTK_WINDOW (dialog), "");
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 14);

	button = gtk_button_new_from_stock ("gtk-open");
	gtk_widget_set_can_default (button, TRUE);
	gtk_widget_show (button);

	gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button, GTK_RESPONSE_OK);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

	response = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	g_free (primary);
	g_free (secondary);

	return response;
}

void
open_folder_cb (GtkAction * action,
                gpointer data)
{
	GSearchWindow * gsearch = data;
	GtkTreeModel * model;
	GFile * g_file = NULL;
	GFileInfo * g_file_info = NULL;
	GAppInfo * g_app_info = NULL;
	GList * list;
	guint idx;

	if (gtk_tree_selection_count_selected_rows (GTK_TREE_SELECTION (gsearch->search_results_selection)) == 0) {
		return;
	}

	list = gtk_tree_selection_get_selected_rows (GTK_TREE_SELECTION (gsearch->search_results_selection),
						     &model);

	if (g_list_length (list) > SILENT_WINDOW_OPEN_LIMIT) {
		gint response;

		response = display_dialog_folder_open_limit (gsearch->window, g_list_length (list));

		if (response == GTK_RESPONSE_CANCEL) {
			g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
			g_list_free (list);
			return;
		}
	}

	for (idx = 0; idx < g_list_length (list); idx++) {

		gchar * locale_folder;
		gchar * utf8_folder;
		gchar * locale_file;
		GtkTreeIter iter;

		gtk_tree_model_get_iter (GTK_TREE_MODEL (gsearch->search_results_list_store), &iter,
					 g_list_nth (list, idx)->data);

		gtk_tree_model_get (GTK_TREE_MODEL (gsearch->search_results_list_store), &iter,
				    COLUMN_RELATIVE_PATH, &utf8_folder,
				    COLUMN_LOCALE_FILE, &locale_file,
				    -1);

		locale_folder = g_path_get_dirname (locale_file);

		if (idx == 0) {
			g_file = g_file_new_for_path (locale_folder);
			g_file_info = g_file_query_info (g_file, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, G_FILE_QUERY_INFO_NONE, NULL, NULL);
			g_app_info = g_app_info_get_default_for_type (g_file_info_get_content_type (g_file_info), FALSE);
		}

		if (open_file_with_application (gsearch->window, locale_folder, g_app_info) == FALSE) {

			if (open_file_with_filemanager (gsearch->window, locale_folder) == FALSE) {

				display_dialog_could_not_open_folder (gsearch->window, utf8_folder);

				g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
				g_list_free (list);
				g_free (locale_folder);
				g_free (utf8_folder);
				g_object_unref (g_file);
				g_object_unref (g_file_info);
				g_object_unref (g_app_info);
				return;
			}
		}
		g_free (locale_folder);
		g_free (locale_file);
		g_free (utf8_folder);
	}
	g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (list);
	g_object_unref (g_file);
	g_object_unref (g_file_info);
	g_object_unref (g_app_info);
}

void
file_changed_cb (GFileMonitor * handle,
                 const gchar * monitor_uri,
                 const gchar * info_uri,
                 GFileMonitorEvent event_type,
                 gpointer data)
{
	GSearchMonitor * monitor = data;
	GSearchWindow * gsearch = monitor->gsearch;
	GtkTreeModel * model;
	GtkTreePath * path;
	GtkTreeIter iter;

	switch (event_type) {
	case G_FILE_MONITOR_EVENT_DELETED:
		path = gtk_tree_row_reference_get_path (monitor->reference);
		model = gtk_tree_row_reference_get_model (monitor->reference);
		gtk_tree_model_get_iter (model, &iter, path);
		tree_model_iter_free_monitor (model, NULL, &iter, NULL);
		gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
		update_search_counts (gsearch);
		break;
	default:
		break;
	}
}

static void
display_dialog_could_not_move_to_trash (GtkWidget * window,
                                        const gchar * file,
                                        const gchar * message)
{
	GtkWidget * dialog;
	gchar * primary;

	primary = g_strdup_printf (_("Could not move \"%s\" to trash."), file);

	dialog = gtk_message_dialog_new (GTK_WINDOW (window),
	                                 GTK_DIALOG_DESTROY_WITH_PARENT,
	                                 GTK_MESSAGE_ERROR,
	                                 GTK_BUTTONS_OK,
	                                 primary, NULL);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
	                                          message, NULL);

	gtk_window_set_title (GTK_WINDOW (dialog), "");
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 14);

	g_signal_connect (G_OBJECT (dialog),
               		  "response",
               		  G_CALLBACK (gtk_widget_destroy), NULL);
	gtk_widget_show (dialog);
	g_free (primary);
}

static gint
display_dialog_delete_permanently (GtkWidget * window,
                                   const gchar * file)
{
	GtkWidget * dialog;
	GtkWidget * button;
	gchar * primary;
	gchar * secondary;
	gint response;

	primary = g_strdup_printf (_("Do you want to delete \"%s\" permanently?"),
	                           g_path_get_basename (file));

	secondary = g_strdup_printf (_("Trash is unavailable.  Could not move \"%s\" to the trash."),
	                             file);

	dialog = gtk_message_dialog_new (GTK_WINDOW (window),
	                                 GTK_DIALOG_DESTROY_WITH_PARENT,
	                                 GTK_MESSAGE_QUESTION,
	                                 GTK_BUTTONS_CANCEL,
	                                 primary, NULL);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
	                                          secondary, NULL);

	gtk_window_set_title (GTK_WINDOW (dialog), "");
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 14);

	button = gtk_button_new_from_stock ("gtk-delete");
	gtk_widget_set_can_default (button, TRUE);
	gtk_widget_show (button);

	gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button, GTK_RESPONSE_OK);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

	response = gtk_dialog_run (GTK_DIALOG (dialog));

	gtk_widget_destroy (GTK_WIDGET(dialog));
	g_free (primary);
	g_free (secondary);

	return response;
}

static void
display_dialog_could_not_delete (GtkWidget * window,
                                 const gchar * file,
                                 const gchar * message)
{
	GtkWidget * dialog;
	gchar * primary;

	primary = g_strdup_printf (_("Could not delete \"%s\"."), file);

	dialog = gtk_message_dialog_new (GTK_WINDOW (window),
	                                 GTK_DIALOG_DESTROY_WITH_PARENT,
	                                 GTK_MESSAGE_ERROR,
	                                 GTK_BUTTONS_OK,
	                                 primary, NULL);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
	                                          message, NULL);

	gtk_window_set_title (GTK_WINDOW (dialog), "");
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 14);

	g_signal_connect (G_OBJECT (dialog),
               		  "response",
               		  G_CALLBACK (gtk_widget_destroy), NULL);
	gtk_widget_show (dialog);
	g_free (primary);
}

void
move_to_trash_cb (GtkAction * action,
                  gpointer data)
{
	GSearchWindow * gsearch = data;
	GtkTreePath * last_selected_path = NULL;
	gint total;
	gint idx;

	if (gtk_tree_selection_count_selected_rows (GTK_TREE_SELECTION (gsearch->search_results_selection)) == 0) {
		return;
	}

	total = gtk_tree_selection_count_selected_rows (GTK_TREE_SELECTION (gsearch->search_results_selection));

	for (idx = 0; idx < total; idx++) {
		gboolean no_files_found = FALSE;
		GtkTreeModel * model;
		GtkTreeIter iter;
		GList * list;
		GFile * g_file;
		GError * error = NULL;
		gchar * utf8_basename;
		gchar * utf8_filename;
		gchar * locale_filename;
		gboolean result;

		list = gtk_tree_selection_get_selected_rows (GTK_TREE_SELECTION (gsearch->search_results_selection),
 		                                             &model);

		gtk_tree_model_get_iter (GTK_TREE_MODEL (gsearch->search_results_list_store), &iter,
					 g_list_nth (list, 0)->data);

		gtk_tree_model_get (GTK_TREE_MODEL (gsearch->search_results_list_store), &iter,
    				    COLUMN_NAME, &utf8_basename,
				    COLUMN_LOCALE_FILE, &locale_filename,
			   	    COLUMN_NO_FILES_FOUND, &no_files_found,
			   	    -1);
		utf8_filename = g_filename_display_name (locale_filename);

		if (no_files_found) {
			g_free (utf8_basename);
			g_free (locale_filename);
			return;
		}
		
		if (idx + 1 == total) {
			last_selected_path = gtk_tree_model_get_path (GTK_TREE_MODEL (gsearch->search_results_list_store), &iter);
		}

		if ((!g_file_test (locale_filename, G_FILE_TEST_EXISTS)) &&
		    (!g_file_test (locale_filename, G_FILE_TEST_IS_SYMLINK))) {
			gtk_tree_selection_unselect_iter (GTK_TREE_SELECTION (gsearch->search_results_selection), &iter);
			display_dialog_could_not_move_to_trash (gsearch->window, utf8_basename,
			                                        _("The document does not exist."));
		}

		g_file = g_file_new_for_path (locale_filename);
		result = g_file_trash (g_file, NULL, &error);

		gtk_tree_selection_unselect_iter (GTK_TREE_SELECTION (gsearch->search_results_selection), &iter);
		g_object_unref (g_file);

		if (result == TRUE) {
			tree_model_iter_free_monitor (GTK_TREE_MODEL (gsearch->search_results_list_store),
						      NULL, &iter, NULL);
			gtk_list_store_remove (GTK_LIST_STORE (gsearch->search_results_list_store), &iter);
		}
		else {
			gint response;

			gtk_tree_selection_unselect_iter (GTK_TREE_SELECTION (gsearch->search_results_selection), &iter);
			response = display_dialog_delete_permanently (gsearch->window, utf8_filename);

			if (response == GTK_RESPONSE_OK) {
				GFile * g_file_tmp;
				GError * error_tmp = NULL;

				g_file_tmp = g_file_new_for_path (locale_filename);
				result = g_file_delete (g_file_tmp, NULL, &error_tmp);
				g_object_unref (g_file_tmp);

				if (result == TRUE) {
					tree_model_iter_free_monitor (GTK_TREE_MODEL (gsearch->search_results_list_store),
								      NULL, &iter, NULL);
					gtk_list_store_remove (GTK_LIST_STORE (gsearch->search_results_list_store), &iter);
				}
				else {
					gchar * message;

					message = g_strdup_printf (_("Deleting \"%s\" failed: %s."),
					                             utf8_filename, error_tmp->message);

					display_dialog_could_not_delete (gsearch->window, utf8_basename, message);

					g_error_free (error_tmp);
					g_free (message);
				}
			}
			else {
				gchar * message;

				message = g_strdup_printf (_("Moving \"%s\" failed: %s."),
				                           utf8_filename,
				                           error->message);
				display_dialog_could_not_move_to_trash (gsearch->window, utf8_basename,
				                                        message);
				g_error_free (error);
				g_free (message);
			}
		}
		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (list);
		g_free (locale_filename);
		g_free (utf8_filename);
		g_free (utf8_basename);
	}

	/* Bugzilla #397945: Select next row in the search results list */
	if (last_selected_path != NULL) {
		if (gtk_tree_selection_count_selected_rows (GTK_TREE_SELECTION (gsearch->search_results_selection)) == 0) {
			gtk_tree_selection_select_path (GTK_TREE_SELECTION (gsearch->search_results_selection), 
			                                last_selected_path);
			if (gtk_tree_selection_count_selected_rows (GTK_TREE_SELECTION (gsearch->search_results_selection)) == 0) {
				gtk_tree_path_prev (last_selected_path);
				gtk_tree_selection_select_path (GTK_TREE_SELECTION (gsearch->search_results_selection), 
				                                last_selected_path);
			}
		}
		gtk_tree_path_free (last_selected_path);
	}
	
	if (gsearch->command_details->command_status != RUNNING) {
		update_search_counts (gsearch);
	}
}

gboolean
file_button_press_event_cb (GtkWidget * widget,
                            GdkEventButton * event,
                            gpointer data)
{
	GtkTreeView * tree = data;
	GtkTreePath * path;

	row_selected_by_button_press_event = TRUE;

	if (event->window != gtk_tree_view_get_bin_window (tree)) {
		return FALSE;
	}

	if (gtk_tree_view_get_path_at_pos (tree, event->x, event->y,
		&path, NULL, NULL, NULL)) {

		if ((event->button == 1 || event->button == 2 || event->button == 3)
			&& gtk_tree_selection_path_is_selected (gtk_tree_view_get_selection (tree), path)) {
			row_selected_by_button_press_event = FALSE;
		}
		gtk_tree_path_free (path);
	}
	else {
		gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (tree));
	}

	return !(row_selected_by_button_press_event);
}

gboolean
file_key_press_event_cb (GtkWidget * widget,
                         GdkEventKey * event,
                         gpointer data)
{
	if (event->keyval == GDK_space  ||
	    event->keyval == GDK_Return ||
	    event->keyval == GDK_KP_Enter) {
		if (event->state != GDK_CONTROL_MASK) {
			open_file_cb ((GtkMenuItem *) NULL, data);
			return TRUE;
		}
	}
	else if (event->keyval == GDK_Delete) {
		move_to_trash_cb ((GtkAction *) NULL, data);
		return TRUE;
	}
	return FALSE;
}

static gint
open_with_list_sort (gconstpointer a,
                     gconstpointer b)
{
	const gchar * a_app_name = g_app_info_get_name ((GAppInfo *) a);
	const gchar * b_app_name = g_app_info_get_name ((GAppInfo *) b);
	gchar * a_utf8;
	gchar * b_utf8;
	gint result;

	a_utf8 = g_utf8_casefold (a_app_name, -1);
	b_utf8 = g_utf8_casefold (b_app_name, -1);

	result = g_utf8_collate (a_utf8, b_utf8);

	g_free (a_utf8);
	g_free (b_utf8);

	return result;
}

static void
build_popup_menu_for_file (GSearchWindow * gsearch,
                           gchar * file)
{
	GtkWidget * new1, * image1, * separatormenuitem1;
	GtkWidget * new2;
	gint i;

	if (GTK_IS_MENU (gsearch->search_results_popup_menu) == TRUE) {
		g_object_ref_sink (gsearch->search_results_popup_menu);
		g_object_unref (gsearch->search_results_popup_menu);
	}

	if (GTK_IS_MENU (gsearch->search_results_popup_submenu) == TRUE) {
		g_object_ref_sink (gsearch->search_results_popup_submenu);
		g_object_unref (gsearch->search_results_popup_submenu);
	}

	gsearch->search_results_popup_menu = gtk_menu_new ();

	if (file == NULL || g_file_test (file, G_FILE_TEST_IS_DIR) == TRUE) {
		/* Popup menu item: Open */
		new1 = gtk_image_menu_item_new_with_mnemonic  (_("_Open"));
		gtk_container_add (GTK_CONTAINER (gsearch->search_results_popup_menu), new1);
		gtk_widget_show (new1);

		image1 = gtk_image_new_from_stock ("gtk-open", GTK_ICON_SIZE_MENU);
		gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (new1), image1);
		gtk_widget_show (image1);

		g_signal_connect (G_OBJECT (new1),
		                  "activate",
		                  G_CALLBACK (open_file_cb),
		                  (gpointer) gsearch);
	}
	else {
		GFile * g_file;
		GFileInfo * file_info;
		GIcon * file_icon;
		GList * list;
		gchar * str;
		gint list_length;
	
		g_file = g_file_new_for_path (file);
		file_info = g_file_query_info (g_file, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, G_FILE_QUERY_INFO_NONE, NULL, NULL);
		list = g_app_info_get_all_for_type (g_file_info_get_content_type (file_info));

		list_length = g_list_length (list);

		if (list_length <= 0) {

			/* Popup menu item: Open */
			new1 = gtk_image_menu_item_new_with_mnemonic  (_("_Open"));
			gtk_container_add (GTK_CONTAINER (gsearch->search_results_popup_menu), new1);
			gtk_widget_show (new1);

			image1 = gtk_image_new_from_stock ("gtk-open", GTK_ICON_SIZE_MENU);
			gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (new1), image1);
			gtk_widget_show (image1);

			g_signal_connect (G_OBJECT (new1),
			                  "activate",
			                  G_CALLBACK (open_file_cb),
		        	          (gpointer) gsearch);
		}
		else {
			if (list_length >= 3) { /* Sort all except first application by name */
				GList * tmp;

				tmp = g_list_first (list);
				list = g_list_remove_link (list, tmp);
				list = g_list_sort (list, open_with_list_sort);
				list = g_list_prepend (list, tmp->data);
				g_list_free (tmp);
			}
		
			/* Popup menu item: Open with (default) */
			str = g_strdup_printf (_("_Open with %s"),  g_app_info_get_name (list->data));
			new1 = gtk_image_menu_item_new_with_mnemonic (str);
			gtk_widget_show (new1);

			g_object_set_data_full (G_OBJECT (new1), "app", (GAppInfo *)list->data,
			                        (GDestroyNotify) g_object_unref);

			gtk_container_add (GTK_CONTAINER (gsearch->search_results_popup_menu), new1);
			g_signal_connect ((gpointer) new1, "activate", G_CALLBACK (open_file_cb),
					  (gpointer) gsearch);

			if (g_app_info_get_icon ((GAppInfo *)list->data) != NULL) {
				file_icon = g_object_ref (g_app_info_get_icon ((GAppInfo *)list->data));
				gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (new1), file_icon != NULL);

				if (file_icon == NULL) {
					file_icon = g_themed_icon_new (GTK_STOCK_OPEN);
				}

				image1 = gtk_image_new_from_gicon (file_icon, GTK_ICON_SIZE_MENU);
				g_object_unref (file_icon);
				gtk_widget_show (image1);
				gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (new1), image1);
			}
			
			separatormenuitem1 = gtk_separator_menu_item_new ();
			gtk_widget_show (separatormenuitem1);
			gtk_container_add (GTK_CONTAINER (gsearch->search_results_popup_menu), separatormenuitem1);
			gtk_widget_set_sensitive (separatormenuitem1, FALSE);			

			for (list = g_list_next (list), i = 0; list != NULL; list = g_list_next (list), i++) {

				/* Popup menu item: Open with (others) */
				if (list_length < 4) {
					str = g_strdup_printf (_("Open with %s"),  g_app_info_get_name (list->data));
				}
				else {
					str = g_strdup_printf ("%s",  g_app_info_get_name (list->data));
				}
				
				new1 = gtk_image_menu_item_new_with_mnemonic (str);
				gtk_widget_show (new1);

				g_object_set_data_full (G_OBJECT (new1), "app", (GAppInfo *)list->data,
			                                (GDestroyNotify) g_object_unref);

				if (list_length >= 4) {

					if (g_app_info_get_icon ((GAppInfo *)list->data) != NULL) {
						file_icon = g_object_ref (g_app_info_get_icon ((GAppInfo *)list->data));
						gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (new1), file_icon != NULL);

						if (file_icon == NULL) {
							file_icon = g_themed_icon_new (GTK_STOCK_OPEN);
						}
				
						image1 = gtk_image_new_from_gicon (file_icon, GTK_ICON_SIZE_MENU);
						g_object_unref (file_icon);
						gtk_widget_show (image1);
						gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (new1), image1);
					}
					
					if (i == 0) {
						gsearch->search_results_popup_submenu = gtk_menu_new ();

						/* Popup menu item: Open With */
					  	new2 = gtk_menu_item_new_with_mnemonic  (_("Open Wit_h"));
				  		gtk_widget_show (new2);
					 	gtk_container_add (GTK_CONTAINER (gsearch->search_results_popup_menu), new2);

		  			  	gtk_menu_item_set_submenu (GTK_MENU_ITEM (new2), gsearch->search_results_popup_submenu);
		                       	}
					gtk_container_add (GTK_CONTAINER (gsearch->search_results_popup_submenu), new1);

					/* For submenu items, the "activate" signal is only emitted if the user first clicks 
					   on the parent menu item.  Since submenus in gtk+ are automatically displayed when
					   the user hovers over them, most will never click on the parent menu item.  
					   The work-around is to connect to "button-press-event". */
					g_signal_connect (G_OBJECT(new1), "button-press-event", G_CALLBACK (open_file_event_cb),
					                  (gpointer) gsearch);
				}
				else {
					if (g_app_info_get_icon ((GAppInfo *)list->data) != NULL) {

						file_icon = g_object_ref (g_app_info_get_icon ((GAppInfo *)list->data));
						gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (new1), file_icon != NULL);

						if (file_icon == NULL) {
							file_icon = g_themed_icon_new (GTK_STOCK_OPEN);
						}
				
						image1 = gtk_image_new_from_gicon (file_icon, GTK_ICON_SIZE_MENU);
						g_object_unref (file_icon);
						gtk_widget_show (image1);
						gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (new1), image1);
					}
					gtk_container_add (GTK_CONTAINER (gsearch->search_results_popup_menu), new1);
					g_signal_connect ((gpointer) new1, "activate", G_CALLBACK (open_file_cb),
					                  (gpointer) gsearch);
				}
			}
		
			if (list_length >= 2) {
				separatormenuitem1 = gtk_separator_menu_item_new ();
				gtk_container_add (GTK_CONTAINER (gsearch->search_results_popup_menu), separatormenuitem1);
				gtk_widget_show (separatormenuitem1);
			}
		}
	}

	/* Popup menu item: Open Folder */
	new1 = gtk_image_menu_item_new_with_mnemonic  (_("Open _Folder"));
	gtk_container_add (GTK_CONTAINER (gsearch->search_results_popup_menu), new1);
	gtk_widget_show (new1);

	image1 = gtk_image_new_from_stock ("gtk-open", GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (new1), image1);
	gtk_widget_show (image1);

	g_signal_connect (G_OBJECT (new1),
	                  "activate",
	                  G_CALLBACK (open_folder_cb),
	                  (gpointer) gsearch);

	/* Popup menu item: Move to Trash */
	separatormenuitem1 = gtk_separator_menu_item_new ();
	gtk_container_add (GTK_CONTAINER (gsearch->search_results_popup_menu), separatormenuitem1);
	gtk_widget_show (separatormenuitem1);

	new1 = gtk_image_menu_item_new_with_mnemonic  (_("Mo_ve to Trash"));
	gtk_container_add (GTK_CONTAINER (gsearch->search_results_popup_menu), new1);
	gtk_widget_show (new1);

	GtkIconTheme *icon_theme;
	GdkPixbuf *pixbuf;
	icon_theme = gtk_icon_theme_get_default ();
	pixbuf = gtk_icon_theme_load_icon (icon_theme, "user-trash", GTK_ICON_SIZE_MENU, 0, NULL);
	image1 = gtk_image_new_from_pixbuf (pixbuf);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (new1), image1);
	gtk_widget_show (image1);

	g_signal_connect (G_OBJECT (new1),
	                  "activate",
	                  G_CALLBACK (move_to_trash_cb),
	                  (gpointer) gsearch);

	/* Popup menu item: Save Results As... */
	separatormenuitem1 = gtk_separator_menu_item_new ();
	gtk_container_add (GTK_CONTAINER (gsearch->search_results_popup_menu), separatormenuitem1);
	gtk_widget_show (separatormenuitem1);

	gsearch->search_results_save_results_as_item = gtk_image_menu_item_new_with_mnemonic  (_("_Save Results As..."));
	gtk_container_add (GTK_CONTAINER (gsearch->search_results_popup_menu), gsearch->search_results_save_results_as_item);
	gtk_widget_show (gsearch->search_results_save_results_as_item);

	if (gsearch->command_details->command_status == RUNNING) {
		gtk_widget_set_sensitive (gsearch->search_results_save_results_as_item, FALSE);
	}

	image1 = gtk_image_new_from_stock ("gtk-save", GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (gsearch->search_results_save_results_as_item), image1);
	gtk_widget_show (image1);

	g_signal_connect (G_OBJECT (gsearch->search_results_save_results_as_item),
	                  "activate",
	                  G_CALLBACK (show_file_selector_cb),
	                  (gpointer) gsearch);
}	

gboolean
file_button_release_event_cb (GtkWidget * widget,
                              GdkEventButton * event,
                              gpointer data)
{
	GSearchWindow * gsearch = data;

	if (event->window != gtk_tree_view_get_bin_window (GTK_TREE_VIEW (gsearch->search_results_tree_view))) {
		return FALSE;
	}

	if (event->button == 1 || event->button == 2) {
		GtkTreePath *path;

		if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (gsearch->search_results_tree_view), event->x, event->y,
		                                   &path, NULL, NULL, NULL)) {
			if ((event->state & GDK_SHIFT_MASK) || (event->state & GDK_CONTROL_MASK)) {
				if (row_selected_by_button_press_event) {
					gtk_tree_selection_select_path (gtk_tree_view_get_selection (GTK_TREE_VIEW(gsearch->search_results_tree_view)), path);
				}
				else {
					gtk_tree_selection_unselect_path (gtk_tree_view_get_selection (GTK_TREE_VIEW(gsearch->search_results_tree_view)), path);
				}
			}
			else {
				if (gsearch->is_search_results_single_click_to_activate == FALSE) {
					gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (GTK_TREE_VIEW(gsearch->search_results_tree_view)));
				}
				gtk_tree_selection_select_path (gtk_tree_view_get_selection (GTK_TREE_VIEW(gsearch->search_results_tree_view)), path);
			}
		}
		gtk_tree_path_free (path);
	}

	if (gtk_tree_selection_count_selected_rows (GTK_TREE_SELECTION (gsearch->search_results_selection)) == 0) {
		return FALSE;
	}

	if (event->button == 3) {
		gboolean no_files_found = FALSE;
		GtkTreeModel * model;
		GtkTreeIter iter;
		GList * list;
		gchar * utf8_name_first;
		gchar * locale_file_first;

		list = gtk_tree_selection_get_selected_rows (GTK_TREE_SELECTION (gsearch->search_results_selection),
		                                             &model);

		gtk_tree_model_get_iter (GTK_TREE_MODEL (gsearch->search_results_list_store), &iter,
		                         g_list_first (list)->data);

		gtk_tree_model_get (GTK_TREE_MODEL (gsearch->search_results_list_store), &iter,
		                    COLUMN_NAME, &utf8_name_first,
				    COLUMN_LOCALE_FILE, &locale_file_first,
			    	    COLUMN_NO_FILES_FOUND, &no_files_found,
			   	    -1);

		if (!no_files_found) {

			gboolean show_app_list = TRUE;
			GAppInfo * first_app_info = NULL;
			GTimer * timer;
			GList * tmp;
			gchar * locale_file_tmp;
			gchar * file = NULL;
			gint idx;

			timer = g_timer_new ();
			g_timer_start (timer);

			if (g_list_length (list) >= 2) {

				/* Verify the selected files each have the same default handler. */
				for (tmp = g_list_first (list), idx = 0; tmp != NULL; tmp = g_list_next (tmp), idx++) {
			
					GFile * g_file;
					GAppInfo * app_info;

					gtk_tree_model_get_iter (GTK_TREE_MODEL (gsearch->search_results_list_store), &iter,
					                         tmp->data);

					gtk_tree_model_get (GTK_TREE_MODEL (gsearch->search_results_list_store), &iter,
							    COLUMN_LOCALE_FILE, &locale_file_tmp,
					                    -1);

					g_file = g_file_new_for_path (locale_file_tmp);
					app_info = g_file_query_default_handler (g_file, NULL, NULL);

					if (G_IS_APP_INFO (app_info) == FALSE) {
						show_app_list = FALSE;
					}
					else {
						if (idx == 0) { 
							first_app_info = g_app_info_dup (app_info);
							g_object_unref (app_info);
							continue;
						}

						show_app_list = g_app_info_equal (app_info, first_app_info);
						g_object_unref (app_info);

						/* Break out, if more that 1.5 seconds have passed */
						if (g_timer_elapsed (timer, NULL) > 1.50) {
							show_app_list = FALSE;
						}
					}
					g_object_unref (g_file);
					g_free (locale_file_tmp);

					if (show_app_list == FALSE) {
						break;
					}
				}
				g_timer_destroy (timer);
				if (first_app_info != NULL) {
					g_object_unref (first_app_info);
				}
			}
			
			file = g_strdup (((show_app_list == TRUE) ? locale_file_first : NULL));

			build_popup_menu_for_file (gsearch, file);
			gtk_menu_popup (GTK_MENU (gsearch->search_results_popup_menu), NULL, NULL, NULL, NULL,
			                event->button, event->time);
			g_free (file);

		}
		g_free (locale_file_first);
		g_free (utf8_name_first);
		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (list);
	}
	else if (event->button == 1 || event->button == 2) {
		if (gsearch->is_search_results_single_click_to_activate == TRUE) {
			if (!(event->state & GDK_CONTROL_MASK) && !(event->state & GDK_SHIFT_MASK)) {
			     	open_file_cb ((GtkMenuItem *) NULL, data);
			}
		}
	}
	return FALSE;
}

gboolean
file_event_after_cb  (GtkWidget * widget,
                      GdkEventButton * event,
                      gpointer data)
{
	GSearchWindow * gsearch = data;

	if (event->window != gtk_tree_view_get_bin_window (GTK_TREE_VIEW (gsearch->search_results_tree_view))) {
		return FALSE;
	}

	if (gtk_tree_selection_count_selected_rows (GTK_TREE_SELECTION (gsearch->search_results_selection)) == 0) {
		return FALSE;
	}

	if (!(event->state & GDK_CONTROL_MASK) && !(event->state & GDK_SHIFT_MASK)) {
		if (gsearch->is_search_results_single_click_to_activate == FALSE) {
			if (event->type == GDK_2BUTTON_PRESS) {
				open_file_cb ((GtkMenuItem *) NULL, data);
				return TRUE;
			}
		}
	}
	return FALSE;
}

gboolean
file_motion_notify_cb (GtkWidget *widget,
                       GdkEventMotion *event,
                       gpointer user_data)
{
        GSearchWindow * gsearch = user_data;
	GdkCursor * cursor;
	GtkTreePath * last_hover_path;
	GtkTreeIter iter;

	if (gsearch->is_search_results_single_click_to_activate == FALSE) {
		return FALSE;
	}

	if (event->window != gtk_tree_view_get_bin_window (GTK_TREE_VIEW (gsearch->search_results_tree_view))) {
                return FALSE;
	}

	last_hover_path = gsearch->search_results_hover_path;

	gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget),
				       event->x, event->y,
				       &gsearch->search_results_hover_path,
				       NULL, NULL, NULL);

	if (gsearch->search_results_hover_path != NULL) {
		cursor = gdk_cursor_new (GDK_HAND2);
	}
	else {
		cursor = NULL;
	}

	gdk_window_set_cursor (event->window, cursor);

	/* Redraw if the hover row has changed */
	if (!(last_hover_path == NULL && gsearch->search_results_hover_path == NULL) &&
	    (!(last_hover_path != NULL && gsearch->search_results_hover_path != NULL) ||
	     gtk_tree_path_compare (last_hover_path, gsearch->search_results_hover_path))) {
		if (last_hover_path) {
			gtk_tree_model_get_iter (GTK_TREE_MODEL (gsearch->search_results_list_store),
			                         &iter, last_hover_path);
			gtk_tree_model_row_changed (GTK_TREE_MODEL (gsearch->search_results_list_store),
			                            last_hover_path, &iter);
		}

		if (gsearch->search_results_hover_path) {
			gtk_tree_model_get_iter (GTK_TREE_MODEL (gsearch->search_results_list_store),
			                         &iter, gsearch->search_results_hover_path);
			gtk_tree_model_row_changed (GTK_TREE_MODEL (gsearch->search_results_list_store),
			                            gsearch->search_results_hover_path, &iter);
		}
	}

	gtk_tree_path_free (last_hover_path);

 	return FALSE;
}

gboolean
file_leave_notify_cb (GtkWidget *widget,
                      GdkEventCrossing *event,
                      gpointer user_data)
{
        GSearchWindow * gsearch = user_data;
	GtkTreeIter iter;

	if (gsearch->is_search_results_single_click_to_activate && (gsearch->search_results_hover_path != NULL)) {
		gtk_tree_model_get_iter (GTK_TREE_MODEL (gsearch->search_results_list_store),
		                         &iter,
		                         gsearch->search_results_hover_path);
		gtk_tree_model_row_changed (GTK_TREE_MODEL (gsearch->search_results_list_store),
		                            gsearch->search_results_hover_path,
		                            &iter);

		gtk_tree_path_free (gsearch->search_results_hover_path);
		gsearch->search_results_hover_path = NULL;

		return TRUE;
	}

	return FALSE;
}

void
drag_begin_file_cb (GtkWidget * widget,
                    GdkDragContext * context,
                    gpointer data)
{
	GSearchWindow * gsearch = data;
	gint number_of_selected_rows;

	number_of_selected_rows = gtk_tree_selection_count_selected_rows (GTK_TREE_SELECTION (gsearch->search_results_selection));

	if (number_of_selected_rows > 1) {
		gtk_drag_set_icon_stock (context, GTK_STOCK_DND_MULTIPLE, 0, 0);
	}
	else if (number_of_selected_rows == 1) {
		GdkPixbuf * pixbuf;
		GtkTreeModel * model;
		GtkTreeIter iter;
		GList * list;

		list = gtk_tree_selection_get_selected_rows (GTK_TREE_SELECTION (gsearch->search_results_selection),
		                                             &model);

		gtk_tree_model_get_iter (GTK_TREE_MODEL (gsearch->search_results_list_store), &iter,
		                         g_list_first (list)->data);

		gtk_tree_model_get (GTK_TREE_MODEL (gsearch->search_results_list_store), &iter,
		                    COLUMN_ICON, &pixbuf,
		                    -1);
		g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (list);

		if (pixbuf) {
			gtk_drag_set_icon_pixbuf (context, pixbuf, 0, 0);
		}
		else {
			gtk_drag_set_icon_stock (context, GTK_STOCK_DND, 0, 0);
		}
	}
}

void
drag_file_cb  (GtkWidget * widget,
               GdkDragContext * context,
               GtkSelectionData * selection_data,
               guint info,
               guint drag_time,
               gpointer data)
{
	GSearchWindow * gsearch = data;
	gchar * uri_list = NULL;
	GList * list;
	GtkTreeModel * model;
	GtkTreeIter iter;
	guint idx;

	if (gtk_tree_selection_count_selected_rows (GTK_TREE_SELECTION (gsearch->search_results_selection)) == 0) {
		return;
	}

	list = gtk_tree_selection_get_selected_rows (GTK_TREE_SELECTION (gsearch->search_results_selection),
                                                     &model);

	for (idx = 0; idx < g_list_length (list); idx++) {

		gboolean no_files_found = FALSE;
		gchar * utf8_name;
		gchar * locale_file;

		gtk_tree_model_get_iter (GTK_TREE_MODEL (gsearch->search_results_list_store), &iter,
		                         g_list_nth (list, idx)->data);

		gtk_tree_model_get (GTK_TREE_MODEL (gsearch->search_results_list_store), &iter,
		                    COLUMN_NAME, &utf8_name,
		                    COLUMN_LOCALE_FILE, &locale_file,
		                    COLUMN_NO_FILES_FOUND, &no_files_found,
		                    -1);

		if (!no_files_found) {
			gchar * tmp_uri = g_filename_to_uri (locale_file, NULL, NULL);

			if (uri_list == NULL) {
				uri_list = g_strdup (tmp_uri);
			}
			else {
				uri_list = g_strconcat (uri_list, "\n", tmp_uri, NULL);
			}
			gtk_selection_data_set (selection_data,
			                        selection_data->target,
			                        8,
			                        (guchar *) uri_list,
			                        strlen (uri_list));
			g_free (tmp_uri);
		}
		else {
			gtk_selection_data_set_text (selection_data, utf8_name, -1);
		}
		g_free (utf8_name);
		g_free (locale_file);
	}
	g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (list);
	g_free (uri_list);
}


void
show_file_selector_cb (GtkAction * action,
                       gpointer data)
{
	GSearchWindow * gsearch = data;
	GtkWidget * file_chooser;

	file_chooser = gtk_file_chooser_dialog_new (_("Save Search Results As..."),
	                                            GTK_WINDOW (gsearch->window),
	                                            GTK_FILE_CHOOSER_ACTION_SAVE,
	                                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	                                            GTK_STOCK_SAVE, GTK_RESPONSE_OK,
	                                            NULL);

#if GTK_CHECK_VERSION(2,7,3)
	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (file_chooser), TRUE);
#endif
	if (gsearch->save_results_as_default_filename != NULL) {
		gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (file_chooser),
		                               gsearch->save_results_as_default_filename);
	}

	g_signal_connect (G_OBJECT (file_chooser), "response",
			  G_CALLBACK (save_results_cb), gsearch);

	gtk_window_set_modal (GTK_WINDOW (file_chooser), TRUE);
	gtk_window_set_position (GTK_WINDOW (file_chooser), GTK_WIN_POS_CENTER_ON_PARENT);

	gtk_widget_show (GTK_WIDGET (file_chooser));
}

static void
display_dialog_could_not_save_no_name (GtkWidget * window)
{
	GtkWidget * dialog;
	gchar * primary;
	gchar * secondary;

	primary = g_strdup (_("Could not save document."));
	secondary = g_strdup (_("You did not select a document name."));

	dialog = gtk_message_dialog_new (GTK_WINDOW (window),
	                                 GTK_DIALOG_DESTROY_WITH_PARENT,
	                                 GTK_MESSAGE_ERROR,
	                                 GTK_BUTTONS_OK,
	                                 primary, NULL);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
	                                          secondary, NULL);

	gtk_window_set_title (GTK_WINDOW (dialog), "");
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 14);

	g_signal_connect (G_OBJECT (dialog),
	                  "response",
	                  G_CALLBACK (gtk_widget_destroy), NULL);
	gtk_widget_show (dialog);
	g_free (primary);
	g_free (secondary);
}

static void
display_dialog_could_not_save_to (GtkWidget * window,
                                  const gchar * file,
                                  const gchar * message)
{
	GtkWidget * dialog;
	gchar * primary;

	primary = g_strdup_printf (_("Could not save \"%s\" document to \"%s\"."),
	                           g_path_get_basename (file),
	                           g_path_get_dirname (file));

	dialog = gtk_message_dialog_new (GTK_WINDOW (window),
	                                 GTK_DIALOG_DESTROY_WITH_PARENT,
	                                 GTK_MESSAGE_ERROR,
	                                 GTK_BUTTONS_OK,
	                                 primary, NULL);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
	                                          message, NULL);

	gtk_window_set_title (GTK_WINDOW (dialog), "");
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 14);

	g_signal_connect (G_OBJECT (dialog),
	                  "response",
	                  G_CALLBACK (gtk_widget_destroy), NULL);
	gtk_widget_show (dialog);
	g_free (primary);
}

#if !GTK_CHECK_VERSION(2,7,3)
static gint
display_dialog_could_not_save_exists (GtkWidget * window,
                                      const gchar * file)
{
	GtkWidget * dialog;
	GtkWidget * button;
	gchar * primary;
	gchar * secondary;
	gint response;

	primary = g_strdup_printf (_("The document \"%s\" already exists.  "
	                             "Would you like to replace it?"),
	                           g_path_get_basename (file));

	secondary = g_strdup (_("If you replace an existing file, "
	                        "its contents will be overwritten."));

	dialog = gtk_message_dialog_new (GTK_WINDOW (window),
	                                 GTK_DIALOG_DESTROY_WITH_PARENT,
	                                 GTK_MESSAGE_QUESTION,
	                                 GTK_BUTTONS_CANCEL,
	                                 primary);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
	                                          secondary);

	gtk_window_set_title (GTK_WINDOW (dialog), "");
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 14);

	button = gsearchtool_button_new_with_stock_icon (_("_Replace"), GTK_STOCK_OK);
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	gtk_widget_show (button);

	gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button, GTK_RESPONSE_OK);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

	response = gtk_dialog_run (GTK_DIALOG (dialog));

	gtk_widget_destroy (GTK_WIDGET(dialog));
	g_free (primary);
	g_free (secondary);

	return response;
}
#endif

void
save_results_cb (GtkWidget * chooser,
                 gint response,
                 gpointer data)
{
	GSearchWindow * gsearch = data;
	GtkListStore * store;
	GtkTreeIter iter;
	FILE * fp;
	gchar * utf8 = NULL;

	if (response != GTK_RESPONSE_OK) {
		gtk_widget_destroy (GTK_WIDGET (chooser));
		return;
	}

	store = gsearch->search_results_list_store;
	g_free (gsearch->save_results_as_default_filename);

	gsearch->save_results_as_default_filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
	gtk_widget_destroy (chooser);

	if (gsearch->save_results_as_default_filename != NULL) {
		utf8 = g_filename_to_utf8 (gsearch->save_results_as_default_filename, -1, NULL, NULL, NULL);
	}

	if (utf8 == NULL) {
		display_dialog_could_not_save_no_name (gsearch->window);
		return;
	}

	if (g_file_test (gsearch->save_results_as_default_filename, G_FILE_TEST_IS_DIR)) {
		display_dialog_could_not_save_to (gsearch->window, utf8,
		                                  _("The document name you selected is a folder."));
		g_free (utf8);
		return;
	}

#if !GTK_CHECK_VERSION(2,7,3)
	if (g_file_test (gsearch->save_results_as_default_filename, G_FILE_TEST_EXISTS)) {

		gint response;

		response = display_dialog_could_not_save_exists (gsearch->window, utf8);

		if (response != GTK_RESPONSE_OK) {
			g_free (utf8);
			return;
		}
	}
#endif

	if ((fp = fopen (gsearch->save_results_as_default_filename, "w")) != NULL) {

		gint idx;

		for (idx = 0; idx < gtk_tree_model_iter_n_children (GTK_TREE_MODEL (store), NULL); idx++)
		{
			if (gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (store), &iter, NULL, idx) == TRUE) {

				gchar * locale_file;

				gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, COLUMN_LOCALE_FILE, &locale_file, -1);
				fprintf (fp, "%s\n", locale_file);
				g_free (locale_file);
			}
		}
		fclose (fp);
	}
	else {
		display_dialog_could_not_save_to (gsearch->window, utf8,
		                                  _("You may not have write permissions to the document."));
	}
	g_free (utf8);
}

void
save_session_cb (EggSMClient * client,
                 GKeyFile * state_file,
                 gpointer client_data)
{
	GSearchWindow * gsearch = client_data;
	char ** argv;
	int argc;

	set_clone_command (gsearch, &argc, &argv, "mate-search-tool", FALSE);
	egg_sm_client_set_restart_command (client, argc, (const char **) argv);
}

gboolean
key_press_cb (GtkWidget * widget,
              GdkEventKey * event,
              gpointer data)
{
	GSearchWindow * gsearch = data;

	g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);

	if (event->keyval == GDK_Escape) {
		if (gsearch->command_details->command_status == RUNNING) {
			click_stop_cb (widget, data);
		}
		else if (gsearch->command_details->is_command_timeout_enabled == FALSE) {
			quit_cb (widget, (GdkEvent *) NULL, data);
		}
	}
	else if (event->keyval == GDK_F10) {
		if (event->state & GDK_SHIFT_MASK) {
			gboolean no_files_found = FALSE;
			GtkTreeModel * model;
			GtkTreeIter iter;
			GList * list;

			if (gtk_tree_selection_count_selected_rows (GTK_TREE_SELECTION (gsearch->search_results_selection)) == 0) {
				return FALSE;
			}

			list = gtk_tree_selection_get_selected_rows (GTK_TREE_SELECTION (gsearch->search_results_selection),
			                                             &model);

			gtk_tree_model_get_iter (GTK_TREE_MODEL (gsearch->search_results_list_store), &iter,
						 g_list_first (list)->data);

			gtk_tree_model_get (GTK_TREE_MODEL (gsearch->search_results_list_store), &iter,
					    COLUMN_NO_FILES_FOUND, &no_files_found, -1);

			g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
			g_list_free (list);

			if (!no_files_found) {
				gtk_menu_popup (GTK_MENU (gsearch->search_results_popup_menu), NULL, NULL, NULL, NULL,
				                event->keyval, event->time);
				return TRUE;
			}
		}
	}
	return FALSE;
}

gboolean
not_running_timeout_cb (gpointer data)
{
	GSearchWindow * gsearch = data;

	gsearch->command_details->is_command_timeout_enabled = FALSE;
	return FALSE;
}

void
disable_quick_search_cb (GtkWidget * dialog,
                         gint response,
                         gpointer data)
{
	gtk_widget_destroy (GTK_WIDGET (dialog));

	if (response == GTK_RESPONSE_OK) {
		gsearchtool_mateconf_set_boolean ("/apps/mate-search-tool/disable_quick_search", TRUE);
	}
}

void
single_click_to_activate_key_changed_cb (MateConfClient * client,
                                         guint cnxn_id,
                                         MateConfEntry * entry,
                                         gpointer user_data)
{
	GSearchWindow * gsearch = user_data;
	MateConfValue * value;

	value = mateconf_entry_get_value (entry);

	g_return_if_fail (value->type == MATECONF_VALUE_STRING);

	gsearch->is_search_results_single_click_to_activate =
		(strncmp (mateconf_value_get_string (value), "single", 6) == 0) ? TRUE : FALSE;
}

void
columns_changed_cb (GtkTreeView * treeview,
                    gpointer user_data)
{
	GSList * order;

	order = gsearchtool_get_columns_order (treeview);

	if (g_slist_length (order) == NUM_VISIBLE_COLUMNS) {
		gsearchtool_mateconf_set_list ("/apps/mate-search-tool/columns_order", order, MATECONF_VALUE_INT);
	}
	g_slist_free (order);
}

gboolean
window_state_event_cb (GtkWidget * widget,
                       GdkEventWindowState * event,
                       gpointer data)
{
	GSearchWindow * gsearch = data;

	if (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) {
		gsearch->is_window_maximized = TRUE;
	}
	else {
		gsearch->is_window_maximized = FALSE;
	}
	return FALSE;
}
