/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2004 Bastien Nocera
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
#include "totem-screenshot.h"

#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <unistd.h>

#include "totem-interface.h"
#include "totem-screenshot-plugin.h"
#include "mate-screenshot-widget.h"

struct TotemScreenshotPrivate {
	MateScreenshotWidget *widget;
};

G_DEFINE_TYPE (TotemScreenshot, totem_screenshot, GTK_TYPE_DIALOG)

static void
totem_screenshot_temp_file_create (TotemScreenshot *screenshot)
{
	char *dir, *fulldir, *temp_filename;
	GdkPixbuf *pixbuf;

	dir = g_strdup_printf ("totem-screenshot-%d", getpid ());
	fulldir = g_build_filename (g_get_tmp_dir (), dir, NULL);
	if (g_mkdir (fulldir, 0700) < 0) {
		g_free (fulldir);
		g_free (dir);
		return;
	}

	/* Write the screenshot to the temporary file */
	temp_filename = g_build_filename (g_get_tmp_dir (), dir, _("Screenshot.png"), NULL);
	pixbuf = mate_screenshot_widget_get_screenshot (screenshot->priv->widget);

	if (gdk_pixbuf_save (pixbuf, temp_filename, "png", NULL, NULL) == FALSE)
		goto error;

	mate_screenshot_widget_set_temporary_filename (screenshot->priv->widget, temp_filename);

error:
	g_free (temp_filename);
}

static void
totem_screenshot_temp_file_remove (MateScreenshotWidget *widget)
{
	char *dirname;
	const gchar *temp_filename;

	temp_filename = mate_screenshot_widget_get_temporary_filename (widget);
	if (temp_filename == NULL)
		return;

	g_unlink (temp_filename);
	dirname = g_path_get_dirname (temp_filename);
	g_rmdir (dirname);
	g_free (dirname);

	mate_screenshot_widget_set_temporary_filename (widget, NULL);
}

static void
totem_screenshot_response (GtkDialog *dialog, int response)
{
	TotemScreenshot *screenshot = TOTEM_SCREENSHOT (dialog);
	char *uri, *path;
	GdkPixbuf *pixbuf;
	GError *err = NULL;
	GFile *file;

	if (response != GTK_RESPONSE_ACCEPT)
		return;

	uri = mate_screenshot_widget_get_uri (screenshot->priv->widget);
	file = g_file_new_for_uri (uri);
	path = g_file_get_path (file);

	pixbuf = mate_screenshot_widget_get_screenshot (screenshot->priv->widget);

	if (gdk_pixbuf_save (pixbuf, path, "png", &err, NULL) == FALSE) {
		totem_interface_error (_("There was an error saving the screenshot."),
				       err->message,
			 	       GTK_WINDOW (screenshot));
		g_error_free (err);
		g_free (uri);
		g_free (path);
		return;
	}

	totem_screenshot_plugin_update_file_chooser (uri);
	g_free (uri);
	g_free (path);
}

static void
totem_screenshot_init (TotemScreenshot *screenshot)
{
	GtkBox *content_area;

	screenshot->priv = G_TYPE_INSTANCE_GET_PRIVATE (screenshot, TOTEM_TYPE_SCREENSHOT, TotemScreenshotPrivate);

	gtk_container_set_border_width (GTK_CONTAINER (screenshot), 5);
	gtk_dialog_add_buttons (GTK_DIALOG (screenshot),
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
				NULL);
	gtk_window_set_title (GTK_WINDOW (screenshot), _("Save Screenshot"));
	gtk_dialog_set_default_response (GTK_DIALOG (screenshot), GTK_RESPONSE_ACCEPT);
	gtk_window_set_resizable (GTK_WINDOW (screenshot), FALSE);

	content_area = GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (screenshot)));
	gtk_box_set_spacing (content_area, 2);
}

GtkWidget *
totem_screenshot_new (Totem *totem, TotemPlugin *screenshot_plugin, GdkPixbuf *screen_image)
{
	TotemScreenshot *screenshot;
	GtkContainer *content_area;
	gchar *movie_title, *interface_path, *initial_uri;

	screenshot = TOTEM_SCREENSHOT (g_object_new (TOTEM_TYPE_SCREENSHOT, NULL));

	movie_title = totem_get_short_title (totem);

	/* Create the screenshot widget */
	/* Translators: %s is the movie title and %d is an auto-incrementing number to make filename unique */
	initial_uri = totem_screenshot_plugin_setup_file_chooser (N_("Screenshot-%s-%d.png"), movie_title);
	g_free (movie_title);
	interface_path = totem_plugin_find_file (screenshot_plugin, "mate-screenshot.ui");
	screenshot->priv->widget = MATE_SCREENSHOT_WIDGET (mate_screenshot_widget_new (interface_path, screen_image, initial_uri));
	g_free (interface_path);
	g_free (initial_uri);

	/* Ensure we remove the temporary file before we're destroyed */
	g_signal_connect (screenshot->priv->widget, "destroy", G_CALLBACK (totem_screenshot_temp_file_remove), NULL);

	content_area = GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (screenshot)));
	gtk_container_add (content_area, GTK_WIDGET (screenshot->priv->widget));
	gtk_container_set_border_width (GTK_CONTAINER (screenshot->priv->widget), 5);

	totem_screenshot_temp_file_create (screenshot);

	return GTK_WIDGET (screenshot);
}

static void
totem_screenshot_class_init (TotemScreenshotClass *klass)
{
	g_type_class_add_private (klass, sizeof (TotemScreenshotPrivate));
	GTK_DIALOG_CLASS (klass)->response = totem_screenshot_response;
}

