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
 *          Miguel de Icaza <miguel@kernel.org>
 *          Cody Russell <bratsche@mate.org>
 *          Rusty Conover <rconover@bangtail.net>
 */

/* These includes are set up for standalone compile. If/when this codebase
   is integrated into libmateui, the includes will need to change. */

#include <math.h>
#include <string.h>

#include <gtk/gtk.h>
#include "mate-canvas.h"
#include "mate-canvas-util.h"

#include "mate-canvas-bpath.h"
#include "mate-canvas-shape.h"
#include "mate-canvas-shape-private.h"
#include "mate-canvas-path-def.h"

enum {
	PROP_0,
	PROP_BPATH
};

static void mate_canvas_bpath_class_init   (MateCanvasBpathClass *class);
static void mate_canvas_bpath_init         (MateCanvasBpath      *bpath);
static void mate_canvas_bpath_destroy      (GtkObject               *object);
static void mate_canvas_bpath_set_property (GObject               *object,
					     guint                  param_id,
					     const GValue          *value,
                                             GParamSpec            *pspec);
static void mate_canvas_bpath_get_property (GObject               *object,
					     guint                  param_id,
					     GValue                *value,
                                             GParamSpec            *pspec);

static void   mate_canvas_bpath_update      (MateCanvasItem *item, double *affine, ArtSVP *clip_path, int flags);


static MateCanvasShapeClass *parent_class;

GType
mate_canvas_bpath_get_type (void)
{
	static GType bpath_type;

	if (!bpath_type) {
		const GTypeInfo object_info = {
			sizeof (MateCanvasBpathClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) mate_canvas_bpath_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,			/* class_data */
			sizeof (MateCanvasBpath),
			0,			/* n_preallocs */
			(GInstanceInitFunc) mate_canvas_bpath_init,
			NULL			/* value_table */
		};

		bpath_type = g_type_register_static (MATE_TYPE_CANVAS_SHAPE, "MateCanvasBpath",
						     &object_info, 0);
	}

	return bpath_type;
}

static void
mate_canvas_bpath_class_init (MateCanvasBpathClass *class)
{
	GObjectClass         *gobject_class;
	GtkObjectClass       *object_class;
	MateCanvasItemClass *item_class;

	gobject_class = (GObjectClass *) class;
	object_class = (GtkObjectClass *) class;
	item_class = (MateCanvasItemClass *) class;

	parent_class = g_type_class_peek_parent (class);

	/* when this gets checked into libmateui, change the
           GTK_TYPE_POINTER to GTK_TYPE_MATE_CANVAS_BPATH, and add an
           entry to mate-boxed.defs */

	gobject_class->set_property = mate_canvas_bpath_set_property;
	gobject_class->get_property = mate_canvas_bpath_get_property;

	object_class->destroy = mate_canvas_bpath_destroy;

	g_object_class_install_property (gobject_class,
                                         PROP_BPATH,
                                         g_param_spec_boxed ("bpath", NULL, NULL,
                                                             MATE_TYPE_CANVAS_PATH_DEF,
                                                             (G_PARAM_READABLE | G_PARAM_WRITABLE)));

	item_class->update = mate_canvas_bpath_update;
}

static void
mate_canvas_bpath_init (MateCanvasBpath *bpath)
{

}

static void
mate_canvas_bpath_destroy (GtkObject *object)
{
	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
mate_canvas_bpath_set_property (GObject      *object,
                                 guint         param_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
	MateCanvasItem         *item;
	MateCanvasPathDef      *gpp;

	item = MATE_CANVAS_ITEM (object);

	switch (param_id) {
	case PROP_BPATH:
		gpp = (MateCanvasPathDef*) g_value_get_boxed (value);

		mate_canvas_shape_set_path_def (MATE_CANVAS_SHAPE (object), gpp);

		mate_canvas_item_request_update (item);
		break;

	default:
		break;
	}
}


static void
mate_canvas_bpath_get_property (GObject     *object,
                                 guint        param_id,
                                 GValue      *value,
                                 GParamSpec  *pspec)
{
	MateCanvasShape        *shape;

	shape = MATE_CANVAS_SHAPE(object);

	switch (param_id) {
	case PROP_BPATH:
		g_value_set_boxed (value, shape->priv->path);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
mate_canvas_bpath_update (MateCanvasItem *item, double *affine, ArtSVP *clip_path, int flags)
{
	if(MATE_CANVAS_ITEM_CLASS(parent_class)->update) {
		(* MATE_CANVAS_ITEM_CLASS(parent_class)->update)(item, affine, clip_path, flags);
	}
}
