/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2006 Carlos Garnacho
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
 * Authors: Carlos Garnacho <carlosg@mate.org>
 */

#include <config.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "gst.h"
#include "services-tool.h"
#include "service-settings-table.h"

extern GstTool *tool;

enum {
	COL_NAME,
	COL_RUNLEVEL,
	COL_STATUS,
	COL_STATUS_STRING,
	COL_PRIORITY,
	COL_LAST
};

enum {
	STATUS_COL_STRING,
	STATUS_COL_ID,
	STATUS_COL_LAST
};

static GtkTreeModel*
create_model (void)
{
	GtkListStore *store;

	store = gtk_list_store_new (COL_LAST,
				    G_TYPE_STRING,
				    G_TYPE_POINTER,
				    G_TYPE_INT,
				    G_TYPE_STRING,
				    G_TYPE_INT);

	return GTK_TREE_MODEL (store);
}

static GtkTreeModel*
create_combo_model (void)
{
	GtkListStore *store;
	GtkTreeIter iter;

	store = gtk_list_store_new (STATUS_COL_LAST,
				    G_TYPE_STRING,
				    G_TYPE_INT);

	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter,
			    STATUS_COL_STRING, _("Start"),
			    STATUS_COL_ID, OOBS_SERVICE_START,
			    -1);

	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter,
			    STATUS_COL_STRING, _("Stop"),
			    STATUS_COL_ID, OOBS_SERVICE_STOP,
			    -1);

	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter,
			    STATUS_COL_STRING, _("Ignore"),
			    STATUS_COL_ID, OOBS_SERVICE_IGNORE,
			    -1);

	return GTK_TREE_MODEL (store);
}

static gchar*
status_to_string (gint status)
{
	switch (status) {
	case OOBS_SERVICE_START:
		return _("Start");
	case OOBS_SERVICE_STOP:
		return _("Stop");
	case OOBS_SERVICE_IGNORE:
	default:
		return _("Ignore");
	}		
}

static gint
string_to_status (const gchar *str)
{
	if (strcmp (str, _("Start")) == 0)
		return OOBS_SERVICE_START;
	else if (strcmp (str, _("Stop")) == 0)
		return OOBS_SERVICE_STOP;
	else
		return OOBS_SERVICE_IGNORE;
}


static void
on_status_edited (GtkCellRendererText *cell_renderer,
		  const gchar         *path_string,
		  const gchar         *new_text,
		  gpointer             data)
{
	GtkTreeModel *model;
	GtkTreePath *path;
	GtkTreeIter iter;

	model = GTK_TREE_MODEL (data);
	path = gtk_tree_path_new_from_string (path_string);
	gtk_tree_model_get_iter (model, &iter, path);

	gtk_list_store_set (GTK_LIST_STORE (model), &iter,
			    COL_STATUS, string_to_status (new_text),
			    COL_STATUS_STRING, new_text,
			    -1);

	gtk_tree_path_free (path);
}

static void
on_priority_edited (GtkCellRendererText *cell_renderer,
		    const gchar         *path_string,
		    const gchar         *new_text,
		    gpointer             data)
{
	GtkTreeModel *model;
	GtkTreePath *path;
	GtkTreeIter iter;
	gint val;

	model = GTK_TREE_MODEL (data);
	path = gtk_tree_path_new_from_string (path_string);
	gtk_tree_model_get_iter (model, &iter, path);

	val = (gint) g_strtod (new_text, NULL);

	gtk_list_store_set (GTK_LIST_STORE (model), &iter,
			    COL_PRIORITY, val,
			    -1);

	gtk_tree_path_free (path);
}

static void
add_columns (GtkWidget    *table,
	     GtkTreeModel *model)
{
	GtkTreeModel *combo_model;
	GtkCellRenderer *renderer;
	GtkObject *adjustment;

	/* runlevel name */
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (table), -1,
						     _("Runlevel"),
						     renderer,
						     "text", COL_NAME,
						     NULL);

	/* runlevel status */
	renderer = gtk_cell_renderer_combo_new ();
	combo_model = create_combo_model ();

	g_object_set (G_OBJECT (renderer),
		      "editable", TRUE,
		      "has-entry", FALSE,
		      "text-column", STATUS_COL_STRING,
		      "model", combo_model,
		      NULL);

	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (table), -1,
						     _("Status"),
						     renderer,
						     "text", COL_STATUS_STRING,
						     NULL);
	g_object_unref (combo_model);

	g_signal_connect (G_OBJECT (renderer), "edited",
			  G_CALLBACK (on_status_edited), model);

	/* runlevel priority */
	renderer = gtk_cell_renderer_spin_new ();
	adjustment = gtk_adjustment_new (0, 0, 99, 1, 10, 10);

	g_object_set (G_OBJECT (renderer),
		      "editable", TRUE,
		      "adjustment", adjustment,
		      NULL);

	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (table), -1,
						     _("Priority"),
						     renderer,
						     "text", COL_PRIORITY,
						     NULL);

	g_signal_connect (G_OBJECT (renderer), "edited",
			  G_CALLBACK (on_priority_edited), model);
}

void
service_settings_table_create (void)
{
	GtkWidget *table;
	GtkTreeModel *model;

	table = gst_dialog_get_widget (tool->main_dialog, "service_settings_table");

	model = create_model ();
	gtk_tree_view_set_model (GTK_TREE_VIEW (table), model);
	g_object_unref (model);

	add_columns (table, model);
}

static void
populate_table (GList       *runlevels,
		OobsService *service)
{
	GtkWidget *table;
	GtkTreeModel *model;
	GtkTreeIter iter;
	OobsServicesRunlevel *runlevel;
	gint priority;
	OobsServiceStatus status;

	table = gst_dialog_get_widget (tool->main_dialog, "service_settings_table");
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (table));

	gtk_list_store_clear (GTK_LIST_STORE (model));

	while (runlevels) {
		runlevel = runlevels->data;

		oobs_service_get_runlevel_configuration (service,
							 runlevel,
							 &status,
							 &priority);

		gtk_list_store_append (GTK_LIST_STORE (model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter,
				    COL_NAME, runlevel->name,
				    COL_RUNLEVEL, runlevel,
				    COL_STATUS, status,
				    COL_STATUS_STRING, status_to_string (status),
				    COL_PRIORITY, priority,
				    -1);

		runlevels = runlevels->next;
	}
}

void
service_settings_table_set_service (OobsServicesConfig *config,
				    OobsService        *service)
{
	GList *runlevels;

	runlevels = oobs_services_config_get_runlevels (config);
	populate_table (runlevels, service);
	g_list_free (runlevels);
}

void
service_settings_table_save (OobsService *service)
{
	GtkWidget *table;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean valid;
	OobsServicesRunlevel *rl;
	gint status, priority;

	table = gst_dialog_get_widget (tool->main_dialog, "service_settings_table");
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (table));
	valid = gtk_tree_model_get_iter_first (model, &iter);

	while (valid) {
		gtk_tree_model_get (model, &iter,
				    COL_RUNLEVEL, &rl,
				    COL_STATUS, &status,
				    COL_PRIORITY, &priority,
				    -1);

		oobs_service_set_runlevel_configuration (service, rl, status, priority);
		valid = gtk_tree_model_iter_next (model, &iter);
	}
}
