/*
 * Copyright (C) 1998, 1999 Free Software Foundation
 * Copyright (C) 2000 Red Hat, Inc.
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

/* MateIconList widget - scrollable icon list
 *
 *
 * Authors:
 *   Federico Mena   <federico@nuclecu.unam.mx>
 *   Miguel de Icaza <miguel@nuclecu.unam.mx>
 */

#ifndef _MATE_ICON_LIST_H_
#define _MATE_ICON_LIST_H_

#ifndef MATE_DISABLE_DEPRECATED

#include <libmatecanvas/mate-canvas.h>
#include <libmatecanvas/mate-canvas-rich-text.h>
#include <libmatecanvas/mate-canvas-pixbuf.h>
#include <libmateui/mate-icon-item.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

#define MATE_TYPE_ICON_LIST            (mate_icon_list_get_type ())
#define MATE_ICON_LIST(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_ICON_LIST, MateIconList))
#define MATE_ICON_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_ICON_LIST, MateIconListClass))
#define MATE_IS_ICON_LIST(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_ICON_LIST))
#define MATE_IS_ICON_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_ICON_LIST))
#define MATE_ICON_LIST_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATE_TYPE_ICON_LIST, MateIconListClass))

typedef struct _MateIconList        MateIconList;
typedef struct _MateIconListPrivate MateIconListPrivate;
typedef struct _MateIconListClass   MateIconListClass;

typedef enum {
	MATE_ICON_LIST_ICONS,
	MATE_ICON_LIST_TEXT_BELOW,
	MATE_ICON_LIST_TEXT_RIGHT
} MateIconListMode;

/* This structure has been converted to use public and private parts.  To avoid
 * breaking binary compatibility, the slots for private fields have been
 * replaced with padding.  Please remove these fields when mate-libs has
 * reached another major version and it is "fine" to break binary compatibility.
 */
struct _MateIconList {
	MateCanvas canvas;

	/* Scroll adjustments */
	GtkAdjustment *adj;
	GtkAdjustment *hadj;
	
	/*< private >*/
	MateIconListPrivate * _priv;
};

struct _MateIconListClass {
	MateCanvasClass parent_class;

	void     (*select_icon)      (MateIconList *gil, gint num, GdkEvent *event);
	void     (*unselect_icon)    (MateIconList *gil, gint num, GdkEvent *event);
	void     (*focus_icon)       (MateIconList *gil, gint num);
	gboolean (*text_changed)     (MateIconList *gil, gint num, const char *new_text);

	/* Key Binding signals */
	void     (*move_cursor)      (MateIconList *gil, GtkDirectionType dir, gboolean clear_selection);
	void     (*toggle_cursor_selection) (MateIconList *gil);
	void     (*unused)       (MateIconList *unused);
	
	/* Padding for possible expansion */
	gpointer padding1;
	gpointer padding2;
};

enum {
	MATE_ICON_LIST_IS_EDITABLE	= 1 << 0,
	MATE_ICON_LIST_STATIC_TEXT	= 1 << 1
};

GType          mate_icon_list_get_type            (void) G_GNUC_CONST;

GtkWidget     *mate_icon_list_new                 (guint         icon_width,
						    GtkAdjustment *adj,
						    int           flags);

void           mate_icon_list_construct           (MateIconList *gil,
						    guint icon_width,
						    GtkAdjustment *adj,
						    int flags);

void           mate_icon_list_set_hadjustment    (MateIconList *gil,
						   GtkAdjustment *hadj);

void           mate_icon_list_set_vadjustment    (MateIconList *gil,
						   GtkAdjustment *vadj);

/* To avoid excesive recomputes during insertion/deletion */
void           mate_icon_list_freeze              (MateIconList *gil);
void           mate_icon_list_thaw                (MateIconList *gil);


void           mate_icon_list_insert              (MateIconList *gil,
						    int pos,
						    const char *icon_filename,
						    const char *text);
void           mate_icon_list_insert_pixbuf       (MateIconList *gil,
						    int pos,
						    GdkPixbuf *im,
						    const char *icon_filename,
						    const char *text);

int            mate_icon_list_append              (MateIconList *gil,
						    const char *icon_filename,
						    const char *text);
int            mate_icon_list_append_pixbuf       (MateIconList *gil,
						    GdkPixbuf *im,
						    const char *icon_filename,
						    const char *text);

void           mate_icon_list_clear               (MateIconList *gil);
void           mate_icon_list_remove              (MateIconList *gil,
						    int pos);

guint          mate_icon_list_get_num_icons       (MateIconList *gil);


/* Managing the selection */
GtkSelectionMode mate_icon_list_get_selection_mode(MateIconList *gil);
void           mate_icon_list_set_selection_mode  (MateIconList *gil,
						    GtkSelectionMode mode);
void           mate_icon_list_select_icon         (MateIconList *gil,
						    int pos);
void           mate_icon_list_unselect_icon       (MateIconList *gil,
						    int pos);
void           mate_icon_list_select_all          (MateIconList *gil);
int            mate_icon_list_unselect_all        (MateIconList *gil);
GList *        mate_icon_list_get_selection       (MateIconList *gil);

/* Managing focus */
void           mate_icon_list_focus_icon          (MateIconList *gil,
						    gint idx);

/* Setting the spacing values */
void           mate_icon_list_set_icon_width      (MateIconList *gil,
						    int w);
void           mate_icon_list_set_row_spacing     (MateIconList *gil,
						    int pixels);
void           mate_icon_list_set_col_spacing     (MateIconList *gil,
						    int pixels);
void           mate_icon_list_set_text_spacing    (MateIconList *gil,
						    int pixels);
void           mate_icon_list_set_icon_border     (MateIconList *gil,
						    int pixels);
void           mate_icon_list_set_separators      (MateIconList *gil,
						    const char *sep);
/* Icon filename. */
gchar *        mate_icon_list_get_icon_filename   (MateIconList *gil,
						    int idx);
int            mate_icon_list_find_icon_from_filename (MateIconList *gil,
							const char *filename);

/* Attaching information to the icons */
void           mate_icon_list_set_icon_data       (MateIconList *gil,
						    int idx, gpointer data);
void           mate_icon_list_set_icon_data_full  (MateIconList *gil,
						    int pos, gpointer data,
						    GDestroyNotify destroy);
int            mate_icon_list_find_icon_from_data (MateIconList *gil,
						    gpointer data);
gpointer       mate_icon_list_get_icon_data       (MateIconList *gil,
						    int pos);

/* Visibility */
void           mate_icon_list_moveto              (MateIconList *gil,
						    int pos, double yalign);
GtkVisibility  mate_icon_list_icon_is_visible     (MateIconList *gil,
						    int pos);

int            mate_icon_list_get_icon_at         (MateIconList *gil,
						    int x, int y);

int            mate_icon_list_get_items_per_line  (MateIconList *gil);

/* Accessibility functions */
MateIconTextItem *mate_icon_list_get_icon_text_item (MateIconList *gil,
						       int idx);

MateCanvasPixbuf *mate_icon_list_get_icon_pixbuf_item (MateIconList *gil,
							 int idx);

G_END_DECLS

#endif /* MATE_DISABLE_DEPRECATED */

#endif /* _MATE_ICON_LIST_H_ */

