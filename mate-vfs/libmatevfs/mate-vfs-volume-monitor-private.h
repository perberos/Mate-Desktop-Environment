/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-volume-monitor-private.h - Handling of volumes for the MATE Virtual File System.

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

#ifndef MATE_VFS_VOLUME_MONITOR_PRIVATE_H
#define MATE_VFS_VOLUME_MONITOR_PRIVATE_H

#include <glib.h>
#include "mate-vfs-volume-monitor.h"

#ifdef USE_DAEMON
#include <dbus/dbus.h>
#endif

#define CONNECTED_SERVERS_DIR "/desktop/mate/connected_servers"

struct _MateVFSVolumeMonitorPrivate {
	GMutex *mutex;

	GList *fstab_drives;
	GList *vfs_drives;

	GList *mtab_volumes;
	GList *server_volumes;
	GList *vfs_volumes;
};

struct _MateVFSVolumePrivate {
	gulong id;
	MateVFSVolumeType volume_type;
	MateVFSDeviceType device_type;
	MateVFSDrive *drive; /* Non-owning ref */

	char *activation_uri;
	char *filesystem_type;
	char *display_name;
	char *display_name_key;
	char *icon;
	
	gboolean is_user_visible;
	gboolean is_read_only;
	gboolean is_mounted;

	/* Only for unix mounts: */
	char *device_path;
	dev_t unix_device;

	/* Only for HAL devices: */
	char *hal_udi;
	char *hal_drive_udi; /* only available to daemon; not exported */

	/* Only for connected servers */
	char *mateconf_id;
};


struct _MateVFSDrivePrivate {
	gulong id;
	MateVFSDeviceType device_type;
	GList *volumes; /* MateVFSVolume list (Owning ref) */

	/* Only for unix mounts: */
	char *device_path;
	
	char *activation_uri;
	char *display_name;
	char *display_name_key;
	char *icon;
	
	gboolean is_user_visible;
	gboolean is_connected;

	/* Only for HAL devices: */
	char *hal_udi;
	char *hal_drive_udi; /* only available to daemon; not exported */
	char *hal_backing_crypto_volume_udi; /* only available to daemon; not exported */

	gboolean must_eject_at_unmount;
};

void mate_vfs_volume_set_drive_private         (MateVFSVolume        *volume,
						 MateVFSDrive         *drive);
void mate_vfs_drive_add_mounted_volume_private (MateVFSDrive         *drive,
						 MateVFSVolume        *volume);
void mate_vfs_drive_remove_volume_private      (MateVFSDrive         *drive,
						 MateVFSVolume        *volume);
void mate_vfs_volume_unset_drive_private       (MateVFSVolume        *volume,
						 MateVFSDrive         *drive);
void _mate_vfs_volume_monitor_mounted          (MateVFSVolumeMonitor *volume_monitor,
						 MateVFSVolume        *volume);
void _mate_vfs_volume_monitor_unmounted        (MateVFSVolumeMonitor *volume_monitor,
						 MateVFSVolume        *volume);
void _mate_vfs_volume_monitor_connected        (MateVFSVolumeMonitor *volume_monitor,
						 MateVFSDrive         *drive);
void _mate_vfs_volume_monitor_disconnected     (MateVFSVolumeMonitor *volume_monitor,
						 MateVFSDrive         *drive);
void _mate_vfs_volume_monitor_disconnect_all   (MateVFSVolumeMonitor *volume_monitor);
void _mate_vfs_volume_monitor_unmount_all      (MateVFSVolumeMonitor *volume_monitor);
void mate_vfs_volume_monitor_emit_pre_unmount (MateVFSVolumeMonitor *volume_monitor,
						MateVFSVolume        *volume);
void _mate_vfs_volume_monitor_force_probe (MateVFSVolumeMonitor *volume_monitor);

MateVFSVolumeMonitor *_mate_vfs_get_volume_monitor_internal (gboolean create);
void _mate_vfs_volume_monitor_shutdown (void);

int _mate_vfs_device_type_get_sort_group (MateVFSDeviceType type);

#ifdef USE_DAEMON
MateVFSVolume *_mate_vfs_volume_from_dbus (DBusMessageIter       *iter,
					     MateVFSVolumeMonitor *volume_monitor);
gboolean        mate_vfs_volume_to_dbus    (DBusMessageIter       *iter,
					     MateVFSVolume        *volume);
gboolean        mate_vfs_drive_to_dbus     (DBusMessageIter       *iter,
					     MateVFSDrive         *drive);
MateVFSDrive * _mate_vfs_drive_from_dbus  (DBusMessageIter       *iter,
					     MateVFSVolumeMonitor *volume_monitor);
#endif

MateVFSVolume *_mate_vfs_volume_monitor_find_mtab_volume_by_activation_uri (MateVFSVolumeMonitor *volume_monitor,
									      const char            *activation_uri);
MateVFSDrive * _mate_vfs_volume_monitor_find_fstab_drive_by_activation_uri (MateVFSVolumeMonitor *volume_monitor,
									      const char            *activation_uri);
MateVFSVolume *_mate_vfs_volume_monitor_find_connected_server_by_mateconf_id  (MateVFSVolumeMonitor *volume_monitor,
									      const char            *id);

#ifdef USE_HAL
MateVFSVolume *_mate_vfs_volume_monitor_find_volume_by_hal_udi (MateVFSVolumeMonitor *volume_monitor,
								  const char            *hal_udi);
MateVFSDrive *_mate_vfs_volume_monitor_find_drive_by_hal_udi   (MateVFSVolumeMonitor *volume_monitor,
								  const char            *hal_udi);

MateVFSVolume *_mate_vfs_volume_monitor_find_volume_by_hal_drive_udi (MateVFSVolumeMonitor *volume_monitor,
									const char            *hal_drive_udi);
MateVFSDrive *_mate_vfs_volume_monitor_find_drive_by_hal_drive_udi   (MateVFSVolumeMonitor *volume_monitor,
									const char            *hal_drive_udi);

#endif /* USE_HAL */

MateVFSVolume *_mate_vfs_volume_monitor_find_volume_by_device_path (MateVFSVolumeMonitor *volume_monitor,
								      const char            *device_path);
MateVFSDrive *_mate_vfs_volume_monitor_find_drive_by_device_path   (MateVFSVolumeMonitor *volume_monitor,
								      const char            *device_path);



char *_mate_vfs_volume_monitor_uniquify_volume_name (MateVFSVolumeMonitor *volume_monitor,
						      const char            *name);
char *_mate_vfs_volume_monitor_uniquify_drive_name  (MateVFSVolumeMonitor *volume_monitor,
						      const char            *name);



#endif /* MATE_VFS_VOLUME_MONITOR_PRIVATE_H */
