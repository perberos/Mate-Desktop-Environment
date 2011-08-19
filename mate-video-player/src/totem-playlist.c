/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* totem-playlist.c

   Copyright (C) 2002, 2003, 2004, 2005 Bastien Nocera

   The Mate Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301  USA.

   Author: Bastien Nocera <hadess@hadess.net>
 */

#include "config.h"
#include "totem-playlist.h"
#include "totemplaylist-marshal.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <mateconf/mateconf-client.h>
#include <gio/gio.h>
#include <string.h>

#include "eggfileformatchooser.h"
#include "totem-dnd-menu.h"
#include "totem-uri.h"
#include "totem-interface.h"
#include "video-utils.h"

#define PL_LEN (gtk_tree_model_iter_n_children (playlist->priv->model, NULL))

static void ensure_shuffled (TotemPlaylist *playlist);
static gboolean totem_playlist_add_one_mrl (TotemPlaylist *playlist, const char *mrl, const char *display_name);

typedef gboolean (*ClearComparisonFunc) (TotemPlaylist *playlist, GtkTreeIter *iter, gconstpointer data);

static void totem_playlist_clear_with_compare (TotemPlaylist *playlist,
					       ClearComparisonFunc func,
					       gconstpointer data);

/* Callback function for GtkBuilder */
G_MODULE_EXPORT void totem_playlist_save_files (GtkWidget *widget, TotemPlaylist *playlist);
G_MODULE_EXPORT void totem_playlist_add_files (GtkWidget *widget, TotemPlaylist *playlist);
G_MODULE_EXPORT void playlist_remove_button_clicked (GtkWidget *button, TotemPlaylist *playlist);
G_MODULE_EXPORT void totem_playlist_up_files (GtkWidget *widget, TotemPlaylist *playlist);
G_MODULE_EXPORT void totem_playlist_down_files (GtkWidget *widget, TotemPlaylist *playlist);
G_MODULE_EXPORT void playlist_copy_location_action_callback (GtkAction *action, TotemPlaylist *playlist);
G_MODULE_EXPORT void playlist_select_subtitle_action_callback (GtkAction *action, TotemPlaylist *playlist);
G_MODULE_EXPORT void playlist_remove_action_callback (GtkAction *action, TotemPlaylist *playlist);


typedef struct {
	TotemPlaylist *playlist;
	TotemPlaylistForeachFunc callback;
	gpointer user_data;
} PlaylistForeachContext;

struct TotemPlaylistPrivate
{
	GtkWidget *treeview;
	GtkTreeModel *model;
	GtkTreePath *current;
	GtkTreeSelection *selection;
	TotemPlParser *parser;

	GtkActionGroup *action_group;
	GtkUIManager *ui_manager;

	/* Widgets */
	GtkWidget *save_button;
	GtkWidget *remove_button;
	GtkWidget *up_button;
	GtkWidget *down_button;

	/* These is the current paths for the file selectors */
	char *path;
	char *save_path;
	guint save_format;
	GtkWidget *file_chooser;

	/* Shuffle mode */
	int *shuffled;
	int current_shuffled, shuffle_len;

	MateConfClient *gc;

	/* Used to know the position for drops */
	GtkTreePath *tree_path;
	GtkTreeViewDropPosition drop_pos;

	/* Cursor ref: 0 if the cursor is unbusy; positive numbers indicate the number of nested calls to set_waiting_cursor() */
	guint cursor_ref;

	/* This is a scratch list for when we're removing files */
	GList *list;
	guint current_to_be_removed : 1;

	guint disable_save_to_disk : 1;

	/* Repeat mode */
	guint repeat : 1;

	/* Reorder Flag */
	guint drag_started : 1;

	/* Drop disabled flag */
	guint drop_disabled : 1;

	/* Shuffle mode */
	guint shuffle : 1;
};

/* Signals */
enum {
	CHANGED,
	ITEM_ACTIVATED,
	ACTIVE_NAME_CHANGED,
	CURRENT_REMOVED,
	REPEAT_TOGGLED,
	SHUFFLE_TOGGLED,
	SUBTITLE_CHANGED,
	ITEM_ADDED,
	ITEM_REMOVED,
	LAST_SIGNAL
};

enum {
	PLAYING_COL,
	FILENAME_COL,
	FILENAME_ESCAPED_COL,
	URI_COL,
	TITLE_CUSTOM_COL,
	SUBTITLE_URI_COL,
	FILE_MONITOR_COL,
	NUM_COLS
};

typedef struct {
	const char *name;
	const char *suffix;
	TotemPlParserType type;
} PlaylistSaveType;

static const PlaylistSaveType save_types [] = {
	{ NULL, NULL, -1 }, /* By extension entry */
	{ N_("MP3 ShoutCast playlist"), "pls", TOTEM_PL_PARSER_PLS },
	{ N_("MP3 audio (streamed)"), "m3u", TOTEM_PL_PARSER_M3U },
	{ N_("MP3 audio (streamed, DOS format)"), "m3u", TOTEM_PL_PARSER_M3U_DOS },
	{ N_("XML Shareable Playlist"), "xspf", TOTEM_PL_PARSER_XSPF }
};

static int totem_playlist_table_signals[LAST_SIGNAL];

/* casts are to shut gcc up */
static const GtkTargetEntry target_table[] = {
	{ (gchar*) "text/uri-list", 0, 0 },
	{ (gchar*) "_NETSCAPE_URL", 0, 1 }
};

static void init_treeview (GtkWidget *treeview, TotemPlaylist *playlist);

#define totem_playlist_unset_playing(x) totem_playlist_set_playing(x, TOTEM_PLAYLIST_STATUS_NONE)

G_DEFINE_TYPE (TotemPlaylist, totem_playlist, GTK_TYPE_VBOX)

/* Helper functions */
static gboolean
totem_playlist_gtk_tree_model_iter_previous (GtkTreeModel *tree_model,
		GtkTreeIter *iter)
{
	GtkTreePath *path;
	gboolean ret;

	path = gtk_tree_model_get_path (tree_model, iter);
	ret = gtk_tree_path_prev (path);
	if (ret != FALSE)
		gtk_tree_model_get_iter (tree_model, iter, path);

	gtk_tree_path_free (path);
	return ret;
}

static gboolean
totem_playlist_gtk_tree_path_equals (GtkTreePath *path1, GtkTreePath *path2)
{
	char *str1, *str2;
	gboolean retval;

	if (path1 == NULL && path2 == NULL)
		return TRUE;
	if (path1 == NULL || path2 == NULL)
		return FALSE;

	str1 = gtk_tree_path_to_string (path1);
	str2 = gtk_tree_path_to_string (path2);

	if (strcmp (str1, str2) == 0)
		retval = TRUE;
	else
		retval = FALSE;

	g_free (str1);
	g_free (str2);

	return retval;
}

static GtkWindow *
totem_playlist_get_toplevel (TotemPlaylist *playlist)
{
	return GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (playlist)));
}

static void
set_waiting_cursor (TotemPlaylist *playlist)
{
	totem_gdk_window_set_waiting_cursor (gtk_widget_get_window (GTK_WIDGET (totem_playlist_get_toplevel (playlist))));
	playlist->priv->cursor_ref++;
}

static void
unset_waiting_cursor (TotemPlaylist *playlist)
{
	if (--playlist->priv->cursor_ref < 1)
		gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (totem_playlist_get_toplevel (playlist))), NULL);
}

static void
totem_playlist_error (char *title, char *reason, TotemPlaylist *playlist)
{
	GtkWidget *error_dialog;

	error_dialog =
		gtk_message_dialog_new (totem_playlist_get_toplevel (playlist),
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_OK,
				"%s", title);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (error_dialog),
						  "%s", reason);

	gtk_container_set_border_width (GTK_CONTAINER (error_dialog), 5);
	gtk_dialog_set_default_response (GTK_DIALOG (error_dialog),
			GTK_RESPONSE_OK);
	g_signal_connect (G_OBJECT (error_dialog), "destroy", G_CALLBACK
			(gtk_widget_destroy), error_dialog);
	g_signal_connect (G_OBJECT (error_dialog), "response", G_CALLBACK
			(gtk_widget_destroy), error_dialog);
	gtk_window_set_modal (GTK_WINDOW (error_dialog), TRUE);

	gtk_widget_show (error_dialog);
}

void
totem_playlist_select_subtitle_dialog(TotemPlaylist *playlist, TotemPlaylistSelectDialog mode)
{
	char *subtitle, *current, *uri;
	GFile *file, *dir;
	TotemPlaylistStatus playing;
	GtkTreeIter iter;

	if (mode == TOTEM_PLAYLIST_DIALOG_PLAYING) {
		/* Set subtitle file for the currently playing movie */
		gtk_tree_model_get_iter (playlist->priv->model, &iter, playlist->priv->current);
	} else if (mode == TOTEM_PLAYLIST_DIALOG_SELECTED) {
		/* Set subtitle file in for the first selected playlist item */
		GList *l;

		l = gtk_tree_selection_get_selected_rows (playlist->priv->selection, NULL);
		gtk_tree_model_get_iter (playlist->priv->model, &iter, l->data);
		g_list_foreach (l, (GFunc) gtk_tree_path_free, NULL);
		g_list_free (l);
	} else {
		g_assert_not_reached ();
	}

	/* Look for the directory of the current movie */
	gtk_tree_model_get (playlist->priv->model, &iter,
			    URI_COL, &current,
			    -1);

	if (current == NULL)
		return;

	uri = NULL;
	file = g_file_new_for_uri (current);
	dir = g_file_get_parent (file);
	g_object_unref (file);
	if (dir != NULL) {
		uri = g_file_get_uri (dir);
		g_object_unref (dir);
	}

	subtitle = totem_add_subtitle (totem_playlist_get_toplevel (playlist), uri);
	g_free (uri);

	if (subtitle == NULL)
		return;

	gtk_tree_model_get (playlist->priv->model, &iter,
			    PLAYING_COL, &playing,
			    -1);

	gtk_list_store_set (GTK_LIST_STORE(playlist->priv->model), &iter,
			    SUBTITLE_URI_COL, subtitle,
			    -1);

	if (playing != TOTEM_PLAYLIST_STATUS_NONE) {
		g_signal_emit (G_OBJECT (playlist),
			       totem_playlist_table_signals[SUBTITLE_CHANGED], 0,
			       NULL);
	}

	g_free(subtitle);
}

void
totem_playlist_set_current_subtitle (TotemPlaylist *playlist, const char *subtitle_uri)
{
	GtkTreeIter iter;

	if (playlist->priv->current == NULL)
		return;

	gtk_tree_model_get_iter (playlist->priv->model, &iter, playlist->priv->current);

	gtk_list_store_set (GTK_LIST_STORE(playlist->priv->model), &iter,
			    SUBTITLE_URI_COL, subtitle_uri,
			    -1);

	g_signal_emit (G_OBJECT (playlist),
		       totem_playlist_table_signals[SUBTITLE_CHANGED], 0,
		       NULL);
}

/* This one returns a new string, in UTF8 even if the MRL is encoded
 * in the locale's encoding
 */
static char *
totem_playlist_mrl_to_title (const gchar *mrl)
{
	GFile *file;
	char *filename_for_display, *unescaped;

	if (g_str_has_prefix (mrl, "dvd://") != FALSE) {
		/* This is "Title 3", where title is a DVD title
		 * Note: NOT a DVD chapter */
		return g_strdup_printf (_("Title %d"), (int) g_strtod (mrl + 6, NULL));
	} else if (g_str_has_prefix (mrl, "dvb://") != FALSE) {
		/* This is "BBC ONE(BBC)" for "dvb://BBC ONE(BBC)" */
		return g_strdup (mrl + 6);
	}

	file = g_file_new_for_uri (mrl);
	unescaped = g_file_get_basename (file);
	g_object_unref (file);

	filename_for_display = g_filename_to_utf8 (unescaped,
			-1,             /* length */
			NULL,           /* bytes_read */
			NULL,           /* bytes_written */
			NULL);          /* error */

	if (filename_for_display == NULL)
	{
		filename_for_display = g_locale_to_utf8 (unescaped,
				-1, NULL, NULL, NULL);
		if (filename_for_display == NULL) {
			filename_for_display = g_filename_display_name
				(unescaped);
		}
		g_free (unescaped);
		return filename_for_display;
	}

	g_free (unescaped);

	return filename_for_display;
}

static void
totem_playlist_update_save_button (TotemPlaylist *playlist)
{
	gboolean state;

	state = (!playlist->priv->disable_save_to_disk) && (PL_LEN != 0);
	gtk_widget_set_sensitive (playlist->priv->save_button, state);
}

static gboolean
totem_playlist_save_iter_foreach (GtkTreeModel *model,
				  GtkTreePath  *path,
				  GtkTreeIter  *iter,
				  gpointer      user_data)
{
	TotemPlPlaylist *playlist = user_data;
	TotemPlPlaylistIter pl_iter;
	gchar *uri, *title;
	gboolean custom_title;

	gtk_tree_model_get (model, iter,
			    URI_COL, &uri,
			    FILENAME_COL, &title,
			    TITLE_CUSTOM_COL, &custom_title,
			    -1);

	totem_pl_playlist_append (playlist, &pl_iter);
	totem_pl_playlist_set (playlist, &pl_iter,
			       TOTEM_PL_PARSER_FIELD_URI, uri,
			       TOTEM_PL_PARSER_FIELD_TITLE, (custom_title) ? title : NULL,
			       NULL);

	g_free (uri);
	g_free (title);

	return FALSE;
}

void
totem_playlist_save_current_playlist (TotemPlaylist *playlist, const char *output)
{
	totem_playlist_save_current_playlist_ext (playlist, output, TOTEM_PL_PARSER_PLS);
}

void
totem_playlist_save_current_playlist_ext (TotemPlaylist *playlist, const char *output, TotemPlParserType type)
{
	TotemPlPlaylist *pl_playlist;
	GError *error = NULL;
	GFile *output_file;
	gboolean retval;

	pl_playlist = totem_pl_playlist_new ();
	output_file = g_file_new_for_commandline_arg (output);

	gtk_tree_model_foreach (playlist->priv->model,
				totem_playlist_save_iter_foreach,
				pl_playlist);

	retval = totem_pl_parser_save (playlist->priv->parser,
				       pl_playlist,
				       output_file,
				       NULL, type, &error);

	if (retval == FALSE)
	{
		totem_playlist_error (_("Could not save the playlist"),
				error->message, playlist);
		g_error_free (error);
	}

	g_object_unref (pl_playlist);
	g_object_unref (output_file);
}

static void
gtk_tree_selection_has_selected_foreach (GtkTreeModel *model,
		GtkTreePath *path, GtkTreeIter *iter, gpointer user_data)
{
	int *retval = (gboolean *)user_data;
	*retval = TRUE;
}

static gboolean
gtk_tree_selection_has_selected (GtkTreeSelection *selection)
{
	int retval, *boolean;

	retval = FALSE;
	boolean = &retval;
	gtk_tree_selection_selected_foreach (selection,
			gtk_tree_selection_has_selected_foreach,
			(gpointer) (boolean));

	return retval;
}

static void
drop_finished_cb (TotemPlaylist *playlist, GAsyncResult *result, gpointer user_data)
{
	/* Emit the "changed" signal once the last dropped MRL has been added to the playlist */
	g_signal_emit (G_OBJECT (playlist),
	               totem_playlist_table_signals[CHANGED], 0,
	               NULL);
}

static void
drop_cb (GtkWidget        *widget,
         GdkDragContext   *context, 
	 gint              x,
	 gint              y,
	 GtkSelectionData *data, 
	 guint             info, 
	 guint             _time, 
	 TotemPlaylist    *playlist)
{
	char **list;
	GList *p, *file_list;
	guint i;
	GdkDragAction action;

	if (gdk_drag_context_get_suggested_action (context) == GDK_ACTION_ASK) {
		action = totem_drag_ask (PL_LEN != 0);
		gdk_drag_status (context, action, GDK_CURRENT_TIME);
		if (action == GDK_ACTION_DEFAULT) {
			gtk_drag_finish (context, FALSE, FALSE, _time);
			return;
		}
	}

	action = gdk_drag_context_get_selected_action (context);
	if (action == GDK_ACTION_MOVE)
		totem_playlist_clear (playlist);

	list = g_uri_list_extract_uris ((char *) gtk_selection_data_get_data (data));
	file_list = NULL;

	for (i = 0; list[i] != NULL; i++) {
		/* We get the list in the wrong order here,
		 * so when we insert the files at the same position
		 * in the tree, they are in the right order.*/
		file_list = g_list_prepend (file_list, list[i]);
	}

	if (file_list == NULL) {
		gtk_drag_finish (context, FALSE, FALSE, _time);
		return;
	}

	playlist->priv->tree_path = gtk_tree_path_new ();
	gtk_tree_view_get_dest_row_at_pos (GTK_TREE_VIEW (playlist->priv->treeview),
					   x, y,
					   &playlist->priv->tree_path,
					   &playlist->priv->drop_pos);

	/* But we reverse the list if we don't have any items in the
	 * list, as we insert new items at the end */
	if (playlist->priv->tree_path == NULL)
		file_list = g_list_reverse (file_list);

	for (p = file_list; p != NULL; p = p->next) {
		char *filename, *title;

		if (p->data == NULL)
			continue;

		filename = totem_create_full_path (p->data);
		if (filename == NULL)
			filename = g_strdup (p->data);
		title = NULL;

		if (info == 1) {
			p = p->next;
			if (p != NULL) {
				if (g_str_has_prefix (p->data, "file:") != FALSE)
					title = (char *)p->data + 5;
				else
					title = p->data;
			}
		}

		/* Add the MRL to the playlist asynchronously. If it's the last MRL, emit the "changed"
		 * signal once we're done adding it */
		if (p->next == NULL)
			totem_playlist_add_mrl (playlist, filename, title, TRUE, NULL, (GAsyncReadyCallback) drop_finished_cb, NULL);
		else
			totem_playlist_add_mrl (playlist, filename, title, TRUE, NULL, NULL, NULL);

		g_free (filename);
	}

	g_strfreev (list);
	g_list_free (file_list);
	gtk_drag_finish (context, TRUE, FALSE, _time);
	gtk_tree_path_free (playlist->priv->tree_path);
	playlist->priv->tree_path = NULL;
}

void
playlist_select_subtitle_action_callback (GtkAction *action, TotemPlaylist *playlist)
{
	totem_playlist_select_subtitle_dialog (playlist, TOTEM_PLAYLIST_DIALOG_SELECTED);
}

void
playlist_copy_location_action_callback (GtkAction *action, TotemPlaylist *playlist)
{
	GList *l;
	GtkClipboard *clip;
	char *url;
	GtkTreeIter iter;

	l = gtk_tree_selection_get_selected_rows (playlist->priv->selection,
			NULL);
	gtk_tree_model_get_iter (playlist->priv->model, &iter, l->data);
	g_list_foreach (l, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (l);

	gtk_tree_model_get (playlist->priv->model,
			&iter,
			URI_COL, &url,
			-1);

	/* Set both the middle-click and the super-paste buffers */
	clip = gtk_clipboard_get_for_display
		(gdk_display_get_default(), GDK_SELECTION_CLIPBOARD);
	gtk_clipboard_set_text (clip, url, -1);
	clip = gtk_clipboard_get_for_display
		(gdk_display_get_default(), GDK_SELECTION_PRIMARY);
	gtk_clipboard_set_text (clip, url, -1);
	g_free (url);

}

static gboolean
playlist_show_popup_menu (TotemPlaylist *playlist, GdkEventButton *event)
{
	guint button = 0;
	guint32 _time;
	GtkTreePath *path;
	gint count;
	GtkWidget *menu;
	GtkAction *copy_location;
	GtkAction *select_subtitle;

	if (event != NULL) {
		button = event->button;
		_time = event->time;

		if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (playlist->priv->treeview),
				 event->x, event->y, &path, NULL, NULL, NULL)) {
			if (!gtk_tree_selection_path_is_selected (playlist->priv->selection, path)) {
				gtk_tree_selection_unselect_all (playlist->priv->selection);
				gtk_tree_selection_select_path (playlist->priv->selection, path);
			}
			gtk_tree_path_free (path);
		} else {
			gtk_tree_selection_unselect_all (playlist->priv->selection);
		}
	} else {
		_time = gtk_get_current_event_time ();
	}

	count = gtk_tree_selection_count_selected_rows (playlist->priv->selection);
	
	if (count == 0) {
		return FALSE;
	}

	copy_location = gtk_action_group_get_action (playlist->priv->action_group, "copy-location");
	select_subtitle = gtk_action_group_get_action (playlist->priv->action_group, "select-subtitle");
	gtk_action_set_sensitive (copy_location, count == 1);
	gtk_action_set_sensitive (select_subtitle, count == 1);

	menu = gtk_ui_manager_get_widget (playlist->priv->ui_manager, "/totem-playlist-popup");

	gtk_menu_shell_select_first (GTK_MENU_SHELL (menu), FALSE);

	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
			button, _time);

	return TRUE;
}

static gboolean
treeview_button_pressed (GtkTreeView *treeview, GdkEventButton *event,
		TotemPlaylist *playlist)
{
	if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
		return playlist_show_popup_menu (playlist, event);
	}

	return FALSE;
}

static gboolean
playlist_treeview_popup_menu (GtkTreeView *treeview, TotemPlaylist *playlist)
{
	return playlist_show_popup_menu (playlist, NULL);
}

static void
totem_playlist_set_reorderable (TotemPlaylist *playlist, gboolean set)
{
	guint num_items, i;

	gtk_tree_view_set_reorderable
		(GTK_TREE_VIEW (playlist->priv->treeview), set);

	if (set != FALSE)
		return;

	num_items = PL_LEN;
	for (i = 0; i < num_items; i++)
	{
		GtkTreeIter iter;
		char *playlist_index;
		GtkTreePath *path;
		TotemPlaylistStatus playing;

		playlist_index = g_strdup_printf ("%d", i);
		if (gtk_tree_model_get_iter_from_string
				(playlist->priv->model,
				 &iter, playlist_index) == FALSE)
		{
			g_free (playlist_index);
			continue;
		}
		g_free (playlist_index);

		gtk_tree_model_get (playlist->priv->model, &iter, PLAYING_COL, &playing, -1);
		if (playing == TOTEM_PLAYLIST_STATUS_NONE)
			continue;

		/* Only emit the changed signal if we changed the ->current */
		path = gtk_tree_path_new_from_indices (i, -1);
		if (gtk_tree_path_compare (path, playlist->priv->current) == 0) {
			gtk_tree_path_free (path);
		} else {
			gtk_tree_path_free (playlist->priv->current);
			playlist->priv->current = path;
			g_signal_emit (G_OBJECT (playlist),
					totem_playlist_table_signals[CHANGED],
					0, NULL);
		}

		break;
	}
}

static gboolean 
button_press_cb (GtkWidget *treeview, GdkEventButton *event, gpointer data)
{ 
	TotemPlaylist *playlist = (TotemPlaylist *)data;

	if (playlist->priv->drop_disabled)
		return FALSE;

	playlist->priv->drop_disabled = TRUE;
	gtk_drag_dest_unset (treeview);
	g_signal_handlers_block_by_func (treeview, (GFunc) drop_cb, data);

	totem_playlist_set_reorderable (playlist, TRUE);

	return FALSE;
}

static gboolean 
button_release_cb (GtkWidget *treeview, GdkEventButton *event, gpointer data)
{
	TotemPlaylist *playlist = (TotemPlaylist *)data;

	if (!playlist->priv->drag_started && playlist->priv->drop_disabled)
	{
		playlist->priv->drop_disabled = FALSE;
		totem_playlist_set_reorderable (playlist, FALSE);
		gtk_tree_view_enable_model_drag_dest (GTK_TREE_VIEW (treeview),
						      target_table, G_N_ELEMENTS (target_table),
						      GDK_ACTION_COPY | GDK_ACTION_MOVE);

		g_signal_handlers_unblock_by_func (treeview,
				(GFunc) drop_cb, data);
	}

	return FALSE;
}

static void 
drag_begin_cb (GtkWidget *treeview, GdkDragContext *context, gpointer data)
{
	TotemPlaylist *playlist = (TotemPlaylist *)data;

	playlist->priv->drag_started = TRUE;

	return;
}

static void 
drag_end_cb (GtkWidget *treeview, GdkDragContext *context, gpointer data)
{
	TotemPlaylist *playlist = (TotemPlaylist *)data;

	playlist->priv->drop_disabled = FALSE;
	playlist->priv->drag_started = FALSE;
	totem_playlist_set_reorderable (playlist, FALSE);

	gtk_tree_view_enable_model_drag_dest (GTK_TREE_VIEW (treeview),
					      target_table, G_N_ELEMENTS (target_table),
					      GDK_ACTION_COPY | GDK_ACTION_MOVE);

	g_signal_handlers_unblock_by_func (treeview, (GFunc) drop_cb, data);

	return;
}

static void
selection_changed (GtkTreeSelection *treeselection, TotemPlaylist *playlist)
{
	gboolean sensitivity;

	if (gtk_tree_selection_has_selected (treeselection))
		sensitivity = TRUE;
	else
		sensitivity = FALSE;

	gtk_widget_set_sensitive (playlist->priv->remove_button, sensitivity);
	gtk_widget_set_sensitive (playlist->priv->up_button, sensitivity);
	gtk_widget_set_sensitive (playlist->priv->down_button, sensitivity);
}

/* This function checks if the current item is NULL, and try to update it
 * as the first item of the playlist if so. It returns TRUE if there is a
 * current item */
static gboolean
update_current_from_playlist (TotemPlaylist *playlist)
{
	int indice;

	if (playlist->priv->current != NULL)
		return TRUE;

	if (PL_LEN != 0)
	{
		if (playlist->priv->shuffle == FALSE)
		{
			indice = 0;
		} else {
			indice = playlist->priv->shuffled[0];
			playlist->priv->current_shuffled = 0;
		}

		playlist->priv->current = gtk_tree_path_new_from_indices
			(indice, -1);
	} else {
		return FALSE;
	}

	return TRUE;
}

void
totem_playlist_add_files (GtkWidget *widget, TotemPlaylist *playlist)
{
	GSList *filenames, *l;

	filenames = totem_add_files (totem_playlist_get_toplevel (playlist), NULL);
	if (filenames == NULL)
		return;

	for (l = filenames; l != NULL; l = l->next) {
		char *mrl;

		mrl = l->data;
		totem_playlist_add_mrl (playlist, mrl, NULL, TRUE, NULL, NULL, NULL);
		g_free (mrl);
	}

	g_slist_free (filenames);
}

static void
totem_playlist_foreach_selected (GtkTreeModel *model, GtkTreePath *path,
		GtkTreeIter *iter, gpointer data)
{
	TotemPlaylist *playlist = (TotemPlaylist *)data;
	GtkTreeRowReference *ref;

	/* We can't use gtk_list_store_remove() here
	 * So we build a list a RowReferences */
	ref = gtk_tree_row_reference_new (playlist->priv->model, path);
	playlist->priv->list = g_list_prepend
		(playlist->priv->list, (gpointer) ref);
	if (playlist->priv->current_to_be_removed == FALSE
	    && playlist->priv->current != NULL
	    && gtk_tree_path_compare (path, playlist->priv->current) == 0)
		playlist->priv->current_to_be_removed = TRUE;
}

static void
totem_playlist_emit_item_removed (TotemPlaylist *playlist,
				  GtkTreeIter   *iter)
{
	gchar *filename = NULL;
	gchar *uri = NULL;

	gtk_tree_model_get (playlist->priv->model, iter,
			    URI_COL, &uri, FILENAME_COL, &filename, -1);

	g_signal_emit (playlist,
		       totem_playlist_table_signals[ITEM_REMOVED],
		       0, filename, uri);

	g_free (filename);
	g_free (uri);
}

static void
playlist_remove_files (TotemPlaylist *playlist)
{
	totem_playlist_clear_with_compare (playlist, NULL, NULL);
}

void
playlist_remove_button_clicked (GtkWidget *button, TotemPlaylist *playlist)
{
	playlist_remove_files (playlist);
}

void
playlist_remove_action_callback (GtkAction *action, TotemPlaylist *playlist)
{
	playlist_remove_files (playlist);
}

static void
totem_playlist_save_playlist (TotemPlaylist *playlist, char *filename, gint active_format)
{
	if (active_format == 0)
		active_format = 1;

	totem_playlist_save_current_playlist_ext (playlist, filename,
						  save_types[active_format].type);
}

static char *
suffix_match_replace (const char *fname, guint old_format, guint new_format)
{
	char *ext;

	ext = g_strdup_printf (".%s", save_types[old_format].suffix);
	if (g_str_has_suffix (fname, ext) != FALSE) {
		char *no_suffix, *new_fname;

		no_suffix = g_strndup (fname, strlen (fname) - strlen (ext));
		new_fname = g_strconcat (no_suffix, ".", save_types[new_format].suffix, NULL);
		g_free (no_suffix);
		g_free (ext);

		return new_fname;
	}
	g_free (ext);

	return NULL;
}

static void
format_selection_changed (EggFileFormatChooser *chooser, TotemPlaylist *playlist)
{
	guint format;

	format = egg_file_format_chooser_get_format (chooser, NULL);

	if (format != playlist->priv->save_format) {
		char *fname, *new_fname;

		new_fname = NULL;
		fname = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (playlist->priv->file_chooser));

		if (format == 0) {
			/* The new format is "By extension" don't touch anything */
		} else if (playlist->priv->save_format == 0) {
			guint i;

			for (i = 1; i < G_N_ELEMENTS (save_types); i++) {
				new_fname = suffix_match_replace (fname, i, format);
				if (new_fname != NULL)
					break;
			}
		} else {
			new_fname = suffix_match_replace (fname, playlist->priv->save_format, format);
		}
		if (new_fname != NULL) {
			char *basename;

			basename = g_path_get_basename (new_fname);
			g_free (new_fname);
			gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (playlist->priv->file_chooser), basename);
			g_free (basename);
		}
		playlist->priv->save_format = format;
	}
}

static GtkWidget *
totem_playlist_save_add_format_chooser (GtkFileChooser *fc, TotemPlaylist *playlist)
{
	GtkWidget *format_chooser;
	guint i;

	format_chooser = egg_file_format_chooser_new ();

	playlist->priv->save_format = 0;

	for (i = 1; i < G_N_ELEMENTS (save_types) ; i++) {
		egg_file_format_chooser_add_format (
		    EGG_FILE_FORMAT_CHOOSER (format_chooser), 0, _(save_types[i].name),
		    "mate-mime-audio", save_types[i].suffix, NULL);
	}

	g_signal_connect (format_chooser, "selection-changed",
			  G_CALLBACK (format_selection_changed), playlist);

	gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (fc),
					   format_chooser);

	return format_chooser;
}

void
totem_playlist_save_files (GtkWidget *widget, TotemPlaylist *playlist)
{
	GtkWidget *fs, *format_chooser;
	char *filename;
	int response;

	g_assert (playlist->priv->file_chooser == NULL);

	fs = gtk_file_chooser_dialog_new (_("Save Playlist"),
					  totem_playlist_get_toplevel (playlist),
					  GTK_FILE_CHOOSER_ACTION_SAVE,
					  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					  GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					  NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (fs), GTK_RESPONSE_ACCEPT);
	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (fs), FALSE);
	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (fs), TRUE);

	/* translators: Playlist is the default saved playlist filename,
	 * without the suffix */
	filename = g_strconcat (_("Playlist"), ".", save_types[1].suffix, NULL);
	gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (fs), filename);
	g_free (filename);
	format_chooser = totem_playlist_save_add_format_chooser (GTK_FILE_CHOOSER (fs), playlist);

	if (playlist->priv->save_path != NULL) {
		gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (fs),
				playlist->priv->save_path);
	}

	playlist->priv->file_chooser = fs;

	response = gtk_dialog_run (GTK_DIALOG (fs));
	gtk_widget_hide (fs);
	while (gtk_events_pending())
		gtk_main_iteration();

	if (response == GTK_RESPONSE_ACCEPT) {
		char *fname;
		guint active_format;

		fname = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (fs));
		active_format = egg_file_format_chooser_get_format (EGG_FILE_FORMAT_CHOOSER (format_chooser),
								    fname);

		playlist->priv->file_chooser = NULL;
		gtk_widget_destroy (fs);

		if (fname == NULL)
			return;

		g_free (playlist->priv->save_path);
		playlist->priv->save_path = g_path_get_dirname (fname);

		totem_playlist_save_playlist (playlist, fname, active_format);
		g_free (fname);
	} else {
		playlist->priv->file_chooser = NULL;
		gtk_widget_destroy (fs);
	}
}

static void
totem_playlist_move_files (TotemPlaylist *playlist, gboolean direction_up)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkListStore *store;
	GtkTreeIter iter;
	GtkTreeRowReference *current;
	GList *paths, *refs, *l;
	int pos;

	selection = gtk_tree_view_get_selection
		(GTK_TREE_VIEW (playlist->priv->treeview));
	if (selection == NULL)
		return;

	model = gtk_tree_view_get_model
		(GTK_TREE_VIEW (playlist->priv->treeview));
	store = GTK_LIST_STORE (model);
	pos = -2;
	refs = NULL;

	if (playlist->priv->current != NULL) {
		current = gtk_tree_row_reference_new (model,
				playlist->priv->current);
	} else {
		current = NULL;
	}

	/* Build a list of tree references */
	paths = gtk_tree_selection_get_selected_rows (selection, NULL);
	for (l = paths; l != NULL; l = l->next) {
		GtkTreePath *path = l->data;
		int cur_pos, *indices;

		refs = g_list_prepend (refs,
				gtk_tree_row_reference_new (model, path));
		indices = gtk_tree_path_get_indices (path);
		cur_pos = indices[0];
		if (pos == -2)
		{
			pos = cur_pos;
		} else {
			if (direction_up == FALSE)
				pos = MAX (cur_pos, pos);
			else
				pos = MIN (cur_pos, pos);
		}
	}
	g_list_foreach (paths, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (paths);

	/* Otherwise we reverse the items when moving down */
	if (direction_up != FALSE)
		refs = g_list_reverse (refs);

	if (direction_up == FALSE)
		pos = pos + 2;
	else
		pos = pos - 2;

	for (l = refs; l != NULL; l = l->next) {
		GtkTreeIter *position, cur;
		GtkTreeRowReference *ref = l->data;
		GtkTreePath *path;

		if (pos < 0) {
			position = NULL;
		} else {
			char *str;

			str = g_strdup_printf ("%d", pos);
			if (gtk_tree_model_get_iter_from_string (model,
					&iter, str))
				position = &iter;
			else
				position = NULL;

			g_free (str);
		}

		path = gtk_tree_row_reference_get_path (ref);
		gtk_tree_model_get_iter (model, &cur, path);
		gtk_tree_path_free (path);

		if (direction_up == FALSE)
		{
			pos--;
			gtk_list_store_move_before (store, &cur, position);
		} else {
			gtk_list_store_move_after (store, &cur, position);
			pos++;
		}
	}

	g_list_foreach (refs, (GFunc) gtk_tree_row_reference_free, NULL);
	g_list_free (refs);

	/* Update the current path */
	if (current != NULL) {
		gtk_tree_path_free (playlist->priv->current);
		playlist->priv->current = gtk_tree_row_reference_get_path
			(current);
		gtk_tree_row_reference_free (current);
	}

	g_signal_emit (G_OBJECT (playlist),
			totem_playlist_table_signals[CHANGED], 0,
			NULL);
}

void
totem_playlist_up_files (GtkWidget *widget, TotemPlaylist *playlist)
{
	totem_playlist_move_files (playlist, TRUE);
}

void
totem_playlist_down_files (GtkWidget *widget, TotemPlaylist *playlist)
{
	totem_playlist_move_files (playlist, FALSE);
}

static int
totem_playlist_key_press (GtkWidget *win, GdkEventKey *event, TotemPlaylist *playlist)
{
	/* Special case some shortcuts */
	if (event->state != 0) {
		if ((event->state & GDK_CONTROL_MASK)
		    && event->keyval == GDK_a) {
			gtk_tree_selection_select_all
				(playlist->priv->selection);
			return TRUE;
		}
	}

	/* If we have modifiers, and either Ctrl, Mod1 (Alt), or any
	 * of Mod3 to Mod5 (Mod2 is num-lock...) are pressed, we
	 * let Gtk+ handle the key */
	if (event->state != 0
			&& ((event->state & GDK_CONTROL_MASK)
			|| (event->state & GDK_MOD1_MASK)
			|| (event->state & GDK_MOD3_MASK)
			|| (event->state & GDK_MOD4_MASK)
			|| (event->state & GDK_MOD5_MASK)))
		return FALSE;

	if (event->keyval == GDK_Delete)
	{
		playlist_remove_files (playlist);
		return TRUE;
	}

	return FALSE;
}

static void
set_playing_icon (GtkTreeViewColumn *column, GtkCellRenderer *renderer,
		  GtkTreeModel *model, GtkTreeIter *iter, TotemPlaylist *playlist)
{
	TotemPlaylistStatus playing;
	const char *stock_id;

	gtk_tree_model_get (model, iter, PLAYING_COL, &playing, -1);

	switch (playing) {
		case TOTEM_PLAYLIST_STATUS_PLAYING:
			stock_id = GTK_STOCK_MEDIA_PLAY;
			break;
		case TOTEM_PLAYLIST_STATUS_PAUSED:
			stock_id = GTK_STOCK_MEDIA_PAUSE;
			break;
		default:
			stock_id = NULL;
	}

	g_object_set (renderer, "stock-id", stock_id, NULL);
}

static void
init_columns (GtkTreeView *treeview, TotemPlaylist *playlist)
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	/* Playing pix */
	renderer = gtk_cell_renderer_pixbuf_new ();
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func (column, renderer,
			(GtkTreeCellDataFunc) set_playing_icon, playlist, NULL);
	g_object_set (renderer, "stock-size", GTK_ICON_SIZE_MENU, NULL);
	gtk_tree_view_append_column (treeview, column);

	/* Labels */
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column, renderer,
			"text", FILENAME_COL, NULL);
}

static void
treeview_row_changed (GtkTreeView *treeview, GtkTreePath *arg1,
		GtkTreeViewColumn *arg2, TotemPlaylist *playlist)
{
	if (totem_playlist_gtk_tree_path_equals
	    (arg1, playlist->priv->current) != FALSE) {
		g_signal_emit (G_OBJECT (playlist),
				totem_playlist_table_signals[ITEM_ACTIVATED], 0,
				NULL);
		return;
	}

	if (playlist->priv->current != NULL) {
		totem_playlist_unset_playing (playlist);
		gtk_tree_path_free (playlist->priv->current);
	}

	playlist->priv->current = gtk_tree_path_copy (arg1);

	if (playlist->priv->shuffle != FALSE) {
		int *indices, indice, i;

		indices = gtk_tree_path_get_indices (playlist->priv->current);
		indice = indices[0];

		for (i = 0; i < PL_LEN; i++)
		{
			if (playlist->priv->shuffled[i] == indice)
			{
				playlist->priv->current_shuffled = i;
				break;
			}
		}
	}
	g_signal_emit (G_OBJECT (playlist),
			totem_playlist_table_signals[CHANGED], 0,
			NULL);

	if (playlist->priv->drop_disabled) {
		playlist->priv->drop_disabled = FALSE;
		totem_playlist_set_reorderable (playlist, FALSE);

		gtk_tree_view_enable_model_drag_dest (GTK_TREE_VIEW (treeview),
						      target_table, G_N_ELEMENTS (target_table),
						      GDK_ACTION_COPY | GDK_ACTION_MOVE);

		g_signal_handlers_unblock_by_func (treeview,
				(GFunc) drop_cb, playlist);
	}
}

static gboolean
search_equal_is_match (const gchar * s, const gchar * lc_key)
{
	gboolean match = FALSE;

	if (s != NULL) {
		gchar *lc_s;

		/* maybe also normalize both strings? */
		lc_s = g_utf8_strdown (s, -1);
		match = (lc_s != NULL && strstr (lc_s, lc_key) != NULL);
		g_free (lc_s);
	}

	return match;
}

static gboolean
search_equal_func (GtkTreeModel *model, gint col, const gchar *key,
                   GtkTreeIter *iter, gpointer userdata)
{
	gboolean match;
	gchar *lc_key, *fn = NULL;

	lc_key = g_utf8_strdown (key, -1);

        /* type-ahead search: first check display filename / title, then URI */
	gtk_tree_model_get (model, iter, FILENAME_COL, &fn, -1);
	match = search_equal_is_match (fn, lc_key);
	g_free (fn);

	if (!match) {
		gchar *uri = NULL;

		gtk_tree_model_get (model, iter, URI_COL, &uri, -1);
		fn = g_filename_from_uri (uri, NULL, NULL);
		match = search_equal_is_match (fn, lc_key);
		g_free (fn);
		g_free (uri);
	}

	g_free (lc_key);
	return !match; /* needs to return FALSE if row matches */
}

static void
init_treeview (GtkWidget *treeview, TotemPlaylist *playlist)
{
	GtkTreeSelection *selection;

	init_columns (GTK_TREE_VIEW (treeview), playlist);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
	g_signal_connect (G_OBJECT (selection), "changed",
			G_CALLBACK (selection_changed), playlist);
	g_signal_connect (G_OBJECT (treeview), "row-activated",
			G_CALLBACK (treeview_row_changed), playlist);
	g_signal_connect (G_OBJECT (treeview), "button-press-event",
			G_CALLBACK (treeview_button_pressed), playlist);
	g_signal_connect (G_OBJECT (treeview), "popup-menu",
			G_CALLBACK (playlist_treeview_popup_menu), playlist);

	/* Drag'n'Drop */
	g_signal_connect (G_OBJECT (treeview), "drag_data_received",
			G_CALLBACK (drop_cb), playlist);
        g_signal_connect (G_OBJECT (treeview), "button_press_event",
			G_CALLBACK (button_press_cb), playlist);
        g_signal_connect (G_OBJECT (treeview), "button_release_event",
			G_CALLBACK (button_release_cb), playlist);
	g_signal_connect (G_OBJECT (treeview), "drag_begin",
                        G_CALLBACK (drag_begin_cb), playlist);
	g_signal_connect (G_OBJECT (treeview), "drag_end",
                        G_CALLBACK (drag_end_cb), playlist);

	gtk_tree_view_enable_model_drag_dest (GTK_TREE_VIEW (treeview),
					      target_table, G_N_ELEMENTS (target_table),
					      GDK_ACTION_COPY | GDK_ACTION_MOVE);

	playlist->priv->selection = selection;

	/* make type-ahead search work in the playlist */
	gtk_tree_view_set_search_equal_func (GTK_TREE_VIEW (treeview),
	                                     search_equal_func, NULL, NULL);

	gtk_widget_show (treeview);
}

static void
update_repeat_cb (MateConfClient *client, guint cnxn_id,
		MateConfEntry *entry, TotemPlaylist *playlist)
{
	gboolean repeat;

	repeat = mateconf_value_get_bool (entry->value);
	playlist->priv->repeat = (repeat != FALSE);

	g_signal_emit (G_OBJECT (playlist),
			totem_playlist_table_signals[CHANGED], 0,
			NULL);
	g_signal_emit (G_OBJECT (playlist),
			totem_playlist_table_signals[REPEAT_TOGGLED], 0,
			repeat, NULL);
}

typedef struct {
	int random;
	int index;
} RandomData;

static int
compare_random (gconstpointer ptr_a, gconstpointer ptr_b)
{
	RandomData *a = (RandomData *) ptr_a;
	RandomData *b = (RandomData *) ptr_b;

	if (a->random < b->random)
		return -1;
	else if (a->random > b->random)
		return 1;
	else
		return 0;
}

static void
ensure_shuffled (TotemPlaylist *playlist)
{
	RandomData data;
	GArray *array;
	int i, current, current_new;
	int *indices;

	if (playlist->priv->shuffled == NULL)
		playlist->priv->shuffled = g_new (int, PL_LEN);
	else if (PL_LEN != playlist->priv->shuffle_len)
		playlist->priv->shuffled = g_renew (int, playlist->priv->shuffled, PL_LEN);
	playlist->priv->shuffle_len = PL_LEN;

	if (PL_LEN == 0)
		return;

	if (playlist->priv->current != NULL) {
		indices = gtk_tree_path_get_indices (playlist->priv->current);
		current = indices[0];
	} else {
		current = -1;
	}
	
	current_new = -1;

	array = g_array_sized_new (FALSE, FALSE, sizeof (RandomData), PL_LEN);

	for (i = 0; i < PL_LEN; i++) {
		data.random = g_random_int_range (0, PL_LEN);
		data.index = i;

		g_array_append_val (array, data);
	}

	g_array_sort (array, compare_random);

	for (i = 0; i < PL_LEN; i++) {
		playlist->priv->shuffled[i] = g_array_index (array, RandomData, i).index;

		if (playlist->priv->current != NULL && playlist->priv->shuffled[i] == current)
			current_new = i;
	}

	if (current_new > -1) {
		playlist->priv->shuffled[current_new] = playlist->priv->shuffled[0];
		playlist->priv->shuffled[0] = current;
		playlist->priv->current_shuffled = 0;
	}

	g_array_free (array, TRUE);
}

static void
update_shuffle_cb (MateConfClient *client, guint cnxn_id,
		MateConfEntry *entry, TotemPlaylist *playlist)
{
	gboolean shuffle;

	shuffle = mateconf_value_get_bool (entry->value);
	playlist->priv->shuffle = shuffle;
	if (shuffle == FALSE) {
		g_free (playlist->priv->shuffled);
		playlist->priv->shuffled = NULL;
		playlist->priv->shuffle_len = 0;
	} else {
		ensure_shuffled (playlist);
	}

	g_signal_emit (G_OBJECT (playlist),
			totem_playlist_table_signals[CHANGED], 0,
			NULL);
	g_signal_emit (G_OBJECT (playlist),
			totem_playlist_table_signals[SHUFFLE_TOGGLED], 0,
			shuffle, NULL);
}

static void
update_lockdown (MateConfClient *client, guint cnxn_id,
		MateConfEntry *entry, TotemPlaylist *playlist)
{
	playlist->priv->disable_save_to_disk = mateconf_client_get_bool
			(playlist->priv->gc,
			"/desktop/mate/lockdown/disable_save_to_disk", NULL) != FALSE;
	totem_playlist_update_save_button (playlist);
}

static void
init_config (TotemPlaylist *playlist)
{
	playlist->priv->gc = mateconf_client_get_default ();

	playlist->priv->disable_save_to_disk = mateconf_client_get_bool
	       		(playlist->priv->gc,
			"/desktop/mate/lockdown/disable_save_to_disk", NULL) != FALSE;
	totem_playlist_update_save_button (playlist);

	mateconf_client_add_dir (playlist->priv->gc, MATECONF_PREFIX,
			MATECONF_CLIENT_PRELOAD_ONELEVEL, NULL);
	mateconf_client_notify_add (playlist->priv->gc, MATECONF_PREFIX"/repeat",
			(MateConfClientNotifyFunc) update_repeat_cb,
			playlist, NULL, NULL);
	mateconf_client_notify_add (playlist->priv->gc, MATECONF_PREFIX"/shuffle",
			(MateConfClientNotifyFunc) update_shuffle_cb,
			playlist, NULL, NULL);

	mateconf_client_add_dir (playlist->priv->gc, "/desktop/mate/lockdown",
			MATECONF_CLIENT_PRELOAD_ONELEVEL, NULL);
	mateconf_client_notify_add (playlist->priv->gc,
			"/desktop/mate/lockdown/disable_save_to_disk",
			(MateConfClientNotifyFunc) update_lockdown,
			playlist, NULL, NULL);

	playlist->priv->repeat = mateconf_client_get_bool (playlist->priv->gc,
			MATECONF_PREFIX"/repeat", NULL) != FALSE;
	playlist->priv->shuffle = mateconf_client_get_bool (playlist->priv->gc,
			MATECONF_PREFIX"/shuffle", NULL) != FALSE;
}

static void
totem_playlist_entry_parsed (TotemPlParser *parser,
			     const char *uri,
			     GHashTable *metadata,
			     TotemPlaylist *playlist)
{
	const char *title;
	gint64 duration;

	/* We ignore 0-length items in playlists, they're usually just banners */
	duration = totem_pl_parser_parse_duration
		(g_hash_table_lookup (metadata, TOTEM_PL_PARSER_FIELD_DURATION), FALSE);
	if (duration == 0)
		return;
	title = g_hash_table_lookup (metadata, TOTEM_PL_PARSER_FIELD_TITLE);
	totem_playlist_add_one_mrl (playlist, uri, title);
}

static gboolean
totem_playlist_compare_with_monitor (TotemPlaylist *playlist, GtkTreeIter *iter, gconstpointer data)
{
	GFileMonitor *monitor = (GFileMonitor *) data;
	GFileMonitor *_monitor;
	gboolean retval = FALSE;

	gtk_tree_model_get (playlist->priv->model, iter,
			    FILE_MONITOR_COL, &_monitor, -1);

	if (_monitor == monitor)
		retval = TRUE;

	if (_monitor != NULL)
		g_object_unref (_monitor);

	return retval;
}

static void
totem_playlist_file_changed (GFileMonitor *monitor,
			     GFile *file,
			     GFile *other_file,
			     GFileMonitorEvent event_type,
			     TotemPlaylist *playlist)
{
	if (event_type == G_FILE_MONITOR_EVENT_PRE_UNMOUNT ||
	    event_type == G_FILE_MONITOR_EVENT_UNMOUNTED) {
		totem_playlist_clear_with_compare (playlist,
						   (ClearComparisonFunc) totem_playlist_compare_with_monitor,
						   monitor);
	}
}

static void
totem_playlist_dispose (GObject *object)
{
	TotemPlaylist *playlist = TOTEM_PLAYLIST (object);

	if (playlist->priv->parser != NULL) {
		g_object_unref (playlist->priv->parser);
		playlist->priv->parser = NULL;
	}

	if (playlist->priv->ui_manager != NULL) {
		g_object_unref (G_OBJECT (playlist->priv->ui_manager));
		playlist->priv->ui_manager = NULL;
	}

	if (playlist->priv->action_group != NULL) {
		g_object_unref (G_OBJECT (playlist->priv->action_group));
		playlist->priv->action_group = NULL;
	}

	G_OBJECT_CLASS (totem_playlist_parent_class)->dispose (object);
}

static void
totem_playlist_finalize (GObject *object)
{
	TotemPlaylist *playlist = TOTEM_PLAYLIST (object);

	if (playlist->priv->current != NULL)
		gtk_tree_path_free (playlist->priv->current);

	G_OBJECT_CLASS (totem_playlist_parent_class)->finalize (object);
}

static void
totem_playlist_init (TotemPlaylist *playlist)
{
	GtkWidget *container;
	GtkBuilder *xml;

	playlist->priv = G_TYPE_INSTANCE_GET_PRIVATE (playlist, TOTEM_TYPE_PLAYLIST, TotemPlaylistPrivate);
	playlist->priv->parser = totem_pl_parser_new ();

	totem_pl_parser_add_ignored_scheme (playlist->priv->parser, "dvd:");
	totem_pl_parser_add_ignored_scheme (playlist->priv->parser, "cdda:");
	totem_pl_parser_add_ignored_scheme (playlist->priv->parser, "vcd:");
	totem_pl_parser_add_ignored_scheme (playlist->priv->parser, "cd:");
	totem_pl_parser_add_ignored_scheme (playlist->priv->parser, "dvb:");
	totem_pl_parser_add_ignored_mimetype (playlist->priv->parser, "application/x-trash");

	g_signal_connect (G_OBJECT (playlist->priv->parser),
			"entry-parsed",
			G_CALLBACK (totem_playlist_entry_parsed),
			playlist);

	xml = totem_interface_load ("playlist.ui", TRUE, NULL, playlist);

	if (xml == NULL)
		return;

	/* popup menu */
	playlist->priv->action_group = GTK_ACTION_GROUP (gtk_builder_get_object (xml, "playlist-action-group"));
	g_object_ref (playlist->priv->action_group);
	playlist->priv->ui_manager = GTK_UI_MANAGER (gtk_builder_get_object (xml, "totem-playlist-ui-manager"));
	g_object_ref (playlist->priv->ui_manager);

	gtk_widget_add_events (GTK_WIDGET (playlist), GDK_KEY_PRESS_MASK);
	g_signal_connect (G_OBJECT (playlist), "key_press_event",
			  G_CALLBACK (totem_playlist_key_press), playlist);

	/* Buttons */
	playlist->priv->save_button = GTK_WIDGET (gtk_builder_get_object (xml, "save_button"));;
	playlist->priv->remove_button = GTK_WIDGET (gtk_builder_get_object (xml, "remove_button"));
	playlist->priv->up_button = GTK_WIDGET (gtk_builder_get_object (xml, "up_button"));
	playlist->priv->down_button = GTK_WIDGET (gtk_builder_get_object (xml, "down_button"));

	/* Reparent the vbox */
	container = GTK_WIDGET (gtk_builder_get_object (xml, "vbox4"));
	g_object_ref (container);
	gtk_box_pack_start (GTK_BOX (playlist),
			container,
			TRUE,       /* expand */
			TRUE,       /* fill */
			0);         /* padding */
	g_object_unref (container);

	playlist->priv->treeview = GTK_WIDGET (gtk_builder_get_object (xml, "treeview1"));
	init_treeview (playlist->priv->treeview, playlist);
	playlist->priv->model = gtk_tree_view_get_model
		(GTK_TREE_VIEW (playlist->priv->treeview));

	/* tooltips */
	gtk_tree_view_set_tooltip_column(GTK_TREE_VIEW(playlist->priv->treeview),
					 FILENAME_ESCAPED_COL);

	/* The configuration */
	init_config (playlist);

	gtk_widget_show_all (GTK_WIDGET (playlist));

	g_object_unref (xml);
}

GtkWidget*
totem_playlist_new (void)
{
	TotemPlaylist *playlist;

	playlist = TOTEM_PLAYLIST (g_object_new (TOTEM_TYPE_PLAYLIST, NULL));
	if (playlist->priv->ui_manager == NULL) {
		g_object_unref (playlist);
		return NULL;
	}

	return GTK_WIDGET (playlist);
}

static gboolean
totem_playlist_add_one_mrl (TotemPlaylist *playlist,
			    const char *mrl,
			    const char *display_name)
{
	GtkListStore *store;
	GtkTreeIter iter;
	char *filename_for_display, *uri, *escaped_filename;
	GtkTreeRowReference *ref;
	GFileMonitor *monitor;
	GFile *file;
	int pos;

	g_return_val_if_fail (TOTEM_IS_PLAYLIST (playlist), FALSE);
	g_return_val_if_fail (mrl != NULL, FALSE);

	if (display_name == NULL || *display_name == '\0')
		filename_for_display = totem_playlist_mrl_to_title (mrl);
	else
		filename_for_display = g_strdup (display_name);

	ref = NULL;
	uri = totem_create_full_path (mrl);

	g_debug ("totem_playlist_add_one_mrl (): %s %s %s\n", filename_for_display, uri, display_name);

	if (playlist->priv->tree_path != NULL && playlist->priv->current != NULL) {
		int *indices;
		indices = gtk_tree_path_get_indices (playlist->priv->tree_path);
		pos = indices[0];
		ref = gtk_tree_row_reference_new (playlist->priv->model, playlist->priv->current);
	} else {
		pos = G_MAXINT;
	}

	store = GTK_LIST_STORE (playlist->priv->model);

	/* Get the file monitor */
	file = g_file_new_for_uri (uri ? uri : mrl);
	if (g_file_is_native (file) != FALSE) {
		monitor = g_file_monitor_file (file,
					       G_FILE_MONITOR_NONE,
					       NULL,
					       NULL);
		g_signal_connect (G_OBJECT (monitor),
				  "changed",
				  G_CALLBACK (totem_playlist_file_changed),
				  playlist);
	} else {
		monitor = NULL;
	}

	escaped_filename = g_markup_escape_text (filename_for_display, -1);
	gtk_list_store_insert_with_values (store, &iter, pos,
			PLAYING_COL, TOTEM_PLAYLIST_STATUS_NONE,
			FILENAME_COL, filename_for_display,
			FILENAME_ESCAPED_COL, escaped_filename,
			URI_COL, uri ? uri : mrl,
			TITLE_CUSTOM_COL, display_name ? TRUE : FALSE,
			FILE_MONITOR_COL, monitor,
			-1);
	g_free (escaped_filename);

	g_signal_emit (playlist,
		       totem_playlist_table_signals[ITEM_ADDED],
		       0, filename_for_display, uri ? uri : mrl);

	g_free (filename_for_display);
	g_free (uri);

	if (playlist->priv->current == NULL && playlist->priv->shuffle == FALSE)
		playlist->priv->current = gtk_tree_model_get_path (playlist->priv->model, &iter);
	if (playlist->priv->shuffle)
		ensure_shuffled (playlist);

	/* And update current to point to the right file again */
	if (ref != NULL) {
		gtk_tree_path_free (playlist->priv->current);
		playlist->priv->current = gtk_tree_row_reference_get_path (ref);
		gtk_tree_row_reference_free (ref);
	}

	g_signal_emit (G_OBJECT (playlist),
			totem_playlist_table_signals[CHANGED], 0,
			NULL);
	totem_playlist_update_save_button (playlist);

	return TRUE;
}

typedef struct {
	GAsyncReadyCallback callback;
	gpointer user_data;
	gboolean cursor;
	TotemPlaylist *playlist;
	gchar *mrl;
	gchar *display_name;
} AddMrlData;

static void
add_mrl_data_free (AddMrlData *data)
{
	g_object_unref (data->playlist);
	g_free (data->mrl);
	g_free (data->display_name);
	g_slice_free (AddMrlData, data);
}

static gboolean
handle_parse_result (TotemPlParserResult res, TotemPlaylist *playlist, const gchar *mrl, const gchar *display_name)
{
	if (res == TOTEM_PL_PARSER_RESULT_UNHANDLED)
		return totem_playlist_add_one_mrl (playlist, mrl, display_name);
	if (res == TOTEM_PL_PARSER_RESULT_ERROR) {
		char *msg;

		msg = g_strdup_printf (_("The playlist '%s' could not be parsed. It might be damaged."), display_name ? display_name : mrl);
		totem_playlist_error (_("Playlist error"), msg, playlist);
		g_free (msg);

		return FALSE;
	}
	if (res == TOTEM_PL_PARSER_RESULT_IGNORED)
		return FALSE;

	return TRUE;
}

static void
add_mrl_cb (TotemPlParser *parser, GAsyncResult *result, AddMrlData *data)
{
	TotemPlParserResult res;
	GSimpleAsyncResult *async_result;
	GError *error = NULL;

	/* Finish parsing the playlist */
	res = totem_pl_parser_parse_finish (parser, result, &error);

	/* Remove the cursor, if one was set */
	if (data->cursor)
		unset_waiting_cursor (data->playlist);

	/* Create an async result which will return the result to the code which called totem_playlist_add_mrl() */
	if (error != NULL)
		async_result = g_simple_async_result_new_from_error (G_OBJECT (data->playlist), data->callback, data->user_data, error);
	else
		async_result = g_simple_async_result_new (G_OBJECT (data->playlist), data->callback, data->user_data, totem_playlist_add_mrl);

	/* Handle the various return cases from the playlist parser */
	g_simple_async_result_set_op_res_gboolean (async_result, handle_parse_result (res, data->playlist, data->mrl, data->display_name));

	/* Free the closure's data, now that we're finished with it */
	add_mrl_data_free (data);

	/* Synchronously call the calling code's callback function (i.e. what was passed to totem_playlist_add_mrl()'s @callback parameter)
	 * in the main thread to return the result */
	g_simple_async_result_complete (async_result);
}

void
totem_playlist_add_mrl (TotemPlaylist *playlist, const char *mrl, const char *display_name, gboolean cursor,
                        GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
	AddMrlData *data;

	g_return_if_fail (mrl != NULL);

	/* Display a waiting cursor if required */
	if (cursor)
		set_waiting_cursor (playlist);

	/* Build the data struct to pass to the callback function */
	data = g_slice_new (AddMrlData);
	data->callback = callback;
	data->user_data = user_data;
	data->cursor = cursor;
	data->playlist = g_object_ref (playlist);
	data->mrl = g_strdup (mrl);
	data->display_name = g_strdup (display_name);

	/* Start parsing the playlist. Once this is complete, add_mrl_cb() is called, which will interpret the results and call @callback to
	 * finish the process. */
	totem_pl_parser_parse_async (playlist->priv->parser, mrl, FALSE, cancellable, (GAsyncReadyCallback) add_mrl_cb, data);
}

gboolean
totem_playlist_add_mrl_finish (TotemPlaylist *playlist, GAsyncResult *result)
{
	g_assert (g_simple_async_result_get_source_tag (G_SIMPLE_ASYNC_RESULT (result)) == totem_playlist_add_mrl);

	return g_simple_async_result_get_op_res_gboolean (G_SIMPLE_ASYNC_RESULT (result));
}

gboolean
totem_playlist_add_mrl_sync (TotemPlaylist *playlist, const char *mrl, const char *display_name)
{
	g_return_val_if_fail (mrl != NULL, FALSE);

	return handle_parse_result (totem_pl_parser_parse (playlist->priv->parser, mrl, FALSE), playlist, mrl, display_name);
}

static gboolean
totem_playlist_clear_cb (GtkTreeModel *model,
			 GtkTreePath  *path,
			 GtkTreeIter  *iter,
			 gpointer      data)
{
	totem_playlist_emit_item_removed (data, iter);
	return FALSE;
}

gboolean
totem_playlist_clear (TotemPlaylist *playlist)
{
	GtkListStore *store;

	g_return_val_if_fail (TOTEM_IS_PLAYLIST (playlist), FALSE);

	if (PL_LEN == 0)
		return FALSE;

	gtk_tree_model_foreach (playlist->priv->model,
				totem_playlist_clear_cb,
				playlist);

	store = GTK_LIST_STORE (playlist->priv->model);
	gtk_list_store_clear (store);

	if (playlist->priv->current != NULL)
		gtk_tree_path_free (playlist->priv->current);
	playlist->priv->current = NULL;

	totem_playlist_update_save_button (playlist);

	return TRUE;
}

static int
compare_removal (GtkTreeRowReference *ref, GtkTreePath *path)
{
	GtkTreePath *ref_path;
	int ret = -1;

	ref_path = gtk_tree_row_reference_get_path (ref);
	if (gtk_tree_path_compare (path, ref_path) == 0)
		ret = 0;
	gtk_tree_path_free (ref_path);
	return ret;
}

/* Whether the item in question will be removed */
static gboolean
totem_playlist_item_to_be_removed (TotemPlaylist *playlist,
				   GtkTreePath *path,
				   ClearComparisonFunc func)
{
	GList *ret;

	if (func == NULL) {
		GtkTreeSelection *selection;

		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (playlist->priv->treeview));
		return gtk_tree_selection_path_is_selected (selection, path);
	}

	ret = g_list_find_custom (playlist->priv->list, path, (GCompareFunc) compare_removal);
	return (ret != NULL);
}

static void
totem_playlist_clear_with_compare (TotemPlaylist *playlist,
				   ClearComparisonFunc func,
				   gconstpointer data)
{
	GtkTreeRowReference *ref;
	GtkTreeRowReference *next;

	ref = NULL;
	next = NULL;

	if (func == NULL) {
		GtkTreeSelection *selection;

		selection = gtk_tree_view_get_selection
			(GTK_TREE_VIEW (playlist->priv->treeview));
		if (selection == NULL)
			return;

		gtk_tree_selection_selected_foreach (selection,
						     totem_playlist_foreach_selected,
						     (gpointer) playlist);
	} else {
		guint num_items, i;

		num_items = PL_LEN;
		if (num_items == 0)
			return;

		for (i = 0; i < num_items; i++) {
			GtkTreeIter iter;
			char *playlist_index;

			playlist_index = g_strdup_printf ("%d", i);
			if (gtk_tree_model_get_iter_from_string (playlist->priv->model, &iter, playlist_index) == FALSE) {
				g_free (playlist_index);
				continue;
			}
			g_free (playlist_index);

			if ((* func) (playlist, &iter, data) != FALSE) {
				GtkTreePath *path;
				GtkTreeRowReference *r;

				path = gtk_tree_path_new_from_indices (i, -1);
				r = gtk_tree_row_reference_new (playlist->priv->model, path);
				if (playlist->priv->current_to_be_removed == FALSE && playlist->priv->current != NULL) {
					if (gtk_tree_path_compare (path, playlist->priv->current) == 0) {
						playlist->priv->current_to_be_removed = TRUE;
					}
				}
				playlist->priv->list = g_list_prepend (playlist->priv->list, r);
				gtk_tree_path_free (path);
			}
		}

		if (playlist->priv->list == NULL)
			return;
	}

	/* If the current item is to change, we need to keep an static
	 * reference to it, TreeIter and TreePath don't allow that */
	if (playlist->priv->current_to_be_removed == FALSE &&
	    playlist->priv->current != NULL) {
		ref = gtk_tree_row_reference_new (playlist->priv->model,
						  playlist->priv->current);
	} else if (playlist->priv->current != NULL) {
		GtkTreePath *item;

		item = gtk_tree_path_copy (playlist->priv->current);
		gtk_tree_path_next (item);
		next = gtk_tree_row_reference_new (playlist->priv->model, item);
		while (next != NULL) {
			if (totem_playlist_item_to_be_removed (playlist, item, func) == FALSE) {
				/* Found the item after the current one that
				 * won't be removed, thus the new current */
				break;
			}
			gtk_tree_row_reference_free (next);
			gtk_tree_path_next (item);
			next = gtk_tree_row_reference_new (playlist->priv->model, item);
		}
	}

	/* We destroy the items, one-by-one from the list built above */
	while (playlist->priv->list != NULL) {
		GtkTreePath *path;
		GtkTreeIter iter;

		path = gtk_tree_row_reference_get_path
			((GtkTreeRowReference *)(playlist->priv->list->data));
		gtk_tree_model_get_iter (playlist->priv->model, &iter, path);
		gtk_tree_path_free (path);

		totem_playlist_emit_item_removed (playlist, &iter);
		gtk_list_store_remove (GTK_LIST_STORE (playlist->priv->model), &iter);

		gtk_tree_row_reference_free
			((GtkTreeRowReference *)(playlist->priv->list->data));
		playlist->priv->list = g_list_remove (playlist->priv->list,
				playlist->priv->list->data);
	}
	g_list_free (playlist->priv->list);
	playlist->priv->list = NULL;

	if (playlist->priv->current_to_be_removed != FALSE) {
		/* The current item was removed from the playlist */
		if (next != NULL) {
			playlist->priv->current = gtk_tree_row_reference_get_path (next);
			gtk_tree_row_reference_free (next);
		} else {
			playlist->priv->current = NULL;
		}

		playlist->priv->current_shuffled = -1;
		if (playlist->priv->shuffle)
			ensure_shuffled (playlist);

		g_signal_emit (G_OBJECT (playlist),
				totem_playlist_table_signals[CURRENT_REMOVED],
				0, NULL);
	} else {
		if (ref != NULL) {
			/* The path to the current item changed */
			playlist->priv->current = gtk_tree_row_reference_get_path (ref);
		}

		if (playlist->priv->shuffle)
			ensure_shuffled (playlist);

		g_signal_emit (G_OBJECT (playlist),
				totem_playlist_table_signals[CHANGED], 0,
				NULL);
	}
	if (ref != NULL)
		gtk_tree_row_reference_free (ref);
	totem_playlist_update_save_button (playlist);
	gtk_tree_view_columns_autosize (GTK_TREE_VIEW (playlist->priv->treeview));

	playlist->priv->current_to_be_removed = FALSE;
}

static gboolean
totem_playlist_compare_with_mount (TotemPlaylist *playlist, GtkTreeIter *iter, gconstpointer data)
{
	GMount *clear_mount = (GMount *) data;
	char *mrl;

	GMount *mount;
	gboolean retval = FALSE;

	gtk_tree_model_get (playlist->priv->model, iter,
			    URI_COL, &mrl, -1);
	mount = totem_get_mount_for_media (mrl);
	g_free (mrl);

	if (mount == clear_mount)
		retval = TRUE;

	if (mount != NULL)
		g_object_unref (mount);

	return retval;
}

void
totem_playlist_clear_with_g_mount (TotemPlaylist *playlist,
				   GMount *mount)
{
	totem_playlist_clear_with_compare (playlist,
					   (ClearComparisonFunc) totem_playlist_compare_with_mount,
					   mount);
}

char *
totem_playlist_get_current_mrl (TotemPlaylist *playlist, char **subtitle)
{
	GtkTreeIter iter;
	char *path;

	if (subtitle != NULL)
		*subtitle = NULL;

	g_return_val_if_fail (TOTEM_IS_PLAYLIST (playlist), NULL);

	if (update_current_from_playlist (playlist) == FALSE)
		return NULL;

	if (gtk_tree_model_get_iter (playlist->priv->model, &iter,
				     playlist->priv->current) == FALSE)
		return NULL;

	if (subtitle != NULL) {
		gtk_tree_model_get (playlist->priv->model, &iter,
				    URI_COL, &path,
				    SUBTITLE_URI_COL, subtitle,
				    -1);
	} else {
		gtk_tree_model_get (playlist->priv->model, &iter,
				    URI_COL, &path,
				    -1);
	}

	return path;
}

char *
totem_playlist_get_current_title (TotemPlaylist *playlist, gboolean *custom)
{
	GtkTreeIter iter;
	char *path;

	g_return_val_if_fail (TOTEM_IS_PLAYLIST (playlist), NULL);

	if (update_current_from_playlist (playlist) == FALSE)
		return NULL;

	gtk_tree_model_get_iter (playlist->priv->model,
			&iter,
			playlist->priv->current);

	if (custom != NULL) {
		gtk_tree_model_get (playlist->priv->model,
				    &iter,
				    FILENAME_COL, &path,
				    TITLE_CUSTOM_COL, custom,
				    -1);
	} else {
		gtk_tree_model_get (playlist->priv->model,
				    &iter,
				    FILENAME_COL, &path,
				    -1);
	}

	return path;
}

char *
totem_playlist_get_title (TotemPlaylist *playlist, guint title_index)
{
	GtkTreeIter iter;
	GtkTreePath *path;
	char *title;

	g_return_val_if_fail (TOTEM_IS_PLAYLIST (playlist), NULL);

	path = gtk_tree_path_new_from_indices (title_index, -1);

	gtk_tree_model_get_iter (playlist->priv->model,
				 &iter, path);

	gtk_tree_path_free (path);

	gtk_tree_model_get (playlist->priv->model,
			    &iter,
			    FILENAME_COL, &title,
			    -1);

	return title;
}

gboolean
totem_playlist_has_previous_mrl (TotemPlaylist *playlist)
{
	GtkTreeIter iter;

	g_return_val_if_fail (TOTEM_IS_PLAYLIST (playlist), FALSE);

	if (update_current_from_playlist (playlist) == FALSE)
		return FALSE;

	if (playlist->priv->repeat != FALSE)
		return TRUE;

	if (playlist->priv->shuffle == FALSE)
	{
		gtk_tree_model_get_iter (playlist->priv->model,
				&iter,
				playlist->priv->current);

		return totem_playlist_gtk_tree_model_iter_previous
			(playlist->priv->model, &iter);
	} else {
		if (playlist->priv->current_shuffled == 0)
			return FALSE;
	}

	return TRUE;
}

gboolean
totem_playlist_has_next_mrl (TotemPlaylist *playlist)
{
	GtkTreeIter iter;

	g_return_val_if_fail (TOTEM_IS_PLAYLIST (playlist), FALSE);

	if (update_current_from_playlist (playlist) == FALSE)
		return FALSE;

	if (playlist->priv->repeat != FALSE)
		return TRUE;

	if (playlist->priv->shuffle == FALSE)
	{
		gtk_tree_model_get_iter (playlist->priv->model,
				&iter,
				playlist->priv->current);

		return gtk_tree_model_iter_next (playlist->priv->model, &iter);
	} else {
		if (playlist->priv->current_shuffled == PL_LEN - 1)
			return FALSE;
	}

	return TRUE;
}

gboolean
totem_playlist_set_title (TotemPlaylist *playlist, const char *title, gboolean force)
{
	GtkListStore *store;
	GtkTreeIter iter;
	gboolean custom_title;
	char *escaped_title;

	g_return_val_if_fail (TOTEM_IS_PLAYLIST (playlist), FALSE);

	if (update_current_from_playlist (playlist) == FALSE)
		return FALSE;

	store = GTK_LIST_STORE (playlist->priv->model);
	gtk_tree_model_get_iter (playlist->priv->model,
			&iter,
			playlist->priv->current);

	if (&iter == NULL)
		return FALSE;

	if (force == FALSE) {
		gtk_tree_model_get (playlist->priv->model, &iter,
				TITLE_CUSTOM_COL, &custom_title,
				-1);
		if (custom_title != FALSE)
			return TRUE;
	}

	escaped_title = g_markup_escape_text (title, -1);
	gtk_list_store_set (store, &iter,
			FILENAME_COL, title,
			FILENAME_ESCAPED_COL, escaped_title,
			TITLE_CUSTOM_COL, TRUE,
			-1);
	g_free (escaped_title);

	g_signal_emit (playlist,
		       totem_playlist_table_signals[ACTIVE_NAME_CHANGED], 0);

	return TRUE;
}

gboolean
totem_playlist_set_playing (TotemPlaylist *playlist, TotemPlaylistStatus state)
{
	GtkListStore *store;
	GtkTreeIter iter;
	GtkTreePath *path;

	g_return_val_if_fail (TOTEM_IS_PLAYLIST (playlist), FALSE);

	if (update_current_from_playlist (playlist) == FALSE)
		return FALSE;

	store = GTK_LIST_STORE (playlist->priv->model);
	gtk_tree_model_get_iter (playlist->priv->model,
			&iter,
			playlist->priv->current);

	g_return_val_if_fail (&iter != NULL, FALSE);

	gtk_list_store_set (store, &iter,
			PLAYING_COL, state,
			-1);

	if (state == FALSE)
		return TRUE;

	path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &iter);
	gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (playlist->priv->treeview),
				      path, NULL,
				      TRUE, 0.5, 0);
	gtk_tree_path_free (path);
	
	return TRUE;
}

TotemPlaylistStatus
totem_playlist_get_playing (TotemPlaylist *playlist)
{
	GtkTreeIter iter;
	TotemPlaylistStatus status;

	g_return_val_if_fail (TOTEM_IS_PLAYLIST (playlist), TOTEM_PLAYLIST_STATUS_NONE);

	if (gtk_tree_model_get_iter (playlist->priv->model, &iter, playlist->priv->current) == FALSE)
		return TOTEM_PLAYLIST_STATUS_NONE;

	gtk_tree_model_get (playlist->priv->model,
			    &iter,
			    PLAYING_COL, &status,
			    -1);

	return status;
}

void
totem_playlist_set_previous (TotemPlaylist *playlist)
{
	GtkTreeIter iter;

	g_return_if_fail (TOTEM_IS_PLAYLIST (playlist));

	if (totem_playlist_has_previous_mrl (playlist) == FALSE)
		return;

	totem_playlist_unset_playing (playlist);

	if (playlist->priv->shuffle == FALSE)
	{
		char *path;

		path = gtk_tree_path_to_string (playlist->priv->current);
		if (strcmp (path, "0") == 0)
		{
			totem_playlist_set_at_end (playlist);
			g_free (path);
			return;
		}
		g_free (path);

		gtk_tree_model_get_iter (playlist->priv->model,
				&iter,
				playlist->priv->current);

		totem_playlist_gtk_tree_model_iter_previous
			(playlist->priv->model, &iter);
		gtk_tree_path_free (playlist->priv->current);
		playlist->priv->current = gtk_tree_model_get_path
			(playlist->priv->model, &iter);
	} else {
		int indice;

		gtk_tree_path_free (playlist->priv->current);
		playlist->priv->current_shuffled--;
		if (playlist->priv->current_shuffled < 0) {
			indice = playlist->priv->shuffled[PL_LEN -1];
			playlist->priv->current_shuffled = PL_LEN -1;
		} else {
			indice = playlist->priv->shuffled[playlist->priv->current_shuffled];
		}
		playlist->priv->current = gtk_tree_path_new_from_indices
			(indice, -1);
	}
}

void
totem_playlist_set_next (TotemPlaylist *playlist)
{
	GtkTreeIter iter;

	g_return_if_fail (TOTEM_IS_PLAYLIST (playlist));

	if (totem_playlist_has_next_mrl (playlist) == FALSE) {
		totem_playlist_set_at_start (playlist);
		return;
	}

	totem_playlist_unset_playing (playlist);

	if (playlist->priv->shuffle == FALSE) {
		gtk_tree_model_get_iter (playlist->priv->model,
					 &iter,
					 playlist->priv->current);

		gtk_tree_model_iter_next (playlist->priv->model, &iter);
		gtk_tree_path_free (playlist->priv->current);
		playlist->priv->current = gtk_tree_model_get_path (playlist->priv->model, &iter);
	} else {
		int indice;

		gtk_tree_path_free (playlist->priv->current);
		playlist->priv->current_shuffled++;
		if (playlist->priv->current_shuffled == PL_LEN)
			playlist->priv->current_shuffled = 0;
		indice = playlist->priv->shuffled[playlist->priv->current_shuffled];
		playlist->priv->current = gtk_tree_path_new_from_indices
			                        (indice, -1);
	}
}

gboolean
totem_playlist_get_repeat (TotemPlaylist *playlist)
{
	g_return_val_if_fail (TOTEM_IS_PLAYLIST (playlist), FALSE);

	return playlist->priv->repeat;
}
	
void
totem_playlist_set_repeat (TotemPlaylist *playlist, gboolean repeat)
{
	g_return_if_fail (TOTEM_IS_PLAYLIST (playlist));

	mateconf_client_set_bool (playlist->priv->gc, MATECONF_PREFIX"/repeat",
			repeat, NULL);
}

gboolean
totem_playlist_get_shuffle (TotemPlaylist *playlist)
{
	g_return_val_if_fail (TOTEM_IS_PLAYLIST (playlist), FALSE);

	return playlist->priv->shuffle;
}

void
totem_playlist_set_shuffle (TotemPlaylist *playlist, gboolean shuffle)
{
	g_return_if_fail (TOTEM_IS_PLAYLIST (playlist));

	mateconf_client_set_bool (playlist->priv->gc, MATECONF_PREFIX"/shuffle",
			shuffle, NULL);
}

void
totem_playlist_set_at_start (TotemPlaylist *playlist)
{
	g_return_if_fail (TOTEM_IS_PLAYLIST (playlist));

	totem_playlist_unset_playing (playlist);

	if (playlist->priv->current != NULL)
	{
		gtk_tree_path_free (playlist->priv->current);
		playlist->priv->current = NULL;
	}
	update_current_from_playlist (playlist);
}

void
totem_playlist_set_at_end (TotemPlaylist *playlist)
{
	int indice;

	g_return_if_fail (TOTEM_IS_PLAYLIST (playlist));

	totem_playlist_unset_playing (playlist);

	if (playlist->priv->current != NULL)
	{
		gtk_tree_path_free (playlist->priv->current);
		playlist->priv->current = NULL;
	}

	if (PL_LEN)
	{
		if (playlist->priv->shuffle == FALSE)
			indice = PL_LEN - 1;
		else
			indice = playlist->priv->shuffled[PL_LEN - 1];

		playlist->priv->current = gtk_tree_path_new_from_indices
			(indice, -1);
	}
}

int
totem_playlist_get_current (TotemPlaylist *playlist)
{
	char *path;
	double current_index;

	g_return_val_if_fail (TOTEM_IS_PLAYLIST (playlist), -1);

	if (playlist->priv->current == NULL)
		return -1;
	path = gtk_tree_path_to_string (playlist->priv->current);
	if (path == NULL)
		return -1;

	current_index = g_ascii_strtod (path, NULL);
	g_free (path);

	return current_index;
}

int
totem_playlist_get_last (TotemPlaylist *playlist)
{
	guint len = PL_LEN;

	g_return_val_if_fail (TOTEM_IS_PLAYLIST (playlist), -1);

	if (len == 0)
		return -1;

	return len - 1;
}

void
totem_playlist_set_current (TotemPlaylist *playlist, guint current_index)
{
	g_return_if_fail (TOTEM_IS_PLAYLIST (playlist));

	if (current_index >= (guint) PL_LEN)
		return;

	totem_playlist_unset_playing (playlist);
	//FIXME problems when shuffled?
	gtk_tree_path_free (playlist->priv->current);
	playlist->priv->current = gtk_tree_path_new_from_indices (current_index, -1);
}

static void
totem_playlist_class_init (TotemPlaylistClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (TotemPlaylistPrivate));

	object_class->dispose = totem_playlist_dispose;
	object_class->finalize = totem_playlist_finalize;

	/* Signals */
	totem_playlist_table_signals[CHANGED] =
		g_signal_new ("changed",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (TotemPlaylistClass, changed),
				NULL, NULL,
				g_cclosure_marshal_VOID__VOID,
				G_TYPE_NONE, 0);
	totem_playlist_table_signals[ITEM_ACTIVATED] =
		g_signal_new ("item-activated",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (TotemPlaylistClass, item_activated),
				NULL, NULL,
				g_cclosure_marshal_VOID__VOID,
				G_TYPE_NONE, 0);
	totem_playlist_table_signals[ACTIVE_NAME_CHANGED] =
		g_signal_new ("active-name-changed",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (TotemPlaylistClass, active_name_changed),
				NULL, NULL,
				g_cclosure_marshal_VOID__VOID,
				G_TYPE_NONE, 0);
	totem_playlist_table_signals[CURRENT_REMOVED] =
		g_signal_new ("current-removed",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (TotemPlaylistClass,
						 current_removed),
				NULL, NULL,
				g_cclosure_marshal_VOID__VOID,
				G_TYPE_NONE, 0);
	totem_playlist_table_signals[REPEAT_TOGGLED] =
		g_signal_new ("repeat-toggled",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (TotemPlaylistClass,
						 repeat_toggled),
				NULL, NULL,
				g_cclosure_marshal_VOID__BOOLEAN,
				G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
	totem_playlist_table_signals[SHUFFLE_TOGGLED] =
		g_signal_new ("shuffle-toggled",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (TotemPlaylistClass,
						 shuffle_toggled),
				NULL, NULL,
				g_cclosure_marshal_VOID__BOOLEAN,
				G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
	totem_playlist_table_signals[SUBTITLE_CHANGED] =
		g_signal_new ("subtitle-changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (TotemPlaylistClass,
					       subtitle_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
	totem_playlist_table_signals[ITEM_ADDED] =
		g_signal_new ("item-added",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (TotemPlaylistClass,
					item_added),
				NULL, NULL,
				totemplaylist_marshal_VOID__STRING_STRING,
				G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_STRING);
	totem_playlist_table_signals[ITEM_REMOVED] =
		g_signal_new ("item-removed",
				G_TYPE_FROM_CLASS (klass),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (TotemPlaylistClass,
					item_removed),
				NULL, NULL,
				totemplaylist_marshal_VOID__STRING_STRING,
				G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_STRING);
}

static gboolean
totem_playlist_foreach_cb (GtkTreeModel *model,
			   GtkTreePath  *path,
			   GtkTreeIter  *iter,
			   gpointer      data)
{
	PlaylistForeachContext *context = data;
	gchar *filename = NULL;
	gchar *uri = NULL;

	gtk_tree_model_get (model, iter, URI_COL, &uri, FILENAME_COL, &filename, -1);
	context->callback (context->playlist, filename, uri, context->user_data);

	g_free (filename);
	g_free (uri);

	return FALSE;
}

void
totem_playlist_foreach (TotemPlaylist            *playlist,
			TotemPlaylistForeachFunc  callback,
			gpointer                  user_data)
{
	PlaylistForeachContext context = { playlist, callback, user_data };

	g_return_if_fail (TOTEM_IS_PLAYLIST (playlist));
	g_return_if_fail (NULL != callback);

	gtk_tree_model_foreach (playlist->priv->model,
				totem_playlist_foreach_cb,
				&context);
}
