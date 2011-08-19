/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* test-channel.c - Test for the `open_as_channel' functionality of the MATE
   Virtual File System.

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

   Author: Ettore Perazzoli <ettore@comm2000.it> */

#include <config.h>

#include <glib.h>
#include <libmatevfs/mate-vfs-async-ops.h>
#include <libmatevfs/mate-vfs-init.h>
#include <stdio.h>

#define BUFFER_SIZE 4096

static GMainLoop *main_loop;

static gboolean
io_channel_callback (GIOChannel *source,
		     GIOCondition condition,
		     gpointer data)
{
	gchar buffer[BUFFER_SIZE + 1];
	gsize bytes_read;

	printf ("\n\n************ IO Channel callback!\n");

	if (condition & G_IO_IN) {
		g_io_channel_read_chars (source, buffer, sizeof (buffer) - 1, &bytes_read, NULL);
		buffer[bytes_read] = 0;
		printf ("---> Read %lu byte(s):\n%s\n\n(***END***)\n",
			(gulong)bytes_read, buffer);
		fflush (stdout);
	}

	if (condition & G_IO_NVAL) {
		g_warning ("channel_callback got NVAL condition.");
		return FALSE;
	}

	if (condition & G_IO_HUP) {
		printf ("\n----- EOF -----\n");
		fflush (stdout);
		g_io_channel_shutdown (source, FALSE, NULL);
		g_main_loop_quit (main_loop);
		return FALSE;
	}

	return TRUE;
}

static void open_callback (MateVFSAsyncHandle *handle,
			   GIOChannel *channel,
			   MateVFSResult result,
			   gpointer data)
{
	if (result != MATE_VFS_OK) {
		printf ("Error opening: %s.\n",
			mate_vfs_result_to_string (result));
		return;
	}

	printf ("Open successful, callback data `%s'.\n", (gchar *) data);
	g_io_add_watch_full (channel,
			     G_PRIORITY_HIGH,
			     G_IO_IN | G_IO_NVAL | G_IO_HUP,
			     io_channel_callback, handle,
			     NULL);

	g_io_channel_unref (channel);
}


int
main (int argc, char **argv)
{
	MateVFSAsyncHandle *handle;

	if (argc < 2) {
		fprintf (stderr, "Usage: %s <uri>\n", argv[0]);
		return 1;
	}

	puts ("Initializing mate-vfs...");
	mate_vfs_init ();

	printf ("Starting open for `%s'...\n", argv[1]);
	mate_vfs_async_open_as_channel (&handle, argv[1],
					 MATE_VFS_OPEN_READ,
					 BUFFER_SIZE,
					 0,
					 open_callback,
					 "open_callback");

	puts ("GTK+ main loop running.");
	main_loop = g_main_loop_new (NULL, TRUE);
	g_main_loop_run (main_loop);
	g_main_loop_unref (main_loop);

	puts ("GTK+ main loop finished.");

	puts ("All done");

	while (1)
		;

	return 0;
}
