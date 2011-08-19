/* Eye Of Mate - Thumbnail View
 *
 * Copyright (C) 2006-2008 The Free Software Foundation
 *
 * Author: Claudio Saavedra <csaavedra@mate.org>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "eog-thumb-view.h"
#include "eog-list-store.h"
#include "eog-image.h"
#include "eog-job-queue.h"

#ifdef HAVE_EXIF
#include "eog-exif-util.h"
#include <libexif/exif-data.h>
#endif

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <stdlib.h>
#include <string.h>

#define EOG_THUMB_VIEW_SPACING 0

#define EOG_THUMB_VIEW_GET_PRIVATE(object)				\
	(G_TYPE_INSTANCE_GET_PRIVATE ((object), EOG_TYPE_THUMB_VIEW, EogThumbViewPrivate))

G_DEFINE_TYPE (EogThumbView, eog_thumb_view, GTK_TYPE_ICON_VIEW);

static EogImage* eog_thumb_view_get_image_from_path (EogThumbView      *thumbview,
						     GtkTreePath       *path);

static void      eog_thumb_view_popup_menu          (EogThumbView      *widget,
						     GdkEventButton    *event);

struct _EogThumbViewPrivate {
	gint start_thumb; /* the first visible thumbnail */
	gint end_thumb;   /* the last visible thumbnail  */
	GtkWidget *menu;  /* a contextual menu for thumbnails */
	GtkCellRenderer *pixbuf_cell;
};

/* Drag 'n Drop */

static void
eog_thumb_view_finalize (GObject *object)
{
	EogThumbView *thumbview;
	g_return_if_fail (EOG_IS_THUMB_VIEW (object));
	thumbview = EOG_THUMB_VIEW (object);

	G_OBJECT_CLASS (eog_thumb_view_parent_class)->finalize (object);
}

static void
eog_thumb_view_destroy (GtkObject *object)
{
	EogThumbView *thumbview;
	g_return_if_fail (EOG_IS_THUMB_VIEW (object));
	thumbview = EOG_THUMB_VIEW (object);

	GTK_OBJECT_CLASS (eog_thumb_view_parent_class)->destroy (object);
}

static void
eog_thumb_view_class_init (EogThumbViewClass *class)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (class);
	GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);

	gobject_class->finalize = eog_thumb_view_finalize;
	object_class->destroy = eog_thumb_view_destroy;

	g_type_class_add_private (class, sizeof (EogThumbViewPrivate));
}

static void
eog_thumb_view_clear_range (EogThumbView *thumbview,
			    const gint start_thumb,
			    const gint end_thumb)
{
	GtkTreePath *path;
	GtkTreeIter iter;
	EogListStore *store = EOG_LIST_STORE (gtk_icon_view_get_model (GTK_ICON_VIEW (thumbview)));
	gint thumb = start_thumb;
	gboolean result;

	g_assert (start_thumb <= end_thumb);

	path = gtk_tree_path_new_from_indices (start_thumb, -1);
	for (result = gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, path);
	     result && thumb <= end_thumb;
	     result = gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter), thumb++) {
		eog_list_store_thumbnail_unset (store, &iter);
	}
	gtk_tree_path_free (path);
}

static void
eog_thumb_view_add_range (EogThumbView *thumbview,
			  const gint start_thumb,
			  const gint end_thumb)
{
	GtkTreePath *path;
	GtkTreeIter iter;
	EogListStore *store = EOG_LIST_STORE (gtk_icon_view_get_model (GTK_ICON_VIEW (thumbview)));
	gint thumb = start_thumb;
	gboolean result;

	g_assert (start_thumb <= end_thumb);

	path = gtk_tree_path_new_from_indices (start_thumb, -1);
	for (result = gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, path);
	     result && thumb <= end_thumb;
	     result = gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter), thumb++) {
		eog_list_store_thumbnail_set (store, &iter);
	}
	gtk_tree_path_free (path);
}

static void
eog_thumb_view_update_visible_range (EogThumbView *thumbview,
				     const gint start_thumb,
				     const gint end_thumb)
{
	EogThumbViewPrivate *priv = thumbview->priv;
	int old_start_thumb, old_end_thumb;

	old_start_thumb= priv->start_thumb;
	old_end_thumb = priv->end_thumb;

	if (start_thumb == old_start_thumb &&
	    end_thumb == old_end_thumb) {
		return;
	}

	if (old_start_thumb < start_thumb)
		eog_thumb_view_clear_range (thumbview, old_start_thumb, MIN (start_thumb - 1, old_end_thumb));

	if (old_end_thumb > end_thumb)
		eog_thumb_view_clear_range (thumbview, MAX (end_thumb + 1, old_start_thumb), old_end_thumb);

	eog_thumb_view_add_range (thumbview, start_thumb, end_thumb);

	priv->start_thumb = start_thumb;
	priv->end_thumb = end_thumb;
}

static void
thumbview_on_visible_range_changed_cb (EogThumbView *thumbview,
				       gpointer user_data)
{
	GtkTreePath *path1, *path2;

	if (!gtk_icon_view_get_visible_range (GTK_ICON_VIEW (thumbview), &path1, &path2)) {
		return;
	}

	if (path1 == NULL) {
		path1 = gtk_tree_path_new_first ();
	}
	if (path2 == NULL) {
		gint n_items = gtk_tree_model_iter_n_children (gtk_icon_view_get_model (GTK_ICON_VIEW (thumbview)), NULL);
		path2 = gtk_tree_path_new_from_indices (n_items - 1 , -1);
	}

	eog_thumb_view_update_visible_range (thumbview, gtk_tree_path_get_indices (path1) [0],
					     gtk_tree_path_get_indices (path2) [0]);

	gtk_tree_path_free (path1);
	gtk_tree_path_free (path2);
}

static void
thumbview_on_adjustment_changed_cb (EogThumbView *thumbview,
				    gpointer user_data)
{
	GtkTreePath *path1, *path2;
	gint start_thumb, end_thumb;

	if (!gtk_icon_view_get_visible_range (GTK_ICON_VIEW (thumbview), &path1, &path2)) {
		return;
	}

	if (path1 == NULL) {
		path1 = gtk_tree_path_new_first ();
	}
	if (path2 == NULL) {
		gint n_items = gtk_tree_model_iter_n_children (gtk_icon_view_get_model (GTK_ICON_VIEW (thumbview)), NULL);
		path2 = gtk_tree_path_new_from_indices (n_items - 1 , -1);
	}

	start_thumb = gtk_tree_path_get_indices (path1) [0];
	end_thumb = gtk_tree_path_get_indices (path2) [0];

	eog_thumb_view_add_range (thumbview, start_thumb, end_thumb);

	/* case we added an image, we need to make sure that the shifted thumbnail is cleared */
	eog_thumb_view_clear_range (thumbview, end_thumb + 1, end_thumb + 1);

	thumbview->priv->start_thumb = start_thumb;
	thumbview->priv->end_thumb = end_thumb;

	gtk_tree_path_free (path1);
	gtk_tree_path_free (path2);
}

static void
thumbview_on_parent_set_cb (GtkWidget *widget,
			    GtkObject *old_parent,
			    gpointer   user_data)
{
	EogThumbView *thumbview = EOG_THUMB_VIEW (widget);
	GtkScrolledWindow *sw;
	GtkAdjustment *hadjustment;
	GtkAdjustment *vadjustment;

	GtkWidget *parent = gtk_widget_get_parent (GTK_WIDGET (thumbview));
	if (!GTK_IS_SCROLLED_WINDOW (parent)) {
		return;
	}

	/* if we have been set to a ScrolledWindow, we connect to the callback
	   to set and unset thumbnails. */
	sw = GTK_SCROLLED_WINDOW (parent);
	hadjustment = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (sw));
	vadjustment = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (sw));

	/* when scrolling */
	g_signal_connect_data (G_OBJECT (hadjustment), "value-changed",
			       G_CALLBACK (thumbview_on_visible_range_changed_cb),
			       thumbview, NULL, G_CONNECT_SWAPPED | G_CONNECT_AFTER);
	g_signal_connect_data (G_OBJECT (vadjustment), "value-changed",
			       G_CALLBACK (thumbview_on_visible_range_changed_cb),
			       thumbview, NULL, G_CONNECT_SWAPPED | G_CONNECT_AFTER);

	/* when the adjustment is changed, ie. probably we have new images added. */
	g_signal_connect_data (G_OBJECT (hadjustment), "changed",
			       G_CALLBACK (thumbview_on_adjustment_changed_cb),
			       thumbview, NULL, G_CONNECT_SWAPPED | G_CONNECT_AFTER);
	g_signal_connect_data (G_OBJECT (vadjustment), "changed",
			       G_CALLBACK (thumbview_on_adjustment_changed_cb),
			       thumbview, NULL, G_CONNECT_SWAPPED | G_CONNECT_AFTER);

	/* when resizing the scrolled window */
	g_signal_connect_swapped (G_OBJECT (sw), "size-allocate",
				  G_CALLBACK (thumbview_on_visible_range_changed_cb),
				  thumbview);
}

static gboolean
thumbview_on_button_press_event_cb (GtkWidget *thumbview, GdkEventButton *event,
				    gpointer user_data)
{
	GtkTreePath *path;

	/* Ignore double-clicks and triple-clicks */
	if (event->button == 3 && event->type == GDK_BUTTON_PRESS)
	{
		path = gtk_icon_view_get_path_at_pos (GTK_ICON_VIEW (thumbview),
						      (gint) event->x, (gint) event->y);
		if (path == NULL) {
			return FALSE;
		}

		if (!gtk_icon_view_path_is_selected (GTK_ICON_VIEW (thumbview), path) ||
		    eog_thumb_view_get_n_selected (EOG_THUMB_VIEW (thumbview)) != 1) {
			gtk_icon_view_unselect_all (GTK_ICON_VIEW (thumbview));
			gtk_icon_view_select_path (GTK_ICON_VIEW (thumbview), path);
			gtk_icon_view_set_cursor (GTK_ICON_VIEW (thumbview), path, NULL, FALSE);
		}
		eog_thumb_view_popup_menu (EOG_THUMB_VIEW (thumbview), event);

		gtk_tree_path_free (path);

		return TRUE;
	}

	return FALSE;
}

static void
thumbview_on_drag_data_get_cb (GtkWidget        *widget,
			       GdkDragContext   *drag_context,
			       GtkSelectionData *data,
			       guint             info,
			       guint             time,
			       gpointer          user_data)
{
	GList *list;
	GList *node;
	EogImage *image;
	GFile *file;
	gchar **uris = NULL;
	gint i = 0, n_images;

	list = eog_thumb_view_get_selected_images (EOG_THUMB_VIEW (widget));
	n_images = eog_thumb_view_get_n_selected (EOG_THUMB_VIEW (widget));

	uris = g_new (gchar *, n_images + 1);

	for (node = list; node != NULL; node = node->next, i++) {
		image = EOG_IMAGE (node->data);
		file = eog_image_get_file (image);
		uris[i] = g_file_get_uri (file);
		g_object_unref (image);
		g_object_unref (file);
	}
	uris[i] = NULL;

	gtk_selection_data_set_uris (data, uris);
	g_strfreev (uris);
	g_list_free (list);
}

static gchar *
thumbview_get_tooltip_string (EogImage *image)
{
	gchar *bytes;
	char *type_str;
	gint width, height;
	GFile *file;
	GFileInfo *file_info;
	const char *mime_str;
	gchar *tooltip_string;
#ifdef HAVE_EXIF
	ExifData *exif_data;
#endif

	bytes = g_format_size_for_display (eog_image_get_bytes (image));

	eog_image_get_size (image, &width, &height);

	file = eog_image_get_file (image);
	file_info = g_file_query_info (file,
				       G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
				       0, NULL, NULL);
	g_object_unref (file);
	if (file_info == NULL) {
		return NULL;
	}

	mime_str = g_file_info_get_content_type (file_info);

	if (G_UNLIKELY (mime_str == NULL)) {
		g_free (bytes);
		g_object_unref (image);
		return NULL;
	}

	type_str = g_content_type_get_description (mime_str);
	g_object_unref (file_info);

	if (width > -1 && height > -1) {
		tooltip_string = g_markup_printf_escaped ("<b><big>%s</big></b>\n"
							  "%i x %i %s\n"
							  "%s\n"
							  "%s",
							  eog_image_get_caption (image),
							  width,
							  height,
							  ngettext ("pixel",
								    "pixels",
								    height),
							  bytes,
							  type_str);
	} else {
		tooltip_string = g_markup_printf_escaped ("<b><big>%s</big></b>\n"
							  "%s\n"
							  "%s",
							  eog_image_get_caption (image),
							  bytes,
							  type_str);

	}

#ifdef HAVE_EXIF
	exif_data = (ExifData *) eog_image_get_exif_info (image);

	if (exif_data) {
		gchar *extra_info, *tmp, *date;
		/* The EXIF standard says that the DATE_TIME tag is
		 * 20 bytes long. A 32-byte buffer should be large enough. */
		gchar time_buffer[32];

		date = eog_exif_util_format_date (
			eog_exif_util_get_value (exif_data, EXIF_TAG_DATE_TIME_ORIGINAL, time_buffer, 32));

		if (date) {
			extra_info = g_strdup_printf ("\n%s %s", _("Taken on"), date);

			tmp = g_strconcat (tooltip_string, extra_info, NULL);

			g_free (date);
			g_free (extra_info);
			g_free (tooltip_string);

			tooltip_string = tmp;
		}
		exif_data_unref (exif_data);
	}
#endif

	g_free (type_str);
	g_free (bytes);

	return tooltip_string;
}

static void
on_data_loaded_cb (EogJob *job, gpointer data)
{
	if (!job->error) {
		gtk_tooltip_trigger_tooltip_query (gdk_display_get_default());
	}
}

static gboolean
thumbview_on_query_tooltip_cb (GtkWidget  *widget,
			       gint        x,
			       gint        y,
			       gboolean    keyboard_mode,
			       GtkTooltip *tooltip,
			       gpointer    user_data)
{
	GtkTreePath *path;
	EogImage *image;
	gchar *tooltip_string;
	EogImageData data = 0;

	if (!gtk_icon_view_get_tooltip_context (GTK_ICON_VIEW (widget),
						&x, &y, keyboard_mode,
						NULL, &path, NULL)) {
		return FALSE;
	}

	image = eog_thumb_view_get_image_from_path (EOG_THUMB_VIEW (widget),
						    path);
	gtk_tree_path_free (path);

	if (image == NULL) {
		return FALSE;
	}

	if (!eog_image_has_data (image, EOG_IMAGE_DATA_EXIF) &&
            eog_image_get_metadata_status (image) == EOG_IMAGE_METADATA_NOT_READ) {
		data = EOG_IMAGE_DATA_EXIF;
	}

	if (!eog_image_has_data (image, EOG_IMAGE_DATA_DIMENSION)) {
		data |= EOG_IMAGE_DATA_DIMENSION;
	}

	if (data) {
		EogJob *job;

		job = eog_job_load_new (image, data);
		g_signal_connect (G_OBJECT (job), "finished",
				  G_CALLBACK (on_data_loaded_cb),
				  widget);
		eog_job_queue_add_job (job);
		g_object_unref (image);
		g_object_unref (job);
		return FALSE;
	}

	tooltip_string = thumbview_get_tooltip_string (image);
	g_object_unref (image);

	if (tooltip_string == NULL) {
		return FALSE;
	}

	gtk_tooltip_set_markup (tooltip, tooltip_string);
	g_free (tooltip_string);

	return TRUE;
}

static void
eog_thumb_view_init (EogThumbView *thumbview)
{
	thumbview->priv = EOG_THUMB_VIEW_GET_PRIVATE (thumbview);

	thumbview->priv->pixbuf_cell = gtk_cell_renderer_pixbuf_new ();

	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (thumbview),
	      	  		    thumbview->priv->pixbuf_cell,
	      			    FALSE);

	g_object_set (thumbview->priv->pixbuf_cell,
	              "follow-state", FALSE,
	              "height", 100,
	              "width", 115,
	              "yalign", 0.5,
	              "xalign", 0.5,
	              NULL);

	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (thumbview),
	                                thumbview->priv->pixbuf_cell,
	      		                "pixbuf", EOG_LIST_STORE_THUMBNAIL,
	                                NULL);

	gtk_icon_view_set_selection_mode (GTK_ICON_VIEW (thumbview),
 					  GTK_SELECTION_MULTIPLE);

	gtk_icon_view_set_column_spacing (GTK_ICON_VIEW (thumbview),
					  EOG_THUMB_VIEW_SPACING);

	gtk_icon_view_set_row_spacing (GTK_ICON_VIEW (thumbview),
				       EOG_THUMB_VIEW_SPACING);

	g_object_set (thumbview, "has-tooltip", TRUE, NULL);

	g_signal_connect (thumbview,
			  "query-tooltip",
			  G_CALLBACK (thumbview_on_query_tooltip_cb),
			  NULL);

	thumbview->priv->start_thumb = 0;
	thumbview->priv->end_thumb = 0;
	thumbview->priv->menu = NULL;

	g_signal_connect (G_OBJECT (thumbview), "parent-set",
			  G_CALLBACK (thumbview_on_parent_set_cb), NULL);

	gtk_icon_view_enable_model_drag_source (GTK_ICON_VIEW (thumbview), 0,
						NULL, 0,
						GDK_ACTION_COPY);
	gtk_drag_source_add_uri_targets (GTK_WIDGET (thumbview));

	g_signal_connect (G_OBJECT (thumbview), "drag-data-get",
			  G_CALLBACK (thumbview_on_drag_data_get_cb), NULL);
}

/**
 * eog_thumb_view_new:
 *
 * Creates a new #EogThumbView object.
 *
 * Returns: a newly created #EogThumbView.
 **/
GtkWidget *
eog_thumb_view_new (void)
{
	EogThumbView *thumbview;

	thumbview = g_object_new (EOG_TYPE_THUMB_VIEW, NULL);

	return GTK_WIDGET (thumbview);
}

/**
 * eog_thumb_view_set_model:
 * @thumbview: A #EogThumbView.
 * @store: A #EogListStore.
 *
 * Sets the #EogListStore to be used with @thumbview. If an initial image
 * was set during @store creation, its thumbnail will be selected and visible.
 *
 **/
void
eog_thumb_view_set_model (EogThumbView *thumbview, EogListStore *store)
{
	gint index;

	g_return_if_fail (EOG_IS_THUMB_VIEW (thumbview));
	g_return_if_fail (EOG_IS_LIST_STORE (store));

	index = eog_list_store_get_initial_pos (store);

	gtk_icon_view_set_model (GTK_ICON_VIEW (thumbview), GTK_TREE_MODEL (store));

	if (index >= 0) {
		GtkTreePath *path = gtk_tree_path_new_from_indices (index, -1);
		gtk_icon_view_select_path (GTK_ICON_VIEW (thumbview), path);
		gtk_icon_view_set_cursor (GTK_ICON_VIEW (thumbview), path, NULL, FALSE);
		gtk_icon_view_scroll_to_path (GTK_ICON_VIEW (thumbview), path, FALSE, 0, 0);
		gtk_tree_path_free (path);
	}
}

/**
 * eog_thumb_view_set_item_height:
 * @thumbview: A #EogThumbView.
 * @height: The desired height.
 *
 * Sets the height of each thumbnail in @thumbview.
 *
 **/
void
eog_thumb_view_set_item_height (EogThumbView *thumbview, gint height)
{
	g_return_if_fail (EOG_IS_THUMB_VIEW (thumbview));

	g_object_set (thumbview->priv->pixbuf_cell,
	              "height", height,
	              NULL);
}

static void
eog_thumb_view_get_n_selected_helper (GtkIconView *thumbview,
				      GtkTreePath *path,
				      gpointer data)
{
	/* data is of type (guint *) */
	(*(guint *) data) ++;
}

/**
 * eog_thumb_view_get_n_selected:
 * @thumbview: An #EogThumbView.
 *
 * Gets the number of images that are currently selected in @thumbview.
 *
 * Returns: the number of selected images in @thumbview.
 **/
guint
eog_thumb_view_get_n_selected (EogThumbView *thumbview)
{
	guint count = 0;
	gtk_icon_view_selected_foreach (GTK_ICON_VIEW (thumbview),
					eog_thumb_view_get_n_selected_helper,
					(&count));
	return count;
}

/**
 * eog_thumb_view_get_image_from_path:
 * @thumbview: A #EogThumbView.
 * @path: A #GtkTreePath pointing to a #EogImage in the model for @thumbview.
 *
 * Gets the #EogImage stored in @thumbview's #EogListStore at the position indicated
 * by @path.
 *
 * Returns: A #EogImage.
 **/
static EogImage *
eog_thumb_view_get_image_from_path (EogThumbView *thumbview, GtkTreePath *path)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	EogImage *image;

	model = gtk_icon_view_get_model (GTK_ICON_VIEW (thumbview));
	gtk_tree_model_get_iter (model, &iter, path);

	gtk_tree_model_get (model, &iter,
			    EOG_LIST_STORE_EOG_IMAGE, &image,
			    -1);

	return image;
}

/**
 * eog_thumb_view_get_first_selected_image:
 * @thumbview: A #EogThumbView.
 *
 * Returns the first selected image. Note that the returned #EogImage
 * is not ensured to be really the first selected image in @thumbview, but
 * generally, it will be.
 *
 * Returns: A #EogImage.
 **/
EogImage *
eog_thumb_view_get_first_selected_image (EogThumbView *thumbview)
{
	/* The returned list is not sorted! We need to find the
	   smaller tree path value => tricky and expensive. Do we really need this?
	*/
	EogImage *image;
	GtkTreePath *path;
	GList *list = gtk_icon_view_get_selected_items (GTK_ICON_VIEW (thumbview));

	if (list == NULL) {
		return NULL;
	}

	path = (GtkTreePath *) (list->data);

	image = eog_thumb_view_get_image_from_path (thumbview, path);

	g_list_foreach (list, (GFunc) gtk_tree_path_free , NULL);
	g_list_free (list);

	return image;
}

/**
 * eog_thumb_view_get_selected_images:
 * @thumbview: A #EogThumbView.
 *
 * Gets a list with the currently selected images. Note that a new reference is
 * hold for each image and the list must be freed with g_list_free().
 *
 * Returns: A newly allocated list of #EogImage's.
 **/
GList *
eog_thumb_view_get_selected_images (EogThumbView *thumbview)
{
	GList *l, *item;
	GList *list = NULL;

	GtkTreePath *path;

	l = gtk_icon_view_get_selected_items (GTK_ICON_VIEW (thumbview));

	for (item = l; item != NULL; item = item->next) {
		path = (GtkTreePath *) item->data;
		list = g_list_prepend (list, eog_thumb_view_get_image_from_path (thumbview, path));
		gtk_tree_path_free (path);
	}

	g_list_free (l);
	list = g_list_reverse (list);

	return list;
}

/**
 * eog_thumb_view_set_current_image:
 * @thumbview: A #EogThumbView.
 * @image: The image to be selected.
 * @deselect_other: Whether to deselect currently selected images.
 *
 * Changes the status of a image, marking it as currently selected.
 * If @deselect_other is %TRUE, all other selected images will be
 * deselected.
 *
 **/
void
eog_thumb_view_set_current_image (EogThumbView *thumbview, EogImage *image,
				  gboolean deselect_other)
{
	GtkTreePath *path;
	EogListStore *store;
	gint pos;

	store = EOG_LIST_STORE (gtk_icon_view_get_model (GTK_ICON_VIEW (thumbview)));
	pos = eog_list_store_get_pos_by_image (store, image);
	path = gtk_tree_path_new_from_indices (pos, -1);

	if (path == NULL) {
		return;
	}

	if (deselect_other) {
		gtk_icon_view_unselect_all (GTK_ICON_VIEW (thumbview));
	}

	gtk_icon_view_select_path (GTK_ICON_VIEW (thumbview), path);
	gtk_icon_view_set_cursor (GTK_ICON_VIEW (thumbview), path, NULL, FALSE);
	gtk_icon_view_scroll_to_path (GTK_ICON_VIEW (thumbview), path, FALSE, 0, 0);

	gtk_tree_path_free (path);
}

/**
 * eog_thumb_view_select_single:
 * @thumbview: A #EogThumbView.
 * @change: A #EogThumbViewSelectionChange, describing the
 * desired selection change.
 *
 * Changes the current selection according to a single movement
 * described by #EogThumbViewSelectionChange. If there are no
 * thumbnails currently selected, one is selected according to the
 * natural selection according to the #EogThumbViewSelectionChange
 * used, p.g., when %EOG_THUMB_VIEW_SELECT_RIGHT is the selected change,
 * the first thumbnail will be selected.
 *
 **/
void
eog_thumb_view_select_single (EogThumbView *thumbview,
			      EogThumbViewSelectionChange change)
{
  	GtkTreePath *path = NULL;
	GtkTreeModel *model;
	GList *list;
	gint n_items;

	g_return_if_fail (EOG_IS_THUMB_VIEW (thumbview));

	model = gtk_icon_view_get_model (GTK_ICON_VIEW (thumbview));

	n_items = eog_list_store_length (EOG_LIST_STORE (model));

	if (n_items == 0) {
		return;
	}

	if (eog_thumb_view_get_n_selected (thumbview) == 0) {
		switch (change) {
		case EOG_THUMB_VIEW_SELECT_CURRENT:
			break;
		case EOG_THUMB_VIEW_SELECT_RIGHT:
		case EOG_THUMB_VIEW_SELECT_FIRST:
			path = gtk_tree_path_new_first ();
			break;
		case EOG_THUMB_VIEW_SELECT_LEFT:
		case EOG_THUMB_VIEW_SELECT_LAST:
			path = gtk_tree_path_new_from_indices (n_items - 1, -1);
			break;
		case EOG_THUMB_VIEW_SELECT_RANDOM:
			path = gtk_tree_path_new_from_indices ((int)(((float)(n_items - 1) * rand()) / (float)(RAND_MAX + 1.f)), -1);
			break;
		}
	} else {
		list = gtk_icon_view_get_selected_items (GTK_ICON_VIEW (thumbview));
		path = gtk_tree_path_copy ((GtkTreePath *) list->data);
		g_list_foreach (list, (GFunc) gtk_tree_path_free , NULL);
		g_list_free (list);

		gtk_icon_view_unselect_all (GTK_ICON_VIEW (thumbview));

		switch (change) {
		case EOG_THUMB_VIEW_SELECT_CURRENT:
			break;
		case EOG_THUMB_VIEW_SELECT_LEFT:
			if (!gtk_tree_path_prev (path)) {
				gtk_tree_path_free (path);
				path = gtk_tree_path_new_from_indices (n_items - 1, -1);
			}
			break;
		case EOG_THUMB_VIEW_SELECT_RIGHT:
			if (gtk_tree_path_get_indices (path) [0] == n_items - 1) {
				gtk_tree_path_free (path);
				path = gtk_tree_path_new_first ();
			} else {
				gtk_tree_path_next (path);
			}
			break;
		case EOG_THUMB_VIEW_SELECT_FIRST:
			gtk_tree_path_free (path);
			path = gtk_tree_path_new_first ();
			break;
		case EOG_THUMB_VIEW_SELECT_LAST:
			gtk_tree_path_free (path);
			path = gtk_tree_path_new_from_indices (n_items - 1, -1);
			break;
		case EOG_THUMB_VIEW_SELECT_RANDOM:
			gtk_tree_path_free (path);
			path = gtk_tree_path_new_from_indices ((int)(((float)(n_items - 1) * rand()) / (float)(RAND_MAX + 1.f)), -1);
			break;
		}
	}

	gtk_icon_view_select_path (GTK_ICON_VIEW (thumbview), path);
	gtk_icon_view_set_cursor (GTK_ICON_VIEW (thumbview), path, NULL, FALSE);
	gtk_icon_view_scroll_to_path (GTK_ICON_VIEW (thumbview), path, FALSE, 0, 0);
	gtk_tree_path_free (path);
}


/**
 * eog_thumb_view_set_thumbnail_popup:
 * @thumbview: An #EogThumbView.
 * @menu: A #GtkMenu.
 *
 * Set the contextual menu to be used with the thumbnails in the
 * widget. This can be done only once.
 *
 **/
void
eog_thumb_view_set_thumbnail_popup (EogThumbView *thumbview,
				    GtkMenu      *menu)
{
	g_return_if_fail (EOG_IS_THUMB_VIEW (thumbview));
	g_return_if_fail (thumbview->priv->menu == NULL);

	thumbview->priv->menu = g_object_ref (menu);

	gtk_menu_attach_to_widget (GTK_MENU (thumbview->priv->menu),
				   GTK_WIDGET (thumbview),
				   NULL);

	g_signal_connect (G_OBJECT (thumbview), "button_press_event",
			  G_CALLBACK (thumbview_on_button_press_event_cb), NULL);

}


static void
eog_thumb_view_popup_menu (EogThumbView *thumbview, GdkEventButton *event)
{
	GtkWidget *popup;
	int button, event_time;

	popup = thumbview->priv->menu;

	if (event) {
		button = event->button;
		event_time = event->time;
	} else {
		button = 0;
		event_time = gtk_get_current_event_time ();
	}

	gtk_menu_popup (GTK_MENU (popup), NULL, NULL, NULL, NULL,
			button, event_time);
}
