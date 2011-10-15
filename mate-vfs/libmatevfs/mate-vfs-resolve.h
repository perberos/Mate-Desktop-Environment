/* mate-vfs-resolve.h - Resolver API
 *
 * Copyright (C) 2004 Christian Kellner
 *
 * The Mate Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.

 * The Mate Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.

 * You should have received a copy of the GNU Library General Public
 * License along with the Mate Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef MATE_VFS_RESOLVE_H
#define MATE_VFS_RESOLVE_H

#include <libmatevfs/mate-vfs-result.h>
#include <libmatevfs/mate-vfs-address.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MateVFSResolveHandle_ MateVFSResolveHandle;

MateVFSResult mate_vfs_resolve              (const char              *hostname,
									  MateVFSResolveHandle  **handle);
gboolean       mate_vfs_resolve_next_address (MateVFSResolveHandle   *handle,
									  MateVFSAddress        **address);
void           mate_vfs_resolve_reset_to_beginning
                                              (MateVFSResolveHandle   *handle);
void           mate_vfs_resolve_free         (MateVFSResolveHandle   *handle);

#ifdef __cplusplus
}
#endif

#endif /* ! MATE_VFS_RESOLVE_H */
