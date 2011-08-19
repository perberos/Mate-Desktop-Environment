/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2005-2008  Marcel Holtmann <marcel@holtmann.org>
 *  Copyright (C) 2006-2007  Bastien Nocera <hadess@hadess.net>
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef __BLUETOOTH_FILTER_WIDGET_H
#define __BLUETOOTH_FILTER_WIDGET_H

#include <gtk/gtk.h>
#include <bluetooth-enums.h>
#include <bluetooth-chooser.h>

G_BEGIN_DECLS

#define BLUETOOTH_TYPE_FILTER_WIDGET (bluetooth_filter_widget_get_type())
#define BLUETOOTH_FILTER_WIDGET(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
			       	BLUETOOTH_TYPE_FILTER_WIDGET, BluetoothFilterWidget))
#define BLUETOOTH_FILTER_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), \
				       BLUETOOTH_TYPE_FILTER_WIDGET, BluetoothFilterWidgetClass))
#define BLUETOOTH_IS_FILTER_WIDGET(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
						BLUETOOTH_TYPE_FILTER_WIDGET))
#define BLUETOOTH_IS_FILTER_WIDGET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), \
						BLUETOOTH_TYPE_FILTER_WIDGET))
#define BLUETOOTH_GET_FILTER_WIDGET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), \
				BLUETOOTH_TYPE_FILTER_WIDGET, BluetoothFilterWidgetClass))

typedef struct _BluetoothFilterWidget BluetoothFilterWidget;
typedef struct _BluetoothFilterWidgetClass BluetoothFilterWidgetClass;

struct _BluetoothFilterWidget {
	GtkVBox parent;
};

struct _BluetoothFilterWidgetClass {
	GtkVBoxClass parent_class;
};

GType bluetooth_filter_widget_get_type (void);

GtkWidget *bluetooth_filter_widget_new (void);

void bluetooth_filter_widget_set_title (BluetoothFilterWidget *self, gchar *title);

void bluetooth_filter_widget_bind_filter (BluetoothFilterWidget *self, BluetoothChooser *chooser);

G_END_DECLS

#endif /* __BLUETOOTH_FILTER_WIDGET_H */
