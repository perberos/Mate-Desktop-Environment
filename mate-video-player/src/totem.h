/* 
 * Copyright (C) 2001,2002,2003,2004,2005 Bastien Nocera <hadess@hadess.net>
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

#ifndef __TOTEM_H__
#define __TOTEM_H__

#include <glib-object.h>
#include <gtk/gtk.h>
#include <totem-disc.h>
#include "totem-playlist.h"

/**
 * TOTEM_MATECONF_PREFIX:
 *
 * The MateConf prefix under which all Totem MateConf keys are stored.
 **/
#define TOTEM_MATECONF_PREFIX "/apps/totem"

G_BEGIN_DECLS

/**
 * TotemRemoteCommand:
 * @TOTEM_REMOTE_COMMAND_UNKNOWN: unknown command
 * @TOTEM_REMOTE_COMMAND_PLAY: play the current stream
 * @TOTEM_REMOTE_COMMAND_PAUSE: pause the current stream
 * @TOTEM_REMOTE_COMMAND_STOP: stop playing the current stream
 * @TOTEM_REMOTE_COMMAND_PLAYPAUSE: toggle play/pause on the current stream
 * @TOTEM_REMOTE_COMMAND_NEXT: play the next playlist item
 * @TOTEM_REMOTE_COMMAND_PREVIOUS: play the previous playlist item
 * @TOTEM_REMOTE_COMMAND_SEEK_FORWARD: seek forwards in the current stream
 * @TOTEM_REMOTE_COMMAND_SEEK_BACKWARD: seek backwards in the current stream
 * @TOTEM_REMOTE_COMMAND_VOLUME_UP: increase the volume
 * @TOTEM_REMOTE_COMMAND_VOLUME_DOWN: decrease the volume
 * @TOTEM_REMOTE_COMMAND_FULLSCREEN: toggle fullscreen mode
 * @TOTEM_REMOTE_COMMAND_QUIT: quit the instance of Totem
 * @TOTEM_REMOTE_COMMAND_ENQUEUE: enqueue a new playlist item
 * @TOTEM_REMOTE_COMMAND_REPLACE: replace an item in the playlist
 * @TOTEM_REMOTE_COMMAND_SHOW: show the Totem instance
 * @TOTEM_REMOTE_COMMAND_TOGGLE_CONTROLS: toggle the control visibility
 * @TOTEM_REMOTE_COMMAND_UP: go up (DVD controls)
 * @TOTEM_REMOTE_COMMAND_DOWN: go down (DVD controls)
 * @TOTEM_REMOTE_COMMAND_LEFT: go left (DVD controls)
 * @TOTEM_REMOTE_COMMAND_RIGHT: go right (DVD controls)
 * @TOTEM_REMOTE_COMMAND_SELECT: select the current item (DVD controls)
 * @TOTEM_REMOTE_COMMAND_DVD_MENU: go to the DVD menu
 * @TOTEM_REMOTE_COMMAND_ZOOM_UP: increase the zoom level
 * @TOTEM_REMOTE_COMMAND_ZOOM_DOWN: decrease the zoom level
 * @TOTEM_REMOTE_COMMAND_EJECT: eject the current disc
 * @TOTEM_REMOTE_COMMAND_PLAY_DVD: play a DVD in a drive
 * @TOTEM_REMOTE_COMMAND_MUTE: toggle mute
 * @TOTEM_REMOTE_COMMAND_TOGGLE_ASPECT: toggle the aspect ratio
 *
 * Represents a command which can be sent to a running Totem instance remotely.
 **/
typedef enum {
	TOTEM_REMOTE_COMMAND_UNKNOWN = 0,
	TOTEM_REMOTE_COMMAND_PLAY,
	TOTEM_REMOTE_COMMAND_PAUSE,
	TOTEM_REMOTE_COMMAND_STOP,
	TOTEM_REMOTE_COMMAND_PLAYPAUSE,
	TOTEM_REMOTE_COMMAND_NEXT,
	TOTEM_REMOTE_COMMAND_PREVIOUS,
	TOTEM_REMOTE_COMMAND_SEEK_FORWARD,
	TOTEM_REMOTE_COMMAND_SEEK_BACKWARD,
	TOTEM_REMOTE_COMMAND_VOLUME_UP,
	TOTEM_REMOTE_COMMAND_VOLUME_DOWN,
	TOTEM_REMOTE_COMMAND_FULLSCREEN,
	TOTEM_REMOTE_COMMAND_QUIT,
	TOTEM_REMOTE_COMMAND_ENQUEUE,
	TOTEM_REMOTE_COMMAND_REPLACE,
	TOTEM_REMOTE_COMMAND_SHOW,
	TOTEM_REMOTE_COMMAND_TOGGLE_CONTROLS,
	TOTEM_REMOTE_COMMAND_UP,
	TOTEM_REMOTE_COMMAND_DOWN,
	TOTEM_REMOTE_COMMAND_LEFT,
	TOTEM_REMOTE_COMMAND_RIGHT,
	TOTEM_REMOTE_COMMAND_SELECT,
	TOTEM_REMOTE_COMMAND_DVD_MENU,
	TOTEM_REMOTE_COMMAND_ZOOM_UP,
	TOTEM_REMOTE_COMMAND_ZOOM_DOWN,
	TOTEM_REMOTE_COMMAND_EJECT,
	TOTEM_REMOTE_COMMAND_PLAY_DVD,
	TOTEM_REMOTE_COMMAND_MUTE,
	TOTEM_REMOTE_COMMAND_TOGGLE_ASPECT
} TotemRemoteCommand;

/**
 * TotemRemoteSetting:
 * @TOTEM_REMOTE_SETTING_SHUFFLE: whether shuffle is enabled
 * @TOTEM_REMOTE_SETTING_REPEAT: whether repeat is enabled
 *
 * Represents a boolean setting or preference on a remote Totem instance.
 **/
typedef enum {
	TOTEM_REMOTE_SETTING_SHUFFLE,
	TOTEM_REMOTE_SETTING_REPEAT
} TotemRemoteSetting;

GType totem_remote_command_get_type	(void);
GQuark totem_remote_command_quark	(void);
#define TOTEM_TYPE_REMOTE_COMMAND	(totem_remote_command_get_type())
#define TOTEM_REMOTE_COMMAND		totem_remote_command_quark ()

GType totem_remote_setting_get_type	(void);
GQuark totem_remote_setting_quark	(void);
#define TOTEM_TYPE_REMOTE_SETTING	(totem_remote_setting_get_type())
#define TOTEM_REMOTE_SETTING		totem_remote_setting_quark ()

#define TOTEM_TYPE_OBJECT              (totem_object_get_type ())
#define TOTEM_OBJECT(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), totem_object_get_type (), TotemObject))
#define TOTEM_OBJECT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), totem_object_get_type (), TotemObjectClass))
#define TOTEM_IS_OBJECT(obj)           (G_TYPE_CHECK_INSTANCE_TYPE (obj, totem_object_get_type ()))
#define TOTEM_IS_OBJECT_CLASS(klass)   (G_CHECK_INSTANCE_GET_CLASS ((klass), totem_object_get_type ()))

/**
 * Totem:
 *
 * The #Totem object is a handy synonym for #TotemObject, and the two can be used interchangably.
 **/

/**
 * TotemObject:
 *
 * All the fields in the #TotemObject structure are private and should never be accessed directly.
 **/
typedef struct _TotemObject Totem;
typedef struct _TotemObject TotemObject;

/**
 * TotemObjectClass:
 * @parent_class: the parent class
 * @file_opened: the generic signal handler for the #TotemObject::file-opened signal,
 * which can be overridden by inheriting classes
 * @file_closed: the generic signal handler for the #TotemObject::file-closed signal,
 * which can be overridden by inheriting classes
 * @metadata_updated: the generic signal handler for the #TotemObject::metadata-updated signal,
 * which can be overridden by inheriting classes
 *
 * The class structure for the #TotemPlParser type.
 **/
typedef struct {
	GObjectClass parent_class;

	void (*file_opened)			(Totem *totem, const char *mrl);
	void (*file_closed)			(Totem *totem);
	void (*metadata_updated)		(Totem *totem,
						 const char *artist,
						 const char *title,
						 const char *album,
						 guint track_num);
} TotemObjectClass;

GType	totem_object_get_type			(void);
void    totem_object_plugins_init		(TotemObject *totem);
void    totem_object_plugins_shutdown		(void);
void	totem_file_opened			(TotemObject *totem,
						 const char *mrl);
void	totem_file_closed			(TotemObject *totem);
void	totem_metadata_updated			(TotemObject *totem,
						 const char *artist,
						 const char *title,
						 const char *album,
						 guint track_num);

void	totem_action_exit			(Totem *totem) G_GNUC_NORETURN;
void	totem_action_play			(Totem *totem);
void	totem_action_stop			(Totem *totem);
void	totem_action_play_pause			(Totem *totem);
void	totem_action_pause			(Totem *totem);
void	totem_action_fullscreen_toggle		(Totem *totem);
void	totem_action_fullscreen			(Totem *totem, gboolean state);
void	totem_action_next			(Totem *totem);
void	totem_action_previous			(Totem *totem);
void	totem_action_seek_time			(Totem *totem, gint64 msec, gboolean accurate);
void	totem_action_seek_relative		(Totem *totem, gint64 offset, gboolean accurate);
double	totem_get_volume			(Totem *totem);
void	totem_action_volume			(Totem *totem, double volume);
void	totem_action_volume_relative		(Totem *totem, double off_pct);
void	totem_action_volume_toggle_mute		(Totem *totem);
gboolean totem_action_set_mrl			(Totem *totem,
						 const char *mrl,
						 const char *subtitle);
void	totem_action_set_mrl_and_play		(Totem *totem,
						 const char *mrl, 
						 const char *subtitle);

gboolean totem_action_set_mrl_with_warning	(Totem *totem,
						 const char *mrl,
						 const char *subtitle,
						 gboolean warn);

void	totem_action_play_media			(Totem *totem,
						 TotemDiscMediaType type,
						 const char *device);

void	totem_action_toggle_aspect_ratio	(Totem *totem);
void	totem_action_set_aspect_ratio		(Totem *totem, int ratio);
int	totem_action_get_aspect_ratio		(Totem *totem);
void	totem_action_toggle_controls		(Totem *totem);
void	totem_action_next_angle			(Totem *totem);

void	totem_action_set_scale_ratio		(Totem *totem, gfloat ratio);
void    totem_action_error                      (const char *title,
						 const char *reason,
						 Totem *totem);
void    totem_action_play_media_device		(Totem *totem,
						 const char *device);

gboolean totem_is_fullscreen			(Totem *totem);
gboolean totem_is_playing			(Totem *totem);
gboolean totem_is_paused			(Totem *totem);
gboolean totem_is_seekable			(Totem *totem);
GtkWindow *totem_get_main_window		(Totem *totem);
GtkUIManager *totem_get_ui_manager		(Totem *totem);
GtkWidget *totem_get_video_widget		(Totem *totem);
char *totem_get_video_widget_backend_name	(Totem *totem);
char *totem_get_version				(void);

/* Current media information */
char *	totem_get_short_title			(Totem *totem);
gint64	totem_get_current_time			(Totem *totem);

/* Playlist handling */
guint	totem_get_playlist_length		(Totem *totem);
void	totem_action_set_playlist_index		(Totem *totem,
						 guint index);
int	totem_get_playlist_pos			(Totem *totem);
char *	totem_get_title_at_playlist_pos		(Totem *totem,
						 guint playlist_index);
void totem_add_to_playlist_and_play		(Totem *totem,
						 const char *uri,
						 const char *display_name,
						 gboolean add_to_recent);
char *  totem_get_current_mrl			(Totem *totem);
void	totem_set_current_subtitle		(Totem *totem,
						 const char *subtitle_uri);
/* Sidebar handling */
void    totem_add_sidebar_page			(Totem *totem,
						 const char *page_id,
						 const char *title,
						 GtkWidget *main_widget);
void    totem_remove_sidebar_page		(Totem *totem,
						 const char *page_id);

/* Remote actions */
void    totem_action_remote			(Totem *totem,
						 TotemRemoteCommand cmd,
						 const char *url);
void	totem_action_remote_set_setting		(Totem *totem,
						 TotemRemoteSetting setting,
						 gboolean value);
gboolean totem_action_remote_get_setting	(Totem *totem,
						 TotemRemoteSetting setting);

#endif /* __TOTEM_H__ */
