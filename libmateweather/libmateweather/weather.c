/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* weather.c - Overall weather server functions
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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#ifdef HAVE_VALUES_H
#include <values.h>
#endif

#include <time.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#define MATEWEATHER_I_KNOW_THIS_IS_UNSTABLE
#include "weather.h"
#include "weather-priv.h"

#define MOON_PHASES 36

static void _weather_internal_check (void);


static inline void
mateweather_gettext_init (void)
{
    static gsize mateweather_gettext_initialized = FALSE;

    if (G_UNLIKELY (g_once_init_enter (&mateweather_gettext_initialized))) {
        bindtextdomain (GETTEXT_PACKAGE, MATELOCALEDIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif
        g_once_init_leave (&mateweather_gettext_initialized, TRUE);
    }
}

const char *
mateweather_gettext (const char *str)
{
    mateweather_gettext_init ();
    return dgettext (GETTEXT_PACKAGE, str);
}

const char *
mateweather_dpgettext (const char *context,
                    const char *str)
{
    mateweather_gettext_init ();
    return g_dpgettext2 (GETTEXT_PACKAGE, context, str);
}

/*
 * Convert string of the form "DD-MM-SSH" to radians
 * DD:degrees (to 3 digits), MM:minutes, SS:seconds H:hemisphere (NESW)
 * Return value is positive for N,E; negative for S,W.
 */
static gdouble
dmsh2rad (const gchar *latlon)
{
    char *p1, *p2;
    int deg, min, sec, dir;
    gdouble value;

    if (latlon == NULL)
	return DBL_MAX;
    p1 = strchr (latlon, '-');
    p2 = strrchr (latlon, '-');
    if (p1 == NULL || p1 == latlon) {
        return DBL_MAX;
    } else if (p1 == p2) {
	sscanf (latlon, "%d-%d", &deg, &min);
	sec = 0;
    } else if (p2 == 1 + p1) {
	return DBL_MAX;
    } else {
	sscanf (latlon, "%d-%d-%d", &deg, &min, &sec);
    }
    if (deg > 180 || min >= 60 || sec >= 60)
	return DBL_MAX;
    value = (gdouble)((deg * 60 + min) * 60 + sec) * M_PI / 648000.;

    dir = g_ascii_toupper (latlon[strlen (latlon) - 1]);
    if (dir == 'W' || dir == 'S')
	value = -value;
    else if (dir != 'E' && dir != 'N' && (value != 0.0 || dir != '0'))
	value = DBL_MAX;
    return value;
}

WeatherLocation *
weather_location_new (const gchar *name, const gchar *code,
		      const gchar *zone, const gchar *radar,
		      const gchar *coordinates,
		      const gchar *country_code,
		      const gchar *tz_hint)
{
    WeatherLocation *location;

    _weather_internal_check ();

    location = g_new (WeatherLocation, 1);

    /* name and metar code must be set */
    location->name = g_strdup (name);
    location->code = g_strdup (code);

    if (zone) {
        location->zone = g_strdup (zone);
    } else {
        location->zone = g_strdup ("------");
    }

    if (radar) {
        location->radar = g_strdup (radar);
    } else {
        location->radar = g_strdup ("---");
    }

    if (location->zone[0] == '-') {
        location->zone_valid = FALSE;
    } else {
        location->zone_valid = TRUE;
    }

    location->coordinates = NULL;
    if (coordinates)
    {
	char **pieces;

	pieces = g_strsplit (coordinates, " ", -1);

	if (g_strv_length (pieces) == 2)
	{
            location->coordinates = g_strdup (coordinates);
            location->latitude = dmsh2rad (pieces[0]);
	    location->longitude = dmsh2rad (pieces[1]);
	}

	g_strfreev (pieces);
    }

    if (!location->coordinates)
    {
        location->coordinates = g_strdup ("---");
        location->latitude = DBL_MAX;
        location->longitude = DBL_MAX;
    }

    location->latlon_valid = (location->latitude < DBL_MAX && location->longitude < DBL_MAX);

    location->country_code = g_strdup (country_code);
    location->tz_hint = g_strdup (tz_hint);

    return location;
}

WeatherLocation *
weather_location_clone (const WeatherLocation *location)
{
    WeatherLocation *clone;

    g_return_val_if_fail (location != NULL, NULL);

    clone = weather_location_new (location->name,
				  location->code, location->zone,
				  location->radar, location->coordinates,
				  location->country_code, location->tz_hint);
    clone->latitude = location->latitude;
    clone->longitude = location->longitude;
    clone->latlon_valid = location->latlon_valid;
    return clone;
}

void
weather_location_free (WeatherLocation *location)
{
    if (location) {
        g_free (location->name);
        g_free (location->code);
        g_free (location->zone);
        g_free (location->radar);
        g_free (location->coordinates);
        g_free (location->country_code);
        g_free (location->tz_hint);

        g_free (location);
    }
}

gboolean
weather_location_equal (const WeatherLocation *location1, const WeatherLocation *location2)
{
    /* if something is NULL, then it's TRUE if and only if both are NULL) */
    if (location1 == NULL || location2 == NULL)
        return (location1 == location2);
    if (!location1->code || !location2->code)
        return (location1->code == location2->code);
    if (!location1->name || !location2->name)
        return (location1->name == location2->name);

    return ((strcmp (location1->code, location2->code) == 0) &&
	    (strcmp (location1->name, location2->name) == 0));
}

static const gchar *wind_direction_str[] = {
    N_("Variable"),
    N_("North"), N_("North - NorthEast"), N_("Northeast"), N_("East - NorthEast"),
    N_("East"), N_("East - Southeast"), N_("Southeast"), N_("South - Southeast"),
    N_("South"), N_("South - Southwest"), N_("Southwest"), N_("West - Southwest"),
    N_("West"), N_("West - Northwest"), N_("Northwest"), N_("North - Northwest")
};

const gchar *
weather_wind_direction_string (WeatherWindDirection wind)
{
    if (wind <= WIND_INVALID || wind >= WIND_LAST)
	return _("Invalid");

    return _(wind_direction_str[(int)wind]);
}

static const gchar *sky_str[] = {
    N_("Clear Sky"),
    N_("Broken clouds"),
    N_("Scattered clouds"),
    N_("Few clouds"),
    N_("Overcast")
};

const gchar *
weather_sky_string (WeatherSky sky)
{
    if (sky <= SKY_INVALID || sky >= SKY_LAST)
	return _("Invalid");

    return _(sky_str[(int)sky]);
}


/*
 * Even though tedious, I switched to a 2D array for weather condition
 * strings, in order to facilitate internationalization, esp. for languages
 * with genders.
 */

/*
 * Almost all reportable combinations listed in
 * http://www.crh.noaa.gov/arx/wx.tbl.php are entered below, except those
 * having 2 qualifiers mixed together [such as "Blowing snow in vicinity"
 * (VCBLSN), "Thunderstorm in vicinity" (VCTS), etc].
 * Combinations that are not possible are filled in with "??".
 * Some other exceptions not handled yet, such as "SN BLSN" which has
 * special meaning.
 */

/*
 * Note, magic numbers, when you change the size here, make sure to change
 * the below function so that new values are recognized
 */
/*                   NONE                         VICINITY                             LIGHT                      MODERATE                      HEAVY                      SHALLOW                      PATCHES                         PARTIAL                      THUNDERSTORM                    BLOWING                      SHOWERS                         DRIFTING                      FREEZING                      */
/*               *******************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************/
static const gchar *conditions_str[24][13] = {
/* TRANSLATOR: If you want to know what "blowing" "shallow" "partial"
 * etc means, you can go to http://www.weather.com/glossary/ and
 * http://www.crh.noaa.gov/arx/wx.tbl.php */
    /* NONE          */ {"??",                        "??",                                "??",                      "??",                         "??",                      "??",                        "??",                           "??",                        N_("Thunderstorm"),             "??",                        "??",                           "??",                         "??"                         },
    /* DRIZZLE       */ {N_("Drizzle"),               "??",                                N_("Light drizzle"),       N_("Moderate drizzle"),       N_("Heavy drizzle"),       "??",                        "??",                           "??",                        "??",                           "??",                        "??",                           "??",                         N_("Freezing drizzle")       },
    /* RAIN          */ {N_("Rain"),                  "??",                                N_("Light rain"),          N_("Moderate rain"),          N_("Heavy rain"),          "??",                        "??",                           "??",                        N_("Thunderstorm"),             "??",                        N_("Rain showers"),             "??",                         N_("Freezing rain")          },
    /* SNOW          */ {N_("Snow"),                  "??",                                N_("Light snow"),          N_("Moderate snow"),          N_("Heavy snow"),          "??",                        "??",                           "??",                        N_("Snowstorm"),                N_("Blowing snowfall"),      N_("Snow showers"),             N_("Drifting snow"),          "??"                         },
    /* SNOW_GRAINS   */ {N_("Snow grains"),           "??",                                N_("Light snow grains"),   N_("Moderate snow grains"),   N_("Heavy snow grains"),   "??",                        "??",                           "??",                        "??",                           "??",                        "??",                           "??",                         "??"                         },
    /* ICE_CRYSTALS  */ {N_("Ice crystals"),          "??",                                "??",                      N_("Ice crystals"),           "??",                      "??",                        "??",                           "??",                        "??",                           "??",                        "??",                           "??",                         "??"                         },
    /* ICE_PELLETS   */ {N_("Ice pellets"),           "??",                                N_("Few ice pellets"),     N_("Moderate ice pellets"),   N_("Heavy ice pellets"),   "??",                        "??",                           "??",                        N_("Ice pellet storm"),         "??",                        N_("Showers of ice pellets"),   "??",                         "??"                         },
    /* HAIL          */ {N_("Hail"),                  "??",                                "??",                      N_("Hail"),                   "??",                      "??",                        "??",                           "??",                        N_("Hailstorm"),                "??",                        N_("Hail showers"),             "??",                         "??",                        },
    /* SMALL_HAIL    */ {N_("Small hail"),            "??",                                "??",                      N_("Small hail"),             "??",                      "??",                        "??",                           "??",                        N_("Small hailstorm"),          "??",                        N_("Showers of small hail"),    "??",                         "??"                         },
    /* PRECIPITATION */ {N_("Unknown precipitation"), "??",                                "??",                      "??",                         "??",                      "??",                        "??",                           "??",                        "??",                           "??",                        "??",                           "??",                         "??"                         },
    /* MIST          */ {N_("Mist"),                  "??",                                "??",                      N_("Mist"),                   "??",                      "??",                        "??",                           "??",                        "??",                           "??",                        "??",                           "??",                         "??"                         },
    /* FOG           */ {N_("Fog"),                   N_("Fog in the vicinity") ,          "??",                      N_("Fog"),                    "??",                      N_("Shallow fog"),           N_("Patches of fog"),           N_("Partial fog"),           "??",                           "??",                        "??",                           "??",                         N_("Freezing fog")           },
    /* SMOKE         */ {N_("Smoke"),                 "??",                                "??",                      N_("Smoke"),                  "??",                      "??",                        "??",                           "??",                        "??",                           "??",                        "??",                           "??",                         "??"                         },
    /* VOLCANIC_ASH  */ {N_("Volcanic ash"),          "??",                                "??",                      N_("Volcanic ash"),           "??",                      "??",                        "??",                           "??",                        "??",                           "??",                        "??",                           "??",                         "??"                         },
    /* SAND          */ {N_("Sand"),                  "??",                                "??",                      N_("Sand"),                   "??",                      "??",                        "??",                           "??",                        "??",                           N_("Blowing sand"),          "",                             N_("Drifting sand"),          "??"                         },
    /* HAZE          */ {N_("Haze"),                  "??",                                "??",                      N_("Haze"),                   "??",                      "??",                        "??",                           "??",                        "??",                           "??",                        "??",                           "??",                         "??"                         },
    /* SPRAY         */ {"??",                        "??",                                "??",                      "??",                         "??",                      "??",                        "??",                           "??",                        "??",                           N_("Blowing sprays"),        "??",                           "??",                         "??"                         },
    /* DUST          */ {N_("Dust"),                  "??",                                "??",                      N_("Dust"),                   "??",                      "??",                        "??",                           "??",                        "??",                           N_("Blowing dust"),          "??",                           N_("Drifting dust"),          "??"                         },
    /* SQUALL        */ {N_("Squall"),                "??",                                "??",                      N_("Squall"),                 "??",                      "??",                        "??",                           "??",                        "??",                           "??",                        "??",                           "??",                         "??"                         },
    /* SANDSTORM     */ {N_("Sandstorm"),             N_("Sandstorm in the vicinity") ,    "??",                      N_("Sandstorm"),              N_("Heavy sandstorm"),     "??",                        "??",                           "??",                        "??",                           "??",                        "??",                           "??",                         "??"                         },
    /* DUSTSTORM     */ {N_("Duststorm"),             N_("Duststorm in the vicinity") ,    "??",                      N_("Duststorm"),              N_("Heavy duststorm"),     "??",                        "??",                           "??",                        "??",                           "??",                        "??",                           "??",                         "??"                         },
    /* FUNNEL_CLOUD  */ {N_("Funnel cloud"),          "??",                                "??",                      "??",                         "??",                      "??",                        "??",                           "??",                        "??",                           "??",                        "??",                           "??",                         "??"                         },
    /* TORNADO       */ {N_("Tornado"),               "??",                                "??",                      "??",                         "??",                      "??",                        "??",                           "??",                        "??",                           "??",                        "??",                           "??",                         "??"                         },
    /* DUST_WHIRLS   */ {N_("Dust whirls"),           N_("Dust whirls in the vicinity") ,  "??",                      N_("Dust whirls"),            "??",                      "??",                        "??",                           "??",                        "??",                           "??",                        "??",                           "??",                         "??"                         }
};

const gchar *
weather_conditions_string (WeatherConditions cond)
{
    const gchar *str;

    if (!cond.significant) {
	return "-";
    } else {
	if (cond.phenomenon > PHENOMENON_INVALID &&
	    cond.phenomenon < PHENOMENON_LAST &&
	    cond.qualifier > QUALIFIER_INVALID &&
	    cond.qualifier < QUALIFIER_LAST)
	    str = _(conditions_str[(int)cond.phenomenon][(int)cond.qualifier]);
	else
	    str = _("Invalid");
	return (strlen (str) > 0) ? str : "-";
    }
}

/* Locals turned global to facilitate asynchronous HTTP requests */


gboolean
requests_init (WeatherInfo *info)
{
    if (info->requests_pending)
        return FALSE;

    return TRUE;
}

void request_done (WeatherInfo *info, gboolean ok)
{
    if (ok) {
	(void) calc_sun (info);
	info->moonValid = info->valid && calc_moon (info);
    }
    if (!--info->requests_pending)
        info->finish_cb (info, info->cb_data);
}

/* it's OK to pass in NULL */
void
free_forecast_list (WeatherInfo *info)
{
    GSList *p;

    if (!info)
	return;

    for (p = info->forecast_list; p; p = p->next)
	weather_info_free (p->data);

    if (info->forecast_list) {
	g_slist_free (info->forecast_list);
	info->forecast_list = NULL;
    }
}

/* Relative humidity computation - thanks to <Olof.Oberg@modopaper.modogroup.com> */

static inline gdouble
calc_humidity (gdouble temp, gdouble dewp)
{
    gdouble esat, esurf;

    if (temp > -500.0 && dewp > -500.0) {
	temp = TEMP_F_TO_C (temp);
	dewp = TEMP_F_TO_C (dewp);

	esat = 6.11 * pow (10.0, (7.5 * temp) / (237.7 + temp));
	esurf = 6.11 * pow (10.0, (7.5 * dewp) / (237.7 + dewp));
    } else {
	esurf = -1.0;
	esat = 1.0;
    }
    return ((esurf/esat) * 100.0);
}

static inline gdouble
calc_apparent (WeatherInfo *info)
{
    gdouble temp = info->temp;
    gdouble wind = WINDSPEED_KNOTS_TO_MPH (info->windspeed);
    gdouble apparent = -1000.;

    /*
     * Wind chill calculations as of 01-Nov-2001
     * http://www.nws.noaa.gov/om/windchill/index.shtml
     * Some pages suggest that the formula will soon be adjusted
     * to account for solar radiation (bright sun vs cloudy sky)
     */
    if (temp <= 50.0) {
        if (wind > 3.0) {
	    gdouble v = pow (wind, 0.16);
	    apparent = 35.74 + 0.6215 * temp - 35.75 * v + 0.4275 * temp * v;
	} else if (wind >= 0.) {
	    apparent = temp;
	}
    }
    /*
     * Heat index calculations:
     * http://www.srh.noaa.gov/fwd/heatindex/heat5.html
     */
    else if (temp >= 80.0) {
        if (info->temp >= -500. && info->dew >= -500.) {
	    gdouble humidity = calc_humidity (info->temp, info->dew);
	    gdouble t2 = temp * temp;
	    gdouble h2 = humidity * humidity;

#if 1
	    /*
	     * A really precise formula.  Note that overall precision is
	     * constrained by the accuracy of the instruments and that the
	     * we receive the temperature and dewpoints as integers.
	     */
	    gdouble t3 = t2 * temp;
	    gdouble h3 = h2 * temp;

	    apparent = 16.923
		+ 0.185212 * temp
		+ 5.37941 * humidity
		- 0.100254 * temp * humidity
		+ 9.41695e-3 * t2
		+ 7.28898e-3 * h2
		+ 3.45372e-4 * t2 * humidity
		- 8.14971e-4 * temp * h2
		+ 1.02102e-5 * t2 * h2
		- 3.8646e-5 * t3
		+ 2.91583e-5 * h3
		+ 1.42721e-6 * t3 * humidity
		+ 1.97483e-7 * temp * h3
		- 2.18429e-8 * t3 * h2
		+ 8.43296e-10 * t2 * h3
		- 4.81975e-11 * t3 * h3;
#else
	    /*
	     * An often cited alternative: values are within 5 degrees for
	     * most ranges between 10% and 70% humidity and to 110 degrees.
	     */
	    apparent = - 42.379
		+  2.04901523 * temp
		+ 10.14333127 * humidity
		-  0.22475541 * temp * humidity
		-  6.83783e-3 * t2
		-  5.481717e-2 * h2
		+  1.22874e-3 * t2 * humidity
		+  8.5282e-4 * temp * h2
		-  1.99e-6 * t2 * h2;
#endif
	}
    } else {
        apparent = temp;
    }

    return apparent;
}

WeatherInfo *
_weather_info_fill (WeatherInfo *info,
		    WeatherLocation *location,
		    const WeatherPrefs *prefs,
		    WeatherInfoFunc cb,
		    gpointer data)
{
    g_return_val_if_fail (((info == NULL) && (location != NULL)) || \
			  ((info != NULL) && (location == NULL)), NULL);
    g_return_val_if_fail (prefs != NULL, NULL);

    /* FIXME: i'm not sure this works as intended anymore */
    if (!info) {
    	info = g_new0 (WeatherInfo, 1);
    	info->requests_pending = 0;
    	info->location = weather_location_clone (location);
    } else {
        location = info->location;
	if (info->forecast)
	    g_free (info->forecast);
	info->forecast = NULL;

	free_forecast_list (info);

	if (info->radar != NULL) {
	    g_object_unref (info->radar);
	    info->radar = NULL;
	}
    }

    /* Update in progress */
    if (!requests_init (info)) {
        return NULL;
    }

    /* Defaults (just in case...) */
    /* Well, no just in case anymore.  We may actually fail to fetch some
     * fields. */
    info->forecast_type = prefs->type;

    info->temperature_unit = prefs->temperature_unit;
    info->speed_unit = prefs->speed_unit;
    info->pressure_unit = prefs->pressure_unit;
    info->distance_unit = prefs->distance_unit;

    info->update = 0;
    info->sky = -1;
    info->cond.significant = FALSE;
    info->cond.phenomenon = PHENOMENON_NONE;
    info->cond.qualifier = QUALIFIER_NONE;
    info->temp = -1000.0;
    info->tempMinMaxValid = FALSE;
    info->temp_min = -1000.0;
    info->temp_max = -1000.0;
    info->dew = -1000.0;
    info->wind = -1;
    info->windspeed = -1;
    info->pressure = -1.0;
    info->visibility = -1.0;
    info->sunriseValid = FALSE;
    info->sunsetValid = FALSE;
    info->moonValid = FALSE;
    info->sunrise = 0;
    info->sunset = 0;
    info->moonphase = 0;
    info->moonlatitude = 0;
    info->forecast = NULL;
    info->forecast_list = NULL;
    info->radar = NULL;
    info->radar_url = prefs->radar && prefs->radar_custom_url ?
    		      g_strdup (prefs->radar_custom_url) : NULL;
    info->finish_cb = cb;
    info->cb_data = data;

    if (!info->session) {
	info->session = soup_session_async_new ();
#ifdef HAVE_LIBSOUP_MATE
	soup_session_add_feature_by_type (info->session, SOUP_TYPE_PROXY_RESOLVER_MATE);
#endif
    }

    metar_start_open (info);
    iwin_start_open (info);

    if (prefs->radar) {
        wx_start_open (info);
    }

    return info;
}

void
weather_info_abort (WeatherInfo *info)
{
    g_return_if_fail (info != NULL);

    if (info->session) {
	soup_session_abort (info->session);
	info->requests_pending = 0;
    }
}

WeatherInfo *
weather_info_clone (const WeatherInfo *info)
{
    WeatherInfo *clone;

    g_return_val_if_fail (info != NULL, NULL);

    clone = g_new (WeatherInfo, 1);


    /* move everything */
    g_memmove (clone, info, sizeof (WeatherInfo));


    /* special moves */
    clone->location = weather_location_clone (info->location);
    /* This handles null correctly */
    clone->forecast = g_strdup (info->forecast);
    clone->radar_url = g_strdup (info->radar_url);

    if (info->forecast_list) {
	GSList *p;

	clone->forecast_list = NULL;
	for (p = info->forecast_list; p; p = p->next) {
	    clone->forecast_list = g_slist_prepend (clone->forecast_list, weather_info_clone (p->data));
	}

	clone->forecast_list = g_slist_reverse (clone->forecast_list);
    }

    clone->radar = info->radar;
    if (clone->radar != NULL)
	g_object_ref (clone->radar);

    return clone;
}

void
weather_info_free (WeatherInfo *info)
{
    if (!info)
        return;

    weather_info_abort (info);
    if (info->session)
	g_object_unref (info->session);

    weather_location_free (info->location);
    info->location = NULL;

    g_free (info->forecast);
    info->forecast = NULL;

    free_forecast_list (info);

    if (info->radar != NULL) {
        g_object_unref (info->radar);
        info->radar = NULL;
    }
	
    g_free (info);
}

gboolean
weather_info_is_valid (WeatherInfo *info)
{
    g_return_val_if_fail (info != NULL, FALSE);
    return info->valid;
}

gboolean
weather_info_network_error (WeatherInfo *info)
{
    g_return_val_if_fail (info != NULL, FALSE);
    return info->network_error;
}

void
weather_info_to_metric (WeatherInfo *info)
{
    g_return_if_fail (info != NULL);

    info->temperature_unit = TEMP_UNIT_CENTIGRADE;
    info->speed_unit = SPEED_UNIT_MS;
    info->pressure_unit = PRESSURE_UNIT_HPA;
    info->distance_unit = DISTANCE_UNIT_METERS;
}

void
weather_info_to_imperial (WeatherInfo *info)
{
    g_return_if_fail (info != NULL);

    info->temperature_unit = TEMP_UNIT_FAHRENHEIT;
    info->speed_unit = SPEED_UNIT_MPH;
    info->pressure_unit = PRESSURE_UNIT_INCH_HG;
    info->distance_unit = DISTANCE_UNIT_MILES;
}

const WeatherLocation *
weather_info_get_location (WeatherInfo *info)
{
    g_return_val_if_fail (info != NULL, NULL);
    return info->location;
}

const gchar *
weather_info_get_location_name (WeatherInfo *info)
{
    g_return_val_if_fail (info != NULL, NULL);
    g_return_val_if_fail (info->location != NULL, NULL);
    return info->location->name;
}

const gchar *
weather_info_get_update (WeatherInfo *info)
{
    static gchar buf[200];
    char *utf8, *timeformat;

    g_return_val_if_fail (info != NULL, NULL);

    if (!info->valid)
        return "-";

    if (info->update != 0) {
        struct tm tm;
        localtime_r (&info->update, &tm);
	/* TRANSLATOR: this is a format string for strftime
	 *             see `man 3 strftime` for more details
	 */
	timeformat = g_locale_from_utf8 (_("%a, %b %d / %H:%M"), -1,
					 NULL, NULL, NULL);
	if (!timeformat) {
	    strcpy (buf, "???");
	}
	else if (strftime (buf, sizeof (buf), timeformat, &tm) <= 0) {
	    strcpy (buf, "???");
	}
	g_free (timeformat);

	/* Convert to UTF-8 */
	utf8 = g_locale_to_utf8 (buf, -1, NULL, NULL, NULL);
	strcpy (buf, utf8);
	g_free (utf8);
    } else {
        strncpy (buf, _("Unknown observation time"), sizeof (buf));
	buf[sizeof (buf)-1] = '\0';
    }

    return buf;
}

const gchar *
weather_info_get_sky (WeatherInfo *info)
{
    g_return_val_if_fail (info != NULL, NULL);
    if (!info->valid)
        return "-";
    if (info->sky < 0)
	return _("Unknown");
    return weather_sky_string (info->sky);
}

const gchar *
weather_info_get_conditions (WeatherInfo *info)
{
    g_return_val_if_fail (info != NULL, NULL);
    if (!info->valid)
        return "-";
    return weather_conditions_string (info->cond);
}

static const gchar *
temperature_string (gfloat temp_f, TempUnit to_unit, gboolean want_round)
{
    static gchar buf[100];

    switch (to_unit) {
    case TEMP_UNIT_FAHRENHEIT:
	if (!want_round) {
	    /* TRANSLATOR: This is the temperature in degrees Fahrenheit (\302\260 is U+00B0 DEGREE SIGN) */
	    g_snprintf (buf, sizeof (buf), _("%.1f \302\260F"), temp_f);
	} else {
	    /* TRANSLATOR: This is the temperature in degrees Fahrenheit (\302\260 is U+00B0 DEGREE SIGN) */
	    g_snprintf (buf, sizeof (buf), _("%d \302\260F"), (int)floor (temp_f + 0.5));
	}
	break;
    case TEMP_UNIT_CENTIGRADE:
	if (!want_round) {
	    /* TRANSLATOR: This is the temperature in degrees Celsius (\302\260 is U+00B0 DEGREE SIGN) */
	    g_snprintf (buf, sizeof (buf), _("%.1f \302\260C"), TEMP_F_TO_C (temp_f));
	} else {
	    /* TRANSLATOR: This is the temperature in degrees Celsius (\302\260 is U+00B0 DEGREE SIGN) */
	    g_snprintf (buf, sizeof (buf), _("%d \302\260C"), (int)floor (TEMP_F_TO_C (temp_f) + 0.5));
	}
	break;
    case TEMP_UNIT_KELVIN:
	if (!want_round) {
	    /* TRANSLATOR: This is the temperature in kelvin */
	    g_snprintf (buf, sizeof (buf), _("%.1f K"), TEMP_F_TO_K (temp_f));
	} else {
	    /* TRANSLATOR: This is the temperature in kelvin */
	    g_snprintf (buf, sizeof (buf), _("%d K"), (int)floor (TEMP_F_TO_K (temp_f)));
	}
	break;

    case TEMP_UNIT_INVALID:
    case TEMP_UNIT_DEFAULT:
    default:
	g_warning ("Conversion to illegal temperature unit: %d", to_unit);
	return _("Unknown");
    }

    return buf;
}

const gchar *
weather_info_get_temp (WeatherInfo *info)
{
    g_return_val_if_fail (info != NULL, NULL);

    if (!info->valid)
        return "-";
    if (info->temp < -500.0)
        return _("Unknown");

    return temperature_string (info->temp, info->temperature_unit, FALSE);
}

const gchar *
weather_info_get_temp_min (WeatherInfo *info)
{
    g_return_val_if_fail (info != NULL, NULL);

    if (!info->valid || !info->tempMinMaxValid)
        return "-";
    if (info->temp_min < -500.0)
        return _("Unknown");

    return temperature_string (info->temp_min, info->temperature_unit, FALSE);
}

const gchar *
weather_info_get_temp_max (WeatherInfo *info)
{
    g_return_val_if_fail (info != NULL, NULL);

    if (!info->valid || !info->tempMinMaxValid)
        return "-";
    if (info->temp_max < -500.0)
        return _("Unknown");

    return temperature_string (info->temp_max, info->temperature_unit, FALSE);
}

const gchar *
weather_info_get_dew (WeatherInfo *info)
{
    g_return_val_if_fail (info != NULL, NULL);

    if (!info->valid)
        return "-";
    if (info->dew < -500.0)
        return _("Unknown");

    return temperature_string (info->dew, info->temperature_unit, FALSE);
}

const gchar *
weather_info_get_humidity (WeatherInfo *info)
{
    static gchar buf[20];
    gdouble humidity;

    g_return_val_if_fail (info != NULL, NULL);

    if (!info->valid)
        return "-";

    humidity = calc_humidity (info->temp, info->dew);
    if (humidity < 0.0)
        return _("Unknown");

    /* TRANSLATOR: This is the humidity in percent */
    g_snprintf (buf, sizeof (buf), _("%.f%%"), humidity);
    return buf;
}

const gchar *
weather_info_get_apparent (WeatherInfo *info)
{
    gdouble apparent;

    g_return_val_if_fail (info != NULL, NULL);
    if (!info->valid)
        return "-";

    apparent = calc_apparent (info);
    if (apparent < -500.0)
        return _("Unknown");

    return temperature_string (apparent, info->temperature_unit, FALSE);
}

static const gchar *
windspeed_string (gfloat knots, SpeedUnit to_unit)
{
    static gchar buf[100];

    switch (to_unit) {
    case SPEED_UNIT_KNOTS:
	/* TRANSLATOR: This is the wind speed in knots */
	g_snprintf (buf, sizeof (buf), _("%0.1f knots"), knots);
	break;
    case SPEED_UNIT_MPH:
	/* TRANSLATOR: This is the wind speed in miles per hour */
	g_snprintf (buf, sizeof (buf), _("%.1f mph"), WINDSPEED_KNOTS_TO_MPH (knots));
	break;
    case SPEED_UNIT_KPH:
	/* TRANSLATOR: This is the wind speed in kilometers per hour */
	g_snprintf (buf, sizeof (buf), _("%.1f km/h"), WINDSPEED_KNOTS_TO_KPH (knots));
	break;
    case SPEED_UNIT_MS:
	/* TRANSLATOR: This is the wind speed in meters per second */
	g_snprintf (buf, sizeof (buf), _("%.1f m/s"), WINDSPEED_KNOTS_TO_MS (knots));
	break;
    case SPEED_UNIT_BFT:
	/* TRANSLATOR: This is the wind speed as a Beaufort force factor
	 * (commonly used in nautical wind estimation).
	 */
	g_snprintf (buf, sizeof (buf), _("Beaufort force %.1f"),
		    WINDSPEED_KNOTS_TO_BFT (knots));
	break;
    case SPEED_UNIT_INVALID:
    case SPEED_UNIT_DEFAULT:
    default:
	g_warning ("Conversion to illegal speed unit: %d", to_unit);
	return _("Unknown");
    }

    return buf;
}

const gchar *
weather_info_get_wind (WeatherInfo *info)
{
    static gchar buf[200];

    g_return_val_if_fail (info != NULL, NULL);

    if (!info->valid)
        return "-";
    if (info->windspeed < 0.0 || info->wind < 0)
        return _("Unknown");
    if (info->windspeed == 0.00) {
        strncpy (buf, _("Calm"), sizeof (buf));
	buf[sizeof (buf)-1] = '\0';
    } else {
        /* TRANSLATOR: This is 'wind direction' / 'wind speed' */
        g_snprintf (buf, sizeof (buf), _("%s / %s"),
		    weather_wind_direction_string (info->wind),
		    windspeed_string (info->windspeed, info->speed_unit));
    }
    return buf;
}

const gchar *
weather_info_get_pressure (WeatherInfo *info)
{
    static gchar buf[100];

    g_return_val_if_fail (info != NULL, NULL);

    if (!info->valid)
        return "-";
    if (info->pressure < 0.0)
        return _("Unknown");

    switch (info->pressure_unit) {
    case PRESSURE_UNIT_INCH_HG:
	/* TRANSLATOR: This is pressure in inches of mercury */
	g_snprintf (buf, sizeof (buf), _("%.2f inHg"), info->pressure);
	break;
    case PRESSURE_UNIT_MM_HG:
	/* TRANSLATOR: This is pressure in millimeters of mercury */
	g_snprintf (buf, sizeof (buf), _("%.1f mmHg"), PRESSURE_INCH_TO_MM (info->pressure));
	break;
    case PRESSURE_UNIT_KPA:
	/* TRANSLATOR: This is pressure in kiloPascals */
	g_snprintf (buf, sizeof (buf), _("%.2f kPa"), PRESSURE_INCH_TO_KPA (info->pressure));
	break;
    case PRESSURE_UNIT_HPA:
	/* TRANSLATOR: This is pressure in hectoPascals */
	g_snprintf (buf, sizeof (buf), _("%.2f hPa"), PRESSURE_INCH_TO_HPA (info->pressure));
	break;
    case PRESSURE_UNIT_MB:
	/* TRANSLATOR: This is pressure in millibars */
	g_snprintf (buf, sizeof (buf), _("%.2f mb"), PRESSURE_INCH_TO_MB (info->pressure));
	break;
    case PRESSURE_UNIT_ATM:
	/* TRANSLATOR: This is pressure in atmospheres */
	g_snprintf (buf, sizeof (buf), _("%.3f atm"), PRESSURE_INCH_TO_ATM (info->pressure));
	break;

    case PRESSURE_UNIT_INVALID:
    case PRESSURE_UNIT_DEFAULT:
    default:
	g_warning ("Conversion to illegal pressure unit: %d", info->pressure_unit);
	return _("Unknown");
    }

    return buf;
}

const gchar *
weather_info_get_visibility (WeatherInfo *info)
{
    static gchar buf[100];

    g_return_val_if_fail (info != NULL, NULL);

    if (!info->valid)
        return "-";
    if (info->visibility < 0.0)
        return _("Unknown");

    switch (info->distance_unit) {
    case DISTANCE_UNIT_MILES:
	/* TRANSLATOR: This is the visibility in miles */
	g_snprintf (buf, sizeof (buf), _("%.1f miles"), info->visibility);
	break;
    case DISTANCE_UNIT_KM:
	/* TRANSLATOR: This is the visibility in kilometers */
	g_snprintf (buf, sizeof (buf), _("%.1f km"), VISIBILITY_SM_TO_KM (info->visibility));
	break;
    case DISTANCE_UNIT_METERS:
	/* TRANSLATOR: This is the visibility in meters */
	g_snprintf (buf, sizeof (buf), _("%.0fm"), VISIBILITY_SM_TO_M (info->visibility));
	break;

    case DISTANCE_UNIT_INVALID:
    case DISTANCE_UNIT_DEFAULT:
    default:
	g_warning ("Conversion to illegal visibility unit: %d", info->pressure_unit);
	return _("Unknown");
    }

    return buf;
}

const gchar *
weather_info_get_sunrise (WeatherInfo *info)
{
    static gchar buf[200];
    struct tm tm;

    g_return_val_if_fail (info && info->location, NULL);

    if (!info->location->latlon_valid)
        return "-";
    if (!info->valid)
        return "-";
    if (!calc_sun (info))
        return "-";

    localtime_r (&info->sunrise, &tm);
    if (strftime (buf, sizeof (buf), _("%H:%M"), &tm) <= 0)
        return "-";
    return buf;
}

const gchar *
weather_info_get_sunset (WeatherInfo *info)
{
    static gchar buf[200];
    struct tm tm;

    g_return_val_if_fail (info && info->location, NULL);

    if (!info->location->latlon_valid)
        return "-";
    if (!info->valid)
        return "-";
    if (!calc_sun (info))
        return "-";

    localtime_r (&info->sunset, &tm);
    if (strftime (buf, sizeof (buf), _("%H:%M"), &tm) <= 0)
        return "-";
    return buf;
}

const gchar *
weather_info_get_forecast (WeatherInfo *info)
{
    g_return_val_if_fail (info != NULL, NULL);
    return info->forecast;
}

/**
 * weather_info_get_forecast_list:
 * Returns list of WeatherInfo* objects for the forecast.
 * The list is owned by the 'info' object thus is alive as long
 * as the 'info'. This list is filled only when requested with
 * type FORECAST_LIST and if available for given location.
 * The 'update' property is the date/time when the forecast info
 * is used for.
 **/
GSList *
weather_info_get_forecast_list (WeatherInfo *info)
{
    g_return_val_if_fail (info != NULL, NULL);

    if (!info->valid)
	return NULL;

    return info->forecast_list;
}

GdkPixbufAnimation *
weather_info_get_radar (WeatherInfo *info)
{
    g_return_val_if_fail (info != NULL, NULL);
    return info->radar;
}

const gchar *
weather_info_get_temp_summary (WeatherInfo *info)
{
    g_return_val_if_fail (info != NULL, NULL);

    if (!info->valid || info->temp < -500.0)
        return "--";

    return temperature_string (info->temp, info->temperature_unit, TRUE);

}

gchar *
weather_info_get_weather_summary (WeatherInfo *info)
{
    const gchar *buf;

    g_return_val_if_fail (info != NULL, NULL);

    if (!info->valid)
	return g_strdup (_("Retrieval failed"));
    buf = weather_info_get_conditions (info);
    if (!strcmp (buf, "-"))
        buf = weather_info_get_sky (info);
    return g_strdup_printf ("%s: %s", weather_info_get_location_name (info), buf);
}

const gchar *
weather_info_get_icon_name (WeatherInfo *info)
{
    WeatherConditions cond;
    WeatherSky        sky;
    time_t            current_time;
    gboolean          daytime;
    gchar*            icon;
    static gchar      icon_buffer[32];
    WeatherMoonPhase  moonPhase;
    WeatherMoonLatitude moonLat;
    gint              phase;

    g_return_val_if_fail (info != NULL, NULL);

    if (!info->valid)
        return NULL;

    cond = info->cond;
    sky = info->sky;

    if (cond.significant) {
	if (cond.phenomenon != PHENOMENON_NONE &&
	    cond.qualifier == QUALIFIER_THUNDERSTORM)
            return "weather-storm";

        switch (cond.phenomenon) {
	case PHENOMENON_INVALID:
	case PHENOMENON_LAST:
	case PHENOMENON_NONE:
	    break;

	case PHENOMENON_DRIZZLE:
	case PHENOMENON_RAIN:
	case PHENOMENON_UNKNOWN_PRECIPITATION:
	case PHENOMENON_HAIL:
	case PHENOMENON_SMALL_HAIL:
	    return "weather-showers";

	case PHENOMENON_SNOW:
	case PHENOMENON_SNOW_GRAINS:
	case PHENOMENON_ICE_PELLETS:
	case PHENOMENON_ICE_CRYSTALS:
	    return "weather-snow";

	case PHENOMENON_TORNADO:
	case PHENOMENON_SQUALL:
	    return "weather-storm";

	case PHENOMENON_MIST:
	case PHENOMENON_FOG:
	case PHENOMENON_SMOKE:
	case PHENOMENON_VOLCANIC_ASH:
	case PHENOMENON_SAND:
	case PHENOMENON_HAZE:
	case PHENOMENON_SPRAY:
	case PHENOMENON_DUST:
	case PHENOMENON_SANDSTORM:
	case PHENOMENON_DUSTSTORM:
	case PHENOMENON_FUNNEL_CLOUD:
	case PHENOMENON_DUST_WHIRLS:
	    return "weather-fog";
        }
    }

    if (info->midnightSun ||
	(!info->sunriseValid && !info->sunsetValid))
	daytime = TRUE;
    else if (info->polarNight)
	daytime = FALSE;
    else {
	current_time = time (NULL);
	daytime =
	    ( !info->sunriseValid || (current_time >= info->sunrise) ) &&
	    ( !info->sunsetValid || (current_time < info->sunset) );
    }

    switch (sky) {
    case SKY_INVALID:
    case SKY_LAST:
    case SKY_CLEAR:
	if (daytime)
	    return "weather-clear";
	else {
	    icon = g_stpcpy(icon_buffer, "weather-clear-night");
	    break;
	}

    case SKY_BROKEN:
    case SKY_SCATTERED:
    case SKY_FEW:
	if (daytime)
	    return "weather-few-clouds";
	else {
	    icon = g_stpcpy(icon_buffer, "weather-few-clouds-night");
	    break;
	}

    case SKY_OVERCAST:
	return "weather-overcast";

    default: /* unrecognized */
	return NULL;
    }

    /*
     * A phase-of-moon icon is to be returned.
     * Determine which one based on the moon's location
     */
    if (info->moonValid && weather_info_get_value_moonphase(info, &moonPhase, &moonLat)) {
	phase = (gint)((moonPhase * MOON_PHASES / 360.) + 0.5);
	if (phase == MOON_PHASES) {
	    phase = 0;
	} else if (phase > 0 &&
		   (RADIANS_TO_DEGREES(weather_info_get_location(info)->latitude)
		    < moonLat)) {
	    /*
	     * Locations south of the moon's latitude will see the moon in the
	     * northern sky.  The moon waxes and wanes from left to right
	     * so we reference an icon running in the opposite direction.
	     */
	    phase = MOON_PHASES - phase;
	}

	/*
	 * If the moon is not full then append the angle to the icon string.
	 * Note that an icon by this name is not required to exist:
	 * the caller can use GTK_ICON_LOOKUP_GENERIC_FALLBACK to fall back to
	 * the full moon image.
	 */
	if ((0 == (MOON_PHASES & 0x1)) && (MOON_PHASES/2 != phase)) {
	    g_snprintf(icon, sizeof(icon_buffer) - strlen(icon_buffer),
		       "-%03d", phase * 360 / MOON_PHASES);
	}
    }
    return icon_buffer;
}

static gboolean
temperature_value (gdouble temp_f,
		   TempUnit to_unit,
		   gdouble *value,
		   TempUnit def_unit)
{
    gboolean ok = TRUE;

    *value = 0.0;
    if (temp_f < -500.0)
	return FALSE;

    if (to_unit == TEMP_UNIT_DEFAULT)
	    to_unit = def_unit;

    switch (to_unit) {
        case TEMP_UNIT_FAHRENHEIT:
	    *value = temp_f;
	    break;
        case TEMP_UNIT_CENTIGRADE:
	    *value = TEMP_F_TO_C (temp_f);
	    break;
        case TEMP_UNIT_KELVIN:
	    *value = TEMP_F_TO_K (temp_f);
	    break;
        case TEMP_UNIT_INVALID:
        case TEMP_UNIT_DEFAULT:
	default:
	    ok = FALSE;
	    break;
    }

    return ok;
}

static gboolean
speed_value (gdouble knots, SpeedUnit to_unit, gdouble *value, SpeedUnit def_unit)
{
    gboolean ok = TRUE;

    *value = -1.0;

    if (knots < 0.0)
	return FALSE;

    if (to_unit == SPEED_UNIT_DEFAULT)
	    to_unit = def_unit;

    switch (to_unit) {
        case SPEED_UNIT_KNOTS:
            *value = knots;
	    break;
        case SPEED_UNIT_MPH:
            *value = WINDSPEED_KNOTS_TO_MPH (knots);
	    break;
        case SPEED_UNIT_KPH:
            *value = WINDSPEED_KNOTS_TO_KPH (knots);
	    break;
        case SPEED_UNIT_MS:
            *value = WINDSPEED_KNOTS_TO_MS (knots);
	    break;
	case SPEED_UNIT_BFT:
	    *value = WINDSPEED_KNOTS_TO_BFT (knots);
	    break;
        case SPEED_UNIT_INVALID:
        case SPEED_UNIT_DEFAULT:
        default:
            ok = FALSE;
            break;
    }

    return ok;
}

static gboolean
pressure_value (gdouble inHg, PressureUnit to_unit, gdouble *value, PressureUnit def_unit)
{
    gboolean ok = TRUE;

    *value = -1.0;

    if (inHg < 0.0)
	return FALSE;

    if (to_unit == PRESSURE_UNIT_DEFAULT)
	    to_unit = def_unit;

    switch (to_unit) {
        case PRESSURE_UNIT_INCH_HG:
            *value = inHg;
	    break;
        case PRESSURE_UNIT_MM_HG:
            *value = PRESSURE_INCH_TO_MM (inHg);
	    break;
        case PRESSURE_UNIT_KPA:
            *value = PRESSURE_INCH_TO_KPA (inHg);
	    break;
        case PRESSURE_UNIT_HPA:
            *value = PRESSURE_INCH_TO_HPA (inHg);
	    break;
        case PRESSURE_UNIT_MB:
            *value = PRESSURE_INCH_TO_MB (inHg);
	    break;
        case PRESSURE_UNIT_ATM:
            *value = PRESSURE_INCH_TO_ATM (inHg);
	    break;
        case PRESSURE_UNIT_INVALID:
        case PRESSURE_UNIT_DEFAULT:
        default:
	    ok = FALSE;
	    break;
    }

    return ok;
}

static gboolean
distance_value (gdouble miles, DistanceUnit to_unit, gdouble *value, DistanceUnit def_unit)
{
    gboolean ok = TRUE;

    *value = -1.0;

    if (miles < 0.0)
	return FALSE;

    if (to_unit == DISTANCE_UNIT_DEFAULT)
	    to_unit = def_unit;

    switch (to_unit) {
        case DISTANCE_UNIT_MILES:
            *value = miles;
            break;
        case DISTANCE_UNIT_KM:
            *value = VISIBILITY_SM_TO_KM (miles);
            break;
        case DISTANCE_UNIT_METERS:
            *value = VISIBILITY_SM_TO_M (miles);
            break;
        case DISTANCE_UNIT_INVALID:
        case DISTANCE_UNIT_DEFAULT:
        default:
	    ok = FALSE;
	    break;
    }

    return ok;
}

gboolean
weather_info_get_value_sky (WeatherInfo *info, WeatherSky *sky)
{
    g_return_val_if_fail (info != NULL, FALSE);
    g_return_val_if_fail (sky != NULL, FALSE);

    if (!info->valid)
	return FALSE;

    if (info->sky <= SKY_INVALID || info->sky >= SKY_LAST)
	return FALSE;

    *sky = info->sky;

    return TRUE;
}

gboolean
weather_info_get_value_conditions (WeatherInfo *info, WeatherConditionPhenomenon *phenomenon, WeatherConditionQualifier *qualifier)
{
    g_return_val_if_fail (info != NULL, FALSE);
    g_return_val_if_fail (phenomenon != NULL, FALSE);
    g_return_val_if_fail (qualifier != NULL, FALSE);

    if (!info->valid)
	return FALSE;

    if (!info->cond.significant)
	return FALSE;

    if (!(info->cond.phenomenon > PHENOMENON_INVALID &&
	  info->cond.phenomenon < PHENOMENON_LAST &&
	  info->cond.qualifier > QUALIFIER_INVALID &&
	  info->cond.qualifier < QUALIFIER_LAST))
        return FALSE;

    *phenomenon = info->cond.phenomenon;
    *qualifier = info->cond.qualifier;

    return TRUE;
}

gboolean
weather_info_get_value_temp (WeatherInfo *info, TempUnit unit, gdouble *value)
{
    g_return_val_if_fail (info != NULL, FALSE);
    g_return_val_if_fail (value != NULL, FALSE);

    if (!info->valid)
	return FALSE;

    return temperature_value (info->temp, unit, value, info->temperature_unit);
}

gboolean
weather_info_get_value_temp_min (WeatherInfo *info, TempUnit unit, gdouble *value)
{
    g_return_val_if_fail (info != NULL, FALSE);
    g_return_val_if_fail (value != NULL, FALSE);

    if (!info->valid || !info->tempMinMaxValid)
	return FALSE;

    return temperature_value (info->temp_min, unit, value, info->temperature_unit);
}

gboolean
weather_info_get_value_temp_max (WeatherInfo *info, TempUnit unit, gdouble *value)
{
    g_return_val_if_fail (info != NULL, FALSE);
    g_return_val_if_fail (value != NULL, FALSE);

    if (!info->valid || !info->tempMinMaxValid)
	return FALSE;

    return temperature_value (info->temp_max, unit, value, info->temperature_unit);
}

gboolean
weather_info_get_value_dew (WeatherInfo *info, TempUnit unit, gdouble *value)
{
    g_return_val_if_fail (info != NULL, FALSE);
    g_return_val_if_fail (value != NULL, FALSE);

    if (!info->valid)
	return FALSE;

    return temperature_value (info->dew, unit, value, info->temperature_unit);
}

gboolean
weather_info_get_value_apparent (WeatherInfo *info, TempUnit unit, gdouble *value)
{
    g_return_val_if_fail (info != NULL, FALSE);
    g_return_val_if_fail (value != NULL, FALSE);

    if (!info->valid)
	return FALSE;

    return temperature_value (calc_apparent (info), unit, value, info->temperature_unit);
}

gboolean
weather_info_get_value_update (WeatherInfo *info, time_t *value)
{
    g_return_val_if_fail (info != NULL, FALSE);
    g_return_val_if_fail (value != NULL, FALSE);

    if (!info->valid)
	return FALSE;

    *value = info->update;

    return TRUE;
}

gboolean
weather_info_get_value_sunrise (WeatherInfo *info, time_t *value)
{
    g_return_val_if_fail (info != NULL, FALSE);
    g_return_val_if_fail (value != NULL, FALSE);

    if (!info->valid || !info->sunriseValid)
	return FALSE;

    *value = info->sunrise;

    return TRUE;
}

gboolean
weather_info_get_value_sunset (WeatherInfo *info, time_t *value)
{
    g_return_val_if_fail (info != NULL, FALSE);
    g_return_val_if_fail (value != NULL, FALSE);

    if (!info->valid || !info->sunsetValid)
	return FALSE;

    *value = info->sunset;

    return TRUE;
}

gboolean
weather_info_get_value_moonphase (WeatherInfo      *info,
				  WeatherMoonPhase *value,
				  WeatherMoonLatitude *lat)
{
    g_return_val_if_fail (info != NULL, FALSE);
    g_return_val_if_fail (value != NULL, FALSE);

    if (!info->valid || !info->moonValid)
	return FALSE;

    *value = info->moonphase;
    *lat   = info->moonlatitude;

    return TRUE;
}

gboolean
weather_info_get_value_wind (WeatherInfo *info, SpeedUnit unit, gdouble *speed, WeatherWindDirection *direction)
{
    gboolean res = FALSE;

    g_return_val_if_fail (info != NULL, FALSE);
    g_return_val_if_fail (speed != NULL, FALSE);
    g_return_val_if_fail (direction != NULL, FALSE);

    if (!info->valid)
	return FALSE;

    if (info->windspeed < 0.0 || info->wind <= WIND_INVALID || info->wind >= WIND_LAST)
        return FALSE;

    res = speed_value (info->windspeed, unit, speed, info->speed_unit);
    *direction = info->wind;

    return res;
}

gboolean
weather_info_get_value_pressure (WeatherInfo *info, PressureUnit unit, gdouble *value)
{
    g_return_val_if_fail (info != NULL, FALSE);
    g_return_val_if_fail (value != NULL, FALSE);

    if (!info->valid)
	return FALSE;

    return pressure_value (info->pressure, unit, value, info->pressure_unit);
}

gboolean
weather_info_get_value_visibility (WeatherInfo *info, DistanceUnit unit, gdouble *value)
{
    g_return_val_if_fail (info != NULL, FALSE);
    g_return_val_if_fail (value != NULL, FALSE);

    if (!info->valid)
	return FALSE;

    return distance_value (info->visibility, unit, value, info->distance_unit);
}

/**
 * weather_info_get_upcoming_moonphases:
 * @info:   WeatherInfo containing the time_t of interest
 * @phases: An array of four time_t values that will hold the returned values.
 *    The values are estimates of the time of the next new, quarter, full and
 *    three-quarter moons.
 *
 * Returns: gboolean indicating success or failure
 */
gboolean
weather_info_get_upcoming_moonphases (WeatherInfo *info, time_t *phases)
{
    g_return_val_if_fail (info != NULL, FALSE);
    g_return_val_if_fail (phases != NULL, FALSE);

    return calc_moon_phases(info, phases);
}

static void
_weather_internal_check (void)
{
    g_assert (G_N_ELEMENTS (wind_direction_str) == WIND_LAST);
    g_assert (G_N_ELEMENTS (sky_str) == SKY_LAST);
    g_assert (G_N_ELEMENTS (conditions_str) == PHENOMENON_LAST);
    g_assert (G_N_ELEMENTS (conditions_str[0]) == QUALIFIER_LAST);
}
