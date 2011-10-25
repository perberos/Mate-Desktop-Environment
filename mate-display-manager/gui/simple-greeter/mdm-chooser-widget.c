/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 Ray Strode <rstrode@redhat.com>
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
 * Copyright (C) 2008 Red Hat, Inc.
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
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <syslog.h>
#include <locale.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include "mdm-chooser-widget.h"
#include "mdm-scrollable-widget.h"
#include "mdm-cell-renderer-timer.h"
#include "mdm-timer.h"

#define MDM_CHOOSER_WIDGET_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_CHOOSER_WIDGET, MdmChooserWidgetPrivate))

#ifndef MDM_CHOOSER_WIDGET_DEFAULT_ICON_SIZE
#define MDM_CHOOSER_WIDGET_DEFAULT_ICON_SIZE 64
#endif

typedef enum {
        MDM_CHOOSER_WIDGET_STATE_GROWN = 0,
        MDM_CHOOSER_WIDGET_STATE_GROWING,
        MDM_CHOOSER_WIDGET_STATE_SHRINKING,
        MDM_CHOOSER_WIDGET_STATE_SHRUNK,
} MdmChooserWidgetState;

struct MdmChooserWidgetPrivate
{
        GtkWidget                *frame;
        GtkWidget                *frame_alignment;
        GtkWidget                *scrollable_widget;

        GtkWidget                *items_view;
        GtkListStore             *list_store;

        GtkTreeModelFilter       *model_filter;
        GtkTreeModelSort         *model_sorter;

        GdkPixbuf                *is_in_use_pixbuf;

        /* row for the list_store model */
        GtkTreeRowReference      *active_row;
        GtkTreeRowReference      *separator_row;

        GHashTable               *rows_with_timers;

        GtkTreeViewColumn        *status_column;
        GtkTreeViewColumn        *image_column;

        char                     *inactive_text;
        char                     *active_text;
        char                     *in_use_message;

        gint                     number_of_normal_rows;
        gint                     number_of_separated_rows;
        gint                     number_of_rows_with_status;
        gint                     number_of_rows_with_images;
        gint                     number_of_active_timers;

        guint                    update_idle_id;
        guint                    update_separator_idle_id;
        guint                    update_cursor_idle_id;
        guint                    update_visibility_idle_id;
        guint                    update_items_idle_id;
        guint                    timer_animation_timeout_id;

        gboolean                 list_visible;

        guint32                  should_hide_inactive_items : 1;
        guint32                  emit_activated_after_resize_animation : 1;

        MdmChooserWidgetPosition separator_position;
        MdmChooserWidgetState    state;

        double                   active_row_normalized_position;
};

enum {
        PROP_0,
        PROP_INACTIVE_TEXT,
        PROP_ACTIVE_TEXT,
        PROP_LIST_VISIBLE
};

enum {
        ACTIVATED = 0,
        DEACTIVATED,
        LOADED,
        NUMBER_OF_SIGNALS
};

static guint    signals[NUMBER_OF_SIGNALS];

static void     mdm_chooser_widget_class_init  (MdmChooserWidgetClass *klass);
static void     mdm_chooser_widget_init        (MdmChooserWidget      *chooser_widget);
static void     mdm_chooser_widget_finalize    (GObject               *object);

static void     update_timer_from_time         (MdmChooserWidget    *widget,
                                                GtkTreeRowReference *row,
                                                double               now);

G_DEFINE_TYPE (MdmChooserWidget, mdm_chooser_widget, GTK_TYPE_ALIGNMENT)

enum {
        CHOOSER_IMAGE_COLUMN = 0,
        CHOOSER_NAME_COLUMN,
        CHOOSER_COMMENT_COLUMN,
        CHOOSER_PRIORITY_COLUMN,
        CHOOSER_ITEM_IS_IN_USE_COLUMN,
        CHOOSER_ITEM_IS_SEPARATED_COLUMN,
        CHOOSER_ITEM_IS_VISIBLE_COLUMN,
        CHOOSER_TIMER_START_TIME_COLUMN,
        CHOOSER_TIMER_DURATION_COLUMN,
        CHOOSER_TIMER_VALUE_COLUMN,
        CHOOSER_ID_COLUMN,
        CHOOSER_LOAD_FUNC_COLUMN,
        CHOOSER_LOAD_DATA_COLUMN,
        NUMBER_OF_CHOOSER_COLUMNS
};

static gboolean
find_item (MdmChooserWidget *widget,
           const char       *id,
           GtkTreeIter      *iter)
{
        GtkTreeModel *model;
        gboolean      found_item;

        g_assert (MDM_IS_CHOOSER_WIDGET (widget));
        g_assert (id != NULL);

        found_item = FALSE;
        model = GTK_TREE_MODEL (widget->priv->list_store);

        if (!gtk_tree_model_get_iter_first (model, iter)) {
                return FALSE;
        }

        do {
                char *item_id;

                gtk_tree_model_get (model,
                                    iter,
                                    CHOOSER_ID_COLUMN,
                                    &item_id,
                                    -1);

                g_assert (item_id != NULL);

                if (strcmp (id, item_id) == 0) {
                        found_item = TRUE;
                }
                g_free (item_id);

        } while (!found_item && gtk_tree_model_iter_next (model, iter));

        return found_item;
}

typedef struct {
        MdmChooserWidget           *widget;
        MdmChooserUpdateForeachFunc func;
        gpointer                    user_data;
} UpdateForeachData;

static gboolean
foreach_item (GtkTreeModel      *model,
              GtkTreePath       *path,
              GtkTreeIter       *iter,
              UpdateForeachData *data)
{
        GdkPixbuf *image;
        char      *name;
        char      *comment;
        gboolean   in_use;
        gboolean   is_separate;
        gboolean   res;
        char      *id;
        gulong     priority;

        gtk_tree_model_get (model,
                            iter,
                            CHOOSER_ID_COLUMN, &id,
                            CHOOSER_IMAGE_COLUMN, &image,
                            CHOOSER_NAME_COLUMN, &name,
                            CHOOSER_COMMENT_COLUMN, &comment,
                            CHOOSER_PRIORITY_COLUMN, &priority,
                            CHOOSER_ITEM_IS_IN_USE_COLUMN, &in_use,
                            CHOOSER_ITEM_IS_SEPARATED_COLUMN, &is_separate,
                            -1);
        res = data->func (data->widget,
                          (const char *)id,
                          &image,
                          &name,
                          &comment,
                          &priority,
                          &in_use,
                          &is_separate,
                          data->user_data);
        if (res) {
                gtk_list_store_set (GTK_LIST_STORE (model),
                                    iter,
                                    CHOOSER_ID_COLUMN, id,
                                    CHOOSER_IMAGE_COLUMN, image,
                                    CHOOSER_NAME_COLUMN, name,
                                    CHOOSER_COMMENT_COLUMN, comment,
                                    CHOOSER_PRIORITY_COLUMN, priority,
                                    CHOOSER_ITEM_IS_IN_USE_COLUMN, in_use,
                                    CHOOSER_ITEM_IS_SEPARATED_COLUMN, is_separate,
                                    -1);
        }

        g_free (name);
        g_free (comment);
        if (image != NULL) {
                g_object_unref (image);
        }

        return FALSE;
}

void
mdm_chooser_widget_update_foreach_item (MdmChooserWidget           *widget,
                                        MdmChooserUpdateForeachFunc func,
                                        gpointer                    user_data)
{
        UpdateForeachData fdata;

        fdata.widget = widget;
        fdata.func = func;
        fdata.user_data = user_data;
        gtk_tree_model_foreach (GTK_TREE_MODEL (widget->priv->list_store),
                                (GtkTreeModelForeachFunc) foreach_item,
                                &fdata);
}

static void
translate_list_path_to_view_path (MdmChooserWidget  *widget,
                                  GtkTreePath      **path)
{
        GtkTreePath *filtered_path;
        GtkTreePath *sorted_path;

        /* the child model is the source for the filter */
        filtered_path = gtk_tree_model_filter_convert_child_path_to_path (widget->priv->model_filter,
                                                                          *path);
        sorted_path = gtk_tree_model_sort_convert_child_path_to_path (widget->priv->model_sorter,
                                                                      filtered_path);
        gtk_tree_path_free (filtered_path);

        gtk_tree_path_free (*path);
        *path = sorted_path;
}


static void
translate_view_path_to_list_path (MdmChooserWidget  *widget,
                                  GtkTreePath      **path)
{
        GtkTreePath *filtered_path;
        GtkTreePath *list_path;

        /* the child model is the source for the filter */
        filtered_path = gtk_tree_model_sort_convert_path_to_child_path (widget->priv->model_sorter,
                                                                        *path);

        list_path = gtk_tree_model_filter_convert_path_to_child_path (widget->priv->model_filter,
                                                                      filtered_path);
        gtk_tree_path_free (filtered_path);

        gtk_tree_path_free (*path);
        *path = list_path;
}

static GtkTreePath *
get_list_path_to_active_row (MdmChooserWidget *widget)
{
        GtkTreePath *path;

        if (widget->priv->active_row == NULL) {
                return NULL;
        }

        path = gtk_tree_row_reference_get_path (widget->priv->active_row);
        if (path == NULL) {
                return NULL;
        }

        return path;
}

static GtkTreePath *
get_view_path_to_active_row (MdmChooserWidget *widget)
{
        GtkTreePath *path;

        path = get_list_path_to_active_row (widget);
        if (path == NULL) {
                return NULL;
        }

        translate_list_path_to_view_path (widget, &path);

        return path;
}

static char *
get_active_item_id (MdmChooserWidget *widget,
                    GtkTreeIter      *iter)
{
        char         *item_id;
        GtkTreeModel *model;
        GtkTreePath  *path;

        g_return_val_if_fail (MDM_IS_CHOOSER_WIDGET (widget), NULL);

        model = GTK_TREE_MODEL (widget->priv->list_store);
        item_id = NULL;

        if (widget->priv->active_row == NULL) {
                return NULL;
        }

        path = get_list_path_to_active_row (widget);
        if (path == NULL) {
                return NULL;
        }

        if (gtk_tree_model_get_iter (model, iter, path)) {
                gtk_tree_model_get (model,
                                    iter,
                                    CHOOSER_ID_COLUMN,
                                    &item_id,
                                    -1);
        }
        gtk_tree_path_free (path);

        return item_id;
}

char *
mdm_chooser_widget_get_active_item (MdmChooserWidget *widget)
{
        GtkTreeIter iter;

        return get_active_item_id (widget, &iter);
}

static void
get_selected_list_path (MdmChooserWidget *widget,
                        GtkTreePath     **pathp)
{
        GtkTreeSelection    *selection;
        GtkTreeModel        *sort_model;
        GtkTreeIter          sorted_iter;
        GtkTreePath         *path;

        path = NULL;

        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget->priv->items_view));
        if (gtk_tree_selection_get_selected (selection, &sort_model, &sorted_iter)) {

                g_assert (sort_model == GTK_TREE_MODEL (widget->priv->model_sorter));

                path = gtk_tree_model_get_path (sort_model, &sorted_iter);

                translate_view_path_to_list_path (widget, &path);
        } else {
                g_debug ("MdmChooserWidget: no rows selected");
        }

        *pathp = path;
}

char *
mdm_chooser_widget_get_selected_item (MdmChooserWidget *widget)
{
        GtkTreeIter   iter;
        GtkTreeModel *model;
        GtkTreePath  *path;
        char         *id;

        id = NULL;

        get_selected_list_path (widget, &path);

        if (path == NULL) {
                return NULL;
        }

        model = GTK_TREE_MODEL (widget->priv->list_store);

        if (gtk_tree_model_get_iter (model, &iter, path)) {
                gtk_tree_model_get (model,
                                    &iter,
                                    CHOOSER_ID_COLUMN,
                                    &id,
                                    -1);
        }

        gtk_tree_path_free (path);

        return id;
}

void
mdm_chooser_widget_set_selected_item (MdmChooserWidget *widget,
                                      const char       *id)
{
        GtkTreeIter       iter;
        GtkTreeSelection *selection;
        GtkTreeModel     *model;

        g_return_if_fail (MDM_IS_CHOOSER_WIDGET (widget));

        g_debug ("MdmChooserWidget: setting selected item '%s'",
                 id ? id : "(null)");

        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget->priv->items_view));

        model = GTK_TREE_MODEL (widget->priv->list_store);

        if (find_item (widget, id, &iter)) {
                GtkTreePath  *path;

                path = gtk_tree_model_get_path (model, &iter);
                translate_list_path_to_view_path (widget, &path);

                gtk_tree_selection_select_path (selection, path);

                gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (widget->priv->items_view),
                                              path,
                                              NULL,
                                              TRUE,
                                              0.5,
                                              0.0);

                gtk_tree_view_set_cursor (GTK_TREE_VIEW (widget->priv->items_view),
                                          path,
                                          NULL,
                                          FALSE);
                gtk_tree_path_free (path);
        } else {
                gtk_tree_selection_unselect_all (selection);
        }
}

static void
activate_from_item_id (MdmChooserWidget *widget,
                       const char       *item_id)
{
        GtkTreeModel *model;
        GtkTreePath  *path;
        GtkTreeIter   iter;
        char         *path_str;

        model = GTK_TREE_MODEL (widget->priv->list_store);
        path = NULL;

        if (find_item (widget, item_id, &iter)) {
                path = gtk_tree_model_get_path (model, &iter);

                path_str = gtk_tree_path_to_string (path);
                g_debug ("MdmChooserWidget: got list path '%s'", path_str);
                g_free (path_str);

                translate_list_path_to_view_path (widget, &path);

                path_str = gtk_tree_path_to_string (path);
                g_debug ("MdmChooserWidget: translated to view path '%s'", path_str);
                g_free (path_str);
        }

        if (path == NULL) {
                g_debug ("MdmChooserWidget: unable to activate - path for item '%s' not found", item_id);
                return;
        }

        gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (widget->priv->items_view),
                                      path,
                                      NULL,
                                      TRUE,
                                      0.5,
                                      0.0);

        gtk_tree_view_set_cursor (GTK_TREE_VIEW (widget->priv->items_view),
                                  path,
                                  NULL,
                                  FALSE);

        gtk_tree_view_row_activated (GTK_TREE_VIEW (widget->priv->items_view),
                                     path,
                                     NULL);
        gtk_tree_path_free (path);
}

static void
set_frame_text (MdmChooserWidget *widget,
                const char       *text)
{
        GtkWidget *label;

        label = gtk_frame_get_label_widget (GTK_FRAME (widget->priv->frame));

        if (text == NULL && label != NULL) {
                gtk_frame_set_label_widget (GTK_FRAME (widget->priv->frame),
                                            NULL);
                gtk_alignment_set_padding (GTK_ALIGNMENT (widget->priv->frame_alignment),
                                           0, 0, 0, 0);
        } else if (text != NULL && label == NULL) {
                label = gtk_label_new ("");
                gtk_label_set_mnemonic_widget (GTK_LABEL (label),
                                               widget->priv->items_view);
                gtk_widget_show (label);
                gtk_frame_set_label_widget (GTK_FRAME (widget->priv->frame),
                                            label);
                gtk_alignment_set_padding (GTK_ALIGNMENT (widget->priv->frame_alignment),
                                           0, 0, 0, 0);
        }

        if (label != NULL && text != NULL) {
                char *markup;
                markup = g_strdup_printf ("%s", text);
                gtk_label_set_markup_with_mnemonic (GTK_LABEL (label), markup);
                g_free (markup);
        }
}

static void
on_shrink_animation_step (MdmScrollableWidget *scrollable_widget,
                          double               progress,
                          int                 *new_height,
                          MdmChooserWidget    *widget)
{
        GtkTreePath   *active_row_path;
        const double   final_alignment = 0.5;
        double         row_alignment;

        active_row_path = get_view_path_to_active_row (widget);
        row_alignment = widget->priv->active_row_normalized_position + progress * (final_alignment - widget->priv->active_row_normalized_position);

        gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (widget->priv->items_view),
                                     active_row_path, NULL, TRUE, row_alignment, 0.0);
        gtk_tree_path_free (active_row_path);
}

static gboolean
update_separator_visibility (MdmChooserWidget *widget)
{
        GtkTreePath *separator_path;
        GtkTreeIter  iter;
        gboolean     is_visible;

        g_debug ("MdmChooserWidget: updating separator visibility");

        separator_path = gtk_tree_row_reference_get_path (widget->priv->separator_row);

        if (separator_path == NULL) {
                goto out;
        }

        gtk_tree_model_get_iter (GTK_TREE_MODEL (widget->priv->list_store),
                                 &iter, separator_path);

        if (widget->priv->number_of_normal_rows > 0 &&
            widget->priv->number_of_separated_rows > 0 &&
            widget->priv->state != MDM_CHOOSER_WIDGET_STATE_SHRUNK) {
                is_visible = TRUE;
        } else {
                is_visible = FALSE;
        }

        gtk_list_store_set (widget->priv->list_store,
                            &iter,
                            CHOOSER_ITEM_IS_VISIBLE_COLUMN, is_visible,
                            -1);

 out:
        widget->priv->update_separator_idle_id = 0;
        return FALSE;
}

static void
queue_update_separator_visibility (MdmChooserWidget *widget)
{
        if (widget->priv->update_separator_idle_id == 0) {
                g_debug ("MdmChooserWidget: queuing update separator visibility");

                widget->priv->update_separator_idle_id =
                        g_idle_add ((GSourceFunc) update_separator_visibility, widget);
        }
}

static gboolean
update_visible_items (MdmChooserWidget *widget)
{
        GtkTreePath *path;
        GtkTreePath *end;
        GtkTreeIter  iter;

        if (! gtk_tree_view_get_visible_range (GTK_TREE_VIEW (widget->priv->items_view), &path, &end)) {
                g_debug ("MdmChooserWidget: Unable to get visible range");
                goto out;
        }

        for (; gtk_tree_path_compare (path, end) <= 0; gtk_tree_path_next (path)) {
                char                        *id;
                gpointer                     user_data;
                MdmChooserWidgetItemLoadFunc func;

                if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (widget->priv->model_sorter), &iter, path))
                        break;

                id = NULL;
                gtk_tree_model_get (GTK_TREE_MODEL (widget->priv->model_sorter),
                                    &iter,
                                    CHOOSER_ID_COLUMN, &id,
                                    CHOOSER_LOAD_FUNC_COLUMN, &func,
                                    CHOOSER_LOAD_DATA_COLUMN, &user_data,
                                    -1);
                if (id != NULL && func != NULL) {
                        GtkTreeIter  child_iter;
                        GtkTreeIter  list_iter;

                        g_debug ("Updating for %s", id);

                        gtk_tree_model_sort_convert_iter_to_child_iter (widget->priv->model_sorter,
                                                                        &child_iter,
                                                                        &iter);

                        gtk_tree_model_filter_convert_iter_to_child_iter (widget->priv->model_filter,
                                                                          &list_iter,
                                                                          &child_iter);
                        /* remove the func so it doesn't need to load again */
                        gtk_list_store_set (GTK_LIST_STORE (widget->priv->list_store),
                                            &list_iter,
                                            CHOOSER_LOAD_FUNC_COLUMN, NULL,
                                            -1);

                        func (widget, id, user_data);
                }

                g_free (id);
        }

        gtk_tree_path_free (path);
        gtk_tree_path_free (end);
 out:
        widget->priv->update_items_idle_id = 0;

        return FALSE;
}

static void
set_chooser_list_visible (MdmChooserWidget *widget,
                          gboolean          is_visible)
{
        if (widget->priv->list_visible != is_visible) {
                widget->priv->list_visible = is_visible;
                g_object_notify (G_OBJECT (widget), "list-visible");
        }
}

static gboolean
update_chooser_visibility (MdmChooserWidget *widget)
{
        update_visible_items (widget);

        if (mdm_chooser_widget_get_number_of_items (widget) > 0) {
                gtk_widget_show (widget->priv->frame);
                set_chooser_list_visible (widget, TRUE);
        } else {
                gtk_widget_hide (widget->priv->frame);
                set_chooser_list_visible (widget, FALSE);
        }

        widget->priv->update_visibility_idle_id = 0;

        return FALSE;
}

static inline gboolean
iters_equal (GtkTreeIter *a,
             GtkTreeIter *b)
{
        if (a->stamp != b->stamp)
                return FALSE;

        if (a->user_data != b->user_data)
                return FALSE;

        /* user_data2 and user_data3 are not used in GtkListStore */

        return TRUE;
}

static void
set_inactive_items_visible (MdmChooserWidget *widget,
                            gboolean          should_show)
{
        GtkTreeModel *model;
        GtkTreeModel *view_model;
        char         *active_item_id;
        GtkTreeIter   active_item_iter;
        GtkTreeIter   iter;

        g_debug ("setting inactive items visible");

        active_item_id = get_active_item_id (widget, &active_item_iter);
        if (active_item_id == NULL) {
                g_debug ("MdmChooserWidget: No active item set");
        }

        model = GTK_TREE_MODEL (widget->priv->list_store);

        if (!gtk_tree_model_get_iter_first (model, &iter)) {
                goto out;
        }

        /* unset tree view model to hide row add/remove signals from gail */
        view_model = gtk_tree_view_get_model (GTK_TREE_VIEW (widget->priv->items_view));
        g_object_ref (view_model);
        gtk_tree_view_set_model (GTK_TREE_VIEW (widget->priv->items_view), NULL);

        g_debug ("MdmChooserWidget: Setting inactive items visible: %s", should_show ? "true" : "false");

        do {
                if (active_item_id == NULL || !iters_equal (&active_item_iter, &iter)) {
                        /* inactive item */
                        gtk_list_store_set (widget->priv->list_store,
                                            &iter,
                                            CHOOSER_ITEM_IS_VISIBLE_COLUMN, should_show,
                                            -1);
                } else {
                        /* always show the active item */
                        gtk_list_store_set (widget->priv->list_store,
                                            &iter,
                                            CHOOSER_ITEM_IS_VISIBLE_COLUMN, TRUE,
                                            -1);
                }
        } while (gtk_tree_model_iter_next (model, &iter));

        gtk_tree_view_set_model (GTK_TREE_VIEW (widget->priv->items_view), view_model);
        g_object_unref (view_model);

        queue_update_separator_visibility (widget);

 out:
        g_free (active_item_id);
}

static void
on_shrink_animation_complete (MdmScrollableWidget *scrollable_widget,
                              MdmChooserWidget    *widget)
{
        g_assert (widget->priv->state == MDM_CHOOSER_WIDGET_STATE_SHRINKING);

        g_debug ("MdmChooserWidget: shrink complete");

        widget->priv->active_row_normalized_position = 0.5;
        set_inactive_items_visible (MDM_CHOOSER_WIDGET (widget), FALSE);
        gtk_tree_view_set_enable_search (GTK_TREE_VIEW (widget->priv->items_view), FALSE);
        widget->priv->state = MDM_CHOOSER_WIDGET_STATE_SHRUNK;

        queue_update_separator_visibility (widget);

        if (widget->priv->emit_activated_after_resize_animation) {
                g_signal_emit (widget, signals[ACTIVATED], 0);
                widget->priv->emit_activated_after_resize_animation = FALSE;
        }
}

static int
get_height_of_row_at_path (MdmChooserWidget *widget,
                           GtkTreePath      *path)
{
        GdkRectangle area;

        gtk_tree_view_get_background_area (GTK_TREE_VIEW (widget->priv->items_view),
                                           path, NULL, &area);

        return area.height;
}

static double
get_normalized_position_of_row_at_path (MdmChooserWidget *widget,
                                        GtkTreePath      *path)
{
        GdkRectangle area_of_row_at_path;
        GdkRectangle area_of_visible_rows;
        GtkAllocation items_view_allocation;

        gtk_widget_get_allocation (widget->priv->items_view, &items_view_allocation);

        gtk_tree_view_get_background_area (GTK_TREE_VIEW (widget->priv->items_view),
                                           path, NULL, &area_of_row_at_path);

        gtk_tree_view_convert_tree_to_widget_coords (GTK_TREE_VIEW (widget->priv->items_view),
                                                     area_of_visible_rows.x,
                                                     area_of_visible_rows.y,
                                                     &area_of_visible_rows.x,
                                                     &area_of_visible_rows.y);
        return CLAMP (((double) area_of_row_at_path.y) / items_view_allocation.height, 0.0, 1.0);
}

static void
start_shrink_animation (MdmChooserWidget *widget)
{
       GtkTreePath *active_row_path;
       int          active_row_height;
       int          number_of_visible_rows;

       g_assert (widget->priv->active_row != NULL);

       number_of_visible_rows = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (widget->priv->model_sorter), NULL);

       if (number_of_visible_rows <= 1) {
               on_shrink_animation_complete (MDM_SCROLLABLE_WIDGET (widget->priv->scrollable_widget),
                                             widget);
               return;
       }

       active_row_path = get_view_path_to_active_row (widget);
       active_row_height = get_height_of_row_at_path (widget, active_row_path);
       widget->priv->active_row_normalized_position = get_normalized_position_of_row_at_path (widget, active_row_path);
       gtk_tree_path_free (active_row_path);

       mdm_scrollable_widget_slide_to_height (MDM_SCROLLABLE_WIDGET (widget->priv->scrollable_widget),
                                              active_row_height,
                                              (MdmScrollableWidgetSlideStepFunc)
                                              on_shrink_animation_step, widget,
                                              (MdmScrollableWidgetSlideDoneFunc)
                                              on_shrink_animation_complete, widget);
}

static char *
get_first_item (MdmChooserWidget *widget)
{
        GtkTreeModel *model;
        GtkTreeIter   iter;
        char         *id;

        model = GTK_TREE_MODEL (widget->priv->list_store);

        if (!gtk_tree_model_get_iter_first (model, &iter)) {
                g_assert_not_reached ();
        }

        gtk_tree_model_get (model, &iter,
                            CHOOSER_ID_COLUMN, &id, -1);
        return id;
}

static gboolean
activate_if_one_item (MdmChooserWidget *widget)
{
        char *id;

        g_debug ("MdmChooserWidget: attempting to activate single item");

        if (mdm_chooser_widget_get_number_of_items (widget) != 1) {
                g_debug ("MdmChooserWidget: unable to activate single item - has %d items", mdm_chooser_widget_get_number_of_items (widget));
                return FALSE;
        }

        id = get_first_item (widget);
        if (id != NULL) {
                mdm_chooser_widget_set_active_item (widget, id);
                g_free (id);
        }

        return FALSE;
}

static void
_grab_focus (GtkWidget *widget)
{
        GtkWidget *foc_widget;

        foc_widget = MDM_CHOOSER_WIDGET (widget)->priv->items_view;
        g_debug ("MdmChooserWidget: grabbing focus");
        if (! gtk_widget_get_realized (foc_widget)) {
                g_debug ("MdmChooserWidget: not grabbing focus - not realized");
                return;
        }

        if (gtk_widget_has_focus (foc_widget)) {
                g_debug ("MdmChooserWidget: not grabbing focus - already has it");
                return;
        }

        gtk_widget_grab_focus (foc_widget);
}

static void
queue_update_visible_items (MdmChooserWidget *widget)
{
        if (widget->priv->update_items_idle_id != 0) {
                g_source_remove (widget->priv->update_items_idle_id);
        }

        widget->priv->update_items_idle_id =
                g_timeout_add (100, (GSourceFunc) update_visible_items, widget);
}

static void
on_grow_animation_complete (MdmScrollableWidget *scrollable_widget,
                            MdmChooserWidget    *widget)
{
        g_assert (widget->priv->state == MDM_CHOOSER_WIDGET_STATE_GROWING);
        widget->priv->state = MDM_CHOOSER_WIDGET_STATE_GROWN;
        gtk_tree_view_set_enable_search (GTK_TREE_VIEW (widget->priv->items_view), TRUE);
        queue_update_visible_items (widget);

        _grab_focus (GTK_WIDGET (widget));
}

static int
get_number_of_on_screen_rows (MdmChooserWidget *widget)
{
        GtkTreePath *start_path;
        GtkTreePath *end_path;
        int         *start_index;
        int         *end_index;
        int          number_of_rows;

        if (!gtk_tree_view_get_visible_range (GTK_TREE_VIEW (widget->priv->items_view),
                                              &start_path, &end_path)) {
                return 0;
        }

        start_index = gtk_tree_path_get_indices (start_path);
        end_index = gtk_tree_path_get_indices (end_path);

        number_of_rows = *end_index - *start_index + 1;

        gtk_tree_path_free (start_path);
        gtk_tree_path_free (end_path);

        return number_of_rows;
}

static void
on_grow_animation_step (MdmScrollableWidget *scrollable_widget,
                        double               progress,
                        int                 *new_height,
                        MdmChooserWidget    *widget)
{
        int number_of_visible_rows;
        int number_of_on_screen_rows;

        number_of_visible_rows = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (widget->priv->model_sorter), NULL);
        number_of_on_screen_rows = get_number_of_on_screen_rows (widget);

        GtkRequisition scrollable_widget_req;
        gtk_widget_get_requisition (gtk_bin_get_child (GTK_BIN (scrollable_widget)), &scrollable_widget_req);
        *new_height = scrollable_widget_req.height;
}

static void
start_grow_animation (MdmChooserWidget *widget)
{
        GtkRequisition scrollable_widget_req;
        gtk_widget_get_requisition (gtk_bin_get_child (GTK_BIN (widget->priv->scrollable_widget)),
                                    &scrollable_widget_req);

        set_inactive_items_visible (widget, TRUE);

        mdm_scrollable_widget_slide_to_height (MDM_SCROLLABLE_WIDGET (widget->priv->scrollable_widget),
                                               scrollable_widget_req.height,
                                               (MdmScrollableWidgetSlideStepFunc)
                                               on_grow_animation_step, widget,
                                               (MdmScrollableWidgetSlideDoneFunc)
                                               on_grow_animation_complete, widget);
}

static void
skip_resize_animation (MdmChooserWidget *widget)
{
        if (widget->priv->state == MDM_CHOOSER_WIDGET_STATE_SHRINKING) {
                set_inactive_items_visible (MDM_CHOOSER_WIDGET (widget), FALSE);
                gtk_tree_view_set_enable_search (GTK_TREE_VIEW (widget->priv->items_view), FALSE);
                widget->priv->state = MDM_CHOOSER_WIDGET_STATE_SHRUNK;
        } else if (widget->priv->state == MDM_CHOOSER_WIDGET_STATE_GROWING) {
                set_inactive_items_visible (MDM_CHOOSER_WIDGET (widget), TRUE);
                gtk_tree_view_set_enable_search (GTK_TREE_VIEW (widget->priv->items_view), TRUE);
                widget->priv->state = MDM_CHOOSER_WIDGET_STATE_GROWN;
                _grab_focus (GTK_WIDGET (widget));
        }
}

static void
mdm_chooser_widget_grow (MdmChooserWidget *widget)
{
        if (widget->priv->state == MDM_CHOOSER_WIDGET_STATE_GROWN) {
                g_debug ("MdmChooserWidget: Asking for grow but already grown");
                return;
        }

        if (widget->priv->state == MDM_CHOOSER_WIDGET_STATE_SHRINKING) {
                mdm_scrollable_widget_stop_sliding (MDM_SCROLLABLE_WIDGET (widget->priv->scrollable_widget));
        }

        gtk_alignment_set (GTK_ALIGNMENT (widget->priv->frame_alignment),
                           0.0, 0.0, 1.0, 1.0);

        set_frame_text (widget, widget->priv->inactive_text);

        widget->priv->state = MDM_CHOOSER_WIDGET_STATE_GROWING;

        if (gtk_widget_get_visible (GTK_WIDGET (widget))) {
                start_grow_animation (widget);
        } else {
                skip_resize_animation (widget);
        }
}

static gboolean
move_cursor_to_top (MdmChooserWidget *widget)
{
        GtkTreeModel *model;
        GtkTreePath  *path;
        GtkTreeIter   iter;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (widget->priv->items_view));
        path = gtk_tree_path_new_first ();
        if (gtk_tree_model_get_iter (model, &iter, path)) {
                gtk_tree_view_set_cursor (GTK_TREE_VIEW (widget->priv->items_view),
                                          path,
                                          NULL,
                                          FALSE);

                gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (widget->priv->items_view),
                                              path,
                                              NULL,
                                              TRUE,
                                              0.5,
                                              0.0);

        }
        gtk_tree_path_free (path);

        widget->priv->update_cursor_idle_id = 0;
        return FALSE;
}

static gboolean
clear_selection (MdmChooserWidget *widget)
{
        GtkTreeSelection *selection;
        GtkWidget        *window;

        g_debug ("MdmChooserWidget: clearing selection");

        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget->priv->items_view));
        gtk_tree_selection_unselect_all (selection);

        window = gtk_widget_get_ancestor (GTK_WIDGET (widget), GTK_TYPE_WINDOW);

        if (window != NULL) {
                gtk_window_set_focus (GTK_WINDOW (window), NULL);
        }

        return FALSE;
}

static void
mdm_chooser_widget_shrink (MdmChooserWidget *widget)
{
        g_assert (widget->priv->should_hide_inactive_items == TRUE);

        if (widget->priv->state == MDM_CHOOSER_WIDGET_STATE_GROWING) {
                mdm_scrollable_widget_stop_sliding (MDM_SCROLLABLE_WIDGET (widget->priv->scrollable_widget));
        }

        set_frame_text (widget, widget->priv->active_text);

        gtk_alignment_set (GTK_ALIGNMENT (widget->priv->frame_alignment),
                           0.0, 0.0, 1.0, 0.0);

        clear_selection (widget);

        widget->priv->state = MDM_CHOOSER_WIDGET_STATE_SHRINKING;

        if (gtk_widget_get_visible (GTK_WIDGET (widget))) {
                start_shrink_animation (widget);
        } else {
                skip_resize_animation (widget);
        }
}

static void
activate_from_row (MdmChooserWidget    *widget,
                   GtkTreeRowReference *row)
{
        g_assert (row != NULL);
        g_assert (gtk_tree_row_reference_valid (row));

        if (widget->priv->active_row != NULL) {
                gtk_tree_row_reference_free (widget->priv->active_row);
                widget->priv->active_row = NULL;
        }

        widget->priv->active_row = gtk_tree_row_reference_copy (row);

        if (widget->priv->should_hide_inactive_items) {
                g_debug ("MdmChooserWidget: will emit activated after resize");
                widget->priv->emit_activated_after_resize_animation = TRUE;
                mdm_chooser_widget_shrink (widget);
        } else {
                g_debug ("MdmChooserWidget: emitting activated");
                g_signal_emit (widget, signals[ACTIVATED], 0);
        }
}

static void
deactivate (MdmChooserWidget *widget)
{
        GtkTreePath *path;

        if (widget->priv->active_row == NULL) {
                return;
        }

        path = get_view_path_to_active_row (widget);

        gtk_tree_row_reference_free (widget->priv->active_row);
        widget->priv->active_row = NULL;

        gtk_tree_view_set_cursor (GTK_TREE_VIEW (widget->priv->items_view),
                                  path, NULL, FALSE);
        gtk_tree_path_free (path);

        if (widget->priv->state != MDM_CHOOSER_WIDGET_STATE_GROWN) {
                mdm_chooser_widget_grow (widget);
        } else {
                queue_update_visible_items (widget);
        }

        g_signal_emit (widget, signals[DEACTIVATED], 0);
}

void
mdm_chooser_widget_activate_selected_item (MdmChooserWidget *widget)
{
        GtkTreeRowReference *row;
        gboolean             is_already_active;
        GtkTreePath         *path;
        GtkTreeModel        *model;

        row = NULL;
        model = GTK_TREE_MODEL (widget->priv->list_store);
        is_already_active = FALSE;

        path = NULL;

        get_selected_list_path (widget, &path);
        if (path == NULL) {
                g_debug ("MdmChooserWidget: no row selected");
                return;
        }

        if (widget->priv->active_row != NULL) {
                GtkTreePath *active_path;

                active_path = gtk_tree_row_reference_get_path (widget->priv->active_row);

                if (gtk_tree_path_compare (path, active_path) == 0) {
                        is_already_active = TRUE;
                }
                gtk_tree_path_free (active_path);
        }
        g_assert (path != NULL);
        row = gtk_tree_row_reference_new (model, path);
        gtk_tree_path_free (path);

        if (!is_already_active) {
                activate_from_row (widget, row);
        } else {
                g_debug ("MdmChooserWidget: row is already active");
        }
        gtk_tree_row_reference_free (row);
}

void
mdm_chooser_widget_set_active_item (MdmChooserWidget *widget,
                                    const char       *id)
{
        g_return_if_fail (MDM_IS_CHOOSER_WIDGET (widget));

        g_debug ("MdmChooserWidget: setting active item '%s'",
                 id ? id : "(null)");

        if (id != NULL) {
                activate_from_item_id (widget, id);
        } else {
                deactivate (widget);
        }
}

static void
mdm_chooser_widget_set_property (GObject        *object,
                                 guint           prop_id,
                                 const GValue   *value,
                                 GParamSpec     *pspec)
{
        MdmChooserWidget *self;

        self = MDM_CHOOSER_WIDGET (object);

        switch (prop_id) {

        case PROP_INACTIVE_TEXT:
                g_free (self->priv->inactive_text);
                self->priv->inactive_text = g_value_dup_string (value);

                if (self->priv->active_row == NULL) {
                        set_frame_text (self, self->priv->inactive_text);
                }
                break;

        case PROP_ACTIVE_TEXT:
                g_free (self->priv->active_text);
                self->priv->active_text = g_value_dup_string (value);

                if (self->priv->active_row != NULL) {
                        set_frame_text (self, self->priv->active_text);
                }
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
mdm_chooser_widget_get_property (GObject        *object,
                                 guint           prop_id,
                                 GValue         *value,
                                 GParamSpec     *pspec)
{
        MdmChooserWidget *self;

        self = MDM_CHOOSER_WIDGET (object);

        switch (prop_id) {
        case PROP_INACTIVE_TEXT:
                g_value_set_string (value, self->priv->inactive_text);
                break;

        case PROP_ACTIVE_TEXT:
                g_value_set_string (value, self->priv->active_text);
                break;
        case PROP_LIST_VISIBLE:
                g_value_set_boolean (value, self->priv->list_visible);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
mdm_chooser_widget_dispose (GObject *object)
{
        MdmChooserWidget *widget;

        widget = MDM_CHOOSER_WIDGET (object);

        if (widget->priv->update_items_idle_id > 0) {
                g_source_remove (widget->priv->update_items_idle_id);
                widget->priv->update_items_idle_id = 0;
        }

        if (widget->priv->update_separator_idle_id > 0) {
                g_source_remove (widget->priv->update_separator_idle_id);
                widget->priv->update_separator_idle_id = 0;
        }

        if (widget->priv->update_visibility_idle_id > 0) {
                g_source_remove (widget->priv->update_visibility_idle_id);
                widget->priv->update_visibility_idle_id = 0;
        }

        if (widget->priv->update_cursor_idle_id > 0) {
                g_source_remove (widget->priv->update_cursor_idle_id);
                widget->priv->update_cursor_idle_id = 0;
        }

        if (widget->priv->separator_row != NULL) {
                gtk_tree_row_reference_free (widget->priv->separator_row);
                widget->priv->separator_row = NULL;
        }

        if (widget->priv->active_row != NULL) {
                gtk_tree_row_reference_free (widget->priv->active_row);
                widget->priv->active_row = NULL;
        }

        if (widget->priv->inactive_text != NULL) {
                g_free (widget->priv->inactive_text);
                widget->priv->inactive_text = NULL;
        }

        if (widget->priv->active_text != NULL) {
                g_free (widget->priv->active_text);
                widget->priv->active_text = NULL;
        }

        if (widget->priv->in_use_message != NULL) {
                g_free (widget->priv->in_use_message);
                widget->priv->in_use_message = NULL;
        }

        G_OBJECT_CLASS (mdm_chooser_widget_parent_class)->dispose (object);
}

static void
mdm_chooser_widget_hide (GtkWidget *widget)
{
        skip_resize_animation (MDM_CHOOSER_WIDGET (widget));
        GTK_WIDGET_CLASS (mdm_chooser_widget_parent_class)->hide (widget);
}

static void
mdm_chooser_widget_show (GtkWidget *widget)
{
        skip_resize_animation (MDM_CHOOSER_WIDGET (widget));

        GTK_WIDGET_CLASS (mdm_chooser_widget_parent_class)->show (widget);
}

static void
mdm_chooser_widget_map (GtkWidget *widget)
{
        queue_update_visible_items (MDM_CHOOSER_WIDGET (widget));

        GTK_WIDGET_CLASS (mdm_chooser_widget_parent_class)->map (widget);
}

static void
mdm_chooser_widget_size_allocate (GtkWidget     *widget,
                                  GtkAllocation *allocation)
{
        MdmChooserWidget *chooser_widget;

        GTK_WIDGET_CLASS (mdm_chooser_widget_parent_class)->size_allocate (widget, allocation);

        chooser_widget = MDM_CHOOSER_WIDGET (widget);

}

static gboolean
mdm_chooser_widget_focus (GtkWidget        *widget,
                          GtkDirectionType  direction)
{
        /* Since we only have one focusable child (the tree view),
         * no matter which direction we're going the rules are the
         * same:
         *
         *    1) if it's aready got focus, return FALSE to surrender
         *    that focus.
         *    2) if it doesn't already have focus, then grab it
         */
        if (gtk_container_get_focus_child (GTK_CONTAINER (widget)) != NULL) {
                g_debug ("MdmChooserWidget: not focusing - focus child not null");
                return FALSE;
        }

        _grab_focus (widget);

        return TRUE;
}

static gboolean
mdm_chooser_widget_focus_in_event (GtkWidget     *widget,
                                   GdkEventFocus *focus_event)
{
        /* We don't ever want the chooser widget itself to have focus.
         * Focus should always go to the tree view.
         */
        _grab_focus (widget);

        return FALSE;
}

static void
mdm_chooser_widget_class_init (MdmChooserWidgetClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);
        GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

        object_class->get_property = mdm_chooser_widget_get_property;
        object_class->set_property = mdm_chooser_widget_set_property;
        object_class->dispose = mdm_chooser_widget_dispose;
        object_class->finalize = mdm_chooser_widget_finalize;
        widget_class->size_allocate = mdm_chooser_widget_size_allocate;
        widget_class->hide = mdm_chooser_widget_hide;
        widget_class->show = mdm_chooser_widget_show;
        widget_class->map = mdm_chooser_widget_map;
        widget_class->focus = mdm_chooser_widget_focus;
        widget_class->focus_in_event = mdm_chooser_widget_focus_in_event;

        signals [LOADED] = g_signal_new ("loaded",
                                         G_TYPE_FROM_CLASS (object_class),
                                         G_SIGNAL_RUN_LAST,
                                         G_STRUCT_OFFSET (MdmChooserWidgetClass, loaded),
                                         NULL,
                                         NULL,
                                         g_cclosure_marshal_VOID__VOID,
                                         G_TYPE_NONE,
                                         0);

        signals [ACTIVATED] = g_signal_new ("activated",
                                            G_TYPE_FROM_CLASS (object_class),
                                            G_SIGNAL_RUN_LAST,
                                            G_STRUCT_OFFSET (MdmChooserWidgetClass, activated),
                                            NULL,
                                            NULL,
                                            g_cclosure_marshal_VOID__VOID,
                                            G_TYPE_NONE,
                                            0);

        signals [DEACTIVATED] = g_signal_new ("deactivated",
                                              G_TYPE_FROM_CLASS (object_class),
                                              G_SIGNAL_RUN_LAST,
                                              G_STRUCT_OFFSET (MdmChooserWidgetClass, deactivated),
                                              NULL,
                                              NULL,
                                              g_cclosure_marshal_VOID__VOID,
                                              G_TYPE_NONE,
                                              0);

        g_object_class_install_property (object_class,
                                         PROP_INACTIVE_TEXT,
                                         g_param_spec_string ("inactive-text",
                                                              _("Inactive Text"),
                                                              _("The text to use in the label if the "
                                                                "user hasn't picked an item yet"),
                                                              NULL,
                                                              (G_PARAM_READWRITE |
                                                               G_PARAM_CONSTRUCT)));
        g_object_class_install_property (object_class,
                                         PROP_ACTIVE_TEXT,
                                         g_param_spec_string ("active-text",
                                                              _("Active Text"),
                                                              _("The text to use in the label if the "
                                                                "user has picked an item"),
                                                              NULL,
                                                              (G_PARAM_READWRITE |
                                                               G_PARAM_CONSTRUCT)));

        g_object_class_install_property (object_class,
                                         PROP_LIST_VISIBLE,
                                         g_param_spec_boolean ("list-visible",
                                                              _("List Visible"),
                                                              _("Whether the chooser list is visible"),
                                                              FALSE,
                                                              G_PARAM_READABLE));

        g_type_class_add_private (klass, sizeof (MdmChooserWidgetPrivate));
}

static void
on_row_activated (GtkTreeView          *tree_view,
                  GtkTreePath          *tree_path,
                  GtkTreeViewColumn    *tree_column,
                  MdmChooserWidget     *widget)
{
        char *path_str;

        path_str = gtk_tree_path_to_string (tree_path);
        g_debug ("MdmChooserWidget: row activated '%s'", path_str ? path_str : "(null)");
        g_free (path_str);
        mdm_chooser_widget_activate_selected_item (widget);
}

static gboolean
path_is_separator (MdmChooserWidget *widget,
                   GtkTreeModel     *model,
                   GtkTreePath      *path)
{
        GtkTreePath      *separator_path;
        GtkTreePath      *translated_path;
        gboolean          is_separator;

        separator_path = gtk_tree_row_reference_get_path (widget->priv->separator_row);

        if (separator_path == NULL) {
                return FALSE;
        }

        if (model == GTK_TREE_MODEL (widget->priv->model_sorter)) {
                GtkTreePath *filtered_path;

                filtered_path = gtk_tree_model_sort_convert_path_to_child_path (widget->priv->model_sorter, path);

                translated_path = gtk_tree_model_filter_convert_path_to_child_path (widget->priv->model_filter, filtered_path);
                gtk_tree_path_free (filtered_path);
        } else if (model == GTK_TREE_MODEL (widget->priv->model_filter)) {
                translated_path = gtk_tree_model_filter_convert_path_to_child_path (widget->priv->model_filter, path);
        } else {
                g_assert (model == GTK_TREE_MODEL (widget->priv->list_store));
                translated_path = gtk_tree_path_copy (path);
        }

        if (gtk_tree_path_compare (separator_path, translated_path) == 0) {
                is_separator = TRUE;
        } else {
                is_separator = FALSE;
        }
        gtk_tree_path_free (translated_path);

        return is_separator;
}

static int
mdm_multilingual_collate (const gchar *text_a, const gchar *text_b)
{
        PangoDirection          direction_a;
        PangoDirection          direction_b;
        const gchar            *p_a;
        const gchar            *p_b;
        gchar                  *org_locale = NULL;
        gchar                  *sub_a;
        gchar                  *sub_b;
        gunichar                ch_a;
        gunichar                ch_b;
        GUnicodeScript          script_a;
        GUnicodeScript          script_b;
        gboolean                composed_alpha_a;
        gboolean                composed_alpha_b;
        gboolean                all_alpha_a;
        gboolean                all_alpha_b;
        int                     result;

        direction_a = pango_find_base_dir (text_a, -1);
        direction_b = pango_find_base_dir (text_b, -1);
        if ((direction_a == PANGO_DIRECTION_LTR ||
             direction_a == PANGO_DIRECTION_NEUTRAL) &&
            direction_b == PANGO_DIRECTION_RTL)
                return -1;
        if (direction_a == PANGO_DIRECTION_RTL &&
            (direction_b == PANGO_DIRECTION_LTR ||
             direction_b == PANGO_DIRECTION_NEUTRAL))
                return 1;

        if (!g_get_charset (NULL)) {
                org_locale = g_strdup (setlocale (LC_ALL, NULL));
                if (!setlocale (LC_ALL, "en_US.UTF-8")) {
                        setlocale (LC_ALL, org_locale);
                        g_free (org_locale);
                        org_locale = NULL;
                }
        }

        result = 0;
        all_alpha_a = all_alpha_b = TRUE;
        for (p_a = text_a, p_b = text_b;
             p_a && *p_a && p_b && *p_b;
             p_a = g_utf8_next_char (p_a), p_b = g_utf8_next_char (p_b)) {
                ch_a = g_utf8_get_char (p_a);
                ch_b = g_utf8_get_char (p_b);
                script_a = g_unichar_get_script (ch_a);
                script_b = g_unichar_get_script (ch_b);
                composed_alpha_a = (script_a == G_UNICODE_SCRIPT_LATIN ||
                                    script_a == G_UNICODE_SCRIPT_GREEK ||
                                    script_a == G_UNICODE_SCRIPT_CYRILLIC );
                composed_alpha_b = (script_b == G_UNICODE_SCRIPT_LATIN ||
                                    script_b == G_UNICODE_SCRIPT_GREEK ||
                                    script_b == G_UNICODE_SCRIPT_CYRILLIC );
                all_alpha_a &= composed_alpha_a;
                all_alpha_b &= composed_alpha_b;
                if (all_alpha_a && !composed_alpha_b &&
                    ch_b >= 0x530) {
                    result = -1;
                    break;
                } else if (!composed_alpha_a && all_alpha_b &&
                           ch_a >= 0x530) {
                    result = 1;
                    break;
                } else if (ch_a != ch_b) {
                    sub_a = g_strndup (text_a, g_utf8_next_char (p_a) - text_a);
                    sub_b = g_strndup (text_b, g_utf8_next_char (p_b) - text_b);
                    result = g_utf8_collate (sub_a, sub_b);
                    g_free (sub_a);
                    g_free (sub_b);
                    if (result != 0) {
                            break;
                    }
                }
        }
        if (result != 0) {
                if (org_locale) {
                        setlocale (LC_ALL, org_locale);
                        g_free (org_locale);
                        org_locale = NULL;
                }
                return result;
        }
        if (org_locale) {
                setlocale (LC_ALL, org_locale);
                g_free (org_locale);
                org_locale = NULL;
        }
        return g_utf8_collate (text_a, text_b);
}

static int
compare_item  (GtkTreeModel *model,
               GtkTreeIter  *a,
               GtkTreeIter  *b,
               gpointer      data)
{
        MdmChooserWidget *widget;
        char             *name_a;
        char             *name_b;
        gulong            prio_a;
        gulong            prio_b;
        gboolean          is_separate_a;
        gboolean          is_separate_b;
        int               result;
        int               direction;
        GtkTreeIter      *separator_iter;
        char             *id;
        PangoAttrList    *attrs;

        g_assert (MDM_IS_CHOOSER_WIDGET (data));

        widget = MDM_CHOOSER_WIDGET (data);

        separator_iter = NULL;
        if (widget->priv->separator_row != NULL) {

                GtkTreePath      *path_a;
                GtkTreePath      *path_b;

                path_a = gtk_tree_model_get_path (model, a);
                path_b = gtk_tree_model_get_path (model, b);

                if (path_is_separator (widget, model, path_a)) {
                        separator_iter = a;
                } else if (path_is_separator (widget, model, path_b)) {
                        separator_iter = b;
                }

                gtk_tree_path_free (path_a);
                gtk_tree_path_free (path_b);
        }

        name_a = NULL;
        is_separate_a = FALSE;
        if (separator_iter != a) {
                gtk_tree_model_get (model, a,
                                    CHOOSER_NAME_COLUMN, &name_a,
                                    CHOOSER_PRIORITY_COLUMN, &prio_a,
                                    CHOOSER_ITEM_IS_SEPARATED_COLUMN, &is_separate_a,
                                    -1);
        }

        name_b = NULL;
        is_separate_b = FALSE;
        if (separator_iter != b) {
                gtk_tree_model_get (model, b,
                                    CHOOSER_NAME_COLUMN, &name_b,
                                    CHOOSER_ID_COLUMN, &id,
                                    CHOOSER_PRIORITY_COLUMN, &prio_b,
                                    CHOOSER_ITEM_IS_SEPARATED_COLUMN, &is_separate_b,
                                    -1);
        }

        if (widget->priv->separator_position == MDM_CHOOSER_WIDGET_POSITION_TOP) {
                direction = -1;
        } else {
                direction = 1;
        }

        if (separator_iter == b) {
                result = is_separate_a? 1 : -1;
                result *= direction;
        } else if (separator_iter == a) {
                result = is_separate_b? -1 : 1;
                result *= direction;
        } else if (is_separate_b == is_separate_a) {
                if (prio_a == prio_b) {
                        char *text_a;
                        char *text_b;

                        text_a = NULL;
                        text_b = NULL;
                        pango_parse_markup (name_a, -1, 0, &attrs, &text_a, NULL, NULL);
                        if (text_a == NULL) {
                                g_debug ("MdmChooserWidget: unable to parse markup: '%s'", name_a);
                        }
                        pango_parse_markup (name_b, -1, 0, &attrs, &text_b, NULL, NULL);
                        if (text_b == NULL) {
                                g_debug ("MdmChooserWidget: unable to parse markup: '%s'", name_b);
                        }
                        if (text_a != NULL && text_b != NULL) {
                                result = mdm_multilingual_collate (text_a, text_b);
                        } else {
                                result = mdm_multilingual_collate (name_a, name_b);
                        }
                        g_free (text_a);
                        g_free (text_b);
                } else if (prio_a > prio_b) {
                        result = -1;
                } else {
                        result = 1;
                }
        } else {
                result = is_separate_a - is_separate_b;
                result *= direction;
        }

        g_free (name_a);
        g_free (name_b);

        return result;
}

static void
name_cell_data_func (GtkTreeViewColumn  *tree_column,
                     GtkCellRenderer    *cell,
                     GtkTreeModel       *model,
                     GtkTreeIter        *iter,
                     MdmChooserWidget   *widget)
{
        gboolean is_in_use;
        char    *name;
        char    *markup;

        name = NULL;
        gtk_tree_model_get (model,
                            iter,
                            CHOOSER_ITEM_IS_IN_USE_COLUMN, &is_in_use,
                            CHOOSER_NAME_COLUMN, &name,
                            -1);

        if (is_in_use) {
                markup = g_strdup_printf ("<b>%s</b>\n"
                                          "<i><span size=\"x-small\">%s</span></i>",
                                          name ? name : "(null)", widget->priv->in_use_message);
        } else {
                markup = g_strdup_printf ("%s", name ? name : "(null)");
        }
        g_free (name);

        g_object_set (cell, "markup", markup, NULL);
        g_free (markup);
}

static void
check_cell_data_func (GtkTreeViewColumn    *tree_column,
                      GtkCellRenderer      *cell,
                      GtkTreeModel         *model,
                      GtkTreeIter          *iter,
                      MdmChooserWidget     *widget)
{
        gboolean   is_in_use;
        GdkPixbuf *pixbuf;

        gtk_tree_model_get (model,
                            iter,
                            CHOOSER_ITEM_IS_IN_USE_COLUMN, &is_in_use,
                            -1);

        if (is_in_use) {
                pixbuf = widget->priv->is_in_use_pixbuf;
        } else {
                pixbuf = NULL;
        }

        g_object_set (cell, "pixbuf", pixbuf, NULL);
}

static GdkPixbuf *
get_is_in_use_pixbuf (MdmChooserWidget *widget)
{
        GtkIconTheme *theme;
        GdkPixbuf    *pixbuf;

        theme = gtk_icon_theme_get_default ();
        pixbuf = gtk_icon_theme_load_icon (theme,
                                           "emblem-default",
                                           MDM_CHOOSER_WIDGET_DEFAULT_ICON_SIZE / 3,
                                           0,
                                           NULL);

        return pixbuf;
}

static gboolean
separator_func (GtkTreeModel *model,
                GtkTreeIter  *iter,
                gpointer      data)
{
        MdmChooserWidget *widget;
        GtkTreePath      *path;
        gboolean          is_separator;

        g_assert (MDM_IS_CHOOSER_WIDGET (data));

        widget = MDM_CHOOSER_WIDGET (data);

        g_assert (widget->priv->separator_row != NULL);

        path = gtk_tree_model_get_path (model, iter);

        is_separator = path_is_separator (widget, model, path);

        gtk_tree_path_free (path);

        return is_separator;
}

static void
add_separator (MdmChooserWidget *widget)
{
        GtkTreeIter   iter;
        GtkTreeModel *model;
        GtkTreePath  *path;

        g_assert (widget->priv->separator_row == NULL);

        model = GTK_TREE_MODEL (widget->priv->list_store);

        gtk_list_store_insert_with_values (widget->priv->list_store,
                                           &iter, 0,
                                           CHOOSER_ID_COLUMN, "-", -1);
        path = gtk_tree_model_get_path (model, &iter);
        widget->priv->separator_row = gtk_tree_row_reference_new (model, path);
        gtk_tree_path_free (path);
}

static gboolean
update_column_visibility (MdmChooserWidget *widget)
{
        g_debug ("MdmChooserWidget: updating column visibility");

        if (widget->priv->number_of_rows_with_images > 0) {
                gtk_tree_view_column_set_visible (widget->priv->image_column,
                                                  TRUE);
        } else {
                gtk_tree_view_column_set_visible (widget->priv->image_column,
                                                  FALSE);
        }
        if (widget->priv->number_of_rows_with_status > 0) {
                gtk_tree_view_column_set_visible (widget->priv->status_column,
                                                  TRUE);
        } else {
                gtk_tree_view_column_set_visible (widget->priv->status_column,
                                                  FALSE);
        }

        return FALSE;
}

static void
clear_canceled_visibility_update (MdmChooserWidget *widget)
{
        widget->priv->update_idle_id = 0;
}

static void
queue_column_visibility_update (MdmChooserWidget *widget)
{
        if (widget->priv->update_idle_id == 0) {
                widget->priv->update_idle_id =
                        g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
                                         (GSourceFunc)
                                         update_column_visibility, widget,
                                         (GDestroyNotify)
                                         clear_canceled_visibility_update);
        }
}

static void
on_row_changed (GtkTreeModel     *model,
                GtkTreePath      *path,
                GtkTreeIter      *iter,
                MdmChooserWidget *widget)
{
        queue_column_visibility_update (widget);
}

static void
add_frame (MdmChooserWidget *widget)
{
        widget->priv->frame = gtk_frame_new (NULL);
        gtk_frame_set_shadow_type (GTK_FRAME (widget->priv->frame),
                                   GTK_SHADOW_NONE);
        gtk_widget_show (widget->priv->frame);
        gtk_container_add (GTK_CONTAINER (widget), widget->priv->frame);

        widget->priv->frame_alignment = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
        gtk_widget_show (widget->priv->frame_alignment);
        gtk_container_add (GTK_CONTAINER (widget->priv->frame),
                           widget->priv->frame_alignment);
}

static gboolean
on_button_release (GtkTreeView      *items_view,
                   GdkEventButton   *event,
                   MdmChooserWidget *widget)
{
        GtkTreeModel     *model;
        GtkTreeIter       iter;
        GtkTreeSelection *selection;

        if (!widget->priv->should_hide_inactive_items) {
                return FALSE;
        }

        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget->priv->items_view));
        if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
                GtkTreePath *path;

                path = gtk_tree_model_get_path (model, &iter);
                gtk_tree_view_row_activated (GTK_TREE_VIEW (items_view),
                                             path, NULL);
                gtk_tree_path_free (path);
        }

        return FALSE;
}

static gboolean
search_equal_func (GtkTreeModel     *model,
                   int               column,
                   const char       *key,
                   GtkTreeIter      *iter,
                   MdmChooserWidget *widget)
{
        char       *id;
        char       *name;
        char       *key_folded;
        gboolean    ret;

        if (key == NULL) {
                return FALSE;
        }

        ret = TRUE;
        id = NULL;
        name = NULL;

        key_folded = g_utf8_casefold (key, -1);

        gtk_tree_model_get (model,
                            iter,
                            CHOOSER_ID_COLUMN, &id,
                            CHOOSER_NAME_COLUMN, &name,
                            -1);
        if (name != NULL) {
                char *name_folded;

                name_folded = g_utf8_casefold (name, -1);
                ret = !g_str_has_prefix (name_folded, key_folded);
                g_free (name_folded);

                if (!ret) {
                        goto out;
                }
        }

        if (id != NULL) {
                char *id_folded;


                id_folded = g_utf8_casefold (id, -1);
                ret = !g_str_has_prefix (id_folded, key_folded);
                g_free (id_folded);

                if (!ret) {
                        goto out;
                }
        }
 out:
        g_free (id);
        g_free (name);
        g_free (key_folded);

        return ret;
}

static void
search_position_func (GtkTreeView *tree_view,
                      GtkWidget   *search_dialog,
                      gpointer     user_data)
{
        /* Move it outside the region viewable by
         * the user.
         * FIXME: This is pretty inelegant.
         *
         * It might be nicer to make a MdmOffscreenBin
         * widget that we pack into the chooser widget below
         * the frame but gets redirected offscreen.
         *
         * Then we would add a GtkEntry to the bin and set
         * that entry as the search entry for the tree view
         * instead of using a search position func.
         */
        gtk_window_move (GTK_WINDOW (search_dialog), -24000, -24000);
}

static void
on_selection_changed (GtkTreeSelection *selection,
                      MdmChooserWidget *widget)
{
        GtkTreePath *path;

        get_selected_list_path (widget, &path);
        if (path != NULL) {
                char *path_str;
                path_str = gtk_tree_path_to_string (path);
                g_debug ("MdmChooserWidget: selection change to list path '%s'", path_str);
                g_free (path_str);
        } else {
                g_debug ("MdmChooserWidget: selection cleared");
        }
}

static void
on_adjustment_value_changed (GtkAdjustment    *adjustment,
                             MdmChooserWidget *widget)
{
        queue_update_visible_items (widget);
}

static void
mdm_chooser_widget_init (MdmChooserWidget *widget)
{
        GtkTreeViewColumn *column;
        GtkTreeSelection  *selection;
        GtkCellRenderer   *renderer;
        GtkAdjustment     *adjustment;

        widget->priv = MDM_CHOOSER_WIDGET_GET_PRIVATE (widget);

        /* Even though, we're a container and also don't ever take
         * focus for ourselve, we set CAN_FOCUS so that gtk_widget_grab_focus
         * works on us.  We then override grab_focus requests to
         * be redirected our internal tree view
         */
        gtk_widget_set_can_focus (GTK_WIDGET (widget), TRUE);

        gtk_alignment_set_padding (GTK_ALIGNMENT (widget), 0, 0, 0, 0);

        add_frame (widget);

        widget->priv->scrollable_widget = mdm_scrollable_widget_new ();
        gtk_widget_show (widget->priv->scrollable_widget);
        gtk_container_add (GTK_CONTAINER (widget->priv->frame_alignment),
                           widget->priv->scrollable_widget);

        widget->priv->items_view = gtk_tree_view_new ();
        gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (widget->priv->items_view),
                                           FALSE);
        g_signal_connect (widget->priv->items_view,
                          "row-activated",
                          G_CALLBACK (on_row_activated),
                          widget);

        gtk_tree_view_set_search_equal_func (GTK_TREE_VIEW (widget->priv->items_view),
                                             (GtkTreeViewSearchEqualFunc)search_equal_func,
                                             widget,
                                             NULL);

        gtk_tree_view_set_search_position_func (GTK_TREE_VIEW (widget->priv->items_view),
                                                (GtkTreeViewSearchPositionFunc)search_position_func,
                                                widget,
                                                NULL);

        /* hack to make single-click activate work
         */
        g_signal_connect_after (widget->priv->items_view,
                               "button-release-event",
                               G_CALLBACK (on_button_release),
                               widget);

        gtk_widget_show (widget->priv->items_view);
        gtk_container_add (GTK_CONTAINER (widget->priv->scrollable_widget),
                           widget->priv->items_view);

        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget->priv->items_view));
        gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);

        g_signal_connect (selection, "changed", G_CALLBACK (on_selection_changed), widget);

        g_assert (NUMBER_OF_CHOOSER_COLUMNS == 13);
        widget->priv->list_store = gtk_list_store_new (NUMBER_OF_CHOOSER_COLUMNS,
                                                       GDK_TYPE_PIXBUF,
                                                       G_TYPE_STRING,
                                                       G_TYPE_STRING,
                                                       G_TYPE_ULONG,
                                                       G_TYPE_BOOLEAN,
                                                       G_TYPE_BOOLEAN,
                                                       G_TYPE_BOOLEAN,
                                                       G_TYPE_DOUBLE,
                                                       G_TYPE_DOUBLE,
                                                       G_TYPE_DOUBLE,
                                                       G_TYPE_STRING,
                                                       G_TYPE_POINTER,
                                                       G_TYPE_POINTER);

        widget->priv->model_filter = GTK_TREE_MODEL_FILTER (gtk_tree_model_filter_new (GTK_TREE_MODEL (widget->priv->list_store), NULL));

        gtk_tree_model_filter_set_visible_column (widget->priv->model_filter,
                                                  CHOOSER_ITEM_IS_VISIBLE_COLUMN);
        g_signal_connect (G_OBJECT (widget->priv->model_filter), "row-changed",
                          G_CALLBACK (on_row_changed), widget);

        widget->priv->model_sorter = GTK_TREE_MODEL_SORT (gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL (widget->priv->model_filter)));

        gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (widget->priv->model_sorter),
                                         CHOOSER_ID_COLUMN,
                                         compare_item,
                                         widget, NULL);

        gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (widget->priv->model_sorter),
                                              CHOOSER_ID_COLUMN,
                                              GTK_SORT_ASCENDING);
        gtk_tree_view_set_model (GTK_TREE_VIEW (widget->priv->items_view),
                                 GTK_TREE_MODEL (widget->priv->model_sorter));
        gtk_tree_view_set_row_separator_func (GTK_TREE_VIEW (widget->priv->items_view),
                                              separator_func,
                                              widget, NULL);

        /* IMAGE COLUMN */
        renderer = gtk_cell_renderer_pixbuf_new ();
        column = gtk_tree_view_column_new ();
        gtk_tree_view_column_pack_start (column, renderer, FALSE);
        gtk_tree_view_append_column (GTK_TREE_VIEW (widget->priv->items_view), column);
        widget->priv->image_column = column;

        gtk_tree_view_column_set_attributes (column,
                                             renderer,
                                             "pixbuf", CHOOSER_IMAGE_COLUMN,
                                             NULL);

        g_object_set (renderer,
                      "xalign", 1.0,
                      NULL);

        /* NAME COLUMN */
        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new ();
        gtk_tree_view_column_pack_start (column, renderer, FALSE);
        gtk_tree_view_append_column (GTK_TREE_VIEW (widget->priv->items_view), column);
        gtk_tree_view_column_set_cell_data_func (column,
                                                 renderer,
                                                 (GtkTreeCellDataFunc) name_cell_data_func,
                                                 widget,
                                                 NULL);

        gtk_tree_view_set_tooltip_column (GTK_TREE_VIEW (widget->priv->items_view),
                                          CHOOSER_COMMENT_COLUMN);

        /* STATUS COLUMN */
        renderer = gtk_cell_renderer_pixbuf_new ();
        column = gtk_tree_view_column_new ();
        gtk_tree_view_column_pack_start (column, renderer, FALSE);
        gtk_tree_view_append_column (GTK_TREE_VIEW (widget->priv->items_view), column);
        widget->priv->status_column = column;

        gtk_tree_view_column_set_cell_data_func (column,
                                                 renderer,
                                                 (GtkTreeCellDataFunc) check_cell_data_func,
                                                 widget,
                                                 NULL);
        widget->priv->is_in_use_pixbuf = get_is_in_use_pixbuf (widget);

        renderer = mdm_cell_renderer_timer_new ();
        gtk_tree_view_column_pack_start (column, renderer, FALSE);
        gtk_tree_view_column_add_attribute (column, renderer, "value",
                                            CHOOSER_TIMER_VALUE_COLUMN);

        widget->priv->rows_with_timers =
            g_hash_table_new_full (g_str_hash,
                                   g_str_equal,
                                  (GDestroyNotify) g_free,
                                  (GDestroyNotify)
                                  gtk_tree_row_reference_free);

        add_separator (widget);
        queue_column_visibility_update (widget);

	#if GTK_CHECK_VERSION(3, 0, 0)
		adjustment = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(widget->priv->items_view));
	#else
		adjustment = gtk_tree_view_get_vadjustment(GTK_TREE_VIEW(widget->priv->items_view));
	#endif

        g_signal_connect (adjustment, "value-changed", G_CALLBACK (on_adjustment_value_changed), widget);
}

static void
mdm_chooser_widget_finalize (GObject *object)
{
        MdmChooserWidget *widget;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_CHOOSER_WIDGET (object));

        widget = MDM_CHOOSER_WIDGET (object);

        g_return_if_fail (widget->priv != NULL);

        g_hash_table_destroy (widget->priv->rows_with_timers);
        widget->priv->rows_with_timers = NULL;

        G_OBJECT_CLASS (mdm_chooser_widget_parent_class)->finalize (object);
}

GtkWidget *
mdm_chooser_widget_new (const char *inactive_text,
                        const char *active_text)
{
        GObject *object;

        object = g_object_new (MDM_TYPE_CHOOSER_WIDGET,
                               "inactive-text", inactive_text,
                               "active-text", active_text, NULL);

        return GTK_WIDGET (object);
}

void
mdm_chooser_widget_update_item (MdmChooserWidget *widget,
                                const char       *id,
                                GdkPixbuf        *new_image,
                                const char       *new_name,
                                const char       *new_comment,
                                gulong            new_priority,
                                gboolean          new_in_use,
                                gboolean          new_is_separate)
{
        GtkTreeModel *model;
        GtkTreeIter   iter;
        GdkPixbuf    *image;
        gboolean      is_separate;
        gboolean      in_use;

        g_return_if_fail (MDM_IS_CHOOSER_WIDGET (widget));

        model = GTK_TREE_MODEL (widget->priv->list_store);

        if (!find_item (widget, id, &iter)) {
                g_critical ("Tried to remove non-existing item from chooser");
                return;
        }

        is_separate = FALSE;
        gtk_tree_model_get (model, &iter,
                            CHOOSER_IMAGE_COLUMN, &image,
                            CHOOSER_ITEM_IS_IN_USE_COLUMN, &in_use,
                            CHOOSER_ITEM_IS_SEPARATED_COLUMN, &is_separate,
                            -1);

        if (image != new_image) {
                if (image == NULL && new_image != NULL) {
                        widget->priv->number_of_rows_with_images++;
                } else if (image != NULL && new_image == NULL) {
                        widget->priv->number_of_rows_with_images--;
                }
                queue_column_visibility_update (widget);
        }
        if (image != NULL) {
                g_object_unref (image);
        }

        if (in_use != new_in_use) {
                if (new_in_use) {
                        widget->priv->number_of_rows_with_status++;
                } else {
                        widget->priv->number_of_rows_with_status--;
                }
                queue_column_visibility_update (widget);
        }

        if (is_separate != new_is_separate) {
                if (new_is_separate) {
                        widget->priv->number_of_separated_rows++;
                        widget->priv->number_of_normal_rows--;
                } else {
                        widget->priv->number_of_separated_rows--;
                        widget->priv->number_of_normal_rows++;
                }
                queue_update_separator_visibility (widget);
        }

        gtk_list_store_set (widget->priv->list_store,
                            &iter,
                            CHOOSER_IMAGE_COLUMN, new_image,
                            CHOOSER_NAME_COLUMN, new_name,
                            CHOOSER_COMMENT_COLUMN, new_comment,
                            CHOOSER_PRIORITY_COLUMN, new_priority,
                            CHOOSER_ITEM_IS_IN_USE_COLUMN, new_in_use,
                            CHOOSER_ITEM_IS_SEPARATED_COLUMN, new_is_separate,
                            -1);
}

static void
queue_update_chooser_visibility (MdmChooserWidget *widget)
{
        if (widget->priv->update_visibility_idle_id == 0) {
                widget->priv->update_visibility_idle_id =
                        g_idle_add ((GSourceFunc) update_chooser_visibility, widget);
        }
}

static void
queue_move_cursor_to_top (MdmChooserWidget *widget)
{
        if (widget->priv->update_cursor_idle_id == 0) {
                widget->priv->update_cursor_idle_id =
                        g_idle_add ((GSourceFunc) move_cursor_to_top, widget);
        }
}

void
mdm_chooser_widget_add_item (MdmChooserWidget *widget,
                             const char       *id,
                             GdkPixbuf        *image,
                             const char       *name,
                             const char       *comment,
                             gulong            priority,
                             gboolean          in_use,
                             gboolean          keep_separate,
                             MdmChooserWidgetItemLoadFunc load_func,
                             gpointer                     load_data)
{
        gboolean is_visible;

        g_return_if_fail (MDM_IS_CHOOSER_WIDGET (widget));

        if (keep_separate) {
                widget->priv->number_of_separated_rows++;
        } else {
                widget->priv->number_of_normal_rows++;
        }
        queue_update_separator_visibility (widget);

        if (in_use) {
                widget->priv->number_of_rows_with_status++;
                queue_column_visibility_update (widget);
        }

        if (image != NULL) {
                widget->priv->number_of_rows_with_images++;
                queue_column_visibility_update (widget);
        }

        is_visible = widget->priv->active_row == NULL;

        gtk_list_store_insert_with_values (widget->priv->list_store,
                                           NULL, 0,
                                           CHOOSER_IMAGE_COLUMN, image,
                                           CHOOSER_NAME_COLUMN, name,
                                           CHOOSER_COMMENT_COLUMN, comment,
                                           CHOOSER_PRIORITY_COLUMN, priority,
                                           CHOOSER_ITEM_IS_IN_USE_COLUMN, in_use,
                                           CHOOSER_ITEM_IS_SEPARATED_COLUMN, keep_separate,
                                           CHOOSER_ITEM_IS_VISIBLE_COLUMN, is_visible,
                                           CHOOSER_ID_COLUMN, id,
                                           CHOOSER_LOAD_FUNC_COLUMN, load_func,
                                           CHOOSER_LOAD_DATA_COLUMN, load_data,
                                           -1);

        queue_update_chooser_visibility (widget);
}

void
mdm_chooser_widget_remove_item (MdmChooserWidget *widget,
                                const char       *id)
{
        GtkTreeModel *model;
        GtkTreeIter   iter;
        GdkPixbuf    *image;
        gboolean      is_separate;
        gboolean      is_in_use;

        g_return_if_fail (MDM_IS_CHOOSER_WIDGET (widget));

        model = GTK_TREE_MODEL (widget->priv->list_store);

        if (!find_item (widget, id, &iter)) {
                g_critical ("Tried to remove non-existing item from chooser");
                return;
        }

        is_separate = FALSE;
        gtk_tree_model_get (model, &iter,
                            CHOOSER_IMAGE_COLUMN, &image,
                            CHOOSER_ITEM_IS_IN_USE_COLUMN, &is_in_use,
                            CHOOSER_ITEM_IS_SEPARATED_COLUMN, &is_separate,
                            -1);

        if (image != NULL) {
                widget->priv->number_of_rows_with_images--;
                g_object_unref (image);
        }

        if (is_in_use) {
                widget->priv->number_of_rows_with_status--;
                queue_column_visibility_update (widget);
        }

        if (is_separate) {
                widget->priv->number_of_separated_rows--;
        } else {
                widget->priv->number_of_normal_rows--;
        }
        queue_update_separator_visibility (widget);

        gtk_list_store_remove (widget->priv->list_store, &iter);

        queue_update_chooser_visibility (widget);
}

gboolean
mdm_chooser_widget_lookup_item (MdmChooserWidget *widget,
                                const char       *id,
                                GdkPixbuf       **image,
                                char            **name,
                                char            **comment,
                                gulong           *priority,
                                gboolean         *is_in_use,
                                gboolean         *is_separate)
{
        GtkTreeIter   iter;
        char         *active_item_id;

        g_return_val_if_fail (MDM_IS_CHOOSER_WIDGET (widget), FALSE);
        g_return_val_if_fail (id != NULL, FALSE);

        active_item_id = get_active_item_id (widget, &iter);

        if (active_item_id == NULL || strcmp (active_item_id, id) != 0) {
                g_free (active_item_id);

                if (!find_item (widget, id, &iter)) {
                        return FALSE;
                }
        }
        g_free (active_item_id);

        if (image != NULL) {
                gtk_tree_model_get (GTK_TREE_MODEL (widget->priv->list_store), &iter,
                                    CHOOSER_IMAGE_COLUMN, image, -1);
        }

        if (name != NULL) {
                gtk_tree_model_get (GTK_TREE_MODEL (widget->priv->list_store), &iter,
                                    CHOOSER_NAME_COLUMN, name, -1);
        }

        if (priority != NULL) {
                gtk_tree_model_get (GTK_TREE_MODEL (widget->priv->list_store), &iter,
                                    CHOOSER_PRIORITY_COLUMN, priority, -1);
        }

        if (is_in_use != NULL) {
                gtk_tree_model_get (GTK_TREE_MODEL (widget->priv->list_store), &iter,
                                    CHOOSER_ITEM_IS_IN_USE_COLUMN, is_in_use, -1);
        }

        if (is_separate != NULL) {
                gtk_tree_model_get (GTK_TREE_MODEL (widget->priv->list_store), &iter,
                                    CHOOSER_ITEM_IS_SEPARATED_COLUMN, is_separate, -1);
        }

        return TRUE;
}

void
mdm_chooser_widget_set_item_in_use (MdmChooserWidget *widget,
                                    const char       *id,
                                    gboolean          is_in_use)
{
        GtkTreeIter   iter;
        gboolean      was_in_use;

        g_return_if_fail (MDM_IS_CHOOSER_WIDGET (widget));

        if (!find_item (widget, id, &iter)) {
                return;
        }

        gtk_tree_model_get (GTK_TREE_MODEL (widget->priv->list_store), &iter,
                            CHOOSER_ITEM_IS_IN_USE_COLUMN, &was_in_use,
                            -1);

        if (was_in_use != is_in_use) {

                if (is_in_use) {
                        widget->priv->number_of_rows_with_status++;
                } else {
                        widget->priv->number_of_rows_with_status--;
                }
                queue_column_visibility_update (widget);

                gtk_list_store_set (widget->priv->list_store,
                                    &iter,
                                    CHOOSER_ITEM_IS_IN_USE_COLUMN, is_in_use,
                                    -1);

        }
}

void
mdm_chooser_widget_set_item_priority (MdmChooserWidget *widget,
                                      const char       *id,
                                      gulong            priority)
{
        GtkTreeIter   iter;
        gulong        was_priority;

        g_return_if_fail (MDM_IS_CHOOSER_WIDGET (widget));

        if (!find_item (widget, id, &iter)) {
                return;
        }

        gtk_tree_model_get (GTK_TREE_MODEL (widget->priv->list_store), &iter,
                            CHOOSER_PRIORITY_COLUMN, &was_priority,
                            -1);

        if (was_priority != priority) {

                gtk_list_store_set (widget->priv->list_store,
                                    &iter,
                                    CHOOSER_PRIORITY_COLUMN, priority,
                                    -1);
                gtk_tree_model_filter_refilter (widget->priv->model_filter);
        }
}

static double
get_current_time (void)
{
  const double microseconds_per_second = 1000000.0;
  double       timestamp;
  GTimeVal     now;

  g_get_current_time (&now);

  timestamp = ((microseconds_per_second * now.tv_sec) + now.tv_usec) /
               microseconds_per_second;

  return timestamp;
}

static gboolean
on_timer_timeout (MdmChooserWidget *widget)
{
        GHashTableIter  iter;
        GSList         *list;
        GSList         *tmp;
        gpointer        key;
        gpointer        value;
        double          now;

        list = NULL;
        g_hash_table_iter_init (&iter, widget->priv->rows_with_timers);
        while (g_hash_table_iter_next (&iter, &key, &value)) {
                list = g_slist_prepend (list, value);
        }

        now = get_current_time ();
        for (tmp = list; tmp != NULL; tmp = tmp->next) {
                GtkTreeRowReference *row;

                row = (GtkTreeRowReference *) tmp->data;

                update_timer_from_time (widget, row, now);
        }
        g_slist_free (list);

        return TRUE;
}

static void
start_timer (MdmChooserWidget    *widget,
             GtkTreeRowReference *row,
             double               duration)
{
        GtkTreeModel *model;
        GtkTreePath  *path;
        GtkTreeIter   iter;

        model = GTK_TREE_MODEL (widget->priv->list_store);

        path = gtk_tree_row_reference_get_path (row);
        gtk_tree_model_get_iter (model, &iter, path);
        gtk_tree_path_free (path);

        gtk_list_store_set (widget->priv->list_store, &iter,
                            CHOOSER_TIMER_START_TIME_COLUMN,
                            get_current_time (), -1);
        gtk_list_store_set (widget->priv->list_store, &iter,
                            CHOOSER_TIMER_DURATION_COLUMN,
                            duration, -1);
        gtk_list_store_set (widget->priv->list_store, &iter,
                            CHOOSER_TIMER_VALUE_COLUMN, 0.0, -1);

        widget->priv->number_of_active_timers++;
        if (widget->priv->timer_animation_timeout_id == 0) {
                g_assert (g_hash_table_size (widget->priv->rows_with_timers) == 1);

                widget->priv->timer_animation_timeout_id =
                    g_timeout_add (1000 / 20,
                                   (GSourceFunc) on_timer_timeout,
                                   widget);

        }
}

static void
stop_timer (MdmChooserWidget    *widget,
            GtkTreeRowReference *row)
{
        GtkTreeModel *model;
        GtkTreePath  *path;
        GtkTreeIter   iter;

        model = GTK_TREE_MODEL (widget->priv->list_store);

        path = gtk_tree_row_reference_get_path (row);
        gtk_tree_model_get_iter (model, &iter, path);
        gtk_tree_path_free (path);

        gtk_list_store_set (widget->priv->list_store, &iter,
                            CHOOSER_TIMER_START_TIME_COLUMN,
                            0.0, -1);
        gtk_list_store_set (widget->priv->list_store, &iter,
                            CHOOSER_TIMER_DURATION_COLUMN,
                            0.0, -1);
        gtk_list_store_set (widget->priv->list_store, &iter,
                            CHOOSER_TIMER_VALUE_COLUMN, 0.0, -1);

        widget->priv->number_of_active_timers--;
        if (widget->priv->number_of_active_timers == 0) {
                g_source_remove (widget->priv->timer_animation_timeout_id);
                widget->priv->timer_animation_timeout_id = 0;
        }
}

static void
update_timer_from_time (MdmChooserWidget    *widget,
                        GtkTreeRowReference *row,
                        double               now)
{
        GtkTreeModel *model;
        GtkTreePath  *path;
        GtkTreeIter   iter;
        double        start_time;
        double        duration;
        double        elapsed_ratio;

        model = GTK_TREE_MODEL (widget->priv->list_store);

        path = gtk_tree_row_reference_get_path (row);
        gtk_tree_model_get_iter (model, &iter, path);
        gtk_tree_path_free (path);

        gtk_tree_model_get (model, &iter,
                            CHOOSER_TIMER_START_TIME_COLUMN, &start_time,
                            CHOOSER_TIMER_DURATION_COLUMN, &duration, -1);

        if (duration > G_MINDOUBLE) {
                elapsed_ratio = (now - start_time) / duration;
        } else {
                elapsed_ratio = 0.0;
        }

        gtk_list_store_set (widget->priv->list_store, &iter,
                            CHOOSER_TIMER_VALUE_COLUMN,
                            elapsed_ratio, -1);

        if (elapsed_ratio > .999) {
                char *id;

                stop_timer (widget, row);

                gtk_tree_model_get (model, &iter,
                                    CHOOSER_ID_COLUMN, &id, -1);
                g_hash_table_remove (widget->priv->rows_with_timers,
                                     id);
                g_free (id);

                widget->priv->number_of_rows_with_status--;
                queue_column_visibility_update (widget);
        }
}

void
mdm_chooser_widget_set_item_timer (MdmChooserWidget *widget,
                                   const char       *id,
                                   gulong            timeout)
{
        GtkTreeModel        *model;
        GtkTreeRowReference *row;

        g_return_if_fail (MDM_IS_CHOOSER_WIDGET (widget));

        model = GTK_TREE_MODEL (widget->priv->list_store);

        row = g_hash_table_lookup (widget->priv->rows_with_timers,
                                   id);

        g_assert (row == NULL || gtk_tree_row_reference_valid (row));

        if (row != NULL) {
                stop_timer (widget, row);
        }

        if (timeout == 0) {
                if (row == NULL) {
                        g_warning ("could not find item with ID '%s' to "
                                   "remove timer", id);
                        return;
                }

                g_hash_table_remove (widget->priv->rows_with_timers,
                                     id);
                gtk_tree_model_filter_refilter (widget->priv->model_filter);
                return;
        }

        if (row == NULL) {
                GtkTreeIter  iter;
                GtkTreePath *path;

                if (!find_item (widget, id, &iter)) {
                        g_warning ("could not find item with ID '%s' to "
                                   "add timer", id);
                        return;
                }

                path = gtk_tree_model_get_path (model, &iter);
                row = gtk_tree_row_reference_new (model, path);

                g_hash_table_insert (widget->priv->rows_with_timers,
                                     g_strdup (id), row);

                widget->priv->number_of_rows_with_status++;
                queue_column_visibility_update (widget);
        }

        start_timer (widget, row, timeout / 1000.0);
}

void
mdm_chooser_widget_set_in_use_message (MdmChooserWidget *widget,
                                       const char       *message)
{
        g_return_if_fail (MDM_IS_CHOOSER_WIDGET (widget));

        g_free (widget->priv->in_use_message);
        widget->priv->in_use_message = g_strdup (message);
}

void
mdm_chooser_widget_set_separator_position (MdmChooserWidget         *widget,
                                           MdmChooserWidgetPosition  position)
{
        g_return_if_fail (MDM_IS_CHOOSER_WIDGET (widget));

        if (widget->priv->separator_position != position) {
                widget->priv->separator_position = position;
        }

        gtk_tree_model_filter_refilter (widget->priv->model_filter);
}

void
mdm_chooser_widget_set_hide_inactive_items (MdmChooserWidget  *widget,
                                            gboolean           should_hide)
{
        widget->priv->should_hide_inactive_items = should_hide;

        if (should_hide &&
            (widget->priv->state != MDM_CHOOSER_WIDGET_STATE_SHRUNK
             || widget->priv->state != MDM_CHOOSER_WIDGET_STATE_SHRINKING) &&
            widget->priv->active_row != NULL) {
                mdm_chooser_widget_shrink (widget);
        } else if (!should_hide &&
              (widget->priv->state != MDM_CHOOSER_WIDGET_STATE_GROWN
               || widget->priv->state != MDM_CHOOSER_WIDGET_STATE_GROWING)) {
                mdm_chooser_widget_grow (widget);
        }
}

int
mdm_chooser_widget_get_number_of_items (MdmChooserWidget *widget)
{
        return widget->priv->number_of_normal_rows +
               widget->priv->number_of_separated_rows;
}

void
mdm_chooser_widget_activate_if_one_item (MdmChooserWidget *widget)
{
        activate_if_one_item (widget);
}

void
mdm_chooser_widget_propagate_pending_key_events (MdmChooserWidget *widget)
{
        if (!mdm_scrollable_widget_has_queued_key_events (MDM_SCROLLABLE_WIDGET (widget->priv->scrollable_widget))) {
                return;
        }

        mdm_scrollable_widget_replay_queued_key_events (MDM_SCROLLABLE_WIDGET (widget->priv->scrollable_widget));
}

void
mdm_chooser_widget_loaded (MdmChooserWidget *widget)
{
        g_signal_emit (widget, signals[LOADED], 0);
        update_chooser_visibility (widget);
        queue_move_cursor_to_top (widget);
}
