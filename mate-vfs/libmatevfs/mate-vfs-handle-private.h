/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-handle-private.h - Handle object functions for MATE VFS files.

   Copyright (C) 2002 Seth Nickell

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

   Author: Seth Nickell <snickell@stanford.edu>
*/

#ifndef MATE_VFS_HANDLE_PRIVATE_H
#define MATE_VFS_HANDLE_PRIVATE_H

#ifdef __cplusplus
extern "C" {
#endif

MateVFSHandle * _mate_vfs_handle_new                (MateVFSURI             *uri,
						      MateVFSMethodHandle    *method_handle,
						      MateVFSOpenMode         open_mode);
void             _mate_vfs_handle_destroy            (MateVFSHandle          *handle);
MateVFSOpenMode _mate_vfs_handle_get_open_mode      (MateVFSHandle          *handle);
MateVFSResult   _mate_vfs_handle_do_close           (MateVFSHandle          *handle,
						      MateVFSContext         *context);
MateVFSResult   _mate_vfs_handle_do_read            (MateVFSHandle          *handle,
						      gpointer                 buffer,
						      MateVFSFileSize         num_bytes,
						      MateVFSFileSize        *bytes_read,
						      MateVFSContext         *context);
MateVFSResult   _mate_vfs_handle_do_write           (MateVFSHandle          *handle,
						      gconstpointer            buffer,
						      MateVFSFileSize         num_bytes,
						      MateVFSFileSize        *bytes_written,
						      MateVFSContext         *context);
MateVFSResult   __mate_vfs_handle_do_close_directory (MateVFSHandle          *handle,
						      MateVFSContext         *context);
MateVFSResult   __mate_vfs_handle_do_read_directory  (MateVFSHandle          *handle,
						      MateVFSFileInfo        *file_info,
						      MateVFSContext         *context);
MateVFSResult   _mate_vfs_handle_do_seek            (MateVFSHandle          *handle,
						      MateVFSSeekPosition     whence,
						      MateVFSFileSize         offset,
						      MateVFSContext         *context);
MateVFSResult   _mate_vfs_handle_do_tell            (MateVFSHandle          *handle,
						      MateVFSFileSize        *offset_return);
MateVFSResult   _mate_vfs_handle_do_get_file_info   (MateVFSHandle          *handle,
						      MateVFSFileInfo        *info,
						      MateVFSFileInfoOptions  options,
						      MateVFSContext         *context);
MateVFSResult   _mate_vfs_handle_do_truncate        (MateVFSHandle          *handle,
						      MateVFSFileSize         length,
						      MateVFSContext         *context);
MateVFSResult   _mate_vfs_handle_do_file_control    (MateVFSHandle          *handle,
						      const char              *operation,
						      gpointer                 operation_data,
						      MateVFSContext         *context);
MateVFSResult   _mate_vfs_handle_forget_cache       (MateVFSHandle         *handle,
						       MateVFSFileOffset      offset,
						       MateVFSFileSize        size);

#ifdef __cplusplus
}
#endif

#endif
