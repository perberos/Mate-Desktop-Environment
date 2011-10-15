/*
 * scale.c
 *
 * Copyright (C) 2002 Sun Microsystems, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Authors: Lucas Rocha <lucasr@gnome.org>
 */

#include "config.h"

#include "matedialog.h"
#include "util.h"

static GtkWidget *scale;

static void matedialog_scale_value_changed (GtkWidget *widget, gpointer data);
static void matedialog_scale_dialog_response (GtkWidget *widget, int response, gpointer data);

void 
matedialog_scale (MateDialogData *data, MateDialogScaleData *scale_data)
{
  GtkBuilder *builder;
  GtkWidget *dialog;
  GObject *text;

  builder = matedialog_util_load_ui_file ("matedialog_scale_dialog", "adjustment1", NULL);

  if (builder == NULL) {
    data->exit_code = matedialog_util_return_exit_code (MATEDIALOG_ERROR);
    return;
  }

  dialog = GTK_WIDGET (gtk_builder_get_object (builder, "matedialog_scale_dialog"));
  scale = GTK_WIDGET (gtk_builder_get_object (builder, "matedialog_scale_hscale"));
  text = gtk_builder_get_object (builder, "matedialog_scale_text");

  g_signal_connect (G_OBJECT (dialog), "response",
                    G_CALLBACK (matedialog_scale_dialog_response), data);
	
  if (scale_data->min_value >= scale_data->max_value) {
    g_printerr (_("Maximum value must be greater than minimum value.\n")); 
    data->exit_code = matedialog_util_return_exit_code (MATEDIALOG_ERROR);
    return;
  }

  if (scale_data->value < scale_data->min_value ||
      scale_data->value > scale_data->max_value) {
    g_printerr (_("Value out of range.\n")); 
    data->exit_code = matedialog_util_return_exit_code (MATEDIALOG_ERROR);
    return;
  }

  gtk_builder_connect_signals (builder, NULL);
        
  if (data->dialog_title)
    gtk_window_set_title (GTK_WINDOW (dialog), data->dialog_title);

  matedialog_util_set_window_icon (dialog, data->window_icon, MATEDIALOG_IMAGE_FULLPATH ("matedialog-scale.png"));
  
  if (data->width > -1 || data->height > -1)
    gtk_window_set_default_size (GTK_WINDOW (dialog), data->width, data->height);
        
  if (scale_data->dialog_text) 
    gtk_label_set_markup (GTK_LABEL (text), g_strcompress (scale_data->dialog_text));

  gtk_range_set_range (GTK_RANGE (scale), scale_data->min_value, scale_data->max_value);
  gtk_range_set_value (GTK_RANGE (scale), scale_data->value);
  gtk_range_set_increments (GTK_RANGE (scale), scale_data->step, 0);

  if (scale_data->print_partial) 
    g_signal_connect (G_OBJECT (scale), "value-changed",
                      G_CALLBACK (matedialog_scale_value_changed), data);
  
  if (scale_data->hide_value)
    gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
  
  matedialog_util_show_dialog (dialog);

  if(data->timeout_delay > 0) {
    g_timeout_add_seconds (data->timeout_delay, (GSourceFunc) matedialog_util_timeout_handle, NULL);
  }

  g_object_unref (builder);

  gtk_main ();
}

static void
matedialog_scale_value_changed (GtkWidget *widget, gpointer data)
{
  g_print ("%.0f\n", gtk_range_get_value (GTK_RANGE (widget)));
}

static void
matedialog_scale_dialog_response (GtkWidget *widget, int response, gpointer data)
{
  MateDialogData *zen_data = data;

  switch (response) {
    case GTK_RESPONSE_OK:
      zen_data->exit_code = matedialog_util_return_exit_code (MATEDIALOG_OK);
      g_print ("%.0f\n", gtk_range_get_value (GTK_RANGE (scale)));
      break;

    case GTK_RESPONSE_CANCEL:
      zen_data->exit_code = matedialog_util_return_exit_code (MATEDIALOG_CANCEL);
      break;

    default:
      zen_data->exit_code = matedialog_util_return_exit_code (MATEDIALOG_ESC);
      break;
  }
  gtk_main_quit ();
}
