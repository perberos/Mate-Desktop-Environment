/* 
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * Copytight (C) 2000 Helix Code, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-1999.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */
#ifndef __MATECOMPONENT_SOCKET_H__
#define __MATECOMPONENT_SOCKET_H__

#include <glib.h>
#include <gtk/gtk.h>

typedef struct _MateComponentSocket MateComponentSocket;

#include <matecomponent/matecomponent-control-frame.h>

G_BEGIN_DECLS

#define MATECOMPONENT_TYPE_SOCKET          (matecomponent_socket_get_type ())
#define MATECOMPONENT_SOCKET(obj)          G_TYPE_CHECK_INSTANCE_CAST (obj, matecomponent_socket_get_type (), MateComponentSocket)
#define MATECOMPONENT_SOCKET_CLASS(klass)  G_TYPE_CHECK_CLASS_CAST (klass, matecomponent_socket_get_type (), MateComponentSocketClass)
#define MATECOMPONENT_IS_SOCKET(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, matecomponent_socket_get_type ())

struct _MateComponentSocket {
	GtkSocket           socket;

	MateComponentControlFrame *frame;
	gpointer            priv;
};

typedef struct {
	GtkSocketClass parent_class;

	gpointer dummy[4];
} MateComponentSocketClass;


GType               matecomponent_socket_get_type          (void);
GtkWidget*          matecomponent_socket_new               (void);

G_END_DECLS

#endif /* __MATECOMPONENT_SOCKET_H__ */
