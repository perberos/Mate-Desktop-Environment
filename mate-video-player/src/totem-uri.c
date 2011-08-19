/* totem-uri.c

   Copyright (C) 2004 Bastien Nocera

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

#include <glib.h>
#include <glib/gi18n.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <gio/gio.h>

#include "totem-mime-types.h"
#include "totem-uri.h"
#include "totem-private.h"

/* 5 minute threshold. We don't want to save the position within a 3
 * minute song for example. */
#define SAVE_POSITION_THRESHOLD 5 * 60 * 1000
/* Don't save the position of a stream if we're within 5% of the beginning or end so that,
 * for example, we don't save if the user exits when they reach the credits of a film */
#define SAVE_POSITION_END_THRESHOLD 0.05
/* The GIO file attribute used to store the position in a stream */
#define SAVE_POSITION_FILE_ATTRIBUTE "metadata::totem::position"

static GtkFileFilter *filter_all = NULL;
static GtkFileFilter *filter_subs = NULL;
static GtkFileFilter *filter_supported = NULL;
static GtkFileFilter *filter_audio = NULL;
static GtkFileFilter *filter_video = NULL;

gboolean
totem_playing_dvd (const char *uri)
{
	if (uri == NULL)
		return FALSE;

	return g_str_has_prefix (uri, "dvd:/");
}

static void
totem_ensure_dir (const char *path)
{
	if (g_file_test (path, G_FILE_TEST_IS_DIR) != FALSE)
		return;

	g_mkdir_with_parents (path, 0700);
}

const char *
totem_dot_dir (void)
{
	static char *totem_dir = NULL;

	if (totem_dir != NULL) {
		totem_ensure_dir (totem_dir);
		return totem_dir;
	}

	totem_dir = g_build_filename (g_get_user_config_dir (),
				      "totem",
				      NULL);

	totem_ensure_dir (totem_dir);

	return (const char *)totem_dir;
}

const char *
totem_data_dot_dir (void)
{
	static char *totem_dir = NULL;

	if (totem_dir != NULL) {
		totem_ensure_dir (totem_dir);
		return totem_dir;
	}

	totem_dir = g_build_filename (g_get_user_data_dir (),
				      "totem",
				      NULL);

	totem_ensure_dir (totem_dir);

	return (const char *)totem_dir;
}

char *
totem_pictures_dir (void)
{
	const char *dir;

	dir = g_get_user_special_dir (G_USER_DIRECTORY_PICTURES);
	if (dir == NULL)
		return NULL;
	return g_strdup (dir);
}

static GMount *
totem_get_mount_for_uri (const char *path)
{
	GMount *mount;
	GFile *file;

	file = g_file_new_for_path (path);
	mount = g_file_find_enclosing_mount (file, NULL, NULL);
	g_object_unref (file);

	if (mount == NULL)
		return NULL;

	/* FIXME: We used to explicitly check whether it was a CD/DVD */
	if (g_mount_can_eject (mount) == TRUE) {
		g_object_unref (mount);
		return NULL;
	}

	return mount;
}

static GMount *
totem_get_mount_for_dvd (const char *uri)
{
	GMount *mount;
	char *path;

	mount = NULL;
	path = g_strdup (uri + strlen ("dvd://"));

	/* If it's a device, we need to find the volume that corresponds to it,
	 * and then the mount for the volume */
	if (g_str_has_prefix (path, "/dev/")) {
		GVolumeMonitor *volume_monitor;
		GList *volumes, *l;

		volume_monitor = g_volume_monitor_get ();
		volumes = g_volume_monitor_get_volumes (volume_monitor);
		g_object_unref (volume_monitor);

		for (l = volumes; l != NULL; l = l->next) {
			char *id;

			id = g_volume_get_identifier (l->data, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
			if (g_strcmp0 (id, path) == 0) {
				g_free (id);
				mount = g_volume_get_mount (l->data);
				break;
			}
			g_free (id);
		}
		g_list_foreach (volumes, (GFunc) g_object_unref, NULL);
		g_list_free (volumes);
	} else {
		mount = totem_get_mount_for_uri (path);
		g_free (path);
	}
	/* We have a path to the file itself */
	return mount;
}

static char *
totem_get_mountpoint_for_vcd (const char *uri)
{
	return NULL;
}

GMount *
totem_get_mount_for_media (const char *uri)
{
	GMount *ret;
	char *mount_path;

	if (uri == NULL)
		return NULL;

	mount_path = NULL;

	if (g_str_has_prefix (uri, "dvd://") != FALSE)
		return totem_get_mount_for_dvd (uri);
	else if (g_str_has_prefix (uri, "vcd:") != FALSE)
		mount_path = totem_get_mountpoint_for_vcd (uri);
	else if (g_str_has_prefix (uri, "file:") != FALSE)
		mount_path = g_filename_from_uri (uri, NULL, NULL);

	if (mount_path == NULL)
		return NULL;

	ret = totem_get_mount_for_uri (mount_path);
	g_free (mount_path);

	return ret;
}

gboolean
totem_is_special_mrl (const char *uri)
{
	GMount *mount;

	if (uri == NULL || g_str_has_prefix (uri, "file:") != FALSE)
		return FALSE;
	if (g_str_has_prefix (uri, "dvb:") != FALSE)
		return TRUE;

	mount = totem_get_mount_for_media (uri);
	if (mount != NULL)
		g_object_unref (mount);

	return (mount != NULL);
}

gboolean
totem_is_block_device (const char *uri)
{
	struct stat buf;
	char *local;

	if (uri == NULL)
		return FALSE;

	if (g_str_has_prefix (uri, "file:") == FALSE)
		return FALSE;
	local = g_filename_from_uri (uri, NULL, NULL);
	if (local == NULL)
		return FALSE;
	if (stat (local, &buf) != 0) {
		g_free (local);
		return FALSE;
	}
	g_free (local);

	return (S_ISBLK (buf.st_mode));
}

char *
totem_create_full_path (const char *path)
{
	GFile *file;
	char *retval;

	g_return_val_if_fail (path != NULL, NULL);

	if (strstr (path, "://") != NULL)
		return NULL;
	if (totem_is_special_mrl (path) != FALSE)
		return NULL;

	file = g_file_new_for_commandline_arg (path);
	retval = g_file_get_uri (file);
	g_object_unref (file);

	return retval;
}

static void
totem_action_on_unmount (GVolumeMonitor *volume_monitor,
			 GMount *mount,
			 Totem *totem)
{
	totem_playlist_clear_with_g_mount (totem->playlist, mount);
}

void
totem_setup_file_monitoring (Totem *totem)
{
	totem->monitor = g_volume_monitor_get ();

	g_signal_connect (G_OBJECT (totem->monitor),
			  "mount-pre-unmount",
			  G_CALLBACK (totem_action_on_unmount),
			  totem);
}

/* List from xine-lib's demux_sputext.c */
static const char subtitle_ext[][4] = {
	"sub",
	"srt",
	"smi",
	"ssa",
	"ass",
	"asc"
};

gboolean
totem_uri_is_subtitle (const char *uri)
{
	guint len, i;

	len = strlen (uri);
	if (len < 4 || uri[len - 4] != '.')
		return FALSE;
	for (i = 0; i < G_N_ELEMENTS (subtitle_ext); i++) {
		if (g_str_has_suffix (uri, subtitle_ext[i]) != FALSE)
			return TRUE;
	}
	return FALSE;
}

static inline gboolean
totem_uri_exists (const char *uri)
{
	GFile *file = g_file_new_for_uri (uri);
	if (file != NULL) {
		if (g_file_query_exists (file, NULL)) {
			g_object_unref (file);
			return TRUE;
		}
		g_object_unref (file);
	}
	return FALSE;
}

static char *
totem_uri_get_subtitle_for_uri (const char *uri)
{
	char *subtitle;
	guint len, i;
	gint suffix;

	/* Find the filename suffix delimiter */
	len = strlen (uri);
	for (suffix = len - 1; suffix > 0; suffix--) {
		if (uri[suffix] == G_DIR_SEPARATOR ||
		    (uri[suffix] == '/')) {
			/* This filename has no extension; we'll need to 
			 * add one */
			suffix = len;
			break;
		}
		if (uri[suffix] == '.') {
			/* Found our extension marker */
			break;
		}
	}
	if (suffix < 0)
		return NULL;

	/* Generate a subtitle string with room at the end to store the
	 * 3 character extensions for which we want to search */
	subtitle = g_malloc0 (suffix + 4 + 1);
	g_return_val_if_fail (subtitle != NULL, NULL);
	g_strlcpy (subtitle, uri, suffix + 4 + 1);
	g_strlcpy (subtitle + suffix, ".???", 5);

	/* Search for any files with one of our known subtitle extensions */
	for (i = 0; i < G_N_ELEMENTS (subtitle_ext) ; i++) {
		char *subtitle_ext_upper;
		memcpy (subtitle + suffix + 1, subtitle_ext[i], 3);

		if (totem_uri_exists (subtitle))
			return subtitle;

		/* Check with upper-cased extension */
		subtitle_ext_upper = g_ascii_strup (subtitle_ext[i], -1);
		memcpy (subtitle + suffix + 1, subtitle_ext_upper, 3);
		g_free (subtitle_ext_upper);

		if (totem_uri_exists (subtitle))
			return subtitle;
	}
	g_free (subtitle);
	return NULL;
}

static char *
totem_uri_get_subtitle_in_subdir (GFile *file, const char *subdir)
{
	char *filename, *subtitle, *full_path_str;
	GFile *parent, *full_path, *directory;

	/* Get the sibling directory @subdir of the file @file */
	parent = g_file_get_parent (file);
	directory = g_file_get_child (parent, subdir);
	g_object_unref (parent);

	/* Get the file of the same name as @file in the @subdir directory */
	filename = g_file_get_basename (file);
	full_path = g_file_get_child (directory, filename);
	g_object_unref (directory);
	g_free (filename);

	/* Get the subtitles from that URI */
	full_path_str = g_file_get_uri (full_path);
	g_object_unref (full_path);
	subtitle = totem_uri_get_subtitle_for_uri (full_path_str);
	g_free (full_path_str);

	return subtitle;
}

static char *
totem_uri_get_cached_subtitle_for_uri (const char *uri)
{
	char *filename, *basename, *fake_filename, *fake_uri, *ret;

	filename = g_filename_from_uri (uri, NULL, NULL);
	if (filename == NULL)
		return NULL;

	basename = g_path_get_basename (filename);
	g_free (filename);
	if (basename == NULL || strcmp (basename, ".") == 0) {
		g_free (basename);
		return NULL;
	}

	fake_filename = g_build_filename (g_get_user_cache_dir (),
				"totem",
				"subtitles",
				basename,
				NULL);
	g_free (basename);
	fake_uri = g_filename_to_uri (fake_filename, NULL, NULL);
	g_free (fake_filename);

	ret = totem_uri_get_subtitle_for_uri (fake_uri);
	g_free (fake_uri);

	return ret;
}

char *
totem_uri_get_subtitle_uri (const char *uri)
{
	GFile *file;
	char *subtitle;

	if (g_str_has_prefix (uri, "http") != FALSE ||
	    g_str_has_prefix (uri, "rtsp") != FALSE ||
	    g_str_has_prefix (uri, "rtmp") != FALSE)
		return NULL;

	/* Has the user specified a subtitle file manually? */
	if (strstr (uri, "#subtitle:") != NULL)
		return NULL;

	/* Does the file exist? */
	file = g_file_new_for_uri (uri);
	if (g_file_query_exists (file, NULL) != TRUE) {
		g_object_unref (file);
		return NULL;
	}

	/* Try in the cached subtitles directory */
	subtitle = totem_uri_get_cached_subtitle_for_uri (uri);
	if (subtitle != NULL) {
		g_object_unref (file);
		return subtitle;
	}

	/* Try in the current directory */
	subtitle = totem_uri_get_subtitle_for_uri (uri);
	if (subtitle != NULL) {
		g_object_unref (file);
		return subtitle;
	}

	subtitle = totem_uri_get_subtitle_in_subdir (file, "subtitles");
	g_object_unref (file);

	return subtitle;
}

char *
totem_uri_escape_for_display (const char *uri)
{
	GFile *file;
	char *disp;

	file = g_file_new_for_uri (uri);
	disp = g_file_get_parse_name (file);
	g_object_unref (file);

	return disp;
}

void
totem_setup_file_filters (void)
{
	guint i;

	filter_all = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter_all, _("All files"));
	gtk_file_filter_add_pattern (filter_all, "*");
	g_object_ref_sink (filter_all);

	filter_supported = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter_supported, _("Supported files"));
	for (i = 0; i < G_N_ELEMENTS (mime_types); i++) {
		gtk_file_filter_add_mime_type (filter_supported, mime_types[i]);
	}

	/* Add the special Disc-as-files formats */
	gtk_file_filter_add_mime_type (filter_supported, "application/x-cd-image");
	gtk_file_filter_add_mime_type (filter_supported, "application/x-cue");
	g_object_ref_sink (filter_supported);

	/* Audio files */
	filter_audio = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter_audio, _("Audio files"));
	for (i = 0; i < G_N_ELEMENTS (audio_mime_types); i++) {
		gtk_file_filter_add_mime_type (filter_audio, audio_mime_types[i]);
	}
	g_object_ref_sink (filter_audio);

	/* Video files */
	filter_video = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter_video, _("Video files"));
	for (i = 0; i < G_N_ELEMENTS (video_mime_types); i++) {
		gtk_file_filter_add_mime_type (filter_video, video_mime_types[i]);
	}
	gtk_file_filter_add_mime_type (filter_video, "application/x-cd-image");
	gtk_file_filter_add_mime_type (filter_video, "application/x-cue");
	g_object_ref_sink (filter_video);

	/* Subtitles files */
	filter_subs = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter_subs, _("Subtitle files"));
	gtk_file_filter_add_mime_type (filter_subs, "application/x-subrip"); /* *.srt */
	gtk_file_filter_add_mime_type (filter_subs, "text/plain"); /* *.asc, *.txt */
	gtk_file_filter_add_mime_type (filter_subs, "application/x-sami"); /* *.smi, *.sami */
	gtk_file_filter_add_mime_type (filter_subs, "text/x-microdvd"); /* *.sub */
	gtk_file_filter_add_mime_type (filter_subs, "text/x-mpsub"); /* *.sub */
	gtk_file_filter_add_mime_type (filter_subs, "text/x-ssa"); /* *.ssa, *.ass */
	gtk_file_filter_add_mime_type (filter_subs, "text/x-subviewer"); /* *.sub */
	g_object_ref_sink (filter_subs);
}

void
totem_destroy_file_filters (void)
{
	if (filter_all != NULL) {
		g_object_unref (filter_all);
		filter_all = NULL;
		g_object_unref (filter_supported);
		g_object_unref (filter_audio);
		g_object_unref (filter_video);
		g_object_unref (filter_subs);
	}
}

static const GUserDirectory dir_types[] = {
	G_USER_DIRECTORY_VIDEOS,
	G_USER_DIRECTORY_MUSIC
};

static void
totem_add_default_dirs (GtkFileChooser *dialog)
{
	guint i;
	for (i = 0; i < G_N_ELEMENTS (dir_types); i++) {
		const char *dir;

		dir = g_get_user_special_dir (dir_types[i]);
		if (dir == NULL)
			continue;
		gtk_file_chooser_add_shortcut_folder (dialog, dir, NULL);
	}
}

char *
totem_add_subtitle (GtkWindow *parent, const char *uri)
{
	GtkWidget *fs;
	MateConfClient *conf;
	char *new_path;
	char *subtitle = NULL;
	gboolean folder_set;

	fs = gtk_file_chooser_dialog_new (_("Select Text Subtitles"), 
					  parent,
					  GTK_FILE_CHOOSER_ACTION_OPEN,
					  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					  GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					  NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (fs), GTK_RESPONSE_ACCEPT);
	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (fs), FALSE);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (fs), filter_all);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (fs), filter_subs);
	gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (fs), filter_subs);

	conf = mateconf_client_get_default ();
	folder_set = FALSE;

	/* Add the subtitles cache dir as a shortcut */
	new_path = g_build_filename (g_get_user_cache_dir (),
				     "totem",
				     "subtitles",
				     NULL);
	gtk_file_chooser_add_shortcut_folder_uri (GTK_FILE_CHOOSER (fs), new_path, NULL);
	g_free (new_path);

	/* Add the last open path as a shortcut */
	new_path = mateconf_client_get_string (conf, "/apps/totem/open_path", NULL);
	if (new_path != NULL && *new_path != '\0') {
		gtk_file_chooser_add_shortcut_folder_uri (GTK_FILE_CHOOSER (fs), new_path, NULL);
	}
	g_free (new_path);

	/* Try to set the passed path as the current folder */
	if (uri != NULL) {
		folder_set = gtk_file_chooser_set_current_folder_uri
			(GTK_FILE_CHOOSER (fs), uri);
		gtk_file_chooser_add_shortcut_folder_uri (GTK_FILE_CHOOSER (fs), uri, NULL);
	}
	
	/* And set it as home if it fails */
	if (folder_set == FALSE) {
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (fs),
						     g_get_home_dir ());
	}
	totem_add_default_dirs (GTK_FILE_CHOOSER (fs));

	if (gtk_dialog_run (GTK_DIALOG (fs)) == GTK_RESPONSE_ACCEPT) {
		subtitle = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (fs));
	}

	gtk_widget_destroy (fs);
	return subtitle;
}

GSList *
totem_add_files (GtkWindow *parent, const char *path)
{
	GtkWidget *fs;
	int response;
	GSList *filenames;
	char *mrl, *new_path;
	MateConfClient *conf;
	gboolean set_folder;

	fs = gtk_file_chooser_dialog_new (_("Select Movies or Playlists"),
			parent,
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_ADD, GTK_RESPONSE_ACCEPT,
			NULL);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (fs), filter_all);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (fs), filter_supported);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (fs), filter_audio);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (fs), filter_video);
	gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (fs), filter_supported);
	gtk_dialog_set_default_response (GTK_DIALOG (fs), GTK_RESPONSE_ACCEPT);
	gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (fs), TRUE);
	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (fs), FALSE);

	conf = mateconf_client_get_default ();
	set_folder = TRUE;
	if (path != NULL) {
		set_folder = gtk_file_chooser_set_current_folder_uri
			(GTK_FILE_CHOOSER (fs), path);
	} else {
		new_path = mateconf_client_get_string (conf, "/apps/totem/open_path", NULL);
		if (new_path != NULL && *new_path != '\0') {
			set_folder = gtk_file_chooser_set_current_folder_uri
				(GTK_FILE_CHOOSER (fs), new_path);
		}
		g_free (new_path);
	}

	/* We didn't manage to change the directory */
	if (set_folder == FALSE) {
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (fs),
						     g_get_home_dir ());
	}
	totem_add_default_dirs (GTK_FILE_CHOOSER (fs));

	response = gtk_dialog_run (GTK_DIALOG (fs));

	if (response != GTK_RESPONSE_ACCEPT) {
		gtk_widget_destroy (fs);
		g_object_unref (conf);
		return NULL;
	}

	filenames = gtk_file_chooser_get_uris (GTK_FILE_CHOOSER (fs));
	if (filenames == NULL) {
		gtk_widget_destroy (fs);
		g_object_unref (conf);
		return NULL;
	}
	gtk_widget_destroy (fs);

	mrl = filenames->data;
	if (mrl != NULL) {
		new_path = g_path_get_dirname (mrl);
		mateconf_client_set_string (conf, "/apps/totem/open_path",
				new_path, NULL);
		g_free (new_path);
	}

	g_object_unref (conf);

	return filenames;
}

void
totem_save_position (Totem *totem)
{
	gint64 stream_length, position;
	char *pos_str;
	GFile *file;
	GError *error = NULL;

	if (totem->remember_position == FALSE)
		return;
	if (totem->mrl == NULL)
		return;

	stream_length = bacon_video_widget_get_stream_length (totem->bvw);
	position = bacon_video_widget_get_current_time (totem->bvw);

	file = g_file_new_for_uri (totem->mrl);

	/* Don't save if it's:
	 *  - a live stream
	 *  - too short to make saving useful
	 *  - too close to the beginning or end to make saving useful
	 */
	if (stream_length < SAVE_POSITION_THRESHOLD ||
	    (stream_length - position) < stream_length * SAVE_POSITION_END_THRESHOLD ||
	    position < stream_length * SAVE_POSITION_END_THRESHOLD) {
		g_debug ("not saving position because the video/track is too short");

		/* Remove the attribute if it is currently set on the file; this ensures that if we start watching a stream and save the position
		 * half-way through, then later continue watching it to the end, the mid-way saved position will be removed when we finish the
		 * stream. Only do this for non-live streams. */
		if (stream_length > 0) {
			g_file_set_attribute_string (file, SAVE_POSITION_FILE_ATTRIBUTE, "", G_FILE_QUERY_INFO_NONE, NULL, &error);
			if (error != NULL) {
				g_warning ("g_file_set_attribute_string failed: %s", error->message);
				g_error_free (error);
			}
		}

		g_object_unref (file);
		return;
	}

	g_debug ("saving position: %"G_GINT64_FORMAT, position);

	/* Save the position in the stream as a file attribute */
	pos_str = g_strdup_printf ("%"G_GINT64_FORMAT, position);
	g_file_set_attribute_string (file, SAVE_POSITION_FILE_ATTRIBUTE, pos_str, G_FILE_QUERY_INFO_NONE, NULL, &error);
	g_free (pos_str);

	if (error != NULL) {
		g_warning ("g_file_set_attribute_string failed: %s", error->message);
		g_error_free (error);
	}
	g_object_unref (file);
}

void
totem_try_restore_position (Totem *totem, const char *mrl)
{
	GFile *file;
	GFileInfo *file_info;
	const char *seek_str;

	if (totem->remember_position == FALSE)
		return;

	if (mrl == NULL)
		return;

	file = g_file_new_for_uri (mrl);
	g_debug ("trying to restore position of: %s", mrl);

	/* Get the file attribute containing the position */
	file_info = g_file_query_info (file, SAVE_POSITION_FILE_ATTRIBUTE, G_FILE_QUERY_INFO_NONE, NULL, NULL);
	g_object_unref (file);

	if (file_info == NULL)
		return;

	seek_str = g_file_info_get_attribute_string (file_info, SAVE_POSITION_FILE_ATTRIBUTE);
	g_debug ("seek time: %s", seek_str);

	if (seek_str != NULL)
		totem->seek_to = g_ascii_strtoull (seek_str, NULL, 0);

	g_object_unref (file_info);
}
