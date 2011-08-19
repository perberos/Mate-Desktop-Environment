/* gmp-util.c: utility functions */

/*
 * Copyright (C) 2003 Thomas Vander Stichele
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "gmp-util.h"

GtkBuilder *
gmp_util_load_builder_file (const char *filename,
			    GtkWindow  *error_dialog_parent,
			    GError **err)
{
  static GtkWidget *no_glade_dialog = NULL;
  gchar *path;
  GtkBuilder *builder;
  GError *error = NULL;

  path = g_strconcat ("./", filename, NULL);

  builder = gtk_builder_new ();

  /* Try current dir, for debugging */
  if (g_file_test (path, G_FILE_TEST_EXISTS) && gtk_builder_add_from_file (builder, path, &error))
    goto end;

  if (error != NULL) {
    g_warning (error->message);
    g_error_free (error);
    error = NULL;
  }

  g_free (path);
  path = g_build_filename (GMP_UIDIR, filename, NULL);
  if (g_file_test (path, G_FILE_TEST_EXISTS) && gtk_builder_add_from_file (builder, path, &error))
    goto end;

  gmp_util_show_error_dialog (error_dialog_parent, &no_glade_dialog,
			      _("The file \"%s\" is missing. This indicates that the application is installed incorrectly, so the dialog can't be displayed."), path);
  g_free (path);
  if (error != NULL) {
    g_propagate_error (err, error);
  }

  return builder;

 end:
  g_free (path);

  return builder;
}

void
gmp_util_run_error_dialog (GtkWindow *transient_parent, const char *message_format, ...)
{
  char *message;
  va_list args;
  GtkWidget *dialog;

  if (message_format)
  {
    va_start (args, message_format);
    message = g_strdup_vprintf (message_format, args);
    va_end (args);
  }
  else message = NULL;

  dialog = gtk_message_dialog_new (transient_parent,
                                 GTK_DIALOG_DESTROY_WITH_PARENT,
                                 GTK_MESSAGE_ERROR,
                                 GTK_BUTTONS_CLOSE,
                                 "%s",
                                 message);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy(GTK_WIDGET (dialog));
}

/**
 * gmp_util_show_error_dialog:
 * @transient_parent: parent of the future dialog window;
 * @weap_ptr: pointer to a #Widget pointer, to control the population.
 * @message_format: printf() style format string
 *
 * Create a #GtkMessageDialog window with the message, and present it,
 * handling its buttons.
 * If @weap_ptr is not #NULL, only create the dialog if
 * <literal>*weap_ptr</literal> is #NULL
 * (and in that case, set @weap_ptr to be a weak pointer to the new dialog),
 * otherwise just present <literal>*weak_ptr</literal>. Note that in this
 * last case, the message <emph>will</emph>  be changed.
 */

void
gmp_util_show_error_dialog (GtkWindow *transient_parent,
                            GtkWidget **weak_ptr,
                            const char *message_format, ...)
{
  char *message;
  va_list args;

  if (message_format)
  {
    va_start (args, message_format);
    message = g_strdup_vprintf (message_format, args);
    va_end (args);
  }
  else message = NULL;

  if (weak_ptr == NULL || *weak_ptr == NULL)
  {
    GtkWidget *dialog;
    dialog = gtk_message_dialog_new (transient_parent,
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_CLOSE,
                                     "%s",
                                     message);

    g_signal_connect (G_OBJECT (dialog), "response",
                      G_CALLBACK (gtk_widget_destroy), NULL);

    if (weak_ptr != NULL)
    {
      *weak_ptr = dialog;
      g_object_add_weak_pointer (G_OBJECT (dialog), (void**)weak_ptr);
    }

    gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

    gtk_widget_show_all (dialog);
  }
  else
  {
    g_return_if_fail (GTK_IS_MESSAGE_DIALOG (*weak_ptr));

    g_object_set (*weak_ptr, "text", message, NULL);

    gtk_window_present (GTK_WINDOW (*weak_ptr));
  }
}
