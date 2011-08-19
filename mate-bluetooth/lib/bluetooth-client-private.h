/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2005-2008  Marcel Holtmann <marcel@holtmann.org>
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

#ifndef __BLUETOOTH_CLIENT_PRIVATE_H
#define __BLUETOOTH_CLIENT_PRIVATE_H

#include <glib-object.h>
#include <gtk/gtk.h>
#include <dbus/dbus-glib.h>
#include <bluetooth-enums.h>

G_BEGIN_DECLS

guint bluetooth_class_to_type (guint32 class);

DBusGProxy *bluetooth_client_get_default_adapter(BluetoothClient *client);

gboolean bluetooth_client_start_discovery(BluetoothClient *client);
gboolean bluetooth_client_stop_discovery(BluetoothClient *client);
gboolean bluetooth_client_set_discoverable (BluetoothClient *client,
					    gboolean discoverable);

typedef void (*BluetoothClientCreateDeviceFunc) (BluetoothClient *client,
						 const char *path,
						 const GError *error,
						 gpointer data);

gboolean bluetooth_client_create_device(BluetoothClient *client,
			const char *address, const char *agent,
			BluetoothClientCreateDeviceFunc func, gpointer data);

gboolean bluetooth_client_set_trusted(BluetoothClient *client,
					const char *device, gboolean trusted);

typedef void (*BluetoothClientConnectFunc) (BluetoothClient *client,
					    gboolean success,
					    gpointer data);

gboolean bluetooth_client_connect_service(BluetoothClient *client,
					  const char *device,
					  BluetoothClientConnectFunc func,
					  gpointer data);
gboolean bluetooth_client_disconnect_service (BluetoothClient *client,
					      const char *device,
					      BluetoothClientConnectFunc func,
					      gpointer data);

void bluetooth_client_dump_device (GtkTreeModel *model,
				   GtkTreeIter *iter,
				   gboolean recurse);

G_END_DECLS

#endif /* __BLUETOOTH_CLIENT_PRIVATE_H */
