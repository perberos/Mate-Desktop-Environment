/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* mate-vfs-find-directory.h - Special directory location functions for
   the MATE Virtual File System.

   Copyright (C) 2000 Eazel, Inc.
   Copyright (C) 2001 Free Software Foundation

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

   Authors: Pavel Cisler <pavel@eazel.com>
            Seth Nickell <snickell@stanford.edu>
*/

#ifndef MATE_VFS_FIND_DIRECTORY_H
#define MATE_VFS_FIND_DIRECTORY_H

#include <libmatevfs/mate-vfs-result.h>
#include <libmatevfs/mate-vfs-uri.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * MateVFSFindDirectoryKind:
 * @MATE_VFS_DIRECTORY_KIND_DESKTOP: Desktop directory.
 * @MATE_VFS_DIRECTORY_KIND_TRASH: Trash directory.
 *
 * Specifies what directory mate_vfs_find_directory() or
 * mate_vfs_find_directory_cancellable() should find.
 **/
typedef enum {
	MATE_VFS_DIRECTORY_KIND_DESKTOP = 1000,
	MATE_VFS_DIRECTORY_KIND_TRASH = 1001
} MateVFSFindDirectoryKind;

MateVFSResult mate_vfs_find_directory (MateVFSURI                *near_uri,
					 MateVFSFindDirectoryKind   kind,
					 MateVFSURI               **result,
					 gboolean                    create_if_needed,
					 gboolean                    find_if_needed,
					 guint                       permissions);

#ifdef __cplusplus
}
#endif

#endif
