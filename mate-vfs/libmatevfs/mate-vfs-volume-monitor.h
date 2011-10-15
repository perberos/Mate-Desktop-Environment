/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-volume-monitor.h - Handling of volumes for the MATE Virtual File System.

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

#ifndef MATE_VFS_VOLUME_MONITOR_H
#define MATE_VFS_VOLUME_MONITOR_H

#include <glib-object.h>
#include <libmatevfs/mate-vfs-volume.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATE_VFS_TYPE_VOLUME_MONITOR        (mate_vfs_volume_monitor_get_type ())
#define MATE_VFS_VOLUME_MONITOR(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), MATE_VFS_TYPE_VOLUME_MONITOR, MateVFSVolumeMonitor))
#define MATE_VFS_VOLUME_MONITOR_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), MATE_VFS_TYPE_VOLUME_MONITOR, MateVFSVolumeMonitorClass))
#define MATE_IS_VFS_VOLUME_MONITOR(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), MATE_VFS_TYPE_VOLUME_MONITOR))
#define MATE_IS_VFS_VOLUME_MONITOR_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), MATE_VFS_TYPE_VOLUME_MONITOR))

typedef struct _MateVFSVolumeMonitorPrivate MateVFSVolumeMonitorPrivate;

typedef struct _MateVFSVolumeMonitor MateVFSVolumeMonitor;
typedef struct _MateVFSVolumeMonitorClass MateVFSVolumeMonitorClass;

struct _MateVFSVolumeMonitor {
	GObject parent;

        MateVFSVolumeMonitorPrivate *priv;
};

struct _MateVFSVolumeMonitorClass {
	GObjectClass parent_class;

	/*< public >*/
	/* signals */
	void (* volume_mounted)	  	(MateVFSVolumeMonitor *volume_monitor,
				   	 MateVFSVolume	       *volume);
	void (* volume_pre_unmount)	(MateVFSVolumeMonitor *volume_monitor,
				   	 MateVFSVolume	       *volume);
	void (* volume_unmounted)	(MateVFSVolumeMonitor *volume_monitor,
				   	 MateVFSVolume	       *volume);
	void (* drive_connected) 	(MateVFSVolumeMonitor *volume_monitor,
				   	 MateVFSDrive	       *drive);
	void (* drive_disconnected)	(MateVFSVolumeMonitor *volume_monitor,
				   	 MateVFSDrive         *drive);
};

GType mate_vfs_volume_monitor_get_type (void) G_GNUC_CONST;

MateVFSVolumeMonitor *mate_vfs_get_volume_monitor   (void);
MateVFSVolumeMonitor *mate_vfs_volume_monitor_ref   (MateVFSVolumeMonitor *volume_monitor);
void                   mate_vfs_volume_monitor_unref (MateVFSVolumeMonitor *volume_monitor);

GList *         mate_vfs_volume_monitor_get_mounted_volumes  (MateVFSVolumeMonitor *volume_monitor);
GList *         mate_vfs_volume_monitor_get_connected_drives (MateVFSVolumeMonitor *volume_monitor);
MateVFSVolume *mate_vfs_volume_monitor_get_volume_for_path  (MateVFSVolumeMonitor *volume_monitor,
							       const char            *path);
MateVFSVolume *mate_vfs_volume_monitor_get_volume_by_id     (MateVFSVolumeMonitor *volume_monitor,
							       gulong                 id);
MateVFSDrive * mate_vfs_volume_monitor_get_drive_by_id      (MateVFSVolumeMonitor *volume_monitor,
							       gulong                 id);

#ifdef __cplusplus
}
#endif

#endif /* MATE_VFS_VOLUME_MONITOR_H */
