/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* mate-vfs-mime-private.h
 *
 * Copyright (C) 2000 Eazel, Inc
 *
 * The Mate Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Mate Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Mate Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef MATE_VFS_MIME_PRIVATE_H
#define MATE_VFS_MIME_PRIVATE_H

#include <libmatevfs/mate-vfs-mime-monitor.h>
#include <libmatevfs/mate-vfs-mime-handlers.h>

#ifdef __cplusplus
extern "C" {
#endif

void _mate_vfs_mime_info_shutdown 	      (void);
void _mate_vfs_mime_monitor_emit_data_changed (MateVFSMIMEMonitor *monitor);

typedef struct FileDateTracker FileDateTracker;

FileDateTracker	*_mate_vfs_file_date_tracker_new                 (void);
void             _mate_vfs_file_date_tracker_free                (FileDateTracker *tracker);
void             _mate_vfs_file_date_tracker_start_tracking_file (FileDateTracker *tracker,
								  const char      *local_file_path);
gboolean         _mate_vfs_file_date_tracker_date_has_changed    (FileDateTracker *tracker);
void             _mate_vfs_mime_info_mark_mate_mime_dir_dirty  (void);
void             _mate_vfs_mime_info_mark_user_mime_dir_dirty   (void);

MateVFSResult _mate_vfs_get_slow_mime_type_internal (const char  *text_uri,
						       char       **mime_type);

#ifdef __cplusplus
}
#endif

#endif /* MATE_VFS_MIME_PRIVATE_H */
