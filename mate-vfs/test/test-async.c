/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* test-vfs.c - Test program for the MATE Virtual File System.

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

   Author: Ettore Perazzoli <ettore@comm2000.it>
*/

#include <config.h>

#include <glib.h>
#include <libmatevfs/mate-vfs-async-ops.h>
#include <libmatevfs/mate-vfs-init.h>
#include <stdio.h>

#define QUEUE_LENGTH 4000

static GMainLoop *main_loop;

/* Callbacks.  */
static void
close_callback (MateVFSAsyncHandle *handle,
		MateVFSResult result,
		gpointer callback_data)
{
	fprintf (stderr, "Close: %s.\n", mate_vfs_result_to_string (result));
	g_main_loop_quit (main_loop);
}

static void
file_control_callback (MateVFSAsyncHandle *handle,
		       MateVFSResult result,
		       gpointer operation_data,
		       gpointer callback_data)
{
	if (result != MATE_VFS_OK) {
		fprintf (stderr, "file_control failed: %s\n",
			 mate_vfs_result_to_string (result));
	} else {
		printf ("file_control result: %s\n", *(char **)operation_data);
	}
	
	fprintf (stderr, "Now closing the file.\n");
	mate_vfs_async_close (handle, close_callback, "close");
}

static void
read_callback (MateVFSAsyncHandle *handle,
	       MateVFSResult result,
               gpointer buffer,
               MateVFSFileSize bytes_requested,
	       MateVFSFileSize bytes_read,
               gpointer callback_data)
{
	char **op_data;
	
	if (result != MATE_VFS_OK) {
		fprintf (stderr, "Read failed: %s\n",
			 mate_vfs_result_to_string (result));
	} else {
		printf ("%"MATE_VFS_SIZE_FORMAT_STR"/"
			"%"MATE_VFS_SIZE_FORMAT_STR" "
			"byte(s) read, callback data `%s'\n",
			bytes_read, bytes_requested,
			(gchar *) callback_data);
		*((gchar *) buffer + bytes_read) = 0;
		fprintf (stderr, "%s", (char *) buffer);
	}

	fprintf (stderr, "Now testing file_control.\n");
	op_data = g_new (char *, 1);
	mate_vfs_async_file_control (handle, "file:test", op_data, g_free, file_control_callback, "file_control");
}

static void
open_callback  (MateVFSAsyncHandle *handle,
		MateVFSResult result,
                gpointer callback_data)
{
	if (result != MATE_VFS_OK) {
		fprintf (stderr, "Open failed: %s.\n",
			 mate_vfs_result_to_string (result));
		g_main_loop_quit (main_loop);
	} else {
		gchar *buffer;
		const gulong buffer_size = 1024;

		fprintf (stderr, "File opened correctly, data `%s'.\n",
			 (gchar *) callback_data);

		buffer = g_malloc (buffer_size);
		mate_vfs_async_read (handle,
				      buffer,
				      buffer_size - 1,
				      read_callback,
				      "read_callback");
	}
}

static void
dummy_close_callback (MateVFSAsyncHandle *handle,
		      MateVFSResult result,
		      gpointer callback_data)
{
}

static void
async_queue_callback  (MateVFSAsyncHandle *handle,
		       MateVFSResult result,
		       gpointer callback_data)
{
	int *completed = callback_data;

	(*completed)++;

	if (result == MATE_VFS_OK)
		mate_vfs_async_close (handle, dummy_close_callback, NULL);
}

int
main (int argc, char **argv)
{
	int completed, i;
	MateVFSAsyncHandle *handle;

	if (argc < 2) {
		fprintf (stderr, "Usage: %s <uri of text file>\n", argv[0]);
		return 1;
	}

	fprintf (stderr, "Initializing mate-vfs...\n");
	mate_vfs_init ();

	fprintf (stderr, "Creating async context...\n");

	fprintf (stderr, "Starting open for `%s'...\n", argv[1]);
	mate_vfs_async_open (&handle, argv[1], MATE_VFS_OPEN_READ,
			      MATE_VFS_PRIORITY_MIN,
			      open_callback, "open_callback");

	fprintf (stderr, "Main loop running.\n");
	main_loop = g_main_loop_new (NULL, TRUE);
	g_main_loop_run (main_loop);

	fprintf (stderr, "Main loop finished.\n");

	fprintf (stderr, "Test async queue efficiency ...");

	for (completed = i = 0; i < QUEUE_LENGTH; i++) {
		mate_vfs_async_open (&handle, argv [1], MATE_VFS_OPEN_READ, 0,
				      async_queue_callback, &completed);
	}

	while (completed < QUEUE_LENGTH)
		g_main_context_iteration (NULL, TRUE);

	fprintf (stderr, "Passed\n");

	g_main_loop_unref (main_loop);

	fprintf (stderr, "All done\n");

	mate_vfs_shutdown ();

	return 0;
}
