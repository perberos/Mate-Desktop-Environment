#ifndef MATE_CANVAS_CLIPGROUP_H
#define MATE_CANVAS_CLIPGROUP_H

/* Clipping group implementation for MateCanvas
 *
 * MateCanvas is basically a port of the Tk toolkit's most excellent canvas widget.  Tk is
 * copyrighted by the Regents of the University of California, Sun Microsystems, and other parties.
 *
 * TODO: Implement this in libmateui, possibly merge with real group
 *
 * Copyright (C) 1998,1999 The Free Software Foundation
 *
 * Author:
 *          Lauris Kaplinski <lauris@ximian.com>
 */

#include <libmatecanvas/mate-canvas.h>
#include <libmatecanvas/mate-canvas-util.h>

#include <libart_lgpl/art_bpath.h>
#include <libart_lgpl/art_svp_wind.h>
#include <libart_lgpl/art_vpath_dash.h>
#include <libmatecanvas/mate-canvas-path-def.h>

#ifdef __cplusplus
extern "C" {
#endif


#define MATE_TYPE_CANVAS_CLIPGROUP            (mate_canvas_clipgroup_get_type ())
#define MATE_CANVAS_CLIPGROUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_CANVAS_CLIPGROUP, MateCanvasClipgroup))
#define MATE_CANVAS_CLIPGROUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_CANVAS_CLIPGROUP, MateCanvasClipgroupClass))
#define MATE_IS_CANVAS_CLIPGROUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_CANVAS_CLIPGROUP))
#define MATE_IS_CANVAS_CLIPGROUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_CANVAS_CLIPGROUP))


typedef struct _MateCanvasClipgroup MateCanvasClipgroup;
typedef struct _MateCanvasClipgroupClass MateCanvasClipgroupClass;

struct _MateCanvasClipgroup {
	MateCanvasGroup group;

	MateCanvasPathDef * path;
	ArtWindRule wind;

	ArtSVP * svp;
};

struct _MateCanvasClipgroupClass {
	MateCanvasGroupClass parent_class;
};


/* Standard Gtk function */
GType mate_canvas_clipgroup_get_type (void) G_GNUC_CONST;


G_END_DECLS

#endif
