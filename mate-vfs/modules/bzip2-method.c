/*
 * bzip2-method.c - Bzip2 access method for the MATE Virtual File
 *                  System.
 *
 * Copyright (C) 1999 Free Software Foundation
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
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite
 * 330, Boston, MA 02111-1307, USA.
 *
 * Author: Cody Russell  <cody@jhu.edu>
 */

#include <config.h>

#include <glib.h>
#include <libmatevfs/mate-vfs-handle.h>
#include <libmatevfs/mate-vfs-mime.h>
#include <libmatevfs/mate-vfs-module.h>
#include <libmatevfs/mate-vfs-ops.h>
#include <stdio.h>
#include <string.h>
#include <string.h>
#include <time.h>

#include <bzlib.h>

#ifdef HAVE_OLDER_BZIP2
#define BZ2_bzDecompressInit  bzDecompressInit
#define BZ2_bzCompressInit    bzCompressInit
#define BZ2_bzDecompress      bzDecompress
#define BZ2_bzCompress        bzCompress
#endif

#define BZ_BUFSIZE   5000

struct _Bzip2MethodHandle {
	MateVFSURI      *uri;
	MateVFSHandle   *parent_handle;
	MateVFSOpenMode open_mode;

	BZFILE           *file;
	MateVFSResult   last_vfs_result;
	gint             last_bz_result;
	bz_stream        bzstream;
	guchar           *buffer;
};

typedef struct _Bzip2MethodHandle Bzip2MethodHandle;

static MateVFSResult do_open (MateVFSMethod *method,
			       MateVFSMethodHandle **method_handle,
			       MateVFSURI *uri,
			       MateVFSOpenMode mode,
			       MateVFSContext *context);

static MateVFSResult do_create (MateVFSMethod *method,
				 MateVFSMethodHandle **method_handle,
				 MateVFSURI *uri,
				 MateVFSOpenMode mode,
				 gboolean exclusive,
				 guint perm,
				 MateVFSContext *context);

static MateVFSResult do_close (MateVFSMethod *method,
				MateVFSMethodHandle *method_handle,
				MateVFSContext *context);

static MateVFSResult do_read (MateVFSMethod *method,
			       MateVFSMethodHandle *method_handle,
			       gpointer buffer,
			       MateVFSFileSize num_bytes,
			       MateVFSFileSize *bytes_read,
			       MateVFSContext *context);

static MateVFSResult do_write (MateVFSMethod *method,
				MateVFSMethodHandle *method_handle,
				gconstpointer buffer,
				MateVFSFileSize num_bytes,
				MateVFSFileSize *bytes_written,
				MateVFSContext *context);

static MateVFSResult do_get_file_info  (MateVFSMethod *method,
		                         MateVFSURI *uri,
				         MateVFSFileInfo *file_info,
					 MateVFSFileInfoOptions options,
					 MateVFSContext *context);

static gboolean do_is_local (MateVFSMethod *method, const MateVFSURI *uri);

static MateVFSMethod method = {
	sizeof (MateVFSMethod),
	do_open,
	do_create,
	do_close,
	do_read,
	do_write,
	NULL,		/* seek            */
	NULL,		/* tell            */
	NULL,		/* truncate_handle */
	NULL,		/* open_directory  */
	NULL,		/* close_directory */
	NULL,		/* read_directory  */
	do_get_file_info,
	NULL,		/* get_file_info_from_handle */
	do_is_local,
	NULL,		/* make_directory  */
	NULL,		/* remove_directory */
	NULL,		/* move */
	NULL,		/* unlink */
	NULL,		/* set_file_info */
	NULL, 		/* truncate */
	NULL, 		/* find_directory */
	NULL            /* create_symbolic_link */
};

#define RETURN_IF_FAIL(action)			\
G_STMT_START {					\
	MateVFSResult __tmp_result;		\
						\
	__tmp_result = (action);		\
	if (__tmp_result != MATE_VFS_OK)	\
		return __tmp_result;		\
} G_STMT_END

#define VALID_URI(u) ((u)->parent!=NULL&&(((u)->text==NULL)||((u)->text[0]=='\0')||(((u)->text[0]=='/')&&((u)->text[1]=='\0'))))

static Bzip2MethodHandle *
bzip2_method_handle_new (MateVFSHandle *parent_handle,
			 MateVFSURI *uri,
			 MateVFSOpenMode open_mode)
{
	Bzip2MethodHandle *new;

	new = g_new (Bzip2MethodHandle, 1);

	new->parent_handle = parent_handle;
	new->uri = mate_vfs_uri_ref (uri);
	new->open_mode = open_mode;

	new->buffer = NULL;

	return new;
}

static void
bzip2_method_handle_destroy (Bzip2MethodHandle *handle)
{
	mate_vfs_uri_unref (handle->uri);
	g_free (handle->buffer);
	g_free (handle);
}

static gboolean
bzip2_method_handle_init_for_decompress (Bzip2MethodHandle *handle)
{
	handle->bzstream.bzalloc = NULL;
	handle->bzstream.bzfree  = NULL;
	handle->bzstream.opaque  = NULL;

	g_free (handle->buffer);

	handle->buffer = g_malloc (BZ_BUFSIZE);
	handle->bzstream.next_in = (char *)handle->buffer;
	handle->bzstream.avail_in = 0;

	/* FIXME bugzilla.eazel.com 1177: Make small, and possibly verbosity, configurable! */
	if (BZ2_bzDecompressInit (&handle->bzstream, 0, 0) != BZ_OK) {
		g_free (handle->buffer);
		return FALSE;
	}

	handle->last_bz_result = BZ_OK;
	handle->last_vfs_result = MATE_VFS_OK;

	return TRUE;
}

static gboolean
bzip2_method_handle_init_for_compress (Bzip2MethodHandle *handle) G_GNUC_UNUSED;

static gboolean
bzip2_method_handle_init_for_compress (Bzip2MethodHandle *handle)
{
	handle->bzstream.bzalloc = NULL;
	handle->bzstream.bzfree  = NULL;
	handle->bzstream.opaque  = NULL;

	g_free (handle->buffer);

	handle->buffer = g_malloc (BZ_BUFSIZE);
	handle->bzstream.next_out = (char *)handle->buffer;
	handle->bzstream.avail_out = BZ_BUFSIZE;

	/* FIXME bugzilla.eazel.com 1174: We want this to be user configurable.  */
	if (BZ2_bzCompressInit (&handle->bzstream, 3, 0, 30) != BZ_OK) {
		g_free (handle->buffer);
		return FALSE;
	}

	handle->last_bz_result = BZ_OK;
	handle->last_vfs_result = MATE_VFS_OK;

	return TRUE;
}

static MateVFSResult
result_from_bz_result (gint bz_result)
{
	switch (bz_result) {
	case BZ_OK:
	case BZ_STREAM_END:
		return MATE_VFS_OK;

	case BZ_MEM_ERROR:
		return MATE_VFS_ERROR_NO_MEMORY;

	case BZ_PARAM_ERROR:
		return MATE_VFS_ERROR_BAD_PARAMETERS;

	case BZ_DATA_ERROR:
		return MATE_VFS_ERROR_CORRUPTED_DATA;

	case BZ_UNEXPECTED_EOF:
		return MATE_VFS_ERROR_EOF;

	case BZ_SEQUENCE_ERROR:
		return MATE_VFS_ERROR_NOT_PERMITTED;

	default:
		return MATE_VFS_ERROR_INTERNAL;
	}
}

static MateVFSResult
flush_write (Bzip2MethodHandle *bzip2_handle)
{
	MateVFSHandle *parent_handle;
	MateVFSResult result;
	gboolean done;
	bz_stream *bzstream;
	gint bz_result;

	bzstream = &bzip2_handle->bzstream;
	bzstream->avail_in = 0;
	parent_handle = bzip2_handle->parent_handle;

	done = FALSE;
	bz_result = BZ_OK;
	while (bz_result == BZ_OK || bz_result == BZ_STREAM_END) {
		MateVFSFileSize bytes_written;
		MateVFSFileSize len;

		len = BZ_BUFSIZE - bzstream->avail_out;

		result = mate_vfs_write (parent_handle, bzip2_handle->buffer,
					  len, &bytes_written);
		RETURN_IF_FAIL (result);

		bzstream->next_out = (char *)bzip2_handle->buffer;
		bzstream->avail_out = BZ_BUFSIZE;

		if (done)
			break;

		bz_result = BZ2_bzCompress (bzstream, BZ_FINISH);

		done = (bzstream->avail_out != 0 || bz_result == BZ_STREAM_END);
	}

	if (bz_result == BZ_OK || bz_result == BZ_STREAM_END)
		return MATE_VFS_OK;
	else
		return result_from_bz_result (bz_result);
}

/* Open */
/* TODO: Check that there is no subpath. */

static MateVFSResult
do_open (MateVFSMethod *method,
	 MateVFSMethodHandle **method_handle,
	 MateVFSURI *uri,
	 MateVFSOpenMode open_mode,
	 MateVFSContext *context)
{
	MateVFSHandle *parent_handle;
	MateVFSURI *parent_uri;
	MateVFSResult result;
	Bzip2MethodHandle *bzip2_handle;

	_MATE_VFS_METHOD_PARAM_CHECK (method_handle != NULL);
	_MATE_VFS_METHOD_PARAM_CHECK (uri != NULL);

	/* Check that the URI is valid.  */
	if (!VALID_URI(uri)) return MATE_VFS_ERROR_INVALID_URI;

	if (open_mode & MATE_VFS_OPEN_WRITE)
		return MATE_VFS_ERROR_INVALID_OPEN_MODE;

	parent_uri = uri->parent;

	if (open_mode & MATE_VFS_OPEN_RANDOM)
		return MATE_VFS_ERROR_NOT_SUPPORTED;

	result = mate_vfs_open_uri (&parent_handle, parent_uri, open_mode);
	RETURN_IF_FAIL (result);

	bzip2_handle = bzip2_method_handle_new (parent_handle, uri, open_mode);

	if (result != MATE_VFS_OK) {
		mate_vfs_close (parent_handle);
		bzip2_method_handle_destroy (bzip2_handle);
		return result;
	}

	if (!bzip2_method_handle_init_for_decompress (bzip2_handle)) {
		mate_vfs_close (parent_handle);
		bzip2_method_handle_destroy (bzip2_handle);
		return MATE_VFS_ERROR_INTERNAL;
	}

	*method_handle = (MateVFSMethodHandle *) bzip2_handle;

	return MATE_VFS_OK;
}

/* Create */

static MateVFSResult
do_create (MateVFSMethod *method,
	   MateVFSMethodHandle **method_handle,
	   MateVFSURI *uri,
	   MateVFSOpenMode mode,
	   gboolean exclusive,
	   guint perm,
	   MateVFSContext *context)
{
	_MATE_VFS_METHOD_PARAM_CHECK (method_handle != NULL);
	_MATE_VFS_METHOD_PARAM_CHECK (uri != NULL);

	return MATE_VFS_ERROR_NOT_SUPPORTED;
}

/* Close */

static MateVFSResult
do_close (MateVFSMethod *method,
	  MateVFSMethodHandle *method_handle,
	  MateVFSContext *context)
{
	Bzip2MethodHandle *bzip2_handle;
	MateVFSResult result;

	_MATE_VFS_METHOD_PARAM_CHECK (method_handle != NULL);

	bzip2_handle = (Bzip2MethodHandle *) method_handle;

	if (bzip2_handle->open_mode & MATE_VFS_OPEN_WRITE)
		result = flush_write (bzip2_handle);
	else
		result = MATE_VFS_OK;

	if (result == MATE_VFS_OK)
		result = mate_vfs_close (bzip2_handle->parent_handle);

	bzip2_method_handle_destroy (bzip2_handle);

	return result;
}

/* Read */

static MateVFSResult
fill_buffer (Bzip2MethodHandle *bzip2_handle,
	     MateVFSFileSize num_bytes)
{
	MateVFSResult result;
	MateVFSFileSize count;
	bz_stream *bzstream;

	bzstream = &bzip2_handle->bzstream;

	if (bzstream->avail_in > 0)
		return MATE_VFS_OK;

	result = mate_vfs_read (bzip2_handle->parent_handle,
				 bzip2_handle->buffer,
				 BZ_BUFSIZE,
				 &count);

	if (result != MATE_VFS_OK) {
		if (bzstream->avail_out == num_bytes)
			return result;
		bzip2_handle->last_vfs_result = result;
	} else {
		bzstream->next_in = (char *)bzip2_handle->buffer;
		bzstream->avail_in = count;
	}

	return MATE_VFS_OK;
}

/* TODO: Concatenated Bzip2 file handling. */

static MateVFSResult
do_read (MateVFSMethod *method,
	 MateVFSMethodHandle *method_handle,
	 gpointer buffer,
	 MateVFSFileSize num_bytes,
	 MateVFSFileSize *bytes_read,
	 MateVFSContext *context)
{
	Bzip2MethodHandle *bzip2_handle;
	MateVFSResult result;
	bz_stream *bzstream;
	int bz_result;

	*bytes_read = 0;

	bzip2_handle = (Bzip2MethodHandle *) method_handle;
	bzstream = &bzip2_handle->bzstream;

	if (bzip2_handle->last_bz_result != BZ_OK) {
		if (bzip2_handle->last_bz_result == BZ_STREAM_END)
			return MATE_VFS_ERROR_EOF;
		else
			return result_from_bz_result (bzip2_handle->last_bz_result);
	} else if (bzip2_handle->last_vfs_result != MATE_VFS_OK) {
		return bzip2_handle->last_vfs_result;
	}

	bzstream->next_out = buffer;
	bzstream->avail_out = num_bytes;

	while (bzstream->avail_out != 0) {
		result = fill_buffer (bzip2_handle, num_bytes);
		RETURN_IF_FAIL (result);

		bz_result = BZ2_bzDecompress (&bzip2_handle->bzstream);

		if (bzip2_handle->last_bz_result != BZ_OK
		    && bzstream->avail_out == num_bytes) {
			bzip2_handle->last_bz_result = bz_result;
			return result_from_bz_result (bzip2_handle->last_bz_result);
		}

		*bytes_read = num_bytes - bzstream->avail_out;

		if (bz_result == BZ_STREAM_END) {
			bzip2_handle->last_bz_result = bz_result;
			break;
		}

	}

	return MATE_VFS_OK;
}

/* Write. */

static MateVFSResult
do_write (MateVFSMethod *method,
	  MateVFSMethodHandle *method_handle,
	  gconstpointer buffer,
	  MateVFSFileSize num_bytes,
	  MateVFSFileSize *bytes_written,
	  MateVFSContext *context)
{
	Bzip2MethodHandle *bzip2_handle;
	MateVFSResult result;
	bz_stream *bzstream;
	gint bz_result;

	bzip2_handle = (Bzip2MethodHandle *) method_handle;
	bzstream = &bzip2_handle->bzstream;

	bzstream->next_in = (gpointer) buffer;
	bzstream->avail_in = num_bytes;

	result = MATE_VFS_OK;

	while (bzstream->avail_in != 0 && result == MATE_VFS_OK) {
		if (bzstream->avail_out == 0) {
			MateVFSFileSize written;

			bzstream->next_out = (char *)bzip2_handle->buffer;
			result = mate_vfs_write (bzip2_handle->parent_handle,
						  bzip2_handle->buffer,
						  BZ_BUFSIZE, &written);
			if (result != MATE_VFS_OK)
				break;

			bzstream->avail_out += written;
		}

		bz_result = BZ2_bzCompress (bzstream, BZ_RUN);
		result = result_from_bz_result (bz_result);
	}

	*bytes_written = num_bytes - bzstream->avail_in;

	return result;
}

static gboolean
do_is_local (MateVFSMethod *method, const MateVFSURI *uri)
{
	g_return_val_if_fail (uri != NULL, FALSE);
	return mate_vfs_uri_is_local (uri->parent);
}

static MateVFSResult 
do_get_file_info  (MateVFSMethod *method,
	           MateVFSURI *uri,
		   MateVFSFileInfo *file_info,
		   MateVFSFileInfoOptions options,
		   MateVFSContext *context) 
{
	MateVFSResult result;

	/* Check that the URI is valid.  */
	if (!VALID_URI(uri)) return MATE_VFS_ERROR_INVALID_URI;

	result = mate_vfs_get_file_info_uri(uri->parent, file_info, options);

	if(result == MATE_VFS_OK) {
		gint namelen = strlen(file_info->name);
		
		/* work out the name */
		/* FIXME bugzilla.eazel.com 2790: handle uppercase */
		if(namelen > 4 &&
				file_info->name[namelen-1] == '2' &&
				file_info->name[namelen-2] == 'z' &&
				file_info->name[namelen-3] == 'b' &&
				file_info->name[namelen-4] == '.')
			file_info->name[namelen-4] = '\0';

		/* we can't tell the size without uncompressing it */
		/*file_info->valid_fields &= ~MATE_VFS_FILE_INFO_FIELDS_SIZE;*/

		/* guess the mime type of the file inside */
		/* FIXME bugzilla.eazel.com 2791: guess mime based on contents */
		g_free(file_info->mime_type);
		file_info->mime_type = g_strdup(mate_vfs_mime_type_from_name(file_info->name));
	}

	return result;
}

MateVFSMethod *
vfs_module_init (const char *method_name, const char *args)
{
	return &method;
}

void
vfs_module_shutdown (MateVFSMethod *method)
{
	return;
}
