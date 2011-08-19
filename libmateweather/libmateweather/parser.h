/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* parser.h - Locations.xml parser
 *
 * Copyright 2008, Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef MATEWEATHER_PARSER_H
#define MATEWEATHER_PARSER_H 1

#include <libxml/xmlreader.h>
#include "mateweather-timezone.h"

typedef struct {
    xmlTextReaderPtr xml;
    const char * const *locales;
    gboolean use_regions;
    time_t year_start, year_end;
} MateWeatherParser;

MateWeatherParser *mateweather_parser_new                 (gboolean        use_regions);
void            mateweather_parser_free                (MateWeatherParser *parser);

char           *mateweather_parser_get_value           (MateWeatherParser *parser);
char           *mateweather_parser_get_localized_value (MateWeatherParser *parser);

/* from mateweather-timezone.c */
MateWeatherTimezone **mateweather_timezones_parse_xml (MateWeatherParser *parser);

#endif
