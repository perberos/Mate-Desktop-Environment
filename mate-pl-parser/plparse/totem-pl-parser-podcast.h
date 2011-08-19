/* 
   Copyright (C) 2007 Bastien Nocera <hadess@hadess.net>

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

#ifndef TOTEM_PL_PARSER_PODCAST_H
#define TOTEM_PL_PARSER_PODCAST_H

G_BEGIN_DECLS

#ifndef TOTEM_PL_PARSER_MINI
#include "totem-pl-parser.h"
#include "totem-pl-parser-private.h"
#include <gio/gio.h>
#else
#include "totem-pl-parser-mini.h"
#endif /* !TOTEM_PL_PARSER_MINI */

const char * totem_pl_parser_is_rss (const char *data, gsize len);
const char * totem_pl_parser_is_atom (const char *data, gsize len);
const char * totem_pl_parser_is_opml (const char *data, gsize len);
const char * totem_pl_parser_is_xml_feed (const char *data, gsize len);

#ifndef TOTEM_PL_PARSER_MINI

#ifndef HAVE_GMIME
#define WARN_NO_GMIME { \
	g_warning("Trying to parse a podcast, but totem-pl-parser built without libgmime support. Please contact your distribution provider."); \
	return TOTEM_PL_PARSER_RESULT_ERROR; \
}
#endif /* !HAVE_GMIME */

gboolean totem_pl_parser_is_itms_feed (GFile *file);

TotemPlParserResult totem_pl_parser_add_xml_feed (TotemPlParser *parser,
						  GFile *file,
						  GFile *base_file,
						  TotemPlParseData *parse_data,
						  gpointer data);
TotemPlParserResult totem_pl_parser_add_atom (TotemPlParser *parser,
					      GFile *file,
					      GFile *base_file,
					      TotemPlParseData *parse_data,
					      gpointer data);
TotemPlParserResult totem_pl_parser_add_rss (TotemPlParser *parser,
					     GFile *file,
					     GFile *base_file,
					     TotemPlParseData *parse_data,
					     gpointer data);
TotemPlParserResult totem_pl_parser_add_itpc (TotemPlParser *parser,
					      GFile *file,
					      GFile *base_file,
					      TotemPlParseData *parse_data,
					      gpointer data);
TotemPlParserResult totem_pl_parser_add_zune (TotemPlParser *parser,
					      GFile *file,
					      GFile *base_file,
					      TotemPlParseData *parse_data,
					      gpointer data);
TotemPlParserResult totem_pl_parser_add_itms (TotemPlParser *parser,
					      GFile *file,
					      GFile *base_file,
					      TotemPlParseData *parse_data,
					      gpointer data);
TotemPlParserResult totem_pl_parser_add_opml (TotemPlParser *parser,
					      GFile *file,
					      GFile *base_file,
					      TotemPlParseData *parse_data,
					      gpointer data);

#endif /* !TOTEM_PL_PARSER_MINI */

G_END_DECLS

#endif /* TOTEM_PL_PARSER_PODCAST_H */
