/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-inet-connection.h - Functions for creating and destroying Internet
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

#ifndef MATE_VFS_INET_CONNECTION_H
#define MATE_VFS_INET_CONNECTION_H

#include <libmatevfs/mate-vfs-cancellation.h>
#include <libmatevfs/mate-vfs-socket.h>
#include <libmatevfs/mate-vfs-socket-buffer.h>
#include <libmatevfs/mate-vfs-address.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MateVFSInetConnection MateVFSInetConnection;

MateVFSResult	 mate_vfs_inet_connection_create
					(MateVFSInetConnection **connection_return,
					 const gchar *host_name,
					 guint host_port,
					 MateVFSCancellation *cancellation);

MateVFSResult   mate_vfs_inet_connection_create_from_address
                                        (MateVFSInetConnection **connection_return,
					 MateVFSAddress         *address,
					 guint                    host_port,
					 MateVFSCancellation    *cancellation);

/* free the connection structure and close the socket */
void		 mate_vfs_inet_connection_destroy
					(MateVFSInetConnection *connection,
					 MateVFSCancellation *cancellation);

/* free the connection structure without closing the socket */
void		 mate_vfs_inet_connection_free
					(MateVFSInetConnection *connection,
					 MateVFSCancellation *cancellation);

MateVFSSocket * mate_vfs_inet_connection_to_socket
					(MateVFSInetConnection *connection);

MateVFSSocketBuffer *mate_vfs_inet_connection_to_socket_buffer
                                        (MateVFSInetConnection *connection);

int mate_vfs_inet_connection_get_fd    (MateVFSInetConnection *connection);

char *mate_vfs_inet_connection_get_ip  (MateVFSInetConnection *connection);

MateVFSAddress *mate_vfs_inet_connection_get_address
                                        (MateVFSInetConnection *connection);

#ifdef __cplusplus
}
#endif

#endif /* MATE_VFS_INET_CONNECTION_H */
