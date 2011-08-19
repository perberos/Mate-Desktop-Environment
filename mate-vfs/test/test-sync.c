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

   Author: Ettore Perazzoli <ettore@gnu.org> */

#include <config.h>

#include <libmatevfs/mate-vfs-init.h>
#include <libmatevfs/mate-vfs-ops.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void
show_result (MateVFSResult result, const gchar *what, const gchar *text_uri)
{
	fprintf (stderr, "%s `%s': %s\n",
		 what, text_uri, mate_vfs_result_to_string (result));
	if (result != MATE_VFS_OK && result != MATE_VFS_ERROR_EOF)
		exit (1);
}

int
main (int argc, char **argv)
{
	MateVFSResult    result;
	MateVFSHandle   *handle;
	gchar             buffer[1024];
	MateVFSFileSize  bytes_read;
	MateVFSURI 	 *uri;
	gchar            *text_uri;
	gchar            *out;

	if (argc != 2) {
		printf ("Usage: %s <uri>\n", argv[0]);
		return 1;
	}

	if (! mate_vfs_init ()) {
		fprintf (stderr, "Cannot initialize mate-vfs.\n");
		return 1;
	}

	uri = mate_vfs_uri_new (argv[1]);
	if (uri == NULL) {
		fprintf (stderr, "URI not valid.\n");
		return 1;
	}

	text_uri = mate_vfs_uri_to_string (uri, MATE_VFS_URI_HIDE_NONE);

	result = mate_vfs_open_uri (&handle, uri, MATE_VFS_OPEN_READ);
	show_result (result, "open", text_uri);

	while( result==MATE_VFS_OK ) {
		result = mate_vfs_read (handle, buffer, sizeof buffer - 1,
				 	&bytes_read);
		show_result (result, "read", text_uri);
	
		buffer[bytes_read] = 0;
		write (1,buffer,bytes_read);
		if(!bytes_read) break;
	}

	result = mate_vfs_file_control (handle, "file:test", &out);
	show_result (result, "file_control", text_uri);
	if (result == MATE_VFS_OK) {
		g_print ("file_control file:test: %s\n", out);
	} 
		
	
	
	result = mate_vfs_close (handle);
	show_result (result, "close", text_uri);

	g_free (text_uri);

	return 0;
}
