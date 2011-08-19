/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* test-unlink.c - Test program for unlink operation in the MATE
   Virtual File System.

   Copyright (C) 1999 Free Software Foundation
   Copyright (C) 2000 Eazel Inc.

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
   	Ettore Perazzoli <ettore@gnu.org> 
	Ian McKellar <yakk@yakk.net.au>
 */

#include <config.h>

#include <libmatevfs/mate-vfs-init.h>
#include <libmatevfs/mate-vfs-ops.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void
show_result (MateVFSResult result, const gchar *what, const gchar *text_uri)
{
	fprintf (stderr, "%s `%s': %s\n",
		 what, text_uri, mate_vfs_result_to_string (result));
	if (result != MATE_VFS_OK)
		exit (1);
}

int
main (int argc, char **argv)
{
	MateVFSResult    result;
	MateVFSURI 	 *uri;
	gchar            *text_uri;

	if (argc != 3 && argc != 4) {
		printf ("Usage: %s make <uri> [mode]\n       %s remove) <uri>\n", argv[0], argv[0]);
		return 1;
	}

	if (! mate_vfs_init ()) {
		fprintf (stderr, "Cannot initialize mate-vfs.\n");
		return 1;
	}

	uri = mate_vfs_uri_new (argv[2]);
	if (uri == NULL) {
		fprintf (stderr, "URI not valid.\n");
		return 1;
	}

        text_uri = mate_vfs_uri_to_string (uri, MATE_VFS_URI_HIDE_NONE);

	if(!strcmp(argv[1], "make")) {
		int mode = 0755;

		if (argc == 4) {
			mode = strtol (argv[3], (char **)NULL, 8);
		}
		result = mate_vfs_make_directory_for_uri (uri, mode);
		show_result (result, "make_directory", text_uri);
	} else if(!strcmp(argv[1], "remove")) {
		result = mate_vfs_remove_directory_from_uri (uri);
		show_result (result, "remove_directory", text_uri);
	} else {
		fprintf (stderr, "Unknown directory operation `%s'\n", argv[1]);
	}

	g_free (text_uri);

	return 0;
}
