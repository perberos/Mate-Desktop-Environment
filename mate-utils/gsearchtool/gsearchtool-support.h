/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * MATE Search Tool
 *
 *  File:  gsearchtool-support.h
 *
 *  (C) 2002 the Free Software Foundation
 *
 *  Authors:	Dennis Cranston  <dennis_cranston@yahoo.com>
 *		George Lebl
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 */

#ifndef _GSEARCHTOOL_SUPPORT_H_
#define _GSEARCHTOOL_SUPPORT_H_

#ifdef __cplusplus
extern "C" {
#pragma }
#endif

#include "gsearchtool.h"

#define ICON_SIZE 24

gboolean
gsearchtool_mateconf_get_boolean (const gchar * key);

void
gsearchtool_mateconf_set_boolean (const gchar * key,
                               const gboolean flag);
gint
gsearchtool_mateconf_get_int (const gchar * key);

void
gsearchtool_mateconf_set_int (const gchar * key,
                           const gint value);
char *
gsearchtool_mateconf_get_string (const gchar * key);

void
gsearchtool_mateconf_set_string (const gchar * key,
                              const gchar * value);

GSList *
gsearchtool_mateconf_get_list (const gchar * key,
                            MateConfValueType list_type);
void
gsearchtool_mateconf_set_list (const gchar * key,
                            GSList * list,
                            MateConfValueType list_type);
void
gsearchtool_mateconf_add_dir (const gchar * dir);

void
gsearchtool_mateconf_watch_key (const gchar * dir,
                             const gchar * key,
                             MateConfClientNotifyFunc callback,
                             gpointer user_data);
gboolean
is_path_hidden (const gchar * path);

gboolean
is_quick_search_excluded_path (const gchar * path);

gboolean
is_second_scan_excluded_path (const gchar * path);

gboolean
compare_regex (const gchar * regex,
               const gchar * string);
gboolean
limit_string_to_x_lines (GString * string,
                         gint x);
gchar *
escape_single_quotes (const gchar * string);

gchar *
escape_double_quotes (const gchar * string);

gchar *
backslash_backslash_characters (const gchar * string);

gchar *
backslash_special_characters (const gchar * string);

gchar *
remove_mnemonic_character (const gchar * string);

gchar *
get_readable_date (const gchar * format_string,
                   const time_t file_time_raw);
gchar *
gsearchtool_strdup_strftime (const gchar * format,
                             struct tm * time_pieces);
gchar *
get_file_type_description (const gchar * file,
                           GFileInfo * file_info);
GdkPixbuf *
get_file_pixbuf (GSearchWindow * gsearch,
                 GFileInfo * file_info);
gboolean
open_file_with_filemanager (GtkWidget * window,
                            const gchar * file);
gboolean
open_file_with_application (GtkWidget * window,
                            const gchar * file,
                            GAppInfo * app);
gboolean
launch_file (const gchar * file);

gchar *
gsearchtool_get_unique_filename (const gchar * path,
                                 const gchar * suffix);
GtkWidget *
gsearchtool_button_new_with_stock_icon (const gchar * string,
                                        const gchar * stock_id);
GSList *
gsearchtool_get_columns_order (GtkTreeView * treeview);

void
gsearchtool_set_columns_order (GtkTreeView * treeview);

void
gsearchtool_get_stored_window_geometry (gint * width,
                                        gint * height);
gchar *
gsearchtool_get_next_duplicate_name (const gchar * basname);

#ifdef __cplusplus
}
#endif

#endif /* _GSEARCHTOOL_SUPPORT_H */
