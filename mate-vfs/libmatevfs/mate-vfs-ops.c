/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-ops.c - Synchronous operations for the MATE Virtual File
   System.

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

#include <config.h>
#include "mate-vfs-ops.h"
#include "mate-vfs-monitor-private.h"
#include "mate-vfs-cancellable-ops.h"
#include "mate-vfs-handle-private.h"
#include "mate-vfs-private-utils.h"
#include <glib.h>

/**
 * mate_vfs_open:
 * @handle: pointer to a pointer to a #MateVFSHandle object.
 * @text_uri: string representing the uri to open.
 * @open_mode: open mode.
 * 
 * Open @text_uri according to mode @open_mode.  On return, @handle will then
 * contain a pointer to a handle for the open file.
 * 
 * Return value: an integer representing the result of the operation.
 */
MateVFSResult
mate_vfs_open (MateVFSHandle **handle,
		const gchar *text_uri,
		MateVFSOpenMode open_mode)
{
	MateVFSURI *uri;
	MateVFSResult result;

	g_return_val_if_fail (handle != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);
	*handle = NULL;
	g_return_val_if_fail (text_uri != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);

	uri = mate_vfs_uri_new (text_uri);
	if (uri == NULL)
		return MATE_VFS_ERROR_INVALID_URI;

	result = mate_vfs_open_uri (handle, uri, open_mode);

	mate_vfs_uri_unref (uri);

	return result;
}

/**
 * mate_vfs_open_uri:
 * @handle: pointer to a pointer to a #MateVFSHandle object.
 * @uri: uri to open.
 * @open_mode: open mode.
 * 
 * Open @uri according to mode @open_mode.  On return, @handle will then
 * contain a pointer to a handle for the open file.
 * 
 * Return value: an integer representing the result of the operation.
 */
MateVFSResult
mate_vfs_open_uri (MateVFSHandle **handle,
		    MateVFSURI *uri,
		    MateVFSOpenMode open_mode)
{
	return mate_vfs_open_uri_cancellable (handle, uri, open_mode, NULL);
}

/**
 * mate_vfs_create:
 * @handle: pointer to a pointer to a #MateVFSHandle object.
 * @text_uri: string representing the uri to create.
 * @open_mode: mode to leave the file opened in after creation (or %MATE_VFS_OPEN_MODE_NONE
 * to leave the file closed after creation).
 * @exclusive: whether the file should be created in "exclusive" mode.
 * i.e. if this flag is nonzero, operation will fail if a file with the
 * same name already exists.
 * @perm: bitmap representing the permissions for the newly created file
 * (Unix style).
 * 
 * Create @text_uri according to mode @open_mode.  On return, @handle will then
 * contain a pointer to a handle for the open file.
 * 
 * Return value: an integer representing the result of the operation.
 */
MateVFSResult
mate_vfs_create (MateVFSHandle **handle,
		  const gchar *text_uri,
		  MateVFSOpenMode open_mode,
		  gboolean exclusive,
		  guint perm)
{
	MateVFSURI *uri;
	MateVFSResult result;

	g_return_val_if_fail (handle != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);
	*handle = NULL;
	g_return_val_if_fail (text_uri != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);

	uri = mate_vfs_uri_new (text_uri);
	if (uri == NULL)
		return MATE_VFS_ERROR_INVALID_URI;

	result = mate_vfs_create_uri (handle, uri, open_mode, exclusive, perm);

	mate_vfs_uri_unref (uri);

	return result;
}

/**
 * mate_vfs_forget_cache:
 * @handle: handle of the file to affect.
 * @offset: start point of the region to be freed.
 * @size: length of the region to be freed (or until the end of the
 * file if 0 is specified).
 * 
 * With this call you can announce to mate-vfs that you will no longer
 * use the region of data starting at @offset with the size of @size. Any
 * cached data for this region might then be freed.
 * 
 * This might be useful if you stream large files, for example.
 * 
 * Return value: an integer representing the result of the operation.
 * 
 * Since: 2.12
 */

MateVFSResult
mate_vfs_forget_cache (MateVFSHandle *handle,
			MateVFSFileOffset offset,
			MateVFSFileSize size)
{
	g_return_val_if_fail (handle != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);

	return _mate_vfs_handle_forget_cache (handle, offset, size);
}


/**
 * mate_vfs_create_uri:
 * @handle: pointer to a pointer to a #MateVFSHandle object.
 * @uri: uri for the file to create.
 * @open_mode: open mode.
 * @exclusive: whether the file should be created in "exclusive" mode.
 * i.e. if this flag is nonzero, operation will fail if a file with the
 * same name already exists.
 * @perm: bitmap representing the permissions for the newly created file
 * (Unix style).
 * 
 * Create @uri according to mode @open_mode.  On return, @handle will then
 * contain a pointer to a handle for the open file.
 * 
 * Return value: an integer representing the result of the operation.
 */
MateVFSResult
mate_vfs_create_uri (MateVFSHandle **handle,
		      MateVFSURI *uri,
		      MateVFSOpenMode open_mode,
		      gboolean exclusive,
		      guint perm)
{
	return mate_vfs_create_uri_cancellable (handle, uri, open_mode,
						 exclusive, perm, NULL);
}

/**
 * mate_vfs_close:
 * @handle: pointer to a #MateVFSHandle object.
 * 
 * Close file associated with @handle.
 * 
 * Return value: an integer representing the result of the operation.
 */
MateVFSResult
mate_vfs_close (MateVFSHandle *handle)
{
	return mate_vfs_close_cancellable (handle, NULL);
}

/**
 * mate_vfs_read:
 * @handle: handle of the file to read data from.
 * @buffer: pointer to a buffer that must be at least @bytes bytes large.
 * @bytes: number of bytes to read.
 * @bytes_read: pointer to a variable that will hold the number of bytes
 * effectively read on return.
 * 
 * Read @bytes from @handle.  As with Unix system calls, the number of
 * bytes read can effectively be less than @bytes on return and will be
 * stored in @bytes_read.
 * 
 * Return value: an integer representing the result of the operation.
 */
MateVFSResult
mate_vfs_read (MateVFSHandle *handle,
		gpointer buffer,
		MateVFSFileSize bytes,
		MateVFSFileSize *bytes_read)
{
	return mate_vfs_read_cancellable (handle, buffer, bytes, bytes_read,
					   NULL);
}

/**
 * mate_vfs_write:
 * @handle: handle of the file to write data to.
 * @buffer: pointer to the buffer containing the data to be written.
 * @bytes: number of bytes to write.
 * @bytes_written: pointer to a variable that will hold the number of bytes
 * effectively written on return.
 * 
 * Write @bytes into the file opened through @handle.  As with Unix system
 * calls, the number of bytes written can effectively be less than @bytes on
 * return and will be stored in @bytes_written.
 * 
 * Return value: an integer representing the result of the operation.
 */
MateVFSResult
mate_vfs_write (MateVFSHandle *handle,
		 gconstpointer buffer,
		 MateVFSFileSize bytes,
		 MateVFSFileSize *bytes_written)
{
	return mate_vfs_write_cancellable (handle, buffer, bytes,
					    bytes_written, NULL);
}

/**
 * mate_vfs_seek:
 * @handle: handle for which the current position must be changed.
 * @whence: integer value representing the starting position.
 * @offset: number of bytes to skip from the position specified by @whence.
 * (a positive value means to move forward; a negative one to move backwards).
 * 
 * Set the current position for reading/writing through @handle.
 * 
 * Return value: an integer representing the result of the operation.
 */
MateVFSResult
mate_vfs_seek (MateVFSHandle *handle,
		MateVFSSeekPosition whence,
		MateVFSFileOffset offset)
{
	return mate_vfs_seek_cancellable (handle, whence, offset, NULL);
}

/**
 * mate_vfs_tell:
 * @handle: handle for which the current position must be retrieved.
 * @offset_return: pointer to a variable that will contain the current position
 * on return.
 * 
 * Return the current position on @handle. This is the point in the file
 * pointed to by handle that reads and writes will occur on.
 * 
 * Return value: an integer representing the result of the operation.
 */
MateVFSResult
mate_vfs_tell (MateVFSHandle *handle,
		MateVFSFileSize *offset_return)
{
	g_return_val_if_fail (handle != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);

	return _mate_vfs_handle_do_tell (handle, offset_return);
}

/**
 * mate_vfs_get_file_info:
 * @text_uri: uri of the file for which information will be retrieved.
 * @info: pointer to a #MateVFSFileInfo object that will hold the information
 * for the file on return.
 * @options: options for retrieving file information.
 * 
 * Retrieve information about @text_uri.  The information will be stored in
 * @info.
 * 
 * Return value: an integer representing the result of the operation.
 */
MateVFSResult
mate_vfs_get_file_info (const gchar *text_uri,
			 MateVFSFileInfo *info,
			 MateVFSFileInfoOptions options)
{
	MateVFSURI *uri;
	MateVFSResult result;

	uri = mate_vfs_uri_new (text_uri);

	if (uri == NULL)
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	
	result = mate_vfs_get_file_info_uri(uri, info, options);
	mate_vfs_uri_unref (uri);

	return result;
}

/**
 * mate_vfs_get_file_info_uri:
 * @uri: uri of the file for which information will be retrieved.
 * @info: pointer to a #MateVFSFileInfo object that will hold the information
 * for the file on return.
 * @options: options for retrieving file information.
 * 
 * Retrieve information about @text_uri.  The information will be stored in
 * @info.
 * 
 * Return value: an integer representing the result of the operation.
 */
MateVFSResult
mate_vfs_get_file_info_uri (MateVFSURI *uri,
			     MateVFSFileInfo *info,
			     MateVFSFileInfoOptions options)
{
	return mate_vfs_get_file_info_uri_cancellable (uri, 
							info, 
							options,
							NULL);
}

/**
 * mate_vfs_get_file_info_from_handle:
 * @handle: handle of the file for which information must be retrieved.
 * @info: pointer to a #MateVFSFileInfo object that will hold the information
 * for the file on return.
 * @options: options for retrieving file information.
 * 
 * Retrieve information about an open file.  The information will be stored in
 * @info.
 * 
 * Return value: an integer representing the result of the operation.
 */
MateVFSResult
mate_vfs_get_file_info_from_handle (MateVFSHandle *handle,
				     MateVFSFileInfo *info,
				     MateVFSFileInfoOptions options)
{
	return mate_vfs_get_file_info_from_handle_cancellable (handle, info,
								options,
								NULL);
}

/**
 * mate_vfs_truncate:
 * @text_uri: string representing the file to be truncated.
 * @length: length of the new file at @text_uri.
 * 
 * Truncate the file at @text_uri to @length bytes.
 * 
 * Return value: an integer representing the result of the operation.
 */
MateVFSResult
mate_vfs_truncate (const char *text_uri, MateVFSFileSize length)
{
	MateVFSURI *uri;
	MateVFSResult result;

	uri = mate_vfs_uri_new (text_uri);

	if (uri == NULL)
		return MATE_VFS_ERROR_NOT_SUPPORTED;

	result = mate_vfs_truncate_uri(uri, length);
	mate_vfs_uri_unref (uri);

	return result;
}


/**
 * mate_vfs_truncate_uri:
 * @uri: uri of the file to be truncated.
 * @length: length of the new file at @uri.
 * 
 * Truncate the file at @uri to be only @length bytes. Data past @length
 * bytes will be discarded.
 * 
 * Return value: an integer representing the result of the operation.
 */
MateVFSResult
mate_vfs_truncate_uri (MateVFSURI *uri, MateVFSFileSize length)
{
	return mate_vfs_truncate_uri_cancellable(uri, length, NULL);
}

/**
 * mate_vfs_truncate_handle:
 * @handle: a handle to the file to be truncated.
 * @length: length of the new file the handle is open to.
 * 
 * Truncate the file pointed to by @handle to be only @length bytes. 
 * Data past @length bytes will be discarded.
 * 
 * Return value: an integer representing the result of the operation.
 */
MateVFSResult
mate_vfs_truncate_handle (MateVFSHandle *handle, MateVFSFileSize length)
{
	return mate_vfs_truncate_handle_cancellable(handle, length, NULL);
}

/**
 * mate_vfs_make_directory_for_uri:
 * @uri: uri of the directory to be created.
 * @perm: Unix-style permissions for the newly created directory.
 * 
 * Create a directory at @uri. Only succeeds if a file or directory
 * does not already exist at @uri.
 * 
 * Return value: an integer representing the result of the operation.
 */
MateVFSResult
mate_vfs_make_directory_for_uri (MateVFSURI *uri,
				  guint perm)
{
	return mate_vfs_make_directory_for_uri_cancellable (uri, perm, NULL);
}

/**
 * mate_vfs_make_directory:
 * @text_uri: uri of the directory to be created.
 * @perm: Unix-style permissions for the newly created directory
 * 
 * Create @text_uri as a directory.
 * 
 * Return value: an integer representing the result of the operation.
 */
MateVFSResult
mate_vfs_make_directory (const gchar *text_uri,
			  guint perm)
{
	MateVFSResult result;
	MateVFSURI *uri;

	g_return_val_if_fail (text_uri != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);

	uri = mate_vfs_uri_new (text_uri);
	if (uri == NULL)
		return MATE_VFS_ERROR_INVALID_URI;

	result = mate_vfs_make_directory_for_uri (uri, perm);

	mate_vfs_uri_unref (uri);

	return result;
}

/**
 * mate_vfs_remove_directory_from_uri:
 * @uri: uri of the directory to be removed.
 * 
 * Remove @uri. @uri must be an empty directory.
 * 
 * Return value: an integer representing the result of the operation.
 */
MateVFSResult
mate_vfs_remove_directory_from_uri (MateVFSURI *uri)
{
	return mate_vfs_remove_directory_from_uri_cancellable (uri, NULL);
}

/**
 * mate_vfs_remove_directory:
 * @text_uri: path of the directory to be removed.
 * 
 * Remove @text_uri. @text_uri must be an empty directory.
 * 
 * Return value: an integer representing the result of the operation.
 */
MateVFSResult
mate_vfs_remove_directory (const gchar *text_uri)
{
	MateVFSResult result;
	MateVFSURI *uri;

	g_return_val_if_fail (text_uri != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);

	uri = mate_vfs_uri_new (text_uri);
	if (uri == NULL)
		return MATE_VFS_ERROR_INVALID_URI;

	result = mate_vfs_remove_directory_from_uri (uri);

	mate_vfs_uri_unref (uri);

	return result;
}

/**
 * mate_vfs_unlink_from_uri:
 * @uri: uri of the file to be unlinked.
 * 
 * Unlink @uri (i.e. delete the file).
 * 
 * Return value: an integer representing the result of the operation.
 */
MateVFSResult
mate_vfs_unlink_from_uri (MateVFSURI *uri)
{
	return mate_vfs_unlink_from_uri_cancellable (uri, NULL);
}

/**
 * mate_vfs_create_symbolic_link:
 * @uri: uri to create a link at.
 * @target_reference: uri "reference" to point the link to (uri or relative path).
 *
 * Creates a symbolic link, or eventually, a uri link (as necessary) 
 * at @uri pointing to @target_reference.
 *
 * Return value: an integer representing the result of the operation.
 */
MateVFSResult
mate_vfs_create_symbolic_link (MateVFSURI *uri, const gchar *target_reference)
{
	return mate_vfs_create_symbolic_link_cancellable (uri, target_reference, NULL);
}

/**
 * mate_vfs_unlink:
 * @text_uri: uri of the file to be unlinked.
 * 
 * Unlink @text_uri (i.e. delete the file).
 * 
 * Return value: an integer representing the result of the operation.
 */
MateVFSResult
mate_vfs_unlink (const gchar *text_uri)
{
	MateVFSResult result;
	MateVFSURI *uri;

	g_return_val_if_fail (text_uri != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);

	uri = mate_vfs_uri_new (text_uri);
	if (uri == NULL)
		return MATE_VFS_ERROR_INVALID_URI;

	result = mate_vfs_unlink_from_uri (uri);

	mate_vfs_uri_unref (uri);

	return result;
}

/**
 * mate_vfs_move_uri:
 * @old_uri: source uri.
 * @new_uri: destination uri.
 * @force_replace: if %TRUE, move @old_uri to @new_uri even if there 
 * is already a file at @new_uri. If there is a file, it will be discarded.
 * 
 * Move a file from uri @old_uri to @new_uri.  This will only work if @old_uri 
 * and @new_uri are on the same file system.  Otherwise, it is necessary 
 * to use the more general mate_vfs_xfer_uri() function.
 * 
 * Return value: an integer representing the result of the operation.
 */
MateVFSResult
mate_vfs_move_uri (MateVFSURI *old_uri,
		    MateVFSURI *new_uri,
		    gboolean force_replace)
{
	return mate_vfs_move_uri_cancellable (old_uri, new_uri, 
					       force_replace, NULL);
}

/**
 * mate_vfs_move:
 * @old_text_uri: string representing the source file location.
 * @new_text_uri: string representing the destination file location.
 * @force_replace: if %TRUE, perform the operation even if it unlinks an existing
 * file at @new_text_uri.
 * 
 * Move a file from @old_text_uri to @new_text_uri.  This will only work 
 * if @old_text_uri and @new_text_uri are on the same file system.  Otherwise,
 * it is necessary to use the more general mate_vfs_xfer_uri() function.
 * 
 * Return value: an integer representing the result of the operation.
 */
MateVFSResult
mate_vfs_move (const gchar *old_text_uri,
		const gchar *new_text_uri,
		gboolean force_replace)
{
	MateVFSURI *old_uri, *new_uri;
	MateVFSResult retval;

	g_return_val_if_fail (old_text_uri != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (new_text_uri != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);

	old_uri = mate_vfs_uri_new (old_text_uri);
	if (old_uri == NULL)
		return MATE_VFS_ERROR_INVALID_URI;

	new_uri = mate_vfs_uri_new (new_text_uri);
	if (new_uri == NULL) {
		mate_vfs_uri_unref (old_uri);
		return MATE_VFS_ERROR_INVALID_URI;
	}

	retval = mate_vfs_move_uri (old_uri, new_uri, force_replace);

	mate_vfs_uri_unref (old_uri);
	mate_vfs_uri_unref (new_uri);

	return retval;
}

/**
 * mate_vfs_check_same_fs_uris:
 * @source_uri: a uri.
 * @target_uri: another uri.
 * @same_fs_return: pointer to a boolean variable which will be set to %TRUE
 * on return if @source_uri and @target_uri are on the same file system.
 * 
 * Check if @source_uri and @target_uri are on the same file system.
 * 
 * Return value: an integer representing the result of the operation.
 */
MateVFSResult
mate_vfs_check_same_fs_uris (MateVFSURI *source_uri,
			      MateVFSURI *target_uri,
			      gboolean *same_fs_return)
{
	return mate_vfs_check_same_fs_uris_cancellable (source_uri, 
							 target_uri, 
							 same_fs_return,
							 NULL);
}

/**
 * mate_vfs_check_same_fs:
 * @source: path to a file.
 * @target: path to another file.
 * @same_fs_return: pointer to a boolean variable which will be set to %TRUE
 * on return if @source and @target are on the same file system.
 *
 * Check if @source and @target are on the same file system.
 *
 * Return value: an integer representing the result of the operation.
 */
MateVFSResult
mate_vfs_check_same_fs (const gchar *source,
			 const gchar *target,
			 gboolean *same_fs_return)
{
	MateVFSURI *a_uri, *b_uri;
	MateVFSResult retval;

	g_return_val_if_fail (source != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (target != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (same_fs_return != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);

	*same_fs_return = FALSE;

	a_uri = mate_vfs_uri_new (source);
	if (a_uri == NULL)
		return MATE_VFS_ERROR_INVALID_URI;

	b_uri = mate_vfs_uri_new (target);
	if (b_uri == NULL) {
		mate_vfs_uri_unref (a_uri);
		return MATE_VFS_ERROR_INVALID_URI;
	}

	retval = mate_vfs_check_same_fs_uris (a_uri, b_uri, same_fs_return);

	mate_vfs_uri_unref (a_uri);
	mate_vfs_uri_unref (b_uri);

	return retval;
}

/**
 * mate_vfs_set_file_info_uri:
 * @uri: a uri.
 * @info: information that must be set for the file.
 * @mask: bit mask representing which fields of @info need to be set.
 *
 * Set file information for @uri; only the information for which the
 * corresponding bit in @mask is set is actually modified.
 *
 * <note>
 * @info's %valid_fields is not required to contain the
 * #MateVFSFileInfoFields corresponding to the specified 
 * #MateVFSSetFileInfoMask fields of @mask. It
 * is assumed that the @info fields referenced by @mask
 * are valid.
 * </note>
 * 
 * Return value: an integer representing the result of the operation.
 */
MateVFSResult
mate_vfs_set_file_info_uri (MateVFSURI *uri,
			     MateVFSFileInfo *info,
			     MateVFSSetFileInfoMask mask)
{
	return mate_vfs_set_file_info_cancellable (uri, info, mask, NULL);
}

/**
 * mate_vfs_set_file_info:
 * @text_uri: string representing the file location.
 * @info: information that must be set for the file.
 * @mask: bit mask representing which fields of @info need to be set.
 * 
 * Set file information for @uri; only the information for which the
 * corresponding bit in @mask is set is actually modified.
 *
 * <note>
 * @info's %valid_fields is not required to contain the
 * #MateVFSFileInfoFields corresponding to the specified 
 * #MateVFSSetFileInfoMask fields of @mask. It
 * is assumed that the @info fields referenced by @mask
 * are valid.
 * </note>
 * 
 * Return value: an integer representing the result of the operation.
 */
MateVFSResult
mate_vfs_set_file_info (const gchar *text_uri,
			 MateVFSFileInfo *info,
			 MateVFSSetFileInfoMask mask)
{
	MateVFSURI *uri;
	MateVFSResult result;

	uri = mate_vfs_uri_new (text_uri);
	if (uri == NULL)
		return MATE_VFS_ERROR_INVALID_URI;

	result = mate_vfs_set_file_info_uri (uri, info, mask);

	mate_vfs_uri_unref (uri);

	return result;
}

/**
 * mate_vfs_uri_exists:
 * @uri: a uri.
 * 
 * Check if the uri points to an existing entity.
 * 
 * Return value: %TRUE if uri exists.
 */
gboolean
mate_vfs_uri_exists (MateVFSURI *uri)
{
	MateVFSFileInfo *info;
	MateVFSResult result;

	info = mate_vfs_file_info_new ();
	result = mate_vfs_get_file_info_uri (uri, info, MATE_VFS_FILE_INFO_DEFAULT);
	mate_vfs_file_info_unref (info);

	return result == MATE_VFS_OK;
}

/**
 * mate_vfs_monitor_add:
 * @handle: after the call, @handle will be a pointer to an operation handle.
 * @text_uri: string representing the uri to monitor.
 * @monitor_type: add a directory or file monitor.
 * @callback: function to call when the monitor is tripped.
 * @user_data: data to pass to @callback.
 *
 * Watch the file or directory at @text_uri for changes (or the creation/deletion of the file)
 * and call @callback when there is a change. If a directory monitor is added, @callback is
 * notified when any file in the directory changes.
 *
 * Return value: an integer representing the result of the operation.
 */
MateVFSResult 
mate_vfs_monitor_add (MateVFSMonitorHandle **handle,
                       const gchar *text_uri,
                       MateVFSMonitorType monitor_type,
                       MateVFSMonitorCallback callback,
                       gpointer user_data)
{
	MateVFSURI *uri;
	MateVFSResult result;

	g_return_val_if_fail (handle != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);
	*handle = NULL;
	g_return_val_if_fail (text_uri != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);
	uri = mate_vfs_uri_new (text_uri);
	if (uri == NULL) {
		return MATE_VFS_ERROR_INVALID_URI;
	}

	if (!VFS_METHOD_HAS_FUNC(uri->method, monitor_add)) {
		mate_vfs_uri_unref (uri);
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

	result = _mate_vfs_monitor_do_add (uri->method, handle, uri,
						monitor_type, callback, 
						user_data);

	mate_vfs_uri_unref (uri);

	return result;
}

/**
 * mate_vfs_monitor_cancel:
 * @handle: handle of the monitor to cancel.
 *
 * Cancel the monitor pointed to be @handle.
 *
 * Return value: an integer representing the result of the operation.
 */
MateVFSResult 
mate_vfs_monitor_cancel (MateVFSMonitorHandle *handle)
{
	g_return_val_if_fail (handle != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);

	return _mate_vfs_monitor_do_cancel (handle);
}

/**
 * mate_vfs_file_control:
 * @handle: handle of the file to affect.
 * @operation: operation to execute.
 * @operation_data: data needed to execute the operation.
 *
 * Execute a backend dependent operation specified by the string @operation.
 * This is typically used for specialized vfs backends that need additional
 * operations that mate-vfs doesn't have. Compare it to the unix call ioctl().
 * The format of @operation_data depends on the operation. Operation that are
 * backend specific are normally namespaced by their module name.
 *
 * Return value: an integer representing the success of the operation.
 */
MateVFSResult
mate_vfs_file_control (MateVFSHandle *handle,
			const char *operation,
			gpointer operation_data)
{
	return mate_vfs_file_control_cancellable (handle, operation, operation_data, NULL);
}

