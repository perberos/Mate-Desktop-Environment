/* -*- mode: C; c-basic-offset: 4 -*-
 * fontilus - a collection of font utilities for MATE
 * Copyright (C) 2002-2003  James Henstridge <james@daa.com.au>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <gio/gio.h>

static unsigned long
vfs_stream_read(FT_Stream stream,
		unsigned long offset,
		unsigned char *buffer,
		unsigned long count)
{
    GFileInputStream *handle = (GFileInputStream *)stream->descriptor.pointer;
    gssize bytes_read = 0;

    if (! g_seekable_seek (G_SEEKABLE (handle), offset, G_SEEK_SET, NULL, NULL))
        return 0;

    if (count > 0) {
        bytes_read = g_input_stream_read (G_INPUT_STREAM (handle), buffer, count, NULL, NULL);

        if (bytes_read == -1)
            return 0;
    }

    return bytes_read;
}

static void
vfs_stream_close(FT_Stream stream)
{
    GFileInputStream *handle = (GFileInputStream *)stream->descriptor.pointer;

    if (! handle)
        return;

    g_object_unref (handle);

    stream->descriptor.pointer = NULL;
    stream->size               = 0;
    stream->base               = NULL;
}

static FT_Error
vfs_stream_open(FT_Stream stream,
		const char *uri)
{
    GFile *file;
    GError *error = NULL;
    GFileInfo *info;
    GFileInputStream *handle;

    if (!stream)
        return FT_Err_Invalid_Stream_Handle;

    file = g_file_new_for_uri (uri);

    handle = g_file_read (file, NULL, &error);
    if (! handle) {
        g_message ("%s", error->message);
	g_object_unref (file);

        g_error_free (error);
        return FT_Err_Cannot_Open_Resource;
    }

    info = g_file_query_info (file, G_FILE_ATTRIBUTE_STANDARD_SIZE,
                              G_FILE_QUERY_INFO_NONE, NULL, &error);
    g_object_unref (file);

    if (! info) {
        g_warning ("%s", error->message);

        g_error_free (error);
        return FT_Err_Cannot_Open_Resource;
    }

    stream->size = g_file_info_get_size (info);

    g_object_unref (info);

    stream->descriptor.pointer = handle;
    stream->pathname.pointer   = NULL;
    stream->pos                = 0;

    stream->read  = vfs_stream_read;
    stream->close = vfs_stream_close;

    return FT_Err_Ok;
}

/* load a typeface from a URI */
FT_Error
FT_New_Face_From_URI(FT_Library           library,
		     const gchar*         uri,
		     FT_Long              face_index,
		     FT_Face             *aface)
{
    FT_Open_Args args;
    FT_Stream stream;
    FT_Error error;

    if ((stream = calloc(1, sizeof(*stream))) == NULL)
	return FT_Err_Out_Of_Memory;

    error = vfs_stream_open(stream, uri);
    if (error != FT_Err_Ok) {
	free(stream);
	return error;
    }

    /* freetype-2.1.3 accidentally broke compatibility. */
#if defined(FT_OPEN_STREAM) && !defined(ft_open_stream)
#  define ft_open_stream FT_OPEN_STREAM
#endif
    args.flags  = ft_open_stream;
    args.stream = stream;

    error = FT_Open_Face(library, &args, face_index, aface);

    if (error != FT_Err_Ok) {
	if (stream->close) stream->close(stream);
	free(stream);
	return error;
    }

    /* so that freetype will free the stream */
    (*aface)->face_flags &= ~FT_FACE_FLAG_EXTERNAL_STREAM;

    return error;
}
