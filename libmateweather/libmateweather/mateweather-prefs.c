/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* mateweather-prefs.c - Preference handling functions
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE__NL_MEASUREMENT_MEASUREMENT
#include <langinfo.h>
#endif

#include <mateconf/mateconf-client.h>

#define MATEWEATHER_I_KNOW_THIS_IS_UNSTABLE
#include "mateweather-prefs.h"
#include "weather-priv.h"

static MateConfEnumStringPair temp_unit_enum_map [] = {
    { TEMP_UNIT_DEFAULT,    N_("Default") },
    /* translators: Kelvin */
    { TEMP_UNIT_KELVIN,     N_("K")       },
    /* translators: Celsius */
    { TEMP_UNIT_CENTIGRADE, N_("C")       },
    /* translators: Fahrenheit */
    { TEMP_UNIT_FAHRENHEIT, N_("F")       },
    { 0, NULL }
};

static MateConfEnumStringPair speed_unit_enum_map [] = {
    { SPEED_UNIT_DEFAULT, N_("Default")        },
    /* translators: meters per second */
    { SPEED_UNIT_MS,      N_("m/s")            },
    /* translators: kilometers per hour */
    { SPEED_UNIT_KPH,     N_("km/h")           },
    /* translators: miles per hour */
    { SPEED_UNIT_MPH,     N_("mph")            },
    /* translators: knots (speed unit) */
    { SPEED_UNIT_KNOTS,   N_("knots")          },
    /* translators: wind speed */
    { SPEED_UNIT_BFT,     N_("Beaufort scale") },
    { 0, NULL }
};

static MateConfEnumStringPair pressure_unit_enum_map [] = {
    { PRESSURE_UNIT_DEFAULT, N_("Default") },
    /* translators: kilopascals */
    { PRESSURE_UNIT_KPA,     N_("kPa")     },
    /* translators: hectopascals */
    { PRESSURE_UNIT_HPA,     N_("hPa")     },
    /* translators: millibars */
    { PRESSURE_UNIT_MB,      N_("mb")      },
    /* translators: millimeters of mercury */
    { PRESSURE_UNIT_MM_HG,   N_("mmHg")    },
    /* translators: inches of mercury */
    { PRESSURE_UNIT_INCH_HG, N_("inHg")    },
    /* translators: atmosphere */
    { PRESSURE_UNIT_ATM,     N_("atm")     },
    { 0, NULL }
};

static MateConfEnumStringPair distance_unit_enum_map [] = {
    { DISTANCE_UNIT_DEFAULT, N_("Default") },
    /* translators: meters */
    { DISTANCE_UNIT_METERS,  N_("m")       },
    /* translators: kilometers */
    { DISTANCE_UNIT_KM,      N_("km")      },
    /* translators: miles */
    { DISTANCE_UNIT_MILES,   N_("mi")      },
    { 0, NULL }
};


static void
parse_temp_string (const gchar *mateconf_str, MateWeatherPrefs *prefs)
{
    gint value = 0;
#ifdef HAVE__NL_MEASUREMENT_MEASUREMENT
    char *imperial = NULL;
#endif

    prefs->temperature_unit = TEMP_UNIT_INVALID;
    prefs->use_temperature_default = TRUE;

    if ( mateconf_str && mateconf_string_to_enum (temp_unit_enum_map, mateconf_str, &value) ) {
        prefs->temperature_unit = value;

	if ((prefs->temperature_unit == TEMP_UNIT_DEFAULT) &&
	    (mateconf_string_to_enum (temp_unit_enum_map, _("DEFAULT_TEMP_UNIT"), &value)) ) {
	    prefs->temperature_unit = value;
	} else {
            prefs->use_temperature_default = FALSE;
        }
    } else {
        /* TRANSLATOR: This is the default unit to use for temperature measurements. */
        /* Valid values are: "K" (Kelvin), "C" (Celsius) and "F" (Fahrenheit) */
        if (mateconf_string_to_enum (temp_unit_enum_map, _("DEFAULT_TEMP_UNIT"), &value) ) {
            prefs->temperature_unit = value;
        }
    }
    if (!prefs->temperature_unit || prefs->temperature_unit == TEMP_UNIT_DEFAULT ) {
#ifdef HAVE__NL_MEASUREMENT_MEASUREMENT
        imperial = nl_langinfo (_NL_MEASUREMENT_MEASUREMENT);
        if ( imperial && imperial[0] == 2 )  {
            /* imperial */
            prefs->temperature_unit = TEMP_UNIT_FAHRENHEIT;
        } else
#endif
            prefs->temperature_unit = TEMP_UNIT_CENTIGRADE;
    }
}

static void
parse_speed_string (const gchar *mateconf_str, MateWeatherPrefs *prefs)
{
    gint value = 0;
#ifdef HAVE__NL_MEASUREMENT_MEASUREMENT
    char *imperial = NULL;
#endif

    prefs->speed_unit = SPEED_UNIT_INVALID;
    prefs->use_speed_default = TRUE;

    if (mateconf_str && mateconf_string_to_enum (speed_unit_enum_map, mateconf_str, &value) ) {
        prefs->speed_unit = value;
	if ((prefs->speed_unit == SPEED_UNIT_DEFAULT) &&
	    (mateconf_string_to_enum (speed_unit_enum_map, _("DEFAULT_SPEED_UNIT"), &value)) ) {
	    prefs->speed_unit = value;
	} else {
            prefs->use_speed_default = FALSE;
        }
    }
    else {
        /* TRANSLATOR: This is the default unit to use for wind speed. */
        /* Valid values are: "m/s" (meters per second), "km/h" (kilometers per hour), */
        /* "mph" (miles per hour) and "knots"  */
        if (mateconf_string_to_enum (speed_unit_enum_map, _("DEFAULT_SPEED_UNIT"), &value) ) {
            prefs->speed_unit = value;
        }
    }
    if ((!prefs->speed_unit) || prefs->speed_unit == SPEED_UNIT_DEFAULT) {
#ifdef HAVE__NL_MEASUREMENT_MEASUREMENT
        imperial = nl_langinfo (_NL_MEASUREMENT_MEASUREMENT);
        if (imperial && imperial[0] == 2)  {
            /* imperial */
            prefs->speed_unit = SPEED_UNIT_KNOTS;
        } else
#endif
            prefs->speed_unit = SPEED_UNIT_MS;
    }
}


static void
parse_pressure_string (const gchar *mateconf_str, MateWeatherPrefs *prefs)
{
    gint value = 0;
#ifdef _NL_MEASUREMENT_MEASUREMENT
    char *imperial = NULL;
#endif

    prefs->pressure_unit = PRESSURE_UNIT_INVALID;
    prefs->use_pressure_default = TRUE;

    if ( mateconf_str && mateconf_string_to_enum (pressure_unit_enum_map, mateconf_str, &value) ) {
        prefs->pressure_unit = value;

        if ((prefs->pressure_unit == PRESSURE_UNIT_DEFAULT) &&
	    (mateconf_string_to_enum (pressure_unit_enum_map, _("DEFAULT_PRESSURE_UNIT"), &value)) ) {
	    prefs->pressure_unit = value;
	} else {
            prefs->use_pressure_default = FALSE;
        }
    }
    else {
        /* TRANSLATOR: This is the default unit to use for atmospheric pressure. */
        /* Valid values are: "kPa" (kiloPascals), "hPa" (hectoPascals),
           "mb" (millibars), "mmHg" (millimeters of mercury),
           "inHg" (inches of mercury) and "atm" (atmosphere) */
        if (mateconf_string_to_enum (pressure_unit_enum_map, _("DEFAULT_PRESSURE_UNIT"), &value) ) {
            prefs->pressure_unit = value;
        }
    }
    if ( (!prefs->pressure_unit) || prefs->pressure_unit == PRESSURE_UNIT_DEFAULT ) {
#ifdef _NL_MEASUREMENT_MEASUREMENT
        imperial = nl_langinfo (_NL_MEASUREMENT_MEASUREMENT);
        if (imperial && imperial[0] == 2)  {
            /* imperial */
            prefs->pressure_unit = PRESSURE_UNIT_INCH_HG;
        } else
#endif
            prefs->pressure_unit = PRESSURE_UNIT_HPA;
    }
}

static void
parse_distance_string (const gchar *mateconf_str, MateWeatherPrefs *prefs)
{
    gint value = 0;
#ifdef _NL_MEASUREMENT_MEASUREMENT
    char *imperial = NULL;
#endif

    prefs->distance_unit = DISTANCE_UNIT_INVALID;
    prefs->use_distance_default = TRUE;
    if (mateconf_str && mateconf_string_to_enum (distance_unit_enum_map, mateconf_str, &value)) {
        prefs->distance_unit = value;

	if ((prefs->distance_unit == DISTANCE_UNIT_DEFAULT) &&
	    (mateconf_string_to_enum (distance_unit_enum_map, _("DEFAULT_DISTANCE_UNIT"), &value)) ) {
	    prefs->distance_unit = value;
	} else {
            prefs->use_distance_default = FALSE;
        }
    }
    else {
        /* TRANSLATOR: This is the default unit to use for visibility distance. */
        /* Valid values are: "m" (meters), "km" (kilometers) and "mi" (miles)   */
        if (mateconf_string_to_enum (distance_unit_enum_map, _("DEFAULT_DISTANCE_UNIT"), &value) ) {
            prefs->distance_unit = value;
        }
    }

    if ((!prefs->distance_unit) || prefs->distance_unit == DISTANCE_UNIT_DEFAULT) {
#ifdef _NL_MEASUREMENT_MEASUREMENT
        imperial = nl_langinfo (_NL_MEASUREMENT_MEASUREMENT);
        if (imperial && imperial[0] == 2)  {
            /* imperial */
            prefs->distance_unit = DISTANCE_UNIT_MILES;
        } else
#endif
            prefs->distance_unit = DISTANCE_UNIT_METERS;
    }

    return;
}

const char *
mateweather_prefs_temp_enum_to_string (TempUnit temp)
{
    return mateconf_enum_to_string (temp_unit_enum_map, temp);
}

const char *
mateweather_prefs_speed_enum_to_string (SpeedUnit speed)
{
    return mateconf_enum_to_string (speed_unit_enum_map, speed);
}

const char *
mateweather_prefs_pressure_enum_to_string (PressureUnit pressure)
{
    return mateconf_enum_to_string (pressure_unit_enum_map, pressure);
}

const char *
mateweather_prefs_distance_enum_to_string (DistanceUnit distance)
{
    return mateconf_enum_to_string (distance_unit_enum_map, distance);
}


void
mateweather_prefs_load (MateWeatherPrefs *prefs, MateWeatherMateConf *ctx)
{
    GError *error = NULL;
    gchar  *mateconf_str = NULL;

    g_return_if_fail (prefs != NULL);
    g_return_if_fail (ctx != NULL);

    if (prefs->location) {
	weather_location_free (prefs->location);
    }
    prefs->location = mateweather_mateconf_get_location (ctx);

    /* Assume we use unit defaults */
    prefs->use_temperature_default = TRUE;
    prefs->use_speed_default = TRUE;
    prefs->use_pressure_default = TRUE;
    prefs->use_distance_default = TRUE;

    prefs->update_interval =
    	mateweather_mateconf_get_int (ctx, "auto_update_interval", &error);
    if (error) {
	g_print ("%s \n", error->message);
	g_error_free (error);
	error = NULL;
    }
    prefs->update_interval = MAX (prefs->update_interval, 60);
    prefs->update_enabled =
    	mateweather_mateconf_get_bool (ctx, "auto_update", NULL);
    prefs->detailed =
    	mateweather_mateconf_get_bool (ctx, "enable_detailed_forecast", NULL);
    prefs->radar_enabled =
    	mateweather_mateconf_get_bool (ctx, "enable_radar_map", NULL);
    prefs->use_custom_radar_url =
    	mateweather_mateconf_get_bool (ctx, "use_custom_radar_url", NULL);

    if (prefs->radar) {
        g_free (prefs->radar);
        prefs->radar = NULL;
    }
    prefs->radar = mateweather_mateconf_get_string (ctx, "radar", NULL);

    mateconf_str = mateweather_mateconf_get_string (ctx, MATECONF_TEMP_UNIT, NULL);
    parse_temp_string (mateconf_str, prefs);
    g_free (mateconf_str);

    mateconf_str = mateweather_mateconf_get_string (ctx, MATECONF_SPEED_UNIT, NULL);
    parse_speed_string (mateconf_str, prefs);
    g_free (mateconf_str);

    mateconf_str = mateweather_mateconf_get_string (ctx, MATECONF_PRESSURE_UNIT, NULL);
    parse_pressure_string (mateconf_str, prefs);
    g_free (mateconf_str);

    mateconf_str = mateweather_mateconf_get_string (ctx, MATECONF_DISTANCE_UNIT, NULL);
    parse_distance_string (mateconf_str, prefs);
    g_free (mateconf_str);

    return;
}

TempUnit
mateweather_prefs_parse_temperature (const char *str, gboolean *is_default)
{
    MateWeatherPrefs prefs;

    g_return_val_if_fail (str != NULL, TEMP_UNIT_INVALID);
    g_return_val_if_fail (is_default != NULL, TEMP_UNIT_INVALID);

    parse_temp_string (str, &prefs);
    *is_default = prefs.use_temperature_default;
    return prefs.temperature_unit;
}

SpeedUnit
mateweather_prefs_parse_speed (const char *str, gboolean *is_default)
{
    MateWeatherPrefs prefs;

    g_return_val_if_fail (str != NULL, SPEED_UNIT_INVALID);
    g_return_val_if_fail (is_default != NULL, SPEED_UNIT_INVALID);

    parse_speed_string (str, &prefs);
    *is_default = prefs.use_speed_default;
    return prefs.speed_unit;
}

static const char *
get_translated_unit (int unit, MateConfEnumStringPair *pairs, int min_value, int max_value)
{
    g_return_val_if_fail (unit >= min_value && unit <= max_value, NULL);

    return _(pairs[unit - 1].str); /* minus 1 because enum value 0 is for "invalid" (look at weather.h) */
}

const char *
mateweather_prefs_get_temp_display_name (TempUnit temp)
{
    return get_translated_unit (temp, temp_unit_enum_map, TEMP_UNIT_DEFAULT, TEMP_UNIT_FAHRENHEIT);
}

const char *
mateweather_prefs_get_speed_display_name (SpeedUnit speed)
{
    return get_translated_unit (speed, speed_unit_enum_map, SPEED_UNIT_DEFAULT, SPEED_UNIT_BFT);
}

const char *
mateweather_prefs_get_pressure_display_name (PressureUnit pressure)
{
    return get_translated_unit (pressure, pressure_unit_enum_map, PRESSURE_UNIT_DEFAULT, PRESSURE_UNIT_ATM);
}

const char *
mateweather_prefs_get_distance_display_name (DistanceUnit distance)
{
    return get_translated_unit (distance, distance_unit_enum_map, DISTANCE_UNIT_DEFAULT, DISTANCE_UNIT_MILES);
}
