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
#include <sys/wait.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include <glib/gi18n.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <mate-keyring.h>
#include <mateconf/mateconf-client.h>
#include <libupower-glib/upower.h>

#include "egg-debug.h"
#include "egg-console-kit.h"

#include "gpm-screensaver.h"
#include "gpm-common.h"
#include "gpm-control.h"
#include "gpm-networkmanager.h"

#define GPM_CONTROL_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GPM_TYPE_CONTROL, GpmControlPrivate))

struct GpmControlPrivate
{
	MateConfClient		*conf;
	UpClient		*client;
};

enum {
	RESUME,
	SLEEP,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };
static gpointer gpm_control_object = NULL;

G_DEFINE_TYPE (GpmControl, gpm_control, G_TYPE_OBJECT)

/**
 * gpm_control_error_quark:
 * Return value: Our personal error quark.
 **/
GQuark
gpm_control_error_quark (void)
{
	static GQuark quark = 0;
	if (!quark)
		quark = g_quark_from_static_string ("gpm_control_error");
	return quark;
}

/**
 * gpm_control_shutdown:
 * @control: This class instance
 *
 * Shuts down the computer
 **/
gboolean
gpm_control_shutdown (GpmControl *control, GError **error)
{
	gboolean ret;
	EggConsoleKit *console;
	console = egg_console_kit_new ();
	ret = egg_console_kit_stop (console, error);
	g_object_unref (console);
	return ret;
}

/**
 * gpm_control_get_lock_policy:
 * @control: This class instance
 * @policy: The policy mateconf string.
 *
 * This function finds out if we should lock the screen when we do an
 * action. It is required as we can either use the mate-screensaver policy
 * or the custom policy. See the yelp file for more information.
 *
 * Return value: TRUE if we should lock.
 **/
gboolean
gpm_control_get_lock_policy (GpmControl *control, const gchar *policy)
{
	gboolean do_lock;
	gboolean use_ss_setting;
	/* This allows us to over-ride the custom lock settings set in mateconf
	   with a system default set in mate-screensaver.
	   See bug #331164 for all the juicy details. :-) */
	use_ss_setting = mateconf_client_get_bool (control->priv->conf, GPM_CONF_LOCK_USE_SCREENSAVER, NULL);
	if (use_ss_setting) {
		do_lock = mateconf_client_get_bool (control->priv->conf, GS_PREF_LOCK_ENABLED, NULL);
		egg_debug ("Using ScreenSaver settings (%i)", do_lock);
	} else {
		do_lock = mateconf_client_get_bool (control->priv->conf, policy, NULL);
		egg_debug ("Using custom locking settings (%i)", do_lock);
	}
	return do_lock;
}

/**
 * gpm_control_suspend:
 **/
gboolean
gpm_control_suspend (GpmControl *control, GError **error)
{
	gboolean allowed;
	gboolean ret = FALSE;
	gboolean do_lock;
	gboolean nm_sleep;
	gboolean lock_mate_keyring;
	MateKeyringResult keyres;
	GpmScreensaver *screensaver;
	guint32 throttle_cookie = 0;

	screensaver = gpm_screensaver_new ();

	g_object_get (control->priv->client,
		      "can-suspend", &allowed,
		      NULL);
	if (!allowed) {
		egg_debug ("cannot suspend as not allowed from policy");
		g_set_error_literal (error, GPM_CONTROL_ERROR, GPM_CONTROL_ERROR_GENERAL, "Cannot suspend");
		goto out;
	}

	/* we should perhaps lock keyrings when sleeping #375681 */
	lock_mate_keyring = mateconf_client_get_bool (control->priv->conf, GPM_CONF_LOCK_MATE_KEYRING_SUSPEND, NULL);
	if (lock_mate_keyring) {
		keyres = mate_keyring_lock_all_sync ();
		if (keyres != MATE_KEYRING_RESULT_OK)
			egg_warning ("could not lock keyring");
	}

	do_lock = gpm_control_get_lock_policy (control, GPM_CONF_LOCK_ON_SUSPEND);
	if (do_lock) {
		throttle_cookie = gpm_screensaver_add_throttle (screensaver, "suspend");
		gpm_screensaver_lock (screensaver);
	}

	nm_sleep = mateconf_client_get_bool (control->priv->conf, GPM_CONF_NETWORKMANAGER_SLEEP, NULL);
	if (nm_sleep)
		gpm_networkmanager_sleep ();

	/* Do the suspend */
	egg_debug ("emitting sleep");
	g_signal_emit (control, signals [SLEEP], 0, GPM_CONTROL_ACTION_SUSPEND);

	ret = up_client_suspend_sync (control->priv->client, NULL, error);

	egg_debug ("emitting resume");
	g_signal_emit (control, signals [RESUME], 0, GPM_CONTROL_ACTION_SUSPEND);

	if (do_lock) {
		gpm_screensaver_poke (screensaver);
		if (throttle_cookie)
			gpm_screensaver_remove_throttle (screensaver, throttle_cookie);
	}

	nm_sleep = mateconf_client_get_bool (control->priv->conf, GPM_CONF_NETWORKMANAGER_SLEEP, NULL);
	if (nm_sleep)
		gpm_networkmanager_wake ();

out:
	g_object_unref (screensaver);
	return ret;
}

/**
 * gpm_control_hibernate:
 **/
gboolean
gpm_control_hibernate (GpmControl *control, GError **error)
{
	gboolean allowed;
	gboolean ret = FALSE;
	gboolean do_lock;
	gboolean nm_sleep;
	gboolean lock_mate_keyring;
	MateKeyringResult keyres;
	GpmScreensaver *screensaver;
	guint32 throttle_cookie = 0;

	screensaver = gpm_screensaver_new ();

	g_object_get (control->priv->client,
		      "can-hibernate", &allowed,
		      NULL);
	if (!allowed) {
		egg_debug ("cannot hibernate as not allowed from policy");
		g_set_error_literal (error, GPM_CONTROL_ERROR, GPM_CONTROL_ERROR_GENERAL, "Cannot hibernate");
		goto out;
	}

	/* we should perhaps lock keyrings when sleeping #375681 */
	lock_mate_keyring = mateconf_client_get_bool (control->priv->conf, GPM_CONF_LOCK_MATE_KEYRING_HIBERNATE, NULL);
	if (lock_mate_keyring) {
		keyres = mate_keyring_lock_all_sync ();
		if (keyres != MATE_KEYRING_RESULT_OK) {
			egg_warning ("could not lock keyring");
		}
	}

	do_lock = gpm_control_get_lock_policy (control, GPM_CONF_LOCK_ON_HIBERNATE);
	if (do_lock) {
		throttle_cookie = gpm_screensaver_add_throttle (screensaver, "hibernate");
		gpm_screensaver_lock (screensaver);
	}

	nm_sleep = mateconf_client_get_bool (control->priv->conf, GPM_CONF_NETWORKMANAGER_SLEEP, NULL);
	if (nm_sleep)
		gpm_networkmanager_sleep ();

	egg_debug ("emitting sleep");
	g_signal_emit (control, signals [SLEEP], 0, GPM_CONTROL_ACTION_HIBERNATE);

	ret = up_client_hibernate_sync (control->priv->client, NULL, error);

	egg_debug ("emitting resume");
	g_signal_emit (control, signals [RESUME], 0, GPM_CONTROL_ACTION_HIBERNATE);

	if (do_lock) {
		gpm_screensaver_poke (screensaver);
		if (throttle_cookie)
			gpm_screensaver_remove_throttle (screensaver, throttle_cookie);
	}

	nm_sleep = mateconf_client_get_bool (control->priv->conf, GPM_CONF_NETWORKMANAGER_SLEEP, NULL);
	if (nm_sleep)
		gpm_networkmanager_wake ();

out:
	g_object_unref (screensaver);
	return ret;
}

/**
 * gpm_control_finalize:
 **/
static void
gpm_control_finalize (GObject *object)
{
	GpmControl *control;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GPM_IS_CONTROL (object));
	control = GPM_CONTROL (object);

	g_object_unref (control->priv->conf);
	g_object_unref (control->priv->client);

	g_return_if_fail (control->priv != NULL);
	G_OBJECT_CLASS (gpm_control_parent_class)->finalize (object);
}

/**
 * gpm_control_class_init:
 **/
static void
gpm_control_class_init (GpmControlClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gpm_control_finalize;

	signals [RESUME] =
		g_signal_new ("resume",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GpmControlClass, resume),
			      NULL,
			      NULL,
			      g_cclosure_marshal_VOID__INT,
			      G_TYPE_NONE, 1, G_TYPE_INT);
	signals [SLEEP] =
		g_signal_new ("sleep",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GpmControlClass, sleep),
			      NULL,
			      NULL,
			      g_cclosure_marshal_VOID__INT,
			      G_TYPE_NONE, 1, G_TYPE_INT);

	g_type_class_add_private (klass, sizeof (GpmControlPrivate));
}

/**
 * gpm_control_init:
 * @control: This control class instance
 **/
static void
gpm_control_init (GpmControl *control)
{
	control->priv = GPM_CONTROL_GET_PRIVATE (control);

	control->priv->client = up_client_new ();
	control->priv->conf = mateconf_client_get_default ();
}

/**
 * gpm_control_new:
 * Return value: A new control class instance.
 **/
GpmControl *
gpm_control_new (void)
{
	if (gpm_control_object != NULL) {
		g_object_ref (gpm_control_object);
	} else {
		gpm_control_object = g_object_new (GPM_TYPE_CONTROL, NULL);
		g_object_add_weak_pointer (gpm_control_object, &gpm_control_object);
	}
	return GPM_CONTROL (gpm_control_object);
}

