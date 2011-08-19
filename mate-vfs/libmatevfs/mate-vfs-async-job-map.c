/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-async-job-map.c

   Copyright (C) 2001 Eazel Inc.

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

   Author: Pavel Cisler <pavel@eazel.com> */

#include <config.h>
#include "mate-vfs-async-job-map.h"

#include "mate-vfs-job.h"
#include <glib.h>

/* job map bits guarded by this lock */
static GStaticRecMutex async_job_map_lock = G_STATIC_REC_MUTEX_INIT;
static guint async_job_map_next_id;
static int async_job_map_locked = 0;
static gboolean async_job_map_shutting_down;
static GHashTable *async_job_map;

/* callback map bits guarded by this lock */
static GStaticMutex async_job_callback_map_lock = G_STATIC_MUTEX_INIT;
static GHashTable *async_job_callback_map;
static guint async_job_callback_map_next_id;

void async_job_callback_map_destroy (void);

void 
_mate_vfs_async_job_map_init (void)
{
}

MateVFSJob *
_mate_vfs_async_job_map_get_job (const MateVFSAsyncHandle *handle)
{
	_mate_vfs_async_job_map_assert_locked ();
	
	if (async_job_map == NULL) {
		/* This can happen if cancellations are run after
		 * shutdown or before any job is run. This is just
		 * a special case of there handle not being in the
		 * hash */
		return NULL;
	}

	return g_hash_table_lookup (async_job_map, handle);
}

void
_mate_vfs_async_job_map_add_job (MateVFSJob *job)
{
	_mate_vfs_async_job_map_lock ();

	g_assert (!async_job_map_shutting_down);

	/* Assign a unique id to each job. The MateVFSAsyncHandle pointers each
	 * async op call deals with this will really be these unique IDs
	 */
	job->job_handle = GUINT_TO_POINTER (++async_job_map_next_id);

	if (async_job_map == NULL) {
		/* First job, allocate a new hash table. */
		async_job_map = g_hash_table_new (NULL, NULL);
	}

	g_hash_table_insert (async_job_map, job->job_handle, job);

	_mate_vfs_async_job_map_unlock ();
}

void
_mate_vfs_async_job_map_remove_job (MateVFSJob *job)
{
	_mate_vfs_async_job_map_lock ();
	
	g_assert (async_job_map);

	g_hash_table_remove (async_job_map, job->job_handle);
	
	_mate_vfs_async_job_map_unlock ();
}


static void
mate_vfs_async_job_map_destroy (void)
{
	_mate_vfs_async_job_map_assert_locked ();
	g_assert (async_job_map_shutting_down);
	g_assert (async_job_map != NULL);
	
	g_hash_table_destroy (async_job_map);
	async_job_map = NULL;
}

gboolean 
_mate_vfs_async_job_completed (MateVFSAsyncHandle *handle)
{
	MateVFSJob *job;

	_mate_vfs_async_job_map_lock ();

	JOB_DEBUG (("%d", GPOINTER_TO_UINT (handle)));
	/* Job done, remove it's id from the map */

	g_assert (async_job_map != NULL);

	job = _mate_vfs_async_job_map_get_job (handle);
	if (job != NULL) {
		g_hash_table_remove (async_job_map, handle);
	}
	
	if (async_job_map_shutting_down && g_hash_table_size (async_job_map) == 0) {
		/* We were the last active job, turn the lights off. */
		mate_vfs_async_job_map_destroy ();
	}
	
	_mate_vfs_async_job_map_unlock ();
	
	return job != NULL;
}

void
_mate_vfs_async_job_map_shutdown (void)
{
	_mate_vfs_async_job_map_lock ();
	
	if (async_job_map) {

		/* tell the async jobs it's quitting time */
		async_job_map_shutting_down = TRUE;

		if (g_hash_table_size (async_job_map) == 0) {
			/* No more outstanding jobs to finish, just delete
			 * the hash table directly.
			 */
			mate_vfs_async_job_map_destroy ();
		}
	}

	/* The last expiring job will delete the hash table. */
	_mate_vfs_async_job_map_unlock ();
	
	async_job_callback_map_destroy ();
}

void 
_mate_vfs_async_job_map_lock (void)
{
	g_static_rec_mutex_lock (&async_job_map_lock);
	async_job_map_locked++;
}

void 
_mate_vfs_async_job_map_unlock (void)
{
	async_job_map_locked--;
	g_static_rec_mutex_unlock (&async_job_map_lock);
}

void 
_mate_vfs_async_job_map_assert_locked (void)
{
	g_assert (async_job_map_locked);
}

void 
_mate_vfs_async_job_callback_valid (guint     callback_id,
				    gboolean *valid,
				    gboolean *cancelled)
{
	MateVFSNotifyResult *notify_result;
	
	g_static_mutex_lock (&async_job_callback_map_lock);
	
	if (async_job_callback_map == NULL) {
		g_assert (async_job_map_shutting_down);
		*valid = FALSE;
		*cancelled = FALSE;
	}

	notify_result = (MateVFSNotifyResult *) g_hash_table_lookup
		(async_job_callback_map, GUINT_TO_POINTER (callback_id));
	
	*valid = notify_result != NULL;
	*cancelled = notify_result != NULL && notify_result->cancelled;

	g_static_mutex_unlock (&async_job_callback_map_lock);
}

gboolean 
_mate_vfs_async_job_add_callback (MateVFSJob *job, MateVFSNotifyResult *notify_result)
{
	gboolean cancelled;

	g_static_mutex_lock (&async_job_callback_map_lock);

	g_assert (!async_job_map_shutting_down);
	
	/* Assign a unique id to each job callback. Use unique IDs instead of the
	 * notify_results pointers to avoid aliasing problems.
	 */
	notify_result->callback_id = ++async_job_callback_map_next_id;

	JOB_DEBUG (("adding callback %d ", notify_result->callback_id));

	if (async_job_callback_map == NULL) {
		/* First job, allocate a new hash table. */
		async_job_callback_map = g_hash_table_new (NULL, NULL);
	}

	/* we are using async_job_callback_map_lock to ensure atomicity of
	 * checking/clearing job->cancelled and adding/cancelling callbacks
	 */
	cancelled = job->cancelled;
	
	if (!cancelled) {
		g_hash_table_insert (async_job_callback_map, GUINT_TO_POINTER (notify_result->callback_id),
			notify_result);
	}
	g_static_mutex_unlock (&async_job_callback_map_lock);
	
	return !cancelled;
}

void 
_mate_vfs_async_job_remove_callback (guint callback_id)
{
	g_assert (async_job_callback_map != NULL);

	JOB_DEBUG (("removing callback %d ", callback_id));
	g_static_mutex_lock (&async_job_callback_map_lock);

	g_hash_table_remove (async_job_callback_map, GUINT_TO_POINTER (callback_id));

	g_static_mutex_unlock (&async_job_callback_map_lock);
}

static void
callback_map_cancel_one (gpointer key, gpointer value, gpointer user_data)
{
	MateVFSNotifyResult *notify_result;
	
	notify_result = (MateVFSNotifyResult *) value;
	
	if (notify_result->job_handle == (MateVFSAsyncHandle *)user_data) {
		JOB_DEBUG (("cancelling callback %u - job %u cancelled",
			    GPOINTER_TO_UINT (key),
			    GPOINTER_TO_UINT (user_data)));
		notify_result->cancelled = TRUE;
	}
}

void
_mate_vfs_async_job_cancel_job_and_callbacks (MateVFSAsyncHandle *job_handle, MateVFSJob *job)
{
	g_static_mutex_lock (&async_job_callback_map_lock);
	
	if (job != NULL) {
		job->cancelled = TRUE;
	}
	
	if (async_job_callback_map == NULL) {
		JOB_DEBUG (("job %u, no callbacks scheduled yet",
			    GPOINTER_TO_UINT (job_handle)));
	} else {
		g_hash_table_foreach (async_job_callback_map,
				      callback_map_cancel_one, job_handle);
	}

	g_static_mutex_unlock (&async_job_callback_map_lock);
}

void
async_job_callback_map_destroy (void)
{
	g_static_mutex_lock (&async_job_callback_map_lock);

	if (async_job_callback_map) {
		g_hash_table_destroy (async_job_callback_map);
		async_job_callback_map = NULL;
	}

	g_static_mutex_unlock (&async_job_callback_map_lock);
}
