/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* test-queue.c - Test program for the MATE Virtual File System.

   Copyright (C) 2001 Free Software Foundation

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

   Author: László Péter <laca@ireland.sun.com>
*/

/* This is a simple test program for the job queue.
   It tests some basic features but to see what's really going on on the
   inside you should change 
      #undef QUEUE_DEBUG
   to 
      #define QUEUE_DEBUG 1
   in libmatevfs/mate-vfs-job-queue.c and rebuild mate-vfs.
*/

#include <config.h>

#include <glib.h>
#include <libmatevfs/mate-vfs-async-ops.h>
#include <libmatevfs/mate-vfs-job-limit.h>
#include <libmatevfs/mate-vfs-init.h>
#include <libmatevfs/mate-vfs-job.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define TEST_LIMIT 3

static MateVFSAsyncHandle *test_handle;
static gpointer test_callback_data;
static int jobs_started;
static int jobs_finished;
static int finished_7, finished_0;
static int test_no;
static gboolean verbose = FALSE;

static gboolean at_least_one_test_failed = FALSE;

static int prio_db[100];

static void
my_yield (int count)
{
	for (; count > 0; count--) {
		if (!g_main_context_iteration (NULL, FALSE)) {
			break;
		}
		g_usleep (10);
	}
}

static gboolean
wait_until_vfs_jobs_gone (void)
{
	int i;

	if (mate_vfs_job_get_count () == 0) {
		return TRUE;
	}

	for (i = 0; i < 2000; i++) {
		g_usleep (1000);
		g_main_context_iteration (NULL, FALSE);
		if (mate_vfs_job_get_count () == 0) {
			return TRUE;
		}
	}
	return FALSE;
}

static void
get_file_info_callback (MateVFSAsyncHandle *handle,
			GList *results,
			gpointer callback_data)
{
	int this_test_no = *(int *)callback_data;
	jobs_finished ++;
	if (prio_db[this_test_no] == 7) {
		finished_7 = jobs_finished;
	}
	if (prio_db[this_test_no] == 0) {
		finished_0 = jobs_finished;
	}
	if (verbose) {
		printf ("finished test #%d: get_file_info, priority = %d\n", this_test_no, prio_db[this_test_no]);
	}
	g_free (callback_data);
	jobs_started --;
}

static gboolean
test_get_file_info (const char *uri, int priority)
{
	GList *uri_list;

	/* Start a get_file_info call. */
	test_callback_data = g_malloc (sizeof (int));
	*(int *)test_callback_data = ++test_no;
	prio_db[test_no] = priority;

	uri_list = g_list_prepend (NULL, mate_vfs_uri_new (uri));

	jobs_started ++;

	if (verbose) {
		printf ("starting test #%d: get_file_info, priority = %d\n", test_no, priority);
	}

	test_handle = NULL;
	mate_vfs_async_get_file_info (&test_handle,
				       uri_list,
				       MATE_VFS_FILE_INFO_DEFAULT,
				       priority,
				       get_file_info_callback,
				       test_callback_data);
	if (test_handle == NULL) {
		jobs_started --;
		return FALSE;
	}

	g_list_free (uri_list);
	my_yield (20);

	return TRUE;
}

static void
test_find_directory_callback (MateVFSAsyncHandle *handle,
			      GList *results,
			      gpointer callback_data)
{
	int this_test_no = *(int *)callback_data;
	jobs_finished++;
	if (verbose) {
		printf ("finished test #%d: find_directory, priority = %d\n", this_test_no, prio_db[this_test_no]);
	}
	fflush (stdout);
	g_free (callback_data);
	jobs_started --;
}

static void
test_find_directory (const char *uri, int priority)
{
	MateVFSAsyncHandle *handle;
	GList *vfs_uri_as_list;

	test_callback_data = g_malloc (sizeof (int));
	*(int *)test_callback_data = ++test_no;
	prio_db[test_no] = priority;

	vfs_uri_as_list = g_list_append (NULL, mate_vfs_uri_new (uri));
	jobs_started++;

	if (verbose) {
		printf ("starting test #%d: find_directory, priority = %d\n", test_no, priority);
	}
	
	mate_vfs_async_find_directory (&handle, vfs_uri_as_list,
		MATE_VFS_DIRECTORY_KIND_TRASH, FALSE, TRUE, 0777, priority,
		test_find_directory_callback, test_callback_data);
	my_yield (20);
}


static int usage (void)
{
	printf ("Usage: test-queue [-h|--help] [-v|--verbose]\n");
	printf ("\t-h, --help      display usage information\n");
	printf ("\t-v, --verbose   verbose mode\n\n");
	exit (1);
}

int
main (int argc, char **argv)
{
	int limit;
	int finished_before_7;
	int finished_before_0;
	int i;

	for (i = 1; i < argc; i++) {
		if (!strcmp (argv[i], "-h") ||
		    !strcmp (argv[i], "--help")) {
			usage ();
			continue;
		}
		if (!strcmp (argv[i], "-v") ||
		    !strcmp (argv[i], "--verbose")) {
			verbose = TRUE;
			continue;
		}
		printf ("Unrecognized option: %s\n\n", argv[i]);
		usage ();
	}			

	mate_vfs_init ();

	limit = mate_vfs_async_get_job_limit ();
	if (verbose) {
		printf ("Current job limit: %d\n", limit);
		printf ("Trying to set the limit to 1; should result in a warning\n");
	} else {
		printf ("You should see a warning and 2 critical errors below.\n");
	}

	mate_vfs_async_set_job_limit (1);
	if (limit != mate_vfs_async_get_job_limit ()) {
		at_least_one_test_failed = TRUE;
		g_warning ("Current job limit changed to %d\n",
			   mate_vfs_async_get_job_limit ());
	}

	printf ("Trying to start a job with priority = -11; should result in a critical error\n");
	if (test_get_file_info ("file:///dev/null", -11)) {
		at_least_one_test_failed = TRUE;
	}

	printf ("Trying to start a job with priority = 11; should result in a critical error\n");
	if (test_get_file_info ("file:///dev/null", 11)) {
		at_least_one_test_failed = TRUE;
	}

	if (!verbose) {
		printf ("No warning or error messages are expected beyond this point\n");
	}

	if (verbose) {
		printf ("Setting job limit to %d\n", TEST_LIMIT);
	}

	mate_vfs_async_set_job_limit (TEST_LIMIT);
	limit = mate_vfs_async_get_job_limit ();
	if (limit != TEST_LIMIT) {
		at_least_one_test_failed = TRUE;
		g_warning ("Limit is %d, not %d as expected", limit, TEST_LIMIT);
	}

	for (i = 0; i < 10; i++) {
		test_find_directory ("test:///usr", 8);
	}

	finished_before_7 = jobs_finished;
	test_find_directory ("test:///usr", 7);
	finished_before_0 = jobs_finished;
	test_get_file_info ("test:///etc/passwd", 0);

	wait_until_vfs_jobs_gone ();

	/* Do some random tests with different priorities */
	test_find_directory ("file:///", 10);
	test_find_directory ("file:///usr", 6);
	test_find_directory ("file:///usr/local", -1);
	test_find_directory ("file:///home", 2);
	test_find_directory ("file:///etc", 5);
	test_find_directory ("file:///tmp", 5);
	test_find_directory ("file:///opt", 9);
	test_find_directory ("file:///var", 10);
	test_find_directory ("file:///", 10);
	test_find_directory ("file:///home", 10);
	test_get_file_info ("file:///dev/null", -1);
	test_get_file_info ("file:///etc/passwd", -5);
	test_find_directory ("file:///", -8);
	test_find_directory ("file:///tmp", -10);
	test_find_directory ("file:///etc", -5);
	test_find_directory ("file:///usr", -1);

	wait_until_vfs_jobs_gone ();
		
	if (jobs_started != 0) {
		printf ("%d jobs appear to be still running.\n", jobs_started - 2);
		at_least_one_test_failed = TRUE;
	}

	/* at most TEST_LIMIT low priority jobs + the 0 priority job may have
	   finished before this one */
	if ((finished_7 > 0) && (finished_7 - finished_before_7 > TEST_LIMIT + 1)) {
		printf ("Scheduling error: %d lower priority jobs finished before "
			"the 7 priority job when the job limit is %d\n",
			finished_7 - finished_before_7,
			TEST_LIMIT);
		at_least_one_test_failed = TRUE;
	}

	/* at most TEST_LIMIT low priority jobs may have finished before this one */
	if ((finished_0 > 0) && (finished_0 - finished_before_0 > TEST_LIMIT + 1)) {
		printf ("Scheduling error: %d lower priority jobs finished before "
			"the 0 priority job when the job limit is %d\n",
			finished_0 - finished_before_0,
			TEST_LIMIT);
		at_least_one_test_failed = TRUE;
	}

	if (verbose) {
		printf ("Shutting down\n");
	}

	mate_vfs_shutdown ();

	/* Report to "make check" on whether it all worked or not. */
	return at_least_one_test_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
