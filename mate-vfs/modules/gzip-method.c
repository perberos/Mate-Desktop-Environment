/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gzip-method.c - GZIP access method for the MATE Virtual File
   System.

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

   Author: Ettore Perazzoli <ettore@comm2000.it> */

#include <config.h>

#include <glib.h>
#include <libmatevfs/mate-vfs-mime.h>
#include <libmatevfs/mate-vfs-module.h>
#include <libmatevfs/mate-vfs-ops.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <zlib.h>

struct _GZipMethodHandle {
	MateVFSURI *uri;
	MateVFSHandle *parent_handle;
	MateVFSOpenMode open_mode;
	time_t modification_time;

	MateVFSResult last_vfs_result;
	gint last_z_result;
	z_stream zstream;
	guchar *buffer;
	guint32 crc;
};
typedef struct _GZipMethodHandle GZipMethodHandle;


#define GZIP_MAGIC_1 0x1f
#define GZIP_MAGIC_2 0x8b

#define GZIP_FLAG_ASCII        0x01 /* bit 0 set: file probably ascii text */
#define GZIP_FLAG_HEAD_CRC     0x02 /* bit 1 set: header CRC present */
#define GZIP_FLAG_EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
#define GZIP_FLAG_ORIG_NAME    0x08 /* bit 3 set: original file name present */
#define GZIP_FLAG_COMMENT      0x10 /* bit 4 set: file comment present */
#define GZIP_FLAG_RESERVED     0xE0 /* bits 5..7: reserved */

#define GZIP_HEADER_SIZE 10
#define GZIP_FOOTER_SIZE 8

#define Z_BUFSIZE 16384


static MateVFSResult	do_open		(MateVFSMethod *method,
					 MateVFSMethodHandle **method_handle,
					 MateVFSURI *uri,
					 MateVFSOpenMode mode,
					 MateVFSContext *context);

static MateVFSResult	do_create	(MateVFSMethod *method,
					 MateVFSMethodHandle **method_handle,
					 MateVFSURI *uri,
					 MateVFSOpenMode mode,
					 gboolean exclusive,
					 guint perm,
					 MateVFSContext *context);

static MateVFSResult	do_close	(MateVFSMethod *method,
					 MateVFSMethodHandle *method_handle,
					 MateVFSContext *context);

static MateVFSResult	do_read		(MateVFSMethod *method,
					 MateVFSMethodHandle *method_handle,
					 gpointer buffer,
					 MateVFSFileSize num_bytes,
					 MateVFSFileSize *bytes_read,
					 MateVFSContext *context);

static MateVFSResult	do_write	(MateVFSMethod *method,
					 MateVFSMethodHandle *method_handle,
					 gconstpointer buffer,
					 MateVFSFileSize num_bytes,
					 MateVFSFileSize *bytes_written,
					 MateVFSContext *context);

static MateVFSResult	do_get_file_info(MateVFSMethod *method,
	           			 MateVFSURI *uri,
		   			 MateVFSFileInfo *file_info,
		   			 MateVFSFileInfoOptions options,
		   			 MateVFSContext *context);

static gboolean		do_is_local	(MateVFSMethod *method,
					 const MateVFSURI *uri);

static MateVFSMethod method = {
	sizeof (MateVFSMethod),
	do_open,
	do_create,
	do_close,
	do_read,
	do_write,
	NULL,			/* seek */
	NULL,			/* tell */
	NULL,			/* truncate_handle FIXME bugzilla.eazel.com 1175 */
	NULL,			/* open_directory */
	NULL,			/* close_directory */
	NULL,			/* read_directory */
	do_get_file_info,
	NULL,			/* get_file_info_from_handle */
	do_is_local,
	NULL,			/* make_directory */
	NULL,			/* remove_directory */
	NULL,			/* move */
	NULL,                   /* unlink */
	NULL,			/* check_same_fs */
	NULL,			/* set_file_info */
	NULL, 			/* truncate */
	NULL,			/* find_directory */
	NULL                    /* create_symbolic_link */
};

#define RETURN_IF_FAIL(action)			\
G_STMT_START{					\
	MateVFSResult __tmp_result;		\
						\
	__tmp_result = (action);		\
	if (__tmp_result != MATE_VFS_OK)	\
		return __tmp_result;		\
}G_STMT_END

#define VALID_URI(u) ((u)->parent!=NULL&&(((u)->text==NULL)||((u)->text[0]=='\0')||(((u)->text[0]=='/')&&((u)->text[1]=='\0'))))


/* GZip handle creation/destruction.  */

static GZipMethodHandle *
gzip_method_handle_new (MateVFSHandle *parent_handle,
                        time_t modification_time,
                        MateVFSURI *uri,
			MateVFSOpenMode open_mode)
{
	GZipMethodHandle *new;

	new = g_new (GZipMethodHandle, 1);

	new->parent_handle = parent_handle;
	new->modification_time = modification_time;
	new->uri = mate_vfs_uri_ref (uri);
	new->open_mode = open_mode;

	new->buffer = NULL;
	new->crc = crc32 (0, Z_NULL, 0);

	return new;
}

static void
gzip_method_handle_destroy (GZipMethodHandle *handle)
{
	mate_vfs_uri_unref (handle->uri);
	g_free (handle->buffer);
	g_free (handle);
}


/* GZip method initialization for compression/decompression.  */

static gboolean
gzip_method_handle_init_for_inflate (GZipMethodHandle *handle)
{
	handle->zstream.zalloc = NULL;
	handle->zstream.zfree = NULL;
	handle->zstream.opaque = NULL;

	g_free (handle->buffer);

	handle->buffer = g_malloc (Z_BUFSIZE);
	handle->zstream.next_in = handle->buffer;
	handle->zstream.avail_in = 0;

	if (inflateInit2 (&handle->zstream, -MAX_WBITS) != Z_OK) {
		g_free (handle->buffer);
		return FALSE;
	}

	handle->last_z_result = Z_OK;
	handle->last_vfs_result = MATE_VFS_OK;

	return TRUE;
}

static gboolean
gzip_method_handle_init_for_deflate (GZipMethodHandle *handle)
{
	handle->zstream.zalloc = NULL;
	handle->zstream.zfree = NULL;
	handle->zstream.opaque = NULL;

	g_free (handle->buffer);

	handle->buffer = g_malloc (Z_BUFSIZE);
	handle->zstream.next_out = handle->buffer;
	handle->zstream.avail_out = Z_BUFSIZE;

	/* FIXME bugzilla.eazel.com 1174: We want this to be user-configurable.  */
	if (deflateInit2 (&handle->zstream, Z_DEFAULT_COMPRESSION,
			  Z_DEFLATED, -MAX_WBITS, MAX_MEM_LEVEL,
			  Z_DEFAULT_STRATEGY) != Z_OK) {
		g_free (handle->buffer);
		return FALSE;
	}

	handle->last_z_result = Z_OK;
	handle->last_vfs_result = MATE_VFS_OK;

	return TRUE;
}


static MateVFSResult
result_from_z_result (gint z_result)
{
	switch (z_result) {
	case Z_OK:
	case Z_STREAM_END: /* FIXME bugzilla.eazel.com 1173: Is this right? */
		return MATE_VFS_OK;
	case Z_DATA_ERROR:
		return MATE_VFS_ERROR_CORRUPTED_DATA;
	default:
		return MATE_VFS_ERROR_INTERNAL;
	}
}


/* Functions to skip data in the file.  */

static MateVFSResult
skip_string (MateVFSHandle *handle)
{
	MateVFSResult result;
	guchar c;
	MateVFSFileSize bytes_read;

	do {
		result = mate_vfs_read (handle, &c, 1, &bytes_read);
		RETURN_IF_FAIL (result);

		if (bytes_read != 1)
			return MATE_VFS_ERROR_WRONG_FORMAT;
	} while (c != 0);

	return MATE_VFS_OK;
}

static gboolean
skip (MateVFSHandle *handle,
      MateVFSFileSize num_bytes)
{
	MateVFSResult result;
	guchar *tmp;
	MateVFSFileSize bytes_read;

	tmp = g_alloca (num_bytes);

	result = mate_vfs_read (handle, tmp, num_bytes, &bytes_read);
	RETURN_IF_FAIL (result);

	if (bytes_read != num_bytes)
		return MATE_VFS_ERROR_WRONG_FORMAT;

	return TRUE;
}


/* Utility function to write a gulong value.  */

static MateVFSResult
write_guint32 (MateVFSHandle *handle,
               guint32 value)
{
	guint i;
	guchar buffer[4];
	MateVFSFileSize bytes_written;

	for (i = 0; i < 4; i++) {
		buffer[i] = value & 0xff;
		value >>= 8;
	}

	return mate_vfs_write (handle, buffer, 4, &bytes_written);
}


/* GZIP Header functions.  */

static MateVFSResult
read_gzip_header (MateVFSHandle *handle,
                  time_t *modification_time)
{
	MateVFSResult result;
	guchar buffer[GZIP_HEADER_SIZE];
	MateVFSFileSize bytes_read;
	guint mode;
	guint flags;

	result = mate_vfs_read (handle, buffer, GZIP_HEADER_SIZE,
				 &bytes_read);
	RETURN_IF_FAIL (result);

	if (bytes_read != GZIP_HEADER_SIZE)
		return MATE_VFS_ERROR_WRONG_FORMAT;

	if (buffer[0] != GZIP_MAGIC_1 || buffer[1] != GZIP_MAGIC_2)
		return MATE_VFS_ERROR_WRONG_FORMAT;

	mode = buffer[2];
	if (mode != 8) /* Mode: deflate */
		return MATE_VFS_ERROR_WRONG_FORMAT;

	flags = buffer[3];

	if (flags & GZIP_FLAG_RESERVED)
		return MATE_VFS_ERROR_WRONG_FORMAT;

	if (flags & GZIP_FLAG_EXTRA_FIELD) {
		guchar tmp[2];
		MateVFSFileSize bytes_read;

		if (mate_vfs_read (handle, tmp, 2, &bytes_read)
		    || bytes_read != 2)
			return MATE_VFS_ERROR_WRONG_FORMAT;
		if (! skip (handle, tmp[0] | (tmp[0] << 8)))
			return MATE_VFS_ERROR_WRONG_FORMAT;
	}

	if (flags & GZIP_FLAG_ORIG_NAME)
		RETURN_IF_FAIL (skip_string (handle));

	if (flags & GZIP_FLAG_COMMENT)
		RETURN_IF_FAIL (skip_string (handle));

	if (flags & GZIP_FLAG_HEAD_CRC)
		RETURN_IF_FAIL (skip (handle, 2));

	*modification_time = (buffer[4] | (buffer[5] << 8)
			      | (buffer[6] << 16) | (buffer[7] << 24));
	return MATE_VFS_OK;
}

static MateVFSResult
write_gzip_header (MateVFSHandle *handle, time_t modification_time)
{
	MateVFSResult result;
	guchar buffer[GZIP_HEADER_SIZE];
	MateVFSFileSize bytes_written;

	buffer[0] = GZIP_MAGIC_1;     /* magic 1 */
	buffer[1] = GZIP_MAGIC_2;     /* magic 2 */
	buffer[2] = Z_DEFLATED;       /* method */
	buffer[3] = 0;                /* flags */
	buffer[4] = (guchar) ((modification_time >>  0) & 0xFF);   /* time 1 */
	buffer[5] = (guchar) ((modification_time >>  8) & 0xFF);   /* time 2 */
	buffer[6] = (guchar) ((modification_time >> 16) & 0xFF);   /* time 3 */
	buffer[7] = (guchar) ((modification_time >> 24) & 0xFF);   /* time 4 */
	buffer[8] = 0;                /* xflags */
	buffer[9] = 3;                /* OS (Unix) */

	result = mate_vfs_write (handle, buffer, GZIP_HEADER_SIZE,
				  &bytes_written);
	RETURN_IF_FAIL (result);

	if (bytes_written != GZIP_HEADER_SIZE)
		return MATE_VFS_ERROR_IO;

	return MATE_VFS_OK;
}

static MateVFSResult
flush_write (GZipMethodHandle *gzip_handle)
{
	MateVFSHandle *parent_handle;
	MateVFSResult result;
	gboolean done;
	z_stream *zstream;
	gint z_result;

	zstream = &gzip_handle->zstream;
	zstream->avail_in = 0;         /* (Should be zero already anyway.)  */

	parent_handle = gzip_handle->parent_handle;

	done = FALSE;
	z_result = Z_OK;
	while (z_result == Z_OK || z_result == Z_STREAM_END) {
		MateVFSFileSize bytes_written;
		MateVFSFileSize len;

		len = Z_BUFSIZE - zstream->avail_out;

		result = mate_vfs_write (parent_handle, gzip_handle->buffer,
					  len, &bytes_written);
		RETURN_IF_FAIL (result);

		zstream->next_out = gzip_handle->buffer;
		zstream->avail_out = Z_BUFSIZE;

		if (done)
			break;

		z_result = deflate(zstream, Z_FINISH);

		/* Ignore the second of two consecutive flushes.  */
		if (z_result == Z_BUF_ERROR)
			z_result = Z_OK;

		/* Deflate has finished flushing only when it hasn't used up
		   all the available space in the output buffer.  */
		done = (zstream->avail_out != 0 || z_result == Z_STREAM_END);
	}

	result = write_guint32 (parent_handle, gzip_handle->crc);
	RETURN_IF_FAIL (result);

	result = write_guint32 (parent_handle, zstream->total_in);
	RETURN_IF_FAIL (result);

	if (z_result == Z_OK || z_result == Z_STREAM_END)
		return MATE_VFS_OK;
	else
		return result_from_z_result (z_result);
}


/* Open.  */

/* TODO: 
   - Check that there is no subpath.  */
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
	GZipMethodHandle *gzip_handle;
	time_t modification_time;

	_MATE_VFS_METHOD_PARAM_CHECK (method_handle != NULL);
	_MATE_VFS_METHOD_PARAM_CHECK (uri != NULL);

	/* Check that the URI is valid.  */
	if (!VALID_URI(uri)) return MATE_VFS_ERROR_INVALID_URI;

	parent_uri = uri->parent;

	if (open_mode & MATE_VFS_OPEN_RANDOM)
		return MATE_VFS_ERROR_NOT_SUPPORTED;

	result = mate_vfs_open_uri (&parent_handle, parent_uri, open_mode);
	RETURN_IF_FAIL (result);

	if (open_mode & MATE_VFS_OPEN_READ) {
		result = read_gzip_header (parent_handle, &modification_time);
		if (result != MATE_VFS_OK) {
			mate_vfs_close (parent_handle);
			return result;
		}

		gzip_handle = gzip_method_handle_new (parent_handle,
						      modification_time,
						      uri,
						      open_mode);

		if (! gzip_method_handle_init_for_inflate (gzip_handle)) {
			mate_vfs_close (parent_handle);
			gzip_method_handle_destroy (gzip_handle);
			return MATE_VFS_ERROR_INTERNAL;
		}
	} else {                          /* MATE_VFS_OPEN_WRITE */
		modification_time = time (NULL);
		result = write_gzip_header (parent_handle, modification_time);
		RETURN_IF_FAIL (result);

		/* FIXME bugzilla.eazel.com 1172: need to set modification_time */
		gzip_handle = gzip_method_handle_new (parent_handle,
						      modification_time,
						      uri,
						      open_mode);

		if (! gzip_method_handle_init_for_deflate (gzip_handle)) {
			mate_vfs_close (parent_handle);
			gzip_method_handle_destroy (gzip_handle);
			return MATE_VFS_ERROR_INTERNAL;
		}
	}

	*method_handle = (MateVFSMethodHandle *) gzip_handle;

	return MATE_VFS_OK;
}


/* Create.  */

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

	return MATE_VFS_ERROR_NOT_SUPPORTED; /* FIXME bugzilla.eazel.com 1170 */
}


/* Close.  */

static MateVFSResult
do_close (MateVFSMethod *method,
	  MateVFSMethodHandle *method_handle,
	  MateVFSContext *context)
{
	GZipMethodHandle *gzip_handle;
	MateVFSResult result;

	_MATE_VFS_METHOD_PARAM_CHECK (method_handle != NULL);

	gzip_handle = (GZipMethodHandle *) method_handle;

	if (gzip_handle->open_mode & MATE_VFS_OPEN_WRITE)
		result = flush_write (gzip_handle);
	else
		result = MATE_VFS_OK;

	if (result == MATE_VFS_OK)
		result = mate_vfs_close (gzip_handle->parent_handle);

	gzip_method_handle_destroy (gzip_handle);

	return result;
}


/* Read. */
static MateVFSResult
fill_buffer (GZipMethodHandle *gzip_handle,
	     MateVFSFileSize num_bytes)
{
	MateVFSResult result;
	MateVFSFileSize count;
	z_stream *zstream;

	zstream = &gzip_handle->zstream;

	if (zstream->avail_in > 0)
		return MATE_VFS_OK;

	result = mate_vfs_read (gzip_handle->parent_handle,
				 gzip_handle->buffer,
				 Z_BUFSIZE,
				 &count);

	if (result != MATE_VFS_OK) {
		if (zstream->avail_out == num_bytes)
			return result;
		gzip_handle->last_vfs_result = result;
	} else {
		zstream->next_in = gzip_handle->buffer;
		zstream->avail_in = count;
	}

	return MATE_VFS_OK;
}

/* FIXME bugzilla.eazel.com 1165: TODO:
   - Concatenated GZIP file handling.  */
static MateVFSResult
do_read (MateVFSMethod *method,
	 MateVFSMethodHandle *method_handle,
	 gpointer buffer,
	 MateVFSFileSize num_bytes,
	 MateVFSFileSize *bytes_read,
	 MateVFSContext *context)
{
	GZipMethodHandle *gzip_handle;
	MateVFSResult result;
	z_stream *zstream;
	int z_result;
	guchar *crc_start;

	*bytes_read = 0;

	crc_start = buffer;

	gzip_handle = (GZipMethodHandle *) method_handle;

	zstream = &gzip_handle->zstream;

	if (gzip_handle->last_z_result != Z_OK) {
		if (gzip_handle->last_z_result == Z_STREAM_END) {
			*bytes_read = 0;
			return MATE_VFS_ERROR_EOF;
		} else
			return result_from_z_result (gzip_handle->last_z_result);
	} else if (gzip_handle->last_vfs_result != MATE_VFS_OK) {
		return gzip_handle->last_vfs_result;
	}

	zstream->next_out = buffer;
	zstream->avail_out = num_bytes;

	while (zstream->avail_out != 0) {
		result = fill_buffer (gzip_handle, num_bytes);
		RETURN_IF_FAIL (result);

		z_result = inflate (&gzip_handle->zstream, Z_NO_FLUSH);
		if (z_result == Z_STREAM_END) {
			gzip_handle->last_z_result = z_result;
			break;
		} else if (z_result != Z_OK) {	
			/* FIXME bugzilla.eazel.com 1165: Concatenated GZIP files?  */
			gzip_handle->last_z_result = z_result;
		}

		if (gzip_handle->last_z_result != Z_OK
		    && zstream->avail_out == num_bytes)
			return result_from_z_result (gzip_handle->last_z_result);
	}

	gzip_handle->crc = crc32 (gzip_handle->crc,
				  crc_start,
				  (guint) (zstream->next_out - crc_start));

	*bytes_read = num_bytes - zstream->avail_out;

	return MATE_VFS_OK;
}


/* Write.  */

static MateVFSResult
do_write (MateVFSMethod *method,
	  MateVFSMethodHandle *method_handle,
	  gconstpointer buffer,
	  MateVFSFileSize num_bytes,
	  MateVFSFileSize *bytes_written,
	  MateVFSContext *context)
{
	GZipMethodHandle *gzip_handle;
	MateVFSResult result;
	z_stream *zstream;
	gint z_result;

	gzip_handle = (GZipMethodHandle *) method_handle;
	zstream = &gzip_handle->zstream;

	/* This cast sucks.  It is not my fault, though.  :-)  */
	zstream->next_in = (gpointer) buffer;
	zstream->avail_in = num_bytes;

	result = MATE_VFS_OK;

	while (zstream->avail_in != 0 && result == MATE_VFS_OK) {
		if (zstream->avail_out == 0) {
			MateVFSFileSize written;

			zstream->next_out = gzip_handle->buffer;
			result = mate_vfs_write (gzip_handle->parent_handle,
						  gzip_handle->buffer,
						  Z_BUFSIZE, &written);

			if (result != MATE_VFS_OK)
				break;

			zstream->avail_out += written;
		}

		z_result = deflate (zstream, Z_NO_FLUSH);
		result = result_from_z_result (z_result);
	}

	gzip_handle->crc = crc32 (gzip_handle->crc, buffer, num_bytes);

	*bytes_written = num_bytes - zstream->avail_in;

	return result;
}


static MateVFSResult 
do_get_file_info  (MateVFSMethod *method,
	           MateVFSURI *uri,
		   MateVFSFileInfo *file_info,
		   MateVFSFileInfoOptions options,
		   MateVFSContext *context) {
	MateVFSResult result;

	if (!VALID_URI(uri)) return MATE_VFS_ERROR_INVALID_URI;

	result = mate_vfs_get_file_info_uri(uri->parent, file_info, options);
	if(result == MATE_VFS_OK) {
		gint namelen = strlen(file_info->name);
		
		/* work out the name */
		/* FIXME bugzilla.eazel.com 2790: handle uppercase */
		if(namelen > 3 &&
				file_info->name[namelen-1] == 'z' &&
				file_info->name[namelen-2] == 'g' &&
				file_info->name[namelen-3] == '.')
			file_info->name[namelen-3] = '\0';

		/* we can't tell the size without uncompressing it */
		/*file_info->valid_fields &= ~MATE_VFS_FILE_INFO_FIELDS_SIZE;*/

		/* guess the mime type of the file inside */
		/* FIXME bugzilla.eazel.com 2791: guess mime based on contents */
		file_info->mime_type = g_strdup(mate_vfs_mime_type_from_name(file_info->name));
	}

	return result;
}



static gboolean
do_is_local (MateVFSMethod *method,
	     const MateVFSURI *uri)
{
	g_return_val_if_fail (uri != NULL, FALSE);

	return mate_vfs_uri_is_local (uri->parent);
}


/* Init.  */

MateVFSMethod *
vfs_module_init (const char *method_name, const char *args)
{
	return &method;
}

void
vfs_module_shutdown (MateVFSMethod *method)
{
}

