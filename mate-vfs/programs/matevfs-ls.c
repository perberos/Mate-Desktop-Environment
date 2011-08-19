/* matevfs-ls.c - Test for open_dir(), read_dir() and close_dir() for mate-vfs

   Copyright (C) 2003, 2005, Red Hat

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <locale.h>
#include <libmatevfs/mate-vfs.h>

#include "authentication.c"

static gboolean timing = FALSE;
static gboolean quiet = FALSE;
static gboolean access_rights = FALSE;
static gboolean selinux_context = FALSE;

static GOptionEntry entries[] = 
{
	{ "time", 't', 0, G_OPTION_ARG_NONE, &timing, "Time the directory listening operation", NULL },
	{ "quiet", 'q', 0, G_OPTION_ARG_NONE, &quiet, "Do not output the stat information (useful in conjunction with the --time)", NULL},
	{ "access-rights", 'a', 0, G_OPTION_ARG_NONE, &access_rights, "Get access rights", NULL},

#ifdef HAVE_SELINUX
	{ "selinux-context", 'Z', 0, G_OPTION_ARG_NONE, &selinux_context, "Get selinux context", NULL},
#endif

	{ NULL }
};

static void show_data (gpointer item, const char *directory);
static void list (const char *directory);

static const gchar *
type_to_string (MateVFSFileInfo *info)
{
	if (!(info->valid_fields & MATE_VFS_FILE_INFO_FIELDS_TYPE)) {
		return "Unknown";
	}

	switch (info->type) {
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
show_data (gpointer item, const char *directory)
{
	MateVFSFileInfo *info;
	char *path;

	info = (MateVFSFileInfo *) item;

	path = g_strconcat (directory, "/", info->name, NULL);

	g_print ("%s\t%s%s%s\t(%s, %s)\tsize %" MATE_VFS_SIZE_FORMAT_STR "",
			info->name,
			MATE_VFS_FILE_INFO_SYMLINK (info) ? " [link: " : "",
			MATE_VFS_FILE_INFO_SYMLINK (info) ? info->symlink_name
			: "",
			MATE_VFS_FILE_INFO_SYMLINK (info) ? " ]" : "",
			type_to_string (info),
			info->mime_type,
			info->size);

	if (info->valid_fields & MATE_VFS_FILE_INFO_FIELDS_PERMISSIONS) {
			g_print ("\tmode %04o", info->permissions);
	}

	if (info->valid_fields & MATE_VFS_FILE_INFO_FIELDS_ACCESS) {
		g_print ("\t");
		
		if (info->permissions & MATE_VFS_PERM_ACCESS_READABLE) {
			g_print ("r");
		}
		
		if (info->permissions & MATE_VFS_PERM_ACCESS_WRITABLE) {
			g_print ("w");
		}
		
		if (info->permissions & MATE_VFS_PERM_ACCESS_EXECUTABLE) {
			g_print ("x");
		}
	}

	if (info->valid_fields & MATE_VFS_FILE_INFO_FIELDS_SELINUX_CONTEXT)
		g_print ("\tcontext %s", info->selinux_context);

	g_print ("\n");
	
	g_free (path);
}

void
list (const char *directory)
{
	MateVFSResult result;
	MateVFSFileInfo *info;
	MateVFSDirectoryHandle *handle;
	MateVFSFileInfoOptions options;
	GTimer *timer;
	
	timer = NULL;
	if (timing) {
		timer = g_timer_new ();
		g_timer_start (timer);	
	}
	
	options = MATE_VFS_FILE_INFO_GET_MIME_TYPE
			| MATE_VFS_FILE_INFO_FOLLOW_LINKS;

	if (access_rights) {
		options |= MATE_VFS_FILE_INFO_GET_ACCESS_RIGHTS;
	}

	if (selinux_context)
		options |= MATE_VFS_FILE_INFO_GET_SELINUX_CONTEXT;
	
	result = mate_vfs_directory_open (&handle, directory, options);

	if (result != MATE_VFS_OK) {
		g_print("Error opening: %s\n", mate_vfs_result_to_string
				(result));
		return;
	}

	info = mate_vfs_file_info_new ();
	while ((result = mate_vfs_directory_read_next (handle, info)) == MATE_VFS_OK) {
		if (!quiet) {
			show_data ((gpointer) info, directory);
		}
	}
	
	mate_vfs_file_info_unref (info);

	if ((result != MATE_VFS_OK) && (result != MATE_VFS_ERROR_EOF)) {
		g_print ("Error: %s\n", mate_vfs_result_to_string (result));
		return;
	}

	result = mate_vfs_directory_close (handle);

	if ((result != MATE_VFS_OK) && (result != MATE_VFS_ERROR_EOF)) {
		g_print ("Error closing: %s\n", mate_vfs_result_to_string (result));
		return;
	}

	if (timing) {
		gdouble duration;
		gulong  msecs;
		
		g_timer_stop (timer);
		
		duration = g_timer_elapsed (timer, &msecs);
		g_timer_destroy (timer);
		g_print ("Total time: %.16e\n", duration);
	}

	
}

int
main (int argc, char *argv[])
{
  	GError *error;
	GOptionContext *context;

  	setlocale (LC_ALL, "");

	mate_vfs_init ();

	command_line_authentication_init ();

	error = NULL;
	context = g_option_context_new ("- list files at <uri>");
  	g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
  	g_option_context_parse (context, &argc, &argv, &error);

	if (argc > 1) {
		int i;

		for (i = 1; i < argc; i++) {
			char *directory;
			directory = mate_vfs_make_uri_from_shell_arg (argv[i]);
			list (directory);
			g_free (directory);
		}
	} else {
		char *tmp, *directory;

		tmp = g_get_current_dir ();
		directory = mate_vfs_get_uri_from_local_path (tmp);
		list (directory);
		g_free (tmp);
		g_free (directory);
	}

	mate_vfs_shutdown ();
	return 0;
}
