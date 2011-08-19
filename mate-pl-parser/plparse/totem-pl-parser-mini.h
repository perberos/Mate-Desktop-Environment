/* 
   Copyright (C) 2002, 2003, 2004, 2005, 2006 Bastien Nocera
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

#ifndef TOTEM_PL_PARSER_MINI_H
#define TOTEM_PL_PARSER_MINI_H

#include <glib.h>

G_BEGIN_DECLS

gboolean totem_pl_parser_can_parse_from_data	 (const char *data,
 						  gsize len,
						  gboolean debug);
gboolean totem_pl_parser_can_parse_from_filename (const char *filename,
						  gboolean debug);

G_END_DECLS

#endif /* TOTEM_PL_PARSER_MINI_H */
