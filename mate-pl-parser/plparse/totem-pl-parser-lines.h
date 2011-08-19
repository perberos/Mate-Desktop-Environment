/* 
   2002, 2003, 2004, 2005, 2006, 2007 Bastien Nocera
   Copyright (C) 2003 Colin Walters <walters@verbum.org>

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

#ifndef TOTEM_PL_PARSER_LINES_H
#define TOTEM_PL_PARSER_LINES_H

G_BEGIN_DECLS

#ifndef TOTEM_PL_PARSER_MINI
#include "totem-pl-parser.h"
#include "totem-pl-parser-private.h"
#include <gio/gio.h>
#else
#include "totem-pl-parser-mini.h"
#endif /* !TOTEM_PL_PARSER_MINI */

const char * totem_pl_parser_is_uri_list (const char *data, gsize len);

#ifndef TOTEM_PL_PARSER_MINI
gboolean totem_pl_parser_save_m3u (TotemPlParser *parser,
                                   TotemPlPlaylist *playlist,
                                   GFile *output,
                                   gboolean dos_compatible,
                                   GError **error);

TotemPlParserResult totem_pl_parser_add_ram (TotemPlParser *parser,
					     GFile *file,
					     TotemPlParseData *parse_data,
					     gpointer data);
TotemPlParserResult totem_pl_parser_add_m3u (TotemPlParser *parser,
					     GFile *file,
					     GFile *base_file,
					     TotemPlParseData *parse_data,
					     gpointer data);
TotemPlParserResult totem_pl_parser_add_ra (TotemPlParser *parser,
					    GFile *file,
					    GFile *base_file,
					    TotemPlParseData *parse_data,
					    gpointer data);
#endif /* !TOTEM_PL_PARSER_MINI */

G_END_DECLS

#endif /* TOTEM_PL_PARSER_LINES_H */
