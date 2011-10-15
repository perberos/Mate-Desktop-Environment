/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-context.h - context VFS modules can use to communicate with mate-vfs proper

   Copyright (C) 1999, 2001 Free Software Foundation

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

   Author: Havoc Pennington <hp@redhat.com>
           Seth Nickell <snickell@stanford.edu>
*/

#ifndef MATE_VFS_CONTEXT_H
#define MATE_VFS_CONTEXT_H

#include <libmatevfs/mate-vfs-cancellation.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MateVFSContext MateVFSContext;

MateVFSContext* mate_vfs_context_new                   (void);
void             mate_vfs_context_free                  (MateVFSContext *ctx);

/* To be really thread-safe, these need to return objects with an increased
   refcount; however they don't, only one thread at a time
   can use MateVFSContext */

MateVFSCancellation*
                 mate_vfs_context_get_cancellation      (const MateVFSContext *ctx);

/* Convenience - both of these accept a NULL context object */
#define          mate_vfs_context_check_cancellation(x) (mate_vfs_cancellation_check((x) ? mate_vfs_context_get_cancellation((x)) : NULL))

const MateVFSContext *mate_vfs_context_peek_current	  (void);
gboolean	       mate_vfs_context_check_cancellation_current (void);

#ifdef __cplusplus
}
#endif

#endif /* MATE_VFS_CONTEXT_H */
