/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-acl.h - ACL Handling for the MATE Virtual File System.
   Virtual File System.

   Copyright (C) 2005 Christian Kellner
   Copyright (C) 2005 Sun Microsystems

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

   Author: Christian Kellner <gicmo@gnome.org>
   Author: Alvaro Lopez Ortega <alvaro@sun.com>
*/

#include <glib.h>
#include <libmatevfs/mate-vfs-acl.h>
#include <libmatevfs/mate-vfs-context.h>
#include <libmatevfs/mate-vfs-file-info.h>

#ifndef FILE_ACL_H
#define FILE_ACL_H

#ifdef __cplusplus
extern "C" {
#endif

MateVFSResult file_get_acl (const char       *path,
                             MateVFSFileInfo *info,
                             struct stat      *statbuf, /* needed? */
                             MateVFSContext  *context);

MateVFSResult file_set_acl (const char             *path,
			     const MateVFSFileInfo *info,
                             MateVFSContext         *context);

#ifdef __cplusplus
}
#endif


#endif /*FILEACL_H*/
