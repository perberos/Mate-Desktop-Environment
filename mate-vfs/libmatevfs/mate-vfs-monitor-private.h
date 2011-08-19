/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-monitor.h - File Monitoring for the MATE Virtual File System.

   Copyright (C) 2001 Ian McKellar

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

   Author: Ian McKellar <yakk@yakk.net>
*/

#ifndef MATE_VFS_MONITOR_PRIVATE_H
#define MATE_VFS_MONITOR_PRIVATE_H

#include <glib.h>
#include <libmatevfs/mate-vfs-monitor.h>
#include <libmatevfs/mate-vfs-method.h>
#include <libmatevfs/mate-vfs-uri.h>

MateVFSResult _mate_vfs_monitor_do_add (MateVFSMethod *method,
			  		 MateVFSMonitorHandle **handle,
					 MateVFSURI *uri,
                          		 MateVFSMonitorType monitor_type,
			  		 MateVFSMonitorCallback callback,
			  		 gpointer user_data);

MateVFSResult _mate_vfs_monitor_do_cancel (MateVFSMonitorHandle *handle);

#endif /* MATE_VFS_MONITOR_PRIVATE_H */
