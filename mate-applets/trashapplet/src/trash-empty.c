/*
 * trash-empty.c: a routine to empty the trash
 *
 * Copyright © 2008 Ryan Lortie
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <mateconf/mateconf-client.h>
#include <gio/gio.h>
#include <glib/gi18n.h>

#include "trash-empty.h"
#include "config.h"

/* only one concurrent trash empty operation can occur */
static GtkDialog          *trash_empty_confirm_dialog;
static GtkDialog          *trash_empty_dialog;
static GtkProgressBar     *trash_empty_progress_bar;
static GtkLabel           *trash_empty_location;
static GtkLabel           *trash_empty_file;

/* the rules:
 * 1) nothing here may be modified while trash_empty_update_pending.
 * 2) an idle may only be scheduled if trash_empty_update_pending.
 * 3) only the worker may set trash_empty_update_pending = TRUE.
 * 4) only the UI updater may set trash_empty_update_pending = FALSE.
 *
 * i -think- this is threadsafe...  ((famous last words...))
 */
static GFile *     volatile trash_empty_current_file;
static gsize       volatile trash_empty_deleted_files;
static gsize       volatile trash_empty_total_files;
static gboolean    volatile trash_empty_update_pending;

static gboolean
trash_empty_clear_pending (gpointer user_data)
{
  trash_empty_update_pending = FALSE;

  return FALSE;
}

static gboolean
trash_empty_update_dialog (gpointer user_data)
{
  gsize deleted, total;
  GFile *file;

  g_assert (trash_empty_update_pending);

  deleted = trash_empty_deleted_files;
  total = trash_empty_total_files;
  file = trash_empty_current_file;

  /* maybe the done() got processed first. */
  if (trash_empty_dialog)
    {
      char *index_str, *total_str;
      char *text_tmp, *text;
      char *tmp;

      /* The i18n tools can't handle a direct embedding of the
       * size format using a macro. This is a work-around. */
      index_str = g_strdup_printf ("%"G_GSIZE_FORMAT, deleted + 1);
      total_str = g_strdup_printf ("%"G_GSIZE_FORMAT, total);
      /* Translators: the %s in this string should be read as %d. */
      text = g_strdup_printf (_("Removing item %s of %s"),
                              index_str, total_str);
      gtk_progress_bar_set_text (trash_empty_progress_bar, text);
      g_free (total_str);
      g_free (index_str);
      g_free (text);

      if (deleted > total)
        gtk_progress_bar_set_fraction (trash_empty_progress_bar, 1.0);
      else
        gtk_progress_bar_set_fraction (trash_empty_progress_bar,
                                       (gdouble) deleted / (gdouble) total);

      /* no g_file_get_basename? */
      {
        GFile *parent;

        parent = g_file_get_parent (file);
        text = g_file_get_uri (parent);
        g_object_unref (parent);
      }
      gtk_label_set_text (trash_empty_location, text);
      g_free (text);

      tmp = g_file_get_basename (file);
      /* Translators: %s is a file name */
      text_tmp = g_strdup_printf (_("Removing: %s"), tmp);
      text = g_markup_printf_escaped ("<i>%s</i>", text_tmp);
      gtk_label_set_markup (trash_empty_file, text);
      g_free (text);
      g_free (text_tmp);
      g_free (tmp);

      /* unhide the labels */
      gtk_widget_show_all (GTK_WIDGET (trash_empty_dialog));
    }

  trash_empty_current_file = NULL;
  g_object_unref (file);

  trash_empty_update_pending = FALSE;

  return FALSE;
}

static gboolean
trash_empty_done (gpointer user_data)
{
  gtk_object_destroy (GTK_OBJECT (trash_empty_dialog));

  g_assert (trash_empty_dialog == NULL);

  return FALSE;
}

/* =============== worker thread code begins here =============== */
static void
trash_empty_maybe_schedule_update (GIOSchedulerJob *job,
                                   GFile           *file,
                                   gsize            deleted)
{
  if (!trash_empty_update_pending)
    {
      g_assert (trash_empty_current_file == NULL);

      trash_empty_current_file = g_object_ref (file);
      trash_empty_deleted_files = deleted;

      trash_empty_update_pending = TRUE;
      g_io_scheduler_job_send_to_mainloop_async (job,
                                                 trash_empty_update_dialog,
                                                 NULL, NULL);
    }
}

static void
trash_empty_delete_contents (GIOSchedulerJob *job,
                             GCancellable    *cancellable,
                             GFile           *file,
                             gboolean         actually_delete,
                             gsize           *deleted)
{
  GFileEnumerator *enumerator;
  GFileInfo *info;
  GFile *child;

  if (g_cancellable_is_cancelled (cancellable))
    return;

  enumerator = g_file_enumerate_children (file,
                                          G_FILE_ATTRIBUTE_STANDARD_NAME ","
                                          G_FILE_ATTRIBUTE_STANDARD_TYPE,
                                          G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS,
                                          cancellable, NULL);
  if (enumerator)
    {
      while ((info = g_file_enumerator_next_file (enumerator,
                                                  cancellable, NULL)) != NULL)
        {
          child = g_file_get_child (file, g_file_info_get_name (info));

          if (g_file_info_get_file_type (info) == G_FILE_TYPE_DIRECTORY)
            trash_empty_delete_contents (job, cancellable, child,
                                         actually_delete, deleted);

          if (actually_delete)
            {
              trash_empty_maybe_schedule_update (job, child, *deleted);
              g_file_delete (child, cancellable, NULL);
            }

          (*deleted)++;

          g_object_unref (child);
          g_object_unref (info);

          if (g_cancellable_is_cancelled (cancellable))
            break;
        }

      g_object_unref (enumerator);
    }
}

static gboolean
trash_empty_job (GIOSchedulerJob *job,
                 GCancellable    *cancellable,
                 gpointer         user_data)
{
  gsize deleted;
  GFile *trash;

  trash = g_file_new_for_uri ("trash:///");

  /* first do a dry run to count the number of files */
  deleted = 0;
  trash_empty_delete_contents (job, cancellable, trash, FALSE, &deleted);
  trash_empty_total_files = deleted;

  /* now do the real thing */
  deleted = 0;
  trash_empty_delete_contents (job, cancellable, trash, TRUE, &deleted);

  /* done */
  g_object_unref (trash);
  g_io_scheduler_job_send_to_mainloop_async (job,
                                             trash_empty_done,
                                             NULL, NULL);

  return FALSE;
}
/* ================ worker thread code ends here ================ */

static void
trash_empty_start (GtkWidget *parent)
{
  struct { const char *name; gpointer *pointer; } widgets[] =
    {
      { "empty_trash",       (gpointer *) &trash_empty_dialog        },
      { "progressbar",       (gpointer *) &trash_empty_progress_bar  },
      { "location_label",    (gpointer *) &trash_empty_location      },
      { "file_label",        (gpointer *) &trash_empty_file          }
    };
  GCancellable *cancellable;
  GtkBuilder *builder;
  gint i;

  builder = gtk_builder_new ();
  gtk_builder_add_from_file (builder,
                             GTK_BUILDERDIR "/trashapplet-empty-progress.ui",
                             NULL);

  for (i = 0; i < G_N_ELEMENTS (widgets); i++)
    {
      GObject *object;

      object = gtk_builder_get_object (builder, widgets[i].name);

      if (object == NULL)
        {
          g_critical ("failed to parse trash-empty dialog markup");

          if (trash_empty_dialog)
            gtk_object_destroy (GTK_OBJECT (trash_empty_dialog));

          g_object_unref (builder);
          return;
        }

      *widgets[i].pointer = object;
      g_object_add_weak_pointer (object, widgets[i].pointer);
    }
  g_object_unref (builder);

  cancellable = g_cancellable_new ();
  g_signal_connect_object (trash_empty_dialog, "response",
                           G_CALLBACK (g_cancellable_cancel),
                           cancellable, G_CONNECT_SWAPPED);
  g_io_scheduler_push_job (trash_empty_job, NULL, NULL, 0, cancellable);
  g_object_unref (cancellable);

  gtk_window_set_screen (GTK_WINDOW (trash_empty_dialog),
                         gtk_widget_get_screen (parent));
  gtk_widget_show (GTK_WIDGET (trash_empty_dialog));
}

static gboolean
trash_empty_require_confirmation (void)
{
  return mateconf_client_get_bool (mateconf_client_get_default (),
                                "/apps/caja/preferences/confirm_trash",
                                NULL);
}

static void
trash_empty_confirmation_response (GtkDialog *dialog,
                                   gint       response_id,
                                   gpointer   user_data)
{
  if (response_id == GTK_RESPONSE_YES)
    trash_empty_start (GTK_WIDGET (dialog));

  gtk_object_destroy (GTK_OBJECT (dialog));
  g_assert (trash_empty_confirm_dialog == NULL);
}

/*
 * The code in trash_empty_show_confirmation_dialog() was taken from
 * libcaja-private/caja-file-operations.c (confirm_empty_trash)
 * by Michiel Sikkes <michiel@eyesopened.nl> and adapted for the applet.
 */
static void
trash_empty_show_confirmation_dialog (GtkWidget *parent)
{
  GtkWidget *dialog;
  GtkWidget *button;
  GdkScreen *screen;

  if (!trash_empty_require_confirmation ())
    {
      trash_empty_start (parent);
      return;
    }

  screen = gtk_widget_get_screen (parent);

  dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL,
                                   GTK_MESSAGE_WARNING,
                                   GTK_BUTTONS_NONE,
                                   _("Empty all of the items from "
                                     "the trash?"));
  trash_empty_confirm_dialog = GTK_DIALOG (dialog);
  g_object_add_weak_pointer (G_OBJECT (dialog),
                             (gpointer *) &trash_empty_confirm_dialog);

  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                            _("If you choose to empty "
                                              "the trash, all items in "
                                              "it will be permanently "
                                              "lost. Please note that "
                                              "you can also delete them "
                                              "separately."));

  gtk_window_set_screen (GTK_WINDOW (dialog), screen);
  atk_object_set_role (gtk_widget_get_accessible (dialog), ATK_ROLE_ALERT);
  gtk_window_set_wmclass (GTK_WINDOW (dialog), "empty_trash", "Caja");

  gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CANCEL,
                         GTK_RESPONSE_CANCEL);

  button = gtk_button_new_with_mnemonic (_("_Empty Trash"));
  gtk_widget_show (button);
  gtk_widget_set_can_default (button, TRUE);

  gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button,
                                GTK_RESPONSE_YES);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog),
                                   GTK_RESPONSE_YES);

  gtk_widget_show (dialog);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (trash_empty_confirmation_response), NULL);
}

void
trash_empty (GtkWidget *parent)
{
  if (trash_empty_confirm_dialog)
    gtk_window_present (GTK_WINDOW (trash_empty_confirm_dialog));
  else if (trash_empty_dialog)
    gtk_window_present (GTK_WINDOW (trash_empty_dialog));

  /* theoretically possible that an update is pending, but very unlikely. */
  else if (!trash_empty_update_pending)
    trash_empty_show_confirmation_dialog (parent);
}
