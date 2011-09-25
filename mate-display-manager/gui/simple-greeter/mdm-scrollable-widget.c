/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 * USA.
 *
 * Written by: Ray Strode <rstrode@redhat.com>
 *
 * Parts taken from gtkscrolledwindow.c in the GTK+ toolkit.
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "mdm-scrollable-widget.h"
#include "mdm-timer.h"

#define MDM_SCROLLABLE_WIDGET_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_SCROLLABLE_WIDGET, MdmScrollableWidgetPrivate))

enum
{
        SCROLL_CHILD,
        MOVE_FOCUS_OUT,
        NUMBER_OF_SIGNALS
};

typedef struct MdmScrollableWidgetAnimation MdmScrollableWidgetAnimation;

struct MdmScrollableWidgetPrivate
{
        GtkWidget *scrollbar;

        MdmScrollableWidgetAnimation *animation;
        GtkWidget *invisible_event_sink;
        guint      key_press_signal_id;
        guint      key_release_signal_id;

        GQueue    *key_event_queue;

        guint      child_adjustments_stale : 1;
};

struct MdmScrollableWidgetAnimation
{
        GtkWidget *widget;
        MdmTimer  *timer;
        int        start_height;
        int        desired_height;
        MdmScrollableWidgetSlideStepFunc step_func;
        gpointer   step_func_user_data;
        MdmScrollableWidgetSlideDoneFunc done_func;
        gpointer   done_func_user_data;
};

static void     mdm_scrollable_widget_class_init  (MdmScrollableWidgetClass *klass);
static void     mdm_scrollable_widget_init        (MdmScrollableWidget      *clock_widget);
static void     mdm_scrollable_widget_finalize    (GObject             *object);

static guint signals[NUMBER_OF_SIGNALS] = { 0 };

G_DEFINE_TYPE (MdmScrollableWidget, mdm_scrollable_widget, GTK_TYPE_BIN)

static MdmScrollableWidgetAnimation *
mdm_scrollable_widget_animation_new (GtkWidget *widget,
                                     int        start_height,
                                     int        desired_height,
                                     MdmScrollableWidgetSlideStepFunc step_func,
                                     gpointer   step_func_user_data,
                                     MdmScrollableWidgetSlideDoneFunc done_func,
                                     gpointer   done_func_user_data)
{
        MdmScrollableWidgetAnimation *animation;

        animation = g_slice_new (MdmScrollableWidgetAnimation);

        animation->widget = widget;
        animation->timer = mdm_timer_new ();
        animation->start_height = start_height;
        animation->desired_height = desired_height;
        animation->step_func = step_func;
        animation->step_func_user_data = step_func_user_data;
        animation->done_func = done_func;
        animation->done_func_user_data = done_func_user_data;

        return animation;
}

static void
mdm_scrollable_widget_animation_free (MdmScrollableWidgetAnimation *animation)
{
        g_object_unref (animation->timer);
        animation->timer = NULL;
        g_slice_free (MdmScrollableWidgetAnimation, animation);
}

static void
on_animation_tick (MdmScrollableWidgetAnimation *animation,
                   double                        progress)
{
        int progress_in_pixels;
        int width;
        int height;

        progress_in_pixels = progress * (animation->start_height - animation->desired_height);

        height = animation->start_height - progress_in_pixels;

        gtk_widget_get_size_request (animation->widget, &width, NULL);
        gtk_widget_set_size_request (animation->widget, width, height);

        if (animation->step_func != NULL) {
                MdmTimer *timer;

                height = animation->desired_height;

                height -= gtk_widget_get_style (animation->widget)->ythickness * 2;
                height -= gtk_container_get_border_width (GTK_CONTAINER (animation->widget)) * 2;

                timer = g_object_ref (animation->timer);
                animation->step_func (MDM_SCROLLABLE_WIDGET (animation->widget),
                                      progress,
                                      &height,
                                      animation->step_func_user_data);

                if (mdm_timer_is_started (timer)) {
                        height += gtk_widget_get_style (animation->widget)->ythickness * 2;
                        height += gtk_container_get_border_width (GTK_CONTAINER (animation->widget)) * 2;

                        animation->desired_height = height;
                }
                g_object_unref (timer);
        }
}

static gboolean
on_key_event (MdmScrollableWidget *scrollable_widget,
              GdkEventKey         *key_event)
{
        g_queue_push_tail (scrollable_widget->priv->key_event_queue,
                           gdk_event_copy ((GdkEvent *)key_event));
        return FALSE;
}

static gboolean
mdm_scrollable_redirect_input_to_event_sink (MdmScrollableWidget *scrollable_widget)
{
        GdkGrabStatus status;

        status = gdk_pointer_grab (gtk_widget_get_window (scrollable_widget->priv->invisible_event_sink),
                          FALSE, 0, NULL, NULL, GDK_CURRENT_TIME);
        if (status != GDK_GRAB_SUCCESS) {
                return FALSE;
        }

        status = gdk_keyboard_grab (gtk_widget_get_window (scrollable_widget->priv->invisible_event_sink),
                           FALSE, GDK_CURRENT_TIME);
        if (status != GDK_GRAB_SUCCESS) {
                gdk_pointer_ungrab (GDK_CURRENT_TIME);
                return FALSE;
        }

        scrollable_widget->priv->key_press_signal_id =
            g_signal_connect_swapped (scrollable_widget->priv->invisible_event_sink,
                                      "key-press-event", G_CALLBACK (on_key_event),
                                      scrollable_widget);

        scrollable_widget->priv->key_release_signal_id =
            g_signal_connect_swapped (scrollable_widget->priv->invisible_event_sink,
                                      "key-release-event", G_CALLBACK (on_key_event),
                                      scrollable_widget);

        return TRUE;
}

static void
mdm_scrollable_unredirect_input (MdmScrollableWidget *scrollable_widget)
{
        g_signal_handler_disconnect (scrollable_widget->priv->invisible_event_sink,
                                     scrollable_widget->priv->key_press_signal_id);
        scrollable_widget->priv->key_press_signal_id = 0;

        g_signal_handler_disconnect (scrollable_widget->priv->invisible_event_sink,
                                     scrollable_widget->priv->key_release_signal_id);
        scrollable_widget->priv->key_release_signal_id = 0;
        gdk_keyboard_ungrab (GDK_CURRENT_TIME);
        gdk_pointer_ungrab (GDK_CURRENT_TIME);
}

static void
on_animation_stop (MdmScrollableWidgetAnimation *animation)
{
        MdmScrollableWidget *widget;
        int                  width;

        widget = MDM_SCROLLABLE_WIDGET (animation->widget);

        if (animation->done_func != NULL) {
                animation->done_func (widget, animation->done_func_user_data);
        }

        mdm_scrollable_widget_animation_free (widget->priv->animation);
        widget->priv->animation = NULL;

        gtk_widget_get_size_request (GTK_WIDGET (widget), &width, NULL);
        gtk_widget_set_size_request (GTK_WIDGET (widget), width, -1);
        gtk_widget_queue_resize (GTK_WIDGET (widget));

        mdm_scrollable_unredirect_input (widget);
}

static void
mdm_scrollable_widget_animation_start (MdmScrollableWidgetAnimation *animation)
{
        g_signal_connect_swapped (G_OBJECT (animation->timer), "tick",
                                  G_CALLBACK (on_animation_tick),
                                  animation);
        g_signal_connect_swapped (G_OBJECT (animation->timer), "stop",
                                  G_CALLBACK (on_animation_stop),
                                  animation);
        mdm_timer_start (animation->timer, .10);
}

static void
mdm_scrollable_widget_animation_stop (MdmScrollableWidgetAnimation *animation)
{
        mdm_timer_stop (animation->timer);
}

static gboolean
mdm_scrollable_widget_needs_scrollbar (MdmScrollableWidget *widget)
{
        gboolean needs_scrollbar;

        if (widget->priv->scrollbar == NULL) {
                return FALSE;
        }

        if (widget->priv->animation != NULL) {
                return FALSE;
        }

        if (widget->priv->child_adjustments_stale) {
                return FALSE;
        }

        if (gtk_bin_get_child (GTK_BIN (widget)) != NULL) {
                GtkRequisition child_requisition;
                GtkAllocation widget_allocation;
                int available_height;

                gtk_widget_get_allocation (GTK_WIDGET (widget), &widget_allocation);

                gtk_widget_get_child_requisition (gtk_bin_get_child (GTK_BIN (widget)),
                                                  &child_requisition);
                available_height = widget_allocation.height;
                needs_scrollbar = child_requisition.height > available_height;
        } else {
                needs_scrollbar = FALSE;
        }

        return needs_scrollbar;
}

static void
mdm_scrollable_widget_size_request (GtkWidget      *widget,
                                    GtkRequisition *requisition)
{
        MdmScrollableWidget *scrollable_widget;
        GtkRequisition       child_requisition;
        gboolean             child_adjustments_stale;

        scrollable_widget = MDM_SCROLLABLE_WIDGET (widget);

        requisition->width = 2 * gtk_container_get_border_width (GTK_CONTAINER (widget));
        requisition->height = 2 * gtk_container_get_border_width (GTK_CONTAINER (widget));

        requisition->width += 2 * gtk_widget_get_style (widget)->xthickness;
        requisition->height += 2 * gtk_widget_get_style (widget)->ythickness;

        child_adjustments_stale = FALSE;
        GtkWidget *child = gtk_bin_get_child (GTK_BIN (widget));

         if (child && gtk_widget_get_visible (child)) {

                int old_child_height;
                gtk_widget_get_child_requisition (child,
                                                  &child_requisition);
                old_child_height = child_requisition.height;

                gtk_widget_size_request (child,
                                         &child_requisition);

                requisition->width += child_requisition.width;
                requisition->height += child_requisition.height;

                child_adjustments_stale = old_child_height != child_requisition.height;
        }

        if (mdm_scrollable_widget_needs_scrollbar (scrollable_widget)) {
                GtkRequisition scrollbar_requisition;

                gtk_widget_show (scrollable_widget->priv->scrollbar);

                gtk_widget_size_request (scrollable_widget->priv->scrollbar,
                                         &scrollbar_requisition);

                requisition->height = MAX (requisition->height,
                                           scrollbar_requisition.height);
                requisition->width += scrollbar_requisition.width;
        } else {
                gtk_widget_hide (scrollable_widget->priv->scrollbar);
        }

        scrollable_widget->priv->child_adjustments_stale = child_adjustments_stale;
}

static void
mdm_scrollable_widget_size_allocate (GtkWidget     *widget,
                                     GtkAllocation *allocation)
{
        MdmScrollableWidget *scrollable_widget;
        GtkAllocation        scrollbar_allocation;
        GtkAllocation        child_allocation;
        gboolean             has_child;
        gboolean             needs_scrollbar;
        gboolean             is_flipped;

        scrollable_widget = MDM_SCROLLABLE_WIDGET (widget);

        gtk_widget_set_allocation (widget, allocation);

        GtkWidget *child = gtk_bin_get_child (GTK_BIN (widget));

        has_child = child && gtk_widget_get_visible (child);
        needs_scrollbar = mdm_scrollable_widget_needs_scrollbar (scrollable_widget);
        is_flipped = gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL;

        if (needs_scrollbar) {
                GtkRequisition scrollbar_requisition;

                gtk_widget_get_child_requisition (scrollable_widget->priv->scrollbar,
                                                  &scrollbar_requisition);

                scrollbar_allocation.width = scrollbar_requisition.width;

                if (!is_flipped) {
                        scrollbar_allocation.x = allocation->x + allocation->width;
                        scrollbar_allocation.x -= gtk_container_get_border_width (GTK_CONTAINER (widget));
                        scrollbar_allocation.x -= scrollbar_allocation.width;
                } else {
                        scrollbar_allocation.x = allocation->x;
                        scrollbar_allocation.x += gtk_container_get_border_width (GTK_CONTAINER (widget));
                }

                scrollbar_allocation.height = allocation->height;
                scrollbar_allocation.height -= 2 * gtk_container_get_border_width (GTK_CONTAINER (widget));

                scrollbar_allocation.y = allocation->y;
                scrollbar_allocation.y += gtk_container_get_border_width (GTK_CONTAINER (widget));

                gtk_widget_size_allocate (scrollable_widget->priv->scrollbar,
                                          &scrollbar_allocation);
        }

        if (has_child) {
                child_allocation.width = allocation->width;
                child_allocation.width -= 2 * gtk_container_get_border_width (GTK_CONTAINER (widget));
                child_allocation.width -= 2 * gtk_widget_get_style (widget)->xthickness;

                if (needs_scrollbar) {
                        child_allocation.width -= scrollbar_allocation.width;
                }

                if (!is_flipped) {
                        child_allocation.x = allocation->x;
                        child_allocation.x += gtk_container_get_border_width (GTK_CONTAINER (widget));
                        child_allocation.x += gtk_widget_get_style (widget)->xthickness;
                } else {
                        child_allocation.x = allocation->x + allocation->width;
                        child_allocation.x -= gtk_container_get_border_width (GTK_CONTAINER (widget));
                        child_allocation.x -= child_allocation.width;
                        child_allocation.x -= gtk_widget_get_style (widget)->xthickness;
                }

                child_allocation.height = allocation->height;
                child_allocation.height -= 2 * gtk_container_get_border_width (GTK_CONTAINER (widget));
                child_allocation.height -= 2 * gtk_widget_get_style (widget)->ythickness;

                child_allocation.y = allocation->y;
                child_allocation.y += gtk_container_get_border_width (GTK_CONTAINER (widget));
                child_allocation.y += gtk_widget_get_style (widget)->ythickness;

                gtk_widget_size_allocate (child,
                                          &child_allocation);
                scrollable_widget->priv->child_adjustments_stale = FALSE;
        }
}

static void
mdm_scrollable_widget_add (GtkContainer *container,
                           GtkWidget    *child)
{
        GtkAdjustment *adjustment;

        GTK_CONTAINER_CLASS (mdm_scrollable_widget_parent_class)->add (container, child);

        adjustment = gtk_range_get_adjustment (GTK_RANGE (MDM_SCROLLABLE_WIDGET (container)->priv->scrollbar));

        g_signal_connect_swapped (adjustment, "changed",
                                  G_CALLBACK (gtk_widget_queue_resize),
                                  container);

        gtk_widget_set_scroll_adjustments (child, NULL, adjustment);
}

static void
mdm_scrollable_widget_remove (GtkContainer *container,
                              GtkWidget    *child)
{
        gtk_widget_set_scroll_adjustments (child, NULL, NULL);

        GTK_CONTAINER_CLASS (mdm_scrollable_widget_parent_class)->remove (container, child);
}

static void
mdm_scrollable_widget_forall (GtkContainer *container,
                              gboolean      include_internals,
                              GtkCallback   callback,
                              gpointer      callback_data)
{

        MdmScrollableWidget *scrollable_widget;

        scrollable_widget = MDM_SCROLLABLE_WIDGET (container);

        GTK_CONTAINER_CLASS (mdm_scrollable_widget_parent_class)->forall (container,
                                                                          include_internals,
                                                                          callback,
                                                                          callback_data);

        if (!include_internals) {
                return;
        }

        if (scrollable_widget->priv->scrollbar != NULL) {
                callback (scrollable_widget->priv->scrollbar, callback_data);
        }
}

static void
mdm_scrollable_widget_destroy (GtkObject *object)
{
        MdmScrollableWidget *scrollable_widget;

        scrollable_widget = MDM_SCROLLABLE_WIDGET (object);

        gtk_widget_unparent (scrollable_widget->priv->scrollbar);
        gtk_widget_destroy (scrollable_widget->priv->scrollbar);

        GTK_OBJECT_CLASS (mdm_scrollable_widget_parent_class)->destroy (object);
}

static void
mdm_scrollable_widget_finalize (GObject *object)
{
        MdmScrollableWidget *scrollable_widget;

        scrollable_widget = MDM_SCROLLABLE_WIDGET (object);

        g_queue_free (scrollable_widget->priv->key_event_queue);

        G_OBJECT_CLASS (mdm_scrollable_widget_parent_class)->finalize (object);
}

static gboolean
mdm_scrollable_widget_expose_event (GtkWidget      *widget,
                                    GdkEventExpose *event)
{
        MdmScrollableWidget *scrollable_widget;
        int                  x;
        int                  y;
        int                  width;
        int                  height;
        gboolean             is_flipped;
        GtkAllocation widget_allocation;

        gtk_widget_get_allocation (widget, &widget_allocation);

        scrollable_widget = MDM_SCROLLABLE_WIDGET (widget);

        if (!gtk_widget_is_drawable (widget)) {
                return FALSE;
        }

        is_flipped = gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL;

        x = widget_allocation.x;
        x += 2 * gtk_container_get_border_width (GTK_CONTAINER (widget));

        width = widget_allocation.width;
        width -= 2 * gtk_container_get_border_width (GTK_CONTAINER (widget));

        if (mdm_scrollable_widget_needs_scrollbar (scrollable_widget)) {
                GtkAllocation scrollbar_allocation;
                gtk_widget_get_allocation (scrollable_widget->priv->scrollbar, &scrollbar_allocation);
                width -= scrollbar_allocation.width;

                if (is_flipped) {
                        x += scrollbar_allocation.width;
                }
        }

        y = widget_allocation.y;
        y += 2 * gtk_container_get_border_width (GTK_CONTAINER (widget));

        height = widget_allocation.height;
        height -= 2 * gtk_container_get_border_width (GTK_CONTAINER (widget));

        gtk_paint_shadow (gtk_widget_get_style (widget), gtk_widget_get_window (widget),
                          gtk_widget_get_state (widget), GTK_SHADOW_IN,
                          &event->area, widget, "scrolled_window",
                          x, y, width, height);

        return GTK_WIDGET_CLASS (mdm_scrollable_widget_parent_class)->expose_event (widget, event);
}

static gboolean
mdm_scrollable_widget_scroll_event (GtkWidget      *widget,
                                    GdkEventScroll *event)
{
        if (event->direction != GDK_SCROLL_UP && event->direction != GDK_SCROLL_DOWN) {
                return FALSE;
        }

        if (!gtk_widget_get_visible (GTK_WIDGET (widget))) {
                return FALSE;
        }

        return gtk_widget_event (MDM_SCROLLABLE_WIDGET (widget)->priv->scrollbar,
                                 (GdkEvent *) event);
}

static void
add_scroll_binding (GtkBindingSet  *binding_set,
                    guint           keyval,
                    GdkModifierType mask,
                    GtkScrollType   scroll)
{
        guint keypad_keyval = keyval - GDK_Left + GDK_KP_Left;

        gtk_binding_entry_add_signal (binding_set, keyval, mask,
                                      "scroll-child", 1,
                                      GTK_TYPE_SCROLL_TYPE, scroll);
        gtk_binding_entry_add_signal (binding_set, keypad_keyval, mask,
                                      "scroll-child", 1,
                                      GTK_TYPE_SCROLL_TYPE, scroll);
}

static void
add_tab_bindings (GtkBindingSet    *binding_set,
                  GdkModifierType   modifiers,
                  GtkDirectionType  direction)
{
        gtk_binding_entry_add_signal (binding_set, GDK_Tab, modifiers,
                                      "move-focus-out", 1,
                                      GTK_TYPE_DIRECTION_TYPE, direction);
        gtk_binding_entry_add_signal (binding_set, GDK_KP_Tab, modifiers,
                                      "move-focus-out", 1,
                                      GTK_TYPE_DIRECTION_TYPE, direction);
}

static void
mdm_scrollable_widget_class_install_bindings (MdmScrollableWidgetClass *klass)
{
        GtkBindingSet *binding_set;

        binding_set = gtk_binding_set_by_class (klass);

        add_scroll_binding (binding_set, GDK_Up, GDK_CONTROL_MASK, GTK_SCROLL_STEP_BACKWARD);
        add_scroll_binding (binding_set, GDK_Down, GDK_CONTROL_MASK, GTK_SCROLL_STEP_FORWARD);

        add_scroll_binding (binding_set, GDK_Page_Up, 0, GTK_SCROLL_PAGE_BACKWARD);
        add_scroll_binding (binding_set, GDK_Page_Down, 0, GTK_SCROLL_PAGE_FORWARD);

        add_scroll_binding (binding_set, GDK_Home, 0, GTK_SCROLL_START);
        add_scroll_binding (binding_set, GDK_End, 0, GTK_SCROLL_END);

        add_tab_bindings (binding_set, GDK_CONTROL_MASK, GTK_DIR_TAB_FORWARD);
        add_tab_bindings (binding_set, GDK_CONTROL_MASK | GDK_SHIFT_MASK, GTK_DIR_TAB_BACKWARD);
}

static void
mdm_scrollable_widget_class_init (MdmScrollableWidgetClass *klass)
{
        GObjectClass             *object_class;
        GtkObjectClass           *gtk_object_class;
        GtkWidgetClass           *widget_class;
        GtkContainerClass        *container_class;
        MdmScrollableWidgetClass *scrollable_widget_class;

        object_class = G_OBJECT_CLASS (klass);
        gtk_object_class = GTK_OBJECT_CLASS (klass);
        widget_class = GTK_WIDGET_CLASS (klass);
        container_class = GTK_CONTAINER_CLASS (klass);
        scrollable_widget_class = MDM_SCROLLABLE_WIDGET_CLASS (klass);

        object_class->finalize = mdm_scrollable_widget_finalize;

        gtk_object_class->destroy = mdm_scrollable_widget_destroy;

        widget_class->size_request = mdm_scrollable_widget_size_request;
        widget_class->size_allocate = mdm_scrollable_widget_size_allocate;
        widget_class->expose_event = mdm_scrollable_widget_expose_event;
        widget_class->scroll_event = mdm_scrollable_widget_scroll_event;

        container_class->add = mdm_scrollable_widget_add;
        container_class->remove = mdm_scrollable_widget_remove;
        container_class->forall = mdm_scrollable_widget_forall;

        signals[SCROLL_CHILD] =
          g_signal_new ("scroll-child",
                        G_TYPE_FROM_CLASS (object_class),
                        G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                        G_STRUCT_OFFSET (GtkScrolledWindowClass, scroll_child),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__ENUM,
                        G_TYPE_BOOLEAN, 1,
                        GTK_TYPE_SCROLL_TYPE);
        signals[MOVE_FOCUS_OUT] =
          g_signal_new ("move-focus-out",
                        G_TYPE_FROM_CLASS (object_class),
                        G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                        G_STRUCT_OFFSET (GtkScrolledWindowClass, move_focus_out),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__ENUM,
                        G_TYPE_NONE, 1,
                        GTK_TYPE_DIRECTION_TYPE);
        mdm_scrollable_widget_class_install_bindings (klass);

        g_type_class_add_private (klass, sizeof (MdmScrollableWidgetPrivate));
}

static void
mdm_scrollable_widget_add_scrollbar (MdmScrollableWidget *widget)
{
        gtk_widget_push_composite_child ();
        widget->priv->scrollbar = gtk_vscrollbar_new (NULL);
        gtk_widget_set_composite_name (widget->priv->scrollbar, "scrollbar");
        gtk_widget_pop_composite_child ();
        gtk_widget_set_parent (widget->priv->scrollbar, GTK_WIDGET (widget));
        g_object_ref (widget->priv->scrollbar);
}

static void
mdm_scrollable_widget_add_invisible_event_sink (MdmScrollableWidget *widget)
{
        widget->priv->invisible_event_sink =
            gtk_invisible_new_for_screen (gtk_widget_get_screen (GTK_WIDGET (widget)));
        gtk_widget_show (widget->priv->invisible_event_sink);

        widget->priv->key_event_queue = g_queue_new ();
}

static void
mdm_scrollable_widget_init (MdmScrollableWidget *widget)
{
        widget->priv = MDM_SCROLLABLE_WIDGET_GET_PRIVATE (widget);

        mdm_scrollable_widget_add_scrollbar (widget);
        mdm_scrollable_widget_add_invisible_event_sink (widget);
}

GtkWidget *
mdm_scrollable_widget_new (void)
{
        GObject *object;

        object = g_object_new (MDM_TYPE_SCROLLABLE_WIDGET, NULL);

        return GTK_WIDGET (object);
}

static gboolean
mdm_scrollable_widget_animations_are_disabled (MdmScrollableWidget *scrollable_widget)
{
        GtkSettings *settings;
        gboolean     animations_are_enabled;

        settings = gtk_settings_get_for_screen (gtk_widget_get_screen (GTK_WIDGET (scrollable_widget)));
        g_object_get (settings, "gtk-enable-animations", &animations_are_enabled, NULL);

        return animations_are_enabled == FALSE;
}

void
mdm_scrollable_widget_stop_sliding (MdmScrollableWidget *scrollable_widget)
{
        g_return_if_fail (MDM_IS_SCROLLABLE_WIDGET (scrollable_widget));

        if (scrollable_widget->priv->animation != NULL) {
                mdm_scrollable_widget_animation_stop (scrollable_widget->priv->animation);
        }

        g_assert (scrollable_widget->priv->animation == NULL);
}

void
mdm_scrollable_widget_slide_to_height (MdmScrollableWidget *scrollable_widget,
                                       int                  height,
                                       MdmScrollableWidgetSlideStepFunc step_func,
                                       gpointer             step_user_data,
                                       MdmScrollableWidgetSlideDoneFunc done_func,
                                       gpointer             done_user_data)
{
        GtkWidget *widget;
        gboolean   input_redirected;

        g_return_if_fail (MDM_IS_SCROLLABLE_WIDGET (scrollable_widget));
        widget = GTK_WIDGET (scrollable_widget);

        mdm_scrollable_widget_stop_sliding (scrollable_widget);

        input_redirected = mdm_scrollable_redirect_input_to_event_sink (scrollable_widget);

        if (!input_redirected || mdm_scrollable_widget_animations_are_disabled (scrollable_widget)) {
                if (step_func != NULL) {
                        step_func (scrollable_widget, 0.0, &height, step_user_data);
                }

                if (done_func != NULL) {
                        done_func (scrollable_widget, done_user_data);
                }

                if (input_redirected) {
                        mdm_scrollable_unredirect_input (scrollable_widget);
                }

                return;
        }

        height += gtk_widget_get_style (widget)->ythickness * 2;
        height += gtk_container_get_border_width (GTK_CONTAINER (widget)) * 2;

        GtkAllocation widget_allocation;
        gtk_widget_get_allocation (widget, &widget_allocation);

        scrollable_widget->priv->animation =
            mdm_scrollable_widget_animation_new (widget,
                                                 widget_allocation.height,
                                                 height, step_func, step_user_data,
                                                 done_func, done_user_data);

        mdm_scrollable_widget_animation_start (scrollable_widget->priv->animation);
}

gboolean
mdm_scrollable_widget_has_queued_key_events (MdmScrollableWidget *widget)
{
        g_return_val_if_fail (MDM_IS_SCROLLABLE_WIDGET (widget), FALSE);

        return !g_queue_is_empty (widget->priv->key_event_queue);
}

void
mdm_scrollable_widget_replay_queued_key_events (MdmScrollableWidget *widget)
{
        GtkWidget *toplevel;
        GdkEvent  *event;

        toplevel = gtk_widget_get_toplevel (GTK_WIDGET (widget));

        while ((event = g_queue_pop_head (widget->priv->key_event_queue)) != NULL) {
                gtk_propagate_event (toplevel, event);
        }
}
