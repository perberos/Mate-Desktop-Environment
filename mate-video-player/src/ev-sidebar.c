/* this file is part of evince, a mate document viewer
 *
 *  Copyright (C) 2004 Red Hat, Inc.
 *            (C) 2007 Jan Arne Petersen
 *
 *  Authors:
 *    Jonathan Blandford <jrb@alum.mit.edu>
 *    Jan Arne Petersen <jpetersen@jpetersen.org>
 *
 * Evince is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Evince is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02110-1301  USA.
 *
 * Thursday 03 May 2007: Bastien Nocera: Add exception clause.
 * See license_change file for details.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "ev-sidebar.h"

enum
{
	PAGE_COLUMN_ID,
	PAGE_COLUMN_TITLE,
	PAGE_COLUMN_NUM_COLS
};

struct _EvSidebarPrivate {
	GtkWidget *combobox;
	GtkWidget *notebook;
};

enum {
	CLOSED,
	LAST_SIGNAL
};

static int ev_sidebar_table_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (EvSidebar, ev_sidebar, GTK_TYPE_VBOX)

#define EV_SIDEBAR_GET_PRIVATE(object) \
		(G_TYPE_INSTANCE_GET_PRIVATE ((object), EV_TYPE_SIDEBAR, EvSidebarPrivate))

static void
ev_sidebar_destroy (GtkObject *object)
{
	(* GTK_OBJECT_CLASS (ev_sidebar_parent_class)->destroy) (object);
}

static void
ev_sidebar_class_init (EvSidebarClass *ev_sidebar_class)
{
	GObjectClass *g_object_class;
	GtkWidgetClass *widget_class;
	GtkObjectClass *gtk_object_klass;
 
	g_object_class = G_OBJECT_CLASS (ev_sidebar_class);
	widget_class = GTK_WIDGET_CLASS (ev_sidebar_class);
	gtk_object_klass = GTK_OBJECT_CLASS (ev_sidebar_class);
	   
	g_type_class_add_private (g_object_class, sizeof (EvSidebarPrivate));
	   
	gtk_object_klass->destroy = ev_sidebar_destroy;

	ev_sidebar_table_signals[CLOSED] =
		g_signal_new ("closed",
				G_TYPE_FROM_CLASS (g_object_class),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (EvSidebarClass, closed),
				NULL, NULL,
				g_cclosure_marshal_VOID__VOID,
				G_TYPE_NONE, 0);

}

static void
ev_sidebar_close_clicked_cb (GtkWidget *widget,
			     gpointer   user_data)
{
	EvSidebar *ev_sidebar = EV_SIDEBAR (user_data);

	g_signal_emit (G_OBJECT (ev_sidebar),
			ev_sidebar_table_signals[CLOSED], 0, NULL);
	gtk_widget_hide (GTK_WIDGET (ev_sidebar));
}

static void
ev_sidebar_combobox_changed_cb (GtkComboBox *combo_box,
				gpointer   user_data)
{
	EvSidebar *ev_sidebar = EV_SIDEBAR (user_data);
	GtkTreeModel *model;
	GtkTreeIter iter;

	model = gtk_combo_box_get_model (combo_box);

	if (gtk_combo_box_get_active_iter (combo_box, &iter)) {
		GtkTreePath *path;
		gint *indices;

		path = gtk_tree_model_get_path (model, &iter);
		indices = gtk_tree_path_get_indices (path);

		if (indices != NULL) {
			gtk_notebook_set_current_page (GTK_NOTEBOOK (ev_sidebar->priv->notebook), indices[0]);
		}

		gtk_tree_path_free (path);
	}
}

static void
ev_sidebar_init (EvSidebar *ev_sidebar)
{
	GtkTreeModel *page_model;
	GtkWidget *vbox, *hbox;
	GtkWidget *close_button;
	GtkCellRenderer *renderer;
	GtkWidget *image;

	ev_sidebar->priv = EV_SIDEBAR_GET_PRIVATE (ev_sidebar);

	/* data model */
	page_model = (GtkTreeModel *)
			gtk_list_store_new (PAGE_COLUMN_NUM_COLS,
					    G_TYPE_STRING,
					    G_TYPE_STRING,
					    GTK_TYPE_WIDGET,
					    G_TYPE_INT);

	/* create a 6 6 6 0 border with GtkBoxes */
	hbox = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (ev_sidebar), hbox, TRUE, TRUE, 6);

	vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), gtk_vbox_new (FALSE, 0), FALSE, FALSE, 0);

	/* top option menu */
	hbox = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);

	ev_sidebar->priv->combobox = gtk_combo_box_new_with_model (page_model);
	g_signal_connect (ev_sidebar->priv->combobox, "changed",
			  G_CALLBACK (ev_sidebar_combobox_changed_cb),
			  ev_sidebar);

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (ev_sidebar->priv->combobox), renderer, TRUE);
	gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (ev_sidebar->priv->combobox), renderer, "text", PAGE_COLUMN_TITLE);

	gtk_box_pack_start (GTK_BOX (hbox), ev_sidebar->priv->combobox, TRUE, TRUE, 0);
	gtk_widget_show (ev_sidebar->priv->combobox);

	g_object_unref (G_OBJECT (page_model));

	close_button = gtk_button_new ();
	gtk_button_set_relief (GTK_BUTTON (close_button), GTK_RELIEF_NONE);
	g_signal_connect (close_button, "clicked",
			  G_CALLBACK (ev_sidebar_close_clicked_cb),
			  ev_sidebar);

	image = gtk_image_new_from_stock (GTK_STOCK_CLOSE,
					  GTK_ICON_SIZE_MENU);
	gtk_container_add (GTK_CONTAINER (close_button), image);
	gtk_widget_show (image);
   
	gtk_box_pack_end (GTK_BOX (hbox), close_button, FALSE, FALSE, 0);
	gtk_widget_show (close_button);
   
	ev_sidebar->priv->notebook = gtk_notebook_new ();
	gtk_notebook_set_show_border (GTK_NOTEBOOK (ev_sidebar->priv->notebook), FALSE);
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (ev_sidebar->priv->notebook), FALSE);
	gtk_box_pack_start (GTK_BOX (vbox), ev_sidebar->priv->notebook,
			    TRUE, TRUE, 0);
	gtk_widget_show (ev_sidebar->priv->notebook);
}

/* Public functions */

GtkWidget *
ev_sidebar_new (void)
{
	GtkWidget *ev_sidebar;

	ev_sidebar = g_object_new (EV_TYPE_SIDEBAR, NULL);

	return ev_sidebar;
}

/* NOTE: Return values from this have to be g_free()d */
char *
ev_sidebar_get_current_page (EvSidebar *ev_sidebar)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	char *id;

	g_return_val_if_fail (EV_IS_SIDEBAR (ev_sidebar), NULL);
	g_return_val_if_fail (ev_sidebar->priv != NULL, NULL);

	model = gtk_combo_box_get_model (GTK_COMBO_BOX (ev_sidebar->priv->combobox));

	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (ev_sidebar->priv->combobox), &iter)) {
		gtk_tree_model_get (model, &iter, PAGE_COLUMN_ID, &id, -1);

		return id;
	}

	return NULL;
}

static gboolean
ev_sidebar_get_iter_for_page_id (EvSidebar *ev_sidebar,
				 const char *new_page_id,
				 GtkTreeIter *iter)
{
	GtkTreeModel *model;
	gboolean valid;
	gchar *page_id;

	g_return_val_if_fail (EV_IS_SIDEBAR (ev_sidebar), FALSE);
	g_return_val_if_fail (ev_sidebar->priv != NULL, FALSE);
	g_return_val_if_fail (iter != NULL, FALSE);

	model = gtk_combo_box_get_model (GTK_COMBO_BOX (ev_sidebar->priv->combobox));

	valid = gtk_tree_model_get_iter_first (model, iter);

	while (valid) {
		gtk_tree_model_get (model, iter, PAGE_COLUMN_ID, &page_id, -1);

		if (page_id != NULL && strcmp (new_page_id, page_id) == 0) {
			g_free (page_id);
			return TRUE;
		}
		g_free (page_id);

		valid = gtk_tree_model_iter_next (model, iter);
	}

	return FALSE;
}

void
ev_sidebar_set_current_page (EvSidebar *ev_sidebar, const char *new_page_id)
{
	GtkTreeIter iter;

	g_return_if_fail (EV_IS_SIDEBAR (ev_sidebar));
	g_return_if_fail (new_page_id != NULL);


	if (ev_sidebar_get_iter_for_page_id (ev_sidebar, new_page_id, &iter)) {
		gtk_combo_box_set_active_iter (GTK_COMBO_BOX (ev_sidebar->priv->combobox), &iter);
	}
}

void
ev_sidebar_add_page (EvSidebar   *ev_sidebar,
		     const gchar *page_id,
		     const gchar *title,
		     GtkWidget   *main_widget)
{
	GtkTreeIter iter, iter2;
	GtkTreeModel *model;
	   
	g_return_if_fail (EV_IS_SIDEBAR (ev_sidebar));
	g_return_if_fail (page_id != NULL);
	g_return_if_fail (title != NULL);
	g_return_if_fail (GTK_IS_WIDGET (main_widget));

	model = gtk_combo_box_get_model (GTK_COMBO_BOX (ev_sidebar->priv->combobox));

	gtk_widget_set_sensitive (GTK_WIDGET (ev_sidebar), TRUE);

	gtk_widget_show (main_widget);	
	gtk_notebook_append_page (GTK_NOTEBOOK (ev_sidebar->priv->notebook), main_widget, NULL);

	gtk_list_store_append (GTK_LIST_STORE (model), &iter);
	gtk_list_store_set (GTK_LIST_STORE (model), &iter,
			    PAGE_COLUMN_ID, page_id,
			    PAGE_COLUMN_TITLE, title,
			    -1);

	if (!gtk_combo_box_get_active_iter (GTK_COMBO_BOX (ev_sidebar->priv->combobox), &iter2)) {
		gtk_combo_box_set_active_iter (GTK_COMBO_BOX (ev_sidebar->priv->combobox), &iter);
	}
}

void
ev_sidebar_remove_page (EvSidebar   *ev_sidebar,
			const gchar *page_id)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	   
	g_return_if_fail (EV_IS_SIDEBAR (ev_sidebar));
	g_return_if_fail (page_id != NULL);

	model = gtk_combo_box_get_model (GTK_COMBO_BOX (ev_sidebar->priv->combobox));

	if (ev_sidebar_get_iter_for_page_id (ev_sidebar, page_id, &iter)) {
		GtkTreePath *path;
		gint *indices;

		path = gtk_tree_model_get_path (model, &iter);
		indices = gtk_tree_path_get_indices (path);

		g_assert (indices != NULL);
		gtk_notebook_remove_page (GTK_NOTEBOOK (ev_sidebar->priv->notebook), indices[0]);

		gtk_list_store_remove (GTK_LIST_STORE (model), &iter);

		if (gtk_tree_model_iter_n_children (model, NULL) != 0) {
			gtk_tree_path_prev (path);

			if (gtk_tree_model_get_iter (model, &iter, path)) {
				gtk_combo_box_set_active_iter (GTK_COMBO_BOX (ev_sidebar->priv->combobox), &iter);
			}
		} else {
			gtk_widget_set_sensitive (GTK_WIDGET (ev_sidebar), FALSE);
		}

		gtk_tree_path_free (path);
	}
}

