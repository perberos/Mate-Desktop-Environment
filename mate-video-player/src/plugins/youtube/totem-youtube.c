/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2009 Philip Withnall <philip@tecnocode.co.uk>
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
 * The Totem project hereby grant permission for non-GPL compatible GStreamer
 * plugins to be used and distributed together with GStreamer and Totem. This
 * permission are above and beyond the permissions granted by the GPL license
 * Totem is covered by.
 *
 * See license_change file for details.
 */

#include <config.h>
#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <gdata/gdata.h>
#include <libsoup/soup.h>

#include "totem-plugin.h"
#include "totem.h"
#include "totem-video-list.h"
#include "totem-interface.h"
#include "backend/bacon-video-widget.h"
#include "totem-cell-renderer-video.h"

/* Notebook pages */
enum {
	SEARCH_TREE_VIEW = 0,
	RELATED_TREE_VIEW,
	NUM_TREE_VIEWS
};

#define DEVELOPER_KEY	"AI39si5D82T7zgTGS9fmUQAZ7KO5EvKNN_Hf1yoEPf1bpVOTD0At-z7Ovgjupke6o0xdS4drF8SDLfjfmuIXLQQNdE3foPfIdg"
#define CLIENT_ID	"ytapi-MATE-Totem-444fubtt-1"
#define MAX_RESULTS	10
#define THUMBNAIL_WIDTH	180
#define PULSE_INTERVAL	200

#define TOTEM_TYPE_YOUTUBE_PLUGIN		(totem_youtube_plugin_get_type ())
#define TOTEM_YOUTUBE_PLUGIN(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), TOTEM_TYPE_YOUTUBE_PLUGIN, TotemYouTubePlugin))
#define TOTEM_YOUTUBE_PLUGIN_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), TOTEM_TYPE_YOUTUBE_PLUGIN, TotemYouTubePluginClass))
#define TOTEM_IS_YOUTUBE_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), TOTEM_TYPE_YOUTUBE_PLUGIN))
#define TOTEM_IS_YOUTUBE_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), TOTEM_TYPE_YOUTUBE_PLUGIN))
#define TOTEM_YOUTUBE_PLUGIN_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), TOTEM_TYPE_YOUTUBE_PLUGIN, TotemYouTubePluginClass))

typedef struct {
	TotemPlugin parent;
	Totem *totem;
	GDataYouTubeService *service;
	SoupSession *session;
	BaconVideoWidget *bvw;

	guint current_tree_view;
	GDataQuery *query[NUM_TREE_VIEWS];
	GCancellable *cancellable[NUM_TREE_VIEWS];
	GRegex *regex;
	gboolean button_down;
	GDataYouTubeVideo *playing_video;

	GtkEntry *search_entry;
	GtkButton *search_button;
	GtkProgressBar *progress_bar[NUM_TREE_VIEWS];
	gfloat progress_bar_increment[NUM_TREE_VIEWS];
	GtkNotebook *notebook;
	GtkWidget *vbox;
	GtkAdjustment *vadjust[NUM_TREE_VIEWS];
	GtkListStore *list_store[NUM_TREE_VIEWS];
	GtkTreeView *tree_view[NUM_TREE_VIEWS];
	GtkWidget *cancel_button;
} TotemYouTubePlugin;

typedef struct {
	TotemPluginClass parent_class;
} TotemYouTubePluginClass;

G_MODULE_EXPORT GType register_totem_plugin	(GTypeModule *module);
GType totem_youtube_plugin_get_type		(void) G_GNUC_CONST;

static gboolean impl_activate			(TotemPlugin *plugin, TotemObject *totem, GError **error);
static void impl_deactivate			(TotemPlugin *plugin, TotemObject *totem);

/* GtkBuilder callbacks */
void notebook_switch_page_cb (GtkNotebook *notebook, gpointer *page, guint page_num, TotemYouTubePlugin *self);
void search_button_clicked_cb (GtkButton *button, TotemYouTubePlugin *self);
void cancel_button_clicked_cb (GtkButton *button, TotemYouTubePlugin *self);
void search_entry_activate_cb (GtkEntry *entry, TotemYouTubePlugin *self);
gboolean button_press_event_cb (GtkWidget *widget, GdkEventButton *event, TotemYouTubePlugin *self);
gboolean button_release_event_cb (GtkWidget *widget, GdkEventButton *event, TotemYouTubePlugin *self);
void open_in_web_browser_activate_cb (GtkAction *action, TotemYouTubePlugin *self);
void value_changed_cb (GtkAdjustment *adjustment, TotemYouTubePlugin *self);
gboolean starting_video_cb (TotemVideoList *video_list, GtkTreePath *path, TotemYouTubePlugin *self);

TOTEM_PLUGIN_REGISTER (TotemYouTubePlugin, totem_youtube_plugin)

static void
totem_youtube_plugin_class_init (TotemYouTubePluginClass *klass)
{
	TotemPluginClass *plugin_class = TOTEM_PLUGIN_CLASS (klass);

	plugin_class->activate = impl_activate;
	plugin_class->deactivate = impl_deactivate;
}

static void
totem_youtube_plugin_init (TotemYouTubePlugin *plugin)
{
	/* Nothing to see here; move along */
}

/* ----------------------------------------------------------------------------------------------------------------- */
/* Copied from http://bugzilla.mate.org/show_bug.cgi?id=575900 while waiting for them to be committed to gdk-pixbuf */

typedef	struct {
	gint width;
	gint height;
	gboolean preserve_aspect_ratio;
} AtScaleData;

static void
new_from_stream_thread (GSimpleAsyncResult *result,
			GInputStream       *stream,
			GCancellable       *cancellable)
{
	GdkPixbuf *pixbuf;
	AtScaleData *data;
	GError *error = NULL;

	/* If data != NULL, we're scaling the pixbuf while loading it */
	data = g_simple_async_result_get_op_res_gpointer (result);
	if (data != NULL)
		pixbuf = gdk_pixbuf_new_from_stream_at_scale (stream, data->width, data->height, data->preserve_aspect_ratio, cancellable, &error);
	else
		pixbuf = gdk_pixbuf_new_from_stream (stream, cancellable, &error);

	g_simple_async_result_set_op_res_gpointer (result, NULL, NULL);

	/* Set the new pixbuf as the result, or error out */
	if (pixbuf == NULL) {
		g_simple_async_result_set_from_error (result, error);
		g_error_free (error);
	} else {
		g_simple_async_result_set_op_res_gpointer (result, pixbuf, g_object_unref);
	}
}

 /**
 * gdk_pixbuf_new_from_stream_at_scale_async:
 * @stream: a #GInputStream from which to load the pixbuf
 * @width: the width the image should have or -1 to not constrain the width
 * @height: the height the image should have or -1 to not constrain the height
 * @preserve_aspect_ratio: %TRUE to preserve the image's aspect ratio
 * @cancellable: optional #GCancellable object, %NULL to ignore
 * @callback: a #GAsyncReadyCallback to call when the the pixbuf is loaded
 * @user_data: the data to pass to the callback function 
 *
 * Creates a new pixbuf by asynchronously loading an image from an input stream.  
 *
 * For more details see gdk_pixbuf_new_from_stream_at_scale(), which is the synchronous
 * version of this function.
 *
 * When the operation is finished, @callback will be called in the main thread.
 * You can then call gdk_pixbuf_new_from_stream_finish() to get the result of the operation.
 *
 * Since: 2.18
 **/
static void
totem_gdk_pixbuf_new_from_stream_at_scale_async (GInputStream        *stream,
					   gint                 width,
					   gint                 height,
					   gboolean             preserve_aspect_ratio,
					   GCancellable        *cancellable,
					   GAsyncReadyCallback  callback,
					   gpointer             user_data)
{
	GSimpleAsyncResult *result;
	AtScaleData *data;

	g_return_if_fail (G_IS_INPUT_STREAM (stream));
	g_return_if_fail (callback != NULL);

	data = g_new (AtScaleData, 1);
	data->width = width;
	data->height = height;
	data->preserve_aspect_ratio = preserve_aspect_ratio;

	result = g_simple_async_result_new (G_OBJECT (stream), callback, user_data, totem_gdk_pixbuf_new_from_stream_at_scale_async);
	g_simple_async_result_set_op_res_gpointer (result, data, (GDestroyNotify) g_free);
	g_simple_async_result_run_in_thread (result, (GSimpleAsyncThreadFunc) new_from_stream_thread, G_PRIORITY_DEFAULT, cancellable);
	g_object_unref (result);
}

/**
 * gdk_pixbuf_new_from_stream_async:
 * @stream: a #GInputStream from which to load the pixbuf
 * @cancellable: optional #GCancellable object, %NULL to ignore
 * @callback: a #GAsyncReadyCallback to call when the the pixbuf is loaded
 * @user_data: the data to pass to the callback function 
 *
 * Creates a new pixbuf by asynchronously loading an image from an input stream.  
 *
 * For more details see gdk_pixbuf_new_from_stream(), which is the synchronous
 * version of this function.
 *
 * When the operation is finished, @callback will be called in the main thread.
 * You can then call gdk_pixbuf_new_from_stream_finish() to get the result of the operation.
 *
 * Since: 2.18
 **/
static void
totem_gdk_pixbuf_new_from_stream_async (GInputStream        *stream,
				  GCancellable        *cancellable,
				  GAsyncReadyCallback  callback,
				  gpointer             user_data)
{
	GSimpleAsyncResult *result;

	g_return_if_fail (G_IS_INPUT_STREAM (stream));
	g_return_if_fail (callback != NULL);

	result = g_simple_async_result_new (G_OBJECT (stream), callback, user_data, totem_gdk_pixbuf_new_from_stream_async);
	g_simple_async_result_run_in_thread (result, (GSimpleAsyncThreadFunc) new_from_stream_thread, G_PRIORITY_DEFAULT, cancellable);
	g_object_unref (result);
}

/**
 * gdk_pixbuf_new_from_stream_finish:
 * @async_result: a #GAsyncResult
 * @error: a #GError, or %NULL
 *
 * Finishes an asynchronous pixbuf creation operation started with
 * gdk_pixbuf_new_from_stream_async().
 *
 * Return value: a #GdkPixbuf or %NULL on error. Free the returned
 * object with g_object_unref().
 *
 * Since: 2.18
 **/
static GdkPixbuf *
totem_gdk_pixbuf_new_from_stream_finish (GAsyncResult  *async_result,
				   GError       **error)
{
	GdkPixbuf *pixbuf;
	GSimpleAsyncResult *result = G_SIMPLE_ASYNC_RESULT (async_result);

	g_return_val_if_fail (G_IS_ASYNC_RESULT (async_result), NULL);
	g_warn_if_fail (g_simple_async_result_get_source_tag (result) == totem_gdk_pixbuf_new_from_stream_async ||
			g_simple_async_result_get_source_tag (result) == totem_gdk_pixbuf_new_from_stream_at_scale_async);

	if (g_simple_async_result_propagate_error (result, error))
		return NULL;

	pixbuf = GDK_PIXBUF (g_simple_async_result_get_op_res_gpointer (result));
	if (pixbuf != NULL)
		return g_object_ref (pixbuf);

	return NULL;
}

/* ----------------------------------------------------------------------------------------------------------------- */

static void
set_up_tree_view (TotemYouTubePlugin *self, GtkBuilder *builder, guint key)
{
	GtkUIManager *ui_manager;
	GtkActionGroup *action_group;
	GtkAction *action, *menu_item;
	GtkWidget *vscroll, *tree_view;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;

	/* Add the cell renderer. This can't be done with GtkBuilder, because it unavoidably sets the expand parameter to FALSE */
	/* TODO: Depends on bug #453692 */
	renderer = GTK_CELL_RENDERER (totem_cell_renderer_video_new (TRUE));
	column = GTK_TREE_VIEW_COLUMN (gtk_builder_get_object (builder,
							       (key == SEARCH_TREE_VIEW) ? "yt_treeview_search_column" : "yt_treeview_related_column"));
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column, renderer, "thumbnail", 0, "title", 1, NULL);

	/* Give the video lists a handle to Totem and connect their scrollbar signals */
	if (key == SEARCH_TREE_VIEW) {
		tree_view = GTK_WIDGET (gtk_builder_get_object (builder, "yt_treeview_search"));
		vscroll = gtk_scrolled_window_get_vscrollbar (GTK_SCROLLED_WINDOW (gtk_builder_get_object (builder, "yt_scrolled_window_search")));
		self->list_store[key] = GTK_LIST_STORE (gtk_builder_get_object (builder, "yt_list_store_search"));
		self->tree_view[key] = GTK_TREE_VIEW (tree_view);
		self->progress_bar[key] = GTK_PROGRESS_BAR (gtk_builder_get_object (builder, "yt_progress_bar_search"));
	} else {
		tree_view = GTK_WIDGET (gtk_builder_get_object (builder, "yt_treeview_related"));
		vscroll = gtk_scrolled_window_get_vscrollbar (GTK_SCROLLED_WINDOW (gtk_builder_get_object (builder, "yt_scrolled_window_related")));
		self->list_store[key] = GTK_LIST_STORE (gtk_builder_get_object (builder, "yt_list_store_related"));
		self->tree_view[key] = GTK_TREE_VIEW (tree_view);
		self->progress_bar[key] = GTK_PROGRESS_BAR (gtk_builder_get_object (builder, "yt_progress_bar_related"));
	}
	g_object_set (tree_view, "totem", self->totem, NULL);
	g_signal_connect (vscroll, "button-press-event", G_CALLBACK (button_press_event_cb), self);
	g_signal_connect (vscroll, "button-release-event", G_CALLBACK (button_release_event_cb), self);

	/* Add the extra popup menu options. This is done here rather than in the UI file, because it's done for multiple treeviews;
	 * if it were done in the UI file, the same action group would be used multiple times, which GTK+ doesn't like. */
	ui_manager = totem_video_list_get_ui_manager (TOTEM_VIDEO_LIST (tree_view));
	action_group = gtk_action_group_new ("youtube-action-group");
	action = gtk_action_new ("open-in-web-browser", _("_Open in Web Browser"), _("Open the video in your web browser"), "gtk-jump-to");
	gtk_action_group_add_action_with_accel (action_group, action, NULL);

	gtk_ui_manager_insert_action_group (ui_manager, action_group, 1);
	gtk_ui_manager_add_ui (ui_manager, gtk_ui_manager_new_merge_id (ui_manager),
			       "/ui/totem-video-list-popup/",
			       "open-in-web-browser",
			       "open-in-web-browser",
			       GTK_UI_MANAGER_MENUITEM,
			       FALSE);

	menu_item = gtk_ui_manager_get_action (ui_manager, "/ui/totem-video-list-popup/open-in-web-browser");
	g_signal_connect (menu_item, "activate", G_CALLBACK (open_in_web_browser_activate_cb), self);

	/* Connect to more scroll events */
	self->vadjust[key] = gtk_tree_view_get_vadjustment (GTK_TREE_VIEW (tree_view));
	g_signal_connect (self->vadjust[key], "value-changed", G_CALLBACK (value_changed_cb), self);

	self->cancel_button = GTK_WIDGET (gtk_builder_get_object (builder, "yt_cancel_button"));
}

static gboolean
impl_activate (TotemPlugin *plugin, TotemObject *totem, GError **error)
{
	TotemYouTubePlugin *self = TOTEM_YOUTUBE_PLUGIN (plugin);
	GtkWindow *main_window;
	GtkBuilder *builder;
	guint i;

	self->totem = g_object_ref (totem);
	self->bvw = BACON_VIDEO_WIDGET (totem_get_video_widget (totem));

	/* Set up the interface */
	main_window = totem_get_main_window (totem);
	builder = totem_plugin_load_interface (plugin, "youtube.ui", TRUE, main_window, self);
	g_object_unref (main_window);

	self->search_entry = GTK_ENTRY (gtk_builder_get_object (builder, "yt_search_entry"));
	self->search_button = GTK_BUTTON (gtk_builder_get_object (builder, "yt_search_button"));
	self->notebook = GTK_NOTEBOOK (gtk_builder_get_object (builder, "yt_notebook"));

	/* Set up the tree view pages */
	for (i = 0; i < NUM_TREE_VIEWS; i++)
		set_up_tree_view (self, builder, i);
	self->current_tree_view = SEARCH_TREE_VIEW;

	self->vbox = GTK_WIDGET (gtk_builder_get_object (builder, "yt_vbox"));
	gtk_widget_show_all (self->vbox);

	/* Add the sidebar page */
	totem_add_sidebar_page (totem, "youtube", _("YouTube"), self->vbox);
	g_object_unref (builder);

	return TRUE;
}

static void
impl_deactivate	(TotemPlugin *plugin, TotemObject *totem)
{
	guint i;
	TotemYouTubePlugin *self = TOTEM_YOUTUBE_PLUGIN (plugin);

	totem_remove_sidebar_page (self->totem, "youtube");

	for (i = 0; i < NUM_TREE_VIEWS; i++) {
		/* Cancel any queries which are still underway */
		if (self->cancellable[i] != NULL)
			g_cancellable_cancel (self->cancellable[i]);

		if (self->query[i] != NULL)
			g_object_unref (self->query[i]);
	}

	if (self->playing_video != NULL)
		g_object_unref (self->playing_video);
	if (self->service != NULL)
		g_object_unref (self->service);
	if (self->session != NULL)
		g_object_unref (self->session);
	g_object_unref (self->bvw);
	g_object_unref (self->totem);
	if (self->regex != NULL)
		g_regex_unref (self->regex);
}

typedef struct {
	TotemYouTubePlugin *plugin;
	guint tree_view;
} ProgressBarData;

static gboolean
progress_bar_pulse_cb (ProgressBarData *data)
{
	TotemYouTubePlugin *self = data->plugin;

	if (self->progress_bar_increment[data->tree_view] != 0.0) {
		g_slice_free (ProgressBarData, data);
		return FALSE; /* The first entry has been retrieved */
	}

	gtk_progress_bar_pulse (self->progress_bar[data->tree_view]);
	return TRUE;
}

static void
set_progress_bar_text (TotemYouTubePlugin *self, const gchar *text, guint tree_view)
{
	ProgressBarData *data;
	GdkCursor *cursor;

	/* Set the cursor to a watch */
	cursor = gdk_cursor_new (GDK_WATCH);
	gdk_window_set_cursor (gtk_widget_get_window (self->vbox), cursor);
	gdk_cursor_unref (cursor);

	/* Call the pulse method */
	data = g_slice_new (ProgressBarData);
	data->plugin = self;
	data->tree_view = tree_view;

	gtk_progress_bar_set_text (self->progress_bar[tree_view], text);
	gtk_progress_bar_set_fraction (self->progress_bar[tree_view], 0.0);
	self->progress_bar_increment[tree_view] = 0.0;
	g_timeout_add (PULSE_INTERVAL, (GSourceFunc) progress_bar_pulse_cb, data);
}

static void
increment_progress_bar_fraction (TotemYouTubePlugin *self, guint tree_view)
{
	gdouble new_value = MIN (gtk_progress_bar_get_fraction (self->progress_bar[tree_view]) + self->progress_bar_increment[tree_view], 1.0);

	g_debug ("Incrementing progress bar by %f (new value: %f)", self->progress_bar_increment[tree_view], new_value);
	gtk_progress_bar_set_fraction (self->progress_bar[tree_view], new_value);

	/* Change the text if the operation's been cancelled */
	if (self->cancellable[tree_view] == NULL || g_cancellable_is_cancelled (self->cancellable[tree_view]) == TRUE)
		gtk_progress_bar_set_text (self->progress_bar[tree_view], _("Cancelling query…"));

	/* Update the UI */
	if (gtk_progress_bar_get_fraction (self->progress_bar[tree_view]) == 1.0) {
		/* The entire search process (including loading thumbnails and t params) is finished, so update the progress bar */
		gdk_window_set_cursor (gtk_widget_get_window (self->vbox), NULL);
		gtk_progress_bar_set_text (self->progress_bar[tree_view], "");
		gtk_progress_bar_set_fraction (self->progress_bar[tree_view], 0.0);
	}
}

typedef struct {
	TotemYouTubePlugin *plugin;
	GDataEntry *entry;
	GtkTreePath *path;
	guint tree_view;
	SoupMessage *message;
	gulong cancelled_id;
	GCancellable *cancellable;
} TParamData;

/* Ranked list of formats to prefer, from http://en.wikipedia.org/wiki/YouTube#Quality_and_codecs */
static const guint fmt_preferences[] = {
	17, /* least preferred (lowest connection speed needed) */
	5,
	18,
	18,
	34,
	35,
	43,
	43,
	22,
	45,
	45,
	37,
	38 /* most preferred (highest connection speed needed) */
};

static void
resolve_t_param_cb (SoupSession *session, SoupMessage *message, TParamData *data)
{
	gchar *video_uri = NULL;
	const gchar *video_id, *contents;
	gsize length;
	GMatchInfo *match_info;
	GtkTreeIter iter;
	TotemYouTubePlugin *self = data->plugin;

	/* Prevent cancellation */
	g_cancellable_disconnect (data->cancellable, data->cancelled_id);

	/* Finish loading the page */
	if (message->status_code != SOUP_STATUS_OK) {
		GtkWindow *window;

		/* Bail out if the operation was cancelled */
		if (message->status_code == SOUP_STATUS_CANCELLED)
			goto free_data;

		/* Couldn't load the page contents; error */
		window = totem_get_main_window (data->plugin->totem);
		totem_interface_error (_("Error Looking Up Video URI"), message->response_body->data, window);
		g_object_unref (window);
		goto free_data;
	}

	contents = message->response_body->data;
	length = message->response_body->length;

	video_id = gdata_youtube_video_get_video_id (GDATA_YOUTUBE_VIDEO (data->entry));

	/* Check for the fmt_url_map parameter */
	g_regex_match (self->regex, contents, 0, &match_info);
	if (g_match_info_matches (match_info) == TRUE) {
		gchar *fmt_url_map_escaped, *fmt_url_map;
		gchar **mappings, **i;
		GHashTable *fmt_table;
		gint connection_speed;

		/* We have a match */
		fmt_url_map_escaped = g_match_info_fetch (match_info, 1);
		fmt_url_map = g_uri_unescape_string (fmt_url_map_escaped, NULL);
		g_free (fmt_url_map_escaped);

		/* The fmt_url_map parameter is in the following format:
		 *   fmt1|uri1,fmt2|uri2,fmt3|uri3,...
		 * where fmtN is an identifier for the audio and video encoding and resolution as described here:
		 * (http://en.wikipedia.org/wiki/YouTube#Quality_and_codecs) and uriN is the playback URI for that format.
		 *
		 * We parse it into a hash table from format to URI, and use that against a ranked list of preferred formats (based on the user's
		 * connection speed) to determine the URI to use. */
		fmt_table = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_free);
		mappings = g_strsplit (fmt_url_map, ",", 0);

		for (i = mappings; *i != NULL; i++) {
			/* For the moment we just take the first format we get */
			gchar **mapping;
			gint fmt;

			mapping = g_strsplit (*i, "|", 2);

			if (mapping[0] == NULL || mapping[1] == NULL) {
				g_warning ("Bad format-URI mapping: %s", *i);
				g_strfreev (mapping);
				continue;
			}

			fmt = atoi (mapping[0]);

			if (fmt < 1) {
				g_warning ("Badly-formed format: %s", mapping[0]);
				g_strfreev (mapping);
				continue;
			}

			g_hash_table_insert (fmt_table, GUINT_TO_POINTER ((guint) fmt), g_strdup (mapping[1]));
			g_strfreev (mapping);
		}

		g_strfreev (mappings);

		/* Starting with the highest connection speed we support, look for video URIs matching our connection speed. */
		connection_speed = MIN (bacon_video_widget_get_connection_speed (self->bvw), (gint) G_N_ELEMENTS (fmt_preferences) - 1);
		for (; connection_speed >= 0; connection_speed--) {
			guint idx = (guint) connection_speed;
			video_uri = g_strdup (g_hash_table_lookup (fmt_table, GUINT_TO_POINTER (fmt_preferences [idx])));

			/* Have we found a match yet? */
			if (video_uri != NULL) {
				g_debug ("Using video URI for format %u (connection speed %u)", fmt_preferences[idx], idx);
				break;
			}
		}

		g_hash_table_destroy (fmt_table);
	}

	/* Fallback */
	if (video_uri == NULL) {
		GDataMediaContent *content;

		/* We don't have a match, which is odd; fall back to the FLV URI as advertised by the YouTube API */
		content = GDATA_MEDIA_CONTENT (gdata_youtube_video_look_up_content (GDATA_YOUTUBE_VIDEO (data->entry),
										    "application/x-shockwave-flash"));
		if (content != NULL) {
			video_uri = g_strdup (gdata_media_content_get_uri (content));
			g_debug ("Couldn't find the t param of entry %s; falling back to its FLV URI (\"%s\")", video_id, video_uri);
		} else {
			/* Cop out */
			g_warning ("Couldn't find the t param of entry %s or its FLV URI.", video_uri);
			video_uri = NULL;
		}
	}

	g_match_info_free (match_info);

	/* Update the tree view with the new MRL */
	if (gtk_tree_model_get_iter (GTK_TREE_MODEL (self->list_store[data->tree_view]), &iter, data->path) == TRUE) {
		gtk_list_store_set (self->list_store[data->tree_view], &iter, 2, video_uri, -1);
		g_debug ("Updated list store with new video URI (\"%s\") for entry %s", video_uri, video_id);
	}

	g_free (video_uri);

free_data:
	/* Update the progress bar */
	increment_progress_bar_fraction (self, data->tree_view);

	g_object_unref (data->cancellable);
	g_object_unref (data->plugin);
	g_object_unref (data->entry);
	gtk_tree_path_free (data->path);
	g_slice_free (TParamData, data);
}

static void
resolve_t_param_cancelled_cb (GCancellable *cancellable, TParamData *data)
{
	/* This will cause resolve_t_param_cb() to be called, which will free the data */
	soup_session_cancel_message (data->plugin->session, data->message, SOUP_STATUS_CANCELLED);
}

static void
resolve_t_param (TotemYouTubePlugin *self, GDataEntry *entry, GtkTreeIter *iter, guint tree_view, GCancellable *cancellable)
{
	GDataLink *page_link;
	TParamData *data;

	/* We have to get the t parameter from the actual HTML video page, since Google changed how their URIs work */
	page_link = gdata_entry_look_up_link (entry, GDATA_LINK_ALTERNATE);
	g_assert (page_link != NULL);

	data = g_slice_new (TParamData);
	data->plugin = g_object_ref (self);
	data->entry = g_object_ref (entry);
	data->path = gtk_tree_model_get_path (GTK_TREE_MODEL (self->list_store[tree_view]), iter);
	data->tree_view = tree_view;
	data->cancellable = g_object_ref (cancellable);

	data->message = soup_message_new (SOUP_METHOD_GET, gdata_link_get_uri (page_link));
	data->cancelled_id = g_cancellable_connect (cancellable, (GCallback) resolve_t_param_cancelled_cb, data, NULL);

	/* Send the message. Consumes a reference to data->message after resolve_t_param_cb() finishes */
	soup_session_queue_message (self->session, data->message, (SoupSessionCallback) resolve_t_param_cb, data);
}

typedef struct {
	TotemYouTubePlugin *plugin;
	GtkTreePath *path;
	guint tree_view;
	GCancellable *cancellable;
} ThumbnailData;

static void
thumbnail_loaded_cb (GObject *source_object, GAsyncResult *result, ThumbnailData *data)
{
	GdkPixbuf *thumbnail;
	GError *error = NULL;
	GtkTreeIter iter;
	TotemYouTubePlugin *self = data->plugin;

	/* Finish loading the thumbnail */
	thumbnail = totem_gdk_pixbuf_new_from_stream_finish (result, &error);

	if (thumbnail == NULL) {
		/* Bail out if the operation was cancelled */
		if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED) == TRUE) {
			g_error_free (error);
			goto free_data;
		}

		/* Don't display an error message, since this isn't really meant to happen */
		g_warning ("Error loading video thumbnail: %s", error->message);
		g_error_free (error);
		goto free_data;
	}

	g_debug ("Finished creating thumbnail from stream");

	/* Update the tree view */
	if (gtk_tree_model_get_iter (GTK_TREE_MODEL (self->list_store[data->tree_view]), &iter, data->path) == TRUE) {
		gtk_list_store_set (self->list_store[data->tree_view], &iter, 0, thumbnail, -1);
		g_debug ("Updated list store with new thumbnail");
	}

	g_object_unref (thumbnail);

free_data:
	/* Update the progress bar */
	increment_progress_bar_fraction (self, data->tree_view);

	g_object_unref (data->plugin);
	g_object_unref (data->cancellable);
	gtk_tree_path_free (data->path);
	g_slice_free (ThumbnailData, data);
}

static void
thumbnail_opened_cb (GObject *source_object, GAsyncResult *result, ThumbnailData *data)
{
	GFile *thumbnail_file;
	GFileInputStream *input_stream;
	GError *error = NULL;

	/* Finish opening the thumbnail */
	thumbnail_file = G_FILE (source_object);
	input_stream = g_file_read_finish (thumbnail_file, result, &error);

	if (input_stream == NULL) {
		/* Don't display an error message, since this isn't really meant to happen */
		g_warning ("Error loading video thumbnail: %s", error->message);
		g_error_free (error);
		return;
	}

	/* NOTE: There's no need to reset the cancellable before using it again, as we'll have bailed before now if it was ever cancelled. */

	g_debug ("Creating thumbnail from stream");
	totem_gdk_pixbuf_new_from_stream_at_scale_async (G_INPUT_STREAM (input_stream), THUMBNAIL_WIDTH, -1, TRUE,
							 data->cancellable, (GAsyncReadyCallback) thumbnail_loaded_cb, data);
	g_object_unref (input_stream);
}

typedef struct {
	TotemYouTubePlugin *plugin;
	guint tree_view;
	GCancellable *query_cancellable;
	GCancellable *t_param_cancellable;
	GCancellable *thumbnail_cancellable;
} QueryData;

static void
query_data_free (QueryData *data)
{
	if (data->thumbnail_cancellable != NULL)
		g_object_unref (data->thumbnail_cancellable);
	if (data->t_param_cancellable != NULL)
		g_object_unref (data->t_param_cancellable);

	g_object_unref (data->query_cancellable);
	g_object_unref (data->plugin);

	g_slice_free (QueryData, data);
}

static void
query_finished_cb (GObject *source_object, GAsyncResult *result, QueryData *data)
{
	GtkWindow *window;
	GDataFeed *feed;
	GError *error = NULL;
	TotemYouTubePlugin *self = data->plugin;

	g_debug ("Search finished!");

	feed = gdata_service_query_finish (GDATA_SERVICE (self->service), result, &error);

	/* Stop the progress bar; a little hacky, but it works */
	self->progress_bar_increment[data->tree_view] = 1.0;
	increment_progress_bar_fraction (self, data->tree_view);

	if (feed != NULL) {
		/* Success! */
		g_object_unref (feed);
		query_data_free (data);

		return;
	}

	/* Bail out if the operation was cancelled */
	if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED) == TRUE) {
		/* Cancel the t-param and thumbnail threads, if applicable */
		if (data->t_param_cancellable != NULL)
			g_cancellable_cancel (data->t_param_cancellable);
		if (data->thumbnail_cancellable != NULL)
			g_cancellable_cancel (data->thumbnail_cancellable);

		goto finish;
	}

	/* Error! */
	window = totem_get_main_window (data->plugin->totem);
	if (g_error_matches (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR) == TRUE) {
		/* Hide the ugly technical message libgdata gives behind a nice one telling them it's out of date (which it likely is
		 * if we're receiving a protocol error). */
		totem_interface_error (_("Error Searching for Videos"),
				       _("The response from the server could not be understood. "
				         "Please check you are running the latest version of libgdata."), window);
	} else {
		/* Spew out the error message as provided */
		totem_interface_error (_("Error Searching for Videos"), error->message, window);
	}

	g_object_unref (window);

finish:
	g_error_free (error);
	query_data_free (data);

	return;
}

static void
query_progress_cb (GDataEntry *entry, guint entry_key, guint entry_count, QueryData *data)
{
	GList *thumbnails;
	GDataMediaThumbnail *thumbnail = NULL;
	gint delta = G_MININT;
	GtkTreeIter iter;
	const gchar *title, *id;
	GtkProgressBar *progress_bar;
	TotemYouTubePlugin *self = data->plugin;

	/* Add the entry to the tree view */
	title = gdata_entry_get_title (entry);
	id = gdata_youtube_video_get_video_id (GDATA_YOUTUBE_VIDEO (entry));

	gtk_list_store_append (self->list_store[data->tree_view], &iter);
	gtk_list_store_set (self->list_store[data->tree_view], &iter,
			    0, NULL, /* the thumbnail will be downloaded asynchronously and added to the tree view later */
			    1, title,
			    2, NULL, /* the video URI will be resolved asynchronously and added to the tree view later */
			    3, entry,
			    -1);
	g_debug ("Added entry %s to tree view (title: \"%s\")", id, title);

	/* Update the progress bar; we have three steps for each entry in the results: the entry, its thumbnail, and its t parameter */
	g_assert (entry_count > 0);
	progress_bar = self->progress_bar[data->tree_view];
	self->progress_bar_increment[data->tree_view] = 1.0 / (entry_count * 3.0);
	g_debug ("Setting progress_bar_increment to 1.0 / (%u * 3.0) = %f", entry_count, self->progress_bar_increment[data->tree_view]);
	gtk_progress_bar_set_fraction (progress_bar, gtk_progress_bar_get_fraction (progress_bar) + self->progress_bar_increment[data->tree_view]);

	/* Resolve the t parameter for the video, which is required before it can be played */
	/* This will be cancelled if the main query is cancelled, in query_finished_cb() */
	data->t_param_cancellable = g_cancellable_new ();
	resolve_t_param (self, entry, &iter, data->tree_view, data->t_param_cancellable);

	/* Download the entry's thumbnail, ready for adding it to the tree view.
	 * Find the thumbnail size which is closest to the wanted size (THUMBNAIL_WIDTH), so that we:
	 * a) avoid fuzzy images due to scaling up, and
	 * b) avoid downloading too much just to scale down by a factor of 10. */
	thumbnails = gdata_youtube_video_get_thumbnails (GDATA_YOUTUBE_VIDEO (entry));
	for (; thumbnails != NULL; thumbnails = thumbnails->next) {
		gint new_delta;
		GDataMediaThumbnail *current_thumb = (GDataMediaThumbnail*) thumbnails->data;

		g_debug ("%u pixel wide thumbnail available for entry %s", gdata_media_thumbnail_get_width (current_thumb), id);

		new_delta = gdata_media_thumbnail_get_width (current_thumb) - THUMBNAIL_WIDTH;
		if (delta == 0) {
			break;
		} else if ((delta == G_MININT) ||
			   (delta < 0 && new_delta > delta) ||
			   (delta > 0 && new_delta > 0 && new_delta < delta)) {
			delta = new_delta;
			thumbnail = current_thumb;
			g_debug ("Choosing a %u pixel wide thumbnail (delta: %i) for entry %s",
				 gdata_media_thumbnail_get_width (current_thumb), new_delta, id);
		}
	}

	if (thumbnail != NULL) {
		GFile *thumbnail_file;
		ThumbnailData *t_data;

		t_data = g_slice_new (ThumbnailData);
		t_data->plugin = g_object_ref (self);
		t_data->path = gtk_tree_model_get_path (GTK_TREE_MODEL (self->list_store[data->tree_view]), &iter);
		t_data->tree_view = data->tree_view;

		/* We can use the same cancellable for reading the file and making a pixbuf out of it, as they're consecutive operations */
		/* This will be cancelled if the main query is cancelled, in query_finished_cb() */
		data->thumbnail_cancellable = g_cancellable_new ();
		t_data->cancellable = g_object_ref (data->thumbnail_cancellable);

		g_debug ("Starting thumbnail download for entry %s", id);
		thumbnail_file = g_file_new_for_uri (gdata_media_thumbnail_get_uri (thumbnail));
		g_file_read_async (thumbnail_file, G_PRIORITY_DEFAULT, data->thumbnail_cancellable,
				   (GAsyncReadyCallback) thumbnail_opened_cb, t_data);
		g_object_unref (thumbnail_file);
	}
}

/* Called when self->cancellable[tree_view] is destroyed (for either tree view) */
static void
cancellable_notify_cb (TotemYouTubePlugin *self, GCancellable *old_cancellable)
{
	guint i;

	/* Disable the "Cancel" button, if it applies to the current tree view */
	if (self->cancellable[self->current_tree_view] == old_cancellable)
		gtk_widget_set_sensitive (self->cancel_button, FALSE);

	/* NULLify the cancellable */
	for (i = 0; i < NUM_TREE_VIEWS; i++) {
		if (self->cancellable[i] == old_cancellable)
			self->cancellable[i] = NULL;
	}
}

static void
set_current_operation (TotemYouTubePlugin *self, guint tree_view, GCancellable *cancellable)
{
	/* Cancel previous searches on this tree view */
	if (self->cancellable[tree_view] != NULL)
		g_cancellable_cancel (self->cancellable[tree_view]);

	/* Make this the current cancellable action for the given tab */
	g_object_weak_ref (G_OBJECT (cancellable), (GWeakNotify) cancellable_notify_cb, self);
	self->cancellable[tree_view] = cancellable;

	/* Enable the "Cancel" button if it applies to the current tree view */
	if (self->current_tree_view == tree_view)
		gtk_widget_set_sensitive (self->cancel_button, TRUE);
}

static void
execute_query (TotemYouTubePlugin *self, guint tree_view, gboolean clear_tree_view)
{
	QueryData *data;

	/* Set up the query */
	data = g_slice_new (QueryData);
	data->plugin = g_object_ref (self);
	data->tree_view = tree_view;
	data->query_cancellable = g_cancellable_new ();
	data->t_param_cancellable = NULL;
	data->thumbnail_cancellable = NULL;

	/* Make this the current cancellable action for the given tab */
	set_current_operation (self, tree_view, data->query_cancellable);

	/* Clear the tree views */
	if (clear_tree_view == TRUE)
		gtk_list_store_clear (self->list_store[tree_view]);

	if (tree_view == SEARCH_TREE_VIEW) {
		gdata_youtube_service_query_videos_async (self->service, self->query[tree_view], data->query_cancellable,
							  (GDataQueryProgressCallback) query_progress_cb, data,
							  (GAsyncReadyCallback) query_finished_cb, data);
	} else {
		gdata_youtube_service_query_related_async (self->service, self->playing_video, self->query[tree_view], data->query_cancellable,
							   (GDataQueryProgressCallback) query_progress_cb, data,
							   (GAsyncReadyCallback) query_finished_cb, data);
	}
}

void
search_button_clicked_cb (GtkButton *button, TotemYouTubePlugin *self)
{
	const gchar *search_terms;

	search_terms = gtk_entry_get_text (self->search_entry);
	g_debug ("Searching for \"%s\"", search_terms);

	/* Focus the "Search" page */
	gtk_notebook_set_current_page (self->notebook, SEARCH_TREE_VIEW);

	/* Update the UI */
	set_progress_bar_text (self, _("Fetching search results…"), SEARCH_TREE_VIEW);

	/* Clear details pertaining to related videos, since we're doing a new search */
	gtk_list_store_clear (self->list_store[RELATED_TREE_VIEW]);
	if (self->playing_video != NULL)
		g_object_unref (self->playing_video);
	self->playing_video = NULL;

	/* If this is the first query, set up some stuff which we didn't do before to save memory */
	if (self->query[SEARCH_TREE_VIEW] == NULL) {
		/* If this is the first query, compile the regex used to resolve the t param. Doing this here rather than when
		 * activating the plugin means we don't waste cycles if the plugin's never used. It also means we don't waste
		 * cycles repeatedly creating new regexes for each video whose t param we resolve. */
		/* We're looking for a line of the form:
		 * var swfHTML = (isIE) ? "<object...econds=194&t=vjVQa1PpcFP36LLlIaDqZIG1w6e30b-7WVBgsQLLA3s%3D&rv.6.id=OzLjC6Pm... */
		self->regex = g_regex_new ("swfHTML = .*&fmt_url_map=([^&]+)&", G_REGEX_OPTIMIZE, 0, NULL);
		g_assert (self->regex != NULL);

		/* Set up the GData service (needed for the tree views' queries) */
		self->service = gdata_youtube_service_new (DEVELOPER_KEY, CLIENT_ID);

		/* Set up network timeouts, if they're supported by our version of libgdata.
		 * This will return from queries with %GDATA_SERVICE_ERROR_NETWORK_ERROR if network operations take longer than 30 seconds. */
#ifdef HAVE_LIBGDATA_0_7
		gdata_service_set_timeout (GDATA_SERVICE (self->service), 30);
#endif /* HAVE_LIBGDATA_0_7 */

		/* Set up the queries */
		self->query[SEARCH_TREE_VIEW] = gdata_query_new_with_limits (NULL, 0, MAX_RESULTS);
		self->query[RELATED_TREE_VIEW] = gdata_query_new_with_limits (NULL, 0, MAX_RESULTS);

		/* Lazily create the SoupSession used in resolve_t_param() */
		self->session = soup_session_async_new ();
	}

	/* Do the query */
	gdata_query_set_q (self->query[SEARCH_TREE_VIEW], search_terms);
	execute_query (self, SEARCH_TREE_VIEW, TRUE);
}

void
cancel_button_clicked_cb (GtkButton *button, TotemYouTubePlugin *self)
{
	/* It's possible for the operation to finish (and consequently the cancellable to disappear) while the GtkButton is deciding whether the
	 * user is actually pressing it (in its timeout). */
	if (self->cancellable[self->current_tree_view] == NULL)
		return;

	g_debug ("Cancelling search");
	g_cancellable_cancel (self->cancellable[self->current_tree_view]);
}

void
search_entry_activate_cb (GtkEntry *entry, TotemYouTubePlugin *self)
{
	search_button_clicked_cb (self->search_button, self);
}

static void
load_related_videos (TotemYouTubePlugin *self)
{
	g_assert (self->playing_video != NULL);
	g_debug ("Loading related videos for %s", gdata_youtube_video_get_video_id (self->playing_video));

	/* Update the UI */
	set_progress_bar_text (self, _("Fetching related videos…"), RELATED_TREE_VIEW);

	/* Clear the existing results and do the query */
	gtk_list_store_clear (self->list_store[RELATED_TREE_VIEW]);
	execute_query (self, RELATED_TREE_VIEW, FALSE);
}

void
notebook_switch_page_cb (GtkNotebook *notebook, gpointer *page, guint page_num, TotemYouTubePlugin *self)
{
	/* Change the tree view */
	self->current_tree_view = page_num;

	/* Sort out the "Cancel" button's sensitivity */
	gtk_widget_set_sensitive (self->cancel_button, (self->cancellable[page_num] != NULL) ? TRUE : FALSE);

	/* If we're changing to the "Related Videos" tree view and have played a video, load
	 * the related videos for that video; but only if the related tree view's empty first */
	if (page_num == RELATED_TREE_VIEW && self->playing_video != NULL &&
	    gtk_tree_model_iter_n_children (GTK_TREE_MODEL (self->list_store[RELATED_TREE_VIEW]), NULL) == 0) {
		load_related_videos (self);
	}
}

void
open_in_web_browser_activate_cb (GtkAction *action, TotemYouTubePlugin *self)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GList *paths, *path;

	selection = gtk_tree_view_get_selection (self->tree_view[self->current_tree_view]);
	paths = gtk_tree_selection_get_selected_rows (selection, &model);

	for (path = paths; path != NULL; path = path->next) {
		GtkTreeIter iter;
		GDataYouTubeVideo *video;
		GDataLink *link;
		GError *error = NULL;

		if (gtk_tree_model_get_iter (model, &iter, (GtkTreePath*) (path->data)) == FALSE)
			continue;

		/* Get the HTML page for the video; its <link rel="alternate" ... /> */
		gtk_tree_model_get (model, &iter, 3, &video, -1);
		link = gdata_entry_look_up_link (GDATA_ENTRY (video), GDATA_LINK_ALTERNATE);
		g_object_unref (video);

		/* Display the page */
		if (gtk_show_uri (gtk_widget_get_screen (GTK_WIDGET (self->bvw)), gdata_link_get_uri (link), GDK_CURRENT_TIME, &error) == FALSE) {
			GtkWindow *window = totem_get_main_window (self->totem);
			totem_interface_error (_("Error Opening Video in Web Browser"), error->message, window);
			g_object_unref (window);
			g_error_free (error);
		}
	}

	g_list_foreach (paths, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (paths);
}

void
value_changed_cb (GtkAdjustment *adjustment, TotemYouTubePlugin *self)
{
	if (self->button_down == FALSE &&
	    gtk_tree_model_iter_n_children (GTK_TREE_MODEL (self->list_store[self->current_tree_view]), NULL) >= MAX_RESULTS &&
	    (gtk_adjustment_get_value (adjustment) + gtk_adjustment_get_page_size (adjustment)) / gtk_adjustment_get_upper (adjustment) > 0.8) {
		/* Only load more results if we're not already querying */
		if (self->cancellable[self->current_tree_view] != NULL)
			return;

		set_progress_bar_text (self, _("Fetching more videos…"), self->current_tree_view);
		gdata_query_next_page (self->query[self->current_tree_view]);
		execute_query (self, self->current_tree_view, FALSE);
	}
}

gboolean
button_press_event_cb (GtkWidget *widget, GdkEventButton *event, TotemYouTubePlugin *self)
{
	self->button_down = TRUE;
	return FALSE;
}

gboolean
button_release_event_cb (GtkWidget *widget, GdkEventButton *event, TotemYouTubePlugin *self)
{
	self->button_down = FALSE;
	value_changed_cb (self->vadjust[self->current_tree_view], self);
	return FALSE;
}

gboolean
starting_video_cb (TotemVideoList *video_list, GtkTreePath *path, TotemYouTubePlugin *self)
{
	GtkTreeIter iter;
	GDataYouTubeVideo *video_entry;

	/* Store the current entry */
	if (gtk_tree_model_get_iter (GTK_TREE_MODEL (self->list_store[self->current_tree_view]), &iter, path) == FALSE)
		return FALSE;
	gtk_tree_model_get (GTK_TREE_MODEL (self->list_store[self->current_tree_view]), &iter, 3, &video_entry, -1);

	if (self->playing_video != NULL)
		g_object_unref (self->playing_video);
	self->playing_video = g_object_ref (video_entry);

	/* If we're currently viewing the related videos page, load the new related videos */
	if (self->current_tree_view == RELATED_TREE_VIEW)
		load_related_videos (self);

	return TRUE;
}
