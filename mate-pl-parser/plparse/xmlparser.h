/*
 *  Copyright (C) 2002-2003,2007 the xine project
 *
 *  This file is part of xine, a free video player.
 *
 * The xine-lib XML parser is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The xine-lib XML parser is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Mate Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth
 * Floor, Boston, MA 02110, USA
 */

#ifndef XML_PARSER_H
#define XML_PARSER_H

#ifndef XINE_DEPRECATED
#define XINE_DEPRECATED
#endif

#ifndef XINE_PROTECTED
#define XINE_PROTECTED
#endif

/* parser modes */
#define XML_PARSER_CASE_INSENSITIVE  0
#define XML_PARSER_CASE_SENSITIVE    1

/* return codes */
#define XML_PARSER_OK                0
#define XML_PARSER_ERROR             1

/* xml_parser_build_tree_with_options flag bits */
#define XML_PARSER_RELAXED		1
#define XML_PARSER_MULTI_TEXT		2

/* node name for extra text chunks */
#define CDATA_MARKER "[CDATA]"

/* xml property */
typedef struct xml_property_s {
	char *name;
	char *value;
	struct xml_property_s *next;
} xml_property_t;

/* xml node */
/* .data contains any text which precedes any subtree elements;
 * subtree elements may also contain only text; if so, name is "[CDATA]".
 * e.g. <a>b<c />d</a>
 *   node1: .name="a"       .data="b"  .child=node2 .next=NULL
 *   node2: .name="c"       .data=NULL .child=NULL  .next=node3
 *   node3: .name="[CDATA]" .data="d"  .child=NULL  .next=NULL
 * Adjacent text items are merged.
 */
typedef struct xml_node_s {
	char *name;
	char *data;
	struct xml_property_s *props;
	struct xml_node_s *child;
	struct xml_node_s *next;
} xml_node_t;

/* xml parser */
typedef struct xml_parser_s {
	struct lexer *lexer;
	int mode;
} xml_parser_t;

void xml_parser_init(const char * buf, int size, int mode) XINE_DEPRECATED XINE_PROTECTED;
xml_parser_t *xml_parser_init_r(const char * buf, int size, int mode) XINE_PROTECTED;
void xml_parser_finalize_r(xml_parser_t *xml_parser) XINE_PROTECTED;

int xml_parser_build_tree(xml_node_t **root_node) XINE_DEPRECATED XINE_PROTECTED;
int xml_parser_build_tree_r(xml_parser_t *xml_parser, xml_node_t **root_node) XINE_PROTECTED;

int xml_parser_build_tree_with_options(xml_node_t **root_node, int flags) XINE_DEPRECATED XINE_PROTECTED;
int xml_parser_build_tree_with_options_r(xml_parser_t *xml_parser, xml_node_t **root_node, int flags) XINE_PROTECTED;

void xml_parser_free_tree(xml_node_t *root_node) XINE_PROTECTED;

const char *xml_parser_get_property (const xml_node_t *node, const char *name) XINE_PROTECTED;
int   xml_parser_get_property_int (const xml_node_t *node, const char *name,
				   int def_value) XINE_PROTECTED;
int xml_parser_get_property_bool (const xml_node_t *node, const char *name,
				  int def_value) XINE_PROTECTED;

/* for output:
 * returns an escaped string (free() it when done)
 * input must be in ASCII or UTF-8
 */

typedef enum {
  XML_ESCAPE_NO_QUOTE,
  XML_ESCAPE_SINGLE_QUOTE,
  XML_ESCAPE_DOUBLE_QUOTE
} xml_escape_quote_t;
char *xml_escape_string (const char *s, xml_escape_quote_t quote_type) XINE_PROTECTED;

/* for debugging purposes: dump read-in xml tree in a nicely
 * indented fashion
 */

void xml_parser_dump_tree (const xml_node_t *node) XINE_PROTECTED;

#endif
