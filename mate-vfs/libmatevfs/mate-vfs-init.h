/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-init.h - Initialization for the MATE Virtual File System.

   Copyright (C) 1999 Free Software Foundation

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

   Author: Ettore Perazzoli <ettore@comm2000.it>
*/

#ifndef MATE_VFS_INIT_H
#define MATE_VFS_INIT_H

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

gboolean mate_vfs_init        (void);
gboolean mate_vfs_initialized (void);
void     mate_vfs_shutdown    (void);

#ifndef MATE_VFS_DISABLE_DEPRECATED
/* Stuff for use in a MateModuleInfo */
void     mate_vfs_loadinit    (gpointer app,
				gpointer modinfo);
void     mate_vfs_preinit     (gpointer app,
				gpointer modinfo);
void     mate_vfs_postinit    (gpointer app,
				gpointer modinfo);
#endif

#ifdef __cplusplus
}
#endif

#endif
