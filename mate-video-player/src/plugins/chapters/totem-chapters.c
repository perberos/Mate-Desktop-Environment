/*
 * Copyright (C) 2010 Alexander Saprykin <xelfium@gmail.com>
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
 *
 * The Totem project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and Totem. This
 * permission are above and beyond the permissions granted by the GPL license
 * Totem is covered by.
 */

#include "config.h"

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <glib/gprintf.h>
#include <gio/gio.h>
#include <gmodule.h>
#include <gdk-pixbuf/gdk-pixdata.h>
#include <gdk/gdkkeysyms.h>
#include <mateconf/mateconf-client.h>
#include <unistd.h>
#include <math.h>

#include "bacon-video-widget.h"
#include "totem-plugin.h"
#include "totem-interface.h"
#include "totem.h"
#include "totem-cmml-parser.h"
#include "totem-chapters-utils.h"
#include "totem-edit-chapter.h"

#define TOTEM_TYPE_CHAPTERS_PLUGIN		(totem_chapters_plugin_get_type ())
#define TOTEM_CHAPTERS_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), TOTEM_TYPE_CHAPTERS_PLUGIN, TotemChaptersPlugin))
#define TOTEM_CHAPTERS_PLUGIN_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), TOTEM_TYPE_CHAPTERS_PLUGIN, TotemChaptersPluginClass))
#define TOTEM_IS_CHAPTERS_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), TOTEM_TYPE_CHAPTERS_PLUGIN))
#define TOTEM_IS_CHAPTERS_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), TOTEM_TYPE_CHAPTERS_PLUGIN))
#define TOTEM_CHAPTERS_PLUGIN_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), TOTEM_TYPE_CHAPTERS_PLUGIN, TotemChaptersPluginClass))

#define TOTEM_CHAPTERS_PLUGIN_GET_PRIVATE(obj)	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), TOTEM_TYPE_CHAPTERS_PLUGIN, TotemChaptersPluginPrivate))

#define CHAPTER_TOOLTIP(title, start) g_strdup_printf	( _("<b>Title: </b>%s\n<b>Start time: </b>%s"),	\
							 ( title ), ( start ) )

#define CHAPTER_TITLE(title, start) g_strdup_printf	("<big>%s</big>\n"				\
							 "<small><span foreground='grey'>%s"		\
							 "</span></small>",				\
							 ( title ), ( start ) )
#define ICON_SCALE_RATIO 2

typedef struct {
	GtkWidget	*tree;
	GtkWidget	*add_button,
			*remove_button,
			*save_button,
			*load_button,
			*goto_button,
			*continue_button,
			*list_box,
			*load_box;
	GtkActionGroup	*action_group;
	GtkUIManager	*ui_manager;
	gboolean	was_played;
	GdkPixbuf	*last_frame;
	gint64		last_time;
	gchar		*cmml_mrl;
	gboolean	autoload;
	GCancellable	*cancellable[2];
	MateConfClient	*mateconf;
	guint		autoload_handle_id;
} TotemChaptersPluginPrivate;

typedef struct {
	TotemPlugin			 parent;
	TotemObject			*totem;
	TotemEditChapter		*edit_chapter;
	TotemChaptersPluginPrivate	*priv;
} TotemChaptersPlugin;

typedef struct {
	TotemPluginClass		parent_class;
} TotemChaptersPluginClass;

enum {
	CHAPTERS_PIXBUF_COLUMN = 0,
	CHAPTERS_TITLE_COLUMN,
	CHAPTERS_TOOLTIP_COLUMN,
	CHAPTERS_TITLE_PRIV_COLUMN,
	CHAPTERS_TIME_PRIV_COLUMN,
	CHAPTERS_N_COLUMNS
};

G_MODULE_EXPORT GType register_totem_plugin (GTypeModule *module);
GType totem_chapters_plugin_get_type (void) G_GNUC_CONST;
static gboolean impl_activate (TotemPlugin *plugin, TotemObject *totem, GError **error);
static void impl_deactivate (TotemPlugin *plugin, TotemObject *totem);
static void totem_chapters_plugin_finalize (GObject *object);
static void totem_file_opened_async_cb (TotemObject *totem, const gchar *uri, TotemChaptersPlugin *plugin);
static void totem_file_opened_result_cb (gpointer data, gpointer user_data);
static void totem_file_closed_cb (TotemObject *totem, TotemChaptersPlugin *plugin);
static void add_chapter_to_the_list (gpointer data, gpointer user_data);
static void add_chapter_to_the_list_new (TotemChaptersPlugin *plugin, const gchar *title, gint64 time);
static gboolean check_available_time (TotemChaptersPlugin *plugin, gint64 time);
static GdkPixbuf * get_chapter_pixbuf (GdkPixbuf *src);
static void chapter_edit_dialog_response_cb (GtkDialog *dialog, gint response, TotemChaptersPlugin *plugin);
static void prepare_chapter_edit (GtkCellRenderer *renderer, GtkCellEditable *editable, gchar *path, gpointer user_data);
static void finish_chapter_edit (GtkCellRendererText *renderer, gchar *path, gchar *new_text, gpointer user_data);
static void chapter_selection_changed_cb (GtkTreeSelection *tree_selection, TotemChaptersPlugin *plugin);
static void show_chapter_edit_dialog (TotemChaptersPlugin *plugin);
static void save_chapters_result_cb (gpointer data, gpointer user_data);
static GList * get_chapters_list (TotemChaptersPlugin *plugin);
static gboolean show_popup_menu (TotemChaptersPlugin *plugin, GdkEventButton *event);
static void mateconf_autoload_changed_cb (MateConfClient *mateconf, guint cnx_id, MateConfEntry *entry, TotemChaptersPlugin	*plugin);
static void load_chapters_from_file (const gchar *uri, gboolean from_dialog, TotemChaptersPlugin *plugin);
static void set_no_data_visible (gboolean visible, gboolean show_buttons, TotemChaptersPlugin *plugin);

/* GtkBuilder callbacks */
void add_button_clicked_cb (GtkButton *button, TotemChaptersPlugin *plugin);
void remove_button_clicked_cb (GtkButton *button, TotemChaptersPlugin *plugin);
void save_button_clicked_cb (GtkButton *button, TotemChaptersPlugin *plugin);
void goto_button_clicked_cb (GtkButton *button, TotemChaptersPlugin *plugin);
void tree_view_row_activated_cb (GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, TotemChaptersPlugin *plugin);
gboolean tree_view_button_press_cb (GtkTreeView *tree_view, GdkEventButton *event, TotemChaptersPlugin *plugin);
gboolean tree_view_key_press_cb (GtkTreeView *tree_view, GdkEventKey *event, TotemChaptersPlugin *plugin);
gboolean tree_view_popup_menu_cb (GtkTreeView *tree_view, TotemChaptersPlugin *plugin);
void popup_remove_action_cb (GtkAction *action, TotemChaptersPlugin *plugin);
void popup_goto_action_cb (GtkAction *action, TotemChaptersPlugin *plugin);
void load_button_clicked_cb (GtkButton *button, TotemChaptersPlugin *plugin);
void continue_button_clicked_cb (GtkButton *button, TotemChaptersPlugin *plugin);

TOTEM_PLUGIN_REGISTER (TotemChaptersPlugin, totem_chapters_plugin)

static void
totem_chapters_plugin_class_init (TotemChaptersPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	TotemPluginClass *plugin_class = TOTEM_PLUGIN_CLASS (klass);

	g_type_class_add_private (klass, sizeof (TotemChaptersPluginPrivate));

	object_class->finalize = totem_chapters_plugin_finalize;

	plugin_class->activate = impl_activate;
	plugin_class->deactivate = impl_deactivate;
}

static void
totem_chapters_plugin_init (TotemChaptersPlugin *plugin)
{
	plugin->priv = TOTEM_CHAPTERS_PLUGIN_GET_PRIVATE (plugin);
}

static void
totem_chapters_plugin_finalize (GObject *object)
{
	G_OBJECT_CLASS (totem_chapters_plugin_parent_class)->finalize (object);
}

static GdkPixbuf *
get_chapter_pixbuf (GdkPixbuf *src)
{
	GdkPixbuf	*pixbuf;
	gint		width, height;
	gfloat		pix_width, pix_height;
	gfloat		ratio, new_width;

	gtk_icon_size_lookup (GTK_ICON_SIZE_LARGE_TOOLBAR, &width, &height);
	height *= ICON_SCALE_RATIO;

	if (src != NULL) {
		pix_width = (gfloat) gdk_pixbuf_get_width (src);
		pix_height = (gfloat) gdk_pixbuf_get_height (src);

		/* calc height ratio and apply it to width */
		ratio = pix_height / height;
		new_width = pix_width / ratio;
		width = ceil (new_width);

		/* scale video frame if need */
		pixbuf = gdk_pixbuf_scale_simple (src, width, height, GDK_INTERP_BILINEAR);
	} else {
		/* 16:10 aspect ratio by default */
		new_width = (gfloat) height * 1.6;
		width = ceil (new_width);

		pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, width, height);
		gdk_pixbuf_fill (pixbuf, 0x00000000);
	}

	return pixbuf;
}

static void
add_chapter_to_the_list (gpointer	data,
			 gpointer	user_data)
{
	TotemChaptersPlugin	*plugin;
	GdkPixbuf		*pixbuf;
	GtkTreeIter		iter;
	GtkWidget		*tree;
	GtkTreeStore		*store;
	TotemCmmlClip		*clip;
	gchar			*text, *start, *tip;

	g_return_if_fail (data != NULL);
	g_return_if_fail (TOTEM_IS_CHAPTERS_PLUGIN (user_data));

	plugin = TOTEM_CHAPTERS_PLUGIN (user_data);
	tree = plugin->priv->tree;
	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (plugin->priv->tree)));
	clip = ((TotemCmmlClip *) data);

	/* prepare tooltip data */
	start = totem_cmml_convert_msecs_to_str (clip->time_start);
	tip = CHAPTER_TOOLTIP (clip->title, start);

	/* append clip to the sidebar list */
	gtk_tree_store_append (store, &iter, NULL);
	text = CHAPTER_TITLE (clip->title, start);

	if (G_LIKELY (clip->pixbuf != NULL))
		pixbuf = g_object_ref (clip->pixbuf);
	else
		pixbuf = get_chapter_pixbuf (NULL);

	gtk_tree_store_set (store, &iter,
			    CHAPTERS_TITLE_COLUMN, text,
			    CHAPTERS_TOOLTIP_COLUMN, tip,
			    CHAPTERS_PIXBUF_COLUMN, pixbuf,
			    CHAPTERS_TITLE_PRIV_COLUMN, clip->title,
			    CHAPTERS_TIME_PRIV_COLUMN, clip->time_start,
			    -1);

	g_object_unref (pixbuf);
	g_free (text);
	g_free (start);
	g_free (tip);
}

static void
add_chapter_to_the_list_new (TotemChaptersPlugin	*plugin,
			     const gchar		*title,
			     gint64			time)
{
	GdkPixbuf	*pixbuf;
	GtkTreeIter	iter, cur_iter, res_iter;
	GtkTreeModel	*store;
	gchar		*text, *start, *tip;
	gboolean	valid;
	gint64		cur_time, prev_time = 0;
	gint		iter_count = 0;

	g_return_if_fail (TOTEM_IS_CHAPTERS_PLUGIN (plugin));
	g_return_if_fail (title != NULL);
	g_return_if_fail (time >= 0);

	store = gtk_tree_view_get_model (GTK_TREE_VIEW (plugin->priv->tree));
	valid = gtk_tree_model_get_iter_first (store, &cur_iter);

	while (valid) {
		gtk_tree_model_get (store, &cur_iter,
				    CHAPTERS_TIME_PRIV_COLUMN, &cur_time,
				    -1);

		if (time < cur_time && time > prev_time)
			break;

		iter_count += 1;
		res_iter = cur_iter;
		prev_time = cur_time;

		valid = gtk_tree_model_iter_next (store, &cur_iter);
	}

	/* prepare tooltip data */
	start = totem_cmml_convert_msecs_to_str (time);
	tip = CHAPTER_TOOLTIP (title, start);

	/* insert clip into the sidebar list at proper position */
	if (iter_count > 0)
		gtk_tree_store_insert_after (GTK_TREE_STORE (store), &iter, NULL, &res_iter);
	else
		gtk_tree_store_insert_after (GTK_TREE_STORE (store), &iter, NULL, NULL);

	text = CHAPTER_TITLE (title, start);
	pixbuf = get_chapter_pixbuf (plugin->priv->last_frame);

	gtk_tree_store_set (GTK_TREE_STORE (store), &iter,
			    CHAPTERS_TITLE_COLUMN, text,
			    CHAPTERS_TOOLTIP_COLUMN, tip,
			    CHAPTERS_PIXBUF_COLUMN, pixbuf,
			    CHAPTERS_TITLE_PRIV_COLUMN, title,
			    CHAPTERS_TIME_PRIV_COLUMN, time,
			    -1);

	g_object_unref (pixbuf);
	g_free (text);
	g_free (start);
	g_free (tip);
}

static gboolean
check_available_time (TotemChaptersPlugin	*plugin,
		      gint64			time)
{
	GtkTreeModel	*store;
	GtkTreeIter	iter;
	gboolean	valid;
	gint64		cur_time;

	g_return_val_if_fail (TOTEM_IS_CHAPTERS_PLUGIN (plugin), FALSE);

	store = gtk_tree_view_get_model (GTK_TREE_VIEW (plugin->priv->tree));

	valid = gtk_tree_model_get_iter_first (store, &iter);
	while (valid) {
		gtk_tree_model_get (store, &iter,
				    CHAPTERS_TIME_PRIV_COLUMN, &cur_time,
				    -1);

		if (cur_time == time)
			return FALSE;

		if (cur_time > time)
			return TRUE;

		valid = gtk_tree_model_iter_next (store, &iter);
	}

	return TRUE;
}

static void
totem_file_opened_result_cb (gpointer	data,
			     gpointer	user_data)
{
	TotemCmmlAsyncData	*adata;
	TotemChaptersPlugin	*plugin;

	g_return_if_fail (data != NULL);

	adata = (TotemCmmlAsyncData *) data;
	plugin = TOTEM_CHAPTERS_PLUGIN (adata->user_data);

	if (G_UNLIKELY (!adata->successful)) {
		if (G_UNLIKELY (g_cancellable_is_cancelled (adata->cancellable))) {
			/* if operation was cancelled due to plugin deactivation only clean up */
			g_object_unref (adata->cancellable);
			g_free (adata->file);
			g_free (adata->error);
			g_list_foreach (adata->list, (GFunc) totem_cmml_clip_free, NULL);
			g_list_free (adata->list);
			g_free (adata);
			return;
		} else
			totem_action_error (_("Error while reading file with chapters"),
					    adata->error, plugin->totem);
	}

	if (adata->is_exists && adata->from_dialog) {
		g_free (plugin->priv->cmml_mrl);
		plugin->priv->cmml_mrl = g_strdup (adata->file);
	}

	g_list_foreach (adata->list, (GFunc) add_chapter_to_the_list, plugin);
	g_list_foreach (adata->list, (GFunc) totem_cmml_clip_free, NULL);
	g_list_free (adata->list);

	/* do not show tree if read operation failed */
	set_no_data_visible (!adata->successful || !adata->is_exists, TRUE, plugin);

	g_object_unref (adata->cancellable);
	g_free (adata->file);
	g_free (adata->error);
	g_free (adata);
}

static void
totem_file_opened_async_cb (TotemObject			*totem,
			    const gchar			*uri,
			    TotemChaptersPlugin		*plugin)
{
	gchar	*cmml_file;

	g_return_if_fail (TOTEM_IS_OBJECT (totem));
	g_return_if_fail (TOTEM_IS_CHAPTERS_PLUGIN (plugin));
	g_return_if_fail (uri != NULL);

	if (g_str_has_prefix (uri, "http") != FALSE)
		return;

	cmml_file = totem_change_file_extension (uri, "cmml");
	/* if file has no extension - append it */
	if (cmml_file == NULL)
		cmml_file = g_strconcat (uri, ".cmml", NULL);

	plugin->priv->cmml_mrl = cmml_file;

	if (!plugin->priv->autoload)
		set_no_data_visible (TRUE, TRUE, plugin);
	else
		load_chapters_from_file (cmml_file, FALSE, plugin);
}

static void
totem_file_closed_cb (TotemObject		*totem,
		      TotemChaptersPlugin	*plugin)
{
	GtkTreeStore	*store;

	g_return_if_fail (TOTEM_IS_OBJECT (totem) && TOTEM_IS_CHAPTERS_PLUGIN (plugin));

	store = GTK_TREE_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (plugin->priv->tree)));

	gtk_tree_store_clear (store);

	if (G_UNLIKELY (plugin->edit_chapter != NULL))
		gtk_widget_destroy (GTK_WIDGET (plugin->edit_chapter));

	if (G_UNLIKELY (plugin->priv->last_frame != NULL))
		g_object_unref (G_OBJECT (plugin->priv->last_frame));

	g_free (plugin->priv->cmml_mrl);
	plugin->priv->cmml_mrl = NULL;

	gtk_widget_set_sensitive (plugin->priv->remove_button, FALSE);
	gtk_widget_set_sensitive (plugin->priv->save_button, FALSE);

	set_no_data_visible (TRUE, FALSE, plugin);
}

static void
chapter_edit_dialog_response_cb (GtkDialog		*dialog,
				 gint			response,
				 TotemChaptersPlugin	*plugin)
{
	gchar		*title;

	g_return_if_fail (TOTEM_IS_EDIT_CHAPTER (dialog));
	g_return_if_fail (TOTEM_IS_CHAPTERS_PLUGIN (plugin));

	if (response != GTK_RESPONSE_OK) {
		gtk_widget_destroy (GTK_WIDGET (plugin->edit_chapter));

		if (plugin->priv->last_frame != NULL)
			g_object_unref (G_OBJECT (plugin->priv->last_frame));

		if (plugin->priv->was_played)
			totem_action_play (plugin->totem);
		return;
	}

	gtk_widget_hide (GTK_WIDGET (dialog));

	title = totem_edit_chapter_get_title (TOTEM_EDIT_CHAPTER (dialog));
	add_chapter_to_the_list_new (plugin, title, plugin->priv->last_time);

	gtk_widget_set_sensitive (plugin->priv->save_button, TRUE);

	if (G_LIKELY (plugin->priv->last_frame != NULL))
		g_object_unref (G_OBJECT (plugin->priv->last_frame));

	g_free (title);
	gtk_widget_destroy (GTK_WIDGET (plugin->edit_chapter));

	if (plugin->priv->was_played)
		totem_action_play (plugin->totem);
}

static void
prepare_chapter_edit (GtkCellRenderer	*renderer,
		      GtkCellEditable	*editable,
		      gchar		*path,
		      gpointer		user_data)
{
	TotemChaptersPlugin	*plugin;
	GtkTreeModel		*store;
	GtkTreeIter		iter;
	gchar			*title;
	GtkEntry		*entry;

	g_return_if_fail (GTK_IS_ENTRY (editable));
	g_return_if_fail (TOTEM_IS_CHAPTERS_PLUGIN (user_data));
	g_return_if_fail (path != NULL);

	plugin = TOTEM_CHAPTERS_PLUGIN (user_data);
	entry = GTK_ENTRY (editable);
	store = gtk_tree_view_get_model (GTK_TREE_VIEW (plugin->priv->tree));

	if (G_UNLIKELY (!gtk_tree_model_get_iter_from_string (store, &iter, path)))
		return;

	gtk_tree_model_get (store, &iter, CHAPTERS_TITLE_PRIV_COLUMN, &title, -1);
	gtk_entry_set_text (entry, title);

	g_free (title);
	return;
}

static void
finish_chapter_edit (GtkCellRendererText	*renderer,
		     gchar			*path,
		     gchar			*new_text,
		     gpointer			user_data)
{
	TotemChaptersPlugin	*plugin;
	GtkTreeModel		*store;
	GtkTreeIter		iter;
	gchar			*time_str, *tip, *new_title, *old_title;
	gint64			time;

	g_return_if_fail (TOTEM_IS_CHAPTERS_PLUGIN (user_data));
	g_return_if_fail (new_text != NULL);
	g_return_if_fail (path != NULL);

	plugin = TOTEM_CHAPTERS_PLUGIN (user_data);
	store = gtk_tree_view_get_model (GTK_TREE_VIEW (plugin->priv->tree));

	if (G_UNLIKELY (!gtk_tree_model_get_iter_from_string (store, &iter, path)))
		return;

	gtk_tree_model_get (store, &iter,
			    CHAPTERS_TIME_PRIV_COLUMN, &time,
			    CHAPTERS_TITLE_PRIV_COLUMN, &old_title,
			    -1);

	if (g_strcmp0 (old_title, new_text) == 0) {
		g_free (old_title);
		return;
	}

	time_str = totem_cmml_convert_msecs_to_str (time);
	new_title = CHAPTER_TITLE (new_text, time_str);
	tip = CHAPTER_TOOLTIP (new_text, time_str);

	gtk_tree_store_set (GTK_TREE_STORE (store), &iter,
			    CHAPTERS_TITLE_COLUMN, new_title,
			    CHAPTERS_TOOLTIP_COLUMN, tip,
			    CHAPTERS_TITLE_PRIV_COLUMN, new_text,
			    -1);

	gtk_widget_set_sensitive (plugin->priv->save_button, TRUE);

	g_free (old_title);
	g_free (new_title);
	g_free (tip);
	g_free (time_str);
}

static void
show_chapter_edit_dialog (TotemChaptersPlugin	*plugin)
{
	GtkWindow		*main_window;
	BaconVideoWidget	*bvw;
	gint64			time;

	g_return_if_fail (TOTEM_IS_CHAPTERS_PLUGIN (plugin));

	if (G_UNLIKELY (plugin->edit_chapter != NULL)) {
		gtk_window_present (GTK_WINDOW (plugin->edit_chapter));
		return;
	}

	main_window = totem_get_main_window (plugin->totem);
	plugin->priv->was_played = totem_is_playing (plugin->totem);
	totem_action_pause (plugin->totem);

	/* adding a new one, check if it's time available */
	g_object_get (G_OBJECT (plugin->totem), "current-time", &time, NULL);
	if (G_UNLIKELY (!check_available_time (plugin, time))) {
		totem_interface_error_blocking (_("Chapter with the same time already exists"),
						_("Try another name or remove an existing chapter"),
						main_window);
		g_object_unref (main_window);
		if (plugin->priv->was_played)
			totem_action_play (plugin->totem);
		return;
	}
	plugin->priv->last_time = time;

	/* capture frame */
	bvw = BACON_VIDEO_WIDGET (totem_get_video_widget (plugin->totem));
	plugin->priv->last_frame = bacon_video_widget_get_current_frame (bvw);
	g_object_add_weak_pointer (G_OBJECT (plugin->priv->last_frame), (gpointer *) &plugin->priv->last_frame);
	g_object_unref (bvw);

	/* create chapter-edit dialog */
	plugin->edit_chapter = TOTEM_EDIT_CHAPTER (totem_edit_chapter_new ());
	g_object_add_weak_pointer (G_OBJECT (plugin->edit_chapter), (gpointer *) &(plugin->edit_chapter));

	g_signal_connect (G_OBJECT (plugin->edit_chapter), "delete-event",
			  G_CALLBACK (gtk_widget_destroy), NULL);
	g_signal_connect (G_OBJECT (plugin->edit_chapter), "response",
			  G_CALLBACK (chapter_edit_dialog_response_cb), plugin);

	gtk_window_set_transient_for (GTK_WINDOW (plugin->edit_chapter),
				      main_window);
	gtk_widget_show (GTK_WIDGET (plugin->edit_chapter));

	g_object_unref (main_window);
}

static void
chapter_selection_changed_cb (GtkTreeSelection		*tree_selection,
			      TotemChaptersPlugin	*plugin)
{
	gint		count;
	gboolean	allow_remove, allow_goto;

	g_return_if_fail (GTK_IS_TREE_SELECTION (tree_selection));
	g_return_if_fail (TOTEM_IS_CHAPTERS_PLUGIN (plugin));

	count = gtk_tree_selection_count_selected_rows (tree_selection);
	allow_remove = (count > 0);
	allow_goto = (count == 1);

	gtk_widget_set_sensitive (plugin->priv->remove_button, allow_remove);
	gtk_widget_set_sensitive (plugin->priv->goto_button, allow_goto);
}

static void
mateconf_autoload_changed_cb (MateConfClient		*mateconf,
			   guint		cnx_id,
			   MateConfEntry		*entry,
			   TotemChaptersPlugin	*plugin)
{
	g_return_if_fail (MATECONF_IS_CLIENT (mateconf));
	g_return_if_fail (entry != NULL);
	g_return_if_fail (TOTEM_IS_CHAPTERS_PLUGIN (plugin));

	if (G_LIKELY (entry->value != NULL))
		plugin->priv->autoload = mateconf_value_get_bool (entry->value);
	else
		plugin->priv->autoload = TRUE;
}

static void
load_chapters_from_file (const gchar		*uri,
			 gboolean		from_dialog,
			 TotemChaptersPlugin	*plugin)
{
	TotemCmmlAsyncData	*data;

	g_return_if_fail (TOTEM_IS_CHAPTERS_PLUGIN (plugin));

	if (plugin->priv->cancellable[0] != NULL) {
		g_cancellable_cancel (plugin->priv->cancellable[0]);
		g_object_unref (plugin->priv->cancellable[0]);
	}

	data = g_new0 (TotemCmmlAsyncData, 1);
	/* do not forget to save this pointer in the result function */
	data->file = g_strdup (uri);
	data->final = totem_file_opened_result_cb;
	data->user_data = (gpointer) plugin;
	if (from_dialog)
		data->from_dialog = TRUE;
	/* cancellable object shouldn't be finalized during result func */
	plugin->priv->cancellable[0] = g_cancellable_new ();
	g_object_add_weak_pointer (G_OBJECT (plugin->priv->cancellable[0]),
				   (gpointer *) &(plugin->priv->cancellable[0]));
	data->cancellable = plugin->priv->cancellable[0];

	if (G_UNLIKELY (totem_cmml_read_file_async (data) < 0)) {
		g_warning ("chapters: wrong parameters for reading CMML file, may be a bug");

		set_no_data_visible (TRUE, TRUE, plugin);

		g_object_unref (plugin->priv->cancellable[0]);
		g_free (data);
	}
}

static void
set_no_data_visible (gboolean			visible,
		     gboolean			show_buttons,
		     TotemChaptersPlugin	*plugin)
{
	g_return_if_fail (TOTEM_IS_CHAPTERS_PLUGIN (plugin));

	if (visible) {
		gtk_widget_hide (plugin->priv->list_box);
		gtk_widget_show (plugin->priv->load_box);
	} else {
		gtk_widget_hide (plugin->priv->load_box);
		gtk_widget_show (plugin->priv->list_box);
	}

	gtk_widget_set_sensitive (plugin->priv->add_button, !visible);
	gtk_widget_set_sensitive (plugin->priv->tree, !visible);

	gtk_widget_set_visible (plugin->priv->load_button, show_buttons);
	gtk_widget_set_visible (plugin->priv->continue_button, show_buttons);
}

void
add_button_clicked_cb (GtkButton		*button,
		       TotemChaptersPlugin	*plugin)
{
	g_return_if_fail (TOTEM_IS_CHAPTERS_PLUGIN (plugin));

	show_chapter_edit_dialog (plugin);
}

void
remove_button_clicked_cb (GtkButton		*button,
			  TotemChaptersPlugin	*plugin)
{
	GtkTreeSelection	*selection;
	GtkTreeIter		iter;
	GtkTreeModel		*store;
	GList			*list;

	g_return_if_fail (TOTEM_IS_CHAPTERS_PLUGIN (plugin));

	store = gtk_tree_view_get_model (GTK_TREE_VIEW (plugin->priv->tree));
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (plugin->priv->tree));
	list = gtk_tree_selection_get_selected_rows (selection, NULL);

	g_return_if_fail (g_list_length (list) != 0);

	list = g_list_last (list);
	while (list != NULL) {
		gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, (GtkTreePath *) list->data);
		gtk_tree_store_remove (GTK_TREE_STORE (store), &iter);

		list = list->prev;
	}

	gtk_widget_set_sensitive (plugin->priv->save_button, TRUE);

	g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (list);
}

static void
save_chapters_result_cb (gpointer	data,
			 gpointer	user_data)
{
	TotemCmmlAsyncData	*adata;
	TotemChaptersPlugin	*plugin;

	g_return_if_fail (data != NULL);

	adata = (TotemCmmlAsyncData *) data;
	plugin = TOTEM_CHAPTERS_PLUGIN (adata->user_data);

	if (G_UNLIKELY (!adata->successful && !g_cancellable_is_cancelled (adata->cancellable))) {
		totem_action_error (_("Error while writing file with chapters"),
				    adata->error, plugin->totem);
		gtk_widget_set_sensitive (plugin->priv->save_button, TRUE);
	}

	g_object_unref (adata->cancellable);
	g_list_foreach (adata->list, (GFunc) totem_cmml_clip_free, NULL);
	g_list_free (adata->list);
	g_free (adata->error);
	g_free (adata);
}

static GList *
get_chapters_list (TotemChaptersPlugin	*plugin)
{
	GList		*list = NULL;
	TotemCmmlClip	*clip;
	GtkTreeModel	*store;
	GtkTreeIter	iter;
	gchar		*title;
	gint64		time;
	GdkPixbuf	*pixbuf;
	gboolean	valid;

	g_return_val_if_fail (TOTEM_IS_CHAPTERS_PLUGIN (plugin), NULL);

	store = gtk_tree_view_get_model (GTK_TREE_VIEW (plugin->priv->tree));

	valid = gtk_tree_model_get_iter_first (store, &iter);
	while (valid) {
		gtk_tree_model_get (store, &iter,
				    CHAPTERS_TITLE_PRIV_COLUMN, &title,
				    CHAPTERS_TIME_PRIV_COLUMN, &time,
				    CHAPTERS_PIXBUF_COLUMN, &pixbuf,
				    -1);
		clip = totem_cmml_clip_new (title, NULL, time, pixbuf);
		list = g_list_prepend (list, clip);

		g_free (title);
		g_object_unref (pixbuf);

		valid = gtk_tree_model_iter_next (store, &iter);
	}
	list = g_list_reverse (list);

	return list;
}

static gboolean
show_popup_menu (TotemChaptersPlugin	*plugin,
		 GdkEventButton		*event)
{
	guint			button = 0;
	guint32			_time;
	GtkTreePath		*path;
	gint			count;
	GtkWidget		*menu;
	GtkAction		*remove_act, *goto_act;
	GtkTreeSelection	*selection;

	g_return_val_if_fail (TOTEM_IS_CHAPTERS_PLUGIN (plugin), FALSE);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (plugin->priv->tree));

	if (event != NULL) {
		button = event->button;
		_time = event->time;

		if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (plugin->priv->tree),
						   event->x, event->y, &path, NULL, NULL, NULL)) {
			if (!gtk_tree_selection_path_is_selected (selection, path)) {
				gtk_tree_selection_unselect_all (selection);
				gtk_tree_selection_select_path (selection, path);
			}
			gtk_tree_path_free (path);
		} else
			gtk_tree_selection_unselect_all (selection);
	} else
		_time = gtk_get_current_event_time ();

	count = gtk_tree_selection_count_selected_rows (selection);

	if (count == 0)
		return FALSE;

	remove_act = gtk_action_group_get_action (plugin->priv->action_group, "remove");
	goto_act = gtk_action_group_get_action (plugin->priv->action_group, "goto");
	gtk_action_set_sensitive (remove_act, count > 0);
	gtk_action_set_sensitive (goto_act, count == 1);

	menu = gtk_ui_manager_get_widget (plugin->priv->ui_manager, "/totem-chapters-popup");

	gtk_menu_shell_select_first (GTK_MENU_SHELL (menu), FALSE);

	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
			button, _time);

	return TRUE;
}

void
save_button_clicked_cb (GtkButton		*button,
			TotemChaptersPlugin	*plugin)
{
	TotemCmmlAsyncData	*data;

	g_return_if_fail (TOTEM_IS_CHAPTERS_PLUGIN (plugin));

	if (plugin->priv->cancellable[1] != NULL) {
		g_cancellable_cancel (plugin->priv->cancellable[1]);
		g_object_unref (plugin->priv->cancellable[1]);
	}

	data = g_new0 (TotemCmmlAsyncData, 1);
	data->file = plugin->priv->cmml_mrl;
	data->list = get_chapters_list (plugin);
	data->final = save_chapters_result_cb;
	data->user_data = (gpointer) plugin;
	/* cancellable object shouldn't be finalized during result func */
	data->cancellable = g_cancellable_new ();
	plugin->priv->cancellable[1] = data->cancellable;
	g_object_add_weak_pointer (G_OBJECT (plugin->priv->cancellable[1]),
				   (gpointer *) &(plugin->priv->cancellable[1]));

	if (G_UNLIKELY (totem_cmml_write_file_async (data) < 0)) {
		totem_action_error (_("Error occurred while saving chapters"),
				    _("Please check you rights and free space"), plugin->totem);
		g_free (data);
		g_object_unref (plugin->priv->cancellable);
	} else
		gtk_widget_set_sensitive (plugin->priv->save_button, FALSE);
}

void
tree_view_row_activated_cb (GtkTreeView			*tree_view,
			    GtkTreePath			*path,
			    GtkTreeViewColumn		*column,
			    TotemChaptersPlugin		*plugin)
{
	GtkTreeModel	*store;
	GtkTreeIter	iter;
	gboolean	seekable;
	gint64		time;

	g_return_if_fail (TOTEM_IS_CHAPTERS_PLUGIN (plugin));
	g_return_if_fail (GTK_IS_TREE_VIEW (tree_view));
	g_return_if_fail (path != NULL);

	store = gtk_tree_view_get_model (tree_view);
	seekable = totem_is_seekable (plugin->totem);
	if (!seekable) {
		g_warning ("chapters: unable to seek stream!");
		return;
	}

	gtk_tree_model_get_iter (store, &iter, path);
	gtk_tree_model_get (store, &iter, CHAPTERS_TIME_PRIV_COLUMN, &time, -1);

	totem_action_seek_time (plugin->totem, time, TRUE);
}

gboolean
tree_view_button_press_cb (GtkTreeView			*tree_view,
			   GdkEventButton		*event,
			   TotemChaptersPlugin		*plugin)
{
	g_return_val_if_fail (TOTEM_IS_CHAPTERS_PLUGIN (plugin), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	if (event->type == GDK_BUTTON_PRESS && event->button == 3)
		return show_popup_menu (plugin, event);

	return FALSE;
}

gboolean
tree_view_key_press_cb (GtkTreeView		*tree_view,
			GdkEventKey		*event,
			TotemChaptersPlugin	*plugin)
{
	GtkTreeSelection	*selection;

	g_return_val_if_fail (TOTEM_IS_CHAPTERS_PLUGIN (plugin), FALSE);
	g_return_val_if_fail (event != NULL, FALSE);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (plugin->priv->tree));

	/* Special case some shortcuts */
	if (event->state != 0) {
		if ((event->state & GDK_CONTROL_MASK) &&
		    event->keyval == GDK_a) {
			gtk_tree_selection_select_all (selection);
			return TRUE;
		}
	}

	/* If we have modifiers, and either Ctrl, Mod1 (Alt), or any
	 * of Mod3 to Mod5 (Mod2 is num-lock...) are pressed, we
	 * let Gtk+ handle the key */
	if (event->state != 0 &&
	    ((event->state & GDK_CONTROL_MASK)
	    || (event->state & GDK_MOD1_MASK)
	    || (event->state & GDK_MOD3_MASK)
	    || (event->state & GDK_MOD4_MASK)
	    || (event->state & GDK_MOD5_MASK)))
		return FALSE;

	if (event->keyval == GDK_Delete) {
		if (gtk_tree_selection_count_selected_rows (selection) > 0)
			remove_button_clicked_cb (GTK_BUTTON (plugin->priv->remove_button), plugin);
		return TRUE;
	}

	return FALSE;
}

gboolean
tree_view_popup_menu_cb (GtkTreeView		*tree_view,
			 TotemChaptersPlugin	*plugin)
{
	g_return_val_if_fail (TOTEM_IS_CHAPTERS_PLUGIN (plugin), FALSE);

	return show_popup_menu (plugin, NULL);
}

void
popup_remove_action_cb (GtkAction		*action,
			TotemChaptersPlugin	*plugin)
{
	g_return_if_fail (TOTEM_IS_CHAPTERS_PLUGIN (plugin));

	remove_button_clicked_cb (GTK_BUTTON (plugin->priv->remove_button), plugin);
}

void
popup_goto_action_cb (GtkAction			*action,
		      TotemChaptersPlugin	*plugin)
{
	g_return_if_fail (TOTEM_IS_CHAPTERS_PLUGIN (plugin));

	goto_button_clicked_cb (GTK_BUTTON (plugin->priv->goto_button), plugin);
}

void
load_button_clicked_cb (GtkButton		*button,
			TotemChaptersPlugin	*plugin)
{
	GtkWindow	*main_window;
	GtkWidget	*dialog;
	GFile		*cur, *parent;
	GtkFileFilter	*filter_supported, *filter_all;
	gchar		*filename, *mrl, *dir;

	g_return_if_fail (TOTEM_IS_CHAPTERS_PLUGIN (plugin));

	plugin->priv->was_played = totem_is_playing (plugin->totem);
	totem_action_pause (plugin->totem);

	mrl = totem_get_current_mrl (plugin->totem);
	main_window = totem_get_main_window (plugin->totem);
	dialog = gtk_file_chooser_dialog_new (_("Open Chapters File"), main_window, GTK_FILE_CHOOSER_ACTION_OPEN,
					      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
					      NULL);
	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (dialog), FALSE);

	cur = g_file_new_for_uri (mrl);
	parent = g_file_get_parent (cur);

	if (parent != NULL)
		dir = g_file_get_uri (parent);
	else
		dir = g_strdup (G_DIR_SEPARATOR_S);

	filter_supported = gtk_file_filter_new ();
	filter_all = gtk_file_filter_new ();

	gtk_file_filter_add_pattern (filter_supported, "*.cmml");
	gtk_file_filter_add_pattern (filter_supported, "*.CMML");
	gtk_file_filter_set_name (filter_supported, _("Supported files"));

	gtk_file_filter_add_pattern (filter_all, "*");
	gtk_file_filter_set_name (filter_all, _("All files"));

	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter_supported);
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER (dialog), filter_all);

	gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (dialog), dir);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		filename = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (dialog));

		load_chapters_from_file (filename, TRUE, plugin);

		g_free (filename);
	}

	if (plugin->priv->was_played)
		totem_action_play (plugin->totem);

	gtk_widget_destroy (dialog);
	g_object_unref (main_window);
	g_object_unref (cur);
	g_object_unref (parent);
	g_free (mrl);
	g_free (dir);
}

void
continue_button_clicked_cb (GtkButton		*button,
			    TotemChaptersPlugin	*plugin)
{
	g_return_if_fail (TOTEM_IS_CHAPTERS_PLUGIN (plugin));

	set_no_data_visible (FALSE, FALSE, plugin);
}

void
goto_button_clicked_cb (GtkButton		*button,
			TotemChaptersPlugin	*plugin)
{
	GtkTreeView		*tree;
	GtkTreeModel		*store;
	GtkTreeSelection	*selection;
	GList			*list;

	g_return_if_fail (TOTEM_IS_CHAPTERS_PLUGIN (plugin));

	tree = GTK_TREE_VIEW (plugin->priv->tree);
	store = gtk_tree_view_get_model (tree);
	selection = gtk_tree_view_get_selection (tree);

	list = gtk_tree_selection_get_selected_rows (selection, &store);

	tree_view_row_activated_cb (tree, (GtkTreePath *) list->data, NULL, plugin);

	g_list_foreach (list, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (list);
}

static gboolean
impl_activate (TotemPlugin *plugin,
	       TotemObject *totem,
	       GError **error)
{
	GtkWindow		*main_window;
	MateConfClient		*mateconf;
	GtkBuilder		*builder;
	GtkWidget		*main_box;
	GtkTreeSelection	*selection;
	TotemChaptersPlugin	*cplugin;
	GtkCellRenderer		*renderer;
	GtkTreeViewColumn	*column;
	gchar			*mrl;

	g_return_val_if_fail (TOTEM_IS_CHAPTERS_PLUGIN (plugin), FALSE);

	cplugin = TOTEM_CHAPTERS_PLUGIN (plugin);
	main_window = totem_get_main_window (totem);

	builder = totem_plugin_load_interface (plugin, "chapters-list.ui", TRUE,
					       main_window, cplugin);
	g_object_unref (main_window);

	if (builder == NULL) {
		//FIXME set error
		return FALSE;
	}

	mateconf = mateconf_client_get_default ();
	if (G_LIKELY (mateconf != NULL)) {
		cplugin->priv->autoload = mateconf_client_get_bool (mateconf, MATECONF_PREFIX"/autoload_chapters", NULL);
		cplugin->priv->autoload_handle_id = mateconf_client_notify_add (mateconf, MATECONF_PREFIX"/autoload_chapters",
									     (MateConfClientNotifyFunc) mateconf_autoload_changed_cb,
									      cplugin, NULL, NULL);
	} else
		cplugin->priv->autoload = TRUE;
	cplugin->priv->mateconf = mateconf;

	cplugin->priv->tree = GTK_WIDGET (gtk_builder_get_object (builder, "chapters_tree_view"));
	cplugin->priv->action_group = GTK_ACTION_GROUP (gtk_builder_get_object (builder, "chapters-action-group"));
	g_object_ref (cplugin->priv->action_group);
	cplugin->priv->ui_manager = GTK_UI_MANAGER (gtk_builder_get_object (builder, "totem-chapters-ui-manager"));
	g_object_ref (cplugin->priv->ui_manager);

	renderer = gtk_cell_renderer_pixbuf_new ();
	column = gtk_tree_view_column_new_with_attributes ("Pixbuf", renderer, "pixbuf", CHAPTERS_PIXBUF_COLUMN, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (cplugin->priv->tree), column);

	renderer = gtk_cell_renderer_text_new ();

	g_object_set (renderer, "editable", TRUE, NULL);
	g_signal_connect (G_OBJECT (renderer), "editing-started",
			  G_CALLBACK (prepare_chapter_edit), cplugin);
	g_signal_connect (G_OBJECT (renderer), "edited",
			  G_CALLBACK (finish_chapter_edit), cplugin);

	column = gtk_tree_view_column_new_with_attributes ("Title", renderer,
							   "markup", CHAPTERS_TITLE_COLUMN, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (cplugin->priv->tree), column);

	cplugin->totem = g_object_ref (totem);
	/* for read operation */
	cplugin->priv->cancellable[0] = NULL;
	/* for write operation */
	cplugin->priv->cancellable[1] = NULL;
	cplugin->edit_chapter = NULL;
	cplugin->priv->last_frame = NULL;
	cplugin->priv->cmml_mrl = NULL;
	cplugin->priv->last_time = 0;

	cplugin->priv->add_button = GTK_WIDGET (gtk_builder_get_object (builder, "add_button"));
	cplugin->priv->remove_button = GTK_WIDGET (gtk_builder_get_object (builder, "remove_button"));
	cplugin->priv->save_button = GTK_WIDGET (gtk_builder_get_object (builder, "save_button"));
	cplugin->priv->goto_button = GTK_WIDGET (gtk_builder_get_object (builder, "goto_button"));
	cplugin->priv->load_button = GTK_WIDGET (gtk_builder_get_object (builder, "load_button"));
	cplugin->priv->continue_button = GTK_WIDGET (gtk_builder_get_object (builder, "continue_button"));

	gtk_widget_hide (cplugin->priv->load_button);
	gtk_widget_hide (cplugin->priv->continue_button);

	cplugin->priv->list_box = GTK_WIDGET (gtk_builder_get_object (builder, "main_vbox"));
	cplugin->priv->load_box = GTK_WIDGET (gtk_builder_get_object (builder, "load_vbox"));

	main_box = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (main_box), cplugin->priv->list_box, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (main_box), cplugin->priv->load_box, TRUE, TRUE, 0);

	set_no_data_visible (TRUE, FALSE, cplugin);

	totem_add_sidebar_page (totem, "chapters", _("Chapters"), main_box);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (cplugin->priv->tree));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

	g_signal_connect (G_OBJECT (totem),
			  "file-opened",
			  G_CALLBACK (totem_file_opened_async_cb),
			  plugin);
	g_signal_connect (G_OBJECT (totem),
			  "file-closed",
			  G_CALLBACK (totem_file_closed_cb),
			  plugin);
	g_signal_connect (G_OBJECT (selection),
			  "changed",
			  G_CALLBACK (chapter_selection_changed_cb),
			  plugin);

	mrl = totem_get_current_mrl (cplugin->totem);
	if (mrl != NULL)
		totem_file_opened_async_cb (cplugin->totem, mrl, cplugin);

	g_object_unref (builder);
	g_free (mrl);

	return TRUE;
}

static void
impl_deactivate (TotemPlugin *plugin,
		 TotemObject *totem)
{
	TotemChaptersPlugin	*cplugin;

	g_return_if_fail (TOTEM_IS_CHAPTERS_PLUGIN (plugin));

	cplugin = TOTEM_CHAPTERS_PLUGIN (plugin);

	/* FIXME: do not cancel async operation if any */

	g_signal_handlers_disconnect_by_func (G_OBJECT (totem),
					      totem_file_opened_async_cb,
					      plugin);
	g_signal_handlers_disconnect_by_func (G_OBJECT (totem),
					      totem_file_closed_cb,
					      plugin);
	if (cplugin->priv->mateconf != NULL) {
		mateconf_client_notify_remove (cplugin->priv->mateconf, cplugin->priv->autoload_handle_id);
		g_object_unref (cplugin->priv->mateconf);
	}

	if (G_UNLIKELY (cplugin->priv->last_frame != NULL))
		g_object_unref (G_OBJECT (cplugin->priv->last_frame));

	if (G_UNLIKELY (cplugin->edit_chapter != NULL))
		gtk_widget_destroy (GTK_WIDGET (cplugin->edit_chapter));

	if (G_LIKELY (cplugin->priv->action_group != NULL))
		g_object_unref (cplugin->priv->action_group);

	if (G_LIKELY (cplugin->priv->ui_manager != NULL))
		g_object_unref (cplugin->priv->ui_manager);

	if (G_LIKELY (cplugin->priv->cancellable[0] != NULL))
		g_cancellable_cancel (cplugin->priv->cancellable[0]);

	if (G_LIKELY (cplugin->priv->cancellable[1] != NULL))
		g_cancellable_cancel (cplugin->priv->cancellable[1]);


	g_object_unref (cplugin->totem);
	g_free (cplugin->priv->cmml_mrl);

	totem_remove_sidebar_page (totem, "chapters");
}
