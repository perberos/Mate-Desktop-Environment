/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* test-seek.c - Test for the seek emulation functionality of the MATE Virtual
   File System library.

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

   Author: Michael Meeks <michael@imaginator.com> */

#include <config.h>

#include <errno.h>
#include <glib.h>
#include <libmatevfs/mate-vfs-init.h>
#include <libmatevfs/mate-vfs-ops.h>
#include <stdio.h>
#include <stdlib.h>

static void
show_result (MateVFSResult result, const gchar *what, const gchar *text_uri)
{
	fprintf (stderr, "%s `%s': %s\n",
		 what, text_uri, mate_vfs_result_to_string (result));
	if (result != MATE_VFS_OK)
		exit (1);
}

static gboolean
show_if_error (MateVFSResult result, const gchar *what)
{
	if (result != MATE_VFS_OK) {
		fprintf (stderr, "%s: `%s'\n",
			 what, mate_vfs_result_to_string (result));
		return TRUE;
	} else
		return FALSE;
}

static const char *
translate_vfs_seek_pos (MateVFSSeekPosition whence, int *unix_whence)
{
	const char *txt;
	int         ref_whence;

	switch (whence) {
	case MATE_VFS_SEEK_START:
		txt = "seek_start";
		ref_whence = SEEK_SET;
		break;
	case MATE_VFS_SEEK_CURRENT:
		txt = "seek_current";
		ref_whence = SEEK_CUR;
		break;
	case MATE_VFS_SEEK_END:
		txt = "seek_end";
		ref_whence = SEEK_END;
		break;
	default:
		txt = "unknown seek type";
		ref_whence = SEEK_SET;
		g_warning ("Unknown seek type");
	}
	if (unix_whence)
		*unix_whence = ref_whence;

	return txt;	
}

static gboolean
seek_test_chunk (MateVFSHandle      *handle,
		 FILE                *ref,
		 MateVFSFileOffset   vfs_offset,
		 MateVFSSeekPosition whence,
		 MateVFSFileSize     length)
{
	MateVFSResult result;
	int            ref_whence;
	
	translate_vfs_seek_pos (whence, &ref_whence);

	{ /* Preliminary tell */
		MateVFSFileSize offset = 0;
		long ref_off;
		result  = mate_vfs_tell (handle, &offset);
		if (show_if_error (result, "head mate_vfs_tell"))
			return FALSE;

		ref_off = ftell (ref);
		if (ref_off < 0) {
			g_warning ("Wierd ftell failure");
			return FALSE;
		}

		if (ref_off != offset) {
			g_warning ("Offset mismatch %d should be %d", (int)offset, (int)ref_off);
			return FALSE;
		}
	}

	{ /* seek */
		int fseekres;
		result   = mate_vfs_seek (handle, whence, vfs_offset);
		fseekres = fseek (ref, vfs_offset, ref_whence);

		if (fseekres == 0 &&
		    result != MATE_VFS_OK) {
			g_warning ("seek success difference '%d - %d' - '%s'",
				   fseekres, errno, mate_vfs_result_to_string (result));
			return FALSE;
		}
	}

	{ /* read - leaks like a sieve on error =] */
		guint8 *data, *data_ref;
		int     bytes_read_ref;
		MateVFSFileSize bytes_read;

		data     = g_new (guint8, length);
		data_ref = g_new (guint8, length);
		
		result = mate_vfs_read (handle, data, length, &bytes_read);
		bytes_read_ref = fread (data_ref, 1, length, ref);

		if (bytes_read_ref != bytes_read) {
			g_warning ("read failure: vfs read %d and fread %d bytes ('%s')",
				   (int)bytes_read, bytes_read_ref,
				   mate_vfs_result_to_string (result));
			return FALSE;
		}
		if (result != MATE_VFS_OK) {
			g_warning ("VFS read failed with '%s'",
				   mate_vfs_result_to_string (result));
			return FALSE;
		}
		
		{ /* Compare the data */
			int i;
			for (i = 0; i < bytes_read; i++)
				if (data[i] != data_ref[i]) {
					g_warning ("vfs read data mismatch at byte %d, '%d' != '%d'",
						   i, data[i], data_ref[i]);
					return FALSE;
				}
		}

		g_free (data_ref);
		g_free (data);
	}
	
	{ /* Tail tell */
		MateVFSFileSize offset;
		long ref_off;
		result  = mate_vfs_tell (handle, &offset);
		if (show_if_error (result, "tail mate_vfs_tell"))
			return FALSE;

		ref_off = ftell (ref);
		if (ref_off < 0) {
			g_warning ("Wierd ftell failure");
			return FALSE;
		}

		if (ref_off != offset) {
			g_warning ("Offset mismatch %d should be %d", (int)offset, (int)ref_off);
			return FALSE;
		}
	}

	return TRUE;
}

int
main (int argc, char **argv)
{
	MateVFSResult result;
	MateVFSHandle *handle;
	FILE *ref;
	int i, failures;

	if (! mate_vfs_init ()) {
		fprintf (stderr, "Cannot initialize mate-vfs.\n");
		return 1;
	}

	if (argc != 3) {
		fprintf (stderr, "This is a program to test seek emulation on linear filesystems\n");
		fprintf (stderr, "Usage: %s <source file uri> <seekable local reference fname>\n",
			 argv[0]);
		return 1;
	}

	result = mate_vfs_open (&handle, argv[1], MATE_VFS_OPEN_READ|MATE_VFS_OPEN_RANDOM);
	show_result (result, "mate_vfs_open", argv[1]);

	if (!(ref = fopen (argv[2], "r"))) {
		fprintf (stderr, "Failed to open '%s' to compare seek history\n", argv[2]);
		exit (1);
	}

	failures = 0;
	for (i = 0; i < 10; i++) {
		MateVFSFileSize     length  = (1000.0 * rand () / (RAND_MAX + 1.0));
		MateVFSFileOffset   seekpos = (1000.0 * rand () / (RAND_MAX + 1.0));
		MateVFSSeekPosition w = (int)(2.0 * rand () / (RAND_MAX + 1.0));

		if (!seek_test_chunk (handle, ref, seekpos, w, length)) {
			printf ("Failed: seek (offset %d, whence '%s'), read (length %d), tell = %ld\n",
				(int)seekpos, translate_vfs_seek_pos (w, NULL),
				(int)length, ftell (ref));
			failures++;
		}
	}
	if (failures)
		printf ("%d tests failed\n", failures);
	else
		printf ("All test successful\n");

	result = mate_vfs_close (handle);
	show_result (result, "mate_vfs_close", argv[1]);
	fclose (ref);
	
	return 0;
}
