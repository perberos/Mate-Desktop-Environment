/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-ui-node.c: Code to manipulate MateComponentUINode objects
 *                   lightweight, cutdown XML node representations
 *
 * Authors:
 *      Michael Meeks    <michael@ximian.com>
 *	Havoc Pennington <hp@redhat.com>
 *
 * Copyright 2000 Red Hat, Inc.
 *           2001 Ximian, Inc.
 */
#include <config.h>
#include <string.h>
#include <matecomponent/matecomponent-ui-node.h>
#include <matecomponent/matecomponent-ui-node-private.h>

#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#include <libxml/xmlmemory.h>

#if LIBXML_VERSION < 20212
#	error Something extremely stupid has happened
#endif

#define attr(n,i) g_array_index((n)->attrs, MateComponentUIAttr, (i))

/**
 * matecomponent_ui_node_new:
 * @name: The name for the node
 * 
 * Creates a new node with name @name
 * 
 * Return value: a new node pointer
 **/
MateComponentUINode*
matecomponent_ui_node_new (const char *name)
{
	MateComponentUINode *node = g_new0 (MateComponentUINode, 1);

	node->name_id = g_quark_from_string (name);
	node->ref_count = 1;

	/* FIXME: we could do this idly */
	node->attrs = g_array_new (FALSE, FALSE, sizeof (MateComponentUIAttr));

	return node;
}

/**
 * matecomponent_ui_node_new_child:
 * @parent: the parent
 * @name: the name of the new child
 * 
 * Create a new node as a child of @parent with name @name
 * 
 * Return value: pointer to the new child
 **/
MateComponentUINode*
matecomponent_ui_node_new_child (MateComponentUINode *parent,
                          const char   *name)
{
	MateComponentUINode *node;

	node = matecomponent_ui_node_new (name);

	matecomponent_ui_node_add_child (parent, node);

	return node;
}

/**
 * matecomponent_ui_node_copy:
 * @node: the node
 * @recursive: whether to dup children too.
 * 
 * Copy an XML node, if @recursive do a deep copy, otherwise just dup the node itself.
 * 
 * Return value: a copy of the noce
 **/
MateComponentUINode*
matecomponent_ui_node_copy (MateComponentUINode *node,
                     gboolean recursive)
{
	MateComponentUINode *copy;

	copy = g_new0 (MateComponentUINode, 1);
	copy->ref_count = 1;
	copy->name_id = node->name_id;

	copy->content = g_strdup (node->content);

	matecomponent_ui_node_copy_attrs (node, copy);

	if (recursive) {
		MateComponentUINode *l, *last = NULL;

		for (l = node->children; l; l = l->next) {
			MateComponentUINode *child;
			
			child = matecomponent_ui_node_copy (l, TRUE);

			if (!last)
				copy->children = child;
			else {
				child->prev = last;
				last->next  = child;
			}
			last = child;
		}
	}

	return copy;
}

/**
 * matecomponent_ui_node_add_child:
 * @parent: the parent
 * @child: the new child
 * 
 * Add a @child node to the @parent node ( after the other children )
 **/
void
matecomponent_ui_node_add_child (MateComponentUINode *parent,
			  MateComponentUINode *child)
{
	MateComponentUINode *l, *last = NULL;

	for (l = parent->children; l; l = l->next)
		last = l;

	child->prev = last;
	child->next = NULL;
	if (!last)
		parent->children = child;
	else
		last->next  = child;
	child->parent = parent;
}

void
matecomponent_ui_node_add_after (MateComponentUINode *before,
			  MateComponentUINode *new_after)
{
	new_after->next = before->next;
	new_after->prev = before;

	if (new_after->next)
		new_after->next->prev = new_after;

	before->next = new_after;

	new_after->parent = before->parent;
}

/**
 * matecomponent_ui_node_insert_before:
 * @after: the placeholder for insertion
 * @new_before: the node to insert
 * 
 * Insert a @sibling before @prev_sibling in a node list
 **/
void
matecomponent_ui_node_insert_before (MateComponentUINode *after,
			      MateComponentUINode *new_before)
{
	matecomponent_ui_node_unlink (new_before);

	new_before->prev = after->prev;

	if (after->prev) {
		after->prev->next = new_before;
	} else {
		if (after->parent)
			after->parent->children = new_before;
	}
	new_before->next = after;
	after->prev = new_before;

	new_before->parent = after->parent;
}

/**
 * matecomponent_ui_node_unlink:
 * @node: the node
 * 
 * Unlink @node from its tree, ie. disassociate it with its parent
 **/
void
matecomponent_ui_node_unlink (MateComponentUINode *node)
{
	if (!node)
		return;

	if (!node->prev) {
		if (node->parent)
			node->parent->children = node->next;
	} else
		node->prev->next = node->next;

	if (node->next)
		node->next->prev = node->prev;

	node->next   = NULL;
	node->prev   = NULL;
	node->parent = NULL;
}

/**
 * matecomponent_ui_node_replace:
 * @old_node: node to be replaced
 * @new_node: node to replace with
 * 
 * Replace @old_node with @new_node in the tree. @old_node is
 * left unlinked and floating with its children.
 **/
void
matecomponent_ui_node_replace (MateComponentUINode *old_node,
			MateComponentUINode *new_node)
{
	matecomponent_ui_node_unlink (new_node);

	new_node->next   = old_node->next;
	new_node->prev   = old_node->prev;
	new_node->parent = old_node->parent;

	old_node->next   = NULL;
	old_node->prev   = NULL;
	old_node->parent = NULL;

	if (new_node->next)
		new_node->next->prev = new_node;

	if (new_node->prev)
		new_node->prev->next = new_node;
	else {
		if (new_node->parent)
			new_node->parent->children = new_node;
	}
}

void
matecomponent_ui_node_move_children (MateComponentUINode *from, MateComponentUINode *to)
{
	MateComponentUINode *l;

	g_return_if_fail (to != NULL);
	g_return_if_fail (from != NULL);
	g_return_if_fail (matecomponent_ui_node_children (to) == NULL);

	to->children   = from->children;
	from->children = NULL;

	for (l = to->children; l; l = l->next)
		l->parent = to;
}

static void
node_free_attrs (MateComponentUINode *node)
{
	int    i;

	for (i = 0; i < node->attrs->len; i++)
		if (attr (node, i).value)
			xmlFree (attr (node, i).value);

	g_array_free (node->attrs, TRUE);
}

static void
node_free_internal (MateComponentUINode *node)
{
	MateComponentUINode *l, *next;

	g_return_if_fail (node->ref_count >= 0);

	if (node->parent || node->next || node->prev)
		matecomponent_ui_node_unlink (node);

	node_free_attrs (node);

	g_free (node->content);

	for (l = node->children; l; l = next) {
		next = l->next;
		matecomponent_ui_node_unlink (l);
		matecomponent_ui_node_unref (l);
	}

	g_free (node);
}

void
matecomponent_ui_node_unref (MateComponentUINode *node)
{
	if (--node->ref_count <= 0) 
		node_free_internal (node);
}

MateComponentUINode *
matecomponent_ui_node_ref (MateComponentUINode *node)
{
	node->ref_count++;
	return node;
}

/**
 * matecomponent_ui_node_free:
 * @node: a node.
 * 
 * Frees the memory associated with the @node and unlink it from the tree
 **/
void
matecomponent_ui_node_free (MateComponentUINode *node)
{
	if (node->ref_count > 1)
		g_warning ("Freeing referenced node %p", node);

	matecomponent_ui_node_unref (node);
}


/**
 * matecomponent_ui_node_set_data:
 * @node: the node
 * @data: user data
 * 
 * Associates some user data with the node pointer
 **/
void
matecomponent_ui_node_set_data (MateComponentUINode *node,
                         gpointer      data)
{
	node->user_data = data;
}

/**
 * matecomponent_ui_node_get_data:
 * @node: the node
 * 
 * Gets user data associated with @node
 * 
 * Return value: the user data, see matecomponent_ui_node_set_data
 **/
gpointer
matecomponent_ui_node_get_data (MateComponentUINode *node)
{
	return node->user_data;
}

static MateComponentUIAttr *
get_attr (MateComponentUINode *node, GQuark  id, MateComponentUIAttr **opt_space)
{
	int i;
	MateComponentUIAttr *a;

	if (opt_space)
		*opt_space = NULL;

	for (i = 0; i < node->attrs->len; i++) {
		a = &attr (node, i);

		if (a->id == id)
			return a;

		if (a->id == 0 && opt_space)
			*opt_space = a;
	}

	return NULL;
}

static inline gboolean
do_set_attr (MateComponentUINode *node,
	     GQuark        id,
	     const char   *value)
{
	gboolean different = TRUE;
	MateComponentUIAttr *a, *space;

	g_return_val_if_fail (node != NULL, FALSE);

	a = get_attr (node, id, &space);

	if (a) {
		different = !value || strcmp (a->value, value);

		if (different) {
			xmlFree (a->value);
			a->value = NULL;
			
			if (!value) /* Unset the attribute */
				a->id = 0;
			else {
				a->value = xmlStrdup (value);
			}
		}
	} else {
		if (!value)
			return FALSE;

		if (space) {
			space->id = id;
			space->value = xmlStrdup (value);
		} else {
			MateComponentUIAttr na;

			na.id = id;
			na.value = xmlStrdup (value);

			g_array_append_val (node->attrs, na);
		}
	}

	return different;
}

void
matecomponent_ui_node_set_attr_by_id (MateComponentUINode *node,
			       GQuark        id,
			       const char   *value)
{
	do_set_attr (node, id, value);
}


gboolean
matecomponent_ui_node_try_set_attr (MateComponentUINode *node,
			     GQuark        prop,
			     const char   *value)
{
	return do_set_attr (node, prop, value);
}

/**
 * matecomponent_ui_node_set_attr:
 * @node: The node
 * @name: the name of the attr
 * @value: the value for the attr
 * 
 * Set the attribute of @name on @node to @value overriding any
 * previous values of that attr.
 **/
void
matecomponent_ui_node_set_attr (MateComponentUINode *node,
                         const char   *name,
                         const char   *value)
{
	matecomponent_ui_node_set_attr_by_id (
		node, g_quark_from_string (name), value);
}

const char *
matecomponent_ui_node_get_attr_by_id (MateComponentUINode *node,
			       GQuark        id)
{
	MateComponentUIAttr *a;

	if (!node)
		return NULL;
	
	a = get_attr (node, id, NULL);

	return a ? a->value : NULL;
}

/**
 * matecomponent_ui_node_get_attr:
 * @node: the node
 * @name: the name of the attr to get
 * 
 * Fetch the value of an attr of name @name from @node
 * see also: matecomponent_ui_node_free_string
 * 
 * Return value: the attr text.
 **/
char*
matecomponent_ui_node_get_attr (MateComponentUINode *node,
                         const char   *name)
{
	return xmlStrdup (
		matecomponent_ui_node_get_attr_by_id (
			node, g_quark_from_string (name)));
}

/**
 * matecomponent_ui_node_has_attr:
 * @node: the node
 * @name: the name of the attr to detect
 * 
 * Determines whether the @node has an attribute of name @name
 * 
 * Return value: TRUE if the attr exists
 **/
gboolean
matecomponent_ui_node_has_attr (MateComponentUINode *node,
                         const char   *name)
{
	return matecomponent_ui_node_get_attr_by_id (
		node, g_quark_from_string (name)) != NULL;
}

/**
 * matecomponent_ui_node_remove_attr:
 * @node: the node
 * @name: name of the attribute
 * 
 * remove any attribute with name @name from @node
 **/
void
matecomponent_ui_node_remove_attr (MateComponentUINode *node,
                            const char   *name)
{
	matecomponent_ui_node_set_attr (node, name, NULL);
}

/**
 * matecomponent_ui_node_set_content:
 * @node: the node
 * @content: the new content
 * 
 * Set the textual content of @node to @content
 **/
void
matecomponent_ui_node_set_content (MateComponentUINode *node,
                            const char   *content)
{
	g_free (node->content);
	node->content = g_strdup (content);
}

const char *
matecomponent_ui_node_peek_content (MateComponentUINode *node)
{
	return node->content;
}

/**
 * matecomponent_ui_node_get_content:
 * @node: the node
 * 
 * see also: matecomponent_ui_node_free_string
 *
 * Return value: the content of @node
 **/
char *
matecomponent_ui_node_get_content (MateComponentUINode *node)
{
	return xmlStrdup (matecomponent_ui_node_peek_content (node));
}

/**
 * matecomponent_ui_node_next:
 * @node: the node
 * 
 * Return value: the node after @node in the list
 **/
MateComponentUINode*
matecomponent_ui_node_next (MateComponentUINode *node)
{
	return node->next;
}

/**
 * matecomponent_ui_node_prev:
 * @node: the node
 * 
 * Return value: the node after @node in the list
 **/
MateComponentUINode*
matecomponent_ui_node_prev (MateComponentUINode *node)
{
	return node->prev;
}

/**
 * matecomponent_ui_node_children:
 * @node: the node
 * 
 * Return value: the first child of @node
 **/
MateComponentUINode*
matecomponent_ui_node_children (MateComponentUINode *node)
{
	return node->children;
}

/**
 * matecomponent_ui_node_parent:
 * @node: the node
 * 
 * Return value: the parent node of @node
 **/
MateComponentUINode*
matecomponent_ui_node_parent (MateComponentUINode *node)
{
	return node->parent;
}

/**
 * matecomponent_ui_node_get_name:
 * @node: the node
 * 
 * Return value: the name of @node
 **/
const char*
matecomponent_ui_node_get_name (MateComponentUINode *node)
{
	return g_quark_to_string (node->name_id);
}

gboolean
matecomponent_ui_node_has_name_by_id (MateComponentUINode *node,
			       GQuark        id)
{
	return (node->name_id == id);
}

/**
 * matecomponent_ui_node_has_name:
 * @node: the node
 * @name: a name the node might have
 * 
 * Return value: TRUE if @node has name == @name
 **/
gboolean
matecomponent_ui_node_has_name (MateComponentUINode *node,
			 const char   *name)
{
        return matecomponent_ui_node_has_name_by_id (
		node, g_quark_from_string (name));
}

/**
 * matecomponent_ui_node_free_string:
 * @str: the string to free.
 * 
 * Frees a string returned by any of the get routines.
 **/
void
matecomponent_ui_node_free_string (char *str)
{
        if (str)
                xmlFree (str);
}

/**
 * matecomponent_ui_node_transparent:
 * @node: the node
 * 
 * Determines whether @node is transparent. A node is
 * transparent if it has no content and either no attributes
 * or a single 'name' attribute.
 * 
 * Return value: TRUE if transparent
 **/
gboolean
matecomponent_ui_node_transparent (MateComponentUINode *node)
{
	gboolean ret = FALSE;
	static GQuark  name_id = 0;
	static GQuark  separator_id;

	g_return_val_if_fail (node != NULL, TRUE);

	if (!name_id) {
		name_id = g_quark_from_static_string ("name");
		/* FIXME: ugly to have specific widgets in here */
		separator_id = g_quark_from_static_string ("separator");
	}

	if (node->content)
		ret = FALSE;

	else if (node->attrs->len == 0)

		if (node->name_id == separator_id)
			ret = FALSE;
		else
			ret = TRUE;

	else if (node->attrs->len == 1 && attr (node, 0).id == name_id)
		ret = TRUE;

	return ret;
}

/**
 * matecomponent_ui_node_copy_attrs:
 * @src: the attr source node
 * @dest: where to dump the attrs.
 * 
 * This function copies all the attributes from @src to @dest
 * effectively cloning the @src node as @dest
 **/
void
matecomponent_ui_node_copy_attrs (const MateComponentUINode *src,
			   MateComponentUINode       *dest)
{
	int i;

	if (dest->attrs)
		node_free_attrs (dest);

	dest->attrs = g_array_new (FALSE, FALSE, sizeof (MateComponentUIAttr));
	g_array_set_size (dest->attrs, src->attrs->len);

	for (i = 0; i < src->attrs->len; i++) {
		MateComponentUIAttr *as = &attr (src, i);
		MateComponentUIAttr *ad = &attr (dest, i);
		
		ad->id    = as->id;
		ad->value = xmlStrdup (as->value);
	}
}

/**
 * matecomponent_ui_node_strip:
 * @node: a pointer to the node's pointer
 * 
 * A compat function for legacy reasons.
 **/
void
matecomponent_ui_node_strip (MateComponentUINode **node)
{
}


/* SAX Parser */

typedef struct {
	MateComponentUINode *root;

	MateComponentUINode *cur;

	GString      *content;
} ParseState;

static ParseState *
parse_state_new (void)
{
	ParseState *ps = g_new0 (ParseState, 1);

	ps->root = matecomponent_ui_node_new ("");
	ps->cur  = ps->root;

	ps->content = g_string_sized_new (0);

	return ps;
}

static MateComponentUINode *
parse_state_free (ParseState *ps, gboolean free_root)
{
	MateComponentUINode *ret = NULL;

	if (ps) {
		if (!free_root) {
			ret = ps->root->children;
			matecomponent_ui_node_unlink (ret);
		}

		matecomponent_ui_node_free (ps->root);

		g_string_free (ps->content, TRUE);

		g_free (ps);
	}

	return ret;
}

static void
uiStartDocument (ParseState *ps)
{
	ps->cur = ps->root;
}

static void
uiStartElement (ParseState     *ps,
		const xmlChar  *name,
		const xmlChar **attrs)
{
	int           i;
	MateComponentUINode *node;

	/* FIXME: we want to keep a stack of list end nodes
	   here for speed ... */

	node = matecomponent_ui_node_new_child (ps->cur, name);

	ps->cur = node;

	for (i = 0; attrs && attrs [i]; i++) {
		MateComponentUIAttr a;

		a.id = g_quark_from_string (attrs [i++]);
		a.value = xmlStrdup (attrs [i]);

		g_array_append_val (node->attrs, a);
	}
}

static void
uiEndElement (ParseState *ps, const xmlChar *name)
{
	if (ps->content->len > 0) {
		int   i;
		char *content = ps->content->str;

		for (i = 0; content [i] != '\0'; i++) {
			if (content [i] != ' ' &&
			    content [i] != '\t' &&
			    content [i] != '\n')
				break;
		}

		if (content [i] != '\0') {
			g_free (ps->cur->content);
			ps->cur->content = content;

			g_string_free (ps->content, FALSE);
		} else
			g_string_free (ps->content, TRUE);

		ps->content = g_string_sized_new (0);
	}

	ps->cur = ps->cur->parent;
}

static void
uiCharacters (ParseState *ps, const xmlChar *chars, int len)
{
	g_string_append_len (ps->content, chars, len);
}

static void
uiWarning (ParseState *ps, const char *msg, ...)
{
	va_list args;

	va_start (args, msg);
	g_logv   ("XML", G_LOG_LEVEL_WARNING, msg, args);
	va_end   (args);
}

static void
uiError (ParseState *ps, const char *msg, ...)
{
	va_list args;

	va_start (args, msg);
	g_logv   ("XML", G_LOG_LEVEL_CRITICAL, msg, args);
	va_end   (args);
}

static void
uiFatalError (ParseState *ps, const char *msg, ...)
{
	va_list args;

	va_start (args, msg);
	g_logv   ("XML", G_LOG_LEVEL_ERROR, msg, args);
	va_end   (args);
}

static xmlSAXHandler matecomponentSAXParser = {
	NULL, /* internalSubset */
	NULL, /* isStandalone */
	NULL, /* hasInternalSubset */
	NULL, /* hasExternalSubset */
	NULL, /* resolveEntity */
	NULL, /* getEntity */
	NULL, /* entityDecl */
	NULL, /* notationDecl */
	NULL, /* attributeDecl */
	NULL, /* elementDecl */
	NULL, /* unparsedEntityDecl */
	NULL, /* setDocumentLocator */
	(startDocumentSAXFunc) uiStartDocument, /* startDocument */
	(endDocumentSAXFunc) NULL, /* endDocument */
	(startElementSAXFunc) uiStartElement, /* startElement */
	(endElementSAXFunc) uiEndElement, /* endElement */
	NULL, /* reference */
	(charactersSAXFunc) uiCharacters, /* characters */
	NULL, /* ignorableWhitespace */
	NULL, /* processingInstruction */
	NULL, /* comment */
	(warningSAXFunc) uiWarning, /* warning */
	(errorSAXFunc) uiError, /* error */
	(fatalErrorSAXFunc) uiFatalError, /* fatalError */
};

static MateComponentUINode *
do_parse (xmlParserCtxt *ctxt)
{
	int ret = 0;
	ParseState *ps;
	xmlSAXHandlerPtr oldsax;

	if (!ctxt)
		return NULL;

	ps = parse_state_new ();
    
	oldsax = ctxt->sax;
	ctxt->sax = &matecomponentSAXParser;
	ctxt->userData = ps;
	/* Magic to make entities work as expected */
	ctxt->replaceEntities = TRUE;

	xmlParseDocument (ctxt);
    
	if (ctxt->wellFormed)
		ret = 0;
	else {
		if (ctxt->errNo != 0)
			ret = ctxt->errNo;
		else
			ret = -1;
	}
	ctxt->sax = oldsax;
	xmlFreeParserCtxt (ctxt);

	if (ret < 0) {
		g_warning ("XML not well formed!");
		parse_state_free (ps, TRUE);
		return NULL;
	}

	return parse_state_free (ps, FALSE);
}

/**
 * matecomponent_ui_node_from_string:
 * @xml: the xml string
 * 
 * Parses a string into an XML tree
 * 
 * Return value: the xml tree.
 **/
MateComponentUINode*
matecomponent_ui_node_from_string (const char *xml)
{
	guint len;

	g_return_val_if_fail (xml != NULL, NULL);

	len = strlen (xml);
	if (len < 3)
		return NULL;

	return do_parse (xmlCreateMemoryParserCtxt ((char *) xml, len));
}

/**
 * matecomponent_ui_node_from_file:
 * @fname: the filename containing the xml
 * 
 * Loads and parses the filename into an XML tree
 * 
 * Return value: the xml tree.
 **/
MateComponentUINode*
matecomponent_ui_node_from_file (const char *fname)
{
	g_return_val_if_fail (fname != NULL, NULL);
	
	return do_parse (xmlCreateFileParserCtxt (fname));
}

/*
 * Cut and paste from gmarkup.c: what a hack.
 */
static void
append_escaped_text (GString     *str,
                     const gchar *text)
{
	const gchar *p;

	p = text;

	while (*p != '\0') {
		const gchar *next;
		next = g_utf8_next_char (p);

		switch (*p) {
		case '&':
			g_string_append (str, "&amp;");
			break;

		case '<':
			g_string_append (str, "&lt;");
			break;

		case '>':
			g_string_append (str, "&gt;");
			break;

		case '\'':
			g_string_append (str, "&apos;");
			break;

		case '"':
			g_string_append (str, "&quot;");
			break;

		default:
			g_string_append_len (str, p, next - p);
			break;
		}
		
		p = next;
	}
}

static void
internal_to_string (GString      *str,
		    MateComponentUINode *node,
		    gboolean      recurse)
{
	int         i;
	gboolean    contains;
	const char *tag_name;

	contains = node->content || (node->children && recurse);
	tag_name = g_quark_to_string (node->name_id);
	
	g_string_append_c (str, '<');
	g_string_append   (str, tag_name);

	for (i = 0; i < node->attrs->len; i++) {
		MateComponentUIAttr *a = &attr (node, i);

		if (!a->id)
			continue;

		g_string_append_c (str, ' ');
		g_string_append   (str, g_quark_to_string (a->id));
		g_string_append_c (str, '=');
		g_string_append_c (str, '\"');
		append_escaped_text (str, a->value);
		g_string_append_c (str, '\"');
	}

	if (contains) {
		g_string_append_c (str, '>');
		
		if (recurse && node->children) {
			MateComponentUINode *l;

			g_string_append (str, "\n");
			
			for (l = node->children; l; l = l->next)
				internal_to_string (str, l, recurse);
		}

		if (node->content)
			append_escaped_text (str, node->content);

		g_string_append   (str, "</");
		g_string_append   (str, tag_name);
		g_string_append   (str, ">\n");
	} else
		g_string_append   (str, "/>\n");
}

#if 0
static void
validate_tree (MateComponentUINode *node)
{
	MateComponentUINode *l, *last = NULL;

	if (!node)
		return;

	for (l = node->children; l; l = l->next) {

		if (l->parent != node)
			g_warning ("Parent chaining error on '%p' parent %p should be %p",
				   l, l->parent, node);

		if (l->prev != last)
			g_warning ("Previous chaining error on '%p' prev %p should be %p",
				   l, l->prev, last);
		validate_tree (l);
		
		last = l;
	}
}
#endif

/**
 * matecomponent_ui_node_to_string:
 * @node: the node tree
 * @recurse: whether to dump its children as well
 * 
 * Convert the node to its XML string representation.
 * 
 * Return value: the string representation or NULL on error
 * Use g_free to free.
 **/
char *
matecomponent_ui_node_to_string (MateComponentUINode *node,
			  gboolean      recurse)
{
	GString *str = g_string_sized_new (64);

	g_return_val_if_fail (node != NULL, NULL);

	internal_to_string (str, node, recurse);

/*	fprintf (stderr, "nodes to string: '%s'", str->str); */
/*	validate_tree (node); */

	return g_string_free (str, FALSE);
}

const char *
matecomponent_ui_node_peek_attr (MateComponentUINode *node,
			  const char   *name)
{
	GQuark id = g_quark_from_string (name);

	return matecomponent_ui_node_get_attr_by_id (node, id);
}

/**
 * matecomponent_ui_node_get_path_child:
 * @node: parent node
 * @name: 'name' of child node.
 * 
 *    Finds the child with the right name, based on
 * the normal path traversal naming rules.
 * 
 * Return value: the child node or NULL.
 **/
MateComponentUINode *
matecomponent_ui_node_get_path_child (MateComponentUINode *node,
			       const char   *name)
{
	MateComponentUINode *l;
	GQuark name_as_quark;
	static GQuark name_string_id = 0;

	name_as_quark = g_quark_try_string (name);

	if (!name_string_id)
		name_string_id = g_quark_from_static_string ("name");

	for (l = node->children; l; l = l->next) {
		MateComponentUIAttr *a;

		if ((a = get_attr (l, name_string_id, NULL)) && a->value &&
		    !strcmp (a->value, name))
			return l;

		if (l->name_id && l->name_id == name_as_quark)
			return l;
	}

	return NULL;
}
