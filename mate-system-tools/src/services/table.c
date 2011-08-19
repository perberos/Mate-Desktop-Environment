/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* table.c: this file is part of services-admin, a mate-system-tool frontend 
 * for run level services administration.
 * 
 * Copyright (C) 2002 Ximian, Inc.
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
 * Authors: Carlos Garnacho <garnacho@tuxerver.net>.
 */

#include <config.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "gst.h"
#include "services-tool.h"

#include "table.h"
#include "service.h"
#include "callbacks.h"


extern GstTool *tool;

GtkActionEntry popup_menu_items[] = {
	{ "Properties", GTK_STOCK_PROPERTIES, "_Properties", NULL, NULL, G_CALLBACK (on_popup_settings_activate) }
};

const gchar *ui_description =
	"<ui>"
	"  <popup name='MainMenu'>"
	"    <menuitem action='Properties'/>"
	"  </popup>"
	"</ui>";

static void
table_popup_menu_create (GtkTreeView *table)
{
	GtkUIManager   *ui_manager;
	GtkActionGroup *action_group;
	GtkWidget      *popup;

	action_group = gtk_action_group_new ("MenuActions");
	gtk_action_group_set_translation_domain (action_group, NULL);
	gtk_action_group_add_actions (action_group, popup_menu_items,
				      G_N_ELEMENTS (popup_menu_items), table);

	ui_manager = gtk_ui_manager_new ();
	gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);

	if (!gtk_ui_manager_add_ui_from_string (ui_manager, ui_description, -1, NULL))
		return;

	popup = gtk_ui_manager_get_widget (ui_manager, "/MainMenu");

	g_object_set_data_full (G_OBJECT (table),
				"popup", popup,
				(GDestroyNotify) gtk_widget_destroy);
}

static void
add_columns (GtkTreeView *treeview)
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	column = gtk_tree_view_column_new ();

	/* Checkbox */
	renderer = gtk_cell_renderer_toggle_new ();

	g_signal_connect (G_OBJECT (renderer), "toggled",
			  G_CALLBACK (on_service_toggled), tool);

	g_object_set (G_OBJECT (renderer), "xpad", 6, NULL);

	gtk_tree_view_insert_column_with_attributes (treeview,
						     -1, "",
						     renderer,
						     "active", COL_ACTIVE,
						     NULL);
	/* Image */
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "pixbuf", COL_IMAGE,
					     NULL);

	/* Text */
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column, renderer,
					     "markup", COL_DESC,
					     NULL);

	g_object_set (G_OBJECT (renderer), "yalign", 0, NULL);
	gtk_tree_view_insert_column (treeview, column, -1);

	gtk_tree_view_column_set_sort_column_id (column, 1);
	gtk_tree_view_column_clicked (column);
}

static GtkTreeModel*
create_model (void)
{
	GtkListStore *model;

	model = gtk_list_store_new (COL_LAST,
				    G_TYPE_BOOLEAN,
				    G_TYPE_STRING,
	                            G_TYPE_STRING,
				    GDK_TYPE_PIXBUF,
				    G_TYPE_INT,
				    G_TYPE_OBJECT,
				    OOBS_TYPE_LIST_ITER);
	return GTK_TREE_MODEL(model);
}

void
table_create (void)
{
	GtkWidget *runlevel_table = gst_dialog_get_widget (tool->main_dialog, "services_list");
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	
	model = create_model ();
	gtk_tree_view_set_model (GTK_TREE_VIEW (runlevel_table), model);
	g_object_unref (G_OBJECT (model));
	
	add_columns (GTK_TREE_VIEW (runlevel_table));
	table_popup_menu_create (GTK_TREE_VIEW (runlevel_table));
	gtk_tree_view_set_tooltip_column (GTK_TREE_VIEW (runlevel_table), COL_TOOLTIP);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (runlevel_table));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
}

static gboolean
service_get_active (OobsService *service)
{
	OobsServicesRunlevel *rl;
	guint status;
	gint priority;

	rl = (OobsServicesRunlevel *) GST_SERVICES_TOOL (tool)->default_runlevel;
	oobs_service_get_runlevel_configuration (service, rl, &status, &priority);

	return (status == OOBS_SERVICE_START);
}

static GdkPixbuf*
get_service_icon (GstTool     *tool,
		  const gchar *icon_name)
{
	GdkPixbuf *icon = NULL;

	if (icon_name)
		icon = gtk_icon_theme_load_icon (tool->icon_theme, icon_name, 32, 0, NULL);

	/* fallback icon */
	if (!icon)
		icon = gtk_icon_theme_load_icon (tool->icon_theme, "exec", 32, 0, NULL);

	return icon;
}

void
table_add_service (OobsService *service, OobsListIter *list_iter)
{
	const ServiceDescription *desc;
	GtkTreeView *table;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GdkPixbuf *icon;
	gchar *description;

	desc = service_search (service);

	if (desc) {
		icon = get_service_icon (tool, desc->icon);
		description = g_strdup_printf ("<span weight=\"bold\" size=\"larger\">%s</span>\n<i>%s</i>",
		                               oobs_service_get_name (service),
		                               _(desc->description));
	}
	else {
		icon = get_service_icon (tool, NULL);
		description = g_strdup_printf ("<span weight=\"bold\" size=\"larger\">%s</span>",
		                               oobs_service_get_name (service));
	}

	table = GTK_TREE_VIEW (gst_dialog_get_widget (tool->main_dialog, "services_list"));
	model = gtk_tree_view_get_model (table);

	gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter,
			    COL_ACTIVE, service_get_active (service),
			    COL_DESC, description,
	                    COL_TOOLTIP, desc ? _(desc->long_description) : NULL,
			    COL_IMAGE, icon,
			    COL_DANGEROUS, desc ? desc->dangerous : FALSE,
			    COL_OBJECT, service,
			    COL_ITER, list_iter,
			    -1);

	if (icon)
		g_object_unref (icon);

	g_free (description);
}
