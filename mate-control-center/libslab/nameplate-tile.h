/*
 * This file is part of libtile.
 *
 * Copyright (c) 2006 Novell, Inc.
 *
 * Libtile is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * Libtile is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libslab; if not, write to the Free Software Foundation, Inc., 51
 * Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __NAMEPLATE_TILE_H__
#define __NAMEPLATE_TILE_H__

#include <libslab/tile.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define NAMEPLATE_TILE_TYPE         (nameplate_tile_get_type ())
#define NAMEPLATE_TILE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), NAMEPLATE_TILE_TYPE, NameplateTile))
#define NAMEPLATE_TILE_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c), NAMEPLATE_TILE_TYPE, NameplateTileClass))
#define IS_NAMEPLATE_TILE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), NAMEPLATE_TILE_TYPE))
#define IS_NAMEPLATE_TILE_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c), NAMEPLATE_TILE_TYPE))
#define NAMEPLATE_TILE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), NAMEPLATE_TILE_TYPE, NameplateTileClass))

typedef struct {
	Tile tile;

	GtkWidget *image;
	GtkWidget *header;
	GtkWidget *subheader;
} NameplateTile;

typedef struct {
	TileClass tile_class;
} NameplateTileClass;

GType nameplate_tile_get_type (void);

GtkWidget *nameplate_tile_new (const gchar * uri, GtkWidget * image, GtkWidget * header,
	GtkWidget * subheader);

G_END_DECLS
#endif
