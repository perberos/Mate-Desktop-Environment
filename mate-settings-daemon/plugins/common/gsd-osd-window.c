/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * On-screen-display (OSD) window for mate-settings-daemon's plugins
 *
 * Copyright (C) 2006-2007 William Jon McCann <mccann@jhu.edu> 
 * Copyright (C) 2009 Novell, Inc
 *
 * Authors:
 *   William Jon McCann <mccann@jhu.edu>
 *   Federico Mena-Quintero <federico@novell.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "gsd-osd-window.h"

#define DIALOG_TIMEOUT 2000     /* dialog timeout in ms */
#define DIALOG_FADE_TIMEOUT 1500 /* timeout before fade starts */
#define FADE_TIMEOUT 10        /* timeout in ms between each frame of the fade */

#define BG_ALPHA 0.75

#define GSD_OSD_WINDOW_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GSD_TYPE_OSD_WINDOW, GsdOsdWindowPrivate))

struct GsdOsdWindowPrivate
{
        guint                    is_composited : 1;
        guint                    hide_timeout_id;
        guint                    fade_timeout_id;
        double                   fade_out_alpha;
};

enum {
        EXPOSE_WHEN_COMPOSITED,
        LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (GsdOsdWindow, gsd_osd_window, GTK_TYPE_WINDOW)

static gboolean
fade_timeout (GsdOsdWindow *window)
{
        if (window->priv->fade_out_alpha <= 0.0) {
                gtk_widget_hide (GTK_WIDGET (window));

                /* Reset it for the next time */
                window->priv->fade_out_alpha = 1.0;
                window->priv->fade_timeout_id = 0;

                return FALSE;
        } else {
                GdkRectangle rect;
                GtkWidget *win = GTK_WIDGET (window);
                GtkAllocation allocation;

                window->priv->fade_out_alpha -= 0.10;

                rect.x = 0;
                rect.y = 0;
                gtk_widget_get_allocation (win, &allocation);
                rect.width = allocation.width;
                rect.height = allocation.height;

                gtk_widget_realize (win);
                gdk_window_invalidate_rect (gtk_widget_get_window (win), &rect, FALSE);
        }

        return TRUE;
}

static gboolean
hide_timeout (GsdOsdWindow *window)
{
        if (window->priv->is_composited) {
                window->priv->hide_timeout_id = 0;
                window->priv->fade_timeout_id = g_timeout_add (FADE_TIMEOUT,
                                                               (GSourceFunc) fade_timeout,
                                                               window);
        } else {
                gtk_widget_hide (GTK_WIDGET (window));
        }

        return FALSE;
}

static void
remove_hide_timeout (GsdOsdWindow *window)
{
        if (window->priv->hide_timeout_id != 0) {
                g_source_remove (window->priv->hide_timeout_id);
                window->priv->hide_timeout_id = 0;
        }

        if (window->priv->fade_timeout_id != 0) {
                g_source_remove (window->priv->fade_timeout_id);
                window->priv->fade_timeout_id = 0;
                window->priv->fade_out_alpha = 1.0;
        }
}

static void
add_hide_timeout (GsdOsdWindow *window)
{
        int timeout;

        if (window->priv->is_composited) {
                timeout = DIALOG_FADE_TIMEOUT;
        } else {
                timeout = DIALOG_TIMEOUT;
        }
        window->priv->hide_timeout_id = g_timeout_add (timeout,
                                                       (GSourceFunc) hide_timeout,
                                                       window);
}

void
gsd_osd_window_draw_rounded_rectangle (cairo_t* cr,
                                       gdouble  aspect,
                                       gdouble  x,
                                       gdouble  y,
                                       gdouble  corner_radius,
                                       gdouble  width,
                                       gdouble  height)
{
        gdouble radius = corner_radius / aspect;

        cairo_move_to (cr, x + radius, y);

        cairo_line_to (cr,
                       x + width - radius,
                       y);
        cairo_arc (cr,
                   x + width - radius,
                   y + radius,
                   radius,
                   -90.0f * G_PI / 180.0f,
                   0.0f * G_PI / 180.0f);
        cairo_line_to (cr,
                       x + width,
                       y + height - radius);
        cairo_arc (cr,
                   x + width - radius,
                   y + height - radius,
                   radius,
                   0.0f * G_PI / 180.0f,
                   90.0f * G_PI / 180.0f);
        cairo_line_to (cr,
                       x + radius,
                       y + height);
        cairo_arc (cr,
                   x + radius,
                   y + height - radius,
                   radius,
                   90.0f * G_PI / 180.0f,
                   180.0f * G_PI / 180.0f);
        cairo_line_to (cr,
                       x,
                       y + radius);
        cairo_arc (cr,
                   x + radius,
                   y + radius,
                   radius,
                   180.0f * G_PI / 180.0f,
                   270.0f * G_PI / 180.0f);
        cairo_close_path (cr);
}

void
gsd_osd_window_color_reverse (const GdkColor *a,
                              GdkColor       *b)
{
        gdouble red;
        gdouble green;
        gdouble blue;
        gdouble h;
        gdouble s;
        gdouble v;

        red = (gdouble) a->red / 65535.0;
        green = (gdouble) a->green / 65535.0;
        blue = (gdouble) a->blue / 65535.0;

        gtk_rgb_to_hsv (red, green, blue, &h, &s, &v);

        v = 0.5 + (0.5 - v);
        if (v > 1.0)
                v = 1.0;
        else if (v < 0.0)
                v = 0.0;

        gtk_hsv_to_rgb (h, s, v, &red, &green, &blue);

        b->red = red * 65535.0;
        b->green = green * 65535.0;
        b->blue = blue * 65535.0;
}

/* This is our expose-event handler when the window is in a compositing manager.
 * We draw everything by hand, using Cairo, so that we can have a nice
 * transparent/rounded look.
 */
static void
expose_when_composited (GtkWidget *widget, GdkEventExpose *event)
{
	GsdOsdWindow    *window;
        cairo_t         *context;
        cairo_t         *cr;
        cairo_surface_t *surface;
        int              width;
        int              height;
        GtkStyle        *style;
        GdkColor         color;
        double           r, g, b;

	window = GSD_OSD_WINDOW (widget);

        context = gdk_cairo_create (gtk_widget_get_window (widget));

        style = gtk_widget_get_style (widget);
        cairo_set_operator (context, CAIRO_OPERATOR_SOURCE);
        gtk_window_get_size (GTK_WINDOW (widget), &width, &height);

        surface = cairo_surface_create_similar (cairo_get_target (context),
                                                CAIRO_CONTENT_COLOR_ALPHA,
                                                width,
                                                height);

        if (cairo_surface_status (surface) != CAIRO_STATUS_SUCCESS) {
                goto done;
        }

        cr = cairo_create (surface);
        if (cairo_status (cr) != CAIRO_STATUS_SUCCESS) {
                goto done;
        }
        cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.0);
        cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
        cairo_paint (cr);

        /* draw a box */
        gsd_osd_window_draw_rounded_rectangle (cr, 1.0, 0.5, 0.5, height / 10, width-1, height-1);
        gsd_osd_window_color_reverse (&style->bg[GTK_STATE_NORMAL], &color);
        r = (float)color.red / 65535.0;
        g = (float)color.green / 65535.0;
        b = (float)color.blue / 65535.0;
        cairo_set_source_rgba (cr, r, g, b, BG_ALPHA);
        cairo_fill_preserve (cr);

        gsd_osd_window_color_reverse (&style->text_aa[GTK_STATE_NORMAL], &color);
        r = (float)color.red / 65535.0;
        g = (float)color.green / 65535.0;
        b = (float)color.blue / 65535.0;
        cairo_set_source_rgba (cr, r, g, b, BG_ALPHA / 2);
        cairo_set_line_width (cr, 1);
        cairo_stroke (cr);

        g_signal_emit (window, signals[EXPOSE_WHEN_COMPOSITED], 0, cr);

        cairo_destroy (cr);

        /* Make sure we have a transparent background */
        cairo_rectangle (context, 0, 0, width, height);
        cairo_set_source_rgba (context, 0.0, 0.0, 0.0, 0.0);
        cairo_fill (context);

        cairo_set_source_surface (context, surface, 0, 0);
        cairo_paint_with_alpha (context, window->priv->fade_out_alpha);

 done:
        if (surface != NULL) {
                cairo_surface_destroy (surface);
        }
        cairo_destroy (context);
}

/* This is our expose-event handler when the window is *not* in a compositing manager.
 * We just draw a rectangular frame by hand.  We do this with hardcoded drawing code,
 * instead of GtkFrame, to avoid changing the window's internal widget hierarchy:  in
 * either case (composited or non-composited), callers can assume that this works
 * identically to a GtkWindow without any intermediate widgetry.
 */
static void
expose_when_not_composited (GtkWidget *widget, GdkEventExpose *event)
{
	GsdOsdWindow *window;
	GtkAllocation allocation;

	window = GSD_OSD_WINDOW (widget);

	gtk_widget_get_allocation (widget, &allocation);

	gtk_paint_shadow (gtk_widget_get_style (widget),
			  gtk_widget_get_window (widget),
			  gtk_widget_get_state (widget),
			  GTK_SHADOW_OUT,
			  &event->area,
			  widget,
			  NULL, /* NULL detail -> themes should use the GsdOsdWindow widget name, probably */
			  0,
			  0,
			  allocation.width,
			  allocation.height);
}

static gboolean
gsd_osd_window_expose_event (GtkWidget          *widget,
			     GdkEventExpose     *event)
{
	GsdOsdWindow *window;
	GtkWidget *child;

	window = GSD_OSD_WINDOW (widget);

	if (window->priv->is_composited)
		expose_when_composited (widget, event);
	else
		expose_when_not_composited (widget, event);

	child = gtk_bin_get_child (GTK_BIN (window));
	if (child)
		gtk_container_propagate_expose (GTK_CONTAINER (window), child, event);

        return FALSE;
}

static void
gsd_osd_window_real_show (GtkWidget *widget)
{
        GsdOsdWindow *window;

        if (GTK_WIDGET_CLASS (gsd_osd_window_parent_class)->show) {
                GTK_WIDGET_CLASS (gsd_osd_window_parent_class)->show (widget);
        }

        window = GSD_OSD_WINDOW (widget);
        remove_hide_timeout (window);
        add_hide_timeout (window);
}

static void
gsd_osd_window_real_hide (GtkWidget *widget)
{
        GsdOsdWindow *window;

        if (GTK_WIDGET_CLASS (gsd_osd_window_parent_class)->hide) {
                GTK_WIDGET_CLASS (gsd_osd_window_parent_class)->hide (widget);
        }

        window = GSD_OSD_WINDOW (widget);
        remove_hide_timeout (window);
}

static void
gsd_osd_window_real_realize (GtkWidget *widget)
{
        GdkColormap *colormap;
        GtkAllocation allocation;
        GdkBitmap *mask;
        cairo_t *cr;

        colormap = gdk_screen_get_rgba_colormap (gtk_widget_get_screen (widget));

        if (colormap != NULL) {
                gtk_widget_set_colormap (widget, colormap);
        }

        if (GTK_WIDGET_CLASS (gsd_osd_window_parent_class)->realize) {
                GTK_WIDGET_CLASS (gsd_osd_window_parent_class)->realize (widget);
        }

        gtk_widget_get_allocation (widget, &allocation);
        mask = gdk_pixmap_new (gtk_widget_get_window (widget),
                               allocation.width,
                               allocation.height,
                               1);
        cr = gdk_cairo_create (mask);

        cairo_set_source_rgba (cr, 1., 1., 1., 0.);
        cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
        cairo_paint (cr);

        /* make the whole window ignore events */
        gdk_window_input_shape_combine_mask (gtk_widget_get_window (widget), mask, 0, 0);
        g_object_unref (mask);
        cairo_destroy (cr);
}

static void
gsd_osd_window_style_set (GtkWidget *widget,
                          GtkStyle  *previous_style)
{
        GtkStyle *style;

        GTK_WIDGET_CLASS (gsd_osd_window_parent_class)->style_set (widget, previous_style);

        /* We set our border width to 12 (per the MATE standard), plus the
         * thickness of the frame that we draw in our expose handler.  This will
         * make our child be 12 pixels away from the frame.
         */

        style = gtk_widget_get_style (widget);
        gtk_container_set_border_width (GTK_CONTAINER (widget), 12 + MAX (style->xthickness, style->ythickness));
}

static void
gsd_osd_window_size_request (GtkWidget      *widget,
                             GtkRequisition *requisition)
{
        GtkStyle *style;

        GTK_WIDGET_CLASS (gsd_osd_window_parent_class)->size_request (widget, requisition);

        /* See the comment in gsd_osd_window_style_set() for why we add the thickness here */

        style = gtk_widget_get_style (widget);

        requisition->width  += style->xthickness;
        requisition->height += style->ythickness;
}

static GObject *
gsd_osd_window_constructor (GType                  type,
                            guint                  n_construct_properties,
                            GObjectConstructParam *construct_params)
{
        GObject *object;

        object = G_OBJECT_CLASS (gsd_osd_window_parent_class)->constructor (type, n_construct_properties, construct_params);

        g_object_set (object,
                      "type", GTK_WINDOW_POPUP,
                      "type-hint", GDK_WINDOW_TYPE_HINT_NOTIFICATION,
                      "skip-taskbar-hint", TRUE,
                      "skip-pager-hint", TRUE,
                      "focus-on-map", FALSE,
                      NULL);

        return object;
}

static void
gsd_osd_window_class_init (GsdOsdWindowClass *klass)
{
        GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
        GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

        gobject_class->constructor = gsd_osd_window_constructor;

        widget_class->show = gsd_osd_window_real_show;
        widget_class->hide = gsd_osd_window_real_hide;
        widget_class->realize = gsd_osd_window_real_realize;
        widget_class->style_set = gsd_osd_window_style_set;
        widget_class->size_request = gsd_osd_window_size_request;
	widget_class->expose_event = gsd_osd_window_expose_event;

        signals[EXPOSE_WHEN_COMPOSITED] = g_signal_new ("expose-when-composited",
                                                        G_TYPE_FROM_CLASS (gobject_class),
                                                        G_SIGNAL_RUN_FIRST,
                                                        G_STRUCT_OFFSET (GsdOsdWindowClass, expose_when_composited),
                                                        NULL, NULL,
                                                        g_cclosure_marshal_VOID__POINTER,
                                                        G_TYPE_NONE, 1,
                                                        G_TYPE_POINTER);

        g_type_class_add_private (klass, sizeof (GsdOsdWindowPrivate));
}

/**
 * gsd_osd_window_is_composited:
 * @window: a #GsdOsdWindow
 *
 * Return value: whether the window was created on a composited screen.
 */
gboolean
gsd_osd_window_is_composited (GsdOsdWindow *window)
{
        return window->priv->is_composited;
}

/**
 * gsd_osd_window_is_valid:
 * @window: a #GsdOsdWindow
 *
 * Return value: TRUE if the @window's idea of being composited matches whether
 * its current screen is actually composited.
 */
gboolean
gsd_osd_window_is_valid (GsdOsdWindow *window)
{
        GdkScreen *screen = gtk_widget_get_screen (GTK_WIDGET (window));
        return gdk_screen_is_composited (screen) == window->priv->is_composited;
}

static void
gsd_osd_window_init (GsdOsdWindow *window)
{
        GdkScreen *screen;

        window->priv = GSD_OSD_WINDOW_GET_PRIVATE (window);

        screen = gtk_widget_get_screen (GTK_WIDGET (window));

        window->priv->is_composited = gdk_screen_is_composited (screen);

        if (window->priv->is_composited) {
                gdouble scalew, scaleh, scale;
                gint size;

                gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
                gtk_widget_set_app_paintable (GTK_WIDGET (window), TRUE);

                /* assume 130x130 on a 640x480 display and scale from there */
                scalew = gdk_screen_get_width (screen) / 640.0;
                scaleh = gdk_screen_get_height (screen) / 480.0;
                scale = MIN (scalew, scaleh);
                size = 130 * MAX (1, scale);

                gtk_window_set_default_size (GTK_WINDOW (window), size, size);

                window->priv->fade_out_alpha = 1.0;
        } else {
		gtk_container_set_border_width (GTK_CONTAINER (window), 12);
        }
}

GtkWidget *
gsd_osd_window_new (void)
{
        return g_object_new (GSD_TYPE_OSD_WINDOW, NULL);
}

/**
 * gsd_osd_window_update_and_hide:
 * @window: a #GsdOsdWindow
 *
 * Queues the @window for immediate drawing, and queues a timer to hide the window.
 */
void
gsd_osd_window_update_and_hide (GsdOsdWindow *window)
{
        remove_hide_timeout (window);
        add_hide_timeout (window);

        if (window->priv->is_composited) {
                gtk_widget_queue_draw (GTK_WIDGET (window));
        }
}
