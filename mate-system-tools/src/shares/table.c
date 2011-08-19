/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* table.c: this file is part of shares-admin, a mate-system-tool frontend 
 * for shared folders administration.
 * 
 * Copyright (C) 2004 Carlos Garnacho
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors: Carlos Garnacho Parro <carlosg@mate.org>.
 */

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "gst-tool.h"
#include "gst.h"
#include "table.h"
#include "callbacks.h"

extern GstTool *tool;

static GtkTargetEntry drop_types[] = {
	{ "text/uri-list", 0, SHARES_DND_URI_LIST },
};

GtkActionEntry popup_menu_items [] = {
	{ "Add",        GTK_STOCK_ADD,        N_("_Add"),        NULL, NULL, G_CALLBACK (on_add_share_clicked) },
	{ "Properties", GTK_STOCK_PROPERTIES, N_("_Properties"), NULL, NULL, G_CALLBACK (on_edit_share_clicked) },
	{ "Delete",     GTK_STOCK_DELETE,     N_("_Delete"),     NULL, NULL, G_CALLBACK (on_delete_share_clicked) },
};

const gchar *ui_description =
	"<ui>"
	"  <popup name='MainMenu'>"
	"    <menuitem action='Add'/>"
	"    <separator/>"
	"    <menuitem action='Properties'/>"
	"    <menuitem action='Delete'/>"
	"  </popup>"
	"</ui>";

static GtkWidget*
popup_menu_create (GtkTreeView *treeview)
{
	GtkUIManager   *ui_manager;
	GtkActionGroup *action_group;
	GtkWidget      *popup;

	g_return_val_if_fail (treeview != NULL, NULL);

	action_group = gtk_action_group_new ("MenuActions");
	gtk_action_group_set_translation_domain (action_group, NULL);
	gtk_action_group_add_actions (action_group,
				      popup_menu_items,
				      G_N_ELEMENTS (popup_menu_items), treeview);

	ui_manager = gtk_ui_manager_new ();
	gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);

	if (!gtk_ui_manager_add_ui_from_string (ui_manager, ui_description, -1, NULL))
		return NULL;

	popup = gtk_ui_manager_get_widget (ui_manager, "/MainMenu");

	return popup;
}

static GtkTreeModel*
create_table_model (void)
{
	GtkListStore *store;

	store = gtk_list_store_new (COL_LAST,
				    GDK_TYPE_PIXBUF,
				    G_TYPE_STRING,
				    G_TYPE_OBJECT,
				    OOBS_TYPE_LIST_ITER);

	return GTK_TREE_MODEL (store);
}

static void
add_table_columns (GtkTreeView *table)
{
	GtkCellRenderer   *renderer;

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (table), 1,
						     "Share",
						     renderer,
						     "pixbuf", 0,
						     NULL);
	
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (table), 1,
						     "Path",
						     renderer,
						     "text", 1,
						     NULL);
}

void
table_create (GstTool *tool)
{
	GtkWidget        *table = gst_dialog_get_widget (tool->main_dialog, "shares_table");
	GtkWidget        *popup;
	GtkTreeSelection *selection;
	GtkTreeModel     *model;

	model = create_table_model ();
	gtk_tree_view_set_model (GTK_TREE_VIEW (table), model);
	g_object_unref (G_OBJECT (model));

	add_table_columns (GTK_TREE_VIEW (table));

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (table));
	g_signal_connect (G_OBJECT (selection), "changed",
			  G_CALLBACK (on_shares_table_selection_changed), NULL);

	popup = popup_menu_create (GTK_TREE_VIEW (table));
	g_signal_connect (G_OBJECT (table), "button-press-event",
			  G_CALLBACK (on_shares_table_button_press), (gpointer) popup);
	g_signal_connect (G_OBJECT (table), "popup_menu",
			  G_CALLBACK (on_shares_table_popup_menu), (gpointer) popup);

	/* Drag and Drop stuff */
	gtk_drag_dest_unset (table);
	gtk_drag_dest_set (table, GTK_DEST_DEFAULT_ALL, drop_types,
			   sizeof (drop_types) / sizeof (drop_types[0]),
			   GDK_ACTION_COPY);
	g_signal_connect (G_OBJECT (table), "drag_data_received",
			  G_CALLBACK (on_shares_dragged_folder), NULL);
}

void
table_clear (void)
{
	GtkWidget *table = gst_dialog_get_widget (tool->main_dialog, "shares_table");
	GtkTreeModel *model;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (table));
	gtk_list_store_clear (GTK_LIST_STORE (model));
}

static GdkPixbuf*
get_share_icon (OobsShare *share)
{
	GdkPixbuf *pixbuf = NULL;

	if (OOBS_IS_SHARE_SMB (share)) {
		pixbuf = gtk_icon_theme_load_icon (gst_tool_get_icon_theme (tool),
						   "mate-fs-smb",
						   48, 0, NULL);
	} else if (OOBS_IS_SHARE_NFS (share)) {
		pixbuf = gtk_icon_theme_load_icon (gst_tool_get_icon_theme (tool),
						   "mate-fs-nfs",
						   48, 0, NULL);
	}

	return pixbuf;
}

void
table_add_share (OobsShare *share, OobsListIter *list_iter)
{
	GtkWidget    *table = gst_dialog_get_widget (tool->main_dialog, "shares_table");
	GtkTreeModel *model;
	GtkTreeIter   iter;

	g_return_if_fail (share != NULL);
	g_return_if_fail (OOBS_IS_SHARE (share));
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (table));
	gtk_list_store_append (GTK_LIST_STORE (model), &iter);

	gtk_list_store_set (GTK_LIST_STORE (model),
			    &iter,
			    COL_PIXBUF, get_share_icon (share),
			    COL_PATH, oobs_share_get_path (share),
			    COL_SHARE, share,
			    COL_ITER, list_iter,
			    -1);
}

void
table_modify_share_at_iter (GtkTreeIter *iter, OobsShare *share, OobsListIter *list_iter)
{
	GtkWidget    *table = gst_dialog_get_widget (tool->main_dialog, "shares_table");
	GtkTreeModel *model;

	g_return_if_fail (share != NULL);
	g_return_if_fail (OOBS_IS_SHARE (share));

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (table));
	gtk_list_store_set (GTK_LIST_STORE (model),
			    iter,
			    COL_PIXBUF, get_share_icon (share),
			    COL_PATH, oobs_share_get_path (share),
			    COL_SHARE, share,
			    COL_ITER, list_iter,
			    -1);
}

OobsShare*
table_get_share_at_iter (GtkTreeIter *iter, OobsListIter **list_iter)
{
	GtkWidget    *table = gst_dialog_get_widget (tool->main_dialog, "shares_table");
	GtkTreeModel *model;
	OobsShare     *share;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (table));

	gtk_tree_model_get (model, iter,
			    COL_SHARE, &share,
			    COL_ITER, list_iter,
			    -1);
	return share;
}

void
table_delete_share_at_iter (GtkTreeIter *iter)
{
	GtkWidget    *table = gst_dialog_get_widget (tool->main_dialog, "shares_table");
	GtkTreeModel *model;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (table));
	gtk_list_store_remove (GTK_LIST_STORE (model), iter);
}

gboolean
table_get_iter_with_path (const gchar *path, GtkTreeIter *iter)
{
	GtkWidget    *table = gst_dialog_get_widget (tool->main_dialog, "shares_table");
	GtkTreeModel *model;
	gboolean      valid, found;
	gchar        *iter_path;

	if (!path)
		return FALSE;

	found = FALSE;
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (table));
	valid = gtk_tree_model_get_iter_first (model, iter);

	while (valid) {
		gtk_tree_model_get (model, iter,
				    COL_PATH, &iter_path,
				    -1);

		if (strcmp (iter_path, path) == 0) {
			found = TRUE;
			valid = FALSE;
		} else
			valid = gtk_tree_model_iter_next (model, iter);

		g_free (iter_path);
	}

	return found;
}
