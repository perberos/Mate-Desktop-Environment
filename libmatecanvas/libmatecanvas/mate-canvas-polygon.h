/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
 * All rights reserved.
 *
 * This file is part of the Mate Library.
 *
 * The Mate Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Mate Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Mate Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */
/* Polygon item type for MateCanvas widget
 *
 * MateCanvas is basically a port of the Tk toolkit's most excellent canvas widget.  Tk is
 * copyrighted by the Regents of the University of California, Sun Microsystems, and other parties.
 *
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 *         Rusty Conover <rconover@bangtail.net>
 */

#ifndef MATE_CANVAS_POLYGON_H
#define MATE_CANVAS_POLYGON_H


#include <libmatecanvas/mate-canvas.h>
#include <libmatecanvas/mate-canvas-shape.h>
#include <libmatecanvas/mate-canvas-path-def.h>

G_BEGIN_DECLS


/* Polygon item for the canvas.  A polygon is a bit different from rectangles and ellipses in that
 * points inside it will always be considered "inside", even if the fill color is not set.  If you
 * want to have a hollow polygon, use a line item instead.
 *
 * The following object arguments are available:
 *
 * name			type			read/write	description
 * ------------------------------------------------------------------------------------------
 * points		MateCanvasPoints*	RW		Pointer to a MateCanvasPoints structure.
 *								This can be created by a call to
 *								mate_canvas_points_new() (in mate-canvas-util.h).
 *								X coordinates are in the even indices of the
 *								points->coords array, Y coordinates are in
 *								the odd indices.
 */

#define MATE_TYPE_CANVAS_POLYGON            (mate_canvas_polygon_get_type ())
#define MATE_CANVAS_POLYGON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_CANVAS_POLYGON, MateCanvasPolygon))
#define MATE_CANVAS_POLYGON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_CANVAS_POLYGON, MateCanvasPolygonClass))
#define MATE_IS_CANVAS_POLYGON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_CANVAS_POLYGON))
#define MATE_IS_CANVAS_POLYGON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_CANVAS_POLYGON))
#define MATE_CANVAS_POLYGON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATE_TYPE_CANVAS_POLYGON, MateCanvasPolygonClass))


typedef struct _MateCanvasPolygon MateCanvasPolygon;
typedef struct _MateCanvasPolygonClass MateCanvasPolygonClass;

struct _MateCanvasPolygon {
	MateCanvasShape item;

	MateCanvasPathDef *path_def;
};

struct _MateCanvasPolygonClass {
	MateCanvasShapeClass parent_class;
};


/* Standard Gtk function */
GType mate_canvas_polygon_get_type (void) G_GNUC_CONST;

G_END_DECLS
#endif
