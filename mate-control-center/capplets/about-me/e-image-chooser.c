/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* e-image-chooser.c
 * Copyright (C) 2004  Novell, Inc.
 * Author: Chris Toshok <toshok@ximian.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include <string.h>

#include <glib-object.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#include "e-image-chooser.h"

struct _EImageChooserPrivate {

	GtkWidget *image;
	GtkWidget *browse_button;

	char *image_buf;
	int   image_buf_size;
	int   image_width;
	int   image_height;

	gboolean editable;
};

enum {
	CHANGED,
	LAST_SIGNAL
};


static gint image_chooser_signals [LAST_SIGNAL] = { 0 };

static void e_image_chooser_init	 (EImageChooser		 *chooser);
static void e_image_chooser_class_init	 (EImageChooserClass	 *klass);
static void e_image_chooser_dispose      (GObject *object);

static gboolean image_drag_motion_cb (GtkWidget *widget,
				      GdkDragContext *context,
				      gint x, gint y, guint time, EImageChooser *chooser);
static gboolean image_drag_drop_cb (GtkWidget *widget,
				    GdkDragContext *context,
				    gint x, gint y, guint time, EImageChooser *chooser);
static void image_drag_data_received_cb (GtkWidget *widget,
					 GdkDragContext *context,
					 gint x, gint y,
					 GtkSelectionData *selection_data,
					 guint info, guint time, EImageChooser *chooser);

static GtkObjectClass *parent_class = NULL;
#define PARENT_TYPE GTK_TYPE_VBOX

enum DndTargetType {
	DND_TARGET_TYPE_URI_LIST
};
#define URI_LIST_TYPE "text/uri-list"

static GtkTargetEntry image_drag_types[] = {
	{ URI_LIST_TYPE, 0, DND_TARGET_TYPE_URI_LIST },
};
static const int num_image_drag_types = sizeof (image_drag_types) / sizeof (image_drag_types[0]);

GtkWidget *
e_image_chooser_new (void)
{
	return g_object_new (E_TYPE_IMAGE_CHOOSER, NULL);
}

GType
e_image_chooser_get_type (void)
{
	static GType eic_type = 0;

	if (!eic_type) {
		static const GTypeInfo eic_info =  {
			sizeof (EImageChooserClass),
			NULL,           /* base_init */
			NULL,           /* base_finalize */
			(GClassInitFunc) e_image_chooser_class_init,
			NULL,           /* class_finalize */
			NULL,           /* class_data */
			sizeof (EImageChooser),
			0,             /* n_preallocs */
			(GInstanceInitFunc) e_image_chooser_init,
		};

		eic_type = g_type_register_static (PARENT_TYPE, "EImageChooser", &eic_info, 0);
	}

	return eic_type;
}


static void
e_image_chooser_class_init (EImageChooserClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_ref (PARENT_TYPE);

	image_chooser_signals [CHANGED] =
		g_signal_new ("changed",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (EImageChooserClass, changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      GTK_TYPE_NONE, 0);

	object_class->dispose = e_image_chooser_dispose;
}

static void
e_image_chooser_init (EImageChooser *chooser)
{
	EImageChooserPrivate *priv;

	priv = chooser->priv = g_new0 (EImageChooserPrivate, 1);

	priv->image = gtk_image_new ();

	gtk_box_set_homogeneous (GTK_BOX (chooser), FALSE);
	gtk_box_pack_start (GTK_BOX (chooser), priv->image, TRUE, TRUE, 0);

	gtk_drag_dest_set (priv->image, 0, image_drag_types, num_image_drag_types, GDK_ACTION_COPY);
	g_signal_connect (priv->image,
			  "drag_motion", G_CALLBACK (image_drag_motion_cb), chooser);
	g_signal_connect (priv->image,
			  "drag_drop", G_CALLBACK (image_drag_drop_cb), chooser);
	g_signal_connect (priv->image,
			  "drag_data_received", G_CALLBACK (image_drag_data_received_cb), chooser);

	gtk_widget_show_all (priv->image);

	/* we default to being editable */
	priv->editable = TRUE;
}

static void
e_image_chooser_dispose (GObject *object)
{
	EImageChooser *eic = E_IMAGE_CHOOSER (object);

	if (eic->priv) {
		EImageChooserPrivate *priv = eic->priv;

		if (priv->image_buf) {
			g_free (priv->image_buf);
			priv->image_buf = NULL;
		}

		g_free (eic->priv);
		eic->priv = NULL;
	}

	if (G_OBJECT_CLASS (parent_class)->dispose)
		(* G_OBJECT_CLASS (parent_class)->dispose) (object);
}


static gboolean
set_image_from_data (EImageChooser *chooser,
		     char *data, int length)
{
	gboolean rv = FALSE;
	GdkPixbufLoader *loader = gdk_pixbuf_loader_new ();
	GdkPixbuf *pixbuf;

	gdk_pixbuf_loader_write (loader, data, length, NULL);
	gdk_pixbuf_loader_close (loader, NULL);

	pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
	if (pixbuf)
		g_object_ref (pixbuf);
	g_object_unref (loader);

	if (pixbuf) {
		GdkPixbuf *scaled;
		GtkRequisition chooser_size;

		float scale;
		int new_height, new_width;

		gtk_widget_size_request (gtk_widget_get_parent (GTK_WIDGET (chooser)),
		                         &chooser_size);
		chooser_size.width -= 5;
		chooser_size.height -= 5;

		new_height = gdk_pixbuf_get_height (pixbuf);
		new_width = gdk_pixbuf_get_width (pixbuf);

		if (chooser->priv->image_height == 0
		    && chooser->priv->image_width == 0) {
			scale = 1.0;
		}
		else if (chooser->priv->image_height < new_height
			 || chooser->priv->image_width < new_width) {
			/* we need to scale down */
			if (new_height > new_width)
				scale = (float)chooser_size.height / new_height;
			else
				scale = (float)chooser_size.width / new_width;
		}
		else {
			/* we need to scale up */
			if (new_height > new_width)
				scale = (float)new_height / chooser_size.height;
			else
				scale = (float)new_width / chooser_size.width;
		}

		if (scale == 1.0) {
			gtk_image_set_from_pixbuf (GTK_IMAGE (chooser->priv->image), pixbuf);

			chooser->priv->image_width = new_width;
			chooser->priv->image_height = new_height;
		}
		else {
			new_width *= scale;
			new_height *= scale;
			new_width = MIN (new_width, chooser_size.width);
			new_height = MIN (new_height, chooser_size.height);

			scaled = gdk_pixbuf_scale_simple (pixbuf,
							  new_width, new_height,
							  GDK_INTERP_BILINEAR);

			gtk_image_set_from_pixbuf (GTK_IMAGE (chooser->priv->image), scaled);
			g_object_unref (scaled);
		}

		g_object_unref (pixbuf);

		g_free (chooser->priv->image_buf);
		chooser->priv->image_buf = data;
		chooser->priv->image_buf_size = length;

		g_signal_emit (chooser,
			       image_chooser_signals [CHANGED], 0);

		rv = TRUE;
	}

	return rv;
}

static gboolean
image_drag_motion_cb (GtkWidget *widget,
		      GdkDragContext *context,
		      gint x, gint y, guint time, EImageChooser *chooser)
{
	GList *p;

	if (!chooser->priv->editable)
		return FALSE;

	for (p = context->targets; p != NULL; p = p->next) {
		char *possible_type;

		possible_type = gdk_atom_name (GDK_POINTER_TO_ATOM (p->data));
		if (!strcmp (possible_type, URI_LIST_TYPE)) {
			g_free (possible_type);
			gdk_drag_status (context, GDK_ACTION_COPY, time);
			return TRUE;
		}

		g_free (possible_type);
	}

	return FALSE;
}

static gboolean
image_drag_drop_cb (GtkWidget *widget,
		    GdkDragContext *context,
		    gint x, gint y, guint time, EImageChooser *chooser)
{
	GList *p;

	if (!chooser->priv->editable)
		return FALSE;

	if (context->targets == NULL) {
		return FALSE;
	}

	for (p = context->targets; p != NULL; p = p->next) {
		char *possible_type;

		possible_type = gdk_atom_name (GDK_POINTER_TO_ATOM (p->data));
		if (!strcmp (possible_type, URI_LIST_TYPE)) {
			g_free (possible_type);
			gtk_drag_get_data (widget, context,
					   GDK_POINTER_TO_ATOM (p->data),
					   time);
			return TRUE;
		}

		g_free (possible_type);
	}

	return FALSE;
}

static void
image_drag_data_received_cb (GtkWidget *widget,
			     GdkDragContext *context,
			     gint x, gint y,
			     GtkSelectionData *selection_data,
			     guint info, guint time, EImageChooser *chooser)
{
	char *target_type;
	gboolean handled = FALSE;

	target_type = gdk_atom_name (gtk_selection_data_get_target (selection_data));

	if (!strcmp (target_type, URI_LIST_TYPE)) {
		const char *data = gtk_selection_data_get_data (selection_data);
		char *uri;
		GFile *file;
		GInputStream *istream;
		char *nl = strstr (data, "\r\n");

		if (nl)
			uri = g_strndup (data, nl - (char *) data);
		else
			uri = g_strdup (data);

		file = g_file_new_for_uri (uri);
		istream = G_INPUT_STREAM (g_file_read (file, NULL, NULL));

		if (istream != NULL) {
			GFileInfo *info;

			info = g_file_query_info (file,
						  G_FILE_ATTRIBUTE_STANDARD_SIZE,
						  G_FILE_QUERY_INFO_NONE,
						  NULL, NULL);

			if (info != NULL) {
				gsize size;
				gboolean success;
				gchar *buf;

				size = g_file_info_get_size (info);
				g_object_unref (info);

				buf = g_malloc (size);

				success = g_input_stream_read_all (istream,
								   buf,
								   size,
								   &size,
								   NULL,
								   NULL);
				g_input_stream_close (istream, NULL, NULL);

				if (success &&
						set_image_from_data (chooser, buf, size))
					handled = TRUE;
				else
					g_free (buf);
			}

			g_object_unref (istream);
		}

		g_object_unref (file);
		g_free (uri);
	}

	gtk_drag_finish (context, handled, FALSE, time);
}

gboolean
e_image_chooser_set_from_file (EImageChooser *chooser, const char *filename)
{
	gchar *data;
	gsize data_length;

	g_return_val_if_fail (E_IS_IMAGE_CHOOSER (chooser), FALSE);
	g_return_val_if_fail (filename, FALSE);

	if (!g_file_get_contents (filename, &data, &data_length, NULL)) {
		return FALSE;
	}

	if (!set_image_from_data (chooser, data, data_length))
		g_free (data);

	return TRUE;
}

void
e_image_chooser_set_editable (EImageChooser *chooser, gboolean editable)
{
	g_return_if_fail (E_IS_IMAGE_CHOOSER (chooser));

	chooser->priv->editable = editable;

	gtk_widget_set_sensitive (chooser->priv->browse_button, editable);
}

gboolean
e_image_chooser_get_image_data (EImageChooser *chooser, char **data, gsize *data_length)
{
	g_return_val_if_fail (E_IS_IMAGE_CHOOSER (chooser), FALSE);
	g_return_val_if_fail (data != NULL, FALSE);
	g_return_val_if_fail (data_length != NULL, FALSE);

	*data_length = chooser->priv->image_buf_size;
	*data = g_malloc (*data_length);
	memcpy (*data, chooser->priv->image_buf, *data_length);

	return TRUE;
}

gboolean
e_image_chooser_set_image_data (EImageChooser *chooser, char *data, gsize data_length)
{
	char *buf;

	g_return_val_if_fail (E_IS_IMAGE_CHOOSER (chooser), FALSE);
	g_return_val_if_fail (data != NULL, FALSE);

	/* yuck, a copy... */
	buf = g_malloc (data_length);
	memcpy (buf, data, data_length);

	if (!set_image_from_data (chooser, buf, data_length)) {
		g_free (buf);
		return FALSE;
	}

	return TRUE;
}
