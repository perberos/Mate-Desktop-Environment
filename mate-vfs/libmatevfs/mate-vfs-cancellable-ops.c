/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-private-ops.c - Private synchronous operations for the MATE
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

/* This file provides private versions of the ops for internal use.  These are
   meant to be used within the MATE VFS and its modules: they are not for
   public consumption through the external API.  */

#include <config.h>
#include "mate-vfs-cancellable-ops.h"
#include "mate-vfs-method.h"
#include "mate-vfs-private-utils.h"
#include "mate-vfs-utils.h"
#include "mate-vfs-handle-private.h"

#include <glib.h>
#include <string.h>

MateVFSResult
mate_vfs_open_uri_cancellable (MateVFSHandle **handle,
				MateVFSURI *uri,
				MateVFSOpenMode open_mode,
				MateVFSContext *context)
{
	MateVFSMethodHandle *method_handle;
	MateVFSResult result;

	g_return_val_if_fail (handle != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (uri != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (uri->method != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);

	if (mate_vfs_context_check_cancellation (context))
		return MATE_VFS_ERROR_CANCELLED;

	if (!VFS_METHOD_HAS_FUNC(uri->method, open))
		return MATE_VFS_ERROR_NOT_SUPPORTED;

	result = (uri->method->open) (uri->method, &method_handle, uri, open_mode,
				      context);

	if (result != MATE_VFS_OK)
		return result;

	*handle = _mate_vfs_handle_new (uri, method_handle, open_mode);
	
	return MATE_VFS_OK;
}

MateVFSResult
mate_vfs_create_uri_cancellable (MateVFSHandle **handle,
				  MateVFSURI *uri,
				  MateVFSOpenMode open_mode,
				  gboolean exclusive,
				  guint perm,
				  MateVFSContext *context)
{
	MateVFSMethodHandle *method_handle;
	MateVFSResult result;

	g_return_val_if_fail (handle != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (uri != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);

	if (mate_vfs_context_check_cancellation (context))
		return MATE_VFS_ERROR_CANCELLED;

	if (!VFS_METHOD_HAS_FUNC(uri->method, create))
		return MATE_VFS_ERROR_NOT_SUPPORTED;

	result = uri->method->create (uri->method, &method_handle, uri, open_mode,
				      exclusive, perm, context);
	if (result != MATE_VFS_OK)
		return result;

	*handle = _mate_vfs_handle_new (uri, method_handle, open_mode);

	return MATE_VFS_OK;
}

MateVFSResult
mate_vfs_close_cancellable (MateVFSHandle *handle,
			     MateVFSContext *context)
{
	g_return_val_if_fail (handle != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);

	if (mate_vfs_context_check_cancellation (context))
		return MATE_VFS_ERROR_CANCELLED;

	return _mate_vfs_handle_do_close (handle, context);
}

MateVFSResult
mate_vfs_read_cancellable (MateVFSHandle *handle,
			    gpointer buffer,
			    MateVFSFileSize bytes,
			    MateVFSFileSize *bytes_read,
			    MateVFSContext *context)
{
	MateVFSFileSize dummy_bytes_read;
	MateVFSResult res;

	g_return_val_if_fail (handle != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);

	if (mate_vfs_context_check_cancellation (context))
		return MATE_VFS_ERROR_CANCELLED;

	if (bytes_read == NULL) {
		bytes_read = &dummy_bytes_read;
	}

	if (bytes == 0) {
		*bytes_read = 0;
		return MATE_VFS_OK;
	}

	res = _mate_vfs_handle_do_read (handle, buffer, bytes, bytes_read,
					 context);
	if (res != MATE_VFS_OK) {
		*bytes_read = 0;
	}

	return res;
}

MateVFSResult
mate_vfs_write_cancellable (MateVFSHandle *handle,
			     gconstpointer buffer,
			     MateVFSFileSize bytes,
			     MateVFSFileSize *bytes_written,
			     MateVFSContext *context)
{
	MateVFSFileSize dummy_bytes_written;

	g_return_val_if_fail (handle != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);

	if (mate_vfs_context_check_cancellation (context))
		return MATE_VFS_ERROR_CANCELLED;

	if (bytes_written == NULL) {
		bytes_written = &dummy_bytes_written;
	}

	if (bytes == 0) {
		*bytes_written = 0;
		return MATE_VFS_OK;
	}
	
	return _mate_vfs_handle_do_write (handle, buffer, bytes,
					  bytes_written, context);
}

MateVFSResult
mate_vfs_seek_cancellable (MateVFSHandle *handle,
			    MateVFSSeekPosition whence,
			    MateVFSFileOffset offset,
			    MateVFSContext *context)
{
	g_return_val_if_fail (handle != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);

	if (mate_vfs_context_check_cancellation (context))
		return MATE_VFS_ERROR_CANCELLED;

	return _mate_vfs_handle_do_seek (handle, whence, offset, context);
}

MateVFSResult
mate_vfs_get_file_info_uri_cancellable (MateVFSURI *uri,
					 MateVFSFileInfo *info,
					 MateVFSFileInfoOptions options,
					 MateVFSContext *context)
{
	MateVFSResult result;

	g_return_val_if_fail (uri != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (info != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);
	
	if (mate_vfs_context_check_cancellation (context))
		return MATE_VFS_ERROR_CANCELLED;

	if (!VFS_METHOD_HAS_FUNC(uri->method, get_file_info))
		return MATE_VFS_ERROR_NOT_SUPPORTED;

	result = uri->method->get_file_info (uri->method, uri, info, options,
					     context);

	return result;
}

MateVFSResult
mate_vfs_get_file_info_from_handle_cancellable (MateVFSHandle *handle,
						 MateVFSFileInfo *info,
						 MateVFSFileInfoOptions options,
						 MateVFSContext *context)

{
	MateVFSResult result;

	g_return_val_if_fail (handle != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);

	if (mate_vfs_context_check_cancellation (context))
		return MATE_VFS_ERROR_CANCELLED;


	result =  _mate_vfs_handle_do_get_file_info (handle, info,
						     options,
						     context);

	return result;
}

MateVFSResult
mate_vfs_truncate_uri_cancellable (MateVFSURI *uri,
				    MateVFSFileSize length,
				    MateVFSContext *context)
{
	g_return_val_if_fail (uri != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);

	if (mate_vfs_context_check_cancellation (context))
		return MATE_VFS_ERROR_CANCELLED;

	if (!VFS_METHOD_HAS_FUNC(uri->method, truncate))
		return MATE_VFS_ERROR_NOT_SUPPORTED;

	return uri->method->truncate(uri->method, uri, length, context);
}

MateVFSResult
mate_vfs_truncate_handle_cancellable (MateVFSHandle *handle,
				       MateVFSFileSize length,
				       MateVFSContext *context)
{
	g_return_val_if_fail (handle != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);

	if (mate_vfs_context_check_cancellation (context))
		return MATE_VFS_ERROR_CANCELLED;

	return _mate_vfs_handle_do_truncate (handle, length, context);
}

MateVFSResult
mate_vfs_make_directory_for_uri_cancellable (MateVFSURI *uri,
					      guint perm,
					      MateVFSContext *context)
{
	MateVFSResult result;

	g_return_val_if_fail (uri != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);

	if (mate_vfs_context_check_cancellation (context))
		return MATE_VFS_ERROR_CANCELLED;

	if (!VFS_METHOD_HAS_FUNC(uri->method, make_directory))
		return MATE_VFS_ERROR_NOT_SUPPORTED;

	result = uri->method->make_directory (uri->method, uri, perm, context);
	return result;
}

MateVFSResult
mate_vfs_find_directory_cancellable (MateVFSURI *near_uri,
				      MateVFSFindDirectoryKind kind,
				      MateVFSURI **result_uri,
				      gboolean create_if_needed,
				      gboolean find_if_needed,
				      guint permissions,
				      MateVFSContext *context)
{
	MateVFSResult result;
	MateVFSURI *resolved_uri;

	g_return_val_if_fail (result_uri != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);

	*result_uri = NULL;

	if (mate_vfs_context_check_cancellation (context))
		return MATE_VFS_ERROR_CANCELLED;

	if (near_uri != NULL) {
		mate_vfs_uri_ref (near_uri);
	} else {
		char *text_uri;

		text_uri = mate_vfs_get_uri_from_local_path (g_get_home_dir ());
		g_assert (text_uri != NULL);
		/* assume file: method and the home directory */
		near_uri = mate_vfs_uri_new (text_uri);
		g_free (text_uri);
	}

	g_assert (near_uri != NULL);

	if (!VFS_METHOD_HAS_FUNC (near_uri->method, find_directory)) {
		/* skip file systems not supporting find_directory.
		 *
		 * TODO if we decide to introduce cross-method links (e.g. http allows
		 * arbitrary URIs), this could be slightly wrong, because the target
		 * method may support find_directory, so we'd also have to make sure
		 * that a method doesn't support cross-method links.
		 **/
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

	/* Need to expand the final symlink, since if the directory is a symlink
	 * we want to look at the device the symlink points to, not the one the
	 * symlink is stored on
	 */
	result = _mate_vfs_uri_resolve_all_symlinks_uri (near_uri, &resolved_uri);
	if (result == MATE_VFS_OK) {
		mate_vfs_uri_unref (near_uri);
		near_uri = resolved_uri;
	} else
		return result;

	g_assert (near_uri != NULL);

	if (!VFS_METHOD_HAS_FUNC(near_uri->method, find_directory)) {
		mate_vfs_uri_unref (near_uri);
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

	result = near_uri->method->find_directory (near_uri->method, near_uri, kind,
		result_uri, create_if_needed, find_if_needed, permissions, context);

	mate_vfs_uri_unref (near_uri);
	return result;
}

MateVFSResult
mate_vfs_remove_directory_from_uri_cancellable (MateVFSURI *uri,
						 MateVFSContext *context)
{
	MateVFSResult result;

	g_return_val_if_fail (uri != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);

	if (mate_vfs_context_check_cancellation (context)) {
		return MATE_VFS_ERROR_CANCELLED;
	}

	if (!VFS_METHOD_HAS_FUNC(uri->method, remove_directory)) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

	result = uri->method->remove_directory (uri->method, uri, context);
	return result;
}

MateVFSResult
mate_vfs_unlink_from_uri_cancellable (MateVFSURI *uri,
				       MateVFSContext *context)
{
	g_return_val_if_fail (uri != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);

	if (mate_vfs_context_check_cancellation (context)) {
		return MATE_VFS_ERROR_CANCELLED;
	}

	if (!VFS_METHOD_HAS_FUNC(uri->method, unlink)) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

	return uri->method->unlink (uri->method, uri, context);
}

MateVFSResult
mate_vfs_create_symbolic_link_cancellable (MateVFSURI *uri,
					    const char *target_reference,
					    MateVFSContext *context)
{
	g_return_val_if_fail (uri != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);
	
	if (mate_vfs_context_check_cancellation (context)) {
		return MATE_VFS_ERROR_CANCELLED;
	}

	if (!VFS_METHOD_HAS_FUNC(uri->method, create_symbolic_link)) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

	return uri->method->create_symbolic_link (uri->method, uri, target_reference, context);
}

static gboolean
check_same_fs_in_uri (MateVFSURI *a,
		      MateVFSURI *b)
{
	if (a->method != b->method) {
		return FALSE;
	}
	
	if (strcmp (a->method_string, b->method_string) != 0) {
		return FALSE;
	}

	return TRUE;
}

MateVFSResult
mate_vfs_move_uri_cancellable (MateVFSURI *old,
				MateVFSURI *new,
				gboolean force_replace,
				MateVFSContext *context)
{
	g_return_val_if_fail (old != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (new != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);

	if (mate_vfs_context_check_cancellation (context))
		return MATE_VFS_ERROR_CANCELLED;

	if (! check_same_fs_in_uri (old, new))
		return MATE_VFS_ERROR_NOT_SAME_FILE_SYSTEM;

	if (mate_vfs_uri_equal (old, new)) {
		return MATE_VFS_OK;
	}

	if (!VFS_METHOD_HAS_FUNC(old->method, move))
		return MATE_VFS_ERROR_NOT_SUPPORTED;

	return old->method->move (old->method, old, new, force_replace, context);
}

MateVFSResult
mate_vfs_check_same_fs_uris_cancellable (MateVFSURI *a,
					  MateVFSURI *b,
					  gboolean *same_fs_return,
					  MateVFSContext *context)
{
	g_return_val_if_fail (a != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (b != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (same_fs_return != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);

	if (mate_vfs_context_check_cancellation (context))
		return MATE_VFS_ERROR_CANCELLED;

	if (! check_same_fs_in_uri (a, b)) {
		*same_fs_return = FALSE;
		return MATE_VFS_OK;
	}

	if (!VFS_METHOD_HAS_FUNC(a->method, check_same_fs)) {
		*same_fs_return = FALSE;
		return MATE_VFS_OK;
	}

	return a->method->check_same_fs (a->method, a, b, same_fs_return, context);
}

MateVFSResult
mate_vfs_set_file_info_cancellable (MateVFSURI *a,
				     const MateVFSFileInfo *info,
				     MateVFSSetFileInfoMask mask,
				     MateVFSContext *context)
{
	g_return_val_if_fail (a != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (info != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);

	if (mate_vfs_context_check_cancellation (context))
		return MATE_VFS_ERROR_CANCELLED;

	if (!VFS_METHOD_HAS_FUNC(a->method, set_file_info))
		return MATE_VFS_ERROR_NOT_SUPPORTED;
		
	if (mask & MATE_VFS_SET_FILE_INFO_NAME) {
		if (strchr (info->name, '/') != NULL
#ifdef G_OS_WIN32
		    || strchr (info->name, '\\') != NULL
#endif
		    ) {
			return MATE_VFS_ERROR_BAD_PARAMETERS;
		}
	}

	return a->method->set_file_info (a->method, a, info, mask, context);
}

MateVFSResult
mate_vfs_file_control_cancellable (MateVFSHandle *handle,
				    const char *operation,
				    gpointer operation_data,
				    MateVFSContext *context)
{
	g_return_val_if_fail (handle != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (operation != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);

	if (mate_vfs_context_check_cancellation (context))
		return MATE_VFS_ERROR_CANCELLED;

	return _mate_vfs_handle_do_file_control (handle, operation, operation_data, context);
}

