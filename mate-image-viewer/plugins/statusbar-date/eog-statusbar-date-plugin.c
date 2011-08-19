/* Statusbar Date -- Shows the EXIF date in EOG's statusbar
 *
 * Copyright (C) 2008 The Free Software Foundation
 *
 * Author: Claudio Saavedra  <csaavedra@mate.org>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "eog-statusbar-date-plugin.h"

#include <gmodule.h>
#include <glib/gi18n-lib.h>

#include <libexif/exif-data.h>

#include <eog-debug.h>
#include <eog-scroll-view.h>
#include <eog-image.h>
#include <eog-thumb-view.h>
#include <eog-exif-util.h>


#define WINDOW_DATA_KEY "EogStatusbarDateWindowData"

EOG_PLUGIN_REGISTER_TYPE(EogStatusbarDatePlugin, eog_statusbar_date_plugin)

typedef struct
{
	GtkWidget *statusbar_date;
	gulong signal_id;
} WindowData;

static void
free_window_data (WindowData *data)
{
	g_return_if_fail (data != NULL);

	eog_debug (DEBUG_PLUGINS);

	g_free (data);
}

static void
statusbar_set_date (GtkStatusbar *statusbar, EogThumbView *view)
{
	EogImage *image;
	gchar *date = NULL;
	gchar time_buffer[32];
	ExifData *exif_data;

	if (eog_thumb_view_get_n_selected (view) == 0)
		return;

	image = eog_thumb_view_get_first_selected_image (view);

	gtk_statusbar_pop (statusbar, 0);

	if (!eog_image_has_data (image, EOG_IMAGE_DATA_EXIF)) {
		if (!eog_image_load (image, EOG_IMAGE_DATA_EXIF, NULL, NULL)) {
			gtk_widget_hide (GTK_WIDGET (statusbar));
		}
	}

	exif_data = (ExifData *) eog_image_get_exif_info (image);
	if (exif_data) {
		date = eog_exif_util_format_date (
			eog_exif_util_get_value (exif_data, EXIF_TAG_DATE_TIME_ORIGINAL, time_buffer, 32));
		exif_data_unref (exif_data);
	}

	if (date) {
		gtk_statusbar_push (statusbar, 0, date);
		gtk_widget_show (GTK_WIDGET (statusbar));
		g_free (date);
	} else {
		gtk_widget_hide (GTK_WIDGET (statusbar));
	}
}

static void
selection_changed_cb (EogThumbView *view, WindowData *data)
{
	statusbar_set_date (GTK_STATUSBAR (data->statusbar_date), view);
}
static void
eog_statusbar_date_plugin_init (EogStatusbarDatePlugin *plugin)
{
	eog_debug_message (DEBUG_PLUGINS, "EogStatusbarDatePlugin initializing");
}

static void
eog_statusbar_date_plugin_finalize (GObject *object)
{
	eog_debug_message (DEBUG_PLUGINS, "EogStatusbarDatePlugin finalizing");

	G_OBJECT_CLASS (eog_statusbar_date_plugin_parent_class)->finalize (object);
}

static void
impl_activate (EogPlugin *plugin,
	       EogWindow *window)
{
	GtkWidget *statusbar = eog_window_get_statusbar (window);
	GtkWidget *thumbview = eog_window_get_thumb_view (window);
	WindowData *data;

	eog_debug (DEBUG_PLUGINS);

	data = g_new (WindowData, 1);
	data->statusbar_date = gtk_statusbar_new ();
	gtk_statusbar_set_has_resize_grip (GTK_STATUSBAR (data->statusbar_date),
					   FALSE);
	gtk_widget_set_size_request (data->statusbar_date, 200, 10);
	gtk_box_pack_end (GTK_BOX (statusbar),
			  data->statusbar_date,
			  FALSE, FALSE, 0);

	data->signal_id = g_signal_connect_after (G_OBJECT (thumbview), "selection_changed",
						  G_CALLBACK (selection_changed_cb), data);

	statusbar_set_date (GTK_STATUSBAR (data->statusbar_date),
			    EOG_THUMB_VIEW (eog_window_get_thumb_view (window)));

	g_object_set_data_full (G_OBJECT (window),
				WINDOW_DATA_KEY,
				data,
				(GDestroyNotify) free_window_data);
}

static void
impl_deactivate	(EogPlugin *plugin,
		 EogWindow *window)
{
	GtkWidget *statusbar = eog_window_get_statusbar (window);
	GtkWidget *view = eog_window_get_thumb_view (window);
	WindowData *data;

	data = (WindowData *) g_object_get_data (G_OBJECT (window),
						 WINDOW_DATA_KEY);

	g_signal_handler_disconnect (view, data->signal_id);

	gtk_container_remove (GTK_CONTAINER (statusbar), data->statusbar_date);

	g_object_set_data (G_OBJECT (window),
			   WINDOW_DATA_KEY,
			   NULL);
}

static void
impl_update_ui (EogPlugin *plugin,
		EogWindow *window)
{
}

static void
eog_statusbar_date_plugin_class_init (EogStatusbarDatePluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	EogPluginClass *plugin_class = EOG_PLUGIN_CLASS (klass);

	object_class->finalize = eog_statusbar_date_plugin_finalize;

	plugin_class->activate = impl_activate;
	plugin_class->deactivate = impl_deactivate;
	plugin_class->update_ui = impl_update_ui;
}
