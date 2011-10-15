/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * @file libmatenotify/internal.h Internal definitions
 *
 * @Copyright (C) 2006 Christian Hammond
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 */
#ifndef _LIBMATENOTIFY_INTERNAL_H_
#define _LIBMATENOTIFY_INTERNAL_H_

#include "config.h"

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

#define NOTIFY_DBUS_NAME           "org.freedesktop.Notifications"
#define NOTIFY_DBUS_CORE_INTERFACE "org.freedesktop.Notifications"
#define NOTIFY_DBUS_CORE_OBJECT    "/org/freedesktop/Notifications"

#ifdef __cplusplus
extern "C" {
#endif

DBusGConnection * _notify_get_dbus_g_conn (void);
DBusGProxy      * _notify_get_g_proxy     (void);

void            _notify_cache_add_notification              (NotifyNotification       *n);
void            _notify_cache_remove_notification           (NotifyNotification       *n);
gint            _notify_notification_get_timeout            (const NotifyNotification *n);
gboolean        _notify_notification_has_nondefault_actions (const NotifyNotification *n);
gboolean        _notify_check_spec_version                  (int major, int minor);

G_END_DECLS

#endif /* _LIBMATENOTIFY_INTERNAL_H_ */
