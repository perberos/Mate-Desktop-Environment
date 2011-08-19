/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-adapter.c
 *
 * Copyright (C) 2009 David Zeuthen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include <stdlib.h>
#include <string.h>
#include <dbus/dbus-glib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>

#include "gdu-private.h"
#include "gdu-pool.h"
#include "gdu-adapter.h"
#include "udisks-adapter-glue.h"

/* --- SUCKY CODE BEGIN --- */

/* This totally sucks; dbus-bindings-tool and dbus-glib should be able
 * to do this for us.
 *
 * TODO: keep in sync with code in tools/udisks in udisks.
 */

typedef struct
{
        gchar *native_path;

        gchar *vendor;
        gchar *model;
        gchar *driver;
        gchar *fabric;
        guint num_ports;
} AdapterProperties;

static void
collect_props (const char *key, const GValue *value, AdapterProperties *props)
{
        gboolean handled = TRUE;

        if (strcmp (key, "NativePath") == 0)
                props->native_path = g_strdup (g_value_get_string (value));

        else if (strcmp (key, "Vendor") == 0)
                props->vendor = g_value_dup_string (value);
        else if (strcmp (key, "Model") == 0)
                props->model = g_value_dup_string (value);
        else if (strcmp (key, "Driver") == 0)
                props->driver = g_value_dup_string (value);
        else if (strcmp (key, "Fabric") == 0)
                props->fabric = g_value_dup_string (value);
        else if (strcmp (key, "NumPorts") == 0)
                props->num_ports = g_value_get_uint (value);
        else
                handled = FALSE;

        if (!handled)
                g_warning ("unhandled property '%s'", key);
}

static void
adapter_properties_free (AdapterProperties *props)
{
        g_free (props->native_path);
        g_free (props->vendor);
        g_free (props->model);
        g_free (props->driver);
        g_free (props->fabric);
        g_free (props);
}

static AdapterProperties *
adapter_properties_get (DBusGConnection *bus,
                        const char *object_path)
{
        AdapterProperties *props;
        GError *error;
        GHashTable *hash_table;
        DBusGProxy *prop_proxy;
        const char *ifname = "org.freedesktop.UDisks.Adapter";

        props = g_new0 (AdapterProperties, 1);

	prop_proxy = dbus_g_proxy_new_for_name (bus,
                                                "org.freedesktop.UDisks",
                                                object_path,
                                                "org.freedesktop.DBus.Properties");
        error = NULL;
        if (!dbus_g_proxy_call (prop_proxy,
                                "GetAll",
                                &error,
                                G_TYPE_STRING,
                                ifname,
                                G_TYPE_INVALID,
                                dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_VALUE),
                                &hash_table,
                                G_TYPE_INVALID)) {
                g_warning ("Couldn't call GetAll() to get properties for %s: %s", object_path, error->message);
                g_error_free (error);

                adapter_properties_free (props);
                props = NULL;
                goto out;
        }

        g_hash_table_foreach (hash_table, (GHFunc) collect_props, props);

        g_hash_table_unref (hash_table);

#if 0
        g_print ("----------------------------------------------------------------------\n");
        g_print ("native_path: %s\n", props->native_path);
        g_print ("vendor:      %s\n", props->vendor);
        g_print ("model:       %s\n", props->model);
        g_print ("driver:      %s\n", props->driver);
        g_print ("fabric:      %s\n", props->fabric);
        g_print ("num_ports:   %d\n", props->num_ports);
#endif

out:
        g_object_unref (prop_proxy);
        return props;
}

/* --- SUCKY CODE END --- */

struct _GduAdapterPrivate
{
        DBusGProxy *proxy;
        GduPool *pool;

        char *object_path;

        AdapterProperties *props;
};

enum {
        CHANGED,
        REMOVED,
        LAST_SIGNAL,
};

static GObjectClass *parent_class = NULL;
static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (GduAdapter, gdu_adapter, G_TYPE_OBJECT);

GduPool *
gdu_adapter_get_pool (GduAdapter *adapter)
{
        return g_object_ref (adapter->priv->pool);
}

static void
gdu_adapter_finalize (GduAdapter *adapter)
{
        g_debug ("##### finalized adapter %s",
                 adapter->priv->props != NULL ? adapter->priv->props->native_path : adapter->priv->object_path);

        g_free (adapter->priv->object_path);
        if (adapter->priv->proxy != NULL)
                g_object_unref (adapter->priv->proxy);
        if (adapter->priv->pool != NULL)
                g_object_unref (adapter->priv->pool);
        if (adapter->priv->props != NULL)
                adapter_properties_free (adapter->priv->props);

        if (G_OBJECT_CLASS (parent_class)->finalize)
                (* G_OBJECT_CLASS (parent_class)->finalize) (G_OBJECT (adapter));
}

static void
gdu_adapter_class_init (GduAdapterClass *klass)
{
        GObjectClass *obj_class = (GObjectClass *) klass;

        parent_class = g_type_class_peek_parent (klass);

        obj_class->finalize = (GObjectFinalizeFunc) gdu_adapter_finalize;

        g_type_class_add_private (klass, sizeof (GduAdapterPrivate));

        signals[CHANGED] =
                g_signal_new ("changed",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GduAdapterClass, changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE, 0);
        signals[REMOVED] =
                g_signal_new ("removed",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GduAdapterClass, removed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE, 0);
}

static void
gdu_adapter_init (GduAdapter *adapter)
{
        adapter->priv = G_TYPE_INSTANCE_GET_PRIVATE (adapter, GDU_TYPE_ADAPTER, GduAdapterPrivate);
}

static gboolean
update_info (GduAdapter *adapter)
{
        AdapterProperties *new_properties;

        new_properties = adapter_properties_get (_gdu_pool_get_connection (adapter->priv->pool),
                                                 adapter->priv->object_path);
        if (new_properties != NULL) {
                if (adapter->priv->props != NULL)
                        adapter_properties_free (adapter->priv->props);
                adapter->priv->props = new_properties;
                return TRUE;
        } else {
                return FALSE;
        }
}


GduAdapter *
_gdu_adapter_new_from_object_path (GduPool *pool, const char *object_path)
{
        GduAdapter *adapter;

        adapter = GDU_ADAPTER (g_object_new (GDU_TYPE_ADAPTER, NULL));
        adapter->priv->object_path = g_strdup (object_path);
        adapter->priv->pool = g_object_ref (pool);

	adapter->priv->proxy = dbus_g_proxy_new_for_name (_gdu_pool_get_connection (adapter->priv->pool),
                                                          "org.freedesktop.UDisks",
                                                          adapter->priv->object_path,
                                                          "org.freedesktop.UDisks.Adapter");
        dbus_g_proxy_set_default_timeout (adapter->priv->proxy, INT_MAX);
        dbus_g_proxy_add_signal (adapter->priv->proxy, "Changed", G_TYPE_INVALID);

        /* TODO: connect signals */

        if (!update_info (adapter))
                goto error;

        g_debug ("_gdu_adapter_new_from_object_path: %s", adapter->priv->props->native_path);

        return adapter;
error:
        g_object_unref (adapter);
        return NULL;
}

gboolean
_gdu_adapter_changed (GduAdapter *adapter)
{
        g_debug ("_gdu_adapter_changed: %s", adapter->priv->props->native_path);
        if (update_info (adapter)) {
                g_signal_emit (adapter, signals[CHANGED], 0);
                return TRUE;
        } else {
                return FALSE;
        }
}

const gchar *
gdu_adapter_get_object_path (GduAdapter *adapter)
{
        return adapter->priv->object_path;
}


const gchar *
gdu_adapter_get_native_path (GduAdapter *adapter)
{
        return adapter->priv->props->native_path;
}

const gchar *
gdu_adapter_get_vendor (GduAdapter *adapter)
{
        return adapter->priv->props->vendor;
}

const gchar *
gdu_adapter_get_model (GduAdapter *adapter)
{
        return adapter->priv->props->model;
}

const gchar *
gdu_adapter_get_driver (GduAdapter *adapter)
{
        return adapter->priv->props->driver;
}

const gchar *
gdu_adapter_get_fabric (GduAdapter *adapter)
{
        return adapter->priv->props->fabric;
}

guint
gdu_adapter_get_num_ports (GduAdapter *adapter)
{
        return adapter->priv->props->num_ports;
}
