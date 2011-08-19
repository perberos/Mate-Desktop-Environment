/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* mate-vfs-xfer.c - File transfers in the MATE Virtual File System.

   Copyright (C) 1999 Free Software Foundation
   Copyright (C) 2000, 2001 Eazel, Inc.

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
   Ettore Perazzoli <ettore@comm2000.it> 
   Pavel Cisler <pavel@eazel.com> 
*/

/* FIME bugzilla.eazel.com 1197: 
   Check that all the flags passed by address are set at least once by
   functions that are expected to set them.  */
/* FIXME bugzilla.eazel.com 1198: 
   There should be a concept of a `context' in the file methods that would
   allow us to set a prefix URI for all the operations.  This way we can
   greatly optimize access to "strange" file systems.  */

#include <config.h>
#include "mate-vfs-xfer.h"

#include "mate-vfs-cancellable-ops.h"
#include "mate-vfs-directory.h"
#include "mate-vfs-ops.h"
#include "mate-vfs-utils.h"
#include "mate-vfs-private-utils.h"
#include <glib.h>
#include <string.h>
#include <sys/time.h>

/* Implementation of file transfers (`mate_vfs_xfer*()' functions).  */

static MateVFSResult 	remove_directory 	(MateVFSURI *uri,
						 gboolean recursive,
						 MateVFSProgressCallbackState *progress,
						 MateVFSXferOptions xfer_options,
						 MateVFSXferErrorMode *error_mode,
						 gboolean *skip);


enum {
	/* size used for accounting for the expense of copying directories, 
	 * doing a move operation, etc. in a progress callback
	 * (during a move the file size is used for a regular file).
	 */
	DEFAULT_SIZE_OVERHEAD = 1024
};

/* When the total copy size is over this limit, we try to
 * forget the data cached for the files, in order to
 * lower VM pressure
 */
#define DROP_CACHE_SIZE_LIMIT (20*1024*1024)

/* Frequent fadvise() calls significantly degrade performance
 * on some file systems (e.g. XFS), so we drop at least this
 * amount of bytes at once. */
#define DROP_CACHE_BATCH_SIZE (512 * 1024)

/* in asynch mode the progress callback does a context switch every time
 * it gets called. We'll only call it every now and then to not loose a
 * lot of performance
 */
#define UPDATE_PERIOD ((gint64) (500 * 1000))

static gint64
system_time (void)
{
	GTimeVal time_of_day;

	g_get_current_time (&time_of_day);
	return (gint64) time_of_day.tv_usec + ((gint64) time_of_day.tv_sec) * 1000000;
}

static void
init_progress (MateVFSProgressCallbackState *progress_state,
	       MateVFSXferProgressInfo *progress_info)
{
	progress_info->source_name = NULL;
	progress_info->target_name = NULL;
	progress_info->status = MATE_VFS_XFER_PROGRESS_STATUS_OK;
	progress_info->vfs_status = MATE_VFS_OK;
	progress_info->phase = MATE_VFS_XFER_PHASE_INITIAL;
	progress_info->file_index = 0;
	progress_info->files_total = 0;
	progress_info->bytes_total = 0;
	progress_info->file_size = 0;
	progress_info->bytes_copied = 0;
	progress_info->total_bytes_copied = 0;
	progress_info->duplicate_name = NULL;
	progress_info->duplicate_count = 0;
	progress_info->top_level_item = FALSE;

	progress_state->progress_info = progress_info;
	progress_state->sync_callback = NULL;
	progress_state->update_callback = NULL;
	progress_state->async_job_data = NULL;
	progress_state->next_update_callback_time = 0;
	progress_state->next_text_update_callback_time = 0;
	progress_state->update_callback_period = UPDATE_PERIOD;
}

static void
free_progress (MateVFSXferProgressInfo *progress_info)
{
	g_free (progress_info->source_name);
	progress_info->source_name = NULL;
	g_free (progress_info->target_name);
	progress_info->target_name = NULL;
	g_free (progress_info->duplicate_name);
	progress_info->duplicate_name = NULL;
}

static void
progress_set_source_target_uris (MateVFSProgressCallbackState *progress, 
	      const MateVFSURI *source_uri, const MateVFSURI *dest_uri)
{
	g_free (progress->progress_info->source_name);
	progress->progress_info->source_name = source_uri ? mate_vfs_uri_to_string (source_uri,
						       MATE_VFS_URI_HIDE_NONE) : NULL;
	g_free (progress->progress_info->target_name);
	progress->progress_info->target_name = dest_uri ? mate_vfs_uri_to_string (dest_uri,
						       MATE_VFS_URI_HIDE_NONE) : NULL;

}

static void
progress_set_source_target (MateVFSProgressCallbackState *progress, 
	      const char *source, const char *dest)
{
	g_free (progress->progress_info->source_name);
	progress->progress_info->source_name = source ? g_strdup (source) : NULL;
	g_free (progress->progress_info->target_name);
	progress->progress_info->target_name = dest ? g_strdup (dest) : NULL;

}

static int
call_progress (MateVFSProgressCallbackState *progress, MateVFSXferPhase phase)
{
	int result;

	/* FIXME bugzilla.eazel.com 1199: should use proper progress result values from an enum here */

	result = 0;
	progress_set_source_target_uris (progress, NULL, NULL);

	progress->next_update_callback_time = system_time () + progress->update_callback_period;
	
	progress->progress_info->phase = phase;

	if (progress->sync_callback != NULL)
		result = (* progress->sync_callback) (progress->progress_info, progress->user_data);

	if (progress->update_callback != NULL) {
		result = (* progress->update_callback) (progress->progress_info, 
			progress->async_job_data);
	}

	return result;	
}

static MateVFSXferErrorAction
call_progress_with_current_names (MateVFSProgressCallbackState *progress, MateVFSXferPhase phase)
{
	int result;

	result = MATE_VFS_XFER_ERROR_ACTION_ABORT;

	progress->next_text_update_callback_time = system_time () + progress->update_callback_period;
	progress->next_update_callback_time = progress->next_text_update_callback_time;
	
	progress->progress_info->phase = phase;

	if (progress->sync_callback != NULL) {
		result = (* progress->sync_callback) (progress->progress_info, progress->user_data);
	}
	
	if (progress->update_callback != NULL) {
		result = (* progress->update_callback) (progress->progress_info, 
			progress->async_job_data);
	}

	return result;	
}

static int
call_progress_uri (MateVFSProgressCallbackState *progress, 
		   const MateVFSURI *source_uri, const MateVFSURI *dest_uri, 
		   MateVFSXferPhase phase)
{
	int result;

	result = 0;
	progress_set_source_target_uris (progress, source_uri, dest_uri);

	progress->next_text_update_callback_time = system_time () + progress->update_callback_period;
	progress->next_update_callback_time = progress->next_text_update_callback_time;
	
	progress->progress_info->phase = phase;

	if (progress->sync_callback != NULL) {
		result = (* progress->sync_callback) (progress->progress_info, progress->user_data);
	}
	
	if (progress->update_callback != NULL) {
		result = (* progress->update_callback) (progress->progress_info,
			progress->async_job_data);
	}
	
	return result;	
}

static gboolean
at_end (MateVFSProgressCallbackState *progress)
{
	return progress->progress_info->total_bytes_copied
		>= progress->progress_info->bytes_total;
}

static int
call_progress_often_internal (MateVFSProgressCallbackState *progress,
			      MateVFSXferPhase phase,
			      gint64 *next_time)
{
	int result;
	gint64 now;

	result = 1;
	now = system_time ();

	progress->progress_info->phase = phase;

	if (progress->sync_callback != NULL) {
		result = (* progress->sync_callback) (progress->progress_info, progress->user_data);
	}

	if (now < *next_time && !at_end (progress)) {
		return result;
	}

	*next_time = now + progress->update_callback_period;
	if (progress->update_callback != NULL) {
		result = (* progress->update_callback) (progress->progress_info, progress->async_job_data);
	}

	return result;
}

static int
call_progress_often (MateVFSProgressCallbackState *progress,
		     MateVFSXferPhase phase)
{
	return call_progress_often_internal
		(progress, phase, &progress->next_update_callback_time);
}

static int
call_progress_with_uris_often (MateVFSProgressCallbackState *progress, 
			       const MateVFSURI *source_uri, const MateVFSURI *dest_uri, 
			       MateVFSXferPhase phase)
{
	progress_set_source_target_uris (progress, source_uri, dest_uri);
	return call_progress_often_internal
		(progress, phase, &progress->next_text_update_callback_time);
}

/* Handle an error condition according to `error_mode'.  Returns `TRUE' if the
 * operation should be retried, `FALSE' otherwise.  `*result' will be set to
 * the result value we want to be returned to the caller of the xfer
 * function.
 */
static gboolean
handle_error (MateVFSResult *result,
	      MateVFSProgressCallbackState *progress,
	      MateVFSXferErrorMode *error_mode,
	      gboolean *skip)
{
	MateVFSXferErrorAction action;

	*skip = FALSE;

	switch (*error_mode) {
	case MATE_VFS_XFER_ERROR_MODE_ABORT:
		return FALSE;

	case MATE_VFS_XFER_ERROR_MODE_QUERY:
		progress->progress_info->status = MATE_VFS_XFER_PROGRESS_STATUS_VFSERROR;
		progress->progress_info->vfs_status = *result;
		action = call_progress_with_current_names (progress, progress->progress_info->phase);
		progress->progress_info->status = MATE_VFS_XFER_PROGRESS_STATUS_OK;

		switch (action) {
		case MATE_VFS_XFER_ERROR_ACTION_RETRY:
			return TRUE;
		case MATE_VFS_XFER_ERROR_ACTION_ABORT:
			*result = MATE_VFS_ERROR_INTERRUPTED;
			return FALSE;
		case MATE_VFS_XFER_ERROR_ACTION_SKIP:
			*result = MATE_VFS_OK;
			*skip = TRUE;
			return FALSE;
		}
		break;
	}

	*skip = FALSE;
	return FALSE;
}

/* This is conceptually similiar to the previous `handle_error()' function, but
 * handles the overwrite case. 
 */
static gboolean
handle_overwrite (MateVFSResult *result,
		  MateVFSProgressCallbackState *progress,
		  MateVFSXferErrorMode *error_mode,
		  MateVFSXferOverwriteMode *overwrite_mode,
		  gboolean *replace,
		  gboolean *skip)
{
	MateVFSXferOverwriteAction action;

	switch (*overwrite_mode) {
	case MATE_VFS_XFER_OVERWRITE_MODE_ABORT:
		*replace = FALSE;
		*result = MATE_VFS_ERROR_FILE_EXISTS;
		*skip = FALSE;
		return FALSE;
	case MATE_VFS_XFER_OVERWRITE_MODE_REPLACE:
		*replace = TRUE;
		*skip = FALSE;
		return TRUE;
	case MATE_VFS_XFER_OVERWRITE_MODE_SKIP:
		*replace = FALSE;
		*skip = TRUE;
		return FALSE;
	case MATE_VFS_XFER_OVERWRITE_MODE_QUERY:
		progress->progress_info->vfs_status = *result;
		progress->progress_info->status = MATE_VFS_XFER_PROGRESS_STATUS_OVERWRITE;
		action = call_progress_with_current_names (progress, progress->progress_info->phase);
		progress->progress_info->status = MATE_VFS_XFER_PROGRESS_STATUS_OK;

		switch (action) {
		case MATE_VFS_XFER_OVERWRITE_ACTION_ABORT:
			*replace = FALSE;
			*result = MATE_VFS_ERROR_FILE_EXISTS;
			*skip = FALSE;
			return FALSE;
		case MATE_VFS_XFER_OVERWRITE_ACTION_REPLACE:
			*replace = TRUE;
			*skip = FALSE;
			return TRUE;
		case MATE_VFS_XFER_OVERWRITE_ACTION_REPLACE_ALL:
			*replace = TRUE;
			*overwrite_mode = MATE_VFS_XFER_OVERWRITE_MODE_REPLACE;
			*skip = FALSE;
			return TRUE;
		case MATE_VFS_XFER_OVERWRITE_ACTION_SKIP:
			*replace = FALSE;
			*skip = TRUE;
			return FALSE;
		case MATE_VFS_XFER_OVERWRITE_ACTION_SKIP_ALL:
			*replace = FALSE;
			*skip = TRUE;
			*overwrite_mode = MATE_VFS_XFER_OVERWRITE_MODE_SKIP;
			return FALSE;
		}
	}

	*replace = FALSE;
	*skip = FALSE;
	return FALSE;
}

static MateVFSResult
remove_file (MateVFSURI *uri,
	     MateVFSProgressCallbackState *progress,
	     MateVFSXferOptions xfer_options,
	     MateVFSXferErrorMode *error_mode,
	     gboolean *skip)
{
	MateVFSResult result;
	gboolean retry;

	progress->progress_info->file_index++;
	do {
		retry = FALSE;
		result = mate_vfs_unlink_from_uri (uri);		
		if (result != MATE_VFS_OK) {
			retry = handle_error (&result, progress,
					      error_mode, skip);
		} else {
			/* add some small size for each deleted item because delete overhead
			 * does not depend on file/directory size
			 */
			progress->progress_info->total_bytes_copied += DEFAULT_SIZE_OVERHEAD;

			if (call_progress_with_uris_often (progress, uri, NULL, 
							   MATE_VFS_XFER_PHASE_DELETESOURCE) 
				== MATE_VFS_XFER_OVERWRITE_ACTION_ABORT) {
				result = MATE_VFS_ERROR_INTERRUPTED;
				break;
			}
		}


	} while (retry);

	return result;
}

static MateVFSResult
empty_directory (MateVFSURI *uri,
		 MateVFSProgressCallbackState *progress,
		 MateVFSXferOptions xfer_options,
		 MateVFSXferErrorMode *error_mode,
		 gboolean *skip)
{
	MateVFSResult result;
	MateVFSDirectoryHandle *directory_handle;
	gboolean retry;


	*skip = FALSE;
	do {
		result = mate_vfs_directory_open_from_uri (&directory_handle, uri, 
							    MATE_VFS_FILE_INFO_DEFAULT);
		retry = FALSE;
		if (result != MATE_VFS_OK) {
			retry = handle_error (&result, progress,
					      error_mode, skip);
		}
	} while (retry);

	if (result != MATE_VFS_OK || *skip) {
		return result;
	}
	
	for (;;) {
		MateVFSFileInfo *info;
		MateVFSURI *item_uri;
		
		item_uri = NULL;
		info = mate_vfs_file_info_new ();
		result = mate_vfs_directory_read_next (directory_handle, info);
		if (result != MATE_VFS_OK) {
			mate_vfs_file_info_unref (info);
			break;
		}

		/* Skip "." and "..".  */
		if (strcmp (info->name, ".") != 0 && strcmp (info->name, "..") != 0) {

			item_uri = mate_vfs_uri_append_file_name (uri, info->name);
			
			progress_set_source_target_uris (progress, item_uri, NULL);
			
			if (info->type == MATE_VFS_FILE_TYPE_DIRECTORY) {
				result = remove_directory (item_uri, TRUE, 
							   progress, xfer_options, error_mode, 
							   skip);
			} else {
				result = remove_file (item_uri, progress, 
						      xfer_options,
						      error_mode,
						      skip);
			}

		}

		mate_vfs_file_info_unref (info);
		
		if (item_uri != NULL) {
			mate_vfs_uri_unref (item_uri);
		}
		
		if (result != MATE_VFS_OK) {
			break;
		}
	}
	mate_vfs_directory_close (directory_handle);

	if (result == MATE_VFS_ERROR_EOF) {
		result = MATE_VFS_OK;
	}

	return result;
}

typedef struct {
	const MateVFSURI *base_uri;
	GList *uri_list;
} PrependOneURIParams;

static gboolean
PrependOneURIToList (const gchar *rel_path, MateVFSFileInfo *info,
	gboolean recursing_will_loop, gpointer cast_to_params, gboolean *recurse)
{
	PrependOneURIParams *params;

	params = (PrependOneURIParams *)cast_to_params;
	params->uri_list = g_list_prepend (params->uri_list, mate_vfs_uri_append_path (
		params->base_uri, rel_path));

	if (recursing_will_loop) {
		return FALSE;
	}
	*recurse = TRUE;
	return TRUE;
}

static MateVFSResult
non_recursive_empty_directory (MateVFSURI *directory_uri,
			       MateVFSProgressCallbackState *progress,
			       MateVFSXferOptions xfer_options,
			       MateVFSXferErrorMode *error_mode,
			       gboolean *skip)
{
	/* Used as a fallback when we run out of file descriptors during 
	 * a deep recursive deletion. 
	 * We instead compile a flat list of URIs, doing a recursion that does not
	 * keep directories open.
	 */

	GList *uri_list;
	GList *p;
	MateVFSURI *uri;
	MateVFSResult result;
	MateVFSFileInfo *info;
	PrependOneURIParams visit_params;

	/* Build up the list so that deep items appear before their parents
	 * so that we can delete directories, children first.
	 */
	visit_params.base_uri = directory_uri;
	visit_params.uri_list = NULL;
	result = mate_vfs_directory_visit_uri (directory_uri, MATE_VFS_FILE_INFO_DEFAULT, 
						MATE_VFS_DIRECTORY_VISIT_SAMEFS | MATE_VFS_DIRECTORY_VISIT_LOOPCHECK,
						PrependOneURIToList, &visit_params);

	uri_list = visit_params.uri_list;

	if (result == MATE_VFS_OK) {
		for (p = uri_list; p != NULL; p = p->next) {

			uri = (MateVFSURI *)p->data;
			info = mate_vfs_file_info_new ();
			result = mate_vfs_get_file_info_uri (uri, info, MATE_VFS_FILE_INFO_DEFAULT);
			progress_set_source_target_uris (progress, uri, NULL);
			if (result == MATE_VFS_OK) {
				if (info->type == MATE_VFS_FILE_TYPE_DIRECTORY) {
					result = remove_directory (uri, FALSE, 
						progress, xfer_options, error_mode, skip);
				} else {
					result = remove_file (uri, progress, 
						xfer_options, error_mode, skip);
				}
			}
			mate_vfs_file_info_unref (info);
		}
	}

	mate_vfs_uri_list_free (uri_list);

	return result;
}

static MateVFSResult
remove_directory (MateVFSURI *uri,
		  gboolean recursive,
		  MateVFSProgressCallbackState *progress,
		  MateVFSXferOptions xfer_options,
		  MateVFSXferErrorMode *error_mode,
		  gboolean *skip)
{
	MateVFSResult result;
	gboolean retry;

	result = MATE_VFS_OK;

	if (recursive) {
		result = empty_directory (uri, progress, xfer_options, error_mode, skip);
		if (result == MATE_VFS_ERROR_TOO_MANY_OPEN_FILES) {
			result = non_recursive_empty_directory (uri, progress, xfer_options,
				error_mode, skip);
		}
	}

	if (result == MATE_VFS_ERROR_EOF) {
		result = MATE_VFS_OK;
	}

	if (result == MATE_VFS_OK) {
		progress->progress_info->file_index++;

		do {
			retry = FALSE;

			result = mate_vfs_remove_directory_from_uri (uri);
			if (result != MATE_VFS_OK) {
				retry = handle_error (&result, progress,
						      error_mode, skip);
			} else {
				/* add some small size for each deleted item */
				progress->progress_info->total_bytes_copied += DEFAULT_SIZE_OVERHEAD;

				if (call_progress_with_uris_often (progress, uri, NULL, 
					MATE_VFS_XFER_PHASE_DELETESOURCE) 
					== MATE_VFS_XFER_OVERWRITE_ACTION_ABORT) {
					result = MATE_VFS_ERROR_INTERRUPTED;
					break;
				}
			}

		} while (retry);
	}

	return result;
}

/* iterates the list of names in a given directory, applies @callback on each,
 * optionally recurses into directories
 */
static MateVFSResult
mate_vfs_visit_list (const GList *name_uri_list,
		      MateVFSFileInfoOptions info_options,
		      MateVFSDirectoryVisitOptions visit_options,
		      gboolean recursive,
		      MateVFSDirectoryVisitFunc callback,
		      gpointer data)
{
	MateVFSResult result;
	const GList *p;
	MateVFSURI *uri;
	MateVFSFileInfo *info;
	gboolean tmp_recurse;
	
	result = MATE_VFS_OK;

	/* go through our list of items */
	for (p = name_uri_list; p != NULL; p = p->next) {
		
		/* get the URI and VFSFileInfo for each */
		uri = (MateVFSURI *)p->data;
		info = mate_vfs_file_info_new ();
		result = mate_vfs_get_file_info_uri (uri, info, info_options);
		
		if (result == MATE_VFS_OK) {
			tmp_recurse = TRUE;
			
			/* call our callback on each item*/
			if (!callback (mate_vfs_uri_get_path (uri), info, FALSE, data, &tmp_recurse)) {
				result = MATE_VFS_ERROR_INTERRUPTED;
			}
			
			if (result == MATE_VFS_OK
			    && recursive
			    && info->type == MATE_VFS_FILE_TYPE_DIRECTORY) {
				/* let mate_vfs_directory_visit_uri call our callback 
				 * recursively 
				 */
				result = mate_vfs_directory_visit_uri (uri, info_options, 
					visit_options, callback, data);
			}
		}
		mate_vfs_file_info_unref (info);
		
		if (result != MATE_VFS_OK) {
			break;
		}
	}
	return result;
}

typedef struct {
	MateVFSProgressCallbackState *progress;
	MateVFSResult result;
} CountEachFileSizeParams;

/* iteratee for count_items_and_size */
static gboolean
count_each_file_size_one (const gchar *rel_path,
			  MateVFSFileInfo *info,
			  gboolean recursing_will_loop,
			  gpointer data,
			  gboolean *recurse)
{
	CountEachFileSizeParams *params;

	params = (CountEachFileSizeParams *)data;

	if (call_progress_often (params->progress, params->progress->progress_info->phase) == 0) {
		/* progress callback requested to stop */
		params->result = MATE_VFS_ERROR_INTERRUPTED;
		*recurse = FALSE;
		return FALSE;
	}

	/* keep track of the item we are counting so we can correctly report errors */
	progress_set_source_target (params->progress, rel_path, NULL);

	/* count each file, folder, symlink */
	params->progress->progress_info->files_total++;
	if (info->type == MATE_VFS_FILE_TYPE_REGULAR) {
		/* add each file size */
		params->progress->progress_info->bytes_total += info->size + DEFAULT_SIZE_OVERHEAD;
	} else if (info->type == MATE_VFS_FILE_TYPE_DIRECTORY) {
		/* add some small size for each directory */
		params->progress->progress_info->bytes_total += DEFAULT_SIZE_OVERHEAD;
	}

	/* watch out for infinite recursion*/
	if (recursing_will_loop) {
		params->result = MATE_VFS_ERROR_LOOP;
		return FALSE;
	}

	*recurse = TRUE;

	return TRUE;
}

static MateVFSResult
list_add_items_and_size (const GList *name_uri_list,
			 MateVFSXferOptions xfer_options,
			 MateVFSProgressCallbackState *progress,
			 gboolean recurse)
{
	/*
	 * FIXME bugzilla.eazel.com 1200:
	 * Deal with errors here, respond to skip by pulling items from the name list
	 */
	MateVFSFileInfoOptions info_options;
	MateVFSDirectoryVisitOptions visit_options;
	CountEachFileSizeParams each_params;

	/* set up the params for recursion */
	visit_options = MATE_VFS_DIRECTORY_VISIT_LOOPCHECK;
	if (xfer_options & MATE_VFS_XFER_SAMEFS)
		visit_options |= MATE_VFS_DIRECTORY_VISIT_SAMEFS;
	each_params.progress = progress;
	each_params.result = MATE_VFS_OK;

	info_options = MATE_VFS_FILE_INFO_DEFAULT;
	if (xfer_options & MATE_VFS_XFER_FOLLOW_LINKS) {
		info_options |= MATE_VFS_FILE_INFO_FOLLOW_LINKS;
	}

	return mate_vfs_visit_list (name_uri_list, info_options,
				     visit_options, recurse,
				     count_each_file_size_one, &each_params);
}

/* calculate the number of items and their total size; used as a preflight
 * before the transfer operation starts
 */
static MateVFSResult
count_items_and_size (const GList *name_uri_list,
		      MateVFSXferOptions xfer_options,
		      MateVFSProgressCallbackState *progress,
		      gboolean move,
		      gboolean link)
{
 	/* initialize the results */
	progress->progress_info->files_total = 0;
	progress->progress_info->bytes_total = 0;

	return list_add_items_and_size (name_uri_list,
					xfer_options,
					progress,
					!link && !move && (xfer_options & MATE_VFS_XFER_RECURSIVE) != 0);
}

/* calculate the number of items and their total size; used as a preflight
 * before the transfer operation starts
 */
static MateVFSResult
directory_add_items_and_size (MateVFSURI *dir_uri,
			      MateVFSXferOptions xfer_options,
			      MateVFSProgressCallbackState *progress)
{
	MateVFSFileInfoOptions info_options;
	MateVFSDirectoryVisitOptions visit_options;
	CountEachFileSizeParams each_params;

	/* set up the params for recursion */
	visit_options = MATE_VFS_DIRECTORY_VISIT_LOOPCHECK;
	if (xfer_options & MATE_VFS_XFER_SAMEFS)
		visit_options |= MATE_VFS_DIRECTORY_VISIT_SAMEFS;
	each_params.progress = progress;
	each_params.result = MATE_VFS_OK;

	info_options = MATE_VFS_FILE_INFO_DEFAULT;
	if (xfer_options & MATE_VFS_XFER_FOLLOW_LINKS) {
		info_options |= MATE_VFS_FILE_INFO_FOLLOW_LINKS;
	}

	return mate_vfs_directory_visit_uri (dir_uri, info_options,
		visit_options, count_each_file_size_one, &each_params);

}

/* Checks to see if any part of the source pathname of a move
 * is inside the target. If it is we can't remove the target
 * and then move the source to it since we would then remove
 * the source before we moved it.
 */
static gboolean
move_source_is_in_target (MateVFSURI *source, MateVFSURI *target)
{
	MateVFSURI *parent, *tmp;
	gboolean res;

	parent = mate_vfs_uri_ref (source);

	res = FALSE;
	while (parent != NULL) {
		if (_mate_vfs_uri_is_in_subdir (parent, target)) {
			res = TRUE;
			break;
		}
		tmp = mate_vfs_uri_get_parent (parent);
		mate_vfs_uri_unref (parent);
		parent = tmp;
	}
	if (parent != NULL) {
		mate_vfs_uri_unref (parent);
	}
	
	return res;
}

typedef struct {
	MateVFSURI *source_uri;
	MateVFSURI *target_uri;
	MateVFSXferOptions xfer_options;
	MateVFSXferErrorMode *error_mode;
	MateVFSProgressCallbackState *progress;
	MateVFSResult result;
} HandleMergedNameConflictsParams;

static gboolean
handle_merged_name_conflict_visit (const gchar *rel_path,
				   MateVFSFileInfo *info,
				   gboolean recursing_will_loop,
				   gpointer data,
				   gboolean *recurse)
{
	HandleMergedNameConflictsParams *params;
	MateVFSURI *source_uri;
	MateVFSURI *target_uri;
	MateVFSFileInfo *source_info;
	MateVFSFileInfo *target_info;
	MateVFSResult result;
	gboolean skip;

	params = data;

	skip = FALSE;
	result = MATE_VFS_OK;
	target_info = mate_vfs_file_info_new ();
	target_uri = mate_vfs_uri_append_path (params->target_uri, rel_path);
	if (mate_vfs_get_file_info_uri (target_uri, target_info, MATE_VFS_FILE_INFO_DEFAULT) == MATE_VFS_OK) {
		source_info = mate_vfs_file_info_new ();
		source_uri = mate_vfs_uri_append_path (params->source_uri, rel_path);
		
		if (mate_vfs_get_file_info_uri (source_uri, source_info, MATE_VFS_FILE_INFO_DEFAULT) == MATE_VFS_OK) {
			if (target_info->type != MATE_VFS_FILE_TYPE_DIRECTORY ||
			    source_info->type != MATE_VFS_FILE_TYPE_DIRECTORY) {
				/* Both not directories, get rid of the conflicting target file */
				
				/* FIXME bugzilla.eazel.com 1207:
				 * move items to Trash here
				 */
				if (target_info->type == MATE_VFS_FILE_TYPE_DIRECTORY) {
					if (move_source_is_in_target (source_uri, target_uri)) {
						/* Would like a better error here */
						result = MATE_VFS_ERROR_DIRECTORY_NOT_EMPTY;
					} else {
						result = remove_directory (target_uri, TRUE, params->progress, 
									   params->xfer_options, params->error_mode, &skip);
					}
				} else {
					result = remove_file (target_uri, params->progress,
							      params->xfer_options, params->error_mode, &skip);
				}
			}
		}
		
		mate_vfs_file_info_unref (source_info);
		mate_vfs_uri_unref (source_uri);
	}

	mate_vfs_file_info_unref (target_info);
	mate_vfs_uri_unref (target_uri);

	*recurse = !recursing_will_loop;
	params->result = result;
	return result == MATE_VFS_OK;
}


static MateVFSResult
handle_merged_directory_name_conflicts (MateVFSXferOptions xfer_options,
					MateVFSXferErrorMode *error_mode,
					MateVFSProgressCallbackState *progress,
					MateVFSURI *source_uri,
					MateVFSURI *target_uri)
{
	MateVFSFileInfoOptions info_options;
	MateVFSDirectoryVisitOptions visit_options;
	HandleMergedNameConflictsParams params;

	params.source_uri = source_uri;
	params.target_uri = target_uri;
	params.xfer_options = xfer_options;
	params.error_mode = error_mode;
	params.progress = progress;
	
	/* set up the params for recursion */
	visit_options = MATE_VFS_DIRECTORY_VISIT_LOOPCHECK;
	if (xfer_options & MATE_VFS_XFER_SAMEFS)
		visit_options |= MATE_VFS_DIRECTORY_VISIT_SAMEFS;

	info_options = MATE_VFS_FILE_INFO_DEFAULT;
	if (xfer_options & MATE_VFS_XFER_FOLLOW_LINKS) {
		info_options |= MATE_VFS_FILE_INFO_FOLLOW_LINKS;
	}

	params.result = MATE_VFS_OK;
	mate_vfs_directory_visit_uri (source_uri,
				       MATE_VFS_FILE_INFO_DEFAULT,
				       visit_options,
				       handle_merged_name_conflict_visit,
				       &params);

	return params.result;
}


/* Compares the list of files about to be created by a transfer with
 * any possible existing files with conflicting names in the target directory.
 * Handles conflicts, optionaly removing the conflicting file/directory
 */
static MateVFSResult
handle_name_conflicts (GList **source_uri_list,
		       GList **target_uri_list,
		       MateVFSXferOptions xfer_options,
		       MateVFSXferErrorMode *error_mode,
		       MateVFSXferOverwriteMode *overwrite_mode,
		       MateVFSProgressCallbackState *progress,
		       gboolean move,
		       gboolean link,
		       GList **merge_source_uri_list,
		       GList **merge_target_uri_list)
{
	MateVFSResult result;
	GList *source_item;
	GList *target_item;
	MateVFSFileInfo *target_info;
	MateVFSFileInfo *source_info;
	GList *source_item_to_remove;
	GList *target_item_to_remove;
	int conflict_count; /* values are 0, 1, many */
	
	result = MATE_VFS_OK;
	conflict_count = 0;
	
	/* Go through the list of names, find out if there is 0, 1 or more conflicts. */
	for (target_item = *target_uri_list; target_item != NULL;
	     target_item = target_item->next) {
               if (mate_vfs_uri_exists ((MateVFSURI *)target_item->data)) {
                       conflict_count++;
                       if (conflict_count > 1) {
                               break;
                       }
               }
	}
	
	if (conflict_count == 0) {
		/* No conflicts, we are done. */
		return MATE_VFS_OK;
	}

	/* Pass in the conflict count so that we can decide to present the Replace All
	 * for multiple conflicts.
	 */
	progress->progress_info->duplicate_count = conflict_count;
	
	target_info = mate_vfs_file_info_new ();
	
	/* Go through the list of names again, present overwrite alerts for each. */
	for (target_item = *target_uri_list, source_item = *source_uri_list; 
	     target_item != NULL;) {
		gboolean replace;
		gboolean skip;
		gboolean is_move_to_self;
		MateVFSURI *uri, *source_uri;
		gboolean must_copy;

		must_copy = FALSE;
		skip = FALSE;
		source_uri = (MateVFSURI *)source_item->data;
		uri = (MateVFSURI *)target_item->data;
		is_move_to_self = (xfer_options & MATE_VFS_XFER_REMOVESOURCE) != 0
			&& mate_vfs_uri_equal (source_uri, uri);
		if (!is_move_to_self &&
		    mate_vfs_get_file_info_uri (uri, target_info, MATE_VFS_FILE_INFO_DEFAULT) == MATE_VFS_OK) {
			progress_set_source_target_uris (progress, source_uri, uri);
			
			/* Skip if we are trying to copy the file to itself */
			if (mate_vfs_uri_equal (uri, source_uri)) {
				replace = FALSE;
				skip = TRUE;
			} else {
				/* no error getting info -- file exists, ask what to do */
				replace = handle_overwrite (&result, progress, error_mode,
							    overwrite_mode, &replace, &skip);
			}
			
			/* FIXME bugzilla.eazel.com 1207:
			 * move items to Trash here
			 */
			
			/* get rid of the conflicting file */
			if (replace) {
				progress_set_source_target_uris (progress, uri, NULL);
				if (target_info->type == MATE_VFS_FILE_TYPE_DIRECTORY) {
					if (move_source_is_in_target (source_uri, uri)) {
						/* Would like a better error here */
						result = MATE_VFS_ERROR_DIRECTORY_NOT_EMPTY;
					} else {
						source_info = mate_vfs_file_info_new ();
						if (!link &&
						    mate_vfs_get_file_info_uri (source_uri, source_info, MATE_VFS_FILE_INFO_DEFAULT) == MATE_VFS_OK &&
						    source_info->type == MATE_VFS_FILE_TYPE_DIRECTORY) {
							if (move || (xfer_options & MATE_VFS_XFER_RECURSIVE)) {
								result = handle_merged_directory_name_conflicts (xfer_options, error_mode,
														 progress, source_uri, uri);
							}
							must_copy = TRUE;
						} else {
							result = remove_directory (uri, TRUE, progress, 
										   xfer_options, error_mode, &skip);
						}
						mate_vfs_file_info_unref (source_info);
					}
				} else {
					result = remove_file (uri, progress,
							      xfer_options, error_mode, &skip);
				}
			}
			
			mate_vfs_file_info_clear (target_info);
		}

		
		if (result != MATE_VFS_OK) {
			break;
		}

		if (skip) {
			/* skipping a file, remove it's name from the source and target name
			 * lists.
			 */
			source_item_to_remove = source_item;
			target_item_to_remove = target_item;
			
			source_item = source_item->next;
			target_item = target_item->next;

			mate_vfs_uri_unref ((MateVFSURI *)source_item_to_remove->data);
			mate_vfs_uri_unref ((MateVFSURI *)target_item_to_remove->data);
			*source_uri_list = g_list_remove_link (*source_uri_list, source_item_to_remove);
			*target_uri_list = g_list_remove_link (*target_uri_list, target_item_to_remove);
			
			continue;
		}

		if (move && must_copy) {
			/* We're moving, but there was a conflict, so move this over to the list of items that has to be copied */
			
			source_item_to_remove = source_item;
			target_item_to_remove = target_item;
			
			source_item = source_item->next;
			target_item = target_item->next;

			*merge_source_uri_list = g_list_prepend (*merge_source_uri_list, (MateVFSURI *)source_item_to_remove->data);
			*merge_target_uri_list = g_list_prepend (*merge_target_uri_list, (MateVFSURI *)target_item_to_remove->data);
			*source_uri_list = g_list_remove_link (*source_uri_list, source_item_to_remove);
			*target_uri_list = g_list_remove_link (*target_uri_list, target_item_to_remove);
			
			continue;
		}

		target_item = target_item->next; 
		source_item = source_item->next;
	}
	mate_vfs_file_info_unref (target_info);
	
	return result;
}

/* Create new directory. If MATE_VFS_XFER_USE_UNIQUE_NAMES is set, 
 * return with an error if a name conflict occurs, else
 * handle the overwrite.
 * On success, opens the new directory
 */
static MateVFSResult
create_directory (MateVFSURI *dir_uri,
		  MateVFSDirectoryHandle **return_handle,
		  MateVFSXferOptions xfer_options,
		  MateVFSXferErrorMode *error_mode,
		  MateVFSXferOverwriteMode *overwrite_mode,
		  MateVFSProgressCallbackState *progress,
		  gboolean *skip)
{
	MateVFSResult result;
	gboolean retry, info_result;
	MateVFSFileInfo *info;
	
	*skip = FALSE;
	do {
		retry = FALSE;

		result = mate_vfs_make_directory_for_uri (dir_uri, 0777);

		if (result == MATE_VFS_ERROR_FILE_EXISTS) {
			gboolean force_replace;

			if ((xfer_options & MATE_VFS_XFER_USE_UNIQUE_NAMES) != 0) {
				/* just let the caller pass a unique name*/
				return result;
			}

			info = mate_vfs_file_info_new ();
			info_result = mate_vfs_get_file_info_uri (dir_uri, info, MATE_VFS_FILE_INFO_DEFAULT);
			if (info_result == MATE_VFS_OK &&
			    info->type == MATE_VFS_FILE_TYPE_DIRECTORY) {
				/* Already got a directory here, its fine */
				result = MATE_VFS_OK;
			}
			mate_vfs_file_info_unref (info);
			
			if (result != MATE_VFS_OK) {			
				retry = handle_overwrite (&result,
							  progress,
							  error_mode,
							  overwrite_mode,
							  &force_replace,
							  skip);
				
				if (*skip) {
					return MATE_VFS_OK;
				}
				if (force_replace) {
					result = remove_file (dir_uri, progress, 
							      xfer_options, error_mode, 
							      skip);
				} else {
					result = MATE_VFS_OK;
				}
			}
		}

		if (result == MATE_VFS_OK) {
			return mate_vfs_directory_open_from_uri (return_handle, 
								  dir_uri, 
								  MATE_VFS_FILE_INFO_DEFAULT);
		}
		/* handle the error case */
		retry = handle_error (&result, progress,
				      error_mode, skip);

		if (*skip) {
			return MATE_VFS_OK;
		}

	} while (retry);

	return result;
}

/* Copy the data of a single file. */
static MateVFSResult
copy_file_data (MateVFSHandle *target_handle,
		MateVFSHandle *source_handle,
		MateVFSProgressCallbackState *progress,
		MateVFSXferOptions xfer_options,
		MateVFSXferErrorMode *error_mode,
		guint source_block_size,
		guint target_block_size,
		gboolean *skip)
{
	MateVFSResult result;
	gpointer buffer;
	const char *write_buffer;
	MateVFSFileSize total_bytes_read;
	MateVFSFileSize last_cache_drop_point;
	gboolean forget_cache;
	guint block_size;

	*skip = FALSE;

	if (call_progress_often (progress, MATE_VFS_XFER_PHASE_COPYING) == 0) {
		return MATE_VFS_ERROR_INTERRUPTED;
	}

	block_size = MAX (source_block_size, target_block_size);
	buffer = g_malloc (block_size);
	total_bytes_read = 0;
	last_cache_drop_point = 0;

	/* Enable streaming if the total size is large */
	forget_cache = progress->progress_info->bytes_total >= DROP_CACHE_SIZE_LIMIT;
	
	do {
		MateVFSFileSize bytes_read;
		MateVFSFileSize bytes_to_write;
		MateVFSFileSize bytes_written;
		gboolean retry;

		progress->progress_info->status = MATE_VFS_XFER_PROGRESS_STATUS_OK;
		progress->progress_info->vfs_status = MATE_VFS_OK;

		progress->progress_info->phase = MATE_VFS_XFER_PHASE_READSOURCE;

		do {
			retry = FALSE;

			result = mate_vfs_read (source_handle, buffer,
						 block_size, &bytes_read);
			if (result != MATE_VFS_OK && result != MATE_VFS_ERROR_EOF)
				retry = handle_error (&result, progress,
						      error_mode, skip);
		} while (retry);

		if (result != MATE_VFS_OK || bytes_read == 0 || *skip) {
			break;
		}

		total_bytes_read += bytes_read;
		bytes_to_write = bytes_read;

		progress->progress_info->phase = MATE_VFS_XFER_PHASE_WRITETARGET;

		write_buffer = buffer;
		do {
			retry = FALSE;

			result = mate_vfs_write (target_handle, write_buffer,
						  bytes_to_write,
						  &bytes_written);

			if (result != MATE_VFS_OK) {
				retry = handle_error (&result, progress, error_mode, skip);
			}

			bytes_to_write -= bytes_written;
			write_buffer += bytes_written;
		} while ((result == MATE_VFS_OK && bytes_to_write > 0) || retry);
		
		if (forget_cache && bytes_to_write == 0 &&
		    total_bytes_read - last_cache_drop_point > DROP_CACHE_BATCH_SIZE) {
			mate_vfs_forget_cache (source_handle,
						last_cache_drop_point,
						total_bytes_read - last_cache_drop_point);
			mate_vfs_forget_cache (target_handle,
						last_cache_drop_point,
						total_bytes_read - last_cache_drop_point);

			last_cache_drop_point = total_bytes_read;
		}

		progress->progress_info->phase = MATE_VFS_XFER_PHASE_COPYING;

		progress->progress_info->bytes_copied += bytes_read;
		progress->progress_info->total_bytes_copied += bytes_read;

		if (call_progress_often (progress, MATE_VFS_XFER_PHASE_COPYING) == 0) {
			g_free (buffer);
			return MATE_VFS_ERROR_INTERRUPTED;
		}

		if (*skip) {
			break;
		}

	} while (result == MATE_VFS_OK);

	if (result == MATE_VFS_ERROR_EOF) {
		result = MATE_VFS_OK;
	}

	if (result == MATE_VFS_OK) {
		/* tiny (0 - sized) files still present non-zero overhead during a copy, make sure
		 * we count at least a default amount for each file
		 */
		progress->progress_info->total_bytes_copied += DEFAULT_SIZE_OVERHEAD;

		call_progress_often (progress, MATE_VFS_XFER_PHASE_COPYING);
	}

	g_free (buffer);

	return result;
}

static MateVFSResult
xfer_open_source (MateVFSHandle **source_handle,
		  MateVFSURI *source_uri,
		  MateVFSProgressCallbackState *progress,
		  MateVFSXferOptions xfer_options,
		  MateVFSXferErrorMode *error_mode,
		  gboolean *skip)
{
	MateVFSResult result;
	gboolean retry;
	
        call_progress_often (progress, MATE_VFS_XFER_PHASE_OPENSOURCE);

	*skip = FALSE;
	do {
		retry = FALSE;

		result = mate_vfs_open_uri (source_handle, source_uri,
					     MATE_VFS_OPEN_READ);

		if (result != MATE_VFS_OK) {
			retry = handle_error (&result, progress, error_mode, skip);
			*source_handle = NULL;
		}
	} while (retry);

	/* if we didn't get a file size earlier,
	 * try to get one from the handle (#330498)
	 */
	if (*source_handle != NULL && progress->progress_info->file_size == 0) {
		MateVFSFileInfo *info;

		info = mate_vfs_file_info_new ();
		result = mate_vfs_get_file_info_from_handle (*source_handle,
							      info,
							      MATE_VFS_FILE_INFO_DEFAULT);
		if (result == MATE_VFS_OK &&
		    info->valid_fields & MATE_VFS_FILE_INFO_FIELDS_SIZE) {
			progress->progress_info->file_size = info->size;
		}
		mate_vfs_file_info_unref (info);
	}

	return result;
}

static MateVFSResult
xfer_create_target (MateVFSHandle **target_handle,
		    MateVFSURI *target_uri,
		    MateVFSProgressCallbackState *progress,
		    MateVFSXferOptions xfer_options,
		    MateVFSXferErrorMode *error_mode,
		    MateVFSXferOverwriteMode *overwrite_mode,
		    gboolean *skip)
{
	MateVFSResult result;
	gboolean retry;
	gboolean exclusive;

	exclusive = (*overwrite_mode != MATE_VFS_XFER_OVERWRITE_MODE_REPLACE);

	*skip = FALSE;
	do {
		retry = FALSE;

		result = mate_vfs_create_uri (target_handle, target_uri,
					       MATE_VFS_OPEN_WRITE,
					       exclusive, 0666);

		if (result == MATE_VFS_ERROR_FILE_EXISTS) {
			gboolean replace;

			retry = handle_overwrite (&result,
						  progress,
						  error_mode,
						  overwrite_mode,
						  &replace,
						  skip);

			if (replace) {
				exclusive = FALSE;
			}

		} else if (result != MATE_VFS_OK) {
			retry = handle_error (&result,  progress, error_mode, skip);
		}
	} while (retry);

	return result;
}

static MateVFSResult
copy_symlink (MateVFSURI *source_uri,
	      MateVFSURI *target_uri,
	      const char *link_name,
	      MateVFSXferErrorMode *error_mode,
	      MateVFSXferOverwriteMode *overwrite_mode,
	      MateVFSProgressCallbackState *progress,
	      gboolean *skip)
{
	MateVFSResult result;
	gboolean retry;
	gboolean tried_remove, remove;

	tried_remove = FALSE;

	*skip = FALSE;
	do {
		retry = FALSE;

		result = mate_vfs_create_symbolic_link (target_uri, link_name);

		if (result == MATE_VFS_ERROR_FILE_EXISTS) {
			remove = FALSE;
			
			if (*overwrite_mode == MATE_VFS_XFER_OVERWRITE_MODE_REPLACE &&
			    !tried_remove) {
				remove = TRUE;
				tried_remove = TRUE;
			} else {
				retry = handle_overwrite (&result,
							  progress,
							  error_mode,
							  overwrite_mode,
							  &remove,
							  skip);
			}
			if (remove) {
				mate_vfs_unlink_from_uri (target_uri);
			}
		} else if (result != MATE_VFS_OK) {
			retry = handle_error (&result, progress, error_mode, skip);
		} else if (result == MATE_VFS_OK &&
			   call_progress_with_uris_often (progress, source_uri,
							  target_uri, MATE_VFS_XFER_PHASE_OPENTARGET) == 0) {
			result = MATE_VFS_ERROR_INTERRUPTED;
		}
	} while (retry);

	if (*skip) {
		return MATE_VFS_OK;
	}
	
	return result;
}

static MateVFSResult
copy_file (MateVFSFileInfo *info,
	   MateVFSFileInfo *target_dir_info,
	   MateVFSURI *source_uri,
	   MateVFSURI *target_uri,
	   MateVFSXferOptions xfer_options,
	   MateVFSXferErrorMode *error_mode,
	   MateVFSXferOverwriteMode *overwrite_mode,
	   MateVFSProgressCallbackState *progress,
	   gboolean *skip)
{
	MateVFSResult close_result, result;
	MateVFSHandle *source_handle, *target_handle;

	progress->progress_info->phase = MATE_VFS_XFER_PHASE_OPENSOURCE;
	progress->progress_info->bytes_copied = 0;
        
        progress->progress_info->file_size = info->size;
        progress_set_source_target_uris (progress, source_uri, target_uri);

	result = xfer_open_source (&source_handle, source_uri,
				   progress, xfer_options,
				   error_mode, skip);
	if (*skip) {
		return MATE_VFS_OK;
	}
	
	if (result != MATE_VFS_OK) {
		return result;
	}

	progress->progress_info->phase = MATE_VFS_XFER_PHASE_OPENTARGET;
	result = xfer_create_target (&target_handle, target_uri,
				     progress, xfer_options,
				     error_mode, overwrite_mode,
				     skip);


	if (*skip) {
		mate_vfs_close (source_handle);
		return MATE_VFS_OK;
	}
	if (result != MATE_VFS_OK) {
		mate_vfs_close (source_handle);
		return result;
	}

	if (call_progress_with_uris_often (progress, source_uri, target_uri, 
		MATE_VFS_XFER_PHASE_OPENTARGET) != MATE_VFS_XFER_OVERWRITE_ACTION_ABORT) {

		result = copy_file_data (target_handle, source_handle,
					 progress, xfer_options, error_mode,
					 /* use an arbitrary default block size of 8192
					  * if one isn't available for this file system 
					  */
					 (info->valid_fields & MATE_VFS_FILE_INFO_FIELDS_IO_BLOCK_SIZE && info->io_block_size > 0)
					 ? info->io_block_size : 8192,
					 (target_dir_info->valid_fields & MATE_VFS_FILE_INFO_FIELDS_IO_BLOCK_SIZE && target_dir_info->io_block_size > 0)
					 ? target_dir_info->io_block_size : 8192,
					 skip);
	}

	if (result == MATE_VFS_OK 
		&& call_progress_often (progress, MATE_VFS_XFER_PHASE_CLOSETARGET) == 0) {
		result = MATE_VFS_ERROR_INTERRUPTED;
	}

	close_result = mate_vfs_close (source_handle);
	if (result == MATE_VFS_OK && close_result != MATE_VFS_OK) {
		handle_error (&close_result, progress, error_mode, skip);
		return close_result;
	}

	close_result = mate_vfs_close (target_handle);
	if (result == MATE_VFS_OK && close_result != MATE_VFS_OK) {
		handle_error (&close_result, progress, error_mode, skip);
		return close_result;
	}

	if (result == MATE_VFS_OK) {
		/* ignore errors while setting file info attributes at this point */
		
		if (!(xfer_options & MATE_VFS_XFER_TARGET_DEFAULT_PERMS)) {
			/* FIXME the modules should ignore setting of permissions if
			 * "valid_fields & MATE_VFS_FILE_INFO_FIELDS_PERMISSIONS" is clear
			 * for now, make sure permissions aren't set to 000
			 */
			if ((info->valid_fields & MATE_VFS_FILE_INFO_FIELDS_PERMISSIONS) != 0) {
				/* Call this separately from the time one, since one of them may fail,
				   making the other not run. */
				mate_vfs_set_file_info_uri (target_uri, info, 
							     MATE_VFS_SET_FILE_INFO_OWNER | MATE_VFS_SET_FILE_INFO_PERMISSIONS);
			}
		}
		
		/* Call this last so nothing else changes the times */
		mate_vfs_set_file_info_uri (target_uri, info, MATE_VFS_SET_FILE_INFO_TIME);
	}

	if (*skip) {
		return MATE_VFS_OK;
	}

	return result;
}

static MateVFSResult
copy_directory (MateVFSFileInfo *source_file_info,
		MateVFSURI *source_dir_uri,
		MateVFSURI *target_dir_uri,
		MateVFSXferOptions xfer_options,
		MateVFSXferErrorMode *error_mode,
		MateVFSXferOverwriteMode *overwrite_mode,
		MateVFSProgressCallbackState *progress,
		gboolean *skip)
{
	MateVFSResult result;
	MateVFSDirectoryHandle *source_directory_handle;
	MateVFSDirectoryHandle *dest_directory_handle;
	MateVFSFileInfo *target_dir_info;

	source_directory_handle = NULL;
	dest_directory_handle = NULL;
	target_dir_info = NULL;
	
	result = mate_vfs_directory_open_from_uri (&source_directory_handle, source_dir_uri, 
						    MATE_VFS_FILE_INFO_DEFAULT);

	if (result != MATE_VFS_OK) {
		return result;
	}

	progress->progress_info->bytes_copied = 0;
	if (call_progress_with_uris_often (progress, source_dir_uri, target_dir_uri, 
			       MATE_VFS_XFER_PHASE_COPYING) 
		== MATE_VFS_XFER_OVERWRITE_ACTION_ABORT) {
		return MATE_VFS_ERROR_INTERRUPTED;
	}

	result = create_directory (target_dir_uri, 
				   &dest_directory_handle,
				   xfer_options,
				   error_mode,
				   overwrite_mode,
				   progress,
				   skip);

	if (*skip) {
		mate_vfs_directory_close (source_directory_handle);
		return MATE_VFS_OK;
	}
	
	if (result != MATE_VFS_OK) {
		mate_vfs_directory_close (source_directory_handle);
		return result;
	}

	target_dir_info = mate_vfs_file_info_new ();
	result = mate_vfs_get_file_info_uri (target_dir_uri, target_dir_info,
                                        MATE_VFS_FILE_INFO_DEFAULT);
	if (result != MATE_VFS_OK) {
		mate_vfs_file_info_unref (target_dir_info);
		target_dir_info = NULL;
	}

	if (call_progress_with_uris_often (progress, source_dir_uri, target_dir_uri, 
					   MATE_VFS_XFER_PHASE_OPENTARGET) != 0) {

		progress->progress_info->total_bytes_copied += DEFAULT_SIZE_OVERHEAD;
		progress->progress_info->top_level_item = FALSE;

		/* We do not deal with symlink loops here.  That's OK
		 * because we don't follow symlinks, unless the user
		 * explicitly requests this with
		 * MATE_VFS_XFER_FOLLOW_LINKS_RECURSIVE.
		 */
		do {
			MateVFSURI *source_uri;
			MateVFSURI *dest_uri;
			MateVFSFileInfo *info;
			gboolean skip_child;

			source_uri = NULL;
			dest_uri = NULL;
			info = mate_vfs_file_info_new ();

			result = mate_vfs_directory_read_next (source_directory_handle, info);
			if (result != MATE_VFS_OK) {
				mate_vfs_file_info_unref (info);	
				break;
			}
			if (target_dir_info && MATE_VFS_FILE_INFO_SGID(target_dir_info)) {
				info->gid = target_dir_info->gid;
			}
			
			/* Skip "." and "..".  */
			if (strcmp (info->name, ".") != 0 && strcmp (info->name, "..") != 0) {

				progress->progress_info->file_index++;

				source_uri = mate_vfs_uri_append_file_name (source_dir_uri, info->name);
				dest_uri = mate_vfs_uri_append_file_name (target_dir_uri, info->name);
				progress_set_source_target_uris (progress, source_uri, dest_uri);

				if (info->type == MATE_VFS_FILE_TYPE_REGULAR) {
					result = copy_file (info, target_dir_info,
							    source_uri, dest_uri, 
							    xfer_options, error_mode, overwrite_mode, 
							    progress, &skip_child);
				} else if (info->type == MATE_VFS_FILE_TYPE_DIRECTORY) {
					result = copy_directory (info, source_uri, dest_uri, 
								 xfer_options, error_mode, overwrite_mode, 
								 progress, &skip_child);
				} else if (info->type == MATE_VFS_FILE_TYPE_SYMBOLIC_LINK) {
					if (xfer_options & MATE_VFS_XFER_FOLLOW_LINKS_RECURSIVE) {
						MateVFSFileInfo *symlink_target_info = mate_vfs_file_info_new ();
						result = mate_vfs_get_file_info_uri (source_uri, symlink_target_info,
										      MATE_VFS_FILE_INFO_FOLLOW_LINKS);
						if (result == MATE_VFS_OK) 
							result = copy_file (symlink_target_info, target_dir_info,
									    source_uri, dest_uri, 
									    xfer_options, error_mode, overwrite_mode, 
									    progress, &skip_child);
						mate_vfs_file_info_unref (symlink_target_info);
					} else {
						result = copy_symlink (source_uri, dest_uri, info->symlink_name,
								       error_mode, overwrite_mode, progress, &skip_child);
					}
				}
				/* We don't want to overwrite a previous skip with FALSE, so we only
				 * set it if skip_child actually skiped.
				 */
				if (skip_child)
					*skip = TRUE;
				/* just ignore all the other special file system objects here */
			}
			
			mate_vfs_file_info_unref (info);
			
			if (dest_uri != NULL) {
				mate_vfs_uri_unref (dest_uri);
			}
			if (source_uri != NULL) {
				mate_vfs_uri_unref (source_uri);
			}

		} while (result == MATE_VFS_OK);
	}

	if (result == MATE_VFS_ERROR_EOF) {
		/* all is well, we just finished iterating the directory */
		result = MATE_VFS_OK;
	}

	mate_vfs_directory_close (dest_directory_handle);
	mate_vfs_directory_close (source_directory_handle);

	if (result == MATE_VFS_OK) {			
		MateVFSFileInfo *info;

		if (target_dir_info && MATE_VFS_FILE_INFO_SGID (target_dir_info)) {
			info = mate_vfs_file_info_dup (source_file_info);
			info->gid = target_dir_info->gid;
		} else {
			/* No need to dup the file info in this case */
			mate_vfs_file_info_ref (source_file_info);
			info = source_file_info;
		}

		if (!(xfer_options & MATE_VFS_XFER_TARGET_DEFAULT_PERMS)) {
			/* FIXME the modules should ignore setting of permissions if
			 * "valid_fields & MATE_VFS_FILE_INFO_FIELDS_PERMISSIONS" is clear
			 * for now, make sure permissions aren't set to 000
			 */
			if ((info->valid_fields & MATE_VFS_FILE_INFO_FIELDS_PERMISSIONS) != 0) {
				
				/* Call this separately from the time one, since one of them may fail,
				   making the other not run. */
				mate_vfs_set_file_info_uri (target_dir_uri, info, 
							     MATE_VFS_SET_FILE_INFO_OWNER | 
							     MATE_VFS_SET_FILE_INFO_PERMISSIONS);
			}
		}
		
		/* Call this last so nothing else changes the times */
		mate_vfs_set_file_info_uri (target_dir_uri, info, MATE_VFS_SET_FILE_INFO_TIME);
		mate_vfs_file_info_unref (info);
	}
	
	if (target_dir_info) {
		mate_vfs_file_info_unref (target_dir_info);
	}

	return result;
}

static MateVFSResult
copy_items (const GList *source_uri_list,
	    const GList *target_uri_list,
	    MateVFSXferOptions xfer_options,
	    MateVFSXferErrorMode *error_mode,
	    MateVFSXferOverwriteMode overwrite_mode,
	    MateVFSProgressCallbackState *progress,
	    GList **p_source_uris_copied_list)
{
	MateVFSResult result;
	const GList *source_item, *target_item;
	
	result = MATE_VFS_OK;

	/* go through the list of names */
	for (source_item = source_uri_list, target_item = target_uri_list; source_item != NULL;) {

		MateVFSURI *source_uri;
		MateVFSURI *target_uri;
		MateVFSURI *target_dir_uri;

		MateVFSFileInfo *info;
		gboolean skip;
		int count;
		int progress_result;

		progress->progress_info->file_index++;

		skip = FALSE;
		target_uri = NULL;

		source_uri = (MateVFSURI *)source_item->data;
		target_dir_uri = mate_vfs_uri_get_parent ((MateVFSURI *)target_item->data);
		
		/* get source URI and file info */
		info = mate_vfs_file_info_new ();
		result = mate_vfs_get_file_info_uri (source_uri, info, 
						      MATE_VFS_FILE_INFO_DEFAULT);

		g_free (progress->progress_info->duplicate_name);
		progress->progress_info->duplicate_name =
			mate_vfs_uri_extract_short_path_name
			((MateVFSURI *)target_item->data);

		if (result == MATE_VFS_OK) {
			MateVFSFileInfo *target_dir_info;
			/* get target_dir URI and file info to take care of SGID */
			target_dir_info = mate_vfs_file_info_new ();
			result = mate_vfs_get_file_info_uri (target_dir_uri, target_dir_info,
							      MATE_VFS_FILE_INFO_DEFAULT);
			if (result == MATE_VFS_OK && MATE_VFS_FILE_INFO_SGID(target_dir_info)) {
				info->gid = target_dir_info->gid;
			}
			result = MATE_VFS_OK;

			/* optionally keep trying until we hit a unique target name */
			for (count = 1; ; count++) {
				MateVFSXferOverwriteMode overwrite_mode_abort;

				target_uri = mate_vfs_uri_append_string
					(target_dir_uri, 
					 progress->progress_info->duplicate_name);

				progress->progress_info->status = MATE_VFS_XFER_PROGRESS_STATUS_OK;
				progress->progress_info->file_size = info->size;
				progress->progress_info->bytes_copied = 0;
				progress->progress_info->top_level_item = TRUE;

				if (call_progress_with_uris_often (progress, source_uri, target_uri, 
						       MATE_VFS_XFER_PHASE_COPYING) == 0) {
					result = MATE_VFS_ERROR_INTERRUPTED;
				}

				/* If we're creating unique names, always abort on overwrite and
				 * calculate a new name */
				overwrite_mode_abort = overwrite_mode;
				if ((xfer_options & MATE_VFS_XFER_USE_UNIQUE_NAMES) != 0) {
					overwrite_mode_abort = MATE_VFS_XFER_OVERWRITE_MODE_ABORT;
				} 
				
				if (info->type == MATE_VFS_FILE_TYPE_REGULAR) {
					result = copy_file (info, target_dir_info,
							    source_uri, target_uri, 
							    xfer_options, error_mode, 
							    &overwrite_mode_abort, 
							    progress, &skip);
				} else if (info->type == MATE_VFS_FILE_TYPE_DIRECTORY) {
					result = copy_directory (info, source_uri, target_uri, 
								 xfer_options, error_mode,
								 &overwrite_mode_abort,
								 progress, &skip);
                                } else if (info->type == MATE_VFS_FILE_TYPE_SYMBOLIC_LINK) {
					result = copy_symlink (source_uri, target_uri, info->symlink_name,
							       error_mode, &overwrite_mode_abort,
							       progress, &skip);
                                }
				/* just ignore all the other special file system objects here */

				if (result == MATE_VFS_OK && !skip) {
					/* Add to list of successfully copied files... */
					*p_source_uris_copied_list = g_list_prepend (*p_source_uris_copied_list, source_uri);
					mate_vfs_uri_ref (source_uri);
				}

				if (result != MATE_VFS_ERROR_FILE_EXISTS) {
					/* whatever happened, it wasn't a name conflict */
					mate_vfs_uri_unref (target_uri);
					break;
				}

				if ((xfer_options & MATE_VFS_XFER_USE_UNIQUE_NAMES) == 0) {
					mate_vfs_uri_unref (target_uri);
					break;
				}

				/* pass in the current target name, progress will update it to 
				 * a new unique name such as 'foo (copy)' or 'bar (copy 2)'
				 */
				g_free (progress->progress_info->duplicate_name);
				progress->progress_info->duplicate_name = 
					mate_vfs_uri_extract_short_path_name
					((MateVFSURI *)target_item->data);
				progress->progress_info->duplicate_count = count;
				progress->progress_info->status = MATE_VFS_XFER_PROGRESS_STATUS_DUPLICATE;
				progress->progress_info->vfs_status = result;
				progress_result = call_progress_uri (progress, source_uri, target_uri, 
						       MATE_VFS_XFER_PHASE_COPYING);
				progress->progress_info->status = MATE_VFS_XFER_PROGRESS_STATUS_OK;

				mate_vfs_uri_unref (target_uri);

				if (progress_result == MATE_VFS_XFER_OVERWRITE_ACTION_ABORT) {
					break;
				}

				if (skip) {
					break;
				}
				
				/* try again with new uri */
			}

			mate_vfs_file_info_unref (target_dir_info);
		}

		mate_vfs_file_info_unref (info);

		if (result != MATE_VFS_OK) {
			break;
		}

		mate_vfs_uri_unref (target_dir_uri);

		source_item = source_item->next;
		target_item = target_item->next;

		g_assert ((source_item != NULL) == (target_item != NULL));
	}

	return result;
}

static MateVFSResult
move_items (const GList *source_uri_list,
	    const GList *target_uri_list,
	    MateVFSXferOptions xfer_options,
	    MateVFSXferErrorMode *error_mode,
	    MateVFSXferOverwriteMode *overwrite_mode,
	    MateVFSProgressCallbackState *progress)
{
	MateVFSResult result;
	const GList *source_item, *target_item;
	
	result = MATE_VFS_OK;

	/* go through the list of names */
	for (source_item = source_uri_list, target_item = target_uri_list; source_item != NULL;) {

		MateVFSURI *source_uri;
		MateVFSURI *target_uri;
		MateVFSURI *target_dir_uri;
		gboolean retry;
		gboolean skip;
		int conflict_count;
		int progress_result;

		progress->progress_info->file_index++;

		source_uri = (MateVFSURI *)source_item->data;
		target_dir_uri = mate_vfs_uri_get_parent ((MateVFSURI *)target_item->data);

		g_free (progress->progress_info->duplicate_name);
		progress->progress_info->duplicate_name =  
			mate_vfs_uri_extract_short_path_name
			((MateVFSURI *)target_item->data);

		skip = FALSE;
		conflict_count = 1;

		do {
			retry = FALSE;
			target_uri = mate_vfs_uri_append_string (target_dir_uri, 
				 progress->progress_info->duplicate_name);

			progress->progress_info->file_size = 0;
			progress->progress_info->bytes_copied = 0;
			progress_set_source_target_uris (progress, source_uri, target_uri);
			progress->progress_info->top_level_item = TRUE;

			/* no matter what the replace mode, just overwrite the destination
			 * handle_name_conflicts took care of conflicting files
			 */
			result = mate_vfs_move_uri (source_uri, target_uri, 
						     (xfer_options & MATE_VFS_XFER_USE_UNIQUE_NAMES) != 0
						     ? MATE_VFS_XFER_OVERWRITE_MODE_ABORT
						     : MATE_VFS_XFER_OVERWRITE_MODE_REPLACE);


			if (result == MATE_VFS_ERROR_FILE_EXISTS) {
				/* deal with a name conflict -- ask the progress_callback for a better name */
				g_free (progress->progress_info->duplicate_name);
				progress->progress_info->duplicate_name =
					mate_vfs_uri_extract_short_path_name
					((MateVFSURI *)target_item->data);
				progress->progress_info->duplicate_count = conflict_count;
				progress->progress_info->status = MATE_VFS_XFER_PROGRESS_STATUS_DUPLICATE;
				progress->progress_info->vfs_status = result;
				progress_result = call_progress_uri (progress, source_uri, target_uri, 
						       MATE_VFS_XFER_PHASE_COPYING);
				progress->progress_info->status = MATE_VFS_XFER_PROGRESS_STATUS_OK;

				mate_vfs_uri_unref (target_uri);

				if (progress_result == MATE_VFS_XFER_OVERWRITE_ACTION_ABORT)
					break;

				conflict_count++;
				retry = TRUE;
				continue;
			}

			if (result != MATE_VFS_OK) {
				retry = handle_error (&result, progress, error_mode, &skip);
			}

			if (result == MATE_VFS_OK 
			    && !skip
			    && call_progress_with_uris_often (progress, source_uri,
					target_uri, MATE_VFS_XFER_PHASE_MOVING) == 0) {
				result = MATE_VFS_ERROR_INTERRUPTED;
				mate_vfs_uri_unref (target_uri);
				break;
			}
			mate_vfs_uri_unref (target_uri);
		} while (retry);
		
		mate_vfs_uri_unref (target_dir_uri);

		if (result != MATE_VFS_OK && !skip)
			break;

		source_item = source_item->next;
		target_item = target_item->next;
		g_assert ((source_item != NULL) == (target_item != NULL));
	}

	return result;
}

static MateVFSResult
link_items (const GList *source_uri_list,
	    const GList *target_uri_list,
	    MateVFSXferOptions xfer_options,
	    MateVFSXferErrorMode *error_mode,
	    MateVFSXferOverwriteMode *overwrite_mode,
	    MateVFSProgressCallbackState *progress)
{
	MateVFSResult result;
	const GList *source_item, *target_item;
	MateVFSURI *source_uri;
	MateVFSURI *target_dir_uri;
	MateVFSURI *target_uri;
	gboolean retry;
	gboolean skip;
	int conflict_count;
	int progress_result;
	char *source_reference;

	result = MATE_VFS_OK;

	/* go through the list of names, create a link to each item */
	for (source_item = source_uri_list, target_item = target_uri_list; source_item != NULL;) {

		progress->progress_info->file_index++;

		source_uri = (MateVFSURI *)source_item->data;
		source_reference = mate_vfs_uri_to_string (source_uri, MATE_VFS_URI_HIDE_NONE);

		target_dir_uri = mate_vfs_uri_get_parent ((MateVFSURI *)target_item->data);

		g_free (progress->progress_info->duplicate_name);
		progress->progress_info->duplicate_name =
			mate_vfs_uri_extract_short_path_name
			((MateVFSURI *)target_item->data);

		skip = FALSE;
		conflict_count = 1;

		do {
			retry = FALSE;
			target_uri = mate_vfs_uri_append_string
				(target_dir_uri,
				 progress->progress_info->duplicate_name);

			progress->progress_info->file_size = 0;
			progress->progress_info->bytes_copied = 0;
			progress->progress_info->top_level_item = TRUE;
			progress_set_source_target_uris (progress, source_uri, target_uri);

			/* no matter what the replace mode, just overwrite the destination
			 * handle_name_conflicts took care of conflicting files
			 */
			result = mate_vfs_create_symbolic_link (target_uri, source_reference); 
			if (result == MATE_VFS_ERROR_FILE_EXISTS) {
				/* deal with a name conflict -- ask the progress_callback for a better name */
				g_free (progress->progress_info->duplicate_name);
				progress->progress_info->duplicate_name =
					mate_vfs_uri_extract_short_path_name
					((MateVFSURI *)target_item->data);
				progress->progress_info->duplicate_count = conflict_count;
				progress->progress_info->status = MATE_VFS_XFER_PROGRESS_STATUS_DUPLICATE;
				progress->progress_info->vfs_status = result;
				progress_result = call_progress_uri (progress, source_uri, target_uri, 
						       MATE_VFS_XFER_PHASE_COPYING);
				progress->progress_info->status = MATE_VFS_XFER_PROGRESS_STATUS_OK;

				mate_vfs_uri_unref (target_uri);

				if (progress_result == MATE_VFS_XFER_OVERWRITE_ACTION_ABORT)
					break;

				conflict_count++;
				retry = TRUE;
				continue;
			}
			
			if (result != MATE_VFS_OK) {
				retry = handle_error (&result, progress, error_mode, &skip);
			}

			if (result == MATE_VFS_OK
				&& call_progress_with_uris_often (progress, source_uri,
						target_uri, MATE_VFS_XFER_PHASE_OPENTARGET) == 0) {
				result = MATE_VFS_ERROR_INTERRUPTED;
				mate_vfs_uri_unref (target_uri);
				break;
			}
			mate_vfs_uri_unref (target_uri);
		} while (retry);
		
		mate_vfs_uri_unref (target_dir_uri);
		g_free (source_reference);

		if (result != MATE_VFS_OK && !skip)
			break;

		source_item = source_item->next;
		target_item = target_item->next;
		g_assert ((source_item != NULL) == (target_item != NULL));
	}

	return result;
}


static MateVFSResult
mate_vfs_xfer_empty_directories (const GList *trash_dir_uris,
			          MateVFSXferErrorMode error_mode,
			          MateVFSProgressCallbackState *progress)
{
	MateVFSResult result;
	const GList *p;
	gboolean skip;

	result = MATE_VFS_OK;

		/* initialize the results */
	progress->progress_info->files_total = 0;
	progress->progress_info->bytes_total = 0;
	progress->progress_info->phase = MATE_VFS_XFER_PHASE_COLLECTING;


	for (p = trash_dir_uris;  p != NULL; p = p->next) {
		result = directory_add_items_and_size (p->data,
			MATE_VFS_XFER_REMOVESOURCE | MATE_VFS_XFER_RECURSIVE, 
			progress);
		if (result == MATE_VFS_ERROR_TOO_MANY_OPEN_FILES) {
			/* out of file descriptors -- we will deal with that */
			result = MATE_VFS_OK;
			break;
		}
		/* set up a fake total size to represent the bulk of the operation
		 * -- we'll subtract a proportional value for every deletion
		 */
		progress->progress_info->bytes_total 
			= progress->progress_info->files_total * DEFAULT_SIZE_OVERHEAD;
	}
	call_progress (progress, MATE_VFS_XFER_PHASE_READYTOGO);
	for (p = trash_dir_uris;  p != NULL; p = p->next) {
		result = empty_directory ((MateVFSURI *)p->data, progress, 
			MATE_VFS_XFER_REMOVESOURCE | MATE_VFS_XFER_RECURSIVE, 
			&error_mode, &skip);
		if (result == MATE_VFS_ERROR_TOO_MANY_OPEN_FILES) {
			result = non_recursive_empty_directory ((MateVFSURI *)p->data, 
				progress, MATE_VFS_XFER_REMOVESOURCE | MATE_VFS_XFER_RECURSIVE, 
				&error_mode, &skip);
		}
	}

	return result;
}


static MateVFSResult
mate_vfs_xfer_delete_items_common (const GList *source_uri_list,
			            MateVFSXferErrorMode error_mode,
			            MateVFSXferOptions xfer_options,
			            MateVFSProgressCallbackState *progress)
{
	MateVFSFileInfo *info;
	MateVFSResult result;
	MateVFSURI *uri;
	const GList *p;
	gboolean skip;

	result = MATE_VFS_OK;
	
	for (p = source_uri_list;  p != NULL; p = p->next) {
	
		skip = FALSE;
		/* get the URI and VFSFileInfo for each */
		uri = p->data;

		info = mate_vfs_file_info_new ();
		result = mate_vfs_get_file_info_uri (uri, info, 
						      MATE_VFS_FILE_INFO_DEFAULT);

		if (result != MATE_VFS_OK) {
			mate_vfs_file_info_unref (info);
			break;
		}

		progress_set_source_target_uris (progress, uri, NULL);
		if (info->type == MATE_VFS_FILE_TYPE_DIRECTORY) {
			result = remove_directory (uri, TRUE, progress, xfer_options, 
						   &error_mode, &skip);
		} else {
			result = remove_file (uri, progress, xfer_options, &error_mode,
					      &skip);
		}

		mate_vfs_file_info_unref (info);
	}

	return result;
}


static MateVFSResult
mate_vfs_xfer_delete_items (const GList *source_uri_list,
			     MateVFSXferErrorMode error_mode,
			     MateVFSXferOptions xfer_options,
			     MateVFSProgressCallbackState *progress)
{

	MateVFSResult result;
		
		/* initialize the results */
	progress->progress_info->files_total = 0;
	progress->progress_info->bytes_total = 0;
	call_progress (progress, MATE_VFS_XFER_PHASE_COLLECTING);

	result = count_items_and_size (source_uri_list,
		MATE_VFS_XFER_REMOVESOURCE | MATE_VFS_XFER_RECURSIVE, progress, FALSE, FALSE);

	/* When deleting, ignore the real file sizes, just count the same DEFAULT_SIZE_OVERHEAD
	 * for each file.
	 */
	progress->progress_info->bytes_total
		= progress->progress_info->files_total * DEFAULT_SIZE_OVERHEAD;
	progress->progress_info->top_level_item = TRUE;
	if (result != MATE_VFS_ERROR_INTERRUPTED) {
		call_progress (progress, MATE_VFS_XFER_PHASE_READYTOGO);
		result = mate_vfs_xfer_delete_items_common (source_uri_list,
			error_mode, xfer_options, progress);
	}

	return result;
}


static MateVFSResult
mate_vfs_new_directory_with_unique_name (const MateVFSURI *target_dir_uri,
					  const char *name,
					  MateVFSXferErrorMode error_mode,
					  MateVFSXferOverwriteMode overwrite_mode,
					  MateVFSProgressCallbackState *progress)
{
	MateVFSResult result;
	MateVFSURI *target_uri;
	MateVFSDirectoryHandle *dest_directory_handle;
	gboolean dummy;
	int progress_result;
	int conflict_count;
	
	dest_directory_handle = NULL;
	progress->progress_info->top_level_item = TRUE;
	progress->progress_info->duplicate_name = g_strdup (name);

	for (conflict_count = 1; ; conflict_count++) {

		target_uri = mate_vfs_uri_append_string
			(target_dir_uri, 
			 progress->progress_info->duplicate_name);
		result = create_directory (target_uri, 
					   &dest_directory_handle,
					   MATE_VFS_XFER_USE_UNIQUE_NAMES,
					   &error_mode,
					   &overwrite_mode,
					   progress,
					   &dummy);

		if (result != MATE_VFS_ERROR_FILE_EXISTS
			&& result != MATE_VFS_ERROR_NAME_TOO_LONG) {
			break;
		}

		/* deal with a name conflict -- ask the progress_callback for a better name */
		g_free (progress->progress_info->duplicate_name);
		progress->progress_info->duplicate_name = g_strdup (name);
		progress->progress_info->duplicate_count = conflict_count;
		progress->progress_info->status = MATE_VFS_XFER_PROGRESS_STATUS_DUPLICATE;
		progress->progress_info->vfs_status = result;
		progress_result = call_progress_uri (progress, NULL, target_uri, 
				       MATE_VFS_XFER_PHASE_COPYING);
		progress->progress_info->status = MATE_VFS_XFER_PROGRESS_STATUS_OK;

		if (progress_result == MATE_VFS_XFER_OVERWRITE_ACTION_ABORT) {
			break;
		}

		mate_vfs_uri_unref (target_uri);
	}

	progress->progress_info->vfs_status = result;
	
	call_progress_uri (progress, NULL, target_uri,
		MATE_VFS_XFER_PHASE_OPENTARGET);

	if (dest_directory_handle != NULL) {
		mate_vfs_directory_close (dest_directory_handle);
	}

	mate_vfs_uri_unref (target_uri);

	return result;
}

static MateVFSResult
mate_vfs_destination_is_writable (MateVFSURI *uri)
{
	MateVFSResult result;
	MateVFSFileInfo *info;
	
	info   = mate_vfs_file_info_new ();	    
	result = mate_vfs_get_file_info_uri (uri, info,
					      MATE_VFS_FILE_INFO_GET_ACCESS_RIGHTS);
	
		
	if (result != MATE_VFS_OK) {
		mate_vfs_file_info_unref (info);
		return result;
	}

	/* Default to MATE_VFS_OK so we return that if we don't have support
	   for MATE_VFS_FILE_INFO_GET_ACCESS_RIGHTS */
	result = MATE_VFS_OK;
	
	if (info->valid_fields & MATE_VFS_FILE_INFO_FIELDS_ACCESS) {
		
		if (! (info->permissions & MATE_VFS_PERM_ACCESS_WRITABLE &&
		       info->permissions & MATE_VFS_PERM_ACCESS_EXECUTABLE)) {
			result = MATE_VFS_ERROR_ACCESS_DENIED;
		}
	}

	mate_vfs_file_info_unref (info);
	return result;
}

static MateVFSResult
mate_vfs_xfer_uri_internal (const GList *source_uris,
			     const GList *target_uris,
			     MateVFSXferOptions xfer_options,
			     MateVFSXferErrorMode error_mode,
			     MateVFSXferOverwriteMode overwrite_mode,
			     MateVFSProgressCallbackState *progress)
{
	MateVFSResult result;
	GList *source_uri_list, *target_uri_list;
	GList *source_uri, *target_uri;
	GList *source_uri_list_copied;
	GList *merge_source_uri_list, *merge_target_uri_list;
	MateVFSURI *target_dir_uri;
	gboolean move, link;
	MateVFSFileSize free_bytes;
	MateVFSFileSize bytes_total;
	gulong files_total;
	gboolean skip;
	
	result = MATE_VFS_OK;
	move = FALSE;
	link = FALSE;
	target_dir_uri = NULL;
	source_uri_list_copied = NULL;

	/* Check and see if target is writable. Return error if it is not. */
	call_progress (progress, MATE_VFS_XFER_CHECKING_DESTINATION);
	target_dir_uri = mate_vfs_uri_get_parent ((MateVFSURI *)((GList *)target_uris)->data);
	result = mate_vfs_destination_is_writable (target_dir_uri);
	progress_set_source_target_uris (progress, NULL, target_dir_uri);
	if (result != MATE_VFS_OK) {
		handle_error (&result, progress, &error_mode, &skip);
		mate_vfs_uri_unref (target_dir_uri);
		return result;
	}

	move = (xfer_options & MATE_VFS_XFER_REMOVESOURCE) != 0;
	link = (xfer_options & MATE_VFS_XFER_LINK_ITEMS) != 0;

	if (move && link) {
		return MATE_VFS_ERROR_BAD_PARAMETERS;
	}

	/* Create an owning list of source and destination uris.
	 * We want to be able to remove items that we decide to skip during
	 * name conflict check.
	 */
	source_uri_list = mate_vfs_uri_list_copy ((GList *)source_uris);
	target_uri_list = mate_vfs_uri_list_copy ((GList *)target_uris);
	merge_source_uri_list = NULL;
	merge_target_uri_list = NULL;

	if ((xfer_options & MATE_VFS_XFER_USE_UNIQUE_NAMES) == 0) {
		/* see if moved files are on the same file system so that we decide to do
		 * a simple move or have to do a copy/remove
		 */
		for (source_uri = source_uri_list, target_uri = target_uri_list;
			source_uri != NULL;
			source_uri = source_uri->next, target_uri = target_uri->next) {
			gboolean same_fs;

			g_assert (target_dir_uri != NULL);

			result = mate_vfs_check_same_fs_uris ((MateVFSURI *)source_uri->data, 
				target_dir_uri, &same_fs);

			if (result != MATE_VFS_OK) {
				break;
			}

			move &= same_fs;
		}
	}

	if (target_dir_uri != NULL) {
		mate_vfs_uri_unref (target_dir_uri);
		target_dir_uri = NULL;
	}
	
	if (result == MATE_VFS_OK) {
		call_progress (progress, MATE_VFS_XFER_PHASE_COLLECTING);
		result = count_items_and_size (source_uri_list, xfer_options, progress, move, link);
		if (result != MATE_VFS_ERROR_INTERRUPTED) {
			/* Ignore anything but interruptions here -- we will deal with the errors
			 * during the actual copy
			 */
			result = MATE_VFS_OK;
		}
	}
			
	if (result == MATE_VFS_OK) {
		/* Calculate free space on destination. If an error is returned, we have a non-local
		 * file system, so we just forge ahead and hope for the best 
		 */
		target_dir_uri = mate_vfs_uri_get_parent ((MateVFSURI *)target_uri_list->data);
		result = mate_vfs_get_volume_free_space (target_dir_uri, &free_bytes);

		if (result == MATE_VFS_OK) {
			if (!move && !link && progress->progress_info->bytes_total > free_bytes) {
				result = MATE_VFS_ERROR_NO_SPACE;
				progress_set_source_target_uris (progress, NULL, target_dir_uri);
				handle_error (&result, progress, &error_mode, &skip);
			}
		} else {
			/* Errors from mate_vfs_get_volume_free_space should be ignored */
			result = MATE_VFS_OK;
		}
		
		if (target_dir_uri != NULL) {
			mate_vfs_uri_unref (target_dir_uri);
			target_dir_uri = NULL;
		}

		if (result != MATE_VFS_OK) {
			return result;
		}
			
		if ((xfer_options & MATE_VFS_XFER_USE_UNIQUE_NAMES) == 0) {
		
			/* Save the preflight numbers, handle_name_conflicts would overwrite them */
			bytes_total = progress->progress_info->bytes_total;
			files_total = progress->progress_info->files_total;

			/* Set the preflight numbers to 0, we don't want to run progress on the
			 * removal of conflicting items.
			 */
			progress->progress_info->bytes_total = 0;
			progress->progress_info->files_total = 0;

			result = handle_name_conflicts (&source_uri_list, &target_uri_list,
						        xfer_options, &error_mode, &overwrite_mode,
						        progress,
							move,
							link,
							&merge_source_uri_list,
							&merge_target_uri_list);

			progress->progress_info->bytes_total = 0;
			progress->progress_info->files_total = 0;
			
			if (result == MATE_VFS_OK && move && merge_source_uri_list != NULL) {
				/* Some moves was converted to copy,
				   remove previously added non-recursive sizes,
				   and add recursive sizes */

				result = list_add_items_and_size (merge_source_uri_list, xfer_options, progress, FALSE);
				if (result != MATE_VFS_ERROR_INTERRUPTED) {
					/* Ignore anything but interruptions here -- we will deal with the errors
					 * during the actual copy
					 */
					result = MATE_VFS_OK;
				}

				progress->progress_info->bytes_total = -progress->progress_info->bytes_total;
				progress->progress_info->files_total = -progress->progress_info->files_total;
				
				if (result == MATE_VFS_OK) {
					result = list_add_items_and_size (merge_source_uri_list, xfer_options, progress, TRUE);
					if (result != MATE_VFS_ERROR_INTERRUPTED) {
						/* Ignore anything but interruptions here -- we will deal with the errors
						 * during the actual copy
						 */
						result = MATE_VFS_OK;
					}
				}
				
				if (result == MATE_VFS_OK) {
					/* We're moving, and some moves were converted to copies.
					 * Make sure we have space for the copies.
					 */
					target_dir_uri = mate_vfs_uri_get_parent ((MateVFSURI *)merge_target_uri_list->data);
					result = mate_vfs_get_volume_free_space (target_dir_uri, &free_bytes);
					
					if (result == MATE_VFS_OK) {
						if (progress->progress_info->bytes_total > free_bytes) {
							result = MATE_VFS_ERROR_NO_SPACE;
							progress_set_source_target_uris (progress, NULL, target_dir_uri);
						}
					} else {
						/* Errors from mate_vfs_get_volume_free_space should be ignored */
						result = MATE_VFS_OK;
					}
					
					if (target_dir_uri != NULL) {
						mate_vfs_uri_unref (target_dir_uri);
						target_dir_uri = NULL;
					}
				}
			}

			/* Add previous size (non-copy size in the case of a move) */
			progress->progress_info->bytes_total += bytes_total;
			progress->progress_info->files_total += files_total;
		}


		/* reset the preflight numbers */
		progress->progress_info->file_index = 0;
		progress->progress_info->total_bytes_copied = 0;

		if (result != MATE_VFS_OK) {
			if (handle_error (&result, progress, &error_mode, &skip)) {
				/* whatever error it was, we handled it */
				result = MATE_VFS_OK;
			}
		} else {
			call_progress (progress, MATE_VFS_XFER_PHASE_READYTOGO);

			if (move) {
				g_assert (!link);
				result = move_items (source_uri_list, target_uri_list,
						     xfer_options, &error_mode, &overwrite_mode, 
						     progress);
				if (result == MATE_VFS_OK && merge_source_uri_list != NULL) {
					result = copy_items (merge_source_uri_list, merge_target_uri_list,
							     xfer_options, &error_mode, overwrite_mode, 
							     progress, &source_uri_list_copied);
				}
			} else if (link) {
				result = link_items (source_uri_list, target_uri_list,
						     xfer_options, &error_mode, &overwrite_mode,
						     progress);
			} else {
				result = copy_items (source_uri_list, target_uri_list,
						     xfer_options, &error_mode, overwrite_mode, 
						     progress, &source_uri_list_copied);
			}
			
			if (result == MATE_VFS_OK) {
				if (xfer_options & MATE_VFS_XFER_REMOVESOURCE
				    && !link) {
					call_progress (progress, MATE_VFS_XFER_PHASE_CLEANUP);
					result = mate_vfs_xfer_delete_items_common ( 
						 	source_uri_list_copied,
						 	error_mode, xfer_options, progress);
				}
			}
		}
	}

	mate_vfs_uri_list_free (source_uri_list);
	mate_vfs_uri_list_free (target_uri_list);
	mate_vfs_uri_list_free (merge_source_uri_list);
	mate_vfs_uri_list_free (merge_target_uri_list);
	mate_vfs_uri_list_free (source_uri_list_copied);

	return result;
}

MateVFSResult
_mate_vfs_xfer_private (const GList *source_uri_list,
			const GList *target_uri_list,
			MateVFSXferOptions xfer_options,
			MateVFSXferErrorMode error_mode,
			MateVFSXferOverwriteMode overwrite_mode,
			MateVFSXferProgressCallback progress_callback,
			gpointer data,
			MateVFSXferProgressCallback sync_progress_callback,
			gpointer sync_progress_data)
{
	MateVFSProgressCallbackState progress_state;
	MateVFSXferProgressInfo progress_info;
	MateVFSURI *target_dir_uri;
	MateVFSResult result;
	char *short_name;
	
	init_progress (&progress_state, &progress_info);
	progress_state.sync_callback = sync_progress_callback;
	progress_state.user_data = sync_progress_data;
	progress_state.update_callback = progress_callback;
	progress_state.async_job_data = data;


	if ((xfer_options & MATE_VFS_XFER_EMPTY_DIRECTORIES) != 0) {
		/* Directory empty operation (Empty Trash, etc.). */
		g_assert (source_uri_list != NULL);
		g_assert (target_uri_list == NULL);
		
		call_progress (&progress_state, MATE_VFS_XFER_PHASE_INITIAL);
	 	result = mate_vfs_xfer_empty_directories (source_uri_list, error_mode, &progress_state);
	} else if ((xfer_options & MATE_VFS_XFER_DELETE_ITEMS) != 0) {
		/* Delete items operation */
		g_assert (source_uri_list != NULL);
		g_assert (target_uri_list == NULL);
				
		call_progress (&progress_state, MATE_VFS_XFER_PHASE_INITIAL);
		result = mate_vfs_xfer_delete_items (source_uri_list,
		      error_mode, xfer_options, &progress_state);
	} else if ((xfer_options & MATE_VFS_XFER_NEW_UNIQUE_DIRECTORY) != 0) {
		/* New directory create operation */
		g_assert (source_uri_list == NULL);
		g_assert (g_list_length ((GList *) target_uri_list) == 1);

		target_dir_uri = mate_vfs_uri_get_parent ((MateVFSURI *) target_uri_list->data);
		result = MATE_VFS_ERROR_INVALID_URI;
		if (target_dir_uri != NULL) { 
			short_name = mate_vfs_uri_extract_short_path_name ((MateVFSURI *) target_uri_list->data);
			result = mate_vfs_new_directory_with_unique_name (target_dir_uri, short_name,
									   error_mode, overwrite_mode, &progress_state);
			g_free (short_name);
			mate_vfs_uri_unref (target_dir_uri);
		}
	} else {
		/* Copy items operation */
		g_assert (source_uri_list != NULL);
		g_assert (target_uri_list != NULL);
		g_assert (g_list_length ((GList *)source_uri_list) == g_list_length ((GList *)target_uri_list));

		call_progress (&progress_state, MATE_VFS_XFER_PHASE_INITIAL);
		result = mate_vfs_xfer_uri_internal (source_uri_list, target_uri_list,
			xfer_options, error_mode, overwrite_mode, &progress_state);
	}

	call_progress (&progress_state, MATE_VFS_XFER_PHASE_COMPLETED);
	free_progress (&progress_info);

	/* FIXME bugzilla.eazel.com 1218:
	 * 
	 * The async job setup will try to call the callback function with the callback data
	 * even though they are usually dead at this point because the callback detected
	 * that we are giving up and cleaned up after itself.
	 * 
	 * Should fix this in the async job call setup.
	 * 
	 * For now just pretend everything worked well.
	 * 
	 */
	result = MATE_VFS_OK;

	return result;
}

/**
 * mate_vfs_xfer_uri_list:
 * @source_uri_list: A #GList of source #MateVFSURIs.
 * @target_uri_list: A #GList of target #MateVFSURIs, each corresponding to one URI in
 * @source_uri_list.
 * @xfer_options: #MateVFSXferOptions defining the desired operation and parameters.
 * @error_mode: A #MateVFSErrorMode specifying how to proceed if a VFS error occurs.
 * @overwrite_mode: A #MateVFSOverwriteMode specifying how to proceed if a file is being overwritten.
 * @progress_callback: This #MateVFSProgressCallback is used to inform the user about
 * the progress of a transfer, and to request further input from him if a problem occurs.
 * @data: Data to be passed back in callbacks from the xfer engine.
 *
 * This function will transfer multiple files to multiple targets, given
 * source URIs and destination URIs. If you want to do asynchronous
 * file transfers, you have to use mate_vfs_async_xfer() instead.
 *
 * Returns: If all goes well it returns %MATE_VFS_OK.  Check #MateVFSResult for
 * other values.
 **/
MateVFSResult
mate_vfs_xfer_uri_list (const GList *source_uri_list,
			 const GList *target_uri_list,
			 MateVFSXferOptions xfer_options,
			 MateVFSXferErrorMode error_mode,
			 MateVFSXferOverwriteMode overwrite_mode,
			 MateVFSXferProgressCallback progress_callback,
			 gpointer data)
{
	MateVFSProgressCallbackState progress_state;
	MateVFSXferProgressInfo progress_info;
	MateVFSResult result;

	g_return_val_if_fail (source_uri_list != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (target_uri_list != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);	
	g_return_val_if_fail (progress_callback != NULL || error_mode != MATE_VFS_XFER_ERROR_MODE_QUERY,
			      MATE_VFS_ERROR_BAD_PARAMETERS);	
		
	init_progress (&progress_state, &progress_info);
	progress_state.sync_callback = progress_callback;
	progress_state.user_data = data;

	call_progress (&progress_state, MATE_VFS_XFER_PHASE_INITIAL);

	result = mate_vfs_xfer_uri_internal (source_uri_list, target_uri_list,
		xfer_options, error_mode, overwrite_mode, &progress_state);

	call_progress (&progress_state, MATE_VFS_XFER_PHASE_COMPLETED);
	free_progress (&progress_info);

	return result;
}

/**
 * mate_vfs_xfer_uri:
 * @source_uri: A source #MateVFSURI.
 * @target_uri: A target #MateVFSURI.
 * @xfer_options: #MateVFSXferOptions defining the desired operation and parameters.
 * @error_mode: A #MateVFSErrorMode specifying how to proceed if a VFS error occurs.
 * @overwrite_mode: A #MateVFSOverwriteMode specifying how to proceed if a file is being overwritten.
 * @progress_callback: This #MateVFSProgressCallback is used to inform the user about
 * the progress of a transfer, and to request further input from him if a problem occurs.
 * @data: Data to be passed back in callbacks from the xfer engine.
 * 
 * This function works exactly like mate_vfs_xfer_uri_list(), and is a
 * convenience wrapper for only acting on one source/target URI pair.
 *
 * Return value: an integer representing the result of the operation.
 **/
MateVFSResult	
mate_vfs_xfer_uri (const MateVFSURI *source_uri,
		    const MateVFSURI *target_uri,
		    MateVFSXferOptions xfer_options,
		    MateVFSXferErrorMode error_mode,
		    MateVFSXferOverwriteMode overwrite_mode,
		    MateVFSXferProgressCallback progress_callback,
		    gpointer data)
{
	GList *source_uri_list, *target_uri_list;
	MateVFSResult result;

	g_return_val_if_fail (source_uri != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (target_uri != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);	
	g_return_val_if_fail (progress_callback != NULL || error_mode != MATE_VFS_XFER_ERROR_MODE_QUERY,
			      MATE_VFS_ERROR_BAD_PARAMETERS);	

	source_uri_list = g_list_append (NULL, (void *)source_uri);
	target_uri_list = g_list_append (NULL, (void *)target_uri);

	result = mate_vfs_xfer_uri_list (source_uri_list, target_uri_list,
		xfer_options, error_mode, overwrite_mode, progress_callback, data);

	g_list_free (source_uri_list);
	g_list_free (target_uri_list);

	return result;
}

/**
 * mate_vfs_xfer_delete_list:
 * @source_uri_list: This is a list containing uris.
 * @error_mode: Decide how you want to deal with interruptions.
 * @xfer_options: Set whatever transfer options you need.
 * @progress_callback: Callback to check on progress of transfer.
 * @data: Data to be passed back in callbacks from the xfer engine.
 *
 * Unlinks items in the @source_uri_list from their filesystems.
 *
 * Return value: %MATE_VFS_OK if successful, or the appropriate error code otherwise.
 **/
MateVFSResult 
mate_vfs_xfer_delete_list (const GList *source_uri_list, 
                            MateVFSXferErrorMode error_mode,
                            MateVFSXferOptions xfer_options,
		            MateVFSXferProgressCallback progress_callback,
                            gpointer data)
{
	MateVFSProgressCallbackState progress_state;
	MateVFSXferProgressInfo progress_info;
	MateVFSResult result;

	g_return_val_if_fail (source_uri_list != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (progress_callback != NULL || error_mode != MATE_VFS_XFER_ERROR_MODE_QUERY,
			      MATE_VFS_ERROR_BAD_PARAMETERS);

	init_progress (&progress_state, &progress_info);
	progress_state.sync_callback = progress_callback;
	progress_state.user_data = data;
	call_progress (&progress_state, MATE_VFS_XFER_PHASE_INITIAL);

	result = mate_vfs_xfer_delete_items (source_uri_list, error_mode, xfer_options,
		&progress_state);
	
	call_progress (&progress_state, MATE_VFS_XFER_PHASE_COMPLETED);
	free_progress (&progress_info);

	return result;
}

