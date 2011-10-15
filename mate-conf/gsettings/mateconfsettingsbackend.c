/*
 * Copyright (C) 2010 Novell, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Vincent Untz <vuntz@gnome.org>
 */

#include "config.h"

#include <string.h>

#include <glib.h>
#include <gio/gio.h>
#include <mateconf/mateconf-client.h>

#include "mateconfsettingsbackend.h"

G_DEFINE_DYNAMIC_TYPE (MateConfSettingsBackend, mateconf_settings_backend, G_TYPE_SETTINGS_BACKEND);

typedef struct _MateConfSettingsBackendNotifier MateConfSettingsBackendNotifier;

struct _MateConfSettingsBackendPrivate
{
  MateConfClient *client;
  GSList      *notifiers;
  /* By definition, with GSettings, we can't write to a key if we're not
   * subscribed to it or its parent. This means we'll be monitoring it, and
   * that we'll get a change notification for the write. That's something that
   * should get ignored. */
  GHashTable  *ignore_notifications;
};

/* The rationale behind the non-trivial handling of notifiers here is that we
 * only want to receive one notification when a key changes. The naive approach
 * would be to register one notifier per subscribed path, but in mateconf, we get
 * notificiations for all keys living below the path. So subscribing to
 * /apps/panel and /apps/panel/general will lead to two notifications for a key
 * living under /apps/panel/general. We want to avoid that, so we will only
 * have a notifier for /apps/panel in such a case. */
struct _MateConfSettingsBackendNotifier
{
  MateConfSettingsBackendNotifier *parent;
  gchar  *path;
  guint   refcount;
  guint   notify_id;
  GSList *subpaths;
};

static void
mateconf_settings_backend_notified (MateConfClient          *client,
                                 guint                 cnxn_id,
                                 MateConfEntry           *entry,
                                 MateConfSettingsBackend *mateconf);

/**********************\
 * Notifiers handling *
\**********************/

static MateConfSettingsBackendNotifier *
mateconf_settings_backend_find_notifier_or_parent (MateConfSettingsBackend *mateconf,
                                                const gchar          *path)
{
  MateConfSettingsBackendNotifier *parent;
  GSList *l;

  l = mateconf->priv->notifiers;
  parent = NULL;

  while (l != NULL)
    {
      MateConfSettingsBackendNotifier *notifier;
      notifier = l->data;
      if (g_str_equal (path, notifier->path))
        return notifier;
      if (g_str_has_prefix (path, notifier->path))
        {
          parent = notifier;
          l = parent->subpaths;
          continue;
        }
      if (g_str_has_prefix (notifier->path, path))
        break;

      l = l->next;
    }

  return parent;
}

static void
mateconf_settings_backend_free_notifier (MateConfSettingsBackendNotifier *notifier,
                                      MateConfSettingsBackend         *mateconf)
{
  if (notifier->path)
    g_free (notifier->path);
  notifier->path = NULL;

  if (notifier->notify_id)
    mateconf_client_notify_remove (mateconf->priv->client, notifier->notify_id);
  notifier->notify_id = 0;

  g_slist_foreach (notifier->subpaths, (GFunc) mateconf_settings_backend_free_notifier, mateconf);
  g_slist_free (notifier->subpaths);
  notifier->subpaths = NULL;

  g_slice_free (MateConfSettingsBackendNotifier, notifier);
}

/* Returns: TRUE if the notifier was created, FALSE if it was already existing. */
static gboolean
mateconf_settings_backend_add_notifier (MateConfSettingsBackend *mateconf,
                                     const gchar          *path)
{
  MateConfSettingsBackendNotifier *n_or_p;
  MateConfSettingsBackendNotifier *notifier;
  GSList *siblings;
  GSList *l;

  n_or_p = mateconf_settings_backend_find_notifier_or_parent (mateconf, path);

  if (n_or_p && g_str_equal (path, n_or_p->path))
    {
      n_or_p->refcount += 1;
      return FALSE;
    }

  notifier = g_slice_new0 (MateConfSettingsBackendNotifier);
  notifier->parent = n_or_p;
  notifier->path = g_strdup (path);
  notifier->refcount = 1;

  if (notifier->parent == NULL)
    notifier->notify_id = mateconf_client_notify_add (mateconf->priv->client, path,
                                                   (MateConfClientNotifyFunc) mateconf_settings_backend_notified, mateconf,
                                                   NULL, NULL);
  else
    notifier->notify_id = 0;

  /* Move notifiers living at the same level but that are subpaths below this
   * new notifier, removing their notify handler if necessary. */
  if (notifier->parent)
    siblings = notifier->parent->subpaths;
  else
    siblings = mateconf->priv->notifiers;

  l = siblings;
  while (l != NULL)
    {
      MateConfSettingsBackendNotifier *sibling;
      GSList *next;

      sibling = l->data;
      next = l->next;

      if (g_str_has_prefix (sibling->path, notifier->path))
        {
          if (sibling->notify_id)
            {
              mateconf_client_notify_remove (mateconf->priv->client,
                                          sibling->notify_id);
              sibling->notify_id = 0;
            }

          siblings = g_slist_remove_link (siblings, l);
          l->next = notifier->subpaths;
          notifier->subpaths = l;
        }

      l = next;
    }

  siblings = g_slist_prepend (siblings, notifier);

  if (notifier->parent)
    notifier->parent->subpaths = siblings;
  else
    mateconf->priv->notifiers = siblings;

  return TRUE;
}

/* Returns: TRUE if the notifier was removed, FALSE if it is still referenced. */
static gboolean
mateconf_settings_backend_remove_notifier (MateConfSettingsBackend *mateconf,
                                        const gchar          *path)
{
  MateConfSettingsBackendNotifier *notifier;

  notifier = mateconf_settings_backend_find_notifier_or_parent (mateconf, path);

  g_assert (notifier && g_str_equal (path, notifier->path));

  notifier->refcount -= 1;

  if (notifier->refcount > 0)
    return FALSE;

  /* Move subpaths to the parent, and add a notify handler for each of them if
   * they have no parent anymore. */
  if (notifier->parent)
    {
      GSList *l;

      for (l = notifier->subpaths; l != NULL; l = l->next)
        {
          MateConfSettingsBackendNotifier *child = l->data;
          child->parent = notifier->parent;
        }

      notifier->parent->subpaths = g_slist_remove (notifier->parent->subpaths,
                                                   notifier);
      notifier->parent->subpaths = g_slist_concat (notifier->parent->subpaths,
                                                   notifier->subpaths);
    }
  else
    {
      GSList *l;

      for (l = notifier->subpaths; l != NULL; l = l->next)
        {
          MateConfSettingsBackendNotifier *child = l->data;
          child->parent = NULL;
          child->notify_id = mateconf_client_notify_add (mateconf->priv->client, child->path,
                                                      (MateConfClientNotifyFunc) mateconf_settings_backend_notified, mateconf,
                                                      NULL, NULL);
        }

      mateconf->priv->notifiers = g_slist_remove (mateconf->priv->notifiers,
                                               notifier);
      mateconf->priv->notifiers = g_slist_concat (mateconf->priv->notifiers,
                                               notifier->subpaths);
    }

  notifier->subpaths = NULL;

  mateconf_settings_backend_free_notifier (notifier, mateconf);

  return TRUE;
}



/***************************\
 * MateConfValue <=> GVariant *
\***************************/

static gboolean
mateconf_settings_backend_simple_mateconf_value_type_is_compatible (MateConfValueType      type,
                                                              const GVariantType *expected_type)
{
  switch (type)
    {
    case MATECONF_VALUE_STRING:
      return (g_variant_type_equal (expected_type, G_VARIANT_TYPE_STRING)      ||
              g_variant_type_equal (expected_type, G_VARIANT_TYPE_OBJECT_PATH) ||
              g_variant_type_equal (expected_type, G_VARIANT_TYPE_SIGNATURE));
    case MATECONF_VALUE_INT:
      return (g_variant_type_equal (expected_type, G_VARIANT_TYPE_BYTE)   ||
              g_variant_type_equal (expected_type, G_VARIANT_TYPE_INT16)  ||
              g_variant_type_equal (expected_type, G_VARIANT_TYPE_UINT16) ||
              g_variant_type_equal (expected_type, G_VARIANT_TYPE_INT32)  ||
              g_variant_type_equal (expected_type, G_VARIANT_TYPE_UINT32) ||
              g_variant_type_equal (expected_type, G_VARIANT_TYPE_INT64)  ||
              g_variant_type_equal (expected_type, G_VARIANT_TYPE_UINT64) ||
              g_variant_type_equal (expected_type, G_VARIANT_TYPE_HANDLE));
    case MATECONF_VALUE_FLOAT:
      return g_variant_type_equal (expected_type, G_VARIANT_TYPE_DOUBLE);
    case MATECONF_VALUE_BOOL:
      return g_variant_type_equal (expected_type, G_VARIANT_TYPE_BOOLEAN);
    case MATECONF_VALUE_LIST:
    case MATECONF_VALUE_PAIR:
      return FALSE;
    default:
      return FALSE;
    }
}

static GVariant *
mateconf_settings_backend_simple_mateconf_value_type_to_gvariant (MateConfValue         *mateconf_value,
                                                            const GVariantType *expected_type)
{
  /* Note: it's guaranteed that the types are compatible */
  GVariant *variant = NULL;

  if (g_variant_type_equal (expected_type, G_VARIANT_TYPE_BOOLEAN))
    variant = g_variant_new_boolean (mateconf_value_get_bool (mateconf_value));
  else if (g_variant_type_equal (expected_type, G_VARIANT_TYPE_BYTE))
    {
      int value = mateconf_value_get_int (mateconf_value);
      if (value < 0 || value > 255)
        return NULL;
      variant = g_variant_new_byte (value);
    }
  else if (g_variant_type_equal (expected_type, G_VARIANT_TYPE_INT16))
    {
      int value = mateconf_value_get_int (mateconf_value);
      if (value < G_MINSHORT || value > G_MAXSHORT)
        return NULL;
      variant = g_variant_new_int16 (value);
    }
  else if (g_variant_type_equal (expected_type, G_VARIANT_TYPE_UINT16))
    {
      int value = mateconf_value_get_int (mateconf_value);
      if (value < 0 || value > G_MAXUSHORT)
        return NULL;
      variant = g_variant_new_uint16 (value);
    }
  else if (g_variant_type_equal (expected_type, G_VARIANT_TYPE_INT32))
    variant = g_variant_new_int32 (mateconf_value_get_int (mateconf_value));
  else if (g_variant_type_equal (expected_type, G_VARIANT_TYPE_UINT32))
    {
      int value = mateconf_value_get_int (mateconf_value);
      if (value < 0)
        return NULL;
      variant = g_variant_new_uint32 (value);
    }
  else if (g_variant_type_equal (expected_type, G_VARIANT_TYPE_INT64))
    variant = g_variant_new_int64 ((gint64) mateconf_value_get_int (mateconf_value));
  else if (g_variant_type_equal (expected_type, G_VARIANT_TYPE_UINT64))
    {
      int value = mateconf_value_get_int (mateconf_value);
      if (value < 0)
        return NULL;
      variant = g_variant_new_uint64 (value);
    }
  else if (g_variant_type_equal (expected_type, G_VARIANT_TYPE_HANDLE))
    {
      int value = mateconf_value_get_int (mateconf_value);
      if (value < 0)
        return NULL;
      variant = g_variant_new_handle (value);
    }
  else if (g_variant_type_equal (expected_type, G_VARIANT_TYPE_DOUBLE))
    variant = g_variant_new_double (mateconf_value_get_float (mateconf_value));
  else if (g_variant_type_equal (expected_type, G_VARIANT_TYPE_STRING))
    variant = g_variant_new_string (mateconf_value_get_string (mateconf_value));
  else if (g_variant_type_equal (expected_type, G_VARIANT_TYPE_OBJECT_PATH))
    variant = g_variant_new_object_path (mateconf_value_get_string (mateconf_value));
  else if (g_variant_type_equal (expected_type, G_VARIANT_TYPE_SIGNATURE))
    variant = g_variant_new_signature (mateconf_value_get_string (mateconf_value));

  return variant;
}

static GVariant *
mateconf_settings_backend_mateconf_value_to_gvariant (MateConfValue         *mateconf_value,
                                                const GVariantType *expected_type)
{
  switch (mateconf_value->type)
    {
    case MATECONF_VALUE_STRING:
    case MATECONF_VALUE_INT:
    case MATECONF_VALUE_FLOAT:
    case MATECONF_VALUE_BOOL:
      if (!mateconf_settings_backend_simple_mateconf_value_type_is_compatible (mateconf_value->type, expected_type))
        return NULL;
      return mateconf_settings_backend_simple_mateconf_value_type_to_gvariant (mateconf_value, expected_type);
    case MATECONF_VALUE_LIST:
      {
        MateConfValueType      list_type;
        const GVariantType *array_type;
        GSList             *list;
        GPtrArray          *array;
        GVariant           *result;

        if (!g_variant_type_is_array (expected_type))
          return NULL;

        list_type = mateconf_value_get_list_type (mateconf_value);
        array_type = g_variant_type_element (expected_type);
        if (!mateconf_settings_backend_simple_mateconf_value_type_is_compatible (list_type, array_type))
          return NULL;

        array = g_ptr_array_new ();
        for (list = mateconf_value_get_list (mateconf_value); list != NULL; list = list->next)
          {
            GVariant *variant;
            variant = mateconf_settings_backend_simple_mateconf_value_type_to_gvariant (list->data, array_type);
            g_ptr_array_add (array, variant);
          }

        result = g_variant_new_array (array_type, (GVariant **) array->pdata, array->len);
        g_ptr_array_free (array, TRUE);

        return result;
      }
      break;
    case MATECONF_VALUE_PAIR:
      {
        MateConfValue         *car;
        MateConfValue         *cdr;
        const GVariantType *first_type;
        const GVariantType *second_type;
        GVariant           *tuple[2];
        GVariant           *result;

        if (!g_variant_type_is_tuple (expected_type) ||
            g_variant_type_n_items (expected_type) != 2)
          return NULL;

        car = mateconf_value_get_car (mateconf_value);
        cdr = mateconf_value_get_cdr (mateconf_value);
        first_type = g_variant_type_first (expected_type);
        second_type = g_variant_type_next (first_type);

        if (!mateconf_settings_backend_simple_mateconf_value_type_is_compatible (car->type, first_type) ||
            !mateconf_settings_backend_simple_mateconf_value_type_is_compatible (cdr->type, second_type))
          return NULL;

        tuple[0] = mateconf_settings_backend_simple_mateconf_value_type_to_gvariant (car, first_type);
        tuple[1] = mateconf_settings_backend_simple_mateconf_value_type_to_gvariant (cdr, second_type);

        result = g_variant_new_tuple (tuple, 2);
        return result;
      }
      break;
    default:
      return NULL;
    }

  g_assert_not_reached ();

  return NULL;
}

static MateConfValueType
mateconf_settings_backend_simple_gvariant_type_to_mateconf_value_type (const GVariantType *type)
{
  if (g_variant_type_equal (type, G_VARIANT_TYPE_BOOLEAN))
      return MATECONF_VALUE_BOOL;
  else if (g_variant_type_equal (type, G_VARIANT_TYPE_BYTE)     ||
           g_variant_type_equal (type, G_VARIANT_TYPE_INT16)    ||
           g_variant_type_equal (type, G_VARIANT_TYPE_UINT16)   ||
           g_variant_type_equal (type, G_VARIANT_TYPE_INT32)    ||
           g_variant_type_equal (type, G_VARIANT_TYPE_UINT32)   ||
           g_variant_type_equal (type, G_VARIANT_TYPE_INT64)    ||
           g_variant_type_equal (type, G_VARIANT_TYPE_UINT64)   ||
           g_variant_type_equal (type, G_VARIANT_TYPE_HANDLE))
      return MATECONF_VALUE_INT;
  else if (g_variant_type_equal (type, G_VARIANT_TYPE_DOUBLE))
      return MATECONF_VALUE_FLOAT;
  else if (g_variant_type_equal (type, G_VARIANT_TYPE_STRING)      ||
           g_variant_type_equal (type, G_VARIANT_TYPE_OBJECT_PATH) ||
           g_variant_type_equal (type, G_VARIANT_TYPE_SIGNATURE))
      return MATECONF_VALUE_STRING;

  return MATECONF_VALUE_INVALID;
}

static MateConfValue *
mateconf_settings_backend_simple_gvariant_to_mateconf_value (GVariant           *value,
                                                       const GVariantType *type)
{
  MateConfValue *mateconf_value = NULL;

  if (g_variant_type_equal (type, G_VARIANT_TYPE_BOOLEAN))
    {
      mateconf_value = mateconf_value_new (MATECONF_VALUE_BOOL);
      mateconf_value_set_bool (mateconf_value, g_variant_get_boolean (value));
    }
  else if (g_variant_type_equal (type, G_VARIANT_TYPE_BYTE))
    {
      guchar i = g_variant_get_byte (value);
      mateconf_value = mateconf_value_new (MATECONF_VALUE_INT);
      mateconf_value_set_int (mateconf_value, i);
    }
  else if (g_variant_type_equal (type, G_VARIANT_TYPE_INT16))
    {
      gint16 i = g_variant_get_int16 (value);
      mateconf_value = mateconf_value_new (MATECONF_VALUE_INT);
      mateconf_value_set_int (mateconf_value, i);
    }
  else if (g_variant_type_equal (type, G_VARIANT_TYPE_UINT16))
    {
      guint16 i = g_variant_get_uint16 (value);
      if (i > G_MAXINT)
        return NULL;
      mateconf_value = mateconf_value_new (MATECONF_VALUE_INT);
      mateconf_value_set_int (mateconf_value, i);
    }
  else if (g_variant_type_equal (type, G_VARIANT_TYPE_INT32))
    {
      gint32 i = g_variant_get_int32 (value);
      mateconf_value = mateconf_value_new (MATECONF_VALUE_INT);
      mateconf_value_set_int (mateconf_value, i);
    }
  else if (g_variant_type_equal (type, G_VARIANT_TYPE_UINT32))
    {
      guint32 i = g_variant_get_uint32 (value);
      if (i > G_MAXINT)
        return NULL;
      mateconf_value = mateconf_value_new (MATECONF_VALUE_INT);
      mateconf_value_set_int (mateconf_value, i);
    }
  else if (g_variant_type_equal (type, G_VARIANT_TYPE_INT64))
    {
      gint64 i = g_variant_get_int64 (value);
      if (i < G_MININT || i > G_MAXINT)
        return NULL;
      mateconf_value = mateconf_value_new (MATECONF_VALUE_INT);
      mateconf_value_set_int (mateconf_value, i);
    }
  else if (g_variant_type_equal (type, G_VARIANT_TYPE_UINT64))
    {
      guint64 i = g_variant_get_uint64 (value);
      if (i > G_MAXINT)
        return NULL;
      mateconf_value = mateconf_value_new (MATECONF_VALUE_INT);
      mateconf_value_set_int (mateconf_value, i);
    }
  else if (g_variant_type_equal (type, G_VARIANT_TYPE_HANDLE))
    {
      guint32 i = g_variant_get_handle (value);
      if (i > G_MAXINT)
        return NULL;
      mateconf_value = mateconf_value_new (MATECONF_VALUE_INT);
      mateconf_value_set_int (mateconf_value, i);
    }
  else if (g_variant_type_equal (type, G_VARIANT_TYPE_DOUBLE))
    {
      mateconf_value = mateconf_value_new (MATECONF_VALUE_FLOAT);
      mateconf_value_set_float (mateconf_value, g_variant_get_double (value));
    }
  else if (g_variant_type_equal (type, G_VARIANT_TYPE_STRING)      ||
           g_variant_type_equal (type, G_VARIANT_TYPE_OBJECT_PATH) ||
           g_variant_type_equal (type, G_VARIANT_TYPE_SIGNATURE))
    {
      mateconf_value = mateconf_value_new (MATECONF_VALUE_STRING);
      mateconf_value_set_string (mateconf_value, g_variant_get_string (value, NULL));
    }

  return mateconf_value;
}

static MateConfValue *
mateconf_settings_backend_gvariant_to_mateconf_value (GVariant *value)
{
  const GVariantType *type;
  MateConfValue         *mateconf_value = NULL;

  type = g_variant_get_type (value);
  if (g_variant_type_is_basic (type) &&
      !g_variant_type_equal (type, G_VARIANT_TYPE_BASIC))
    mateconf_value = mateconf_settings_backend_simple_gvariant_to_mateconf_value (value, type);
  else if (g_variant_type_is_array (type))
    {
      const GVariantType *array_type;
      array_type = g_variant_type_element (type);

      if (g_variant_type_is_basic (array_type) &&
          !g_variant_type_equal (array_type, G_VARIANT_TYPE_BASIC))
        {
          MateConfValueType  value_type;
          int             i;
          GSList        *list = NULL;

          for (i = 0; i < g_variant_n_children (value); i++)
            {
              MateConfValue *l;

              l = mateconf_settings_backend_simple_gvariant_to_mateconf_value (g_variant_get_child_value (value, i),
                                                                         array_type);
              list = g_slist_prepend (list, l);
            }

          list = g_slist_reverse (list);

          value_type = mateconf_settings_backend_simple_gvariant_type_to_mateconf_value_type (array_type);
          mateconf_value = mateconf_value_new (MATECONF_VALUE_LIST);
          mateconf_value_set_list_type (mateconf_value, value_type);
          mateconf_value_set_list (mateconf_value, list);

          g_slist_foreach (list, (GFunc) mateconf_value_free, NULL);
          g_slist_free (list);
        }
    }
  else if (g_variant_type_is_tuple (type) &&
            g_variant_type_n_items (type) == 2)
    {
      const GVariantType *first_type;
      const GVariantType *second_type;

      first_type = g_variant_type_first (type);
      second_type = g_variant_type_next (first_type);

      if (g_variant_type_is_basic (first_type) &&
          !g_variant_type_equal (first_type, G_VARIANT_TYPE_BASIC) &&
          g_variant_type_is_basic (second_type) &&
          !g_variant_type_equal (second_type, G_VARIANT_TYPE_BASIC))
        {
          MateConfValue *car;
          MateConfValue *cdr;

          mateconf_value = mateconf_value_new (MATECONF_VALUE_PAIR);

          car = mateconf_settings_backend_simple_gvariant_to_mateconf_value (g_variant_get_child_value (value, 0), first_type);
          cdr = mateconf_settings_backend_simple_gvariant_to_mateconf_value (g_variant_get_child_value (value, 1), second_type);

          if (car)
            mateconf_value_set_car_nocopy (mateconf_value, car);
          if (cdr)
            mateconf_value_set_cdr_nocopy (mateconf_value, cdr);

          if (car == NULL || cdr == NULL)
            {
              mateconf_value_free (mateconf_value);
              mateconf_value = NULL;
            }
        }
    }

  return mateconf_value;
}


/**************************\
 * Backend implementation *
\**************************/

static GVariant *
mateconf_settings_backend_read (GSettingsBackend   *backend,
                             const gchar        *key,
                             const GVariantType *expected_type,
                             gboolean            default_value)
{
  MateConfSettingsBackend *mateconf = MATECONF_SETTINGS_BACKEND (backend);
  MateConfValue *mateconf_value;
  GVariant *value;

  mateconf_value = mateconf_client_get_without_default (mateconf->priv->client,
                                                  key, NULL);
  if (mateconf_value == NULL)
    return NULL;

  value = mateconf_settings_backend_mateconf_value_to_gvariant (mateconf_value, expected_type);
  mateconf_value_free (mateconf_value);

  if (value != NULL)
    g_variant_ref_sink (value);

  return value;
}

static gboolean
mateconf_settings_backend_write (GSettingsBackend *backend,
                              const gchar      *key,
                              GVariant         *value,
                              gpointer          origin_tag)
{
  MateConfSettingsBackend *mateconf = MATECONF_SETTINGS_BACKEND (backend);
  MateConfValue           *mateconf_value;
  GError               *error;

  g_variant_ref_sink (value);
  mateconf_value = mateconf_settings_backend_gvariant_to_mateconf_value (value);
  g_variant_unref (value);
  if (mateconf_value == NULL)
    return FALSE;

  error = NULL;
  mateconf_client_set (mateconf->priv->client, key, mateconf_value, &error);
  mateconf_value_free (mateconf_value);

  if (error != NULL)
    {
      g_error_free (error);
      return FALSE;
    }

  g_settings_backend_changed (backend, key, origin_tag);

  g_hash_table_replace (mateconf->priv->ignore_notifications,
                        g_strdup (key), GINT_TO_POINTER (1));

  return TRUE;
}

static gboolean
mateconf_settings_backend_write_one_to_changeset (const gchar    *key,
                                               GVariant       *value,
                                               MateConfChangeSet *changeset)
{
  MateConfValue *mateconf_value;

  mateconf_value = mateconf_settings_backend_gvariant_to_mateconf_value (value);
  if (mateconf_value == NULL)
    return TRUE;

  mateconf_change_set_set_nocopy (changeset, key, mateconf_value);

  return FALSE;
}

static gboolean
mateconf_settings_backend_add_ignore_notifications (const gchar          *key,
                                                 GVariant             *value,
                                                 MateConfSettingsBackend *mateconf)
{
  g_hash_table_replace (mateconf->priv->ignore_notifications,
                        g_strdup (key), GINT_TO_POINTER (1));
  return FALSE;
}

static gboolean
mateconf_settings_backend_remove_ignore_notifications (MateConfChangeSet       *changeset,
                                                    const gchar          *key,
                                                    MateConfValue           *value,
                                                    MateConfSettingsBackend *mateconf)
{
  g_hash_table_remove (mateconf->priv->ignore_notifications, key);
  return FALSE;
}

static gboolean
mateconf_settings_backend_write_tree (GSettingsBackend *backend,
                                   GTree            *tree,
                                   gpointer          origin_tag)
{
  MateConfSettingsBackend *mateconf = MATECONF_SETTINGS_BACKEND (backend);
  MateConfChangeSet       *changeset;
  MateConfChangeSet       *reversed;
  gboolean              success;

  changeset = mateconf_change_set_new ();

  g_tree_foreach (tree, (GTraverseFunc) mateconf_settings_backend_write_one_to_changeset, changeset);

  if (mateconf_change_set_size (changeset) != g_tree_nnodes (tree))
    {
      mateconf_change_set_unref (changeset);
      return FALSE;
    }

  reversed = mateconf_client_reverse_change_set (mateconf->priv->client, changeset, NULL);
  success = mateconf_client_commit_change_set (mateconf->priv->client, changeset, TRUE, NULL);

  g_tree_foreach (tree, (GTraverseFunc) mateconf_settings_backend_add_ignore_notifications, mateconf);

  if (!success)
    {
      /* This is a tricky situation: when committing, some keys will have been
       * changed, so there will be notifications that we'll want to ignore. But
       * we can't ignore notifications for what was not committed. Note that
       * when we'll commit the reversed changeset, it should fail for the same
       * key, so there'll be no other notifications created. And in the worst
       * case, it's no big deal... */
      mateconf_change_set_foreach (changeset,
                                (MateConfChangeSetForeachFunc) mateconf_settings_backend_remove_ignore_notifications,
                                mateconf);
      mateconf_client_commit_change_set (mateconf->priv->client, reversed, FALSE, NULL);
    }
  else
    g_settings_backend_changed_tree (backend, tree, origin_tag);

  mateconf_change_set_unref (changeset);
  mateconf_change_set_unref (reversed);

  return success;
}

static void
mateconf_settings_backend_reset (GSettingsBackend *backend,
                              const gchar      *key,
                              gpointer          origin_tag)
{
  MateConfSettingsBackend *mateconf = MATECONF_SETTINGS_BACKEND (backend);

  if (mateconf_client_unset (mateconf->priv->client, key, NULL))
    g_settings_backend_changed (backend, key, origin_tag);
}

static gboolean
mateconf_settings_backend_get_writable (GSettingsBackend *backend,
                                     const gchar      *name)
{
  MateConfSettingsBackend *mateconf = MATECONF_SETTINGS_BACKEND (backend);
  MateConfValue *value;

  /* We don't support checking writabality for a whole subpath, so we just say
   * it's not writable in such a case. */
  if (name[strlen(name) - 1] == '/')
    return FALSE;

  value = mateconf_client_get (mateconf->priv->client, name, NULL);
  if (value == NULL)
    return TRUE;
  else
    mateconf_value_free (value);

  return mateconf_client_key_is_writable (mateconf->priv->client, name, NULL);
}

static char *
mateconf_settings_backend_get_mateconf_path_from_name (const gchar *name)
{
  /* We don't want trailing slash since mateconf directories shouldn't have a
   * trailing slash. */
  if (name[strlen(name) - 1] != '/')
    {
      const gchar *slash;
      slash = strrchr (name, '/');
      g_assert (slash != NULL);
      return g_strndup (name, slash - name);
    }
  else
    return g_strndup (name, strlen(name) - 1);
}

static void
mateconf_settings_backend_notified (MateConfClient          *client,
                                 guint                 cnxn_id,
                                 MateConfEntry           *entry,
                                 MateConfSettingsBackend *mateconf)
{
  if (g_hash_table_lookup_extended (mateconf->priv->ignore_notifications, entry->key,
                                    NULL, NULL))
    {
      g_hash_table_remove (mateconf->priv->ignore_notifications, entry->key);
      return;
    }

  g_settings_backend_changed (G_SETTINGS_BACKEND (mateconf), entry->key, NULL);
}

static void
mateconf_settings_backend_subscribe (GSettingsBackend *backend,
                                  const gchar      *name)
{
  MateConfSettingsBackend *mateconf = MATECONF_SETTINGS_BACKEND (backend);
  gchar                *path;

  path = mateconf_settings_backend_get_mateconf_path_from_name (name);
  if (mateconf_settings_backend_add_notifier (mateconf, path))
    mateconf_client_add_dir (mateconf->priv->client, path, MATECONF_CLIENT_PRELOAD_ONELEVEL, NULL);
  g_free (path);
}

static void
mateconf_settings_backend_unsubscribe (GSettingsBackend *backend,
                                    const gchar      *name)
{
  MateConfSettingsBackend *mateconf = MATECONF_SETTINGS_BACKEND (backend);
  gchar                *path;

  path = mateconf_settings_backend_get_mateconf_path_from_name (name);
  if (mateconf_settings_backend_remove_notifier (mateconf, path))
    mateconf_client_remove_dir (mateconf->priv->client, path, NULL);
  g_free (path);
}

static void
mateconf_settings_backend_finalize (GObject *object)
{
  MateConfSettingsBackend *mateconf = MATECONF_SETTINGS_BACKEND (object);

  g_slist_foreach (mateconf->priv->notifiers, (GFunc) mateconf_settings_backend_free_notifier, mateconf);
  g_slist_free (mateconf->priv->notifiers);
  mateconf->priv->notifiers = NULL;

  g_object_unref (mateconf->priv->client);
  mateconf->priv->client = NULL;

  g_hash_table_unref (mateconf->priv->ignore_notifications);
  mateconf->priv->ignore_notifications = NULL;

  G_OBJECT_CLASS (mateconf_settings_backend_parent_class)
    ->finalize (object);
}

static void
mateconf_settings_backend_init (MateConfSettingsBackend *mateconf)
{
  mateconf->priv = G_TYPE_INSTANCE_GET_PRIVATE (mateconf,
                                             MATECONF_TYPE_SETTINGS_BACKEND,
                                             MateConfSettingsBackendPrivate);
  mateconf->priv->client = mateconf_client_get_default ();
  mateconf->priv->notifiers = NULL;
  mateconf->priv->ignore_notifications = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                             g_free, NULL);
}

static void
mateconf_settings_backend_class_finalize (MateConfSettingsBackendClass *class)
{                               
}

static void
mateconf_settings_backend_class_init (MateConfSettingsBackendClass *class)
{
  GSettingsBackendClass *backend_class = G_SETTINGS_BACKEND_CLASS (class);
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = mateconf_settings_backend_finalize;

  backend_class->read = mateconf_settings_backend_read;
  backend_class->write = mateconf_settings_backend_write;
  backend_class->write_tree = mateconf_settings_backend_write_tree;
  backend_class->reset = mateconf_settings_backend_reset;
  backend_class->get_writable = mateconf_settings_backend_get_writable;
  backend_class->subscribe = mateconf_settings_backend_subscribe;
  backend_class->unsubscribe = mateconf_settings_backend_unsubscribe;

  g_type_class_add_private (class, sizeof (MateConfSettingsBackendPrivate));
}

void 
mateconf_settings_backend_register (GIOModule *module)
{
  mateconf_settings_backend_register_type (G_TYPE_MODULE (module));
  g_io_extension_point_implement (G_SETTINGS_BACKEND_EXTENSION_POINT_NAME,
                                  MATECONF_TYPE_SETTINGS_BACKEND,
                                  "mateconf",
                                  -1);
}
