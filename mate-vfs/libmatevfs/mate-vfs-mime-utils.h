/*
 * Copyright (C) 1997-2001 Free Software Foundation
 * Copyright (C) 2000, 2001 Eazel, Inc.
 * All rights reserved.
 *
 * This file is part of the Mate Library.
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

#ifndef MATE_VFS_MIME_UTILS_H
#define MATE_VFS_MIME_UTILS_H

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * MATE_VFS_MIME_TYPE_UNKNOWN:
 *
 * The value returned for the MIME type when a file did
 * not match any entries in the MIME database. May be
 * treated as a file of an unknown type.
 **/
#define MATE_VFS_MIME_TYPE_UNKNOWN "application/octet-stream"

/**
 * MateVFSMimeEquivalence:
 * @MATE_VFS_MIME_UNRELATED: The two MIME types are not related.
 * @MATE_VFS_MIME_IDENTICAL: The two MIME types are identical.
 * @MATE_VFS_MIME_PARENT: One of the two MIME types is a parent of the other one.
 * 			   Note that this relationship is transient, i.e. if
 * 			   %a is a parent of %b and %b is a parent of %c,
 * 			   %a is also considered a parent of %c.
 *
 * Describes the possible relationship between two MIME types, returned by
 * mate_vfs_mime_type_get_equivalence().
 */
typedef enum {
  MATE_VFS_MIME_UNRELATED,
  MATE_VFS_MIME_IDENTICAL,
  MATE_VFS_MIME_PARENT
} MateVFSMimeEquivalence;

MateVFSMimeEquivalence mate_vfs_mime_type_get_equivalence   (const char    *mime_type,
							       const char    *base_mime_type);
gboolean                mate_vfs_mime_type_is_equal          (const char    *a,
							       const char    *b);

const char             *mate_vfs_get_mime_type_for_name      (const char *filename);
const char             *mate_vfs_get_mime_type_for_data      (gconstpointer  data,
							       int            data_size);
const char             *mate_vfs_get_mime_type_for_name_and_data (const char    *filename,
							           gconstpointer  data,
							           gssize         data_size);

char                   *mate_vfs_get_mime_type               (const char    *text_uri);
char                   *mate_vfs_get_slow_mime_type          (const char    *text_uri);


#ifdef __cplusplus
}
#endif

#endif /* MATE_VFS_MIME_UTILS_H */
