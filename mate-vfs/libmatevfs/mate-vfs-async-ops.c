/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-async-ops.c - Asynchronous operations supported by the
   MATE Virtual File System (version for POSIX threads).

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

   Author: Ettore Perazzoli <ettore@comm2000.it> */

#include <config.h>

#include "mate-vfs-async-ops.h"
#include "mate-vfs-async-job-map.h"
#include "mate-vfs-job.h"
#include "mate-vfs-job-queue.h"
#include "mate-vfs-job-limit.h"
#include <glib.h>
#include <unistd.h>

/**
 * mate_vfs_async_cancel:
 * @handle: handle of the async operation to be cancelled.
 *
 * Cancel an asynchronous operation and close all its callbacks.
 *
 * In a single-threaded application, its guaranteed that if you
 * call this before the operation finished callback has been called
 * the callback will never be called.
 *
 * However, in a multithreaded application, or to be more specific, if
 * you call mate_vfs_async_cancel from another thread than the thread
 * handling the glib mainloop, there is a race condition where if
 * the operation finished callback was just dispatched, you might
 * still cancel the operation. So, in this case you need to handle the
 * fact that the operation callback might still run even though another
 * thread has cancelled the operation.
 *
 * One way to avoid problems from this is to mark the data structure you're
 * using as callback_data as destroyed, and then queue an idle and do the
 * actual freeing in an idle handler. The idle handler is guaranteed to run
 * after the callback has been exectuted, so by then it is safe to destroy
 * the callback_data. The callback handler must handle the case where the
 * callback_data is marked destroyed by doing nothing.
 *
 * This is clearly not ideal for multithreaded applications, but as good as
 * we can with the current API. Eventually we'll have to change the API to
 * make this work better.
 */
void
mate_vfs_async_cancel (MateVFSAsyncHandle *handle)
{
	MateVFSJob *job;
	
	_mate_vfs_async_job_map_lock ();

	job = _mate_vfs_async_job_map_get_job (handle);
	if (job == NULL) {
		JOB_DEBUG (("job %u - job no longer exists", GPOINTER_TO_UINT (handle)));
		/* have to cancel the callbacks because they still can be pending */
		_mate_vfs_async_job_cancel_job_and_callbacks (handle, NULL);
	} else {
		/* Cancel the job in progress. OK to do outside of job->job_lock,
		 * job lifetime is protected by _mate_vfs_async_job_map_lock.
		 */
		_mate_vfs_job_module_cancel (job);
		_mate_vfs_async_job_cancel_job_and_callbacks (handle, job);
	}

	_mate_vfs_async_job_map_unlock ();
}

static MateVFSAsyncHandle *
async_open (MateVFSURI *uri,
	    MateVFSOpenMode open_mode,
	    int priority,
	    MateVFSAsyncOpenCallback callback,
	    gpointer callback_data)
{
	MateVFSJob *job;
	MateVFSOpenOp *open_op;
	MateVFSAsyncHandle *result;

	job = _mate_vfs_job_new (MATE_VFS_OP_OPEN, priority, (GFunc) callback, callback_data);
	
	open_op = &job->op->specifics.open;
	
	open_op->uri = uri == NULL ? NULL : mate_vfs_uri_ref (uri);
	open_op->open_mode = open_mode;

	result = job->job_handle;
	_mate_vfs_job_go (job);

	return result;
}

/**
 * mate_vfs_async_open_uri:
 * @handle_return: pointer to a pointer to a #MateVFSHandle object.
 * @uri: uri to open.
 * @open_mode: open mode.
 * @priority: a value from %MATE_VFS_PRIORITY_MIN to %MATE_VFS_PRIORITY_MAX (normally
 * should be %MATE_VFS_PRIORITY_DEFAULT) indicating the priority to assign to this job
 * in allocating threads from the thread pool.
 * @callback: function to be called when the operation is complete.
 * @callback_data: data to pass to @callback.
 * 
 * Open @uri according to mode @open_mode.  On return, @handle_return will
 * contain a pointer to the operation. Once the file has been successfully opened,
 * @callback will be called with the #MateVFSResult.
 */
void
mate_vfs_async_open_uri (MateVFSAsyncHandle **handle_return,
			  MateVFSURI *uri,
			  MateVFSOpenMode open_mode,
			  int priority,
			  MateVFSAsyncOpenCallback callback,
			  gpointer callback_data)
{
	g_return_if_fail (handle_return != NULL);
	g_return_if_fail (uri != NULL);
	g_return_if_fail (callback != NULL);
      	g_return_if_fail (priority >= MATE_VFS_PRIORITY_MIN);
	g_return_if_fail (priority <= MATE_VFS_PRIORITY_MAX);

	*handle_return = async_open (uri, open_mode, priority,
				     callback, callback_data);
}

/**
 * mate_vfs_async_open:
 * @handle_return: pointer to a pointer to a #MateVFSHandle object.
 * @text_uri: string of the uri to open.
 * @open_mode: open mode.
 * @priority: a value from %MATE_VFS_PRIORITY_MIN to %MATE_VFS_PRIORITY_MAX (normally
 * should be %MATE_VFS_PRIORITY_DEFAULT) indicating the priority to assign to this job
 * in allocating threads from the thread pool.
 * @callback: function to be called when the operation is complete.
 * @callback_data: data to pass to @callback.
 * 
 * Open @text_uri according to mode @open_mode.  On return, @handle_return will
 * contain a pointer to the operation. Once the file has been successfully opened,
 * @callback will be called with the #MateVFSResult.
 */
void
mate_vfs_async_open (MateVFSAsyncHandle **handle_return,
		      const gchar *text_uri,
		      MateVFSOpenMode open_mode,
		      int priority,
		      MateVFSAsyncOpenCallback callback,
		      gpointer callback_data)
{
	MateVFSURI *uri;

	g_return_if_fail (handle_return != NULL);
	g_return_if_fail (text_uri != NULL);
	g_return_if_fail (callback != NULL);
	g_return_if_fail (priority >= MATE_VFS_PRIORITY_MIN);
	g_return_if_fail (priority <= MATE_VFS_PRIORITY_MAX);

	uri = mate_vfs_uri_new (text_uri);
	*handle_return = async_open (uri, open_mode, priority,
				     callback, callback_data);
	if (uri != NULL) {
		mate_vfs_uri_unref (uri);
	}
}

static MateVFSAsyncHandle *
async_open_as_channel (MateVFSURI *uri,
		       MateVFSOpenMode open_mode,
		       guint advised_block_size,
		       int priority,
		       MateVFSAsyncOpenAsChannelCallback callback,
		       gpointer callback_data)
{
	MateVFSJob *job;
	MateVFSOpenAsChannelOp *open_as_channel_op;
	MateVFSAsyncHandle *result;

	job = _mate_vfs_job_new (MATE_VFS_OP_OPEN_AS_CHANNEL, priority, (GFunc) callback, callback_data);

	open_as_channel_op = &job->op->specifics.open_as_channel;
	open_as_channel_op->uri = uri == NULL ? NULL : mate_vfs_uri_ref (uri);
	open_as_channel_op->open_mode = open_mode;
	open_as_channel_op->advised_block_size = advised_block_size;

	result = job->job_handle;
	_mate_vfs_job_go (job);

	return result;
}

/**
 * mate_vfs_async_open_uri_as_channel:
 * @handle_return: pointer to a pointer to a #MateVFSHandle object.
 * @uri: uri to open as a #GIOChannel.
 * @open_mode: open mode i.e. for reading, writing, random, etc.
 * @advised_block_size: the preferred block size for #GIOChannel to read.
 * @priority: a value from %MATE_VFS_PRIORITY_MIN to %MATE_VFS_PRIORITY_MAX (normally
 * should be %MATE_VFS_PRIORITY_DEFAULT) indicating the priority to assign this job
 * in allocating threads from the thread pool.
 * @callback: function to be called when the operation is complete.
 * @callback_data: data to pass to @callback.
 *
 * Open @uri as a #GIOChannel. Once the channel has been established
 * @callback will be called with @callback_data, the result of the operation,
 * and if the result was %MATE_VFS_OK, a reference to a #GIOChannel pointing
 * at @uri in @open_mode.
 *
 * Deprecated: This function has been deprecated due to behaving weirdly which suggests 
 * that it shouldn't be used. See bugs #157266, #157265, #157261, #138398 in
 * the MATE Bugzilla. If the *_as_channel functions are needed they should be
 * fixed and undeprecated.
 */
void
mate_vfs_async_open_uri_as_channel (MateVFSAsyncHandle **handle_return,
				     MateVFSURI *uri,
				     MateVFSOpenMode open_mode,
				     guint advised_block_size,
				     int priority,
				     MateVFSAsyncOpenAsChannelCallback callback,
				     gpointer callback_data)
{
	g_return_if_fail (handle_return != NULL);
	g_return_if_fail (uri != NULL);
	g_return_if_fail (callback != NULL);
	g_return_if_fail (priority >= MATE_VFS_PRIORITY_MIN);
	g_return_if_fail (priority <= MATE_VFS_PRIORITY_MAX);

	*handle_return = async_open_as_channel (uri, open_mode, advised_block_size,
						priority, callback, callback_data);
}

/**
 * mate_vfs_async_open_as_channel:
 * @handle_return: pointer to a pointer to a #MateVFSHandle object.
 * @text_uri: string of the uri to open as a #GIOChannel.
 * @open_mode: open mode i.e. for reading, writing, random, etc.
 * @advised_block_size: the preferred block size for #GIOChannel to read.
 * @priority: a value from %MATE_VFS_PRIORITY_MIN to %MATE_VFS_PRIORITY_MAX (normally
 * should be %MATE_VFS_PRIORITY_DEFAULT) indicating the priority to assign this job
 * in allocating threads from the thread pool.
 * @callback: function to be called when the operation is complete.
 * @callback_data: data to pass to @callback.
 *
 * Open @text_uri as a #GIOChannel. Once the channel has been established
 * @callback will be called with @callback_data, the result of the operation,
 * and if the result was %MATE_VFS_OK, a reference to a #GIOChannel pointing
 * at @text_uri in @open_mode.
 *
 * This function has been deprecated due to behaving weirdly which suggests 
 * that it hasn't been used. See bugs #157266, #157265, #157261, #138398 in
 * the MATE Bugzilla. If the *_as_channel functions are needed they should be
 * fixed and undeprecated.
 */
void
mate_vfs_async_open_as_channel (MateVFSAsyncHandle **handle_return,
				 const gchar *text_uri,
				 MateVFSOpenMode open_mode,
				 guint advised_block_size,
				 int priority,
				 MateVFSAsyncOpenAsChannelCallback callback,
				 gpointer callback_data)
{
	MateVFSURI *uri;

	g_return_if_fail (handle_return != NULL);
	g_return_if_fail (text_uri != NULL);
	g_return_if_fail (callback != NULL);
	g_return_if_fail (priority >= MATE_VFS_PRIORITY_MIN);
	g_return_if_fail (priority <= MATE_VFS_PRIORITY_MAX);

	uri = mate_vfs_uri_new (text_uri);
	*handle_return = async_open_as_channel (uri, open_mode, advised_block_size,
						priority, callback, callback_data);
	if (uri != NULL) {
		mate_vfs_uri_unref (uri);
	}
}

static MateVFSAsyncHandle *
async_create (MateVFSURI *uri,
	      MateVFSOpenMode open_mode,
	      gboolean exclusive,
	      guint perm,
	      int priority,
	      MateVFSAsyncOpenCallback callback,
	      gpointer callback_data)
{
	MateVFSJob *job;
	MateVFSCreateOp *create_op;
	MateVFSAsyncHandle *result;

	job = _mate_vfs_job_new (MATE_VFS_OP_CREATE, priority, (GFunc) callback, callback_data);

	create_op = &job->op->specifics.create;
	create_op->uri = uri == NULL ? NULL : mate_vfs_uri_ref (uri);
	create_op->open_mode = open_mode;
	create_op->exclusive = exclusive;
	create_op->perm = perm;

	result = job->job_handle;
	_mate_vfs_job_go (job);

	return result;
}

/**
 * mate_vfs_async_create_uri:
 * @handle_return: pointer to a pointer to a #MateVFSHandle object.
 * @uri: uri to create a file at.
 * @open_mode: mode to leave the file opened in after creation (or %MATE_VFS_OPEN_MODE_NONE
 * to leave the file closed after creation).
 * @exclusive: Whether the file should be created in "exclusive" mode:
 * i.e. if this flag is nonzero, operation will fail if a file with the
 * same name already exists.
 * @perm: bitmap representing the permissions for the newly created file
 * (Unix style).
 * @priority: a value from %MATE_VFS_PRIORITY_MIN to %MATE_VFS_PRIORITY_MAX (normally
 * should be %MATE_VFS_PRIORITY_DEFAULT) indicating the priority to assign this job
 * in allocating threads from the thread pool.
 * @callback: function to be called when the operation is complete.
 * @callback_data: data to pass to @callback.
 * 
 * Create a file at @uri according to mode @open_mode, with permissions @perm (in
 * the standard UNIX packed bit permissions format). When the create has been completed
 * @callback will be called with the result code and @callback_data.
 */
void
mate_vfs_async_create_uri (MateVFSAsyncHandle **handle_return,
			    MateVFSURI *uri,
			    MateVFSOpenMode open_mode,
			    gboolean exclusive,
			    guint perm,
			    int priority,
			    MateVFSAsyncOpenCallback callback,
			    gpointer callback_data)
{
	g_return_if_fail (handle_return != NULL);
	g_return_if_fail (uri != NULL);
	g_return_if_fail (callback != NULL);
	g_return_if_fail (priority >= MATE_VFS_PRIORITY_MIN);
	g_return_if_fail (priority <= MATE_VFS_PRIORITY_MAX);

	*handle_return = async_create (uri, open_mode, exclusive, perm,
				       priority, callback, callback_data);
}

/**
 * mate_vfs_async_create:
 * @handle_return: pointer to a pointer to a #MateVFSHandle object.
 * @text_uri: string representing the uri to create.
 * @open_mode: mode to leave the file opened in after creation (or %MATE_VFS_OPEN_MODE_NONE
 * to leave the file closed after creation).
 * @exclusive: whether the file should be created in "exclusive" mode:
 * i.e. if this flag is nonzero, operation will fail if a file with the
 * same name already exists.
 * @perm: bitmap representing the permissions for the newly created file
 * (Unix style).
 * @priority: a value from %MATE_VFS_PRIORITY_MIN to %MATE_VFS_PRIORITY_MAX (normally
 * should be %MATE_VFS_PRIORITY_DEFAULT) indicating the priority to assign this job
 * in allocating threads from the thread pool.
 * @callback: function to be called when the operation is complete.
 * @callback_data: data to pass to @callback.
 * 
 * Create a file at @uri according to mode @open_mode, with permissions @perm (in
 * the standard UNIX packed bit permissions format). When the create has been completed
 * @callback will be called with the result code and @callback_data.
 */
void
mate_vfs_async_create (MateVFSAsyncHandle **handle_return,
			const gchar *text_uri,
			MateVFSOpenMode open_mode,
			gboolean exclusive,
			guint perm,
			int priority,
			MateVFSAsyncOpenCallback callback,
			gpointer callback_data)
{
	MateVFSURI *uri;

	g_return_if_fail (handle_return != NULL);
	g_return_if_fail (text_uri != NULL);
	g_return_if_fail (callback != NULL);
	g_return_if_fail (priority >= MATE_VFS_PRIORITY_MIN);
	g_return_if_fail (priority <= MATE_VFS_PRIORITY_MAX);

	uri = mate_vfs_uri_new (text_uri);
	*handle_return = async_create (uri, open_mode, exclusive, perm,
				       priority, callback, callback_data);
	if (uri != NULL) {
		mate_vfs_uri_unref (uri);
	}
}

/**
 * mate_vfs_async_create_as_channel:
 * @handle_return: pointer to a pointer to a #MateVFSHandle object.
 * @text_uri: string of the uri to open as a #GIOChannel, creating it as necessary.
 * @open_mode: open mode i.e. for reading, writing, random, etc.
 * @exclusive: replace the file if it already exists.
 * @perm: standard POSIX-style permissions bitmask, permissions of created file.
 * @priority: a value from %MATE_VFS_PRIORITY_MIN to %MATE_VFS_PRIORITY_MAX (normally
 * should be %MATE_VFS_PRIORITY_DEFAULT) indicating the priority to assign this job
 * in allocating threads from the thread pool.
 * @callback: function to be called when the operation is complete.
 * @callback_data: data to pass to @callback.
 *
 * Open @text_uri as a #GIOChannel, creating it as necessary. Once the channel has 
 * been established @callback will be called with @callback_data, the result of the 
 * operation, and if the result was %MATE_VFS_OK, a reference to a #GIOChannel pointing
 * at @text_uri in @open_mode.
 *
 * This function has been deprecated due to behaving weirdly which suggests 
 * that it hasn't been used. See bugs #157266, #157265, #157261, #138398 in
 * the MATE Bugzilla. If the *_as_channel functions are needed they should be
 * fixed and undeprecated.
 */
void
mate_vfs_async_create_as_channel (MateVFSAsyncHandle **handle_return,
				   const gchar *text_uri,
				   MateVFSOpenMode open_mode,
				   gboolean exclusive,
				   guint perm,
				   int priority,
				   MateVFSAsyncOpenAsChannelCallback callback,
				   gpointer callback_data)
{
	MateVFSURI *uri;
	g_return_if_fail (text_uri != NULL);
	
	uri = mate_vfs_uri_new (text_uri);
	mate_vfs_async_create_uri_as_channel (handle_return, uri, open_mode, 
					       exclusive, perm, priority, 
					       callback, callback_data);
	if (uri != NULL) {
		mate_vfs_uri_unref (uri);
	}
}


/**
 * mate_vfs_async_create_uri_as_channel:
 * @handle_return: pointer to a pointer to a #MateVFSHandle object.
 * @uri: uri to open as a #GIOChannel, creating it as necessary.
 * @open_mode: open mode i.e. for reading, writing, random, etc.
 * @exclusive: replace the file if it already exists.
 * @perm: standard POSIX-style permissions bitmask, permissions of created file.
 * @priority: a value from %MATE_VFS_PRIORITY_MIN to %MATE_VFS_PRIORITY_MAX (normally
 * should be %MATE_VFS_PRIORITY_DEFAULT) indicating the priority to assign this job
 * in allocating threads from the thread pool.
 * @callback: function to be called when the operation is complete.
 * @callback_data: data to pass to @callback.
 *
 * Open @uri as a #GIOChannel, creating it as necessary. Once the channel has 
 * been established @callback will be called with @callback_data, the result of the 
 * operation, and if the result was %MATE_VFS_OK, a reference to a #GIOChannel pointing
 * at @text_uri in @open_mode.
 *
 * Since: 2.12
 */
void           mate_vfs_async_create_uri_as_channel  (MateVFSAsyncHandle                  **handle_return,
						       MateVFSURI                           *uri,
						       MateVFSOpenMode                       open_mode,
						       gboolean                               exclusive,
						       guint                                  perm,
						       int				      priority,
						       MateVFSAsyncCreateAsChannelCallback   callback,
						       gpointer                               callback_data)
{
	MateVFSJob *job;
	MateVFSCreateAsChannelOp *create_as_channel_op;

	g_return_if_fail (handle_return != NULL);
	g_return_if_fail (uri != NULL);
	g_return_if_fail (callback != NULL);
	g_return_if_fail (priority >= MATE_VFS_PRIORITY_MIN);
	g_return_if_fail (priority <= MATE_VFS_PRIORITY_MAX);

	job = _mate_vfs_job_new (MATE_VFS_OP_CREATE_AS_CHANNEL, priority, (GFunc) callback, callback_data);


	create_as_channel_op = &job->op->specifics.create_as_channel;
	create_as_channel_op->uri = uri == NULL ? NULL : mate_vfs_uri_ref (uri);
	create_as_channel_op->open_mode = open_mode;
	create_as_channel_op->exclusive = exclusive;
	create_as_channel_op->perm = perm;

	*handle_return = job->job_handle;
	_mate_vfs_job_go (job);
}


/**
 * mate_vfs_async_close:
 * @handle: async handle to close.
 * @callback: function to be called when the operation is complete.
 * @callback_data: data to pass to @callback.
 *
 * Close @handle opened with mate_vfs_async_open(). When the close
 * has completed, @callback will be called with @callback_data and
 * the result of the operation.
 */
void
mate_vfs_async_close (MateVFSAsyncHandle *handle,
		       MateVFSAsyncCloseCallback callback,
		       gpointer callback_data)
{
	MateVFSJob *job;

	g_return_if_fail (handle != NULL);
	g_return_if_fail (callback != NULL);

	for (;;) {
		_mate_vfs_async_job_map_lock ();
		job = _mate_vfs_async_job_map_get_job (handle);
		if (job == NULL) {
			g_warning ("trying to read a non-existing handle");
			_mate_vfs_async_job_map_unlock ();
			return;
		}

		if (job->op->type != MATE_VFS_OP_READ &&
		    job->op->type != MATE_VFS_OP_WRITE) {
			_mate_vfs_job_set (job, MATE_VFS_OP_CLOSE,
					   (GFunc) callback, callback_data);
			_mate_vfs_job_go (job);
			_mate_vfs_async_job_map_unlock ();
			return;
		}
		/* Still reading, wait a bit, cancel should be pending.
		 * This mostly handles a race condition that can happen
		 * on a dual CPU machine where a cancel stops a read before
		 * the read thread picks up and a close then gets scheduled
		 * on a new thread. Without this the job op type would be
		 * close for both threads and two closes would get executed
		 */
		_mate_vfs_async_job_map_unlock ();
		g_usleep (100);
	}
}

/**
 * mate_vfs_async_read:
 * @handle: handle for the file to be read.
 * @buffer: allocated block of memory to read into.
 * @bytes: number of bytes to read.
 * @callback: function to be called when the operation is complete.
 * @callback_data: data to pass to @callback.
 * 
 * Read @bytes bytes from the file pointed to be @handle into @buffer.
 * When the operation is complete, @callback will be called with the
 * result of the operation and @callback_data.
 */
void
mate_vfs_async_read (MateVFSAsyncHandle *handle,
		      gpointer buffer,
		      guint bytes,
		      MateVFSAsyncReadCallback callback,
		      gpointer callback_data)
{
	MateVFSJob *job;
	MateVFSReadOp *read_op;

	g_return_if_fail (handle != NULL);
	g_return_if_fail (buffer != NULL);
	g_return_if_fail (callback != NULL);

	_mate_vfs_async_job_map_lock ();
	job = _mate_vfs_async_job_map_get_job (handle);
	if (job == NULL) {
		g_warning ("trying to read from a non-existing handle");
		_mate_vfs_async_job_map_unlock ();
		return;
	}

	_mate_vfs_job_set (job, MATE_VFS_OP_READ,
			   (GFunc) callback, callback_data);

	read_op = &job->op->specifics.read;
	read_op->buffer = buffer;
	read_op->num_bytes = bytes;

	_mate_vfs_job_go (job);
	_mate_vfs_async_job_map_unlock ();
}

/**
 * mate_vfs_async_write:
 * @handle: handle for the file to be written.
 * @buffer: block of memory containing data to be written.
 * @bytes: number of bytes to write.
 * @callback: function to be called when the operation is complete.
 * @callback_data: data to pass to @callback.
 * 
 * Write @bytes bytes from @buffer into the file pointed to be @handle.
 * When the operation is complete, @callback will be called with the
 * result of the operation and @callback_data.
 */
void
mate_vfs_async_write (MateVFSAsyncHandle *handle,
		       gconstpointer buffer,
		       guint bytes,
		       MateVFSAsyncWriteCallback callback,
		       gpointer callback_data)
{
	MateVFSJob *job;
	MateVFSWriteOp *write_op;

	g_return_if_fail (handle != NULL);
	g_return_if_fail (buffer != NULL);
	g_return_if_fail (callback != NULL);

	_mate_vfs_async_job_map_lock ();
	job = _mate_vfs_async_job_map_get_job (handle);
	if (job == NULL) {
		g_warning ("trying to write to a non-existing handle");
		_mate_vfs_async_job_map_unlock ();
		return;
	}

	_mate_vfs_job_set (job, MATE_VFS_OP_WRITE,
			   (GFunc) callback, callback_data);

	write_op = &job->op->specifics.write;
	write_op->buffer = buffer;
	write_op->num_bytes = bytes;

	_mate_vfs_job_go (job);
	_mate_vfs_async_job_map_unlock ();
}

/**
 * mate_vfs_async_seek:
 * @handle: handle for which the current position must be changed.
 * @whence: integer value representing the starting position.
 * @offset: number of bytes to skip from the position specified by @whence.
 * (a positive value means to move forward; a negative one to move backwards).
 * @callback: function to be called when the operation is complete.
 * @callback_data: data to pass to @callback.
 * 
 * Set the current position for reading/writing through @handle.
 * When the operation is complete, @callback will be called with the
 * result of the operation and @callback_data.
 */
void
mate_vfs_async_seek (MateVFSAsyncHandle *handle,
		      MateVFSSeekPosition whence,
		      MateVFSFileOffset offset,
		      MateVFSAsyncSeekCallback callback,
		      gpointer callback_data)
{
	MateVFSJob *job;
	MateVFSSeekOp *seek_op;

	g_return_if_fail (handle != NULL);
	g_return_if_fail (callback != NULL);

	_mate_vfs_async_job_map_lock ();
	job = _mate_vfs_async_job_map_get_job (handle);
	if (job == NULL) {
		g_warning ("trying to seek in a non-existing handle");
		_mate_vfs_async_job_map_unlock ();
		return;
	}

	_mate_vfs_job_set (job, MATE_VFS_OP_SEEK,
			   (GFunc) callback, callback_data);

	seek_op = &job->op->specifics.seek;
	seek_op->whence = whence;
	seek_op->offset = offset;

	_mate_vfs_job_go (job);
	_mate_vfs_async_job_map_unlock ();
}

/**
 * mate_vfs_async_create_symbolic_link:
 * @handle_return: when the function returns, will point to a handle for
 * the async operation.
 * @uri: location to create the link at.
 * @uri_reference: location to point @uri to (can be a uri fragment, i.e. relative).
 * @priority: a value from %MATE_VFS_PRIORITY_MIN to %MATE_VFS_PRIORITY_MAX (normally
 * should be %MATE_VFS_PRIORITY_DEFAULT) indicating the priority to assign to this job
 * in allocating threads from the thread pool.
 * @callback: function to be called when the operation is complete.
 * @callback_data: data to pass to @callback.
 * 
 * Create a symbolic link at @uri pointing to @uri_reference. When the operation
 * has completed @callback will be called with the result of the operation and
 * @callback_data.
 */
void
mate_vfs_async_create_symbolic_link (MateVFSAsyncHandle **handle_return,
				      MateVFSURI *uri,
				      const gchar *uri_reference,
				      int priority,
				      MateVFSAsyncOpenCallback callback,
				      gpointer callback_data)
{
	MateVFSJob *job;
	MateVFSCreateLinkOp *create_op;

	g_return_if_fail (handle_return != NULL);
	g_return_if_fail (uri != NULL);
	g_return_if_fail (callback != NULL);
	g_return_if_fail (priority >= MATE_VFS_PRIORITY_MIN);
	g_return_if_fail (priority <= MATE_VFS_PRIORITY_MAX);

	job = _mate_vfs_job_new (MATE_VFS_OP_CREATE_SYMBOLIC_LINK, priority, (GFunc) callback, callback_data);

	create_op = &job->op->specifics.create_symbolic_link;
	create_op->uri = mate_vfs_uri_ref (uri);
	create_op->uri_reference = g_strdup (uri_reference);

	*handle_return = job->job_handle;
	_mate_vfs_job_go (job);
}

/**
 * mate_vfs_async_get_file_info:
 * @handle_return: when the function returns, will point to a handle for
 * the async operation.
 * @uri_list: a #GList of #MateVFSURIs to fetch information about.
 * @options: packed boolean type providing control over various details
 * of the get_file_info operation.
 * @priority: a value from %MATE_VFS_PRIORITY_MIN to %MATE_VFS_PRIORITY_MAX (normally
 * should be %MATE_VFS_PRIORITY_DEFAULT) indicating the priority to assign this job
 * in allocating threads from the thread pool.
 * @callback: function to be called when the operation is complete.
 * @callback_data: data to pass to @callback.
 * 
 * Fetch information about the files indicated in @uri_list and return the
 * information progressively to @callback.
 */
void
mate_vfs_async_get_file_info (MateVFSAsyncHandle **handle_return,
			       GList *uri_list,
			       MateVFSFileInfoOptions options,
			       int priority,
			       MateVFSAsyncGetFileInfoCallback callback,
			       gpointer callback_data)
{
	MateVFSJob *job;
	MateVFSGetFileInfoOp *get_info_op;

	g_return_if_fail (handle_return != NULL);
	g_return_if_fail (callback != NULL);
	g_return_if_fail (priority >= MATE_VFS_PRIORITY_MIN);
	g_return_if_fail (priority <= MATE_VFS_PRIORITY_MAX);

	job = _mate_vfs_job_new (MATE_VFS_OP_GET_FILE_INFO, priority, (GFunc) callback, callback_data);

	get_info_op = &job->op->specifics.get_file_info;

	get_info_op->uris = mate_vfs_uri_list_copy (uri_list);
	get_info_op->options = options;

	*handle_return = job->job_handle;
	_mate_vfs_job_go (job);
}

/**
 * mate_vfs_async_set_file_info:
 * @handle_return: when the function returns, will point to a handle for
 * the async operation.
 * @uri: uri to set the file info of.
 * @info: the struct containing new information about the file.
 * @mask: control which fields of @info are to be changed about the file at @uri.
 * @options: packed boolean type providing control over various details
 * of the set_file_info operation.
 * @priority: a value from %MATE_VFS_PRIORITY_MIN to %MATE_VFS_PRIORITY_MAX (normally
 * should be %MATE_VFS_PRIORITY_DEFAULT) indicating the priority to assign this job
 * in allocating threads from the thread pool.
 * @callback: function to be called when the operation is complete.
 * @callback_data: data to pass to @callback.
 * 
 * Set file info details about the file at @uri, such as permissions, name,
 * owner, and modification time.
 */
void
mate_vfs_async_set_file_info (MateVFSAsyncHandle **handle_return,
			       MateVFSURI *uri,
			       MateVFSFileInfo *info,
			       MateVFSSetFileInfoMask mask,
			       MateVFSFileInfoOptions options,
			       int priority,
			       MateVFSAsyncSetFileInfoCallback callback,
			       gpointer callback_data)
{
	MateVFSJob *job;
	MateVFSSetFileInfoOp *op;

	g_return_if_fail (handle_return != NULL);
	g_return_if_fail (uri != NULL);
	g_return_if_fail (info != NULL);
	g_return_if_fail (callback != NULL);
	g_return_if_fail (priority >= MATE_VFS_PRIORITY_MIN);
	g_return_if_fail (priority <= MATE_VFS_PRIORITY_MAX);

	job = _mate_vfs_job_new (MATE_VFS_OP_SET_FILE_INFO, priority, (GFunc) callback, callback_data);

	op = &job->op->specifics.set_file_info;

	op->uri = mate_vfs_uri_ref (uri);
	op->info = mate_vfs_file_info_new ();
	mate_vfs_file_info_copy (op->info, info);
	op->mask = mask;
	op->options = options;

	*handle_return = job->job_handle;
	_mate_vfs_job_go (job);
}

/**
 * mate_vfs_async_find_directory:
 * @handle_return: when the function returns, will point to a handle for the operation.
 * @near_uri_list: a #GList of #MateVFSURIs, find a special directory on the same 
 * volume as @near_uri_list.
 * @kind: kind of special directory.
 * @create_if_needed: if directory we are looking for does not exist, try to create it.
 * @find_if_needed: if we don't know where the directory is yet, look for it.
 * @permissions: if creating, use these permissions.
 * @priority: a value from %MATE_VFS_PRIORITY_MIN to %MATE_VFS_PRIORITY_MAX (normally
 * should be %MATE_VFS_PRIORITY_DEFAULT) indicating the priority to assign to this job
 * in allocating threads from the thread pool.
 * @callback: function to be called when the operation is complete.
 * @user_data: data to pass to @callback. 
 *
 * Used to return special directories such as Trash and Desktop from different
 * file systems.
 * 
 * There is quite a complicated logic behind finding/creating a Trash directory
 * and you need to be aware of some implications:
 * Finding the Trash the first time when using the file method may be pretty 
 * expensive. A cache file is used to store the location of that Trash file
 * for next time.
 * If @create_if_needed is specified without @find_if_needed, you may end up
 * creating a Trash file when there already is one. Your app should start out
 * by doing a mate_vfs_find_directory with the @find_if_needed to avoid this
 * and then use the @create_if_needed flag to create Trash lazily when it is
 * needed for throwing away an item on a given disk.
 * 
 * When the operation has completed, @callback will be called with the result
 * of the operation and @user_data.
 */
void
mate_vfs_async_find_directory (MateVFSAsyncHandle **handle_return,
				GList *near_uri_list,
				MateVFSFindDirectoryKind kind,
				gboolean create_if_needed,
				gboolean find_if_needed,
				guint permissions,
				int priority,
				MateVFSAsyncFindDirectoryCallback callback,
				gpointer user_data)
{
	MateVFSJob *job;
	MateVFSFindDirectoryOp *get_info_op;

	g_return_if_fail (handle_return != NULL);
	g_return_if_fail (callback != NULL);
	g_return_if_fail (priority >= MATE_VFS_PRIORITY_MIN);
	g_return_if_fail (priority <= MATE_VFS_PRIORITY_MAX);

	job = _mate_vfs_job_new (MATE_VFS_OP_FIND_DIRECTORY, priority, (GFunc) callback, user_data);

	get_info_op = &job->op->specifics.find_directory;

	get_info_op->uris = mate_vfs_uri_list_copy (near_uri_list);
	get_info_op->kind = kind;
	get_info_op->create_if_needed = create_if_needed;
	get_info_op->find_if_needed = find_if_needed;
	get_info_op->permissions = permissions;

	*handle_return = job->job_handle;
	_mate_vfs_job_go (job);
}

/* Register MateVFSFindDirectoryResult in the GType system */
GType
mate_vfs_find_directory_result_get_type (void)
{
	static GType our_type = 0;

	if (our_type == 0) {
		our_type = g_boxed_type_register_static ("MateVfsFindDirectoryResult",
			(GBoxedCopyFunc) mate_vfs_find_directory_result_dup,
			(GBoxedFreeFunc) mate_vfs_find_directory_result_free);
      
	}
		
	return our_type;
}

/**
 * mate_vfs_find_directory_result_dup:
 * @result: a #MateVFSFindDirectoryResult.
 *
 * Duplicates @result.
 *
 * Note: The internal uri is not duplicated but its
 * refcount is incremented.
 * 
 * Return value: A 1:1 copy of @result.
 * 
 * Since: 2.12
 */
MateVFSFindDirectoryResult*
mate_vfs_find_directory_result_dup (MateVFSFindDirectoryResult *result)
{
	MateVFSFindDirectoryResult *copy;
	
	g_return_val_if_fail (result != NULL, NULL);

	copy = g_new0 (MateVFSFindDirectoryResult, 1);

	/* "Copy" and ref the objects. */
	copy->uri = result->uri;
	mate_vfs_uri_ref (result->uri);

	copy->result = result->result;

	return copy;
}

/**
 * mate_vfs_find_directory_result_free:
 * @result: a #MateVFSFindDirectoryResult.
 *
 * Unrefs the inner uri object and frees the memory 
 * allocated for @result.
 *
 * Since: 2.12
 */
void
mate_vfs_find_directory_result_free (MateVFSFindDirectoryResult* result)
{
	g_return_if_fail (result != NULL);

	mate_vfs_uri_unref (result->uri);
	result->uri = NULL;

	g_free (result);
}


static MateVFSAsyncHandle *
async_load_directory (MateVFSURI *uri,
		      MateVFSFileInfoOptions options,
		      guint items_per_notification,
		      int priority,
		      MateVFSAsyncDirectoryLoadCallback callback,
		      gpointer callback_data)
{
	MateVFSJob *job;
	MateVFSLoadDirectoryOp *load_directory_op;
	MateVFSAsyncHandle *result;

	job = _mate_vfs_job_new (MATE_VFS_OP_LOAD_DIRECTORY, priority, (GFunc) callback, callback_data);

	load_directory_op = &job->op->specifics.load_directory;
	load_directory_op->uri = uri == NULL ? NULL : mate_vfs_uri_ref (uri);
	load_directory_op->options = options;
	load_directory_op->items_per_notification = items_per_notification;

	result = job->job_handle;
	_mate_vfs_job_go (job);

	return result;
}



/**
 * mate_vfs_async_load_directory:
 * @handle_return: when the function returns, will point to a handle for
 * the async operation.
 * @text_uri: string representing the uri of the directory to be loaded.
 * @options: packed boolean type providing control over various details
 * of the get_file_info operation.
 * @items_per_notification: number of files to process in a row before calling @callback
 * @priority: a value from %MATE_VFS_PRIORITY_MIN to %MATE_VFS_PRIORITY_MAX (normally
 * should be %MATE_VFS_PRIORITY_DEFAULT) indicating the priority to assign to this job
 * in allocating threads from the thread pool.
 * @callback: function to be called when the operation is complete.
 * @callback_data: data to pass to @callback.
 * 
 * Read the contents of the directory at @text_uri, passing back #MateVFSFileInfo 
 * structs about each file in the directory to @callback. @items_per_notification
 * files will be processed between each call to @callback.
 */
void
mate_vfs_async_load_directory (MateVFSAsyncHandle **handle_return,
				const gchar *text_uri,
				MateVFSFileInfoOptions options,
				guint items_per_notification,
				int priority,
				MateVFSAsyncDirectoryLoadCallback callback,
				gpointer callback_data)
{
	MateVFSURI *uri;

	g_return_if_fail (handle_return != NULL);
	g_return_if_fail (text_uri != NULL);
	g_return_if_fail (callback != NULL);
	g_return_if_fail (priority >= MATE_VFS_PRIORITY_MIN);
	g_return_if_fail (priority <= MATE_VFS_PRIORITY_MAX);

	uri = mate_vfs_uri_new (text_uri);
	*handle_return = async_load_directory (uri, options,
				               items_per_notification,
				               priority,
					       callback, callback_data);
	if (uri != NULL) {
		mate_vfs_uri_unref (uri);
	}
}

/**
 * mate_vfs_async_load_directory_uri:
 * @handle_return: when the function returns, will point to a handle for
 * the async operation.
 * @uri: uri of the directory to be loaded.
 * @options: packed boolean type providing control over various details
 * of the get_file_info operation.
 * @items_per_notification: number of files to process in a row before calling @callback
 * @priority: a value from %MATE_VFS_PRIORITY_MIN to %MATE_VFS_PRIORITY_MAX (normally
 * should be %MATE_VFS_PRIORITY_DEFAULT) indicating the priority to assign this job
 * in allocating threads from the thread pool.
 * @callback: function to be called when the operation is complete.
 * @callback_data: data to pass to @callback.
 * 
 * Read the contents of the directory at @uri, passing back #MateVFSFileInfo structs
 * about each file in the directory to @callback. @items_per_notification
 * files will be processed between each call to @callback.
 */
void
mate_vfs_async_load_directory_uri (MateVFSAsyncHandle **handle_return,
				    MateVFSURI *uri,
				    MateVFSFileInfoOptions options,
				    guint items_per_notification,
				    int priority,
				    MateVFSAsyncDirectoryLoadCallback callback,
				    gpointer callback_data)
{
	g_return_if_fail (handle_return != NULL);
	g_return_if_fail (uri != NULL);
	g_return_if_fail (callback != NULL);
	g_return_if_fail (priority >= MATE_VFS_PRIORITY_MIN);
	g_return_if_fail (priority <= MATE_VFS_PRIORITY_MAX);

	*handle_return = async_load_directory (uri, options,
					       items_per_notification,
					       priority,
					       callback, callback_data);
}

/**
 * mate_vfs_async_xfer:
 * @handle_return: when the function returns, will point to a handle for the operation.
 * @source_uri_list: A #GList of source #MateVFSURIs.
 * @target_uri_list: A #GList of target #MateVFSURIs, each corresponding to one URI in
 * @source_uri_list.
 * @xfer_options: #MateVFSXferOptions defining the desired operation and parameters.
 * @error_mode: A #MateVFSErrorMode specifying how to proceed if a VFS error occurs.
 * @overwrite_mode: A #MateVFSOverwriteMode specifying how to proceed if a file is being overwritten.
 * @priority: A value from %MATE_VFS_PRIORITY_MIN to %MATE_VFS_PRIORITY_MAX (normally
 * should be %MATE_VFS_PRIORITY_DEFAULT) indicating the priority to assign this job
 * in allocating threads from the thread pool.
 * @progress_update_callback: A #MateVFSAsyncXferCallback called periodically for
 * informing the program about progress, and when the program requires responses to
 * interactive queries (e.g. overwriting files, handling errors, etc).
 * @update_callback_data: User data to pass to @progress_update_callback.
 * @progress_sync_callback: An optional #MateVFSXferCallback called whenever some state changed.
 * @sync_callback_data: User data to pass to @progress_sync_callback.
 *
 * Performs an Xfer operation in a seperate thread, otherwise like
 * mate_vfs_xfer_uri_list().
 *
 * See #MateVFSAsyncXferProgressCallback and #MateVFSXferProgressCallback for details on how the
 * callback mechanism works.
 *
 * <note>
 *  <para>
 *   @progress_sync_callback should only be used if you want to execute additional
 *   actions that may not wait until after the transfer, for instance because
 *   you have to do them for each transferred file/directory, and that require
 *   a very specific action to be taken. For instance, the Caja application
 *   schedules metadata removal/moving/copying at specific phases.
 *  </para>
 *  <para>
 *   Do not use @progress_sync_callback if you just need user feedback, because
 *   each invocation is expensive, and requires a context switch.
 *  </para>
 *  <para>
 *   If you use both @progress_update_callback and @progress_sync_callback,
 *   the @progress_sync_callback will always be invoked before the
 *   @progress_update_callback. It is recommended to do conflict
 *   handling in @progress_update_callback, and always return %TRUE
 *   in @progress_sync_callback, because if the Xfer's
 *   #MateVFSXferProgressStatus is %MATE_VFS_XFER_PROGRESS_STATUS_OK,
 *   @progress_update_callback will only be invoked if it hasn't
 *   been invoked within the last 100 milliseconds, and if 
 *   @progress_update_callback is not invoked, only
 *   @progress_sync_callback is authoritative for the
 *   further processing, causing abortion if it is %FALSE.
 *  </para>
 * </note>
 *
 * Return value: %MATE_VFS_OK if the paramaters were in order, 
 * or %MATE_VFS_ERROR_BAD_PARAMETERS if something was wrong in the passed in arguments.
 */
MateVFSResult
mate_vfs_async_xfer (MateVFSAsyncHandle **handle_return,
		      GList *source_uri_list,
		      GList *target_uri_list,
		      MateVFSXferOptions xfer_options,
		      MateVFSXferErrorMode error_mode,
		      MateVFSXferOverwriteMode overwrite_mode,
		      int priority,
		      MateVFSAsyncXferProgressCallback progress_update_callback,
		      gpointer update_callback_data,
		      MateVFSXferProgressCallback progress_sync_callback,
		      gpointer sync_callback_data)
{
	MateVFSJob *job;
	MateVFSXferOp *xfer_op;

	g_return_val_if_fail (handle_return != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (progress_update_callback != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (priority >= MATE_VFS_PRIORITY_MIN, MATE_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (priority <= MATE_VFS_PRIORITY_MAX, MATE_VFS_ERROR_BAD_PARAMETERS);

	job = _mate_vfs_job_new (MATE_VFS_OP_XFER,
				 priority, 
			         (GFunc) progress_update_callback,
			         update_callback_data);


	xfer_op = &job->op->specifics.xfer;
	xfer_op->source_uri_list = mate_vfs_uri_list_copy (source_uri_list);
	xfer_op->target_uri_list = mate_vfs_uri_list_copy (target_uri_list);
	xfer_op->xfer_options = xfer_options;
	xfer_op->error_mode = error_mode;
	xfer_op->overwrite_mode = overwrite_mode;
	xfer_op->progress_sync_callback = progress_sync_callback;
	xfer_op->sync_callback_data = sync_callback_data;

	*handle_return = job->job_handle;
	_mate_vfs_job_go (job);

	return MATE_VFS_OK;
}

/**
 * mate_vfs_async_file_control:
 * @handle: handle of the file to affect.
 * @operation: operation to execute.
 * @operation_data: data needed to execute the operation.
 * @operation_data_destroy_func: callback to destroy @operation_data when its no longer needed.
 * @callback: function to be called when the @operation is complete.
 * @callback_data: data to pass to @callback.
 * 
 * Execute a backend dependent operation specified by the string @operation.
 * This is typically used for specialized vfs backends that need additional
 * operations that mate-vfs doesn't have. Compare it to the unix call ioctl().
 * The format of @operation_data depends on the operation. Operation that are
 * backend specific are normally namespaced by their module name.
 *
 * When the operation is complete, @callback will be called with the
 * result of the operation, @operation_data and @callback_data.
 *
 * Since: 2.2
 */
void
mate_vfs_async_file_control (MateVFSAsyncHandle *handle,
			      const char *operation,
			      gpointer operation_data,
			      GDestroyNotify operation_data_destroy_func,
			      MateVFSAsyncFileControlCallback callback,
			      gpointer callback_data)
{
	MateVFSJob *job;
	MateVFSFileControlOp *file_control_op;

	g_return_if_fail (handle != NULL);
	g_return_if_fail (operation != NULL);
	g_return_if_fail (callback != NULL);

	_mate_vfs_async_job_map_lock ();
	job = _mate_vfs_async_job_map_get_job (handle);
	if (job == NULL) {
		g_warning ("trying to call file_control on a non-existing handle");
		_mate_vfs_async_job_map_unlock ();
		return;
	}

	_mate_vfs_job_set (job, MATE_VFS_OP_FILE_CONTROL,
			   (GFunc) callback, callback_data);

	file_control_op = &job->op->specifics.file_control;
	file_control_op->operation = g_strdup (operation);
	file_control_op->operation_data = operation_data;
	file_control_op->operation_data_destroy_func = operation_data_destroy_func;

	_mate_vfs_job_go (job);
	_mate_vfs_async_job_map_unlock ();
}

#ifdef OLD_CONTEXT_DEPRECATED

guint
mate_vfs_async_add_status_callback (MateVFSAsyncHandle *handle,
				     MateVFSStatusCallback callback,
				     gpointer user_data)
{
	MateVFSJob *job;
	guint result;
	
	g_return_val_if_fail (handle != NULL, 0);
	g_return_val_if_fail (callback != NULL, 0);

	_mate_vfs_async_job_map_lock ();
	job = _mate_vfs_async_job_map_get_job (handle);

	if (job->op != NULL || job->op->context != NULL) {
		g_warning ("job or context not found");
		_mate_vfs_async_job_map_unlock ();
		return 0;
	}

	result = mate_vfs_message_callbacks_add
		(mate_vfs_context_get_message_callbacks (job->op->context),
		 callback, user_data);
	_mate_vfs_async_job_map_unlock ();
	
	return result;
}

void
mate_vfs_async_remove_status_callback (MateVFSAsyncHandle *handle,
					guint callback_id)
{
	MateVFSJob *job;

	g_return_if_fail (handle != NULL);
	g_return_if_fail (callback_id > 0);

	_mate_vfs_async_job_map_lock ();
	job = _mate_vfs_async_job_map_get_job (handle);

	if (job->op != NULL || job->op->context != NULL) {
		g_warning ("job or context not found");
		_mate_vfs_async_job_map_unlock ();
		return;
	}

	mate_vfs_message_callbacks_remove
		(mate_vfs_context_get_message_callbacks (job->op->context),
		 callback_id);

	_mate_vfs_async_job_map_unlock ();
}

#endif /* OLD_CONTEXT_DEPRECATED */
