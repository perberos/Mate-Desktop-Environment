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

#include <config.h>
#include "koto-cell-renderer-pixbuf.h"

G_DEFINE_TYPE (KotoCellRendererPixbuf,
               koto_cell_renderer_pixbuf,
               GTK_TYPE_CELL_RENDERER_PIXBUF);

enum {
  ACTIVATED,
  LAST_SIGNAL,
};

static guint signals[LAST_SIGNAL] = { 0 };

/*
 * Utility function to determine if a point is inside a rectangle
 */
G_GNUC_CONST static gboolean
contains (GdkRectangle *rect, int x, int y)
{
  return (rect->x + rect->width) > x && rect->x <= x &&
    (rect->y + rect->height) > y && rect->y <= y;
}

static gboolean
koto_cell_renderer_pixbuf_activate (GtkCellRenderer *cell,
                                    GdkEvent *event, GtkWidget *widget,
                                    const gchar *path,
                                    GdkRectangle *background_area,
                                    GdkRectangle *cell_area,
                                    GtkCellRendererState flags)
{
  gdouble x, y;

  if (event && gdk_event_get_coords (event, &x, &y)) {
    if (contains (cell_area, (int)x, (int)y)) {
      g_signal_emit (cell, signals[ACTIVATED], 0, path);
      return TRUE;
    }
  }
  return FALSE;
}

/*
 * Object methods
 */

static void
koto_cell_renderer_pixbuf_class_init (KotoCellRendererPixbufClass *class)
{
  GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS (class);

  cell_class->activate = koto_cell_renderer_pixbuf_activate;

  signals[ACTIVATED] =
    g_signal_new ("activated",
                  G_OBJECT_CLASS_TYPE (class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (KotoCellRendererPixbufClass, activated),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1, G_TYPE_STRING);
}

static void
koto_cell_renderer_pixbuf_init (KotoCellRendererPixbuf *cellpixbuf)
{
  GTK_CELL_RENDERER (cellpixbuf)->mode = GTK_CELL_RENDERER_MODE_ACTIVATABLE;
}

/*
 * Public methods
 */

GtkCellRenderer *
koto_cell_renderer_pixbuf_new (void)
{
  return g_object_new (KOTO_TYPE_CELL_RENDERER_PIXBUF, NULL);
}
