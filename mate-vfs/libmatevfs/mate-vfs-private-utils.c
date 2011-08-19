/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-utils.c - Private utility functions for the MATE Virtual
   File System.

   Copyright (C) 1999 Free Software Foundation

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

   Author: Ettore Perazzoli <ettore@comm2000.it>

   `_mate_vfs_canonicalize_pathname()' derived from code by Brian Fox and Chet
   Ramey in GNU Bash, the Bourne Again SHell.  Copyright (C) 1987, 1988, 1989,
   1990, 1991, 1992 Free Software Foundation, Inc.  */

#include <config.h>
#include "mate-vfs-private-utils.h"

#include "mate-vfs-utils.h"
#include "mate-vfs-cancellation.h"
#include "mate-vfs-ops.h"
#include "mate-vfs-uri.h"
#include <errno.h>
#include <glib.h>
#include <mateconf/mateconf-client.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#ifndef G_OS_WIN32
#include <sys/socket.h>
#include <sys/wait.h>
#else
#include <winsock2.h>
#endif

#define MATECONF_URL_HANDLER_PATH      	 "/desktop/mate/url-handlers/"
#define MATECONF_DEFAULT_TERMINAL_EXEC_PATH "/desktop/mate/applications/terminal/exec"
#define MATECONF_DEFAULT_TERMINAL_ARG_PATH  "/desktop/mate/applications/terminal/exec_arg"

/* Check whether the node is IPv6 enabled.*/
gboolean
_mate_vfs_have_ipv6 (void)
{
#ifdef ENABLE_IPV6
	int s;

	s = socket (AF_INET6, SOCK_STREAM, 0);
	if (s != -1) {
		close (s);
		return TRUE;
	}

#endif
	return FALSE;
}

static int
find_next_slash (const char *path, int current_offset)
{
	const char *match;
	
	g_assert (current_offset <= strlen (path));
	
	match = strchr (path + current_offset, MATE_VFS_URI_PATH_CHR);
	return match == NULL ? -1 : match - path;
}

static int
find_slash_before_offset (const char *path, int to)
{
	int result;
	int next_offset;

	result = -1;
	next_offset = 0;
	for (;;) {
		next_offset = find_next_slash (path, next_offset);
		if (next_offset < 0 || next_offset >= to) {
			break;
		}
		result = next_offset;
		next_offset++;
	}
	return result;
}

static void
collapse_slash_runs (char *path, int from_offset)
{
	int i;
	/* Collapse multiple `/'s in a row. */
	for (i = from_offset;; i++) {
		if (path[i] != MATE_VFS_URI_PATH_CHR) {
			break;
		}
	}

	if (from_offset < i) {
		memmove (path + from_offset, path + i, strlen (path + i) + 1);
		i = from_offset + 1;
	}
}

/* Canonicalize path, and return a new path.  Do everything in situ.  The new
   path differs from path in:

     Multiple `/'s are collapsed to a single `/'.
     Leading `./'s and trailing `/.'s are removed.
     Non-leading `../'s and trailing `..'s are handled by removing
     portions of the path.  */
gchar *
_mate_vfs_canonicalize_pathname (gchar *path)
{
	int i, marker;

	if (path == NULL || strlen (path) == 0) {
		return "";
	}

	/* Walk along path looking for things to compact. */
	for (i = 0, marker = 0;;) {
		if (!path[i])
			break;

		/* Check for `../', `./' or trailing `.' by itself. */
		if (path[i] == '.') {
			/* Handle trailing `.' by itself. */
			if (path[i + 1] == '\0') {
				if (i > 1 && path[i - 1] == MATE_VFS_URI_PATH_CHR) {
					/* strip the trailing /. */
					path[i - 1] = '\0';
				} else {
					/* convert path "/." to "/" */
					path[i] = '\0';
				}
				break;
			}

			/* Handle `./'. */
			if (path[i + 1] == MATE_VFS_URI_PATH_CHR) {
				memmove (path + i, path + i + 2, 
					 strlen (path + i + 2) + 1);
				if (i == 0) {
					/* don't leave leading '/' for paths that started
					 * as relative (.//foo)
					 */
					collapse_slash_runs (path, i);
					marker = 0;
				}
				continue;
			}

			/* Handle `../' or trailing `..' by itself. 
			 * Remove the previous xxx/ part 
			 */
			if (path[i + 1] == '.'
			    && (path[i + 2] == MATE_VFS_URI_PATH_CHR
				|| path[i + 2] == '\0')) {

				/* ignore ../ at the beginning of a path */
				if (i != 0) {
					marker = find_slash_before_offset (path, i - 1);

					/* Either advance past '/' or point to the first character */
					marker ++;
					if (path [i + 2] == '\0' && marker > 1) {
						/* If we are looking at a /.. at the end of the uri and we
						 * need to eat the last '/' too.
						 */
						 marker--;
					}
					g_assert(marker < i);
					
					if (path[i + 2] == MATE_VFS_URI_PATH_CHR) {
						/* strip the entire ../ string */
						i++;
					}

					memmove (path + marker, path + i + 2,
						 strlen (path + i + 2) + 1);
					i = marker;
				} else {
					i = 2;
					if (path[i] == MATE_VFS_URI_PATH_CHR) {
						i++;
					}
				}
				collapse_slash_runs (path, i);
				continue;
			}
		}
		
		/* advance to the next '/' */
		i = find_next_slash (path, i);

		/* If we didn't find any slashes, then there is nothing left to do. */
		if (i < 0) {
			break;
		}

		marker = i++;
		collapse_slash_runs (path, i);
	}
	return path;
}

/**
 * mate_vfs_create_temp:
 * @prefix: Prefix for the name of the temporary file
 * @name_return: Pointer to a pointer that, on return, will point to
 * the dynamically allocated name for the new temporary file created.
 * @handle_return: Pointer to a variable that will hold a file handle for
 * the new temporary file on return.
 * 
 * Create a temporary file whose name is prefixed with @prefix, and return an
 * open file handle for it in @*handle_return.
 * 
 * Return value: An integer value representing the result of the operation
 **/
MateVFSResult
mate_vfs_create_temp (const gchar *prefix,
		       gchar **name_return,
		       MateVFSHandle **handle_return)
{
	MateVFSHandle *handle;
	MateVFSResult result;
	gchar *name;
	gint fd;

	while (1) {
		name = g_strdup_printf("%sXXXXXX", prefix);
		fd = g_mkstemp (name);

		if (fd < 0)
			return MATE_VFS_ERROR_INTERNAL;
		
#ifdef HAVE_FCHMOD
		fchmod (fd, 0600);
#endif
		close (fd);

		result = mate_vfs_open
			(&handle, name,
			 MATE_VFS_OPEN_WRITE | MATE_VFS_OPEN_READ);

		if (result == MATE_VFS_OK) {
			*name_return = name;
			*handle_return = handle;
			return MATE_VFS_OK;
		}

		if (result != MATE_VFS_ERROR_FILE_EXISTS) {
			*name_return = NULL;
			*handle_return = NULL;
			g_free (name);
			return result;
		}
	}
}

#ifndef G_OS_WIN32

/* The following comes from GNU Wget with minor changes by myself.
   Copyright (C) 1995, 1996, 1997, 1998 Free Software Foundation, Inc.  */

/* Converts struct tm to time_t, assuming the data in tm is UTC rather
   than local timezone (mktime assumes the latter).

   Contributed by Roger Beeman <beeman@cisco.com>, with the help of
   Mark Baushke <mdb@cisco.com> and the rest of the Gurus at CISCO.  */
static time_t
mktime_from_utc (struct tm *t)
{
	time_t tl, tb;

	tl = mktime (t);
	if (tl == -1)
		return -1;
	tb = mktime (gmtime (&tl));
	return (tl <= tb ? (tl + (tl - tb)) : (tl - (tb - tl)));
}

/* Check whether the result of strptime() indicates success.
   strptime() returns the pointer to how far it got to in the string.
   The processing has been successful if the string is at `GMT' or
   `+X', or at the end of the string.

   In extended regexp parlance, the function returns 1 if P matches
   "^ *(GMT|[+-][0-9]|$)", 0 otherwise.  P being NULL (a valid result of
   strptime()) is considered a failure and 0 is returned.  */
static int
check_end (const gchar *p)
{
	if (!p)
		return 0;
	while (g_ascii_isspace (*p))
		++p;
	if (!*p
	    || (p[0] == 'G' && p[1] == 'M' && p[2] == 'T')
	    || ((p[0] == '+' || p[1] == '-') && g_ascii_isdigit (p[1])))
		return 1;
	else
		return 0;
}

#endif /* G_OS_WIN32 */

/* Convert TIME_STRING time to time_t.  TIME_STRING can be in any of
   the three formats RFC2068 allows the HTTP servers to emit --
   RFC1123-date, RFC850-date or asctime-date.  Timezones are ignored,
   and should be GMT.

   We use strptime() to recognize various dates, which makes it a
   little bit slacker than the RFC1123/RFC850/asctime (e.g. it always
   allows shortened dates and months, one-digit days, etc.).  It also
   allows more than one space anywhere where the specs require one SP.
   The routine should probably be even more forgiving (as recommended
   by RFC2068), but I do not have the time to write one.

   Return the computed time_t representation, or -1 if all the
   schemes fail.

   Needless to say, what we *really* need here is something like
   Marcus Hennecke's atotm(), which is forgiving, fast, to-the-point,
   and does not use strptime().  atotm() is to be found in the sources
   of `phttpd', a little-known HTTP server written by Peter Erikson.  */
gboolean
mate_vfs_atotm (const gchar *time_string,
		 time_t *value_return)
{
#ifndef G_OS_WIN32
	struct tm t;

	/* Roger Beeman says: "This function dynamically allocates struct tm
	   t, but does no initialization.  The only field that actually
	   needs initialization is tm_isdst, since the others will be set by
	   strptime.  Since strptime does not set tm_isdst, it will return
	   the data structure with whatever data was in tm_isdst to begin
	   with.  For those of us in timezones where DST can occur, there
	   can be a one hour shift depending on the previous contents of the
	   data area where the data structure is allocated."  */
	t.tm_isdst = -1;

	/* Note that under foreign locales Solaris strptime() fails to
	   recognize English dates, which renders this function useless.  I
	   assume that other non-GNU strptime's are plagued by the same
	   disease.  We solve this by setting only LC_MESSAGES in
	   i18n_initialize(), instead of LC_ALL.

	   Another solution could be to temporarily set locale to C, invoke
	   strptime(), and restore it back.  This is slow and dirty,
	   however, and locale support other than LC_MESSAGES can mess other
	   things, so I rather chose to stick with just setting LC_MESSAGES.

	   Also note that none of this is necessary under GNU strptime(),
	   because it recognizes both international and local dates.  */

	/* NOTE: We don't use `%n' for white space, as OSF's strptime uses
	   it to eat all white space up to (and including) a newline, and
	   the function fails if there is no newline (!).

	   Let's hope all strptime() implementations use ` ' to skip *all*
	   whitespace instead of just one (it works that way on all the
	   systems I've tested it on).  */

	/* RFC1123: Thu, 29 Jan 1998 22:12:57 */
	if (check_end (strptime (time_string, "%a, %d %b %Y %T", &t))) {
		*value_return = mktime_from_utc (&t);
		return TRUE;
	}

	/* RFC850:  Thu, 29-Jan-98 22:12:57 */
	if (check_end (strptime (time_string, "%a, %d-%b-%y %T", &t))) {
		*value_return = mktime_from_utc (&t);
		return TRUE;
	}

	/* asctime: Thu Jan 29 22:12:57 1998 */
	if (check_end (strptime (time_string, "%a %b %d %T %Y", &t))) {
		*value_return = mktime_from_utc (&t);
		return TRUE;
	}

	/* Failure.  */
	return FALSE;
#else 
	g_error ("Not yet implemented: mate_vfs_atotm()");
	return FALSE;
#endif /* G_OS_WIN32 */
}

/* _mate_vfs_istr_has_prefix
 * copy-pasted from Caja
 */
gboolean
_mate_vfs_istr_has_prefix (const char *haystack, const char *needle)
{
	const char *h, *n;
	char hc, nc;

	/* Eat one character at a time. */
	h = haystack == NULL ? "" : haystack;
	n = needle == NULL ? "" : needle;
	do {
		if (*n == '\0') {
			return TRUE;
		}
		if (*h == '\0') {
			return FALSE;
		}
		hc = *h++;
		nc = *n++;
		hc = g_ascii_tolower (hc);
		nc = g_ascii_tolower (nc);
	} while (hc == nc);
	return FALSE;
}

/* _mate_vfs_istr_has_suffix
 * copy-pasted from Caja
 */
gboolean
_mate_vfs_istr_has_suffix (const char *haystack, const char *needle)
{
	const char *h, *n;
	char hc, nc;

	if (needle == NULL) {
		return TRUE;
	}
	if (haystack == NULL) {
		return needle[0] == '\0';
	}
		
	/* Eat one character at a time. */
	h = haystack + strlen (haystack);
	n = needle + strlen (needle);
	do {
		if (n == needle) {
			return TRUE;
		}
		if (h == haystack) {
			return FALSE;
		}
		hc = *--h;
		nc = *--n;
		hc = g_ascii_tolower (hc);
		nc = g_ascii_tolower (nc);
	} while (hc == nc);
	return FALSE;
}

/**
 * _mate_vfs_use_handler_for_scheme:
 * @scheme: the URI scheme
 *
 * Checks MateConf to see if there is a URL handler
 * defined for this scheme and if it is enabled.
 *
 * Return value: TRUE if handler is defined and enabled,
 * FALSE otherwise.
 *
 * Since: 2.4
 */
gboolean
_mate_vfs_use_handler_for_scheme (const char *scheme)
{
	MateConfClient *client;
	gboolean ret;
	char *path;
	
	g_return_val_if_fail (scheme != NULL, FALSE);
	
	if (!mateconf_is_initialized ()) {
		if (!mateconf_init (0, NULL, NULL)) {
			return FALSE;
		}
	}
	
	client = mateconf_client_get_default ();
	path = g_strconcat (MATECONF_URL_HANDLER_PATH, scheme, "/enabled", NULL);
	ret = mateconf_client_get_bool (client, path, NULL);
	
	g_free (path);
	g_object_unref (G_OBJECT (client));
	
	return ret;
}

/**
 * _mate_vfs_url_show_using_handler_with_env:
 * @url: the url to show
 * @envp: environment for the handler
 * 
 * Same as mate_url_show_using_handler except that the handler
 * will be launched with the given environment.
 *
 * Return value: MATE_VFS_OK on success.
 * MATE_VFS_ERROR_BAD_PAREMETER if the URL is invalid.
 * MATE_VFS_ERROR_NOT_SUPPORTED if no handler is defined.
 * MATE_VFS_ERROR_PARSE if the handler command can not be parsed.
 * MATE_VFS_ERROR_LAUNCH if the handler command can not be launched.
 * MATE_VFS_ERROR_INTERNAL for internal/MateConf errors.
 *
 * Since: 2.4
 */
MateVFSResult
_mate_vfs_url_show_using_handler_with_env (const char  *url,
			                    char       **envp)
{
	MateConfClient *client;
	char *path;
	char *scheme;
	char *template;
	char **argv;
	int argc;
	int i;
	gboolean ret;
	
	g_return_val_if_fail (url != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);
	
	scheme = mate_vfs_get_uri_scheme (url);
	
	g_return_val_if_fail (scheme != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);
	
	if (!mateconf_is_initialized ()) {
		if (!mateconf_init (0, NULL, NULL)) {
			g_free (scheme);
			return MATE_VFS_ERROR_INTERNAL;
		}
	}

	client = mateconf_client_get_default ();
	path = g_strconcat (MATECONF_URL_HANDLER_PATH, scheme, "/command", NULL);
	template = mateconf_client_get_string (client, path, NULL);
	g_free (path);

	if (template == NULL) {
		g_free (template);
		g_free (scheme);
		g_object_unref (G_OBJECT (client));
		return MATE_VFS_ERROR_NO_HANDLER;
	}
	
	if (!g_shell_parse_argv (template,
				 &argc,
				 &argv,
				 NULL)) {
		g_free (template);
		g_free (scheme);
		g_object_unref (G_OBJECT (client));
		return MATE_VFS_ERROR_PARSE;
	}

	g_free (template);

	path = g_strconcat (MATECONF_URL_HANDLER_PATH, scheme, "/needs_terminal", NULL);
	if (mateconf_client_get_bool (client, path, NULL)) {
		if (!_mate_vfs_prepend_terminal_to_vector (&argc, &argv)) {
			g_free (path);
			g_free (scheme);
			g_strfreev (argv);
			return MATE_VFS_ERROR_INTERNAL;
		}
	}
	g_free (path);
	g_free (scheme);
	
	g_object_unref (G_OBJECT (client));

	for (i = 0; i < argc; i++) {
		char *arg;
		char **strs;

		if (!strstr (argv[i], "%s"))
			continue;

		/* we can't simply printf the uri into argv[i], since the format
		 * string might contain other specifiers (%d and friends) or multiple
		 * references to the URI, which may result in crashes if expanded using
		 * printf. */
		arg = argv[i];

		strs = g_strsplit (argv[i], "%s", 0);
		argv[i] = g_strjoinv (url, strs);
		g_strfreev (strs);

		g_free (arg);
	}

	ret = g_spawn_async (NULL /* working directory */,
			     argv,
			     envp,
			     G_SPAWN_SEARCH_PATH /* flags */,
			     NULL /* child_setup */,
			     NULL /* data */,
			     NULL /* child_pid */,
			     NULL);
	g_strfreev (argv);

	if (!ret) {
		return MATE_VFS_ERROR_LAUNCH;
	}

	return MATE_VFS_OK;
}

/* This is a copy from libmate, internalized here to avoid
   strange dependency loops */

/**
 * _mate_vfs_prepend_terminal_to_vector:
 * @argc: a pointer to the vector size
 * @argv: a pointer to the vector
 *
 * Prepends a terminal (either the one configured as default in MateVFS
 * or one of the common xterm emulators) to the passed in vector, modifying
 * it in the process. The vector should be allocated with #g_malloc, as 
 * this will #g_free the original vector. Also all elements must
 * have been allocated separately. That is the standard glib/MATE way of
 * doing vectors. If the integer that @argc points to is negative, the
 * size will first be computed. Also note that passing in pointers to a vector
 * that is empty, will just create a new vector for you.
 *
 * Return value: TRUE if successful, FALSE otherwise.
 *
 * Since: 2.4
 */
gboolean
_mate_vfs_prepend_terminal_to_vector (int    *argc,
				       char ***argv)
{
#ifndef G_OS_WIN32
        char **real_argv;
        int real_argc;
        int i, j;
	char **term_argv = NULL;
	int term_argc = 0;
	MateConfClient *client;

	char *terminal = NULL;
	char **the_argv;

        g_return_val_if_fail (argc != NULL, FALSE);
        g_return_val_if_fail (argv != NULL, FALSE);

	/* sanity */
        if(*argv == NULL) {
                *argc = 0;
	}
	
	the_argv = *argv;

	/* compute size if not given */
	if (*argc < 0) {
		for (i = 0; the_argv[i] != NULL; i++)
			;
		*argc = i;
	}

	if (!mateconf_is_initialized ()) {
		if (!mateconf_init (0, NULL, NULL)) {
			return FALSE;
		}
	}

	client = mateconf_client_get_default ();
	terminal = mateconf_client_get_string (client, MATECONF_DEFAULT_TERMINAL_EXEC_PATH, NULL);
	
	if (terminal) {
		gchar *exec_flag;
		exec_flag = mateconf_client_get_string (client, MATECONF_DEFAULT_TERMINAL_ARG_PATH, NULL);

		if (exec_flag == NULL) {
			term_argc = 1;
			term_argv = g_new0 (char *, 2);
			term_argv[0] = terminal;
			term_argv[1] = NULL;
		} else {
			term_argc = 2;
			term_argv = g_new0 (char *, 3);
			term_argv[0] = terminal;
			term_argv[1] = exec_flag;
			term_argv[2] = NULL;
		}
	}

	g_object_unref (G_OBJECT (client));

	if (term_argv == NULL) {
		char *check;

		term_argc = 2;
		term_argv = g_new0 (char *, 3);

		check = g_find_program_in_path ("mate-terminal");
		if (check != NULL) {
			term_argv[0] = check;
			/* Note that mate-terminal takes -x and
			 * as -e in mate-terminal is broken we use that. */
			term_argv[1] = g_strdup ("-x");
		} else {
			if (check == NULL)
				check = g_find_program_in_path ("nxterm");
			if (check == NULL)
				check = g_find_program_in_path ("color-xterm");
			if (check == NULL)
				check = g_find_program_in_path ("rxvt");
			if (check == NULL)
				check = g_find_program_in_path ("xterm");
			if (check == NULL)
				check = g_find_program_in_path ("dtterm");
			if (check == NULL) {
				check = g_strdup ("xterm");
				g_warning ("couldn't find a terminal, falling back to xterm");
			}
			term_argv[0] = check;
			term_argv[1] = g_strdup ("-e");
		}
	}

        real_argc = term_argc + *argc;
        real_argv = g_new (char *, real_argc + 1);

        for (i = 0; i < term_argc; i++)
                real_argv[i] = term_argv[i];

        for (j = 0; j < *argc; j++, i++)
                real_argv[i] = (char *)the_argv[j];

	real_argv[i] = NULL;

	g_free (*argv);
	*argv = real_argv;
	*argc = real_argc;

	/* we use g_free here as we sucked all the inner strings
	 * out from it into real_argv */
	g_free (term_argv);
	return TRUE;
#else
	return FALSE;
#endif /* G_OS_WIN32 */
}		  

#ifndef G_OS_WIN32
/**
 * _mate_vfs_set_fd_flags:
 * @fd: a valid file descriptor
 * @flags: file status flags to set
 *
 * Set the file status flags part of the descriptor’s flags to the
 * value specified by @flags.
 *
 * Return value: TRUE if successful, FALSE otherwise.
 *
 * Since: 2.7
 */

static gboolean
_set_fd_flags (int fd, int flags)
{
	int val;

	val = fcntl (fd, F_GETFL, 0);
	if (val < 0) {
		g_warning ("fcntl() F_GETFL failed: %s", strerror (errno));
		return FALSE;
	}

	val |= flags;
	
	val = fcntl (fd, F_SETFL, val);
	if (val < 0) {
		g_warning ("fcntl() F_SETFL failed: %s", strerror (errno));
		return FALSE;
	}

	return TRUE;
}

/**
 * _mate_vfs_clear_fd_flags:
 * @fd: a valid file descriptor
 * @flags: file status flags to clear
 *
 * Clear the flags sepcified by @flags of the file status flags part of the 
 * descriptor flags. 
 *
 * Return value: TRUE if successful, FALSE otherwise.
 *
 * Since: 2.7
 */

static gboolean
_clear_fd_flags (int fd, int flags)
{
	int val;

	val = fcntl (fd, F_GETFL, 0);
	if (val < 0) {
		g_warning ("fcntl() F_GETFL failed: %s", strerror (errno));
		return FALSE;
	}

	val &= ~flags;
	
	val = fcntl (fd, F_SETFL, val);
	if (val < 0) {
		g_warning ("fcntl() F_SETFL failed: %s", strerror (errno));
		return FALSE;
	}
	
	return TRUE;

}
#endif /* G_OS_WIN32 */

gboolean
_mate_vfs_socket_set_blocking (int sock_fd, gboolean blocking)
{
#ifndef G_OS_WIN32
	gboolean result;
	if (blocking) {
		result = _clear_fd_flags (sock_fd, O_NONBLOCK);
	} else {
		result = _set_fd_flags (sock_fd, O_NONBLOCK);
	}

	return result;
#else 
	u_long val;
	
	val = blocking ? 0 : 1;
 
	if (ioctlsocket (sock_fd, FIONBIO, &val) == SOCKET_ERROR) {
		g_warning ("ioctlsocket(FIONBIO) failed: %s",
			   _mate_vfs_winsock_strerror (WSAGetLastError ()));
		return FALSE;
	}

	return TRUE;
#endif /* G_OS_WIN32 */
}

/**
 * _mate_vfs_pipe:
 * @fds: a pointer to an array or two ints
 *
 * Creates a unidirectional IPC pipe. On Unix, a pipe. On Win32, a
 * pair of connected TCP sockets (thus actually bidirectional, but
 * ignore that).
 *
 * Return value: TRUE if successful, FALSE otherwise.
 *
 * Since: 2.11
 */

int
_mate_vfs_pipe (int *fds)
{
#ifndef G_OS_WIN32
	return pipe (fds);
#else
	SOCKET temp, socket1 = -1, socket2 = -1;
	struct sockaddr_in saddr;
	int len;
	u_long arg;
	fd_set read_set, write_set;
	struct timeval tv;

	temp = socket (AF_INET, SOCK_STREAM, 0);
	
	if (temp == INVALID_SOCKET) {
		_mate_vfs_map_winsock_error_to_errno ();
		goto out0;
	}
  	
	arg = 1;
	if (ioctlsocket (temp, FIONBIO, &arg) == SOCKET_ERROR) {
		_mate_vfs_map_winsock_error_to_errno ();
		goto out0;
	}

	memset (&saddr, 0, sizeof (saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = 0;
	saddr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);

	if (bind (temp, (struct sockaddr *)&saddr, sizeof (saddr))) {
		_mate_vfs_map_winsock_error_to_errno ();
		goto out0;
	}

	if (listen (temp, 1) == SOCKET_ERROR) {
		_mate_vfs_map_winsock_error_to_errno ();
		goto out0;
	}

	len = sizeof (saddr);
	if (getsockname (temp, (struct sockaddr *)&saddr, &len)) {
		_mate_vfs_map_winsock_error_to_errno ();
		goto out0;
	}

	socket1 = socket (AF_INET, SOCK_STREAM, 0);
	
	if (socket1 == INVALID_SOCKET) {
		_mate_vfs_map_winsock_error_to_errno ();
		goto out0;
	}

	arg = 1;
	if (ioctlsocket (socket1, FIONBIO, &arg) == SOCKET_ERROR) { 
		_mate_vfs_map_winsock_error_to_errno ();
		goto out1;
	}

	if (connect (socket1, (struct sockaddr  *)&saddr, len) != SOCKET_ERROR ||
			WSAGetLastError () != WSAEWOULDBLOCK) {
		_mate_vfs_map_winsock_error_to_errno ();
		goto out1;
	}

	FD_ZERO (&read_set);
	FD_SET (temp, &read_set);

	tv.tv_sec = 0;
	tv.tv_usec = 0;

	if (select (0, &read_set, NULL, NULL, NULL) == SOCKET_ERROR) {
		_mate_vfs_map_winsock_error_to_errno ();
		goto out1;
	}

	if (!FD_ISSET (temp, &read_set)) {
		errno = EIO;	/* Oh well, whatever */
		goto out1;
	}

	socket2 = accept (temp, (struct sockaddr *) &saddr, &len);
	if (socket2 == INVALID_SOCKET) {
		_mate_vfs_map_winsock_error_to_errno ();
		goto out1;
	}

	FD_ZERO (&write_set);
	FD_SET (socket1, &write_set);

	tv.tv_sec = 0;
	tv.tv_usec = 0;

	if (select (0, NULL, &write_set, NULL, NULL) == SOCKET_ERROR) {
		_mate_vfs_map_winsock_error_to_errno ();
		goto out2;
	}

	if (!FD_ISSET (socket1, &write_set)) {
		errno = EIO;
		goto out2;
	}

	arg = 0;
	if (ioctlsocket (socket1, FIONBIO, &arg) == SOCKET_ERROR) {
		_mate_vfs_map_winsock_error_to_errno ();
		goto out2;
	}

	arg = 0;
	if (ioctlsocket (socket2, FIONBIO, &arg) == SOCKET_ERROR) {
		_mate_vfs_map_winsock_error_to_errno ();
		goto out2;
	}

	fds[0] = socket1;
	fds[1] = socket2;

	closesocket (temp);

	return 0;

out2:
	closesocket (socket2);
out1:
	closesocket (socket1);
out0:
	closesocket (temp);

	return -1;

#endif
}

gboolean
_mate_vfs_pipe_set_blocking  (int pipe_fd, gboolean blocking)
{
	return _mate_vfs_socket_set_blocking (pipe_fd, blocking);
}


#ifdef G_OS_WIN32
void
_mate_vfs_map_winsock_error_to_errno (void)
{
	errno = WSAGetLastError ();
	switch (errno) {
	case WSAEBADF:
		errno = EBADF; break;
	case WSAEWOULDBLOCK:
		errno = EAGAIN; break;
	default:
		errno = EIO; break; /* Oh well */
	}
}

const char *
_mate_vfs_winsock_strerror (int error)
{

	switch (error) {
	case WSAEOPNOTSUPP:
		return "Operation not supported on transport endpoint";
	case WSAEPFNOSUPPORT:
		return "Protocol family not supported";
	case WSAECONNRESET:
		return "Connection reset by peer";
	case WSAENOBUFS:
		return "No buffer space available";
	case WSAEAFNOSUPPORT:
		return "Address family not supported by protocol family";
	case WSAENOTSOCK:
		return "Socket operation on non-socket";
	case WSAENOPROTOOPT:
		return "Protocol not available";
	case WSAESHUTDOWN:
		return "Can't send after socket shutdown";
	case WSAECONNREFUSED:
		return "Connection refused";
	case WSAEADDRINUSE:
		return "Address already in use";
	case WSAECONNABORTED:
		return "Connection aborted";
	case WSAENETUNREACH:
		return "Network is unreachable";
	case WSAENETDOWN:
		return "Network interface is not configured";
	case WSAETIMEDOUT:
		return "Connection timed out";
	case WSAEHOSTDOWN:
		return "Host is down";
	case WSAEHOSTUNREACH:
		return "Host is unreachable";
	case WSAEINPROGRESS:
		return "Connection already in progress";
	case WSAEALREADY:
		return "Socket already connected";
	case WSAEPROTONOSUPPORT:
		return "Unknown protocol";
	case WSAESOCKTNOSUPPORT:
		return "Socket type not supported";
	case WSAEADDRNOTAVAIL:
		return "Address not available";
	case WSAEISCONN:
		return "Socket is already connected";
	case WSAENOTCONN:
		return "Socket is not connected";
	}
	return "Unknown Windows Sockets error";
}
#endif

