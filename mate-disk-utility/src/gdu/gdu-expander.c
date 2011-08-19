/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-expander.c
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
#include "gdu-expander.h"
#include "udisks-expander-glue.h"

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
        gchar *revision;
        guint num_ports;
        gchar **upstream_ports;
        gchar *adapter;
} ExpanderProperties;

static void
collect_props (const char *key, const GValue *value, ExpanderProperties *props)
{
        gboolean handled = TRUE;

        if (strcmp (key, "NativePath") == 0)
                props->native_path = g_strdup (g_value_get_string (value));

        else if (strcmp (key, "Vendor") == 0)
                props->vendor = g_value_dup_string (value);
        else if (strcmp (key, "Model") == 0)
                props->model = g_value_dup_string (value);
        else if (strcmp (key, "Revision") == 0)
                props->revision = g_value_dup_string (value);
        else if (strcmp (key, "NumPorts") == 0)
                props->num_ports = g_value_get_uint (value);
        else if (strcmp (key, "UpstreamPorts") == 0) {
                guint n;
                GPtrArray *object_paths;

                object_paths = g_value_get_boxed (value);

                props->upstream_ports = g_new0 (char *, object_paths->len + 1);
                for (n = 0; n < object_paths->len; n++)
                        props->upstream_ports[n] = g_strdup (object_paths->pdata[n]);
                props->upstream_ports[n] = NULL;
        }
        else if (strcmp (key, "Adapter") == 0)
                props->adapter = g_value_dup_boxed (value);

        else
                handled = FALSE;

        if (!handled)
                g_warning ("unhandled property '%s'", key);
}

static void
expander_properties_free (ExpanderProperties *props)
{
        g_free (props->native_path);
        g_free (props->vendor);
        g_free (props->model);
        g_free (props->revision);
        g_free (props->adapter);
        g_strfreev (props->upstream_ports);
        g_free (props);
}

static ExpanderProperties *
expander_properties_get (DBusGConnection *bus,
                           const char *object_path)
{
        ExpanderProperties *props;
        GError *error;
        GHashTable *hash_table;
        DBusGProxy *prop_proxy;
        const char *ifname = "org.freedesktop.UDisks.Expander";

        props = g_new0 (ExpanderProperties, 1);

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

                expander_properties_free (props);
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
        g_print ("revision:    %s\n", props->revision);
        g_print ("adapter:     %s\n", props->adapter);
        g_print ("num_ports:   %d\n", props->num_ports);
        g_print ("upstream_ports:\n");
        {
                guint n;
                for (n = 0; props->upstream_ports != NULL && props->upstream_ports[n] != NULL; n++) {
                        g_print ("  %s\n", props->upstream_ports[n]);
                }
        }
#endif

out:
        g_object_unref (prop_proxy);
        return props;
}

/* --- SUCKY CODE END --- */

struct _GduExpanderPrivate
{
        DBusGProxy *proxy;
        GduPool *pool;

        char *object_path;

        ExpanderProperties *props;
};

enum {
        CHANGED,
        REMOVED,
        LAST_SIGNAL,
};

static GObjectClass *parent_class = NULL;
static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (GduExpander, gdu_expander, G_TYPE_OBJECT);

GduPool *
gdu_expander_get_pool (GduExpander *expander)
{
        return g_object_ref (expander->priv->pool);
}

static void
gdu_expander_finalize (GduExpander *expander)
{
        g_debug ("##### finalized expander %s",
                 expander->priv->props != NULL ? expander->priv->props->native_path : expander->priv->object_path);

        g_free (expander->priv->object_path);
        if (expander->priv->proxy != NULL)
                g_object_unref (expander->priv->proxy);
        if (expander->priv->pool != NULL)
                g_object_unref (expander->priv->pool);
        if (expander->priv->props != NULL)
                expander_properties_free (expander->priv->props);

        if (G_OBJECT_CLASS (parent_class)->finalize)
                (* G_OBJECT_CLASS (parent_class)->finalize) (G_OBJECT (expander));
}

static void
gdu_expander_class_init (GduExpanderClass *klass)
{
        GObjectClass *obj_class = (GObjectClass *) klass;

        parent_class = g_type_class_peek_parent (klass);

        obj_class->finalize = (GObjectFinalizeFunc) gdu_expander_finalize;

        g_type_class_add_private (klass, sizeof (GduExpanderPrivate));

        signals[CHANGED] =
                g_signal_new ("changed",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GduExpanderClass, changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE, 0);
        signals[REMOVED] =
                g_signal_new ("removed",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GduExpanderClass, removed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE, 0);
}

static void
gdu_expander_init (GduExpander *expander)
{
        expander->priv = G_TYPE_INSTANCE_GET_PRIVATE (expander, GDU_TYPE_EXPANDER, GduExpanderPrivate);
}

static gboolean
update_info (GduExpander *expander)
{
        ExpanderProperties *new_properties;

        new_properties = expander_properties_get (_gdu_pool_get_connection (expander->priv->pool),
                                                  expander->priv->object_path);
        if (new_properties != NULL) {
                if (expander->priv->props != NULL)
                        expander_properties_free (expander->priv->props);
                expander->priv->props = new_properties;
                return TRUE;
        } else {
                return FALSE;
        }
}


GduExpander *
_gdu_expander_new_from_object_path (GduPool *pool, const char *object_path)
{
        GduExpander *expander;

        expander = GDU_EXPANDER (g_object_new (GDU_TYPE_EXPANDER, NULL));
        expander->priv->object_path = g_strdup (object_path);
        expander->priv->pool = g_object_ref (pool);

	expander->priv->proxy = dbus_g_proxy_new_for_name (_gdu_pool_get_connection (expander->priv->pool),
                                                           "org.freedesktop.UDisks",
                                                           expander->priv->object_path,
                                                           "org.freedesktop.UDisks.Expander");
        dbus_g_proxy_set_default_timeout (expander->priv->proxy, INT_MAX);
        dbus_g_proxy_add_signal (expander->priv->proxy, "Changed", G_TYPE_INVALID);

        /* TODO: connect signals */

        if (!update_info (expander))
                goto error;

        g_debug ("_gdu_expander_new_from_object_path: %s", expander->priv->props->native_path);

        return expander;
error:
        g_object_unref (expander);
        return NULL;
}

gboolean
_gdu_expander_changed (GduExpander *expander)
{
        g_debug ("_gdu_expander_changed: %s", expander->priv->props->native_path);
        if (update_info (expander)) {
                g_signal_emit (expander, signals[CHANGED], 0);
                return TRUE;
        } else {
                return FALSE;
        }
}

const gchar *
gdu_expander_get_object_path (GduExpander *expander)
{
        return expander->priv->object_path;
}


const gchar *
gdu_expander_get_native_path (GduExpander *expander)
{
        return expander->priv->props->native_path;
}

const gchar *
gdu_expander_get_vendor (GduExpander *expander)
{
        return expander->priv->props->vendor;
}

const gchar *
gdu_expander_get_model (GduExpander *expander)
{
        return expander->priv->props->model;
}

const gchar *
gdu_expander_get_revision (GduExpander *expander)
{
        return expander->priv->props->revision;
}

guint
gdu_expander_get_num_ports (GduExpander *expander)
{
        return expander->priv->props->num_ports;
}

gchar **
gdu_expander_get_upstream_ports (GduExpander *expander)
{
        return expander->priv->props->upstream_ports;
}

const gchar *
gdu_expander_get_adapter (GduExpander *expander)
{
        return expander->priv->props->adapter;
}

