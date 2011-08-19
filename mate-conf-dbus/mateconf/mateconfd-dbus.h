/* MateConf
 * Copyright (C) 2003  CodeFactory AB
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef MATECONF_MATECONFD_DBUS_H
#define MATECONF_MATECONFD_DBUS_H

#include <dbus/dbus.h>

gboolean mateconfd_dbus_init                     (void);
gboolean mateconfd_dbus_check_in_shutdown        (DBusConnection   *connection,
					       DBusMessage      *message);
guint    mateconfd_dbus_client_count             (void);

/* Convenience function copied from andercas old code */
gboolean mateconfd_dbus_get_message_args         (DBusConnection *connection,
					       DBusMessage    *message,
					       int             first_arg_type,
					       ...);
gboolean mateconfd_dbus_set_exception            (DBusConnection  *connection,
					       DBusMessage     *message,
					       GError         **error);
gboolean mateconfd_dbus_check_in_shutdown        (DBusConnection *connection,
					       DBusMessage    *message);
DBusConnection *mateconfd_dbus_get_connection    (void);

#endif
