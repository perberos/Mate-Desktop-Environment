/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-unix-mounts.h - read and monitor fstab/mtab

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

#ifndef MATE_VFS_UNIX_MOUNTS_H
#define MATE_VFS_UNIX_MOUNTS_H

#include <glib.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef G_OS_WIN32
#include <unistd.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	char *mount_path;
	char *device_path;
	char *filesystem_type;
	gboolean is_read_only;
} MateVFSUnixMount;

typedef struct {
	char *mount_path;
	char *device_path;
	char *filesystem_type;
	char *dev_opt;
	gboolean is_read_only;
	gboolean is_user_mountable;
	gboolean is_loopback;
} MateVFSUnixMountPoint;


typedef void (* MateVFSUnixMountCallback) (gpointer user_data);

void     _mate_vfs_unix_mount_free             (MateVFSUnixMount          *mount_entry);
void     _mate_vfs_unix_mount_point_free       (MateVFSUnixMountPoint     *mount_point);
gint     _mate_vfs_unix_mount_compare          (MateVFSUnixMount          *mount_entry1,
						 MateVFSUnixMount          *mount_entry2);
gint     _mate_vfs_unix_mount_point_compare    (MateVFSUnixMountPoint     *mount_point1,
						 MateVFSUnixMountPoint     *mount_point2);
gboolean _mate_vfs_get_unix_mount_table        (GList                     **return_list);
gboolean _mate_vfs_get_current_unix_mounts     (GList                     **return_list);
GList *  _mate_vfs_unix_mount_get_unix_device  (GList                      *mounts);
void     _mate_vfs_monitor_unix_mounts         (MateVFSUnixMountCallback   mount_table_changed,
						 gpointer                    mount_table_changed_user_data,
						 MateVFSUnixMountCallback   current_mounts_changed,
						 gpointer                    current_mounts_user_data);
void     _mate_vfs_stop_monitoring_unix_mounts (void);



#ifdef __cplusplus
}
#endif

#endif /* MATE_VFS_UNIX_MOUNTS_H */
