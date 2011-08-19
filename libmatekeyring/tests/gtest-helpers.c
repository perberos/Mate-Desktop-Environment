/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* test-helpers.c: Common functions called from gtest unit tests

   Copyright (C) 2008 Stefan Walter

   The Mate Keyring Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Keyring Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Stef Walter <stef@memberwebs.com>
*/

/* This file is included into the main .c file for each gtest unit-test program */

#include "config.h"

#include <glib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include "gtest-helpers.h"

#include "egg/egg-secure-memory.h"

static GStaticMutex memory_mutex = G_STATIC_MUTEX_INIT;
static const gchar *test_path = NULL;

void
egg_memory_lock (void)
{
	g_static_mutex_lock (&memory_mutex);
}

void
egg_memory_unlock (void)
{
	g_static_mutex_unlock (&memory_mutex);
}

void*
egg_memory_fallback (void *p, size_t sz)
{
	return g_realloc (p, sz);
}

static GMainLoop *mainloop = NULL;

static gboolean
quit_loop (gpointer unused)
{
	g_main_loop_quit (mainloop);
	return TRUE;
}

void
test_mainloop_quit (void)
{
	g_main_loop_quit (mainloop);
}

void
test_mainloop_run (int timeout)
{
	guint id = 0;

	if (timeout)
		id = g_timeout_add (timeout, quit_loop, NULL);
	g_main_loop_run (mainloop);
	if (timeout)
		g_source_remove (id);
}

GMainLoop*
test_mainloop_get (void)
{
	if (!mainloop)
		mainloop = g_main_loop_new (NULL, FALSE);
	return mainloop;
}

gchar*
test_build_filename (const gchar *basename)
{
	return g_build_filename (test_path, basename, NULL);
}

const gchar*
test_dir_testdata (void)
{
	const gchar *dir;
	gchar *cur, *env;

	dir = g_getenv ("TEST_DATA");
	if (dir == NULL)
		dir = "./test-data";
	if (!g_path_is_absolute (dir)) {
		cur = g_get_current_dir ();
		if (strncmp (dir, "./", 2) == 0)
			dir += 2;
		env = g_build_filename (cur, dir, NULL);
		g_free (cur);
		g_setenv ("TEST_DATA", env, TRUE);
		g_free (env);
		dir = g_getenv ("TEST_DATA");
	}

	return dir;
}

guchar*
test_read_testdata (const gchar *basename, gsize *n_result)
{
	GError *error = NULL;
	gchar *result;
	gchar *file;

	file = g_build_filename (test_dir_testdata (), basename, NULL);
	if (!g_file_get_contents (file, &result, n_result, &error)) {
		g_warning ("could not read test data file: %s: %s", file,
		           error && error->message ? error->message : "");
		g_assert_not_reached ();
	}

	g_free (file);
	return (guchar*)result;
}

static void
chdir_base_dir (char* argv0)
{
	gchar *dir, *base;

	dir = g_path_get_dirname (argv0);
	if (chdir (dir) < 0)
		g_warning ("couldn't change directory to: %s: %s",
		           dir, g_strerror (errno));

	base = g_path_get_basename (dir);
	if (strcmp (base, ".libs") == 0) {
		if (chdir ("..") < 0)
			g_warning ("couldn't change directory to ..: %s",
			           g_strerror (errno));
	}

	g_free (dir);
}

#ifdef TEST_WITH_DAEMON

static pid_t daemon_pid = 0;

static gboolean
daemon_start ()
{
	GError *err = NULL;
	gchar *args[5];
	const gchar *path, *service, *address;

	/* Need to have DBUS running */
	address = g_getenv ("DBUS_SESSION_BUS_ADDRESS");
	if (!address || !address[0]) {
		g_printerr ("\nNo DBUS session available, skipping tests!\n\n");
		return FALSE;
	}

	path = g_getenv ("MATE_KEYRING_TEST_PATH");
	if (path && !path[0])
		path = NULL;

	service = g_getenv ("MATE_KEYRING_TEST_SERVICE");
	if (service && !service[0])
		service = NULL;

	if (!path && !service) {
		g_mkdir_with_parents ("/tmp/keyring-test/data", 0700);
		g_setenv ("MATE_KEYRING_TEST_PATH", "/tmp/keyring-test/data", TRUE);
		g_setenv ("MATE_KEYRING_TEST_SERVICE", "org.mate.keyring.Test", TRUE);

		g_printerr ("Starting mate-keyring-daemon...\n");

		args[0] = MATE_KEYRING_DAEMON_PATH;
		args[1] = "--foreground";
		args[2] = "--control-directory";
		args[3] = "/tmp/keyring-test";
		args[4] = NULL;

		if (!g_spawn_async (NULL, args, NULL, G_SPAWN_LEAVE_DESCRIPTORS_OPEN | G_SPAWN_DO_NOT_REAP_CHILD,
							NULL, NULL, &daemon_pid, &err)) {
			g_error ("couldn't start mate-keyring-daemon for testing: %s",
					 err && err->message ? err->message : "");
			g_assert_not_reached ();
		}

		/* Let it startup properly */
		sleep (2);
	}

	return TRUE;
}

static void
daemon_stop (void)
{
	if (daemon_pid)
		kill (daemon_pid, SIGTERM);
	daemon_pid = 0;
}

#endif /* TEST_WITH_DAEMON */

int
main (int argc, char* argv[])
{
	GLogLevelFlags fatal_mask;
	int ret = 0;

	chdir_base_dir (argv[0]);
	g_test_init (&argc, &argv, NULL);
	mainloop = g_main_loop_new (NULL, FALSE);

	fatal_mask = g_log_set_always_fatal (G_LOG_FATAL_MASK);
	fatal_mask |= G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL;
	g_log_set_always_fatal (fatal_mask);

	initialize_tests ();

#ifdef TEST_WITH_DAEMON
	if (daemon_start ()) {
#endif

		start_tests ();
		ret = g_test_run ();
		stop_tests();

#ifdef TEST_WITH_DAEMON
		daemon_stop();
	}
#endif

	return ret;
}
