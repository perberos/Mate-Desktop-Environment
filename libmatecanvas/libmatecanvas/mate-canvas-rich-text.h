/* Editable MateCanvas text item based on GtkTextLayout, borrowed heavily
 * from GtkTextView.
 *
 * Copyright (c) 2000 Red Hat, Inc.
 * Copyright (c) 2001 Joe Shaw
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

#ifndef MATE_CANVAS_RICH_TEXT_H
#define MATE_CANVAS_RICH_TEXT_H

#include <libmatecanvas/mate-canvas.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MATE_TYPE_CANVAS_RICH_TEXT             (mate_canvas_rich_text_get_type ())
#define MATE_CANVAS_RICH_TEXT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_CANVAS_RICH_TEXT, MateCanvasRichText))
#define MATE_CANVAS_RICH_TEXT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_CANVAS_RICH_TEXT, MateCanvasRichTextClass))
#define MATE_IS_CANVAS_RICH_TEXT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_CANVAS_RICH_TEXT))
#define MATE_IS_CANVAS_RICH_TEXT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_CANVAS_RICH_TEXT))
#define MATE_CANVAS_RICH_TEXT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), MATE_TYPE_CANVAS_RICH_TEXT, MateCanvasRichTextClass))

typedef struct _MateCanvasRichText             MateCanvasRichText;
typedef struct _MateCanvasRichTextPrivate      MateCanvasRichTextPrivate;
typedef struct _MateCanvasRichTextClass        MateCanvasRichTextClass;

struct _MateCanvasRichText {
	MateCanvasItem item;

    MateCanvasRichTextPrivate *_priv;
};

struct _MateCanvasRichTextClass {
	MateCanvasItemClass parent_class;

	void (* tag_changed)(MateCanvasRichText *text,
			     GtkTextTag *tag);
};

GType mate_canvas_rich_text_get_type(void) G_GNUC_CONST;

void mate_canvas_rich_text_cut_clipboard(MateCanvasRichText *text);

void mate_canvas_rich_text_copy_clipboard(MateCanvasRichText *text);

void mate_canvas_rich_text_paste_clipboard(MateCanvasRichText *text);

void mate_canvas_rich_text_set_buffer(MateCanvasRichText *text,
				       GtkTextBuffer *buffer);

GtkTextBuffer *mate_canvas_rich_text_get_buffer(MateCanvasRichText *text);
void
mate_canvas_rich_text_get_iter_location (MateCanvasRichText *text,
					  const GtkTextIter *iter,
					  GdkRectangle      *location);
void
mate_canvas_rich_text_get_iter_at_location (MateCanvasRichText *text,
                                    GtkTextIter *iter,
                                    gint         x,
					     gint         y);

G_END_DECLS

#endif /* MATE_CANVAS_RICH_TEXT_H */
