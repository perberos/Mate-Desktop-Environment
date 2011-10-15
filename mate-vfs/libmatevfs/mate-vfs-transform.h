/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-transform.h - transform function declarations

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

   Author: Ettore Perazzoli <ettore@gnu.org>
           Seth Nickell <snickell@stanford.edu>
*/

#ifndef MATE_VFS_TRANSFORM_H
#define MATE_VFS_TRANSFORM_H

#include <libmatevfs/mate-vfs-result.h>
#include <libmatevfs/mate-vfs-context.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MateVFSTransform MateVFSTransform;

typedef MateVFSTransform * (* MateVFSTransformInitFunc) (const char        *method_name,
							   const char        *config_args);

typedef MateVFSResult (* MateVFSTransformFunc)          (MateVFSTransform *transform,
							   const char        *old_uri,
							   char             **new_uri,
							   MateVFSContext   *context);

struct MateVFSTransform {
	MateVFSTransformFunc transform;
};

#ifdef __cplusplus
}
#endif

#endif
