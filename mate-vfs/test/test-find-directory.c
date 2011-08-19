/* test-mime.c - Test for the mate_vfs_find_directory call
   Virtual File System Library

   Copyright (C) 2000 Eazel

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

   Author: Pavel Cisler <pavel@eazel.com>
*/

#include <config.h>

#include <glib.h>
#include <libmatevfs/mate-vfs-find-directory.h>
#include <libmatevfs/mate-vfs-init.h>
#include <libmatevfs/mate-vfs-uri.h>
#include <stdio.h>
#include <string.h>

int
main (int argc, char **argv)
{
	MateVFSURI *uri;
	MateVFSURI *result;
	MateVFSResult error;
	char *path;
	gboolean create;

	create = FALSE;

	if (!mate_vfs_init ()) {
		fprintf (stderr, "Cannot initialize mate-vfs.\n");
		return 1;
	}

	if (argc == 1) {
		fprintf (stderr, "Usage: %s [-create] near_uri \n", *argv);
		return 1;
	}


	++argv;

	if (strcmp (*argv, "-create") == 0) {
		create = TRUE;
		++argv;
	}

	uri = mate_vfs_uri_new (*argv);
	error = mate_vfs_find_directory (uri, MATE_VFS_DIRECTORY_KIND_TRASH, &result, create, 
		TRUE, 0777);
	if (error == MATE_VFS_OK) {
		path = mate_vfs_uri_to_string (result, MATE_VFS_URI_HIDE_NONE);
		g_print ("found trash at %s\n", path);
		g_free (path);
		error = mate_vfs_find_directory (uri, MATE_VFS_DIRECTORY_KIND_TRASH, 
			&result, FALSE, FALSE, 0777);
		if (error == MATE_VFS_OK) {
			path = mate_vfs_uri_to_string (result, MATE_VFS_URI_HIDE_NONE);
			g_print ("found it again in a cached entry at %s\n", path);
			g_free (path);
		} else {
			g_print ("error %s finding cached trash entry near %s\n", mate_vfs_result_to_string (error),
				*argv);
		}
	} else {
		g_print ("error %s finding trash near %s\n", mate_vfs_result_to_string (error),
			*argv);
	}
	
	return 0;
}
