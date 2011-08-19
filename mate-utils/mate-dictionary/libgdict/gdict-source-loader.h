/* gdict-source-loader.h - Source loader for Gdict
 *
 * Copyright (C) 2005  Emmanuele Bassi <ebassi@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 */

#ifndef __GDICT_SOURCE_LOADER_H__
#define __GDICT_SOURCE_LOADER_H__

#include <glib-object.h>
#include "gdict-source.h"

G_BEGIN_DECLS

#define GDICT_TYPE_SOURCE_LOADER	    (gdict_source_loader_get_type ())
#define GDICT_SOURCE_LOADER(obj)	    (G_TYPE_CHECK_INSTANCE_CAST ((obj), GDICT_TYPE_SOURCE_LOADER, GdictSourceLoader))
#define GDICT_IS_SOURCE_LOADER(obj)	    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDICT_TYPE_SOURCE_LOADER))
#define GDICT_SOURCE_LOADER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GDICT_TYPE_SOURCE_LOADER, GdictSourceLoaderClass))
#define GDICT_IS_SOURCE_LOADER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GDICT_TYPE_SOURCE_LOADER))
#define GDICT_SOURCE_LOADER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GDICT_TYPE_SOURCE_LOADER, GdictSourceLoaderClass))

typedef struct _GdictSourceLoader	 GdictSourceLoader;
typedef struct _GdictSourceLoaderClass	 GdictSourceLoaderClass;
typedef struct _GdictSourceLoaderPrivate GdictSourceLoaderPrivate;


struct _GdictSourceLoader
{
  /*< private >*/
  GObject parent_instance;
  
  GdictSourceLoaderPrivate *priv;
};

struct _GdictSourceLoaderClass
{
  GObjectClass parent_class;
  
  void (*source_loaded) (GdictSourceLoader *loader,
  			 GdictSource       *source);
  
  /* padding for future expansion */
  void (*_gdict_source_1) (void);
  void (*_gdict_source_2) (void);
  void (*_gdict_source_3) (void);
  void (*_gdict_source_4) (void);
};

GType gdict_source_loader_get_type (void) G_GNUC_CONST;

GdictSourceLoader *    gdict_source_loader_new             (void);
void                   gdict_source_loader_update          (GdictSourceLoader *loader);
void                   gdict_source_loader_add_search_path (GdictSourceLoader *loader,
							    const gchar       *path);
G_CONST_RETURN GSList *gdict_source_loader_get_paths       (GdictSourceLoader *loader);
gchar **               gdict_source_loader_get_names       (GdictSourceLoader *loader,
							    gsize             *length) G_GNUC_MALLOC;
G_CONST_RETURN GSList *gdict_source_loader_get_sources     (GdictSourceLoader *loader);
GdictSource *          gdict_source_loader_get_source      (GdictSourceLoader *loader,
							    const gchar       *name);
gboolean               gdict_source_loader_has_source      (GdictSourceLoader *loader,
                                                            const gchar       *source_name);
gboolean               gdict_source_loader_remove_source   (GdictSourceLoader *loader,
							    const gchar       *name);

G_END_DECLS

#endif /* __GDICT_SOURCE_LOADER_H__ */
