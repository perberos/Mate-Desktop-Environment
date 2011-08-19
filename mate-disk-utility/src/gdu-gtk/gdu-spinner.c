/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 * Based on BlingSpinner: Copyright (C) 2007 John Stowers, Neil Jagdish Patel.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 *
 * Code adapted from egg-spinner
 * by Christian Hergert <christian.hergert@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>
#include <math.h>
#include <string.h>

#include "gdu-spinner.h"

typedef struct {
		gfloat red;
		gfloat green;
		gfloat blue;
		gfloat alpha;
} BlingColor;

static int
getdec(char hexchar)
{
   if ((hexchar >= '0') && (hexchar <= '9')) return hexchar - '0';
   if ((hexchar >= 'A') && (hexchar <= 'F')) return hexchar - 'A' + 10;
   if ((hexchar >= 'a') && (hexchar <= 'f')) return hexchar - 'a' + 10;

   return -1; // Wrong character

}

static void
hex2float(const char* HexColor, float* FloatColor)
{
   const char* HexColorPtr = HexColor;

   int i = 0;
   for (i = 0; i < 4; i++)
   {
	 int IntColor = (getdec(HexColorPtr[0]) * 16) +
					 getdec(HexColorPtr[1]);

	 FloatColor[i] = (float) IntColor / 255.0;
	 HexColorPtr += 2;
   }

}

static void
bling_color_parse_string (BlingColor *color, const gchar *str)
{
		gfloat colors[4];
		size_t len;

		len = strlen (str);
		if (len != 8) {
				/* Always return a valid color */
				color->red = 0.0;
				color->green = 0.0;
				color->blue = 0.0;
				color->alpha = 1.0;
		} else {
				hex2float (str, colors);
				color->red = colors[0];
				color->green = colors[1];
				color->blue = colors[2];
				color->alpha = colors[3];
		}
}


#define GDU_SPINNER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GDU_TYPE_SPINNER, GduSpinnerPrivate))

G_DEFINE_TYPE (GduSpinner, gdu_spinner, GTK_TYPE_DRAWING_AREA);

enum
{
	PROP_0,
	PROP_COLOR,
	PROP_NUM_LINES
};

/* STRUCTS & ENUMS */
struct GduSpinnerPrivate
{
	/* state */
	gboolean have_alpha;
	guint current;
	guint timeout;

	/* appearance */
	BlingColor color;
	guint lines;
};

/* FORWARDS */
static void gdu_spinner_class_init(GduSpinnerClass *klass);
static void gdu_spinner_init(GduSpinner *spinner);
static void gdu_spinner_set_property(GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static gboolean gdu_spinner_expose(GtkWidget *widget, GdkEventExpose *event);
static void gdu_spinner_screen_changed (GtkWidget* widget, GdkScreen* old_screen);

static GtkDrawingAreaClass *parent_class;

/* DRAWING FUNCTIONS */
static void
draw (GtkWidget *widget, cairo_t *cr)
{
	GtkAllocation allocation;
	double x, y;
	double radius;
	double half;
	guint i;
	int width, height;

	GduSpinnerPrivate *priv;

	priv = GDU_SPINNER_GET_PRIVATE (widget);

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	gtk_widget_get_allocation (widget, &allocation);
	width = allocation.width;
	height = allocation.height;

	if ( (width < 12) || (height <12) ) {
		gtk_widget_set_size_request(widget, 12, 12);
		gtk_widget_get_allocation (widget, &allocation);
		width = allocation.width;
		height = allocation.height;
	}

	/* x = widget->allocation.x + widget->allocation.width / 2;
	 * y = widget->allocation.y + widget->allocation.height / 2; */
	x = width / 2;
	y = height / 2;
	radius = MIN (width / 2, height / 2);
	half = priv->lines / 2;

	/*FIXME: render in B&W for non transparency */

	for (i = 0; i < priv->lines; i++) {
		int inset = 0.7 * radius;
		/* transparency is a function of time and intial value */
		double t = (double) ((i + priv->lines - priv->current)
				   % priv->lines) / priv->lines;

		cairo_save (cr);

		cairo_set_source_rgba (cr, 0, 0, 0, t);
		//cairo_set_line_width (cr, 2 * cairo_get_line_width (cr));
		cairo_set_line_width (cr, 2.0);
		cairo_move_to (cr,
					   x + (radius - inset) * cos (i * M_PI / half),
					   y + (radius - inset) * sin (i * M_PI / half));
		cairo_line_to (cr,
					   x + radius * cos (i * M_PI / half),
					   y + radius * sin (i * M_PI / half));
		cairo_stroke (cr);

		cairo_restore (cr);
	}
}


/*	GOBJECT INIT CODE */
static void
gdu_spinner_class_init(GduSpinnerClass *klass)
{
	GObjectClass *gobject_class;
	GtkWidgetClass *widget_class;

	parent_class = g_type_class_peek_parent(klass);

	gobject_class = G_OBJECT_CLASS(klass);
	g_type_class_add_private (gobject_class, sizeof (GduSpinnerPrivate));
	gobject_class->set_property = gdu_spinner_set_property;

	widget_class = GTK_WIDGET_CLASS(klass);
	widget_class->expose_event = gdu_spinner_expose;
	widget_class->screen_changed = gdu_spinner_screen_changed;

	g_object_class_install_property(gobject_class, PROP_COLOR,
		g_param_spec_string("color", "Color",
							"Main color",
							"454545C8",
							G_PARAM_CONSTRUCT | G_PARAM_WRITABLE));

	g_object_class_install_property(gobject_class, PROP_NUM_LINES,
		g_param_spec_uint("lines", "Num Lines",
							"The number of lines to animate",
							0,20,12,
							G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE));

}

static void
gdu_spinner_init (GduSpinner *spinner)
{
	GduSpinnerPrivate *priv;

	priv = GDU_SPINNER_GET_PRIVATE (spinner);
	priv->current = 0;
	priv->timeout = 0;

	gtk_widget_set_has_window (GTK_WIDGET (spinner), FALSE);
}

static gboolean
gdu_spinner_expose (GtkWidget *widget, GdkEventExpose *event)
{
	cairo_t *cr;

	/* get cairo context */
	cr = gdk_cairo_create (gtk_widget_get_window (widget));

	/* set a clip region for the expose event */
	cairo_rectangle (cr,
					 event->area.x, event->area.y,
					 event->area.width, event->area.height);
	cairo_clip (cr);

	cairo_translate (cr, event->area.x, event->area.y);

	/* draw clip region */
	draw (widget, cr);

	/* free memory */
	cairo_destroy (cr);

	return FALSE;
}

static void
gdu_spinner_screen_changed (GtkWidget* widget, GdkScreen* old_screen)
{
	GduSpinner *spinner;
	GduSpinnerPrivate *priv;
	GdkScreen* new_screen;
	GdkColormap* colormap;

	spinner = GDU_SPINNER(widget);
	priv = GDU_SPINNER_GET_PRIVATE (spinner);

	new_screen = gtk_widget_get_screen (widget);
	colormap = gdk_screen_get_rgba_colormap (new_screen);

	if (!colormap) {
		colormap = gdk_screen_get_rgb_colormap (new_screen);
		priv->have_alpha = FALSE;
	} else
		priv->have_alpha = TRUE;

	gtk_widget_set_colormap (widget, colormap);
}

static gboolean
gdu_spinner_timeout (gpointer data)
{
	GduSpinner *spinner;
	GduSpinnerPrivate *priv;

	spinner = GDU_SPINNER (data);
	priv = GDU_SPINNER_GET_PRIVATE (spinner);

	if (priv->current + 1 >= priv->lines) {
		priv->current = 0;
	} else {
		priv->current++;
	}

	gtk_widget_queue_draw (GTK_WIDGET (data));

	return TRUE;
}

static void
gdu_spinner_set_property(GObject *gobject, guint prop_id,
					const GValue *value, GParamSpec *pspec)
{
	GduSpinner *spinner;
	GduSpinnerPrivate *priv;

	spinner = GDU_SPINNER(gobject);
	priv = GDU_SPINNER_GET_PRIVATE (spinner);

	switch (prop_id)
	{
		case PROP_COLOR:
			bling_color_parse_string (&priv->color, g_value_get_string(value));
			break;
		case PROP_NUM_LINES:
			priv->lines = g_value_get_uint(value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, prop_id, pspec);
			break;
	}
}

/**
 * gdu_spinner_new
 *
 * Returns a default spinner. Not yet started.
 *
 * Returns: a new #GduSpinner
 */
GtkWidget *
gdu_spinner_new (void)
{
	return g_object_new (GDU_TYPE_SPINNER, NULL);
}

/**
 * gdu_spinner_start
 *
 * Starts the animation
 */
void
gdu_spinner_start (GduSpinner *spinner)
{
	GduSpinnerPrivate *priv;

	g_return_if_fail (GDU_IS_SPINNER (spinner));

	priv = GDU_SPINNER_GET_PRIVATE (spinner);
	if (priv->timeout != 0)
		return;
	priv->timeout = g_timeout_add (80, gdu_spinner_timeout, spinner);
}

/**
 * gdu_spinner_stop
 *
 * Stops the animation
 */
void
gdu_spinner_stop (GduSpinner *spinner)
{
	GduSpinnerPrivate *priv;

	g_return_if_fail (GDU_IS_SPINNER (spinner));

	priv = GDU_SPINNER_GET_PRIVATE (spinner);
	if (priv->timeout == 0)
		return;
	g_source_remove (priv->timeout);
	priv->timeout = 0;
}
