/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* inotify-helper.c - Mate VFS Monitor based on inotify.

   Copyright (C) 2005 John McCutchan

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
		 John McCutchan <john@johnmccutchan.com>
*/

#include "config.h"
#include <errno.h>
#include <time.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/inotify.h>
#include <libmatevfs/mate-vfs-module-shared.h>
#include <libmatevfs/mate-vfs-utils.h>
#include "inotify-helper.h"
#include "inotify-missing.h"
#include "inotify-path.h"
#include "inotify-diag.h"

static gboolean		ih_debug_enabled = FALSE;
#define IH_W if (ih_debug_enabled) g_warning 

static void ih_event_callback (ik_event_t *event, ih_sub_t *sub);
static void ih_not_missing_callback (ih_sub_t *sub);

/* We share this lock with inotify-kernel.c and inotify-missing.c
 *
 * inotify-kernel.c takes the lock when it reads events from
 * the kernel and when it processes those events
 *
 * inotify-missing.c takes the lock when it is scanning the missing
 * list.
 * 
 * We take the lock in all public functions 
 */
G_LOCK_DEFINE (inotify_lock);

static MateVFSMonitorEventType ih_mask_to_EventType (guint32 mask);

/**
 * Initializes the inotify backend.  This must be called before
 * any other functions in this module.
 *
 * @returns TRUE if initialization succeeded, FALSE otherwise
 */
gboolean
ih_startup (void)
{
	static gboolean initialized = FALSE;
	static gboolean result = FALSE;

	G_LOCK(inotify_lock);
	
	if (initialized == TRUE) {
		G_UNLOCK(inotify_lock);
		return result;
	}

	initialized = TRUE;

	result = ip_startup (ih_event_callback);
	if (!result) {
		g_warning( "Could not initialize inotify\n");
		G_UNLOCK(inotify_lock);
		return FALSE;
	}
	im_startup (ih_not_missing_callback);
	id_startup ();

	IH_W ("started mate-vfs inotify backend\n");

	G_UNLOCK(inotify_lock);
	return TRUE;
}

/**
 * Adds a subscription to be monitored.
 */
gboolean
ih_sub_add (ih_sub_t * sub)
{
	G_LOCK(inotify_lock);
	
	if (!ip_start_watching (sub))
	{
		im_add (sub);
	}

	G_UNLOCK(inotify_lock);
	return TRUE;
}

/**
 * Cancels a subscription which was being monitored.
 */
gboolean
ih_sub_cancel (ih_sub_t * sub)
{
	G_LOCK(inotify_lock);

	if (!sub->cancelled)
	{
		IH_W("cancelling %s\n", sub->pathname);
		sub->cancelled = TRUE;
		im_rm (sub);
		ip_stop_watching (sub);
	}

	G_UNLOCK(inotify_lock);
	return TRUE;
}


static void ih_event_callback (ik_event_t *event, ih_sub_t *sub)
{
	gchar *fullpath, *info_uri_str;
	MateVFSURI *info_uri;
	MateVFSMonitorEventType gevent;

	gevent = ih_mask_to_EventType (event->mask);
	if (event->name)
	{
		fullpath = g_strdup_printf ("%s/%s", sub->dirname, event->name);
	} else {
		fullpath = g_strdup_printf ("%s/", sub->dirname);
	}

	info_uri_str = mate_vfs_get_uri_from_local_path (fullpath);
	info_uri = mate_vfs_uri_new (info_uri_str);
	g_free (info_uri_str);
	mate_vfs_monitor_callback ((MateVFSMethodHandle *)sub, info_uri, gevent);
	mate_vfs_uri_unref (info_uri);
	g_free(fullpath);
}

static void ih_not_missing_callback (ih_sub_t *sub)
{
	gchar *fullpath, *info_uri_str;
	MateVFSURI *info_uri;
	MateVFSMonitorEventType gevent;
	guint32 mask;

	if (sub->filename)
	{
		fullpath = g_strdup_printf ("%s/%s", sub->dirname, sub->filename);
		if (!g_file_test (fullpath, G_FILE_TEST_EXISTS)) {
			g_free (fullpath);
			return;
		}
		mask = IN_CREATE;
	} else {
		fullpath = g_strdup_printf ("%s/", sub->dirname);
		mask = IN_CREATE|IN_ISDIR;
	}

	gevent = ih_mask_to_EventType (mask);
	info_uri_str = mate_vfs_get_uri_from_local_path (fullpath);
	info_uri = mate_vfs_uri_new (info_uri_str);
	g_free (info_uri_str);
	mate_vfs_monitor_callback ((MateVFSMethodHandle *)sub, info_uri, gevent);
	mate_vfs_uri_unref (info_uri);
	g_free(fullpath);
}

/* Transforms a inotify event to a mate-vfs event. */
static MateVFSMonitorEventType
ih_mask_to_EventType (guint32 mask)
{
	mask &= ~IN_ISDIR;
	switch (mask)
	{
	case IN_MODIFY:
		return MATE_VFS_MONITOR_EVENT_CHANGED;
	break;
	case IN_ATTRIB:
		return MATE_VFS_MONITOR_EVENT_METADATA_CHANGED;
	break;
	case IN_MOVE_SELF:
	case IN_MOVED_FROM:
	case IN_DELETE:
	case IN_DELETE_SELF:
	case IN_UNMOUNT:
		return MATE_VFS_MONITOR_EVENT_DELETED;
	break;
	case IN_CREATE:
	case IN_MOVED_TO:
		return MATE_VFS_MONITOR_EVENT_CREATED;
	break;
	case IN_Q_OVERFLOW:
	case IN_OPEN:
	case IN_CLOSE_WRITE:
	case IN_CLOSE_NOWRITE:
	case IN_ACCESS:
	case IN_IGNORED:
	default:
		return -1;
	break;
	}
}
