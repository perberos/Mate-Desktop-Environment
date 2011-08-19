/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2007 Bastien Nocera <hadess@hadess.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
 *
 * The Totem project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and Totem. This
 * permission are above and beyond the permissions granted by the GPL license
 * Totem is covered by.
 *
 * Monday 7th February 2005: Christian Schaller: Add exception clause.
 * See license_change file for details.
 *
 */

/**
 * SECTION:totem-object
 * @short_description: main Totem object
 * @stability: Unstable
 * @include: totem.h
 *
 * #TotemObject is the core object of Totem; a singleton which controls all Totem's main functions.
 **/

#include "config.h"

#include <glib-object.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <totem-disc.h>
#include <stdlib.h>
#include <math.h>
#include <gio/gio.h>

#include <string.h>

#ifdef GDK_WINDOWING_X11
/* X11 headers */
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#endif

#include "totem.h"
#include "totemobject-marshal.h"
#include "totem-private.h"
#include "totem-plugins-engine.h"
#include "ev-sidebar.h"
#include "totem-playlist.h"
#include "bacon-video-widget.h"
#include "totem-statusbar.h"
#include "totem-time-label.h"
#include "totem-sidebar.h"
#include "totem-menu.h"
#include "totem-uri.h"
#include "totem-interface.h"
#include "video-utils.h"
#include "totem-dnd-menu.h"
#include "totem-preferences.h"

#define REWIND_OR_PREVIOUS 4000

#define SEEK_FORWARD_SHORT_OFFSET 15
#define SEEK_BACKWARD_SHORT_OFFSET -5

#define SEEK_FORWARD_LONG_OFFSET 10*60
#define SEEK_BACKWARD_LONG_OFFSET -3*60

#define ZOOM_UPPER 2.0
#define ZOOM_RESET 1.0
#define ZOOM_LOWER 0.1
#define ZOOM_DISABLE (ZOOM_LOWER - 1)
#define ZOOM_ENABLE (ZOOM_UPPER + 1)

#define DEFAULT_WINDOW_W 650
#define DEFAULT_WINDOW_H 500

#define VOLUME_EPSILON (1e-10)

/* casts are to shut gcc up */
static const GtkTargetEntry target_table[] = {
	{ (gchar*) "text/uri-list", 0, 0 },
	{ (gchar*) "_NETSCAPE_URL", 0, 1 }
};

static gboolean totem_action_open_files_list (Totem *totem, GSList *list);
static gboolean totem_action_load_media (Totem *totem, TotemDiscMediaType type, const char *device);
static void update_buttons (Totem *totem);
static void update_fill (Totem *totem, gdouble level);
static void update_media_menu_items (Totem *totem);
static void playlist_changed_cb (GtkWidget *playlist, Totem *totem);
static void play_pause_set_label (Totem *totem, TotemStates state);

/* Callback functions for GtkBuilder */
G_MODULE_EXPORT gboolean main_window_destroy_cb (GtkWidget *widget, GdkEvent *event, Totem *totem);
G_MODULE_EXPORT gboolean window_state_event_cb (GtkWidget *window, GdkEventWindowState *event, Totem *totem);
G_MODULE_EXPORT gboolean seek_slider_pressed_cb (GtkWidget *widget, GdkEventButton *event, Totem *totem);
G_MODULE_EXPORT void seek_slider_changed_cb (GtkAdjustment *adj, Totem *totem);
G_MODULE_EXPORT gboolean seek_slider_released_cb (GtkWidget *widget, GdkEventButton *event, Totem *totem);
G_MODULE_EXPORT void volume_button_value_changed_cb (GtkScaleButton *button, gdouble value, Totem *totem);
G_MODULE_EXPORT gboolean window_key_press_event_cb (GtkWidget *win, GdkEventKey *event, Totem *totem);
G_MODULE_EXPORT int window_scroll_event_cb (GtkWidget *win, GdkEventScroll *event, Totem *totem);
G_MODULE_EXPORT void main_pane_size_allocated (GtkWidget *main_pane, GtkAllocation *allocation, Totem *totem);
G_MODULE_EXPORT void fs_exit1_activate_cb (GtkButton *button, Totem *totem);

enum {
	PROP_0,
	PROP_FULLSCREEN,
	PROP_PLAYING,
	PROP_STREAM_LENGTH,
	PROP_SEEKABLE,
	PROP_CURRENT_TIME,
	PROP_CURRENT_MRL
};

enum {
	FILE_OPENED,
	FILE_CLOSED,
	METADATA_UPDATED,
	LAST_SIGNAL
};

static void totem_object_set_property		(GObject *object,
						 guint property_id,
						 const GValue *value,
						 GParamSpec *pspec);
static void totem_object_get_property		(GObject *object,
						 guint property_id,
						 GValue *value,
						 GParamSpec *pspec);
static void totem_object_finalize (GObject *totem);

static int totem_table_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE(TotemObject, totem_object, G_TYPE_OBJECT)

static void
totem_object_class_init (TotemObjectClass *klass)
{
	GObjectClass *object_class;

	object_class = (GObjectClass *) klass;

	object_class->set_property = totem_object_set_property;
	object_class->get_property = totem_object_get_property;
	object_class->finalize = totem_object_finalize;

	/**
	 * TotemObject:fullscreen:
	 *
	 * If %TRUE, Totem is in fullscreen mode.
	 **/
	g_object_class_install_property (object_class, PROP_FULLSCREEN,
					 g_param_spec_boolean ("fullscreen", NULL, NULL,
							       FALSE, G_PARAM_READABLE));

	/**
	 * TotemObject:playing:
	 *
	 * If %TRUE, Totem is playing an audio or video file.
	 **/
	g_object_class_install_property (object_class, PROP_PLAYING,
					 g_param_spec_boolean ("playing", NULL, NULL,
							       FALSE, G_PARAM_READABLE));

	/**
	 * TotemObject:stream-length:
	 *
	 * The length of the current stream, in milliseconds.
	 **/
	g_object_class_install_property (object_class, PROP_STREAM_LENGTH,
					 g_param_spec_int64 ("stream-length", NULL, NULL,
							     G_MININT64, G_MAXINT64, 0,
							     G_PARAM_READABLE));

	/**
	 * TotemObject:current-time:
	 *
	 * The player's position (time) in the current stream, in milliseconds.
	 **/
	g_object_class_install_property (object_class, PROP_CURRENT_TIME,
					 g_param_spec_int64 ("current-time", NULL, NULL,
							     G_MININT64, G_MAXINT64, 0,
							     G_PARAM_READABLE));

	/**
	 * TotemObject:seekable:
	 *
	 * If %TRUE, the current stream is seekable.
	 **/
	g_object_class_install_property (object_class, PROP_SEEKABLE,
					 g_param_spec_boolean ("seekable", NULL, NULL,
							       FALSE, G_PARAM_READABLE));

	/**
	 * TotemObject:current-mrl:
	 *
	 * The MRL of the current stream.
	 **/
	g_object_class_install_property (object_class, PROP_CURRENT_MRL,
					 g_param_spec_string ("current-mrl", NULL, NULL,
							      NULL, G_PARAM_READABLE));

	/**
	 * TotemObject::file-opened:
	 * @totem: the #TotemObject which received the signal
	 * @mrl: the MRL of the opened stream
	 *
	 * The #TotemObject::file-opened signal is emitted when a new stream is opened by Totem.
	 */
	totem_table_signals[FILE_OPENED] =
		g_signal_new ("file-opened",
				G_TYPE_FROM_CLASS (object_class),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (TotemObjectClass, file_opened),
				NULL, NULL,
				g_cclosure_marshal_VOID__STRING,
				G_TYPE_NONE, 1, G_TYPE_STRING);

	/**
	 * TotemObject::file-closed:
	 * @totem: the #TotemObject which received the signal
	 *
	 * The #TotemObject::file-closed signal is emitted when Totem closes a stream.
	 */
	totem_table_signals[FILE_CLOSED] =
		g_signal_new ("file-closed",
				G_TYPE_FROM_CLASS (object_class),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (TotemObjectClass, file_closed),
				NULL, NULL,
				g_cclosure_marshal_VOID__VOID,
				G_TYPE_NONE, 0, G_TYPE_NONE);

	/**
	 * TotemObject::metadata-updated:
	 * @totem: the #TotemObject which received the signal
	 * @artist: the name of the artist, or %NULL
	 * @title: the stream title, or %NULL
	 * @album: the name of the stream's album, or %NULL
	 * @track_number: the stream's track number
	 *
	 * The #TotemObject::metadata-updated signal is emitted when the metadata of a stream is updated, typically
	 * when it's being loaded.
	 */
	totem_table_signals[METADATA_UPDATED] =
		g_signal_new ("metadata-updated",
				G_TYPE_FROM_CLASS (object_class),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (TotemObjectClass, metadata_updated),
				NULL, NULL,
				totemobject_marshal_VOID__STRING_STRING_STRING_UINT,
				G_TYPE_NONE, 4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT);
}

static void
totem_object_init (TotemObject *totem)
{
	//FIXME nothing yet
}

static void
totem_object_finalize (GObject *object)
{

	G_OBJECT_CLASS (totem_object_parent_class)->finalize (object);
}

static void
totem_object_set_property (GObject *object,
			   guint property_id,
			   const GValue *value,
			   GParamSpec *pspec)
{
	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
totem_object_get_property (GObject *object,
			   guint property_id,
			   GValue *value,
			   GParamSpec *pspec)
{
	TotemObject *totem;

	totem = TOTEM_OBJECT (object);

	switch (property_id)
	{
	case PROP_FULLSCREEN:
		g_value_set_boolean (value, totem_is_fullscreen (totem));
		break;
	case PROP_PLAYING:
		g_value_set_boolean (value, totem_is_playing (totem));
		break;
	case PROP_STREAM_LENGTH:
		g_value_set_int64 (value, bacon_video_widget_get_stream_length (totem->bvw));
		break;
	case PROP_CURRENT_TIME:
		g_value_set_int64 (value, bacon_video_widget_get_current_time (totem->bvw));
		break;
	case PROP_SEEKABLE:
		g_value_set_boolean (value, totem_is_seekable (totem));
		break;
	case PROP_CURRENT_MRL:
		g_value_set_string (value, totem->mrl);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

/**
 * totem_object_plugins_init:
 * @totem: a #TotemObject
 *
 * Initialises the plugin engine and activates all the
 * enabled plugins.
 **/
void
totem_object_plugins_init (TotemObject *totem)
{
	totem_plugins_engine_init (totem);
}

/**
 * totem_object_plugins_shutdown:
 *
 * Shuts down the plugin engine and deactivates all the
 * plugins.
 **/
void
totem_object_plugins_shutdown (void)
{
	totem_plugins_engine_shutdown ();
}

/**
 * totem_get_main_window:
 * @totem: a #TotemObject
 *
 * Gets Totem's main window and increments its reference count.
 *
 * Return value: Totem's main window
 **/
GtkWindow *
totem_get_main_window (Totem *totem)
{
	g_return_val_if_fail (TOTEM_IS_OBJECT (totem), NULL);

	g_object_ref (G_OBJECT (totem->win));

	return GTK_WINDOW (totem->win);
}

/**
 * totem_get_ui_manager:
 * @totem: a #TotemObject
 *
 * Gets Totem's UI manager, but does not change its reference count.
 *
 * Return value: Totem's UI manager
 **/
GtkUIManager *
totem_get_ui_manager (Totem *totem)
{
	g_return_val_if_fail (TOTEM_IS_OBJECT (totem), NULL);

	return totem->ui_manager;
}

/**
 * totem_get_video_widget:
 * @totem: a #TotemObject
 *
 * Gets Totem's video widget and increments its reference count.
 *
 * Return value: Totem's video widget
 **/
GtkWidget *
totem_get_video_widget (Totem *totem)
{
	g_return_val_if_fail (TOTEM_IS_OBJECT (totem), NULL);

	g_object_ref (G_OBJECT (totem->bvw));

	return GTK_WIDGET (totem->bvw);
}

/**
 * totem_get_video_widget_backend_name:
 * @totem: a #TotemObject
 *
 * Gets the name string of the backend video widget, typically the video library's
 * version string (e.g. what's returned by gst_version_string()). Free with g_free().
 *
 * Return value: a newly-allocated string of the name of the backend video widget
 **/
char *
totem_get_video_widget_backend_name (Totem *totem)
{
	return bacon_video_widget_get_backend_name (totem->bvw);
}

/**
 * totem_get_version:
 *
 * Gets the application name and version (e.g. "Totem 2.28.0").
 *
 * Return value: a newly-allocated string of the name and version of the application
 **/
char *
totem_get_version (void)
{
	/* Translators: %s is the totem version number */
	return g_strdup_printf (_("Totem %s"), VERSION);
}

/**
 * totem_get_current_time:
 * @totem: a #TotemObject
 *
 * Gets the current position's time in the stream as a gint64.
 *
 * Return value: the current position in the stream
 **/
gint64
totem_get_current_time (Totem *totem)
{
	g_return_val_if_fail (TOTEM_IS_OBJECT (totem), 0);

	return bacon_video_widget_get_current_time (totem->bvw);
}

typedef struct {
	Totem *totem;
	gchar *uri;
	gchar *display_name;
	gboolean add_to_recent;
} AddToPlaylistData;

static void
add_to_playlist_and_play_cb (TotemPlaylist *playlist, GAsyncResult *async_result, AddToPlaylistData *data)
{
	int end;
	gboolean playlist_changed;

	playlist_changed = totem_playlist_add_mrl_finish (playlist, async_result);

	if (data->add_to_recent != FALSE)
		gtk_recent_manager_add_item (data->totem->recent_manager, data->uri);
	end = totem_playlist_get_last (playlist);

	totem_signal_unblock_by_data (playlist, data->totem);

	if (playlist_changed && end != -1) {
		char *mrl, *subtitle;

		subtitle = NULL;
		totem_playlist_set_current (playlist, end);
		mrl = totem_playlist_get_current_mrl (playlist, &subtitle);
		totem_action_set_mrl_and_play (data->totem, mrl, subtitle);
		g_free (mrl);
		g_free (subtitle);
	}

	/* Free the closure data */
	g_object_unref (data->totem);
	g_free (data->uri);
	g_free (data->display_name);
	g_slice_free (AddToPlaylistData, data);
}

/**
 * totem_add_to_playlist_and_play:
 * @totem: a #TotemObject
 * @uri: the URI to add to the playlist
 * @display_name: the display name of the URI
 * @add_to_recent: if %TRUE, add the URI to the recent items list
 *
 * Add @uri to the playlist and play it immediately.
 **/
void
totem_add_to_playlist_and_play (Totem *totem,
				const char *uri,
				const char *display_name,
				gboolean add_to_recent)
{
	AddToPlaylistData *data;

	/* Block all signals from the playlist until we're finished. They're unblocked in the callback, add_to_playlist_and_play_cb.
	 * There are no concurrency issues here, since blocking the signals multiple times should require them to be unblocked the
	 * same number of times before they fire again. */
	totem_signal_block_by_data (totem->playlist, totem);

	data = g_slice_new (AddToPlaylistData);
	data->totem = g_object_ref (totem);
	data->uri = g_strdup (uri);
	data->display_name = g_strdup (display_name);
	data->add_to_recent = add_to_recent;

	totem_playlist_add_mrl (totem->playlist, uri, display_name, TRUE,
	                        NULL, (GAsyncReadyCallback) add_to_playlist_and_play_cb, data);
}

/**
 * totem_get_current_mrl:
 * @totem: a #TotemObject
 *
 * Get the MRL of the current stream, or %NULL if nothing's playing.
 * Free with g_free().
 *
 * Return value: a newly-allocated string containing the MRL of the current stream
 **/
char *
totem_get_current_mrl (Totem *totem)
{
	return totem_playlist_get_current_mrl (totem->playlist, NULL);
}

/**
 * totem_get_playlist_length:
 * @totem: a #TotemObject
 *
 * Returns the length of the current playlist.
 *
 * Return value: the playlist length
 **/
guint
totem_get_playlist_length (Totem *totem)
{
	int last;

	last = totem_playlist_get_last (totem->playlist);
	if (last == -1)
		return 0;
	return last + 1;
}

/**
 * totem_get_playlist_pos:
 * @totem: a #TotemObject
 *
 * Returns the %0-based index of the current entry in the playlist. If
 * there is no current entry in the playlist, %-1 is returned.
 *
 * Return value: the index of the current playlist entry, or %-1
 **/
int
totem_get_playlist_pos (Totem *totem)
{
	return totem_playlist_get_current (totem->playlist);
}

/**
 * totem_get_title_at_playlist_pos:
 * @totem: a #TotemObject
 * @playlist_index: the %0-based entry index
 *
 * Gets the title of the playlist entry at @index.
 *
 * Return value: the entry title at @index, or %NULL; free with g_free()
 **/
char *
totem_get_title_at_playlist_pos (Totem *totem, guint playlist_index)
{
	return totem_playlist_get_title (totem->playlist, playlist_index);
}

/**
 * totem_get_short_title:
 * @totem: a #TotemObject
 *
 * Gets the title of the current entry in the playlist.
 *
 * Return value: the current entry's title, or %NULL; free with g_free()
 **/
char *
totem_get_short_title (Totem *totem)
{
	gboolean custom;
	return totem_playlist_get_current_title (totem->playlist, &custom);
}

/**
 * totem_set_current_subtitle:
 * @totem: a #TotemObject
 * @subtitle_uri: the URI of the subtitle file to add
 *
 * Add the @subtitle_uri subtitle file to the playlist, setting it as the subtitle for the current
 * playlist entry.
 **/
void
totem_set_current_subtitle (Totem *totem, const char *subtitle_uri)
{
	totem_playlist_set_current_subtitle (totem->playlist, subtitle_uri);
}

/**
 * totem_add_sidebar_page:
 * @totem: a #TotemObject
 * @page_id: a string used to identify the page
 * @title: the page's title
 * @main_widget: the main widget for the page
 *
 * Adds a sidebar page to Totem's sidebar with the given @page_id.
 * @main_widget is added into the page and shown automatically, while
 * @title is displayed as the page's title in the tab bar.
 **/
void
totem_add_sidebar_page (Totem *totem,
			const char *page_id,
			const char *title,
			GtkWidget *main_widget)
{
	ev_sidebar_add_page (EV_SIDEBAR (totem->sidebar),
			     page_id,
			     title,
			     main_widget);
}

/**
 * totem_remove_sidebar_page:
 * @totem: a #TotemObject
 * @page_id: a string used to identify the page
 *
 * Removes the page identified by @page_id from Totem's sidebar.
 * If @page_id doesn't exist in the sidebar, this function does
 * nothing.
 **/
void
totem_remove_sidebar_page (Totem *totem,
			   const char *page_id)
{
	ev_sidebar_remove_page (EV_SIDEBAR (totem->sidebar),
				page_id);
}

/**
 * totem_file_opened:
 * @totem: a #TotemObject
 * @mrl: the MRL opened
 *
 * Emits the #TotemObject::file-opened signal on @totem, with the
 * specified @mrl.
 **/
void
totem_file_opened (TotemObject *totem,
		   const char *mrl)
{
	g_signal_emit (G_OBJECT (totem),
		       totem_table_signals[FILE_OPENED],
		       0, mrl);
}

/**
 * totem_file_closed:
 * @totem: a #TotemObject
 *
 * Emits the #TotemObject::file-closed signal on @totem.
 **/
void
totem_file_closed (TotemObject *totem)
{
	g_signal_emit (G_OBJECT (totem),
		       totem_table_signals[FILE_CLOSED],
		       0);

}

/**
 * totem_metadata_updated:
 * @totem: a #TotemObject
 * @artist: the stream's artist, or %NULL
 * @title: the stream's title, or %NULL
 * @album: the stream's album, or %NULL
 * @track_num: the track number of the stream
 *
 * Emits the #TotemObject::metadata-updated signal on @totem,
 * with the specified stream data.
 **/
void
totem_metadata_updated (TotemObject *totem,
			const char *artist,
			const char *title,
			const char *album,
			guint track_num)
{
	g_signal_emit (G_OBJECT (totem),
		       totem_table_signals[METADATA_UPDATED],
		       0,
		       artist,
		       title,
		       album,
		       track_num);
}

GQuark
totem_remote_command_quark (void)
{
	static GQuark quark = 0;
	if (!quark)
		quark = g_quark_from_static_string ("totem_remote_command");

	return quark;
}

/* This should really be standard. */
#define ENUM_ENTRY(NAME, DESC) { NAME, "" #NAME "", DESC }

GType
totem_remote_command_get_type (void)
{
	static GType etype = 0;

	if (etype == 0) {
		static const GEnumValue values[] = {
			ENUM_ENTRY (TOTEM_REMOTE_COMMAND_UNKNOWN, "unknown"),
			ENUM_ENTRY (TOTEM_REMOTE_COMMAND_PLAY, "play"),
			ENUM_ENTRY (TOTEM_REMOTE_COMMAND_PAUSE, "pause"),
			ENUM_ENTRY (TOTEM_REMOTE_COMMAND_STOP, "stop"),
			ENUM_ENTRY (TOTEM_REMOTE_COMMAND_PLAYPAUSE, "play-pause"),
			ENUM_ENTRY (TOTEM_REMOTE_COMMAND_NEXT, "next"),
			ENUM_ENTRY (TOTEM_REMOTE_COMMAND_PREVIOUS, "previous"),
			ENUM_ENTRY (TOTEM_REMOTE_COMMAND_SEEK_FORWARD, "seek-forward"),
			ENUM_ENTRY (TOTEM_REMOTE_COMMAND_SEEK_BACKWARD, "seek-backward"),
			ENUM_ENTRY (TOTEM_REMOTE_COMMAND_VOLUME_UP, "volume-up"),
			ENUM_ENTRY (TOTEM_REMOTE_COMMAND_VOLUME_DOWN, "volume-down"),
			ENUM_ENTRY (TOTEM_REMOTE_COMMAND_FULLSCREEN, "fullscreen"),
			ENUM_ENTRY (TOTEM_REMOTE_COMMAND_QUIT, "quit"),
			ENUM_ENTRY (TOTEM_REMOTE_COMMAND_ENQUEUE, "enqueue"),
			ENUM_ENTRY (TOTEM_REMOTE_COMMAND_REPLACE, "replace"),
			ENUM_ENTRY (TOTEM_REMOTE_COMMAND_SHOW, "show"),
			ENUM_ENTRY (TOTEM_REMOTE_COMMAND_TOGGLE_CONTROLS, "toggle-controls"),
			ENUM_ENTRY (TOTEM_REMOTE_COMMAND_UP, "up"),
			ENUM_ENTRY (TOTEM_REMOTE_COMMAND_DOWN, "down"),
			ENUM_ENTRY (TOTEM_REMOTE_COMMAND_LEFT, "left"),
			ENUM_ENTRY (TOTEM_REMOTE_COMMAND_RIGHT, "right"),
			ENUM_ENTRY (TOTEM_REMOTE_COMMAND_SELECT, "select"),
			ENUM_ENTRY (TOTEM_REMOTE_COMMAND_DVD_MENU, "dvd-menu"),
			ENUM_ENTRY (TOTEM_REMOTE_COMMAND_ZOOM_UP, "zoom-up"),
			ENUM_ENTRY (TOTEM_REMOTE_COMMAND_ZOOM_DOWN, "zoom-down"),
			ENUM_ENTRY (TOTEM_REMOTE_COMMAND_EJECT, "eject"),
			ENUM_ENTRY (TOTEM_REMOTE_COMMAND_PLAY_DVD, "play-dvd"),
			ENUM_ENTRY (TOTEM_REMOTE_COMMAND_MUTE, "mute"),
			ENUM_ENTRY (TOTEM_REMOTE_COMMAND_TOGGLE_ASPECT, "toggle-aspect-ratio"),
			{ 0, NULL, NULL }
		};

		etype = g_enum_register_static ("TotemRemoteCommand", values);
	}

	return etype;
}

GQuark
totem_remote_setting_quark (void)
{
	static GQuark quark = 0;
	if (!quark)
		quark = g_quark_from_static_string ("totem_remote_setting");

	return quark;
}

GType
totem_remote_setting_get_type (void)
{
	static GType etype = 0;

	if (etype == 0) {
		static const GEnumValue values[] = {
			ENUM_ENTRY (TOTEM_REMOTE_SETTING_SHUFFLE, "shuffle"),
			ENUM_ENTRY (TOTEM_REMOTE_SETTING_REPEAT, "repeat"),
			{ 0, NULL, NULL }
		};

		etype = g_enum_register_static ("TotemRemoteSetting", values);
	}

	return etype;
}

static void
reset_seek_status (Totem *totem)
{
	/* Release the lock and reset everything so that we
	 * avoid being "stuck" seeking on errors */

	if (totem->seek_lock != FALSE) {
		totem_statusbar_set_seeking (TOTEM_STATUSBAR (totem->statusbar), FALSE);
		totem_time_label_set_seeking (TOTEM_TIME_LABEL (totem->fs->time_label), FALSE);
		totem->seek_lock = FALSE;
		bacon_video_widget_seek (totem->bvw, 0, NULL);
		totem_action_stop (totem);
	}
}

/**
 * totem_action_error:
 * @title: the error dialog title
 * @reason: the error dialog text
 * @totem: a #TotemObject
 *
 * Displays a non-blocking error dialog with the
 * given @title and @reason.
 **/
void
totem_action_error (const char *title, const char *reason, Totem *totem)
{
	reset_seek_status (totem);
	totem_interface_error (title, reason,
			GTK_WINDOW (totem->win));
}

G_GNUC_NORETURN void
totem_action_error_and_exit (const char *title,
		const char *reason, Totem *totem)
{
	reset_seek_status (totem);
	totem_interface_error_blocking (title, reason,
			GTK_WINDOW (totem->win));
	totem_action_exit (totem);
}

static void
totem_action_save_size (Totem *totem)
{
	GtkPaned *item;

	if (totem->bvw == NULL)
		return;

	if (totem_is_fullscreen (totem) != FALSE)
		return;

	/* Save the size of the video widget */
	item = GTK_PANED (gtk_builder_get_object (totem->xml, "tmw_main_pane"));
	gtk_window_get_size (GTK_WINDOW (totem->win), &totem->window_w,
			&totem->window_h);
	totem->sidebar_w = totem->window_w
		- gtk_paned_get_position (item);
}

static void
totem_action_save_state (Totem *totem, const char *page_id)
{
	GKeyFile *keyfile;
	char *contents, *filename;

	if (totem->win == NULL)
		return;
	if (totem->window_w == 0
	    || totem->window_h == 0)
		return;

	keyfile = g_key_file_new ();
	g_key_file_set_integer (keyfile, "State",
				"window_w", totem->window_w);
	g_key_file_set_integer (keyfile, "State",
			"window_h", totem->window_h);
	g_key_file_set_boolean (keyfile, "State",
			"show_sidebar", totem_sidebar_is_visible (totem));
	g_key_file_set_boolean (keyfile, "State",
			"maximised", totem->maximised);
	g_key_file_set_integer (keyfile, "State",
			"sidebar_w", totem->sidebar_w);

	g_key_file_set_string (keyfile, "State",
			"sidebar_page", page_id);

	contents = g_key_file_to_data (keyfile, NULL, NULL);
	g_key_file_free (keyfile);
	filename = g_build_filename (totem_dot_dir (), "state.ini", NULL);
	g_file_set_contents (filename, contents, -1, NULL);

	g_free (filename);
	g_free (contents);
}

G_GNUC_NORETURN static void
totem_action_wait_force_exit (gpointer user_data)
{
	g_usleep (10 * G_USEC_PER_SEC);
	exit (1);
}

/**
 * totem_action_exit:
 * @totem: a #TotemObject
 *
 * Closes Totem.
 **/
void
totem_action_exit (Totem *totem)
{
	GdkDisplay *display = NULL;
	char *page_id;

	/* Exit forcefully if we can't do the shutdown in 10 seconds */
	g_thread_create ((GThreadFunc) totem_action_wait_force_exit,
			 NULL, FALSE, NULL);

	if (gtk_main_level () > 0)
		gtk_main_quit ();

	if (totem == NULL)
		exit (0);

	if (totem->win != NULL) {
		gtk_widget_hide (totem->win);
		display = gtk_widget_get_display (totem->win);
	}

	if (totem->prefs != NULL)
		gtk_widget_hide (totem->prefs);

	/* Save the page ID before we close the plugins, otherwise
	 * we'll never save it properly */
	page_id = totem_sidebar_get_current_page (totem);
	totem_object_plugins_shutdown ();

	if (display != NULL)
		gdk_display_sync (display);

	if (totem->bvw) {
		totem_action_save_size (totem);
		totem_save_position (totem);
		bacon_video_widget_close (totem->bvw);
	}

	if (totem->app != NULL)
		g_object_unref (totem->app);
	totem_action_save_state (totem, page_id);
	g_free (page_id);

	totem_sublang_exit (totem);
	totem_destroy_file_filters ();

	if (totem->gc)
		g_object_unref (G_OBJECT (totem->gc));

	if (totem->fs)
		g_object_unref (totem->fs);

	if (totem->win)
		gtk_widget_destroy (GTK_WIDGET (totem->win));

	g_object_unref (totem);

	exit (0);
}

static void
totem_action_menu_popup (Totem *totem, guint button)
{
	GtkWidget *menu;

	menu = gtk_ui_manager_get_widget (totem->ui_manager,
			"/totem-main-popup");
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
			button, gtk_get_current_event_time ());
	gtk_menu_shell_select_first (GTK_MENU_SHELL (menu), FALSE);
}

G_GNUC_NORETURN gboolean
main_window_destroy_cb (GtkWidget *widget, GdkEvent *event, Totem *totem)
{
	totem_action_exit (totem);
}

static void
play_pause_set_label (Totem *totem, TotemStates state)
{
	GtkAction *action;
	const char *id, *tip;
	GSList *l, *proxies;

	if (state == totem->state)
		return;

	switch (state)
	{
	case STATE_PLAYING:
		totem_statusbar_set_text (TOTEM_STATUSBAR (totem->statusbar),
				_("Playing"));
		id = GTK_STOCK_MEDIA_PAUSE;
		tip = N_("Pause");
		totem_playlist_set_playing (totem->playlist, TOTEM_PLAYLIST_STATUS_PLAYING);
		break;
	case STATE_PAUSED:
		totem_statusbar_set_text (TOTEM_STATUSBAR (totem->statusbar),
				_("Paused"));
		id = GTK_STOCK_MEDIA_PLAY;
		tip = N_("Play");
		totem_playlist_set_playing (totem->playlist, TOTEM_PLAYLIST_STATUS_PAUSED);
		break;
	case STATE_STOPPED:
		totem_statusbar_set_text (TOTEM_STATUSBAR (totem->statusbar),
				_("Stopped"));
		totem_statusbar_set_time_and_length
			(TOTEM_STATUSBAR (totem->statusbar), 0, 0);
		id = GTK_STOCK_MEDIA_PLAY;
		totem_playlist_set_playing (totem->playlist, TOTEM_PLAYLIST_STATUS_NONE);
		tip = N_("Play");
		break;
	default:
		g_assert_not_reached ();
		return;
	}

	action = gtk_action_group_get_action (totem->main_action_group, "play");
	g_object_set (G_OBJECT (action),
			"tooltip", _(tip),
			"stock-id", id, NULL);

	proxies = gtk_action_get_proxies (action);
	for (l = proxies; l != NULL; l = l->next) {
		atk_object_set_name (gtk_widget_get_accessible (l->data),
				_(tip));
	}

	totem->state = state;

	g_object_notify (G_OBJECT (totem), "playing");
}

void
totem_action_eject (Totem *totem)
{
	GMount *mount;

	mount = totem_get_mount_for_media (totem->mrl);
	if (mount == NULL)
		return;

	g_free (totem->mrl);
	totem->mrl = NULL;
	bacon_video_widget_close (totem->bvw);
	totem_file_closed (totem);

	/* The volume monitoring will take care of removing the items */
	g_mount_eject_with_operation (mount, G_MOUNT_UNMOUNT_NONE, NULL, NULL, NULL, NULL);
	g_object_unref (mount);
}

void
totem_action_show_properties (Totem *totem)
{
	if (totem_is_fullscreen (totem) == FALSE)
		totem_sidebar_set_current_page (totem, "properties", TRUE);
}

/**
 * totem_action_play:
 * @totem: a #TotemObject
 *
 * Plays the current stream. If Totem is already playing, it continues
 * to play. If the stream cannot be played, and error dialog is displayed.
 **/
void
totem_action_play (Totem *totem)
{
	GError *err = NULL;
	int retval;
	char *msg, *disp;

	if (totem->mrl == NULL)
		return;

	if (bacon_video_widget_is_playing (totem->bvw) != FALSE)
		return;

	retval = bacon_video_widget_play (totem->bvw,  &err);
	play_pause_set_label (totem, retval ? STATE_PLAYING : STATE_STOPPED);

	if (retval != FALSE)
		return;

	disp = totem_uri_escape_for_display (totem->mrl);
	msg = g_strdup_printf(_("Totem could not play '%s'."), disp);
	g_free (disp);

	totem_action_error (msg, err->message, totem);
	totem_action_stop (totem);
	g_free (msg);
	g_error_free (err);
}

static void
totem_action_seek (Totem *totem, double pos)
{
	GError *err = NULL;
	int retval;

	if (totem->mrl == NULL)
		return;
	if (bacon_video_widget_is_seekable (totem->bvw) == FALSE)
		return;

	retval = bacon_video_widget_seek (totem->bvw, pos, &err);

	if (retval == FALSE)
	{
		char *msg, *disp;

		disp = totem_uri_escape_for_display (totem->mrl);
		msg = g_strdup_printf(_("Totem could not play '%s'."), disp);
		g_free (disp);

		reset_seek_status (totem);

		totem_action_error (msg, err->message, totem);
		g_free (msg);
		g_error_free (err);
	}
}

/**
 * totem_action_set_mrl_and_play:
 * @totem: a #TotemObject
 * @mrl: the MRL to play
 * @subtitle: a subtitle file to load, or %NULL
 *
 * Loads the specified @mrl and plays it, if possible.
 * Calls totem_action_set_mrl() then totem_action_play().
 * For more information, see the documentation for totem_action_set_mrl_with_warning().
 **/
void
totem_action_set_mrl_and_play (Totem *totem, const char *mrl, const char *subtitle)
{
	if (totem_action_set_mrl (totem, mrl, subtitle) != FALSE)
		totem_action_play (totem);
}

static gboolean
totem_action_open_dialog (Totem *totem, const char *path, gboolean play)
{
	GSList *filenames;
	gboolean playlist_modified;

	filenames = totem_add_files (GTK_WINDOW (totem->win), path);

	if (filenames == NULL)
		return FALSE;

	playlist_modified = totem_action_open_files_list (totem,
			filenames);

	if (playlist_modified == FALSE) {
		g_slist_foreach (filenames, (GFunc) g_free, NULL);
		g_slist_free (filenames);
		return FALSE;
	}

	g_slist_foreach (filenames, (GFunc) g_free, NULL);
	g_slist_free (filenames);

	if (play != FALSE) {
		char *mrl, *subtitle;

		mrl = totem_playlist_get_current_mrl (totem->playlist, &subtitle);
		totem_action_set_mrl_and_play (totem, mrl, subtitle);
		g_free (mrl);
		g_free (subtitle);
	}

	return TRUE;
}

static gboolean
totem_action_load_media (Totem *totem, TotemDiscMediaType type, const char *device)
{
	char **mrls;
	GError *error = NULL;
	const char *link, *link_text, *secondary;
	gboolean retval;

	mrls = bacon_video_widget_get_mrls (totem->bvw, type, device, &error);
	if (mrls == NULL) {
		char *msg;

		/* No errors? Weird */
		if (error == NULL) {
			msg = g_strdup_printf (_("Totem could not play this media (%s) although a plugin is present to handle it."), _(totem_cd_get_human_readable_name (type)));
			totem_action_error (msg, _("You might want to check that a disc is present in the drive and that it is correctly configured."), totem);
			g_free (msg);
			return FALSE;
		}

		/* No plugin for the media type */
		if (g_error_matches (error, BVW_ERROR, BVW_ERROR_NO_PLUGIN_FOR_FILE) != FALSE) {
			link = "http://projects.mate.org/totem/#codecs";
			link_text = _("More information about media plugins");
			secondary = _("Please install the necessary plugins and restart Totem to be able to play this media.");
			if (type == MEDIA_TYPE_DVD || type == MEDIA_TYPE_VCD)
				msg = g_strdup_printf (_("Totem cannot play this type of media (%s) because it does not have the appropriate plugins to be able to read from the disc."), _(totem_cd_get_human_readable_name (type)));
			else
				msg = g_strdup_printf (_("Totem cannot play this type of media (%s) because you do not have the appropriate plugins to handle it."), _(totem_cd_get_human_readable_name (type)));
		/* Unsupported type (ie. CDDA) */
		} else if (g_error_matches (error, BVW_ERROR, BVW_ERROR_INVALID_LOCATION) != FALSE) {
			msg = g_strdup_printf(_("Totem cannot play this type of media (%s) because it is not supported."), _(totem_cd_get_human_readable_name (type)));
			totem_action_error (msg, _("Please insert another disc to play back."), totem);
			g_free (msg);
			return FALSE;
		} else {
			g_assert_not_reached ();
		}

		totem_interface_error_with_link (msg, secondary, link, link_text, GTK_WINDOW (totem->win), totem);
		g_free (msg);
		return FALSE;
	}

	retval = totem_action_open_files (totem, mrls);
	g_strfreev (mrls);

	return retval;
}

static gboolean
totem_action_load_media_device (Totem *totem, const char *device)
{
	TotemDiscMediaType type;
	GError *error = NULL;
	char *device_path, *url;
	gboolean retval;

	if (g_str_has_prefix (device, "file://") != FALSE)
		device_path = g_filename_from_uri (device, NULL, NULL);
	else
		device_path = g_strdup (device);

	type = totem_cd_detect_type_with_url (device_path, &url, &error);

	switch (type) {
		case MEDIA_TYPE_ERROR:
			totem_action_error (_("Totem was not able to play this disc."),
					    error ? error->message : _("No reason."),
					    totem);
			retval = FALSE;
			break;
		case MEDIA_TYPE_DATA:
			/* Set default location to the mountpoint of
			 * this device */
			retval = totem_action_open_dialog (totem, url, FALSE);
			break;
		case MEDIA_TYPE_DVD:
		case MEDIA_TYPE_VCD:
			retval = totem_action_load_media (totem, type, device_path);
			break;
		case MEDIA_TYPE_CDDA:
			totem_action_error (_("Totem does not support playback of Audio CDs"),
					    _("Please consider using a music player or a CD extractor to play this CD"),
					    totem);
			retval = FALSE;
			break;
		default:
			g_assert_not_reached ();
	}

	g_free (url);
	g_free (device_path);

	return retval;
}

/**
 * totem_action_play_media_device:
 * @totem: a #TotemObject
 * @device: the media device's path
 *
 * Attempts to play the media device (for example, a DVD drive or CD drive)
 * with the given @device path by first adding it to the playlist, then
 * playing it.
 *
 * An error dialog will be displayed if Totem cannot read or play what's on
 * the media device.
 **/
void
totem_action_play_media_device (Totem *totem, const char *device)
{
	char *mrl;

	if (totem_action_load_media_device (totem, device) != FALSE) {
		mrl = totem_playlist_get_current_mrl (totem->playlist, NULL);
		totem_action_set_mrl_and_play (totem, mrl, NULL);
		g_free (mrl);
	}
}

/**
 * totem_action_play_media:
 * @totem: a #TotemObject
 * @type: the type of disc media
 * @device: the media's device path
 *
 * Attempts to play the media found on @device (for example, a DVD in a drive or a DVB
 * tuner) by first adding it to the playlist, then playing it.
 *
 * An error dialog will be displayed if Totem cannot support media of @type.
 **/
void
totem_action_play_media (Totem *totem, TotemDiscMediaType type, const char *device)
{
	char *mrl;

	if (totem_action_load_media (totem, type, device) != FALSE) {
		mrl = totem_playlist_get_current_mrl (totem->playlist, NULL);
		totem_action_set_mrl_and_play (totem, mrl, NULL);
		g_free (mrl);
	}
}

/**
 * totem_action_stop:
 * @totem: a #TotemObject
 *
 * Stops the current stream.
 **/
void
totem_action_stop (Totem *totem)
{
	bacon_video_widget_stop (totem->bvw);
	play_pause_set_label (totem, STATE_STOPPED);
}

/**
 * totem_action_play_pause:
 * @totem: a #TotemObject
 *
 * Gets the current MRL from the playlist and attempts to play it.
 * If the stream is already playing, playback is paused.
 **/
void
totem_action_play_pause (Totem *totem)
{
	if (totem->mrl == NULL) {
		char *mrl, *subtitle;

		/* Try to pull an mrl from the playlist */
		mrl = totem_playlist_get_current_mrl (totem->playlist, &subtitle);
		if (mrl == NULL) {
			play_pause_set_label (totem, STATE_STOPPED);
			return;
		} else {
			totem_action_set_mrl_and_play (totem, mrl, subtitle);
			g_free (mrl);
			g_free (subtitle);
			return;
		}
	}

	if (bacon_video_widget_is_playing (totem->bvw) == FALSE) {
		bacon_video_widget_play (totem->bvw, NULL);
		play_pause_set_label (totem, STATE_PLAYING);
	} else {
		bacon_video_widget_pause (totem->bvw);
		play_pause_set_label (totem, STATE_PAUSED);

		/* Save the stream position */
		totem_save_position (totem);
	}
}

/**
 * totem_action_pause:
 * @totem: a #TotemObject
 *
 * Pauses the current stream. If Totem is already paused, it continues
 * to be paused.
 **/
void
totem_action_pause (Totem *totem)
{
	if (bacon_video_widget_is_playing (totem->bvw) != FALSE) {
		bacon_video_widget_pause (totem->bvw);
		play_pause_set_label (totem, STATE_PAUSED);

		/* Save the stream position */
		totem_save_position (totem);
	}
}

gboolean
window_state_event_cb (GtkWidget *window, GdkEventWindowState *event,
		       Totem *totem)
{
	if (event->changed_mask & GDK_WINDOW_STATE_MAXIMIZED) {
		totem->maximised = (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) != 0;
                gtk_statusbar_set_has_resize_grip (GTK_STATUSBAR (totem->statusbar),
                                                   !totem->maximised);
		totem_action_set_sensitivity ("zoom-1-2", !totem->maximised);
		totem_action_set_sensitivity ("zoom-1-1", !totem->maximised);
		totem_action_set_sensitivity ("zoom-2-1", !totem->maximised);
		return FALSE;
	}

	if ((event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN) == 0)
		return FALSE;

	if (event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN) {
		if (totem->controls_visibility != TOTEM_CONTROLS_UNDEFINED)
			totem_action_save_size (totem);
		totem_fullscreen_set_fullscreen (totem->fs, TRUE);

		totem->controls_visibility = TOTEM_CONTROLS_FULLSCREEN;
		show_controls (totem, FALSE);
		totem_action_set_sensitivity ("fullscreen", FALSE);
	} else {
		GtkAction *action;

		totem_fullscreen_set_fullscreen (totem->fs, FALSE);

		action = gtk_action_group_get_action (totem->main_action_group,
				"show-controls");

		if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)))
			totem->controls_visibility = TOTEM_CONTROLS_VISIBLE;
		else
			totem->controls_visibility = TOTEM_CONTROLS_HIDDEN;

		show_controls (totem, TRUE);
		totem_action_set_sensitivity ("fullscreen", TRUE);
	}

	g_object_notify (G_OBJECT (totem), "fullscreen");

	return FALSE;
}

/**
 * totem_action_fullscreen_toggle:
 * @totem: a #TotemObject
 *
 * Toggles Totem's fullscreen state; if Totem is fullscreened, calling
 * this makes it unfullscreened and vice-versa.
 **/
void
totem_action_fullscreen_toggle (Totem *totem)
{
	if (totem_is_fullscreen (totem) != FALSE)
		gtk_window_unfullscreen (GTK_WINDOW (totem->win));
	else
		gtk_window_fullscreen (GTK_WINDOW (totem->win));
}

/**
 * totem_action_fullscreen:
 * @totem: a #TotemObject
 * @state: %TRUE if Totem should be fullscreened
 *
 * Sets Totem's fullscreen state according to @state.
 **/
void
totem_action_fullscreen (Totem *totem, gboolean state)
{
	if (totem_is_fullscreen (totem) == state)
		return;

	totem_action_fullscreen_toggle (totem);
}

void
fs_exit1_activate_cb (GtkButton *button, Totem *totem)
{
	totem_action_fullscreen (totem, FALSE);
}

void
totem_action_open (Totem *totem)
{
	totem_action_open_dialog (totem, NULL, TRUE);
}

static void
totem_open_location_response_cb (GtkDialog *dialog, gint response, Totem *totem)
{
	char *uri;

	if (response != GTK_RESPONSE_OK) {
		gtk_widget_destroy (GTK_WIDGET (totem->open_location));
		return;
	}

	gtk_widget_hide (GTK_WIDGET (dialog));

	/* Open the specified URI */
	uri = totem_open_location_get_uri (totem->open_location);

	if (uri != NULL)
	{
		char *mrl, *subtitle;
		const char *filenames[2];

		filenames[0] = uri;
		filenames[1] = NULL;
		totem_action_open_files (totem, (char **) filenames);

		mrl = totem_playlist_get_current_mrl (totem->playlist, &subtitle);
		totem_action_set_mrl_and_play (totem, mrl, subtitle);
		g_free (mrl);
		g_free (subtitle);
	}
 	g_free (uri);

	gtk_widget_destroy (GTK_WIDGET (totem->open_location));
}

void
totem_action_open_location (Totem *totem)
{
	if (totem->open_location != NULL) {
		gtk_window_present (GTK_WINDOW (totem->open_location));
		return;
	}

	totem->open_location = TOTEM_OPEN_LOCATION (totem_open_location_new ());

	g_signal_connect (G_OBJECT (totem->open_location), "delete-event",
			G_CALLBACK (gtk_widget_destroy), NULL);
	g_signal_connect (G_OBJECT (totem->open_location), "response",
			G_CALLBACK (totem_open_location_response_cb), totem);
	g_object_add_weak_pointer (G_OBJECT (totem->open_location), (gpointer *)&(totem->open_location));

	gtk_window_set_transient_for (GTK_WINDOW (totem->open_location),
			GTK_WINDOW (totem->win));
	gtk_widget_show (GTK_WIDGET (totem->open_location));
}

static char *
totem_get_nice_name_for_stream (Totem *totem)
{
	GValue title_value = { 0, };
	GValue album_value = { 0, };
	GValue artist_value = { 0, };
	GValue value = { 0, };
	char *retval;
	int tracknum;

	bacon_video_widget_get_metadata (totem->bvw, BVW_INFO_TITLE, &title_value);
	bacon_video_widget_get_metadata (totem->bvw, BVW_INFO_ARTIST, &artist_value);
	bacon_video_widget_get_metadata (totem->bvw, BVW_INFO_ALBUM, &album_value);
	bacon_video_widget_get_metadata (totem->bvw,
					 BVW_INFO_TRACK_NUMBER,
					 &value);

	tracknum = g_value_get_int (&value);
	g_value_unset (&value);

	totem_metadata_updated (totem,
				g_value_get_string (&artist_value),
				g_value_get_string (&title_value),
				g_value_get_string (&album_value),
				tracknum);

	if (g_value_get_string (&title_value) == NULL) {
		retval = NULL;
		goto bail;
	}
	if (g_value_get_string (&artist_value) == NULL) {
		retval = g_value_dup_string (&title_value);
		goto bail;
	}

	if (tracknum != 0) {
		retval = g_strdup_printf ("%02d. %s - %s",
					  tracknum,
					  g_value_get_string (&artist_value),
					  g_value_get_string (&title_value));
	} else {
		retval = g_strdup_printf ("%s - %s",
					  g_value_get_string (&artist_value),
					  g_value_get_string (&title_value));
	}

bail:
	g_value_unset (&album_value);
	g_value_unset (&artist_value);
	g_value_unset (&title_value);

	return retval;
}

static void
update_mrl_label (Totem *totem, const char *name)
{
	if (name != NULL)
	{
		/* Update the mrl label */
		totem_fullscreen_set_title (totem->fs, name);

		/* Title */
		gtk_window_set_title (GTK_WINDOW (totem->win), name);
	} else {
		totem_statusbar_set_time_and_length (TOTEM_STATUSBAR
				(totem->statusbar), 0, 0);
		totem_statusbar_set_text (TOTEM_STATUSBAR (totem->statusbar),
				_("Stopped"));

		g_object_notify (G_OBJECT (totem), "stream-length");

		/* Update the mrl label */
		totem_fullscreen_set_title (totem->fs, NULL);

		/* Title */
		gtk_window_set_title (GTK_WINDOW (totem->win), _("Movie Player"));
	}
}

/**
 * totem_action_set_mrl_with_warning:
 * @totem: a #TotemObject
 * @mrl: the MRL to play
 * @subtitle: a subtitle file to load, or %NULL
 * @warn: %TRUE if error dialogs should be displayed
 *
 * Loads the specified @mrl and optionally the specified subtitle
 * file. If @subtitle is %NULL Totem will attempt to auto-locate
 * any subtitle files for @mrl.
 *
 * If a stream is already playing, it will be stopped and closed.
 *
 * If any errors are encountered, error dialogs will only be displayed
 * if @warn is %TRUE.
 *
 * Return value: %TRUE on success
 **/
gboolean
totem_action_set_mrl_with_warning (Totem *totem,
				   const char *mrl, 
				   const char *subtitle,
				   gboolean warn)
{
	gboolean retval = TRUE;

	if (totem->mrl != NULL) {
		totem_save_position (totem);
		g_free (totem->mrl);
		totem->mrl = NULL;
		bacon_video_widget_close (totem->bvw);
		totem_file_closed (totem);
		play_pause_set_label (totem, STATE_STOPPED);
		update_fill (totem, -1.0);
	}

	if (mrl == NULL) {
		retval = FALSE;

		play_pause_set_label (totem, STATE_STOPPED);

		/* Play/Pause */
		totem_action_set_sensitivity ("play", FALSE);

		/* Volume */
		totem_main_set_sensitivity ("tmw_volume_button", FALSE);
		totem_action_set_sensitivity ("volume-up", FALSE);
		totem_action_set_sensitivity ("volume-down", FALSE);
		totem->volume_sensitive = FALSE;

		/* Control popup */
		totem_fullscreen_set_can_set_volume (totem->fs, FALSE);
		totem_fullscreen_set_seekable (totem->fs, FALSE);
		totem_action_set_sensitivity ("next-chapter", FALSE);
		totem_action_set_sensitivity ("previous-chapter", FALSE);

		/* Clear the playlist */
		totem_action_set_sensitivity ("clear-playlist", FALSE);

		/* Subtitle selection */
		totem_action_set_sensitivity ("select-subtitle", FALSE);

		/* Set the logo */
		bacon_video_widget_set_logo_mode (totem->bvw, TRUE);
		update_mrl_label (totem, NULL);

		/* Unset the drag */
		gtk_drag_source_unset (GTK_WIDGET (totem->bvw));
	} else {
		gboolean caps;
		gdouble volume;
		char *autoload_sub = NULL;
		GError *err = NULL;

		bacon_video_widget_set_logo_mode (totem->bvw, FALSE);

		if (subtitle == NULL && totem->autoload_subs != FALSE)
			autoload_sub = totem_uri_get_subtitle_uri (mrl);

		/* HACK: Bad bad Apple */
		if (g_str_has_prefix (mrl, "http://movies.apple.com")
				|| g_str_has_prefix (mrl, "http://trailers.apple.com"))
			bacon_video_widget_set_user_agent (totem->bvw, "Quicktime/7.2.0");
		else
			bacon_video_widget_set_user_agent (totem->bvw, NULL);

		totem_gdk_window_set_waiting_cursor (gtk_widget_get_window (totem->win));
		totem_try_restore_position (totem, mrl);
		retval = bacon_video_widget_open (totem->bvw, mrl, subtitle ? subtitle : autoload_sub, &err);
		g_free (autoload_sub);
		gdk_window_set_cursor (gtk_widget_get_window (totem->win), NULL);
		totem->mrl = g_strdup (mrl);

		/* Play/Pause */
		totem_action_set_sensitivity ("play", TRUE);

		/* Volume */
		caps = bacon_video_widget_can_set_volume (totem->bvw);
		totem_main_set_sensitivity ("tmw_volume_button", caps);
		totem_fullscreen_set_can_set_volume (totem->fs, caps);
		volume = bacon_video_widget_get_volume (totem->bvw);
		totem_action_set_sensitivity ("volume-up", caps && volume < (1.0 - VOLUME_EPSILON));
		totem_action_set_sensitivity ("volume-down", caps && volume > VOLUME_EPSILON);
		totem->volume_sensitive = caps;

		/* Clear the playlist */
		totem_action_set_sensitivity ("clear-playlist", retval);

		/* Subtitle selection */
		totem_action_set_sensitivity ("select-subtitle", !totem_is_special_mrl (mrl) && retval);
	
		/* Set the playlist */
		play_pause_set_label (totem, retval ? STATE_PAUSED : STATE_STOPPED);

		if (retval == FALSE && warn != FALSE) {
			char *msg, *disp;

			disp = totem_uri_escape_for_display (totem->mrl);
			msg = g_strdup_printf(_("Totem could not play '%s'."), disp);
			g_free (disp);
			if (err && err->message) {
				totem_action_error (msg, err->message, totem);
			}
			else {
				totem_action_error (msg, _("No error message"), totem);
			}
			g_free (msg);
		}

		if (retval == FALSE) {
			if (err)
				g_error_free (err);
			g_free (totem->mrl);
			totem->mrl = NULL;
			bacon_video_widget_set_logo_mode (totem->bvw, TRUE);
		} else {
			char *display_name;
			/* cast is to shut gcc up */
			const GtkTargetEntry source_table[] = {
				{ (gchar*) "text/uri-list", 0, 0 }
			};

			totem_file_opened (totem, totem->mrl);

			/* Set the drag source */
			gtk_drag_source_set (GTK_WIDGET (totem->bvw),
					     GDK_BUTTON1_MASK | GDK_BUTTON3_MASK,
					     source_table, G_N_ELEMENTS (source_table),
					     GDK_ACTION_COPY);

			display_name = totem_playlist_get_current_title (totem->playlist, NULL);
			totem_action_add_recent (totem, totem->mrl, display_name);
			g_free (display_name);
		}
	}
	update_buttons (totem);
	update_media_menu_items (totem);

	return retval;
}

/**
 * totem_action_set_mrl:
 * @totem: a #TotemObject
 * @mrl: the MRL to load
 * @subtitle: a subtitle file to load, or %NULL
 *
 * Calls totem_action_set_mrl_with_warning() with warnings enabled.
 * For more information, see the documentation for totem_action_set_mrl_with_warning().
 *
 * Return value: %TRUE on success
 **/
gboolean
totem_action_set_mrl (Totem *totem, const char *mrl, const char *subtitle)
{
	return totem_action_set_mrl_with_warning (totem, mrl, subtitle, TRUE);
}

static gboolean
totem_time_within_seconds (Totem *totem)
{
	gint64 _time;

	_time = bacon_video_widget_get_current_time (totem->bvw);

	return (_time < REWIND_OR_PREVIOUS);
}

static void
totem_action_direction (Totem *totem, TotemPlaylistDirection dir)
{
	if (totem_playing_dvd (totem->mrl) == FALSE &&
		totem_playlist_has_direction (totem->playlist, dir) == FALSE
		&& totem_playlist_get_repeat (totem->playlist) == FALSE)
		return;

	if (totem_playing_dvd (totem->mrl) != FALSE)
	{
		bacon_video_widget_dvd_event (totem->bvw,
				dir == TOTEM_PLAYLIST_DIRECTION_NEXT ?
				BVW_DVD_NEXT_CHAPTER :
				BVW_DVD_PREV_CHAPTER);
		return;
	}
	
	if (dir == TOTEM_PLAYLIST_DIRECTION_NEXT
			|| bacon_video_widget_is_seekable (totem->bvw) == FALSE
			|| totem_time_within_seconds (totem) != FALSE)
	{
		char *mrl, *subtitle;

		totem_playlist_set_direction (totem->playlist, dir);
		mrl = totem_playlist_get_current_mrl (totem->playlist, &subtitle);
		totem_action_set_mrl_and_play (totem, mrl, subtitle);

		g_free (subtitle);
		g_free (mrl);
	} else {
		totem_action_seek (totem, 0);
	}
}

/**
 * totem_action_previous:
 * @totem: a #TotemObject
 *
 * If a DVD is being played, goes to the previous chapter. If a normal stream
 * is being played, goes to the start of the stream if possible. If seeking is
 * not possible, plays the previous entry in the playlist.
 **/
void
totem_action_previous (Totem *totem)
{
	totem_action_direction (totem, TOTEM_PLAYLIST_DIRECTION_PREVIOUS);
}

/**
 * totem_action_next:
 * @totem: a #TotemObject
 *
 * If a DVD is being played, goes to the next chapter. If a normal stream
 * is being played, plays the next entry in the playlist.
 **/
void
totem_action_next (Totem *totem)
{
	totem_action_direction (totem, TOTEM_PLAYLIST_DIRECTION_NEXT);
}

static void
totem_seek_time_rel (Totem *totem, gint64 _time, gboolean relative, gboolean accurate)
{
	GError *err = NULL;
	gint64 sec;

	if (totem->mrl == NULL)
		return;
	if (bacon_video_widget_is_seekable (totem->bvw) == FALSE)
		return;

	totem_statusbar_set_seeking (TOTEM_STATUSBAR (totem->statusbar), TRUE);
	totem_time_label_set_seeking (TOTEM_TIME_LABEL (totem->fs->time_label), TRUE);

	if (relative != FALSE) {
		gint64 oldmsec;
		oldmsec = bacon_video_widget_get_current_time (totem->bvw);
		sec = MAX (0, oldmsec + _time);
	} else {
		sec = _time;
	}

	bacon_video_widget_seek_time (totem->bvw, sec, accurate, &err);

	totem_statusbar_set_seeking (TOTEM_STATUSBAR (totem->statusbar), FALSE);
	totem_time_label_set_seeking (TOTEM_TIME_LABEL (totem->fs->time_label), FALSE);

	if (err != NULL)
	{
		char *msg, *disp;

		disp = totem_uri_escape_for_display (totem->mrl);
		msg = g_strdup_printf(_("Totem could not play '%s'."), disp);
		g_free (disp);

		totem_action_stop (totem);
		totem_action_error (msg, err->message, totem);
		g_free (msg);
		g_error_free (err);
	}
}

/**
 * totem_action_seek_relative:
 * @totem: a #TotemObject
 * @offset: the time offset to seek to
 * @accurate: whether to use accurate seek, an accurate seek might be slower for some formats (see GStreamer docs)
 *
 * Seeks to an @offset from the current position in the stream,
 * or displays an error dialog if that's not possible.
 **/
void
totem_action_seek_relative (Totem *totem, gint64 offset, gboolean accurate)
{
	totem_seek_time_rel (totem, offset, TRUE, accurate);
}

/**
 * totem_action_seek_time:
 * @totem: a #TotemObject
 * @msec: the time to seek to
 * @accurate: whether to use accurate seek, an accurate seek might be slower for some formats (see GStreamer docs)
 *
 * Seeks to an absolute time in the stream, or displays an
 * error dialog if that's not possible.
 **/
void
totem_action_seek_time (Totem *totem, gint64 msec, gboolean accurate)
{
	totem_seek_time_rel (totem, msec, FALSE, accurate);
}

static void
totem_action_zoom (Totem *totem, double zoom)
{
	GtkAction *action;
	gboolean zoom_reset, zoom_in, zoom_out;

	if (zoom == ZOOM_ENABLE)
		zoom = bacon_video_widget_get_zoom (totem->bvw);

	if (zoom == ZOOM_DISABLE) {
		zoom_reset = zoom_in = zoom_out = FALSE;
	} else if (zoom < ZOOM_LOWER || zoom > ZOOM_UPPER) {
		return;
	} else {
		bacon_video_widget_set_zoom (totem->bvw, zoom);
		zoom_reset = (zoom != ZOOM_RESET);
		zoom_out = zoom != ZOOM_LOWER;
		zoom_in = zoom != ZOOM_UPPER;
	}

	action = gtk_action_group_get_action (totem->zoom_action_group,
			"zoom-in");
	gtk_action_set_sensitive (action, zoom_in);

	action = gtk_action_group_get_action (totem->zoom_action_group,
			"zoom-out");
	gtk_action_set_sensitive (action, zoom_out);

	action = gtk_action_group_get_action (totem->zoom_action_group,
			"zoom-reset");
	gtk_action_set_sensitive (action, zoom_reset);
}

void
totem_action_zoom_relative (Totem *totem, double off_pct)
{
	double zoom;

	zoom = bacon_video_widget_get_zoom (totem->bvw);
	totem_action_zoom (totem, zoom + off_pct);
}

void
totem_action_zoom_reset (Totem *totem)
{
	totem_action_zoom (totem, ZOOM_RESET);
}

/**
 * totem_get_volume:
 * @totem: a #TotemObject
 *
 * Gets the current volume level, as a value between %0.0 and %1.0.
 *
 * Return value: the volume level
 **/
double
totem_get_volume (Totem *totem)
{
	return bacon_video_widget_get_volume (totem->bvw);
}

/**
 * totem_action_volume:
 * @totem: a #TotemObject
 * @volume: the new absolute volume value
 *
 * Sets the volume, with %1.0 being the maximum, and %0.0 being the minimum level.
 **/
void
totem_action_volume (Totem *totem, double volume)
{
	if (bacon_video_widget_can_set_volume (totem->bvw) == FALSE)
		return;

	bacon_video_widget_set_volume (totem->bvw, volume);
}

/**
 * totem_action_volume_relative:
 * @totem: a #TotemObject
 * @off_pct: the value by which to increase or decrease the volume
 *
 * Sets the volume relative to its current level, with %1.0 being the
 * maximum, and %0.0 being the minimum level.
 **/
void
totem_action_volume_relative (Totem *totem, double off_pct)
{
	double vol;

	if (bacon_video_widget_can_set_volume (totem->bvw) == FALSE)
		return;
	if (totem->muted != FALSE)
		totem_action_volume_toggle_mute (totem);

	vol = bacon_video_widget_get_volume (totem->bvw);
	bacon_video_widget_set_volume (totem->bvw, vol + off_pct);
}

/**
 * totem_action_volume_toggle_mute:
 * @totem: a #TotemObject
 *
 * Toggles the mute status.
 **/
void
totem_action_volume_toggle_mute (Totem *totem)
{
	if (totem->muted == FALSE) {
		totem->muted = TRUE;
		totem->prev_volume = bacon_video_widget_get_volume (totem->bvw);
		bacon_video_widget_set_volume (totem->bvw, 0.0);
	} else {
		totem->muted = FALSE;
		bacon_video_widget_set_volume (totem->bvw, totem->prev_volume);
	}
}

/**
 * totem_action_toggle_aspect_ratio:
 * @totem: a #TotemObject
 *
 * Toggles the aspect ratio selected in the menu to the
 * next one in the list.
 **/
void
totem_action_toggle_aspect_ratio (Totem *totem)
{
	GtkAction *action;
	int tmp;

	tmp = totem_action_get_aspect_ratio (totem);
	tmp++;
	if (tmp > BVW_RATIO_DVB)
		tmp = BVW_RATIO_AUTO;

	action = gtk_action_group_get_action (totem->main_action_group, "aspect-ratio-auto");
	gtk_radio_action_set_current_value (GTK_RADIO_ACTION (action), tmp);
}

/**
 * totem_action_set_aspect_ratio:
 * @totem: a #TotemObject
 * @ratio: the aspect ratio to use
 *
 * Sets the aspect ratio selected in the menu to @ratio,
 * as defined in #BvwAspectRatio.
 **/
void
totem_action_set_aspect_ratio (Totem *totem, int ratio)
{
	bacon_video_widget_set_aspect_ratio (totem->bvw, ratio);
}

/**
 * totem_action_get_aspect_ratio:
 * @totem: a #TotemObject
 *
 * Gets the current aspect ratio as defined in #BvwAspectRatio.
 *
 * Return value: the current aspect ratio
 **/
int
totem_action_get_aspect_ratio (Totem *totem)
{
	return (bacon_video_widget_get_aspect_ratio (totem->bvw));
}

/**
 * totem_action_set_scale_ratio:
 * @totem: a #TotemObject
 * @ratio: the scale ratio to use
 *
 * Sets the video scale ratio, as a float where, for example,
 * 1.0 is 1:1 and 2.0 is 2:1.
 **/
void
totem_action_set_scale_ratio (Totem *totem, gfloat ratio)
{
	bacon_video_widget_set_scale_ratio (totem->bvw, ratio);
}

void
totem_action_show_help (Totem *totem)
{
	GError *error = NULL;

	if (gtk_show_uri (gtk_widget_get_screen (totem->win), "ghelp:totem", gtk_get_current_event_time (), &error) == FALSE) {
		totem_action_error (_("Totem could not display the help contents."), error->message, totem);
		g_error_free (error);
	}
}

typedef struct {
	gint add_mrl_complete;
	Totem *totem;
} DropFilesData;

/* This is called in the main thread */
static void
totem_action_drop_files_finished (TotemPlaylist *playlist, GAsyncResult *result, DropFilesData *data)
{
	/* When add_mrl_complete reaches 0, this is the last callback to occur and we can safely reconnect the playlist's changed signal (which was
	 * disconnected below in totem_action_drop_files(). We can also free the data struct and generally clean up. */
	if (g_atomic_int_dec_and_test (&(data->add_mrl_complete)) == TRUE) {
		char *mrl, *subtitle;

		/* Reconnect the signal */
		g_signal_connect (G_OBJECT (playlist), "changed", G_CALLBACK (playlist_changed_cb), data->totem);
		mrl = totem_playlist_get_current_mrl (playlist, &subtitle);
		totem_action_set_mrl_and_play (data->totem, mrl, subtitle);
		g_free (mrl);
		g_free (subtitle);

		/* Free the data struct */
		g_object_unref (data->totem);
		g_slice_free (DropFilesData, data);
	}
}

static gboolean
totem_action_drop_files (Totem *totem, GtkSelectionData *data,
		int drop_type, gboolean empty_pl)
{
	char **list;
	guint i, len;
	DropFilesData *drop_files_data = NULL /* shut up gcc */;
	GList *p, *file_list;
	gboolean cleared = FALSE;

	list = g_uri_list_extract_uris ((const char *) gtk_selection_data_get_data (data));
	file_list = NULL;

	for (i = 0; list[i] != NULL; i++) {
		char *filename;

		if (list[i] == NULL)
			continue;

		filename = totem_create_full_path (list[i]);
		file_list = g_list_prepend (file_list,
					    filename ? filename : g_strdup (list[i]));
	}
	g_strfreev (list);

	if (file_list == NULL)
		return FALSE;

	if (drop_type != 1)
		file_list = g_list_sort (file_list, (GCompareFunc) strcmp);
	else
		file_list = g_list_reverse (file_list);

	/* How many files? Check whether those could be subtitles */
	len = g_list_length (file_list);
	if (len == 1 || (len == 2 && drop_type == 1)) {
		if (totem_uri_is_subtitle (file_list->data) != FALSE) {
			totem_playlist_set_current_subtitle (totem->playlist, file_list->data);
			goto bail;
		}
	}

	if (empty_pl != FALSE) {
		/* The function that calls us knows better if we should be doing something with the changed playlist... */
		g_signal_handlers_disconnect_by_func (G_OBJECT (totem->playlist), playlist_changed_cb, totem);
		totem_playlist_clear (totem->playlist);
		cleared = TRUE;

		/* Allocate some shared memory to count how many add_mrl operations have completed (see the comment below).
		 * It's freed in totem_action_drop_files_cb() once all add_mrl operations have finished. */
		drop_files_data = g_slice_new (DropFilesData);
		drop_files_data->add_mrl_complete = len;
		drop_files_data->totem = g_object_ref (totem);
	}

	/* Add each MRL to the playlist asynchronously */
	for (p = file_list; p != NULL; p = p->next) {
		const char *filename;
		char *title;

		filename = p->data;
		title = NULL;

		/* Super _NETSCAPE_URL trick */
		if (drop_type == 1) {
			p = p->next;
			if (p != NULL) {
				if (g_str_has_prefix (p->data, "File:") != FALSE)
					title = (char *)p->data + 5;
				else
					title = p->data;
			}
		}

		/* Add the MRL to the playlist. We need to reconnect playlist's "changed" signal once all of the add_mrl operations have completed,
		 * so we use a piece of allocated memory shared between the async operations to count how many have completed.
		 * If we haven't cleared the playlist, there's no need to do this. */
		if (cleared == TRUE) {
			totem_playlist_add_mrl (totem->playlist, filename, title, TRUE, NULL,
			                        (GAsyncReadyCallback) totem_action_drop_files_finished, drop_files_data);
		} else {
			totem_playlist_add_mrl (totem->playlist, filename, title, TRUE, NULL, NULL, NULL);
		}
	}

bail:
	g_list_foreach (file_list, (GFunc) g_free, NULL);
	g_list_free (file_list);

	return TRUE;
}

static void
drop_video_cb (GtkWidget     *widget,
	 GdkDragContext     *context,
	 gint                x,
	 gint                y,
	 GtkSelectionData   *data,
	 guint               info,
	 guint               _time,
	 Totem              *totem)
{
	GtkWidget *source_widget;
	gboolean empty_pl;
	GdkDragAction action = gdk_drag_context_get_selected_action (context);

	source_widget = gtk_drag_get_source_widget (context);

	/* Drop of video on itself */
	if (source_widget && widget == source_widget && action == GDK_ACTION_MOVE) {
		gtk_drag_finish (context, FALSE, FALSE, _time);
		return;
	}

	if (action == GDK_ACTION_ASK) {
		action = totem_drag_ask (totem_get_playlist_length (totem) > 0);
		gdk_drag_status (context, action, GDK_CURRENT_TIME);
	}

	/* User selected cancel */
	if (action == GDK_ACTION_DEFAULT) {
		gtk_drag_finish (context, FALSE, FALSE, _time);
		return;
	}

	empty_pl = (action == GDK_ACTION_MOVE);
	totem_action_drop_files (totem, data, info, empty_pl);
	gtk_drag_finish (context, TRUE, FALSE, _time);
	return;
}

static void
drag_motion_video_cb (GtkWidget      *widget,
                      GdkDragContext *context,
                      gint            x,
                      gint            y,
                      guint           _time,
                      Totem          *totem)
{
	GdkModifierType mask;

	gdk_window_get_pointer (gtk_widget_get_window (widget), NULL, NULL, &mask);
	if (mask & GDK_CONTROL_MASK) {
		gdk_drag_status (context, GDK_ACTION_COPY, _time);
	} else if (mask & GDK_MOD1_MASK || gdk_drag_context_get_suggested_action (context) == GDK_ACTION_ASK) {
		gdk_drag_status (context, GDK_ACTION_ASK, _time);
	} else {
		gdk_drag_status (context, GDK_ACTION_MOVE, _time);
	}
}

static void
drop_playlist_cb (GtkWidget     *widget,
	       GdkDragContext     *context,
	       gint                x,
	       gint                y,
	       GtkSelectionData   *data,
	       guint               info,
	       guint               _time,
	       Totem              *totem)
{
	gboolean empty_pl;
	GdkDragAction action = gdk_drag_context_get_selected_action (context);

	if (action == GDK_ACTION_ASK) {
		action = totem_drag_ask (totem_get_playlist_length (totem) > 0);
		gdk_drag_status (context, action, GDK_CURRENT_TIME);
	}

	if (action == GDK_ACTION_DEFAULT) {
		gtk_drag_finish (context, FALSE, FALSE, _time);
		return;
	}

	empty_pl = (action == GDK_ACTION_MOVE);

	totem_action_drop_files (totem, data, info, empty_pl);
	gtk_drag_finish (context, TRUE, FALSE, _time);
}

static void
drag_motion_playlist_cb (GtkWidget      *widget,
			 GdkDragContext *context,
			 gint            x,
			 gint            y,
			 guint           _time,
			 Totem          *totem)
{
	GdkModifierType mask;

	gdk_window_get_pointer (gtk_widget_get_window (widget), NULL, NULL, &mask);

	if (mask & GDK_MOD1_MASK || gdk_drag_context_get_suggested_action (context) == GDK_ACTION_ASK)
		gdk_drag_status (context, GDK_ACTION_ASK, _time);
}
static void
drag_video_cb (GtkWidget *widget,
	       GdkDragContext *context,
	       GtkSelectionData *selection_data,
	       guint info,
	       guint32 _time,
	       gpointer callback_data)
{
	Totem *totem = (Totem *) callback_data;
	char *text;
	int len;
	GFile *file;

	g_assert (selection_data != NULL);

	if (totem->mrl == NULL)
		return;

	/* Canonicalise the MRL as a proper URI */
	file = g_file_new_for_commandline_arg (totem->mrl);
	text = g_file_get_uri (file);
	g_object_unref (file);

	g_return_if_fail (text != NULL);

	len = strlen (text);

	gtk_selection_data_set (selection_data, gtk_selection_data_get_target (selection_data),
				8, (guchar *) text, len);

	g_free (text);
}

static void
on_got_redirect (BaconVideoWidget *bvw, const char *mrl, Totem *totem)
{
	char *new_mrl;

	if (strstr (mrl, "://") != NULL) {
		new_mrl = NULL;
	} else {
		GFile *old_file, *parent, *new_file;
		char *old_mrl;

		/* Get the parent for the current MRL, that's our base */
		old_mrl = totem_playlist_get_current_mrl (TOTEM_PLAYLIST (totem->playlist), NULL);
		old_file = g_file_new_for_uri (old_mrl);
		g_free (old_mrl);
		parent = g_file_get_parent (old_file);
		g_object_unref (old_file);

		/* Resolve the URL */
		new_file = g_file_get_child (parent, mrl);
		g_object_unref (parent);

		new_mrl = g_file_get_uri (new_file);
		g_object_unref (new_file);
	}

	bacon_video_widget_close (totem->bvw);
	totem_file_closed (totem);
	totem_gdk_window_set_waiting_cursor (gtk_widget_get_window (totem->win));
	bacon_video_widget_open (totem->bvw, new_mrl ? new_mrl : mrl, NULL, NULL);
	totem_file_opened (totem, new_mrl ? new_mrl : mrl);
	gdk_window_set_cursor (gtk_widget_get_window (totem->win), NULL);
	bacon_video_widget_play (bvw, NULL);
	g_free (new_mrl);
}

static void
on_channels_change_event (BaconVideoWidget *bvw, Totem *totem)
{
	gchar *name;

	totem_sublang_update (totem);

	/* updated stream info (new song) */
	name = totem_get_nice_name_for_stream (totem);

	if (name != NULL) {
		update_mrl_label (totem, name);
		totem_playlist_set_title
			(TOTEM_PLAYLIST (totem->playlist), name, TRUE);
		g_free (name);
	}
}

static void
on_playlist_change_name (TotemPlaylist *playlist, Totem *totem)
{
	char *name;

	name = totem_playlist_get_current_title (playlist, NULL);
	if (name != NULL) {
		update_mrl_label (totem, name);
		g_free (name);
	}
}

static void
on_got_metadata_event (BaconVideoWidget *bvw, Totem *totem)
{
        char *name = NULL;
	
	name = totem_get_nice_name_for_stream (totem);

	if (name != NULL) {
		totem_playlist_set_title
			(TOTEM_PLAYLIST (totem->playlist), name, FALSE);
		g_free (name);
	}
	
	on_playlist_change_name (TOTEM_PLAYLIST (totem->playlist), totem);
}

static void
on_error_event (BaconVideoWidget *bvw, char *message,
                gboolean playback_stopped, gboolean fatal, Totem *totem)
{
	/* Clear the seek if it's there, we only want to try and seek
	 * the first file, even if it's not there */
	totem->seek_to = 0;
	totem->seek_to_start = 0;

	if (playback_stopped)
		play_pause_set_label (totem, STATE_STOPPED);

	if (fatal == FALSE) {
		totem_action_error (_("An error occurred"), message, totem);
	} else {
		totem_action_error_and_exit (_("An error occurred"),
				message, totem);
	}
}

static void
on_buffering_event (BaconVideoWidget *bvw, int percentage, Totem *totem)
{
	totem_statusbar_push (TOTEM_STATUSBAR (totem->statusbar), percentage);
}

static void
on_download_buffering_event (BaconVideoWidget *bvw, gdouble level, Totem *totem)
{
	update_fill (totem, level);
}

static void
update_fill (Totem *totem, gdouble level)
{
	if (level < 0.0) {
		gtk_range_set_show_fill_level (GTK_RANGE (totem->seek), FALSE);
		gtk_range_set_show_fill_level (GTK_RANGE (totem->fs->seek), FALSE);
	} else {
		gtk_range_set_fill_level (GTK_RANGE (totem->seek), level * 65535.0f);
		gtk_range_set_show_fill_level (GTK_RANGE (totem->seek), TRUE);

		gtk_range_set_fill_level (GTK_RANGE (totem->fs->seek), level * 65535.0f);
		gtk_range_set_show_fill_level (GTK_RANGE (totem->fs->seek), TRUE);
	}
}

static void
update_seekable (Totem *totem)
{
	GtkAction *action;
	GtkActionGroup *action_group;
	gboolean seekable;

	seekable = bacon_video_widget_is_seekable (totem->bvw);
	if (totem->seekable == seekable)
		return;
	totem->seekable = seekable;

	/* Check if the stream is seekable */
	gtk_widget_set_sensitive (totem->seek, seekable);

	totem_main_set_sensitivity ("tmw_seek_hbox", seekable);

	totem_fullscreen_set_seekable (totem->fs, seekable);

	/* FIXME: We can use this code again once bug #457631 is fixed and
	 * skip-* are back in the main action group. */
	/*totem_action_set_sensitivity ("skip-forward", seekable);
	totem_action_set_sensitivity ("skip-backwards", seekable);*/
	action_group = GTK_ACTION_GROUP (gtk_builder_get_object (totem->xml, "skip-action-group"));

	action = gtk_action_group_get_action (action_group, "skip-forward");
	gtk_action_set_sensitive (action, seekable);

	action = gtk_action_group_get_action (action_group, "skip-backwards");
	gtk_action_set_sensitive (action, seekable);

	/* This is for the session restore and the position saving
	 * to seek to the saved time */
	if (seekable != FALSE) {
		if (totem->seek_to_start != 0) {
			bacon_video_widget_seek_time (totem->bvw,
						      totem->seek_to_start, FALSE, NULL);
			totem_action_pause (totem);
		} else if (totem->seek_to != 0) {
			bacon_video_widget_seek_time (totem->bvw,
						      totem->seek_to, FALSE, NULL);
		}
	}
	totem->seek_to = 0;
	totem->seek_to_start = 0;

	g_object_notify (G_OBJECT (totem), "seekable");
}

static void
update_current_time (BaconVideoWidget *bvw,
		gint64 current_time,
		gint64 stream_length,
		double current_position,
		gboolean seekable, Totem *totem)
{
	if (totem->seek_lock == FALSE)
	{
		gtk_adjustment_set_value (totem->seekadj,
				current_position * 65535);

		if (stream_length == 0 && totem->mrl != NULL)
		{
			totem_statusbar_set_time_and_length
				(TOTEM_STATUSBAR (totem->statusbar),
				(int) (current_time / 1000), -1);
		} else {
			totem_statusbar_set_time_and_length
				(TOTEM_STATUSBAR (totem->statusbar),
				(int) (current_time / 1000),
				(int) (stream_length / 1000));
		}

		totem_time_label_set_time
			(TOTEM_TIME_LABEL (totem->fs->time_label),
			 current_time, stream_length);
	}

	if (totem->stream_length != stream_length) {
		g_object_notify (G_OBJECT (totem), "stream-length");
		totem->stream_length = stream_length;
	}
}

void
volume_button_value_changed_cb (GtkScaleButton *button, gdouble value, Totem *totem)
{
	totem->muted = FALSE;
	bacon_video_widget_set_volume (totem->bvw, value);
}

static void
update_volume_sliders (Totem *totem)
{
	double volume;
	GtkAction *action;

	volume = bacon_video_widget_get_volume (totem->bvw);

	g_signal_handlers_block_by_func (totem->volume, volume_button_value_changed_cb, totem);
	gtk_scale_button_set_value (GTK_SCALE_BUTTON (totem->volume), volume);
	g_signal_handlers_unblock_by_func (totem->volume, volume_button_value_changed_cb, totem);
  
	action = gtk_action_group_get_action (totem->main_action_group, "volume-down");
	gtk_action_set_sensitive (action, volume > VOLUME_EPSILON && totem->volume_sensitive);

	action = gtk_action_group_get_action (totem->main_action_group, "volume-up");
	gtk_action_set_sensitive (action, volume < (1.0 - VOLUME_EPSILON) && totem->volume_sensitive);
}

static void
property_notify_cb_volume (BaconVideoWidget *bvw, GParamSpec *spec, Totem *totem)
{
	update_volume_sliders (totem);
}

static void
property_notify_cb_logo_mode (BaconVideoWidget *bvw, GParamSpec *spec, Totem *totem)
{
	gboolean enabled;
	enabled = bacon_video_widget_get_logo_mode (totem->bvw);
	totem_action_zoom (totem, enabled ? ZOOM_DISABLE : ZOOM_ENABLE);
}

static void
property_notify_cb_seekable (BaconVideoWidget *bvw, GParamSpec *spec, Totem *totem)
{
	update_seekable (totem);
}

gboolean
seek_slider_pressed_cb (GtkWidget *widget, GdkEventButton *event, Totem *totem)
{
	/* HACK: we want the behaviour you get with the middle button, so we
	 * mangle the event.  clicking with other buttons moves the slider in
	 * step increments, clicking with the middle button moves the slider to
	 * the location of the click.
	 */
	event->button = 2;

	totem->seek_lock = TRUE;
	if (bacon_video_widget_can_direct_seek (totem->bvw) == FALSE) {
		totem_statusbar_set_seeking (TOTEM_STATUSBAR (totem->statusbar), TRUE);
		totem_time_label_set_seeking (TOTEM_TIME_LABEL (totem->fs->time_label), TRUE);
	}

	return FALSE;
}

void
seek_slider_changed_cb (GtkAdjustment *adj, Totem *totem)
{
	double pos;
	gint _time;

	if (totem->seek_lock == FALSE)
		return;

	pos = gtk_adjustment_get_value (adj) / 65535;
	_time = bacon_video_widget_get_stream_length (totem->bvw);
	totem_statusbar_set_time_and_length (TOTEM_STATUSBAR (totem->statusbar),
			(int) (pos * _time / 1000), _time / 1000);
	totem_time_label_set_time
			(TOTEM_TIME_LABEL (totem->fs->time_label),
			 (int) (pos * _time), _time);

	if (bacon_video_widget_can_direct_seek (totem->bvw) != FALSE)
		totem_action_seek (totem, pos);
}

gboolean
seek_slider_released_cb (GtkWidget *widget, GdkEventButton *event, Totem *totem)
{
	GtkAdjustment *adj;
	gdouble val;

	/* HACK: see seek_slider_pressed_cb */
	event->button = 2;

	/* set to FALSE here to avoid triggering a final seek when
	 * syncing the adjustments while being in direct seek mode */
	totem->seek_lock = FALSE;

	/* sync both adjustments */
	adj = gtk_range_get_adjustment (GTK_RANGE (widget));
	val = gtk_adjustment_get_value (adj);

	if (bacon_video_widget_can_direct_seek (totem->bvw) == FALSE)
		totem_action_seek (totem, val / 65535.0);

	totem_statusbar_set_seeking (TOTEM_STATUSBAR (totem->statusbar), FALSE);
	totem_time_label_set_seeking (TOTEM_TIME_LABEL (totem->fs->time_label),
			FALSE);
	return FALSE;
}

gboolean
totem_action_open_files (Totem *totem, char **list)
{
	GSList *slist = NULL;
	int i, retval;

	for (i = 0 ; list[i] != NULL; i++)
		slist = g_slist_prepend (slist, list[i]);

	slist = g_slist_reverse (slist);
	retval = totem_action_open_files_list (totem, slist);
	g_slist_free (slist);

	return retval;
}

static gboolean
totem_action_open_files_list (Totem *totem, GSList *list)
{
	GSList *l;
	gboolean changed;
	gboolean cleared;

	changed = FALSE;
	cleared = FALSE;

	if (list == NULL)
		return changed;

	totem_gdk_window_set_waiting_cursor (gtk_widget_get_window (totem->win));

	for (l = list ; l != NULL; l = l->next)
	{
		char *filename;
		char *data = l->data;

		if (data == NULL)
			continue;

		/* Ignore relatives paths that start with "--", tough luck */
		if (data[0] == '-' && data[1] == '-')
			continue;

		/* Get the subtitle part out for our tests */
		filename = totem_create_full_path (data);
		if (filename == NULL)
			filename = g_strdup (data);

		if (g_file_test (filename, G_FILE_TEST_IS_REGULAR)
				|| strstr (filename, "#") != NULL
				|| strstr (filename, "://") != NULL
				|| g_str_has_prefix (filename, "dvd:") != FALSE
				|| g_str_has_prefix (filename, "vcd:") != FALSE
				|| g_str_has_prefix (filename, "dvb:") != FALSE)
		{
			if (cleared == FALSE)
			{
				/* The function that calls us knows better
				 * if we should be doing something with the 
				 * changed playlist ... */
				g_signal_handlers_disconnect_by_func
					(G_OBJECT (totem->playlist),
					 playlist_changed_cb, totem);
				changed = totem_playlist_clear (totem->playlist);
				bacon_video_widget_close (totem->bvw);
				totem_file_closed (totem);
				cleared = TRUE;
			}

			if (totem_is_block_device (filename) != FALSE) {
				totem_action_load_media_device (totem, data);
				changed = TRUE;
			} else if (g_str_has_prefix (filename, "dvb:/") != FALSE) {
				totem_playlist_add_mrl (totem->playlist, data, NULL, FALSE, NULL, NULL, NULL);
				changed = TRUE;
			} else {
				totem_playlist_add_mrl (totem->playlist, filename, NULL, FALSE, NULL, NULL, NULL);
				changed = TRUE;
			}
		}

		g_free (filename);
	}

	gdk_window_set_cursor (gtk_widget_get_window (totem->win), NULL);

	/* ... and reconnect because we're nice people */
	if (cleared != FALSE)
	{
		g_signal_connect (G_OBJECT (totem->playlist),
				"changed", G_CALLBACK (playlist_changed_cb),
				totem);
	}

	return changed;
}

void
show_controls (Totem *totem, gboolean was_fullscreen)
{
	GtkAction *action;
	GtkWidget *menubar, *controlbar, *statusbar, *bvw_box, *widget;
	GtkAllocation allocation;
	int width = 0, height = 0;

	if (totem->bvw == NULL)
		return;

	menubar = GTK_WIDGET (gtk_builder_get_object (totem->xml, "tmw_menubar_box"));
	controlbar = GTK_WIDGET (gtk_builder_get_object (totem->xml, "tmw_controls_vbox"));
	statusbar = GTK_WIDGET (gtk_builder_get_object (totem->xml, "tmw_statusbar"));
	bvw_box = GTK_WIDGET (gtk_builder_get_object (totem->xml, "tmw_bvw_box"));
	widget = GTK_WIDGET (totem->bvw);

	action = gtk_action_group_get_action (totem->main_action_group, "show-controls");
	gtk_action_set_sensitive (action, !totem_is_fullscreen (totem));
	gtk_widget_get_allocation (widget, &allocation);

	if (totem->controls_visibility == TOTEM_CONTROLS_VISIBLE) {
		if (was_fullscreen == FALSE) {
			height = allocation.height;
			width = allocation.width;
		}

		gtk_widget_set_sensitive (menubar, TRUE);
		gtk_widget_show (menubar);
		gtk_widget_show (controlbar);
		gtk_widget_show (statusbar);
		if (totem_sidebar_is_visible (totem) != FALSE) {
			/* This is uglier then you might expect because of the
			   resize handle between the video and sidebar. There
			   is no convenience method to get the handle's width.
			   */
			GValue value = { 0, };
			GtkWidget *pane;
			GtkAllocation allocation_sidebar;
			int handle_size;

			g_value_init (&value, G_TYPE_INT);
			pane = GTK_WIDGET (gtk_builder_get_object (totem->xml,
					"tmw_main_pane"));
			gtk_widget_style_get_property (pane, "handle-size",
					&value);
			handle_size = g_value_get_int (&value);
			g_value_unset (&value);

			gtk_widget_show (totem->sidebar);
			gtk_widget_get_allocation (totem->sidebar, &allocation_sidebar);
			width += allocation_sidebar.width + handle_size;
		} else {
			gtk_widget_hide (totem->sidebar);
		}

		if (was_fullscreen == FALSE) {
			GtkAllocation allocation_menubar;
			GtkAllocation allocation_controlbar;
			GtkAllocation allocation_statusbar;

			gtk_widget_get_allocation (menubar, &allocation_menubar);
			gtk_widget_get_allocation (controlbar, &allocation_controlbar);
			gtk_widget_get_allocation (statusbar, &allocation_statusbar);
			height += allocation_menubar.height
				+ allocation_controlbar.height
				+ allocation_statusbar.height;
			gtk_window_resize (GTK_WINDOW(totem->win),
					width, height);
		}
	} else {
		if (totem->controls_visibility == TOTEM_CONTROLS_HIDDEN) {
			width = allocation.width;
			height = allocation.height;
		}

		/* Hide and make the menubar unsensitive */
		gtk_widget_set_sensitive (menubar, FALSE);
		gtk_widget_hide (menubar);

		gtk_widget_hide (controlbar);
		gtk_widget_hide (statusbar);
		gtk_widget_hide (totem->sidebar);

		 /* We won't show controls in fullscreen */
		gtk_container_set_border_width (GTK_CONTAINER (bvw_box), 0);

		if (totem->controls_visibility == TOTEM_CONTROLS_HIDDEN) {
			gtk_window_resize (GTK_WINDOW(totem->win),
					width, height);
		}
	}
}

/**
 * totem_action_toggle_controls:
 * @totem: a #TotemObject
 *
 * If Totem's not fullscreened, this toggles the state of the "Show Controls"
 * menu entry, and consequently shows or hides the controls in the UI.
 **/
void
totem_action_toggle_controls (Totem *totem)
{
	GtkAction *action;
	gboolean state;

	if (totem_is_fullscreen (totem) != FALSE)
		return;

 	action = gtk_action_group_get_action (totem->main_action_group,
 		"show-controls");
 	state = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
 	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), !state);
}

/**
 * totem_action_next_angle:
 * @totem: a #TotemObject
 *
 * Switches to the next angle, if watching a DVD. If not watching a DVD, this is a
 * no-op.
 **/
void
totem_action_next_angle (Totem *totem)
{
	if (totem_playing_dvd (totem->mrl) != FALSE)
		bacon_video_widget_dvd_event (totem->bvw, BVW_DVD_NEXT_ANGLE);
}

/**
 * totem_action_set_playlist_index:
 * @totem: a #TotemObject
 * @index: the new playlist index
 *
 * Sets the %0-based playlist index to @index, causing Totem to load and
 * start playing that playlist entry.
 *
 * If @index is higher than the current length of the playlist, this
 * has the effect of restarting the current playlist entry.
 **/
void
totem_action_set_playlist_index (Totem *totem, guint playlist_index)
{
	char *mrl, *subtitle;

	totem_playlist_set_current (totem->playlist, playlist_index);
	mrl = totem_playlist_get_current_mrl (totem->playlist, &subtitle);
	totem_action_set_mrl_and_play (totem, mrl, subtitle);
	g_free (mrl);
	g_free (subtitle);
}

/**
 * totem_action_remote:
 * @totem: a #TotemObject
 * @cmd: a #TotemRemoteCommand
 * @url: an MRL to play, or %NULL
 *
 * Executes the specified @cmd on this instance of Totem. If @cmd
 * is an operation requiring an MRL, @url is required; it can be %NULL
 * otherwise.
 *
 * If Totem's fullscreened and the operation is executed correctly,
 * the controls will appear as if the user had moved the mouse.
 **/
void
totem_action_remote (Totem *totem, TotemRemoteCommand cmd, const char *url)
{
	const char *icon_name;
	gboolean handled;

	icon_name = NULL;
	handled = TRUE;

	switch (cmd) {
	case TOTEM_REMOTE_COMMAND_PLAY:
		totem_action_play (totem);
		icon_name = "gtk-media-play";
		break;
	case TOTEM_REMOTE_COMMAND_PLAYPAUSE:
		if (bacon_video_widget_is_playing (totem->bvw) == FALSE)
			icon_name = "gtk-media-play";
		else
			icon_name = "gtk-media-pause";
		totem_action_play_pause (totem);
		break;
	case TOTEM_REMOTE_COMMAND_PAUSE:
		totem_action_pause (totem);
		icon_name = "gtk-media-pause";
		break;
	case TOTEM_REMOTE_COMMAND_STOP: {
		char *mrl, *subtitle;

		totem_playlist_set_at_start (totem->playlist);
		update_buttons (totem);
		totem_action_stop (totem);
		mrl = totem_playlist_get_current_mrl (totem->playlist, &subtitle);
		if (mrl != NULL) {
			totem_action_set_mrl_with_warning (totem, mrl, subtitle, FALSE);
			bacon_video_widget_pause (totem->bvw);
			g_free (mrl);
			g_free (subtitle);
		}
		icon_name = "gtk-media-stop";
		break;
	};
	case TOTEM_REMOTE_COMMAND_SEEK_FORWARD: {
		double offset = 0;

		if (url != NULL)
			offset = g_ascii_strtod (url, NULL);
		if (offset == 0) {
			totem_action_seek_relative (totem, SEEK_FORWARD_OFFSET * 1000, FALSE);
		} else {
			totem_action_seek_relative (totem, offset * 1000, FALSE);
		}
		icon_name = "gtk-media-forward";
		break;
	}
	case TOTEM_REMOTE_COMMAND_SEEK_BACKWARD: {
		double offset = 0;

		if (url != NULL)
			offset = g_ascii_strtod (url, NULL);
		if (offset == 0)
			totem_action_seek_relative (totem, SEEK_BACKWARD_OFFSET * 1000, FALSE);
		else
			totem_action_seek_relative (totem,  - (offset * 1000), FALSE);
		icon_name = "gtk-media-rewind";
		break;
	}
	case TOTEM_REMOTE_COMMAND_VOLUME_UP:
		totem_action_volume_relative (totem, VOLUME_UP_OFFSET);
		break;
	case TOTEM_REMOTE_COMMAND_VOLUME_DOWN:
		totem_action_volume_relative (totem, VOLUME_DOWN_OFFSET);
		break;
	case TOTEM_REMOTE_COMMAND_NEXT:
		totem_action_next (totem);
		icon_name = "gtk-media-next";
		break;
	case TOTEM_REMOTE_COMMAND_PREVIOUS:
		totem_action_previous (totem);
		icon_name = "gtk-media-previous";
		break;
	case TOTEM_REMOTE_COMMAND_FULLSCREEN:
		totem_action_fullscreen_toggle (totem);
		break;
	case TOTEM_REMOTE_COMMAND_QUIT:
		totem_action_exit (totem);
		break;
	case TOTEM_REMOTE_COMMAND_ENQUEUE:
		g_assert (url != NULL);
		totem_playlist_add_mrl (totem->playlist, url, NULL, TRUE, NULL, NULL, NULL);
		break;
	case TOTEM_REMOTE_COMMAND_REPLACE:
		totem_playlist_clear (totem->playlist);
		if (url == NULL) {
			bacon_video_widget_close (totem->bvw);
			totem_file_closed (totem);
			totem_action_set_mrl (totem, NULL, NULL);
			break;
		}
		if (strcmp (url, "dvd:") == 0) {
			/* FIXME b0rked */
			totem_action_play_media (totem, MEDIA_TYPE_DVD, NULL);
		} else if (strcmp (url, "vcd:") == 0) {
			/* FIXME b0rked */
			totem_action_play_media (totem, MEDIA_TYPE_VCD, NULL);
		} else {
			totem_playlist_add_mrl (totem->playlist, url, NULL, TRUE, NULL, NULL, NULL);
		}
		break;
	case TOTEM_REMOTE_COMMAND_SHOW:
		gtk_window_present (GTK_WINDOW (totem->win));
		break;
	case TOTEM_REMOTE_COMMAND_TOGGLE_CONTROLS:
		if (totem->controls_visibility != TOTEM_CONTROLS_FULLSCREEN)
		{
			GtkToggleAction *action;
			gboolean state;

			action = GTK_TOGGLE_ACTION (gtk_action_group_get_action
					(totem->main_action_group,
					 "show-controls"));
			state = gtk_toggle_action_get_active (action);
			gtk_toggle_action_set_active (action, !state);
		}
		break;
	case TOTEM_REMOTE_COMMAND_UP:
		bacon_video_widget_dvd_event (totem->bvw,
				BVW_DVD_ROOT_MENU_UP);
		break;
	case TOTEM_REMOTE_COMMAND_DOWN:
		bacon_video_widget_dvd_event (totem->bvw,
				BVW_DVD_ROOT_MENU_DOWN);
		break;
	case TOTEM_REMOTE_COMMAND_LEFT:
		bacon_video_widget_dvd_event (totem->bvw,
				BVW_DVD_ROOT_MENU_LEFT);
		break;
	case TOTEM_REMOTE_COMMAND_RIGHT:
		bacon_video_widget_dvd_event (totem->bvw,
				BVW_DVD_ROOT_MENU_RIGHT);
		break;
	case TOTEM_REMOTE_COMMAND_SELECT:
		bacon_video_widget_dvd_event (totem->bvw,
				BVW_DVD_ROOT_MENU_SELECT);
		break;
	case TOTEM_REMOTE_COMMAND_DVD_MENU:
		bacon_video_widget_dvd_event (totem->bvw,
				BVW_DVD_ROOT_MENU);
		break;
	case TOTEM_REMOTE_COMMAND_ZOOM_UP:
		totem_action_zoom_relative (totem, ZOOM_IN_OFFSET);
		break;
	case TOTEM_REMOTE_COMMAND_ZOOM_DOWN:
		totem_action_zoom_relative (totem, ZOOM_OUT_OFFSET);
		break;
	case TOTEM_REMOTE_COMMAND_EJECT:
		totem_action_eject (totem);
		icon_name = "media-eject";
		break;
	case TOTEM_REMOTE_COMMAND_PLAY_DVD:
		/* TODO - how to see if can, and play the DVD (like the menu item) */
		break;
	case TOTEM_REMOTE_COMMAND_MUTE:
		totem_action_volume_toggle_mute (totem);
		break;
	case TOTEM_REMOTE_COMMAND_TOGGLE_ASPECT:
		totem_action_toggle_aspect_ratio (totem);
		break;
	case TOTEM_REMOTE_COMMAND_UNKNOWN:
	default:
		handled = FALSE;
		break;
	}

	if (handled != FALSE
			&& gtk_window_is_active (GTK_WINDOW (totem->win))
			&& totem_fullscreen_is_fullscreen (totem->fs) != FALSE) {
		totem_fullscreen_show_popups_or_osd (totem->fs, icon_name, TRUE);
	}
}

/**
 * totem_action_remote_set_setting:
 * @totem: a #TotemObject
 * @setting: a #TotemRemoteSetting
 * @value: the new value for the setting
 *
 * Sets @setting to @value on this instance of Totem.
 **/
void totem_action_remote_set_setting (Totem *totem,
				      TotemRemoteSetting setting,
				      gboolean value)
{
	GtkAction *action;

	action = NULL;

	switch (setting) {
	case TOTEM_REMOTE_SETTING_SHUFFLE:
		action = gtk_action_group_get_action (totem->main_action_group, "shuffle-mode");
		break;
	case TOTEM_REMOTE_SETTING_REPEAT:
		action = gtk_action_group_get_action (totem->main_action_group, "repeat-mode");
		break;
	default:
		g_assert_not_reached ();
	}

	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), value);
}

/**
 * totem_action_remote_get_setting:
 * @totem: a #TotemObject
 * @setting: a #TotemRemoteSetting
 *
 * Returns the value of @setting for this instance of Totem.
 *
 * Return value: %TRUE if the setting is enabled, %FALSE otherwise
 **/
gboolean totem_action_remote_get_setting (Totem *totem,
					  TotemRemoteSetting setting)
{
	GtkAction *action;

	action = NULL;

	switch (setting) {
	case TOTEM_REMOTE_SETTING_SHUFFLE:
		action = gtk_action_group_get_action (totem->main_action_group, "shuffle-mode");
		break;
	case TOTEM_REMOTE_SETTING_REPEAT:
		action = gtk_action_group_get_action (totem->main_action_group, "repeat-mode");
		break;
	default:
		g_assert_not_reached ();
	}

	return gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
}

static void
playlist_changed_cb (GtkWidget *playlist, Totem *totem)
{
	char *mrl, *subtitle;

	update_buttons (totem);
	mrl = totem_playlist_get_current_mrl (totem->playlist, &subtitle);

	if (mrl == NULL)
		return;

	if (totem_playlist_get_playing (totem->playlist) == TOTEM_PLAYLIST_STATUS_NONE)
		totem_action_set_mrl_and_play (totem, mrl, subtitle);

	g_free (mrl);
	g_free (subtitle);
}

static void
item_activated_cb (GtkWidget *playlist, Totem *totem)
{
	totem_action_seek (totem, 0);
}

static void
current_removed_cb (GtkWidget *playlist, Totem *totem)
{
	char *mrl, *subtitle;

	/* Set play button status */
	play_pause_set_label (totem, STATE_STOPPED);
	mrl = totem_playlist_get_current_mrl (totem->playlist, &subtitle);

	if (mrl == NULL) {
		g_free (subtitle);
		subtitle = NULL;
		totem_playlist_set_at_start (totem->playlist);
		update_buttons (totem);
		mrl = totem_playlist_get_current_mrl (totem->playlist, &subtitle);
	} else {
		update_buttons (totem);
	}

	totem_action_set_mrl_and_play (totem, mrl, subtitle);
	g_free (mrl);
	g_free (subtitle);
}

static void
subtitle_changed_cb (GtkWidget *playlist, Totem *totem)
{
	char *mrl, *subtitle;

	totem_action_stop (totem);
	mrl = totem_playlist_get_current_mrl (totem->playlist, &subtitle);
	totem_action_set_mrl_and_play (totem, mrl, subtitle);

	g_free (mrl);
	g_free (subtitle);
}

static void
playlist_repeat_toggle_cb (TotemPlaylist *playlist, gboolean repeat, Totem *totem)
{
	GtkAction *action;

	action = gtk_action_group_get_action (totem->main_action_group, "repeat-mode");

	g_signal_handlers_block_matched (G_OBJECT (action), G_SIGNAL_MATCH_DATA, 0, 0,
			NULL, NULL, totem);

	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), repeat);

	g_signal_handlers_unblock_matched (G_OBJECT (action), G_SIGNAL_MATCH_DATA, 0, 0,
			NULL, NULL, totem);
}

static void
playlist_shuffle_toggle_cb (TotemPlaylist *playlist, gboolean shuffle, Totem *totem)
{
	GtkAction *action;

	action = gtk_action_group_get_action (totem->main_action_group, "shuffle-mode");

	g_signal_handlers_block_matched (G_OBJECT (action), G_SIGNAL_MATCH_DATA, 0, 0,
			NULL, NULL, totem);

	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), shuffle);

	g_signal_handlers_unblock_matched (G_OBJECT (action), G_SIGNAL_MATCH_DATA, 0, 0,
			NULL, NULL, totem);
}

/**
 * totem_is_fullscreen:
 * @totem: a #TotemObject
 *
 * Returns %TRUE if Totem is fullscreened.
 *
 * Return value: %TRUE if Totem is fullscreened
 **/
gboolean
totem_is_fullscreen (Totem *totem)
{
	g_return_val_if_fail (TOTEM_IS_OBJECT (totem), FALSE);

	return (totem->controls_visibility == TOTEM_CONTROLS_FULLSCREEN);
}

/**
 * totem_is_playing:
 * @totem: a #TotemObject
 *
 * Returns %TRUE if Totem is playing a stream.
 *
 * Return value: %TRUE if Totem is playing a stream
 **/
gboolean
totem_is_playing (Totem *totem)
{
	g_return_val_if_fail (TOTEM_IS_OBJECT (totem), FALSE);

	if (totem->bvw == NULL)
		return FALSE;

	return bacon_video_widget_is_playing (totem->bvw) != FALSE;
}

/**
 * totem_is_paused:
 * @totem: a #TotemObject
 *
 * Returns %TRUE if playback is paused.
 *
 * Return value: %TRUE if playback is paused, %FALSE otherwise
 **/
gboolean
totem_is_paused (Totem *totem)
{
	g_return_val_if_fail (TOTEM_IS_OBJECT (totem), FALSE);

	return totem->state == STATE_PAUSED;
}

/**
 * totem_is_seekable:
 * @totem: a #TotemObject
 *
 * Returns %TRUE if the current stream is seekable.
 *
 * Return value: %TRUE if the current stream is seekable
 **/
gboolean
totem_is_seekable (Totem *totem)
{
	g_return_val_if_fail (TOTEM_IS_OBJECT (totem), FALSE);

	if (totem->bvw == NULL)
		return FALSE;

	return bacon_video_widget_is_seekable (totem->bvw) != FALSE;
}

static void
on_mouse_click_fullscreen (GtkWidget *widget, Totem *totem)
{
	if (totem_fullscreen_is_fullscreen (totem->fs) != FALSE)
		totem_fullscreen_show_popups (totem->fs, TRUE);
}

static gboolean
on_video_button_press_event (BaconVideoWidget *bvw, GdkEventButton *event,
		Totem *totem)
{
	if (event->type == GDK_BUTTON_PRESS && event->button == 1) {
		gtk_widget_grab_focus (GTK_WIDGET (bvw));
		return TRUE;
	} else if (event->type == GDK_2BUTTON_PRESS && event->button == 1) {
		totem_action_fullscreen_toggle(totem);
		return TRUE;
	} else if (event->type == GDK_BUTTON_PRESS && event->button == 2) {
		if (totem_is_fullscreen (totem) != FALSE) {
			const char *icon_name;
			if (bacon_video_widget_is_playing (totem->bvw) == FALSE)
				icon_name = "gtk-media-play";
			else
				icon_name = "gtk-media-pause";
			totem_fullscreen_show_popups_or_osd (totem->fs, icon_name, FALSE);
		}
		totem_action_play_pause (totem);
		return TRUE;
	} else if (event->type == GDK_BUTTON_PRESS && event->button == 3) {
		totem_action_menu_popup (totem, event->button);
		return TRUE;
	}

	return FALSE;
}

static gboolean
on_eos_event (GtkWidget *widget, Totem *totem)
{
	reset_seek_status (totem);

	if (bacon_video_widget_get_logo_mode (totem->bvw) != FALSE)
		return FALSE;

	if (totem_playlist_has_next_mrl (totem->playlist) == FALSE &&
	    totem_playlist_get_repeat (totem->playlist) == FALSE &&
	    (totem_playlist_get_last (totem->playlist) != 0 ||
	     totem_is_seekable (totem) == FALSE)) {
		char *mrl, *subtitle;

		/* Set play button status */
		totem_playlist_set_at_start (totem->playlist);
		update_buttons (totem);
		totem_action_stop (totem);
		mrl = totem_playlist_get_current_mrl (totem->playlist, &subtitle);
		totem_action_set_mrl_with_warning (totem, mrl, subtitle, FALSE);
		bacon_video_widget_pause (totem->bvw);
		g_free (mrl);
		g_free (subtitle);
	} else {
		if (totem_playlist_get_last (totem->playlist) == 0 &&
		    totem_is_seekable (totem)) {
			if (totem_playlist_get_repeat (totem->playlist) != FALSE) {
				totem_action_seek_time (totem, 0, FALSE);
				totem_action_play (totem);
			} else {
				totem_action_pause (totem);
				totem_action_seek_time (totem, 0, FALSE);
			}
		} else {
			totem_action_next (totem);
		}
	}

	return FALSE;
}

static gboolean
totem_action_handle_key_release (Totem *totem, GdkEventKey *event)
{
	gboolean retval = TRUE;

	switch (event->keyval) {
	case GDK_Left:
	case GDK_Right:
		totem_statusbar_set_seeking (TOTEM_STATUSBAR (totem->statusbar), FALSE);
		totem_time_label_set_seeking (TOTEM_TIME_LABEL (totem->fs->time_label), FALSE);
		break;
	default:
		retval = FALSE;
	}

	return retval;
}

static void
totem_action_handle_seek (Totem *totem, GdkEventKey *event, gboolean is_forward)
{
	if (is_forward != FALSE) {
		if (event->state & GDK_SHIFT_MASK)
			totem_action_seek_relative (totem, SEEK_FORWARD_SHORT_OFFSET * 1000, FALSE);
		else if (event->state & GDK_CONTROL_MASK)
			totem_action_seek_relative (totem, SEEK_FORWARD_LONG_OFFSET * 1000, FALSE);
		else
			totem_action_seek_relative (totem, SEEK_FORWARD_OFFSET * 1000, FALSE);
	} else {
		if (event->state & GDK_SHIFT_MASK)
			totem_action_seek_relative (totem, SEEK_BACKWARD_SHORT_OFFSET * 1000, FALSE);
		else if (event->state & GDK_CONTROL_MASK)
			totem_action_seek_relative (totem, SEEK_BACKWARD_LONG_OFFSET * 1000, FALSE);
		else
			totem_action_seek_relative (totem, SEEK_BACKWARD_OFFSET * 1000, FALSE);
	}
}

static gboolean
totem_action_handle_key_press (Totem *totem, GdkEventKey *event)
{
	gboolean retval;
	const char *icon_name;

	retval = TRUE;
	icon_name = NULL;

	switch (event->keyval) {
	case GDK_A:
	case GDK_a:
		totem_action_toggle_aspect_ratio (totem);
		break;
	case GDK_AudioPrev:
	case GDK_Back:
	case GDK_B:
	case GDK_b:
		totem_action_previous (totem);
		icon_name = "gtk-media-previous";
		break;
	case GDK_C:
	case GDK_c:
		bacon_video_widget_dvd_event (totem->bvw,
				BVW_DVD_CHAPTER_MENU);
		break;
	case GDK_F11:
	case GDK_f:
	case GDK_F:
		totem_action_fullscreen_toggle (totem);
		break;
	case GDK_g:
	case GDK_G:
		totem_action_next_angle (totem);
		break;
	case GDK_h:
	case GDK_H:
		totem_action_toggle_controls (totem);
		break;
	case GDK_M:
	case GDK_m:
		bacon_video_widget_dvd_event (totem->bvw, BVW_DVD_ROOT_MENU);
		break;
	case GDK_AudioNext:
	case GDK_Forward:
	case GDK_N:
	case GDK_n:
	case GDK_End:
		totem_action_next (totem);
		icon_name = "gtk-media-next";
		break;
	case GDK_OpenURL:
		totem_action_fullscreen (totem, FALSE);
		totem_action_open_location (totem);
		break;
	case GDK_O:
	case GDK_o:
	case GDK_Open:
		totem_action_fullscreen (totem, FALSE);
		totem_action_open (totem);
		break;
	case GDK_AudioPlay:
	case GDK_p:
	case GDK_P:
		if (event->state & GDK_CONTROL_MASK) {
			totem_action_show_properties (totem);
		} else {
			if (bacon_video_widget_is_playing (totem->bvw) == FALSE)
				icon_name = "gtk-media-play";
			else
				icon_name = "gtk-media-pause";
			totem_action_play_pause (totem);
		}
		break;
	case GDK_comma:
		totem_action_pause (totem);
		bacon_video_widget_step (totem->bvw, FALSE, NULL);
		break;
	case GDK_period:
		totem_action_pause (totem);
		bacon_video_widget_step (totem->bvw, TRUE, NULL);
		break;
	case GDK_AudioPause:
	case GDK_AudioStop:
		totem_action_pause (totem);
		icon_name = "gtk-media-pause";
		break;
	case GDK_q:
	case GDK_Q:
		totem_action_exit (totem);
		break;
	case GDK_r:
	case GDK_R:
	case GDK_ZoomIn:
		totem_action_zoom_relative (totem, ZOOM_IN_OFFSET);
		break;
	case GDK_t:
	case GDK_T:
	case GDK_ZoomOut:
		totem_action_zoom_relative (totem, ZOOM_OUT_OFFSET);
		break;
	case GDK_Eject:
		totem_action_eject (totem);
		icon_name = "media-eject";
		break;
	case GDK_Escape:
		if (event->state & GDK_SUPER_MASK)
			bacon_video_widget_dvd_event (totem->bvw, BVW_DVD_ROOT_MENU);
		else
			totem_action_fullscreen (totem, FALSE);
		break;
	case GDK_space:
	case GDK_Return:
		{
			GtkWidget *focus = gtk_window_get_focus (GTK_WINDOW (totem->win));
			if (totem_is_fullscreen (totem) != FALSE || focus == NULL ||
			    focus == GTK_WIDGET (totem->bvw) || focus == totem->seek) {
				if (event->keyval == GDK_space) {
					if (bacon_video_widget_is_playing (totem->bvw) == FALSE)
						icon_name = "gtk-media-play";
					else
						icon_name = "gtk-media-pause";
					totem_action_play_pause (totem);
				} else if (bacon_video_widget_has_menus (totem->bvw) != FALSE) {
					bacon_video_widget_dvd_event (totem->bvw, BVW_DVD_ROOT_MENU_SELECT);
				}
			} else
				retval = FALSE;
		}
		break;
	case GDK_Left:
	case GDK_Right:
		if (bacon_video_widget_has_menus (totem->bvw) == FALSE) {
			gboolean is_forward;

			is_forward = (event->keyval == GDK_Right);
			/* Switch direction in RTL environment */
			if (gtk_widget_get_direction (totem->win) == GTK_TEXT_DIR_RTL)
				is_forward = !is_forward;
			icon_name = is_forward ? "gtk-media-forward" : "gtk-media-rewind";

			totem_action_handle_seek (totem, event, is_forward);
		} else {
			if (event->keyval == GDK_Left)
				bacon_video_widget_dvd_event (totem->bvw, BVW_DVD_ROOT_MENU_LEFT);
			else
				bacon_video_widget_dvd_event (totem->bvw, BVW_DVD_ROOT_MENU_RIGHT);
		}
		break;
	case GDK_Home:
		totem_action_seek (totem, 0);
		icon_name = "gtk-media-rewind";
		break;
	case GDK_Up:
		if (bacon_video_widget_has_menus (totem->bvw) != FALSE)
			bacon_video_widget_dvd_event (totem->bvw, BVW_DVD_ROOT_MENU_UP);
		else
			totem_action_volume_relative (totem, VOLUME_UP_OFFSET);
		break;
	case GDK_Down:
		if (bacon_video_widget_has_menus (totem->bvw) != FALSE)
			bacon_video_widget_dvd_event (totem->bvw, BVW_DVD_ROOT_MENU_DOWN);
		else
			totem_action_volume_relative (totem, VOLUME_DOWN_OFFSET);
		break;
	case GDK_0:
		if (event->state & GDK_CONTROL_MASK)
			totem_action_zoom_reset (totem);
		else
			totem_action_set_scale_ratio (totem, 0.5);
		break;
	case GDK_onehalf:
		totem_action_set_scale_ratio (totem, 0.5);
		break;
	case GDK_1:
		totem_action_set_scale_ratio (totem, 1);
		break;
	case GDK_2:
		totem_action_set_scale_ratio (totem, 2);
		break;
	case GDK_Menu:
		totem_action_menu_popup (totem, 0);
		break;
	case GDK_F10:
		if (!(event->state & GDK_SHIFT_MASK))
			return FALSE;

		totem_action_menu_popup (totem, 0);
		break;
	case GDK_plus:
	case GDK_KP_Add:
		if (!(event->state & GDK_CONTROL_MASK)) {
			totem_action_next (totem);
		} else {
			totem_action_zoom_relative (totem, ZOOM_IN_OFFSET);
		}
		break;
	case GDK_minus:
	case GDK_KP_Subtract:
		if (!(event->state & GDK_CONTROL_MASK)) {
			totem_action_previous (totem);
		} else {
			totem_action_zoom_relative (totem, ZOOM_OUT_OFFSET);
		}
		break;
	case GDK_KP_Up:
	case GDK_KP_8:
		bacon_video_widget_dvd_event (totem->bvw, 
					      BVW_DVD_ROOT_MENU_UP);
		break;
	case GDK_KP_Down:
	case GDK_KP_2:
		bacon_video_widget_dvd_event (totem->bvw, 
					      BVW_DVD_ROOT_MENU_DOWN);
		break;
	case GDK_KP_Right:
	case GDK_KP_6:
		bacon_video_widget_dvd_event (totem->bvw, 
					      BVW_DVD_ROOT_MENU_RIGHT);
		break;
	case GDK_KP_Left:
	case GDK_KP_4:
		bacon_video_widget_dvd_event (totem->bvw, 
					      BVW_DVD_ROOT_MENU_LEFT);
		break;
	case GDK_KP_Begin:
	case GDK_KP_5:
		bacon_video_widget_dvd_event (totem->bvw,
					      BVW_DVD_ROOT_MENU_SELECT);
	default:
		retval = FALSE;
	}

	if (totem_is_fullscreen (totem) != FALSE && icon_name != NULL)
		totem_fullscreen_show_popups_or_osd (totem->fs,
						     icon_name,
						     FALSE);

	return retval;
}

static gboolean
totem_action_handle_scroll (Totem *totem, GdkScrollDirection direction)
{
	gboolean retval = TRUE;

	if (totem_fullscreen_is_fullscreen (totem->fs) != FALSE)
		totem_fullscreen_show_popups (totem->fs, TRUE);

	switch (direction) {
	case GDK_SCROLL_UP:
		totem_action_seek_relative (totem, SEEK_FORWARD_SHORT_OFFSET * 1000, FALSE);
		break;
	case GDK_SCROLL_DOWN:
		totem_action_seek_relative (totem, SEEK_BACKWARD_SHORT_OFFSET * 1000, FALSE);
		break;
	default:
		retval = FALSE;
	}

	return retval;
}

gboolean
window_key_press_event_cb (GtkWidget *win, GdkEventKey *event, Totem *totem)
{
	gboolean sidebar_handles_kbd;

	/* Shortcuts disabled? */
	if (totem->disable_kbd_shortcuts != FALSE)
		return FALSE;

	/* Check whether the sidebar needs the key events */
	if (event->type == GDK_KEY_PRESS) {
		if (totem_sidebar_is_focused (totem, &sidebar_handles_kbd) != FALSE) {
			/* Make Escape pass the focus to the video widget */
			if (sidebar_handles_kbd == FALSE &&
			    event->keyval == GDK_Escape)
				gtk_widget_grab_focus (GTK_WIDGET (totem->bvw));
			return FALSE;
		}
	} else {
		if (totem_sidebar_is_focused (totem, NULL) != FALSE)
			return FALSE;
	}

	/* Special case Eject, Open, Open URI and
	 * seeking keyboard shortcuts */
	if (event->state != 0
			&& (event->state & GDK_CONTROL_MASK))
	{
		switch (event->keyval) {
		case GDK_E:
		case GDK_e:
		case GDK_O:
		case GDK_o:
		case GDK_L:
		case GDK_l:
		case GDK_q:
		case GDK_Q:
		case GDK_S:
		case GDK_s:
		case GDK_Right:
		case GDK_Left:
		case GDK_plus:
		case GDK_KP_Add:
		case GDK_minus:
		case GDK_KP_Subtract:
		case GDK_0:
			if (event->type == GDK_KEY_PRESS)
				return totem_action_handle_key_press (totem, event);
			else
				return totem_action_handle_key_release (totem, event);
		default:
			break;
		}
	}

	if (event->state != 0 && (event->state & GDK_SUPER_MASK)) {
		switch (event->keyval) {
		case GDK_Escape:
			if (event->type == GDK_KEY_PRESS)
				return totem_action_handle_key_press (totem, event);
			else
				return totem_action_handle_key_release (totem, event);
		default:
			break;
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

	if (event->type == GDK_KEY_PRESS) {
		return totem_action_handle_key_press (totem, event);
	} else {
		return totem_action_handle_key_release (totem, event);
	}
}

gboolean
window_scroll_event_cb (GtkWidget *win, GdkEventScroll *event, Totem *totem)
{
	return totem_action_handle_scroll (totem, event->direction);
}

static void
update_media_menu_items (Totem *totem)
{
	GMount *mount;
	gboolean playing;

	playing = totem_playing_dvd (totem->mrl);

	totem_action_set_sensitivity ("dvd-root-menu", playing);
	totem_action_set_sensitivity ("dvd-title-menu", playing);
	totem_action_set_sensitivity ("dvd-audio-menu", playing);
	totem_action_set_sensitivity ("dvd-angle-menu", playing);
	totem_action_set_sensitivity ("dvd-chapter-menu", playing);
	/* FIXME we should only show that if we have multiple angles */
	totem_action_set_sensitivity ("next-angle", playing);

	mount = totem_get_mount_for_media (totem->mrl);
	totem_action_set_sensitivity ("eject", mount != NULL);
	if (mount != NULL)
		g_object_unref (mount);
}

static void
update_buttons (Totem *totem)
{
	gboolean has_item;

	/* Previous */
	if (totem_playing_dvd (totem->mrl) != FALSE)
		has_item = bacon_video_widget_has_previous_track (totem->bvw);
	else
		has_item = totem_playlist_has_previous_mrl (totem->playlist);

	totem_action_set_sensitivity ("previous-chapter", has_item);

	/* Next */
	if (totem_playing_dvd (totem->mrl) != FALSE)
		has_item = bacon_video_widget_has_next_track (totem->bvw);
	else
		has_item = totem_playlist_has_next_mrl (totem->playlist);

	totem_action_set_sensitivity ("next-chapter", has_item);
}

void
main_pane_size_allocated (GtkWidget *main_pane, GtkAllocation *allocation, Totem *totem)
{
	gulong handler_id;

	if (!totem->maximised || gtk_widget_get_mapped (totem->win)) {
		handler_id = g_signal_handler_find (main_pane, 
				G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA,
				0, 0, NULL,
				main_pane_size_allocated, totem);
		g_signal_handler_disconnect (main_pane, handler_id);

		gtk_paned_set_position (GTK_PANED (main_pane), allocation->width - totem->sidebar_w);
	}
}

char *
totem_setup_window (Totem *totem)
{
	GKeyFile *keyfile;
	int w, h, i;
	gboolean show_sidebar;
	char *filename, *page_id;
	GError *err = NULL;
	GtkWidget *vbox;
	GdkColor black;

	filename = g_build_filename (totem_dot_dir (), "state.ini", NULL);
	keyfile = g_key_file_new ();
	if (g_key_file_load_from_file (keyfile, filename,
			G_KEY_FILE_NONE, NULL) == FALSE) {
		totem->sidebar_w = 0;
		w = DEFAULT_WINDOW_W;
		h = DEFAULT_WINDOW_H;
		show_sidebar = TRUE;
		page_id = NULL;
		g_free (filename);
	} else {
		g_free (filename);

		w = g_key_file_get_integer (keyfile, "State", "window_w", &err);
		if (err != NULL) {
			w = 0;
			g_error_free (err);
			err = NULL;
		}

		h = g_key_file_get_integer (keyfile, "State", "window_h", &err);
		if (err != NULL) {
			h = 0;
			g_error_free (err);
			err = NULL;
		}

		show_sidebar = g_key_file_get_boolean (keyfile, "State",
				"show_sidebar", &err);
		if (err != NULL) {
			show_sidebar = TRUE;
			g_error_free (err);
			err = NULL;
		}

		totem->maximised = g_key_file_get_boolean (keyfile, "State",
				"maximised", &err);
		if (err != NULL) {
			g_error_free (err);
			err = NULL;
		}

		page_id = g_key_file_get_string (keyfile, "State",
				"sidebar_page", &err);
		if (err != NULL) {
			g_error_free (err);
			page_id = NULL;
			err = NULL;
		}

		totem->sidebar_w = g_key_file_get_integer (keyfile, "State",
				"sidebar_w", &err);
		if (err != NULL) {
			g_error_free (err);
			totem->sidebar_w = 0;
		}
		g_key_file_free (keyfile);
	}

	if (w > 0 && h > 0 && totem->maximised == FALSE) {
		gtk_window_set_default_size (GTK_WINDOW (totem->win),
				w, h);
		totem->window_w = w;
		totem->window_h = h;
	} else if (totem->maximised != FALSE) {
		gtk_window_maximize (GTK_WINDOW (totem->win));
	}

	/* Set the vbox to be completely black */
	vbox = GTK_WIDGET (gtk_builder_get_object (totem->xml, "tmw_bvw_box"));
	gdk_color_parse ("Black", &black);
	for (i = 0; i <= GTK_STATE_INSENSITIVE; i++)
		gtk_widget_modify_bg (vbox, i, &black);

	totem_sidebar_setup (totem, show_sidebar, page_id);
	return page_id;
}

void
totem_callback_connect (Totem *totem)
{
	GtkWidget *item, *arrow;
	GtkAction *action;
	GtkActionGroup *action_group;
	GtkBox *box;

	/* Menu items */
	action = gtk_action_group_get_action (totem->main_action_group, "repeat-mode");
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
		totem_playlist_get_repeat (totem->playlist));
	action = gtk_action_group_get_action (totem->main_action_group, "shuffle-mode");
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
		totem_playlist_get_shuffle (totem->playlist));

	/* Controls */
	box = GTK_BOX (gtk_builder_get_object (totem->xml, "tmw_buttons_hbox"));

	/* Previous */
	action = gtk_action_group_get_action (totem->main_action_group,
			"previous-chapter");
	item = gtk_action_create_tool_item (action);
	gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM (item), 
					_("Previous Chapter/Movie"));
	atk_object_set_name (gtk_widget_get_accessible (item),
			_("Previous Chapter/Movie"));
	gtk_box_pack_start (box, item, FALSE, FALSE, 0);

	/* Play/Pause */
	action = gtk_action_group_get_action (totem->main_action_group, "play");
	item = gtk_action_create_tool_item (action);
	atk_object_set_name (gtk_widget_get_accessible (item),
			_("Play / Pause"));
	gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM (item),
 					_("Play / Pause"));
	gtk_box_pack_start (box, item, FALSE, FALSE, 0);

	/* Next */
	action = gtk_action_group_get_action (totem->main_action_group,
			"next-chapter");
	item = gtk_action_create_tool_item (action);
	gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM (item), 
					_("Next Chapter/Movie"));
	atk_object_set_name (gtk_widget_get_accessible (item),
			_("Next Chapter/Movie"));
	gtk_box_pack_start (box, item, FALSE, FALSE, 0);

	/* Separator */
	item = GTK_WIDGET(gtk_separator_tool_item_new ());
	gtk_box_pack_start (box, item, FALSE, FALSE, 0);

	/* Fullscreen button */
	action = gtk_action_group_get_action (totem->main_action_group,
			"fullscreen");
	item = gtk_action_create_tool_item (action);
	gtk_tool_item_set_tooltip_text (GTK_TOOL_ITEM (item),
					_("Fullscreen"));
	atk_object_set_name (gtk_widget_get_accessible (item),
			_("Fullscreen"));
	gtk_box_pack_start (box, item, FALSE, FALSE, 0);

	/* Sidebar button (Drag'n'Drop) */
	box = GTK_BOX (gtk_builder_get_object (totem->xml, "tmw_sidebar_button_hbox"));
	action = gtk_action_group_get_action (totem->main_action_group, "sidebar");
	item = gtk_toggle_button_new ();
	gtk_activatable_set_related_action (GTK_ACTIVATABLE (item), action);
	arrow = gtk_arrow_new (GTK_ARROW_RIGHT, GTK_SHADOW_NONE);
	g_object_set_data (G_OBJECT (box), "arrow", arrow);
	gtk_button_set_image_position (GTK_BUTTON (item), GTK_POS_RIGHT);
	gtk_button_set_image (GTK_BUTTON (item), arrow);
	gtk_box_pack_start (box, item, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (item), "drag_data_received",
			G_CALLBACK (drop_playlist_cb), totem);
	g_signal_connect (G_OBJECT (item), "drag_motion",
			G_CALLBACK (drag_motion_playlist_cb), totem);
	gtk_drag_dest_set (item, GTK_DEST_DEFAULT_ALL,
			target_table, G_N_ELEMENTS (target_table),
			GDK_ACTION_COPY | GDK_ACTION_MOVE);

	/* Fullscreen window buttons */
	g_signal_connect (G_OBJECT (totem->fs->exit_button), "clicked",
			  G_CALLBACK (fs_exit1_activate_cb), totem);

	action = gtk_action_group_get_action (totem->main_action_group, "play");
	item = gtk_action_create_tool_item (action);
	gtk_box_pack_start (GTK_BOX (totem->fs->buttons_box), item, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (item), "clicked",
			G_CALLBACK (on_mouse_click_fullscreen), totem);

	action = gtk_action_group_get_action (totem->main_action_group, "previous-chapter");
	item = gtk_action_create_tool_item (action);
	gtk_box_pack_start (GTK_BOX (totem->fs->buttons_box), item, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (item), "clicked",
			G_CALLBACK (on_mouse_click_fullscreen), totem);

	action = gtk_action_group_get_action (totem->main_action_group, "next-chapter");
	item = gtk_action_create_tool_item (action);
	gtk_box_pack_start (GTK_BOX (totem->fs->buttons_box), item, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (item), "clicked",
			G_CALLBACK (on_mouse_click_fullscreen), totem);

	/* Connect the keys */
	gtk_widget_add_events (totem->win, GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);

	/* Connect the mouse wheel */
	gtk_widget_add_events (totem->win, GDK_SCROLL_MASK);
	gtk_widget_add_events (totem->seek, GDK_SCROLL_MASK);
	gtk_widget_add_events (totem->fs->seek, GDK_SCROLL_MASK);

	/* FIXME Hack to fix bug #462286 and #563894 */
	g_signal_connect (G_OBJECT (totem->fs->seek), "button-press-event",
			G_CALLBACK (seek_slider_pressed_cb), totem);
	g_signal_connect (G_OBJECT (totem->fs->seek), "button-release-event",
			G_CALLBACK (seek_slider_released_cb), totem);
	g_signal_connect (G_OBJECT (totem->fs->seek), "scroll-event",
			  G_CALLBACK (window_scroll_event_cb), totem);


	/* Set sensitivity of the toolbar buttons */
	totem_action_set_sensitivity ("play", FALSE);
	totem_action_set_sensitivity ("next-chapter", FALSE);
	totem_action_set_sensitivity ("previous-chapter", FALSE);
	/* FIXME: We can use this code again once bug #457631 is fixed
	 * and skip-* are back in the main action group. */
	/*totem_action_set_sensitivity ("skip-forward", FALSE);
	totem_action_set_sensitivity ("skip-backwards", FALSE);*/

	action_group = GTK_ACTION_GROUP (gtk_builder_get_object (totem->xml, "skip-action-group"));

	action = gtk_action_group_get_action (action_group, "skip-forward");
	gtk_action_set_sensitive (action, FALSE);

	action = gtk_action_group_get_action (action_group, "skip-backwards");
	gtk_action_set_sensitive (action, FALSE);
}

void
playlist_widget_setup (Totem *totem)
{
	totem->playlist = TOTEM_PLAYLIST (totem_playlist_new ());

	if (totem->playlist == NULL)
		totem_action_exit (totem);

	gtk_widget_show_all (GTK_WIDGET (totem->playlist));

	g_signal_connect (G_OBJECT (totem->playlist), "active-name-changed",
			  G_CALLBACK (on_playlist_change_name), totem);
	g_signal_connect (G_OBJECT (totem->playlist), "item-activated",
			  G_CALLBACK (item_activated_cb), totem);
	g_signal_connect (G_OBJECT (totem->playlist),
			  "changed", G_CALLBACK (playlist_changed_cb),
			  totem);
	g_signal_connect (G_OBJECT (totem->playlist),
			  "current-removed", G_CALLBACK (current_removed_cb),
			  totem);
	g_signal_connect (G_OBJECT (totem->playlist),
			  "repeat-toggled",
			  G_CALLBACK (playlist_repeat_toggle_cb),
			  totem);
	g_signal_connect (G_OBJECT (totem->playlist),
			  "shuffle-toggled",
			  G_CALLBACK (playlist_shuffle_toggle_cb),
			  totem);
	g_signal_connect (G_OBJECT (totem->playlist),
			  "subtitle-changed",
			  G_CALLBACK (subtitle_changed_cb),
			  totem);
}

void
video_widget_create (Totem *totem)
{
	GError *err = NULL;
	GtkContainer *container;
	BaconVideoWidget **bvw;

	totem->bvw = BACON_VIDEO_WIDGET
		(bacon_video_widget_new (-1, -1, BVW_USE_TYPE_VIDEO, &err));

	if (totem->bvw == NULL) {
		totem_action_error_and_exit (_("Totem could not startup."), err != NULL ? err->message : _("No reason."), totem);
		if (err != NULL)
			g_error_free (err);
	}

	totem_action_zoom (totem, ZOOM_RESET);

	g_signal_connect_after (G_OBJECT (totem->bvw),
			"button-press-event",
			G_CALLBACK (on_video_button_press_event),
			totem);
	g_signal_connect (G_OBJECT (totem->bvw),
			"eos",
			G_CALLBACK (on_eos_event),
			totem);
	g_signal_connect (G_OBJECT (totem->bvw),
			"got-redirect",
			G_CALLBACK (on_got_redirect),
			totem);
	g_signal_connect (G_OBJECT(totem->bvw),
			"channels-change",
			G_CALLBACK (on_channels_change_event),
			totem);
	g_signal_connect (G_OBJECT (totem->bvw),
			"tick",
			G_CALLBACK (update_current_time),
			totem);
	g_signal_connect (G_OBJECT (totem->bvw),
			"got-metadata",
			G_CALLBACK (on_got_metadata_event),
			totem);
	g_signal_connect (G_OBJECT (totem->bvw),
			"buffering",
			G_CALLBACK (on_buffering_event),
			totem);
	g_signal_connect (G_OBJECT (totem->bvw),
			"download-buffering",
			G_CALLBACK (on_download_buffering_event),
			totem);
	g_signal_connect (G_OBJECT (totem->bvw),
			"error",
			G_CALLBACK (on_error_event),
			totem);

	container = GTK_CONTAINER (gtk_builder_get_object (totem->xml, "tmw_bvw_box"));
	gtk_container_add (container,
			GTK_WIDGET (totem->bvw));

	/* Events for the widget video window as well */
	gtk_widget_add_events (GTK_WIDGET (totem->bvw),
			GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
	g_signal_connect (G_OBJECT(totem->bvw), "key_press_event",
			G_CALLBACK (window_key_press_event_cb), totem);
	g_signal_connect (G_OBJECT(totem->bvw), "key_release_event",
			G_CALLBACK (window_key_press_event_cb), totem);

	g_signal_connect (G_OBJECT (totem->bvw), "drag_data_received",
			G_CALLBACK (drop_video_cb), totem);
	g_signal_connect (G_OBJECT (totem->bvw), "drag_motion",
			G_CALLBACK (drag_motion_video_cb), totem);
	gtk_drag_dest_set (GTK_WIDGET (totem->bvw), GTK_DEST_DEFAULT_ALL,
			target_table, G_N_ELEMENTS (target_table),
			GDK_ACTION_COPY | GDK_ACTION_MOVE);

	g_signal_connect (G_OBJECT (totem->bvw), "drag_data_get",
			G_CALLBACK (drag_video_cb), totem);

	bvw = &(totem->bvw);
	g_object_add_weak_pointer (G_OBJECT (totem->bvw),
				   (gpointer *) bvw);

	gtk_widget_realize (GTK_WIDGET (totem->bvw));
	gtk_widget_show (GTK_WIDGET (totem->bvw));

	totem_preferences_visuals_setup (totem);

	g_signal_connect (G_OBJECT (totem->bvw), "notify::volume",
			G_CALLBACK (property_notify_cb_volume), totem);
	g_signal_connect (G_OBJECT (totem->bvw), "notify::logo-mode",
			G_CALLBACK (property_notify_cb_logo_mode), totem);
	g_signal_connect (G_OBJECT (totem->bvw), "notify::seekable",
			G_CALLBACK (property_notify_cb_seekable), totem);
	update_volume_sliders (totem);
}
