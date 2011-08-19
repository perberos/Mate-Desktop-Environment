/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-socket-buffer.c
 *
 * Copyright (C) 2001 Seth Nickell
 * Copyright (C) 2001 Maciej Stachowiak
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
 *
 */
/*
 * Authors: Seth Nickell <snickell@stanford.edu>
 *          Maciej Stachowiak <mjs@noisehavoc.org>
 *          (reverse-engineered from code by Ian McKellar <yakk@yakk.net>)
 */

#include <config.h>
#include "mate-vfs-socket-buffer.h"

#include <string.h>
#include <glib.h>


#define BUFFER_SIZE 4096

struct Buffer {
	gchar data[BUFFER_SIZE];
	guint offset;
	guint byte_count;
	MateVFSResult last_error;
};
typedef struct Buffer Buffer;


struct  MateVFSSocketBuffer {
        MateVFSSocket *socket;
	Buffer input_buffer;
	Buffer output_buffer;
};


static void
buffer_init (Buffer *buffer)
{
	buffer->byte_count = 0;
	buffer->offset = 0;
	buffer->last_error = MATE_VFS_OK;
}

/**
 * mate_vfs_socket_buffer_new:
 * @socket: socket to be buffered.
 *
 * Create a socket buffer around @socket. A buffered
 * socket allows data to be poked at without reading it
 * as it will be buffered. A future read will retrieve
 * the data again.
 *
 * Return value: a newly allocated #MateVFSSocketBuffer.
 */
MateVFSSocketBuffer*  
mate_vfs_socket_buffer_new (MateVFSSocket *socket)
{
	MateVFSSocketBuffer *socket_buffer;

	g_return_val_if_fail (socket != NULL, NULL);

	socket_buffer = g_new (MateVFSSocketBuffer, 1);
	socket_buffer->socket = socket;

	buffer_init (&socket_buffer->input_buffer);
	buffer_init (&socket_buffer->output_buffer);

	return socket_buffer;
}

/**
 * mate_vfs_socket_buffer_destroy:
 * @socket_buffer: buffered socket to destroy.
 * @close_socket: if %TRUE, the socket being buffered will be closed.
 * @cancellation: handle allowing cancellation of the operation.
 *
 * Free the socket buffer.
 *
 * Return value: #MateVFSResult indicating the success of the operation.
 */
MateVFSResult   
mate_vfs_socket_buffer_destroy  (MateVFSSocketBuffer *socket_buffer, 
				  gboolean close_socket,
				  MateVFSCancellation *cancellation)
{
	mate_vfs_socket_buffer_flush (socket_buffer, cancellation);

        if (close_socket) {
		mate_vfs_socket_close (socket_buffer->socket, cancellation);
	}
	g_free (socket_buffer);
	return MATE_VFS_OK;
}




static gboolean
refill_input_buffer (MateVFSSocketBuffer *socket_buffer,
		     MateVFSCancellation *cancellation)
{
	Buffer *input_buffer;
	MateVFSResult result;
	MateVFSFileSize bytes_read;
	char *data_pos;

	input_buffer = &socket_buffer->input_buffer;

	if (input_buffer->last_error != MATE_VFS_OK) {
		return FALSE;
	}


	/* If there is data left in the buffer move it to the front */
	if (input_buffer->offset > 0) {
		data_pos = &(input_buffer->data[input_buffer->offset]);
		memmove (input_buffer->data, data_pos, input_buffer->byte_count);
		input_buffer->offset = 0;
	}
	
	result = mate_vfs_socket_read (socket_buffer->socket,
					input_buffer->data + input_buffer->byte_count,
					BUFFER_SIZE - input_buffer->byte_count,
					&bytes_read,
					cancellation);

	
	if (result != MATE_VFS_OK) {
		input_buffer->last_error = result;
		return FALSE;
	}

	input_buffer->byte_count += bytes_read;

	return TRUE;
}

/**
 * mate_vfs_socket_buffer_read:
 * @socket_buffer: buffered socket to read data from.
 * @buffer: allocated buffer of at least @bytes bytes to be read into.
 * @bytes: number of bytes to read from @socket_buffer into @buffer.
 * @bytes_read: pointer to a #MateVFSFileSize, will contain
 * the number of bytes actually read from the @socket_buffer on return.
 * @cancellation: handle allowing cancellation of the operation.
 *
 * Read @bytes bytes of data from the @socket into @socket_buffer.
 *
 * Return value: #MateVFSResult indicating the success of the operation.
 */
MateVFSResult   
mate_vfs_socket_buffer_read (MateVFSSocketBuffer *socket_buffer,
			      gpointer buffer,
			      MateVFSFileSize bytes,
			      MateVFSFileSize *bytes_read,
			      MateVFSCancellation *cancellation)
{
	Buffer *input_buffer;
	MateVFSResult result;
	MateVFSFileSize n;
	
	g_return_val_if_fail (socket_buffer != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (buffer != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);
	
	/* Quote from UNIX 98:
	   "If nbyte is 0, read() will return 0 and have no other results."
	*/
	if (bytes == 0) {
		if (bytes_read != NULL);
			*bytes_read = 0;

		return MATE_VFS_OK;
	}
		
	input_buffer = &socket_buffer->input_buffer;

	result = MATE_VFS_OK;

	if (input_buffer->byte_count == 0) {
		if (! refill_input_buffer (socket_buffer, cancellation)) {
			/* The buffer is empty but we had an error last time we
			   filled it, so we report the error.  */
			result = input_buffer->last_error;
			input_buffer->last_error = MATE_VFS_OK;
		}
	}

	n = 0;
	
	if (input_buffer->byte_count != 0) {
		n = MIN (bytes, input_buffer->byte_count);
		memcpy (buffer, input_buffer->data + input_buffer->offset, n);
		input_buffer->byte_count -= n;
		input_buffer->offset += n;
	}
	
	if (bytes_read != NULL) {
		*bytes_read = n;
	}
	
	return result;
}

/**
 * mate_vfs_socket_buffer_read_until:
 * @socket_buffer: buffered socket to read data from.
 * @buffer: allocated buffer of at least @bytes bytes to be read into.
 * @bytes: maximum number of bytes to read from @socket_buffer into @buffer.
 * @boundary: the boundary until which is read.
 * @boundary_len: the length of the @boundary.
 * @bytes_read: pointer to a #MateVFSFileSize, will contain
 * the number of bytes actually read from the @socket_buffer on return.
 * @got_boundary: pointer to a #gboolean  which will be %TRUE if the boundary
 * was found or %FALSE otherwise.
 * @cancellation: handle allowing cancellation of the operation.
 *
 * Read up to @bytes bytes of data from the @socket_buffer into @buffer 
 * until boundary is reached. @got_boundary will be set accordingly.
 *
 * Note that if @bytes is smaller than @boundary_len there is no way
 * to detected the boundary! So if you want to make sure that every boundary
 * is found (in a loop maybe) assure that @bytes is at least as big as 
 * @boundary_len.
 *
 * Return value: #MateVFSResult indicating the success of the operation.
 *
 * Since: 2.8
 */
MateVFSResult
mate_vfs_socket_buffer_read_until (MateVFSSocketBuffer *socket_buffer,
				    gpointer buffer,
				    MateVFSFileSize bytes,
				    gconstpointer boundary,
				    MateVFSFileSize boundary_len,
				    MateVFSFileSize *bytes_read,
				    gboolean *got_boundary,
				    MateVFSCancellation *cancellation)
{
	Buffer *input_buffer;
	MateVFSResult result;
	MateVFSFileSize n, max_scan;
	char *iter, *start, *delim;

	g_return_val_if_fail (socket_buffer != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (buffer != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (boundary != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (got_boundary != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (boundary_len < BUFFER_SIZE, MATE_VFS_ERROR_TOO_BIG);

	*got_boundary = FALSE;

	/* Quote from UNIX 98:
	   "If nbyte is 0, read() will return 0 and have no other results."
	*/
	if (bytes == 0) {
		if (bytes_read != NULL)
			*bytes_read = 0;

		return MATE_VFS_OK;
	}

	input_buffer = &socket_buffer->input_buffer;
	result = MATE_VFS_OK;
	
	/* we are looping here to catch the case where we are close
	 * to eof and haveing less or equal bytes then boundary_len */
	while (input_buffer->byte_count <= boundary_len) {
		if (! refill_input_buffer (socket_buffer, cancellation)) {
			break;
		}
	}

	/* At this point we have either byte_count > boundary_len or
	 * we have and error during refill */

	n = 0;
	start = input_buffer->data + input_buffer->offset;
	max_scan = MIN (input_buffer->byte_count, bytes);

	/* if max_scan is greater then boundary_len do a scan (I) 
	 * otherwise we had an error during the loop above or bytes is
	 * too small. (II) We handle the case where boundary_len ==
	 * max_scan in (II). */

	if (max_scan > boundary_len) {

		delim = start + max_scan;
		for (iter = start; iter + boundary_len <= delim; iter++) {
			if (!memcmp (iter, boundary, boundary_len)) {
				*got_boundary = TRUE;
				/* We wanna have the boundary fetched */
				iter += boundary_len;
				break;
			}
		}

		/* Fetch data data until iter */
		n = iter - start;

	} else /* (II) */ {

		if (max_scan == boundary_len &&
			!memcmp (start, boundary, boundary_len)) {
			*got_boundary = TRUE;
		}

		n = max_scan;
	}

	if (n > 0) {

		memcpy (buffer, start, n);
		input_buffer->byte_count -= n;
		input_buffer->offset += n;
		/* queque up the fill buffer error if any
		 * until the buffer is flushed */
	} else {
		result = input_buffer->last_error;
		input_buffer->last_error = MATE_VFS_OK;
	}

	if (bytes_read != NULL) {
		*bytes_read = n;
	}

	return result;
}

/**
 * mate_vfs_socket_buffer_peekc:
 * @socket_buffer: the socket buffer to read from.
 * @character: pointer to a char, will contain a character on return from
 * a successful "peek".
 * @cancellation: handle allowing cancellation of the operation.
 *
 * Peek at the next character in @socket_buffer without actually reading
 * the character in. The next read will retrieve @c (as well as any following
 * data if requested).
 *
 * Return value: #MateVFSResult indicating the success of the operation.
 */
MateVFSResult
mate_vfs_socket_buffer_peekc (MateVFSSocketBuffer *socket_buffer,
			       gchar *character,
			       MateVFSCancellation *cancellation)
{
	MateVFSResult result;
	Buffer *input_buffer;

	g_return_val_if_fail (socket_buffer != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (character != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);

	input_buffer = &socket_buffer->input_buffer;
	result = MATE_VFS_OK;

	if (input_buffer->byte_count == 0) {
		if (!refill_input_buffer (socket_buffer, cancellation)) {
			/* The buffer is empty but we had an error last time we
			   filled it, so we report the error.  */
			result = input_buffer->last_error;
			input_buffer->last_error = MATE_VFS_OK;
		}
	}

	if (result == MATE_VFS_OK) {
		*character = *(input_buffer->data + input_buffer->offset);
	}

	return result;
}



static MateVFSResult
flush (MateVFSSocketBuffer *socket_buffer,
       MateVFSCancellation *cancellation)
{
	Buffer *output_buffer;
	MateVFSResult result;
	MateVFSFileSize bytes_written;

	output_buffer = &socket_buffer->output_buffer;

	while (output_buffer->byte_count > 0) {
		result = mate_vfs_socket_write (socket_buffer->socket, 
						 output_buffer->data,
						 output_buffer->byte_count,
						 &bytes_written,
						 cancellation);
		output_buffer->last_error = result;

		if (result != MATE_VFS_OK) {
			return result;
		}

		memmove (output_buffer->data,
			 output_buffer->data + bytes_written,
			 output_buffer->byte_count - bytes_written);
		output_buffer->byte_count -= bytes_written;
	}

	return MATE_VFS_OK;
}

/**
 * mate_vfs_socket_buffer_write:
 * @socket_buffer: buffered socket to write data to.
 * @buffer: data to write to the @socket_buffer.
 * @bytes: number of bytes to write from @buffer to @socket_buffer.
 * @bytes_written: pointer to a #MateVFSFileSize, will contain
 * the number of bytes actually written to the @socket_buffer on return.
 * @cancellation: handle allowing cancellation of the operation.
 *
 * Write @bytes bytes of data from @buffer to @socket_buffer.
 *
 * Return value: #MateVFSResult indicating the success of the operation.
 */ 
MateVFSResult   
mate_vfs_socket_buffer_write (MateVFSSocketBuffer *socket_buffer, 
			       gconstpointer buffer,
			       MateVFSFileSize bytes,
			       MateVFSFileSize *bytes_written,
			       MateVFSCancellation *cancellation)
{
	Buffer *output_buffer;
	MateVFSFileSize write_count;
	MateVFSResult result;
	const gchar *p;

	g_return_val_if_fail (socket_buffer != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (buffer != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (bytes_written != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);

	output_buffer = &socket_buffer->output_buffer;

	result = MATE_VFS_OK;

	p = buffer;
	write_count = 0;
	while (write_count < bytes) {
		if (output_buffer->byte_count < BUFFER_SIZE) {
			MateVFSFileSize n;

			n = MIN (BUFFER_SIZE - output_buffer->byte_count,
				 bytes - write_count);
			memcpy (output_buffer->data + output_buffer->byte_count,
				p, n);
			p += n;
			write_count += n;
			output_buffer->byte_count += n;
		}
		if (output_buffer->byte_count >= BUFFER_SIZE) {
			result = flush (socket_buffer, cancellation);
			if (result != MATE_VFS_OK) {
				break;
			}
		}
	}

	if (bytes_written != NULL) {
		*bytes_written = write_count;
	}
		
	return result;
}

/**
 * mate_vfs_socket_buffer_flush:
 * @socket_buffer: buffer to flush.
 * @cancellation: handle allowing cancellation of the operation.
 *
 * Write all outstanding data to @socket_buffer.
 *
 * Return value: #MateVFSResult indicating the success of the operation.
 */
MateVFSResult   
mate_vfs_socket_buffer_flush (MateVFSSocketBuffer *socket_buffer,
			       MateVFSCancellation *cancellation)
{
	g_return_val_if_fail (socket_buffer != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);

	return flush (socket_buffer, cancellation);
}



