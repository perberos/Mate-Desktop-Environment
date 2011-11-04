/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-method.h - Virtual class for access methods in the MATE
   Virtual File System.

   Copyright (C) 1999, 2001 Free Software Foundation

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

   Author: Ettore Perazzoli <ettore@comm2000.it>
           Seth Nickell <snickell@stanford.edu>
*/

#ifndef MATE_VFS_METHOD_H
#define MATE_VFS_METHOD_H

/*
 * The following include helps Solaris copy with its own headers.  (With 64-
 * bit stuff enabled they like to #define open open64, etc.)
 * See http://bugzilla.gnome.org/show_bug.cgi?id=71184 for details.
 */
#ifndef _WIN32
#include <unistd.h>
#endif

#include <libmatevfs/mate-vfs-context.h>
#include <libmatevfs/mate-vfs-file-info.h>
#include <libmatevfs/mate-vfs-find-directory.h>
#include <libmatevfs/mate-vfs-handle.h>
#include <libmatevfs/mate-vfs-transform.h>
#include <libmatevfs/mate-vfs-monitor.h>

#ifdef __cplusplus
extern "C" {
#endif

#define _MATE_VFS_METHOD_PARAM_CHECK(expression)			\
	g_return_val_if_fail ((expression), MATE_VFS_ERROR_BAD_PARAMETERS);

typedef struct MateVFSMethod MateVFSMethod;

typedef MateVFSMethod * (* MateVFSMethodInitFunc)(const char *method_name, const char *config_args);
typedef void (*MateVFSMethodShutdownFunc)(MateVFSMethod *method);

typedef MateVFSResult (* MateVFSMethodOpenFunc)
					(MateVFSMethod *method,
					 MateVFSMethodHandle
			       	 	**method_handle_return,
					 MateVFSURI *uri,
					 MateVFSOpenMode mode,
					 MateVFSContext *context);

typedef MateVFSResult (* MateVFSMethodCreateFunc)
					(MateVFSMethod *method,
					 MateVFSMethodHandle
			       	 	**method_handle_return,
					 MateVFSURI *uri,
					 MateVFSOpenMode mode,
					 gboolean exclusive,
					 guint perm,
					 MateVFSContext *context);

typedef MateVFSResult (* MateVFSMethodCloseFunc)
					(MateVFSMethod *method,
					 MateVFSMethodHandle *method_handle,
					 MateVFSContext *context);

typedef MateVFSResult (* MateVFSMethodReadFunc)
					(MateVFSMethod *method,
					 MateVFSMethodHandle *method_handle,
					 gpointer buffer,
					 MateVFSFileSize num_bytes,
					 MateVFSFileSize *bytes_read_return,
					 MateVFSContext *context);

typedef MateVFSResult (* MateVFSMethodWriteFunc)
					(MateVFSMethod *method,
					 MateVFSMethodHandle *method_handle,
					 gconstpointer buffer,
					 MateVFSFileSize num_bytes,
					 MateVFSFileSize *bytes_written_return,
					 MateVFSContext *context);

typedef MateVFSResult (* MateVFSMethodSeekFunc)
					(MateVFSMethod *method,
					 MateVFSMethodHandle *method_handle,
					 MateVFSSeekPosition  whence,
					 MateVFSFileOffset    offset,
					 MateVFSContext *context);

typedef MateVFSResult (* MateVFSMethodTellFunc)
					(MateVFSMethod *method,
					 MateVFSMethodHandle *method_handle,
					 MateVFSFileSize *offset_return);

typedef MateVFSResult (* MateVFSMethodOpenDirectoryFunc)
					(MateVFSMethod *method,
					 MateVFSMethodHandle **method_handle,
					 MateVFSURI *uri,
					 MateVFSFileInfoOptions options,
					 MateVFSContext *context);

typedef MateVFSResult (* MateVFSMethodCloseDirectoryFunc)
					(MateVFSMethod *method,
					 MateVFSMethodHandle *method_handle,
					 MateVFSContext *context);

typedef MateVFSResult (* MateVFSMethodReadDirectoryFunc)
					(MateVFSMethod *method,
					 MateVFSMethodHandle *method_handle,
					 MateVFSFileInfo *file_info,
					 MateVFSContext *context);

typedef MateVFSResult (* MateVFSMethodGetFileInfoFunc)
					(MateVFSMethod *method,
					 MateVFSURI *uri,
					 MateVFSFileInfo *file_info,
					 MateVFSFileInfoOptions options,
					 MateVFSContext *context);

typedef MateVFSResult (* MateVFSMethodGetFileInfoFromHandleFunc)
					(MateVFSMethod *method,
					 MateVFSMethodHandle *method_handle,
					 MateVFSFileInfo *file_info,
					 MateVFSFileInfoOptions options,
					 MateVFSContext *context);

typedef MateVFSResult (* MateVFSMethodTruncateFunc) (MateVFSMethod *method,
						       MateVFSURI *uri,
						       MateVFSFileSize length,
						       MateVFSContext *context);
typedef MateVFSResult (* MateVFSMethodTruncateHandleFunc) (MateVFSMethod *method,
							     MateVFSMethodHandle *handle,
							     MateVFSFileSize length,
							     MateVFSContext *context);

typedef gboolean       (* MateVFSMethodIsLocalFunc)
					(MateVFSMethod *method,
					 const MateVFSURI *uri);

typedef MateVFSResult (* MateVFSMethodMakeDirectoryFunc)
					(MateVFSMethod *method,
					 MateVFSURI *uri,
					 guint perm,
					 MateVFSContext *context);

typedef MateVFSResult (* MateVFSMethodFindDirectoryFunc)
					(MateVFSMethod *method,
					 MateVFSURI *find_near_uri,
					 MateVFSFindDirectoryKind kind,
					 MateVFSURI **result_uri,
					 gboolean create_if_needed,
					 gboolean find_if_needed,
					 guint perm,
					 MateVFSContext *context);

typedef MateVFSResult (* MateVFSMethodRemoveDirectoryFunc)
					(MateVFSMethod *method,
					 MateVFSURI *uri,
					 MateVFSContext *context);

typedef MateVFSResult (* MateVFSMethodMoveFunc)
					(MateVFSMethod *method,
					 MateVFSURI *old_uri,
					 MateVFSURI *new_uri,
					 gboolean force_replace,
					 MateVFSContext *context);

typedef MateVFSResult (* MateVFSMethodUnlinkFunc)
                                        (MateVFSMethod *method,
					 MateVFSURI *uri,
					 MateVFSContext *context);

typedef MateVFSResult (* MateVFSMethodCheckSameFSFunc)
					(MateVFSMethod *method,
					 MateVFSURI *a,
					 MateVFSURI *b,
					 gboolean *same_fs_return,
					 MateVFSContext *context);

typedef MateVFSResult (* MateVFSMethodSetFileInfo)
					(MateVFSMethod *method,
					 MateVFSURI *a,
					 const MateVFSFileInfo *info,
					 MateVFSSetFileInfoMask mask,
					 MateVFSContext *context);

typedef MateVFSResult (* MateVFSMethodCreateSymbolicLinkFunc)
                                        (MateVFSMethod *method,
                                         MateVFSURI *uri,
                                         const gchar *target_reference,
                                         MateVFSContext *context);
typedef MateVFSResult (* MateVFSMethodMonitorAddFunc)
     					(MateVFSMethod *method,
      					 MateVFSMethodHandle **method_handle_return,
      					 MateVFSURI *uri,
      					 MateVFSMonitorType monitor_type);

typedef MateVFSResult (* MateVFSMethodMonitorCancelFunc)
     					(MateVFSMethod *method,
      					 MateVFSMethodHandle *handle);

typedef MateVFSResult (* MateVFSMethodFileControlFunc)
     					(MateVFSMethod *method,
      					 MateVFSMethodHandle *method_handle,
					 const char *operation,
					 gpointer operation_data,
					 MateVFSContext *context);

typedef MateVFSResult (* MateVFSMethodForgetCacheFunc)
     					(MateVFSMethod *method,
      					 MateVFSMethodHandle *method_handle,
					 MateVFSFileOffset offset,
					 MateVFSFileSize size);

typedef MateVFSResult (* MateVFSMethodGetVolumeFreeSpaceFunc)
     					(MateVFSMethod *method,
					 const MateVFSURI *uri,
				 	 MateVFSFileSize *free_space);


/* Use this macro to test whether a given function is implemented in
 * a given MateVFSMethod.  Note that it checks the expected size of the structure
 * prior to testing NULL.
 */

#define VFS_METHOD_HAS_FUNC(method,func) ((((char *)&((method)->func)) - ((char *)(method)) < (method)->method_table_size) && method->func != NULL)

/* Structure defining an access method.	 This is also defined as an
   opaque type in `mate-vfs-types.h'.	*/
struct MateVFSMethod {
	gsize method_table_size;			/* Used for versioning */
	MateVFSMethodOpenFunc open;
	MateVFSMethodCreateFunc create;
	MateVFSMethodCloseFunc close;
	MateVFSMethodReadFunc read;
	MateVFSMethodWriteFunc write;
	MateVFSMethodSeekFunc seek;
	MateVFSMethodTellFunc tell;
	MateVFSMethodTruncateHandleFunc truncate_handle;
	MateVFSMethodOpenDirectoryFunc open_directory;
	MateVFSMethodCloseDirectoryFunc close_directory;
	MateVFSMethodReadDirectoryFunc read_directory;
	MateVFSMethodGetFileInfoFunc get_file_info;
	MateVFSMethodGetFileInfoFromHandleFunc get_file_info_from_handle;
	MateVFSMethodIsLocalFunc is_local;
	MateVFSMethodMakeDirectoryFunc make_directory;
	MateVFSMethodRemoveDirectoryFunc remove_directory;
	MateVFSMethodMoveFunc move;
	MateVFSMethodUnlinkFunc unlink;
	MateVFSMethodCheckSameFSFunc check_same_fs;
	MateVFSMethodSetFileInfo set_file_info;
	MateVFSMethodTruncateFunc truncate;
	MateVFSMethodFindDirectoryFunc find_directory;
	MateVFSMethodCreateSymbolicLinkFunc create_symbolic_link;
	MateVFSMethodMonitorAddFunc monitor_add;
	MateVFSMethodMonitorCancelFunc monitor_cancel;
	MateVFSMethodFileControlFunc file_control;
	MateVFSMethodForgetCacheFunc forget_cache;
	MateVFSMethodGetVolumeFreeSpaceFunc get_volume_free_space;
};

gboolean	   mate_vfs_method_init   (void);
MateVFSMethod    *mate_vfs_method_get    (const gchar *name);
MateVFSTransform *mate_vfs_transform_get (const gchar *name);

void              _mate_vfs_method_shutdown (void);

#ifdef __cplusplus
}
#endif

#endif /* MATE_VFS_METHOD_H */
