/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-inet-connection.c - Functions for creating and destroying Internet
   connections.

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

   Author: Ettore Perazzoli <ettore@gnu.org> */

#include <config.h>
#include "mate-vfs-inet-connection.h"
#include "mate-vfs-private-utils.h"
#include "mate-vfs-resolve.h"

#include <errno.h>
#include <glib.h>
#include <string.h>
/* Keep <sys/types.h> above the network includes for FreeBSD. */
#include <sys/types.h>

#ifndef G_OS_WIN32
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#else
#include <winsock2.h>
#endif

#include <sys/time.h>

struct MateVFSInetConnection {

	MateVFSAddress *address;	
	guint sock;
	struct timeval *timeout;
};

/**
 * mate_vfs_inet_connection_create:
 * @connection_return: pointer to a pointer to a #MateVFSInetConnection, which will
 * contain an allocated #MateVFSInetConnection object on return.
 * @host_name: string indicating the host to establish an internet connection with.
 * @host_port: port number to connect to.
 * @cancellation: handle allowing cancellation of the operation.
 *
 * Creates a connection at @connection_return to @host_name using
 * port @port.
 *
 * Return value: #MateVFSResult indicating the success of the operation.
 */
MateVFSResult
mate_vfs_inet_connection_create (MateVFSInetConnection **connection_return,
				  const gchar             *host_name,
				  guint                    host_port,
				  MateVFSCancellation    *cancellation)
{
	MateVFSInetConnection *new;
	MateVFSResolveHandle *rh;
	MateVFSAddress *address;
	MateVFSResult res;
	gint sock, len, ret;
	struct sockaddr *saddr;

	g_return_val_if_fail (connection_return != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (host_name != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);
	g_return_val_if_fail (host_port != 0, MATE_VFS_ERROR_BAD_PARAMETERS);

	res = mate_vfs_resolve (host_name, &rh);

	if (res != MATE_VFS_OK)
		return res;

	sock = -1;
	
	while (mate_vfs_resolve_next_address (rh, &address)) {
		sock = socket (mate_vfs_address_get_family_type (address),
			       SOCK_STREAM, 0);
#ifdef G_OS_WIN32
		if (sock == SOCKET_ERROR) {
			_mate_vfs_map_winsock_error_to_errno ();
			sock = -1;
		}
#endif
		if (sock > -1) {
			saddr = mate_vfs_address_get_sockaddr (address,
								host_port,
								&len);
			ret = connect (sock, saddr, len);
			g_free (saddr);
			
			if (ret == 0)
				break;

			_MATE_VFS_SOCKET_CLOSE (sock);
			sock = -1;
		}

		mate_vfs_address_free (address);
		
	}

	mate_vfs_resolve_free (rh);
	
	if (sock < 0)
		return mate_vfs_result_from_errno ();
	
	new = g_new0 (MateVFSInetConnection, 1);
	new->address = address;
	new->sock = sock;

	_mate_vfs_socket_set_blocking (new->sock, FALSE);

	*connection_return = new;
	return MATE_VFS_OK;
}

/**
 * mate_vfs_inet_connection_create_from_address:
 * @connection_return: pointer to a pointer to a #MateVFSInetConnection, which will 
 * contain an allocated #MateVFSInetConnection object on return.
 * @address: a valid #MateVFSAddress.
 * @host_port: port number to connect to.
 * @cancellation: handle allowing cancellation of the operation.
 *
 * Creates a connection at @connection_return to @address using
 * port @port.
 *
 * Return value: #MateVFSResult indicating the success of the operation.
 *
 * Since: 2.8
 */
MateVFSResult
mate_vfs_inet_connection_create_from_address (MateVFSInetConnection **connection_return,
					       MateVFSAddress         *address,
					       guint                    host_port,
					       MateVFSCancellation    *cancellation)
{
	MateVFSInetConnection *new;
	gint sock, len, ret;
	struct sockaddr *saddr;
	
	sock = socket (mate_vfs_address_get_family_type (address),
		       SOCK_STREAM, 0);
#ifdef G_OS_WIN32
	if (sock == SOCKET_ERROR) {
		_mate_vfs_map_winsock_error_to_errno ();
		sock = -1;
	}
#endif
	if (sock < 0)
		return mate_vfs_result_from_errno ();
	
	saddr = mate_vfs_address_get_sockaddr (address,
						host_port,
						&len);
	
	ret = connect (sock, saddr, len);
	g_free (saddr);

	if (ret < 0) {
		_MATE_VFS_SOCKET_CLOSE (sock);
		return mate_vfs_result_from_errno ();
	}

		
	new = g_new0 (MateVFSInetConnection, 1);
	new->address = mate_vfs_address_dup (address);
	new->sock = sock;

	_mate_vfs_socket_set_blocking (new->sock, FALSE);

	*connection_return = new;
	return MATE_VFS_OK;
}


/**
 * mate_vfs_inet_connection_destroy:
 * @connection: connection to destroy.
 * @cancellation: handle for cancelling the operation.
 *
 * Closes/Destroys @connection.
 */
void
mate_vfs_inet_connection_destroy (MateVFSInetConnection *connection,
				   MateVFSCancellation   *cancellation)
{
	g_return_if_fail (connection != NULL);

	_MATE_VFS_SOCKET_CLOSE (connection->sock);
	
	mate_vfs_inet_connection_free (connection, cancellation);
}


/**
 * mate_vfs_inet_connection_free:
 * @connection: connection to free.
 * @cancellation: handle for cancelling the operation.
 *
 * Frees @connection without closing the socket.
 */
void
mate_vfs_inet_connection_free (MateVFSInetConnection *connection,
				MateVFSCancellation *cancellation)
{
	g_return_if_fail (connection != NULL);
	
	if (connection->timeout)
		g_free (connection->timeout);

	if (connection->address)
		mate_vfs_address_free (connection->address);

	g_free (connection);
	
}

/**
 * mate_vfs_inet_connection_close:
 * @connection: connection to close.
 * @cancellation: handle allowing cancellation of the operation.
 *
 * Closes @connection, freeing all used resources.
 * Same as mate_vfs_inet_connection_destroy().
 */
static void
mate_vfs_inet_connection_close (MateVFSInetConnection *connection,
				 MateVFSCancellation *cancellation)
{
	mate_vfs_inet_connection_destroy (connection, cancellation);
}

/**
 * mate_vfs_inet_connection_get_fd:
 * @connection: connection to get the file descriptor from.
 *
 * Retrieve the UNIX file descriptor corresponding to @connection.
 *
 * Return value: file descriptor.
 */
gint 
mate_vfs_inet_connection_get_fd (MateVFSInetConnection *connection)
{
	g_return_val_if_fail (connection != NULL, -1);
	return connection->sock;
}

/**
 * mate_vfs_inet_connection_get_ip:
 * @connection: connection to get the ip from.
 *
 * Retrieve the ip address of the other side of a connected @connection.
 *
 * Return value: string version of the ip.
 *
 * Since: 2.8
 */
char *
mate_vfs_inet_connection_get_ip (MateVFSInetConnection *connection)
{
	return mate_vfs_address_to_string (connection->address);
}

/**
 * mate_vfs_inet_connection_get_address:
 * @connection: connection to get the address from. 
 *
 * Retrieve the address of the other side of a connected @connection.
 * 
 * Return Value: a #MateVFSAddress containing the address.
 *
 * Since 2.8
 */
MateVFSAddress *
mate_vfs_inet_connection_get_address (MateVFSInetConnection *connection)
{
	return mate_vfs_address_dup (connection->address);
}

/* SocketImpl for InetConnections */

/**
 * mate_vfs_inet_connection_read:
 * @connection: connection to read data from.
 * @buffer: allocated buffer of at least @bytes bytes to be read into.
 * @bytes: number of bytes to read from socket into @buffer.
 * @bytes_read: pointer to a #MateVFSFileSize, will contain
 * the number of bytes actually read from the socket on return.
 * @cancellation: handle allowing cancellation of the operation.
 *
 * Read @bytes bytes of data from @connection into @buffer.
 *
 * Return value: a #MateVFSResult indicating the success of the operation.
 */
static MateVFSResult 
mate_vfs_inet_connection_read (MateVFSInetConnection *connection,
		                gpointer buffer,
		                MateVFSFileSize bytes,
		                MateVFSFileSize *bytes_read,
				MateVFSCancellation *cancellation)
{
	gint     read_val;
	fd_set   read_fds;
	int max_fd, cancel_fd;
	struct timeval timeout;

	cancel_fd = -1;
	
read_loop:
	read_val = _MATE_VFS_SOCKET_READ (connection->sock, buffer, bytes);
#ifdef G_OS_WIN32
	if (read_val == SOCKET_ERROR) {
		_mate_vfs_map_winsock_error_to_errno ();
		read_val = -1;
	}
#endif
	if (read_val == -1 && errno == EAGAIN) {

		FD_ZERO (&read_fds);
		FD_SET (connection->sock, &read_fds);
		max_fd = connection->sock;
	
		if (cancellation != NULL) {
			cancel_fd = mate_vfs_cancellation_get_fd (cancellation);
			FD_SET (cancel_fd, &read_fds);
			max_fd = MAX (max_fd, cancel_fd);
		}
		
		/* select modifies the timeval struct so set it every loop */
		if (connection->timeout != NULL) {
			timeout.tv_sec = connection->timeout->tv_sec;
			timeout.tv_usec = connection->timeout->tv_usec;
		}
		
		read_val = select (max_fd + 1, &read_fds, NULL, NULL,
				   connection->timeout ? &timeout : NULL);
#ifdef G_OS_WIN32
		if (read_val == SOCKET_ERROR) {
			_mate_vfs_map_winsock_error_to_errno ();
			read_val = -1;
		}
#endif
		if (read_val == 0) {
			return MATE_VFS_ERROR_TIMEOUT;
		} else if (read_val != -1) { 	
			
			if (cancel_fd != -1 && FD_ISSET (cancel_fd, &read_fds)) {
				return MATE_VFS_ERROR_CANCELLED;
			}
			
			if (FD_ISSET (connection->sock, &read_fds)) {
				goto read_loop;
			}

		}
	} 
	
	if (read_val == -1) {
		*bytes_read = 0;

		if (mate_vfs_cancellation_check (cancellation)) {
			return MATE_VFS_ERROR_CANCELLED;
		} 
		
		if (errno == EINTR) {
			goto read_loop;
		} else {
			return mate_vfs_result_from_errno ();
		}

	} else {
		*bytes_read = read_val;
	}

	return *bytes_read == 0 ? MATE_VFS_ERROR_EOF : MATE_VFS_OK;
}

/**
 * mate_vfs_inet_connection_write:
 * @connection: connection to write data to.
 * @buffer: data to write to the connection.
 * @bytes: number of bytes to write from @buffer to socket.
 * @bytes_written: pointer to a #MateVFSFileSize, will contain
 * the number of bytes actually written to the @connection on return.
 * @cancellation: handle allowing cancellation of the operation.
 *
 * Write @bytes bytes of data from @buffer to @connection.
 *
 * Return value: a #MateVFSResult indicating the success of the operation.
 */
static MateVFSResult 
mate_vfs_inet_connection_write (MateVFSInetConnection *connection,
			         gconstpointer buffer,
			         MateVFSFileSize bytes,
			         MateVFSFileSize *bytes_written,
				 MateVFSCancellation *cancellation)
{
	gint    write_val;
	fd_set  write_fds, *read_fds, pipe_fds;
	int max_fd, cancel_fd;
	struct timeval timeout;


	cancel_fd = -1;
	read_fds  = NULL;
	
write_loop:	
	write_val = _MATE_VFS_SOCKET_WRITE (connection->sock, buffer, bytes);
#ifdef G_OS_WIN32
	if (write_val == SOCKET_ERROR) {
		_mate_vfs_map_winsock_error_to_errno ();
		write_val = -1;
	}
#endif
	if (write_val == -1 && errno == EAGAIN) {
			
         	FD_ZERO (&write_fds);
		FD_SET (connection->sock, &write_fds);
		max_fd = connection->sock;

		if (cancellation != NULL) {
			cancel_fd = mate_vfs_cancellation_get_fd (cancellation);
			read_fds = &pipe_fds;
			FD_ZERO (read_fds);
			FD_SET (cancel_fd, read_fds);
			max_fd = MAX (max_fd, cancel_fd);
		}

		/* select modifies the timeval struct so set it every loop */
		if (connection->timeout != NULL) {
			timeout.tv_sec = connection->timeout->tv_sec;
			timeout.tv_usec = connection->timeout->tv_usec;
		}

		
		write_val = select (max_fd + 1, read_fds, &write_fds, NULL,
					connection->timeout ? &timeout : NULL);
#ifdef G_OS_WIN32
		if (write_val == SOCKET_ERROR) {
			_mate_vfs_map_winsock_error_to_errno ();
			write_val = -1;
		}
#endif
		if (write_val == 0) {
			return MATE_VFS_ERROR_TIMEOUT;
		} else if (write_val != -1) {

			if (cancel_fd != -1 && FD_ISSET (cancel_fd, read_fds)) {
				return MATE_VFS_ERROR_CANCELLED;
			}
			
			if (FD_ISSET (connection->sock, &write_fds)) {
				goto write_loop;
			}

		}
	}

	if (write_val == -1) {
		*bytes_written = 0;

	        if (mate_vfs_cancellation_check (cancellation)) {
			return MATE_VFS_ERROR_CANCELLED;
		}

		if (errno == EINTR) {
			goto write_loop;
		} else {
			return mate_vfs_result_from_errno ();
		}

	} else {
		*bytes_written = write_val;
		return MATE_VFS_OK;
	}
}

/**
 * mate_vfs_inet_connection_set_timeout:
 * @connection: connection to set the timeout of.
 * @timeout: timeout to set.
 * @cancellation: optional cancellation object.
 *
 * Set a timeout of @timeout. If @timeout is %NULL following operations
 * will block indefinitely).
 *
 * Note if you set @timeout to 0 (means tv_sec and tv_usec are both 0)
 * every following operation will return immediately. (This can be used
 * for polling.)
 *
 * Return value: a #MateVFSResult indicating the success of the operation.
 *
 * Since: 2.8
 */


static MateVFSResult
mate_vfs_inet_connection_set_timeout (MateVFSInetConnection *connection,
				       GTimeVal *timeout,
				       MateVFSCancellation *cancellation)
{
	if (timeout == NULL) {
		if (connection->timeout != NULL) {
			g_free (connection->timeout);
			connection->timeout = NULL;
		}
	} else {
		if (connection->timeout == NULL)
			connection->timeout = g_new0 (struct timeval, 1);

		connection->timeout->tv_sec  = timeout->tv_sec;
		connection->timeout->tv_usec = timeout->tv_usec;
	}
	
	return MATE_VFS_OK;
}

static MateVFSSocketImpl inet_connection_socket_impl = {
	(MateVFSSocketReadFunc)mate_vfs_inet_connection_read,
	(MateVFSSocketWriteFunc)mate_vfs_inet_connection_write,
	(MateVFSSocketCloseFunc)mate_vfs_inet_connection_close,
	(MateVFSSocketSetTimeoutFunc)mate_vfs_inet_connection_set_timeout
};

/**
 * mate_vfs_inet_connection_to_socket:
 * @connection: connection to be wrapped into a #MateVFSSocket.
 *
 * Wrap @connection inside a standard #MateVFSSocket for convenience.
 *
 * Return value: a newly created #MateVFSSocket around @connection.
 */
MateVFSSocket *
mate_vfs_inet_connection_to_socket (MateVFSInetConnection *connection)
{
	return mate_vfs_socket_new (&inet_connection_socket_impl, connection);
}

/**
 * mate_vfs_inet_connection_to_socket_buffer:
 * @connection: connection to be wrapped into a #MateVFSSocketBuffer.
 *
 * Wrap @connection inside a standard #MateVFSSocketBuffer for convenience.
 *
 * Return value: a newly created #MateVFSSocketBuffer around @connection.
 */
MateVFSSocketBuffer *
mate_vfs_inet_connection_to_socket_buffer (MateVFSInetConnection *connection)
{
	MateVFSSocket *socket;
	socket = mate_vfs_inet_connection_to_socket (connection);
	return mate_vfs_socket_buffer_new (socket);
}
