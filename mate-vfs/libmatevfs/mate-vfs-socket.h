/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-socket.h
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

#ifndef MATE_VFS_SOCKET_H
#define MATE_VFS_SOCKET_H

#include <glib.h>
#include <libmatevfs/mate-vfs-cancellation.h>
#include <libmatevfs/mate-vfs-file-size.h>
#include <libmatevfs/mate-vfs-result.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * MateVFSSocket:
 *
 * An handle to a generic unbuffered socket connection established with
 * mate_vfs_socket_new().
 *
 * The specifics of the underlying socket implementation are hidden
 * inside the #MateVFSSocketImpl passed on construction.
 *
 * If you need buffered I/O, you will also have to create a
 * #MateVFSSocketBuffer.
 **/
typedef struct MateVFSSocket MateVFSSocket;

/**
 * MateVFSSocketReadFunc:
 * @connection: The socket connection.
 * @buffer: A connection buffer.
 * @bytes: The bytes to read.
 * @bytes_read_out: The bytes that were read (out).
 * @cancellation: A cancellation handle that allows clients to cancel the read operation.
 *
 * This is a generic prototype for a function that reads from a socket.
 *
 * This function is implemented by a #MateVFSSocketImpl, and it defines how data
 * should be written to a buffer using the mate_vfs_socket_read()
 * function which hides the socket implementation details.
 *
 * Returns: A #MateVFSResult signalling the result of the read operation.
 **/
typedef MateVFSResult (*MateVFSSocketReadFunc)  (gpointer connection,
						   gpointer buffer,
						   MateVFSFileSize bytes,
						   MateVFSFileSize *bytes_read_out,
						   MateVFSCancellation *cancellation);

/**
 * MateVFSSocketWriteFunc:
 * @connection: The socket connection.
 * @buffer: A connection buffer.
 * @bytes: The bytes to write.
 * @bytes_written_out: The bytes that were written.
 * @cancellation: A cancellation handle that allows clients to cancel the write operation.
 *
 * This is a generic prototype for a function that writes to a socket.
 *
 * This function is implemented by a #MateVFSSocketImpl, and it defines how data
 * should be written to a buffer using the mate_vfs_socket_write()
 * function which hides the socket implementation details.
 *
 * Returns: A #MateVFSResult signalling the result of the write operation.
 **/
typedef MateVFSResult (*MateVFSSocketWriteFunc) (gpointer connection,
						   gconstpointer buffer,
						   MateVFSFileSize bytes,
						   MateVFSFileSize *bytes_written_out,
						   MateVFSCancellation *cancellation);
/**
 * MateVFSSocketCloseFunc:
 * @cancellation: A cancellation handle that allows clients to cancel the write operation.
 *
 * This is a generic prototype for a function that closes a socket.
 *
 * This function is implemented by a #MateVFSSocketImpl, and it defines how an
 * open socket that was previously opened by mate_vfs_socket_new()
 * should be closed using the mate_vfs_socket_set_timeout() function which
 * hides the socket implementation details.
 **/
typedef void           (*MateVFSSocketCloseFunc) (gpointer connection,
						   MateVFSCancellation *cancellation);

/**
 * MateVFSSocketSetTimeoutFunc:
 * @cancellation: A cancellation handle that allows clients to cancel the write operation.
 *
 * This is a generic prototype for a function that sets a socket timeout.
 *
 * This function is implemented by a #MateVFSSocketImpl, and it defines how
 * a socket timeout should be set using
 * should be closed by the mate_vfs_socket_close() function which
 * hides the socket implementation details.
 *
 * Returns: A #MateVFSResult signalling the result of the write operation.
 **/
typedef MateVFSResult (*MateVFSSocketSetTimeoutFunc) (gpointer connection,
							GTimeVal *timeout,
							MateVFSCancellation *cancellation);

/**
 * MateVFSSocketImpl:
 * @read: A #MateVFSSocketReadFunc function used for reading from a socket.
 * @write: A #MateVFSSocketWriteFunc function used for writing to a socket.
 * @close: A #MateVFSSocketCloseFunc function used for closing an open socket.
 * @set_timeout: A #MateVFSSocketSetTimeoutFunc function used for setting a socket's timeout.
 *
 * An implementation of a generic socket (i.e. of #MateVFSSocket)
 * encapsulating the details of how socket I/O works.
 *
 * Please refer to #MateVFSSSL for a sample implementation of this interface.
 **/
typedef struct {
  MateVFSSocketReadFunc read;
  MateVFSSocketWriteFunc write;
  MateVFSSocketCloseFunc close;
  MateVFSSocketSetTimeoutFunc set_timeout;
} MateVFSSocketImpl;


MateVFSSocket* mate_vfs_socket_new     (MateVFSSocketImpl *impl,
					  void               *connection);
MateVFSResult  mate_vfs_socket_write   (MateVFSSocket     *socket,
					  gconstpointer       buffer,
					  int                 bytes,
					  MateVFSFileSize   *bytes_written,
					  MateVFSCancellation *cancellation);
MateVFSResult  mate_vfs_socket_close   (MateVFSSocket     *socket,
					  MateVFSCancellation *cancellation);
MateVFSResult  mate_vfs_socket_read    (MateVFSSocket     *socket,
					  gpointer            buffer,
					  MateVFSFileSize    bytes,
					  MateVFSFileSize   *bytes_read,
					  MateVFSCancellation *cancellation);
MateVFSResult  mate_vfs_socket_set_timeout
					 (MateVFSSocket *socket,
					  GTimeVal *timeout,
					  MateVFSCancellation *cancellation);
void            mate_vfs_socket_free   (MateVFSSocket *socket);
#ifdef __cplusplus
}
#endif

#endif /* MATE_VFS_SOCKET_H */
