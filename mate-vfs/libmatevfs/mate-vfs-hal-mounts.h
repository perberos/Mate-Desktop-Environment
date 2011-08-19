/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-hal-mounts.h - read and monitor volumes using freedesktop HAL

   Copyright (C) 2004 Red Hat, Inc.

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

   Author: David Zeuthen <davidz@redhat.com>
*/

#ifndef MATE_VFS_HAL_MOUNTS_H
#define MATE_VFS_HAL_MOUNTS_H

#include "mate-vfs-volume-monitor-daemon.h"

gboolean _mate_vfs_hal_mounts_init (MateVFSVolumeMonitorDaemon *volume_monitor_daemon);

void _mate_vfs_hal_mounts_force_reprobe (MateVFSVolumeMonitorDaemon *volume_monitor_daemon);

void _mate_vfs_hal_mounts_shutdown (MateVFSVolumeMonitorDaemon *volume_monitor_daemon);

MateVFSDrive *_mate_vfs_hal_mounts_modify_drive (MateVFSVolumeMonitorDaemon *volume_monitor_daemon, 
						   MateVFSDrive *drive);
MateVFSVolume *_mate_vfs_hal_mounts_modify_volume (MateVFSVolumeMonitorDaemon *volume_monitor_daemon, 
						     MateVFSVolume *volume);


#endif /* MATE_VFS_HAL_MOUNTS_H */
