/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* parser.c - Locations.xml parser
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define MATEWEATHER_I_KNOW_THIS_IS_UNSTABLE
#include "weather-priv.h"

#include "parser.h"

#include <string.h>
#include <glib.h>
#include <libxml/xmlreader.h>

/**
 * mateweather_parser_get_value:
 * @parser: a #MateWeatherParser
 *
 * Gets the text of the element whose start tag @parser is pointing to.
 * Leaves @parser pointing at the next node after the element's end tag.
 *
 * Return value: the text of the current node, as a libxml-allocated
 * string, or %NULL if the node is empty.
 **/
char *
mateweather_parser_get_value (MateWeatherParser *parser)
{
    char *value;

    /* check for null node */
    if (xmlTextReaderIsEmptyElement (parser->xml))
	return NULL;

    /* the next "node" is the text node containing the value we want to get */
    if (xmlTextReaderRead (parser->xml) != 1)
	return NULL;

    value = (char *) xmlTextReaderValue (parser->xml);

    /* move on to the end of this node */
    while (xmlTextReaderNodeType (parser->xml) != XML_READER_TYPE_END_ELEMENT) {
	if (xmlTextReaderRead (parser->xml) != 1) {
	    xmlFree (value);
	    return NULL;
	}
    }

    /* consume the end element too */
    if (xmlTextReaderRead (parser->xml) != 1) {
	xmlFree (value);
	return NULL;
    }

    return value;
}

/**
 * mateweather_parser_get_localized_value:
 * @parser: a #MateWeatherParser
 *
 * Looks at the name of the element @parser is currently pointing to, and
 * returns the content of either that node, or a following node with
 * the same name but an "xml:lang" attribute naming one of the locale
 * languages. Leaves @parser pointing to the next node after the last
 * consecutive element with the same name as the original element.
 *
 * Return value: the localized (or unlocalized) text, as a
 * libxml-allocated string, or %NULL if the node is empty.
 **/
char *
mateweather_parser_get_localized_value (MateWeatherParser *parser)
{
    const char *this_language;
    int best_match = INT_MAX;
    const char *lang, *tagname, *next_tagname;
    gboolean keep_going;
    char *name = NULL;
    int i;

    tagname = (const char *) xmlTextReaderConstName (parser->xml);

    do {
	/* First let's get the language */
	lang = (const char *) xmlTextReaderConstXmlLang (parser->xml);

	if (lang == NULL)
	    this_language = "C";
	else
	    this_language = lang;

	/* the next "node" is text node containing the actual name */
	if (xmlTextReaderRead (parser->xml) != 1) {
	    if (name)
		xmlFree (name);
	    return NULL;
	}

	for (i = 0; parser->locales[i] && i < best_match; i++) {
	    if (!strcmp (parser->locales[i], this_language)) {
		/* if we've already encounted a less accurate
		   translation, then free it */
		g_free (name);

		name = (char *) xmlTextReaderValue (parser->xml);
		best_match = i;

		break;
	    }
	}

	/* Skip to close tag */
	while (xmlTextReaderNodeType (parser->xml) != XML_READER_TYPE_END_ELEMENT) {
	    if (xmlTextReaderRead (parser->xml) != 1) {
		xmlFree (name);
		return NULL;
	    }
	}

	/* Skip junk */
	do {
	    if (xmlTextReaderRead (parser->xml) != 1) {
		xmlFree (name);
		return NULL;
	    }
	} while (xmlTextReaderNodeType (parser->xml) != XML_READER_TYPE_ELEMENT &&
		 xmlTextReaderNodeType (parser->xml) != XML_READER_TYPE_END_ELEMENT);

	/* if the next tag has the same name then keep going */
	next_tagname = (const char *) xmlTextReaderConstName (parser->xml);
	keep_going = !strcmp (next_tagname, tagname);

    } while (keep_going);

    return name;
}

MateWeatherParser *
mateweather_parser_new (gboolean use_regions)
{
    MateWeatherParser *parser;
    int zlib_support;
    int i, keep_going;
    char *filename;
    char *tagname, *format;
    time_t now;
    struct tm tm;

    parser = g_slice_new0 (MateWeatherParser);
    parser->use_regions = use_regions;
    parser->locales = g_get_language_names ();

    zlib_support = xmlHasFeature (XML_WITH_ZLIB);

    /* First try to load a locale-specific XML. It's much faster. */
    filename = NULL;
    for (i = 0; parser->locales[i] != NULL; i++) {
	filename = g_strdup_printf ("%s/Locations.%s.xml",
				    MATEWEATHER_XML_LOCATION_DIR,
				    parser->locales[i]);

	if (g_file_test (filename, G_FILE_TEST_IS_REGULAR))
	    break;

	g_free (filename);
	filename = NULL;

        if (!zlib_support)
            continue;

	filename = g_strdup_printf ("%s/Locations.%s.xml.gz",
				    MATEWEATHER_XML_LOCATION_DIR,
				    parser->locales[i]);

	if (g_file_test (filename, G_FILE_TEST_IS_REGULAR))
	    break;

	g_free (filename);
	filename = NULL;
    }

    /* Fall back on the file containing either all translations, or only
     * the english names (depending on the configure flags).
     */
    if (!filename)
	filename = g_build_filename (MATEWEATHER_XML_LOCATION_DIR, "Locations.xml", NULL);

    if (!g_file_test (filename, G_FILE_TEST_IS_REGULAR) && zlib_support) {
        g_free (filename);
	filename = g_build_filename (MATEWEATHER_XML_LOCATION_DIR, "Locations.xml.gz", NULL);
    }

    /* Open the xml file containing the different locations */
    parser->xml = xmlNewTextReaderFilename (filename);
    g_free (filename);

    if (parser->xml == NULL)
	goto error_out;

    /* fast forward to the first element */
    do {
	/* if we encounter a problem here, exit right away */
	if (xmlTextReaderRead (parser->xml) != 1)
	    goto error_out;
    } while (xmlTextReaderNodeType (parser->xml) != XML_READER_TYPE_ELEMENT);

    /* check the name and format */
    tagname = (char *) xmlTextReaderName (parser->xml);
    keep_going = tagname && !strcmp (tagname, "mateweather");
    xmlFree (tagname);

    if (!keep_going)
	goto error_out;

    format = (char *) xmlTextReaderGetAttribute (parser->xml, (xmlChar *) "format");
    keep_going = format && !strcmp (format, "1.0");
    xmlFree (format);

    if (!keep_going)
	goto error_out;

    /* Get timestamps for the start and end of this year */
    now = time (NULL);
    tm = *gmtime (&now);
    tm.tm_mon = 0;
    tm.tm_mday = 1;
    tm.tm_hour = tm.tm_min = tm.tm_sec = 0;
    parser->year_start = mktime (&tm);
    tm.tm_year++;
    parser->year_end = mktime (&tm);

    return parser;

error_out:
    mateweather_parser_free (parser);
    return NULL;
}

void
mateweather_parser_free (MateWeatherParser *parser)
{
    if (parser->xml)
	xmlFreeTextReader (parser->xml);
    g_slice_free (MateWeatherParser, parser);
}
