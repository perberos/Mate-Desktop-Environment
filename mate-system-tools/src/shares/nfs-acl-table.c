/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* nfs-acl-table.c: this file is part of shares-admin, a mate-system-tool frontend 
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

#include "nfs-acl-table.h"
#include "gst.h"

extern GstTool *tool;


static GtkTreeModel*
create_nfs_acl_table_model ()
{
	GtkListStore *store;

	store = gtk_list_store_new (NFS_COL_LAST,
				    G_TYPE_STRING,
				    G_TYPE_BOOLEAN);
	return GTK_TREE_MODEL (store);
}

static void
add_nfs_acl_table_columns (GtkTreeView *table)
{
	GtkCellRenderer *renderer;

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (table), 1,
						     _("Allowed host/network"),
						     renderer,
						     "text", 0,
						     NULL);
	renderer = gtk_cell_renderer_toggle_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (table), 1,
						     _("Read only"),
						     renderer,
						     "active", 1,
						     NULL);
}

void
nfs_acl_table_create (void)
{
	GtkWidget *table = gst_dialog_get_widget (tool->main_dialog, "share_nfs_acl");
	GtkTreeModel *model;

	model = create_nfs_acl_table_model ();
	gtk_tree_view_set_model (GTK_TREE_VIEW (table), model);
	g_object_unref (G_OBJECT (model));

	add_nfs_acl_table_columns (GTK_TREE_VIEW (table));
}

void
nfs_acl_table_add_element (OobsShareAclElement *element)
{
	GtkWidget    *table = gst_dialog_get_widget (tool->main_dialog, "share_nfs_acl");
	GtkTreeModel *model;
	GtkTreeIter   iter;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (table));

	gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter,
			    0, element->element,
			    1, element->read_only,
			    -1);
}

void
nfs_acl_table_insert_elements (OobsShareNFS *share)
{
	GtkWidget    *table = gst_dialog_get_widget (tool->main_dialog, "share_nfs_acl");
	GtkTreeModel *model;
	GtkTreeIter   iter;
	gboolean      valid, read_only;
	gchar        *element;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (table));
	valid = gtk_tree_model_get_iter_first (model, &iter);

	while (valid) {
		gtk_tree_model_get (model, &iter,
				    0, &element,
				    1, &read_only,
				    -1);

		oobs_share_nfs_add_acl_element (share, element, read_only);
		g_free (element);

		valid = gtk_tree_model_iter_next (model, &iter);
	}
}
