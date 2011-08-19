/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* main.c: this file is part of shares-admin, a mate-system-tool frontend 
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

#include "shares-tool.h"
#include "gst.h"
#include "table.h"
#include "nfs-acl-table.h"
#include "share-settings.h"
#include "share-nfs-add-hosts.h"
#include "callbacks.h"

GstTool *tool;

static GstDialogSignal signals [] = {
	/* Main dialog */
	{ "add_share",             "clicked",         G_CALLBACK (on_add_share_clicked) },
	{ "edit_share",            "clicked",         G_CALLBACK (on_edit_share_clicked) },
	{ "delete_share",          "clicked",         G_CALLBACK (on_delete_share_clicked) },
	{ "smb_is_wins",           "toggled",         G_CALLBACK (on_is_wins_toggled) },
	{ "smb_workgroup",         "focus-out-event", G_CALLBACK (on_workgroup_focus_out) },
	{ "smb_wins_server",       "focus-out-event", G_CALLBACK (on_wins_server_focus_out) },
	/* Shares dialog */
	{ "share_type",            "changed",         G_CALLBACK (on_share_type_changed) },
	{ "share_type",            "changed",         G_CALLBACK (on_dialog_validate) },
	{ "share_nfs_delete",      "clicked",         G_CALLBACK (on_share_nfs_delete_clicked) },
	{ "share_nfs_add",         "clicked",         G_CALLBACK (on_share_nfs_add_clicked) },
	{ "share_smb_name",        "changed",         G_CALLBACK (on_share_smb_name_modified) },
	{ "share_smb_name",        "changed",         G_CALLBACK (on_dialog_validate) },
	{ "share_path",            "current-folder-changed", G_CALLBACK (on_shared_folder_changed) },
	/* NFS add hosts dialog */
	{ "share_nfs_host_type",   "changed",         G_CALLBACK (on_share_nfs_host_type_changed) },
	{ NULL }
};

static const gchar *policy_widgets [] = {
	"add_share",
	"delete_share",
	"smb_workgroup",
	"smb_is_wins",
	"smb_wins_server",
	"share_path",
	"share_type",
	"share_smb_name",
	"share_smb_comment",
	"share_smb_readonly",
	"share_nfs_acl",
	"share_nfs_add",
	"share_nfs_delete",
	"share_properties_ok",
	"users_table",
	NULL
};

void
initialize_tables (GstTool *tool)
{
	table_create (tool);
	nfs_acl_table_create ();
	share_nfs_add_hosts_dialog_setup ();
	share_settings_create_combo ();
}

void
initialize_filters (void)
{
	gst_filter_init (GTK_ENTRY (gst_dialog_get_widget (tool->main_dialog, "share_nfs_address")), GST_FILTER_IPV4);
	gst_filter_init (GTK_ENTRY (gst_dialog_get_widget (tool->main_dialog, "share_nfs_network")), GST_FILTER_IPV4);
	gst_filter_init (GTK_ENTRY (gst_dialog_get_widget (tool->main_dialog, "share_nfs_netmask")), GST_FILTER_IPV4);
}

int
main (int argc, char *argv[])
{
	gchar *path = NULL;

	GOptionEntry entries[] = {
		{ "add-share", 'a', 0, G_OPTION_ARG_STRING, &path, N_("Add a shared path, modifies it if it already exists"), N_("PATH") },
		{ NULL }
	};

	g_thread_init (NULL);
	gst_init_tool ("shares-admin", argc, argv, entries);
	tool = GST_TOOL (gst_shares_tool_new ());

	initialize_tables (tool);
	gst_dialog_require_authentication_for_widgets (tool->main_dialog, policy_widgets);
	gst_dialog_connect_signals (tool->main_dialog, signals);
	initialize_filters ();

	if (path)
		GST_SHARES_TOOL (tool)->path = path;
	else
		gtk_widget_show (GTK_WIDGET (tool->main_dialog));

	gtk_main ();

	return 0;
}
