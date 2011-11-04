/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* test-parse-ls-lga.c - Test program for the MATE Virtual File System.

   Copyright (C) 2005 Christian Kellner <gicmo@gnome.org>

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

*/

#include <config.h>
#include <glib.h>
#include <libmatevfs/mate-vfs-parse-ls.h>
#include <stdlib.h>
#include <string.h>

/* From modules/smb-methdo.c */

static gboolean
string_compare (const char *a, const char *b)
{
	if (a != NULL && b != NULL) {
		return strcmp (a, b) == 0;
	} else {
		return a == b;
	}
}

typedef struct {

	char *line;
	int   ret;
	char *filename;
	char *linkname;

} Test;

static Test tests[] = {

	/*  Microsoft FTP Service */
	{"dr-xr-xr-x   1 owner    group               0 Dec  4  2004 bla", 			 1, "bla", NULL},
	/* vsFTPd 1.2.2 */
	{"-rw-r--r--    1 1113     0            1037 Aug 22  2001 welcome.msg",                  1, "welcome.msg", NULL},
	{"lrwxrwxrwx    1 0        0              19 Jul 30  2004 MATE -> ../mirror/gnome.org", 1, "MATE",       "../mirror/gnome.org"},
	/* vsFTPd 2.0.1 */
	{"-rw-r--r--    1 ftp      ftp      28664404 Feb 13 00:22 ls-lR.gz",			 1, "ls-lR.gz",	   NULL},
	{"drwxr-xr-x   2 ftp      ftp            48 Feb 13 12:47 2099",                          1, "2099",        NULL},
	{NULL, -1, NULL, NULL}
};


int
main (int argc, char **argv)
{
	Test *iter;
	gboolean one_test_failed;

	one_test_failed = FALSE;

	for (iter = tests; iter->line; iter++) {
		int ret;
		char *filename, *linkname;
		struct stat s;

		ret = mate_vfs_parse_ls_lga (iter->line, &s, &filename, &linkname);

		if (ret != iter->ret ||
		    ! string_compare (iter->filename, filename)	||
		    ! string_compare (iter->linkname, linkname)) {
			one_test_failed = TRUE;
			g_critical ("parsing %s FAILED\n", iter->line);
			g_print ("\t%s,%s\n\t%s,%s\n", filename, iter->filename, linkname, iter->linkname);
		}

	}

	return one_test_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}



