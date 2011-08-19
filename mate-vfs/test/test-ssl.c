/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* test-sync.c - Test program for synchronous operation of the MATE
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

   Author: Ian McKellar <yakk@yakk.net> 
 */

#include <config.h>

#include <glib.h>
#include <libmatevfs/mate-vfs-init.h>
#include <libmatevfs/mate-vfs-socket-buffer.h>
#include <libmatevfs/mate-vfs-socket.h>
#include <libmatevfs/mate-vfs-ssl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum {
	SSL, SOCKET, SOCKETBUFFER
} abstraction = SOCKETBUFFER;

static void
show_result (MateVFSResult result, 
	     const gchar *what, 
	     const gchar *host, 
	     gint port)
{
	fprintf (stderr, "%s `%s:%d': %s\n",
		 what, host, port, mate_vfs_result_to_string (result));
	if (result != MATE_VFS_OK)
		exit (1);
}

#define HTTP_REQUEST "GET / HTTP/1.0\r\n\r\n"

int
main (int argc, char **argv)
{
	MateVFSResult        result = MATE_VFS_OK;
	gchar                 buffer[1024];
	gchar		     *host;
	gint                  port;
	MateVFSFileSize      bytes_read;
	MateVFSSSL          *ssl = NULL;
	MateVFSSocket       *socket = NULL;
	MateVFSSocketBuffer *socketbuffer = NULL;

	if (argc != 3) {
		printf ("Usage: %s <host> <port>\n", argv[0]);
		return 1;
	}

	host = argv[1];
	port = atoi (argv[2]);

	if (port <= 0) {
		printf ("Invalid port\n");
		return 1;
	}

	if (! mate_vfs_init ()) {
		fprintf (stderr, "Cannot initialize mate-vfs.\n");
		return 1;
	}

	switch (abstraction) {
		case SOCKETBUFFER:
			g_print ("Testing MateVFSSocketBuffer");
		case SOCKET:
			g_print (" and MateVFSSocket");
		case SSL:
			g_print (" and MateVFSSSL");
	}
	g_print (".\n");

	result = mate_vfs_ssl_create (&ssl, host, port, NULL);

	show_result (result, "ssl_create", host, port);

	if (ssl == NULL) {
		fprintf (stderr, "couln't connect\n");
		return -1;
	}

	if (abstraction >= SOCKET) {
		socket = mate_vfs_ssl_to_socket (ssl);
		if (socket == NULL) {
			fprintf (stderr, "couldn't create socket object\n");
			return -1;
		}

		if (abstraction == SOCKETBUFFER) {
			socketbuffer = mate_vfs_socket_buffer_new (socket);
			if (socketbuffer == NULL) {
				fprintf (stderr, 
				       "couldn't create socketbuffer object\n");
				return -1;
			}
		}
	}

	switch (abstraction) {
		case SSL:
			result = mate_vfs_ssl_write (ssl, HTTP_REQUEST, 
						      strlen(HTTP_REQUEST), &bytes_read,
						      NULL);
			break;
		case SOCKET:
			result = mate_vfs_socket_write (socket, HTTP_REQUEST, 
							 strlen(HTTP_REQUEST), &bytes_read,
							 NULL);
			break;
		case SOCKETBUFFER:
			result = mate_vfs_socket_buffer_write (socketbuffer,
								HTTP_REQUEST, strlen(HTTP_REQUEST),
								&bytes_read,
								NULL);
			mate_vfs_socket_buffer_flush (socketbuffer, NULL);
			break;
	}

	show_result (result, "write", host, port);

	while( result==MATE_VFS_OK ) {
		switch (abstraction) {
			case SSL:
				result = mate_vfs_ssl_read (ssl, buffer, 
							     sizeof buffer - 1, &bytes_read,
							     NULL);
				break;
			case SOCKET:
				result = mate_vfs_socket_read (socket, buffer, 
								sizeof buffer - 1, &bytes_read,
								NULL);
				break;
			case SOCKETBUFFER:
				result = mate_vfs_socket_buffer_read (
						socketbuffer, buffer, 
						sizeof buffer - 1, &bytes_read,
						NULL);
				break;
		}
		show_result (result, "read", host, port);
	
		buffer[bytes_read] = 0;
		write (1,buffer,bytes_read);
		if(!bytes_read) break;
	}

	switch (abstraction) {
		case SSL:
			mate_vfs_ssl_destroy (ssl, NULL);
			break;
		case SOCKET:
			mate_vfs_socket_close (socket, NULL);
			break;
		case SOCKETBUFFER:
			mate_vfs_socket_buffer_destroy (socketbuffer, TRUE, NULL);
			break;
	}

	return 0;
}
