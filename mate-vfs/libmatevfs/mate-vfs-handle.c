/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-handle.c - Handle object for MATE VFS files.

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

   Author: Ettore Perazzoli <ettore@gnu.org>
*/

#include <config.h>
#include "mate-vfs-handle.h"
#include "mate-vfs-handle-private.h"
#include "mate-vfs-method.h"

#include <glib.h>

struct MateVFSHandle {
	/* URI of the file being accessed through the handle.  */
	MateVFSURI *uri;

	/* Method-specific handle.  */
	MateVFSMethodHandle *method_handle;

	/* Open mode.  */
	MateVFSOpenMode open_mode;
};

#define CHECK_IF_OPEN(handle)			\
G_STMT_START{					\
	if (handle->uri == NULL)		\
		return MATE_VFS_ERROR_NOT_OPEN;	\
}G_STMT_END

#define CHECK_IF_SUPPORTED(handle, what)		\
G_STMT_START{						\
	if (!VFS_METHOD_HAS_FUNC(handle->uri->method, what))		\
		return MATE_VFS_ERROR_NOT_SUPPORTED;	\
}G_STMT_END

#define INVOKE(result, handle, what, params)		\
G_STMT_START{						\
	CHECK_IF_OPEN (handle);				\
	CHECK_IF_SUPPORTED (handle, what);		\
	(result) = handle->uri->method->what params;	\
}G_STMT_END

#define INVOKE_AND_RETURN(handle, what, params)		\
G_STMT_START{						\
        MateVFSResult __result;			\
							\
	INVOKE (__result, handle, what, params);	\
	return __result;				\
}G_STMT_END


MateVFSHandle *
_mate_vfs_handle_new (MateVFSURI *uri,
		      MateVFSMethodHandle *method_handle,
		      MateVFSOpenMode open_mode)
{
	MateVFSHandle *new;

	g_return_val_if_fail (uri != NULL, NULL);
	g_return_val_if_fail (method_handle != NULL, NULL);

	new = g_new (MateVFSHandle, 1);

	new->uri = mate_vfs_uri_ref (uri);
	new->method_handle = method_handle;
	new->open_mode = open_mode;

	return new;
}

void
_mate_vfs_handle_destroy (MateVFSHandle *handle)
{
	g_return_if_fail (handle != NULL);

	mate_vfs_uri_unref (handle->uri);

	g_free (handle);
}


MateVFSOpenMode
_mate_vfs_handle_get_open_mode (MateVFSHandle *handle)
{
	g_return_val_if_fail (handle != NULL, (MateVFSOpenMode) 0);

	return handle->open_mode;
}


/* Actions.  */

MateVFSResult
_mate_vfs_handle_do_close (MateVFSHandle *handle,
			   MateVFSContext *context)
{
	MateVFSResult result;

	INVOKE (result, handle, close, (handle->uri->method, handle->method_handle, context));

	/* Even if close has failed, we shut down the handle. */
	_mate_vfs_handle_destroy (handle);

	return result;
}

MateVFSResult
_mate_vfs_handle_do_read (MateVFSHandle *handle,
			  gpointer buffer,
			  MateVFSFileSize num_bytes,
			  MateVFSFileSize *bytes_read,
			  MateVFSContext *context)
{
	INVOKE_AND_RETURN (handle, read, (handle->uri->method, handle->method_handle,
					  buffer, num_bytes, bytes_read,
					  context));
}

MateVFSResult
_mate_vfs_handle_forget_cache (MateVFSHandle *handle,
				MateVFSFileOffset offset,
				MateVFSFileSize size)
{
	INVOKE_AND_RETURN (handle, forget_cache, (handle->uri->method, handle->method_handle,
						  offset, size));
}


MateVFSResult
_mate_vfs_handle_do_write (MateVFSHandle *handle,
			   gconstpointer buffer,
			   MateVFSFileSize num_bytes,
			   MateVFSFileSize *bytes_written,
			   MateVFSContext *context)
{
	INVOKE_AND_RETURN (handle, write, (handle->uri->method, handle->method_handle,
					   buffer, num_bytes, bytes_written,
					   context));
}


MateVFSResult
_mate_vfs_handle_do_seek (MateVFSHandle *handle,
			  MateVFSSeekPosition whence,
			  MateVFSFileSize offset,
			  MateVFSContext *context)
{
	INVOKE_AND_RETURN (handle, seek, (handle->uri->method, handle->method_handle,
					  whence, offset, context));
}

MateVFSResult
_mate_vfs_handle_do_tell (MateVFSHandle *handle,
			  MateVFSFileSize *offset_return)
{
	INVOKE_AND_RETURN (handle, tell, (handle->uri->method, handle->method_handle,
					  offset_return));
}


MateVFSResult
_mate_vfs_handle_do_get_file_info (MateVFSHandle *handle,
				   MateVFSFileInfo *info,
				   MateVFSFileInfoOptions options,
				   MateVFSContext *context)
{
	INVOKE_AND_RETURN (handle, get_file_info_from_handle,
			   (handle->uri->method, handle->method_handle, info, options,
			    context));
}

MateVFSResult _mate_vfs_handle_do_truncate     (MateVFSHandle *handle,
						 MateVFSFileSize length,
						 MateVFSContext *context)
{
	INVOKE_AND_RETURN (handle, truncate_handle, (handle->uri->method, handle->method_handle, length, context));
}

MateVFSResult
_mate_vfs_handle_do_file_control  (MateVFSHandle          *handle,
				   const char              *operation,
				   gpointer                 operation_data,
				   MateVFSContext         *context)
{
	INVOKE_AND_RETURN (handle, file_control, (handle->uri->method, handle->method_handle, operation, operation_data, context));
}
