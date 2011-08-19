/* * -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2007 Openismus GmbH
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
 * See license_change file for details.
 *
 * Author:
 * 	Mathias Hasselmann
 *
 */

#include "config.h"

#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <mateconf/mateconf-client.h>

#include <libepc/consumer.h>
#include <libepc/enums.h>
#include <libepc/publisher.h>
#include <libepc/service-monitor.h>

#include <libepc-ui/progress-window.h>
#include <gio/gio.h>

#include "ev-sidebar.h"
#include "totem-plugin.h"
#include "totem-interface.h"
#include "totem-private.h"
#include "totem.h"

#define TOTEM_TYPE_PUBLISH_PLUGIN		(totem_publish_plugin_get_type ())
#define TOTEM_PUBLISH_PLUGIN(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), TOTEM_TYPE_PUBLISH_PLUGIN, TotemPublishPlugin))
#define TOTEM_PUBLISH_PLUGIN_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), TOTEM_TYPE_PUBLISH_PLUGIN, TotemPublishPluginClass))
#define TOTEM_IS_PUBLISH_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), TOTEM_TYPE_PUBLISH_PLUGIN))
#define TOTEM_IS_PUBLISH_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), TOTEM_TYPE_PUBLISH_PLUGIN))
#define TOTEM_PUBLISH_PLUGIN_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), TOTEM_TYPE_PUBLISH_PLUGIN, TotemPublishPluginClass))

#define TOTEM_PUBLISH_CONFIG_ROOT		MATECONF_PREFIX "/plugins/publish"
#define TOTEM_PUBLISH_CONFIG_NAME		MATECONF_PREFIX "/plugins/publish/name"
#define TOTEM_PUBLISH_CONFIG_PROTOCOL		MATECONF_PREFIX "/plugins/publish/protocol"

enum
{
	NAME_COLUMN,
	INFO_COLUMN,
	LAST_COLUMN
};

typedef struct
{
	TotemPlugin parent;

	TotemObject       *totem;
	MateConfClient       *client;
	GtkWidget         *settings;
	GtkWidget         *scanning;
	GtkBuilder        *ui;

	EpcPublisher      *publisher;
	EpcServiceMonitor *monitor;
	GtkListStore      *neighbours;
	GSList            *playlist;

	guint name_id;
	guint protocol_id;

	guint scanning_id;

	gulong item_added_id;
	gulong item_removed_id;
} TotemPublishPlugin;

typedef struct
{
	TotemPluginClass parent_class;
} TotemPublishPluginClass;

G_MODULE_EXPORT GType register_totem_plugin		   (GTypeModule     *module);
static GType totem_publish_plugin_get_type		   (void);

void totem_publish_plugin_service_name_entry_changed_cb	   (GtkEntry        *entry,
							    gpointer         data);
void totem_publish_plugin_encryption_button_toggled_cb	   (GtkToggleButton *button,
							    gpointer         data);
void totem_publish_plugin_dialog_response_cb		   (GtkDialog       *dialog,
							    gint             response,
							    gpointer         data);
void totem_publish_plugin_neighbours_list_row_activated_cb (GtkTreeView       *view,
							    GtkTreePath       *path,
							    GtkTreeViewColumn *column,
							    gpointer           data);

G_LOCK_DEFINE_STATIC(totem_publish_plugin_lock);
TOTEM_PLUGIN_REGISTER(TotemPublishPlugin, totem_publish_plugin);

static void
totem_publish_plugin_name_changed_cb (MateConfClient *client,
				      guint        id,
				      MateConfEntry  *entry,
				      gpointer     data)
{
	TotemPublishPlugin *self = TOTEM_PUBLISH_PLUGIN (data);
	const gchar *pattern = mateconf_value_get_string (entry->value);
	gchar *name = epc_publisher_expand_name (pattern, NULL);

	epc_publisher_set_service_name (self->publisher, name);

	g_free (name);
}

void
totem_publish_plugin_service_name_entry_changed_cb (GtkEntry *entry,
						    gpointer  data)
{
	TotemPublishPlugin *self = TOTEM_PUBLISH_PLUGIN (data);

	mateconf_client_set_string (self->client,
				 TOTEM_PUBLISH_CONFIG_NAME,
				 gtk_entry_get_text (entry),
				 NULL);
}

void
totem_publish_plugin_encryption_button_toggled_cb (GtkToggleButton *button,
						   gpointer         data)
{
	TotemPublishPlugin *self = TOTEM_PUBLISH_PLUGIN (data);
	gboolean encryption = gtk_toggle_button_get_active (button);

	mateconf_client_set_string (self->client,
				 TOTEM_PUBLISH_CONFIG_PROTOCOL,
				 encryption ? "https" : "http",
				 NULL);
}

static void
totem_publish_plugin_protocol_changed_cb (MateConfClient *client,
					  guint        id,
					  MateConfEntry  *entry,
					  gpointer     data)
{
	TotemPublishPlugin *self = TOTEM_PUBLISH_PLUGIN (data);
	const gchar *protocol_name;
	EpcProtocol protocol;
	GError *error = NULL;

	protocol_name = mateconf_value_get_string (entry->value);
	protocol = epc_protocol_from_name (protocol_name, EPC_PROTOCOL_HTTPS);

	epc_publisher_quit (self->publisher);
	epc_publisher_set_protocol (self->publisher, protocol);
	epc_publisher_run_async (self->publisher, &error);

	if (error) {
		g_warning ("%s: %s", G_STRFUNC, error->message);
		g_error_free (error);
	}
}

static gchar*
totem_publish_plugin_build_key (const gchar *filename)
{
	return g_strconcat ("media/", filename, NULL);
}

static EpcContents*
totem_publish_plugin_playlist_cb (EpcPublisher *publisher,
				  const gchar  *key,
				  gpointer      data)
{
	TotemPublishPlugin *self = TOTEM_PUBLISH_PLUGIN (data);
	GString *buffer = g_string_new (NULL);
	EpcContents *contents = NULL;
	GSList *iter;
	gint i;

	G_LOCK (totem_publish_plugin_lock);

	g_string_append_printf (buffer,
				"[playlist]\nNumberOfEntries=%d\n",
				g_slist_length (self->playlist));

	for (iter = self->playlist, i = 1; iter; iter = iter->next, ++i) {
		gchar *file_key = iter->data;
		gchar *uri;

		uri = epc_publisher_get_uri (publisher, file_key, NULL);

		g_string_append_printf (buffer,
					"File%d=%s\nTitle%d=%s\n",
					i, uri, i, file_key + 6);

		g_free (uri);
	}

	G_UNLOCK (totem_publish_plugin_lock);

	contents = epc_contents_new ("audio/x-scpls",
				     buffer->str, buffer->len,
				     g_free);

	g_string_free (buffer, FALSE);

	return contents;
}

static gboolean
totem_publish_plugin_stream_cb (EpcContents *contents,
				gpointer     buffer,
				gsize       *length,
				gpointer     data)
{
	GInputStream *stream = data;
	gssize size = 65536;

	g_return_val_if_fail (NULL != contents, FALSE);
	g_return_val_if_fail (NULL != length, FALSE);

	if (NULL == data || *length < (gsize)size) {
		*length = MAX (*length, (gsize)size);
		return FALSE;
	}

	size = g_input_stream_read (stream, buffer, size, NULL, NULL);
	if (size == -1) {
		g_input_stream_close (stream, NULL, NULL);
		size = 0;
	}

	*length = size;

	return size > 0;
}

static EpcContents*
totem_publish_plugin_media_cb (EpcPublisher *publisher,
			       const gchar  *key,
			       gpointer      data)
{
	GFileInputStream *stream;
	const gchar *url = data;
	GFile *file;

	file = g_file_new_for_uri (url);
	stream = g_file_read (file, NULL, NULL);
	g_object_unref (file);

	if (stream) {
		EpcContents *output = epc_contents_stream_new (
			NULL, totem_publish_plugin_stream_cb,
			stream, g_object_unref);

		return output;
	}

	return NULL;
}

static void
totem_publish_plugin_rebuild_playlist_cb (TotemPlaylist *playlist,
					  const gchar   *filename,
					  const gchar   *url,
					  gpointer       data)
{
	TotemPublishPlugin *self = TOTEM_PUBLISH_PLUGIN (data);
	gchar *key = totem_publish_plugin_build_key (filename);
	self->playlist = g_slist_prepend (self->playlist, key);
}

static void
totem_publish_plugin_playlist_changed_cb (TotemPlaylist *playlist,
					  gpointer       data)
{
	TotemPublishPlugin *self = TOTEM_PUBLISH_PLUGIN (data);

	G_LOCK (totem_publish_plugin_lock);

	g_slist_foreach (self->playlist, (GFunc) g_free, NULL);
	g_slist_free (self->playlist);
	self->playlist = NULL;

	totem_playlist_foreach (playlist,
				totem_publish_plugin_rebuild_playlist_cb,
				self);

	self->playlist = g_slist_reverse (self->playlist);

	G_UNLOCK (totem_publish_plugin_lock);
}

static void
totem_publish_plugin_playlist_item_added_cb (TotemPlaylist *playlist,
					     const gchar   *filename,
					     const gchar   *url,
					     gpointer       data)
{
	TotemPublishPlugin *self = TOTEM_PUBLISH_PLUGIN (data);
	gchar *key = totem_publish_plugin_build_key (filename);

	epc_publisher_add_handler (self->publisher, key,
				   totem_publish_plugin_media_cb,
				   g_strdup (url), g_free);

	g_free (key);

}

static void
totem_publish_plugin_playlist_item_removed_cb (TotemPlaylist *playlist,
				      const gchar   *filename,
				      const gchar   *url,
				      gpointer       data)
{
	TotemPublishPlugin *self = TOTEM_PUBLISH_PLUGIN (data);
	gchar *key = totem_publish_plugin_build_key (filename);
	epc_publisher_remove (self->publisher, key);
	g_free (key);
}

static void
totem_publish_plugin_service_found_cb (TotemPublishPlugin *self,
				       const gchar        *name,
				       EpcServiceInfo     *info)
{
	GtkTreeIter iter;

	gtk_list_store_append (self->neighbours, &iter);
	gtk_list_store_set (self->neighbours, &iter, NAME_COLUMN, name,
						     INFO_COLUMN, info,
						     -1);
}

static void
totem_publish_plugin_service_removed_cb (TotemPublishPlugin *self,
					 const gchar        *name,
					 const gchar        *type)
{
	GtkTreeModel *model = GTK_TREE_MODEL (self->neighbours);
	GtkTreeIter iter;

	if (gtk_tree_model_get_iter_first (model, &iter)) {
		GSList *path_list = NULL, *path_iter;
		GtkTreePath *path;
		gchar *stored;

		do {
			gtk_tree_model_get (model, &iter, NAME_COLUMN, &stored, -1);

			if (g_str_equal (stored, name)) {
				path = gtk_tree_model_get_path (model, &iter);
				path_list = g_slist_prepend (path_list, path);
			}
		} while (gtk_tree_model_iter_next (model, &iter));

		for (path_iter = path_list; path_iter; path_iter = path_iter->next) {
			path = path_iter->data;

			if (gtk_tree_model_get_iter (model, &iter, path))
				gtk_list_store_remove (self->neighbours, &iter);

			gtk_tree_path_free (path);
		}

		g_slist_free (path_list);
	}
}

static void
totem_publish_plugin_scanning_done_cb (TotemPublishPlugin *self,
				       const gchar        *type)
{
	if (self->scanning_id) {
		g_source_remove (self->scanning_id);
		self->scanning_id = 0;
	}

	if (self->scanning)
		gtk_widget_hide (self->scanning);
}

static void
totem_publish_plugin_load_playlist (TotemPublishPlugin   *self,
				    const EpcServiceInfo *info)
{
	EpcConsumer *consumer = epc_consumer_new (info);
	GKeyFile *keyfile = g_key_file_new ();
	gchar *contents = NULL;
	GError *error = NULL;
	gsize length = 0;

	contents = epc_consumer_lookup (consumer, "playlist.pls", &length, &error);

	if (contents && g_key_file_load_from_data (keyfile, contents, length, G_KEY_FILE_NONE, &error)) {
		gint i, n_entries;

		/* returns zero in case of errors */
		n_entries = g_key_file_get_integer (keyfile, "playlist", "NumberOfEntries", &error);

		if (error)
			goto out;

		ev_sidebar_set_current_page (EV_SIDEBAR (self->totem->sidebar), "playlist");
		totem_playlist_clear (self->totem->playlist);

		for (i = 1; i <= n_entries; ++i) {
			gchar *key, *mrl, *title;

			key = g_strdup_printf ("File%d", i);
			mrl = g_key_file_get_string (keyfile, "playlist", key, NULL);
			g_free (key);

			key = g_strdup_printf ("Title%d", i);
			title = g_key_file_get_string (keyfile, "playlist", key, NULL);
			g_free (key);

			if (mrl)
				totem_playlist_add_mrl (self->totem->playlist, mrl, title, FALSE, NULL, NULL, NULL);

			g_free (title);
			g_free (mrl);
		}
	}

out:
	if (error) {
		g_warning ("Cannot load playlist: %s", error->message);
		g_error_free (error);
	}

	g_key_file_free (keyfile);
	g_free (contents);

	g_object_unref (consumer);
}

void
totem_publish_plugin_neighbours_list_row_activated_cb (GtkTreeView       *view,
						       GtkTreePath       *path,
						       GtkTreeViewColumn *column,
						       gpointer           data)
{
	TotemPublishPlugin *self = TOTEM_PUBLISH_PLUGIN (data);
	EpcServiceInfo *info = NULL;
	GtkTreeIter iter;

	if (gtk_tree_model_get_iter (GTK_TREE_MODEL (self->neighbours), &iter, path)) {
		gtk_tree_model_get (GTK_TREE_MODEL (self->neighbours),
				    &iter, INFO_COLUMN, &info, -1);
		totem_publish_plugin_load_playlist (self, info);
		epc_service_info_unref (info);
	}
}

static gboolean
totem_publish_plugin_scanning_cb (gpointer data)
{
	gtk_progress_bar_pulse (GTK_PROGRESS_BAR (data));
	return TRUE;
}

static GtkWidget*
totem_publish_plugin_create_neigbours_page (TotemPublishPlugin *self)
{
	GtkWidget *page, *list;

	page = GTK_WIDGET (gtk_builder_get_object (self->ui, "neighbours-page"));
	list = GTK_WIDGET (gtk_builder_get_object (self->ui, "neighbours-list"));

	self->scanning = GTK_WIDGET (gtk_builder_get_object (self->ui, "neighbours-scanning"));
	self->scanning_id = g_timeout_add (100, totem_publish_plugin_scanning_cb, self->scanning);

	g_signal_connect_swapped (self->monitor, "service-found",
				  G_CALLBACK (totem_publish_plugin_service_found_cb),
				  self);
	g_signal_connect_swapped (self->monitor, "service-removed",
				  G_CALLBACK (totem_publish_plugin_service_removed_cb),
				  self);
	g_signal_connect_swapped (self->monitor, "scanning-done",
				  G_CALLBACK (totem_publish_plugin_scanning_done_cb),
				  self);

	self->neighbours = gtk_list_store_new (LAST_COLUMN, G_TYPE_STRING, EPC_TYPE_SERVICE_INFO);

	gtk_tree_view_set_model (GTK_TREE_VIEW (list),
				 GTK_TREE_MODEL (self->neighbours));

	gtk_tree_view_append_column (GTK_TREE_VIEW (list),
		gtk_tree_view_column_new_with_attributes (
			NULL, gtk_cell_renderer_text_new (),
			"text", NAME_COLUMN, NULL));

	g_object_ref (page);
	gtk_widget_unparent (page);

	return page;
}

static gboolean
totem_publish_plugin_activate (TotemPlugin  *plugin,
			       TotemObject  *totem,
			       GError      **error)
{
	TotemPublishPlugin *self = TOTEM_PUBLISH_PLUGIN (plugin);
	EpcProtocol protocol = EPC_PROTOCOL_HTTPS;
	GtkWindow *window;
	GError *internal_error = NULL;

	gchar *protocol_name;

	gchar *service_pattern;
	gchar *service_name;

	g_return_val_if_fail (NULL == self->publisher, FALSE);
	g_return_val_if_fail (NULL == self->totem, FALSE);

	G_LOCK (totem_publish_plugin_lock);

	self->totem = g_object_ref (totem);
	self->ui = totem_plugin_load_interface (plugin, "publish-plugin.ui", TRUE, NULL, self);

	window = totem_get_main_window (self->totem);
	epc_progress_window_install (window);
	g_object_unref (window);

	mateconf_client_add_dir (self->client,
			      TOTEM_PUBLISH_CONFIG_ROOT,
			      MATECONF_CLIENT_PRELOAD_ONELEVEL,
			      NULL);

	protocol_name = mateconf_client_get_string (self->client, TOTEM_PUBLISH_CONFIG_PROTOCOL, NULL);
	service_pattern = mateconf_client_get_string (self->client, TOTEM_PUBLISH_CONFIG_NAME, NULL);

	if (!protocol_name) {
		protocol_name = g_strdup ("http");
		mateconf_client_set_string (self->client,
					 TOTEM_PUBLISH_CONFIG_PROTOCOL,
					 protocol_name, NULL);
	}

	if (!service_pattern) {
		service_pattern = g_strdup ("%a of %u on %h");
		mateconf_client_set_string (self->client,
					 TOTEM_PUBLISH_CONFIG_NAME,
					 service_pattern, NULL);
	}

	self->name_id = mateconf_client_notify_add (self->client,
						 TOTEM_PUBLISH_CONFIG_NAME,
						 totem_publish_plugin_name_changed_cb,
						 self, NULL, NULL);
	self->protocol_id = mateconf_client_notify_add (self->client,
						     TOTEM_PUBLISH_CONFIG_PROTOCOL,
						     totem_publish_plugin_protocol_changed_cb,
						     self, NULL, NULL);

	protocol = epc_protocol_from_name (protocol_name, EPC_PROTOCOL_HTTPS);
	service_name = epc_publisher_expand_name (service_pattern, &internal_error);
	g_free (service_pattern);

	if (internal_error) {
		g_warning ("%s: %s", G_STRFUNC, internal_error->message);
		g_clear_error (&internal_error);
	}

	self->monitor = epc_service_monitor_new ("totem", NULL, EPC_PROTOCOL_UNKNOWN);
	epc_service_monitor_set_skip_our_own (self->monitor, TRUE);

	/* Translators: computers on the local network which are publishing their playlists over the network */
	ev_sidebar_add_page (EV_SIDEBAR (self->totem->sidebar), "neighbours", _("Neighbors"),
			     totem_publish_plugin_create_neigbours_page (self));

	self->publisher = epc_publisher_new (service_name, "totem", NULL);
	epc_publisher_set_protocol (self->publisher, protocol);

	g_free (protocol_name);
	g_free (service_name);

	epc_publisher_add_handler (self->publisher, "playlist.pls",
				   totem_publish_plugin_playlist_cb,
				   self, NULL);
	epc_publisher_add_bookmark (self->publisher, "playlist.pls", NULL);

	self->item_added_id = g_signal_connect (self->totem->playlist, "changed",
		G_CALLBACK (totem_publish_plugin_playlist_changed_cb), self);
	self->item_added_id = g_signal_connect (self->totem->playlist, "item-added",
		G_CALLBACK (totem_publish_plugin_playlist_item_added_cb), self);
	self->item_removed_id = g_signal_connect (self->totem->playlist, "item-removed",
		G_CALLBACK (totem_publish_plugin_playlist_item_removed_cb), self);

	G_UNLOCK (totem_publish_plugin_lock);

	totem_playlist_foreach (self->totem->playlist,
				totem_publish_plugin_playlist_item_added_cb, self);

	totem_publish_plugin_playlist_changed_cb (self->totem->playlist, self);

	return epc_publisher_run_async (self->publisher, error);
}

static void
totem_publish_plugin_deactivate (TotemPlugin *plugin,
				 TotemObject *totem)
{
	TotemPublishPlugin *self = TOTEM_PUBLISH_PLUGIN (plugin);
	TotemPlaylist *playlist = NULL;

	G_LOCK (totem_publish_plugin_lock);

	if (self->totem)
		playlist = self->totem->playlist;

	if (self->scanning_id) {
		g_source_remove (self->scanning_id);
		self->scanning_id = 0;
	}

	if (playlist && self->item_added_id) {
		g_signal_handler_disconnect (playlist, self->item_added_id);
		self->item_added_id = 0;
	}

	if (playlist && self->item_removed_id) {
		g_signal_handler_disconnect (playlist, self->item_removed_id);
		self->item_removed_id = 0;
	}

	if (self->monitor) {
		g_object_unref (self->monitor);
		self->monitor = NULL;
	}

	if (self->publisher) {
		epc_publisher_quit (self->publisher);
		g_object_unref (self->publisher);
		self->publisher = NULL;
	}

	if (self->totem) {
		mateconf_client_notify_remove (self->client, self->name_id);
		mateconf_client_notify_remove (self->client, self->protocol_id);
		mateconf_client_remove_dir (self->client, TOTEM_PUBLISH_CONFIG_ROOT, NULL);

		ev_sidebar_remove_page (EV_SIDEBAR (self->totem->sidebar), "neighbours");

		g_object_unref (self->totem);
		self->totem = NULL;
	}

	if (self->settings) {
		gtk_widget_destroy (self->settings);
		self->settings = NULL;
	}

	if (self->ui) {
		g_object_unref (self->ui);
		self->ui = NULL;
	}

	if (self->playlist) {
		g_slist_foreach (self->playlist, (GFunc) g_free, NULL);
		g_slist_free (self->playlist);
		self->playlist = NULL;
	}

	G_UNLOCK (totem_publish_plugin_lock);

	self->scanning = NULL;
}

void
totem_publish_plugin_dialog_response_cb (GtkDialog *dialog,
					 gint       response,
					 gpointer   data G_GNUC_UNUSED)
{
	if (response)
		gtk_widget_hide (GTK_WIDGET (dialog));
}

static GtkWidget*
totem_publish_plugin_create_configure_dialog (TotemPlugin *plugin)
{
	TotemPublishPlugin *self = TOTEM_PUBLISH_PLUGIN (plugin);

	if (NULL == self->settings && GTK_IS_BUILDER (self->ui)) {
		gchar *service_name = mateconf_client_get_string (self->client, TOTEM_PUBLISH_CONFIG_NAME, NULL);
		EpcProtocol protocol = epc_publisher_get_protocol (self->publisher);
		GtkWidget *widget;

		self->settings = g_object_ref (gtk_builder_get_object (self->ui, "publish-settings-dialog"));

		widget = GTK_WIDGET (gtk_builder_get_object (self->ui, "service-name-entry"));
		gtk_entry_set_text (GTK_ENTRY (widget), service_name);

		widget = GTK_WIDGET (gtk_builder_get_object (self->ui, "encryption-button"));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
					      EPC_PROTOCOL_HTTPS == protocol);

		g_free (service_name);
	}

	return self->settings;
}

static void
totem_publish_plugin_init (TotemPublishPlugin *self)
{
	self->client = mateconf_client_get_default ();
}

static void
totem_publish_plugin_dispose (GObject *object)
{
	TotemPublishPlugin *self = TOTEM_PUBLISH_PLUGIN (object);

	if (self->client != NULL) {
		g_object_unref (self->client);
		self->client = NULL;
	}
	totem_publish_plugin_deactivate (TOTEM_PLUGIN (object), NULL);
	G_OBJECT_CLASS (totem_publish_plugin_parent_class)->dispose (object);
}

static void
totem_publish_plugin_class_init (TotemPublishPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	TotemPluginClass *plugin_class = TOTEM_PLUGIN_CLASS (klass);

	object_class->dispose = totem_publish_plugin_dispose;

	plugin_class->activate = totem_publish_plugin_activate;
	plugin_class->deactivate = totem_publish_plugin_deactivate;
	plugin_class->create_configure_dialog = totem_publish_plugin_create_configure_dialog;
}

