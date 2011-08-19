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

#include "config.h"
#include <glib/gi18n-lib.h>

#include <string.h>

#include "gdu-gtk.h"
#include "gdu-pool-tree-model.h"

struct GduPoolTreeModelPrivate
{
        GPtrArray *pools;
        GduPresentable *root;
        GduPoolTreeModelFlags flags;
        gboolean constructed;
};

G_DEFINE_TYPE (GduPoolTreeModel, gdu_pool_tree_model, GTK_TYPE_TREE_STORE)

enum
{
        PROP_0,
        PROP_ROOT,
        PROP_POOLS,
        PROP_FLAGS,
};

/* ---------------------------------------------------------------------------------------------------- */

static void on_presentable_added   (GduPool          *pool,
                                    GduPresentable   *presentable,
                                    gpointer          user_data);
static void on_presentable_removed (GduPool          *pool,
                                    GduPresentable   *presentable,
                                    gpointer          user_data);
static void on_presentable_changed (GduPool          *pool,
                                    GduPresentable   *presentable,
                                    gpointer          user_data);
static void add_presentable        (GduPoolTreeModel *model,
                                    GduPresentable   *presentable,
                                    GtkTreeIter      *iter_out);

/* ---------------------------------------------------------------------------------------------------- */

static void
disconnect_from_pool_cb (GduPool  *pool,
                         gpointer  user_data)
{
        GduPoolTreeModel *model = GDU_POOL_TREE_MODEL (user_data);
        g_signal_handlers_disconnect_by_func (pool, on_presentable_added, model);
        g_signal_handlers_disconnect_by_func (pool, on_presentable_removed, model);
        g_signal_handlers_disconnect_by_func (pool, on_presentable_changed, model);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
gdu_pool_tree_model_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
        GduPoolTreeModel *model = GDU_POOL_TREE_MODEL (object);

        switch (prop_id) {
        case PROP_POOLS:
                gdu_pool_tree_model_set_pools (model, g_value_get_boxed (value));
                break;

        case PROP_ROOT:
                if (g_value_get_object (value) != NULL)
                        model->priv->root = g_value_dup_object (value);
                break;

        case PROP_FLAGS:
                model->priv->flags = g_value_get_flags (value);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gdu_pool_tree_model_get_property (GObject     *object,
                                  guint        prop_id,
                                  GValue      *value,
                                  GParamSpec  *pspec)
{
        GduPoolTreeModel *model = GDU_POOL_TREE_MODEL (object);

        switch (prop_id) {
        case PROP_POOLS:
                g_value_set_boxed (value, model->priv->pools);
                break;

        case PROP_ROOT:
                g_value_set_object (value, model->priv->root);
                break;

        case PROP_FLAGS:
                g_value_set_flags (value, model->priv->flags);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
    }
}

static void
gdu_pool_tree_model_finalize (GObject *object)
{
        GduPoolTreeModel *model = GDU_POOL_TREE_MODEL (object);

        if (model->priv->pools != NULL) {
                g_ptr_array_foreach (model->priv->pools, (GFunc) disconnect_from_pool_cb, model);
                g_ptr_array_unref (model->priv->pools);
        }

        if (G_OBJECT_CLASS (gdu_pool_tree_model_parent_class)->finalize != NULL)
                G_OBJECT_CLASS (gdu_pool_tree_model_parent_class)->finalize (object);
}

static gint
presentable_sort_func (GtkTreeModel *model,
                       GtkTreeIter  *a,
                       GtkTreeIter  *b,
                       gpointer      userdata)
{
        GduPresentable *p1;
        GduPresentable *p2;
        gint result;

        result = 0;

        gtk_tree_model_get (model, a, GDU_POOL_TREE_MODEL_COLUMN_PRESENTABLE, &p1, -1);
        gtk_tree_model_get (model, b, GDU_POOL_TREE_MODEL_COLUMN_PRESENTABLE, &p2, -1);
        if (p1 == NULL || p2 == NULL)
                goto out;

        result = gdu_presentable_compare (p1, p2);

 out:
        if (p1 != NULL)
                g_object_unref (p1);
        if (p2 != NULL)
                g_object_unref (p2);

        return result;
}

static void
do_coldplug (GduPoolTreeModel *model)
{
        guint n;

        /* remove all.. */
        gtk_tree_store_clear (GTK_TREE_STORE (model));

        /* then coldplug it back */
        for (n = 0; n < model->priv->pools->len; n++) {
                GduPool *pool = GDU_POOL (model->priv->pools->pdata[n]);
                GList *presentables;
                GList *l;

                presentables = gdu_pool_get_presentables (pool);
                for (l = presentables; l != NULL; l = l->next) {
                        GduPresentable *presentable = GDU_PRESENTABLE (l->data);

                        add_presentable (model, presentable, NULL);
                        g_object_unref (presentable);
                }
                g_list_free (presentables);
        }
}

static void
gdu_pool_tree_model_constructed (GObject *object)
{
        GduPoolTreeModel *model = GDU_POOL_TREE_MODEL (object);
        GType column_types[8];

        column_types[0] = G_TYPE_ICON;
        column_types[1] = G_TYPE_STRING;
        column_types[2] = G_TYPE_STRING;
        column_types[3] = G_TYPE_STRING;
        column_types[4] = GDU_TYPE_PRESENTABLE;
        column_types[5] = G_TYPE_BOOLEAN;
        column_types[6] = G_TYPE_BOOLEAN;
        column_types[7] = G_TYPE_BOOLEAN;

        gtk_tree_store_set_column_types (GTK_TREE_STORE (model),
                                         G_N_ELEMENTS (column_types),
                                         column_types);

        gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (model),
                                         GDU_POOL_TREE_MODEL_COLUMN_PRESENTABLE,
                                         presentable_sort_func,
                                         NULL,
                                         NULL);

        gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (model),
                                              GDU_POOL_TREE_MODEL_COLUMN_PRESENTABLE,
                                              GTK_SORT_ASCENDING);

        model->priv->constructed = TRUE;

        do_coldplug (model);

        if (G_OBJECT_CLASS (gdu_pool_tree_model_parent_class)->constructed != NULL)
                G_OBJECT_CLASS (gdu_pool_tree_model_parent_class)->constructed (object);
}

void
gdu_pool_tree_model_set_pools (GduPoolTreeModel      *model,
                               GPtrArray             *pools)
{
        guint n;

        /* Do a deep copy here */
        if (model->priv->pools != NULL) {
                g_ptr_array_foreach (model->priv->pools, (GFunc) disconnect_from_pool_cb, model);
                g_ptr_array_unref (model->priv->pools);
        }

        model->priv->pools = g_ptr_array_new_with_free_func (g_object_unref);
        for (n = 0; pools != NULL && n < pools->len; n++) {
                GduPool *pool = GDU_POOL (pools->pdata[n]);

                g_signal_connect (pool,
                                  "presentable-added",
                                  G_CALLBACK (on_presentable_added),
                                  model);
                g_signal_connect (pool,
                                  "presentable-removed",
                                  G_CALLBACK (on_presentable_removed),
                                  model);
                g_signal_connect (pool,
                                  "presentable-changed",
                                  G_CALLBACK (on_presentable_changed),
                                  model);

                g_ptr_array_add (model->priv->pools, g_object_ref (pool));
        }

        /* only do coldplug if we have been constructed as the result depends on the value of the flags */
        if (model->priv->constructed)
                do_coldplug (model);
}

static void
gdu_pool_tree_model_class_init (GduPoolTreeModelClass *klass)
{
        GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

        gobject_class->finalize     = gdu_pool_tree_model_finalize;
        gobject_class->constructed  = gdu_pool_tree_model_constructed;
        gobject_class->set_property = gdu_pool_tree_model_set_property;
        gobject_class->get_property = gdu_pool_tree_model_get_property;

        g_type_class_add_private (klass, sizeof (GduPoolTreeModelPrivate));

        /**
         * GduPoolTreeModel:pools:
         *
         * The pools displayed - this must be a #GPtrArray of #GduPool objects.
         */
        g_object_class_install_property (gobject_class,
                                         PROP_POOLS,
                                         g_param_spec_boxed ("pools",
                                                             NULL,
                                                             NULL,
                                                             G_TYPE_PTR_ARRAY,
                                                             G_PARAM_WRITABLE |
                                                             G_PARAM_READABLE |
                                                             G_PARAM_CONSTRUCT));

        /**
         * GduPoolTreeModel:root:
         *
         * %NULL to include all #GduPresentable objects in #GduPoolTreeModel:pool, otherwise only
         * include presentables that are descendents of this #GduPresentable.
         */
        g_object_class_install_property (gobject_class,
                                         PROP_ROOT,
                                         g_param_spec_object ("root",
                                                              NULL,
                                                              NULL,
                                                              GDU_TYPE_PRESENTABLE,
                                                              G_PARAM_WRITABLE |
                                                              G_PARAM_READABLE |
                                                              G_PARAM_CONSTRUCT_ONLY));

        /**
         * GduPoolTreeModel:flags:
         *
         * The flags for the model.
         */
        g_object_class_install_property (gobject_class,
                                         PROP_FLAGS,
                                         g_param_spec_flags ("flags",
                                                             NULL,
                                                             NULL,
                                                             GDU_TYPE_POOL_TREE_MODEL_FLAGS,
                                                             GDU_POOL_TREE_MODEL_FLAGS_NONE,
                                                             G_PARAM_WRITABLE |
                                                             G_PARAM_READABLE |
                                                             G_PARAM_CONSTRUCT_ONLY));
}

static void
gdu_pool_tree_model_init (GduPoolTreeModel *model)
{
        model->priv = G_TYPE_INSTANCE_GET_PRIVATE (model,
                                                   GDU_TYPE_POOL_TREE_MODEL,
                                                   GduPoolTreeModelPrivate);
}

GduPoolTreeModel *
gdu_pool_tree_model_new (GPtrArray             *pools,
                         GduPresentable        *root,
                         GduPoolTreeModelFlags  flags)
{
        return GDU_POOL_TREE_MODEL (g_object_new (GDU_TYPE_POOL_TREE_MODEL,
                                                  "pools", pools,
                                                  "root", root,
                                                  "flags", flags,
                                                  NULL));
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct {
        GduPresentable *presentable;
        gboolean found;
        GtkTreeIter iter;
} FIBDData;

static gboolean
find_iter_by_presentable_foreach (GtkTreeModel *model,
                                  GtkTreePath  *path,
                                  GtkTreeIter  *iter,
                                  gpointer      data)
{
        gboolean ret;
        GduPresentable *presentable = NULL;
        FIBDData *fibd_data = (FIBDData *) data;

        ret = FALSE;
        gtk_tree_model_get (model,
                            iter,
                            GDU_POOL_TREE_MODEL_COLUMN_PRESENTABLE, &presentable,
                            -1);
        if (g_strcmp0 (gdu_presentable_get_id (presentable), gdu_presentable_get_id (fibd_data->presentable)) == 0) {
                fibd_data->found = TRUE;
                fibd_data->iter = *iter;
                ret = TRUE;
        }
        if (presentable != NULL)
                g_object_unref (presentable);

        return ret;
}


gboolean
gdu_pool_tree_model_get_iter_for_presentable (GduPoolTreeModel *model,
                                              GduPresentable   *presentable,
                                              GtkTreeIter      *out_iter)
{
        FIBDData fibd_data;
        gboolean ret;

        fibd_data.presentable = presentable;
        fibd_data.found = FALSE;
        gtk_tree_model_foreach (GTK_TREE_MODEL (model),
                                find_iter_by_presentable_foreach,
                                &fibd_data);
        if (fibd_data.found) {
                if (out_iter != NULL)
                        *out_iter = fibd_data.iter;
                ret = TRUE;
        } else {
                ret = FALSE;
        }

        return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

static void
set_data_for_presentable (GduPoolTreeModel *model,
                          GtkTreeIter      *iter,
                          GduPresentable   *presentable)
{
        GduDevice *device;
        GIcon *icon;
        gchar *vpd_name;
        gchar *name;
        gchar *desc;

        device = gdu_presentable_get_device (presentable);

        name = gdu_presentable_get_name (presentable);
        desc = gdu_presentable_get_description (presentable);
        vpd_name = gdu_presentable_get_vpd_name (presentable);

        icon = gdu_presentable_get_icon (presentable);

        /* TODO: insert NAME */
        gtk_tree_store_set (GTK_TREE_STORE (model),
                            iter,
                            GDU_POOL_TREE_MODEL_COLUMN_ICON, icon,
                            GDU_POOL_TREE_MODEL_COLUMN_VPD_NAME, vpd_name,
                            GDU_POOL_TREE_MODEL_COLUMN_NAME, name,
                            GDU_POOL_TREE_MODEL_COLUMN_DESCRIPTION, desc,
                            GDU_POOL_TREE_MODEL_COLUMN_PRESENTABLE, presentable,
                            GDU_POOL_TREE_MODEL_COLUMN_VISIBLE, TRUE,
                            GDU_POOL_TREE_MODEL_COLUMN_TOGGLED, FALSE,
                            GDU_POOL_TREE_MODEL_COLUMN_CAN_BE_TOGGLED, FALSE,
                            -1);

        g_object_unref (icon);
        g_free (vpd_name);
        g_free (name);
        g_free (desc);
        if (device != NULL)
                g_object_unref (device);
}

static gboolean
should_include_presentable (GduPoolTreeModel *model,
                            GduPresentable   *presentable)
{
        gboolean ret;

        ret = FALSE;

        /* see if it should be ignored because it is a volume */
        if ((model->priv->flags & GDU_POOL_TREE_MODEL_FLAGS_NO_VOLUMES) &&
            (GDU_IS_VOLUME (presentable) || GDU_IS_VOLUME_HOLE (presentable)))
                goto out;

        if (model->priv->root != NULL) {
                gboolean is_enclosed_by_root;
                GduPresentable *p_iter;

                is_enclosed_by_root = FALSE;
                p_iter = g_object_ref (presentable);
                do {
                        GduPresentable *p;

                        p = gdu_presentable_get_enclosing_presentable (p_iter);
                        g_object_unref (p_iter);
                        if (p == NULL)
                                break;

                        if (p == model->priv->root) {
                                is_enclosed_by_root = TRUE;
                                g_object_unref (p);
                                break;
                        }

                        p_iter = p;

                } while (TRUE);

                if (!is_enclosed_by_root)
                        goto out;
        }

        if (GDU_IS_DRIVE (presentable)) {
                if ((model->priv->flags & GDU_POOL_TREE_MODEL_FLAGS_NO_UNALLOCATABLE_DRIVES) &&
                    (!gdu_drive_can_create_volume (GDU_DRIVE (presentable), NULL, NULL, NULL, NULL)))
                        goto out;
        }

        ret = TRUE;

 out:
        return ret;
}


static void
add_presentable (GduPoolTreeModel *model,
                 GduPresentable   *presentable,
                 GtkTreeIter      *iter_out)
{
        GtkTreeIter  iter;
        GtkTreeIter  iter2;
        GtkTreeIter *parent_iter;
        GduPresentable *enclosing_presentable;

        /* check to see if presentable is already added */
        if (gdu_pool_tree_model_get_iter_for_presentable (model, presentable, NULL))
                goto out;

        if (!should_include_presentable (model, presentable))
                goto out;

        /* set up parent relationship */
        parent_iter = NULL;
        enclosing_presentable = gdu_presentable_get_enclosing_presentable (presentable);
        if (enclosing_presentable != NULL) {
                if (gdu_pool_tree_model_get_iter_for_presentable (model, enclosing_presentable, &iter2)) {
                        parent_iter = &iter2;
                } else {
                        if (should_include_presentable (model, enclosing_presentable)) {
                                /* add parent if it's not already added */
                                g_warning ("No parent for %s", gdu_presentable_get_id (enclosing_presentable));
                                add_presentable (model, enclosing_presentable, &iter2);
                                parent_iter = &iter2;
                        } else {
                                /* parent explicitly excluded */
                        }
                }
                g_object_unref (enclosing_presentable);
        }


        /* g_debug ("adding %s (%p)", gdu_presentable_get_id (presentable), presentable); */

        gtk_tree_store_append (GTK_TREE_STORE (model),
                               &iter,
                               parent_iter);

        set_data_for_presentable (model,
                                  &iter,
                                  presentable);


        if (iter_out != NULL)
                *iter_out = iter;

out:
        ;
}

static void
on_presentable_added (GduPool          *pool,
                      GduPresentable   *presentable,
                      gpointer          user_data)
{
        GduPoolTreeModel *model = GDU_POOL_TREE_MODEL (user_data);

        /* g_debug ("on_added `%s' (%p)", gdu_presentable_get_id (presentable), presentable); */

        add_presentable (model, presentable, NULL);
}

static void
on_presentable_removed (GduPool          *pool,
                        GduPresentable   *presentable,
                        gpointer          user_data)
{
        GduPoolTreeModel *model = GDU_POOL_TREE_MODEL (user_data);
        GtkTreeIter iter;

        /* g_debug ("on_removed `%s' (%p)", gdu_presentable_get_id (presentable), presentable); */

        if (gdu_pool_tree_model_get_iter_for_presentable (model, presentable, &iter)) {
                /* g_debug ("removed row for `%s' (%p)", gdu_presentable_get_id (presentable), presentable); */
                gtk_tree_store_remove (GTK_TREE_STORE (model), &iter);
        }
}

static void
on_presentable_changed (GduPool          *pool,
                        GduPresentable   *presentable,
                        gpointer          user_data)
{
        GduPoolTreeModel *model = GDU_POOL_TREE_MODEL (user_data);
        GtkTreeIter iter;

        if (gdu_pool_has_presentable (pool, presentable)) {
                /* g_debug ("on_changed `%s' (%p)", gdu_presentable_get_id (presentable), presentable); */

                /* will do NOP if presentable has already been added */
                add_presentable (model, presentable, NULL);

                /* update name and icon */
                if (gdu_pool_tree_model_get_iter_for_presentable (model, presentable, &iter)) {
                        set_data_for_presentable (model,
                                                  &iter,
                                                  presentable);
                }
        }
}
