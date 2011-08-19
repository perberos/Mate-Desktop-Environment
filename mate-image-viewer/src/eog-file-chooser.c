/*
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
 * Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "eog-file-chooser.h"
#include "eog-config-keys.h"
#include "eog-pixbuf-util.h"

#include <stdlib.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <mateconf/mateconf-client.h>

/* We must define MATE_DESKTOP_USE_UNSTABLE_API to be able
   to use MateDesktopThumbnail */
#ifndef MATE_DESKTOP_USE_UNSTABLE_API
#define MATE_DESKTOP_USE_UNSTABLE_API
#endif
#include <libmateui/mate-desktop-thumbnail.h>

static char *last_dir[] = { NULL, NULL, NULL, NULL };

#define FILE_FORMAT_KEY "file-format"

#define EOG_FILE_CHOOSER_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), \
					     EOG_TYPE_FILE_CHOOSER,		    \
					     EogFileChooserPrivate))

struct _EogFileChooserPrivate
{
	MateDesktopThumbnailFactory *thumb_factory;

	GtkWidget *image;
	GtkWidget *size_label;
	GtkWidget *dim_label;
	GtkWidget *creator_label;
};

G_DEFINE_TYPE(EogFileChooser, eog_file_chooser, GTK_TYPE_FILE_CHOOSER_DIALOG)

static void
eog_file_chooser_finalize (GObject *object)
{
	EogFileChooserPrivate *priv;

	priv = EOG_FILE_CHOOSER (object)->priv;

	if (priv->thumb_factory != NULL)
		g_object_unref (priv->thumb_factory);

	(* G_OBJECT_CLASS (eog_file_chooser_parent_class)->finalize) (object);
}

static void
eog_file_chooser_class_init (EogFileChooserClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;

	object_class->finalize = eog_file_chooser_finalize;

	g_type_class_add_private (object_class, sizeof (EogFileChooserPrivate));
}

static void
eog_file_chooser_init (EogFileChooser *chooser)
{
	chooser->priv = EOG_FILE_CHOOSER_GET_PRIVATE (chooser);
}

static void
response_cb (GtkDialog *dlg, gint id, gpointer data)
{
	char *dir;
	GtkFileChooserAction action;

	if (id == GTK_RESPONSE_OK) {
		dir = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (dlg));
		action = gtk_file_chooser_get_action (GTK_FILE_CHOOSER (dlg));

		if (last_dir [action] != NULL)
			g_free (last_dir [action]);

		last_dir [action] = dir;
	}
}

static void
save_response_cb (GtkDialog *dlg, gint id, gpointer data)
{
	GFile *file;
	GdkPixbufFormat *format;

	if (id != GTK_RESPONSE_OK)
		return;

	file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dlg));
	format = eog_pixbuf_get_format (file);
	g_object_unref (file);

	if (!format || !gdk_pixbuf_format_is_writable (format)) {
		GtkWidget *msg_dialog;

		msg_dialog = gtk_message_dialog_new (
						     GTK_WINDOW (dlg),
						     GTK_DIALOG_MODAL,
						     GTK_MESSAGE_ERROR,
						     GTK_BUTTONS_OK,
						     _("File format is unknown or unsupported"));

		gtk_message_dialog_format_secondary_text (
						GTK_MESSAGE_DIALOG (msg_dialog),
						"%s\n%s",
		 				_("Eye of MATE could not determine a supported writable file format based on the filename."),
		  				_("Please try a different file extension like .png or .jpg."));

		gtk_dialog_run (GTK_DIALOG (msg_dialog));
		gtk_widget_destroy (msg_dialog);

		g_signal_stop_emission_by_name (dlg, "response");
	} else {
		response_cb (dlg, id, data);
	}
}

static void
eog_file_chooser_add_filter (EogFileChooser *chooser)
{
	GSList *it;
	GSList *formats;
 	GtkFileFilter *all_file_filter;
	GtkFileFilter *all_img_filter;
	GtkFileFilter *filter;
	GSList *filters = NULL;
	gchar **mime_types, **pattern, *tmp;
	int i;
	GtkFileChooserAction action;

	action = gtk_file_chooser_get_action (GTK_FILE_CHOOSER (chooser));

	if (action != GTK_FILE_CHOOSER_ACTION_SAVE && action != GTK_FILE_CHOOSER_ACTION_OPEN) {
		return;
	}

	/* All Files Filter */
	all_file_filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (all_file_filter, _("All Files"));
	gtk_file_filter_add_pattern (all_file_filter, "*");

	/* All Image Filter */
	all_img_filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (all_img_filter, _("All Images"));

	if (action == GTK_FILE_CHOOSER_ACTION_SAVE) {
		formats = eog_pixbuf_get_savable_formats ();
	}
	else {
		formats = gdk_pixbuf_get_formats ();
	}

	/* Image filters */
	for (it = formats; it != NULL; it = it->next) {
		char *filter_name;
		char *description, *extension;
		GdkPixbufFormat *format;
		filter = gtk_file_filter_new ();

		format = (GdkPixbufFormat*) it->data;
		description = gdk_pixbuf_format_get_description (format);
		extension = gdk_pixbuf_format_get_name (format);

		/* Filter name: First description then file extension, eg. "The PNG-Format (*.png)".*/
		filter_name = g_strdup_printf (_("%s (*.%s)"), description, extension);
		g_free (description);
		g_free (extension);

		gtk_file_filter_set_name (filter, filter_name);
		g_free (filter_name);

		mime_types = gdk_pixbuf_format_get_mime_types ((GdkPixbufFormat *) it->data);
		for (i = 0; mime_types[i] != NULL; i++) {
			gtk_file_filter_add_mime_type (filter, mime_types[i]);
			gtk_file_filter_add_mime_type (all_img_filter, mime_types[i]);
		}
		g_strfreev (mime_types);

		pattern = gdk_pixbuf_format_get_extensions ((GdkPixbufFormat *) it->data);
		for (i = 0; pattern[i] != NULL; i++) {
			tmp = g_strconcat ("*.", pattern[i], NULL);
			gtk_file_filter_add_pattern (filter, tmp);
			gtk_file_filter_add_pattern (all_img_filter, tmp);
			g_free (tmp);
		}
		g_strfreev (pattern);

		/* attach GdkPixbufFormat to filter, see also
		 * eog_file_chooser_get_format. */
		g_object_set_data (G_OBJECT (filter),
				   FILE_FORMAT_KEY,
				   format);

		filters = g_slist_prepend (filters, filter);
	}
	g_slist_free (formats);

	/* Add filter to filechooser */
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), all_file_filter);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), all_img_filter);
	gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (chooser), all_img_filter);

	for (it = filters; it != NULL; it = it->next) {
		gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), GTK_FILE_FILTER (it->data));
	}
	g_slist_free (filters);
}

static void
set_preview_label (GtkWidget *label, const char *str)
{
	if (str == NULL) {
		gtk_widget_hide (GTK_WIDGET (label));
	}
	else {
		gtk_label_set_text (GTK_LABEL (label), str);
		gtk_widget_show (GTK_WIDGET (label));
	}
}

/* Sets the pixbuf as preview thumbnail and tries to read and display
 * further information according to the thumbnail spec.
 */
static void
set_preview_pixbuf (EogFileChooser *chooser, GdkPixbuf *pixbuf, goffset size)
{
	EogFileChooserPrivate *priv;
	int bytes;
	int pixels;
	const char *bytes_str;
	const char *width;
	const char *height;
	const char *creator = NULL;
	char *size_str    = NULL;
	char *dim_str     = NULL;

	g_return_if_fail (EOG_IS_FILE_CHOOSER (chooser));

	priv = chooser->priv;

	gtk_image_set_from_pixbuf (GTK_IMAGE (priv->image), pixbuf);

	if (pixbuf != NULL) {
		/* try to read file size */
		bytes_str = gdk_pixbuf_get_option (pixbuf, "tEXt::Thumb::Size");
		if (bytes_str != NULL) {
			bytes = atoi (bytes_str);
			size_str = g_format_size_for_display (bytes);
		}
		else {
			size_str = g_format_size_for_display (size);
		}

		/* try to read image dimensions */
		width  = gdk_pixbuf_get_option (pixbuf, "tEXt::Thumb::Image::Width");
		height = gdk_pixbuf_get_option (pixbuf, "tEXt::Thumb::Image::Height");

		if ((width != NULL) && (height != NULL)) {
			pixels = atoi (height);
			/* Pixel size of image: width x height in pixel */
			dim_str = g_strdup_printf ("%s x %s %s", width, height, ngettext ("pixel", "pixels", pixels));
		}

#if 0
		/* Not sure, if this is really useful, therefore its commented out for now. */

		/* try to read creator of the thumbnail */
		creator = gdk_pixbuf_get_option (pixbuf, "tEXt::Software");

		/* stupid workaround to display nicer string if the
		 * thumbnail is created through the mate libraries.
		 */
		if (g_ascii_strcasecmp (creator, "Mate::ThumbnailFactory") == 0) {
			creator = "MATE Libs";
		}
#endif
	}

	set_preview_label (priv->size_label, size_str);
	set_preview_label (priv->dim_label, dim_str);
	set_preview_label (priv->creator_label, creator);

	if (size_str != NULL) {
		g_free (size_str);
	}

	if (dim_str != NULL) {
		g_free (dim_str);
	}
}

static void
update_preview_cb (GtkFileChooser *file_chooser, gpointer data)
{
	EogFileChooserPrivate *priv;
	char *uri;
	char *thumb_path = NULL;
	GFile *file;
	GFileInfo *file_info;
	GdkPixbuf *pixbuf = NULL;
	gboolean have_preview = FALSE;

	priv = EOG_FILE_CHOOSER (file_chooser)->priv;

	uri = gtk_file_chooser_get_preview_uri (file_chooser);
	if (uri == NULL) {
		gtk_file_chooser_set_preview_widget_active (file_chooser, FALSE);
		return;
	}

	file = g_file_new_for_uri (uri);
	file_info = g_file_query_info (file,
				       G_FILE_ATTRIBUTE_TIME_MODIFIED ","
				       G_FILE_ATTRIBUTE_STANDARD_SIZE,
				       0, NULL, NULL);
	g_object_unref (file);

	if ((file_info != NULL) && (priv->thumb_factory != NULL)) {
		guint64 mtime;

		mtime = g_file_info_get_attribute_uint64 (file_info,
							  G_FILE_ATTRIBUTE_TIME_MODIFIED);
		thumb_path = mate_desktop_thumbnail_factory_lookup (priv->thumb_factory, uri, mtime);
		if (thumb_path == NULL) {
			/* read files smaller than 100kb directly */
			if (g_file_info_get_size (file_info) <= 100000) {
				/* FIXME: we should then output also the image dimensions */
				thumb_path = gtk_file_chooser_get_preview_filename (file_chooser);
			}
		}

		if (thumb_path != NULL && g_file_test (thumb_path, G_FILE_TEST_EXISTS)) {
			/* try to load and display preview thumbnail */
			pixbuf = gdk_pixbuf_new_from_file (thumb_path, NULL);

			have_preview = (pixbuf != NULL);

			set_preview_pixbuf (EOG_FILE_CHOOSER (file_chooser), pixbuf,
					    g_file_info_get_size (file_info));

			if (pixbuf != NULL) {
				g_object_unref (pixbuf);
			}
		}
	}

	if (thumb_path != NULL) {
		g_free (thumb_path);
	}

	g_free (uri);
	g_object_unref (file_info);

	gtk_file_chooser_set_preview_widget_active (file_chooser, have_preview);
}

static void
eog_file_chooser_add_preview (GtkWidget *widget)
{
	EogFileChooserPrivate *priv;
	GtkWidget *vbox;

	priv = EOG_FILE_CHOOSER (widget)->priv;

	vbox = gtk_vbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);

	priv->image      = gtk_image_new ();
	/* 128x128 is maximum size of thumbnails */
	gtk_widget_set_size_request (priv->image, 128,128);

	priv->dim_label  = gtk_label_new (NULL);
	priv->size_label = gtk_label_new (NULL);
	priv->creator_label = gtk_label_new (NULL);

	gtk_box_pack_start (GTK_BOX (vbox), priv->image, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), priv->dim_label, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), priv->size_label, FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), priv->creator_label, FALSE, TRUE, 0);

	gtk_widget_show_all (vbox);

	gtk_file_chooser_set_preview_widget (GTK_FILE_CHOOSER (widget), vbox);
	gtk_file_chooser_set_preview_widget_active (GTK_FILE_CHOOSER (widget), FALSE);

	priv->thumb_factory = mate_desktop_thumbnail_factory_new (MATE_DESKTOP_THUMBNAIL_SIZE_NORMAL);

	g_signal_connect (widget, "update-preview",
			  G_CALLBACK (update_preview_cb), NULL);
}

GtkWidget *
eog_file_chooser_new (GtkFileChooserAction action)
{
	GtkWidget *chooser;
	gchar *title = NULL;

	chooser = g_object_new (EOG_TYPE_FILE_CHOOSER,
				"action", action,
				"select-multiple", (action == GTK_FILE_CHOOSER_ACTION_OPEN),
				"local-only", FALSE,
				NULL);

	switch (action) {
	case GTK_FILE_CHOOSER_ACTION_OPEN:
		gtk_dialog_add_buttons (GTK_DIALOG (chooser),
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OPEN, GTK_RESPONSE_OK,
					NULL);
		title = _("Open Image");
		break;

	case GTK_FILE_CHOOSER_ACTION_SAVE:
		gtk_dialog_add_buttons (GTK_DIALOG (chooser),
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_SAVE, GTK_RESPONSE_OK,
					NULL);
		title = _("Save Image");
		break;

	case GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER:
		gtk_dialog_add_buttons (GTK_DIALOG (chooser),
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_OPEN, GTK_RESPONSE_OK,
					NULL);
		title = _("Open Folder");
		break;

	default:
		g_assert_not_reached ();
	}

	if (action != GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER) {
		eog_file_chooser_add_filter (EOG_FILE_CHOOSER (chooser));
		eog_file_chooser_add_preview (chooser);
	}

	if (last_dir[action] != NULL) {
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser), last_dir [action]);
	}

	g_signal_connect (chooser, "response",
			  G_CALLBACK ((action == GTK_FILE_CHOOSER_ACTION_SAVE) ?
				      save_response_cb : response_cb),
			  NULL);

 	gtk_window_set_title (GTK_WINDOW (chooser), title);
	gtk_dialog_set_default_response (GTK_DIALOG (chooser), GTK_RESPONSE_OK);

	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (chooser), TRUE);

	return chooser;
}

GdkPixbufFormat *
eog_file_chooser_get_format (EogFileChooser *chooser)
{
	GtkFileFilter *filter;
	GdkPixbufFormat* format;

	g_return_val_if_fail (EOG_IS_FILE_CHOOSER (chooser), NULL);

	filter = gtk_file_chooser_get_filter (GTK_FILE_CHOOSER (chooser));
	if (filter == NULL)
		return NULL;

	format = g_object_get_data (G_OBJECT (filter), FILE_FORMAT_KEY);

	return format;
}
