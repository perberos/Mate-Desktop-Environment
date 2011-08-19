/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* privileges-table.c: this file is part of users-admin, a ximian-setup-tool frontend 
 * for user administration.
 * 
 * Copyright (C) 2004 Carlos Garnacho
 * Copyright (C) 2005 Carlos Garnacho, Sivan Greenberg
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
 *          Sivan Greenberg       <sivan@workaround.org>
 */

#include <config.h>

#include <stdlib.h>
#include <glib/gi18n.h>

#include "gst.h"
#include "privileges-table.h"
#include "user-profiles.h"
#include "user-settings.h"

extern GstTool *tool;

enum {
	COL_MEMBER,
	COL_DESCRIPTION,
	COL_GROUP
};

typedef struct _PrivilegeDescription PrivilegeDescription;

struct _PrivilegeDescription {
	const gchar *group;
	const gchar *privilege;
};

/* keep this sorted, or you'll go to hell */
static const PrivilegeDescription descriptions[] = {
	{ "adm", N_("Monitor system logs") },
	{ ADMIN_GROUP, N_("Administer the system") },
	{ "audio", N_("Use audio devices") },
	{ "cdrom", N_("Use CD-ROM drives") },
	{ "cdwrite", N_("Burn CDs / DVDs") },
	{ "dialout", N_("Use modems") },
	{ "dip", N_("Connect to Internet using a modem") },
	{ "fax", N_("Send and receive faxes") },
	{ "floppy", N_("Use floppy drives") },
	{ "fuse", N_("Mount user-space filesystems (FUSE)") },
	{ "lpadmin", N_("Configure printers") },
	{ "netdev", N_("Connect to wireless and ethernet networks") },
	{ "plugdev", N_("Access external storage devices automatically") },
	{ "powerdev", N_("Suspend and hibernate the computer") },
	{ "proc", N_("Access /proc filesystem") },
	{ "scanner", N_("Use scanners") },
	{ "tape", N_("Use tape drives") },
	{ "usb", N_("Use USB devices") },
	{ "vboxusers", N_("Use VirtualBox virtualization solution") },
	{ "video", N_("Use video devices") },
	{ "wheel", N_("Be able to get administrator privileges") },
};

static int
compare_groups (const void *p1, const void *p2)
{
	PrivilegeDescription *desc1 = (PrivilegeDescription *) p1;
	PrivilegeDescription *desc2 = (PrivilegeDescription *) p2;

	return strcmp (desc1->group, desc2->group);
}

static const PrivilegeDescription*
privilege_search (const gchar *group)
{
	PrivilegeDescription p = { 0, };

	if (!group)
		return NULL;

	p.group = group;

	return (const PrivilegeDescription *) bsearch (&p, descriptions, G_N_ELEMENTS (descriptions),
						       sizeof (PrivilegeDescription), compare_groups);
}


static void
on_user_privilege_toggled (GtkCellRendererToggle *cell, gchar *path_str, gpointer data)
{
	GtkTreeModel *model, *child_model;
	GtkTreePath  *path  = gtk_tree_path_new_from_string (path_str);
	GtkTreeIter   iter, child_iter;
	OobsGroup    *group;
	gboolean      value;

	model = (GtkTreeModel*) data;

	if (gtk_tree_model_get_iter (model, &iter, path)) {
		child_model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (model));
		gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (model),
								  &child_iter, &iter);

		gtk_tree_model_get (child_model, &child_iter,
		                    COL_MEMBER, &value,
		                    COL_GROUP, &group, -1);

		/* check that current user should be allowed to lose admin rights,
		 * possibly showing a warning/error dialog */
		if (!value || strcmp (oobs_group_get_name (group), ADMIN_GROUP) != 0
		           || user_settings_check_revoke_admin_rights ())
			gtk_list_store_set (GTK_LIST_STORE (child_model), &child_iter, COL_MEMBER, !value, -1);

		g_object_unref (group);
	}

	gtk_tree_path_free (path);
}

static gboolean
privileges_table_visible_func (GtkTreeModel *model,
			       GtkTreeIter  *iter,
			       gpointer      data)
{
	gboolean visible;
	gchar *str;

	gtk_tree_model_get (model, iter,
			    COL_DESCRIPTION, &str,
			    -1);
	visible = (str != NULL);
	g_free (str);

	return visible;
}

void
create_user_privileges_table (void)
{
	GtkWidget *list;
	GtkTreeModel *model, *filter_model;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	list = gst_dialog_get_widget (tool->main_dialog, "user_privileges");

	model = GTK_TREE_MODEL (gtk_list_store_new (3, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_OBJECT));
	filter_model = gtk_tree_model_filter_new (model, NULL);
	gtk_tree_view_set_model (GTK_TREE_VIEW (list), filter_model);
	g_object_unref (filter_model);
	g_object_unref (model);

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (model), COL_DESCRIPTION, GTK_SORT_ASCENDING);
	gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (filter_model),
						privileges_table_visible_func,
						NULL, NULL);

	column = gtk_tree_view_column_new ();

	renderer = gtk_cell_renderer_toggle_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column,
					     renderer,
					     "active", COL_MEMBER,
					     NULL);
	g_signal_connect (G_OBJECT (renderer), "toggled",
			  G_CALLBACK (on_user_privilege_toggled), filter_model);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_end (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column,
					     renderer,
					     "text", COL_DESCRIPTION,
					     NULL);

	gtk_tree_view_column_set_sort_column_id (column, 1);
	gtk_tree_view_insert_column (GTK_TREE_VIEW (list), column, 0);
}

void
privileges_table_add_group (OobsGroup *group)
{
	const PrivilegeDescription *p;
	GtkWidget *table;
	GtkTreeModel *model;
	GtkListStore *store;
	GtkTreeIter iter;

	p = privilege_search (oobs_group_get_name (group));
	if (p == NULL) /* Ignore groups that don't match a privilege */
		return;

	table = gst_dialog_get_widget (tool->main_dialog, "user_privileges");
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (table));
	store = GTK_LIST_STORE (gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (model)));

	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter,
			    COL_MEMBER, FALSE,
			    COL_DESCRIPTION, (p) ? _(p->privilege) : NULL,
			    COL_GROUP, group,
			    -1);
}

void
privileges_table_set_from_user (OobsUser *user)
{
	GtkWidget *table;
	GtkTreeModel *model, *child_model;
	GtkTreeIter iter;
	gboolean valid;
	OobsGroup *group;
	GList *users;

	table = gst_dialog_get_widget (tool->main_dialog, "user_privileges");
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (table));
	child_model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (model));
	valid = gtk_tree_model_get_iter_first (child_model, &iter);

	while (valid) {
		gtk_tree_model_get (child_model, &iter,
				    COL_GROUP, &group,
				    -1);

		users = oobs_group_get_users (group);
		gtk_list_store_set (GTK_LIST_STORE (child_model), &iter,
				    COL_MEMBER, (g_list_find (users, user) != NULL),
				    -1);
		g_list_free (users);
		g_object_unref (group);
		valid = gtk_tree_model_iter_next (child_model, &iter);
	}
}

static gboolean
find_group_in_profile (OobsGroup      *group,
		       GstUserProfile *profile)
{
	gchar **groups;
	const gchar *name;

	if (!profile->groups)
		return FALSE;

	groups = profile->groups;
	name = oobs_group_get_name (group);

	while (*groups) {
		if (strcmp (*groups, name) == 0)
			return TRUE;

		groups++;
	}

	return FALSE;
}

void
privileges_table_set_from_profile (GstUserProfile *profile)
{
	GtkWidget *table;
	GtkTreeModel *model, *child_model;
	GtkTreeIter iter;
	gboolean valid;
	OobsGroup *group;

	table = gst_dialog_get_widget (tool->main_dialog, "user_privileges");
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (table));
	child_model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (model));
	valid = gtk_tree_model_get_iter_first (child_model, &iter);

	while (valid) {
		gtk_tree_model_get (child_model, &iter,
				    COL_GROUP, &group,
				    -1);

		gtk_list_store_set (GTK_LIST_STORE (child_model), &iter,
				    COL_MEMBER, find_group_in_profile (group, profile),
				    -1);

		g_object_unref (group);
		valid = gtk_tree_model_iter_next (child_model, &iter);
	}
}

void
privileges_table_save (OobsUser *user)
{
	GtkWidget *table;
	GtkTreeModel *model, *child_model;
	GtkTreeIter iter;
	OobsGroup *group;
	gboolean valid, member;

	table = gst_dialog_get_widget (tool->main_dialog, "user_privileges");
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (table));
	child_model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (model));
	valid = gtk_tree_model_get_iter_first (child_model, &iter);

	while (valid) {
		gtk_tree_model_get (child_model, &iter,
				    COL_GROUP, &group,
				    COL_MEMBER, &member,
				    -1);
		if (member)
			oobs_group_add_user (group, user);
		else
			oobs_group_remove_user (group, user);

		valid = gtk_tree_model_iter_next (child_model, &iter);
	}
}

void
privileges_table_clear (void)
{
	GtkWidget *table;
	GtkTreeModel *model, *child_model;

	table = gst_dialog_get_widget (tool->main_dialog, "user_privileges");
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (table));
	child_model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (model));

	gtk_list_store_clear (GTK_LIST_STORE (child_model));
}
