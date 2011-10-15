/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-module-shared.h - code shared between the different modules
   place.

   Copyright (C) 2001 Free Software Foundation

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

   Author: Seth Nickell <snickell@stanford.edu
 */

#ifndef MATE_VFS_MODULE_SHARED_H
#define MATE_VFS_MODULE_SHARED_H

/* This check is for Linux, but should be harmless on other platforms. */
#if !defined (_LARGEFILE64_SOURCE) || _FILE_OFFSET_BITS+0 != 64
#error configuration macros set inconsistently, mate_vfs_stat_to_file_info will malfunction
#endif

#include <libmatevfs/mate-vfs-file-info.h>
#include <libmatevfs/mate-vfs-monitor.h>
#include <libmatevfs/mate-vfs-method.h>

#ifdef __cplusplus
extern "C" {
#endif

const char *    mate_vfs_mime_type_from_mode            (mode_t             mode);
const char *    mate_vfs_mime_type_from_mode_or_default (mode_t             mode,
							  const char        *defaultv);

void            mate_vfs_stat_to_file_info	(MateVFSFileInfo  *file_info,
					    	 const struct stat *statptr);

MateVFSResult  mate_vfs_set_meta          	(MateVFSFileInfo  *info,
					    	 const char        *file_name,
					    	 const char        *meta_key);

MateVFSResult  mate_vfs_set_meta_for_list 	(MateVFSFileInfo  *info,
					    	 const char        *file_name,
					    	 const GList       *meta_keys);

const char     *mate_vfs_get_special_mime_type (MateVFSURI       *uri);

void            mate_vfs_monitor_callback (MateVFSMethodHandle *method_handle,
					    MateVFSURI *info_uri,
					    MateVFSMonitorEventType event_type);


#ifdef __cplusplus
}
#endif

#endif
