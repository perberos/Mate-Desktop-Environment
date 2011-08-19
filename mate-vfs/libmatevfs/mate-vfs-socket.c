/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-socket.c
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
#include "mate-vfs-socket.h"

#include <glib.h>

struct MateVFSSocket {
	MateVFSSocketImpl *impl;
	gpointer connection;
};


/**
 * mate_vfs_socket_new:
 * @impl: an implementation of socket, e.g. #MateVFSSSL.
 * @connection: pointer to a connection object used by @impl to track.
 * state (the exact nature of @connection varies from implementation to
 * implementation).
 *
 * Creates a new #MateVFSSocket using the specific implementation
 * @impl.
 *
 * Return value: a newly created socket.
 */
MateVFSSocket* mate_vfs_socket_new (MateVFSSocketImpl *impl, 
				      void               *connection) 
{
	MateVFSSocket *socket;
	
	socket = g_new0 (MateVFSSocket, 1);
	socket->impl = impl;
	socket->connection = connection; 
	
	return socket;
}

/**
 * mate_vfs_socket_write:
 * @socket: socket to write data to.
 * @buffer: data to write to the socket.
 * @bytes: number of bytes from @buffer to write to @socket.
 * @bytes_written: pointer to a #MateVFSFileSize, will contain
 * the number of bytes actually written to the @socket on return.
 * @cancellation: optional cancellation object.
 *
 * Write @bytes bytes of data from @buffer to @socket.
 *
 * Return value: #MateVFSResult indicating the success of the operation.
 */
MateVFSResult  
mate_vfs_socket_write (MateVFSSocket *socket, 
			gconstpointer buffer,
			int bytes, 
			MateVFSFileSize *bytes_written,
			MateVFSCancellation *cancellation)
{
	if (mate_vfs_cancellation_check (cancellation)) {

		if (bytes_written != NULL) {
			*bytes_written = 0;
		}
		
		return MATE_VFS_ERROR_CANCELLED;
	}
	
	return socket->impl->write (socket->connection,
				    buffer, bytes, bytes_written,
				    cancellation);
}

/**
 * mate_vfs_socket_close:
 * @socket: the socket to be closed.
 * @cancellation: optional cancellation object.
 *
 * Close @socket, freeing any resources it may be using.
 *
 * Return value: #MateVFSResult indicating the success of the operation.
 */
MateVFSResult  
mate_vfs_socket_close (MateVFSSocket *socket,
			MateVFSCancellation *cancellation)
{
	socket->impl->close (socket->connection, cancellation);
	g_free (socket);
	return MATE_VFS_OK;
}

/**
 * mate_vfs_socket_read:
 * @socket: socket to read data from.
 * @buffer: allocated buffer of at least @bytes bytes to be read into.
 * @bytes: number of bytes to read from @socket into @buffer.
 * @bytes_read: pointer to a #MateVFSFileSize, will contain
 * the number of bytes actually read from the socket on return.
 * @cancellation: optional cancellation object.
 *
 * Read @bytes bytes of data from the @socket into @buffer.
 *
 * Return value: #MateVFSResult indicating the success of the operation.
 */
MateVFSResult  
mate_vfs_socket_read  (MateVFSSocket *socket, 
			gpointer buffer, 
			MateVFSFileSize bytes,
			MateVFSFileSize *bytes_read,
			MateVFSCancellation *cancellation)
{
	if (mate_vfs_cancellation_check (cancellation)) {

		if (bytes_read != NULL) {
			*bytes_read = 0;
		}
		
		return MATE_VFS_ERROR_CANCELLED;
	}
	
	return socket->impl->read (socket->connection,
				   buffer, bytes, bytes_read,
				   cancellation);
}

/**
 * mate_vfs_socket_set_timeout:
 * @socket: socket to set the timeout of.
 * @timeout: the timeout.
 * @cancellation: optional cancellation object.
 *
 * Set a timeout of @timeout. If @timeout is %NULL, following operations
 * will block indefinitely).
 *
 * Note if you set @timeout to 0 (means tv_sec and tv_usec are both 0)
 * every following operation will return immediately. (This can be used
 * for polling.)
 *
 * Return value: #MateVFSResult indicating the success of the operation.
 *
 * Since: 2.8
 */

MateVFSResult
mate_vfs_socket_set_timeout (MateVFSSocket *socket,
			      GTimeVal *timeout,
			      MateVFSCancellation *cancellation)
{
	return socket->impl->set_timeout (socket->connection,
					  timeout,
					  cancellation);
}


/**
 * mate_vfs_socket_free:
 * @socket: The #MateVFSSocket you want to free. 
 * 
 * Frees the memory allocated for @socket, but does
 * not call any #MateVFSSocketImpl function.
 *
 * Since: 2.8
 */
void
mate_vfs_socket_free (MateVFSSocket *socket)
{
	g_return_if_fail (socket != NULL);
	
	g_free (socket);
}
