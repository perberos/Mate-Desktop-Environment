/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * Copyright (C) 1998 Miguel de Icaza
 * Copyright (C) 1997 Paolo Molaro
 * Copyright (C) 2000, 2001 Eazel, Inc.
 * All rights reserved.
 *
 * This file is part of the Mate Library.
 *
 * The Mate Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Mate Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Mate Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include "mate-vfs-mime.h"
#include "xdgmime.h"

#include "mate-vfs-mime-private.h"
#include "mate-vfs-mime-sniff-buffer-private.h"
#include "mate-vfs-mime-utils.h"
#include "mate-vfs-mime-info.h"
#include "mate-vfs-module-shared.h"
#include "mate-vfs-ops.h"
#include "mate-vfs-result.h"
#include "mate-vfs-uri.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <glib/gstdio.h>

#define DEFAULT_DATE_TRACKER_INTERVAL	5	/* in milliseconds */

typedef struct {
	char *file_path;
	time_t mtime;
} FileDateRecord;

struct FileDateTracker {
	time_t last_checked;
	guint check_interval;
	GHashTable *records;
};


#ifdef G_THREADS_ENABLED

/* We lock this mutex whenever we modify global state in this module.  */
G_LOCK_DEFINE (mate_vfs_mime_mutex);

#endif /* G_LOCK_DEFINE_STATIC */


/**
 * mate_vfs_mime_shutdown:
 *
 * Deprecated: This function doesn't have to be called as the
 * operating system automatically cleans up resources when exiting.
 *
 * Unload the MIME database from memory.
 */

void
mate_vfs_mime_shutdown (void)
{
	G_LOCK (mate_vfs_mime_mutex);

	xdg_mime_shutdown ();

	G_UNLOCK (mate_vfs_mime_mutex);
}

/**
 * mate_vfs_mime_type_from_name_or_default:
 * @filename: a filename (the file does not necessarily exist).
 * @defaultv: a default value to be returned if no match is found.
 *
 * This routine tries to determine the mime-type of the filename
 * only by looking at the filename from the MATE database of mime-types.
 *
 * Returns: the mime-type of the @filename.  If no value could be
 * determined, it will return @defaultv.
 */
const char *
mate_vfs_mime_type_from_name_or_default (const char *filename, const char *defaultv)
{
	const char *mime_type;
	const char *separator;

	if (filename == NULL) {
		return defaultv;
	}

	separator = strrchr (filename, '/');
#ifdef G_OS_WIN32
	{
		const char *sep2 = strrchr (filename, '\\');
		if (separator == NULL ||
		    (sep2 != NULL && sep2 > separator))
			separator = sep2;
	}
#endif
	if (separator != NULL) {
		separator++;
		if (*separator == '\000')
			return defaultv;
	} else {
		separator = filename;
	}

	G_LOCK (mate_vfs_mime_mutex);
	mime_type = xdg_mime_get_mime_type_from_file_name (separator);
	G_UNLOCK (mate_vfs_mime_mutex);

	if (mime_type)
		return mime_type;
	else
		return defaultv;
}

/**
 * mate_vfs_get_mime_type_for_name:
 * @filename: a filename.
 *
 * Determine the mime type for @filename. The file @filename may
 * not exist, this function does not access the actual file.
 * If the mime-type cannot be determined, %MATE_VFS_MIME_TYPE_UNKNOWN
 * is returned.
 * 
 * Returns: the mime-type for this filename or
 * %MATE_VFS_MIME_TYPE_UNKNOWN if mime-type could not be determined.
 *
 * Since: 2.14
 */
const char *
mate_vfs_get_mime_type_for_name (const char *filename)
{
	return mate_vfs_mime_type_from_name_or_default (filename,
							 MATE_VFS_MIME_TYPE_UNKNOWN);
}

/**
 * mate_vfs_mime_type_from_name:
 * @filename: a filename (the file does not necessarily exist).
 *
 * Determine the mime type for @filename.
 *
 * Deprecated: This function is deprecated, use
 * mate_vfs_get_mime_type_for_name() instead.
 *
 * Returns: the mime-type for this filename. Will return
 * %MATE_VFS_MIME_TYPE_UNKNOWN if mime-type could not be found.
 */
const char *
mate_vfs_mime_type_from_name (const gchar *filename)
{
	return mate_vfs_get_mime_type_for_name (filename);
}

/**
 * mate_vfs_get_mime_type_for_name_and_data:
 * @filename: a filename.
 * @data: a pointer to the data in the memory
 * @data_size: the size of @data
 *
 * Determine the mime-type for @filename and @data. This function tries
 * to be smart (e.g. mime subclassing) about returning the right mime-type 
 * by looking at both the @data and the @filename. The file will not be 
 * accessed.
 * If the mime-type cannot be determined, %MATE_VFS_MIME_TYPE_UNKNOWN
 * is returned.
 * 
 * Returns: the mime-type for this filename or
 * %MATE_VFS_MIME_TYPE_UNKNOWN if mime-type could not be determined.
 *
 * Since: 2.14
 */
const char *
mate_vfs_get_mime_type_for_name_and_data (const char    *filename,
					   gconstpointer  data,
					   gssize         data_size)
{
	MateVFSMimeSniffBuffer *sniff_buffer;
	const char *mime_type;
	
	sniff_buffer = mate_vfs_mime_sniff_buffer_new_from_existing_data (data,
									   data_size);

	
	mime_type = _mate_vfs_get_mime_type_internal (sniff_buffer,
						       filename,
						       TRUE);

	mate_vfs_mime_sniff_buffer_free (sniff_buffer);

	return mime_type;
}

static const char *
mate_vfs_get_mime_type_from_uri_internal (MateVFSURI *uri)
{
	char *base_name;
	const char *mime_type;

	/* Return a mime type based on the file extension or NULL if no match. */
	base_name = mate_vfs_uri_extract_short_path_name (uri);
	if (base_name == NULL) {
		return NULL;
	}

	mime_type = mate_vfs_mime_type_from_name_or_default (base_name, NULL);
	g_free (base_name);
	return mime_type;
}

enum
{
	MAX_SNIFF_BUFFER_ALLOWED=4096
};
static const char *
_mate_vfs_read_mime_from_buffer (MateVFSMimeSniffBuffer *buffer)
{
	int max_extents;
	MateVFSResult result = MATE_VFS_OK;
	const char *mime_type;
	int prio;

	G_LOCK (mate_vfs_mime_mutex);
	max_extents = xdg_mime_get_max_buffer_extents ();
	G_UNLOCK (mate_vfs_mime_mutex);
	max_extents = CLAMP (max_extents, 0, MAX_SNIFF_BUFFER_ALLOWED);

	if (!buffer->read_whole_file) {
		result = _mate_vfs_mime_sniff_buffer_get (buffer, max_extents);
	}
	if (result != MATE_VFS_OK && result != MATE_VFS_ERROR_EOF) {
		return NULL;
	}
	G_LOCK (mate_vfs_mime_mutex);

	mime_type = xdg_mime_get_mime_type_for_data (buffer->buffer, buffer->buffer_length, &prio);

	G_UNLOCK (mate_vfs_mime_mutex);

	return mime_type;
	
}

const char *
_mate_vfs_get_mime_type_internal (MateVFSMimeSniffBuffer *buffer,
				   const char              *file_name,
				   gboolean                 use_suffix)
{
	const char *result;
	const char *fn_result;

	result = fn_result = NULL;

	/* Determine the mime type by filename as well, because
	 * we need this for the subclassing check later
	 */
	if (file_name != NULL) {
		fn_result = mate_vfs_mime_type_from_name_or_default (file_name, NULL);
	}

	if (buffer != NULL) {
		result = _mate_vfs_read_mime_from_buffer (buffer);
		
		if (result != NULL && result != XDG_MIME_TYPE_UNKNOWN) {
				
			if ((strcmp (result, "application/x-ole-storage") == 0) ||
			    (strcmp (result, "text/xml") == 0) ||
			    (strcmp (result, "application/x-bzip") == 0) ||
			    (strcmp (result, "application/x-gzip") == 0) ||
			    (strcmp (result, "application/zip") == 0)) {
				/* So many file types come compressed by gzip 
				 * that extensions are more reliable than magic
				 * typing. If the file has a suffix, then use 
				 * the type from the suffix.
		 		 */

				if (fn_result != NULL && fn_result != XDG_MIME_TYPE_UNKNOWN) {
					result = fn_result;
				}
				
			} else if (fn_result && fn_result != XDG_MIME_TYPE_UNKNOWN) {
				G_LOCK (mate_vfs_mime_mutex);
				if (xdg_mime_mime_type_subclass (fn_result, result)) {
					result = fn_result;
				}
				G_UNLOCK (mate_vfs_mime_mutex);
			}
			
			return result;
		}
		
		if (result == NULL || result == XDG_MIME_TYPE_UNKNOWN) {
			if (_mate_vfs_sniff_buffer_looks_like_text (buffer)) {
				/* Text file -- treat extensions as a more 
				 * accurate source of type information BUT _only_ if
				 * the extension is a subtype of text/plain.
				 */
				if ((fn_result != NULL) && (fn_result != XDG_MIME_TYPE_UNKNOWN)) {
					G_LOCK (mate_vfs_mime_mutex);
					if (xdg_mime_mime_type_subclass (fn_result, "text/plain")) {
						G_UNLOCK (mate_vfs_mime_mutex);
						return fn_result;
					}
					G_UNLOCK (mate_vfs_mime_mutex);
				}

				/* Didn't find an extension match, assume plain text. */
				return "text/plain";

			} else if (_mate_vfs_sniff_buffer_looks_like_mp3 (buffer)) {
				return "audio/mpeg";
			}
		}
	}

	if (use_suffix &&
	    (result == NULL || result == XDG_MIME_TYPE_UNKNOWN)) {
		/* No type recognized -- fall back on extensions. */
		result = fn_result;
	}
	
	if (result == NULL) {
		result = XDG_MIME_TYPE_UNKNOWN;
	}
	
	return result;
}

/**
 * mate_vfs_get_mime_type_common:
 * @uri: a real file or a non-existent uri.
 *
 * Tries to guess the mime type of the file represented by @uri.
 * Favors using the file data to the @uri extension.
 * Handles @uri of a non-existent file by falling back
 * on returning a type based on the extension. If cant find the mime-type based on the 
 * extension also then returns 'application/octet-stream'.
 *
 * FIXME: This function will not necessarily return the same mime type as doing a
 * get file info on the text uri.
 *
 * Returns: the mime-type for @uri.
 */
const char *
mate_vfs_get_mime_type_common (MateVFSURI *uri)
{
	const char *result;
	char *base_name;
	MateVFSMimeSniffBuffer *buffer;
	MateVFSHandle *handle;
	MateVFSResult error;

	/* Check for special stat-defined file types first. */
	result = mate_vfs_get_special_mime_type (uri);
	if (result != NULL) {
		return result;
	}

	error = mate_vfs_open_uri (&handle, uri, MATE_VFS_OPEN_READ);

	if (error != MATE_VFS_OK) {
		/* file may not exist, return type based on name only */
		return mate_vfs_get_mime_type_from_uri_internal (uri);
	}
	
	buffer = _mate_vfs_mime_sniff_buffer_new_from_handle (handle);

	base_name = mate_vfs_uri_extract_short_path_name (uri);

	result = _mate_vfs_get_mime_type_internal (buffer, base_name, TRUE);
	g_free (base_name);

	mate_vfs_mime_sniff_buffer_free (buffer);
	mate_vfs_close (handle);

	return result;
}

static MateVFSResult
file_seek_binder (gpointer context, MateVFSSeekPosition whence, 
		  MateVFSFileOffset offset)
{
	FILE *file = (FILE *)context;
	int result;
	result = fseek (file, offset, whence);
	if (result < 0) {
		return mate_vfs_result_from_errno ();
	}
	return MATE_VFS_OK;
}

static MateVFSResult
file_read_binder (gpointer context, gpointer buffer, 
		  MateVFSFileSize bytes, MateVFSFileSize *bytes_read)
{
	FILE *file = (FILE *)context;
	
	*bytes_read = fread (buffer, 1, bytes, file);
	
	/* short read, check eof */
	if (*bytes_read < bytes) {
	
		if (feof (file)) {
			return MATE_VFS_ERROR_EOF;
		} else {
			return mate_vfs_result_from_errno ();
		} 
	}

	return MATE_VFS_OK;
}

static const char *
mate_vfs_get_file_mime_type_internal (const char *path, const struct stat *optional_stat_info,
				       gboolean suffix_only, gboolean suffix_first)
{
	const char *result;
	MateVFSMimeSniffBuffer *buffer;
	struct stat tmp_stat_buffer;
	FILE *file;

	file = NULL;
	result = NULL;

	/* get the stat info if needed */
	if (optional_stat_info == NULL && g_stat (path, &tmp_stat_buffer) == 0) {
		optional_stat_info = &tmp_stat_buffer;
	}

	/* single out special file types */
	if (optional_stat_info && !S_ISREG(optional_stat_info->st_mode)) {
		if (S_ISDIR(optional_stat_info->st_mode)) {
			return "x-directory/normal";
		} else if (S_ISCHR(optional_stat_info->st_mode)) {
			return "x-special/device-char";
		} else if (S_ISBLK(optional_stat_info->st_mode)) {
			return "x-special/device-block";
		} else if (S_ISFIFO(optional_stat_info->st_mode)) {
			return "x-special/fifo";
#ifdef S_ISSOCK
		} else if (S_ISSOCK(optional_stat_info->st_mode)) {
			return "x-special/socket";
#endif
		} else {
			/* unknown entry type, return generic file type */
			return MATE_VFS_MIME_TYPE_UNKNOWN;
		}
	}

	if (suffix_first && !suffix_only) {
		result = _mate_vfs_get_mime_type_internal (NULL, path, TRUE);
		if (result != NULL &&
		    result != XDG_MIME_TYPE_UNKNOWN) {
			return result;
		}
	}
	
	if (!suffix_only) {
		file = g_fopen(path, "r");
	}

	if (file != NULL) {
		buffer = _mate_vfs_mime_sniff_buffer_new_generic
			(file_seek_binder, file_read_binder, file);

		result = _mate_vfs_get_mime_type_internal (buffer, path, !suffix_first);
		mate_vfs_mime_sniff_buffer_free (buffer);
		fclose (file);
	} else {
		result = _mate_vfs_get_mime_type_internal (NULL, path, !suffix_first);
	}

	
	g_assert (result != NULL);
	return result;
}


/**
 * mate_vfs_get_file_mime_type_fast:
 * @path: path of the file whose mime type is to be found out.
 * @optional_stat_info: optional stat buffer.
 *
 * Tries to guess the mime type of the file represented by @path.
 * It uses extention/name detection first, and if that fails
 * it falls back to mime-magic based lookup. This is faster
 * than always doing mime-magic but doesn't always produce
 * the right answer, so for important decisions 
 * you should use mate_vfs_get_file_mime_type().
 *
 * Returns: the mime-type for file at @path.
 */
const char *
mate_vfs_get_file_mime_type_fast (const char *path, const struct stat *optional_stat_info)
{
	return mate_vfs_get_file_mime_type_internal (path, optional_stat_info, FALSE, TRUE);
}


/**
 * mate_vfs_get_file_mime_type:
 * @path: path of a file whose mime type is to be found out.
 * @optional_stat_info: optional stat buffer.
 * @suffix_only: whether or not to do a magic-based lookup.
 *
 * Tries to guess the mime type of the file represented by @path.
 * If @suffix_only is false, uses the mime-magic based lookup first.
 * Handles @path of a non-existent file by falling back
 * on returning a type based on the extension.
 *
 * If you need a faster, less accurate version, use
 * mate_vfs_get_file_mime_type_fast().
 *
 * Returns: the mime-type for file at @path.
 */
const char *
mate_vfs_get_file_mime_type (const char *path, const struct stat *optional_stat_info,
			      gboolean suffix_only)
{
	return mate_vfs_get_file_mime_type_internal (path, optional_stat_info, suffix_only, FALSE);
}

/**
 * mate_vfs_get_mime_type_from_uri:
 * @uri: a file uri.
 *
 * Tries to guess the mime type of the file @uri by
 * checking the file name extension. Works on non-existent
 * files.
 *
 * Returns: the mime-type for file at @uri.
 */
const char *
mate_vfs_get_mime_type_from_uri (MateVFSURI *uri)
{
	const char *result;

	result = mate_vfs_get_mime_type_from_uri_internal (uri);
	if (result == NULL) {
		/* no type, return generic file type */
		result = MATE_VFS_MIME_TYPE_UNKNOWN;
	}

	return result;
}

/**
 * mate_vfs_get_mime_type_from_file_data:
 * @uri: a file uri.
 *
 * Tries to guess the mime type of the file @uri by
 * checking the file data using the magic patterns. Does not handle text files properly.
 *
 * Deprecated:
 *
 * Returns: the mime-type for this filename.
 */
const char *
mate_vfs_get_mime_type_from_file_data (MateVFSURI *uri)
{
	const char *result;
	MateVFSMimeSniffBuffer *buffer;
	MateVFSHandle *handle;
	MateVFSResult error;

	error = mate_vfs_open_uri (&handle, uri, MATE_VFS_OPEN_READ);

	if (error != MATE_VFS_OK) {
		return MATE_VFS_MIME_TYPE_UNKNOWN;
	}
	
	buffer = _mate_vfs_mime_sniff_buffer_new_from_handle (handle);
	result = _mate_vfs_get_mime_type_internal (buffer, NULL, FALSE);	
	mate_vfs_mime_sniff_buffer_free (buffer);
	mate_vfs_close (handle);

	return result;
}

/**
 * mate_vfs_get_mime_type_for_data:
 * @data: a pointer to data in memory.
 * @data_size: size of the data.
 *
 * Tries to guess the mime type of the data in @data
 * using the magic patterns.
 *
 * Returns: the mime-type for @data.
 */
const char *
mate_vfs_get_mime_type_for_data (gconstpointer data, int data_size)
{
	const char *result;
	MateVFSMimeSniffBuffer *buffer;

	buffer = mate_vfs_mime_sniff_buffer_new_from_existing_data
		(data, data_size);
	result = _mate_vfs_get_mime_type_internal (buffer, NULL, FALSE);	

	mate_vfs_mime_sniff_buffer_free (buffer);

	return result;
}

/**
 * mate_vfs_mime_type_is_supertype:
 * @mime_type: a const char * identifying a mime type.
 *
 * Returns: Whether @mime_type is of the form "foo<literal>/</literal>*".
 **/
gboolean
mate_vfs_mime_type_is_supertype (const char *mime_type)
{
	int length;

	if (mime_type == NULL) {
		return FALSE;
	}

	length = strlen (mime_type);

	return length > 2
	       && mime_type[length - 2] == '/' 
	       && mime_type[length - 1] == '*';
}

static char *
extract_prefix_add_suffix (const char *string, const char *separator, const char *suffix)
{
        const char *separator_position;
        int prefix_length;
        char *result;

        separator_position = strstr (string, separator);
        prefix_length = separator_position == NULL
                ? strlen (string)
                : separator_position - string;

        result = g_malloc (prefix_length + strlen (suffix) + 1);
        
        strncpy (result, string, prefix_length);
        result[prefix_length] = '\0';

        strcat (result, suffix);

        return result;
}

/**
 * mate_vfs_get_supertype_from_mime_type:
 * @mime_type: mime type to get the supertype for.
 *
 * Returns: The supertype for @mime_type, allocating new memory.
 *
 * The supertype of an application is computed by removing its
 * suffix, and replacing it with "*". Thus, "foo/bar" will be
 * converted to "foo<literal>/</literal>*".
 *
 * <note>
 * If this function is called on a supertype, it will return a copy of the supertype.
 * </note>
 */
char *
mate_vfs_get_supertype_from_mime_type (const char *mime_type)
{
	if (mime_type == NULL) {
		return NULL;
	}
        return extract_prefix_add_suffix (mime_type, "/", "/*");
}


/**
 * mate_vfs_mime_type_is_equal:
 * @a: a const char * containing a mime type, e.g. "image/png".
 * @b: a const char * containing a mime type, e.g. "image/png".
 * 
 * Compares two mime types to determine if they are equivalent.  They are
 * equivalent if and only if they refer to the same mime type.
 * 
 * Return value: %TRUE, if a and b are equivalent mime types.
 */
gboolean
mate_vfs_mime_type_is_equal (const char *a,
			      const char *b)
{
	gboolean res;
	
	g_return_val_if_fail (a != NULL, FALSE);
	g_return_val_if_fail (b != NULL, FALSE);

	G_LOCK (mate_vfs_mime_mutex);
	res = xdg_mime_mime_type_equal (a, b);
	G_UNLOCK (mate_vfs_mime_mutex);

	return res;
}

/**
 * mate_vfs_mime_type_get_equivalence:
 * @mime_type: a const char * containing a mime type, e.g. "image/png".
 * @base_mime_type: a const char * containing either a mime type or a base mime type.
 * 
 * Compares @mime_type to @base_mime_type.  There are three possible
 * relationships between the two strings.
 *
 * <itemizedlist>
 * <listitem><para>
 *  If they are identical and @mime_type is the same as @base_mime_type,
 *  then %MATE_VFS_MIME_IDENTICAL is returned. This would be the case if
 *  "audio/midi" and "audio/x-midi" are passed in.
 * </para></listitem>
 * <listitem><para>
 * If @base_mime_type is a parent type of @mime_type, then
 * %MATE_VFS_MIME_PARENT is returned.  As an example, "text/plain" is a parent
 * of "text/rss", "image" is a parent of "image/png", and
 * "application/octet-stream" is a parent of almost all types.
 * </para></listitem>
 * <listitem><para>
 * Finally, if the two mime types are unrelated, then %MATE_VFS_MIME_UNRELATED
 * is returned.
 * </para></listitem>
 * </itemizedlist>
 * 
 * Returns: A #MateVFSMimeEquivalence indicating the relationship between @mime_type and @base_mime_type.
 */
MateVFSMimeEquivalence
mate_vfs_mime_type_get_equivalence (const char *mime_type,
				     const char *base_mime_type)
{
	g_return_val_if_fail (mime_type != NULL, MATE_VFS_MIME_UNRELATED);
	g_return_val_if_fail (base_mime_type != NULL, MATE_VFS_MIME_UNRELATED);

	if (mate_vfs_mime_type_is_equal (mime_type, base_mime_type)) {
		return MATE_VFS_MIME_IDENTICAL;
	} else {
		G_LOCK (mate_vfs_mime_mutex);
		if (xdg_mime_mime_type_subclass (mime_type, base_mime_type)) {
			G_UNLOCK (mate_vfs_mime_mutex);
			return MATE_VFS_MIME_PARENT;
		}
		G_UNLOCK (mate_vfs_mime_mutex);
	}

	return MATE_VFS_MIME_UNRELATED;
}



static void
file_date_record_update_mtime (FileDateRecord *record)
{
	struct stat s;
	record->mtime = (g_stat (record->file_path, &s) != -1) ? s.st_mtime : 0;
}

static FileDateRecord *
file_date_record_new (const char *file_path) {
	FileDateRecord *record;

	record = g_new0 (FileDateRecord, 1);
	record->file_path = g_strdup (file_path);

	file_date_record_update_mtime (record);

	return record;
}

static void
file_date_record_free (FileDateRecord *record)
{
	g_free (record->file_path);
	g_free (record);
}

FileDateTracker *
_mate_vfs_file_date_tracker_new (void)
{
	FileDateTracker *tracker;

	tracker = g_new0 (FileDateTracker, 1);
	tracker->check_interval = DEFAULT_DATE_TRACKER_INTERVAL;
	tracker->records = g_hash_table_new (g_str_hash, g_str_equal);

	return tracker;
}

static gboolean
release_key_and_value (gpointer key, gpointer value, gpointer user_data)
{
	g_free (key);
	file_date_record_free (value);

	return TRUE;
}

void
_mate_vfs_file_date_tracker_free (FileDateTracker *tracker)
{
	g_hash_table_foreach_remove (tracker->records, release_key_and_value, NULL);
	g_hash_table_destroy (tracker->records);
	g_free (tracker);
}

/*
 * Record the current mod date for a specified file, so that we can check
 * later whether it has changed.
 */
void
_mate_vfs_file_date_tracker_start_tracking_file (FileDateTracker *tracker, 
				                 const char *local_file_path)
{
	FileDateRecord *record;

	record = g_hash_table_lookup (tracker->records, local_file_path);
	if (record != NULL) {
		file_date_record_update_mtime (record);
	} else {
		g_hash_table_insert (tracker->records, 
				     g_strdup (local_file_path), 
				     file_date_record_new (local_file_path));
	}
}

static void 
check_and_update_one (gpointer key, gpointer value, gpointer user_data)
{
	FileDateRecord *record;
	gboolean *return_has_changed;
	struct stat s;

	g_assert (key != NULL);
	g_assert (value != NULL);
	g_assert (user_data != NULL);

	record = (FileDateRecord *)value;
	return_has_changed = (gboolean *)user_data;

	if (g_stat (record->file_path, &s) != -1) {
		if (s.st_mtime != record->mtime) {
			record->mtime = s.st_mtime;
			*return_has_changed = TRUE;
		}
	}
}

gboolean
_mate_vfs_file_date_tracker_date_has_changed (FileDateTracker *tracker)
{
	time_t now;
	gboolean any_date_changed;

	now = time (NULL);

	/* Note that this might overflow once in a blue moon, but the
	 * only side-effect of that would be a slightly-early check
	 * for changes.
	 */
	if (tracker->last_checked + tracker->check_interval >= now) {
		return FALSE;
	}

	any_date_changed = FALSE;

	g_hash_table_foreach (tracker->records, check_and_update_one, &any_date_changed);

	tracker->last_checked = now;

	return any_date_changed;
}




/**
 * mate_vfs_get_mime_type:
 * @text_uri: path of the file for which to get the mime type.
 *
 * Determine the mime type of @text_uri. The mime type is determined
 * in the same way as by mate_vfs_get_file_info(). This is meant as
 * a convenient function for times when you only want the mime type.
 *
 * Return value: The mime type, or %NULL if there is an error reading
 * the file.
 */
char *
mate_vfs_get_mime_type (const char *text_uri)
{
	MateVFSFileInfo *info;
	char *mime_type;
	MateVFSResult result;

	info = mate_vfs_file_info_new ();
	result = mate_vfs_get_file_info (text_uri, info,
					  MATE_VFS_FILE_INFO_GET_MIME_TYPE |
					  MATE_VFS_FILE_INFO_FOLLOW_LINKS);
	if (info->mime_type == NULL || result != MATE_VFS_OK) {
		mime_type = NULL;
	} else {
		mime_type = g_strdup (info->mime_type);
	}
	mate_vfs_file_info_unref (info);

	return mime_type;
}

/**
 * mate_vfs_get_slow_mime_type:
 * @text_uri: URI of the file for which to get the mime type
 * 
 * Determine the mime type of @text_uri. The mime type is determined
 * in the same way as by mate_vfs_get_file_info(). This is meant as
 * a convenience function for times when you only want the mime type.
 * 
 * Return value: The mime type, or NULL if there is an error reading 
 * the file.
 *
 * Since: 2.14
 **/
char *
mate_vfs_get_slow_mime_type (const char *text_uri)
{
	MateVFSResult result;
	char *ret;

	result = _mate_vfs_get_slow_mime_type_internal (text_uri, &ret);
	g_assert (result == MATE_VFS_OK || ret == NULL);

	return ret;
}

/* This is private due to the feature freeze, maybe it should be public */
MateVFSResult
_mate_vfs_get_slow_mime_type_internal (const char  *text_uri,
					char       **mime_type)
{
	MateVFSFileInfo *info;
	MateVFSResult result;

	g_assert (mime_type != NULL);

	info = mate_vfs_file_info_new ();
	result = mate_vfs_get_file_info (text_uri, info,
					  MATE_VFS_FILE_INFO_GET_MIME_TYPE |
					  MATE_VFS_FILE_INFO_FORCE_SLOW_MIME_TYPE |
					  MATE_VFS_FILE_INFO_FOLLOW_LINKS);
	if (result == MATE_VFS_OK && info->mime_type == NULL) {
		result = MATE_VFS_ERROR_GENERIC;
	}

	if (result != MATE_VFS_OK) {
		*mime_type = NULL;
	} else {
		*mime_type = g_strdup (info->mime_type);
	}

	mate_vfs_file_info_unref (info);

	return result;
}

/**
 * mate_vfs_mime_reload:
 *
 * Reload the MIME database.
 **/
void
mate_vfs_mime_reload (void)
{
        mate_vfs_mime_info_cache_reload (NULL);
        mate_vfs_mime_info_reload ();
}
