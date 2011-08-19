#ifndef _MUX_CELL_RENDERER_TEXT
#define _MUX_CELL_RENDERER_TEXT

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MUX_TYPE_CELL_RENDERER_TEXT mux_cell_renderer_text_get_type()

#define MUX_CELL_RENDERER_TEXT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MUX_TYPE_CELL_RENDERER_TEXT, MuxCellRendererText))

#define MUX_CELL_RENDERER_TEXT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MUX_TYPE_CELL_RENDERER_TEXT, MuxCellRendererTextClass))

#define MUX_IS_CELL_RENDERER_TEXT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MUX_TYPE_CELL_RENDERER_TEXT))

#define MUX_IS_CELL_RENDERER_TEXT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MUX_TYPE_CELL_RENDERER_TEXT))

#define MUX_CELL_RENDERER_TEXT_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), MUX_TYPE_CELL_RENDERER_TEXT, MuxCellRendererTextClass))

typedef struct {
  GtkCellRendererText parent;
} MuxCellRendererText;

typedef struct {
  GtkCellRendererTextClass parent_class;
  void (*activated) (MuxCellRendererText *cell, const gchar *path);
} MuxCellRendererTextClass;

GType mux_cell_renderer_text_get_type (void);

GtkCellRenderer* mux_cell_renderer_text_new (void);

G_END_DECLS

#endif /* _MUX_CELL_RENDERER_TEXT */

