/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2001, 2002 Anders Carlsson <andersca@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include "mateconf-bookmarks-dialog.h"
#include <mateconf/mateconf-client.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "mateconf-stock-icons.h"

#define BOOKMARKS_KEY "/apps/mateconf-editor/bookmarks"

G_DEFINE_TYPE(MateConfBookmarksDialog, mateconf_bookmarks_dialog, GTK_TYPE_DIALOG)

static void
mateconf_bookmarks_dialog_response (GtkDialog *dialog, gint response_id)
{
	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
mateconf_bookmarks_dialog_destroy (GtkObject *object)
{
	MateConfClient *client;
	MateConfBookmarksDialog *dialog;
	
	client = mateconf_client_get_default ();
	dialog = MATECONF_BOOKMARKS_DIALOG (object);
	
	if (dialog->notify_id != 0) {
		mateconf_client_notify_remove (client, dialog->notify_id);
		mateconf_client_remove_dir (client, BOOKMARKS_KEY, NULL);
		dialog->notify_id = 0;
	}

	g_object_unref (client);
	
	if (GTK_OBJECT_CLASS (mateconf_bookmarks_dialog_parent_class)->destroy) {
		(* GTK_OBJECT_CLASS (mateconf_bookmarks_dialog_parent_class)->destroy) (object);
	}
}

static void
mateconf_bookmarks_dialog_class_init (MateConfBookmarksDialogClass *klass)
{
	GtkDialogClass *dialog_class;
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass *)klass;
	dialog_class = (GtkDialogClass *)klass;

	object_class->destroy = mateconf_bookmarks_dialog_destroy;
	dialog_class->response = mateconf_bookmarks_dialog_response;
}

static void
mateconf_bookmarks_dialog_populate_model (MateConfBookmarksDialog *dialog)
{
	MateConfClient *client;
	GSList *value_list, *p;
	GtkTreeIter iter;
	
	client = mateconf_client_get_default ();
	
	/* First clear the list store */
	dialog->changing_model = TRUE;
	gtk_list_store_clear (dialog->list_store);

	value_list = mateconf_client_get_list (client, BOOKMARKS_KEY,
					    MATECONF_VALUE_STRING, NULL);

	for (p = value_list; p; p = p->next) {
		gtk_list_store_append (dialog->list_store, &iter);
		gtk_list_store_set (dialog->list_store, &iter,
				    0, p->data,
				    -1);
		g_free (p->data);
	}

	if (value_list)
		g_slist_free (value_list);

	dialog->changing_model = FALSE;

	g_object_unref (client);
}

static void
mateconf_bookmarks_dialog_selection_changed (GtkTreeSelection *selection, MateConfBookmarksDialog *dialog)
{
	gtk_widget_set_sensitive (dialog->delete_button,
				  gtk_tree_selection_get_selected (selection, NULL, NULL));

}

static void
mateconf_bookmarks_dialog_update_mateconf_key (MateConfBookmarksDialog *dialog)
{
	GSList *list;
	GtkTreeIter iter;
	char *bookmark;
	MateConfClient *client;
	
	list = NULL;

	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (dialog->list_store), &iter)) {
		do {
			gtk_tree_model_get (GTK_TREE_MODEL (dialog->list_store), &iter,
					    0, &bookmark,
					    -1);
			list = g_slist_append (list, bookmark);
		} while (gtk_tree_model_iter_next (GTK_TREE_MODEL (dialog->list_store), &iter));
	}

	client = mateconf_client_get_default ();

	dialog->changing_key = TRUE;
	mateconf_client_set_list (client, BOOKMARKS_KEY,
			       MATECONF_VALUE_STRING, list, NULL);

	g_object_unref (client);
}

static void
mateconf_bookmarks_dialog_row_deleted (GtkTreeModel *tree_model, GtkTreePath *path,
				    MateConfBookmarksDialog *dialog)
{
	if (dialog->changing_model) {
		return;
	}

	mateconf_bookmarks_dialog_update_mateconf_key (dialog);

}

static void
mateconf_bookmarks_dialog_delete_bookmark (GtkWidget *button, MateConfBookmarksDialog *dialog)
{
	GtkTreeIter iter;
	
	gtk_tree_selection_get_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->tree_view)),
					 NULL, &iter);

	dialog->changing_model = TRUE;
	gtk_list_store_remove (dialog->list_store, &iter);
	dialog->changing_model = FALSE;

	mateconf_bookmarks_dialog_update_mateconf_key (dialog);
}

void
mateconf_bookmarks_dialog_bookmarks_key_changed (MateConfClient* client,
					      guint cnxn_id,
					      MateConfEntry *entry,
					      gpointer user_data)
{
	MateConfBookmarksDialog *dialog;

	dialog = user_data;

	if (dialog->changing_key) {
		dialog->changing_key = FALSE;
		return;
	}
	
	mateconf_bookmarks_dialog_populate_model (dialog);	
}

static void
mateconf_bookmarks_dialog_init (MateConfBookmarksDialog *dialog)
{
	GtkWidget *scrolled_window, *hbox, *vbox;
	GtkWidget *content_area;
	GtkCellRenderer *cell;
	GtkTreeViewColumn *column;
	MateConfClient *client;
       
	hbox = gtk_hbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);

	content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

	gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
	gtk_box_set_spacing (GTK_BOX (content_area), 2);
	gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);
	gtk_window_set_default_size (GTK_WINDOW (dialog), 300, 200);
	
	gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
	gtk_window_set_title (GTK_WINDOW (dialog), _("Edit Bookmarks"));

	dialog->list_store = gtk_list_store_new (1, G_TYPE_STRING);
	g_signal_connect (dialog->list_store, "row_deleted",
			  G_CALLBACK (mateconf_bookmarks_dialog_row_deleted), dialog);
	scrolled_window = gtk_scrolled_window_new (NULL, NULL);
	gtk_box_pack_start (GTK_BOX (hbox), scrolled_window, TRUE, TRUE, 0);
	
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	dialog->tree_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (dialog->list_store));
	gtk_tree_view_set_reorderable (GTK_TREE_VIEW (dialog->tree_view), TRUE);
	
	g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->tree_view)), "changed",
			  G_CALLBACK (mateconf_bookmarks_dialog_selection_changed), dialog);
	mateconf_bookmarks_dialog_populate_model (dialog);

	column = gtk_tree_view_column_new ();

	cell = gtk_cell_renderer_pixbuf_new ();
	g_object_set (G_OBJECT (cell),
		      "stock-id", STOCK_BOOKMARK,
		      NULL);
	gtk_tree_view_column_pack_start (column, cell, FALSE);
	
	cell = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, cell, TRUE);
	gtk_tree_view_column_set_attributes (column, cell,
					     "text", 0,
					     NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (dialog->tree_view), column);

	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (dialog->tree_view), FALSE);
	
	g_object_unref (dialog->list_store);
	gtk_container_add (GTK_CONTAINER (scrolled_window), dialog->tree_view);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
	dialog->delete_button = gtk_button_new_from_stock (GTK_STOCK_DELETE);
	gtk_widget_set_sensitive (dialog->delete_button, FALSE);
	g_signal_connect (dialog->delete_button, "clicked",
			  G_CALLBACK (mateconf_bookmarks_dialog_delete_bookmark), dialog);
	
	gtk_box_pack_start (GTK_BOX (vbox), dialog->delete_button, FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (content_area), hbox, TRUE, TRUE, 0);
	gtk_widget_show_all (hbox);

	/* Listen for mateconf changes */
	client = mateconf_client_get_default ();
	mateconf_client_add_dir (client, BOOKMARKS_KEY, MATECONF_CLIENT_PRELOAD_NONE, NULL);

	dialog->notify_id = mateconf_client_notify_add (client, BOOKMARKS_KEY,
						     mateconf_bookmarks_dialog_bookmarks_key_changed,
						     dialog,
						     NULL,
						     NULL);

	g_object_unref (client);
}


GtkWidget *
mateconf_bookmarks_dialog_new (GtkWindow *parent)
{
	GtkWidget *dialog;

	dialog = g_object_new (MATECONF_TYPE_BOOKMARKS_DIALOG, NULL);
	gtk_window_set_transient_for (GTK_WINDOW (dialog), parent);
	gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);

	return dialog;
}
