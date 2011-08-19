/*  -*- Mode: C; c-set-style: linux; indent-tabs-mode: nil; c-basic-offset: 8 -*-

   Copyright (C) 1999 Red Hat, Inc.
   All rights reserved.
    
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
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   MatePixmap Developers: Havoc Pennington, Jonathan Blandford
*/
/*
  @NOTATION@
*/

#include <config.h>
#include <libmate/mate-macros.h>

#ifndef MATE_DISABLE_DEPRECATED_SOURCE

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "mate-pixmap.h"


static void mate_pixmap_class_init    (MatePixmapClass *class);
static void mate_pixmap_instance_init (MatePixmap      *gpixmap);

/* Not used currently */
struct _MatePixmapPrivate {
	int dummy;
};

/**
 * mate_pixmap_get_type:
 *
 * Registers the &MatePixmap class if necessary, and returns the type ID
 * associated to it.
 *
 * Returns: the type ID of the &MatePixmap class.
 */
MATE_CLASS_BOILERPLATE (MatePixmap, mate_pixmap,
			 GtkImage, GTK_TYPE_IMAGE)

/*
 * Widget functions
 */

static void
mate_pixmap_instance_init (MatePixmap *gpixmap)
{
}

static void
mate_pixmap_class_init (MatePixmapClass *class)
{
}



/*
 * Public functions
 */


/**
 * mate_pixmap_new_from_file:
 * @filename: The filename of the file to be loaded.
 *
 * Note that the new_from_file functions give you no way to detect errors;
 * if the file isn't found/loaded, you get an empty widget.
 * to detect errors just do:
 *
 * <programlisting>
 * pixbuf = gdk_pixbuf_new_from_file (filename);
 * if (pixbuf != NULL) {
 *         gpixmap = gtk_image_new_from_pixbuf (pixbuf);
 * } else {
 *         // handle your error...
 * }
 * </programlisting>
 * 
 * Return value: A newly allocated @MatePixmap with the file at @filename loaded.
 **/
GtkWidget*
mate_pixmap_new_from_file (const char *filename)
{
	GtkWidget *retval = g_object_new (MATE_TYPE_PIXMAP, NULL);
	gtk_image_set_from_file (GTK_IMAGE (retval), filename);
	return retval;
}

/**
 * mate_pixmap_new_from_file_at_size:
 * @filename: The filename of the file to be loaded.
 * @width: The width to scale the image to.
 * @height: The height to scale the image to.
 *
 * Loads a new @MatePixmap from a file, and scales it (if necessary) to the
 * size indicated by @height and @width.  If either are set to -1, then the
 * "natural" dimension of the image is used in place.  See
 * @mate_pixmap_new_from_file for information on error handling.
 *
 * Return value: value: A newly allocated @MatePixmap with the file at @filename loaded.
 **/
GtkWidget*
mate_pixmap_new_from_file_at_size (const gchar *filename, gint width, gint height)
{
	GtkWidget *retval = NULL;
	GdkPixbuf *pixbuf;
        GError *error = NULL;

	g_return_val_if_fail (filename != NULL, NULL);

	pixbuf = gdk_pixbuf_new_from_file (filename, &error);
        if (error != NULL) {
                g_warning (G_STRLOC ": cannot open %s: %s",
                           filename, error->message);
                g_error_free (error);
        }
	if (pixbuf != NULL) {
		GdkPixbuf *scaled = gdk_pixbuf_scale_simple (pixbuf,
							     width,
							     height,
							     GDK_INTERP_BILINEAR);
		retval = g_object_new (MATE_TYPE_PIXMAP, NULL);
		gtk_image_set_from_pixbuf (GTK_IMAGE (retval), scaled);
		g_object_unref (G_OBJECT (scaled));
		g_object_unref (G_OBJECT (pixbuf));
	} else {
		retval = g_object_new (MATE_TYPE_PIXMAP, NULL);
	}
	return retval;
}

/**
 * mate_pixmap_new_from_xpm_d:
 * @xpm_data: The xpm data to be loaded.
 *
 * Loads a new @MatePixmap from the @xpm_data, and scales it (if necessary) to
 * the size indicated by @height and @width.  If either are set to -1, then the
 * "natural" dimension of the image is used in place.
 *
 * Return value: value: A newly allocated @MatePixmap with the image from @xpm_data loaded.
 **/
GtkWidget*
mate_pixmap_new_from_xpm_d (const char **xpm_data)
{
	GtkWidget *retval = NULL;
	GdkPixbuf *pixbuf;

	g_return_val_if_fail (xpm_data != NULL, NULL);

	pixbuf = gdk_pixbuf_new_from_xpm_data (xpm_data);
	if (pixbuf != NULL) {
		retval = g_object_new (MATE_TYPE_PIXMAP, NULL);
		gtk_image_set_from_pixbuf (GTK_IMAGE (retval), pixbuf);
		g_object_unref (G_OBJECT (pixbuf));
	} else {
		retval = g_object_new (MATE_TYPE_PIXMAP, NULL);
	}
	return retval;
}

/**
 * mate_pixmap_new_from_xpm_d_at_size:
 * @xpm_data: The xpm data to be loaded.
 * @width: The width to scale the image to.
 * @height: The height to scale the image to.
 *
 * Loads a new @MatePixmap from the @xpm_data, and scales it (if necessary) to
 * the size indicated by @height and @width.  If either are set to -1, then the
 * "natural" dimension of the image is used in place.
 *
 * Return value: value: A newly allocated @MatePixmap with the image from @xpm_data loaded.
 **/
GtkWidget*
mate_pixmap_new_from_xpm_d_at_size (const char **xpm_data, int width, int height)
{
	GtkWidget *retval = NULL;
	GdkPixbuf *pixbuf;

	g_return_val_if_fail (xpm_data != NULL, NULL);

	pixbuf = gdk_pixbuf_new_from_xpm_data (xpm_data);
	if (pixbuf != NULL) {
		GdkPixbuf *scaled = gdk_pixbuf_scale_simple (pixbuf,
							     width,
							     height,
							     GDK_INTERP_BILINEAR);
		retval = g_object_new (MATE_TYPE_PIXMAP, NULL);
		gtk_image_set_from_pixbuf (GTK_IMAGE (retval), scaled);
		g_object_unref (G_OBJECT (scaled));
		g_object_unref (G_OBJECT (pixbuf));
	} else {
		retval = g_object_new (MATE_TYPE_PIXMAP, NULL);
	}
	return retval;
}


GtkWidget *
mate_pixmap_new_from_mate_pixmap (MatePixmap *gpixmap)
{
	GtkWidget *retval = NULL;
	GdkPixbuf *pixbuf;

	g_return_val_if_fail (gpixmap != NULL, NULL);
	g_return_val_if_fail (MATE_IS_PIXMAP (gpixmap), NULL);

	pixbuf = gtk_image_get_pixbuf (GTK_IMAGE (gpixmap));
	if (pixbuf != NULL) {
		retval = g_object_new (MATE_TYPE_PIXMAP, NULL);
		gtk_image_set_from_pixbuf (GTK_IMAGE (retval), pixbuf);
	} else {
		retval = g_object_new (MATE_TYPE_PIXMAP, NULL);
	}
	return retval;
}

void
mate_pixmap_load_file (MatePixmap *gpixmap, const char *filename)
{
	g_return_if_fail (gpixmap != NULL);
	g_return_if_fail (MATE_IS_PIXMAP (gpixmap));

	gtk_image_set_from_file (GTK_IMAGE (gpixmap), filename);
}

void
mate_pixmap_load_file_at_size (MatePixmap *gpixmap,
				const char *filename,
				int width, int height)
{
	GdkPixbuf *pixbuf;
        GError *error = NULL;

	g_return_if_fail (gpixmap != NULL);
	g_return_if_fail (MATE_IS_PIXMAP (gpixmap));
	g_return_if_fail (filename != NULL);

	pixbuf = gdk_pixbuf_new_from_file (filename, &error);
        if (error != NULL) {
                g_warning (G_STRLOC ": cannot open %s: %s",
                           filename, error->message);
                g_error_free (error);
        }
	if (pixbuf != NULL) {
		GdkPixbuf *scaled = gdk_pixbuf_scale_simple (pixbuf,
							     width,
							     height,
							     GDK_INTERP_BILINEAR);
		gtk_image_set_from_pixbuf (GTK_IMAGE (gpixmap), scaled);
		g_object_unref (G_OBJECT (scaled));
		g_object_unref (G_OBJECT (pixbuf));
	} else {
		gtk_image_set_from_file (GTK_IMAGE (gpixmap), NULL);
	}
}

void
mate_pixmap_load_xpm_d (MatePixmap *gpixmap,
			 const char **xpm_data)
{
	GdkPixbuf *pixbuf;

	g_return_if_fail (gpixmap != NULL);
	g_return_if_fail (MATE_IS_PIXMAP (gpixmap));

	pixbuf = gdk_pixbuf_new_from_xpm_data (xpm_data);
	if (pixbuf != NULL) {
		gtk_image_set_from_pixbuf (GTK_IMAGE (gpixmap), pixbuf);
		g_object_unref (G_OBJECT (pixbuf));
	} else {
		gtk_image_set_from_file (GTK_IMAGE (gpixmap), NULL);
	}
}

void
mate_pixmap_load_xpm_d_at_size (MatePixmap *gpixmap,
				 const char **xpm_data,
				 int width, int height)
{
	GdkPixbuf *pixbuf;

	g_return_if_fail (gpixmap != NULL);
	g_return_if_fail (MATE_IS_PIXMAP (gpixmap));

	pixbuf = gdk_pixbuf_new_from_xpm_data (xpm_data);
	if (pixbuf != NULL) {
		GdkPixbuf *scaled = gdk_pixbuf_scale_simple (pixbuf,
							     width,
							     height,
							     GDK_INTERP_BILINEAR);
		gtk_image_set_from_pixbuf (GTK_IMAGE (gpixmap), scaled);
		g_object_unref (G_OBJECT (scaled));
		g_object_unref (G_OBJECT (pixbuf));
	} else {
		gtk_image_set_from_file (GTK_IMAGE (gpixmap), NULL);
	}
}

#endif /* MATE_DISABLE_DEPRECATED_SOURCE */
