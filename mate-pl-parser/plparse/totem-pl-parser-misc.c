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

#include "totem-pl-parser.h"
#include "totemplparser-marshal.h"
#include "totem-disc.h"
#endif /* !TOTEM_PL_PARSER_MINI */

#include "totem-pl-parser-mini.h"
#include "totem-pl-parser-misc.h"
#include "totem-pl-parser-private.h"

#ifndef TOTEM_PL_PARSER_MINI
TotemPlParserResult
totem_pl_parser_add_gvp (TotemPlParser *parser,
			 GFile *file,
			 GFile *base_file,
			 TotemPlParseData *parse_data,
			 gpointer data)
{
	TotemPlParserResult retval = TOTEM_PL_PARSER_RESULT_UNHANDLED;
	char *contents, **lines, *title, *link, *version;
	gsize size;

	if (g_file_load_contents (file, NULL, &contents, &size, NULL, NULL) == FALSE)
		return TOTEM_PL_PARSER_RESULT_ERROR;

	if (g_str_has_prefix (contents, "#.download.the.free.Google.Video.Player") == FALSE && g_str_has_prefix (contents, "# download the free Google Video Player") == FALSE) {
		g_free (contents);
		return retval;
	}

	lines = g_strsplit (contents, "\n", 0);
	g_free (contents);

	/* We only handle GVP version 1.1 for now */
	version = totem_pl_parser_read_ini_line_string_with_sep (lines, "gvp_version", ":");
	if (version == NULL || strcmp (version, "1.1") != 0) {
		g_free (version);
		g_strfreev (lines);
		return retval;
	}
	g_free (version);

	link = totem_pl_parser_read_ini_line_string_with_sep (lines, "url", ":");
	if (link == NULL) {
		g_strfreev (lines);
		return retval;
	}

	retval = TOTEM_PL_PARSER_RESULT_SUCCESS;

	title = totem_pl_parser_read_ini_line_string_with_sep (lines, "title", ":");

	totem_pl_parser_add_one_uri (parser, link, title);

	g_free (link);
	g_free (title);
	g_strfreev (lines);

	return retval;
}

TotemPlParserResult
totem_pl_parser_add_desktop (TotemPlParser *parser,
			     GFile *file,
			     GFile *base_file,
			     TotemPlParseData *parse_data,
			     gpointer data)
{
	char *contents, **lines;
	const char *path, *display_name, *type;
	GFile *target;
	gsize size;
	TotemPlParserResult res = TOTEM_PL_PARSER_RESULT_ERROR;

	if (g_file_load_contents (file, NULL, &contents, &size, NULL, NULL) == FALSE)
		return res;

	lines = g_strsplit (contents, "\n", 0);
	g_free (contents);

	type = totem_pl_parser_read_ini_line_string (lines, "Type");
	if (type == NULL)
		goto bail;
	
	if (g_ascii_strcasecmp (type, "Link") != 0
	    && g_ascii_strcasecmp (type, "FSDevice") != 0) {
		goto bail;
	}

	path = totem_pl_parser_read_ini_line_string (lines, "URL");
	if (path == NULL)
		goto bail;
	target = g_file_new_for_uri (path);

	display_name = totem_pl_parser_read_ini_line_string (lines, "Name");

	if (totem_pl_parser_ignore (parser, path) == FALSE
	    && g_ascii_strcasecmp (type, "FSDevice") != 0) {
		totem_pl_parser_add_one_file (parser, target, display_name);
	} else {
		if (totem_pl_parser_parse_internal (parser, target, NULL, parse_data) != TOTEM_PL_PARSER_RESULT_SUCCESS)
			totem_pl_parser_add_one_file (parser, target, display_name);
	}

	res = TOTEM_PL_PARSER_RESULT_SUCCESS;

bail:
	g_strfreev (lines);

	return res;
}

#endif /* !TOTEM_PL_PARSER_MINI */


