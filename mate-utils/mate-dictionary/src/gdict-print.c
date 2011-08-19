/* gdict-print.c - print-related helper functions
 *
 * This file is part of MATE Dictionary
 *
 * Copyright (C) 2005 Emmanuele Bassi
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include <mateconf/mateconf-client.h>

#include <libgdict/gdict.h>

#include "gdict-pref-dialog.h"
#include "gdict-print.h"

#define HEADER_HEIGHT(lines)    ((lines) * 72 / 25.4)
#define HEADER_GAP(lines)       ((lines) * 72 / 25.4)

typedef struct _GdictPrintData
{
  GdictDefbox *defbox;
  gchar *word;

  PangoFontDescription *font_desc;
  gdouble font_size;

  gchar **lines;
  
  gint n_lines;
  gint lines_per_page;
  gint n_pages;
} GdictPrintData;

static void
begin_print (GtkPrintOperation *operation,
             GtkPrintContext   *context,
             gpointer           user_data)
{
  GdictPrintData *data = user_data;
  gchar *contents;
  gdouble height;

  height = gtk_print_context_get_height (context)
           - HEADER_HEIGHT (10)
           - HEADER_GAP (3);

  contents = gdict_defbox_get_text (data->defbox, NULL);

  data->lines = g_strsplit (contents, "\n", 0);
  data->n_lines = g_strv_length (data->lines);
  data->lines_per_page = floor (height / data->font_size);

  data->n_pages = (data->n_lines - 1) / data->lines_per_page + 1;

  gtk_print_operation_set_n_pages (operation, data->n_pages);

  g_free (contents);
}

static void
draw_page (GtkPrintOperation *operation,
           GtkPrintContext   *context,
           gint               page_number,
           gpointer           user_data)
{
  GdictPrintData *data = user_data;
  cairo_t *cr;
  PangoLayout *layout;
  gint text_width, text_height;
  gdouble width;
  gint line, i;
  PangoFontDescription *desc;
  gchar *page_str;

  cr = gtk_print_context_get_cairo_context (context);
  width = gtk_print_context_get_width (context);

  cairo_rectangle (cr, 0, 0, width, HEADER_HEIGHT (10));
  
  cairo_set_source_rgb (cr, 0.8, 0.8, 0.8);
  cairo_fill_preserve (cr);
  
  cairo_set_source_rgb (cr, 0, 0, 0);
  cairo_set_line_width (cr, 1);
  cairo_stroke (cr);

  /* header */
  layout = gtk_print_context_create_pango_layout (context);

  desc = pango_font_description_from_string ("sans 14");
  pango_layout_set_font_description (layout, desc);
  pango_font_description_free (desc);

  pango_layout_set_text (layout, data->word, -1);
  pango_layout_get_pixel_size (layout, &text_width, &text_height);

  if (text_width > width)
    {
      pango_layout_set_width (layout, width);
      pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_START);
      pango_layout_get_pixel_size (layout, &text_width, &text_height);
    }

  cairo_move_to (cr, (width - text_width) / 2,
                     (HEADER_HEIGHT (10) - text_height) / 2);
  pango_cairo_show_layout (cr, layout);

  page_str = g_strdup_printf ("%d/%d", page_number + 1, data->n_pages);
  pango_layout_set_text (layout, page_str, -1);
  g_free (page_str);

  pango_layout_set_width (layout, -1);
  pango_layout_get_pixel_size (layout, &text_width, &text_height);
  cairo_move_to (cr, width - text_width - 4,
                     (HEADER_HEIGHT (10) - text_height) / 2);
  pango_cairo_show_layout (cr, layout);
  
  g_object_unref (layout);

  /* text */
  layout = gtk_print_context_create_pango_layout (context);
  pango_font_description_set_size (data->font_desc,
                                   data->font_size * PANGO_SCALE);
  pango_layout_set_font_description (layout, data->font_desc);
  
  cairo_move_to (cr, 0, HEADER_HEIGHT (10) + HEADER_GAP (3));
  line = page_number * data->lines_per_page;
  for (i = 0; i < data->lines_per_page && line < data->n_lines; i++) 
    {
      pango_layout_set_text (layout, data->lines[line], -1);

      pango_cairo_show_layout (cr, layout);
      cairo_rel_move_to (cr, 0, data->font_size);
      line++;
    }

  g_object_unref (layout);
}

static void
end_print (GtkPrintOperation *operation,
           GtkPrintContext   *context,
           gpointer           user_data)
{
  GdictPrintData *data = user_data;

  pango_font_description_free (data->font_desc);
  g_free (data->word);
  g_strfreev (data->lines);
  g_free (data);
}

static gchar *
get_print_font (void)
{
  MateConfClient *client;
  gchar *print_font;
  
  client = mateconf_client_get_default ();
  print_font = mateconf_client_get_string (client,
  					GDICT_MATECONF_PRINT_FONT_KEY,
  					NULL);
  if (!print_font)
    print_font = g_strdup (GDICT_DEFAULT_PRINT_FONT);
  
  g_object_unref (client);
  
  return print_font;
}

void
gdict_show_print_preview (GtkWindow   *parent,
                          GdictDefbox *defbox)
{
  GtkPrintOperation *operation;
  GdictPrintData *data;
  gchar *print_font;
  gchar *word;
  GError *error;
  
  g_return_if_fail (parent == NULL || GTK_IS_WINDOW (parent));
  g_return_if_fail (GDICT_IS_DEFBOX (defbox));

  g_object_get (defbox, "word", &word, NULL);
  if (!word)
    {
      g_warning ("Preview should be disabled.");
      return;
    }

  data = g_new0 (GdictPrintData, 1);
  data->defbox = defbox;
  data->word = word;

  operation = gtk_print_operation_new ();
  print_font = get_print_font ();
  data->font_desc = pango_font_description_from_string (print_font);
  data->font_size = pango_font_description_get_size (data->font_desc)
                    / PANGO_SCALE;
  g_free (print_font);

  g_signal_connect (operation, "begin-print", 
		    G_CALLBACK (begin_print), data);
  g_signal_connect (operation, "draw-page", 
		    G_CALLBACK (draw_page), data);
  g_signal_connect (operation, "end-print", 
		    G_CALLBACK (end_print), data);

  error = NULL;
  gtk_print_operation_run (operation,
                           GTK_PRINT_OPERATION_ACTION_PREVIEW,
                           parent,
                           &error);
  g_object_unref (operation);

  if (error)
    {
      GtkWidget *dialog;

      dialog = gtk_message_dialog_new (parent,
                                       GTK_DIALOG_DESTROY_WITH_PARENT,
                                       GTK_MESSAGE_ERROR,
                                       GTK_BUTTONS_CLOSE,
				       _("Unable to display the preview: %s"),
                                       error->message);
      g_error_free (error);

      g_signal_connect (dialog, "response",
			G_CALLBACK (gtk_widget_destroy), NULL);

      gtk_widget_show (dialog);
    }
}

void
gdict_show_print_dialog (GtkWindow   *parent,
			 GdictDefbox *defbox)
{
  GtkPrintOperation *operation;
  GdictPrintData *data;
  gchar *print_font;
  gchar *word;
  GError *error;
  
  g_return_if_fail (parent == NULL || GTK_IS_WINDOW (parent));
  g_return_if_fail (GDICT_IS_DEFBOX (defbox));

  g_object_get (defbox, "word", &word, NULL);
  if (!word)
    {
      g_warning ("Print should be disabled.");
      return;
    }

  data = g_new0 (GdictPrintData, 1);
  data->defbox = defbox;
  data->word = word;

  operation = gtk_print_operation_new ();
  print_font = get_print_font ();
  data->font_desc = pango_font_description_from_string (print_font);
  data->font_size = pango_font_description_get_size (data->font_desc)
                    / PANGO_SCALE;
  g_free (print_font);

  g_signal_connect (operation, "begin-print", 
		    G_CALLBACK (begin_print), data);
  g_signal_connect (operation, "draw-page", 
		    G_CALLBACK (draw_page), data);
  g_signal_connect (operation, "end-print", 
		    G_CALLBACK (end_print), data);

  error = NULL;
  gtk_print_operation_run (operation,
                           GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
                           parent,
                           &error);
  g_object_unref (operation);

  if (error)
    {
      GtkWidget *dialog;

      dialog = gtk_message_dialog_new (parent,
                                       GTK_DIALOG_DESTROY_WITH_PARENT,
                                       GTK_MESSAGE_ERROR,
                                       GTK_BUTTONS_CLOSE,
				       _("Unable to display the preview: %s"),
                                       error->message);
      g_error_free (error);

      g_signal_connect (dialog, "response",
			G_CALLBACK (gtk_widget_destroy), NULL);

      gtk_widget_show (dialog);
    }
}
