/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2001-2007 Philip Withnall <philip@tecnocode.co.uk>
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
 * SECTION:totem-video-list
 * @short_description: a #GtkTreeView widget for listing videos
 * @stability: Unstable
 * @include: totem-video-list.h
 * @see_also: #TotemCellRendererVideo
 *
 * #TotemVideoList is a #GtkTreeView designed for listing videos by their thumbnails using #TotemCellRendererVideo.
 * It supports tooltips, loading the videos by activating tree view rows, and #GtkUIManager actions in the popup menu.
 **/

#include "config.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>

#include "totem.h"
#include "totem-video-list.h"
#include "totem-private.h"
#include "totemvideolist-marshal.h"
#include "totem-interface.h"

struct _TotemVideoListPrivate {
	gint tooltip_column;
	gint mrl_column;
	Totem *totem;
	GtkBuilder *xml;
	GtkActionGroup *action_group;
	GtkUIManager *ui_manager;
};

#define TOTEM_VIDEO_LIST_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TOTEM_TYPE_VIDEO_LIST, TotemVideoListPrivate))

enum {
	PROP_TOOLTIP_COLUMN = 1,
	PROP_MRL_COLUMN,
	PROP_TOTEM
};

enum {
	STARTING_VIDEO,
	LAST_SIGNAL
};

static void totem_video_list_dispose (GObject *object);
static void totem_video_list_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void totem_video_list_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static gboolean query_tooltip_cb (GtkWidget *widget, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, gpointer user_data);
static void selection_changed_cb (GtkTreeSelection *selection, GtkWidget *tree_view);
static void row_activated_cb (GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data);
static gboolean default_starting_video_cb (TotemVideoList *video_list, GtkTreePath *path);
static gboolean show_popup_menu (TotemVideoList *self, GdkEventButton *event);
static gboolean button_pressed_cb (GtkTreeView *tree_view, GdkEventButton *event, TotemVideoList *video_list);
static gboolean popup_menu_cb (GtkTreeView *tree_view, TotemVideoList *video_list);

/* Callback functions for GtkBuilder */
G_MODULE_EXPORT void add_to_playlist_action_callback (GtkAction *action, TotemVideoList *self);
G_MODULE_EXPORT void copy_location_action_callback (GtkAction *action, TotemVideoList *self);

static gint totem_video_list_table_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (TotemVideoList, totem_video_list, GTK_TYPE_TREE_VIEW)

/**
 * totem_video_list_new:
 *
 * Creates a new #TotemVideoList with default properties.
 *
 * Return value: a new #TotemVideoList
 **/
TotemVideoList *
totem_video_list_new (void)
{
	TotemVideoList *video_list;

	video_list = TOTEM_VIDEO_LIST (g_object_new (TOTEM_TYPE_VIDEO_LIST, NULL));
	if (video_list->priv->xml == NULL || video_list->priv->ui_manager == NULL) {
		g_object_unref (video_list);
		return NULL;
	}

	return video_list;
}

static void
totem_video_list_class_init (TotemVideoListClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (TotemVideoListPrivate));

	object_class->set_property = totem_video_list_set_property;
	object_class->get_property = totem_video_list_get_property;
	object_class->dispose = totem_video_list_dispose;
	klass->starting_video = default_starting_video_cb;

	/**
	 * TotemVideoList:tooltip-column:
	 *
	 * The column number of the #GtkTreeModel column containing tooltips to be displayed by the video list.
	 * If it's set to -1, no tooltips shall be displayed.
	 **/
	g_object_class_install_property (object_class, PROP_TOOLTIP_COLUMN,
				g_param_spec_int ("tooltip-column", NULL, NULL,
					-1, G_MAXINT, -1, G_PARAM_READWRITE));

	/**
	 * TotemVideoList:mrl-column:
	 *
	 * The column number of the #GtkTreeModel column containing MRLs of the videos in the video list.
	 * If it's set to -1, video rows will not be activatable (e.g. by double-clicking them).
	 **/
	g_object_class_install_property (object_class, PROP_MRL_COLUMN,
				g_param_spec_int ("mrl-column", NULL, NULL,
					-1, G_MAXINT, -1, G_PARAM_READWRITE));

	/**
	 * TotemVideoList:totem:
	 *
	 * A #TotemObject for integration purposes.
	 **/
	/* FIXME: Is there no better way to do this? */
	g_object_class_install_property (object_class, PROP_TOTEM,
				g_param_spec_object ("totem", NULL, NULL,
					TOTEM_TYPE_OBJECT, G_PARAM_READWRITE));

	/**
	 * TotemVideoList::starting-video:
	 * @video_list: the #TotemVideoList which received the signal
	 * @tree_path: the #GtkTreePath of the video row about to be played
	 *
	 * The #TotemVideoList::starting-video signal is emitted when a video row is activated, just before the video is
	 * added to the playlist. It allows for the video's MRL in the #GtkTreeModel to be modified, for example.
	 *
	 * If this returns %TRUE, the video will be played; otherwise, it will not.
	 */
	totem_video_list_table_signals[STARTING_VIDEO] = g_signal_new ("starting-video",
				G_TYPE_FROM_CLASS (object_class),
				G_SIGNAL_RUN_LAST,
				G_STRUCT_OFFSET (TotemVideoListClass, starting_video),
				NULL, NULL,
				totemvideolist_marshal_BOOLEAN__BOXED,
				G_TYPE_BOOLEAN, 1,
				GTK_TYPE_TREE_PATH);
}

static void
totem_video_list_init (TotemVideoList *self)
{
	GtkTreeSelection *selection;

	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, TOTEM_TYPE_VIDEO_LIST, TotemVideoListPrivate);
	self->priv->totem = NULL;
	self->priv->tooltip_column = -1;
	self->priv->mrl_column = -1;

	/* Get the interface */
	self->priv->xml = totem_interface_load ("video-list.ui", TRUE, NULL, self);

	if (self->priv->xml == NULL)
		return;

	self->priv->action_group = GTK_ACTION_GROUP (gtk_builder_get_object (self->priv->xml, "video-list-action-group"));
	self->priv->ui_manager = GTK_UI_MANAGER (gtk_builder_get_object (self->priv->xml, "totem-video-list-ui-manager"));

	/* Set up tooltips and the context menu */
	g_object_set (self, "has-tooltip", TRUE, NULL);
	g_signal_connect (self, "row-activated", G_CALLBACK (row_activated_cb), NULL);
	g_signal_connect (self, "query-tooltip", G_CALLBACK (query_tooltip_cb), NULL);
	g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (self)),
			  "changed", G_CALLBACK (selection_changed_cb), GTK_TREE_VIEW (self));
	g_signal_connect (self, "button-press-event", G_CALLBACK (button_pressed_cb), self);
	g_signal_connect (self, "popup-menu", G_CALLBACK (popup_menu_cb), self);

	/* Allow selection of multiple rows */
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (self));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
}

static void
totem_video_list_dispose (GObject *object)
{
	TotemVideoListPrivate *priv = TOTEM_VIDEO_LIST_GET_PRIVATE (object);

	if (priv->totem != NULL) {
		g_object_unref (priv->totem);
		priv->totem = NULL;
		g_object_unref (priv->xml);
		/* priv->xml automatically unrefs and destroys its widgets,
		 * since we haven't reffed them ourselves, and neither has
		 * gtk_builder_get_object. */
	}

	G_OBJECT_CLASS (totem_video_list_parent_class)->dispose (object);
}

static void
totem_video_list_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	TotemVideoListPrivate *priv = TOTEM_VIDEO_LIST_GET_PRIVATE (object);

	switch (property_id)
	{
		case PROP_TOOLTIP_COLUMN:
			priv->tooltip_column = g_value_get_int (value);
			g_object_notify (object, "tooltip-column");
			break;
		case PROP_MRL_COLUMN:
			priv->mrl_column = g_value_get_int (value);
			g_object_notify (object, "mrl-column");
			break;
		case PROP_TOTEM:
			if (priv->totem != NULL)
				g_object_unref (priv->totem);
			priv->totem = (Totem*) g_value_dup_object (value);
			g_object_notify (object, "totem");
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
totem_video_list_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	TotemVideoListPrivate *priv = TOTEM_VIDEO_LIST_GET_PRIVATE (object);

	switch (property_id)
	{
		case PROP_TOOLTIP_COLUMN:
			g_value_set_int (value, priv->tooltip_column);
			break;
		case PROP_MRL_COLUMN:
			g_value_set_int (value, priv->mrl_column);
			break;
		case PROP_TOTEM:
			g_value_set_object (value, G_OBJECT (priv->totem));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static gboolean
query_tooltip_cb (GtkWidget *widget, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, gpointer user_data)
{
	TotemVideoList *self = TOTEM_VIDEO_LIST (widget);
	GtkTreeIter iter;
	gchar *tooltip_text;
	gchar *mrl_text;
	gchar *final_text;
	GtkTreeView *tree_view = GTK_TREE_VIEW (widget);
	GtkTreeModel *model = gtk_tree_view_get_model (tree_view);
	GtkTreePath *path = NULL;

	if (self->priv->tooltip_column == -1)
		return FALSE;

	if (!gtk_tree_view_get_tooltip_context (tree_view, &x, &y,
				keyboard_mode,
				&model, &path, &iter))
		return FALSE;

	if (self->priv->mrl_column == -1) {
		gtk_tree_model_get (model, &iter, self->priv->tooltip_column, &tooltip_text, -1);
		gtk_tooltip_set_text (tooltip, tooltip_text);
		g_free (tooltip_text);
	} else {
		gtk_tree_model_get (model, &iter,
				self->priv->tooltip_column, &tooltip_text,
				self->priv->mrl_column, &mrl_text,
				-1);

		/* Display "No video URI" if the MRL is unset */
		if (mrl_text == NULL)
			mrl_text = g_strdup (_("No video URI"));

		final_text = g_strdup_printf ("%s\n%s", tooltip_text, mrl_text);
		gtk_tooltip_set_text (tooltip, final_text);

		g_free (tooltip_text);
		g_free (mrl_text);
		g_free (final_text);
	}

	gtk_tree_view_set_tooltip_row (tree_view, tooltip, path);
	gtk_tree_path_free (path);

	return TRUE;
}

static void
selection_changed_cb (GtkTreeSelection *selection, GtkWidget *tree_view)
{
	gtk_widget_trigger_tooltip_query (tree_view);
}

static void
row_activated_cb (GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
	GtkTreeIter iter;
	gchar *mrl, *display_name = NULL;
	gboolean play_video = TRUE;
	TotemVideoList *self = TOTEM_VIDEO_LIST (tree_view);
	GtkTreeModel *model = gtk_tree_view_get_model (tree_view);

	if (self->priv->mrl_column == -1)
		return;

	/* Get the MRL and display name before emitting the signal, since the signal handler
	 * could clear the tree model */
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter,
				self->priv->mrl_column, &mrl,
				self->priv->tooltip_column, &display_name,
				-1);

	/* Emit the starting-video signal, to allow video playback to be cancelled */
	g_signal_emit (tree_view, totem_video_list_table_signals[STARTING_VIDEO], 0,
		       path,
		       &play_video);

	if (play_video == FALSE)
		goto finish;

	if (mrl != NULL)
		totem_add_to_playlist_and_play (self->priv->totem, mrl, display_name, FALSE);

finish:
	g_free (mrl);
	g_free (display_name);
}

static gboolean
default_starting_video_cb (TotemVideoList *video_list, GtkTreePath *path)
{
	/* Play the video by default */
	return TRUE;
}

static gboolean
show_popup_menu (TotemVideoList *self, GdkEventButton *event)
{
	guint button = 0;
	guint32 _time;
	GtkTreePath *path;
	gint count;
	GtkWidget *menu;
	GtkAction *action; 
	gboolean have_mrl = FALSE;
	GList *rows, *row;
	GtkTreeModel *model;
	GtkTreeView *tree_view = GTK_TREE_VIEW (self);
	GtkTreeSelection *selection = gtk_tree_view_get_selection (tree_view);

	if (event != NULL) {
		button = event->button;
		_time = event->time;

		if (gtk_tree_view_get_path_at_pos (tree_view,
				 event->x, event->y, &path, NULL, NULL, NULL)) {
			if (!gtk_tree_selection_path_is_selected (selection, path)) {
				gtk_tree_selection_unselect_all (selection);
				gtk_tree_selection_select_path (selection, path);
			}
			gtk_tree_path_free (path);
		} else {
			gtk_tree_selection_unselect_all (selection);
		}
	} else {
		_time = gtk_get_current_event_time ();
	}

	count = gtk_tree_selection_count_selected_rows (selection);
	
	if (count == 0)
		return FALSE;

	/* Work out if any of the rows have the MRL set */
	rows = gtk_tree_selection_get_selected_rows (selection, &model);
	for (row = rows; row != NULL; row = row->next) {
		GtkTreeIter iter;
		gchar *mrl;

		if (gtk_tree_model_get_iter (model, &iter, (GtkTreePath*) (row->data)) == FALSE)
			continue;

		gtk_tree_model_get (model, &iter, self->priv->mrl_column, &mrl, -1);
		if (mrl != NULL)
			have_mrl = TRUE;
		g_free (mrl);
		gtk_tree_path_free (row->data);
	}
	g_list_free (rows);

	/* Only allow the location to be copied iff one row is selected and we have an MRL */
	action = gtk_action_group_get_action (self->priv->action_group, "copy-location");
	gtk_action_set_sensitive (action, (count == 1 && have_mrl == TRUE) ? TRUE : FALSE);

	/* Likewise, only allow things to be enqueued if we have an MRL */
	action = gtk_action_group_get_action (self->priv->action_group, "add-to-playlist");
	gtk_action_set_sensitive (action, have_mrl);

	menu = gtk_ui_manager_get_widget (self->priv->ui_manager, "/totem-video-list-popup");

	gtk_menu_shell_select_first (GTK_MENU_SHELL (menu), FALSE);

	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
			button, _time);

	return TRUE;
}

static gboolean
button_pressed_cb (GtkTreeView *tree_view, GdkEventButton *event, TotemVideoList *video_list)
{
	if (event->type == GDK_BUTTON_PRESS && event->button == 3)
		return show_popup_menu (video_list, event);

	return FALSE;
}

static gboolean
popup_menu_cb (GtkTreeView *tree_view, TotemVideoList *video_list)
{
	return show_popup_menu (video_list, NULL);
}

void
add_to_playlist_action_callback (GtkAction *action, TotemVideoList *self)
{
	GtkTreePath *path;
	GtkTreeIter iter;
	gchar *mrl, *display_name;
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (self));
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (self));
	GList *l = gtk_tree_selection_get_selected_rows (selection, NULL);
	TotemPlaylist *playlist = self->priv->totem->playlist;

	for (; l != NULL; l = l->next) {
		path = l->data;

		gtk_tree_model_get_iter (model, &iter, path);
		gtk_tree_model_get (model,
				&iter,
				self->priv->mrl_column, &mrl,
				self->priv->tooltip_column, &display_name,
				-1);

		/* We can only continue if we have an MRL */
		if (mrl == NULL) {
			g_free (display_name);
			continue;
		}

		totem_playlist_add_mrl (playlist, mrl, display_name, TRUE, NULL, NULL, NULL);

		g_free (mrl);
		g_free (display_name);
	}

	g_list_foreach (l, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (l);
}

void
copy_location_action_callback (GtkAction *action, TotemVideoList *self)
{
	GList *l;
	GtkTreePath *path;
	GtkClipboard *clip;
	gchar *mrl;
	GtkTreeIter iter;
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (self));
	GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (self));

	l = gtk_tree_selection_get_selected_rows (selection, NULL);
	path = l->data;

	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model,
				&iter,
				self->priv->mrl_column, &mrl,
				-1);

	/* We can only continue if we have an MRL */
	if (mrl == NULL)
		goto error;

	/* Set both the middle-click and the super-paste buffers */
	clip = gtk_clipboard_get_for_display
		(gdk_display_get_default(), GDK_SELECTION_CLIPBOARD);
	gtk_clipboard_set_text (clip, mrl, -1);
	clip = gtk_clipboard_get_for_display
		(gdk_display_get_default(), GDK_SELECTION_PRIMARY);
	gtk_clipboard_set_text (clip, mrl, -1);
	g_free (mrl);

error:
	g_list_foreach (l, (GFunc) gtk_tree_path_free, NULL);
	g_list_free (l);
}

/**
 * totem_video_list_get_ui_manager:
 * @self: a #TotemVideoList
 *
 * Returns the #GtkUIManager in use by @self.
 *
 * Return value: @self's #GtkUIManager
 **/
GtkUIManager *
totem_video_list_get_ui_manager (TotemVideoList *self)
{
	return self->priv->ui_manager;
}

