/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* mate-vfs-async-ops.h - Asynchronous operations in the MATE Virtual File
   System.

   Copyright (C) 1999 Free Software Foundation

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

   Author: Ettore Perazzoli <ettore@comm2000.it> */

#ifndef MATE_VFS_ASYNC_OPS_H
#define MATE_VFS_ASYNC_OPS_H

#include <glib.h>
#include <libmatevfs/mate-vfs-file-info.h>
#include <libmatevfs/mate-vfs-find-directory.h>
#include <libmatevfs/mate-vfs-handle.h>
#include <libmatevfs/mate-vfs-xfer.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * MATE_VFS_PRIORITY_MIN:
 *
 * The minimuum priority a job can have.
 **/
/**
 * MATE_VFS_PRIORITY_MAX:
 *
 * The maximuum priority a job can have.
 **/
/**
 * MATE_VFS_PRIORITY_DEFAULT:
 *
 * The default job priority. Its best to use this
 * unless you have a reason to do otherwise.
 **/

#define MATE_VFS_PRIORITY_MIN     -10
#define MATE_VFS_PRIORITY_MAX     10
#define MATE_VFS_PRIORITY_DEFAULT 0

typedef struct MateVFSAsyncHandle MateVFSAsyncHandle;

/**
 * MateVFSAsyncCallback:
 * @handle: handle of the operation generating the callback
 * @result: %MATE_VFS_OK if the operation was successful, otherwise
 * an error code.
 * @callback_data: user data defined when the callback was established
 *
 * Basic callback from an async operation that passes no data back,
 * informing the user of the @result of the operation.
 **/
typedef void	(* MateVFSAsyncCallback)	(MateVFSAsyncHandle *handle,
						 MateVFSResult result,
						 gpointer callback_data);

/**
 * MateVFSAsyncOpenCallback:
 * @handle: handle of the operation generating the callback
 * @result: %MATE_VFS_OK if the operation was successful, otherwise
 * an error code.
 * @callback_data: user data defined when the callback was established
 *
 * Callback for the mate_vfs_async_open() function,
 * informing the user of the @result of the operation.
 **/
typedef void	(* MateVFSAsyncOpenCallback)	(MateVFSAsyncHandle *handle,
						 MateVFSResult result,
						 gpointer callback_data);

/**
 * MateVFSAsyncCreateCallback:
 * @handle: handle of the operation generating the callback
 * @result: %MATE_VFS_OK if the operation was successful, otherwise
 * an error code.
 * @callback_data: user data defined when the callback was established
 *
 * Callback for the mate_vfs_async_create() function,
 * informing the user of the @result of the operation.
 **/
typedef void	(* MateVFSAsyncCreateCallback)	(MateVFSAsyncHandle *handle,
						 MateVFSResult result,
						 gpointer callback_data);

/**
 * MateVFSAsyncCloseCallback:
 * @handle: handle of the operation generating the callback
 * @result: %MATE_VFS_OK if the operation was successful, otherwise
 * an error code.
 * @callback_data: user data defined when the callback was established
 *
 * Callback for the mate_vfs_async_close() function,
 * informing the user of the @result of the operation.
 **/
typedef void	(* MateVFSAsyncCloseCallback)	(MateVFSAsyncHandle *handle,
						 MateVFSResult result,
						 gpointer callback_data);

#ifndef MATE_VFS_DISABLE_DEPRECATED
/**
 * MateVFSAsyncOpenAsChannelCallback:
 * @handle: handle of the operation generating the callback.
 * @channel: a #GIOChannel corresponding to the file opened
 * @result: %MATE_VFS_OK if the operation was successful, otherwise
 * an error code.
 * @callback_data: user data defined when the callback was established.
 *
 * Callback for the mate_vfs_async_open_as_channel() function,
 * informing the user of the @result of the operation.
 **/
typedef void	(* MateVFSAsyncOpenAsChannelCallback) (MateVFSAsyncHandle *handle,
							GIOChannel *channel,
							MateVFSResult result,
							gpointer callback_data);
#endif

#ifndef MATE_VFS_DISABLE_DEPRECATED
/**
 * MateVFSAsyncCreateAsChannelCallback:
 * @handle: handle of the operation generating the callback.
 * @channel: a #GIOChannel corresponding to the file created
 * @result: %MATE_VFS_OK if the operation was successful, otherwise
 * an error code.
 * @callback_data: user data defined when the callback was established.
 *
 * Callback for the mate_vfs_async_create_as_channel() function,
 * informing the user of the @result of the operation.
 **/
typedef void	(* MateVFSAsyncCreateAsChannelCallback) (MateVFSAsyncHandle *handle,
							  GIOChannel *channel,
							  MateVFSResult result,
							  gpointer callback_data);
#endif

/**
 * MateVFSAsyncReadCallback:
 * @handle: handle of the operation generating the callback
 * @result: %MATE_VFS_OK if the operation was successful, otherwise
 * an error code.
 * @buffer: buffer containing data read from @handle.
 * @bytes_requested: the number of bytes asked for in the call to
 * mate_vfs_async_read().
 * @bytes_read: the number of bytes actually read from @handle into @buffer.
 * @callback_data: user data defined when the callback was established
 *
 * Callback for the mate_vfs_async_read() function,
 * informing the user of the @result of the operation.
 **/
typedef void	(* MateVFSAsyncReadCallback)	(MateVFSAsyncHandle *handle,
						 MateVFSResult result,
						 gpointer buffer,
						 MateVFSFileSize bytes_requested,
						 MateVFSFileSize bytes_read,
						 gpointer callback_data);

/**
 * MateVFSAsyncWriteCallback:
 * @handle: handle of the operation generating the callback
 * @result: %MATE_VFS_OK if the operation was successful, otherwise
 * an error code.
 * @buffer: buffer containing data written to @handle.
 * @bytes_requested: the number of bytes asked to write in the call to
 * mate_vfs_async_write().
 * @bytes_written: the number of bytes actually written to @handle from @buffer.
 * @callback_data: user data defined when the callback was established.
 *
 * Callback for the mate_vfs_async_write() function,
 * informing the user of the @result of the operation.
 **/
typedef void	(* MateVFSAsyncWriteCallback)	(MateVFSAsyncHandle *handle,
						 MateVFSResult result,
						 gconstpointer buffer,
						 MateVFSFileSize bytes_requested,
						 MateVFSFileSize bytes_written,
						 gpointer callback_data);


/**
 * MateVFSAsyncSeekCallback:
 * @handle: handle of the operation generating the callback
 * @result: %MATE_VFS_OK if the operation was successful, otherwise
 * an error code.
 * @callback_data: user data defined when the callback was established.
 *
 * Callback for the mate_vfs_async_seek() function,
 * informing the user of the @result of the operation.
 **/
typedef void	(* MateVFSAsyncSeekCallback)	(MateVFSAsyncHandle *handle,
						 MateVFSResult result,
						 gpointer callback_data);


/**
 * MateVFSAsyncGetFileInfoCallback:
 * @handle: handle of the operation generating the callback
 * @results: #GList of #MateVFSFileInfoResult * items representing
 * the success of each mate_vfs_get_file_info() and the data retrieved.
 * @callback_data: user data defined when the callback was established.
 *
 * Callback for the mate_vfs_async_get_file_info() function,
 * informing the user of the @results of the operation.
 **/
typedef void    (* MateVFSAsyncGetFileInfoCallback) (MateVFSAsyncHandle *handle,
						      GList *results,
						      gpointer callback_data);

/**
 * MateVFSAsyncSetFileInfoCallback:
 * @handle: handle of the operation generating the callback
 * @result: %MATE_VFS_OK if the operation was successful, otherwise a
 * #MateVFSResult error code.
 * @file_info: if @result is %MATE_VFS_OK, a #MateVFSFileInfo struct containing
 * information about the file.
 * @callback_data: user data defined when the callback was established
 *
 * Callback for the mate_vfs_async_set_file_info() function,
 * informing the user of the @result of the operation and
 * returning the new @file_info.
 *
 * <note>
 *  <para>
 *   Setting the file info sometimes changes more information than the
 *   caller specified; for example, if the name changes the MIME type might
 *   change, and if the owner changes the SUID & SGID bits might change.
 *   Therefore the callback returns the new @file_info for the caller's
 *   convenience. The MateVFSFileInfoOptions passed here are those used
 *   for the returned file info; they are not used when setting.
 *  </para>
 * </note>
 **/
typedef void	(* MateVFSAsyncSetFileInfoCallback) (MateVFSAsyncHandle *handle,
						      MateVFSResult result,
						      MateVFSFileInfo *file_info,
						      gpointer callback_data);


/**
 * MateVFSAsyncDirectoryLoadCallback:
 * @handle: handle of the operation generating the callback.
 * @result: %MATE_VFS_OK if the operation was sucessful,
 * %MATE_VFS_ERROR_EOF if the last file in the directory
 * has been read, otherwise a #MateVFSResult error code
 * @list: a #GList of #MateVFSFileInfo structs representing
 * information about the files just loaded.
 * @entries_read: number of entries read from @handle for this instance of
 * the callback.
 * @callback_data: user data defined when the callback was established.
 *
 * Callback for the mate_vfs_async_directory_load() function.
 * informing the user of the @result of the operation and
 * providing a file @list if the @result is %MATE_VFS_OK.
 **/
typedef void	(* MateVFSAsyncDirectoryLoadCallback) (MateVFSAsyncHandle *handle,
							MateVFSResult result,
							GList *list,
							guint entries_read,
							gpointer callback_data);

/**
 * MateVFSAsyncXferProgressCallback:
 * @handle: Handle of the Xfer operation generating the callback.
 * @info: Information on the current progress in the transfer.
 * @user_data: The user data that was passed to mate_vfs_async_xfer().
 *
 * This callback is passed to mate_vfs_async_xfer() and should
 * be used for user interaction. That said, it serves two purposes:
 * Informing the user about the progress of the operation, and
 * making decisions.
 *
 * On the one hand, when the transfer progresses normally,
 * i.e. when the @info's status is %MATE_VFS_XFER_PROGRESS_STATUS_OK
 * it is called periodically whenever new progress information
 * is available, and it wasn't called already within the last
 * 100 milliseconds.
 *
 * On the other hand, it is called whenever a decision is
 * requested from the user, i.e. whenever the @info's %status
 * is not %MATE_VFS_XFER_PROGRESS_STATUS_OK.
 *
 * Either way, it acts like #MateVFSXferProgressCallback
 * would act in non-asynchronous mode. The differences in
 * invocation are explained in the mate_vfs_async_xfer()
 * documentation.
 *
 * Returns: %gint depending on the #MateVFSXferProgressInfo.
 * 	    Please consult #MateVFSXferProgressCallback for details.
 *
 **/
typedef gint    (* MateVFSAsyncXferProgressCallback) (MateVFSAsyncHandle *handle,
						       MateVFSXferProgressInfo *info,
						       gpointer user_data);

/**
 * MateVFSFindDirectoryResult:
 * @uri: The #MateVFSURI that was found.
 * @result: The #MateVFSResult that was obtained when finding @uri.
 *
 * This structure is passed to a #MateVFSAsyncFindDirectoryCallback
 * by mate_vfs_async_find_directory() and contains the information
 * associated with a single #MateVFSURI matching the specified
 * find request.
 **/
struct _MateVFSFindDirectoryResult {
	/*< public >*/
	MateVFSURI *uri;
	MateVFSResult result;

	/*< private >*/
	/* Reserved to avoid future breaks in ABI compatibility */
	void *reserved1;
	void *reserved2;
};

typedef struct _MateVFSFindDirectoryResult MateVFSFindDirectoryResult;

GType                        mate_vfs_find_directory_result_get_type (void);
MateVFSFindDirectoryResult* mate_vfs_find_directory_result_dup      (MateVFSFindDirectoryResult* result);
void                         mate_vfs_find_directory_result_free     (MateVFSFindDirectoryResult* result);

/**
 * MateVFSAsyncFindDirectoryCallback:
 * @handle: handle of the operation generating the callback
 * @results: #GList of #MateVFSFindDirectoryResult *s containing
 * special directories matching the find criteria.
 * @data: user data defined when the operation was established
 *
 * Callback for the mate_vfs_async_find_directory() function,
 * informing the user of the @results of the operation.
 **/
typedef void    (* MateVFSAsyncFindDirectoryCallback) (MateVFSAsyncHandle *handle,
							GList *results,
							gpointer data);

/**
 * MateVFSAsyncFileControlCallback:
 * @handle: handle of the operation generating the callback
 * @result: %MATE_VFS_OK if the operation was successful, otherwise a
 * #MateVFSResult error code.
 * @operation_data: The data requested from the module if @result
 * is %MATE_VFS_OK.
 * @callback_data: User data defined when the operation was established.
 *
 * Callback for the mate_vfs_async_find_directory() function.
 * informing the user of the @result of the operation, and
 * providing the requested @operation_data.
 **/
typedef void	(* MateVFSAsyncFileControlCallback)	(MateVFSAsyncHandle *handle,
							 MateVFSResult result,
							 gpointer operation_data,
							 gpointer callback_data);

void           mate_vfs_async_cancel                 (MateVFSAsyncHandle                   *handle);

void           mate_vfs_async_open                   (MateVFSAsyncHandle                  **handle_return,
						       const gchar                           *text_uri,
						       MateVFSOpenMode                       open_mode,
						       int				      priority,
						       MateVFSAsyncOpenCallback              callback,
						       gpointer                               callback_data);
void           mate_vfs_async_open_uri               (MateVFSAsyncHandle                  **handle_return,
						       MateVFSURI                           *uri,
						       MateVFSOpenMode                       open_mode,
						       int				      priority,
						       MateVFSAsyncOpenCallback              callback,
						       gpointer                               callback_data);

#ifndef MATE_VFS_DISABLE_DEPRECATED
void           mate_vfs_async_open_as_channel        (MateVFSAsyncHandle                  **handle_return,
						       const gchar                           *text_uri,
						       MateVFSOpenMode                       open_mode,
						       guint                                  advised_block_size,
						       int				      priority,
						       MateVFSAsyncOpenAsChannelCallback     callback,
						       gpointer                               callback_data);
void           mate_vfs_async_open_uri_as_channel    (MateVFSAsyncHandle                  **handle_return,
						       MateVFSURI                           *uri,
						       MateVFSOpenMode                       open_mode,
						       guint                                  advised_block_size,
						       int				      priority,
						       MateVFSAsyncOpenAsChannelCallback     callback,
						       gpointer                               callback_data);
#endif /* MATE_VFS_DISABLE_DEPRECATED */

void           mate_vfs_async_create                 (MateVFSAsyncHandle                  **handle_return,
						       const gchar                           *text_uri,
						       MateVFSOpenMode                       open_mode,
						       gboolean                               exclusive,
						       guint                                  perm,
						       int				      priority,
						       MateVFSAsyncOpenCallback              callback,
						       gpointer                               callback_data);
void           mate_vfs_async_create_uri             (MateVFSAsyncHandle                  **handle_return,
						       MateVFSURI                           *uri,
						       MateVFSOpenMode                       open_mode,
						       gboolean                               exclusive,
						       guint                                  perm,
						       int				      priority,
						       MateVFSAsyncOpenCallback              callback,
						       gpointer                               callback_data);
void           mate_vfs_async_create_symbolic_link   (MateVFSAsyncHandle                  **handle_return,
						       MateVFSURI                           *uri,
						       const gchar                           *uri_reference,
						       int				      priority,
						       MateVFSAsyncOpenCallback              callback,
						       gpointer                               callback_data);
#ifndef MATE_VFS_DISABLE_DEPRECATED
void           mate_vfs_async_create_as_channel      (MateVFSAsyncHandle                  **handle_return,
						       const gchar                           *text_uri,
						       MateVFSOpenMode                       open_mode,
						       gboolean                               exclusive,
						       guint                                  perm,
						       int				      priority,
						       MateVFSAsyncCreateAsChannelCallback   callback,
						       gpointer                               callback_data);
void           mate_vfs_async_create_uri_as_channel  (MateVFSAsyncHandle                  **handle_return,
						       MateVFSURI                           *uri,
						       MateVFSOpenMode                       open_mode,
						       gboolean                               exclusive,
						       guint                                  perm,
						       int				      priority,
						       MateVFSAsyncCreateAsChannelCallback   callback,
						       gpointer                               callback_data);
#endif /* MATE_VFS_DISABLE_DEPRECATED */

void           mate_vfs_async_close                  (MateVFSAsyncHandle                   *handle,
						       MateVFSAsyncCloseCallback             callback,
						       gpointer                               callback_data);
void           mate_vfs_async_read                   (MateVFSAsyncHandle                   *handle,
						       gpointer                               buffer,
						       guint                                  bytes,
						       MateVFSAsyncReadCallback              callback,
						       gpointer                               callback_data);
void           mate_vfs_async_write                  (MateVFSAsyncHandle                   *handle,
						       gconstpointer                          buffer,
						       guint                                  bytes,
						       MateVFSAsyncWriteCallback             callback,
						       gpointer                               callback_data);
void           mate_vfs_async_seek                   (MateVFSAsyncHandle                   *handle,
						       MateVFSSeekPosition                   whence,
						       MateVFSFileOffset                     offset,
						       MateVFSAsyncSeekCallback              callback,
						       gpointer                               callback_data);
void           mate_vfs_async_get_file_info          (MateVFSAsyncHandle                  **handle_return,
						       GList                                 *uri_list,
						       MateVFSFileInfoOptions                options,
						       int				      priority,
						       MateVFSAsyncGetFileInfoCallback       callback,
						       gpointer                               callback_data);
void           mate_vfs_async_set_file_info          (MateVFSAsyncHandle                  **handle_return,
						       MateVFSURI                           *uri,
						       MateVFSFileInfo                      *info,
						       MateVFSSetFileInfoMask                mask,
						       MateVFSFileInfoOptions                options,
						       int				      priority,
						       MateVFSAsyncSetFileInfoCallback       callback,
						       gpointer                               callback_data);
void           mate_vfs_async_load_directory         (MateVFSAsyncHandle                  **handle_return,
						       const gchar                           *text_uri,
						       MateVFSFileInfoOptions                options,
						       guint                                  items_per_notification,
						       int				      priority,
						       MateVFSAsyncDirectoryLoadCallback     callback,
						       gpointer                               callback_data);
void           mate_vfs_async_load_directory_uri     (MateVFSAsyncHandle                  **handle_return,
						       MateVFSURI                           *uri,
						       MateVFSFileInfoOptions                options,
						       guint                                  items_per_notification,
						       int				      priority,
						       MateVFSAsyncDirectoryLoadCallback     callback,
						       gpointer                               callback_data);
MateVFSResult mate_vfs_async_xfer                   (MateVFSAsyncHandle                  **handle_return,
						       GList                                 *source_uri_list,
						       GList                                 *target_uri_list,
						       MateVFSXferOptions                    xfer_options,
						       MateVFSXferErrorMode                  error_mode,
						       MateVFSXferOverwriteMode              overwrite_mode,
						       int				      priority,
						       MateVFSAsyncXferProgressCallback      progress_update_callback,
						       gpointer                               update_callback_data,
						       MateVFSXferProgressCallback           progress_sync_callback,
						       gpointer                               sync_callback_data);
void           mate_vfs_async_find_directory         (MateVFSAsyncHandle                  **handle_return,
						       GList                                 *near_uri_list,
						       MateVFSFindDirectoryKind              kind,
						       gboolean                               create_if_needed,
						       gboolean                               find_if_needed,
						       guint                                  permissions,
						       int				      priority,
						       MateVFSAsyncFindDirectoryCallback     callback,
						       gpointer                               user_data);

void           mate_vfs_async_file_control           (MateVFSAsyncHandle                   *handle,
						       const char                            *operation,
						       gpointer                               operation_data,
						       GDestroyNotify                         operation_data_destroy_func,
						       MateVFSAsyncFileControlCallback       callback,
						       gpointer                               callback_data);

#ifdef __cplusplus
}
#endif

#endif /* MATE_VFS_ASYNC_OPS_H */
