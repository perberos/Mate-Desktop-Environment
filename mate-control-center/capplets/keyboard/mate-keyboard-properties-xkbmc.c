/* -*- mode: c; style: linux -*- */

/* mate-keyboard-properties-xkbmc.c
 * Copyright (C) 2003-2007 Sergey V. Udaltsov
 *
 * Written by: Sergey V. Udaltsov <svu@gnome.org>
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gdk/gdkx.h>
#include <mateconf/mateconf-client.h>
#include <glib/gi18n.h>

#include "capplet-util.h"

#include "mate-keyboard-properties-xkb.h"

static gchar *current_model_name = NULL;
static gchar *current_vendor_name = NULL;

static void fill_models_list (GtkBuilder * chooser_dialog);

static gboolean fill_vendors_list (GtkBuilder * chooser_dialog);

static GtkTreePath *
gtk_list_store_find_entry (GtkListStore * list_store,
			   GtkTreeIter * iter, gchar * name, int column_id)
{
	GtkTreePath *path;
	char *current_name = NULL;
	if (gtk_tree_model_get_iter_first
	    (GTK_TREE_MODEL (list_store), iter)) {
		do {
			gtk_tree_model_get (GTK_TREE_MODEL
					    (list_store), iter, column_id,
					    &current_name, -1);
			if (!g_ascii_strcasecmp (name, current_name)) {
				path =
				    gtk_tree_model_get_path
				    (GTK_TREE_MODEL (list_store), iter);
				return path;
			}
			g_free (current_name);
		} while (gtk_tree_model_iter_next
			 (GTK_TREE_MODEL (list_store), iter));
	}
	return NULL;
}

static void
add_vendor_to_list (XklConfigRegistry * config_registry,
		    XklConfigItem * config_item,
		    GtkTreeView * vendors_list)
{
	GtkTreeIter iter;
	GtkTreePath *found_existing;
	GtkListStore *list_store;

	gchar *vendor_name =
	    (gchar *) g_object_get_data (G_OBJECT (config_item),
					 XCI_PROP_VENDOR);

	if (vendor_name == NULL)
		return;

	list_store =
	    GTK_LIST_STORE (gtk_tree_view_get_model (vendors_list));

	if (!g_ascii_strcasecmp (config_item->name, current_model_name)) {
		current_vendor_name = g_strdup (vendor_name);
	}

	found_existing =
	    gtk_list_store_find_entry (list_store, &iter, vendor_name, 0);
	/* This vendor is already there */
	if (found_existing != NULL) {
		gtk_tree_path_free (found_existing);
		return;
	}

	gtk_list_store_append (list_store, &iter);
	gtk_list_store_set (list_store, &iter, 0, vendor_name, -1);
}

static void
add_model_to_list (XklConfigRegistry * config_registry,
		   XklConfigItem * config_item, GtkTreeView * models_list)
{
	GtkTreeIter iter;
	GtkListStore *list_store =
	    GTK_LIST_STORE (gtk_tree_view_get_model (models_list));
	char *utf_model_name;
	if (current_vendor_name != NULL) {
		gchar *vendor_name =
		    (gchar *) g_object_get_data (G_OBJECT (config_item),
						 XCI_PROP_VENDOR);
		if (vendor_name == NULL)
			return;

		if (g_ascii_strcasecmp (vendor_name, current_vendor_name))
			return;
	}
	utf_model_name = xci_desc_to_utf8 (config_item);
	gtk_list_store_append (list_store, &iter);
	gtk_list_store_set (list_store, &iter,
			    0, utf_model_name, 1, config_item->name, -1);

	g_free (utf_model_name);
}

static void
xkb_model_chooser_change_vendor_sel (GtkTreeSelection * selection,
				     GtkBuilder * chooser_dialog)
{
	GtkTreeIter iter;
	GtkTreeModel *list_store = NULL;
	if (gtk_tree_selection_get_selected
	    (selection, &list_store, &iter)) {
		gchar *vendor_name = NULL;
		gtk_tree_model_get (list_store, &iter,
				    0, &vendor_name, -1);

		current_vendor_name = vendor_name;
		fill_models_list (chooser_dialog);
		g_free (vendor_name);
	} else {
		current_vendor_name = NULL;
		fill_models_list (chooser_dialog);
	}

}

static void
xkb_model_chooser_change_model_sel (GtkTreeSelection * selection,
				    GtkBuilder * chooser_dialog)
{
	gboolean anysel =
	    gtk_tree_selection_get_selected (selection, NULL, NULL);
	gtk_dialog_set_response_sensitive (GTK_DIALOG
					   (CWID ("xkb_model_chooser")),
					   GTK_RESPONSE_OK, anysel);
}

static void
prepare_vendors_list (GtkBuilder * chooser_dialog)
{
	GtkWidget *vendors_list = CWID ("vendors_list");
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
	GtkTreeViewColumn *vendor_col =
	    gtk_tree_view_column_new_with_attributes (_("Vendors"),
						      renderer,
						      "text", 0,
						      NULL);
	gtk_tree_view_column_set_visible (vendor_col, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (vendors_list),
				     vendor_col);
}

static gboolean
fill_vendors_list (GtkBuilder * chooser_dialog)
{
	GtkWidget *vendors_list = CWID ("vendors_list");
	GtkListStore *list_store = gtk_list_store_new (1, G_TYPE_STRING);
	GtkTreeIter iter;
	GtkTreePath *path;

	gtk_tree_view_set_model (GTK_TREE_VIEW (vendors_list),
				 GTK_TREE_MODEL (list_store));

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE
					      (list_store), 0,
					      GTK_SORT_ASCENDING);

	current_vendor_name = NULL;

	xkl_config_registry_foreach_model (config_registry,
					   (ConfigItemProcessFunc)
					   add_vendor_to_list,
					   vendors_list);

	if (current_vendor_name != NULL) {
		path = gtk_list_store_find_entry (list_store,
						  &iter,
						  current_vendor_name, 0);
		if (path != NULL) {
			gtk_tree_selection_select_iter
			    (gtk_tree_view_get_selection
			     (GTK_TREE_VIEW (vendors_list)), &iter);
			gtk_tree_view_scroll_to_cell
			    (GTK_TREE_VIEW (vendors_list),
			     path, NULL, TRUE, 0.5, 0);
			gtk_tree_path_free (path);
		}
		fill_models_list (chooser_dialog);
		g_free (current_vendor_name);
	} else {
		fill_models_list (chooser_dialog);
	}

	g_signal_connect (G_OBJECT
			  (gtk_tree_view_get_selection
			   (GTK_TREE_VIEW (vendors_list))), "changed",
			  G_CALLBACK (xkb_model_chooser_change_vendor_sel),
			  chooser_dialog);

	return gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list_store),
					      &iter);
}

static void
prepare_models_list (GtkBuilder * chooser_dialog)
{
	GtkWidget *models_list = CWID ("models_list");
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
	GtkTreeViewColumn *description_col =
	    gtk_tree_view_column_new_with_attributes (_("Models"),
						      renderer,
						      "text", 0,
						      NULL);
	gtk_tree_view_column_set_visible (description_col, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (models_list),
				     description_col);
}

static void
fill_models_list (GtkBuilder * chooser_dialog)
{
	GtkWidget *models_list = CWID ("models_list");
	GtkTreeIter iter;
	GtkTreePath *path;

	GtkListStore *list_store =
	    gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE
					      (list_store), 0,
					      GTK_SORT_ASCENDING);

	gtk_tree_view_set_model (GTK_TREE_VIEW (models_list),
				 GTK_TREE_MODEL (list_store));

	xkl_config_registry_foreach_model (config_registry,
					   (ConfigItemProcessFunc)
					   add_model_to_list, models_list);

	if (current_model_name != NULL) {
		path = gtk_list_store_find_entry (list_store,
						  &iter,
						  current_model_name, 1);
		if (path != NULL) {
			gtk_tree_selection_select_iter
			    (gtk_tree_view_get_selection
			     (GTK_TREE_VIEW (models_list)), &iter);
			gtk_tree_view_scroll_to_cell
			    (GTK_TREE_VIEW (models_list),
			     path, NULL, TRUE, 0.5, 0);
			gtk_tree_path_free (path);
		}
	}

	g_signal_connect (G_OBJECT
			  (gtk_tree_view_get_selection
			   (GTK_TREE_VIEW (models_list))), "changed",
			  G_CALLBACK (xkb_model_chooser_change_model_sel),
			  chooser_dialog);
}

static void
xkb_model_chooser_response (GtkDialog * dialog,
			    gint response, GtkBuilder * chooser_dialog)
{
	if (response == GTK_RESPONSE_OK) {
		GtkWidget *models_list = CWID ("models_list");
		GtkTreeSelection *selection =
		    gtk_tree_view_get_selection (GTK_TREE_VIEW
						 (models_list));
		GtkTreeIter iter;
		GtkTreeModel *list_store = NULL;
		if (gtk_tree_selection_get_selected
		    (selection, &list_store, &iter)) {
			gchar *model_name = NULL;
			gtk_tree_model_get (list_store, &iter,
					    1, &model_name, -1);

			mateconf_client_set_string (xkb_mateconf_client,
						 MATEKBD_KEYBOARD_CONFIG_KEY_MODEL,
						 model_name, NULL);
			g_free (model_name);
		}
	}
}

void
choose_model (GtkBuilder * dialog)
{
	GtkBuilder *chooser_dialog;
    GtkWidget *chooser;

    chooser_dialog = gtk_builder_new ();
    gtk_builder_add_from_file (chooser_dialog, MATECC_UI_DIR
                               "/mate-keyboard-properties-model-chooser.ui",
                               NULL);
	chooser = CWID ("xkb_model_chooser");
	gtk_window_set_transient_for (GTK_WINDOW (chooser),
				      GTK_WINDOW (WID
						  ("keyboard_dialog")));
	current_model_name =
	    mateconf_client_get_string (xkb_mateconf_client,
				     MATEKBD_KEYBOARD_CONFIG_KEY_MODEL, NULL);


	prepare_vendors_list (chooser_dialog);
	prepare_models_list (chooser_dialog);

	if (!fill_vendors_list (chooser_dialog)) {
		gtk_widget_hide_all (CWID ("vendors_label"));
		gtk_widget_hide_all (CWID ("vendors_scrolledwindow"));
		current_vendor_name = NULL;
		fill_models_list (chooser_dialog);
	}

	g_signal_connect (G_OBJECT (chooser),
			  "response",
			  G_CALLBACK (xkb_model_chooser_response),
			  chooser_dialog);
	gtk_dialog_run (GTK_DIALOG (chooser));
	gtk_widget_destroy (chooser);
	g_free (current_model_name);
}
