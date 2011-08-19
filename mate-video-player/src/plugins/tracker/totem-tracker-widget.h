/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-
 *
 * Copyright (C) 2007 Javier Goday <jgoday@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 * Author : Javier Goday <jgoday@gmail.com>
 */
#ifndef __TOTEM_TRACKER_WIDGET_H__
#define __TOTEM_TRACKER_WIDGET_H__

#include "totem.h"

#include <gtk/gtk.h>
#include <libtracker-client/tracker-client.h>

#define TOTEM_TYPE_TRACKER_WIDGET               (totem_tracker_widget_get_type ())
#define TOTEM_TRACKER_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), TOTEM_TYPE_TRACKER_WIDGET, TotemTrackerWidget))
#define TOTEM_TRACKER_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), TOTEM_TYPE_TRACKER_WIDGET, TotemTrackerWidgetClass))
#define TOTEM_IS_TRACKER_WIDGET(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TOTEM_TYPE_TRACKER_WIDGET))
#define TOTEM_IS_TRACKER_WIDGET_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), TOTEM_TYPE_TRACKER_WIDGET))
#define TOTEM_TRACKER_WIDGET_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), TOTEM_TYPE_TRACKER_WIDGET, TotemTrackerWidgetClass))

typedef struct TotemTrackerWidgetPrivate  TotemTrackerWidgetPrivate;

typedef struct TotemTrackerWidget {
	GtkEventBox parent;
	
	TotemObject *totem;
	TotemTrackerWidgetPrivate *priv;
} TotemTrackerWidget;

typedef struct {
	GtkEventBoxClass parent_class;
} TotemTrackerWidgetClass;

GType       totem_tracker_widget_get_type   (void);
GtkWidget*  totem_tracker_widget_new        (TotemObject *totem); 

#endif /* __TOTEM_TRACKER_WIDGET_H__ */
