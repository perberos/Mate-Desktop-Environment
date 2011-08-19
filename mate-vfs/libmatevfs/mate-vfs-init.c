/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-init.c - Initialization for the MATE Virtual File System.

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

   Author: Ettore Perazzoli <ettore@gnu.org>
*/

#include <config.h>
#include "mate-vfs-init.h"

#include "mate-vfs-mime.h"

#include "mate-vfs-configuration.h"
#include "mate-vfs-method.h"
#include "mate-vfs-utils.h"
#include "mate-vfs-private-utils.h"

#include "mate-vfs-async-job-map.h"
#include "mate-vfs-job-queue.h"
#include "mate-vfs-volume-monitor-private.h"
#include "mate-vfs-module-callback-private.h"

#include <errno.h>
#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>
#include <glib.h>

#ifdef USE_DAEMON
#ifndef DBUS_API_SUBJECT_TO_CHANGE
#define DBUS_API_SUBJECT_TO_CHANGE 1
#endif
#include <dbus/dbus-glib.h>
#endif

#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>

static gboolean vfs_already_initialized = FALSE;
G_LOCK_DEFINE_STATIC (vfs_already_initialized);

static GPrivate * private_is_primary_thread;

static gboolean
ensure_dot_mate_exists (void)
{
	gboolean retval = TRUE;
	gboolean create_dirs;
	gchar *dirname;

	/* If the user does not have a writable HOME directory, then
	   avoid creating the directory. */
	create_dirs = (g_access (g_get_home_dir(), W_OK) == 0);

        if (create_dirs != TRUE)
		return TRUE;

	dirname = g_build_filename (g_get_home_dir (), ".mate2", NULL);

	if (!g_file_test (dirname, G_FILE_TEST_EXISTS)) {
		if (g_mkdir (dirname, S_IRWXU) != 0) {
			g_warning ("Unable to create ~/.mate2 directory: %s",
				   g_strerror (errno));
			retval = FALSE;
		}
	} else if (!g_file_test (dirname, G_FILE_TEST_IS_DIR)) {
		g_warning ("Error: ~/.mate2 must be a directory.");
		retval = FALSE;
	}

	g_free (dirname);
	return retval;
}

static void
mate_vfs_thread_init (void)
{
	private_is_primary_thread = g_private_new (NULL);
	g_private_set (private_is_primary_thread, GUINT_TO_POINTER (1));
	
	_mate_vfs_module_callback_private_init ();
	
	_mate_vfs_async_job_map_init ();
	_mate_vfs_job_queue_init ();
}

/**
 * mate_vfs_init:
 *
 * If mate-vfs is not already initialized, initialize it. This must be
 * called prior to performing any other mate-vfs operations, and may
 * be called multiple times without error.
 * 
 * Return value: %TRUE if mate-vfs is successfully initialized (or was
 * already initialized).
 */
gboolean 
mate_vfs_init (void)
{
	gboolean retval;
	/*
	char *bogus_argv[2] = { "dummy", NULL };
	*/
	
	if (!ensure_dot_mate_exists ()) {
		return FALSE;
	}

 	if (!g_thread_supported ())
 		g_thread_init (NULL);

	G_LOCK (vfs_already_initialized);

	if (!vfs_already_initialized) {
#ifdef ENABLE_NLS
		bindtextdomain (GETTEXT_PACKAGE, MATE_VFS_LOCALEDIR);
		bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif   
		mate_vfs_thread_init ();
#ifdef USE_DAEMON
		dbus_g_thread_init ();
#endif
 		/* Make sure the type system is inited. */
		g_type_init ();

		retval = mate_vfs_method_init ();

		if (retval) {
			retval = _mate_vfs_configuration_init ();
		}
		
#ifdef SIGPIPE
		if (retval) {
			signal (SIGPIPE, SIG_IGN);
		}
#endif		
	} else {
		retval = TRUE;	/* Who cares after all.  */
	}

	vfs_already_initialized = TRUE;
	G_UNLOCK (vfs_already_initialized);

	return retval;
}

/**
 * mate_vfs_initialized:
 *
 * Detects if mate-vfs has already been initialized (mate-vfs must be
 * initialized prior to using any methods or operations).
 * 
 * Return value: %TRUE if mate-vfs has already been initialized.
 */
gboolean
mate_vfs_initialized (void)
{
	gboolean out;

	G_LOCK (vfs_already_initialized);
	out = vfs_already_initialized;
	G_UNLOCK (vfs_already_initialized);
	return out;
}

/**
 * mate_vfs_shutdown:
 *
 * Cease all active mate-vfs operations and unload the MIME
 * database from memory.
 * 
 */
void
mate_vfs_shutdown (void)
{
	mate_vfs_mime_shutdown ();
#ifndef G_OS_WIN32
	_mate_vfs_volume_monitor_shutdown ();
#endif
	_mate_vfs_method_shutdown ();
}

void
mate_vfs_loadinit (gpointer app, gpointer modinfo)
{
}

void
mate_vfs_preinit (gpointer app, gpointer modinfo)
{
}

void
mate_vfs_postinit (gpointer app, gpointer modinfo)
{
	G_LOCK (vfs_already_initialized);

	mate_vfs_thread_init ();

	mate_vfs_method_init ();
	_mate_vfs_configuration_init ();

#ifdef SIGPIPE
	signal (SIGPIPE, SIG_IGN);
#endif

	vfs_already_initialized = TRUE;
	G_UNLOCK (vfs_already_initialized);
}

/**
 * mate_vfs_is_primary_thread:
 *
 * Check if the current thread is the thread with the main glib event loop.
 *
 * Return value: %TRUE if the current thread is the thread with the 
 * main glib event loop.
 */
gboolean
mate_vfs_is_primary_thread (void)
{
	if (g_thread_supported()) {
		return GPOINTER_TO_UINT(g_private_get (private_is_primary_thread)) == 1;
	} else {
		return TRUE;
	}
}
