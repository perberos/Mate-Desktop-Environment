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
/* Miscellaneous utility functions for the MateCanvas widget
 *
 * MateCanvas is basically a port of the Tk toolkit's most excellent canvas widget.  Tk is
 * copyrighted by the Regents of the University of California, Sun Microsystems, and other parties.
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#ifndef MATE_CANVAS_UTIL_H
#define MATE_CANVAS_UTIL_H

#include <libmatecanvas/mate-canvas.h>

#include <libart_lgpl/art_svp.h>
#include <libart_lgpl/art_vpath.h>
#include <libart_lgpl/art_svp_vpath_stroke.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef struct _MateCanvasPoints MateCanvasPoints;

/* This structure defines an array of points.  X coordinates are stored in the even-numbered
 * indices, and Y coordinates are stored in the odd-numbered indices.  num_points indicates the
 * number of points, so the array is 2*num_points elements big.
 */
struct _MateCanvasPoints {
	double *coords;
	int num_points;
	int ref_count;
};


/* Allocate a new MateCanvasPoints structure with enough space for the specified number of points */
MateCanvasPoints *mate_canvas_points_new (int num_points);

/* Increate ref count */
MateCanvasPoints *mate_canvas_points_ref (MateCanvasPoints *points);
#define mate_canvas_points_unref mate_canvas_points_free

/* Decrease ref count and free structure if it has reached zero */
void mate_canvas_points_free (MateCanvasPoints *points);

/* Given three points forming an angle, compute the coordinates of the inside and outside points of
 * the mitered corner formed by a line of a given width at that angle.
 *
 * If the angle is less than 11 degrees, then FALSE is returned and the return points are not
 * modified.  Otherwise, TRUE is returned.
 */
int mate_canvas_get_miter_points (double x1, double y1, double x2, double y2, double x3, double y3,
				   double width,
				   double *mx1, double *my1, double *mx2, double *my2);

/* Compute the butt points of a line segment.  If project is FALSE, then the results are as follows:
 *
 *            -------------------* (bx1, by1)
 *                               |
 *   (x1, y1) *------------------* (x2, y2)
 *                               |
 *            -------------------* (bx2, by2)
 *
 * that is, the line is not projected beyond (x2, y2).  If project is TRUE, then the results are as
 * follows:
 *
 *            -------------------* (bx1, by1)
 *                      (x2, y2) |
 *   (x1, y1) *-------------*    |
 *                               |
 *            -------------------* (bx2, by2)
 */
void mate_canvas_get_butt_points (double x1, double y1, double x2, double y2,
				   double width, int project,
				   double *bx1, double *by1, double *bx2, double *by2);

/* Calculate the distance from a polygon to a point.  The polygon's X coordinates are in the even
 * indices of the poly array, and the Y coordinates are in the odd indices.
 */
double mate_canvas_polygon_to_point (double *poly, int num_points, double x, double y);


/* Render the svp over the buf. */
void mate_canvas_render_svp (MateCanvasBuf *buf, ArtSVP *svp, guint32 rgba);

/* Sets the svp to the new value, requesting repaint on what's changed. This function takes responsibility for
 * freeing new_svp.
 */
void mate_canvas_update_svp (MateCanvas *canvas, ArtSVP **p_svp, ArtSVP *new_svp);

/* Sets the svp to the new value, clipping if necessary, and requesting repaint
 * on what's changed. This function takes responsibility for freeing new_svp.
 */
void mate_canvas_update_svp_clip (MateCanvas *canvas, ArtSVP **p_svp, ArtSVP *new_svp,
				   ArtSVP *clip_svp);

/* Sets the svp to the new value, requesting repaint on what's changed. This
 * function takes responsibility for freeing new_svp. This routine also adds the
 * svp's bbox to the item's.
 */
void mate_canvas_item_reset_bounds (MateCanvasItem *item);

/* Sets the svp to the new value, requesting repaint on what's changed. This function takes responsibility for
 * freeing new_svp. This routine also adds the svp's bbox to the item's.
 */
void mate_canvas_item_update_svp (MateCanvasItem *item, ArtSVP **p_svp, ArtSVP *new_svp);

/* Sets the svp to the new value, clipping if necessary, and requesting repaint
 * on what's changed. This function takes responsibility for freeing new_svp.
 */
void mate_canvas_item_update_svp_clip (MateCanvasItem *item, ArtSVP **p_svp, ArtSVP *new_svp,
					ArtSVP *clip_svp);

/* Request redraw of the svp if in aa mode, or the entire item in in xlib
 * mode.
 */
void mate_canvas_item_request_redraw_svp (MateCanvasItem *item, const ArtSVP *svp);

/* Sets the bbox to the new value, requesting full repaint. */
void mate_canvas_update_bbox (MateCanvasItem *item, int x1, int y1, int x2, int y2);

/* Ensure that the buffer is in RGB format, suitable for compositing. */
void mate_canvas_buf_ensure_buf (MateCanvasBuf *buf);

/* Convert from GDK line join specifier to libart. */
ArtPathStrokeJoinType mate_canvas_join_gdk_to_art (GdkJoinStyle gdk_join);

/* Convert from GDK line cap specifier to libart. */
ArtPathStrokeCapType mate_canvas_cap_gdk_to_art (GdkCapStyle gdk_cap);

#ifdef __cplusplus
}
#endif

#endif
