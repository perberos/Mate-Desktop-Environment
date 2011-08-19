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

#ifndef GDU_POOL_TREE_MODEL_H
#define GDU_POOL_TREE_MODEL_H

#include <gdu-gtk/gdu-gtk-types.h>

#define GDU_TYPE_POOL_TREE_MODEL             (gdu_pool_tree_model_get_type ())
#define GDU_POOL_TREE_MODEL(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GDU_TYPE_POOL_TREE_MODEL, GduPoolTreeModel))
#define GDU_POOL_TREE_MODEL_CLASS(obj)       (G_TYPE_CHECK_CLASS_CAST ((obj), GDU_POOL_TREE_MODEL,  GduPoolTreeModelClass))
#define GDU_IS_POOL_TREE_MODEL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDU_TYPE_POOL_TREE_MODEL))
#define GDU_IS_POOL_TREE_MODEL_CLASS(obj)    (G_TYPE_CHECK_CLASS_TYPE ((obj), GDU_TYPE_POOL_TREE_MODEL))
#define GDU_POOL_TREE_MODEL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), GDU_TYPE_POOL_TREE_MODEL, GduPoolTreeModelClass))

typedef struct GduPoolTreeModelClass       GduPoolTreeModelClass;
typedef struct GduPoolTreeModelPrivate     GduPoolTreeModelPrivate;

struct GduPoolTreeModel
{
        GtkTreeStore parent;

        /* private */
        GduPoolTreeModelPrivate *priv;
};

struct GduPoolTreeModelClass
{
        GtkTreeStoreClass parent_class;
};


GType             gdu_pool_tree_model_get_type                 (void) G_GNUC_CONST;
GduPoolTreeModel *gdu_pool_tree_model_new                      (GPtrArray             *pools,
                                                                GduPresentable        *root,
                                                                GduPoolTreeModelFlags  flags);
void              gdu_pool_tree_model_set_pools                (GduPoolTreeModel      *model,
                                                                GPtrArray             *pools);
gboolean          gdu_pool_tree_model_get_iter_for_presentable (GduPoolTreeModel      *model,
                                                                GduPresentable        *presentable,
                                                                GtkTreeIter           *out_iter);

#endif /* GDU_POOL_TREE_MODEL_H */
