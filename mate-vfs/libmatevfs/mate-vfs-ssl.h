/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* mate-vfs-ssl.h
 *
 * Copyright (C) 2001 Ian McKellar
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
/*
 * Authors: Ian McKellar <yakk@yakk.net>
 */

#ifndef MATE_VFS_SSL_H
#define MATE_VFS_SSL_H

#include <libmatevfs/mate-vfs-socket.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MateVFSSSL MateVFSSSL;

gboolean        mate_vfs_ssl_enabled        (void);
/* FIXME: add *some* kind of cert verification! */
MateVFSResult  mate_vfs_ssl_create         (MateVFSSSL **handle_return,
		                              const char *host,
		                              unsigned int port,
					      MateVFSCancellation *cancellation);
MateVFSResult  mate_vfs_ssl_create_from_fd (MateVFSSSL **handle_return,
					      gint fd,
					      MateVFSCancellation *cancellation);
MateVFSResult  mate_vfs_ssl_read           (MateVFSSSL *ssl,
					      gpointer buffer,
					      MateVFSFileSize bytes,
				      	      MateVFSFileSize *bytes_read,
					      MateVFSCancellation *cancellation);
MateVFSResult  mate_vfs_ssl_write          (MateVFSSSL *ssl,
					      gconstpointer buffer,
					      MateVFSFileSize bytes,
					      MateVFSFileSize *bytes_written,
					      MateVFSCancellation *cancellation);
void            mate_vfs_ssl_destroy        (MateVFSSSL *ssl,
					      MateVFSCancellation *cancellation);
MateVFSResult  mate_vfs_ssl_set_timeout    (MateVFSSSL *ssl,
					      GTimeVal *timeout,
					      MateVFSCancellation *cancellation);
MateVFSSocket *mate_vfs_ssl_to_socket      (MateVFSSSL *ssl);

#ifdef __cplusplus
}
#endif

#endif /* MATE_VFS_SSL_H */
