/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */

#include <glib.h>
#include <string.h>
#include <time.h>

#define MATEWEATHER_I_KNOW_THIS_IS_UNSTABLE
#include "weather-priv.h"

int
main (int argc, char **argv)
{
    WeatherInfo     info;
    GOptionContext* context;
    GError*         error = NULL;
    gdouble         latitude, longitude;
    WeatherLocation location;
    gchar*          gtime = NULL;
    GDate           gdate;
    struct tm       tm;
    gboolean        bsun, bmoon;
    time_t          phases[4];
    const GOptionEntry entries[] = {
	{ "latitude", 0, 0, G_OPTION_ARG_DOUBLE, &latitude,
	  "observer's latitude in degrees north", NULL },
	{ "longitude", 0, 0,  G_OPTION_ARG_DOUBLE, &longitude,
	  "observer's longitude in degrees east", NULL },
	{ "time", 0, 0, G_OPTION_ARG_STRING, &gtime,
	  "time in seconds from Unix epoch", NULL },
	{ NULL }
    };

    memset(&location, 0, sizeof(WeatherLocation));
    memset(&info, 0, sizeof(WeatherInfo));

    context = g_option_context_new ("- test libmateweather sun/moon calculations");
    g_option_context_add_main_entries (context, entries, NULL);
    g_option_context_parse (context, &argc, &argv, &error);

    if (error) {
	perror (error->message);
	return error->code;
    }
    else if (latitude < -90. || latitude > 90.) {
	perror ("invalid latitude: should be [-90 .. 90]");
	return -1;
    } else if (longitude < -180. || longitude > 180.) {
	perror ("invalid longitude: should be [-180 .. 180]");
	return -1;
    }

    location.latitude = DEGREES_TO_RADIANS(latitude);
    location.longitude = DEGREES_TO_RADIANS(longitude);
    location.latlon_valid = TRUE;
    info.location = &location;
    info.valid = TRUE;

    if (gtime != NULL) {
	//	printf(" gtime=%s\n", gtime);
	g_date_set_parse(&gdate, gtime);
	g_date_to_struct_tm(&gdate, &tm);
	info.update = mktime(&tm);
    } else {
	info.update = time(NULL);
    }

    bsun = calc_sun_time(&info, info.update);
    bmoon = calc_moon(&info);

    printf ("  Latitude %7.3f %c  Longitude %7.3f %c for %s  All times UTC\n",
	    fabs(latitude), (latitude >= 0. ? 'N' : 'S'),
	    fabs(longitude), (longitude >= 0. ? 'E' : 'W'),
	    asctime(gmtime(&info.update)));
    printf("sunrise:   %s",
	   (info.sunriseValid ? ctime(&info.sunrise) : "(invalid)\n"));
    printf("sunset:    %s",
	   (info.sunsetValid ? ctime(&info.sunset)  : "(invalid)\n"));
    if (bmoon) {
	printf("moonphase: %g\n", info.moonphase);
	printf("moonlat:   %g\n", info.moonlatitude);

	if (calc_moon_phases(&info, phases)) {
	    printf("    New:   %s", asctime(gmtime(&phases[0])));
	    printf("    1stQ:  %s", asctime(gmtime(&phases[1])));
	    printf("    Full:  %s", asctime(gmtime(&phases[2])));
	    printf("    3rdQ:  %s", asctime(gmtime(&phases[3])));
	}
    }
    return 0;
}
