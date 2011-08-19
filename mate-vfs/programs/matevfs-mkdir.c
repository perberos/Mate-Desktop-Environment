/* matevfs-mkdir.c - Test for mkdir() for mate-vfs

   Copyright (C) 2003, Red Hat

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
*/

#include <libmatevfs/mate-vfs.h>
#include <stdio.h>
#include <string.h>

#include "authentication.c"

static MateVFSResult
make_directory_with_parents_for_uri (MateVFSURI * uri,
		guint perm)
{
	MateVFSResult result;
	MateVFSURI *parent, *work_uri;
	GList *list = NULL;

	result = mate_vfs_make_directory_for_uri (uri, perm);
	if (result == MATE_VFS_OK || result != MATE_VFS_ERROR_NOT_FOUND)
		return result;

	work_uri = uri;

	while (result == MATE_VFS_ERROR_NOT_FOUND) {
		parent = mate_vfs_uri_get_parent (work_uri);
		result = mate_vfs_make_directory_for_uri (parent, perm);

		if (result == MATE_VFS_ERROR_NOT_FOUND)
			list = g_list_prepend (list, parent);
		work_uri = parent;
	}

	if (result != MATE_VFS_OK) {
		/* Clean up */
		while (list != NULL) {
			mate_vfs_uri_unref ((MateVFSURI *) list->data);
			list = g_list_remove (list, list->data);
		}
	}

	while (result == MATE_VFS_OK && list != NULL) {
		result = mate_vfs_make_directory_for_uri
		    ((MateVFSURI *) list->data, perm);

		mate_vfs_uri_unref ((MateVFSURI *) list->data);
		list = g_list_remove (list, list->data);
	}

	result = mate_vfs_make_directory_for_uri (uri, perm);
	return result;
}

static MateVFSResult
make_directory_with_parents (const gchar * text_uri, guint perm)
{
	MateVFSURI *uri;
	MateVFSResult result;

	uri = mate_vfs_uri_new (text_uri);
	result = make_directory_with_parents_for_uri (uri, perm);
	mate_vfs_uri_unref (uri);

	return result;
}

int
main (int argc, char *argv[])
{
	char *text_uri;
	char *directory;
	MateVFSResult result;
	gboolean with_parents;

	if (argc > 1) {
		if (strcmp (argv[1], "-p") == 0) {
			directory = argv[2];
			with_parents = TRUE;
		} else {
			directory = argv[1];
			with_parents = FALSE;
		}
	} else {
		fprintf (stderr, "Usage: %s [-p] <dir>\n", argv[0]);
		fprintf (stderr, "   -p: Create parents of the directory if needed\n");
		return 0;
	}

	if (!mate_vfs_init ()) {
		fprintf (stderr, "Cannot initialize mate-vfs.\n");
		return 1;
	}

	command_line_authentication_init ();

	text_uri = mate_vfs_make_uri_from_shell_arg (directory);

	if (text_uri == NULL) {
		fprintf (stderr, "Could not guess URI from %s.\n", argv[1]);
		return 1;
	}	
	
	if (with_parents) {
		result = make_directory_with_parents (text_uri,
				MATE_VFS_PERM_USER_ALL
				| MATE_VFS_PERM_GROUP_ALL
				| MATE_VFS_PERM_OTHER_READ);
	} else {
		result = mate_vfs_make_directory (text_uri,
				MATE_VFS_PERM_USER_ALL
				| MATE_VFS_PERM_GROUP_ALL
				| MATE_VFS_PERM_OTHER_READ);
	}
	
	g_free (text_uri);

	if (result != MATE_VFS_OK) {
		g_print ("Error making directory %s\nReason: %s\n",
				directory,
				mate_vfs_result_to_string (result));
		return 0;
	}
	
	mate_vfs_shutdown ();
	return 0;
}
