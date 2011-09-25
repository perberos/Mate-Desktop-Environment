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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libmate/mate-desktop-item.h>
#include <gio/gio.h>
#include <gdk/gdkkeysyms.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>

#include <glib/gi18n-lib.h>

#include "app-shell.h"
#include "shell-window.h"
#include "app-resizer.h"
#include "slab-section.h"
#include "slab-mate-util.h"
#include "search-bar.h"

#include "application-tile.h"
#include "themed-icon.h"

#define TILE_EXEC_NAME "Tile_desktop_exec_name"
#define SECONDS_IN_DAY 86400
#define EXIT_SHELL_ON_ACTION_START "exit_shell_on_action_start"
#define EXIT_SHELL_ON_ACTION_HELP "exit_shell_on_action_help"
#define EXIT_SHELL_ON_ACTION_ADD_REMOVE "exit_shell_on_action_add_remove"
#define EXIT_SHELL_ON_ACTION_UPGRADE_UNINSTALL "exit_shell_on_action_upgrade_uninstall"
#define NEW_APPS_FILE_KEY "new_apps_file_key"

static void create_application_category_sections (AppShellData * app_data);
static GtkWidget *create_filter_section (AppShellData * app_data, const gchar * title);
static GtkWidget *create_groups_section (AppShellData * app_data, const gchar * title);
static GtkWidget *create_actions_section (AppShellData * app_data, const gchar * title,
	void (*actions_handler) (Tile *, TileEvent *, gpointer));

static void generate_category (const char * category, MateMenuTreeDirectory * root_dir, AppShellData * app_data, gboolean recursive);
static void generate_launchers (MateMenuTreeDirectory * root_dir, AppShellData * app_data,
	CategoryData * cat_data, gboolean recursive);
static void generate_new_apps (AppShellData * app_data);
static void insert_launcher_into_category (CategoryData * cat_data, MateDesktopItem * desktop_item,
	AppShellData * app_data);

static gboolean main_keypress_callback (GtkWidget * widget, GdkEventKey * event,
	AppShellData * app_data);
static gboolean main_delete_callback (GtkWidget * widget, GdkEvent * event,
	AppShellData * app_data);
static void application_launcher_clear_search_bar (AppShellData * app_data);
static void launch_selected_app (AppShellData * app_data);
static void generate_potential_apps (gpointer catdata, gpointer user_data);

static void relayout_shell (AppShellData * app_data);
static gboolean handle_filter_changed (NldSearchBar * search_bar, int context, const char *text,
	gpointer user_data);
static void handle_group_clicked (Tile * tile, TileEvent * event, gpointer user_data);
static void set_state (AppShellData * app_data, GtkWidget * widget);
static void populate_groups_section (AppShellData * app_data);
static void generate_filtered_lists (gpointer catdata, gpointer user_data);
static void show_no_results_message (AppShellData * app_data, GtkWidget * containing_vbox);
static void populate_application_category_sections (AppShellData * app_data,
	GtkWidget * containing_vbox);
static void populate_application_category_section (AppShellData * app_data, SlabSection * section,
	GList * launcher_list);
static void tile_activated_cb (Tile * tile, TileEvent * event, gpointer user_data);
static void handle_launcher_single_clicked (Tile * launcher, gpointer data);
static void handle_menu_action_performed (Tile * launcher, TileEvent * event, TileAction * action,
	gpointer data);
static gint application_launcher_compare (gconstpointer a, gconstpointer b);
static void matemenu_tree_changed_callback (MateMenuTree * tree, gpointer user_data);
gboolean regenerate_categories (AppShellData * app_data);

void
hide_shell (AppShellData * app_data)
{
	gtk_window_get_position (GTK_WINDOW (app_data->main_app),
		&app_data->main_app_window_x, &app_data->main_app_window_y);
	/* printf("x:%d, y:%d\n", app_data->main_app_window_x, app_data->main_app_window_y); */
	/* clear the search bar now so reshowing is fast and flicker free - BNC#283186 */
	application_launcher_clear_search_bar (app_data);
	gtk_widget_hide (app_data->main_app);
}

void
show_shell (AppShellData * app_data)
{
	gtk_widget_show_all (app_data->main_app);
	if (!app_data->static_actions)
		gtk_widget_hide_all (app_data->actions_section);  /* don't show unless a launcher is selected */

	if (app_data->main_app_window_shown_once)
		gtk_window_move (GTK_WINDOW (app_data->main_app),
			app_data->main_app_window_x, app_data->main_app_window_y);

	/* if this is the first time shown, need to clear this handler */
	else
		shell_window_clear_resize_handler (SHELL_WINDOW (app_data->shell));
	app_data->main_app_window_shown_once = TRUE;
}

gboolean
create_main_window (AppShellData * app_data, const gchar * app_name, const gchar * title,
	const gchar * window_icon, gint width, gint height, gboolean hidden)
{
	GtkWidget *main_app = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	app_data->main_app = main_app;
	gtk_widget_set_name (main_app, app_name);
	gtk_window_set_title (GTK_WINDOW (main_app), title);
	/* gtk_window_set_default_size(GTK_WINDOW(main_app), width, height); */
	gtk_window_set_icon_name (GTK_WINDOW (main_app), window_icon);
	gtk_container_add (GTK_CONTAINER (main_app), app_data->shell);

	g_signal_connect (main_app, "delete-event", G_CALLBACK (main_delete_callback), app_data);
	g_signal_connect (main_app, "key-press-event", G_CALLBACK (main_keypress_callback),
		app_data);

	gtk_window_set_position (GTK_WINDOW (app_data->main_app), GTK_WIN_POS_CENTER);
	if (!hidden)
		show_shell (app_data);

	return TRUE;
}

static void
generate_potential_apps (gpointer catdata, gpointer user_data)
{
	GHashTable *app_hash = (GHashTable *) user_data;
	CategoryData *data = (CategoryData *) catdata;
	gchar *uri;

	GList *launcher_list = data->filtered_launcher_list;

	while (launcher_list)
	{
		g_object_get (launcher_list->data, "tile-uri", &uri, NULL);
		/* eliminate dups of same app in multiple categories */
		if (!g_hash_table_lookup (app_hash, uri))
			g_hash_table_insert (app_hash, uri, launcher_list->data);
		else
			g_free (uri);
		launcher_list = g_list_next (launcher_list);
	}
}

static gboolean
return_first_entry (gpointer key, gpointer value, gpointer unused)
{
	return TRUE;	/*better way to pull an entry out ? */
}

static void
launch_selected_app (AppShellData * app_data)
{
	GHashTable *app_hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
	guint num_apps;

	g_list_foreach (app_data->categories_list, generate_potential_apps, app_hash);
	num_apps = g_hash_table_size (app_hash);
	if (num_apps == 1)
	{
		ApplicationTile *launcher =
			APPLICATION_TILE (g_hash_table_find (app_hash, return_first_entry, NULL));
		g_hash_table_destroy (app_hash);
		handle_launcher_single_clicked (TILE (launcher), app_data);
		return;
	}

	g_hash_table_destroy (app_hash);
}

static gboolean
main_keypress_callback (GtkWidget * widget, GdkEventKey * event, AppShellData * app_data)
{
	if (event->keyval == GDK_Return)
	{
		SlabSection *section = SLAB_SECTION (app_data->filter_section);
		NldSearchBar *search_bar;

		/* Make sure our implementation has not changed */
		g_assert (NLD_IS_SEARCH_BAR (section->contents));
		search_bar = NLD_SEARCH_BAR (section->contents);
		if (nld_search_bar_has_focus (search_bar))
		{
			launch_selected_app (app_data);
			return TRUE;
		}
	}

	/* quit on ESC or Ctl-W or Ctl-Q */
	if (event->keyval == GDK_Escape ||
		((event->keyval == GDK_w || event->keyval == GDK_W)	&& (event->state & GDK_CONTROL_MASK)) ||
		((event->keyval == GDK_q || event->keyval == GDK_Q) && (event->state & GDK_CONTROL_MASK)))
	{
		if (app_data->exit_on_close)
			gtk_main_quit ();
		else
			hide_shell (app_data);
		return TRUE;
	}
	return FALSE;
}

static gboolean
main_delete_callback (GtkWidget * widget, GdkEvent * event, AppShellData * app_data)
{
	if (app_data->exit_on_close)
	{
		gtk_main_quit ();
		return FALSE;
	}

	hide_shell (app_data);
	return TRUE;		/* stop the processing of this event */
}

void
layout_shell (AppShellData * app_data, const gchar * filter_title, const gchar * groups_title,
	const gchar * actions_title, GSList * actions,
	void (*actions_handler) (Tile *, TileEvent *, gpointer))
{
	GtkWidget *filter_section;
	GtkWidget *groups_section;
	GtkWidget *actions_section;

	GtkWidget *left_vbox;
	GtkWidget *right_vbox;
	gint num_cols;

	GtkWidget *sw;
	GtkAdjustment *adjustment;

	app_data->shell = shell_window_new (app_data);
	app_data->static_actions = actions;

	right_vbox = gtk_vbox_new (FALSE, CATEGORY_SPACING);

	num_cols = SIZING_SCREEN_WIDTH_LARGE_NUMCOLS;
	if (gdk_screen_width () <= SIZING_SCREEN_WIDTH_LARGE)
	{
		if (gdk_screen_width () <= SIZING_SCREEN_WIDTH_MEDIUM)
			num_cols = SIZING_SCREEN_WIDTH_SMALL_NUMCOLS;
		else
			num_cols = SIZING_SCREEN_WIDTH_MEDIUM_NUMCOLS;
	}
	app_data->category_layout =
		app_resizer_new (GTK_VBOX (right_vbox), num_cols, TRUE, app_data);

	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC,
		GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (sw), app_data->category_layout);
	adjustment = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (sw));
	g_object_set (adjustment, "step-increment", (double) 20, NULL);

	create_application_category_sections (app_data);
	populate_application_category_sections (app_data, right_vbox);
	app_resizer_set_table_cache (APP_RESIZER (app_data->category_layout),
		app_data->cached_tables_list);

	gtk_container_set_focus_vadjustment (GTK_CONTAINER (right_vbox),
		gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (sw)));

	left_vbox = gtk_vbox_new (FALSE, 15);

	filter_section = create_filter_section (app_data, filter_title);
	app_data->filter_section = filter_section;
	gtk_box_pack_start (GTK_BOX (left_vbox), filter_section, FALSE, FALSE, 0);

	groups_section = create_groups_section (app_data, groups_title);
	app_data->groups_section = groups_section;
	populate_groups_section (app_data);
	gtk_box_pack_start (GTK_BOX (left_vbox), groups_section, FALSE, FALSE, 0);

	actions_section = create_actions_section (app_data, actions_title, actions_handler);
	app_data->actions_section = actions_section;
	gtk_box_pack_start (GTK_BOX (left_vbox), actions_section, FALSE, FALSE, 0);

	shell_window_set_contents (SHELL_WINDOW (app_data->shell), left_vbox, sw);
}

static gboolean
relayout_shell_partial (gpointer user_data)
{
	AppShellData *app_data = (AppShellData *) user_data;
	GtkVBox *vbox = APP_RESIZER (app_data->category_layout)->child;
	CategoryData *data;

	if (app_data->stop_incremental_relayout)
		return FALSE;

	if (app_data->incremental_relayout_cat_list != NULL)
	{
		/* There are still categories to layout */
		data = (CategoryData *) app_data->incremental_relayout_cat_list->data;
		if (data->filtered_launcher_list != NULL)
		{
			populate_application_category_section (app_data, data->section,
				data->filtered_launcher_list);
			gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (data->section), TRUE, TRUE,
				0);
			app_data->filtered_out_everything = FALSE;
		}

		app_data->incremental_relayout_cat_list =
			g_list_next (app_data->incremental_relayout_cat_list);
		return TRUE;
	}

	/* We're done laying out the categories; finish up */
	if (app_data->filtered_out_everything)
		show_no_results_message (app_data, GTK_WIDGET (vbox));

	app_resizer_set_table_cache (APP_RESIZER (app_data->category_layout),
		app_data->cached_tables_list);
	populate_groups_section (app_data);

	gtk_widget_show_all (app_data->category_layout);
	gdk_window_set_cursor (app_data->shell->window, NULL);

	app_data->stop_incremental_relayout = TRUE;
	return FALSE;
}

static void
relayout_shell_incremental (AppShellData * app_data)
{
	GtkVBox *vbox = APP_RESIZER (app_data->category_layout)->child;

	app_data->stop_incremental_relayout = FALSE;
	app_data->filtered_out_everything = TRUE;
	app_data->incremental_relayout_cat_list = app_data->categories_list;

	if (app_data->cached_tables_list)
		g_list_free (app_data->cached_tables_list);
	app_data->cached_tables_list = NULL;

	remove_container_entries (GTK_CONTAINER (vbox));

	g_idle_add ((GSourceFunc) relayout_shell_partial, app_data);
}

static void
relayout_shell (AppShellData * app_data)
{
	GtkWidget *shell = app_data->shell;
	GtkVBox *vbox = APP_RESIZER (app_data->category_layout)->child;

	populate_application_category_sections (app_data, GTK_WIDGET (vbox));
	app_resizer_set_table_cache (APP_RESIZER (app_data->category_layout),
		app_data->cached_tables_list);
	populate_groups_section (app_data);

	gtk_widget_show_all (shell);
	if (!app_data->static_actions && !app_data->last_clicked_launcher)
		gtk_widget_hide_all (app_data->actions_section);  /* don't show unless a launcher is selected */
}

static GtkWidget *
create_actions_section (AppShellData * app_data, const gchar * title,
	void (*actions_handler) (Tile *, TileEvent *, gpointer))
{
	GtkWidget *section, *launcher;
	GtkWidget *vbox;
	GSList *actions;
	AppAction *action;
	AtkObject *a11y_cat;

	g_assert (app_data != NULL);

	section = slab_section_new (title, Style1);
	g_object_ref (section);

	vbox = gtk_vbox_new (FALSE, 0);
	slab_section_set_contents (SLAB_SECTION (section), vbox);

	if (app_data->static_actions)
	{
		for (actions = app_data->static_actions; actions; actions = actions->next)
		{
			GtkWidget *header;

			action = (AppAction *) actions->data;
			header = gtk_label_new (action->name);
			gtk_misc_set_alignment (GTK_MISC (header), 0, 0.5);
			launcher = nameplate_tile_new (NULL, NULL, header, NULL);

			g_object_set_data (G_OBJECT (launcher), APP_ACTION_KEY, action->item);
			g_signal_connect (launcher, "tile-activated", G_CALLBACK (actions_handler),
				app_data);
			gtk_box_pack_start (GTK_BOX (vbox), launcher, FALSE, FALSE, 0);

			a11y_cat = gtk_widget_get_accessible (GTK_WIDGET (launcher));
			atk_object_set_name (a11y_cat, action->name);
		}
	}

	return section;
}

static GtkWidget *
create_groups_section (AppShellData * app_data, const gchar * title)
{
	GtkWidget *section;
	GtkWidget *vbox;

	g_assert (app_data != NULL);

	section = slab_section_new (title, Style1);
	g_object_ref (section);

	vbox = gtk_vbox_new (FALSE, 0);
	slab_section_set_contents (SLAB_SECTION (section), vbox);

	return section;
}

static void
populate_groups_section (AppShellData * app_data)
{
	SlabSection *section = SLAB_SECTION (app_data->groups_section);
	GtkVBox *vbox;
	GList *cat_list;

	/* Make sure our implementation has not changed and it's still a GtkVBox */
	g_assert (GTK_IS_VBOX (section->contents));

	vbox = GTK_VBOX (section->contents);
	remove_container_entries (GTK_CONTAINER (vbox));

	cat_list = app_data->categories_list;
	do
	{
		CategoryData *data = (CategoryData *) cat_list->data;
		if (NULL != data->filtered_launcher_list)
		{
			gtk_widget_set_state (GTK_WIDGET (data->group_launcher), GTK_STATE_NORMAL);
			gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (data->group_launcher),
				FALSE, FALSE, 0);
		}
	}
	while (NULL != (cat_list = g_list_next (cat_list)));
}

static void
handle_group_clicked (Tile * tile, TileEvent * event, gpointer user_data)
{
	AppShellData *app_data = (AppShellData *) user_data;
	GtkWidget *section = NULL;

	gint clicked_pos =
		GPOINTER_TO_INT (g_object_get_data (G_OBJECT (tile), GROUP_POSITION_NUMBER_KEY));

	GList *cat_list = app_data->categories_list;

	gint total = 0;
	do
	{
		CategoryData *cat_data = (CategoryData *) cat_list->data;
		gint pos =
			GPOINTER_TO_INT (g_object_get_data (G_OBJECT (cat_data->group_launcher),
				GROUP_POSITION_NUMBER_KEY));
		if (pos == clicked_pos)
		{
			section = GTK_WIDGET (cat_data->section);
			break;
		}

		if (NULL != cat_data->filtered_launcher_list)
		{
			total += GTK_WIDGET (cat_data->section)->allocation.height +
				CATEGORY_SPACING;
		}
	}
	while (NULL != (cat_list = g_list_next (cat_list)));

	g_assert (section != NULL);
	set_state (app_data, section);

	app_resizer_set_vadjustment_value (app_data->category_layout, total);
}

static void
set_state (AppShellData * app_data, GtkWidget * widget)
{
	if (app_data->selected_group)
	{
		slab_section_set_selected (app_data->selected_group, FALSE);
		app_data->selected_group = NULL;
	}

	if (widget)
	{
		app_data->selected_group = SLAB_SECTION (widget);
		slab_section_set_selected (SLAB_SECTION (widget), TRUE);
	}
	gtk_widget_queue_draw (app_data->shell);
}

static GtkWidget *
create_filter_section (AppShellData * app_data, const gchar * title)
{
	GtkWidget *section;

	GtkWidget *search_bar;

	section = slab_section_new (title, Style1);
	g_object_ref (section);

	search_bar = nld_search_bar_new ();
	nld_search_bar_set_search_timeout (NLD_SEARCH_BAR (search_bar), 0);
	slab_section_set_contents (SLAB_SECTION (section), search_bar);

	g_signal_connect (G_OBJECT (search_bar), "search", G_CALLBACK (handle_filter_changed),
		app_data);

	return section;
}

static gboolean
handle_filter_changed_delayed (gpointer user_data)
{
	AppShellData *app_data = (AppShellData *) user_data;

	g_list_foreach (app_data->categories_list, generate_filtered_lists,
		(gpointer) app_data->filter_string);
	app_data->last_clicked_launcher = NULL;

	/*  showing the updates incremtally is very visually distracting. Much worse than just blanking until
	   the incremental work is done and then doing one show. It would be nice to optimize this though
	   somehow and not even show any change but the cursor change until all the work is done. But since
	   we do the work incrementally in an idle loop I don't know how else besides hiding to not show
	   incremental updates
	 */
	/* gdk_window_freeze_updates(app_data->category_layout->window); */
	gtk_widget_hide (app_data->category_layout);
	app_data->busy_cursor =
		gdk_cursor_new_for_display (gtk_widget_get_display (app_data->shell), GDK_WATCH);
	gdk_window_set_cursor (app_data->shell->window, app_data->busy_cursor);
	gdk_cursor_unref (app_data->busy_cursor);

	set_state (app_data, NULL);
	app_resizer_set_vadjustment_value (app_data->category_layout, 0);

	relayout_shell_incremental (app_data);

	app_data->filter_changed_timeout = 0;
	return FALSE;
}

static gboolean
handle_filter_changed (NldSearchBar * search_bar, int context, const char *text, gpointer data)
{
	AppShellData *app_data;

	app_data = (AppShellData *) data;

	if (app_data->filter_string)
		g_free (app_data->filter_string);
	app_data->filter_string = g_strdup (text);

	if (app_data->filter_changed_timeout)
		g_source_remove (app_data->filter_changed_timeout);

	app_data->filter_changed_timeout =
		g_timeout_add (75, handle_filter_changed_delayed, app_data);
	app_data->stop_incremental_relayout = TRUE;

	return FALSE;
}

static void
generate_filtered_lists (gpointer catdata, gpointer user_data)
{
	CategoryData *data = (CategoryData *) catdata;

	/* Fixme - everywhere you use ascii you need to fix up for multibyte */
	gchar *filter_string = g_ascii_strdown (user_data, -1);
	gchar *temp1, *temp2;
	GList *launcher_list = data->launcher_list;

	g_list_free (data->filtered_launcher_list);
	data->filtered_launcher_list = NULL;

	do
	{
		ApplicationTile *launcher = APPLICATION_TILE (launcher_list->data);
		const gchar *filename;

		temp1 = NULL;
		temp2 = NULL;

		/* Since the filter may remove this entry from the
		   container it will not get a mouse out event */
		gtk_widget_set_state (GTK_WIDGET (launcher), GTK_STATE_NORMAL);
		filename = g_object_get_data (G_OBJECT (launcher), TILE_EXEC_NAME); /* do I need to free this */

		temp1 = g_ascii_strdown (launcher->name, -1);
		if (launcher->description)
			temp2 = g_ascii_strdown (launcher->description, -1);
		if (g_strrstr (temp1, filter_string) || (launcher->description
				&& g_strrstr (temp2, filter_string))
			|| g_strrstr (filename, filter_string))
		{
			data->filtered_launcher_list =
				g_list_append (data->filtered_launcher_list, launcher);
		}
		if (temp1)
			g_free (temp1);
		if (temp2)
			g_free (temp2);
	}
	while (NULL != (launcher_list = g_list_next (launcher_list)));
	g_free (filter_string);
}

static void
delete_old_data (AppShellData * app_data)
{
	GList *temp;
	GList *cat_list;

	g_assert (app_data != NULL);
	g_assert (app_data->categories_list != NULL);

	cat_list = app_data->categories_list;

	do
	{
		CategoryData *data = (CategoryData *) cat_list->data;
		gtk_widget_destroy (GTK_WIDGET (data->section));
		gtk_widget_destroy (GTK_WIDGET (data->group_launcher));
		g_object_unref (data->section);
		g_object_unref (data->group_launcher);
		g_free (data->category);

		for (temp = data->launcher_list; temp; temp = g_list_next (temp))
		{
			g_free (g_object_get_data (G_OBJECT (temp->data), TILE_EXEC_NAME));
			g_object_unref (temp->data);
		}

		g_list_free (data->launcher_list);
		g_list_free (data->filtered_launcher_list);
		g_free (data);
	}
	while (NULL != (cat_list = g_list_next (cat_list)));

	g_list_free (app_data->categories_list);
	app_data->categories_list = NULL;
	app_data->selected_group = NULL;
}

static void
create_application_category_sections (AppShellData * app_data)
{
	GList *cat_list;
	AtkObject *a11y_cat;
	gint pos = 0;

	g_assert (app_data != NULL);
	g_assert (app_data->categories_list != NULL);	/* Fixme - pop up a dialog box and then close */

	cat_list = app_data->categories_list;

	do
	{
		CategoryData *data = (CategoryData *) cat_list->data;
		GtkWidget *header = gtk_label_new (data->category);
		gchar *markup;
		GtkWidget *hbox;
		GtkWidget *table;

		gtk_misc_set_alignment (GTK_MISC (header), 0, 0.5);
		data->group_launcher = TILE (nameplate_tile_new (NULL, NULL, header, NULL));
		g_object_ref (data->group_launcher);

		g_object_set_data (G_OBJECT (data->group_launcher), GROUP_POSITION_NUMBER_KEY,
			GINT_TO_POINTER (pos));
		pos++;
		g_signal_connect (data->group_launcher, "tile-activated",
			G_CALLBACK (handle_group_clicked), app_data);
		a11y_cat = gtk_widget_get_accessible (GTK_WIDGET (data->group_launcher));
		atk_object_set_name (a11y_cat, data->category);

		markup = g_markup_printf_escaped ("<span size=\"x-large\" weight=\"bold\">%s</span>",
			data->category);
		data->section = SLAB_SECTION (slab_section_new_with_markup (markup, Style2));

		/* as we filter these will be added/removed from parent container and we dont want them destroyed */
		g_object_ref (data->section);
		g_free (markup);

		hbox = gtk_hbox_new (FALSE, 0);
		table = gtk_table_new (0, 0, TRUE);
		gtk_table_set_col_spacings (GTK_TABLE (table), 5);
		gtk_table_set_row_spacings (GTK_TABLE (table), 5);
		gtk_box_pack_start (GTK_BOX (hbox), table, FALSE, FALSE, 15);
		slab_section_set_contents (SLAB_SECTION (data->section), hbox);
	}
	while (NULL != (cat_list = g_list_next (cat_list)));
}

static void
show_no_results_message (AppShellData * app_data, GtkWidget * containing_vbox)
{
	gchar *markup;
	gchar *str1;
	gchar *str2;

	if (!app_data->filtered_out_everything_widget)
	{
		GtkWidget *hbox;
		GtkWidget *image;
		GtkWidget *label;

		app_data->filtered_out_everything_widget = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
		g_object_ref (app_data->filtered_out_everything_widget);

		hbox = gtk_hbox_new (FALSE, 0);
		image = themed_icon_new ("face-surprise", GTK_ICON_SIZE_DIALOG);
		gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);

		label = gtk_label_new (NULL);
		gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
		gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 15);
		app_data->filtered_out_everything_widget_label = GTK_LABEL (label);

		gtk_container_add (GTK_CONTAINER (app_data->filtered_out_everything_widget), hbox);
	}

	str1 = g_markup_printf_escaped ("<b>%s</b>", app_data->filter_string);
	str2 = g_strdup_printf (_("Your filter \"%s\" does not match any items."), str1);
	markup = g_strdup_printf ("<span size=\"large\"><b>%s</b></span>\n\n%s",
		_("No matches found."), str2);
	gtk_label_set_text (app_data->filtered_out_everything_widget_label, markup);
	gtk_label_set_use_markup (app_data->filtered_out_everything_widget_label, TRUE);
	gtk_box_pack_start (GTK_BOX (containing_vbox), app_data->filtered_out_everything_widget,
		TRUE, TRUE, 0);
	g_free (str1);
	g_free (str2);
	g_free (markup);
}

static void
populate_application_category_sections (AppShellData * app_data, GtkWidget * containing_vbox)
{
	GList *cat_list = app_data->categories_list;
	gboolean filtered_out_everything = TRUE;
	if (app_data->cached_tables_list)
		g_list_free (app_data->cached_tables_list);
	app_data->cached_tables_list = NULL;

	remove_container_entries (GTK_CONTAINER (containing_vbox));
	do
	{
		CategoryData *data = (CategoryData *) cat_list->data;
		if (NULL != data->filtered_launcher_list)
		{
			populate_application_category_section (app_data, data->section,
				data->filtered_launcher_list);
			gtk_box_pack_start (GTK_BOX (containing_vbox), GTK_WIDGET (data->section),
				TRUE, TRUE, 0);
			filtered_out_everything = FALSE;
		}
	}
	while (NULL != (cat_list = g_list_next (cat_list)));

	if (TRUE == filtered_out_everything)
		show_no_results_message (app_data, containing_vbox);
}

static void
populate_application_category_section (AppShellData * app_data, SlabSection * section,
	GList * launcher_list)
{
	GtkWidget *hbox;
	GtkTable *table;
	GList *children;

	g_assert (GTK_IS_HBOX (section->contents));
	hbox = GTK_WIDGET (section->contents);

	children = gtk_container_get_children (GTK_CONTAINER (hbox));
	table = children->data;
	g_list_free (children);

	/* Make sure our implementation has not changed and it's still a GtkTable */
	g_assert (GTK_IS_TABLE (table));

	app_data->cached_tables_list = g_list_append (app_data->cached_tables_list, table);

	app_resizer_layout_table_default (APP_RESIZER (app_data->category_layout), table,
		launcher_list);

}

gboolean
regenerate_categories (AppShellData * app_data)
{
	delete_old_data (app_data);
	generate_categories (app_data);
	create_application_category_sections (app_data);
	relayout_shell (app_data);

	return FALSE;	/* remove this function from the list */
}

static void
matemenu_tree_changed_callback (MateMenuTree * old_tree, gpointer user_data)
{
	/*
	This method only gets called on the first change (matemenu appears to ignore subsequent) until
	we reget the root dir which we can't do in this method because if we do for some reason this
	method then gets called multiple times for one actual change. This actually is okay because
	it's probably a good idea to wait a couple seconds to regenerate the categories in case there
	are multiple quick changes being made, no sense regenerating multiple times.
	*/
	g_timeout_add_full (G_PRIORITY_DEFAULT_IDLE, 3000, (GSourceFunc) regenerate_categories,
		user_data, NULL);
}

AppShellData *
appshelldata_new (const gchar * menu_name, NewAppConfig * new_apps, const gchar * mateconf_keys_prefix,
	GtkIconSize icon_size, gboolean show_tile_generic_name, gboolean exit_on_close)
{
	AppShellData *app_data = g_new0 (AppShellData, 1);
	app_data->mateconf_prefix = mateconf_keys_prefix;
	app_data->new_apps = new_apps;
	app_data->menu_name = menu_name;
	app_data->icon_size = icon_size;
	app_data->stop_incremental_relayout = TRUE;
	app_data->show_tile_generic_name = show_tile_generic_name;
	app_data->exit_on_close = exit_on_close;
	return app_data;
}

void
generate_categories (AppShellData * app_data)
{
	MateMenuTreeDirectory *root_dir;
	GSList *contents, *l;
	gboolean need_misc = FALSE;

	if (!app_data->tree)
	{
		app_data->tree = matemenu_tree_lookup (app_data->menu_name, MATEMENU_TREE_FLAGS_NONE);
		matemenu_tree_add_monitor (app_data->tree, matemenu_tree_changed_callback, app_data);
	}
	root_dir = matemenu_tree_get_root_directory (app_data->tree);
	if (root_dir)
		contents = matemenu_tree_directory_get_contents (root_dir);
	else
		contents = NULL;
	if (!root_dir || !contents)
	{
		GtkWidget *dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Failure loading - %s",
			app_data->menu_name);
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		exit (1);	/* Fixme - is there a MATE/GTK way to do this. */
	}

	for (l = contents; l; l = l->next)
	{
		const char *category;
		MateMenuTreeItem *item = l->data;

		switch (matemenu_tree_item_get_type (item))
		{
		case MATEMENU_TREE_ITEM_DIRECTORY:
			category = matemenu_tree_directory_get_name ((MateMenuTreeDirectory*)item);
			generate_category(category, (MateMenuTreeDirectory*)item, app_data, TRUE);
			break;
		case MATEMENU_TREE_ITEM_ENTRY:
			need_misc = TRUE;
			break;
		default:
			break;
		}

		matemenu_tree_item_unref (item);
	}
	g_slist_free (contents);

	if (need_misc)
		generate_category (_("Other"), root_dir, app_data, FALSE);

	if (app_data->hash)
	{
		g_hash_table_destroy (app_data->hash);
		app_data->hash = NULL;
	}

	matemenu_tree_item_unref (root_dir);

	if (app_data->new_apps && (app_data->new_apps->max_items > 0))
		generate_new_apps (app_data);
}

static void
generate_category (const char * category, MateMenuTreeDirectory * root_dir, AppShellData * app_data, gboolean recursive)
{
	CategoryData *data;
	/* This is not needed. MateMenu already returns an ordered, non duplicate list
	GList *list_entry;
	list_entry =
		g_list_find_custom (app_data->categories_list, category,
		category_name_compare);
	if (!list_entry)
	{
	*/
		data = g_new0 (CategoryData, 1);
		data->category = g_strdup (category);
		app_data->categories_list =
			/* use the matemenu order instead of alphabetical */
			g_list_append (app_data->categories_list, data);
			/* g_list_insert_sorted (app_data->categories_list, data, category_data_compare); */
	/*
	}
	else
	{
		data = list_entry->data;
	}
	*/

	if (app_data->hash)	/* used to eliminate dups on a per category basis. */
		g_hash_table_destroy (app_data->hash);
	app_data->hash = g_hash_table_new (g_str_hash, g_str_equal);
	generate_launchers (root_dir, app_data, data, recursive);
}

static gboolean
check_specific_apps_hack (MateDesktopItem * item)
{
	static const gchar *COMMAND_LINE_LOCKDOWN_MATECONF_KEY =
		"/desktop/mate/lockdown/disable_command_line";
	static const gchar *COMMAND_LINE_LOCKDOWN_DESKTOP_CATEGORY = "TerminalEmulator";
	static gboolean got_lockdown_value = FALSE;
	static gboolean command_line_lockdown;

	gchar *path;
	const char *exec;

	if (!got_lockdown_value)
	{
		got_lockdown_value = TRUE;
		command_line_lockdown = get_slab_mateconf_bool (COMMAND_LINE_LOCKDOWN_MATECONF_KEY);
	}

	/* This seems like an ugly hack but it's the way it's currently done in the old control center */
	exec = mate_desktop_item_get_string (item, MATE_DESKTOP_ITEM_EXEC);

	/* discard xscreensaver if mate-screensaver is installed */
	if ((exec && !strcmp (exec, "xscreensaver-demo"))
		&& (path = g_find_program_in_path ("mate-screensaver-preferences")))
	{
		g_free (path);
		return TRUE;
	}

	/* discard mate-keyring-manager if CASA is installed */
	if ((exec && !strcmp (exec, "mate-keyring-manager"))
		&& (path = g_find_program_in_path ("CASAManager.sh")))
	{
		g_free (path);
		return TRUE;
	}

	/* discard terminals if lockdown key is set */
	if (command_line_lockdown)
	{
		const gchar *categories =
			mate_desktop_item_get_string (item, MATE_DESKTOP_ITEM_CATEGORIES);
		if (g_strrstr (categories, COMMAND_LINE_LOCKDOWN_DESKTOP_CATEGORY))
		{
			/* printf ("eliminating %s\n", mate_desktop_item_get_location (item)); */
			return TRUE;
		}
	}

	return FALSE;
}

static void
generate_launchers (MateMenuTreeDirectory * root_dir, AppShellData * app_data, CategoryData * cat_data, gboolean recursive)
{
	MateDesktopItem *desktop_item;
	const gchar *desktop_file;
	GSList *contents, *l;

	contents = matemenu_tree_directory_get_contents (root_dir);
	for (l = contents; l; l = l->next)
	{
		switch (matemenu_tree_item_get_type (l->data))
		{
		case MATEMENU_TREE_ITEM_DIRECTORY:
			/* g_message ("Found sub-category %s", matemenu_tree_directory_get_name (l->data)); */
			if (recursive)
				generate_launchers (l->data, app_data, cat_data, TRUE);
			break;
		case MATEMENU_TREE_ITEM_ENTRY:
			/* g_message ("Found item name is:%s", matemenu_tree_entry_get_name (l->data)); */
			desktop_file = matemenu_tree_entry_get_desktop_file_path (l->data);
			if (desktop_file)
			{
				if (g_hash_table_lookup (app_data->hash, desktop_file))
				{
					break;	/* duplicate */
				}
				/* Fixme - make sure it's safe to store this without duping it. As far as I can tell it is
				   safe as long as I don't hang on to this anylonger than I hang on to the MateMenuTreeEntry*
				   which brings up another point - am I supposed to free these or does freeing the top level recurse
				*/
				g_hash_table_insert (app_data->hash, (gpointer) desktop_file,
					(gpointer) desktop_file);
			}
			desktop_item = mate_desktop_item_new_from_file (desktop_file, 0, NULL);
			if (!desktop_item)
			{
				g_critical ("Failure - mate_desktop_item_new_from_file(%s)",
					    desktop_file);
				break;
			}
			if (!check_specific_apps_hack (desktop_item))
				insert_launcher_into_category (cat_data, desktop_item, app_data);
			mate_desktop_item_unref (desktop_item);
			break;
		default:
			break;
		}

		matemenu_tree_item_unref (l->data);
	}
	g_slist_free (contents);
}

static void
generate_new_apps (AppShellData * app_data)
{
	GHashTable *all_apps_cache = NULL;
	gchar *all_apps;
	GError *error = NULL;
	gchar *separator = "\n";
	gchar *mateconf_key;

	gchar *basename;
	gchar *all_apps_file_name;
	gchar **all_apps_split;
	gint x;
	gboolean got_new_apps;
	CategoryData *new_apps_category = NULL;
	GList *categories, *launchers;
	GHashTable *new_apps_dups;

	mateconf_key = g_strdup_printf ("%s%s", app_data->mateconf_prefix, NEW_APPS_FILE_KEY);
	basename = get_slab_mateconf_string (mateconf_key);
	g_free (mateconf_key);
	if (!basename)
	{
		g_warning ("Failure getting mateconf key NEW_APPS_FILE_KEY");
		return;
	}

	all_apps_file_name = g_build_filename (g_get_home_dir (), basename, NULL);
	g_free (basename);

	if (!g_file_get_contents (all_apps_file_name, &all_apps, NULL, &error))
	{
		/* If file does not exist, this is the first time this user has run this, create the baseline file */
		GList *categories, *launchers;
		GString *gstr;
		gchar *dirname;

		g_error_free (error);
		error = NULL;

		/* best initial size determined by running on a couple different platforms */
		gstr = g_string_sized_new (10000);

		for (categories = app_data->categories_list; categories; categories = categories->next)
		{
			CategoryData *data = categories->data;
			for (launchers = data->launcher_list; launchers; launchers = launchers->next)
			{
				Tile *tile = TILE (launchers->data);
				MateDesktopItem *item =
					application_tile_get_desktop_item (APPLICATION_TILE (tile));
				const gchar *uri = mate_desktop_item_get_location (item);
				g_string_append (gstr, uri);
				g_string_append (gstr, separator);
			}
		}

		dirname = g_path_get_dirname (all_apps_file_name);
		g_mkdir_with_parents (dirname, 0700);	/* creates if does not exist */
		g_free (dirname);

		if (!g_file_set_contents (all_apps_file_name, gstr->str, -1, &error))
			g_warning ("Error setting all apps file:%s\n", error->message);

		g_string_free (gstr, TRUE);
		g_free (all_apps_file_name);
		return;
	}

	all_apps_cache = g_hash_table_new (g_str_hash, g_str_equal);
	all_apps_split = g_strsplit (all_apps, separator, -1);
	for (x = 0; all_apps_split[x]; x++)
	{
		g_hash_table_insert (all_apps_cache, all_apps_split[x], all_apps_split[x]);
	}

	got_new_apps = FALSE;
	new_apps_dups = g_hash_table_new (g_str_hash, g_str_equal);
	for (categories = app_data->categories_list; categories; categories = categories->next)
	{
		CategoryData *cat_data = categories->data;
		for (launchers = cat_data->launcher_list; launchers; launchers = launchers->next)
		{
			Tile *tile = TILE (launchers->data);
			MateDesktopItem *item =
				application_tile_get_desktop_item (APPLICATION_TILE (tile));
			const gchar *uri = mate_desktop_item_get_location (item);
			if (!g_hash_table_lookup (all_apps_cache, uri))
			{
				GFile *file;
				GFileInfo *info;
				long filetime;

				if (g_hash_table_lookup (new_apps_dups, uri))
				{
					/* if a desktop file is in 2 or more top level categories, only show it once */
					/* printf("Discarding Newapp duplicate:%s\n", uri); */
					break;
				}
				g_hash_table_insert (new_apps_dups, (gpointer) uri, (gpointer) uri);

				if (!got_new_apps)
				{
					new_apps_category = g_new0 (CategoryData, 1);
					new_apps_category->category =
						g_strdup (app_data->new_apps->name);
					app_data->new_apps->garray =
						g_array_sized_new (FALSE, TRUE,
						sizeof (NewAppData *),
						app_data->new_apps->max_items);

					/* should not need this, but a bug in glib does not actually clear the elements until you call this method */
					g_array_set_size (app_data->new_apps->garray, app_data->new_apps->max_items);
					got_new_apps = TRUE;
				}

				file = g_file_new_for_uri (uri);
				info = g_file_query_info (file,
							  G_FILE_ATTRIBUTE_TIME_MODIFIED,
							  0, NULL, NULL);

				if (!info)
				{
					g_object_unref (file);
					g_warning ("Cant get vfs info for %s\n", uri);
					return;
				}
				filetime = (long) g_file_info_get_attribute_uint64 (info,
										    G_FILE_ATTRIBUTE_TIME_MODIFIED);
				g_object_unref (info);
				g_object_unref (file);

				for (x = 0; x < app_data->new_apps->max_items; x++)
				{
					NewAppData *temp_data = (NewAppData *)
						g_array_index (app_data->new_apps->garray, NewAppData *, x);
					if (!temp_data || filetime > temp_data->time)	/* if this slot is empty or we are newer than this slot */
					{
						NewAppData *temp = g_new0 (NewAppData, 1);
						temp->time = filetime;
						temp->item = item;
						g_array_insert_val (app_data->new_apps->garray, x,
							temp);
						break;
					}
				}
			}
		}
	}
	g_hash_table_destroy (new_apps_dups);
	g_hash_table_destroy (all_apps_cache);

	if (got_new_apps)
	{
		for (x = 0; x < app_data->new_apps->max_items; x++)
		{
			NewAppData *data =
				(NewAppData *) g_array_index (app_data->new_apps->garray,
				NewAppData *, x);
			if (data)
			{
				insert_launcher_into_category (new_apps_category, data->item,
					app_data);
				g_free (data);
			}
			else
				break;
		}
		app_data->categories_list =
			g_list_prepend (app_data->categories_list, new_apps_category);

		g_array_free (app_data->new_apps->garray, TRUE);
	}
	g_free (all_apps);
	g_free (all_apps_file_name);
	g_strfreev (all_apps_split);
}

static void
insert_launcher_into_category (CategoryData * cat_data, MateDesktopItem * desktop_item,
	AppShellData * app_data)
{
	GtkWidget *launcher;
	static GtkSizeGroup *icon_group = NULL;

	gchar *filepath;
	gchar *filename;
	GtkWidget *tile_icon;

	if (!icon_group)
		icon_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

	launcher =
		application_tile_new_full (mate_desktop_item_get_location (desktop_item),
		app_data->icon_size, app_data->show_tile_generic_name, app_data->mateconf_prefix);
	gtk_widget_set_size_request (launcher, SIZING_TILE_WIDTH, -1);

	filepath =
		g_strdup (mate_desktop_item_get_string (desktop_item, MATE_DESKTOP_ITEM_EXEC));
	g_strdelimit (filepath, " ", '\0');	/* just want the file name - no args or replacements */
	filename = g_strrstr (filepath, "/");
	if (filename)
		g_stpcpy (filepath, filename + 1);
	filename = g_ascii_strdown (filepath, -1);
	g_free (filepath);
	g_object_set_data (G_OBJECT (launcher), TILE_EXEC_NAME, filename);

	tile_icon = NAMEPLATE_TILE (launcher)->image;
	gtk_size_group_add_widget (icon_group, tile_icon);

	g_signal_connect (launcher, "tile-activated", G_CALLBACK (tile_activated_cb), app_data);

	/* Note that this will handle the case of the action being launched via the side panel as
	   well as directly from the context menu of an individual launcher, because they both
	   funnel through tile_button_action_activate.
	*/
	g_signal_connect (launcher, "tile-action-triggered",
		G_CALLBACK (handle_menu_action_performed), app_data);

	/* These will be inserted/removed from tables as the filter changes and we dont want them */
	/* destroyed when they are removed */
	g_object_ref (launcher);

	/* use alphabetical order instead of the matemenu order. We group all sub items in each top level
	category together, ignoring sub menus, so we also ignore sub menu layout hints */
	cat_data->launcher_list =
		/* g_list_insert (cat_data->launcher_list, launcher, -1); */
		g_list_insert_sorted (cat_data->launcher_list, launcher, application_launcher_compare);
	cat_data->filtered_launcher_list =
		/* g_list_insert (cat_data->filtered_launcher_list, launcher, -1); */
		g_list_insert_sorted (cat_data->filtered_launcher_list, launcher, application_launcher_compare);
}

static gint
application_launcher_compare (gconstpointer a, gconstpointer b)
{
	ApplicationTile *launcher1 = APPLICATION_TILE (a);
	ApplicationTile *launcher2 = APPLICATION_TILE (b);

	gchar *val1 = launcher1->name;
	gchar *val2 = launcher2->name;

	if (val1 == NULL || val2 == NULL)
	{
		g_assert_not_reached ();
	}
	return g_ascii_strcasecmp (val1, val2);
}

static void
application_launcher_clear_search_bar (AppShellData * app_data)
{
	SlabSection *section = SLAB_SECTION (app_data->filter_section);
	NldSearchBar *search_bar;
	g_assert (NLD_IS_SEARCH_BAR (section->contents));
	search_bar = NLD_SEARCH_BAR (section->contents);
	nld_search_bar_set_text (search_bar, "", TRUE);
}

/*
static gint
category_name_compare (gconstpointer a, gconstpointer b)
{
	CategoryData *data = (CategoryData *) a;
	const gchar *category = b;

	if (category == NULL || data->category == NULL)
	{
		g_assert_not_reached ();
	}
	return g_ascii_strcasecmp (category, data->category);
}
*/

static void
tile_activated_cb (Tile * tile, TileEvent * event, gpointer user_data)
{
	switch (event->type)
	{
	case TILE_EVENT_ACTIVATED_SINGLE_CLICK:
	case TILE_EVENT_ACTIVATED_KEYBOARD:
		handle_launcher_single_clicked (tile, user_data);
		break;
	default:
		break;
	}

}

static void
handle_launcher_single_clicked (Tile * launcher, gpointer data)
{
	AppShellData *app_data = (AppShellData *) data;
	gchar *mateconf_key;

	tile_trigger_action (launcher, launcher->actions[APPLICATION_TILE_ACTION_START]);

	mateconf_key = g_strdup_printf ("%s%s", app_data->mateconf_prefix, EXIT_SHELL_ON_ACTION_START);
	if (get_slab_mateconf_bool (mateconf_key))
	{
		if (app_data->exit_on_close)
			gtk_main_quit ();
		else
			hide_shell (app_data);
	}
	g_free (mateconf_key);
}

static void
handle_menu_action_performed (Tile * launcher, TileEvent * event, TileAction * action,
	gpointer data)
{
	AppShellData *app_data = (AppShellData *) data;
	gchar *temp;

	temp = NULL;
	if (action == launcher->actions[APPLICATION_TILE_ACTION_START])
	{
		temp = g_strdup_printf ("%s%s", app_data->mateconf_prefix, EXIT_SHELL_ON_ACTION_START);
	}

	else if (action == launcher->actions[APPLICATION_TILE_ACTION_HELP])
	{
		temp = g_strdup_printf ("%s%s", app_data->mateconf_prefix, EXIT_SHELL_ON_ACTION_HELP);
	}

	else if (action == launcher->actions[APPLICATION_TILE_ACTION_UPDATE_MAIN_MENU]
		|| action == launcher->actions[APPLICATION_TILE_ACTION_UPDATE_STARTUP])
	{
		temp = g_strdup_printf ("%s%s", app_data->mateconf_prefix,
			EXIT_SHELL_ON_ACTION_ADD_REMOVE);
	}

	else if (action == launcher->actions[APPLICATION_TILE_ACTION_UPGRADE_PACKAGE]
		|| action == launcher->actions[APPLICATION_TILE_ACTION_UNINSTALL_PACKAGE])
	{
		temp = g_strdup_printf ("%s%s", app_data->mateconf_prefix,
			EXIT_SHELL_ON_ACTION_UPGRADE_UNINSTALL);
	}

	if (temp)
	{
		if (get_slab_mateconf_bool (temp))
		{
			if (app_data->exit_on_close)
				gtk_main_quit ();
			else
				hide_shell (app_data);
		}
		g_free (temp);
	}
	else
		g_warning ("Unknown Action");
}
