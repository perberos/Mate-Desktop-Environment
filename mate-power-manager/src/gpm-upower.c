/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Richard Hughes <richard@hughsie.com>
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

#include <glib/gi18n.h>
#include <libupower-glib/upower.h>

#include "egg-debug.h"
#include "egg-precision.h"

#include "gpm-upower.h"
#include "gpm-common.h"

#define GPM_UP_TIME_PRECISION			5*60
#define GPM_UP_TEXT_MIN_TIME			120

/**
 * gpm_upower_get_device_icon_index:
 * @percent: The charge of the device
 *
 * The index value depends on the percentage charge:
 *	00-10  = 000
 *	10-30  = 020
 *	30-50  = 040
 *	50-70  = 060
 *	70-90  = 080
 *	90-100 = 100
 *
 * Return value: The character string for the filename suffix.
 **/
static const gchar *
gpm_upower_get_device_icon_index (UpDevice *device)
{
	gdouble percentage;
	/* get device properties */
	g_object_get (device, "percentage", &percentage, NULL);
	if (percentage < 10)
		return "000";
	else if (percentage < 30)
		return "020";
	else if (percentage < 50)
		return "040";
	else if (percentage < 70)
		return "060";
	else if (percentage < 90)
		return "080";
	return "100";
}

/**
 * gpm_upower_get_device_icon:
 *
 * Need to free the return value
 *
 **/
gchar *
gpm_upower_get_device_icon (UpDevice *device)
{
	gchar *filename = NULL;
	const gchar *prefix = NULL;
	const gchar *index_str;
	UpDeviceKind kind;
	UpDeviceState state;
	gboolean is_present;
	gdouble percentage;

	g_return_val_if_fail (device != NULL, NULL);

	/* get device properties */
	g_object_get (device,
		      "kind", &kind,
		      "state", &state,
		      "percentage", &percentage,
		      "is-present", &is_present,
		      NULL);

	/* get correct icon prefix */
	prefix = up_device_kind_to_string (kind);

	/* get the icon from some simple rules */
	if (kind == UP_DEVICE_KIND_LINE_POWER) {
		filename = g_strdup ("gpm-ac-adapter");
	} else if (kind == UP_DEVICE_KIND_MONITOR) {
		filename = g_strdup ("gpm-monitor");
	} else if (kind == UP_DEVICE_KIND_UPS) {
		if (!is_present) {
			/* battery missing */
			filename = g_strdup_printf ("gpm-%s-missing", prefix);

		} else if (state == UP_DEVICE_STATE_FULLY_CHARGED) {
			filename = g_strdup_printf ("gpm-%s-100", prefix);

		} else if (state == UP_DEVICE_STATE_CHARGING) {
			index_str = gpm_upower_get_device_icon_index (device);
			filename = g_strdup_printf ("gpm-%s-%s-charging", prefix, index_str);

		} else if (state == UP_DEVICE_STATE_DISCHARGING) {
			index_str = gpm_upower_get_device_icon_index (device);
			filename = g_strdup_printf ("gpm-%s-%s", prefix, index_str);
		}
	} else if (kind == UP_DEVICE_KIND_BATTERY) {
		if (!is_present) {
			/* battery missing */
			filename = g_strdup_printf ("gpm-%s-missing", prefix);

		} else if (state == UP_DEVICE_STATE_EMPTY) {
			filename = g_strdup_printf ("gpm-%s-empty", prefix);

		} else if (state == UP_DEVICE_STATE_FULLY_CHARGED) {
			filename = g_strdup_printf ("gpm-%s-charged", prefix);

		} else if (state == UP_DEVICE_STATE_CHARGING) {
			index_str = gpm_upower_get_device_icon_index (device);
			filename = g_strdup_printf ("gpm-%s-%s-charging", prefix, index_str);

		} else if (state == UP_DEVICE_STATE_DISCHARGING) {
			index_str = gpm_upower_get_device_icon_index (device);
			filename = g_strdup_printf ("gpm-%s-%s", prefix, index_str);

		} else if (state == UP_DEVICE_STATE_PENDING_CHARGE) {
			index_str = gpm_upower_get_device_icon_index (device);
			/* FIXME: do new grey icons */
			filename = g_strdup_printf ("gpm-%s-%s-charging", prefix, index_str);

		} else if (state == UP_DEVICE_STATE_PENDING_DISCHARGE) {
			index_str = gpm_upower_get_device_icon_index (device);
			filename = g_strdup_printf ("gpm-%s-%s", prefix, index_str);
		} else {
			filename = g_strdup ("gpm-battery-missing");
		}

	} else if (kind == UP_DEVICE_KIND_MOUSE ||
		   kind == UP_DEVICE_KIND_KEYBOARD ||
		   kind == UP_DEVICE_KIND_PHONE) {
		if (!is_present) {
			/* battery missing */
			filename = g_strdup_printf ("gpm-%s-000", prefix);

		} else if (state == UP_DEVICE_STATE_FULLY_CHARGED) {
			filename = g_strdup_printf ("gpm-%s-100", prefix);

		} else if (state == UP_DEVICE_STATE_DISCHARGING) {
			index_str = gpm_upower_get_device_icon_index (device);
			filename = g_strdup_printf ("gpm-%s-%s", prefix, index_str);
		}
	}

	/* nothing matched */
	if (filename == NULL) {
		egg_warning ("nothing matched, falling back to default icon");
		filename = g_strdup ("dialog-warning");
	}

	egg_debug ("got filename: %s", filename);
	return filename;
}

/**
 * gpm_upower_get_device_summary:
 **/
gchar *
gpm_upower_get_device_summary (UpDevice *device)
{
	const gchar *kind_desc = NULL;
	gchar *description = NULL;
	guint time_to_full_round;
	guint time_to_empty_round;
	gchar *time_to_full_str;
	gchar *time_to_empty_str;
	UpDeviceKind kind;
	UpDeviceState state;
	gdouble percentage;
	gboolean is_present;
	gint64 time_to_full;
	gint64 time_to_empty;

	/* get device properties */
	g_object_get (device,
		      "kind", &kind,
		      "state", &state,
		      "percentage", &percentage,
		      "is-present", &is_present,
		      "time-to-full", &time_to_full,
		      "time-to-empty", &time_to_empty,
		      NULL);

	if (!is_present)
		return NULL;

	kind_desc = gpm_device_kind_to_localised_text (kind, 1);

	/* don't display all the extra stuff for keyboards and mice */
	if (kind == UP_DEVICE_KIND_MOUSE ||
	    kind == UP_DEVICE_KIND_KEYBOARD ||
	    kind == UP_DEVICE_KIND_PDA)
		return g_strdup_printf ("%s (%.1f%%)", kind_desc, percentage);

	/* we care if we are on AC */
	if (kind == UP_DEVICE_KIND_PHONE) {
		if (state == UP_DEVICE_STATE_CHARGING || !state == UP_DEVICE_STATE_DISCHARGING) {
			/* TRANSLATORS: a phone is charging */
			return g_strdup_printf (_("%s charging (%.1f%%)"), kind_desc, percentage);
		}
		return g_strdup_printf ("%s (%.1f%%)", kind_desc, percentage);
	}

	/* precalculate so we don't get Unknown time remaining */
	time_to_full_round = egg_precision_round_down (time_to_full, GPM_UP_TIME_PRECISION);
	time_to_empty_round = egg_precision_round_down (time_to_empty, GPM_UP_TIME_PRECISION);

	/* we always display "Laptop battery 16 minutes remaining" as we need to clarify what device we are refering to */
	if (state == UP_DEVICE_STATE_FULLY_CHARGED) {

		if (kind == UP_DEVICE_KIND_BATTERY && time_to_empty_round > GPM_UP_TEXT_MIN_TIME) {
			time_to_empty_str = gpm_get_timestring (time_to_empty_round);
			/* TRANSLATORS: The laptop battery is fully charged, and we know a time */
			description = g_strdup_printf (_("Battery is fully charged.\nProvides %s laptop runtime"),
							time_to_empty_str);
			g_free (time_to_empty_str);
		} else {
			/* TRANSLATORS: the device is fully charged */
			description = g_strdup_printf (_("%s is fully charged"), kind_desc);
		}

	} else if (state == UP_DEVICE_STATE_DISCHARGING) {

		if (time_to_empty_round > GPM_UP_TEXT_MIN_TIME) {
			time_to_empty_str = gpm_get_timestring (time_to_empty_round);
			/* TRANSLATORS: the device is discharging, and we have a time remaining */
			description = g_strdup_printf (_("%s %s remaining (%.1f%%)"),
							kind_desc, time_to_empty_str, percentage);
			g_free (time_to_empty_str);
		} else {
			/* TRANSLATORS: the device is discharging, but we only have a percentage */
			description = g_strdup_printf (_("%s discharging (%.1f%%)"),
							kind_desc, percentage);
		}

	} else if (state == UP_DEVICE_STATE_CHARGING) {

		if (time_to_full_round > GPM_UP_TEXT_MIN_TIME &&
		    time_to_empty_round > GPM_UP_TEXT_MIN_TIME) {

			/* display both discharge and charge time */
			time_to_full_str = gpm_get_timestring (time_to_full_round);
			time_to_empty_str = gpm_get_timestring (time_to_empty_round);

			/* TRANSLATORS: the device is charging, and we have a time to full and empty */
			description = g_strdup_printf (_("%s %s until charged (%.1f%%)\nProvides %s battery runtime"),
							kind_desc, time_to_full_str, percentage, time_to_empty_str);
			g_free (time_to_full_str);
			g_free (time_to_empty_str);

		} else if (time_to_full_round > GPM_UP_TEXT_MIN_TIME) {

			/* display only charge time */
			time_to_full_str = gpm_get_timestring (time_to_full_round);

			/* TRANSLATORS: device is charging, and we have a time to full and a percentage */
			description = g_strdup_printf (_("%s %s until charged (%.1f%%)"),
						kind_desc, time_to_full_str, percentage);
			g_free (time_to_full_str);
		} else {

			/* TRANSLATORS: device is charging, but we only have a percentage */
			description = g_strdup_printf (_("%s charging (%.1f%%)"),
						kind_desc, percentage);
		}

	} else if (state == UP_DEVICE_STATE_PENDING_DISCHARGE) {

		/* TRANSLATORS: this is only shown for laptops with multiple batteries */
		description = g_strdup_printf (_("%s waiting to discharge (%.1f%%)"),
						kind_desc, percentage);

	} else if (state == UP_DEVICE_STATE_PENDING_CHARGE) {

		/* TRANSLATORS: this is only shown for laptops with multiple batteries */
		description = g_strdup_printf (_("%s waiting to charge (%.1f%%)"), kind_desc, percentage);

	} else {
		egg_warning ("in an undefined state we are not charging or "
			     "discharging and the batteries are also not charged");
		description = g_strdup_printf ("%s (%.1f%%)", kind_desc, percentage);
	}
	return description;
}

/**
 * gpm_upower_get_device_description:
 **/
gchar *
gpm_upower_get_device_description (UpDevice *device)
{
	GString	*details;
	const gchar *text;
	gchar *time_str;
	UpDeviceKind kind;
	UpDeviceState state;
	UpDeviceTechnology technology;
	gdouble percentage;
	gdouble capacity;
	gdouble energy;
	gdouble energy_full;
	gdouble energy_full_design;
	gdouble energy_rate;
	gboolean is_present;
	gint64 time_to_full;
	gint64 time_to_empty;
	gchar *vendor = NULL;
	gchar *serial = NULL;
	gchar *model = NULL;

	g_return_val_if_fail (device != NULL, NULL);

	/* get device properties */
	g_object_get (device,
		      "kind", &kind,
		      "state", &state,
		      "percentage", &percentage,
		      "is-present", &is_present,
		      "time-to-full", &time_to_full,
		      "time-to-empty", &time_to_empty,
		      "technology", &technology,
		      "capacity", &capacity,
		      "energy", &energy,
		      "energy-full", &energy_full,
		      "energy-full-design", &energy_full_design,
		      "energy-rate", &energy_rate,
		      "vendor", &vendor,
		      "serial", &serial,
		      "model", &model,
		      NULL);

	details = g_string_new ("");
	text = gpm_device_kind_to_localised_text (kind, 1);
	/* TRANSLATORS: the type of data, e.g. Laptop battery */
	g_string_append_printf (details, "<b>%s</b> %s\n", _("Product:"), text);

	if (!is_present) {
		/* TRANSLATORS: device is missing */
		g_string_append_printf (details, "<b>%s</b> %s\n", _("Status:"), _("Missing"));
	} else if (state == UP_DEVICE_STATE_FULLY_CHARGED) {
		/* TRANSLATORS: device is charged */
		g_string_append_printf (details, "<b>%s</b> %s\n", _("Status:"), _("Charged"));
	} else if (state == UP_DEVICE_STATE_CHARGING) {
		/* TRANSLATORS: device is charging */
		g_string_append_printf (details, "<b>%s</b> %s\n", _("Status:"), _("Charging"));
	} else if (state == UP_DEVICE_STATE_DISCHARGING) {
		/* TRANSLATORS: device is discharging */
		g_string_append_printf (details, "<b>%s</b> %s\n", _("Status:"), _("Discharging"));
	}

	if (percentage >= 0) {
		/* TRANSLATORS: percentage */
		g_string_append_printf (details, "<b>%s</b> %.1f%%\n", _("Percentage charge:"), percentage);
	}
	if (vendor) {
		/* TRANSLATORS: manufacturer */
		g_string_append_printf (details, "<b>%s</b> %s\n", _("Vendor:"), vendor);
	}
	if (technology != UP_DEVICE_TECHNOLOGY_UNKNOWN) {
		text = gpm_device_technology_to_localised_string (technology);
		/* TRANSLATORS: how the battery is made, e.g. Lithium Ion */
		g_string_append_printf (details, "<b>%s</b> %s\n", _("Technology:"), text);
	}
	if (serial) {
		/* TRANSLATORS: serial number of the battery */
		g_string_append_printf (details, "<b>%s</b> %s\n", _("Serial number:"), serial);
	}
	if (model) {
		/* TRANSLATORS: model number of the battery */
		g_string_append_printf (details, "<b>%s</b> %s\n", _("Model:"), model);
	}
	if (time_to_full > 0) {
		time_str = gpm_get_timestring (time_to_full);
		/* TRANSLATORS: time to fully charged */
		g_string_append_printf (details, "<b>%s</b> %s\n", _("Charge time:"), time_str);
		g_free (time_str);
	}
	if (time_to_empty > 0) {
		time_str = gpm_get_timestring (time_to_empty);
		/* TRANSLATORS: time to empty */
		g_string_append_printf (details, "<b>%s</b> %s\n", _("Discharge time:"), time_str);
		g_free (time_str);
	}
	if (capacity > 0) {
		const gchar *condition;
		if (capacity > 99) {
			/* TRANSLATORS: Excellent, Good, Fair and Poor are all related to battery Capacity */
			condition = _("Excellent");
		} else if (capacity > 90) {
			condition = _("Good");
		} else if (capacity > 70) {
			condition = _("Fair");
		} else {
			condition = _("Poor");
		}
		/* TRANSLATORS: %.1f is a percentage and %s the condition (Excellent, Good, ...) */
		g_string_append_printf (details, "<b>%s</b> %.1f%% (%s)\n",
					_("Capacity:"), capacity, condition);
	}
	if (kind == UP_DEVICE_KIND_BATTERY) {
		if (energy > 0) {
			/* TRANSLATORS: current charge */
			g_string_append_printf (details, "<b>%s</b> %.1f Wh\n",
						_("Current charge:"), energy);
		}
		if (energy_full > 0 &&
		    energy_full_design != energy_full) {
			/* TRANSLATORS: last full is the charge the battery was seen to charge to */
			g_string_append_printf (details, "<b>%s</b> %.1f Wh\n",
						_("Last full charge:"), energy_full);
		}
		if (energy_full_design > 0) {
			/* Translators:  */
			/* TRANSLATORS: Design charge is the amount of charge the battery is designed to have when brand new */
			g_string_append_printf (details, "<b>%s</b> %.1f Wh\n",
						_("Design charge:"), energy_full_design);
		}
		if (energy_rate > 0) {
			/* TRANSLATORS: the charge or discharge rate */
			g_string_append_printf (details, "<b>%s</b> %.1f W\n",
						_("Charge rate:"), energy_rate);
		}
	}
	if (kind == UP_DEVICE_KIND_MOUSE ||
	    kind == UP_DEVICE_KIND_KEYBOARD) {
		if (energy > 0) {
			/* TRANSLATORS: the current charge for CSR devices */
			g_string_append_printf (details, "<b>%s</b> %.0f/7\n",
						_("Current charge:"), energy);
		}
		if (energy_full_design > 0) {
			/* TRANSLATORS: the design charge for CSR devices */
			g_string_append_printf (details, "<b>%s</b> %.0f/7\n",
						_("Design charge:"), energy_full_design);
		}
	}
	/* remove the last \n */
	g_string_truncate (details, details->len-1);

	g_free (vendor);
	g_free (serial);
	g_free (model);
	return g_string_free (details, FALSE);
}

/**
 * gpm_device_kind_to_localised_text:
 **/
const gchar *
gpm_device_kind_to_localised_text (UpDeviceKind kind, guint number)
{
	const gchar *text = NULL;
	switch (kind) {
	case UP_DEVICE_KIND_LINE_POWER:
		/* TRANSLATORS: system power cord */
		text = ngettext ("AC adapter", "AC adapters", number);
		break;
	case UP_DEVICE_KIND_BATTERY:
		/* TRANSLATORS: laptop primary battery */
		text = ngettext ("Laptop battery", "Laptop batteries", number);
		break;
	case UP_DEVICE_KIND_UPS:
		/* TRANSLATORS: battery-backed AC power source */
		text = ngettext ("UPS", "UPSs", number);
		break;
	case UP_DEVICE_KIND_MONITOR:
		/* TRANSLATORS: a monitor is a device to measure voltage and current */
		text = ngettext ("Monitor", "Monitors", number);
		break;
	case UP_DEVICE_KIND_MOUSE:
		/* TRANSLATORS: wireless mice with internal batteries */
		text = ngettext ("Mouse", "Mice", number);
		break;
	case UP_DEVICE_KIND_KEYBOARD:
		/* TRANSLATORS: wireless keyboard with internal battery */
		text = ngettext ("Keyboard", "Keyboards", number);
		break;
	case UP_DEVICE_KIND_PDA:
		/* TRANSLATORS: portable device */
		text = ngettext ("PDA", "PDAs", number);
		break;
	case UP_DEVICE_KIND_PHONE:
		/* TRANSLATORS: cell phone (mobile...) */
		text = ngettext ("Cell phone", "Cell phones", number);
		break;
#if UP_CHECK_VERSION(0,9,5)
	case UP_DEVICE_KIND_MEDIA_PLAYER:
		/* TRANSLATORS: media player, mp3 etc */
		text = ngettext ("Media player", "Media players", number);
		break;
	case UP_DEVICE_KIND_TABLET:
		/* TRANSLATORS: tablet device */
		text = ngettext ("Tablet", "Tablets", number);
		break;
	case UP_DEVICE_KIND_COMPUTER:
		/* TRANSLATORS: tablet device */
		text = ngettext ("Computer", "Computers", number);
		break;
#endif
	default:
		egg_warning ("enum unrecognised: %i", kind);
		text = up_device_kind_to_string (kind);
	}
	return text;
}

/**
 * gpm_device_kind_to_icon:
 **/
const gchar *
gpm_device_kind_to_icon (UpDeviceKind kind)
{
	const gchar *icon = NULL;
	switch (kind) {
	case UP_DEVICE_KIND_LINE_POWER:
		icon = "gpm-ac-adapter";
		break;
	case UP_DEVICE_KIND_BATTERY:
		icon = "battery";
		break;
	case UP_DEVICE_KIND_UPS:
		icon = "network-wired";
		break;
	case UP_DEVICE_KIND_MONITOR:
		icon = "application-certificate";
		break;
	case UP_DEVICE_KIND_MOUSE:
		icon = "input-mouse";
		break;
	case UP_DEVICE_KIND_KEYBOARD:
		icon = "input-keyboard";
		break;
	case UP_DEVICE_KIND_PDA:
		icon = "pda";
		break;
	case UP_DEVICE_KIND_PHONE:
		icon = "phone";
		break;
#if UP_CHECK_VERSION(0,9,5)
	case UP_DEVICE_KIND_MEDIA_PLAYER:
		icon = "multimedia-player";
		break;
	case UP_DEVICE_KIND_TABLET:
		icon = "input-tablet";
		break;
	case UP_DEVICE_KIND_COMPUTER:
		icon = "computer-apple-ipad";
		break;
#endif
	default:
		egg_warning ("enum unrecognised: %i", kind);
		icon = "gtk-help";
	}
	return icon;
}

/**
 * gpm_device_technology_to_localised_string:
 **/
const gchar *
gpm_device_technology_to_localised_string (UpDeviceTechnology technology_enum)
{
	const gchar *technology = NULL;
	switch (technology_enum) {
	case UP_DEVICE_TECHNOLOGY_LITHIUM_ION:
		/* TRANSLATORS: battery technology */
		technology = _("Lithium Ion");
		break;
	case UP_DEVICE_TECHNOLOGY_LITHIUM_POLYMER:
		/* TRANSLATORS: battery technology */
		technology = _("Lithium Polymer");
		break;
	case UP_DEVICE_TECHNOLOGY_LITHIUM_IRON_PHOSPHATE:
		/* TRANSLATORS: battery technology */
		technology = _("Lithium Iron Phosphate");
		break;
	case UP_DEVICE_TECHNOLOGY_LEAD_ACID:
		/* TRANSLATORS: battery technology */
		technology = _("Lead acid");
		break;
	case UP_DEVICE_TECHNOLOGY_NICKEL_CADMIUM:
		/* TRANSLATORS: battery technology */
		technology = _("Nickel Cadmium");
		break;
	case UP_DEVICE_TECHNOLOGY_NICKEL_METAL_HYDRIDE:
		/* TRANSLATORS: battery technology */
		technology = _("Nickel metal hydride");
		break;
	case UP_DEVICE_TECHNOLOGY_UNKNOWN:
		/* TRANSLATORS: battery technology */
		technology = _("Unknown technology");
		break;
	default:
		g_assert_not_reached ();
		break;
	}
	return technology;
}

/**
 * gpm_device_state_to_localised_string:
 **/
const gchar *
gpm_device_state_to_localised_string (UpDeviceState state)
{
	const gchar *state_string = NULL;

	switch (state) {
	case UP_DEVICE_STATE_CHARGING:
		/* TRANSLATORS: battery state */
		state_string = _("Charging");
		break;
	case UP_DEVICE_STATE_DISCHARGING:
		/* TRANSLATORS: battery state */
		state_string = _("Discharging");
		break;
	case UP_DEVICE_STATE_EMPTY:
		/* TRANSLATORS: battery state */
		state_string = _("Empty");
		break;
	case UP_DEVICE_STATE_FULLY_CHARGED:
		/* TRANSLATORS: battery state */
		state_string = _("Charged");
		break;
	case UP_DEVICE_STATE_PENDING_CHARGE:
		/* TRANSLATORS: battery state */
		state_string = _("Waiting to charge");
		break;
	case UP_DEVICE_STATE_PENDING_DISCHARGE:
		/* TRANSLATORS: battery state */
		state_string = _("Waiting to discharge");
		break;
	default:
		g_assert_not_reached ();
		break;
	}
	return state_string;
}

