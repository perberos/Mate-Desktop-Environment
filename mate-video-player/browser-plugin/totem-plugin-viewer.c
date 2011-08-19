/* Totem Plugin Viewer
 *
 * Copyright © 2004-2006 Bastien Nocera <hadess@hadess.net>
 * Copyright © 2002 David A. Schleef <ds@schleef.org>
 * Copyright © 2006, 2009 Christian Persch
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301  USA.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <mateconf/mateconf-client.h>

#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

#include <totem-pl-parser.h>
#include <totem-scrsaver.h>

#include <dbus/dbus-glib.h>

#include "bacon-video-widget.h"
#include "totem-interface.h"
#include "totem-statusbar.h"
#include "totem-time-label.h"
#include "totem-fullscreen.h"
#include "totem-glow-button.h"
#include "video-utils.h"

#include "totem-plugin-viewer-constants.h"
#include "totem-plugin-viewer-options.h"
#include "marshal.h"

GtkWidget *totem_statusbar_create (void);
GtkWidget *totem_volume_create (void);
GtkWidget *totem_pp_create (void);

/* Private function in totem-pl-parser, not for use
 * by anyone but us */
char * totem_pl_parser_resolve_uri (GFile *base_gfile, const char *relative_uri);

#define VOLUME_DOWN_OFFSET (-0.08)
#define VOLUME_UP_OFFSET (0.08)
#define MINIMUM_VIDEO_SIZE 150

/* For newer D-Bus version */
#ifndef DBUS_NAME_FLAG_PROHIBIT_REPLACEMENT
#define DBUS_NAME_FLAG_PROHIBIT_REPLACEMENT 0
#endif

typedef enum {
	TOTEM_PLUGIN_TYPE_GMP,
	TOTEM_PLUGIN_TYPE_NARROWSPACE,
	TOTEM_PLUGIN_TYPE_MULLY,
	TOTEM_PLUGIN_TYPE_CONE,
	TOTEM_PLUGIN_TYPE_LAST
} TotemPluginType;

#define TOTEM_TYPE_EMBEDDED (totem_embedded_get_type ())
#define TOTEM_EMBEDDED(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), TOTEM_TYPE_EMBEDDED, TotemEmbedded))
#define TOTEM_EMBEDDED_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), TOTEM_TYPE_EMBEDDED, TotemEmbeddedClass))
#define TOTEM_IS_EMBEDDED(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), TOTEM_TYPE_EMBEDDED))
#define TOTEM_IS_EMBEDDED_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), TOTEM_TYPE_EMBEDDED))
#define TOTEM_EMBEDDED_GET_CLASS(o)(G_TYPE_INSTANCE_GET_CLASS ((o), TOTEM_TYPE_EMBEDDED, TotemEmbeddedClass))

typedef GObjectClass TotemEmbeddedClass;

typedef struct {
	char *uri;
	char *subtitle;
	char *title;
	int duration;
	int starttime;
} TotemPlItem;

typedef struct _TotemEmbedded {
	GObject parent;

	DBusGConnection *conn;
	GtkWidget *window;
	GtkBuilder *menuxml, *xml;
	GtkWidget *about;
	GtkWidget *pp_button;
	GtkWidget *pp_fs_button;
	TotemStatusbar *statusbar;
	TotemScrsaver *scrsaver;
	int width, height;
        char *user_agent;
	const char *mimetype;
        char *referrer_uri;
	char *base_uri;
	char *current_uri;
	char *current_subtitle_uri;
	char *href_uri;
	char *target;
	char *stream_uri;
	gint64 size; /* the size of the streamed file for fd://0 */
	BaconVideoWidget *bvw;
	TotemStates state;
	GdkCursor *cursor;

	/* Playlist, a GList of TotemPlItem */
	GList *playlist, *current;
	guint parser_id;
	int num_items;

	/* Open menu item */
	GAppInfo *app;
	GtkWidget *menu_item;

	/* Seek bits */
	GtkAdjustment *seekadj;
	GtkWidget *seek;

	/* Volume */
	GtkWidget *volume;

	/* Fullscreen */
	TotemFullscreen *fs;
	GtkWidget * fs_window;

	/* Error */
	GError *error;

	guint type : 3; /* TotemPluginType */

	guint is_browser_stream : 1;
	guint is_playlist : 1;
	guint controller_hidden : 1;
	guint show_statusbar : 1;
	guint hidden : 1;
	guint repeat : 1;
	guint seeking : 1;
	guint autostart : 1;
	guint audioonly : 1;
} TotemEmbedded;

GType totem_embedded_get_type (void);

#define TOTEM_EMBEDDED_ERROR_QUARK (g_quark_from_static_string ("TotemEmbeddedErrorQuark"))

enum
{
	TOTEM_EMBEDDED_UNKNOWN_PLUGIN_TYPE,
	TOTEM_EMBEDDED_SETWINDOW_UNSUPPORTED_CONTROLS,
	TOTEM_EMBEDDED_SETWINDOW_HAVE_WINDOW,
	TOTEM_EMBEDDED_SETWINDOW_INVALID_XID,
	TOTEM_EMBEDDED_NO_URI,
	TOTEM_EMBEDDED_OPEN_FAILED,
	TOTEM_EMBEDDED_UNKNOWN_COMMAND
};

G_DEFINE_TYPE (TotemEmbedded, totem_embedded, G_TYPE_OBJECT);
static void totem_embedded_init (TotemEmbedded *emb) { }

static gboolean totem_embedded_do_command (TotemEmbedded *emb, const char *command, GError **err);
static gboolean totem_embedded_push_parser (gpointer data);
static gboolean totem_embedded_play (TotemEmbedded *embedded, GError **error);
static void totem_embedded_set_logo_by_name (TotemEmbedded *embedded, const char *name);

static void totem_embedded_update_menu (TotemEmbedded *emb);
static void on_open1_activate (GtkButton *button, TotemEmbedded *emb);
static void totem_embedded_toggle_fullscreen (TotemEmbedded *emb);

void on_about1_activate (GtkButton *button, TotemEmbedded *emb);
void on_preferences1_activate (GtkButton *button, TotemEmbedded *emb);
void on_copy_location1_activate (GtkButton *button, TotemEmbedded *emb);
void on_fullscreen1_activate (GtkMenuItem *menuitem, TotemEmbedded *emb);

static void update_fill (TotemEmbedded *emb, gdouble level);

enum {
	BUTTON_PRESS,
	START_STREAM,
	STOP_STREAM,
	SIGNAL_TICK,
	PROPERTY_CHANGE,
	LAST_SIGNAL
};

static int signals[LAST_SIGNAL] = { 0 };

static void
totem_embedded_finalize (GObject *object)
{
	TotemEmbedded *embedded = TOTEM_EMBEDDED (object);

	if (embedded->window)
		gtk_widget_destroy (embedded->window);

	if (embedded->xml)
		g_object_unref (embedded->xml);
	if (embedded->menuxml)
		g_object_unref (embedded->menuxml);
	if (embedded->fs)
		g_object_unref (embedded->fs);

	/* FIXME etc */

	G_OBJECT_CLASS (totem_embedded_parent_class)->finalize (object);
}

static void totem_embedded_class_init (TotemEmbeddedClass *klass)
{
	GType param_types[3];
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = totem_embedded_finalize;

	param_types[0] = param_types[1] = G_TYPE_UINT;
	param_types[2] = G_TYPE_STRING;
	signals[BUTTON_PRESS] =
		g_signal_newv ("button-press",
				G_TYPE_FROM_CLASS (object_class),
				G_SIGNAL_RUN_LAST,
				NULL /* class closure */,
				NULL /* accu */, NULL /* accu data */,
				totempluginviewer_marshal_VOID__UINT_UINT,
				G_TYPE_NONE, 2, param_types);
	signals[START_STREAM] =
		g_signal_newv ("start-stream",
				G_TYPE_FROM_CLASS (object_class),
				G_SIGNAL_RUN_LAST,
				NULL /* class closure */,
				NULL /* accu */, NULL /* accu data */,
				g_cclosure_marshal_VOID__VOID,
				G_TYPE_NONE,
				0, NULL);
	signals[STOP_STREAM] =
		g_signal_newv ("stop-stream",
				G_TYPE_FROM_CLASS (object_class),
				G_SIGNAL_RUN_LAST,
				NULL /* class closure */,
				NULL /* accu */, NULL /* accu data */,
				g_cclosure_marshal_VOID__VOID,
				G_TYPE_NONE,
				0, NULL);
	signals[SIGNAL_TICK] =
		g_signal_newv ("tick",
				G_TYPE_FROM_CLASS (object_class),
				G_SIGNAL_RUN_LAST,
				NULL /* class closure */,
				NULL /* accu */, NULL /* accu data */,
				totempluginviewer_marshal_VOID__UINT_UINT_STRING,
				G_TYPE_NONE, 3, param_types);
	signals[PROPERTY_CHANGE] =
		g_signal_new ("property-change",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      0 /* class offset */,
			      NULL /* accu */, NULL /* accu data */,
			      totempluginviewer_marshal_VOID__STRING_BOXED,
			      G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_VALUE);
}

static void
totem_embedded_exit (TotemEmbedded *emb)
{
	//FIXME what happens when embedded, and we can't go on?
	exit (1);
}

static void
totem_embedded_error_and_exit (char *title, char *reason, TotemEmbedded *emb)
{
	/* Avoid any more contacts, so drop off the bus */
	if (emb->conn != NULL) {
		dbus_g_connection_unregister_g_object(emb->conn, G_OBJECT (emb));
		emb->conn = NULL;
	}

	/* FIXME send a signal to the plugin with the error message instead! */
	totem_interface_error_blocking (title, reason,
			GTK_WINDOW (emb->window));
	totem_embedded_exit (emb);
}

static void
totem_embedded_volume_changed (TotemEmbedded *emb, double volume)
{
	GValue value = { 0, };

	g_value_init (&value, G_TYPE_DOUBLE);
	g_value_set_double (&value, volume);
	g_signal_emit (emb, signals[PROPERTY_CHANGE], 0,
		       TOTEM_PROPERTY_VOLUME,
		       &value);
}

static void
totem_embedded_set_error (TotemEmbedded *emb,
			  int code,
			  char *secondary)
{
	emb->error = g_error_new_literal (TOTEM_EMBEDDED_ERROR_QUARK,
	                                  code,
	                                  secondary);
	g_message ("totem_embedded_set_error: '%s'", secondary);
}

static gboolean
totem_embedded_set_error_logo (TotemEmbedded *embedded,
			       GError *error)
{
	g_message ("totem_embedded_set_error_logo called by browser plugin");
	totem_embedded_set_logo_by_name (embedded, "image-missing");
	return TRUE;
}


static void
totem_embedded_set_state (TotemEmbedded *emb, TotemStates state)
{
	GtkWidget *image;
	const gchar *id;

	if (state == emb->state)
		return;

	g_message ("Viewer state: %s", totem_states[state]);

	image = gtk_button_get_image (GTK_BUTTON (emb->pp_button));

	switch (state) {
	case TOTEM_STATE_STOPPED:
		id = GTK_STOCK_MEDIA_PLAY;
		totem_statusbar_set_text (emb->statusbar, _("Stopped"));
		totem_statusbar_set_time_and_length (emb->statusbar, 0, 0);
		totem_time_label_set_time 
			(TOTEM_TIME_LABEL (emb->fs->time_label), 0, 0);
		if (emb->href_uri != NULL && emb->hidden == FALSE) {
			gdk_window_set_cursor
				(gtk_widget_get_window (GTK_WIDGET (emb->bvw)),
				 emb->cursor);
		}
		break;
	case TOTEM_STATE_PAUSED:
		id = GTK_STOCK_MEDIA_PLAY;
		totem_statusbar_set_text (emb->statusbar, _("Paused"));
		break;
	case TOTEM_STATE_PLAYING:
		id = GTK_STOCK_MEDIA_PAUSE;
		totem_statusbar_set_text (emb->statusbar, _("Playing"));
		if (emb->href_uri == NULL && emb->hidden == FALSE) {
			gdk_window_set_cursor
				(gtk_widget_get_window (GTK_WIDGET (emb->bvw)),
				 NULL);
		}
		break;
	default:
		g_assert_not_reached ();
		break;
	}

	if (emb->scrsaver != NULL)
		totem_scrsaver_set_state (emb->scrsaver, (state == TOTEM_STATE_PLAYING) ? FALSE : TRUE);
	gtk_image_set_from_stock (GTK_IMAGE (image), id, GTK_ICON_SIZE_MENU);
	gtk_tool_button_set_stock_id (GTK_TOOL_BUTTON (emb->pp_fs_button), id);

	emb->state = state;
}

static void
totem_embedded_set_logo_by_name (TotemEmbedded *embedded,
				 const char *name)
{
	totem_embedded_set_state (embedded, TOTEM_STATE_STOPPED);

	if (embedded->audioonly != FALSE || embedded->hidden != FALSE)
		return;

	bacon_video_widget_set_logo (embedded->bvw, name);
}

static void
totem_embedded_set_pp_state (TotemEmbedded *emb, gboolean state)
{
	gtk_widget_set_sensitive (emb->pp_button, state);
	gtk_widget_set_sensitive (emb->pp_fs_button, state);
}

static gboolean
totem_embedded_open_internal (TotemEmbedded *emb,
			      gboolean start_play,
			      GError **error)
{
	gboolean retval;
	char *uri;

	/* FIXME: stop previous content, or is that automatic ? */

	if (emb->is_browser_stream) {
		if (emb->size > 0)
			uri = g_strdup_printf ("fd://0?size=%"G_GINT64_FORMAT, emb->size);
		else
			uri = g_strdup ("fd://0");
	} else {
		uri = g_strdup (emb->current_uri);
	}

	if (!uri) {
		g_set_error_literal (error,
                                     TOTEM_EMBEDDED_ERROR_QUARK,
                                     TOTEM_EMBEDDED_NO_URI,
                                     _("No URI to play"));
		//FIXME totem_embedded_set_error (emb, error); |error| may be null?

		return FALSE;
	}

	g_message ("totem_embedded_open_internal '%s' subtitle '%s' is-browser-stream %d start-play %d",
		   uri, emb->current_subtitle_uri, emb->is_browser_stream, start_play);

	bacon_video_widget_set_logo_mode (emb->bvw, FALSE);

	retval = bacon_video_widget_open (emb->bvw, uri, emb->current_subtitle_uri, NULL);
	g_free (uri);

	/* FIXME we shouldn't even do that here */
	if (start_play)
		totem_embedded_play (emb, NULL);
	else
		totem_glow_button_set_glow (TOTEM_GLOW_BUTTON (emb->pp_button), TRUE);

	totem_embedded_update_menu (emb);
	if (emb->href_uri != NULL)
		totem_fullscreen_set_title (emb->fs, emb->href_uri);
	else
		totem_fullscreen_set_title (emb->fs, emb->current_uri);

	return retval;
}

static gboolean
totem_embedded_play (TotemEmbedded *emb,
		     GError **error)
{
	GError *err = NULL;

	if (emb->current_uri == NULL) {
		totem_glow_button_set_glow (TOTEM_GLOW_BUTTON (emb->pp_button), FALSE);
		g_signal_emit (emb, signals[BUTTON_PRESS], 0,
			       gtk_get_current_event_time (), 0);
		return TRUE;
	}

	totem_glow_button_set_glow (TOTEM_GLOW_BUTTON (emb->pp_button), FALSE);

	if (bacon_video_widget_play (emb->bvw, &err) != FALSE) {
		totem_embedded_set_state (emb, TOTEM_STATE_PLAYING);
		totem_embedded_set_pp_state (emb, TRUE);
	} else {
		g_warning ("Error in bacon_video_widget_play: %s", err->message);
		g_error_free (err);
	}

	return TRUE;
}

static gboolean
totem_embedded_pause (TotemEmbedded *emb,
		      GError **error)
{
	totem_embedded_set_state (emb, TOTEM_STATE_PAUSED);
	bacon_video_widget_pause (emb->bvw);

	return TRUE;
}

static gboolean
totem_embedded_stop (TotemEmbedded *emb,
		     GError **error)
{
	totem_embedded_set_state (emb, TOTEM_STATE_STOPPED);
	bacon_video_widget_stop (emb->bvw);

	g_signal_emit (emb, signals[SIGNAL_TICK], 0,
		       (guint32) 0,
		       (guint32) 0,
		       totem_states[emb->state]);

	return TRUE;
}

static gboolean
totem_embedded_do_command (TotemEmbedded *embedded,
			   const char *command,
			   GError **error)
{
	g_return_val_if_fail (command != NULL, FALSE);

	g_message ("totem_embedded_do_command: %s", command);

	if (strcmp (command, TOTEM_COMMAND_PLAY) == 0) {
		return totem_embedded_play (embedded, error);
	}
	if (strcmp (command, TOTEM_COMMAND_PAUSE) == 0) {
		return totem_embedded_pause (embedded, error);
	}
	if (strcmp (command, TOTEM_COMMAND_STOP) == 0) {
		return totem_embedded_stop (embedded, error);
	}
		
	g_set_error (error,
                     TOTEM_EMBEDDED_ERROR_QUARK,
                     TOTEM_EMBEDDED_UNKNOWN_COMMAND,
                     "Unknown command '%s'", command);
	return FALSE;
}

static gboolean
totem_embedded_set_href (TotemEmbedded *embedded,
			 const char *href_uri,
			 const char *target,
			 GError *error)
{
	g_message ("totem_embedded_set_href %s (target: %s)",
		   href_uri, target);

	g_free (embedded->href_uri);
	g_free (embedded->target);

	if (href_uri != NULL) {
		embedded->href_uri = g_strdup (href_uri);
	} else {
		g_free (embedded->href_uri);
		embedded->href_uri = NULL;
		gdk_window_set_cursor
			(gtk_widget_get_window (GTK_WIDGET (embedded->bvw)), NULL);
	}

	if (target != NULL) {
		embedded->target = g_strdup (target);
	} else {
		embedded->target = NULL;
	}

	return TRUE;
}

static gboolean
totem_embedded_set_volume (TotemEmbedded *embedded,
			   gdouble volume,
			   GError *error)
{
	g_message ("totem_embedded_set_volume: %f", volume);
	bacon_video_widget_set_volume (embedded->bvw, volume);
	totem_embedded_volume_changed (embedded, volume);
	return TRUE;
}

static gboolean
totem_embedded_launch_player (TotemEmbedded *embedded,
 			      guint32 user_time)
{
	GList *uris = NULL;
	GdkScreen *screen;
	GAppLaunchContext *ctx;
	gboolean result;
	const char *uri;

	g_return_val_if_fail (embedded->app != NULL, FALSE);

	if (embedded->type == TOTEM_PLUGIN_TYPE_NARROWSPACE
		   && embedded->href_uri != NULL) {
		uris = g_list_prepend (uris, embedded->href_uri);
		uri = embedded->href_uri;
	} else {
		uris = g_list_prepend (uris, embedded->current_uri);
		uri = embedded->current_uri;
	}

	ctx = G_APP_LAUNCH_CONTEXT (gdk_app_launch_context_new ());
	screen = gtk_widget_get_screen (embedded->window);
	gdk_app_launch_context_set_screen (GDK_APP_LAUNCH_CONTEXT (ctx), screen);

	gdk_app_launch_context_set_timestamp (GDK_APP_LAUNCH_CONTEXT (ctx), user_time);
	gdk_app_launch_context_set_icon (GDK_APP_LAUNCH_CONTEXT (ctx), g_app_info_get_icon (embedded->app));

	result = g_app_info_launch_uris (embedded->app, uris, ctx, NULL);

	g_list_free (uris);

	return result;
}

static void
totem_embedded_set_uri (TotemEmbedded *emb,
		        const char *uri,
		        const char *base_uri,
		        const char *subtitle,
		        gboolean is_browser_stream)
{
	GFile *base_gfile;
	char *old_uri, *old_base, *old_href, *old_subtitle;

	base_gfile = NULL;
	old_uri = emb->current_uri;
	old_subtitle = emb->current_subtitle_uri;
	old_base = emb->base_uri;
	old_href = emb->href_uri;

	emb->base_uri = g_strdup (base_uri);
	if (base_uri)
		base_gfile = g_file_new_for_uri (base_uri);
	emb->current_uri = totem_pl_parser_resolve_uri (base_gfile, uri);
	if (subtitle != NULL)
		emb->current_subtitle_uri = totem_pl_parser_resolve_uri (base_gfile, subtitle);
	else
		emb->current_subtitle_uri = NULL;
	if (base_gfile)
		g_object_unref (base_gfile);
	emb->is_browser_stream = (is_browser_stream != FALSE);
	emb->href_uri = NULL;

	if (uri != NULL)
		g_print ("totem_embedded_set_uri uri %s base %s => resolved %s (subtitle %s => resolved %s)\n",
			 uri, base_uri, emb->current_uri, subtitle, emb->current_subtitle_uri);
	else
		g_print ("Emptying current_uri\n");

	if (old_uri != NULL &&
	    g_str_has_prefix (old_uri, "fd://") != FALSE) {
		int fd;
		fd = strtol (old_uri + strlen ("fd://"), NULL, 0);
		close (fd);
	}
	g_free (old_uri);
	g_free (old_base);
	g_free (old_href);
	g_free (old_subtitle);
	g_free (emb->stream_uri);
	emb->stream_uri = NULL;
}

static void
totem_embedded_update_title (TotemEmbedded *emb, const char *title)
{
	if (title == NULL)
		gtk_window_set_title (GTK_WINDOW (emb->fs_window), _("Totem Movie Player"));
	else
		gtk_window_set_title (GTK_WINDOW (emb->fs_window), title);
	totem_fullscreen_set_title (emb->fs, title);
}

static void
totem_pl_item_free (gpointer data, gpointer user_data)
{
	TotemPlItem *item = (TotemPlItem *) data;

	if (!item)
		return;
	g_free (item->uri);
	g_free (item->title);
	g_free (item->subtitle);
	g_slice_free (TotemPlItem, item);
}

static gboolean
totem_embedded_clear_playlist (TotemEmbedded *emb, GError *error)
{
	g_message ("totem_embedded_clear_playlist");

	g_list_foreach (emb->playlist, (GFunc) totem_pl_item_free, NULL);
	g_list_free (emb->playlist);

	emb->playlist = NULL;
	emb->current = NULL;
	emb->num_items = 0;

	totem_embedded_set_uri (emb, NULL, NULL, NULL, FALSE);

	bacon_video_widget_close (emb->bvw);
	update_fill (emb, -1.0);
	totem_embedded_update_title (emb, NULL);

	return TRUE;
}

static gboolean
totem_embedded_add_item (TotemEmbedded *embedded,
			 const char *base_uri,
			 const char *uri,
			 const char *title,
			 const char *subtitle,
			 GError *error)
{
	TotemPlItem *item;

	g_message ("totem_embedded_add_item: %s (base: %s title: %s subtitle: %s)",
		   uri, base_uri, title, subtitle);

	item = g_slice_new0 (TotemPlItem);
	item->uri = g_strdup (uri);
	item->title = g_strdup (title);
	item->subtitle = g_strdup (subtitle);
	item->duration = -1;
	item->starttime = -1;

	embedded->playlist = g_list_append (embedded->playlist, item);
	embedded->num_items++;

	if (embedded->current_uri == NULL) {
		embedded->current = embedded->playlist;
		totem_embedded_set_uri (embedded,
					(const char *) uri,
					base_uri,
					subtitle,
					FALSE);
		totem_embedded_open_internal (embedded, FALSE, NULL /* FIXME */);
	}

	return TRUE;
}

static gboolean
totem_embedded_set_fullscreen (TotemEmbedded *emb,
			       gboolean fullscreen_enabled,
			       GError **error)
{
	GValue value = { 0, };
	GtkAction *fs_action;
	
	fs_action = GTK_ACTION (gtk_builder_get_object 
				(emb->menuxml, "fullscreen1"));

	if (totem_fullscreen_is_fullscreen (emb->fs) == fullscreen_enabled)
		return TRUE;

	g_message ("totem_embedded_set_fullscreen: %d", fullscreen_enabled);

	if (fullscreen_enabled == FALSE) {
		GtkWidget * container;
		container = GTK_WIDGET (gtk_builder_get_object (emb->xml,
								"video_box"));

		totem_fullscreen_set_fullscreen (emb->fs, FALSE);
		gtk_widget_reparent (GTK_WIDGET (emb->bvw), container);
		gtk_widget_hide_all (emb->fs_window);

		gtk_action_set_sensitive (fs_action, TRUE);
	} else {
		GdkRectangle rect;
		int monitor;

		/* Move the fullscreen window to the screen where the
		 * video widget currently is */
		monitor = gdk_screen_get_monitor_at_window (gtk_widget_get_screen (GTK_WIDGET (emb->bvw)),
							    gtk_widget_get_window (GTK_WIDGET (emb->bvw)));
		gdk_screen_get_monitor_geometry (gtk_widget_get_screen (GTK_WIDGET (emb->bvw)),
						 monitor, &rect);
		gtk_window_move (GTK_WINDOW (emb->fs_window), rect.x, rect.y);

		gtk_widget_reparent (GTK_WIDGET (emb->bvw), emb->fs_window);
		bacon_video_widget_set_fullscreen (emb->bvw, TRUE);
		gtk_window_fullscreen (GTK_WINDOW (emb->fs_window));
		totem_fullscreen_set_fullscreen (emb->fs, TRUE);
		gtk_widget_show_all (emb->fs_window);

		gtk_action_set_sensitive (fs_action, FALSE);
	}

	g_value_init (&value, G_TYPE_BOOLEAN);
	g_value_set_boolean (&value, fullscreen_enabled);
	g_signal_emit (emb, signals[PROPERTY_CHANGE], 0,
		       TOTEM_PROPERTY_ISFULLSCREEN,
		       &value);

	return TRUE;
}

static gboolean
totem_embedded_set_time (TotemEmbedded *emb,
			 guint64 time,
			 GError **error)
{
	g_message ("totem_embedded_set_time: %"G_GUINT64_FORMAT, time);

	bacon_video_widget_seek_time (emb->bvw, time, FALSE, NULL);

	return TRUE;
}

static gboolean
totem_embedded_open_uri (TotemEmbedded *emb,
			 const char *uri,
			 const char *base_uri,
			 GError **error)
{
	g_message ("totem_embedded_open_uri: uri %s base_uri: %s", uri, base_uri);

	totem_embedded_clear_playlist (emb, NULL);

	totem_embedded_set_uri (emb, uri, base_uri, NULL, FALSE);
	/* We can only have one item in the "playlist" when
	 * we open a particular URI like this */
	emb->num_items = 1;

	return totem_embedded_open_internal (emb, TRUE, error);
}

static gboolean
totem_embedded_setup_stream (TotemEmbedded *emb,
			     const char *uri,
			     const char *base_uri,
			     GError **error)
{
	g_message ("totem_embedded_setup_stream called: uri %s, base_uri: %s", uri, base_uri);

	totem_embedded_clear_playlist (emb, NULL);

	totem_embedded_set_uri (emb, uri, base_uri, NULL, TRUE);
	/* We can only have one item in the "playlist" when
	 * we open a browser stream */
	emb->num_items = 1;

	/* FIXME: consume any remaining input from stdin */

	return TRUE;
}

static gboolean
totem_embedded_open_stream (TotemEmbedded *emb,
			    gint64 size,
			    GError **error)
{
	g_message ("totem_embedded_open_stream called: with size %"G_GINT64_FORMAT, size);

	emb->size = size;

	return totem_embedded_open_internal (emb, TRUE, error);
}

static gboolean
totem_embedded_close_stream (TotemEmbedded *emb,
			     GError *error)
{
	g_message ("totem_embedded_close_stream");

	if (!emb->is_browser_stream)
		return TRUE;

	/* FIXME this enough? */
	bacon_video_widget_close (emb->bvw);
	update_fill (emb, -1.0);

	return TRUE;
}

static gboolean
totem_embedded_open_playlist_item (TotemEmbedded *emb,
				   GList *item)
{
	TotemPlItem *plitem;
	gboolean eop;

	if (!emb->playlist)
		return FALSE;

	eop = (item == NULL);

	/* Start at the head */
	if (item == NULL)
		item = emb->playlist;

	/* FIXME: if (emb->current == item) { just start again, depending on repeat/autostart settings } */
	emb->current = item;
	g_assert (item != NULL);

	plitem = (TotemPlItem *) item->data;

	totem_embedded_set_uri (emb,
				(const char *) plitem->uri,
			        emb->base_uri /* FIXME? */,
			        plitem->subtitle,
			        FALSE);

	bacon_video_widget_close (emb->bvw);
	update_fill (emb, -1.0);

	//FIXME set the title from the URI if possible
	totem_embedded_update_title (emb, plitem->title);
	if (totem_embedded_open_internal (emb, FALSE, NULL /* FIXME */)) {
		if (plitem->starttime > 0) {
			gboolean retval;

			g_message ("Seeking to %d seconds for starttime", plitem->starttime);
			retval = bacon_video_widget_seek_time (emb->bvw,
							       plitem->starttime * 1000,
							       FALSE,
							       NULL /* FIXME */);
			if (!retval)
				return TRUE;
		}

		if ((eop != FALSE && emb->repeat != FALSE) || (eop == FALSE)) {
		    	    totem_embedded_play (emb, NULL);
		}
	}

	return TRUE;
}

static gboolean
totem_embedded_set_local_file (TotemEmbedded *emb,
			       const char *path,
			       const char *uri,
			       const char *base_uri,
			       GError **error)
{
	char *file_uri;

	g_message ("Setting the current path to %s (uri: %s base: %s)",
		   path, uri, base_uri);

	totem_embedded_clear_playlist (emb, NULL);

	file_uri = g_filename_to_uri (path, NULL, error);
	if (!file_uri)
		return FALSE;

	/* FIXME what about |uri| param?!! */
	totem_embedded_set_uri (emb, file_uri, base_uri, emb->current_subtitle_uri, FALSE);
	g_free (file_uri);

	return totem_embedded_open_internal (emb, TRUE, error);
}

static gboolean
totem_embedded_set_local_cache (TotemEmbedded *emb,
				const char *path,
				GError **error)
{
	int fd;

	g_message ("totem_embedded_set_local_cache: %s", path);

	/* FIXME Should also handle playlists */
	if (!emb->is_browser_stream)
		return TRUE;

	/* Keep the temporary file open, so that StreamAsFile
	 * doesn't remove it from under us */
	fd = open (path, O_RDONLY);
	if (fd < 0) {
		g_message ("Failed to open local cache file '%s': %s",
			   path, g_strerror (errno));
		return FALSE;
	}

	emb->stream_uri = emb->current_uri;
	emb->current_uri = g_strdup_printf ("fd://%d", fd);

	return TRUE;
}

static gboolean
totem_embedded_set_playlist (TotemEmbedded *emb,
			     const char *path,
			     const char *uri,
			     const char *base_uri,
			     GError **error)
{
	char *file_uri;
	char *tmp_file;
	GError *err = NULL;
	GFile *src, *dst;
	int fd;

	g_message ("Setting the current playlist to %s (uri: %s base: %s)",
		   path, uri, base_uri);

	totem_embedded_clear_playlist (emb, NULL);

	/* FIXME, we should remove that when we can
	 * parse from memory or 
	 * https://bugzilla.mate.org/show_bug.cgi?id=598702 is fixed */
	fd = g_file_open_tmp ("totem-browser-plugin-playlist-XXXXXX",
			      &tmp_file,
			      &err);
	if (fd < 0) {
		g_warning ("Couldn't open temporary file for playlist: %s",
			   err->message);
		g_error_free (err);
		return TRUE;
	}
	src = g_file_new_for_path (path);
	dst = g_file_new_for_path (tmp_file);
	if (g_file_copy (src, dst, G_FILE_COPY_OVERWRITE | G_FILE_COPY_TARGET_DEFAULT_PERMS, NULL, NULL, NULL, &err) == FALSE) {
		g_warning ("Failed to copy playlist '%s' to '%s': %s",
			   path, tmp_file, err->message);
		g_error_free (err);
		g_object_unref (src);
		g_object_unref (dst);
		g_free (tmp_file);
		close (fd);
		return TRUE;
	}
	g_free (tmp_file);

	file_uri = g_file_get_uri (dst);

	g_object_unref (src);
	g_object_unref (dst);
	close (fd);

	totem_embedded_set_uri (emb, file_uri, base_uri, NULL, FALSE);
	g_free (file_uri);

	/* Schedule parsing on idle */
	if (emb->parser_id == 0)
		emb->parser_id = g_idle_add (totem_embedded_push_parser,
					     emb);

	return TRUE;
}

static GAppInfo *
totem_embedded_get_app_for_uri (const char *uri)
{
	char *type;
	GAppInfo *info;

	type = g_content_type_guess (uri, NULL, 0, NULL);
	info = g_app_info_get_default_for_type (type, TRUE);
	g_free (type);

	return info;
}

static void
totem_embedded_update_menu (TotemEmbedded *emb)
{
	GtkWidget *item, *image;
	GtkMenuShell *menu;
	char *label;

	if (emb->menu_item != NULL) {
		gtk_widget_destroy (emb->menu_item);
		emb->menu_item = NULL;
	}
	if (emb->app != NULL) {
		g_object_unref (emb->app);
		emb->app = NULL;
	}

	if (emb->mimetype && strcmp (emb->mimetype, "application/octet-stream") != 0) {
		emb->app = g_app_info_get_default_for_type (emb->mimetype, TRUE);
	} else {
		const char *uri;

		/* If we're reading a local file, that's not a playlist,
		 * we need to use that to get its proper mime-type */
		if (emb->stream_uri != NULL)
			uri = emb->stream_uri;
		else
			uri = emb->current_uri;
		emb->app = totem_embedded_get_app_for_uri (uri);
	}

	if (emb->app == NULL) {
		if (emb->mimetype != NULL) {
			g_message ("Mimetype '%s' doesn't have a handler", emb->mimetype);
		} else {
			char *type;

			type = g_content_type_guess (emb->current_uri, NULL, 0, NULL);
			g_message ("No handler for URI '%s' (guessed mime-type '%s')",
				   emb->current_uri, type);
			g_free (type);
		}
		return;
	}

	/* translators: this is:
	 * Open With ApplicationName
	 * as in caja' right-click menu */
	label = g_strdup_printf (_("_Open with \"%s\""), g_app_info_get_name (emb->app));
	item = gtk_image_menu_item_new_with_mnemonic (label);
	g_free (label);
	image = gtk_image_new_from_stock (GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
	g_signal_connect (G_OBJECT (item), "activate",
			  G_CALLBACK (on_open1_activate), emb);
	gtk_widget_show (item);
	emb->menu_item = item;

	menu = GTK_MENU_SHELL (gtk_builder_get_object (emb->menuxml, "menu"));
	gtk_menu_shell_insert (menu, item, 1);
}

static void
on_open1_activate (GtkButton *button, TotemEmbedded *emb)
{
	GTimeVal val;
	g_get_current_time (&val);
	totem_embedded_launch_player (emb, val.tv_sec);
	totem_embedded_stop (emb, NULL);
}

void
on_fullscreen1_activate (GtkMenuItem *menuitem, TotemEmbedded *emb)
{
	if (totem_fullscreen_is_fullscreen (emb->fs) == FALSE)
		totem_embedded_toggle_fullscreen (emb);
}

void
on_about1_activate (GtkButton *button, TotemEmbedded *emb)
{
	char *backend_version, *description, *license;
	GtkWidget **about;

	const char *authors[] =
	{
		"Bastien Nocera <hadess@hadess.net>",
		"Ronald Bultje <rbultje@ronald.bitfreak.net>",
		"Christian Persch" " <" "chpe" "@" "mate" "." "org" ">",
		NULL
	};
	
	if (emb->about != NULL)
	{
		gtk_window_present (GTK_WINDOW (emb->about));
		return;
	}

	backend_version = bacon_video_widget_get_backend_name (emb->bvw);
	description = g_strdup_printf (_("Browser Plugin using %s"),
				       backend_version);
	license = totem_interface_get_license ();

	emb->about = g_object_new (GTK_TYPE_ABOUT_DIALOG,
				   "program-name", _("Totem Browser Plugin"),
				   "version", VERSION,
				   "copyright", "Copyright © 2002-2007 Bastien Nocera\n"
                                                "Copyright © 2006, 2007, 2008 Christian Persch",
				   "comments", description,
				   "authors", authors,
				   "translator-credits", _("translator-credits"),
				   "logo-icon-name", "totem",
				   "license", license,
				   "wrap-license", TRUE,
				   NULL);

	g_free (backend_version);
	g_free (description);
	g_free (license);

	totem_interface_set_transient_for (GTK_WINDOW (emb->about),
					   GTK_WINDOW (emb->window));

	about = &emb->about;
	g_object_add_weak_pointer (G_OBJECT (emb->about),
				   (gpointer *) about);

	g_signal_connect (G_OBJECT (emb->about), "response",
			  G_CALLBACK (gtk_widget_destroy), NULL);

	gtk_widget_show (emb->about);
}

void
on_copy_location1_activate (GtkButton *button, TotemEmbedded *emb)
{
	GdkDisplay *display;
	GtkClipboard *clip;
	const char *uri;

	if (emb->stream_uri != NULL) {
		uri = emb->stream_uri;
	} else if (emb->href_uri != NULL) {
		uri = emb->href_uri;
	} else {
		uri = emb->current_uri;
	}

	display = gtk_widget_get_display (GTK_WIDGET (emb->window));

	/* Set both the middle-click and the super-paste buffers */
	clip = gtk_clipboard_get_for_display (display,
					      GDK_SELECTION_CLIPBOARD);
	gtk_clipboard_set_text (clip, uri, -1);

	clip = gtk_clipboard_get_for_display (display,
					      GDK_SELECTION_PRIMARY);
	gtk_clipboard_set_text (clip, uri, -1);
}

void
on_preferences1_activate (GtkButton *button, TotemEmbedded *emb)
{
	/* TODO: */
}

static void
on_play_pause (GtkWidget *widget, TotemEmbedded *emb)
{
	if (emb->state == TOTEM_STATE_PLAYING) {
		totem_embedded_pause (emb, NULL);
	} else {
		if (emb->current_uri == NULL) {
			totem_glow_button_set_glow (TOTEM_GLOW_BUTTON (emb->pp_button), FALSE);
			g_signal_emit (emb, signals[BUTTON_PRESS], 0,
				       gtk_get_current_event_time (), 0);
		} else {
			totem_embedded_play (emb, NULL);
		}
	}
}

static void
popup_menu_position_func (GtkMenu *menu, gint *x, gint *y, gboolean *push_in, GtkWidget *button) 
{
	GtkWidget *widget = GTK_WIDGET (button);
	GtkRequisition menu_req;
	GtkTextDirection direction;
	GtkAllocation allocation;

	gtk_widget_size_request (GTK_WIDGET (menu), &menu_req);

	direction = gtk_widget_get_direction (widget);

	gdk_window_get_origin (gtk_widget_get_window (widget), x, y);
	gtk_widget_get_allocation (widget, &allocation);
	*x += allocation.x;
	*y += allocation.y;

	if (direction == GTK_TEXT_DIR_LTR)
		*x += MAX (allocation.width - menu_req.width, 0);
	else if (menu_req.width > allocation.width)
		*x -= menu_req.width - allocation.width;

	/* This might not work properly if the popup button is right at the
	 * top of the screen, but really, what are the chances */
	*y -= menu_req.height;

	*push_in = FALSE;
}

static void
popup_menu_over_arrow (GtkToggleButton *button,
		       GtkMenu *menu,
		       GdkEventButton *event)
{
	gtk_menu_popup (menu, NULL, NULL,
			(GtkMenuPositionFunc) popup_menu_position_func,
			button,
			event ? event->button : 0,
			event ? event->time : gtk_get_current_event_time ());
}

static void
on_popup_button_toggled (GtkToggleButton *button, TotemEmbedded *emb)
{
	GtkMenu *menu;

	menu = GTK_MENU (gtk_builder_get_object (emb->menuxml, "menu"));

	if (gtk_toggle_button_get_active (button) && !gtk_widget_get_visible (GTK_WIDGET (menu))) {
		/* we get here only when the menu is activated by a key
		 * press, so that we can select the first menu item */
		popup_menu_over_arrow (button, menu, NULL);
		gtk_menu_shell_select_first (GTK_MENU_SHELL (menu), FALSE);
	}
}

static gboolean
on_popup_button_button_pressed (GtkToggleButton *button,
				GdkEventButton *event,
				TotemEmbedded *emb)
{
	if (event->button == 1) {
		GtkMenu *menu;

		menu = GTK_MENU (gtk_builder_get_object (emb->menuxml, "menu"));
		if (!gtk_widget_get_visible (GTK_WIDGET (menu))) {
			popup_menu_over_arrow (button, menu, event);
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
		} else {
			gtk_menu_popdown (menu);
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), FALSE);
		}

		return TRUE;
	}

	return FALSE;
}

static void
on_popup_menu_unmap (GtkWidget *menu,
		     TotemEmbedded *emb)
{
	GtkWidget *button;

	button = GTK_WIDGET (gtk_builder_get_object (emb->xml, "popup_button"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), FALSE);
}

static char *
resolve_redirect (const char *old_mrl, const char *mrl)
{
	GFile *old_file, *parent, *new_file;
	char *retval;

	/* Get the parent for the current MRL, that's our base */
	old_file = g_file_new_for_uri (old_mrl);
	parent = g_file_get_parent (old_file);
	g_object_unref (old_file);

	/* Resolve the URL */
	new_file = g_file_get_child (parent, mrl);
	g_object_unref (parent);

	retval = g_file_get_uri (new_file);
	g_object_unref (new_file);

	return retval;
}

static void
on_got_redirect (GtkWidget *bvw, const char *mrl, TotemEmbedded *emb)
{
	char *new_uri = NULL;

	g_message ("stream uri: %s", emb->stream_uri ? emb->stream_uri : "(null)");
	g_message ("current uri: %s", emb->current_uri ? emb->current_uri : "(null)");
	g_message ("base uri: %s", emb->base_uri ? emb->base_uri : "(null)");
	g_message ("redirect: %s", mrl);

	bacon_video_widget_close (emb->bvw);
	update_fill (emb, -1.0);

	/* If we don't have a relative URI */
	if (strstr (mrl, "://") != NULL)
		new_uri = NULL;
	/* We are using a local cache, so we resolve against the stream_uri */
	else if (emb->stream_uri)
		new_uri = resolve_redirect (emb->stream_uri, mrl);
	/* We don't have a local cache, so resolve against the URI */
	else if (emb->current_uri)
		new_uri = resolve_redirect (emb->current_uri, mrl);
	/* FIXME: not sure that this is actually correct... */
	else if (emb->base_uri)
		new_uri = resolve_redirect (emb->base_uri, mrl);

	g_message ("Redirecting to '%s'", new_uri ? new_uri : mrl);

	/* FIXME: clear playlist? or replace current entry? or add a new entry? */
	/* FIXME: use totem_embedded_open_uri? */

	totem_embedded_set_uri (emb, new_uri ? new_uri : mrl , emb->base_uri /* FIXME? */, emb->current_subtitle_uri, FALSE);

	totem_embedded_set_state (emb, TOTEM_STATE_STOPPED);

	totem_embedded_open_internal (emb, TRUE, NULL /* FIXME? */);

	g_free (new_uri);
}

static char *
totem_embedded_get_nice_name_for_stream (BaconVideoWidget *bvw)
{
	char *title, *artist, *retval;
	int tracknum;
	GValue value = { 0, };

	bacon_video_widget_get_metadata (bvw, BVW_INFO_TITLE, &value);
	title = g_value_dup_string (&value);
	g_value_unset (&value);

	if (title == NULL)
		return NULL;

	bacon_video_widget_get_metadata (bvw, BVW_INFO_ARTIST, &value);
	artist = g_value_dup_string (&value);
	g_value_unset (&value);

	if (artist == NULL)
		return title;

	bacon_video_widget_get_metadata (bvw,
					 BVW_INFO_TRACK_NUMBER,
					 &value);
	tracknum = g_value_get_int (&value);

	if (tracknum != 0) {
		retval = g_strdup_printf ("%02d. %s - %s",
				tracknum, artist, title);
	} else {
		retval = g_strdup_printf ("%s - %s", artist, title);
	}
	g_free (artist);
	g_free (title);

	return retval;
}

static void
on_got_metadata (BaconVideoWidget *bvw, TotemEmbedded *emb)
{
	char *title;

	title = totem_embedded_get_nice_name_for_stream (bvw);
	if (title == NULL)
		return;

	totem_embedded_update_title (emb, title);
}

static void
totem_embedded_toggle_fullscreen (TotemEmbedded *emb)
{
	if (totem_fullscreen_is_fullscreen (emb->fs) != FALSE)
		totem_embedded_set_fullscreen (emb, FALSE, NULL);
	else
		totem_embedded_set_fullscreen (emb, TRUE, NULL);
}

static void
totem_embedded_on_fullscreen_exit (GtkWidget *widget, TotemEmbedded *emb)
{
	if (totem_fullscreen_is_fullscreen (emb->fs) != FALSE)
		totem_embedded_toggle_fullscreen (emb);
}

static gboolean
on_video_button_press_event (BaconVideoWidget *bvw,
			     GdkEventButton *event,
			     TotemEmbedded *emb)
{
	guint state = event->state & gtk_accelerator_get_default_mod_mask ();
	GtkMenu *menu;

	//g_print ("button-press type %d button %d state %d play-state %d\n",
	//	 event->type, event->button, state, emb->state);

	menu = GTK_MENU (gtk_builder_get_object (emb->menuxml, "menu"));

	if (event->type == GDK_BUTTON_PRESS && event->button == 1 && state == 0 && emb->state == TOTEM_STATE_STOPPED) {
		if (emb->error != NULL) {
			totem_interface_error (_("An error occurred"), emb->error->message, (GtkWindow *) (emb->window));
			g_error_free (emb->error);
			emb->error = NULL;
		} else if (!gtk_widget_get_visible (GTK_WIDGET (menu))) {
			g_message ("emitting signal");
			g_signal_emit (emb, signals[BUTTON_PRESS], 0,
				       event->time,
				       event->button);
		} else {
			gtk_menu_popdown (menu);
		}
	} else if (event->type == GDK_BUTTON_PRESS && event->button == 3 && state == 0) {
		GtkWidget *popup_button;

		popup_button = GTK_WIDGET (gtk_builder_get_object (emb->xml, "popup_button"));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (popup_button), FALSE);

		gtk_menu_popup (menu, NULL, NULL, NULL, NULL,
				event->button, event->time);
	} else if (event->type == GDK_2BUTTON_PRESS && event->button == 1 && state == 0) {
		totem_embedded_toggle_fullscreen (emb);
	}

	return FALSE;
}

static void
on_eos_event (BaconVideoWidget *bvw, TotemEmbedded *emb)
{
	gboolean start_play;

	totem_embedded_set_state (emb, TOTEM_STATE_STOPPED);
	gtk_adjustment_set_value (emb->seekadj, 0);

	/* FIXME: the plugin needs to handle EOS itself, e.g. for QTNext */

	start_play = (emb->repeat != FALSE && emb->autostart);

	/* No playlist if we have fd://0, right? */
	if (emb->is_browser_stream) {
		/* Verify that we had a SetLocalCache */
		if (g_str_has_prefix (emb->current_uri, "file://") != FALSE) {
			emb->num_items = 1;
			emb->is_browser_stream = FALSE;
			bacon_video_widget_close (emb->bvw);
			update_fill (emb, -1.0);
			totem_embedded_open_internal (emb, start_play, NULL /* FIXME? */);
		} else {
			/* FIXME: should find a way to enable playback of the stream again without re-requesting it */
			totem_embedded_set_pp_state (emb, FALSE);
		}
	/* FIXME? else if (emb->playing_nth_item == emb->playlist_num_items) ? */
	} else if (emb->num_items == 1) {
		if (g_str_has_prefix (emb->current_uri, "file://") != FALSE) {
			if (bacon_video_widget_is_seekable (emb->bvw) != FALSE) {
				bacon_video_widget_pause (emb->bvw);
				bacon_video_widget_seek (emb->bvw, 0.0, NULL);
				if (start_play != FALSE)
					totem_embedded_play (emb, NULL);
			} else {
				bacon_video_widget_close (emb->bvw);
				update_fill (emb, -1.0);
				totem_embedded_open_internal (emb, start_play, NULL /* FIXME? */);
			}
		} else {
			bacon_video_widget_close (emb->bvw);
			update_fill (emb, -1.0);
			totem_embedded_open_internal (emb, start_play, NULL /* FIXME? */);
		}
	} else if (emb->current) {
		totem_embedded_open_playlist_item (emb, emb->current->next);
	}
}

static gboolean
skip_unplayable_stream (TotemEmbedded *emb)
{
	on_eos_event (BACON_VIDEO_WIDGET (emb->bvw), emb);
	return FALSE;
}

static void
on_error_event (BaconVideoWidget *bvw,
		char *message,
                gboolean playback_stopped,
		gboolean fatal,
		TotemEmbedded *emb)
{
	if (playback_stopped) {
		/* FIXME: necessary? */
		if (emb->is_browser_stream)
			g_signal_emit (emb, signals[STOP_STREAM], 0);
	
		totem_embedded_set_state (emb, TOTEM_STATE_STOPPED);

		/* If we have a playlist, and that the current item
		 * is < 60 seconds long, just go through it
		 *
		 * Same thing for all the items in a non-repeat playlist,
		 * other than the last one
		 *
		 * FIXME we should mark streams as not playable though
		 * so we don't loop through unplayable streams... */
		if (emb->num_items > 1 && emb->current) {
			TotemPlItem *item = emb->current->data;

			if ((item->duration > 0 && item->duration < 60)
			    || (!emb->repeat && emb->current->next)) {
				g_idle_add ((GSourceFunc) skip_unplayable_stream, emb);
				return;
			}
		}
	}

	if (fatal) {
		/* FIXME: report error back to plugin */
		exit (1);
	}

	totem_embedded_set_error (emb, BVW_ERROR_GENERIC, message);
	totem_embedded_set_error_logo (emb, NULL);
}

static void
cb_vol (GtkWidget *val, gdouble value, TotemEmbedded *emb)
{
	bacon_video_widget_set_volume (emb->bvw, value);
	totem_embedded_volume_changed (emb, value);
}

static void
on_tick (GtkWidget *bvw,
		gint64 current_time,
		gint64 stream_length,
		double current_position,
		gboolean seekable,
		TotemEmbedded *emb)
{
	if (emb->state != TOTEM_STATE_STOPPED) {
		gtk_widget_set_sensitive (emb->seek, seekable);
		gtk_widget_set_sensitive (emb->fs->seek, seekable);
		if (emb->seeking == FALSE)
			gtk_adjustment_set_value (emb->seekadj,
					current_position * 65535);
		if (stream_length == 0) {
			totem_statusbar_set_time_and_length (emb->statusbar,
					(int) (current_time / 1000), -1);
		} else {
			totem_statusbar_set_time_and_length (emb->statusbar,
					(int) (current_time / 1000),
					(int) (stream_length / 1000));
		}

		totem_time_label_set_time 
			(TOTEM_TIME_LABEL (emb->fs->time_label),
			 current_time, stream_length);
	}

	g_signal_emit (emb, signals[SIGNAL_TICK], 0,
		       (guint32) current_time,
		       (guint32) stream_length,
		       totem_states[emb->state]);
}

static void
on_buffering (BaconVideoWidget *bvw, guint percentage, TotemEmbedded *emb)
{
//	g_message ("Buffering: %d %%", percentage);
}

static void
update_fill (TotemEmbedded *emb, gdouble level)
{
	if (level < 0.0) {
		gtk_range_set_show_fill_level (GTK_RANGE (emb->seek), FALSE);
		gtk_range_set_show_fill_level (GTK_RANGE (emb->fs->seek), FALSE);
	} else {
		gtk_range_set_fill_level (GTK_RANGE (emb->seek), level * 65535.0f);
		gtk_range_set_show_fill_level (GTK_RANGE (emb->seek), TRUE);

		gtk_range_set_fill_level (GTK_RANGE (emb->fs->seek), level * 65535.0f);
		gtk_range_set_show_fill_level (GTK_RANGE (emb->fs->seek), TRUE);
	}
}

static void
on_download_buffering (BaconVideoWidget *bvw, gdouble level, TotemEmbedded *emb)
{
	update_fill (emb, level);
}

static void
property_notify_cb_volume (BaconVideoWidget *bvw,
			   GParamSpec *spec,
			   TotemEmbedded *emb)
{
	double volume;
	
	volume = bacon_video_widget_get_volume (emb->bvw);
	
	g_signal_handlers_block_by_func (emb->volume, cb_vol, emb);
	gtk_scale_button_set_value (GTK_SCALE_BUTTON (emb->volume), volume);
	totem_embedded_volume_changed (emb, volume);
	g_signal_handlers_unblock_by_func (emb->volume, cb_vol, emb);
}

static gboolean
on_seek_start (GtkWidget *widget, GdkEventButton *event, TotemEmbedded *emb)
{
	/* HACK: we want the behaviour you get with the middle button, so we
	 * mangle the event.  clicking with other buttons moves the slider in
	 * step increments, clicking with the middle button moves the slider to
	 * the location of the click.
	 */
	event->button = 2;

	emb->seeking = TRUE;
	totem_time_label_set_seeking (TOTEM_TIME_LABEL (emb->fs->time_label),
				      TRUE);

	return FALSE;
}

static gboolean
cb_on_seek (GtkWidget *widget, GdkEventButton *event, TotemEmbedded *emb)
{
	/* HACK: see on_seek_start */
	event->button = 2;

	bacon_video_widget_seek (emb->bvw,
		gtk_range_get_value (GTK_RANGE (widget)) / 65535, NULL);
	totem_time_label_set_seeking (TOTEM_TIME_LABEL (emb->fs->time_label),
				      FALSE);
	emb->seeking = FALSE;

	return FALSE;
}

#ifdef MATE_ENABLE_DEBUG
static void
controls_size_allocate_cb (GtkWidget *controls,
			   GtkAllocation *allocation,
			   gpointer data)
{
	g_message ("Viewer: Controls height is %dpx", allocation->height);
	g_signal_handlers_disconnect_by_func (controls, G_CALLBACK (controls_size_allocate_cb), NULL);
}
#endif

static void
video_widget_size_allocate_cb (GtkWidget *controls,
			       GtkAllocation *allocation,
			       BaconVideoWidget *bvw)
{
	bacon_video_widget_set_show_visuals (bvw, allocation->height > MINIMUM_VIDEO_SIZE);
	g_signal_handlers_disconnect_by_func (controls, G_CALLBACK (video_widget_size_allocate_cb), NULL);
}

static void
totem_embedded_action_volume_relative (TotemEmbedded *emb, double off_pct)
{
	double vol;
	
	if (bacon_video_widget_can_set_volume (emb->bvw) == FALSE)
		return;
	
	vol = bacon_video_widget_get_volume (emb->bvw);
	bacon_video_widget_set_volume (emb->bvw, vol + off_pct);
	totem_embedded_volume_changed (emb, vol + off_pct);
}

static gboolean
totem_embedded_handle_key_press (TotemEmbedded *emb, GdkEventKey *event)
{
	switch (event->keyval) {
	case GDK_Escape:
		if (totem_fullscreen_is_fullscreen (emb->fs) != FALSE)
			totem_embedded_toggle_fullscreen (emb);
		return TRUE;
	case GDK_F11:
	case GDK_f:
	case GDK_F:
		totem_embedded_toggle_fullscreen (emb);
		return TRUE;
	case GDK_space:
		on_play_pause (NULL, emb);
		return TRUE;
	case GDK_Up:
		totem_embedded_action_volume_relative (emb, VOLUME_UP_OFFSET);
		return TRUE;
	case GDK_Down:
		totem_embedded_action_volume_relative (emb, VOLUME_DOWN_OFFSET);
		return TRUE;
	case GDK_Left:
	case GDK_Right:
		/* FIXME: */
		break;
	}

	return FALSE;
}

static gboolean
totem_embedded_key_press_event (GtkWidget *widget, GdkEventKey *event,
				TotemEmbedded *emb)
{
	if (event->type != GDK_KEY_PRESS)
		return FALSE;
	
	if (event->state & GDK_CONTROL_MASK)
	{
		switch (event->keyval) {
		case GDK_Left:
		case GDK_Right:
			return totem_embedded_handle_key_press (emb, event);
		}
	}

	if (event->state & GDK_CONTROL_MASK ||
	    event->state & GDK_MOD1_MASK ||
	    event->state & GDK_MOD3_MASK ||
	    event->state & GDK_MOD4_MASK ||
	    event->state & GDK_MOD5_MASK)
		return FALSE;

	return totem_embedded_handle_key_press (emb, event);
}

static gboolean
totem_embedded_construct (TotemEmbedded *emb,
			  GdkNativeWindow xid,
			  int width,
			  int height)
{
	GtkWidget *child, *container, *image;
	GtkWidget *popup_button;
	GtkWidget *menu;
	BvwUseType type;
	GError *err = NULL;

	emb->xml = totem_interface_load ("mozilla-viewer.ui", TRUE,
					 GTK_WINDOW (emb->window), emb);
	g_assert (emb->xml);

	if (xid != 0) {
		g_assert (!emb->hidden);

		emb->window = gtk_plug_new (xid);

		/* Can't do anything before it's realized */
		gtk_widget_realize (emb->window);

		child = GTK_WIDGET (gtk_builder_get_object (emb->xml, "content_box"));
		gtk_container_add (GTK_CONTAINER (emb->window), child);
	} else {
		g_assert (emb->hidden);

		emb->window = GTK_WIDGET (gtk_builder_get_object (emb->xml, "window"));
	}

	if (emb->hidden || emb->audioonly != FALSE)
		type = BVW_USE_TYPE_AUDIO;
	else
		type = BVW_USE_TYPE_VIDEO;

	if (type == BVW_USE_TYPE_VIDEO && emb->controller_hidden != FALSE) {
		emb->bvw = BACON_VIDEO_WIDGET (bacon_video_widget_new
					       (width, height, BVW_USE_TYPE_VIDEO, &err));
	} else {
		emb->bvw = BACON_VIDEO_WIDGET (bacon_video_widget_new
					       (-1, -1, type, &err));
	}

	/* FIXME: check the UA strings of the legacy plugins themselves */
	/* FIXME: at least hxplayer seems to send different UAs depending on the protocol!? */
	if (emb->user_agent != NULL) {
                bacon_video_widget_set_user_agent (emb->bvw, emb->user_agent);
		g_free (emb->user_agent);
		emb->user_agent = NULL;
	}

        /* FIXMEchpe: this is just the initial value. I think in case e.g. we're doing
         * a playlist, the playlist's URI should become the referrer?
         */
        g_print ("Referrer URI: %s\n", emb->referrer_uri);
        if (emb->referrer_uri != NULL) {
                bacon_video_widget_set_referrer (emb->bvw, emb->referrer_uri);
                g_free (emb->referrer_uri);
                emb->referrer_uri = NULL;
        }

	/* Fullscreen setup */
	emb->fs_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_widget_realize (emb->fs_window);
	gtk_window_set_title (GTK_WINDOW (emb->fs_window), _("Totem Movie Player"));
	gtk_window_set_default_icon_name ("totem");

	emb->fs = totem_fullscreen_new (GTK_WINDOW (emb->fs_window));
	totem_fullscreen_set_video_widget (emb->fs, emb->bvw);
	g_signal_connect (G_OBJECT (emb->fs->exit_button), "clicked",
			  G_CALLBACK (totem_embedded_on_fullscreen_exit), emb);

	emb->pp_fs_button = GTK_WIDGET (gtk_tool_button_new_from_stock 
					(GTK_STOCK_MEDIA_PLAY));
	g_signal_connect (G_OBJECT (emb->pp_fs_button), "clicked",
			  G_CALLBACK (on_play_pause), emb);
	gtk_container_add (GTK_CONTAINER (emb->fs->buttons_box), emb->pp_fs_button);

	/* Connect the keys */
	gtk_widget_add_events (emb->fs_window, GDK_KEY_PRESS_MASK |
			       GDK_KEY_RELEASE_MASK);
	g_signal_connect (G_OBJECT(emb->fs_window), "key_press_event",
			G_CALLBACK (totem_embedded_key_press_event), emb);
	g_signal_connect (G_OBJECT(emb->fs_window), "key_release_event",
			G_CALLBACK (totem_embedded_key_press_event), emb);
	gtk_widget_add_events (GTK_WIDGET (emb->bvw), GDK_KEY_PRESS_MASK | 
			       GDK_KEY_RELEASE_MASK);
	g_signal_connect (G_OBJECT(emb->bvw), "key_press_event",
			G_CALLBACK (totem_embedded_key_press_event), emb);
	g_signal_connect (G_OBJECT(emb->bvw), "key_release_event",
			G_CALLBACK (totem_embedded_key_press_event), emb);

	g_signal_connect (G_OBJECT(emb->bvw), "got-redirect",
			G_CALLBACK (on_got_redirect), emb);
	g_signal_connect (G_OBJECT(emb->bvw), "got-metadata",
			  G_CALLBACK (on_got_metadata), emb);
	g_signal_connect (G_OBJECT (emb->bvw), "eos",
			G_CALLBACK (on_eos_event), emb);
	g_signal_connect (G_OBJECT (emb->bvw), "error",
			G_CALLBACK (on_error_event), emb);
	g_signal_connect (G_OBJECT(emb->bvw), "button-press-event",
			G_CALLBACK (on_video_button_press_event), emb);
	g_signal_connect (G_OBJECT(emb->bvw), "tick",
			G_CALLBACK (on_tick), emb);
	g_signal_connect (G_OBJECT (emb->bvw), "buffering",
			  G_CALLBACK (on_buffering), emb);
	g_signal_connect (G_OBJECT (emb->bvw), "download-buffering",
			  G_CALLBACK (on_download_buffering), emb);

	g_signal_connect (G_OBJECT (emb->bvw), "notify::volume",
			  G_CALLBACK (property_notify_cb_volume), emb);

	container = GTK_WIDGET (gtk_builder_get_object (emb->xml, "video_box"));
	if (type == BVW_USE_TYPE_VIDEO) {
		gtk_container_add (GTK_CONTAINER (container), GTK_WIDGET (emb->bvw));
		/* FIXME: why can't this wait until the whole window is realised? */
		gtk_widget_realize (GTK_WIDGET (emb->bvw));
		gtk_widget_show (GTK_WIDGET (emb->bvw));

		/* Let us know when the size has been allocated for the video widget */
		g_signal_connect_after (G_OBJECT (emb->bvw), "size-allocate",
					G_CALLBACK (video_widget_size_allocate_cb), emb->bvw);
	} else if (emb->audioonly != FALSE) {
		gtk_widget_hide (container);
	}

	emb->seek = GTK_WIDGET (gtk_builder_get_object (emb->xml, "time_hscale"));
	emb->seekadj = gtk_range_get_adjustment (GTK_RANGE (emb->seek));
	gtk_range_set_adjustment (GTK_RANGE (emb->fs->seek), emb->seekadj);
	g_signal_connect (emb->seek, "button-press-event",
			  G_CALLBACK (on_seek_start), emb);
	g_signal_connect (emb->seek, "button-release-event",
			  G_CALLBACK (cb_on_seek), emb);
	g_signal_connect (emb->fs->seek, "button-press-event",
			  G_CALLBACK (on_seek_start), emb);
	g_signal_connect (emb->fs->seek, "button-release-event",
			  G_CALLBACK (cb_on_seek), emb);

	emb->pp_button = GTK_WIDGET (gtk_builder_get_object (emb->xml, "pp_button"));
	image = gtk_image_new_from_stock (GTK_STOCK_MEDIA_PLAY, GTK_ICON_SIZE_MENU);
	gtk_button_set_image (GTK_BUTTON (emb->pp_button), image);
	g_signal_connect (G_OBJECT (emb->pp_button), "clicked",
			  G_CALLBACK (on_play_pause), emb);

	popup_button = GTK_WIDGET (gtk_builder_get_object (emb->xml, "popup_button"));
	g_signal_connect (G_OBJECT (popup_button), "toggled",
			  G_CALLBACK (on_popup_button_toggled), emb);
	g_signal_connect (G_OBJECT (popup_button), "button-press-event",
			  G_CALLBACK (on_popup_button_button_pressed), emb);

	emb->volume = GTK_WIDGET (gtk_builder_get_object (emb->xml, "volume_button"));
	gtk_scale_button_set_adjustment (GTK_SCALE_BUTTON (emb->fs->volume),
					 gtk_scale_button_get_adjustment 
					 (GTK_SCALE_BUTTON (emb->volume)));
	gtk_button_set_focus_on_click (GTK_BUTTON (emb->volume), FALSE);
	g_signal_connect (G_OBJECT (emb->volume), "value-changed",
			  G_CALLBACK (cb_vol), emb);

	emb->statusbar = TOTEM_STATUSBAR (gtk_builder_get_object (emb->xml, "statusbar"));
	gtk_statusbar_set_has_resize_grip (GTK_STATUSBAR (emb->statusbar), FALSE);

	if (!emb->hidden) {
		gtk_widget_set_size_request (emb->window, width, height);
		emb->scrsaver = totem_scrsaver_new ();
		g_object_set (emb->scrsaver,
			      "reason", _("Playing a movie"),
			      NULL);
	}

#ifdef MATE_ENABLE_DEBUG
	child = GTK_WIDGET (gtk_builder_get_object (emb->xml, "controls"));
	g_signal_connect_after (G_OBJECT (child), "size-allocate", G_CALLBACK (controls_size_allocate_cb), NULL);
#endif

	if (emb->controller_hidden != FALSE) {
		child = GTK_WIDGET (gtk_builder_get_object (emb->xml, "controls"));
		gtk_widget_hide (child);
	}

	if (!emb->show_statusbar) {
		gtk_widget_hide (GTK_WIDGET (emb->statusbar));
	}

	/* Try to make controls smaller */
	{
        	GtkRcStyle *rcstyle;

		rcstyle = gtk_rc_style_new ();
		rcstyle->xthickness = rcstyle->ythickness = 0;

		gtk_widget_modify_style (emb->pp_button, rcstyle);
		
		child = GTK_WIDGET (gtk_builder_get_object (emb->xml, "time_hscale"));
		gtk_widget_modify_style (child, rcstyle);

		gtk_widget_modify_style (emb->volume, rcstyle);

		g_object_unref (rcstyle);
	}

	/* Create the cursor before setting the state */
	if (!emb->hidden) {
		emb->cursor = gdk_cursor_new_for_display
			(gtk_widget_get_display (emb->window),
			 GDK_HAND2);
	}
	totem_embedded_set_state (emb, TOTEM_STATE_STOPPED);

	if (!emb->hidden) {
		gtk_widget_show (emb->window);
	}

	/* popup */
	emb->menuxml = totem_interface_load ("mozilla-viewer.ui", TRUE,
					     GTK_WINDOW (emb->window), emb);
	g_assert (emb->menuxml);

	menu = GTK_WIDGET (gtk_builder_get_object (emb->menuxml, "menu"));
	g_signal_connect (G_OBJECT (menu), "unmap",
			  G_CALLBACK (on_popup_menu_unmap), emb);

	/* Set the logo and the button glow */
	if (!emb->hidden && emb->autostart == FALSE) {
		totem_glow_button_set_glow (TOTEM_GLOW_BUTTON (emb->pp_button), TRUE);
		if (gtk_widget_get_direction (GTK_WIDGET (emb->bvw)) == GTK_TEXT_DIR_LTR)
			totem_embedded_set_logo_by_name (emb, "gtk-media-play-ltr");
		else
			totem_embedded_set_logo_by_name (emb, "gtk-media-play-rtl");
	} else {
		totem_embedded_set_logo_by_name (emb, "totem");
	}

	return TRUE;
}

static gboolean
totem_embedded_set_window (TotemEmbedded *embedded,
			   const char *controls,
			   guint window,
			   int width,
			   int height,
			   GError **error)
{
	g_print ("Viewer: SetWindow XID %u size %d:%d\n", window, width, height);

	if (strcmp (controls, "All") != 0 &&
	    strcmp (controls, "ImageWindow") != 0) {
		g_set_error (error,
			     TOTEM_EMBEDDED_ERROR_QUARK,
			     TOTEM_EMBEDDED_SETWINDOW_UNSUPPORTED_CONTROLS,
			     "Unsupported controls '%s'", controls);
		return FALSE;
	}

	if (embedded->window != NULL) {
		g_warning ("Viewer: Already have a window!");

		g_set_error_literal (error,
                                     TOTEM_EMBEDDED_ERROR_QUARK,
                                     TOTEM_EMBEDDED_SETWINDOW_HAVE_WINDOW,
                                     "Already have a window");
		return FALSE;
	}

	if (window == 0) {
		g_set_error_literal (error,
                                     TOTEM_EMBEDDED_ERROR_QUARK,
                                     TOTEM_EMBEDDED_SETWINDOW_INVALID_XID,
                                     "Invalid XID");
		return FALSE;
	}

	embedded->width = width;
	embedded->height = height;

	totem_embedded_construct (embedded, (GdkNativeWindow) window,
				  width, height);

	return TRUE;
}

static gboolean
totem_embedded_unset_window (TotemEmbedded *embedded,
			    guint window,
			    GError **error)
{
	g_warning ("UnsetWindow unimplemented");
	return TRUE;
}

static void
entry_metadata_foreach (const char *key,
			const char *value,
			gpointer data)
{
	if (g_ascii_strcasecmp (key, "url") == 0)
		return;
	g_print ("\t%s = '%s'\n", key, value);
}

static void
entry_parsed (TotemPlParser *parser,
	     const char *uri,
	     GHashTable *metadata,
	     gpointer data)
{
	TotemEmbedded *emb = (TotemEmbedded *) data;
	TotemPlItem *item;
	int duration;

	if (g_str_has_prefix (uri, "file://") != FALSE
	    && g_str_has_prefix (emb->base_uri, "file://") == FALSE) {
		g_print ("not adding URI '%s' (local file referenced from remote location)\n", uri);
		return;
	}

	g_print ("added URI '%s'\n", uri);
	g_hash_table_foreach (metadata, (GHFunc) entry_metadata_foreach, NULL);

	/* Skip short advert streams */
	duration = totem_pl_parser_parse_duration (g_hash_table_lookup (metadata, TOTEM_PL_PARSER_FIELD_DURATION), FALSE);
	if (duration == 0)
		return;

	item = g_slice_new0 (TotemPlItem);
	item->uri = g_strdup (uri);
	item->title = g_strdup (g_hash_table_lookup (metadata, TOTEM_PL_PARSER_FIELD_TITLE));
	item->duration = duration;
	item->starttime = totem_pl_parser_parse_duration (g_hash_table_lookup (metadata, TOTEM_PL_PARSER_FIELD_STARTTIME), FALSE);

	emb->playlist = g_list_prepend (emb->playlist, item);
}

static gboolean
totem_embedded_push_parser (gpointer data)
{
	TotemEmbedded *emb = (TotemEmbedded *) data;
	TotemPlParser *parser;
	TotemPlParserResult res;
	GFile *file;

	emb->parser_id = 0;

	parser = totem_pl_parser_new ();
	g_object_set (parser, "force", TRUE,
		      "disable-unsafe", TRUE,
		      NULL);
	g_signal_connect (parser, "entry-parsed", G_CALLBACK (entry_parsed), emb);
	res = totem_pl_parser_parse_with_base (parser, emb->current_uri,
					       emb->base_uri, FALSE);
	g_object_unref (parser);

	/* Delete the temporary file created in
	 * totem_embedded_set_playlist */
	file = g_file_new_for_uri (emb->current_uri);
	g_file_delete (file, NULL, NULL);
	g_object_unref (file);

	if (res != TOTEM_PL_PARSER_RESULT_SUCCESS) {
		//FIXME show a proper error message
		switch (res) {
		case TOTEM_PL_PARSER_RESULT_UNHANDLED:
			g_print ("url '%s' unhandled\n", emb->current_uri);
			break;
		case TOTEM_PL_PARSER_RESULT_ERROR:
			g_print ("error handling url '%s'\n", emb->current_uri);
			break;
		case TOTEM_PL_PARSER_RESULT_IGNORED:
			g_print ("ignored url '%s'\n", emb->current_uri);
			break;
		default:
			g_assert_not_reached ();
			;;
		}
	}

	/* Check if we have anything in the playlist now */
	if (emb->playlist == NULL && res != TOTEM_PL_PARSER_RESULT_SUCCESS) {
		g_message ("Couldn't parse playlist '%s'", emb->current_uri);
		totem_embedded_set_error (emb, BVW_ERROR_EMPTY_FILE,
					  _("No playlist or playlist empty"));
		totem_embedded_set_error_logo (emb, NULL);
		return FALSE;
	} else if (emb->playlist == NULL) {
		g_message ("Playlist empty");
		totem_embedded_set_logo_by_name (emb, "totem");
		return FALSE;
	}

	emb->playlist = g_list_reverse (emb->playlist);
	emb->num_items = g_list_length (emb->playlist);

	/* Launch the first item */
	totem_embedded_open_playlist_item (emb, emb->playlist);

	/* don't run again */
	return FALSE;
}

static char *arg_user_agent = NULL;
static char *arg_mime_type = NULL;
static char **arg_remaining = NULL;
static char *arg_referrer = NULL;
static gboolean arg_no_controls = FALSE;
static gboolean arg_statusbar = FALSE;
static gboolean arg_hidden = FALSE;
static gboolean arg_is_playlist = FALSE;
static gboolean arg_repeat = FALSE;
static gboolean arg_no_autostart = FALSE;
static gboolean arg_audioonly = FALSE;
static TotemPluginType arg_plugin_type = TOTEM_PLUGIN_TYPE_LAST;

static gboolean
parse_plugin_type (const gchar *option_name,
	           const gchar *value,
	           gpointer data,
	           GError **error)
{
	const char types[TOTEM_PLUGIN_TYPE_LAST][12] = {
		"gmp",
		"narrowspace",
		"mully",
		"cone"
	};
	TotemPluginType type;

	for (type = 0; type < TOTEM_PLUGIN_TYPE_LAST; ++type) {
		if (strcmp (value, types[type]) == 0) {
			arg_plugin_type = type;
			return TRUE;
		}
	}

	g_print ("Unknown plugin type '%s'\n", value);
	exit (1);
}

static GOptionEntry option_entries [] =
{
	{ TOTEM_OPTION_PLUGIN_TYPE, 0, 0, G_OPTION_ARG_CALLBACK, parse_plugin_type, NULL, NULL },
	{ TOTEM_OPTION_USER_AGENT, 0, 0, G_OPTION_ARG_STRING, &arg_user_agent, NULL, NULL },
	{ TOTEM_OPTION_MIMETYPE, 0, 0, G_OPTION_ARG_STRING, &arg_mime_type, NULL, NULL },
	{ TOTEM_OPTION_CONTROLS_HIDDEN, 0, 0, G_OPTION_ARG_NONE, &arg_no_controls, NULL, NULL },
	{ TOTEM_OPTION_STATUSBAR, 0, 0, G_OPTION_ARG_NONE, &arg_statusbar, NULL, NULL },
	{ TOTEM_OPTION_HIDDEN, 0, 0, G_OPTION_ARG_NONE, &arg_hidden, NULL, NULL },
	{ TOTEM_OPTION_PLAYLIST, 0, 0, G_OPTION_ARG_NONE, &arg_is_playlist, NULL, NULL },
	{ TOTEM_OPTION_REPEAT, 0, 0, G_OPTION_ARG_NONE, &arg_repeat, NULL, NULL },
	{ TOTEM_OPTION_NOAUTOSTART, 0, 0, G_OPTION_ARG_NONE, &arg_no_autostart, NULL, NULL },
	{ TOTEM_OPTION_AUDIOONLY, 0, 0, G_OPTION_ARG_NONE, &arg_audioonly, NULL, NULL },
        { TOTEM_OPTION_REFERRER, 0, 0, G_OPTION_ARG_STRING, &arg_referrer, NULL, NULL },
	{ G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY /* STRING? */, &arg_remaining, NULL },
	{ NULL }
};

#include "totem-plugin-viewer-interface.h"

int main (int argc, char **argv)
{
	TotemEmbedded *emb;
	DBusGProxy *proxy;
	DBusGConnection *conn;
	guint res;
	GError *e = NULL;
	char svcname[256];

	bindtextdomain (GETTEXT_PACKAGE, MATELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	g_thread_init (NULL);

	g_set_application_name (_("Movie browser plugin"));
	gtk_window_set_default_icon_name ("totem");

#ifdef MATE_ENABLE_DEBUG
	{
		int i;
		g_print ("Viewer: PID %d, args: ", getpid ());
		for (i = 0; i < argc; i++)
			g_print ("%s ", argv[i]);
		g_print ("\n");
	}
#endif

	if (XInitThreads () == 0)
	{
		gtk_init (&argc, &argv);
		totem_embedded_error_and_exit (_("Could not initialize the thread-safe libraries."), _("Verify your system installation. The Totem plugin will now exit."), NULL);
	}

	dbus_g_thread_init ();

#ifdef MATE_ENABLE_DEBUG
	{
		const char *env;

		env = g_getenv ("TOTEM_EMBEDDED_GDB");
		if (env && g_ascii_strtoull (env, NULL, 10) == 1) {
			char *gdbargv[4];
			char *gdb;
			GError *gdberr = NULL;
			int gdbargc = 0;

			gdb = g_strdup_printf ("gdb %s %d", argv[0], getpid ());

			gdbargv[gdbargc++] = "/usr/bin/mate-terminal";
			gdbargv[gdbargc++] = "-e";
			gdbargv[gdbargc++] = gdb;
			gdbargv[gdbargc++] = NULL;

			if (!g_spawn_async (NULL,
					    gdbargv,
					    NULL /* env */,
					    0,
					    NULL, NULL,
					    NULL,
					    &gdberr)) {
				g_warning ("Failed to spawn debugger: %s", gdberr->message);
				g_error_free (gdberr);
			} else {
				g_print ("Sleeping....\n");
				g_usleep (10* 1000 * 1000); /* 10s */
			}
		}
	}
#endif

	if (!gtk_init_with_args (&argc, &argv, NULL, option_entries, GETTEXT_PACKAGE, &e))
	{
		g_print ("%s\n", e->message);
		g_error_free (e);
		exit (1);
	}

	if (arg_audioonly != FALSE)
		g_setenv("PULSE_PROP_media.role", "video", TRUE);
	else
		g_setenv("PULSE_PROP_media.role", "music", TRUE);

        // FIXME check that ALL necessary params were given!
	if (arg_plugin_type == TOTEM_PLUGIN_TYPE_LAST) {
		g_warning ("Plugin type is required\n");
		exit (1);
	}

	bacon_video_widget_init_backend (NULL, NULL);

	dbus_g_object_type_install_info (TOTEM_TYPE_EMBEDDED,
					 &dbus_glib_totem_embedded_object_info);
	g_snprintf (svcname, sizeof (svcname),
		    TOTEM_PLUGIN_VIEWER_NAME_TEMPLATE, getpid());

	if (!(conn = dbus_g_bus_get (DBUS_BUS_SESSION, &e))) {
		g_warning ("Failed to get DBUS connection: %s", e->message);
		g_error_free (e);
		exit (1);
	}

	proxy = dbus_g_proxy_new_for_name (conn,
					   "org.freedesktop.DBus",
					   "/org/freedesktop/DBus",
					   "org.freedesktop.DBus");
	g_assert (proxy != NULL);

	if (!dbus_g_proxy_call (proxy, "RequestName", &e,
	     			G_TYPE_STRING, svcname,
				G_TYPE_UINT, DBUS_NAME_FLAG_PROHIBIT_REPLACEMENT,
				G_TYPE_INVALID,
				G_TYPE_UINT, &res,
				G_TYPE_INVALID)) {
		g_warning ("RequestName for '%s'' failed: %s\n",
			   svcname, e->message);
		g_error_free (e);

		exit (1);
	}

	emb = g_object_new (TOTEM_TYPE_EMBEDDED, NULL);

	emb->state = TOTEM_STATE_INVALID;
	emb->width = -1;
	emb->height = -1;
	emb->controller_hidden = arg_no_controls;
	emb->show_statusbar = arg_statusbar;
	emb->current_uri = arg_remaining ? arg_remaining[0] : NULL;
	emb->mimetype = arg_mime_type;
	emb->hidden = arg_hidden;
	emb->is_playlist = arg_is_playlist;
	emb->repeat = arg_repeat;
	emb->autostart = !arg_no_autostart;
	emb->audioonly = arg_audioonly;
	emb->type = arg_plugin_type;
        emb->user_agent = arg_user_agent;
        emb->referrer_uri = arg_referrer;

	/* FIXME: register this BEFORE requesting the service name? */
	dbus_g_connection_register_g_object (conn,
					     TOTEM_PLUGIN_VIEWER_DBUS_PATH,
					     G_OBJECT (emb));
	emb->conn = conn;

	/* If we're hidden, construct a hidden window;
	 * else wait to be plugged in.
	 */
	if (emb->hidden) {
		totem_embedded_construct (emb, 0, -1, -1);
	}

	gtk_main ();

	g_object_unref (emb);

	g_message ("Viewer [PID %d]: exiting\n", getpid ());

	return 0;
}
