/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-volume-monitor-client.h - client implementation of volume monitor

   Copyright (C) 2003 Red Hat, Inc

   The Mate Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Alexander Larsson <alexl@redhat.com>
*/

#ifndef MATE_VFS_VOLUME_MONITOR_CLIENT_H
#define MATE_VFS_VOLUME_MONITOR_CLIENT_H

#include <glib-object.h>
#include <mateconf/mateconf.h>
#include <mateconf/mateconf-client.h>
#include "mate-vfs-volume-monitor.h"

#include <dbus/dbus.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATE_VFS_TYPE_VOLUME_MONITOR_CLIENT        (mate_vfs_volume_monitor_client_get_type ())
#define MATE_VFS_VOLUME_MONITOR_CLIENT(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), MATE_VFS_TYPE_VOLUME_MONITOR_CLIENT, MateVFSVolumeMonitorClient))
#define MATE_VFS_VOLUME_MONITOR_CLIENT_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), MATE_VFS_TYPE_VOLUME_MONITOR_CLIENT, MateVFSVolumeMonitorClientClass))
#define MATE_IS_VFS_VOLUME_MONITOR_CLIENT(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), MATE_VFS_TYPE_VOLUME_MONITOR_CLIENT))
#define MATE_IS_VFS_VOLUME_MONITOR_CLIENT_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), MATE_VFS_TYPE_VOLUME_MONITOR_CLIENT))

typedef struct _MateVFSVolumeMonitorClient MateVFSVolumeMonitorClient;
typedef struct _MateVFSVolumeMonitorClientClass MateVFSVolumeMonitorClientClass;

struct _MateVFSVolumeMonitorClient {
	MateVFSVolumeMonitor parent;
	gboolean is_shutdown;
	DBusConnection *dbus_conn;
};

struct _MateVFSVolumeMonitorClientClass {
	MateVFSVolumeMonitorClass parent_class;
};

GType mate_vfs_volume_monitor_client_get_type (void) G_GNUC_CONST;

void _mate_vfs_volume_monitor_client_daemon_died            (MateVFSVolumeMonitor *volume_monitor);
void mate_vfs_volume_monitor_client_shutdown_private        (MateVFSVolumeMonitorClient *volume_monitor_client);
void _mate_vfs_volume_monitor_client_dbus_force_probe       (MateVFSVolumeMonitorClient *volume_monitor_client);
void _mate_vfs_volume_monitor_client_dbus_emit_pre_unmount  (MateVFSVolumeMonitorClient *volume_monitor_client,
							      MateVFSVolume              *volume);
void _mate_vfs_volume_monitor_client_dbus_emit_mtab_changed (MateVFSVolumeMonitorClient *volume_monitor_client);
void _mate_vfs_volume_monitor_client_dbus_force_probe       (MateVFSVolumeMonitorClient *volume_monitor_client);
void _mate_vfs_volume_monitor_client_dbus_emit_pre_unmount  (MateVFSVolumeMonitorClient *volume_monitor_client,
							      MateVFSVolume              *volume);

#ifdef __cplusplus
}
#endif

#endif /* MATE_VFS_VOLUME_MONITOR_CLIENT_H */
