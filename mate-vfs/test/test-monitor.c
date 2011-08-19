/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* test-monitor.c - Test program for file monitoringu in the MATE Virtual
   File System.

   Copyright (C) 2001 Ian McKellar

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
	Ian McKellar <yakk@yakk.net>
 */

#include <libmatevfs/mate-vfs.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <glib.h>

static GMainLoop *main_loop;

static gboolean
timeout_cb (gpointer data)
{
	static int i = 0;

	if (i == 0)
	{
		unlink ("/tmp/test-monitor");
		g_spawn_command_line_sync ("touch /tmp/test-monitor",
				NULL, NULL, NULL, NULL);
	}

	if (i == 1)
	{
		unlink ("/tmp/test-monitor");
	}

	i++;

	return TRUE;
}

static void
show_result (MateVFSResult result, const gchar *what, const gchar *text_uri)
{
	fprintf (stderr, "%s `%s': %s\n",
		 what, text_uri, mate_vfs_result_to_string (result));
	if (result != MATE_VFS_OK)
		exit (1);
}

static void
callback (MateVFSMonitorHandle *handle,
          const gchar *monitor_uri,
	  const gchar *info_uri,
	  MateVFSMonitorEventType event_type,
	  gpointer user_data) {
	static int i = 0;

	g_print ("Got a callback: ");
	switch (event_type) {
		case MATE_VFS_MONITOR_EVENT_CHANGED:
			g_print ("MATE_VFS_MONITOR_EVENT_CHANGED");
			break;
		case MATE_VFS_MONITOR_EVENT_DELETED:
			g_print ("MATE_VFS_MONITOR_EVENT_DELETED");
			break;
		case MATE_VFS_MONITOR_EVENT_STARTEXECUTING:
			g_print ("MATE_VFS_MONITOR_EVENT_STARTEXECUTING");
			break;
		case MATE_VFS_MONITOR_EVENT_STOPEXECUTING:
			g_print ("MATE_VFS_MONITOR_EVENT_STOPEXECUTING");
			break;
		case MATE_VFS_MONITOR_EVENT_CREATED:
			g_print ("MATE_VFS_MONITOR_EVENT_CREATED");
			break;
		case MATE_VFS_MONITOR_EVENT_METADATA_CHANGED:
			g_print ("MATE_VFS_MONITOR_EVENT_METADATA_CHANGED");
			break;
		default:
			g_print ("Unknown monitor type, exiting...");
			exit (1);
	}

	g_print (" (%s)", info_uri);
	g_print ("\n");
	i++;
	if (i >= 2)
		exit (0);
}

int
main (int argc, char **argv)
{
	MateVFSResult    result;
	gchar            *text_uri = "/tmp/";
	MateVFSMonitorHandle *handle;

	if (! mate_vfs_init ()) {
		fprintf (stderr, "Cannot initialize mate-vfs.\n");
		return 1;
	}

	if (argc == 2) {
		text_uri = argv[1];
	}

	result = mate_vfs_monitor_add (&handle, text_uri,
			MATE_VFS_MONITOR_DIRECTORY, callback, "user data");
	printf ("handle is %p\n", handle);
	show_result (result, "monitor_add", text_uri);

	g_timeout_add (1000, timeout_cb, NULL);

	if (result == MATE_VFS_OK) {
		main_loop = g_main_loop_new (NULL, TRUE);
		g_main_loop_run (main_loop);
		g_main_loop_unref (main_loop);
	}

	g_free (text_uri);

	return 0;
}
