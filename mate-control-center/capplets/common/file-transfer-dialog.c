/* file-transfer-dialog.c
 * Copyright (C) 2002 Ximian, Inc.
 *
 * Written by Rachel Hestilow <hestilow@ximian.com>
 *            Jens Granseuer <jensgr@gmx.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gio/gio.h>
#include <limits.h>

#include "file-transfer-dialog.h"

enum
{
	PROP_0,
	PROP_FROM_URI,
	PROP_TO_URI,
	PROP_FRACTION_COMPLETE,
	PROP_NTH_URI,
	PROP_TOTAL_URIS,
	PROP_PARENT
};

enum
{
	CANCEL,
	DONE,
	LAST_SIGNAL
};

guint file_transfer_dialog_signals[LAST_SIGNAL] = {0, };

struct _FileTransferDialogPrivate
{
	GtkWidget *progress;
	GtkWidget *status;
	guint nth;
	guint total;
	GCancellable *cancellable;
};

#define FILE_TRANSFER_DIALOG_GET_PRIVATE(object) (G_TYPE_INSTANCE_GET_PRIVATE ((object), file_transfer_dialog_get_type (), FileTransferDialogPrivate))

typedef struct _FileTransferJob
{
	FileTransferDialog *dialog;
	GtkDialog *overwrite_dialog;
	GSList *source_files;
	GSList *target_files;
	FileTransferDialogOptions options;
} FileTransferJob;

/* structure passed to the various callbacks */
typedef struct {
	FileTransferDialog *dialog;
	gchar *source;
	gchar *target;
	guint current_file;
	guint total_files;
	goffset current_bytes;
	goffset total_bytes;
	gint response;
	GtkDialog *overwrite_dialog;
} FileTransferData;

G_DEFINE_TYPE (FileTransferDialog, file_transfer_dialog, GTK_TYPE_DIALOG)

static void
file_transfer_dialog_update_num_files (FileTransferDialog *dlg)
{
	gchar *str;

	if (dlg->priv->total <= 1)
		return;

	str = g_strdup_printf (_("Copying file: %u of %u"),
			      dlg->priv->nth, dlg->priv->total);
	gtk_progress_bar_set_text (GTK_PROGRESS_BAR (dlg->priv->progress), str);
	g_free (str);
}

static void
file_transfer_dialog_response (GtkDialog *dlg, gint response_id)
{
	FileTransferDialog *dialog = FILE_TRANSFER_DIALOG (dlg);

	g_cancellable_cancel (dialog->priv->cancellable);
}

static void
file_transfer_dialog_finalize (GObject *object)
{
	FileTransferDialog *dlg = FILE_TRANSFER_DIALOG (object);

	if (dlg->priv->cancellable)
	{
		g_object_unref (dlg->priv->cancellable);
		dlg->priv->cancellable = NULL;
	}

	G_OBJECT_CLASS (file_transfer_dialog_parent_class)->finalize (object);
}

static void
file_transfer_dialog_set_prop (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	FileTransferDialog *dlg = FILE_TRANSFER_DIALOG (object);
	GFile *file;
	gchar *str;
	gchar *str2;
	gchar *base;
	gchar *escaped;
	GtkWindow *parent;
	guint n;

	switch (prop_id)
	{
	case PROP_FROM_URI:
		file = g_file_new_for_uri (g_value_get_string (value));
		base = g_file_get_basename (file);
		escaped = g_uri_escape_string (base, G_URI_RESERVED_CHARS_ALLOWED_IN_PATH, TRUE);

		str = g_strdup_printf (_("Copying '%s'"), escaped);
		str2 = g_strdup_printf ("<big><b>%s</b></big>", str);
		gtk_label_set_markup (GTK_LABEL (dlg->priv->status), str2);

		g_free (base);
		g_free (escaped);
		g_free (str);
		g_free (str2);
		g_object_unref (file);
		break;
	case PROP_TO_URI:
		break;
	case PROP_FRACTION_COMPLETE:
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (dlg->priv->progress), g_value_get_double (value));
		break;
	case PROP_NTH_URI:
		n = g_value_get_uint (value);
		if (n != dlg->priv->nth)
		{
			dlg->priv->nth = g_value_get_uint (value);
			file_transfer_dialog_update_num_files (dlg);
		}
		break;
	case PROP_TOTAL_URIS:
		n = g_value_get_uint (value);
		if (n != dlg->priv->nth)
		{
			dlg->priv->total = g_value_get_uint (value);
			file_transfer_dialog_update_num_files (dlg);
		}
		break;
	case PROP_PARENT:
		parent = g_value_get_pointer (value);
		if (parent)
		{
			gtk_window_set_title (GTK_WINDOW (dlg), gtk_window_get_title (parent));
			gtk_window_set_transient_for (GTK_WINDOW (dlg), parent);
		}
		else
			gtk_window_set_title (GTK_WINDOW (dlg),
					      _("Copying files"));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
file_transfer_dialog_get_prop (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	FileTransferDialog *dlg = FILE_TRANSFER_DIALOG (object);

	switch (prop_id)
	{
	case PROP_NTH_URI:
		g_value_set_uint (value, dlg->priv->nth);
		break;
	case PROP_TOTAL_URIS:
		g_value_set_uint (value, dlg->priv->total);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
file_transfer_dialog_class_init (FileTransferDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = file_transfer_dialog_finalize;
	object_class->get_property = file_transfer_dialog_get_prop;
	object_class->set_property = file_transfer_dialog_set_prop;

	GTK_DIALOG_CLASS (klass)->response = file_transfer_dialog_response;

	g_object_class_install_property
		(object_class, PROP_PARENT,
		 g_param_spec_pointer ("parent",
				      _("Parent Window"),
				      _("Parent window of the dialog"),
				      G_PARAM_READWRITE));

	g_object_class_install_property
		(object_class, PROP_FROM_URI,
		 g_param_spec_string ("from_uri",
				      _("From URI"),
				      _("URI currently transferring from"),
				      NULL,
				      G_PARAM_READWRITE));

	g_object_class_install_property
		(object_class, PROP_TO_URI,
		 g_param_spec_string ("to_uri",
				      _("To URI"),
				      _("URI currently transferring to"),
				      NULL,
				      G_PARAM_WRITABLE));

	g_object_class_install_property
		(object_class, PROP_FRACTION_COMPLETE,
		 g_param_spec_double ("fraction_complete",
				      _("Fraction completed"),
				      _("Fraction of transfer currently completed"),
				      0, 1, 0,
				      G_PARAM_WRITABLE));

	g_object_class_install_property
		(object_class, PROP_NTH_URI,
		 g_param_spec_uint ("nth_uri",
				    _("Current URI index"),
				    _("Current URI index - starts from 1"),
				    1, INT_MAX, 1,
				    G_PARAM_READWRITE));

	g_object_class_install_property
		(object_class, PROP_TOTAL_URIS,
		 g_param_spec_uint ("total_uris",
				    _("Total URIs"),
				    _("Total number of URIs"),
				    1, INT_MAX, 1,
				    G_PARAM_READWRITE));

	file_transfer_dialog_signals[CANCEL] =
		g_signal_new ("cancel",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_FIRST,
			      0,
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	file_transfer_dialog_signals[DONE] =
		g_signal_new ("done",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      0,
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

        g_type_class_add_private (klass, sizeof (FileTransferDialogPrivate));
}

static void
file_transfer_dialog_init (FileTransferDialog *dlg)
{
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *progress_vbox;
	GtkWidget *table;
	char      *markup;
	GtkWidget *content_area;

	content_area = gtk_dialog_get_content_area (GTK_DIALOG (dlg));
	dlg->priv = FILE_TRANSFER_DIALOG_GET_PRIVATE (dlg);
	dlg->priv->cancellable = g_cancellable_new ();

	gtk_container_set_border_width (GTK_CONTAINER (content_area), 4);
	gtk_box_set_spacing (GTK_BOX (content_area), 4);

	gtk_widget_set_size_request (GTK_WIDGET (dlg), 350, -1);

	vbox = gtk_vbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
	gtk_box_pack_start (GTK_BOX (content_area), vbox, TRUE, TRUE, 0);

	dlg->priv->status = gtk_label_new (NULL);
	markup = g_strconcat ("<big><b>", _("Copying files"), "</b></big>", NULL);
	gtk_label_set_markup (GTK_LABEL (dlg->priv->status), markup);
	g_free (markup);

	gtk_misc_set_alignment (GTK_MISC (dlg->priv->status), 0.0, 0.0);

	gtk_box_pack_start (GTK_BOX (vbox), dlg->priv->status, FALSE, FALSE, 0);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

	table = gtk_table_new (2, 2, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (table), 4);
	gtk_table_set_col_spacings (GTK_TABLE (table), 4);

	gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (table), FALSE, FALSE, 0);

	progress_vbox = gtk_vbox_new (TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), progress_vbox, FALSE, FALSE, 0);

	dlg->priv->progress = gtk_progress_bar_new ();
	gtk_box_pack_start (GTK_BOX (progress_vbox),
			    dlg->priv->progress, FALSE, FALSE, 0);

	gtk_dialog_add_button (GTK_DIALOG (dlg),
			       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);

	gtk_dialog_set_has_separator (GTK_DIALOG (dlg), FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (dlg), 6);

	gtk_widget_show_all (content_area);
}

GtkWidget*
file_transfer_dialog_new (void)
{
	return GTK_WIDGET (g_object_new (file_transfer_dialog_get_type (),
					 NULL));
}

GtkWidget*
file_transfer_dialog_new_with_parent (GtkWindow *parent)
{
	return GTK_WIDGET (g_object_new (file_transfer_dialog_get_type (),
					 "parent", parent, NULL));
}

static gboolean
file_transfer_job_update (gpointer user_data)
{
	FileTransferData *data = user_data;
	gdouble fraction;
	gdouble current_fraction;

	if (data->total_bytes == 0)
		current_fraction = 0.0;
	else
		current_fraction = ((gdouble) data->current_bytes) / data->total_bytes;

	fraction = ((gdouble) data->current_file - 1) / data->total_files +
		   (1.0 / data->total_files) * current_fraction;

	g_object_set (data->dialog,
		      "from_uri", data->source,
		      "to_uri", data->target,
		      "nth_uri", data->current_file,
		      "fraction_complete", fraction,
		      NULL);
	return FALSE;
}

static void
file_transfer_job_progress (goffset current_bytes,
			    goffset total_bytes,
			    gpointer user_data)
{
	FileTransferData *data = user_data;

	data->current_bytes = current_bytes;
	data->total_bytes = total_bytes;

	gdk_threads_enter ();
	file_transfer_job_update (data);
	gdk_threads_leave ();
}

static void
file_transfer_job_destroy (FileTransferJob *job)
{
	g_object_unref (job->dialog);
	g_slist_foreach (job->source_files, (GFunc) g_object_unref, NULL);
	g_slist_foreach (job->target_files, (GFunc) g_object_unref, NULL);
	g_slist_free (job->source_files);
	g_slist_free (job->target_files);
	if (job->overwrite_dialog != NULL)
		gtk_widget_destroy (GTK_WIDGET (job->overwrite_dialog));
	g_free (job);
}

static gboolean
file_transfer_dialog_done (FileTransferDialog *dialog)
{
	g_signal_emit (dialog,
		       file_transfer_dialog_signals[DONE],
		       0, NULL);
	return FALSE;
}

static gboolean
file_transfer_dialog_cancel (FileTransferDialog *dialog)
{
	g_signal_emit (dialog,
		       file_transfer_dialog_signals[CANCEL],
		       0, NULL);
	return FALSE;
}

static gboolean
file_transfer_dialog_overwrite (gpointer user_data)
{
	FileTransferData *data = user_data;
	GtkDialog *dialog;

	dialog = data->overwrite_dialog;

	if (dialog != NULL) {
	} else {
		GtkWidget *button;

		dialog = GTK_DIALOG (gtk_message_dialog_new (GTK_WINDOW (data->dialog),
							     GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION,
							     GTK_BUTTONS_NONE,
							     _("File '%s' already exists. Do you want to overwrite it?"),
							     data->target));

		gtk_dialog_add_button (dialog, _("_Skip"), GTK_RESPONSE_NO);
		gtk_dialog_add_button (dialog, _("Overwrite _All"), GTK_RESPONSE_APPLY);

		button = gtk_button_new_with_label (_("_Overwrite"));
		gtk_button_set_image (GTK_BUTTON (button),
				      gtk_image_new_from_stock (GTK_STOCK_APPLY,
								GTK_ICON_SIZE_BUTTON));
		gtk_dialog_add_action_widget (dialog, button, GTK_RESPONSE_YES);
		gtk_widget_show (button);

		data->overwrite_dialog = dialog;
	}

	data->response = gtk_dialog_run (dialog);

	gtk_widget_hide (GTK_WIDGET (dialog));
	return FALSE;
}

/* TODO: support transferring directories recursively? */
static gboolean
file_transfer_job_schedule (GIOSchedulerJob *io_job,
			    GCancellable *cancellable,
			    FileTransferJob *job)
{
	GFile *source, *target;
	gboolean success;
	GFileCopyFlags copy_flags = G_FILE_COPY_NONE;
	FileTransferData data;
	GError *error;
	gboolean retry;

	/* take the first file from the list and copy it */
	source = job->source_files->data;
	job->source_files = g_slist_delete_link (job->source_files, job->source_files);

	target = job->target_files->data;
	job->target_files = g_slist_delete_link (job->target_files, job->target_files);

	data.dialog = job->dialog;
	data.overwrite_dialog = job->overwrite_dialog;
	data.current_file = job->dialog->priv->nth + 1;
	data.total_files = job->dialog->priv->total;
	data.current_bytes = data.total_bytes = 0;
	data.source = g_file_get_basename (source);
	data.target = g_file_get_basename (target);

	g_io_scheduler_job_send_to_mainloop (io_job,
					     file_transfer_job_update,
					     &data,
					     NULL);

	if (job->options & FILE_TRANSFER_DIALOG_OVERWRITE)
		copy_flags |= G_FILE_COPY_OVERWRITE;

	do {
		retry = FALSE;
		error = NULL;
		success = g_file_copy (source, target,
				       copy_flags,
				       job->dialog->priv->cancellable,
				       file_transfer_job_progress,
				       &data,
				       &error);

		if (error != NULL)
		{
			if (error->domain == G_IO_ERROR &&
			    error->code == G_IO_ERROR_EXISTS)
			{
				/* since the job is run in a thread, we cannot simply run
				 * a dialog here and need to defer it to the mainloop */
				data.response = GTK_RESPONSE_NONE;
				g_io_scheduler_job_send_to_mainloop (io_job,
								     file_transfer_dialog_overwrite,
								     &data,
								     NULL);

				if (data.response == GTK_RESPONSE_YES) {
					retry = TRUE;
					copy_flags |= G_FILE_COPY_OVERWRITE;
				} else if (data.response == GTK_RESPONSE_APPLY) {
					retry = TRUE;
					job->options |= FILE_TRANSFER_DIALOG_OVERWRITE;
					copy_flags |= G_FILE_COPY_OVERWRITE;
				} else {
					success = TRUE;
				}

				job->overwrite_dialog = data.overwrite_dialog;
			}
			g_error_free (error);
		}
	} while (retry);

	g_object_unref (source);
	g_object_unref (target);

	g_free (data.source);
	g_free (data.target);

	if (success)
	{
		if (job->source_files == NULL)
		{
			g_io_scheduler_job_send_to_mainloop_async (io_job,
								   (GSourceFunc) file_transfer_dialog_done,
								   g_object_ref (job->dialog),
								   g_object_unref);
			return FALSE;
		}
	}
	else /* error on copy or cancelled */
	{
		g_io_scheduler_job_send_to_mainloop_async (io_job,
							   (GSourceFunc) file_transfer_dialog_cancel,
							   g_object_ref (job->dialog),
							   g_object_unref);
		return FALSE;
	}

	/* more work to do... */
	return TRUE;
}

void
file_transfer_dialog_copy_async (FileTransferDialog *dlg,
				 GList *source_files,
				 GList *target_files,
				 FileTransferDialogOptions options,
				 int priority)
{
	FileTransferJob *job;
	GList *l;
	guint n;

	job = g_new0 (FileTransferJob, 1);
	job->dialog = g_object_ref (dlg);
	job->options = options;

	/* we need to copy the list contents for private use */
	n = 0;
	for (l = g_list_last (source_files); l; l = l->prev, ++n)
	{
		job->source_files = g_slist_prepend (job->source_files,
						     g_object_ref (l->data));
	}
	for (l = g_list_last (target_files); l; l = l->prev)
	{
		job->target_files = g_slist_prepend (job->target_files,
						     g_object_ref (l->data));
	}

	g_object_set (dlg, "total_uris", n, NULL);

	g_io_scheduler_push_job ((GIOSchedulerJobFunc) file_transfer_job_schedule,
				 job,
				 (GDestroyNotify) file_transfer_job_destroy,
				 priority,
				 dlg->priv->cancellable);
}
