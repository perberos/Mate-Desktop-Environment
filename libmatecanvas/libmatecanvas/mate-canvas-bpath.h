/* Bpath item type for MateCanvas widget
 *
 * MateCanvas is basically a port of the Tk toolkit's most excellent canvas widget.  Tk is
 * copyrighted by the Regents of the University of California, Sun Microsystems, and other parties.
 *
 * Copyright (C) 1998,1999 The Free Software Foundation
 *
 * Authors: Federico Mena <federico@nuclecu.unam.mx>
 *          Raph Levien <raph@acm.org>
 *          Lauris Kaplinski <lauris@ximian.com>
 *          Rusty Conover <rconover@bangtail.net>
 */

#ifndef MATE_CANVAS_BPATH_H
#define MATE_CANVAS_BPATH_H

#include <libmatecanvas/mate-canvas.h>
#include <libmatecanvas/mate-canvas-shape.h>
#include <libmatecanvas/mate-canvas-path-def.h>

#ifdef __cplusplus
extern "C" {
#endif


/* Bpath item for the canvas.
 *
 * The following object arguments are available:
 *
 * name			type			read/write	description
 * ------------------------------------------------------------------------------------------
 * bpath		MateCanvasPathDef *		RW		Pointer to an MateCanvasPathDef structure.
 *								This can be created by a call to
 *								gp_path_new() in (gp-path.h).
 */

#define MATE_TYPE_CANVAS_BPATH            (mate_canvas_bpath_get_type ())
#define MATE_CANVAS_BPATH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_CANVAS_BPATH, MateCanvasBpath))
#define MATE_CANVAS_BPATH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_CANVAS_BPATH, MateCanvasBpathClass))
#define MATE_IS_CANVAS_BPATH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_CANVAS_BPATH))
#define MATE_IS_CANVAS_BPATH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_CANVAS_BPATH))


typedef struct _MateCanvasBpath MateCanvasBpath;
typedef struct _MateCanvasBpathPriv MateCanvasBpathPriv;
typedef struct _MateCanvasBpathClass MateCanvasBpathClass;

struct _MateCanvasBpath {
	MateCanvasShape item;

};

struct _MateCanvasBpathClass {
	MateCanvasShapeClass parent_class;
};


/* Standard Gtk function */
GType mate_canvas_bpath_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif
