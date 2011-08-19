/* Copyright (C) 1991, 1992, 1993 Free Software Foundation, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* Stripped-down UTF-8 fnmatch() lifted from GTK+ */

#include <string.h>
#include <glib.h>

static gunichar
get_char (const char **str)
{
  gunichar c = g_utf8_get_char (*str);
  *str = g_utf8_next_char (*str);

#ifdef G_PLATFORM_WIN32
  c = g_unichar_tolower (c);
#endif

  return c;
}

#if defined(G_OS_WIN32) || defined(G_WITH_CYGWIN)
#define DO_ESCAPE 0
#else  
#define DO_ESCAPE 1
#endif  

static gunichar
get_unescaped_char (const char **str,
		    gboolean    *was_escaped)
{
  gunichar c = get_char (str);

  *was_escaped = DO_ESCAPE && c == '\\';
  if (*was_escaped)
    c = get_char (str);
  
  return c;
}

/* Match STRING against the filename pattern PATTERN, returning zero if
   it matches, nonzero if not.  */

static gboolean
gtk_fnmatch_intern (const char *pattern,
		    const char *string,
		    gboolean    component_start,
		    gboolean    no_leading_period)
{
  const char *p = pattern, *n = string;
  
  while (*p)
    {
      const char *last_n = n;
      
      gunichar c = get_char (&p);
      gunichar nc = get_char (&n);
      
      switch (c)
	{
   	case '?':
	  if (nc == '\0')
	    return FALSE;
	  else if (nc == G_DIR_SEPARATOR)
	    return FALSE;
	  else if (nc == '.' && component_start && no_leading_period)
	    return FALSE;
	  break;
	case '\\':
	  if (DO_ESCAPE)
	    c = get_char (&p);
	  if (nc != c)
	    return FALSE;
	  break;
	case '*':
	  if (nc == '.' && component_start && no_leading_period)
	    return FALSE;

	  {
	    const char *last_p = p;

	    for (last_p = p, c = get_char (&p);
		 c == '?' || c == '*';
		 last_p = p, c = get_char (&p))
	      {
		if (c == '?')
		  {
		    if (nc == '\0')
		      return FALSE;
		    else if (nc == G_DIR_SEPARATOR)
		      return FALSE;
		    else
		      {
			last_n = n; nc = get_char (&n);
		      }
		  }
	      }

	    /* If the pattern ends with wildcards, we have a
	     * guaranteed match unless there is a dir separator
	     * in the remainder of the string.
	     */
	    if (c == '\0')
	      {
		if (strchr (last_n, G_DIR_SEPARATOR) != NULL)
		  return FALSE;
		else
		  return TRUE;
	      }

	    if (DO_ESCAPE && c == '\\')
	      c = get_char (&p);

	    for (p = last_p; nc != '\0';)
	      {
		if ((c == '[' || nc == c) &&
		    gtk_fnmatch_intern (p, last_n, component_start, no_leading_period))
		  return TRUE;
		
		component_start = (nc == G_DIR_SEPARATOR);
		last_n = n;
		nc = get_char (&n);
	      }
		  
	    return FALSE;
	  }

	case '[':
	  {
	    /* Nonzero if the sense of the character class is inverted.  */
	    gboolean not;
	    gboolean was_escaped;

	    if (nc == '\0' || nc == G_DIR_SEPARATOR)
	      return FALSE;

	    if (nc == '.' && component_start && no_leading_period)
	      return FALSE;

	    not = (*p == '!' || *p == '^');
	    if (not)
	      ++p;

	    c = get_unescaped_char (&p, &was_escaped);
	    for (;;)
	      {
		register gunichar cstart = c, cend = c;
		if (c == '\0')
		  /* [ (unterminated) loses.  */
		  return FALSE;

		c = get_unescaped_char (&p, &was_escaped);
		
		if (!was_escaped && c == '-' && *p != ']')
		  {
		    cend = get_unescaped_char (&p, &was_escaped);
		    if (cend == '\0')
		      return FALSE;

		    c = get_char (&p);
		  }

		if (nc >= cstart && nc <= cend)
		  goto matched;

		if (!was_escaped && c == ']')
		  break;
	      }
	    if (!not)
	      return FALSE;
	    break;

	  matched:;
	    /* Skip the rest of the [...] that already matched.  */
	    /* XXX 1003.2d11 is unclear if was_escaped is right.  */
	    while (was_escaped || c != ']')
	      {
		if (c == '\0')
		  /* [... (unterminated) loses.  */
		  return FALSE;

		c = get_unescaped_char (&p, &was_escaped);
	      }
	    if (not)
	      return FALSE;
	  }
	  break;

	default:
	  if (c != nc)
	    return FALSE;
	}

      component_start = (nc == G_DIR_SEPARATOR);
    }

  if (*n == '\0')
    return TRUE;

  return FALSE;
}

/* Match STRING against the filename pattern PATTERN, returning zero if
 *  it matches, nonzero if not.
 *
 * GTK+ used to use a old version of GNU fnmatch() that was buggy
 * in various ways and didn't handle UTF-8. The following is
 * converted to UTF-8. To simplify the process of making it
 * correct, this is special-cased to the combinations of flags
 * that gtkfilesel.c uses.
 *
 *   FNM_FILE_NAME   - always set
 *   FNM_LEADING_DIR - never set
 *   FNM_NOESCAPE    - set only on windows
 *   FNM_CASEFOLD    - set only on windows
 */
int 
fnmatch_utf8 (const char *pattern,
	      const char *string,
	      int         flags)
{
  /* The xdgmime code always calls us with flags==0, make sure it
   * stays like that, since GTK+'s stripped-down fnmatch() doesn't
   * handle any of the flags. Or actually, as it says above, it acts
   * as if FNM_FILE_NAME was always set, so let's hope that doesn't
   * matter.
   */
  g_assert (flags == 0);
  /* GTK+'s _gtk_fnmatch() returns TRUE on match, FALSE on
   * non-match. The real fnmatch() returns 0 on match, non-zero on
   * non-match. Here we want to work like the real fnmatch(), thus the
   * NOT operator.
   */
  return !gtk_fnmatch_intern (pattern, string, TRUE, FALSE);
}
