/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-volume-grid.c
 *
 * Copyright (C) 2009 David Zeuthen
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
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include <math.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>
#include <X11/XKBlib.h>

#include <gdu-gtk/gdu-gtk.h>

#include "gdu-volume-grid.h"

/* ---------------------------------------------------------------------------------------------------- */

#define ELEMENT_MINIMUM_WIDTH 60

typedef enum
{
        GRID_EDGE_NONE    = 0,
        GRID_EDGE_TOP    = (1<<0),
        GRID_EDGE_BOTTOM = (1<<1),
        GRID_EDGE_LEFT   = (1<<2),
        GRID_EDGE_RIGHT  = (1<<3)
} GridEdgeFlags;

typedef struct GridElement GridElement;

struct GridElement
{
        /* these values are set in recompute_grid() */
        gdouble size_ratio;
        GduPresentable *presentable; /* if NULL, it means no media is available */
        GList *embedded_elements;
        GridElement *parent;
        GridElement *prev;
        GridElement *next;

        /* these values are set in recompute_size() */
        guint x;
        guint y;
        guint width;
        guint height;
        GridEdgeFlags edge_flags;

        /* used for the job spinner */
        guint spinner_current;
};

static void
grid_element_free (GridElement *element)
{
        if (element->presentable != NULL)
                g_object_unref (element->presentable);

        g_list_foreach (element->embedded_elements, (GFunc) grid_element_free, NULL);
        g_list_free (element->embedded_elements);

        g_free (element);
}

/* ---------------------------------------------------------------------------------------------------- */

struct GduVolumeGridPrivate
{
        GduPool *pool;
        GduDrive *drive;
        GduDevice *device;

        GList *elements;

        GridElement *selected;
        GridElement *focused;

        guint animation_timeout_id;
};

enum
{
        PROP_0,
        PROP_DRIVE,
};

enum
{
        CHANGED_SIGNAL,
        LAST_SIGNAL,
};

static guint signals[LAST_SIGNAL] = {0};

G_DEFINE_TYPE (GduVolumeGrid, gdu_volume_grid, GTK_TYPE_DRAWING_AREA)

static guint get_depth (GList *elements);

static guint get_num_elements_for_slice (GList *elements);

static void recompute_grid (GduVolumeGrid *grid);

static void recompute_size (GduVolumeGrid *grid,
                            guint          width,
                            guint          height);

static GridElement *find_element_for_presentable (GduVolumeGrid *grid,
                                                  GduPresentable *presentable);

static GridElement *find_element_for_position (GduVolumeGrid *grid,
                                               guint x,
                                               guint y);

static gboolean gdu_volume_grid_expose_event (GtkWidget           *widget,
                                              GdkEventExpose      *event);

static void on_presentable_added        (GduPool        *pool,
                                         GduPresentable *p,
                                         gpointer        user_data);
static void on_presentable_removed      (GduPool        *pool,
                                         GduPresentable *p,
                                         gpointer        user_data);
static void on_presentable_changed      (GduPool        *pool,
                                         GduPresentable *p,
                                         gpointer        user_data);
static void on_presentable_job_changed (GduPool        *pool,
                                        GduPresentable *p,
                                        gpointer        user_data);

static void
gdu_volume_grid_finalize (GObject *object)
{
        GduVolumeGrid *grid = GDU_VOLUME_GRID (object);

        g_list_foreach (grid->priv->elements, (GFunc) grid_element_free, NULL);
        g_list_free (grid->priv->elements);

        g_object_unref (grid->priv->drive);
        if (grid->priv->device != NULL)
                g_object_unref (grid->priv->device);

        g_signal_handlers_disconnect_by_func (grid->priv->pool,
                                              on_presentable_added,
                                              grid);
        g_signal_handlers_disconnect_by_func (grid->priv->pool,
                                              on_presentable_removed,
                                              grid);
        g_signal_handlers_disconnect_by_func (grid->priv->pool,
                                              on_presentable_changed,
                                              grid);
        g_signal_handlers_disconnect_by_func (grid->priv->pool,
                                              on_presentable_job_changed,
                                              grid);
        g_object_unref (grid->priv->pool);

        if (grid->priv->animation_timeout_id > 0) {
                g_source_remove (grid->priv->animation_timeout_id);
                grid->priv->animation_timeout_id = 0;
        }

        if (G_OBJECT_CLASS (gdu_volume_grid_parent_class)->finalize != NULL)
                G_OBJECT_CLASS (gdu_volume_grid_parent_class)->finalize (object);
}

static void
gdu_volume_grid_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
        GduVolumeGrid *grid = GDU_VOLUME_GRID (object);

        switch (property_id) {
        case PROP_DRIVE:
                g_value_set_object (value, grid->priv->drive);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
gdu_volume_grid_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
        GduVolumeGrid *grid = GDU_VOLUME_GRID (object);

        switch (property_id) {
        case PROP_DRIVE:
                grid->priv->drive = GDU_DRIVE (g_value_dup_object (value));
                grid->priv->pool = gdu_presentable_get_pool (GDU_PRESENTABLE (grid->priv->drive));
                grid->priv->device = gdu_presentable_get_device (GDU_PRESENTABLE (grid->priv->drive));
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
gdu_volume_grid_constructed (GObject *object)
{
        GduVolumeGrid *grid = GDU_VOLUME_GRID (object);

        gtk_widget_set_size_request (GTK_WIDGET (grid),
                                     -1,
                                     100);

        g_signal_connect (grid->priv->pool,
                          "presentable-added",
                          G_CALLBACK (on_presentable_added),
                          grid);
        g_signal_connect (grid->priv->pool,
                          "presentable-removed",
                          G_CALLBACK (on_presentable_removed),
                          grid);
        g_signal_connect (grid->priv->pool,
                          "presentable-changed",
                          G_CALLBACK (on_presentable_changed),
                          grid);
        g_signal_connect (grid->priv->pool,
                          "presentable-job-changed",
                          G_CALLBACK (on_presentable_job_changed),
                          grid);

        recompute_grid (grid);

        /* select the first element */
        if (grid->priv->elements != NULL) {
                GridElement *element = grid->priv->elements->data;
                grid->priv->selected = element;
                grid->priv->focused = element;
        }

        if (G_OBJECT_CLASS (gdu_volume_grid_parent_class)->constructed != NULL)
                G_OBJECT_CLASS (gdu_volume_grid_parent_class)->constructed (object);
}

static gboolean
is_ctrl_pressed (void)
{
        gboolean ret;
        XkbStateRec state;
        Bool status;

        ret = FALSE;

        gdk_error_trap_push ();
        status = XkbGetState (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), XkbUseCoreKbd, &state);
        gdk_error_trap_pop ();

        if (status == Success) {
                ret = ((state.mods & ControlMask) != 0);
        }

        return ret;
}

static gboolean
gdu_volume_grid_key_press_event (GtkWidget      *widget,
                                 GdkEventKey    *event)
{
        GduVolumeGrid *grid = GDU_VOLUME_GRID (widget);
        gboolean handled;
        GridElement *target;

        handled = FALSE;

        if (event->type != GDK_KEY_PRESS)
                goto out;

        switch (event->keyval) {
        case GDK_Left:
        case GDK_Right:
        case GDK_Up:
        case GDK_Down:
                target = NULL;

                if (grid->priv->focused == NULL) {
                        g_warning ("TODO: handle nothing being selected/focused");
                } else {
                        GridElement *element;

                        element = grid->priv->focused;
                        if (element != NULL) {
                                if (event->keyval == GDK_Left) {
                                        if (element->prev != NULL) {
                                                target = element->prev;
                                        } else {
                                                if (element->parent && element->parent->prev != NULL)
                                                        target = element->parent->prev;
                                        }
                                } else if (event->keyval == GDK_Right) {
                                        if (element->next != NULL) {
                                                target = element->next;
                                        } else {
                                                if (element->parent && element->parent->next != NULL)
                                                        target = element->parent->next;
                                        }
                                } else if (event->keyval == GDK_Up) {
                                        if (element->parent != NULL) {
                                                target = element->parent;
                                        }
                                } else if (event->keyval == GDK_Down) {
                                        if (element->embedded_elements != NULL) {
                                                target = (GridElement *) element->embedded_elements->data;
                                        }
                                }
                        }
                }

                if (target != NULL) {
                        if (is_ctrl_pressed ()) {
                                grid->priv->focused = target;
                        } else {
                                grid->priv->selected = target;
                                grid->priv->focused = target;
                                g_signal_emit (grid,
                                               signals[CHANGED_SIGNAL],
                                               0);
                        }
                        gtk_widget_queue_draw (GTK_WIDGET (grid));
                }
                handled = TRUE;
                break;

        case GDK_Return:
        case GDK_space:
                if (grid->priv->focused != grid->priv->selected &&
                    grid->priv->focused != NULL) {
                        grid->priv->selected = grid->priv->focused;
                        g_signal_emit (grid,
                                       signals[CHANGED_SIGNAL],
                                       0);
                        gtk_widget_queue_draw (GTK_WIDGET (grid));
                }

                handled = TRUE;
                break;

        default:
                break;
        }

 out:
        return handled;
}

static gboolean
gdu_volume_grid_button_press_event (GtkWidget      *widget,
                                    GdkEventButton *event)
{
        GduVolumeGrid *grid = GDU_VOLUME_GRID (widget);
        gboolean handled;

        handled = FALSE;

        if (event->type != GDK_BUTTON_PRESS)
                goto out;

        if (event->button == 1) {
                GridElement *element;

                element = find_element_for_position (grid, event->x, event->y);
                if (element != NULL) {
                        grid->priv->selected = element;
                        grid->priv->focused = element;

                        g_signal_emit (grid,
                                       signals[CHANGED_SIGNAL],
                                       0);

                        gtk_widget_grab_focus (GTK_WIDGET (grid));
                        gtk_widget_queue_draw (GTK_WIDGET (grid));
                }

                handled = TRUE;
        }

 out:
        return handled;
}

static void
gdu_volume_grid_realize (GtkWidget *widget)
{
        GduVolumeGrid *grid = GDU_VOLUME_GRID (widget);
        GdkWindow *window;
        GdkWindowAttr attributes;
        gint attributes_mask;
        GtkAllocation allocation;

        gtk_widget_set_realized (widget, TRUE);
        gtk_widget_get_allocation (widget, &allocation);

        attributes.x = allocation.x;
        attributes.y = allocation.y;
        attributes.width = allocation.width;
        attributes.height = allocation.height;
        attributes.wclass = GDK_INPUT_OUTPUT;
        attributes.window_type = GDK_WINDOW_CHILD;
        attributes.event_mask = gtk_widget_get_events (widget) |
                GDK_KEY_PRESS_MASK |
                GDK_EXPOSURE_MASK |
                GDK_BUTTON_PRESS_MASK |
                GDK_BUTTON_RELEASE_MASK |
                GDK_ENTER_NOTIFY_MASK |
                GDK_LEAVE_NOTIFY_MASK;
        attributes.visual = gtk_widget_get_visual (widget);
        attributes.colormap = gtk_widget_get_colormap (widget);

        attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

        window = gtk_widget_get_parent_window (widget);
        gtk_widget_set_window (widget, window);
        g_object_ref (window);

        window = gdk_window_new (gtk_widget_get_parent_window (widget),
                                 &attributes,
                                 attributes_mask);
        gtk_widget_set_window (widget, window);
        gdk_window_set_user_data (window, grid);

        gtk_widget_style_attach (widget);
        gtk_style_set_background (gtk_widget_get_style (widget),
                                  window,
                                  GTK_STATE_NORMAL);
}

static guint
get_num_elements_for_slice (GList *elements)
{
        GList *l;
        guint num_elements;

        num_elements = 0;
        for (l = elements; l != NULL; l = l->next) {
                GridElement *element = l->data;
                num_elements += get_num_elements_for_slice (element->embedded_elements);
        }

        if (num_elements > 0)
                return num_elements;
        else
                return 1;
}


static void
gdu_volume_grid_size_request (GtkWidget      *widget,
                              GtkRequisition *requisition)
{
        GduVolumeGrid *grid = GDU_VOLUME_GRID (widget);
        guint num_elements;

        num_elements = get_num_elements_for_slice (grid->priv->elements);
        requisition->width = num_elements * ELEMENT_MINIMUM_WIDTH;
}

static void
gdu_volume_grid_class_init (GduVolumeGridClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

        g_type_class_add_private (klass, sizeof (GduVolumeGridPrivate));

        object_class->get_property = gdu_volume_grid_get_property;
        object_class->set_property = gdu_volume_grid_set_property;
        object_class->constructed  = gdu_volume_grid_constructed;
        object_class->finalize     = gdu_volume_grid_finalize;

        widget_class->realize            = gdu_volume_grid_realize;
        widget_class->key_press_event    = gdu_volume_grid_key_press_event;
        widget_class->button_press_event = gdu_volume_grid_button_press_event;
        widget_class->size_request       = gdu_volume_grid_size_request;
        widget_class->expose_event       = gdu_volume_grid_expose_event;

        g_object_class_install_property (object_class,
                                         PROP_DRIVE,
                                         g_param_spec_object ("drive",
                                                              _("Drive"),
                                                              _("Drive to show volumes for"),
                                                              GDU_TYPE_DRIVE,
                                                              G_PARAM_READABLE |
                                                              G_PARAM_WRITABLE |
                                                              G_PARAM_CONSTRUCT_ONLY));

        signals[CHANGED_SIGNAL] = g_signal_new ("changed",
                                                GDU_TYPE_VOLUME_GRID,
                                                G_SIGNAL_RUN_LAST,
                                                G_STRUCT_OFFSET (GduVolumeGridClass, changed),
                                                NULL,
                                                NULL,
                                                g_cclosure_marshal_VOID__VOID,
                                                G_TYPE_NONE,
                                                0);
}

static void
gdu_volume_grid_init (GduVolumeGrid *grid)
{
        grid->priv = G_TYPE_INSTANCE_GET_PRIVATE (grid, GDU_TYPE_VOLUME_GRID, GduVolumeGridPrivate);

        gtk_widget_set_can_focus (GTK_WIDGET (grid), TRUE);
}

GtkWidget *
gdu_volume_grid_new (GduDrive *drive)
{
        return GTK_WIDGET (g_object_new (GDU_TYPE_VOLUME_GRID,
                                         "drive", drive,
                                         NULL));
}

static gint
presentable_sort_offset (GduPresentable *a, GduPresentable *b)
{
        guint64 oa, ob;

        oa = gdu_presentable_get_offset (a);
        ob = gdu_presentable_get_offset (b);

        if (oa < ob)
                return -1;
        else if (oa > ob)
                return 1;
        else
                return 0;
}

static guint
get_depth (GList *elements)
{
        guint depth;
        GList *l;

        depth = 0;
        if (elements == NULL)
                goto out;

        for (l = elements; l != NULL; l = l->next) {
                GridElement *ee = l->data;
                guint ee_depth;

                ee_depth = get_depth (ee->embedded_elements) + 1;
                if (ee_depth > depth)
                        depth = ee_depth;
        }

 out:
        return depth;
}

static void
recompute_size_for_slice (GduPresentable *presentable,
                          GList          *elements,
                          guint           width,
                          guint           height,
                          guint           total_width,
                          guint           total_height,
                          guint           offset_x,
                          guint           offset_y)
{
        GList *l;
        gint x;
        gint pixels_left;
        guint num_elements;

        /* first steal all the allocated minimum width - then distribute remaining pixels
         * based on the size_ratio and add the allocated minimum width.
         */
        num_elements = get_num_elements_for_slice (elements);
        width -= num_elements * ELEMENT_MINIMUM_WIDTH;
        g_warn_if_fail (width >= 0);

        x = 0;
        pixels_left = width;
        for (l = elements; l != NULL; l = l->next) {
                GridElement *element = l->data;
                gint element_width;
                gboolean is_last;
                guint element_depth;

                is_last  = (l->next == NULL);

                element_depth = get_depth (element->embedded_elements);
                //g_debug ("element_depth = %d (x,y)=(%d,%d) height=%d", element_depth, offset_x, offset_y, height);

                if (is_last) {
                        element_width = pixels_left;
                        pixels_left = 0;
                } else {

                        element_width = element->size_ratio * width;
                        if (element_width > pixels_left)
                                element_width = pixels_left;
                        pixels_left -= element_width;
                }

                num_elements = get_num_elements_for_slice (element->embedded_elements);
                element_width += num_elements * ELEMENT_MINIMUM_WIDTH;

                element->x = x + offset_x;
                element->y = offset_y;
                element->width = element_width;
                if (element_depth > 0) {
                        element->height = height / (element_depth + 1);
                } else {
                        element->height = height;
                }

                if (element->x == 0)
                        element->edge_flags |= GRID_EDGE_LEFT;
                if (element->y == 0)
                        element->edge_flags |= GRID_EDGE_TOP;
                if (element->x + element->width == total_width)
                        element->edge_flags |= GRID_EDGE_RIGHT;
                if (element->y + element->height == total_height)
                        element->edge_flags |= GRID_EDGE_BOTTOM;

                x += element_width;

                recompute_size_for_slice (element->presentable,
                                          element->embedded_elements,
                                          element->width,
                                          height - element->height,
                                          total_width,
                                          total_height,
                                          element->x,
                                          element->height + element->y);
        }
}

static void
recompute_size (GduVolumeGrid *grid,
                guint          width,
                guint          height)
{
        recompute_size_for_slice (GDU_PRESENTABLE (grid->priv->drive),
                                  grid->priv->elements,
                                  width,
                                  height,
                                  width,
                                  height,
                                  0,
                                  0);
}

static void
add_luks_holders (GduVolumeGrid *grid,
                  GridElement   *element)
{
        GduDevice *d;
        GduDevice *dholder;
        GduPresentable *pholder;
        const gchar *holder;
        GridElement *holder_element;

        d = NULL;
        dholder = NULL;
        pholder = NULL;

        d = gdu_presentable_get_device (element->presentable);
        if (d == NULL)
                goto out;

        if (g_strcmp0 (gdu_device_id_get_usage (d), "crypto") != 0)
                goto out;

        holder = gdu_device_luks_get_holder (d);
        if (holder == NULL || g_strcmp0 (holder, "/") == 0)
                goto out;

        dholder = gdu_pool_get_by_object_path (grid->priv->pool, holder);
        if (dholder == NULL)
                goto out;

        pholder = gdu_pool_get_volume_by_device (grid->priv->pool, dholder);
        if (pholder == NULL)
                goto out;

        holder_element = g_new0 (GridElement, 1);
        holder_element->size_ratio = 1.0;
        holder_element->presentable = g_object_ref (pholder);
        holder_element->parent = element;

        g_assert (element->embedded_elements == NULL);

        //g_debug ("added holder %s for %s", gdu_device_get_device_file (dholder), gdu_device_get_device_file (d));

        element->embedded_elements = g_list_append (element->embedded_elements,
                                                    holder_element);

        /* recurse - because we might have more than one level of encryption - for example
         *
         *   sda1 -> LUKS -> LUKS -> filesystem
         */
        add_luks_holders (grid, holder_element);

 out:
        if (d != NULL)
                g_object_unref (d);
        if (dholder != NULL)
                g_object_unref (dholder);
        if (pholder != NULL)
                g_object_unref (pholder);
}


static void
recompute_grid (GduVolumeGrid *grid)
{
        GduPresentable *cur_selected_presentable;
        GduPresentable *cur_focused_presentable;
        GList *enclosed_partitions;
        GList *l;
        guint64 size;
        GridElement *element;
        GridElement *prev_element;

        cur_selected_presentable = NULL;
        cur_focused_presentable = NULL;

        if (grid->priv->selected != NULL && grid->priv->selected->presentable != NULL)
                cur_selected_presentable = g_object_ref (grid->priv->selected->presentable);

        if (grid->priv->focused != NULL && grid->priv->focused->presentable != NULL)
                cur_focused_presentable = g_object_ref (grid->priv->focused->presentable);

        /* first delete old elements */
        g_list_foreach (grid->priv->elements, (GFunc) grid_element_free, NULL);
        g_list_free (grid->priv->elements);
        grid->priv->elements = NULL;

        /* then add new elements */
        size = gdu_presentable_get_size (GDU_PRESENTABLE (grid->priv->drive));
        enclosed_partitions = gdu_pool_get_enclosed_presentables (grid->priv->pool,
                                                                  GDU_PRESENTABLE (grid->priv->drive));
        enclosed_partitions = g_list_sort (enclosed_partitions, (GCompareFunc) presentable_sort_offset);
        prev_element = NULL;
        for (l = enclosed_partitions; l != NULL; l = l->next) {
                GduPresentable *ep = GDU_PRESENTABLE (l->data);
                GduDevice *ed;
                guint64 psize;

                ed = gdu_presentable_get_device (ep);

                psize = gdu_presentable_get_size (ep);

                element = g_new0 (GridElement, 1);
                element->size_ratio = ((gdouble) psize) / size;
                element->presentable = g_object_ref (ep);
                element->prev = prev_element;
                if (prev_element != NULL)
                        prev_element->next = element;
                prev_element = element;

                grid->priv->elements = g_list_append (grid->priv->elements, element);

                if (ed != NULL && gdu_device_is_partition (ed) &&
                    (g_strcmp0 (gdu_device_partition_get_type (ed), "0x05") == 0 ||
                     g_strcmp0 (gdu_device_partition_get_type (ed), "0x0f") == 0 ||
                     g_strcmp0 (gdu_device_partition_get_type (ed), "0x85") == 0)) {
                        GList *enclosed_logical_partitions;
                        GList *ll;
                        GridElement *logical_element;
                        GridElement *logical_prev_element;

                        enclosed_logical_partitions = gdu_pool_get_enclosed_presentables (grid->priv->pool, ep);
                        enclosed_logical_partitions = g_list_sort (enclosed_logical_partitions,
                                                                   (GCompareFunc) presentable_sort_offset);
                        logical_prev_element = NULL;
                        for (ll = enclosed_logical_partitions; ll != NULL; ll = ll->next) {
                                GduPresentable *logical_ep = GDU_PRESENTABLE (ll->data);
                                guint64 lsize;

                                lsize = gdu_presentable_get_size (logical_ep);

                                logical_element = g_new0 (GridElement, 1);
                                logical_element->size_ratio = ((gdouble) lsize) / psize;
                                logical_element->presentable = g_object_ref (logical_ep);
                                logical_element->parent = element;
                                logical_element->prev = logical_prev_element;
                                if (logical_prev_element != NULL)
                                        logical_prev_element->next = logical_element;
                                logical_prev_element = logical_element;

                                element->embedded_elements = g_list_append (element->embedded_elements,
                                                                            logical_element);

                                add_luks_holders (grid, logical_element);

                        }
                        g_list_foreach (enclosed_logical_partitions, (GFunc) g_object_unref, NULL);
                        g_list_free (enclosed_logical_partitions);

                }

                add_luks_holders (grid, element);

                if (ed != NULL)
                        g_object_unref (ed);

        }
        g_list_foreach (enclosed_partitions, (GFunc) g_object_unref, NULL);
        g_list_free (enclosed_partitions);

        /* If we have no elements, then make up an element with a NULL presentable - this is to handle
         * the "No Media Inserted" case.
         */
        if (grid->priv->elements == NULL) {
                element = g_new0 (GridElement, 1);
                element->size_ratio = 1.0;
                grid->priv->elements = g_list_append (grid->priv->elements, element);
        }

        /* reselect focused and selected elements */
        grid->priv->focused = NULL;
        if (cur_focused_presentable != NULL) {
                grid->priv->focused = find_element_for_presentable (grid, cur_focused_presentable);
                g_object_unref (cur_focused_presentable);
        }
        grid->priv->selected = NULL;
        if (cur_selected_presentable != NULL) {
                grid->priv->selected = find_element_for_presentable (grid, cur_selected_presentable);
                g_object_unref (cur_selected_presentable);
        }

        /* ensure something is always focused/selected */
        if ( grid->priv->focused == NULL) {
                grid->priv->focused = grid->priv->elements->data;
        }
        if (grid->priv->selected == NULL) {
                grid->priv->selected = grid->priv->focused;
        }

        /* queue a redraw */
        gtk_widget_queue_draw (GTK_WIDGET (grid));
}

static void
render_spinner (cairo_t   *cr,
                guint      size,
                guint      num_lines,
                guint      current,
                gdouble    x,
                gdouble    y)
{
        guint n;
        gdouble radius;
        gdouble cx;
        gdouble cy;
        gdouble half;

        cx = x + size/2.0;
        cy = y + size/2.0;
        radius = size/2.0;
        half = num_lines / 2;

        current = current % num_lines;

        for (n = 0; n < num_lines; n++) {
                gdouble inset;
                gdouble t;

                inset = 0.7 * radius;

                /* transparency is a function of time and intial value */
                t = (gdouble) ((n + num_lines - current) % num_lines) / num_lines;

                cairo_set_source_rgba (cr, 0, 0, 0, t);
                cairo_set_line_width (cr, 2.0);
                cairo_move_to (cr,
                               cx + (radius - inset) * cos (n * M_PI / half),
                               cy + (radius - inset) * sin (n * M_PI / half));
                cairo_line_to (cr,
                               cx + radius * cos (n * M_PI / half),
                               cy + radius * sin (n * M_PI / half));
                cairo_stroke (cr);
        }
}

static void
render_pixbuf (cairo_t   *cr,
               gdouble    x,
               gdouble    y,
               GdkPixbuf *pixbuf)
{
        gdk_cairo_set_source_pixbuf (cr, pixbuf, x, y);
        cairo_rectangle (cr,
                         x,
                         y,
                         gdk_pixbuf_get_width (pixbuf),
                         gdk_pixbuf_get_height (pixbuf));
        cairo_fill (cr);
}

static void
round_rect (cairo_t *cr,
            gdouble x, gdouble y,
            gdouble w, gdouble h,
            gdouble r,
            GridEdgeFlags edge_flags)
{
        gboolean top_left_round;
        gboolean top_right_round;
        gboolean bottom_right_round;
        gboolean bottom_left_round;

        top_left_round     = ((edge_flags & GRID_EDGE_TOP)    && (edge_flags & GRID_EDGE_LEFT));
        top_right_round    = ((edge_flags & GRID_EDGE_TOP)    && (edge_flags & GRID_EDGE_RIGHT));
        bottom_right_round = ((edge_flags & GRID_EDGE_BOTTOM) && (edge_flags & GRID_EDGE_RIGHT));
        bottom_left_round  = ((edge_flags & GRID_EDGE_BOTTOM) && (edge_flags & GRID_EDGE_LEFT));

        if (top_left_round) {
                cairo_move_to  (cr,
                                x + r, y);
        } else {
                cairo_move_to  (cr,
                                x, y);
        }

        if (top_right_round) {
                cairo_line_to  (cr,
                                x + w - r, y);
                cairo_curve_to (cr,
                                x + w, y,
                                x + w, y,
                                x + w, y + r);
        } else {
                cairo_line_to  (cr,
                                x + w, y);
        }

        if (bottom_right_round) {
                cairo_line_to  (cr,
                                x + w, y + h - r);
                cairo_curve_to (cr,
                                x + w, y + h,
                                x + w, y + h,
                                x + w - r, y + h);
        } else {
                cairo_line_to  (cr,
                                x + w, y + h);
        }

        if (bottom_left_round) {
                cairo_line_to  (cr,
                                x + r, y + h);
                cairo_curve_to (cr,
                                x, y + h,
                                x, y + h,
                                x, y + h - r);
        } else {
                cairo_line_to  (cr,
                                x, y + h);
        }

        if (top_left_round) {
                cairo_line_to  (cr,
                                x, y + r);
                cairo_curve_to (cr,
                                x, y,
                                x, y,
                                x + r, y);
        } else {
                cairo_line_to  (cr,
                                x, y);
        }
}

/* returns true if an animation timeout is needed */
static gboolean
render_element (GduVolumeGrid *grid,
                cairo_t       *cr,
                GridElement   *element,
                gboolean       is_selected,
                gboolean       is_focused,
                gboolean       is_grid_focused)
{
        gboolean need_animation_timeout;
        gdouble fill_red;
        gdouble fill_green;
        gdouble fill_blue;
        gdouble fill_selected_red;
        gdouble fill_selected_green;
        gdouble fill_selected_blue;
        gdouble fill_selected_not_focused_red;
        gdouble fill_selected_not_focused_green;
        gdouble fill_selected_not_focused_blue;
        gdouble focus_rect_red;
        gdouble focus_rect_green;
        gdouble focus_rect_blue;
        gdouble focus_rect_selected_red;
        gdouble focus_rect_selected_green;
        gdouble focus_rect_selected_blue;
        gdouble focus_rect_selected_not_focused_red;
        gdouble focus_rect_selected_not_focused_green;
        gdouble focus_rect_selected_not_focused_blue;
        gdouble stroke_red;
        gdouble stroke_green;
        gdouble stroke_blue;
        gdouble stroke_selected_red;
        gdouble stroke_selected_green;
        gdouble stroke_selected_blue;
        gdouble stroke_selected_not_focused_red;
        gdouble stroke_selected_not_focused_green;
        gdouble stroke_selected_not_focused_blue;
        gdouble text_red;
        gdouble text_green;
        gdouble text_blue;
        gdouble text_selected_red;
        gdouble text_selected_green;
        gdouble text_selected_blue;
        gdouble text_selected_not_focused_red;
        gdouble text_selected_not_focused_green;
        gdouble text_selected_not_focused_blue;
        GtkStyle *style;
        PangoLayout *layout;
        PangoFontDescription *desc;
        gint width, height;

        need_animation_timeout = FALSE;

        style = gtk_widget_get_style (GTK_WIDGET (grid));

        fill_red   = style->base[GTK_STATE_NORMAL].red   / 65535.0;
        fill_green = style->base[GTK_STATE_NORMAL].green / 65535.0;
        fill_blue  = style->base[GTK_STATE_NORMAL].blue  / 65535.0;
        fill_selected_red   = style->base[GTK_STATE_SELECTED].red   / 65535.0;
        fill_selected_green = style->base[GTK_STATE_SELECTED].green / 65535.0;
        fill_selected_blue  = style->base[GTK_STATE_SELECTED].blue  / 65535.0;
        fill_selected_not_focused_red   = style->base[GTK_STATE_ACTIVE].red   / 65535.0;
        fill_selected_not_focused_green = style->base[GTK_STATE_ACTIVE].green / 65535.0;
        fill_selected_not_focused_blue  = style->base[GTK_STATE_ACTIVE].blue  / 65535.0;

        stroke_red   = fill_red   * 0.75;
        stroke_green = fill_green * 0.75;
        stroke_blue  = fill_blue  * 0.75;
        stroke_selected_red   = fill_selected_red   * 0.75;
        stroke_selected_green = fill_selected_green * 0.75;
        stroke_selected_blue  = fill_selected_blue  * 0.75;
        stroke_selected_not_focused_red   = fill_selected_not_focused_red   * 0.75;
        stroke_selected_not_focused_green = fill_selected_not_focused_green * 0.75;
        stroke_selected_not_focused_blue  = fill_selected_not_focused_blue  * 0.75;

        focus_rect_red   = style->text_aa[GTK_STATE_NORMAL].red   / 65535.0;
        focus_rect_green = style->text_aa[GTK_STATE_NORMAL].green / 65535.0;
        focus_rect_blue  = style->text_aa[GTK_STATE_NORMAL].blue  / 65535.0;
        focus_rect_selected_red   = style->text_aa[GTK_STATE_SELECTED].red   / 65535.0;
        focus_rect_selected_green = style->text_aa[GTK_STATE_SELECTED].green / 65535.0;
        focus_rect_selected_blue  = style->text_aa[GTK_STATE_SELECTED].blue  / 65535.0;
        focus_rect_selected_not_focused_red   = style->text_aa[GTK_STATE_ACTIVE].red   / 65535.0;
        focus_rect_selected_not_focused_green = style->text_aa[GTK_STATE_ACTIVE].green / 65535.0;
        focus_rect_selected_not_focused_blue  = style->text_aa[GTK_STATE_ACTIVE].blue  / 65535.0;

        text_red   = style->fg[GTK_STATE_NORMAL].red   / 65535.0;
        text_green = style->fg[GTK_STATE_NORMAL].green / 65535.0;
        text_blue  = style->fg[GTK_STATE_NORMAL].blue  / 65535.0;
        text_selected_red   = style->fg[GTK_STATE_SELECTED].red   / 65535.0;
        text_selected_green = style->fg[GTK_STATE_SELECTED].green / 65535.0;
        text_selected_blue  = style->fg[GTK_STATE_SELECTED].blue  / 65535.0;
        text_selected_not_focused_red   = style->fg[GTK_STATE_ACTIVE].red   / 65535.0;
        text_selected_not_focused_green = style->fg[GTK_STATE_ACTIVE].green / 65535.0;
        text_selected_not_focused_blue  = style->fg[GTK_STATE_ACTIVE].blue  / 65535.0;

#if 0
        g_debug ("rendering element: x=%d w=%d",
                 element->x,
                 element->width);
#endif

        cairo_save (cr);
        cairo_rectangle (cr,
                         element->x + 0.5,
                         element->y + 0.5,
                         element->width,
                         element->height);
        cairo_clip (cr);

        round_rect (cr,
                    element->x + 0.5,
                    element->y + 0.5,
                    element->width,
                    element->height,
                    10,
                    element->edge_flags);

        if (is_selected) {
                cairo_pattern_t *gradient;
                gradient = cairo_pattern_create_radial (element->x + element->width / 2,
                                                        element->y + element->height / 2,
                                                        0.0,
                                                        element->x + element->width / 2,
                                                        element->y + element->height / 2,
                                                        element->width/2.0);
                if (is_grid_focused) {
                        cairo_pattern_add_color_stop_rgb (gradient,
                                                          0.0,
                                                          1.0 * fill_selected_red,
                                                          1.0 * fill_selected_green,
                                                          1.0 * fill_selected_blue);
                        cairo_pattern_add_color_stop_rgb (gradient,
                                                          1.0,
                                                          0.8 * fill_selected_red,
                                                          0.8 * fill_selected_green,
                                                          0.8 * fill_selected_blue);
                } else {
                        cairo_pattern_add_color_stop_rgb (gradient,
                                                          0.0,
                                                          1.0 * fill_selected_not_focused_red,
                                                          1.0 * fill_selected_not_focused_green,
                                                          1.0 * fill_selected_not_focused_blue);
                        cairo_pattern_add_color_stop_rgb (gradient,
                                                          1.0,
                                                          0.8 * fill_selected_not_focused_red,
                                                          0.8 * fill_selected_not_focused_green,
                                                          0.8 * fill_selected_not_focused_blue);
                }
                cairo_set_source (cr, gradient);
                cairo_pattern_destroy (gradient);
        } else {
                cairo_set_source_rgb (cr,
                                      fill_red,
                                      fill_green,
                                      fill_blue);
        }
        cairo_fill_preserve (cr);
        if (is_selected) {
                if (is_grid_focused) {
                        cairo_set_source_rgb (cr,
                                              stroke_selected_red,
                                              stroke_selected_green,
                                              stroke_selected_blue);
                } else {
                        cairo_set_source_rgb (cr,
                                              stroke_selected_not_focused_red,
                                              stroke_selected_not_focused_green,
                                              stroke_selected_not_focused_blue);
                }
        } else {
                cairo_set_source_rgb (cr,
                                      stroke_red,
                                      stroke_green,
                                      stroke_blue);
        }
        cairo_set_dash (cr, NULL, 0, 0.0);
        cairo_set_line_width (cr, 1.0);
        cairo_stroke (cr);

        /* focus indicator */
        if (is_focused && is_grid_focused) {
                gdouble dashes[] = {2.0};
                round_rect (cr,
                            element->x + 0.5 + 3,
                            element->y + 0.5 + 3,
                            element->width - 3 * 2,
                            element->height - 3 * 2,
                            20,
                            element->edge_flags);
                cairo_set_source_rgb (cr, focus_rect_red, focus_rect_green, focus_rect_blue);
                cairo_set_dash (cr, dashes, 1, 0.0);
                cairo_set_line_width (cr, 1.0);
                cairo_stroke (cr);
        }

        if (element->presentable == NULL) { /* no media available */
                const gchar *text;

                if (GDU_IS_LINUX_MD_DRIVE (grid->priv->drive)) {
                        /* Translators: This is shown in the grid for a RAID array that is not running */
                        text = _("RAID Array is not running");
                } else {
                        /* Translators: This is shown in the grid for a drive when no media has been detected */
                        text = _("No Media Detected");
                }

                if (is_selected) {
                        if (is_grid_focused) {
                                cairo_set_source_rgb (cr,
                                                      text_selected_red,
                                                      text_selected_green,
                                                      text_selected_blue);
                        } else {
                                cairo_set_source_rgb (cr,
                                                      text_selected_not_focused_red,
                                                      text_selected_not_focused_green,
                                                      text_selected_not_focused_blue);
                        }
                } else {
                        cairo_set_source_rgb (cr, text_red, text_green, text_blue);
                }
                layout = pango_cairo_create_layout (cr);
                pango_layout_set_text (layout, text, -1);
                desc = pango_font_description_from_string ("Sans 7.0");
                pango_layout_set_font_description (layout, desc);
                pango_font_description_free (desc);
                pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);
                pango_layout_set_width (layout, pango_units_from_double (element->width));
                pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);
                pango_layout_get_size (layout, &width, &height);
                cairo_move_to (cr,
                               ceil(element->x),
                               ceil (element->y + element->height / 2 - pango_units_to_double (height) / 2));
                pango_cairo_show_layout (cr, layout);
                g_object_unref (layout);

        } else { /* render descriptive text + icons for the presentable */
                GString *str;
                GduDevice *d;
                gboolean render_padlock_closed;
                gboolean render_padlock_open;
                gboolean render_job_in_progress;
                GPtrArray *pixbufs_to_render;
                guint icon_offset;
                guint n;
                guint64 size;
                gchar *size_str;

                render_padlock_closed = FALSE;
                render_padlock_open = FALSE;
                render_job_in_progress = FALSE;

                d = gdu_presentable_get_device (element->presentable);

                if (d != NULL && gdu_device_job_in_progress (d))
                        render_job_in_progress = TRUE;

                str = g_string_new (NULL);

                size = gdu_presentable_get_size (element->presentable);
                size_str = gdu_util_get_size_for_display (size,
                                                          FALSE,
                                                          FALSE);

                if (d != NULL && g_strcmp0 (gdu_device_id_get_usage (d), "filesystem") == 0) {
                        gchar *fstype_str;
                        gchar *size_str;

                        fstype_str = gdu_util_get_fstype_for_display (gdu_device_id_get_type (d),
                                                                      gdu_device_id_get_version (d),
                                                                      FALSE);
                        size_str = gdu_util_get_size_for_display (gdu_device_get_size (d),
                                                                  FALSE,
                                                                  FALSE);

                        if (strlen (gdu_device_id_get_label (d)) > 0)
                                g_string_append_printf (str, "%s\n", gdu_device_id_get_label (d));
                        g_string_append_printf (str, "%s %s", size_str, fstype_str);

                        g_free (fstype_str);
                } else if (d != NULL && gdu_device_is_partition (d) &&
                           (g_strcmp0 (gdu_device_partition_get_type (d), "0x05") == 0 ||
                            g_strcmp0 (gdu_device_partition_get_type (d), "0x0f") == 0 ||
                            g_strcmp0 (gdu_device_partition_get_type (d), "0x85") == 0)) {

                        g_string_append_printf (str,
                                                "%s\n%s",
                                                /* Translators: shown in the grid for an extended MS-DOS partition */
                                                _("Extended"),
                                                size_str);

                } else if (d != NULL && g_strcmp0 (gdu_device_id_get_usage (d), "crypto") == 0) {

                        g_string_append_printf (str,
                                                "%s\n%s",
                                                /* Translators: shown in the grid for an encrypted LUKS volume */
                                                _("Encrypted"),
                                                size_str);

                        if (g_strcmp0 (gdu_device_luks_get_holder (d), "/") == 0) {
                                render_padlock_closed = TRUE;
                        } else {
                                render_padlock_open = TRUE;
                        }
                } else if (d != NULL && g_strcmp0 (gdu_device_id_get_usage (d), "raid") == 0 &&
                           gdu_device_is_linux_md_component (d)) {

                        g_string_append_printf (str,
                                                "%s\n%s",
                                                /* Translators: shown in the grid for an RAID Component */
                                                _("RAID Component"),
                                                gdu_device_linux_md_component_get_name (d));

                } else if (!gdu_presentable_is_allocated (element->presentable)) {

                        g_string_append_printf (str,
                                                "%s\n%s",
                                                /* Translators: shown for free/unallocated space */
                                                _("Free"),
                                                size_str);

                } else if (!gdu_presentable_is_recognized (element->presentable)) {

                        g_string_append_printf (str,
                                                "%s\n%s",
                                                /* Translators: shown when we don't know contents of the volume */
                                                _("Unknown"),
                                                size_str);
                } else {
                        gchar *presentable_name;

                        /* FALLBACK */
                        presentable_name = gdu_presentable_get_name (element->presentable);
                        if (size > 0) {
                                g_string_append_printf (str, "%s\n%s", presentable_name, size_str);
                        } else {
                                g_string_append (str, presentable_name);
                        }
                        g_free (presentable_name);
                }
                g_free (size_str);


                if (is_selected) {
                        if (is_grid_focused) {
                                cairo_set_source_rgb (cr,
                                                      text_selected_red,
                                                      text_selected_green,
                                                      text_selected_blue);
                        } else {
                                cairo_set_source_rgb (cr,
                                                      text_selected_not_focused_red,
                                                      text_selected_not_focused_green,
                                                      text_selected_not_focused_blue);
                        }
                } else {
                        cairo_set_source_rgb (cr, text_red, text_green, text_blue);
                }

                layout = pango_cairo_create_layout (cr);
                pango_layout_set_text (layout, str->str, -1);
                desc = pango_font_description_from_string ("Sans 7");
                pango_layout_set_font_description (layout, desc);
                pango_font_description_free (desc);
                pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);
                pango_layout_set_width (layout, pango_units_from_double (element->width));
                pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);
                pango_layout_get_size (layout, &width, &height);
                cairo_move_to (cr,
                               ceil (element->x),
                               ceil (element->y + element->height / 2 - pango_units_to_double (height) / 2));
                pango_cairo_show_layout (cr, layout);
                g_object_unref (layout);

                /* OK, done with the text - now render spinner and icons */
                icon_offset = 0;

                if (render_job_in_progress) {
                        render_spinner (cr,
                                        16,
                                        12,
                                        element->spinner_current,
                                        ceil (element->x + element->width - 16 - icon_offset - 4),
                                        ceil (element->y + element->height - 16 - 4));

                        icon_offset += 16 + 2; /* padding */

                        element->spinner_current += 1;

                        need_animation_timeout = TRUE;
                }

                /* icons */
                pixbufs_to_render = g_ptr_array_new_with_free_func (g_object_unref);
                if (render_padlock_open)
                        g_ptr_array_add (pixbufs_to_render,
                                         gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                                                   "gdu-encrypted-unlock",
                                                                   16, 0, NULL));
                if (render_padlock_closed)
                        g_ptr_array_add (pixbufs_to_render,
                                         gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                                                   "gdu-encrypted-lock",
                                                                   16, 0, NULL));
                for (n = 0; n < pixbufs_to_render->len; n++) {
                        GdkPixbuf *pixbuf = GDK_PIXBUF (pixbufs_to_render->pdata[n]);
                        guint icon_width;
                        guint icon_height;

                        icon_width = gdk_pixbuf_get_width (pixbuf);
                        icon_height = gdk_pixbuf_get_height (pixbuf);

                        render_pixbuf (cr,
                                       ceil (element->x + element->width - icon_width - icon_offset - 4),
                                       ceil (element->y + element->height - icon_height - 4),
                                       pixbuf);

                        icon_offset += icon_width + 2; /* padding */
                }
                g_ptr_array_free (pixbufs_to_render, TRUE);


                if (d != NULL)
                        g_object_unref (d);
        }

        cairo_restore (cr);

        return need_animation_timeout;
}

static gboolean
on_animation_timeout (gpointer data)
{
        GduVolumeGrid *grid = GDU_VOLUME_GRID (data);

        gtk_widget_queue_draw (GTK_WIDGET (grid));

        return TRUE; /* keep timeout around */
}

static gboolean
render_slice (GduVolumeGrid *grid,
              cairo_t       *cr,
              GList         *elements)
{
        GList *l;
        gboolean need_animation_timeout;

        need_animation_timeout = FALSE;
        for (l = elements; l != NULL; l = l->next) {
                GridElement *element = l->data;
                gboolean is_selected;
                gboolean is_focused;
                gboolean is_grid_focused;

                is_selected = FALSE;
                is_focused = FALSE;
                is_grid_focused = gtk_widget_has_focus (GTK_WIDGET (grid));

                if (element == grid->priv->selected)
                        is_selected = TRUE;

                if (element == grid->priv->focused) {
                        if (grid->priv->focused != grid->priv->selected && is_grid_focused)
                                is_focused = TRUE;
                }

                need_animation_timeout |= render_element (grid,
                                                          cr,
                                                          element,
                                                          is_selected,
                                                          is_focused,
                                                          is_grid_focused);

                need_animation_timeout |= render_slice (grid,
                                                        cr,
                                                        element->embedded_elements);
        }

        return need_animation_timeout;
}

static gboolean
gdu_volume_grid_expose_event (GtkWidget           *widget,
                              GdkEventExpose      *event)
{
        GduVolumeGrid *grid = GDU_VOLUME_GRID (widget);
        GtkAllocation allocation;
        cairo_t *cr;
        gdouble width;
        gdouble height;
        gboolean need_animation_timeout;

        gtk_widget_get_allocation (widget, &allocation);
        width = allocation.width;
        height = allocation.height;

        recompute_size (grid,
                        width - 1,
                        height -1);

        cr = gdk_cairo_create (gtk_widget_get_window (widget));
        cairo_rectangle (cr,
                         event->area.x, event->area.y,
                         event->area.width, event->area.height);
        cairo_clip (cr);

        need_animation_timeout = render_slice (grid, cr, grid->priv->elements);

        cairo_destroy (cr);

        if (need_animation_timeout) {
                if (grid->priv->animation_timeout_id == 0) {
                        grid->priv->animation_timeout_id = g_timeout_add (80,
                                                                          on_animation_timeout,
                                                                          grid);
                }
        } else {
                if (grid->priv->animation_timeout_id > 0) {
                        g_source_remove (grid->priv->animation_timeout_id);
                        grid->priv->animation_timeout_id = 0;
                }
        }

        return FALSE;
}

static GridElement *
do_find_element_for_presentable (GList *elements,
                                 GduPresentable *presentable)
{
        GList *l;
        GridElement *ret;

        ret = NULL;

        for (l = elements; l != NULL; l = l->next) {
                GridElement *e = l->data;
                if (e->presentable == presentable) {
                        ret = e;
                        goto out;
                }

                ret = do_find_element_for_presentable (e->embedded_elements, presentable);
                if (ret != NULL)
                        goto out;

        }

 out:
        return ret;
}

static GridElement *
find_element_for_presentable (GduVolumeGrid *grid,
                              GduPresentable *presentable)
{
        return do_find_element_for_presentable (grid->priv->elements, presentable);
}

static GridElement *
do_find_element_for_position (GList *elements,
                              guint  x,
                              guint  y)
{
        GList *l;
        GridElement *ret;

        ret = NULL;

        for (l = elements; l != NULL; l = l->next) {
                GridElement *e = l->data;

                if ((x >= e->x) &&
                    (x  < e->x + e->width) &&
                    (y >= e->y) &&
                    (y  < e->y + e->height)) {
                        ret = e;
                        goto out;
                }

                ret = do_find_element_for_position (e->embedded_elements, x, y);
                if (ret != NULL)
                        goto out;

        }

 out:
        return ret;
}

static GridElement *
find_element_for_position (GduVolumeGrid *grid,
                           guint x,
                           guint y)
{
        return do_find_element_for_position (grid->priv->elements, x, y);
}

GduPresentable *
gdu_volume_grid_get_selected (GduVolumeGrid *grid)
{
        GduPresentable *ret;

        g_return_val_if_fail (GDU_IS_VOLUME_GRID (grid), NULL);

                ret = NULL;
        if (grid->priv->selected != NULL) {
                if (grid->priv->selected->presentable != NULL)
                        ret = g_object_ref (grid->priv->selected->presentable);
        }

        return ret;
}

static void
maybe_recompute (GduVolumeGrid  *grid,
                 GduPresentable *p)
{
        gboolean recompute;

        recompute = FALSE;
        if (p == GDU_PRESENTABLE (grid->priv->drive) ||
            gdu_presentable_encloses (GDU_PRESENTABLE (grid->priv->drive), p) ||
            find_element_for_presentable (grid, p) != NULL) {
                recompute = TRUE;
        }

        if (recompute) {
                recompute_grid (grid);
                g_signal_emit (grid,
                               signals[CHANGED_SIGNAL],
                               0);
        }
}

static void
on_presentable_added (GduPool        *pool,
                      GduPresentable *p,
                      gpointer        user_data)
{
        GduVolumeGrid *grid = GDU_VOLUME_GRID (user_data);
        maybe_recompute (grid, p);
}

static void
on_presentable_removed (GduPool        *pool,
                        GduPresentable *p,
                        gpointer        user_data)
{
        GduVolumeGrid *grid = GDU_VOLUME_GRID (user_data);
        maybe_recompute (grid, p);
}

static void
on_presentable_changed (GduPool        *pool,
                        GduPresentable *p,
                        gpointer        user_data)
{
        GduVolumeGrid *grid = GDU_VOLUME_GRID (user_data);
        maybe_recompute (grid, p);
}

static void
on_presentable_job_changed (GduPool        *pool,
                            GduPresentable *p,
                            gpointer        user_data)
{
        GduVolumeGrid *grid = GDU_VOLUME_GRID (user_data);
        maybe_recompute (grid, p);
}

gboolean
gdu_volume_grid_select (GduVolumeGrid       *grid,
                        GduPresentable      *volume)
{
        GridElement *element;
        gboolean ret;

        g_return_val_if_fail (GDU_IS_VOLUME_GRID (grid), FALSE);

        ret = FALSE;

        element = find_element_for_presentable (grid, volume);
        if (element != NULL) {
                grid->priv->selected = element;
                grid->priv->focused = element;

                g_signal_emit (grid,
                               signals[CHANGED_SIGNAL],
                               0);

                gtk_widget_grab_focus (GTK_WIDGET (grid));
                gtk_widget_queue_draw (GTK_WIDGET (grid));

                ret = TRUE;
        }

        return ret;
}
