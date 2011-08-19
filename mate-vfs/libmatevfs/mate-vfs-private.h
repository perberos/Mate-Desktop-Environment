/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* mate-vfs-private.h - Private header file for the MATE Virtual
   File System.

   Copyright (C) 1999 Free Software Foundation

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

   Author: Ettore Perazzoli <ettore@gnu.org> */

#ifndef MATE_VFS_PRIVATE_H
#define MATE_VFS_PRIVATE_H

#include <glib.h>
#include <libmatevfs/mate-vfs-volume-monitor.h>

#define MATE_VFS_MODULE_SUBDIR  "mate-vfs-2.0/modules"

typedef void (*MateVFSDaemonForceProbeCallback) (MateVFSVolumeMonitor *volume_monitor);

void mate_vfs_set_is_daemon (GType volume_monitor_type,
			      MateVFSDaemonForceProbeCallback force_probe);
gboolean mate_vfs_get_is_daemon (void);

GType mate_vfs_get_daemon_volume_monitor_type (void);
MateVFSDaemonForceProbeCallback _mate_vfs_get_daemon_force_probe_callback (void);


#endif /* MATE_VFS_PRIVATE_H */
