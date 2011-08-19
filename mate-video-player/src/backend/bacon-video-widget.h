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

#ifndef HAVE_BACON_VIDEO_WIDGET_H
#define HAVE_BACON_VIDEO_WIDGET_H

#include <gtk/gtk.h>
/* for optical disc enumeration type */
#include <totem-disc.h>

G_BEGIN_DECLS

#define BACON_TYPE_VIDEO_WIDGET		     (bacon_video_widget_get_type ())
#define BACON_VIDEO_WIDGET(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), bacon_video_widget_get_type (), BaconVideoWidget))
#define BACON_VIDEO_WIDGET_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), bacon_video_widget_get_type (), BaconVideoWidgetClass))
#define BACON_IS_VIDEO_WIDGET(obj)           (G_TYPE_CHECK_INSTANCE_TYPE (obj, bacon_video_widget_get_type ()))
#define BACON_IS_VIDEO_WIDGET_CLASS(klass)   (G_CHECK_INSTANCE_GET_CLASS ((klass), bacon_video_widget_get_type ()))
#define BVW_ERROR bacon_video_widget_error_quark ()

typedef struct BaconVideoWidgetPrivate BaconVideoWidgetPrivate;
typedef struct BaconVideoWidgetCommon BaconVideoWidgetCommon;

/**
 * BaconVideoWidget:
 *
 * All the fields in the #BaconVideoWidget structure are private and should never be accessed directly.
 **/
typedef struct {
	/*< private >*/
	GtkEventBox parent;
	BaconVideoWidgetPrivate *priv;
} BaconVideoWidget;

/**
 * BaconVideoWidgetClass:
 *
 * All the fields in the #BaconVideoWidgetClass structure are private and should never be accessed directly.
 **/
typedef struct {
	/*< private >*/
	GtkEventBoxClass parent_class;

	void (*error) (GtkWidget *bvw, const char *message,
                       gboolean playback_stopped, gboolean fatal);
	void (*eos) (GtkWidget *bvw);
	void (*got_metadata) (GtkWidget *bvw);
	void (*got_redirect) (GtkWidget *bvw, const char *mrl);
	void (*title_change) (GtkWidget *bvw, const char *title);
	void (*channels_change) (GtkWidget *bvw);
	void (*tick) (GtkWidget *bvw, gint64 current_time, gint64 stream_length,
			double current_position, gboolean seekable);
	void (*buffering) (GtkWidget *bvw, guint progress);
	void (*download_buffering) (GtkWidget *bvw, gdouble percentage);
} BaconVideoWidgetClass;

/**
 * BvwError:
 * @BVW_ERROR_AUDIO_PLUGIN: Error loading audio output plugin or device.
 * @BVW_ERROR_NO_PLUGIN_FOR_FILE: A required GStreamer plugin or xine feature is missing.
 * @BVW_ERROR_VIDEO_PLUGIN: Error loading video output plugin or device.
 * @BVW_ERROR_AUDIO_BUSY: Audio output device is busy.
 * @BVW_ERROR_BROKEN_FILE: The movie file is broken and cannot be decoded.
 * @BVW_ERROR_FILE_GENERIC: A generic error for problems with movie files.
 * @BVW_ERROR_FILE_PERMISSION: Permission was refused to access the stream, or authentication was required.
 * @BVW_ERROR_FILE_ENCRYPTED: The stream is encrypted and cannot be played.
 * @BVW_ERROR_FILE_NOT_FOUND: The stream cannot be found.
 * @BVW_ERROR_DVD_ENCRYPTED: The DVD is encrypted and libdvdcss is not installed.
 * @BVW_ERROR_INVALID_DEVICE: The device given in an MRL (e.g. DVD drive or DVB tuner) did not exist.
 * @BVW_ERROR_DEVICE_BUSY: The device was busy.
 * @BVW_ERROR_UNKNOWN_HOST: The host for a given stream could not be resolved.
 * @BVW_ERROR_NETWORK_UNREACHABLE: The host for a given stream could not be reached.
 * @BVW_ERROR_CONNECTION_REFUSED: The server for a given stream refused the connection.
 * @BVW_ERROR_INVALID_LOCATION: An MRL was malformed, or CDDB playback was attempted (which is now unsupported).
 * @BVW_ERROR_GENERIC: A generic error occurred.
 * @BVW_ERROR_CODEC_NOT_HANDLED: The audio or video codec required by the stream is not supported.
 * @BVW_ERROR_AUDIO_ONLY: An audio-only stream could not be played due to missing audio output support.
 * @BVW_ERROR_CANNOT_CAPTURE: Error determining frame capture support for a video with bacon_video_widget_can_get_frames().
 * @BVW_ERROR_READ_ERROR: A generic error for problems reading streams.
 * @BVW_ERROR_PLUGIN_LOAD: A library or plugin could not be loaded.
 * @BVW_ERROR_EMPTY_FILE: A movie file was empty.
 *
 * Error codes for #BaconVideoWidget operations.
 **/
typedef enum {
	/* Plugins */
	BVW_ERROR_AUDIO_PLUGIN,
	BVW_ERROR_NO_PLUGIN_FOR_FILE,
	BVW_ERROR_VIDEO_PLUGIN,
	BVW_ERROR_AUDIO_BUSY,
	/* File */
	BVW_ERROR_BROKEN_FILE,
	BVW_ERROR_FILE_GENERIC,
	BVW_ERROR_FILE_PERMISSION,
	BVW_ERROR_FILE_ENCRYPTED,
	BVW_ERROR_FILE_NOT_FOUND,
	/* Devices */
	BVW_ERROR_DVD_ENCRYPTED,
	BVW_ERROR_INVALID_DEVICE,
	BVW_ERROR_DEVICE_BUSY,
	/* Network */
	BVW_ERROR_UNKNOWN_HOST,
	BVW_ERROR_NETWORK_UNREACHABLE,
	BVW_ERROR_CONNECTION_REFUSED,
	/* Generic */
	BVW_ERROR_INVALID_LOCATION,
	BVW_ERROR_GENERIC,
	BVW_ERROR_CODEC_NOT_HANDLED,
	BVW_ERROR_AUDIO_ONLY,
	BVW_ERROR_CANNOT_CAPTURE,
	BVW_ERROR_READ_ERROR,
	BVW_ERROR_PLUGIN_LOAD,
	BVW_ERROR_EMPTY_FILE
} BvwError;

GQuark bacon_video_widget_error_quark		 (void) G_GNUC_CONST;
GType bacon_video_widget_get_type                (void);
GOptionGroup* bacon_video_widget_get_option_group (void);
/* This can be used if the app does not use popt */
void bacon_video_widget_init_backend		 (int *argc, char ***argv);

/**
 * BvwUseType:
 * @BVW_USE_TYPE_VIDEO: fully-featured with video, audio, capture and metadata support
 * @BVW_USE_TYPE_AUDIO: audio and metadata support
 * @BVW_USE_TYPE_CAPTURE: capture support only
 * @BVW_USE_TYPE_METADATA: metadata support only
 *
 * The purpose for which a #BaconVideoWidget will be used, as specified to
 * bacon_video_widget_new(). This determines which features will be enabled
 * in the created widget.
 **/
typedef enum {
	BVW_USE_TYPE_VIDEO,
	BVW_USE_TYPE_AUDIO,
	BVW_USE_TYPE_CAPTURE,
	BVW_USE_TYPE_METADATA
} BvwUseType;

GtkWidget *bacon_video_widget_new		 (int width, int height,
						  BvwUseType type,
						  GError **error);

char *bacon_video_widget_get_backend_name (BaconVideoWidget *bvw);

/* Actions */
gboolean bacon_video_widget_open		 (BaconVideoWidget *bvw,
						  const char *mrl,
						  const char *subtitle_uri,
						  GError **error);
gboolean bacon_video_widget_play                 (BaconVideoWidget *bvw,
						  GError **error);
void bacon_video_widget_pause			 (BaconVideoWidget *bvw);
gboolean bacon_video_widget_is_playing           (BaconVideoWidget *bvw);

/* Seeking and length */
gboolean bacon_video_widget_is_seekable          (BaconVideoWidget *bvw);
gboolean bacon_video_widget_seek		 (BaconVideoWidget *bvw,
						  double position,
						  GError **error);
gboolean bacon_video_widget_seek_time		 (BaconVideoWidget *bvw,
						  gint64 _time,
						  gboolean accurate,
						  GError **error);
gboolean bacon_video_widget_step		 (BaconVideoWidget *bvw,
						  gboolean forward,
						  GError **error);
gboolean bacon_video_widget_can_direct_seek	 (BaconVideoWidget *bvw);
double bacon_video_widget_get_position           (BaconVideoWidget *bvw);
gint64 bacon_video_widget_get_current_time       (BaconVideoWidget *bvw);
gint64 bacon_video_widget_get_stream_length      (BaconVideoWidget *bvw);

void bacon_video_widget_stop                     (BaconVideoWidget *bvw);
void bacon_video_widget_close                    (BaconVideoWidget *bvw);

/* Audio volume */
gboolean bacon_video_widget_can_set_volume       (BaconVideoWidget *bvw);
void bacon_video_widget_set_volume               (BaconVideoWidget *bvw,
						  double volume);
double bacon_video_widget_get_volume             (BaconVideoWidget *bvw);

/* Properties */
void bacon_video_widget_set_logo		 (BaconVideoWidget *bvw,
						  const char *name);
void  bacon_video_widget_set_logo_mode		 (BaconVideoWidget *bvw,
						  gboolean logo_mode);
gboolean bacon_video_widget_get_logo_mode	 (BaconVideoWidget *bvw);

void bacon_video_widget_set_fullscreen		 (BaconVideoWidget *bvw,
						  gboolean fullscreen);

void bacon_video_widget_set_show_cursor          (BaconVideoWidget *bvw,
						  gboolean show_cursor);
gboolean bacon_video_widget_get_show_cursor      (BaconVideoWidget *bvw);

gboolean bacon_video_widget_get_auto_resize	 (BaconVideoWidget *bvw);
void bacon_video_widget_set_auto_resize		 (BaconVideoWidget *bvw,
						  gboolean auto_resize);

void bacon_video_widget_set_connection_speed     (BaconVideoWidget *bvw,
						  int speed);
int bacon_video_widget_get_connection_speed      (BaconVideoWidget *bvw);

gchar **bacon_video_widget_get_mrls		 (BaconVideoWidget *bvw,
						  TotemDiscMediaType type,
						  const char *device,
						  GError **error);
void bacon_video_widget_set_subtitle_font	 (BaconVideoWidget *bvw,
						  const char *font);
void bacon_video_widget_set_subtitle_encoding	 (BaconVideoWidget *bvw,
						  const char *encoding);

void bacon_video_widget_set_user_agent           (BaconVideoWidget *bvw,
                                                  const char *user_agent);

void bacon_video_widget_set_referrer             (BaconVideoWidget *bvw,
                                                  const char *referrer);

/* Metadata */
/**
 * BvwMetadataType:
 * @BVW_INFO_TITLE: the stream's title
 * @BVW_INFO_ARTIST: the artist who created the work
 * @BVW_INFO_YEAR: the year in which the work was created
 * @BVW_INFO_COMMENT: a comment attached to the stream
 * @BVW_INFO_ALBUM: the album in which the work was released
 * @BVW_INFO_DURATION: the stream's duration, in seconds
 * @BVW_INFO_TRACK_NUMBER: the track number of the work on the album
 * @BVW_INFO_COVER: a #GdkPixbuf of the cover artwork
 * @BVW_INFO_HAS_VIDEO: whether the stream has video
 * @BVW_INFO_DIMENSION_X: the video's width, in pixels
 * @BVW_INFO_DIMENSION_Y: the video's height, in pixels
 * @BVW_INFO_VIDEO_BITRATE: the video's bitrate, in kilobits per second
 * @BVW_INFO_VIDEO_CODEC: the video's codec
 * @BVW_INFO_FPS: the number of frames per second in the video
 * @BVW_INFO_HAS_AUDIO: whether the stream has audio
 * @BVW_INFO_AUDIO_BITRATE: the audio's bitrate, in kilobits per second
 * @BVW_INFO_AUDIO_CODEC: the audio's codec
 * @BVW_INFO_AUDIO_SAMPLE_RATE: the audio sample rate, in bits per second
 * @BVW_INFO_AUDIO_CHANNELS: a string describing the number of audio channels in the stream
 *
 * The different metadata available for querying from a #BaconVideoWidget
 * stream with bacon_video_widget_get_metadata().
 **/
typedef enum {
	BVW_INFO_TITLE,
	BVW_INFO_ARTIST,
	BVW_INFO_YEAR,
	BVW_INFO_COMMENT,
	BVW_INFO_ALBUM,
	BVW_INFO_DURATION,
	BVW_INFO_TRACK_NUMBER,
	BVW_INFO_COVER,
	/* Video */
	BVW_INFO_HAS_VIDEO,
	BVW_INFO_DIMENSION_X,
	BVW_INFO_DIMENSION_Y,
	BVW_INFO_VIDEO_BITRATE,
	BVW_INFO_VIDEO_CODEC,
	BVW_INFO_FPS,
	/* Audio */
	BVW_INFO_HAS_AUDIO,
	BVW_INFO_AUDIO_BITRATE,
	BVW_INFO_AUDIO_CODEC,
	BVW_INFO_AUDIO_SAMPLE_RATE,
	BVW_INFO_AUDIO_CHANNELS
} BvwMetadataType;

void bacon_video_widget_get_metadata		 (BaconVideoWidget *bvw,
						  BvwMetadataType type,
						  GValue *value);

/* Visualisation functions */
/**
 * BvwVisualsQuality:
 * @VISUAL_SMALL: small size (240×15)
 * @VISUAL_NORMAL: normal size (320×25)
 * @VISUAL_LARGE: large size (480×25)
 * @VISUAL_EXTRA_LARGE: extra large size (600×30)
 * @NUM_VISUAL_QUALITIES: the number of visual qualities available
 *
 * The different visualisation sizes or qualities available for use
 * with bacon_video_widget_set_visuals_quality().
 **/
typedef enum {
	VISUAL_SMALL = 0,
	VISUAL_NORMAL,
	VISUAL_LARGE,
	VISUAL_EXTRA_LARGE,
	NUM_VISUAL_QUALITIES
} BvwVisualsQuality;

void bacon_video_widget_set_show_visuals	  (BaconVideoWidget *bvw,
						   gboolean show_visuals);
GList *bacon_video_widget_get_visuals_list	  (BaconVideoWidget *bvw);
gboolean bacon_video_widget_set_visuals		  (BaconVideoWidget *bvw,
						   const char *name);
void bacon_video_widget_set_visuals_quality	  (BaconVideoWidget *bvw,
						   BvwVisualsQuality quality);

/* Picture settings */
/**
 * BvwVideoProperty:
 * @BVW_VIDEO_BRIGHTNESS: the video brightness
 * @BVW_VIDEO_CONTRAST: the video contrast
 * @BVW_VIDEO_SATURATION: the video saturation
 * @BVW_VIDEO_HUE: the video hue
 *
 * The video properties queryable with bacon_video_widget_get_video_property(),
 * and settable with bacon_video_widget_set_video_property().
 **/
typedef enum {
	BVW_VIDEO_BRIGHTNESS,
	BVW_VIDEO_CONTRAST,
	BVW_VIDEO_SATURATION,
	BVW_VIDEO_HUE
} BvwVideoProperty;

/**
 * BvwAspectRatio:
 * @BVW_RATIO_AUTO: automatic
 * @BVW_RATIO_SQUARE: square (1:1)
 * @BVW_RATIO_FOURBYTHREE: four-by-three (4:3)
 * @BVW_RATIO_ANAMORPHIC: anamorphic (16:9)
 * @BVW_RATIO_DVB: DVB (20:9)
 *
 * The pixel aspect ratios available in which to display videos using
 * @bacon_video_widget_set_aspect_ratio().
 **/
typedef enum {
	BVW_RATIO_AUTO = 0,
	BVW_RATIO_SQUARE = 1,
	BVW_RATIO_FOURBYTHREE = 2,
	BVW_RATIO_ANAMORPHIC = 3,
	BVW_RATIO_DVB = 4
} BvwAspectRatio;

void bacon_video_widget_set_deinterlacing        (BaconVideoWidget *bvw,
						  gboolean deinterlace);
gboolean bacon_video_widget_get_deinterlacing    (BaconVideoWidget *bvw);

void bacon_video_widget_set_aspect_ratio         (BaconVideoWidget *bvw,
						  BvwAspectRatio ratio);
BvwAspectRatio bacon_video_widget_get_aspect_ratio
						 (BaconVideoWidget *bvw);

void bacon_video_widget_set_scale_ratio          (BaconVideoWidget *bvw,
						  float ratio);

void bacon_video_widget_set_zoom		 (BaconVideoWidget *bvw,
						  double zoom);
double bacon_video_widget_get_zoom		 (BaconVideoWidget *bvw);

int bacon_video_widget_get_video_property        (BaconVideoWidget *bvw,
						  BvwVideoProperty type);
void bacon_video_widget_set_video_property       (BaconVideoWidget *bvw,
						  BvwVideoProperty type,
						  int value);

gboolean bacon_video_widget_has_menus            (BaconVideoWidget *bvw);

/* DVD functions */
/**
 * BvwDVDEvent:
 * @BVW_DVD_ROOT_MENU: root menu
 * @BVW_DVD_TITLE_MENU: title menu
 * @BVW_DVD_SUBPICTURE_MENU: subpicture menu (if available)
 * @BVW_DVD_AUDIO_MENU: audio menu (if available)
 * @BVW_DVD_ANGLE_MENU: angle menu (if available)
 * @BVW_DVD_CHAPTER_MENU: chapter menu
 * @BVW_DVD_NEXT_CHAPTER: the next chapter
 * @BVW_DVD_PREV_CHAPTER: the previous chapter
 * @BVW_DVD_NEXT_TITLE: the next title in the current chapter
 * @BVW_DVD_PREV_TITLE: the previous title in the current chapter
 * @BVW_DVD_NEXT_ANGLE: the next angle
 * @BVW_DVD_PREV_ANGLE: the previous angle
 * @BVW_DVD_ROOT_MENU_UP: go up in the menu
 * @BVW_DVD_ROOT_MENU_DOWN: go down in the menu
 * @BVW_DVD_ROOT_MENU_LEFT: go left in the menu
 * @BVW_DVD_ROOT_MENU_RIGHT: go right in the menu
 * @BVW_DVD_ROOT_MENU_SELECT: select the current menu entry
 *
 * The DVD navigation actions available to fire as DVD events to
 * the #BaconVideoWidget.
 **/
typedef enum {
	BVW_DVD_ROOT_MENU,
	BVW_DVD_TITLE_MENU,
	BVW_DVD_SUBPICTURE_MENU,
	BVW_DVD_AUDIO_MENU,
	BVW_DVD_ANGLE_MENU,
	BVW_DVD_CHAPTER_MENU,
	BVW_DVD_NEXT_CHAPTER,
	BVW_DVD_PREV_CHAPTER,
	BVW_DVD_NEXT_TITLE,
	BVW_DVD_PREV_TITLE,
	BVW_DVD_NEXT_ANGLE,
	BVW_DVD_PREV_ANGLE,
	BVW_DVD_ROOT_MENU_UP,
	BVW_DVD_ROOT_MENU_DOWN,
	BVW_DVD_ROOT_MENU_LEFT,
	BVW_DVD_ROOT_MENU_RIGHT,
	BVW_DVD_ROOT_MENU_SELECT
} BvwDVDEvent;

void bacon_video_widget_dvd_event                (BaconVideoWidget *bvw,
						  BvwDVDEvent type);
GList *bacon_video_widget_get_languages          (BaconVideoWidget *bvw);
int bacon_video_widget_get_language              (BaconVideoWidget *bvw);
void bacon_video_widget_set_language             (BaconVideoWidget *bvw,
		                                  int language);

GList *bacon_video_widget_get_subtitles          (BaconVideoWidget *bvw);
int bacon_video_widget_get_subtitle              (BaconVideoWidget *bvw);
void bacon_video_widget_set_subtitle             (BaconVideoWidget *bvw,
		                                  int subtitle);

gboolean bacon_video_widget_has_next_track	 (BaconVideoWidget *bvw);
gboolean bacon_video_widget_has_previous_track	 (BaconVideoWidget *bvw);

/* Screenshot functions */
gboolean bacon_video_widget_can_get_frames       (BaconVideoWidget *bvw,
						  GError **error);
GdkPixbuf *bacon_video_widget_get_current_frame (BaconVideoWidget *bvw);

/* Audio-out functions */
/**
 * BvwAudioOutType:
 * @BVW_AUDIO_SOUND_STEREO: stereo output
 * @BVW_AUDIO_SOUND_4CHANNEL: 4-channel output
 * @BVW_AUDIO_SOUND_41CHANNEL: 4.1-channel output
 * @BVW_AUDIO_SOUND_5CHANNEL: 5-channel output
 * @BVW_AUDIO_SOUND_51CHANNEL: 5.1-channel output
 * @BVW_AUDIO_SOUND_AC3PASSTHRU: AC3 passthrough output
 *
 * The audio output types available for use with bacon_video_widget_set_audio_out_type().
 **/
typedef enum {
	BVW_AUDIO_SOUND_STEREO,
	BVW_AUDIO_SOUND_4CHANNEL,
	BVW_AUDIO_SOUND_41CHANNEL,
	BVW_AUDIO_SOUND_5CHANNEL,
	BVW_AUDIO_SOUND_51CHANNEL,
	BVW_AUDIO_SOUND_AC3PASSTHRU
} BvwAudioOutType;

BvwAudioOutType bacon_video_widget_get_audio_out_type
						 (BaconVideoWidget *bvw);
gboolean bacon_video_widget_set_audio_out_type   (BaconVideoWidget *bvw,
						  BvwAudioOutType type);

G_END_DECLS

#endif				/* HAVE_BACON_VIDEO_WIDGET_H */
