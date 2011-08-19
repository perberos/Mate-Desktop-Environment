/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-uri.c - URI handling for the MATE Virtual File System.

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

   Author: Ettore Perazzoli <ettore@gnu.org>

*/

#include <config.h>
#include "mate-vfs-uri.h"

#include "mate-vfs-module.h"
#include "mate-vfs-private-utils.h"
#include "mate-vfs-transform.h"
#include "mate-vfs-utils.h"
#include <glib.h>
#include <stdio.h>
#include <string.h>

/* 
   split_toplevel_uri

   Extract hostname and username from "path" with length "path_len"

   examples:
       sunsite.unc.edu/pub/linux
       miguel@sphinx.nuclecu.unam.mx/c/nc
       tsx-11.mit.edu:8192/
       joe@foo.edu:11321/private
       joe:password@foo.se

   This function implements the following regexp: (whitespace for clarity)

   ( ( ([^:@/]*) (:[^@/]*)? @ )? ([^/:]*) (:([0-9]*)?) )?  (/.*)?
   ( ( ( user  ) (  pw  )?   )?   (host)    (port)?   )? (path <return value>)?

  It returns NULL if neither <host> nor <path> could be matched.

  port is checked to ensure that it does not exceed 0xffff.

  return value is <path> or is "/" if the path portion is not present
  All other arguments are set to 0 or NULL if their portions are not present

  pedantic: this function ends up doing an unbounded lookahead, making it 
  potentially O(n^2) instead of O(n).  This could be avoided.  Realistically, though,
  its just the password field.

  Differences between the old and the new implemention:

                     Old                     New
  localhost:8080     host="localhost:8080"   host="localhost" port=8080
  /Users/mikef       host=""                 host=NULL

*/ 


#define URI_MOVE_PAST_DELIMITER \
	do {							\
		cur_tok_start = (++cur);			\
		if (path_end == cur) {				\
			success = FALSE;			\
			goto done;				\
		}						\
	} while (0);


#define uri_strlen_to(from, to)  ( (to) - (from) )
#define uri_strdup_to(from, to)  g_strndup ((from), uri_strlen_to((from), (to)))

typedef struct {
	const char *chrs;
	gboolean primed;
	char bv[32];
} UriStrspnSet; 

static UriStrspnSet uri_strspn_sets[] = {
	{":@]" MATE_VFS_URI_PATH_STR, FALSE, ""},
	{"@" MATE_VFS_URI_PATH_STR, FALSE, ""},
	{":" MATE_VFS_URI_PATH_STR, FALSE, ""},
	{"]" MATE_VFS_URI_PATH_STR, FALSE, ""}
};

#define URI_DELIMITER_ALL_SET (uri_strspn_sets + 0)
#define URI_DELIMITER_USER_SET (uri_strspn_sets + 1)
#define URI_DELIMITER_HOST_SET (uri_strspn_sets + 2)
#define URI_DELIMITER_IPV6_SET (uri_strspn_sets + 3)

#define BV_SET(bv, idx) (bv)[((guchar)(idx))>>3] |= (1 << ( (idx) & 7) )
#define BV_IS_SET(bv, idx) ((bv)[((guchar)(idx))>>3] & (1 << ( (idx) & 7)))

static const char *
uri_strspn_to(const char *str, UriStrspnSet *set, const char *path_end)
{
	const char *cur;
	const char *cur_chr;

	if (!set->primed) {
		memset (set->bv, 0, sizeof(set->bv));
	
		for (cur_chr = set->chrs; '\0' != *cur_chr; cur_chr++) {
			BV_SET (set->bv, *cur_chr);
		}

		BV_SET (set->bv, '\0');
		set->primed = TRUE;
	}
	
	for (cur = str; cur < path_end && ! BV_IS_SET (set->bv, *cur); cur++) 
		;

	if (cur >= path_end || '\0' == *cur) {
		return NULL;
	}

	return cur;
}


static gchar *
split_toplevel_uri (const gchar *path, guint path_len,
		    gchar **host_return, gchar **user_return,
		    guint *port_return, gchar **password_return)
{
	const char *path_end;
	const char *cur_tok_start;
	const char *cur;
	const char *next_delimiter;
	char *ret;
	char *host;
	gboolean success;

	g_assert (host_return != NULL);
	g_assert (user_return != NULL);
	g_assert (port_return != NULL);
	g_assert (password_return != NULL);

	*host_return = NULL;
	*user_return = NULL;
	*port_return = 0;
	*password_return = NULL;
	ret = NULL;

	success = FALSE;

	if (path == NULL || path_len == 0) {
		return g_strdup ("/");
	}
	

	path_end = path + path_len;

	cur_tok_start = path;
	cur = uri_strspn_to (cur_tok_start, URI_DELIMITER_ALL_SET, path_end);

	if (cur != NULL) {
		const char *tmp;

		if (*cur == ':') {
			/* This ':' belongs to username or IPv6 address.*/
			tmp = uri_strspn_to (cur_tok_start, URI_DELIMITER_USER_SET, path_end);

			if (tmp == NULL || *tmp != '@') {
				tmp = uri_strspn_to (cur_tok_start, URI_DELIMITER_IPV6_SET, path_end);

				if (tmp != NULL && *tmp == ']') {
					cur = tmp;
				}
			}
		}
	}

	if (cur != NULL) {

		/* Check for IPv6 address. */
		if (*cur == ']') {

			/*  No username:password in the URI  */
			/*  cur points to ']'  */

			cur = uri_strspn_to (cur, URI_DELIMITER_HOST_SET, path_end);
		}
	}

	if (cur != NULL) {
		next_delimiter = uri_strspn_to (cur, URI_DELIMITER_USER_SET, path_end);
	} else {
		next_delimiter = NULL;
	}
	
	if (cur != NULL
		&& (*cur == '@'
		    || (next_delimiter != NULL && *next_delimiter != '/' ))) {

		/* *cur == ':' or '@' and string contains a @ before a / */

		if (uri_strlen_to (cur_tok_start, cur) > 0) {
			char *tmp;
			tmp = uri_strdup_to (cur_tok_start,cur);
			*user_return = mate_vfs_unescape_string (tmp, NULL);
			g_free (tmp);
		}

		if (*cur == ':') {
			URI_MOVE_PAST_DELIMITER;

			cur = uri_strspn_to(cur_tok_start, URI_DELIMITER_USER_SET, path_end);

			if (cur == NULL || *cur != '@') {
				success = FALSE;
				goto done;
			} else if (uri_strlen_to (cur_tok_start, cur) > 0) {
				char *tmp;
				tmp = uri_strdup_to (cur_tok_start,cur);
				*password_return = mate_vfs_unescape_string (tmp, NULL);
				g_free (tmp);
			}
		}

		if (*cur != '/') {
			URI_MOVE_PAST_DELIMITER;

			/* Move cur to point to ':' after ']' */
			cur = uri_strspn_to (cur_tok_start, URI_DELIMITER_IPV6_SET, path_end);

			if (cur != NULL && *cur == ']') {  /* For IPv6 address */
				cur = uri_strspn_to (cur, URI_DELIMITER_HOST_SET, path_end);
			} else {
				cur = uri_strspn_to (cur_tok_start, URI_DELIMITER_HOST_SET, path_end);
			}
		} else {
			cur_tok_start = cur;
		}
	}

	if (cur == NULL) {
		/* [^:/]+$ */
		if (uri_strlen_to (cur_tok_start, path_end) > 0) {
			*host_return = uri_strdup_to (cur_tok_start, path_end);
			if (*(path_end - 1) == MATE_VFS_URI_PATH_CHR) {
				ret = g_strdup (MATE_VFS_URI_PATH_STR);
			} else {
				ret = g_strdup ("");
			}
			success = TRUE;
		} else { /* No host, no path */
			success = FALSE;
		}

		goto done;

	} else if (*cur == ':') {
		guint port;
		/* [^:/]*:.* */

		if (uri_strlen_to (cur_tok_start, cur) > 0) {
			*host_return = uri_strdup_to (cur_tok_start, cur);
		} else {
			success = FALSE;
			goto done;	/*No host but a port?*/
		}

		URI_MOVE_PAST_DELIMITER;

		port = 0;

		for ( ; cur < path_end && g_ascii_isdigit (*cur); cur++) {
			port *= 10;
			port += *cur - '0'; 
		}

		/* We let :(/.*)$ be treated gracefully */
		if (*cur != '\0' && *cur != MATE_VFS_URI_PATH_CHR) {
			success = FALSE;
			goto done;	/* ...but this would be an error */
		} 

		if (port > 0xffff) {
			success = FALSE;
			goto done;
		}

		*port_return = port;

		cur_tok_start = cur;
		
	} else /* MATE_VFS_URI_PATH_CHR == *cur */ {
		/* ^[^:@/]+/.*$ */

		if (uri_strlen_to (cur_tok_start, cur) > 0) {
			*host_return = uri_strdup_to (cur_tok_start, cur);
		}

		cur_tok_start = cur;
	}

	if (*cur_tok_start != '\0' && uri_strlen_to (cur_tok_start, path_end) > 0) {
		ret = uri_strdup_to(cur, path_end);
	} else if (*host_return != NULL) {
		ret = g_strdup (MATE_VFS_URI_PATH_STR);
	}

	success = TRUE;

done:
	if (*host_return != NULL) {

		/* Check for an IPv6 address in square brackets.*/
		if (strchr (*host_return, '[') && strchr (*host_return, ']') && strchr (*host_return, ':')) {

			/* Extract the IPv6 address from square braced string. */
			host = g_ascii_strdown ((*host_return) + 1, strlen (*host_return) - 2);
		} else {
			host = g_ascii_strdown (*host_return, -1);
		}

		g_free (*host_return);
		*host_return = host;

	}

	/* If we didn't complete our mission, discard all the partials */
	if (!success) {
		g_free (*host_return);
		g_free (*user_return);
		g_free (*password_return);
		g_free (ret);

		*host_return = NULL;
		*user_return = NULL;
		*port_return = 0;
		*password_return = NULL;
		ret = NULL;
	}

	return ret;
}


static void
set_uri_element (MateVFSURI *uri,
		 const gchar *text,
		 guint len)
{
	char *escaped_text;

	if (text == NULL || len == 0) {
		uri->text = g_strdup ("/");
		return;
	}

	if (uri->parent == NULL && text[0] == '/' && text[1] == '/') {
		MateVFSToplevelURI *toplevel;

		toplevel = (MateVFSToplevelURI *) uri;
		uri->text = split_toplevel_uri (text + 2, len - 2,
						&toplevel->host_name,
						&toplevel->user_name,
						&toplevel->host_port,
						&toplevel->password);
	} else {
		uri->text = g_strndup (text, len);
	}

	/* FIXME: this should be handled/supported by the specific method.
	 * This is a quick and dirty hack to minimize the amount of changes
	 * right before a milestone release. 
	 * 
	 * Do some method specific escaping. This for instance converts
	 * '?' to %3F in every method except "http" where it has a special 
	 * meaning.
	 */
	if ( ! (strcmp (uri->method_string, "http") == 0 
	        || strcmp (uri->method_string, "https") == 0
		|| strcmp (uri->method_string, "dav") == 0
		|| strcmp (uri->method_string, "davs") == 0
	        || strcmp (uri->method_string, "ghelp") == 0
	        || strcmp (uri->method_string, "mate-help") == 0
	        || strcmp (uri->method_string, "help") == 0
		)) {

		escaped_text = mate_vfs_escape_set (uri->text, ";?&=+$,");
		g_free (uri->text);
		uri->text = escaped_text;
	}
	
	mate_vfs_remove_optional_escapes (uri->text);
	_mate_vfs_canonicalize_pathname (uri->text);
}

static const gchar *
get_method_string (const gchar *substring, gchar **method_string)
{
	const gchar *p;
	char *method;
	
	for (p = substring;
	     g_ascii_isalnum (*p) || *p == '+' || *p == '-' || *p == '.';
	     p++)
		;

	if (*p == ':'
#ifdef G_OS_WIN32
	              &&
	    !(p == substring + 1 && g_ascii_isalpha (*substring))
#endif
								 ) {
		/* Found toplevel method specification.  */
		method = g_strndup (substring, p - substring);
		*method_string = g_ascii_strdown (method, -1);
		g_free (method);
		p++;
	} else {
		*method_string = g_strdup ("file");
		p = substring;
	}
	return p;
}

static MateVFSURI *
parse_uri_substring (const gchar *substring, MateVFSURI *parent)
{
	MateVFSMethod *method;
	MateVFSURI *uri, *child_uri;
	gchar *method_string;
	const gchar *method_scanner;
	const gchar *extension_scanner;

	if (substring == NULL || *substring == '\0') {
		return NULL;
	}
	
	method_scanner = get_method_string (substring, &method_string);

	method = mate_vfs_method_get (method_string);
	if (!method) {
		g_free (method_string);
		return NULL;
	}

	uri = g_new0 (MateVFSURI, 1);
	uri->method = method;
	uri->method_string = method_string;
	uri->ref_count = 1;
	uri->parent = parent;

	extension_scanner = strchr (method_scanner, MATE_VFS_URI_MAGIC_CHR);

	if (extension_scanner == NULL) {
		set_uri_element (uri, method_scanner, strlen (method_scanner));
		return uri;
	}

	/* handle '#' */
	set_uri_element (uri, method_scanner, extension_scanner - method_scanner);

	if (strchr (extension_scanner, ':') == NULL) {
		/* extension is a fragment identifier */
		uri->fragment_id = g_strdup (extension_scanner + 1);
		return uri;
	}

	/* extension is a uri chain */
	child_uri = parse_uri_substring (extension_scanner + 1, uri);

	if (child_uri != NULL) {
		return child_uri;
	}

	return uri;
}

/**
 * mate_vfs_uri_new:
 * @text_uri: an escaped string representing a uri.
 * 
 * Create a new uri from @text_uri. Unsupported and unsafe methods
 * are not allowed and will result in %NULL being returned. URL
 * transforms are allowed.
 *
 * The @a text_uri must be an escaped URI string such as returned by 
 * mate_vfs_get_uri_from_local_path(), mate_vfs_make_uri_from_input(), 
 * or gtk_file_chooser_get_uri().
 * 
 * Return value: The new uri.
 */
MateVFSURI *
mate_vfs_uri_new (const gchar *text_uri)
{
#ifdef G_OS_WIN32
	gchar *method_string;
	const gchar *rest;

	rest = get_method_string (text_uri, &method_string);
	if (strcmp (method_string, "file") == 0) {
		gchar *slashified, *p, *full;
		MateVFSURI *result;

		if (g_ascii_strncasecmp (text_uri, "file://", 7) == 0)
			rest += 2;
		slashified = g_strdup (rest);
		for (p = slashified; *p; p++)
			if (*p == '\\')
				*p = '/';
		if (*slashified == '/')
			full = g_strconcat ("file://", slashified, NULL);
		else
			full = g_strconcat ("file:///", slashified, NULL);
		result = mate_vfs_uri_new_private (full, FALSE, FALSE, TRUE);
		g_free (full);
		g_free (slashified);
		g_free (method_string);

		return result;
	}
	g_free (method_string);
#endif
	return mate_vfs_uri_new_private (text_uri, FALSE, FALSE, TRUE);
}

MateVFSURI *
mate_vfs_uri_new_private (const gchar *text_uri,
			   gboolean allow_unknown_methods,
			   gboolean allow_unsafe_methods,
			   gboolean allow_transforms)
{
	MateVFSMethod *method;
	MateVFSTransform *trans;
	MateVFSToplevelURI *toplevel;
	MateVFSURI *uri, *child_uri;
	const gchar *method_scanner, *extension_scanner;
	gchar *method_string;
	gchar *new_uri_string = NULL;

	g_return_val_if_fail (text_uri != NULL, NULL);

	if (text_uri[0] == '\0') {
		return NULL;
	}

	method_scanner = get_method_string (text_uri, &method_string);
	if (strcmp (method_string, "pipe") == 0 && !allow_unsafe_methods) {
		g_free (method_string);
		return NULL;
	}

	toplevel = g_new (MateVFSToplevelURI, 1);
	toplevel->host_name = NULL;
	toplevel->host_port = 0;
	toplevel->user_name = NULL;
	toplevel->password = NULL;

	uri = (MateVFSURI *) toplevel;
	uri->parent = NULL;

	if (allow_transforms) {
		trans = mate_vfs_transform_get (method_string);
		if (trans != NULL && trans->transform) {
			const MateVFSContext *context;
			
			context = mate_vfs_context_peek_current ();
			(* trans->transform) (trans, 
					      method_scanner, 
					      &new_uri_string, 
					      (MateVFSContext *) context);
			if (new_uri_string != NULL) {
				toplevel->urn = g_strdup (text_uri);
				g_free (method_string);
				method_scanner = get_method_string (new_uri_string, &method_string);
			}
		}
	}
	
	method = mate_vfs_method_get (method_string);
	/* The toplevel URI element is special, as it also contains host/user
           information.  */
	uri->method = method;
	uri->ref_count = 1;
	uri->method_string = method_string;
	uri->text = NULL;
	uri->fragment_id = NULL;
	if (method == NULL && !allow_unknown_methods) {
		g_free (new_uri_string);
		mate_vfs_uri_unref (uri);
		return NULL;
	}

	extension_scanner = strchr (method_scanner, MATE_VFS_URI_MAGIC_CHR);
	if (extension_scanner == NULL) {
		set_uri_element (uri, method_scanner, strlen (method_scanner));
		g_free (new_uri_string);
		return uri;
	}

	/* handle '#' */
	set_uri_element (uri, method_scanner, extension_scanner - method_scanner);

	if (strchr (extension_scanner, ':') == NULL) {
		/* extension is a fragment identifier */
		uri->fragment_id = g_strdup (extension_scanner + 1);
		g_free (new_uri_string);
		return uri;
	}

	/* extension is a uri chain */
	child_uri = parse_uri_substring (extension_scanner + 1, uri);

	g_free (new_uri_string);

	if (child_uri != NULL) {
		return child_uri;
	}
	
	return uri;
}

/* Destroy an URI element, but not its parent.  */
static void
destroy_element (MateVFSURI *uri)
{
	g_free (uri->text);
	g_free (uri->fragment_id);
	g_free (uri->method_string);

	if (uri->parent == NULL) {
		MateVFSToplevelURI *toplevel;

		toplevel = (MateVFSToplevelURI *) uri;
		g_free (toplevel->host_name);
		g_free (toplevel->user_name);
		g_free (toplevel->password);
	}

	g_free (uri);
}

static gboolean
is_uri_relative (const char *uri)
{
	const char *current;

	/* RFC 2396 section 3.1 */
	for (current = uri ; 
		*current
		&& 	((*current >= 'a' && *current <= 'z')
			 || (*current >= 'A' && *current <= 'Z')
			 || (*current >= '0' && *current <= '9')
			 || ('-' == *current)
			 || ('+' == *current)
			 || ('.' == *current)) ;
	     current++);

	return  !(':' == *current);
}


/*
 * Remove "./" segments
 * Compact "../" segments inside the URI
 * Remove "." at the end of the URL 
 * Leave any ".."'s at the beginning of the URI
 
*/
/*
 * FIXME this is not the simplest or most time-efficent way
 * to do this.  Probably a far more clear way of doing this processing
 * is to split the path into segments, rather than doing the processing
 * in place.
 */
static void
remove_internal_relative_components (char *uri_current)
{
	char *segment_prev, *segment_cur;
	gsize len_prev, len_cur;

	len_prev = len_cur = 0;
	segment_prev = NULL;

	segment_cur = uri_current;

	while (*segment_cur) {
		len_cur = strcspn (segment_cur, "/");

		if (len_cur == 1 && segment_cur[0] == '.') {
			/* Remove "." 's */
			if (segment_cur[1] == '\0') {
				segment_cur[0] = '\0';
				break;
			} else {
				memmove (segment_cur, segment_cur + 2, strlen (segment_cur + 2) + 1);
				continue;
			}
		} else if (len_cur == 2 && segment_cur[0] == '.' && segment_cur[1] == '.' ) {
			/* Remove ".."'s (and the component to the left of it) that aren't at the
			 * beginning or to the right of other ..'s
			 */
			if (segment_prev) {
				if (! (len_prev == 2
				       && segment_prev[0] == '.'
				       && segment_prev[1] == '.')) {
				       	if (segment_cur[2] == '\0') {
						segment_prev[0] = '\0';
						break;
				       	} else {
						memmove (segment_prev, segment_cur + 3, strlen (segment_cur + 3) + 1);

						segment_cur = segment_prev;
						len_cur = len_prev;

						/* now we find the previous segment_prev */
						if (segment_prev == uri_current) {
							segment_prev = NULL;
						} else if (segment_prev - uri_current >= 2) {
							segment_prev -= 2;
							for ( ; segment_prev > uri_current && segment_prev[0] != '/' 
							      ; segment_prev-- );
							if (segment_prev[0] == '/') {
								segment_prev++;
							}
						}
						continue;
					}
				}
			}
		}

		/*Forward to next segment */

		if (segment_cur [len_cur] == '\0') {
			break;
		}
		 
		segment_prev = segment_cur;
		len_prev = len_cur;
		segment_cur += len_cur + 1;	
	}
	
}

/* If I had known this relative uri code would have ended up this long, I would
 * have done it a different way
 */
static char *
make_full_uri_from_relative (const char *base_uri, const char *uri)
{
	char *result = NULL;

	char *mutable_base_uri;
	char *mutable_uri;
	
	char *uri_current;
	gsize base_uri_length;
	char *separator;
	
	/* We may need one extra character
	 * to append a "/" to uri's that have no "/"
	 * (such as help:)
	 */

	mutable_base_uri = g_malloc(strlen(base_uri)+2);
	strcpy (mutable_base_uri, base_uri);
		
	uri_current = mutable_uri = g_strdup (uri);

	/* Chew off Fragment and Query from the base_url */

	separator = strrchr (mutable_base_uri, '#'); 

	if (separator) {
		*separator = '\0';
	}

	separator = strrchr (mutable_base_uri, '?');

	if (separator) {
		*separator = '\0';
	}

	if ('/' == uri_current[0] && '/' == uri_current [1]) {
		/* Relative URI's beginning with the authority
		 * component inherit only the scheme from their parents
		 */

		separator = strchr (mutable_base_uri, ':');

		if (separator) {
			separator[1] = '\0';
		}			  
	} else if ('/' == uri_current[0]) {
		/* Relative URI's beginning with '/' absolute-path based
		 * at the root of the base uri
		 */

		separator = strchr (mutable_base_uri, ':');

		/* g_assert (separator), really */
		if (separator) {
			/* If we start with //, skip past the authority section */
			if ('/' == separator[1] && '/' == separator[2]) {
				separator = strchr (separator + 3, '/');
				if (separator) {
					separator[0] = '\0';
				}
			} else {
				/* If there's no //, just assume the scheme is the root */
				separator[1] = '\0';
			}
		}
	} else if ('#' != uri_current[0]) {
		/* Handle the ".." convention for relative uri's */

		/* If there's a trailing '/' on base_url, treat base_url
		 * as a directory path.
		 * Otherwise, treat it as a file path, and chop off the filename
		 */

		base_uri_length = strlen (mutable_base_uri);
		if ('/' == mutable_base_uri[base_uri_length-1]) {
			/* Trim off '/' for the operation below */
			mutable_base_uri[base_uri_length-1] = 0;
		} else {
			separator = strrchr (mutable_base_uri, '/');
			if (separator) {
				/* Make sure we don't eat a domain part */
				char *tmp = separator - 1;
				if ((separator != mutable_base_uri) && (*tmp != '/')) {
					*separator = '\0';
				} else {
					/* Maybe there is no domain part and this is a toplevel URI's child */
					char *tmp2 = strstr (mutable_base_uri, ":///");
					if (tmp2 != NULL && tmp2 + 3 == separator) {
						*(separator + 1) = '\0';
					}
				}
			}
		}

		remove_internal_relative_components (uri_current);

		/* handle the "../"'s at the beginning of the relative URI */
		while (0 == strncmp ("../", uri_current, 3)) {
			uri_current += 3;
			separator = strrchr (mutable_base_uri, '/');
			if (separator) {
				*separator = '\0';
			} else {
				/* <shrug> */
				break;
			}
		}

		/* handle a ".." at the end */
		if (uri_current[0] == '.' && uri_current[1] == '.' 
		    && uri_current[2] == '\0') {

			uri_current += 2;
			separator = strrchr (mutable_base_uri, '/');
			if (separator) {
				*separator = '\0';
			}
		}

		/* Re-append the '/' */
		mutable_base_uri [strlen(mutable_base_uri)+1] = '\0';
		mutable_base_uri [strlen(mutable_base_uri)] = '/';
	}

	result = g_strconcat (mutable_base_uri, uri_current, NULL);
	g_free (mutable_base_uri); 
	g_free (mutable_uri); 
	
	return result;
}

/**
 * mate_vfs_uri_resolve_relative:
 * @base: base uri.
 * @relative_reference: a string representing a possibly relative uri reference.
 * 
 * Create a new uri from @relative_reference, relative to @base. The resolution
 * algorithm in some aspects follows <ulink url="http://www.ietf.org/rfc/rfc2396.txt">RFC
 * 2396</ulink>, section 5.2, but is not identical due to some extra assumptions MateVFS
 * makes about URIs.
 *
 * If @relative_reference begins with a valid scheme identifier followed by ':',
 * it is assumed to refer to an absolute URI, and a #MateVFSURI is created from
 * it using mate_vfs_uri_new(). 
 *
 * Otherwise, depending on its precise syntax, it inherits some aspects of the parent URI,
 * but the parents' fragment and query components are ignored.
 *
 * If @relative_reference begins with "//", it only inherits the @base scheme,
 * if it begins with '/' (i.e. is an absolute path reference), it inherits everything
 * ecxept the @base path. Otherwise, it replaces the part of @base after the last '/'.
 *
 * <note>
 *  This function should not be used by application authors unless they expect very
 *  distinct semantics. Instead, authors should use mate_vfs_uri_append_file_name(),
 *  mate_vfs_uri_append_path(), mate_vfs_uri_append_string() or
 *  mate_vfs_uri_resolve_symbolic_link().
 * </note>
 *
 * Return value: A #MateVFSURI referring to @relative_reference, or %NULL
 * if @relative_reference was malformed.
 */
MateVFSURI *
mate_vfs_uri_resolve_relative (const MateVFSURI *base,
				const gchar *relative_reference)
{
	char *text_base;
	char *text_new;
	MateVFSURI *uri;

	g_return_val_if_fail (relative_reference != NULL, NULL);

	if (base == NULL) {
		text_base = g_strdup ("");
	} else {
		text_base = mate_vfs_uri_to_string (base, 0);
	}

	if (is_uri_relative (relative_reference)) {
		text_new = make_full_uri_from_relative (text_base, 
							relative_reference);
	} else {
		text_new = g_strdup (relative_reference);
	}
	
	uri = mate_vfs_uri_new (text_new);

	g_free (text_base);
	g_free (text_new);

	return uri;
}

/**
 * mate_vfs_uri_resolve_symbolic_link:
 * @base: base uri.
 * @symbolic_link: a string representing a possibly relative or absolute path reference.
 * 
 * Create a new uri from @symbolic_link, relative to @base.
 *
 * If @symbolic_link begins with a '/', it replaces the path of @base,
 * otherwise it is appended after the last '/' character of @base.
 *
 * Return value: A new #MateVFSURI referring to @symbolic_link.
 *
 * Since: 2.16
 */
MateVFSURI *
mate_vfs_uri_resolve_symbolic_link (const MateVFSURI *base,
				     const gchar *symbolic_link)
{
	MateVFSURI *uri;

	g_return_val_if_fail (base != NULL, NULL);
	g_return_val_if_fail (symbolic_link != NULL, NULL);

	uri = mate_vfs_uri_dup (base);
	g_free (uri->text);
	uri->text = mate_vfs_resolve_symlink (mate_vfs_uri_get_path (base) != NULL ?
					       mate_vfs_uri_get_path (base) : "/",
					       symbolic_link);

	return uri;
}

/**
 * mate_vfs_uri_ref:
 * @uri: a #MateVFSURI.
 * 
 * Increment @uri's reference count.
 * 
 * Return value: @uri.
 */
MateVFSURI *
mate_vfs_uri_ref (MateVFSURI *uri)
{
	MateVFSURI *p;

	g_return_val_if_fail (uri != NULL, NULL);

	for (p = uri; p != NULL; p = p->parent)
		p->ref_count++;

	return uri;
}

/**
 * mate_vfs_uri_unref:
 * @uri: a #MateVFSURI.
 * 
 * Decrement @uri's reference count.  If the reference count reaches zero,
 * @uri is destroyed.
 */
void
mate_vfs_uri_unref (MateVFSURI *uri)
{
	MateVFSURI *p, *parent;

	g_return_if_fail (uri != NULL);
	g_return_if_fail (uri->ref_count > 0);

	for (p = uri; p != NULL; p = parent) {
		parent = p->parent;
		g_assert (p->ref_count > 0);
		p->ref_count--;
		if (p->ref_count == 0)
			destroy_element (p);
	}
}

/**
 * mate_vfs_uri_dup:
 * @uri: a #MateVFSURI.
 * 
 * Duplicate @uri.
 * 
 * Return value: a pointer to a new uri that is exactly the same as @uri.
 */
MateVFSURI *
mate_vfs_uri_dup (const MateVFSURI *uri)
{
	const MateVFSURI *p;
	MateVFSURI *new_uri, *child;

	if (uri == NULL) {
		return NULL;
	}

	new_uri = NULL;
	child = NULL;
	for (p = uri; p != NULL; p = p->parent) {
		MateVFSURI *new_element;

		if (p->parent == NULL) {
			MateVFSToplevelURI *toplevel;
			MateVFSToplevelURI *new_toplevel;

			toplevel = (MateVFSToplevelURI *) p;
			new_toplevel = g_new (MateVFSToplevelURI, 1);

			new_toplevel->host_name = g_strdup (toplevel->host_name);
			new_toplevel->host_port = toplevel->host_port;
			new_toplevel->user_name = g_strdup (toplevel->user_name);
			new_toplevel->password = g_strdup (toplevel->password);

			new_element = (MateVFSURI *) new_toplevel;
		} else {
			new_element = g_new (MateVFSURI, 1);
		}

		new_element->ref_count = 1;
		new_element->text = g_strdup (p->text);
		new_element->fragment_id = g_strdup (p->fragment_id);
		new_element->method_string = g_strdup (p->method_string);
		new_element->method = p->method;
		new_element->parent = NULL;

		if (child != NULL) {
			child->parent = new_element;
		} else {
			new_uri = new_element;
		}
			
		child = new_element;
	}

	return new_uri;
}

/**
 * mate_vfs_uri_append_string:
 * @uri: a #MateVFSURI.
 * @uri_fragment: a piece of a uri (ie a fully escaped partial path).
 * 
 * Create a new uri obtained by appending @uri_fragment to @uri.  This will take care
 * of adding an appropriate directory separator between the end of @uri and
 * the start of @uri_fragment if necessary.
 * 
 * Return value: The new uri obtained by combining @uri and @uri_fragment.
 */
MateVFSURI *
mate_vfs_uri_append_string (const MateVFSURI *uri,
			     const gchar *uri_fragment)
{
	gchar *uri_string;
	MateVFSURI *new_uri;
	gchar *new_string;
	guint len;

	g_return_val_if_fail (uri != NULL, NULL);
	g_return_val_if_fail (uri_fragment != NULL, NULL);

	uri_string = mate_vfs_uri_to_string (uri, MATE_VFS_URI_HIDE_NONE);
	len = strlen (uri_string);
	if (len == 0) {
		g_free (uri_string);
		return mate_vfs_uri_new (uri_fragment);
	}

	len--;
	while (uri_string[len] == MATE_VFS_URI_PATH_CHR && len > 0) {
		len--;
	}

	uri_string[len + 1] = '\0';

	while (*uri_fragment == MATE_VFS_URI_PATH_CHR) {
		uri_fragment++;
	}

	if (uri_fragment[0] != MATE_VFS_URI_MAGIC_CHR) {
		new_string = g_strconcat (uri_string, MATE_VFS_URI_PATH_STR, uri_fragment, NULL);
	} else {
		new_string = g_strconcat (uri_string, uri_fragment, NULL);
	}
	new_uri = mate_vfs_uri_new (new_string);

	g_free (new_string);
	g_free (uri_string);

	return new_uri;
}

/**
 * mate_vfs_uri_append_path:
 * @uri: a #MateVFSURI.
 * @path: a non-escaped file path.
 * 
 * Create a new uri obtained by appending @path to @uri.  This will take care
 * of adding an appropriate directory separator between the end of @uri and
 * the start of @path if necessary as well as escaping @path as necessary.
 * 
 * Return value: The new uri obtained by combining @uri and @path.
 */
MateVFSURI *
mate_vfs_uri_append_path (const MateVFSURI *uri,
			   const gchar *path)
{
	gchar *escaped_string;
	MateVFSURI *new_uri;

	g_return_val_if_fail (uri != NULL, NULL);
	g_return_val_if_fail (path != NULL, NULL);

	escaped_string = mate_vfs_escape_path_string (path);
	new_uri = mate_vfs_uri_append_string (uri, escaped_string);
	g_free (escaped_string);
	return new_uri;
}

/**
 * mate_vfs_uri_append_file_name:
 * @uri: a #MateVFSURI.
 * @filename: any "regular" file name (can include #, /, etc) in the file system encoding. This is not an escaped URI.
 * 
 * Create a new uri obtained by appending @file_name to @uri.  This will take care
 * of adding an appropriate directory separator between the end of @uri and
 * the start of @file_name if necessary. @file_name might, for instance, be the 
 * result of a call to g_dir_read_name().
 * 
 * Return value: The new uri obtained by combining @uri and @path.
 */
MateVFSURI *
mate_vfs_uri_append_file_name (const MateVFSURI *uri,
				const gchar *filename)
{
	gchar *escaped_string;
	MateVFSURI *new_uri;

	g_return_val_if_fail (uri != NULL, NULL);
	g_return_val_if_fail (filename != NULL, NULL);
	
	escaped_string = mate_vfs_escape_string (filename);
	new_uri = mate_vfs_uri_append_string (uri, escaped_string);
	g_free (escaped_string);
	return new_uri;
}


/**
 * mate_vfs_uri_to_string:
 * @uri: a #MateVFSURI.
 * @hide_options: bitmask specifying what uri elements (e.g. password,
 * user name etc.) should not be represented in the returned string.
 * 
 * Translate @uri into a printable string.  The string will not
 * contain the uri elements specified by @hide_options.
 *
 * A file: URI on Win32 might look like file:///x:/foo/bar.txt. Note
 * that the part after file:// is not a legal file name, you need to
 * remove the / in front of the drive letter. This function does that
 * automatically if @hide_options specifies that the toplevel method,
 * user name, password, host name and host port should be hidden.
 *
 * On the other hand, a file: URI for a UNC path looks like
 * file:////server/share/foo/bar.txt, and in that case the part after
 * file:// is the correct file name.
 * 
 * Return value: a malloc'd printable string representing @uri.
 */
gchar *
mate_vfs_uri_to_string (const MateVFSURI *uri,
			 MateVFSURIHideOptions hide_options)
{
	GString *string;
	gchar *result, *tmp;

	g_return_val_if_fail (uri != NULL, NULL);

	string = g_string_new (uri->method_string);
	g_string_append_c (string, ':');

	if (uri->parent == NULL) {
		MateVFSToplevelURI *top_level_uri = (MateVFSToplevelURI *)uri;
		gboolean shown_user_pass = FALSE;

		if (top_level_uri->user_name != NULL
			|| top_level_uri->host_name != NULL
			|| (uri->text != NULL && uri->text[0] == MATE_VFS_URI_PATH_CHR)) {
			/* don't append '//' for uris such as pipe:foo */
			g_string_append (string, "//");
		}

		if ((hide_options & MATE_VFS_URI_HIDE_TOPLEVEL_METHOD) != 0) {
			g_string_free (string, TRUE); /* throw away method */
			string = g_string_new ("");
		}

		if (top_level_uri->user_name != NULL
			&& (hide_options & MATE_VFS_URI_HIDE_USER_NAME) == 0) {
			tmp = mate_vfs_escape_string (top_level_uri->user_name);
			g_string_append (string, tmp);
			g_free (tmp);
			shown_user_pass = TRUE;
		}

		if (top_level_uri->password != NULL
			&& (hide_options & MATE_VFS_URI_HIDE_PASSWORD) == 0) {
			tmp = mate_vfs_escape_string (top_level_uri->password);
			g_string_append_c (string, ':');
			g_string_append (string, tmp);
			g_free (tmp);
			shown_user_pass = TRUE;
		}

		if (shown_user_pass) {
			g_string_append_c (string, '@');
		}

		if (top_level_uri->host_name != NULL
			&& (hide_options & MATE_VFS_URI_HIDE_HOST_NAME) == 0) {

			/* Check for an IPv6 address. */

			if (strchr (top_level_uri->host_name, ':')) {
				g_string_append_c (string, '[');
				g_string_append (string, top_level_uri->host_name);
				g_string_append_c (string, ']');
			} else {
				g_string_append (string, top_level_uri->host_name);
			}
		}
		
		if (top_level_uri->host_port > 0 
			&& (hide_options & MATE_VFS_URI_HIDE_HOST_PORT) == 0) {
			gchar temp[128];
			sprintf (temp, ":%u", top_level_uri->host_port);
			g_string_append (string, temp);
		}

	}
	
	if (uri->text != NULL) {
#ifndef G_OS_WIN32
		g_string_append (string, uri->text);
#else
		if (uri->text[0] == '/' &&
		    g_ascii_isalpha (uri->text[1]) &&
		    uri->text[2] == ':' &&
		    (hide_options & MATE_VFS_URI_HIDE_TOPLEVEL_METHOD) &&
		    (hide_options & MATE_VFS_URI_HIDE_USER_NAME) &&
		    (hide_options & MATE_VFS_URI_HIDE_PASSWORD) &&
		    (hide_options & MATE_VFS_URI_HIDE_HOST_NAME) &&
		    (hide_options & MATE_VFS_URI_HIDE_HOST_PORT))
			g_string_append (string, uri->text + 1);
		else
			g_string_append (string, uri->text);
#endif
	}

	if (uri->fragment_id != NULL 
		&& (hide_options & MATE_VFS_URI_HIDE_FRAGMENT_IDENTIFIER) == 0) {
		g_string_append_c (string, '#');
		g_string_append (string, uri->fragment_id);
	}

	if (uri->parent != NULL) {
		gchar *uri_str;
		uri_str = mate_vfs_uri_to_string (uri->parent, hide_options);
		g_string_prepend_c (string, '#');
		g_string_prepend (string, uri_str);
		g_free (uri_str);
	}

	result = string->str;
	g_string_free (string, FALSE);

	return result;
}

/**
 * mate_vfs_uri_is_local:
 * @uri: a #MateVFSURI.
 * 
 * Check if @uri is a local URI. Note that the return value of this
 * function entirely depends on the #MateVFSMethod associated with the
 * URI. It is up to the method author to distinguish between remote URIs
 * add URIs referring to entities on the local computer.
 *
 * Warning, this can be slow, as it does i/o to detect things like NFS mounts.
 * 
 * Return value: %TRUE if @uri refers to a local entity, %FALSE otherwise.
 */
gboolean
mate_vfs_uri_is_local (const MateVFSURI *uri)
{
	g_return_val_if_fail (uri != NULL, FALSE);

	/* It's illegal to have is_local be NULL in a method.
	 * That's why we fail here. If we decide that it's legal,
	 * then we can change this into an if statement.
	 */
	g_return_val_if_fail (VFS_METHOD_HAS_FUNC (uri->method, is_local), FALSE);

	return uri->method->is_local (uri->method, uri);
}

/**
 * mate_vfs_uri_has_parent:
 * @uri: a #MateVFSURI.
 * 
 * Check if uri has a parent or not.
 * 
 * Return value: %TRUE if @uri has a parent, %FALSE otherwise.
 */
gboolean
mate_vfs_uri_has_parent (const MateVFSURI *uri)
{
	MateVFSURI *parent;

	g_return_val_if_fail (uri != NULL, FALSE);

	parent = mate_vfs_uri_get_parent (uri);
	if (parent == NULL) {
		return FALSE;
	}

	mate_vfs_uri_unref (parent);
	return TRUE;
}

/**
 * mate_vfs_uri_get_parent:
 * @uri: a #MateVFSURI.
 * 
 * Retrieve @uri's parent uri.
 * 
 * Return value: a pointer to @uri's parent uri.
 */
MateVFSURI *
mate_vfs_uri_get_parent (const MateVFSURI *uri)
{
	g_return_val_if_fail (uri != NULL, NULL);

	if (uri->text != NULL && strchr (uri->text, MATE_VFS_URI_PATH_CHR) != NULL) {
		gchar *p;
		guint len;

		len = strlen (uri->text);
		p = uri->text + len - 1;

		/* Skip trailing slashes  */
		while (p != uri->text && *p == MATE_VFS_URI_PATH_CHR)
			p--;

		/* Search backwards to the next slash.  */
		while (p != uri->text && *p != MATE_VFS_URI_PATH_CHR)
			p--;

		/* Get the parent without slashes  */
		while (p > uri->text + 1 && p[-1] == MATE_VFS_URI_PATH_CHR)
			p--;

		if (p[1] != '\0') {
			MateVFSURI *new_uri;
			char *new_uri_text;
			int length;

			/* build a new parent text */
			length = p - uri->text;			
			if (length == 0) {
				new_uri_text = g_strdup (MATE_VFS_URI_PATH_STR);
			} else {
				new_uri_text = g_malloc (length + 1);
				memcpy (new_uri_text, uri->text, length);
				new_uri_text[length] = '\0';
			}

			/* copy the uri and replace the uri text with the new parent text */
			new_uri = mate_vfs_uri_dup (uri);
			g_free (new_uri->text);
			new_uri->text = new_uri_text;

			/* The parent doesn't have the child's fragment */
			g_free (new_uri->fragment_id);
			new_uri->fragment_id = NULL;
			
			return new_uri;
		}
	}

	return mate_vfs_uri_dup (uri->parent);
}

/**
 * mate_vfs_uri_get_toplevel:
 * @uri: a #MateVFSURI.
 * 
 * Retrieve the toplevel uri in @uri.
 * 
 * Return value: a pointer to the toplevel uri object.
 */
MateVFSToplevelURI *
mate_vfs_uri_get_toplevel (const MateVFSURI *uri)
{
	const MateVFSURI *p;

	g_return_val_if_fail (uri != NULL, NULL);

	for (p = uri; p->parent != NULL; p = p->parent)
		;

	return (MateVFSToplevelURI *) p;
}

/**
 * mate_vfs_uri_get_host_name:
 * @uri: a #MateVFSURI.
 * 
 * Retrieve the host name for @uri.
 * 
 * Return value: a string representing the host name.
 */
const gchar *
mate_vfs_uri_get_host_name (const MateVFSURI *uri)
{
	MateVFSToplevelURI *toplevel;

	g_return_val_if_fail (uri != NULL, NULL);

	toplevel = mate_vfs_uri_get_toplevel (uri);
	return toplevel->host_name;
}

/**
 * mate_vfs_uri_get_scheme:
 * @uri: a #MateVFSURI.
 *
 * Retrieve the scheme used for @uri.
 *
 * Return value: a string representing the scheme.
 */
const gchar *
mate_vfs_uri_get_scheme (const MateVFSURI *uri)
{
	g_return_val_if_fail (uri != NULL, NULL);

	return uri->method_string;
}

/**
 * mate_vfs_uri_get_host_port:
 * @uri: a #MateVFSURI.
 * 
 * Retrieve the host port number in @uri.
 * 
 * Return value: The host port number used by @uri.  If the value is zero, the
 * default port value for the specified toplevel access method is used.
 */
guint
mate_vfs_uri_get_host_port (const MateVFSURI *uri)
{
	MateVFSToplevelURI *toplevel;

	g_return_val_if_fail (uri != NULL, 0);

	toplevel = mate_vfs_uri_get_toplevel (uri);
	return toplevel->host_port;
}

/**
 * mate_vfs_uri_get_user_name:
 * @uri: a #MateVFSURI.
 * 
 * Retrieve the user name in @uri.
 * 
 * Return value: a string representing the user name in @uri.
 */
const gchar *
mate_vfs_uri_get_user_name (const MateVFSURI *uri)
{
	MateVFSToplevelURI *toplevel;

	g_return_val_if_fail (uri != NULL, NULL);

	toplevel = mate_vfs_uri_get_toplevel (uri);
	return toplevel->user_name;
}

/**
 * mate_vfs_uri_get_password:
 * @uri: a #MateVFSURI.
 * 
 * Retrieve the password for @uri.
 * 
 * Return value: The password for @uri.
 */
const gchar *
mate_vfs_uri_get_password (const MateVFSURI *uri)
{
	MateVFSToplevelURI *toplevel;

	g_return_val_if_fail (uri != NULL, NULL);

	toplevel = mate_vfs_uri_get_toplevel (uri);
	return toplevel->password;
}

/**
 * mate_vfs_uri_set_host_name:
 * @uri: a #MateVFSURI.
 * @host_name: a string representing a host name.
 * 
 * Set @host_name as the host name accessed by @uri.
 */
void
mate_vfs_uri_set_host_name (MateVFSURI *uri,
			     const gchar *host_name)
{
	MateVFSToplevelURI *toplevel;

	g_return_if_fail (uri != NULL);

	toplevel = mate_vfs_uri_get_toplevel (uri);

	g_free (toplevel->host_name);
	toplevel->host_name = g_strdup (host_name);
}

/**
 * mate_vfs_uri_set_host_port:
 * @uri: a #MateVFSURI.
 * @host_port: a TCP/IP port number.
 * 
 * Set the host port number in @uri.  If @host_port is zero, the default port
 * for @uri's toplevel access method is used.
 */
void
mate_vfs_uri_set_host_port (MateVFSURI *uri,
			     guint host_port)
{
	MateVFSToplevelURI *toplevel;

	g_return_if_fail (uri != NULL);

	toplevel = mate_vfs_uri_get_toplevel (uri);

	toplevel->host_port = host_port;
}

/**
 * mate_vfs_uri_set_user_name:
 * @uri: a #MateVFSURI.
 * @user_name: a string representing a user name on the host accessed by @uri.
 * 
 * Set @user_name as the user name for @uri.
 */
void
mate_vfs_uri_set_user_name (MateVFSURI *uri,
			     const gchar *user_name)
{
	MateVFSToplevelURI *toplevel;

	g_return_if_fail (uri != NULL);

	toplevel = mate_vfs_uri_get_toplevel (uri);

	g_free (toplevel->user_name);
	toplevel->user_name = g_strdup (user_name);
}

/**
 * mate_vfs_uri_set_password:
 * @uri: a #MateVFSURI.
 * @password: a password string.
 * 
 * Set @password as the password for @uri.
 */
void
mate_vfs_uri_set_password (MateVFSURI *uri,
			    const gchar *password)
{
	MateVFSToplevelURI *toplevel;

	g_return_if_fail (uri != NULL);

	toplevel = mate_vfs_uri_get_toplevel (uri);

	g_free (toplevel->password);
	toplevel->password = g_strdup (password);
}

static gboolean
string_match (const gchar *a, const gchar *b)
{
	if (a == NULL || *a == '\0') {
		return b == NULL || *b == '\0';
	}

	if (a == NULL || b == NULL)
		return FALSE;

	return strcmp (a, b) == 0;
}

static gboolean
compare_elements (const MateVFSURI *a,
		  const MateVFSURI *b)
{
	if (!string_match (a->text, b->text)
		|| !string_match (a->method_string, b->method_string))
		return FALSE;

	/* The following should never fail, but we make sure anyway. */
	return a->method == b->method;
}

/**
 * mate_vfs_uri_equal:
 * @a: a #MateVFSURI.
 * @b: a #MateVFSURI.
 * 
 * Compare @a and @b.
 * 
 * FIXME: This comparison should take into account the possiblity
 * that unreserved characters may be escaped.
 * ...or perhaps mate_vfs_uri_new() should unescape unreserved characters?
 *
 * Return value: %TRUE if @a and @b are equal, %FALSE otherwise.
 */
gboolean
mate_vfs_uri_equal (const MateVFSURI *a,
		     const MateVFSURI *b)
{
	const MateVFSToplevelURI *toplevel_a;
	const MateVFSToplevelURI *toplevel_b;

	g_return_val_if_fail (a != NULL, FALSE);
	g_return_val_if_fail (b != NULL, FALSE);

	if (a == b)
		return TRUE;

	/* First check non-toplevel elements.  */
	while (a->parent != NULL && b->parent != NULL) {
		if (!compare_elements (a, b)) {
			return FALSE;
		}
	}

	/* Now we should be at toplevel for both.  */
	if (a->parent != NULL || b->parent != NULL) {
		return FALSE;
	}

	if (!compare_elements (a, b)) {
		return FALSE;
	}

	toplevel_a = (MateVFSToplevelURI *) a;
	toplevel_b = (MateVFSToplevelURI *) b;

	/* Finally, compare the extra toplevel members.  */
	return toplevel_a->host_port == toplevel_b->host_port
	    && string_match (toplevel_a->host_name, toplevel_b->host_name)
	    && string_match (toplevel_a->user_name, toplevel_b->user_name)
	    && string_match (toplevel_a->password, toplevel_b->password);
}

/* Convenience function that deals with the problem where we distinguish
 * uris "foo://bar.com" and "foo://bar.com/" but we do not define
 * what a child item of "foo://bar.com" would be -- to work around this,
 * we will consider both "foo://bar.com" and "foo://bar.com/" the parent
 * of "foo://bar.com/child"
 */
static gboolean
uri_matches_as_parent (const MateVFSURI *possible_parent, const MateVFSURI *parent)
{
	MateVFSURI *alternate_possible_parent;
	gboolean result;

	if (possible_parent->text == NULL ||
	    strlen (possible_parent->text) == 0) {
		alternate_possible_parent = mate_vfs_uri_append_string (possible_parent,
			MATE_VFS_URI_PATH_STR);

		result = mate_vfs_uri_equal (alternate_possible_parent, parent);
		
		mate_vfs_uri_unref (alternate_possible_parent);
		return result;
	}
	
	return mate_vfs_uri_equal (possible_parent, parent);
}

/**
 * mate_vfs_uri_is_parent:
 * @possible_parent: a #MateVFSURI.
 * @possible_child: a #MateVFSURI.
 * @recursive: a flag to turn recursive check on.
 * 
 * Check if @possible_child is contained by @possible_parent.
 * If @recursive is %FALSE, just try the immediate parent directory, else
 * search up through the hierarchy.
 * 
 * Return value: %TRUE if @possible_child is contained in  @possible_parent.
 */
gboolean
mate_vfs_uri_is_parent (const MateVFSURI *possible_parent,
			 const MateVFSURI *possible_child,
			 gboolean recursive)
{
	gboolean result;
	MateVFSURI *item_parent_uri;
	MateVFSURI *item;

	g_return_val_if_fail (possible_parent != NULL, FALSE);
	g_return_val_if_fail (possible_child != NULL, FALSE);

	if (!recursive) {
		item_parent_uri = mate_vfs_uri_get_parent (possible_child);

		if (item_parent_uri == NULL) {
			return FALSE;
		}

		result = uri_matches_as_parent (possible_parent, item_parent_uri);	
		mate_vfs_uri_unref (item_parent_uri);

		return result;
	}
	
	item = mate_vfs_uri_dup (possible_child);
	for (;;) {
		item_parent_uri = mate_vfs_uri_get_parent (item);
		mate_vfs_uri_unref (item);
		
		if (item_parent_uri == NULL) {
			return FALSE;
		}

		result = uri_matches_as_parent (possible_parent, item_parent_uri);
	
		if (result) {
			mate_vfs_uri_unref (item_parent_uri);
			break;
		}

		item = item_parent_uri;
	}

	return result;
}

/**
 * mate_vfs_uri_get_path:
 * @uri: a #MateVFSURI.
 * 
 * Retrieve full path name for @uri.
 * 
 * Return value: a pointer to the full path name in @uri.  Notice that the
 * pointer points to the path name stored in @uri, so the path name returned must not
 * be modified nor freed.
 */
const gchar *
mate_vfs_uri_get_path (const MateVFSURI *uri)
{
	/* FIXME bugzilla.eazel.com 1472 */
	/* this is based on the assumtion that uri->text won't contain the
	 * query string.
	 */
	g_return_val_if_fail (uri != NULL, NULL);

	return uri->text;
}

/**
 * mate_vfs_uri_get_fragment_id:
 * @uri: a #MateVFSURI.
 * 
 * Retrieve the optional fragment identifier for @uri.
 * 
 * Return value: a pointer to the fragment identifier for the @uri or %NULL.
 */
const gchar *
mate_vfs_uri_get_fragment_identifier (const MateVFSURI *uri)
{
	g_return_val_if_fail (uri != NULL, NULL);

	return uri->fragment_id;
}

/**
 * mate_vfs_uri_extract_dirname:
 * @uri: a #MateVFSURI.
 * 
 * Extract the name of the directory in which the file pointed to by @uri is
 * stored as a newly allocated string.  The string will end with a
 * %MATE_VFS_URI_PATH_CHR.
 * 
 * Return value: a pointer to the newly allocated string representing the
 * parent directory.
 */
gchar *
mate_vfs_uri_extract_dirname (const MateVFSURI *uri)
{
	const gchar *base;

	g_return_val_if_fail (uri != NULL, NULL);

	if (uri->text == NULL) {
		return NULL;
	}
	
	base = strrchr (uri->text, MATE_VFS_URI_PATH_CHR);

	if (base == NULL || base == uri->text) {
		return g_strdup (MATE_VFS_URI_PATH_STR);
	}

	return g_strndup (uri->text, base - uri->text);
}

/**
 * mate_vfs_uri_extract_short_name:
 * @uri: a #MateVFSURI.
 * 
 * Retrieve base file name for @uri, ignoring any trailing path separators.
 * This matches the XPG definition of basename, but not g_basename. This is
 * often useful when you want the name of something that's pointed to by a
 * uri, and don't care whether the uri has a directory or file form.
 * If @uri points to the root of a domain, returns the host name. If there's
 * no host name, returns %MATE_VFS_URI_PATH_STR.
 * 
 * See also: mate_vfs_uri_extract_short_path_name().
 * 
 * Return value: a pointer to the newly allocated string representing the
 * unescaped short form of the name.
 */
gchar *
mate_vfs_uri_extract_short_name (const MateVFSURI *uri)
{
	gchar *escaped_short_path_name, *short_path_name;
	const gchar *host_name;

	g_return_val_if_fail (uri != NULL, NULL);

	escaped_short_path_name = mate_vfs_uri_extract_short_path_name (uri);
	short_path_name = mate_vfs_unescape_string (escaped_short_path_name, "/");
	g_free (escaped_short_path_name);

	if (short_path_name != NULL
		&& strcmp (short_path_name, MATE_VFS_URI_PATH_STR) != 0) {
		return short_path_name;
	}

	host_name = mate_vfs_uri_get_host_name (uri);

	if (host_name != NULL && strlen (host_name) != 0) {
		g_free (short_path_name);
		return g_strdup (host_name);
	} else if (short_path_name != NULL) {
		return short_path_name;
	} else {
		return g_strdup (mate_vfs_uri_get_path (uri));
	}
}

/**
 * mate_vfs_uri_extract_short_path_name:
 * @uri: a #MateVFSURI.
 * 
 * Retrieve base file name for @uri, ignoring any trailing path separators.
 * This matches the XPG definition of basename, but not g_basename. This is
 * often useful when you want the name of something that's pointed to by a
 * uri, and don't care whether the uri has a directory or file form.
 * If @uri points to the root (including the root of any domain),
 * returns %MATE_VFS_URI_PATH_STR.
 * 
 * See also: mate_vfs_uri_extract_short_name(). 
 * 
 * Return value: a pointer to the newly allocated string representing the
 * escaped short form of the name.
 */
gchar *
mate_vfs_uri_extract_short_path_name (const MateVFSURI *uri)
{
	const gchar *p, *short_name_start, *short_name_end;

	g_return_val_if_fail (uri != NULL, NULL);

	if (uri->text == NULL) {
		return NULL;
	}

	/* Search for the last run of non-'/' characters. */
	p = uri->text;
	short_name_start = NULL;
	short_name_end = p;
	do {
		if (*p == '\0' || *p == MATE_VFS_URI_PATH_CHR) {
			/* While we are in a run of non-separators, short_name_end is NULL. */
			if (short_name_end == NULL)
				short_name_end = p;
		} else {
			/* While we are in a run of separators, short_name_end is not NULL. */
			if (short_name_end != NULL) {
				short_name_start = p;
				short_name_end = NULL;
			}
		}
	} while (*p++ != '\0');
	g_assert (short_name_end != NULL);
	
	/* If we never found a short name, that means that the string is all
	   directory separators. Since it can't be an empty string, that means
	   it points to the root, so "/" is a good result.
	*/
	if (short_name_start == NULL) {
		return g_strdup (MATE_VFS_URI_PATH_STR);
	}

	/* Return a copy of the short name. */
	return g_strndup (short_name_start, short_name_end - short_name_start);
}

/* The following functions are useful for creating URI hash tables.  */

/**
 * mate_vfs_uri_hequal:
 * @a: a pointer to a #MateVFSURI.
 * @b: a pointer to a #MateVFSURI.
 *
 * Function intended for use as a hash table "are these two items
 * the same" comparison. Useful for creating a hash table of uris.
 *
 * Return value: %TRUE if the uris are the same.
 */
gint
mate_vfs_uri_hequal (gconstpointer a,
		      gconstpointer b)
{
	return mate_vfs_uri_equal (a, b);
}

/**
 * mate_vfs_uri_hash:
 * @p: a pointer to a #MateVFSURI.
 *
 * Creates an integer value from a #MateVFSURI, appropriate
 * for using as the key to a hash table entry.
 *
 * Return value: a hash key corresponding to @p.
 */
guint
mate_vfs_uri_hash (gconstpointer p)
{
	const MateVFSURI *uri;
	const MateVFSURI *uri_p;
	guint hash_value;

#define HASH_STRING(value, string)		\
	if ((string) != NULL)			\
		(value) ^= g_str_hash (string);

#define HASH_NUMBER(value, number)		\
	(value) ^= number;

	uri = (const MateVFSURI *) p;
	hash_value = 0;

	for (uri_p = uri; uri_p != NULL; uri_p = uri_p->parent) {
		HASH_STRING (hash_value, uri_p->text);
		HASH_STRING (hash_value, uri_p->method_string);

		if (uri_p->parent == NULL) {
			const MateVFSToplevelURI *toplevel;

			toplevel = (const MateVFSToplevelURI *) uri_p;

			HASH_STRING (hash_value, toplevel->host_name);
			HASH_NUMBER (hash_value, toplevel->host_port);
			HASH_STRING (hash_value, toplevel->user_name);
			HASH_STRING (hash_value, toplevel->password);
		}
	}

	return hash_value;

#undef HASH_STRING
#undef HASH_NUMBER
}

/**
 * mate_vfs_uri_list_ref:
 * @list: list of #MateVFSURI elements.
 *
 * Increments the reference count of the items in @list by one.
 *
 * Return value: @list.
 */
GList *
mate_vfs_uri_list_ref (GList *list)
{
	g_list_foreach (list, (GFunc) mate_vfs_uri_ref, NULL);
	return list;
}

/**
 * mate_vfs_uri_list_unref:
 * @list: list of #MateVFSURI elements.
 *
 * Decrements the reference count of the items in @list by one.
 * Note that the list is *not freed* even if each member of the list
 * is freed.
 *
 * Return value: @list.
 */
GList *
mate_vfs_uri_list_unref (GList *list)
{
	g_list_foreach (list, (GFunc) mate_vfs_uri_unref, NULL);
	return list;
}

/**
 * mate_vfs_uri_list_copy:
 * @list: list of #MateVFSURI elements.
 *
 * Creates a duplicate of @list, and references each member of
 * that list.
 *
 * Return value: a newly referenced duplicate of @list.
 */
GList *
mate_vfs_uri_list_copy (GList *list)
{
	return g_list_copy (mate_vfs_uri_list_ref (list));
}

/**
 * mate_vfs_uri_list_free:
 * @list: list of #MateVFSURI elements.
 *
 * Decrements the reference count of each member of @list by one,
 * and frees the list itself.
 */
void
mate_vfs_uri_list_free (GList *list)
{
	g_list_free (mate_vfs_uri_list_unref (list));
}

/**
 * mate_vfs_uri_make_full_from_relative:
 * @base_uri: a string representing the base uri.
 * @relative_uri: a uri fragment/reference to be appended to @base_uri.
 * 
 * Returns a full uri given a full base uri, and a secondary uri which may
 * be relative.
 *
 * Return value: a newly allocated string containing the uri 
 * (%NULL for some bad errors).
 */
char *
mate_vfs_uri_make_full_from_relative (const char *base_uri,
				       const char *relative_uri)
{
	char *result = NULL;

	/* See section 5.2 in RFC 2396 */

	if (base_uri == NULL && relative_uri == NULL) {
		result = NULL;
	} else if (base_uri == NULL) {
		result = g_strdup (relative_uri);
	} else if (relative_uri == NULL) {
		result = g_strdup (base_uri);
	} else if (is_uri_relative (relative_uri)) {
		result = make_full_uri_from_relative (base_uri, relative_uri);
	} else {
		result = g_strdup (relative_uri);
	}
	
	return result;
}

/**
 * mate_vfs_uri_list_parse:
 * @uri_list: string consists of #MateVFSURIs and/or paths seperated by newline character.
 * 
 * Extracts a list of #MateVFSURI objects from a standard text/uri-list,
 * such as one you would get on a drop operation.  Use
 * mate_vfs_uri_list_free() when you are done with the list.
 *
 * Return value: a #GList of #MateVFSURIs.
 */
GList*
mate_vfs_uri_list_parse (const gchar* uri_list)
{
	/* Note that this is mostly very stolen from old libmate/mate-mime.c */

	const gchar *p, *q;
	gchar *retval;
	MateVFSURI *uri;
	GList *result = NULL;

	g_return_val_if_fail (uri_list != NULL, NULL);

	p = uri_list;

	/* We don't actually try to validate the URI according to RFC
	 * 2396, or even check for allowed characters - we just ignore
	 * comments and trim whitespace off the ends.  We also
	 * allow LF delimination as well as the specified CRLF.
	 */
	while (p != NULL) {
		if (*p != '#') {
			while (g_ascii_isspace (*p))
				p++;

			q = p;
			while ((*q != '\0')
			       && (*q != '\n')
			       && (*q != '\r'))
				q++;

			if (q > p) {
				q--;
				while (q > p
				       && g_ascii_isspace (*q))
					q--;

				retval = g_malloc (q - p + 2);
				strncpy (retval, p, q - p + 1);
				retval[q - p + 1] = '\0';

				uri = mate_vfs_uri_new (retval);

				g_free (retval);

				if (uri != NULL)
					result = g_list_prepend (result, uri);
			}
		}
		p = strchr (p, '\n');
		if (p != NULL)
			p++;
	}

	return g_list_reverse (result);
}

/* enumerations from "mate-vfs-uri.h"
 * This is normally automatically-generated, but glib-mkenums could not
 * guess the correct get_type() function name for this enum.
 */
GType
mate_vfs_uri_hide_options_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GFlagsValue values[] = {
      { MATE_VFS_URI_HIDE_NONE, "MATE_VFS_URI_HIDE_NONE", "none" },
      { MATE_VFS_URI_HIDE_USER_NAME, "MATE_VFS_URI_HIDE_USER_NAME", "user-name" },
      { MATE_VFS_URI_HIDE_PASSWORD, "MATE_VFS_URI_HIDE_PASSWORD", "password" },
      { MATE_VFS_URI_HIDE_HOST_NAME, "MATE_VFS_URI_HIDE_HOST_NAME", "host-name" },
      { MATE_VFS_URI_HIDE_HOST_PORT, "MATE_VFS_URI_HIDE_HOST_PORT", "host-port" },
      { MATE_VFS_URI_HIDE_TOPLEVEL_METHOD, "MATE_VFS_URI_HIDE_TOPLEVEL_METHOD", "toplevel-method" },
      { MATE_VFS_URI_HIDE_FRAGMENT_IDENTIFIER, "MATE_VFS_URI_HIDE_FRAGMENT_IDENTIFIER", "fragment-identifier" },
      { 0, NULL, NULL }
    };
    etype = g_flags_register_static ("MateVFSURIHideOptions", values);
  }
  return etype;
}
