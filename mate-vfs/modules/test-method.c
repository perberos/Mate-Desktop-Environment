/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* Test-method.c: Mate-VFS testing method

   Copyright (C) 2000 Eazel

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

   Authors: Seth Nickell      (seth@eazel.com) */

/* Create a settings file and put it at PREFIX/etc/vfs/Test-conf.xml,
 * then point mate-vfs clients to test:<uri_minus_scheme> which will
 * translate into the "real" method (e.g. test:///home/seth).
 *
 * The delay is in milliseconds.
 *
 * Here's a sample config file (pointing to the file method):
 *
 *    <?xml version="1.0"?>
 *        <TestModule method="file">
 *	      <function name="open_directory" delay="2000"/>
 *        </TestModule>
 * */

#include <config.h>

#include <glib/gi18n-lib.h>
#include <glib.h>
#include <libmatevfs/mate-vfs-cancellable-ops.h>
#include <libmatevfs/mate-vfs-ops.h>
#include <libmatevfs/mate-vfs-module.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define TEST_CONF_ENV_VARIABLE "MATE_VFS_TEST_CONFIG_FILE"

typedef struct {
	char *operation_name;
        int delay;
	gboolean skip;
	gboolean override_result;
	MateVFSResult overridden_result_value;
} OperationSettings;

#define NUM_RESULT_STRINGS 41

static gboolean properly_initialized;

static guchar *test_method_name;
static GList *settings_list;

static const char * const
result_strings[NUM_RESULT_STRINGS] = {
	"MATE_VFS_OK",
	"MATE_VFS_ERROR_NOT_FOUND",
	"MATE_VFS_ERROR_GENERIC",
	"MATE_VFS_ERROR_INTERNAL",
	"MATE_VFS_ERROR_BAD_PARAMETERS",
	"MATE_VFS_ERROR_NOT_SUPPORTED",
	"MATE_VFS_ERROR_IO",
	"MATE_VFS_ERROR_CORRUPTED_DATA",
	"MATE_VFS_ERROR_WRONG_FORMAT",
	"MATE_VFS_ERROR_BAD_FILE",
	"MATE_VFS_ERROR_TOO_BIG",
	"MATE_VFS_ERROR_NO_SPACE",
	"MATE_VFS_ERROR_READ_ONLY",
	"MATE_VFS_ERROR_INVALID_URI",
	"MATE_VFS_ERROR_NOT_OPEN",
	"MATE_VFS_ERROR_INVALID_OPEN_MODE",
	"MATE_VFS_ERROR_ACCESS_DENIED",
	"MATE_VFS_ERROR_TOO_MANY_OPEN_FILES",
	"MATE_VFS_ERROR_EOF",
	"MATE_VFS_ERROR_NOT_A_DIRECTORY",
	"MATE_VFS_ERROR_IN_PROGRESS",
	"MATE_VFS_ERROR_INTERRUPTED",
	"MATE_VFS_ERROR_FILE_EXISTS",
	"MATE_VFS_ERROR_LOOP",
	"MATE_VFS_ERROR_NOT_PERMITTED",
	"MATE_VFS_ERROR_IS_DIRECTORY",
	"MATE_VFS_ERROR_NO_MEMORY",
	"MATE_VFS_ERROR_HOST_NOT_FOUND",
	"MATE_VFS_ERROR_INVALID_HOST_NAME",
	"MATE_VFS_ERROR_HOST_HAS_NO_ADDRESS",
	"MATE_VFS_ERROR_LOGIN_FAILED",
	"MATE_VFS_ERROR_CANCELLED",
	"MATE_VFS_ERROR_DIRECTORY_BUSY",
	"MATE_VFS_ERROR_DIRECTORY_NOT_EMPTY",
	"MATE_VFS_ERROR_TOO_MANY_LINKS",
	"MATE_VFS_ERROR_READ_ONLY_FILE_SYSTEM",
	"MATE_VFS_ERROR_NOT_SAME_FILE_SYSTEM",
	"MATE_VFS_ERROR_NAME_TOO_LONG",
	"MATE_VFS_ERROR_SERVICE_NOT_AVAILABLE",
	"MATE_VFS_NUM_ERRORS"
};

/* Module entry points. */
MateVFSMethod *vfs_module_init     (const char     *method_name,
				        const char     *args);
void            vfs_module_shutdown  (MateVFSMethod *method);

static MateVFSURI *
translate_uri (MateVFSURI *uri)
{
	MateVFSURI *translated_uri;
	char *uri_text;
	char *translated_uri_text;
	char *no_method;

	uri_text = mate_vfs_uri_to_string (uri, MATE_VFS_URI_HIDE_NONE);
	no_method = strchr (uri_text, ':');
	
	if (test_method_name != NULL) {
	  translated_uri_text = g_strconcat ((char *)test_method_name, 
					     no_method, NULL);
	} else {
	  translated_uri_text = NULL;
	}

	if (translated_uri_text != NULL) {
	  translated_uri = mate_vfs_uri_new (translated_uri_text);
	} else {
	  translated_uri = NULL;
	}

	g_free (translated_uri_text);
	g_free (uri_text);

	return translated_uri;
}

/* reads the configuration file and returns TRUE if there are special options for
   this execution of the operation.

   if TRUE is returned then result will contain the result the operation should return
   and perform_operation will be TRUE if the operation should execute the underlying
   operation anyway
*/
static const OperationSettings *
get_operation_settings (const char *function_identifier)
{
	static OperationSettings empty_settings;
        GList *node;
	OperationSettings *settings;

	for (node = settings_list; node != NULL; node = node->next) {
	        settings = node->data;
		if (g_ascii_strcasecmp (settings->operation_name, function_identifier) == 0) {
			return settings;
		}
	}
	
	return &empty_settings;
}

static const OperationSettings *
start_operation (const char *name,
		 MateVFSURI **uri,
		 MateVFSURI **saved_uri)
{
	const OperationSettings *settings;

	settings = get_operation_settings (name);

	g_usleep (settings->delay * 1000);
	
	if (uri != NULL) {
		*saved_uri = *uri;
		*uri = translate_uri (*uri);
	}
	return settings;
}

static MateVFSResult
finish_operation (const OperationSettings *settings,
		  MateVFSResult result,
		  MateVFSURI **uri,
		  MateVFSURI **saved_uri)
{
	if (uri != NULL) {
		mate_vfs_uri_unref (*uri);
		*uri = *saved_uri;
	}

	if (settings->override_result) {
		return settings->overridden_result_value;
	}
	return result;
}

#define PERFORM_OPERATION(name, operation)			\
do {								\
	const OperationSettings *settings;			\
	MateVFSURI *saved_uri;					\
	MateVFSResult result;					\
								\
        if (!properly_initialized) {                            \
                return MATE_VFS_ERROR_SERVICE_NOT_AVAILABLE;   \
        }                                                       \
                                                                \
	settings = start_operation (#name, &uri, &saved_uri);	\
	if (settings->skip) {					\
		result = MATE_VFS_OK;				\
	} else {						\
		result = operation;				\
	}							\
	return finish_operation (settings, result,		\
				 &uri, &saved_uri);		\
} while (0)


#define PERFORM_OPERATION_NO_URI(name, operation)		\
do {								\
	const OperationSettings *settings;			\
	MateVFSResult result;					\
								\
        if (!properly_initialized) {                            \
                return MATE_VFS_ERROR_SERVICE_NOT_AVAILABLE;   \
        }                                                       \
                                                                \
	settings = start_operation (#name, NULL, NULL);		\
	if (settings->skip) {					\
		result = MATE_VFS_OK;				\
	} else {						\
		result = operation;				\
	}							\
	return finish_operation (settings, result,		\
				 NULL, NULL);			\
} while (0)

static gboolean
parse_result_text (const char *result_text,
		   MateVFSResult *result_code)
{
	int i;

	for (i = 0; i < NUM_RESULT_STRINGS; i++) {
		if (g_ascii_strcasecmp (result_text, result_strings[i]) == 0) {
			*result_code = i;
			return TRUE;
		}
	}
	
	return FALSE;
}

static gboolean
load_settings (const char *filename) 
{
	xmlDocPtr doc;
	xmlNodePtr node;
	char *name;
	OperationSettings *operation;
	char *str;

	doc = xmlParseFile (filename); 

	if (doc == NULL
	    || doc->xmlRootNode == NULL
	    || doc->xmlRootNode->name == NULL
	    || g_ascii_strcasecmp ((char *)doc->xmlRootNode->name, "testmodule") != 0) {
		xmlFreeDoc(doc);
		return FALSE;
	}

	test_method_name = xmlGetProp (doc->xmlRootNode, (guchar *)"method");
	
	for (node = doc->xmlRootNode->xmlChildrenNode; node != NULL; node = node->next) {
		name = (char *)xmlGetProp (node, (guchar *)"name");
		if (name == NULL) {
			continue;
		}

		operation = g_new0 (OperationSettings, 1);
		operation->operation_name = name;

		str = (char *)xmlGetProp (node, (guchar *)"delay");
		if (str != NULL) {
			sscanf (str, "%d", &operation->delay);
		}
		xmlFree (str);

		str = (char *)xmlGetProp(node, (guchar *)"execute_operation");
		if (str != NULL && g_ascii_strcasecmp (str, "FALSE") == 0) {
			operation->skip = TRUE;
		}
		xmlFree (str);

		str = (char *)xmlGetProp (node, (guchar *)"result");
		if (str != NULL) {
			operation->override_result = parse_result_text
				(str, &operation->overridden_result_value);
		}
		xmlFree (str);
		
		settings_list = g_list_prepend (settings_list, operation);
	}
	return TRUE;
}

static MateVFSResult
do_open (MateVFSMethod *method,
	 MateVFSMethodHandle **method_handle,
	 MateVFSURI *uri,
	 MateVFSOpenMode mode,
	 MateVFSContext *context)
{	
	PERFORM_OPERATION (open, mate_vfs_open_uri_cancellable ((MateVFSHandle **) method_handle, uri, mode, context));
}

static MateVFSResult
do_create (MateVFSMethod *method,
	   MateVFSMethodHandle **method_handle,
	   MateVFSURI *uri,
	   MateVFSOpenMode mode,
	   gboolean exclusive,
	   guint perm,
	   MateVFSContext *context)
{
	/* FIXME bugzilla.eazel.com 3837: Not implemented. */
	return MATE_VFS_ERROR_INTERNAL;
}

static MateVFSResult
do_close (MateVFSMethod *method,
	  MateVFSMethodHandle *method_handle,
	  MateVFSContext *context)
{
	PERFORM_OPERATION_NO_URI (close, mate_vfs_close_cancellable ((MateVFSHandle *) method_handle, context));
}

static MateVFSResult
do_read (MateVFSMethod *method,
	 MateVFSMethodHandle *method_handle,
	 gpointer buffer,
	 MateVFSFileSize num_bytes,
	 MateVFSFileSize *bytes_read,
	 MateVFSContext *context)
{
	PERFORM_OPERATION_NO_URI (read, mate_vfs_read_cancellable ((MateVFSHandle *) method_handle, buffer, num_bytes, bytes_read, context));
}

static MateVFSResult
do_write (MateVFSMethod *method,
	  MateVFSMethodHandle *method_handle,
	  gconstpointer buffer,
	  MateVFSFileSize num_bytes,
	  MateVFSFileSize *bytes_written,
	  MateVFSContext *context)
{
	PERFORM_OPERATION_NO_URI (write, mate_vfs_write_cancellable((MateVFSHandle *) method_handle, buffer, num_bytes, bytes_written, context));
}

static MateVFSResult
do_seek (MateVFSMethod *method,
	 MateVFSMethodHandle *method_handle,
	 MateVFSSeekPosition whence,
	 MateVFSFileOffset offset,
	 MateVFSContext *context)
{
	PERFORM_OPERATION_NO_URI (seek, mate_vfs_seek_cancellable ((MateVFSHandle *) method_handle, whence, offset, context));
}

static MateVFSResult
do_tell (MateVFSMethod *method,
	 MateVFSMethodHandle *method_handle,
	 MateVFSFileSize *offset_return)
{
	PERFORM_OPERATION_NO_URI (tell, mate_vfs_tell ((MateVFSHandle *) method_handle, offset_return));
}


static MateVFSResult
do_open_directory (MateVFSMethod *method,
		   MateVFSMethodHandle **method_handle,
		   MateVFSURI *uri,
		   MateVFSFileInfoOptions options,
		   MateVFSContext *context)
{
	PERFORM_OPERATION (open_directory, mate_vfs_directory_open_from_uri ((MateVFSDirectoryHandle **) method_handle, uri, options));
}

static MateVFSResult
do_close_directory (MateVFSMethod *method,
		    MateVFSMethodHandle *method_handle,
		    MateVFSContext *context)
{
	PERFORM_OPERATION_NO_URI (close_directory, mate_vfs_directory_close ((MateVFSDirectoryHandle *) method_handle));
}

static MateVFSResult
do_read_directory (MateVFSMethod *method,
		   MateVFSMethodHandle *method_handle,
		   MateVFSFileInfo *file_info,
		   MateVFSContext *context)
{
	PERFORM_OPERATION_NO_URI (read_directory, mate_vfs_directory_read_next ((MateVFSDirectoryHandle *) method_handle, file_info));
}

static MateVFSResult
do_get_file_info (MateVFSMethod *method,
		  MateVFSURI *uri,
		  MateVFSFileInfo *file_info,
		  MateVFSFileInfoOptions options,
		  MateVFSContext *context)
{
	PERFORM_OPERATION (get_file_info, mate_vfs_get_file_info_uri_cancellable (uri, file_info, options, context));
}

static MateVFSResult
do_get_file_info_from_handle (MateVFSMethod *method,
			      MateVFSMethodHandle *method_handle,
			      MateVFSFileInfo *file_info,
			      MateVFSFileInfoOptions options,
			      MateVFSContext *context)
{
	PERFORM_OPERATION_NO_URI (get_file_info_from_handle, mate_vfs_get_file_info_from_handle_cancellable ((MateVFSHandle *) method_handle, file_info, options, context));
}

static gboolean
do_is_local (MateVFSMethod *method,
	     const MateVFSURI *uri)
{
	/* FIXME bugzilla.eazel.com 3837: Not implemented. */
	return TRUE;
}

static MateVFSResult
do_make_directory (MateVFSMethod *method,
		   MateVFSURI *uri,
		   guint perm,
		   MateVFSContext *context)
{
	PERFORM_OPERATION (make_directory, mate_vfs_make_directory_for_uri_cancellable (uri, perm, context));
}

static MateVFSResult
do_remove_directory (MateVFSMethod *method,
		     MateVFSURI *uri,
		     MateVFSContext *context)
{
	PERFORM_OPERATION (remove_directory, mate_vfs_remove_directory_from_uri_cancellable (uri, context));
}

static MateVFSResult
do_move (MateVFSMethod *method,
	 MateVFSURI *old_uri,
	 MateVFSURI *new_uri,
	 gboolean force_replace,
	 MateVFSContext *context)
{
	/* FIXME bugzilla.eazel.com 3837: Not implemented. */
	return mate_vfs_move_uri_cancellable (old_uri, new_uri, force_replace, context);
}

static MateVFSResult
do_unlink (MateVFSMethod *method,
	   MateVFSURI *uri,
	   MateVFSContext *context)
{
	PERFORM_OPERATION (unlink, mate_vfs_unlink_from_uri_cancellable (uri, context));
}

static MateVFSResult
do_check_same_fs (MateVFSMethod *method,
		  MateVFSURI *a,
		  MateVFSURI *b,
		  gboolean *same_fs_return,
		  MateVFSContext *context)
{
	/* FIXME bugzilla.eazel.com 3837: Not implemented. */
	return mate_vfs_check_same_fs_uris_cancellable (a, b, same_fs_return, context);
}

static MateVFSResult
do_set_file_info (MateVFSMethod *method,
		  MateVFSURI *uri,
		  const MateVFSFileInfo *info,
		  MateVFSSetFileInfoMask mask,
		  MateVFSContext *context)
{	
	PERFORM_OPERATION (set_file_info, mate_vfs_set_file_info_cancellable (uri, info, mask, context));
}

static MateVFSResult
do_truncate (MateVFSMethod *method,
	     MateVFSURI *uri,
	     MateVFSFileSize where,
	     MateVFSContext *context)
{
	PERFORM_OPERATION (truncate, mate_vfs_truncate_uri_cancellable (uri, where, context));
}

static MateVFSResult
do_truncate_handle (MateVFSMethod *method,
		    MateVFSMethodHandle *method_handle,
		    MateVFSFileSize where,
		    MateVFSContext *context)
{
	PERFORM_OPERATION_NO_URI (truncate_handle, mate_vfs_truncate_handle_cancellable ((MateVFSHandle *) method_handle, where, context));
}

static MateVFSResult
do_find_directory (MateVFSMethod *method,
		   MateVFSURI *uri,
		   MateVFSFindDirectoryKind kind,
		   MateVFSURI **result_uri,
		   gboolean create_if_needed,
		   gboolean find_if_needed,
		   guint permissions,
		   MateVFSContext *context)
{
	PERFORM_OPERATION (find_directory, mate_vfs_find_directory_cancellable (uri, kind, result_uri, create_if_needed, find_if_needed, permissions, context));
}

static MateVFSResult
do_create_symbolic_link (MateVFSMethod *method,
			 MateVFSURI *uri,
			 const char *target_reference,
			 MateVFSContext *context)
{
	PERFORM_OPERATION (create_symbolic_link, mate_vfs_create_symbolic_link_cancellable (uri, target_reference, context));
}

static MateVFSMethod method = {
	sizeof (MateVFSMethod),
	do_open,
	do_create,
	do_close,
	do_read,
	do_write,
	do_seek,
	do_tell,
	do_truncate_handle,
	do_open_directory,
	do_close_directory,
	do_read_directory,
	do_get_file_info,
	do_get_file_info_from_handle,
	do_is_local,
	do_make_directory,
	do_remove_directory,
	do_move,
	do_unlink,
	do_check_same_fs,
        do_set_file_info,
	do_truncate,
	do_find_directory,
	do_create_symbolic_link
};


MateVFSMethod *
vfs_module_init (const char *method_name, const char *args)
{
	char *conf_file;

	LIBXML_TEST_VERSION
	
	conf_file = getenv (TEST_CONF_ENV_VARIABLE);

	if (conf_file == NULL) {
		conf_file = MATE_VFS_PREFIX "/etc/vfs/Test-conf.xml";
	}

	if (load_settings (conf_file) == FALSE) {

	  /* FIXME: we probably shouldn't use printf to output the message */
	  printf (_("Didn't find a valid settings file at %s\n"), 
		  conf_file);
	  printf (_("Use the %s environment variable to specify a different location.\n"),
		  TEST_CONF_ENV_VARIABLE);
	  properly_initialized = FALSE;
	} else {
	  properly_initialized = TRUE;
	}

	return &method;
}

void
vfs_module_shutdown (MateVFSMethod *method)
{
	GList *node;
	OperationSettings *settings;

	for (node = settings_list; node != NULL; node = node->next) {
	        settings = node->data;
		xmlFree (settings->operation_name);
		g_free (settings);
	}
	g_list_free (settings_list);
	xmlFree (test_method_name);
}
