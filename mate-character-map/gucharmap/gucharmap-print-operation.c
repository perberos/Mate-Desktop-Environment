/*
 *  Copyright © 2007, 2008 Christian Persch
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope print_operation it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include <gtk/gtk.h>

#include "gucharmap-print-operation.h"

#define GUCHARMAP_PRINT_OPERATION_GET_PRIVATE(print_operation)(G_TYPE_INSTANCE_GET_PRIVATE ((print_operation), GUCHARMAP_TYPE_PRINT_OPERATION, GucharmapPrintOperationPrivate))

#define GRID_LINE_WIDTH (0.25)

struct _GucharmapPrintOperationPrivate
{
  GucharmapCodepointList *codepoint_list;
  PangoFontDescription *font_desc;
  PangoLayout *character_layout;
  PangoLayout *info_text_layout;
  int last_codepoint_index;
  int n_columns;
  int n_rows;
  double width;
  double height;
  double character_width;
  double character_height;
  double info_text_width;
  double info_text_height;
  double info_text_gap;
  double cell_width;
  double cell_height;
  double cell_margin_left;
  double cell_margin_right;
  double cell_margin_top;
  double cell_margin_bottom;
};

enum
{
  PROP_0,
  PROP_CODEPOINT_LIST,
  PROP_FONT_DESC
};

G_DEFINE_TYPE (GucharmapPrintOperation, gucharmap_print_operation, GTK_TYPE_PRINT_OPERATION)

/* helper functions */

static void
draw_character_cell (GucharmapPrintOperation *print_operation,
                     cairo_t *cr,
                     int row,
                     int col,
                     int character_index)
{
  GucharmapPrintOperationPrivate *priv = print_operation->priv;
  gunichar wc;
  char utf8[7];
  int len;
  double x, y;
  PangoRectangle ink_rect;

  wc = gucharmap_codepoint_list_get_char (priv->codepoint_list, character_index);
  if (!gucharmap_unichar_validate (wc))
    return; /* FIXME print info nevertheless */

  len = g_unichar_to_utf8 (wc, utf8);
  pango_layout_set_text (priv->character_layout, utf8, len);

  pango_layout_get_extents (priv->character_layout, &ink_rect, NULL);

  /* FIXME: RTL? */
  x = col * priv->cell_width + priv->cell_margin_left;
  y = row * priv->cell_height + priv->cell_margin_top;

  cairo_move_to (cr, x, y);
  pango_cairo_show_layout (cr, priv->character_layout);

  cairo_set_line_width (cr, GRID_LINE_WIDTH);
  cairo_rectangle (cr,
                   col * priv->cell_width,
                   row * priv->cell_height,
                   priv->cell_width,
                   priv->cell_height);
  cairo_stroke (cr);
}

static void
draw_cell_grid (GucharmapPrintOperation *print_operation,
                cairo_t *cr,
                int last_row,
                int last_col)
{
#if 0
  GucharmapPrintOperationPrivate *priv = print_operation->priv;
  int row, col;
  double x, y;

  cairo_set_line_width (cr, GRID_LINE_WIDTH);

  x = y = 0;
  for (row = 0; row <= priv->n_rows; ++row) {
    cairo_move_to (cr, x, y);
    cairo_line_to (cr, priv->width, y);

    y += priv->cell_height;
  }

  x = y = 0;
  for (col = 0; col <= priv->n_columns; ++col) {
    cairo_move_to (cr, x, y);
    cairo_line_to (cr, x, priv->height);

    x += priv->cell_width;
  }

  cairo_stroke (cr);
#endif
}

/* GtkPrintOperation class implementation */

/* Before first draw_page */
static void
gucharmap_print_operation_begin_print (GtkPrintOperation *operation,
                                       GtkPrintContext   *context)
{
  GucharmapPrintOperation *print_operation = GUCHARMAP_PRINT_OPERATION (operation);
  GucharmapPrintOperationPrivate *priv = print_operation->priv;
  PangoFontDescription *info_font_desc;
  double width, height;
  int page_size, pages;

  priv->last_codepoint_index = gucharmap_codepoint_list_get_last_index (priv->codepoint_list);
  g_print ("last_codepoint_index %d\n", priv->last_codepoint_index);
  if (priv->last_codepoint_index < 0) {
    /* Nothing to print */
    gtk_print_operation_cancel (operation);
    return;
  }

  gtk_print_operation_set_unit (operation, GTK_UNIT_POINTS);
  
  width = priv->width = gtk_print_context_get_width (context);
  height = priv->height = gtk_print_context_get_height (context);

#define N_COLS 8
#define MARGIN 5 /* pt */

  priv->n_columns = N_COLS;

  priv->cell_width = width / N_COLS;
  priv->cell_height = priv->cell_width;

  priv->info_text_width = 0;
  priv->info_text_height = 0;
  priv->info_text_gap = 0;

  priv->cell_margin_left = priv->cell_margin_right = priv->cell_margin_top = priv->cell_margin_bottom = MARGIN;
  //XXX  calculate cell_width/height

  priv->character_width = priv->cell_width -
                          priv->cell_margin_left -
                          priv->cell_margin_right;
  priv->character_height = priv->cell_height -
                           priv->cell_margin_top -
                           priv->cell_margin_bottom -
                           priv->info_text_gap -
                           priv->info_text_height;

  priv->n_rows = height / priv->cell_height;
  page_size = priv->n_rows * priv->n_columns;
  pages = (priv->last_codepoint_index + page_size) / page_size;

  gtk_print_operation_set_n_pages (operation, pages);

  g_print ("cols: %d rows: %d pages: %d\n", priv->n_columns, priv->n_rows, pages);
  
  priv->character_layout = gtk_print_context_create_pango_layout (context);
  pango_layout_set_font_description (priv->character_layout, priv->font_desc);
  pango_layout_set_width (priv->character_layout, priv->character_width);
//   pango_layout_set_height (priv->character_layout, priv->character_height);

  priv->info_text_layout = gtk_print_context_create_pango_layout (context);

  info_font_desc = pango_font_description_from_string ("Sans 8");
  pango_layout_set_font_description (priv->info_text_layout, info_font_desc);
  pango_font_description_free (info_font_desc);

  pango_layout_set_width (priv->info_text_layout, priv->cell_width);
}

static void
gucharmap_print_operation_draw_page (GtkPrintOperation *operation,
                                     GtkPrintContext   *context,
                                     int                page_nr)
{
  GucharmapPrintOperation *print_operation = GUCHARMAP_PRINT_OPERATION (operation);
  GucharmapPrintOperationPrivate *priv = print_operation->priv;
  cairo_t *cr;
  int row, col, page_size;
  int i, start_index, last_index;

  page_size = priv->n_rows * priv->n_columns;
  start_index = page_nr * page_size;
  last_index = start_index + page_size - 1;
  last_index = MIN (last_index, priv->last_codepoint_index);
  /* g_assert (i <= priv->last_codepoint_index); */

  cr = gtk_print_context_get_cairo_context (context);

  cairo_set_source_rgb (cr, 0, 0, 0);

  col = row = 0;
  for (i = start_index; i <= last_index; ++i) {
    draw_character_cell (print_operation, cr, row, col, i);

    if (++col == priv->n_columns) {
      ++row;
      col = 0;
    }
  }

  draw_cell_grid (print_operation, cr, row, col);
}
  
/* After last draw_page */
static void
gucharmap_print_operation_end_print (GtkPrintOperation *operation,
                                     GtkPrintContext   *context)
{
  GucharmapPrintOperation *print_operation = GUCHARMAP_PRINT_OPERATION (operation);
  GucharmapPrintOperationPrivate *priv = print_operation->priv;

  g_object_unref (priv->character_layout);
  priv->character_layout = NULL;

  g_object_unref (priv->info_text_layout);
  priv->info_text_layout = NULL;
}
  
static GtkWidget *
gucharmap_print_operation_create_custom_widget (GtkPrintOperation *operation)
{
/*  GucharmapPrintOperation *print_operation = GUCHARMAP_PRINT_OPERATION (operation);
  GucharmapPrintOperationPrivate *priv = print_operation->priv;*/
  return GTK_PRINT_OPERATION_CLASS (gucharmap_print_operation_parent_class)->create_custom_widget (operation);
}

static void
gucharmap_print_operation_custom_widget_apply (GtkPrintOperation *operation,
                                               GtkWidget *widget)
{
/*  GucharmapPrintOperation *print_operation = GUCHARMAP_PRINT_OPERATION (operation);
  GucharmapPrintOperationPrivate *priv = print_operation->priv;*/
}

/* GObject class implementation */

static void
gucharmap_print_operation_init (GucharmapPrintOperation *print_operation)
{
  GucharmapPrintOperationPrivate *priv;

  priv = print_operation->priv = GUCHARMAP_PRINT_OPERATION_GET_PRIVATE (print_operation);
}

static GObject *
gucharmap_print_operation_constructor (GType type,
                                       guint n_construct_properties,
                                       GObjectConstructParam *construct_params)
{
  GObject *object;
  GucharmapPrintOperation *print_operation;
  GucharmapPrintOperationPrivate *priv;

  object = G_OBJECT_CLASS (gucharmap_print_operation_parent_class)->constructor
             (type, n_construct_properties, construct_params);

  print_operation = GUCHARMAP_PRINT_OPERATION (object);
  priv = print_operation->priv;

  g_assert (priv->codepoint_list != NULL);
  g_assert (priv->font_desc != NULL);

  return object;
}

static void
gucharmap_print_operation_finalize (GObject *object)
{
//   GucharmapPrintOperation *print_operation = GUCHARMAP_PRINT_OPERATION (object);
//   GucharmapPrintOperationPrivate *priv = print_operation->priv;

  G_OBJECT_CLASS (gucharmap_print_operation_parent_class)->finalize (object);
}

static void
gucharmap_print_operation_set_property (GObject *object,
			   guint prop_id,
			   const GValue *value,
			   GParamSpec *pspec)
{
  GucharmapPrintOperation *print_operation = GUCHARMAP_PRINT_OPERATION (object);
  GucharmapPrintOperationPrivate *priv = print_operation->priv;

  switch (prop_id) {
    case PROP_CODEPOINT_LIST:
      priv->codepoint_list = g_value_dup_object (value);
      break;
    case PROP_FONT_DESC:
      priv->font_desc = g_value_dup_boxed (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gucharmap_print_operation_class_init (GucharmapPrintOperationClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkPrintOperationClass *print_op_class = GTK_PRINT_OPERATION_CLASS (klass);

  gobject_class->constructor = gucharmap_print_operation_constructor;
  gobject_class->finalize = gucharmap_print_operation_finalize;
  gobject_class->set_property = gucharmap_print_operation_set_property;

  print_op_class->begin_print = gucharmap_print_operation_begin_print;
  print_op_class->draw_page = gucharmap_print_operation_draw_page;
  print_op_class->end_print = gucharmap_print_operation_end_print;
  print_op_class->create_custom_widget = gucharmap_print_operation_create_custom_widget;
  print_op_class->custom_widget_apply = gucharmap_print_operation_custom_widget_apply;

  g_object_class_install_property
    (gobject_class,
     PROP_CODEPOINT_LIST,
     g_param_spec_object ("codepoint-list", NULL, NULL,
                          GUCHARMAP_TYPE_CODEPOINT_LIST,
                          G_PARAM_WRITABLE | G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB |
                          G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property
    (gobject_class,
     PROP_FONT_DESC,
     g_param_spec_boxed ("font-desc", NULL, NULL,
                         PANGO_TYPE_FONT_DESCRIPTION,
                         G_PARAM_WRITABLE | G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB |
                         G_PARAM_CONSTRUCT_ONLY));

  g_type_class_add_private (gobject_class, sizeof (GucharmapPrintOperationPrivate));
}

/* public API */

GtkPrintOperation *
gucharmap_print_operation_new (GucharmapCodepointList *codepoint_list,
                               PangoFontDescription *font_desc)
{
  return g_object_new (GUCHARMAP_TYPE_PRINT_OPERATION,
                       "codepoint-list", codepoint_list,
                       "font-desc", font_desc,
                       NULL);
}
