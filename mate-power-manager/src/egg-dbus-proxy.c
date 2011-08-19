/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2006-2008 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU General Public License Version 2
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <dbus/dbus-glib.h>

#include "egg-debug.h"
#include "egg-dbus-monitor.h"
#include "egg-dbus-proxy.h"

static void     egg_dbus_proxy_finalize   (GObject        *object);

#define EGG_DBUS_PROXY_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), EGG_TYPE_DBUS_PROXY, EggDbusProxyPrivate))

/* this is a managed proxy, i.e. a proxy that handles messagebus and DBUS service restarts. */
struct EggDbusProxyPrivate
{
	gchar			*service;
	gchar			*interface;
	gchar			*path;
	DBusGProxy		*proxy;
	EggDbusMonitor		*monitor;
	gboolean		 assigned;
	DBusGConnection		*connection;
	gulong			 monitor_callback_id;
};

enum {
	PROXY_STATUS,
	LAST_SIGNAL
};

static guint	     signals [LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (EggDbusProxy, egg_dbus_proxy, G_TYPE_OBJECT)

/**
 * egg_dbus_proxy_connect:
 * @proxy: This class instance
 * Return value: success
 **/
static gboolean
egg_dbus_proxy_connect (EggDbusProxy *proxy)
{
	GError *error = NULL;

	g_return_val_if_fail (EGG_IS_DBUS_PROXY (proxy), FALSE);

	/* are already connected? */
	if (proxy->priv->proxy != NULL) {
		egg_debug ("already connected to %s", proxy->priv->service);
		return FALSE;
	}

	proxy->priv->proxy = dbus_g_proxy_new_for_name_owner (proxy->priv->connection,
							      proxy->priv->service,
							      proxy->priv->path,
							      proxy->priv->interface,
							      &error);
	/* check for any possible error */
	if (error) {
		egg_warning ("DBUS error: %s", error->message);
		g_error_free (error);
		proxy->priv->proxy = NULL;
	}

	/* shouldn't be, but make sure proxy valid */
	if (proxy->priv->proxy == NULL) {
		egg_debug ("proxy is NULL, maybe the daemon responsible "
			   "for %s is not running?", proxy->priv->service);
		return FALSE;
	}

	g_signal_emit (proxy, signals [PROXY_STATUS], 0, TRUE);

	return TRUE;
}

/**
 * egg_dbus_proxy_disconnect:
 * @proxy: This class instance
 * Return value: success
 **/
static gboolean
egg_dbus_proxy_disconnect (EggDbusProxy *proxy)
{
	g_return_val_if_fail (EGG_IS_DBUS_PROXY (proxy), FALSE);

	/* are already disconnected? */
	if (proxy->priv->proxy == NULL) {
		if (proxy->priv->service)
			egg_debug ("already disconnected from %s", proxy->priv->service);
		else 
			egg_debug ("already disconnected.");
		return FALSE;
	}

	g_signal_emit (proxy, signals [PROXY_STATUS], 0, FALSE);

	g_object_unref (proxy->priv->proxy);
	proxy->priv->proxy = NULL;

	return TRUE;
}

/**
 * dbus_monitor_connection_cb:
 * @proxy: The dbus raw proxy
 * @status: The status of the service, where TRUE is connected
 * @screensaver: This class instance
 **/
static void
dbus_monitor_connection_cb (EggDbusMonitor *monitor, gboolean status, EggDbusProxy *proxy)
{
	g_return_if_fail (EGG_IS_DBUS_PROXY (proxy));
	if (proxy->priv->assigned == FALSE)
		return;
	if (status)
		egg_dbus_proxy_connect (proxy);
	else
		egg_dbus_proxy_disconnect (proxy);
}

/**
 * egg_dbus_proxy_assign:
 * @proxy: This class instance
 * @connections: The bus connection
 * @service: The DBUS service name
 * @interface: The DBUS interface
 * @path: The DBUS path
 * Return value: The DBUS proxy, or NULL if we haven't connected yet.
 **/
DBusGProxy *
egg_dbus_proxy_assign (EggDbusProxy *proxy, DBusGConnection *connection,
		       const gchar *service, const gchar *path, const gchar *interface)
{
	g_return_val_if_fail (EGG_IS_DBUS_PROXY (proxy), NULL);
	g_return_val_if_fail (connection != NULL, NULL);
	g_return_val_if_fail (service != NULL, NULL);
	g_return_val_if_fail (interface != NULL, NULL);
	g_return_val_if_fail (path != NULL, NULL);

	if (proxy->priv->assigned) {
		egg_warning ("already assigned proxy!");
		return NULL;
	}

	proxy->priv->service = g_strdup (service);
	proxy->priv->interface = g_strdup (interface);
	proxy->priv->path = g_strdup (path);
	proxy->priv->connection = connection;
	proxy->priv->assigned = TRUE;

	/* We have to save the connection and remove the signal id later as
	   instances of this object are likely to be registering with a
	   singleton object many times */
	egg_dbus_monitor_assign (proxy->priv->monitor, connection, service);

	/* try to connect and return proxy (or NULL if invalid) */
	egg_dbus_proxy_connect (proxy);

	return proxy->priv->proxy;
}

/**
 * egg_dbus_proxy_get_proxy:
 * @proxy: This class instance
 * Return value: The DBUS proxy, or NULL if we are not connected
 **/
DBusGProxy *
egg_dbus_proxy_get_proxy (EggDbusProxy *proxy)
{
	g_return_val_if_fail (EGG_IS_DBUS_PROXY (proxy), NULL);
	if (proxy->priv->assigned == FALSE)
		return NULL;
	return proxy->priv->proxy;
}

/**
 * egg_dbus_proxy_is_connected:
 * @proxy: This class instance
 * Return value: if we are connected to a valid proxy
 **/
gboolean
egg_dbus_proxy_is_connected (EggDbusProxy *proxy)
{
	g_return_val_if_fail (EGG_IS_DBUS_PROXY (proxy), FALSE);
	if (proxy->priv->assigned == FALSE)
		return FALSE;
	if (proxy->priv->proxy == NULL)
		return FALSE;
	return TRUE;
}

/**
 * egg_dbus_proxy_class_init:
 * @proxy: This class instance
 **/
static void
egg_dbus_proxy_class_init (EggDbusProxyClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = egg_dbus_proxy_finalize;
	g_type_class_add_private (klass, sizeof (EggDbusProxyPrivate));

	signals [PROXY_STATUS] =
		g_signal_new ("proxy-status",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (EggDbusProxyClass, proxy_status),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__BOOLEAN,
			      G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
}

/**
 * egg_dbus_proxy_init:
 * @egg_dbus_proxy: This class instance
 **/
static void
egg_dbus_proxy_init (EggDbusProxy *proxy)
{
	proxy->priv = EGG_DBUS_PROXY_GET_PRIVATE (proxy);

	proxy->priv->connection = NULL;
	proxy->priv->proxy = NULL;
	proxy->priv->service = NULL;
	proxy->priv->interface = NULL;
	proxy->priv->path = NULL;
	proxy->priv->assigned = FALSE;
	proxy->priv->monitor = egg_dbus_monitor_new ();
	proxy->priv->monitor_callback_id =
		g_signal_connect (proxy->priv->monitor, "connection-changed",
				  G_CALLBACK (dbus_monitor_connection_cb), proxy);
	proxy->priv->monitor_callback_id = 0;
}

/**
 * egg_dbus_proxy_finalize:
 * @object: This class instance
 **/
static void
egg_dbus_proxy_finalize (GObject *object)
{
	EggDbusProxy *proxy;
	g_return_if_fail (object != NULL);
	g_return_if_fail (EGG_IS_DBUS_PROXY (object));

	proxy = EGG_DBUS_PROXY (object);
	proxy->priv = EGG_DBUS_PROXY_GET_PRIVATE (proxy);

	if (proxy->priv->monitor_callback_id != 0)
		g_signal_handler_disconnect (proxy->priv->monitor,
					     proxy->priv->monitor_callback_id);

	egg_dbus_proxy_disconnect (proxy);

	if (proxy->priv->proxy != NULL)
		g_object_unref (proxy->priv->proxy);
	g_object_unref (proxy->priv->monitor);
	g_free (proxy->priv->service);
	g_free (proxy->priv->interface);
	g_free (proxy->priv->path);

	G_OBJECT_CLASS (egg_dbus_proxy_parent_class)->finalize (object);
}

/**
 * egg_dbus_proxy_new:
 * Return value: new class instance.
 **/
EggDbusProxy *
egg_dbus_proxy_new (void)
{
	EggDbusProxy *proxy;
	proxy = g_object_new (EGG_TYPE_DBUS_PROXY, NULL);
	return EGG_DBUS_PROXY (proxy);
}

