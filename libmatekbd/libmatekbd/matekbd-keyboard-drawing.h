/*
 * Copyright (C) 2006 Sergey V. Udaltsov <svu@mate.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef MATEKBD_KEYBOARD_DRAWING_H
#define MATEKBD_KEYBOARD_DRAWING_H 1

#include <gtk/gtk.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKBgeom.h>

G_BEGIN_DECLS
#define MATEKBD_KEYBOARD_DRAWING(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), matekbd_keyboard_drawing_get_type (), \
                               MatekbdKeyboardDrawing))
#define MATEKBD_KEYBOARD_DRAWING_CLASS(clazz) (G_TYPE_CHECK_CLASS_CAST ((clazz), matekbd_keyboard_drawing_get_type () \
                                       MatekbdKeyboardDrawingClass))
#define MATEKBD_IS_KEYBOARD_DRAWING(obj) G_TYPE_CHECK_INSTANCE_TYPE ((obj), matekbd_keyboard_drawing_get_type ())
typedef struct _MatekbdKeyboardDrawing MatekbdKeyboardDrawing;
typedef struct _MatekbdKeyboardDrawingClass MatekbdKeyboardDrawingClass;

typedef struct _MatekbdKeyboardDrawingItem MatekbdKeyboardDrawingItem;
typedef struct _MatekbdKeyboardDrawingKey MatekbdKeyboardDrawingKey;
typedef struct _MatekbdKeyboardDrawingDoodad MatekbdKeyboardDrawingDoodad;
typedef struct _MatekbdKeyboardDrawingGroupLevel
 MatekbdKeyboardDrawingGroupLevel;
typedef struct _MatekbdKeyboardDrawingRenderContext
 MatekbdKeyboardDrawingRenderContext;

typedef enum {
	MATEKBD_KEYBOARD_DRAWING_ITEM_TYPE_INVALID = 0,
	MATEKBD_KEYBOARD_DRAWING_ITEM_TYPE_KEY,
	MATEKBD_KEYBOARD_DRAWING_ITEM_TYPE_KEY_EXTRA,
	MATEKBD_KEYBOARD_DRAWING_ITEM_TYPE_DOODAD
} MatekbdKeyboardDrawingItemType;

typedef enum {
	MATEKBD_KEYBOARD_DRAWING_POS_TOPLEFT,
	MATEKBD_KEYBOARD_DRAWING_POS_TOPRIGHT,
	MATEKBD_KEYBOARD_DRAWING_POS_BOTTOMLEFT,
	MATEKBD_KEYBOARD_DRAWING_POS_BOTTOMRIGHT,
	MATEKBD_KEYBOARD_DRAWING_POS_TOTAL,
	MATEKBD_KEYBOARD_DRAWING_POS_FIRST =
	    MATEKBD_KEYBOARD_DRAWING_POS_TOPLEFT,
	MATEKBD_KEYBOARD_DRAWING_POS_LAST =
	    MATEKBD_KEYBOARD_DRAWING_POS_BOTTOMRIGHT
} MatekbdKeyboardDrawingGroupLevelPosition;

/* units are in xkb form */
struct _MatekbdKeyboardDrawingItem {
	/*< private > */

	MatekbdKeyboardDrawingItemType type;
	gint origin_x;
	gint origin_y;
	gint angle;
	guint priority;
};

/* units are in xkb form */
struct _MatekbdKeyboardDrawingKey {
	/*< private > */

	MatekbdKeyboardDrawingItemType type;
	gint origin_x;
	gint origin_y;
	gint angle;
	guint priority;

	XkbKeyRec *xkbkey;
	gboolean pressed;
	guint keycode;
};

/* units are in xkb form */
struct _MatekbdKeyboardDrawingDoodad {
	/*< private > */

	MatekbdKeyboardDrawingItemType type;
	gint origin_x;
	gint origin_y;
	gint angle;
	guint priority;

	XkbDoodadRec *doodad;
	gboolean on;		/* for indicator doodads */
};

struct _MatekbdKeyboardDrawingGroupLevel {
	gint group;
	gint level;
};

struct _MatekbdKeyboardDrawingRenderContext {
	cairo_t *cr;

	gint angle;		/* current angle pango is set to draw at, in tenths of a degree */
	PangoLayout *layout;
	PangoFontDescription *font_desc;

	gint scale_numerator;
	gint scale_denominator;

	GdkColor *dark_color;
};

struct _MatekbdKeyboardDrawing {
	/*< private > */

	GtkDrawingArea parent;

	GdkPixmap *pixmap;
	XkbDescRec *xkb;
	gboolean xkbOnDisplay;
	guint l3mod;

	MatekbdKeyboardDrawingRenderContext *renderContext;

	/* Indexed by keycode */
	MatekbdKeyboardDrawingKey *keys;

	/* list of stuff to draw in priority order */
	GList *keyboard_items;

	GdkColor *colors;

	guint timeout;
	guint idle_redraw;

	MatekbdKeyboardDrawingGroupLevel **groupLevels;

	guint mods;

	Display *display;
	gint screen_num;

	gint xkb_event_type;

	MatekbdKeyboardDrawingDoodad **physical_indicators;
	gint physical_indicators_size;

	guint track_config:1;
	guint track_modifiers:1;
};

struct _MatekbdKeyboardDrawingClass {
	GtkDrawingAreaClass parent_class;

	/* we send this signal when the user presses a key that "doesn't exist"
	 * according to the keyboard geometry; it probably means their xkb
	 * configuration is incorrect */
	void (*bad_keycode) (MatekbdKeyboardDrawing * drawing, guint keycode);
};

GType matekbd_keyboard_drawing_get_type (void);
GtkWidget *matekbd_keyboard_drawing_new (void);

GdkPixbuf *matekbd_keyboard_drawing_get_pixbuf (MatekbdKeyboardDrawing *
					     kbdrawing);
gboolean matekbd_keyboard_drawing_render (MatekbdKeyboardDrawing * kbdrawing,
				       cairo_t * cr,
				       PangoLayout * layout,
				       double x, double y,
				       double width, double height,
				       gdouble dpi_x, gdouble dpi_y);
gboolean matekbd_keyboard_drawing_set_keyboard (MatekbdKeyboardDrawing *
					     kbdrawing,
					     XkbComponentNamesRec * names);

const gchar* matekbd_keyboard_drawing_get_keycodes(MatekbdKeyboardDrawing* kbdrawing);
const gchar* matekbd_keyboard_drawing_get_geometry(MatekbdKeyboardDrawing* kbdrawing);
const gchar* matekbd_keyboard_drawing_get_symbols(MatekbdKeyboardDrawing* kbdrawing);
const gchar* matekbd_keyboard_drawing_get_types(MatekbdKeyboardDrawing* kbdrawing);
const gchar* matekbd_keyboard_drawing_get_compat(MatekbdKeyboardDrawing* kbdrawing);

void matekbd_keyboard_drawing_set_track_modifiers (MatekbdKeyboardDrawing *
						kbdrawing,
						gboolean enable);
void matekbd_keyboard_drawing_set_track_config (MatekbdKeyboardDrawing *
					     kbdrawing, gboolean enable);

void matekbd_keyboard_drawing_set_groups_levels (MatekbdKeyboardDrawing *
					      kbdrawing,
					      MatekbdKeyboardDrawingGroupLevel
					      * groupLevels[]);


void matekbd_keyboard_drawing_print (MatekbdKeyboardDrawing * drawing,
				  GtkWindow * parent_window,
				  const gchar * description);


GtkWidget* matekbd_keyboard_drawing_new_dialog (gint group, gchar* group_name);

G_END_DECLS
#endif				/* #ifndef MATEKBD_KEYBOARD_DRAWING_H */
