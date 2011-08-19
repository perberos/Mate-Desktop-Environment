/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* users-table.c: this file is part of shares-admin, a mate-system-tool frontend
 * for shared folders administration.
 *
 * Copyright (C) 2007 Carlos Garnacho
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
 * Authors: Carlos Garnacho <carlosg@mate.org>.
 */

#include "users-table.h"

enum {
	COL_ACTIVE,
	COL_NAME,
	COL_USER,
	LAST_COL
};

static void
user_toggled_cb (GtkCellRendererToggle *renderer,
		 gchar                 *path,
		 gpointer               user_data)
{
	GstSharesTool *tool;
	GtkWidget *table;
	GtkTreeModel *model;
	GtkTreeIter iter;
	OobsUser *user;
	gboolean active;

	tool = GST_SHARES_TOOL (user_data);
	table = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "users_table");
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (table));

	if (! gtk_tree_model_get_iter_from_string (model, &iter, path))
		return;

	gtk_tree_model_get (model, &iter,
			    COL_ACTIVE, &active,
			    COL_USER, &user,
			    -1);

	/* we want to invert the state */
	active ^= 1;

	if (!active) {
		oobs_smb_config_delete_user_password (OOBS_SMB_CONFIG (tool->smb_config), user);
	} else {
		GtkWidget *dialog;
		gint response;

		dialog = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "smb_password_dialog");
		gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (GST_TOOL (tool)->main_dialog));
		response = gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_hide (dialog);

		if (response == GTK_RESPONSE_OK) {
			GtkWidget *entry;
			const gchar *password;

			entry = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "smb_password_entry");
			password = gtk_entry_get_text (GTK_ENTRY (entry));

			oobs_smb_config_set_user_password (OOBS_SMB_CONFIG (tool->smb_config), user, password);
		}
	}

	active = oobs_smb_config_user_has_password (OOBS_SMB_CONFIG (tool->smb_config), user);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter,
			    COL_ACTIVE, active,
			    -1);
	g_object_unref (user);

	gst_tool_commit_async (GST_TOOL (tool), tool->smb_config, NULL, NULL, NULL);
}

static void
users_table_text_cell_data_func (GtkTreeViewColumn *column,
				 GtkCellRenderer   *renderer,
				 GtkTreeModel      *model,
				 GtkTreeIter       *iter,
				 gpointer           user_data)
{
	OobsUser *user;
	const gchar *name;

	gtk_tree_model_get (model, iter,
			    COL_USER, &user,
			    -1);

	name = oobs_user_get_full_name (user);

	if (!name) {
		name = oobs_user_get_login_name (user);
	}

	g_object_set (renderer, "text", name, NULL);
	g_object_unref (user);
}

void
users_table_create (GstTool *tool)
{
	GtkWidget *table;
	GtkCellRenderer *renderer;

	table = gst_dialog_get_widget (tool->main_dialog, "users_table");
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (table), FALSE);

	renderer = gtk_cell_renderer_toggle_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (table),
						     -1, "",
						     renderer,
						     "active", COL_ACTIVE,
						     NULL);
	g_signal_connect (renderer, "toggled",
			  G_CALLBACK (user_toggled_cb), tool);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_data_func (GTK_TREE_VIEW (table),
						    -1, "",
						    renderer,
						    users_table_text_cell_data_func,
						    g_object_ref (tool),
						    (GDestroyNotify) g_object_unref);
}

void
users_table_set_config (GstSharesTool *tool)
{
	GtkWidget *table;
	GtkListStore *store;
	GtkTreeIter iter;
	OobsList *users;
	OobsListIter list_iter;
	gboolean valid;

	table = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "users_table");
	store = gtk_list_store_new (LAST_COL, G_TYPE_BOOLEAN, G_TYPE_STRING, OOBS_TYPE_USER);

	users = oobs_users_config_get_users (OOBS_USERS_CONFIG (tool->users_config));
	valid = oobs_list_get_iter_first (users, &list_iter);

	while (valid) {
		GObject *user;
		gboolean active;

		user = oobs_list_get (users, &list_iter);
		active = oobs_smb_config_user_has_password (OOBS_SMB_CONFIG (tool->smb_config),
							    OOBS_USER (user));

		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
				    COL_USER, user,
				    COL_NAME, oobs_user_get_full_name (OOBS_USER (user)),
				    COL_ACTIVE, active,
				    -1);

		g_object_unref (user);
		valid = oobs_list_iter_next (users, &list_iter);
	}

	gtk_tree_view_set_model (GTK_TREE_VIEW (table), GTK_TREE_MODEL (store));
	g_object_unref (store);
}
