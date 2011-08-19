/* gdict-source-loader.c - Source loader for Gdict
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

/**
 * SECTION:gdict-source-loader
 * @short_description: Loader object for a set of dictionary sources
 *
 * #GdictSourceLoader allows searching for dictionary source definition
 * files inside a set of paths and return a #GdictSource using its name.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>

#include "gdict-source-loader.h"
#include "gdict-utils.h"
#include "gdict-enum-types.h"
#include "gdict-marshal.h"
#include "gdict-private.h"

#define GDICT_SOURCE_FILE_SUFFIX	 	".desktop"

#define GDICT_SOURCE_LOADER_GET_PRIVATE(obj)	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), GDICT_TYPE_SOURCE_LOADER, GdictSourceLoaderPrivate))

struct _GdictSourceLoaderPrivate
{
  GSList *paths;
  
  GSList *sources;
  GHashTable *sources_by_name;
  
  guint paths_dirty : 1;
};

enum
{
  PROP_0,
  
  PROP_PATHS,
  PROP_SOURCES
};

enum
{
  SOURCE_LOADED,
  
  LAST_SIGNAL
};

static guint loader_signals[LAST_SIGNAL] = { 0 };



G_DEFINE_TYPE (GdictSourceLoader, gdict_source_loader, G_TYPE_OBJECT);


static void
gdict_source_loader_finalize (GObject *object)
{
  GdictSourceLoaderPrivate *priv = GDICT_SOURCE_LOADER_GET_PRIVATE (object);
  
  if (priv->paths)
    {
      g_slist_foreach (priv->paths,
      		       (GFunc) g_free,
      		       NULL);
      g_slist_free (priv->paths);
      
      priv->paths = NULL;
    }
  
  if (priv->sources_by_name)
    g_hash_table_destroy (priv->sources_by_name);
  
  if (priv->sources)
    {
      g_slist_foreach (priv->sources,
      		       (GFunc) g_object_unref,
      		       NULL);
      g_slist_free (priv->sources);
      
      priv->sources = NULL;
    }
  
  G_OBJECT_CLASS (gdict_source_loader_parent_class)->finalize (object);
}

static void
gdict_source_loader_set_property (GObject      *object,
				  guint         prop_id,
				  const GValue *value,
				  GParamSpec   *pspec)
{
  switch (prop_id)
    {
    case PROP_PATHS:
      break;
    case PROP_SOURCES:
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gdict_source_loader_get_property (GObject    *object,
				  guint       prop_id,
				  GValue     *value,
				  GParamSpec *pspec)
{
  switch (prop_id)
    {
    case PROP_PATHS:
      break;
    case PROP_SOURCES:
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gdict_source_loader_class_init (GdictSourceLoaderClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  
  gobject_class->set_property = gdict_source_loader_set_property;
  gobject_class->get_property = gdict_source_loader_get_property;
  gobject_class->finalize = gdict_source_loader_finalize;
  
  /**
   * GdictSourceLoader:paths
   *
   * The search paths used by this object
   *
   * Since: 1.0
   */
  g_object_class_install_property (gobject_class,
  				   PROP_PATHS,
  				   g_param_spec_pointer ("paths",
  				   			 _("Paths"),
  				   			 _("Search paths used by this object"),
  				   			 G_PARAM_READABLE));
  /**
   * GdictSourceLoader:sources
   *
   * The #GdictSource objects found by this object
   *
   * Since: 1.0
   */
  g_object_class_install_property (gobject_class,
  				   PROP_SOURCES,
  				   g_param_spec_pointer ("sources",
  				   			 _("Sources"),
  				   			 _("Dictionary sources found"),
  				   			 G_PARAM_READABLE));
  
  /**
   * GdictSourceLoader::source-loaded
   * @loader: the object which received the signal
   * @source: the new #GdictSource object found
   * 
   * This signal is emitted when a new dictionary source has been added
   * to the list.
   *
   * Since: 1.0
   */
  loader_signals[SOURCE_LOADED] =
    g_signal_new ("source-loaded",
    		  G_OBJECT_CLASS_TYPE (gobject_class),
    		  G_SIGNAL_RUN_LAST,
    		  G_STRUCT_OFFSET (GdictSourceLoaderClass, source_loaded),
    		  NULL, NULL,
    		  gdict_marshal_VOID__OBJECT,
    		  G_TYPE_NONE, 1,
    		  GDICT_TYPE_SOURCE);
  
  g_type_class_add_private (klass, sizeof (GdictSourceLoaderPrivate));
}

static void
gdict_source_loader_init (GdictSourceLoader *loader)
{
  GdictSourceLoaderPrivate *priv;
  
  priv = GDICT_SOURCE_LOADER_GET_PRIVATE (loader);
  loader->priv = priv;
  
  priv->paths = NULL;
  /* add the default, system-wide path */
  priv->paths = g_slist_prepend (priv->paths, g_strdup (GDICTSOURCESDIR));
  
  priv->sources = NULL;
  priv->sources_by_name = g_hash_table_new_full (g_str_hash, g_str_equal,
		  				 (GDestroyNotify) g_free,
						 NULL);
  
  /* ensure that the sources list will be updated */
  priv->paths_dirty = TRUE;
}

/**
 * gdict_source_loader_new:
 *
 * Creates a new #GdictSourceLoader object.  This object is used to search
 * into a list of paths for dictionary source files.  See #GdictSource for
 * more informations about the format of dictionary source files.
 *
 * Return value: a new #GdictSourceLoader object
 *
 * Since: 1.0
 */
GdictSourceLoader *
gdict_source_loader_new (void)
{
  return g_object_new (GDICT_TYPE_SOURCE_LOADER, NULL);
}

/**
 * gdict_source_loader_update:
 * @loader: a #GdictSourceLoader
 *
 * Queue an update of the sources inside @loader.
 *
 * Since: 1.0
 */
void
gdict_source_loader_update (GdictSourceLoader *loader)
{
  g_return_if_fail (GDICT_IS_SOURCE_LOADER (loader));
  
  loader->priv->paths_dirty = TRUE;
}

/**
 * gdict_source_loader_add_search_path:
 * @loader: a #GdictSourceLoader
 * @path: a path to be added to the search path list
 *
 * Adds @path to the search paths list of @loader.
 *
 * Since: 1.0
 */
void
gdict_source_loader_add_search_path (GdictSourceLoader *loader,
				     const gchar       *path)
{
  GSList *l;

  g_return_if_fail (GDICT_IS_SOURCE_LOADER (loader));
  g_return_if_fail (path != NULL);
 
  /* avoid duplications */
  for (l = loader->priv->paths; l != NULL; l = l->next)
    if (strcmp (path, (gchar *) l->data) == 0)
      return;
  
  loader->priv->paths = g_slist_append (loader->priv->paths, g_strdup (path));
  loader->priv->paths_dirty = TRUE;
}

/**
 * gdict_source_loader_get_paths:
 * @loader: a #GdictSourceLoader
 *
 * Gets the list of paths used by @loader to search for dictionary source
 * files.
 *
 * Return value: a list containing the paths.  The returned list is owned
 *   by the #GdictSourceLoader object and should never be free or modified.
 *
 * Since: 1.0
 */
G_CONST_RETURN GSList *
gdict_source_loader_get_paths (GdictSourceLoader *loader)
{
  g_return_val_if_fail (GDICT_IS_SOURCE_LOADER (loader), NULL);

  return loader->priv->paths;
}

/* create the list of dictionary source files, by scanning the search path
 * directories for .desktop files; we disavow symlinks and sub-directories
 * for the time being.
 */
static GSList *
build_source_filenames (GdictSourceLoader *loader)
{
  GSList *retval, *d;
  
  g_assert (GDICT_IS_SOURCE_LOADER (loader));
  
  if (!loader->priv->paths)
    return NULL;
  
  retval = NULL;
  for (d = loader->priv->paths; d != NULL; d = d->next)
    {
      gchar *path = (gchar *) d->data;
      const gchar *filename;
      GDir *dir;
      
      dir = g_dir_open (path, 0, NULL);
      if (!dir)
        continue;
      
      do
        {
          filename = g_dir_read_name (dir);
          if (filename)
            {
              gchar *full_path;
              
              if (!g_str_has_suffix (filename, GDICT_SOURCE_FILE_SUFFIX))
                break;
              
              full_path = g_build_filename (path, filename, NULL);
              if (g_file_test (full_path, G_FILE_TEST_IS_REGULAR))
                {
		  retval = g_slist_prepend (retval, full_path);
		}
            }
        }
      while (filename != NULL);

      g_dir_close (dir);
    }

  return g_slist_reverse (retval);
}

static void
gdict_source_loader_update_sources (GdictSourceLoader *loader)
{
  GSList *filenames, *f;
  
  g_assert (GDICT_IS_SOURCE_LOADER (loader));

  g_slist_foreach (loader->priv->sources,
		   (GFunc) g_object_unref,
		   NULL);
  g_slist_free (loader->priv->sources);
  loader->priv->sources = NULL;

  filenames = build_source_filenames (loader);
  for (f = filenames; f != NULL; f = f->next)
    {
      GdictSource *source;
      GError *load_err;
      gchar *path = (gchar *) f->data;

      g_assert (path != NULL);

      source = gdict_source_new ();
          
      load_err = NULL;
      gdict_source_load_from_file (source, path, &load_err);
      if (load_err)
        {
           g_warning ("Unable to load dictionary source at '%s': %s\n",
                      path,
                      load_err->message);
           g_error_free (load_err);
           
           continue;
        }
      
      loader->priv->sources = g_slist_append (loader->priv->sources,
                                              source);
      g_hash_table_replace (loader->priv->sources_by_name,
                            g_strdup (gdict_source_get_name (source)),
                            source);

      g_signal_emit (loader, loader_signals[SOURCE_LOADED], 0, source);
    }
  
  g_slist_foreach (filenames,
                   (GFunc) g_free,
                   NULL);
  g_slist_free (filenames);
  
  loader->priv->paths_dirty = FALSE;
}

/**
 * gdict_source_loader_get_names:
 * @loader: a #GdictSourceLoader
 * @length: return location for the number of source names, or %NULL
 *
 * Retrieves the list of dictionary source names available into the
 * search paths of @loader.
 *
 * Return value: a newly allocated, %NULL terminated array of strings.  You
 *   should free the returned string array with g_strfreev()
 *
 * Since: 1.0
 */
gchar **
gdict_source_loader_get_names (GdictSourceLoader *loader,
			       gsize             *length)
{
  GSList *l;
  gchar **names;
  gsize i;
  
  g_return_val_if_fail (GDICT_IS_SOURCE_LOADER (loader), NULL);
  
  if (loader->priv->paths_dirty)
    gdict_source_loader_update_sources (loader);
  
  names = g_new0 (gchar *, g_slist_length (loader->priv->sources) + 1);
  
  i = 0;
  for (l = loader->priv->sources; l != NULL; l = l->next)
    {
      GdictSource *s = GDICT_SOURCE (l->data);
      
      g_assert (s != NULL);
      
      names[i++] = g_strdup (gdict_source_get_name (s));
    }
  names[i] = NULL;
  
  if (length)
    *length = i;
  
  return names;
}

/**
 * gdict_source_loader_get_sources:
 * @loader: a #GdictSourceLoader
 *
 * Retrieves the list of dictionary sources available into the search
 * paths of @loader, in form of #GdictSource objects.
 *
 * Return value: a list of #GdictSource objects.  The returned list
 *   is owned by the #GdictSourceLoader object, and should never be
 *   freed or modified.
 *
 * Since: 1.0
 */
G_CONST_RETURN GSList *
gdict_source_loader_get_sources (GdictSourceLoader *loader)
{
  g_return_val_if_fail (GDICT_IS_SOURCE_LOADER (loader), NULL);
  
  if (loader->priv->paths_dirty)
    gdict_source_loader_update_sources (loader);
  
  return loader->priv->sources;
}

/**
 * gdict_source_loader_get_source:
 * @loader: a #GdictSourceLoader
 * @name: a name of a dictionary source
 *
 * Retrieves a dictionary source using @name.  You can use the returned
 * #GdictSource object to create the right #GdictContext for that
 * dictionary source.
 *
 * Return value: a referenced #GdictSource object.  You should de-reference
 *   it using g_object_unref() when you finished using it.
 *
 * Since: 1.0
 */
GdictSource *
gdict_source_loader_get_source (GdictSourceLoader *loader,
				const gchar       *name)
{
  GdictSource *retval;
  
  g_return_val_if_fail (GDICT_IS_SOURCE_LOADER (loader), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  
  if (loader->priv->paths_dirty)
    gdict_source_loader_update_sources (loader);
  
  retval = g_hash_table_lookup (loader->priv->sources_by_name, name);
  if (retval)
    return g_object_ref (retval);
  
  return NULL;
}

/**
 * gdict_source_loader_remove_source:
 * @loader: a #GdictSourceLoader
 * @name: name of a dictionary source
 *
 * Removes the dictionary source @name from @loader.  This function will
 * also remove the dictionary source definition file bound to it.
 *
 * Return value: %TRUE if the dictionary source was successfully removed
 *
 * Since: 1.0
 */
gboolean
gdict_source_loader_remove_source (GdictSourceLoader *loader,
				   const gchar       *name)
{
  GdictSourceLoaderPrivate *priv;
  GSList *l;
  
  g_return_val_if_fail (GDICT_IS_SOURCE_LOADER (loader), FALSE);
  g_return_val_if_fail (name != NULL, FALSE);
  
  priv = loader->priv;
  
  if (priv->paths_dirty)
    gdict_source_loader_update_sources (loader);
  
  for (l = priv->sources; l != NULL; l = l->next)
    {
      GdictSource *s = GDICT_SOURCE (l->data);
      
      if (strcmp (gdict_source_get_name (s), name) == 0)
        {
          gchar *filename;
          
          g_object_get (G_OBJECT (s), "filename", &filename, NULL);
          
          if (g_unlink (filename) == -1)
            {
              g_warning ("Unable to remove filename '%s' for the "
                         "dictionary source '%s'\n",
                         filename,
                         name);
              
              return FALSE;
            }
          
          g_hash_table_remove (priv->sources_by_name, name);
          
          priv->sources = g_slist_remove_link (priv->sources, l);
          
          g_object_unref (s);
          g_slist_free (l);
          
          return TRUE;
        }
    }
  
  return FALSE;
}

/**
 * gdict_source_loader_has_source:
 * @loader: a #GdictSourceLoader
 * @source_name: the name of a dictionary source
 *
 * Checks whether @loader has a dictionary source with name @source_name.
 *
 * Return value: %TRUE if the dictionary source is known
 *
 * Since: 0.12
 */
gboolean
gdict_source_loader_has_source (GdictSourceLoader *loader,
                                const gchar       *source_name)
{
  g_return_val_if_fail (GDICT_IS_SOURCE_LOADER (loader), FALSE);
  g_return_val_if_fail (source_name != NULL, FALSE);

  if (loader->priv->paths_dirty)
    gdict_source_loader_update_sources (loader);

  return (g_hash_table_lookup (loader->priv->sources_by_name, source_name) != NULL);
}
