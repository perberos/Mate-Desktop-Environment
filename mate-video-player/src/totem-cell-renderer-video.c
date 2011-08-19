/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2001-2007 Philip Withnall <philip@tecnocode.co.uk>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
 *
 * The Totem project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and Totem. This
 * permission are above and beyond the permissions granted by the GPL license
 * Totem is covered by.
 *
 * Monday 7th February 2005: Christian Schaller: Add exception clause.
 * See license_change file for details.
 *
 */

/**
 * SECTION:totem-cell-renderer-video
 * @short_description: a #GtkCellRenderer widget for listing videos
 * @stability: Unstable
 * @include: totem-cell-renderer-video.h
 * @see_also: #TotemVideoList
 *
 * #TotemCellRendererVideo is a #GtkCellRenderer for rendering video thumbnails, typically in a #TotemVideoList.
 * It supports drawing a video thumbnail, and the video's title underneath.
 **/

#include "config.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <string.h>

#include "totem.h"
#include "totem-cell-renderer-video.h"
#include "totem-private.h"

struct _TotemCellRendererVideoPrivate {
	gchar *title;
	GdkPixbuf *thumbnail;
	PangoAlignment alignment;
	gboolean use_placeholder;
};

#define TOTEM_CELL_RENDERER_VIDEO_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TOTEM_TYPE_CELL_RENDERER_VIDEO, TotemCellRendererVideoPrivate))

enum {
	PROP_THUMBNAIL = 1,
	PROP_TITLE,
	PROP_ALIGNMENT,
	PROP_USE_PLACEHOLDER
};

static void totem_cell_renderer_video_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void totem_cell_renderer_video_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void totem_cell_renderer_video_dispose (GObject *object);
static void totem_cell_renderer_video_finalize (GObject *object);
static void totem_cell_renderer_video_get_size (GtkCellRenderer *cell, GtkWidget *widget, GdkRectangle *cell_area, gint *x_offset, gint *y_offset, gint *width, gint *height);
static void totem_cell_renderer_video_render (GtkCellRenderer *cell, GdkDrawable *window, GtkWidget *widget, GdkRectangle *background_area, GdkRectangle *cell_area, GdkRectangle *expose_area, GtkCellRendererState flags);

G_DEFINE_TYPE (TotemCellRendererVideo, totem_cell_renderer_video, GTK_TYPE_CELL_RENDERER)

/**
 * totem_cell_renderer_video_new:
 * @use_placeholder: if %TRUE, use a placeholder thumbnail
 *
 * Creates a new #TotemCellRendererVideo with its #TotemCellRendererVideo:use-placeholder
 * property set to @use_placeholder. If @use_placeholder is %TRUE,
 * the renderer will display a generic placeholder thumbnail if a
 * proper one is not available.
 *
 * Return value: a new #TotemCellRendererVideo
 **/
TotemCellRendererVideo *
totem_cell_renderer_video_new (gboolean use_placeholder)
{
	return g_object_new (TOTEM_TYPE_CELL_RENDERER_VIDEO,
				"use-placeholder", use_placeholder,
				NULL); 
}

static void
totem_cell_renderer_video_class_init (TotemCellRendererVideoClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkCellRendererClass *renderer_class = GTK_CELL_RENDERER_CLASS (klass);

	g_type_class_add_private (klass, sizeof (TotemCellRendererVideoPrivate));

	object_class->set_property = totem_cell_renderer_video_set_property;
	object_class->get_property = totem_cell_renderer_video_get_property;
	object_class->dispose = totem_cell_renderer_video_dispose;
	object_class->finalize = totem_cell_renderer_video_finalize;
	renderer_class->get_size = totem_cell_renderer_video_get_size;
	renderer_class->render = totem_cell_renderer_video_render;

	/**
	 * TotemCellRendererVideo:thumbnail:
	 *
	 * A #GdkPixbuf of a thumbnail of the video to display. When rendered, it will scale to the width of the parent #GtkTreeView,
	 * so can be as large as reasonable.
	 **/
	g_object_class_install_property (object_class, PROP_THUMBNAIL,
				g_param_spec_object ("thumbnail", NULL, NULL,
					GDK_TYPE_PIXBUF, G_PARAM_READWRITE));

	/**
	 * TotemCellRendererVideo:title:
	 *
	 * The title of the video, as it should be displayed.
	 **/
	g_object_class_install_property (object_class, PROP_TITLE,
				g_param_spec_string ("title", NULL, NULL,
					_("Unknown video"), G_PARAM_READWRITE));

	/**
	 * TotemCellRendererVideo:alignment:
	 *
	 * A #PangoAlignment giving the text alignment for the video title.
	 **/
	g_object_class_install_property (object_class, PROP_ALIGNMENT,
				g_param_spec_enum ("alignment", NULL, NULL,
					PANGO_TYPE_ALIGNMENT,
					PANGO_ALIGN_CENTER,
					G_PARAM_READWRITE));

	/**
	 * TotemCellRendererVideo:use-placeholder:
	 *
	 * If %TRUE, a placeholder image should be used for the video thumbnail if a #TotemCellRendererVideo:thumbnail isn't provided.
	 **/
	g_object_class_install_property (object_class, PROP_USE_PLACEHOLDER,
				g_param_spec_boolean ("use-placeholder", NULL, NULL,
					FALSE, G_PARAM_READWRITE));
}

static void
totem_cell_renderer_video_init (TotemCellRendererVideo *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, TOTEM_TYPE_CELL_RENDERER_VIDEO, TotemCellRendererVideoPrivate);
	self->priv->title = NULL;
	self->priv->thumbnail = NULL;
	self->priv->alignment = PANGO_ALIGN_CENTER;
	self->priv->use_placeholder = FALSE;

	/* Make sure we're in the right mode */
	g_object_set ((gpointer) self, "mode", GTK_CELL_RENDERER_MODE_ACTIVATABLE, NULL);
}

static void
totem_cell_renderer_video_dispose (GObject *object)
{
	TotemCellRendererVideo *self = TOTEM_CELL_RENDERER_VIDEO (object);

	if (self->priv->thumbnail != NULL)
		g_object_unref (self->priv->thumbnail);
	self->priv->thumbnail = NULL;

	/* Chain up to the parent class */
	G_OBJECT_CLASS (totem_cell_renderer_video_parent_class)->dispose (object);
}

static void
totem_cell_renderer_video_finalize (GObject *object)
{
	TotemCellRendererVideo *self = TOTEM_CELL_RENDERER_VIDEO (object);

	g_free (self->priv->title);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (totem_cell_renderer_video_parent_class)->finalize (object);
}

static void
totem_cell_renderer_video_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	TotemCellRendererVideoPrivate *priv = TOTEM_CELL_RENDERER_VIDEO_GET_PRIVATE (object);

	switch (property_id)
	{
		case PROP_THUMBNAIL:
			if (priv->thumbnail != NULL)
				g_object_unref (priv->thumbnail);
			priv->thumbnail = GDK_PIXBUF (g_value_dup_object (value));
			break;
		case PROP_TITLE:
			g_free (priv->title);
			priv->title = g_value_dup_string (value);
			break;
		case PROP_ALIGNMENT:
			priv->alignment = g_value_get_enum (value);
			break;
		case PROP_USE_PLACEHOLDER:
			priv->use_placeholder = g_value_get_boolean (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
totem_cell_renderer_video_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	TotemCellRendererVideoPrivate *priv = TOTEM_CELL_RENDERER_VIDEO_GET_PRIVATE (object);

	switch (property_id)
	{
		case PROP_THUMBNAIL:
			g_value_set_object (value, G_OBJECT (priv->thumbnail));
			break;
		case PROP_TITLE:
			g_value_set_string (value, priv->title);
			break;
		case PROP_ALIGNMENT:
			g_value_set_enum (value, priv->alignment);
			break;
		case PROP_USE_PLACEHOLDER:
			g_value_set_boolean (value, priv->use_placeholder);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
get_size (GtkCellRenderer *cell,
	  GtkWidget *widget,
	  GdkRectangle *cell_area,
	  GdkRectangle *draw_area,
	  GdkRectangle *title_area,
	  GdkRectangle *thumbnail_area)
{
	TotemCellRendererVideoPrivate *priv = TOTEM_CELL_RENDERER_VIDEO (cell)->priv;
	GtkStyle *style;
	guint pixbuf_width = 0;
	guint pixbuf_height = 0;
	guint title_width, title_height;
	guint calc_width, calc_height;
	gint cell_width;
	guint cell_xpad, cell_ypad;
	gfloat cell_xalign, cell_yalign;
	PangoContext *context;
	PangoFontMetrics *metrics;
	PangoFontDescription *font_desc;

	g_object_get (cell,
		      "width", &cell_width,
		      "xpad", &cell_xpad,
		      "ypad", &cell_ypad,
		      "xalign", &cell_xalign,
		      "yalign", &cell_yalign,
		      NULL);

	/* Calculate thumbnail dimensions */
	if (priv->thumbnail != NULL) {
		pixbuf_width = gdk_pixbuf_get_width (priv->thumbnail);
		pixbuf_height = gdk_pixbuf_get_height (priv->thumbnail);
	} else if (priv->use_placeholder == TRUE) {
		gint width, height; /* Sort out signedness weirdness (damn GTK+) */

		if (gtk_icon_size_lookup (GTK_ICON_SIZE_DIALOG, &width, &height) == TRUE) {
			pixbuf_width = width;
			pixbuf_height = height;
		} else {
			pixbuf_width = cell_width;
			pixbuf_height = cell_width;
		}
	}

	/* Calculate title dimensions */
	style = gtk_widget_get_style (widget);
	font_desc = pango_font_description_copy_static (style->font_desc);
	if (priv->thumbnail != NULL)
		pango_font_description_set_weight (font_desc, PANGO_WEIGHT_BOLD);
	context = gtk_widget_get_pango_context (widget);
	metrics = pango_context_get_metrics (context, font_desc, pango_context_get_language (context));

	if (cell_area)
		title_width = cell_area->width;
	else if (cell_width != -1)
		title_width = cell_width;
	else
		title_width = pixbuf_width;

	title_height = PANGO_PIXELS (pango_font_metrics_get_ascent (metrics) + pango_font_metrics_get_descent (metrics));

	pango_font_metrics_unref (metrics);
	pango_font_description_free (font_desc);

	/* Calculate the total final size */
	calc_width = cell_xpad * 2 + MAX (pixbuf_width, title_width);
	calc_height = cell_ypad * 3 + pixbuf_height + title_height;

	if (draw_area) {
		if (cell_area && calc_width > 0 && calc_height > 0) {
			draw_area->x = (((gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL) ?
				(1.0 - cell_xalign) : cell_xalign) * (cell_area->width - calc_width));
			draw_area->x = MAX (draw_area->x, 0);
			draw_area->y = (cell_yalign * (cell_area->height - calc_height));
			draw_area->y = MAX (draw_area->y, 0);
		} else {
			draw_area->x = 0;
			draw_area->y = 0;
		}

		draw_area->x += cell_xpad;
		draw_area->y += cell_ypad;
		draw_area->width = calc_width;
		draw_area->height = calc_height;

		if (title_area) {
			if (cell_area) {
				title_area->width = cell_area->width;
				title_area->x = (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL) ? 1.0 : 0.0;
			} else {
				title_area->width = calc_width;
				title_area->x = draw_area->x;
			}

			title_area->height = title_height;
			if (pixbuf_height > 0)
				title_area->y = draw_area->y + pixbuf_height + cell_ypad;
			else
				title_area->y = draw_area->y;
		}

		if (pixbuf_height > 0 && thumbnail_area) {
			thumbnail_area->x = (((gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL) ?
				(1.0 - cell_xalign) : cell_xalign) *
				(cell_area->width - pixbuf_width));
			thumbnail_area->x = MAX (thumbnail_area->x, 0);
			thumbnail_area->y = draw_area->y;
			thumbnail_area->width = cell_xpad * 2 + pixbuf_width;
			thumbnail_area->height = pixbuf_height;
		}
	}
}

static void
totem_cell_renderer_video_get_size (GtkCellRenderer *cell,
				    GtkWidget *widget,
				    GdkRectangle *cell_area,
				    gint *x_offset,
				    gint *y_offset,
				    gint *width,
				    gint *height)
{
	GdkRectangle draw_area;

	get_size (cell, widget, cell_area, &draw_area, NULL, NULL);

	if (x_offset)
		*x_offset = draw_area.x;
	if (y_offset)
		*y_offset = draw_area.y;
	if (width)
		*width = draw_area.width;
	if (height)
		*height = draw_area.height;
}

static void
totem_cell_renderer_video_render (GtkCellRenderer *cell,
				  GdkDrawable *window,
				  GtkWidget *widget,
				  GdkRectangle *background_area,
				  GdkRectangle *cell_area,
				  GdkRectangle *expose_area,
				  GtkCellRendererState flags)
{
	TotemCellRendererVideoPrivate *priv = TOTEM_CELL_RENDERER_VIDEO (cell)->priv;
	GdkPixbuf *pixbuf;
	GdkRectangle draw_rect;
	GdkRectangle draw_area;
	GdkRectangle title_area;
	GdkRectangle thumbnail_area;
	cairo_t *cr;
	PangoLayout *layout;
	GtkStateType state;
	GtkStyle *style;
	guint cell_xpad, cell_ypad;
	gboolean cell_is_expander;

	g_object_get (cell,
		      "xpad", &cell_xpad,
		      "ypad", &cell_ypad,
		      "is_expander", &cell_is_expander,
		      NULL);

	get_size (cell, widget, cell_area, &draw_area, &title_area, &thumbnail_area);

	draw_area.x += cell_area->x;
	draw_area.y += cell_area->y;
	draw_area.width -= cell_xpad * 2;
	draw_area.height -= cell_ypad * 2;

	if (!gdk_rectangle_intersect (cell_area, &draw_area, &draw_rect) ||
	    !gdk_rectangle_intersect (expose_area, &draw_rect, &draw_rect))
		return;

	/* Sort out the thumbnail */
	if (priv->thumbnail != NULL)
		pixbuf = priv->thumbnail;
	else if (priv->use_placeholder == TRUE)
		pixbuf = gtk_widget_render_icon (widget, GTK_STOCK_MISSING_IMAGE, GTK_ICON_SIZE_DIALOG, NULL);
	else
		pixbuf = NULL;

	if (cell_is_expander)
		return;

	/* Sort out the title */
	if (!gtk_cell_renderer_get_sensitive (cell)) {
		state = GTK_STATE_INSENSITIVE;
	} else if ((flags & GTK_CELL_RENDERER_SELECTED) == GTK_CELL_RENDERER_SELECTED) {
		if (gtk_widget_has_focus (widget))
			state = GTK_STATE_SELECTED;
		else
			state = GTK_STATE_ACTIVE;
	} else if ((flags & GTK_CELL_RENDERER_PRELIT) == GTK_CELL_RENDERER_PRELIT &&
				gtk_widget_get_state (widget) == GTK_STATE_PRELIGHT) {
		state = GTK_STATE_PRELIGHT;
	} else {
		if (gtk_widget_get_state (widget) == GTK_STATE_INSENSITIVE)
			state = GTK_STATE_INSENSITIVE;
		else
			state = GTK_STATE_NORMAL;
	}

	/* Draw the title */
	style = gtk_widget_get_style (widget);
	layout = gtk_widget_create_pango_layout (widget, priv->title);
	if (pixbuf != NULL) {
		PangoFontDescription *desc = pango_font_description_copy_static (style->font_desc);
		pango_font_description_set_weight (desc, PANGO_WEIGHT_BOLD);
		pango_layout_set_font_description (layout, desc);
		pango_font_description_free (desc);
	}

	pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);
	pango_layout_set_width (layout, title_area.width * PANGO_SCALE);
	pango_layout_set_alignment (layout, priv->alignment);

	gtk_paint_layout (style, window, state, TRUE, expose_area, widget, "cellrenderervideotitle",
			  cell_area->x + title_area.x,
			  cell_area->y + title_area.y,
			  layout);

	g_object_unref (layout);

	/* Draw the thumbnail */
	if (pixbuf != NULL) {
		cr = gdk_cairo_create (window);

		gdk_cairo_set_source_pixbuf (cr, pixbuf, cell_area->x + thumbnail_area.x, cell_area->y + thumbnail_area.y);
		gdk_cairo_rectangle (cr, &draw_rect);
		cairo_fill (cr);

		cairo_destroy (cr);
	}
}
