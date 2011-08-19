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

#ifndef __BLUETOOTH_PLUGIN_MANAGER_H
#define __BLUETOOTH_PLUGIN_MANAGER_H

G_BEGIN_DECLS

gboolean bluetooth_plugin_manager_init (void);
void bluetooth_plugin_manager_cleanup (void);

GList *bluetooth_plugin_manager_get_widgets (const char *bdaddr,
					     const char **uuids);
void bluetooth_plugin_manager_device_deleted (const char *bdaddr);

G_END_DECLS

#endif /* __BLUETOOTH_PLUGIN_MANAGER_H */
