/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-private-ops.h - Private synchronous operations for the MATE
   Virtual File System.

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

#ifndef MATE_VFS_CANCELLABLE_OPS_H
#define MATE_VFS_CANCELLABLE_OPS_H

#include <libmatevfs/mate-vfs-directory.h>
#include <libmatevfs/mate-vfs-find-directory.h>
#include <libmatevfs/mate-vfs-handle.h>
#include <libmatevfs/mate-vfs-xfer.h>

#ifdef __cplusplus
extern "C" {
#endif

MateVFSResult mate_vfs_open_uri_cancellable
					(MateVFSHandle **handle,
					 MateVFSURI *uri,
					 MateVFSOpenMode open_mode,
					 MateVFSContext *context);

MateVFSResult mate_vfs_create_uri_cancellable
					(MateVFSHandle **handle,
					 MateVFSURI *uri,
					 MateVFSOpenMode open_mode,
					 gboolean exclusive,
					 guint perm,
					 MateVFSContext *context);

MateVFSResult mate_vfs_close_cancellable
					(MateVFSHandle *handle,
					 MateVFSContext *context);

MateVFSResult mate_vfs_read_cancellable
					(MateVFSHandle *handle,
					 gpointer buffer,
					 MateVFSFileSize bytes,
					 MateVFSFileSize *bytes_written,
					 MateVFSContext *context);

MateVFSResult mate_vfs_write_cancellable
					(MateVFSHandle *handle,
					 gconstpointer buffer,
					 MateVFSFileSize bytes,
					 MateVFSFileSize *bytes_written,
					 MateVFSContext *context);

MateVFSResult mate_vfs_seek_cancellable
					(MateVFSHandle *handle,
					 MateVFSSeekPosition whence,
					 MateVFSFileOffset offset,
					 MateVFSContext *context);

MateVFSResult mate_vfs_get_file_info_uri_cancellable
					(MateVFSURI *uri,
					 MateVFSFileInfo *info,
					 MateVFSFileInfoOptions options,
					 MateVFSContext *context);

MateVFSResult mate_vfs_get_file_info_from_handle_cancellable
					(MateVFSHandle *handle,
					 MateVFSFileInfo *info,
					 MateVFSFileInfoOptions options,
					 MateVFSContext *context);

MateVFSResult mate_vfs_truncate_uri_cancellable (MateVFSURI *uri,
						   MateVFSFileSize length,
						   MateVFSContext *context);

MateVFSResult mate_vfs_truncate_handle_cancellable (MateVFSHandle *handle,
						      MateVFSFileSize length,
						      MateVFSContext *context);

MateVFSResult mate_vfs_make_directory_for_uri_cancellable
					(MateVFSURI *uri,
					 guint perm,
					 MateVFSContext *context);

MateVFSResult mate_vfs_find_directory_cancellable (MateVFSURI *near_uri,
						     MateVFSFindDirectoryKind kind,
						     MateVFSURI **result_uri,
						     gboolean create_if_needed,
		  				     gboolean find_if_needed,
						     guint permissions,
						     MateVFSContext *context);

MateVFSResult mate_vfs_remove_directory_from_uri_cancellable
					(MateVFSURI *uri,
					 MateVFSContext *context);

MateVFSResult mate_vfs_unlink_from_uri_cancellable
					(MateVFSURI *uri,
					 MateVFSContext *context);
MateVFSResult mate_vfs_create_symbolic_link_cancellable
                                        (MateVFSURI *uri,
					 const gchar *target_reference,
					 MateVFSContext *context);
MateVFSResult mate_vfs_move_uri_cancellable
					(MateVFSURI *old,
					 MateVFSURI *new,
					 gboolean force_replace,
					 MateVFSContext *context);

MateVFSResult mate_vfs_check_same_fs_uris_cancellable
					 (MateVFSURI *a,
					  MateVFSURI *b,
					  gboolean *same_fs_return,
					  MateVFSContext *context);

MateVFSResult mate_vfs_set_file_info_cancellable
					 (MateVFSURI *a,
					  const MateVFSFileInfo *info,
					  MateVFSSetFileInfoMask mask,
					  MateVFSContext *context);

MateVFSResult _mate_vfs_xfer_private   (const GList *source_uri_list,
					 const GList *target_uri_list,
					 MateVFSXferOptions xfer_options,
					 MateVFSXferErrorMode error_mode,
					 MateVFSXferOverwriteMode overwrite_mode,
					 MateVFSXferProgressCallback progress_callback,
					 gpointer data,
					 MateVFSXferProgressCallback sync_progress_callback,
					 gpointer sync_progress_data);

MateVFSResult	mate_vfs_directory_read_next_cancellable
					(MateVFSDirectoryHandle *handle,
					 MateVFSFileInfo *info,
					 MateVFSContext *context);

MateVFSResult  mate_vfs_directory_open_from_uri_cancellable
					(MateVFSDirectoryHandle **handle,
					 MateVFSURI *uri,
					 MateVFSFileInfoOptions options,
					 MateVFSContext *context);

MateVFSResult  mate_vfs_file_control_cancellable
                                        (MateVFSHandle *handle,
					 const char *operation,
					 gpointer operation_data,
					 MateVFSContext *context);

#ifdef __cplusplus
}
#endif

#endif /* MATE_VFS_CANCELLABLE_OPS_H */
