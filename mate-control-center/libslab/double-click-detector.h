/*
 * This file is part of libslab.
 *
 * Copyright (c) 2006 Novell, Inc.
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

#ifndef __DOUBLE_CLICK_DETECTOR_H__
#define __DOUBLE_CLICK_DETECTOR_H__

#include <glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DOUBLE_CLICK_DETECTOR_TYPE         (double_click_detector_get_type ())
#define DOUBLE_CLICK_DETECTOR(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), DOUBLE_CLICK_DETECTOR_TYPE, DoubleClickDetector))
#define DOUBLE_CLICK_DETECTOR_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c), DOUBLE_CLICK_DETECTOR_TYPE, DoubleClickDetectorClass))
#define IS_DOUBLE_CLICK_DETECTOR(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), DOUBLE_CLICK_DETECTOR_TYPE))
#define IS_DOUBLE_CLICK_DETECTOR_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c), DOUBLE_CLICK_DETECTOR_TYPE))
#define DOUBLE_CLICK_DETECTOR_GET_CLASS(o) (G_TYPE_CHECK_GET_CLASS ((o), DOUBLE_CLICK_DETECTOR_TYPE, DoubleClickDetectorClass))

typedef struct
{
	GObject parent_placeholder;

	guint32 double_click_time;
	guint32 last_click_time;
} DoubleClickDetector;

typedef struct
{
	GObjectClass parent_class;
} DoubleClickDetectorClass;

GType double_click_detector_get_type (void);

DoubleClickDetector *double_click_detector_new (void);

gboolean double_click_detector_is_double_click (DoubleClickDetector * detector, guint32 event_time,
	gboolean auto_update);

void double_click_detector_update_click_time (DoubleClickDetector * detector, guint32 event_time);

#ifdef __cplusplus
}
#endif
#endif /* __DOUBLE_CLICK_DETECTOR_H__ */
