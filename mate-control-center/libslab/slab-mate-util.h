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

#ifndef __SLAB_MATE_UTIL_H__
#define __SLAB_MATE_UTIL_H__

#include <glib.h>
#include <gtk/gtk.h>
#include <libmate/mate-desktop-item.h>

G_BEGIN_DECLS

#define SLAB_APPLICATION_BROWSER_KEY    "/desktop/mate/applications/main-menu/application_browser"
#define SLAB_SYSTEM_LIST_KEY            "/desktop/mate/applications/main-menu/system_list"
#define SLAB_FILE_BROWSER_KEY           "/desktop/mate/applications/main-menu/file_browser"
#define SLAB_SYSTEM_MONITOR_KEY         "/desktop/mate/applications/main-menu/system_monitor"
#define SLAB_NETWORK_CONFIG_TOOL_KEY    "/desktop/mate/applications/main-menu/network_config_tool"
#define SLAB_NETWORK_CONFIG_TOOL_NM_KEY "/desktop/mate/applications/main-menu/network_config_tool_nm"
#define SLAB_URGENT_CLOSE_KEY           "/desktop/mate/applications/main-menu/urgent_close"
#define SLAB_LOCK_SCREEN_PRIORITY_KEY   "/desktop/mate/applications/main-menu/lock_screen_priority"
#define SLAB_MAIN_MENU_REORDERING_KEY   "/desktop/mate/applications/main-menu/main_menu_reordering"
#define SLAB_UPGRADE_PACKAGE_KEY        "/desktop/mate/applications/main-menu/upgrade_package_command"
#define SLAB_UNINSTALL_PACKAGE_KEY      "/desktop/mate/applications/main-menu/uninstall_package_command"
#define SLAB_USER_SPECIFIED_APPS_KEY    "/desktop/mate/applications/main-menu/file-area/user_specified_apps"
#define SLAB_APPLICATION_USE_DB_KEY     "/desktop/mate/applications/main-menu/file-area/app_use_db"
#define SLAB_FILE_ITEM_LIMIT            "/desktop/mate/applications/main-menu/file-area/item_limit"
#define SLAB_FILE_BLACKLIST             "/desktop/mate/applications/main-menu/file-area/file_blacklist"
#define SLAB_FILE_MANAGER_OPEN_CMD      "/desktop/mate/applications/main-menu/file-area/file_mgr_open_cmd"
#define SLAB_FILE_SEND_TO_CMD           "/desktop/mate/applications/main-menu/file-area/file_send_to_cmd"

gboolean get_slab_mateconf_bool (const gchar * key);
gint get_slab_mateconf_int (const gchar * key);
GSList *get_slab_mateconf_slist (const gchar * key);
void free_slab_mateconf_slist_of_strings (GSList * list);
void free_list_of_strings (GList * list);
gchar *get_slab_mateconf_string (const gchar * key);

MateDesktopItem *load_desktop_item_from_mateconf_key (const gchar * key);
MateDesktopItem *load_desktop_item_from_unknown (const gchar * id);

gchar *get_package_name_from_desktop_item (MateDesktopItem * desktop_item);

gboolean open_desktop_item_exec (MateDesktopItem * desktop_item);
gboolean open_desktop_item_help (MateDesktopItem * desktop_item);

gboolean desktop_item_is_in_main_menu (MateDesktopItem * desktop_item);
gboolean desktop_uri_is_in_main_menu (const gchar * uri);

gint desktop_item_location_compare (gconstpointer a, gconstpointer b);

gboolean slab_load_image (GtkImage * image, GtkIconSize size, const gchar * image_id);

gchar *string_replace_once (const gchar * str_template, const gchar * key, const gchar * value);

void spawn_process (const gchar * command);
void copy_file (const gchar * src_uri, const gchar * dst_uri);

G_END_DECLS
#endif /* __SLAB_MATE_UTIL_H__ */
