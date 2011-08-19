/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * MATE Search Tool
 *
 *  File:  gsearchtool.h
 *
 *  (C) 1998,2002 the Free Software Foundation
 *
 *  Authors:	Dennis Cranston  <dennis_cranston@yahoo.com>
 *              George Lebl
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

#ifndef _GSEARCHTOOL_H_
#define _GSEARCHTOOL_H_

#ifdef __cplusplus
extern "C" {
#pragma }
#endif

#include <gtk/gtk.h>
#include <mateconf/mateconf.h>
#include <mateconf/mateconf-client.h>

#define GSEARCH_TYPE_WINDOW gsearch_window_get_type()
#define GSEARCH_WINDOW(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSEARCH_TYPE_WINDOW, GSearchWindow))
#define GSEARCH_WINDOW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GSEARCH_TYPE_WINDOW, GSearchWindowClass))
#define GSEARCH_IS_WINDOW(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSEARCH_TYPE_WINDOW))
#define GSEARCH_IS_WINDOW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GSEARCH_TYPE_WINDOW))
#define GSEARCH_WINDOW_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), GSEARCH_TYPE_WINDOW, GSearchWindowClass))

#define MATE_SEARCH_TOOL_ICON "system-search"
#define MINIMUM_WINDOW_WIDTH   420
#define MINIMUM_WINDOW_HEIGHT  310
#define DEFAULT_WINDOW_WIDTH   554
#define DEFAULT_WINDOW_HEIGHT  350
#define WINDOW_HEIGHT_STEP      35
#define NUM_VISIBLE_COLUMNS      5

typedef enum {
	STOPPED,
	ABORTED,
	RUNNING,
	MAKE_IT_STOP,
	MAKE_IT_QUIT
} GSearchCommandStatus;

typedef enum {
	COLUMN_ICON,
	COLUMN_NAME,
	COLUMN_RELATIVE_PATH,
	COLUMN_LOCALE_FILE,
	COLUMN_READABLE_SIZE,
	COLUMN_SIZE,
	COLUMN_TYPE,
	COLUMN_READABLE_DATE,
	COLUMN_DATE,
	COLUMN_MONITOR,
	COLUMN_NO_FILES_FOUND,
	NUM_COLUMNS
} GSearchResultColumns;

typedef struct _GSearchWindow GSearchWindow;
typedef struct _GSearchWindowClass GSearchWindowClass;
typedef struct _GSearchCommandDetails GSearchCommandDetails;
typedef struct _GSearchConstraint GSearchConstraint;
typedef struct _GSearchMonitor GSearchMonitor;

struct _GSearchWindow {
	GtkWindow               parent_instance;

	GtkWidget             * window;
	GtkUIManager          * window_ui_manager;
	GdkGeometry             window_geometry;
	gint                    window_width;
	gint                    window_height;
	gboolean                is_window_maximized;
	gboolean                is_window_accessible;

	GtkWidget             * name_contains_entry;
	GtkWidget             * look_in_folder_button;
	GtkWidget             * name_and_folder_table;
	GtkWidget             * progress_spinner;
	GtkWidget             * find_button;
	GtkWidget             * stop_button;
	GtkWidget             * focus;

	GtkWidget             * show_more_options_expander;
	GtkWidget             * available_options_vbox;
	GtkWidget             * available_options_label;
	GtkWidget             * available_options_combo_box;
	GtkWidget             * available_options_add_button;
	GtkSizeGroup          * available_options_button_size_group;
	GList                 * available_options_selected_list;

	GtkWidget             * files_found_label;
	GtkWidget             * search_results_vbox;
	GtkWidget             * search_results_popup_menu;
	GtkWidget             * search_results_popup_submenu;
	GtkWidget             * search_results_save_results_as_item;
	GtkTreeView           * search_results_tree_view;
	GtkTreeViewColumn     * search_results_folder_column;
	GtkTreeViewColumn     * search_results_size_column;
	GtkTreeViewColumn     * search_results_type_column;
	GtkTreeViewColumn     * search_results_date_column;
	GtkListStore          * search_results_list_store;
	GtkCellRenderer       * search_results_name_cell_renderer;
	GtkTreeSelection      * search_results_selection;
	GtkTreeIter             search_results_iter;
	GtkTreePath           * search_results_hover_path;
	GHashTable            * search_results_filename_hash_table;
	GHashTable            * search_results_pixbuf_hash_table;
	gchar                 * search_results_date_format_string;
	gint		        show_thumbnails_file_size_limit;
	gboolean		show_thumbnails;
	gboolean                is_search_results_single_click_to_activate;
	gboolean		is_locate_database_check_finished;
	gboolean		is_locate_database_available;

	gchar                 * save_results_as_default_filename;

	GSearchCommandDetails * command_details;
};

struct _GSearchCommandDetails {
	pid_t                   command_pid;
	GSearchCommandStatus    command_status;

	gchar                 * name_contains_pattern_string;
	gchar                 * name_contains_regex_string;
	gchar                 * look_in_folder_string;

	gboolean		is_command_first_pass;
	gboolean		is_command_using_quick_mode;
	gboolean		is_command_second_pass_enabled;
	gboolean		is_command_show_hidden_files_enabled;
	gboolean		is_command_regex_matching_enabled;
	gboolean		is_command_timeout_enabled;
};

struct _GSearchConstraint {
	gint constraint_id;
	union {
		gchar * text;
		gint 	time;
		gint 	number;
	} data;
};

struct _GSearchWindowClass {
	GtkWindowClass parent_class;
};

struct _GSearchMonitor {
	GSearchWindow         * gsearch;
	GtkTreeRowReference   * reference;
	GFileMonitor          * handle;
};

GType
gsearch_window_get_type (void);

gchar *
build_search_command (GSearchWindow * gsearch,
                      gboolean first_pass);
void
spawn_search_command (GSearchWindow * gsearch,
                      gchar * command);
void
add_constraint (GSearchWindow * gsearch,
                gint constraint_id,
                gchar * value,
                gboolean show_constraint);
void
update_constraint_info (GSearchConstraint * constraint,
                        gchar * info);
void
remove_constraint (gint constraint_id);

void
set_constraint_mateconf_boolean (gint constraint_id,
                              gboolean flag);
void
set_constraint_selected_state (GSearchWindow * gsearch,
                               gint constraint_id,
                               gboolean state);
void
set_clone_command (GSearchWindow * gsearch,
                   gint * argcp,
                   gchar *** argvp,
                   gpointer client_data,
                   gboolean escape_values);
void
update_search_counts (GSearchWindow * gsearch);

gboolean
tree_model_iter_free_monitor (GtkTreeModel * model,
                              GtkTreePath * path,
                              GtkTreeIter * iter,
                              gpointer data);

#ifdef __cplusplus
}
#endif

#endif /* _GSEARCHTOOL_H_ */
