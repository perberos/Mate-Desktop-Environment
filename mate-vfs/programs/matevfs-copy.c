/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* matevfs-copy.c - Test for open(), read() and write() for mate-vfs

   Copyright (C) 2003-2005, Red Hat

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

   Author: Bastien Nocera <hadess@hadess.net>
           Chrsitian Kellner <gicmo@gnome.org>
*/

#include <libmatevfs/mate-vfs.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "authentication.c"

int
main (int argc, char **argv)
{
	MateVFSResult result;
	char *text_uri;
	MateVFSURI *src, *dest;
	MateVFSFileInfo *info;
	
	if (argc != 3) {
		printf ("Usage: %s <src> <dest>\n", argv[0]);
		return 1;
	}

	if (!mate_vfs_init ()) {
		fprintf (stderr, "Cannot initialize mate-vfs.\n");
		return 1;
	}

	command_line_authentication_init ();
	
	text_uri = mate_vfs_make_uri_from_shell_arg (argv[1]);

	src = mate_vfs_uri_new (text_uri);
	g_free (text_uri);

	text_uri = mate_vfs_make_uri_from_shell_arg (argv[2]);
	
	dest = mate_vfs_uri_new (text_uri);
	g_free (text_uri);

	if (src == NULL || dest == NULL) {
		result = MATE_VFS_ERROR_INVALID_URI;
		goto out;
	}
	
	info   = mate_vfs_file_info_new ();
	result = mate_vfs_get_file_info_uri (dest, info,
					      MATE_VFS_FILE_INFO_DEFAULT);

	if (result != MATE_VFS_OK && result != MATE_VFS_ERROR_NOT_FOUND) {
		mate_vfs_file_info_unref (info);
		goto out;
	}

	/* If the target is a directory do not overwrite it but copy the
	   source into the directory! (This is like cp does it) */
	if (info->valid_fields & MATE_VFS_FILE_INFO_FIELDS_TYPE &&
	    info->type == MATE_VFS_FILE_TYPE_DIRECTORY) {
		char *name;
		MateVFSURI *new_dest;
		   
		name     = mate_vfs_uri_extract_short_path_name (src);
		new_dest = mate_vfs_uri_append_string (dest, name);
		mate_vfs_uri_unref (dest);
		g_free (name);
		dest = new_dest;
		   
	}

	mate_vfs_file_info_unref (info);
	
	result = mate_vfs_xfer_uri (src, dest,
				     MATE_VFS_XFER_RECURSIVE,
				     MATE_VFS_XFER_ERROR_MODE_ABORT,
				     MATE_VFS_XFER_OVERWRITE_MODE_REPLACE,
				     NULL, NULL);
	
out:
	if (src) {
		mate_vfs_uri_unref (src);
	}

	if (dest) {
		mate_vfs_uri_unref (dest);
	}
		
	if (result != MATE_VFS_OK) {
		fprintf (stderr, "Failed to copy %s to %s\nReason: %s\n",
			 argv[1], argv[2], mate_vfs_result_to_string (result));
		return 1;
	}

	return 0;
}
