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
/*
  @NOTATION@
 */

#ifndef MATE_VFS_UTIL_H
#define MATE_VFS_UTIL_H



#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libmatevfs/mate-vfs-result.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =======================================================================
 * gdk-pixbuf handing stuff.
 *
 * Shamelessly stolen from caja-gdk-pixbuf-extensions.h which is
 * Copyright (C) 2000 Eazel, Inc.
 * Authors: Darin Adler <darin@eazel.com>
 *
 * =======================================================================
 */

typedef struct MateGdkPixbufAsyncHandle    MateGdkPixbufAsyncHandle;
typedef void (*MateGdkPixbufLoadCallback) (MateGdkPixbufAsyncHandle *handle,
                                            MateVFSResult             error,
                                            GdkPixbuf                 *pixbuf,
                                            gpointer                   cb_data);
typedef void (*MateGdkPixbufDoneCallback) (MateGdkPixbufAsyncHandle *handle,
                                            gpointer                   cb_data);

/* Loading a GdkPixbuf with a URI. */
GdkPixbuf *
mate_gdk_pixbuf_new_from_uri        (const char                 *uri);

/* Loading a GdkPixbuf with a URI. The image will be scaled to fit in the
 * requested size. */
GdkPixbuf *
mate_gdk_pixbuf_new_from_uri_at_scale (const char                 *uri,
                                       gint                        width,
                                       gint                        height,
                                       gboolean                    preserve_aspect_ratio);

/* Same thing async. */
MateGdkPixbufAsyncHandle *
mate_gdk_pixbuf_new_from_uri_async  (const char                 *uri,
                                      MateGdkPixbufLoadCallback  load_callback,
                                      MateGdkPixbufDoneCallback  done_callback,
                                      gpointer                    callback_data);

void
mate_gdk_pixbuf_new_from_uri_cancel (MateGdkPixbufAsyncHandle  *handle);

#ifdef __cplusplus
}
#endif

#endif
