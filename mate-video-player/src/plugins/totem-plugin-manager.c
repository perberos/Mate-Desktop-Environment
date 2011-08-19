/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * heavily based on code from Rhythmbox and Gedit
 *
 * Copyright (C) 2002 Paolo Maggi and James Willcox
 * Copyright (C) 2003-2005 Paolo Maggi
 * Copyright (C) 2007 Bastien Nocera <hadess@hadess.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301  USA.
 *
 * Sunday 13th May 2007: Bastien Nocera: Add exception clause.
 * See license_change file for details.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "totem-interface.h"
#include "totem-plugin-manager.h"
#include "totem-plugins-engine.h"
#include "totem-plugin.h"

enum
{
	ACTIVE_COLUMN,
	VISIBLE_COLUMN,
	INFO_COLUMN,
	N_COLUMNS
};

#define PLUGIN_MANAGER_NAME_TITLE _("Plugin")
#define PLUGIN_MANAGER_ACTIVE_TITLE _("Enabled")

#define TOTEM_PLUGIN_MANAGER_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), TOTEM_TYPE_PLUGIN_MANAGER, TotemPluginManagerPrivate))

struct _TotemPluginManagerPrivate
{
	GList		*plugins;
	GtkWidget	*tree;
	GtkTreeModel	*plugin_model;

	GtkWidget	*configure_button;
	GtkWidget	*header_hbox;
	GtkWidget	*plugin_icon;
	GtkWidget	*site_text;
	GtkWidget	*copyright_text;
	GtkWidget	*authors_text;
	GtkWidget	*description_text;
	GtkWidget	*plugin_title;
};

G_DEFINE_TYPE(TotemPluginManager, totem_plugin_manager, GTK_TYPE_VBOX)

static void totem_plugin_manager_finalize (GObject *o);
static TotemPluginInfo *plugin_manager_get_selected_plugin (TotemPluginManager *pm);
static void plugin_manager_toggle_active (GtkTreeIter *iter, GtkTreeModel *model, TotemPluginManager *pm);
static void plugin_manager_toggle_all (TotemPluginManager *pm);

/* Callback functions for GtkBuilder */
G_MODULE_EXPORT void configure_button_cb (GtkWidget *button, TotemPluginManager *pm);

static void
totem_plugin_manager_class_init (TotemPluginManagerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = totem_plugin_manager_finalize;

	g_type_class_add_private (object_class, sizeof (TotemPluginManagerPrivate));
}

void
configure_button_cb (GtkWidget          *button,
		     TotemPluginManager *pm)
{
	TotemPluginInfo *info;
	GtkWindow *toplevel;

	info = plugin_manager_get_selected_plugin (pm);

	g_return_if_fail (info != NULL);

	toplevel = GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (pm)));

	totem_plugins_engine_configure_plugin (info, toplevel);
}

static void
plugin_manager_view_cell_cb (GtkTreeViewColumn *tree_column,
			     GtkCellRenderer   *cell,
			     GtkTreeModel      *tree_model,
			     GtkTreeIter       *iter,
			     gpointer           data)
{
	TotemPluginInfo *info;

	g_return_if_fail (tree_model != NULL);
	g_return_if_fail (tree_column != NULL);

	gtk_tree_model_get (tree_model, iter, INFO_COLUMN, &info, -1);

	if (info == NULL)
		return;

	g_return_if_fail (totem_plugins_engine_get_plugin_name (info) != NULL);

	g_object_set (G_OBJECT (cell),
		      "text",
		      totem_plugins_engine_get_plugin_name (info),
		      NULL);
}

static void
active_toggled_cb (GtkCellRendererToggle *cell,
		   gchar                 *path_str,
		   TotemPluginManager    *pm)
{
	GtkTreeIter iter;
	GtkTreePath *path;
	GtkTreeModel *model;

	path = gtk_tree_path_new_from_string (path_str);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (pm->priv->tree));
	g_return_if_fail (model != NULL);

	if (gtk_tree_model_get_iter (model, &iter, path))
		plugin_manager_toggle_active (&iter, model, pm);

	gtk_tree_path_free (path);
}

static void
cursor_changed_cb (GtkTreeSelection *selection,
		   gpointer     data)
{
	TotemPluginManager *pm = data;
	GtkTreeView *view;
	TotemPluginInfo *info;
	char *string;
	GdkPixbuf *icon;

	view = gtk_tree_selection_get_tree_view (selection);
	info = plugin_manager_get_selected_plugin (pm);

	if (info == NULL)
		return;

	/* update info widgets */
	string = g_strdup_printf ("<span size=\"x-large\">%s</span>",
				  totem_plugins_engine_get_plugin_name (info));
	gtk_label_set_markup (GTK_LABEL (pm->priv->plugin_title), string);
	g_free (string);
	gtk_label_set_text (GTK_LABEL (pm->priv->description_text),
			    totem_plugins_engine_get_plugin_description (info));
	gtk_label_set_text (GTK_LABEL (pm->priv->copyright_text),
			    totem_plugins_engine_get_plugin_copyright (info));
	gtk_label_set_text (GTK_LABEL (pm->priv->site_text),
			    totem_plugins_engine_get_plugin_website (info));

	string = g_strjoinv ("\n", (gchar**)totem_plugins_engine_get_plugin_authors (info));
	gtk_label_set_text (GTK_LABEL (pm->priv->authors_text), string);
	g_free (string);

	icon = totem_plugins_engine_get_plugin_icon (info);
	if (icon != NULL) {
		/* rescale icon to fit header if needed */
		GdkPixbuf *icon_scaled;
		gint width, height, header_height;
		GtkAllocation allocation;

		width = gdk_pixbuf_get_width (icon);
		height = gdk_pixbuf_get_height (icon);
		gtk_widget_get_allocation (pm->priv->header_hbox, &allocation);
		header_height = allocation.height;
		if (height > header_height) {
			icon_scaled = gdk_pixbuf_scale_simple (icon, 
							       (gfloat)width/height*header_height, header_height,
							       GDK_INTERP_BILINEAR);
			gtk_image_set_from_pixbuf (GTK_IMAGE (pm->priv->plugin_icon), icon_scaled);
			g_object_unref (G_OBJECT (icon_scaled));
		} else {
			gtk_image_set_from_pixbuf (GTK_IMAGE (pm->priv->plugin_icon), icon);
		}
	} else {
		gtk_image_set_from_pixbuf (GTK_IMAGE (pm->priv->plugin_icon), NULL);
	}

	gtk_widget_set_sensitive (GTK_WIDGET (pm->priv->configure_button),
				  (info != NULL) &&
				   totem_plugins_engine_plugin_is_configurable (info));
}

static void
row_activated_cb (GtkTreeView       *tree_view,
		  GtkTreePath       *path,
		  GtkTreeViewColumn *column,
		  gpointer           data)
{
	TotemPluginManager *pm = data;
	GtkTreeIter iter;
	GtkTreeModel *model;
	gboolean found;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (pm->priv->tree));
	g_return_if_fail (model != NULL);

	found = gtk_tree_model_get_iter (model, &iter, path);
	g_return_if_fail (found);

	plugin_manager_toggle_active (&iter, model, pm);
}

static void
column_clicked_cb (GtkTreeViewColumn *tree_column,
		   gpointer           data)
{
	TotemPluginManager *pm = TOTEM_PLUGIN_MANAGER (data);

	plugin_manager_toggle_all (pm);
}

static void
plugin_manager_populate_lists (TotemPluginManager *pm)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GList *p;

	for (p = pm->priv->plugins; p != NULL; p = g_list_next (p)) {
		TotemPluginInfo *info;
		info = (TotemPluginInfo *)p->data;

		gtk_list_store_append (GTK_LIST_STORE (pm->priv->plugin_model), &iter);
		gtk_list_store_set (GTK_LIST_STORE (pm->priv->plugin_model), &iter,
				    ACTIVE_COLUMN, totem_plugins_engine_plugin_is_active (info),
				    VISIBLE_COLUMN, totem_plugins_engine_plugin_is_visible (info),
				    INFO_COLUMN, info,
				    -1);
	}

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (pm->priv->tree));
	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (model), &iter)) {
		GtkTreeSelection *selection;

		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (pm->priv->tree));
		g_return_if_fail (selection != NULL);

		gtk_tree_selection_select_iter (selection, &iter);
	}
}

static void
plugin_manager_set_active (GtkTreeIter  *iter,
			   GtkTreeModel *model,
			   gboolean      active,
			   TotemPluginManager *pm)
{
	TotemPluginInfo *info;
	GtkTreeIter child_iter;

	gtk_tree_model_get (model, iter, INFO_COLUMN, &info, -1);

	g_return_if_fail (info != NULL);

	if (active) {
		/* activate the plugin */
		if (!totem_plugins_engine_activate_plugin (info)) {
			active ^= 1;
		}
	} else {
		/* deactivate the plugin */
		if (!totem_plugins_engine_deactivate_plugin (info)) {
			active ^= 1;
		}
	}

	/* set new value */
	gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (model),
							  &child_iter, iter);
	gtk_list_store_set (GTK_LIST_STORE (pm->priv->plugin_model),
			    &child_iter,
			    ACTIVE_COLUMN,
			    totem_plugins_engine_plugin_is_active (info),
			    -1);

	/* cause the configure button sensitivity to be updated */
	cursor_changed_cb (gtk_tree_view_get_selection (GTK_TREE_VIEW (pm->priv->tree)), pm);
}

static void
plugin_manager_toggle_active (GtkTreeIter  *iter,
			      GtkTreeModel *model,
			      TotemPluginManager *pm)
{
	gboolean active, visible;

	gtk_tree_model_get (model, iter,
			    ACTIVE_COLUMN, &active,
			    VISIBLE_COLUMN, &visible,
			    -1);

	if (visible) {
		active ^= 1;
		plugin_manager_set_active (iter, model, active, pm);
	}
}

static TotemPluginInfo *
plugin_manager_get_selected_plugin (TotemPluginManager *pm)
{
	TotemPluginInfo *info = NULL;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *selection;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (pm->priv->tree));
	g_return_val_if_fail (model != NULL, NULL);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (pm->priv->tree));
	g_return_val_if_fail (selection != NULL, NULL);

	if (gtk_tree_selection_get_selected (selection, NULL, &iter)) {
		gtk_tree_model_get (model, &iter, INFO_COLUMN, &info, -1);
	}

	return info;
}

static void
plugin_manager_toggle_all (TotemPluginManager *pm)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	static gboolean active;

	active ^= 1;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (pm->priv->tree));

	g_return_if_fail (model != NULL);

	if (gtk_tree_model_get_iter_first (model, &iter)) {
		do {
			plugin_manager_set_active (&iter, model, active, pm);
		} while (gtk_tree_model_iter_next (model, &iter));
	}
}

/* Callback used as the interactive search comparison function */
static gboolean
name_search_cb (GtkTreeModel *model,
		gint          column,
		const gchar  *key,
		GtkTreeIter  *iter,
		gpointer      data)
{
	TotemPluginInfo *info;
	gchar *normalized_string;
	gchar *normalized_key;
	gchar *case_normalized_string;
	gchar *case_normalized_key;
	gint key_len;
	gboolean retval;

	gtk_tree_model_get (model, iter, INFO_COLUMN, &info, -1);
	if (!info)
		return FALSE;

	normalized_string = g_utf8_normalize (totem_plugins_engine_get_plugin_name (info), -1, G_NORMALIZE_ALL);
	normalized_key = g_utf8_normalize (key, -1, G_NORMALIZE_ALL);
	case_normalized_string = g_utf8_casefold (normalized_string, -1);
	case_normalized_key = g_utf8_casefold (normalized_key, -1);

	key_len = strlen (case_normalized_key);

	/* Oddly enough, this callback must return whether to stop the search
	 * because we found a match, not whether we actually matched.
	 */
	retval = (strncmp (case_normalized_key, case_normalized_string, key_len) != 0);

	g_free (normalized_key);
	g_free (normalized_string);
	g_free (case_normalized_key);
	g_free (case_normalized_string);

	return retval;
}

static void
plugin_manager_construct_tree (TotemPluginManager *pm)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer *cell;
	GtkTreeModel *filter;
	GtkTreeSelection *selection;

	pm->priv->plugin_model = GTK_TREE_MODEL (gtk_list_store_new (N_COLUMNS, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_POINTER));
	filter = gtk_tree_model_filter_new (pm->priv->plugin_model, NULL);
	gtk_tree_model_filter_set_visible_column (GTK_TREE_MODEL_FILTER (filter), VISIBLE_COLUMN);
	gtk_tree_view_set_model (GTK_TREE_VIEW (pm->priv->tree), filter);
	g_object_unref (filter);

	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (pm->priv->tree), TRUE);
	gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW (pm->priv->tree), TRUE);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (pm->priv->tree));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);

	/* first column */
	cell = gtk_cell_renderer_toggle_new ();
	g_signal_connect (cell,
			  "toggled",
			  G_CALLBACK (active_toggled_cb),
			  pm);
	column = gtk_tree_view_column_new_with_attributes (PLUGIN_MANAGER_ACTIVE_TITLE,
							   cell,
							  "active",
							   ACTIVE_COLUMN,
							   NULL);
	gtk_tree_view_column_set_clickable (column, TRUE);
	gtk_tree_view_column_set_resizable (column, TRUE);
	g_signal_connect (column, "clicked", G_CALLBACK (column_clicked_cb), pm);
	gtk_tree_view_append_column (GTK_TREE_VIEW (pm->priv->tree), column);

	/* second column */
	cell = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (PLUGIN_MANAGER_NAME_TITLE, cell, NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_cell_data_func (column, cell, plugin_manager_view_cell_cb,
						 pm, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (pm->priv->tree), column);

	/* Enable search for our non-string column */
	gtk_tree_view_set_search_column (GTK_TREE_VIEW (pm->priv->tree), INFO_COLUMN);
	gtk_tree_view_set_search_equal_func (GTK_TREE_VIEW (pm->priv->tree),
					     name_search_cb,
					     NULL,
					     NULL);

	g_signal_connect (selection,
			  "changed",
			  G_CALLBACK (cursor_changed_cb),
			  pm);
	g_signal_connect (pm->priv->tree,
			  "row_activated",
			  G_CALLBACK (row_activated_cb),
			  pm);

	gtk_widget_show (pm->priv->tree);
}

static int
plugin_name_cmp (gconstpointer a, gconstpointer b)
{
	TotemPluginInfo *lhs = (TotemPluginInfo*)a;
	TotemPluginInfo *rhs = (TotemPluginInfo*)b;
	return strcmp (totem_plugins_engine_get_plugin_name (lhs),
		       totem_plugins_engine_get_plugin_name (rhs));
}

static void
totem_plugin_manager_init (TotemPluginManager *pm)
{
	GtkBuilder *xml;
	GtkWidget *plugins_window;

	pm->priv = TOTEM_PLUGIN_MANAGER_GET_PRIVATE (pm);

	xml = totem_interface_load ("plugins.ui", TRUE, NULL, pm);

	gtk_container_add (GTK_CONTAINER (pm), GTK_WIDGET (gtk_builder_get_object (xml, "plugins_vbox")));

	gtk_box_set_spacing (GTK_BOX (pm), 6);

	pm->priv->tree = gtk_tree_view_new ();
	plugins_window = GTK_WIDGET (gtk_builder_get_object (xml, "plugins_list_window"));
	gtk_container_add (GTK_CONTAINER (plugins_window), pm->priv->tree);

	pm->priv->configure_button = GTK_WIDGET (gtk_builder_get_object (xml, "configure_button"));
	pm->priv->header_hbox = GTK_WIDGET (gtk_builder_get_object (xml, "header_hbox"));
	pm->priv->plugin_title = GTK_WIDGET (gtk_builder_get_object (xml, "plugin_title"));

	pm->priv->plugin_icon = GTK_WIDGET (gtk_builder_get_object (xml, "plugin_icon"));
	pm->priv->site_text = GTK_WIDGET (gtk_builder_get_object (xml, "site_text"));
	pm->priv->copyright_text = GTK_WIDGET (gtk_builder_get_object (xml, "copyright_text"));
	pm->priv->authors_text = GTK_WIDGET (gtk_builder_get_object (xml, "authors_text"));
	pm->priv->description_text = GTK_WIDGET (gtk_builder_get_object (xml, "description_text"));

	plugin_manager_construct_tree (pm);

	/* get the list of available plugins (or installed) */
	pm->priv->plugins = totem_plugins_engine_get_plugins_list ();
	pm->priv->plugins = g_list_sort (pm->priv->plugins, plugin_name_cmp);
	plugin_manager_populate_lists (pm);
	g_object_unref (xml);
}

GtkWidget *
totem_plugin_manager_new (void)
{
	return g_object_new (TOTEM_TYPE_PLUGIN_MANAGER, 0);
}

static void
totem_plugin_manager_finalize (GObject *o)
{
	TotemPluginManager *pm = TOTEM_PLUGIN_MANAGER (o);

	g_list_free (pm->priv->plugins);

	G_OBJECT_CLASS (totem_plugin_manager_parent_class)->finalize (o);
}
