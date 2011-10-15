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

#ifndef _MATE_ICON_TEXT_ITEM_H_
#define _MATE_ICON_TEXT_ITEM_H_

#ifndef MATE_DISABLE_DEPRECATED

#include <libmatecanvas/mate-canvas.h>
#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _MateIconTextItem        MateIconTextItem;
typedef struct _MateIconTextItemClass   MateIconTextItemClass;
typedef struct _MateIconTextItemPrivate MateIconTextItemPrivate;

#define MATE_TYPE_ICON_TEXT_ITEM            (mate_icon_text_item_get_type ())
#define MATE_ICON_TEXT_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_ICON_TEXT_ITEM, MateIconTextItem))
#define MATE_ICON_TEXT_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_ICON_TEXT_ITEM, MateIconTextItemClass))
#define MATE_IS_ICON_TEXT_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_ICON_TEXT_ITEM))
#define MATE_IS_ICON_TEXT_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_ICON_TEXT_ITEM))
#define MATE_ICON_TEXT_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATE_TYPE_ICON_TEXT_ITEM, MateIconTextItemClass))

struct _MateIconTextItem {
	MateCanvasItem parent_instance;

	/* Size and maximum allowed width */
	int x, y;
	int width;

	/* Font name */
	char *fontname;

	/* Actual text */
	char *text;

	/* Whether the text is being edited */
	unsigned int editing : 1;

	/* Whether the text item is selected */
	unsigned int selected : 1;

	/* Whether the text item is focused */
	unsigned int focused : 1;

	/* Whether the text is editable */
	unsigned int is_editable : 1;

	/* Whether the text is allocated by us (FALSE if allocated by the client) */
	unsigned int is_text_allocated : 1;

	MateIconTextItemPrivate *_priv;
};

struct _MateIconTextItemClass {
	MateCanvasItemClass parent_class;

	/* Signals we emit */
	gboolean  (* text_changed)      (MateIconTextItem *iti);
	void (* height_changed)    (MateIconTextItem *iti);
	void (* width_changed)     (MateIconTextItem *iti);
	void (* editing_started)   (MateIconTextItem *iti);
	void (* editing_stopped)   (MateIconTextItem *iti);
	void (* selection_started) (MateIconTextItem *iti);
	void (* selection_stopped) (MateIconTextItem *iti);

	/* Virtual functions */
	GtkEntry* (* create_entry)  (MateIconTextItem *iti);

        /* Padding for possible expansion */
	gpointer padding1;
};

GType        mate_icon_text_item_get_type      (void) G_GNUC_CONST;

void         mate_icon_text_item_configure     (MateIconTextItem *iti,
						 int                x,
						 int                y,
						 int                width,
						 const char        *fontname,
						 const char        *text,
						 gboolean           is_editable,
						 gboolean           is_static);

void         mate_icon_text_item_setxy         (MateIconTextItem *iti,
						 int                x,
						 int                y);

void         mate_icon_text_item_select        (MateIconTextItem *iti,
						 gboolean                sel);

void         mate_icon_text_item_focus         (MateIconTextItem *iti,
						 gboolean                focused);

const char  *mate_icon_text_item_get_text      (MateIconTextItem *iti);

void         mate_icon_text_item_start_editing (MateIconTextItem *iti);

void         mate_icon_text_item_stop_editing  (MateIconTextItem *iti,
						 gboolean           accept);

GtkEditable *mate_icon_text_item_get_editable  (MateIconTextItem *iti);

#ifdef __cplusplus
}
#endif

#endif /* MATE_DISABLE_DEPRECATED */

#endif /* _MATE_ICON_TEXT_ITEM_H_ */
