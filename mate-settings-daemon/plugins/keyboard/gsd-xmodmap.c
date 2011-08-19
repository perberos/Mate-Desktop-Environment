/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright © 2005 Novell Inc.
 *
 * Written by Shakti Sen <shprasad@novell.com>
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

#include <string.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <mateconf/mateconf-client.h>

#include "gsd-xmodmap.h"

static const char DISABLE_XMM_WARNING_KEY[] =
    "/desktop/mate/peripherals/keyboard/disable_xmm_and_xkb_warning";

static const char LOADED_FILES_KEY[] =
    "/desktop/mate/peripherals/keyboard/general/update_handlers";


static void
check_button_callback (GtkWidget *chk_button,
                       gpointer   data)
{
        MateConfClient *client;

        client = mateconf_client_get_default ();

        mateconf_client_set_bool (client,
                               DISABLE_XMM_WARNING_KEY,
                               gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (chk_button)),
                               NULL);

        g_object_unref (client);
}

void
gsd_load_modmap_files (void)
{
        MateConfClient *client;
        GSList      *tmp;
        GSList      *loaded_file_list;

        client = mateconf_client_get_default ();

        loaded_file_list = mateconf_client_get_list (client, LOADED_FILES_KEY, MATECONF_VALUE_STRING, NULL);

        for (tmp = loaded_file_list; tmp != NULL; tmp = tmp->next) {
                gchar *file;
                gchar *command;

                file = g_build_filename (g_get_home_dir (), (gchar *) tmp->data, NULL);
                command = g_strconcat ("xmodmap ", file, NULL);
                g_free (file);

                g_spawn_command_line_async (command, NULL);

                g_free (command);
                g_free (tmp->data);
        }

        g_slist_free (loaded_file_list);
        g_object_unref (client);
}

static void
response_callback (GtkWidget *dialog,
                   int        id,
                   void      *data)
{
        if (id == GTK_RESPONSE_OK) {
                GtkWidget *chk_button = g_object_get_data (G_OBJECT (dialog), "check_button");
                check_button_callback (chk_button, NULL);
                gsd_load_modmap_files ();
        }
        gtk_widget_destroy (dialog);
}

static void
get_selected_files_func (GtkTreeModel      *model,
                         GtkTreePath       *path,
                         GtkTreeIter       *iter,
                         gpointer           data)
{
        GSList **list = data;
        gchar *filename;

        filename = NULL;
        gtk_tree_model_get (model,
                            iter,
                            0,
                            &filename,
                            -1);

        *list = g_slist_prepend (*list, filename);
}

static GSList*
remove_string_from_list (GSList     *list,
                         const char *str)
{
        GSList *tmp;

        for (tmp = list; tmp != NULL; tmp = tmp->next) {
                if (strcmp (tmp->data, str) == 0) {
                        g_free (tmp->data);
                        list = g_slist_delete_link (list, tmp);
                        break;
                }
        }

        return list;
}


static void
remove_button_clicked_callback (GtkWidget *button,
                                void      *data)
{
        GtkWidget        *dialog;
        GtkListStore     *tree = NULL;
        GtkTreeSelection *selection;
        GtkWidget        *treeview;
        MateConfClient      *client;
        GSList           *filenames = NULL;
        GSList           *tmp = NULL;
        GSList           *loaded_files = NULL;

        dialog = data;

        treeview = g_object_get_data (G_OBJECT (dialog), "treeview1");

        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
        gtk_tree_selection_selected_foreach (selection,
                                             get_selected_files_func,
                                             &filenames);

        if (!filenames)
                return;

        /* Remove the selected file */

        client = mateconf_client_get_default ();

        loaded_files = mateconf_client_get_list (client,
                                              LOADED_FILES_KEY,
                                              MATECONF_VALUE_STRING,
                                              NULL);
        loaded_files = remove_string_from_list (loaded_files, (char *)filenames->data);

        mateconf_client_set_list (client,
                               LOADED_FILES_KEY,
                               MATECONF_VALUE_STRING,
                               loaded_files,
                               NULL);
        g_object_unref (client);

        tree = g_object_get_data (G_OBJECT (dialog), "tree");

        gtk_list_store_clear (tree);
        for (tmp = loaded_files; tmp != NULL; tmp = tmp->next) {
                GtkTreeIter iter;
                gtk_list_store_append (tree, &iter);
                gtk_list_store_set (tree, &iter,
                                    0,
                                    tmp->data,
                                    -1);
        }

        g_slist_foreach (loaded_files, (GFunc) g_free, NULL);
        g_slist_free (loaded_files);
}

static void
load_button_clicked_callback (GtkWidget *button,
                              void      *data)
{
        GtkWidget        *dialog;
        GtkListStore     *tree = NULL;
        GtkTreeSelection *selection;
        GtkWidget        *treeview;
        GSList           *filenames = NULL;
        GSList           *tmp = NULL;
        GSList           *loaded_files = NULL;
        MateConfClient      *client;

        dialog = data;

        treeview = g_object_get_data (G_OBJECT (dialog),
                                      "loaded-treeview");
        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
        gtk_tree_selection_selected_foreach (selection,
                                             get_selected_files_func,
                                             &filenames);

        if (!filenames)
                return;

        /* Add the files to left-tree-view */
        client = mateconf_client_get_default ();

        loaded_files = mateconf_client_get_list (client,
                                              LOADED_FILES_KEY,
                                              MATECONF_VALUE_STRING,
                                              NULL);

        if (g_slist_find_custom (loaded_files, filenames->data, (GCompareFunc) strcmp)) {
                g_free (filenames->data);
                g_slist_free (filenames);
                goto out;
        }

        loaded_files = g_slist_append (loaded_files, filenames->data);
        mateconf_client_set_list (client,
                               LOADED_FILES_KEY,
                               MATECONF_VALUE_STRING,
                               loaded_files,
                               NULL);


        tree = g_object_get_data (G_OBJECT (dialog), "tree");

        gtk_list_store_clear (tree);
        for (tmp = loaded_files; tmp != NULL; tmp = tmp->next) {
                GtkTreeIter iter;
                gtk_list_store_append (tree, &iter);
                gtk_list_store_set (tree, &iter,
                                    0,
                                    tmp->data,
                                    -1);
        }

out:
        g_object_unref (client);
        g_slist_foreach (loaded_files, (GFunc) g_free, NULL);
        g_slist_free (loaded_files);
}

void
gsd_modmap_dialog_call (void)
{
        GtkBuilder        *builder;
        guint              res;
        GError            *error;
        GtkWidget         *load_dialog;
        GtkListStore      *tree;
        GtkCellRenderer   *cell_renderer;
        GtkTreeIter        parent_iter;
        GtkTreeIter        iter;
        GtkTreeModel      *sort_model;
        GtkTreeSelection  *selection;
        GtkWidget         *treeview;
        GtkWidget         *treeview1;
        GtkTreeViewColumn *column;
        GtkWidget         *add_button;
        GtkWidget         *remove_button;
        GtkWidget         *chk_button;
        GSList            *tmp;
        GDir              *homeDir;
        GSList            *loaded_files;
        const char        *fname;
        MateConfClient       *client;

        homeDir = g_dir_open (g_get_home_dir (), 0, NULL);
        if (homeDir == NULL)
                return;

        error = NULL;
        builder = gtk_builder_new ();
        res = gtk_builder_add_from_file (builder,
                                         DATADIR "/modmap-dialog.ui",
                                         &error);

        if (res == 0) {
                g_warning ("Could not load UI file: %s", error->message);
                g_error_free (error);
                g_object_unref (builder);
                g_dir_close (homeDir);
                return;
        }

        load_dialog = GTK_WIDGET (gtk_builder_get_object (builder, "dialog1"));
        gtk_window_set_modal (GTK_WINDOW (load_dialog), TRUE);
        g_signal_connect (load_dialog,
                          "response",
                          G_CALLBACK (response_callback),
                          builder);
        add_button =  GTK_WIDGET (gtk_builder_get_object (builder, "button7"));
        g_signal_connect (add_button,
                          "clicked",
                          G_CALLBACK (load_button_clicked_callback),
                          load_dialog);
        remove_button =  GTK_WIDGET (gtk_builder_get_object (builder,
                                                             "button6"));
        g_signal_connect (remove_button,
                          "clicked",
                          G_CALLBACK (remove_button_clicked_callback),
                          load_dialog);
        chk_button =  GTK_WIDGET (gtk_builder_get_object (builder,
                                                          "checkbutton1"));
        g_signal_connect (chk_button,
                          "toggled",
                          G_CALLBACK (check_button_callback),
                          NULL);
        g_object_set_data (G_OBJECT (load_dialog), "check_button", chk_button);
        treeview = GTK_WIDGET (gtk_builder_get_object (builder, "treeview1"));
        g_object_set_data (G_OBJECT (load_dialog), "treeview1", treeview);
        treeview =  GTK_WIDGET (gtk_builder_get_object (builder, "treeview2"));
        g_object_set_data (G_OBJECT (load_dialog), "loaded-treeview", treeview);
        tree = gtk_list_store_new (1, G_TYPE_STRING);
        cell_renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes ("modmap",
                                                           cell_renderer,
                                                           "text", 0,
                                                           NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
        gtk_tree_view_column_set_sort_column_id (column, 0);

        /* Add the data */
        while ((fname = g_dir_read_name (homeDir)) != NULL) {
                if (g_strrstr (fname, "modmap")) {
                        gtk_list_store_append (tree, &parent_iter);
                        gtk_list_store_set (tree, &parent_iter,
                                            0,
                                            fname,
                                            -1);
                }
        }
        sort_model = gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL (tree));
        gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (sort_model),
                                              0,
                                              GTK_SORT_ASCENDING);
        gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), sort_model);
        g_object_unref (G_OBJECT (tree));
        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
        gtk_tree_selection_set_mode (GTK_TREE_SELECTION (selection),
                                     GTK_SELECTION_MULTIPLE);
        gtk_widget_show (load_dialog);

        g_dir_close (homeDir);

        /* Left treeview */
        treeview1 =  GTK_WIDGET (gtk_builder_get_object (builder, "treeview1"));
        tree = gtk_list_store_new (1, G_TYPE_STRING);
        cell_renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes ("modmap",
                                                           cell_renderer,
                                                           "text", 0,
                                                           NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview1), column);
        gtk_tree_view_column_set_sort_column_id (column, 0);

        client = mateconf_client_get_default ();
        loaded_files = mateconf_client_get_list (client, LOADED_FILES_KEY, MATECONF_VALUE_STRING, NULL);
        g_object_unref (client);

        /* Add the data */
        for (tmp = loaded_files; tmp != NULL; tmp = tmp->next) {
                gtk_list_store_append (tree, &iter);
                gtk_list_store_set (tree, &iter,
                                    0,
                                    tmp->data,
                                    -1);
        }

        g_slist_foreach (loaded_files, (GFunc) g_free, NULL);
        g_slist_free (loaded_files);

        sort_model = gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL (tree));
        gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (sort_model),
                                              0,
                                              GTK_SORT_ASCENDING);
        gtk_tree_view_set_model (GTK_TREE_VIEW (treeview1), sort_model);
        g_object_unref (G_OBJECT (tree));
        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview1));
        gtk_tree_selection_set_mode (GTK_TREE_SELECTION (selection),
                                     GTK_SELECTION_MULTIPLE);
        g_object_set_data (G_OBJECT (load_dialog), "tree", tree);
        g_object_unref (builder);
}
