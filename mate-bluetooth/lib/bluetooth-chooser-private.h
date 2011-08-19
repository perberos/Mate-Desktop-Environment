/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2009  Bastien Nocera <hadess@hadess.net>
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

#ifndef __BLUETOOTH_CHOOSER_PRIVATE_H
#define __BLUETOOTH_CHOOSER_PRIVATE_H

#include <glib-object.h>
#include <gtk/gtk.h>
#include <dbus/dbus-glib.h>
#include <bluetooth-enums.h>

G_BEGIN_DECLS

GtkTreeModel *bluetooth_chooser_get_model (BluetoothChooser *self);
GtkTreeViewColumn *bluetooth_chooser_get_type_column (BluetoothChooser *self);
GtkWidget *bluetooth_chooser_get_treeview (BluetoothChooser *self);
gboolean bluetooth_chooser_remove_selected_device (BluetoothChooser *self);

G_END_DECLS

#endif /* __BLUETOOTH_CHOOSER_PRIVATE_H */
