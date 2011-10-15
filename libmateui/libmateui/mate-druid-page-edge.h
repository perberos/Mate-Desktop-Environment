/* mate-druid-page-edge.h
 * Copyright (C) 1999  Red Hat, Inc.
 *
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
*/
#ifndef __MATE_DRUID_PAGE_EDGE_H__
#define __MATE_DRUID_PAGE_EDGE_H__

#ifndef MATE_DISABLE_DEPRECATED

#include <gtk/gtk.h>
#include <libmatecanvas/mate-canvas.h>
#include "mate-druid-page.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MATE_TYPE_DRUID_PAGE_EDGE            (mate_druid_page_edge_get_type ())
#define MATE_DRUID_PAGE_EDGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_DRUID_PAGE_EDGE, MateDruidPageEdge))
#define MATE_DRUID_PAGE_EDGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_DRUID_PAGE_EDGE, MateDruidPageEdgeClass))
#define MATE_IS_DRUID_PAGE_EDGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_DRUID_PAGE_EDGE))
#define MATE_IS_DRUID_PAGE_EDGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_DRUID_PAGE_EDGE))
#define MATE_DRUID_PAGE_EDGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATE_TYPE_DRUID_PAGE_EDGE, MateDruidPageEdgeClass))

/**
 * MateEdgePosition:
 * @MATE_EDGE_START: The current page is at the beginning of the druid.
 * @MATE_EDGE_FINISH: The current page is at the end of the druid.
 * @MATE_EDGE_OTHER: The current page is neither the first nor the last page
 * (usually not required).
 * @MATE_EDGE_LAST: Used internally to indicate the last value of the
 * enumeration. This should not be passed in to any function expecting a
 * #MateEdgePosition value.
 *
 * Used to pass around information about the position of a #MateDruidPage
 * within the overall #MateDruid. This enables the correct "surrounding"
 * content for the page to be drawn.
 */
typedef enum {
  /* update structure when adding enums */
	MATE_EDGE_START,
	MATE_EDGE_FINISH,
	MATE_EDGE_OTHER,
	MATE_EDGE_LAST /* for counting purposes */
} MateEdgePosition;


typedef struct _MateDruidPageEdge        MateDruidPageEdge;
typedef struct _MateDruidPageEdgePrivate MateDruidPageEdgePrivate;
typedef struct _MateDruidPageEdgeClass   MateDruidPageEdgeClass;

/**
 * MateDruidPageEdge:
 * @title: The current title of the displayed page.
 * @text: The current text of the displayed page.
 * @logo_image: The logo of the displayed page.
 * @watermark_image: The watermark on the left side of the displayed page.
 * @top_watermark_image: The watermark on the top of the displayed page.
 * @background_color: The color of the edge of the current page (outside the
 * text area).
 * @textbox_color: The color of the textbox area of the displayed page.
 * @logo_background_color: The background color of the displayed page's logo.
 * @title_color: The color of the title text.
 * @text_color: The color of the body text.
 * @position: The position of the current page within the druid (a
 * #MateEdgePosition value).
 *
 * A widget holding information about the overall look of the currently
 * displaying druid page.
 */
struct _MateDruidPageEdge
{
	MateDruidPage parent;

	/*< public >*/
	gchar *title;
	gchar *text;
	GdkPixbuf *logo_image;
	GdkPixbuf *watermark_image;
	GdkPixbuf *top_watermark_image;

	GdkColor background_color;
	GdkColor textbox_color;
	GdkColor logo_background_color;
	GdkColor title_color;
	GdkColor text_color;

	guint position : 2; /* MateEdgePosition */

	/*< private >*/
	MateDruidPageEdgePrivate *_priv;
};

struct _MateDruidPageEdgeClass
{
	MateDruidPageClass parent_class;

	/* Padding for possible expansion */
	gpointer padding1;
	gpointer padding2;
};

GType      mate_druid_page_edge_get_type          (void) G_GNUC_CONST;
GtkWidget *mate_druid_page_edge_new               (MateEdgePosition   position);
GtkWidget *mate_druid_page_edge_new_aa            (MateEdgePosition   position);
GtkWidget *mate_druid_page_edge_new_with_vals     (MateEdgePosition   position,
						    gboolean            antialiased,
						    const gchar        *title,
						    const gchar        *text,
						    GdkPixbuf          *logo,
						    GdkPixbuf          *watermark,
						    GdkPixbuf	       *top_watermark);
void       mate_druid_page_edge_construct         (MateDruidPageEdge *druid_page_edge,
						    MateEdgePosition   position,
						    gboolean            antialiased,
						    const gchar        *title,
						    const gchar        *text,
						    GdkPixbuf          *logo,
						    GdkPixbuf          *watermark,
						    GdkPixbuf          *top_watermark);
void       mate_druid_page_edge_set_bg_color      (MateDruidPageEdge *druid_page_edge,
						    GdkColor           *color);
void       mate_druid_page_edge_set_textbox_color (MateDruidPageEdge *druid_page_edge,
						    GdkColor           *color);
void       mate_druid_page_edge_set_logo_bg_color (MateDruidPageEdge *druid_page_edge,
						    GdkColor           *color);
void       mate_druid_page_edge_set_title_color   (MateDruidPageEdge *druid_page_edge,
						    GdkColor           *color);
void       mate_druid_page_edge_set_text_color    (MateDruidPageEdge *druid_page_edge,
						    GdkColor           *color);
void       mate_druid_page_edge_set_text          (MateDruidPageEdge *druid_page_edge,
						    const gchar        *text);
void       mate_druid_page_edge_set_title         (MateDruidPageEdge *druid_page_edge,
						    const gchar        *title);
void       mate_druid_page_edge_set_logo          (MateDruidPageEdge *druid_page_edge,
						    GdkPixbuf          *logo_image);
void       mate_druid_page_edge_set_watermark     (MateDruidPageEdge *druid_page_edge,
						    GdkPixbuf          *watermark);
void       mate_druid_page_edge_set_top_watermark (MateDruidPageEdge *druid_page_edge,
						    GdkPixbuf          *top_watermark_image);

#ifdef __cplusplus
}
#endif

#endif /* MATE_DISABLE_DEPRECATED */

#endif /* __MATE_DRUID_PAGE_EDGE_H__ */
