/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* test-symlinks.c: verifies that symlinks are being created properly
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

   Author: Seth Nickell <snickell@stanford.edu> */

#include <config.h>

#include <glib.h>
#include <libmatevfs/mate-vfs-async-ops.h>
#include <libmatevfs/mate-vfs-init.h>
#include <libmatevfs/mate-vfs-ops.h>
#include <stdio.h>
#include <string.h>

static GMainLoop *main_loop;

typedef struct {
	MateVFSResult expected_result;
	const char *uri;
	const char *target_uri;
	const char *target_reference;
} CallbackData;

static int
deal_with_result (MateVFSResult result, MateVFSResult expected_result, 
	const char *uri, const char *target_uri, const char *target_reference, 
	gboolean unlink) 
{
	char read_buffer[1024];
	const char *write_buffer = "this is test data...we should read the same thing";
	MateVFSHandle *handle;
	MateVFSFileSize bytes_written, temp;
	MateVFSFileInfo info_uri = {NULL};
	MateVFSFileInfo info_target = {NULL};
	int return_value = 1;
	const gchar *result_string;
	MateVFSResult error;	
	MateVFSURI *real_uri, *real_uri_target;
	MateVFSFileInfo *info;

	real_uri = mate_vfs_uri_new (uri);
	real_uri_target = mate_vfs_uri_new (target_uri);

	if (result != expected_result) {
		result_string = mate_vfs_result_to_string (result);
		printf ("creating a link from %s to %s returned %s instead of %s.\n", uri, target_reference,
			result_string, mate_vfs_result_to_string (expected_result));
		return_value = 0;
	} else if (result == MATE_VFS_OK) { 
		info = mate_vfs_file_info_new();
		error = mate_vfs_get_file_info_uri (real_uri, info, MATE_VFS_FILE_INFO_DEFAULT);
		if ((error != MATE_VFS_OK) || (info->type != MATE_VFS_FILE_TYPE_SYMBOLIC_LINK)) {
			printf ("Symlink problem: mate_vfs_file_info returns wrong for link %s\n", uri);
		} else {
			/* our link seems to have been created correctly - lets see if its real */
			error = mate_vfs_open_uri (&handle, real_uri_target, MATE_VFS_OPEN_WRITE);
			if (error == MATE_VFS_ERROR_NOT_FOUND) 
				error = mate_vfs_create_uri (&handle, real_uri_target, MATE_VFS_OPEN_WRITE, 0, MATE_VFS_PERM_USER_ALL);
			if (error == MATE_VFS_OK) {
				/* write stuff to our link location */
				error = mate_vfs_write (handle, write_buffer, strlen (write_buffer) + 1, &bytes_written);
				error = mate_vfs_close (handle);
				error = mate_vfs_open_uri (&handle, real_uri, MATE_VFS_OPEN_READ);
				if (error == MATE_VFS_OK) {
					error = mate_vfs_read (handle, read_buffer, bytes_written, &temp);
					read_buffer[temp] = 0;
					error = mate_vfs_close (handle);
					if (strcmp (read_buffer, write_buffer) != 0) {
						printf ("Symlink problem: value written is not the same as the value read!\n");
						printf ("Written to %s: #%s# \nRead from link %s: #%s#\n", 
							target_uri, write_buffer, uri, read_buffer);
						return_value = 0;
					}
				}
			}
			mate_vfs_get_file_info_uri (real_uri, &info_uri, MATE_VFS_FILE_INFO_FOLLOW_LINKS);
			mate_vfs_get_file_info_uri (real_uri_target, &info_target, MATE_VFS_FILE_INFO_FOLLOW_LINKS);
			if (info_uri.inode != info_target.inode) {
				printf ("Symlink problem: link following is not working\n");
				printf ("File: %s   Link: %s\n", target_uri, uri);
			}
			mate_vfs_file_info_clear (&info_uri);
			mate_vfs_get_file_info_uri (real_uri, &info_uri, MATE_VFS_FILE_INFO_DEFAULT);
			mate_vfs_file_info_clear (&info_target);
			mate_vfs_get_file_info_uri (real_uri_target, &info_target, MATE_VFS_FILE_INFO_DEFAULT);
			if (info_uri.inode == info_target.inode) {
				printf ("Symlink problem: link following is happening when it shouldn't be.\n");
				printf ("File: %s   Link: %s\n", target_uri, uri);
			}
			mate_vfs_file_info_clear (&info_uri);
			mate_vfs_file_info_clear (&info_target);
		}
		mate_vfs_file_info_unref (info);
		if (unlink) {
			mate_vfs_unlink_from_uri (real_uri_target);
			error = mate_vfs_unlink_from_uri (real_uri);
			if (error != MATE_VFS_OK) {
				printf ("Problem unlinking URI %s", uri);
			}
		}
	}


	mate_vfs_uri_unref (real_uri);
	mate_vfs_uri_unref (real_uri_target);

	return return_value;
}

static void
create_link_callback (MateVFSAsyncHandle *handle,
		      MateVFSResult result,
		      gpointer callback_data)
{
	const char *uri, *target_uri, *target_reference;
	MateVFSResult expected_result;
	CallbackData *info;

	info = (CallbackData*) callback_data;
	
	uri = info->uri;
	target_uri = info->target_uri;
	expected_result = info->expected_result;
	target_reference = info->target_reference;

	deal_with_result (result, expected_result, uri, target_uri, target_reference, TRUE);	

	g_free (callback_data);
	g_main_loop_quit (main_loop);
}


static int
make_link (const char *uri, const char *target_reference, const char *target_uri, MateVFSResult expected_result, gboolean unlink)
{
	MateVFSURI *real_uri, *real_uri_target;
	MateVFSResult result;

	int return_value = 1;

	real_uri = mate_vfs_uri_new (uri);
	real_uri_target = mate_vfs_uri_new (target_uri);

	result = mate_vfs_create_symbolic_link (real_uri, target_reference);

	return_value = deal_with_result(result, expected_result, uri, target_uri, target_reference, unlink);


	
	mate_vfs_uri_unref (real_uri);
	mate_vfs_uri_unref (real_uri_target);

	return return_value;
}

static void
make_link_async (const char *uri, const char *target_reference, const char *target_uri, MateVFSResult expected_result)
{
	CallbackData *info;
	MateVFSAsyncHandle *handle;

	info = g_malloc (sizeof (CallbackData));
	info->uri = uri;
	info->target_uri = target_uri;
	info->expected_result = expected_result;
	info->target_reference = target_reference;

	mate_vfs_async_create_symbolic_link (&handle, mate_vfs_uri_new(uri), target_reference, 0, create_link_callback, info);
}

static void
check_broken_links (const char *uri)
{
	MateVFSHandle *handle;
	MateVFSResult error;
	MateVFSURI *real_uri, *real_uri_target;

	real_uri = mate_vfs_uri_new (uri);
	real_uri_target = mate_vfs_uri_new ("file:///tmp/deadlink");

	mate_vfs_unlink_from_uri (real_uri_target);
	mate_vfs_create_symbolic_link (real_uri, "deadlink");

	error = mate_vfs_open_uri (&handle, real_uri, MATE_VFS_OPEN_READ);
	if (error != MATE_VFS_ERROR_NOT_FOUND) {
		printf ("MATE_VFS_BROKEN_SYMLINK not returned open attempting to open a broken symlink.\n");
		printf ("Value returned: %d\n", error);
	}

	mate_vfs_unlink_from_uri (real_uri);
	mate_vfs_unlink_from_uri (real_uri_target);

	mate_vfs_uri_unref (real_uri);
	mate_vfs_uri_unref (real_uri_target);
}


int
main (int argc, const char **argv)
{
	MateVFSURI *directory, *file_to_delete;

	if (argc != 2) {
		fprintf (stderr, "Usage: %s <directory>\n", argv[0]);
		return 1;
	}

	mate_vfs_init ();
	directory = mate_vfs_uri_new ("file:///tmp/tmp");

	mate_vfs_make_directory_for_uri (directory, MATE_VFS_PERM_USER_ALL);

	make_link ("file:///tmp/link_to_ditz", "file:///tmp/ditz", "file:///tmp/ditz", MATE_VFS_OK, TRUE);
	make_link ("file:///tmp/link_to_ditz_relative", "ditz", "file:///tmp/ditz", MATE_VFS_OK, TRUE);
	make_link ("file:///tmp/tmp/link_to_ditz", "../ditz", "file:///tmp/ditz", MATE_VFS_OK, FALSE);
	make_link ("file:///tmp/link_to_link", "tmp/link_to_ditz", "file:///tmp/tmp/link_to_ditz", MATE_VFS_OK, TRUE);
				
	mate_vfs_remove_directory_from_uri (directory);
	mate_vfs_uri_unref (directory);

	file_to_delete = mate_vfs_uri_new ("file:///tmp/ditz");
	mate_vfs_unlink_from_uri (file_to_delete);
	mate_vfs_uri_unref (file_to_delete);

	check_broken_links("file:///tmp/link");

	make_link ("file:///tmp/link_to_ditz_offfs", "http://www.a.com/ditz", "http://www.a.com/ditz", MATE_VFS_ERROR_NOT_SUPPORTED, TRUE);
	make_link ("http://www.eazel.com/link_to_ditz", "file:///tmp/ditz", "file:///tmp/ditz", MATE_VFS_ERROR_NOT_SUPPORTED, TRUE);
	make_link ("http://www.a.com/link_to_ditz_relative", "ditz", "http://www.a.com/ditz", MATE_VFS_ERROR_NOT_SUPPORTED, TRUE);

	make_link_async ("file:///tmp/async_link", "file:///tmp/link", "file:///tmp/link", MATE_VFS_OK);

	main_loop = g_main_loop_new (NULL, TRUE);
	g_main_loop_run (main_loop);
	g_main_loop_unref (main_loop);

	return 0;
}
