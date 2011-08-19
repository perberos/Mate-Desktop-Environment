/* MateComponent component browser
 *
 * AUTHORS:
 *      Dan Siemon <dan@coverfire.com>
 *      Rodrigo Moya <rodrigo@mate-db.org>
 *      Patanjali Somayaji <patanjali@morelinux.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "oaf-helper.h"
#include "component-list.h"

#define PARENT_TYPE GTK_TYPE_VBOX

/*
  Defines for the GTK+ tree view columns.
*/
#define NUM_COLUMNS 4
#define COL_ACTIVE 0
#define COL_NAME 1
#define COL_TYPE 2
#define COL_IID 3

struct _ComponentListPrivate {
	GtkWidget *scroll;
	GtkWidget *list;
	GtkListStore *model;
	GList *components;
	GdkPixbuf *active_icon;
	GdkPixbuf *inactive_icon;
};

enum {
	COMPONENTDETAILS_SIGNAL,
	LAST_SIGNAL
};

static gint component_list_signals[LAST_SIGNAL] = {0};

static void component_list_class_init (ComponentListClass *klass);
static void component_list_init       (ComponentList *comp_list,
				       ComponentListClass *klass);
static void component_list_finalize   (GObject *object);

static GObjectClass *parent_class = NULL;

/***************************
 * Callbacks
 ***************************/

/**
 * Called when the user double clicks on one of the rows or hits the
 * space bar when a row is selected.
 */
static void
row_activated_cb (GtkTreeView *tree, GtkTreePath *path,
		  GtkTreeViewColumn *col, ComponentList *list)
{
	g_signal_emit_by_name (G_OBJECT (list), "component-details");
}

/*
 * Tree View sorts
 */
static gint
sort_component_active (GtkTreeModel *model, GtkTreeIter *a,
			    GtkTreeIter *b, gpointer user_data)
{
	gboolean component1, component2;

	gtk_tree_model_get (model, a, COL_ACTIVE, &component1, -1);
	gtk_tree_model_get (model, b, COL_ACTIVE, &component2, -1);

	if (component1 == component2) {
		return 0;
	} else if ((component1 == TRUE) && (component2 == FALSE)) {
		/* 'larger' */
		return 1;
	} else {
		/* component1 == FALSE && component2 == TRUE */
		/* 'smaller' */
		return -1;
	}
}

static gint
sort_component_name (GtkTreeModel *model, GtkTreeIter *a,
			  GtkTreeIter *b, gpointer user_data)
{
	gchar *component1, *component2;
	gint ret;

	gtk_tree_model_get (model, a, COL_NAME, &component1, -1);
	gtk_tree_model_get (model, b, COL_NAME, &component2, -1);

	ret = g_utf8_collate (component1, component2);

	g_free (component1);
	g_free (component2);

	return ret;
}

static gint
sort_component_type (GtkTreeModel *model, GtkTreeIter *a,
			  GtkTreeIter *b, gpointer user_data)
{
	gchar *component1, *component2;
	gint ret;

	gtk_tree_model_get (model, a, COL_TYPE, &component1, -1);
	gtk_tree_model_get (model, b, COL_TYPE, &component2, -1);

	ret = g_utf8_collate (component1, component2);

	g_free (component1);
	g_free (component2);

	return ret;
}

static gint
sort_component_iid (GtkTreeModel *model, GtkTreeIter *a,
			 GtkTreeIter *b, gpointer user_data)
{
	gchar *component1, *component2;
	gint ret;

	gtk_tree_model_get (model, a, COL_IID, &component1, -1);
	gtk_tree_model_get (model, b, COL_IID, &component2, -1);

	ret = g_utf8_collate (component1, component2);

	g_free (component1);
	g_free (component2);

	return ret;
}

/*
 * ComponentList class implementation
 */

static void
component_list_class_init (ComponentListClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	component_list_signals[COMPONENTDETAILS_SIGNAL] =
		g_signal_new ("component-details",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (ComponentListClass,
					       component_details),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	parent_class = g_type_class_peek_parent (klass);

	object_class->finalize = component_list_finalize;
}

static void
set_pixbuf_func (GtkTreeViewColumn *tree_column,
		 GtkCellRenderer *cell,
		 GtkTreeModel *model,
		 GtkTreeIter *iter,
		 gpointer user_data)
{
	ComponentList *comp_list;
	gboolean active;

	comp_list = (ComponentList *) user_data;

	gtk_tree_model_get (model, iter, 0, &active, -1);

	if (active && comp_list->priv->active_icon != NULL) {
		g_object_set (GTK_CELL_RENDERER (cell),
			      "pixbuf",
			      comp_list->priv->active_icon,
			      NULL);
	}
	else if (comp_list->priv->inactive_icon != NULL) {
		g_object_set (GTK_CELL_RENDERER (cell),
			      "pixbuf",
			      comp_list->priv->inactive_icon,
			      NULL);
	}
}

static void
set_name_func (GtkTreeViewColumn *tree_column,
	       GtkCellRenderer *cell,
	       GtkTreeModel *model,
	       GtkTreeIter *iter,
	       gpointer user_data)
{
	gchar *name;

	gtk_tree_model_get (model, iter, 1, &name, -1);

	g_object_set (G_OBJECT (cell), "text", name, NULL);

	g_free (name);
}

static void
set_iid_func (GtkTreeViewColumn *tree_column,
	      GtkCellRenderer *cell,
	      GtkTreeModel *model,
	      GtkTreeIter *iter,
	      gpointer user_data)
{
	gchar *iid;

	gtk_tree_model_get (model, iter, 3, &iid, -1);

	g_object_set (G_OBJECT (cell), "text", iid, NULL);

	g_free (iid);
}

static void
set_type_func (GtkTreeViewColumn *tree_column,
	       GtkCellRenderer *cell,
	       GtkTreeModel *model,
	       GtkTreeIter *iter,
	       gpointer user_data)
{
	gchar *type;
 
	gtk_tree_model_get (model, iter, 2, &type, -1);

	g_object_set (G_OBJECT (cell), "text", type, NULL);

	g_free (type);
}

static void
component_list_init (ComponentList *comp_list, ComponentListClass *klass)
{
	GtkIconSize size;
	gint w, h;
	GdkPixbuf *scaled;
	GtkWidget *hbox;
	GtkTreeViewColumn *column;
	GtkCellRenderer *text_renderer;
	GtkCellRenderer *pixbuf_renderer = NULL;

	g_return_if_fail (IS_COMPONENT_LIST (comp_list));

	comp_list->priv = g_new0 (ComponentListPrivate, 1);
	comp_list->priv->components = NULL;

	/* Create the active/inactive icons */
	/* Active icon */
	size = GTK_ICON_SIZE_BUTTON;
	comp_list->priv->active_icon = gtk_widget_render_icon (
		GTK_WIDGET (comp_list), GTK_STOCK_YES, size, NULL);
	gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &w, &h);
	scaled = gdk_pixbuf_scale_simple (comp_list->priv->active_icon,
					  w, h,
					  GDK_INTERP_BILINEAR);
	g_object_unref (comp_list->priv->active_icon);
	comp_list->priv->active_icon = scaled;

	/* Inactive icon */
	size = GTK_ICON_SIZE_BUTTON;
	comp_list->priv->inactive_icon = gtk_widget_render_icon (
		GTK_WIDGET (comp_list), GTK_STOCK_NO, size, NULL);
	gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &w, &h);
	scaled = gdk_pixbuf_scale_simple (
		comp_list->priv->inactive_icon,
		w, h,
		GDK_INTERP_BILINEAR);
	g_object_unref (comp_list->priv->inactive_icon);
	comp_list->priv->inactive_icon = scaled;

	/* create the main container */
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (comp_list), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);

	/* create the list */
	comp_list->priv->scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (
		GTK_SCROLLED_WINDOW (comp_list->priv->scroll),
		GTK_POLICY_AUTOMATIC,
		GTK_POLICY_AUTOMATIC);
	gtk_widget_show (comp_list->priv->scroll);
	gtk_box_pack_start (GTK_BOX (comp_list), comp_list->priv->scroll,
			    TRUE, TRUE, 0);

	comp_list->priv->list = gtk_tree_view_new ();
	gtk_widget_show (comp_list->priv->list);
	gtk_container_add (GTK_CONTAINER (comp_list->priv->scroll),
			   comp_list->priv->list);

	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (comp_list->priv->list),
				      TRUE);

	/* Connect the row_activated signal */
	g_signal_connect (G_OBJECT (comp_list->priv->list),
			  "row_activated",
			  G_CALLBACK (row_activated_cb), comp_list);

	/* Create the model */
	comp_list->priv->model = gtk_list_store_new (NUM_COLUMNS,
						     G_TYPE_BOOLEAN,
						     G_TYPE_STRING,
						     G_TYPE_STRING,
						     G_TYPE_STRING);
	gtk_tree_view_set_model (GTK_TREE_VIEW (comp_list->priv->list),
				 GTK_TREE_MODEL (comp_list->priv->model));

	/* Set the sort functions */
	gtk_tree_sortable_set_sort_func (
		GTK_TREE_SORTABLE (comp_list->priv->model),
		COL_ACTIVE, sort_component_active,
		NULL, NULL);
	gtk_tree_sortable_set_sort_func (
		GTK_TREE_SORTABLE (comp_list->priv->model),
		COL_NAME, sort_component_name,
		NULL, NULL);
	gtk_tree_sortable_set_sort_func (
		GTK_TREE_SORTABLE (comp_list->priv->model),
		COL_TYPE, sort_component_type,
		NULL, NULL);
	gtk_tree_sortable_set_sort_func (
		GTK_TREE_SORTABLE (comp_list->priv->model),
		COL_IID, sort_component_iid,
		NULL, NULL);
	g_object_unref (comp_list->priv->model);

	/* add columns */
	pixbuf_renderer = gtk_cell_renderer_pixbuf_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Active"),
							   pixbuf_renderer,
							   NULL);
	gtk_tree_view_column_set_cell_data_func (column, pixbuf_renderer,
						 set_pixbuf_func,
						 comp_list, NULL);
	gtk_tree_view_column_set_sort_column_id (column, COL_ACTIVE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (comp_list->priv->list),
				     column);

	text_renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Name"),
							   text_renderer,
							   NULL);
	gtk_tree_view_column_set_cell_data_func (column, text_renderer,
						 set_name_func, NULL, NULL);
	gtk_tree_view_column_set_sort_column_id (column, COL_NAME);
	gtk_tree_view_append_column (GTK_TREE_VIEW (comp_list->priv->list),
				     column);

	text_renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Type"),
							   text_renderer,
							   NULL);
	gtk_tree_view_column_set_cell_data_func (column, text_renderer,
						 set_type_func, NULL, NULL);
	gtk_tree_view_column_set_sort_column_id (column, COL_TYPE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (comp_list->priv->list),
				     column);

	text_renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("IID"),
							   text_renderer,
							   NULL);
	gtk_tree_view_column_set_cell_data_func (column, text_renderer,
						 set_iid_func, NULL, NULL);
	gtk_tree_view_column_set_sort_column_id (column, COL_IID);
	gtk_tree_view_append_column (GTK_TREE_VIEW (comp_list->priv->list),
				     column);
}

static void
component_list_finalize (GObject *object)
{
	ComponentList *comp_list = (ComponentList *) object;

	g_return_if_fail (IS_COMPONENT_LIST (comp_list));

	matecomponent_browser_free_components_list (comp_list->priv->components);

	g_object_unref (comp_list->priv->active_icon);
	g_object_unref (comp_list->priv->inactive_icon);

	g_free (comp_list->priv);
	comp_list->priv = NULL;

	/* chain to parent class */
	parent_class->finalize (object);
}

GType
component_list_get_type (void)
{
	static GType type = 0;

        if (!type) {
                if (type == 0) {
                        static GTypeInfo info = {
                                sizeof (ComponentListClass),
                                (GBaseInitFunc) NULL,
                                (GBaseFinalizeFunc) NULL,
                                (GClassInitFunc) component_list_class_init,
                                NULL, NULL,
                                sizeof (ComponentList),
                                0,
                                (GInstanceInitFunc) component_list_init
                        };
                        type = g_type_register_static (PARENT_TYPE,
						       "ComponentList",
						       &info, 0);
                }
        }

        return type;
}

GtkWidget *
component_list_new (void)
{
	ComponentList *comp_list;

	comp_list = g_object_new (COMPONENT_LIST_TYPE, NULL);
	return GTK_WIDGET (comp_list);
}

gchar *
component_list_get_selected_iid (ComponentList *comp_list)
{
	gchar *iid;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	gboolean isSelected;

	g_assert (IS_COMPONENT_LIST (comp_list));

	selection = gtk_tree_view_get_selection (
		GTK_TREE_VIEW (comp_list->priv->list));

	isSelected = gtk_tree_selection_get_selected (selection,
						      NULL, &iter);

	if (isSelected == FALSE) {
		g_print ("Umm... Should we be here?\n");
		return NULL;
	}

	gtk_tree_model_get (GTK_TREE_MODEL (comp_list->priv->model),
			    &iter, 3, &iid, -1);

	return iid;
}

void
component_list_show (ComponentList *comp_list, gchar *query)
{
	GList *temp;
	GtkListStore *model;

	g_return_if_fail (IS_COMPONENT_LIST (comp_list));

	model = comp_list->priv->model;

	matecomponent_browser_free_components_list (comp_list->priv->components);

	comp_list->priv->components = matecomponent_browser_get_components_list (
		query);

	/* FIXME - Debugging function */
	/* matecomponent_component_list_print (comp_list->priv->components);*/

	gtk_list_store_clear (GTK_LIST_STORE (model));

	for (temp = comp_list->priv->components; temp; temp = temp->next) {
		GtkTreeIter iter;
		MateComponentComponentInfo *info;

		info = (MateComponentComponentInfo *) temp->data;
		gtk_list_store_append (model, &iter);

		gtk_list_store_set (model, &iter,
				    COL_ACTIVE, info->active,
				    COL_NAME, info->component_name,
				    COL_TYPE, info->component_type,
				    COL_IID, info->component_iid,
				    -1);
	}
}
