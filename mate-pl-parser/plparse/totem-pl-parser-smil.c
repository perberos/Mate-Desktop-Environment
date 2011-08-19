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
#include <glib.h>

#include <gio/gio.h>
#include "xmlparser.h"
#include "totem-pl-parser.h"
#include "totemplparser-marshal.h"
#endif /* !TOTEM_PL_PARSER_MINI */

#include "totem-pl-parser-mini.h"
#include "totem-pl-parser-smil.h"
#include "totem-pl-parser-private.h"

#ifndef TOTEM_PL_PARSER_MINI
static void
parse_smil_entry_add (TotemPlParser *parser,
		      GFile *base_file,
		      const char *uri,
		      const char *title,
		      const char *abstract,
		      const char *copyright,
		      const char *author,
		      const char *clip_begin,
		      const char *dur,
		      const char *subtitle_uri)
{
	char *resolved_uri, *sub;
	GFile *resolved;

	resolved_uri = totem_pl_parser_resolve_uri (base_file, uri);
	if (resolved_uri == NULL)
		resolved = g_file_new_for_uri (uri);
	else
		resolved = g_file_new_for_uri (resolved_uri);
	g_free (resolved_uri);

	sub = NULL;
	if (subtitle_uri != NULL)
		sub = totem_pl_parser_resolve_uri (base_file, subtitle_uri);

	totem_pl_parser_add_uri (parser,
				 TOTEM_PL_PARSER_FIELD_FILE, resolved,
				 TOTEM_PL_PARSER_FIELD_TITLE, title,
				 TOTEM_PL_PARSER_FIELD_ABSTRACT, abstract,
				 TOTEM_PL_PARSER_FIELD_COPYRIGHT, copyright,
				 TOTEM_PL_PARSER_FIELD_AUTHOR, author,
				 TOTEM_PL_PARSER_FIELD_STARTTIME, clip_begin,
				 TOTEM_PL_PARSER_FIELD_DURATION, dur,
				 TOTEM_PL_PARSER_FIELD_SUBTITLE_URI, sub ? sub : subtitle_uri,
				 NULL);
	g_object_unref (resolved);
	g_free (sub);
}

static TotemPlParserResult
parse_smil_entry (TotemPlParser *parser,
		  GFile *base_file,
		  xml_node_t *doc,
		  xml_node_t *parent,
		  const char *parent_title)
{
	xml_node_t *node;
	const char *title, *uri, *author, *abstract, *dur, *clip_begin, *copyright, *subtitle_uri;
	TotemPlParserResult retval = TOTEM_PL_PARSER_RESULT_ERROR;
	gboolean added;

	title = NULL;
	uri = NULL;
	subtitle_uri = NULL;
	author = NULL;
	abstract = NULL;
	dur = NULL;
	clip_begin = NULL;
	copyright = NULL;
	added = FALSE;

	for (node = parent->child; node != NULL; node = node->next) {
		if (node->name == NULL)
			continue;

		/* ENTRY should only have one ref and one title nodes */
		if (g_ascii_strcasecmp (node->name, "video") == 0
		    || g_ascii_strcasecmp (node->name, "audio") == 0
		    || g_ascii_strcasecmp (node->name, "media") == 0) {
			/* Send the previous entry */
			if (uri != NULL && added == FALSE) {
				parse_smil_entry_add (parser,
						      base_file,
						      uri,
						      title ? title : parent_title,
						      abstract,
						      copyright,
						      author,
						      clip_begin,
						      dur,
						      subtitle_uri);
				added = TRUE;
				retval = TOTEM_PL_PARSER_RESULT_SUCCESS;
			}

			uri = xml_parser_get_property (node, "src");
			title = xml_parser_get_property (node, "title");
			author = xml_parser_get_property (node, "author");
			dur = xml_parser_get_property (node, "dur");
			clip_begin = xml_parser_get_property (node, "clip-begin");
			abstract = xml_parser_get_property (node, "abstract");
			copyright = xml_parser_get_property (node, "copyright");
			subtitle_uri = NULL;
			added = FALSE;
		} else if (g_ascii_strcasecmp (node->name, "textstream") == 0) {
			subtitle_uri = xml_parser_get_property (node, "src");
		} else {
			if (parse_smil_entry (parser,
						base_file, doc, node, parent_title) != FALSE)
				retval = TOTEM_PL_PARSER_RESULT_SUCCESS;
		}
	}

	if (uri != NULL && added == FALSE) {
		parse_smil_entry_add (parser,
				      base_file,
				      uri,
				      title ? title : parent_title,
				      abstract,
				      copyright,
				      author,
				      clip_begin,
				      dur,
				      subtitle_uri);
		added = TRUE;
		retval = TOTEM_PL_PARSER_RESULT_SUCCESS;
	}

	return retval;
}

static const char*
parse_smil_head (TotemPlParser *parser, xml_node_t *doc, xml_node_t *parent)
{
	xml_node_t *node;
	const char *title;
	
	title = NULL;

	for (node = parent->child; node != NULL; node = node->next) {
		if (g_ascii_strcasecmp (node->name, "meta") == 0) {
			const char *prop;
			prop = xml_parser_get_property (node, "name");
			if (prop != NULL && g_ascii_strcasecmp (prop, "title") == 0) {
				title = xml_parser_get_property (node, "content");
				if (title != NULL)
					break;
			}
		}
	}

	return title;
}

static TotemPlParserResult
parse_smil_entries (TotemPlParser *parser, GFile *base_file, xml_node_t *doc)
{
	TotemPlParserResult retval = TOTEM_PL_PARSER_RESULT_ERROR;
	const char *title;
	xml_node_t *node;

	title = NULL;

	for (node = doc->child; node != NULL; node = node->next) {
		if (node->name == NULL)
			continue;

		if (g_ascii_strcasecmp (node->name, "body") == 0) {
			if (parse_smil_entry (parser, base_file,
					      doc, node, title) != FALSE) {
				retval = TOTEM_PL_PARSER_RESULT_SUCCESS;
			}
		} else if (title == NULL) {
			if (g_ascii_strcasecmp (node->name, "head") == 0)
				title = parse_smil_head (parser, doc, node);
		}
	}

	return retval;
}

static TotemPlParserResult
totem_pl_parser_add_smil_with_doc (TotemPlParser *parser, GFile *file,
				   GFile *base_file, xml_node_t *doc)
{
	TotemPlParserResult retval = TOTEM_PL_PARSER_RESULT_UNHANDLED;

	/* If the document has no root, or no name */
	if(doc->name == NULL
	   || g_ascii_strcasecmp (doc->name, "smil") != 0) {
		return TOTEM_PL_PARSER_RESULT_ERROR;
	}

	retval = parse_smil_entries (parser, base_file, doc);

	return retval;
}

TotemPlParserResult
totem_pl_parser_add_smil (TotemPlParser *parser,
			  GFile *file,
			  GFile *base_file,
			  TotemPlParseData *parse_data,
			  gpointer data)
{
	char *contents;
	gsize size;
	TotemPlParserResult retval;

	if (g_file_load_contents (file, NULL, &contents, &size, NULL, NULL) == FALSE)
		return TOTEM_PL_PARSER_RESULT_ERROR;

	retval = totem_pl_parser_add_smil_with_data (parser, file,
						     base_file, contents, size);
	g_free (contents);

	return retval;
}

TotemPlParserResult
totem_pl_parser_add_smil_with_data (TotemPlParser *parser,
				    GFile *file,
				    GFile *base_file,
				    const char *contents, int size)
{
	xml_node_t* doc;
	TotemPlParserResult retval;
	char *contents_dup;

	contents_dup = g_strndup (contents, size);
	doc = totem_pl_parser_parse_xml_relaxed (contents_dup, size);
	if (doc == NULL) {
		g_free (contents_dup);
		return TOTEM_PL_PARSER_RESULT_ERROR;
	}

	retval = totem_pl_parser_add_smil_with_doc (parser, file, base_file, doc);
	g_free (contents_dup);
	xml_parser_free_tree (doc);

	return retval;
}

#endif /* !TOTEM_PL_PARSER_MINI */

