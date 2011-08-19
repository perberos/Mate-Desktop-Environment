/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2006-2007 Richard Hughes <richard@hughsie.com>
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
#include <mateconf/mateconf-client.h>

#include "gpm-screensaver.h"
#include "gpm-common.h"
#include "egg-debug.h"

static void     gpm_screensaver_finalize   (GObject		*object);

#define GPM_SCREENSAVER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GPM_TYPE_SCREENSAVER, GpmScreensaverPrivate))

#define GS_LISTENER_SERVICE	"org.mate.ScreenSaver"
#define GS_LISTENER_PATH	"/"
#define GS_LISTENER_INTERFACE	"org.mate.ScreenSaver"

struct GpmScreensaverPrivate
{
	DBusGProxy		*proxy;
	MateConfClient		*conf;
};

enum {
	AUTH_REQUEST,
	LAST_SIGNAL
};

#if 0
static guint signals [LAST_SIGNAL] = { 0 };
#endif
static gpointer gpm_screensaver_object = NULL;

G_DEFINE_TYPE (GpmScreensaver, gpm_screensaver, G_TYPE_OBJECT)

#if 0

/** Invoked when we get the AuthenticationRequestBegin from g-s when the user
 *  has moved their mouse and we are showing the authentication box.
 */
static void
gpm_screensaver_auth_begin (DBusGProxy     *proxy,
			    GpmScreensaver *screensaver)
{
	egg_debug ("emitting auth-request : (%i)", TRUE);
	g_signal_emit (screensaver, signals [AUTH_REQUEST], 0, TRUE);
}

/** Invoked when we get the AuthenticationRequestEnd from g-s when the user
 *  has entered a valid password or re-authenticated.
 */
static void
gpm_screensaver_auth_end (DBusGProxy     *proxy,
			  GpmScreensaver *screensaver)
{
	egg_debug ("emitting auth-request : (%i)", FALSE);
	g_signal_emit (screensaver, signals [AUTH_REQUEST], 0, FALSE);
}

/**
 * gpm_screensaver_proxy_connect_more:
 * @screensaver: This class instance
 **/
static gboolean
gpm_screensaver_proxy_connect_more (GpmScreensaver *screensaver)
{
	g_return_val_if_fail (GPM_IS_SCREENSAVER (screensaver), FALSE);

	if (screensaver->priv->proxy == NULL) {
		egg_warning ("not connected");
		return FALSE;
	}

	/* get AuthenticationRequestBegin */
	dbus_g_proxy_add_signal (screensaver->priv->proxy,
				 "AuthenticationRequestBegin", G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (screensaver->priv->proxy,
				     "AuthenticationRequestBegin",
				     G_CALLBACK (gpm_screensaver_auth_begin),
				     screensaver, NULL);

	/* get AuthenticationRequestEnd */
	dbus_g_proxy_add_signal (screensaver->priv->proxy,
				 "AuthenticationRequestEnd", G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (screensaver->priv->proxy,
				     "AuthenticationRequestEnd",
				     G_CALLBACK (gpm_screensaver_auth_end),
				     screensaver, NULL);

	return TRUE;
}

/**
 * gpm_screensaver_proxy_disconnect_more:
 * @screensaver: This class instance
 **/
static gboolean
gpm_screensaver_proxy_disconnect_more (GpmScreensaver *screensaver)
{
	g_return_val_if_fail (GPM_IS_SCREENSAVER (screensaver), FALSE);
	egg_debug ("mate-screensaver disconnected from the session DBUS");
	return TRUE;
}
#endif

/**
 * gpm_screensaver_lock_enabled:
 * @screensaver: This class instance
 * Return value: If mate-screensaver is set to lock the screen on screensave
 **/
gboolean
gpm_screensaver_lock_enabled (GpmScreensaver *screensaver)
{
	gboolean enabled;
	g_return_val_if_fail (GPM_IS_SCREENSAVER (screensaver), FALSE);
	enabled = mateconf_client_get_bool (screensaver->priv->conf, GS_PREF_LOCK_ENABLED, NULL);
	return enabled;
}

/**
 * gpm_screensaver_lock
 * @screensaver: This class instance
 * Return value: Success value
 **/
gboolean
gpm_screensaver_lock (GpmScreensaver *screensaver)
{
	guint sleepcount = 0;

	g_return_val_if_fail (GPM_IS_SCREENSAVER (screensaver), FALSE);

	if (screensaver->priv->proxy == NULL) {
		egg_warning ("not connected");
		return FALSE;
	}

	egg_debug ("doing mate-screensaver lock");
	dbus_g_proxy_call_no_reply (screensaver->priv->proxy,
				    "Lock", G_TYPE_INVALID);

	/* When we send the Lock signal to g-ss it takes maybe a second
	   or so to fade the screen and lock. If we suspend mid fade then on
	   resume the X display is still present for a split second
	   (since fade is gamma) and as such it can leak information.
	   Instead we wait until g-ss reports running and thus blanked
	   solidly before we continue from the screensaver_lock action.
	   The interior of g-ss is async, so we cannot get the dbus method
	   to block until lock is complete. */
	while (! gpm_screensaver_check_running (screensaver)) {
		/* Sleep for 1/10s */
		g_usleep (1000 * 100);
		if (sleepcount++ > 50) {
			egg_debug ("timeout waiting for mate-screensaver");
			break;
		}
	}

	return TRUE;
}

/**
 * gpm_screensaver_add_throttle:
 * @screensaver: This class instance
 * @reason:      The reason for throttling
 * Return value: Success value, or zero for failure
 **/
guint
gpm_screensaver_add_throttle (GpmScreensaver *screensaver,
			      const char     *reason)
{
	GError  *error = NULL;
	gboolean ret;
	guint32  cookie;

	g_return_val_if_fail (GPM_IS_SCREENSAVER (screensaver), 0);
	g_return_val_if_fail (reason != NULL, 0);

	if (screensaver->priv->proxy == NULL) {
		egg_warning ("not connected");
		return 0;
	}

	ret = dbus_g_proxy_call (screensaver->priv->proxy,
				 "Throttle", &error,
				 G_TYPE_STRING, "Power screensaver",
				 G_TYPE_STRING, reason,
				 G_TYPE_INVALID,
				 G_TYPE_UINT, &cookie,
				 G_TYPE_INVALID);
	if (error) {
		egg_debug ("ERROR: %s", error->message);
		g_error_free (error);
	}
	if (!ret) {
		/* abort as the DBUS method failed */
		egg_warning ("Throttle failed!");
		return 0;
	}

	egg_debug ("adding throttle reason: '%s': id %u", reason, cookie);
	return cookie;
}

/**
 * gpm_screensaver_remove_throttle:
 **/
gboolean
gpm_screensaver_remove_throttle (GpmScreensaver *screensaver, guint cookie)
{
	gboolean ret;
	GError *error = NULL;

	g_return_val_if_fail (GPM_IS_SCREENSAVER (screensaver), FALSE);

	if (screensaver->priv->proxy == NULL) {
		egg_warning ("not connected");
		return FALSE;
	}

	egg_debug ("removing throttle: id %u", cookie);
	ret = dbus_g_proxy_call (screensaver->priv->proxy,
				 "UnThrottle", &error,
				 G_TYPE_UINT, cookie,
				 G_TYPE_INVALID,
				 G_TYPE_INVALID);
	if (error) {
		egg_debug ("ERROR: %s", error->message);
		g_error_free (error);
	}
	if (!ret) {
		/* abort as the DBUS method failed */
		egg_warning ("UnThrottle failed!");
		return FALSE;
	}

	return TRUE;
}

/**
 * gpm_screensaver_check_running:
 * @screensaver: This class instance
 * Return value: TRUE if mate-screensaver is running
 **/
gboolean
gpm_screensaver_check_running (GpmScreensaver *screensaver)
{
	gboolean ret;
	gboolean temp = TRUE;
	GError *error = NULL;

	g_return_val_if_fail (GPM_IS_SCREENSAVER (screensaver), FALSE);

	if (screensaver->priv->proxy == NULL) {
		egg_warning ("not connected");
		return FALSE;
	}

	ret = dbus_g_proxy_call (screensaver->priv->proxy,
				 "GetActive", &error,
				 G_TYPE_INVALID,
				 G_TYPE_BOOLEAN, &temp,
				 G_TYPE_INVALID);
	if (error) {
		egg_debug ("ERROR: %s", error->message);
		g_error_free (error);
	}

	return ret;
}

/**
 * gpm_screensaver_poke:
 * @screensaver: This class instance
 *
 * Pokes MATE Screensaver simulating hardware events. This displays the unlock
 * dialogue when we resume, so the user doesn't have to move the mouse or press
 * any key before the window comes up.
 **/
gboolean
gpm_screensaver_poke (GpmScreensaver *screensaver)
{
	g_return_val_if_fail (GPM_IS_SCREENSAVER (screensaver), FALSE);

	if (screensaver->priv->proxy == NULL) {
		egg_warning ("not connected");
		return FALSE;
	}

	egg_debug ("poke");
	dbus_g_proxy_call_no_reply (screensaver->priv->proxy,
				    "SimulateUserActivity",
				    G_TYPE_INVALID);
	return TRUE;
}

/**
 * gpm_screensaver_class_init:
 * @klass: This class instance
 **/
static void
gpm_screensaver_class_init (GpmScreensaverClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gpm_screensaver_finalize;
	g_type_class_add_private (klass, sizeof (GpmScreensaverPrivate));

#if 0
	signals [AUTH_REQUEST] =
		g_signal_new ("auth-request",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GpmScreensaverClass, auth_request),
			      NULL,
			      NULL,
			      g_cclosure_marshal_VOID__BOOLEAN,
			      G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
#endif
}

/**
 * gpm_screensaver_init:
 * @screensaver: This class instance
 **/
static void
gpm_screensaver_init (GpmScreensaver *screensaver)
{
	DBusGConnection *connection;

	screensaver->priv = GPM_SCREENSAVER_GET_PRIVATE (screensaver);

	connection = dbus_g_bus_get (DBUS_BUS_SESSION, NULL);
	screensaver->priv->proxy = dbus_g_proxy_new_for_name (connection,
							      GS_LISTENER_SERVICE,
							      GS_LISTENER_PATH,
							      GS_LISTENER_INTERFACE);
	screensaver->priv->conf = mateconf_client_get_default ();
}

/**
 * gpm_screensaver_finalize:
 * @object: This class instance
 **/
static void
gpm_screensaver_finalize (GObject *object)
{
	GpmScreensaver *screensaver;
	g_return_if_fail (object != NULL);
	g_return_if_fail (GPM_IS_SCREENSAVER (object));

	screensaver = GPM_SCREENSAVER (object);
	screensaver->priv = GPM_SCREENSAVER_GET_PRIVATE (screensaver);

	g_object_unref (screensaver->priv->conf);
	g_object_unref (screensaver->priv->proxy);

	G_OBJECT_CLASS (gpm_screensaver_parent_class)->finalize (object);
}

/**
 * gpm_screensaver_new:
 * Return value: new GpmScreensaver instance.
 **/
GpmScreensaver *
gpm_screensaver_new (void)
{
	if (gpm_screensaver_object != NULL) {
		g_object_ref (gpm_screensaver_object);
	} else {
		gpm_screensaver_object = g_object_new (GPM_TYPE_SCREENSAVER, NULL);
		g_object_add_weak_pointer (gpm_screensaver_object, &gpm_screensaver_object);
	}
	return GPM_SCREENSAVER (gpm_screensaver_object);
}
/***************************************************************************
 ***                          MAKE CHECK TESTS                           ***
 ***************************************************************************/
#ifdef EGG_TEST
#include "egg-test.h"

#if 0
static gboolean test_got_request = FALSE;
static void
gpm_screensaver_test_auth_request_cb (GpmScreensaver *screensaver, gboolean auth, EggTest *test)
{
	egg_debug ("auth request = %i", auth);
	test_got_request = auth;

	egg_test_loop_quit (test);
}
#endif

void
gpm_screensaver_test (gpointer data)
{
	GpmScreensaver *screensaver;
//	guint value;
	gboolean ret;
	EggTest *test = (EggTest *) data;

	if (egg_test_start (test, "GpmScreensaver") == FALSE)
		return;

	/************************************************************/
	egg_test_title (test, "make sure we get a non null screensaver");
	screensaver = gpm_screensaver_new ();
	egg_test_assert (test, (screensaver != NULL));

#if 0
	/* connect signals */
	g_signal_connect (screensaver, "auth-request",
			  G_CALLBACK (gpm_screensaver_test_auth_request_cb), test);
#endif
	/************************************************************/
	egg_test_title (test, "lock screensaver");
	ret = gpm_screensaver_lock (screensaver);
	egg_test_assert (test, ret);

	/************************************************************/
	egg_test_title (test, "poke screensaver");
	ret = gpm_screensaver_poke (screensaver);
	egg_test_assert (test, ret);

	g_object_unref (screensaver);

	egg_test_end (test);
}

#endif

