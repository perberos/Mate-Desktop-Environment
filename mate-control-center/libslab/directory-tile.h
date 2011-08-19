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

#ifndef __DIRECTORY_TILE_H__
#define __DIRECTORY_TILE_H__

#include <time.h>

#include <libslab/nameplate-tile.h>

G_BEGIN_DECLS

#define DIRECTORY_TILE_TYPE         (directory_tile_get_type ())
#define DIRECTORY_TILE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), DIRECTORY_TILE_TYPE, DirectoryTile))
#define DIRECTORY_TILE_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c), DIRECTORY_TILE_TYPE, DirectoryTileClass))
#define IS_DIRECTORY_TILE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), DIRECTORY_TILE_TYPE))
#define IS_DIRECTORY_TILE_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c), DIRECTORY_TILE_TYPE))
#define DIRECTORY_TILE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), DIRECTORY_TILE_TYPE, DirectoryTileClass))

typedef struct {
	NameplateTile nameplate_tile;
} DirectoryTile;

typedef struct {
	NameplateTileClass nameplate_tile_class;
} DirectoryTileClass;

#define DIRECTORY_TILE_ACTION_OPEN          0
#define DIRECTORY_TILE_ACTION_RENAME        1
#define DIRECTORY_TILE_ACTION_MOVE_TO_TRASH 2
#define DIRECTORY_TILE_ACTION_DELETE        3
#define DIRECTORY_TILE_ACTION_SEND_TO       4

GType directory_tile_get_type (void);

GtkWidget *directory_tile_new (const gchar *uri, const gchar *title, const gchar *icon_name, const gchar *mime_type);

G_END_DECLS

#endif
