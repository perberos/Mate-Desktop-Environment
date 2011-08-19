/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* inotify-helper.c - Mate VFS Monitor based on inotify.

   Copyright (C) 2006 John McCutchan

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
#include <string.h>
#include <glib.h>
#include <libmatevfs/mate-vfs-module-shared.h>
#include <libmatevfs/mate-vfs-utils.h>

#include "inotify-sub.h"

static gboolean     is_debug_enabled = FALSE;
#define IS_W if (is_debug_enabled) g_warning

static void ih_sub_setup (ih_sub_t *sub);

ih_sub_t *
ih_sub_new (MateVFSURI *uri, MateVFSMonitorType mon_type)
{
	ih_sub_t *sub = NULL;

	sub = g_new0 (ih_sub_t, 1);
	sub->type = mon_type;
	sub->uri = uri;
	mate_vfs_uri_ref (uri);
	sub->pathname = mate_vfs_unescape_string (mate_vfs_uri_get_path (uri), "/");
	/* mate_vfs_unescape_string returns NULL when
	 * the uri path passed to it is invalid.
	 */
	if (!sub->pathname)
	{
		IS_W("new subscription for %s failed because of invalid characters.\n", mate_vfs_uri_get_path (uri));
		g_free (sub);
		mate_vfs_uri_unref (uri);
		return NULL;
	}
/* TODO: WAITING for flag to be implemented
	if (mon_type & MATE_VFS_DONT_FOLLOW_SYMLINK)
	{
		sub->extra_flags |= IN_DONT_FOLLOW;
	}
*/

	IS_W("new subscription for %s being setup\n", sub->pathname);

	ih_sub_setup (sub);
	return sub;
}

void
ih_sub_free (ih_sub_t *sub)
{
	if (sub->filename)
		g_free (sub->filename);
	if (sub->dirname)
	    g_free (sub->dirname);
	g_free (sub->pathname);
	mate_vfs_uri_unref (sub->uri);
	g_free (sub);
}

static
gchar *ih_sub_get_uri_dirname (MateVFSURI *uri)
{
	gchar *t, *out;
	t = mate_vfs_uri_extract_dirname (uri);
	out = mate_vfs_unescape_string (t, "/");
	g_free (t);

	return out;
}

static
gchar *ih_sub_get_uri_filename (MateVFSURI *uri)
{
	gchar *t, *out;
	t = mate_vfs_uri_extract_short_name (uri);
	out = mate_vfs_unescape_string (t, "/");
	g_free (t);

	return out;
}

static 
void ih_sub_fix_dirname (ih_sub_t *sub)
{
	size_t len = 0;
	
	g_assert (sub->dirname);

	len = strlen (sub->dirname);

	/* We need to strip a trailing slash
	 * to get the correct behaviour
	 * out of the kernel
	 */
	if (sub->dirname[len] == '/')
		sub->dirname[len] = '\0';
}

/*
 * XXX: Currently we just follow the mate vfs monitor type flags when
 * deciding how to treat the path. In the future we could try
 * and determine whether the path points to a directory or a file but
 * that is racey.
 */
static void
ih_sub_setup (ih_sub_t *sub)
{
	if (sub->type & MATE_VFS_MONITOR_DIRECTORY)
	{
		sub->dirname = g_strdup (sub->pathname);
		sub->filename = NULL;
	} else {
		sub->dirname = ih_sub_get_uri_dirname (sub->uri);
		sub->filename = ih_sub_get_uri_filename (sub->uri);
	}

	ih_sub_fix_dirname (sub);

	IS_W("sub->dirname = %s\n", sub->dirname);
	if (sub->filename)
	{
		IS_W("sub->filename = %s\n", sub->filename);
	}
}
