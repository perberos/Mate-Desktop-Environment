/* MATE libraries - GdkPixbuf item for the MATE canvas
 *
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Author: Federico Mena-Quintero <federico@gimp.org>
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

#ifndef MATE_CANVAS_PIXBUF_H
#define MATE_CANVAS_PIXBUF_H


#include <libmatecanvas/mate-canvas.h>

#ifdef __cplusplus
extern "C" {
#endif


#define MATE_TYPE_CANVAS_PIXBUF            (mate_canvas_pixbuf_get_type ())
#define MATE_CANVAS_PIXBUF(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_CANVAS_PIXBUF, MateCanvasPixbuf))
#define MATE_CANVAS_PIXBUF_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_CANVAS_PIXBUF, MateCanvasPixbufClass))
#define MATE_IS_CANVAS_PIXBUF(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_CANVAS_PIXBUF))
#define MATE_IS_CANVAS_PIXBUF_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_CANVAS_PIXBUF))
#define MATE_CANVAS_PIXBUF_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATE_TYPE_CANVAS_PIXBUF, MateCanvasPixbufClass))


typedef struct _MateCanvasPixbuf MateCanvasPixbuf;
typedef struct _MateCanvasPixbufClass MateCanvasPixbufClass;

struct _MateCanvasPixbuf {
	MateCanvasItem item;

	/* Private data */
	gpointer priv;
};

struct _MateCanvasPixbufClass {
	MateCanvasItemClass parent_class;
};


GType mate_canvas_pixbuf_get_type (void) G_GNUC_CONST;


#ifdef __cplusplus
}
#endif

#endif
