/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008-2009 Richard Hughes <richard@hughsie.com>
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

#include "gpm-session.h"
#include "gpm-common.h"
#include "egg-debug.h"
#include "gpm-marshal.h"

static void     gpm_session_finalize   (GObject		*object);

#define GPM_SESSION_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GPM_TYPE_SESSION, GpmSessionPrivate))

#define GPM_SESSION_MANAGER_SERVICE			"org.mate.SessionManager"
#define GPM_SESSION_MANAGER_PATH			"/org/mate/SessionManager"
#define GPM_SESSION_MANAGER_INTERFACE			"org.mate.SessionManager"
#define GPM_SESSION_MANAGER_PRESENCE_PATH		"/org/mate/SessionManager/Presence"
#define GPM_SESSION_MANAGER_PRESENCE_INTERFACE		"org.mate.SessionManager.Presence"
#define GPM_SESSION_MANAGER_CLIENT_PRIVATE_INTERFACE	"org.mate.SessionManager.ClientPrivate"
#define GPM_DBUS_PROPERTIES_INTERFACE			"org.freedesktop.DBus.Properties"

typedef enum {
	GPM_SESSION_STATUS_ENUM_AVAILABLE = 0,
	GPM_SESSION_STATUS_ENUM_INVISIBLE,
	GPM_SESSION_STATUS_ENUM_BUSY,
	GPM_SESSION_STATUS_ENUM_IDLE,
	GPM_SESSION_STATUS_ENUM_UNKNOWN
} GpmSessionStatusEnum;

typedef enum {
	GPM_SESSION_INHIBIT_MASK_LOGOUT = 1,
	GPM_SESSION_INHIBIT_MASK_SWITCH = 2,
	GPM_SESSION_INHIBIT_MASK_SUSPEND = 4,
	GPM_SESSION_INHIBIT_MASK_IDLE = 8
} GpmSessionInhibitMask;

struct GpmSessionPrivate
{
	DBusGProxy		*proxy;
	DBusGProxy		*proxy_presence;
	DBusGProxy		*proxy_client_private;
	DBusGProxy		*proxy_prop;
	gboolean		 is_idle_old;
	gboolean		 is_idle_inhibited_old;
	gboolean		 is_suspend_inhibited_old;
};

enum {
	IDLE_CHANGED,
	INHIBITED_CHANGED,
	STOP,
	QUERY_END_SESSION,
	END_SESSION,
	CANCEL_END_SESSION,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };
static gpointer gpm_session_object = NULL;

G_DEFINE_TYPE (GpmSession, gpm_session, G_TYPE_OBJECT)

/**
 * gpm_session_logout:
 **/
gboolean
gpm_session_logout (GpmSession *session)
{
	g_return_val_if_fail (GPM_IS_SESSION (session), FALSE);

	/* no mate-session */
	if (session->priv->proxy == NULL) {
		egg_warning ("no mate-session");
		return FALSE;
	}

	/* we have to use no reply, as the SM calls into g-p-m to get the can_suspend property */
	dbus_g_proxy_call_no_reply (session->priv->proxy, "Shutdown", G_TYPE_INVALID);
	return TRUE;
}

/**
 * gpm_session_get_idle:
 **/
gboolean
gpm_session_get_idle (GpmSession *session)
{
	g_return_val_if_fail (GPM_IS_SESSION (session), FALSE);
	return session->priv->is_idle_old;
}

/**
 * gpm_session_get_idle_inhibited:
 **/
gboolean
gpm_session_get_idle_inhibited (GpmSession *session)
{
	g_return_val_if_fail (GPM_IS_SESSION (session), FALSE);
	return session->priv->is_idle_inhibited_old;
}

/**
 * gpm_session_get_suspend_inhibited:
 **/
gboolean
gpm_session_get_suspend_inhibited (GpmSession *session)
{
	g_return_val_if_fail (GPM_IS_SESSION (session), FALSE);
	return session->priv->is_suspend_inhibited_old;
}

/**
 * gpm_session_presence_status_changed_cb:
 **/
static void
gpm_session_presence_status_changed_cb (DBusGProxy *proxy, guint status, GpmSession *session)
{
	gboolean is_idle;
	is_idle = (status == GPM_SESSION_STATUS_ENUM_IDLE);
	if (is_idle != session->priv->is_idle_old) {
		egg_debug ("emitting idle-changed : (%i)", is_idle);
		session->priv->is_idle_old = is_idle;
		g_signal_emit (session, signals [IDLE_CHANGED], 0, is_idle);
	}
}

/**
 * gpm_session_is_idle:
 **/
static gboolean
gpm_session_is_idle (GpmSession *session)
{
	gboolean ret;
	gboolean is_idle = FALSE;
	GError *error = NULL;
	GValue *value;

	/* no mate-session */
	if (session->priv->proxy_prop == NULL) {
		egg_warning ("no mate-session");
		goto out;
	}

	value = g_new0(GValue, 1);
	/* find out if this change altered the inhibited state */
	ret = dbus_g_proxy_call (session->priv->proxy_prop, "Get", &error,
				 G_TYPE_STRING, GPM_SESSION_MANAGER_PRESENCE_INTERFACE,
				 G_TYPE_STRING, "status",
				 G_TYPE_INVALID,
				 G_TYPE_VALUE, value,
				 G_TYPE_INVALID);
	if (!ret) {
		egg_warning ("failed to get idle status: %s", error->message);
		g_error_free (error);
		is_idle = FALSE;
		goto out;
	}
	is_idle = (g_value_get_uint (value) == GPM_SESSION_STATUS_ENUM_IDLE);
	g_free (value);
out:
	return is_idle;
}

/**
 * gpm_session_is_idle_inhibited:
 **/
static gboolean
gpm_session_is_idle_inhibited (GpmSession *session)
{
	gboolean ret;
	gboolean is_inhibited = FALSE;
	GError *error = NULL;

	/* no mate-session */
	if (session->priv->proxy == NULL) {
		egg_warning ("no mate-session");
		goto out;
	}

	/* find out if this change altered the inhibited state */
	ret = dbus_g_proxy_call (session->priv->proxy, "IsInhibited", &error,
				 G_TYPE_UINT, GPM_SESSION_INHIBIT_MASK_IDLE,
				 G_TYPE_INVALID,
				 G_TYPE_BOOLEAN, &is_inhibited,
				 G_TYPE_INVALID);
	if (!ret) {
		egg_warning ("failed to get inhibit status: %s", error->message);
		g_error_free (error);
		is_inhibited = FALSE;
	}
out:
	return is_inhibited;
}

/**
 * gpm_session_is_suspend_inhibited:
 **/
static gboolean
gpm_session_is_suspend_inhibited (GpmSession *session)
{
	gboolean ret;
	gboolean is_inhibited = FALSE;
	GError *error = NULL;

	/* no mate-session */
	if (session->priv->proxy == NULL) {
		egg_warning ("no mate-session");
		goto out;
	}

	/* find out if this change altered the inhibited state */
	ret = dbus_g_proxy_call (session->priv->proxy, "IsInhibited", &error,
				 G_TYPE_UINT, GPM_SESSION_INHIBIT_MASK_SUSPEND,
				 G_TYPE_INVALID,
				 G_TYPE_BOOLEAN, &is_inhibited,
				 G_TYPE_INVALID);
	if (!ret) {
		egg_warning ("failed to get inhibit status: %s", error->message);
		g_error_free (error);
		is_inhibited = FALSE;
	}
out:
	return is_inhibited;
}

/**
 * gpm_session_stop_cb:
 **/
static void
gpm_session_stop_cb (DBusGProxy *proxy, GpmSession *session)
{
	egg_debug ("emitting ::stop()");
	g_signal_emit (session, signals [STOP], 0);
}

/**
 * gpm_session_query_end_session_cb:
 **/
static void
gpm_session_query_end_session_cb (DBusGProxy *proxy, guint flags, GpmSession *session)
{
	egg_debug ("emitting ::query-end-session(%i)", flags);
	g_signal_emit (session, signals [QUERY_END_SESSION], 0, flags);
}

/**
 * gpm_session_end_session_cb:
 **/
static void
gpm_session_end_session_cb (DBusGProxy *proxy, guint flags, GpmSession *session)
{
	egg_debug ("emitting ::end-session(%i)", flags);
	g_signal_emit (session, signals [END_SESSION], 0, flags);
}

/**
 * gpm_session_end_session_response:
 **/
gboolean
gpm_session_end_session_response (GpmSession *session, gboolean is_okay, const gchar *reason)
{
	gboolean ret = FALSE;
	GError *error = NULL;

	g_return_val_if_fail (GPM_IS_SESSION (session), FALSE);
	g_return_val_if_fail (session->priv->proxy_client_private != NULL, FALSE);

	/* no mate-session */
	if (session->priv->proxy_client_private == NULL) {
		egg_warning ("no mate-session proxy");
		goto out;
	}

	/* send response */
	ret = dbus_g_proxy_call (session->priv->proxy_client_private, "EndSessionResponse", &error,
				 G_TYPE_BOOLEAN, is_okay,
				 G_TYPE_STRING, reason,
				 G_TYPE_INVALID,
				 G_TYPE_INVALID);
	if (!ret) {
		egg_warning ("failed to send session response: %s", error->message);
		g_error_free (error);
		goto out;
	}
out:
	return ret;
}

/**
 * gpm_session_register_client:
 **/
gboolean
gpm_session_register_client (GpmSession *session, const gchar *app_id, const gchar *client_startup_id)
{
	gboolean ret = FALSE;
	gchar *client_id = NULL;
	GError *error = NULL;
	DBusGConnection *connection;

	g_return_val_if_fail (GPM_IS_SESSION (session), FALSE);

	/* no mate-session */
	if (session->priv->proxy == NULL) {
		egg_warning ("no mate-session");
		goto out;
	}

	/* find out if this change altered the inhibited state */
	ret = dbus_g_proxy_call (session->priv->proxy, "RegisterClient", &error,
				 G_TYPE_STRING, app_id,
				 G_TYPE_STRING, client_startup_id,
				 G_TYPE_INVALID,
				 DBUS_TYPE_G_OBJECT_PATH, &client_id,
				 G_TYPE_INVALID);
	if (!ret) {
		egg_warning ("failed to register client '%s': %s", client_startup_id, error->message);
		g_error_free (error);
		goto out;
	}

	/* get org.mate.Session.ClientPrivate interface */
	connection = dbus_g_bus_get (DBUS_BUS_SESSION, NULL);
	session->priv->proxy_client_private = dbus_g_proxy_new_for_name_owner (connection, GPM_SESSION_MANAGER_SERVICE,
									       client_id, GPM_SESSION_MANAGER_CLIENT_PRIVATE_INTERFACE, &error);
	if (session->priv->proxy_client_private == NULL) {
		egg_warning ("DBUS error: %s", error->message);
		g_error_free (error);
		goto out;
	}

	/* get Stop */
	dbus_g_proxy_add_signal (session->priv->proxy_client_private, "Stop", G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (session->priv->proxy_client_private, "Stop", G_CALLBACK (gpm_session_stop_cb), session, NULL);

	/* get QueryEndSession */
	dbus_g_proxy_add_signal (session->priv->proxy_client_private, "QueryEndSession", G_TYPE_UINT, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (session->priv->proxy_client_private, "QueryEndSession", G_CALLBACK (gpm_session_query_end_session_cb), session, NULL);

	/* get EndSession */
	dbus_g_proxy_add_signal (session->priv->proxy_client_private, "EndSession", G_TYPE_UINT, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (session->priv->proxy_client_private, "EndSession", G_CALLBACK (gpm_session_end_session_cb), session, NULL);

	egg_debug ("registered startup '%s' to client id '%s'", client_startup_id, client_id);
out:
	g_free (client_id);
	return ret;
}

/**
 * gpm_session_inhibit_changed_cb:
 **/
static void
gpm_session_inhibit_changed_cb (DBusGProxy *proxy, const gchar *id, GpmSession *session)
{
	gboolean is_idle_inhibited;
	gboolean is_suspend_inhibited;

	is_idle_inhibited = gpm_session_is_idle_inhibited (session);
	is_suspend_inhibited = gpm_session_is_suspend_inhibited (session);
	if (is_idle_inhibited != session->priv->is_idle_inhibited_old || is_suspend_inhibited != session->priv->is_suspend_inhibited_old) {
		egg_debug ("emitting inhibited-changed : idle=(%i), suspend=(%i)", is_idle_inhibited, is_suspend_inhibited);
		session->priv->is_idle_inhibited_old = is_idle_inhibited;
		session->priv->is_suspend_inhibited_old = is_suspend_inhibited;
		g_signal_emit (session, signals [INHIBITED_CHANGED], 0, is_idle_inhibited, is_suspend_inhibited);
	}
}

/**
 * gpm_session_class_init:
 * @klass: This class instance
 **/
static void
gpm_session_class_init (GpmSessionClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gpm_session_finalize;
	g_type_class_add_private (klass, sizeof (GpmSessionPrivate));

	signals [IDLE_CHANGED] =
		g_signal_new ("idle-changed",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GpmSessionClass, idle_changed),
			      NULL, NULL, g_cclosure_marshal_VOID__BOOLEAN,
			      G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
	signals [INHIBITED_CHANGED] =
		g_signal_new ("inhibited-changed",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GpmSessionClass, inhibited_changed),
			      NULL, NULL, gpm_marshal_VOID__BOOLEAN_BOOLEAN,
			      G_TYPE_NONE, 2, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);
	signals [STOP] =
		g_signal_new ("stop",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GpmSessionClass, stop),
			      NULL, NULL, g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
	signals [QUERY_END_SESSION] =
		g_signal_new ("query-end-session",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GpmSessionClass, query_end_session),
			      NULL, NULL, g_cclosure_marshal_VOID__UINT,
			      G_TYPE_NONE, 1, G_TYPE_UINT);
	signals [END_SESSION] =
		g_signal_new ("end-session",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GpmSessionClass, end_session),
			      NULL, NULL, g_cclosure_marshal_VOID__UINT,
			      G_TYPE_NONE, 1, G_TYPE_UINT);
	signals [CANCEL_END_SESSION] =
		g_signal_new ("cancel-end-session",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GpmSessionClass, cancel_end_session),
			      NULL, NULL, g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
}

/**
 * gpm_session_init:
 * @session: This class instance
 **/
static void
gpm_session_init (GpmSession *session)
{
	DBusGConnection *connection;
	GError *error = NULL;

	session->priv = GPM_SESSION_GET_PRIVATE (session);
	session->priv->is_idle_old = FALSE;
	session->priv->is_idle_inhibited_old = FALSE;
	session->priv->is_suspend_inhibited_old = FALSE;
	session->priv->proxy_client_private = NULL;

	connection = dbus_g_bus_get (DBUS_BUS_SESSION, NULL);

	/* get org.mate.Session interface */
	session->priv->proxy = dbus_g_proxy_new_for_name_owner (connection, GPM_SESSION_MANAGER_SERVICE,
								GPM_SESSION_MANAGER_PATH,
								GPM_SESSION_MANAGER_INTERFACE, &error);
	if (session->priv->proxy == NULL) {
		egg_warning ("DBUS error: %s", error->message);
		g_error_free (error);
		return;
	}

	/* get org.mate.Session.Presence interface */
	session->priv->proxy_presence = dbus_g_proxy_new_for_name_owner (connection, GPM_SESSION_MANAGER_SERVICE,
									 GPM_SESSION_MANAGER_PRESENCE_PATH,
									 GPM_SESSION_MANAGER_PRESENCE_INTERFACE, &error);
	if (session->priv->proxy_presence == NULL) {
		egg_warning ("DBUS error: %s", error->message);
		g_error_free (error);
		return;
	}

	/* get properties interface */
	session->priv->proxy_prop = dbus_g_proxy_new_for_name_owner (connection, GPM_SESSION_MANAGER_SERVICE,
								     GPM_SESSION_MANAGER_PRESENCE_PATH,
								     GPM_DBUS_PROPERTIES_INTERFACE, &error);
	if (session->priv->proxy_prop == NULL) {
		egg_warning ("DBUS error: %s", error->message);
		g_error_free (error);
		return;
	}

	/* get StatusChanged */
	dbus_g_proxy_add_signal (session->priv->proxy_presence, "StatusChanged", G_TYPE_UINT, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (session->priv->proxy_presence, "StatusChanged", G_CALLBACK (gpm_session_presence_status_changed_cb), session, NULL);

	/* get InhibitorAdded */
	dbus_g_proxy_add_signal (session->priv->proxy, "InhibitorAdded", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (session->priv->proxy, "InhibitorAdded", G_CALLBACK (gpm_session_inhibit_changed_cb), session, NULL);

	/* get InhibitorRemoved */
	dbus_g_proxy_add_signal (session->priv->proxy, "InhibitorRemoved", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (session->priv->proxy, "InhibitorRemoved", G_CALLBACK (gpm_session_inhibit_changed_cb), session, NULL);

	/* coldplug */
	session->priv->is_idle_inhibited_old = gpm_session_is_idle_inhibited (session);
	session->priv->is_suspend_inhibited_old = gpm_session_is_suspend_inhibited (session);
	session->priv->is_idle_old = gpm_session_is_idle (session);
	egg_debug ("idle: %i, idle_inhibited: %i, suspend_inhibited: %i", session->priv->is_idle_old, session->priv->is_idle_inhibited_old, session->priv->is_suspend_inhibited_old);
}

/**
 * gpm_session_finalize:
 * @object: This class instance
 **/
static void
gpm_session_finalize (GObject *object)
{
	GpmSession *session;
	g_return_if_fail (object != NULL);
	g_return_if_fail (GPM_IS_SESSION (object));

	session = GPM_SESSION (object);
	session->priv = GPM_SESSION_GET_PRIVATE (session);

	g_object_unref (session->priv->proxy);
	g_object_unref (session->priv->proxy_presence);
	if (session->priv->proxy_client_private != NULL)
		g_object_unref (session->priv->proxy_client_private);
	g_object_unref (session->priv->proxy_prop);

	G_OBJECT_CLASS (gpm_session_parent_class)->finalize (object);
}

/**
 * gpm_session_new:
 * Return value: new GpmSession instance.
 **/
GpmSession *
gpm_session_new (void)
{
	if (gpm_session_object != NULL) {
		g_object_ref (gpm_session_object);
	} else {
		gpm_session_object = g_object_new (GPM_TYPE_SESSION, NULL);
		g_object_add_weak_pointer (gpm_session_object, &gpm_session_object);
	}
	return GPM_SESSION (gpm_session_object);
}
