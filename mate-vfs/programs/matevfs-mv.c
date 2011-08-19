/* matevfs-rm.c - Test for unlink() for mate-vfs

   Copyright (C) 1999 Free Software Foundation
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

   Author: Ettore Perazzoli <ettore@gnu.org>
           Bastien Nocera <hadess@hadess.net>
*/

#include <libmatevfs/mate-vfs.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "authentication.c"

static void
show_result (MateVFSResult result, const gchar *what, const gchar *from, const gchar *to)
{
	if (result != MATE_VFS_OK) {
		fprintf (stderr, "%s `%s' `%s': %s\n",
				what, from, to,
				mate_vfs_result_to_string (result));
		exit (1);
	}
}

int
main (int argc, char **argv)
{
	MateVFSResult    result;
	char  	         *from, *to;

	if (argc != 3) {
		fprintf (stderr, "Usage: %s <from> <to>\n", argv[0]);
		return 1;
	}

	if (! mate_vfs_init ()) {
		fprintf (stderr, "Cannot initialize mate-vfs.\n");
		return 1;
	}

	command_line_authentication_init ();

	from = mate_vfs_make_uri_from_shell_arg (argv[1]);
	
	if (from == NULL) {
		fprintf (stderr, "Could not guess URI from %s\n", argv[1]);
		return 1;
	}

	to = mate_vfs_make_uri_from_shell_arg (argv[2]);

	if (to == NULL) {
		g_free (from);
		fprintf (stderr, "Could not guess URI from %s\n", argv[2]);
		return 1;
	}

	result = mate_vfs_move (from, to, TRUE);
	show_result (result, "move", from, to);

	g_free (from);
	g_free (to);

	return 0;
}

