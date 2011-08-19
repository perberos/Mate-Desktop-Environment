/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2006 Carlos Garnacho
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
 * Authors: Carlos Garnacho Parro <carlosg@mate.org>
 */

#include "gst.h"
#include "users-table.h"
#include "table.h"
#include "group-members-table.h"

extern GstTool *tool;

static GtkTreeModel*
get_model (void)
{
	GtkWidget *users_table;
	GtkTreeModel *model;

	users_table = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "users_table");
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (users_table));

	return model;
}

static void
on_group_member_toggled (GtkCellRendererToggle *cell, gchar *path_str, gpointer data)
{
	GtkTreeModel *model, *sort_model, *filter_model;
	GtkTreePath *sort_path, *filter_path, *path;
	GtkTreeIter iter;
	gboolean value;

	sort_model = (GtkTreeModel*) data;
	filter_model = gtk_tree_model_sort_get_model (GTK_TREE_MODEL_SORT (sort_model));
	model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (filter_model));

	sort_path = gtk_tree_path_new_from_string (path_str);
	filter_path = gtk_tree_model_sort_convert_path_to_child_path (GTK_TREE_MODEL_SORT (sort_model),
	                                                              sort_path);
	path = gtk_tree_model_filter_convert_path_to_child_path (GTK_TREE_MODEL_FILTER (filter_model),
	                                                         filter_path);

	if (gtk_tree_model_get_iter (GTK_TREE_MODEL (model), &iter, path)) {
		gtk_tree_model_get (model, &iter, COL_USER_MEMBER, &value, -1);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
				    COL_USER_MEMBER, !value,
				    -1);
	}

	gtk_tree_path_free (sort_path);
	gtk_tree_path_free (filter_path);
	gtk_tree_path_free (path);
}

static void
user_name_cell_data_func (GtkCellLayout   *layout,
			  GtkCellRenderer *renderer,
			  GtkTreeModel    *model,
			  GtkTreeIter     *iter,
			  gpointer         data)
{
	OobsUser *user;
	const gchar *name;

	gtk_tree_model_get (model, iter,
			    COL_USER_OBJECT, &user,
			    -1);

	name = oobs_user_get_full_name_fallback (user);

	g_object_set (renderer, "text", name, NULL);
	g_object_unref (user);
}

void
create_group_members_table (void)
{
	GtkWidget *list;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeModel *model;

	list = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "group_settings_members");
	model = get_model ();

	gtk_tree_view_set_model (GTK_TREE_VIEW (list), model);
	column = gtk_tree_view_column_new ();

	renderer = gtk_cell_renderer_toggle_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column,
					     renderer,
					     "active", COL_USER_MEMBER,
					     NULL);
	g_signal_connect (G_OBJECT (renderer), "toggled", G_CALLBACK (on_group_member_toggled), model);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_end (column, renderer, TRUE);
	gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (column), renderer,
					    user_name_cell_data_func,
					    NULL, NULL);

	gtk_tree_view_insert_column (GTK_TREE_VIEW (list), column, 0);
}

void
group_members_table_set_from_group (OobsGroup *group)
{
	GtkWidget *table;
	GtkTreeModel *sort_model, *filter_model, *model;
	GtkTreeIter iter;
	GList *users;
	gboolean valid;
	OobsUser *user;

	table = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "group_settings_members");
	sort_model = gtk_tree_view_get_model (GTK_TREE_VIEW (table));
	filter_model = gtk_tree_model_sort_get_model (GTK_TREE_MODEL_SORT (sort_model));
	model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (filter_model));
	users = oobs_group_get_users (group);

	valid = gtk_tree_model_get_iter_first (model, &iter);

	while (valid) {
		gtk_tree_model_get (model, &iter,
				    COL_USER_OBJECT, &user,
				    -1);

		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
				    COL_USER_MEMBER, (g_list_find (users, user) != NULL),
				    -1);

		valid = gtk_tree_model_iter_next (model, &iter);
	}
	
	g_list_free (users);
}

void
group_members_table_save (OobsGroup *group)
{
	GtkWidget *table;
	GtkTreeModel *model;
	GtkTreeIter iter;
	OobsUser *user;
	gboolean valid, member;

	table = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "group_settings_members");
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (table));
	valid = gtk_tree_model_get_iter_first (model, &iter);

	while (valid) {
		gtk_tree_model_get (model, &iter,
				    COL_USER_OBJECT, &user,
				    COL_USER_MEMBER, &member,
				    -1);
		if (member)
			oobs_group_add_user (group, user);
		else
			oobs_group_remove_user (group, user);

		valid = gtk_tree_model_iter_next (model, &iter);
	}
}
