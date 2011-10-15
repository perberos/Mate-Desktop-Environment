/* mate-vfs-address.h - Address functions

   Copyright (C) 2004 Christian Kellner

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

*/

#ifndef MATE_VFS_ADDRESS_H
#define MATE_VFS_ADDRESS_H

#include <libmatevfs/mate-vfs-result.h>

#include <glib.h>
#include <glib-object.h>
#ifndef G_OS_WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#else
#include <winsock2.h>
#undef interface
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MATE_VFS_TYPE_ADDRESS  (mate_vfs_address_get_type ())

typedef struct _MateVFSAddress MateVFSAddress;

GType            mate_vfs_address_get_type          (void);

MateVFSAddress *mate_vfs_address_new_from_string   (const char      *address);
MateVFSAddress *mate_vfs_address_new_from_ipv4     (guint32          ipv4_address);
MateVFSAddress *mate_vfs_address_new_from_sockaddr (struct sockaddr *sa,
						      int              len);

int              mate_vfs_address_get_family_type   (MateVFSAddress *address);
char *           mate_vfs_address_to_string         (MateVFSAddress *address);
guint32          mate_vfs_address_get_ipv4          (MateVFSAddress *address);
struct sockaddr *mate_vfs_address_get_sockaddr      (MateVFSAddress *address,
						      guint16          port,
						      int             *len);

gboolean         mate_vfs_address_equal             (const MateVFSAddress *a,
						      const MateVFSAddress *b);

gboolean         mate_vfs_address_match             (const MateVFSAddress *a,
						      const MateVFSAddress *b,
						      guint             prefix);

MateVFSAddress *mate_vfs_address_dup               (MateVFSAddress *address);
void             mate_vfs_address_free              (MateVFSAddress *address);

#ifdef __cplusplus
}
#endif

#endif
