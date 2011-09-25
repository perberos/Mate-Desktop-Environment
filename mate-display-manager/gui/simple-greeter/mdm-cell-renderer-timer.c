/*
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
 *  Written by: Ray Strode <rstrode@redhat.com>
 */

#include "config.h"
#include "mdm-cell-renderer-timer.h"
#include <glib/gi18n.h>

#define MDM_CELL_RENDERER_TIMER_GET_PRIVATE(object) (G_TYPE_INSTANCE_GET_PRIVATE ((object), MDM_TYPE_CELL_RENDERER_TIMER, MdmCellRendererTimerPrivate))

struct _MdmCellRendererTimerPrivate
{
        gdouble    value;
};

enum
{
        PROP_0,
        PROP_VALUE,
};

G_DEFINE_TYPE (MdmCellRendererTimer, mdm_cell_renderer_timer, GTK_TYPE_CELL_RENDERER)

static void
mdm_cell_renderer_timer_get_property (GObject *object,
                                      guint param_id,
                                      GValue *value,
                                      GParamSpec *pspec)
{
        MdmCellRendererTimer *renderer;

        renderer = MDM_CELL_RENDERER_TIMER (object);

        switch (param_id) {
                case PROP_VALUE:
                        g_value_set_double (value, renderer->priv->value);
                        break;
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
        }
}

static void
mdm_cell_renderer_timer_set_value (MdmCellRendererTimer *renderer,
                                   gdouble               value)
{
        renderer->priv->value = value;
}

static void
mdm_cell_renderer_timer_set_property (GObject *object,
                                      guint param_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
        MdmCellRendererTimer *renderer;

        renderer = MDM_CELL_RENDERER_TIMER (object);

        switch (param_id) {
                case PROP_VALUE:
                        mdm_cell_renderer_timer_set_value (renderer,
                                                           g_value_get_double (value));
                        break;
                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
    }
}

static void
mdm_cell_renderer_timer_get_size (GtkCellRenderer *cell,
                                  GtkWidget       *widget,
                                  GdkRectangle    *cell_area,
                                  gint            *x_offset,
                                  gint            *y_offset,
                                  gint            *width,
                                  gint            *height)
{

        MdmCellRendererTimer *renderer;

        renderer = MDM_CELL_RENDERER_TIMER (cell);

        if (cell_area != NULL) {
                if (x_offset != NULL) {
                        *x_offset = 0;
                }

                if (y_offset != NULL) {
                        *y_offset = 0;
                }
        }

	gfloat xpad, ypad;
	gtk_cell_renderer_get_alignment (cell, &xpad, &ypad);

        if (width != NULL) {
                *width = xpad * 2 + 24;
        }

        if (height != NULL) {
                *height = ypad * 2 + 24;
        }
}

static double
get_opacity_for_value (double value)
{
        const double start_value = 0.05;
        const double end_value = 0.33;

        if (value < start_value) {
                return 0.0;
        }

        if (value >= end_value) {
                return 1.0;
        }

        return ((value - start_value) / (end_value - start_value));
}

static void
draw_timer (MdmCellRendererTimer *renderer,
            cairo_t              *context,
            GdkColor             *fg,
            GdkColor             *bg,
            int                   width,
            int                   height)
{
        double radius;
        double opacity;

        opacity = get_opacity_for_value (renderer->priv->value);

        if (opacity <= G_MINDOUBLE) {
                return;
        }

        radius = .5 * (MIN (width, height) / 2.0);

        cairo_translate (context, width / 2., height / 2.);

        cairo_set_source_rgba (context,
                               fg->red / 65535.0,
                               fg->green / 65535.0,
                               fg->blue / 65535.0,
                               opacity);

        cairo_move_to (context, 0, 0);
        cairo_arc (context, 0, 0, radius + 1, 0, 2 * G_PI);
        cairo_fill (context);

        cairo_set_source_rgb (context,
                              bg->red / 65535.0,
                              bg->green / 65535.0,
                              bg->blue / 65535.0);
        cairo_move_to (context, 0, 0);
        cairo_arc (context, 0, 0, radius, - G_PI / 2,
                   renderer->priv->value * 2 * G_PI - G_PI / 2);
        cairo_clip (context);
        cairo_paint_with_alpha (context, opacity);
}

static void
mdm_cell_renderer_timer_render (GtkCellRenderer      *cell,
                                GdkWindow            *window,
                                GtkWidget            *widget,
                                GdkRectangle         *background_area,
                                GdkRectangle         *cell_area,
                                GdkRectangle         *expose_area,
                                GtkCellRendererState  renderer_state)
{
        MdmCellRendererTimer *renderer;
        cairo_t              *context;
        GtkStateType          widget_state;

        renderer = MDM_CELL_RENDERER_TIMER (cell);

        if (renderer->priv->value <= G_MINDOUBLE) {
                return;
        }

        context = gdk_cairo_create (GDK_DRAWABLE (window));

        if (expose_area != NULL) {
                gdk_cairo_rectangle (context, expose_area);
                cairo_clip (context);
        }

	gfloat xpad, ypad;
	gtk_cell_renderer_get_alignment (cell, &xpad, &ypad);

        cairo_translate (context,
                         cell_area->x + xpad,
                         cell_area->y + ypad);

        widget_state = GTK_STATE_NORMAL;
        if (renderer_state & GTK_CELL_RENDERER_SELECTED) {
                if (gtk_widget_has_focus (widget)) {
                        widget_state = GTK_STATE_SELECTED;
                } else {
                        widget_state = GTK_STATE_ACTIVE;
                }
        }

        if (renderer_state & GTK_CELL_RENDERER_INSENSITIVE) {
                widget_state = GTK_STATE_INSENSITIVE;
        }

        draw_timer (renderer, context,
                    &gtk_widget_get_style (widget)->text_aa[widget_state],
                    &gtk_widget_get_style (widget)->base[widget_state],
                    cell_area->width, cell_area->height);

        cairo_destroy (context);
}

static void
mdm_cell_renderer_timer_class_init (MdmCellRendererTimerClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS (klass);

        object_class->get_property = mdm_cell_renderer_timer_get_property;
        object_class->set_property = mdm_cell_renderer_timer_set_property;

        cell_class->get_size = mdm_cell_renderer_timer_get_size;
        cell_class->render = mdm_cell_renderer_timer_render;

        g_object_class_install_property (object_class,
                                         PROP_VALUE,
                                         g_param_spec_double ("value",
                                         _("Value"),
                                         _("percentage of time complete"),
                                         0.0, 1.0, 0.0,
                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

        g_type_class_add_private (object_class,
                            sizeof (MdmCellRendererTimerPrivate));
}

static void
mdm_cell_renderer_timer_init (MdmCellRendererTimer *renderer)
{
        renderer->priv = MDM_CELL_RENDERER_TIMER_GET_PRIVATE (renderer);
}

GtkCellRenderer*
mdm_cell_renderer_timer_new (void)
{
        return g_object_new (MDM_TYPE_CELL_RENDERER_TIMER, NULL);
}

