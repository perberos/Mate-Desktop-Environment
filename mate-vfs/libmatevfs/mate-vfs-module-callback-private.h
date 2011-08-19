/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-module-callbacks-private.h - private entry points for module callback mechanism

   Copyright (C) 2001 Maciej Stachowiak

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

   Author: Maciej Stachowiak <mjs@noisehavoc.org>
*/

#ifndef MATE_VFS_MODULE_CALLBACKS_PRIVATE_H
#define MATE_VFS_MODULE_CALLBACKS_PRIVATE_H

#include <glib.h>
#include <libmatevfs/mate-vfs-job.h>

#ifdef USE_DAEMON
#include <dbus/dbus.h>
#endif

void                             _mate_vfs_module_callback_private_init        (void);
MateVFSModuleCallbackStackInfo *_mate_vfs_module_callback_get_stack_info      (void);
void                             _mate_vfs_module_callback_free_stack_info     (MateVFSModuleCallbackStackInfo *stack_info);
void                             _mate_vfs_module_callback_use_stack_info      (MateVFSModuleCallbackStackInfo *stack_info);
void                             _mate_vfs_module_callback_clear_stacks        (void);
void                             _mate_vfs_module_callback_set_in_async_thread (gboolean in_async_thread);


/* For callback marshalling: */
gboolean                         _mate_vfs_module_callback_marshal_invoke (const char    *callback_name,
									    gconstpointer  in,
									    gsize          in_size,
									    gpointer       out,
									    gsize          out_size);


#ifdef USE_DAEMON
gboolean _mate_vfs_module_callback_demarshal_invoke (const char  *callback_name,
						      DBusMessageIter *iter_in,
						      DBusMessage *reply);
#endif

#endif
