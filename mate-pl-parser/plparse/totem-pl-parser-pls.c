/*
   Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007 Bastien Nocera
   Copyright (C) 2003, 2004 Colin Walters <walters@rhythmbox.org>

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
   write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301  USA.

   Author: Bastien Nocera <hadess@hadess.net>
 */

#include "config.h"

#ifndef TOTEM_PL_PARSER_MINI
#include <string.h>
#include <glib.h>
#include <glib/gi18n-lib.h>

#include "totem-pl-parser.h"
#include "totemplparser-marshal.h"
#endif /* !TOTEM_PL_PARSER_MINI */

#include "totem-pl-parser-mini.h"
#include "totem-pl-parser-pls.h"
#include "totem-pl-parser-private.h"

#ifndef TOTEM_PL_PARSER_MINI
gboolean
totem_pl_parser_save_pls (TotemPlParser    *parser,
                          TotemPlPlaylist  *playlist,
                          GFile            *output,
                          const gchar      *title,
                          GError          **error)
{
        TotemPlPlaylistIter iter;
	GFileOutputStream *stream;
	int num_entries, i;
	gboolean valid, success;
	char *buf;

	num_entries = totem_pl_parser_num_entries (parser, playlist);

	stream = g_file_replace (output, NULL, FALSE, G_FILE_CREATE_NONE, NULL, error);
	if (stream == NULL)
		return FALSE;

	buf = g_strdup ("[playlist]\n");
	success = totem_pl_parser_write_string (G_OUTPUT_STREAM (stream), buf, error);
	g_free (buf);
	if (success == FALSE)
		return FALSE;

	if (title != NULL) {
		buf = g_strdup_printf ("X-MATE-Title=%s\n", title);
		success = totem_pl_parser_write_string (G_OUTPUT_STREAM (stream), buf, error);
		g_free (buf);
		if (success == FALSE)
			return FALSE;
	}

	buf = g_strdup_printf ("NumberOfEntries=%d\n", num_entries);
	success = totem_pl_parser_write_string (G_OUTPUT_STREAM (stream), buf, error);
	g_free (buf);
	if (success == FALSE)
		return FALSE;

        valid = totem_pl_playlist_iter_first (playlist, &iter);
        i = 0;

        while (valid) {
                gchar *uri, *title, *relative;
                GFile *file;

                totem_pl_playlist_get (playlist, &iter,
                                       TOTEM_PL_PARSER_FIELD_URI, &uri,
                                       TOTEM_PL_PARSER_FIELD_TITLE, &title,
                                       NULL);

                valid = totem_pl_playlist_iter_next (playlist, &iter);

                if (!uri) {
                        g_free (title);
                        continue;
                }

                file = g_file_new_for_uri (uri);

                if (totem_pl_parser_scheme_is_ignored (parser, file)) {
                        g_object_unref (file);
                        g_free (uri);
                        g_free (title);
                        continue;
                }

                g_object_unref (file);
                i++;

                relative = totem_pl_parser_relative (output, uri);
                buf = g_strdup_printf ("File%d=%s\n", i, relative ? relative : uri);
                g_free (relative);
                g_free (uri);

                success = totem_pl_parser_write_string (G_OUTPUT_STREAM (stream), buf, error);
                g_free (buf);

                if (success == FALSE) {
                        g_free (title);
                        return FALSE;
                }

                if (!title) {
                        continue;
                }

                buf = g_strdup_printf ("Title%d=%s\n", i, title);
                success = totem_pl_parser_write_string (G_OUTPUT_STREAM (stream), buf, error);
                g_free (buf);
                g_free (title);

                if (success == FALSE) {
                        return FALSE;
                }
        }

	g_object_unref (stream);
	return TRUE;
}

static char *
ensure_utf8_valid (char *input)
{
	char *utf8_valid;

	utf8_valid = g_strdup (input);

	if (!g_utf8_validate (utf8_valid, -1, NULL)) {
		gint i;

		for (i = 0; i < g_utf8_strlen (utf8_valid, -1); i++) {
			gunichar c;
			c = g_utf8_get_char_validated (&utf8_valid[i], -1);
			if (c > 127) {
				utf8_valid[i] = '?';
			}
		}
	}
	return utf8_valid;
}

TotemPlParserResult
totem_pl_parser_add_pls_with_contents (TotemPlParser *parser,
				       GFile *file,
				       GFile *_base_file,
				       const char *contents,
				       TotemPlParseData *parse_data)
{
	TotemPlParserResult retval = TOTEM_PL_PARSER_RESULT_UNHANDLED;
	GFile *base_file;
	char **lines;
	guint i, num_entries;
	char *playlist_title;
	gboolean fallback;
	GHashTable *entries;
	guint found_entries;

	entries = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

	lines = g_strsplit_set (contents, "\r\n", 0);

	/* [playlist] */
	i = 0;
	num_entries = 0;
	playlist_title = NULL;

	/* Ignore empty lines */
	while (totem_pl_parser_line_is_empty (lines[i]) != FALSE)
		i++;

	if (lines[i] == NULL
	    || g_ascii_strncasecmp (lines[i], "[playlist]",
				    (gsize)strlen ("[playlist]")) != 0) {
		g_strfreev (lines);
		goto bail;
	}

	playlist_title = totem_pl_parser_read_ini_line_string (lines,
							       "X-MATE-Title");

	if (playlist_title != NULL) {
		totem_pl_parser_add_uri (parser,
					 TOTEM_PL_PARSER_FIELD_IS_PLAYLIST, TRUE,
					 TOTEM_PL_PARSER_FIELD_FILE, file,
					 TOTEM_PL_PARSER_FIELD_TITLE, playlist_title,
					 NULL);
	}

	/* Load the file in hash table to speed up the later processing */
	for (i = 0; lines[i] != NULL; i++) {
		char **bits;
		char *value;

		if (totem_pl_parser_line_is_empty (lines[i]))
			continue;

		if (lines[i][0] == '#' || lines[i][0] == '[')
			continue;

		bits = g_strsplit (lines[i], "=", 2);
		if (bits[0] == NULL || bits [1] == NULL) {
			g_strfreev (bits);
			continue;
		}

		if (g_ascii_strncasecmp (g_strchug (bits[0]), "file", strlen ("file")) == 0)
			num_entries++;

		value = g_strdup (bits[1]);

		g_hash_table_insert (entries,
				     g_ascii_strdown (bits[0], strlen (bits[0])),
				     value);
		g_strfreev (bits);
	}
	g_strfreev (lines);

	/* Base? */
	if (_base_file == NULL)
		base_file = g_file_get_parent (file);
	else
		base_file = g_object_ref (_base_file);

	retval = TOTEM_PL_PARSER_RESULT_SUCCESS;

	found_entries = 0;
	for (i = 1; found_entries < num_entries; i++) {
		char *file_str, *title, *genre, *length;
		char *file_key, *title_key, *genre_key, *length_key;
		gint64 length_num;

		file_key = g_strdup_printf ("file%d", i);
		title_key = g_strdup_printf ("title%d", i);
		length_key = g_strdup_printf ("length%d", i);
		length_num = 0;
		/* Genre is our own little extension */
		genre_key = g_strdup_printf ("genre%d", i);

		file_str = g_hash_table_lookup (entries, file_key);
		title = g_hash_table_lookup (entries, title_key);
		genre = g_hash_table_lookup (entries, genre_key);
		length = g_hash_table_lookup (entries, length_key);

		g_free (file_key);
		g_free (title_key);
		g_free (genre_key);
		g_free (length_key);

		if (file_str == NULL)
			continue;
		found_entries++;

		fallback = parse_data->fallback;
		if (parse_data->recurse)
			parse_data->fallback = FALSE;

		/* Get the length, if it's negative, that means that we have a stream
		 * and should push the entry straight away */
		if (length != NULL)
			length_num = totem_pl_parser_parse_duration (length, totem_pl_parser_is_debugging_enabled (parser));

		if (strstr (file_str, "://") != NULL || file_str[0] == G_DIR_SEPARATOR) {
			GFile *target;

			target = g_file_new_for_commandline_arg (file_str);
			if (length_num < 0 || totem_pl_parser_parse_internal (parser, target, NULL, parse_data) != TOTEM_PL_PARSER_RESULT_SUCCESS) {
				totem_pl_parser_add_uri (parser,
							 TOTEM_PL_PARSER_FIELD_URI, file_str,
							 TOTEM_PL_PARSER_FIELD_TITLE, title,
							 TOTEM_PL_PARSER_FIELD_GENRE, genre,
							 TOTEM_PL_PARSER_FIELD_DURATION, length,
							 TOTEM_PL_PARSER_FIELD_BASE_FILE, base_file, NULL);
			}
			g_object_unref (target);
		} else {
			GFile *target;
			char *utf8_filename;

			utf8_filename = ensure_utf8_valid (file_str);
			target = g_file_get_child_for_display_name (base_file, utf8_filename, NULL);
			g_free (utf8_filename);

			if (length_num < 0 || totem_pl_parser_parse_internal (parser, target, base_file, parse_data) != TOTEM_PL_PARSER_RESULT_SUCCESS) {
				totem_pl_parser_add_uri (parser,
							 TOTEM_PL_PARSER_FIELD_FILE, target,
							 TOTEM_PL_PARSER_FIELD_TITLE, title,
							 TOTEM_PL_PARSER_FIELD_GENRE, genre,
							 TOTEM_PL_PARSER_FIELD_DURATION, length,
							 TOTEM_PL_PARSER_FIELD_BASE_FILE, base_file, NULL);
			}

			g_object_unref (target);
		}

		parse_data->fallback = fallback;
	}

	if (playlist_title != NULL)
		totem_pl_parser_playlist_end (parser, playlist_title);

	g_object_unref (base_file);

bail:
	g_free (playlist_title);
        g_hash_table_destroy (entries);

	return retval;
}

TotemPlParserResult
totem_pl_parser_add_pls (TotemPlParser *parser,
			 GFile *file,
			 GFile *base_file,
			 TotemPlParseData *parse_data,
			 gpointer data)
{
	TotemPlParserResult retval = TOTEM_PL_PARSER_RESULT_UNHANDLED;
	char *contents;
	gsize size;

	if (g_file_load_contents (file, NULL, &contents, &size, NULL, NULL) == FALSE)
		return TOTEM_PL_PARSER_RESULT_ERROR;

	if (size == 0) {
		g_free (contents);
		return TOTEM_PL_PARSER_RESULT_SUCCESS;
	}

	retval = totem_pl_parser_add_pls_with_contents (parser, file, base_file, contents, parse_data);
	g_free (contents);

	return retval;
}

#endif /* !TOTEM_PL_PARSER_MINI */

