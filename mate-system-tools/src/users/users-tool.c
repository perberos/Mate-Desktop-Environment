/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2005 Carlos Garnacho
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

#include <glib.h>
#include <glib/gi18n.h>
#include "callbacks.h"
#include "user-profiles.h"
#include "users-table.h"
#include "groups-table.h"
#include "privileges-table.h"
#include "table.h"
#include "users-tool.h"
#include "gst.h"

static void  gst_users_tool_class_init     (GstUsersToolClass *class);
static void  gst_users_tool_init           (GstUsersTool      *tool);
static void  gst_users_tool_finalize       (GObject           *object);
static void  gst_users_tool_update_config  (GstTool *tool);

static GObject* gst_users_tool_constructor (GType                  type,
					    guint                  n_construct_properties,
					    GObjectConstructParam *construct_params);

G_DEFINE_TYPE (GstUsersTool, gst_users_tool, GST_TYPE_TOOL);

static void
gst_users_tool_class_init (GstUsersToolClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	GstToolClass *tool_class = GST_TOOL_CLASS (class);

	object_class->constructor = gst_users_tool_constructor;
	object_class->finalize = gst_users_tool_finalize;
	tool_class->update_gui = gst_users_tool_update_gui;
	tool_class->update_config = gst_users_tool_update_config;
}

static void
on_option_changed (GSettings  *settings,
                   const char *key,
                   gpointer    user_data)
{
	GstTool *tool = GST_TOOL (user_data);
	GtkWidget *widget;
	GtkTreeModel *sort_model, *filter_model;

	GST_USERS_TOOL (tool)->showall = g_settings_get_boolean (settings, "showall");
	GST_USERS_TOOL (tool)->showroot = g_settings_get_boolean (settings, "showroot");

	widget = gst_dialog_get_widget (tool->main_dialog, "users_table");
	sort_model = gtk_tree_view_get_model (GTK_TREE_VIEW (widget));
	filter_model = gtk_tree_model_sort_get_model (GTK_TREE_MODEL_SORT (sort_model));
	gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (filter_model));
}

static void
gst_users_tool_init (GstUsersTool *tool)
{
	tool->users_config = oobs_users_config_get ();
	gst_tool_add_configuration_object (GST_TOOL (tool), tool->users_config, TRUE);

	tool->groups_config = oobs_groups_config_get ();
	gst_tool_add_configuration_object (GST_TOOL (tool), tool->groups_config, TRUE);

	tool->self_config = oobs_self_config_get ();
	gst_tool_add_configuration_object (GST_TOOL (tool), tool->self_config, TRUE);

	tool->profiles = gst_user_profiles_get ();

	tool->settings = g_settings_new ("org.mate.system-tools.users");
}

static GObject*
gst_users_tool_constructor (GType                  type,
			    guint                  n_construct_properties,
			    GObjectConstructParam *construct_params)
{
	GObject *object;
	GstUsersTool *tool;

	object = (* G_OBJECT_CLASS (gst_users_tool_parent_class)->constructor) (type,
										n_construct_properties,
										construct_params);

	tool = GST_USERS_TOOL (object);

	g_signal_connect (tool->settings, "changed::showall",
	                  (GCallback) on_option_changed, tool);
	g_signal_connect (tool->settings, "changed::showroot",
	                  (GCallback) on_option_changed, tool);

	tool->showall = g_settings_get_boolean (tool->settings, "showall");
	tool->showroot = g_settings_get_boolean (tool->settings, "showroot");

	return object;
}

static void
gst_users_tool_finalize (GObject *object)
{
	GstUsersTool *tool = GST_USERS_TOOL (object);

	g_object_unref (tool->users_config);
	g_object_unref (tool->self_config);
	g_object_unref (tool->groups_config);
	g_object_unref (tool->profiles);
	g_object_unref (tool->settings);

	/* Clear models to unreference OobsUsers and OobsGroups
	 * to be sure they are finalized properly (passwords...) */
	users_table_clear ();
	groups_table_clear ();

	(* G_OBJECT_CLASS (gst_users_tool_parent_class)->finalize) (object);
}

static void
update_users (GstUsersTool *tool)
{
	OobsList *list;
	OobsListIter iter;
	GObject *user;
	OobsUser *self;
	GtkTreePath *path;
	gboolean valid;

	users_table_clear ();
	list = oobs_users_config_get_users (OOBS_USERS_CONFIG (tool->users_config));
	self = oobs_self_config_get_user (OOBS_SELF_CONFIG (tool->self_config));

	valid = oobs_list_get_iter_first (list, &iter);

	while (valid) {
		user = oobs_list_get (list, &iter);
		path = users_table_add_user (OOBS_USER (user));
		gst_tool_add_configuration_object (GST_TOOL (tool), OOBS_OBJECT (user), FALSE);

		if (self == (OobsUser *) user)
			users_table_select_path (path);

		g_object_unref (user);
		gtk_tree_path_free (path);
		valid = oobs_list_iter_next (list, &iter);
	}
}

static void
update_groups (GstUsersTool *tool)
{
	OobsList *list;
	OobsListIter iter;
	GObject *group;
	gboolean valid;

	groups_table_clear ();
	privileges_table_clear ();
	list = oobs_groups_config_get_groups (OOBS_GROUPS_CONFIG (tool->groups_config));

	valid = oobs_list_get_iter_first (list, &iter);

	while (valid) {
		group = oobs_list_get (list, &iter);
		groups_table_add_group (OOBS_GROUP (group));
		gst_tool_add_configuration_object (GST_TOOL (tool), OOBS_OBJECT (group), FALSE);

		/* update privileges table too */
		privileges_table_add_group (OOBS_GROUP (group));

		g_object_unref (group);
		valid = oobs_list_iter_next (list, &iter);
	}
}

static void
update_profiles (GstUsersTool *tool)
{
	GList *list, *l;
	GtkWidget *label1, *label2, *button;
	GstUserProfile *profile;
	int max_len, len;

	list = gst_user_profiles_get_list (tool->profiles);
	table_populate_profiles (tool, list);

	/* Hide profiles line in main dialog if only one profile is available */
	label1 = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog,
	                                "user_settings_profile");
	label2 = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog,
	                                "user_settings_profile_label");
	button = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog,
	                                "edit_user_profile_button");

	if (g_list_length (list) > 1) {
		gtk_widget_show (label1);
		gtk_widget_show (label2);
		gtk_widget_show (button);
	}
	else {
		gtk_widget_hide (label1);
		gtk_widget_hide (label2);
		gtk_widget_hide (button);
		return;
	}

	/* use the length of the longest profile name to avoid resizing
	 * the label and moving widgets around */
	max_len = 0;
	for (l = list; l; l = l->next) {
		profile = l->data;
		len = g_utf8_strlen (profile->name, -1);
		if (len > max_len)
			max_len = len;
	}

	gtk_label_set_width_chars (GTK_LABEL (label1), max_len);
}

static void
update_shells (GstUsersTool *tool)
{
	GtkWidget *combo;
	GtkTreeModel *model;
	GList *shells;
	GtkTreeIter iter;

	combo = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "user_settings_shell");
	model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));
	shells = oobs_users_config_get_available_shells (OOBS_USERS_CONFIG (tool->users_config));

	while (shells) {
		gtk_list_store_append (GTK_LIST_STORE (model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
				    0, shells->data,
				    -1);
		shells = shells->next;
	}
}

void
gst_users_tool_update_gui (GstTool *tool)
{
	update_users (GST_USERS_TOOL (tool));
	update_groups (GST_USERS_TOOL (tool));
	update_profiles (GST_USERS_TOOL (tool));
	update_shells (GST_USERS_TOOL (tool));
}

static void
gst_users_tool_update_config (GstTool *tool)
{
	GstUsersTool *users_tool;

	users_tool = GST_USERS_TOOL (tool);

	g_object_get (G_OBJECT (users_tool->users_config),
		      "minimum-uid", &users_tool->minimum_uid,
		      "maximum-uid", &users_tool->maximum_uid,
		      NULL);
	g_object_get (G_OBJECT (users_tool->groups_config),
		      "minimum-gid", &users_tool->minimum_gid,
		      "maximum-gid", &users_tool->maximum_gid,
		      NULL);
}

GstTool*
gst_users_tool_new (void)
{
	return g_object_new (GST_TYPE_USERS_TOOL,
			     "name", "users",
			     "title", _("Users Settings"),
			     "icon", "config-users",
	                     "lock-button", FALSE,
			     NULL);
}
