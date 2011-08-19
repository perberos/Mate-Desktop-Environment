/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-monitor.c - File Monitoring for the MATE Virtual File System.

   Copyright (C) 2001 Ian McKellar

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

   Author: Ian McKellar <yakk@yakk.net>
*/

#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <libmatevfs/mate-vfs-monitor.h>
#include <libmatevfs/mate-vfs-monitor-private.h>
#include <libmatevfs/mate-vfs-method.h>
#include <libmatevfs/mate-vfs-module-shared.h>
#include <glib.h>

typedef enum {
	CALLBACK_STATE_NOT_SENT,
	CALLBACK_STATE_SENDING,
	CALLBACK_STATE_SENT
} CallbackState;
	

struct MateVFSMonitorHandle {
	MateVFSURI *uri; /* the URI being monitored */
	MateVFSMethodHandle *method_handle;
	MateVFSMonitorType type;
	MateVFSMonitorCallback callback;
	gpointer user_data; /* FIXME - how does this get freed */

	gboolean cancelled;
	gboolean in_dispatch;
	
	GQueue *pending_callbacks; /* protected by handle_hash */
	guint pending_timeout; /* protected by handle_hash */
	time_t min_send_at;
};

struct MateVFSMonitorCallbackData {
	char *info_uri;
	MateVFSMonitorEventType event_type;
	CallbackState send_state;
	time_t send_at;
};

/* Number of seconds between consecutive events of the same type to the same file */
#define CONSECUTIVE_CALLBACK_DELAY 2

typedef struct MateVFSMonitorCallbackData MateVFSMonitorCallbackData;

/* This hash maps the module-supplied handle pointer to our own MonitrHandle */
static GHashTable *handle_hash = NULL;
G_LOCK_DEFINE_STATIC (handle_hash);

static gint actually_dispatch_callback (gpointer data);
static guint32 get_min_send_at (GQueue *queue);

static void 
init_hash_table (void)
{
	G_LOCK (handle_hash);

	if (handle_hash == NULL) {
		handle_hash = g_hash_table_new (g_direct_hash, g_direct_equal);
	}

	G_UNLOCK (handle_hash);
}

static void
free_callback_data (MateVFSMonitorCallbackData *callback_data)
{
	g_free (callback_data->info_uri);
	g_free (callback_data);
}

MateVFSResult
_mate_vfs_monitor_do_add (MateVFSMethod *method,
			  MateVFSMonitorHandle **handle,
			  MateVFSURI *uri,
                          MateVFSMonitorType monitor_type,
			  MateVFSMonitorCallback callback,
			  gpointer user_data)
{
	MateVFSResult result;
	MateVFSMonitorHandle *monitor_handle = 
		g_new0(MateVFSMonitorHandle, 1);

	init_hash_table ();
	mate_vfs_uri_ref (uri);
	monitor_handle->uri = uri;

	monitor_handle->type = monitor_type;
	monitor_handle->callback = callback;
	monitor_handle->user_data = user_data;
	monitor_handle->pending_callbacks = g_queue_new ();
	monitor_handle->min_send_at = 0;

	result = uri->method->monitor_add (uri->method, 
			&monitor_handle->method_handle, uri, monitor_type);

	if (result != MATE_VFS_OK) {
		mate_vfs_uri_unref (uri);
		g_free (monitor_handle);
		monitor_handle = NULL;
	} else {
		G_LOCK (handle_hash);
		g_hash_table_insert (handle_hash, 
				     monitor_handle->method_handle,
				     monitor_handle);
		G_UNLOCK (handle_hash);
	}

	*handle = monitor_handle;

	return result;
}

/* Called with handle_hash lock held */
static gboolean
no_live_callbacks (MateVFSMonitorHandle *monitor_handle)
{
	GList *l;
	MateVFSMonitorCallbackData *callback_data;
	
	l = monitor_handle->pending_callbacks->head;
	while (l != NULL) {
		callback_data = l->data;

		if (callback_data->send_state == CALLBACK_STATE_NOT_SENT ||
		    callback_data->send_state == CALLBACK_STATE_SENDING) {
			return FALSE;
		}
		
		l = l->next;
	}
	return TRUE;
}

/* Called with handle_hash lock held */
static void
destroy_monitor_handle (MateVFSMonitorHandle *handle)
{
	gboolean res;

	if (handle->pending_timeout) 
		g_source_remove (handle->pending_timeout);
	
	g_queue_foreach (handle->pending_callbacks, (GFunc) free_callback_data, NULL);
	g_queue_free (handle->pending_callbacks);
	handle->pending_callbacks = NULL;
	
	res = g_hash_table_remove (handle_hash, handle->method_handle);
	if (!res) {
		g_warning ("mate-vfs-monitor.c: A monitor handle was destroyed "
			   "before it was added to the method hash table. This "
			   "is a bug in the application and can cause crashes. "
			   "It is probably a race-condition.");
	}

	mate_vfs_uri_unref (handle->uri);
	g_free (handle);
}

MateVFSResult 
_mate_vfs_monitor_do_cancel (MateVFSMonitorHandle *handle)
{
	MateVFSResult result;

	init_hash_table ();

	if (!VFS_METHOD_HAS_FUNC(handle->uri->method, monitor_cancel)) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

	result = handle->uri->method->monitor_cancel (handle->uri->method,
						      handle->method_handle);

	if (result == MATE_VFS_OK) {
		G_LOCK (handle_hash);
		/* mark this monitor as cancelled */
		handle->cancelled = TRUE;

		/* we might be in the unlocked part of actually_dispatch_callback,
		 * in that case, don't free the handle now.
		 * Instead we do it in actually_dispatch_callback.
		 */
		if (!handle->in_dispatch) {
			destroy_monitor_handle (handle);
		}
		G_UNLOCK (handle_hash);
	}

	return result;
}

/* Called with handle_hash lock held */
static void
install_timeout (MateVFSMonitorHandle *monitor_handle, time_t now)
{
	gint delay;

	if (monitor_handle->pending_timeout) 
		g_source_remove (monitor_handle->pending_timeout);

	delay = monitor_handle->min_send_at - now;
	if (delay <= 0)
		monitor_handle->pending_timeout = g_idle_add (actually_dispatch_callback, monitor_handle);
	else
		monitor_handle->pending_timeout = g_timeout_add (delay * 1000, actually_dispatch_callback, monitor_handle);
}

static gint
actually_dispatch_callback (gpointer data)
{
	MateVFSMonitorHandle *monitor_handle = data;
	MateVFSMonitorCallbackData *callback_data;
	gchar *uri;
	GList *l, *next;
	GList *dispatch;
	time_t now;

	/* This function runs on the main loop, so it won't reenter,
	 * although other threads may add stuff to the pending queue
	 * while we don't have the lock
	 */

	time (&now);

	G_LOCK (handle_hash);

	/* Mark this so that do_cancel doesn't free the handle while
	 * we run without the lock during dispatch */
	monitor_handle->in_dispatch = TRUE;
	
	if (!monitor_handle->cancelled) {
		/* Find all callbacks that needs to be dispatched */
		dispatch = NULL;
		l = monitor_handle->pending_callbacks->head;
		while (l != NULL) {
			callback_data = l->data;
			
			g_assert (callback_data->send_state != CALLBACK_STATE_SENDING);

			if (callback_data->send_state == CALLBACK_STATE_NOT_SENT &&
			    callback_data->send_at <= now) {
				callback_data->send_state = CALLBACK_STATE_SENDING;
				dispatch = g_list_prepend (dispatch, callback_data);
			}

			l = l->next;
		}

		dispatch = g_list_reverse (dispatch);
		
		G_UNLOCK (handle_hash);
		
		l = dispatch;
		while (l != NULL) {
			callback_data = l->data;
			
			uri = mate_vfs_uri_to_string 
				(monitor_handle->uri, 
				 MATE_VFS_URI_HIDE_NONE);


			/* actually run app code */
			monitor_handle->callback (monitor_handle, uri,
						  callback_data->info_uri, 
						  callback_data->event_type,
						  monitor_handle->user_data);
			
			g_free (uri);
			callback_data->send_state = CALLBACK_STATE_SENT;

			l = l->next;
		}
			
		g_list_free (dispatch);
		
		G_LOCK (handle_hash);

		l = monitor_handle->pending_callbacks->head;
		while (l != NULL) {
			callback_data = l->data;
			next = l->next;
			
			g_assert (callback_data->send_state != CALLBACK_STATE_SENDING);

			/* If we've sent the event, and its not affecting coming events, free it */
			if (callback_data->send_state == CALLBACK_STATE_SENT &&
			    callback_data->send_at + CONSECUTIVE_CALLBACK_DELAY <= now) {
				/* free the callback_data */
				free_callback_data (callback_data);
				
				g_queue_delete_link (monitor_handle->pending_callbacks, l);
			}

			l = next;
		}

	}

	monitor_handle->in_dispatch = FALSE;
	
	/* if we were waiting for this callback to be dispatched
	 * to free this monitor, then do it now. */
	if (monitor_handle->cancelled) {
		destroy_monitor_handle (monitor_handle);
	} else if (no_live_callbacks (monitor_handle)) {
		monitor_handle->pending_timeout = 0;
		monitor_handle->min_send_at = 0;
	} else {
		/* pending callbacks left, install another timeout */
		monitor_handle->min_send_at = get_min_send_at (monitor_handle->pending_callbacks);
		install_timeout (monitor_handle, now);
	}

	G_UNLOCK (handle_hash);

	return FALSE;
}

/* Called with handle_hash lock held */
static void
send_uri_changes_now (MateVFSMonitorHandle *monitor_handle,
		      const char *uri,
		      gint32 now)
{
	GList *l;
	MateVFSMonitorCallbackData *callback_data;

	l = monitor_handle->pending_callbacks->head;
	while (l != NULL) {
		callback_data = l->data;
		if (callback_data->send_state != CALLBACK_STATE_SENT &&
		    strcmp (callback_data->info_uri, uri) == 0) {
			callback_data->send_at = now;
		}
		l = l->next;
	}
}

/* Called with handle_hash lock held */
static guint32
get_min_send_at (GQueue *queue)
{
	time_t min_send_at;
	MateVFSMonitorCallbackData *callback_data;
	GList *list;
	
	min_send_at = G_MAXINT;

	list = queue->head;
	while (list != NULL) {
		callback_data = list->data;

		if (callback_data->send_state == CALLBACK_STATE_NOT_SENT) {
			min_send_at = MIN (min_send_at, callback_data->send_at);
		}

		list = list->next;
	}

	return min_send_at;
}

/**
 * mate_vfs_monitor_callback:
 * @method_handle: Method-specific monitor handle obtained through mate_vfs_monitor_add().
 * @info_uri: URI that triggered the callback.
 * @event_type: The event obtained for @info_uri.
 *
 * mate_vfs_monitor_callback() is used by #MateVFSMethods to indicate that a particular
 * resource changed, and will issue the emission of the #MateVFSMonitorCallback registered
 * using mate_vfs_monitor_add().
 **/
void
mate_vfs_monitor_callback (MateVFSMethodHandle *method_handle,
                            MateVFSURI *info_uri,
                            MateVFSMonitorEventType event_type)
{
	MateVFSMonitorCallbackData *callback_data, *other_data, *last_data;
	MateVFSMonitorHandle *monitor_handle;
	char *uri;
	time_t now;
	GList *l;
	
	g_return_if_fail (info_uri != NULL);

	init_hash_table ();

	/* We need to loop here, because there is a race after we add the
	 * handle and when we add it to the hash table.
	 */
	do  {
		G_LOCK (handle_hash);
		monitor_handle = g_hash_table_lookup (handle_hash, method_handle);
		if (monitor_handle == NULL) {
			G_UNLOCK (handle_hash);
		}
	} while (monitor_handle == NULL);

	if (monitor_handle->cancelled) {
		G_UNLOCK (handle_hash);
		return;
	}
	
	time (&now);

	uri = mate_vfs_uri_to_string (info_uri, MATE_VFS_URI_HIDE_NONE);

	last_data = NULL;
	l = monitor_handle->pending_callbacks->tail;
	while (l != NULL) {
		other_data = l->data;
		if (strcmp (other_data->info_uri, uri) == 0) {
			last_data = l->data;
			break;
		}
		l = l->prev;
	}

	if (last_data == NULL ||
	    (last_data->event_type != event_type ||
	     last_data->send_state == CALLBACK_STATE_SENT)) {
		callback_data = g_new0 (MateVFSMonitorCallbackData, 1);
		callback_data->info_uri = g_strdup (uri);
		callback_data->event_type = event_type;
		callback_data->send_state = CALLBACK_STATE_NOT_SENT;
		if (last_data == NULL) {
			callback_data->send_at = now;
		} else {
			if (last_data->event_type != event_type) {
				/* New type, flush old events */
				send_uri_changes_now (monitor_handle, uri, now);
				callback_data->send_at = now;
			} else {
				callback_data->send_at = last_data->send_at + CONSECUTIVE_CALLBACK_DELAY;
			}
		}
		
		g_queue_push_tail (monitor_handle->pending_callbacks,
				   callback_data);
		if (monitor_handle->min_send_at == 0 ||
		    callback_data->send_at < monitor_handle->min_send_at) {
			monitor_handle->min_send_at = callback_data->send_at;
			install_timeout (monitor_handle, now);
		}
	}
	
	g_free (uri);
	
	G_UNLOCK (handle_hash);

}
