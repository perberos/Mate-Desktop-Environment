/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-socket-buffer.h
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

#ifndef MATE_VFS_SOCKET_BUFFER_H
#define MATE_VFS_SOCKET_BUFFER_H

#include "mate-vfs-socket.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * MateVFSSocketBuffer:
 *
 * A handle to a socket buffer. A socket buffer is a temporary in-memory storage for data
 * that is read from or written to a #MateVFSSocket.
 **/
typedef struct MateVFSSocketBuffer MateVFSSocketBuffer;


MateVFSSocketBuffer* mate_vfs_socket_buffer_new      (MateVFSSocket       *socket);
MateVFSResult        mate_vfs_socket_buffer_destroy  (MateVFSSocketBuffer *socket_buffer,
							gboolean              close_socket,
							MateVFSCancellation *cancellation);
MateVFSResult        mate_vfs_socket_buffer_read     (MateVFSSocketBuffer *socket_buffer,
							gpointer              buffer,
							MateVFSFileSize      bytes,
							MateVFSFileSize     *bytes_read,
							MateVFSCancellation *cancellation);
MateVFSResult        mate_vfs_socket_buffer_read_until (MateVFSSocketBuffer *socket_buffer,
							gpointer buffer,
							MateVFSFileSize bytes,
							gconstpointer boundary,
							MateVFSFileSize boundary_len,
							MateVFSFileSize *bytes_read,
							gboolean *got_boundary,
							MateVFSCancellation *cancellation);
MateVFSResult        mate_vfs_socket_buffer_peekc    (MateVFSSocketBuffer *socket_buffer,
							char                 *character,
							MateVFSCancellation *cancellation);
MateVFSResult        mate_vfs_socket_buffer_write    (MateVFSSocketBuffer *socket_buffer,
							gconstpointer         buffer,
							MateVFSFileSize      bytes,
							MateVFSFileSize     *bytes_written,
							MateVFSCancellation *cancellation);
MateVFSResult        mate_vfs_socket_buffer_flush    (MateVFSSocketBuffer *socket_buffer,
							MateVFSCancellation *cancellation);

#ifdef __cplusplus
}
#endif

#endif
