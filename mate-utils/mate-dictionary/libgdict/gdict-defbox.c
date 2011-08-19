/* gdict-defbox.c - display widget for dictionary definitions
 *
 * Copyright (C) 2005-2006  Emmanuele Bassi <ebassi@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 */

/**
 * SECTION:gdict-defbox
 * @short_description: Display the list of definitions for a word
 *
 * The #GdictDefbox widget is a composite widget showing the list of
 * definitions for a word. It queries the passed #GdictContext and displays
 * the list of #GdictDefinition<!-- -->s obtained.
 *
 * It provides syntax highlighting, clickable links and an embedded find
 * bar.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>

#include "gdict-defbox.h"
#include "gdict-utils.h"
#include "gdict-debug.h"
#include "gdict-private.h"
#include "gdict-enum-types.h"
#include "gdict-marshal.h"

#define QUERY_MARGIN	48
#define ERROR_MARGIN	24

typedef struct
{
  GdictDefinition *definition;

  gint begin;
} Definition;

#define GDICT_DEFBOX_GET_PRIVATE(obj) \
(G_TYPE_INSTANCE_GET_PRIVATE ((obj), GDICT_TYPE_DEFBOX, GdictDefboxPrivate))

struct _GdictDefboxPrivate
{
  GtkWidget *text_view;
  
  /* the "find" pane */
  GtkWidget *find_pane;
  GtkWidget *find_entry;
  GtkWidget *find_next;
  GtkWidget *find_prev;
  GtkWidget *find_label;

  GtkWidget *progress_dialog;
  
  GtkTextBuffer *buffer;

  GdictContext *context;
  GSList *definitions;
  
  gchar *word;
  gchar *database;
  gchar *font_name;
  
  guint show_find : 1;
  guint is_searching : 1;
  guint is_hovering : 1;
  
  GdkCursor *busy_cursor;
  GdkCursor *hand_cursor;
  GdkCursor *regular_cursor;
  
  guint start_id;
  guint end_id;
  guint define_id;
  guint error_id;
  guint hide_timeout;

  GtkTextTag *link_tag;
  GtkTextTag *visited_link_tag;
};

enum
{
  PROP_0,
  
  PROP_CONTEXT,
  PROP_WORD,
  PROP_DATABASE,
  PROP_FONT_NAME,
  PROP_COUNT
};

enum
{
  SHOW_FIND,
  HIDE_FIND,
  FIND_NEXT,
  FIND_PREVIOUS,
  LINK_CLICKED,
  
  LAST_SIGNAL
};

static guint gdict_defbox_signals[LAST_SIGNAL] = { 0 };
static GdkColor default_link_color = { 0, 0, 0, 0xeeee };
static GdkColor default_visited_link_color = { 0, 0x5555, 0x1a1a, 0x8b8b };


G_DEFINE_TYPE (GdictDefbox, gdict_defbox, GTK_TYPE_VBOX);


static Definition *
definition_new (void)
{
  Definition *def;
  
  def = g_slice_new (Definition);
  def->definition = NULL;
  def->begin = -1;
  
  return def;
}

static void
definition_free (Definition *def)
{
  if (!def)
    return;
  
  gdict_definition_unref (def->definition);
  g_slice_free (Definition, def);
}

static void
gdict_defbox_dispose (GObject *gobject)
{
  GdictDefbox *defbox = GDICT_DEFBOX (gobject);
  GdictDefboxPrivate *priv = defbox->priv;

  if (priv->start_id)
    {
      g_signal_handler_disconnect (priv->context, priv->start_id);
      g_signal_handler_disconnect (priv->context, priv->end_id);
      g_signal_handler_disconnect (priv->context, priv->define_id);

      priv->start_id = 0;
      priv->end_id = 0;
      priv->define_id = 0;
    }

  if (priv->error_id)
    {
      g_signal_handler_disconnect (priv->context, priv->error_id);
      priv->error_id = 0;
    }

  if (priv->context)
    {
      g_object_unref (priv->context);
      priv->context = NULL;
    }

  if (priv->buffer)
    {
      g_object_unref (priv->buffer);
      priv->buffer = NULL;
    }

  if (priv->busy_cursor)
    {
      gdk_cursor_unref (priv->busy_cursor);
      priv->busy_cursor = NULL;
    }

  if (priv->hand_cursor)
    {
      gdk_cursor_unref (priv->hand_cursor);
      priv->hand_cursor = NULL;
    }

  if (priv->regular_cursor)
    {
      gdk_cursor_unref (priv->regular_cursor);
      priv->regular_cursor = NULL;
    }

  G_OBJECT_CLASS (gdict_defbox_parent_class)->dispose (gobject);
}

static void
gdict_defbox_finalize (GObject *object)
{
  GdictDefbox *defbox = GDICT_DEFBOX (object);
  GdictDefboxPrivate *priv = defbox->priv;

  g_free (priv->database);
  g_free (priv->word);
  g_free (priv->font_name);

  if (priv->definitions)
    {
      g_slist_foreach (priv->definitions, (GFunc) definition_free, NULL);
      g_slist_free (priv->definitions);

      priv->definitions = NULL;
    }

  G_OBJECT_CLASS (gdict_defbox_parent_class)->finalize (object);
}

static void
set_gdict_context (GdictDefbox  *defbox,
		   GdictContext *context)
{
  GdictDefboxPrivate *priv;
  
  g_assert (GDICT_IS_DEFBOX (defbox));
  
  priv = defbox->priv;
  if (priv->context)
    {
      if (priv->start_id)
        {
          GDICT_NOTE (DEFBOX, "Removing old context handlers");
          
          g_signal_handler_disconnect (priv->context, priv->start_id);
          g_signal_handler_disconnect (priv->context, priv->define_id);
          g_signal_handler_disconnect (priv->context, priv->end_id);
          
          priv->start_id = 0;
          priv->end_id = 0;
          priv->define_id = 0;
        }
      
      if (priv->error_id)
        {
          g_signal_handler_disconnect (priv->context, priv->error_id);

          priv->error_id = 0;
        }

      GDICT_NOTE (DEFBOX, "Removing old context");
      
      g_object_unref (G_OBJECT (priv->context));
    }

  if (!context)
    return;

  if (!GDICT_IS_CONTEXT (context))
    {
      g_warning ("Object of type '%s' instead of a GdictContext\n",
      		 g_type_name (G_OBJECT_TYPE (context)));
      return;
    }

  GDICT_NOTE (DEFBOX, "Setting new context");
    
  priv->context = context;
  g_object_ref (G_OBJECT (priv->context));
}

static void
gdict_defbox_set_property (GObject      *object,
			   guint         prop_id,
			   const GValue *value,
			   GParamSpec   *pspec)
{
  GdictDefbox *defbox = GDICT_DEFBOX (object);
  GdictDefboxPrivate *priv = defbox->priv;
  
  switch (prop_id)
    {
    case PROP_WORD:
      gdict_defbox_lookup (defbox, g_value_get_string (value));
      break;
    case PROP_CONTEXT:
      set_gdict_context (defbox, g_value_get_object (value));
      break;
    case PROP_DATABASE:
      g_free (priv->database);
      priv->database = g_strdup (g_value_get_string (value));
      break;
    case PROP_FONT_NAME:
      gdict_defbox_set_font_name (defbox, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gdict_defbox_get_property (GObject    *object,
			   guint       prop_id,
			   GValue     *value,
			   GParamSpec *pspec)
{
  GdictDefbox *defbox = GDICT_DEFBOX (object);
  GdictDefboxPrivate *priv = defbox->priv;
  
  switch (prop_id)
    {
    case PROP_WORD:
      g_value_set_string (value, priv->word);
      break;
    case PROP_CONTEXT:
      g_value_set_object (value, priv->context);
      break;
    case PROP_DATABASE:
      g_value_set_string (value, priv->database);
      break;
    case PROP_FONT_NAME:
      g_value_set_string (value, priv->font_name);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/*
 * this code has been copied from gtksourceview; it's the implementation
 * for case-insensitive search in a GtkTextBuffer. this is non-trivial, as
 * searches on a utf-8 text stream involve a norm(casefold(norm(utf8)))
 * operation which can be costly on large buffers. luckily for us, it's
 * not the case on a set of definitions.
 */

#define GTK_TEXT_UNKNOWN_CHAR 0xFFFC

/* this function acts like g_utf8_offset_to_pointer() except that if it finds a
 * decomposable character it consumes the decomposition length from the given
 * offset.  So it's useful when the offset was calculated for the normalized
 * version of str, but we need a pointer to str itself. */
static const gchar *
pointer_from_offset_skipping_decomp (const gchar *str, gint offset)
{
	gchar *casefold, *normal;
	const gchar *p, *q;

	p = str;
	while (offset > 0)
	{
		q = g_utf8_next_char (p);
		casefold = g_utf8_casefold (p, q - p);
		normal = g_utf8_normalize (casefold, -1, G_NORMALIZE_NFD);
		offset -= g_utf8_strlen (normal, -1);
		g_free (casefold);
		g_free (normal);
		p = q;
	}
	return p;
}

static gboolean
exact_prefix_cmp (const gchar *string,
		  const gchar *prefix,
		  guint        prefix_len)
{
	GUnicodeType type;

	if (strncmp (string, prefix, prefix_len) != 0)
		return FALSE;
	if (string[prefix_len] == '\0')
		return TRUE;

	type = g_unichar_type (g_utf8_get_char (string + prefix_len));

	/* If string contains prefix, check that prefix is not followed
	 * by a unicode mark symbol, e.g. that trailing 'a' in prefix
	 * is not part of two-char a-with-hat symbol in string. */
	return type != G_UNICODE_COMBINING_MARK &&
		type != G_UNICODE_ENCLOSING_MARK &&
		type != G_UNICODE_NON_SPACING_MARK;
}

static const gchar *
utf8_strcasestr (const gchar *haystack, const gchar *needle)
{
	gsize needle_len;
	gsize haystack_len;
	const gchar *ret = NULL;
	gchar *p;
	gchar *casefold;
	gchar *caseless_haystack;
	gint i;

	g_return_val_if_fail (haystack != NULL, NULL);
	g_return_val_if_fail (needle != NULL, NULL);

	casefold = g_utf8_casefold (haystack, -1);
	caseless_haystack = g_utf8_normalize (casefold, -1, G_NORMALIZE_NFD);
	g_free (casefold);

	needle_len = g_utf8_strlen (needle, -1);
	haystack_len = g_utf8_strlen (caseless_haystack, -1);

	if (needle_len == 0)
	{
		ret = (gchar *)haystack;
		goto finally_1;
	}

	if (haystack_len < needle_len)
	{
		ret = NULL;
		goto finally_1;
	}

	p = (gchar*)caseless_haystack;
	needle_len = strlen (needle);
	i = 0;

	while (*p)
	{
		if (exact_prefix_cmp (p, needle, needle_len))
		{
			ret = pointer_from_offset_skipping_decomp (haystack, i);
			goto finally_1;
		}

		p = g_utf8_next_char (p);
		i++;
	}

finally_1:
	g_free (caseless_haystack);

	return ret;
}

static const gchar *
utf8_strrcasestr (const gchar *haystack, const gchar *needle)
{
	gsize needle_len;
	gsize haystack_len;
	const gchar *ret = NULL;
	gchar *p;
	gchar *casefold;
	gchar *caseless_haystack;
	gint i;

	g_return_val_if_fail (haystack != NULL, NULL);
	g_return_val_if_fail (needle != NULL, NULL);

	casefold = g_utf8_casefold (haystack, -1);
	caseless_haystack = g_utf8_normalize (casefold, -1, G_NORMALIZE_NFD);
	g_free (casefold);

	needle_len = g_utf8_strlen (needle, -1);
	haystack_len = g_utf8_strlen (caseless_haystack, -1);

	if (needle_len == 0)
	{
		ret = (gchar *)haystack;
		goto finally_1;
	}

	if (haystack_len < needle_len)
	{
		ret = NULL;
		goto finally_1;
	}

	i = haystack_len - needle_len;
	p = g_utf8_offset_to_pointer (caseless_haystack, i);
	needle_len = strlen (needle);

	while (p >= caseless_haystack)
	{
		if (exact_prefix_cmp (p, needle, needle_len))
		{
			ret = pointer_from_offset_skipping_decomp (haystack, i);
			goto finally_1;
		}

		p = g_utf8_prev_char (p);
		i--;
	}

finally_1:
	g_free (caseless_haystack);

	return ret;
}

static gboolean
utf8_caselessnmatch (const char *s1, const char *s2,
		     gssize n1, gssize n2)
{
	gchar *casefold;
	gchar *normalized_s1;
	gchar *normalized_s2;
	gint len_s1;
	gint len_s2;
	gboolean ret = FALSE;

	g_return_val_if_fail (s1 != NULL, FALSE);
	g_return_val_if_fail (s2 != NULL, FALSE);
	g_return_val_if_fail (n1 > 0, FALSE);
	g_return_val_if_fail (n2 > 0, FALSE);

	casefold = g_utf8_casefold (s1, n1);
	normalized_s1 = g_utf8_normalize (casefold, -1, G_NORMALIZE_NFD);
	g_free (casefold);

	casefold = g_utf8_casefold (s2, n2);
	normalized_s2 = g_utf8_normalize (casefold, -1, G_NORMALIZE_NFD);
	g_free (casefold);

	len_s1 = strlen (normalized_s1);
	len_s2 = strlen (normalized_s2);

	if (len_s1 < len_s2)
		goto finally_2;

	ret = (strncmp (normalized_s1, normalized_s2, len_s2) == 0);

finally_2:
	g_free (normalized_s1);
	g_free (normalized_s2); 

	return ret;
}

/* FIXME: total horror */
static gboolean
char_is_invisible (const GtkTextIter *iter)
{
	GSList *tags;
	gboolean invisible = FALSE;
	tags = gtk_text_iter_get_tags (iter);
	while (tags)
	{
		gboolean this_invisible, invisible_set;
		g_object_get (tags->data, "invisible", &this_invisible, 
			      "invisible-set", &invisible_set, NULL);
		if (invisible_set)
			invisible = this_invisible;
		tags = g_slist_delete_link (tags, tags);
	}
	return invisible;
}

static void
forward_chars_with_skipping (GtkTextIter *iter,
			     gint         count,
			     gboolean     skip_invisible,
			     gboolean     skip_nontext,
			     gboolean     skip_decomp)
{
	gint i;

	g_return_if_fail (count >= 0);

	i = count;

	while (i > 0)
	{
		gboolean ignored = FALSE;

		/* minimal workaround to avoid the infinite loop of bug #168247.
		 * It doesn't fix the problemjust the symptom...
		 */
		if (gtk_text_iter_is_end (iter))
			return;

		if (skip_nontext && gtk_text_iter_get_char (iter) == GTK_TEXT_UNKNOWN_CHAR)
			ignored = TRUE;

		/* FIXME: char_is_invisible() gets list of tags for each char there,
		   and checks every tag. It doesn't sound like a good idea. */
		if (!ignored && skip_invisible && char_is_invisible (iter))
			ignored = TRUE;

		if (!ignored && skip_decomp)
		{
			/* being UTF8 correct sucks; this accounts for extra
			   offsets coming from canonical decompositions of
			   UTF8 characters (e.g. accented characters) which 
			   g_utf8_normalize() performs */
			gchar *normal;
			gchar buffer[6];
			gint buffer_len;

			buffer_len = g_unichar_to_utf8 (gtk_text_iter_get_char (iter), buffer);
			normal = g_utf8_normalize (buffer, buffer_len, G_NORMALIZE_NFD);
			i -= (g_utf8_strlen (normal, -1) - 1);
			g_free (normal);
		}

		gtk_text_iter_forward_char (iter);

		if (!ignored)
			--i;
	}
}

static gboolean
lines_match (const GtkTextIter *start,
	     const gchar      **lines,
	     gboolean           visible_only,
	     gboolean           slice,
	     GtkTextIter       *match_start,
	     GtkTextIter       *match_end)
{
	GtkTextIter next;
	gchar *line_text;
	const gchar *found;
	gint offset;

	if (*lines == NULL || **lines == '\0')
	{
		if (match_start)
			*match_start = *start;
		if (match_end)
			*match_end = *start;
		return TRUE;
	}

	next = *start;
	gtk_text_iter_forward_line (&next);

	/* No more text in buffer, but *lines is nonempty */
	if (gtk_text_iter_equal (start, &next))
		return FALSE;

	if (slice)
	{
		if (visible_only)
			line_text = gtk_text_iter_get_visible_slice (start, &next);
		else
			line_text = gtk_text_iter_get_slice (start, &next);
	}
	else
	{
		if (visible_only)
			line_text = gtk_text_iter_get_visible_text (start, &next);
		else
			line_text = gtk_text_iter_get_text (start, &next);
	}

	if (match_start) /* if this is the first line we're matching */
	{
		found = utf8_strcasestr (line_text, *lines);
	}
	else
	{
		/* If it's not the first line, we have to match from the
		 * start of the line.
		 */
		if (utf8_caselessnmatch (line_text, *lines, strlen (line_text),
					 strlen (*lines)))
			found = line_text;
		else
			found = NULL;
	}

	if (found == NULL)
	{
		g_free (line_text);
		return FALSE;
	}

	/* Get offset to start of search string */
	offset = g_utf8_strlen (line_text, found - line_text);

	next = *start;

	/* If match start needs to be returned, set it to the
	 * start of the search string.
	 */
	forward_chars_with_skipping (&next, offset, visible_only, !slice, FALSE);
	if (match_start)
	{
		*match_start = next;
	}

	/* Go to end of search string */
	forward_chars_with_skipping (&next, g_utf8_strlen (*lines, -1), visible_only, !slice, TRUE);

	g_free (line_text);

	++lines;

	if (match_end)
		*match_end = next;

	/* pass NULL for match_start, since we don't need to find the
	 * start again.
	 */
	return lines_match (&next, lines, visible_only, slice, NULL, match_end);
}

static gboolean
backward_lines_match (const GtkTextIter *start,
		      const gchar      **lines,
		      gboolean           visible_only,
		      gboolean           slice,
		      GtkTextIter       *match_start,
		      GtkTextIter       *match_end)
{
	GtkTextIter line, next;
	gchar *line_text;
	const gchar *found;
	gint offset;

	if (*lines == NULL || **lines == '\0')
	{
		if (match_start)
			*match_start = *start;
		if (match_end)
			*match_end = *start;
		return TRUE;
	}

	line = next = *start;
	if (gtk_text_iter_get_line_offset (&next) == 0)
	{
		if (!gtk_text_iter_backward_line (&next))
			return FALSE;
	}
	else
		gtk_text_iter_set_line_offset (&next, 0);

	if (slice)
	{
		if (visible_only)
			line_text = gtk_text_iter_get_visible_slice (&next, &line);
		else
			line_text = gtk_text_iter_get_slice (&next, &line);
	}
	else
	{
		if (visible_only)
			line_text = gtk_text_iter_get_visible_text (&next, &line);
		else
			line_text = gtk_text_iter_get_text (&next, &line);
	}

	if (match_start) /* if this is the first line we're matching */
	{
		found = utf8_strrcasestr (line_text, *lines);
	}
	else
	{
		/* If it's not the first line, we have to match from the
		 * start of the line.
		 */
		if (utf8_caselessnmatch (line_text, *lines, strlen (line_text),
					 strlen (*lines)))
			found = line_text;
		else
			found = NULL;
	}

	if (found == NULL)
	{
		g_free (line_text);
		return FALSE;
	}

	/* Get offset to start of search string */
	offset = g_utf8_strlen (line_text, found - line_text);

	forward_chars_with_skipping (&next, offset, visible_only, !slice, FALSE);

	/* If match start needs to be returned, set it to the
	 * start of the search string.
	 */
	if (match_start)
	{
		*match_start = next;
	}

	/* Go to end of search string */
	forward_chars_with_skipping (&next, g_utf8_strlen (*lines, -1), visible_only, !slice, TRUE);

	g_free (line_text);

	++lines;

	if (match_end)
		*match_end = next;

	/* try to match the rest of the lines forward, passing NULL
	 * for match_start so lines_match will try to match the entire
	 * line */
	return lines_match (&next, lines, visible_only,
			    slice, NULL, match_end);
}

/* strsplit () that retains the delimiter as part of the string. */
static gchar **
breakup_string (const char *string,
		const char *delimiter,
		gint        max_tokens)
{
	GSList *string_list = NULL, *slist;
	gchar **str_array, *s, *casefold, *new_string;
	guint i, n = 1;

	g_return_val_if_fail (string != NULL, NULL);
	g_return_val_if_fail (delimiter != NULL, NULL);

	if (max_tokens < 1)
		max_tokens = G_MAXINT;

	s = strstr (string, delimiter);
	if (s)
	{
		guint delimiter_len = strlen (delimiter);

		do
		{
			guint len;

			len = s - string + delimiter_len;
			new_string = g_new (gchar, len + 1);
			strncpy (new_string, string, len);
			new_string[len] = 0;
			casefold = g_utf8_casefold (new_string, -1);
			g_free (new_string);
			new_string = g_utf8_normalize (casefold, -1, G_NORMALIZE_NFD);
			g_free (casefold);
			string_list = g_slist_prepend (string_list, new_string);
			n++;
			string = s + delimiter_len;
			s = strstr (string, delimiter);
		} while (--max_tokens && s);
	}

	if (*string)
	{
		n++;
		casefold = g_utf8_casefold (string, -1);
		new_string = g_utf8_normalize (casefold, -1, G_NORMALIZE_NFD);
		g_free (casefold);
		string_list = g_slist_prepend (string_list, new_string);
	}

	str_array = g_new (gchar*, n);

	i = n - 1;

	str_array[i--] = NULL;
	for (slist = string_list; slist; slist = slist->next)
		str_array[i--] = slist->data;

	g_slist_free (string_list);

	return str_array;
}

static gboolean
gdict_defbox_iter_forward_search (const GtkTextIter   *iter,
                                  const gchar         *str,
                                  GtkTextIter         *match_start,
                                  GtkTextIter         *match_end,
                                  const GtkTextIter   *limit)
{
  gchar **lines = NULL;
  GtkTextIter match;
  gboolean retval = FALSE;
  GtkTextIter search;

  g_return_val_if_fail (iter != NULL, FALSE);
  g_return_val_if_fail (str != NULL, FALSE);

  if (limit && gtk_text_iter_compare (iter, limit) >= 0)
    return FALSE;

  if (*str == '\0')
    {
      /* If we can move one char, return the empty string there */
      match = *iter;

      if (gtk_text_iter_forward_char (&match))
        {
          if (limit && gtk_text_iter_equal (&match, limit))
            return FALSE;

          if (match_start)
            *match_start = match;

          if (match_end)
            *match_end = match;

          return TRUE;
        }
      else
        return FALSE;
    }

  /* locate all lines */
  lines = breakup_string (str, "\n", -1);

  search = *iter;

  /* This loop has an inefficient worst-case, where
   * gtk_text_iter_get_text () is called repeatedly on
   * a single line.
   */
  do
    {
      GtkTextIter end;
      gboolean res;

      if (limit && gtk_text_iter_compare (&search, limit) >= 0)
        break;

      res = lines_match (&search, (const gchar**)lines,
                         TRUE, FALSE,
                         &match, &end);
      if (res)
        {
          if (limit == NULL ||
              (limit && gtk_text_iter_compare (&end, limit) <= 0))
            {
              retval = TRUE;
              
              if (match_start)
                *match_start = match;

              if (match_end)
                *match_end = end;
            }

          break;
        }
    } while (gtk_text_iter_forward_line (&search));

  g_strfreev ((gchar**) lines);

  return retval;
}

static gboolean
gdict_defbox_iter_backward_search (const GtkTextIter   *iter,
                                   const gchar         *str,
                                   GtkTextIter         *match_start,
                                   GtkTextIter         *match_end,
                                   const GtkTextIter   *limit)
{
  gchar **lines = NULL;
  GtkTextIter match;
  gboolean retval = FALSE;
  GtkTextIter search;

  g_return_val_if_fail (iter != NULL, FALSE);
  g_return_val_if_fail (str != NULL, FALSE);

  if (limit && gtk_text_iter_compare (iter, limit) <= 0)
    return FALSE;

  if (*str == '\0')
    {
      /* If we can move one char, return the empty string there */
      match = *iter;

      if (gtk_text_iter_backward_char (&match))
        {
          if (limit && gtk_text_iter_equal (&match, limit))
            return FALSE;

          if (match_start)
            *match_start = match;

          if (match_end)
            *match_end = match;

          return TRUE;
        }
      else
        return FALSE;
    }

  /* locate all lines */
  lines = breakup_string (str, "\n", -1);

  search = *iter;

  /* This loop has an inefficient worst-case, where
   * gtk_text_iter_get_text () is called repeatedly on
   * a single line.
   */
  while (TRUE)
    {
      GtkTextIter end;
      gboolean res;

      if (limit && gtk_text_iter_compare (&search, limit) <= 0)
        break;

      res = backward_lines_match (&search, (const gchar**)lines,
                                  TRUE, FALSE,
                                  &match, &end);
      if (res)
        {
           if (limit == NULL ||
               (limit && gtk_text_iter_compare (&end, limit) > 0))
             {
               retval = TRUE;
               
               if (match_start)
                 *match_start = match;

               if (match_end)
                 *match_end = end;

             }
           
           break;
        }

      if (gtk_text_iter_get_line_offset (&search) == 0)
        {
          if (!gtk_text_iter_backward_line (&search))
            break;
        }
      else
        gtk_text_iter_set_line_offset (&search, 0);
    }

  g_strfreev ((gchar**) lines);

  return retval;
}

static gboolean
gdict_defbox_find_backward (GdictDefbox *defbox,
			    const gchar *text)
{
  GdictDefboxPrivate *priv = defbox->priv;
  GtkTextIter start_iter, end_iter;
  GtkTextIter match_start, match_end;
  GtkTextIter iter;
  GtkTextMark *last_search;
  gboolean res;

  g_assert (GTK_IS_TEXT_BUFFER (priv->buffer));
  
  gtk_text_buffer_get_bounds (priv->buffer, &start_iter, &end_iter);
  
  /* if there already has been another result, begin from there */
  last_search = gtk_text_buffer_get_mark (priv->buffer, "last-search-prev");
  if (last_search)
    gtk_text_buffer_get_iter_at_mark (priv->buffer, &iter, last_search);
  else
    iter = end_iter;
  
  res = gdict_defbox_iter_backward_search (&iter, text,
                                           &match_start, &match_end,
                                           NULL);
  if (res)
    {
      gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (priv->text_view),
      				    &match_start,
      				    0.0,
      				    TRUE,
      				    0.0, 0.0);
      gtk_text_buffer_place_cursor (priv->buffer, &match_end);
      gtk_text_buffer_move_mark (priv->buffer,
      				 gtk_text_buffer_get_mark (priv->buffer, "selection_bound"),
      				 &match_start);
      gtk_text_buffer_create_mark (priv->buffer, "last-search-prev", &match_start, FALSE);
      gtk_text_buffer_create_mark (priv->buffer, "last-search-next", &match_end, FALSE);
      
      return TRUE;
    }
  
  return FALSE;
}

static gboolean
hide_find_pane (gpointer user_data)
{
  GdictDefbox *defbox = user_data;

  gtk_widget_hide (defbox->priv->find_pane);
  defbox->priv->show_find = FALSE;
  
  gtk_widget_grab_focus (defbox->priv->text_view);
  
  defbox->priv->hide_timeout = 0;

  return FALSE;
}

static void
find_prev_clicked_cb (GtkWidget *widget,
		      gpointer   user_data)
{
  GdictDefbox *defbox = GDICT_DEFBOX (user_data);
  GdictDefboxPrivate *priv = defbox->priv;
  const gchar *text;
  gboolean found;

  gtk_widget_hide (priv->find_label);
  
  text = gtk_entry_get_text (GTK_ENTRY (priv->find_entry));
  if (!text)
    return;
  
  found = gdict_defbox_find_backward (defbox, text);
  if (!found)
    {
      gchar *str;
      
      str = g_strconcat ("  <i>", _("Not found"), "</i>", NULL);
      gtk_label_set_markup (GTK_LABEL (priv->find_label), str);
      gtk_widget_show (priv->find_label);
      
      g_free (str);
    }

  if (priv->hide_timeout)
    {
      g_source_remove (priv->hide_timeout);
      priv->hide_timeout = g_timeout_add (5000, hide_find_pane, defbox);
    }
}

static gboolean
gdict_defbox_find_forward (GdictDefbox *defbox,
			   const gchar *text,
			   gboolean     is_typing)
{
  GdictDefboxPrivate *priv = defbox->priv;
  GtkTextIter start_iter, end_iter;
  GtkTextIter match_start, match_end;
  GtkTextIter iter;
  GtkTextMark *last_search;
  gboolean res;

  g_assert (GTK_IS_TEXT_BUFFER (priv->buffer));
  
  gtk_text_buffer_get_bounds (priv->buffer, &start_iter, &end_iter);

  if (!is_typing)
    {
      /* if there already has been another result, begin from there */
      last_search = gtk_text_buffer_get_mark (priv->buffer, "last-search-next");

      if (last_search)
        gtk_text_buffer_get_iter_at_mark (priv->buffer, &iter, last_search);
      else
	iter = start_iter;
    }
  else
    {
      last_search = gtk_text_buffer_get_mark (priv->buffer, "last-search-prev");

      if (last_search)
	gtk_text_buffer_get_iter_at_mark (priv->buffer, &iter, last_search);
      else
	iter = start_iter;
    }
  
  res = gdict_defbox_iter_forward_search (&iter, text,
                                          &match_start,
                                          &match_end,
                                          NULL);
  if (res)
    {
      gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (priv->text_view),
      				    &match_start,
      				    0.0,
      				    TRUE,
      				    0.0, 0.0);
      gtk_text_buffer_place_cursor (priv->buffer, &match_end);
      gtk_text_buffer_move_mark (priv->buffer,
      				 gtk_text_buffer_get_mark (priv->buffer, "selection_bound"),
      				 &match_start);
      gtk_text_buffer_create_mark (priv->buffer, "last-search-prev", &match_start, FALSE);
      gtk_text_buffer_create_mark (priv->buffer, "last-search-next", &match_end, FALSE);
      
      return TRUE;
    }
  
  return FALSE;
}

static void
find_next_clicked_cb (GtkWidget *widget,
		      gpointer   user_data)
{
  GdictDefbox *defbox = GDICT_DEFBOX (user_data);
  GdictDefboxPrivate *priv = defbox->priv;
  const gchar *text;
  gboolean found;
  
  gtk_widget_hide (priv->find_label);
  
  text = gtk_entry_get_text (GTK_ENTRY (priv->find_entry));
  if (!text)
    return;
  
  found = gdict_defbox_find_forward (defbox, text, FALSE);
  if (!found)
    {
      gchar *str;
      
      str = g_strconcat ("  <i>", _("Not found"), "</i>", NULL);
      gtk_label_set_markup (GTK_LABEL (priv->find_label), str);
      gtk_widget_show (priv->find_label);
      
      g_free (str);
    }

  if (priv->hide_timeout)
    {
      g_source_remove (priv->hide_timeout);
      priv->hide_timeout = g_timeout_add (5000, hide_find_pane, defbox);
    }
}

static void
find_entry_changed_cb (GtkWidget *widget,
		       gpointer   user_data)
{
  GdictDefbox *defbox = GDICT_DEFBOX (user_data);
  GdictDefboxPrivate *priv = defbox->priv;
  gchar *text;
  gboolean found;

  gtk_widget_hide (priv->find_label);
  
  text = gtk_editable_get_chars (GTK_EDITABLE (widget), 0, -1);
  if (!text)
    return;

  found = gdict_defbox_find_forward (defbox, text, TRUE);
  if (!found)
    {
      gchar *str;
      
      str = g_strconcat ("  <i>", _("Not found"), "</i>", NULL);
      gtk_label_set_markup (GTK_LABEL (priv->find_label), str);
      gtk_widget_show (priv->find_label);
      
      g_free (str);
    }

  g_free (text);

  if (priv->hide_timeout)
    {
      g_source_remove (priv->hide_timeout);
      priv->hide_timeout = g_timeout_add (5000, hide_find_pane, defbox);
    }
}

static void
close_button_clicked (GtkButton *button,
                      gpointer   data)
{
  GdictDefboxPrivate *priv = GDICT_DEFBOX (data)->priv;

  if (priv->hide_timeout)
    g_source_remove (priv->hide_timeout);

  (void) hide_find_pane (data);
}

static GtkWidget *
create_find_pane (GdictDefbox *defbox)
{
  GdictDefboxPrivate *priv;
  GtkWidget *find_pane;
  GtkWidget *label;
  GtkWidget *sep;
  GtkWidget *hbox1, *hbox2;
  GtkWidget *button;
 
  priv = defbox->priv;
  
  find_pane = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (find_pane), 0);
  
  hbox1 = gtk_hbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (find_pane), hbox1, TRUE, TRUE, 0);
  gtk_widget_show (hbox1);

  button = gtk_button_new ();
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_button_set_image (GTK_BUTTON (button),
                        gtk_image_new_from_stock (GTK_STOCK_CLOSE,
                                                  GTK_ICON_SIZE_BUTTON));
  g_signal_connect (button, "clicked",
                    G_CALLBACK (close_button_clicked), defbox);
  gtk_box_pack_start (GTK_BOX (hbox1), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  hbox2 = gtk_hbox_new (FALSE, 12);
  gtk_box_pack_start (GTK_BOX (hbox1), hbox2, TRUE, TRUE, 0);
  gtk_widget_show (hbox2);
 
  label = gtk_label_new_with_mnemonic (_("F_ind:"));
  gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, FALSE, 0);

  priv->find_entry = gtk_entry_new ();
  g_signal_connect (priv->find_entry, "changed",
  		    G_CALLBACK (find_entry_changed_cb), defbox);
  gtk_box_pack_start (GTK_BOX (hbox2), priv->find_entry, TRUE, TRUE, 0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), priv->find_entry);
  
  sep = gtk_vseparator_new ();
  gtk_box_pack_start (GTK_BOX (hbox1), sep, FALSE, FALSE, 0);
  gtk_widget_show (sep);

  priv->find_prev = gtk_button_new_with_mnemonic (_("_Previous"));
  gtk_button_set_image (GTK_BUTTON (priv->find_prev),
  			gtk_image_new_from_stock (GTK_STOCK_GO_BACK,
  						  GTK_ICON_SIZE_MENU));
  g_signal_connect (priv->find_prev, "clicked",
  		    G_CALLBACK (find_prev_clicked_cb), defbox);
  gtk_box_pack_start (GTK_BOX (hbox1), priv->find_prev, FALSE, FALSE, 0);

  priv->find_next = gtk_button_new_with_mnemonic (_("_Next"));
  gtk_button_set_image (GTK_BUTTON (priv->find_next),
  			gtk_image_new_from_stock (GTK_STOCK_GO_FORWARD,
  						  GTK_ICON_SIZE_MENU));
  g_signal_connect (priv->find_next, "clicked",
  		    G_CALLBACK (find_next_clicked_cb), defbox);
  gtk_box_pack_start (GTK_BOX (hbox1), priv->find_next, FALSE, FALSE, 0);
  
  priv->find_label = gtk_label_new (NULL);
  gtk_label_set_use_markup (GTK_LABEL (priv->find_label), TRUE);
  gtk_box_pack_end (GTK_BOX (find_pane), priv->find_label, FALSE, FALSE, 0);
  gtk_widget_hide (priv->find_label);

  return find_pane;
}

static void
gdict_defbox_init_tags (GdictDefbox *defbox)
{
  GdictDefboxPrivate *priv = defbox->priv;
  GdkColor *link_color, *visited_link_color;
  
  g_assert (GTK_IS_TEXT_BUFFER (priv->buffer));

  gtk_text_buffer_create_tag (priv->buffer, "italic",
  			      "style", PANGO_STYLE_ITALIC,
  			      NULL);
  gtk_text_buffer_create_tag (priv->buffer, "bold",
  			      "weight", PANGO_WEIGHT_BOLD,
  			      NULL);
  gtk_text_buffer_create_tag (priv->buffer, "underline",
  			      "underline", PANGO_UNDERLINE_SINGLE,
  			      NULL);
  
  gtk_text_buffer_create_tag (priv->buffer, "big",
  			      "scale", 1.6,
  			      NULL);
  gtk_text_buffer_create_tag (priv->buffer, "small",
		  	      "scale", PANGO_SCALE_SMALL,
			      NULL);  

  link_color = visited_link_color = NULL;
  gtk_widget_style_get (GTK_WIDGET (defbox),
                        "link-color", &link_color,
                        "visited-link-color", &visited_link_color,
                        NULL);
  if (!link_color)
    link_color = &default_link_color;

  if (!visited_link_color)
    visited_link_color = &default_visited_link_color;

  priv->link_tag =
    gtk_text_buffer_create_tag (priv->buffer, "link",
                                "underline", PANGO_UNDERLINE_SINGLE,
                                "foreground-gdk", link_color,
                                NULL);
  priv->visited_link_tag =
    gtk_text_buffer_create_tag (priv->buffer, "visited-link",
                                "underline", PANGO_UNDERLINE_SINGLE,
                                "foreground-gdk", visited_link_color,
                                NULL);

  if (link_color != &default_link_color)
    gdk_color_free (link_color);

  if (visited_link_color != &default_visited_link_color)
    gdk_color_free (visited_link_color);

  gtk_text_buffer_create_tag (priv->buffer, "phonetic",
                              "foreground", "dark gray",
                              NULL);

  gtk_text_buffer_create_tag (priv->buffer, "query-title",
		              "left-margin", QUERY_MARGIN,
  			      "pixels-above-lines", 5,
  			      "pixels-below-lines", 20,
			      NULL);
  gtk_text_buffer_create_tag (priv->buffer, "query-from",
		  	      "foreground", "dark gray",
			      "scale", PANGO_SCALE_SMALL,
		  	      "left-margin", QUERY_MARGIN,
			      "pixels-above-lines", 5,
			      "pixels-below-lines", 10,
			      NULL);

  gtk_text_buffer_create_tag (priv->buffer, "error-title",
		  	      "foreground", "dark red",
			      "left-margin", ERROR_MARGIN,
			      NULL);
  gtk_text_buffer_create_tag (priv->buffer, "error-message",
		              "left-margin", ERROR_MARGIN,
			      NULL);
}

static void
follow_if_is_link (GdictDefbox *defbox,
                   GtkTextView *text_view,
                   GtkTextIter *iter)
{
  GSList *tags, *l;

  tags = gtk_text_iter_get_tags (iter);
  
  for (l = tags; l != NULL; l = l->next)
    {
      GtkTextTag *tag = l->data;
      gchar *name;

      g_object_get (G_OBJECT (tag), "name", &name, NULL);
      if (name &&
          (strcmp (name, "link") == 0 ||
           strcmp (name, "visited-link") == 0))
        {
          GtkTextBuffer *buffer = gtk_text_view_get_buffer (text_view);
          GtkTextIter start, end;
          gchar *link_str;

          start = *iter;
          end = *iter;

          gtk_text_iter_backward_to_tag_toggle (&start, tag);
          gtk_text_iter_forward_to_tag_toggle (&end, tag);

          link_str = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);

          g_signal_emit (defbox, gdict_defbox_signals[LINK_CLICKED], 0, link_str);

          g_free (link_str);
          g_free (name);
          
          break;
        }

      g_free (name);
    }

  g_slist_free (tags);
}

static gboolean
defbox_event_after_cb (GtkWidget   *text_view,
                       GdkEvent    *event,
                       GdictDefbox *defbox)
{
  GtkTextIter iter;
  GtkTextBuffer *buffer;
  GdkEventButton *button_event;
  gint bx, by;

  if (event->type != GDK_BUTTON_RELEASE)
    return FALSE;

  button_event = (GdkEventButton *) event;

  if (button_event->button != 1)
    return FALSE;

  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (text_view));
  if (gtk_text_buffer_get_has_selection (buffer))
    return FALSE;

  gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (text_view),
                                         GTK_TEXT_WINDOW_WIDGET,
                                         button_event->x, button_event->y,
                                         &bx, &by);

  gtk_text_view_get_iter_at_location (GTK_TEXT_VIEW (text_view),
                                      &iter,
                                      bx, by);

  follow_if_is_link (defbox, GTK_TEXT_VIEW (text_view), &iter);

  return FALSE;
}

static void
set_cursor_if_appropriate (GdictDefbox *defbox,
                           GtkTextView *text_view,
                           gint         x,
                           gint         y)
{
  GdictDefboxPrivate *priv;
  GSList *tags, *l;
  GtkTextIter iter;
  gboolean hovering = FALSE;

  priv = defbox->priv;

  if (!priv->hand_cursor)
    priv->hand_cursor = gdk_cursor_new (GDK_HAND2);

  if (!priv->regular_cursor)
    priv->regular_cursor = gdk_cursor_new (GDK_XTERM);

  gtk_text_view_get_iter_at_location (text_view, &iter, x, y);

  tags = gtk_text_iter_get_tags (&iter);
  for (l = tags; l != NULL; l = l->next)
    {
      GtkTextTag *tag = l->data;
      gchar *name;

      g_object_get (G_OBJECT (tag), "name", &name, NULL);
      if (name &&
          (strcmp (name, "link") == 0 ||
           strcmp (name, "visited-link") == 0))
        {
          hovering = TRUE;
          g_free (name);

          break;
        }

      g_free (name);
    }

  if (hovering != defbox->priv->is_hovering)
    {
      defbox->priv->is_hovering = hovering;

      if (defbox->priv->is_hovering)
        gdk_window_set_cursor (gtk_text_view_get_window (text_view,
                                                         GTK_TEXT_WINDOW_TEXT),
                               defbox->priv->hand_cursor);
      else
        gdk_window_set_cursor (gtk_text_view_get_window (text_view,
                                                         GTK_TEXT_WINDOW_TEXT),
                               defbox->priv->regular_cursor);
    }

  if (tags)
    g_slist_free (tags);
}

static gboolean
defbox_motion_notify_cb (GtkWidget      *text_view,
                         GdkEventMotion *event,
                         GdictDefbox    *defbox)
{
  gint bx, by;

  gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (text_view),
                                         GTK_TEXT_WINDOW_WIDGET,
                                         event->x, event->y,
                                         &bx, &by);

  set_cursor_if_appropriate (defbox, GTK_TEXT_VIEW (text_view), bx, by);

  gdk_window_get_pointer (gtk_widget_get_window (text_view), NULL, NULL, NULL);

  return FALSE;
}

static gboolean
defbox_visibility_notify_cb (GtkWidget          *text_view,
                             GdkEventVisibility *event,
                             GdictDefbox        *defbox)
{
  gint wx, wy;
  gint bx, by;

  gdk_window_get_pointer (gtk_widget_get_window (text_view), &wx, &wy, NULL);

  gtk_text_view_window_to_buffer_coords (GTK_TEXT_VIEW (text_view),
                                         GTK_TEXT_WINDOW_WIDGET,
                                         wx, wy,
                                         &bx, &by);

  set_cursor_if_appropriate (defbox, GTK_TEXT_VIEW (text_view), bx, by);

  return FALSE;
}

static GObject *
gdict_defbox_constructor (GType                  type,
			  guint                  n_construct_properties,
			  GObjectConstructParam *construct_params)
{
  GdictDefbox *defbox;
  GdictDefboxPrivate *priv;
  GObject *object;
  GtkWidget *sw;
  
  object = G_OBJECT_CLASS (gdict_defbox_parent_class)->constructor (type,
  						   n_construct_properties,
  						   construct_params);
  defbox = GDICT_DEFBOX (object);
  priv = defbox->priv;
  
  gtk_widget_push_composite_child ();
  
  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_composite_name (sw, "gdict-defbox-scrolled-window");
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
  				  GTK_POLICY_AUTOMATIC,
  				  GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
  				       GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (defbox), sw, TRUE, TRUE, 0);
  gtk_widget_show (sw);
  
  priv->buffer = gtk_text_buffer_new (NULL);
  gdict_defbox_init_tags (defbox);
  
  priv->text_view = gtk_text_view_new_with_buffer (priv->buffer);
  gtk_widget_set_composite_name (priv->text_view, "gdict-defbox-text-view");
  gtk_text_view_set_editable (GTK_TEXT_VIEW (priv->text_view), FALSE);
  gtk_text_view_set_left_margin (GTK_TEXT_VIEW (priv->text_view), 4);
  gtk_container_add (GTK_CONTAINER (sw), priv->text_view);
  gtk_widget_show (priv->text_view);
  
  priv->find_pane = create_find_pane (defbox);
  gtk_widget_set_composite_name (priv->find_pane, "gdict-defbox-find-pane");
  gtk_box_pack_end (GTK_BOX (defbox), priv->find_pane, FALSE, FALSE, 0);

  /* stuff to make the link machinery work */
  g_signal_connect (priv->text_view, "event-after",
                    G_CALLBACK (defbox_event_after_cb),
                    defbox);
  g_signal_connect (priv->text_view, "motion-notify-event",
                    G_CALLBACK (defbox_motion_notify_cb),
                    defbox);
  g_signal_connect (priv->text_view, "visibility-notify-event",
                    G_CALLBACK (defbox_visibility_notify_cb),
                    defbox);
  
  gtk_widget_pop_composite_child ();
  
  return object;
}

static void
gdict_defbox_style_set (GtkWidget *widget,
			GtkStyle  *old_style)
{
  GdictDefboxPrivate *priv = GDICT_DEFBOX (widget)->priv;
  GdkColor *link_color, *visited_link_color;
  
  if (GTK_WIDGET_CLASS (gdict_defbox_parent_class)->style_set)
    GTK_WIDGET_CLASS (gdict_defbox_parent_class)->style_set (widget, old_style);

  link_color = visited_link_color = NULL;
  gtk_widget_style_get (widget,
                        "link-color", &link_color,
                        "visited-link-color", &visited_link_color,
                        NULL);
  if (!link_color)
    link_color = &default_link_color;

  if (!visited_link_color)
    visited_link_color = &default_visited_link_color;

  g_object_set (G_OBJECT (priv->link_tag),
                "foreground-gdk", link_color,
                NULL);

  g_object_set (G_OBJECT (priv->visited_link_tag),
                "foreground-gdk", visited_link_color,
                NULL);

  if (link_color != &default_link_color)
    gdk_color_free (link_color);

  if (visited_link_color != &default_visited_link_color)
    gdk_color_free (visited_link_color);
}

/* we override the GtkWidget::show_all method since we have widgets
 * we don't want to show, such as the find pane
 */
static void
gdict_defbox_show_all (GtkWidget *widget)
{
  GdictDefbox *defbox = GDICT_DEFBOX (widget);
  GdictDefboxPrivate *priv = defbox->priv;
  
  gtk_widget_show (widget);
  
  if (priv->show_find)
    gtk_widget_show_all (priv->find_pane);
}

static void
gdict_defbox_real_show_find (GdictDefbox *defbox)
{
  gtk_widget_show_all (defbox->priv->find_pane);
  defbox->priv->show_find = TRUE;
  
  gtk_widget_grab_focus (defbox->priv->find_entry);

  defbox->priv->hide_timeout = g_timeout_add (5000, hide_find_pane, defbox);
}

static void
gdict_defbox_real_find_next (GdictDefbox *defbox)
{
  /* synthetize a "clicked" signal to the "next" button */
  gtk_button_clicked (GTK_BUTTON (defbox->priv->find_next));
}

static void
gdict_defbox_real_find_previous (GdictDefbox *defbox)
{
  /* synthetize a "clicked" signal to the "prev" button */
  gtk_button_clicked (GTK_BUTTON (defbox->priv->find_prev));
}

static void
gdict_defbox_real_hide_find (GdictDefbox *defbox)
{
  gtk_widget_hide (defbox->priv->find_pane);
  defbox->priv->show_find = FALSE;
  
  gtk_widget_grab_focus (defbox->priv->text_view);

  if (defbox->priv->hide_timeout)
    {
      g_source_remove (defbox->priv->hide_timeout);
      defbox->priv->hide_timeout = 0;
    }
}

static void
gdict_defbox_class_init (GdictDefboxClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkBindingSet *binding_set;
  
  gobject_class->constructor = gdict_defbox_constructor;
  gobject_class->set_property = gdict_defbox_set_property;
  gobject_class->get_property = gdict_defbox_get_property;
  gobject_class->dispose = gdict_defbox_dispose;
  gobject_class->finalize = gdict_defbox_finalize;
  
  widget_class->show_all = gdict_defbox_show_all;
  widget_class->style_set = gdict_defbox_style_set;

  /**
   * GdictDefbox:word:
   *
   * The word to look up.
   *
   * Since: 0.10
   */
  g_object_class_install_property (gobject_class,
                                   PROP_WORD,
                                   g_param_spec_string ("word",
                                                        "Word",
                                                        "The word to look up",
                                                        NULL,
                                                        G_PARAM_READWRITE));
  /**
   * GdictDefbox:context:
   *
   * The #GdictContext object used to get the word definition.
   *
   * Since: 0.1
   */
  g_object_class_install_property (gobject_class,
  				   PROP_CONTEXT,
  				   g_param_spec_object ("context",
  				   			"Context",
  				   			"The GdictContext object used to get the word definition",
  				   			GDICT_TYPE_CONTEXT,
  				   			(G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT)));
  /**
   * GdictDefbox:database
   *
   * The database used by the #GdictDefbox bound to this object to get the word
   * definition.
   *
   * Since: 0.1 
   */
  g_object_class_install_property (gobject_class,
		  		   PROP_DATABASE,
				   g_param_spec_string ("database",
					   		"Database",
							"The database used to query the GdictContext",
							GDICT_DEFAULT_DATABASE,
							(G_PARAM_READABLE | G_PARAM_WRITABLE)));
  /**
   * GdictDefbox:font-name
   *
   * The name of the font used by the #GdictDefbox to display the definitions.
   * use the same string you use for pango_font_description_from_string().
   *
   * Since: 0.3
   */
  g_object_class_install_property (gobject_class,
		  		   PROP_FONT_NAME,
				   g_param_spec_string ("font-name",
					   		"Font Name",
							"The font to be used by the defbox",
							GDICT_DEFAULT_FONT_NAME,
							(G_PARAM_READABLE | G_PARAM_WRITABLE)));
  
  gdict_defbox_signals[SHOW_FIND] =
    g_signal_new ("show-find",
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (GdictDefboxClass, show_find),
		  NULL, NULL,
		  gdict_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  gdict_defbox_signals[FIND_PREVIOUS] =
    g_signal_new ("find-previous",
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (GdictDefboxClass, find_previous),
                  NULL, NULL,
                  gdict_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
  gdict_defbox_signals[FIND_NEXT] =
    g_signal_new ("find-next",
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (GdictDefboxClass, find_next),
		  NULL, NULL,
		  gdict_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  gdict_defbox_signals[HIDE_FIND] =
    g_signal_new ("hide-find",
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  G_STRUCT_OFFSET (GdictDefboxClass, hide_find),
		  NULL, NULL,
		  gdict_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  gdict_defbox_signals[LINK_CLICKED] =
    g_signal_new ("link-clicked",
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GdictDefboxClass, link_clicked),
                  NULL, NULL,
                  gdict_marshal_VOID__STRING,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);
  
  klass->show_find = gdict_defbox_real_show_find;
  klass->hide_find = gdict_defbox_real_hide_find;
  klass->find_next = gdict_defbox_real_find_next;
  klass->find_previous = gdict_defbox_real_find_previous;
  
  binding_set = gtk_binding_set_by_class (klass);
  gtk_binding_entry_add_signal (binding_set,
  			        GDK_f, GDK_CONTROL_MASK,
  			        "show-find",
  			        0);
  gtk_binding_entry_add_signal (binding_set,
  			        GDK_g, GDK_CONTROL_MASK,
  			        "find-next",
  			        0);
  gtk_binding_entry_add_signal (binding_set,
  				GDK_g, GDK_SHIFT_MASK | GDK_CONTROL_MASK,
  				"find-previous",
  				0);
  gtk_binding_entry_add_signal (binding_set,
  			        GDK_Escape, 0,
  			        "hide-find",
  			        0);

  g_type_class_add_private (klass, sizeof (GdictDefboxPrivate));
}

static void
gdict_defbox_init (GdictDefbox *defbox)
{
  GdictDefboxPrivate *priv;
  
  gtk_box_set_spacing (GTK_BOX (defbox), 6);
  
  priv = GDICT_DEFBOX_GET_PRIVATE (defbox);
  defbox->priv = priv;
  
  priv->context = NULL;
  priv->database = g_strdup (GDICT_DEFAULT_DATABASE);
  priv->font_name = g_strdup (GDICT_DEFAULT_FONT_NAME);
  priv->word = NULL;
  
  priv->definitions = NULL;
  
  priv->busy_cursor = NULL;
  priv->hand_cursor = NULL;
  priv->regular_cursor = NULL;
  
  priv->show_find = FALSE;
  priv->is_searching = FALSE;
  priv->is_hovering = FALSE;

  priv->hide_timeout = 0;
}

/**
 * gdict_defbox_new:
 *
 * Creates a new #GdictDefbox widget.  Use this widget to search for
 * a word using a #GdictContext, and to show the resulting definition(s).
 * You must set a #GdictContext for this widget using
 * gdict_defbox_set_context().
 *
 * Return value: a new #GdictDefbox widget.
 *
 * Since: 0.1
 */
GtkWidget *
gdict_defbox_new (void)
{
  return g_object_new (GDICT_TYPE_DEFBOX, NULL);
}

/**
 * gdict_defbox_new_with_context:
 * @context: a #GdictContext
 *
 * Creates a new #GdictDefbox widget. Use this widget to search for
 * a word using @context, and to show the resulting definition.
 *
 * Return value: a new #GdictDefbox widget.
 *
 * Since: 0.1
 */
GtkWidget *
gdict_defbox_new_with_context (GdictContext *context)
{
  g_return_val_if_fail (GDICT_IS_CONTEXT (context), NULL);
  
  return g_object_new (GDICT_TYPE_DEFBOX, "context", context, NULL);
}

/**
 * gdict_defbox_set_context:
 * @defbox: a #GdictDefbox
 * @context: a #GdictContext
 *
 * Sets @context as the #GdictContext to be used by @defbox in order
 * to retrieve the definitions of a word.
 *
 * Since: 0.1
 */
void
gdict_defbox_set_context (GdictDefbox  *defbox,
			  GdictContext *context)
{
  g_return_if_fail (GDICT_IS_DEFBOX (defbox));
  g_return_if_fail (context == NULL || GDICT_IS_CONTEXT (context));
  
  g_object_set (defbox, "context", context, NULL);
}

/**
 * gdict_defbox_get_context:
 * @defbox: a #GdictDefbox
 *
 * Gets the #GdictContext used by @defbox.
 *
 * Return value: a #GdictContext.
 *
 * Since: 0.1
 */
GdictContext *
gdict_defbox_get_context (GdictDefbox *defbox)
{
  g_return_val_if_fail (GDICT_IS_DEFBOX (defbox), NULL);

  return defbox->priv->context;
}

/**
 * gdict_defbox_set_database:
 * @defbox: a #GdictDefbox
 * @database: a database
 *
 * Sets @database as the database used by the #GdictContext bound to @defbox
 * to query for word definitions.
 *
 * Since: 0.1
 */
void
gdict_defbox_set_database (GdictDefbox *defbox,
			   const gchar *database)
{
  GdictDefboxPrivate *priv;

  g_return_if_fail (GDICT_IS_DEFBOX (defbox));

  priv = defbox->priv;

  g_free (priv->database);
  priv->database = g_strdup (database);

  g_object_notify (G_OBJECT (defbox), "database");
}

/**
 * gdict_defbox_get_database:
 * @defbox: a #GdictDefbox
 *
 * Gets the database used by @defbox.  See gdict_defbox_set_database().
 *
 * Return value: the name of a database. The return string is owned by
 *   the #GdictDefbox widget and should not be modified or freed.
 *
 * Since: 0.1
 */
G_CONST_RETURN gchar *
gdict_defbox_get_database (GdictDefbox *defbox)
{
  g_return_val_if_fail (GDICT_IS_DEFBOX (defbox), NULL);

  return defbox->priv->database;
}

/**
 * gdict_defbox_get_word:
 * @defbox: a #GdictDefbox
 *
 * Retrieves the word being looked up.
 *
 * Return value: the word looked up, or %NULL. The returned string is
 *   owned by the #GdictDefbox widget and should never be modified or
 *   freed.
 *
 * Since: 0.12
 */
G_CONST_RETURN gchar *
gdict_defbox_get_word (GdictDefbox *defbox)
{
  g_return_val_if_fail (GDICT_IS_DEFBOX (defbox), NULL);

  return defbox->priv->word;
}

/**
 * gdict_defbox_set_show_find:
 * @defbox: a #GdictDefbox
 * @show_find: %TRUE to show the find pane
 *
 * Whether @defbox should show the find pane.
 *
 * Since: 0.1
 */
void
gdict_defbox_set_show_find (GdictDefbox *defbox,
			    gboolean     show_find)
{
  GdictDefboxPrivate *priv;
  
  g_return_if_fail (GDICT_IS_DEFBOX (defbox));
  
  priv = defbox->priv;
  
  if (priv->show_find == show_find)
    return;
  
  priv->show_find = show_find;
  if (priv->show_find)
    {
      gtk_widget_show_all (priv->find_pane);
      gtk_widget_grab_focus (priv->find_entry);

      if (!priv->hide_timeout)
        priv->hide_timeout = g_timeout_add (5000, hide_find_pane, defbox);
    }
  else
    {
      gtk_widget_hide (priv->find_pane);

      if (priv->hide_timeout)
        {
          g_source_remove (priv->hide_timeout);
          priv->hide_timeout = 0;
        }
    }
}

/**
 * gdict_defbox_get_show_find:
 * @defbox: a #GdictDefbox
 *
 * Gets whether the find pane should be visible or not.
 *
 * Return value: %TRUE if the find pane is visible.
 *
 * Since: 0.1
 */
gboolean
gdict_defbox_get_show_find (GdictDefbox *defbox)
{
  g_return_val_if_fail (GDICT_IS_DEFBOX (defbox), FALSE);
  
  return (defbox->priv->show_find == TRUE);
}

static void
lookup_start_cb (GdictContext *context,
		 gpointer      user_data)
{
  GdictDefbox *defbox = GDICT_DEFBOX (user_data);
  GdictDefboxPrivate *priv = defbox->priv;
  GdkWindow *window;

  priv->is_searching = TRUE;
  
  if (!priv->busy_cursor)
    priv->busy_cursor = gdk_cursor_new (GDK_WATCH);
  
  window = gtk_text_view_get_window (GTK_TEXT_VIEW (priv->text_view),
  				     GTK_TEXT_WINDOW_WIDGET);
  
  gdk_window_set_cursor (window, priv->busy_cursor);
}

static void
lookup_end_cb (GdictContext *context,
	       gpointer      user_data)
{
  GdictDefbox *defbox = GDICT_DEFBOX (user_data);
  GdictDefboxPrivate *priv = defbox->priv;
  GtkTextBuffer *buffer;
  GtkTextIter start;
  GdkWindow *window;
  
  /* explicitely move the cursor to the beginning */
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->text_view));
  gtk_text_buffer_get_start_iter (buffer, &start);
  gtk_text_buffer_place_cursor (buffer, &start);
  
  window = gtk_text_view_get_window (GTK_TEXT_VIEW (priv->text_view),
  				     GTK_TEXT_WINDOW_WIDGET);
  
  gdk_window_set_cursor (window, NULL);

  priv->is_searching = FALSE;
}

static void
gdict_defbox_insert_word (GdictDefbox *defbox,
			  GtkTextIter *iter,
			  const gchar *word)
{
  GdictDefboxPrivate *priv;
  gchar *text;
  
  if (!word)
    return;
  
  g_assert (GDICT_IS_DEFBOX (defbox));
  priv = defbox->priv;
  
  g_assert (GTK_IS_TEXT_BUFFER (priv->buffer));
  
  text = g_strdup_printf ("%s\n", word);
  gtk_text_buffer_insert_with_tags_by_name (priv->buffer,
  					    iter,
  					    text, strlen (text),
  					    "big", "bold", "query-title",
  					    NULL);
  g_free (text);
}

/* escape a link string; links are expressed as "{...}".
 * the link with the '{}' removed is stored inside link_str, while
 * the returned value is a pointer to what follows the trailing '}'.
 * link_str is allocated and should be freed.
 */
static const gchar *
escape_link (const gchar  *str,
             gchar       **link_str)
{
  gsize str_len;
  GString *link_buf;
  const gchar *p;

  str_len = strlen (str);
  link_buf = g_string_sized_new (str_len - 2);

  for (p = str + 1; *p != '}'; p++)
    {
      link_buf = g_string_append_c (link_buf, *p);
    }

  if (link_str)
    *link_str = g_string_free (link_buf, FALSE);

  p++;

  return p;
}

static const gchar *
escape_phonethic (const gchar  *str,
                  gchar       **phon_str)
{
  gsize str_len;
  GString *phon_buf;
  const gchar *p;

  str_len = strlen (str);
  phon_buf = g_string_sized_new (str_len - 2);

  for (p = str + 1; *p != '\\'; p++)
    {
      phon_buf = g_string_append_c (phon_buf, *p);
    }

  if (phon_str)
    *phon_str = g_string_free (phon_buf, FALSE);

  p++;

  return p;
}

static void
gdict_defbox_insert_body (GdictDefbox *defbox,
			  GtkTextIter *iter,
			  const gchar *body)
{
  GdictDefboxPrivate *priv;
  gchar **words;
  gint len, i;
  GtkTextIter end_iter;
  
  if (!body)
    return;
  
  g_assert (GDICT_IS_DEFBOX (defbox));
  priv = defbox->priv;
  
  g_assert (GTK_IS_TEXT_BUFFER (priv->buffer));

  words = g_strsplit (body, " ", -1);
  len = g_strv_length (words);
  end_iter = *iter;

  for (i = 0; i < len; i++)
    {
      gchar *w = words[i];
      gint w_len = strlen (w);
      gchar *begin, *end;

      if (w_len == 0)
        continue;

      begin = g_utf8_offset_to_pointer (w, 0);

      if (*begin == '{')
        {
          end = g_utf8_strrchr (w, -1, '}');

          /* see this is a self contained link */
          if (end && *end == '}')
            {
              const gchar *rest;
              gchar *link_str;

              rest = escape_link (w, &link_str);

              gtk_text_buffer_insert_with_tags_by_name (priv->buffer,
                                                        &end_iter,
                                                        link_str, -1,
                                                        "link",
                                                        NULL);

              gtk_text_buffer_insert (priv->buffer, &end_iter, rest, -1);

              gtk_text_buffer_get_end_iter (priv->buffer, &end_iter);
              gtk_text_buffer_insert (priv->buffer, &end_iter, " ", 1);

              g_free (link_str);

              continue;
            }
          else
            {
              /* uh-oh: the link ends in another word */
              GString *buf;
              gchar *next;
              gint cur = i;

              buf = g_string_new (NULL);
              next = words[cur++];

              while (next && (end = g_utf8_strrchr (next, -1, '}')) == NULL)
                {
                  buf = g_string_append (buf, next);
                  buf = g_string_append_c (buf, ' ');

                  next = words[cur++];
                }

              buf = g_string_append (buf, next);

              next = g_string_free (buf, FALSE);

              if (end && *end == '}')
                {
                  const gchar *rest;
                  gchar *link_str;

                  rest = escape_link (next, &link_str);

                  gtk_text_buffer_insert_with_tags_by_name (priv->buffer,
                                                            &end_iter,
                                                            link_str, -1,
                                                            "link",
                                                            NULL);

                  gtk_text_buffer_insert (priv->buffer, &end_iter, rest, -1);
                  gtk_text_buffer_insert (priv->buffer, &end_iter, " ", 1);

                  g_free (link_str);
                }

              g_free (next);
              i = cur;

              continue;
            }
        }
      else if (*begin == '\\')
        {
          end = g_utf8_strrchr (w, -1, '\\');

          if (end && *end == '\\')
            {
              const gchar *rest;
              gchar *phon;

              rest = escape_phonethic (w, &phon);

              gtk_text_buffer_insert_with_tags_by_name (priv->buffer,
                                                        &end_iter,
                                                        phon, -1,
                                                        "italic", "phonetic",
                                                        NULL);

              gtk_text_buffer_insert (priv->buffer, &end_iter, rest, -1);

              gtk_text_buffer_get_end_iter (priv->buffer, &end_iter);
              gtk_text_buffer_insert (priv->buffer, &end_iter, " ", -1);

              g_free (phon);

              continue;
            }
        }
      
      gtk_text_buffer_insert (priv->buffer, &end_iter, w, w_len);

      gtk_text_buffer_get_end_iter (priv->buffer, &end_iter);
      gtk_text_buffer_insert (priv->buffer, &end_iter, " ", 1);
    }

  gtk_text_buffer_get_end_iter (priv->buffer, &end_iter);
  gtk_text_buffer_insert (priv->buffer, &end_iter, "\n", 1);

  *iter = end_iter;

  g_strfreev (words);
}

static void
gdict_defbox_insert_from (GdictDefbox *defbox,
			  GtkTextIter *iter,
			  const gchar *database)
{
  GdictDefboxPrivate *priv;
  gchar *text;
  
  if (!database)
    return;
  
  g_assert (GDICT_IS_DEFBOX (defbox));
  priv = defbox->priv;
  
  g_assert (GTK_IS_TEXT_BUFFER (priv->buffer));
  
  text = g_strdup_printf ("\t-- From %s\n\n", database);
  gtk_text_buffer_insert_with_tags_by_name (priv->buffer,
  					    iter,
  					    text, strlen (text),
  					    "small", "query-from",
  					    NULL);
  g_free (text);
}

static void
gdict_defbox_insert_error (GdictDefbox *defbox,
			   GtkTextIter *iter,
			   const gchar *title,
			   const gchar *message)
{
  GdictDefboxPrivate *priv;
  GtkTextMark *mark;
  GtkTextIter cur_iter;
  
  if (!title)
    return;
  
  g_assert (GDICT_IS_DEFBOX (defbox));
  priv = defbox->priv;
  
  g_assert (GTK_IS_TEXT_BUFFER (priv->buffer));

  mark = gtk_text_buffer_create_mark (priv->buffer, "block-cursor", iter, FALSE);
  gtk_text_buffer_get_iter_at_mark (priv->buffer, &cur_iter, mark);
  
  gtk_text_buffer_insert_with_tags_by_name (priv->buffer,
  					    &cur_iter,
  					    title, strlen (title),
  					    "error-title", "big", "bold",
  					    NULL);
  gtk_text_buffer_get_iter_at_mark (priv->buffer, &cur_iter, mark);

  gtk_text_buffer_insert (priv->buffer, &cur_iter, "\n\n", -1);
  gtk_text_buffer_get_iter_at_mark (priv->buffer, &cur_iter, mark);

  gtk_text_buffer_insert_with_tags_by_name (priv->buffer,
		  			    &cur_iter,
					    message, strlen (message),
					    "error-message",
					    NULL);
}

static void
definition_found_cb (GdictContext    *context,
		     GdictDefinition *definition,
		     gpointer         user_data)
{
  GdictDefbox *defbox = GDICT_DEFBOX (user_data);
  GdictDefboxPrivate *priv = defbox->priv;
  GtkTextIter iter;
  Definition *def;
 
  /* insert the word if this is the first definition */
  if (!priv->definitions)
    {
      gtk_text_buffer_get_start_iter (priv->buffer, &iter);
      gdict_defbox_insert_word (defbox, &iter,
				gdict_definition_get_word (definition));
    }
  
  def = definition_new ();
  
  gtk_text_buffer_get_end_iter (priv->buffer, &iter);
  def->begin = gtk_text_iter_get_offset (&iter);
  gdict_defbox_insert_body (defbox, &iter, gdict_definition_get_text (definition));
  
  gtk_text_buffer_get_end_iter (priv->buffer, &iter);
  gdict_defbox_insert_from (defbox, &iter, gdict_definition_get_database (definition));
  
  def->definition = gdict_definition_ref (definition);
  
  priv->definitions = g_slist_append (priv->definitions, def);
}

static void
error_cb (GdictContext *context,
	  const GError *error,
	  gpointer      user_data)
{
  GdictDefbox *defbox = GDICT_DEFBOX (user_data);
  GdictDefboxPrivate *priv = defbox->priv;
  GtkTextIter iter;

  if (!error)
    return;
  
  gdict_defbox_clear (defbox);

  gtk_text_buffer_get_start_iter (priv->buffer, &iter);
  gdict_defbox_insert_error (defbox, &iter,
		             _("Error while looking up definition"),
		             error->message);
  
  g_free (priv->word);
  priv->word = NULL;
  
  defbox->priv->is_searching = FALSE;
}

/**
 * gdict_defbox_lookup:
 * @defbox: a #GdictDefbox
 * @word: the word to look up
 *
 * Searches @word inside the dictionary sources using the #GdictContext
 * provided when creating @defbox or set using gdict_defbox_set_context().
 *
 * Since: 0.1
 */
void
gdict_defbox_lookup (GdictDefbox *defbox,
		     const gchar *word)
{
  GdictDefboxPrivate *priv;
  GError *define_error;
  
  g_return_if_fail (GDICT_IS_DEFBOX (defbox));
  
  priv = defbox->priv;
  
  if (!priv->context)
    {
      g_warning ("Attempting to look up `%s', but no GdictContext "
		 "has been set.  Use gdict_defbox_set_context() "
		 "before invoking gdict_defbox_lookup().",
                 word);
      return;
    }
  
  if (priv->is_searching)
    {
      _gdict_show_error_dialog (GTK_WIDGET (defbox),
      			        _("Another search is in progress"),
      			        _("Please wait until the current search ends."));
      
      return;
    }

  gdict_defbox_clear (defbox);
  
  if (!priv->start_id)
    {
      priv->start_id = g_signal_connect (priv->context, "lookup-start",
  				         G_CALLBACK (lookup_start_cb),
					 defbox);
      priv->define_id = g_signal_connect (priv->context, "definition-found",
  				          G_CALLBACK (definition_found_cb),
  				          defbox);
      priv->end_id = g_signal_connect (priv->context, "lookup-end",
  				       G_CALLBACK (lookup_end_cb),
  				       defbox);
    }
  
  if (!priv->error_id)
    priv->error_id = g_signal_connect (priv->context, "error",
  				       G_CALLBACK (error_cb),
  				       defbox);
  
  priv->word = g_strdup (word);
  g_object_notify (G_OBJECT (defbox), "word");
  
  define_error = NULL;
  gdict_context_define_word (priv->context,
  			     priv->database,
  			     word,
  			     &define_error);
  if (define_error)
    {
      GtkTextIter iter;

      gtk_text_buffer_get_start_iter (priv->buffer, &iter);
      gdict_defbox_insert_error (defbox, &iter,
    			         _("Error while retrieving the definition"),
				 define_error->message);

      g_error_free (define_error);
    }
}

/**
 * gdict_defbox_clear:
 * @defbox: a @GdictDefbox
 *
 * Clears the buffer of @defbox
 *
 * Since: 0.1
 */
void
gdict_defbox_clear (GdictDefbox *defbox)
{
  GdictDefboxPrivate *priv;
  GtkTextIter start, end;
  
  g_return_if_fail (GDICT_IS_DEFBOX (defbox));
  
  priv = defbox->priv;

  /* destroy previously found definitions */
  if (priv->definitions)
    {
      g_slist_foreach (priv->definitions,
      		       (GFunc) definition_free,
      		       NULL);
      g_slist_free (priv->definitions);
      
      priv->definitions = NULL;
    }
  
  gtk_text_buffer_get_bounds (priv->buffer, &start, &end);
  gtk_text_buffer_delete (priv->buffer, &start, &end);
}

/**
 * gdict_defbox_find_next:
 * @defbox: a #GdictDefbox
 * 
 * Emits the "find-next" signal.
 * 
 * Since: 0.1
 */
void
gdict_defbox_find_next (GdictDefbox *defbox)
{
  g_return_if_fail (GDICT_IS_DEFBOX (defbox));

  g_signal_emit (defbox, gdict_defbox_signals[FIND_NEXT], 0);
}

/**
 * gdict_defbox_find_previous:
 * @defbox: a #GdictDefbox
 * 
 * Emits the "find-previous" signal.
 * 
 * Since: 0.1
 */
void
gdict_defbox_find_previous (GdictDefbox *defbox)
{
  g_return_if_fail (GDICT_IS_DEFBOX (defbox));

  g_signal_emit (defbox, gdict_defbox_signals[FIND_PREVIOUS], 0);
}

/**
 * gdict_defbox_select_all:
 * @defbox: a #GdictDefbox
 *
 * Selects all the text displayed by @defbox
 * 
 * Since: 0.1
 */
void
gdict_defbox_select_all (GdictDefbox *defbox)
{
  GdictDefboxPrivate *priv;
  GtkTextBuffer *buffer;
  GtkTextIter start, end;
  
  g_return_if_fail (GDICT_IS_DEFBOX (defbox));
  
  priv = defbox->priv;
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->text_view));
  
  gtk_text_buffer_get_bounds (buffer, &start, &end);
  gtk_text_buffer_select_range (buffer, &start, &end);
}

/**
 * gdict_defbox_copy_to_clipboard:
 * @defbox: a #GdictDefbox
 * @clipboard: a #GtkClipboard
 *
 * Copies the selected text inside @defbox into @clipboard.
 *
 * Since: 0.1
 */
void
gdict_defbox_copy_to_clipboard (GdictDefbox  *defbox,
				GtkClipboard *clipboard)
{
  GdictDefboxPrivate *priv;
  GtkTextBuffer *buffer;
  
  g_return_if_fail (GDICT_IS_DEFBOX (defbox));
  g_return_if_fail (GTK_IS_CLIPBOARD (clipboard));

  priv = defbox->priv;
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->text_view));

  gtk_text_buffer_copy_clipboard (buffer, clipboard);
}

/**
 * gdict_defbox_count_definitions:
 * @defbox: a #GdictDefbox
 *
 * Gets the number of definitions displayed by @defbox
 *
 * Return value: the number of definitions.
 *
 * Since: 0.1
 */
gint
gdict_defbox_count_definitions (GdictDefbox *defbox)
{
  GdictDefboxPrivate *priv;
  
  g_return_val_if_fail (GDICT_IS_DEFBOX (defbox), -1);
  
  priv = defbox->priv;
  if (!priv->definitions)
    return -1;
  
  return g_slist_length (priv->definitions);
}

/**
 * gdict_defbox_jump_to_definition:
 * @defbox: a #GdictDefbox
 * @number: the definition to jump to
 *
 * Scrolls to the definition identified by @number.  If @number is -1,
 * jumps to the last definition.
 *
 * Since: 0.1
 */
void
gdict_defbox_jump_to_definition (GdictDefbox *defbox,
				 gint         number)
{
  GdictDefboxPrivate *priv;
  gint count;
  Definition *def;
  GtkTextBuffer *buffer;
  GtkTextIter def_start;
  
  g_return_if_fail (GDICT_IS_DEFBOX (defbox));
  
  count = gdict_defbox_count_definitions (defbox) - 1;
  if (count == -1)
    return;
  
  if ((number == -1) || (number > count))
    number = count;
  
  priv = defbox->priv;
  def = (Definition *) g_slist_nth_data (priv->definitions, number);
  if (!def)
    return;
  
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->text_view));
  gtk_text_buffer_get_iter_at_offset (buffer, &def_start, def->begin);
  gtk_text_view_scroll_to_iter (GTK_TEXT_VIEW (priv->text_view),
      				&def_start,
      				0.0,
      				TRUE,
      				0.0, 0.0);
}

/**
 * gdict_defbox_get_text:
 * @defbox: a #GdictDefbox
 * @length: return location for the text length or %NULL
 *
 * Gets the full contents of @defbox.
 *
 * Return value: a newly allocated string containing the text displayed by
 *   @defbox.
 *
 * Since: 0.1
 */
gchar *
gdict_defbox_get_text (GdictDefbox *defbox,
		       gsize       *length)
{
  GdictDefboxPrivate *priv;
  GtkTextBuffer *buffer;
  GtkTextIter start, end;
  gchar *retval;
  
  g_return_val_if_fail (GDICT_IS_DEFBOX (defbox), NULL);
  
  priv = defbox->priv;
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->text_view));
  
  gtk_text_buffer_get_bounds (buffer, &start, &end);
  
  retval = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
  
  if (length)
    *length = strlen (retval);
  
  return retval;
}

/**
 * gdict_defbox_set_font_name:
 * @defbox: a #GdictDefbox
 * @font_name: a font description, or %NULL
 *
 * Sets @font_name as the font for @defbox. It calls internally
 * pango_font_description_from_string() and gtk_widget_modify_font().
 * 
 * Passing %NULL for @font_name will reset any previously set font.
 *
 * Since: 0.3.0
 */
void
gdict_defbox_set_font_name (GdictDefbox *defbox,
			    const gchar *font_name)
{
  GdictDefboxPrivate *priv;
  PangoFontDescription *font_desc;

  g_return_if_fail (GDICT_IS_DEFBOX (defbox));

  priv = defbox->priv;

  if (font_name)
    {
      font_desc = pango_font_description_from_string (font_name);
      g_return_if_fail (font_desc != NULL);
    }
  else
    font_desc = NULL;

  gtk_widget_modify_font (priv->text_view, font_desc);

  if (font_desc)
    pango_font_description_free (font_desc);

  g_free (priv->font_name);
  priv->font_name = g_strdup (font_name);
}

/**
 * gdict_defbox_get_font_name:
 * @defbox: a #GdictDefbox
 *
 * Retrieves the font currently used by @defbox.
 *
 * Return value: a font name.  The returned string is owned by @defbox and
 *   should not be modified or freed.
 *
 * Since: 0.3
 */
G_CONST_RETURN gchar *
gdict_defbox_get_font_name (GdictDefbox *defbox)
{
  g_return_val_if_fail (GDICT_IS_DEFBOX (defbox), NULL);

  return defbox->priv->font_name;
}

/**
 * gdict_defbox_get_selected_word:
 * @defbox: a #GdictDefbox
 *
 * Retrieves the selected word from the defbox widget
 *
 * Return value: a newly allocated string containing the selected
 *   word. Use g_free() when done using it.
 *
 * Since: 0.12
 */
gchar *
gdict_defbox_get_selected_word (GdictDefbox *defbox)
{
  GdictDefboxPrivate *priv;
  GtkTextBuffer *buffer;

  g_return_val_if_fail (GDICT_IS_DEFBOX (defbox), NULL);

  priv = defbox->priv;
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (priv->text_view));

  if (!gtk_text_buffer_get_has_selection (buffer))
    return NULL;
  else
    {
      GtkTextIter start, end;
      gchar *retval;

      gtk_text_buffer_get_selection_bounds (buffer, &start, &end);
      retval = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);

      return retval;
    }
}
