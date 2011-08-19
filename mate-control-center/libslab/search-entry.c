/*
 * This file is part of libslab.
 *
 * Copyright (c) 2006 Novell, Inc.
 *
 * Libslab is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * Libslab is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libslab; if not, write to the Free Software Foundation, Inc., 51
 * Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "search-entry.h"
#include "search-entry-watermark.h"

#include <librsvg/rsvg.h>
#include <string.h>

typedef struct
{
	GdkPixbuf *watermark;
	int width, height;
} NldSearchEntryPrivate;

#define NLD_SEARCH_ENTRY_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), NLD_TYPE_SEARCH_ENTRY, NldSearchEntryPrivate))

static void nld_search_entry_class_init (NldSearchEntryClass *);
static void nld_search_entry_init (NldSearchEntry *);
static void nld_search_entry_finalize (GObject *);

static void nld_search_entry_realize (GtkWidget * widget);
static gboolean nld_search_entry_expose_event (GtkWidget * widget, GdkEventExpose * event);

G_DEFINE_TYPE (NldSearchEntry, nld_search_entry, GTK_TYPE_ENTRY)

static void nld_search_entry_class_init (NldSearchEntryClass * nld_search_entry_class)
{
	GObjectClass *g_obj_class = G_OBJECT_CLASS (nld_search_entry_class);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (nld_search_entry_class);

	g_type_class_add_private (nld_search_entry_class, sizeof (NldSearchEntryPrivate));

	widget_class->realize = nld_search_entry_realize;
	widget_class->expose_event = nld_search_entry_expose_event;

	g_obj_class->finalize = nld_search_entry_finalize;
}

static void
nld_search_entry_init (NldSearchEntry * entry)
{
}

static void
nld_search_entry_finalize (GObject * object)
{
	NldSearchEntryPrivate *priv = NLD_SEARCH_ENTRY_GET_PRIVATE (object);

	if (priv->watermark)
		g_object_unref (priv->watermark);

	G_OBJECT_CLASS (nld_search_entry_parent_class)->finalize (object);
}

static void
rsvg_size_callback (int *width, int *height, gpointer user_data)
{
	NldSearchEntryPrivate *priv = user_data;

	*width = priv->width = priv->height * (double) *width / (double) *height;
	*height = priv->height;
}

static void
nld_search_entry_realize (GtkWidget * widget)
{
	NldSearchEntryPrivate *priv = NLD_SEARCH_ENTRY_GET_PRIVATE (widget);
	int height;
	GdkColor *gdkcolor;
	char *svg, color[7];
	RsvgHandle *rsvg;

	GTK_WIDGET_CLASS (nld_search_entry_parent_class)->realize (widget);

	gdk_window_get_geometry (GTK_ENTRY (widget)->text_area, NULL, NULL, NULL, &height, NULL);

	if (height - 2 == priv->height)
		return;
	priv->height = height - 2;

	gdkcolor = &widget->style->fg[GTK_WIDGET_STATE (widget)];
	snprintf (color, 6, "%02x%02x%02x", gdkcolor->red >> 8, gdkcolor->green >> 8,
		gdkcolor->blue >> 8);
	svg = g_strdup_printf (SEARCH_ENTRY_WATERMARK_SVG, color, color);

	rsvg = rsvg_handle_new ();
	rsvg_handle_set_size_callback (rsvg, rsvg_size_callback, priv, NULL);
	rsvg_handle_write (rsvg, (const guchar *) svg, strlen (svg), NULL);
	rsvg_handle_close (rsvg, NULL);
	g_free (svg);

	if (priv->watermark)
		g_object_unref (priv->watermark);
	priv->watermark = rsvg_handle_get_pixbuf (rsvg);
	rsvg_handle_free (rsvg);
}

static gboolean
nld_search_entry_expose_event (GtkWidget * widget, GdkEventExpose * event)
{
	NldSearchEntryPrivate *priv = NLD_SEARCH_ENTRY_GET_PRIVATE (widget);
	GTK_WIDGET_CLASS (nld_search_entry_parent_class)->expose_event (widget, event);

	if (event->window == GTK_ENTRY (widget)->text_area)
	{
		int width, height, x;

		if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_LTR)
		{
			gdk_drawable_get_size (event->window, &width, &height);
			x = width - priv->width - 1;
		}
		else
			x = 1;
		gdk_draw_pixbuf (event->window, widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
			priv->watermark, 0, 0, x, 1, priv->width, priv->height,
			GDK_RGB_DITHER_NORMAL, 0, 0);
	}

	return FALSE;
}

GtkWidget *
nld_search_entry_new (void)
{
	return g_object_new (NLD_TYPE_SEARCH_ENTRY, NULL);
}
