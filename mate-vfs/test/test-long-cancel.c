/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* test-long-cancel.c - Test program for the MATE Virtual File System.

   Copyright (C) 2005 Red Hat, Inc

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

   Author: Alexander Larsson <alexl@redhat.com>
*/

/* This test verifies that cancels work by starting up a "simple webserver"
 * that just waits forever when you request anything from it. We then cancel
 * the request after a while and make the request return. This should cause
 * the request async operation to not be called, and nothing should hang.
 */

#include <config.h>

#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <glib.h>
#ifndef G_OS_WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#define CLOSE_SOCKET(fd) close (fd)
#else
#include <winsock2.h>
#define CLOSE_SOCKET(fd) closesocket(fd)
typedef int socklen_t;
#endif
#include <unistd.h>
#include <stdlib.h>

#include <glib.h>
#include <libmatevfs/mate-vfs-async-ops.h>
#include <libmatevfs/mate-vfs-init.h>

static GMainLoop *main_loop;

static int
create_master_socket (int *port)
{
	struct sockaddr_in sin;
	socklen_t len;
	int s;
	int val;

	memset ((char *)&sin, 0, sizeof (sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = 0;

	/* Create a socket: */
	s = socket (PF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		fprintf (stderr, "Error: couldn't create socket!\n");
		exit (1);
	}

	val = 1;
	setsockopt (s, SOL_SOCKET, SO_REUSEADDR, (char *) &val, sizeof (val));

	/* bind the socket: */
	if (bind (s,(struct sockaddr *)&sin, sizeof (sin)) < 0) {
		fprintf (stderr, "Error: couldn't bind socket to port\n");
		exit (1);
	}
	if (listen (s, 10) < 0) {
		fprintf (stderr, "Error: couldn't listen at port\n");
		exit (1);
	}

	len = sizeof (sin);
	if (getsockname (s, (struct sockaddr *)&sin, &len) == -1) {
		fprintf (stderr, "Error: couldn't get port\n");
		exit (1);
	}

	*port = ntohs (sin.sin_port);

	return s;
}

static int client_fd = -1;

static gboolean
new_client (GIOChannel  *channel,
	    GIOCondition cond,
	    gpointer     callback_data)
{
	int fd;
	struct sockaddr_in addr;
	socklen_t addrlen;

	fd = g_io_channel_unix_get_fd (channel);

	addrlen = sizeof (addr);
	client_fd = accept (fd, (struct sockaddr *) &addr, &addrlen);
	if (client_fd == -1) {
		fprintf (stderr, "Error: couldn't accept client\n");
		exit (1);
	}

	g_print ("Got new client\n");

	return TRUE;
}

static void
got_file_info (MateVFSAsyncHandle *handle,
	       GList *results, /* MateVFSGetFileInfoResult* items */
	       gpointer callback_data)
{
	MateVFSGetFileInfoResult *res;

	res = results->data;

	fprintf (stderr, "Error: got file info (%s), shouldn't happen.\n", mate_vfs_result_to_string (res->result));
	exit (1);
}

static gboolean
long_timeout (gpointer data)
{
	fprintf (stderr, "Error: timed out the cancel.\n");
	exit (1);
	return FALSE;
}


static gboolean
quit_ok_timeout (gpointer data)
{
	fprintf (stderr, "correctly cancelled operation\n");
	g_main_loop_quit (main_loop);
	return FALSE;
}


static gboolean
start_cancel_timeout (gpointer data)
{
	MateVFSAsyncHandle *handle;

	handle = data;

	fprintf (stderr, "cancelling request\n");
	mate_vfs_async_cancel (handle);
	fprintf (stderr, "cancelled\n");

	if (client_fd == -1) {
		fprintf (stderr, "Error: hasn't gotten client request\n");
		exit (1);
	}

	fprintf (stderr, "closing client\n");
	CLOSE_SOCKET (client_fd);

	/* Wait a while to make sure get_info doesn't return after the cancel */
	g_timeout_add (500,
		       quit_ok_timeout, NULL);

	return FALSE;
}

int
main (int argc, char *argv[])
{
	int fd, port;
	GIOChannel *channel;
	char *uri;
	MateVFSAsyncHandle *handle;
	GList *uris;
#ifdef G_OS_WIN32
	DWORD wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup (wVersionRequested, &wsaData)) {
		fprintf (stderr, "WSAStartup failed\n");
		exit (1);
	}
#endif

	fprintf (stderr, "Initializing mate-vfs...\n");
	mate_vfs_init ();

	main_loop = g_main_loop_new (NULL, TRUE);

	fd = create_master_socket (&port);

	channel = g_io_channel_unix_new (fd);
	g_io_add_watch (channel,
			G_IO_IN | G_IO_HUP,
			new_client, NULL);
	g_io_channel_unref (channel);

	uri = g_strdup_printf ("http://127.0.0.1:%d/", port);
	/* uri = "http://gnome.org/"; */
	fprintf (stderr, "uri: %s\n", uri);

	uris = g_list_prepend (NULL, mate_vfs_uri_new (uri));

	mate_vfs_async_get_file_info (&handle,
				       uris, MATE_VFS_FILE_INFO_DEFAULT,
				       MATE_VFS_PRIORITY_DEFAULT,
				       got_file_info, NULL);

	g_timeout_add (500,
		       start_cancel_timeout, handle);

	g_timeout_add (10000,
		       long_timeout, NULL);

	fprintf (stderr, "Main loop running.\n");
	g_main_loop_run (main_loop);


	mate_vfs_shutdown ();

	return 0;
}
