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
 *
 * Author: Richard Hult <richard@imendio.com>
 */

#ifndef MATE_VFS_DBUS_UTILS_H
#define MATE_VFS_DBUS_UTILS_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <dbus/dbus.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DVD_DAEMON_SERVICE                          "org.mate.MateVFS.Daemon"
#define DVD_DAEMON_OBJECT                           "/org/mate/MateVFS/Daemon"
#define DVD_DAEMON_INTERFACE                        "org.mate.MateVFS.Daemon"
#define DVD_CLIENT_OBJECT                           "/org/mate/MateVFS/Client"
#define DVD_CLIENT_INTERFACE                        "org.mate.MateVFS.Client"

/* File monitoring signal. */
#define DVD_DAEMON_MONITOR_SIGNAL                   "MonitorSignal"

#define DVD_DAEMON_METHOD_GET_CONNECTION            "GetConnection"

#define DVD_CLIENT_METHOD_CALLBACK                  "Callback"

/* File ops methods. */
#define DVD_DAEMON_METHOD_OPEN                      "Open"
#define DVD_DAEMON_METHOD_CREATE                    "Create"
#define DVD_DAEMON_METHOD_READ                      "Read"
#define DVD_DAEMON_METHOD_WRITE                     "Write"
#define DVD_DAEMON_METHOD_CLOSE                     "Close"

#define DVD_DAEMON_METHOD_SEEK                      "Seek"
#define DVD_DAEMON_METHOD_TELL                      "Tell"

#define DVD_DAEMON_METHOD_TRUNCATE_HANDLE           "TruncateHandle"

#define DVD_DAEMON_METHOD_OPEN_DIRECTORY            "OpenDirectory"
#define DVD_DAEMON_METHOD_CLOSE_DIRECTORY           "CloseDirectory"
#define DVD_DAEMON_METHOD_READ_DIRECTORY            "ReadDirectory"

#define DVD_DAEMON_METHOD_GET_FILE_INFO             "GetFileInfo"
#define DVD_DAEMON_METHOD_GET_FILE_INFO_FROM_HANDLE "GetFileInfoFromHandle"

#define DVD_DAEMON_METHOD_IS_LOCAL                  "IsLocal"
#define DVD_DAEMON_METHOD_MAKE_DIRECTORY            "MakeDirectory"
#define DVD_DAEMON_METHOD_REMOVE_DIRECTORY          "RemoveDirectory"

#define DVD_DAEMON_METHOD_MOVE                      "Move"
#define DVD_DAEMON_METHOD_UNLINK                    "Unlink"
#define DVD_DAEMON_METHOD_CHECK_SAME_FS             "CheckSameFs"
#define DVD_DAEMON_METHOD_SET_FILE_INFO             "SetFileInfo"
#define DVD_DAEMON_METHOD_TRUNCATE                  "Truncate"
#define DVD_DAEMON_METHOD_FIND_DIRECTORY            "FindDirectory"
#define DVD_DAEMON_METHOD_CREATE_SYMBOLIC_LINK      "CreateSymbolicLink"
#define DVD_DAEMON_METHOD_FORGET_CACHE              "ForgetCache"
#define DVD_DAEMON_METHOD_GET_VOLUME_FREE_SPACE     "GetVolumeFreeSpace"

#define DVD_DAEMON_METHOD_MONITOR_ADD               "MonitorAdd"
#define DVD_DAEMON_METHOD_MONITOR_CANCEL            "MonitorCancel"

#define DVD_DAEMON_METHOD_CANCEL                    "Cancel"

/* Volume monitor methods. */
#define DVD_DAEMON_METHOD_REGISTER_VOLUME_MONITOR   "RegisterVolumeMonitor"
#define DVD_DAEMON_METHOD_DEREGISTER_VOLUME_MONITOR "DeregisterVolumeMonitor"
#define DVD_DAEMON_METHOD_GET_VOLUMES               "GetVolumes"
#define DVD_DAEMON_METHOD_GET_DRIVES                "GetDrives"
#define DVD_DAEMON_METHOD_EMIT_PRE_UNMOUNT_VOLUME   "EmitPreUnmountVolume"
#define DVD_DAEMON_METHOD_FORCE_PROBE               "ForceProbe"

/* Volume monitor signals. */
#define DVD_DAEMON_VOLUME_MOUNTED_SIGNAL            "VolumeMountedSignal"
#define DVD_DAEMON_VOLUME_UNMOUNTED_SIGNAL          "VolumeUnmountedSignal"
#define DVD_DAEMON_VOLUME_PRE_UNMOUNT_SIGNAL        "VolumePreUnmountSignal"
#define DVD_DAEMON_DRIVE_CONNECTED_SIGNAL           "DriveConnectedSignal"
#define DVD_DAEMON_DRIVE_DISCONNECTED_SIGNAL        "DriveDisconnectedSignal"

/* Errors. */
#define DVD_ERROR_FAILED            "org.mate.MateVFS.Daemon.Error.Failed"
#define DVD_ERROR_SOCKET_FAILED     "org.mate.MateVFS.Error.SocketFailed"

#define MATE_VFS_VOLUME_DBUS_TYPE \
	DBUS_STRUCT_BEGIN_CHAR_AS_STRING \
	DBUS_TYPE_INT32_AS_STRING \
	DBUS_TYPE_INT32_AS_STRING \
	DBUS_TYPE_INT32_AS_STRING \
	DBUS_TYPE_INT32_AS_STRING \
	DBUS_TYPE_STRING_AS_STRING \
	DBUS_TYPE_STRING_AS_STRING \
	DBUS_TYPE_STRING_AS_STRING \
	DBUS_TYPE_STRING_AS_STRING \
	DBUS_TYPE_BOOLEAN_AS_STRING \
	DBUS_TYPE_BOOLEAN_AS_STRING \
	DBUS_TYPE_BOOLEAN_AS_STRING \
	DBUS_TYPE_STRING_AS_STRING \
	DBUS_TYPE_INT32_AS_STRING \
	DBUS_TYPE_STRING_AS_STRING \
	DBUS_TYPE_STRING_AS_STRING \
	DBUS_STRUCT_END_CHAR_AS_STRING

#define MATE_VFS_DRIVE_DBUS_TYPE \
	DBUS_STRUCT_BEGIN_CHAR_AS_STRING \
	DBUS_TYPE_INT32_AS_STRING \
	DBUS_TYPE_INT32_AS_STRING \
	DBUS_TYPE_ARRAY_AS_STRING \
	 DBUS_TYPE_INT32_AS_STRING \
	DBUS_TYPE_STRING_AS_STRING \
	DBUS_TYPE_STRING_AS_STRING \
	DBUS_TYPE_STRING_AS_STRING \
	DBUS_TYPE_STRING_AS_STRING \
	DBUS_TYPE_STRING_AS_STRING \
	DBUS_TYPE_BOOLEAN_AS_STRING \
	DBUS_TYPE_BOOLEAN_AS_STRING \
	DBUS_TYPE_BOOLEAN_AS_STRING \
	DBUS_STRUCT_END_CHAR_AS_STRING


#define MATE_VFS_FILE_INFO_DBUS_TYPE \
	DBUS_STRUCT_BEGIN_CHAR_AS_STRING \
	DBUS_TYPE_INT32_AS_STRING \
	DBUS_TYPE_STRING_AS_STRING \
	DBUS_TYPE_INT32_AS_STRING \
	DBUS_TYPE_INT32_AS_STRING \
	DBUS_TYPE_INT32_AS_STRING \
	DBUS_TYPE_INT32_AS_STRING \
	DBUS_TYPE_INT64_AS_STRING \
	DBUS_TYPE_INT32_AS_STRING \
	DBUS_TYPE_UINT32_AS_STRING \
	DBUS_TYPE_UINT32_AS_STRING \
	DBUS_TYPE_INT64_AS_STRING \
	DBUS_TYPE_INT64_AS_STRING \
	DBUS_TYPE_INT32_AS_STRING \
	DBUS_TYPE_INT32_AS_STRING \
	DBUS_TYPE_INT32_AS_STRING \
	DBUS_TYPE_STRING_AS_STRING \
	DBUS_TYPE_STRING_AS_STRING \
	DBUS_STRUCT_END_CHAR_AS_STRING




/* Note: It could make sense to have DVD_TYPE_FILE_SIZE and FILE_OFFSET instead
 * of the 64 bits variants.
 */
typedef enum {
	DVD_TYPE_LAST = -1,
	DVD_TYPE_URI,
	DVD_TYPE_STRING,
	DVD_TYPE_INT32,
	DVD_TYPE_INT64,
	DVD_TYPE_UINT64,
	DVD_TYPE_FILE_INFO,
	DVD_TYPE_BOOL,
	DVD_TYPE_BYTE_ARRAY
} DvdArgumentType;

/* Main thread client connection: */
DBusConnection *_mate_vfs_get_main_dbus_connection (void);

/* daemon per-thread connections: */
DBusConnection *_mate_vfs_daemon_get_current_connection (void);
void            mate_vfs_daemon_set_current_connection  (DBusConnection *conn);

#ifdef __cplusplus
}
#endif

#endif /* MATE_VFS_DBUS_UTILS_H */
