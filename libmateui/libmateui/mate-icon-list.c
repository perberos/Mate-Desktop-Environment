/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 1998, 1999, 2000 Free Software Foundation
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

/*
 * MateIconList widget - scrollable icon list
 *
 * Authors:
 *    Federico Mena <federico@nuclecu.unam.mx>
 *    Miguel de Icaza <miguel@nuclecu.unam.mx>
 *
 * Rewrote from scratch from the code written by Federico Mena
 * <federico@nuclecu.unam.mx> to be based on a MateCanvas, and
 * to support banding selection and allow inline icon renaming.
 *
 * Redone somewhat by Elliot to support gdk-pixbuf, and to use GArray instead of GList for item storage.
 */

#include <config.h>
#include <string.h>
#include <stdio.h>
#include <gtk/gtk.h>
#include "mate-icon-list.h"
#include "mate-icon-item.h"
#include "mate-marshal.h"
#include <libmatecanvas/mate-canvas-pixbuf.h>
#include <libmatecanvas/mate-canvas-rect-ellipse.h>
#include <gdk/gdkkeysyms.h>

#include <libmateuiP.h>
#include "libmateui-access.h"

/* Aliases to minimize screen use in my laptop */
#define GIL(x)       MATE_ICON_LIST(x)
#define GIL_CLASS(x) MATE_ICON_LIST_CLASS(x)
#define IS_GIL(x)    MATE_IS_ICON_LIST(x)

typedef MateIconList Gil;
typedef MateIconListClass GilClass;
typedef MateIconListPrivate GilPrivate;

/* default spacings */
#define DEFAULT_ROW_SPACING  4
#define DEFAULT_COL_SPACING  2
#define DEFAULT_TEXT_SPACING 2
#define DEFAULT_ICON_BORDER  2

/* Autoscroll timeout in milliseconds */
#define SCROLL_TIMEOUT 30


/* Signals */
enum {
	SELECT_ICON,
	UNSELECT_ICON,
	FOCUS_ICON,
	UNFOCUS_ICON,
	TEXT_CHANGED,
	MOVE_CURSOR,
	TOGGLE_CURSOR_SELECTION,
	LAST_SIGNAL
};

typedef enum {
	SYNC_INSERT,
	SYNC_REMOVE
} SyncType;

static guint gil_signals[LAST_SIGNAL] = { 0 };

/**
 * mate_icon_list_get_type:
 *
 * Registers the &MateIconList class if necessary, and returns the type ID
 * associated to it.
 *
 * Returns: The type ID of the &MateIconList class.
 */
MATE_CLASS_BOILERPLATE (MateIconList, mate_icon_list,
			 MateCanvas, MATE_TYPE_CANVAS)

/* Icon structure */
typedef struct {
	/* Icon image and text items */
	MateCanvasPixbuf *image;
	MateIconTextItem *text;

	/* Filename of the icon file. */
	gchar *icon_filename;

	/* User data and destroy notify function */
	gpointer data;
	GDestroyNotify destroy;

	/* ID for the text item's event signal handler */
	guint text_event_id;

	/* Whether the icon is selected, and temporary storage for rubberband
         * selections.
	 */
	guint selected : 1;
	guint tmp_selected : 1;
} Icon;

/* A row of icons */
typedef struct {
	GList *line_icons;
	gint16 y;
	gint16 icon_height, text_height;
} IconLine;

/* Private data of the MateIconList structure */
struct _MateIconListPrivate {
	/* List of icons */
	GArray *icon_list;

	/* List of rows of icons */
	GList *lines;

	Icon *editing_icon;

	/* Separators used to wrap the text below icons */
	char *separators;

	Icon *last_selected_icon;

	/* Rubberband rectangle */
	MateCanvasItem *sel_rect;

	/* Saved event for a pending selection */
	GdkEvent select_pending_event;

	/* Max of the height of all the icon rows and window height */
	int total_height;

	/* Selection mode */
	GtkSelectionMode selection_mode;

	/* A list of integers with the indices of the currently selected icons */
	GList *selection;

	/* The icon that has keyboard focus */
	gint focus_icon;

	/* Number of icons in the list */
	int icons;

	/* Freeze count */
	int frozen;

	/* Width allocated for icons */
	int icon_width;

	/* Spacing values */
	int row_spacing;
	int col_spacing;
	int text_spacing;
	int icon_border;

	/* Index and pointer to last selected icon */
	int last_selected_idx;

	/* Timeout ID for autoscrolling */
	guint timer_tag;

	/* Change the adjustment value by this amount when autoscrolling */
	int value_diff;

	/* Mouse position for autoscrolling */
	int event_last_x;
	int event_last_y;

	/* Selection start position */
	int sel_start_x;
	int sel_start_y;

	int icons_per_row;

	/* Modifier state when the selection began */
	guint sel_state;

	/* Whether the icon texts are editable */
	guint is_editable : 1;

	/* Whether the icon texts need to be copied */
	guint static_text : 1;

	/* Whether the icons need to be laid out */
	guint dirty : 1;

	/* Whether the user is performing a rubberband selection */
	guint selecting : 1;

	/* Whether editing an icon is pending after a button press */
	guint edit_pending : 1;

	/* Whether selection is pending after a button press */
	guint select_pending : 1;

	/* Whether the icon that is pending selection was selected to begin with */
	guint select_pending_was_selected : 1;
};


static GType gil_accessible_get_type (void);


static inline int
icon_line_height (Gil *gil, IconLine *il)
{
	MateIconListPrivate *priv;

	priv = gil->_priv;

	return il->icon_height + il->text_height + priv->row_spacing + priv->text_spacing;
}

static void
icon_get_height (Icon *icon, int *icon_height, int *text_height)
{
	double d_icon_height, dy1, dy2;
	if (icon->image != NULL)
		g_object_get (G_OBJECT (icon->image), "height", &d_icon_height, NULL);
	else
		d_icon_height = 0.0;
	mate_canvas_item_get_bounds (MATE_CANVAS_ITEM (icon->text), NULL, &dy1, NULL, &dy2);
	*icon_height = d_icon_height;
	*text_height = dy2 - dy1;
}

static int
gil_get_items_per_line (Gil *gil)
{
	MateIconListPrivate *priv;
	int available_width;
	int items_per_line;

	priv = gil->_priv;

	available_width = GTK_WIDGET (gil)->allocation.width - 2 * DEFAULT_COL_SPACING;

	items_per_line = ((available_width + priv->col_spacing)
			  / (priv->icon_width + priv->col_spacing));

	if (items_per_line == 0)
		items_per_line = 1;

	return items_per_line;
}

/**
 * mate_icon_list_get_items_per_line:
 * @gil: An icon list.
 *
 * Returns: The number of icons that fit in a line or row.
 */
int
mate_icon_list_get_items_per_line (MateIconList *gil)
{
	g_return_val_if_fail (gil != NULL, 1);
	g_return_val_if_fail (IS_GIL (gil), 1);

	return gil_get_items_per_line (gil);
}

static void
gil_place_icon (Gil *gil, Icon *icon, int x, int y, int icon_height)
{
	MateIconListPrivate *priv;
	int x_offset, y_offset;
	double d_icon_image_height;
	double d_icon_image_width;
	int icon_image_height;
	int icon_image_width;

	priv = gil->_priv;

	if (icon->image != NULL) {
		g_object_get (G_OBJECT (icon->image), "height", &d_icon_image_height, NULL);
		icon_image_height = d_icon_image_height;
		g_assert(icon_image_height != 0);
		if (icon_height > icon_image_height)
			y_offset = (icon_height - icon_image_height) / 2;
		else
			y_offset = 0;

		g_object_get (G_OBJECT (icon->image), "width", &d_icon_image_width, NULL);
		icon_image_width = d_icon_image_width;
		g_assert(icon_image_width != 0);
		if (priv->icon_width > icon_image_width)
			x_offset = (priv->icon_width - icon_image_width) / 2;
		else
			x_offset = 0;

		mate_canvas_item_set (MATE_CANVAS_ITEM (icon->image),
				       "x",  (double) (x + x_offset),
				       "y",  (double) (y + y_offset),
				       NULL);
	}

	mate_icon_text_item_setxy (icon->text,
				    x,
				    y + icon_height + priv->text_spacing);
}

static void
gil_layout_line (Gil *gil, IconLine *il)
{
	MateIconListPrivate *priv;
	GList *l;
	int x;

	priv = gil->_priv;

	x = DEFAULT_COL_SPACING;
	for (l = il->line_icons; l; l = l->next) {
		Icon *icon = l->data;

		gil_place_icon (gil, icon, x, il->y, il->icon_height);
		x += priv->icon_width + priv->col_spacing;
	}
}

static void
gil_add_and_layout_line (Gil *gil, GList *line_icons, int y,
			 int icon_height, int text_height)
{
	MateIconListPrivate *priv;
	IconLine *il;

	priv = gil->_priv;

	il = g_new (IconLine, 1);
	il->line_icons = line_icons;
	il->y = y;
	il->icon_height = icon_height;
	il->text_height = text_height;

	gil_layout_line (gil, il);
	priv->lines = g_list_append (priv->lines, il);
}

static void
gil_relayout_icons_at (Gil *gil, int pos, int y)
{
	MateIconListPrivate *priv;
	int text_height, icon_height;
	int items_per_line, n;
	GList *line_icons;

	priv = gil->_priv;
	items_per_line = gil_get_items_per_line (gil);

	text_height = icon_height = 0;
	line_icons = NULL;

	for (n = pos; n < priv->icon_list->len; n++) {
		Icon *icon = g_array_index(priv->icon_list, Icon*, n);
		int ih, th;

		if (!(n % items_per_line)) {
			if (line_icons) {
				gil_add_and_layout_line (gil, line_icons, y,
							 icon_height, text_height);
				line_icons = NULL;

				y += (icon_height + text_height
				      + priv->row_spacing + priv->text_spacing);
			}

			icon_height = 0;
			text_height = 0;
		}

		icon_get_height (icon, &ih, &th);

		icon_height = MAX (ih, icon_height);
		text_height = MAX (th, text_height);

		line_icons = g_list_append (line_icons, icon);
	}

	if (line_icons)
		gil_add_and_layout_line (gil, line_icons, y, icon_height, text_height);
}

static void
gil_free_line_info (Gil *gil)
{
	MateIconListPrivate *priv;
	GList *l;

	priv = gil->_priv;

	for (l = priv->lines; l; l = l->next) {
		IconLine *il = l->data;

		g_list_free (il->line_icons);
		g_free (il);
	}

	g_list_free (priv->lines);
	priv->lines = NULL;
	priv->total_height = 0;
}

static void
gil_free_line_info_from (Gil *gil, int first_line)
{
	MateIconListPrivate *priv;
	GList *l, *ll;

	priv = gil->_priv;
	ll = g_list_nth (priv->lines, first_line);

	for (l = ll; l; l = l->next) {
		IconLine *il = l->data;

		g_list_free (il->line_icons);
		g_free (il);
	}

	if (priv->lines) {
		if (ll->prev)
			ll->prev->next = NULL;
		else
			priv->lines = NULL;
	}

	g_list_free (ll);
}

static void G_GNUC_UNUSED
gil_layout_from_line (Gil *gil, int line)
{
	MateIconListPrivate *priv;
	GList *l;
	int height;

	priv = gil->_priv;

	gil_free_line_info_from (gil, line);

	height = 0;
	for (l = priv->lines; l; l = l->next) {
		IconLine *il = l->data;

		height += icon_line_height (gil, il);
	}

	gil_relayout_icons_at (gil, line * gil_get_items_per_line (gil), height);
}

static void
gil_layout_all_icons (Gil *gil)
{
	MateIconListPrivate *priv;

	priv = gil->_priv;

	if (!GTK_WIDGET_REALIZED (gil))
		return;

	gil_free_line_info (gil);
	gil_relayout_icons_at (gil, 0, DEFAULT_ROW_SPACING);
	priv->dirty = FALSE;
}

static void
gil_scrollbar_adjust (Gil *gil)
{
	MateIconListPrivate *priv;
	GList *l;
	int height, step_increment;
	double wx, wy;

	priv = gil->_priv;

	if (!GTK_WIDGET_REALIZED (gil))
		return;

	height = 0;
	step_increment = 0;
	for (l = priv->lines; l; l = l->next) {
		IconLine *il = l->data;

		height += icon_line_height (gil, il);

		if (l == priv->lines)
			step_increment = height;
	}

	if (!step_increment)
		step_increment = 10;

	priv->total_height = MAX (height, GTK_WIDGET (gil)->allocation.height);

	wx = wy = 0;
	mate_canvas_window_to_world (MATE_CANVAS (gil), 0, 0, &wx, &wy);

	gil->adj->upper = height;
	gil->adj->step_increment = step_increment;
	gil->adj->page_increment = GTK_WIDGET (gil)->allocation.height;
	gil->adj->page_size = GTK_WIDGET (gil)->allocation.height;

	if (wy > gil->adj->upper - gil->adj->page_size)
		wy = gil->adj->upper - gil->adj->page_size;

	gil->adj->value = wy;

	gtk_adjustment_changed (gil->adj);
}

/* Emits the select_icon or unselect_icon signals as appropriate */
static void
emit_select (Gil *gil, int sel, int i, GdkEvent *event)
{
	g_signal_emit (gil, gil_signals[sel ? SELECT_ICON : UNSELECT_ICON], 0,
		       i, event);
}

static int
gil_unselect_all (MateIconList *gil, GdkEvent *event, gpointer keep)
{
	MateIconListPrivate *priv;
	Icon *icon;
	int i, idx = 0;

	g_return_val_if_fail (gil != NULL, 0);
	g_return_val_if_fail (IS_GIL (gil), 0);

	priv = gil->_priv;

	for (i = 0; i < priv->icon_list->len; i++) {
		icon = g_array_index(priv->icon_list, Icon*, i);

		if (icon == keep)
			idx = i;
		else if (icon->selected)
			emit_select (gil, FALSE, i, event);
	}

	return idx;
}

/**
 * mate_icon_list_select_all:
 * @gil:   An icon list.
 */
void
mate_icon_list_select_all (MateIconList *gil)
{
	guint i;
	
	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));
	
	for (i = 0; i < mate_icon_list_get_num_icons (gil); i++)
		mate_icon_list_select_icon (gil, i);
}

/**
 * mate_icon_list_unselect_all:
 * @gil:   An icon list.
 *
 * Returns: The number of icons in the icon list
 */
int
mate_icon_list_unselect_all (MateIconList *gil)
{
	return gil_unselect_all (gil, NULL, NULL);
}

static void
sync_selection (Gil *gil, int pos, SyncType type)
{
	GList *list;

	for (list = gil->_priv->selection; list; list = list->next) {
		if (GPOINTER_TO_INT (list->data) >= pos) {
			int i = GPOINTER_TO_INT (list->data);

			switch (type) {
			case SYNC_INSERT:
				list->data = GINT_TO_POINTER (i + 1);
				break;

			case SYNC_REMOVE:
				list->data = GINT_TO_POINTER (i - 1);
				break;

			default:
				g_assert_not_reached ();
			}
		}
	}
}

static int
gil_icon_to_index (Gil *gil, Icon *icon)
{
	MateIconListPrivate *priv;
	int n;

	priv = gil->_priv;

	for (n = 0; n < priv->icon_list->len; n++)
		if (g_array_index(priv->icon_list, Icon*, n) == icon)
			return n;

	g_assert_not_reached ();
	return -1; /* Shut up the compiler */
}

/* Event handler for icons when we are in SINGLE or BROWSE mode */
static gint
selection_one_icon_event (Gil *gil, Icon *icon, int idx, int on_text, GdkEvent *event)
{
	MateIconListPrivate *priv;
	MateIconTextItem *text;
	int retval;

	priv = gil->_priv;
	retval = FALSE;

	/* We use a separate variable and ref the object because it may be
	 * destroyed by one of the signal handlers.
	 */
	text = icon->text;
	g_object_ref (G_OBJECT (text));

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		priv->edit_pending = FALSE;
		priv->select_pending = FALSE;

		if (!icon->selected) {
			gil_unselect_all (gil, NULL, NULL);
			emit_select (gil, TRUE, idx, event);
			mate_icon_list_focus_icon (gil, idx);
		} else {
			if (priv->selection_mode == GTK_SELECTION_SINGLE
			    && (event->button.state & GDK_CONTROL_MASK))
				emit_select (gil, FALSE, idx, event);
			else if (on_text && priv->is_editable && event->button.button == 1)
				priv->edit_pending = TRUE;
			else
				emit_select (gil, TRUE, idx, event);
		}

		retval = TRUE;
		break;

	case GDK_2BUTTON_PRESS:
	case GDK_3BUTTON_PRESS:
		emit_select (gil, TRUE, idx, event);
		retval = TRUE;
		break;

	case GDK_BUTTON_RELEASE:
		if (priv->edit_pending) {
			priv->editing_icon = icon;
			mate_icon_text_item_start_editing (text);
			priv->edit_pending = FALSE;
		}
		retval = TRUE;
		break;

	default:
		break;
	}

	/* If the click was on the text and we actually did something, stop the
	 * icon text item's own handler from executing.
	 */
	if (on_text && retval)
		g_signal_stop_emission_by_name (text, "event");

	g_object_unref (G_OBJECT (text));

	return retval;
}

/* Handles range selections when clicking on an icon */
static void
select_range (Gil *gil, Icon *icon, int idx, GdkEvent *event)
{
	MateIconListPrivate *priv;
	int a, b;
	Icon *i;

	priv = gil->_priv;

	if (priv->last_selected_idx == -1) {
		priv->last_selected_idx = idx;
		priv->last_selected_icon = icon;
	}

	if (idx < priv->last_selected_idx) {
		a = idx;
		b = priv->last_selected_idx;
	} else {
		a = priv->last_selected_idx;
		b = idx;
	}

	for (; a <= b; a++) {
		i = g_array_index(priv->icon_list, Icon*, a);

		if (!i->selected)
			emit_select (gil, TRUE, a, NULL);
	}

	/* Actually notify the client of the event */
	emit_select (gil, TRUE, idx, event);
	mate_icon_list_focus_icon (gil, idx);
}

/* Handles icon selection for MULTIPLE or EXTENDED selection modes */
static void
do_select_many (Gil *gil, Icon *icon, int idx, GdkEvent *event, int use_event)
{
	MateIconListPrivate *priv;
	int range, additive;

	priv = gil->_priv;

	range = (event->button.state & GDK_SHIFT_MASK) != 0;
	additive = (event->button.state & GDK_CONTROL_MASK) != 0;

	if (!additive) {
		if (icon->selected)
			gil_unselect_all (gil, NULL, icon);
		else
			gil_unselect_all (gil, NULL, NULL);
	}

	if (!range) {
		if (additive)
			emit_select (gil, !icon->selected, idx, use_event ? event : NULL);
		else
			emit_select (gil, TRUE, idx, use_event ? event : NULL);

		priv->last_selected_idx = idx;
		priv->last_selected_icon = icon;
	} else
		select_range (gil, icon, idx, use_event ? event : NULL);

	mate_icon_list_focus_icon (gil, idx);
}

/* Event handler for icons when we are in MULTIPLE mode */
static gint
selection_many_icon_event (Gil *gil, Icon *icon, int idx, int on_text, GdkEvent *event)
{
	MateIconListPrivate *priv;
	MateIconTextItem *text;
	int retval;
	int additive, range;
	int do_select;

	priv = gil->_priv;
	retval = FALSE;

	/* We use a separate variable and ref the object because it may be
	 * destroyed by one of the signal handlers.
	 */
	text = icon->text;
	g_object_ref (G_OBJECT (text));

	range = (event->button.state & GDK_SHIFT_MASK) != 0;
	additive = (event->button.state & GDK_CONTROL_MASK) != 0;

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		priv->edit_pending = FALSE;
		priv->select_pending = FALSE;

		do_select = TRUE;

		if (additive || range) {
			if (additive && !range) {
				priv->select_pending = TRUE;
				priv->select_pending_event = *event;
				priv->select_pending_was_selected = icon->selected;

				/* We have to emit this so that the client will
				 * know about the click.
				 */
				emit_select (gil, TRUE, idx, event);
				do_select = FALSE;
			}
		} else if (icon->selected) {
			priv->select_pending = TRUE;
			priv->select_pending_event = *event;
			priv->select_pending_was_selected = icon->selected;

			if (on_text && priv->is_editable && event->button.button == 1)
				priv->edit_pending = TRUE;

			emit_select (gil, TRUE, idx, event);
			do_select = FALSE;
		}
#if 0
		} else if (icon->selected && on_text && priv->is_editable
			   && event->button.button == 1) {
			priv->edit_pending = TRUE;
			do_select = FALSE;
		}
#endif

		if (do_select)
			do_select_many (gil, icon, idx, event, TRUE);

		retval = TRUE;
		break;

	case GDK_2BUTTON_PRESS:
	case GDK_3BUTTON_PRESS:

		emit_select (gil, TRUE, idx, event);
		retval = TRUE;
		break;

	case GDK_BUTTON_RELEASE:
		if (priv->select_pending) {
			icon->selected = priv->select_pending_was_selected;
			do_select_many (gil, icon, idx, &priv->select_pending_event, FALSE);
			priv->select_pending = FALSE;
			retval = TRUE;
		}

		if (priv->edit_pending) {
			priv->editing_icon = icon;
			mate_icon_text_item_start_editing (text);
			priv->edit_pending = FALSE;
			retval = TRUE;
		}

		break;

	default:
		break;
	}

	/* If the click was on the text and we actually did something, stop the
	 * icon text item's own handler from executing.
	 */
	if (on_text && retval)
		g_signal_stop_emission_by_name (text, "event");

	g_object_unref (G_OBJECT (text));

	return retval;
}

/* Event handler for icons in the icon list */
static gint
icon_event (MateCanvasItem *item, GdkEvent *event, gpointer data)
{
	Icon *icon;
	Gil *gil;
	MateIconListPrivate *priv;
	int idx;
	int on_text;

	icon = data;
	gil = GIL (item->canvas);
	priv = gil->_priv;
	idx = gil_icon_to_index (gil, icon);
	on_text = item == MATE_CANVAS_ITEM (icon->text);

	/* Don't handle events meant for editing text items */
	if (on_text && priv->is_editable
	    && MATE_ICON_TEXT_ITEM (item)->editing) {
		return FALSE;
	}

	if (priv->editing_icon && event->type == GDK_BUTTON_PRESS) {
		mate_icon_text_item_stop_editing (priv->editing_icon->text, FALSE);
		priv->editing_icon = NULL;
	}

	switch (priv->selection_mode) {
	case GTK_SELECTION_SINGLE:
	case GTK_SELECTION_BROWSE:
		return selection_one_icon_event (gil, icon, idx, on_text, event);

	case GTK_SELECTION_MULTIPLE:
		return selection_many_icon_event (gil, icon, idx, on_text, event);

	default:
		g_assert_not_reached ();
		return FALSE; /* Shut up the compiler */
	}
}

#if 0
/* Handler for the editing_started signal of an icon text item.  We block the
 * event handler so that it will not be called while the text is being edited.
 */
static void G_GNUC_UNUSED
editing_started (MateIconTextItem *iti, gpointer data)
{
	Icon *icon;

	icon = data;
	g_signal_handler_block (iti, icon->text_event_id);
	gil_unselect_all (GIL (MATE_CANVAS_ITEM (iti)->canvas), NULL, icon);
}

/* Handler for the editing_stopped signal of an icon text item.  We unblock the
 * event handler so that we can get events from it again.
 */
static void G_GNUC_UNUSED
editing_stopped (MateIconTextItem *iti, gpointer data)
{
	Icon *icon;

	icon = data;
	g_signal_handler_unblock (iti, icon->text_event_id);
}

static gboolean G_GNUC_UNUSED
text_changed (MateCanvasItem *item, Icon *icon)
{
	Gil *gil;
	gboolean accept;
	gchar *text = NULL;
	int idx;

	gil = GIL (item->canvas);
	accept = TRUE;

	idx = gil_icon_to_index (gil, icon);
	g_object_get (G_OBJECT (icon->text), "text", &text, NULL);
	g_signal_emit (gil, gil_signals[TEXT_CHANGED], 0,
		       idx, text,
		       &accept);

	return accept;
}

static void G_GNUC_UNUSED
height_changed (MateCanvasItem *item, Icon *icon)
{
	Gil *gil;
	MateIconListPrivate *priv;
	int n;

	gil = GIL (item->canvas);
	priv = gil->_priv;

	if (!GTK_WIDGET_REALIZED (gil))
		return;

	if (priv->frozen) {
		priv->dirty = TRUE;
		return;
	}

	n = gil_icon_to_index (gil, icon);
	gil_layout_from_line (gil, n / gil_get_items_per_line (gil));
	gil_scrollbar_adjust (gil);
}
#endif

static Icon *
icon_new_from_pixbuf (MateIconList *gil, GdkPixbuf *im,
		      const char *icon_filename, const char *text)
{
	MateIconListPrivate *priv;
	MateCanvas *canvas;
	Icon *icon;

	priv = gil->_priv;
	canvas = MATE_CANVAS (gil);

	icon = g_new0 (Icon, 1);

	icon->icon_filename = g_strdup (icon_filename);

	if (im != NULL) {
		icon->image = MATE_CANVAS_PIXBUF (mate_canvas_item_new (
			MATE_CANVAS_GROUP (canvas->root),
			MATE_TYPE_CANVAS_PIXBUF,
			"x", 0.0,
			"y", 0.0,
			"width", (double) gdk_pixbuf_get_width (im),
			"height", (double) gdk_pixbuf_get_height (im),
			"pixbuf", im,
			"anchor", GTK_ANCHOR_NW,
			NULL));
	} else {
		icon->image = NULL;
	}

	icon->text = MATE_ICON_TEXT_ITEM (mate_canvas_item_new (
		MATE_CANVAS_GROUP (canvas->root),
		MATE_TYPE_ICON_TEXT_ITEM,
		NULL));

	mate_icon_text_item_configure (icon->text,
					0, 0, priv->icon_width, NULL,
					text, priv->is_editable, priv->static_text);

	if (icon->image != NULL) {
		g_signal_connect (G_OBJECT (icon->image), "event",
				  G_CALLBACK (icon_event),
				  icon);
	}
	icon->text_event_id = g_signal_connect (G_OBJECT (icon->text), "event",
						G_CALLBACK (icon_event),
						icon);

#if 0
	g_signal_connect (G_OBJECT (icon->text), "editing_started",
			  G_CALLBACK (editing_started),
			  icon);
	g_signal_connect (G_OBJECT (icon->text), "editing_stopped",
			  G_CALLBACK (editing_stopped),
			  icon);

	g_signal_connect (G_OBJECT (icon->text), "text_changed",
			  G_CALLBACK (text_changed),
			  icon);
	g_signal_connect (G_OBJECT (icon->text), "height_changed",
			  G_CALLBACK (height_changed),
			  icon);
#endif

	return icon;
}

static Icon *
icon_new (Gil *gil, const char *icon_filename, const char *text)
{
	GdkPixbuf *im;
	Icon *retval;

	if (icon_filename)
		im = gdk_pixbuf_new_from_file (icon_filename, NULL);
	else
		im = NULL;

	retval = icon_new_from_pixbuf (gil, im, icon_filename, text);

	if(im)
		g_object_unref (G_OBJECT (im));

	return retval;
}

static int
icon_list_append (Gil *gil, Icon *icon)
{
	MateIconListPrivate *priv;
	AtkObject *accessible;

	priv = gil->_priv;

	priv->icons++;
	g_array_append_val(priv->icon_list, icon);

	switch (priv->selection_mode) {
	case GTK_SELECTION_BROWSE:
		mate_icon_list_select_icon (gil, 0);
		break;

	default:
		break;
	}

	if (!priv->frozen) {
		/* FIXME: this should only layout the last line */
		gil_layout_all_icons (gil);
		gil_scrollbar_adjust (gil);
	} else
		priv->dirty = TRUE;

	/* Notify the accessible object
	 *
	 * FIXME: do we need to pass the accessible object for the new child
	 * as one of the signal parameters?
	 */

	accessible = _accessibility_get_atk_object (gil);
	if (accessible)
		g_signal_emit_by_name (accessible, "children_changed::add",
				       priv->icons - 1, NULL, NULL);

	return priv->icons - 1;
}

static void
icon_list_insert (Gil *gil, int pos, Icon *icon)
{
	MateIconListPrivate *priv;
	AtkObject *accessible;

	priv = gil->_priv;

	if (pos == priv->icons) {
		icon_list_append (gil, icon);
		return;
	}

	g_array_insert_val(priv->icon_list, pos, icon);
	priv->icons++;

	switch (priv->selection_mode) {
	case GTK_SELECTION_BROWSE:
		mate_icon_list_select_icon (gil, 0);
		break;

	default:
		break;
	}

	if (!priv->frozen) {
		/* FIXME: this should only layout the lines from then one
		 * containing the Icon to the end.
		 */
		gil_layout_all_icons (gil);
		gil_scrollbar_adjust (gil);
	} else
		priv->dirty = TRUE;

	sync_selection (gil, pos, SYNC_INSERT);

	/* Notify the accessible object
	 *
	 * FIXME: do we need to pass the accessible object for the new child
	 * as one of the signal parameters?
	 */

	accessible = _accessibility_get_atk_object (gil);
	if (accessible)
		g_signal_emit_by_name (accessible, "children_changed::add",
				       pos, NULL, NULL);
}

/**
 * mate_icon_list_insert_pixbuf:
 * @gil:      An icon list.
 * @pos:      Position at which the new icon should be inserted.
 * @im:       Pixbuf image with the icon image.
 * @icon_filename: Filename of the image file.
 * @text:     Text to be used for the icon's caption.
 *
 * Inserts an icon in the specified icon list.  The icon is created from the
 * specified Imlib image, and it is inserted at the @pos index.
 */
void
mate_icon_list_insert_pixbuf (MateIconList *gil, int pos, GdkPixbuf *im,
			       const char *icon_filename, const char *text)
{
	Icon *icon;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));
	g_return_if_fail (im != NULL);

	icon = icon_new_from_pixbuf (gil, im, icon_filename, text);
	icon_list_insert (gil, pos, icon);
	return;
}

/**
 * mate_icon_list_insert:
 * @gil:           An icon list.
 * @pos:           Position at which the new icon should be inserted.
 * @icon_filename: Name of the file that holds the icon's image.
 * @text:          Text to be used for the icon's caption.
 *
 * Inserts an icon in the specified icon list.  The icon's image is loaded
 * from the specified file, and it is inserted at the @pos index.
 */
void
mate_icon_list_insert (MateIconList *gil, int pos, const char *icon_filename, const char *text)
{
	Icon *icon;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));

	icon = icon_new (gil, icon_filename, text);
	icon_list_insert (gil, pos, icon);
	return;
}

/**
 * mate_icon_list_append_pixbuf:
 * @gil:      An icon list.
 * @im:       Pixbuf image with the icon image.
 * @icon_filename: Filename of the image file.
 * @text:     Text to be used for the icon's caption.
 *
 * Appends an icon to the specified icon list.  The icon is created from
 * the specified Imlib image.
 *
 * Returns:
 */
int
mate_icon_list_append_pixbuf (MateIconList *gil, GdkPixbuf *im,
			       const char *icon_filename, const char *text)
{
	Icon *icon;

	g_return_val_if_fail (gil != NULL, -1);
	g_return_val_if_fail (IS_GIL (gil), -1);
	g_return_val_if_fail (im != NULL, -1);

	icon = icon_new_from_pixbuf (gil, im, icon_filename, text);
	return icon_list_append (gil, icon);
}

/**
 * mate_icon_list_append:
 * @gil:           An icon list.
 * @icon_filename: Name of the file that holds the icon's image.
 * @text:          Text to be used for the icon's caption.
 *
 * Appends an icon to the specified icon list.  The icon's image is loaded from
 * the specified file, and it is inserted at the @pos index.
 *
 * Returns:
 */
int
mate_icon_list_append (MateIconList *gil, const char *icon_filename,
			const char *text)
{
	Icon *icon;

	g_return_val_if_fail (gil != NULL, -1);
	g_return_val_if_fail (IS_GIL (gil), -1);

	icon = icon_new (gil, icon_filename, text);
	return icon_list_append (gil, icon);
}

static void
icon_destroy (Icon *icon)
{
	if (icon->destroy)
		(* icon->destroy) (icon->data);
	icon->destroy = NULL;
	icon->data = NULL;

	g_free (icon->icon_filename);
	icon->icon_filename = NULL;

	if (icon->image != NULL)
		gtk_object_destroy (GTK_OBJECT (icon->image));
	icon->image = NULL;

	gtk_object_destroy (GTK_OBJECT (icon->text));
	icon->text = NULL;

	g_free (icon);
}

/**
 * mate_icon_list_remove:
 * @gil: An icon list.
 * @pos: Index of the icon that should be removed.
 *
 * Removes the icon at index position @pos.  If a destroy handler was specified
 * for that icon, it will be called with the appropriate data.
 */
void
mate_icon_list_remove (MateIconList *gil, int pos)
{
	MateIconListPrivate *priv;
	int was_selected;
	Icon *icon;
	AtkObject *accessible;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));
	g_return_if_fail (pos >= 0 && pos < gil->_priv->icons);

	priv = gil->_priv;

	was_selected = FALSE;

	icon = g_array_index(priv->icon_list, Icon*, pos);

	if (icon->selected) {
		was_selected = TRUE;

		switch (priv->selection_mode) {
		case GTK_SELECTION_SINGLE:
		case GTK_SELECTION_BROWSE:
		case GTK_SELECTION_MULTIPLE:
			mate_icon_list_unselect_icon (gil, pos);
			break;

		default:
			break;
		}
	}

	g_array_remove_index(priv->icon_list, pos);
	priv->icons--;

	sync_selection (gil, pos, SYNC_REMOVE);

	if (was_selected) {
		switch (priv->selection_mode) {
		case GTK_SELECTION_BROWSE:
			if (pos == priv->icons)
				mate_icon_list_select_icon (gil, pos - 1);
			else
				mate_icon_list_select_icon (gil, pos);

			break;

		default:
			break;
		}
	}

	if (priv->icons >= priv->last_selected_idx)
		priv->last_selected_idx = -1;

	if (priv->icons >= priv->focus_icon)
		priv->focus_icon = -1;

	if (priv->last_selected_icon == icon)
		priv->last_selected_icon = NULL;

	icon_destroy (icon);

	if (!priv->frozen) {
		/* FIXME: Optimize, only re-layout from pos to end */
		gil_layout_all_icons (gil);
		gil_scrollbar_adjust (gil);
	} else
		priv->dirty = TRUE;

	/* Notify the accessibility object
	 *
	 * FIXME: the child no longer exists!  Do we still need to pass its
	 * corresponding accessible object as one of the signal parameters?
	 */
	accessible = _accessibility_get_atk_object (gil);
	if (accessible)
		g_signal_emit_by_name (accessible, "children_changed::remove",
				       pos, NULL, NULL);
}

/**
 * mate_icon_list_clear:
 * @gil: An icon list.
 *
 * Clears the contents for the icon list by removing all the icons.  If destroy
 * handlers were specified for any of the icons, they will be called with the
 * appropriate data.
 */
void
mate_icon_list_clear (MateIconList *gil)
{
	MateIconListPrivate *priv;
	int i;
	AtkObject *accessible;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));

	priv = gil->_priv;

	for (i = 0; i < priv->icon_list->len; i++)
		icon_destroy (g_array_index (priv->icon_list, Icon*, i));

	gil_free_line_info (gil);

	g_list_free (priv->selection);
	priv->selection = NULL;
	g_array_set_size(priv->icon_list, 0);
	priv->icons = 0;
	priv->focus_icon = -1;
	priv->last_selected_idx = -1;
	priv->last_selected_icon = NULL;

	if (!priv->frozen) {
		gil_layout_all_icons (gil);
		gil_scrollbar_adjust (gil);
	} else
		priv->dirty = TRUE;

	/* Notify the accessible object */

	accessible = _accessibility_get_atk_object (gil);
	if (accessible)
		g_signal_emit_by_name (accessible, "children_changed", 0, NULL, NULL);
}

static void
gil_destroy (GtkObject *object)
{
	Gil *gil;

	/* remember, destroy can be run multiple times! */

	gil = GIL (object);

	gil->_priv->frozen = 1;
	gil->_priv->dirty  = TRUE;

	if (gil->_priv->icon_list != NULL) {
		mate_icon_list_clear (gil);
		g_array_free(gil->_priv->icon_list, TRUE);
		gil->_priv->icon_list = NULL;
	}

	if (gil->_priv->timer_tag != 0) {
		g_source_remove (gil->_priv->timer_tag);
		gil->_priv->timer_tag = 0;
	}

	if (gil->adj) {
		g_object_unref (G_OBJECT (gil->adj));
		gil->adj = NULL;
	}

	if (gil->hadj) {
		g_object_unref (G_OBJECT (gil->hadj));
		gil->hadj = NULL;
	}

	MATE_CALL_PARENT (GTK_OBJECT_CLASS, destroy, (object));
}

static void
gil_finalize (GObject *object)
{
	Gil *gil;

	gil = GIL (object);

	g_free (gil->_priv->separators);
	gil->_priv->separators = NULL;

	g_free (gil->_priv);
	gil->_priv = NULL;

	MATE_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));
}

void
mate_icon_list_focus_icon (MateIconList *gil, gint idx)
{
	g_return_if_fail (IS_GIL (gil));
	g_return_if_fail (idx >= 0 && idx < gil->_priv->icons);

	g_signal_emit (gil, gil_signals[FOCUS_ICON], 0,
		       idx);
}

static void
select_icon (Gil *gil, int pos, GdkEvent *event)
{
	MateIconListPrivate *priv;
	gint i;
	Icon *icon;

	priv = gil->_priv;

	switch (priv->selection_mode) {
	case GTK_SELECTION_SINGLE:
	case GTK_SELECTION_BROWSE:
		i = 0;

		for (i = 0; i < priv->icon_list->len; i++) {
			icon = g_array_index (priv->icon_list, Icon*, i);

			if (i != pos && icon->selected)
				emit_select (gil, FALSE, i, event);
		}

		emit_select (gil, TRUE, pos, event);
		break;

	case GTK_SELECTION_MULTIPLE:
		emit_select (gil, TRUE, pos, event);
		break;

	default:
		break;
	}
}

/**
 * mate_icon_list_select_icon:
 * @gil: An icon list.
 * @pos: Index of the icon to be selected.
 *
 * Selects the icon at the index specified by @pos.
 */
void
mate_icon_list_select_icon (MateIconList *gil, int pos)
{
	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));
	g_return_if_fail (pos >= 0 && pos < gil->_priv->icons);

	select_icon (gil, pos, NULL);
}

static void
unselect_icon (Gil *gil, int pos, GdkEvent *event)
{
	MateIconListPrivate *priv;

	priv = gil->_priv;

	switch (priv->selection_mode) {
	case GTK_SELECTION_SINGLE:
	case GTK_SELECTION_BROWSE:
	case GTK_SELECTION_MULTIPLE:
		emit_select (gil, FALSE, pos, event);
		break;

	default:
		break;
	}
}

/**
 * mate_icon_list_unselect_icon:
 * @gil: An icon list.
 * @pos: Index of the icon to be unselected.
 *
 * Unselects the icon at the index specified by @pos.
 */
void
mate_icon_list_unselect_icon (MateIconList *gil, int pos)
{
	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));
	g_return_if_fail (pos >= 0 && pos < gil->_priv->icons);

	unselect_icon (gil, pos, NULL);
}

static void
gil_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	requisition->width = 1;
	requisition->height = 1;
}

static void
gil_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	Gil *gil;
	MateIconListPrivate *priv;

	gil = GIL (widget);
	priv = gil->_priv;

	if (GTK_WIDGET_CLASS (parent_class)->size_allocate)
		(* GTK_WIDGET_CLASS (parent_class)->size_allocate) (widget, allocation);

	if (priv->frozen)
		return;

	gil_layout_all_icons (gil);
	gil_scrollbar_adjust (gil);
}

static void
gil_realize (GtkWidget *widget)
{
	Gil *gil;
	MateIconListPrivate *priv;
	GdkColor color;

	gil = GIL (widget);
	priv = gil->_priv;

	priv->frozen++;

	if (GTK_WIDGET_CLASS (parent_class)->realize)
		(* GTK_WIDGET_CLASS (parent_class)->realize) (widget);

	priv->frozen--;

	/* Change the style to use the base color as the background */

	color = widget->style->base[GTK_STATE_NORMAL];
	gtk_widget_modify_bg (widget, GTK_STATE_NORMAL, &color);

	gdk_window_set_background (GTK_LAYOUT (gil)->bin_window,
				   &widget->style->bg[GTK_STATE_NORMAL]);

	if (priv->frozen)
		return;

	if (priv->dirty) {
		gil_layout_all_icons (gil);
		gil_scrollbar_adjust (gil);
	}
}

static void
real_move_cursor (Gil *gil, GtkDirectionType dir, gboolean clear_selection)
{
	MateIconListPrivate *priv;
	gint items_per_line;
	gint new_focus_icon;

	priv = gil->_priv;

	items_per_line = gil_get_items_per_line (gil);
	new_focus_icon = priv->focus_icon;

	switch (dir) {
	case GTK_DIR_RIGHT:
		if (priv->focus_icon + 1 < priv->icons &&
		    priv->focus_icon % items_per_line != (items_per_line - 1))
			new_focus_icon++;
		break;

	case GTK_DIR_LEFT:
		if (priv->focus_icon - 1 >= 0 &&
		    priv->focus_icon % items_per_line != 0)
			new_focus_icon--;
		break;

	case GTK_DIR_DOWN:
		if (priv->focus_icon + items_per_line < priv->icons)
			new_focus_icon += items_per_line;
		break;

	case GTK_DIR_UP:
		if (priv->focus_icon - items_per_line >= 0)
			new_focus_icon -= items_per_line;
		break;

	case GTK_SCROLL_START:
		if (priv->focus_icon != 0) {
			new_focus_icon = 0;
		}
		break;

	case GTK_SCROLL_END:
		if (priv->focus_icon != (priv->icons -1)) {
			new_focus_icon = priv->icons -1;
		}
		break;

	default:
		break;
	}

	if (new_focus_icon < 0)
		return;

	if (clear_selection &&
	    g_array_index (priv->icon_list, Icon *, new_focus_icon)->selected == FALSE) {
		mate_icon_list_unselect_all (gil);
		mate_icon_list_select_icon (gil, new_focus_icon);
	}

	if (mate_icon_list_icon_is_visible (gil, new_focus_icon) != GTK_VISIBILITY_FULL &&
	    (dir == GTK_DIR_UP || dir == GTK_DIR_DOWN || GTK_SCROLL_START || GTK_SCROLL_END))
		mate_icon_list_moveto (gil, new_focus_icon, dir == GTK_DIR_UP ? 0.0 : 1.0);

	mate_icon_list_focus_icon (gil, new_focus_icon);
}

static void
real_toggle_cursor_selection (Gil *gil)
{
	MateIconListPrivate *priv;
	Icon *icon;

	priv = gil->_priv;

	if (priv->focus_icon == -1)
		return;

	icon = g_array_index (priv->icon_list, Icon*, priv->focus_icon);

	if (icon->selected)
		mate_icon_list_unselect_icon (gil, priv->focus_icon);
	else
		mate_icon_list_select_icon (gil, priv->focus_icon);
}

/* Used from g_list_insert_sorted() */
static gint
selection_list_compare_cb (gconstpointer a, gconstpointer b)
{
	int ia, ib;

	ia = GPOINTER_TO_INT (a);
	ib = GPOINTER_TO_INT (b);

	if (ia < ib)
		return -1;
	else if (ia > ib)
		return 1;
	else
		return 0;
}

static void
real_select_icon (Gil *gil, gint num, GdkEvent *event)
{
	MateIconListPrivate *priv;
	Icon *icon;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));
	g_return_if_fail (num >= 0 && num < gil->_priv->icons);

	priv = gil->_priv;

	icon = g_array_index (priv->icon_list, Icon*, num);

	if (icon->selected)
		return;

	icon->selected = TRUE;
	mate_icon_text_item_select (icon->text, TRUE);
	if (g_list_find(priv->selection, GINT_TO_POINTER(num)) == NULL)
		priv->selection = g_list_insert_sorted (priv->selection, GINT_TO_POINTER (num),
							selection_list_compare_cb);
}

static void
real_unselect_icon (Gil *gil, gint num, GdkEvent *event)
{
	MateIconListPrivate *priv;
	Icon *icon;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));
	g_return_if_fail (num >= 0 && num < gil->_priv->icons);

	priv = gil->_priv;

	icon = g_array_index (priv->icon_list, Icon*, num);

	if (!icon->selected)
		return;

	icon->selected = FALSE;
	mate_icon_text_item_select (icon->text, FALSE);
	priv->selection = g_list_remove (priv->selection, GINT_TO_POINTER (num));
}

static void
real_focus_icon (Gil *gil, gint num)
{
	MateIconListPrivate *priv;
	Icon *icon;

	priv = gil->_priv;

	if (num == priv->focus_icon)
		return;

	if (priv->focus_icon >= 0) {
		icon = g_array_index (priv->icon_list, Icon*, priv->focus_icon);
		mate_icon_text_item_focus (icon->text, FALSE);
	}

	icon = g_array_index (priv->icon_list, Icon*, num);
	mate_icon_text_item_focus (icon->text, TRUE);

	priv->focus_icon = num;
}

/* Saves the selection of the icon list to temporary storage */
static void
store_temp_selection (Gil *gil)
{
	MateIconListPrivate *priv;
	int i;
	Icon *icon;

	priv = gil->_priv;

	for (i = 0; i < priv->icon_list->len; i++) {
		icon = g_array_index(priv->icon_list, Icon*, i);

		icon->tmp_selected = icon->selected;
	}
}

#define gray50_width 2
#define gray50_height 2
static const char gray50_bits[] = {
  0x02, 0x01, };

/* Button press handler for the icon list */
static gint
gil_button_press (GtkWidget *widget, GdkEventButton *event)
{
	Gil *gil;
	MateIconListPrivate *priv;
	int only_one;
	GdkBitmap *stipple;
	double tx, ty;

	gil = GIL (widget);
	priv = gil->_priv;

	if (event->window == GTK_LAYOUT (widget)->bin_window) {
		if (!GTK_WIDGET_HAS_FOCUS (widget))
			gtk_widget_grab_focus (widget);
	}

	/* Invoke the canvas event handler and see if an item picks up the event */
	if ((* GTK_WIDGET_CLASS (parent_class)->button_press_event) (widget, event))
		return TRUE;

	if (!(event->type == GDK_BUTTON_PRESS
	      && event->button == 1
	      && priv->selection_mode != GTK_SELECTION_BROWSE))
		return FALSE;

	only_one = priv->selection_mode == GTK_SELECTION_SINGLE;

	if (only_one || (event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK)) == 0)
		gil_unselect_all (gil, NULL, NULL);

	if (only_one)
		return TRUE;

	if (priv->selecting)
		return FALSE;

	mate_canvas_window_to_world (MATE_CANVAS (gil), event->x, event->y, &tx, &ty);
	priv->sel_start_x = tx;
	priv->sel_start_y = ty;
	priv->sel_state = event->state;
	priv->selecting = TRUE;

	store_temp_selection (gil);

	stipple = gdk_bitmap_create_from_data (NULL, gray50_bits, gray50_width, gray50_height);
	priv->sel_rect = mate_canvas_item_new (mate_canvas_root (MATE_CANVAS (gil)),
						MATE_TYPE_CANVAS_RECT,
						"x1", tx,
						"y1", ty,
						"x2", tx,
						"y2", ty,
						"outline_color", "black",
						"width_pixels", 1,
						"outline_stipple", stipple,
						NULL);
	g_object_unref (G_OBJECT (stipple));

	mate_canvas_item_grab (priv->sel_rect, GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
				NULL, event->time);

	return TRUE;
}

/* Returns whether the specified icon is at least partially inside the specified
 * rectangle.
 */
static int
icon_is_in_area (Icon *icon, int x1, int y1, int x2, int y2)
{
	double ix1, iy1, ix2, iy2;

	if (x1 == x2 && y1 == y2)
		return FALSE;

	if (icon->image != NULL) {
		mate_canvas_item_get_bounds (MATE_CANVAS_ITEM (icon->image),
					      &ix1, &iy1, &ix2, &iy2);

		if (ix1 <= x2 && iy1 <= y2 && ix2 >= x1 && iy2 >= y1)
			return TRUE;
	}

	mate_canvas_item_get_bounds (MATE_CANVAS_ITEM (icon->text), &ix1, &iy1, &ix2, &iy2);

	if (ix1 <= x2 && iy1 <= y2 && ix2 >= x1 && iy2 >= y1)
		return TRUE;

	return FALSE;
}

/* Updates the rubberband selection to the specified point */
static void
update_drag_selection (Gil *gil, int x, int y)
{
	MateIconListPrivate *priv;
	int x1, x2, y1, y2;
	int i;
	Icon *icon;
	int additive, invert;

	priv = gil->_priv;

	/* Update rubberband */

	if (priv->sel_start_x < x) {
		x1 = priv->sel_start_x;
		x2 = x;
	} else {
		x1 = x;
		x2 = priv->sel_start_x;
	}

	if (priv->sel_start_y < y) {
		y1 = priv->sel_start_y;
		y2 = y;
	} else {
		y1 = y;
		y2 = priv->sel_start_y;
	}

	if (x1 < 0)
		x1 = 0;

	if (y1 < 0)
		y1 = 0;

	if (x2 >= GTK_WIDGET (gil)->allocation.width)
		x2 = GTK_WIDGET (gil)->allocation.width - 1;

	if (y2 >= priv->total_height)
		y2 = priv->total_height - 1;

	mate_canvas_item_set (priv->sel_rect,
			       "x1", (double) x1,
			       "y1", (double) y1,
			       "x2", (double) x2,
			       "y2", (double) y2,
			       NULL);

	/* Select or unselect icons as appropriate */

	additive = priv->sel_state & GDK_SHIFT_MASK;
	invert = priv->sel_state & GDK_CONTROL_MASK;

	for (i = 0; i < priv->icon_list->len; i++) {
		icon = g_array_index(priv->icon_list, Icon*, i);

		if (icon_is_in_area (icon, x1, y1, x2, y2)) {
			if (invert) {
				if (icon->selected == icon->tmp_selected)
					emit_select (gil, !icon->selected, i, NULL);
			} else if (additive) {
				if (!icon->selected)
					emit_select (gil, TRUE, i, NULL);
			} else {
				if (!icon->selected)
					emit_select (gil, TRUE, i, NULL);
			}
		} else if (icon->selected != icon->tmp_selected)
			emit_select (gil, icon->tmp_selected, i, NULL);
	}
}

/* Button release handler for the icon list */
static gint
gil_button_release (GtkWidget *widget, GdkEventButton *event)
{
	Gil *gil;
	MateIconListPrivate *priv;
	double x, y;

	gil = GIL (widget);
	priv = gil->_priv;

	if (!priv->selecting)
		return (* GTK_WIDGET_CLASS (parent_class)->button_release_event) (widget, event);

	if (event->button != 1)
		return FALSE;

	mate_canvas_window_to_world (MATE_CANVAS (gil), event->x, event->y, &x, &y);
	update_drag_selection (gil, x, y);
	mate_canvas_item_ungrab (priv->sel_rect, event->time);

	gtk_object_destroy (GTK_OBJECT (priv->sel_rect));
	priv->sel_rect = NULL;
	priv->selecting = FALSE;

	if (priv->timer_tag != 0) {
		g_source_remove (priv->timer_tag);
		priv->timer_tag = 0;
	}

	return TRUE;
}

/* Autoscroll timeout handler for the icon list */
static gint
scroll_timeout (gpointer data)
{
	Gil *gil;
	MateIconListPrivate *priv;
	double x, y;
	int value;

	GDK_THREADS_ENTER ();

	gil = data;
	priv = gil->_priv;

	value = gil->adj->value + priv->value_diff;
	if (value > gil->adj->upper - gil->adj->page_size)
		value = gil->adj->upper - gil->adj->page_size;

	gtk_adjustment_set_value (gil->adj, value);

	mate_canvas_window_to_world (MATE_CANVAS (gil),
				      priv->event_last_x, priv->event_last_y,
				      &x, &y);
	update_drag_selection (gil, x, y);

	GDK_THREADS_LEAVE();

	return TRUE;
}

/* Motion event handler for the icon list */
static gint
gil_motion_notify (GtkWidget *widget, GdkEventMotion *event)
{
	Gil *gil;
	MateIconListPrivate *priv;
	double x, y;
	gint absolute_y;
	GtkAdjustment *adj;

	gil = GIL (widget);
	priv = gil->_priv;

	if (!priv->selecting)
		return (* GTK_WIDGET_CLASS (parent_class)->motion_notify_event) (widget, event);

	adj = gtk_layout_get_vadjustment (GTK_LAYOUT (gil));
	mate_canvas_window_to_world (MATE_CANVAS (gil), event->x, event->y, &x, &y);
	update_drag_selection (gil, x, y);

	absolute_y = event->y - priv->total_height * (adj->value / (adj->upper -  adj->lower));

	/* If we are out of bounds, schedule a timeout that will do the scrolling */
	if (absolute_y < 0 || absolute_y > widget->allocation.height) {

		if (priv->timer_tag == 0)
			priv->timer_tag = g_timeout_add (SCROLL_TIMEOUT, scroll_timeout, gil);

		if (absolute_y < 0)
			priv->value_diff = absolute_y;
		else
			priv->value_diff = absolute_y - widget->allocation.height;

		priv->event_last_x = event->x;
		priv->event_last_y = event->y;

		/* Make the steppings be relative to the mouse distance from the
		 * canvas.  Also notice the timeout above is small to give a
		 * more smooth movement.
		 */
		priv->value_diff /= 4;
	} else if (priv->timer_tag != 0) {
		g_source_remove (priv->timer_tag);
		priv->timer_tag = 0;
	}

	return TRUE;
}

static GObject *
mate_icon_list_constructor (GType                  type,
			     guint                  n_properties,
			     GObjectConstructParam *properties)
{
	GObject *gil;

	gil = G_OBJECT_CLASS (parent_class)->constructor (type,
							  n_properties,
							  properties);

	mate_canvas_set_scroll_region (MATE_CANVAS (gil), 0.0, 0.0, 1000000.0, 1000000.0);
	mate_canvas_scroll_to (MATE_CANVAS (gil), 0, 0);

	return gil;
}

static gint
gil_focus_in (GtkWidget *widget, GdkEventFocus *event)
{
	Gil *gil;

	gil = GIL (widget);

	GTK_WIDGET_SET_FLAGS (widget, GTK_HAS_FOCUS);

	if (gil->_priv->icons > 0 && gil->_priv->focus_icon == -1) {
		mate_icon_list_focus_icon (gil, 0);
	}

	gtk_widget_queue_draw (widget);

	return FALSE;
}

static gint
gil_focus_out (GtkWidget *widget, GdkEventFocus *event)
{
	GTK_WIDGET_UNSET_FLAGS (widget, GTK_HAS_FOCUS);

	gtk_widget_queue_draw (widget);

	return FALSE;
}

static gint
gil_key_press (GtkWidget        *widget,
	       GdkEventKey      *event)
{
	gboolean handled;

	handled = gtk_bindings_activate (GTK_OBJECT (widget),
					 event->keyval,
					 event->state);

	if (handled)
		return TRUE;

	if (GTK_WIDGET_CLASS (parent_class)->key_press_event &&
	    GTK_WIDGET_CLASS (parent_class)->key_press_event (widget, event))
		return TRUE;

	return FALSE;
}

static void
gil_set_scroll_adjustments (GtkLayout *layout,
			    GtkAdjustment *hadjustment,
			    GtkAdjustment *vadjustment)
{
	Gil *gil = GIL (layout);

	mate_icon_list_set_hadjustment (gil, hadjustment);
	mate_icon_list_set_vadjustment (gil, vadjustment);
}

static void
add_move_binding (GtkBindingSet   *binding_set,
                  guint            keyval,
                  GtkDirectionType dir)
{
	gtk_binding_entry_add_signal (binding_set, keyval, 0, "move_cursor", 2,
				      G_TYPE_ENUM, dir,
				      G_TYPE_INT, TRUE);

	gtk_binding_entry_add_signal (binding_set, keyval, GDK_CONTROL_MASK, "move_cursor", 2,
				      G_TYPE_ENUM, dir,
				      G_TYPE_INT, FALSE);
}

static gint
gil_scroll (GtkWidget *widget, GdkEventScroll *event)
{
	gdouble new_value;
	GtkAdjustment *adj;

	if (event->direction != GDK_SCROLL_UP &&
	    event->direction != GDK_SCROLL_DOWN)
		return FALSE;

	adj = GIL (widget)->adj;

	if (event->direction == GDK_SCROLL_UP)
		new_value = adj->value - adj->page_increment / 2;
	else
		new_value = adj->value + adj->page_increment / 2;

	new_value = CLAMP (new_value, adj->lower, adj->upper - adj->page_size);
	gtk_adjustment_set_value (adj, new_value);

	return TRUE;
}

static AtkObject *
gil_get_accessible (GtkWidget *widget)
{
	AtkObject *accessible;

	if ((accessible = _accessibility_get_atk_object (widget)) != NULL)
		return accessible;

	accessible = g_object_new (gil_accessible_get_type (), NULL);

	return _accessibility_set_atk_object_return (widget, accessible);
}

static void
gil_style_set (GtkWidget *widget, GtkStyle *prev_style)
{

	Gil *gil;
	MateIconListPrivate *priv;
	MateIconTextItem *item;
	int item_count;

	gil = GIL (widget);
	priv = gil->_priv;

	if (priv->icons) {
		char *file_name;

		for (item_count=0; item_count < priv->icons; item_count++) {
			item = mate_icon_list_get_icon_text_item (gil, item_count);
			file_name = g_strdup (item->text);
			mate_icon_text_item_configure (item, 0, 0,
											priv->icon_width, NULL,
											file_name, priv->is_editable,
											priv->static_text);

			g_free (file_name);
		}

	}


	if (GTK_WIDGET_CLASS (parent_class)->style_set)
		(* GTK_WIDGET_CLASS (parent_class)->style_set) (widget, prev_style);

}



static void
mate_icon_list_class_init (GilClass *gil_class)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;
	GtkWidgetClass *widget_class;
	GtkLayoutClass *layout_class;

	GtkBindingSet *binding_set;

	binding_set = gtk_binding_set_by_class (gil_class);

	object_class = (GtkObjectClass *)   gil_class;
	gobject_class = (GObjectClass *)    gil_class;
	widget_class = (GtkWidgetClass *)   gil_class;
	layout_class = (GtkLayoutClass *)   gil_class;

	gil_signals[SELECT_ICON] =
		g_signal_new ("select_icon",
			      G_TYPE_FROM_CLASS (gobject_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (MateIconListClass, select_icon),
			      NULL, NULL,
			      _mate_marshal_VOID__INT_BOXED,
			      G_TYPE_NONE, 2,
			      G_TYPE_INT,
			      GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

	gil_signals[UNSELECT_ICON] =
		g_signal_new ("unselect_icon",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (MateIconListClass, unselect_icon),
			      NULL, NULL,
			      _mate_marshal_VOID__INT_BOXED,
			      G_TYPE_NONE, 2,
			      G_TYPE_INT,
			      GDK_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

	gil_signals[FOCUS_ICON] =
		g_signal_new ("focus_icon",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (MateIconListClass, focus_icon),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__INT,
			      G_TYPE_NONE, 1,
			      G_TYPE_INT);

	gil_signals[TEXT_CHANGED] =
		g_signal_new ("text_changed",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (MateIconListClass, text_changed),
			      NULL, NULL,
			      _mate_marshal_BOOLEAN__INT_STRING,
			      G_TYPE_BOOLEAN, 2,
			      G_TYPE_INT,
			      G_TYPE_STRING);

	gil_signals[MOVE_CURSOR] =
		g_signal_new ("move_cursor",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (MateIconListClass, move_cursor),
			      NULL, NULL,
			      _mate_marshal_VOID__ENUM_BOOLEAN,
			      G_TYPE_NONE, 2,
			      GTK_TYPE_DIRECTION_TYPE,
			      G_TYPE_BOOLEAN);

	gil_signals[TOGGLE_CURSOR_SELECTION] =
		g_signal_new ("toggle_cursor_selection",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (MateIconListClass, toggle_cursor_selection),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	add_move_binding (binding_set, GDK_Right, GTK_DIR_RIGHT);
	add_move_binding (binding_set, GDK_Left, GTK_DIR_LEFT);
	add_move_binding (binding_set, GDK_Down, GTK_DIR_DOWN);
	add_move_binding (binding_set, GDK_Up, GTK_DIR_UP);

	add_move_binding (binding_set, GDK_Home, GTK_SCROLL_START);
	add_move_binding (binding_set, GDK_KP_Home, GTK_SCROLL_START);
	add_move_binding (binding_set, GDK_End, GTK_SCROLL_END);
	add_move_binding (binding_set, GDK_KP_End, GTK_SCROLL_END);

	gtk_binding_entry_add_signal (binding_set, GDK_space, 0, "toggle_cursor_selection", 0);
	gtk_binding_entry_add_signal (binding_set, GDK_space, GDK_CONTROL_MASK, "toggle_cursor_selection", 0);

	object_class->destroy = gil_destroy;
	gobject_class->finalize = gil_finalize;
	gobject_class->constructor = mate_icon_list_constructor;

	widget_class->size_request = gil_size_request;
	widget_class->size_allocate = gil_size_allocate;
	widget_class->realize = gil_realize;
	widget_class->button_press_event = gil_button_press;
	widget_class->button_release_event = gil_button_release;
	widget_class->motion_notify_event = gil_motion_notify;
	widget_class->focus_in_event = gil_focus_in;
	widget_class->focus_out_event = gil_focus_out;
	widget_class->key_press_event = gil_key_press;
	widget_class->scroll_event = gil_scroll;
	widget_class->get_accessible = gil_get_accessible;
    widget_class->style_set = gil_style_set;

	/* we override GtkLayout's set_scroll_adjustments signal instead
	 * of creating a new signal so as to keep binary compatibility.
	 * Anyway, a widget class only needs one of these signals, and
	 * this gives the correct implementation for MateIconList */
	layout_class->set_scroll_adjustments = gil_set_scroll_adjustments;

	gil_class->select_icon = real_select_icon;
	gil_class->unselect_icon = real_unselect_icon;
	gil_class->focus_icon = real_focus_icon;
	gil_class->move_cursor = real_move_cursor;
	gil_class->toggle_cursor_selection = real_toggle_cursor_selection;
}

static void
mate_icon_list_instance_init (Gil *gil)
{
	gil->_priv = g_new0 (MateIconListPrivate, 1);

	gil->_priv->icon_list = g_array_new(FALSE, FALSE, sizeof(gpointer));
	gil->_priv->row_spacing = DEFAULT_ROW_SPACING;
	gil->_priv->col_spacing = DEFAULT_COL_SPACING;
	gil->_priv->text_spacing = DEFAULT_TEXT_SPACING;
	gil->_priv->icon_border = DEFAULT_ICON_BORDER;
	gil->_priv->separators = g_strdup (" ");

	gil->_priv->selection_mode = GTK_SELECTION_SINGLE;
	gil->_priv->dirty = TRUE;

	gil->_priv->focus_icon = -1;

	GTK_WIDGET_SET_FLAGS (gil, GTK_CAN_FOCUS);
}

/**
 * mate_icon_list_set_icon_width:
 * @gil: An icon list.
 * @w:   New width for the icon columns.
 *
 * Sets the amount of horizontal space allocated to the icons, i.e. the column
 * width of the icon list.
 */
void
mate_icon_list_set_icon_width (MateIconList *gil, int w)
{
	MateIconListPrivate *priv;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));

	priv = gil->_priv;

	priv->icon_width  = w;

	if (priv->frozen) {
		priv->dirty = TRUE;
		return;
	}

	gil_layout_all_icons (gil);
	gil_scrollbar_adjust (gil);
}

static void
gil_adj_value_changed (GtkAdjustment *adj, Gil *gil)
{
	mate_canvas_scroll_to (MATE_CANVAS (gil), 0, adj->value);
}

/**
 * mate_icon_list_set_hadjustment:
 * @gil: An icon list.
 * @hadj: Adjustment to be used for horizontal scrolling.
 *
 * Sets the adjustment to be used for horizontal scrolling.  This is normally
 * not required, as the icon list can be simply inserted in a &GtkScrolledWindow
 * and scrolling will be handled automatically.
 **/
void
mate_icon_list_set_hadjustment (MateIconList *gil, GtkAdjustment *hadj)
{
	GtkAdjustment *old_adjustment;

	/* hadj isn't used but is here for compatibility with GtkScrolledWindow */

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));

	if (hadj)
		g_return_if_fail (GTK_IS_ADJUSTMENT (hadj));

	if (gil->hadj == hadj)
		return;

	old_adjustment = gil->hadj;

	if (gil->hadj)
		g_object_unref (G_OBJECT (gil->hadj));

	gil->hadj = hadj;

	if (gil->hadj) {
		g_object_ref (G_OBJECT (gil->hadj));
		/* The horizontal adjustment is not used, so set some default
		 * values to indicate that everything is visible horizontally.
		 */
		gil->hadj->lower = 0.0;
		gil->hadj->upper = 1.0;
		gil->hadj->value = 0.0;
		gil->hadj->step_increment = 1.0;
		gil->hadj->page_increment = 1.0;
		gil->hadj->page_size = 1.0;
		gtk_adjustment_changed (gil->hadj);
	}

	if (!gil->hadj || !old_adjustment)
		gtk_widget_queue_resize (GTK_WIDGET (gil));
}

/**
 * mate_icon_list_set_vadjustment:
 * @gil: An icon list.
 * @vadj: Adjustment to be used for horizontal scrolling.
 *
 * Sets the adjustment to be used for vertical scrolling.  This is normally not
 * required, as the icon list can be simply inserted in a &GtkScrolledWindow and
 * scrolling will be handled automatically.
 **/
void
mate_icon_list_set_vadjustment (MateIconList *gil, GtkAdjustment *vadj)
{
	GtkAdjustment *old_adjustment;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));

	if (vadj)
		g_return_if_fail (GTK_IS_ADJUSTMENT (vadj));

	if (gil->adj == vadj)
		return;

	old_adjustment = gil->adj;

	if (gil->adj) {
		g_signal_handlers_disconnect_matched (gil->adj, G_SIGNAL_MATCH_DATA,
						      0, 0, NULL, NULL, gil);
		g_object_unref (gil->adj);
	}

	gil->adj = vadj;

	if (gil->adj) {
		g_object_ref_sink (gil->adj);
		g_signal_connect (gil->adj, "value_changed",
				  G_CALLBACK (gil_adj_value_changed), gil);
		g_signal_connect (gil->adj, "changed",
				  G_CALLBACK (gil_adj_value_changed), gil);
	}

	if (!gil->adj || !old_adjustment)
		gtk_widget_queue_resize (GTK_WIDGET (gil));
}

/**
 * mate_icon_list_construct:
 * @gil: An icon list.
 * @icon_width: Width for the icon columns.
 * @adj:
 * @flags: A combination of %MATE_ICON_LIST_IS_EDITABLE and %MATE_ICON_LIST_STATIC_TEXT.
 *
 * Constructor for the icon list, to be used by derived classes.
 **/
void
mate_icon_list_construct (MateIconList *gil, guint icon_width, GtkAdjustment *adj, int flags)
{
	MateIconListPrivate *priv;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));

	priv = gil->_priv;

	mate_icon_list_set_icon_width (gil, icon_width);
	priv->is_editable = (flags & MATE_ICON_LIST_IS_EDITABLE) != 0;
	priv->static_text = (flags & MATE_ICON_LIST_STATIC_TEXT) != 0;

	if (!adj)
		adj = GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, 1, 0.1, 0.1, 0.1));

	mate_icon_list_set_vadjustment (gil, adj);
}


/**
 * mate_icon_list_new: [constructor]
 * @icon_width: Width for the icon columns.
 * @adj:
 * @flags: A combination of %MATE_ICON_LIST_IS_EDITABLE and
 * %MATE_ICON_LIST_STATIC_TEXT.
 *
 * Creates a new icon list widget.  The icon columns are allocated a width of
 * @icon_width pixels.  Icon captions will be word-wrapped to this width as
 * well.
 *
 * If @flags has the %MATE_ICON_LIST_IS_EDITABLE flag set, then the user will
 * be able to edit the text in the icon captions, and the "text_changed" signal
 * will be emitted when an icon's text is changed.
 *
 * If @flags has the %MATE_ICON_LIST_STATIC_TEXT flags set, then the text
 * for the icon captions will not be copied inside the icon list; it will only
 * store the pointers to the original text strings specified by the application.
 * This is intended to save memory.  If this flag is not set, then the text
 * strings will be copied and managed internally.
 *
 * Returns: a newly-created icon list widget
 */
GtkWidget *
mate_icon_list_new (guint icon_width, GtkAdjustment *adj, int flags)
{
	Gil *gil;

	gtk_widget_push_colormap (gdk_rgb_get_colormap ());
	gil = g_object_new (MATE_TYPE_ICON_LIST, NULL);
	gtk_widget_pop_colormap ();

	mate_icon_list_construct (gil, icon_width, adj, flags);

	return GTK_WIDGET (gil);
}


/**
 * mate_icon_list_freeze:
 * @gil:  An icon list.
 *
 * Freezes an icon list so that any changes made to it will not be
 * reflected on the screen until it is thawed with mate_icon_list_thaw().
 * It is recommended to freeze the icon list before inserting or deleting
 * many icons, for example, so that the layout process will only be executed
 * once, when the icon list is finally thawed.
 *
 * You can call this function multiple times, but it must be balanced with the
 * same number of calls to mate_icon_list_thaw() before the changes will take
 * effect.
 */
void
mate_icon_list_freeze (MateIconList *gil)
{
	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));

	gil->_priv->frozen++;

	/* We hide the root so that the user will not see any changes while the
	 * icon list is doing stuff.
	 */

	if (gil->_priv->frozen == 1)
		mate_canvas_item_hide (MATE_CANVAS (gil)->root);
}

/**
 * mate_icon_list_thaw:
 * @gil:  An icon list.
 *
 * Thaws the icon list and performs any pending layout operations.  This
 * is to be used in conjunction with mate_icon_list_freeze().
 */
void
mate_icon_list_thaw (MateIconList *gil)
{
	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));
	g_return_if_fail (gil->_priv->frozen > 0);

	gil->_priv->frozen--;

	if (gil->_priv->dirty) {
                gil_layout_all_icons (gil);
                gil_scrollbar_adjust (gil);
        }

	if (gil->_priv->frozen == 0)
		mate_canvas_item_show (MATE_CANVAS (gil)->root);
}

/**
 * mate_icon_list_get_selection_mode:
 * @gil:  An icon list.
 *
 * Returns: The current selection mode for an icon list.
 */
GtkSelectionMode
mate_icon_list_get_selection_mode (MateIconList *gil)
{
	g_return_val_if_fail (gil != NULL, 0);
	g_return_val_if_fail (IS_GIL (gil), 0);

	return gil->_priv->selection_mode;
}

/**
 * mate_icon_list_set_selection_mode:
 * @gil:  An icon list.
 * @mode: New selection mode.
 *
 * Sets the selection mode for an icon list.  The %GTK_SELECTION_MULTIPLE and
 * %GTK_SELECTION_EXTENDED modes are considered equivalent.
 */
void
mate_icon_list_set_selection_mode (MateIconList *gil, GtkSelectionMode mode)
{
	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));

	gil->_priv->selection_mode = mode;
	gil->_priv->last_selected_idx = -1;
	gil->_priv->last_selected_icon = NULL;
}

/**
 * mate_icon_list_set_icon_data_full:
 * @gil:     An icon list.
 * @pos:     Index of an icon.
 * @data:    User data to set on the icon.
 * @destroy: Destroy notification handler for the icon.
 *
 * Associates the @data pointer to the icon at the index specified by @pos.  The
 * @destroy argument points to a function that will be called when the icon is
 * destroyed, or NULL if no function is to be called when this happens.
 */
void
mate_icon_list_set_icon_data_full (MateIconList *gil,
				    int pos, gpointer data,
				    GDestroyNotify destroy)
{
	Icon *icon;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));
	g_return_if_fail (pos >= 0 && pos < gil->_priv->icons);

	icon = g_array_index (gil->_priv->icon_list, Icon*, pos);
	icon->data = data;
	icon->destroy = destroy;
}

/**
 * mate_icon_list_get_icon_data:
 * @gil:  An icon list.
 * @pos:  Index of an icon.
 * @data: User data to set on the icon.
 *
 * Associates the @data pointer to the icon at the index specified by @pos.
 */
void
mate_icon_list_set_icon_data (MateIconList *gil, int pos, gpointer data)
{
	mate_icon_list_set_icon_data_full (gil, pos, data, NULL);
}

/**
 * mate_icon_list_get_icon_data:
 * @gil: An icon list.
 * @pos: Index of an icon.
 *
 * Returns: The user data pointer associated to the icon at the index specified
 * by @pos.
 */
gpointer
mate_icon_list_get_icon_data (MateIconList *gil, int pos)
{
	Icon *icon;

	g_return_val_if_fail (gil != NULL, NULL);
	g_return_val_if_fail (IS_GIL (gil), NULL);
	g_return_val_if_fail (pos >= 0 && pos < gil->_priv->icons, NULL);

	icon = g_array_index (gil->_priv->icon_list, Icon*, pos);
	return icon->data;
}

/**
 * mate_icon_list_find_icon_from_data:
 * @gil:    An icon list.
 * @data:   Data pointer associated to an icon.
 *
 * Returns: The index of the icon whose user data has been set to @data,
 * or -1 if no icon has this data associated to it.
 */
int
mate_icon_list_find_icon_from_data (MateIconList *gil, gpointer data)
{
	MateIconListPrivate *priv;
	int n;
	Icon *icon;

	g_return_val_if_fail (gil != NULL, -1);
	g_return_val_if_fail (IS_GIL (gil), -1);

	priv = gil->_priv;

	for (n = 0; n < priv->icon_list->len; n++) {
		icon = g_array_index(priv->icon_list, Icon*, n);
		if (icon->data == data)
			return n;
	}

	return -1;
}

/* Sets an integer value in the private structure of the icon list, and updates it */
static void
set_value (MateIconList *gil, MateIconListPrivate *priv, int *dest, int val)
{
	if (val == *dest)
		return;

	*dest = val;

	if (!priv->frozen) {
		gil_layout_all_icons (gil);
		gil_scrollbar_adjust (gil);
	} else
		priv->dirty = TRUE;
}

/**
 * mate_icon_list_set_row_spacing:
 * @gil:    An icon list.
 * @pixels: Number of pixels for inter-row spacing.
 *
 * Sets the spacing to be used between rows of icons.
 */
void
mate_icon_list_set_row_spacing (MateIconList *gil, int pixels)
{
	MateIconListPrivate *priv;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));

	priv = gil->_priv;
	set_value (gil, priv, &priv->row_spacing, pixels);
}

/**
 * mate_icon_list_set_col_spacing:
 * @gil:    An icon list.
 * @pixels: Number of pixels for inter-column spacing.
 *
 * Sets the spacing to be used between columns of icons.
 */
void
mate_icon_list_set_col_spacing (MateIconList *gil, int pixels)
{
	MateIconListPrivate *priv;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));

	priv = gil->_priv;
	set_value (gil, priv, &priv->col_spacing, pixels);
}

/**
 * mate_icon_list_set_text_spacing:
 * @gil:    An icon list.
 * @pixels: Number of pixels between an icon's image and its caption.
 *
 * Sets the spacing to be used between an icon's image and its text caption.
 */
void
mate_icon_list_set_text_spacing (MateIconList *gil, int pixels)
{
	MateIconListPrivate *priv;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));

	priv = gil->_priv;
	set_value (gil, priv, &priv->text_spacing, pixels);
}

/**
 * mate_icon_list_set_icon_border:
 * @gil:    An icon list.
 * @pixels: Number of border pixels to be used around an icon's image.
 *
 * Sets the width of the border to be displayed around an icon's image.  This is
 * currently not implemented.
 */
void
mate_icon_list_set_icon_border (MateIconList *gil, int pixels)
{
	MateIconListPrivate *priv;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));

	priv = gil->_priv;
	set_value (gil, priv, &priv->icon_border, pixels);
}

/**
 * mate_icon_list_set_separators:
 * @gil: An icon list.
 * @sep: String with characters to be used as word separators.
 *
 * Sets the characters that can be used as word separators when doing
 * word-wrapping in the icon text captions.
 */
void
mate_icon_list_set_separators (MateIconList *gil, const char *sep)
{
	MateIconListPrivate *priv;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));
	g_return_if_fail (sep != NULL);

	priv = gil->_priv;

	g_free (priv->separators);
	priv->separators = g_strdup (sep);

	if (priv->frozen) {
		priv->dirty = TRUE;
		return;
	}

	gil_layout_all_icons (gil);
	gil_scrollbar_adjust (gil);
}

/**
 * mate_icon_list_moveto:
 * @gil:    An icon list.
 * @pos:    Index of an icon.
 * @yalign: Vertical alignment of the icon.
 *
 * Makes the icon whose index is @pos be visible on the screen.  The icon list
 * gets scrolled so that the icon is visible.  An alignment of 0.0 represents
 * the top of the visible part of the icon list, and 1.0 represents the bottom.
 * An icon can be centered on the icon list.
 */
void
mate_icon_list_moveto (MateIconList *gil, int pos, double yalign)
{
	MateIconListPrivate *priv;
	IconLine *il;
	GList *l;
	int i, y, uh, line;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));
	g_return_if_fail (pos >= 0 && pos < gil->_priv->icons);
	g_return_if_fail (yalign >= 0.0 && yalign <= 1.0);

	priv = gil->_priv;

	g_return_if_fail (priv->lines != NULL);

	line = pos / gil_get_items_per_line (gil);

	y = 0;
	for (i = 0, l = priv->lines; l && i < line; l = l->next, i++) {
		il = l->data;
		y += icon_line_height (gil, il);
	}

	il = l->data;

	uh = GTK_WIDGET (gil)->allocation.height - icon_line_height (gil,il);
	gtk_adjustment_set_value (gil->adj, y - uh * yalign);
}

/**
 * mate_icon_list_icon_is_visible:
 * @gil: An icon list.
 * @pos: Index of an icon.
 *
 * Returns: Whether the icon at the index specified by @pos is visible.  This
 * will be %GTK_VISIBILITY_NONE if the icon is not visible at all,
 * %GTK_VISIBILITY_PARTIAL if the icon is at least partially shown, and
 * %GTK_VISIBILITY_FULL if the icon is fully visible.
 */
GtkVisibility
mate_icon_list_icon_is_visible (MateIconList *gil, int pos)
{
	MateIconListPrivate *priv;
	IconLine *il;
	GList *l;
	int line, y1, y2, i;

	g_return_val_if_fail (gil != NULL, GTK_VISIBILITY_NONE);
	g_return_val_if_fail (IS_GIL (gil), GTK_VISIBILITY_NONE);
	g_return_val_if_fail (pos >= 0 && pos < gil->_priv->icons,
			      GTK_VISIBILITY_NONE);

	priv = gil->_priv;

	if (priv->lines == NULL)
		return GTK_VISIBILITY_NONE;

	line = pos / gil_get_items_per_line (gil);
	y1 = 0;
	for (i = 0, l = priv->lines; l && i < line; l = l->next, i++) {
		il = l->data;
		y1 += icon_line_height (gil, il);
	}
	y2 = y1 + icon_line_height (gil, (IconLine *) l->data);

	if (y2 < gil->adj->value)
		return GTK_VISIBILITY_NONE;

	if (y1 > gil->adj->value + GTK_WIDGET (gil)->allocation.height)
		return GTK_VISIBILITY_NONE;

	if (y2 <= gil->adj->value + GTK_WIDGET (gil)->allocation.height &&
	    y1 >= gil->adj->value)
		return GTK_VISIBILITY_FULL;

	return GTK_VISIBILITY_PARTIAL;
}

/**
 * mate_icon_list_get_icon_at:
 * @gil: An icon list.
 * @x:   X position in the icon list window.
 * @y:   Y position in the icon list window.
 *
 * Returns: The index of the icon that is under the specified coordinates, which
 * are relative to the icon list's window.  If there is no icon in that
 * position, -1 is returned.
 */
int
mate_icon_list_get_icon_at (MateIconList *gil, int x, int y)
{
	MateIconListPrivate *priv;
	double wx, wy;
	double dx, dy;
	int cx, cy;
	int n;
	MateCanvasItem *item;
	double dist;

	g_return_val_if_fail (gil != NULL, -1);
	g_return_val_if_fail (IS_GIL (gil), -1);

	priv = gil->_priv;

	dx = x;
	dy = y;

	mate_canvas_window_to_world (MATE_CANVAS (gil), dx, dy, &wx, &wy);
	mate_canvas_w2c (MATE_CANVAS (gil), wx, wy, &cx, &cy);

	for (n = 0; n < priv->icon_list->len; n++) {
		Icon *icon = g_array_index(priv->icon_list, Icon*, n);
		/* Note: this isn't the checking cast, because icon->image
		 * could be NULL */
		MateCanvasItem *image = (MateCanvasItem *) (icon->image);
		MateCanvasItem *text = MATE_CANVAS_ITEM (icon->text);

		if (image != NULL && wx >= image->x1 && wx <= image->x2 && wy >= image->y1 && wy <= image->y2) {
			dist = (* MATE_CANVAS_ITEM_GET_CLASS (image)->point) (
				image,
				wx, wy,
				cx, cy,
				&item);

			if ((int) (dist * MATE_CANVAS (gil)->pixels_per_unit + 0.5)
			    <= MATE_CANVAS (gil)->close_enough)
				return n;
		}

		if (wx >= text->x1 && wx <= text->x2 && wy >= text->y1 && wy <= text->y2) {
			dist = (* MATE_CANVAS_ITEM_GET_CLASS (text)->point) (
				text,
				wx, wy,
				cx, cy,
				&item);

			if ((int) (dist * MATE_CANVAS (gil)->pixels_per_unit + 0.5)
			    <= MATE_CANVAS (gil)->close_enough)
				return n;
		}
	}

	return -1;
}


/**
 * mate_icon_list_get_num_icons:
 * @gil: An icon list.
 *
 * Returns: The number of icons in the icon list.
 */
guint
mate_icon_list_get_num_icons (MateIconList *gil)
{
	g_return_val_if_fail (MATE_IS_ICON_LIST (gil), 0);

	return gil->_priv->icons;
}


/**
 * mate_icon_list_get_selection:
 * @gil: An icon list.
 *
 * Returns: A list of integers with the indices of the currently selected icons.
 */
GList *
mate_icon_list_get_selection (MateIconList *gil)
{
	g_return_val_if_fail (MATE_IS_ICON_LIST (gil), NULL);

	return gil->_priv->selection;
}


/**
 * mate_icon_list_get_icon_filename:
 * @gil: An icon list.
 * @idx: Index of an @icon.
 *
 * Returns: The filename of the icon with index @idx.
 */
gchar *
mate_icon_list_get_icon_filename (MateIconList *gil, int idx)
{
	Icon *icon;

	g_return_val_if_fail (gil != NULL, NULL);
	g_return_val_if_fail (IS_GIL (gil), NULL);
	g_return_val_if_fail (idx >= 0 && idx < gil->_priv->icons, NULL);

	icon = g_array_index (gil->_priv->icon_list, Icon*, idx);
	return icon->icon_filename;
}


/**
 * mate_icon_list_find_icon_from_filename:
 * @gil:       An icon list.
 * @filename:  Filename of an icon.
 *
 * Returns: The index of the icon whose filename is @filename or -1 if
 * there is no icon with this filename.
 */
int
mate_icon_list_find_icon_from_filename (MateIconList *gil,
					 const gchar *filename)
{
	MateIconListPrivate *priv;
	int n;
	Icon *icon;

	g_return_val_if_fail (gil != NULL, -1);
	g_return_val_if_fail (IS_GIL (gil), -1);
	g_return_val_if_fail (filename != NULL, -1);

	priv = gil->_priv;

	for (n = 0; n < priv->icon_list->len; n++) {
		icon = g_array_index(priv->icon_list, Icon*, n);
		if (!strcmp (icon->icon_filename, filename))
			return n;
	}

	return -1;
}


MateIconTextItem *
mate_icon_list_get_icon_text_item (MateIconList *gil,
				    int idx)
{
	Icon *icon;

	g_return_val_if_fail (gil != NULL, NULL);
	g_return_val_if_fail (IS_GIL (gil), NULL);

	if  (idx < 0 || idx >= gil->_priv->icons)
		return  NULL;

	icon = g_array_index (gil->_priv->icon_list, Icon*, idx);

	return icon->text;
}

MateCanvasPixbuf *
mate_icon_list_get_icon_pixbuf_item (MateIconList *gil,
				      int idx)
{
	Icon *icon;

	g_return_val_if_fail (gil != NULL, NULL);
	g_return_val_if_fail (IS_GIL (gil), NULL);

	if  (idx < 0 || idx >= gil->_priv->icons)
		return  NULL;

	icon = g_array_index (gil->_priv->icon_list, Icon*, idx);

	return icon->image;
}



/*** Accessible object for MateIconList ***/

static void selection_interface_init (AtkSelectionIface *iface);

static void gil_accessible_class_init (AtkObjectClass *class);

/* AtkObjectClass implementations */
static gint impl_get_n_children (AtkObject *accessible);
static AtkObject *impl_ref_child (AtkObject *accessible, gint i);
static void impl_initialize (AtkObject *accessible, gpointer data);

/* AtkSelectionIface implementations */
static gboolean impl_selection_add_selection (AtkSelection *selection, gint i);
static gboolean impl_selection_clear_selection (AtkSelection *selection);
static AtkObject* impl_selection_ref_selection (AtkSelection *selection, gint i);
static gint impl_selection_get_selection_count (AtkSelection *selection);
static gboolean impl_selection_is_child_selected (AtkSelection *selection, gint i);
static gboolean impl_selection_remove_selection (AtkSelection *selection, gint i);
static gboolean impl_selection_select_all_selection (AtkSelection *selection);

static gpointer accessible_parent_class = NULL;


/* get_type() function for the accessible object */
static GType
gil_accessible_get_type (void)
{
	static GType type;

	if (G_UNLIKELY (type == 0)) {
		const GInterfaceInfo selection_info = {
			(GInterfaceInitFunc) selection_interface_init,
			NULL, /* interface_finalize */
			NULL /* interface_data */
		};

		type = _accessibility_create_derived_type (
			"MateIconListAccessible",
			MATE_TYPE_CANVAS,
			gil_accessible_class_init);

		g_type_add_interface_static (type, ATK_TYPE_SELECTION, &selection_info);
	}

	return type;
}

/* Fills the ATK selection interface vtable */
static void
selection_interface_init (AtkSelectionIface *iface)
{
	iface->add_selection = impl_selection_add_selection;
	iface->clear_selection = impl_selection_clear_selection;
	iface->ref_selection = impl_selection_ref_selection;
	iface->get_selection_count = impl_selection_get_selection_count;
	iface->is_child_selected = impl_selection_is_child_selected;
	iface->remove_selection = impl_selection_remove_selection;
	iface->select_all_selection = impl_selection_select_all_selection;
}

/* Class initialization function for the accessible object */
static void
gil_accessible_class_init (AtkObjectClass *class)
{
	accessible_parent_class = g_type_class_peek_parent (class);

	class->get_n_children = impl_get_n_children;
	class->ref_child = impl_ref_child;
	class->initialize = impl_initialize;
}



/* AtkSelectionIface implementation */

static gboolean
impl_selection_add_selection (AtkSelection *selection, gint i)
{
	GtkWidget *widget;
	MateIconList *gil;

	widget = GTK_ACCESSIBLE (selection)->widget;
	if (!widget)
		return FALSE;

	gil = MATE_ICON_LIST (widget);
	mate_icon_list_select_icon (gil, i);
	return TRUE;
}

static gboolean
impl_selection_clear_selection (AtkSelection *selection)
{
	GtkWidget *widget;
	MateIconList *gil;

	widget = GTK_ACCESSIBLE (selection)->widget;
	if (!widget)
		return FALSE;

	gil = MATE_ICON_LIST (widget);
	mate_icon_list_unselect_all (gil);
	return TRUE;
}

static AtkObject *
impl_selection_ref_selection (AtkSelection *selection, gint i)
{
	GtkWidget *widget;
	MateIconList *gil;
	GList *sel, *l;
	int n;
	MateIconTextItem *iti;
	AtkObject *atk_object;

	widget = GTK_ACCESSIBLE (selection)->widget;
	if (!widget)
		return NULL;

	gil = MATE_ICON_LIST (widget);

	sel = mate_icon_list_get_selection (gil);
	l = g_list_nth (sel, i);
	if (!l)
		return NULL;

	n = GPOINTER_TO_INT (l->data);
	iti = mate_icon_list_get_icon_text_item (gil, n);
	if (!iti)
		return NULL;

	/* FIXME: is this what we need to return?  How do we distinguish between
	 * the icon text item and the pixbuf?
	 */
	atk_object = atk_gobject_accessible_for_object (G_OBJECT (iti));
	g_object_ref (atk_object);
	return atk_object;
}

static gint
impl_selection_get_selection_count (AtkSelection *selection)
{
	GtkWidget *widget;
	MateIconList *gil;
	GList *sel;

	widget = GTK_ACCESSIBLE (selection)->widget;
	if (!widget)
		return 0;

	gil = MATE_ICON_LIST (widget);
	sel = mate_icon_list_get_selection (gil);
	return g_list_length (sel);
}

static gboolean
impl_selection_is_child_selected (AtkSelection *selection, gint i)
{
	GtkWidget *widget;
	MateIconList *gil;
	GList *l;

	widget = GTK_ACCESSIBLE (selection)->widget;
	if (!widget)
		return FALSE;

	gil = MATE_ICON_LIST (widget);
	for (l = mate_icon_list_get_selection (gil); l; l = l->next) {
		int k;

		k = GPOINTER_TO_INT (l->data);
		if (k == i)
			return TRUE;
	}

	return FALSE;
}

static gboolean
impl_selection_remove_selection (AtkSelection *selection, gint i)
{
	GtkWidget *widget;
	MateIconList *gil;
	GList *sel, *l;
	int n;

	widget = GTK_ACCESSIBLE (selection)->widget;
	if (!widget)
		return FALSE;

	gil = MATE_ICON_LIST (widget);

	sel = mate_icon_list_get_selection (gil);
	l = g_list_nth (sel, i);
	if (!l)
		return FALSE;

	n = GPOINTER_TO_INT (l->data);

	mate_icon_list_unselect_icon (gil, n);
	return TRUE;
}

static gboolean
impl_selection_select_all_selection (AtkSelection *selection)
{
	GtkWidget *widget;
	MateIconList *gil;
	int n, i;

	widget = GTK_ACCESSIBLE (selection)->widget;
	if (!widget)
		return FALSE;

	gil = MATE_ICON_LIST (widget);

	if (mate_icon_list_get_selection_mode (gil) != GTK_SELECTION_MULTIPLE)
		return FALSE;

	n = mate_icon_list_get_num_icons (gil);

	for (i = 0; i < n; i++)
		mate_icon_list_select_icon (gil, i);

	return TRUE;
}



/* AtkObjectClass implementations */

static gint
impl_get_n_children (AtkObject *accessible)
{
	GtkWidget *widget;
	MateIconList *gil;

	widget = GTK_ACCESSIBLE (accessible)->widget;
	if (!widget)
		return 0;

	gil = MATE_ICON_LIST (widget);

	return mate_icon_list_get_num_icons (gil);
}

static AtkObject *
impl_ref_child (AtkObject *accessible, gint i)
{
	GtkWidget *widget;
	MateIconList *gil;
	MateIconTextItem *iti;
	AtkObject *atk_object;

	widget = GTK_ACCESSIBLE (accessible)->widget;
	if (!widget)
		return NULL;

	gil = MATE_ICON_LIST (widget);

	iti = mate_icon_list_get_icon_text_item (gil, i);
	if (!iti)
		return NULL;

	/* FIXME: is this what we need to return?  How do we distinguish between
	 * the icon text item and the pixbuf?
	 */
	atk_object = atk_gobject_accessible_for_object (G_OBJECT (iti));
	g_object_ref (atk_object);
	return atk_object;
}

/* Callback used when an icon changes its selection status */
static void
select_icon_cb (MateIconList *gil, gint num, GdkEvent *event, gpointer data)
{
	AtkObject *accessible;

	accessible = ATK_OBJECT (data);
	g_signal_emit_by_name (accessible, "selection_changed");
}

static void
impl_initialize (AtkObject *accessible, gpointer data)
{
	MateIconList *gil;

	/*
	 * We may have created an accessible object when gail is not loaded
	 */
        if (!GTK_IS_ACCESSIBLE (accessible))
		return;

	ATK_OBJECT_CLASS (accessible_parent_class)->initialize (accessible, data);

	gil = MATE_ICON_LIST (GTK_ACCESSIBLE (accessible)->widget);

	/* We use the same callback for both signals as the handler would do the
	 * same thing for both.
	 */
	g_signal_connect (gil, "select_icon", G_CALLBACK (select_icon_cb), accessible);
	g_signal_connect (gil, "unselect_icon", G_CALLBACK (select_icon_cb), accessible);
}

