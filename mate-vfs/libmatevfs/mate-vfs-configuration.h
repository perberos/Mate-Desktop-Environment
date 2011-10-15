/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-configuration.h - Handling of the MATE Virtual File System
   configuration.

   Copyright (C) 1999 Free Software Foundation

   The Mate Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Ettore Perazzoli <ettore@comm2000.it> */

#ifndef MATE_VFS_CONFIGURATION_H
#define MATE_VFS_CONFIGURATION_H

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

void         _mate_vfs_configuration_add_directory   (const char *dir);
gboolean     _mate_vfs_configuration_init            (void);
void         _mate_vfs_configuration_uninit          (void);
const gchar *_mate_vfs_configuration_get_module_path (const gchar *method_name, const char ** args, gboolean *daemon);
GList       *_mate_vfs_configuration_get_methods_list(void);


#ifdef __cplusplus
}
#endif

#endif /* MATE_VFS_CONFIGURATION_H */
