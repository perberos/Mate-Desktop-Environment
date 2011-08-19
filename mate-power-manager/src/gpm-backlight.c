/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2005-2009 Richard Hughes <richard@hughsie.com>
 * Copyright (C) 2005 William Jon McCann <mccann@jhu.edu>
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
#include <math.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include <glib/gi18n.h>
#include <dbus/dbus-glib.h>
#include <mateconf/mateconf-client.h>
#include <libupower-glib/upower.h>

#include "gpm-button.h"
#include "gpm-backlight.h"
#include "gpm-brightness.h"
#include "gpm-control.h"
#include "gpm-common.h"
#include "egg-debug.h"
#include "gsd-media-keys-window.h"
#include "gpm-dpms.h"
#include "gpm-idle.h"
#include "gpm-marshal.h"
#include "gpm-stock-icons.h"
#include "gpm-prefs-server.h"
#include "egg-console-kit.h"

#define GPM_BACKLIGHT_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GPM_TYPE_BACKLIGHT, GpmBacklightPrivate))

struct GpmBacklightPrivate
{
	UpClient		*client;
	GpmBrightness		*brightness;
	GpmButton		*button;
	MateConfClient		*conf;
	GtkWidget		*popup;
	GpmControl		*control;
	GpmDpms			*dpms;
	GpmIdle			*idle;
	EggConsoleKit		*consolekit;
	gboolean		 can_dim;
	gboolean		 system_is_idle;
	GTimer			*idle_timer;
	guint			 idle_dim_timeout;
	guint			 master_percentage;
};

enum {
	BRIGHTNESS_CHANGED,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (GpmBacklight, gpm_backlight, G_TYPE_OBJECT)

/**
 * gpm_backlight_error_quark:
 * Return value: Our personal error quark.
 **/
GQuark
gpm_backlight_error_quark (void)
{
	static GQuark quark = 0;
	if (!quark)
		quark = g_quark_from_static_string ("gpm_backlight_error");
	return quark;
}

/**
 * gpm_backlight_get_brightness:
 **/
gboolean
gpm_backlight_get_brightness (GpmBacklight *backlight, guint *brightness, GError **error)
{
	guint level;
	gboolean ret;
	g_return_val_if_fail (backlight != NULL, FALSE);
	g_return_val_if_fail (GPM_IS_BACKLIGHT (backlight), FALSE);
	g_return_val_if_fail (brightness != NULL, FALSE);

	/* check if we have the hw */
	if (backlight->priv->can_dim == FALSE) {
		g_set_error_literal (error, gpm_backlight_error_quark (),
				      GPM_BACKLIGHT_ERROR_HARDWARE_NOT_PRESENT,
				      "Dim capable hardware not present");
		return FALSE;
	}

	/* gets the current brightness */
	ret = gpm_brightness_get (backlight->priv->brightness, &level);
	if (ret) {
		*brightness = level;
	} else {
		g_set_error_literal (error, gpm_backlight_error_quark (),
				      GPM_BACKLIGHT_ERROR_DATA_NOT_AVAILABLE,
				      "Data not available");
	}
	return ret;
}

/**
 * gpm_backlight_set_brightness:
 **/
gboolean
gpm_backlight_set_brightness (GpmBacklight *backlight, guint percentage, GError **error)
{
	gboolean ret;
	gboolean hw_changed;

	g_return_val_if_fail (backlight != NULL, FALSE);
	g_return_val_if_fail (GPM_IS_BACKLIGHT (backlight), FALSE);

	/* check if we have the hw */
	if (backlight->priv->can_dim == FALSE) {
		g_set_error_literal (error, gpm_backlight_error_quark (),
				      GPM_BACKLIGHT_ERROR_HARDWARE_NOT_PRESENT,
				      "Dim capable hardware not present");
		return FALSE;
	}

	/* just set the master percentage for now, don't try to be clever */
	backlight->priv->master_percentage = percentage;

	/* sets the current policy brightness */
	ret = gpm_brightness_set (backlight->priv->brightness, percentage, &hw_changed);
	if (!ret) {
		g_set_error_literal (error, gpm_backlight_error_quark (),
				      GPM_BACKLIGHT_ERROR_GENERAL,
				      "Cannot set policy brightness");
	}
	/* we emit a signal for the brightness applet */
	if (ret && hw_changed) {
		egg_debug ("emitting brightness-changed : %i", percentage);
		g_signal_emit (backlight, signals [BRIGHTNESS_CHANGED], 0, percentage);
	}
	return ret;
}

/**
 * gpm_backlight_dialog_init:
 *
 * Initialises the popup, and makes sure that it matches the compositing of the screen.
 **/
static void
gpm_backlight_dialog_init (GpmBacklight *backlight)
{
	if (backlight->priv->popup != NULL
	    && !gsd_media_keys_window_is_valid (GSD_MEDIA_KEYS_WINDOW (backlight->priv->popup))) {
		gtk_widget_destroy (backlight->priv->popup);
		backlight->priv->popup = NULL;
	}

	if (backlight->priv->popup == NULL) {
		backlight->priv->popup= gsd_media_keys_window_new ();
		gsd_media_keys_window_set_action_custom (GSD_MEDIA_KEYS_WINDOW (backlight->priv->popup),
							 "gpm-brightness-lcd",
							 TRUE);
		gtk_window_set_position (GTK_WINDOW (backlight->priv->popup), GTK_WIN_POS_NONE);
	}
}

/**
 * gpm_backlight_dialog_show:
 *
 * Show the brightness popup, and place it nicely on the screen.
 **/
static void
gpm_backlight_dialog_show (GpmBacklight *backlight)
{
	int            orig_w;
	int            orig_h;
	int            screen_w;
	int            screen_h;
	int            x;
	int            y;
	int            pointer_x;
	int            pointer_y;
	GtkRequisition win_req;
	GdkScreen     *pointer_screen;
	GdkRectangle   geometry;
	int            monitor;

	/*
	 * get the window size
	 * if the window hasn't been mapped, it doesn't necessarily
	 * know its true size, yet, so we need to jump through hoops
	 */
	gtk_window_get_default_size (GTK_WINDOW (backlight->priv->popup), &orig_w, &orig_h);
	gtk_widget_size_request (backlight->priv->popup, &win_req);

	if (win_req.width > orig_w) {
		orig_w = win_req.width;
	}
	if (win_req.height > orig_h) {
		orig_h = win_req.height;
	}

	pointer_screen = NULL;
	gdk_display_get_pointer (gtk_widget_get_display (backlight->priv->popup),
				 &pointer_screen,
				 &pointer_x,
				 &pointer_y,
				 NULL);
	monitor = gdk_screen_get_monitor_at_point (pointer_screen,
						   pointer_x,
						   pointer_y);

	gdk_screen_get_monitor_geometry (pointer_screen,
					 monitor,
					 &geometry);

	screen_w = geometry.width;
	screen_h = geometry.height;

	x = ((screen_w - orig_w) / 2) + geometry.x;
	y = geometry.y + (screen_h / 2) + (screen_h / 2 - orig_h) / 2;

	gtk_window_move (GTK_WINDOW (backlight->priv->popup), x, y);

	gtk_widget_show (backlight->priv->popup);

	gdk_display_sync (gtk_widget_get_display (backlight->priv->popup));
}

/**
 * gpm_common_sum_scale:
 *
 * Finds the average between value1 and value2 set on a scale factor
 **/
inline static gfloat
gpm_common_sum_scale (gfloat value1, gfloat value2, gfloat factor)
{
	gfloat diff;
	diff = value1 - value2;
	return value2 + (diff * factor);
}

/**
 * gpm_backlight_brightness_evaluate_and_set:
 **/
static gboolean
gpm_backlight_brightness_evaluate_and_set (GpmBacklight *backlight, gboolean interactive)
{
	gfloat brightness;
	gfloat scale;
	gboolean ret;
	gboolean on_battery;
	gboolean do_laptop_lcd;
	gboolean enable_action;
	gboolean battery_reduce;
	gboolean hw_changed;
	guint value;
	guint old_value;

	if (backlight->priv->can_dim == FALSE) {
		egg_warning ("no dimming hardware");
		return FALSE;
	}

	do_laptop_lcd = mateconf_client_get_bool (backlight->priv->conf, GPM_CONF_BACKLIGHT_ENABLE, NULL);
	if (do_laptop_lcd == FALSE) {
		egg_warning ("policy is no dimming");
		return FALSE;
	}

	/* get the last set brightness */
	brightness = backlight->priv->master_percentage / 100.0f;
	egg_debug ("1. main brightness %f", brightness);

	/* get battery status */
	g_object_get (backlight->priv->client,
		      "on-battery", &on_battery,
		      NULL);

	/* reduce if on battery power if we should */
	battery_reduce = mateconf_client_get_bool (backlight->priv->conf, GPM_CONF_BACKLIGHT_BATTERY_REDUCE, NULL);
	if (on_battery && battery_reduce) {
		value = mateconf_client_get_int (backlight->priv->conf, GPM_CONF_BACKLIGHT_BRIGHTNESS_DIM_BATT, NULL);
		if (value > 100) {
			egg_warning ("cannot use battery brightness value %i, correcting to 50", value);
			value = 50;
		}
		scale = (100 - value) / 100.0f;
		brightness *= scale;
	} else {
		scale = 1.0f;
	}
	egg_debug ("2. battery scale %f, brightness %f", scale, brightness);

	/* reduce if system is momentarily idle */
	if (!on_battery)
		enable_action = mateconf_client_get_bool (backlight->priv->conf, GPM_CONF_BACKLIGHT_IDLE_DIM_AC, NULL);
	else
		enable_action = mateconf_client_get_bool (backlight->priv->conf, GPM_CONF_BACKLIGHT_IDLE_DIM_BATT, NULL);
	if (enable_action && backlight->priv->system_is_idle) {
		value = mateconf_client_get_int (backlight->priv->conf, GPM_CONF_BACKLIGHT_IDLE_BRIGHTNESS, NULL);
		if (value > 100) {
			egg_warning ("cannot use idle brightness value %i, correcting to 50", value);
			value = 50;
		}
		scale = value / 100.0f;
		brightness *= scale;
	} else {
		scale = 1.0f;
	}
	egg_debug ("3. idle scale %f, brightness %f", scale, brightness);

	/* convert to percentage */
	value = (guint) ((brightness * 100.0f) + 0.5);

	/* only do stuff if the brightness is different */
	gpm_brightness_get (backlight->priv->brightness, &old_value);
	if (old_value == value) {
		egg_debug ("values are the same, no action");
		return FALSE;
	}

	/* only show dialog if interactive */
	if (interactive) {
		gpm_backlight_dialog_init (backlight);
		gsd_media_keys_window_set_volume_level (GSD_MEDIA_KEYS_WINDOW (backlight->priv->popup),
							round (brightness));
		gpm_backlight_dialog_show (backlight);
	}

	ret = gpm_brightness_set (backlight->priv->brightness, value, &hw_changed);
	/* we emit a signal for the brightness applet */
	if (ret && hw_changed) {
		egg_debug ("emitting brightness-changed : %i", value);
		g_signal_emit (backlight, signals [BRIGHTNESS_CHANGED], 0, value);
	}
	return TRUE;
}

/**
 * gpm_conf_mateconf_key_changed_cb:
 *
 * We might have to do things when the mateconf keys change; do them here.
 **/
static void
gpm_conf_mateconf_key_changed_cb (MateConfClient *client, guint cnxn_id, MateConfEntry *entry, GpmBacklight *backlight)
{
	MateConfValue *value;
	gboolean on_battery;

	value = mateconf_entry_get_value (entry);
	if (value == NULL)
		return;

	/* get battery status */
	g_object_get (backlight->priv->client,
		      "on-battery", &on_battery,
		      NULL);

	if (!on_battery && strcmp (entry->key, GPM_CONF_BACKLIGHT_BRIGHTNESS_AC) == 0) {
		backlight->priv->master_percentage = mateconf_value_get_int (value);
		gpm_backlight_brightness_evaluate_and_set (backlight, FALSE);

	} else if (on_battery && strcmp (entry->key, GPM_CONF_BACKLIGHT_BRIGHTNESS_DIM_BATT) == 0) {
		gpm_backlight_brightness_evaluate_and_set (backlight, FALSE);

	} else if (strcmp (entry->key, GPM_CONF_BACKLIGHT_IDLE_DIM_AC) == 0 ||
	         strcmp (entry->key, GPM_CONF_BACKLIGHT_ENABLE) == 0 ||
	         strcmp (entry->key, GPM_CONF_TIMEOUT_SLEEP_DISPLAY_BATT) == 0 ||
	         strcmp (entry->key, GPM_CONF_BACKLIGHT_BATTERY_REDUCE) == 0 ||
	         strcmp (entry->key, GPM_CONF_BACKLIGHT_IDLE_BRIGHTNESS) == 0) {
		gpm_backlight_brightness_evaluate_and_set (backlight, FALSE);

	} else if (strcmp (entry->key, GPM_CONF_BACKLIGHT_IDLE_DIM_TIME) == 0) {
		backlight->priv->idle_dim_timeout = mateconf_value_get_int (value);
		gpm_idle_set_timeout_dim (backlight->priv->idle, backlight->priv->idle_dim_timeout);
	} else {
		egg_debug ("unknown key %s", entry->key);
	}
}

/**
 * gpm_backlight_client_changed_cb:
 * @client: The up_client class instance
 * @backlight: This class instance
 *
 * Does the actions when the ac power source is inserted/removed.
 **/
static void
gpm_backlight_client_changed_cb (UpClient *client, GpmBacklight *backlight)
{
	gpm_backlight_brightness_evaluate_and_set (backlight, FALSE);
}

/**
 * gpm_backlight_button_pressed_cb:
 * @power: The power class instance
 * @type: The button type, e.g. "power"
 * @state: The state, where TRUE is depressed or closed
 * @brightness: This class instance
 **/
static void
gpm_backlight_button_pressed_cb (GpmButton *button, const gchar *type, GpmBacklight *backlight)
{
	gboolean ret;
	GError *error = NULL;
	guint percentage;
	gboolean hw_changed;
	egg_debug ("Button press event type=%s", type);

	if (strcmp (type, GPM_BUTTON_BRIGHT_UP) == 0) {
		/* go up one step */
		ret = gpm_brightness_up (backlight->priv->brightness, &hw_changed);

		/* show the new value */
		if (ret) {
			gpm_brightness_get (backlight->priv->brightness, &percentage);
			gpm_backlight_dialog_init (backlight);
			gsd_media_keys_window_set_volume_level (GSD_MEDIA_KEYS_WINDOW (backlight->priv->popup),
								percentage);
			gpm_backlight_dialog_show (backlight);
			/* save the new percentage */
			backlight->priv->master_percentage = percentage;
		}
		/* we emit a signal for the brightness applet */
		if (ret && hw_changed) {
			egg_debug ("emitting brightness-changed : %i", percentage);
			g_signal_emit (backlight, signals [BRIGHTNESS_CHANGED], 0, percentage);
		}
	} else if (strcmp (type, GPM_BUTTON_BRIGHT_DOWN) == 0) {
		/* go up down step */
		ret = gpm_brightness_down (backlight->priv->brightness, &hw_changed);

		/* show the new value */
		if (ret) {
			gpm_brightness_get (backlight->priv->brightness, &percentage);
			gpm_backlight_dialog_init (backlight);
			gsd_media_keys_window_set_volume_level (GSD_MEDIA_KEYS_WINDOW (backlight->priv->popup),
								percentage);
			gpm_backlight_dialog_show (backlight);
			/* save the new percentage */
			backlight->priv->master_percentage = percentage;
		}
		/* we emit a signal for the brightness applet */
		if (ret && hw_changed) {
			egg_debug ("emitting brightness-changed : %i", percentage);
			g_signal_emit (backlight, signals [BRIGHTNESS_CHANGED], 0, percentage);
		}
	} else if (strcmp (type, GPM_BUTTON_LID_OPEN) == 0) {
		/* make sure we undim when we lift the lid */
		gpm_backlight_brightness_evaluate_and_set (backlight, FALSE);

		/* ensure backlight is on */
		ret = gpm_dpms_set_mode (backlight->priv->dpms, GPM_DPMS_MODE_ON, &error);
		if (!ret) {
			egg_warning ("failed to turn on DPMS: %s", error->message);
			g_error_free (error);
		}
	}
}

/**
 * gpm_backlight_notify_system_idle_changed:
 **/
static gboolean
gpm_backlight_notify_system_idle_changed (GpmBacklight *backlight, gboolean is_idle)
{
	gdouble elapsed;

	/* no point continuing */
	if (backlight->priv->system_is_idle == is_idle) {
		egg_debug ("state not changed");
		return FALSE;
	}

	/* get elapsed time and reset timer */
	elapsed = g_timer_elapsed (backlight->priv->idle_timer, NULL);
	g_timer_reset (backlight->priv->idle_timer);

	if (is_idle == FALSE) {
		egg_debug ("we have just been idle for %lfs", elapsed);

		/* The user immediatly undimmed the screen!
		 * We should double the timeout to avoid this happening again */
		if (elapsed < 10) {
			/* double the event time */
			backlight->priv->idle_dim_timeout *= 2.0;
			egg_debug ("increasing idle dim time to %is", backlight->priv->idle_dim_timeout);
			gpm_idle_set_timeout_dim (backlight->priv->idle, backlight->priv->idle_dim_timeout);
		}

		/* We reset the dimming after 2 minutes of idle,
		 * as the user will have changed tasks */
		if (elapsed > 2*60) {
			/* reset back to our default dimming */
			backlight->priv->idle_dim_timeout =
				mateconf_client_get_int (backlight->priv->conf,
					   GPM_CONF_BACKLIGHT_IDLE_DIM_TIME, NULL);
			egg_debug ("resetting idle dim time to %is", backlight->priv->idle_dim_timeout);
			gpm_idle_set_timeout_dim (backlight->priv->idle, backlight->priv->idle_dim_timeout);
		}
	} else {
		egg_debug ("we were active for %lfs", elapsed);
	}

	egg_debug ("changing powersave idle status to %i", is_idle);
	backlight->priv->system_is_idle = is_idle;
	return TRUE;
}

/**
 * idle_changed_cb:
 * @idle: The idle class instance
 * @mode: The idle mode, e.g. GPM_IDLE_MODE_BLANK
 * @manager: This class instance
 *
 * This callback is called when mate-screensaver detects that the idle state
 * has changed. GPM_IDLE_MODE_BLANK is when the session has become inactive,
 * and GPM_IDLE_MODE_SLEEP is where the session has become inactive, AND the
 * session timeout has elapsed for the idle action.
 **/
static void
idle_changed_cb (GpmIdle *idle, GpmIdleMode mode, GpmBacklight *backlight)
{
	gboolean ret;
	GError *error = NULL;
	gboolean on_battery;
	gchar *dpms_method;
	GpmDpmsMode dpms_mode;

	/* don't dim or undim the screen when the lid is closed */
	if (gpm_button_is_lid_closed (backlight->priv->button))
		return;

	/* don't dim or undim the screen unless we are on the active console */
	if (!egg_console_kit_is_active (backlight->priv->consolekit)) {
		egg_debug ("ignoring as not on active console");
		return;
	}

	if (mode == GPM_IDLE_MODE_NORMAL) {
		/* sync lcd brightness */
		gpm_backlight_notify_system_idle_changed (backlight, FALSE);
		gpm_backlight_brightness_evaluate_and_set (backlight, FALSE);

		/* ensure backlight is on */
		ret = gpm_dpms_set_mode (backlight->priv->dpms, GPM_DPMS_MODE_ON, &error);
		if (!ret) {
			egg_warning ("failed to turn on DPMS: %s", error->message);
			g_error_free (error);
		}

	} else if (mode == GPM_IDLE_MODE_DIM) {

		/* sync lcd brightness */
		gpm_backlight_notify_system_idle_changed (backlight, TRUE);
		gpm_backlight_brightness_evaluate_and_set (backlight, FALSE);

		/* ensure backlight is on */
		ret = gpm_dpms_set_mode (backlight->priv->dpms, GPM_DPMS_MODE_ON, &error);
		if (!ret) {
			egg_warning ("failed to turn on DPMS: %s", error->message);
			g_error_free (error);
		}

	} else if (mode == GPM_IDLE_MODE_BLANK) {

		/* sync lcd brightness */
		gpm_backlight_notify_system_idle_changed (backlight, TRUE);
		gpm_backlight_brightness_evaluate_and_set (backlight, FALSE);

		/* get the DPMS state we're supposed to use on the power state */
		g_object_get (backlight->priv->client,
			      "on-battery", &on_battery,
			      NULL);
		if (!on_battery)
			dpms_method = mateconf_client_get_string (backlight->priv->conf, GPM_CONF_BACKLIGHT_DPMS_METHOD_AC, NULL);
		else
			dpms_method = mateconf_client_get_string (backlight->priv->conf, GPM_CONF_BACKLIGHT_DPMS_METHOD_BATT, NULL);

		/* convert the string types to standard types */
		dpms_mode = gpm_dpms_mode_from_string (dpms_method);

		/* check if method is valid */
		if (dpms_mode == GPM_DPMS_MODE_UNKNOWN || dpms_mode == GPM_DPMS_MODE_ON) {
			egg_warning ("BACKLIGHT method %s unknown. Using OFF.", dpms_method);
			dpms_mode = GPM_DPMS_MODE_OFF;
		}

		/* turn backlight off */
		ret = gpm_dpms_set_mode (backlight->priv->dpms, dpms_mode, &error);
		if (!ret) {
			egg_warning ("failed to change DPMS: %s", error->message);
			g_error_free (error);
		}

		g_free (dpms_method);
	}
}

/**
 * brightness_changed_cb:
 * @brightness: The GpmBrightness class instance
 * @percentage: The new percentage brightness
 * @brightness: This class instance
 *
 * This callback is called when the brightness value changes.
 **/
static void
brightness_changed_cb (GpmBrightness *brightness, guint percentage, GpmBacklight *backlight)
{
	/* save the new percentage */
	backlight->priv->master_percentage = percentage;

	/* we emit a signal for the brightness applet */
	egg_debug ("emitting brightness-changed : %i", percentage);
	g_signal_emit (backlight, signals [BRIGHTNESS_CHANGED], 0, percentage);
}

/**
 * control_resume_cb:
 * @control: The control class instance
 * @power: This power class instance
 *
 * We have to update the caches on resume
 **/
static void
control_resume_cb (GpmControl *control, GpmControlAction action, GpmBacklight *backlight)
{
	gboolean ret;
	GError *error = NULL;

	/* ensure backlight is on */
	ret = gpm_dpms_set_mode (backlight->priv->dpms, GPM_DPMS_MODE_ON, &error);
	if (!ret) {
		egg_warning ("failed to turn on DPMS: %s", error->message);
		g_error_free (error);
	}
}

/**
 * gpm_backlight_finalize:
 **/
static void
gpm_backlight_finalize (GObject *object)
{
	GpmBacklight *backlight;
	g_return_if_fail (object != NULL);
	g_return_if_fail (GPM_IS_BACKLIGHT (object));
	backlight = GPM_BACKLIGHT (object);

	g_timer_destroy (backlight->priv->idle_timer);
	gtk_widget_destroy (backlight->priv->popup);

	g_object_unref (backlight->priv->dpms);
	g_object_unref (backlight->priv->control);
	g_object_unref (backlight->priv->conf);
	g_object_unref (backlight->priv->client);
	g_object_unref (backlight->priv->button);
	g_object_unref (backlight->priv->idle);
	g_object_unref (backlight->priv->brightness);
	g_object_unref (backlight->priv->consolekit);

	g_return_if_fail (backlight->priv != NULL);
	G_OBJECT_CLASS (gpm_backlight_parent_class)->finalize (object);
}

/**
 * gpm_backlight_class_init:
 **/
static void
gpm_backlight_class_init (GpmBacklightClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize	   = gpm_backlight_finalize;

	signals [BRIGHTNESS_CHANGED] =
		g_signal_new ("brightness-changed",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GpmBacklightClass, brightness_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__UINT,
			      G_TYPE_NONE, 1, G_TYPE_UINT);

	g_type_class_add_private (klass, sizeof (GpmBacklightPrivate));
}

/**
 * gpm_backlight_init:
 * @brightness: This brightness class instance
 *
 * initialises the brightness class. NOTE: We expect laptop_panel objects
 * to *NOT* be removed or added during the session.
 * We only control the first laptop_panel object if there are more than one.
 **/
static void
gpm_backlight_init (GpmBacklight *backlight)
{
	gboolean lid_is_present = TRUE;
	GpmPrefsServer *prefs_server;

	backlight->priv = GPM_BACKLIGHT_GET_PRIVATE (backlight);

	/* record our idle time */
	backlight->priv->idle_timer = g_timer_new ();

	/* watch for manual brightness changes (for the popup widget) */
	backlight->priv->brightness = gpm_brightness_new ();
	g_signal_connect (backlight->priv->brightness, "brightness-changed",
			  G_CALLBACK (brightness_changed_cb), backlight);

	/* we use up_client for the ac-adapter-changed signal */
	backlight->priv->client = up_client_new ();
	g_signal_connect (backlight->priv->client, "changed",
			  G_CALLBACK (gpm_backlight_client_changed_cb), backlight);

	/* gets caps */
	backlight->priv->can_dim = gpm_brightness_has_hw (backlight->priv->brightness);

	/* we use UPower to see if we should show the lid UI */
	g_object_get (backlight->priv->client,
		      "lid-is-present", &lid_is_present,
		      NULL);

	/* expose ui in prefs program */
	prefs_server = gpm_prefs_server_new ();
	if (lid_is_present)
		gpm_prefs_server_set_capability (prefs_server, GPM_PREFS_SERVER_LID);
	if (backlight->priv->can_dim)
		gpm_prefs_server_set_capability (prefs_server, GPM_PREFS_SERVER_BACKLIGHT);
	g_object_unref (prefs_server);

	/* watch for dim value changes */
	backlight->priv->conf = mateconf_client_get_default ();

	/* watch mate-power-manager keys */
	mateconf_client_add_dir (backlight->priv->conf, GPM_CONF_DIR, MATECONF_CLIENT_PRELOAD_RECURSIVE, NULL);
	mateconf_client_notify_add (backlight->priv->conf, GPM_CONF_DIR,
				 (MateConfClientNotifyFunc) gpm_conf_mateconf_key_changed_cb,
				 backlight, NULL, NULL);

	/* set the main brightness, this is designed to be updated if the user changes the
	 * brightness so we can undim to the 'correct' value */
	backlight->priv->master_percentage = mateconf_client_get_int (backlight->priv->conf, GPM_CONF_BACKLIGHT_BRIGHTNESS_AC, NULL);

	/* watch for brightness up and down buttons and also check lid state */
	backlight->priv->button = gpm_button_new ();
	g_signal_connect (backlight->priv->button, "button-pressed",
			  G_CALLBACK (gpm_backlight_button_pressed_cb), backlight);

	/* watch for idle mode changes */
	backlight->priv->idle = gpm_idle_new ();
	g_signal_connect (backlight->priv->idle, "idle-changed",
			  G_CALLBACK (idle_changed_cb), backlight);

	/* assumption */
	backlight->priv->system_is_idle = FALSE;
	backlight->priv->idle_dim_timeout = mateconf_client_get_int (backlight->priv->conf, GPM_CONF_BACKLIGHT_IDLE_DIM_TIME, NULL);
	gpm_idle_set_timeout_dim (backlight->priv->idle, backlight->priv->idle_dim_timeout);

	/* use a visual widget */
	backlight->priv->popup = gsd_media_keys_window_new ();
	gsd_media_keys_window_set_action_custom (GSD_MEDIA_KEYS_WINDOW (backlight->priv->popup),
						 "gpm-brightness-lcd",
						 TRUE);
        gtk_window_set_position (GTK_WINDOW (backlight->priv->popup), GTK_WIN_POS_NONE);

	/* DPMS mode poll class */
	backlight->priv->dpms = gpm_dpms_new ();

	/* we refresh DPMS on resume */
	backlight->priv->control = gpm_control_new ();
	g_signal_connect (backlight->priv->control, "resume",
			  G_CALLBACK (control_resume_cb), backlight);

	/* Don't do dimming on inactive console */
	backlight->priv->consolekit = egg_console_kit_new ();

	/* sync at startup */
	gpm_backlight_brightness_evaluate_and_set (backlight, FALSE);
}

/**
 * gpm_backlight_new:
 * Return value: A new brightness class instance.
 **/
GpmBacklight *
gpm_backlight_new (void)
{
	GpmBacklight *backlight = g_object_new (GPM_TYPE_BACKLIGHT, NULL);
	return backlight;
}

