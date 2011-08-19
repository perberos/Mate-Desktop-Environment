/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* groups-table.c: this file is part of users-admin, a ximian-setup-tool frontend 
 * for user administration.
 * 
 * Copyright (C) 2000-2001 Ximian, Inc.
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
 * Authors: Carlos Garnacho Parro <garparr@teleline.es>
 */

#include <config.h>
#include "gst.h"
#include <glib/gi18n.h>

#include "table.h"
#include "groups-table.h"
#include "callbacks.h"

extern GstTool *tool;

static void
add_group_columns (GtkTreeView *treeview)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

	column = gtk_tree_view_column_new ();

	/* Group name */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Group name"),
							   renderer,
							   "text", COL_GROUP_NAME, NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sort_column_id (column, 0);
	gtk_tree_view_column_set_expand (column, TRUE);

	gtk_tree_view_insert_column (treeview, column, -1);
}

static GtkTreeModel*
create_groups_model (void)
{
	GtkTreeModel *sort_model;
	GtkListStore *store;

	store = gtk_list_store_new (COL_GROUP_LAST,
	                            G_TYPE_STRING,
	                            G_TYPE_INT,
				    G_TYPE_OBJECT);

	/* Sort model */
	sort_model = gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL (store));
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (sort_model),
	                                      COL_GROUP_NAME, GTK_SORT_ASCENDING);

	return sort_model;
}

void
create_groups_table (void)
{
	GtkWidget *groups_table;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkWidget *popup;
	
	groups_table = gst_dialog_get_widget (tool->main_dialog, "groups_table");

	model = create_groups_model ();
	gtk_tree_view_set_model (GTK_TREE_VIEW (groups_table), model);
	g_object_unref (model);

	add_group_columns (GTK_TREE_VIEW (groups_table));

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (groups_table));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

	popup = popup_menu_create (groups_table, TABLE_GROUPS);
	g_object_set_data_full (G_OBJECT (groups_table),
				"popup", popup,
				(GDestroyNotify) gtk_widget_destroy);

	g_signal_connect (G_OBJECT (selection), "changed",
			  G_CALLBACK (on_table_selection_changed),
			  GINT_TO_POINTER (TABLE_GROUPS));
	g_signal_connect (G_OBJECT (groups_table), "button_press_event",
			  G_CALLBACK (on_table_button_press),
			  GINT_TO_POINTER (TABLE_GROUPS));
	g_signal_connect (G_OBJECT (groups_table), "popup-menu",
			  G_CALLBACK (on_table_popup_menu), NULL);
}

GtkTreeModel *
groups_table_get_model ()
{
	GtkWidget *groups_table;
	GtkTreeModel *sort_model;

	groups_table = gst_dialog_get_widget (tool->main_dialog, "groups_table");
	sort_model = gtk_tree_view_get_model (GTK_TREE_VIEW (groups_table));

	return gtk_tree_model_sort_get_model (GTK_TREE_MODEL_SORT (sort_model));
}

void
groups_table_set_group (OobsGroup *group, GtkTreeIter *iter)
{
	GtkTreeModel *model;

	model = groups_table_get_model ();

	gtk_list_store_set (GTK_LIST_STORE (model), iter,
			    COL_GROUP_NAME, oobs_group_get_name (group),
			    COL_GROUP_ID, oobs_group_get_gid (group),
			    COL_GROUP_OBJECT, group,
			    -1);
}

void
groups_table_add_group (OobsGroup *group)
{
	GtkTreeModel *model;
	GtkTreeIter iter;

	model = groups_table_get_model ();

	gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	groups_table_set_group (group, &iter);
}

void
groups_table_clear (void)
{
	GtkTreeModel *model;

	model = groups_table_get_model ();

	gtk_list_store_clear (GTK_LIST_STORE (model));
}

/*
 * Get selected items, translating them to child GtkListStore references.
 */
GList*
groups_table_get_row_references ()
{
	GtkWidget *groups_table;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeModel *sort_model;
	GtkTreePath *path;
	GList *paths, *elem, *list = NULL;

	groups_table = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "groups_table");
	sort_model = gtk_tree_view_get_model (GTK_TREE_VIEW (groups_table));
	model = gtk_tree_model_sort_get_model (GTK_TREE_MODEL_SORT (sort_model));

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (groups_table));
	paths = elem = gtk_tree_selection_get_selected_rows (selection, NULL);

	if (!paths)
		return NULL;

	while (elem) {
		path = gtk_tree_model_sort_convert_path_to_child_path (GTK_TREE_MODEL_SORT (sort_model),
		                                                       elem->data);

		list = g_list_prepend (list, gtk_tree_row_reference_new (model, path));

		gtk_tree_path_free (path);
		elem = elem->next;
	}

	list = g_list_reverse (list);
	g_list_foreach (paths, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (paths);

	return list;
}

