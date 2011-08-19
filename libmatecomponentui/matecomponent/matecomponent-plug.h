/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

#ifndef __MATECOMPONENT_PLUG_H__
#define __MATECOMPONENT_PLUG_H__

#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

typedef struct _MateComponentPlug MateComponentPlug;

#include <matecomponent/matecomponent-control.h>

G_BEGIN_DECLS

#define MATECOMPONENT_TYPE_PLUG          (matecomponent_plug_get_type ())
#define MATECOMPONENT_PLUG(obj)          G_TYPE_CHECK_INSTANCE_CAST (obj, matecomponent_plug_get_type (), MateComponentPlug)
#define MATECOMPONENT_PLUG_CLASS(klass)  G_TYPE_CHECK_CLASS_CAST (klass, matecomponent_plug_get_type (), MateComponentPlugClass)
#define MATECOMPONENT_IS_PLUG(obj)       G_TYPE_CHECK_INSTANCE_TYPE (obj, matecomponent_plug_get_type ())

typedef struct _MateComponentPlugPrivate MateComponentPlugPrivate;

struct _MateComponentPlug {
	GtkPlug            plug;

	MateComponentControl     *control;
	MateComponentPlugPrivate *priv;
};

typedef struct {
	GtkPlugClass parent_class;

	gpointer dummy[4];
} MateComponentPlugClass;

GType        matecomponent_plug_get_type        (void) G_GNUC_CONST;
void           matecomponent_plug_construct       (MateComponentPlug    *plug,
					    guint32        socket_id);
void           matecomponent_plug_construct_full  (MateComponentPlug    *plug,
					    GdkDisplay    *display,
					    guint32        socket_id);
GtkWidget     *matecomponent_plug_new             (guint32        socket_id);
GtkWidget     *matecomponent_plug_new_for_display (GdkDisplay    *display,
					    guint32        socket_id);

G_END_DECLS

#endif /* __MATECOMPONENT_PLUG_H__ */
