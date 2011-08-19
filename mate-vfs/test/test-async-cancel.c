/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* test-async-cancel.c - Test program for the MATE Virtual File System.

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

   Author: Darin Adler <darin@eazel.com>
*/

#include <config.h>

#include <fcntl.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <libmatevfs/mate-vfs-async-ops.h>
#include <libmatevfs/mate-vfs-init.h>
#include <libmatevfs/mate-vfs-job.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define TEST_ASSERT(expression, message) \
	G_STMT_START { if (!(expression)) test_failed message; } G_STMT_END

#ifndef G_OS_WIN32
#define DEV_NULL "/dev/null"
#define DEV_NULL_URI "file://" DEV_NULL
#else
#define DEV_NULL "NUL:"
#define DEV_NULL_URI "file:///" DEV_NULL
#endif

static MateVFSAsyncHandle *test_handle;
static gpointer test_callback_data;
static gboolean test_done;
static char *temp_file_uri;
static char *tmp_dir_uri;

#define MAX_THREAD_WAIT 500
#define MAX_FD_CHECK 128

static void
stop_after_log (const char *domain, GLogLevelFlags level, 
	const char *message, gpointer data)
{
	void (* saved_handler)(int);
	
	g_log_default_handler(domain, level, message, data);

	saved_handler = signal (SIGINT, SIG_IGN);
	raise(SIGINT);
	signal(SIGINT, saved_handler);
}

static void
make_asserts_break (const char *domain)
{
	g_log_set_handler (domain, 
		(GLogLevelFlags) (G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING),
		stop_after_log, NULL);
}

static int
get_free_file_descriptor_count (void)
{
	int count;
	GList *list, *p;
	int fd = -1;

	list = NULL;
	for (count = 0; fd < MAX_FD_CHECK; count++) {
		fd = open (DEV_NULL, O_RDONLY);
		if (fd == -1) {
			break;
		}
		list = g_list_prepend (list, GINT_TO_POINTER (fd));
	}

	for (p = list; p != NULL; p = p->next) {
		close (GPOINTER_TO_INT (p->data));
	}
	g_list_free (list);

	return count;
}

static int free_at_start;

static int
get_used_file_descriptor_count (void)
{
	return free_at_start - get_free_file_descriptor_count ();
}

static gboolean
wait_for_boolean (gboolean *wait_for_it)
{
	int i;

	if (*wait_for_it) {
		return TRUE;
	}

	for (i = 0; i < MAX_THREAD_WAIT; i++) {
		g_thread_yield ();
		g_main_context_iteration (NULL, FALSE);
		if (*wait_for_it) {
			return TRUE;
		}
	}
	return FALSE;
}

static gboolean
wait_until_vfs_jobs_gone (void)
{
	int i;

	if (mate_vfs_job_get_count () == 0) {
		return TRUE;
	}

	for (i = 0; i < MAX_THREAD_WAIT; i++) {
		g_thread_yield ();
		g_main_context_iteration (NULL, FALSE);
		if (mate_vfs_job_get_count () == 0) {
			return TRUE;
		}
	}
	
	return FALSE;
}

static gboolean
wait_until_vfs_jobs_gone_no_main (void)
{
	int i;

	if (mate_vfs_job_get_count () == 0) {
		return TRUE;
	}

	for (i = 0; i < MAX_THREAD_WAIT; i++) {
		g_thread_yield ();
		if (mate_vfs_job_get_count () == 0) {
			return TRUE;
		}
	}
	return FALSE;
}

static gboolean
wait_until_file_descriptors_gone (void)
{
	int i;

	if (get_used_file_descriptor_count () == 0) {
		return TRUE;
	}

	for (i = 0; i < MAX_THREAD_WAIT; i++) {
		g_thread_yield ();
		g_main_context_iteration (NULL, FALSE);
		if (get_used_file_descriptor_count () == 0) {
			return TRUE;
		}
	}
	return FALSE;
}

static gboolean at_least_one_test_failed = FALSE;

static void
test_failed (const char *format, ...)
{
	va_list arguments;
	char *message;

	va_start (arguments, format);
	message = g_strdup_vprintf (format, arguments);
	va_end (arguments);

	g_message ("test failed: %s", message);
	at_least_one_test_failed = TRUE;
}

static void
get_file_info_callback (MateVFSAsyncHandle *handle,
			GList *results,
			gpointer callback_data)
{
	TEST_ASSERT (handle == test_handle, ("get_file_info, bad handle"));
	TEST_ASSERT (g_list_length (results) == 1, ("get_file_info, bad list length"));
	TEST_ASSERT (callback_data == test_callback_data, ("get_file_info, bad handle"));

	test_handle = NULL;
	g_free (callback_data);

	test_done = TRUE;
}

static void
first_get_file_info (void)
{
	GList *uri_list;

	/* Start a get_file_info call. */
	test_done = FALSE;
	test_callback_data = g_malloc (1);
	uri_list = g_list_prepend (NULL, mate_vfs_uri_new (DEV_NULL_URI));
	mate_vfs_async_get_file_info (&test_handle,
				       uri_list,
				       MATE_VFS_FILE_INFO_DEFAULT,
				       0,
				       get_file_info_callback,
				       test_callback_data);
	g_list_free (uri_list);

	/* Wait until it is done. */
	TEST_ASSERT (wait_for_boolean (&test_done), ("first_get_file_info: callback was not called"));
	TEST_ASSERT (wait_until_vfs_jobs_gone (), ("first_get_file_info: job never went away"));

	/* For some reason, this consumes file descriptors.
	 * I don't know why.
	 */
}

static void
test_get_file_info (void)
{
	GList *uri_list;

	/* Start a get_file_info call. */
	test_done = FALSE;
	test_callback_data = g_malloc (1);
	uri_list = g_list_prepend (NULL, mate_vfs_uri_new (DEV_NULL_URI));
	mate_vfs_async_get_file_info (&test_handle,
				       uri_list,
				       MATE_VFS_FILE_INFO_DEFAULT,
				       0,
				       get_file_info_callback,
				       test_callback_data);
	mate_vfs_uri_list_free (uri_list);

	/* Wait until it is done. */
	TEST_ASSERT (wait_for_boolean (&test_done), ("get_file_info 1: callback was not called"));
	TEST_ASSERT (wait_until_vfs_jobs_gone (), ("get_file_info 1: job never went away"));
	TEST_ASSERT (get_used_file_descriptor_count () == 0,
		     ("get_file_info 1: %d file descriptors leaked", get_used_file_descriptor_count ()));
	free_at_start = get_free_file_descriptor_count ();

	/* Cancel one right after starting it. */
	test_done = FALSE;
	test_callback_data = g_malloc (1);
	uri_list = g_list_prepend (NULL, mate_vfs_uri_new (DEV_NULL_URI));
	mate_vfs_async_get_file_info (&test_handle,
				       uri_list,
				       MATE_VFS_FILE_INFO_DEFAULT,
				       0,
				       get_file_info_callback,
				       test_callback_data);
	mate_vfs_uri_list_free (uri_list);
	mate_vfs_async_cancel (test_handle);
	g_free (test_callback_data);

	/* Wait until it is done. */
	TEST_ASSERT (wait_until_vfs_jobs_gone (), ("get_file_info 2: job never went away"));
	TEST_ASSERT (get_used_file_descriptor_count () == 0,
		     ("get_file_info 2: %d file descriptors leaked", get_used_file_descriptor_count ()));
	TEST_ASSERT (test_done == FALSE, ("get_file_info 2: callback was called"));
	free_at_start = get_free_file_descriptor_count ();
}

static gboolean file_open_flag;

static void
file_open_callback (MateVFSAsyncHandle *handle,
		    MateVFSResult result,
		    gpointer callback_data)
{
	TEST_ASSERT (handle == test_handle, ("open callback, bad handle"));
	TEST_ASSERT (result == MATE_VFS_OK, ("open callback, bad result"));
	TEST_ASSERT (callback_data == test_callback_data, ("open callback, bad callback data"));

	file_open_flag = TRUE;
}

static gboolean file_closed_flag;

static void
file_close_callback (MateVFSAsyncHandle *handle,
		     MateVFSResult result,
		     gpointer callback_data)
{
	TEST_ASSERT (handle == test_handle, ("close callback, bad handle"));
	TEST_ASSERT (result == MATE_VFS_OK, ("close callback, bad result"));
	TEST_ASSERT (callback_data == test_callback_data, ("close callback, bad callback data"));

	file_closed_flag = TRUE;
}

static gboolean file_read_flag;
static char read_buffer[1];

static void
file_read_callback (MateVFSAsyncHandle *handle,
		    MateVFSResult result,
		    gpointer buffer,
		    MateVFSFileSize bytes_requested,
		    MateVFSFileSize bytes_read,
		    gpointer callback_data)
{
	TEST_ASSERT (handle == test_handle, ("read callback, bad handle"));
	TEST_ASSERT (result == MATE_VFS_OK, ("read callback, bad result"));
	TEST_ASSERT (buffer == read_buffer, ("read callback, bad buffer"));
	TEST_ASSERT (bytes_requested == 1, ("read callback, bad bytes_requested"));
	TEST_ASSERT (bytes_read == 1, ("read callback, bad bytes_read"));
	TEST_ASSERT (callback_data == test_callback_data, ("read callback, bad callback data"));

	file_read_flag = TRUE;
}

static gboolean directory_load_flag;

static void
directory_load_callback (MateVFSAsyncHandle *handle,
			 MateVFSResult result,
			 GList *list,
			 guint entries_read,
			 gpointer callback_data)
{
	GList *element;
	MateVFSFileInfo *info;

	for (element = list; element != NULL; element = element->next) {
		info = element->data;
		mate_vfs_file_info_ref (info);
	}
	
	for (element = list; element != NULL; element = element->next) {
		info = element->data;
		mate_vfs_file_info_unref (info);
	}
	
	directory_load_flag = TRUE;
}

static gboolean directory_load_failed_flag;

static void
directory_load_failed_callback (MateVFSAsyncHandle *handle,
				MateVFSResult result,
				GList *list,
				guint entries_read,
				gpointer callback_data)
{
	g_assert (result != MATE_VFS_OK);
	directory_load_failed_flag = TRUE;
}

static void
test_open_read_close (void)
{
	file_open_flag = FALSE;
	mate_vfs_async_open (&test_handle,
			      temp_file_uri,
			      MATE_VFS_OPEN_READ,
			      0,
			      file_open_callback,
			      test_callback_data);
	TEST_ASSERT (wait_for_boolean (&file_open_flag), ("open: callback was not called"));

	file_read_flag = FALSE;
	mate_vfs_async_read (test_handle,
			      read_buffer,
			      1,
			      file_read_callback,
			      test_callback_data);

	TEST_ASSERT (wait_for_boolean (&file_read_flag), ("open read close: read callback was not called"));
	file_closed_flag = FALSE;
	mate_vfs_async_close (test_handle,
			       file_close_callback,
			       test_callback_data);

	TEST_ASSERT (wait_for_boolean (&file_closed_flag), ("open read close: close callback was not called"));

	TEST_ASSERT (wait_until_vfs_jobs_gone (), ("open read cancel close: job never went away"));
	TEST_ASSERT (get_used_file_descriptor_count () == 0,
		     ("open read cancel close: %d file descriptors leaked", get_used_file_descriptor_count ()));
	free_at_start = get_free_file_descriptor_count ();
}

static void
test_open_read_cancel_close (void)
{
	file_open_flag = FALSE;
	mate_vfs_async_open (&test_handle,
			      temp_file_uri,
			      MATE_VFS_OPEN_READ,
			      0,
			      file_open_callback,
			      test_callback_data);
	TEST_ASSERT (wait_for_boolean (&file_open_flag), ("open: callback was not called"));

	file_read_flag = FALSE;
	mate_vfs_async_read (test_handle,
			      read_buffer,
			      1,
			      file_read_callback,
			      test_callback_data);
	mate_vfs_async_cancel (test_handle);

	file_closed_flag = FALSE;
	mate_vfs_async_close (test_handle,
			       file_close_callback,
			       test_callback_data);

	TEST_ASSERT (wait_for_boolean (&file_closed_flag), ("open read cancel close: callback was not called"));
	TEST_ASSERT (!file_read_flag, ("open read cancel close: read callback was called"));

	TEST_ASSERT (wait_until_vfs_jobs_gone (), ("open read cancel close: job never went away"));
	TEST_ASSERT (get_used_file_descriptor_count () == 0,
		     ("open read cancel close: %d file descriptors leaked", get_used_file_descriptor_count ()));
	free_at_start = get_free_file_descriptor_count ();
}

static void
test_open_close (void)
{
	file_open_flag = FALSE;
	mate_vfs_async_open (&test_handle,
			      temp_file_uri,
			      MATE_VFS_OPEN_READ,
			      0,
			      file_open_callback,
			      test_callback_data);
	TEST_ASSERT (wait_for_boolean (&file_open_flag), ("open: open callback was not called"));
	
	file_closed_flag = FALSE;
	mate_vfs_async_close (test_handle,
			       file_close_callback,
			       test_callback_data);


	TEST_ASSERT (wait_for_boolean (&file_closed_flag), ("open close: close callback was not called"));
	
	TEST_ASSERT (wait_until_vfs_jobs_gone (), ("open close 1: job never went away"));
	TEST_ASSERT (get_used_file_descriptor_count () == 0,
		     ("open close 1: %d file descriptors leaked", get_used_file_descriptor_count ()));
	free_at_start = get_free_file_descriptor_count ();
}

static void
empty_close_callback (MateVFSAsyncHandle *handle,
		      MateVFSResult result,
		      gpointer callback_data)
{
}

static void
test_open_cancel (void)
{
	file_open_flag = FALSE;
	mate_vfs_async_open (&test_handle,
			      temp_file_uri,
			      MATE_VFS_OPEN_READ,
			      0,
			      file_open_callback,
			      test_callback_data);
	mate_vfs_async_cancel (test_handle);

	TEST_ASSERT (wait_until_vfs_jobs_gone (), ("open cancel 1: job never went away"));
	TEST_ASSERT (!file_open_flag, ("open cancel 1: open callback was called"));
	TEST_ASSERT (get_used_file_descriptor_count () == 0,
		     ("open cancel 1: %d file descriptors leaked", get_used_file_descriptor_count ()));
	free_at_start = get_free_file_descriptor_count ();

	file_open_flag = FALSE;
	mate_vfs_async_open (&test_handle,
			      temp_file_uri,
			      MATE_VFS_OPEN_READ,
			      0,
			      file_open_callback,
			      test_callback_data);
	wait_until_vfs_jobs_gone_no_main ();
	mate_vfs_async_cancel (test_handle);
	if (file_open_flag) { /* too quick */
		mate_vfs_async_close (test_handle, empty_close_callback, NULL);
	}
	TEST_ASSERT (wait_until_file_descriptors_gone (),
		     ("open cancel 2: %d file descriptors leaked", get_used_file_descriptor_count ()));
	free_at_start = get_free_file_descriptor_count ();
	TEST_ASSERT (wait_until_vfs_jobs_gone (), ("open cancel 2: later job never went away"));
	TEST_ASSERT (!file_open_flag, ("open cancel 2: open callback was called"));
}

static void
file_open_fail_callback (MateVFSAsyncHandle *handle,
			 MateVFSResult result,
			 gpointer callback_data)
{
	TEST_ASSERT (handle == test_handle, ("open callback, bad handle"));
	TEST_ASSERT (result == MATE_VFS_ERROR_NOT_FOUND, ("open callback, bad result"));
	TEST_ASSERT (callback_data == test_callback_data, ("open callback, bad callback data"));

	file_open_flag = TRUE;
}

static void
test_open_fail (void)
{
	file_open_flag = FALSE;
	mate_vfs_async_open (&test_handle,
			      "file:///etc/mugwump-xxx",
			      MATE_VFS_OPEN_READ,
			      0,
			      file_open_fail_callback,
			      test_callback_data);
	TEST_ASSERT (wait_for_boolean (&file_open_flag), ("open fail 1: callback was not called"));
	TEST_ASSERT (wait_until_vfs_jobs_gone (), ("open fail 1: job never went away"));
	TEST_ASSERT (get_used_file_descriptor_count () == 0,
		     ("open fail 1: %d file descriptors leaked", get_used_file_descriptor_count ()));
	free_at_start = get_free_file_descriptor_count ();
}

static void
my_yield (int count)
{
	for (; count > 0; count--) {
		g_usleep (1);
		g_thread_yield ();
		g_main_context_iteration (NULL, FALSE);
	}
}

static void
test_load_directory_cancel (int delay_till_cancel, int chunk_count)
{
	MateVFSAsyncHandle *handle;
	guint num_entries;
	
	
	mate_vfs_async_load_directory (&handle,
					tmp_dir_uri,
					MATE_VFS_FILE_INFO_GET_MIME_TYPE
		 			 | MATE_VFS_FILE_INFO_FORCE_FAST_MIME_TYPE
		 			 | MATE_VFS_FILE_INFO_FOLLOW_LINKS,
					chunk_count,
					0,
					directory_load_callback,
					&num_entries);
	
	g_usleep (delay_till_cancel * 100);
	
	directory_load_flag = FALSE;
	mate_vfs_async_cancel (handle);
	TEST_ASSERT (wait_until_vfs_jobs_gone (), ("load directory cancel 1: job never went away delay %d",
						   delay_till_cancel));
	TEST_ASSERT (!directory_load_flag, ("load directory cancel 1: load callback was called"));

	mate_vfs_async_load_directory (&handle,
					tmp_dir_uri,
					MATE_VFS_FILE_INFO_GET_MIME_TYPE
		 			 | MATE_VFS_FILE_INFO_FORCE_FAST_MIME_TYPE
		 			 | MATE_VFS_FILE_INFO_FOLLOW_LINKS,
					chunk_count,
					0,
					directory_load_callback,
					&num_entries);
	
	my_yield (delay_till_cancel);
	
	directory_load_flag = FALSE;
	mate_vfs_async_cancel (handle);
	TEST_ASSERT (wait_until_vfs_jobs_gone (), ("load directory cancel 2: job never went away delay %d",
						   delay_till_cancel));
	TEST_ASSERT (!directory_load_flag, ("load directory cancel 2: load callback was called"));
}

static void
test_load_directory_fail (void)
{
	MateVFSAsyncHandle *handle;
	guint num_entries;
	
	directory_load_failed_flag = FALSE;
	mate_vfs_async_load_directory (&handle,
					"file:///strcprstskrzkrk",
					MATE_VFS_FILE_INFO_GET_MIME_TYPE
		 			 | MATE_VFS_FILE_INFO_FORCE_FAST_MIME_TYPE
		 			 | MATE_VFS_FILE_INFO_FOLLOW_LINKS,
					32,
					0,
					directory_load_failed_callback,
					&num_entries);
		
	TEST_ASSERT (wait_for_boolean (&directory_load_failed_flag), ("load directory 1: load callback was not called"));
	TEST_ASSERT (wait_until_vfs_jobs_gone (), ("load directory 1: job never went away"));
}

static gboolean find_directory_flag;

static void
test_find_directory_callback (MateVFSAsyncHandle *handle,
			      GList *results,
			      gpointer callback_data)
{
	GList *element;

	find_directory_flag = TRUE;

	for (element = results; element != NULL; element = element->next) {
		MateVFSFindDirectoryResult *result_item = (MateVFSFindDirectoryResult *)element->data;
		
		if (result_item->result == MATE_VFS_OK) {
			mate_vfs_uri_ref (result_item->uri);
			mate_vfs_uri_unref (result_item->uri);
		}
	}
	
	g_assert (callback_data == &find_directory_flag);
}

static void
test_find_directory (int delay_till_cancel)
{
	MateVFSAsyncHandle *handle;
	GList *vfs_uri_as_list = NULL;


#ifndef G_OS_WIN32
	vfs_uri_as_list = g_list_append (vfs_uri_as_list, mate_vfs_uri_new ("file://~"));
#else
	vfs_uri_as_list = g_list_append (vfs_uri_as_list,
					 mate_vfs_uri_new (g_strconcat ("file://",
									 g_get_home_dir (),
									 NULL)));
#endif
	vfs_uri_as_list = g_list_append (vfs_uri_as_list, mate_vfs_uri_new ("file:///ace_of_spades"));
	
	find_directory_flag = FALSE;
	
	mate_vfs_async_find_directory (&handle, vfs_uri_as_list,
		MATE_VFS_DIRECTORY_KIND_TRASH, FALSE, TRUE, 0777, 0,
		test_find_directory_callback, &find_directory_flag);
		
	TEST_ASSERT (wait_for_boolean (&find_directory_flag),
		     ("find directory cancel 1: callback was not called %d",
		      delay_till_cancel));
	
	find_directory_flag = FALSE;
	
	mate_vfs_async_find_directory (&handle, vfs_uri_as_list,
		MATE_VFS_DIRECTORY_KIND_TRASH, FALSE, TRUE, 0777, 0,
		test_find_directory_callback, &find_directory_flag);
	
	g_usleep (delay_till_cancel * 100);
	
	mate_vfs_async_cancel (handle);
	TEST_ASSERT (wait_until_vfs_jobs_gone (), ("find directory cancel 2: job never went away"));
	TEST_ASSERT (!find_directory_flag, ("find directory cancel 2: callback was called"));

	
	mate_vfs_async_find_directory (&handle, vfs_uri_as_list,
		MATE_VFS_DIRECTORY_KIND_TRASH, FALSE, TRUE, 0777, 0,
		test_find_directory_callback, &find_directory_flag);
	
	my_yield (delay_till_cancel);
	
	find_directory_flag = FALSE;
	mate_vfs_async_cancel (handle);
	TEST_ASSERT (wait_until_vfs_jobs_gone (), ("open cancel 3: job never went away"));
	TEST_ASSERT (!find_directory_flag, ("find directory cancel 3: callback was called"));

	mate_vfs_uri_list_free (vfs_uri_as_list);
}

int
main (int argc, char **argv)
{
	char *temp_file_name;
	int fd = g_file_open_tmp (NULL, &temp_file_name, NULL);

	temp_file_uri = g_strconcat ("file:///", temp_file_name, NULL);
	write (fd, "Hello\n", strlen ("Hello\n"));
	close (fd);
	
	tmp_dir_uri = g_strconcat ("file:///", g_get_tmp_dir (), NULL);

	make_asserts_break("MateVFS");
	mate_vfs_init ();

	/* Initialize our own stuff. */
	free_at_start = get_free_file_descriptor_count ();

	/* Do the basic tests of our own tools. */
	TEST_ASSERT (get_used_file_descriptor_count () == 0, ("file descriptor count"));
	TEST_ASSERT (mate_vfs_job_get_count () == 0, ("VFS job count"));

	/* Spend those first few file descriptors. */
	first_get_file_info ();
	free_at_start = get_free_file_descriptor_count ();

	/* Test to see that a simple async. call works without leaking or anything. */
	fprintf (stderr, "Testing get file info...\n");
	test_get_file_info ();
	test_get_file_info ();
	fprintf (stderr, "Testing open, close...\n");
	test_open_close ();
	test_open_close ();
	fprintf (stderr, "Testing read, close...\n");
	test_open_read_close ();
	test_open_read_close ();
	fprintf (stderr, "Testing cancellation...\n");
	test_open_cancel ();
	test_open_cancel ();

	fprintf (stderr, "Testing failed opens...\n");
	test_open_fail ();
	test_open_fail ();
	fprintf (stderr, "Testing read, cancel, closes...\n");
	test_open_read_cancel_close ();
	test_open_read_cancel_close ();

	fprintf (stderr, "Testing directory loads");
	test_load_directory_fail ();
	test_load_directory_cancel (0, 1);
	test_load_directory_cancel (1, 1);
	test_load_directory_cancel (10, 1);
	test_load_directory_cancel (100, 1);
	fprintf (stderr, ".");
	test_load_directory_cancel (0, 1);
	test_load_directory_cancel (1, 1);
	test_load_directory_cancel (10, 1);
	test_load_directory_cancel (100, 1);
	fprintf (stderr, ".");

	test_load_directory_cancel (0, 32);
	test_load_directory_cancel (1, 32);
	test_load_directory_cancel (10, 32);
	test_load_directory_cancel (100, 32);
	fprintf (stderr, ".");
	test_load_directory_cancel (0, 32);
	test_load_directory_cancel (1, 32);
	test_load_directory_cancel (10, 32);
	test_load_directory_cancel (100, 32);

	fprintf (stderr, "\nTesting directory finds");
	test_find_directory (0);
	test_find_directory (0);
	fprintf (stderr, ".");
	test_find_directory (1);
	test_find_directory (1);
	fprintf (stderr, ".");
	test_find_directory (10);
	test_find_directory (10);
	fprintf (stderr, ".");
	test_find_directory (100);
	test_find_directory (100);

	fprintf (stderr, "\nTesting shutdown...\n");
	mate_vfs_shutdown ();

	if (g_getenv ("_MEMPROF_SOCKET")) {
		g_warning ("Waiting for memprof\n");
		g_main_context_iteration (NULL, TRUE);
	}

	if (!at_least_one_test_failed) {
		fprintf (stderr, "All tests passed successfully.\n");
	}

	g_unlink (temp_file_name);

	/* Report to "make check" on whether it all worked or not. */
	return at_least_one_test_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
