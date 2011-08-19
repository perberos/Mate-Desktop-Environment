/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* callbacks.c: this file is part of shares-admin, a mate-system-tool frontend 
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <oobs/oobs.h>

#include "shares-tool.h"
#include "gst.h"
#include "table.h"
#include "callbacks.h"
#include "share-settings.h"
#include "share-nfs-add-hosts.h"

extern GstTool *tool;

void
on_shares_table_selection_changed (GtkTreeSelection *selection, gpointer data)
{
	GtkWidget *edit_share, *delete_share;
	gboolean   active;

	edit_share = gst_dialog_get_widget (tool->main_dialog, "edit_share");
	delete_share = gst_dialog_get_widget (tool->main_dialog, "delete_share");

	active = (gtk_tree_selection_count_selected_rows (selection) > 0);

	gst_dialog_try_set_sensitive (tool->main_dialog, edit_share, active);
	gst_dialog_try_set_sensitive (tool->main_dialog, delete_share, active);
}

static void
do_popup_menu (GtkWidget *popup, GdkEventButton *event)
{
	gint button, event_time;

	if (!popup)
		return;

	if (event) {
		button     = event->button;
		event_time = event->time;
	} else {
		button     = 0;
		event_time = gtk_get_current_event_time ();
	}

	gtk_menu_popup (GTK_MENU (popup), NULL, NULL, NULL, NULL,
			button, event_time);
}

gboolean
on_shares_table_button_press (GtkWidget *table, GdkEventButton *event, gpointer data)
{
	GtkTreePath      *path;
	GtkWidget        *popup;
	GtkTreeSelection *selection;

	popup = (GtkWidget *) data;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (table));

	if (event->button == 3) {
		gtk_widget_grab_focus (table);

		if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (table),
						   event->x, event->y,
						   &path, NULL, NULL, NULL)) {
			gtk_tree_selection_unselect_all (selection);
			gtk_tree_selection_select_path (selection, path);

			do_popup_menu (popup, event);
		}

		return TRUE;
	}

	if (event->type == GDK_2BUTTON_PRESS || event->type == GDK_3BUTTON_PRESS) {
		on_edit_share_clicked (NULL, NULL);
		return TRUE;
	}

	return FALSE;
}

gboolean
on_shares_table_popup_menu (GtkWidget *widget, GtkWidget *popup)
{
	do_popup_menu (popup, NULL);
	return TRUE;
}

void
on_shares_dragged_folder (GtkWidget *widget, GdkDragContext *context,
			  gint x, gint y, GtkSelectionData *selection_data,
			  guint info, guint time, gpointer gdata)
{
	gchar *path, **uris = NULL;
	gint i = 0;

	if (info == SHARES_DND_URI_LIST) {
		uris = g_uri_list_extract_uris((gchar *) gtk_selection_data_get_text(selection_data));

		if (uris != NULL) {
		    while ( uris[i] != NULL) {
			path = g_strdup (g_filename_from_uri(uris[i++], NULL, NULL));

			if (g_file_test (path, G_FILE_TEST_IS_DIR)) {
				share_settings_dialog_run (path, FALSE);
				gtk_drag_finish (context, TRUE, FALSE, time);
			} else {
				gtk_drag_finish (context, FALSE, FALSE, time);
			}

			g_free (path);
		    }	
		}

		g_strfreev(uris);
	}
}

void
on_add_share_clicked (GtkWidget *widget, gpointer data)
{
	share_settings_dialog_run (NULL, FALSE);
}

void
on_edit_share_clicked (GtkWidget *widget, gpointer data)
{
	GtkWidget        *table;
	GtkTreeSelection *selection;
	GtkTreeModel     *model;
	GtkTreeIter       iter;
	gchar            *path;

	table = gst_dialog_get_widget (tool->main_dialog, "shares_table");
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (table));

	if (!gtk_tree_selection_get_selected (selection, &model, &iter))
		return;

	gtk_tree_model_get (model, &iter, COL_PATH, &path, -1);

	share_settings_dialog_run_for_iter (path, &iter, FALSE);
	g_free (path);
}

void
on_delete_share_clicked (GtkWidget *widget, gpointer data)
{
	GtkWidget        *table, *dialog;
	GtkTreeSelection *selection;
	GtkTreeModel     *model;
	GtkTreeIter       iter;
	gint              response;
	OobsList         *list;
	OobsListIter     *list_iter;
	OobsShare        *share;

	table = gst_dialog_get_widget (tool->main_dialog, "shares_table");
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (table));

	if (!gtk_tree_selection_get_selected (selection, &model, &iter))
		return;

	dialog = gtk_message_dialog_new (GTK_WINDOW (tool->main_dialog),
					 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_WARNING,
					 GTK_BUTTONS_NONE,
					 _("Are you sure you want to delete this share?"));
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
						  _("Other computers in your network will stop viewing this."));
	gtk_dialog_add_buttons (GTK_DIALOG (dialog),
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_DELETE, GTK_RESPONSE_ACCEPT,
				NULL);

	gst_dialog_add_edit_dialog (tool->main_dialog, dialog);

	response = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

	gst_dialog_remove_edit_dialog (tool->main_dialog, dialog);

	if (response == GTK_RESPONSE_ACCEPT) {
		gtk_tree_model_get (model, &iter,
				    COL_SHARE, &share,
				    COL_ITER, &list_iter,
				    -1);

		if (OOBS_IS_SHARE_NFS (share)) {
			list = oobs_nfs_config_get_shares OOBS_NFS_CONFIG ((GST_SHARES_TOOL (tool)->nfs_config));
			oobs_list_remove (list, list_iter);
			gst_tool_commit (tool, GST_SHARES_TOOL (tool)->nfs_config);
		} else {
			list = oobs_smb_config_get_shares OOBS_SMB_CONFIG ((GST_SHARES_TOOL (tool)->smb_config));
			oobs_list_remove (list, list_iter);
			gst_tool_commit (tool, GST_SHARES_TOOL (tool)->smb_config);
		}

		gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
		oobs_list_iter_free (list_iter);
		g_object_unref (share);
	}
}

void
on_share_type_changed (GtkWidget *widget, gpointer data)
{
	gint          selected;
	GtkWidget    *smb_frame, *nfs_frame;
	GtkTreeModel *model;
	GtkTreeIter   iter;

	selected  = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
	smb_frame = gst_dialog_get_widget (tool->main_dialog, "smb_properties");
	nfs_frame = gst_dialog_get_widget (tool->main_dialog, "nfs_properties");

	model = gtk_combo_box_get_model (GTK_COMBO_BOX (widget));

	if (!gtk_combo_box_get_active_iter (GTK_COMBO_BOX (widget), &iter))
		return;

	gtk_tree_model_get (model, &iter, 1, &selected, -1);

	if (selected == SHARE_THROUGH_SMB) {
		gtk_widget_show (smb_frame);
		gtk_widget_hide (nfs_frame);
	} else if (selected == SHARE_THROUGH_NFS) {
		gtk_widget_hide (smb_frame);
		gtk_widget_show (nfs_frame);
	} else {
		gtk_widget_hide (smb_frame);
		gtk_widget_hide (nfs_frame);
	}
}

void
on_share_nfs_delete_clicked (GtkWidget *widget, gpointer data)
{
	GtkWidget        *table;
	GtkTreeSelection *selection;
	GtkTreeIter       iter;
	GtkTreeModel     *model;
	
	table = gst_dialog_get_widget (tool->main_dialog, "share_nfs_acl");
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (table));

	if (gtk_tree_selection_get_selected (selection, &model, &iter))
		gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
}

void
on_share_nfs_add_clicked (GtkWidget *widget, gpointer data)
{
	GtkWidget    *parent = gst_dialog_get_widget (tool->main_dialog, "share_properties");
	GtkWidget    *dialog;
	GtkWidget    *table;
	GtkTreeModel *model;
	GtkTreeIter   iter;
	gint          response;
	gchar        *host = NULL;
	gboolean      read_only;

	dialog = share_nfs_add_hosts_dialog_prepare ();

	gst_dialog_add_edit_dialog (tool->main_dialog, dialog);

	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (parent));
	response = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_hide (dialog);

	gst_dialog_remove_edit_dialog (tool->main_dialog, dialog);

	if (response == GTK_RESPONSE_OK) {
		if (!share_nfs_add_hosts_dialog_get_data (&host, &read_only))
			return;

		table = gst_dialog_get_widget (tool->main_dialog, "share_nfs_acl");
		model = gtk_tree_view_get_model (GTK_TREE_VIEW (table));

		gtk_list_store_append (GTK_LIST_STORE (model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
				    0, host,
				    1, read_only,
				    -1);
		g_free (host);
	}
}

void
on_share_nfs_host_type_changed (GtkWidget *widget, gpointer data)
{
	GtkTreeModel *model;
	GtkTreeIter   iter;
	gint          selected_type;

	model = gtk_combo_box_get_model (GTK_COMBO_BOX (widget));

	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (widget), &iter)) {
		gtk_tree_model_get (model, &iter,
				    NFS_HOST_COL_TYPE, &selected_type, -1);

		g_object_set (gst_dialog_get_widget (tool->main_dialog, "share_nfs_hostname_box"),
			      "visible", (selected_type == NFS_SHARE_HOSTNAME), NULL);

		g_object_set (gst_dialog_get_widget (tool->main_dialog, "share_nfs_address_box"),
			      "visible", (selected_type == NFS_SHARE_ADDRESS), NULL);

		g_object_set (gst_dialog_get_widget (tool->main_dialog, "share_nfs_network_box"),
			      "visible", (selected_type == NFS_SHARE_NETWORK), NULL);
	}
}

void
on_dialog_validate (GtkWidget *widget, gpointer data)
{
	GtkWidget *ok_button;

	ok_button = gst_dialog_get_widget (tool->main_dialog, "share_properties_ok");
	gst_dialog_try_set_sensitive (tool->main_dialog, ok_button,
				      share_settings_validate ());
}

void
on_is_wins_toggled (GtkWidget *widget, gpointer data)
{
	GtkWidget *label, *entry;
	gboolean is_wins_server;

	is_wins_server = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

	label = gst_dialog_get_widget (tool->main_dialog, "smb_wins_server_label");
	entry = gst_dialog_get_widget (tool->main_dialog, "smb_wins_server");

	gtk_widget_set_sensitive (label, !is_wins_server);
	gtk_widget_set_sensitive (entry, !is_wins_server);

	oobs_smb_config_set_is_wins_server (OOBS_SMB_CONFIG (GST_SHARES_TOOL (tool)->smb_config), is_wins_server);
	gst_tool_commit (tool, GST_SHARES_TOOL (tool)->smb_config);
}

gboolean
on_workgroup_focus_out (GtkWidget *widget, GdkEvent *event, gpointer data)
{
	gchar *workgroup;

	/* FIXME: check that it has actually changed */
	workgroup = g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));

	if (workgroup && *workgroup) {
		oobs_smb_config_set_workgroup (OOBS_SMB_CONFIG (GST_SHARES_TOOL (tool)->smb_config), workgroup);
		gst_tool_commit (tool, GST_SHARES_TOOL (tool)->smb_config);
	}
	g_free (workgroup);

	return FALSE;
}

gboolean
on_wins_server_focus_out (GtkWidget *widget, GdkEvent *event, gpointer data)
{
	gchar *wins_server;

	/* FIXME: check that it has actually changed */
	wins_server = g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));

	oobs_smb_config_set_wins_server (OOBS_SMB_CONFIG (GST_SHARES_TOOL (tool)->smb_config), wins_server);
	gst_tool_commit (tool, GST_SHARES_TOOL (tool)->smb_config);
	g_free (wins_server);

	return FALSE;
}

void
on_shared_folder_changed (GtkWidget *widget, gpointer data)
{
	gchar *path;

	path = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (widget));
	share_settings_set_name_from_folder (path);
	g_free (path);
}

void
on_share_smb_name_modified (GtkWidget *widget, gpointer data)
{
	g_object_set_data (G_OBJECT (widget), "modified", GINT_TO_POINTER (TRUE));
}
