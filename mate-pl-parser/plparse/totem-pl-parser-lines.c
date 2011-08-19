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

#include <string.h>
#include <glib.h>

#ifndef TOTEM_PL_PARSER_MINI
#include <glib/gi18n-lib.h>

#include <gio/gio.h>

#include "totem-pl-parser.h"
#include "totemplparser-marshal.h"
#include "totem-pl-parser-pls.h"
#endif /* !TOTEM_PL_PARSER_MINI */

#include "totem-pl-parser-mini.h"
#include "totem-pl-parser-lines.h"
#include "totem-pl-parser-private.h"

#ifndef TOTEM_PL_PARSER_MINI

#define EXTINF "#EXTINF:"
#define EXTVLCOPT "#EXTVLCOPT"

static char *
totem_pl_parser_uri_to_dos (const char *uri, GFile *output)
{
	char *retval, *i;

	/* Get a relative URI if there is one */
	retval = totem_pl_parser_relative (output, uri);

	if (retval == NULL)
		retval = g_strdup (uri);

	/* Don't change URIs, but change smb:// */
	if (g_str_has_prefix (retval, "smb://") != FALSE) {
		char *tmp;
		tmp = g_strdup (retval + strlen ("smb:"));
		g_free (retval);
		retval = tmp;
	}

	if (strstr (retval, "://") != NULL)
		return retval;

	i = retval;
	while (*i != '\0')
	{
		if (*i == '/')
			*i = '\\';
		i++;
	}

	return retval;
}

gboolean
totem_pl_parser_save_m3u (TotemPlParser    *parser,
                          TotemPlPlaylist  *playlist,
                          GFile            *output,
                          gboolean          dos_compatible,
                          GError          **error)
{
        TotemPlPlaylistIter iter;
	GFileOutputStream *stream;
	gboolean valid, success;
	char *buf;
	char *cr;

	stream = g_file_replace (output, NULL, FALSE, G_FILE_CREATE_NONE, NULL, error);
	if (stream == NULL)
		return FALSE;

	cr = dos_compatible ? "\r\n" : "\n";

	buf = g_strdup_printf ("#EXTM3U%s", cr);
	success = totem_pl_parser_write_string (G_OUTPUT_STREAM (stream), buf, error);
	g_free (buf);
	if (success == FALSE)
		return FALSE;

        valid = totem_pl_playlist_iter_first (playlist, &iter);

        while (valid) {
		char *uri, *title, *path2;
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

		if (totem_pl_parser_scheme_is_ignored (parser, file) != FALSE) {
			g_object_unref (file);
			g_free (uri);
			g_free (title);
			continue;
		}
		g_object_unref (file);

		if (title) {
			buf = g_strdup_printf (EXTINF",%s%s", title, cr);
			success = totem_pl_parser_write_string (G_OUTPUT_STREAM (stream), buf, error);
			g_free (buf);
			if (success == FALSE) {
				g_free (title);
				g_free (uri);
				return FALSE;
			}
		}
		g_free (title);

		if (dos_compatible == FALSE) {
			char *tmp;

			tmp = totem_pl_parser_relative (output, uri);

			if (tmp == NULL && g_str_has_prefix (uri, "file:")) {
				path2 = g_filename_from_uri (uri, NULL, NULL);
			} else {
				path2 = tmp;
			}
		} else {
			path2 = totem_pl_parser_uri_to_dos (uri, output);
		}

		buf = g_strdup_printf ("%s%s", path2 ? path2 : uri, cr);
		g_free (path2);
		g_free (uri);

		success = totem_pl_parser_write_string (G_OUTPUT_STREAM (stream), buf, error);
		g_free (buf);

		if (success == FALSE)
			return FALSE;
	}

	g_object_unref (stream);

	return TRUE;
}

static void
totem_pl_parser_parse_ram_uri (TotemPlParser *parser, const char *uri)
{
	char *mark, **params;
	GString *str;
	guint i, num_params;
	char *title, *author, *copyright, *abstract, *screensize, *mode, *start, *end;

	if (g_str_has_prefix (uri, "rtsp://") == FALSE
	    && g_str_has_prefix (uri, "pnm://") == FALSE) {
		totem_pl_parser_add_one_uri (parser, uri, NULL);
		return;
	}

	/* Look for "?" */
	mark = strstr (uri, "?");
	if (mark == NULL) {
		totem_pl_parser_add_one_uri (parser, uri, NULL);
		return;
	}

	if (mark[1] == '\0') {
		char *new_uri;

		new_uri = g_strndup (uri, mark + 1 - uri);
		totem_pl_parser_add_one_uri (parser, new_uri, NULL);
		g_free (new_uri);
		return;
	}

	title = author = copyright = abstract = screensize = mode = end = start = NULL;
	num_params = 0;

	str = g_string_new_len (uri, mark - uri);
	params = g_strsplit (mark + 1, "&", -1);
	for (i = 0; params[i] != NULL; i++) {
		if (g_str_has_prefix (params[i], "title=") != FALSE) {
			title = params[i] + strlen ("title=");
		} else if (g_str_has_prefix (params[i], "author=") != FALSE) {
			author = params[i] + strlen ("author=");
		} else if (g_str_has_prefix (params[i], "copyright=") != FALSE) {
			copyright = params[i] + strlen ("copyright=");
		} else if (g_str_has_prefix (params[i], "abstract=") != FALSE) {
			abstract = params[i] + strlen ("abstract=");
		} else if (g_str_has_prefix (params[i], "screensize=") != FALSE) {
			screensize = params[i] + strlen ("screensize=");
		} else if (g_str_has_prefix (params[i], "mode=") != FALSE) {
			mode = params[i] + strlen ("mode=");
		} else if (g_str_has_prefix (params[i], "end=") != FALSE) {
			end = params[i] + strlen ("end=");
		} else if (g_str_has_prefix (params[i], "start=") != FALSE) {
			start = params[i] + strlen ("start=");
		} else {
			if (num_params == 0)
				g_string_append_c (str, '?');
			else
				g_string_append_c (str, '&');
			g_string_append (str, params[i]);
			num_params++;
		}
	}

	totem_pl_parser_add_uri (parser,
				 TOTEM_PL_PARSER_FIELD_URI, str->str,
				 TOTEM_PL_PARSER_FIELD_TITLE, title,
				 TOTEM_PL_PARSER_FIELD_AUTHOR, author,
				 TOTEM_PL_PARSER_FIELD_COPYRIGHT, copyright,
				 TOTEM_PL_PARSER_FIELD_ABSTRACT, abstract,
				 TOTEM_PL_PARSER_FIELD_SCREENSIZE, screensize,
				 TOTEM_PL_PARSER_FIELD_UI_MODE, mode,
				 TOTEM_PL_PARSER_FIELD_STARTTIME, start,
				 TOTEM_PL_PARSER_FIELD_ENDTIME, end,
				 NULL);

	g_string_free (str, TRUE);
	g_strfreev (params);
}

TotemPlParserResult
totem_pl_parser_add_ram (TotemPlParser *parser, GFile *file, TotemPlParseData *parse_data, gpointer data)
{
	gboolean retval = TOTEM_PL_PARSER_RESULT_UNHANDLED;
	char *contents, **lines;
	gsize size;
	guint i;

	if (g_file_load_contents (file, NULL, &contents, &size, NULL, NULL) == FALSE)
		return TOTEM_PL_PARSER_RESULT_ERROR;

	lines = g_strsplit_set (contents, "\r\n", 0);
	g_free (contents);

	for (i = 0; lines[i] != NULL; i++) {
		/* Empty line */
		if (totem_pl_parser_line_is_empty (lines[i]) != FALSE)
			continue;

		retval = TOTEM_PL_PARSER_RESULT_SUCCESS;

		/* Either it's a URI, or it has a proper path ... */
		if (strstr(lines[i], "://") != NULL
				|| lines[i][0] == G_DIR_SEPARATOR) {
			GFile *line_file;

			line_file = g_file_new_for_uri (lines[i]);
			/* .ram files can contain .smil entries */
			if (totem_pl_parser_parse_internal (parser, line_file, NULL, parse_data) != TOTEM_PL_PARSER_RESULT_SUCCESS)
				totem_pl_parser_parse_ram_uri (parser, lines[i]);
			g_object_unref (line_file);
		} else if (strcmp (lines[i], "--stop--") == 0) {
			/* For Real Media playlists, handle the stop command */
			break;
		} else {
			//FIXME
#if 0
			char *base;

			/* Try with a base */
			base = totem_pl_parser_base_uri (uri);

			if (totem_pl_parser_parse_internal (parser, lines[i], base) != TOTEM_PL_PARSER_RESULT_SUCCESS)
			{
				char *fullpath;
				fullpath = g_strdup_printf ("%s/%s", base, lines[i]);
				totem_pl_parser_parse_ram_uri (parser, fullpath);
				g_free (fullpath);
			}
			g_free (base);
#endif
		}
	}

	g_strfreev (lines);

	return retval;
}

static const char *
totem_pl_parser_get_extinfo_title (const char *extinfo)
{
	const char *res, *sep;

	if (extinfo == NULL)
		return NULL;

	/* It's bound to have an EXTINF if we have extinfo */
	res = extinfo + strlen(EXTINF);
	if (res[0] == '\0')
		return NULL;

	/* Handle ',' as a field separator */
	sep = strstr (res, ",");
	if (sep == NULL || sep[1] == '\0') {
		if (res[1] == '\0')
			return NULL;
		return res;
	}

	sep++;
	return sep;
}

TotemPlParserResult
totem_pl_parser_add_m3u (TotemPlParser *parser,
			 GFile *file,
			 GFile *base_file,
			 TotemPlParseData *parse_data,
			 gpointer data)
{
	TotemPlParserResult retval = TOTEM_PL_PARSER_RESULT_UNHANDLED;
	char *contents, **lines;
	gsize size;
	guint i, num_lines;
	gboolean dos_mode = FALSE;
	const char *extinfo;

	if (g_file_load_contents (file, NULL, &contents, &size, NULL, NULL) == FALSE)
		return TOTEM_PL_PARSER_RESULT_ERROR;

	/* .pls files with a .m3u extension, the nasties */
	if (g_str_has_prefix (contents, "[playlist]") != FALSE
			|| g_str_has_prefix (contents, "[Playlist]") != FALSE
			|| g_str_has_prefix (contents, "[PLAYLIST]") != FALSE) {
		retval = totem_pl_parser_add_pls_with_contents (parser, file, base_file, contents, parse_data);
		g_free (contents);
		return retval;
	}

	/* Try to use ISO-8859-1 if we don't have valid UTF-8,
	 * try to parse anyway if it's not ISO-8859-1 */
	if (g_utf8_validate (contents, -1, NULL) == FALSE) {
		char *fixed;
		fixed = g_convert (contents, -1, "UTF-8", "ISO8859-1", NULL, NULL, NULL);
		if (fixed != NULL) {
			g_free (contents);
			contents = fixed;
		}
	}

	/* is non-NULL if there's an EXTINF on a preceding line */
	extinfo = NULL;

	/* figure out whether we're a unix m3u or dos m3u */
	if (strstr(contents, "\x0d")) {
		dos_mode = TRUE;
	}

	lines = g_strsplit_set (contents, "\r\n", 0);
	g_free (contents);
	num_lines = g_strv_length (lines);
	/* We don't count the terminating NULL */
	num_lines--;

	for (i = 0; lines[i] != NULL; i++) {
		if (lines[i][0] == '\0')
			continue;

		retval = TOTEM_PL_PARSER_RESULT_SUCCESS;

		/* Ignore comments, but mark it if we have extra info */
		if (lines[i][0] == '#') {
			if (extinfo == NULL && g_str_has_prefix (lines[i], EXTINF) != FALSE)
				extinfo = lines[i];
			continue;
		}

		/* Either it's a URI, or it has a proper path ... */
		if (strstr(lines[i], "://") != NULL
				|| lines[i][0] == G_DIR_SEPARATOR) {
			GFile *uri;

			uri = g_file_new_for_commandline_arg (lines[i]);
			if (totem_pl_parser_parse_internal (parser, uri, NULL, parse_data) != TOTEM_PL_PARSER_RESULT_SUCCESS) {
				totem_pl_parser_add_one_uri (parser, lines[i],
						totem_pl_parser_get_extinfo_title (extinfo));
			}
			g_object_unref (uri);
			extinfo = NULL;
		} else if (g_ascii_isalpha (lines[i][0]) != FALSE
			   && g_str_has_prefix (lines[i] + 1, ":\\")) {
			/* Path relative to a drive on Windows, we need to use
			 * the base that was passed to us */
			GFile *uri;

			lines[i] = g_strdelimit (lines[i], "\\", '/');
			/* + 2, skip drive letter */
			uri = g_file_get_child (base_file, lines[i] + 2);
			totem_pl_parser_add_one_file (parser, uri,
						     totem_pl_parser_get_extinfo_title (extinfo));
			g_object_unref (uri);
			extinfo = NULL;
		} else if (lines[i][0] == '\\' && lines[i][1] == '\\') {
			/* ... Or it's in the windows smb form
			 * (\\machine\share\filename), Note drive names
			 * (C:\ D:\ etc) are unhandled (unknown base for
			 * drive letters) */
		        char *tmpuri;

			lines[i] = g_strdelimit (lines[i], "\\", '/');
			tmpuri = g_strjoin (NULL, "smb:", lines[i], NULL);

			totem_pl_parser_add_one_uri (parser, lines[i],
					totem_pl_parser_get_extinfo_title (extinfo));
			extinfo = NULL;

			g_free (tmpuri);
		} else {
			/* Try with a base */
			GFile *uri, *_base_file;
			char sep;

			_base_file = g_file_get_parent (file);
			sep = (dos_mode ? '\\' : '/');
			if (sep == '\\')
				lines[i] = g_strdelimit (lines[i], "\\", '/');
			uri = g_file_get_child (_base_file, lines[i]);
			g_object_unref (_base_file);
			totem_pl_parser_add_one_file (parser, uri,
						     totem_pl_parser_get_extinfo_title (extinfo));
			g_object_unref (uri);
			extinfo = NULL;
		}
	}

	g_strfreev (lines);

	return retval;
}

TotemPlParserResult
totem_pl_parser_add_ra (TotemPlParser *parser,
			GFile *file,
			GFile *base_file,
			TotemPlParseData *parse_data,
			gpointer data)
{
	if (data == NULL || totem_pl_parser_is_uri_list (data, strlen (data)) == NULL) {
		totem_pl_parser_add_one_file (parser, file, NULL);
		return TOTEM_PL_PARSER_RESULT_SUCCESS;
	}

	return totem_pl_parser_add_ram (parser, file, parse_data, NULL);
}

#endif /* !TOTEM_PL_PARSER_MINI */

#define CHECK_LEN if (i >= len) { return NULL; }

const char *
totem_pl_parser_is_uri_list (const char *data, gsize len)
{
	guint i = 0;

	/* Find the first bits of text */
	while (data[i] == '\n' || data[i] == '\t' || data[i] == ' ') {
		i++;
		CHECK_LEN;
	}
	CHECK_LEN;

	/* scheme always starts with a letter */
	if (g_ascii_isalpha (data[i]) == FALSE)
		return FALSE;
	while (g_ascii_isalnum (data[i]) != FALSE) {
		i++;
		CHECK_LEN;
	}

	CHECK_LEN;

	/* First non-alphanum character should be a ':' */
	if (data[i] != ':')
		return FALSE;
	i++;
	CHECK_LEN;

	if (data[i] != '/')
		return NULL;
	i++;
	CHECK_LEN;

	if (data[i] != '/')
		return NULL;

	return TEXT_URI_TYPE;
}

