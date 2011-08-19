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

#ifndef GKBD_KEYBOARD_DRAWING_H
#define GKBD_KEYBOARD_DRAWING_H 1

#include <gtk/gtk.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKBgeom.h>

G_BEGIN_DECLS
#define GKBD_KEYBOARD_DRAWING(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), gkbd_keyboard_drawing_get_type (), \
                               GkbdKeyboardDrawing))
#define GKBD_KEYBOARD_DRAWING_CLASS(clazz) (G_TYPE_CHECK_CLASS_CAST ((clazz), gkbd_keyboard_drawing_get_type () \
                                       GkbdKeyboardDrawingClass))
#define GKBD_IS_KEYBOARD_DRAWING(obj) G_TYPE_CHECK_INSTANCE_TYPE ((obj), gkbd_keyboard_drawing_get_type ())
typedef struct _GkbdKeyboardDrawing GkbdKeyboardDrawing;
typedef struct _GkbdKeyboardDrawingClass GkbdKeyboardDrawingClass;

typedef struct _GkbdKeyboardDrawingItem GkbdKeyboardDrawingItem;
typedef struct _GkbdKeyboardDrawingKey GkbdKeyboardDrawingKey;
typedef struct _GkbdKeyboardDrawingDoodad GkbdKeyboardDrawingDoodad;
typedef struct _GkbdKeyboardDrawingGroupLevel
 GkbdKeyboardDrawingGroupLevel;
typedef struct _GkbdKeyboardDrawingRenderContext
 GkbdKeyboardDrawingRenderContext;

typedef enum {
	GKBD_KEYBOARD_DRAWING_ITEM_TYPE_INVALID = 0,
	GKBD_KEYBOARD_DRAWING_ITEM_TYPE_KEY,
	GKBD_KEYBOARD_DRAWING_ITEM_TYPE_KEY_EXTRA,
	GKBD_KEYBOARD_DRAWING_ITEM_TYPE_DOODAD
} GkbdKeyboardDrawingItemType;

typedef enum {
	GKBD_KEYBOARD_DRAWING_POS_TOPLEFT,
	GKBD_KEYBOARD_DRAWING_POS_TOPRIGHT,
	GKBD_KEYBOARD_DRAWING_POS_BOTTOMLEFT,
	GKBD_KEYBOARD_DRAWING_POS_BOTTOMRIGHT,
	GKBD_KEYBOARD_DRAWING_POS_TOTAL,
	GKBD_KEYBOARD_DRAWING_POS_FIRST =
	    GKBD_KEYBOARD_DRAWING_POS_TOPLEFT,
	GKBD_KEYBOARD_DRAWING_POS_LAST =
	    GKBD_KEYBOARD_DRAWING_POS_BOTTOMRIGHT
} GkbdKeyboardDrawingGroupLevelPosition;

/* units are in xkb form */
struct _GkbdKeyboardDrawingItem {
	/*< private > */

	GkbdKeyboardDrawingItemType type;
	gint origin_x;
	gint origin_y;
	gint angle;
	guint priority;
};

/* units are in xkb form */
struct _GkbdKeyboardDrawingKey {
	/*< private > */

	GkbdKeyboardDrawingItemType type;
	gint origin_x;
	gint origin_y;
	gint angle;
	guint priority;

	XkbKeyRec *xkbkey;
	gboolean pressed;
	guint keycode;
};

/* units are in xkb form */
struct _GkbdKeyboardDrawingDoodad {
	/*< private > */

	GkbdKeyboardDrawingItemType type;
	gint origin_x;
	gint origin_y;
	gint angle;
	guint priority;

	XkbDoodadRec *doodad;
	gboolean on;		/* for indicator doodads */
};

struct _GkbdKeyboardDrawingGroupLevel {
	gint group;
	gint level;
};

struct _GkbdKeyboardDrawingRenderContext {
	cairo_t *cr;

	gint angle;		/* current angle pango is set to draw at, in tenths of a degree */
	PangoLayout *layout;
	PangoFontDescription *font_desc;

	gint scale_numerator;
	gint scale_denominator;

	GdkColor *dark_color;
};

struct _GkbdKeyboardDrawing {
	/*< private > */

	GtkDrawingArea parent;

	GdkPixmap *pixmap;
	XkbDescRec *xkb;
	gboolean xkbOnDisplay;
	guint l3mod;

	GkbdKeyboardDrawingRenderContext *renderContext;

	/* Indexed by keycode */
	GkbdKeyboardDrawingKey *keys;

	/* list of stuff to draw in priority order */
	GList *keyboard_items;

	GdkColor *colors;

	guint timeout;
	guint idle_redraw;

	GkbdKeyboardDrawingGroupLevel **groupLevels;

	guint mods;

	Display *display;
	gint screen_num;

	gint xkb_event_type;

	GkbdKeyboardDrawingDoodad **physical_indicators;
	gint physical_indicators_size;

	guint track_config:1;
	guint track_modifiers:1;
};

struct _GkbdKeyboardDrawingClass {
	GtkDrawingAreaClass parent_class;

	/* we send this signal when the user presses a key that "doesn't exist"
	 * according to the keyboard geometry; it probably means their xkb
	 * configuration is incorrect */
	void (*bad_keycode) (GkbdKeyboardDrawing * drawing, guint keycode);
};

GType gkbd_keyboard_drawing_get_type (void);
GtkWidget *gkbd_keyboard_drawing_new (void);

GdkPixbuf *gkbd_keyboard_drawing_get_pixbuf (GkbdKeyboardDrawing *
					     kbdrawing);
gboolean gkbd_keyboard_drawing_render (GkbdKeyboardDrawing * kbdrawing,
				       cairo_t * cr,
				       PangoLayout * layout,
				       double x, double y,
				       double width, double height,
				       gdouble dpi_x, gdouble dpi_y);
gboolean gkbd_keyboard_drawing_set_keyboard (GkbdKeyboardDrawing *
					     kbdrawing,
					     XkbComponentNamesRec * names);

G_CONST_RETURN gchar
    * gkbd_keyboard_drawing_get_keycodes (GkbdKeyboardDrawing * kbdrawing);
G_CONST_RETURN gchar
    * gkbd_keyboard_drawing_get_geometry (GkbdKeyboardDrawing * kbdrawing);
G_CONST_RETURN gchar
    * gkbd_keyboard_drawing_get_symbols (GkbdKeyboardDrawing * kbdrawing);
G_CONST_RETURN gchar *gkbd_keyboard_drawing_get_types (GkbdKeyboardDrawing
						       * kbdrawing);
G_CONST_RETURN gchar *gkbd_keyboard_drawing_get_compat (GkbdKeyboardDrawing
							* kbdrawing);

void gkbd_keyboard_drawing_set_track_modifiers (GkbdKeyboardDrawing *
						kbdrawing,
						gboolean enable);
void gkbd_keyboard_drawing_set_track_config (GkbdKeyboardDrawing *
					     kbdrawing, gboolean enable);

void gkbd_keyboard_drawing_set_groups_levels (GkbdKeyboardDrawing *
					      kbdrawing,
					      GkbdKeyboardDrawingGroupLevel
					      * groupLevels[]);


void gkbd_keyboard_drawing_print (GkbdKeyboardDrawing * drawing,
				  GtkWindow * parent_window,
				  const gchar * description);


GtkWidget* gkbd_keyboard_drawing_new_dialog (gint group, gchar* group_name);

G_END_DECLS
#endif				/* #ifndef GKBD_KEYBOARD_DRAWING_H */
