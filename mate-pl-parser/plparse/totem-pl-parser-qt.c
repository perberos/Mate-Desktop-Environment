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
#include "xmlparser.h"

#include "totem-pl-parser.h"
#include "totemplparser-marshal.h"
#endif /* !TOTEM_PL_PARSER_MINI */

#include "totem-pl-parser-mini.h"
#include "totem-pl-parser-qt.h"
#include "totem-pl-parser-smil.h"
#include "totem-pl-parser-private.h"

#define QT_NEEDLE "<?quicktime"

const char *
totem_pl_parser_is_quicktime (const char *data, gsize len)
{
	if (len == 0)
		return FALSE;
	if (len > MIME_READ_CHUNK_SIZE)
		len = MIME_READ_CHUNK_SIZE;

	/* Check for RTSPtextRTSP Quicktime references */
	if (len <= strlen ("RTSPtextRTSP://"))
		return NULL;
	if (g_str_has_prefix (data, "RTSPtext") != FALSE
			|| g_str_has_prefix (data, "rtsptext") != FALSE) {
		return QUICKTIME_META_MIME_TYPE;
	}
	if (g_str_has_prefix (data, "SMILtext") != FALSE)
		return QUICKTIME_META_MIME_TYPE;

	if (g_strstr_len (data, len, QT_NEEDLE) != NULL)
		return QUICKTIME_META_MIME_TYPE;

	return NULL;
}

#ifndef TOTEM_PL_PARSER_MINI

static TotemPlParserResult
totem_pl_parser_add_quicktime_rtsptext (TotemPlParser *parser,
					GFile *file,
					GFile *base_file,
					TotemPlParseData *parse_data,
					gpointer data)
{
	char *contents = NULL;
	char *volume, *autoplay, *rtspuri;
	gsize size;
	char **lines;

	if (g_file_load_contents (file, NULL, &contents, &size, NULL, NULL) == FALSE)
		return TOTEM_PL_PARSER_RESULT_ERROR;

	lines = g_strsplit_set (contents, "\r\n", 0);

	volume = totem_pl_parser_read_ini_line_string_with_sep
		(lines, "volume", "=");
	autoplay = totem_pl_parser_read_ini_line_string_with_sep
		(lines, "autoplay", "=");

	rtspuri = g_strdup (lines[0] + strlen ("RTSPtext"));
	if (rtspuri[0] == '\0') {
		char **line;
		g_free (rtspuri);

		for (line = lines + 1; line && *line[0] == '\0'; line++);
		if (line == NULL)
			return TOTEM_PL_PARSER_RESULT_ERROR;
		rtspuri = g_strdup (*line);
	}
	g_strstrip (rtspuri);

	totem_pl_parser_add_uri (parser,
				 TOTEM_PL_PARSER_FIELD_URI, rtspuri,
				 TOTEM_PL_PARSER_FIELD_VOLUME, volume,
				 TOTEM_PL_PARSER_FIELD_AUTOPLAY, autoplay,
				 NULL);
	g_free (rtspuri);
	g_free (volume);
	g_free (autoplay);
	g_strfreev (lines);

	return TOTEM_PL_PARSER_RESULT_SUCCESS;
}

static TotemPlParserResult
totem_pl_parser_add_quicktime_metalink (TotemPlParser *parser,
					GFile *file,
					GFile *base_file,
					TotemPlParseData *parse_data,
					gpointer data)
{
	xml_node_t *doc, *node;
	gsize size;
	char *contents;
	const char *item_uri, *autoplay;
	gboolean found;

	if (g_str_has_prefix (data, "RTSPtext") != FALSE
			|| g_str_has_prefix (data, "rtsptext") != FALSE) {
		return totem_pl_parser_add_quicktime_rtsptext (parser, file, base_file, parse_data, data);
	}
	if (g_str_has_prefix (data, "SMILtext") != FALSE) {
		char *contents;
		gsize size;
		TotemPlParserResult retval;

		if (g_file_load_contents (file, NULL, &contents, &size, NULL, NULL) == FALSE)
			return TOTEM_PL_PARSER_RESULT_ERROR;

		retval = totem_pl_parser_add_smil_with_data (parser,
							     file, base_file,
							     contents + strlen ("SMILtext"),
							     size - strlen ("SMILtext"));
		g_free (contents);
		return retval;
	}

	if (g_file_load_contents (file, NULL, &contents, &size, NULL, NULL) == FALSE)
		return TOTEM_PL_PARSER_RESULT_ERROR;

	doc = totem_pl_parser_parse_xml_relaxed (contents, size);
	if (doc == NULL) {
		g_free (contents);
		return TOTEM_PL_PARSER_RESULT_ERROR;
	}
	g_free (contents);

	/* Check for quicktime type */
	for (node = doc, found = FALSE; node != NULL; node = node->next) {
		const char *type;

		if (node->name == NULL)
			continue;
		if (g_ascii_strcasecmp (node->name , "?quicktime") != 0)
			continue;
		type = xml_parser_get_property (node, "type");
		if (g_ascii_strcasecmp ("application/x-quicktime-media-link", type) != 0)
			continue;
		found = TRUE;
	}

	if (found == FALSE) {
		xml_parser_free_tree (doc);
		return TOTEM_PL_PARSER_RESULT_ERROR;
	}

	if (!doc || !doc->name
	    || g_ascii_strcasecmp (doc->name, "embed") != 0) {
		xml_parser_free_tree (doc);
		return TOTEM_PL_PARSER_RESULT_ERROR;
	}

	item_uri = xml_parser_get_property (doc, "src");
	if (!item_uri) {
		xml_parser_free_tree (doc);
		return TOTEM_PL_PARSER_RESULT_ERROR;
	}

	autoplay = xml_parser_get_property (doc, "autoplay");
	/* Add a default as per the QuickTime docs */
	if (autoplay == NULL)
		autoplay = "true";

	totem_pl_parser_add_uri (parser,
				 TOTEM_PL_PARSER_FIELD_URI, item_uri,
				 TOTEM_PL_PARSER_FIELD_AUTOPLAY, autoplay,
				 NULL);
	xml_parser_free_tree (doc);

	return TOTEM_PL_PARSER_RESULT_SUCCESS;
}

TotemPlParserResult
totem_pl_parser_add_quicktime (TotemPlParser *parser,
			       GFile *file,
			       GFile *base_file,
			       TotemPlParseData *parse_data,
			       gpointer data)
{
	if (data == NULL || totem_pl_parser_is_quicktime (data, strlen (data)) == NULL)
		return TOTEM_PL_PARSER_RESULT_UNHANDLED;

	return totem_pl_parser_add_quicktime_metalink (parser, file, base_file, parse_data, data);
}

#endif /* !TOTEM_PL_PARSER_MINI */

