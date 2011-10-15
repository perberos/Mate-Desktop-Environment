/*
 * This file is part of libslab.
 *
 * Copyright (c) 2006 Novell, Inc.
 *
 * Libslab is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * Libslab is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libslab; if not, write to the Free Software Foundation, Inc., 51
 * Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __APP_SHELL_H__
#define __APP_SHELL_H__

#include <glib.h>
#include <gtk/gtk.h>
#define MATEMENU_I_KNOW_THIS_IS_UNSTABLE
#include <matemenu-tree.h>
#include <libmate/mate-desktop-item.h>

#include <libslab/slab-section.h>
#include <libslab/tile.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CATEGORY_SPACING 0
#define GROUP_POSITION_NUMBER_KEY "Unique Group Position Number"
#define APP_ACTION_KEY  "Unique Application Action Key"

/* constants for initial sizing */
#define SIZING_SCREEN_WIDTH_LARGE  1024
#define SIZING_SCREEN_WIDTH_MEDIUM 800
#define SIZING_SCREEN_WIDTH_SMALL  640
#define SIZING_SCREEN_WIDTH_LARGE_NUMCOLS  3
#define SIZING_SCREEN_WIDTH_MEDIUM_NUMCOLS 2
#define SIZING_SCREEN_WIDTH_SMALL_NUMCOLS  1
#define SIZING_TILE_WIDTH 230
#define SIZING_HEIGHT_PERCENT 0.8

typedef struct
{
	const gchar *name;
	gint max_items;
	GArray *garray;
} NewAppConfig;

typedef struct _AppShellData
{
	GtkWidget *main_app;
	gint main_app_window_x;
	gint main_app_window_y;
	gboolean main_app_window_shown_once;

	GtkWidget *shell;
	GtkWidget *groups_section;

	GtkWidget *actions_section;
	/*
		NULL      - if the available actions depend on the current tile selected
		NON-NULL  - a list of AppAction that are always shown
	*/
	GSList *static_actions;

	GtkWidget *filter_section;
	gchar *filter_string;
	GdkCursor *busy_cursor;

	GtkWidget *category_layout;
	GList *categories_list;
	GList *cached_tables_list;	/* list of currently showing (not filtered out) tables */
	Tile *last_clicked_launcher;
	SlabSection *selected_group;
	GtkIconSize icon_size;
	const gchar *mateconf_prefix;
	const gchar *menu_name;
	NewAppConfig *new_apps;
	MateMenuTree *tree;
	GHashTable *hash;

	guint filter_changed_timeout;
	gboolean stop_incremental_relayout;
	GList *incremental_relayout_cat_list;
	gboolean filtered_out_everything;
	GtkWidget *filtered_out_everything_widget;
	GtkLabel *filtered_out_everything_widget_label;

	gboolean show_tile_generic_name;
	gboolean exit_on_close;
} AppShellData;

typedef struct
{
	gchar *category;
	Tile *group_launcher;

	SlabSection *section;
	GList *launcher_list;
	GList *filtered_launcher_list;
} CategoryData;

typedef struct
{
	const gchar *name;
	MateDesktopItem *item;
} AppAction;

typedef struct
{
	long time;
	MateDesktopItem *item;
} NewAppData;

void generate_categories (AppShellData * app_data);

/* If new_apps is NULL then the new applications category is not created */
AppShellData *appshelldata_new (const gchar * menu_name, NewAppConfig * new_apps,
	const gchar * mateconf_keys_prefix, GtkIconSize icon_size,
	gboolean show_tile_generic_name, gboolean exit_on_close);

void layout_shell (AppShellData * app_data, const gchar * filter_title, const gchar * groups_title,
	const gchar * actions_title, GSList * actions,
	void (*actions_handler) (Tile *, TileEvent *, gpointer));

gboolean create_main_window (AppShellData * app_data, const gchar * app_name, const gchar * title,
	const gchar * window_icon, gint width, gint height, gboolean hidden);

void hide_shell (AppShellData * app_data);

void show_shell (AppShellData * app_data);

#ifdef __cplusplus
}
#endif
#endif /* __APP_SHELL_H__ */
