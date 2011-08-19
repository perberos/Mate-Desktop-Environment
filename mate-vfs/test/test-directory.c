/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* test-directory.c - Test program for directory reading in the MATE
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

   Author: Ettore Perazzoli <ettore@comm2000.it> */

#include <config.h>

#include <glib.h>
#include <libmatevfs/mate-vfs-directory.h>
#include <libmatevfs/mate-vfs-init.h>
#include <stdio.h>
#include <stdlib.h>

static gboolean measure_speed = 0;

static GOptionEntry options[] = {
	{ "measure-speed", 'm', G_OPTION_FLAG_IN_MAIN,
	  G_OPTION_ARG_NONE, &measure_speed, "Measure speed without displaying anything", NULL },
	{ NULL }
};

static void
show_result (MateVFSResult result, const gchar *what, const gchar *text_uri)
{
	fprintf (stderr, "%s `%s': %s\n",
		 what, text_uri, mate_vfs_result_to_string (result));
	if (result != MATE_VFS_OK)
	{
		fprintf (stdout, "Error: %s\n",
				mate_vfs_result_to_string (result));
		exit (1);
	}
}

static const gchar *
type_to_string (MateVFSFileType type)
{
	switch (type) {
	case MATE_VFS_FILE_TYPE_UNKNOWN:
		return "Unknown";
	case MATE_VFS_FILE_TYPE_REGULAR:
		return "Regular";
	case MATE_VFS_FILE_TYPE_DIRECTORY:
		return "Directory";
	case MATE_VFS_FILE_TYPE_SYMBOLIC_LINK:
		return "Symbolic Link";
	case MATE_VFS_FILE_TYPE_FIFO:
		return "FIFO";
	case MATE_VFS_FILE_TYPE_SOCKET:
		return "Socket";
	case MATE_VFS_FILE_TYPE_CHARACTER_DEVICE:
		return "Character device";
	case MATE_VFS_FILE_TYPE_BLOCK_DEVICE:
		return "Block device";
	default:
		return "???";
	}
}

static void
print_list (GList *list)
{
	MateVFSFileInfo *info;
	GList *node;

	if (list == NULL) {
		printf ("  (No files)\n");
		return;
	}

	for (node = list; node != NULL; node = node->next) {
		const gchar *mime_type;
	
		info = node->data;

		mime_type = mate_vfs_file_info_get_mime_type (info);
		if (mime_type == NULL)
			mime_type = "(Unknown)";

		printf ("  File `%s'%s%s%s (%s, %s), size %ld, mode %04o\n",
			info->name,
			MATE_VFS_FILE_INFO_SYMLINK (info) ? " [link: " : "",
			MATE_VFS_FILE_INFO_SYMLINK (info) ? info->symlink_name : "",
			MATE_VFS_FILE_INFO_SYMLINK (info) ? " ]" : "",
			type_to_string (info->type),
			mime_type,
			(glong) info->size,
			info->permissions);
	}
}

int
main (int argc, char **argv)
{
	GList *list;
	MateVFSResult result;
	GTimer *timer;
	gchar *text_uri;

	GOptionContext *ctx = NULL;
	GError *error = NULL;

	ctx = g_option_context_new("test-directory");
	g_option_context_add_main_entries(ctx, options, NULL);

	if (!g_option_context_parse(ctx, &argc, &argv, &error)) {
		g_printerr("main: %s\n", error->message);

		g_error_free(error);
		g_option_context_free(ctx);
		return 1;
	}

	g_option_context_free(ctx);

	if (argc != 2 || argv[1] == NULL) {
		g_printerr("Usage: %s [<options>] <uri>\n", argv[0]);
		return 1;
	}

	text_uri = g_strdup(argv[1]);

	mate_vfs_init ();

	printf ("Loading directory...");
	fflush (stdout);

	if (measure_speed) {
		timer = g_timer_new ();
		g_timer_start (timer);
	} else {
		timer = NULL;
	}

	/* Load with without requesting any metadata.  */
	result = mate_vfs_directory_list_load
		(&list, text_uri,
		 (MATE_VFS_FILE_INFO_GET_MIME_TYPE
		  | MATE_VFS_FILE_INFO_FORCE_FAST_MIME_TYPE
		  | MATE_VFS_FILE_INFO_FOLLOW_LINKS));

	if (result == MATE_VFS_OK && measure_speed) {
		gdouble elapsed_seconds;
		guint num_entries;

		g_timer_stop (timer);
		elapsed_seconds = g_timer_elapsed (timer, NULL);
		num_entries = g_list_length (list);
		printf ("\n%.5f seconds for %d unsorted entries, %.5f entries/sec.\n",
			elapsed_seconds, num_entries,
			(double) num_entries / elapsed_seconds);
	}

	if (!measure_speed) {
		printf ("Ok\n");

		show_result (result, "load_directory", text_uri);

		printf ("Listing for `%s':\n", text_uri);
		print_list (list);
	}

	printf ("Destroying.\n");
	mate_vfs_file_info_list_free (list);

	printf ("Done.\n");

	g_free (text_uri);

	return 0;
}
