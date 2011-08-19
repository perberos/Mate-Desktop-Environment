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
#include <unistd.h>
#include <stdio.h>
#include <glib.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus.h>

#include "egg-debug.h"
#include "egg-console-kit.h"

static void     egg_console_kit_finalize	(GObject		*object);

#define EGG_CONSOLE_KIT_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), EGG_TYPE_CONSOLE_KIT, EggConsoleKitPrivate))

#define CONSOLEKIT_NAME			"org.freedesktop.ConsoleKit"
#define CONSOLEKIT_PATH			"/org/freedesktop/ConsoleKit"
#define CONSOLEKIT_INTERFACE		"org.freedesktop.ConsoleKit"

#define CONSOLEKIT_MANAGER_PATH	 	"/org/freedesktop/ConsoleKit/Manager"
#define CONSOLEKIT_MANAGER_INTERFACE    "org.freedesktop.ConsoleKit.Manager"
#define CONSOLEKIT_SEAT_INTERFACE       "org.freedesktop.ConsoleKit.Seat"
#define CONSOLEKIT_SESSION_INTERFACE    "org.freedesktop.ConsoleKit.Session"

struct EggConsoleKitPrivate
{
	DBusGConnection		*connection;
	DBusGProxy		*proxy_manager;
	DBusGProxy		*proxy_session;
	gchar			*session_id;
};

enum {
	EGG_CONSOLE_KIT_ACTIVE_CHANGED,
	EGG_CONSOLE_KIT_LAST_SIGNAL
};

static gpointer egg_console_kit_object = NULL;
static guint signals [EGG_CONSOLE_KIT_LAST_SIGNAL] = { 0 };
G_DEFINE_TYPE (EggConsoleKit, egg_console_kit, G_TYPE_OBJECT)

/**
 * egg_console_kit_restart:
 **/
gboolean
egg_console_kit_restart (EggConsoleKit *console, GError **error)
{
	gboolean ret;
	GError *error_local = NULL;

	g_return_val_if_fail (EGG_IS_CONSOLE_KIT (console), FALSE);
	g_return_val_if_fail (console->priv->proxy_manager != NULL, FALSE);

	ret = dbus_g_proxy_call (console->priv->proxy_manager, "Restart", &error_local,
				 G_TYPE_INVALID, G_TYPE_INVALID);
	if (!ret) {
		egg_warning ("Couldn't restart: %s", error_local->message);
		if (error != NULL)
			*error = g_error_new (1, 0, "%s", error_local->message);
		g_error_free (error_local);
	}
	return ret;
}

/**
 * egg_console_kit_stop:
 **/
gboolean
egg_console_kit_stop (EggConsoleKit *console, GError **error)
{
	gboolean ret;
	GError *error_local = NULL;

	g_return_val_if_fail (EGG_IS_CONSOLE_KIT (console), FALSE);
	g_return_val_if_fail (console->priv->proxy_manager != NULL, FALSE);

	ret = dbus_g_proxy_call (console->priv->proxy_manager, "Stop", &error_local,
				 G_TYPE_INVALID, G_TYPE_INVALID);
	if (!ret) {
		egg_warning ("Couldn't stop: %s", error_local->message);
		if (error != NULL)
			*error = g_error_new (1, 0, "%s", error_local->message);
		g_error_free (error_local);
	}
	return ret;
}

/**
 * egg_console_kit_can_stop:
 **/
gboolean
egg_console_kit_can_stop (EggConsoleKit *console, gboolean *can_stop, GError **error)
{
	gboolean ret;
	GError *error_local = NULL;

	g_return_val_if_fail (EGG_IS_CONSOLE_KIT (console), FALSE);
	g_return_val_if_fail (console->priv->proxy_manager != NULL, FALSE);

	ret = dbus_g_proxy_call (console->priv->proxy_manager, "CanStop", &error_local,
				 G_TYPE_INVALID,
				 G_TYPE_BOOLEAN, can_stop, G_TYPE_INVALID);
	if (!ret) {
		egg_warning ("Couldn't do CanStop: %s", error_local->message);
		if (error != NULL)
			*error = g_error_new (1, 0, "%s", error_local->message);
		g_error_free (error_local);
		/* CanStop was only added in new versions of ConsoleKit,
		 * so assume true if this failed */
		*can_stop = TRUE;
	}
	return ret;
}

/**
 * egg_console_kit_can_restart:
 **/
gboolean
egg_console_kit_can_restart (EggConsoleKit *console, gboolean *can_restart, GError **error)
{
	gboolean ret;
	GError *error_local = NULL;

	g_return_val_if_fail (EGG_IS_CONSOLE_KIT (console), FALSE);
	g_return_val_if_fail (console->priv->proxy_manager != NULL, FALSE);

	ret = dbus_g_proxy_call (console->priv->proxy_manager, "CanRestart", &error_local,
				 G_TYPE_INVALID,
				 G_TYPE_BOOLEAN, can_restart, G_TYPE_INVALID);
	if (!ret) {
		egg_warning ("Couldn't do CanRestart: %s", error_local->message);
		if (error != NULL)
			*error = g_error_new (1, 0, "%s", error_local->message);
		g_error_free (error_local);
		/* CanRestart was only added in new versions of ConsoleKit,
		 * so assume true if this failed */
		*can_restart = TRUE;
	}
	return ret;
}

/**
 * egg_console_kit_is_local:
 *
 * Return value: Returns whether the session is local
 **/
gboolean
egg_console_kit_is_local (EggConsoleKit *console)
{
	gboolean ret = FALSE;
	gboolean value = FALSE;
	GError *error = NULL;

	g_return_val_if_fail (EGG_IS_CONSOLE_KIT (console), FALSE);

	/* maybe console kit does not know about our session */
	if (console->priv->proxy_session == NULL) {
		egg_warning ("no ConsoleKit session");
		goto out;
	}

	/* is our session local */
	ret = dbus_g_proxy_call (console->priv->proxy_session, "IsLocal", &error, G_TYPE_INVALID,
				 G_TYPE_BOOLEAN, &value, G_TYPE_INVALID);
	if (!ret) {
		g_warning ("IsLocal failed: %s", error->message);
		g_error_free (error);
		goto out;
	}

	/* return value only if we successed */
	ret = value;
out:
	return ret;
}

/**
 * egg_console_kit_is_active:
 *
 * Return value: Returns whether the session is active on the Seat that it is attached to.
 **/
gboolean
egg_console_kit_is_active (EggConsoleKit *console)
{
	gboolean ret = FALSE;
	gboolean value = FALSE;
	GError *error = NULL;

	g_return_val_if_fail (EGG_IS_CONSOLE_KIT (console), FALSE);

	/* maybe console kit does not know about our session */
	if (console->priv->proxy_session == NULL) {
		egg_warning ("no ConsoleKit session");
		goto out;
	}

	/* is our session active */
	ret = dbus_g_proxy_call (console->priv->proxy_session, "IsActive", &error, G_TYPE_INVALID,
				 G_TYPE_BOOLEAN, &value, G_TYPE_INVALID);
	if (!ret) {
		g_warning ("IsActive failed: %s", error->message);
		g_error_free (error);
		goto out;
	}

	/* return value only if we successed */
	ret = value;
out:
	return ret;
}

/**
 * egg_console_kit_active_changed_cb:
 **/
static void
egg_console_kit_active_changed_cb (DBusGProxy *proxy, gboolean active, EggConsoleKit *console)
{
	egg_debug ("emitting active: %i", active);
	g_signal_emit (console, signals [EGG_CONSOLE_KIT_ACTIVE_CHANGED], 0, active);
}

/**
 * egg_console_kit_class_init:
 * @klass: The EggConsoleKitClass
 **/
static void
egg_console_kit_class_init (EggConsoleKitClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = egg_console_kit_finalize;
	g_type_class_add_private (klass, sizeof (EggConsoleKitPrivate));
	signals [EGG_CONSOLE_KIT_ACTIVE_CHANGED] =
		g_signal_new ("active-changed",
			      G_TYPE_FROM_CLASS (object_class), G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (EggConsoleKitClass, active_changed),
			      NULL, NULL, g_cclosure_marshal_VOID__BOOLEAN,
			      G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
}

/**
 * egg_console_kit_init:
 **/
static void
egg_console_kit_init (EggConsoleKit *console)
{
	gboolean ret;
	GError *error = NULL;
	guint32 pid;

	console->priv = EGG_CONSOLE_KIT_GET_PRIVATE (console);
	console->priv->proxy_manager = NULL;
	console->priv->session_id = NULL;

	/* connect to D-Bus */
	console->priv->connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
	if (console->priv->connection == NULL) {
		egg_warning ("Failed to connect to the D-Bus daemon: %s", error->message);
		g_error_free (error);
		goto out;
	}

	/* connect to ConsoleKit */
	console->priv->proxy_manager =
		dbus_g_proxy_new_for_name (console->priv->connection, CONSOLEKIT_NAME,
					   CONSOLEKIT_MANAGER_PATH, CONSOLEKIT_MANAGER_INTERFACE);
	if (console->priv->proxy_manager == NULL) {
		egg_warning ("cannot connect to ConsoleKit");
		goto out;
	}

	/* get the session we are running in */
	pid = getpid ();
	ret = dbus_g_proxy_call (console->priv->proxy_manager, "GetSessionForUnixProcess", &error,
				 G_TYPE_UINT, pid,
				 G_TYPE_INVALID,
				 DBUS_TYPE_G_OBJECT_PATH, &console->priv->session_id,
				 G_TYPE_INVALID);
	if (!ret) {
		egg_warning ("Failed to get session for pid %i: %s", pid, error->message);
		g_error_free (error);
		goto out;
	}
	egg_debug ("ConsoleKit session ID: %s", console->priv->session_id);

	/* connect to session */
	console->priv->proxy_session =
		dbus_g_proxy_new_for_name (console->priv->connection, CONSOLEKIT_NAME,
					   console->priv->session_id, CONSOLEKIT_SESSION_INTERFACE);
	if (console->priv->proxy_session == NULL) {
		egg_warning ("cannot connect to: %s", console->priv->session_id);
		goto out;
	}
	dbus_g_proxy_add_signal (console->priv->proxy_session, "ActiveChanged", G_TYPE_BOOLEAN, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (console->priv->proxy_session, "ActiveChanged",
				     G_CALLBACK (egg_console_kit_active_changed_cb), console, NULL);

out:
	return;
}

/**
 * egg_console_kit_finalize:
 * @object: The object to finalize
 **/
static void
egg_console_kit_finalize (GObject *object)
{
	EggConsoleKit *console;

	g_return_if_fail (EGG_IS_CONSOLE_KIT (object));

	console = EGG_CONSOLE_KIT (object);

	g_return_if_fail (console->priv != NULL);
	if (console->priv->proxy_manager != NULL)
		g_object_unref (console->priv->proxy_manager);
	if (console->priv->proxy_session != NULL)
		g_object_unref (console->priv->proxy_session);
	g_free (console->priv->session_id);

	G_OBJECT_CLASS (egg_console_kit_parent_class)->finalize (object);
}

/**
 * egg_console_kit_new:
 *
 * Return value: a new EggConsoleKit object.
 **/
EggConsoleKit *
egg_console_kit_new (void)
{
	if (egg_console_kit_object != NULL) {
		g_object_ref (egg_console_kit_object);
	} else {
		egg_console_kit_object = g_object_new (EGG_TYPE_CONSOLE_KIT, NULL);
		g_object_add_weak_pointer (egg_console_kit_object, &egg_console_kit_object);
	}

	return EGG_CONSOLE_KIT (egg_console_kit_object);
}

