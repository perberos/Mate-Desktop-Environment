/* gdict-sidebar.c - sidebar widget
 *
 * Copyright (C) 2006  Emmanuele Bassi <ebassi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *
 * Based on the equivalent widget from Evince
 * 	by Jonathan Blandford,
 * 	Copyright (C) 2004  Red Hat, Inc.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "gdict-sidebar.h"

typedef struct
{
  guint index;

  gchar *id;
  gchar *name;

  GtkWidget *child;
  GtkWidget *menu_item;
} SidebarPage;

#define GDICT_SIDEBAR_GET_PRIVATE(obj)	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), GDICT_TYPE_SIDEBAR, GdictSidebarPrivate))

struct _GdictSidebarPrivate
{
  GHashTable *pages_by_id;
  GSList *pages;

  GtkWidget *hbox;
  GtkWidget *notebook;
  GtkWidget *menu;
  GtkWidget *close_button;
  GtkWidget *label;
  GtkWidget *select_button;
};

enum
{
  PAGE_CHANGED,
  CLOSED,

  LAST_SIGNAL
};

static guint sidebar_signals[LAST_SIGNAL] = { 0 };
static GQuark sidebar_page_id_quark = 0;

G_DEFINE_TYPE (GdictSidebar, gdict_sidebar, GTK_TYPE_VBOX);

SidebarPage *
sidebar_page_new (const gchar *id,
		  const gchar *name,
		  GtkWidget   *widget)
{
  SidebarPage *page;

  page = g_slice_new (SidebarPage);
  
  page->id = g_strdup (id);
  page->name = g_strdup (name);
  page->child = widget;
  page->index = -1;
  page->menu_item = NULL;

  return page;
}

void
sidebar_page_free (SidebarPage *page)
{
  if (G_LIKELY (page))
    {
      g_free (page->name);
      g_free (page->id);
      
      g_slice_free (SidebarPage, page);
    }
}

static void
gdict_sidebar_finalize (GObject *object)
{
  GdictSidebar *sidebar = GDICT_SIDEBAR (object);
  GdictSidebarPrivate *priv = sidebar->priv;

  if (priv->pages_by_id)
    g_hash_table_destroy (priv->pages_by_id);

  if (priv->pages)
    {
      g_slist_foreach (priv->pages, (GFunc) sidebar_page_free, NULL);
      g_slist_free (priv->pages);
    }

  G_OBJECT_CLASS (gdict_sidebar_parent_class)->finalize (object);
}

static void
gdict_sidebar_dispose (GObject *object)
{
  GdictSidebar *sidebar = GDICT_SIDEBAR (object);

  if (sidebar->priv->menu)
    {
      gtk_menu_detach (GTK_MENU (sidebar->priv->menu));
      sidebar->priv->menu = NULL;
    }

  G_OBJECT_CLASS (gdict_sidebar_parent_class)->dispose (object);
}

static void
gdict_sidebar_menu_position_function (GtkMenu  *menu,
				      gint     *x,
				      gint     *y,
				      gboolean *push_in,
				      gpointer  user_data)
{
  GtkWidget *widget;
  GtkAllocation allocation;

  g_assert (GTK_IS_BUTTON (user_data));

  widget = GTK_WIDGET (user_data);

  gdk_window_get_origin (gtk_widget_get_window (widget), x, y);

  gtk_widget_get_allocation (widget, &allocation);
  *x += allocation.x;
  *y += allocation.y + allocation.height;

  *push_in = FALSE;
}

static gboolean
gdict_sidebar_select_button_press_cb (GtkWidget      *widget,
				      GdkEventButton *event,
				      gpointer        user_data)
{
  GdictSidebar *sidebar = GDICT_SIDEBAR (user_data);
  GtkAllocation allocation;

  if (event->button == 1)
    {
      GtkRequisition req;
      gint width;

      gtk_widget_get_allocation (widget, &allocation);
      width = allocation.width;
      gtk_widget_set_size_request (sidebar->priv->menu, -1, -1);
      gtk_widget_size_request (sidebar->priv->menu, &req);
      gtk_widget_set_size_request (sidebar->priv->menu,
		      		   MAX (width, req.width), -1);
      gtk_widget_grab_focus (widget);

      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
      gtk_menu_popup (GTK_MENU (sidebar->priv->menu),
		      NULL, NULL,
		      gdict_sidebar_menu_position_function, widget,
		      event->button, event->time);

      return TRUE;
    }

  return FALSE;
}

static gboolean
gdict_sidebar_select_key_press_cb (GtkWidget   *widget,
				   GdkEventKey *event,
				   gpointer     user_data)
{
  GdictSidebar *sidebar = GDICT_SIDEBAR (user_data);

  if (event->keyval == GDK_space ||
      event->keyval == GDK_KP_Space ||
      event->keyval == GDK_Return ||
      event->keyval == GDK_KP_Enter)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
      gtk_menu_popup (GTK_MENU (sidebar->priv->menu),
		      NULL, NULL,
		      gdict_sidebar_menu_position_function, widget,
		      1, event->time);

      return TRUE;
    }

  return FALSE;
}

static void
gdict_sidebar_close_clicked_cb (GtkWidget *widget,
				gpointer   user_data)
{
  GdictSidebar *sidebar = GDICT_SIDEBAR (user_data);

  g_signal_emit (sidebar, sidebar_signals[CLOSED], 0);
}

static void
gdict_sidebar_menu_deactivate_cb (GtkWidget *widget,
				  gpointer   user_data)
{
  GdictSidebar *sidebar = GDICT_SIDEBAR (user_data);
  GdictSidebarPrivate *priv = sidebar->priv;
  GtkToggleButton *select_button = GTK_TOGGLE_BUTTON (priv->select_button);

  gtk_toggle_button_set_active (select_button, FALSE);
}

static void
gdict_sidebar_menu_detach_cb (GtkWidget *widget,
			      GtkMenu   *menu)
{
  GdictSidebar *sidebar = GDICT_SIDEBAR (widget);

  sidebar->priv->menu = NULL;
}

static void
gdict_sidebar_menu_item_activate (GtkWidget *widget,
				  gpointer   user_data)
{
  GdictSidebar *sidebar = GDICT_SIDEBAR (user_data);
  GdictSidebarPrivate *priv = sidebar->priv;
  GtkWidget *menu_item;
  const gchar *id;
  SidebarPage *page;
  gint current_index;

  menu_item = gtk_menu_get_active (GTK_MENU (priv->menu));
  id = g_object_get_qdata (G_OBJECT (menu_item), sidebar_page_id_quark);
  g_assert (id != NULL);
  
  page = g_hash_table_lookup (priv->pages_by_id, id);
  g_assert (page != NULL);

  current_index = gtk_notebook_get_current_page (GTK_NOTEBOOK (priv->notebook));
  if (current_index == page->index)
    return;

  gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook),
		  		 page->index);
  gtk_label_set_text (GTK_LABEL (priv->label), page->name);

  g_signal_emit (sidebar, sidebar_signals[PAGE_CHANGED], 0);
}

static void
gdict_sidebar_class_init (GdictSidebarClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (gobject_class, sizeof (GdictSidebarPrivate));
  
  sidebar_page_id_quark = g_quark_from_static_string ("gdict-sidebar-page-id");

  gobject_class->finalize = gdict_sidebar_finalize;
  gobject_class->dispose = gdict_sidebar_dispose;

  sidebar_signals[PAGE_CHANGED] =
    g_signal_new ("page-changed",
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GdictSidebarClass, page_changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  sidebar_signals[CLOSED] =
    g_signal_new ("closed",
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GdictSidebarClass, closed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
}

static void
gdict_sidebar_init (GdictSidebar *sidebar)
{
  GdictSidebarPrivate *priv;
  GtkWidget *hbox;
  GtkWidget *select_hbox;
  GtkWidget *select_button;
  GtkWidget *close_button;
  GtkWidget *arrow;

  sidebar->priv = priv = GDICT_SIDEBAR_GET_PRIVATE (sidebar);

  /* we store all the pages inside the list, but we keep
   * a pointer inside the hash table for faster look up
   * times; what's inside the table will be destroyed with
   * the list, so there's no need to supply the destroy
   * functions for keys and values.
   */
  priv->pages = NULL;
  priv->pages_by_id = g_hash_table_new (g_str_hash, g_str_equal);

  /* top option menu */
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (sidebar), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);
  priv->hbox = hbox;

  select_button = gtk_toggle_button_new ();
  gtk_button_set_relief (GTK_BUTTON (select_button), GTK_RELIEF_NONE);
  g_signal_connect (select_button, "button-press-event",
		    G_CALLBACK (gdict_sidebar_select_button_press_cb),
		    sidebar);
  g_signal_connect (select_button, "key-press-event",
		    G_CALLBACK (gdict_sidebar_select_key_press_cb),
		    sidebar);
  priv->select_button = select_button;

  select_hbox = gtk_hbox_new (FALSE, 0);
  
  priv->label = gtk_label_new (NULL);
  gtk_misc_set_alignment (GTK_MISC (priv->label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (select_hbox), priv->label, FALSE, FALSE, 0);
  gtk_widget_show (priv->label);

  arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE);
  gtk_box_pack_end (GTK_BOX (select_hbox), arrow, FALSE, FALSE, 0);
  gtk_widget_show (arrow);

  gtk_container_add (GTK_CONTAINER (select_button), select_hbox);
  gtk_widget_show (select_hbox);

  gtk_box_pack_start (GTK_BOX (hbox), select_button, TRUE, TRUE, 0);
  gtk_widget_show (select_button);

  close_button = gtk_button_new ();
  gtk_button_set_relief (GTK_BUTTON (close_button), GTK_RELIEF_NONE);
  gtk_button_set_image (GTK_BUTTON (close_button),
		        gtk_image_new_from_stock (GTK_STOCK_CLOSE,
						  GTK_ICON_SIZE_SMALL_TOOLBAR));
  g_signal_connect (close_button, "clicked",
		    G_CALLBACK (gdict_sidebar_close_clicked_cb),
		    sidebar);
  gtk_box_pack_end (GTK_BOX (hbox), close_button, FALSE, FALSE, 0);
  gtk_widget_show (close_button);
  priv->close_button = close_button;

  sidebar->priv->menu = gtk_menu_new ();
  g_signal_connect (sidebar->priv->menu, "deactivate",
		    G_CALLBACK (gdict_sidebar_menu_deactivate_cb),
		    sidebar);
  gtk_menu_attach_to_widget (GTK_MENU (sidebar->priv->menu),
		  	     GTK_WIDGET (sidebar),
			     gdict_sidebar_menu_detach_cb);
  gtk_widget_show (sidebar->priv->menu);

  sidebar->priv->notebook = gtk_notebook_new ();
  gtk_notebook_set_show_border (GTK_NOTEBOOK (sidebar->priv->notebook), FALSE);
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (sidebar->priv->notebook), FALSE);
  gtk_box_pack_start (GTK_BOX (sidebar), sidebar->priv->notebook, TRUE, TRUE, 6);
  gtk_widget_show (sidebar->priv->notebook);
}

/*
 * Public API
 */

GtkWidget *
gdict_sidebar_new (void)
{
  return g_object_new (GDICT_TYPE_SIDEBAR, NULL);
}

void
gdict_sidebar_add_page (GdictSidebar *sidebar,
			const gchar  *page_id,
			const gchar  *page_name,
			GtkWidget    *page_widget)
{
  GdictSidebarPrivate *priv;
  SidebarPage *page;
  GtkWidget *menu_item;
  
  g_return_if_fail (GDICT_IS_SIDEBAR (sidebar));
  g_return_if_fail (page_id != NULL);
  g_return_if_fail (page_name != NULL);
  g_return_if_fail (GTK_IS_WIDGET (page_widget));

  priv = sidebar->priv;
  
  if (g_hash_table_lookup (priv->pages_by_id, page_id))
    {
      g_warning ("Attempting to add a page to the sidebar with "
		 "id `%s', but there already is a page with the "
		 "same id.  Aborting...",
		 page_id);
      return;
    }

  /* add the page inside the page list */
  page = sidebar_page_new (page_id, page_name, page_widget);
  
  priv->pages = g_slist_append (priv->pages, page);
  g_hash_table_insert (priv->pages_by_id, page->id, page);

  page->index = gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook),
		  			  page_widget,
					  NULL);

  /* add the menu item for the page */
  menu_item = gtk_image_menu_item_new_with_label (page_name);
  g_object_set_qdata_full (G_OBJECT (menu_item),
			   sidebar_page_id_quark,
                           g_strdup (page_id),
			   (GDestroyNotify) g_free);
  g_signal_connect (menu_item, "activate",
		    G_CALLBACK (gdict_sidebar_menu_item_activate),
		    sidebar);
  gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), menu_item);
  gtk_widget_show (menu_item);
  page->menu_item = menu_item;

  gtk_menu_shell_select_item (GTK_MENU_SHELL (priv->menu), menu_item);
  gtk_label_set_text (GTK_LABEL (priv->label), page_name);
  gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook), page->index);
}

void
gdict_sidebar_remove_page (GdictSidebar *sidebar,
			   const gchar  *page_id)
{
  GdictSidebarPrivate *priv;
  SidebarPage *page;
  GList *children, *l;
  
  g_return_if_fail (GDICT_IS_SIDEBAR (sidebar));
  g_return_if_fail (page_id != NULL);

  priv = sidebar->priv;
  
  if ((page = g_hash_table_lookup (priv->pages_by_id, page_id)) == NULL)
    {
      g_warning ("Attempting to remove a page from the sidebar with "
		 "id `%s', but there is no page with this id. Aborting...",
		 page_id);
      return;
    }

  children = gtk_container_get_children (GTK_CONTAINER (priv->menu));
  for (l = children; l != NULL; l = l->next)
    {
      GtkWidget *menu_item = l->data;

      if (menu_item == page->menu_item)
        {
          gtk_container_remove (GTK_CONTAINER (priv->menu), menu_item);
	  break;
	}
    }
  g_list_free (children);

  gtk_notebook_remove_page (GTK_NOTEBOOK (priv->notebook), page->index);

  g_hash_table_remove (priv->pages_by_id, page->id);
  priv->pages = g_slist_remove (priv->pages, page);

  sidebar_page_free (page);

  /* select the first page, if present */
  page = priv->pages->data;
  if (page)
    {
      gtk_menu_shell_select_item (GTK_MENU_SHELL (priv->menu), page->menu_item);
      gtk_label_set_text (GTK_LABEL (priv->label), page->name);
      gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook), page->index);
    }
  else
    gtk_widget_hide (GTK_WIDGET (sidebar));
}

void
gdict_sidebar_view_page (GdictSidebar *sidebar,
			 const gchar  *page_id)
{
  GdictSidebarPrivate *priv;
  SidebarPage *page;
  
  g_return_if_fail (GDICT_IS_SIDEBAR (sidebar));
  g_return_if_fail (page_id != NULL);

  priv = sidebar->priv;
  page = g_hash_table_lookup (priv->pages_by_id, page_id);
  if (!page)
    return;

  gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook), page->index);
  gtk_label_set_text (GTK_LABEL (priv->label), page->name);
  gtk_menu_shell_select_item (GTK_MENU_SHELL (priv->menu), page->menu_item);
}

G_CONST_RETURN gchar *
gdict_sidebar_current_page (GdictSidebar *sidebar)
{
  GdictSidebarPrivate *priv;
  gint index;
  SidebarPage *page;
  
  g_return_val_if_fail (GDICT_IS_SIDEBAR (sidebar), NULL);

  priv = sidebar->priv;
  
  index = gtk_notebook_get_current_page (GTK_NOTEBOOK (priv->notebook));
  page = g_slist_nth_data (priv->pages, index);
  g_assert (page != NULL);

  return page->id;
}

gchar **
gdict_sidebar_list_pages (GdictSidebar *sidebar,
                          gsize        *length)
{
  GdictSidebarPrivate *priv;
  gchar **retval;
  gint i;
  GSList *l;

  g_return_val_if_fail (GDICT_IS_SIDEBAR (sidebar), NULL);

  priv = sidebar->priv;

  retval = g_new (gchar*, g_slist_length (priv->pages) + 1);
  for (l = priv->pages, i = 0; l; l = l->next, i++)
    retval[i++] = g_strdup (l->data);

  retval[i] = NULL;

  if (length)
    *length = i;

  return retval;
}
