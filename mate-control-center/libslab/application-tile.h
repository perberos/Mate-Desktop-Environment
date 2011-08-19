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

#ifndef __APPLICATION_TILE_H__
#define __APPLICATION_TILE_H__

#include <libslab/nameplate-tile.h>

#include <libmate/mate-desktop-item.h>

G_BEGIN_DECLS

#define APPLICATION_TILE_TYPE         (application_tile_get_type ())
#define APPLICATION_TILE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), APPLICATION_TILE_TYPE, ApplicationTile))
#define APPLICATION_TILE_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c), APPLICATION_TILE_TYPE, ApplicationTileClass))
#define IS_APPLICATION_TILE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), APPLICATION_TILE_TYPE))
#define IS_APPLICATION_TILE_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c), APPLICATION_TILE_TYPE))
#define APPLICATION_TILE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), APPLICATION_TILE_TYPE, ApplicationTileClass))
#define APPLICATION_TILE_ACTION_START             0
#define APPLICATION_TILE_ACTION_HELP              1
#define APPLICATION_TILE_ACTION_UPDATE_MAIN_MENU  2
#define APPLICATION_TILE_ACTION_UPDATE_STARTUP    3
#define APPLICATION_TILE_ACTION_UPGRADE_PACKAGE   4
#define APPLICATION_TILE_ACTION_UNINSTALL_PACKAGE 5

typedef struct
{
	NameplateTile nameplate_tile;

	gchar *name;
	gchar *description;
	gchar *mateconf_prefix;
} ApplicationTile;

typedef struct
{
	NameplateTileClass nameplate_tile_class;
} ApplicationTileClass;

GType application_tile_get_type (void);

GtkWidget *application_tile_new (const gchar * desktop_item_id);
GtkWidget *application_tile_new_full (const gchar * desktop_item_id,
	GtkIconSize icon_size, gboolean show_generic_name, const gchar *mateconf_prefix);

MateDesktopItem *application_tile_get_desktop_item (ApplicationTile * tile);

G_END_DECLS
#endif
