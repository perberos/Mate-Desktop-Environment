/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-filesystem-type.h - Handling of filesystems for the MATE Virtual File System.

   Copyright (C) 2003 Red Hat, Inc

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

   Author: Alexander Larsson <alexl@redhat.com>
*/

#ifndef MATE_VFS_FILESYSTEM_TYPE_H
#define MATE_VFS_FILESYSTEM_TYPE_H

#include <glib.h>

char *   _mate_vfs_filesystem_volume_name (const char *fs_type);
gboolean _mate_vfs_filesystem_use_trash   (const char *fs_type);

#endif /* MATE_VFS_FILESYSTEM_TYPE_H */
