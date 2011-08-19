/*
 * Copyright © 2004 Noah Levitt
 * Copyright © 2007, 2008 Christian Persch
 *
 * Some code copied from gtk+/gtk/gtkiconview:
 * Copyright © 2002, 2004  Anders Carlsson <andersca@gnu.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02110-1301  USA
 */

#include <config.h>

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "gucharmap-marshal.h"
#include "gucharmap-chartable.h"
#include "gucharmap-unicode-info.h"
#include "gucharmap-private.h"

#define ENABLE_ACCESSIBLE

#ifdef ENABLE_ACCESSIBLE
#include "gucharmap-chartable-accessible.h"
#endif

enum
{
  ACTIVATE,
  STATUS_MESSAGE,
  MOVE_CURSOR,
  COPY_CLIPBOARD,
  PASTE_CLIPBOARD,
  NUM_SIGNALS
};

enum
{
  PROP_0,
  PROP_ACTIVE_CHAR,
  PROP_CODEPOINT_LIST,
  PROP_FONT_DESC,
  PROP_SNAP_POW2,
  PROP_ZOOM_ENABLED,
  PROP_ZOOM_SHOWING
};

static void gucharmap_chartable_class_init (GucharmapChartableClass *klass);
static void gucharmap_chartable_finalize   (GObject *object);

static guint signals[NUM_SIGNALS];

#define DEFAULT_FONT_SIZE (20.0 * (double) PANGO_SCALE)

/* These are chosen for compatibility with the older code that
 * didn't scale the font size by resolution and used 3 and 2.5 here, resp.
 * Where exactly these factors came from, I don't know.
 */
#define FACTOR_WIDTH (2.25) /* 3 / (96 / 72) */
#define FACTOR_HEIGHT (1.875) /* 2.5 / (96 / 72) */

/** Notes
 *
 * 1. Table geometry
 * The allocated rectangle is divided into ::rows rows and ::col columns,
 * numbered 0..rows-1 and 0..cols-1.
 * The available width (height) is divided evenly between all columns (rows).
 * The remaining space is distributed among the columns (rows) so that
 * columns cols-n_padded_columns .. cols-1 (rows rows-n_padded_rows .. rows)
 * are 1px wider (taller) than the others.
 */

/* ATK factory */

#ifdef ENABLE_ACCESSIBLE

typedef AtkObjectFactory      GucharmapChartableAccessibleFactory;
typedef AtkObjectFactoryClass GucharmapChartableAccessibleFactoryClass;

static void
gucharmap_chartable_accessible_factory_init (GucharmapChartableAccessibleFactory *factory)
{
}

static AtkObject*
gucharmap_chartable_accessible_factory_create_accessible (GObject *obj)
{
  return gucharmap_chartable_accessible_new (GUCHARMAP_CHARTABLE (obj));
}

static GType
gucharmap_chartable_accessible_factory_get_accessible_type (void)
{
  return gucharmap_chartable_accessible_get_type ();
}

static void
gucharmap_chartable_accessible_factory_class_init (AtkObjectFactoryClass *klass)
{
  klass->create_accessible = gucharmap_chartable_accessible_factory_create_accessible;
  klass->get_accessible_type = gucharmap_chartable_accessible_factory_get_accessible_type;
}

static GType gucharmap_chartable_accessible_factory_get_type (void);
G_DEFINE_TYPE (GucharmapChartableAccessibleFactory, gucharmap_chartable_accessible_factory, ATK_TYPE_OBJECT_FACTORY)

#endif

/* Type definition */

G_DEFINE_TYPE (GucharmapChartable, gucharmap_chartable, GTK_TYPE_DRAWING_AREA)

/* utility functions */

static void
gucharmap_chartable_set_font_desc_internal (GucharmapChartable *chartable,
                                            PangoFontDescription *font_desc /* adopting */)
{
  GucharmapChartablePrivate *priv = chartable->priv;

  if (priv->font_desc)
    pango_font_description_free (priv->font_desc);

  priv->font_desc = font_desc;
 
  if (priv->pango_layout)
    pango_layout_set_font_description (priv->pango_layout, font_desc);

  gtk_widget_queue_resize (GTK_WIDGET (chartable));

  g_object_notify (G_OBJECT (chartable), "font-desc");
}

static void
gucharmap_chartable_emit_status_message (GucharmapChartable *chartable,
                                         const char *message)
{
  g_signal_emit (chartable, signals[STATUS_MESSAGE], 0, message);
}

typedef enum {
  POSITION_DOWN_ALIGN_LEFT,
  POSITION_DOWN_ALIGN_RIGHT,
  POSITION_RIGHT_ALIGN_TOP,
  POSITION_RIGHT_ALIGN_BOTTOM,
  POSITION_TOP_ALIGN_LEFT,
  POSITION_TOP_ALIGN_RIGHT,
  POSITION_LEFT_ALIGN_TOP,
  POSITION_LEFT_ALIGN_BOTTOM
} PositionType;

static const PositionType rtl_position[] = {
  POSITION_DOWN_ALIGN_RIGHT,
  POSITION_DOWN_ALIGN_LEFT,
  POSITION_LEFT_ALIGN_TOP,
  POSITION_LEFT_ALIGN_BOTTOM,
  POSITION_TOP_ALIGN_RIGHT,
  POSITION_TOP_ALIGN_LEFT,
  POSITION_RIGHT_ALIGN_TOP,
  POSITION_RIGHT_ALIGN_BOTTOM
};

/**
 * position_rectangle:
 * @rect: the rectangle to position. Inout; width and height must be initialised
 * @target_rect: the rectangle to position @rect on
 * @bounding_rect: the bounding rectangle
 * @position: how to position the rectangle
 * @direction: the text direction
 *
 * Returns: %TRUE if @rect could be positioned on @reference_point
 * with positioning according to @gravity inside @bounding_rect
 */ 
static gboolean
position_rectangle (GdkRectangle *position_rect,
                    GdkRectangle *target_rect,
                    GdkRectangle *bounding_rect,
                    PositionType position,
                    GtkTextDirection direction)
{
  GdkRectangle rect;

  if (direction == GTK_TEXT_DIR_RTL) {
    position = rtl_position[position];
  }

  rect.x = target_rect->x;
  rect.y = target_rect->y;
  rect.width = position_rect->width;
  rect.height = position_rect->height;

  switch (position) {
    case POSITION_DOWN_ALIGN_RIGHT:
      rect.x -= rect.width - target_rect->width;
      /* fall-through */
    case POSITION_DOWN_ALIGN_LEFT:
      rect.y += target_rect->height;
      break;

    case POSITION_RIGHT_ALIGN_BOTTOM:
      rect.y -= rect.height - target_rect->height;
      /* fall-through */
    case POSITION_RIGHT_ALIGN_TOP:
      rect.x += target_rect->width;
      break;

    case POSITION_TOP_ALIGN_RIGHT:
      rect.x -= rect.width - target_rect->width;
      /* fall-through */
    case POSITION_TOP_ALIGN_LEFT:
      rect.y -= rect.height;
      break;

    case POSITION_LEFT_ALIGN_BOTTOM:
      rect.y -= rect.height - target_rect->height;
      /* fall-through */
    case POSITION_LEFT_ALIGN_TOP:
      rect.x -= rect.width;
      break;
  }

  *position_rect = rect;

  return rect.x >= bounding_rect->x &&
         rect.y >= bounding_rect->y &&
         rect.x + rect.width <= bounding_rect->x + bounding_rect->width &&
         rect.y + rect.height <= bounding_rect->y + bounding_rect->height;
}

static gboolean
position_rectangle_on_screen (GtkWidget *widget,
                              GdkRectangle *rectangle,
                              GdkRectangle *target_rect)
{
  GtkTextDirection direction;
  GdkRectangle monitor;
  int monitor_num;
  GdkScreen *screen;
  static const PositionType positions[] = {
    POSITION_DOWN_ALIGN_LEFT,
    POSITION_TOP_ALIGN_LEFT,
    POSITION_RIGHT_ALIGN_TOP,
    POSITION_LEFT_ALIGN_TOP,
    POSITION_DOWN_ALIGN_RIGHT,
    POSITION_TOP_ALIGN_RIGHT,
    POSITION_RIGHT_ALIGN_BOTTOM,
    POSITION_LEFT_ALIGN_BOTTOM
  };
  guint i;
  
  direction = gtk_widget_get_direction (widget);
  screen = gtk_widget_get_screen (widget);
  monitor_num = gdk_screen_get_monitor_at_window (screen, gtk_widget_get_window (widget));
  if (monitor_num < 0)
    monitor_num = 0;
  gdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

  for (i = 0; i < G_N_ELEMENTS (positions); ++i) {
    if (position_rectangle (rectangle, target_rect, &monitor, positions[i], direction))
      return TRUE;
  }

  return FALSE;
}

static void
get_root_coords_at_active_char (GucharmapChartable *chartable, 
                                gint *x_root, 
                                gint *y_root)
{
  GucharmapChartablePrivate *priv = chartable->priv;
  GtkWidget *widget = GTK_WIDGET (chartable);
  gint x, y;
  gint row, col;

  gdk_window_get_origin (gtk_widget_get_window (widget), &x, &y);

  row = (priv->active_cell - priv->page_first_cell) / priv->cols;
  col = _gucharmap_chartable_cell_column (chartable, priv->active_cell);

  *x_root = x + _gucharmap_chartable_x_offset (chartable, col);
  *y_root = y + _gucharmap_chartable_y_offset (chartable, row);
}

static void
get_active_cell_rect (GucharmapChartable *chartable, GdkRectangle *rect)
{
  GucharmapChartablePrivate *priv = chartable->priv;
  int row, col;

  get_root_coords_at_active_char (chartable, &rect->x, &rect->y);

  row = (priv->active_cell - priv->page_first_cell) / priv->cols;
  col = _gucharmap_chartable_cell_column (chartable, priv->active_cell);

  rect->width = _gucharmap_chartable_column_width (chartable, col);
  rect->height = _gucharmap_chartable_row_height (chartable, row);
}

static void
get_appropriate_upper_left_xy (GucharmapChartable *chartable, 
                               gint width,  gint height,
                               gint x_root, gint y_root,
                               gint *x,     gint *y)
{
  GucharmapChartablePrivate *priv = chartable->priv;
  gint row, col;

  row = (priv->active_cell - priv->page_first_cell) / priv->cols;
  col = _gucharmap_chartable_cell_column (chartable, priv->active_cell);

  *x = x_root;
  *y = y_root;

  if (row >= priv->rows / 2)
    *y -= height;

  if (col >= priv->cols / 2)
    *x -= width;
}

/* depends on directionality */
static guint
get_cell_at_rowcol (GucharmapChartable *chartable,
                    gint            row,
                    gint            col)
{
  GucharmapChartablePrivate *priv = chartable->priv;
  GtkWidget *widget = GTK_WIDGET (chartable);

  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
    return priv->page_first_cell + row * priv->cols + (priv->cols - col - 1);
  else
    return priv->page_first_cell + row * priv->cols + col;
}

/* Depends on directionality. Column 0 is the furthest left.  */
gint
_gucharmap_chartable_cell_column (GucharmapChartable *chartable,
                              guint cell)
{
  GucharmapChartablePrivate *priv = chartable->priv;
  GtkWidget *widget = GTK_WIDGET (chartable);

  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
    return priv->cols - (cell - priv->page_first_cell) % priv->cols - 1;
  else
    return (cell - priv->page_first_cell) % priv->cols;
}

/* not all columns are necessarily the same width because of padding */
gint
_gucharmap_chartable_column_width (GucharmapChartable *chartable, gint col)
{
  GucharmapChartablePrivate *priv = chartable->priv;
  int num_padded_columns = priv->n_padded_columns;
  int min_col_w = priv->minimal_column_width;

  if (priv->cols - col <= num_padded_columns)
    return min_col_w + 1;
  else
    return min_col_w;
}

/* calculates the position of the left end of the column (just to the right
 * of the left border) */
/* XXX: calling this repeatedly is not the most efficient, but it probably
 * is the most readable */
gint
_gucharmap_chartable_x_offset (GucharmapChartable *chartable, gint col)
{
  gint c, x;

  for (c = 0, x = 1;  c < col;  c++)
    x += _gucharmap_chartable_column_width (chartable, c);

  return x;
}

/* not all rows are necessarily the same height because of padding */
gint
_gucharmap_chartable_row_height (GucharmapChartable *chartable, gint row)
{
  GucharmapChartablePrivate *priv = chartable->priv;
  int num_padded_rows = priv->n_padded_rows;
  int min_row_h = priv->minimal_row_height;

  if (priv->rows - row <= num_padded_rows)
    return min_row_h + 1;
  else
    return min_row_h;
}

/* calculates the position of the top end of the row (just below the top
 * border) */
/* XXX: calling this repeatedly is not the most efficient, but it probably
 * is the most readable */
gint
_gucharmap_chartable_y_offset (GucharmapChartable *chartable, gint row)
{
  gint r, y;

  for (r = 0, y = 1;  r < row;  r++)
    y += _gucharmap_chartable_row_height (chartable, r);

  return y;
}

static void
set_top_row (GucharmapChartable *chartable, 
             gint            row)
{
  GucharmapChartablePrivate *priv = chartable->priv;
  gint r, c;

  g_return_if_fail (row >= 0 && row <= priv->last_cell / priv->cols);

  priv->old_page_first_cell = priv->page_first_cell;
  priv->old_active_cell = priv->active_cell;

  priv->page_first_cell = row * priv->cols;

  /* character is still on the visible page */
  if (priv->active_cell - priv->page_first_cell >= 0
      && priv->active_cell - priv->page_first_cell < priv->page_size)
    return;

  c = priv->old_active_cell % priv->cols;

  if (priv->page_first_cell < priv->old_page_first_cell)
    r = priv->rows - 1;
  else
    r = 0;

  priv->active_cell = priv->page_first_cell + r * priv->cols + c;
  if (priv->active_cell > priv->last_cell)
    priv->active_cell = priv->last_cell;

  g_object_notify (G_OBJECT (chartable), "active-character");
}

/* returns the font family of the last glyph item in the first line of the
 * layout; should be freed by caller */
static gchar *
get_font (PangoLayout *layout)
{
  PangoLayoutLine *line;
  PangoGlyphItem *glyph_item;
  PangoFont *font;
  GSList *run_node;
  gchar *family;
  PangoFontDescription *font_desc;

  line = pango_layout_get_line (layout, 0);

  /* get to the last glyph_item (the one with the character we're drawing */
  for (run_node = line->runs;  
       run_node && run_node->next;  
       run_node = run_node->next);

  if (run_node)
    {
      glyph_item = run_node->data;
      font = glyph_item->item->analysis.font;
      font_desc = pango_font_describe (font);

      family = g_strdup (pango_font_description_get_family (font_desc));

      pango_font_description_free (font_desc);
    }
  else
    family = NULL;

  return family;
}

/* font_family (if not null) gets filled in with the actual font family
 * used to draw the character */
static PangoLayout *
layout_scaled_glyph (GucharmapChartable *chartable, 
                     gunichar uc, 
                     double font_factor,
                     char **font_family)
{
  GucharmapChartablePrivate *priv = chartable->priv;
  PangoFontDescription *font_desc;
  PangoLayout *layout;
  gchar buf[11];

  font_desc = pango_font_description_copy (priv->font_desc);

  if (pango_font_description_get_size_is_absolute (priv->font_desc))
    pango_font_description_set_absolute_size (font_desc,
                                              font_factor * pango_font_description_get_size (priv->font_desc));
  else
    pango_font_description_set_size (font_desc,
                                     font_factor * pango_font_description_get_size (priv->font_desc));

  layout = pango_layout_new (pango_layout_get_context (priv->pango_layout));

  pango_layout_set_font_description (layout, font_desc);

  buf[gucharmap_unichar_to_printable_utf8 (uc, buf)] = '\0';
  pango_layout_set_text (layout, buf, -1);

  if (font_family)
    *font_family = get_font (layout);

  pango_font_description_free (font_desc);

  return layout;
}

static GdkPixmap *
create_glyph_pixmap (GucharmapChartable *chartable,
                     gunichar wc,
                     double font_factor,
                     gboolean draw_font_family)
{
  GtkWidget *widget = GTK_WIDGET (chartable);
  enum { PADDING = 8 };

  PangoLayout *pango_layout, *pango_layout2 = NULL;
  PangoRectangle char_rect, family_rect;
  gint pixmap_width, pixmap_height;
  GtkStyle *style;
  GdkPixmap *pixmap;
  char *family;

  /* Apply the scaling.  Unfortunately not all fonts seem to be scalable.
   * We could fall back to GdkPixbuf scaling, but that looks butt ugly :-/
   */
  pango_layout = layout_scaled_glyph (chartable, wc,
                                      font_factor, &family);
  pango_layout_get_pixel_extents (pango_layout, &char_rect, NULL);

  if (draw_font_family)
    {
      if (family == NULL)
        family = g_strdup (_("[not a printable character]"));

      pango_layout2 = gtk_widget_create_pango_layout (GTK_WIDGET (chartable), family);
      pango_layout_get_pixel_extents (pango_layout2, NULL, &family_rect);

      /* Make the GdkPixmap large enough to account for possible offsets in the
       * ink extents of the glyph. */
      pixmap_width  = MAX (char_rect.width, family_rect.width)  + 2 * PADDING;
      pixmap_height = family_rect.height + char_rect.height + 4 * PADDING;
    }
  else
    {
      pixmap_width  = char_rect.width + 2 * PADDING;
      pixmap_height = char_rect.height + 2 * PADDING;
    }

  style = gtk_widget_get_style (widget);

  pixmap = gdk_pixmap_new (gtk_widget_get_window (widget),
                           pixmap_width, pixmap_height, -1);

  gdk_draw_rectangle (pixmap, style->base_gc[GTK_STATE_NORMAL],
                      TRUE, 0, 0, pixmap_width, pixmap_height);

  /* Draw a rectangular border, taking char_rect offsets into account. */
  gdk_draw_rectangle (pixmap, style->fg_gc[GTK_STATE_INSENSITIVE], 
                      FALSE, 1, 1, pixmap_width - 3, pixmap_height - 3);

  /* Now draw the glyph.  The coordinates are adapted
   * in order to compensate negative char_rect offsets. */
  gdk_draw_layout (pixmap, style->text_gc[GTK_STATE_NORMAL],
                   -char_rect.x + PADDING, -char_rect.y + PADDING,
                   pango_layout);
  g_object_unref (pango_layout);

  if (draw_font_family)
    {
      gdk_draw_line (pixmap, style->dark_gc[GTK_STATE_NORMAL],
                     6 + 1, char_rect.height + 2 * PADDING,
                     pixmap_width - 3 - 6, char_rect.height + 2 * PADDING);
      gdk_draw_layout (pixmap, style->text_gc[GTK_STATE_NORMAL],
                       PADDING, pixmap_height - PADDING - family_rect.height,
                       pango_layout2);

      g_object_unref (pango_layout2);
    }

  g_free (family);

  return pixmap;
}

/* places the zoom window toward the inside of the coordinates */
static void
place_zoom_window (GucharmapChartable *chartable, gint x_root, gint y_root)
{
  GucharmapChartablePrivate *priv = chartable->priv;
  GdkPixmap *pixmap;
  gint width, height, x, y;

  if (!priv->zoom_window)
    return;

  gtk_image_get_pixmap (GTK_IMAGE (priv->zoom_image), &pixmap, NULL);
  if (!pixmap)
    return;

  gdk_drawable_get_size (GDK_DRAWABLE (pixmap), &width, &height);
  get_appropriate_upper_left_xy (chartable, width, height,
                                 x_root, y_root, &x, &y);
  gtk_window_move (GTK_WINDOW (priv->zoom_window), x, y);
}

static void
place_zoom_window_on_active_cell (GucharmapChartable *chartable)
{
  GucharmapChartablePrivate *priv = chartable->priv;
  GdkPixmap *pixmap;
  GdkRectangle rect, keepout_rect;

  if (!priv->zoom_window)
    return;

  gtk_image_get_pixmap (GTK_IMAGE (priv->zoom_image), &pixmap, NULL);
  if (!pixmap)
    return;

  get_active_cell_rect (chartable, &keepout_rect);

  rect.x = rect.y = 0;
  gdk_drawable_get_size (GDK_DRAWABLE (pixmap), &rect.width, &rect.height);

  position_rectangle_on_screen (GTK_WIDGET (chartable),
                                &rect,
                                &keepout_rect);
  gtk_window_move (GTK_WINDOW (priv->zoom_window), rect.x, rect.y);
}

static int
get_font_size_px (GucharmapChartable *chartable)
{
  GucharmapChartablePrivate *priv = chartable->priv;
  GtkWidget *widget = GTK_WIDGET (chartable);
  GdkScreen *screen;
  double resolution;
  int font_size;

  g_assert (priv->font_desc != NULL);

  screen = gtk_widget_get_screen (widget);
  resolution = gdk_screen_get_resolution (screen);
  if (resolution < 0.0) /* will be -1 if the resolution is not defined in the GdkScreen */
    resolution = 96.0;

  if (pango_font_description_get_size_is_absolute (priv->font_desc))
    font_size = pango_font_description_get_size (priv->font_desc);
  else
    font_size = ((double) pango_font_description_get_size (priv->font_desc)) * resolution / 72.0;

  if (PANGO_PIXELS (font_size) <= 0)
    font_size = DEFAULT_FONT_SIZE * resolution / 72.0;

  return PANGO_PIXELS (font_size);
}

static void
update_zoom_window (GucharmapChartable *chartable)
{
  GucharmapChartablePrivate *priv = chartable->priv;
  GtkWidget *widget = GTK_WIDGET (chartable);
  GdkPixmap *pixmap;
  double scale;
  int font_size_px, screen_height;

  font_size_px = get_font_size_px (chartable);
  screen_height = gdk_screen_get_height (gtk_widget_get_screen (widget));

  scale = (0.3 * screen_height) / (FACTOR_WIDTH * font_size_px);
  scale = CLAMP (scale, 1.0, 12.0);

  pixmap = create_glyph_pixmap (chartable,
                                gucharmap_chartable_get_active_character (chartable),
                                scale,
                                TRUE);
  gtk_image_set_from_pixmap (GTK_IMAGE (priv->zoom_image), pixmap, NULL);
  g_object_unref (pixmap);
}

static void
make_zoom_window (GucharmapChartable *chartable)
{
  GucharmapChartablePrivate *priv = chartable->priv;
  GtkWidget *widget = GTK_WIDGET (chartable);

  /* if there is already a zoom window, do nothing */
  if (priv->zoom_window || !priv->zoom_mode_enabled)
    return;

  priv->zoom_window = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_window_set_resizable (GTK_WINDOW (priv->zoom_window), FALSE);
  gtk_window_set_screen (GTK_WINDOW (priv->zoom_window),
                         gtk_widget_get_screen (widget));

  priv->zoom_image = gtk_image_new ();
  gtk_container_add (GTK_CONTAINER (priv->zoom_window),
                     priv->zoom_image);
  gtk_widget_show (priv->zoom_image);
}

static void
destroy_zoom_window (GucharmapChartable *chartable)
{
  GucharmapChartablePrivate *priv = chartable->priv;

  if (priv->zoom_window)
    {
      GtkWidget *widget = GTK_WIDGET (chartable);
      GtkWidget *zoom_window;

      zoom_window = priv->zoom_window;
      priv->zoom_window = NULL;
      priv->zoom_image = NULL;

      gdk_window_set_cursor (gtk_widget_get_window (widget), NULL);
      gtk_widget_destroy (zoom_window);
    }
}

static void
gucharmap_chartable_show_zoom (GucharmapChartable *chartable)
{
  GucharmapChartablePrivate *priv = chartable->priv;

  if (!priv->zoom_mode_enabled)
    return;

  make_zoom_window (chartable);
  update_zoom_window (chartable);

  place_zoom_window_on_active_cell (chartable);

  gtk_widget_show (priv->zoom_window);

  g_object_notify (G_OBJECT (chartable), "zoom-showing");
}

static void
gucharmap_chartable_hide_zoom (GucharmapChartable *chartable)
{
  destroy_zoom_window (chartable);

  g_object_notify (G_OBJECT (chartable), "zoom-showing");
}

static void
gucharmap_chartable_set_active_cell (GucharmapChartable *chartable,
                                     guint cell)
{
  GucharmapChartablePrivate *priv = chartable->priv;
  int _cell = (int) cell;

  priv->old_active_cell = priv->active_cell;
  priv->old_page_first_cell = priv->page_first_cell;

  priv->active_cell = _cell;

  /* update page, if necessary */
  if (_cell < priv->page_first_cell ||
      _cell - priv->page_first_cell >= priv->page_size)
    {
      /* move the page_first_cell as far as active_cell has moved */
      int offset = priv->active_cell - priv->old_active_cell;

      if (priv->old_page_first_cell + offset < 0)
        priv->page_first_cell = 0;
      else if (priv->old_page_first_cell +
               offset >
               priv->last_cell -
               (priv->last_cell % priv->cols) -
               priv->cols * (priv->rows - 1))
        priv->page_first_cell = priv->last_cell -
                                     (priv->last_cell % priv->cols) -
                                     priv->cols * (priv->rows - 1);
      else
        priv->page_first_cell = priv->old_page_first_cell + offset;
    
      /* FIXMEchpe: this should be fixed in the conditions above, but just do it for now: */
      if (priv->page_first_cell < 0)
        priv->page_first_cell = 0;

      /* round down so that it's a multiple of priv->cols */
      priv->page_first_cell -= (priv->page_first_cell % priv->cols);
    
      /* go back up if we should have rounded up */
      if (priv->active_cell - priv->page_first_cell >= priv->page_size)
        priv->page_first_cell += priv->cols;
    }

  g_object_notify (G_OBJECT (chartable), "active-character");

  /* FIXMEchpe: update the scroll adjustment! */
}

static void
set_active_char (GucharmapChartable *chartable,
                 gunichar        wc)
{
  GucharmapChartablePrivate *priv = chartable->priv;

  guint cell = gucharmap_codepoint_list_get_index (priv->codepoint_list, wc);
  if (cell == -1) {
    gtk_widget_error_bell (GTK_WIDGET (chartable));
    return;
  }

  gucharmap_chartable_set_active_cell (chartable, cell);
}

static void
draw_borders (GucharmapChartable *chartable)
{
  GucharmapChartablePrivate *priv = chartable->priv;
  GtkWidget *widget = GTK_WIDGET (chartable);
  GtkAllocation *allocation;
  GtkStyle *style;
  gint x, y, col, row;
#if GTK_CHECK_VERSION (2, 18, 0)
  GtkAllocation widget_allocation;

  gtk_widget_get_allocation (widget, &widget_allocation);
  allocation = &widget_allocation;
#else
  allocation = &widget->allocation;
#endif

  /* dark_gc[GTK_STATE_NORMAL] seems to be what is used to draw the borders
   * around widgets, so we use it for the lines */

  style = gtk_widget_get_style (widget);

  /* vertical lines */
  gdk_draw_line (priv->pixmap,
                 style->dark_gc[GTK_STATE_NORMAL],
                 0, 0, 0, allocation->height - 1);
  for (col = 0, x = 0;  col < priv->cols;  col++)
    {
      x += _gucharmap_chartable_column_width (chartable, col);
      gdk_draw_line (priv->pixmap,
                     style->dark_gc[GTK_STATE_NORMAL],
                     x, 0, x, allocation->height - 1);
    }

  /* horizontal lines */
  gdk_draw_line (priv->pixmap,
                 style->dark_gc[GTK_STATE_NORMAL],
                 0, 0, allocation->width - 1, 0);
  for (row = 0, y = 0;  row < priv->rows;  row++)
    {
      y += _gucharmap_chartable_row_height (chartable, row);
      gdk_draw_line (priv->pixmap,
                     style->dark_gc[GTK_STATE_NORMAL],
                     0, y, allocation->width - 1, y);
    }
}

static void
set_scrollbar_adjustment (GucharmapChartable *chartable)
{
  GucharmapChartablePrivate *priv = chartable->priv;

  /* block our "value_changed" handler */
  g_signal_handler_block (G_OBJECT (priv->vadjustment),
                          priv->vadjustment_changed_handler_id);

  gtk_adjustment_set_value (priv->vadjustment, 1.0 * priv->page_first_cell / priv->cols);

  g_signal_handler_unblock (G_OBJECT (priv->vadjustment),
                            priv->vadjustment_changed_handler_id);
}

/* for mouse clicks */
static gunichar
get_cell_at_xy (GucharmapChartable *chartable, 
                gint            x, 
                gint            y)
{
  GucharmapChartablePrivate *priv = chartable->priv;
  gint r, c, x0, y0;
  guint cell;

  for (c = 0, x0 = 0;  x0 <= x && c < priv->cols;  c++)
    x0 += _gucharmap_chartable_column_width (chartable, c);

  for (r = 0, y0 = 0;  y0 <= y && r < priv->rows;  r++)
    y0 += _gucharmap_chartable_row_height (chartable, r);

  /* cell = rowcol_to_unichar (chartable, r-1, c-1); */
  cell = get_cell_at_rowcol (chartable, r-1, c-1);

  /* XXX: check this somewhere else? */
  if (cell > priv->last_cell)
    return priv->last_cell;

  return cell;
}

static void
draw_character (GucharmapChartable *chartable, 
                gint            row, 
                gint            col)
{
  GucharmapChartablePrivate *priv = chartable->priv;
  GtkWidget *widget = GTK_WIDGET (chartable);
  gint padding_x, padding_y;
  gint char_width, char_height;
  gint square_width, square_height; 
  gunichar wc;
  guint cell;
  GdkGC *gc;
  GtkStyle *style;
  gchar buf[10];
  gint n;

  cell = get_cell_at_rowcol (chartable, row, col);
  wc = gucharmap_codepoint_list_get_char (priv->codepoint_list, cell);

  if (wc > UNICHAR_MAX || !gucharmap_unichar_validate (wc) || !gucharmap_unichar_isdefined (wc))
    return;

  style = gtk_widget_get_style (widget);

#if GTK_CHECK_VERSION (2, 18, 0)
  if (gtk_widget_has_focus (widget) && (gint)cell == priv->active_cell)
#else
  if (GTK_WIDGET_HAS_FOCUS (widget) && (gint)cell == priv->active_cell)
#endif
    gc = style->text_gc[GTK_STATE_SELECTED];
  else if ((gint)cell == priv->active_cell)
    gc = style->text_gc[GTK_STATE_ACTIVE];
  else
    gc = style->text_gc[GTK_STATE_NORMAL];

  square_width = _gucharmap_chartable_column_width (chartable, col) - 1;
  square_height = _gucharmap_chartable_row_height (chartable, row) - 1;

  n = gucharmap_unichar_to_printable_utf8 (wc, buf); 
  pango_layout_set_text (priv->pango_layout, buf, n);

  pango_layout_get_pixel_size (priv->pango_layout, &char_width, &char_height);

  /* (square_width - char_width)/2 is the smaller half */
  padding_x = (square_width - char_width) - (square_width - char_width)/2;
  padding_y = (square_height - char_height) - (square_height - char_height)/2;

  gdk_draw_layout (priv->pixmap, gc,
                   _gucharmap_chartable_x_offset (chartable, col) + padding_x,
                   _gucharmap_chartable_y_offset (chartable, row) + padding_y,
                   priv->pango_layout);
}

static void
draw_square_bg (GucharmapChartable *chartable, gint row, gint col)
{
  GucharmapChartablePrivate *priv = chartable->priv;
  GtkWidget *widget = GTK_WIDGET (chartable);
  gint square_width, square_height; 
  GdkGC *gc;
  GdkColor untinted;
  GtkStyle *style;
  guint cell;
  gunichar wc;

  cell = get_cell_at_rowcol (chartable, row, col);
  wc = gucharmap_codepoint_list_get_char (priv->codepoint_list, cell);

  gc = gdk_gc_new (GDK_DRAWABLE (gtk_widget_get_window (widget)));

  style = gtk_widget_get_style (widget);

#if GTK_CHECK_VERSION (2, 18, 0)
  if (gtk_widget_has_focus (widget) && (gint)cell == priv->active_cell)
#else
  if (GTK_WIDGET_HAS_FOCUS (widget) && (gint)cell == priv->active_cell)
#endif
    untinted = style->base[GTK_STATE_SELECTED];
  else if ((gint)cell == priv->active_cell)
    untinted = style->base[GTK_STATE_ACTIVE];
  else if ((gint)cell > priv->last_cell)
    untinted = style->dark[GTK_STATE_NORMAL];
  else if (! gucharmap_unichar_validate (wc))
    untinted = style->fg[GTK_STATE_INSENSITIVE];
  else if (! gucharmap_unichar_isdefined (wc))
    untinted = style->bg[GTK_STATE_INSENSITIVE];
  else
    untinted = style->base[GTK_STATE_NORMAL];

  gdk_gc_set_rgb_fg_color (gc, &untinted);

  square_width = _gucharmap_chartable_column_width (chartable, col) - 1;
  square_height = _gucharmap_chartable_row_height (chartable, row) - 1;

  gdk_draw_rectangle (priv->pixmap, gc, TRUE,
                      _gucharmap_chartable_x_offset (chartable, col), 
		      _gucharmap_chartable_y_offset (chartable, row),
                      square_width, square_height);

  g_object_unref (gc);
}

static void
expose_square (GucharmapChartable *chartable, gint row, gint col)
{
  GtkWidget *widget = GTK_WIDGET (chartable);

  gtk_widget_queue_draw_area (widget,
                              _gucharmap_chartable_x_offset (chartable, col),
                              _gucharmap_chartable_y_offset (chartable, row),
                              _gucharmap_chartable_column_width (chartable, col) - 1,
                              _gucharmap_chartable_row_height (chartable, row) - 1);
}

static void
draw_square (GucharmapChartable *chartable, gint row, gint col)
{
  draw_square_bg (chartable, row, col);
  draw_character (chartable, row, col);
}

static void
draw_and_expose_cell (GucharmapChartable *chartable,
                      guint cell)
{
  GucharmapChartablePrivate *priv = chartable->priv;

  gint row = (cell - priv->page_first_cell) / priv->cols;
  gint col = _gucharmap_chartable_cell_column (chartable, cell);

  if (row >= 0 && row < priv->rows && col >= 0 && col < priv->cols)
    {
      draw_square (chartable, row, col);
      expose_square (chartable, row, col);
    }
}

/* draws the backing store pixmap */
static void
draw_chartable_from_scratch (GucharmapChartable *chartable)
{
  GucharmapChartablePrivate *priv = chartable->priv;
  GtkWidget *widget = GTK_WIDGET (chartable);
  gint row, col;
  GtkAllocation *allocation;
#if GTK_CHECK_VERSION (2, 18, 0)
  GtkAllocation widget_allocation;

  gtk_widget_get_allocation (widget, &widget_allocation);
  allocation = &widget_allocation;
#else
  allocation = &widget->allocation;
#endif

  /* drawing area may not be exposed yet when restoring last char setting
   */
#if GTK_CHECK_VERSION (2, 20, 0)
  if (!gtk_widget_get_realized (GTK_WIDGET (chartable)))
#else
  if (!GTK_WIDGET_REALIZED (chartable))
#endif
    return;

  if (priv->pixmap == NULL)
    priv->pixmap = gdk_pixmap_new (
	    gtk_widget_get_window (widget),
	    allocation->width,
	    allocation->height, -1);

  draw_borders (chartable);

  /* draw the characters */
  for (row = 0;  row < priv->rows;  row++)
    for (col = 0;  col < priv->cols;  col++)
      {
        draw_square_bg (chartable, row, col);
        draw_character (chartable, row, col);
      }
}

static void
copy_rows (GucharmapChartable *chartable, gint row_offset)
{
  GucharmapChartablePrivate *priv = chartable->priv;
  GtkWidget *widget = GTK_WIDGET (chartable);
  gint num_padded_rows = priv->n_padded_rows;
  gint from_row, to_row;
  GtkStyle *style;
  GtkAllocation *allocation;
#if GTK_CHECK_VERSION (2, 18, 0)
  GtkAllocation widget_allocation;

  gtk_widget_get_allocation (widget, &widget_allocation);
  allocation = &widget_allocation;
#else
  allocation = &widget->allocation;
#endif

  style = gtk_widget_get_style (widget);

  if (ABS (row_offset) < priv->rows - num_padded_rows)
    {
      gint num_rows, height;

      if (row_offset > 0)
        {
          from_row = row_offset;
          to_row = 0;
          num_rows = priv->rows - num_padded_rows - from_row;
        }
      else
        {
          from_row = 0;
          to_row = -row_offset;
          num_rows = priv->rows - num_padded_rows - to_row;
        }

      height = _gucharmap_chartable_y_offset (chartable, num_rows) 
               - _gucharmap_chartable_y_offset (chartable, 0) - 1;

      gdk_draw_drawable (
              priv->pixmap,
              style->base_gc[GTK_STATE_NORMAL],
              priv->pixmap,
              0, _gucharmap_chartable_y_offset (chartable, from_row), 
              0, _gucharmap_chartable_y_offset (chartable, to_row),
              allocation->width, height);
    }

  if (ABS (row_offset) < num_padded_rows)
    {
      /* don't need num_rows or height, cuz we can go off the end */
      if (row_offset > 0)
        {
          from_row = priv->rows - num_padded_rows + row_offset;
          to_row = priv->rows - num_padded_rows;
        }
      else
        {
          from_row = priv->rows - num_padded_rows;
          to_row = priv->rows - num_padded_rows - row_offset;
        }

      /* it's ok to go off the end (so use allocation.height) */
      gdk_draw_drawable (
              priv->pixmap,
              style->base_gc[GTK_STATE_NORMAL],
              priv->pixmap,
              0, _gucharmap_chartable_y_offset (chartable, from_row), 
              0, _gucharmap_chartable_y_offset (chartable, to_row),
              allocation->width,
              allocation->height);
    }
}

static void
redraw_rows (GucharmapChartable *chartable, gint row_offset)
{
  GucharmapChartablePrivate *priv = chartable->priv;
  gint row, col, start_row, end_row;

  if (row_offset > 0) 
    {
      start_row = priv->rows - row_offset;
      end_row = priv->rows - 1;
    }
  else
    {
      start_row = 0;
      end_row = -row_offset - 1;
    }

  for (row = 0;  row <= priv->rows;  row++)
    {
      gboolean draw_row = FALSE;

      draw_row = draw_row || (row >= start_row && row <= end_row);

      if (row + row_offset >= 0 && row + row_offset <= priv->rows)
        {
          draw_row = draw_row || (_gucharmap_chartable_row_height (chartable, row) 
                                  != _gucharmap_chartable_row_height (chartable,  
                                                         row + row_offset));
        }

      if (draw_row)
        {
          for (col = 0;  col < priv->cols;  col++)
            draw_square (chartable, row, col);
        }
    }
}

/* Redraws whatever needs to be redrawn, in the character table and
 * everything, and exposes what needs to be exposed. */
void
_gucharmap_chartable_redraw (GucharmapChartable *chartable, 
                         gboolean        move_zoom)
{
  GucharmapChartablePrivate *priv = chartable->priv;
  GtkWidget *widget = GTK_WIDGET (chartable);
  gint row_offset;
  gboolean actives_done = FALSE;

  row_offset = ((gint) priv->page_first_cell - (gint) priv->old_page_first_cell) / priv->cols;

#ifdef G_OS_WIN32

  if (row_offset != 0)
    {
      /* get around the bug in gdkdrawable-win32.c */
      /* yup, this makes it really slow */
      draw_chartable_from_scratch (chartable);
      gtk_widget_queue_draw (widget);
      actives_done = TRUE;
    }

#else /* #ifdef G_OS_WIN32 */

  if (priv->codepoint_list_changed
          || row_offset >= priv->rows
          || row_offset <= -priv->rows)
    {
      draw_chartable_from_scratch (chartable);
      gtk_widget_queue_draw (widget);
      actives_done = TRUE;
      priv->codepoint_list_changed = FALSE;
    }
  else if (row_offset != 0)
    {
      copy_rows (chartable, row_offset);
      redraw_rows (chartable, row_offset);
      draw_borders (chartable);
      gtk_widget_queue_draw (widget);
    }

#endif

  if (priv->active_cell != priv->old_active_cell)
    {
      set_scrollbar_adjustment (chartable); /* XXX */

      if (!actives_done)
        {
          draw_and_expose_cell (chartable, priv->old_active_cell);
          draw_and_expose_cell (chartable, priv->active_cell);
        }

      if (priv->zoom_window)
        update_zoom_window (chartable);

      if (move_zoom && priv->zoom_window)
        {
          place_zoom_window_on_active_cell (chartable);
        }
    }

  priv->old_page_first_cell = priv->page_first_cell;
  priv->old_active_cell = priv->active_cell;
}

static void
update_scrollbar_adjustment (GucharmapChartable *chartable)
{
  GucharmapChartablePrivate *priv = chartable->priv;
  GtkAdjustment *vadjustment = priv->vadjustment;

  if (!vadjustment)
    return;

  gtk_adjustment_configure (vadjustment,
                            1.0 * priv->page_first_cell / priv->cols,
                            0.0,
                            1.0 * ( priv->last_cell / priv->cols + 1 ),
                            3.0,
                            1.0 * priv->rows,
                            /* FIXMEchpe: shouldn't set page size at all! */
                            /* FIXMEchpe + 1 maybe? so scroll-wheel up/down scroll exactly half a page? */
                            priv->rows);
}

static void
vadjustment_value_changed_cb (GtkAdjustment *adjustment, GucharmapChartable *chartable)
{
  set_top_row (chartable, (gint) gtk_adjustment_get_value (adjustment));
  _gucharmap_chartable_redraw (chartable, TRUE);
}

/* GtkWidget class methods */

/*  - single click with left button: activate character under pointer
 *  - double-click with left button: add active character to text_to_copy
 *  - single-click with middle button: jump to selection_primary
 */
static gboolean
gucharmap_chartable_button_press (GtkWidget *widget,
                                  GdkEventButton *event)
{
  GucharmapChartable *chartable = GUCHARMAP_CHARTABLE (widget);
  GucharmapChartablePrivate *priv = chartable->priv;

  /* in case we lost keyboard focus and are clicking to get it back */
  gtk_widget_grab_focus (widget);

  if (event->button == 1)
    {
      priv->click_x = event->x;
      priv->click_y = event->y;
    }

  /* double-click */
  if (event->button == 1 && event->type == GDK_2BUTTON_PRESS)
    {
      g_signal_emit (chartable, signals[ACTIVATE], 0);
    }
  /* single-click */ 
  else if (event->button == 1 && event->type == GDK_BUTTON_PRESS) 
    {
      gucharmap_chartable_set_active_cell (chartable, get_cell_at_xy (chartable, event->x, event->y));
      _gucharmap_chartable_redraw (chartable, TRUE);
    }
  else if (event->button == 3)
    {
      gucharmap_chartable_set_active_cell (chartable, get_cell_at_xy (chartable, event->x, event->y));
      _gucharmap_chartable_redraw (chartable, FALSE); /* FIXMEchpe */

      if (priv->zoom_mode_enabled)
      {
        make_zoom_window (chartable);

        /* FIXME: shouldn't this be "!=" ? */
        if (priv->active_cell == priv->old_active_cell)
          update_zoom_window (chartable); 

        place_zoom_window (chartable, event->x_root, event->y_root);
        gtk_widget_show (priv->zoom_window);
        g_object_notify (G_OBJECT (chartable), "zoom-showing");
      }
    }

  /* XXX: [need to return false so it gets drag events] */
  /* actually return true because we handle drag_begin because of
   * http://bugzilla.mate.org/show_bug.cgi?id=114534 */
  return TRUE;
}

static gboolean
gucharmap_chartable_button_release (GtkWidget *widget,
                                    GdkEventButton *event)
{
  GucharmapChartable *chartable = GUCHARMAP_CHARTABLE (widget);
  gboolean (* button_press_event) (GtkWidget *, GdkEventButton *) =
    GTK_WIDGET_CLASS (gucharmap_chartable_parent_class)->button_release_event;

  if (event->button == 3)
    gucharmap_chartable_hide_zoom (chartable);

  if (button_press_event)
    return button_press_event (widget, event);
  return FALSE;
}

static void
gucharmap_chartable_drag_begin (GtkWidget *widget,
                                GdkDragContext *context)
{
  GucharmapChartable *chartable = GUCHARMAP_CHARTABLE (widget);
  GdkPixmap *drag_icon;
  double scale;
  int font_size_px, screen_height;

  font_size_px = get_font_size_px (chartable);
  screen_height = gdk_screen_get_height (gtk_widget_get_screen (widget));

  scale = (0.3 * screen_height) / (FACTOR_WIDTH * font_size_px);
  scale = CLAMP (scale, 1.0, 5.0);

  drag_icon = create_glyph_pixmap (chartable,
                                   gucharmap_chartable_get_active_character (chartable),
                                   scale,
                                   FALSE);
  gtk_drag_set_icon_pixmap (context, gtk_widget_get_colormap (widget), 
                            drag_icon, NULL, -8, -8);
  g_object_unref (drag_icon);

  /* no need to chain up */
}

static void
gucharmap_chartable_drag_data_get (GtkWidget *widget, 
                                   GdkDragContext *context,
                                   GtkSelectionData *selection_data,
                                   guint info,
                                   guint time)

{
  GucharmapChartable *chartable = GUCHARMAP_CHARTABLE (widget);
  GucharmapChartablePrivate *priv = chartable->priv;
  gchar buf[7];
  gint n;

  n = g_unichar_to_utf8 (gucharmap_codepoint_list_get_char (priv->codepoint_list, priv->active_cell), buf);
  gtk_selection_data_set_text (selection_data, buf, n);

  /* no need to chain up */
}

static void
gucharmap_chartable_drag_data_received (GtkWidget *widget,
                                        GdkDragContext *context,
                                        gint x, gint y,
                                        GtkSelectionData *selection_data,
                                        guint info,
                                        guint time)
{
  GucharmapChartable *chartable = GUCHARMAP_CHARTABLE (widget);
  GucharmapChartablePrivate *priv = chartable->priv;
  gchar *text;
  gunichar wc;

  if (gtk_selection_data_get_length (selection_data) <= 0 ||
      gtk_selection_data_get_data (selection_data) == NULL)
    return;

  text = (gchar *) gtk_selection_data_get_text (selection_data);

  if (text == NULL) /* XXX: say something in the statusbar? */
    return;

  wc = g_utf8_get_char_validated (text, -1);

  if (wc == (gunichar)(-2) || wc == (gunichar)(-1) || wc > UNICHAR_MAX)
    gucharmap_chartable_emit_status_message (chartable, _("Unknown character, unable to identify."));
  else if (gucharmap_codepoint_list_get_index (priv->codepoint_list, wc) == (guint)(-1))
    gucharmap_chartable_emit_status_message (chartable, _("Not found."));
  else
    {
      gucharmap_chartable_emit_status_message (chartable, _("Character found."));
      set_active_char (chartable, wc);
      _gucharmap_chartable_redraw (chartable, TRUE);
    }

  g_free (text);

  /* no need to chain up */
}

static gboolean
gucharmap_chartable_expose_event (GtkWidget *widget, 
                                  GdkEventExpose *event)
{
  GucharmapChartable *chartable = GUCHARMAP_CHARTABLE (widget);
  GucharmapChartablePrivate *priv = chartable->priv;
#if GTK_CHECK_VERSION(2,90,5)
  cairo_rectangle_int_t *rect;
  cairo_rectangle_int_t r;
#else
  GdkRectangle *rects;
  GdkRectangle *rect;
#endif
  int i, n_rects;
  GdkGC *gc;
  GtkStyle *style;
  GdkWindow *window;

  /* Don't draw anything if we haven't set a codepoint list yet */
  if (priv->codepoint_list == NULL)
    return FALSE;

  if (priv->pixmap == NULL)
    {
      draw_chartable_from_scratch (chartable);
    }

#if GTK_CHECK_VERSION(2,90,5)
  if (cairo_region_is_empty (event->region))
    return FALSE;
  n_rects = cairo_region_num_rectangles (event->region);
#else
  if (gdk_region_empty (event->region))
    return FALSE;
  gdk_region_get_rectangles (event->region, &rects, &n_rects);
#endif

  if (n_rects == 0)
    return FALSE;

#if 0
  {
    g_print ("Exposing area %d:%d@(%d,%d) with %d rects ", event->area.width, event->area.height,
             event->area.x, event->area.y, n_rects);
    for (i = 0; i < n_rects; ++i) {
      g_print ("[Rect %d:%d@(%d,%d)] ", rects[i].width, rects[i].height, rects[i].x, rects[i].y);
    }
    g_print ("\n");
  }
#endif

  style = gtk_widget_get_style (widget);
  window = gtk_widget_get_window (widget);

  gc = style->fg_gc[GTK_STATE_NORMAL];
  for (i = 0; i < n_rects; ++i) {
#if GTK_CHECK_VERSION(2,90,5)
    cairo_region_get_rectangle (event->region, i, &r);
    rect = &r;
#else
    rect = rects + i;
#endif
    gdk_draw_drawable (window,
                       gc,
                       priv->pixmap,
                       rect->x, rect->y,
                       rect->x, rect->y,
                       rect->width, rect->height);
  }

#if GTK_CHECK_VERSION(2,90,5)
#else
  g_free (rects);
#endif

  /* no need to chain up */
  return FALSE;
}

static gboolean
gucharmap_chartable_focus_in_event (GtkWidget *widget, 
                                    GdkEventFocus *event)
{
  GucharmapChartable *chartable = GUCHARMAP_CHARTABLE (widget);
  GucharmapChartablePrivate *priv = chartable->priv;

  if (priv->pixmap != NULL)
    draw_and_expose_cell (chartable, priv->active_cell);

  return GTK_WIDGET_CLASS (gucharmap_chartable_parent_class)->focus_in_event (widget, event);
}

static gboolean
gucharmap_chartable_focus_out_event (GtkWidget *widget,
                                     GdkEventFocus *event)
{
  GucharmapChartable *chartable = GUCHARMAP_CHARTABLE (widget);
  GucharmapChartablePrivate *priv = chartable->priv;

  gucharmap_chartable_hide_zoom (chartable);

  if (priv->pixmap != NULL)
    draw_and_expose_cell (chartable, priv->active_cell);

  /* FIXME: the parent's handler already does a draw... */

  return GTK_WIDGET_CLASS (gucharmap_chartable_parent_class)->focus_out_event (widget, event);
}

static gboolean
gucharmap_chartable_key_press_event (GtkWidget *widget,
                                     GdkEventKey *event)
{
  GucharmapChartable *chartable = GUCHARMAP_CHARTABLE (widget);

  if (event->state & (GDK_MOD1_MASK | GDK_CONTROL_MASK))
    return GTK_WIDGET_CLASS (gucharmap_chartable_parent_class)->key_press_event (widget, event);

  switch (event->keyval)
    {
      case GDK_Shift_L: case GDK_Shift_R:
        gucharmap_chartable_show_zoom (chartable);
        break;

      /* pass on other keys, like tab and stuff that shifts focus */
      default:
        break;
    }

  return GTK_WIDGET_CLASS (gucharmap_chartable_parent_class)->key_press_event (widget, event);
}

static gboolean
gucharmap_chartable_key_release_event (GtkWidget *widget,
                                       GdkEventKey *event)
{
  GucharmapChartable *chartable = GUCHARMAP_CHARTABLE (widget);
  
  switch (event->keyval)
    {
      /* XXX: If the group(shift_toggle) Xkb option is set, then releasing
       * the shift key gives either ISO_Next_Group or ISO_Prev_Group. Is
       * there a better way to handle this case? */
      case GDK_Shift_L:
      case GDK_Shift_R:
      case GDK_ISO_Next_Group:
      case GDK_ISO_Prev_Group:
        gucharmap_chartable_hide_zoom (chartable);
        break;
    }

  return GTK_WIDGET_CLASS (gucharmap_chartable_parent_class)->key_release_event (widget, event);
}

static gboolean
gucharmap_chartable_motion_notify (GtkWidget *widget,
                                   GdkEventMotion *event)
{
  GucharmapChartable *chartable = GUCHARMAP_CHARTABLE (widget);
  GucharmapChartablePrivate *priv = chartable->priv;
  gboolean (* motion_notify_event) (GtkWidget *, GdkEventMotion *) =
    GTK_WIDGET_CLASS (gucharmap_chartable_parent_class)->motion_notify_event;

  if ((event->state & GDK_BUTTON1_MASK) &&
      gtk_drag_check_threshold (widget,
                                priv->click_x,
                                priv->click_y,
                                event->x,
                                event->y) &&
      gucharmap_unichar_validate (gucharmap_chartable_get_active_character (chartable)))
    {
      gtk_drag_begin (widget, priv->target_list,
                      GDK_ACTION_COPY, 1, (GdkEvent *) event);
    }

  if ((event->state & GDK_BUTTON3_MASK) != 0 &&
      priv->zoom_window)
    {
      guint cell = get_cell_at_xy (chartable, MAX (0, event->x), MAX (0, event->y));

      if ((gint)cell != priv->active_cell)
        {
          gtk_widget_hide (priv->zoom_window);
          gucharmap_chartable_set_active_cell (chartable, cell);
          _gucharmap_chartable_redraw (chartable, FALSE);
        }

      place_zoom_window (chartable, event->x_root, event->y_root);
      gtk_widget_show (priv->zoom_window);
    }

  if (motion_notify_event)
    motion_notify_event (widget, event);
  return FALSE;
}

static void
gucharmap_chartable_realize (GtkWidget *widget)
{
  GucharmapChartable *chartable = GUCHARMAP_CHARTABLE (widget);
  GucharmapChartablePrivate *priv = chartable->priv;

  GTK_WIDGET_CLASS (gucharmap_chartable_parent_class)->realize (widget);

  gdk_window_set_back_pixmap (gtk_widget_get_window (widget), NULL, FALSE);
    
  if (priv->pixmap != NULL)
    {
      g_object_unref (priv->pixmap);
      priv->pixmap = NULL;
    }
}

#define FIRST_CELL_IN_SAME_ROW(x) ((x) - ((x) % priv->cols))

static void
gucharmap_chartable_size_allocate (GtkWidget *widget,
                                   GtkAllocation *allocation)
{
  GucharmapChartable *chartable = GUCHARMAP_CHARTABLE (widget);
  GucharmapChartablePrivate *priv = chartable->priv;
  int old_rows, old_cols;
  int total_extra_pixels;
  int new_first_cell;
  int bare_minimal_column_width, bare_minimal_row_height;
  int font_size_px;
#if GTK_CHECK_VERSION (2, 18, 0)
  GtkAllocation widget_allocation;
#endif

  GTK_WIDGET_CLASS (gucharmap_chartable_parent_class)->size_allocate (widget, allocation);

#if GTK_CHECK_VERSION (2, 18, 0)
  gtk_widget_get_allocation (widget, &widget_allocation);
  allocation = &widget_allocation;
#else
  allocation = &widget->allocation;
#endif

  old_rows = priv->rows;
  old_cols = priv->cols;

  font_size_px = get_font_size_px (chartable);

  /* FIXMEchpe bug 329481 */
  bare_minimal_column_width = FACTOR_WIDTH * font_size_px;
  bare_minimal_row_height = FACTOR_HEIGHT * font_size_px;

  if (priv->snap_pow2_enabled)
    priv->cols = (1 << g_bit_nth_msf ((allocation->width - 1) / bare_minimal_column_width, -1));
  else
    priv->cols = (allocation->width - 1) / bare_minimal_column_width;

  priv->rows = (allocation->height - 1) / bare_minimal_row_height;

  /* avoid a horrible floating point exception crash */
  if (priv->rows < 1)
    priv->rows = 1;
  if (priv->cols < 1)
    priv->cols = 1;

  priv->page_size = priv->rows * priv->cols;

  total_extra_pixels = allocation->width - (priv->cols * bare_minimal_column_width + 1);
  priv->minimal_column_width = bare_minimal_column_width + total_extra_pixels / priv->cols;
  priv->n_padded_columns = allocation->width - (priv->minimal_column_width * priv->cols + 1);

  total_extra_pixels = allocation->height - (priv->rows * bare_minimal_row_height + 1);
  priv->minimal_row_height = bare_minimal_row_height + total_extra_pixels / priv->rows;
  priv->n_padded_rows = allocation->height - (priv->minimal_row_height * priv->rows + 1);

  /* force pixmap to be redrawn on next expose event */
  if (priv->pixmap != NULL)
    g_object_unref (priv->pixmap);
  priv->pixmap = NULL;

  if (priv->rows == old_rows && priv->cols == old_cols)
    return;

  /* Need to recalculate the first cell, see bug #517188 */
  new_first_cell = FIRST_CELL_IN_SAME_ROW (priv->active_cell);
  if ((new_first_cell + priv->rows*priv->cols) > (priv->last_cell))
    {
      /* last cell is visible, so make sure it is in the last row */
      new_first_cell = FIRST_CELL_IN_SAME_ROW (priv->last_cell) - priv->page_size + priv->cols;

      if (new_first_cell < 0)
        new_first_cell = 0;
    }
  priv->page_first_cell = new_first_cell;

  update_scrollbar_adjustment (chartable);
}

static void
gucharmap_chartable_size_request (GtkWidget *widget,
                                  GtkRequisition *requisition)
{
  GucharmapChartable *chartable = GUCHARMAP_CHARTABLE (widget);
  int font_size_px;

  font_size_px = get_font_size_px (chartable);

  requisition->width = FACTOR_WIDTH * font_size_px;
  requisition->height = FACTOR_HEIGHT * font_size_px;
}

static void
gucharmap_chartable_style_set (GtkWidget *widget, 
                               GtkStyle *previous_style)
{
  GucharmapChartable *chartable = GUCHARMAP_CHARTABLE (widget);
  GucharmapChartablePrivate *priv = chartable->priv;

  GTK_WIDGET_CLASS (gucharmap_chartable_parent_class)->style_set (widget, previous_style);

  if (priv->pixmap != NULL)
    g_object_unref (priv->pixmap);
  priv->pixmap = NULL;

  if (priv->pango_layout)
    g_object_unref (priv->pango_layout);
  priv->pango_layout = NULL;

  if (priv->font_desc == NULL) {
    GtkStyle *style;
    PangoFontDescription *font_desc;

    style = gtk_widget_get_style (widget);
    font_desc = pango_font_description_copy (style->font_desc);

    /* Use twice the size of the style's font */
    if (pango_font_description_get_size_is_absolute (font_desc))
      pango_font_description_set_absolute_size (font_desc,
                                                2 * pango_font_description_get_size (font_desc));
    else
      pango_font_description_set_size (font_desc,
                                       2 * pango_font_description_get_size (font_desc));

    gucharmap_chartable_set_font_desc_internal (chartable, font_desc /* adopts */);
    g_assert (priv->font_desc != NULL);
  }

  priv->pango_layout = gtk_widget_create_pango_layout (widget, NULL);
  pango_layout_set_font_description (priv->pango_layout,
                                     priv->font_desc);

  /* FIXME: necessary? */
  /* gtk_widget_queue_draw (widget); */
  gtk_widget_queue_resize (widget);
}

#ifdef ENABLE_ACCESSIBLE

static AtkObject *
gucharmap_chartable_get_accessible (GtkWidget *widget)
{
  static gboolean first_time = TRUE;

  if (first_time)
    {
      AtkObjectFactory *factory;
      AtkRegistry *registry;
      GType derived_type; 
      GType derived_atk_type; 

      /*
       * Figure out whether accessibility is enabled by looking at the
       * type of the accessible object which would be created for
       * the parent type of GucharmapChartable.
       */
      derived_type = g_type_parent (GUCHARMAP_TYPE_CHARTABLE);

      registry = atk_get_default_registry ();
      factory = atk_registry_get_factory (registry,
                                          derived_type);
      derived_atk_type = atk_object_factory_get_accessible_type (factory);
      if (g_type_is_a (derived_atk_type, GTK_TYPE_ACCESSIBLE)) 
	atk_registry_set_factory_type (registry, 
				       GUCHARMAP_TYPE_CHARTABLE,
				       gucharmap_chartable_accessible_factory_get_type ());
      first_time = FALSE;
    }

  return GTK_WIDGET_CLASS (gucharmap_chartable_parent_class)->get_accessible (widget);
}

#endif

/* GucharmapChartable class methods */

static void
gucharmap_chartable_set_adjustments (GucharmapChartable *chartable,
                                     GtkAdjustment *hadjustment,
                                     GtkAdjustment *vadjustment)
{
  GucharmapChartablePrivate *priv = chartable->priv;

  if (vadjustment)
    g_return_if_fail (GTK_IS_ADJUSTMENT (vadjustment));
  else
    vadjustment = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));

  if (priv->vadjustment)
    {
      g_signal_handler_disconnect (priv->vadjustment,
                                   priv->vadjustment_changed_handler_id);
      priv->vadjustment_changed_handler_id = 0;
      g_object_unref (priv->vadjustment);
      priv->vadjustment = NULL;
    }

  if (vadjustment)
    {
      priv->vadjustment = g_object_ref_sink (vadjustment);
      priv->vadjustment_changed_handler_id =
          g_signal_connect (vadjustment, "value-changed",
                            G_CALLBACK (vadjustment_value_changed_cb),
                            chartable);
    }

  update_scrollbar_adjustment (chartable);
}

static void
gucharmap_chartable_add_move_binding (GtkBindingSet  *binding_set,
                                      guint           keyval,
                                      guint           modmask,
                                      GtkMovementStep step,
                                      gint            count)
{
  gtk_binding_entry_add_signal (binding_set, keyval, modmask,
                                "move-cursor", 2,
                                G_TYPE_ENUM, step,
                                G_TYPE_INT, count);

  gtk_binding_entry_add_signal (binding_set, keyval, GDK_SHIFT_MASK,
                                "move-cursor", 2,
                                G_TYPE_ENUM, step,
                                G_TYPE_INT, count);

  if ((modmask & GDK_CONTROL_MASK) == GDK_CONTROL_MASK)
   return;

  gtk_binding_entry_add_signal (binding_set, keyval, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
                                "move-cursor", 2,
                                G_TYPE_ENUM, step,
                                G_TYPE_INT, count);

  gtk_binding_entry_add_signal (binding_set, keyval, GDK_CONTROL_MASK,
                                "move-cursor", 2,
                                G_TYPE_ENUM, step,
                                G_TYPE_INT, count);
}

static void
gucharmap_chartable_move_cursor_left_right (GucharmapChartable *chartable,
                                            int count)
{
  GucharmapChartablePrivate *priv = chartable->priv;
  GtkWidget *widget = GTK_WIDGET (chartable);
  gboolean is_rtl;
  int offset, new_cell;

  is_rtl = (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL);
  offset = is_rtl ? -count : count;
  new_cell = priv->active_cell + offset;

  if (new_cell >= 0 &&
      new_cell <= priv->last_cell)
    gucharmap_chartable_set_active_cell (chartable, new_cell);
}

static void
gucharmap_chartable_move_cursor_up_down (GucharmapChartable *chartable,
                                         int count)
{
  GucharmapChartablePrivate *priv = chartable->priv;
  int new_cell;

  new_cell = priv->active_cell + priv->cols * count;
  if (new_cell >= 0 &&
      new_cell <= priv->last_cell)
    gucharmap_chartable_set_active_cell (chartable, new_cell);
}

static void
gucharmap_chartable_move_cursor_page_up_down (GucharmapChartable *chartable,
                                              int count)
{
  GucharmapChartablePrivate *priv = chartable->priv;
  int page_size, new_cell;

  page_size = priv->cols * priv->rows;
  new_cell = priv->active_cell + page_size * count;
  new_cell = CLAMP (new_cell, 0, priv->last_cell);
  gucharmap_chartable_set_active_cell (chartable, new_cell);
}

static void
gucharmap_chartable_move_cursor_start_end (GucharmapChartable *chartable,
                                           int count)
{
  GucharmapChartablePrivate *priv = chartable->priv;
  int new_cell;

  if (count > 0)
    new_cell = priv->last_cell;
  else
    new_cell = 0;

  gucharmap_chartable_set_active_cell (chartable, new_cell);
}

static gboolean
gucharmap_chartable_move_cursor (GucharmapChartable *chartable,
                                 GtkMovementStep     step,
                                 int                 count)
{
  g_return_val_if_fail (step == GTK_MOVEMENT_LOGICAL_POSITIONS ||
                        step == GTK_MOVEMENT_VISUAL_POSITIONS ||
                        step == GTK_MOVEMENT_DISPLAY_LINES ||
                        step == GTK_MOVEMENT_PAGES ||
                        step == GTK_MOVEMENT_BUFFER_ENDS, FALSE);

  switch (step)
    {
    case GTK_MOVEMENT_LOGICAL_POSITIONS:
    case GTK_MOVEMENT_VISUAL_POSITIONS:
      gucharmap_chartable_move_cursor_left_right (chartable, count);
      break;
    case GTK_MOVEMENT_DISPLAY_LINES:
      gucharmap_chartable_move_cursor_up_down (chartable, count);
      break;
    case GTK_MOVEMENT_PAGES:
      gucharmap_chartable_move_cursor_page_up_down (chartable, count);
      break;
    case GTK_MOVEMENT_BUFFER_ENDS:
      gucharmap_chartable_move_cursor_start_end (chartable, count);
      break;
    default:
      g_assert_not_reached ();
    }

  _gucharmap_chartable_redraw (chartable, TRUE);

  return TRUE;
}

static void
gucharmap_chartable_copy_clipboard (GucharmapChartable *chartable)
{
  GtkClipboard *clipboard;
  gunichar wc;
  gchar utf8[7];
  gsize len;

  wc = gucharmap_chartable_get_active_character (chartable);
  if (!gucharmap_unichar_validate (wc))
    return;

  len = g_unichar_to_utf8 (wc, utf8);

  clipboard = gtk_widget_get_clipboard (GTK_WIDGET (chartable),
                                        GDK_SELECTION_CLIPBOARD);
  gtk_clipboard_set_text (clipboard, utf8, len);
}

static void
gucharmap_chartable_paste_received_cb (GtkClipboard *clipboard,
                                       const char *text,
                                       gpointer user_data)
{
  gpointer *data = (gpointer *) user_data;
  GucharmapChartable *chartable = *data;
  gunichar wc;

  g_slice_free (gpointer, data);

  if (!chartable)
    return;

  g_object_remove_weak_pointer (G_OBJECT (chartable), data);

  if (!text)
    return;

  wc = g_utf8_get_char_validated (text, -1);
  if (wc == 0 ||
      !gucharmap_unichar_validate (wc)) {
    gtk_widget_error_bell (GTK_WIDGET (chartable));
    return;
  }

  gucharmap_chartable_set_active_character (chartable, wc);
}

static void
gucharmap_chartable_paste_clipboard (GucharmapChartable *chartable)
{
  GtkClipboard *clipboard;
  gpointer *data;

#if GTK_CHECK_VERSION (2, 20, 0)
  if (!gtk_widget_get_realized (GTK_WIDGET (chartable)))
#else
  if (!GTK_WIDGET_REALIZED (chartable))
#endif
    return;

  data = g_slice_new (gpointer);
  *data = chartable;
  g_object_add_weak_pointer (G_OBJECT (chartable), data);

  clipboard = gtk_widget_get_clipboard (GTK_WIDGET (chartable),
                                        GDK_SELECTION_CLIPBOARD);
  gtk_clipboard_request_text (clipboard,
                              gucharmap_chartable_paste_received_cb,
                              data);
}

/* does all the initial construction */
static void
gucharmap_chartable_init (GucharmapChartable *chartable)
{
  GtkWidget *widget = GTK_WIDGET (chartable);
  GucharmapChartablePrivate *priv;

  priv = chartable->priv = G_TYPE_INSTANCE_GET_PRIVATE (chartable, GUCHARMAP_TYPE_CHARTABLE, GucharmapChartablePrivate);
  
  priv->page_first_cell = 0;
  priv->active_cell = 0;
  priv->rows = 1;
  priv->cols = 1;
  priv->zoom_mode_enabled = TRUE;
  priv->zoom_window = NULL;
  priv->zoom_image = NULL;
  priv->snap_pow2_enabled = FALSE;

/* This didn't fix the slow expose events either: */
/*  gtk_widget_set_double_buffered (widget, FALSE); */

  gtk_widget_set_events (widget,
          GDK_EXPOSURE_MASK | GDK_KEY_PRESS_MASK | GDK_BUTTON_PRESS_MASK |
          GDK_BUTTON_RELEASE_MASK | GDK_BUTTON3_MOTION_MASK |
          GDK_BUTTON1_MOTION_MASK | GDK_FOCUS_CHANGE_MASK | GDK_SCROLL_MASK);

  priv->target_list = gtk_target_list_new (NULL, 0);
  gtk_target_list_add_text_targets (priv->target_list, 0);

  gtk_drag_dest_set (widget, GTK_DEST_DEFAULT_ALL,
                     NULL, 0,
                     GDK_ACTION_COPY);
  gtk_drag_dest_add_text_targets (widget);

  /* this is required to get key_press events */
#if GTK_CHECK_VERSION (2, 18, 0)
  gtk_widget_set_can_focus (widget, TRUE);
#else
  GTK_WIDGET_SET_FLAGS (widget, GTK_CAN_FOCUS);
#endif

  gucharmap_chartable_set_codepoint_list (chartable, NULL);

  gtk_widget_show_all (GTK_WIDGET (chartable));
}

static void
gucharmap_chartable_finalize (GObject *object)
{
  GucharmapChartable *chartable = GUCHARMAP_CHARTABLE (object);
  GucharmapChartablePrivate *priv = chartable->priv;

  if (priv->font_desc)
    pango_font_description_free (priv->font_desc);

  if (priv->pango_layout)
    g_object_unref (priv->pango_layout);

  gtk_target_list_unref (priv->target_list);

  if (priv->codepoint_list)
    g_object_unref (priv->codepoint_list);

  destroy_zoom_window (chartable);

  G_OBJECT_CLASS (gucharmap_chartable_parent_class)->finalize (object);
}

static void
gucharmap_chartable_set_property (GObject *object,
                                  guint prop_id,
                                  const GValue *value,
                                  GParamSpec *pspec)
{
  GucharmapChartable *chartable = GUCHARMAP_CHARTABLE (object);

  switch (prop_id) {
    case PROP_ACTIVE_CHAR:
      gucharmap_chartable_set_active_character (chartable, g_value_get_uint (value));
      break;
    case PROP_CODEPOINT_LIST:
      gucharmap_chartable_set_codepoint_list (chartable, g_value_get_object (value));
      break;
    case PROP_FONT_DESC:
      gucharmap_chartable_set_font_desc (chartable, g_value_get_boxed (value));
      break;
    case PROP_SNAP_POW2:
      gucharmap_chartable_set_snap_pow2 (chartable, g_value_get_boolean (value));
      break;
    case PROP_ZOOM_ENABLED:
      gucharmap_chartable_set_zoom_enabled (chartable, g_value_get_boolean (value));
      break;
    case PROP_ZOOM_SHOWING:
      /* not writable */
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gucharmap_chartable_get_property (GObject *object,
                                  guint prop_id,
                                  GValue *value,
                                  GParamSpec *pspec)
{
  GucharmapChartable *chartable = GUCHARMAP_CHARTABLE (object);
  GucharmapChartablePrivate *priv = chartable->priv;

  switch (prop_id) {
    case PROP_ACTIVE_CHAR:
      g_value_set_uint (value, gucharmap_chartable_get_active_character (chartable));
      break;
    case PROP_CODEPOINT_LIST:
      g_value_set_object (value, gucharmap_chartable_get_codepoint_list (chartable));
      break;
    case PROP_FONT_DESC:
      g_value_set_boxed (value, gucharmap_chartable_get_font_desc (chartable));
      break;
    case PROP_SNAP_POW2:
      g_value_set_boolean (value, priv->snap_pow2_enabled);
      break;
    case PROP_ZOOM_ENABLED:
      g_value_set_boolean (value, priv->zoom_mode_enabled);
      break;
    case PROP_ZOOM_SHOWING:
      g_value_set_boolean (value, priv->zoom_window != NULL);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gucharmap_chartable_class_init (GucharmapChartableClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkBindingSet *binding_set;

  g_type_class_add_private (object_class, sizeof (GucharmapChartablePrivate));

  object_class->finalize = gucharmap_chartable_finalize;
  object_class->get_property = gucharmap_chartable_get_property;
  object_class->set_property = gucharmap_chartable_set_property;

  widget_class->drag_begin = gucharmap_chartable_drag_begin;
  widget_class->drag_data_get = gucharmap_chartable_drag_data_get;
  widget_class->drag_data_received = gucharmap_chartable_drag_data_received;
  widget_class->button_press_event = gucharmap_chartable_button_press;
  widget_class->button_release_event = gucharmap_chartable_button_release;
  widget_class->expose_event = gucharmap_chartable_expose_event;
  widget_class->focus_in_event = gucharmap_chartable_focus_in_event;
  widget_class->focus_out_event = gucharmap_chartable_focus_out_event;
  widget_class->key_press_event = gucharmap_chartable_key_press_event;
  widget_class->key_release_event = gucharmap_chartable_key_release_event;
  widget_class->motion_notify_event = gucharmap_chartable_motion_notify;
  widget_class->realize = gucharmap_chartable_realize;
  widget_class->size_allocate = gucharmap_chartable_size_allocate;
  widget_class->size_request = gucharmap_chartable_size_request;
  widget_class->style_set = gucharmap_chartable_style_set;
#ifdef ENABLE_ACCESSIBLE
  widget_class->get_accessible = gucharmap_chartable_get_accessible;
#endif

  klass->set_scroll_adjustments = gucharmap_chartable_set_adjustments;
  klass->move_cursor = gucharmap_chartable_move_cursor;
  klass->activate = NULL;
  klass->copy_clipboard = gucharmap_chartable_copy_clipboard;
  klass->paste_clipboard = gucharmap_chartable_paste_clipboard;
  klass->set_active_char = NULL;

  widget_class->activate_signal = signals[ACTIVATE] =
    g_signal_new (I_("activate"),
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (GucharmapChartableClass, activate),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  /**
   * GucharmapChartable::set-scroll-adjustments
   * @horizontal: the horizontal #GtkAdjustment
   * @vertical: the vertical #GtkAdjustment
   *
   * Set the scroll adjustments for the text view. Usually scrolled containers
   * like #GtkScrolledWindow will emit this signal to connect two instances
   * of #GtkScrollbar to the scroll directions of the #GucharmapChartable.
   */
  widget_class->set_scroll_adjustments_signal =
    g_signal_new (I_("set-scroll-adjustments"),
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GucharmapChartableClass, set_scroll_adjustments),
                  NULL, NULL, 
                  _gucharmap_marshal_VOID__OBJECT_OBJECT,
                  G_TYPE_NONE, 2,
                  GTK_TYPE_ADJUSTMENT, GTK_TYPE_ADJUSTMENT);

  signals[STATUS_MESSAGE] =
    g_signal_new (I_("status-message"), gucharmap_chartable_get_type (), G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GucharmapChartableClass, status_message),
                  NULL, NULL, g_cclosure_marshal_VOID__STRING, G_TYPE_NONE,
                  1, G_TYPE_STRING);

  signals[MOVE_CURSOR] =
    g_signal_new (I_("move-cursor"),
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (GucharmapChartableClass, move_cursor),
                  NULL, NULL,
                  _gucharmap_marshal_BOOLEAN__ENUM_INT,
                  G_TYPE_BOOLEAN, 2,
                  GTK_TYPE_MOVEMENT_STEP,
                  G_TYPE_INT);

  signals[COPY_CLIPBOARD] =
    g_signal_new (I_("copy-clipboard"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (GucharmapChartableClass, copy_clipboard),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  signals[PASTE_CLIPBOARD] =
    g_signal_new (I_("paste-clipboard"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (GucharmapChartableClass, paste_clipboard),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  /* Not using g_param_spec_unichar on purpose, since it disallows certain values
   * we want (it's performing a g_unichar_validate).
   */
  g_object_class_install_property
    (object_class,
     PROP_ACTIVE_CHAR,
     g_param_spec_uint ("active-character", NULL, NULL,
                        0,
                        UNICHAR_MAX,
                        0,
                        G_PARAM_READWRITE |
                        G_PARAM_STATIC_NAME |
                        G_PARAM_STATIC_NICK |
                        G_PARAM_STATIC_BLURB));

  g_object_class_install_property
    (object_class,
     PROP_CODEPOINT_LIST,
     g_param_spec_object ("codepoint-list", NULL, NULL,
                          gucharmap_codepoint_list_get_type (),
                          G_PARAM_READWRITE |
                          G_PARAM_STATIC_NAME |
                          G_PARAM_STATIC_NICK |
                          G_PARAM_STATIC_BLURB));

  g_object_class_install_property
    (object_class,
     PROP_FONT_DESC,
     g_param_spec_boxed ("font-desc", NULL, NULL,
                         PANGO_TYPE_FONT_DESCRIPTION,
                         G_PARAM_READWRITE |
                         G_PARAM_STATIC_NAME |
                         G_PARAM_STATIC_NICK |
                         G_PARAM_STATIC_BLURB));

  g_object_class_install_property
    (object_class,
     PROP_SNAP_POW2,
     g_param_spec_boolean ("snap-power-2", NULL, NULL,
                           FALSE,
                           G_PARAM_READWRITE |
                           G_PARAM_STATIC_NAME |
                           G_PARAM_STATIC_NICK |
                           G_PARAM_STATIC_BLURB));

  g_object_class_install_property
    (object_class,
     PROP_ZOOM_ENABLED,
     g_param_spec_boolean ("zoom-enabled", NULL, NULL,
                           FALSE,
                           G_PARAM_READWRITE |
                           G_PARAM_STATIC_NAME |
                           G_PARAM_STATIC_NICK |
                           G_PARAM_STATIC_BLURB));

  g_object_class_install_property
    (object_class,
     PROP_ZOOM_SHOWING,
     g_param_spec_boolean ("zoom-showing", NULL, NULL,
                           FALSE,
                           G_PARAM_READABLE |
                           G_PARAM_STATIC_NAME |
                           G_PARAM_STATIC_NICK |
                           G_PARAM_STATIC_BLURB));

  /* Keybindings */
  binding_set = gtk_binding_set_by_class (klass);

  /* Cursor movement */
  gucharmap_chartable_add_move_binding (binding_set, GDK_Up, 0,
                                        GTK_MOVEMENT_DISPLAY_LINES, -1);
  gucharmap_chartable_add_move_binding (binding_set, GDK_KP_Up, 0,
                                        GTK_MOVEMENT_DISPLAY_LINES, -1);

  gucharmap_chartable_add_move_binding (binding_set, GDK_Down, 0,
                                        GTK_MOVEMENT_DISPLAY_LINES, 1);
  gucharmap_chartable_add_move_binding (binding_set, GDK_KP_Down, 0,
                                        GTK_MOVEMENT_DISPLAY_LINES, 1);

  gucharmap_chartable_add_move_binding (binding_set, GDK_p, GDK_CONTROL_MASK,
                                        GTK_MOVEMENT_DISPLAY_LINES, -1);

  gucharmap_chartable_add_move_binding (binding_set, GDK_n, GDK_CONTROL_MASK,
                                        GTK_MOVEMENT_DISPLAY_LINES, 1);

  gucharmap_chartable_add_move_binding (binding_set, GDK_Home, 0,
                                        GTK_MOVEMENT_BUFFER_ENDS, -1);
  gucharmap_chartable_add_move_binding (binding_set, GDK_KP_Home, 0,
                                        GTK_MOVEMENT_BUFFER_ENDS, -1);

  gucharmap_chartable_add_move_binding (binding_set, GDK_End, 0,
                                        GTK_MOVEMENT_BUFFER_ENDS, 1);
  gucharmap_chartable_add_move_binding (binding_set, GDK_KP_End, 0,
                                        GTK_MOVEMENT_BUFFER_ENDS, 1);

  gucharmap_chartable_add_move_binding (binding_set, GDK_Page_Up, 0,
                                        GTK_MOVEMENT_PAGES, -1);
  gucharmap_chartable_add_move_binding (binding_set, GDK_KP_Page_Up, 0,
                                        GTK_MOVEMENT_PAGES, -1);

  gucharmap_chartable_add_move_binding (binding_set, GDK_Page_Down, 0,
                                        GTK_MOVEMENT_PAGES, 1);
  gucharmap_chartable_add_move_binding (binding_set, GDK_KP_Page_Down, 0,
                                        GTK_MOVEMENT_PAGES, 1);

  gucharmap_chartable_add_move_binding (binding_set, GDK_Left, 0,
                                        GTK_MOVEMENT_VISUAL_POSITIONS, -1);
  gucharmap_chartable_add_move_binding (binding_set, GDK_KP_Left, 0,
                                        GTK_MOVEMENT_VISUAL_POSITIONS, -1);

  gucharmap_chartable_add_move_binding (binding_set, GDK_Right, 0,
                                        GTK_MOVEMENT_VISUAL_POSITIONS, 1);
  gucharmap_chartable_add_move_binding (binding_set, GDK_KP_Right, 0,
                                        GTK_MOVEMENT_VISUAL_POSITIONS, 1);
  
  /* Activate */
  gtk_binding_entry_add_signal (binding_set, GDK_Return, 0,
                                "activate", 0);
  gtk_binding_entry_add_signal (binding_set, GDK_ISO_Enter, 0,
                                "activate", 0);
  gtk_binding_entry_add_signal (binding_set, GDK_KP_Enter, 0,
                                "activate", 0);
  gtk_binding_entry_add_signal (binding_set, GDK_space, 0,
                                "activate", 0);

  /* Clipboard actions */
  gtk_binding_entry_add_signal (binding_set, GDK_c, GDK_CONTROL_MASK,
                                "copy-clipboard", 0);
  gtk_binding_entry_add_signal (binding_set, GDK_Insert, GDK_CONTROL_MASK,
                                "copy-clipboard", 0);
  gtk_binding_entry_add_signal (binding_set, GDK_v, GDK_CONTROL_MASK,
                                "paste-clipboard", 0);
  gtk_binding_entry_add_signal (binding_set, GDK_Insert, GDK_SHIFT_MASK,
                                "paste-clipboard", 0);

#if 0
  /* VI keybindings */
  gucharmap_chartable_add_move_binding (binding_set, GDK_k, 0,
                                        GTK_MOVEMENT_DISPLAY_LINES, -1);
  gucharmap_chartable_add_move_binding (binding_set, GDK_K, 0,
                                        GTK_MOVEMENT_DISPLAY_LINES, -1);

  gucharmap_chartable_add_move_binding (binding_set, GDK_j, 0,
                                        GTK_MOVEMENT_DISPLAY_LINES, 1);
  gucharmap_chartable_add_move_binding (binding_set, GDK_J, 0,
                                        GTK_MOVEMENT_DISPLAY_LINES, 1);

  gucharmap_chartable_add_move_binding (binding_set, GDK_b, 0,
                                        GTK_MOVEMENT_PAGES, -1);
  gucharmap_chartable_add_move_binding (binding_set, GDK_B, 0,
                                        GTK_MOVEMENT_PAGES, -1);

  gucharmap_chartable_add_move_binding (binding_set, GDK_h, 0,
                                        GTK_MOVEMENT_VISUAL_POSITIONS, -1);
  gucharmap_chartable_add_move_binding (binding_set, GDK_H, 0,
                                        GTK_MOVEMENT_VISUAL_POSITIONS, -1);

  gucharmap_chartable_add_move_binding (binding_set, GDK_l, 0,
                                        GTK_MOVEMENT_VISUAL_POSITIONS, 1);
  gucharmap_chartable_add_move_binding (binding_set, GDK_L, 0,
                                        GTK_MOVEMENT_VISUAL_POSITIONS, 1);
#endif
}

/* public API */

/**
 * gucharmap_chartable_new:
 *
 * Returns: a new #GucharmapChartable
 */
GtkWidget *
gucharmap_chartable_new (void)
{
  return GTK_WIDGET (g_object_new (gucharmap_chartable_get_type (), NULL));
}

/**
 * gucharmap_chartable_set_zoom_enabled:
 * @chartable: a #GucharmapChartable
 * @enabled: whether to enable zoom mode
 *
 * Enables or disables the zoom popup.
 */
void
gucharmap_chartable_set_zoom_enabled (GucharmapChartable *chartable,
                                      gboolean enabled)
{
  GucharmapChartablePrivate *priv;
  GObject *object;

  g_return_if_fail (GUCHARMAP_IS_CHARTABLE (chartable));

  priv = chartable->priv;

  enabled = enabled != FALSE;
  if (priv->zoom_mode_enabled == enabled)
    return;

  object = G_OBJECT (chartable);
  g_object_freeze_notify (object);

  priv->zoom_mode_enabled = enabled;
  if (!enabled)
    gucharmap_chartable_hide_zoom (chartable);

  g_object_notify (object, "zoom-enabled");
  g_object_thaw_notify (object);
}

/**
 * gucharmap_chartable_get_zoom_enabled:
 * @chartable: a #GucharmapChartable
 *
 * Returns: whether zooming is enabled
 */
gboolean
gucharmap_chartable_get_zoom_enabled (GucharmapChartable *chartable)
{
  g_return_val_if_fail (GUCHARMAP_IS_CHARTABLE (chartable), FALSE);

  return chartable->priv->zoom_mode_enabled;
}

/**
 * gucharmap_chartable_set_font_desc:
 * @chartable: a #GucharmapChartable
 * @font_desc: a #PangoFontDescription
 *
 * Sets @font_desc as the font to use to display the character table.
 */
void
gucharmap_chartable_set_font_desc (GucharmapChartable *chartable,
                                   PangoFontDescription *font_desc)
{
  GucharmapChartablePrivate *priv;

  g_return_if_fail (GUCHARMAP_IS_CHARTABLE (chartable));
  g_return_if_fail (font_desc != NULL);

  priv = chartable->priv;

  if (priv->font_desc &&
      pango_font_description_equal (font_desc, priv->font_desc))
    return;

  gucharmap_chartable_set_font_desc_internal (chartable,
                                              pango_font_description_copy (font_desc));
}

/**
 * gucharmap_chartable_get_font_desc:
 * @chartable: a #GucharmapChartable
 *
 * Returns: the #PangoFontDescription used to display the character table.
 *   The returned object is owned by @chartable and must not be modified or freed.
 */
PangoFontDescription *
gucharmap_chartable_get_font_desc (GucharmapChartable *chartable)
{
  g_return_val_if_fail (GUCHARMAP_IS_CHARTABLE (chartable), NULL);

  return chartable->priv->font_desc;
}

/**
 * gucharmap_chartable_get_active_character:
 * @chartable: a #GucharmapChartable
 *
 * Returns: the currently selected character
 */
gunichar
gucharmap_chartable_get_active_character (GucharmapChartable *chartable)
{
  GucharmapChartablePrivate *priv = chartable->priv;

  if (!priv->codepoint_list)
    return 0;

  return gucharmap_codepoint_list_get_char (priv->codepoint_list, priv->active_cell);
}

/**
 * gucharmap_chartable_get_active_character:
 * @chartable: a #GucharmapChartable
 * @wc: a unicode character (UTF-32)
 *
 * Sets @wc as the selected character.
 */
void
gucharmap_chartable_set_active_character (GucharmapChartable *chartable, 
                                          gunichar wc)
{
  set_active_char (chartable, wc);
  _gucharmap_chartable_redraw (chartable, TRUE);
}

/**
 * gucharmap_chartable_set_snap_pow2:
 * @chartable: a #GucharmapChartable
 * @snap: whether to enable or disable snapping
 *
 * Sets whether the number columns the character table shows should
 * always be a power of 2.
 */
void
gucharmap_chartable_set_snap_pow2 (GucharmapChartable *chartable,
                                   gboolean snap)
{
  GucharmapChartablePrivate *priv = chartable->priv;

  snap = snap != FALSE;

  if (snap != priv->snap_pow2_enabled)
    {
      priv->snap_pow2_enabled = snap;

      gtk_widget_queue_resize (GTK_WIDGET (chartable));

      g_object_notify (G_OBJECT (chartable), "snap-power-2");
    }
}

/**
 * gucharmap_chartable_get_snap_pow2:
 * @chartable: a #GucharmapChartable
 *
 * Returns whether the number of columns the character table shows is
 * always a power of 2.
 */
gboolean
gucharmap_chartable_get_snap_pow2 (GucharmapChartable *chartable)
{
  GucharmapChartablePrivate *priv = chartable->priv;

  return priv->snap_pow2_enabled;
}

/**
 * gucharmap_chartable_get_active_character:
 * @chartable: a #GucharmapChartable
 * @list: a #GucharmapCodepointList
 *
 * Sets the codepoint list to show in the character table.
 */
void
gucharmap_chartable_set_codepoint_list (GucharmapChartable     *chartable,
                                        GucharmapCodepointList *codepoint_list)
{
  GucharmapChartablePrivate *priv = chartable->priv;
  GObject *object = G_OBJECT (chartable);
  GtkWidget *widget = GTK_WIDGET (chartable);

  g_object_freeze_notify (object);

  if (codepoint_list)
    g_object_ref (codepoint_list);
  if (priv->codepoint_list)
    g_object_unref (priv->codepoint_list);
  priv->codepoint_list = codepoint_list;
  priv->codepoint_list_changed = TRUE;

  priv->active_cell = 0;
  priv->page_first_cell = 0;
  if (codepoint_list)
    priv->last_cell = gucharmap_codepoint_list_get_last_index (codepoint_list);
  else
    priv->last_cell = 0;

  /* force pixmap to be redrawn */
  if (priv->pixmap != NULL)
    g_object_unref (priv->pixmap);
  priv->pixmap = NULL;

  g_object_notify (object, "codepoint-list");
  g_object_notify (object, "active-character");

  update_scrollbar_adjustment (chartable);

  gtk_widget_queue_draw (widget);

  g_object_thaw_notify (object);
}

/**
 * gucharmap_chartable_get_codepoint_list:
 * @chartable: a #GucharmapChartable
 *
 * Returns: the current codepoint list
 */
GucharmapCodepointList *
gucharmap_chartable_get_codepoint_list (GucharmapChartable *chartable)
{
  GucharmapChartablePrivate *priv = chartable->priv;

  return priv->codepoint_list;
}
