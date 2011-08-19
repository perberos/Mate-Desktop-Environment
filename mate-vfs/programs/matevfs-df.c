/* matevfs-df.c - Test for mate_vfs_get_volume_free_space() for mate-vfs

   Copyright (C) 1999 Free Software Foundation
   Copyright (C) 2003, 2006, Red Hat

   Example use: matevfs-df /mnt/nas

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
#include <string.h>

#include "authentication.c"

static void
show_free_space (MateVFSVolume *vol,
		 const char *text_uri)
{
	MateVFSURI *uri;
	MateVFSFileSize free_space;
	MateVFSResult result;
	char *size, *local;

	uri = mate_vfs_uri_new (text_uri);
	if (uri == NULL) {
		fprintf (stderr, "Could not make URI from %s\n", text_uri);
		mate_vfs_uri_unref (uri);
		return;
	}

	local = mate_vfs_volume_get_display_name (vol);
	result = mate_vfs_get_volume_free_space (uri, &free_space);
	mate_vfs_uri_unref (uri);
	if (result != MATE_VFS_OK) {
		fprintf (stderr, "Error getting free space on %s: %s\n",
				local, mate_vfs_result_to_string (result));
		g_free (local);
		return;
	}

	local = g_filename_from_uri (text_uri, NULL, NULL);
	size = mate_vfs_format_file_size_for_display (free_space);
	printf ("%-19s %s (%"MATE_VFS_SIZE_FORMAT_STR")\n", local ? local : text_uri, size, free_space);
	g_free (size);
	g_free (local);
}

static char *ignored_fs[] = {
	"binfmt_misc",
	"rpc_pipefs",
	"usbfs",
	"devpts",
	"sysfs",
	"proc",
	"rootfs",
	"nfsd"
};

static gboolean
ignore_volume (MateVFSVolume *vol)
{
	char *type;
	int i;

	type = mate_vfs_volume_get_filesystem_type (vol);

	if (type == NULL) {
		return TRUE;
	}

	for (i = 0; i < G_N_ELEMENTS (ignored_fs); i++) {
		if (strcmp (ignored_fs[i], type) == 0) {
			g_free (type);
			return TRUE;
		}
	}
	return FALSE;
}

int
main (int argc, char **argv)
{
	if (argc != 2 && argc != 1) {
		fprintf (stderr, "Usage: %s [uri]\n", argv[0]);
		return 1;
	}

	if (! mate_vfs_init ()) {
		fprintf (stderr, "Cannot initialize mate-vfs.\n");
		return 1;
	}

	command_line_authentication_init ();

	if (argc == 2) {
		char *text_uri;

		text_uri = mate_vfs_make_uri_from_shell_arg (argv[1]);
		show_free_space (NULL, text_uri);
		g_free (text_uri);
	} else {
		GList *list, *l;

		list = mate_vfs_volume_monitor_get_mounted_volumes
			(mate_vfs_get_volume_monitor ());

		printf ("Volume          Free space\n");
		for (l = list; l != NULL; l = l->next) {
			MateVFSVolume *vol = l->data;
			char *act;

			if (ignore_volume (vol)) {
				mate_vfs_volume_unref (vol);
				continue;
			}

			act = mate_vfs_volume_get_activation_uri (vol);
			show_free_space (vol, act);
			mate_vfs_volume_unref (vol);
		}
		g_list_free (list);

		list = mate_vfs_volume_monitor_get_connected_drives
			(mate_vfs_get_volume_monitor ());

		printf ("Drives:\n");
		for (l = list; l != NULL; l = l->next) {
			MateVFSDrive *drive = l->data;
			gchar *uri = mate_vfs_drive_get_activation_uri (drive);
			gchar *device = mate_vfs_drive_get_device_path (drive);
			gchar *name = mate_vfs_drive_get_display_name (drive);

			printf ("\t%s\t%s\t%s\n", uri ? uri : "\t", device ? device : "\t", name);
			g_free (uri);
			g_free (device);
			g_free (name);
			mate_vfs_drive_unref (drive);
		}
		g_list_free (list);
	}

	mate_vfs_shutdown();

	return 0;
}

