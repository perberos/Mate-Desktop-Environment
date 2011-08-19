/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* weather-sun.c - Astronomy calculations for mateweather
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

/*
 * Formulas from:
 * "Practical Astronomy With Your Calculator" (3e), Peter Duffett-Smith
 * Cambridge University Press 1988
 * Unless otherwise noted, comments referencing "steps" are related to
 * the algorithm presented in section 49 of above
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>
#include <time.h>
#include <glib.h>

#define MATEWEATHER_I_KNOW_THIS_IS_UNSTABLE
#include "weather-priv.h"

#define ECCENTRICITY(d)         (0.01671123 - (d)/36525.*0.00004392)

/* 
 * Ecliptic longitude of the sun at specified time (UT)
 * The algoithm is described in section 47 of Duffett-Smith
 * Return value is in radians
 */
gdouble
sunEclipLongitude(time_t t)
{
    gdouble ndays, meanAnom, eccenAnom, delta, e, longitude;

    /*
     * Start with an estimate based on a fixed daily rate
     */
    ndays = EPOCH_TO_J2000(t) / 86400.;
    meanAnom = DEGREES_TO_RADIANS(MEAN_ECLIPTIC_LONGITUDE(ndays)
				  - PERIGEE_LONGITUDE(ndays));
    
    /*
     * Approximate solution of Kepler's equation:
     * Find E which satisfies  E - e sin(E) = M (mean anomaly)
     */                                         
    eccenAnom = meanAnom;
    e = ECCENTRICITY(ndays);

    while (1e-12 < fabs( delta = eccenAnom - e * sin(eccenAnom) - meanAnom))
    {
	eccenAnom -= delta / (1.- e * cos(eccenAnom));
    }

    /*
     * Earth's longitude on the ecliptic
     */
    longitude = fmod( DEGREES_TO_RADIANS (PERIGEE_LONGITUDE(ndays))
		      + 2. * atan (sqrt ((1.+e)/(1.-e))
				   * tan (eccenAnom / 2.)),
		      2. * M_PI);
    if (longitude < 0.) {
	longitude += 2 * M_PI;
    }
    return longitude;
}

static gdouble
ecliptic_obliquity (gdouble time)
{
    gdouble jc = EPOCH_TO_J2000 (time) / (36525. * 86400.);
    gdouble eclip_secs = (84381.448
			  - (46.84024 * jc)
			  - (59.e-5 * jc * jc)
			  + (1.813e-3 * jc * jc * jc));
    return DEGREES_TO_RADIANS(eclip_secs / 3600.);
}

/*
 * Convert ecliptic longitude and latitude (radians) to equitorial
 * coordinates, expressed as right ascension (hours) and
 * declination (radians)
 */
void
ecl2equ (gdouble time,
	 gdouble eclipLon, gdouble eclipLat,
	 gdouble *ra, gdouble *decl)
{
    gdouble mEclipObliq = ecliptic_obliquity(time);

    if (ra) {
	*ra = RADIANS_TO_HOURS (atan2 ((sin (eclipLon) * cos (mEclipObliq)
					- tan (eclipLat) * sin(mEclipObliq)),
				       cos (eclipLon)));
	if (*ra < 0.)
	    *ra += 24.;
    }
    if (decl) {
	*decl = asin (( sin (eclipLat) * cos (mEclipObliq))
		      + cos (eclipLat) * sin (mEclipObliq) * sin(eclipLon));
    }
}

/*
 * Calculate rising and setting times for an object
 * based on it equitorial coordinates (section 33 & 15)
 * Returned "rise" and "set" values are sideral times in hours
 */
static void
gstObsv (gdouble ra, gdouble decl,
	 gdouble obsLat, gdouble obsLon,
	 gdouble *rise, gdouble *set)
{
    double a = acos (-tan (obsLat) * tan (decl));
    double b;

    if (isnan (a) != 0) {
	*set = *rise = a;
	return;
    }
    a = RADIANS_TO_HOURS (a);
    b = 24. - a + ra;
    a += ra;
    a -= RADIANS_TO_HOURS (obsLon);
    b -= RADIANS_TO_HOURS (obsLon);
    if ((a = fmod (a, 24.)) < 0)
	a += 24.;
    if ((b = fmod (b, 24.)) < 0)
	b += 24.;

    *set = a;
    *rise = b;
}


static gdouble
t0 (time_t date)
{
    gdouble t = ((gdouble)(EPOCH_TO_J2000 (date) / 86400)) / 36525.0;
    gdouble t0 = fmod (6.697374558 + 2400.051366 * t + 2.5862e-5 * t * t, 24.);
    if (t0 < 0.)
        t0 += 24.;
    return t0;
}


static gboolean
calc_sun2 (WeatherInfo *info, time_t t)
{
    gdouble obsLat = info->location->latitude;
    gdouble obsLon = info->location->longitude;
    time_t gm_midn;
    time_t lcl_midn;
    gdouble gm_hoff, lambda;
    gdouble ra1, ra2;
    gdouble decl1, decl2;
    gdouble decl_midn, decl_noon;
    gdouble rise1, rise2;
    gdouble set1, set2;
    gdouble tt, t00;
    gdouble x, u, dt;

    /* Approximate preceding local midnight at observer's longitude */
    obsLat = info->location->latitude;
    obsLon = info->location->longitude;
    gm_midn = t - (t % 86400);
    gm_hoff = floor ((RADIANS_TO_DEGREES (obsLon) + 7.5) / 15.);
    lcl_midn = gm_midn - 3600. * gm_hoff;
    if (t - lcl_midn >= 86400)
        lcl_midn += 86400;
    else if (lcl_midn > t)
        lcl_midn -= 86400;

    lambda = sunEclipLongitude (lcl_midn);
    
    /*
     * Calculate equitorial coordinates of sun at previous
     * and next local midnights
     */
    ecl2equ (lcl_midn, lambda, 0., &ra1, &decl1);
    ecl2equ (lcl_midn + 86400.,
	     lambda + DEGREES_TO_RADIANS(SOL_PROGRESSION), 0.,
	     &ra2, &decl2);
    
    /*
     * If the observer is within the Arctic or Antarctic Circles then
     * the sun may be above or below the horizon for the full day.
     */
    decl_midn = MIN(decl1,decl2);
    decl_noon = (decl1+decl2)/2.;
    info->midnightSun =
	(obsLat > (M_PI/2.-decl_midn)) || (obsLat < (-M_PI/2.-decl_midn));
    info->polarNight =
	(obsLat > (M_PI/2.+decl_noon)) || (obsLat < (-M_PI/2.+decl_noon));
    if (info->midnightSun || info->polarNight) {
	info->sunriseValid = info->sunsetValid = FALSE;
	return FALSE;
    }

    /*
     * Convert to rise and set times based positions for the preceding
     * and following local midnights.
     */
    gstObsv (ra1, decl1, obsLat, obsLon - (gm_hoff * M_PI / 12.), &rise1, &set1);
    gstObsv (ra2, decl2, obsLat, obsLon - (gm_hoff * M_PI / 12.), &rise2, &set2);

    /* TODO: include calculations for regions near the poles. */
    if (isnan(rise1) || isnan(rise2)) {
	info->sunriseValid = info->sunsetValid = FALSE;
        return FALSE;
    }

    if (rise2 < rise1) {
        rise2 += 24.;
    }
    if (set2 < set1) {
        set2 += 24.;
    }

    tt = t0(lcl_midn);
    t00 = tt - (gm_hoff + RADIANS_TO_HOURS(obsLon)) * 1.002737909;

    if (t00 < 0.)
        t00 += 24.;

    if (rise1 < t00) {
        rise1 += 24.;
        rise2 += 24.;
    }
    if (set1 < t00) {
        set1  += 24.;
        set2  += 24.;
    }
   
    /*
     * Interpolate between the two to get a rise and set time
     * based on the sun's position at local noon (step 8)
     */
    rise1 = (24.07 * rise1 - t00 * (rise2 - rise1)) / (24.07 + rise1 - rise2);
    set1 = (24.07 * set1 - t00 * (set2 - set1)) / (24.07 + set1 - set2);

    /*
     * Calculate an adjustment value to account for parallax,
     * refraction and the Sun's finite diameter (steps 9,10)
     */
    decl2 = (decl1 + decl2) / 2.;
    x = DEGREES_TO_RADIANS(0.830725);
    u = acos ( sin(obsLat) / cos(decl2) );
    dt = RADIANS_TO_HOURS ( asin ( sin(x) / sin(u) ) / cos(decl2) );
   
    /*
     * Subtract the correction value from sunrise and add to sunset,
     * then (step 11) convert sideral times to UT
     */
    rise1 = (rise1 - dt - tt) * 0.9972695661;
    if (rise1 < 0.)
	rise1 += 24;
    else if (rise1 >= 24.)
	rise1 -= 24.;
    info->sunriseValid = ((rise1 >= 0.) && (rise1 < 24.));
    info->sunrise = (rise1 * 3600.) + lcl_midn;

    set1  = (set1 + dt - tt) * 0.9972695661;
    if (set1 < 0.)
	set1 += 24;
    else if (set1 >= 24.)
	set1 -= 24.;
    info->sunsetValid = ((set1 >= 0.) && (set1 < 24.));
    info->sunset = (set1 * 3600.) + lcl_midn;

    return (info->sunriseValid || info->sunsetValid);
}


/**
 * calc_sun_time:
 * @info: #WeatherInfo structure containing the observer's latitude
 * and longitude in radians, fills in the sunrise and sunset times.
 * @t: time_t
 *
 * Returns: gboolean indicating if the results are valid.
 */
gboolean
calc_sun_time (WeatherInfo *info, time_t t)
{
    return info->location->latlon_valid && calc_sun2 (info, t);
}

/**
 * calc_sun:
 * @info: #WeatherInfo structure containing the observer's latitude
 * and longitude in radians, fills in the sunrise and sunset times.
 *
 * Returns: gboolean indicating if the results are valid.
 */
gboolean
calc_sun (WeatherInfo *info)
{
    return calc_sun_time(info, time(NULL));
}


/**
 * weather_info_next_sun_event:
 * @info: #WeatherInfo structure
 *
 * Returns: the interval, in seconds, until the next "sun event":
 *  - local midnight, when rise and set times are recomputed
 *  - next sunrise, when icon changes to daytime version
 *  - next sunset, when icon changes to nighttime version
 */
gint
weather_info_next_sun_event (WeatherInfo *info)
{
    time_t    now = time (NULL);
    struct tm ltm;
    time_t    nxtEvent;

    g_return_val_if_fail (info != NULL, -1);

    if (!calc_sun (info))
	return -1;

    /* Determine when the next local midnight occurs */
    (void) localtime_r (&now, &ltm);
    ltm.tm_sec = 0;
    ltm.tm_min = 0;
    ltm.tm_hour = 0;
    ltm.tm_mday++;
    nxtEvent = mktime (&ltm);

    if (info->sunsetValid &&
	(info->sunset > now) && (info->sunset < nxtEvent))
	nxtEvent = info->sunset;
    if (info->sunriseValid &&
	(info->sunrise > now) && (info->sunrise < nxtEvent))
	nxtEvent = info->sunrise;
    return (gint)(nxtEvent - now);
}
