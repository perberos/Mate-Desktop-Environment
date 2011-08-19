/*
 * main.c
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
 * Authors: Glynn Foster <glynn.foster@sun.com>
 */

#include <config.h>

#include "matedialog.h"
#include "option.h"

#include <stdlib.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <langinfo.h>
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

gint 
main (gint argc, gchar **argv) {
  MateDialogParsingOptions *results;
  gint retval;

#ifdef HAVE_LOCALE_H
  setlocale(LC_ALL,"");
#endif

  bindtextdomain(GETTEXT_PACKAGE, MATELOCALEDIR);
  bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
  textdomain(GETTEXT_PACKAGE);

  gtk_init (&argc, &argv);

  results = matedialog_option_parse (argc, argv);

  switch (results->mode) {
    case MODE_CALENDAR:
      matedialog_calendar (results->data, results->calendar_data);
      break;
    case MODE_ENTRY:
      results->entry_data->data = (const gchar **) argv + 1;
      matedialog_entry (results->data, results->entry_data);
      break;
    case MODE_ERROR:
    case MODE_QUESTION:
    case MODE_WARNING:
    case MODE_INFO:
      matedialog_msg (results->data, results->msg_data);
      break;
    case MODE_SCALE:
      matedialog_scale (results->data, results->scale_data);
      break;
    case MODE_FILE:
      matedialog_fileselection (results->data, results->file_data);
      break;
    case MODE_LIST:
      results->tree_data->data = (const gchar **) argv + 1;
      matedialog_tree (results->data, results->tree_data);
      break;
    case MODE_NOTIFICATION:
      matedialog_notification (results->data, results->notification_data);
      break;
    case MODE_PROGRESS:
      matedialog_progress (results->data, results->progress_data);
      break;
    case MODE_TEXTINFO:
      matedialog_text (results->data, results->text_data);
      break;
    case MODE_COLOR:
      matedialog_colorselection (results->data, results->color_data);
      break;
    case MODE_PASSWORD:
      matedialog_password_dialog (results->data, results->password_data);
      break;
    case MODE_ABOUT:
      matedialog_about (results->data);
      break;
    case MODE_VERSION:
      g_print ("%s\n", VERSION); 
      break;
    case MODE_LAST:
      g_printerr (_("You must specify a dialog type. See 'matedialog --help' for details\n"));
      matedialog_option_free ();
      exit (-1);
    default:
      g_assert_not_reached ();
      matedialog_option_free ();
      exit (-1);
  }

  retval = results->data->exit_code;
  
  matedialog_option_free ();

  exit (retval);
}
