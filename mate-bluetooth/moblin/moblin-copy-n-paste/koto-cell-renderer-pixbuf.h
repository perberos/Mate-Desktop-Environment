/*
 * Copyright (C) 2007 OpenedHand Ltd
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef __KOTO_CELL_RENDERER_PIXBUF_H__
#define __KOTO_CELL_RENDERER_PIXBUF_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define KOTO_TYPE_CELL_RENDERER_PIXBUF			(koto_cell_renderer_pixbuf_get_type ())
#define KOTO_CELL_RENDERER_PIXBUF(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), KOTO_TYPE_CELL_RENDERER_PIXBUF, KotoCellRendererPixbuf))
#define KOTO_CELL_RENDERER_PIXBUF_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), KOTO_TYPE_CELL_RENDERER_PIXBUF, KotoCellRendererPixbufClass))
#define KOTO_IS_CELL_RENDERER_PIXBUF(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), KOTO_TYPE_CELL_RENDERER_PIXBUF))
#define KOTO_IS_CELL_RENDERER_PIXBUF_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), KOTO_TYPE_CELL_RENDERER_PIXBUF))
#define KOTO_CELL_RENDERER_PIXBUF_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((obj), KOTO_TYPE_CELL_RENDERER_PIXBUF, KotoCellRendererPixbufClass))

typedef struct _KotoCellRendererPixbuf KotoCellRendererPixbuf;
typedef struct _KotoCellRendererPixbufClass KotoCellRendererPixbufClass;

struct _KotoCellRendererPixbuf
{
  GtkCellRendererPixbuf parent;
};

struct _KotoCellRendererPixbufClass
{
  GtkCellRendererPixbufClass parent_class;

  void (* activated) (KotoCellRendererPixbuf *cell, const char *path);
};

GType koto_cell_renderer_pixbuf_get_type (void) G_GNUC_CONST;
GtkCellRenderer *koto_cell_renderer_pixbuf_new (void);

G_END_DECLS

#endif /* __KOTO_CELL_RENDERER_PIXBUF_H__ */
