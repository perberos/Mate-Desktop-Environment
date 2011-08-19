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

#ifndef __BLUETOOTH_CHOOSER_H
#define __BLUETOOTH_CHOOSER_H

#include <gtk/gtk.h>
#include <bluetooth-enums.h>

G_BEGIN_DECLS

#define BLUETOOTH_TYPE_CHOOSER (bluetooth_chooser_get_type())
#define BLUETOOTH_CHOOSER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
				BLUETOOTH_TYPE_CHOOSER, BluetoothChooser))
#define BLUETOOTH_CHOOSER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), \
				BLUETOOTH_TYPE_CHOOSER, BluetoothChooserClass))
#define BLUETOOTH_IS_CHOOSER(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
						BLUETOOTH_TYPE_CHOOSER))
#define BLUETOOTH_IS_CHOOSER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), \
						BLUETOOTH_TYPE_CHOOSER))
#define BLUETOOTH_GET_CHOOSER_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), \
				BLUETOOTH_TYPE_CHOOSER, BluetoothChooserClass))

typedef struct _BluetoothChooser BluetoothChooser;
typedef struct _BluetoothChooserClass BluetoothChooserClass;

struct _BluetoothChooser {
	GtkVBox parent;
};

struct _BluetoothChooserClass {
	GtkVBoxClass parent_class;

	void (*selected_device_changed) (BluetoothChooser *chooser, const char *address);
};

GType bluetooth_chooser_get_type (void);

GtkWidget *bluetooth_chooser_new (const char *title);

void bluetooth_chooser_set_title (BluetoothChooser  *self, const char *title);

char *bluetooth_chooser_get_selected_device (BluetoothChooser *self);
gboolean bluetooth_chooser_get_selected_device_info (BluetoothChooser *self,
						     const char *field,
						     GValue *value);
char *bluetooth_chooser_get_selected_device_name (BluetoothChooser *self);
char * bluetooth_chooser_get_selected_device_icon (BluetoothChooser *self);
BluetoothType bluetooth_chooser_get_selected_device_type (BluetoothChooser *self);
gboolean bluetooth_chooser_get_selected_device_is_connected (BluetoothChooser *self);

void bluetooth_chooser_start_discovery (BluetoothChooser *self);
void bluetooth_chooser_stop_discovery (BluetoothChooser *self);

G_END_DECLS

#endif /* __BLUETOOTH_CHOOSER_H */
