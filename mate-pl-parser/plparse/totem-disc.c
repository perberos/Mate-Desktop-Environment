/* Totem Disc Content Detection
 * Copyright (C) 2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2004-2007 Bastien Nocera <hadess@hadess.net>
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301  USA.
 *
 * The Totem project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and Totem. This
 * permission are above and beyond the permissions granted by the GPL license
 * Totem is covered by.
 *
 * Monday 7th February 2005: Christian Schaller: Add exception clause.
 * See license_change file for details.
 *
 */

/**
 * SECTION:totem-disc
 * @short_description: disc utility functions
 * @stability: Stable
 * @include: totem-disc.h
 *
 * This file has various different disc utility functions for getting
 * the media types and labels of discs.
 *
 * The functions in this file refer to MRLs, which are a special form
 * of URIs used by xine to refer to things such as DVDs. An example of
 * an MRL would be <literal>dvd:///dev/scd0</literal>, which is not a
 * valid URI as far as, for example, GIO is concerned.
 *
 * The rest of the totem-pl-parser API exclusively uses URIs.
 **/

#include "config.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <sys/stat.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

#include "totem-disc.h"
#include "totem-pl-parser.h"

typedef struct _CdCache {
  /* device node and mountpoint */
  char *device, *mountpoint;
  GVolume *volume;

  char **content_types;

  GFile *iso_file;

  /* Whether we have a medium */
  guint has_medium : 1;
  /* if we're checking a media, or a dir */
  guint is_media : 1;

  /* indicates if we mounted this mountpoint ourselves or if it
   * was already mounted. */
  guint self_mounted : 1;
  guint mounted : 1;

  /* Whether it's a local ISO file */
  guint is_iso : 1;
} CdCache;

typedef struct _CdCacheCallbackData {
  CdCache *cache;
  gboolean called;
  gboolean result;
  GError *error;
} CdCacheCallbackData;

static void cd_cache_free (CdCache *cache);

static char *
totem_resolve_symlink (const char *device, GError **error)
{
  char *dir, *link;
  char *f;
  char *f1;

  f = g_strdup (device);
  while (g_file_test (f, G_FILE_TEST_IS_SYMLINK)) {
    link = g_file_read_link (f, error);
    if(link == NULL) {
      g_free (f);
      return NULL;
    }

    dir = g_path_get_dirname (f);
    f1 = g_build_filename (dir, link, NULL);
    g_free (dir);
    g_free (f);
    f = f1;
  }

  if (f != NULL) {
    GFile *file;

    file = g_file_new_for_path (f);
    f1 = g_file_get_path (file);
    g_object_unref (file);
    g_free (f);
    f = f1;
  }
  return f;
}

static gboolean
cd_cache_get_best_mount_for_drive (GDrive *drive,
				   char **mountpoint,
				   GVolume **volume)
{
  GList *list, *l;
  int rank;

  rank = 0;
  list = g_drive_get_volumes (drive);
  for (l = list; l != NULL; l = l->next) {
    GVolume *v;
    GMount *m;
    GFile *file;
    int new_rank;

    v = l->data;

    m = g_volume_get_mount (v);
    if (m == NULL)
      continue;

    file = g_mount_get_root (m);
    if (g_file_has_uri_scheme (file, "cdda"))
      new_rank = 100;
    else
      new_rank = 50;

    if (new_rank > rank) {
      if (*mountpoint)
	g_free (*mountpoint);
      if (*volume)
	g_object_unref (volume);

      *volume = g_object_ref (v);
      *mountpoint = g_file_get_path (file);
      rank = new_rank;
    }

    g_object_unref (file);
    g_object_unref (m);
  }

  g_list_foreach (list, (GFunc) g_object_unref, NULL);
  g_list_free (list);

  return rank > 0;
}

static gboolean
cd_cache_get_dev_from_volumes (GVolumeMonitor *mon, const char *device,
			      char **mountpoint, GVolume **volume)
{
  GList *list, *l;
  gboolean found;

  found = FALSE;
  list = g_volume_monitor_get_connected_drives (mon);
  for (l = list; l != NULL; l = l->next) {
    GDrive *drive;
    char *ddev, *resolved;

    drive = l->data;
    ddev = g_drive_get_identifier (drive, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
    if (ddev == NULL)
      continue;
    resolved = totem_resolve_symlink (ddev, NULL);
    g_free (ddev);
    if (resolved == NULL)
      continue;

    if (strcmp (resolved, device) == 0) {
      found = cd_cache_get_best_mount_for_drive (drive, mountpoint, volume);
    }

    g_free (resolved);
    if (found != FALSE)
      break;
  }

  g_list_foreach (list, (GFunc) g_object_unref, NULL);
  g_list_free (list);

  if (found != FALSE)
    return found;

  /* Not in the drives? Look in the volumes themselves */
  found = FALSE;
  list = g_volume_monitor_get_volumes (mon);
  for (l = list; l != NULL; l = l->next) {
    GVolume *vol;
    char *ddev, *resolved;

    vol = l->data;
    ddev = g_volume_get_identifier (vol, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
    if (ddev == NULL)
      continue;
    resolved = totem_resolve_symlink (ddev, NULL);
    g_free (ddev);
    if (resolved == NULL)
      continue;

    if (strcmp (resolved, device) == 0) {
      found = TRUE;
      *volume = g_object_ref (vol);
    }

    g_free (resolved);
    if (found != FALSE)
      break;
  }

  g_list_foreach (list, (GFunc) g_object_unref, NULL);
  g_list_free (list);

  return found;
}

static gboolean
cd_cache_has_content_type (CdCache *cache, const char *content_type)
{
  guint i;

  if (cache->content_types == NULL) {
    return FALSE;
  }

  for (i = 0; cache->content_types[i] != NULL; i++) {
    if (g_str_equal (cache->content_types[i], content_type) != FALSE)
      return TRUE;
  }
  return FALSE;
}

static char *
cd_cache_uri_to_archive (const char *uri)
{
  char *escaped, *escaped2, *retval;

  escaped = g_uri_escape_string (uri, NULL, FALSE);
  escaped2 = g_uri_escape_string (escaped, NULL, FALSE);
  g_free (escaped);
  retval = g_strdup_printf ("archive://%s/", escaped2);
  g_free (escaped2);

  return retval;
}

static void
cd_cache_mount_archive_callback (GObject *source_object,
				 GAsyncResult *res,
				 CdCacheCallbackData *data)
{
  data->result = g_file_mount_enclosing_volume_finish (G_FILE (source_object), res, &data->error);
  data->called = TRUE;
}

static CdCache *
cd_cache_new (const char *dev,
	      GError     **error)
{
  CdCache *cache;
  char *mountpoint = NULL, *device, *local;
  GVolumeMonitor *mon;
  GVolume *volume = NULL;
  GFile *file;
  gboolean found, self_mounted;

  if (dev[0] == '/') {
    local = g_strdup (dev);
    file = g_file_new_for_path (dev);
  } else {
    file = g_file_new_for_commandline_arg (dev);
    local = g_file_get_path (file);
  }

  if (local == NULL) {
    /* No error, just no cache */
    g_object_unref (file);
    return NULL;
  }

  self_mounted = FALSE;

  if (g_file_test (local, G_FILE_TEST_IS_DIR) != FALSE) {
    cache = g_new0 (CdCache, 1);
    cache->mountpoint = local;
    cache->is_media = FALSE;
    cache->content_types = g_content_type_guess_for_tree (file);
    g_object_unref (file);

    return cache;
  } else if (g_file_test (local, G_FILE_TEST_IS_REGULAR)) {
    GMount *mount;
    GError *err = NULL;
    char *uri, *archive_path;

    cache = g_new0 (CdCache, 1);
    cache->is_iso = TRUE;
    cache->is_media = FALSE;

    uri = g_file_get_uri (file);
    g_object_unref (file);
    archive_path = cd_cache_uri_to_archive (uri);
    g_free (uri);
    cache->device = local;

    cache->iso_file = g_file_new_for_uri (archive_path);
    g_free (archive_path);

    mount = g_file_find_enclosing_mount (cache->iso_file, NULL, &err);
    if (mount == NULL && g_error_matches (err, G_IO_ERROR, G_IO_ERROR_NOT_MOUNTED)) {
      CdCacheCallbackData data;

      memset (&data, 0, sizeof(data));
      data.cache = cache;
      g_file_mount_enclosing_volume (cache->iso_file,
				     G_MOUNT_MOUNT_NONE,
				     NULL,
				     NULL,
				     (GAsyncReadyCallback) cd_cache_mount_archive_callback,
				     &data);
      while (!data.called) g_main_context_iteration (NULL, TRUE);

      if (!data.result) {
	if (data.error) {
	  g_propagate_error (error, data.error);
	} else {
	  g_set_error (error, TOTEM_PL_PARSER_ERROR, TOTEM_PL_PARSER_ERROR_MOUNT_FAILED,
		       _("Failed to mount %s."), cache->device);
	}
	cd_cache_free (cache);
	return FALSE;
      }
      self_mounted = TRUE;
    } else if (mount == NULL) {
      cd_cache_free (cache);
      return FALSE;
    } else {
      g_object_unref (mount);
    }

    cache->content_types = g_content_type_guess_for_tree (cache->iso_file);
    cache->mountpoint = g_file_get_path (cache->iso_file);
    cache->self_mounted = self_mounted;
    cache->mounted = TRUE;

    return cache;
  }

  g_object_unref (file);

  /* We have a local device
   * retrieve mountpoint and volume from gio volumes */
  device = totem_resolve_symlink (local, error);
  g_free (local);
  if (!device)
    return NULL;
  mon = g_volume_monitor_get ();
  found = cd_cache_get_dev_from_volumes (mon, device, &mountpoint, &volume);
  if (!found) {
    g_set_error (error, TOTEM_PL_PARSER_ERROR, TOTEM_PL_PARSER_ERROR_NO_DISC,
	_("No media in drive for device '%s'."),
	device);
    g_free (device);
    return NULL;
  }

  /* create struture */
  cache = g_new0 (CdCache, 1);
  cache->device = device;
  cache->mountpoint = mountpoint;
  cache->self_mounted = self_mounted;
  cache->volume = volume;
  cache->is_media = TRUE;

  {
    GMount *mount;

    mount = g_volume_get_mount (volume);
    cache->content_types = g_mount_guess_content_type_sync (mount, FALSE, NULL, NULL);
    g_object_unref (mount);
  }

  return cache;
}

static gboolean
cd_cache_has_medium (CdCache *cache)
{
  GDrive *drive;
  gboolean retval;

  if (cache->volume == NULL)
    return FALSE;

  drive = g_volume_get_drive (cache->volume);
  if (drive == NULL)
    return FALSE;
  retval = g_drive_has_media (drive);
  g_object_unref (drive);

  return retval;
}

static gboolean
cd_cache_open_device (CdCache *cache,
		      GError **error)
{
  /* not a medium? */
  if (cache->is_media == FALSE || cache->has_medium != FALSE) {
    return TRUE;
  }

  if (cd_cache_has_medium (cache) == FALSE) {
    g_set_error (error, TOTEM_PL_PARSER_ERROR, TOTEM_PL_PARSER_ERROR_NO_DISC,
	_("Please check that a disc is present in the drive."));
    return FALSE;
  }
  cache->has_medium = TRUE;

  return TRUE;
}

static void
cd_cache_mount_callback (GObject *source_object,
			 GAsyncResult *res,
			 CdCacheCallbackData *data)
{
  data->result = g_volume_mount_finish (data->cache->volume, res, &data->error);
  data->called = TRUE;
}

static gboolean
cd_cache_open_mountpoint (CdCache *cache,
			  GError **error)
{
  GMount *mount;
  GFile *root;

  /* already opened? */
  if (cache->mounted || cache->is_media == FALSE)
    return TRUE;

  /* check for mounting - assume we'll mount ourselves */
  if (cache->volume == NULL)
    return TRUE;

  mount = g_volume_get_mount (cache->volume);
  cache->self_mounted = (mount == NULL);

  /* mount if we have to */
  if (cache->self_mounted) {
    CdCacheCallbackData data;

    memset (&data, 0, sizeof(data));
    data.cache = cache;

    /* mount - wait for callback */
    g_volume_mount (cache->volume,
		    G_MOUNT_MOUNT_NONE,
		    NULL,
		    NULL,
		    (GAsyncReadyCallback) cd_cache_mount_callback,
		    &data);
    /* FIXME wait until it's done, any better way? */
    while (!data.called) g_main_context_iteration (NULL, TRUE);

    if (!data.result) {
      if (data.error) {
	g_propagate_error (error, data.error);
      } else {
	g_set_error (error, TOTEM_PL_PARSER_ERROR, TOTEM_PL_PARSER_ERROR_MOUNT_FAILED,
		     _("Failed to mount %s."), cache->device);
      }
      return FALSE;
    } else {
      cache->mounted = TRUE;
      mount = g_volume_get_mount (cache->volume);
    }
  }

  if (!cache->mountpoint) {
    root = g_mount_get_root (mount);
    cache->mountpoint = g_file_get_path (root);
    g_object_unref (root);
  }

  return TRUE;
}

static void
cd_cache_unmount_callback (GObject *source_object,
			   GAsyncResult *res,
			   CdCacheCallbackData *data)
{
  data->result = g_mount_unmount_with_operation_finish (G_MOUNT (source_object),
                                                        res, NULL);
  data->called = TRUE;
}

static void
cd_cache_free (CdCache *cache)
{
  GMount *mount;

  g_strfreev (cache->content_types);

  if (cache->iso_file && cache->self_mounted) {
    mount = g_file_find_enclosing_mount (cache->iso_file,
					 NULL, NULL);
    if (mount) {
      CdCacheCallbackData data;

      memset (&data, 0, sizeof(data));

      g_mount_unmount_with_operation (mount,
                                      G_MOUNT_UNMOUNT_NONE,
                                      NULL,
                                      NULL,
                                      (GAsyncReadyCallback) cd_cache_unmount_callback,
                                      &data);

      while (!data.called) g_main_context_iteration (NULL, TRUE);
      g_object_unref (mount);
    }
    g_object_unref (cache->iso_file);
  }

  /* free mem */
  if (cache->volume)
    g_object_unref (cache->volume);
  g_free (cache->mountpoint);
  g_free (cache->device);
  g_free (cache);
}

static TotemDiscMediaType
cd_cache_disc_is_cdda (CdCache *cache,
		       GError **error)
{
  /* We can't have audio CDs on disc, yet */
  if (cache->is_media == FALSE) {
    return MEDIA_TYPE_DATA;
  }
  if (!cd_cache_open_device (cache, error))
    return MEDIA_TYPE_ERROR;

  if (cd_cache_has_content_type (cache, "x-content/audio-cdda") != FALSE)
    return MEDIA_TYPE_CDDA;

  return MEDIA_TYPE_DATA;
}

static TotemDiscMediaType
cd_cache_disc_is_vcd (CdCache *cache,
                      GError **error)
{
  /* open disc and open mount */
  if (!cd_cache_open_device (cache, error))
    return MEDIA_TYPE_ERROR;
  if (!cd_cache_open_mountpoint (cache, error))
    return MEDIA_TYPE_ERROR;
  if (!cache->mountpoint)
    return MEDIA_TYPE_ERROR;

  if (cd_cache_has_content_type (cache, "x-content/video-vcd") != FALSE)
    return MEDIA_TYPE_VCD;
  if (cd_cache_has_content_type (cache, "x-content/video-svcd") != FALSE)
    return MEDIA_TYPE_VCD;

  return MEDIA_TYPE_DATA;
}

static TotemDiscMediaType
cd_cache_disc_is_dvd (CdCache *cache,
		      GError **error)
{
  /* open disc, check capabilities and open mount */
  if (!cd_cache_open_device (cache, error))
    return MEDIA_TYPE_ERROR;
  if (!cd_cache_open_mountpoint (cache, error))
    return MEDIA_TYPE_ERROR;
  if (!cache->mountpoint)
    return MEDIA_TYPE_ERROR;

  if (cd_cache_has_content_type (cache, "x-content/video-dvd") != FALSE)
    return MEDIA_TYPE_DVD;

  return MEDIA_TYPE_DATA;
}

/**
 * totem_cd_mrl_from_type:
 * @scheme: a scheme (e.g. "dvd")
 * @dir: a directory URI
 *
 * Builds an MRL using the scheme @scheme and the given URI @dir,
 * taking the filename from the URI if it's a <filename>file://</filename> and just
 * using the whole URI otherwise.
 *
 * Return value: a newly-allocated string containing the MRL
 **/
char *
totem_cd_mrl_from_type (const char *scheme, const char *dir)
{
  char *retval;

  if (g_str_has_prefix (dir, "file://") != FALSE) {
    char *local;
    local = g_filename_from_uri (dir, NULL, NULL);
    retval = g_strdup_printf ("%s://%s", scheme, local);
    g_free (local);
  } else {
    retval = g_strdup_printf ("%s://%s", scheme, dir);
  }
  return retval;
}

static char *
totem_cd_dir_get_parent (const char *dir)
{
  GFile *file, *parent_file;
  char *parent;

  file = g_file_new_for_path (dir);
  parent_file = g_file_get_parent (file);
  g_object_unref (file);

  parent = g_file_get_path (parent_file);
  g_object_unref (parent_file);

  return parent;
}

/**
 * totem_cd_detect_type_from_dir:
 * @dir: a directory URI
 * @mrl: (out) (transfer full) (allow-none): return location for the disc's MRL, or %NULL
 * @error: return location for a #GError, or %NULL
 *
 * Detects the disc's type, given its mount directory URI. If
 * a string pointer is passed to @mrl, it will return the disc's
 * MRL as from totem_cd_mrl_from_type().
 *
 * Note that this function does synchronous I/O.
 *
 * If no disc is present in the drive, a #TOTEM_PL_PARSER_ERROR_NO_DISC
 * error will be returned. On unknown mounting errors, a
 * #TOTEM_PL_PARSER_ERROR_MOUNT_FAILED error will be returned. On other
 * I/O errors, or if resolution of symlinked mount paths failed, a code from
 * #GIOErrorEnum will be returned.
 *
 * Return value: #TotemDiscMediaType corresponding to the disc's type, or #MEDIA_TYPE_ERROR on failure
 **/
TotemDiscMediaType
totem_cd_detect_type_from_dir (const char *dir, char **mrl, GError **error)
{
  CdCache *cache;
  TotemDiscMediaType type;

  g_return_val_if_fail (dir != NULL, MEDIA_TYPE_ERROR);

  if (!(cache = cd_cache_new (dir, error)))
    return MEDIA_TYPE_ERROR;
  if ((type = cd_cache_disc_is_vcd (cache, error)) == MEDIA_TYPE_DATA &&
      (type = cd_cache_disc_is_dvd (cache, error)) == MEDIA_TYPE_DATA) {
    /* is it the directory itself? */
    char *parent;

    cd_cache_free (cache);
    parent = totem_cd_dir_get_parent (dir);
    if (!parent)
      return type;

    cache = cd_cache_new (parent, error);
    g_free (parent);
    if (!cache)
      return MEDIA_TYPE_ERROR;
    if ((type = cd_cache_disc_is_vcd (cache, error)) == MEDIA_TYPE_DATA &&
	(type = cd_cache_disc_is_dvd (cache, error)) == MEDIA_TYPE_DATA) {
      /* crap, nothing found */
      cd_cache_free (cache);
      return type;
    }
  }

  if (mrl == NULL) {
    cd_cache_free (cache);
    return type;
  }

  if (type == MEDIA_TYPE_DVD) {
    *mrl = totem_cd_mrl_from_type ("dvd", cache->mountpoint);
  } else if (type == MEDIA_TYPE_VCD) {
    *mrl = totem_cd_mrl_from_type ("vcd", cache->mountpoint);
  }

  cd_cache_free (cache);

  return type;
}

/**
 * totem_cd_detect_type_with_url:
 * @device: a device node path
 * @mrl: (out) (transfer full) (allow-none): return location for the disc's MRL, or %NULL
 * @error: return location for a #GError, or %NULL
 *
 * Detects the disc's type, given its device node path. If
 * a string pointer is passed to @mrl, it will return the disc's
 * MRL as from totem_cd_mrl_from_type().
 *
 * Note that this function does synchronous I/O.
 *
 * Possible error codes are as per totem_cd_detect_type_from_dir().
 *
 * Return value: #TotemDiscMediaType corresponding to the disc's type, or #MEDIA_TYPE_ERROR on failure
 **/
TotemDiscMediaType
totem_cd_detect_type_with_url (const char *device,
    			       char      **mrl,
			       GError     **error)
{
  CdCache *cache;
  TotemDiscMediaType type;

  if (mrl != NULL)
    *mrl = NULL;

  if (!(cache = cd_cache_new (device, error)))
    return MEDIA_TYPE_ERROR;

  type = cd_cache_disc_is_cdda (cache, error);
  if (type == MEDIA_TYPE_ERROR && *error != NULL) {
    cd_cache_free (cache);
    return type;
  }

  if ((type == MEDIA_TYPE_DATA || type == MEDIA_TYPE_ERROR) &&
      (type = cd_cache_disc_is_vcd (cache, error)) == MEDIA_TYPE_DATA &&
      (type = cd_cache_disc_is_dvd (cache, error)) == MEDIA_TYPE_DATA) {
    /* crap, nothing found */
  }

  if (mrl == NULL) {
    cd_cache_free (cache);
    return type;
  }

  switch (type) {
  case MEDIA_TYPE_DVD:
    {
      const char *str;

      if (!cache->is_iso)
	str = cache->mountpoint ? cache->mountpoint : device;
      else
	str = cache->device;
      *mrl = totem_cd_mrl_from_type ("dvd", str);
    }
    break;
  case MEDIA_TYPE_VCD:
    {
      const char *str;

      if (!cache->is_iso)
	str = cache->mountpoint ? cache->mountpoint : device;
      else
	str = cache->device;
      *mrl = totem_cd_mrl_from_type ("vcd", str);
    }
    break;
  case MEDIA_TYPE_CDDA:
    {
      const char *dev;

      dev = cache->device ? cache->device : device;
      if (g_str_has_prefix (dev, "/dev/") != FALSE)
	*mrl = totem_cd_mrl_from_type ("cdda", dev + 5);
      else
	*mrl = totem_cd_mrl_from_type ("cdda", dev);
    }
    break;
  case MEDIA_TYPE_DATA:
    if (cache->is_iso) {
      type = MEDIA_TYPE_ERROR;
      /* No error, it's just not usable */
    } else {
      *mrl = g_filename_to_uri (cache->mountpoint, NULL, NULL);
      if (*mrl == NULL)
	*mrl = g_strdup (cache->mountpoint);
    }
    break;
  default:
    break;
  }

  cd_cache_free (cache);

  return type;
}

/**
 * totem_cd_detect_type:
 * @device: a device node path
 * @error: return location for a #GError, or %NULL
 *
 * Detects the disc's type, given its device node path.
 *
 * Possible error codes are as per totem_cd_detect_type_with_url().
 *
 * Return value: #TotemDiscMediaType corresponding to the disc's type, or #MEDIA_TYPE_ERROR on failure
 **/
TotemDiscMediaType
totem_cd_detect_type (const char  *device,
		      GError     **error)
{
  return totem_cd_detect_type_with_url (device, NULL, error);
}

/**
 * totem_cd_has_medium:
 * @device: a device node path
 *
 * Returns whether the disc has a physical medium.
 *
 * Return value: %TRUE if the disc physically exists
 **/
gboolean
totem_cd_has_medium (const char *device)
{
  CdCache *cache;
  gboolean retval = TRUE;

  if (!(cache = cd_cache_new (device, NULL)))
    return TRUE;

  retval = cd_cache_has_medium (cache);
  cd_cache_free (cache);

  return retval;
}

/**
 * totem_cd_get_human_readable_name:
 * @type: a #TotemDiscMediaType
 *
 * Returns the human-readable name for the given
 * #TotemDiscMediaType.
 *
 * Return value: the disc media type's readable name, which must not be freed, or %NULL for unhandled media types
 **/
const char *
totem_cd_get_human_readable_name (TotemDiscMediaType type)
{
  switch (type)
  {
  case MEDIA_TYPE_CDDA:
    return N_("Audio CD");
  case MEDIA_TYPE_VCD:
    return N_("Video CD");
  case MEDIA_TYPE_DVD:
    return N_("DVD");
  case MEDIA_TYPE_DVB:
    return N_("Digital Television");
  default:
    g_assert_not_reached ();
  }

  return NULL;
}

GQuark
totem_disc_media_type_quark (void)
{
  static GQuark quark = 0;
  if (!quark)
    quark = g_quark_from_static_string ("totem_disc_media_type");

  return quark;
}

/*
 * vim: sw=2 ts=8 cindent noai bs=2
 */
