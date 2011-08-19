/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007-2008 Richard Hughes <richard@hughsie.com>
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
#include <mateconf/mateconf-client.h>
#include <libupower-glib/upower.h>

#include "egg-debug.h"

#include "gpm-common.h"
#include "gpm-upower.h"
#include "gpm-marshal.h"
#include "gpm-engine.h"
#include "gpm-stock-icons.h"
#include "gpm-prefs-server.h"
#include "gpm-phone.h"

static void     gpm_engine_finalize   (GObject	  *object);

#define GPM_ENGINE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GPM_TYPE_ENGINE, GpmEnginePrivate))
#define GPM_ENGINE_RESUME_DELAY		2*1000
#define GPM_ENGINE_WARN_ACCURACY	20

struct GpmEnginePrivate
{
	MateConfClient		*conf;
	UpClient		*client;
	UpDevice		*battery_composite;
	GPtrArray		*array;
	GpmPhone		*phone;
	GpmIconPolicy		 icon_policy;
	gchar			*previous_icon;
	gchar			*previous_summary;

	gboolean		 use_time_primary;
	gboolean		 time_is_accurate;

	guint			 low_percentage;
	guint			 critical_percentage;
	guint			 action_percentage;
	guint			 low_time;
	guint			 critical_time;
	guint			 action_time;
};

enum {
	ICON_CHANGED,
	SUMMARY_CHANGED,
	FULLY_CHARGED,
	CHARGE_LOW,
	CHARGE_CRITICAL,
	CHARGE_ACTION,
	DISCHARGING,
	LOW_CAPACITY,
	PERHAPS_RECALL,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };
static gpointer gpm_engine_object = NULL;

G_DEFINE_TYPE (GpmEngine, gpm_engine, G_TYPE_OBJECT)

static UpDevice *gpm_engine_get_composite_device (GpmEngine *engine, UpDevice *original_device);
static UpDevice *gpm_engine_update_composite_device (GpmEngine *engine, UpDevice *original_device);

typedef enum {
	GPM_ENGINE_WARNING_NONE = 0,
	GPM_ENGINE_WARNING_DISCHARGING = 1,
	GPM_ENGINE_WARNING_LOW = 2,
	GPM_ENGINE_WARNING_CRITICAL = 3,
	GPM_ENGINE_WARNING_ACTION = 4
} GpmEngineWarning;

/**
 * gpm_engine_get_warning_csr:
 **/
static GpmEngineWarning
gpm_engine_get_warning_csr (GpmEngine *engine, UpDevice *device)
{
	gdouble percentage;

	/* get device properties */
	g_object_get (device, "percentage", &percentage, NULL);

	if (percentage < 26.0f)
		return GPM_ENGINE_WARNING_LOW;
	else if (percentage < 13.0f)
		return GPM_ENGINE_WARNING_CRITICAL;
	return GPM_ENGINE_WARNING_NONE;
}

/**
 * gpm_engine_get_warning_percentage:
 **/
static GpmEngineWarning
gpm_engine_get_warning_percentage (GpmEngine *engine, UpDevice *device)
{
	gdouble percentage;

	/* get device properties */
	g_object_get (device, "percentage", &percentage, NULL);

	if (percentage <= engine->priv->action_percentage)
		return GPM_ENGINE_WARNING_ACTION;
	if (percentage <= engine->priv->critical_percentage)
		return GPM_ENGINE_WARNING_CRITICAL;
	if (percentage <= engine->priv->low_percentage)
		return GPM_ENGINE_WARNING_LOW;
	return GPM_ENGINE_WARNING_NONE;
}

/**
 * gpm_engine_get_warning_time:
 **/
static GpmEngineWarning
gpm_engine_get_warning_time (GpmEngine *engine, UpDevice *device)
{
	UpDeviceKind kind;
	gint64 time_to_empty;

	/* get device properties */
	g_object_get (device,
		      "kind", &kind,
		      "time-to-empty", &time_to_empty,
		      NULL);

	/* this is probably an error condition */
	if (time_to_empty == 0) {
		egg_debug ("time zero, falling back to percentage for %s", up_device_kind_to_string (kind));
		return gpm_engine_get_warning_percentage (engine, device);
	}

	if (time_to_empty <= engine->priv->action_time)
		return GPM_ENGINE_WARNING_ACTION;
	if (time_to_empty <= engine->priv->critical_time)
		return GPM_ENGINE_WARNING_CRITICAL;
	if (time_to_empty <= engine->priv->low_time)
		return GPM_ENGINE_WARNING_LOW;
	return GPM_ENGINE_WARNING_NONE;
}

/**
 * gpm_engine_get_warning:
 *
 * This gets the possible engine state for the device according to the
 * policy, which could be per-percent, or per-time.
 *
 * Return value: A GpmEngine state, e.g. GPM_ENGINE_WARNING_DISCHARGING
 **/
static GpmEngineWarning
gpm_engine_get_warning (GpmEngine *engine, UpDevice *device)
{
	UpDeviceKind kind;
	UpDeviceState state;
	GpmEngineWarning warning_type;

	/* get device properties */
	g_object_get (device,
		      "kind", &kind,
		      "state", &state,
		      NULL);

	/* default to no engine */
	warning_type = GPM_ENGINE_WARNING_NONE;

	/* if the device in question is on ac, don't give a warning */
	if (state == UP_DEVICE_STATE_CHARGING)
		goto out;

	if (kind == UP_DEVICE_KIND_MOUSE ||
	    kind == UP_DEVICE_KIND_KEYBOARD) {

		warning_type = gpm_engine_get_warning_csr (engine, device);

	} else if (kind == UP_DEVICE_KIND_UPS ||
#if UP_CHECK_VERSION(0,9,5)
		   kind == UP_DEVICE_KIND_MEDIA_PLAYER ||
		   kind == UP_DEVICE_KIND_TABLET ||
		   kind == UP_DEVICE_KIND_COMPUTER ||
#endif
		   kind == UP_DEVICE_KIND_PDA) {

		warning_type = gpm_engine_get_warning_percentage (engine, device);

	} else if (kind == UP_DEVICE_KIND_PHONE) {

		warning_type = gpm_engine_get_warning_percentage (engine, device);

	} else if (kind == UP_DEVICE_KIND_BATTERY) {
		/* only use the time when it is accurate, and MateConf is not disabled */
		if (engine->priv->use_time_primary)
			warning_type = gpm_engine_get_warning_time (engine, device);
		else
			warning_type = gpm_engine_get_warning_percentage (engine, device);
	}

	/* If we have no important engines, we should test for discharging */
	if (warning_type == GPM_ENGINE_WARNING_NONE) {
		if (state == UP_DEVICE_STATE_DISCHARGING)
			warning_type = GPM_ENGINE_WARNING_DISCHARGING;
	}

 out:
	return warning_type;
}

/**
 * gpm_engine_get_summary:
 * @engine: This engine class instance
 * @string: The returned string
 *
 * Returns the complete tooltip ready for display
 **/
gchar *
gpm_engine_get_summary (GpmEngine *engine)
{
	guint i;
	GPtrArray *array;
	UpDevice *device;
	UpDeviceState state;
	GString *tooltip = NULL;
	gchar *part;
	gboolean is_present;

	g_return_val_if_fail (GPM_IS_ENGINE (engine), NULL);

	/* need to get AC state */
	tooltip = g_string_new ("");

	/* do we have specific device types? */
	array = engine->priv->array;
	for (i=0;i<array->len;i++) {
		device = g_ptr_array_index (engine->priv->array, i);
		g_object_get (device,
			      "is-present", &is_present,
			      "state", &state,
			      NULL);
		if (!is_present)
			continue;
		if (state == UP_DEVICE_STATE_EMPTY)
			continue;
		part = gpm_upower_get_device_summary (device);
		if (part != NULL)
			g_string_append_printf (tooltip, "%s\n", part);
		g_free (part);
	}

	/* remove the last \n */
	g_string_truncate (tooltip, tooltip->len-1);

	egg_debug ("tooltip: %s", tooltip->str);

	return g_string_free (tooltip, FALSE);
}

/**
 * gpm_engine_get_icon_priv:
 *
 * Returns the icon
 **/
static gchar *
gpm_engine_get_icon_priv (GpmEngine *engine, UpDeviceKind device_kind, GpmEngineWarning warning, gboolean use_state)
{
	guint i;
	GPtrArray *array;
	UpDevice *device;
	GpmEngineWarning warning_temp;
	UpDeviceKind kind;
	UpDeviceState state;
	gboolean is_present;

	/* do we have specific device types? */
	array = engine->priv->array;
	for (i=0;i<array->len;i++) {
		device = g_ptr_array_index (engine->priv->array, i);

		/* get device properties */
		g_object_get (device,
			      "kind", &kind,
			      "state", &state,
			      "is-present", &is_present,
			      NULL);

		/* if battery then use composite device to cope with multiple batteries */
		if (kind == UP_DEVICE_KIND_BATTERY)
			device = gpm_engine_get_composite_device (engine, device);

		warning_temp = GPOINTER_TO_INT(g_object_get_data (G_OBJECT(device), "engine-warning-old"));
		if (kind == device_kind && is_present) {
			if (warning != GPM_ENGINE_WARNING_NONE) {
				if (warning_temp == warning)
					return gpm_upower_get_device_icon (device);
				continue;
			}
			if (use_state) {
				if (state == UP_DEVICE_STATE_CHARGING || state == UP_DEVICE_STATE_DISCHARGING)
					return gpm_upower_get_device_icon (device);
				continue;
			}
			return gpm_upower_get_device_icon (device);
		}
	}
	return NULL;
}

/**
 * gpm_engine_get_icon:
 *
 * Returns the icon
 **/
gchar *
gpm_engine_get_icon (GpmEngine *engine)
{
	gchar *icon = NULL;

	g_return_val_if_fail (GPM_IS_ENGINE (engine), NULL);

	/* policy */
	if (engine->priv->icon_policy == GPM_ICON_POLICY_NEVER) {
		egg_debug ("no icon allowed, so no icon will be displayed.");
		return NULL;
	}

	/* we try CRITICAL: BATTERY, UPS, MOUSE, KEYBOARD */
	icon = gpm_engine_get_icon_priv (engine, UP_DEVICE_KIND_BATTERY, GPM_ENGINE_WARNING_CRITICAL, FALSE);
	if (icon != NULL)
		return icon;
	icon = gpm_engine_get_icon_priv (engine, UP_DEVICE_KIND_UPS, GPM_ENGINE_WARNING_CRITICAL, FALSE);
	if (icon != NULL)
		return icon;
	icon = gpm_engine_get_icon_priv (engine, UP_DEVICE_KIND_MOUSE, GPM_ENGINE_WARNING_CRITICAL, FALSE);
	if (icon != NULL)
		return icon;
	icon = gpm_engine_get_icon_priv (engine, UP_DEVICE_KIND_KEYBOARD, GPM_ENGINE_WARNING_CRITICAL, FALSE);
	if (icon != NULL)
		return icon;

	/* policy */
	if (engine->priv->icon_policy == GPM_ICON_POLICY_CRITICAL) {
		egg_debug ("no devices critical, so no icon will be displayed.");
		return NULL;
	}

	/* we try CRITICAL: BATTERY, UPS, MOUSE, KEYBOARD */
	icon = gpm_engine_get_icon_priv (engine, UP_DEVICE_KIND_BATTERY, GPM_ENGINE_WARNING_LOW, FALSE);
	if (icon != NULL)
		return icon;
	icon = gpm_engine_get_icon_priv (engine, UP_DEVICE_KIND_UPS, GPM_ENGINE_WARNING_LOW, FALSE);
	if (icon != NULL)
		return icon;
	icon = gpm_engine_get_icon_priv (engine, UP_DEVICE_KIND_MOUSE, GPM_ENGINE_WARNING_LOW, FALSE);
	if (icon != NULL)
		return icon;
	icon = gpm_engine_get_icon_priv (engine, UP_DEVICE_KIND_KEYBOARD, GPM_ENGINE_WARNING_LOW, FALSE);
	if (icon != NULL)
		return icon;

	/* policy */
	if (engine->priv->icon_policy == GPM_ICON_POLICY_LOW) {
		egg_debug ("no devices low, so no icon will be displayed.");
		return NULL;
	}

	/* we try (DIS)CHARGING: BATTERY, UPS */
	icon = gpm_engine_get_icon_priv (engine, UP_DEVICE_KIND_BATTERY, GPM_ENGINE_WARNING_NONE, TRUE);
	if (icon != NULL)
		return icon;
	icon = gpm_engine_get_icon_priv (engine, UP_DEVICE_KIND_UPS, GPM_ENGINE_WARNING_NONE, TRUE);
	if (icon != NULL)
		return icon;

	/* policy */
	if (engine->priv->icon_policy == GPM_ICON_POLICY_CHARGE) {
		egg_debug ("no devices (dis)charging, so no icon will be displayed.");
		return NULL;
	}

	/* we try PRESENT: BATTERY, UPS */
	icon = gpm_engine_get_icon_priv (engine, UP_DEVICE_KIND_BATTERY, GPM_ENGINE_WARNING_NONE, FALSE);
	if (icon != NULL)
		return icon;
	icon = gpm_engine_get_icon_priv (engine, UP_DEVICE_KIND_UPS, GPM_ENGINE_WARNING_NONE, FALSE);
	if (icon != NULL)
		return icon;

	/* policy */
	if (engine->priv->icon_policy == GPM_ICON_POLICY_PRESENT) {
		egg_debug ("no devices present, so no icon will be displayed.");
		return NULL;
	}

	/* we fallback to the ac_adapter icon */
	egg_debug ("Using fallback");
	return g_strdup (GPM_STOCK_AC_ADAPTER);
}

/**
 * gpm_engine_recalculate_state_icon:
 */
static gboolean
gpm_engine_recalculate_state_icon (GpmEngine *engine)
{
	gchar *icon;

	g_return_val_if_fail (engine != NULL, FALSE);
	g_return_val_if_fail (GPM_IS_ENGINE (engine), FALSE);

	/* show a different icon if we are disconnected */
	icon = gpm_engine_get_icon (engine);
	if (icon == NULL) {
		/* none before, now none */
		if (engine->priv->previous_icon == NULL)
			return FALSE;
		/* icon before, now none */
		egg_debug ("** EMIT: icon-changed: none");
		g_signal_emit (engine, signals [ICON_CHANGED], 0, NULL);

		g_free (engine->priv->previous_icon);
		engine->priv->previous_icon = NULL;
		return TRUE;
	}

	/* no icon before, now icon */
	if (engine->priv->previous_icon == NULL) {
		egg_debug ("** EMIT: icon-changed: %s", icon);
		g_signal_emit (engine, signals [ICON_CHANGED], 0, icon);
		engine->priv->previous_icon = icon;
		return TRUE;
	}

	/* icon before, now different */
	if (strcmp (engine->priv->previous_icon, icon) != 0) {
		g_free (engine->priv->previous_icon);
		engine->priv->previous_icon = icon;
		egg_debug ("** EMIT: icon-changed: %s", icon);
		g_signal_emit (engine, signals [ICON_CHANGED], 0, icon);
		return TRUE;
	}

	egg_debug ("no change");
	/* nothing to do */
	g_free (icon);
	return FALSE;
}

/**
 * gpm_engine_recalculate_state_summary:
 */
static gboolean
gpm_engine_recalculate_state_summary (GpmEngine *engine)
{
	gchar *summary;

	summary = gpm_engine_get_summary (engine);
	if (engine->priv->previous_summary == NULL) {
		engine->priv->previous_summary = summary;
		egg_debug ("** EMIT: summary-changed(1): %s", summary);
		g_signal_emit (engine, signals [SUMMARY_CHANGED], 0, summary);
		return TRUE;
	}	

	if (strcmp (engine->priv->previous_summary, summary) != 0) {
		g_free (engine->priv->previous_summary);
		engine->priv->previous_summary = summary;
		egg_debug ("** EMIT: summary-changed(2): %s", summary);
		g_signal_emit (engine, signals [SUMMARY_CHANGED], 0, summary);
		return TRUE;
	}
	egg_debug ("no change");
	/* nothing to do */
	g_free (summary);
	return FALSE;
}

/**
 * gpm_engine_recalculate_state:
 */
static void
gpm_engine_recalculate_state (GpmEngine *engine)
{

	g_return_if_fail (engine != NULL);
	g_return_if_fail (GPM_IS_ENGINE (engine));

	gpm_engine_recalculate_state_icon (engine);
	gpm_engine_recalculate_state_summary (engine);
}

/**
 * gpm_engine_conf_key_changed_cb:
 **/
static void
gpm_engine_conf_key_changed_cb (MateConfClient *conf, guint cnxn_id, MateConfEntry *entry, GpmEngine *engine)
{
	MateConfValue *value;
	gchar *icon_policy;

	if (entry == NULL)
		return;
	value = mateconf_entry_get_value (entry);
	if (value == NULL)
		return;

	if (strcmp (entry->key, GPM_CONF_USE_TIME_POLICY) == 0) {

		engine->priv->use_time_primary = mateconf_value_get_bool (value);

	} else if (strcmp (entry->key, GPM_CONF_UI_ICON_POLICY) == 0) {

		/* do we want to display the icon in the tray */
		icon_policy = mateconf_client_get_string (conf, GPM_CONF_UI_ICON_POLICY, NULL);
		engine->priv->icon_policy = gpm_icon_policy_from_string (icon_policy);
		g_free (icon_policy);

		/* perhaps change icon */
		gpm_engine_recalculate_state_icon (engine);
	}
}

/**
 * gpm_engine_device_check_capacity:
 **/
static gboolean
gpm_engine_device_check_capacity (GpmEngine *engine, UpDevice *device)
{
	gboolean ret;
	UpDeviceKind kind;
	gdouble capacity;

	/* get device properties */
	g_object_get (device,
		      "kind", &kind,
		      "capacity", &capacity,
		      NULL);

	/* not laptop battery */
	if (kind != UP_DEVICE_KIND_BATTERY)
		return FALSE;

	/* capacity okay */
	if (capacity > 50.0f)
		return FALSE;

	/* capacity invalid */
	if (capacity < 1.0f)
		return FALSE;

	/* only emit this if specified in mateconf */
	ret = mateconf_client_get_bool (engine->priv->conf, GPM_CONF_NOTIFY_LOW_CAPACITY, NULL);
	if (ret) {
		egg_debug ("** EMIT: low-capacity");
		g_signal_emit (engine, signals [LOW_CAPACITY], 0, device);
	}
	return TRUE;
}

/**
 * gpm_engine_get_composite_device:
 **/
static UpDevice *
gpm_engine_get_composite_device (GpmEngine *engine, UpDevice *original_device)
{
	guint battery_devices = 0;
	GPtrArray *array;
	UpDevice *device;
	UpDeviceKind kind;
	guint i;

	/* find out how many batteries in the system */
	array = engine->priv->array;
	for (i=0;i<array->len;i++) {
		device = g_ptr_array_index (engine->priv->array, i);
		g_object_get (device,
			      "kind", &kind,
			      NULL);
		if (kind == UP_DEVICE_KIND_BATTERY)
			battery_devices++;
	}

	/* just use the original device if only one primary battery */
	if (battery_devices <= 1) {
		egg_debug ("using original device as only one primary battery");
		device = original_device;
		goto out;
	}

	/* use the composite device */
	device = engine->priv->battery_composite;
out:
	/* return composite device or original device */
	return device;
}

/**
 * gpm_engine_update_composite_device:
 **/
static UpDevice *
gpm_engine_update_composite_device (GpmEngine *engine, UpDevice *original_device)
{
	guint i;
	gdouble percentage = 0.0;
	gdouble energy = 0.0;
	gdouble energy_full = 0.0;
	gdouble energy_rate = 0.0;
	gdouble energy_total = 0.0;
	gdouble energy_full_total = 0.0;
	gdouble energy_rate_total = 0.0;
	gint64 time_to_empty = 0;
	gint64 time_to_full = 0;
	guint battery_devices = 0;
	gboolean is_charging = FALSE;
	gboolean is_discharging = FALSE;
	gboolean is_fully_charged = TRUE;
	GPtrArray *array;
	UpDevice *device;
	UpDeviceState state;
	UpDeviceKind kind;
	gboolean debug;
	gchar *text;

	/* are we printing to console? */
	debug = egg_debug_enabled ();

	/* update the composite device */
	array = engine->priv->array;
	for (i=0;i<array->len;i++) {
		device = g_ptr_array_index (engine->priv->array, i);
		g_object_get (device,
			      "kind", &kind,
			      "state", &state,
			      "energy", &energy,
			      "energy-full", &energy_full,
			      "energy-rate", &energy_rate,
			      NULL);
		if (kind != UP_DEVICE_KIND_BATTERY)
			continue;

		if (debug) {
			text = up_device_to_text (device);
			egg_debug ("printing device %i:\n%s", i, text);
			g_free (text);
		}

		/* one of these will be charging or discharging */
		if (state == UP_DEVICE_STATE_CHARGING)
			is_charging = TRUE;
		if (state == UP_DEVICE_STATE_DISCHARGING)
			is_discharging = TRUE;
		if (state != UP_DEVICE_STATE_FULLY_CHARGED)
			is_fully_charged = FALSE;

		/* sum up composite */
		energy_total += energy;
		energy_full_total += energy_full;
		energy_rate_total += energy_rate;
		battery_devices++;
	}

	/* just use the original device if only one primary battery */
	if (battery_devices == 1) {
		egg_debug ("using original device as only one primary battery");
		device = original_device;
		goto out;
	}

	/* use percentage weighted for each battery capacity */
	percentage = 100.0 * energy_total / energy_full_total;

	/* set composite state */
	if (is_charging)
		state = UP_DEVICE_STATE_CHARGING;
	else if (is_discharging)
		state = UP_DEVICE_STATE_DISCHARGING;
	else if (is_fully_charged)
		state = UP_DEVICE_STATE_FULLY_CHARGED;
	else
		state = UP_DEVICE_STATE_UNKNOWN;

	/* calculate a quick and dirty time remaining value */
	if (energy_rate_total > 0) {
		if (state == UP_DEVICE_STATE_DISCHARGING)
			time_to_empty = 3600 * (energy_total / energy_rate_total);
		else if (state == UP_DEVICE_STATE_CHARGING)
			time_to_full = 3600 * ((energy_full_total - energy_total) / energy_rate_total);
	}

	/* okay, we can use the composite device */
	device = engine->priv->battery_composite;

	egg_debug ("printing composite device");
	g_object_set (device,
		      "energy", energy,
		      "energy-full", energy_full,
		      "energy-rate", energy_rate,
		      "time-to-empty", time_to_empty,
		      "time-to-full", time_to_full,
		      "percentage", percentage,
		      "state", state,
		      NULL);
	if (debug) {
		text = up_device_to_text (device);
		egg_debug ("composite:\n%s", text);
		g_free (text);
	}

	/* force update of icon */
	gpm_engine_recalculate_state_icon (engine);
out:
	/* return composite device or original device */
	return device;
}

/**
 * gpm_engine_device_add:
 **/
static void
gpm_engine_device_add (GpmEngine *engine, UpDevice *device)
{
	GpmEngineWarning warning;
	UpDeviceState state;
	UpDeviceKind kind;
	UpDevice *composite;

	/* assign warning */
	warning = gpm_engine_get_warning (engine, device);
	g_object_set_data (G_OBJECT(device), "engine-warning-old", GUINT_TO_POINTER(warning));

	/* check capacity */
	gpm_engine_device_check_capacity (engine, device);

	/* get device properties */
	g_object_get (device,
		      "kind", &kind,
		      "state", &state,
		      NULL);

	/* add old state for transitions */
	egg_debug ("adding %s with state %s", up_device_get_object_path (device), up_device_state_to_string (state));
	g_object_set_data (G_OBJECT(device), "engine-state-old", GUINT_TO_POINTER(state));

	if (kind == UP_DEVICE_KIND_BATTERY) {
		egg_debug ("updating because we added a device");
		composite = gpm_engine_update_composite_device (engine, device);

		/* get the same values for the composite device */
		warning = gpm_engine_get_warning (engine, composite);
		g_object_set_data (G_OBJECT(composite), "engine-warning-old", GUINT_TO_POINTER(warning));
		g_object_get (composite, "state", &state, NULL);
		g_object_set_data (G_OBJECT(composite), "engine-state-old", GUINT_TO_POINTER(state));
	}
}

/**
 * gpm_engine_check_recall:
 **/
static gboolean
gpm_engine_check_recall (GpmEngine *engine, UpDevice *device)
{
	UpDeviceKind kind;
	gboolean recall_notice = FALSE;
	gchar *recall_vendor = NULL;
	gchar *recall_url = NULL;

	/* get device properties */
	g_object_get (device,
		      "kind", &kind,
		      "recall-notice", &recall_notice,
		      "recall-vendor", &recall_vendor,
		      "recall-url", &recall_url,
		      NULL);

	/* not battery */
	if (kind != UP_DEVICE_KIND_BATTERY)
		goto out;

	/* no recall data */
	if (!recall_notice)
		goto out;

	/* emit signal for manager */
	egg_debug ("** EMIT: perhaps-recall");
	g_signal_emit (engine, signals [PERHAPS_RECALL], 0, device, recall_vendor, recall_url);
out:
	g_free (recall_vendor);
	g_free (recall_url);
	return recall_notice;
}

/**
 * gpm_engine_coldplug_idle_cb:
 **/
static gboolean
gpm_engine_coldplug_idle_cb (GpmEngine *engine)
{
	guint i;
	GPtrArray *array;
	gboolean has_battery = FALSE;
	gboolean has_ups = FALSE;
	GpmPrefsServer *prefs_server;
	UpDevice *device;
	UpDeviceKind kind;
	gboolean ret;
	GError *error = NULL;

	g_return_val_if_fail (engine != NULL, FALSE);
	g_return_val_if_fail (GPM_IS_ENGINE (engine), FALSE);

	/* get devices from UPower */
	ret = up_client_enumerate_devices_sync (engine->priv->client, NULL, &error);
	if (!ret) {
		egg_error ("failed to get device list: %s", error->message);
		g_error_free (error);
		goto out;
	}
	engine->priv->array = up_client_get_devices (engine->priv->client);

	/* do we have specific device types? */
	array = engine->priv->array;
	for (i=0;i<array->len;i++) {
		device = g_ptr_array_index (engine->priv->array, i);

		/* get device properties */
		g_object_get (device,
			      "kind", &kind,
			      NULL);

		if (kind == UP_DEVICE_KIND_BATTERY)
			has_battery = TRUE;
		else if (kind == UP_DEVICE_KIND_UPS)
			has_ups = TRUE;
	}

	/* only show the battery prefs section if we have batteries */
	prefs_server = gpm_prefs_server_new ();
	if (has_battery)
		gpm_prefs_server_set_capability (prefs_server, GPM_PREFS_SERVER_BATTERY);
	if (has_ups)
		gpm_prefs_server_set_capability (prefs_server, GPM_PREFS_SERVER_UPS);
	g_object_unref (prefs_server);

	/* connected mobile phones */
	gpm_phone_coldplug (engine->priv->phone);

	gpm_engine_recalculate_state (engine);

	/* add to database */
	for (i=0;i<array->len;i++) {
		device = g_ptr_array_index (engine->priv->array, i);
		gpm_engine_device_add (engine, device);
		gpm_engine_check_recall (engine, device);
	}
out:
	/* never repeat */
	return FALSE;
}

/**
 * gpm_engine_device_added_cb:
 **/
static void
gpm_engine_device_added_cb (UpClient *client, UpDevice *device, GpmEngine *engine)
{
	/* add to list */
	g_ptr_array_add (engine->priv->array, g_object_ref (device));
	gpm_engine_check_recall (engine, device);

	gpm_engine_recalculate_state (engine);
}

/**
 * gpm_engine_device_removed_cb:
 **/
static void
gpm_engine_device_removed_cb (UpClient *client, UpDevice *device, GpmEngine *engine)
{
	gboolean ret;
	ret = g_ptr_array_remove (engine->priv->array, device);
	if (!ret)
		return;
	gpm_engine_recalculate_state (engine);
}


/**
 * gpm_engine_device_changed_cb:
 **/
static void
gpm_engine_device_changed_cb (UpClient *client, UpDevice *device, GpmEngine *engine)
{
	UpDeviceKind kind;
	UpDeviceState state;
	UpDeviceState state_old;
	GpmEngineWarning warning_old;
	GpmEngineWarning warning;

	/* get device properties */
	g_object_get (device,
		      "kind", &kind,
		      NULL);

	/* if battery then use composite device to cope with multiple batteries */
	if (kind == UP_DEVICE_KIND_BATTERY) {
		egg_debug ("updating because %s changed", up_device_get_object_path (device));
		device = gpm_engine_update_composite_device (engine, device);
	}

	/* get device properties (may be composite) */
	g_object_get (device,
		      "state", &state,
		      NULL);

	egg_debug ("%s state is now %s", up_device_get_object_path (device), up_device_state_to_string (state));

	/* see if any interesting state changes have happened */
	state_old = GPOINTER_TO_INT(g_object_get_data (G_OBJECT(device), "engine-state-old"));
	if (state_old != state) {
		if (state == UP_DEVICE_STATE_DISCHARGING) {
			egg_debug ("** EMIT: discharging");
			g_signal_emit (engine, signals [DISCHARGING], 0, device);
		} else if (state == UP_DEVICE_STATE_FULLY_CHARGED) {
			egg_debug ("** EMIT: fully charged");
			g_signal_emit (engine, signals [FULLY_CHARGED], 0, device);
		}

		/* save new state */
		g_object_set_data (G_OBJECT(device), "engine-state-old", GUINT_TO_POINTER(state));
	}

	/* check the warning state has not changed */
	warning_old = GPOINTER_TO_INT(g_object_get_data (G_OBJECT(device), "engine-warning-old"));
	warning = gpm_engine_get_warning (engine, device);
	if (warning != warning_old) {
		if (warning == GPM_ENGINE_WARNING_LOW) {
			egg_debug ("** EMIT: charge-low");
			g_signal_emit (engine, signals [CHARGE_LOW], 0, device);
		} else if (warning == GPM_ENGINE_WARNING_CRITICAL) {
			egg_debug ("** EMIT: charge-critical");
			g_signal_emit (engine, signals [CHARGE_CRITICAL], 0, device);
		} else if (warning == GPM_ENGINE_WARNING_ACTION) {
			egg_debug ("** EMIT: charge-action");
			g_signal_emit (engine, signals [CHARGE_ACTION], 0, device);
		}
		/* save new state */
		g_object_set_data (G_OBJECT(device), "engine-warning-old", GUINT_TO_POINTER(warning));
	}

	gpm_engine_recalculate_state (engine);
}

/**
 * gpm_engine_get_devices:
 *
 * Return value: the UpDevice array, free with g_ptr_array_unref()
 **/
GPtrArray *
gpm_engine_get_devices (GpmEngine *engine)
{
	return g_ptr_array_ref (engine->priv->array);
}

/**
 * phone_device_added_cb:
 **/
static void
phone_device_added_cb (GpmPhone *phone, guint idx, GpmEngine *engine)
{
	UpDevice *device;
	device = up_device_new ();

	egg_debug ("phone added %i", idx);

	/* get device properties */
	g_object_set (device,
		      "kind", UP_DEVICE_KIND_PHONE,
		      "is-rechargeable", TRUE,
		      "native-path", g_strdup_printf ("dummy:phone_%i", idx),
		      "is-present", TRUE,
		      NULL);

	/* state changed */
	gpm_engine_device_add (engine, device);
	g_ptr_array_add (engine->priv->array, g_object_ref (device));
	gpm_engine_recalculate_state (engine);
}

/**
 * phone_device_removed_cb:
 **/
static void
phone_device_removed_cb (GpmPhone *phone, guint idx, GpmEngine *engine)
{
	guint i;
	UpDevice *device;
	UpDeviceKind kind;

	egg_debug ("phone removed %i", idx);

	for (i=0; i<engine->priv->array->len; i++) {
		device = g_ptr_array_index (engine->priv->array, i);

		/* get device properties */
		g_object_get (device,
			      "kind", &kind,
			      NULL);

		if (kind == UP_DEVICE_KIND_PHONE) {
			g_ptr_array_remove_index (engine->priv->array, i);
			break;
		}
	}

	/* state changed */
	gpm_engine_recalculate_state (engine);
}

/**
 * phone_device_refresh_cb:
 **/
static void
phone_device_refresh_cb (GpmPhone *phone, guint idx, GpmEngine *engine)
{
	guint i;
	UpDevice *device;
	UpDeviceKind kind;
	UpDeviceState state;
	gboolean is_present;
	gdouble percentage;

	egg_debug ("phone refresh %i", idx);

	for (i=0; i<engine->priv->array->len; i++) {
		device = g_ptr_array_index (engine->priv->array, i);

		/* get device properties */
		g_object_get (device,
			      "kind", &kind,
			      "state", &state,
			      "percentage", &percentage,
			      "is-present", &is_present,
			      NULL);

		if (kind == UP_DEVICE_KIND_PHONE) {
			is_present = gpm_phone_get_present (phone, idx);
			state = gpm_phone_get_on_ac (phone, idx) ? UP_DEVICE_STATE_CHARGING : UP_DEVICE_STATE_DISCHARGING;
			percentage = gpm_phone_get_percentage (phone, idx);
			break;
		}
	}

	/* state changed */
	gpm_engine_recalculate_state (engine);
}

/**
 * gpm_engine_init:
 * @engine: This class instance
 **/
static void
gpm_engine_init (GpmEngine *engine)
{
	gchar *icon_policy;

	engine->priv = GPM_ENGINE_GET_PRIVATE (engine);

	engine->priv->array = g_ptr_array_new_with_free_func (g_object_unref);
	engine->priv->client = up_client_new ();
	g_signal_connect (engine->priv->client, "device-added",
			  G_CALLBACK (gpm_engine_device_added_cb), engine);
	g_signal_connect (engine->priv->client, "device-removed",
			  G_CALLBACK (gpm_engine_device_removed_cb), engine);
	g_signal_connect (engine->priv->client, "device-changed",
			  G_CALLBACK (gpm_engine_device_changed_cb), engine);

	engine->priv->conf = mateconf_client_get_default ();
	mateconf_client_notify_add (engine->priv->conf, GPM_CONF_DIR,
				 (MateConfClientNotifyFunc) gpm_engine_conf_key_changed_cb,
				 engine, NULL, NULL);

	engine->priv->phone = gpm_phone_new ();
	g_signal_connect (engine->priv->phone, "device-added",
			  G_CALLBACK (phone_device_added_cb), engine);
	g_signal_connect (engine->priv->phone, "device-removed",
			  G_CALLBACK (phone_device_removed_cb), engine);
	g_signal_connect (engine->priv->phone, "device-refresh",
			  G_CALLBACK (phone_device_refresh_cb), engine);

	/* create a fake virtual composite battery */
	engine->priv->battery_composite = up_device_new ();
	g_object_set (engine->priv->battery_composite,
		      "kind", UP_DEVICE_KIND_BATTERY,
		      "is-rechargeable", TRUE,
		      "native-path", "dummy:composite_battery",
		      "power-supply", TRUE,
		      "is-present", TRUE,
		      NULL);

	engine->priv->previous_icon = NULL;
	engine->priv->previous_summary = NULL;

	/* do we want to display the icon in the tray */
	icon_policy = mateconf_client_get_string (engine->priv->conf, GPM_CONF_UI_ICON_POLICY, NULL);
	engine->priv->icon_policy = gpm_icon_policy_from_string (icon_policy);
	g_free (icon_policy);

	/* get percentage policy */
	engine->priv->low_percentage = mateconf_client_get_int (engine->priv->conf, GPM_CONF_THRESH_PERCENTAGE_LOW, NULL);
	engine->priv->critical_percentage = mateconf_client_get_int (engine->priv->conf, GPM_CONF_THRESH_PERCENTAGE_CRITICAL, NULL);
	engine->priv->action_percentage = mateconf_client_get_int (engine->priv->conf, GPM_CONF_THRESH_PERCENTAGE_ACTION, NULL);

	/* get time policy */
	engine->priv->low_time = mateconf_client_get_int (engine->priv->conf, GPM_CONF_THRESH_TIME_LOW, NULL);
	engine->priv->critical_time = mateconf_client_get_int (engine->priv->conf, GPM_CONF_THRESH_TIME_CRITICAL, NULL);
	engine->priv->action_time = mateconf_client_get_int (engine->priv->conf, GPM_CONF_THRESH_TIME_ACTION, NULL);

	/* we can disable this if the time remaining is inaccurate or just plain wrong */
	engine->priv->use_time_primary = mateconf_client_get_bool (engine->priv->conf, GPM_CONF_USE_TIME_POLICY, NULL);
	if (engine->priv->use_time_primary)
		egg_debug ("Using per-time notification policy");
	else
		egg_debug ("Using percentage notification policy");

	g_idle_add ((GSourceFunc) gpm_engine_coldplug_idle_cb, engine);
}

/**
 * gpm_engine_class_init:
 * @engine: This class instance
 **/
static void
gpm_engine_class_init (GpmEngineClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gpm_engine_finalize;
	g_type_class_add_private (klass, sizeof (GpmEnginePrivate));

	signals [ICON_CHANGED] =
		g_signal_new ("icon-changed",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GpmEngineClass, icon_changed),
			      NULL, NULL, g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE, 1, G_TYPE_STRING);
	signals [SUMMARY_CHANGED] =
		g_signal_new ("summary-changed",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GpmEngineClass, summary_changed),
			      NULL, NULL, g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE, 1, G_TYPE_STRING);
	signals [LOW_CAPACITY] =
		g_signal_new ("low-capacity",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GpmEngineClass, low_capacity),
			      NULL, NULL, g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE, 1, G_TYPE_POINTER);
	signals [PERHAPS_RECALL] =
		g_signal_new ("perhaps-recall",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GpmEngineClass, perhaps_recall),
			      NULL, NULL, gpm_marshal_VOID__POINTER_STRING_STRING,
			      G_TYPE_NONE,
			      3, G_TYPE_POINTER, G_TYPE_STRING, G_TYPE_STRING);
	signals [FULLY_CHARGED] =
		g_signal_new ("fully-charged",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GpmEngineClass, fully_charged),
			      NULL, NULL, g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE, 1, G_TYPE_POINTER);
	signals [DISCHARGING] =
		g_signal_new ("discharging",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GpmEngineClass, discharging),
			      NULL, NULL, g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE, 1, G_TYPE_POINTER);
	signals [CHARGE_ACTION] =
		g_signal_new ("charge-action",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GpmEngineClass, charge_action),
			      NULL, NULL, g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE, 1, G_TYPE_POINTER);
	signals [CHARGE_LOW] =
		g_signal_new ("charge-low",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GpmEngineClass, charge_low),
			      NULL, NULL, g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE, 1, G_TYPE_POINTER);
	signals [CHARGE_CRITICAL] =
		g_signal_new ("charge-critical",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GpmEngineClass, charge_critical),
			      NULL, NULL, g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE, 1, G_TYPE_POINTER);
}

/**
 * gpm_engine_finalize:
 * @object: This class instance
 **/
static void
gpm_engine_finalize (GObject *object)
{
	GpmEngine *engine;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GPM_IS_ENGINE (object));

	engine = GPM_ENGINE (object);
	engine->priv = GPM_ENGINE_GET_PRIVATE (engine);

	g_ptr_array_unref (engine->priv->array);
	g_object_unref (engine->priv->client);
	g_object_unref (engine->priv->phone);
	g_object_unref (engine->priv->battery_composite);

	g_free (engine->priv->previous_icon);
	g_free (engine->priv->previous_summary);

	G_OBJECT_CLASS (gpm_engine_parent_class)->finalize (object);
}

/**
 * gpm_engine_new:
 * Return value: new class instance.
 **/
GpmEngine *
gpm_engine_new (void)
{
	if (gpm_engine_object != NULL) {
		g_object_ref (gpm_engine_object);
	} else {
		gpm_engine_object = g_object_new (GPM_TYPE_ENGINE, NULL);
		g_object_add_weak_pointer (gpm_engine_object, &gpm_engine_object);
	}
	return GPM_ENGINE (gpm_engine_object);

}

