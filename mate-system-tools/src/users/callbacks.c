/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* callbacks.c: this file is part of users-admin, a ximian-setup-tool frontend 
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
 * Authors: Tambet Ingo <tambet@ximian.com>,
 *          Arturo Espinosa <arturo@ximian.com> and
 *          Carlos Garnacho Parro <garparr@teleline.es>.
 */

#include <config.h>
#include "gst.h"

#include "callbacks.h"
#include "table.h"
#include "user-settings.h"
#include "users-table.h"
#include "users-tool.h"
#include "group-settings.h"
#include "groups-table.h"

extern GstTool *tool;


/* Common stuff to users and groups tables */

void
on_table_selection_changed (GtkTreeSelection *selection, gpointer data)
{
	gint count, table;
	OobsUser *user = NULL;

	table = GPOINTER_TO_INT (data);
	count = gtk_tree_selection_count_selected_rows (selection);

	if (table == TABLE_USERS) {
		user = users_table_get_current ();
	}

	/* this happens when we unselect all users before selecting a single one */
	if (!user)
		return;

	/* Show the settings for the selected user */
	user_settings_show (user);

	g_object_unref (user);
}

static void
do_popup_menu (GtkTreeView *treeview, GdkEventButton *event)
{
	GtkTreeSelection *selection;
	GtkUIManager *ui_manager;
	gint cont, button, event_time;
	GtkWidget *popup;

	popup = g_object_get_data (G_OBJECT (treeview), "popup");

	if (event) {
		button     = event->button;
		event_time = event->time;
	} else {
		button = 0;
		event_time = gtk_get_current_event_time ();
	}

	selection = gtk_tree_view_get_selection (treeview);
	cont = gtk_tree_selection_count_selected_rows (selection);
	ui_manager = g_object_get_data (G_OBJECT (treeview), "ui-manager");

	gtk_widget_set_sensitive (gtk_ui_manager_get_widget (ui_manager, "/MainMenu/Add"),
				  cont == 1);
	gtk_widget_set_sensitive (gtk_ui_manager_get_widget (ui_manager, "/MainMenu/Properties"),
				  cont == 1);
	gtk_widget_set_sensitive (gtk_ui_manager_get_widget (ui_manager, "/MainMenu/Delete"),
				  cont > 0);

	gtk_menu_popup (GTK_MENU (popup), NULL, NULL, NULL, NULL,
			button, event_time);
}

gboolean
on_table_button_press (GtkTreeView *treeview, GdkEventButton *event, gpointer data)
{
	GtkTreePath *path;
	GtkTreeSelection *selection;
	gint cont, table;

	table = GPOINTER_TO_INT (data);
	selection = gtk_tree_view_get_selection (treeview);
	cont = gtk_tree_selection_count_selected_rows (selection);

	if (event->window != gtk_tree_view_get_bin_window (treeview))
		return FALSE;

	if (event->button == 3)	{
		gtk_widget_grab_focus (GTK_WIDGET (treeview));
		
		if (gtk_tree_view_get_path_at_pos (treeview, event->x, event->y, &path, NULL, NULL, NULL)) {
			if (cont < 1) {
				gtk_tree_selection_unselect_all (selection);
				gtk_tree_selection_select_path (selection, path);
			}

			gtk_tree_path_free (path);

			do_popup_menu (treeview, event);
		}

		return TRUE;
	}

	if (cont != 0 && (event->type == GDK_2BUTTON_PRESS || event->type == GDK_3BUTTON_PRESS)
	    && (table == TABLE_GROUPS))
		on_group_settings_clicked (NULL, NULL);

	return FALSE;
}

gboolean
on_table_popup_menu (GtkTreeView *treeview, GtkWidget *popup)
{
	do_popup_menu (treeview, NULL);
	return TRUE;
}

void
on_popup_add_activate (GtkAction *action, gpointer data)
{
	gint table = GPOINTER_TO_INT (data);

	if (table == TABLE_GROUPS)
		on_group_new_clicked (NULL, NULL);
	else if (table == TABLE_USERS)
		on_user_new (NULL, NULL);
}

void
on_popup_settings_activate (GtkAction *action, gpointer data)
{
	gint table = GPOINTER_TO_INT (data);

	if (table == TABLE_GROUPS)
		on_group_settings_clicked (NULL, NULL);
}

void
on_popup_delete_activate (GtkAction *action, gpointer data)
{
	gint table = GPOINTER_TO_INT (data);

	if (table == TABLE_GROUPS)
		on_group_delete_clicked (NULL, NULL);
	else if (table == TABLE_USERS)
		on_user_delete_clicked (NULL, NULL);
}

void
on_manage_groups_clicked (GtkWidget *widget, gpointer user_data)
{
	GtkWidget *dialog;

	dialog = gst_dialog_get_widget (tool->main_dialog, "groups_dialog");

	/* Force reloading configuration. This is needed when an user was just
	 * created/deleted, in which case groups may have changed. */
	oobs_object_update (GST_USERS_TOOL (tool)->groups_config);
	gst_users_tool_update_gui (tool);

	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (tool->main_dialog));
	while (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_HELP);
	gtk_widget_hide (dialog);
}

/* Groups tab */

void
on_group_new_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *dialog;
	GtkWidget *parent_dialog;
	OobsGroupsConfig *config;
	OobsGroup *group;
	OobsResult result;
	gint response;

	/* Before going further, check for authorizations, authenticating if needed */
	if (!gst_tool_authenticate (tool, GST_USERS_TOOL (tool)->groups_config))
		return;

	group = oobs_group_new (NULL);
	parent_dialog = gst_dialog_get_widget (tool->main_dialog, "groups_dialog");
	dialog = group_settings_dialog_new (group);

	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (parent_dialog));
	response = group_settings_dialog_run (dialog, group);

	if (response == GTK_RESPONSE_OK) {
		group = group_settings_dialog_get_group ();
		config = OOBS_GROUPS_CONFIG (GST_USERS_TOOL (tool)->groups_config);

		/* We need to know about this group before adding it, else we won't be aware
		 * that we triggered the commit, and we will show a "Reload config?" dialog. */
		gst_tool_add_configuration_object (tool, OOBS_OBJECT (group), FALSE);

		result = oobs_groups_config_add_group (config, group);

		if (result == OOBS_RESULT_OK)
			groups_table_add_group (group);
		else
			gst_tool_commit_error (tool, result);
	}
}

void
on_group_settings_clicked (GtkButton *button, gpointer user_data)
{
	GtkWidget *table, *dialog, *parent_dialog;
	GtkTreePath *path;
	GtkTreeModel *model;
	GtkTreeIter iter;
	OobsGroup *group;
	gint response;

	table = gst_dialog_get_widget (tool->main_dialog, "groups_table");
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (table));
        gtk_tree_view_get_cursor (GTK_TREE_VIEW (table), &path, NULL);

	if (!path)
		return;

	if (!gtk_tree_model_get_iter (model, &iter, path))
		return;

	gtk_tree_model_get (model, &iter,
			    COL_GROUP_OBJECT, &group,
			    -1);
	
	dialog = group_settings_dialog_new (group);
	parent_dialog = gst_dialog_get_widget (tool->main_dialog, "groups_dialog");

	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (parent_dialog));
	response = group_settings_dialog_run (dialog, group);

	/* This is the only dialog we show without authentication, because it's the only
	 * way to get some informations. Committing without authorization could leave
	 * changes in queue in case of failure, which would get committed with
	 * the first successful commit later! */
	if (response == GTK_RESPONSE_OK
	    && gst_tool_authenticate (tool, OOBS_OBJECT (group))) {
		group_settings_dialog_get_data (group);
		groups_table_set_group (group, &iter);
		gst_tool_commit (tool, OOBS_OBJECT (group));
	}

	g_object_unref (group);
}

void
on_group_delete_clicked (GtkButton *button, gpointer user_data)
{
	GtkTreeModel *model;
	GtkTreePath *path;
	GList *list, *elem;

	list = elem = groups_table_get_row_references ();
	model = groups_table_get_model ();

	/* Before going further, check for authorizations, authenticating if needed */
	if (!gst_tool_authenticate (tool, GST_USERS_TOOL (tool)->groups_config))
		return;

	while (elem) {
		path = gtk_tree_row_reference_get_path (elem->data);
		group_delete (model, path);

		gtk_tree_path_free (path);
		elem = elem->next;
	}

	g_list_foreach (list, (GFunc) gtk_tree_row_reference_free, NULL);
	g_list_free (list);
}

void
on_groups_dialog_show_help (GtkWidget *widget, gpointer data)
{
	GstDialog *dialog = GST_DIALOG (data);
	GstTool *tool = gst_dialog_get_tool (dialog);

	gst_tool_show_help (tool, NULL);
}
