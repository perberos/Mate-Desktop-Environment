/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-utils.h - Public utility functions for the MATE Virtual
   File System.

   Copyright (C) 1999 Free Software Foundation
   Copyright (C) 2000 Eazel, Inc.

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
*/

#ifndef MATE_VFS_UTILS_H
#define MATE_VFS_UTILS_H

#include <glib.h>
#include <libmatevfs/mate-vfs-file-size.h>
#include <libmatevfs/mate-vfs-result.h>
#include <libmatevfs/mate-vfs-uri.h>
#include <libmatevfs/mate-vfs-handle.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * MateVFSMakeURIDirs:
 * @MATE_VFS_MAKE_URI_DIR_NONE: Don't check any directory
 * @MATE_VFS_MAKE_URI_DIR_HOMEDIR: Check the home directory
 * @MATE_VFS_MAKE_URI_DIR_CURRENT: Check the current direcotry
 *
 * Flags that can be passed to mate_vfs_make_uri_from_input_with_dirs().
 * If the given input might be a relative path it checks for existence of the file
 * in the directory specified by this flag.
 * If both flags are passed the current directory is checked first.
 *
 **/

typedef enum {
	MATE_VFS_MAKE_URI_DIR_NONE = 0,
	MATE_VFS_MAKE_URI_DIR_HOMEDIR = 1 << 0,
	MATE_VFS_MAKE_URI_DIR_CURRENT = 1 << 1
} MateVFSMakeURIDirs;

/* Makes a human-readable string. */
char *mate_vfs_format_file_size_for_display (MateVFSFileSize  size);

/* Converts unsafe characters to % sequences so the string can be
 * used as a piece of a URI. Escapes all reserved URI characters.
 */
char *mate_vfs_escape_string                (const char      *string);

/* Converts unsafe characters to % sequences so the path can be
 * used as a piece of a URI. Escapes all reserved URI characters
 * except for "/".
 */
char *mate_vfs_escape_path_string           (const char      *path);

/* Converts unsafe characters to % sequences so the host/path segment
 * can be used as a piece of a URI.  Allows ":" and "@" in the host
 * section (everything up to the first "/"), and after that, it behaves
 * like mate_vfs_escape_path_string.
 */
char *mate_vfs_escape_host_and_path_string  (const char      *path);

/* Converts only slashes and % characters to % sequences. This is useful
 * for code that wants to use an arbitrary string as a file name. To use
 * it in a URI, you have to escape again, of course.
 */
char *mate_vfs_escape_slashes               (const char      *string);


/* Escapes all the characters that match any of the @match_set */
char *mate_vfs_escape_set 		     (const char      *string,
	              			      const char      *match_set);

/* Returns NULL if any of the illegal character appear in escaped
 * form. If the illegal characters are in there unescaped, that's OK.
 * Typically you pass "/" for illegal characters when converting to a
 * Unix path, since pieces of Unix paths can't contain "/". ASCII 0
 * is always illegal due to the limitations of NULL-terminated strings.
 */
char *mate_vfs_unescape_string              (const char      *escaped_string,
					      const char      *illegal_characters);

/* returns a copy of uri, converted to a canonical form */
char *mate_vfs_make_uri_canonical	     (const char 	*uri);

/* returns a copy of path, converted to a canonical form */
char *mate_vfs_make_path_name_canonical     (const char      *path);

/* returns a copy of path, with initial ~ expanded, or just copy of path
 * if there's no initial ~
 */
char *mate_vfs_expand_initial_tilde	     (const char      *path);

/* Prepare an escaped string for display. Unlike mate_vfs_unescape_string,
 * this doesn't return NULL if an illegal sequences appears in the string,
 * instead doing its best to provide a useful result.
 */
char *mate_vfs_unescape_string_for_display  (const char      *escaped);

/* Turn a "file://" URI in string form into a local path. Returns NULL
 * if it's not a URI that can be converted.
 */
char *mate_vfs_get_local_path_from_uri      (const char      *uri);

/* Turn a path into a "file://" URI. */
char *mate_vfs_get_uri_from_local_path      (const char      *local_full_path);

/* Check whether a string starts with an executable command */
gboolean mate_vfs_is_executable_command_string (const char *command_string);

/* Free the list, freeing each item data with a g_free */
void   mate_vfs_list_deep_free               (GList            *list);


/* Return amount of free space on target */
MateVFSResult	mate_vfs_get_volume_free_space	(const MateVFSURI 	*vfs_uri,
						 MateVFSFileSize 	*size);

char *mate_vfs_icon_path_from_filename       (const char *filename);

/* Convert a file descriptor to a handle */
MateVFSResult	mate_vfs_open_fd	(MateVFSHandle	**handle,
					 int filedes);

/* TRUE if the current thread is the thread with the main glib event loop */
gboolean	mate_vfs_is_primary_thread (void);

/**
 * MATE_VFS_ASSERT_PRIMARY_THREAD:
 *
 * Asserts that the current thread is the thread with
 * the main glib event loop
 **/
#define MATE_VFS_ASSERT_PRIMARY_THREAD g_assert (mate_vfs_is_primary_thread())

/**
 * MATE_VFS_ASSERT_SECONDARY_THREAD:
 *
 * Asserts that the current thread is NOT the thread with
 * the main glib event loop
 **/
#define MATE_VFS_ASSERT_SECONDARY_THREAD g_assert (!mate_vfs_is_primary_thread())

/* Reads the contents of an entire file into memory */
MateVFSResult  mate_vfs_read_entire_file (const char *uri,
					    int *file_size,
					    char **file_contents);

char *   mate_vfs_format_uri_for_display            (const char          *uri);
char *   mate_vfs_make_uri_from_input               (const char     *location);
char *   mate_vfs_make_uri_from_input_with_trailing_ws
                                                     (const char     *location);
char *   mate_vfs_make_uri_from_input_with_dirs     (const char     *location,
						      MateVFSMakeURIDirs  dirs);
char *   mate_vfs_make_uri_canonical_strip_fragment (const char          *uri);
gboolean mate_vfs_uris_match                        (const char          *uri_1,
						      const char          *uri_2);
char *   mate_vfs_get_uri_scheme                    (const char          *uri);
char *   mate_vfs_make_uri_from_shell_arg           (const char          *uri);


#ifndef MATE_VFS_DISABLE_DEPRECATED
char * mate_vfs_make_uri_full_from_relative (const char *base_uri,
					      const char *relative_uri);
#endif /* MATE_VFS_DISABLE_DEPRECATED */

MateVFSResult mate_vfs_url_show                        (const char   *url);
MateVFSResult mate_vfs_url_show_with_env               (const char   *url,
                                                          char        **envp);


#ifdef __cplusplus
}
#endif

#endif /* MATE_VFS_UTILS_H */
