/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-job-queue.c - Job queue for asynchronous MateVFSJobs
   
   Copyright (C) 2005 Christian Kellner
   
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

   Author: Christian Kellner <gicmo@gnome.org>
*/

#include <config.h>
#include "mate-vfs-job-queue.h"
#include "mate-vfs-async-job-map.h"
#include <libmatevfs/mate-vfs-job-limit.h>

#ifndef DEFAULT_THREAD_COUNT_LIMIT
#define DEFAULT_THREAD_COUNT_LIMIT 10
#endif

#ifndef MIN_THREADS
#define MIN_THREADS 2
#endif

static GThreadPool *thread_pool = NULL;

static volatile gboolean mate_vfs_quitting = FALSE;

static void
thread_entry_point (gpointer data, gpointer user_data)
{
	MateVFSJob *job;
	gboolean complete;

	job = (MateVFSJob *) data;
	/* job map must always be locked before the job_lock
	 * if both locks are needed */
	_mate_vfs_async_job_map_lock ();
	
	if (_mate_vfs_async_job_map_get_job (job->job_handle) == NULL) {
		JOB_DEBUG (("job already dead, bail %p",
			    job->job_handle));
		_mate_vfs_async_job_map_unlock ();

		/* FIXME: doesn't that leak here? */
		return;
	}
	
	JOB_DEBUG (("locking job_lock %p", job->job_handle));
	g_mutex_lock (job->job_lock);
	_mate_vfs_async_job_map_unlock ();

	_mate_vfs_job_execute (job);
	complete = _mate_vfs_job_complete (job);
	
	JOB_DEBUG (("Unlocking access lock %p", job->job_handle));
	g_mutex_unlock (job->job_lock);

	if (complete) {
		_mate_vfs_async_job_map_lock ();
		JOB_DEBUG (("job %p done, removing from map and destroying", 
			    job->job_handle));
		_mate_vfs_async_job_completed (job->job_handle);
		_mate_vfs_job_destroy (job);
		_mate_vfs_async_job_map_unlock ();
	}
}

static gint
prioritize_threads (gconstpointer a,
		    gconstpointer b,
		    gpointer      user_data)
{
	MateVFSJob *job_a;
	MateVFSJob *job_b;
	int          prio_a;
	int          prio_b;
	int          retval;
	
	job_a = (MateVFSJob *) a;
	job_b = (MateVFSJob *) b;

	prio_a = job_a->priority;
	prio_b = job_b->priority;

	/* From glib gtk-doc:
	 * 
	 * a negative value if the first task should be processed 
	 * before the second or a positive value if the 
	 * second task should be processed first. 
	 *
	 */
	
	if (prio_a > prio_b) {
		return -1;
	} else if (prio_a < prio_b) {
		return 1;
	}

	/* Since job_handles are just increasing u-ints
	 * we return a negative value if job_a->job_handle >
	 * job_b->job_handle so we have sort the old job
	 * before the newer one  */
	retval = GPOINTER_TO_UINT (job_a->job_handle) -
		 GPOINTER_TO_UINT (job_b->job_handle);

	return retval;
}

void
_mate_vfs_job_queue_init (void)
{
	GError *err = NULL;

	thread_pool = g_thread_pool_new (thread_entry_point,
					 NULL,
					 DEFAULT_THREAD_COUNT_LIMIT,
					 FALSE,
					 &err);

	if (G_UNLIKELY (thread_pool == NULL)) {
		g_error ("Could not create threadpool: %s",
			 err->message);
	}
	
	g_thread_pool_set_sort_function (thread_pool,
					 prioritize_threads,
					 NULL);
}


gboolean
_mate_vfs_job_schedule (MateVFSJob *job)
{
	GError *err = NULL;
	
	if (G_UNLIKELY (mate_vfs_quitting)) {
		/* The application is quitting, the threadpool might already
		 * be dead, just return FALSE 
		 * We are also not calling _mate_vfs_async_job_completed 
		 * because the job map might also be dead */
		g_warning ("Starting of MateVFS async calls after quit.");
		return FALSE;
	}

	g_thread_pool_push (thread_pool, job, &err);

	if (G_UNLIKELY (err != NULL)) {
		g_warning ("Could not push thread %s into pool\n",
			   err->message);

		/* thread did not start up, remove the job from the hash table */
		_mate_vfs_async_job_completed (job->job_handle);
		
		return FALSE;
	}

	return TRUE;	
}

/**
 * mate_vfs_async_set_job_limit:
 * @limit: maximum number of allowable threads.
 *
 * Restrict the number of worker threads used by async operations
 * to @limit.
 */
void
mate_vfs_async_set_job_limit (int limit)
{
	if (limit < MIN_THREADS) {
		g_warning ("Attempt to set the thread_count_limit below %d", 
			   MIN_THREADS);
		return;
	}

	g_thread_pool_set_max_threads (thread_pool, limit, NULL);
}

/**
 * mate_vfs_async_get_job_limit:
 * 
 * Get the current maximum allowable number of
 * worker threads for async operations.
 *
 * Return value: current maximum number of threads.
 */
int
mate_vfs_async_get_job_limit (void)
{
	return g_thread_pool_get_max_threads (thread_pool);
}

void
_mate_vfs_job_queue_shutdown (void)
{
	g_thread_pool_free (thread_pool, FALSE, FALSE);

	mate_vfs_quitting = TRUE;

	while (mate_vfs_job_get_count () != 0) {
		
		g_main_context_iteration (NULL, FALSE);
		g_usleep (20000);

	}

	_mate_vfs_async_job_map_shutdown ();
}

