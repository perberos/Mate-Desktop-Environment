/* -*- Mode: C; c-set-style: gnu indent-tabs-mode: t; c-basic-offset: 4; tab-width: 8 -*- */
/*
 * Copyright (C) 2000 SuSE GmbH
 * Author: Martin Baulig <baulig@suse.de>
 *
 * This file is part of the Mate Library.
 *
 * The Mate Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Mate Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Mate Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <glib-object.h>
#include <gio/gio.h>

#include "mate-vfs-util.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libmatevfs/mate-vfs-async-ops.h>
#include <libmatevfs/mate-vfs-ops.h>
#include <libmatevfs/mate-vfs-utils.h>

/* =======================================================================
 * gdk-pixbuf handing stuff.
 *
 * Shamelessly stolen from caja-gdk-pixbuf-extensions.c which is
 * Copyright (C) 2000 Eazel, Inc.
 * Authors: Darin Adler <darin@eazel.com>
            Ramiro Estrugo <ramiro@eazel.com>
 *
 * =======================================================================
 */

#define LOAD_BUFFER_SIZE 4096

struct MateGdkPixbufAsyncHandle {
    GFile *file;
    GFileInputStream *file_input_stream;
    GCancellable *cancellable;

    MateGdkPixbufLoadCallback load_callback;
    MateGdkPixbufDoneCallback done_callback;
    gpointer callback_data;
    GdkPixbufLoader *loader;
    char buffer[LOAD_BUFFER_SIZE];
};

typedef struct {
    gint width;
    gint height;
    gint input_width;
    gint input_height;
    gboolean preserve_aspect_ratio;
} SizePrepareContext;


static void input_stream_read_callback (GObject *source_object,
					GAsyncResult *res,
					gpointer user_data);
static void input_stream_ready_callback (GObject *source_object,
					 GAsyncResult *res,
					 gpointer user_data);
static void load_done            (MateGdkPixbufAsyncHandle *handle,
                                  MateVFSResult            result,
                                  GdkPixbuf                *pixbuf);

/**
 * mate_gdk_pixbuf_new_from_uri:
 * @uri: the uri of an image
 * 
 * Loads a GdkPixbuf from the image file @uri points to
 * 
 * Return value: The pixbuf, or NULL on error
 **/
GdkPixbuf *
mate_gdk_pixbuf_new_from_uri (const char *uri)
{
	return mate_gdk_pixbuf_new_from_uri_at_scale(uri, -1, -1, TRUE);
}

static void
size_prepared_cb (GdkPixbufLoader *loader, 
		  int              width,
		  int              height,
		  gpointer         data)
{
	SizePrepareContext *info = data;

	g_return_if_fail (width > 0 && height > 0);

	info->input_width = width;
	info->input_height = height;
	
	if (width < info->width && height < info->height) return;
	
	if (info->preserve_aspect_ratio && 
	    (info->width > 0 || info->height > 0)) {
		if (info->width < 0)
		{
			width = width * (double)info->height/(double)height;
			height = info->height;
		}
		else if (info->height < 0)
		{
			height = height * (double)info->width/(double)width;
			width = info->width;
		}
		else if ((double)height * (double)info->width >
			 (double)width * (double)info->height) {
			width = 0.5 + (double)width * (double)info->height / (double)height;
			height = info->height;
		} else {
			height = 0.5 + (double)height * (double)info->width / (double)width;
			width = info->width;
		}
	} else {
		if (info->width > 0)
			width = info->width;
		if (info->height > 0)
			height = info->height;
	}
	
	gdk_pixbuf_loader_set_size (loader, width, height);
}

/**
 * mate_gdk_pixbuf_new_from_uri:
 * @uri: the uri of an image
 * @width: The width the image should have or -1 to not constrain the width
 * @height: The height the image should have or -1 to not constrain the height
 * @preserve_aspect_ratio: %TRUE to preserve the image's aspect ratio
 * 
 * Loads a GdkPixbuf from the image file @uri points to, scaling it to the
 * desired size. If you pass -1 for @width or @height then the value
 * specified in the file will be used.
 *
 * When preserving aspect ratio, if both height and width are set the size
 * is picked such that the scaled image fits in a width * height rectangle.
 * 
 * Return value: The loaded pixbuf, or NULL on error
 *
 * Since: 2.14
 **/
GdkPixbuf *
mate_gdk_pixbuf_new_from_uri_at_scale (const char *uri,
					gint        width,
					gint        height,
					gboolean    preserve_aspect_ratio)
{
    MateVFSResult result;
    char buffer[LOAD_BUFFER_SIZE];
    MateVFSFileSize bytes_read;
    GdkPixbufLoader *loader;
    GdkPixbuf *pixbuf;	
    GdkPixbufAnimation *animation;
    GdkPixbufAnimationIter *iter;
    gboolean has_frame;
    SizePrepareContext info;
    GFile *file;
    GFileInputStream *file_input_stream;

    g_return_val_if_fail (uri != NULL, NULL);

    file = g_file_new_for_uri (uri);
    file_input_stream = g_file_read (file, NULL, NULL);
    if (file_input_stream == NULL) {
	g_object_unref (file);
	return NULL;
    }

    loader = gdk_pixbuf_loader_new ();
    if (1 <= width || 1 <= height) {
        info.width = width;
        info.height = height;
	info.input_width = info.input_height = 0;
        info.preserve_aspect_ratio = preserve_aspect_ratio;        
        g_signal_connect (loader, "size-prepared", G_CALLBACK (size_prepared_cb), &info);
    }

    has_frame = FALSE;

    result = MATE_VFS_ERROR_GENERIC;
    while (!has_frame) {

	bytes_read = g_input_stream_read (G_INPUT_STREAM (file_input_stream),
					  buffer,
					  sizeof (buffer),
					  NULL,
					  NULL);
	if (bytes_read == -1) {
	    break;
	}
	result = MATE_VFS_OK;
	if (bytes_read == 0) {
	    break;
	}

	if (!gdk_pixbuf_loader_write (loader,
				      (unsigned char *)buffer,
				      bytes_read,
				      NULL)) {
	    result = MATE_VFS_ERROR_WRONG_FORMAT;
	    break;
	}

	animation = gdk_pixbuf_loader_get_animation (loader);
	if (animation) {
		iter = gdk_pixbuf_animation_get_iter (animation, NULL);
		if (!gdk_pixbuf_animation_iter_on_currently_loading_frame (iter)) {
			has_frame = TRUE;
		}
		g_object_unref (iter);
	}
    }

    gdk_pixbuf_loader_close (loader, NULL);
    
    if (result != MATE_VFS_OK) {
	g_object_unref (G_OBJECT (loader));
	g_input_stream_close (G_INPUT_STREAM (file_input_stream), NULL, NULL);
	g_object_unref (file_input_stream);
	g_object_unref (file);
	return NULL;
    }

    g_input_stream_close (G_INPUT_STREAM (file_input_stream), NULL, NULL);
    g_object_unref (file_input_stream);
    g_object_unref (file);

    pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
    if (pixbuf != NULL) {
	g_object_ref (G_OBJECT (pixbuf));
	g_object_set_data (G_OBJECT (pixbuf), "mate-original-width",
			   GINT_TO_POINTER (info.input_width));
	g_object_set_data (G_OBJECT (pixbuf), "mate-original-height",
			   GINT_TO_POINTER (info.input_height));
    }
    g_object_unref (G_OBJECT (loader));

    return pixbuf;
}

MateGdkPixbufAsyncHandle *
mate_gdk_pixbuf_new_from_uri_async (const char *uri,
				     MateGdkPixbufLoadCallback load_callback,
				     MateGdkPixbufDoneCallback done_callback,
				     gpointer callback_data)
{
    MateGdkPixbufAsyncHandle *handle;

    handle = g_new0 (MateGdkPixbufAsyncHandle, 1);
    handle->load_callback = load_callback;
    handle->done_callback = done_callback;
    handle->callback_data = callback_data;

    handle->file = g_file_new_for_uri (uri);
    handle->cancellable = g_cancellable_new ();
    g_file_read_async (handle->file, 
		       G_PRIORITY_DEFAULT, 
		       handle->cancellable, 
		       input_stream_ready_callback, 
		       handle);
    return handle;
}

static void 
input_stream_ready_callback (GObject *source_object,
			     GAsyncResult *res,
			     gpointer user_data)
{
    MateGdkPixbufAsyncHandle *handle = user_data;

    handle->file_input_stream = g_file_read_finish (G_FILE (source_object),
						    res, NULL);
    if (handle->file_input_stream == NULL) {
	/* TODO: could map the GError more precisely to the MateVFSError */
	load_done (handle, MATE_VFS_ERROR_GENERIC, NULL);
	return;
    }

    handle->loader = gdk_pixbuf_loader_new ();

    g_input_stream_read_async (G_INPUT_STREAM (handle->file_input_stream),
			       handle->buffer,
			       sizeof (handle->buffer),
			       G_PRIORITY_DEFAULT,
			       handle->cancellable,
			       input_stream_read_callback,
			       handle);
}

static void 
input_stream_read_callback (GObject *source_object,
			    GAsyncResult *res,
			    gpointer user_data)
{
    MateGdkPixbufAsyncHandle *handle = user_data;
    gssize bytes_read;
    MateVFSResult result;

    bytes_read = g_input_stream_read_finish (G_INPUT_STREAM (source_object),
					     res, NULL);
    if (bytes_read == -1) {
	/* TODO: could map the GError more precisely */
	result = MATE_VFS_ERROR_GENERIC;
    } else if (bytes_read > 0) {
	if (!gdk_pixbuf_loader_write (handle->loader,
				      (const guchar *) handle->buffer,
				      bytes_read,
				      NULL)) {
	    result = MATE_VFS_ERROR_WRONG_FORMAT;
	} else {
	    /* read more */
	    g_input_stream_read_async (G_INPUT_STREAM (handle->file_input_stream),
				       handle->buffer,
				       sizeof (handle->buffer),
				       G_PRIORITY_DEFAULT,
				       handle->cancellable,
				       input_stream_read_callback,
				       handle);
	    return;
	}
    } else {
	/* EOF */
	result = MATE_VFS_OK;
    }

    if (result == MATE_VFS_OK) {
	GdkPixbuf *pixbuf;
	pixbuf = gdk_pixbuf_loader_get_pixbuf (handle->loader);
	load_done (handle, result, pixbuf);
    } else {
	load_done (handle, result, NULL);
    }
}

static void
free_pixbuf_load_handle (MateGdkPixbufAsyncHandle *handle)
{
    if (handle->done_callback)
	(* handle->done_callback) (handle, handle->callback_data);
    if (handle->loader != NULL) {
	gdk_pixbuf_loader_close (handle->loader, NULL);
	g_object_unref (G_OBJECT (handle->loader));
    }
    if (handle->file_input_stream != NULL) {
        g_input_stream_close (G_INPUT_STREAM (handle->file_input_stream), NULL, NULL);
        g_object_unref (handle->file_input_stream);
    }
    if (handle->file != NULL) {
        g_object_unref (handle->file);
    }
    if (handle->cancellable != NULL) {
        g_object_unref (handle->cancellable);
    }

    g_free (handle);
}

static void
load_done (MateGdkPixbufAsyncHandle *handle,
	   MateVFSResult result,
	   GdkPixbuf *pixbuf)
{
    (* handle->load_callback) (handle, result, pixbuf, handle->callback_data);
    free_pixbuf_load_handle (handle);
}

void
mate_gdk_pixbuf_new_from_uri_cancel (MateGdkPixbufAsyncHandle *handle)
{
    if (handle == NULL) {
	return;
    }
    if (handle->cancellable != NULL) {
	g_cancellable_cancel (handle->cancellable);
    }
    free_pixbuf_load_handle (handle);
}
