/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2007 Philip Withnall <philip@tecnocode.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
 *
 * The Totem project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and Totem. This
 * permission are above and beyond the permissions granted by the GPL license
 * Totem is covered by.
 *
 * Monday 7th February 2005: Christian Schaller: Add excemption clause.
 * See license_change file for details.
 *
 * Author: Bastien Nocera <hadess@hadess.net>, Philip Withnall <philip@tecnocode.co.uk>
 */

#ifndef TOTEM_OPEN_LOCATION_H
#define TOTEM_OPEN_LOCATION_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TOTEM_TYPE_OPEN_LOCATION		(totem_open_location_get_type ())
#define TOTEM_OPEN_LOCATION(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), TOTEM_TYPE_OPEN_LOCATION, TotemOpenLocation))
#define TOTEM_OPEN_LOCATION_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), TOTEM_TYPE_OPEN_LOCATION, TotemOpenLocationClass))
#define TOTEM_IS_OPEN_LOCATION(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), TOTEM_TYPE_OPEN_LOCATION))
#define TOTEM_IS_OPEN_LOCATION_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), TOTEM_TYPE_OPEN_LOCATION))

typedef struct TotemOpenLocation		TotemOpenLocation;
typedef struct TotemOpenLocationClass		TotemOpenLocationClass;
typedef struct TotemOpenLocationPrivate		TotemOpenLocationPrivate;

struct TotemOpenLocation {
	GtkDialog parent;
	TotemOpenLocationPrivate *priv;
};

struct TotemOpenLocationClass {
	GtkDialogClass parent_class;
};

GType totem_open_location_get_type		(void);
GtkWidget *totem_open_location_new		(void);
char *totem_open_location_get_uri		(TotemOpenLocation *open_location);

G_END_DECLS

#endif /* TOTEM_OPEN_LOCATION_H */
