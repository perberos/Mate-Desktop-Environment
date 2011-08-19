/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */ /*
mate-vfs-job.c - Jobs for asynchronous operation of the MATE Virtual File
System (version for POSIX threads).

   Copyright (C) 1999 Free Software Foundation
   Copyright (C) 2000 Eazel

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

   Authors: 
   	Ettore Perazzoli <ettore@gnu.org> 
  	Pavel Cisler <pavel@eazel.com> 
  	Darin Adler <darin@eazel.com> 

   */

#include <config.h>
#include "mate-vfs-job.h"

#include "mate-vfs-async-job-map.h"
#include "mate-vfs-job-queue.h"
#include "mate-vfs-private-utils.h"
#include "mate-vfs-module-callback-private.h"
#include <errno.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <libmatevfs/mate-vfs-cancellable-ops.h>
#include <libmatevfs/mate-vfs-context.h>
#include <libmatevfs/mate-vfs-backend.h>
#include <string.h>
#include <unistd.h>

static GStaticPrivate job_private = G_STATIC_PRIVATE_INIT;

#if MATE_VFS_JOB_DEBUG

static const char *job_debug_types[] = {
	"open", "open as channel",
	"create", "create symbolic link",
	"create as channel", "close",
	"read", "write", "seek", "read write done",
	"load directory", "find directory",
	"xfer", "get file info", "set file info",
	"module callback", "file control",
	"**error**"
};

/* FIXME bugzilla.eazel.com 1130
 * - this is should use the correct static mutex initialization macro.
 * However glibconfig.h is broken and the supplied macro gives a warning.
 * Since this id debug only, just use what the macro should be here.
 * even though it is not portable.
 */
GStaticMutex debug_mutex = G_STATIC_MUTEX_INIT;
#endif

static int job_count = 0;

static void     mate_vfs_op_destroy                (MateVFSOp           *op);
static void     _mate_vfs_job_destroy_notify_result (MateVFSNotifyResult *notify_result);
static gboolean dispatch_job_callback               (gpointer              data);
static gboolean dispatch_sync_job_callback          (gpointer              data);

static void	clear_current_job 		    (void);
static void	set_current_job 		    (MateVFSJob *context);

/*
 *   Find out whether or not a given job should be left in
 * the job map, preserving it's open VFS handle, since we
 * can do more operations on it later.
 */
gboolean
_mate_vfs_job_complete (MateVFSJob *job)
{
	g_assert (job->op != NULL);
	
	switch (job->op->type) {
	case MATE_VFS_OP_OPEN:
	case MATE_VFS_OP_OPEN_AS_CHANNEL:
	case MATE_VFS_OP_CREATE:
	case MATE_VFS_OP_CREATE_AS_CHANNEL:
	case MATE_VFS_OP_CREATE_SYMBOLIC_LINK:
		/* if job got cancelled, no close expected */
		return job->cancelled || job->failed;

	case MATE_VFS_OP_READ:
	case MATE_VFS_OP_WRITE:
		g_assert_not_reached();
		return FALSE;
	case MATE_VFS_OP_READ_WRITE_DONE:
	case MATE_VFS_OP_FILE_CONTROL:
	case MATE_VFS_OP_SEEK:
		return FALSE;
	
	default:
		return TRUE;
	}
}

/* This notifies the master thread asynchronously, without waiting for an
 * acknowledgment.
 */
static void
job_oneway_notify (MateVFSJob *job, MateVFSNotifyResult *notify_result)
{
	if (_mate_vfs_async_job_add_callback (job, notify_result)) {
		JOB_DEBUG (("job %u, callback %u type '%s'",
			    GPOINTER_TO_UINT (notify_result->job_handle),
			    notify_result->callback_id,
			    JOB_DEBUG_TYPE (job->op->type)));
	
		g_idle_add (dispatch_job_callback, notify_result);
	} else {
		JOB_DEBUG (("Barfing on oneway cancel %u (%d) type '%s'",
			    GPOINTER_TO_UINT (notify_result->job_handle),
			    job->op->type, JOB_DEBUG_TYPE (job->op->type)));
		/* TODO: We can leak handle here, if an open succeded.
		 * See bug #123472 */
		_mate_vfs_job_destroy_notify_result (notify_result);
	}
}

/* This notifies the master threads, waiting until it acknowledges the
   notification.  */
static void
job_notify (MateVFSJob *job, MateVFSNotifyResult *notify_result)
{
	if (!_mate_vfs_async_job_add_callback (job, notify_result)) {
		JOB_DEBUG (("Barfing on sync cancel %u (%d)",
			    GPOINTER_TO_UINT (notify_result->job_handle),
			    job->op->type));
		_mate_vfs_job_destroy_notify_result (notify_result);
		return;
	}

	/* Send the notification.  This will wake up the master thread, which
         * will in turn signal the notify condition.
         */
	g_idle_add (dispatch_sync_job_callback, notify_result);

	JOB_DEBUG (("Wait notify condition %u", GPOINTER_TO_UINT (notify_result->job_handle)));
	/* Wait for the notify condition.  */
	g_cond_wait (job->notify_ack_condition, job->job_lock);

	JOB_DEBUG (("Got notify ack condition %u", GPOINTER_TO_UINT (notify_result->job_handle)));
}

static void
dispatch_open_callback (MateVFSNotifyResult *notify_result)
{
	(* notify_result->specifics.open.callback) (notify_result->job_handle,
						    notify_result->specifics.open.result,
						    notify_result->specifics.open.callback_data);
}

static void
dispatch_create_callback (MateVFSNotifyResult *notify_result)
{
	(* notify_result->specifics.create.callback) (notify_result->job_handle,
						      notify_result->specifics.create.result,
						      notify_result->specifics.create.callback_data);
}

static void
dispatch_open_as_channel_callback (MateVFSNotifyResult *notify_result)
{
	(* notify_result->specifics.open_as_channel.callback) (notify_result->job_handle,
							       notify_result->specifics.open_as_channel.channel,
							       notify_result->specifics.open_as_channel.result,
							       notify_result->specifics.open_as_channel.callback_data);
}

static void
dispatch_create_as_channel_callback (MateVFSNotifyResult *notify_result)
{
	(* notify_result->specifics.create_as_channel.callback) (notify_result->job_handle,
								 notify_result->specifics.create_as_channel.channel,
								 notify_result->specifics.create_as_channel.result,
								 notify_result->specifics.create_as_channel.callback_data);
}

static void
dispatch_close_callback (MateVFSNotifyResult *notify_result)
{
	(* notify_result->specifics.close.callback) (notify_result->job_handle,
						     notify_result->specifics.close.result,
						     notify_result->specifics.close.callback_data);
}

static void
dispatch_read_callback (MateVFSNotifyResult *notify_result)
{
	(* notify_result->specifics.read.callback) (notify_result->job_handle,
						    notify_result->specifics.read.result,
						    notify_result->specifics.read.buffer,
						    notify_result->specifics.read.num_bytes,
						    notify_result->specifics.read.bytes_read,
						    notify_result->specifics.read.callback_data);
}

static void
dispatch_write_callback (MateVFSNotifyResult *notify_result)
{
	(* notify_result->specifics.write.callback) (notify_result->job_handle,
						     notify_result->specifics.write.result,
						     notify_result->specifics.write.buffer,
						     notify_result->specifics.write.num_bytes,
						     notify_result->specifics.write.bytes_written,
						     notify_result->specifics.write.callback_data);
}

static void
dispatch_seek_callback (MateVFSNotifyResult *notify_result)
{
	(* notify_result->specifics.seek.callback) (notify_result->job_handle,
						    notify_result->specifics.seek.result,
						    notify_result->specifics.seek.callback_data);
}

static void
dispatch_load_directory_callback (MateVFSNotifyResult *notify_result)
{
	(* notify_result->specifics.load_directory.callback) (notify_result->job_handle,
							      notify_result->specifics.load_directory.result,
							      notify_result->specifics.load_directory.list,
							      notify_result->specifics.load_directory.entries_read,
							      notify_result->specifics.load_directory.callback_data);
}

static void
dispatch_get_file_info_callback (MateVFSNotifyResult *notify_result)
{
	(* notify_result->specifics.get_file_info.callback) (notify_result->job_handle,
							     notify_result->specifics.get_file_info.result_list,
							     notify_result->specifics.get_file_info.callback_data);
}

static void
dispatch_find_directory_callback (MateVFSNotifyResult *notify_result)
{
	(* notify_result->specifics.find_directory.callback) (notify_result->job_handle,
							      notify_result->specifics.find_directory.result_list,
							      notify_result->specifics.find_directory.callback_data);
}

static void
dispatch_set_file_info_callback (MateVFSNotifyResult *notify_result)
{
	gboolean new_info_is_valid;

	new_info_is_valid = notify_result->specifics.set_file_info.set_file_info_result == MATE_VFS_OK
		&& notify_result->specifics.set_file_info.get_file_info_result == MATE_VFS_OK;
		
	(* notify_result->specifics.set_file_info.callback) (notify_result->job_handle,
							     notify_result->specifics.set_file_info.set_file_info_result,
							     new_info_is_valid ? notify_result->specifics.set_file_info.info : NULL,
							     notify_result->specifics.set_file_info.callback_data);
}

static void
dispatch_xfer_callback (MateVFSNotifyResult *notify_result, gboolean cancelled)
{
	if (cancelled) {
		/* make the xfer operation stop */
		notify_result->specifics.xfer.reply = 0;
		return;
	}
	
	notify_result->specifics.xfer.reply = (* notify_result->specifics.xfer.callback) (
							    notify_result->job_handle,
							    notify_result->specifics.xfer.progress_info,
						            notify_result->specifics.xfer.callback_data);
}

static void
dispatch_module_callback (MateVFSNotifyResult *notify_result)
{
	notify_result->specifics.callback.callback (notify_result->specifics.callback.in,
						    notify_result->specifics.callback.in_size,
						    notify_result->specifics.callback.out,
						    notify_result->specifics.callback.out_size,
						    notify_result->specifics.callback.user_data,
						    notify_result->specifics.callback.response,
						    notify_result->specifics.callback.response_data);
}

static void
dispatch_file_control_callback (MateVFSNotifyResult *notify_result)
{
	notify_result->specifics.file_control.callback (notify_result->job_handle,
							notify_result->specifics.file_control.result,
							notify_result->specifics.file_control.operation_data,
							notify_result->specifics.file_control.callback_data);
}

static void
empty_close_callback (MateVFSAsyncHandle *handle,
		      MateVFSResult result,
		      gpointer callback_data)
{
}

static void
handle_cancelled_open (MateVFSJob *job)
{
	/* schedule a silent close to make sure the handle does not leak */
	_mate_vfs_job_set (job, MATE_VFS_OP_CLOSE, 
			   (GFunc) empty_close_callback, NULL);
	_mate_vfs_job_go (job);
}

static void
free_get_file_info_notify_result (MateVFSGetFileInfoOpResult *notify_result)
{
	GList *p;
	MateVFSGetFileInfoResult *result_item;
	
	for (p = notify_result->result_list; p != NULL; p = p->next) {
		result_item = p->data;

		mate_vfs_uri_unref (result_item->uri);
		mate_vfs_file_info_unref (result_item->file_info);
		g_free (result_item);
	}
	g_list_free (notify_result->result_list);
}

static void
free_find_directory_notify_result (MateVFSFindDirectoryOpResult *notify_result)
{
	GList *p;
	MateVFSFindDirectoryResult *result_item;

	for (p = notify_result->result_list; p != NULL; p = p->next) {
		result_item = p->data;

		if (result_item->uri != NULL) {
			mate_vfs_uri_unref (result_item->uri);
		}
		g_free (result_item);
	}
	g_list_free (notify_result->result_list);
}

static void
_mate_vfs_job_destroy_notify_result (MateVFSNotifyResult *notify_result)
{
	JOB_DEBUG (("%u", notify_result->callback_id));

	switch (notify_result->type) {
	case MATE_VFS_OP_CLOSE:
	case MATE_VFS_OP_CREATE:
	case MATE_VFS_OP_CREATE_AS_CHANNEL:
	case MATE_VFS_OP_CREATE_SYMBOLIC_LINK:
	case MATE_VFS_OP_WRITE:
	case MATE_VFS_OP_SEEK:
	case MATE_VFS_OP_OPEN:
	case MATE_VFS_OP_OPEN_AS_CHANNEL:
	case MATE_VFS_OP_READ:
		g_free (notify_result);
		break;
		
	case MATE_VFS_OP_FILE_CONTROL:
		if (notify_result->specifics.file_control.operation_data_destroy_func) {
			notify_result->specifics.file_control.operation_data_destroy_func (
					    notify_result->specifics.file_control.operation_data);
		}
		g_free (notify_result);
		break;
		
	case MATE_VFS_OP_FIND_DIRECTORY:
		free_find_directory_notify_result (&notify_result->specifics.find_directory);
		g_free (notify_result);
		break;
		
	case MATE_VFS_OP_GET_FILE_INFO:
		free_get_file_info_notify_result (&notify_result->specifics.get_file_info);
		g_free (notify_result);
		break;
		
	case MATE_VFS_OP_SET_FILE_INFO:
		mate_vfs_file_info_unref (notify_result->specifics.set_file_info.info);
		g_free (notify_result);
		break;
		
	case MATE_VFS_OP_LOAD_DIRECTORY:
		mate_vfs_file_info_list_free (notify_result->specifics.load_directory.list);
		g_free (notify_result);
		break;

	case MATE_VFS_OP_XFER:
		/* the XFER result is allocated on the stack */
		break;
		
	case MATE_VFS_OP_MODULE_CALLBACK:
		/* the MODULE_CALLBACK result is allocated on the stack */
		break;
	
	default:
		g_assert_not_reached ();
		break;
	}
}

/* Entry point for sync notification callback */
static gboolean
dispatch_sync_job_callback (gpointer data)
{
	MateVFSNotifyResult *notify_result;
	MateVFSJob *job;
	gboolean valid;
	gboolean cancelled;

	notify_result = (MateVFSNotifyResult *) data;

	_mate_vfs_async_job_callback_valid (notify_result->callback_id, &valid, &cancelled);

	/* Even though the notify result is owned by the async thread and persists
	 * all through the notification, we still keep it in the job map to
	 * make cancellation easier.
	 */
	_mate_vfs_async_job_remove_callback (notify_result->callback_id);

	g_assert (valid);

	switch (notify_result->type) {
	case MATE_VFS_OP_CREATE_AS_CHANNEL:
		dispatch_create_as_channel_callback (notify_result);
		break;
	case MATE_VFS_OP_OPEN_AS_CHANNEL:
		dispatch_open_as_channel_callback (notify_result);
		break;		
	case MATE_VFS_OP_XFER:
		dispatch_xfer_callback (notify_result, cancelled);
		break;
	case MATE_VFS_OP_MODULE_CALLBACK:
		dispatch_module_callback (notify_result);
		break;
	default:
		g_assert_not_reached ();
		break;
	}
	
	_mate_vfs_async_job_map_lock ();
	job = _mate_vfs_async_job_map_get_job (notify_result->job_handle);
	g_mutex_lock (job->job_lock);
	_mate_vfs_async_job_map_unlock ();
	
	g_assert (job != NULL);
	
	JOB_DEBUG (("signalling %u", GPOINTER_TO_UINT (notify_result->job_handle)));
	
	/* Signal the async thread that we are done with the notification. */
	g_cond_signal (job->notify_ack_condition);
	g_mutex_unlock (job->job_lock);

	return FALSE;
}

/* Entry point for async notification callback */
static gboolean
dispatch_job_callback (gpointer data)

{
	MateVFSNotifyResult *notify_result;
	MateVFSJob *job;
	gboolean valid;
	gboolean cancelled;
	
	notify_result = (MateVFSNotifyResult *) data;

	JOB_DEBUG (("%u type '%s'", GPOINTER_TO_UINT (notify_result->job_handle),
		    JOB_DEBUG_TYPE (notify_result->type)));
	
	_mate_vfs_async_job_callback_valid (notify_result->callback_id, &valid, &cancelled);
	_mate_vfs_async_job_remove_callback (notify_result->callback_id);

	if (!valid) {
		/* this can happen when mate vfs is shutting down */
		JOB_DEBUG (("shutting down: callback %u no longer valid",
			    notify_result->callback_id));
		_mate_vfs_job_destroy_notify_result (notify_result);
		return FALSE;
	}
	
	if (cancelled) {
		/* cancel the job in progress */
		JOB_DEBUG (("cancelling job %u %u",
			    GPOINTER_TO_UINT (notify_result->job_handle),
			    notify_result->callback_id));

		_mate_vfs_async_job_map_lock ();

		job = _mate_vfs_async_job_map_get_job (notify_result->job_handle);
		
		if (job != NULL) {
			g_mutex_lock (job->job_lock);

			switch (job->op->type) {
			case MATE_VFS_OP_OPEN:
			case MATE_VFS_OP_OPEN_AS_CHANNEL:
			case MATE_VFS_OP_CREATE:
			case MATE_VFS_OP_CREATE_AS_CHANNEL:
			case MATE_VFS_OP_CREATE_SYMBOLIC_LINK:
				if (job->handle) {
					g_mutex_unlock (job->job_lock);
					handle_cancelled_open (job);
					JOB_DEBUG (("handle cancel open job %u",
						    GPOINTER_TO_UINT (notify_result->job_handle)));
					break;
				} /* else drop through */
			default:
				/* Remove job from the job map. */
				_mate_vfs_async_job_map_remove_job (job);
				g_mutex_unlock (job->job_lock);
				break;
			}
		}
	
		_mate_vfs_async_job_map_unlock ();
		_mate_vfs_job_destroy_notify_result (notify_result);
		return FALSE;
	}
	
		
	JOB_DEBUG (("executing callback %u", GPOINTER_TO_UINT (notify_result->job_handle)));	

	switch (notify_result->type) {
	case MATE_VFS_OP_CLOSE:
		dispatch_close_callback (notify_result);
		break;
	case MATE_VFS_OP_CREATE:
		dispatch_create_callback (notify_result);
		break;
	case MATE_VFS_OP_CREATE_AS_CHANNEL:
		dispatch_create_as_channel_callback (notify_result);
		break;
	case MATE_VFS_OP_CREATE_SYMBOLIC_LINK:
		dispatch_create_callback (notify_result);
		break;
	case MATE_VFS_OP_FIND_DIRECTORY:
		dispatch_find_directory_callback (notify_result);
		break;
	case MATE_VFS_OP_GET_FILE_INFO:
		dispatch_get_file_info_callback (notify_result);
		break;
	case MATE_VFS_OP_LOAD_DIRECTORY:
		dispatch_load_directory_callback (notify_result);
		break;
	case MATE_VFS_OP_OPEN:
		dispatch_open_callback (notify_result);
		break;
	case MATE_VFS_OP_OPEN_AS_CHANNEL:
		dispatch_open_as_channel_callback (notify_result);
		break;
	case MATE_VFS_OP_READ:
		dispatch_read_callback (notify_result);
		break;
	case MATE_VFS_OP_SET_FILE_INFO:
		dispatch_set_file_info_callback (notify_result);
		break;
	case MATE_VFS_OP_WRITE:
		dispatch_write_callback (notify_result);
		break;
	case MATE_VFS_OP_SEEK:
		dispatch_seek_callback (notify_result);
		break;
	case MATE_VFS_OP_FILE_CONTROL:
		dispatch_file_control_callback (notify_result);
		break;
	default:
		g_assert_not_reached ();
		break;
	}

	JOB_DEBUG (("dispatch callback - done %u", GPOINTER_TO_UINT (notify_result->job_handle)));
	_mate_vfs_job_destroy_notify_result (notify_result);

	return FALSE;
}

void
_mate_vfs_job_set (MateVFSJob *job,
		   MateVFSOpType type,
		   GFunc callback,
		   gpointer callback_data)
{
	MateVFSOp *op;

	op = g_new (MateVFSOp, 1);
	op->type = type;
	op->callback = callback;
	op->callback_data = callback_data;
	op->context = mate_vfs_context_new ();
	op->stack_info = _mate_vfs_module_callback_get_stack_info ();

	g_assert (mate_vfs_context_get_cancellation (op->context) != NULL);

	JOB_DEBUG (("locking access lock %u, op %d", GPOINTER_TO_UINT (job->job_handle), type));

	g_mutex_lock (job->job_lock);

	mate_vfs_op_destroy (job->op);
	job->op = op;
	job->cancelled = FALSE;

	g_mutex_unlock (job->job_lock);

	JOB_DEBUG (("%u op type %d, op %p", GPOINTER_TO_UINT (job->job_handle),
		job->op->type, job->op));
}

MateVFSJob *
_mate_vfs_job_new (MateVFSOpType type, int priority, GFunc callback, gpointer callback_data)
{
	MateVFSJob *new_job;
	
	new_job = g_new0 (MateVFSJob, 1);

	new_job->job_lock = g_mutex_new ();
	new_job->notify_ack_condition = g_cond_new ();
	new_job->priority = priority;

	/* Add the new job into the job hash table. This also assigns
	 * the job a unique id
	 */
	_mate_vfs_async_job_map_add_job (new_job);
	_mate_vfs_job_set (new_job, type, callback, callback_data);

	job_count++;

	return new_job;
}

void
_mate_vfs_job_destroy (MateVFSJob *job)
{
	JOB_DEBUG (("destroying job %u", GPOINTER_TO_UINT (job->job_handle)));

	mate_vfs_op_destroy (job->op);

	g_mutex_free (job->job_lock);
	g_cond_free (job->notify_ack_condition);

	memset (job, 0xaa, sizeof (MateVFSJob));

	g_free (job);
	job_count--;

	JOB_DEBUG (("job %u terminated cleanly", GPOINTER_TO_UINT (job->job_handle)));
}

int
mate_vfs_job_get_count (void)
{
	return job_count;
}

static void
mate_vfs_op_destroy (MateVFSOp *op)
{
	if (op == NULL) {
		return;
	}
	
	switch (op->type) {
	case MATE_VFS_OP_CREATE:
		if (op->specifics.create.uri != NULL) {
			mate_vfs_uri_unref (op->specifics.create.uri);
		}
		break;
	case MATE_VFS_OP_CREATE_AS_CHANNEL:
		if (op->specifics.create_as_channel.uri != NULL) {
			mate_vfs_uri_unref (op->specifics.create_as_channel.uri);
		}
		break;
	case MATE_VFS_OP_CREATE_SYMBOLIC_LINK:
		mate_vfs_uri_unref (op->specifics.create_symbolic_link.uri);
		g_free (op->specifics.create_symbolic_link.uri_reference);
		break;
	case MATE_VFS_OP_FIND_DIRECTORY:
		mate_vfs_uri_list_free (op->specifics.find_directory.uris);
		break;
	case MATE_VFS_OP_GET_FILE_INFO:
		mate_vfs_uri_list_free (op->specifics.get_file_info.uris);
		break;
	case MATE_VFS_OP_LOAD_DIRECTORY:
		if (op->specifics.load_directory.uri != NULL) {
			mate_vfs_uri_unref (op->specifics.load_directory.uri);
		}
		break;
	case MATE_VFS_OP_OPEN:
		if (op->specifics.open.uri != NULL) {
			mate_vfs_uri_unref (op->specifics.open.uri);
		}
		break;
	case MATE_VFS_OP_OPEN_AS_CHANNEL:
		if (op->specifics.open_as_channel.uri != NULL) {
			mate_vfs_uri_unref (op->specifics.open_as_channel.uri);
		}
		break;
	case MATE_VFS_OP_SET_FILE_INFO:
		mate_vfs_uri_unref (op->specifics.set_file_info.uri);
		mate_vfs_file_info_unref (op->specifics.set_file_info.info);
		break;
	case MATE_VFS_OP_XFER:
		mate_vfs_uri_list_free (op->specifics.xfer.source_uri_list);
		mate_vfs_uri_list_free (op->specifics.xfer.target_uri_list);
		break;
	case MATE_VFS_OP_READ:
	case MATE_VFS_OP_WRITE:
	case MATE_VFS_OP_SEEK:
	case MATE_VFS_OP_CLOSE:
	case MATE_VFS_OP_READ_WRITE_DONE:
		break;
	case MATE_VFS_OP_FILE_CONTROL:
		g_free (op->specifics.file_control.operation);
		break;
	default:
		g_warning (_("Unknown operation type %u"), op->type);
	}
	
	g_assert (mate_vfs_context_get_cancellation (op->context) != NULL);
	
	mate_vfs_context_free (op->context);
	_mate_vfs_module_callback_free_stack_info (op->stack_info);
	
	g_free (op);
}

void
_mate_vfs_job_go (MateVFSJob *job)
{
	JOB_DEBUG (("new job %u, op %d, type '%s' unlocking job lock",
		    GPOINTER_TO_UINT (job->job_handle), job->op->type,
		    JOB_DEBUG_TYPE (job->op->type)));

	/* Fire up the async job thread. */
	if (!_mate_vfs_job_schedule (job)) {
		g_warning ("Cannot schedule this job.");
		_mate_vfs_job_destroy (job);
		return;
	}
}

#define DEFAULT_BUFFER_SIZE 16384

static void
serve_channel_read (MateVFSHandle *handle,
		    GIOChannel *channel_in,
		    GIOChannel *channel_out,
		    gulong advised_block_size,
		    MateVFSContext *context)
{
	gpointer buffer;
	guint filled_bytes_in_buffer;
	guint written_bytes_in_buffer;
	guint current_buffer_size;
	
	if (advised_block_size == 0) {
		advised_block_size = DEFAULT_BUFFER_SIZE;
	}

	current_buffer_size = advised_block_size;
	buffer = g_malloc(current_buffer_size);
	filled_bytes_in_buffer = 0;
	written_bytes_in_buffer = 0;

	while (1) {
		MateVFSResult result;
		GIOStatus io_result;
		MateVFSFileSize bytes_read;
		
	restart_toplevel_loop:
		
		g_assert(filled_bytes_in_buffer <= current_buffer_size);
		g_assert(written_bytes_in_buffer == 0);
		
		result = mate_vfs_read_cancellable (handle,
						     (char *) buffer + filled_bytes_in_buffer,
						     MIN (advised_block_size, (current_buffer_size
						     	- filled_bytes_in_buffer)),
						     &bytes_read, context);

		if (result == MATE_VFS_ERROR_INTERRUPTED) {
			continue;
		} else if (result != MATE_VFS_OK && result != MATE_VFS_ERROR_EOF) {
			goto end;
		}
	
		filled_bytes_in_buffer += bytes_read;
		
		if (filled_bytes_in_buffer == 0) {
			goto end;
		}
		
		g_assert(written_bytes_in_buffer <= filled_bytes_in_buffer);

		if (mate_vfs_context_check_cancellation(context)) {
			goto end;
		}

		while (written_bytes_in_buffer < filled_bytes_in_buffer) {
			gsize bytes_written;
			
			/* channel_out is nonblocking; if we get
			   EAGAIN (G_IO_STATUS_AGAIN) then we tried to
			   write but the pipe was full. In this case, we
			   want to enlarge our buffer and go back to
			   reading for one iteration, so we can keep
			   collecting data while the main thread is
			   busy. */
			
			io_result = g_io_channel_write_chars
				(channel_out,
				 (char *) buffer + written_bytes_in_buffer,
				 filled_bytes_in_buffer - written_bytes_in_buffer,
				 &bytes_written, NULL);
			
			written_bytes_in_buffer += bytes_written;

			if (mate_vfs_context_check_cancellation(context)) {
				goto end;
			}
			
			if (io_result == G_IO_STATUS_AGAIN) {
				/* if bytes_read == 0 then we reached
				   EOF so there's no point reading
				   again. So turn off nonblocking and
				   do a blocking write next time through. */
				if (bytes_read == 0) {
					int fd;

					fd = g_io_channel_unix_get_fd (channel_out);
					
					_mate_vfs_pipe_set_blocking (fd, TRUE);
				} else {
					if (written_bytes_in_buffer > 0) {
						/* Need to shift the unwritten bytes
						   to the start of the buffer */
						g_memmove(buffer,
							  (char *) buffer + written_bytes_in_buffer,
							  filled_bytes_in_buffer - written_bytes_in_buffer);
						filled_bytes_in_buffer =
							filled_bytes_in_buffer - written_bytes_in_buffer;
						
						written_bytes_in_buffer = 0;
					}
					
 				        /* If the buffer is more than half
					   full, double its size */
					if (filled_bytes_in_buffer * 2 > current_buffer_size) {
						current_buffer_size *= 2;
						buffer = g_realloc(buffer, current_buffer_size);
					}

					/* Leave this loop, start reading again */
					goto restart_toplevel_loop;

				} /* end of else (bytes_read != 0) */
				
			} else if (io_result != G_IO_STATUS_NORMAL || bytes_written == 0) {
				goto end;
			}
		}

		g_assert(written_bytes_in_buffer == filled_bytes_in_buffer);
		
		/* Reset, we wrote everything */
		written_bytes_in_buffer = 0;
		filled_bytes_in_buffer = 0;
	}

 end:
	g_free (buffer);
	g_io_channel_shutdown (channel_out, TRUE, NULL);
	g_io_channel_unref (channel_out);
	g_io_channel_unref (channel_in);
}

static void
serve_channel_write (MateVFSHandle *handle,
		     GIOChannel *channel_in,
		     GIOChannel *channel_out,
		     MateVFSContext *context)
{
	gchar buffer[DEFAULT_BUFFER_SIZE];
	guint buffer_size;

	buffer_size = DEFAULT_BUFFER_SIZE;

	while (1) {
		MateVFSResult result;
		GIOStatus io_result;
		gsize bytes_read;
		gsize bytes_to_write;
		MateVFSFileSize bytes_written;
		gchar *p;

		io_result = g_io_channel_read_chars (channel_in, buffer, buffer_size,
						     &bytes_read, NULL);
                
		if (io_result == G_IO_STATUS_AGAIN)
			continue;
		if (io_result != G_IO_STATUS_NORMAL || bytes_read == 0)
			goto end;

		p = buffer;
		bytes_to_write = bytes_read;
		while (bytes_to_write > 0) {
			result = mate_vfs_write_cancellable (handle,
							      p,
							      bytes_to_write,
							      &bytes_written,
							      context);
			if (result == MATE_VFS_ERROR_INTERRUPTED) {
				continue;
			}
			
			if (result != MATE_VFS_OK || bytes_written == 0) {
				goto end;
			}

			p += bytes_written;
			bytes_to_write -= bytes_written;
		}
	}

 end:
	g_io_channel_shutdown (channel_in, TRUE, NULL);
	g_io_channel_unref (channel_in);
	g_io_channel_unref (channel_out);
}

/* Job execution.  This is performed by the slave thread.  */

static void
execute_open (MateVFSJob *job)
{
	MateVFSResult result;
	MateVFSHandle *handle;
	MateVFSOpenOp *open_op;
	MateVFSNotifyResult *notify_result;

	open_op = &job->op->specifics.open;

	if (open_op->uri == NULL) {
		result = MATE_VFS_ERROR_INVALID_URI;
	} else {
		result = mate_vfs_open_uri_cancellable (&handle, open_op->uri,
							  open_op->open_mode,
							  job->op->context);
		job->handle = handle;
	}
	
	notify_result = g_new0 (MateVFSNotifyResult, 1);
	notify_result->job_handle = job->job_handle;
	notify_result->type = job->op->type;
	notify_result->specifics.open.result = result;
	notify_result->specifics.open.callback = (MateVFSAsyncOpenCallback) job->op->callback;
	notify_result->specifics.open.callback_data = job->op->callback_data;

	if (result != MATE_VFS_OK) {
		/* if the open failed, just drop the job */
		job->failed = TRUE;
	}
	
	job_oneway_notify (job, notify_result);
}

static void
execute_open_as_channel (MateVFSJob *job)
{
	MateVFSResult result;
	MateVFSHandle *handle;
	MateVFSOpenAsChannelOp *open_as_channel_op;
	MateVFSOpenMode open_mode;
	GIOChannel *channel_in, *channel_out;
	gint pipefd[2];
	MateVFSNotifyResult *notify_result;

	open_as_channel_op = &job->op->specifics.open_as_channel;

	if (open_as_channel_op->uri == NULL) {
		result = MATE_VFS_ERROR_INVALID_URI;
	} else {
		result = mate_vfs_open_uri_cancellable
			(&handle,
			 open_as_channel_op->uri,
			 open_as_channel_op->open_mode,
			 job->op->context);
	}

	notify_result = g_new0 (MateVFSNotifyResult, 1);
	notify_result->job_handle = job->job_handle;
	notify_result->type = job->op->type;
	notify_result->specifics.open_as_channel.result = result;
	notify_result->specifics.open_as_channel.callback =
		(MateVFSAsyncOpenAsChannelCallback) job->op->callback;
	notify_result->specifics.open_as_channel.callback_data = job->op->callback_data;

	if (result != MATE_VFS_OK) {
		/* if the open failed, just drop the job */
		job->failed = TRUE;
		job_oneway_notify (job, notify_result);
		return;
	}
	

	if (_mate_vfs_pipe (pipefd) < 0) {
		g_warning (_("Cannot create pipe for open GIOChannel: %s"),
			   g_strerror (errno));
		notify_result->specifics.open_as_channel.result = MATE_VFS_ERROR_INTERNAL;
		/* if the open failed, just drop the job */
		job->failed = TRUE;
		job_oneway_notify (job, notify_result);
		return;
	}

	/* Set up the pipe for nonblocking writes, so if the main
	 * thread is blocking for some reason the slave can keep
	 * reading data.
	 */
	_mate_vfs_pipe_set_blocking (pipefd[1], FALSE);
	
#ifndef G_OS_WIN32
	channel_in = g_io_channel_unix_new (pipefd[0]);
	channel_out = g_io_channel_unix_new (pipefd[1]);
#else
	channel_in = g_io_channel_win32_new_socket (pipefd[0]);
	channel_out = g_io_channel_win32_new_socket (pipefd[1]);
#endif
	open_mode = open_as_channel_op->open_mode;
	
	if (open_mode & MATE_VFS_OPEN_READ) {
		notify_result->specifics.open_as_channel.channel = channel_in;
	} else {
		notify_result->specifics.open_as_channel.channel = channel_out;
	}

	notify_result->specifics.open_as_channel.result = MATE_VFS_OK;

	job_notify (job, notify_result);

	if (open_mode & MATE_VFS_OPEN_READ) {
		serve_channel_read (handle, channel_in, channel_out,
				    open_as_channel_op->advised_block_size,
				    job->op->context);
	} else {
		serve_channel_write (handle, channel_in, channel_out,
				     job->op->context);
	}
}

static void
execute_create (MateVFSJob *job)
{
	MateVFSResult result;
	MateVFSHandle *handle;
	MateVFSCreateOp *create_op;
	MateVFSNotifyResult *notify_result;

	create_op = &job->op->specifics.create;

	if (create_op->uri == NULL) {
		result = MATE_VFS_ERROR_INVALID_URI;
	} else {
		result = mate_vfs_create_uri_cancellable
			(&handle,
			 create_op->uri,
			 create_op->open_mode,
			 create_op->exclusive,
			 create_op->perm,
			 job->op->context);
		
		job->handle = handle;
	}

	notify_result = g_new0 (MateVFSNotifyResult, 1);
	notify_result->job_handle = job->job_handle;
	notify_result->type = job->op->type;
	notify_result->specifics.create.result = result;
	notify_result->specifics.create.callback = (MateVFSAsyncCreateCallback) job->op->callback;
	notify_result->specifics.create.callback_data = job->op->callback_data;

	if (result != MATE_VFS_OK) {
		/* if the open failed, just drop the job */
		job->failed = TRUE;
	}

	job_oneway_notify (job, notify_result);
}

static void
execute_create_symbolic_link (MateVFSJob *job)
{
	MateVFSResult result;
	MateVFSCreateLinkOp *create_op;
	MateVFSNotifyResult *notify_result;

	create_op = &job->op->specifics.create_symbolic_link;

	result = mate_vfs_create_symbolic_link_cancellable
		(create_op->uri,
		 create_op->uri_reference,
		 job->op->context);

	notify_result = g_new0 (MateVFSNotifyResult, 1);
	notify_result->job_handle = job->job_handle;
	notify_result->type = job->op->type;
	notify_result->specifics.create.result = result;
	notify_result->specifics.create.callback = (MateVFSAsyncCreateCallback) job->op->callback;
	notify_result->specifics.create.callback_data = job->op->callback_data;

	if (result != MATE_VFS_OK) {
		/* if the open failed, just drop the job */
		job->failed = TRUE;
	}

	job_oneway_notify (job, notify_result);
}
	
static void
execute_create_as_channel (MateVFSJob *job)
{
	MateVFSResult result;
	MateVFSHandle *handle;
	MateVFSCreateAsChannelOp *create_as_channel_op;
	GIOChannel *channel_in, *channel_out;
	gint pipefd[2];
	MateVFSNotifyResult *notify_result;

	create_as_channel_op = &job->op->specifics.create_as_channel;

	if (create_as_channel_op->uri == NULL) {
		result = MATE_VFS_ERROR_INVALID_URI;
	} else {
		result = mate_vfs_create_uri_cancellable
			(&handle,
			 create_as_channel_op->uri,
			 create_as_channel_op->open_mode,
                         create_as_channel_op->exclusive,
                         create_as_channel_op->perm,
			 job->op->context);
	}
	
	notify_result = g_new0 (MateVFSNotifyResult, 1);
	notify_result->job_handle = job->job_handle;
	notify_result->type = job->op->type;
	notify_result->specifics.create_as_channel.result = result;
	notify_result->specifics.create_as_channel.callback = (MateVFSAsyncCreateAsChannelCallback) job->op->callback;
	notify_result->specifics.create_as_channel.callback_data = job->op->callback_data;

	if (result != MATE_VFS_OK) {
		/* if the open failed, just drop the job */
		job->failed = TRUE;
		job_oneway_notify (job, notify_result);
		return;
	}

	if (_mate_vfs_pipe (pipefd) < 0) {
		g_warning (_("Cannot create pipe for open GIOChannel: %s"),
			   g_strerror (errno));
		notify_result->specifics.create_as_channel.result = MATE_VFS_ERROR_INTERNAL;
		/* if the open failed, just drop the job */
		job->failed = TRUE;
		job_oneway_notify (job, notify_result);
		return;
	}
	
#ifndef G_OS_WIN32
	channel_in = g_io_channel_unix_new (pipefd[0]);
	channel_out = g_io_channel_unix_new (pipefd[1]);
#else
	channel_in = g_io_channel_win32_new_socket (pipefd[0]);
	channel_out = g_io_channel_win32_new_socket (pipefd[1]);
#endif

	notify_result->specifics.create_as_channel.channel = channel_out;

	job_notify (job, notify_result);

	serve_channel_write (handle, channel_in, channel_out, job->op->context);
}

static void
execute_close (MateVFSJob *job)
{
	MateVFSNotifyResult *notify_result;

	notify_result = g_new0 (MateVFSNotifyResult, 1);
	notify_result->job_handle = job->job_handle;
	notify_result->type = job->op->type;
	notify_result->specifics.close.callback = (MateVFSAsyncCloseCallback) job->op->callback;
	notify_result->specifics.close.callback_data = job->op->callback_data;
	notify_result->specifics.close.result
		= mate_vfs_close_cancellable (job->handle, job->op->context);

	job_oneway_notify (job, notify_result);
}

static void
execute_read (MateVFSJob *job)
{
	MateVFSReadOp *read_op;
	MateVFSNotifyResult *notify_result;
	
	read_op = &job->op->specifics.read;

	notify_result = g_new0 (MateVFSNotifyResult, 1);
	notify_result->job_handle = job->job_handle;
	notify_result->type = job->op->type;
	notify_result->specifics.read.callback = (MateVFSAsyncReadCallback) job->op->callback;
	notify_result->specifics.read.callback_data = job->op->callback_data;
	notify_result->specifics.read.buffer = read_op->buffer;
	notify_result->specifics.read.num_bytes = read_op->num_bytes;
	
	notify_result->specifics.read.result = mate_vfs_read_cancellable (job->handle,
									   read_op->buffer,
									   read_op->num_bytes,
									   &notify_result->specifics.read.bytes_read,
									   job->op->context);

	job->op->type = MATE_VFS_OP_READ_WRITE_DONE;

	job_oneway_notify (job, notify_result);
}

static void
execute_write (MateVFSJob *job)
{
	MateVFSWriteOp *write_op;
	MateVFSNotifyResult *notify_result;
	
	write_op = &job->op->specifics.write;

	notify_result = g_new0 (MateVFSNotifyResult, 1);
	notify_result->job_handle = job->job_handle;
	notify_result->type = job->op->type;
	notify_result->specifics.write.callback = (MateVFSAsyncWriteCallback) job->op->callback;
	notify_result->specifics.write.callback_data = job->op->callback_data;
	notify_result->specifics.write.buffer = write_op->buffer;
	notify_result->specifics.write.num_bytes = write_op->num_bytes;

	notify_result->specifics.write.result = mate_vfs_write_cancellable (job->handle,
									     write_op->buffer,
									     write_op->num_bytes,
									     &notify_result->specifics.write.bytes_written,
									     job->op->context);

	job->op->type = MATE_VFS_OP_READ_WRITE_DONE;

	job_oneway_notify (job, notify_result);
}

static void
execute_seek (MateVFSJob *job)
{
	MateVFSResult result;
	MateVFSSeekOp *seek_op;
	MateVFSNotifyResult *notify_result;

	seek_op = &job->op->specifics.seek;

	result = mate_vfs_seek_cancellable (job->handle,
					     seek_op->whence,
					     seek_op->offset,
					     job->op->context);

	notify_result = g_new0 (MateVFSNotifyResult, 1);
	notify_result->job_handle = job->job_handle;
	notify_result->type = job->op->type;
	notify_result->specifics.seek.result = result;
	notify_result->specifics.seek.callback = (MateVFSAsyncSeekCallback) job->op->callback;
	notify_result->specifics.seek.callback_data = job->op->callback_data;

	job_oneway_notify (job, notify_result);
}

static void
execute_get_file_info (MateVFSJob *job)
{
	MateVFSGetFileInfoOp *get_file_info_op;
	GList *p;
	MateVFSGetFileInfoResult *result_item;
	MateVFSNotifyResult *notify_result;

	get_file_info_op = &job->op->specifics.get_file_info;

	notify_result = g_new0 (MateVFSNotifyResult, 1);
	notify_result->job_handle = job->job_handle;
	notify_result->type = job->op->type;
	notify_result->specifics.get_file_info.callback =
		(MateVFSAsyncGetFileInfoCallback) job->op->callback;
	notify_result->specifics.get_file_info.callback_data = job->op->callback_data;

	for (p = get_file_info_op->uris; p != NULL; p = p->next) {
		result_item = g_new (MateVFSGetFileInfoResult, 1);

		result_item->uri = mate_vfs_uri_ref (p->data);
		result_item->file_info = mate_vfs_file_info_new ();

		result_item->result = mate_vfs_get_file_info_uri_cancellable
			(result_item->uri,
			 result_item->file_info,
			 get_file_info_op->options,
			 job->op->context);

		notify_result->specifics.get_file_info.result_list =
			g_list_prepend (notify_result->specifics.get_file_info.result_list, result_item);
	}
	notify_result->specifics.get_file_info.result_list =
		g_list_reverse (notify_result->specifics.get_file_info.result_list);

	job_oneway_notify (job, notify_result);
}

static void
execute_set_file_info (MateVFSJob *job)
{
	MateVFSSetFileInfoOp *set_file_info_op;
	MateVFSURI *parent_uri, *uri_after;
	MateVFSNotifyResult *notify_result;

	set_file_info_op = &job->op->specifics.set_file_info;

	notify_result = g_new0 (MateVFSNotifyResult, 1);
	notify_result->job_handle = job->job_handle;
	notify_result->type = job->op->type;
	notify_result->specifics.set_file_info.callback =
		(MateVFSAsyncSetFileInfoCallback) job->op->callback;
	notify_result->specifics.set_file_info.callback_data =
		job->op->callback_data;

	notify_result->specifics.set_file_info.set_file_info_result =
		mate_vfs_set_file_info_cancellable (set_file_info_op->uri,
			set_file_info_op->info, set_file_info_op->mask,
		 	job->op->context);

	/* Get the new URI after the set_file_info. The name may have
	 * changed.
	 */
	uri_after = NULL;
	if (notify_result->specifics.set_file_info.set_file_info_result == MATE_VFS_OK
	    && (set_file_info_op->mask & MATE_VFS_SET_FILE_INFO_NAME) != 0) {
		parent_uri = mate_vfs_uri_get_parent (set_file_info_op->uri);
		if (parent_uri != NULL) {
			uri_after = mate_vfs_uri_append_file_name
				(parent_uri, set_file_info_op->info->name);
			mate_vfs_uri_unref (parent_uri);
		}
	}
	if (uri_after == NULL) {
		uri_after = set_file_info_op->uri;
		mate_vfs_uri_ref (uri_after);
	}

	notify_result->specifics.set_file_info.info = mate_vfs_file_info_new ();
	if (uri_after == NULL) {
		notify_result->specifics.set_file_info.get_file_info_result
			= MATE_VFS_ERROR_INVALID_URI;
	} else {
		notify_result->specifics.set_file_info.get_file_info_result
			= mate_vfs_get_file_info_uri_cancellable
			(uri_after,
			 notify_result->specifics.set_file_info.info,
			 set_file_info_op->options,
			 job->op->context);
		mate_vfs_uri_unref (uri_after);
	}

	job_oneway_notify (job, notify_result);
}

static void
execute_find_directory (MateVFSJob *job)
{
	MateVFSFindDirectoryOp *find_directory_op;
	GList *p;
	MateVFSFindDirectoryResult *result_item;
	MateVFSNotifyResult *notify_result;

	notify_result = g_new0 (MateVFSNotifyResult, 1);
	notify_result->job_handle = job->job_handle;
	notify_result->type = job->op->type;
	notify_result->specifics.find_directory.callback
		= (MateVFSAsyncFindDirectoryCallback) job->op->callback;
	notify_result->specifics.find_directory.callback_data = job->op->callback_data;

	find_directory_op = &job->op->specifics.find_directory;
	for (p = find_directory_op->uris; p != NULL; p = p->next) {
		result_item = g_new0 (MateVFSFindDirectoryResult, 1);

		result_item->result = mate_vfs_find_directory_cancellable
			((MateVFSURI *) p->data,
			 find_directory_op->kind,
			 &result_item->uri,
			 find_directory_op->create_if_needed,
			 find_directory_op->find_if_needed,
			 find_directory_op->permissions,
			 job->op->context);
		notify_result->specifics.find_directory.result_list =
			g_list_prepend (notify_result->specifics.find_directory.result_list, result_item);
	}

	notify_result->specifics.find_directory.result_list =
		g_list_reverse (notify_result->specifics.find_directory.result_list);
	
	job_oneway_notify (job, notify_result);
}

static void
load_directory_details (MateVFSJob *job)
{
	MateVFSLoadDirectoryOp *load_directory_op;
	MateVFSDirectoryHandle *handle;
	GList *directory_list;
	MateVFSFileInfo *info;
	MateVFSResult result;
	guint count;
	MateVFSNotifyResult *notify_result;

	JOB_DEBUG (("%u", GPOINTER_TO_UINT (job->job_handle)));
	load_directory_op = &job->op->specifics.load_directory;
	
	if (load_directory_op->uri == NULL) {
		result = MATE_VFS_ERROR_INVALID_URI;
	} else {
		result = mate_vfs_directory_open_from_uri_cancellable
			(&handle,
			 load_directory_op->uri,
			 load_directory_op->options,
			 job->op->context);
	}

	if (result != MATE_VFS_OK) {
		notify_result = g_new0 (MateVFSNotifyResult, 1);
		notify_result->job_handle = job->job_handle;
		notify_result->type = job->op->type;
		notify_result->specifics.load_directory.result = result;
		notify_result->specifics.load_directory.callback =
			(MateVFSAsyncDirectoryLoadCallback) job->op->callback;
		notify_result->specifics.load_directory.callback_data = job->op->callback_data;
		job_oneway_notify (job, notify_result);
		return;
	}

	directory_list = NULL;

	count = 0;
	while (1) {
		if (mate_vfs_context_check_cancellation (job->op->context)) {
			JOB_DEBUG (("cancelled, bailing %u",
				    GPOINTER_TO_UINT (job->job_handle)));
			mate_vfs_file_info_list_free (directory_list);
			directory_list = NULL;
			result = MATE_VFS_ERROR_CANCELLED;
			break;
		}

		info = mate_vfs_file_info_new ();

		result = mate_vfs_directory_read_next_cancellable
			(handle, info, job->op->context);

		if (result == MATE_VFS_OK) {
			directory_list = g_list_prepend (directory_list, info);
			count++;
		} else {
			mate_vfs_file_info_unref (info);
		}

		if (count == load_directory_op->items_per_notification
			|| result != MATE_VFS_OK) {

			notify_result = g_new0 (MateVFSNotifyResult, 1);
			notify_result->job_handle = job->job_handle;
			notify_result->type = job->op->type;
			notify_result->specifics.load_directory.result = result;
			notify_result->specifics.load_directory.entries_read = count;
			notify_result->specifics.load_directory.list = 
				g_list_reverse (directory_list);
			notify_result->specifics.load_directory.callback =
				(MateVFSAsyncDirectoryLoadCallback) job->op->callback;
			notify_result->specifics.load_directory.callback_data =
				job->op->callback_data;

			job_oneway_notify (job, notify_result);

			count = 0;
			directory_list = NULL;

			if (result != MATE_VFS_OK) {
				break;
			}
		}
	}

	g_assert (directory_list == NULL);
	mate_vfs_directory_close (handle);
}

static void
execute_load_directory (MateVFSJob *job)
{
	load_directory_details (job);
}

static gint
xfer_callback (MateVFSXferProgressInfo *info,
	       gpointer data)
{
	MateVFSJob *job;
	MateVFSNotifyResult notify_result;

	job = (MateVFSJob *) data;

	/* xfer is fully synchronous, just allocate the notify result struct on the stack */
	notify_result.job_handle = job->job_handle;
	notify_result.callback_id = 0;
	notify_result.cancelled = FALSE;
	notify_result.type = job->op->type;
	notify_result.specifics.xfer.progress_info = info;
	notify_result.specifics.xfer.callback = (MateVFSAsyncXferProgressCallback) job->op->callback;
	notify_result.specifics.xfer.callback_data = job->op->callback_data;

	job_notify (job, &notify_result);

	/* Pass the value returned from the callback in the master thread.  */
	return notify_result.specifics.xfer.reply;
}

static void
execute_xfer (MateVFSJob *job)
{
	MateVFSXferOp *xfer_op;
	MateVFSResult result;
	MateVFSXferProgressInfo info;
	MateVFSNotifyResult notify_result;

	xfer_op = &job->op->specifics.xfer;

	result = _mate_vfs_xfer_private (xfer_op->source_uri_list,
					 xfer_op->target_uri_list,
					 xfer_op->xfer_options,
					 xfer_op->error_mode,
					 xfer_op->overwrite_mode,
					 xfer_callback,
					 job,
					 xfer_op->progress_sync_callback,
					 xfer_op->sync_callback_data);

	/* If the xfer functions returns an error now, something really bad
         * must have happened.
         */
	if (result != MATE_VFS_OK && result != MATE_VFS_ERROR_INTERRUPTED) {

		info.status = MATE_VFS_XFER_PROGRESS_STATUS_VFSERROR;
		info.vfs_status = result;
		info.phase = MATE_VFS_XFER_PHASE_INITIAL;
		info.source_name = NULL;
		info.target_name = NULL;
		info.file_index = 0;
		info.files_total = 0;
		info.bytes_total = 0;
		info.file_size = 0;
		info.bytes_copied = 0;
		info.total_bytes_copied = 0;

		notify_result.job_handle = job->job_handle;
		notify_result.callback_id = 0;
		notify_result.cancelled = FALSE;
		notify_result.type = job->op->type;
		notify_result.specifics.xfer.progress_info = &info;
		notify_result.specifics.xfer.callback = (MateVFSAsyncXferProgressCallback) job->op->callback;
		notify_result.specifics.xfer.callback_data = job->op->callback_data;

		job_notify (job, &notify_result);
	}
}

static void
execute_file_control (MateVFSJob *job)
{
	MateVFSFileControlOp *file_control_op;
	MateVFSNotifyResult *notify_result;
	
	file_control_op = &job->op->specifics.file_control;

	notify_result = g_new0 (MateVFSNotifyResult, 1);
	notify_result->job_handle = job->job_handle;
	notify_result->type = job->op->type;
	notify_result->specifics.file_control.callback = (MateVFSAsyncFileControlCallback) job->op->callback;
	notify_result->specifics.file_control.callback_data = job->op->callback_data;
	notify_result->specifics.file_control.operation_data = file_control_op->operation_data;
	notify_result->specifics.file_control.operation_data_destroy_func = file_control_op->operation_data_destroy_func;
	
	notify_result->specifics.file_control.result = mate_vfs_file_control_cancellable (job->handle,
											   file_control_op->operation,
											   file_control_op->operation_data,
											   job->op->context);

	job->op->type = MATE_VFS_OP_FILE_CONTROL;

	job_oneway_notify (job, notify_result);
}


/*
 * _mate_vfs_job_execute:
 * @job: the job to execute
 * 
 *   This function is called by the slave thread to execute
 * the job - all work performed by a thread starts here.
 */
void
_mate_vfs_job_execute (MateVFSJob *job)
{
	guint id;

	id = GPOINTER_TO_UINT (job->job_handle);

	JOB_DEBUG (("exec job %u", id));

	if (!job->cancelled) {
		set_current_job (job);

		JOB_DEBUG (("executing %u %d type %s", id, job->op->type,
			    JOB_DEBUG_TYPE (job->op->type)));

		switch (job->op->type) {
		case MATE_VFS_OP_OPEN:
			execute_open (job);
			break;
		case MATE_VFS_OP_OPEN_AS_CHANNEL:
			execute_open_as_channel (job);
			break;
		case MATE_VFS_OP_CREATE:
			execute_create (job);
			break;
		case MATE_VFS_OP_CREATE_AS_CHANNEL:
			execute_create_as_channel (job);
			break;
		case MATE_VFS_OP_CREATE_SYMBOLIC_LINK:
			execute_create_symbolic_link (job);
			break;
		case MATE_VFS_OP_CLOSE:
			execute_close (job);
			break;
		case MATE_VFS_OP_READ:
			execute_read (job);
			break;
		case MATE_VFS_OP_WRITE:
			execute_write (job);
			break;
		case MATE_VFS_OP_SEEK:
			execute_seek (job);
			break;
		case MATE_VFS_OP_LOAD_DIRECTORY:
			execute_load_directory (job);
			break;
		case MATE_VFS_OP_FIND_DIRECTORY:
			execute_find_directory (job);
			break;
		case MATE_VFS_OP_XFER:
			execute_xfer (job);
			break;
		case MATE_VFS_OP_GET_FILE_INFO:
			execute_get_file_info (job);
			break;
		case MATE_VFS_OP_SET_FILE_INFO:
			execute_set_file_info (job);
			break;
		case MATE_VFS_OP_FILE_CONTROL:
			execute_file_control (job);
			break;
		default:
			g_warning (_("Unknown job kind %u"), job->op->type);
			break;
		}
		/* NB. 'job' is quite probably invalid now */
		clear_current_job ();
	} else {
		switch (job->op->type) {
		case MATE_VFS_OP_READ:
		case MATE_VFS_OP_WRITE:
			job->op->type = MATE_VFS_OP_READ_WRITE_DONE;
			break;
		default:
			break;
		}
	}
	
	JOB_DEBUG (("done job %u", id));
}

void
_mate_vfs_job_module_cancel (MateVFSJob *job)
{
	MateVFSCancellation *cancellation;

	JOB_DEBUG (("%u", GPOINTER_TO_UINT (job->job_handle)));
	
	cancellation = mate_vfs_context_get_cancellation (job->op->context);
	if (cancellation != NULL) {
		JOB_DEBUG (("cancelling %u", GPOINTER_TO_UINT (job->job_handle)));
		mate_vfs_cancellation_cancel (cancellation);
	}

#ifdef OLD_CONTEXT_DEPRECATED	
	mate_vfs_context_emit_message (job->op->context, _("Operation stopped"));
#endif /* OLD_CONTEXT_DEPRECATED */

	/* Since we are cancelling, we won't have anyone respond to notifications;
	 * set the expectations right.
	 */
	JOB_DEBUG (("done %u", GPOINTER_TO_UINT (job->job_handle)));
}

static void
set_current_job (MateVFSJob *job)
{
	/* There shouldn't have been anything here. */
	g_assert (g_static_private_get (&job_private) == NULL);

	g_static_private_set (&job_private, job, NULL);

	_mate_vfs_module_callback_use_stack_info (job->op->stack_info);
	_mate_vfs_module_callback_set_in_async_thread (TRUE);
}

static void
clear_current_job (void)
{
	g_static_private_set (&job_private, NULL, NULL);

	_mate_vfs_module_callback_clear_stacks ();
}

void
_mate_vfs_get_current_context (MateVFSContext **context)
{
	MateVFSJob *job;
	
	g_return_if_fail (context != NULL);

	job = g_static_private_get (&job_private);

	if (job != NULL) {
		*context = job->op->context;
	} else {
		*context = NULL;
	}
}

void
_mate_vfs_dispatch_module_callback (MateVFSAsyncModuleCallback callback,
				    gconstpointer in, gsize in_size,
				    gpointer out, gsize out_size,
				    gpointer user_data,
				    MateVFSModuleCallbackResponse response,
				    gpointer response_data)
{
	MateVFSJob *job;
	MateVFSNotifyResult notify_result;

	job = g_static_private_get (&job_private);

	g_return_if_fail (job != NULL);

	memset (&notify_result, 0, sizeof (notify_result));

	notify_result.job_handle = job->job_handle;

	notify_result.type = MATE_VFS_OP_MODULE_CALLBACK;

	notify_result.specifics.callback.callback 	= callback;
	notify_result.specifics.callback.user_data 	= user_data;
	notify_result.specifics.callback.in 		= in;
	notify_result.specifics.callback.in_size 	= in_size;
	notify_result.specifics.callback.out 		= out;
	notify_result.specifics.callback.out_size 	= out_size;
	notify_result.specifics.callback.out 		= out;
	notify_result.specifics.callback.out_size 	= out_size;
	notify_result.specifics.callback.response	= response;
	notify_result.specifics.callback.response_data 	= response_data;

	job_notify (job, &notify_result);
}
