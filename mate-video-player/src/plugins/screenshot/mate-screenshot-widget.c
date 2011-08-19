/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * MATE Screenshot widget
 * Copyright (C) 2001-2006  Jonathan Blandford <jrb@alum.mit.edu>
 * Copyright (C) Philip Withnall 2009 <philip@tecnocode.co.uk>
 *
 * MATE Screenshot widget is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * MATE Screenshot widget is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MATE Screenshot widget.  If not, see <http://www.gnu.org/licenses/>.
 *
 * The Totem project hereby grant permission for non-GPL compatible GStreamer
 * plugins to be used and distributed together with GStreamer and Totem. This
 * permission are above and beyond the permissions granted by the GPL license
 * Totem is covered by.
 *
 * Monday 10th August 2009: Philip Withnall: Add exception clause.
 * Permission from the previous authors granted via e-mail.
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "mate-screenshot-widget.h"

static void mate_screenshot_widget_dispose (GObject *object);
static void mate_screenshot_widget_finalize (GObject *object);
static void mate_screenshot_widget_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void mate_screenshot_widget_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

/* GtkBuilder callbacks */
G_MODULE_EXPORT gboolean on_preview_button_press_event (GtkWidget *drawing_area, GdkEventButton *event, MateScreenshotWidget *self);
G_MODULE_EXPORT void on_preview_expose_event (GtkWidget *drawing_area, GdkEventExpose *event, MateScreenshotWidget *self);
G_MODULE_EXPORT gboolean on_preview_button_release_event (GtkWidget *drawing_area, GdkEventButton *event, MateScreenshotWidget *self);
G_MODULE_EXPORT void on_preview_configure_event (GtkWidget *drawing_area, GdkEventConfigure *event, MateScreenshotWidget *self);
G_MODULE_EXPORT void on_preview_drag_data_get (GtkWidget *widget, GdkDragContext *context, GtkSelectionData *selection_data, guint info, guint time,
					       MateScreenshotWidget *self);
G_MODULE_EXPORT void on_preview_drag_begin (GtkWidget *widget, GdkDragContext *context, MateScreenshotWidget *self);

struct _MateScreenshotWidgetPrivate {
	GdkPixbuf *screenshot;
	GdkPixbuf *preview_image;
	GtkFileChooser *file_chooser;
	GtkEntry *filename_entry;
	GtkWidget *preview_area;
	gint drag_x;
	gint drag_y;
	gchar *temporary_filename;
};

enum {
	PROP_TEMPORARY_FILENAME = 1
};

enum {
	TYPE_TEXT_URI_LIST,
	TYPE_IMAGE_PNG,
	LAST_TYPE
};

static GtkTargetEntry drag_types[] = {
	/* typecasts are to shut gcc up */
	{ (gchar*) "text/uri-list", 0, TYPE_TEXT_URI_LIST },
	{ (gchar*) "image/png", 0, TYPE_IMAGE_PNG }
};

static GtkTargetEntry *drag_types_no_uris = drag_types + sizeof (GtkTargetEntry);

G_DEFINE_TYPE (MateScreenshotWidget, mate_screenshot_widget, GTK_TYPE_BOX)
#define MATE_SCREENSHOT_WIDGET_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), MATE_TYPE_SCREENSHOT_WIDGET, MateScreenshotWidgetPrivate))

static void
mate_screenshot_widget_class_init (MateScreenshotWidgetClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (MateScreenshotWidgetPrivate));

	gobject_class->get_property = mate_screenshot_widget_get_property;
	gobject_class->set_property = mate_screenshot_widget_set_property;
	gobject_class->dispose = mate_screenshot_widget_dispose;
	gobject_class->finalize = mate_screenshot_widget_finalize;

	g_object_class_install_property (gobject_class, PROP_TEMPORARY_FILENAME,
				g_param_spec_string ("temporary-filename",
					"Temporary filename", "The filename of a temporary file for use in drag-and-drop.",
					NULL,
					G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
mate_screenshot_widget_init (MateScreenshotWidget *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, MATE_TYPE_SCREENSHOT_WIDGET, MateScreenshotWidgetPrivate);
}

static void
mate_screenshot_widget_dispose (GObject *object)
{
	MateScreenshotWidgetPrivate *priv = MATE_SCREENSHOT_WIDGET (object)->priv;

	if (priv->screenshot != NULL)
		g_object_unref (priv->screenshot);
	priv->screenshot = NULL;

	if (priv->preview_image != NULL)
		g_object_unref (priv->preview_image);
	priv->preview_image = NULL;

	/* Chain up to the parent class */
	G_OBJECT_CLASS (mate_screenshot_widget_parent_class)->dispose (object);
}

static void
mate_screenshot_widget_finalize (GObject *object)
{
	MateScreenshotWidgetPrivate *priv = MATE_SCREENSHOT_WIDGET (object)->priv;

	g_free (priv->temporary_filename);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (mate_screenshot_widget_parent_class)->finalize (object);
}

static void
mate_screenshot_widget_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	MateScreenshotWidgetPrivate *priv = MATE_SCREENSHOT_WIDGET (object)->priv;

	switch (property_id) {
		case PROP_TEMPORARY_FILENAME:
			g_value_set_string (value, priv->temporary_filename);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
mate_screenshot_widget_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	MateScreenshotWidget *self = MATE_SCREENSHOT_WIDGET (object);

	switch (property_id) {
		case PROP_TEMPORARY_FILENAME:
			mate_screenshot_widget_set_temporary_filename (self, g_value_get_string (value));
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
on_filename_entry_realize (GtkWidget *filename_entry, gchar *current_name)
{
	const gchar *ext;
	gint pos;

	/* Select the name of the file but leave out the extension if there's any;
	 * the widget must be realized for select_region to work */
	ext = g_utf8_strrchr (current_name, -1, '.');
	if (ext)
		pos = g_utf8_strlen (current_name, -1) - g_utf8_strlen (ext, -1);
	else
		pos = -1;

	gtk_editable_select_region (GTK_EDITABLE (filename_entry), 0, pos);

	/* Disconnect the signal and free the data */
	g_signal_handlers_disconnect_by_func (filename_entry, on_filename_entry_realize, current_name);
	g_free (current_name);
}

GtkWidget *
mate_screenshot_widget_new (const gchar *interface_filename, GdkPixbuf *screenshot, const gchar *initial_uri)
{
	MateScreenshotWidgetPrivate *priv;
	MateScreenshotWidget *screenshot_widget;
	GtkBuilder *builder;
	GtkAspectFrame *aspect_frame;
	gchar *current_folder, *current_name;
	const gchar *dir;
	GFile *tmp_file, *parent_file;
	gint width, height;

	builder = gtk_builder_new ();
	if (gtk_builder_add_from_file (builder, interface_filename, NULL) == FALSE) {
		g_object_unref (builder);
		return NULL;
	}

	gtk_builder_set_translation_domain (builder, GETTEXT_PACKAGE);
	screenshot_widget = MATE_SCREENSHOT_WIDGET (gtk_builder_get_object (builder, "screenshot_widget"));
	g_object_ref_sink (screenshot_widget);
	gtk_builder_connect_signals (builder, screenshot_widget);

	if (screenshot_widget == NULL) {
		g_object_unref (builder);
		return NULL;
	}

	/* Set up with the data we're passed */
	priv = screenshot_widget->priv;
	priv->screenshot = g_object_ref (screenshot);

	/* Grab our child widgets */
	priv->file_chooser = GTK_FILE_CHOOSER (gtk_builder_get_object (builder, "file_chooser_button"));
	priv->filename_entry = GTK_ENTRY (gtk_builder_get_object (builder, "filename_entry"));
	priv->preview_area = GTK_WIDGET (gtk_builder_get_object (builder, "preview_darea"));
	aspect_frame = GTK_ASPECT_FRAME (gtk_builder_get_object (builder, "aspect_frame"));

	/* Set up the file chooser and filename entry */
	tmp_file = g_file_new_for_uri (initial_uri);
	parent_file = g_file_get_parent (tmp_file);
	current_name = g_file_get_basename (tmp_file);
	current_folder = g_file_get_uri (parent_file);
	g_object_unref (tmp_file);
	g_object_unref (parent_file);

	gtk_file_chooser_set_current_folder_uri (priv->file_chooser, current_folder);
	gtk_entry_set_text (priv->filename_entry, current_name);
	g_free (current_folder);

	/* Add the special pictures directory to the file chooser */
	dir = g_get_user_special_dir (G_USER_DIRECTORY_PICTURES);
	if (dir != NULL)
		gtk_file_chooser_add_shortcut_folder (priv->file_chooser, dir, NULL);

	/* Select the filename (except the extension) when the filename entry is realised.
	 * The on_filename_entry_realize function will free current_name. */
	g_signal_connect (priv->filename_entry, "realize", G_CALLBACK (on_filename_entry_realize), current_name);

	/* Set up the preview area */
	width = gdk_pixbuf_get_width (screenshot) / 5;
	height = gdk_pixbuf_get_height (screenshot) / 5;

	gtk_widget_set_size_request (priv->preview_area, width, height);
	gtk_aspect_frame_set (aspect_frame, 0.0, 0.5, (gfloat) gdk_pixbuf_get_width (screenshot) / (gfloat) gdk_pixbuf_get_height (screenshot), FALSE);

	g_object_unref (builder);

	/* We had to sink the object reference before to prevent it being destroyed when unreffing the builder,
	 * but in order to return a floating-referenced widget (as is the convention), we need to force the
	 * reference to be floating again. */
	g_object_force_floating (G_OBJECT (screenshot_widget));

	return GTK_WIDGET (screenshot_widget);
}

void
on_preview_expose_event (GtkWidget *drawing_area, GdkEventExpose *event, MateScreenshotWidget *self)
{
	GdkPixbuf *pixbuf = NULL;
	gboolean free_pixbuf = FALSE;
	GtkStyle *style;
	cairo_t *cr;

	style = gtk_widget_get_style (drawing_area);

	/* Stolen from GtkImage.  I really should just make the drawing area an
	 * image some day (TODO) */
	if (gtk_widget_get_state (drawing_area) != GTK_STATE_NORMAL) {
		GtkIconSource *source;

		source = gtk_icon_source_new ();
		gtk_icon_source_set_pixbuf (source, self->priv->preview_image);
		gtk_icon_source_set_size (source, GTK_ICON_SIZE_SMALL_TOOLBAR);
		gtk_icon_source_set_size_wildcarded (source, FALSE);

		pixbuf = gtk_style_render_icon (style, source,
						gtk_widget_get_direction (drawing_area), gtk_widget_get_state (drawing_area),
						(GtkIconSize) -1, drawing_area, "gtk-image");
		free_pixbuf = TRUE;
		gtk_icon_source_free (source);
	} else {
		pixbuf = g_object_ref (self->priv->preview_image);
	}

	cr = gdk_cairo_create (gtk_widget_get_window (drawing_area));

	/* FIXME: Draw it insensitive in that case */
	gdk_cairo_rectangle (cr, &(event->area));
	cairo_clip (cr);
	gdk_cairo_set_source_pixbuf (cr, pixbuf, 0.0, 0.0);
	cairo_paint (cr);

	cairo_destroy (cr);
	g_object_unref (pixbuf);
}

gboolean
on_preview_button_press_event (GtkWidget *drawing_area, GdkEventButton *event, MateScreenshotWidget *self)
{
	self->priv->drag_x = (gint) event->x;
	self->priv->drag_y = (gint) event->y;

	return FALSE;
}

gboolean
on_preview_button_release_event (GtkWidget *drawing_area, GdkEventButton *event, MateScreenshotWidget *self)
{
	self->priv->drag_x = 0;
	self->priv->drag_y = 0;

	return FALSE;
}

void
on_preview_configure_event (GtkWidget *drawing_area, GdkEventConfigure *event, MateScreenshotWidget *self)
{
	/* Re-scale the preview image */
	if (self->priv->preview_image)
		g_object_unref (self->priv->preview_image);
	self->priv->preview_image = gdk_pixbuf_scale_simple (self->priv->screenshot, event->width, event->height, GDK_INTERP_BILINEAR);
}

void
on_preview_drag_data_get (GtkWidget *widget, GdkDragContext *context, GtkSelectionData *selection_data, guint info, guint _time,
			  MateScreenshotWidget *self)
{
	switch (info) {
	case TYPE_TEXT_URI_LIST: {
		gchar *uris[2];

		g_assert (self->priv->temporary_filename != NULL);

		uris[0] = g_filename_to_uri (self->priv->temporary_filename, NULL, NULL);
		uris[1] = NULL;

		gtk_selection_data_set_uris (selection_data, uris);
		g_free (uris[0]);

		break;
	}
	case TYPE_IMAGE_PNG:
		gtk_selection_data_set_pixbuf (selection_data, self->priv->screenshot);
		break;
	case LAST_TYPE:
	default:
		g_warning ("Unknown type %d", info);
	}
}

void
on_preview_drag_begin (GtkWidget *widget, GdkDragContext *context, MateScreenshotWidget *self)
{
	gtk_drag_set_icon_pixbuf (context, self->priv->preview_image, self->priv->drag_x, self->priv->drag_y);
}

void
mate_screenshot_widget_focus_entry (MateScreenshotWidget *self)
{
	g_return_if_fail (MATE_IS_SCREENSHOT_WIDGET (self));
	gtk_widget_grab_focus (GTK_WIDGET (self->priv->filename_entry));
}

gchar *
mate_screenshot_widget_get_uri (MateScreenshotWidget *self)
{
	gchar *folder;
	const gchar *filename;
	gchar *uri, *file, *tmp;
	GError *error = NULL;

	g_return_val_if_fail (MATE_IS_SCREENSHOT_WIDGET (self), NULL);

	folder = gtk_file_chooser_get_current_folder_uri (self->priv->file_chooser);
	filename = gtk_entry_get_text (self->priv->filename_entry);

	tmp = g_filename_from_utf8 (filename, -1, NULL, NULL, &error);
	if (error != NULL) {
		g_warning ("Unable to convert \"%s\" to valid UTF-8: %s\nFalling back to default filename.", filename, error->message);
		g_error_free (error);
		tmp = g_strdup (_("Screenshot.png"));
	}

	file = g_uri_escape_string (tmp, NULL, FALSE);
	uri = g_build_filename (folder, file, NULL);

	g_free (folder);
	g_free (tmp);
	g_free (file);

	return uri;
}

gchar *
mate_screenshot_widget_get_folder (MateScreenshotWidget *self)
{
	g_return_val_if_fail (MATE_IS_SCREENSHOT_WIDGET (self), NULL);
	return gtk_file_chooser_get_current_folder_uri (self->priv->file_chooser);
}

GdkPixbuf *
mate_screenshot_widget_get_screenshot (MateScreenshotWidget *self)
{
	g_return_val_if_fail (MATE_IS_SCREENSHOT_WIDGET (self), NULL);
	return self->priv->screenshot;
}

const gchar *
mate_screenshot_widget_get_temporary_filename (MateScreenshotWidget *self)
{
	g_return_val_if_fail (MATE_IS_SCREENSHOT_WIDGET (self), NULL);
	return self->priv->temporary_filename;
}

void
mate_screenshot_widget_set_temporary_filename (MateScreenshotWidget *self, const gchar *temporary_filename)
{
	MateScreenshotWidgetPrivate *priv = self->priv;

	g_free (priv->temporary_filename);
	priv->temporary_filename = g_strdup (temporary_filename);

	/* Set/Unset the preview area as a drag source based on whether we have a temporary file */
	if (priv->temporary_filename == NULL) {
		/* We can only provide pixbuf data */
		gtk_drag_source_set (priv->preview_area, GDK_BUTTON1_MASK | GDK_BUTTON3_MASK,
				     drag_types_no_uris, G_N_ELEMENTS (drag_types_no_uris), GDK_ACTION_COPY);
	} else {
		/* We can provide pixbuf data and temporary file URIs */
		gtk_drag_source_set (priv->preview_area, GDK_BUTTON1_MASK | GDK_BUTTON3_MASK,
				     drag_types, G_N_ELEMENTS (drag_types), GDK_ACTION_COPY);
	}
}
