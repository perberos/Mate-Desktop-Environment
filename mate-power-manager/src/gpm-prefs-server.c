/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 Richard Hughes <richard@hughsie.com>
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>

#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include <glib/gi18n.h>
#include <dbus/dbus-glib.h>

#include "gpm-prefs-server.h"
#include "egg-debug.h"

#define GPM_PREFS_SERVER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GPM_TYPE_PREFS_SERVER, GpmPrefsServerPrivate))

struct GpmPrefsServerPrivate
{
	guint			 capability;
};

enum {
	CAPABILITY_CHANGED,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };
static gpointer gpm_prefs_server_object = NULL;

G_DEFINE_TYPE (GpmPrefsServer, gpm_prefs_server, G_TYPE_OBJECT)

/**
 * gpm_prefs_server_get_capability:
 **/
gboolean
gpm_prefs_server_get_capability (GpmPrefsServer *server,
		       		 gint	        *capability)
{
	g_return_val_if_fail (server != NULL, FALSE);
	g_return_val_if_fail (GPM_IS_PREFS_SERVER (server), FALSE);
	*capability = server->priv->capability;
	return TRUE;
}

/**
 * gpm_prefs_server_set_capability:
 **/
gboolean
gpm_prefs_server_set_capability (GpmPrefsServer *server,
		       		 gint	         capability)
{
	g_return_val_if_fail (server != NULL, FALSE);
	g_return_val_if_fail (GPM_IS_PREFS_SERVER (server), FALSE);
	server->priv->capability = server->priv->capability + capability;
	egg_debug ("capability now %i", server->priv->capability);
	return TRUE;
}

/**
 * gpm_prefs_server_class_init:
 * @klass: This prefs class instance
 **/
static void
gpm_prefs_server_class_init (GpmPrefsServerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	g_type_class_add_private (klass, sizeof (GpmPrefsServerPrivate));
	signals [CAPABILITY_CHANGED] =
		g_signal_new ("capability-changed",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GpmPrefsServerClass, capability_changed),
			      NULL,
			      NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
}

/**
 * gpm_prefs_server_init:
 * @server: This server class instance
 *
 * initialises the server class. NOTE: We expect prefs_server objects
 * to *NOT* be removed or added during the session.
 * We only control the first prefs_server object if there are more than one.
 **/
static void
gpm_prefs_server_init (GpmPrefsServer *server)
{
	server->priv = GPM_PREFS_SERVER_GET_PRIVATE (server);
	server->priv->capability = 0;
}

/**
 * gpm_prefs_server_new:
 * Return value: A new server class instance.
 * Can return NULL if no suitable hardware is found.
 **/
GpmPrefsServer *
gpm_prefs_server_new (void)
{
	if (gpm_prefs_server_object != NULL) {
		g_object_ref (gpm_prefs_server_object);
	} else {
		gpm_prefs_server_object = g_object_new (GPM_TYPE_PREFS_SERVER, NULL);
		g_object_add_weak_pointer (gpm_prefs_server_object, &gpm_prefs_server_object);
	}
	return GPM_PREFS_SERVER (gpm_prefs_server_object);
}
