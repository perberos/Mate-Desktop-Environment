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
/* Rectangle and ellipse item types for MateCanvas widget
 *
 * MateCanvas is basically a port of the Tk toolkit's most excellent canvas widget.  Tk is
 * copyrighted by the Regents of the University of California, Sun Microsystems, and other parties.
 *
 *
 * Authors: Federico Mena <federico@nuclecu.unam.mx>
 *          Rusty Conover <rconover@bangtail.net>
 */

#include <config.h>
#include <math.h>
#include "mate-canvas-rect-ellipse.h"
#include "mate-canvas-util.h"
#include "mate-canvas-shape.h"


#include "libart_lgpl/art_vpath.h"
#include "libart_lgpl/art_svp.h"
#include "libart_lgpl/art_svp_vpath.h"
#include "libart_lgpl/art_rgb_svp.h"

/* Base class for rectangle and ellipse item types */

#define noVERBOSE

enum {
	PROP_0,
	PROP_X1,
	PROP_Y1,
	PROP_X2,
	PROP_Y2
};


static void mate_canvas_re_class_init (MateCanvasREClass *class);
static void mate_canvas_re_init       (MateCanvasRE      *re);
static void mate_canvas_re_destroy    (GtkObject          *object);
static void mate_canvas_re_set_property (GObject              *object,
					  guint                 param_id,
					  const GValue         *value,
					  GParamSpec           *pspec);
static void mate_canvas_re_get_property (GObject              *object,
					  guint                 param_id,
					  GValue               *value,
					  GParamSpec           *pspec);

static void mate_canvas_rect_update      (MateCanvasItem *item, double *affine, ArtSVP *clip_path, int flags);
static void mate_canvas_ellipse_update      (MateCanvasItem *item, double *affine, ArtSVP *clip_path, int flags);

static MateCanvasItemClass *re_parent_class;


GType
mate_canvas_re_get_type (void)
{
	static GType re_type;

	if (!re_type) {
		const GTypeInfo object_info = {
			sizeof (MateCanvasREClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) mate_canvas_re_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,			/* class_data */
			sizeof (MateCanvasRE),
			0,			/* n_preallocs */
			(GInstanceInitFunc) mate_canvas_re_init,
			NULL			/* value_table */
		};

		re_type = g_type_register_static (MATE_TYPE_CANVAS_SHAPE, "MateCanvasRE",
						  &object_info, 0);
	}

	return re_type;
}

static void
mate_canvas_re_class_init (MateCanvasREClass *class)
{
	GObjectClass *gobject_class;
	GtkObjectClass *object_class;

	gobject_class = (GObjectClass *) class;
	object_class = (GtkObjectClass *) class;

	re_parent_class = g_type_class_peek_parent (class);

	gobject_class->set_property = mate_canvas_re_set_property;
	gobject_class->get_property = mate_canvas_re_get_property;

        g_object_class_install_property
                (gobject_class,
                 PROP_X1,
                 g_param_spec_double ("x1", NULL, NULL,
				      -G_MAXDOUBLE, G_MAXDOUBLE, 0,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_Y1,
                 g_param_spec_double ("y1", NULL, NULL,
				      -G_MAXDOUBLE, G_MAXDOUBLE, 0,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_X2,
                 g_param_spec_double ("x2", NULL, NULL,
				      -G_MAXDOUBLE, G_MAXDOUBLE, 0,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (gobject_class,
                 PROP_Y2,
                 g_param_spec_double ("y2", NULL, NULL,
				      -G_MAXDOUBLE, G_MAXDOUBLE, 0,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE)));

	object_class->destroy = mate_canvas_re_destroy;
}

static void
mate_canvas_re_init (MateCanvasRE *re)
{
	re->x1 = 0.0;
	re->y1 = 0.0;
	re->x2 = 0.0;
	re->y2 = 0.0;
	re->path_dirty = 0;
}

static void
mate_canvas_re_destroy (GtkObject *object)
{
	g_return_if_fail (object != NULL);
	g_return_if_fail (MATE_IS_CANVAS_RE (object));

	if (GTK_OBJECT_CLASS (re_parent_class)->destroy)
		(* GTK_OBJECT_CLASS (re_parent_class)->destroy) (object);
}

static void
mate_canvas_re_set_property (GObject              *object,
			      guint                 param_id,
			      const GValue         *value,
			      GParamSpec           *pspec)
{
	MateCanvasItem *item;
	MateCanvasRE *re;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MATE_IS_CANVAS_RE (object));

	item = MATE_CANVAS_ITEM (object);
	re = MATE_CANVAS_RE (object);

	switch (param_id) {
	case PROP_X1:
		re->x1 = g_value_get_double (value);
		re->path_dirty = 1;
		mate_canvas_item_request_update (item);
		break;

	case PROP_Y1:
		re->y1 = g_value_get_double (value);
		re->path_dirty = 1;
		mate_canvas_item_request_update (item);
		break;

	case PROP_X2:
		re->x2 = g_value_get_double (value);
		re->path_dirty = 1;
		mate_canvas_item_request_update (item);
		break;

	case PROP_Y2:
		re->y2 = g_value_get_double (value);
		re->path_dirty = 1;
		mate_canvas_item_request_update (item);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
mate_canvas_re_get_property (GObject              *object,
			      guint                 param_id,
			      GValue               *value,
			      GParamSpec           *pspec)
{
	MateCanvasRE *re;

	g_return_if_fail (object != NULL);
	g_return_if_fail (MATE_IS_CANVAS_RE (object));

	re = MATE_CANVAS_RE (object);

	switch (param_id) {
	case PROP_X1:
		g_value_set_double (value,  re->x1);
		break;

	case PROP_Y1:
		g_value_set_double (value,  re->y1);
		break;

	case PROP_X2:
		g_value_set_double (value,  re->x2);
		break;

	case PROP_Y2:
		g_value_set_double (value,  re->y2);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

/* Rectangle item */
static void mate_canvas_rect_class_init (MateCanvasRectClass *class);



GType
mate_canvas_rect_get_type (void)
{
	static GType rect_type;

	if (!rect_type) {
		const GTypeInfo object_info = {
			sizeof (MateCanvasRectClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) mate_canvas_rect_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,			/* class_data */
			sizeof (MateCanvasRect),
			0,			/* n_preallocs */
			(GInstanceInitFunc) NULL,
			NULL			/* value_table */
		};

		rect_type = g_type_register_static (MATE_TYPE_CANVAS_RE, "MateCanvasRect",
						    &object_info, 0);
	}

	return rect_type;
}

static void
mate_canvas_rect_class_init (MateCanvasRectClass *class)
{
	MateCanvasItemClass *item_class;

	item_class = (MateCanvasItemClass *) class;

	item_class->update = mate_canvas_rect_update;
}

static void
mate_canvas_rect_update (MateCanvasItem *item, double affine[6], ArtSVP *clip_path, gint flags)
{	MateCanvasRE *re;	

	MateCanvasPathDef *path_def;

	re = MATE_CANVAS_RE(item);

	if (re->path_dirty) {		
		path_def = mate_canvas_path_def_new ();
		
		mate_canvas_path_def_moveto(path_def, re->x1, re->y1);
		mate_canvas_path_def_lineto(path_def, re->x2, re->y1);
		mate_canvas_path_def_lineto(path_def, re->x2, re->y2);
		mate_canvas_path_def_lineto(path_def, re->x1, re->y2);
		mate_canvas_path_def_lineto(path_def, re->x1, re->y1);		
		mate_canvas_path_def_closepath_current(path_def);		
		mate_canvas_shape_set_path_def (MATE_CANVAS_SHAPE (item), path_def);
		mate_canvas_path_def_unref(path_def);
		re->path_dirty = 0;
	}

	if (re_parent_class->update)
		(* re_parent_class->update) (item, affine, clip_path, flags);
}

/* Ellipse item */


static void mate_canvas_ellipse_class_init (MateCanvasEllipseClass *class);


GType
mate_canvas_ellipse_get_type (void)
{
	static GType ellipse_type;

	if (!ellipse_type) {
		const GTypeInfo object_info = {
			sizeof (MateCanvasEllipseClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) mate_canvas_ellipse_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,			/* class_data */
			sizeof (MateCanvasEllipse),
			0,			/* n_preallocs */
			(GInstanceInitFunc) NULL,
			NULL			/* value_table */
		};

		ellipse_type = g_type_register_static (MATE_TYPE_CANVAS_RE, "MateCanvasEllipse",
						       &object_info, 0);
	}

	return ellipse_type;
}

static void
mate_canvas_ellipse_class_init (MateCanvasEllipseClass *class)
{
	MateCanvasItemClass *item_class;

	item_class = (MateCanvasItemClass *) class;

	item_class->update = mate_canvas_ellipse_update;
}

#define N_PTS 90

static void
mate_canvas_ellipse_update (MateCanvasItem *item, double affine[6], ArtSVP *clip_path, gint flags) {
	MateCanvasPathDef *path_def;
	MateCanvasRE *re;

	re = MATE_CANVAS_RE(item);

	if (re->path_dirty) {
		gdouble cx, cy, rx, ry;
		gdouble beta = 0.26521648983954400922; /* 4*(1-cos(pi/8))/(3*sin(pi/8)) */
		gdouble sincosA = 0.70710678118654752440; /* sin (pi/4), cos (pi/4) */
		gdouble dx1, dy1, dx2, dy2;
		gdouble mx, my;

		path_def = mate_canvas_path_def_new();

		cx = (re->x2 + re->x1) * 0.5;
		cy = (re->y2 + re->y1) * 0.5;
		rx = re->x2 - cx;
		ry = re->y2 - cy;

		dx1 = beta * rx;
		dy1 = beta * ry;
		dx2 = beta * rx * sincosA;
		dy2 = beta * ry * sincosA;
		mx = rx * sincosA;
		my = ry * sincosA;

		mate_canvas_path_def_moveto (path_def, cx + rx, cy);
		mate_canvas_path_def_curveto (path_def,
					       cx + rx, cy - dy1,
					       cx + mx + dx2, cy - my + dy2,
					       cx + mx, cy - my);
		mate_canvas_path_def_curveto (path_def,
					       cx + mx - dx2, cy - my - dy2,
					       cx + dx1, cy - ry,
					       cx, cy - ry);
		mate_canvas_path_def_curveto (path_def,
					       cx - dx1, cy - ry,
					       cx - mx + dx2, cy - my - dy2,
					       cx - mx, cy - my);
		mate_canvas_path_def_curveto (path_def,
					       cx - mx - dx2, cy - my + dy2,
					       cx - rx, cy - dy1,
					       cx - rx, cy);
		
		mate_canvas_path_def_curveto (path_def,
					       cx - rx, cy + dy1,
					       cx - mx - dx2, cy + my - dy2,
					       cx - mx, cy + my);
		mate_canvas_path_def_curveto (path_def,
					       cx - mx + dx2, cy + my + dy2,
					       cx - dx1, cy + ry,
					       cx, cy + ry);
		mate_canvas_path_def_curveto (path_def,
					       cx + dx1, cy + ry,
					       cx + mx - dx2, cy + my + dy2,
					       cx + mx, cy + my);
		mate_canvas_path_def_curveto (path_def,
					       cx + mx + dx2, cy + my - dy2,
					       cx + rx, cy + dy1,
					       cx + rx, cy);
		
		mate_canvas_path_def_closepath_current(path_def);
		
		mate_canvas_shape_set_path_def (MATE_CANVAS_SHAPE (item), path_def);
		mate_canvas_path_def_unref(path_def);
		re->path_dirty = 0;
	}

	if (re_parent_class->update)
		(* re_parent_class->update) (item, affine, clip_path, flags);
}
