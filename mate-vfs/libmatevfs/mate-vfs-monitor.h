/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-monitor.h - File Monitoring for the MATE Virtual File System.

   Copyright (C) 2001 Ian McKellar

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

   Author: Ian McKellar <yakk@yakk.net>
*/

#ifndef MATE_VFS_MONITOR_H
#define MATE_VFS_MONITOR_H

#include <glib.h>

/**
 * MateVFSMonitorType:
 * @MATE_VFS_MONITOR_FILE: the monitor is registered for a single file.
 * @MATE_VFS_MONITOR_DIRECTORY: the monitor is registered for all files in a directory,
 * 				 and the directory itself.
 *
 * Type of resources that can be monitored.
 **/

typedef enum {
  MATE_VFS_MONITOR_FILE,
  MATE_VFS_MONITOR_DIRECTORY
} MateVFSMonitorType;

/**
 * MateVFSMonitorEventType:
 * @MATE_VFS_MONITOR_EVENT_CHANGED: file data changed (FAM, inotify).
 * @MATE_VFS_MONITOR_EVENT_DELETED: file deleted event (FAM, inotify).
 * @MATE_VFS_MONITOR_EVENT_STARTEXECUTING: file was executed (FAM only).
 * @MATE_VFS_MONITOR_EVENT_STOPEXECUTING: executed file isn't executed anymore (FAM only).
 * @MATE_VFS_MONITOR_EVENT_CREATED: file created event (FAM, inotify).
 * @MATE_VFS_MONITOR_EVENT_METADATA_CHANGED: file metadata changed (inotify only).
 * 
 * Types of events that can be monitored.
 **/

typedef enum {
  MATE_VFS_MONITOR_EVENT_CHANGED,
  MATE_VFS_MONITOR_EVENT_DELETED,
  MATE_VFS_MONITOR_EVENT_STARTEXECUTING,
  MATE_VFS_MONITOR_EVENT_STOPEXECUTING,
  MATE_VFS_MONITOR_EVENT_CREATED,
  MATE_VFS_MONITOR_EVENT_METADATA_CHANGED
} MateVFSMonitorEventType;

/**
 * MateVFSMonitorHandle:
 *
 * a handle representing a file or directory monitor that
 * was registered using mate_vfs_monitor_add() and that
 * can be cancelled using mate_vfs_monitor_cancel().
 **/
typedef struct MateVFSMonitorHandle MateVFSMonitorHandle;

/**
 * MateVFSMonitorCallback:
 * @handle: the handle of the monitor that created the event
 * @monitor_uri: the URI of the monitor that was triggered
 * @info_uri: the URI of the actual file this event is concerned with (this can be different
 * from @monitor_uri if it was a directory monitor)
 * @event_type: what happened to @info_uri
 * @user_data: user data passed to mate_vfs_monitor_add() when the monitor was created
 *
 * Function called when a monitor detects a change.
 *
 **/
typedef void (* MateVFSMonitorCallback) (MateVFSMonitorHandle *handle,
                                          const gchar *monitor_uri,
                                          const gchar *info_uri,
                                          MateVFSMonitorEventType event_type,
                                          gpointer user_data);
#endif /* MATE_VFS_MONITOR_H */
