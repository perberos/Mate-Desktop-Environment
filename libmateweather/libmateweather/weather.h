/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* weather.h - Public header for weather server functions.
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

#ifndef __WEATHER_H_
#define __WEATHER_H_


#ifndef MATEWEATHER_I_KNOW_THIS_IS_UNSTABLE
#error "libmateweather should only be used if you understand that it's subject to change, and is not supported as a fixed API/ABI or as part of the platform"
#endif


#include <gdk-pixbuf/gdk-pixbuf.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Location
 */

struct _WeatherLocation {
    gchar *name;
    gchar *code;
    gchar *zone;
    gchar *radar;
    gboolean zone_valid;
    gchar *coordinates;
    gdouble  latitude;
    gdouble  longitude;
    gboolean latlon_valid;
    gchar *country_code;
    gchar *tz_hint;
};

typedef struct _WeatherLocation WeatherLocation;

WeatherLocation *	weather_location_new 	(const gchar *trans_name,
						 const gchar *code,
						 const gchar *zone,
						 const gchar *radar,
						 const gchar *coordinates,
						 const gchar *country_code,
						 const gchar *tz_hint);
WeatherLocation *	weather_location_clone	(const WeatherLocation *location);
void			weather_location_free	(WeatherLocation *location);
gboolean		weather_location_equal	(const WeatherLocation *location1,
						 const WeatherLocation *location2);

/*
 * Weather prefs
 */

typedef enum _WeatherForecastType {
    FORECAST_STATE,
    FORECAST_ZONE,
    FORECAST_LIST
} WeatherForecastType;

typedef enum {
    TEMP_UNIT_INVALID = 0,
    TEMP_UNIT_DEFAULT,
    TEMP_UNIT_KELVIN,
    TEMP_UNIT_CENTIGRADE,
    TEMP_UNIT_FAHRENHEIT
} TempUnit;

typedef enum {
    SPEED_UNIT_INVALID = 0,
    SPEED_UNIT_DEFAULT,
    SPEED_UNIT_MS,    /* metres per second */
    SPEED_UNIT_KPH,   /* kilometres per hour */
    SPEED_UNIT_MPH,   /* miles per hour */
    SPEED_UNIT_KNOTS, /* Knots */
    SPEED_UNIT_BFT    /* Beaufort scale */
} SpeedUnit;

typedef enum {
    PRESSURE_UNIT_INVALID = 0,
    PRESSURE_UNIT_DEFAULT,
    PRESSURE_UNIT_KPA,    /* kiloPascal */
    PRESSURE_UNIT_HPA,    /* hectoPascal */
    PRESSURE_UNIT_MB,     /* 1 millibars = 1 hectoPascal */
    PRESSURE_UNIT_MM_HG,  /* millimeters of mecury */
    PRESSURE_UNIT_INCH_HG, /* inches of mercury */
    PRESSURE_UNIT_ATM     /* atmosphere */
} PressureUnit;

typedef enum {
    DISTANCE_UNIT_INVALID = 0,
    DISTANCE_UNIT_DEFAULT,
    DISTANCE_UNIT_METERS,
    DISTANCE_UNIT_KM,
    DISTANCE_UNIT_MILES
} DistanceUnit;

struct _WeatherPrefs {
    WeatherForecastType type;

    gboolean radar;
    const char *radar_custom_url;

    TempUnit temperature_unit;
    SpeedUnit speed_unit;
    PressureUnit pressure_unit;
    DistanceUnit distance_unit;
};

typedef struct _WeatherPrefs WeatherPrefs;

/*
 * Weather Info
 */

typedef struct _WeatherInfo WeatherInfo;

typedef void (*WeatherInfoFunc) (WeatherInfo *info, gpointer data);

WeatherInfo *	_weather_info_fill			(WeatherInfo *info,
							 WeatherLocation *location,
							 const WeatherPrefs *prefs,
							 WeatherInfoFunc cb,
							 gpointer data);
#define	weather_info_new(location, prefs, cb, data) _weather_info_fill (NULL, (location), (prefs), (cb), (data))
#define	weather_info_update(info, prefs, cb, data) _weather_info_fill ((info), NULL, (prefs), (cb), (data))

void			weather_info_abort		(WeatherInfo *info);
WeatherInfo *		weather_info_clone		(const WeatherInfo *info);
void			weather_info_free		(WeatherInfo *info);

gboolean		weather_info_is_valid		(WeatherInfo *info);
gboolean		weather_info_network_error	(WeatherInfo *info);

void			weather_info_to_metric		(WeatherInfo *info);
void			weather_info_to_imperial	(WeatherInfo *info);

const WeatherLocation *	weather_info_get_location	(WeatherInfo *info);
const gchar *		weather_info_get_location_name	(WeatherInfo *info);
const gchar *		weather_info_get_update		(WeatherInfo *info);
const gchar *		weather_info_get_sky		(WeatherInfo *info);
const gchar *		weather_info_get_conditions	(WeatherInfo *info);
const gchar *		weather_info_get_temp		(WeatherInfo *info);
const gchar *		weather_info_get_temp_min	(WeatherInfo *info);
const gchar *		weather_info_get_temp_max	(WeatherInfo *info);
const gchar *		weather_info_get_dew		(WeatherInfo *info);
const gchar *		weather_info_get_humidity	(WeatherInfo *info);
const gchar *		weather_info_get_wind		(WeatherInfo *info);
const gchar *		weather_info_get_pressure	(WeatherInfo *info);
const gchar *		weather_info_get_visibility	(WeatherInfo *info);
const gchar *		weather_info_get_apparent	(WeatherInfo *info);
const gchar *		weather_info_get_sunrise	(WeatherInfo *info);
const gchar *		weather_info_get_sunset		(WeatherInfo *info);
const gchar *		weather_info_get_forecast	(WeatherInfo *info);
GSList *		weather_info_get_forecast_list	(WeatherInfo *info);
GdkPixbufAnimation *	weather_info_get_radar		(WeatherInfo *info);

const gchar *		weather_info_get_temp_summary	(WeatherInfo *info);
gchar *			weather_info_get_weather_summary(WeatherInfo *info);

const gchar *		weather_info_get_icon_name	(WeatherInfo *info);
gint			weather_info_next_sun_event	(WeatherInfo *info);

/* values retrieving functions */

enum _WeatherWindDirection {
    WIND_INVALID = -1,
    WIND_VARIABLE,
    WIND_N, WIND_NNE, WIND_NE, WIND_ENE,
    WIND_E, WIND_ESE, WIND_SE, WIND_SSE,
    WIND_S, WIND_SSW, WIND_SW, WIND_WSW,
    WIND_W, WIND_WNW, WIND_NW, WIND_NNW,
    WIND_LAST
};

typedef enum _WeatherWindDirection WeatherWindDirection;

enum _WeatherSky {
    SKY_INVALID = -1,
    SKY_CLEAR,
    SKY_BROKEN,
    SKY_SCATTERED,
    SKY_FEW,
    SKY_OVERCAST,
    SKY_LAST
};

typedef enum _WeatherSky WeatherSky;

enum _WeatherConditionPhenomenon {
    PHENOMENON_INVALID = -1,

    PHENOMENON_NONE,

    PHENOMENON_DRIZZLE,
    PHENOMENON_RAIN,
    PHENOMENON_SNOW,
    PHENOMENON_SNOW_GRAINS,
    PHENOMENON_ICE_CRYSTALS,
    PHENOMENON_ICE_PELLETS,
    PHENOMENON_HAIL,
    PHENOMENON_SMALL_HAIL,
    PHENOMENON_UNKNOWN_PRECIPITATION,

    PHENOMENON_MIST,
    PHENOMENON_FOG,
    PHENOMENON_SMOKE,
    PHENOMENON_VOLCANIC_ASH,
    PHENOMENON_SAND,
    PHENOMENON_HAZE,
    PHENOMENON_SPRAY,
    PHENOMENON_DUST,

    PHENOMENON_SQUALL,
    PHENOMENON_SANDSTORM,
    PHENOMENON_DUSTSTORM,
    PHENOMENON_FUNNEL_CLOUD,
    PHENOMENON_TORNADO,
    PHENOMENON_DUST_WHIRLS,

    PHENOMENON_LAST
};

typedef enum _WeatherConditionPhenomenon WeatherConditionPhenomenon;

enum _WeatherConditionQualifier {
    QUALIFIER_INVALID = -1,

    QUALIFIER_NONE,

    QUALIFIER_VICINITY,

    QUALIFIER_LIGHT,
    QUALIFIER_MODERATE,
    QUALIFIER_HEAVY,
    QUALIFIER_SHALLOW,
    QUALIFIER_PATCHES,
    QUALIFIER_PARTIAL,
    QUALIFIER_THUNDERSTORM,
    QUALIFIER_BLOWING,
    QUALIFIER_SHOWERS,
    QUALIFIER_DRIFTING,
    QUALIFIER_FREEZING,

    QUALIFIER_LAST
};

typedef enum _WeatherConditionQualifier WeatherConditionQualifier;
typedef gdouble WeatherMoonPhase;
typedef gdouble WeatherMoonLatitude;

gboolean weather_info_get_value_update		(WeatherInfo *info, time_t *value);
gboolean weather_info_get_value_sky		(WeatherInfo *info, WeatherSky *sky);
gboolean weather_info_get_value_conditions	(WeatherInfo *info, WeatherConditionPhenomenon *phenomenon, WeatherConditionQualifier *qualifier);
gboolean weather_info_get_value_temp		(WeatherInfo *info, TempUnit unit, gdouble *value);
gboolean weather_info_get_value_temp_min	(WeatherInfo *info, TempUnit unit, gdouble *value);
gboolean weather_info_get_value_temp_max	(WeatherInfo *info, TempUnit unit, gdouble *value);
gboolean weather_info_get_value_dew		(WeatherInfo *info, TempUnit unit, gdouble *value);
gboolean weather_info_get_value_apparent	(WeatherInfo *info, TempUnit unit, gdouble *value);
gboolean weather_info_get_value_wind		(WeatherInfo *info, SpeedUnit unit, gdouble *speed, WeatherWindDirection *direction);
gboolean weather_info_get_value_pressure	(WeatherInfo *info, PressureUnit unit, gdouble *value);
gboolean weather_info_get_value_visibility	(WeatherInfo *info, DistanceUnit unit, gdouble *value);
gboolean weather_info_get_value_sunrise		(WeatherInfo *info, time_t *value);
gboolean weather_info_get_value_sunset 		(WeatherInfo *info, time_t *value);
gboolean weather_info_get_value_moonphase       (WeatherInfo *info, WeatherMoonPhase *value, WeatherMoonLatitude *lat);
gboolean weather_info_get_upcoming_moonphases   (WeatherInfo *info, time_t *phases);


#ifdef __cplusplus
}
#endif

#endif /* __WEATHER_H_ */
