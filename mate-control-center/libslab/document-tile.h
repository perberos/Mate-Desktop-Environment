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

#ifndef __DOCUMENT_TILE_H__
#define __DOCUMENT_TILE_H__

#include <time.h>

#include "nameplate-tile.h"
#include "bookmark-agent.h"

G_BEGIN_DECLS

#define DOCUMENT_TILE_TYPE         (document_tile_get_type ())
#define DOCUMENT_TILE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), DOCUMENT_TILE_TYPE, DocumentTile))
#define DOCUMENT_TILE_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c), DOCUMENT_TILE_TYPE, DocumentTileClass))
#define IS_DOCUMENT_TILE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), DOCUMENT_TILE_TYPE))
#define IS_DOCUMENT_TILE_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c), DOCUMENT_TILE_TYPE))
#define DOCUMENT_TILE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), DOCUMENT_TILE_TYPE, DocumentTileClass))

typedef struct {
	NameplateTile nameplate_tile;
} DocumentTile;

typedef struct {
	NameplateTileClass nameplate_tile_class;
} DocumentTileClass;

#define DOCUMENT_TILE_ACTION_OPEN_WITH_DEFAULT    0
#define DOCUMENT_TILE_ACTION_OPEN_IN_FILE_MANAGER 1
#define DOCUMENT_TILE_ACTION_RENAME               2
#define DOCUMENT_TILE_ACTION_MOVE_TO_TRASH        3
#define DOCUMENT_TILE_ACTION_DELETE               4
#define DOCUMENT_TILE_ACTION_UPDATE_MAIN_MENU     5
#define DOCUMENT_TILE_ACTION_SEND_TO              6
#define DOCUMENT_TILE_ACTION_CLEAN_ITEM                  7
#define DOCUMENT_TILE_ACTION_CLEAN_ALL           8
#define DOCUMENT_TILE_ACTION_NUM_OF_ACTIONS       9 /* must be last entry and equal to the number of actions */

GType document_tile_get_type (void);

GtkWidget *document_tile_new (BookmarkStoreType bookmark_store_type, const gchar *uri, const gchar *mime_type, time_t modified);

//If you want to show a icon instead of a thumbnail
GtkWidget *document_tile_new_force_icon (const gchar *uri, const gchar *mime_type, time_t modified, const gchar *icon);

G_END_DECLS

#endif
