/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2009 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * Author: David Zeuthen <davidz@redhat.com>
 */

#if !defined (__GDU_GTK_INSIDE_GDU_GTK_H) && !defined (GDU_GTK_COMPILATION)
#error "Only <gdu-gtk/gdu-gtk.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef GDU_POOL_TREE_VIEW_H
#define GDU_POOL_TREE_VIEW_H

#include <gdu-gtk/gdu-gtk-types.h>

#define GDU_TYPE_POOL_TREE_VIEW             (gdu_pool_tree_view_get_type ())
#define GDU_POOL_TREE_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GDU_TYPE_POOL_TREE_VIEW, GduPoolTreeView))
#define GDU_POOL_TREE_VIEW_CLASS(obj)       (G_TYPE_CHECK_CLASS_CAST ((obj), GDU_POOL_TREE_VIEW,  GduPoolTreeViewClass))
#define GDU_IS_POOL_TREE_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDU_TYPE_POOL_TREE_VIEW))
#define GDU_IS_POOL_TREE_VIEW_CLASS(obj)    (G_TYPE_CHECK_CLASS_TYPE ((obj), GDU_TYPE_POOL_TREE_VIEW))
#define GDU_POOL_TREE_VIEW_GET_CLASS        (G_TYPE_INSTANCE_GET_CLASS ((obj), GDU_TYPE_POOL_TREE_VIEW, GduPoolTreeViewClass))

typedef struct GduPoolTreeViewClass       GduPoolTreeViewClass;
typedef struct GduPoolTreeViewPrivate     GduPoolTreeViewPrivate;

struct GduPoolTreeView
{
        GtkTreeView parent;

        /*< private >*/
        GduPoolTreeViewPrivate *priv;
};

struct GduPoolTreeViewClass
{
        GtkTreeViewClass parent_class;
};

GType            gdu_pool_tree_view_get_type                 (void) G_GNUC_CONST;
GtkWidget       *gdu_pool_tree_view_new                      (GduPoolTreeModel     *model,
                                                              GduPoolTreeViewFlags  flags);
GduPresentable  *gdu_pool_tree_view_get_selected_presentable (GduPoolTreeView      *view);
void             gdu_pool_tree_view_select_presentable       (GduPoolTreeView      *view,
                                                              GduPresentable       *presentable);
void             gdu_pool_tree_view_select_first_presentable (GduPoolTreeView      *view);

#endif /* GDU_POOL_TREE_VIEW_H */
