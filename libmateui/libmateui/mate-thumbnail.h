/*
 * mate-thumbnail.h: Utilities for handling thumbnails
 *
 * Copyright (C) 2002 Red Hat, Inc.
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
 *
 * Author: Alexander Larsson <alexl@redhat.com>
 */

#ifndef MATE_THUMBNAIL_H
#define MATE_THUMBNAIL_H

#include <glib.h>
#include <glib-object.h>
#include <time.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  MATE_THUMBNAIL_SIZE_NORMAL,
  MATE_THUMBNAIL_SIZE_LARGE
} MateThumbnailSize;

#define MATE_TYPE_THUMBNAIL_FACTORY	(mate_thumbnail_factory_get_type ())
#define MATE_THUMBNAIL_FACTORY(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_THUMBNAIL_FACTORY, MateThumbnailFactory))
#define MATE_THUMBNAIL_FACTORY_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_THUMBNAIL_FACTORY, MateThumbnailFactoryClass))
#define MATE_IS_THUMBNAIL_FACTORY(obj)		(G_TYPE_INSTANCE_CHECK_TYPE ((obj), MATE_TYPE_THUMBNAIL_FACTORY))
#define MATE_IS_THUMBNAIL_FACTORY_CLASS(klass)	(G_TYPE_CLASS_CHECK_CLASS_TYPE ((klass), MATE_TYPE_THUMBNAIL_FACTORY))

typedef struct _MateThumbnailFactory        MateThumbnailFactory;
typedef struct _MateThumbnailFactoryClass   MateThumbnailFactoryClass;
typedef struct _MateThumbnailFactoryPrivate MateThumbnailFactoryPrivate;

struct _MateThumbnailFactory {
	GObject parent;

	MateThumbnailFactoryPrivate *priv;
};

struct _MateThumbnailFactoryClass {
	GObjectClass parent;
};

GType                  mate_thumbnail_factory_get_type (void);
MateThumbnailFactory *mate_thumbnail_factory_new      (MateThumbnailSize     size);

char *                 mate_thumbnail_factory_lookup   (MateThumbnailFactory *factory,
							 const char            *uri,
							 time_t                 mtime);

gboolean               mate_thumbnail_factory_has_valid_failed_thumbnail (MateThumbnailFactory *factory,
									   const char            *uri,
									   time_t                 mtime);
gboolean               mate_thumbnail_factory_can_thumbnail (MateThumbnailFactory *factory,
							      const char            *uri,
							      const char            *mime_type,
							      time_t                 mtime);
GdkPixbuf *            mate_thumbnail_factory_generate_thumbnail (MateThumbnailFactory *factory,
								   const char            *uri,
								   const char            *mime_type);
void                   mate_thumbnail_factory_save_thumbnail (MateThumbnailFactory *factory,
							       GdkPixbuf             *thumbnail,
							       const char            *uri,
							       time_t                 original_mtime);
void                   mate_thumbnail_factory_create_failed_thumbnail (MateThumbnailFactory *factory,
									const char            *uri,
									time_t                 mtime);


/* Thumbnailing utils: */
gboolean   mate_thumbnail_has_uri           (GdkPixbuf          *pixbuf,
					      const char         *uri);
gboolean   mate_thumbnail_is_valid          (GdkPixbuf          *pixbuf,
					      const char         *uri,
					      time_t              mtime);
char *     mate_thumbnail_md5               (const char         *uri);
char *     mate_thumbnail_path_for_uri      (const char         *uri,
					      MateThumbnailSize  size);


/* Pixbuf utils */

GdkPixbuf *mate_thumbnail_scale_down_pixbuf (GdkPixbuf          *pixbuf,
					      int                 dest_width,
					      int                 dest_height);

#ifdef __cplusplus
}
#endif

#endif /* MATE_THUMBNAIL_H */
