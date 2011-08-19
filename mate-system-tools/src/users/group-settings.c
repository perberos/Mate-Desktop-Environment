/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* group_settings.c: this file is part of users-admin, a ximian-setup-tool frontend 
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
 * Authors: Carlos Garnacho Parro <garparr@teleline.es>.
 */

#include <config.h>
#include "gst.h"
#include <glib/gi18n.h>
#include <oobs/oobs-defines.h>

#include <string.h>
#include <ctype.h>

#include "group-members-table.h"
#include "groups-table.h"
#include "table.h"
#include "callbacks.h"
#include "group-settings.h"
#include "test-battery.h"

extern GstTool *tool;

static gboolean
check_group_delete (OobsGroup *group)
{
	GtkWidget *parent;
	GtkWidget *dialog;
	gint reply;

	parent = gst_dialog_get_widget (tool->main_dialog, "group_settings_dialog");

	if (oobs_group_get_gid (group) == 0) {
		dialog = gtk_message_dialog_new (GTK_WINDOW (parent),
						 GTK_DIALOG_MODAL,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_CLOSE,
						 _("Administrator group can not be deleted"));

		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
							  _("This would leave the system unusable."));
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		return FALSE;
	}

	/* FIXME: should check that any user of this group is logged in */

	dialog = gtk_message_dialog_new (GTK_WINDOW (parent),
					 GTK_DIALOG_MODAL,
					 GTK_MESSAGE_WARNING,
					 GTK_BUTTONS_NONE,
					 _("Are you sure you want to delete group \"%s\"?"),
					 oobs_group_get_name (group));
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
						  _("This may leave files with invalid group ID in the filesystem."));
	gtk_dialog_add_buttons (GTK_DIALOG (dialog),
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_DELETE, GTK_RESPONSE_ACCEPT,
				NULL);

	gst_dialog_add_edit_dialog (tool->main_dialog, dialog);
	reply = gtk_dialog_run (GTK_DIALOG (dialog));
	gst_dialog_remove_edit_dialog (tool->main_dialog, dialog);

	gtk_widget_destroy (dialog);

	return (reply == GTK_RESPONSE_ACCEPT);
}

gboolean
group_delete (GtkTreeModel *model, GtkTreePath *path)
{
	GtkTreeIter iter;
	OobsGroupsConfig *config;
	OobsGroup *group;
	OobsResult result;
	gboolean retval = FALSE;

	if (!gtk_tree_model_get_iter (model, &iter, path))
		return FALSE;

	gtk_tree_model_get (model, &iter,
			    COL_GROUP_OBJECT, &group,
			    -1);

	if (check_group_delete (group)) {
		config = OOBS_GROUPS_CONFIG (GST_USERS_TOOL (tool)->groups_config);
		result = oobs_groups_config_delete_group (config, group);
		if (result == OOBS_RESULT_OK) {
			gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
			retval = TRUE;
		}
		else {
			gst_tool_commit_error (tool, result);
			retval = FALSE;
		}
	}

	g_object_unref (group);

	return retval;

}

GtkWidget*
group_settings_dialog_new (OobsGroup *group)
{
	OobsGroupsConfig *config;
	GtkWidget *dialog, *widget;
	const gchar *name;
	gchar *title;
	gid_t gid;

	config = OOBS_GROUPS_CONFIG (GST_USERS_TOOL (tool)->groups_config);

	dialog = gst_dialog_get_widget (tool->main_dialog, "group_settings_dialog");
	name = oobs_group_get_name (group);

	/* Set this before setting the GID so that it's not rejected */
	widget = gst_dialog_get_widget (tool->main_dialog, "group_settings_gid");
	gtk_spin_button_set_range (GTK_SPIN_BUTTON (widget), 0, OOBS_MAX_GID);

	if (!name) {
		g_object_set_data (G_OBJECT (dialog), "is_new", GINT_TO_POINTER (TRUE));
		gtk_window_set_title (GTK_WINDOW (dialog), _("New group"));

		widget = gst_dialog_get_widget (tool->main_dialog, "group_settings_gid");
		gid = oobs_groups_config_find_free_gid (config, 0, 0);
	} else {
		g_object_set_data (G_OBJECT (dialog), "is_new", GINT_TO_POINTER (FALSE));

		title = g_strdup_printf (_("Group '%s' Properties"), name);
		gtk_window_set_title (GTK_WINDOW (dialog), title);
		g_free (title);

		widget = gst_dialog_get_widget (tool->main_dialog, "group_settings_gid");
		gid = oobs_group_get_gid (group);
	}

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), gid);

	widget = gst_dialog_get_widget (tool->main_dialog, "group_settings_name");
	gtk_entry_set_text (GTK_ENTRY (widget), (name) ? name : "");
	gtk_widget_set_sensitive (widget, (name == NULL));

	group_members_table_set_from_group (group);

	return dialog;
}

gboolean
group_settings_dialog_group_is_new (void)
{
	GtkWidget *dialog;
	gboolean is_new;

	dialog = gst_dialog_get_widget (tool->main_dialog, "group_settings_dialog");
	is_new = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (dialog), "is_new"));

	return is_new;
}

/* FIXME: this function is duplicated in user-settings.c */
static gboolean
is_valid_name (const gchar *name)
{
	/*
	 * User/group names must start with a letter, and may not
	 * contain colons, commas, newlines (used in passwd/group
	 * files...) or any non-printable characters.
	 */
        if (!*name || !isalpha(*name))
                return FALSE;

        while (*name) {
		if (!isdigit (*name) && !islower (*name) && *name != '-')
                        return FALSE;
                name++;
        }

        return TRUE;
}

static void
check_name (gchar **primary_text, gchar **secondary_text, gpointer data)
{
	OobsGroupsConfig *config;
	OobsGroup *group = OOBS_GROUP (data);
	GtkWidget *widget;
	const gchar *name;

	config = OOBS_GROUPS_CONFIG (GST_USERS_TOOL (tool)->groups_config);

	widget = gst_dialog_get_widget (tool->main_dialog, "group_settings_name");
	name = gtk_entry_get_text (GTK_ENTRY (widget));

	if (strlen (name) < 1) {
		*primary_text = g_strdup (_("Group name is empty"));
		*secondary_text = g_strdup (_("A group name must be specified."));
	} else if (oobs_group_is_root (group) && strcmp (name, "root") != 0) {
		*primary_text = g_strdup (_("Group name of the administrator group user should not be modified"));
		*secondary_text = g_strdup (_("This would leave the system unusable."));
	} else if (!is_valid_name (name)) {
		*primary_text = g_strdup (_("Group name has invalid characters"));
		*secondary_text = g_strdup (_("Please set a valid group name consisting of "
					      "a lower case letter followed by lower case "
					      "letters and numbers."));
	} else if (group_settings_dialog_group_is_new ()
	           && oobs_groups_config_is_name_used (config, name)) {
		*primary_text = g_strdup_printf (_("Group \"%s\" already exists"), name);
		*secondary_text = g_strdup (_("Please choose a different group name."));
	}
}

static void
check_gid (gchar **primary_text, gchar **secondary_text, gpointer data)
{
	OobsGroup *group = OOBS_GROUP (data);
	OobsGroupsConfig *config;
	OobsGroup *gid_group;
	GtkWidget *widget;
	gid_t gid;
	gboolean new;

	widget = gst_dialog_get_widget (tool->main_dialog, "group_settings_gid");
	/* we know the value is positive because the range is limited */
	gid = (unsigned) gtk_spin_button_get_value (GTK_SPIN_BUTTON (widget));
	new = group_settings_dialog_group_is_new ();

	/* don't warn if nothing has changed */
	if (!new && gid == oobs_group_get_gid (group))
		return;

	config = OOBS_GROUPS_CONFIG (GST_USERS_TOOL (tool)->groups_config);
	gid_group = oobs_groups_config_get_from_gid (config, gid);

	if (oobs_group_is_root (group) && gid != 0) {
		*primary_text = g_strdup (_("Group ID of the Administrator account should not be modified"));
		*secondary_text = g_strdup (_("This would leave the system unusable."));
	}
	else if (gid_group) { /* check that GID is free */
		*primary_text   = g_strdup_printf (_("Group ID %d is already used by group \"%s\""),
		                                   gid, oobs_group_get_name (gid_group));
		if (new)
			*secondary_text = g_strdup (_("Please choose a different numeric identifier for the new group."));
		else
			*secondary_text = g_strdup_printf (_("Please choose a different numeric identifier for group \"%s\"."),
			                                   oobs_group_get_name (group));

		g_object_unref (gid_group);
	}
}

gint
group_settings_dialog_run (GtkWidget *dialog, OobsGroup *group)
{
	gint response;
	gboolean valid;

	TestBattery battery[] = {
		check_name,
		check_gid,
		NULL
	};

	gst_dialog_add_edit_dialog (tool->main_dialog, dialog);

	do {
		response = gtk_dialog_run (GTK_DIALOG (dialog));

		valid = (response == GTK_RESPONSE_OK) ?
			test_battery_run (battery, GTK_WINDOW (dialog), group) : TRUE;
	} while (!valid);

	gtk_widget_hide (dialog);
	gst_dialog_remove_edit_dialog (tool->main_dialog, dialog);

	return response;
}

void
group_settings_dialog_get_data (OobsGroup *group)
{
	GtkWidget *widget;

	widget = gst_dialog_get_widget (tool->main_dialog, "group_settings_gid");
	oobs_group_set_gid (group, gtk_spin_button_get_value (GTK_SPIN_BUTTON (widget)));

	group_members_table_save (group);
}

OobsGroup *
group_settings_dialog_get_group (void)
{
	GtkWidget *widget;
	OobsGroup *group;

	widget = gst_dialog_get_widget (tool->main_dialog, "group_settings_name");
	group  = oobs_group_new(gtk_entry_get_text (GTK_ENTRY (widget)));
	widget = gst_dialog_get_widget (tool->main_dialog, "group_settings_gid");
	oobs_group_set_gid (group, gtk_spin_button_get_value (GTK_SPIN_BUTTON (widget)));

	group_members_table_save (group);

	return group;
}
