/* Eye of Mate - Side bar
 *
 * Copyright (C) 2004 Red Hat, Inc.
 * Copyright (C) 2007 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@mate.org>
 *
 * Based on evince code (shell/ev-sidebar.c) by:
 * 	- Jonathan Blandford <jrb@alum.mit.edu>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "eog-sidebar.h"

enum {
	PROP_0,
	PROP_CURRENT_PAGE
};

enum {
	PAGE_COLUMN_TITLE,
	PAGE_COLUMN_MENU_ITEM,
	PAGE_COLUMN_MAIN_WIDGET,
	PAGE_COLUMN_NOTEBOOK_INDEX,
	PAGE_COLUMN_NUM_COLS
};

enum {
	SIGNAL_PAGE_ADDED,
	SIGNAL_PAGE_REMOVED,
	SIGNAL_LAST
};

static gint signals[SIGNAL_LAST];

struct _EogSidebarPrivate {
	GtkWidget *notebook;
	GtkWidget *select_button;
	GtkWidget *menu;
	GtkWidget *hbox;
	GtkWidget *label;

	GtkTreeModel *page_model;
};

G_DEFINE_TYPE (EogSidebar, eog_sidebar, GTK_TYPE_VBOX)

#define EOG_SIDEBAR_GET_PRIVATE(object) \
	(G_TYPE_INSTANCE_GET_PRIVATE ((object), EOG_TYPE_SIDEBAR, EogSidebarPrivate))

static void
eog_sidebar_destroy (GtkObject *object)
{
	EogSidebar *eog_sidebar = EOG_SIDEBAR (object);

	if (eog_sidebar->priv->menu) {
		gtk_menu_detach (GTK_MENU (eog_sidebar->priv->menu));
		eog_sidebar->priv->menu = NULL;
	}

	if (eog_sidebar->priv->page_model) {
		g_object_unref (eog_sidebar->priv->page_model);
		eog_sidebar->priv->page_model = NULL;
	}

	(* GTK_OBJECT_CLASS (eog_sidebar_parent_class)->destroy) (object);
}

static void
eog_sidebar_select_page (EogSidebar *eog_sidebar, GtkTreeIter *iter)
{
	gchar *title;
	gint index;

	gtk_tree_model_get (eog_sidebar->priv->page_model, iter,
			    PAGE_COLUMN_TITLE, &title,
			    PAGE_COLUMN_NOTEBOOK_INDEX, &index,
			    -1);

	gtk_notebook_set_current_page (GTK_NOTEBOOK (eog_sidebar->priv->notebook), index);
	gtk_label_set_text (GTK_LABEL (eog_sidebar->priv->label), title);

	g_free (title);
}

void
eog_sidebar_set_page (EogSidebar   *eog_sidebar,
		     GtkWidget   *main_widget)
{
	GtkTreeIter iter;
	gboolean valid;

	valid = gtk_tree_model_get_iter_first (eog_sidebar->priv->page_model, &iter);

	while (valid) {
		GtkWidget *widget;

		gtk_tree_model_get (eog_sidebar->priv->page_model, &iter,
				    PAGE_COLUMN_MAIN_WIDGET, &widget,
				    -1);

		if (widget == main_widget) {
			eog_sidebar_select_page (eog_sidebar, &iter);
			valid = FALSE;
		} else {
			valid = gtk_tree_model_iter_next (eog_sidebar->priv->page_model, &iter);
		}

		g_object_unref (widget);
	}

	g_object_notify (G_OBJECT (eog_sidebar), "current-page");
}

static GtkWidget *
eog_sidebar_get_current_page (EogSidebar *sidebar)
{
	GtkNotebook *notebook = GTK_NOTEBOOK (sidebar->priv->notebook);

	return gtk_notebook_get_nth_page
		(notebook, gtk_notebook_get_current_page (notebook));
}

static void
eog_sidebar_set_property (GObject     *object,
		         guint         prop_id,
		         const GValue *value,
		         GParamSpec   *pspec)
{
	EogSidebar *sidebar = EOG_SIDEBAR (object);

	switch (prop_id) {
	case PROP_CURRENT_PAGE:
		eog_sidebar_set_page (sidebar, g_value_get_object (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
eog_sidebar_get_property (GObject    *object,
		          guint       prop_id,
		          GValue     *value,
		          GParamSpec *pspec)
{
	EogSidebar *sidebar = EOG_SIDEBAR (object);

	switch (prop_id) {
	case PROP_CURRENT_PAGE:
		g_value_set_object (value, eog_sidebar_get_current_page (sidebar));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
eog_sidebar_class_init (EogSidebarClass *eog_sidebar_class)
{
	GObjectClass *g_object_class;
	GtkWidgetClass *widget_class;
	GtkObjectClass *gtk_object_klass;

	g_object_class = G_OBJECT_CLASS (eog_sidebar_class);
	widget_class = GTK_WIDGET_CLASS (eog_sidebar_class);
	gtk_object_klass = GTK_OBJECT_CLASS (eog_sidebar_class);

	g_type_class_add_private (g_object_class, sizeof (EogSidebarPrivate));

	gtk_object_klass->destroy = eog_sidebar_destroy;
	g_object_class->get_property = eog_sidebar_get_property;
	g_object_class->set_property = eog_sidebar_set_property;

	g_object_class_install_property (g_object_class,
					 PROP_CURRENT_PAGE,
					 g_param_spec_object ("current-page",
							      "Current page",
							      "The currently visible page",
							      GTK_TYPE_WIDGET,
							      G_PARAM_READWRITE));

	signals[SIGNAL_PAGE_ADDED] =
		g_signal_new ("page-added",
			      EOG_TYPE_SIDEBAR,
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (EogSidebarClass, page_added),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1,
			      GTK_TYPE_WIDGET);

	signals[SIGNAL_PAGE_REMOVED] =
		g_signal_new ("page-removed",
			      EOG_TYPE_SIDEBAR,
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (EogSidebarClass, page_removed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1,
			      GTK_TYPE_WIDGET);
}

static void
eog_sidebar_menu_position_under (GtkMenu  *menu,
				 gint     *x,
				 gint     *y,
				 gboolean *push_in,
				 gpointer  user_data)
{
	GtkWidget *widget;
	GtkAllocation allocation;

	g_return_if_fail (GTK_IS_BUTTON (user_data));
	g_return_if_fail (!gtk_widget_get_has_window (user_data));

	widget = GTK_WIDGET (user_data);
	gtk_widget_get_allocation (widget, &allocation);

	gdk_window_get_origin (gtk_widget_get_window (widget), x, y);

	*x += allocation.x;
	*y += allocation.y + allocation.height;

	*push_in = FALSE;
}

static gboolean
eog_sidebar_select_button_press_cb (GtkWidget      *widget,
				    GdkEventButton *event,
				    gpointer        user_data)
{
	EogSidebar *eog_sidebar = EOG_SIDEBAR (user_data);

	if (event->button == 1) {
		GtkRequisition requisition;
		GtkAllocation allocation;

		gtk_widget_get_allocation (widget, &allocation);

		gtk_widget_set_size_request (eog_sidebar->priv->menu, -1, -1);
		gtk_widget_size_request (eog_sidebar->priv->menu, &requisition);
		gtk_widget_set_size_request (eog_sidebar->priv->menu,
					     MAX (allocation.width,
						  requisition.width), -1);

		gtk_widget_grab_focus (widget);

		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);

		gtk_menu_popup (GTK_MENU (eog_sidebar->priv->menu),
				NULL, NULL, eog_sidebar_menu_position_under, widget,
				event->button, event->time);

		return TRUE;
	}

	return FALSE;
}

static gboolean
eog_sidebar_select_button_key_press_cb (GtkWidget   *widget,
				        GdkEventKey *event,
				        gpointer     user_data)
{
	EogSidebar *eog_sidebar = EOG_SIDEBAR (user_data);

	if (event->keyval == GDK_space ||
	    event->keyval == GDK_KP_Space ||
	    event->keyval == GDK_Return ||
	    event->keyval == GDK_KP_Enter) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);

		gtk_menu_popup (GTK_MENU (eog_sidebar->priv->menu),
			        NULL, NULL, eog_sidebar_menu_position_under, widget,
				1, event->time);

		return TRUE;
	}

	return FALSE;
}

static void
eog_sidebar_close_clicked_cb (GtkWidget *widget,
 			      gpointer   user_data)
{
	EogSidebar *eog_sidebar = EOG_SIDEBAR (user_data);

	gtk_widget_hide (GTK_WIDGET (eog_sidebar));
}

static void
eog_sidebar_menu_deactivate_cb (GtkWidget *widget,
			       gpointer   user_data)
{
	GtkWidget *menu_button;

	menu_button = GTK_WIDGET (user_data);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (menu_button), FALSE);
}

static void
eog_sidebar_menu_detach_cb (GtkWidget *widget,
			   GtkMenu   *menu)
{
	EogSidebar *eog_sidebar = EOG_SIDEBAR (widget);

	eog_sidebar->priv->menu = NULL;
}

static void
eog_sidebar_menu_item_activate_cb (GtkWidget *widget,
				   gpointer   user_data)
{
	EogSidebar *eog_sidebar = EOG_SIDEBAR (user_data);
	GtkTreeIter iter;
	GtkWidget *menu_item, *item;
	gboolean valid;

	menu_item = gtk_menu_get_active (GTK_MENU (eog_sidebar->priv->menu));
	valid = gtk_tree_model_get_iter_first (eog_sidebar->priv->page_model, &iter);

	while (valid) {
		gtk_tree_model_get (eog_sidebar->priv->page_model, &iter,
				    PAGE_COLUMN_MENU_ITEM, &item,
				    -1);

		if (item == menu_item) {
			eog_sidebar_select_page (eog_sidebar, &iter);
			valid = FALSE;
		} else {
			valid = gtk_tree_model_iter_next (eog_sidebar->priv->page_model, &iter);
		}

		g_object_unref (item);
	}

	g_object_notify (G_OBJECT (eog_sidebar), "current-page");
}

static void
eog_sidebar_init (EogSidebar *eog_sidebar)
{
	GtkWidget *hbox;
	GtkWidget *close_button;
	GtkWidget *select_hbox;
	GtkWidget *arrow;
	GtkWidget *image;

	eog_sidebar->priv = EOG_SIDEBAR_GET_PRIVATE (eog_sidebar);

	/* data model */
	eog_sidebar->priv->page_model = (GtkTreeModel *)
			gtk_list_store_new (PAGE_COLUMN_NUM_COLS,
					    G_TYPE_STRING,
					    GTK_TYPE_WIDGET,
					    GTK_TYPE_WIDGET,
					    G_TYPE_INT);

	/* top option menu */
	hbox = gtk_hbox_new (FALSE, 0);
	eog_sidebar->priv->hbox = hbox;
	gtk_box_pack_start (GTK_BOX (eog_sidebar), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);

	eog_sidebar->priv->select_button = gtk_toggle_button_new ();
	gtk_button_set_relief (GTK_BUTTON (eog_sidebar->priv->select_button),
			       GTK_RELIEF_NONE);

	g_signal_connect (eog_sidebar->priv->select_button, "button_press_event",
			  G_CALLBACK (eog_sidebar_select_button_press_cb),
			  eog_sidebar);

	g_signal_connect (eog_sidebar->priv->select_button, "key_press_event",
			  G_CALLBACK (eog_sidebar_select_button_key_press_cb),
			  eog_sidebar);

	select_hbox = gtk_hbox_new (FALSE, 0);

	eog_sidebar->priv->label = gtk_label_new ("");

	gtk_box_pack_start (GTK_BOX (select_hbox),
			    eog_sidebar->priv->label,
			    FALSE, FALSE, 0);

	gtk_widget_show (eog_sidebar->priv->label);

	arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE);
	gtk_box_pack_end (GTK_BOX (select_hbox), arrow, FALSE, FALSE, 0);
	gtk_widget_show (arrow);

	gtk_container_add (GTK_CONTAINER (eog_sidebar->priv->select_button), select_hbox);
	gtk_widget_show (select_hbox);

	gtk_box_pack_start (GTK_BOX (hbox), eog_sidebar->priv->select_button, TRUE, TRUE, 0);
	gtk_widget_show (eog_sidebar->priv->select_button);

	close_button = gtk_button_new ();

	gtk_button_set_relief (GTK_BUTTON (close_button), GTK_RELIEF_NONE);

	g_signal_connect (close_button, "clicked",
			  G_CALLBACK (eog_sidebar_close_clicked_cb),
			  eog_sidebar);

	image = gtk_image_new_from_stock (GTK_STOCK_CLOSE,
					  GTK_ICON_SIZE_MENU);
	gtk_container_add (GTK_CONTAINER (close_button), image);
	gtk_widget_show (image);

	gtk_box_pack_end (GTK_BOX (hbox), close_button, FALSE, FALSE, 0);
	gtk_widget_show (close_button);

	eog_sidebar->priv->menu = gtk_menu_new ();

	g_signal_connect (eog_sidebar->priv->menu, "deactivate",
			  G_CALLBACK (eog_sidebar_menu_deactivate_cb),
			  eog_sidebar->priv->select_button);

	gtk_menu_attach_to_widget (GTK_MENU (eog_sidebar->priv->menu),
				   GTK_WIDGET (eog_sidebar),
				   eog_sidebar_menu_detach_cb);

	gtk_widget_show (eog_sidebar->priv->menu);

	eog_sidebar->priv->notebook = gtk_notebook_new ();

	gtk_notebook_set_show_border (GTK_NOTEBOOK (eog_sidebar->priv->notebook), FALSE);
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (eog_sidebar->priv->notebook), FALSE);

	gtk_box_pack_start (GTK_BOX (eog_sidebar), eog_sidebar->priv->notebook,
			    TRUE, TRUE, 0);

	gtk_widget_show (eog_sidebar->priv->notebook);
}

GtkWidget *
eog_sidebar_new (void)
{
	GtkWidget *eog_sidebar;

	eog_sidebar = g_object_new (EOG_TYPE_SIDEBAR, NULL);

	return eog_sidebar;
}

void
eog_sidebar_add_page (EogSidebar   *eog_sidebar,
		      const gchar  *title,
		      GtkWidget    *main_widget)
{
	GtkTreeIter iter;
	GtkWidget *menu_item;
	gchar *label_title;
	gint index;

	g_return_if_fail (EOG_IS_SIDEBAR (eog_sidebar));
	g_return_if_fail (GTK_IS_WIDGET (main_widget));

	index = gtk_notebook_append_page (GTK_NOTEBOOK (eog_sidebar->priv->notebook),
					  main_widget, NULL);

	menu_item = gtk_image_menu_item_new_with_label (title);

	g_signal_connect (menu_item, "activate",
			  G_CALLBACK (eog_sidebar_menu_item_activate_cb),
			  eog_sidebar);

	gtk_widget_show (menu_item);

	gtk_menu_shell_append (GTK_MENU_SHELL (eog_sidebar->priv->menu),
			       menu_item);

	/* Insert and move to end */
	gtk_list_store_insert_with_values (GTK_LIST_STORE (eog_sidebar->priv->page_model),
					   &iter, 0,
					   PAGE_COLUMN_TITLE, title,
					   PAGE_COLUMN_MENU_ITEM, menu_item,
					   PAGE_COLUMN_MAIN_WIDGET, main_widget,
					   PAGE_COLUMN_NOTEBOOK_INDEX, index,
					   -1);

	gtk_list_store_move_before (GTK_LIST_STORE(eog_sidebar->priv->page_model),
				    &iter,
				    NULL);

	/* Set the first item added as active */
	gtk_tree_model_get_iter_first (eog_sidebar->priv->page_model, &iter);
	gtk_tree_model_get (eog_sidebar->priv->page_model,
			    &iter,
			    PAGE_COLUMN_TITLE, &label_title,
			    PAGE_COLUMN_NOTEBOOK_INDEX, &index,
			    -1);

	gtk_menu_set_active (GTK_MENU (eog_sidebar->priv->menu), index);

	gtk_label_set_text (GTK_LABEL (eog_sidebar->priv->label), label_title);

	gtk_notebook_set_current_page (GTK_NOTEBOOK (eog_sidebar->priv->notebook),
				       index);

	g_free (label_title);

	g_signal_emit (G_OBJECT (eog_sidebar),
		       signals[SIGNAL_PAGE_ADDED], 0, main_widget);
}

void
eog_sidebar_remove_page (EogSidebar *eog_sidebar, GtkWidget *main_widget)
{
	GtkTreeIter iter;
	GtkWidget *widget, *menu_item;
	gboolean valid;
	gint index;

	g_return_if_fail (EOG_IS_SIDEBAR (eog_sidebar));
	g_return_if_fail (GTK_IS_WIDGET (main_widget));

	valid = gtk_tree_model_get_iter_first (eog_sidebar->priv->page_model, &iter);

	while (valid) {
		gtk_tree_model_get (eog_sidebar->priv->page_model, &iter,
				    PAGE_COLUMN_NOTEBOOK_INDEX, &index,
				    PAGE_COLUMN_MENU_ITEM, &menu_item,
				    PAGE_COLUMN_MAIN_WIDGET, &widget,
				    -1);

		if (widget == main_widget) {
			break;
		} else {
			valid = gtk_tree_model_iter_next (eog_sidebar->priv->page_model,
							  &iter);
		}

		g_object_unref (menu_item);
		g_object_unref (widget);
	}

	if (valid) {
		gtk_notebook_remove_page (GTK_NOTEBOOK (eog_sidebar->priv->notebook),
					  index);

		gtk_container_remove (GTK_CONTAINER (eog_sidebar->priv->menu), menu_item);

		gtk_list_store_remove (GTK_LIST_STORE (eog_sidebar->priv->page_model),
				       &iter);

		g_signal_emit (G_OBJECT (eog_sidebar),
			       signals[SIGNAL_PAGE_REMOVED], 0, main_widget);
	}
}

gint
eog_sidebar_get_n_pages (EogSidebar *eog_sidebar)
{
	g_return_val_if_fail (EOG_IS_SIDEBAR (eog_sidebar), TRUE);

	return gtk_tree_model_iter_n_children (
		GTK_TREE_MODEL (eog_sidebar->priv->page_model), NULL);
}

gboolean
eog_sidebar_is_empty (EogSidebar *eog_sidebar)
{
	g_return_val_if_fail (EOG_IS_SIDEBAR (eog_sidebar), TRUE);

	return gtk_tree_model_iter_n_children (
		GTK_TREE_MODEL (eog_sidebar->priv->page_model), NULL) == 0;
}
