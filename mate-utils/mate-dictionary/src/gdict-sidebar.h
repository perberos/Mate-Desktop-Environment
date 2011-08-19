/* gdict-sidebar.h - sidebar widget
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

#ifndef __GDICT_SIDEBAR_H__
#define __GDICT_SIDEBAR_H__

#include <gtk/gtk.h>

#define GDICT_TYPE_SIDEBAR		(gdict_sidebar_get_type ())
#define GDICT_SIDEBAR(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GDICT_TYPE_SIDEBAR, GdictSidebar))
#define GDICT_IS_SIDEBAR(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDICT_TYPE_SIDEBAR))
#define GDICT_SIDEBAR_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GDICT_TYPE_SIDEBAR, GdictSidebarClass))
#define GDICT_IS_SIDEBAR_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GDICT_TYPE_SIDEBAR))
#define GDICT_SIDEBAR_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GDICT_TYPE_SIDEBAR, GdictSidebarClass))

typedef struct _GdictSidebar		GdictSidebar;
typedef struct _GdictSidebarPrivate	GdictSidebarPrivate;
typedef struct _GdictSidebarClass	GdictSidebarClass;

struct _GdictSidebar
{
  GtkVBox parent_instance;

  GdictSidebarPrivate *priv;
};

struct _GdictSidebarClass
{
  GtkVBoxClass parent_class;

  void (*page_changed) (GdictSidebar *sidebar);
  void (*closed)       (GdictSidebar *sidebar);

  void (*_gdict_padding_1) (void);
  void (*_gdict_padding_2) (void);
  void (*_gdict_padding_3) (void);
  void (*_gdict_padding_4) (void);
};

GType                 gdict_sidebar_get_type     (void) G_GNUC_CONST;

GtkWidget *           gdict_sidebar_new          (void);
void                  gdict_sidebar_add_page     (GdictSidebar *sidebar,
						  const gchar  *page_id,
						  const gchar  *page_name,
						  GtkWidget    *page_widget);
void                  gdict_sidebar_remove_page  (GdictSidebar *sidebar,
						  const gchar  *page_id);
void                  gdict_sidebar_view_page    (GdictSidebar *sidebar,
						  const gchar  *page_id);
G_CONST_RETURN gchar *gdict_sidebar_current_page (GdictSidebar *sidebar);
gchar **              gdict_sidebar_list_pages   (GdictSidebar *sidebar,
                                                  gsize        *length) G_GNUC_MALLOC;

#endif /* __GDICT_SIDEBAR_H__ */
