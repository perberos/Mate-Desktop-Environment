/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-volume-monitor-daemon.h - daemon implementation of volume monitor

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

#ifndef MATE_VFS_VOLUME_MONITOR_DAEMON_H
#define MATE_VFS_VOLUME_MONITOR_DAEMON_H

#include <glib-object.h>
#include <mateconf/mateconf.h>
#include <mateconf/mateconf-client.h>
#include "mate-vfs-volume-monitor.h"

#ifdef USE_HAL
#include <libhal.h>
#endif /* USE_HAL */

#ifdef __cplusplus
extern "C" {
#endif

#define MATE_VFS_TYPE_VOLUME_MONITOR_DAEMON        (mate_vfs_volume_monitor_daemon_get_type ())
#define MATE_VFS_VOLUME_MONITOR_DAEMON(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), MATE_VFS_TYPE_VOLUME_MONITOR_DAEMON, MateVFSVolumeMonitorDaemon))
#define MATE_VFS_VOLUME_MONITOR_DAEMON_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), MATE_VFS_TYPE_VOLUME_MONITOR_DAEMON, MateVFSVolumeMonitorDaemonClass))
#define MATE_IS_VFS_VOLUME_MONITOR_DAEMON(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), MATE_VFS_TYPE_VOLUME_MONITOR_DAEMON))
#define MATE_IS_VFS_VOLUME_MONITOR_DAEMON_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), MATE_VFS_TYPE_VOLUME_MONITOR_DAEMON))

typedef struct _MateVFSVolumeMonitorDaemon MateVFSVolumeMonitorDaemon;
typedef struct _MateVFSVolumeMonitorDaemonClass MateVFSVolumeMonitorDaemonClass;

struct _MateVFSVolumeMonitorDaemon {
	MateVFSVolumeMonitor parent;

#ifdef USE_HAL
	LibHalContext *hal_ctx;
#endif /* USE_HAL */
	GList *last_fstab;
	GList *last_mtab;
	GList *last_connected_servers;

	MateConfClient *mateconf_client;
	guint connected_id;
};

struct _MateVFSVolumeMonitorDaemonClass {
	MateVFSVolumeMonitorClass parent_class;
};

GType mate_vfs_volume_monitor_daemon_get_type (void) G_GNUC_CONST;

void mate_vfs_volume_monitor_daemon_force_probe (MateVFSVolumeMonitor *volume_monitor_daemon);

#ifdef __cplusplus
}
#endif

#endif /* MATE_VFS_VOLUME_MONITOR_DAEMON_H */
