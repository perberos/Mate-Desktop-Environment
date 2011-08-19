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

#ifndef __TILE_H__
#define __TILE_H__

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TILE_TYPE         (tile_get_type ())
#define TILE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TILE_TYPE, Tile))
#define TILE_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c), TILE_TYPE, TileClass))
#define IS_TILE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TILE_TYPE))
#define IS_TILE_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c), TILE_TYPE))
#define TILE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TILE_TYPE, TileClass))
#define TILE_ACTION_TYPE         (tile_action_get_type ())
#define TILE_ACTION(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), TILE_ACTION_TYPE, TileAction))
#define TILE_ACTION_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST ((c), TILE_ACTION_TYPE, TileActionClass))
#define IS_TILE_ACTION(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), TILE_ACTION_TYPE))
#define IS_TILE_ACTION_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE ((c), TILE_ACTION_TYPE))
#define TILE_ACTION_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), TILE_ACTION_TYPE, TileActionClass))
#define TILE_ACTION_CHECK_FLAG(action,flag) ((TILE_ACTION (action)->flags & (flag)) != 0)
#define TILE_STATE_ENTERED GTK_STATE_PRELIGHT
#define TILE_STATE_FOCUSED GTK_STATE_PRELIGHT

typedef struct _Tile Tile;
typedef struct _TileClass TileClass;
typedef struct _TileAction TileAction;
typedef struct _TileActionClass TileActionClass;
typedef struct _TileEvent TileEvent;

typedef void (*TileActionFunc) (Tile *, TileEvent *, TileAction *);

typedef enum
{
	TILE_EVENT_ACTIVATED_SINGLE_CLICK,
	TILE_EVENT_ACTIVATED_DOUBLE_CLICK,
	TILE_EVENT_ACTIVATED_KEYBOARD,
	TILE_EVENT_IMPLICIT_DISABLE,
	TILE_EVENT_IMPLICIT_ENABLE,
	TILE_EVENT_ACTION_TRIGGERED
} TileEventType;

typedef enum
{
	TILE_ACTION_OPENS_NEW_WINDOW = 1 << 0,
	TILE_ACTION_OPENS_HELP = 1 << 1
} TileActionFlags;

struct _Tile
{
	GtkButton gtk_button;

	gchar *uri;
	GtkMenu *context_menu;
	gboolean entered;
	gboolean enabled;

	TileAction **actions;
	gint n_actions;

	TileAction *default_action;
};

struct _TileClass
{
	GtkButtonClass gtk_button_class;

	void (*tile_explicit_enable) (Tile *);
	void (*tile_explicit_disable) (Tile *);

	void (*tile_activated) (Tile *, TileEvent *);
	void (*tile_implicit_enable) (Tile *, TileEvent *);
	void (*tile_implicit_disable) (Tile *, TileEvent *);
	void (*tile_action_triggered) (Tile *, TileEvent *, TileAction *);
};

struct _TileAction
{
	GObject parent;

	Tile *tile;

	TileActionFunc func;
	GtkMenuItem *menu_item;

	guint32 flags;
};

struct _TileActionClass
{
	GObjectClass parent_class;
};

struct _TileEvent
{
	TileEventType type;
	guint32 time;
};

GType tile_get_type (void);
GType tile_action_get_type (void);

gint tile_compare (gconstpointer a, gconstpointer b);

void tile_explicit_enable (Tile * tile);
void tile_explicit_disable (Tile * tile);

void tile_implicit_enable (Tile * tile);
void tile_implicit_disable (Tile * tile);
void tile_implicit_enable_with_time (Tile * tile, guint32 time);
void tile_implicit_disable_with_time (Tile * tile, guint32 time);

void tile_trigger_action (Tile * tile, TileAction * action);
void tile_trigger_action_with_time (Tile * tile, TileAction * action, guint32 time);

TileAction *tile_action_new (Tile * tile, TileActionFunc func, const gchar * menu_item_markup,
	guint32 flags);

void tile_action_set_menu_item_label (TileAction * action, const gchar * markup);
GtkMenuItem *tile_action_get_menu_item (TileAction * action);

G_END_DECLS
#endif
