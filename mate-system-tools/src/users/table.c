/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* table.c: this file is part of users-admin, a ximian-setup-tool frontend 
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
 * Authors: Carlos Garnacho Parro <garparr@teleline.es>,
 *          Milan Bouchet-Valat <nalimilan@club.fr>.
 */

#include <config.h>
#include "gst.h"
#include <glib/gi18n.h>
#include <pango/pango.h>

#include "table.h"
#include "users-table.h"
#include "groups-table.h"
#include "privileges-table.h"
#include "group-members-table.h"
#include "callbacks.h"

extern GstTool *tool;

GtkActionEntry popup_menu_items [] = {
	{ "Add",         GTK_STOCK_ADD,        N_("_Add"),        NULL, NULL, G_CALLBACK (on_popup_add_activate)      },
	{ "Properties",  GTK_STOCK_PROPERTIES, N_("_Properties"), NULL, NULL, G_CALLBACK (on_popup_settings_activate) },
	{ "Delete",      GTK_STOCK_DELETE,     N_("_Delete"),     NULL, NULL, G_CALLBACK (on_popup_delete_activate)   }
};

const gchar *ui_description =
	"<ui>"
	"  <popup name='MainMenu'>"
	"    <menuitem action='Add'/>"
	"    <separator/>"
	"    <menuitem action='Properties'/>"
	"    <menuitem action='Delete'/>"
	"  </popup>"
	"</ui>";

GtkWidget*
popup_menu_create (GtkWidget *widget, gint table)
{
	GtkUIManager   *ui_manager;
	GtkActionGroup *action_group;
	GtkWidget      *popup;

	action_group = gtk_action_group_new ("MenuActions");
	gtk_action_group_set_translation_domain (action_group, NULL);
	gtk_action_group_add_actions (action_group, popup_menu_items,
				      G_N_ELEMENTS (popup_menu_items),
				      GINT_TO_POINTER (table));

	ui_manager = gtk_ui_manager_new ();
	gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);

	if (!gtk_ui_manager_add_ui_from_string (ui_manager, ui_description, -1, NULL))
		return NULL;

	g_object_set_data (G_OBJECT (widget), "ui-manager", ui_manager);
	popup = gtk_ui_manager_get_widget (ui_manager, "/MainMenu");

	return popup;
}

static void
setup_groups_combo (void)
{
	GtkWidget *combo = gst_dialog_get_widget (tool->main_dialog, "user_settings_group");
	GtkWidget *table = gst_dialog_get_widget (tool->main_dialog, "groups_table");
	GtkCellRenderer *cell;
	GtkTreeModel *model;

	cell = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT (combo), cell, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT (combo), cell, "text", 0, NULL);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (table));
	gtk_combo_box_set_model (GTK_COMBO_BOX (combo), model);
	g_object_unref (model);
}

static void
setup_shells_combo (GstUsersTool *tool)
{
	GtkWidget *combo;
	GtkTreeModel *model;

	combo = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "user_settings_shell");
	model = GTK_TREE_MODEL (gtk_list_store_new (1, G_TYPE_STRING));

	gtk_combo_box_set_model (GTK_COMBO_BOX (combo), model);
	g_object_unref (model);

	gtk_combo_box_entry_set_text_column (GTK_COMBO_BOX_ENTRY (combo), 0);
}

void
table_populate_profiles (GstUsersTool *tool,
                         GList *profiles)
{
	GstUserProfile *profile;
	GtkWidget *table;
	GtkWidget *radio;
	GtkWidget *label;
	GHashTable *radios;
	GHashTable *labels;
	GHashTableIter iter;
	gpointer value;
	GList *l;
	static int ncols, nrows; /* original size of the table */
	PangoAttribute *attribute;
	PangoAttrList *attributes;
	int i;

	table = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "user_profile_table");
	radios = g_object_get_data (G_OBJECT (table), "radio_buttons");
	labels = g_object_get_data (G_OBJECT (table), "labels");

	/* create the hash table to hold references to radio buttons and their labels */
	if (radios == NULL) {
		/* keys are names owned by GstUserProfiles, values are pointers:
		 * no need to free anything */
		radios = g_hash_table_new (g_str_hash, g_str_equal);
		labels = g_hash_table_new (g_str_hash, g_str_equal);
		g_object_set_data (G_OBJECT (table), "radio_buttons", radios);
		g_object_set_data (G_OBJECT (table), "labels", labels);
		g_object_get (G_OBJECT (table), "n-rows", &nrows, "n-columns", &ncols, NULL);
	}
	else {
		/* free the radio buttons and labels if they were already here */
		g_hash_table_iter_init (&iter, radios);
		while (g_hash_table_iter_next (&iter, NULL, &value)) {
			gtk_widget_destroy (GTK_WIDGET (value));
		}

		g_hash_table_iter_init (&iter, labels);
		while (g_hash_table_iter_next (&iter, NULL, &value)) {
			gtk_widget_destroy (GTK_WIDGET (value));
		}

		g_hash_table_remove_all (radios);
		g_hash_table_remove_all (labels);
	}

	/* increase table's size based on it's "empty" size
	 * we leave an empty line after a radio and its decription label */
	gtk_table_resize (GTK_TABLE (table), g_list_length (profiles) * 3 + nrows, ncols);

	radio = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "user_profile_custom");
	label = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "user_profile_custom_label");

	attributes = pango_attr_list_new ();
	attribute = pango_attr_size_new (9 * PANGO_SCALE);
	pango_attr_list_insert (attributes, attribute);
	attribute = pango_attr_style_new (PANGO_STYLE_ITALIC);
	pango_attr_list_insert (attributes, attribute);
	gtk_label_set_attributes (GTK_LABEL (label), attributes);

	i = 1; /* empty line after "Custom" radio and label */
	for (l = profiles; l; l = l->next) {
		profile = (GstUserProfile *) l->data;
		radio = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radio),
		                                                     profile->name);
		label = gtk_label_new (profile->description);
		gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
		gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
		gtk_misc_set_padding (GTK_MISC (label), 16, 0);
		gtk_label_set_attributes (GTK_LABEL (label), attributes);

		gtk_table_attach_defaults (GTK_TABLE (table),
		                           radio, 0, ncols,
		                           nrows + i, nrows + i + 1);
		gtk_table_attach_defaults (GTK_TABLE (table),
		                           label, 1, ncols,
		                           nrows + i + 1, nrows + i + 2);
		gtk_widget_show (radio);
		gtk_widget_show (label);

		g_hash_table_replace (radios, profile->name, radio);
		g_hash_table_replace (labels, profile->name, label);
		i += 3;
	}
}

void
create_tables (GstUsersTool *tool)
{
	create_users_table (tool);
	create_groups_table ();
	create_user_privileges_table ();
	create_group_members_table ();

	/* not strictly tables, but uses a model */
	setup_groups_combo ();
	setup_shells_combo (tool);
}
