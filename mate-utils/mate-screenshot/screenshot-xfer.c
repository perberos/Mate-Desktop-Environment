/* screenshot-xfer.c - file transfer functions for MATE Screenshot
 *
 * Copyright (C) 2001-2006  Jonathan Blandford <jrb@alum.mit.edu>
 * Copyright (C) 2008  Cosimo Cecchi <cosimoc@mate.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 */

#include "config.h"

#include "screenshot-xfer.h"

#include <time.h>
#include <glib/gi18n.h>

typedef struct
{
  GtkWidget *dialog;
  GtkWidget *progress_bar;
  GCancellable *cancellable;
} TransferDialog;

typedef struct
{
  GFile *source;
  GFile *dest;
  GFileCopyFlags flags;
  TransferCallback callback;
  gpointer callback_data;
  GCancellable *cancellable;
  GtkWidget *parent;
  TransferDialog *dialog;
  TransferResult result;
  GIOSchedulerJob *io_job;
  char *error;
  gint dialog_timeout_id;
  goffset total_bytes;
  goffset current_bytes;
} TransferJob;

typedef struct
{
  int resp;
  GtkWidget *parent;
  char *basename;
} ErrorDialogData;

static gboolean
do_run_overwrite_confirm_dialog (gpointer _data)
{
  ErrorDialogData *data = _data;
  GtkWidget *dialog;
  gint response;

  /* we need to ask the user if they want to overwrite this file */
  dialog = gtk_message_dialog_new (GTK_WINDOW (data->parent),
				   GTK_DIALOG_DESTROY_WITH_PARENT,
				   GTK_MESSAGE_QUESTION,
				   GTK_BUTTONS_NONE,
				   _("File already exists"));

  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
					    _("The file \"%s\" already exists. "
                                              "Would you like to replace it?"), 
					    data->basename);
  gtk_dialog_add_button (GTK_DIALOG (dialog),
			 GTK_STOCK_CANCEL,
			 GTK_RESPONSE_CANCEL);
  gtk_dialog_add_button (GTK_DIALOG (dialog),
			 _("_Replace"),
			 GTK_RESPONSE_OK);

  response = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);

  data->resp = response;
  
  return FALSE;
}

static void
transfer_dialog_response_cb (GtkDialog *d,
                             gint response,
                             GCancellable *cancellable)
{
  if (response == GTK_RESPONSE_CANCEL)
    {
      if (!g_cancellable_is_cancelled (cancellable))
        {
          g_cancellable_cancel (cancellable);
        }
    }
}

static gboolean
transfer_progress_dialog_new (TransferJob *job)
{
  TransferDialog *dialog;
  GtkWidget *gdialog;
  GtkWidget *widget;
  
  dialog = g_new0 (TransferDialog, 1);
  
  gdialog = gtk_message_dialog_new (GTK_WINDOW (job->parent),
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    GTK_MESSAGE_OTHER,
                                    GTK_BUTTONS_CANCEL,
                                    _("Saving file..."));
  widget = gtk_progress_bar_new ();
  gtk_box_pack_start (GTK_BOX (GTK_MESSAGE_DIALOG (gdialog)->label->parent),
                      widget, FALSE, 0, 0);
  gtk_widget_show (widget);
  dialog->progress_bar = widget;
  dialog->dialog = gdialog;
  
  g_signal_connect (gdialog,
                    "response",
                    G_CALLBACK (transfer_dialog_response_cb),
                    job->cancellable);

  job->dialog = dialog;
  gtk_widget_show (gdialog);
  
  return FALSE;
}

static void
transfer_progress_dialog_start (TransferJob *job)
{
  /* sends to the mainloop and schedules show */
  if (job->dialog_timeout_id == 0)
    job->dialog_timeout_id = g_timeout_add_seconds (1,
                                                    (GSourceFunc) transfer_progress_dialog_new,
                                                    job);
}

static int
run_overwrite_confirm_dialog (TransferJob *job)
{
  ErrorDialogData *data;
  gboolean need_timeout;
  int response;
  char *basename;

  basename = g_file_get_basename (job->dest);

  data = g_slice_new0 (ErrorDialogData);
  data->parent = job->parent;
  data->basename = basename;

  need_timeout = (job->dialog_timeout_id > 0);
  
  if (need_timeout)
    {
      g_source_remove (job->dialog_timeout_id);
      job->dialog_timeout_id = 0;
    } 

  g_io_scheduler_job_send_to_mainloop (job->io_job,
                                       do_run_overwrite_confirm_dialog,
                                       data,
                                       NULL);
  response = data->resp;

  if (need_timeout)
    transfer_progress_dialog_start (job);
  
  g_free (basename);
  g_slice_free (ErrorDialogData, data);

  return response;                                       
}

static gboolean
transfer_progress_dialog_update (TransferJob *job)
{
  TransferDialog *dialog = job->dialog;
  double fraction;
  
  fraction = ((double) job->current_bytes) / ((double) job->total_bytes);

  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (dialog->progress_bar),
                                 fraction);
  
  return FALSE;
}

static gboolean
transfer_job_done (gpointer user_data)
{
  TransferJob *job = user_data;
  TransferDialog *dialog;
  
  dialog = job->dialog;
  
  if (job->dialog_timeout_id)
    {
      g_source_remove (job->dialog_timeout_id);
      job->dialog_timeout_id = 0;
    }
  if (dialog)
      gtk_widget_destroy (dialog->dialog);
  
  if (job->callback)
    {
      (job->callback) (job->result,
                       job->error,
                       job->callback_data);
    }
  
  g_object_unref (job->source);
  g_object_unref (job->dest);
  g_object_unref (job->cancellable);

  g_free (dialog);
  g_free (job->error);
  g_slice_free (TransferJob, job);
  
  return FALSE;
}

static void
transfer_progress_cb (goffset current_num_bytes,
                      goffset total_num_bytes,
                      TransferJob *job)
{  
  job->current_bytes = current_num_bytes;

  if (!job->dialog)
    return;
   
  g_io_scheduler_job_send_to_mainloop_async (job->io_job,
                                             (GSourceFunc) transfer_progress_dialog_update,
                                             job,
                                             NULL);
}

static goffset
get_file_size (GFile *file)
{
  GFileInfo *file_info;
  goffset size;
  
  file_info = g_file_query_info (file,
                                 G_FILE_ATTRIBUTE_STANDARD_SIZE,
                                 0, NULL, NULL);
  if (file_info != NULL)
    {
      size = g_file_info_get_size (file_info);
      g_object_unref (file_info);
    }
  else
    {
      /* this should never fail as the source file is always local and in /tmp,
       * but you never know.
       */
      size = -1;
    }
  
  return size;
}

static gboolean
transfer_file (GIOSchedulerJob *io_job,
               GCancellable *cancellable,
               gpointer user_data)
{
  TransferJob *job = user_data;
  GError *error;
  
  job->io_job = io_job;
  job->total_bytes = get_file_size (job->source);
  if (job->total_bytes == -1)
    {
      /* we can't access the source file, abort early */
      error = NULL;
      job->result = TRANSFER_ERROR;
      job->error = g_strdup (_("Can't access source file"));
      
      goto out;
    }

  transfer_progress_dialog_start (job);

retry:
  error = NULL;
  if (!g_file_copy (job->source,
                    job->dest,
                    job->flags,
                    job->cancellable,
                    (GFileProgressCallback) transfer_progress_cb,
                    job,
                    &error))
    {
      if (error->code == G_IO_ERROR_EXISTS)
        {
          int response;
          /* ask the user if he wants to overwrite, otherwise
           * return and report what happened.
           */
          response = run_overwrite_confirm_dialog (job);
          if (response == GTK_RESPONSE_OK)
            {
              job->flags |= G_FILE_COPY_OVERWRITE;
              g_error_free (error);

              goto retry;
            }
          else
            {
              g_cancellable_cancel (job->cancellable);
              job->result = TRANSFER_OVERWRITE;
              goto out;
            }
        }
      else if (error->code == G_IO_ERROR_CANCELLED)
        {
          job->result = TRANSFER_CANCELLED;

          goto out;
        }
      else
        {
          /* other vfs error, abort the transfer and report
           * the error.
           */
          g_cancellable_cancel (job->cancellable);
          job->result = TRANSFER_ERROR;
          job->error = g_strdup (error->message);
          
          goto out;
        }
    }
  else
    {
      /* success */
      job->result = TRANSFER_OK;
      
      goto out;
    }

out:
  if (error)
      g_error_free (error);
    
  g_io_scheduler_job_send_to_mainloop_async (io_job,
                                             transfer_job_done,
                                             job,
                                             NULL);  
  return FALSE;
}

void
screenshot_xfer_uri (GFile *source_file,
                     GFile *target_file,
                     GtkWidget *parent,
                     TransferCallback done_callback,
                     gpointer done_callback_data)
{
  TransferJob *job;
  GCancellable *cancellable;

  cancellable = g_cancellable_new ();

  job = g_slice_new0 (TransferJob);
  job->parent = parent;
  job->source = g_object_ref (source_file);
  job->dest = g_object_ref (target_file);
  job->flags = 0;
  job->error = NULL;
  job->dialog = NULL;
  job->callback = done_callback;
  job->callback_data = done_callback_data;
  job->cancellable = cancellable;

  g_io_scheduler_push_job (transfer_file,
                           job,
                           NULL, 0,
                           job->cancellable);
}

