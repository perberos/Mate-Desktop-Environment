/* bacon-resize.h
 * Copyright (C) 2003-2004, Bastien Nocera <hadess@hadess.net>
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02139, USA.
 */

#ifndef BACON_RESIZE_H
#define BACON_RESIZE_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define BACON_TYPE_RESIZE		(bacon_resize_get_type ())
#define BACON_RESIZE(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), BACON_TYPE_RESIZE, BaconResize))
#define BACON_RESIZE_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), BACON_TYPE_RESIZE, BaconResizeClass))
#define BACON_IS_RESIZE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BACON_TYPE_RESIZE))
#define BACON_IS_RESIZE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), BACON_TYPE_RESIZE))

typedef struct BaconResize		BaconResize;
typedef struct BaconResizeClass		BaconResizeClass;
typedef struct BaconResizePrivate	BaconResizePrivate;

struct BaconResize {
	GObject parent;
	BaconResizePrivate *priv;
};

struct BaconResizeClass {
	GObjectClass parent_class;
};

GType bacon_resize_get_type	(void);
BaconResize *bacon_resize_new	(GtkWidget *video_widget);
void bacon_resize_resize	(BaconResize *resize);
void bacon_resize_restore	(BaconResize *resize);

G_END_DECLS

#endif /* BACON_RESIZE_H */
