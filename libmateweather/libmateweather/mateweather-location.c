/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* mateweather-location.c - Location-handling code
 *
 * Copyright 2008, Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <math.h>
#include <locale.h>
#include <gtk/gtk.h>
#include <libxml/xmlreader.h>

#define MATEWEATHER_I_KNOW_THIS_IS_UNSTABLE
#include "mateweather-location.h"
#include "mateweather-timezone.h"
#include "parser.h"
#include "weather-priv.h"

/**
 * MateWeatherLocation:
 *
 * A #MateWeatherLocation represents a "location" of some type known to
 * libmateweather; anything from a single weather station to the entire
 * world. See #MateWeatherLocationLevel for information about how the
 * hierarchy of locations works. 
 **/

struct _MateWeatherLocation {
    char *name, *sort_name;
    MateWeatherLocation *parent, **children;
    MateWeatherLocationLevel level;
    char *country_code, *tz_hint;
    char *station_code, *forecast_zone, *radar;
    double latitude, longitude;
    gboolean latlon_valid;
    MateWeatherTimezone **zones;

    int ref_count;
};

/**
 * MateWeatherLocationLevel:
 * @MATEWEATHER_LOCATION_WORLD: A location representing the entire world.
 * @MATEWEATHER_LOCATION_REGION: A location representing a continent or
 * other top-level region.
 * @MATEWEATHER_LOCATION_COUNTRY: A location representing a "country" (or
 * other geographic unit that has an ISO-3166 country code)
 * @MATEWEATHER_LOCATION_ADM1: A location representing a "first-level
 * administrative division"; ie, a state, province, or similar
 * division.
 * @MATEWEATHER_LOCATION_ADM2: A location representing a subdivision of a
 * %MATEWEATHER_LOCATION_ADM1 location. (Not currently used.)
 * @MATEWEATHER_LOCATION_CITY: A location representing a city
 * @MATEWEATHER_LOCATION_WEATHER_STATION: A location representing a
 * weather station.
 *
 * The size/scope of a particular #MateWeatherLocation.
 *
 * Locations form a hierarchy, with a %MATEWEATHER_LOCATION_WORLD
 * location at the top, divided into regions or countries, and so on.
 * Countries may or may not be divided into "adm1"s, and "adm1"s may
 * or may not be divided into "adm2"s. A city will have at least one,
 * and possibly several, weather stations inside it. Weather stations
 * will never appear outside of cities.
 **/

static int
sort_locations_by_name (gconstpointer a, gconstpointer b)
{
    MateWeatherLocation *loc_a = *(MateWeatherLocation **)a;
    MateWeatherLocation *loc_b = *(MateWeatherLocation **)b;

    return g_utf8_collate (loc_a->sort_name, loc_b->sort_name);
}
 
static int
sort_locations_by_distance (gconstpointer a, gconstpointer b, gpointer user_data)
{
    MateWeatherLocation *loc_a = *(MateWeatherLocation **)a;
    MateWeatherLocation *loc_b = *(MateWeatherLocation **)b;
    MateWeatherLocation *city = (MateWeatherLocation *)user_data;
    double dist_a, dist_b;

    dist_a = mateweather_location_get_distance (loc_a, city);
    dist_b = mateweather_location_get_distance (loc_b, city);
    if (dist_a < dist_b)
	return -1;
    else if (dist_a > dist_b)
	return 1;
    else
	return 0;
}

static gboolean
parse_coordinates (const char *coordinates,
		   double *latitude, double *longitude)
{
    char *p;

    *latitude = g_ascii_strtod (coordinates, &p) * M_PI / 180.0;
    if (p == (char *)coordinates)
	return FALSE;
    if (*p++ != ' ')
	return FALSE;
    *longitude = g_ascii_strtod (p, &p) * M_PI / 180.0;
    return !*p;
}

static char *
unparse_coordinates (double latitude, double longitude)
{
    int lat_d, lat_m, lat_s, lon_d, lon_m, lon_s;
    char lat_dir, lon_dir;

    latitude = latitude * 180.0 / M_PI;
    longitude = longitude * 180.0 / M_PI;

    if (latitude < 0.0) {
	lat_dir = 'S';
	latitude = -latitude;
    } else
	lat_dir = 'N';
    if (longitude < 0.0) {
	lon_dir = 'W';
	longitude = -longitude;
    } else
	lon_dir = 'E';

    lat_d = (int)latitude;
    lat_m = (int)(latitude * 60.0) - lat_d * 60;
    lat_s = (int)(latitude * 3600.0) - lat_d * 3600 - lat_m * 60;
    lon_d = (int)longitude;
    lon_m = (int)(longitude * 60.0) - lon_d * 60;
    lon_s = (int)(longitude * 3600.0) - lon_d * 3600 - lon_m * 60;

    return g_strdup_printf ("%02d-%02d-%02d%c %03d-%02d-%02d%c",
			    lat_d, lat_m, lat_s, lat_dir,
			    lon_d, lon_m, lon_s, lon_dir);
}

static MateWeatherLocation *
location_new_from_xml (MateWeatherParser *parser, MateWeatherLocationLevel level,
		       MateWeatherLocation *parent)
{
    MateWeatherLocation *loc, *child;
    GPtrArray *children = NULL;
    const char *tagname;
    char *value, *normalized;
    int tagtype, i;

    loc = g_slice_new0 (MateWeatherLocation);
    loc->parent = parent;
    loc->level = level;
    loc->ref_count = 1;
    children = g_ptr_array_new ();

    if (xmlTextReaderRead (parser->xml) != 1)
	goto error_out;
    while ((tagtype = xmlTextReaderNodeType (parser->xml)) !=
	   XML_READER_TYPE_END_ELEMENT) {
	if (tagtype != XML_READER_TYPE_ELEMENT) {
	    if (xmlTextReaderRead (parser->xml) != 1)
		goto error_out;
	    continue;
	}

	tagname = (const char *) xmlTextReaderConstName (parser->xml);
	if (!strcmp (tagname, "name") && !loc->name) {
	    value = mateweather_parser_get_localized_value (parser);
	    if (!value)
		goto error_out;
	    loc->name = g_strdup (value);
	    xmlFree (value);
	    normalized = g_utf8_normalize (loc->name, -1, G_NORMALIZE_ALL);
	    loc->sort_name = g_utf8_casefold (normalized, -1);
	    g_free (normalized);

	} else if (!strcmp (tagname, "iso-code") && !loc->country_code) {
	    value = mateweather_parser_get_value (parser);
	    if (!value)
		goto error_out;
	    loc->country_code = g_strdup (value);
	    xmlFree (value);
	} else if (!strcmp (tagname, "tz-hint") && !loc->tz_hint) {
	    value = mateweather_parser_get_value (parser);
	    if (!value)
		goto error_out;
	    loc->tz_hint = g_strdup (value);
	    xmlFree (value);
	} else if (!strcmp (tagname, "code") && !loc->station_code) {
	    value = mateweather_parser_get_value (parser);
	    if (!value)
		goto error_out;
	    loc->station_code = g_strdup (value);
	    xmlFree (value);
	} else if (!strcmp (tagname, "coordinates") && !loc->latlon_valid) {
	    value = mateweather_parser_get_value (parser);
	    if (!value)
		goto error_out;
	    if (parse_coordinates (value, &loc->latitude, &loc->longitude))
		loc->latlon_valid = TRUE;
	    xmlFree (value);
	} else if (!strcmp (tagname, "zone") && !loc->forecast_zone) {
	    value = mateweather_parser_get_value (parser);
	    if (!value)
		goto error_out;
	    loc->forecast_zone = g_strdup (value);
	    xmlFree (value);
	} else if (!strcmp (tagname, "radar") && !loc->radar) {
	    value = mateweather_parser_get_value (parser);
	    if (!value)
		goto error_out;
	    loc->radar = g_strdup (value);
	    xmlFree (value);

	} else if (!strcmp (tagname, "region")) {
	    child = location_new_from_xml (parser, MATEWEATHER_LOCATION_REGION, loc);
	    if (!child)
		goto error_out;
	    if (parser->use_regions)
		g_ptr_array_add (children, child);
	    else {
		if (child->children) {
		    for (i = 0; child->children[i]; i++)
			g_ptr_array_add (children, mateweather_location_ref (child->children[i]));
		}
		mateweather_location_unref (child);
	    }
	} else if (!strcmp (tagname, "country")) {
	    child = location_new_from_xml (parser, MATEWEATHER_LOCATION_COUNTRY, loc);
	    if (!child)
		goto error_out;
	    g_ptr_array_add (children, child);
	} else if (!strcmp (tagname, "state")) {
	    child = location_new_from_xml (parser, MATEWEATHER_LOCATION_ADM1, loc);
	    if (!child)
		goto error_out;
	    g_ptr_array_add (children, child);
	} else if (!strcmp (tagname, "city")) {
	    child = location_new_from_xml (parser, MATEWEATHER_LOCATION_CITY, loc);
	    if (!child)
		goto error_out;
	    g_ptr_array_add (children, child);
	} else if (!strcmp (tagname, "location")) {
	    child = location_new_from_xml (parser, MATEWEATHER_LOCATION_WEATHER_STATION, loc);
	    if (!child)
		goto error_out;
	    g_ptr_array_add (children, child);

	} else if (!strcmp (tagname, "timezones")) {
	    loc->zones = mateweather_timezones_parse_xml (parser);
	    if (!loc->zones)
		goto error_out;

	} else {
	    if (xmlTextReaderNext (parser->xml) != 1)
		goto error_out;
	}
    }
    if (xmlTextReaderRead (parser->xml) != 1 && parent)
	goto error_out;

    if (children->len) {
	if (level == MATEWEATHER_LOCATION_CITY)
	    g_ptr_array_sort_with_data (children, sort_locations_by_distance, loc);
	else
	    g_ptr_array_sort (children, sort_locations_by_name);

	g_ptr_array_add (children, NULL);
	loc->children = (MateWeatherLocation **)g_ptr_array_free (children, FALSE);
    } else
	g_ptr_array_free (children, TRUE);

    return loc;

error_out:
    mateweather_location_unref (loc);
    for (i = 0; i < children->len; i++)
	mateweather_location_unref (children->pdata[i]);
    g_ptr_array_free (children, TRUE);

    return NULL;
}

/**
 * mateweather_location_new_world:
 * @use_regions: whether or not to divide the world into regions
 *
 * Creates a new #MateWeatherLocation of type %MATEWEATHER_LOCATION_WORLD,
 * representing a hierarchy containing all of the locations from
 * Locations.xml.
 *
 * If @use_regions is %TRUE, the immediate children of the returned
 * location will be %MATEWEATHER_LOCATION_REGION nodes, representing the
 * top-level "regions" of Locations.xml (the continents and a few
 * other divisions), and the country-level nodes will be the children
 * of the regions. If @use_regions is %FALSE, the regions will be
 * skipped, and the children of the returned location will be the
 * %MATEWEATHER_LOCATION_COUNTRY nodes.
 *
 * Return value: (allow-none): a %MATEWEATHER_LOCATION_WORLD location, or
 * %NULL if Locations.xml could not be found or could not be parsed.
 **/
MateWeatherLocation *
mateweather_location_new_world (gboolean use_regions)
{
    MateWeatherParser *parser;
    MateWeatherLocation *world;

    parser = mateweather_parser_new (use_regions);
    if (!parser)
	return NULL;

    world = location_new_from_xml (parser, MATEWEATHER_LOCATION_WORLD, NULL);

    mateweather_parser_free (parser);
    return world;
}

/**
 * mateweather_location_ref:
 * @loc: a #MateWeatherLocation
 *
 * Adds 1 to @loc's reference count.
 *
 * Return value: @loc
 **/
MateWeatherLocation *
mateweather_location_ref (MateWeatherLocation *loc)
{
    g_return_val_if_fail (loc != NULL, NULL);

    loc->ref_count++;
    return loc;
}

/**
 * mateweather_location_unref:
 * @loc: a #MateWeatherLocation
 *
 * Subtracts 1 from @loc's reference count, and frees it if the
 * reference count reaches 0.
 **/
void
mateweather_location_unref (MateWeatherLocation *loc)
{
    int i;

    g_return_if_fail (loc != NULL);

    if (--loc->ref_count)
	return;
    
    g_free (loc->name);
    g_free (loc->sort_name);
    g_free (loc->country_code);
    g_free (loc->tz_hint);
    g_free (loc->station_code);
    g_free (loc->forecast_zone);
    g_free (loc->radar);

    if (loc->children) {
	for (i = 0; loc->children[i]; i++) {
	    loc->children[i]->parent = NULL;
	    mateweather_location_unref (loc->children[i]);
	}
	g_free (loc->children);
    }

    if (loc->zones) {
	for (i = 0; loc->zones[i]; i++)
	    mateweather_timezone_unref (loc->zones[i]);
	g_free (loc->zones);
    }

    g_slice_free (MateWeatherLocation, loc);
}

GType
mateweather_location_get_type (void)
{
    static volatile gsize type_volatile = 0;

    if (g_once_init_enter (&type_volatile)) {
	GType type = g_boxed_type_register_static (
	    g_intern_static_string ("MateWeatherLocation"),
	    (GBoxedCopyFunc) mateweather_location_ref,
	    (GBoxedFreeFunc) mateweather_location_unref);
	g_once_init_leave (&type_volatile, type);
    }
    return type_volatile;
}

/**
 * mateweather_location_get_name:
 * @loc: a #MateWeatherLocation
 *
 * Gets @loc's name, localized into the current language.
 *
 * Note that %MATEWEATHER_LOCATION_WEATHER_STATION nodes are not
 * localized, and so the name returned for those nodes will always be
 * in English, and should therefore not be displayed to the user.
 * (FIXME: should we just not return a name?)
 *
 * Return value: @loc's name
 **/
const char *
mateweather_location_get_name (MateWeatherLocation *loc)
{
    g_return_val_if_fail (loc != NULL, NULL);
    return loc->name;
}

/**
 * mateweather_location_get_sort_name:
 * @loc: a #MateWeatherLocation
 *
 * Gets @loc's "sort name", which is the name after having
 * g_utf8_normalize() (with %G_NORMALIZE_ALL) and g_utf8_casefold()
 * called on it. You can use this to sort locations, or to comparing
 * user input against a location name.
 *
 * Return value: @loc's sort name
 **/
const char *
mateweather_location_get_sort_name (MateWeatherLocation *loc)
{
    g_return_val_if_fail (loc != NULL, NULL);
    return loc->sort_name;
}

/**
 * mateweather_location_get_level:
 * @loc: a #MateWeatherLocation
 *
 * Gets @loc's level, from %MATEWEATHER_LOCATION_WORLD, to
 * %MATEWEATHER_LOCATION_WEATHER_STATION.
 *
 * Return value: @loc's level
 **/
MateWeatherLocationLevel
mateweather_location_get_level (MateWeatherLocation *loc)
{
    g_return_val_if_fail (loc != NULL, MATEWEATHER_LOCATION_WORLD);
    return loc->level;
}

/**
 * mateweather_location_get_parent:
 * @loc: a #MateWeatherLocation
 *
 * Gets @loc's parent location.
 *
 * Return value: (transfer none) (allow-none): @loc's parent, or %NULL
 * if @loc is a %MATEWEATHER_LOCATION_WORLD node.
 **/
MateWeatherLocation *
mateweather_location_get_parent (MateWeatherLocation *loc)
{
    g_return_val_if_fail (loc != NULL, NULL);
    return loc->parent;
}

/**
 * mateweather_location_get_children:
 * @loc: a #MateWeatherLocation
 *
 * Gets an array of @loc's children; this is owned by @loc and will
 * not remain valid if @loc is freed.
 *
 * Return value: (transfer none) (array zero-terminated=1): @loc's
 * children. (May be empty, but will not be %NULL.)
 **/
MateWeatherLocation **
mateweather_location_get_children (MateWeatherLocation *loc)
{
    static MateWeatherLocation *no_children = NULL;

    g_return_val_if_fail (loc != NULL, NULL);

    if (loc->children)
	return loc->children;
    else
	return &no_children;
}


/**
 * mateweather_location_free_children:
 * @loc: a #MateWeatherLocation
 * @children: an array of @loc's children
 *
 * This is a no-op. Do not use it.
 *
 * Deprecated: This is a no-op.
 **/
void
mateweather_location_free_children (MateWeatherLocation  *loc,
				 MateWeatherLocation **children)
{
    ;
}

/**
 * mateweather_location_has_coords:
 * @loc: a #MateWeatherLocation
 *
 * Checks if @loc has valid latitude and longitude.
 *
 * Return value: %TRUE if @loc has valid latitude and longitude.
 **/
gboolean
mateweather_location_has_coords (MateWeatherLocation *loc)
{
    g_return_val_if_fail (loc != NULL, FALSE);
    return loc->latlon_valid;
}

/**
 * mateweather_location_get_coords:
 * @loc: a #MateWeatherLocation
 * @latitude: (out): on return will contain @loc's latitude
 * @longitude: (out): on return will contain @loc's longitude
 *
 * Gets @loc's coordinates; you must check
 * mateweather_location_has_coords() before calling this.
 **/
void
mateweather_location_get_coords (MateWeatherLocation *loc,
			      double *latitude, double *longitude)
{
    //g_return_if_fail (loc->latlon_valid);
    g_return_if_fail (loc != NULL);
    g_return_if_fail (latitude != NULL);
    g_return_if_fail (longitude != NULL);

    *latitude = loc->latitude / M_PI * 180.0;
    *longitude = loc->longitude / M_PI * 180.0;
}

/**
 * mateweather_location_get_distance:
 * @loc: a #MateWeatherLocation
 * @loc2: a second #MateWeatherLocation
 *
 * Determines the distance in kilometers between @loc and @loc2.
 *
 * Return value: the distance between @loc and @loc2.
 **/
double
mateweather_location_get_distance (MateWeatherLocation *loc, MateWeatherLocation *loc2)
{
    /* average radius of the earth in km */
    static const double radius = 6372.795;

    g_return_val_if_fail (loc != NULL, 0);
    g_return_val_if_fail (loc2 != NULL, 0);

    //g_return_val_if_fail (loc->latlon_valid, 0.0);
    //g_return_val_if_fail (loc2->latlon_valid, 0.0);

    return acos (cos (loc->latitude) * cos (loc2->latitude) * cos (loc->longitude - loc2->longitude) +
		 sin (loc->latitude) * sin (loc2->latitude)) * radius;
}

/**
 * mateweather_location_get_country:
 * @loc: a #MateWeatherLocation
 *
 * Gets the ISO 3166 country code of @loc (or %NULL if @loc is a
 * region- or world-level location)
 *
 * Return value: (allow-none): @loc's country code (or %NULL if @loc
 * is a region- or world-level location)
 **/
const char *
mateweather_location_get_country (MateWeatherLocation *loc)
{
    g_return_val_if_fail (loc != NULL, NULL);

    while (loc->parent && !loc->country_code)
	loc = loc->parent;
    return loc->country_code;
}

/**
 * mateweather_location_get_timezone:
 * @loc: a #MateWeatherLocation
 *
 * Gets the timezone associated with @loc, if known.
 *
 * The timezone is owned either by @loc or by one of its parents.
 * FIXME.
 *
 * Return value: (transfer none) (allow-none): @loc's timezone, or
 * %NULL
 **/
MateWeatherTimezone *
mateweather_location_get_timezone (MateWeatherLocation *loc)
{
    const char *tz_hint;
    int i;

    g_return_val_if_fail (loc != NULL, NULL);

    while (loc && !loc->tz_hint)
	loc = loc->parent;
    if (!loc)
	return NULL;
    tz_hint = loc->tz_hint;

    while (loc) {
	while (loc && !loc->zones)
	    loc = loc->parent;
	if (!loc)
	    return NULL;
	for (i = 0; loc->zones[i]; i++) {
	    if (!strcmp (tz_hint, mateweather_timezone_get_tzid (loc->zones[i])))
		return loc->zones[i];
	}
	loc = loc->parent;
    }

    return NULL;
}

static void
add_timezones (MateWeatherLocation *loc, GPtrArray *zones)
{
    int i;

    if (loc->zones) {
	for (i = 0; loc->zones[i]; i++)
	    g_ptr_array_add (zones, mateweather_timezone_ref (loc->zones[i]));
    }
    if (loc->level < MATEWEATHER_LOCATION_COUNTRY && loc->children) {
	for (i = 0; loc->children[i]; i++)
	    add_timezones (loc->children[i], zones);
    }
}

/**
 * mateweather_location_get_timezones:
 * @loc: a #MateWeatherLocation
 *
 * Gets an array of all timezones associated with any location under
 * @loc. You can use mateweather_location_free_timezones() to free this
 * array.
 *
 * Return value: (transfer full) (array zero-terminated=1): an array
 * of timezones. May be empty but will not be %NULL.
 **/
MateWeatherTimezone **
mateweather_location_get_timezones (MateWeatherLocation *loc)
{
    GPtrArray *zones;

    g_return_val_if_fail (loc != NULL, NULL);

    zones = g_ptr_array_new ();
    add_timezones (loc, zones);
    g_ptr_array_add (zones, NULL);
    return (MateWeatherTimezone **)g_ptr_array_free (zones, FALSE);
}

/**
 * mateweather_location_free_timezones:
 * @loc: a #MateWeatherLocation
 * @zones: an array returned from mateweather_location_get_timezones()
 *
 * Frees the array of timezones returned by
 * mateweather_location_get_timezones().
 **/
void
mateweather_location_free_timezones (MateWeatherLocation  *loc,
				  MateWeatherTimezone **zones)
{
    int i;

    g_return_if_fail (loc != NULL);
    g_return_if_fail (zones != NULL);

    for (i = 0; zones[i]; i++)
	mateweather_timezone_unref (zones[i]);
    g_free (zones);
}

/**
 * mateweather_location_get_code:
 * @loc: a #MateWeatherLocation
 *
 * Gets the METAR station code associated with a
 * %MATEWEATHER_LOCATION_WEATHER_STATION location.
 *
 * Return value: (allow-none): @loc's METAR station code, or %NULL
 **/
const char *
mateweather_location_get_code (MateWeatherLocation *loc)
{
    g_return_val_if_fail (loc != NULL, NULL);
    return loc->station_code;
}

/**
 * mateweather_location_get_city_name:
 * @loc: a #MateWeatherLocation
 *
 * For a %MATEWEATHER_LOCATION_CITY location, this is equivalent to
 * mateweather_location_get_name(). For a
 * %MATEWEATHER_LOCATION_WEATHER_STATION location, it is equivalent to
 * calling mateweather_location_get_name() on the location's parent. For
 * other locations it will return %NULL.
 *
 * Return value: (allow-none) @loc's city name, or %NULL
 **/
char *
mateweather_location_get_city_name (MateWeatherLocation *loc)
{
    g_return_val_if_fail (loc != NULL, NULL);

    if (loc->level == MATEWEATHER_LOCATION_CITY)
	return g_strdup (loc->name);
    else if (loc->level == MATEWEATHER_LOCATION_WEATHER_STATION &&
	     loc->parent &&
	     loc->parent->level == MATEWEATHER_LOCATION_CITY)
	return g_strdup (loc->parent->name);
    else
	return NULL;
}

WeatherLocation *
mateweather_location_to_weather_location (MateWeatherLocation *gloc,
				       const char *name)
{
    const char *code = NULL, *zone = NULL, *radar = NULL, *tz_hint = NULL;
    MateWeatherLocation *l;
    WeatherLocation *wloc;
    char *coords;

    g_return_val_if_fail (gloc != NULL, NULL);

    if (!name)
	name = mateweather_location_get_name (gloc);

    if (gloc->level == MATEWEATHER_LOCATION_CITY && gloc->children)
	l = gloc->children[0];
    else
	l = gloc;

    if (l->latlon_valid)
	coords = unparse_coordinates (l->latitude, l->longitude);
    else
	coords = NULL;

    while (l && (!code || !zone || !radar || !tz_hint)) {
	if (!code && l->station_code)
	    code = l->station_code;
	if (!zone && l->forecast_zone)
	    zone = l->forecast_zone;
	if (!radar && l->radar)
	    radar = l->radar;
	if (!tz_hint && l->tz_hint)
	    tz_hint = l->tz_hint;
	l = l->parent;
    }

    wloc = weather_location_new (name, code, zone, radar, coords,
				 mateweather_location_get_country (gloc),
				 tz_hint);
    g_free (coords);
    return wloc;
}

/**
 * mateweather_location_get_weather:
 * @loc: a %MateWeatherLocation
 *
 * Creates a #WeatherInfo corresponding to @loc; you can use
 * weather_info_update() to fill it in.
 *
 * Return value: (transfer full): a #WeatherInfo corresponding to
 * @loc.
 **/
WeatherInfo *
mateweather_location_get_weather (MateWeatherLocation *loc)
{
    WeatherLocation *wloc;
    WeatherInfo *info;

    g_return_val_if_fail (loc != NULL, NULL);

    wloc = mateweather_location_to_weather_location (loc, NULL);
    info = weather_info_new (wloc, NULL, NULL, NULL);
    weather_location_free (wloc);
    return info;
}
