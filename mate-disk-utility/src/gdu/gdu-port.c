/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-port.c
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
#include "gdu-port.h"
#include "udisks-port-glue.h"

/* --- SUCKY CODE BEGIN --- */

/* This totally sucks; dbus-bindings-tool and dbus-glib should be able
 * to do this for us.
 *
 * TODO: keep in sync with code in tools/udisks in udisks.
 */

typedef struct
{
        gchar *native_path;

        gchar *adapter;
        gchar *parent;
        gint number;
        gchar *connector_type;
} PortProperties;

static void
collect_props (const char *key, const GValue *value, PortProperties *props)
{
        gboolean handled = TRUE;

        if (strcmp (key, "NativePath") == 0)
                props->native_path = g_strdup (g_value_get_string (value));

        else if (strcmp (key, "Adapter") == 0)
                props->adapter = g_value_dup_boxed (value);
        else if (strcmp (key, "Parent") == 0)
                props->parent = g_value_dup_boxed (value);
        else if (strcmp (key, "Number") == 0)
                props->number = g_value_get_int (value);
        else if (strcmp (key, "ConnectorType") == 0)
                props->connector_type = g_value_dup_string (value);
        else
                handled = FALSE;

        if (!handled)
                g_warning ("unhandled property '%s'", key);
}

static void
port_properties_free (PortProperties *props)
{
        g_free (props->native_path);
        g_free (props->adapter);
        g_free (props->parent);
        g_free (props->connector_type);
        g_free (props);
}

static PortProperties *
port_properties_get (DBusGConnection *bus,
                     const char *object_path)
{
        PortProperties *props;
        GError *error;
        GHashTable *hash_table;
        DBusGProxy *prop_proxy;
        const char *ifname = "org.freedesktop.UDisks.Port";

        props = g_new0 (PortProperties, 1);

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

                port_properties_free (props);
                props = NULL;
                goto out;
        }

        g_hash_table_foreach (hash_table, (GHFunc) collect_props, props);

        g_hash_table_unref (hash_table);

#if 0
        g_print ("----------------------------------------------------------------------\n");
        g_print ("native_path:    %s\n", props->native_path);
        g_print ("adapter:        %s\n", props->adapter);
        g_print ("parent:         %s\n", props->parent);
        g_print ("number:         %d\n", props->number);
        g_print ("connector_type: %s\n", props->connector_type);
#endif

out:
        g_object_unref (prop_proxy);
        return props;
}

/* --- SUCKY CODE END --- */

struct _GduPortPrivate
{
        DBusGProxy *proxy;
        GduPool *pool;

        char *object_path;

        PortProperties *props;
};

enum {
        CHANGED,
        REMOVED,
        LAST_SIGNAL,
};

static GObjectClass *parent_class = NULL;
static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (GduPort, gdu_port, G_TYPE_OBJECT);

GduPool *
gdu_port_get_pool (GduPort *port)
{
        return g_object_ref (port->priv->pool);
}

static void
gdu_port_finalize (GduPort *port)
{
        g_debug ("##### finalized port %s",
                 port->priv->props != NULL ? port->priv->props->native_path : port->priv->object_path);

        g_free (port->priv->object_path);
        if (port->priv->proxy != NULL)
                g_object_unref (port->priv->proxy);
        if (port->priv->pool != NULL)
                g_object_unref (port->priv->pool);
        if (port->priv->props != NULL)
                port_properties_free (port->priv->props);

        if (G_OBJECT_CLASS (parent_class)->finalize)
                (* G_OBJECT_CLASS (parent_class)->finalize) (G_OBJECT (port));
}

static void
gdu_port_class_init (GduPortClass *klass)
{
        GObjectClass *obj_class = (GObjectClass *) klass;

        parent_class = g_type_class_peek_parent (klass);

        obj_class->finalize = (GObjectFinalizeFunc) gdu_port_finalize;

        g_type_class_add_private (klass, sizeof (GduPortPrivate));

        signals[CHANGED] =
                g_signal_new ("changed",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GduPortClass, changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE, 0);
        signals[REMOVED] =
                g_signal_new ("removed",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GduPortClass, removed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE, 0);
}

static void
gdu_port_init (GduPort *port)
{
        port->priv = G_TYPE_INSTANCE_GET_PRIVATE (port, GDU_TYPE_PORT, GduPortPrivate);
}

static gboolean
update_info (GduPort *port)
{
        PortProperties *new_properties;

        new_properties = port_properties_get (_gdu_pool_get_connection (port->priv->pool),
                                              port->priv->object_path);
        if (new_properties != NULL) {
                if (port->priv->props != NULL)
                        port_properties_free (port->priv->props);
                port->priv->props = new_properties;
                return TRUE;
        } else {
                return FALSE;
        }
}


GduPort *
_gdu_port_new_from_object_path (GduPool *pool, const char *object_path)
{
        GduPort *port;

        port = GDU_PORT (g_object_new (GDU_TYPE_PORT, NULL));
        port->priv->object_path = g_strdup (object_path);
        port->priv->pool = g_object_ref (pool);

	port->priv->proxy = dbus_g_proxy_new_for_name (_gdu_pool_get_connection (port->priv->pool),
                                                       "org.freedesktop.UDisks",
                                                       port->priv->object_path,
                                                       "org.freedesktop.UDisks.Port");
        dbus_g_proxy_set_default_timeout (port->priv->proxy, INT_MAX);
        dbus_g_proxy_add_signal (port->priv->proxy, "Changed", G_TYPE_INVALID);

        /* TODO: connect signals */

        if (!update_info (port))
                goto error;

        g_debug ("_gdu_port_new_from_object_path: %s", port->priv->props->native_path);

        return port;
error:
        g_object_unref (port);
        return NULL;
}

gboolean
_gdu_port_changed (GduPort *port)
{
        g_debug ("_gdu_port_changed: %s", port->priv->props->native_path);
        if (update_info (port)) {
                g_signal_emit (port, signals[CHANGED], 0);
                return TRUE;
        } else {
                return FALSE;
        }
}

const gchar *
gdu_port_get_object_path (GduPort *port)
{
        return port->priv->object_path;
}


const gchar *
gdu_port_get_native_path (GduPort *port)
{
        return port->priv->props->native_path;
}

const gchar *
gdu_port_get_adapter (GduPort *port)
{
        return port->priv->props->adapter;
}

const gchar *
gdu_port_get_parent (GduPort *port)
{
        return port->priv->props->parent;
}

gint
gdu_port_get_number (GduPort *port)
{
        return port->priv->props->number;
}

const gchar *
gdu_port_get_connector_type (GduPort *port)
{
        return port->priv->props->connector_type;
}
