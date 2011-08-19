/* Eye Of Mate - Thumbnailing functions
 *
 * Copyright (C) 2000-2008 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@mate.org>
 *
 * Based on eel code (eel/eel-graphic-effects.c) by:
 * 	- Andy Hertzfeld <andy@eazel.com>
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
#include <config.h>
#endif

/* We must define MATE_DESKTOP_USE_UNSTABLE_API to be able
   to use MateDesktopThumbnail */
#ifndef MATE_DESKTOP_USE_UNSTABLE_API
#define MATE_DESKTOP_USE_UNSTABLE_API
#endif
#include <libmateui/mate-desktop-thumbnail.h>

#include "eog-thumbnail.h"
#include "eog-list-store.h"
#include "eog-debug.h"

#define EOG_THUMB_ERROR eog_thumb_error_quark ()

static MateDesktopThumbnailFactory *factory = NULL;
static GdkPixbuf *frame = NULL;

typedef enum {
	EOG_THUMB_ERROR_VFS,
	EOG_THUMB_ERROR_GENERIC,
	EOG_THUMB_ERROR_UNKNOWN
} EogThumbError;

typedef struct {
	char    *uri_str;
	char    *thumb_path;
	time_t   mtime;
	char    *mime_type;
	gboolean thumb_exists;
	gboolean failed_thumb_exists;
	gboolean can_read;
} EogThumbData;

static GQuark
eog_thumb_error_quark (void)
{
	static GQuark q = 0;
	if (q == 0)
		q = g_quark_from_static_string ("eog-thumb-error-quark");

	return q;
}

static void
set_vfs_error (GError **error, GError *ioerror)
{
	g_set_error (error,
		     EOG_THUMB_ERROR,
		     EOG_THUMB_ERROR_VFS,
		     "%s", ioerror ? ioerror->message : "VFS error making a thumbnail");
}

static void
set_thumb_error (GError **error, int error_id, const char *string)
{
	g_set_error (error,
		     EOG_THUMB_ERROR,
		     error_id,
		     "%s", string);
}

static GdkPixbuf*
get_valid_thumbnail (EogThumbData *data, GError **error)
{
	GdkPixbuf *thumb = NULL;

	g_return_val_if_fail (data != NULL, NULL);

	/* does a thumbnail under the path exists? */
	if (data->thumb_exists) {
		thumb = gdk_pixbuf_new_from_file (data->thumb_path, error);

		/* is this thumbnail file up to date? */
		if (thumb != NULL && !mate_desktop_thumbnail_is_valid (thumb, data->uri_str, data->mtime)) {
			g_object_unref (thumb);
			thumb = NULL;
		}
	}

	return thumb;
}

static GdkPixbuf *
create_thumbnail_from_pixbuf (EogThumbData *data,
			      GdkPixbuf *pixbuf,
			      GError **error)
{
	GdkPixbuf *thumb;
	gint width, height;
	gfloat perc;

	g_assert (factory != NULL);

	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);

	perc = CLAMP (128.0/(MAX (width, height)), 0, 1);

	thumb = mate_desktop_thumbnail_scale_down_pixbuf (pixbuf,
							   width*perc,
							   height*perc);

	return thumb;
}

static void
eog_thumb_data_free (EogThumbData *data)
{
	if (data == NULL)
		return;

	g_free (data->thumb_path);
	g_free (data->mime_type);
	g_free (data->uri_str);

	g_slice_free (EogThumbData, data);
}

static EogThumbData*
eog_thumb_data_new (GFile *file, GError **error)
{
	EogThumbData *data;
	GFileInfo *file_info;
	GError *ioerror = NULL;

	g_return_val_if_fail (file != NULL, NULL);
	g_return_val_if_fail (error != NULL && *error == NULL, NULL);

	data = g_slice_new0 (EogThumbData);

	data->uri_str    = g_file_get_uri (file);
	data->thumb_path = mate_desktop_thumbnail_path_for_uri (data->uri_str, MATE_DESKTOP_THUMBNAIL_SIZE_NORMAL);

	file_info = g_file_query_info (file,
				       G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE ","
				       G_FILE_ATTRIBUTE_TIME_MODIFIED ","
				       G_FILE_ATTRIBUTE_THUMBNAIL_PATH ","
				       G_FILE_ATTRIBUTE_THUMBNAILING_FAILED ","
				       G_FILE_ATTRIBUTE_ACCESS_CAN_READ,
				       0, NULL, &ioerror);
	if (file_info == NULL)
	{
		set_vfs_error (error, ioerror);
		g_clear_error (&ioerror);
	}

	if (*error == NULL) {
		/* if available, copy data */
		data->mtime = g_file_info_get_attribute_uint64 (file_info,
								G_FILE_ATTRIBUTE_TIME_MODIFIED);
		data->mime_type = g_strdup (g_file_info_get_content_type (file_info));

		data->thumb_exists = (g_file_info_get_attribute_byte_string (file_info,
					                                     G_FILE_ATTRIBUTE_THUMBNAIL_PATH) != NULL);
		data->failed_thumb_exists = g_file_info_get_attribute_boolean (file_info,
									       G_FILE_ATTRIBUTE_THUMBNAILING_FAILED);
		data->can_read = TRUE;
		if (g_file_info_has_attribute (file_info, G_FILE_ATTRIBUTE_ACCESS_CAN_READ)) {
			data->can_read = g_file_info_get_attribute_boolean (file_info,
									    G_FILE_ATTRIBUTE_ACCESS_CAN_READ);
		}
	}
	else {
		eog_thumb_data_free (data);
		data = NULL;
		g_clear_error (&ioerror);
	}

	g_object_unref (file_info);

	return data;
}

static void
draw_frame_row (GdkPixbuf *frame_image,
		gint target_width,
		gint source_width,
		gint source_v_position,
		gint dest_v_position,
		GdkPixbuf *result_pixbuf,
		gint left_offset,
		gint height)
{
	gint remaining_width, h_offset, slab_width;

	remaining_width = target_width;
	h_offset = 0;

	while (remaining_width > 0) {
		slab_width = remaining_width > source_width ?
			     source_width : remaining_width;

		gdk_pixbuf_copy_area (frame_image,
				      left_offset,
				      source_v_position,
				      slab_width,
				      height,
				      result_pixbuf,
				      left_offset + h_offset,
				      dest_v_position);

		remaining_width -= slab_width;
		h_offset += slab_width;
	}
}

static void
draw_frame_column (GdkPixbuf *frame_image,
		   gint target_height,
		   gint source_height,
		   gint source_h_position,
		   gint dest_h_position,
		   GdkPixbuf *result_pixbuf,
		   gint top_offset,
		   gint width)
{
	gint remaining_height, v_offset, slab_height;

	remaining_height = target_height;
	v_offset = 0;

	while (remaining_height > 0) {
		slab_height = remaining_height > source_height ?
			      source_height : remaining_height;

		gdk_pixbuf_copy_area (frame_image,
				      source_h_position,
				      top_offset,
				      width,
				      slab_height,
				      result_pixbuf,
				      dest_h_position,
				      top_offset + v_offset);

		remaining_height -= slab_height;
		v_offset += slab_height;
	}
}

static GdkPixbuf *
eog_thumbnail_stretch_frame_image (GdkPixbuf *frame_image,
				   gint left_offset,
				   gint top_offset,
				   gint right_offset,
				   gint bottom_offset,
                                   gint dest_width,
				   gint dest_height,
				   gboolean fill_flag)
{
        GdkPixbuf *result_pixbuf;
        gint frame_width, frame_height;
        gint target_width, target_frame_width;
        gint target_height, target_frame_height;

        frame_width  = gdk_pixbuf_get_width  (frame_image);
        frame_height = gdk_pixbuf_get_height (frame_image);

        if (fill_flag) {
		result_pixbuf = gdk_pixbuf_scale_simple (frame_image,
							 dest_width,
							 dest_height,
							 GDK_INTERP_NEAREST);
        } else {
                result_pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
						TRUE,
						8,
						dest_width,
						dest_height);

		/* Clear pixbuf to fully opaque white */
		gdk_pixbuf_fill (result_pixbuf, 0xffffffff);
        }

        target_width  = dest_width - left_offset - right_offset;
        target_frame_width = frame_width - left_offset - right_offset;

        target_height  = dest_height - top_offset - bottom_offset;
        target_frame_height = frame_height - top_offset - bottom_offset;

        /* Draw the left top corner  and top row */
        gdk_pixbuf_copy_area (frame_image,
			      0, 0,
			      left_offset,
			      top_offset,
			      result_pixbuf,
			      0, 0);

        draw_frame_row (frame_image,
			target_width,
			target_frame_width,
			0, 0,
			result_pixbuf,
			left_offset,
			top_offset);

        /* Draw the right top corner and left column */
        gdk_pixbuf_copy_area (frame_image,
			      frame_width - right_offset,
			      0,
			      right_offset,
			      top_offset,
			      result_pixbuf,
			      dest_width - right_offset,
			      0);

        draw_frame_column (frame_image,
			   target_height,
			   target_frame_height,
			   0, 0,
			   result_pixbuf,
			   top_offset,
			   left_offset);

        /* Draw the bottom right corner and bottom row */
        gdk_pixbuf_copy_area (frame_image,
			      frame_width - right_offset,
			      frame_height - bottom_offset,
			      right_offset,
			      bottom_offset,
			      result_pixbuf,
			      dest_width - right_offset,
			      dest_height - bottom_offset);

        draw_frame_row (frame_image,
			target_width,
			target_frame_width,
			frame_height - bottom_offset,
			dest_height - bottom_offset,
			result_pixbuf,
			left_offset, bottom_offset);

        /* Draw the bottom left corner and the right column */
        gdk_pixbuf_copy_area (frame_image,
			      0,
			      frame_height - bottom_offset,
			      left_offset,
			      bottom_offset,
			      result_pixbuf,
			      0,
			      dest_height - bottom_offset);

        draw_frame_column (frame_image,
			   target_height,
			   target_frame_height,
			   frame_width - right_offset,
			   dest_width - right_offset,
			   result_pixbuf, top_offset,
			   right_offset);

        return result_pixbuf;
}

GdkPixbuf *
eog_thumbnail_add_frame (GdkPixbuf *thumbnail)
{
	GdkPixbuf *result_pixbuf;
	gint source_width, source_height;
	gint dest_width, dest_height;

	source_width  = gdk_pixbuf_get_width  (thumbnail);
	source_height = gdk_pixbuf_get_height (thumbnail);

	dest_width  = source_width  + 9;
	dest_height = source_height + 9;

	result_pixbuf = eog_thumbnail_stretch_frame_image (frame,
							   3, 3, 6, 6,
	                                	           dest_width,
							   dest_height,
							   FALSE);

	gdk_pixbuf_copy_area (thumbnail,
			      0, 0,
			      source_width,
			      source_height,
			      result_pixbuf,
			      3, 3);

	return result_pixbuf;
}

GdkPixbuf *
eog_thumbnail_fit_to_size (GdkPixbuf *thumbnail, gint dimension)
{
	gint width, height;

	width = gdk_pixbuf_get_width (thumbnail);
	height = gdk_pixbuf_get_height (thumbnail);

	if (width > dimension || height > dimension) {
		GdkPixbuf *result_pixbuf;
		gfloat factor;

		if (width > height) {
			factor = (gfloat) dimension / (gfloat) width;
		} else {
			factor = (gfloat) dimension / (gfloat) height;
		}

		width  = MAX (width  * factor, 1);
		height = MAX (height * factor, 1);

		result_pixbuf = mate_desktop_thumbnail_scale_down_pixbuf (thumbnail, width, height);

		return result_pixbuf;
	}
	return gdk_pixbuf_copy (thumbnail);
}

GdkPixbuf*
eog_thumbnail_load (EogImage *image, GError **error)
{
	GdkPixbuf *thumb = NULL;
	GFile *file;
	EogThumbData *data;
	GdkPixbuf *pixbuf;

	g_return_val_if_fail (image != NULL, NULL);
	g_return_val_if_fail (error != NULL && *error == NULL, NULL);

	file = eog_image_get_file (image);
	data = eog_thumb_data_new (file, error);
	g_object_unref (file);

	if (data == NULL)
		return NULL;

	if (!data->can_read ||
	    (data->failed_thumb_exists && mate_desktop_thumbnail_factory_has_valid_failed_thumbnail (factory, data->uri_str, data->mtime))) {
		eog_debug_message (DEBUG_THUMBNAIL, "%s: bad permissions or valid failed thumbnail present",data->uri_str);
		set_thumb_error (error, EOG_THUMB_ERROR_GENERIC, "Thumbnail creation failed");
		return NULL;
	}

	/* check if there is already a valid cached thumbnail */
	thumb = get_valid_thumbnail (data, error);

	if (thumb != NULL) {
		eog_debug_message (DEBUG_THUMBNAIL, "%s: loaded from cache",data->uri_str);
	} else if (mate_desktop_thumbnail_factory_can_thumbnail (factory, data->uri_str, data->mime_type, data->mtime)) {
		pixbuf = eog_image_get_pixbuf (image);

		if (pixbuf != NULL) {
			/* generate a thumbnail from the in-memory image,
			   if we have already loaded the image */
			eog_debug_message (DEBUG_THUMBNAIL, "%s: creating from pixbuf",data->uri_str);
			thumb = create_thumbnail_from_pixbuf (data, pixbuf, error);
			g_object_unref (pixbuf);
		} else {
			/* generate a thumbnail from the file */
			eog_debug_message (DEBUG_THUMBNAIL, "%s: creating from file",data->uri_str);
			thumb = mate_desktop_thumbnail_factory_generate_thumbnail (factory, data->uri_str, data->mime_type);
		}

		if (thumb != NULL) {
			/* Save the new thumbnail */
			mate_desktop_thumbnail_factory_save_thumbnail (factory, thumb, data->uri_str, data->mtime);
			eog_debug_message (DEBUG_THUMBNAIL, "%s: normal thumbnail saved",data->uri_str);
		} else {
			/* Save a failed thumbnail, to stop further thumbnail attempts */
			mate_desktop_thumbnail_factory_create_failed_thumbnail (factory, data->uri_str, data->mtime);
			eog_debug_message (DEBUG_THUMBNAIL, "%s: failed thumbnail saved",data->uri_str);
			set_thumb_error (error, EOG_THUMB_ERROR_GENERIC, "Thumbnail creation failed");
		}
	}

	eog_thumb_data_free (data);

	return thumb;
}

void
eog_thumbnail_init (void)
{
	if (factory == NULL) {
		factory = mate_desktop_thumbnail_factory_new (MATE_DESKTOP_THUMBNAIL_SIZE_NORMAL);
	}

	if (frame == NULL) {
		frame = gdk_pixbuf_new_from_file (EOG_DATA_DIR "/pixmaps/thumbnail-frame.png", NULL);
	}
}
