/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* share-settings.h: this file is part of shares-admin, a mate-system-tool frontend 
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

#include "callbacks.h"
#include "shares-tool.h"
#include "nfs-acl-table.h"
#include "share-settings.h"
#include "table.h"
#include "gst.h"

extern GstTool *tool;

static void
set_smb_name (const gchar *name)
{
	GtkWidget *entry;
	gboolean modified;

	entry = gst_dialog_get_widget (tool->main_dialog, "share_smb_name");

	modified = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (entry), "modified"));

	if (!modified) {
		g_signal_handlers_block_by_func (entry, on_share_smb_name_modified, tool->main_dialog);
		gtk_entry_set_text (GTK_ENTRY (entry), name);
		g_signal_handlers_unblock_by_func (entry, on_share_smb_name_modified, tool->main_dialog);
	}
}

void
share_settings_set_name_from_folder (const gchar *path)
{
	gchar *name;

	g_return_if_fail (path != NULL);

	if (strcmp (path, "/") == 0)
		name = g_strdup (_("File System"));
	else
		name = g_path_get_basename (path);

	set_smb_name (name);
	g_free (name);
}

static void
share_settings_clear_dialog (void)
{
	GtkWidget *widget;
	GtkTreeModel *model;

	/* SMB widgets */
	widget = gst_dialog_get_widget (tool->main_dialog, "share_smb_name");
	gtk_entry_set_text (GTK_ENTRY (widget), "");

	widget = gst_dialog_get_widget (tool->main_dialog, "share_smb_comment");
	gtk_entry_set_text (GTK_ENTRY (widget), "");

        widget = gst_dialog_get_widget (tool->main_dialog, "share_smb_readonly");
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);

	/* NFS widgets */
	widget = gst_dialog_get_widget (tool->main_dialog, "share_nfs_acl");
	model  = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
	gtk_list_store_clear (GTK_LIST_STORE (model));
}

GtkTreeModel*
share_settings_get_combo_model (void)
{
	GtkListStore *store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_INT);

	return GTK_TREE_MODEL (store);
}

void
share_settings_create_combo (void)
{
	GtkWidget    *combo  = gst_dialog_get_widget (tool->main_dialog, "share_type");
	GtkTreeModel *model;

	model = share_settings_get_combo_model ();
	gtk_combo_box_set_model (GTK_COMBO_BOX (combo), model);
	g_object_unref (model);
}

static void
share_settings_set_path (const gchar *path)
{
	GtkWidget *path_entry = gst_dialog_get_widget (tool->main_dialog, "share_path");
	GtkWidget *path_label = gst_dialog_get_widget (tool->main_dialog, "share_path_label");

	if (!path) {
		gtk_widget_show (path_entry);
		gtk_widget_hide (path_label);
	} else {
		gtk_widget_hide (path_entry);
		gtk_widget_show (path_label);
		gtk_label_set_text (GTK_LABEL (path_label), path);
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (path_entry), path);
	}
}

static GtkWidget*
share_settings_prepare_dialog (const gchar *path, gboolean standalone)
{
	GtkWidget *combo = gst_dialog_get_widget (tool->main_dialog, "share_type");
	GtkWidget *dialog = gst_dialog_get_widget (tool->main_dialog, "share_properties");
	GtkTreeModel *model;
	GtkTreeIter iter;

	share_settings_clear_dialog ();
	share_settings_set_path (path);

	model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
	gtk_list_store_clear (GTK_LIST_STORE (model));

	if (standalone) {
		gtk_list_store_append (GTK_LIST_STORE (model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
				    0, _("Do not share"),
				    1, SHARE_DONT_SHARE,
				    -1);
	}

	if (GST_SHARES_TOOL (tool)->smb_available) {
		gtk_list_store_append (GTK_LIST_STORE (model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
				    0, _("Windows networks (SMB)"),
				    1, SHARE_THROUGH_SMB,
				    -1);
	}

	if (GST_SHARES_TOOL (tool)->nfs_available) {
		gtk_list_store_append (GTK_LIST_STORE (model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
				    0, _("Unix networks (NFS)"),
				    1, SHARE_THROUGH_NFS,
				    -1);
	}

	gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);

	return dialog;
}

static void
share_settings_set_share_smb (OobsShareSMB *share)
{
	GtkWidget *widget;
	gint       flags;

	widget = gst_dialog_get_widget (tool->main_dialog, "share_smb_name");
	gtk_entry_set_text (GTK_ENTRY (widget), oobs_share_smb_get_name (share));
	
	widget  = gst_dialog_get_widget (tool->main_dialog, "share_path");
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (widget),
					     oobs_share_get_path (OOBS_SHARE (share)));

	widget  = gst_dialog_get_widget (tool->main_dialog, "share_smb_comment");
	gtk_entry_set_text (GTK_ENTRY (widget),
			    oobs_share_smb_get_comment (share));

	flags = oobs_share_smb_get_flags (share);

	widget = gst_dialog_get_widget (tool->main_dialog, "share_smb_readonly");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
				      ! (flags & OOBS_SHARE_SMB_WRITABLE));
}

static void
share_settings_set_share_nfs (OobsShareNFS *share)
{
	GtkWidget    *widget;
	const GSList *list;

	widget  = gst_dialog_get_widget (tool->main_dialog, "share_path");
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (widget),
					     oobs_share_get_path (OOBS_SHARE (share)));

	list = oobs_share_nfs_get_acl (share);

	while (list) {
		nfs_acl_table_add_element (list->data);
		list = g_slist_next (list);
	}
}

static void
share_settings_set_active (gint val)
{
	GtkWidget    *combo = gst_dialog_get_widget (tool->main_dialog, "share_type");
	GtkTreeModel *model;
	GtkTreeIter   iter;
	gboolean      valid;
	gint          value;

	model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
	valid = gtk_tree_model_get_iter_first (model, &iter);

	while (valid) {
		gtk_tree_model_get (model, &iter, 1, &value, -1);

		if (value == val) {
			gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &iter);
			valid = FALSE;
		} else
			valid = gtk_tree_model_iter_next (model, &iter);
	}
}

static void
share_settings_set_share (OobsShare *share)
{
	if (OOBS_IS_SHARE_SMB (share)) {
		share_settings_set_active (SHARE_THROUGH_SMB);
		share_settings_set_share_smb (OOBS_SHARE_SMB (share));
	} else if (OOBS_IS_SHARE_NFS (share)) {
		share_settings_set_active (SHARE_THROUGH_NFS);
		share_settings_set_share_nfs (OOBS_SHARE_NFS (share));
	}
}

static void
share_settings_close_dialog (void)
{
	GtkWidget *dialog = gst_dialog_get_widget (tool->main_dialog, "share_properties");

	gtk_widget_hide (dialog);
}

static OobsShare*
share_settings_get_share_smb (void)
{
	GtkWidget   *widget;
	const gchar *path, *name, *comment;
	gint         flags = 0;

	widget = gst_dialog_get_widget (tool->main_dialog, "share_path");
	path = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (widget));

	widget = gst_dialog_get_widget (tool->main_dialog, "share_smb_name");
	name = gtk_entry_get_text (GTK_ENTRY (widget));
	
	widget = gst_dialog_get_widget (tool->main_dialog, "share_smb_comment");
	comment = gtk_entry_get_text (GTK_ENTRY (widget));

	widget = gst_dialog_get_widget (tool->main_dialog, "share_smb_readonly");
	if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
		flags |= OOBS_SHARE_SMB_WRITABLE;

	flags |= (OOBS_SHARE_SMB_ENABLED |
		  OOBS_SHARE_SMB_PUBLIC |
		  OOBS_SHARE_SMB_BROWSABLE);

	return OOBS_SHARE (oobs_share_smb_new (path, name, comment, flags));
}

static OobsShare*
share_settings_get_share_nfs ()
{
	GtkWidget    *widget;
	const gchar  *path;
	OobsShareNFS *share;

	widget  = gst_dialog_get_widget (tool->main_dialog, "share_path");
	path    = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (widget));

	share = OOBS_SHARE_NFS (oobs_share_nfs_new (path));
	nfs_acl_table_insert_elements (share);

	return OOBS_SHARE (share);
}

static OobsShare*
share_settings_get_share (void)
{
	GtkWidget    *combo = gst_dialog_get_widget (tool->main_dialog, "share_type");
	GtkTreeModel *model;
	GtkTreeIter   iter;
	gint          selected;

	model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

	if (!gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter))
		return NULL;

	gtk_tree_model_get (model, &iter, 1, &selected, -1);

	if (selected == SHARE_THROUGH_SMB)
		return share_settings_get_share_smb ();
	else if (selected == SHARE_THROUGH_NFS)
		return share_settings_get_share_nfs ();
	else
		return NULL;
}

gboolean
share_settings_validate (void)
{
	GtkWidget    *combo = gst_dialog_get_widget (tool->main_dialog, "share_type");
	GtkWidget    *widget;
	GtkTreeModel *model;
	GtkTreeIter   iter;
	gint          selected;
	const gchar  *text;

	model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

	if (!gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo), &iter))
		return FALSE;

	gtk_tree_model_get (model, &iter, 1, &selected, -1);

	if (selected == SHARE_THROUGH_SMB) {
		widget = gst_dialog_get_widget (tool->main_dialog, "share_smb_name");
		text = gtk_entry_get_text (GTK_ENTRY (widget));

		return (text && *text);
	}

	/* in any other case, it's valid */
	return TRUE;
}

static void
modify_share (OobsShare *new_share, OobsShare *old_share, OobsListIter *list_iter, GtkTreeIter *iter)
{
	OobsObject *old_config, *new_config;
	OobsList *old_list, *new_list;

	if (OOBS_IS_SHARE_SMB (old_share)) {
		old_config = GST_SHARES_TOOL (tool)->smb_config;
		old_list = oobs_smb_config_get_shares (OOBS_SMB_CONFIG (old_config));
	} else {
		old_config = GST_SHARES_TOOL (tool)->nfs_config;
		old_list = oobs_nfs_config_get_shares (OOBS_NFS_CONFIG (old_config));
	}

	oobs_list_remove (old_list, list_iter);

	/* FIXME: this part is almost the same than add_new_share() */
	if (OOBS_IS_SHARE_SMB (new_share)) {
		new_config = GST_SHARES_TOOL (tool)->smb_config;
		new_list = oobs_smb_config_get_shares (OOBS_SMB_CONFIG (new_config));
	} else {
		new_config = GST_SHARES_TOOL (tool)->nfs_config;
		new_list = oobs_nfs_config_get_shares (OOBS_NFS_CONFIG (new_config));
	}

	oobs_list_append (new_list, list_iter);
	oobs_list_set (new_list, list_iter, new_share);
	g_object_unref (new_share);

	table_modify_share_at_iter (iter, new_share, list_iter);

	gst_tool_commit (tool, new_config);

	if (old_config != new_config)
		gst_tool_commit (tool, old_config);
}

static void
add_new_share (OobsShare *share)
{
	OobsObject *config;
	OobsList *list;
	OobsListIter iter;

	if (OOBS_IS_SHARE_SMB (share)) {
		config = GST_SHARES_TOOL (tool)->smb_config;
		list = oobs_smb_config_get_shares (OOBS_SMB_CONFIG (config));
	} else {
		config = GST_SHARES_TOOL (tool)->nfs_config;
		list = oobs_nfs_config_get_shares (OOBS_NFS_CONFIG (config));
	}

	oobs_list_append (list, &iter);
	oobs_list_set (list, &iter, share);
	g_object_unref (share);

	table_add_share (share, &iter);

	gst_tool_commit (tool, config);
}

static void
delete_share (GtkTreeIter *iter, OobsShare *share, OobsListIter *list_iter)
{
	OobsObject *config;
	OobsList *list;

	if (OOBS_IS_SHARE_SMB (share)) {
		config = GST_SHARES_TOOL (tool)->smb_config;
		list = oobs_smb_config_get_shares (OOBS_SMB_CONFIG (config));
	} else {
		config = GST_SHARES_TOOL (tool)->nfs_config;
		list = oobs_nfs_config_get_shares (OOBS_NFS_CONFIG (config));
	}

	oobs_list_remove (list, list_iter);
	table_delete_share_at_iter (iter);

	gst_tool_commit (tool, config);
}

void
share_settings_dialog_run (const gchar *path, gboolean standalone)
{
	GtkTreeIter   iter;

	/* check whether the path already exists */
	if (table_get_iter_with_path (path, &iter))
		share_settings_dialog_run_for_iter (path, &iter, standalone);
	else
		share_settings_dialog_run_for_iter (path, NULL, standalone);
}

void
share_settings_dialog_run_for_iter (const gchar *path,
				    GtkTreeIter *iter,
				    gboolean standalone)
{
	GtkWidget    *dialog;
	gint          response;
	OobsShare    *new_share, *share;
	OobsListIter *list_iter;
	gchar        *title;
	GtkWidget    *name_entry, *file_chooser;

	share  = NULL;
	list_iter = NULL;
	dialog = share_settings_prepare_dialog (path, standalone);

	if (iter) {
		share = table_get_share_at_iter (iter, &list_iter);
		share_settings_set_share (share);

		title = g_strdup_printf (_("Settings for folder '%s'"),
					 oobs_share_get_path (share));
		gtk_window_set_title (GTK_WINDOW (dialog), title);
		g_free (title);
	} else {
		name_entry = gst_dialog_get_widget (tool->main_dialog, "share_smb_name");
		g_object_set_data (G_OBJECT (name_entry), "modified", GINT_TO_POINTER (FALSE));
		gtk_window_set_title (GTK_WINDOW (dialog), _("Share Folder"));

		if (!path) {
			/* make sure the path entry gets filled in */
			file_chooser = gst_dialog_get_widget (tool->main_dialog, "share_path");
			path = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (file_chooser));
		}

		share_settings_set_name_from_folder (path);
	}

	gst_dialog_add_edit_dialog (tool->main_dialog, dialog);

	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (tool->main_dialog));
	response = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_hide (dialog);

	gst_dialog_remove_edit_dialog (tool->main_dialog, dialog);

	if (response == GTK_RESPONSE_OK) {
		new_share = share_settings_get_share ();

		if (new_share) {
			/* FIXME: check type, and move
			 * share between lists if necessary */
			if (iter)
				modify_share (new_share, share, list_iter, iter);
			else
				add_new_share (new_share);
		} else {
			if (iter)
				delete_share (iter, share, list_iter);
		}
	}

	share_settings_close_dialog ();

	if (share)
		g_object_unref (share);

	if (list_iter)
		oobs_list_iter_free (list_iter);

	if (standalone)
		gtk_main_quit ();
}
