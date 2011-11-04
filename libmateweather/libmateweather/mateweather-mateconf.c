/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 * mateweather-mateconf.c: MateConf interaction methods for mateweather.
 *
 * Copyright (C) 2005 Philip Langdale, Papadimitriou Spiros
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *     Philip Langdale <philipl@mail.utexas.edu>
 *     Papadimitriou Spiros <spapadim+@cs.cmu.edu>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#define MATEWEATHER_I_KNOW_THIS_IS_UNSTABLE
#include "mateweather-mateconf.h"
#include "weather-priv.h"

struct _MateWeatherMateConf
{
    MateConfClient *mateconf;
    char *prefix;
};


MateWeatherMateConf *
mateweather_mateconf_new (const char *prefix)
{
    MateWeatherMateConf *ctx = g_new0 (MateWeatherMateConf, 1);
    ctx->mateconf = mateconf_client_get_default ();
    ctx->prefix = g_strdup (prefix);

    return ctx;
}


void
mateweather_mateconf_free (MateWeatherMateConf *ctx)
{
    if (!ctx)
        return;

    g_object_unref (ctx->mateconf);
    g_free (ctx->prefix);
    g_free (ctx);
}


MateConfClient *
mateweather_mateconf_get_client (MateWeatherMateConf *ctx)
{
    g_return_val_if_fail (ctx != NULL, NULL);
    return ctx->mateconf;
}


gchar *
mateweather_mateconf_get_full_key (MateWeatherMateConf *ctx,
			     const gchar   *key)
{
    g_return_val_if_fail (ctx != NULL, NULL);
    g_return_val_if_fail (key != NULL, NULL);

    return g_strdup_printf ("%s/%s", ctx->prefix, key);
}

void
mateweather_mateconf_set_bool (MateWeatherMateConf  *ctx,
			 const gchar    *key,
			 gboolean        the_bool,
			 GError        **opt_error)
{
    gchar *full_key;

    g_return_if_fail (ctx != NULL);
    g_return_if_fail (key != NULL);
    g_return_if_fail (opt_error == NULL || *opt_error == NULL);

    full_key = mateweather_mateconf_get_full_key (ctx, key);
    mateconf_client_set_bool (ctx->mateconf, full_key, the_bool, opt_error);
    g_free (full_key);
}

void
mateweather_mateconf_set_int (MateWeatherMateConf  *ctx,
			const gchar    *key,
			gint            the_int,
			GError        **opt_error)
{
    gchar *full_key;

    g_return_if_fail (ctx != NULL);
    g_return_if_fail (key != NULL);
    g_return_if_fail (opt_error == NULL || *opt_error == NULL);

    full_key = mateweather_mateconf_get_full_key (ctx, key);
    mateconf_client_set_int (ctx->mateconf, full_key, the_int, opt_error);
    g_free (full_key);
}

void
mateweather_mateconf_set_string (MateWeatherMateConf  *ctx,
			   const gchar    *key,
			   const gchar    *the_string,
			   GError        **opt_error)
{
    gchar *full_key;

    g_return_if_fail (ctx != NULL);
    g_return_if_fail (key != NULL);
    g_return_if_fail (opt_error == NULL || *opt_error == NULL);

    full_key = mateweather_mateconf_get_full_key (ctx, key);
    mateconf_client_set_string (ctx->mateconf, full_key, the_string, opt_error);
    g_free (full_key);
}

gboolean
mateweather_mateconf_get_bool (MateWeatherMateConf  *ctx,
			 const gchar    *key,
			 GError        **opt_error)
{
    gchar *full_key;
    gboolean ret;

    g_return_val_if_fail (ctx != NULL, FALSE);
    g_return_val_if_fail (key != NULL, FALSE);
    g_return_val_if_fail (opt_error == NULL || *opt_error == NULL, FALSE);

    full_key = mateweather_mateconf_get_full_key (ctx, key);
    ret = mateconf_client_get_bool (ctx->mateconf, full_key, opt_error);
    g_free (full_key);
    return ret;
}

gint
mateweather_mateconf_get_int (MateWeatherMateConf  *ctx,
			const gchar    *key,
			GError        **opt_error)
{
    gchar *full_key;
    gint ret;

    g_return_val_if_fail (ctx != NULL, 0);
    g_return_val_if_fail (key != NULL, 0);
    g_return_val_if_fail (opt_error == NULL || *opt_error == NULL, 0);

    full_key = mateweather_mateconf_get_full_key (ctx, key);
    ret = mateconf_client_get_int (ctx->mateconf, full_key, opt_error);
    g_free (full_key);
    return ret;
}

gchar *
mateweather_mateconf_get_string (MateWeatherMateConf  *ctx,
			   const gchar    *key,
			   GError        **opt_error)
{
    gchar *full_key;
    gchar *ret;

    g_return_val_if_fail (ctx != NULL, NULL);
    g_return_val_if_fail (key != NULL, NULL);
    g_return_val_if_fail (opt_error == NULL || *opt_error == NULL, NULL);

    full_key = mateweather_mateconf_get_full_key (ctx, key);
    ret = mateconf_client_get_string (ctx->mateconf, full_key, opt_error);
    g_free (full_key);
    return ret;
}


WeatherLocation *
mateweather_mateconf_get_location (MateWeatherMateConf *ctx)
{
    WeatherLocation *location;
    gchar *name, *code, *zone, *radar, *coordinates;

    g_return_val_if_fail (ctx != NULL, NULL);

    name = mateweather_mateconf_get_string (ctx, "location4", NULL);
    if (!name)
    {
        /* TRANSLATOR: Change this to the default location name,
         * used when you first start the Weather Applet. This is
	 * the common localised name that corresponds to
	 * the location code (DEFAULT_CODE) you will put on the next message
	 * For example, for the Greek locale, we set this to "Athens", the
	 * capital city and we write it in Greek. It's important to translate
	 * this name.
	 *
	 * If you do not require a DEFAULT_LOCATION, set this to
	 * "DEFAULT_LOCATION".
	 */
        if (strcmp ("DEFAULT_LOCATION", _("DEFAULT_LOCATION")))
            name = g_strdup (_("DEFAULT_LOCATION"));
        else
            name = g_strdup ("Pittsburgh");
    }

    code = mateweather_mateconf_get_string (ctx, "location1", NULL);
    if (!code)
    {
        /* TRANSLATOR: Change this to the code of your default location that
	 * corresponds to the DEFAULT_LOCATION name you put above. This is
	 * normally a four-letter (ICAO) code and can be found in
	 * http://git.gnome.org/cgit/libmateweather/plain/data/Locations.xml.in
	 * NB. The web page is over 1.7MB in size.
	 * Pick a default location like a capital city so that it would be ok
	 * for more of your users. For example, for Greek, we use "LGAV" for
	 * the capital city, Athens.
	 *
	 * If you do not require a DEFAULT_CODE, set this to "DEFAULT_CODE".
	 */
        if (strcmp ("DEFAULT_CODE", _("DEFAULT_CODE")))
            code = g_strdup (_("DEFAULT_CODE"));
        else
    	    code = g_strdup ("KPIT");
    }

    zone = mateweather_mateconf_get_string (ctx, "location2", NULL);
    if (!zone)
    {
        /* TRANSLATOR: Change this to the zone of your default location that
	 * corresponds to the DEFAULT_LOCATION and DEFAULT_CODE you put above.
	 * Normally, US and Canada locations have zones while the rest do not.
	 * Check
	 * http://git.gnome.org/cgit/libmateweather/plain/data/Locations.xml.in
	 * as any zone you put here must also be present in the Locations.xml
	 * file.
	 *
	 * If your default location does not have a zone, set this to
	 * "DEFAULT_ZONE".
	 */
        if (strcmp ("DEFAULT_ZONE", _("DEFAULT_ZONE")))
            zone = g_strdup (_("DEFAULT_ZONE" ));
        else
            zone = g_strdup ("PAZ021");
    }

    radar = mateweather_mateconf_get_string (ctx, "location3", NULL);
    if (!radar)
    {
        /* TRANSLATOR: Change this to the radar of your default location that
	 * corresponds to the DEFAULT_LOCATION and DEFAULT_CODE you put above.
	 * Normally, US and Canada locations have radar names while the rest do
	 * not. Check
	 * http://git.gnome.org/cgit/libmateweather/plain/data/Locations.xml.in
	 * as any radar you put here must also be present in the Locations.xml
	 * file.
	 *
	 * If your default location does not have a radar, set this to " "
	 * (or space).
	 * If you do not have a default location, set this to DEFAULT_RADAR.
	 */
        if (strcmp ("DEFAULT_RADAR", _("DEFAULT_RADAR")))
            radar = g_strdup (_("DEFAULT_RADAR"));
        else
            radar = g_strdup ("pit");
    }

    coordinates = mateweather_mateconf_get_string (ctx, "coordinates", NULL);
    if (!coordinates)
    {
        /* TRANSLATOR: Change this to the coordinates of your default location
	 * that corresponds to the DEFAULT_LOCATION and DEFAULT_CODE you put
	 * above. Check
	 * http://git.gnome.org/cgit/libmateweather/plain/data/Locations.xml.in
	 * as any coordinates you put here must also be present in the
	 * Locations.xml file.
	 *
	 * If your default location does not have known coordinates, set this
	 * to " " (or space).
	 * If you do not have a default location, set this to
	 * DEFAULT_COORDINATES.
	 */
	if (strcmp ("DEFAULT_COORDINATES", _("DEFAULT_COORDINATES")))
	    coordinates = g_strdup (_("DEFAULT_COORDINATES"));
	else
	    coordinates = g_strdup ("40-32N 080-13W");
    }

    location = weather_location_new (name, code, zone, radar, coordinates,
				     NULL, NULL);

    g_free (name);
    g_free (code);
    g_free (zone);
    g_free (radar);
    g_free (coordinates);

    return location;
}
