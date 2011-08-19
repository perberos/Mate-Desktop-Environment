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
#include "gdu-pool-tree-view.h"
#include "gdu-pool-tree-model.h"
#include "gdu-gtk-enumtypes.h"

struct GduPoolTreeViewPrivate
{
        GduPoolTreeModel *model;
        GduPoolTreeViewFlags flags;
};

G_DEFINE_TYPE (GduPoolTreeView, gdu_pool_tree_view, GTK_TYPE_TREE_VIEW)

enum {
        PROP_0,
        PROP_POOL_TREE_MODEL,
        PROP_FLAGS,
};

static void
gdu_pool_tree_view_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
        GduPoolTreeView *view = GDU_POOL_TREE_VIEW (object);

        switch (prop_id) {
        case PROP_POOL_TREE_MODEL:
                view->priv->model = g_value_dup_object (value);
                break;

        case PROP_FLAGS:
                view->priv->flags = g_value_get_flags (value);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gdu_pool_tree_view_get_property (GObject     *object,
                                 guint        prop_id,
                                 GValue      *value,
                                 GParamSpec  *pspec)
{
        GduPoolTreeView *view = GDU_POOL_TREE_VIEW (object);

        switch (prop_id) {
        case PROP_POOL_TREE_MODEL:
                g_value_set_object (value, view->priv->model);
                break;

        case PROP_FLAGS:
                g_value_set_flags (value, view->priv->flags);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
    }
}

static void
gdu_pool_tree_view_finalize (GObject *object)
{
        GduPoolTreeView *view = GDU_POOL_TREE_VIEW (object);

        if (view->priv->model != NULL)
                g_object_unref (view->priv->model);

        if (G_OBJECT_CLASS (gdu_pool_tree_view_parent_class)->finalize != NULL)
                G_OBJECT_CLASS (gdu_pool_tree_view_parent_class)->finalize (object);
}

static void
on_row_inserted (GtkTreeModel *tree_model,
                 GtkTreePath  *path,
                 GtkTreeIter  *iter,
                 gpointer      user_data)
{
        GduPoolTreeView *view = GDU_POOL_TREE_VIEW (user_data);
        gtk_tree_view_expand_all (GTK_TREE_VIEW (view));
}

static void
on_toggled (GtkCellRendererToggle *renderer,
            const gchar           *path_string,
            gpointer               user_data)
{
        GduPoolTreeView *view = GDU_POOL_TREE_VIEW (user_data);
        GtkTreeIter iter;
        gboolean value;

        if (!gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (view->priv->model),
                                                  &iter,
                                                  path_string))
                goto out;

        gtk_tree_model_get (GTK_TREE_MODEL (view->priv->model),
                            &iter,
                            GDU_POOL_TREE_MODEL_COLUMN_TOGGLED, &value,
                            -1);

        gtk_tree_store_set (GTK_TREE_STORE (view->priv->model),
                            &iter,
                            GDU_POOL_TREE_MODEL_COLUMN_TOGGLED, !value,
                            -1);
 out:
        ;
}

static void
format_markup (GtkCellLayout   *cell_layout,
               GtkCellRenderer *renderer,
               GtkTreeModel    *tree_model,
               GtkTreeIter     *iter,
               gpointer         user_data)
{
        GduPoolTreeView *view = GDU_POOL_TREE_VIEW (user_data);
        GtkTreeSelection *tree_selection;
        gchar *name;
        gchar *vpd_name;
        gchar *desc;
        GduPresentable *p;
        gchar *markup;
        gchar color[16];
        GtkStateType state;

        tree_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));

        gtk_tree_model_get (tree_model,
                            iter,
                            GDU_POOL_TREE_MODEL_COLUMN_PRESENTABLE, &p,
                            GDU_POOL_TREE_MODEL_COLUMN_NAME, &name,
                            GDU_POOL_TREE_MODEL_COLUMN_VPD_NAME, &vpd_name,
                            GDU_POOL_TREE_MODEL_COLUMN_DESCRIPTION, &desc,
                            -1);

        if (gtk_tree_selection_iter_is_selected (tree_selection, iter)) {
                if (gtk_widget_has_focus (GTK_WIDGET (view)))
                        state = GTK_STATE_SELECTED;
                else
                        state = GTK_STATE_ACTIVE;
        } else {
                state = GTK_STATE_NORMAL;
        }
        gdu_util_get_mix_color (GTK_WIDGET (view), state, color, sizeof (color));

        /* Only include VPD name for drives */
        if (GDU_IS_DRIVE (p)) {
                markup = g_strdup_printf ("<small>"
                                          "<b>%s</b>\n"
                                          "<span fgcolor=\"%s\">%s</span>"
                                          "</small>",
                                          name,
                                          color,
                                          vpd_name);
        } else {
                markup = g_strdup_printf ("<small>"
                                          "<b>%s</b>\n"
                                          "<span fgcolor=\"%s\">%s</span>"
                                          "</small>",
                                          name,
                                          color,
                                          desc);
        }

        g_object_set (renderer,
                      "markup", markup,
                      "ellipsize-set", TRUE,
                      "ellipsize", PANGO_ELLIPSIZE_MIDDLE,
                      NULL);

        g_free (name);
        g_free (desc);
        g_free (vpd_name);
        g_free (markup);
        g_object_unref (p);
}

static void
pixbuf_data_func (GtkCellLayout   *cell_layout,
                  GtkCellRenderer *renderer,
                  GtkTreeModel    *tree_model,
                  GtkTreeIter     *iter,
                  gpointer         user_data)
{
        //GduPoolTreeView *view = GDU_POOL_TREE_VIEW (user_data);
        GduPresentable *p;
        GIcon *icon;

        gtk_tree_model_get (tree_model,
                            iter,
                            GDU_POOL_TREE_MODEL_COLUMN_PRESENTABLE, &p,
                            GDU_POOL_TREE_MODEL_COLUMN_ICON, &icon,
                            -1);

        g_object_set (renderer,
                      "gicon", icon,
                      "stock-size", GTK_ICON_SIZE_LARGE_TOOLBAR,
                      NULL);

        g_object_unref (p);
        g_object_unref (icon);
}

static void
gdu_pool_tree_view_constructed (GObject *object)
{
        GduPoolTreeView *view = GDU_POOL_TREE_VIEW (object);
        GtkCellRenderer *renderer;
        GtkTreeViewColumn *column;

        gtk_tree_view_set_model (GTK_TREE_VIEW (view),
                                 GTK_TREE_MODEL (view->priv->model));

        column = gtk_tree_view_column_new ();
        if (view->priv->flags & GDU_POOL_TREE_VIEW_FLAGS_SHOW_TOGGLE) {
                renderer = gtk_cell_renderer_toggle_new ();
                gtk_tree_view_column_pack_start (column,
                                                 renderer,
                                                 FALSE);
                gtk_tree_view_column_set_attributes (column,
                                                     renderer,
                                                     "active", GDU_POOL_TREE_MODEL_COLUMN_TOGGLED,
                                                     NULL);
                g_signal_connect (renderer,
                                  "toggled",
                                  G_CALLBACK (on_toggled),
                                  view);
        }

        renderer = gtk_cell_renderer_pixbuf_new ();
        gtk_tree_view_column_pack_start (column, renderer, FALSE);
        gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (column),
                                            renderer,
                                            pixbuf_data_func,
                                            view,
                                            NULL);
        if (view->priv->flags & GDU_POOL_TREE_VIEW_FLAGS_SHOW_TOGGLE) {
                gtk_tree_view_column_add_attribute (column,
                                                    renderer,
                                                    "sensitive", GDU_POOL_TREE_MODEL_COLUMN_CAN_BE_TOGGLED);
        }

        renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_column_pack_start (column, renderer, TRUE);
        gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (column),
                                            renderer,
                                            format_markup,
                                            view,
                                            NULL);

        if (view->priv->flags & GDU_POOL_TREE_VIEW_FLAGS_SHOW_TOGGLE) {
                gtk_tree_view_column_add_attribute (column,
                                                    renderer,
                                                    "sensitive", GDU_POOL_TREE_MODEL_COLUMN_CAN_BE_TOGGLED);
        }
        gtk_tree_view_append_column (GTK_TREE_VIEW (view), column);

        gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);

        gtk_tree_view_set_show_expanders (GTK_TREE_VIEW (view), FALSE);
        /*gtk_tree_view_set_enable_tree_lines (GTK_TREE_VIEW (view), TRUE);*/
        gtk_tree_view_set_level_indentation (GTK_TREE_VIEW (view), 16);
        gtk_tree_view_expand_all (GTK_TREE_VIEW (view));

        /* expand on insertion - hmm, I wonder if there's an easier way to do this */
        g_signal_connect (view->priv->model,
                          "row-inserted",
                          G_CALLBACK (on_row_inserted),
                          view);

        if (G_OBJECT_CLASS (gdu_pool_tree_view_parent_class)->constructed != NULL)
                G_OBJECT_CLASS (gdu_pool_tree_view_parent_class)->constructed (object);
}

static void
gdu_pool_tree_view_class_init (GduPoolTreeViewClass *klass)
{
        GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

        gobject_class->finalize     = gdu_pool_tree_view_finalize;
        gobject_class->constructed  = gdu_pool_tree_view_constructed;
        gobject_class->set_property = gdu_pool_tree_view_set_property;
        gobject_class->get_property = gdu_pool_tree_view_get_property;

        g_type_class_add_private (klass, sizeof (GduPoolTreeViewPrivate));

        /**
         * GduPoolTreeView:pool-tree-model:
         *
         * The #GduPoolTreeModel instance we are getting information from
         */
        g_object_class_install_property (gobject_class,
                                         PROP_POOL_TREE_MODEL,
                                         g_param_spec_object ("pool-tree-model",
                                                              NULL,
                                                              NULL,
                                                              GDU_TYPE_POOL_TREE_MODEL,
                                                              G_PARAM_WRITABLE |
                                                              G_PARAM_READABLE |
                                                              G_PARAM_CONSTRUCT_ONLY));

        /**
         * GduPoolTreeView:flags:
         *
         * Flags from the #GduPoolTreeViewFlags enumeration to
         * customize behavior of the view.
         */
        g_object_class_install_property (gobject_class,
                                         PROP_FLAGS,
                                         g_param_spec_flags ("flags",
                                                             NULL,
                                                             NULL,
                                                             GDU_TYPE_POOL_TREE_VIEW_FLAGS,
                                                             GDU_POOL_TREE_VIEW_FLAGS_NONE,
                                                             G_PARAM_WRITABLE |
                                                             G_PARAM_READABLE |
                                                             G_PARAM_CONSTRUCT_ONLY));
}

static void
gdu_pool_tree_view_init (GduPoolTreeView *view)
{
        view->priv = G_TYPE_INSTANCE_GET_PRIVATE (view, GDU_TYPE_POOL_TREE_VIEW, GduPoolTreeViewPrivate);
}

GtkWidget *
gdu_pool_tree_view_new (GduPoolTreeModel     *model,
                        GduPoolTreeViewFlags  flags)
{
        return GTK_WIDGET (g_object_new (GDU_TYPE_POOL_TREE_VIEW,
                                         "pool-tree-model", model,
                                         "flags", flags,
                                         NULL));
}

GduPresentable *
gdu_pool_tree_view_get_selected_presentable (GduPoolTreeView *view)
{
        GtkTreePath *path;
        GduPresentable *presentable;

        presentable = NULL;

        gtk_tree_view_get_cursor (GTK_TREE_VIEW (view), &path, NULL);
        if (path != NULL) {
                GtkTreeIter iter;

                if (gtk_tree_model_get_iter (GTK_TREE_MODEL (view->priv->model), &iter, path)) {

                        gtk_tree_model_get (GTK_TREE_MODEL (view->priv->model),
                                            &iter,
                                            GDU_POOL_TREE_MODEL_COLUMN_PRESENTABLE,
                                            &presentable,
                                            -1);
                }

                gtk_tree_path_free (path);
        }

        return presentable;
}

void
gdu_pool_tree_view_select_presentable (GduPoolTreeView *view,
                                       GduPresentable  *presentable)
{
        GtkTreePath *path;
        GtkTreeIter iter;

        if (presentable == NULL) {
                gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (GTK_TREE_VIEW (view)));
        } else {
                if (!gdu_pool_tree_model_get_iter_for_presentable (view->priv->model, presentable, &iter))
                        goto out;

                path = gtk_tree_model_get_path (GTK_TREE_MODEL (view->priv->model), &iter);
                if (path == NULL)
                        goto out;

                gtk_tree_view_set_cursor (GTK_TREE_VIEW (view), path, NULL, FALSE);
                gtk_tree_path_free (path);
        }
out:
        ;
}

void
gdu_pool_tree_view_select_first_presentable (GduPoolTreeView *view)
{
        GtkTreePath *path;
        GtkTreeIter iter;

        if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (view->priv->model), &iter)) {
                path = gtk_tree_model_get_path (GTK_TREE_MODEL (view->priv->model), &iter);
                if (path == NULL)
                        goto out;

                gtk_tree_view_set_cursor (GTK_TREE_VIEW (view), path, NULL, FALSE);
                gtk_tree_path_free (path);
        }
out:
        ;
}
