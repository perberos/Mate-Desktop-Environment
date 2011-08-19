/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2005 William Jon McCann <mccann@jhu.edu>
 * Copyright (C) 2005-2009 Richard Hughes <richard@hughsie.com>
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

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "egg-debug.h"
#include "egg-idletime.h"

#include "gpm-idle.h"
#include "gpm-load.h"
#include "gpm-session.h"

#define GPM_IDLE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GPM_TYPE_IDLE, GpmIdlePrivate))

/* Sets the idle percent limit, i.e. how hard the computer can work
   while considered "at idle" */
#define GPM_IDLE_CPU_LIMIT			5
#define	GPM_IDLE_IDLETIME_ID			1

struct GpmIdlePrivate
{
	EggIdletime	*idletime;
	GpmLoad		*load;
	GpmSession	*session;
	GpmIdleMode	 mode;
	guint		 timeout_dim;		/* in seconds */
	guint		 timeout_blank;		/* in seconds */
	guint		 timeout_sleep;		/* in seconds */
	guint		 timeout_blank_id;
	guint		 timeout_sleep_id;
	gboolean	 x_idle;
	gboolean	 check_type_cpu;
};

enum {
	IDLE_CHANGED,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };
static gpointer gpm_idle_object = NULL;

G_DEFINE_TYPE (GpmIdle, gpm_idle, G_TYPE_OBJECT)

/**
 * gpm_idle_mode_to_string:
 **/
static const gchar *
gpm_idle_mode_to_string (GpmIdleMode mode)
{
	if (mode == GPM_IDLE_MODE_NORMAL)
		return "normal";
	if (mode == GPM_IDLE_MODE_DIM)
		return "dim";
	if (mode == GPM_IDLE_MODE_BLANK)
		return "blank";
	if (mode == GPM_IDLE_MODE_SLEEP)
		return "sleep";
	return "unknown";
}

/**
 * gpm_idle_set_mode:
 * @mode: The new mode, e.g. GPM_IDLE_MODE_SLEEP
 **/
static void
gpm_idle_set_mode (GpmIdle *idle, GpmIdleMode mode)
{
	g_return_if_fail (GPM_IS_IDLE (idle));

	if (mode != idle->priv->mode) {
		idle->priv->mode = mode;
		egg_debug ("Doing a state transition: %s", gpm_idle_mode_to_string (mode));
		g_signal_emit (idle, signals [IDLE_CHANGED], 0, mode);
	}
}

/**
 * gpm_idle_set_check_cpu:
 * @check_type_cpu: If we should check the CPU before mode becomes
 *		    GPM_IDLE_MODE_SLEEP and the event is done.
 **/
void
gpm_idle_set_check_cpu (GpmIdle *idle, gboolean check_type_cpu)
{
	g_return_if_fail (GPM_IS_IDLE (idle));
	egg_debug ("Setting the CPU load check to %i", check_type_cpu);
	idle->priv->check_type_cpu = check_type_cpu;
}

/**
 * gpm_idle_get_mode:
 * Return value: The current mode, e.g. GPM_IDLE_MODE_SLEEP
 **/
GpmIdleMode
gpm_idle_get_mode (GpmIdle *idle)
{
	return idle->priv->mode;
}

/**
 * gpm_idle_blank_cb:
 **/
static gboolean
gpm_idle_blank_cb (GpmIdle *idle)
{
	if (idle->priv->mode > GPM_IDLE_MODE_BLANK) {
		egg_debug ("ignoring current mode %s", gpm_idle_mode_to_string (idle->priv->mode));
		return FALSE;
	}
	gpm_idle_set_mode (idle, GPM_IDLE_MODE_BLANK);
	return FALSE;
}

/**
 * gpm_idle_sleep_cb:
 **/
static gboolean
gpm_idle_sleep_cb (GpmIdle *idle)
{
	gdouble load;
	gboolean ret = FALSE;

	/* get our computed load value */
	if (idle->priv->check_type_cpu) {
		load = gpm_load_get_current (idle->priv->load);
		if (load > GPM_IDLE_CPU_LIMIT) {
			/* check if system is "idle" enough */
			egg_debug ("Detected that the CPU is busy");
			ret = TRUE;
			goto out;
		}
	}
	gpm_idle_set_mode (idle, GPM_IDLE_MODE_SLEEP);
out:
	return ret;
}

/**
 * gpm_idle_evaluate:
 **/
static void
gpm_idle_evaluate (GpmIdle *idle)
{
	gboolean is_idle;
	gboolean is_idle_inhibited;
	gboolean is_suspend_inhibited;

	is_idle = gpm_session_get_idle (idle->priv->session);
	is_idle_inhibited = gpm_session_get_idle_inhibited (idle->priv->session);
	is_suspend_inhibited = gpm_session_get_suspend_inhibited (idle->priv->session);
	egg_debug ("session_idle=%i, idle_inhibited=%i, suspend_inhibited=%i, x_idle=%i", is_idle, is_idle_inhibited, is_suspend_inhibited, idle->priv->x_idle);

	/* check we are really idle */
	if (!idle->priv->x_idle) {
		gpm_idle_set_mode (idle, GPM_IDLE_MODE_NORMAL);
		egg_debug ("X not idle");
		if (idle->priv->timeout_blank_id != 0) {
			g_source_remove (idle->priv->timeout_blank_id);
			idle->priv->timeout_blank_id = 0;
		}
		if (idle->priv->timeout_sleep_id != 0) {
			g_source_remove (idle->priv->timeout_sleep_id);
			idle->priv->timeout_sleep_id = 0;
		}
		goto out;
	}

	/* are we inhibited from going idle */
	if (is_idle_inhibited) {
		egg_debug ("inhibited, so using normal state");
		gpm_idle_set_mode (idle, GPM_IDLE_MODE_NORMAL);
		if (idle->priv->timeout_blank_id != 0) {
			g_source_remove (idle->priv->timeout_blank_id);
			idle->priv->timeout_blank_id = 0;
		}
		if (idle->priv->timeout_sleep_id != 0) {
			g_source_remove (idle->priv->timeout_sleep_id);
			idle->priv->timeout_sleep_id = 0;
		}
		goto out;
	}

	/* normal to dim */
	if (idle->priv->mode == GPM_IDLE_MODE_NORMAL) {
		egg_debug ("normal to dim");
		gpm_idle_set_mode (idle, GPM_IDLE_MODE_DIM);
	}

	/* set up blank callback even when session is not idle,
	 * but only if we actually want to blank. */
	if (idle->priv->timeout_blank_id == 0 &&
	    idle->priv->timeout_blank != 0) {
		egg_debug ("setting up blank callback for %is", idle->priv->timeout_blank);
		idle->priv->timeout_blank_id = g_timeout_add_seconds (idle->priv->timeout_blank, (GSourceFunc) gpm_idle_blank_cb, idle);
	}

	/* are we inhibited from sleeping */
	if (is_suspend_inhibited) {
		egg_debug ("suspend inhibited");
		if (idle->priv->timeout_sleep_id != 0) {
			g_source_remove (idle->priv->timeout_sleep_id);
			idle->priv->timeout_sleep_id = 0;
		}
	} else if (is_idle) {
	/* only do the sleep timeout when the session is idle and we aren't inhibited from sleeping */
		if (idle->priv->timeout_sleep_id == 0 &&
		    idle->priv->timeout_sleep != 0) {
			egg_debug ("setting up sleep callback %is", idle->priv->timeout_sleep);
			idle->priv->timeout_sleep_id = g_timeout_add_seconds (idle->priv->timeout_sleep, (GSourceFunc) gpm_idle_sleep_cb, idle);
		}
	}
out:
	return;
}

/**
 *  gpm_idle_adjust_timeout_dim:
 *  @idle_time: The new timeout we want to set, in seconds.
 *  @timeout: Current idle time, in seconds.
 *
 *  On slow machines, or machines that have lots to load duing login,
 *  the current idle time could be bigger than the requested timeout.
 *  In this case the scheduled idle timeout will never fire, unless
 *  some user activity (keyboard, mouse) resets the current idle time.
 *  Instead of relying on user activity to correct this issue, we need
 *  to adjust timeout, as related to current idle time, so the idle
 *  timeout will fire as designed.
 *
 *  Return value: timeout to set, adjusted acccording to current idle time.
 **/
static guint
gpm_idle_adjust_timeout_dim (guint idle_time, guint timeout)
{
	/* allow 2 sec margin for messaging delay. */
	idle_time += 2;

	/* Double timeout until it's larger than current idle time.
	 * Give up for ultra slow machines. (86400 sec = 24 hours) */
	while (timeout < idle_time && timeout < 86400 && timeout > 0) {
		timeout *= 2;
	}
	return timeout;
}

/**
 * gpm_idle_set_timeout_dim:
 * @timeout: The new timeout we want to set, in seconds
 **/
gboolean
gpm_idle_set_timeout_dim (GpmIdle *idle, guint timeout)
{
	gint64 idle_time_in_msec;
	guint timeout_adjusted;

	g_return_val_if_fail (GPM_IS_IDLE (idle), FALSE);

	idle_time_in_msec = egg_idletime_get_time (idle->priv->idletime);
	timeout_adjusted  = gpm_idle_adjust_timeout_dim (idle_time_in_msec / 1000, timeout);
	egg_debug ("Current idle time=%lldms, timeout was %us, becomes %us after adjustment",
		   (long long int)idle_time_in_msec, timeout, timeout_adjusted);
	timeout = timeout_adjusted;

	egg_debug ("Setting dim idle timeout: %ds", timeout);
	if (idle->priv->timeout_dim != timeout) {
		idle->priv->timeout_dim = timeout;

		if (timeout > 0)
			egg_idletime_alarm_set (idle->priv->idletime, GPM_IDLE_IDLETIME_ID, timeout * 1000);
		else
			egg_idletime_alarm_remove (idle->priv->idletime, GPM_IDLE_IDLETIME_ID);
	}
	return TRUE;
}

/**
 * gpm_idle_set_timeout_blank:
 * @timeout: The new timeout we want to set, in seconds
 **/
gboolean
gpm_idle_set_timeout_blank (GpmIdle *idle, guint timeout)
{
	g_return_val_if_fail (GPM_IS_IDLE (idle), FALSE);

	egg_debug ("Setting blank idle timeout: %ds", timeout);
	if (idle->priv->timeout_blank != timeout) {
		idle->priv->timeout_blank = timeout;
		gpm_idle_evaluate (idle);
	}
	return TRUE;
}

/**
 * gpm_idle_set_timeout_sleep:
 * @timeout: The new timeout we want to set, in seconds
 **/
gboolean
gpm_idle_set_timeout_sleep (GpmIdle *idle, guint timeout)
{
	g_return_val_if_fail (GPM_IS_IDLE (idle), FALSE);

	egg_debug ("Setting sleep idle timeout: %ds", timeout);
	if (idle->priv->timeout_sleep != timeout) {
		idle->priv->timeout_sleep = timeout;
		gpm_idle_evaluate (idle);
	}
	return TRUE;
}

/**
 * gpm_idle_session_idle_changed_cb:
 * @is_idle: If the session is idle
 *
 * The SessionIdleChanged callback from mate-session.
 **/
static void
gpm_idle_session_idle_changed_cb (GpmSession *session, gboolean is_idle, GpmIdle *idle)
{
	egg_debug ("Received mate session idle changed: %i", is_idle);
	gpm_idle_evaluate (idle);
}

/**
 * gpm_idle_session_inhibited_changed_cb:
 **/
static void
gpm_idle_session_inhibited_changed_cb (GpmSession *session, gboolean is_idle_inhibited, gboolean is_suspend_inhibited, GpmIdle *idle)
{
	egg_debug ("Received mate session inhibited changed: idle=(%i), suspend=(%i)", is_idle_inhibited, is_suspend_inhibited);
	gpm_idle_evaluate (idle);
}

/**
 * gpm_idle_idletime_alarm_expired_cb:
 *
 * We're idle, something timed out
 **/
static void
gpm_idle_idletime_alarm_expired_cb (EggIdletime *idletime, guint alarm_id, GpmIdle *idle)
{
	egg_debug ("idletime alarm: %i", alarm_id);

	/* set again */
	idle->priv->x_idle = TRUE;
	gpm_idle_evaluate (idle);
}

/**
 * gpm_idle_idletime_reset_cb:
 *
 * We're no longer idle, the user moved
 **/
static void
gpm_idle_idletime_reset_cb (EggIdletime *idletime, GpmIdle *idle)
{
	egg_debug ("idletime reset");

	idle->priv->x_idle = FALSE;
	gpm_idle_evaluate (idle);
}

/**
 * gpm_idle_finalize:
 * @object: This class instance
 **/
static void
gpm_idle_finalize (GObject *object)
{
	GpmIdle *idle;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GPM_IS_IDLE (object));

	idle = GPM_IDLE (object);

	g_return_if_fail (idle->priv != NULL);

	if (idle->priv->timeout_blank_id != 0)
		g_source_remove (idle->priv->timeout_blank_id);
	if (idle->priv->timeout_sleep_id != 0)
		g_source_remove (idle->priv->timeout_sleep_id);

	g_object_unref (idle->priv->load);
	g_object_unref (idle->priv->session);

	egg_idletime_alarm_remove (idle->priv->idletime, GPM_IDLE_IDLETIME_ID);
	g_object_unref (idle->priv->idletime);

	G_OBJECT_CLASS (gpm_idle_parent_class)->finalize (object);
}

/**
 * gpm_idle_class_init:
 * @klass: This class instance
 **/
static void
gpm_idle_class_init (GpmIdleClass *klass)
{
	GObjectClass   *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gpm_idle_finalize;

	signals [IDLE_CHANGED] =
		g_signal_new ("idle-changed",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GpmIdleClass, idle_changed),
			      NULL, NULL, g_cclosure_marshal_VOID__INT,
			      G_TYPE_NONE, 1, G_TYPE_INT);

	g_type_class_add_private (klass, sizeof (GpmIdlePrivate));
}

/**
 * gpm_idle_init:
 *
 * Gets a DBUS connection, and aquires the session connection so we can
 * get session changed events.
 *
 **/
static void
gpm_idle_init (GpmIdle *idle)
{
	idle->priv = GPM_IDLE_GET_PRIVATE (idle);

	idle->priv->timeout_dim = G_MAXUINT;
	idle->priv->timeout_blank = G_MAXUINT;
	idle->priv->timeout_sleep = G_MAXUINT;
	idle->priv->timeout_blank_id = 0;
	idle->priv->timeout_sleep_id = 0;
	idle->priv->x_idle = FALSE;
	idle->priv->load = gpm_load_new ();
	idle->priv->session = gpm_session_new ();
	g_signal_connect (idle->priv->session, "idle-changed", G_CALLBACK (gpm_idle_session_idle_changed_cb), idle);
	g_signal_connect (idle->priv->session, "inhibited-changed", G_CALLBACK (gpm_idle_session_inhibited_changed_cb), idle);

	idle->priv->idletime = egg_idletime_new ();
	g_signal_connect (idle->priv->idletime, "reset", G_CALLBACK (gpm_idle_idletime_reset_cb), idle);
	g_signal_connect (idle->priv->idletime, "alarm-expired", G_CALLBACK (gpm_idle_idletime_alarm_expired_cb), idle);

	gpm_idle_evaluate (idle);
}

/**
 * gpm_idle_new:
 * Return value: A new GpmIdle instance.
 **/
GpmIdle *
gpm_idle_new (void)
{
	if (gpm_idle_object != NULL) {
		g_object_ref (gpm_idle_object);
	} else {
		gpm_idle_object = g_object_new (GPM_TYPE_IDLE, NULL);
		g_object_add_weak_pointer (gpm_idle_object, &gpm_idle_object);
	}
	return GPM_IDLE (gpm_idle_object);
}

/***************************************************************************
 ***                          MAKE CHECK TESTS                           ***
 ***************************************************************************/
#ifdef EGG_TEST
#include "egg-test.h"
#include "gpm-dpms.h"

static GpmIdleMode _mode = 0;

static void
gpm_idle_test_idle_changed_cb (GpmIdle *idle, GpmIdleMode mode, EggTest *test)
{
	_mode = mode;
	egg_debug ("idle-changed %s", gpm_idle_mode_to_string (mode));
	egg_test_loop_quit (test);
}

static gboolean
gpm_idle_test_delay_cb (EggTest *test)
{
	egg_warning ("timing out");
	egg_test_loop_quit (test);
	return FALSE;
}

void
gpm_idle_test (gpointer data)
{
	GpmIdle *idle;
	gboolean ret;
	EggTest *test = (EggTest *) data;
	GpmIdleMode mode;
	GpmDpms *dpms;

	if (!egg_test_start (test, "GpmIdle"))
		return;

	/************************************************************/
	egg_test_title (test, "get object");
	idle = gpm_idle_new ();
	if (idle != NULL)
		egg_test_success (test, NULL);
	else
		egg_test_failed (test, "got no object");

	/* set up defaults */
	gpm_idle_set_check_cpu (idle, FALSE);
	gpm_idle_set_timeout_dim (idle, 4);
	gpm_idle_set_timeout_blank (idle, 5);
	gpm_idle_set_timeout_sleep (idle, 15);
	g_signal_connect (idle, "idle-changed",
			  G_CALLBACK (gpm_idle_test_idle_changed_cb), test);

	/************************************************************/
	egg_test_title (test, "check cpu type");
	egg_test_assert (test, (idle->priv->check_type_cpu == FALSE));

	/************************************************************/
	egg_test_title (test, "check timeout dim");
	egg_test_assert (test, (idle->priv->timeout_dim == 4));

	/************************************************************/
	egg_test_title (test, "check timeout blank");
	egg_test_assert (test, (idle->priv->timeout_blank == 5));

	/************************************************************/
	egg_test_title (test, "check timeout sleep");
	egg_test_assert (test, (idle->priv->timeout_sleep == 15));

	/************************************************************/
	egg_test_title (test, "check x_idle");
	egg_test_assert (test, (idle->priv->x_idle == FALSE));

	/************************************************************/
	egg_test_title (test, "check blank id");
	egg_test_assert (test, (idle->priv->timeout_blank_id == 0));

	/************************************************************/
	egg_test_title (test, "check sleep id");
	egg_test_assert (test, (idle->priv->timeout_sleep_id == 0));

	/************************************************************/
	egg_test_title (test, "check normal at startup");
	mode = gpm_idle_get_mode (idle);
	if (mode == GPM_IDLE_MODE_NORMAL)
		egg_test_success (test, NULL);
	else
		egg_test_failed (test, "mode: %s", gpm_idle_mode_to_string (mode));

	/************************************************************/
	g_print ("*****************************\n");
	g_print ("*** DO NOT MOVE THE MOUSE ***\n");
	g_print ("*****************************\n");
	egg_test_loop_wait (test, 2000 + 10000);
	egg_test_loop_check (test);

	/************************************************************/
	egg_test_title (test, "check callback mode");
	if (_mode == GPM_IDLE_MODE_DIM)
		egg_test_success (test, NULL);
	else
		egg_test_failed (test, "mode: %s", gpm_idle_mode_to_string (mode));

	/************************************************************/
	egg_test_title (test, "check current mode");
	mode = gpm_idle_get_mode (idle);
	if (mode == GPM_IDLE_MODE_DIM)
		egg_test_success (test, NULL);
	else
		egg_test_failed (test, "mode: %s", gpm_idle_mode_to_string (mode));

	/************************************************************/
	egg_test_title (test, "check x_idle");
	egg_test_assert (test, (idle->priv->x_idle == TRUE));

	/************************************************************/
	egg_test_title (test, "check blank id");
	egg_test_assert (test, (idle->priv->timeout_blank_id != 0));

	/************************************************************/
	egg_test_title (test, "check sleep id");
	egg_test_assert (test, (idle->priv->timeout_sleep_id == 0));

	/************************************************************/
	egg_test_loop_wait (test, 5000 + 1000);
	egg_test_loop_check (test);

	/************************************************************/
	egg_test_title (test, "check callback mode");
	if (_mode == GPM_IDLE_MODE_BLANK)
		egg_test_success (test, NULL);
	else
		egg_test_failed (test, "mode: %s", gpm_idle_mode_to_string (mode));

	/************************************************************/
	egg_test_title (test, "check current mode");
	mode = gpm_idle_get_mode (idle);
	if (mode == GPM_IDLE_MODE_BLANK)
		egg_test_success (test, NULL);
	else
		egg_test_failed (test, "mode: %s", gpm_idle_mode_to_string (mode));

	/************************************************************/
	g_print ("**********************\n");
	g_print ("*** MOVE THE MOUSE ***\n");
	g_print ("**********************\n");
	egg_test_loop_wait (test, G_MAXUINT);
	egg_test_loop_check (test);

	/************************************************************/
	egg_test_title (test, "check callback mode");
	if (_mode == GPM_IDLE_MODE_NORMAL)
		egg_test_success (test, NULL);
	else
		egg_test_failed (test, "mode: %s", gpm_idle_mode_to_string (mode));

	/************************************************************/
	egg_test_title (test, "check current mode");
	mode = gpm_idle_get_mode (idle);
	if (mode == GPM_IDLE_MODE_NORMAL)
		egg_test_success (test, NULL);
	else
		egg_test_failed (test, "mode: %s", gpm_idle_mode_to_string (mode));

	/************************************************************/
	egg_test_title (test, "check x_idle");
	egg_test_assert (test, (idle->priv->x_idle == FALSE));

	/************************************************************/
	egg_test_title (test, "check blank id");
	egg_test_assert (test, (idle->priv->timeout_blank_id == 0));

	/************************************************************/
	g_print ("*****************************\n");
	g_print ("*** DO NOT MOVE THE MOUSE ***\n");
	g_print ("*****************************\n");
	egg_test_loop_wait (test, 4000 + 1500);
	egg_test_loop_check (test);

	/************************************************************/
	egg_test_title (test, "check current mode");
	mode = gpm_idle_get_mode (idle);
	if (mode == GPM_IDLE_MODE_DIM)
		egg_test_success (test, NULL);
	else
		egg_test_failed (test, "mode: %s", gpm_idle_mode_to_string (mode));

	/************************************************************/
	egg_test_title (test, "check x_idle");
	egg_test_assert (test, (idle->priv->x_idle == TRUE));

	egg_test_loop_wait (test, 15000);
	egg_test_loop_check (test);

	/************************************************************/
	egg_test_title (test, "check current mode");
	mode = gpm_idle_get_mode (idle);
	if (mode == GPM_IDLE_MODE_BLANK)
		egg_test_success (test, NULL);
	else
		egg_test_failed (test, "mode: %s", gpm_idle_mode_to_string (mode));

	/************************************************************/
	egg_test_title (test, "set dpms off");
	dpms = gpm_dpms_new ();
	ret = gpm_dpms_set_mode (dpms, GPM_DPMS_MODE_OFF, NULL);
	egg_test_assert (test, ret);

	/* wait for normal event to be suppressed */
	g_timeout_add (2000, (GSourceFunc) gpm_idle_test_delay_cb, test);
	egg_test_loop_wait (test, G_MAXUINT);
	egg_test_loop_check (test);

	/************************************************************/
	egg_test_title (test, "check current mode");
	mode = gpm_idle_get_mode (idle);
	if (mode == GPM_IDLE_MODE_BLANK)
		egg_test_success (test, NULL);
	else
		egg_test_failed (test, "mode: %s", gpm_idle_mode_to_string (mode));

	/************************************************************/
	egg_test_title (test, "check x_idle");
	egg_test_assert (test, (idle->priv->x_idle == TRUE));

	gpm_dpms_set_mode (dpms, GPM_DPMS_MODE_ON, NULL);

	g_object_unref (idle);
	g_object_unref (dpms);

	egg_test_end (test);
}

#endif

