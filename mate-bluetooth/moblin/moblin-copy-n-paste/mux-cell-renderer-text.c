#include "mux-cell-renderer-text.h"

G_DEFINE_TYPE (MuxCellRendererText,
	       mux_cell_renderer_text,
	       GTK_TYPE_CELL_RENDERER_TEXT)

enum {
  ACTIVATED,
  LAST_SIGNAL,
};

static guint signals[LAST_SIGNAL] = { 0 };

static gboolean
contains (GdkRectangle *rect, gint x, gint y)
{
  return (rect->x + rect->width) > x && rect->x <= x &&
  	 (rect->y + rect->height) > y && rect->y <= y;
}

static gboolean
mux_cell_renderer_text_activate (GtkCellRenderer     *cell,
				 GdkEvent            *event,
				 GtkWidget           *widget,
				 const gchar         *path,
				 GdkRectangle        *bg_area,
				 GdkRectangle        *cell_area,
				 GtkCellRendererState flags)
{
  gdouble x, y;

  if (event) {
    gdk_event_get_coords (event, &x, &y);
    if (contains (cell_area, (gint) x, (gint) y)) {
      g_signal_emit (cell, signals[ACTIVATED], 0, path);

      return TRUE;
    }
  }

  return FALSE;
}

static void
mux_cell_renderer_text_render (GtkCellRenderer     *cell,
			       GdkDrawable         *window,
			       GtkWidget           *widget,
			       GdkRectangle        *bg_area,
			       GdkRectangle        *cell_area,
			       GdkRectangle        *expose_area,
			       GtkCellRendererState flags)
{
  GTK_CELL_RENDERER_CLASS (mux_cell_renderer_text_parent_class)->render (cell,
                                                                         window,
                                                                         widget,
                                                                         bg_area,
                                                                         cell_area,
                                                                         expose_area,
                                                                         GTK_CELL_RENDERER_SELECTED);
}


static void
mux_cell_renderer_text_class_init (MuxCellRendererTextClass *klass)
{
  GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS (klass);

  cell_class->activate = mux_cell_renderer_text_activate;
  cell_class->render = mux_cell_renderer_text_render;

  signals[ACTIVATED] =
    g_signal_new ("activated",
    		  G_OBJECT_CLASS_TYPE (klass),
    		  G_SIGNAL_RUN_LAST,
    		  G_STRUCT_OFFSET (MuxCellRendererTextClass, activated),
    		  NULL,
    		  NULL,
    		  g_cclosure_marshal_VOID__STRING,
    		  G_TYPE_NONE, 1, G_TYPE_STRING);
}

static void
mux_cell_renderer_text_init (MuxCellRendererText *self)
{
  GTK_CELL_RENDERER (self)->mode = GTK_CELL_RENDERER_MODE_ACTIVATABLE;
}

GtkCellRenderer*
mux_cell_renderer_text_new (void)
{
  return g_object_new (MUX_TYPE_CELL_RENDERER_TEXT, NULL);
}
