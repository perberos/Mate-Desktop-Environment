/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-uri.h - URI handling for the MATE Virtual File System.

   Copyright (C) 1999 Free Software Foundation

   The Mate Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Ettore Perazzoli <ettore@comm2000.it>
*/

#ifndef MATE_VFS_URI_H
#define MATE_VFS_URI_H

#include <glib.h>
#include <glib-object.h> /* For GType */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * MateVFSURI:
 * @ref_count: Reference count. The URI is freed when it drops to zero.
 * @text: A canonical representation of the path associated with this resource.
 * @fragment_id: Extra data identifying this resource.
 * @method_string: The @method's method associated with this resource.
 * One #MateVFSMethod can be used for multiple method strings.
 * @method: The #MateVFSMethod associated with this resource.
 * @parent: Pointer to the parent element, or %NULL for #MateVFSURI that
 * have no enclosing #MateVFSURI. The process of encapsulating one
 * URI in another one is called URI chaining.
 *
 * Holds information about the location of a particular resource.
 **/
typedef struct MateVFSURI {
	/*< public >*/
	guint ref_count;

	gchar *text;
	gchar *fragment_id;

	gchar *method_string;
	struct MateVFSMethod *method;

	struct MateVFSURI *parent;

	/*< private >*/
	/* Reserved to avoid future breaks in ABI compatibility */
	void *reserved1;
	void *reserved2;
} MateVFSURI;

/**
 * MateVFSToplevelURI:
 * @host_name: The name of the host used to access this resource, o %NULL.
 * @host_port: The port used to access this resource, or %0.
 * @user_name: Unescaped user name used to access this resource, or %NULL.
 * @password: Unescaped password used to access this resource, or %NULL.
 * @urn: The parent URN, or %NULL if it doesn't exist.
 *
 * This is the toplevel URI element used to access ressources stored on
 * a remote server. Toplevel method implementations should cast the #MateVFSURI
 * argument to this type to get the additional host and authentication information.
 *
 * If any of the elements is 0 respectively %NULL, it is unspecified.
 **/
typedef struct {
	MateVFSURI uri;

	/*< public >*/
	gchar *host_name;
	guint host_port;

	gchar *user_name;
	gchar *password;

	gchar *urn;

	/*< private >*/
	/* Reserved to avoid future breaks in ABI compatibility */
	void *reserved1;
	void *reserved2;
} MateVFSToplevelURI;


/**
 * MateVFSURIHideOptions:
 * @MATE_VFS_URI_HIDE_NONE: don't hide anything
 * @MATE_VFS_URI_HIDE_USER_NAME: hide the user name
 * @MATE_VFS_URI_HIDE_PASSWORD: hide the password
 * @MATE_VFS_URI_HIDE_HOST_NAME: hide the host name
 * @MATE_VFS_URI_HIDE_HOST_PORT: hide the port
 * @MATE_VFS_URI_HIDE_TOPLEVEL_METHOD: hide the method (e.g. http, file)
 * @MATE_VFS_URI_HIDE_FRAGMENT_IDENTIFIER: hide the fragment identifier
 *
 * Packed boolean bitfield controlling hiding of various elements
 * of a #MateVFSURI when it is converted to a string using
 * mate_vfs_uri_to_string().
 **/
typedef enum
{
	MATE_VFS_URI_HIDE_NONE = 0,
	MATE_VFS_URI_HIDE_USER_NAME = 1 << 0,
	MATE_VFS_URI_HIDE_PASSWORD = 1 << 1,
	MATE_VFS_URI_HIDE_HOST_NAME = 1 << 2,
	MATE_VFS_URI_HIDE_HOST_PORT = 1 << 3,
	MATE_VFS_URI_HIDE_TOPLEVEL_METHOD = 1 << 4,
	MATE_VFS_URI_HIDE_FRAGMENT_IDENTIFIER = 1 << 8
} MateVFSURIHideOptions;

GType mate_vfs_uri_hide_options_get_type (void);
#define MATE_VFS_TYPE_VFS_URI_HIDE_OPTIONS (mate_vfs_uri_hide_options_get_type())


/**
 * MATE_VFS_URI_MAGIC_CHR:
 *
 * The character used to divide location from
 * extra "arguments" passed to the method.
 **/
/**
 * MATE_VFS_URI_MAGIC_STR:
 *
 * The character used to divide location from
 * extra "arguments" passed to the method.
 **/
#define MATE_VFS_URI_MAGIC_CHR	'#'
#define MATE_VFS_URI_MAGIC_STR "#"

/**
 * MATE_VFS_URI_PATH_CHR:
 *
 * Defines the path seperator character.
 **/
/**
 * MATE_VFS_URI_PATH_STR:
 *
 * Defines the path seperator string.
 **/
#define MATE_VFS_URI_PATH_CHR '/'
#define MATE_VFS_URI_PATH_STR "/"

/* FUNCTIONS */
MateVFSURI 	     *mate_vfs_uri_new                   (const gchar *text_uri);
MateVFSURI 	     *mate_vfs_uri_resolve_relative      (const MateVFSURI *base,
							   const gchar *relative_reference);
MateVFSURI 	     *mate_vfs_uri_resolve_symbolic_link (const MateVFSURI *base,
							   const gchar *relative_reference);
MateVFSURI 	     *mate_vfs_uri_ref                   (MateVFSURI *uri);
void        	      mate_vfs_uri_unref                 (MateVFSURI *uri);

MateVFSURI          *mate_vfs_uri_append_string         (const MateVFSURI *uri,
						           const char *uri_fragment);
MateVFSURI          *mate_vfs_uri_append_path           (const MateVFSURI *uri,
						           const char *path);
MateVFSURI          *mate_vfs_uri_append_file_name      (const MateVFSURI *uri,
						           const gchar *filename);
gchar       	     *mate_vfs_uri_to_string             (const MateVFSURI *uri,
						           MateVFSURIHideOptions hide_options);
MateVFSURI 	     *mate_vfs_uri_dup                   (const MateVFSURI *uri);
gboolean    	      mate_vfs_uri_is_local              (const MateVFSURI *uri);
gboolean	      mate_vfs_uri_has_parent	          (const MateVFSURI *uri);
MateVFSURI	     *mate_vfs_uri_get_parent            (const MateVFSURI *uri);

MateVFSToplevelURI *mate_vfs_uri_get_toplevel           (const MateVFSURI *uri);

const gchar 	    *mate_vfs_uri_get_host_name          (const MateVFSURI *uri);
const gchar         *mate_vfs_uri_get_scheme             (const MateVFSURI *uri);
guint 	    	     mate_vfs_uri_get_host_port          (const MateVFSURI *uri);
const gchar 	    *mate_vfs_uri_get_user_name          (const MateVFSURI *uri);
const gchar	    *mate_vfs_uri_get_password           (const MateVFSURI *uri);

void		     mate_vfs_uri_set_host_name          (MateVFSURI *uri,
						           const gchar *host_name);
void 	    	     mate_vfs_uri_set_host_port          (MateVFSURI *uri,
						           guint host_port);
void		     mate_vfs_uri_set_user_name          (MateVFSURI *uri,
						           const gchar *user_name);
void		     mate_vfs_uri_set_password           (MateVFSURI *uri,
						           const gchar *password);

gboolean	     mate_vfs_uri_equal	          (const MateVFSURI *a,
						           const MateVFSURI *b);

gboolean	     mate_vfs_uri_is_parent	          (const MateVFSURI *possible_parent,
						           const MateVFSURI *possible_child,
						           gboolean recursive);

const gchar 	    *mate_vfs_uri_get_path                (const MateVFSURI *uri);
const gchar 	    *mate_vfs_uri_get_fragment_identifier (const MateVFSURI *uri);
gchar 		    *mate_vfs_uri_extract_dirname         (const MateVFSURI *uri);
gchar		    *mate_vfs_uri_extract_short_name      (const MateVFSURI *uri);
gchar		    *mate_vfs_uri_extract_short_path_name (const MateVFSURI *uri);

gint		     mate_vfs_uri_hequal 	           (gconstpointer a,
						            gconstpointer b);
guint		     mate_vfs_uri_hash		           (gconstpointer p);

GList               *mate_vfs_uri_list_parse              (const gchar* uri_list);
GList               *mate_vfs_uri_list_ref                (GList *list);
GList               *mate_vfs_uri_list_unref              (GList *list);
GList               *mate_vfs_uri_list_copy               (GList *list);
void                 mate_vfs_uri_list_free               (GList *list);

char                *mate_vfs_uri_make_full_from_relative (const char *base_uri,
							    const char *relative_uri);


#ifdef __cplusplus
}
#endif

#endif /* MATE_VFS_URI_H */
