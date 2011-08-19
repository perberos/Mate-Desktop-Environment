/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004 Nokia Corporation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __DBUS_UTILS_H__
#define __DBUS_UTILS_H__

#include <libmatevfs/mate-vfs.h>

#include <dbus/dbus-glib-lowlevel.h>

gboolean          dbus_utils_message_iter_append_file_info (DBusMessageIter        *iter,
							    const MateVFSFileInfo *info);
gboolean          dbus_utils_message_append_file_info      (DBusMessage            *message,
							    const MateVFSFileInfo *info);
MateVFSFileInfo *dbus_utils_message_iter_get_file_info    (DBusMessageIter        *dict);
GList *           dbus_utils_message_get_file_info_list    (DBusMessage            *message);
void              dbus_utils_message_append_volume_list    (DBusMessage            *message,
							    GList                  *volumes);
void              dbus_utils_message_append_drive_list     (DBusMessage            *message,
							    GList                  *drives);
void              dbus_utils_message_append_volume         (DBusMessage            *message,
							    MateVFSVolume         *volume);
void              dbus_utils_message_append_drive          (DBusMessage            *message,
							    MateVFSDrive          *drive);
void              dbus_util_reply_result                   (DBusConnection         *conn,
							    DBusMessage            *message,
							    MateVFSResult          result);
void              dbus_util_reply_id                       (DBusConnection         *conn,
							    DBusMessage            *message,
							    gint32                  id);
void              dbus_util_start_track_name               (DBusConnection         *conn,
							    const char             *name);
void              dbus_util_stop_track_name                (DBusConnection         *conn,
							    const char             *name);


#endif /* __DBUS_UTILS_H__ */
