/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* callbacks.c: this file is part of services-admin, a mate-system-tool frontend 
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
 * Authors: Carlos Garnacho Parro <garnacho@tuxerver.net>.
 */

#include <config.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "gst.h"
#include "table.h"
#include "service-settings-table.h"
#include "callbacks.h"
#include "services-tool.h"

extern GstTool *tool;

/* Helpers */
static void
show_settings (void)
{
	GtkWidget *dialog, *table;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	OobsService *service;
	gchar *title;
	gint response;

	table = gst_dialog_get_widget (tool->main_dialog, "services_list");
	dialog = gst_dialog_get_widget (tool->main_dialog, "service_settings_dialog");

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (table));

	if (!gtk_tree_selection_get_selected (selection, &model, &iter))
		return;

	gtk_tree_model_get (model, &iter,
			    COL_OBJECT, &service,
			    -1);

	/* Don't try to commit if not allowed */
	if (!gst_tool_authenticate (tool, OOBS_OBJECT (service))) {
		g_object_unref (service);
		return;
	}

	title = g_strdup_printf (_("Settings for service \"%s\""),
				 oobs_service_get_name (service));

	gtk_window_set_title (GTK_WINDOW (dialog), title);
	g_free (title);

	service_settings_table_set_service (OOBS_SERVICES_CONFIG (GST_SERVICES_TOOL (tool)->services_config),
					    service);

	gst_dialog_add_edit_dialog (tool->main_dialog, dialog);

	gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (tool->main_dialog));
	response = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_hide (dialog);

	gst_dialog_remove_edit_dialog (tool->main_dialog, dialog);

	if (response != GTK_RESPONSE_NONE) {
		service_settings_table_save (service);
		gst_tool_commit (tool, OOBS_OBJECT (service));
	}

	g_object_unref (service);
}

static void
do_popup_menu (GtkWidget *table, GtkWidget *popup, GdkEventButton *event)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean active;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (table));

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gtk_tree_model_get (model, &iter,
				    COL_ACTIVE, &active,
				    -1);

		gtk_menu_popup (GTK_MENU (popup),
				NULL, NULL, NULL, NULL,
				(event) ? event->button : 0,
				(event) ? event->time : gtk_get_current_event_time ());
	}
}

static gboolean
show_warning_dialog (GstTool *tool, OobsService *service)
{
	GtkWidget *dialog;
	gint       response;

	dialog = gtk_message_dialog_new (GTK_WINDOW (tool->main_dialog),
					 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_WARNING,
					 GTK_BUTTONS_YES_NO,
					 _("Are you sure you want to deactivate %s?"),
					 oobs_service_get_name (service));

	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
						  _("This may affect your system behavior in "
						    "several ways, possibly leading to data loss."));

	response = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

	return (response == GTK_RESPONSE_YES);
}

/* callbacks */
void
on_service_toggled (GtkCellRenderer *renderer, gchar *path_str, gpointer data)
{
	GtkTreeView *treeview = GTK_TREE_VIEW (gst_dialog_get_widget (tool->main_dialog, "services_list"));
	GtkTreeModel *model = gtk_tree_view_get_model (treeview);
	GtkTreePath *path;
	GstTool *tool = GST_TOOL (data);
	GtkTreeIter iter;
	OobsService *service;
	gboolean value, new_value, dangerous;

	path = gtk_tree_path_new_from_string (path_str);
	value = gtk_cell_renderer_toggle_get_active (GTK_CELL_RENDERER_TOGGLE (renderer));
	new_value = !value;

	if (!gtk_tree_model_get_iter (model, &iter, path))
		return;

	gtk_tree_model_get (model,
			    &iter,
			    COL_OBJECT, &service,
			    COL_DANGEROUS, &dangerous,
			    -1);

	/* Don't try to commit if not allowed */
	if (!gst_tool_authenticate (tool, OOBS_OBJECT (service))) {
		g_object_unref (service);
		return;
	}

	if (new_value || !dangerous || show_warning_dialog (tool, service)) {
		OobsServicesRunlevel *rl;
		
		rl = (OobsServicesRunlevel *) GST_SERVICES_TOOL (tool)->default_runlevel;
		oobs_service_set_runlevel_configuration (service, rl,
							 (new_value) ? OOBS_SERVICE_START : OOBS_SERVICE_STOP,
							 /* Keep previous priority, see how liboobs handles this */
							 0);
		if (gst_tool_commit (tool, OOBS_OBJECT (service)) != OOBS_RESULT_OK)
			new_value = !new_value;

		gtk_list_store_set (GTK_LIST_STORE (model),
		                    &iter,
		                    COL_ACTIVE, new_value,
		                    -1);
	}

	gtk_tree_path_free (path);
	g_object_unref (service);
}

void
on_popup_settings_activate (gpointer callback_data, guint action, GtkWidget *widget)
{
	show_settings ();
}

gboolean
on_table_button_press_event (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	GtkTreePath *path;
	GtkTreeView *treeview = GTK_TREE_VIEW (widget);
	GtkWidget *popup;

	if (event->button == 3) {
		if (gtk_tree_view_get_path_at_pos (treeview, event->x, event->y, &path, NULL, NULL, NULL)) {
			gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (treeview));
			gtk_tree_selection_select_path (gtk_tree_view_get_selection (treeview), path);

			popup = g_object_get_data (G_OBJECT (treeview), "popup");
			do_popup_menu (widget, popup, event);
		}

		return TRUE;
	}

	/* FIXME:
	if (event->type == GDK_2BUTTON_PRESS ||
	    event->type == GDK_3BUTTON_PRESS) {
		show_settings ();

		return TRUE;
	}
	*/

	return FALSE;
}

gboolean
on_table_popup_menu (GtkWidget *widget, gpointer data)
{
	GtkWidget *popup;

	popup = g_object_get_data (G_OBJECT (widget), "popup");
	do_popup_menu (widget, popup, NULL);

	return TRUE;
}
