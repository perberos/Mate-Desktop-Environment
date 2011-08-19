/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * mate-display-properties-install-systemwide - Install a RANDR profile for the whole system
 *
 * Copyright (C) 2010 Novell, Inc.
 *
 * Authors: Federico Mena Quintero <federico@novell.com>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <glib/gi18n.h>
#include <glib.h>

#define SYSTEM_RANDR_PATH "/etc/mate-settings-daemon/xrandr"

static void
usage (const char *program_name)
{
	g_print (_("Usage: %s SOURCE_FILE DEST_NAME\n"
		   "\n"
		   "This program installs a RANDR profile for multi-monitor setups into\n"
		   "a systemwide location.  The resulting profile will get used when\n"
		   "the RANDR plug-in gets run in mate-settings-daemon.\n"
		   "\n"
		   "SOURCE_FILE - a full pathname, typically /home/username/.config/monitors.xml\n"
		   "\n"
		   "DEST_NAME - relative name for the installed file.  This will get put in\n"
		   "            the systemwide directory for RANDR configurations,\n"
		   "            so the result will typically be %s/DEST_NAME\n"),
		 program_name,
		 SYSTEM_RANDR_PATH);
}

static gboolean
is_basename (const char *filename)
{
	if (*filename == '\0')
		return FALSE; /* no empty strings, please */

	for (; *filename; filename++)
		if (G_IS_DIR_SEPARATOR (*filename))
			return FALSE;

	return TRUE;
}

static gboolean
copy_file (int source_fd, int dest_fd)
{
	char buf[1024];
	int num_read;
	int num_written;

	while (TRUE) {
		char *p;

		num_read = read (source_fd, buf, sizeof (buf));
		if (num_read == 0)
			break;

		if (num_read == -1) {
			if (errno == EINTR)
				continue;
			else
				return FALSE;
		}

		p = buf;
		while (num_read > 0) {
			num_written = write (dest_fd, p, num_read);
			if (num_written == -1) {
				if (errno == EINTR)
					continue;
				else
					return FALSE;
			}

			num_read -= num_written;
			p += num_written;
		}
	}

	return TRUE;
}

/* This is essentially a "please copy a file to a privileged location" program.
 * We try to be paranoid in the following ways:
 *
 * - We copy only regular files, owned by the user who called pkexec(1), to
 *   avoid attacks like "copy a file that I'm not allowed to read into a
 *   world-readable location".
 *
 * - We copy only to a well-known directory.
 *
 * - We try to avoid race conditions.  We only fstat() files that we have open
 *   to avoid files moving under our feet.  We only create files in directories
 *   that we have open.
 *
 * - We replace the destination file atomically.
 */

int
main (int argc, char **argv)
{
	uid_t uid, euid;
	const char *source_filename;
	const char *dest_name;
	const char *pkexec_uid_str;
	int pkexec_uid;
	struct stat statbuf;
	int err;
	int source_fd;
	int dest_fd;
	char template[100];

	setlocale (LC_ALL, "");
	bindtextdomain (GETTEXT_PACKAGE, MATELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	/* We only run as root */
	uid = getuid ();
	euid = geteuid ();
	if (uid != 0 || euid != 0) {
		/* Translators: only able to install RANDR profiles as root */
		g_print ("%s\n", _("This program can only be used by the root user"));
		return EXIT_FAILURE;
	}

	/* Usage: gsd-xrandr-install-systemwide SOURCE_FILE DEST_NAME */

	if (argc != 3) {
		usage (argv[0]);
		return EXIT_FAILURE;
	}

	source_filename = argv[1];
	dest_name = argv[2];

	/* Absolute source filenames only, please */

	if (!g_path_is_absolute (source_filename)) {
		g_print ("%s\n", _("The source filename must be absolute"));
		return EXIT_FAILURE;
	}

	/* We only copy regular files */

	source_fd = open (source_filename, O_RDONLY);
	if (source_fd == -1) {
		err = errno;

		/* Translators: first %s is a filename; second %s is an error message */
		g_print (_("Could not open %s: %s\n"),
			 source_filename,
			 g_strerror (err));
		return EXIT_FAILURE;
	}

	if (fstat (source_fd, &statbuf) != 0) {
		err = errno;
		/* Translators: first %s is a filename; second %s is an error message */
		g_print (_("Could not get information for %s: %s\n"),
			 source_filename,
			 g_strerror (err));
		return EXIT_FAILURE;
	}

	if (!S_ISREG (statbuf.st_mode)) {
		g_print (_("%s must be a regular file\n"),
			 source_filename);
		return EXIT_FAILURE;
	}

	/* We only copy files that are really owned by the calling user */

	pkexec_uid_str = g_getenv ("PKEXEC_UID");
	if (pkexec_uid_str == NULL) {
		g_print ("%s\n", _("This program must only be run through pkexec(1)"));
		return EXIT_FAILURE;
	}

	if (sscanf (pkexec_uid_str, "%d", &pkexec_uid) != 1) {
		g_print ("%s\n", _("PKEXEC_UID must be set to an integer value"));
		return EXIT_FAILURE;
	}

	if (statbuf.st_uid != pkexec_uid) {
		/* Translators: we are complaining that a file must be really owned by the user who called this program */
		g_print (_("%s must be owned by you\n"), source_filename);
		return EXIT_FAILURE;
	}

	/* We only accept basenames for the destination */

	if (!is_basename (dest_name)) {
		/* Translators: here we are saying that a plain filename must look like "filename", not like "some_dir/filename" */
		g_print (_("%s must not have any directory components\n"),
			 dest_name);
		return EXIT_FAILURE;
	}

	/* Chdir to the destination directory to keep it open... */

	if (chdir (SYSTEM_RANDR_PATH) != 0) {
		g_print (_("%s must be a directory\n"), SYSTEM_RANDR_PATH);
		return EXIT_FAILURE;
	}

	/* ... and open our temporary destination file right there */

	strcpy (template, "gsd-XXXXXX");
	dest_fd = g_mkstemp_full (template, O_WRONLY, 0644);
	if (dest_fd == -1) {
		err = errno;
		/* Translators: the first %s/%s is a directory/filename; the last %s is an error message */
		g_print (_("Could not open %s/%s: %s\n"),
			 SYSTEM_RANDR_PATH,
			 template,
			 g_strerror (err));
		return EXIT_FAILURE;
	}

	/* Do the copy */

	if (!copy_file (source_fd, dest_fd)) {
		/* If something went wrong, remove the destination file to avoid leaving trash around */
		unlink (template);
		return EXIT_FAILURE;
	}

	/* Rename to the final filename */

	if (rename (template, dest_name) != 0) {
		err = errno;
		unlink (template);
		g_print (_("Could not rename %s to %s: %s\n"),
			 template,
			 dest_name,
			 g_strerror (err));
		return EXIT_FAILURE;
	}

	/* Whew!  We'll leave the final closing of the files to the almighty kernel. */

	return EXIT_SUCCESS;
}
