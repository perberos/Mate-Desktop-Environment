/* -*- Mode: C; tab-width: 8; indent-tabs-mode: 8; c-basic-offset: 8 -*- */
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
/* MateCanvas widget - Tk-like canvas widget for Mate
 *
 * MateCanvas is basically a port of the Tk toolkit's most excellent canvas
 * widget.  Tk is copyrighted by the Regents of the University of California,
 * Sun Microsystems, and other parties.
 *
 *
 * Authors: Federico Mena <federico@nuclecu.unam.mx>
 *          Raph Levien <raph@gimp.org>
 */

#ifndef MATE_CANVAS_H
#define MATE_CANVAS_H

#include <gtk/gtk.h>
#include <stdarg.h>
#include <libart_lgpl/art_misc.h>
#include <libart_lgpl/art_rect.h>
#include <libart_lgpl/art_svp.h>
#include <libart_lgpl/art_uta.h>
#include <libart_lgpl/art_affine.h>

#ifdef __cplusplus
extern "C" {
#endif


/* "Small" value used by canvas stuff */
#define MATE_CANVAS_EPSILON 1e-10


/* Macros for building colors that fit in a 32-bit integer.  The values are in
 * [0, 255].
 */

#define MATE_CANVAS_COLOR(r, g, b) ((((unsigned int) (r) & 0xff) << 24)	\
				     | (((unsigned int) (g) & 0xff) << 16)	\
				     | (((unsigned int) (b) & 0xff) << 8)	\
				     | 0xff)

#define MATE_CANVAS_COLOR_A(r, g, b, a) ((((unsigned int) (r) & 0xff) << 24)	\
					  | (((unsigned int) (g) & 0xff) << 16)	\
					  | (((unsigned int) (b) & 0xff) << 8)	\
					  | ((unsigned int) (a) & 0xff))


typedef struct _MateCanvas           MateCanvas;
typedef struct _MateCanvasClass      MateCanvasClass;
typedef struct _MateCanvasItem       MateCanvasItem;
typedef struct _MateCanvasItemClass  MateCanvasItemClass;
typedef struct _MateCanvasGroup      MateCanvasGroup;
typedef struct _MateCanvasGroupClass MateCanvasGroupClass;


/* MateCanvasItem - base item class for canvas items
 *
 * All canvas items are derived from MateCanvasItem.  The only information a
 * MateCanvasItem contains is its parent canvas, its parent canvas item group,
 * its bounding box in world coordinates, and its current affine transformation.
 *
 * Items inside a canvas are organized in a tree of MateCanvasItemGroup nodes
 * and MateCanvasItem leaves.  Each canvas has a single root group, which can
 * be obtained with the mate_canvas_get_root() function.
 *
 * The abstract MateCanvasItem class does not have any configurable or
 * queryable attributes.
 */

/* Object flags for items */
enum {
	MATE_CANVAS_ITEM_REALIZED      = 1 << 4,
	MATE_CANVAS_ITEM_MAPPED        = 1 << 5,
	MATE_CANVAS_ITEM_ALWAYS_REDRAW = 1 << 6,
	MATE_CANVAS_ITEM_VISIBLE       = 1 << 7,
	MATE_CANVAS_ITEM_NEED_UPDATE	= 1 << 8,
	MATE_CANVAS_ITEM_NEED_AFFINE	= 1 << 9,
	MATE_CANVAS_ITEM_NEED_CLIP	= 1 << 10,
	MATE_CANVAS_ITEM_NEED_VIS	= 1 << 11,
	MATE_CANVAS_ITEM_AFFINE_FULL	= 1 << 12
};

/* Update flags for items */
enum {
	MATE_CANVAS_UPDATE_REQUESTED  = 1 << 0,
	MATE_CANVAS_UPDATE_AFFINE     = 1 << 1,
	MATE_CANVAS_UPDATE_CLIP       = 1 << 2,
	MATE_CANVAS_UPDATE_VISIBILITY = 1 << 3,
	MATE_CANVAS_UPDATE_IS_VISIBLE = 1 << 4		/* Deprecated.  FIXME: remove this */
};

/* Data for rendering in antialiased mode */
typedef struct {
	/* 24-bit RGB buffer for rendering */
	guchar *buf;

	/* Rectangle describing the rendering area */
	ArtIRect rect;

	/* Rowstride for the buffer */
	int buf_rowstride;

	/* Background color, given as 0xrrggbb */
	guint32 bg_color;

	/* Invariant: at least one of the following flags is true. */

	/* Set when the render rectangle area is the solid color bg_color */
	unsigned int is_bg : 1;

	/* Set when the render rectangle area is represented by the buf */
	unsigned int is_buf : 1;
} MateCanvasBuf;


#define MATE_TYPE_CANVAS_ITEM            (mate_canvas_item_get_type ())
#define MATE_CANVAS_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_CANVAS_ITEM, MateCanvasItem))
#define MATE_CANVAS_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_CANVAS_ITEM, MateCanvasItemClass))
#define MATE_IS_CANVAS_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_CANVAS_ITEM))
#define MATE_IS_CANVAS_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_CANVAS_ITEM))
#define MATE_CANVAS_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATE_TYPE_CANVAS_ITEM, MateCanvasItemClass))


struct _MateCanvasItem {
	GtkObject object;

	/* Parent canvas for this item */
	MateCanvas *canvas;

	/* Parent canvas group for this item (a MateCanvasGroup) */
	MateCanvasItem *parent;

	/* If NULL, assumed to be the identity tranform.  If flags does not have
	 * AFFINE_FULL, then a two-element array containing a translation.  If
	 * flags contains AFFINE_FULL, a six-element array containing an affine
	 * transformation.
	 */
	double *xform;

	/* Bounding box for this item (in canvas coordinates) */
	double x1, y1, x2, y2;
};

struct _MateCanvasItemClass {
	GtkObjectClass parent_class;

	/* Tell the item to update itself.  The flags are from the update flags
	 * defined above.  The item should update its internal state from its
	 * queued state, and recompute and request its repaint area.  The
	 * affine, if used, is a pointer to a 6-element array of doubles.  The
	 * update method also recomputes the bounding box of the item.
	 */
	void (* update) (MateCanvasItem *item, double *affine, ArtSVP *clip_path, int flags);

	/* Realize an item -- create GCs, etc. */
	void (* realize) (MateCanvasItem *item);

	/* Unrealize an item */
	void (* unrealize) (MateCanvasItem *item);

	/* Map an item - normally only need by items with their own GdkWindows */
	void (* map) (MateCanvasItem *item);

	/* Unmap an item */
	void (* unmap) (MateCanvasItem *item);

	/* Return the microtile coverage of the item */
	ArtUta *(* coverage) (MateCanvasItem *item);

	/* Draw an item of this type.  (x, y) are the upper-left canvas pixel
	 * coordinates of the drawable, a temporary pixmap, where things get
	 * drawn.  (width, height) are the dimensions of the drawable.
	 */
	void (* draw) (MateCanvasItem *item, GdkDrawable *drawable,
		       int x, int y, int width, int height);

	/* Render the item over the buffer given.  The buf data structure
	 * contains both a pointer to a packed 24-bit RGB array, and the
	 * coordinates.  This method is only used for antialiased canvases.
	 *
	 * TODO: figure out where clip paths fit into the rendering framework.
	 */
	void (* render) (MateCanvasItem *item, MateCanvasBuf *buf);

	/* Calculate the distance from an item to the specified point.  It also
         * returns a canvas item which is the item itself in the case of the
         * object being an actual leaf item, or a child in case of the object
         * being a canvas group.  (cx, cy) are the canvas pixel coordinates that
         * correspond to the item-relative coordinates (x, y).
	 */
	double (* point) (MateCanvasItem *item, double x, double y, int cx, int cy,
			  MateCanvasItem **actual_item);

	/* Fetch the item's bounding box (need not be exactly tight).  This
	 * should be in item-relative coordinates.
	 */
	void (* bounds) (MateCanvasItem *item, double *x1, double *y1, double *x2, double *y2);

	/* Signal: an event occurred for an item of this type.  The (x, y)
	 * coordinates are in the canvas world coordinate system.
	 */
	gboolean (* event)                (MateCanvasItem *item, GdkEvent *event);

	/* Reserved for future expansion */
	gpointer spare_vmethods [4];
};


GType mate_canvas_item_get_type (void) G_GNUC_CONST;

/* Create a canvas item using the standard Gtk argument mechanism.  The item is
 * automatically inserted at the top of the specified canvas group.  The last
 * argument must be a NULL pointer.
 */
MateCanvasItem *mate_canvas_item_new (MateCanvasGroup *parent, GType type,
					const gchar *first_arg_name, ...);

/* Constructors for use in derived classes and language wrappers */
void mate_canvas_item_construct (MateCanvasItem *item, MateCanvasGroup *parent,
				  const gchar *first_arg_name, va_list args);

/* Configure an item using the standard Gtk argument mechanism.  The last
 * argument must be a NULL pointer.
 */
void mate_canvas_item_set (MateCanvasItem *item, const gchar *first_arg_name, ...);

/* Used only for language wrappers and the like */
void mate_canvas_item_set_valist (MateCanvasItem *item,
				   const gchar *first_arg_name, va_list args);

/* Move an item by the specified amount */
void mate_canvas_item_move (MateCanvasItem *item, double dx, double dy);

/* Apply a relative affine transformation to the item. */
void mate_canvas_item_affine_relative (MateCanvasItem *item, const double affine[6]);

/* Apply an absolute affine transformation to the item. */
void mate_canvas_item_affine_absolute (MateCanvasItem *item, const double affine[6]);

/* Raise an item in the z-order of its parent group by the specified number of
 * positions.
 */
void mate_canvas_item_raise (MateCanvasItem *item, int positions);

/* Lower an item in the z-order of its parent group by the specified number of
 * positions.
 */
void mate_canvas_item_lower (MateCanvasItem *item, int positions);

/* Raise an item to the top of its parent group's z-order. */
void mate_canvas_item_raise_to_top (MateCanvasItem *item);

/* Lower an item to the bottom of its parent group's z-order */
void mate_canvas_item_lower_to_bottom (MateCanvasItem *item);

/* Show an item (make it visible).  If the item is already shown, it has no
 * effect.
 */
void mate_canvas_item_show (MateCanvasItem *item);

/* Hide an item (make it invisible).  If the item is already invisible, it has
 * no effect.
 */
void mate_canvas_item_hide (MateCanvasItem *item);

/* Grab the mouse for the specified item.  Only the events in event_mask will be
 * reported.  If cursor is non-NULL, it will be used during the duration of the
 * grab.  Time is a proper X event time parameter.  Returns the same values as
 * XGrabPointer().
 */
int mate_canvas_item_grab (MateCanvasItem *item, unsigned int event_mask,
			    GdkCursor *cursor, guint32 etime);

/* Ungrabs the mouse -- the specified item must be the same that was passed to
 * mate_canvas_item_grab().  Time is a proper X event time parameter.
 */
void mate_canvas_item_ungrab (MateCanvasItem *item, guint32 etime);

/* These functions convert from a coordinate system to another.  "w" is world
 * coordinates and "i" is item coordinates.
 */
void mate_canvas_item_w2i (MateCanvasItem *item, double *x, double *y);
void mate_canvas_item_i2w (MateCanvasItem *item, double *x, double *y);

/* Gets the affine transform that converts from item-relative coordinates to
 * world coordinates.
 */
void mate_canvas_item_i2w_affine (MateCanvasItem *item, double affine[6]);

/* Gets the affine transform that converts from item-relative coordinates to
 * canvas pixel coordinates.
 */
void mate_canvas_item_i2c_affine (MateCanvasItem *item, double affine[6]);

/* Remove the item from its parent group and make the new group its parent.  The
 * item will be put on top of all the items in the new group.  The item's
 * coordinates relative to its new parent to *not* change -- this means that the
 * item could potentially move on the screen.
 *
 * The item and the group must be in the same canvas.  An item cannot be
 * reparented to a group that is the item itself or that is an inferior of the
 * item.
 */
void mate_canvas_item_reparent (MateCanvasItem *item, MateCanvasGroup *new_group);

/* Used to send all of the keystroke events to a specific item as well as
 * GDK_FOCUS_CHANGE events.
 */
void mate_canvas_item_grab_focus (MateCanvasItem *item);

/* Fetch the bounding box of the item.  The bounding box may not be exactly
 * tight, but the canvas items will do the best they can.  The returned bounding
 * box is in the coordinate system of the item's parent.
 */
void mate_canvas_item_get_bounds (MateCanvasItem *item,
				   double *x1, double *y1, double *x2, double *y2);

/* Request that the update method eventually get called.  This should be used
 * only by item implementations.
 */
void mate_canvas_item_request_update (MateCanvasItem *item);


/* MateCanvasGroup - a group of canvas items
 *
 * A group is a node in the hierarchical tree of groups/items inside a canvas.
 * Groups serve to give a logical structure to the items.
 *
 * Consider a circuit editor application that uses the canvas for its schematic
 * display.  Hierarchically, there would be canvas groups that contain all the
 * components needed for an "adder", for example -- this includes some logic
 * gates as well as wires.  You can move stuff around in a convenient way by
 * doing a mate_canvas_item_move() of the hierarchical groups -- to move an
 * adder, simply move the group that represents the adder.
 *
 * The following arguments are available:
 *
 * name		type		read/write	description
 * --------------------------------------------------------------------------------
 * x		double		RW		X coordinate of group's origin
 * y		double		RW		Y coordinate of group's origin
 */


#define MATE_TYPE_CANVAS_GROUP            (mate_canvas_group_get_type ())
#define MATE_CANVAS_GROUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_CANVAS_GROUP, MateCanvasGroup))
#define MATE_CANVAS_GROUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_CANVAS_GROUP, MateCanvasGroupClass))
#define MATE_IS_CANVAS_GROUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_CANVAS_GROUP))
#define MATE_IS_CANVAS_GROUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_CANVAS_GROUP))
#define MATE_CANVAS_GROUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATE_TYPE_CANVAS_GROUP, MateCanvasGroupClass))


struct _MateCanvasGroup {
	MateCanvasItem item;

	/* Children of the group */
	GList *item_list;
	GList *item_list_end;
};

struct _MateCanvasGroupClass {
	MateCanvasItemClass parent_class;
};


GType mate_canvas_group_get_type (void) G_GNUC_CONST;


/*** MateCanvas ***/


#define MATE_TYPE_CANVAS            (mate_canvas_get_type ())
#define MATE_CANVAS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_CANVAS, MateCanvas))
#define MATE_CANVAS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_CANVAS, MateCanvasClass))
#define MATE_IS_CANVAS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_CANVAS))
#define MATE_IS_CANVAS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_CANVAS))
#define MATE_CANVAS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATE_TYPE_CANVAS, MateCanvasClass))


struct _MateCanvas {
	GtkLayout layout;

	/* Root canvas group */
	MateCanvasItem *root;

	/* Area that needs redrawing, stored as a microtile array */
	ArtUta *redraw_area;

	/* The item containing the mouse pointer, or NULL if none */
	MateCanvasItem *current_item;

	/* Item that is about to become current (used to track deletions and such) */
	MateCanvasItem *new_current_item;

	/* Item that holds a pointer grab, or NULL if none */
	MateCanvasItem *grabbed_item;

	/* If non-NULL, the currently focused item */
	MateCanvasItem *focused_item;

	/* GC for temporary draw pixmap */
	GdkGC *pixmap_gc;

	/* Event on which selection of current item is based */
	GdkEvent pick_event;

	/* Scrolling region */
	double scroll_x1, scroll_y1;
	double scroll_x2, scroll_y2;

	/* Scaling factor to be used for display */
	double pixels_per_unit;

	/* Idle handler ID */
	guint idle_id;

	/* Signal handler ID for destruction of the root item */
	guint root_destroy_id;

	/* Area that is being redrawn.  Contains (x1, y1) but not (x2, y2).
	 * Specified in canvas pixel coordinates.
	 */
	int redraw_x1, redraw_y1;
	int redraw_x2, redraw_y2;

	/* Offsets of the temprary drawing pixmap */
	int draw_xofs, draw_yofs;

	/* Internal pixel offsets when zoomed out */
	int zoom_xofs, zoom_yofs;

	/* Last known modifier state, for deferred repick when a button is down */
	int state;

	/* Event mask specified when grabbing an item */
	guint grabbed_event_mask;

	/* Tolerance distance for picking items */
	int close_enough;

	/* Whether the canvas should center the scroll region in the middle of
	 * the window if the scroll region is smaller than the window.
	 */
	unsigned int center_scroll_region : 1;

	/* Whether items need update at next idle loop iteration */
	unsigned int need_update : 1;

	/* Whether the canvas needs redrawing at the next idle loop iteration */
	unsigned int need_redraw : 1;

	/* Whether current item will be repicked at next idle loop iteration */
	unsigned int need_repick : 1;

	/* For use by internal pick_current_item() function */
	unsigned int left_grabbed_item : 1;

	/* For use by internal pick_current_item() function */
	unsigned int in_repick : 1;

	/* Whether the canvas is in antialiased mode or not */
	unsigned int aa : 1;

	/* Which dither mode to use for antialiased mode drawing */
	GdkRgbDither dither;
};

struct _MateCanvasClass {
	GtkLayoutClass parent_class;

	/* Draw the background for the area given. This method is only used
	 * for non-antialiased canvases.
	 */
	void (* draw_background) (MateCanvas *canvas, GdkDrawable *drawable,
				  int x, int y, int width, int height);

	/* Render the background for the buffer given. The buf data structure
	 * contains both a pointer to a packed 24-bit RGB array, and the
	 * coordinates. This method is only used for antialiased canvases.
	 */
	void (* render_background) (MateCanvas *canvas, MateCanvasBuf *buf);

	/* Private Virtual methods for groping the canvas inside matecomponent */
	void (* request_update) (MateCanvas *canvas);

	/* Reserved for future expansion */
	gpointer spare_vmethods [4];
};


GType mate_canvas_get_type (void) G_GNUC_CONST;

/* Creates a new canvas.  You should check that the canvas is created with the
 * proper visual and colormap.  Any visual will do unless you intend to insert
 * gdk_imlib images into it, in which case you should use the gdk_imlib visual.
 *
 * You should call mate_canvas_set_scroll_region() soon after calling this
 * function to set the desired scrolling limits for the canvas.
 */
GtkWidget *mate_canvas_new (void);

/* Creates a new antialiased empty canvas.  You should push the GdkRgb colormap
 * and visual for this.
 */
#ifndef MATE_EXCLUDE_EXPERIMENTAL
GtkWidget *mate_canvas_new_aa (void);
#endif

/* Returns the root canvas item group of the canvas */
MateCanvasGroup *mate_canvas_root (MateCanvas *canvas);

/* Sets the limits of the scrolling region, in world coordinates */
void mate_canvas_set_scroll_region (MateCanvas *canvas,
				     double x1, double y1, double x2, double y2);

/* Gets the limits of the scrolling region, in world coordinates */
void mate_canvas_get_scroll_region (MateCanvas *canvas,
				     double *x1, double *y1, double *x2, double *y2);

/* Whether the canvas centers the scroll region if it is smaller than the window */
void mate_canvas_set_center_scroll_region (MateCanvas *canvas, gboolean center_scroll_region);

/* Returns whether the canvas is set to center the scroll region if it is smaller than the window */
gboolean mate_canvas_get_center_scroll_region (MateCanvas *canvas);

/* Sets the number of pixels that correspond to one unit in world coordinates */
void mate_canvas_set_pixels_per_unit (MateCanvas *canvas, double n);

/* Scrolls the canvas to the specified offsets, given in canvas pixel coordinates */
void mate_canvas_scroll_to (MateCanvas *canvas, int cx, int cy);

/* Returns the scroll offsets of the canvas in canvas pixel coordinates.  You
 * can specify NULL for any of the values, in which case that value will not be
 * queried.
 */
void mate_canvas_get_scroll_offsets (MateCanvas *canvas, int *cx, int *cy);

/* Requests that the canvas be repainted immediately instead of in the idle
 * loop.
 */
void mate_canvas_update_now (MateCanvas *canvas);

/* Returns the item that is at the specified position in world coordinates, or
 * NULL if no item is there.
 */
MateCanvasItem *mate_canvas_get_item_at (MateCanvas *canvas, double x, double y);

/* For use only by item type implementations. Request that the canvas eventually
 * redraw the specified region. The region is specified as a microtile
 * array. This function takes over responsibility for freeing the uta argument.
 */
void mate_canvas_request_redraw_uta (MateCanvas *canvas, ArtUta *uta);

/* For use only by item type implementations.  Request that the canvas
 * eventually redraw the specified region, specified in canvas pixel
 * coordinates.  The region contains (x1, y1) but not (x2, y2).
 */
void mate_canvas_request_redraw (MateCanvas *canvas, int x1, int y1, int x2, int y2);

/* Gets the affine transform that converts world coordinates into canvas pixel
 * coordinates.
 */
void mate_canvas_w2c_affine (MateCanvas *canvas, double affine[6]);

/* These functions convert from a coordinate system to another.  "w" is world
 * coordinates, "c" is canvas pixel coordinates (pixel coordinates that are
 * (0,0) for the upper-left scrolling limit and something else for the
 * lower-left scrolling limit).
 */
void mate_canvas_w2c (MateCanvas *canvas, double wx, double wy, int *cx, int *cy);
void mate_canvas_w2c_d (MateCanvas *canvas, double wx, double wy, double *cx, double *cy);
void mate_canvas_c2w (MateCanvas *canvas, int cx, int cy, double *wx, double *wy);

/* This function takes in coordinates relative to the GTK_LAYOUT
 * (canvas)->bin_window and converts them to world coordinates.
 */
void mate_canvas_window_to_world (MateCanvas *canvas,
				   double winx, double winy, double *worldx, double *worldy);

/* This is the inverse of mate_canvas_window_to_world() */
void mate_canvas_world_to_window (MateCanvas *canvas,
				   double worldx, double worldy, double *winx, double *winy);

/* Takes a string specification for a color and allocates it into the specified
 * GdkColor.  If the string is null, then it returns FALSE. Otherwise, it
 * returns TRUE.
 */
int mate_canvas_get_color (MateCanvas *canvas, const char *spec, GdkColor *color);

/* Allocates a color from the RGB value passed into this function. */
gulong mate_canvas_get_color_pixel (MateCanvas *canvas,
				     guint        rgba);


/* Sets the stipple origin of the specified gc so that it will be aligned with
 * all the stipples used in the specified canvas.  This is intended for use only
 * by canvas item implementations.
 */
void mate_canvas_set_stipple_origin (MateCanvas *canvas, GdkGC *gc);

/* Controls the dithering used when the canvas renders.
 * Only applicable to antialiased canvases - ignored by non-antialiased canvases.
 */
void mate_canvas_set_dither (MateCanvas *canvas, GdkRgbDither dither);

/* Returns the dither mode of an antialiased canvas.
 * Only applicable to antialiased canvases - ignored by non-antialiased canvases.
 */
GdkRgbDither mate_canvas_get_dither (MateCanvas *canvas);

#ifdef __cplusplus
}
#endif

#endif
