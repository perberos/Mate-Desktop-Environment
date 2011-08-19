/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2008 Philip Withnall <philip@tecnocode.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
 *
 * The Totem project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and Totem. This
 * permission are above and beyond the permissions granted by the GPL license
 * Totem is covered by.
 *
 * Monday 7th February 2005: Christian Schaller: Add exception clause.
 * See license_change file for details.
 */

#include "config.h"

#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>

#include "totem-gallery-progress.h"

static void totem_gallery_progress_finalize (GObject *object);
static void dialog_response_callback (GtkDialog *dialog, gint response_id, TotemGalleryProgress *self);

struct _TotemGalleryProgressPrivate {
	GPid child_pid;
	GString *line;
	gchar *output_filename;

	GtkProgressBar *progress_bar;
};

G_DEFINE_TYPE (TotemGalleryProgress, totem_gallery_progress, GTK_TYPE_DIALOG)
#define TOTEM_GALLERY_PROGRESS_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TOTEM_TYPE_GALLERY_PROGRESS, TotemGalleryProgressPrivate))

static void
totem_gallery_progress_class_init (TotemGalleryProgressClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (TotemGalleryProgressPrivate));

	gobject_class->finalize = totem_gallery_progress_finalize;
}

static void
totem_gallery_progress_init (TotemGalleryProgress *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, TOTEM_TYPE_GALLERY_PROGRESS, TotemGalleryProgressPrivate);
}

static void
totem_gallery_progress_finalize (GObject *object)
{
	TotemGalleryProgressPrivate *priv = TOTEM_GALLERY_PROGRESS_GET_PRIVATE (object);

	g_spawn_close_pid (priv->child_pid);
	g_free (priv->output_filename);

	if (priv->line != NULL)
		g_string_free (priv->line, TRUE);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (totem_gallery_progress_parent_class)->finalize (object);
}

TotemGalleryProgress *
totem_gallery_progress_new (GPid child_pid, const gchar *output_filename)
{
	TotemGalleryProgress *self;
	GtkWidget *container;
	gchar *label_text;

	/* Create the gallery */
	self = g_object_new (TOTEM_TYPE_GALLERY_PROGRESS, NULL);

	/* Create the widget and initialise class variables */
	self->priv->progress_bar = GTK_PROGRESS_BAR (gtk_progress_bar_new ());
	self->priv->child_pid = child_pid;
	self->priv->output_filename = g_strdup (output_filename);

	/* Set up the window */
	gtk_window_set_title (GTK_WINDOW (self), _("Creating Gallery..."));
	gtk_window_set_resizable (GTK_WINDOW (self), FALSE);
	gtk_dialog_add_button (GTK_DIALOG (self), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
	gtk_dialog_set_default_response (GTK_DIALOG (self), GTK_RESPONSE_CANCEL);

	/* Set the progress label */
	label_text = g_strdup_printf (_("Saving gallery as \"%s\""), output_filename);
	gtk_progress_bar_set_text (self->priv->progress_bar, label_text);
	g_free (label_text);

	g_signal_connect (G_OBJECT (self), "response",
			  G_CALLBACK (dialog_response_callback), self);

	/* Assemble the window */
	container = gtk_dialog_get_content_area (GTK_DIALOG (self));
	gtk_box_pack_start (GTK_BOX (container), GTK_WIDGET (self->priv->progress_bar), TRUE, TRUE, 5);

	gtk_widget_show_all (container);

	return self;
}

static void
dialog_response_callback (GtkDialog *dialog, gint response_id, TotemGalleryProgress *self)
{
	if (response_id != GTK_RESPONSE_OK) {
		/* Cancel the operation by killing the process */
		kill (self->priv->child_pid, SIGINT);

		/* Unlink the output file, just in case (race condition) it's already been created */
		g_unlink (self->priv->output_filename);
	}
}

static gboolean
process_line (TotemGalleryProgress *self, const gchar *line)
{
	gfloat percent_complete;

	if (sscanf (line, "%f%% complete", &percent_complete) == 1) {
		gtk_progress_bar_set_fraction (self->priv->progress_bar, percent_complete / 100.0);
		return TRUE;
	}

	/* Error! */
	return FALSE;
}

static gboolean
stdout_watch_cb (GIOChannel *source, GIOCondition condition, TotemGalleryProgress *self)
{
	/* Code pilfered from caja-burn-process.c (caja-cd-burner) under GPLv2+
	 * Copyright (C) 2006 William Jon McCann <mccann@jhu.edu> */
	TotemGalleryProgressPrivate *priv = self->priv;
	gboolean retval = TRUE;

	if (condition & G_IO_IN) {
		gchar *line;
		gchar buf[1];
		GIOStatus status;

		status = g_io_channel_read_line (source, &line, NULL, NULL, NULL);

		if (status == G_IO_STATUS_NORMAL) {
			if (priv->line != NULL) {
				g_string_append (priv->line, line);
				g_free (line);
				line = g_string_free (priv->line, FALSE);
				priv->line = NULL;
			}

			retval = process_line (self, line);
			g_free (line);
		} else if (status == G_IO_STATUS_AGAIN) {
			/* A non-terminated line was read, read the data into the buffer. */
			status = g_io_channel_read_chars (source, buf, 1, NULL, NULL);

			if (status == G_IO_STATUS_NORMAL) {
				gchar *line2;

				if (priv->line == NULL)
					priv->line = g_string_new (NULL);
				g_string_append_c (priv->line, buf[0]);

				switch (buf[0]) {
				case '\n':
				case '\r':
				case '\xe2':
				case '\0':
					line2 = g_string_free (priv->line, FALSE);
					priv->line = NULL;

					retval = process_line (self, line2);
					g_free (line2);
					break;
				default:
					break;
				}
			}
		} else if (status == G_IO_STATUS_EOF) {
			/* Show as complete and stop processing */
			gtk_progress_bar_set_fraction (self->priv->progress_bar, 1.0);
			retval = FALSE;
		}
	} else if (condition & G_IO_HUP) {
		retval = FALSE;
	}

	if (retval == FALSE) {
		/* We're done processing input, we now have an answer */
		gtk_dialog_response (GTK_DIALOG (self), GTK_RESPONSE_OK);
	}

	return retval;
}

void
totem_gallery_progress_run (TotemGalleryProgress *self, gint stdout_fd)
{
	GIOChannel *channel;

	fcntl (stdout_fd, F_SETFL, O_NONBLOCK);

	/* Watch the output from totem-video-thumbnailer */
	channel = g_io_channel_unix_new (stdout_fd);
	g_io_channel_set_flags (channel, g_io_channel_get_flags (channel) | G_IO_FLAG_NONBLOCK, NULL);

	g_io_add_watch (channel, G_IO_IN | G_IO_HUP | G_IO_ERR | G_IO_NVAL, (GIOFunc) stdout_watch_cb, self);

	g_io_channel_unref (channel);
}

