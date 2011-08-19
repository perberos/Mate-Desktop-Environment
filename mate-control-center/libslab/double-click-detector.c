/*
 * This file is part of libslab.
 *
 * Copyright (c) 2006, 2007 Novell, Inc.
 *
 * Libslab is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * Libslab is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libslab; if not, write to the Free Software Foundation, Inc., 51
 * Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "double-click-detector.h"

#include <gtk/gtk.h>

#include "libslab-utils.h"

G_DEFINE_TYPE (DoubleClickDetector, double_click_detector, G_TYPE_OBJECT);

static void
double_click_detector_class_init (DoubleClickDetectorClass * detector_class)
{
}

static void
double_click_detector_init (DoubleClickDetector * detector)
{
	GtkSettings *settings;
	gint click_interval;

	settings = gtk_settings_get_default ();

	g_object_get (G_OBJECT (settings), "gtk-double-click-time", &click_interval, NULL);

	detector->double_click_time = (guint32) click_interval;
	detector->last_click_time = 0;
}

DoubleClickDetector *
double_click_detector_new ()
{
	return g_object_new (DOUBLE_CLICK_DETECTOR_TYPE, NULL);
}

gboolean
double_click_detector_is_double_click (DoubleClickDetector *this, guint32 event_time,
                                       gboolean auto_update)
{
	gint32 delta;

	if (event_time <= 0)
		event_time = libslab_get_current_time_millis ();

	if (this->last_click_time <= 0) {
		if (auto_update)
			double_click_detector_update_click_time (this, event_time);

		return FALSE;
	}

	delta = event_time - this->last_click_time;

	if (auto_update)
		double_click_detector_update_click_time (this, event_time);

	return delta < this->double_click_time;
}

void
double_click_detector_update_click_time (DoubleClickDetector *this, guint32 event_time)
{
	if (event_time <= 0)
		event_time = libslab_get_current_time_millis ();

	this->last_click_time = event_time;
}
