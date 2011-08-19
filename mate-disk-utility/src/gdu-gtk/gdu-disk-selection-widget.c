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

#define _GNU_SOURCE

#include "config.h"
#include <glib/gi18n-lib.h>

#include <math.h>

#include <gdu/gdu-marshal.h>

#include "gdu-disk-selection-widget.h"
#include "gdu-size-widget.h"

#define DETAILS_WIDTH 180
#define DETAILS_MARGIN 12

#define SIZE_EPSILON 1000

struct GduDiskSelectionWidgetPrivate
{
        GduPool *pool;
        GduDiskSelectionWidgetFlags flags;

        GtkTreeModel *model;
        GtkWidget *tree_view;

        guint64 component_size;

        /* A list of GduDrive objects that are selected */
        GList *selected_drives;
};

enum
{
        PROP_0,
        PROP_POOL,
        PROP_FLAGS,
        PROP_SELECTED_DRIVES,
        PROP_COMPONENT_SIZE,
        PROP_NUM_AVAILABLE_DISKS,
        PROP_LARGEST_SEGMENT_FOR_SELECTED,
        PROP_LARGEST_SEGMENT_FOR_ALL
};

enum
{
        CHANGED_SIGNAL,
        IS_DRIVE_IGNORED_SIGNAL,
        LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void gdu_disk_selection_widget_constructed (GObject *object);

static void emit_changed (GduDiskSelectionWidget *widget);

static void on_presentable_added   (GduPool          *pool,
                                    GduPresentable   *presentable,
                                    gpointer          user_data);
static void on_presentable_removed (GduPool          *pool,
                                    GduPresentable   *presentable,
                                    gpointer          user_data);
static void on_presentable_changed (GduPool          *pool,
                                    GduPresentable   *presentable,
                                    gpointer          user_data);

static void on_row_changed (GtkTreeModel *tree_model,
                            GtkTreePath  *path,
                            GtkTreeIter  *iter,
                            gpointer      user_data);

static void on_row_deleted (GtkTreeModel *tree_model,
                            GtkTreePath  *path,
                            gpointer      user_data);

static void on_row_inserted (GtkTreeModel *tree_model,
                             GtkTreePath  *path,
                             GtkTreeIter  *iter,
                             gpointer      user_data);

G_DEFINE_TYPE (GduDiskSelectionWidget, gdu_disk_selection_widget, GTK_TYPE_VBOX)

static void
gdu_disk_selection_widget_finalize (GObject *object)
{
        GduDiskSelectionWidget *widget = GDU_DISK_SELECTION_WIDGET (object);

        g_signal_handlers_disconnect_by_func (widget->priv->pool, on_presentable_added, widget);
        g_signal_handlers_disconnect_by_func (widget->priv->pool, on_presentable_removed, widget);
        g_signal_handlers_disconnect_by_func (widget->priv->pool, on_presentable_changed, widget);
        g_signal_handlers_disconnect_by_func (widget->priv->model, on_row_changed, widget);
        g_signal_handlers_disconnect_by_func (widget->priv->model, on_row_deleted, widget);
        g_signal_handlers_disconnect_by_func (widget->priv->model, on_row_inserted, widget);

        g_object_unref (widget->priv->pool);
        g_object_unref (widget->priv->model);

        g_list_foreach (widget->priv->selected_drives, (GFunc) g_object_unref, NULL);
        g_list_free (widget->priv->selected_drives);

        if (G_OBJECT_CLASS (gdu_disk_selection_widget_parent_class)->finalize != NULL)
                G_OBJECT_CLASS (gdu_disk_selection_widget_parent_class)->finalize (object);
}

static void
gdu_disk_selection_widget_get_property (GObject    *object,
                                         guint       property_id,
                                         GValue     *value,
                                         GParamSpec *pspec)
{
        GduDiskSelectionWidget *widget = GDU_DISK_SELECTION_WIDGET (object);
        GPtrArray *p;

        switch (property_id) {
        case PROP_POOL:
                g_value_set_object (value, widget->priv->pool);
                break;

        case PROP_FLAGS:
                g_value_set_flags (value, widget->priv->flags);
                break;

        case PROP_SELECTED_DRIVES:
                p = gdu_disk_selection_widget_get_selected_drives (widget);
                g_value_set_boxed (value, p);
                g_ptr_array_unref (p);
                break;

        case PROP_COMPONENT_SIZE:
                g_value_set_uint64 (value, gdu_disk_selection_widget_get_component_size (widget));
                break;

        case PROP_NUM_AVAILABLE_DISKS:
                g_value_set_uint (value, gdu_disk_selection_widget_get_num_available_disks (widget));
                break;

        case PROP_LARGEST_SEGMENT_FOR_SELECTED:
                g_value_set_uint64 (value, gdu_disk_selection_widget_get_largest_segment_for_selected (widget));
                break;

        case PROP_LARGEST_SEGMENT_FOR_ALL:
                g_value_set_uint64 (value, gdu_disk_selection_widget_get_largest_segment_for_all (widget));
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
gdu_disk_selection_widget_set_property (GObject      *object,
                                         guint         property_id,
                                         const GValue *value,
                                         GParamSpec   *pspec)
{
        GduDiskSelectionWidget *widget = GDU_DISK_SELECTION_WIDGET (object);

        switch (property_id) {
        case PROP_POOL:
                widget->priv->pool = g_value_dup_object (value);
                break;

        case PROP_FLAGS:
                widget->priv->flags = g_value_get_flags (value);
                break;

        case PROP_COMPONENT_SIZE:
                gdu_disk_selection_widget_set_component_size (widget, g_value_get_uint64 (value));
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static gboolean
gdu_disk_selection_widget_mnemonic_activate (GtkWidget *_widget,
                                             gboolean   group_cycling)
{
        GduDiskSelectionWidget *widget = GDU_DISK_SELECTION_WIDGET (_widget);
        return gtk_widget_mnemonic_activate (widget->priv->tree_view, group_cycling);
}

static void
gdu_disk_selection_widget_class_init (GduDiskSelectionWidgetClass *klass)
{
        GObjectClass *gobject_class;
        GtkWidgetClass *widget_class;

        gobject_class = G_OBJECT_CLASS (klass);
        widget_class = GTK_WIDGET_CLASS (klass);

        g_type_class_add_private (klass, sizeof (GduDiskSelectionWidgetPrivate));

        gobject_class->get_property = gdu_disk_selection_widget_get_property;
        gobject_class->set_property = gdu_disk_selection_widget_set_property;
        gobject_class->constructed  = gdu_disk_selection_widget_constructed;
        gobject_class->finalize     = gdu_disk_selection_widget_finalize;

        widget_class->mnemonic_activate = gdu_disk_selection_widget_mnemonic_activate;

        g_object_class_install_property (gobject_class,
                                         PROP_POOL,
                                         g_param_spec_object ("pool",
                                                              _("Pool"),
                                                              _("The pool of devices"),
                                                              GDU_TYPE_POOL,
                                                              G_PARAM_READABLE |
                                                              G_PARAM_WRITABLE |
                                                              G_PARAM_CONSTRUCT_ONLY |
                                                              G_PARAM_STATIC_NAME |
                                                              G_PARAM_STATIC_NICK |
                                                              G_PARAM_STATIC_BLURB));

        g_object_class_install_property (gobject_class,
                                         PROP_FLAGS,
                                         g_param_spec_flags ("flags",
                                                             _("Flags"),
                                                             _("Flags for the widget"),
                                                             GDU_TYPE_DISK_SELECTION_WIDGET_FLAGS,
                                                             GDU_DISK_SELECTION_WIDGET_FLAGS_NONE,
                                                             G_PARAM_READABLE |
                                                             G_PARAM_WRITABLE |
                                                             G_PARAM_CONSTRUCT_ONLY |
                                                             G_PARAM_STATIC_NAME |
                                                             G_PARAM_STATIC_NICK |
                                                             G_PARAM_STATIC_BLURB));

        g_object_class_install_property (gobject_class,
                                         PROP_SELECTED_DRIVES,
                                         g_param_spec_boxed ("selected-drives",
                                                             _("Selected Drives"),
                                                             _("Array of selected drives"),
                                                             G_TYPE_PTR_ARRAY,
                                                             G_PARAM_READABLE |
                                                             G_PARAM_STATIC_NAME |
                                                             G_PARAM_STATIC_NICK |
                                                             G_PARAM_STATIC_BLURB));

        g_object_class_install_property (gobject_class,
                                         PROP_NUM_AVAILABLE_DISKS,
                                         g_param_spec_uint ("num-available-disks",
                                                            _("Number of available disks"),
                                                            _("Number of available disks"),
                                                            0,
                                                            G_MAXUINT,
                                                            0,
                                                            G_PARAM_READABLE |
                                                            G_PARAM_STATIC_NAME |
                                                            G_PARAM_STATIC_NICK |
                                                            G_PARAM_STATIC_BLURB));

        g_object_class_install_property (gobject_class,
                                         PROP_LARGEST_SEGMENT_FOR_SELECTED,
                                         g_param_spec_uint64 ("largest-segment-for-selected",
                                                              _("Largest Segment For Selected"),
                                                              _("The largest free segment for the selected drives"),
                                                              0,
                                                              G_MAXUINT64,
                                                              0,
                                                              G_PARAM_READABLE |
                                                              G_PARAM_STATIC_NAME |
                                                              G_PARAM_STATIC_NICK |
                                                              G_PARAM_STATIC_BLURB));

        g_object_class_install_property (gobject_class,
                                         PROP_LARGEST_SEGMENT_FOR_ALL,
                                         g_param_spec_uint64 ("largest-segment-for-all",
                                                              _("Largest Segment For All"),
                                                              _("The largest free segment for all the drives"),
                                                              0,
                                                              G_MAXUINT64,
                                                              0,
                                                              G_PARAM_READABLE |
                                                              G_PARAM_STATIC_NAME |
                                                              G_PARAM_STATIC_NICK |
                                                              G_PARAM_STATIC_BLURB));

        g_object_class_install_property (gobject_class,
                                         PROP_COMPONENT_SIZE,
                                         g_param_spec_uint64 ("component-size",
                                                              _("Component Size"),
                                                              _("The size to use in the details header"),
                                                              0,
                                                              G_MAXUINT64,
                                                              0,
                                                              G_PARAM_READABLE |
                                                              G_PARAM_WRITABLE |
                                                              G_PARAM_STATIC_NAME |
                                                              G_PARAM_STATIC_NICK |
                                                              G_PARAM_STATIC_BLURB));

        signals[CHANGED_SIGNAL] = g_signal_new ("changed",
                                                G_TYPE_FROM_CLASS (klass),
                                                G_SIGNAL_RUN_LAST,
                                                G_STRUCT_OFFSET (GduDiskSelectionWidgetClass, changed),
                                                NULL,
                                                NULL,
                                                g_cclosure_marshal_VOID__VOID,
                                                G_TYPE_NONE,
                                                0);

        /**
         * GduDiskSelectionWidget::is-drive-ignored:
         * @widget: A #GduDiskSelectionWidget.
         * @drive: A #GduDrive.
         *
         * Emitted when @widget needs to determine whether @drive is
         * ignored or not.
         *
         * Returns: %NULL if @drive is not ignored, otherwise an allocated localized string
         * containing the reason why @drive is ignored.  The recipient of the signal will free
         * the string.
         */
        signals[IS_DRIVE_IGNORED_SIGNAL] = g_signal_new ("is-drive-ignored",
                                                         G_TYPE_FROM_CLASS (klass),
                                                         G_SIGNAL_RUN_LAST,
                                                         G_STRUCT_OFFSET (GduDiskSelectionWidgetClass, is_drive_ignored),
                                                         NULL, /* accumulator function */
                                                         NULL, /* user_data for accumulator function */
                                                         gdu_marshal_STRING__OBJECT,
                                                         G_TYPE_STRING,
                                                         1,
                                                         GDU_TYPE_DRIVE);

}

static void
gdu_disk_selection_widget_init (GduDiskSelectionWidget *widget)
{
        widget->priv = G_TYPE_INSTANCE_GET_PRIVATE (widget,
                                                    GDU_TYPE_DISK_SELECTION_WIDGET,
                                                    GduDiskSelectionWidgetPrivate);
}

GtkWidget *
gdu_disk_selection_widget_new (GduPool                     *pool,
                               GduDiskSelectionWidgetFlags  flags)
{
        return GTK_WIDGET (g_object_new (GDU_TYPE_DISK_SELECTION_WIDGET,
                                         "pool", pool,
                                         "flags", flags,
                                         NULL));
}

/* ---------------------------------------------------------------------------------------------------- */

GPtrArray *
gdu_disk_selection_widget_get_selected_drives (GduDiskSelectionWidget  *widget)
{
        GPtrArray *p;
        GList *l;

        g_return_val_if_fail (GDU_IS_DISK_SELECTION_WIDGET (widget), NULL);

        p = g_ptr_array_new_with_free_func (g_object_unref);
        for (l = widget->priv->selected_drives; l != NULL; l = l->next) {
                GduPresentable *drive = GDU_PRESENTABLE (l->data);
                g_ptr_array_add (p, g_object_ref (drive));
        }
        return p;
}

/* ---------------------------------------------------------------------------------------------------- */


static gboolean
is_drive_selectable (GduDiskSelectionWidget *widget,
                     GduDrive               *drive,
                     gchar                 **out_reason)
{
        gboolean ret;
        guint64 largest_segment;
        gboolean whole_disk_is_uninitialized;
        GduDevice *d;
        gchar *reason;

        ret = FALSE;
        d = NULL;
        reason = NULL;

        g_signal_emit (widget,
                       signals[IS_DRIVE_IGNORED_SIGNAL],
                       0, /* detail */
                       drive,
                       &reason);
        if (reason != NULL) {
                goto out;
        }

        d = gdu_presentable_get_device (GDU_PRESENTABLE (drive));
        if (d != NULL && gdu_device_should_ignore (d)) {
                reason = g_strdup (_("Cannot select multipath component"));
                goto out;
        }

        if (gdu_drive_can_create_volume (drive,
                                         &whole_disk_is_uninitialized,
                                         &largest_segment,
                                         NULL, /* total_free */
                                         NULL)) {
                if (widget->priv->flags & GDU_DISK_SELECTION_WIDGET_FLAGS_ALLOW_DISKS_WITH_INSUFFICIENT_SPACE) {
                        /* size is not enforced */
                        ret = TRUE;
                } else {
                        /* do enforce size */
                        if (largest_segment >= widget->priv->component_size) {
                                ret = TRUE;
                        } else {
                                if (largest_segment < SIZE_EPSILON) {
                                        reason = g_strdup (_("No free space."));
                                } else {
                                       gchar *s1;
                                       gchar *s2;
                                       s1 = gdu_util_get_size_for_display (widget->priv->component_size, FALSE, FALSE);
                                       s2 = gdu_util_get_size_for_display (largest_segment, FALSE, FALSE);
                                       /* Translators: Shown when device is unselectable because not enough space is available.
                                        * First %s (e.g. '10 GB') is how much space is needed.
                                        * Second %s (e.g. '5 GB') is how much space is available.
                                        */
                                       reason = g_strdup_printf (_("Insufficient space: %s is needed but largest contiguous free block is %s."),
                                                                 s1,
                                                                 s2);
                                       g_free (s1);
                                       g_free (s2);
                                }
                                goto out;
                        }
                }
        } else {
                reason = g_strdup (_("No free space."));
        }

 out:

        if (d != NULL)
                g_object_unref (d);

        if (ret)
                g_assert (reason == NULL);
        else
                g_assert (reason != NULL);

        if (out_reason != NULL) {
                *out_reason = reason;
        } else {
                g_free (reason);
        }

        return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

static gboolean
drive_is_selected (GduDiskSelectionWidget *widget,
                   GduPresentable         *drive)
{
        return g_list_find (widget->priv->selected_drives, drive) != NULL;
}

static void
drive_remove (GduDiskSelectionWidget *widget,
              GduPresentable         *drive)
{
        GList *l;

        l = g_list_find (widget->priv->selected_drives, drive);
        if (l != NULL) {
                g_object_unref (l->data);
                widget->priv->selected_drives = g_list_delete_link (widget->priv->selected_drives,
                                                                    l);
        }
}

static void
drive_add (GduDiskSelectionWidget *widget,
           GduPresentable         *drive)
{
        g_return_if_fail (!drive_is_selected (widget, drive));

        widget->priv->selected_drives = g_list_prepend (widget->priv->selected_drives,
                                                        g_object_ref (drive));
}


static void
drive_toggle (GduDiskSelectionWidget *widget,
              GduPresentable         *drive)
{
        if (drive_is_selected (widget, drive)) {
                drive_remove (widget, drive);
        } else {
                drive_add (widget, drive);
        }
}


static void
toggle_data_func (GtkCellLayout   *cell_layout,
                  GtkCellRenderer *renderer,
                  GtkTreeModel    *tree_model,
                  GtkTreeIter     *iter,
                  gpointer         user_data)
{
        GduDiskSelectionWidget *widget = GDU_DISK_SELECTION_WIDGET (user_data);
        GduPresentable *p;
        gboolean is_visible;
        gboolean is_toggled;

        gtk_tree_model_get (tree_model,
                            iter,
                            GDU_POOL_TREE_MODEL_COLUMN_PRESENTABLE, &p,
                            -1);

        is_visible = FALSE;
        is_toggled = FALSE;
        if (GDU_IS_DRIVE (p)) {
                is_visible = is_drive_selectable (widget, GDU_DRIVE (p), NULL);
                is_toggled = drive_is_selected (widget, p);
        }

        g_object_set (renderer,
                      "visible", is_visible,
                      "active", is_toggled,
                      NULL);

        g_object_unref (p);
}

static gboolean
update_all_func (GtkTreeModel *model,
                 GtkTreePath  *path,
                 GtkTreeIter  *iter,
                 gpointer      user_data)
{
        gtk_tree_model_row_changed (model, path, iter);
        return FALSE; /* keep iterating */
}

static void
on_disk_toggled (GtkCellRendererToggle *renderer,
                 const gchar           *path_string,
                 gpointer               user_data)
{
        GduDiskSelectionWidget *widget = GDU_DISK_SELECTION_WIDGET (user_data);
        GtkTreeIter iter;
        GtkTreePath *path;
        GduPresentable *p;

        if (!gtk_tree_model_get_iter_from_string (widget->priv->model,
                                                  &iter,
                                                  path_string))
                goto out;

        gtk_tree_model_get (widget->priv->model,
                            &iter,
                            GDU_POOL_TREE_MODEL_COLUMN_PRESENTABLE, &p,
                            -1);

        if (! (widget->priv->flags & GDU_DISK_SELECTION_WIDGET_FLAGS_ALLOW_MULTIPLE)) {
                /* untoggle all */
                g_list_foreach (widget->priv->selected_drives, (GFunc) g_object_unref, NULL);
                g_list_free (widget->priv->selected_drives);
                widget->priv->selected_drives = NULL;

                drive_toggle (widget, p);

                gtk_tree_model_foreach (widget->priv->model,
                                        update_all_func,
                                        NULL);
        } else {
                drive_toggle (widget, p);
                path = gtk_tree_model_get_path (widget->priv->model,
                                                &iter);
                gtk_tree_model_row_changed (widget->priv->model,
                                            path,
                                            &iter);
                gtk_tree_path_free (path);
        }

        g_object_unref (p);
        emit_changed (widget);

 out:
        ;
}

/* ---------------------------------------------------------------------------------------------------- */

static void
icon_data_func (GtkCellLayout   *cell_layout,
                GtkCellRenderer *renderer,
                GtkTreeModel    *tree_model,
                GtkTreeIter     *iter,
                gpointer         user_data)
{
        GduDiskSelectionWidget *widget = GDU_DISK_SELECTION_WIDGET (user_data);
        GduPresentable *p;
        GIcon *icon;
        gboolean sensitive;

        gtk_tree_model_get (tree_model,
                            iter,
                            GDU_POOL_TREE_MODEL_COLUMN_PRESENTABLE, &p,
                            GDU_POOL_TREE_MODEL_COLUMN_ICON, &icon,
                            -1);

        sensitive = TRUE;
        if (p != NULL && GDU_IS_DRIVE (p))
                sensitive = is_drive_selectable (widget, GDU_DRIVE (p), NULL);

        g_object_set (renderer,
                      "gicon", icon,
                      "sensitive", sensitive,
                      NULL);

        if (icon != NULL)
                g_object_unref (icon);
        if (p != NULL)
                g_object_unref (p);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
disk_name_data_func (GtkCellLayout   *cell_layout,
                     GtkCellRenderer *renderer,
                     GtkTreeModel    *tree_model,
                     GtkTreeIter     *iter,
                     gpointer         user_data)
{
        GduDiskSelectionWidget *widget = GDU_DISK_SELECTION_WIDGET (user_data);
        GtkTreeSelection *tree_selection;
        gchar *name;
        gchar *vpd_name;
        gchar *markup;
        gchar color[16];
        GtkStateType state;
        gboolean sensitive;
        GduPresentable *p;

        tree_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget->priv->tree_view));

        gtk_tree_model_get (tree_model,
                            iter,
                            GDU_POOL_TREE_MODEL_COLUMN_PRESENTABLE, &p,
                            GDU_POOL_TREE_MODEL_COLUMN_NAME, &name,
                            GDU_POOL_TREE_MODEL_COLUMN_VPD_NAME, &vpd_name,
                            -1);
        sensitive = TRUE;
        if (p != NULL && GDU_IS_DRIVE (p))
                sensitive = is_drive_selectable (widget, GDU_DRIVE (p), NULL);

        if (gtk_tree_selection_iter_is_selected (tree_selection, iter)) {
                if (gtk_widget_has_focus (GTK_WIDGET (widget->priv->tree_view)))
                        state = GTK_STATE_SELECTED;
                else
                        state = GTK_STATE_ACTIVE;
        } else {
                state = GTK_STATE_NORMAL;
        }
        gdu_util_get_mix_color (GTK_WIDGET (widget->priv->tree_view), state, color, sizeof (color));

        if (sensitive) {
                markup = g_strdup_printf ("<small><b>%s</b>\n"
                                          "<span fgcolor=\"%s\">%s</span></small>",
                                          name,
                                          color,
                                          vpd_name);
        } else {
                markup = g_strdup_printf ("<small><b>%s</b>\n"
                                          "%s</small>",
                                          name,
                                          vpd_name);
        }

        g_object_set (renderer,
                      "ellipsize", PANGO_ELLIPSIZE_MIDDLE,
                      "ellipsize-set", TRUE,
                      "markup", markup,
                      "sensitive", sensitive,
                      NULL);

        g_free (name);
        g_free (vpd_name);
        g_free (markup);

        if (p != NULL)
                g_object_unref (p);
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct {
        GduDiskSelectionWidget *widget;
        guint num_available_disks;
        guint64 largest_segment_for_all;
} CountData;

static gboolean
count_num_available_disks_func (GtkTreeModel *model,
                                GtkTreePath  *path,
                                GtkTreeIter  *iter,
                                gpointer      user_data)
{
        GduPresentable *p;
        CountData *data = user_data;

        gtk_tree_model_get (model,
                            iter,
                            GDU_POOL_TREE_MODEL_COLUMN_PRESENTABLE, &p,
                            -1);

        if (GDU_IS_DRIVE (p)) {
                if (is_drive_selectable (data->widget, GDU_DRIVE (p), NULL))
                        data->num_available_disks = data->num_available_disks + 1;
        }

        g_object_unref (p);

        return FALSE;
}

static gboolean
find_largest_segment_for_all_func (GtkTreeModel *model,
                                   GtkTreePath  *path,
                                   GtkTreeIter  *iter,
                                   gpointer      user_data)
{
        GduPresentable *p;
        CountData *data = user_data;

        gtk_tree_model_get (model,
                            iter,
                            GDU_POOL_TREE_MODEL_COLUMN_PRESENTABLE, &p,
                            -1);

        if (GDU_IS_DRIVE (p)) {
                guint64 largest_segment;

                if (gdu_drive_can_create_volume (GDU_DRIVE (p),
                                                 NULL,
                                                 &largest_segment,
                                                 NULL, /* total_free */
                                                 NULL)) {
                        if (largest_segment > data->largest_segment_for_all)
                                data->largest_segment_for_all = largest_segment;
                }

        }

        g_object_unref (p);

        return FALSE;
}

static void
get_sizes (GduDiskSelectionWidget *widget,
           guint                  *out_num_disks,
           guint                  *out_num_available_disks,
           guint64                *out_largest_segment_for_selected,
           guint64                *out_largest_segment_for_all)
{
        guint num_disks;
        CountData data;
        guint64 largest_segment_for_selected;
        GList *l;

        num_disks = 0;
        largest_segment_for_selected = G_MAXUINT64;
        data.widget = widget;
        data.num_available_disks = 0;
        data.largest_segment_for_all = 0;

        for (l = widget->priv->selected_drives; l != NULL; l = l->next) {
                GduPresentable *p = GDU_PRESENTABLE (l->data);
                guint64 largest_segment;
                gboolean whole_disk_is_uninitialized;

                if (gdu_drive_can_create_volume (GDU_DRIVE (p),
                                                 &whole_disk_is_uninitialized,
                                                 &largest_segment,
                                                 NULL, /* total_free */
                                                 NULL)) {
                        if (largest_segment < largest_segment_for_selected)
                                largest_segment_for_selected = largest_segment;

                        num_disks++;
                }
        }
        if (largest_segment_for_selected == G_MAXUINT64)
                largest_segment_for_selected = 0;

        gtk_tree_model_foreach (widget->priv->model,
                                count_num_available_disks_func,
                                &data);

        gtk_tree_model_foreach (widget->priv->model,
                                find_largest_segment_for_all_func,
                                &data);

        if (out_num_disks != NULL)
                *out_num_disks = num_disks;

        if (out_num_available_disks != NULL)
                *out_num_available_disks = data.num_available_disks;

        if (out_largest_segment_for_selected != NULL)
                *out_largest_segment_for_selected = largest_segment_for_selected;

        if (out_largest_segment_for_all != NULL)
                *out_largest_segment_for_all = data.largest_segment_for_all;
}

static void
notes_data_func (GtkCellLayout   *cell_layout,
                 GtkCellRenderer *renderer,
                 GtkTreeModel    *tree_model,
                 GtkTreeIter     *iter,
                 gpointer         user_data)
{
        GduDiskSelectionWidget *widget = GDU_DISK_SELECTION_WIDGET (user_data);
        GduPresentable *p;
        gchar *markup;
        gchar *s;
        gboolean sensitive;
        GduDevice *d;
        gboolean can_create_volume;
        guint64 largest_segment;
        gboolean whole_disk_is_uninitialized;
        guint num_partitions;
        gboolean is_partitionable;
        gboolean is_partitioned;
        guint64 total_free;
        guint64 remaining_size;
        gboolean is_selectable;
        gchar *reason;
        gchar *strsize;
        gchar *rem_strsize;

        d = NULL;
        markup = NULL;
        sensitive = TRUE;
        reason = NULL;

        gtk_tree_model_get (tree_model,
                            iter,
                            GDU_POOL_TREE_MODEL_COLUMN_PRESENTABLE, &p,
                            -1);

        if (!GDU_IS_DRIVE (p))
                goto out;

        d = gdu_presentable_get_device (p);

        num_partitions = 0;
        is_partitioned = FALSE;
        is_partitionable = TRUE;
        remaining_size = 0;
        if (d != NULL && gdu_device_is_partition_table (d)) {
                is_partitioned = TRUE;
                num_partitions = gdu_device_partition_table_get_count (d);
        }

        /* TODO: probably want a vfunc for this */
        if (GDU_IS_LINUX_LVM2_VOLUME_GROUP (p))
                is_partitionable = FALSE;

        can_create_volume = gdu_drive_can_create_volume (GDU_DRIVE (p),
                                                         &whole_disk_is_uninitialized,
                                                         &largest_segment,
                                                         &total_free,
                                                         NULL);

        is_selectable = is_drive_selectable (widget, GDU_DRIVE (p), &reason);
        sensitive = is_selectable;

        remaining_size = total_free - widget->priv->component_size;

        /* handle when the drive is not selectable */
        if (!is_selectable) {
                markup = reason; /* steal string */
                reason = NULL;
                goto out;
        }

        /* drive is selectable */
        if (drive_is_selected (widget, p)) {
                strsize = gdu_util_get_size_for_display (widget->priv->component_size, FALSE, FALSE);
                rem_strsize = gdu_util_get_size_for_display (remaining_size, FALSE, FALSE);

                if (whole_disk_is_uninitialized) {
                        if (widget->priv->component_size < SIZE_EPSILON) {
                                /* Translators: This is shown in the details column */
                                markup = g_strdup (_("The disk will be partitioned and a partition will be created"));
                        } else {
                                if (remaining_size < SIZE_EPSILON) {
                                        /* Translators: This is shown in the Details column.
                                         * First %s is the component size e.g. '42 GB'.
                                         */
                                        markup = g_strdup_printf (_("The disk will be partitioned and a %s partition "
                                                                    "will be created. "
                                                                    "Afterwards no space will be available."),
                                                                  strsize);
                                } else {
                                        /* Translators: This is shown in the Details column.
                                         * First %s is the component size e.g. '42 GB'.
                                         * Second %s is the remaining free space after the operation.
                                         */
                                        markup = g_strdup_printf (_("The disk will be partitioned and a %s partition "
                                                                    "will be created. "
                                                                    "Afterwards %s will be available."),
                                                                  strsize,
                                                                  rem_strsize);
                                }
                        }
                } else {
                        if (widget->priv->component_size < SIZE_EPSILON) {
                                if (is_partitionable) {
                                        /* Translators: This is shown in the details column */
                                        markup = g_strdup (_("A partition will be created"));
                                } else {
                                        /* Translators: This is shown in the details column */
                                        markup = g_strdup (_("A volume will be created"));
                                }
                        } else {
                                if (remaining_size < SIZE_EPSILON) {
                                        if (is_partitionable) {
                                                /* Translators: This is shown in the Details column.
                                                 * First %s is the component size e.g. '42 GB'.
                                                 */
                                                markup = g_strdup_printf (_("A %s partition will be created. "
                                                                            "Afterwards no space will be available."),
                                                                          strsize);
                                        } else {
                                                /* Translators: This is shown in the Details column.
                                                 * First %s is the component size e.g. '42 GB'.
                                                 */
                                                markup = g_strdup_printf (_("A %s volume will be created. "
                                                                            "Afterwards no space will be available."),
                                                                          strsize);
                                        }
                                } else {
                                        if (is_partitionable) {
                                                /* Translators: This is shown in the Details column.
                                                 * First %s is the component size e.g. '42 GB'.
                                                 * Second %s is the remaining free space after the operation.
                                                 */
                                                markup = g_strdup_printf (_("A %s partition will be created. "
                                                                            "Afterwards %s will be available."),
                                                                          strsize,
                                                                          rem_strsize);
                                        } else {
                                                /* Translators: This is shown in the Details column.
                                                 * First %s is the component size e.g. '42 GB'.
                                                 * Second %s is the remaining free space after the operation.
                                                 */
                                                markup = g_strdup_printf (_("A %s volume will be created. "
                                                                            "Afterwards %s will be available."),
                                                                          strsize,
                                                                          rem_strsize);
                                        }
                                }
                        }
                }
                g_free (strsize);
                g_free (rem_strsize);
        } else {
                gchar *strsize;

                /* Drive is not selected */

                strsize = gdu_util_get_size_for_display (largest_segment, FALSE, FALSE);
                if (whole_disk_is_uninitialized) {
                        /* Translators: This is shown in the Details column.
                         * %s is the component size e.g. '42 GB'.
                         */
                        markup = g_strdup_printf (_("Whole disk is uninitialized. %s available for use"),
                                                  strsize);
                } else {
                        if (!is_partitioned) {
                                /* Translators: This is shown in the Details column.
                                 * %s is the component size e.g. '42 GB'.
                                 */
                                markup = g_strdup_printf (_("%s available for use"), strsize);
                        } else {
                                if (num_partitions == 0) {
                                        /* Translators: This is shown in the Details column.
                                         * %s is the component size e.g. '42 GB'.
                                         */
                                        markup = g_strdup_printf (_("The disk has no partitions. "
                                                                    "%s available for use"),
                                                                  strsize);
                                } else {
                                        s = g_strdup_printf (dngettext (GETTEXT_PACKAGE,
                                                                        "The disk has %d partition",
                                                                        "The disk has %d partitions",
                                                                        num_partitions),
                                                             num_partitions);
                                        /* Translators: This is shown in the Details column.
                                         * First %s is the dngettext() result of "The disk has %d partitions.".
                                         * Second %s is the component size e.g. '42 GB'.
                                         */
                                        markup = g_strdup_printf (_("%s. Largest contiguous free block is %s"),
                                                                  s,
                                                                  strsize);
                                        g_free (s);
                                }
                        }
                }
                g_free (strsize);
        }

 out:
        if (markup == NULL)
                markup = g_strdup ("");
        s = g_strconcat ("<small>",
                         markup,
                         "</small>",
                         NULL);
        g_object_set (renderer,
                      "markup", s,
                      "wrap-width", DETAILS_WIDTH - DETAILS_MARGIN,
                      "sensitive", sensitive,
                      NULL);
        g_free (s);

        g_free (markup);
        g_object_unref (p);
        g_free (reason);

        if (d != NULL)
                g_object_unref (d);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
gdu_disk_selection_widget_constructed (GObject *object)
{
        GduDiskSelectionWidget *widget = GDU_DISK_SELECTION_WIDGET (object);
        GtkWidget *tree_view;
        GtkWidget *scrolled_window;
        GtkCellRenderer *renderer;
        GtkTreeViewColumn *column;
        GtkTreeModel *model;
        GPtrArray *pools;

        pools = g_ptr_array_new ();
        g_ptr_array_add (pools, widget->priv->pool);
        model = GTK_TREE_MODEL (gdu_pool_tree_model_new (pools,
                                                         NULL,
                                                         GDU_POOL_TREE_MODEL_FLAGS_NO_VOLUMES /* | GDU_POOL_TREE_MODEL_FLAGS_NO_UNALLOCATABLE_DRIVES */));
        g_ptr_array_unref (pools);

        widget->priv->model = gtk_tree_model_filter_new (model, NULL);
        g_object_unref (model);
#if 0
        gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (widget->priv->model),
                                                model_visible_func,
                                                widget,
                                                NULL);
#endif

        tree_view = gtk_tree_view_new_with_model (widget->priv->model);
        gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (tree_view), TRUE);
        widget->priv->tree_view = tree_view;

        column = gtk_tree_view_column_new ();
        /* Tranlators: this string is used for the column header */
        gtk_tree_view_column_set_title (column, _("Storage Devices"));
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
        gtk_tree_view_column_set_expand (column, TRUE);
        gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);

        renderer = gtk_cell_renderer_toggle_new ();
        if (! (widget->priv->flags & GDU_DISK_SELECTION_WIDGET_FLAGS_ALLOW_MULTIPLE))
                gtk_cell_renderer_toggle_set_radio (GTK_CELL_RENDERER_TOGGLE (renderer), TRUE);
        gtk_tree_view_column_pack_start (column, renderer, FALSE);
        g_signal_connect (renderer,
                          "toggled",
                          G_CALLBACK (on_disk_toggled),
                          widget);
        gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (column),
                                            renderer,
                                            toggle_data_func,
                                            widget,
                                            NULL);

        renderer = gtk_cell_renderer_pixbuf_new ();
        gtk_tree_view_column_pack_start (column, renderer, FALSE);
        gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (column),
                                            renderer,
                                            icon_data_func,
                                            widget,
                                            NULL);
        g_object_set (renderer,
                      "stock-size", GTK_ICON_SIZE_MENU,
                      NULL);

        renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_column_pack_start (column, renderer, TRUE);
        gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (column),
                                            renderer,
                                            disk_name_data_func,
                                            widget,
                                            NULL);

        column = gtk_tree_view_column_new ();
        /* Tranlators: this string is used for the column header */
        gtk_tree_view_column_set_title (column, _("Details"));
        gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
        gtk_tree_view_column_set_min_width (column, DETAILS_WIDTH);
        gtk_tree_view_column_set_max_width (column, DETAILS_WIDTH);
        gtk_tree_view_column_set_fixed_width (column, DETAILS_WIDTH);
        gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);

        renderer = gtk_cell_renderer_text_new ();
        gtk_tree_view_column_pack_end (column, renderer, FALSE);
        gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (column),
                                            renderer,
                                            notes_data_func,
                                            widget,
                                            NULL);
        g_object_set (renderer,
                      "xalign", 0.0,
                      "yalign", 0.0,
                      "wrap-mode", PANGO_WRAP_WORD_CHAR,
                      NULL);

        gtk_tree_view_set_show_expanders (GTK_TREE_VIEW (tree_view), FALSE);
        /*gtk_tree_view_set_enable_tree_lines (GTK_TREE_VIEW (tree_view), TRUE);*/
        gtk_tree_view_set_level_indentation (GTK_TREE_VIEW (tree_view), 16);
        gtk_tree_view_expand_all (GTK_TREE_VIEW (tree_view));

        scrolled_window = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                        GTK_POLICY_NEVER,
                                        GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                             GTK_SHADOW_IN);

        gtk_container_add (GTK_CONTAINER (scrolled_window), tree_view);

        gtk_box_pack_start (GTK_BOX (widget), scrolled_window, TRUE, TRUE, 0);

        /* -------------------------------------------------------------------------------- */

        g_signal_connect (widget->priv->pool,
                          "presentable-added",
                          G_CALLBACK (on_presentable_added),
                          widget);
        g_signal_connect (widget->priv->pool,
                          "presentable-removed",
                          G_CALLBACK (on_presentable_removed),
                          widget);
        g_signal_connect (widget->priv->pool,
                          "presentable-changed",
                          G_CALLBACK (on_presentable_changed),
                          widget);
        g_signal_connect (widget->priv->model,
                          "row-changed",
                          G_CALLBACK (on_row_changed),
                          widget);
        g_signal_connect (widget->priv->model,
                          "row-deleted",
                          G_CALLBACK (on_row_deleted),
                          widget);
        g_signal_connect (widget->priv->model,
                          "row-inserted",
                          G_CALLBACK (on_row_inserted),
                          widget);

        if (G_OBJECT_CLASS (gdu_disk_selection_widget_parent_class)->constructed != NULL)
                G_OBJECT_CLASS (gdu_disk_selection_widget_parent_class)->constructed (object);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
on_presentable_added (GduPool          *pool,
                      GduPresentable   *presentable,
                      gpointer          user_data)
{
        GduDiskSelectionWidget *widget = GDU_DISK_SELECTION_WIDGET (user_data);
        emit_changed (widget);
}

static void
on_presentable_removed (GduPool          *pool,
                        GduPresentable   *presentable,
                        gpointer          user_data)
{
        GduDiskSelectionWidget *widget = GDU_DISK_SELECTION_WIDGET (user_data);

        if (drive_is_selected (widget, presentable))
                drive_remove (widget, presentable);

        emit_changed (widget);
}

static void
on_presentable_changed (GduPool          *pool,
                        GduPresentable   *presentable,
                        gpointer          user_data)
{
        GduDiskSelectionWidget *widget = GDU_DISK_SELECTION_WIDGET (user_data);
        emit_changed (widget);
}

static void
on_row_changed (GtkTreeModel *tree_model,
                GtkTreePath  *path,
                GtkTreeIter  *iter,
                gpointer      user_data)
{
        GduDiskSelectionWidget *widget = GDU_DISK_SELECTION_WIDGET (user_data);
        emit_changed (widget);
}

static void
on_row_deleted (GtkTreeModel *tree_model,
                GtkTreePath  *path,
                gpointer      user_data)
{
        GduDiskSelectionWidget *widget = GDU_DISK_SELECTION_WIDGET (user_data);
        emit_changed (widget);
}

static void
on_row_inserted (GtkTreeModel *tree_model,
                 GtkTreePath  *path,
                 GtkTreeIter  *iter,
                 gpointer      user_data)
{
        GduDiskSelectionWidget *widget = GDU_DISK_SELECTION_WIDGET (user_data);
        emit_changed (widget);
}

/* ---------------------------------------------------------------------------------------------------- */

guint64
gdu_disk_selection_widget_get_component_size (GduDiskSelectionWidget *widget)
{
        g_return_val_if_fail (GDU_IS_DISK_SELECTION_WIDGET (widget), 0);
        return widget->priv->component_size;
}

void
gdu_disk_selection_widget_set_component_size (GduDiskSelectionWidget *widget,
                                              guint64                 component_size)
{
        g_return_if_fail (GDU_IS_DISK_SELECTION_WIDGET (widget));
        if (widget->priv->component_size != component_size) {
                widget->priv->component_size = component_size;
                gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (widget->priv->model));
        }
}

guint
gdu_disk_selection_widget_get_num_available_disks (GduDiskSelectionWidget *widget)
{
        guint num_available_disks;

        get_sizes (widget,
                   NULL,
                   &num_available_disks,
                   NULL,
                   NULL);

        return num_available_disks;
}

guint64
gdu_disk_selection_widget_get_largest_segment_for_selected (GduDiskSelectionWidget *widget)
{
        guint64 largest_segment_for_selected;

        g_return_val_if_fail (GDU_IS_DISK_SELECTION_WIDGET (widget), 0);

        get_sizes (widget,
                   NULL,
                   NULL,
                   &largest_segment_for_selected,
                   NULL);

        return largest_segment_for_selected;
}

guint64
gdu_disk_selection_widget_get_largest_segment_for_all (GduDiskSelectionWidget *widget)
{
        guint64 largest_segment_for_all;

        g_return_val_if_fail (GDU_IS_DISK_SELECTION_WIDGET (widget), 0);

        get_sizes (widget,
                   NULL,
                   NULL,
                   NULL,
                   &largest_segment_for_all);

        return largest_segment_for_all;
}

/* ---------------------------------------------------------------------------------------------------- */

static void
emit_changed (GduDiskSelectionWidget *widget)
{
        g_signal_emit (widget, signals[CHANGED_SIGNAL], 0);
}

