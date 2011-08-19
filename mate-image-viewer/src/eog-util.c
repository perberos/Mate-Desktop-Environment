/* Eye Of Mate - General Utilities
 *
 * Copyright (C) 2006 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@mate.org>
 *
 * Based on code by:
 *	- Jens Finke <jens@mate.org>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/time.h>
#ifdef HAVE_STRPTIME
#define _XOPEN_SOURCE
#endif /* HAVE_STRPTIME */

#include <time.h>

#include "eog-util.h"

#include <errno.h>
#include <string.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <gio/gio.h>
#include <glib/gi18n.h>

void
eog_util_show_help (const gchar *section, GtkWindow *parent)
{
	GError *error = NULL;
	gchar *uri = NULL;

	if (section)
		uri = g_strdup_printf ("ghelp:eog?%s", section);

	gtk_show_uri (NULL, ((uri != NULL) ? uri : "ghelp:eog"),
		      gtk_get_current_event_time (), &error);

	g_free (uri);

	if (error) {
		GtkWidget *dialog;

		dialog = gtk_message_dialog_new (parent,
						 0,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_OK,
						 _("Could not display help for Eye of MATE"));

		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
							  "%s", error->message);

		g_signal_connect_swapped (dialog, "response",
					  G_CALLBACK (gtk_widget_destroy),
					  dialog);
		gtk_widget_show (dialog);

		g_error_free (error);
	}
}

gchar *
eog_util_make_valid_utf8 (const gchar *str)
{
	GString *string;
	const char *remainder, *invalid;
	int remaining_bytes, valid_bytes;

	string = NULL;
	remainder = str;
	remaining_bytes = strlen (str);

	while (remaining_bytes != 0) {
		if (g_utf8_validate (remainder, remaining_bytes, &invalid)) {
			break;
		}

		valid_bytes = invalid - remainder;

		if (string == NULL) {
			string = g_string_sized_new (remaining_bytes);
		}

		g_string_append_len (string, remainder, valid_bytes);
		g_string_append_c (string, '?');

		remaining_bytes -= valid_bytes + 1;
		remainder = invalid + 1;
	}

	if (string == NULL) {
		return g_strdup (str);
	}

	g_string_append (string, remainder);
	g_string_append (string, _(" (invalid Unicode)"));

	g_assert (g_utf8_validate (string->str, -1, NULL));

	return g_string_free (string, FALSE);
}

GSList*
eog_util_parse_uri_string_list_to_file_list (const gchar *uri_list)
{
	GSList* file_list = NULL;
	gsize i = 0;
	gchar **uris;

	uris = g_uri_list_extract_uris (uri_list);

	while (uris[i] != NULL) {
		file_list = g_slist_append (file_list, g_file_new_for_uri (uris[i]));
		i++;
	}

	g_strfreev (uris);

	return file_list;
}

GSList*
eog_util_string_list_to_file_list (GSList *string_list)
{
	GSList *it = NULL;
	GSList *file_list = NULL;

	for (it = string_list; it != NULL; it = it->next) {
		char *uri_str;

		uri_str = (gchar *) it->data;

		file_list = g_slist_prepend (file_list,
					     g_file_new_for_uri (uri_str));
	}

	return g_slist_reverse (file_list);
}

#ifdef HAVE_DBUS
GSList*
eog_util_strings_to_file_list (gchar **strings)
{
	int i;
 	GSList *file_list = NULL;

	for (i = 0; strings[i]; i++) {
 		file_list = g_slist_prepend (file_list,
					      g_file_new_for_uri (strings[i]));
 	}

 	return g_slist_reverse (file_list);
}
#endif

GSList*
eog_util_string_array_to_list (const gchar **files, gboolean create_uri)
{
	gint i;
	GSList *list = NULL;

	if (files == NULL) return list;

	for (i = 0; files[i]; i++) {
		char *str;

		if (create_uri) {
			GFile *file;

			file = g_file_new_for_commandline_arg (files[i]);
			str = g_file_get_uri (file);

			g_object_unref (file);
		} else {
			str = g_strdup (files[i]);
		}

		if (str) {
			list = g_slist_prepend (list, g_strdup (str));
			g_free (str);
		}
	}

	return g_slist_reverse (list);
}

gchar **
eog_util_string_array_make_absolute (gchar **files)
{
	int i;
	int size;
	gchar **abs_files;
	GFile *file;

	if (files == NULL)
		return NULL;

	size = g_strv_length (files);

	/* Ensure new list is NULL-terminated */
	abs_files = g_new0 (gchar *, size+1);

	for (i = 0; i < size; i++) {
		file = g_file_new_for_commandline_arg (files[i]);
		abs_files[i] = g_file_get_uri (file);

		g_object_unref (file);
	}

	return abs_files;
}

static gchar *dot_dir = NULL;

static gboolean
ensure_dir_exists (const char *dir)
{
	if (g_file_test (dir, G_FILE_TEST_IS_DIR))
		return TRUE;

	if (g_mkdir_with_parents (dir, 0700) == 0)
		return TRUE;

	if (errno == EEXIST)
		return g_file_test (dir, G_FILE_TEST_IS_DIR);

	g_warning ("Failed to create directory %s: %s", dir, strerror (errno));
	return FALSE;
}

const gchar *
eog_util_dot_dir (void)
{
	if (dot_dir == NULL) {
		gboolean exists;

		dot_dir = g_build_filename (g_get_home_dir (),
					    ".mate2",
					    "eog",
					    NULL);

		exists = ensure_dir_exists (dot_dir);

		if (G_UNLIKELY (!exists)) {
			static gboolean printed_warning = FALSE;

			if (!printed_warning) {
				g_warning ("EOG could not save some of your preferences in its settings directory due to a file with the same name (%s) blocking its creation. Please remove that file, or move it away.", dot_dir);
				printed_warning = TRUE;
			}
			dot_dir = NULL;
			return NULL;
		}
	}

	return dot_dir;
}

/* Based on eel_filename_strip_extension() */

/**
 * eog_util_filename_get_extension:
 * @filename: a filename
 *
 * Returns a reasonably good guess of the file extension of @filename.
 *
 * Returns: a newly allocated string with the file extension of @filename.
 **/
char *
eog_util_filename_get_extension (const char * filename)
{
	char *begin, *begin2;

	if (filename == NULL) {
		return NULL;
	}

	begin = strrchr (filename, '.');

	if (begin && begin != filename) {
		if (strcmp (begin, ".gz") == 0 ||
		    strcmp (begin, ".bz2") == 0 ||
		    strcmp (begin, ".sit") == 0 ||
		    strcmp (begin, ".Z") == 0) {
			begin2 = begin - 1;
			while (begin2 > filename &&
			       *begin2 != '.') {
				begin2--;
			}
			if (begin2 != filename) {
				begin = begin2;
			}
		}
		begin ++;
	} else {
		return NULL;
	}

	return g_strdup (begin);
}


/**
 * eog_util_file_is_persistent:
 * @file: a #GFile
 *
 * Checks whether @file is a non-removable local mount.
 *
 * Returns: %TRUE if @file is in a non-removable mount,
 * %FALSE otherwise or when it is remote.
 **/
gboolean
eog_util_file_is_persistent (GFile *file)
{
	GMount *mount;

	if (!g_file_is_native (file))
		return FALSE;

	mount = g_file_find_enclosing_mount (file, NULL, NULL);
	if (mount) {
		if (g_mount_can_unmount (mount)) {
			return FALSE;
		}
	}

	return TRUE;
}
