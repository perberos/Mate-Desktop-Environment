/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007-2008 William Jon McCann <mccann@jhu.edu>
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
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>

#include "gsm-store.h"

#define GSM_STORE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GSM_TYPE_STORE, GsmStorePrivate))

struct GsmStorePrivate
{
        GHashTable *objects;
        gboolean    locked;
};

enum {
        ADDED,
        REMOVED,
        LAST_SIGNAL
};

enum {
        PROP_0,
        PROP_LOCKED
};

static guint signals [LAST_SIGNAL] = { 0 };

static void     gsm_store_class_init    (GsmStoreClass *klass);
static void     gsm_store_init          (GsmStore      *store);
static void     gsm_store_finalize      (GObject       *object);

G_DEFINE_TYPE (GsmStore, gsm_store, G_TYPE_OBJECT)

GQuark
gsm_store_error_quark (void)
{
        static GQuark ret = 0;
        if (ret == 0) {
                ret = g_quark_from_static_string ("gsm_store_error");
        }

        return ret;
}

guint
gsm_store_size (GsmStore    *store)
{
        return g_hash_table_size (store->priv->objects);
}

gboolean
gsm_store_remove (GsmStore   *store,
                  const char *id)
{
        GObject *found;
        gboolean removed;
        char    *id_copy;

        g_return_val_if_fail (store != NULL, FALSE);

        found = g_hash_table_lookup (store->priv->objects, id);
        if (found == NULL) {
                return FALSE;
        }

        id_copy = g_strdup (id);

        g_object_ref (found);

        removed = g_hash_table_remove (store->priv->objects, id_copy);
        g_assert (removed);

        g_signal_emit (store, signals [REMOVED], 0, id_copy);

        g_object_unref (found);
        g_free (id_copy);

        return TRUE;
}

void
gsm_store_foreach (GsmStore    *store,
                   GsmStoreFunc func,
                   gpointer     user_data)
{
        g_return_if_fail (store != NULL);
        g_return_if_fail (func != NULL);

        g_hash_table_find (store->priv->objects,
                           (GHRFunc)func,
                           user_data);
}

GObject *
gsm_store_find (GsmStore    *store,
                GsmStoreFunc predicate,
                gpointer     user_data)
{
        GObject *object;

        g_return_val_if_fail (store != NULL, NULL);
        g_return_val_if_fail (predicate != NULL, NULL);

        object = g_hash_table_find (store->priv->objects,
                                    (GHRFunc)predicate,
                                    user_data);
        return object;
}

GObject *
gsm_store_lookup (GsmStore   *store,
                  const char *id)
{
        GObject *object;

        g_return_val_if_fail (store != NULL, NULL);
        g_return_val_if_fail (id != NULL, NULL);

        object = g_hash_table_lookup (store->priv->objects, id);

        return object;
}


typedef struct
{
        GsmStoreFunc func;
        gpointer     user_data;
        GsmStore    *store;
        GList       *removed;
} WrapperData;

static gboolean
foreach_remove_wrapper (const char  *id,
                        GObject     *object,
                        WrapperData *data)
{
        gboolean res;

        res = (data->func) (id, object, data->user_data);
        if (res) {
                data->removed = g_list_prepend (data->removed, g_strdup (id));
        }

        return res;
}

guint
gsm_store_foreach_remove (GsmStore    *store,
                          GsmStoreFunc func,
                          gpointer     user_data)
{
        guint       ret;
        WrapperData data;

        g_return_val_if_fail (store != NULL, 0);
        g_return_val_if_fail (func != NULL, 0);

        data.store = store;
        data.user_data = user_data;
        data.func = func;
        data.removed = NULL;

        ret = g_hash_table_foreach_remove (store->priv->objects,
                                           (GHRFunc)foreach_remove_wrapper,
                                           &data);

        while (data.removed != NULL) {
                char *id;
                id = data.removed->data;
                g_debug ("GsmStore: emitting removed for %s", id);
                g_signal_emit (store, signals [REMOVED], 0, id);
                g_free (data.removed->data);
                data.removed->data = NULL;
                data.removed = g_list_delete_link (data.removed, data.removed);
        }

        return ret;
}

static gboolean
_remove_all (const char *id,
             GObject    *object,
             gpointer    data)
{
        return TRUE;
}

void
gsm_store_clear (GsmStore *store)
{
        g_return_if_fail (store != NULL);

        g_debug ("GsmStore: Clearing object store");

        gsm_store_foreach_remove (store,
                                  _remove_all,
                                  NULL);
}

gboolean
gsm_store_add (GsmStore   *store,
               const char *id,
               GObject    *object)
{
        g_return_val_if_fail (store != NULL, FALSE);
        g_return_val_if_fail (id != NULL, FALSE);
        g_return_val_if_fail (object != NULL, FALSE);

        /* If we're locked, we don't accept any new session
           objects. */
        if (store->priv->locked) {
                return FALSE;
        }

        g_debug ("GsmStore: Adding object id %s to store", id);

        g_hash_table_insert (store->priv->objects,
                             g_strdup (id),
                             g_object_ref (object));

        g_signal_emit (store, signals [ADDED], 0, id);

        return TRUE;
}

void
gsm_store_set_locked (GsmStore *store,
                      gboolean  locked)
{
        g_return_if_fail (GSM_IS_STORE (store));

        store->priv->locked = locked;
}

gboolean
gsm_store_get_locked (GsmStore *store)
{
        g_return_val_if_fail (GSM_IS_STORE (store), FALSE);

        return store->priv->locked;
}

static void
gsm_store_set_property (GObject      *object,
                        guint         prop_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
        GsmStore *self;

        self = GSM_STORE (object);

        switch (prop_id) {
        case PROP_LOCKED:
                gsm_store_set_locked (self, g_value_get_boolean (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gsm_store_get_property (GObject    *object,
                        guint       prop_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
        GsmStore *self;

        self = GSM_STORE (object);

        switch (prop_id) {
        case PROP_LOCKED:
                g_value_set_boolean (value, self->priv->locked);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gsm_store_dispose (GObject *object)
{
        GsmStore *store;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GSM_IS_STORE (object));

        store = GSM_STORE (object);

        gsm_store_clear (store);

        G_OBJECT_CLASS (gsm_store_parent_class)->dispose (object);
}

static void
gsm_store_class_init (GsmStoreClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = gsm_store_get_property;
        object_class->set_property = gsm_store_set_property;
        object_class->finalize = gsm_store_finalize;
        object_class->dispose = gsm_store_dispose;

        signals [ADDED] =
                g_signal_new ("added",
                              G_TYPE_FROM_CLASS (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GsmStoreClass, added),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1, G_TYPE_STRING);
        signals [REMOVED] =
                g_signal_new ("removed",
                              G_TYPE_FROM_CLASS (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GsmStoreClass, removed),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1, G_TYPE_STRING);
        g_object_class_install_property (object_class,
                                         PROP_LOCKED,
                                         g_param_spec_boolean ("locked",
                                                               NULL,
                                                               NULL,
                                                               FALSE,
                                                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

        g_type_class_add_private (klass, sizeof (GsmStorePrivate));
}

static void
_destroy_object (GObject *object)
{
        g_debug ("GsmStore: Unreffing object: %p", object);
        g_object_unref (object);
}

static void
gsm_store_init (GsmStore *store)
{

        store->priv = GSM_STORE_GET_PRIVATE (store);

        store->priv->objects = g_hash_table_new_full (g_str_hash,
                                                      g_str_equal,
                                                      g_free,
                                                      (GDestroyNotify) _destroy_object);
}

static void
gsm_store_finalize (GObject *object)
{
        GsmStore *store;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GSM_IS_STORE (object));

        store = GSM_STORE (object);

        g_return_if_fail (store->priv != NULL);

        g_hash_table_destroy (store->priv->objects);

        G_OBJECT_CLASS (gsm_store_parent_class)->finalize (object);
}

GsmStore *
gsm_store_new (void)
{
        GObject *object;

        object = g_object_new (GSM_TYPE_STORE,
                               NULL);

        return GSM_STORE (object);
}
