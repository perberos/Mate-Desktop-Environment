/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
 * mate-vfs-mime-sniff-buffer.c
 * Utility for implementing mate_vfs_mime_type_from_magic, and other
 * mime-type sniffing calls.
 *
 * Copyright (C) 2000 Eazel, Inc.
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
#include "mate-vfs-mime-sniff-buffer.h"

#include "mate-vfs-handle.h"
#include "mate-vfs-mime-sniff-buffer-private.h"
#include "mate-vfs-ops.h"
#include <string.h>

static MateVFSResult
handle_seek_glue (gpointer context, MateVFSSeekPosition whence, 
	MateVFSFileOffset offset)
{
	MateVFSHandle *handle = (MateVFSHandle *)context;
	return mate_vfs_seek (handle, whence, offset);
}

static MateVFSResult
handle_read_glue (gpointer context, gpointer buffer, 
	MateVFSFileSize bytes, MateVFSFileSize *bytes_read)
{
	MateVFSHandle *handle = (MateVFSHandle *)context;
	return mate_vfs_read (handle, buffer, bytes, bytes_read);
}

MateVFSMimeSniffBuffer *
_mate_vfs_mime_sniff_buffer_new_from_handle (MateVFSHandle *file)
{
	MateVFSMimeSniffBuffer *result;

	result = g_new0 (MateVFSMimeSniffBuffer, 1);
	result->owning = TRUE;
	result->context = file;
	result->seek = handle_seek_glue;
	result->read = handle_read_glue;

	return result;
}

MateVFSMimeSniffBuffer	*
_mate_vfs_mime_sniff_buffer_new_generic (MateVFSSniffBufferSeekCall seek_callback, 
					 MateVFSSniffBufferReadCall read_callback,
					 gpointer context)
{
	MateVFSMimeSniffBuffer	* result;
	
	result = g_new0 (MateVFSMimeSniffBuffer, 1);

	result->owning = TRUE;
	result->seek = seek_callback;
	result->read = read_callback;
	result->context = context;

	return result;
}

MateVFSMimeSniffBuffer * 
_mate_vfs_mime_sniff_buffer_new_from_memory (const guchar *buffer, 
					     gssize buffer_length)
{
	MateVFSMimeSniffBuffer *result;

	result = g_new0 (MateVFSMimeSniffBuffer, 1);
	result->owning = TRUE;
	result->buffer = g_malloc (buffer_length);
	result->buffer_length = buffer_length;
	memcpy (result->buffer, buffer, buffer_length);
	result->read_whole_file = TRUE;

	return result;
}

MateVFSMimeSniffBuffer	*
mate_vfs_mime_sniff_buffer_new_from_existing_data (const guchar *buffer, 
					 	    gssize buffer_length)
{
	MateVFSMimeSniffBuffer *result;

	result = g_new0 (MateVFSMimeSniffBuffer, 1);
	result->owning = FALSE;
	result->buffer = (guchar *)buffer;
	result->buffer_length = buffer_length;
	result->read_whole_file = TRUE;

	return result;
}

void
mate_vfs_mime_sniff_buffer_free (MateVFSMimeSniffBuffer *buffer)
{
	if (buffer->owning)
		g_free (buffer->buffer);
	g_free (buffer);
}

enum {
	MATE_VFS_SNIFF_BUFFER_INITIAL_CHUNK = 256,
	MATE_VFS_SNIFF_BUFFER_MIN_CHUNK = 128
};

MateVFSResult
_mate_vfs_mime_sniff_buffer_get (MateVFSMimeSniffBuffer *buffer,
				 gssize size)
{
	MateVFSResult result;
	MateVFSFileSize bytes_to_read, bytes_read;

	/* check to see if we already have enough data */
	if (buffer->buffer_length >= size) {
		return MATE_VFS_OK;
	}

	/* if we've read the whole file, don't try to read any more */
	if (buffer->read_whole_file) {
		return MATE_VFS_ERROR_EOF;
	}

	/* figure out how much to read */
	bytes_to_read = size - buffer->buffer_length;

	/* don't bother to read less than this */
	if (bytes_to_read < MATE_VFS_SNIFF_BUFFER_MIN_CHUNK) {
		bytes_to_read = MATE_VFS_SNIFF_BUFFER_MIN_CHUNK;
	}

	/* make room in buffer for new data */
	buffer->buffer = g_realloc (buffer->buffer,
				    buffer->buffer_length + bytes_to_read);

	/* read in more data */
	result = (* buffer->read) (buffer->context, 
				   buffer->buffer + buffer->buffer_length,
				   bytes_to_read,
				   &bytes_read);
	if (result == MATE_VFS_ERROR_EOF) {
		buffer->read_whole_file = TRUE;
	} else if (result != MATE_VFS_OK) {
		return result;
	}
	buffer->buffer_length += bytes_read;

	/* check to see if we have enough data */
	if (buffer->buffer_length >= size) {
		return MATE_VFS_OK;
	}

	/* not enough data */
	return MATE_VFS_ERROR_EOF;
}

/*
 * mate_vfs_get_mime_type_for_buffer:
 * @buffer: a sniff buffer referencing either a file or data in memory
 *
 * This routine uses a magic database to guess the mime type of the
 * data represented by @buffer.
 *
 * Returns a pointer to an internal copy of the mime-type for @buffer.
 */
const char *
mate_vfs_get_mime_type_for_buffer (MateVFSMimeSniffBuffer *buffer)
{
	return _mate_vfs_get_mime_type_internal (buffer, NULL, FALSE);
}
