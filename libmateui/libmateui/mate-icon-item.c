/*
 * Copyright (C) 1998, 1999, 2000, 2001 Free Software Foundation
 * Copyright (C) 2001 Anders Carlsson
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
 *
 * Author: Anders Carlsson <andersca@gnu.org>
 *
 * Based on the MATE 1.0 icon item by Miguel de Icaza and Federico Mena.
 */

#include <config.h>
#include <math.h>
#include <libmate/mate-macros.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk/gdk.h>
#include <libart_lgpl/art_rgb_affine.h>
#include <libart_lgpl/art_rgb_rgba_affine.h>
#include <string.h>

/* Must be before all other mate includes!! */
#include <glib/gi18n-lib.h>

#include "mate-icon-item.h"
#include "mate-marshal.h"

/* Margins used to display the information */
#define MARGIN_X 2
#define MARGIN_Y 2

#define ROUND(n) (floor ((n) + .5))

/* Private part of the MateIconTextItem structure */
struct _MateIconTextItemPrivate {

	/* Our layout */
	PangoLayout *layout;
	int layout_width, layout_height;
	GtkWidget *entry_top;
	GtkWidget *entry;

	guint selection_start;
	guint min_width;
	guint min_height;

	GdkGC *cursor_gc;

	/* Whether the user pressed the mouse while the item was unselected */
	guint unselected_click : 1;

	/* Whether we need to update the position */
	guint need_pos_update : 1;
	/* Whether we need to update the font */
	guint need_font_update : 1;

	/* Whether we need to update the text */
	guint need_text_update : 1;

	/* Whether we need to update because the editing/selected state changed */
	guint need_state_update : 1;

	/* Whether selection is occuring */
	guint selecting         : 1;
};

enum {
	TEXT_CHANGED,
	HEIGHT_CHANGED,
	WIDTH_CHANGED,
	EDITING_STARTED,
	EDITING_STOPPED,
	SELECTION_STARTED,
	SELECTION_STOPPED,
	LAST_SIGNAL
};

static guint iti_signals [LAST_SIGNAL] = { 0 };

MATE_CLASS_BOILERPLATE (MateIconTextItem, mate_icon_text_item,
			 MateCanvasItem, MATE_TYPE_CANVAS_ITEM)

static void
send_focus_event (MateIconTextItem *iti, gboolean in)
{
	MateIconTextItemPrivate *priv;
	GtkWidget *widget;
	gboolean has_focus;
	GdkEvent fake_event;

	g_return_if_fail (in == FALSE || in == TRUE);

	priv = iti->_priv;
	if (priv->entry == NULL) {
		g_assert (!in);
		return;
	}

	widget = GTK_WIDGET (priv->entry);
	has_focus = GTK_WIDGET_HAS_FOCUS (widget);
	if (has_focus == in) {
		return;
	}

	memset (&fake_event, 0, sizeof (fake_event));
	fake_event.focus_change.type = GDK_FOCUS_CHANGE;
	fake_event.focus_change.window = widget->window;
	fake_event.focus_change.in = in;
	gtk_widget_event (widget, &fake_event);

	/* FIXME: this is failing */
#if 0
	g_return_if_fail (GTK_WIDGET_HAS_FOCUS (widget) == in);
#endif
}

/* Updates the pango layout width */
static void
update_pango_layout (MateIconTextItem *iti)
{
	MateIconTextItemPrivate *priv;
	PangoRectangle bounds;
	const char *text;

	priv = iti->_priv;

	if (iti->editing) {
		text = gtk_entry_get_text (GTK_ENTRY (priv->entry));
	} else {
		text = iti->text;
	}

	pango_layout_set_wrap (priv->layout, PANGO_WRAP_WORD_CHAR);
	pango_layout_set_text (priv->layout, text, strlen (text));
	pango_layout_set_width (priv->layout, iti->width * PANGO_SCALE);

	/* In PANGO_WRAP_WORD mode, words wider than a line of text make
	 * PangoLayout overflow the layout width.  If this happens, switch to
	 * character-based wrapping.
	 */

	pango_layout_get_pixel_extents (iti->_priv->layout, NULL, &bounds);

	priv->layout_width = bounds.width;
	priv->layout_height = bounds.height;
}

/* Stops the editing state of an icon text item */
static void
iti_stop_editing (MateIconTextItem *iti)
{
	iti->editing = FALSE;
	send_focus_event (iti, FALSE);
	update_pango_layout (iti);
	mate_canvas_item_request_update (MATE_CANVAS_ITEM (iti));
	g_signal_emit (iti, iti_signals[EDITING_STOPPED], 0);
}


/* Accepts the text in the off-screen entry of an icon text item */
static void
iti_edition_accept (MateIconTextItem *iti)
{
	MateIconTextItemPrivate *priv;
	gboolean accept;

	priv = iti->_priv;
	accept = TRUE;

	g_signal_emit (iti, iti_signals [TEXT_CHANGED], 0, &accept);

	if (iti->editing){
		if (accept) {
			if (iti->is_text_allocated)
				g_free (iti->text);

			iti->text = g_strdup (gtk_entry_get_text (GTK_ENTRY(priv->entry)));
			iti->is_text_allocated = 1;
		}

		iti_stop_editing (iti);
	}
	update_pango_layout (iti);
	priv->need_text_update = TRUE;

	mate_canvas_item_request_update (MATE_CANVAS_ITEM (iti));
}

/* Ensure the item gets focused (both globally, and local to Gtk) */
static void
iti_ensure_focus (MateCanvasItem *item)
{
	GtkWidget *toplevel;

        /* mate_canvas_item_grab_focus still generates focus out/in
         * events when focused_item == item
         */
        if (MATE_CANVAS_ITEM (item)->canvas->focused_item != item) {
        	mate_canvas_item_grab_focus (MATE_CANVAS_ITEM (item));
        }

	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (item->canvas));
	if (toplevel != NULL && GTK_WIDGET_REALIZED (toplevel)) {
		gdk_window_focus (toplevel->window, GDK_CURRENT_TIME);
	}
}

/* Starts the selection state in the icon text item */
static void
iti_start_selecting (MateIconTextItem *iti, int idx, guint32 event_time)
{
	MateIconTextItemPrivate *priv;
	GtkEditable *e;
	GdkCursor *ibeam;

	priv = iti->_priv;
	e = GTK_EDITABLE (priv->entry);

	gtk_editable_select_region (e, idx, idx);
	gtk_editable_set_position (e, idx);
	ibeam = gdk_cursor_new (GDK_XTERM);
	mate_canvas_item_grab (MATE_CANVAS_ITEM (iti),
				GDK_BUTTON_RELEASE_MASK |
				GDK_POINTER_MOTION_MASK,
				ibeam, event_time);
	gdk_cursor_unref (ibeam);

	gtk_editable_select_region (e, idx, idx);
	priv->selecting = TRUE;
	priv->selection_start = idx;

	mate_canvas_item_request_update (MATE_CANVAS_ITEM (iti));

	g_signal_emit (iti, iti_signals[SELECTION_STARTED], 0);
}

/* Stops the selection state in the icon text item */
static void
iti_stop_selecting (MateIconTextItem *iti, guint32 event_time)
{
	MateIconTextItemPrivate *priv;
	MateCanvasItem *item;

	priv = iti->_priv;
	item = MATE_CANVAS_ITEM (iti);

	mate_canvas_item_ungrab (item, event_time);
	priv->selecting = FALSE;

	mate_canvas_item_request_update (MATE_CANVAS_ITEM (iti));
	g_signal_emit (iti, iti_signals[SELECTION_STOPPED], 0);
}

/* Handles selection range changes on the icon text item */
static void
iti_selection_motion (MateIconTextItem *iti, int idx)
{
	MateIconTextItemPrivate *priv;
	GtkEditable *e;
	g_assert (idx >= 0);

	priv = iti->_priv;
	e = GTK_EDITABLE (priv->entry);

	if (idx < (int) priv->selection_start) {
		gtk_editable_select_region (e, idx, priv->selection_start);
	} else {
		gtk_editable_select_region (e, priv->selection_start, idx);
	}

	mate_canvas_item_request_update (MATE_CANVAS_ITEM (iti));
}

static void
iti_entry_text_changed_by_clipboard (GtkObject *widget, gpointer data)
{
	MateIconTextItem *iti;
	MateCanvasItem *item;

        /* Update text item to reflect changes */
        iti = MATE_ICON_TEXT_ITEM (data);

	update_pango_layout (iti);
	iti->_priv->need_text_update = TRUE;

        item = MATE_CANVAS_ITEM (iti);
        mate_canvas_item_request_update (item);
}


/* Position insertion point based on arrow key event */
static void
iti_handle_arrow_key_event (MateIconTextItem *iti, GdkEvent *event)
{
	/* FIXME: Implement this */
}

static int
get_layout_index (MateIconTextItem *iti, int x, int y)
{
	int index;
	int trailing;
	const char *cluster;
	const char *cluster_end;

	MateIconTextItemPrivate *priv;
	PangoRectangle extents;

	trailing = 0;
	index = 0;
	priv = iti->_priv;

	pango_layout_get_extents (priv->layout, NULL, &extents);

	x = (x * PANGO_SCALE) + extents.x;
	y = (y * PANGO_SCALE) + extents.y;

	pango_layout_xy_to_index (priv->layout, x, y, &index, &trailing);

	cluster = gtk_entry_get_text (GTK_ENTRY (priv->entry)) + index;
	cluster_end = cluster;
	while (trailing) {
		cluster_end = g_utf8_next_char (cluster_end);
		--trailing;
	}
	index += (cluster_end - cluster);

	return index;
}

static void
iti_entry_activate (GtkObject *widget, gpointer data)
{
	iti_edition_accept (MATE_ICON_TEXT_ITEM (data));
}

static void
realize_cursor_gc (MateIconTextItem *iti)
{
	GtkWidget *widget;
	GdkColor *cursor_color;

	widget = GTK_WIDGET (MATE_CANVAS_ITEM (iti)->canvas);

	if (iti->_priv->cursor_gc) {
		g_object_unref (iti->_priv->cursor_gc);
	}

	iti->_priv->cursor_gc = gdk_gc_new (widget->window);

	gtk_widget_style_get (GTK_WIDGET (iti->_priv->entry), "cursor_color",
			      &cursor_color, NULL);
	if (cursor_color) {
		gdk_gc_set_rgb_fg_color (iti->_priv->cursor_gc, cursor_color);
	} else {
		gdk_gc_set_rgb_fg_color (iti->_priv->cursor_gc,
					 &widget->style->black);
	}
}


static void
mate_icon_text_item_realize (MateCanvasItem *item)
{
	MateIconTextItem *iti = MATE_ICON_TEXT_ITEM (item);

	if (iti->_priv->entry) {
		realize_cursor_gc (iti);
	}

	MATE_CALL_PARENT (MATE_CANVAS_ITEM_CLASS, realize, (item));
}

static void
mate_icon_text_item_unrealize (MateCanvasItem *item)
{
	MateIconTextItem *iti = MATE_ICON_TEXT_ITEM (item);

	if (iti->_priv->cursor_gc) {
		g_object_unref (iti->_priv->cursor_gc);
		iti->_priv->cursor_gc = NULL;
	}

	MATE_CALL_PARENT (MATE_CANVAS_ITEM_CLASS, unrealize, (item));
}

/* Starts the editing state of an icon text item */
static void
iti_start_editing (MateIconTextItem *iti)
{
	MateIconTextItemPrivate *priv;

	priv = iti->_priv;

	if (iti->editing)
		return;

	/* We place an entry offscreen to handle events and selections.  */
	if (priv->entry_top == NULL) {
		priv->entry = gtk_entry_new ();
		g_signal_connect (priv->entry, "activate",
				  G_CALLBACK (iti_entry_activate), iti);


		if (GTK_WIDGET_REALIZED (GTK_WIDGET (MATE_CANVAS_ITEM (iti)->canvas))) {
			realize_cursor_gc (iti);
		}

		/* Make clipboard functions cause an update, since clipboard
		 * functions will change the offscreen entry */
		g_signal_connect_after (priv->entry, "changed",
					G_CALLBACK (iti_entry_text_changed_by_clipboard),
					iti);
		priv->entry_top = gtk_window_new (GTK_WINDOW_POPUP);
		gtk_container_add (GTK_CONTAINER (priv->entry_top),
				   GTK_WIDGET (priv->entry));
		gtk_window_move (GTK_WINDOW (priv->entry_top), 20000, 20000);
		gtk_widget_show_all (priv->entry_top);
	}

	gtk_entry_set_text (GTK_ENTRY (priv->entry), iti->text);
	gtk_editable_select_region (GTK_EDITABLE (priv->entry), 0, -1);

	iti->editing = TRUE;
	priv->need_state_update = TRUE;

	send_focus_event (iti, TRUE);
	mate_canvas_item_request_update (MATE_CANVAS_ITEM (iti));

	g_signal_emit (iti, iti_signals[EDITING_STARTED], 0);
}

/* Recomputes the bounding box of an icon text item */
static void
recompute_bounding_box (MateIconTextItem *iti)
{

	MateCanvasItem *item;
	double x1, y1, x2, y2;
	int width_c, height_c;
	int width_w, height_w;
	int max_width_w;

	item = MATE_CANVAS_ITEM (iti);

	width_c = iti->_priv->layout_width + 2 * MARGIN_X;
	height_c = iti->_priv->layout_height + 2 * MARGIN_Y;
	width_w = ROUND (width_c / item->canvas->pixels_per_unit);
	height_w = ROUND (height_c / item->canvas->pixels_per_unit);
	max_width_w = ROUND (iti->width / item->canvas->pixels_per_unit);

	x1 = iti->x;
	y1 = iti->y;

	mate_canvas_item_i2w (item, &x1, &y1);

	x1 += ROUND ((max_width_w - width_w) / 2);
	x2 = x1 + width_w;
	y2 = y1 + height_w;

	mate_canvas_w2c_d (item->canvas, x1, y1, &item->x1, &item->y1);
	mate_canvas_w2c_d (item->canvas, x2, y2, &item->x2, &item->y2);
}

static void
mate_icon_text_item_update (MateCanvasItem *item, double *affine, ArtSVP *clip_path, int flags)
{
	MateIconTextItem *iti;
	MateIconTextItemPrivate *priv;

	iti = MATE_ICON_TEXT_ITEM (item);
	priv = iti->_priv;

	MATE_CALL_PARENT (MATE_CANVAS_ITEM_CLASS, update, (item, affine, clip_path, flags));

	/* If necessary, queue a redraw of the old bounding box */
	if ((flags & MATE_CANVAS_UPDATE_VISIBILITY)
	    || (flags & MATE_CANVAS_UPDATE_AFFINE)
	    || priv->need_pos_update
	    || priv->need_font_update
	    || priv->need_text_update)
		mate_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);

	/* Compute new bounds */
	if (priv->need_pos_update
	    || priv->need_font_update
	    || priv->need_text_update)
		recompute_bounding_box (iti);

	/* Queue redraw */
	mate_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);

	priv->need_pos_update = FALSE;
	priv->need_font_update = FALSE;
	priv->need_text_update = FALSE;
	priv->need_state_update = FALSE;
}

static void
iti_draw_cursor (MateIconTextItem *iti, GdkDrawable *drawable,
		 int x, int y)
{
	int stem_width;
	int i;
	int cursor_offset;
	PangoRectangle pos;
	GtkEntry *entry;

	g_return_if_fail (iti->_priv->cursor_gc != NULL);

	entry = GTK_ENTRY (iti->_priv->entry);
	cursor_offset = gtk_editable_get_position (GTK_EDITABLE (entry));
	pango_layout_get_cursor_pos (iti->_priv->layout,
				     g_utf8_offset_to_pointer (entry->text, cursor_offset) - entry->text,
				     &pos, NULL);
	stem_width = PANGO_PIXELS (pos.height) / 30 + 1;
	for (i = 0; i < stem_width; i++) {
		gdk_draw_line (drawable, iti->_priv->cursor_gc,
			       x + PANGO_PIXELS (pos.x) + i - stem_width / 2,
			       y + PANGO_PIXELS (pos.y),
			       x + PANGO_PIXELS (pos.x) + i - stem_width / 2,
			       y + PANGO_PIXELS (pos.y) + PANGO_PIXELS (pos.height));
	}
}

/* Draw method handler for the icon text item */
static void
mate_icon_text_item_draw (MateCanvasItem *item, GdkDrawable *drawable,
			   int x, int y, int width, int height)
{
	GtkWidget *widget;
	GtkStyle *style;
	int xofs, yofs;
	int text_xofs, text_yofs;
	int w, h;
	MateIconTextItem *iti;
	MateIconTextItemPrivate *priv;
	GtkStateType state;

	widget = GTK_WIDGET (item->canvas);
	iti = MATE_ICON_TEXT_ITEM (item);
	priv = iti->_priv;

	style = GTK_WIDGET (MATE_CANVAS_ITEM (iti)->canvas)->style;

	w = priv->layout_width + 2 * MARGIN_X;
	h = priv->layout_height + 2 * MARGIN_Y;

	xofs = item->x1 - x;
	yofs = item->y1 - y;

	text_xofs = xofs - (iti->width - priv->layout_width - 2 * MARGIN_X) / 2;
	text_yofs = yofs + MARGIN_Y;

	if (iti->selected && GTK_WIDGET_HAS_FOCUS (widget))
		state = GTK_STATE_SELECTED;
	else if (iti->selected)
		state = GTK_STATE_ACTIVE;
	else
		state = GTK_STATE_NORMAL;

	if (iti->selected && !iti->editing)
		gdk_draw_rectangle (drawable,
				    style->base_gc[state],
				    TRUE,
				    xofs + 1, yofs + 1,
				    w - 2, h - 2);

	if (GTK_WIDGET_HAS_FOCUS (widget) && iti->focused && ! iti->editing) {
		GdkRectangle r;

		r.x = xofs;
		r.y = yofs;
		r.width = w - 1;
		r.height = h - 1;
		gtk_paint_focus (style, drawable,
				 iti->selected ? GTK_STATE_SELECTED: GTK_STATE_NORMAL,
				 &r, widget, NULL,
				 xofs, yofs, w - 1, h - 1);
#if 0
		gtk_draw_focus (style,
				drawable,
				xofs, yofs,
				w - 1, h - 1);
#endif
	}

	if (iti->editing) {
		/* FIXME: are these the right graphics contexts? */
		gdk_draw_rectangle (drawable,
				    style->base_gc[GTK_STATE_NORMAL],
				    TRUE,
				    xofs, yofs, w - 1, h - 1);
		gdk_draw_rectangle (drawable,
				    style->fg_gc[GTK_STATE_NORMAL],
				    FALSE,
				    xofs, yofs, w - 1, h - 1);
	}

	gdk_draw_layout (drawable,
			 style->text_gc[iti->editing ? GTK_STATE_NORMAL : state],
			 text_xofs,
			 text_yofs,
			 priv->layout);

	if (iti->editing) {
		int range[2];
		if (gtk_editable_get_selection_bounds (GTK_EDITABLE (priv->entry), &range[0], &range[1])) {
			GdkRegion *clip_region;
			GdkColor *selection_color, *text_color;
			guint8 state;

			range[0] = g_utf8_offset_to_pointer (GTK_ENTRY (priv->entry)->text, range[0]) - GTK_ENTRY (priv->entry)->text;
			range[1] = g_utf8_offset_to_pointer (GTK_ENTRY (priv->entry)->text, range[1]) - GTK_ENTRY (priv->entry)->text;

			state = GTK_WIDGET_HAS_FOCUS (widget) ? GTK_STATE_SELECTED : GTK_STATE_ACTIVE;
			selection_color = &widget->style->base[state];
			text_color = &widget->style->text[state];
			clip_region = gdk_pango_layout_get_clip_region
				(priv->layout,
				 text_xofs,
				 text_yofs,
				 range, 1);
			gdk_gc_set_clip_region (widget->style->black_gc,
						clip_region);
			gdk_draw_layout_with_colors (drawable,
						     widget->style->black_gc,
						     text_xofs,
						     text_yofs,
						     priv->layout,
						     text_color,
						     selection_color);
			gdk_gc_set_clip_region (widget->style->black_gc, NULL);
			gdk_region_destroy (clip_region);
		} else {
			iti_draw_cursor (iti, drawable, text_xofs, text_yofs);
		}
	}
}

static void
draw_pixbuf_aa (GdkPixbuf *pixbuf, MateCanvasBuf *buf, double affine[6], int x_offset, int y_offset)
{
	void (* affine_function)
		(art_u8 *dst, int x0, int y0, int x1, int y1, int dst_rowstride,
		 const art_u8 *src, int src_width, int src_height, int src_rowstride,
		 const double affine[6],
		 ArtFilterLevel level,
		 ArtAlphaGamma *alpha_gamma);

	affine[4] += x_offset;
	affine[5] += y_offset;

	affine_function = gdk_pixbuf_get_has_alpha (pixbuf)
		? art_rgb_rgba_affine
		: art_rgb_affine;

	(* affine_function)
		(buf->buf,
		 buf->rect.x0, buf->rect.y0,
		 buf->rect.x1, buf->rect.y1,
		 buf->buf_rowstride,
		 gdk_pixbuf_get_pixels (pixbuf),
		 gdk_pixbuf_get_width (pixbuf),
		 gdk_pixbuf_get_height (pixbuf),
		 gdk_pixbuf_get_rowstride (pixbuf),
		 affine,
		 ART_FILTER_NEAREST,
		 NULL);

	affine[4] -= x_offset;
	affine[5] -= y_offset;
}

static void
mate_icon_text_item_render (MateCanvasItem *item, MateCanvasBuf *buffer)
{
	GdkVisual *visual;
	GdkPixmap *pixmap;
	GdkPixbuf *text_pixbuf;
	double affine[6];
	int width, height;

	visual = gdk_rgb_get_visual ();
	art_affine_identity(affine);
	width  = ROUND (item->x2 - item->x1);
	height = ROUND (item->y2 - item->y1);

	pixmap = gdk_pixmap_new (NULL, width, height, visual->depth);

	gdk_draw_rectangle (pixmap, GTK_WIDGET (item->canvas)->style->white_gc,
			    TRUE,
			    0, 0,
			    width,
			    height);

	/* use a common routine to draw the label into the pixmap */
	mate_icon_text_item_draw (item, pixmap,
				   ROUND (item->x1), ROUND (item->y1),
				   width, height);

	/* turn it into a pixbuf */
	text_pixbuf = gdk_pixbuf_get_from_drawable
		(NULL, pixmap, gdk_rgb_get_colormap (),
		 0, 0,
		 0, 0,
		 width,
		 height);

	/* draw the pixbuf containing the label */
	draw_pixbuf_aa (text_pixbuf, buffer, affine, ROUND (item->x1), ROUND (item->y1));
	g_object_unref (text_pixbuf);

	buffer->is_bg = FALSE;
	buffer->is_buf = TRUE;
}

/* Bounds method handler for the icon text item */
static void
mate_icon_text_item_bounds (MateCanvasItem *item, double *x1, double *y1, double *x2, double *y2)
{
	MateIconTextItem *iti;
	MateIconTextItemPrivate *priv;
	int width, height;

	iti = MATE_ICON_TEXT_ITEM (item);
	priv = iti->_priv;

	width = priv->layout_width + 2 * MARGIN_X;
	height = priv->layout_height + 2 * MARGIN_Y;

	*x1 = iti->x + (iti->width - width) / 2;
	*y1 = iti->y;
	*x2 = *x1 + width;
	*y2 = *y1 + height;
}

/* Point method handler for the icon text item */
static double
mate_icon_text_item_point (MateCanvasItem *item, double x, double y, int cx, int cy, MateCanvasItem **actual_item)
{
	double dx, dy;

	*actual_item = item;

	if (cx < item->x1)
		dx = item->x1 - cx;
	else if (cx > item->x2)
		dx = cx - item->x2;
	else
		dx = 0.0;

	if (cy < item->y1)
		dy = item->y1 - cy;
	else if (cy > item->y2)
		dy = cy - item->y2;
	else
		dy = 0.0;

	return sqrt (dx * dx + dy * dy);
}

static gboolean
mate_icon_text_item_event (MateCanvasItem *item, GdkEvent *event)
{
	MateIconTextItem *iti;
	MateIconTextItemPrivate *priv;
	int idx;
	double cx, cy;

	iti = MATE_ICON_TEXT_ITEM (item);
	priv = iti->_priv;

	switch (event->type) {
	case GDK_KEY_PRESS:
		if (!iti->editing) {
			break;
		}

		switch(event->key.keyval) {

		/* Pass these events back to parent */
		case GDK_Escape:
		case GDK_Return:
		case GDK_KP_Enter:
			return FALSE;

		/* Handle up and down arrow keys.  GdkEntry does not know
		 * how to handle multi line items */
		case GDK_Up:
		case GDK_Down:
			iti_handle_arrow_key_event(iti, event);
			break;

		default:
			/* Check for control key operations */
			if (event->key.state & GDK_CONTROL_MASK) {
				return FALSE;
			}

			/* Handle any events that reach us */
			gtk_widget_event (GTK_WIDGET (priv->entry), event);
			break;
		}

		/* Update text item to reflect changes */
		update_pango_layout (iti);
		priv->need_text_update = TRUE;
		mate_canvas_item_request_update (item);
		return TRUE;

	case GDK_BUTTON_PRESS:
		if (!iti->editing) {
			break;
		}

		if (event->button.button == 1) {
			mate_canvas_w2c_d (item->canvas, event->button.x, event->button.y, &cx, &cy);
			idx = get_layout_index (iti,
						(cx - item->x1) + MARGIN_X,
						(cy - item->y1) + MARGIN_Y);
			iti_start_selecting (iti, idx, event->button.time);
		}
		return TRUE;
	case GDK_MOTION_NOTIFY:
		if (!priv->selecting)
			break;

		gtk_widget_event (GTK_WIDGET (priv->entry), event);
		mate_canvas_w2c_d (item->canvas, event->button.x, event->button.y, &cx, &cy);
		idx = get_layout_index (iti,
					floor ((cx - (item->x1 + MARGIN_X)) + .5),
					floor ((cy - (item->y1 + MARGIN_Y)) + .5));

		iti_selection_motion (iti, idx);
		return TRUE;

	case GDK_BUTTON_RELEASE:
		if (priv->selecting && event->button.button == 1)
			iti_stop_selecting (iti, event->button.time);
		else
			break;
		return TRUE;
	default:
		break;
	}

	return FALSE;
}

static void
mate_icon_text_item_destroy (GtkObject *object)
{
	MateIconTextItem *iti = MATE_ICON_TEXT_ITEM (object);
	MateIconTextItemPrivate *priv = iti->_priv;

	if (iti->text && iti->is_text_allocated) {
		g_free (iti->text);
		iti->text = NULL;
	}

	if (priv) {
		if (priv->layout) {
			g_object_unref (priv->layout);
			priv->layout = NULL;
		}

		if (priv->entry_top) {
			gtk_widget_destroy (priv->entry_top);
			priv->entry_top = NULL;
		}

		if (priv->cursor_gc) {
			g_object_unref (priv->cursor_gc);
			priv->cursor_gc = NULL;
		}

		g_free (priv);
		iti->_priv = NULL;
	}

	MATE_CALL_PARENT (GTK_OBJECT_CLASS, destroy, (object));
}

static void
mate_icon_text_item_class_init (MateIconTextItemClass *klass)
{
	MateCanvasItemClass *canvas_item_class;
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass *)klass;
	canvas_item_class = (MateCanvasItemClass *)klass;

	iti_signals [TEXT_CHANGED] =
		g_signal_new ("text_changed",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (MateIconTextItemClass, text_changed),
			      NULL, NULL,
			      _mate_marshal_BOOLEAN__VOID,
			      G_TYPE_BOOLEAN, 0);

	iti_signals [HEIGHT_CHANGED] =
		g_signal_new ("height_changed",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (MateIconTextItemClass, height_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	iti_signals [WIDTH_CHANGED] =
		g_signal_new ("width_changed",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (MateIconTextItemClass, width_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	iti_signals[EDITING_STARTED] =
		g_signal_new ("editing_started",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (MateIconTextItemClass, editing_started),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	iti_signals[EDITING_STOPPED] =
		g_signal_new ("editing_stopped",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (MateIconTextItemClass, editing_stopped),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	iti_signals[SELECTION_STARTED] =
		g_signal_new ("selection_started",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (MateIconTextItemClass, selection_started),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	iti_signals[SELECTION_STOPPED] =
		g_signal_new ("selection_stopped",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (MateIconTextItemClass, selection_stopped),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	object_class->destroy = mate_icon_text_item_destroy;

	canvas_item_class->update = mate_icon_text_item_update;
	canvas_item_class->draw = mate_icon_text_item_draw;
	canvas_item_class->render = mate_icon_text_item_render;
	canvas_item_class->bounds = mate_icon_text_item_bounds;
	canvas_item_class->point = mate_icon_text_item_point;
	canvas_item_class->event = mate_icon_text_item_event;
	canvas_item_class->realize = mate_icon_text_item_realize;
	canvas_item_class->unrealize = mate_icon_text_item_unrealize;
}

static void
mate_icon_text_item_instance_init (MateIconTextItem *item)
{
	item->_priv = g_new0 (MateIconTextItemPrivate, 1);
}

/**
 * mate_icon_text_item_configure:
 * @iti: An icon text item.
 * @x: X position in which to place the item.
 * @y: Y position in which to place the item.
 * @width: Maximum width allowed for this item, to be used for word wrapping.
 * @fontname: Ignored.
 * @text: Text that is going to be displayed.
 * @is_editable: Deprecated.
 * @is_static: Whether @text points to a static string or not.
 *
 * This routine is used to configure a &MateIconTextItem.
 *
 * @x and @y specify the cordinates where the item is placed inside the canvas.
 * The @x coordinate should be the leftmost position that the icon text item can
 * assume at any one time, that is, the left margin of the column in which the
 * icon is to be placed.  The @y coordinate specifies the top of the icon text
 * item.
 *
 * @width is the maximum width allowed for this icon text item.  The coordinates
 * define the upper-left corner of an icon text item with maximum width; this may
 * actually be outside the bounding box of the item if the text is narrower than
 * the maximum width.
 *
 * If @is_static is true, it means that there is no need for the item to
 * allocate memory for the string (it is a guarantee that the text is allocated
 * by the caller and it will not be deallocated during the lifetime of this
 * item).  This is an optimization to reduce memory usage for large icon sets.
 */
void
mate_icon_text_item_configure (MateIconTextItem *iti, int x, int y,
				int width, const char *fontname,
				const char *text,
				gboolean is_editable, gboolean is_static)
{
	MateIconTextItemPrivate *priv = iti->_priv;

	g_return_if_fail (MATE_IS_ICON_TEXT_ITEM (iti));
	g_return_if_fail (width > 2 * MARGIN_X);
	g_return_if_fail (text != NULL);

	iti->x = x;
	iti->y = y;
	iti->width = width;
	iti->is_editable = is_editable != FALSE;

	if (iti->editing)
		iti_stop_editing (iti);

	if (iti->text && iti->is_text_allocated)
		g_free (iti->text);

	iti->is_text_allocated = !is_static;

	/* This cast is to shut up the compiler */
	if (is_static)
		iti->text = (char *) text;
	else
		iti->text = g_strdup (text);

	/* Create our new PangoLayout */
	if (priv->layout != NULL)
		g_object_unref (priv->layout);
	priv->layout = gtk_widget_create_pango_layout (GTK_WIDGET (MATE_CANVAS_ITEM (iti)->canvas), iti->text);

	pango_layout_set_font_description (priv->layout, GTK_WIDGET (MATE_CANVAS_ITEM (iti)->canvas)->style->font_desc);

	pango_layout_set_alignment (priv->layout, PANGO_ALIGN_CENTER);
	update_pango_layout (iti);

	priv->need_pos_update = TRUE;
	priv->need_font_update = TRUE;
	priv->need_text_update = TRUE;
	mate_canvas_item_request_update (MATE_CANVAS_ITEM (iti));
}

/**
 * mate_icon_text_item_setxy:
 * @iti:  An icon text item.
 * @x: X position.
 * @y: Y position.
 *
 * Sets the coordinates at which the icon text item should be placed.
 *
 * See also: mate_icon_text_item_configure().
 */
void
mate_icon_text_item_setxy (MateIconTextItem *iti, int x, int y)
{
	MateIconTextItemPrivate *priv;

	g_return_if_fail (MATE_IS_ICON_TEXT_ITEM (iti));

	priv = iti->_priv;

	iti->x = x;
	iti->y = y;

	priv->need_pos_update = TRUE;
	mate_canvas_item_request_update (MATE_CANVAS_ITEM (iti));
}

/**
 * mate_icon_text_item_focus:
 * @iti:  An icon text item.
 * @focused: whether to set or unset the icon text item focus.
 *
 * Sets or unsets the focus on the icon text item depending on @focused.
 **/
void
mate_icon_text_item_focus (MateIconTextItem *iti, gboolean focused)
{
	MateIconTextItemPrivate *priv;

	g_return_if_fail (MATE_IS_ICON_TEXT_ITEM (iti));

	priv = iti->_priv;

	if (!iti->focused == !focused)
		return;

	iti->focused = focused ? TRUE : FALSE;

	priv->need_state_update = TRUE;
	mate_canvas_item_request_update (MATE_CANVAS_ITEM (iti));
}

/**
 * mate_icon_text_item_select:
 * @iti: An icon text item
 * @sel: Whether the icon text item should be displayed as selected.
 *
 * This function is used to control whether an icon text item is displayed as
 * selected or not.  Mouse events are ignored by the item when it is unselected;
 * when the user clicks on a selected icon text item, it will start the text
 * editing process.
 */
void
mate_icon_text_item_select (MateIconTextItem *iti, gboolean sel)
{
	MateIconTextItemPrivate *priv;

	g_return_if_fail (MATE_IS_ICON_TEXT_ITEM (iti));

	priv = iti->_priv;

	if (!iti->selected == !sel)
		return;

	iti->selected = sel ? TRUE : FALSE;

#ifdef FIXME
	if (!iti->selected && iti->editing)
		iti_edition_accept (iti);
#endif
	priv->need_state_update = TRUE;
	mate_canvas_item_request_update (MATE_CANVAS_ITEM (iti));
}


/**
 * mate_icon_text_item_get_text:
 * @iti: An icon text item.
 *
 * Returns the current text string in an icon text item.  The client should not
 * free this string, as it is internal to the icon text item.
 */
const char *
mate_icon_text_item_get_text (MateIconTextItem *iti)
{
	MateIconTextItemPrivate *priv;

	g_return_val_if_fail (MATE_IS_ICON_TEXT_ITEM (iti), NULL);

	priv = iti->_priv;

	if (iti->editing) {
		return gtk_entry_get_text (GTK_ENTRY(priv->entry));
	} else {
		return iti->text;
	}
}


/**
 * mate_icon_text_item_start_editing:
 * @iti: An icon text item.
 *
 * Starts the editing state of an icon text item.
 **/
void
mate_icon_text_item_start_editing (MateIconTextItem *iti)
{
	g_return_if_fail (MATE_IS_ICON_TEXT_ITEM (iti));

	if (iti->editing)
		return;

	iti->selected = TRUE; /* Ensure that we are selected */
	iti_ensure_focus (MATE_CANVAS_ITEM (iti));
	iti_start_editing (iti);
}

/**
 * mate_icon_text_item_stop_editing:
 * @iti: An icon text item.
 * @accept: Whether to accept the current text or to discard it.
 *
 * Terminates the editing state of an icon text item.  The @accept argument
 * controls whether the item's current text should be accepted or discarded.  If
 * it is discarded, then the icon's original text will be restored.
 **/
void
mate_icon_text_item_stop_editing (MateIconTextItem *iti,
				   gboolean accept)
{
	g_return_if_fail (MATE_IS_ICON_TEXT_ITEM (iti));

	if (!iti->editing)
		return;

	if (accept)
		iti_edition_accept (iti);
	else
		iti_stop_editing (iti);
}

/**
 * mate_icon_text_item_get_editable:
 * @iti: An icon text item.
 *
 * Retrieves the entry widget associated with the icon text.
 * 
 * Returns: a #GtkEditable if the entry widget is initialised, NULL otherwise.
 **/
GtkEditable *
mate_icon_text_item_get_editable  (MateIconTextItem *iti)
{
	MateIconTextItemPrivate *priv;

	priv = iti->_priv;
	return priv->entry ? GTK_EDITABLE (priv->entry) : NULL;
}
