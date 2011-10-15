/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-daemon-method.h - Method that proxies work to the daemon

   Copyright (C) 2003 Red Hat Inc.

   The Mate Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Alexander Larsson <alexl@redhat.com> */

#ifndef MATE_VFS_DAEMON_METHOD_H
#define MATE_VFS_DAEMON_METHOD_H

#include <libmatevfs/mate-vfs-method.h>
#include <dbus/dbus.h>

#ifdef __cplusplus
extern "C" {
#endif

MateVFSMethod *_mate_vfs_daemon_method_get (void);

gboolean          mate_vfs_daemon_message_iter_append_file_info (DBusMessageIter        *iter,
								  const MateVFSFileInfo *info);
gboolean          mate_vfs_daemon_message_append_file_info      (DBusMessage            *message,
								  const MateVFSFileInfo *info);
MateVFSFileInfo *mate_vfs_daemon_message_iter_get_file_info    (DBusMessageIter        *iter);

#ifdef __cplusplus
}
#endif

#endif /* MATE_VFS_DAEMON_METHOD_H */
