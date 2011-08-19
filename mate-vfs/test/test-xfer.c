/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* test-xfer.c - Test program for the xfer functions in the MATE Virtual File
   System.

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
#include <libmatevfs/mate-vfs-init.h>
#include <libmatevfs/mate-vfs-xfer.h>
#include <stdio.h>
#include <stdlib.h>

static gboolean remove_source = FALSE;
static gboolean recursive = FALSE;
static gboolean follow_symlinks = FALSE;
static gboolean replace = FALSE;
static gboolean follow_symlinks_recursive = FALSE;

static GOptionEntry options[] = {
	{ "delete-source", 'd', G_OPTION_FLAG_IN_MAIN,
	  G_OPTION_ARG_NONE, &remove_source, "Delete source files", NULL },
	{ "recursive", 'r', G_OPTION_FLAG_IN_MAIN,
	  G_OPTION_ARG_NONE, &recursive, "Copy directories recursively", NULL },
	{ "follow-symlinks", 'L', G_OPTION_FLAG_IN_MAIN,
	  G_OPTION_ARG_NONE, &follow_symlinks, "Follow symlinks", NULL },
	{ "replace", 'R', G_OPTION_FLAG_IN_MAIN,
	  G_OPTION_ARG_NONE, &replace, "Replace files automatically", NULL },
	{ "recursive-symlinks", 'Z', G_OPTION_FLAG_IN_MAIN,
	  G_OPTION_ARG_NONE, &follow_symlinks_recursive, "Follow symlinks", NULL },
	{ NULL }
};

static void
show_result (MateVFSResult result, const gchar *what)
{
	fprintf (stderr, "%s: %s\n", what, mate_vfs_result_to_string (result));
	if (result != MATE_VFS_OK)
		exit (1);
}

static gint
xfer_progress_callback (MateVFSXferProgressInfo *info,
			gpointer data)
{
	switch (info->status) {
	case MATE_VFS_XFER_PROGRESS_STATUS_VFSERROR:
		printf ("VFS Error: %s\n",
			mate_vfs_result_to_string (info->vfs_status));
		exit (1);
		break;
	case MATE_VFS_XFER_PROGRESS_STATUS_OVERWRITE:
		printf ("Overwriting `%s' with `%s'\n",
			info->target_name, info->source_name);
		exit (1);
		break;
	case MATE_VFS_XFER_PROGRESS_STATUS_OK:
		printf ("Status: OK\n");
		switch (info->phase) {
		case MATE_VFS_XFER_PHASE_INITIAL:
			printf ("Initial phase\n");
			return TRUE;
		case MATE_VFS_XFER_PHASE_COLLECTING:
			printf ("Collecting file list\n");
			return TRUE;
		case MATE_VFS_XFER_PHASE_READYTOGO:
			printf ("Ready to go!\n");
			return TRUE;
		case MATE_VFS_XFER_PHASE_OPENSOURCE:
			printf ("Opening source\n");
			return TRUE;
		case MATE_VFS_XFER_PHASE_OPENTARGET:
			printf ("Opening target\n");
			return TRUE;
		case MATE_VFS_XFER_PHASE_COPYING:
			printf ("Transferring `%s' to `%s' (file %ld/%ld, byte %ld/%ld in file, "
				"%" MATE_VFS_SIZE_FORMAT_STR "/%" MATE_VFS_SIZE_FORMAT_STR " total)\n",
				info->source_name,
				info->target_name,
				info->file_index,
				info->files_total,
				(glong) info->bytes_copied,
				(glong) info->file_size,
				info->total_bytes_copied,
				info->bytes_total);
			return TRUE;
		case MATE_VFS_XFER_PHASE_CLOSESOURCE:
			printf ("Closing source\n");
			return TRUE;
		case MATE_VFS_XFER_PHASE_CLOSETARGET:
			printf ("Closing target\n");
			return TRUE;
		case MATE_VFS_XFER_PHASE_FILECOMPLETED:
			printf ("Done with `%s' -> `%s', going next\n",
				info->source_name, info->target_name);
			return TRUE;
		case MATE_VFS_XFER_PHASE_COMPLETED:
			printf ("All done.\n");
			return TRUE;
		default:
			printf ("Unexpected phase %d\n", info->phase);
			return TRUE; /* keep going anyway */
		}
	case MATE_VFS_XFER_PROGRESS_STATUS_DUPLICATE:
		break;
	}

	printf ("Boh!\n");
	return FALSE;
}

int
main (int argc, char **argv)
{
	MateVFSURI *src_uri, *dest_uri;
	GList *src_uri_list, *dest_uri_list;
	MateVFSResult result;
	MateVFSXferOptions xfer_options;
	MateVFSXferOverwriteMode overwrite_mode;

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

	if (! mate_vfs_init ()) {
		fprintf (stderr,
			 "Cannot initialize the MATE Virtual File System.\n");
		return 1;
	}

	if (argc != 3 || argv[1] == NULL || argv[2] != NULL) {
		g_printerr("Usage: %s [<options>] <src> <target>\n", argv[0]);
		return 1;
	}

	src_uri = mate_vfs_uri_new(argv[1]);
	if (src_uri == NULL) {
		fprintf (stderr, "%s: invalid URI\n", argv[0]);
		return 1;
	}

	dest_uri = mate_vfs_uri_new(argv[2]);
	if (dest_uri == NULL) {
		fprintf (stderr, "%s: invalid URI\n", argv[1]);
		return 1;
	}

	xfer_options = 0;
	overwrite_mode = MATE_VFS_XFER_OVERWRITE_MODE_QUERY;
	if (recursive) {
		fprintf (stderr, "Warning: Recursive xfer of directories.\n");
		xfer_options |= MATE_VFS_XFER_RECURSIVE;
	}
	if (follow_symlinks) {
		fprintf (stderr, "Warning: Following symlinks.\n");
		xfer_options |= MATE_VFS_XFER_FOLLOW_LINKS;
	}
	if (follow_symlinks_recursive) {
		fprintf (stderr, "Warning: Following symlinks recursively.\n");
		xfer_options |= MATE_VFS_XFER_FOLLOW_LINKS_RECURSIVE;
	}
	if (replace) {
		fprintf (stderr, "Warning: Using replace overwrite mode.\n");
		overwrite_mode = MATE_VFS_XFER_OVERWRITE_MODE_REPLACE;
	}
	if (remove_source) {
		fprintf (stderr, "Warning: Removing source files.\n");
		xfer_options |= MATE_VFS_XFER_REMOVESOURCE;
	}

	src_uri_list = g_list_append (NULL, src_uri);
	dest_uri_list = g_list_append (NULL, dest_uri);
	result = mate_vfs_xfer_uri_list (src_uri_list, dest_uri_list,
					  xfer_options,
					  MATE_VFS_XFER_ERROR_MODE_QUERY,
					  overwrite_mode,
					  xfer_progress_callback,
					  NULL);

	show_result (result, "mate_vfs_xfer");

	mate_vfs_uri_list_free (src_uri_list);
	mate_vfs_uri_list_free (dest_uri_list);

	return 0;
}
