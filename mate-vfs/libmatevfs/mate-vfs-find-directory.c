/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-find-directory.c - Public utility functions for the MATE Virtual
   File System.

   Copyright (C) 2000 Eazel, Inc.

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
*/

#include <config.h>
#include "mate-vfs-find-directory.h"

#include "mate-vfs-cancellable-ops.h"

/**
 * mate_vfs_find_directory:
 * @near_uri: find a well known directory on the same volume as @near_uri.
 * @kind: kind of well known directory.
 * @result: newly created uri of the directory we found.
 * @create_if_needed: if directory we are looking for does not exist, try to create it.
 * @find_if_needed: if we don't know where directory is yet, look for it.
 * @permissions: if creating, use these permissions.
 * 
 * Used to return well known directories such as Trash, Desktop, etc. from different
 * file systems.
 * 
 * There is quite a complicated logic behind finding/creating a Trash directory
 * and you need to be aware of some implications:
 * Finding the Trash the first time when using the file method may be pretty 
 * expensive. A cache file is used to store the location of that Trash file
 * for next time.
 * If @create_if_needed is specified without @find_if_needed, you may end up
 * creating a Trash file when there already is one. Your app should start out
 * by doing a mate_vfs_find_directory() with the @find_if_needed to avoid this
 * and then use the @create_if_needed flag to create Trash lazily when it is
 * needed for throwing away an item on a given disk.
 * 
 * Returns: an integer representing the result of the operation.
 */
MateVFSResult	
mate_vfs_find_directory (MateVFSURI 			*near_uri,
			  MateVFSFindDirectoryKind 	kind,
			  MateVFSURI 			**result,
			  gboolean 			create_if_needed,
			  gboolean			find_if_needed,
			  guint 			permissions)
{
	return mate_vfs_find_directory_cancellable (near_uri, kind, result,
		create_if_needed, find_if_needed, permissions, NULL);
}
