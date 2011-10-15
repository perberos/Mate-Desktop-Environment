/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-ops.h - Synchronous operations for the MATE Virtual File
   System.

   Copyright (C) 1999, 2001 Free Software Foundation

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

   Author: Ettore Perazzoli <ettore@comm2000.it>
           Seth Nickell <snickell@stanford.edu>
*/

#ifndef MATE_VFS_OPS_H
#define MATE_VFS_OPS_H

#include <libmatevfs/mate-vfs-file-info.h>
#include <libmatevfs/mate-vfs-handle.h>
#include <libmatevfs/mate-vfs-uri.h>
#include <libmatevfs/mate-vfs-monitor.h>

#ifdef __cplusplus
extern "C" {
#endif

MateVFSResult	 mate_vfs_open			(MateVFSHandle **handle,
						 const gchar *text_uri,
						 MateVFSOpenMode open_mode);

MateVFSResult	 mate_vfs_open_uri		(MateVFSHandle **handle,
						 MateVFSURI *uri,
						 MateVFSOpenMode open_mode);

MateVFSResult	 mate_vfs_create		(MateVFSHandle **handle,
						 const gchar *text_uri,
						 MateVFSOpenMode open_mode,
						 gboolean exclusive,
						 guint perm);

MateVFSResult	 mate_vfs_create_uri		(MateVFSHandle **handle,
						 MateVFSURI *uri,
						 MateVFSOpenMode open_mode,
						 gboolean exclusive,
						 guint perm);

MateVFSResult 	 mate_vfs_close 		(MateVFSHandle *handle);

MateVFSResult	 mate_vfs_read			(MateVFSHandle *handle,
						 gpointer buffer,
						 MateVFSFileSize bytes,
						 MateVFSFileSize *bytes_read);

MateVFSResult	 mate_vfs_write 		(MateVFSHandle *handle,
						 gconstpointer buffer,
						 MateVFSFileSize bytes,
						 MateVFSFileSize *bytes_written);

MateVFSResult	 mate_vfs_seek			(MateVFSHandle *handle,
						 MateVFSSeekPosition whence,
						 MateVFSFileOffset offset);

MateVFSResult	 mate_vfs_tell			(MateVFSHandle *handle,
						 MateVFSFileSize *offset_return);

MateVFSResult	 mate_vfs_get_file_info	(const gchar *text_uri,
						 MateVFSFileInfo *info,
						 MateVFSFileInfoOptions options);

MateVFSResult	 mate_vfs_get_file_info_uri    (MateVFSURI *uri,
						 MateVFSFileInfo *info,
						 MateVFSFileInfoOptions options);

MateVFSResult	 mate_vfs_get_file_info_from_handle
						(MateVFSHandle *handle,
						 MateVFSFileInfo *info,
						 MateVFSFileInfoOptions options);

MateVFSResult   mate_vfs_truncate             (const gchar *text_uri,
						 MateVFSFileSize length);
MateVFSResult   mate_vfs_truncate_uri         (MateVFSURI *uri,
						 MateVFSFileSize length);
MateVFSResult   mate_vfs_truncate_handle      (MateVFSHandle *handle,
						 MateVFSFileSize length);

MateVFSResult	 mate_vfs_make_directory_for_uri
						(MateVFSURI *uri, guint perm);
MateVFSResult	 mate_vfs_make_directory	(const gchar *text_uri,
						 guint perm);

MateVFSResult	 mate_vfs_remove_directory_from_uri
						(MateVFSURI *uri);
MateVFSResult	 mate_vfs_remove_directory	(const gchar *text_uri);

MateVFSResult   mate_vfs_unlink_from_uri      (MateVFSURI *uri);
MateVFSResult   mate_vfs_create_symbolic_link (MateVFSURI *uri,
						 const gchar *target_reference);
MateVFSResult   mate_vfs_unlink               (const gchar *text_uri);

MateVFSResult   mate_vfs_move_uri		(MateVFSURI *old_uri,
						 MateVFSURI *new_uri,
						 gboolean force_replace);
MateVFSResult   mate_vfs_move 		(const gchar *old_text_uri,
						 const gchar *new_text_uri,
						 gboolean force_replace);

MateVFSResult	 mate_vfs_check_same_fs_uris	(MateVFSURI *source_uri,
						 MateVFSURI *target_uri,
						 gboolean *same_fs_return);
MateVFSResult	 mate_vfs_check_same_fs	(const gchar *source,
						 const gchar *target,
						 gboolean *same_fs_return);

gboolean	 mate_vfs_uri_exists		(MateVFSURI *uri);

MateVFSResult   mate_vfs_set_file_info_uri    (MateVFSURI *uri,
						 MateVFSFileInfo *info,
						 MateVFSSetFileInfoMask mask);
MateVFSResult   mate_vfs_set_file_info        (const gchar *text_uri,
						 MateVFSFileInfo *info,
						 MateVFSSetFileInfoMask mask);

MateVFSResult mate_vfs_monitor_add (MateVFSMonitorHandle **handle,
                                      const gchar *text_uri,
                                      MateVFSMonitorType monitor_type,
                                      MateVFSMonitorCallback callback,
                                      gpointer user_data);

MateVFSResult mate_vfs_monitor_cancel (MateVFSMonitorHandle *handle);

MateVFSResult mate_vfs_file_control   (MateVFSHandle *handle,
					 const char *operation,
					 gpointer operation_data);

MateVFSResult mate_vfs_forget_cache (MateVFSHandle *handle,
				       MateVFSFileOffset offset,
				       MateVFSFileSize size);


#ifdef __cplusplus
}
#endif

#endif /* MATE_VFS_OPS_H */
