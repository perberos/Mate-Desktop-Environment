/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* mate-vfs-utils.c - Private utility functions for the MATE Virtual
   File System.

   Copyright (C) 1999 Free Software Foundation
   Copyright (C) 2000, 2001 Eazel, Inc.

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
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Authors: Ettore Perazzoli <ettore@comm2000.it>
   	    John Sullivan <sullivan@eazel.com> 
            Darin Adler <darin@eazel.com>
*/

#include <config.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include "mate-vfs-utils.h"

#include "mate-vfs-private-utils.h"
#include "mate-vfs-ops.h"
#include "mate-vfs-mime-handlers.h"
#include "mate-vfs-mime-private.h"
#include "mate-vfs-method.h"
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>
#include <mateconf/mateconf-client.h>
#ifndef G_OS_WIN32
#include <pwd.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define KILOBYTE_FACTOR 1024.0
#define MEGABYTE_FACTOR (1024.0 * 1024.0)
#define GIGABYTE_FACTOR (1024.0 * 1024.0 * 1024.0)

#define READ_CHUNK_SIZE 8192

#define MAX_SYMLINKS_FOLLOWED 32


/**
 * mate_vfs_format_file_size_for_display:
 * @size: a #MateVFSFileSize (a #guint64 value).
 * 
 * Formats the file @size passed so that it is easy for
 * the user to read. Gives the size in bytes, kilobytes, megabytes or
 * gigabytes, choosing whatever is appropriate.
 * 
 * Returns: a newly allocated string with the size ready to be shown.
 */

gchar*
mate_vfs_format_file_size_for_display (MateVFSFileSize size)
{
	if (size < (MateVFSFileSize) KILOBYTE_FACTOR) {
		return g_strdup_printf (dngettext(GETTEXT_PACKAGE, "%u byte", "%u bytes",(guint) size), (guint) size);
	} else {
		gdouble displayed_size;

		if (size < (MateVFSFileSize) MEGABYTE_FACTOR) {
			displayed_size = (gdouble) size / KILOBYTE_FACTOR;
			return g_strdup_printf (_("%.1f KB"),
						       displayed_size);
		} else if (size < (MateVFSFileSize) GIGABYTE_FACTOR) {
			displayed_size = (gdouble) size / MEGABYTE_FACTOR;
			return g_strdup_printf (_("%.1f MB"),
						       displayed_size);
		} else {
			displayed_size = (gdouble) size / GIGABYTE_FACTOR;
			return g_strdup_printf (_("%.1f GB"),
						       displayed_size);
		}
	}
}

typedef enum {
	UNSAFE_ALL        = 0x1,  /* Escape all unsafe characters   */
	UNSAFE_ALLOW_PLUS = 0x2,  /* Allows '+'  */
	UNSAFE_PATH       = 0x4,  /* Allows '/' and '&' and '='  */
	UNSAFE_DOS_PATH   = 0x8,  /* Allows '/' and '&' and '=' and ':' */
	UNSAFE_HOST       = 0x10, /* Allows '/' and ':' and '@' */
	UNSAFE_SLASHES    = 0x20  /* Allows all characters except for '/' and '%' */
} UnsafeCharacterSet;

static const guchar acceptable[96] =
{ /*  X0   X1   X2   X3   X4   X5   X6   X7   X8   X9   XA   XB   XC   XD   XE   XF */
  /* spc    !    "    #    $    %    &    '    (    )    *    +    ,    -    .    / */
    0x00,0x3F,0x20,0x20,0x20,0x00,0x2C,0x3F,0x3F,0x3F,0x3F,0x22,0x20,0x3F,0x3F,0x1C, 
  /*   0    1    2    3    4    5    6    7    8    9    :    ;    <    =    >    ? */
    0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x38,0x20,0x20,0x2C,0x20,0x20, 
  /*   @    A    B    C    D    E    F    G    H    I    J    K    L    M    N    O */
    0x30,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F, 
  /*   P    Q    R    S    T    U    V    W    X    Y    Z    [    \    ]    ^    _ */
    0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x20,0x20,0x20,0x20,0x3F, 
  /*   `    a    b    c    d    e    f    g    h    i    j    k    l    m    n    o */
    0x20,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F, 
  /*   p    q    r    s    t    u    v    w    x    y    z    {    |    }    ~  DEL */
    0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x20,0x20,0x20,0x3F,0x20  
};

enum {
	RESERVED = 1,
	UNRESERVED,
	DELIMITERS,
	UNWISE,
	CONTROL,
	SPACE	
};

static const guchar uri_character_kind[128] =
{
    CONTROL   ,CONTROL   ,CONTROL   ,CONTROL   ,CONTROL   ,CONTROL   ,CONTROL   ,CONTROL   , 
    CONTROL   ,CONTROL   ,CONTROL   ,CONTROL   ,CONTROL   ,CONTROL   ,CONTROL   ,CONTROL   ,
    CONTROL   ,CONTROL   ,CONTROL   ,CONTROL   ,CONTROL   ,CONTROL   ,CONTROL   ,CONTROL   ,
    CONTROL   ,CONTROL   ,CONTROL   ,CONTROL   ,CONTROL   ,CONTROL   ,CONTROL   ,CONTROL   ,
    /* ' '        !          "          #          $          %          &          '      */
    SPACE     ,UNRESERVED,DELIMITERS,DELIMITERS,RESERVED  ,DELIMITERS,RESERVED  ,UNRESERVED,
    /*  (         )          *          +          ,          -          .          /      */
    UNRESERVED,UNRESERVED,UNRESERVED,RESERVED  ,RESERVED  ,UNRESERVED,UNRESERVED,RESERVED  , 
    /*  0         1          2          3          4          5          6          7      */
    UNRESERVED,UNRESERVED,UNRESERVED,UNRESERVED,UNRESERVED,UNRESERVED,UNRESERVED,UNRESERVED,
    /*  8         9          :          ;          <          =          >          ?      */
    UNRESERVED,UNRESERVED,RESERVED  ,RESERVED  ,DELIMITERS,RESERVED  ,DELIMITERS,RESERVED  ,
    /*  @         A          B          C          D          E          F          G      */
    RESERVED  ,UNRESERVED,UNRESERVED,UNRESERVED,UNRESERVED,UNRESERVED,UNRESERVED,UNRESERVED,
    /*  H         I          J          K          L          M          N          O      */
    UNRESERVED,UNRESERVED,UNRESERVED,UNRESERVED,UNRESERVED,UNRESERVED,UNRESERVED,UNRESERVED,
    /*  P         Q          R          S          T          U          V          W      */
    UNRESERVED,UNRESERVED,UNRESERVED,UNRESERVED,UNRESERVED,UNRESERVED,UNRESERVED,UNRESERVED,
    /*  X         Y          Z          [          \          ]          ^          _      */
    UNRESERVED,UNRESERVED,UNRESERVED,UNWISE    ,UNWISE    ,UNWISE    ,UNWISE    ,UNRESERVED,
    /*  `         a          b          c          d          e          f          g      */
    UNWISE    ,UNRESERVED,UNRESERVED,UNRESERVED,UNRESERVED,UNRESERVED,UNRESERVED,UNRESERVED,
    /*  h         i          j          k          l          m          n          o      */
    UNRESERVED,UNRESERVED,UNRESERVED,UNRESERVED,UNRESERVED,UNRESERVED,UNRESERVED,UNRESERVED,
    /*  p         q          r          s          t          u          v          w      */
    UNRESERVED,UNRESERVED,UNRESERVED,UNRESERVED,UNRESERVED,UNRESERVED,UNRESERVED,UNRESERVED,
    /*  x         y          z         {           |          }          ~         DEL     */
    UNRESERVED,UNRESERVED,UNRESERVED,UNWISE    ,UNWISE    ,UNWISE    ,UNRESERVED,CONTROL
};


/*  Below modified from libwww HTEscape.c */

#define HEX_ESCAPE '%'

/*  Escape undesirable characters using %
 *  -------------------------------------
 *
 * This function takes a pointer to a string in which
 * some characters may be unacceptable unescaped.
 * It returns a string which has these characters
 * represented by a '%' character followed by two hex digits.
 *
 * This routine returns a g_malloced string.
 */

static const gchar hex[16] = "0123456789ABCDEF";

static gchar *
mate_vfs_escape_string_internal (const gchar *string, 
				  UnsafeCharacterSet mask)
{
#define ACCEPTABLE_CHAR(a) ((a)>=32 && (a)<128 && (acceptable[(a)-32] & use_mask))

	const gchar *p;
	gchar *q;
	gchar *result;
	guchar c;
	gint unacceptable;
	UnsafeCharacterSet use_mask;

	g_return_val_if_fail (mask == UNSAFE_ALL
			      || mask == UNSAFE_ALLOW_PLUS
			      || mask == UNSAFE_PATH
			      || mask == UNSAFE_DOS_PATH
			      || mask == UNSAFE_HOST
			      || mask == UNSAFE_SLASHES, NULL);

	if (string == NULL) {
		return NULL;
	}
	
	unacceptable = 0;
	use_mask = mask;
	for (p = string; *p != '\0'; p++) {
		c = *p;
		if (!ACCEPTABLE_CHAR (c)) {
			unacceptable++;
		}
		if ((use_mask == UNSAFE_HOST) && 
		    (unacceptable || (c == '/'))) {
			/* when escaping a host, if we hit something that needs to be escaped, or we finally
			 * hit a path separator, revert to path mode (the host segment of the url is over).
			 */
			use_mask = UNSAFE_PATH;
		}
	}
	
	result = g_malloc (p - string + unacceptable * 2 + 1);

	use_mask = mask;
	for (q = result, p = string; *p != '\0'; p++){
		c = *p;
		
		if (!ACCEPTABLE_CHAR (c)) {
			*q++ = HEX_ESCAPE; /* means hex coming */
			*q++ = hex[c >> 4];
			*q++ = hex[c & 15];
		} else {
			*q++ = c;
		}
		if ((use_mask == UNSAFE_HOST) &&
		    (!ACCEPTABLE_CHAR (c) || (c == '/'))) {
			use_mask = UNSAFE_PATH;
		}
	}
	
	*q = '\0';
	
	return result;
}

/**
 * mate_vfs_escape_string:
 * @string: string to be escaped.
 *
 * Escapes @string, replacing any and all special characters 
 * with equivalent escape sequences.
 *
 * Return value: a newly allocated string equivalent to @string
 * but with all special characters escaped.
 */
gchar *
mate_vfs_escape_string (const gchar *string)
{
	return mate_vfs_escape_string_internal (string, UNSAFE_ALL);
}

/**
 * mate_vfs_escape_path_string:
 * @path: string to be escaped.
 *
 * Escapes @path, replacing only special characters that would not
 * be found in paths (so '/', '&', and '=' will not be escaped by
 * this function).
 *
 * Return value: a newly allocated string equivalent to @path but
 * with non-path characters escaped.
 */
gchar *
mate_vfs_escape_path_string (const gchar *path)
{
#ifdef G_OS_WIN32
	gchar *path_copy = g_strdup (path);
	gchar *p = path_copy;
	gchar *retval;

	/* Replace backslashes with slashes, that is what is used in
	 * Explorer's "Internet Shortcuts" (.url files), so presumably
	 * it's the standard.
	 */
	while (*p) {
		if (*p == '\\')
			*p = '/';
		p++;
	}
	/* Paths starting with a drive letter get an extra leading slash */
	if (g_ascii_isalpha (path_copy[0]) && path_copy[1] == ':') {
		gchar *escaped_rest = mate_vfs_escape_string_internal (path_copy+2, UNSAFE_PATH);
		path_copy[2] = '\0';
		retval = g_strconcat ("/", path_copy, escaped_rest, NULL);
		g_free (escaped_rest);
	} else
		retval = mate_vfs_escape_string_internal (path_copy, UNSAFE_PATH);
	g_free (path_copy);

	return retval;
#else
	return mate_vfs_escape_string_internal (path, UNSAFE_PATH);
#endif
}

/**
 * mate_vfs_escape_host_and_path_string:
 * @path: string to be escaped.
 *
 * Escapes @path, replacing only special characters that would not
 * be found in paths or host name (so '/', '&', '=', ':' and '@'
 * will not be escaped by this function).
 *
 * Return value: a newly allocated string equivalent to @path but
 * with non-path/host characters escaped.
 */
gchar *
mate_vfs_escape_host_and_path_string (const gchar *path)
{
	return mate_vfs_escape_string_internal (path, UNSAFE_HOST);
}

/**
 * mate_vfs_escape_slashes:
 * @string: string to be escaped.
 *
 * Escapes only '/' and '%' characters in @string, replacing
 * them with their escape sequence equivalents.
 *
 * Return value: a newly allocated string equivalent to @string,
 * but with no unescaped '/' or '%' characters.
 */
gchar *
mate_vfs_escape_slashes (const gchar *string)
{
	return mate_vfs_escape_string_internal (string, UNSAFE_SLASHES);
}

/**
 * mate_vfs_escape_set:
 * @string: string to be escaped.
 * @match_set: a string containing all characters to be escaped in @string. 
 *  
 * Escapes all characters in @string which are listed in @match_set.  
 * 
 * Return value: a newly allocated string equivalent to @string but 
 * with characters in @match_string escaped.
 */
char *
mate_vfs_escape_set (const char *string,
	              const char *match_set)
{
	char *result;
	const char *scanner;
	char *result_scanner;
	int escape_count;

	escape_count = 0;

	if (string == NULL) {
		return NULL;
	}

	if (match_set == NULL) {
		return g_strdup (string);
	}
	
	for (scanner = string; *scanner != '\0'; scanner++) {
		if (strchr(match_set, *scanner) != NULL) {
			/* this character is in the set of characters 
			 * we want escaped.
			 */
			escape_count++;
		}
	}
	
	if (escape_count == 0) {
		return g_strdup (string);
	}

	/* allocate two extra characters for every character that
	 * needs escaping and space for a trailing zero
	 */
	result = g_malloc (scanner - string + escape_count * 2 + 1);
	for (scanner = string, result_scanner = result; *scanner != '\0'; scanner++) {
		if (strchr(match_set, *scanner) != NULL) {
			/* this character is in the set of characters 
			 * we want escaped.
			 */
			*result_scanner++ = HEX_ESCAPE;
			*result_scanner++ = hex[*scanner >> 4];
			*result_scanner++ = hex[*scanner & 15];
			
		} else {
			*result_scanner++ = *scanner;
		}
	}

	*result_scanner = '\0';

	return result;
}

/**
 * mate_vfs_expand_initial_tilde:
 * @path: a local file path which may start with a '~'.
 *
 * If @path starts with a ~, representing the user's home
 * directory, expand it to the actual path location.
 *
 * Return value: a newly allocated string with the initial
 * tilde (if there was one) converted to an actual path.
 */
char *
mate_vfs_expand_initial_tilde (const char *path)
{
#ifndef G_OS_WIN32
	char *slash_after_user_name, *user_name;
	struct passwd *passwd_file_entry;

	g_return_val_if_fail (path != NULL, NULL);

	if (path[0] != '~') {
		return g_strdup (path);
	}
	
	if (path[1] == '/' || path[1] == '\0') {
		return g_strconcat (g_get_home_dir (), &path[1], NULL);
	}

	slash_after_user_name = strchr (&path[1], '/');
	if (slash_after_user_name == NULL) {
		user_name = g_strdup (&path[1]);
	} else {
		user_name = g_strndup (&path[1],
				       slash_after_user_name - &path[1]);
	}
	passwd_file_entry = getpwnam (user_name);
	g_free (user_name);

	if (passwd_file_entry == NULL || passwd_file_entry->pw_dir == NULL) {
		return g_strdup (path);
	}

	return g_strconcat (passwd_file_entry->pw_dir,
			    slash_after_user_name,
			    NULL);
#else
	return g_strdup (path);
#endif
}

static int
hex_to_int (gchar c)
{
	return  c >= '0' && c <= '9' ? c - '0'
		: c >= 'A' && c <= 'F' ? c - 'A' + 10
		: c >= 'a' && c <= 'f' ? c - 'a' + 10
		: -1;
}

static int
unescape_character (const char *scanner)
{
	int first_digit;
	int second_digit;

	first_digit = hex_to_int (*scanner++);
	if (first_digit < 0) {
		return -1;
	}

	second_digit = hex_to_int (*scanner++);
	if (second_digit < 0) {
		return -1;
	}

	return (first_digit << 4) | second_digit;
}

/**
 * mate_vfs_unescape_string:
 * @escaped_string: an escaped uri, path, or other string.
 * @illegal_characters: a string containing a sequence of characters
 * considered "illegal" to be escaped, '\0' is automatically in this list.
 *
 * Decodes escaped characters (i.e. PERCENTxx sequences) in @escaped_string.
 * Characters are encoded in PERCENTxy form, where xy is the ASCII hex code 
 * for character 16x+y.
 * 
 * Return value: a newly allocated string with the unescaped
 * equivalents, or %NULL if @escaped_string contained an escaped
 * encoding of one of the characters in @illegal_characters.
 */
char *
mate_vfs_unescape_string (const gchar *escaped_string, 
			   const gchar *illegal_characters)
{
	const gchar *in;
	gchar *out, *result;
	gint character;
	
	if (escaped_string == NULL) {
		return NULL;
	}

	result = g_malloc (strlen (escaped_string) + 1);
	
	out = result;
	for (in = escaped_string; *in != '\0'; in++) {
		character = *in;
		if (*in == HEX_ESCAPE) {
			character = unescape_character (in + 1);

			/* Check for an illegal character. We consider '\0' illegal here. */
			if (character <= 0
			    || (illegal_characters != NULL
				&& strchr (illegal_characters, (char)character) != NULL)) {
				g_free (result);
				return NULL;
			}
			in += 2;
		}
		*out++ = (char)character;
	}
	
	*out = '\0';
	g_assert (out - result <= strlen (escaped_string));
	return result;
	
}

/**
 * mate_vfs_unescape_string_for_display:
 * @escaped: a string encoded with escaped sequences.
 * 
 * Similar to mate_vfs_unescape_string, but it returns something
 * semi-intelligable to a user even upon receiving traumatic input
 * such as %00 or URIs in bad form.
 * 
 * See also: mate_vfs_unescape_string().
 * 
 * WARNING: You should never use this function on a whole URI!  It
 * unescapes reserved characters, and can result in a mangled URI
 * that can not be re-entered.  For example, it unescapes "#" "&" and "?",
 * which have special meanings in URI strings.

 * Return value: a pointer to a g_malloc'd string with all characters
 * replacing their escaped hex values.
 */
gchar *
mate_vfs_unescape_string_for_display (const gchar *escaped)
{
	const gchar *in, *start_escape;
	gchar *out, *result;
	gint i,j;
	gchar c;
	gint invalid_escape;

	if (escaped == NULL) {
		return NULL;
	}

	result = g_malloc (strlen (escaped) + 1);
	
	out = result;
	for (in = escaped; *in != '\0'; ) {
		start_escape = in;
		c = *in++;
		invalid_escape = 0;
		
		if (c == HEX_ESCAPE) {
			/* Get the first hex digit. */
			i = hex_to_int (*in++);
			if (i < 0) {
				invalid_escape = 1;
				in--;
			}
			c = i << 4;
			
			if (invalid_escape == 0) {
				/* Get the second hex digit. */
				i = hex_to_int (*in++);
				if (i < 0) {
					invalid_escape = 2;
					in--;
				}
				c |= i;
			}
			if (invalid_escape == 0) {
				/* Check for an illegal character. */
				if (c == '\0') {
					invalid_escape = 3;
				}
			}
		}
		if (invalid_escape != 0) {
			for (j = 0; j < invalid_escape; j++) {
				*out++ = *start_escape++;
			}
		} else {
			*out++ = c;
		}
	}
	
	*out = '\0';
	g_assert (out - result <= strlen (escaped));
	return result;
}

/**
 * mate_vfs_remove_optional_escapes:
 * @uri: an escaped uri.
 * 
 * Scans the @uri and converts characters that do not have to be 
 * escaped into an un-escaped form. The characters that get treated this
 * way are defined as unreserved by the RFC.
 * 
 * Return value: an error value if the @uri is found to be malformed.
 */
MateVFSResult
mate_vfs_remove_optional_escapes (char *uri)
{
	guchar *scanner;
	int character;
	int length;

	if (uri == NULL) {
		return MATE_VFS_OK;
	}
	
	length = strlen (uri);

	for (scanner = (guchar *)uri; *scanner != '\0'; scanner++, length--) {
		if (*scanner == HEX_ESCAPE) {
			character = unescape_character ((char *)scanner + 1);
			if (character < 0) {
				/* invalid hexadecimal character */
				return MATE_VFS_ERROR_INVALID_URI;
			}

			if (uri_character_kind [character] == UNRESERVED) {
				/* This character does not need to be escaped, convert it
				 * to a non-escaped form.
				 */
				*scanner = (guchar)character;
				g_assert (length >= 3);

				/* Shrink the string covering up the two extra digits of the
				 * escaped character. Include the trailing '\0' in the copy
				 * to keep the string terminated.
				 */
				memmove (scanner + 1, scanner + 3, length - 2);
			} else {
				/* This character must stay escaped, skip the entire
				 * escaped sequence
				 */
				scanner += 2;
			}
			length -= 2;

		} else if (*scanner > 127
			|| uri_character_kind [*scanner] == DELIMITERS
			|| uri_character_kind [*scanner] == UNWISE
			|| uri_character_kind [*scanner] == CONTROL) {
			/* It is illegal for this character to be in an un-escaped form
			 * in the uri.
			 */
			return MATE_VFS_ERROR_INVALID_URI;
		}
	}
	return MATE_VFS_OK;
}

static char *
mate_vfs_make_uri_canonical_old (const char *original_uri_text)
{
	MateVFSURI *uri;
	char *result;

	uri = mate_vfs_uri_new_private (original_uri_text, TRUE, TRUE, FALSE);
	if (uri == NULL) {
		return NULL;
	} 

	result = mate_vfs_uri_to_string (uri, MATE_VFS_URI_HIDE_NONE);
	mate_vfs_uri_unref (uri);

	return result;
}

/**
 * mate_vfs_make_path_name_canonical:
 * @path: a file path, relative or absolute.
 * 
 * Calls _mate_vfs_canonicalize_pathname(), allocating storage for the 
 * result and providing for a cleaner memory management.
 * 
 * Return value: a canonical version of @path.
 */
gchar *
mate_vfs_make_path_name_canonical (const gchar *path)
{
	char *path_clone;
	char *result;

	path_clone = g_strdup (path);
	result = _mate_vfs_canonicalize_pathname (path_clone);
	if (result != path_clone) {
		g_free (path_clone);
		return g_strdup (result);
	}

	return path_clone;
}

/**
 * mate_vfs_list_deep_free:
 * @list: list to be freed.
 *
 * Free @list and call g_free() on all data members.
 */
void
mate_vfs_list_deep_free (GList *list)
{
	GList *p;

	if (list == NULL)
		return;

	for (p = list; p != NULL; p = p->next) {
		g_free (p->data);
	}
	g_list_free (list);
}

/**
 * mate_vfs_get_local_path_from_uri:
 * @uri: uri to convert to a local path.
 * 
 * Create a local path for a file:/// uri. Do not use with uris
 * of other methods.
 *
 * Return value: a newly allocated string containing the local path.
 * %NULL is returned on error or if the uri isn't a file: uri
 * without a fragment identifier (or chained uri).
 */
char *
mate_vfs_get_local_path_from_uri (const char *uri)
{
	const char *path_part;

	if (!_mate_vfs_istr_has_prefix (uri, "file:/")) {
		return NULL;
	}
	
	path_part = uri + strlen ("file:");
	if (strchr (path_part, '#') != NULL) {
		return NULL;
	}
	
	if (_mate_vfs_istr_has_prefix (path_part, "///")) {
		path_part += 2;
	} else if (_mate_vfs_istr_has_prefix (path_part, "//")) {
		return NULL;
	}

	return mate_vfs_unescape_string (path_part, "/");
}

/**
 * mate_vfs_get_uri_from_local_path:
 * @local_full_path: a full local filesystem path (i.e. not relative).
 * 
 * Returns a file:/// URI for the local path @local_full_path, 
 * such as a path provided by gtk_file_chooser_get_filename().
 * The resulting URI may be provided, for instance, to mate_vfs_uri_new().
 *
 * On Windows @local_full_path should be in the UTF-8 encoding, and can start with a drive
 * letter, but doesn't have to.
 *
 * Return value: a newly allocated string containing the uri corresponding 
 * to @local_full_path (%NULL for some bad errors).
 */
char *
mate_vfs_get_uri_from_local_path (const char *local_full_path)
{
	char *escaped_path, *result;
	
	if (local_full_path == NULL) {
		return NULL;
	}

	g_return_val_if_fail (g_path_is_absolute (local_full_path), NULL);

	escaped_path = mate_vfs_escape_path_string (local_full_path);

	result = g_strconcat ("file://", escaped_path, NULL);
	g_free (escaped_path);
	return result;
}

/**
 * mate_vfs_get_volume_free_space:
 * @vfs_uri: a #MateVFSURI.
 * @size: a #MateVFSFileSize.
 * 
 * Stores the amount of free space in bytes on @vfs-uri's
 * volume in @size.
 *
 * Returns: %MATE_VFS_OK on success, otherwise an
 * %MATE_VFS_ERROR_* code.
 */
MateVFSResult
mate_vfs_get_volume_free_space (const MateVFSURI *vfs_uri, 
				 MateVFSFileSize  *size)
{	
	if (!VFS_METHOD_HAS_FUNC(vfs_uri->method, get_volume_free_space))
		return MATE_VFS_ERROR_NOT_SUPPORTED;

	return vfs_uri->method->get_volume_free_space (vfs_uri->method, vfs_uri, size);
}

/**
 * mate_vfs_icon_path_from_filename:
 * @filename: path to a file. Could be relative or absolute path.
 *
 * Return value: Returns the icon path for the @filename. Example:
 * mate_vfs_icon_path_from_filename ("caja/caja-desktop.png") will return a string 
 * forming the full path of the file caja-desktop.png ie $PREFIX/share/pixmaps/caja/caja-desktop.png.
 */
char *
mate_vfs_icon_path_from_filename (const char *filename)
{
	const char *mate_var;
	char *full_filename;
	char **paths, **temp_paths;

	if (g_path_is_absolute (filename) &&
	    g_file_test (filename, G_FILE_TEST_EXISTS))
		return g_strdup (filename);

	mate_var = g_getenv ("MATE_PATH");

	if (mate_var == NULL) {
		mate_var = MATE_VFS_PREFIX;
	}

	paths = g_strsplit (mate_var, G_SEARCHPATH_SEPARATOR_S, 0); 

	for (temp_paths = paths; *temp_paths != NULL; temp_paths++) {
		full_filename = g_build_filename (*temp_paths,
						  "share",
						  "pixmaps",
						  filename,
						  NULL);
		if (g_file_test (full_filename, G_FILE_TEST_EXISTS)) {
			g_strfreev (paths);
			return full_filename;
		}
		g_free (full_filename);
		full_filename = NULL;
	}

	g_strfreev (paths);
	return NULL;
}

static char *
strdup_to (const char *string, const char *end)
{
	if (end == NULL) {
		return g_strdup (string);
	}
	return g_strndup (string, end - string);
}

static gboolean
is_executable_file (const char *path)
{
	/* Check that it exists and is regular. */
	if (!g_file_test (path, G_FILE_TEST_IS_REGULAR)) {
		return FALSE;
	}

	/* Check that it's executable. */
	if (!g_file_test (path, G_FILE_TEST_IS_EXECUTABLE)) {
		return FALSE;
	}

	return TRUE;
}

static gboolean
executable_in_path (const char *executable_name)
{
	const char *path_list, *piece_start, *piece_end;
	char *piece, *raw_path, *expanded_path;
	gboolean is_good;

	path_list = g_getenv ("PATH");

	for (piece_start = path_list; ; piece_start = piece_end + 1) {
		/* Find the next piece of PATH. */
		piece_end = strchr (piece_start, G_SEARCHPATH_SEPARATOR);
		piece = strdup_to (piece_start, piece_end);
		g_strstrip (piece);
		
		if (piece[0] == '\0') {
			is_good = FALSE;
		} else {
			/* Try out this path with the executable. */
			raw_path = g_build_filename (piece, executable_name, NULL);
			/* XXX Can this be necessary? Surely PATH
			 * isn't supposed to contain actual tildes,
			 * but the shell expands any tilde one tries
			 * to add to PATH?
			 */ 
			expanded_path = mate_vfs_expand_initial_tilde (raw_path);
			g_free (raw_path);
			
			is_good = is_executable_file (expanded_path);
			g_free (expanded_path);
		}
		
		g_free (piece);
		
		if (is_good) {
			return TRUE;
		}

		if (piece_end == NULL) {
			return FALSE;
		}
	}
}

static char *
get_executable_name_from_command_string (const char *command_string)
{
	/* FIXME bugzilla.eazel.com 2757: 
	 * We need to handle quoting here for the full-path case */
	return g_strstrip (strdup_to (command_string, strchr (command_string, ' ')));
}

/**
 * mate_vfs_is_executable_command_string:
 * @command_string: a string representing a command ie "xterm -bg white".
 * 
 * Checks if @command_string starts with the full path of an executable file
 * or an executable in $PATH.
 *
 * Returns: %TRUE if @command_string started with an executable file, and is in $PATH,
 * %FALSE otherwise.
 */
gboolean
mate_vfs_is_executable_command_string (const char *command_string)
{
	char *executable_name;
	char *executable_path;
	gboolean found;

	/* Check whether command_string is a full path for an executable. */
	if (g_path_is_absolute (command_string)) {

		/* FIXME bugzilla.eazel.com 2757:
		 * Because we don't handle quoting, we can check for full
		 * path including spaces, but no parameters, and full path
		 * with no spaces with or without parameters. But this will
		 * fail for quoted full path with spaces, and parameters.
		 */

		/* This works if command_string contains a space, but not
		 * if command_string has parameters.
		 */
		if (is_executable_file (command_string)) {
			return TRUE;
		}

		/* This works if full path has no spaces, with or without parameters */
		executable_path = get_executable_name_from_command_string (command_string);
		found = is_executable_file (executable_path);
		g_free (executable_path);

		return found;
	}
	
	executable_name = get_executable_name_from_command_string (command_string);
	found = executable_in_path (executable_name);
	g_free (executable_name);

	return found;
}

/**
 * mate_vfs_read_entire_file:
 * @uri: uri of the file to read.
 * @file_size: after reading the file, contains the size of the file read.
 * @file_contents: contains the file_size bytes, the contents of the file at @uri.
 * 
 * Reads an entire file into memory for convenience. Beware accidentally
 * loading large files into memory with this function.
 *
 * Since version 2.10 the string stored in @file_contents will be null-terminated,
 * so for text files you can use result as a normal string.
 *
 * Return value: an integer representing the result of the operation.
 *
 * Since: 2.2
 */
MateVFSResult
mate_vfs_read_entire_file (const char *uri,
			    int *file_size,
			    char **file_contents)
{
	MateVFSResult result;
	MateVFSHandle *handle;
	char *buffer;
	MateVFSFileSize total_bytes_read;
	MateVFSFileSize bytes_read;

	g_assert (file_contents != NULL);

	if (file_size != NULL)
		*file_size = 0;
	*file_contents = NULL;

	/* Open the file. */
	result = mate_vfs_open (&handle, uri, MATE_VFS_OPEN_READ);
	if (result != MATE_VFS_OK) {
		return result;
	}

	/* Read the whole thing. */
	buffer = NULL;
	total_bytes_read = 0;
	do {
		buffer = g_realloc (buffer, total_bytes_read + READ_CHUNK_SIZE);
		result = mate_vfs_read (handle,
					 buffer + total_bytes_read,
					 READ_CHUNK_SIZE,
					 &bytes_read);
		if (result != MATE_VFS_OK && result != MATE_VFS_ERROR_EOF) {
			g_free (buffer);
			mate_vfs_close (handle);
			return result;
		}

		/* Check for overflow. */
		if (total_bytes_read + bytes_read < total_bytes_read) {
			g_free (buffer);
			mate_vfs_close (handle);
			return MATE_VFS_ERROR_TOO_BIG;
		}

		total_bytes_read += bytes_read;
	} while (result == MATE_VFS_OK);

	buffer = g_realloc (buffer, total_bytes_read + 1);
	buffer[total_bytes_read] = '\0';

	/* Close the file. */
	result = mate_vfs_close (handle);
	if (result != MATE_VFS_OK) {
		g_free (buffer);
		return result;
	}

	/* Return the file. */
	if (file_size != NULL)
		*file_size = total_bytes_read;
	*file_contents = g_realloc (buffer, total_bytes_read + 1);
	return MATE_VFS_OK;
}

static char *
mate_vfs_make_valid_utf8 (const char *name)
{
	GString *string;
	const char *remainder, *invalid;
	int remaining_bytes, valid_bytes;

	string = NULL;
	remainder = name;
	remaining_bytes = strlen (name);

	while (remaining_bytes != 0) {
		if (g_utf8_validate (remainder, remaining_bytes, &invalid)) {
			break;
		}
		valid_bytes = invalid - remainder;

		if (string == NULL) {
			string = g_string_sized_new (remaining_bytes);
		}
		g_string_append_len (string, remainder, valid_bytes);
		g_string_append_c (string, '?');

		remaining_bytes -= valid_bytes + 1;
		remainder = invalid + 1;
	}

	if (string == NULL) {
		return g_strdup (name);
	}

	g_string_append (string, remainder);
	g_string_append (string, _(" (invalid Unicode)"));
	g_assert (g_utf8_validate (string->str, -1, NULL));

	return g_string_free (string, FALSE);
}

static char *
mate_vfs_format_uri_for_display_internal (const char *uri,
					   gboolean filenames_are_utf8)
{
	char *canonical_uri, *path, *utf8_path;

	g_return_val_if_fail (uri != NULL, g_strdup (""));

	canonical_uri = mate_vfs_make_uri_canonical_old (uri);

	/* If there's no fragment and it's a local path. */
	path = mate_vfs_get_local_path_from_uri (canonical_uri);
	
	if (path != NULL) {
		if (!filenames_are_utf8) {
			utf8_path = g_filename_to_utf8(path, -1, NULL, NULL, NULL);
			if (utf8_path) {
				g_free (canonical_uri);
				g_free (path);
				return utf8_path;
			} 
		} else if (g_utf8_validate (path, -1, NULL)) {
			g_free (canonical_uri);
			return path;
		}
	}

	if (canonical_uri && !g_utf8_validate (canonical_uri, -1, NULL)) {
		utf8_path = mate_vfs_make_valid_utf8 (canonical_uri);
		g_free (canonical_uri);
		canonical_uri = utf8_path;
	}

	g_free (path);
	return canonical_uri;
}


/**
 * mate_vfs_format_uri_for_display:
 * @uri: uri to be displayed.
 *
 * Filter, modify, unescape and change @uri to make it appropriate
 * for display to users. The conversion is done such that the roundtrip
 * to UTF-8 is reversible.
 * 
 * Rules:
 * 	file: uri without fragments should appear as local paths.
 * 	file: uri with fragments should appear as file:uri.
 * 	All other uri appear as expected.
 *
 * Return value: a newly allocated string which represents @uri and can be displayed.
 *
 * Since: 2.2
 */

char *
mate_vfs_format_uri_for_display (const char *uri) 
{
	gboolean utf8;
	const char **charsets;

	utf8 = g_get_filename_charsets (&charsets);

	return mate_vfs_format_uri_for_display_internal (uri, utf8);
}

static gboolean
is_valid_scheme_character (char c)
{
	return g_ascii_isalnum (c) || c == '+' || c == '-' || c == '.';
}

static gboolean
has_valid_scheme (const char *uri)
{
	const char *p;

	p = uri;

	if (!is_valid_scheme_character (*p)) {
		return FALSE;
	}

	do {
		p++;
	} while (is_valid_scheme_character (*p));

	return *p == ':';
}

static char *
mate_vfs_escape_high_chars (const guchar *string)
{
	char *result;
	const guchar *scanner;
	guchar *result_scanner;
	int escape_count;
	static const gchar hex[16] = "0123456789ABCDEF";

#define ACCEPTABLE(a) ((a)>=32 && (a)<128)
	
	escape_count = 0;

	if (string == NULL) {
		return NULL;
	}

	for (scanner = string; *scanner != '\0'; scanner++) {
		if (!ACCEPTABLE(*scanner)) {
			escape_count++;
		}
	}
	
	if (escape_count == 0) {
		return g_strdup ((char *)string);
	}

	/* allocate two extra characters for every character that
	 * needs escaping and space for a trailing zero
	 */
	result = g_malloc (scanner - string + escape_count * 2 + 1);
	for (scanner = string, result_scanner = (guchar *)result; *scanner != '\0'; scanner++) {
		if (!ACCEPTABLE(*scanner)) {
			*result_scanner++ = '%';
			*result_scanner++ = hex[*scanner >> 4];
			*result_scanner++ = hex[*scanner & 15];
			
		} else {
			*result_scanner++ = *scanner;
		}
	}

	*result_scanner = '\0';

	return result;
}

/* http uris look like <something>.<2-4 letters>, possibly followed by a slash and some text. */
static gboolean
looks_like_http_uri (const char *str)
{
	int len;
	int i;
	char c;
	const char *first_slash;

	first_slash = strchr(str, '/');
	if (first_slash == NULL) {
		len = strlen (str);
	} else {
		len = first_slash - str;
	}
	for (i = 0; i < 5 && i < len; i++) {
		c = str[len - 1 - i];
		if (i >= 2 && c == '.') {
			return TRUE;
		}
		if (!g_ascii_isalpha (c)) {
			return FALSE;
		}
	}
	return FALSE;
}

/* The strip_trailing_whitespace option is intended to make copy/paste of
 * URIs less error-prone when it is known that trailing whitespace isn't
 * part of the uri.
 */
static char *
mate_vfs_make_uri_from_input_internal (const char *text,
					gboolean filenames_are_utf8,
					gboolean strip_trailing_whitespace)
{
	char *stripped, *uri, *locale_path, *escaped;

	g_return_val_if_fail (text != NULL, g_strdup (""));

	/* Strip off leading whitespaces (since they can't be part of a valid
	   uri).   Only strip off trailing whitespaces when requested since
	   they might be part of a valid uri.
	 */
	if (strip_trailing_whitespace) {
		stripped = g_strstrip (g_strdup (text));
	} else {
		stripped = g_strchug (g_strdup (text));
	}

	if (g_path_is_absolute (stripped)) {
		if (!filenames_are_utf8) {
			locale_path = g_filename_to_utf8 (stripped, -1, NULL, NULL, NULL);
			if (locale_path != NULL) {
				uri = mate_vfs_get_uri_from_local_path (locale_path);
				g_free (locale_path);
			} else {
				/* We couldn't convert to the locale. */
				/* FIXME: We should probably give a user-visible error here. */
				uri = g_strdup("");
			}
		} else {
			uri = mate_vfs_get_uri_from_local_path (stripped);
		}
	} else switch (stripped[0]) {
	case '\0':
		uri = g_strdup ("");
		break;
#ifndef G_OS_WIN32
	case '~': {
		char *path, *filesystem_path;
		if (!filenames_are_utf8) {
			filesystem_path = g_filename_to_utf8(stripped, -1, NULL, NULL, NULL);
		} else {
			filesystem_path = g_strdup (stripped);
		}
                /* deliberately falling into default case on fail */
		if (filesystem_path != NULL) {
			path = mate_vfs_expand_initial_tilde (filesystem_path);
			g_free (filesystem_path);
			if (*path == '/') {
				uri = mate_vfs_get_uri_from_local_path (path);
				g_free (path);
				break;
			}
			g_free (path);
		}
                /* don't insert break here, read above comment */
	}
#endif
	default:
		if (has_valid_scheme (stripped)) {
			uri = mate_vfs_escape_high_chars ((guchar *)stripped);
		} else if (looks_like_http_uri (stripped)) {
			escaped = mate_vfs_escape_high_chars ((guchar *)stripped);
			uri = g_strconcat ("http://", escaped, NULL);
			g_free (escaped);
		} else {
			escaped = mate_vfs_escape_high_chars ((guchar *)stripped);
			uri = g_strconcat ("file:///", escaped, NULL);
			g_free (escaped);
		}
	}

	g_free (stripped);

	return uri;
	
}

/**
 * mate_vfs_make_uri_from_input:
 * @location: a possibly mangled "uri", in UTF-8.
 *
 * Takes a user input path/uri and makes a valid uri out of it.
 *
 * This function is the reverse of mate_vfs_format_uri_for_display()
 * but it also handles the fact that the user could have typed
 * arbitrary UTF-8 in the entry showing the string.
 *
 * Returns: a newly allocated uri.
 *
 * Since: 2.2
 */
char *
mate_vfs_make_uri_from_input (const char *location)
{
	gboolean utf8;
	const char **charsets;

	utf8 = g_get_filename_charsets (&charsets);

	return mate_vfs_make_uri_from_input_internal (location, utf8, TRUE);
}

/**
 * mate_vfs_make_uri_from_input_with_trailing_ws:
 * @location: a possibly mangled uri, in UTF-8.
 *
 * Takes a user input path/uri and makes a valid uri out of it.
 *
 * This function is indentical to mate_vfs_make_uri_from_input() except
 * that this version won't strip any trailing slashes.
 *
 * Returns: a newly allocated uri.
 *
 * Since: 2.12
 */
char *
mate_vfs_make_uri_from_input_with_trailing_ws (const char *location)
{
	gboolean utf8;
	const char **charsets;

	utf8 = g_get_filename_charsets (&charsets);

	return mate_vfs_make_uri_from_input_internal (location, utf8, FALSE);	
}

/**
 * mate_vfs_make_uri_from_input_with_dirs:
 * @location: a relative or absolute path.
 * @dirs: directory to use as a base directory if @location is a relative path.
 *
 * Determines a fully qualified uri from a relative or absolute input path.
 * Basically calls mate_vfs_make_uri_from_input() except it specifically
 * tries to support paths relative to the specified directories (can be homedir
 * and/or current directory). See #MateVFSMakeURIDirs for more information.
 *
 * Return value: a newly allocated string containing the fully qualified uri.
 *
 * Since: 2.4
 */
char *
mate_vfs_make_uri_from_input_with_dirs (const char *location,
					 MateVFSMakeURIDirs dirs)
{
	char *uri, *path, *dir;

	if (g_path_is_absolute (location))
		uri = mate_vfs_make_uri_from_input (location);
	else switch (location[0]) {
	case '\0':
		uri = g_strdup ("");
		break;
#ifndef G_OS_WIN32		
	case '~':
		uri = mate_vfs_make_uri_from_input (location);
		break;
#endif
	default:
		/* this might be a relative path, check if it exists relative
		 * to current dir and home dir.
		 */
		uri = NULL;
		if (dirs & MATE_VFS_MAKE_URI_DIR_CURRENT) {
			dir = g_get_current_dir ();
			path = g_build_filename (dir, location, NULL);
			g_free (dir);
			
			if (g_file_test (path, G_FILE_TEST_EXISTS)) {
				uri = mate_vfs_make_uri_from_input (path);
			}
			g_free (path);
		}

		if (uri == NULL &&
		    dirs & MATE_VFS_MAKE_URI_DIR_HOMEDIR) {
			path = g_build_filename (g_get_home_dir (), location, NULL);
		
			if (g_file_test (path, G_FILE_TEST_EXISTS)) {
				uri = mate_vfs_make_uri_from_input (path);
			}
			g_free (path);
		}

		if (uri == NULL) {
			uri = mate_vfs_make_uri_from_input (location);
		}
	}
	
	return uri;
}


/**
 * mate_vfs_make_uri_canonical_strip_fragment:
 * @uri: a #MateVFSURI.
 *
 * Returns a canonicalized uri. If @uri contains a fragment (anything after a '#') strips off that and
 * then makes the @uri canonical.
 *
 * Returns: a newly allocated string containing a canonical @uri.
 *
 * Since: 2.2
 */

char *
mate_vfs_make_uri_canonical_strip_fragment (const char *uri)
{
	const char *fragment;
	char *without_fragment, *canonical;

	fragment = strchr (uri, '#');
	if (fragment == NULL) {
		return mate_vfs_make_uri_canonical (uri);
	}

	without_fragment = g_strndup (uri, fragment - uri);
	canonical = mate_vfs_make_uri_canonical (without_fragment);
	g_free (without_fragment);
	return canonical;
}

static gboolean
uris_match (const char *uri_1, const char *uri_2, gboolean ignore_fragments)
{
	char *canonical_1, *canonical_2;
	gboolean result;

	if (ignore_fragments) {
		canonical_1 = mate_vfs_make_uri_canonical_strip_fragment (uri_1);
		canonical_2 = mate_vfs_make_uri_canonical_strip_fragment (uri_2);
	} else {
		canonical_1 = mate_vfs_make_uri_canonical (uri_1);
		canonical_2 = mate_vfs_make_uri_canonical (uri_2);
	}

	result = strcmp (canonical_1, canonical_2) ? FALSE : TRUE;

	g_free (canonical_1);
	g_free (canonical_2);
	
	return result;
}

/**
 * mate_vfs_uris_match:
 * @uri_1: stringified uri to compare with @uri_2.
 * @uri_2: stringified uri to compare with @uri_1.
 * 
 * Compare two uris.
 *
 * Return value: %TRUE if they are the same, %FALSE otherwise.
 *
 * Since: 2.2
 */

gboolean
mate_vfs_uris_match (const char *uri_1, const char *uri_2)
{
	return uris_match (uri_1, uri_2, FALSE);
}

static gboolean
mate_vfs_uri_is_local_scheme (const char *uri)
{
	gboolean is_local_scheme;
	char *temp_scheme;
	int i;
	char *local_schemes[] = {"file:", "help:", "ghelp:", "mate-help:",
				 "trash:", "man:", "info:", 
				 "hardware:", "search:", "pipe:",
				 "mate-trash:", NULL};

	is_local_scheme = FALSE;
	for (temp_scheme = *local_schemes, i = 0; temp_scheme != NULL; i++, temp_scheme = local_schemes[i]) {
		is_local_scheme = _mate_vfs_istr_has_prefix (uri, temp_scheme);
		if (is_local_scheme) {
			break;
		}
	}
	

	return is_local_scheme;
}

static char *
mate_vfs_handle_trailing_slashes (const char *uri)
{
	char *temp, *uri_copy;
	gboolean previous_char_is_column, previous_chars_are_slashes_without_column;
	gboolean previous_chars_are_slashes_with_column;
	gboolean is_local_scheme;

	g_assert (uri != NULL);

	uri_copy = g_strdup (uri);
	if (strlen (uri_copy) <= 2) {
		return uri_copy;
	}

	is_local_scheme = mate_vfs_uri_is_local_scheme (uri);

	previous_char_is_column = FALSE;
	previous_chars_are_slashes_without_column = FALSE;
	previous_chars_are_slashes_with_column = FALSE;

	/* remove multiple trailing slashes */
	for (temp = uri_copy; *temp != '\0'; temp++) {
		if (*temp == '/' && !previous_char_is_column) {
			previous_chars_are_slashes_without_column = TRUE;
		} else if (*temp == '/' && previous_char_is_column) {
			previous_chars_are_slashes_without_column = FALSE;
			previous_char_is_column = TRUE;
			previous_chars_are_slashes_with_column = TRUE;
		} else {
			previous_chars_are_slashes_without_column = FALSE;
			previous_char_is_column = FALSE;
			previous_chars_are_slashes_with_column = FALSE;
		}

		if (*temp == ':') {
			previous_char_is_column = TRUE;
		}
	}

	if (*temp == '\0' && previous_chars_are_slashes_without_column) {
		if (is_local_scheme) {
			/* go back till you remove them all. */
			for (temp--; *(temp) == '/'; temp--) {
				*temp = '\0';
			}
		} else {
			/* go back till you remove them all but one. */
			for (temp--; *(temp - 1) == '/'; temp--) {
				*temp = '\0';
			}			
		}
	}

	if (*temp == '\0' && previous_chars_are_slashes_with_column) {
		/* go back till you remove them all but three. */
		for (temp--; *(temp - 3) != ':' && *(temp - 2) != ':' && *(temp - 1) != ':'; temp--) {
			*temp = '\0';
		}
	}


	return uri_copy;
}

/**
 * mate_vfs_make_uri_canonical:
 * @uri: an absolute or relative stringified uri. It might have scheme.
 *
 * Standardizes the format of the @uri, so that it can be used
 * later in other functions that expect a canonical uri.
 *
 * Returns: a newly allocated string that contains the canonical 
 * representation of @uri.
 *
 * Since: 2.2
 */

char *
mate_vfs_make_uri_canonical (const char *uri)
{
	char *canonical_uri, *old_uri, *p;
	gboolean relative_uri;

	relative_uri = FALSE;

	if (uri == NULL) {
		return NULL;
	}

	/* FIXME bugzilla.eazel.com 648: 
	 * This currently ignores the issue of two uris that are not identical but point
	 * to the same data except for the specific cases of trailing '/' characters,
	 * file:/ and file:///, and "lack of file:".
	 */

	canonical_uri = mate_vfs_handle_trailing_slashes (uri);

	/* Note: In some cases, a trailing slash means nothing, and can
	 * be considered equivalent to no trailing slash. But this is
	 * not true in every case; specifically not for web addresses passed
	 * to a web-browser. So we don't have the trailing-slash-equivalence
	 * logic here, but we do use that logic in EelDirectory where
	 * the rules are more strict.
	 */

#ifndef G_OS_WIN32
#define URI_CONTAINS_NO_SCHEME(uri) (strchr (uri, ':') == NULL)
#else
#define URI_CONTAINS_NO_SCHEME(uri) \
  (strlen (uri) > 2 && strchr (uri + 2, ':') == NULL)
#endif


	/* Add file: if there is no scheme. */
	if (URI_CONTAINS_NO_SCHEME (canonical_uri)) {
		old_uri = canonical_uri;

		if (!g_path_is_absolute (old_uri)) {
			/* FIXME bugzilla.eazel.com 5069: 
			 *  bandaid alert. Is this really the right thing to do?
			 * 
			 * We got what really is a relative path. We do a little bit of
			 * a stretch here and assume it was meant to be a cryptic absolute path,
			 * and convert it to one. Since we can't call mate_vfs_uri_new and
			 * mate_vfs_uri_to_string to do the right make-canonical conversion,
			 * we have to do it ourselves.
			 */
			relative_uri = TRUE;
			canonical_uri = mate_vfs_make_path_name_canonical (old_uri);
			g_free (old_uri);
			old_uri = canonical_uri;
			canonical_uri = g_strconcat ("file:///", old_uri, NULL);
		} else {
			canonical_uri = g_strconcat ("file:", old_uri, NULL);
		}
		g_free (old_uri);
	}

	/* Lower-case the scheme. */
	for (p = canonical_uri; *p != ':'; p++) {
		g_assert (*p != '\0');
		*p = g_ascii_tolower (*p);
	}

	if (!relative_uri) {
		old_uri = canonical_uri;
		canonical_uri = mate_vfs_make_uri_canonical_old (canonical_uri);
		if (canonical_uri != NULL) {
			g_free (old_uri);
		} else {
			canonical_uri = old_uri;
		}
	}
	
	/* FIXME bugzilla.eazel.com 2802:
	 * Work around mate-vfs's desire to convert file:foo into file://foo
	 * by converting to file:///foo here. When you remove this, check that
	 * typing "foo" into location bar does not crash and returns an error
	 * rather than displaying the contents of /
	 */
	if (canonical_uri != NULL
	    && g_str_has_prefix (canonical_uri, "file://")
	    && !g_str_has_prefix (canonical_uri, "file:///")) {
		old_uri = canonical_uri;
		canonical_uri = g_strconcat ("file:/", old_uri + 5, NULL);
		g_free (old_uri);
	}

	return canonical_uri;
}

/**
 * mate_vfs_get_uri_scheme:
 * @uri: a stringified uri.
 *
 * Retrieve the scheme used in @uri.
 *
 * Return value: a newly allocated string containing the scheme, %NULL
 * if @uri doesn't contain a scheme.
 *
 * Since: 2.2
 */

char *
mate_vfs_get_uri_scheme (const char *uri)
{
	char *colon;

	g_return_val_if_fail (uri != NULL, NULL);

	colon = strchr (uri, ':');
	
	if (colon == NULL) {
		return NULL;
	}

	return g_ascii_strdown (uri, colon - uri);
}

/* Note that NULL's and full paths are also handled by this function.
 * A NULL location will return the current working directory
 */
static char *
file_uri_from_local_relative_path (const char *location)
{
	char *current_dir;
	char *base_uri, *base_uri_slash;
	char *location_escaped;
	char *uri;

	current_dir = g_get_current_dir ();
	base_uri = mate_vfs_get_uri_from_local_path (current_dir);
	/* g_get_current_dir returns w/o trailing / */
	base_uri_slash = g_strconcat (base_uri, "/", NULL);

	location_escaped = mate_vfs_escape_path_string (location);

	uri = mate_vfs_uri_make_full_from_relative (base_uri_slash, location_escaped);

	g_free (location_escaped);
	g_free (base_uri_slash);
	g_free (base_uri);
	g_free (current_dir);

	return uri;
}

/**
 * mate_vfs_make_uri_from_shell_arg:
 * @uri: path to make the uri from.
 *
 * Similar to mate_vfs_make_uri_from_input(), except that:
 * 
 * 1) guesses relative paths instead of http domains.
 * 2) doesn't bother stripping leading/trailing white space.
 * 3) doesn't bother with ~ expansion--that's done by the shell.
 *
 * Returns: a newly allocated string representing @uri.
 *
 * Since: 2.2
 */

char *
mate_vfs_make_uri_from_shell_arg (const char *location)
{
	char *uri;

	g_return_val_if_fail (location != NULL, g_strdup (""));

	if (g_path_is_absolute (location))
		uri = mate_vfs_get_uri_from_local_path (location);
	else switch (location[0]) {
	case '\0':
		uri = g_strdup ("");
		break;
	default:
		if (has_valid_scheme (location)) {
			uri = g_strdup (location);
		} else {
			uri = file_uri_from_local_relative_path (location);
		}
	}

	return uri;
}

/**
 * mate_vfs_make_uri_full_from_relative:
 * @base_uri: path to use as the base for the full uri.
 * @relative_uri: full or relative path to be appended to the @base_uri.
 *
 * Returns a string representing the full uri given a full @base_uri and a 
 * secondary uri which may be relative.
 *
 * Deprecated: This function is deprecated, please use 
 * mate_vfs_uri_make_full_from_relative() from mate-vfs-uri.h .
 *
 * Return value: a newly allocated string containing the uri.
 * (%NULL for some bad errors).
 *
 * Since: 2.2
 */

char *
mate_vfs_make_uri_full_from_relative (const char *base_uri, 
				       const char *relative_uri)
{
	return mate_vfs_uri_make_full_from_relative (base_uri, relative_uri);
}

MateVFSResult
_mate_vfs_uri_resolve_all_symlinks_uri (MateVFSURI *uri,
					 MateVFSURI **result_uri)
{
	MateVFSURI *new_uri, *resolved_uri;
	MateVFSFileInfo *info;
	MateVFSResult res;
	char *escaped_symlink;
	char *p;
	int n_followed_symlinks;

	/* Ref the original uri so we don't lose it */
	uri = mate_vfs_uri_ref (uri);

	*result_uri = NULL;

	info = mate_vfs_file_info_new ();

	p = uri->text;
	n_followed_symlinks = 0;
	while (*p != 0) {
		while (*p == MATE_VFS_URI_PATH_CHR)
			p++;
		while (*p != 0 && *p != MATE_VFS_URI_PATH_CHR)
			p++;

		new_uri = mate_vfs_uri_dup (uri);
		g_free (new_uri->text);
		new_uri->text = g_strndup (uri->text, p - uri->text);
		
		mate_vfs_file_info_clear (info);
		res = mate_vfs_get_file_info_uri (new_uri, info, MATE_VFS_FILE_INFO_DEFAULT);
		if (res != MATE_VFS_OK) {
			mate_vfs_uri_unref (new_uri);
			goto out;
		}
		if (info->type == MATE_VFS_FILE_TYPE_SYMBOLIC_LINK &&
		    info->valid_fields & MATE_VFS_FILE_INFO_FIELDS_SYMLINK_NAME) {
			n_followed_symlinks++;
			if (n_followed_symlinks > MAX_SYMLINKS_FOLLOWED) {
				res = MATE_VFS_ERROR_TOO_MANY_LINKS;
				mate_vfs_uri_unref (new_uri);
				goto out;
			}
			escaped_symlink = mate_vfs_escape_path_string (info->symlink_name);
			resolved_uri = mate_vfs_uri_resolve_symbolic_link (new_uri, escaped_symlink);
			g_assert (resolved_uri != NULL);

			if (*p != 0) {
				mate_vfs_uri_unref (new_uri);

				new_uri = mate_vfs_uri_append_string (resolved_uri, p);

				mate_vfs_uri_unref (uri);
				mate_vfs_uri_unref (resolved_uri);

				uri = mate_vfs_uri_ref (new_uri);
			} else {
				mate_vfs_uri_unref (uri);
				uri = resolved_uri;
			}

			p = uri->text;
		} 
		mate_vfs_uri_unref (new_uri);
	}

	res = MATE_VFS_OK;
	*result_uri = mate_vfs_uri_dup (uri);
 out:
	mate_vfs_file_info_unref (info);
	mate_vfs_uri_unref (uri);
	return res;
}

MateVFSResult
_mate_vfs_uri_resolve_all_symlinks (const char *text_uri,
				     char **resolved_text_uri)
{
	MateVFSURI *uri, *resolved_uri;
	MateVFSResult res;

	*resolved_text_uri = NULL;

	uri = mate_vfs_uri_new (text_uri);

	if (uri == NULL) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}
	if (uri->text == NULL) {
		mate_vfs_uri_unref (uri);
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

	res = _mate_vfs_uri_resolve_all_symlinks_uri (uri, &resolved_uri);

	if (res == MATE_VFS_OK) {
		*resolved_text_uri = mate_vfs_uri_to_string (resolved_uri, MATE_VFS_URI_HIDE_NONE);
		mate_vfs_uri_unref (resolved_uri);
	}
	mate_vfs_uri_unref (uri);
	return res;
}

char *
mate_vfs_resolve_symlink (const char *path,
			   const char *symlink)
{
	char *p, *filename;
	char **strs;
	int i, j, n;
	GString *res_path;

	g_assert (path != NULL);
	g_assert (symlink != NULL);

	p = strrchr (path, '/');
	
	if (symlink[0] == '/' || p == NULL) {
		return g_strdup (symlink);
	}

	/* either use whole path as base (if it ends in '/'),
	 * or chop its filename part */
	p = g_strndup (path, p - path);
		
	filename = g_build_filename (p, symlink, NULL);
	g_free (p);

	strs = g_strsplit (filename, "/", -1);

	g_free (filename);

	n = g_strv_length (strs);

	for (i = 0; i < n; i++) {
		if (!strcmp (strs[i], "") ||
		    !strcmp (strs[i], ".")) {
			g_free (strs[i]);
			strs[i] = NULL;
		} else if (!strcmp (strs[i], "..")) {
			g_free (strs[i]);
			strs[i] = NULL;

			for (j = i; strs[j] == NULL && j > 0; j--)
				;

			g_free (strs[j]);
			strs[j] = NULL;
		}
	}

	res_path = g_string_new (NULL);

	for (i = 0; i < n; i++)
		if (strs[i] != NULL) {
			g_string_append_c (res_path, '/');
			g_string_append (res_path, strs[i]);
			g_free (strs[i]);
		}

	/* TODO also re-append '/' if the symlink ends in '/'? */ 
	if (res_path->len == 0)
		g_string_append_c (res_path, '/');

	g_free (strs);

	return g_string_free (res_path, FALSE);
}

gboolean 
_mate_vfs_uri_is_in_subdir (MateVFSURI *uri, MateVFSURI *dir)
{
	MateVFSFileInfo *dirinfo, *info;
	MateVFSURI *resolved_dir, *parent, *tmp;
	MateVFSResult res;
	gboolean is_in_dir;

	resolved_dir = NULL;
	parent = NULL;

	is_in_dir = FALSE;
	
	dirinfo = mate_vfs_file_info_new ();
	info = mate_vfs_file_info_new ();

	res = mate_vfs_get_file_info_uri (dir, dirinfo, MATE_VFS_FILE_INFO_DEFAULT);
	if (res != MATE_VFS_OK || dirinfo->type != MATE_VFS_FILE_TYPE_DIRECTORY) {
		goto out;
	}

	res = _mate_vfs_uri_resolve_all_symlinks_uri (dir, &resolved_dir);
	if (res != MATE_VFS_OK) {
		goto out;
	}
	
	res = _mate_vfs_uri_resolve_all_symlinks_uri (uri, &tmp);
	if (res != MATE_VFS_OK) {
		goto out;
	}
	
	parent = mate_vfs_uri_get_parent (tmp);
	mate_vfs_uri_unref (tmp);

	while (parent != NULL) {
		res = mate_vfs_get_file_info_uri (parent, info, MATE_VFS_FILE_INFO_DEFAULT);
		if (res != MATE_VFS_OK) {
			break;
		}

		if (dirinfo->valid_fields & MATE_VFS_FILE_INFO_FIELDS_DEVICE &&
		    dirinfo->valid_fields & MATE_VFS_FILE_INFO_FIELDS_INODE &&
		    info->valid_fields & MATE_VFS_FILE_INFO_FIELDS_DEVICE &&
		    info->valid_fields & MATE_VFS_FILE_INFO_FIELDS_INODE) {
			if (dirinfo->device == info->device &&
			    dirinfo->inode == info->inode) {
				is_in_dir = TRUE;
				break;
			}
		} else {
			if (mate_vfs_uri_equal (dir, parent)) {
				is_in_dir = TRUE;
				break;
			}
		}
		
		tmp = mate_vfs_uri_get_parent (parent);
		mate_vfs_uri_unref (parent);
		parent = tmp;
	}

 out:
	if (resolved_dir != NULL) {
		mate_vfs_uri_unref (resolved_dir);
	}
	if (parent != NULL) {
		mate_vfs_uri_unref (parent);
	}
	mate_vfs_file_info_unref (info);
	mate_vfs_file_info_unref (dirinfo);
	return is_in_dir;
}

/**
 * mate_vfs_url_show:
 * @url: url to be shown.
 * 
 * Launches the default application or component associated with the given @url.
 *
 * Return value: %MATE_VFS_OK if the default action was launched,
 * %MATE_VFS_ERROR_BAD_PARAMETERS for an invalid or non-existant @url,
 * %MATE_VFS_ERROR_NO_DEFAULT if no default action is associated with the @url.
 * Also error codes from mate_vfs_mime_action_launch()
 * or mate_vfs_mime_action_launch_with_env().
 *
 * Since: 2.4
 */
MateVFSResult
mate_vfs_url_show (const char *url)
{
	return mate_vfs_url_show_with_env (url, NULL);
}

/**
 * mate_vfs_url_show_with_env:
 * @url: url to be shown.
 * @envp: environment data.
 * 
 * Like mate_vfs_url_show() except that the default action will be launched
 * with the given environment.
 *
 * Return value: %MATE_VFS_OK if the default action was launched.
 * %MATE_VFS_ERROR_BAD_PARAMETERS for an invalid or non-existant @url,
 * %MATE_VFS_ERROR_NO_DEFAULT if no default action is associated with the @url.
 * Also error codes from mate_vfs_mime_application_launch_with_env()
 * or mate_vfs_mime_action_launch_with_env().
 *
 * Since: 2.4
 */
MateVFSResult
mate_vfs_url_show_with_env (const char  *url,
                             char       **envp)
{
	MateVFSMimeApplication *app;
	MateVFSMimeAction *action;
	MateVFSResult result;
	GList params;
	char *type;
	char *scheme;

	g_return_val_if_fail (url != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);

	scheme = mate_vfs_get_uri_scheme (url);
	if (scheme == NULL) {
		return MATE_VFS_ERROR_BAD_PARAMETERS;
	}
	
	/* check if this scheme requires special handling */
	if (_mate_vfs_use_handler_for_scheme (scheme)) {
		result = _mate_vfs_url_show_using_handler_with_env (url, envp);
		g_free (scheme);
		return result;
	}
	
	g_free (scheme);

	result = _mate_vfs_get_slow_mime_type_internal (url, &type);
	if (result != MATE_VFS_OK) {
		return result;
	}
	
	params.data = (char *) url;
	params.prev = NULL;
	params.next = NULL;

	app = mate_vfs_mime_get_default_application_for_uri (url, type);
	
	if (app != NULL) {
		result = mate_vfs_mime_application_launch_with_env (app, &params, envp);
		mate_vfs_mime_application_free (app);
		g_free (type);
		return result;
	}
	
	action = mate_vfs_mime_get_default_action (type);
	
	if (action != NULL) {
		result = mate_vfs_mime_action_launch_with_env (action, &params, envp);
		mate_vfs_mime_action_free (action);
		g_free (type);
		return result;
	}
	
	g_free (type);
	return MATE_VFS_ERROR_NO_DEFAULT;	
}	  

