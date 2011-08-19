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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef XINE_COMPILE
# include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#ifdef _MSC_VER
#define snprintf sprintf_s
#define strcasecmp stricmp
#endif

#define LOG_MODULE "xmlparser"
#define LOG_VERBOSE
/*
#define LOG
*/

#include <totem_internal.h>
#ifdef XINE_COMPILE
#include <xine/xineutils.h>
#else
#define lprintf(...)
#define XINE_MALLOC
#endif
#include "xmllexer.h"
#include "xmlparser.h"


#define TOKEN_SIZE  64 * 1024
#define DATA_SIZE   64 * 1024
#define MAX_RECURSION 26

/* private global variables */
xml_parser_t * static_xml_parser;

/* private functions */

static char * strtoupper(char * str) {
  int i = 0;

  while (str[i] != '\0') {
    str[i] = (char)toupper((int)str[i]);
    i++;
  }
  return str;
}

static xml_node_t * new_xml_node(void) {
  xml_node_t * new_node;

  new_node = (xml_node_t*) malloc(sizeof(xml_node_t));
  new_node->name  = NULL;
  new_node->data  = NULL;
  new_node->props = NULL;
  new_node->child = NULL;
  new_node->next  = NULL;
  return new_node;
}

static const char cdata[] = CDATA_MARKER;

static void free_xml_node(xml_node_t * node) {
  if (node->name != cdata)
    free (node->name);
  free (node->data);
  free(node);
}

static xml_property_t *XINE_MALLOC new_xml_property(void) {
  xml_property_t * new_property;

  new_property = (xml_property_t*) malloc(sizeof(xml_property_t));
  new_property->name  = NULL;
  new_property->value = NULL;
  new_property->next  = NULL;
  return new_property;
}

static void free_xml_property(xml_property_t * property) {
  free (property->name);
  free (property->value);
  free(property);
}

/* for ABI compatibility */
void xml_parser_init(const char * buf, int size, int mode) {
  if (static_xml_parser) {
    xml_parser_finalize_r(static_xml_parser);
  }
  static_xml_parser = xml_parser_init_r(buf, size, mode);
}

xml_parser_t *xml_parser_init_r(const char * buf, int size, int mode) {
  xml_parser_t *xml_parser = malloc(sizeof(*xml_parser));
  xml_parser->lexer = lexer_init_r(buf, size);
  xml_parser->mode = mode;
  return xml_parser;
}

void xml_parser_finalize_r(xml_parser_t *xml_parser) {
  lexer_finalize_r(xml_parser->lexer);
  free(xml_parser);
}

static void xml_parser_free_props(xml_property_t *current_property) {
  if (current_property) {
    if (!current_property->next) {
      free_xml_property(current_property);
    } else {
      xml_parser_free_props(current_property->next);
      free_xml_property(current_property);
    }
  }
}

static void xml_parser_free_tree_rec(xml_node_t *current_node, int free_next) {
  lprintf("xml_parser_free_tree_rec: %s\n", current_node->name);

  if (current_node) {
    /* properties */
    if (current_node->props) {
      xml_parser_free_props(current_node->props);
    }

    /* child nodes */
    if (current_node->child) {
      lprintf("xml_parser_free_tree_rec: child\n");
      xml_parser_free_tree_rec(current_node->child, 1);
    }

    /* next nodes */
    if (free_next) {
      xml_node_t *next_node = current_node->next;
      xml_node_t *next_next_node;

      while (next_node) {
        next_next_node = next_node->next;
        lprintf("xml_parser_free_tree_rec: next\n");
        xml_parser_free_tree_rec(next_node, 0);
        next_node = next_next_node;
      }
    }

    free_xml_node(current_node);
  }
}

void xml_parser_free_tree(xml_node_t *current_node) {
  lprintf("xml_parser_free_tree\n");
   xml_parser_free_tree_rec(current_node, 1);
}

typedef enum {
  /*0*/
  STATE_IDLE,
  /* <foo ...> */
  STATE_NODE,
  STATE_ATTRIBUTE,
  STATE_NODE_CLOSE,
  STATE_TAG_TERM,
  STATE_ATTRIBUTE_EQUALS,
  STATE_STRING,
  STATE_TAG_TERM_IGNORE,
  /* <?foo ...?> */
  STATE_Q_NODE,
  STATE_Q_ATTRIBUTE,
  STATE_Q_NODE_CLOSE,
  STATE_Q_TAG_TERM,
  STATE_Q_ATTRIBUTE_EQUALS,
  STATE_Q_STRING,
  /* Others */
  STATE_COMMENT,
  STATE_DOCTYPE,
  STATE_CDATA,
} parser_state_t;

static xml_node_t *xml_parser_append_text (xml_node_t *node, xml_node_t *subnode, const char *text, int flags)
{
  if (!text || !*text)
    return subnode; /* empty string -> nothing to do */

  if ((flags & XML_PARSER_MULTI_TEXT) && subnode) {
    /* we have a subtree, so we can't use node->data */
    if (subnode->name == cdata) {
      /* most recent node is CDATA - append to it */
      char *newtext;
      asprintf (&newtext, "%s%s", subnode->data, text);
      free (subnode->data);
      subnode->data = newtext;
    } else {
      /* most recent node is not CDATA - add a sibling */
      subnode->next = new_xml_node ();
      subnode->next->name = cdata;
      subnode->next->data = strdup (text);
      subnode = subnode->next;
    }
  } else if (node->data) {
    /* "no" subtree, but we have existing text - append to it */
    char *newtext;
    asprintf (&newtext, "%s%s", node->data, text);
    free (node->data);
    node->data = newtext;
  } else {
    /* no text, "no" subtree - duplicate & assign */
    while (isspace (*text))
      ++text;
    if (*text)
      node->data = strdup (text);
  }

  return subnode;
}

#define Q_STATE(CURRENT,NEW) (STATE_##NEW + state - STATE_##CURRENT)


static int xml_parser_get_node_internal (xml_parser_t *xml_parser,
				 char ** token_buffer, int * token_buffer_size,
                                 char ** pname_buffer, int * pname_buffer_size,
                                 char ** nname_buffer, int * nname_buffer_size,
                                 xml_node_t *current_node, char *root_names[], int rec, int flags)
{
  char *tok = *token_buffer;
  char *property_name = *pname_buffer;
  char *node_name = *nname_buffer;
  parser_state_t state = STATE_IDLE;
  int res = 0;
  int parse_res;
  int bypass_get_token = 0;
  int retval = 0; /* used when state==4; non-0 if there are missing </...> */
  xml_node_t *subtree = NULL;
  xml_node_t *current_subtree = NULL;
  xml_property_t *current_property = NULL;
  xml_property_t *properties = NULL;

  if (rec < MAX_RECURSION) {

    memset (tok, 0, *token_buffer_size);

    while ((bypass_get_token) || (res = lexer_get_token_d_r(xml_parser->lexer, token_buffer, token_buffer_size, 0)) != T_ERROR) {
      tok = *token_buffer;
      bypass_get_token = 0;
      lprintf("info: %d - %d : '%s'\n", state, res, tok);

      switch (state) {
      case STATE_IDLE:
	switch (res) {
	case (T_EOL):
	case (T_SEPAR):
	  /* do nothing */
	  break;
	case (T_EOF):
	  return retval; /* normal end */
	  break;
	case (T_M_START_1):
	  state = STATE_NODE;
	  break;
	case (T_M_START_2):
	  state = STATE_NODE_CLOSE;
	  break;
	case (T_C_START):
	  state = STATE_COMMENT;
	  break;
	case (T_TI_START):
	  state = STATE_Q_NODE;
	  break;
	case (T_DOCTYPE_START):
	  state = STATE_DOCTYPE;
	  break;
	case (T_CDATA_START):
	  state = STATE_CDATA;
	  break;
	case (T_DATA):
	  /* current data */
	  {
	    char *decoded = lexer_decode_entities (tok);
	    current_subtree = xml_parser_append_text (current_node, current_subtree, decoded, flags);
	    free (decoded);
	  }
	  lprintf("info: node data : %s\n", current_node->data);
	  break;
	default:
	  lprintf("error: unexpected token \"%s\", state %d\n", tok, state);
	  return -1;
	  break;
	}
	break;

      case STATE_NODE:
      case STATE_Q_NODE:
	switch (res) {
	case (T_IDENT):
	  properties = NULL;
	  current_property = NULL;

	  /* save node name */
	  if (xml_parser->mode == XML_PARSER_CASE_INSENSITIVE) {
	    strtoupper(tok);
	  }
	  if (state == STATE_Q_NODE) {
	    asprintf (&node_name, "?%s", tok);
	    free (*nname_buffer);
	    *nname_buffer = node_name;
	    *nname_buffer_size = strlen (node_name) + 1;
	    state = STATE_Q_ATTRIBUTE;
	  } else {
	    free (*nname_buffer);
	    *nname_buffer = node_name = strdup (tok);
	    *nname_buffer_size = strlen (node_name) + 1;
	    state = STATE_ATTRIBUTE;
	  }
	  lprintf("info: current node name \"%s\"\n", node_name);
	  break;
	default:
	  lprintf("error: unexpected token \"%s\", state %d\n", tok, state);
	  return -1;
	  break;
	}
	break;

      case STATE_ATTRIBUTE:
	switch (res) {
	case (T_EOL):
	case (T_SEPAR):
	  /* nothing */
	  break;
	case (T_M_STOP_1):
	  /* new subtree */
	  subtree = new_xml_node();

	  /* set node name */
	  subtree->name = strdup(node_name);

	  /* set node propertys */
	  subtree->props = properties;
	  lprintf("info: rec %d new subtree %s\n", rec, node_name);
	  root_names[rec + 1] = strdup (node_name);
	  parse_res = xml_parser_get_node_internal (xml_parser, token_buffer, token_buffer_size,
						    pname_buffer, pname_buffer_size,
						    nname_buffer, nname_buffer_size,
						    subtree, root_names, rec + 1, flags);
	  tok = *token_buffer;
	  free (root_names[rec + 1]);
	  if (parse_res == -1 || parse_res > 0) {
	    return parse_res;
	  }
	  if (current_subtree == NULL) {
	    current_node->child = subtree;
	    current_subtree = subtree;
	  } else {
	    current_subtree->next = subtree;
	    current_subtree = subtree;
	  }
	  if (parse_res < -1) {
	    /* badly-formed XML (missing close tag) */
	    return parse_res + 1 + (parse_res == -2);
	  }
	  state = STATE_IDLE;
	  break;
	case (T_M_STOP_2):
	  /* new leaf */
	  /* new subtree */
	  new_leaf:
	  subtree = new_xml_node();

	  /* set node name */
	  subtree->name = strdup (node_name);

	  /* set node propertys */
	  subtree->props = properties;

	  lprintf("info: rec %d new subtree %s\n", rec, node_name);

	  if (current_subtree == NULL) {
	    current_node->child = subtree;
	    current_subtree = subtree;
	  } else {
	    current_subtree->next = subtree;
	    current_subtree = subtree;
	  }
	  state = STATE_IDLE;
	  break;
	case (T_IDENT):
	  /* save property name */
	  new_prop:
	  if (xml_parser->mode == XML_PARSER_CASE_INSENSITIVE) {
	    strtoupper(tok);
	  }
	  /* make sure the buffer for the property name is big enough */
	  if (*token_buffer_size > *pname_buffer_size) {
	    char *tmp_prop;
	    *pname_buffer_size = *token_buffer_size;
	    tmp_prop = realloc (*pname_buffer, *pname_buffer_size);
	    if (!tmp_prop)
	      return -1;
	    *pname_buffer = tmp_prop;
	    property_name = tmp_prop;
	  } else {
	    property_name = *pname_buffer;
	  }
	  strcpy(property_name, tok);
	  state = Q_STATE(ATTRIBUTE, ATTRIBUTE_EQUALS);
	  lprintf("info: current property name \"%s\"\n", property_name);
	  break;
	default:
	  lprintf("error: unexpected token \"%s\", state %d\n", tok, state);
	  return -1;
	  break;
	}
	break;

      case STATE_Q_ATTRIBUTE:
	switch (res) {
	case (T_EOL):
	case (T_SEPAR):
	  /* nothing */
	  break;
	case (T_TI_STOP):
	  goto new_leaf;
	case (T_IDENT):
	  goto new_prop;
	default:
	  lprintf("error: unexpected token \"%s\", state %d\n", tok, state);
	  return -1;
	  break;
	}
	break;

      case STATE_NODE_CLOSE:
	switch (res) {
	case (T_IDENT):
	  /* must be equal to root_name */
	  if (xml_parser->mode == XML_PARSER_CASE_INSENSITIVE) {
	    strtoupper(tok);
	  }
	  if (strcmp(tok, root_names[rec]) == 0) {
	    state = STATE_TAG_TERM;
	  } else if (flags & XML_PARSER_RELAXED) {
	    int r = rec;
	    while (--r >= 0)
	      if (strcmp(tok, root_names[r]) == 0) {
		lprintf("warning: wanted %s, got %s - assuming missing close tags\n", root_names[rec], tok);
		retval = r - rec - 1; /* -1 - (no. of implied close tags) */
		state = STATE_TAG_TERM;
		break;
	      }
	    /* relaxed parsing, ignoring extra close tag (but we don't handle out-of-order) */
	    if (r < 0) {
	      lprintf("warning: extra close tag %s - ignoring\n", tok);
	      state = STATE_TAG_TERM_IGNORE;
	    }
	  }
	  else
	  {
	    lprintf("error: xml struct, tok=%s, waited_tok=%s\n", tok, root_names[rec]);
	    return -1;
	  }
	  break;
	default:
	  lprintf("error: unexpected token \"%s\", state %d\n", tok, state);
	  return -1;
	  break;
	}
	break;

				/* > expected */
      case STATE_TAG_TERM:
	switch (res) {
	case (T_M_STOP_1):
	  return retval;
	  break;
	default:
	  lprintf("error: unexpected token \"%s\", state %d\n", tok, state);
	  return -1;
	  break;
	}
	break;

				/* = or > or ident or separator expected */
      case STATE_ATTRIBUTE_EQUALS:
	switch (res) {
	case (T_EOL):
	case (T_SEPAR):
	  /* do nothing */
	  break;
	case (T_EQUAL):
	  state = STATE_STRING;
	  break;
	case (T_IDENT):
	  bypass_get_token = 1; /* jump to state 2 without get a new token */
	  state = STATE_ATTRIBUTE;
	  break;
	case (T_M_STOP_1):
	  /* add a new property without value */
	  if (current_property == NULL) {
	    properties = new_xml_property();
	    current_property = properties;
	  } else {
	    current_property->next = new_xml_property();
	    current_property = current_property->next;
	  }
	  current_property->name = strdup (property_name);
	  lprintf("info: new property %s\n", current_property->name);
	  bypass_get_token = 1; /* jump to state 2 without get a new token */
	  state = STATE_ATTRIBUTE;
	  break;
	default:
	  lprintf("error: unexpected token \"%s\", state %d\n", tok, state);
	  return -1;
	  break;
	}
	break;

				/* = or ?> or ident or separator expected */
      case STATE_Q_ATTRIBUTE_EQUALS:
	switch (res) {
	case (T_EOL):
	case (T_SEPAR):
	  /* do nothing */
	  break;
	case (T_EQUAL):
	  state = STATE_Q_STRING;
	  break;
	case (T_IDENT):
	  bypass_get_token = 1; /* jump to state 2 without get a new token */
	  state = STATE_Q_ATTRIBUTE;
	  break;
	case (T_TI_STOP):
	  /* add a new property without value */
	  if (current_property == NULL) {
	    properties = new_xml_property();
	    current_property = properties;
	  } else {
	    current_property->next = new_xml_property();
	    current_property = current_property->next;
	  }
	  current_property->name = strdup (property_name);
	  lprintf("info: new property %s\n", current_property->name);
	  bypass_get_token = 1; /* jump to state 2 without get a new token */
	  state = STATE_Q_ATTRIBUTE;
	  break;
	default:
	  lprintf("error: unexpected token \"%s\", state %d\n", tok, state);
	  return -1;
	  break;
	}
	break;

				/* string or ident or separator expected */
      case STATE_STRING:
      case STATE_Q_STRING:
	switch (res) {
	case (T_EOL):
	case (T_SEPAR):
	  /* do nothing */
	  break;
	case (T_STRING):
	case (T_IDENT):
	  /* add a new property */
	  if (current_property == NULL) {
	    properties = new_xml_property();
	    current_property = properties;
	  } else {
	    current_property->next = new_xml_property();
	    current_property = current_property->next;
	  }
	  current_property->name = strdup(property_name);
	  current_property->value = lexer_decode_entities(tok);
	  lprintf("info: new property %s=%s\n", current_property->name, current_property->value);
	  state = Q_STATE(STRING, ATTRIBUTE);
	  break;
	default:
	  lprintf("error: unexpected token \"%s\", state %d\n", tok, state);
	  return -1;
	  break;
	}
	break;

				/* --> expected */
      case STATE_COMMENT:
	switch (res) {
	case (T_C_STOP):
	  state = STATE_IDLE;
	  break;
	default:
	  break;
	}
	break;

				/* > expected */
      case STATE_DOCTYPE:
	switch (res) {
	case (T_M_STOP_1):
	  state = 0;
	  break;
	default:
	  break;
	}
	break;

				/* ]]> expected */
      case STATE_CDATA:
	switch (res) {
	case (T_CDATA_STOP):
	  current_subtree = xml_parser_append_text (current_node, current_subtree, tok, flags);
	  lprintf("info: node cdata : %s\n", tok);
	  state = STATE_IDLE;
	  break;
        default:
	  lprintf("error: unexpected token \"%s\", state %d\n", tok, state);
	  return -1;
	  break;
	}
	break;

				/* > expected (following unmatched "</...") */
      case STATE_TAG_TERM_IGNORE:
	switch (res) {
	case (T_M_STOP_1):
	  state = STATE_IDLE;
	  break;
	default:
	  lprintf("error: unexpected token \"%s\", state %d\n", tok, state);
	  return -1;
	  break;
	}
	break;


      default:
	lprintf("error: unknown parser state, state=%d\n", state);
	return -1;
      }
    }
    /* lex error */
    lprintf("error: lexer error\n");
    return -1;
  } else {
    /* max recursion */
    lprintf("error: max recursion\n");
    return -1;
  }
}

static int xml_parser_get_node (xml_parser_t *xml_parser, xml_node_t *current_node, int flags)
{
  int res = 0;
  int token_buffer_size = TOKEN_SIZE;
  int pname_buffer_size = TOKEN_SIZE;
  int nname_buffer_size = TOKEN_SIZE;
  char *token_buffer = calloc(1, token_buffer_size);
  char *pname_buffer = calloc(1, pname_buffer_size);
  char *nname_buffer = calloc(1, nname_buffer_size);
  char *root_names[MAX_RECURSION + 1];
  root_names[0] = "";

  res = xml_parser_get_node_internal (xml_parser,
			     &token_buffer, &token_buffer_size,
                             &pname_buffer, &pname_buffer_size,
                             &nname_buffer, &nname_buffer_size,
                             current_node, root_names, 0, flags);

  free (token_buffer);
  free (pname_buffer);
  free (nname_buffer);

  return res;
}

/* for ABI compatibility */
int xml_parser_build_tree_with_options(xml_node_t **root_node, int flags) {
  return xml_parser_build_tree_with_options_r(static_xml_parser, root_node, flags);
}

int xml_parser_build_tree_with_options_r(xml_parser_t *xml_parser, xml_node_t **root_node, int flags) {
  xml_node_t *tmp_node, *pri_node, *q_node;
  int res;

  tmp_node = new_xml_node();
  res = xml_parser_get_node(xml_parser, tmp_node, flags);

  /* delete any top-level [CDATA] nodes */;
  pri_node = tmp_node->child;
  q_node = NULL;
  while (pri_node) {
    if (pri_node->name == cdata) {
      xml_node_t *old = pri_node;
      if (q_node)
        q_node->next = pri_node->next;
      else
        q_node = pri_node;
      pri_node = pri_node->next;
      free_xml_node (old);
    } else {
      q_node = pri_node;
      pri_node = pri_node->next;
    }
  }

  /* find first non-<?...?> node */;
  for (pri_node = tmp_node->child, q_node = NULL;
       pri_node && pri_node->name[0] == '?';
       pri_node = pri_node->next)
    q_node = pri_node; /* last <?...?> node (eventually), or NULL */

  if (pri_node && !pri_node->next) {
    /* move the tail to the head (for compatibility reasons) */
    if (q_node) {
      pri_node->next = tmp_node->child;
      q_node->next = NULL;
    }
    *root_node = pri_node;
    free_xml_node(tmp_node);
    res = 0;
  } else {
    lprintf("error: xml struct\n");
    xml_parser_free_tree(tmp_node);
    res = -1;
  }
  return res;
}

/* for ABI compatibility */
int xml_parser_build_tree(xml_node_t **root_node) {
  return xml_parser_build_tree_with_options_r (static_xml_parser, root_node, 0);
}

int xml_parser_build_tree_r(xml_parser_t *xml_parser, xml_node_t **root_node) {
  return xml_parser_build_tree_with_options_r(xml_parser, root_node, 0);
}

const char *xml_parser_get_property (const xml_node_t *node, const char *name) {

  xml_property_t *prop;

  prop = node->props;
  while (prop) {

    lprintf ("looking for %s in %s\n", name, prop->name);

    if (!strcasecmp (prop->name, name)) {
      lprintf ("found it. value=%s\n", prop->value);
      return prop->value;
    }

    prop = prop->next;
  }

  return NULL;
}

int xml_parser_get_property_int (const xml_node_t *node, const char *name,
				 int def_value) {

  const char *v;
  int         ret;

  v = xml_parser_get_property (node, name);

  if (!v)
    return def_value;

  if (sscanf (v, "%d", &ret) != 1)
    return def_value;
  else
    return ret;
}

int xml_parser_get_property_bool (const xml_node_t *node, const char *name,
				  int def_value) {

  const char *v;

  v = xml_parser_get_property (node, name);

  if (!v)
    return def_value;

  return !strcasecmp (v, "true");
}

static int xml_escape_string_internal (char *buf, const char *s,
				       xml_escape_quote_t quote_type)
{
  int c, length = 0;
  int sl = buf ? 8 : 0;
  /* calculate max required buffer size */
  while ((c = *s++ & 0xFF))
    switch (c)
    {
    case '"':  if (quote_type != XML_ESCAPE_DOUBLE_QUOTE) goto literal;
	       length += snprintf (buf + length, sl, "&quot;"); break;
    case '\'': if (quote_type != XML_ESCAPE_SINGLE_QUOTE) goto literal;
	       length += snprintf (buf + length, sl, "&apos;"); break;
    case '&':  length += snprintf (buf + length, sl, "&amp;");  break;
    case '<':  length += snprintf (buf + length, sl, "&lt;");   break;
    case '>':  length += snprintf (buf + length, sl, "&gt;");   break;
    case 127:  length += snprintf (buf + length, sl, "&#127;"); break;
    case '\t':
    case '\n':
      literal: if (buf)	buf[length] = c; ++length; break;
    default:   if (c >= ' ') goto literal;
	       length += snprintf (buf + length, sl, "&#%d;", c); break;
    }
  if (buf)
    buf[length] = 0;
  return length + 1;
}

char *xml_escape_string (const char *s, xml_escape_quote_t quote_type)
{
  char *buf = calloc (1, xml_escape_string_internal (NULL, s, quote_type));
  return buf ? (xml_escape_string_internal (buf, s, quote_type), buf) : NULL;
}

static void xml_parser_dump_node (const xml_node_t *node, int indent) {
  size_t l;

  xml_property_t *p;
  xml_node_t     *n;

  printf ("%*s<%s ", indent, "", node->name);

  l = strlen (node->name);

  p = node->props;
  while (p) {
    char *value = xml_escape_string (p->value, XML_ESCAPE_SINGLE_QUOTE);
    printf ("%s='%s'", p->name, value);
    free (value);
    p = p->next;
    if (p) {
      printf ("\n%*s", indent+2+l, "");
    }
  }
  printf (">\n");

  n = node->child;
  while (n) {

    xml_parser_dump_node (n, indent+2);

    n = n->next;
  }

  printf ("%*s</%s>\n", indent, "", node->name);
}

void xml_parser_dump_tree (const xml_node_t *node) {
  do {
    xml_parser_dump_node (node, 0);
    node = node->next;
  } while (node);
}

#ifdef XINE_XML_PARSER_TEST
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void *xine_xmalloc (size_t size)
{
  return malloc (size);
}

int main (int argc, char **argv)
{
  int i, ret = 0;
  for (i = 1; argv[i]; ++i)
  {
    xml_node_t *tree;
    int fd;
    void *buf;
    struct stat st;

    if (stat (argv[i], &st))
    {
      perror (argv[i]);
      ret = 1;
      continue;
    }
    if (!S_ISREG (st.st_mode))
    {
      printf ("%s: not a file\n", argv[i]);
      ret = 1;
      continue;
    }
    fd = open (argv[i], O_RDONLY);
    if (!fd)
    {
      perror (argv[i]);
      ret = 1;
      continue;
    }
    buf = mmap (NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (!buf)
    {
      perror (argv[i]);
      if (close (fd))
        perror (argv[i]);
      ret = 1;
      continue;
    }

    xml_parser_init (buf, st.st_size, 0);
    if (!xml_parser_build_tree (&tree))
    {
      puts (argv[i]);
      xml_parser_dump_tree (tree);
      xml_parser_free_tree (tree);
    }
    else
      printf ("%s: parser failure\n", argv[i]);

    if (close (fd))
    {
      perror (argv[i]);
      ret = 1;
    }
  }
  return ret;
}
#endif
