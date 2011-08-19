/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* weather-moon.c - Lunar calculations for mateweather
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
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef __FreeBSD__
#include <sys/types.h>
#endif

#include <math.h>
#include <time.h>
#include <string.h>
#include <glib.h>

#define MATEWEATHER_I_KNOW_THIS_IS_UNSTABLE
#include "weather-priv.h"

/*
 * Elements of the Moon's matecorba, epoch 2000 Jan 1.5
 * http://ssd.jpl.nasa.gov/?sat_elem#earth
 * The page only lists most values to 2 decimal places
 */

#define LUNAR_MEAN_LONGITUDE	218.316
#define LUNAR_PERIGEE_MEAN_LONG	318.15
#define LUNAR_NODE_MEAN_LONG	125.08
#define LUNAR_PROGRESSION	13.176358
#define LUNAR_INCLINATION	DEGREES_TO_RADIANS(5.145396)

/**
 * calc_moon:
 * @info:  WeatherInfo containing time_t of interest.  The
 *    values moonphase, moonlatitude and moonValid are updated
 *    on success.
 *
 * Returns: gboolean indicating success or failure.
 *    moonphase is expressed as degrees where '0' is a new moon,
 *    '90' is first quarter, etc.
 */

gboolean
calc_moon (WeatherInfo *info)
{
    time_t  t;
    gdouble ra_h;
    gdouble decl_r;
    gdouble ndays, sunMeanAnom_d;
    gdouble moonLong_d;
    gdouble moonMeanAnom_d, moonMeanAnom_r;
    gdouble sunEclipLong_r;
    gdouble ascNodeMeanLong_d;
    gdouble corrLong_d, eviction_d;
    gdouble sinSunMeanAnom;
    gdouble Ae, A3, Ec, A4, lN_r;
    gdouble lambda_r, beta_r;

    /*
     * The comments refer to the enumerated steps to calculate the
     * position of the moon (section 65 of above reference)
     */
    t = info->update;
    ndays = EPOCH_TO_J2000(t) / 86400.;
    sunMeanAnom_d = fmod (MEAN_ECLIPTIC_LONGITUDE (ndays) - PERIGEE_LONGITUDE (ndays),
			  360.);
    sunEclipLong_r = sunEclipLongitude (t);
    moonLong_d = fmod (LUNAR_MEAN_LONGITUDE + (ndays * LUNAR_PROGRESSION),
		       360.);
                                               /*  5: moon's mean anomaly */
    moonMeanAnom_d = fmod ((moonLong_d - (0.1114041 * ndays)
			    - (LUNAR_PERIGEE_MEAN_LONG + LUNAR_NODE_MEAN_LONG)),
			   360.);
                                               /*  6: ascending node mean longitude */
    ascNodeMeanLong_d = fmod (LUNAR_NODE_MEAN_LONG - (0.0529539 * ndays),
			      360.);
    eviction_d = 1.2739                        /*  7: eviction */
        * sin (DEGREES_TO_RADIANS (2.0 * (moonLong_d - RADIANS_TO_DEGREES (sunEclipLong_r))
				   - moonMeanAnom_d));
    sinSunMeanAnom = sin (DEGREES_TO_RADIANS (sunMeanAnom_d));
    Ae = 0.1858 * sinSunMeanAnom;
    A3 = 0.37   * sinSunMeanAnom;              /*  8: annual equation    */
    moonMeanAnom_d += eviction_d - Ae - A3;    /*  9: "third correction" */
    moonMeanAnom_r = DEGREES_TO_RADIANS (moonMeanAnom_d);
    Ec = 6.2886 * sin (moonMeanAnom_r);        /* 10: equation of center */
    A4 = 0.214 * sin (2.0 * moonMeanAnom_r);   /* 11: "yet another correction" */

    /* Steps 12-14 give the true longitude after correcting for variation */
    moonLong_d += eviction_d + Ec - Ae + A4
        + (0.6583 * sin (2.0 * (moonMeanAnom_r - sunEclipLong_r)));

                                        /* 15: corrected longitude of node */
    corrLong_d = ascNodeMeanLong_d - 0.16 * sinSunMeanAnom;

    /*
     * Calculate ecliptic latitude (16-19) and longitude (20) of the moon,
     * then convert to right ascension and declination.
     */
    lN_r = DEGREES_TO_RADIANS (moonLong_d - corrLong_d);   /* l''-N' */
    lambda_r = DEGREES_TO_RADIANS(corrLong_d)
        + atan2 (sin (lN_r) * cos (LUNAR_INCLINATION),  cos (lN_r));
    beta_r = asin (sin (lN_r) * sin (LUNAR_INCLINATION));
    ecl2equ (t, lambda_r, beta_r, &ra_h, &decl_r);

    /*
     * The phase is the angle from the sun's longitude to the moon's 
     */
    info->moonphase =
        fmod (15.*ra_h - RADIANS_TO_DEGREES (sunEclipLongitude (info->update)),
	      360.);
    if (info->moonphase < 0)
        info->moonphase += 360;
    info->moonlatitude = RADIANS_TO_DEGREES (decl_r);
    info->moonValid = TRUE;

    return TRUE;
}


/**
 * calc_moon_phases:
 * @info:   WeatherInfo containing the time_t of interest
 * @phases: An array of four time_t values that will hold the returned values.
 *    The values are estimates of the time of the next new, quarter, full and
 *    three-quarter moons.
 *
 * Returns: gboolean indicating success or failure
 */

gboolean
calc_moon_phases (WeatherInfo *info, time_t *phases)
{
    WeatherInfo temp;
    time_t      *ptime;
    int         idx;
    gdouble     advance;
    int         iter;
    time_t      dtime;

    g_return_val_if_fail (info != NULL &&
			  (info->moonValid || calc_moon (info)),
			  FALSE);

    ptime = phases;
    memset(&temp, 0, sizeof(WeatherInfo));

    for (idx = 0; idx < 4; idx++) {
	temp.update = info->update;
	temp.moonphase = info->moonphase;

	/*
	 * First estimate on how far the moon needs to advance
	 * to get to the required phase
	 */
	advance = (idx * 90.) - info->moonphase;
	if (advance < 0.)
	    advance += 360.;

	for (iter = 0; iter < 10; iter++) {
	    /* Convert angle change (degrees) to dtime (seconds) */
	    dtime = advance / LUNAR_PROGRESSION * 86400.;
	    if ((dtime > -10) && (dtime < 10))
		break;
	    temp.update += dtime;
	    (void)calc_moon (&temp);

	    if (idx == 0 && temp.moonphase > 180.) {
		advance = 360. - temp.moonphase;
	    } else {
		advance = (idx * 90.) - temp.moonphase;
	    }
	}
	*ptime++ = temp.update;
    }

    return TRUE;
}
