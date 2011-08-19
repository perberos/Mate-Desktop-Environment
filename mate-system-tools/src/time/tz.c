/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* Generic timezone utilities.
 *
 * Copyright (C) 2000-2001 Ximian, Inc.
 *
 * Authors: Hans Petter Jansson <hpj@ximian.com>
 * 
 * Largely based on Michael Fulbright's work on Anaconda.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */


#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include "tz.h"


/* Forward declarations for private functions */

static float convert_pos (gchar *pos, int digits);
static int compare_country_names (const void *a, const void *b);
static void sort_locations_by_country (GPtrArray *locations);
static gchar * tz_data_file_get (void);


/* ---------------- *
 * Public interface *
 * ---------------- */
TzDB *
tz_load_db (void)
{
	gchar *tz_data_file;
	TzDB *tz_db;
	FILE *tzfile;
	char buf[4096];

	tz_data_file = tz_data_file_get ();
	if (!tz_data_file) {
		g_warning ("Could not get the TimeZone data file name");
		return NULL;
	}
	tzfile = fopen (tz_data_file, "r");
	if (!tzfile) {
		g_warning ("Could not open *%s*\n", tz_data_file);
		g_free (tz_data_file);
		return NULL;
	}

	tz_db = g_new0 (TzDB, 1);
	tz_db->locations = g_ptr_array_new ();

	while (fgets (buf, sizeof(buf), tzfile))
	{
		gchar **tmpstrarr;
		gchar *latstr, *lngstr, *p;
		TzLocation *loc;

		if (*buf == '#') continue;

		g_strchomp(buf);
		tmpstrarr = g_strsplit(buf,"\t", 6);
		
		latstr = g_strdup (tmpstrarr[1]);
		p = latstr + 1;
		while (*p != '-' && *p != '+') p++;
		lngstr = g_strdup (p);
		*p = '\0';
		
		loc = g_new0 (TzLocation, 1);
		loc->country = g_strdup (tmpstrarr[0]);
		loc->zone = g_strdup (tmpstrarr[2]);
		loc->latitude  = convert_pos (latstr, 2);
		loc->longitude = convert_pos (lngstr, 3);
		
#ifdef __sun
		if (tmpstrarr[3] && *tmpstrarr[3] == '-' && tmpstrarr[4])
			loc->comment = g_strdup (tmpstrarr[4]);

		if (tmpstrarr[3] && *tmpstrarr[3] != '-' && !islower(loc->zone)) {
			TzLocation *locgrp;

			/* duplicate entry */
			locgrp = g_new0 (TzLocation, 1);
			locgrp->country = g_strdup (tmpstrarr[0]);
			locgrp->zone = g_strdup (tmpstrarr[3]);
			locgrp->latitude  = convert_pos (latstr, 2);
			locgrp->longitude = convert_pos (lngstr, 3);
			locgrp->comment = (tmpstrarr[4]) ? g_strdup (tmpstrarr[4]) : NULL;

			g_ptr_array_add (tz_db->locations, (gpointer) locgrp);
		}
#else
		loc->comment = (tmpstrarr[3]) ? g_strdup(tmpstrarr[3]) : NULL;
#endif

		g_ptr_array_add (tz_db->locations, (gpointer) loc);

		g_free (latstr);
		g_free (lngstr);
		g_strfreev (tmpstrarr);
	}
	
	fclose (tzfile);
	
	/* now sort by country */
	sort_locations_by_country (tz_db->locations);
	
	g_free (tz_data_file);
	
	return tz_db;
}    

GPtrArray *
tz_get_locations (TzDB *db)
{
	return db->locations;
}


gchar *
tz_location_get_country (TzLocation *loc)
{
	return loc->country;
}


gchar *
tz_location_get_zone (TzLocation *loc)
{
	return loc->zone;
}


gchar *
tz_location_get_comment (TzLocation *loc)
{
	return loc->comment;
}


void
tz_location_get_position (TzLocation *loc, double *longitude, double *latitude)
{
	*longitude = loc->longitude;
	*latitude = loc->latitude;
}

glong
tz_location_get_utc_offset (TzLocation *loc)
{
	TzInfo *tz_info;
	glong offset;

	tz_info = tz_info_from_location (loc);
	offset = tz_info->utc_offset;
	tz_info_free (tz_info);
	return offset;
}

gint
tz_location_set_locally (TzLocation *loc)
{
	gchar *str;
	time_t curtime;
	struct tm *curzone;
	gboolean is_dst = FALSE;
	gint correction = 0;

	g_return_val_if_fail (loc != NULL, 0);
	g_return_val_if_fail (loc->zone != NULL, 0);
	
	curtime = time (NULL);
	curzone = localtime (&curtime);
	is_dst = curzone->tm_isdst;

	str = g_strdup_printf ("TZ=%s", loc->zone);
	putenv (str);
#if 0
	curtime = time (NULL);
	curzone = localtime (&curtime);

	if (!is_dst && curzone->tm_isdst) {
		correction = (60 * 60);
	}
	else if (is_dst && !curzone->tm_isdst) {
		correction = 0;
	}
#endif

	return correction;
}

TzInfo *
tz_info_from_location (TzLocation *loc)
{
	TzInfo *tzinfo;
	gchar *str;
	time_t curtime;
	struct tm *curzone;
	
	g_return_val_if_fail (loc != NULL, NULL);
	g_return_val_if_fail (loc->zone != NULL, NULL);
	
	str = g_strdup_printf ("TZ=%s", loc->zone);
	putenv (str);
	
#if 0
	tzset ();
#endif
	tzinfo = g_new0 (TzInfo, 1);

	curtime = time (NULL);
	curzone = localtime (&curtime);

#ifndef __sun
	/* Currently this solution doesnt seem to work - I get that */
	/* America/Phoenix uses daylight savings, which is wrong    */
	tzinfo->tzname_normal = g_strdup (curzone->tm_zone);
	if (curzone->tm_isdst) 
		tzinfo->tzname_daylight =
			g_strdup (&curzone->tm_zone[curzone->tm_isdst]);
	else
		tzinfo->tzname_daylight = NULL;

	tzinfo->utc_offset = curzone->tm_gmtoff;
#else
	tzinfo->tzname_normal = NULL;
	tzinfo->tzname_daylight = NULL;
	tzinfo->utc_offset = 0;
#endif

	tzinfo->daylight = curzone->tm_isdst;
	
	return tzinfo;
}


void
tz_info_free (TzInfo *tzinfo)
{
	g_return_if_fail (tzinfo != NULL);
	
	if (tzinfo->tzname_normal) g_free (tzinfo->tzname_normal);
	if (tzinfo->tzname_daylight) g_free (tzinfo->tzname_daylight);
	g_free (tzinfo);
}

/* ----------------- *
 * Private functions *
 * ----------------- */

static gchar *
tz_data_file_get (void)
{
	gchar *file;

	file = g_strdup (TZ_DATA_FILE);

	return file;
}

static float
convert_pos (gchar *pos, int digits)
{
	gchar whole[10];
	gchar *fraction;
	gint i;
	float t1, t2;
	
	if (!pos || strlen(pos) < 4 || digits > 9) return 0.0;
	
	for (i = 0; i < digits + 1; i++) whole[i] = pos[i];
	whole[i] = '\0';
	fraction = pos + digits + 1;

	t1 = g_strtod (whole, NULL);
	t2 = g_strtod (fraction, NULL);

	if (t1 >= 0.0) return t1 + t2/pow (10.0, strlen(fraction));
	else return t1 - t2/pow (10.0, strlen(fraction));
}


#if 0

/* Currently not working */
static void
free_tzdata (TzLocation *tz)
{
	
	if (tz->country)
	  g_free(tz->country);
	if (tz->zone)
	  g_free(tz->zone);
	if (tz->comment)
	  g_free(tz->comment);
	
	g_free(tz);
}
#endif


static int
compare_country_names (const void *a, const void *b)
{
	const TzLocation *tza = * (TzLocation **) a;
	const TzLocation *tzb = * (TzLocation **) b;
	
	return strcmp (tza->zone, tzb->zone);
}


static void
sort_locations_by_country (GPtrArray *locations)
{
	qsort (locations->pdata, locations->len, sizeof (gpointer),
	       compare_country_names);
}
