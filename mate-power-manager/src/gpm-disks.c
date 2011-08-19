/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2009 Richard Hughes <richard@hughsie.com>
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

#include <glib.h>
#include <dbus/dbus-glib.h>

#include "egg-debug.h"
#include "gpm-disks.h"

static void     gpm_disks_finalize   (GObject	  *object);

#define GPM_DISKS_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GPM_TYPE_DISKS, GpmDisksPrivate))

struct GpmDisksPrivate
{
	DBusGProxy		*proxy;
	gchar			*cookie;
};

static gpointer gpm_disks_object = NULL;

G_DEFINE_TYPE (GpmDisks, gpm_disks, G_TYPE_OBJECT)

/**
 * gpm_disks_unregister:
 **/
static gboolean
gpm_disks_unregister (GpmDisks *disks)
{
	gboolean ret = FALSE;
	GError *error = NULL;

	/* no UDisks */
	if (disks->priv->proxy == NULL) {
		egg_warning ("no UDisks");
		goto out;
	}

	/* clear spindown timeouts */
	ret = dbus_g_proxy_call (disks->priv->proxy, "DriveUnsetAllSpindownTimeouts", &error,
				 G_TYPE_STRING, disks->priv->cookie,
				 G_TYPE_INVALID,
				 G_TYPE_INVALID);
	if (!ret) {
		egg_warning ("failed to clear spindown timeout: %s", error->message);
		g_error_free (error);
		goto out;
	}
out:
	/* reset */
	g_free (disks->priv->cookie);
	disks->priv->cookie = NULL;

	return ret;
}

/**
 * gpm_disks_register:
 **/
static gboolean
gpm_disks_register (GpmDisks *disks, gint timeout)
{
	gboolean ret = FALSE;
	GError *error = NULL;
	const gchar **options = {NULL};

	/* no UDisks */
	if (disks->priv->proxy == NULL) {
		egg_warning ("no UDisks");
		goto out;
	}

	/* set spindown timeouts */
	ret = dbus_g_proxy_call (disks->priv->proxy, "DriveSetAllSpindownTimeouts", &error,
				 G_TYPE_INT, timeout,
				 G_TYPE_STRV, options,
				 G_TYPE_INVALID,
				 G_TYPE_STRING, &disks->priv->cookie,
				 G_TYPE_INVALID);
	if (!ret) {
		egg_warning ("failed to set spindown timeout: %s", error->message);
		g_error_free (error);
		goto out;
	}
out:
	return ret;
}

/**
 * gpm_disks_set_spindown_timeout:
 **/
gboolean
gpm_disks_set_spindown_timeout (GpmDisks *disks, gint timeout)
{
	gboolean ret = TRUE;

	/* get rid of last request */
	if (disks->priv->cookie != NULL) {
		egg_debug ("unregistering %s", disks->priv->cookie);
		gpm_disks_unregister (disks);
	}

	/* is not enabled? */
	if (timeout == 0) {
		egg_debug ("disk spindown disabled");
		goto out;
	}

	/* register */
	ret = gpm_disks_register (disks, timeout);
	if (ret)
		egg_debug ("registered %s (%i)", disks->priv->cookie, timeout);
out:
	return ret;
}

/**
 * gpm_disks_init:
 */
static void
gpm_disks_init (GpmDisks *disks)
{
	GError *error = NULL;
	DBusGConnection *connection;

	disks->priv = GPM_DISKS_GET_PRIVATE (disks);

	disks->priv->cookie = NULL;

	/* get proxy to interface */
	connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, NULL);
	disks->priv->proxy = dbus_g_proxy_new_for_name_owner (connection,
							      "org.freedesktop.UDisks",
							      "/org/freedesktop/UDisks",
							      "org.freedesktop.UDisks", &error);
	if (disks->priv->proxy == NULL) {
		egg_warning ("DBUS error: %s", error->message);
		g_error_free (error);
	}
}

/**
 * gpm_disks_coldplug:
 *
 * @object: This disks instance
 */
static void
gpm_disks_finalize (GObject *object)
{
	GpmDisks *disks;
	g_return_if_fail (object != NULL);
	g_return_if_fail (GPM_IS_DISKS (object));
	disks = GPM_DISKS (object);
	g_return_if_fail (disks->priv != NULL);

	g_free (disks->priv->cookie);
	g_object_unref (disks->priv->proxy);

	G_OBJECT_CLASS (gpm_disks_parent_class)->finalize (object);
}

/**
 * gpm_disks_class_init:
 * @klass: This class instance
 **/
static void
gpm_disks_class_init (GpmDisksClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gpm_disks_finalize;
	g_type_class_add_private (klass, sizeof (GpmDisksPrivate));
}

/**
 * gpm_disks_new:
 * Return value: new #GpmDisks instance.
 **/
GpmDisks *
gpm_disks_new (void)
{
	if (gpm_disks_object != NULL) {
		g_object_ref (gpm_disks_object);
	} else {
		gpm_disks_object = g_object_new (GPM_TYPE_DISKS, NULL);
		g_object_add_weak_pointer (gpm_disks_object, &gpm_disks_object);
	}
	return GPM_DISKS (gpm_disks_object);
}

