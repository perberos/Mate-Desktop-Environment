/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* weather-iwin.c - US National Weather Service IWIN forecast source
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

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <libxml/parser.h>

#define MATEWEATHER_I_KNOW_THIS_IS_UNSTABLE
#include "weather.h"
#include "weather-priv.h"

/**
 *  Humans don't deal well with .MONDAY...SUNNY AND BLAH BLAH.TUESDAY...THEN THIS AND THAT.WEDNESDAY...RAINY BLAH BLAH.
 *  This function makes it easier to read.
 */
static gchar *
formatWeatherMsg (gchar *forecast)
{
    gchar *ptr = forecast;
    gchar *startLine = NULL;

    while (0 != *ptr) {
        if (ptr[0] == '\n' && ptr[1] == '.') {
          /* This removes the preamble by shifting the relevant data
           * down to the start of the buffer. */
            if (NULL == startLine) {
                memmove (forecast, ptr, strlen (ptr) + 1);
                ptr = forecast;
                ptr[0] = ' ';
            }
            ptr[1] = '\n';
            ptr += 2;
            startLine = ptr;
        } else if (ptr[0] == '.' && ptr[1] == '.' && ptr[2] == '.' && NULL != startLine) {
            memmove (startLine + 2, startLine, (ptr - startLine) * sizeof (gchar));
            startLine[0] = ' ';
            startLine[1] = '\n';
            ptr[2] = '\n';

            ptr += 3;

        } else if (ptr[0] == '$' && ptr[1] == '$') {
            ptr[0] = ptr[1] = ' ';

        } else {
            ptr++;
        }
    }

    return forecast;
}

static gboolean
hasAttr (xmlNode *node, const char *attr_name, const char *attr_value)
{
    xmlChar *attr;
    gboolean res = FALSE;

    if (!node)
        return res;

    attr = xmlGetProp (node, (const xmlChar *) attr_name);

    if (!attr)
        return res;

    res = g_str_equal ((const char *)attr, attr_value);

    xmlFree (attr);

    return res;
}

static GSList *
parseForecastXml (const char *buff, WeatherInfo *master_info)
{
    GSList *res = NULL;
    xmlDocPtr doc;
    xmlNode *root, *node;

    g_return_val_if_fail (master_info != NULL, NULL);

    if (!buff || !*buff)
        return NULL;

    #define XC (const xmlChar *)
    #define isElem(_node,_name) g_str_equal ((const char *)_node->name, _name)

    doc = xmlParseMemory (buff, strlen (buff));
    if (!doc)
        return NULL;

    /* Description at http://www.weather.gov/mdl/XML/Design/MDL_XML_Design.pdf */
    root = xmlDocGetRootElement (doc);
    for (node = root->xmlChildrenNode; node; node = node->next) {
        if (node->name == NULL || node->type != XML_ELEMENT_NODE)
            continue;

        if (isElem (node, "data")) {
            xmlNode *n;
            char *time_layout = NULL;
            time_t update_times[7] = {0};

            for (n = node->children; n; n = n->next) {
                if (!n->name)
                    continue;

                if (isElem (n, "time-layout")) {
                    if (!time_layout && hasAttr (n, "summarization", "24hourly")) {
                        xmlNode *c;
                        int count = 0;

                        for (c = n->children; c && (count < 7 || !time_layout); c = c->next) {
                            if (c->name && !time_layout && isElem (c, "layout-key")) {
                                xmlChar *val = xmlNodeGetContent (c);

                                if (val) {
                                    time_layout = g_strdup ((const char *)val);
                                    xmlFree (val);
                                }
                            } else if (c->name && isElem (c, "start-valid-time")) {
                                xmlChar *val = xmlNodeGetContent (c);

                                if (val) {
                                    GTimeVal tv;

                                    if (g_time_val_from_iso8601 ((const char *)val, &tv)) {
                                        update_times[count] = tv.tv_sec;
                                    } else {
                                        update_times[count] = 0;
                                    }

                                    count++;

                                    xmlFree (val);
                                }
                            }
                        }

                        if (count != 7) {
                            /* There can be more than one time-layout element, the other
                               with only few children, which is not the one to use. */
                            g_free (time_layout);
                            time_layout = NULL;
                        }
                    }
                } else if (isElem (n, "parameters")) {
                    xmlNode *p;

                    /* time-layout should be always before parameters */
                    if (!time_layout)
                        break;

                    if (!res) {
                        int i;

                        for (i = 0; i < 7;  i++) {
                            WeatherInfo *nfo = weather_info_clone (master_info);

                            if (nfo) {
                                nfo->valid = FALSE;
                                nfo->forecast_type = FORECAST_ZONE;
                                nfo->update = update_times [i];
                                nfo->sky = -1;
                                nfo->temperature_unit = TEMP_UNIT_FAHRENHEIT;
                                nfo->temp = -1000.0;
                                nfo->temp_min = -1000.0;
                                nfo->temp_max = -1000.0;
                                nfo->tempMinMaxValid = FALSE;
                                nfo->cond.significant = FALSE;
                                nfo->cond.phenomenon = PHENOMENON_NONE;
                                nfo->cond.qualifier = QUALIFIER_NONE;
                                nfo->dew = -1000.0;
                                nfo->wind = -1;
                                nfo->windspeed = -1;
                                nfo->pressure = -1.0;
                                nfo->visibility = -1.0;
                                nfo->sunriseValid = FALSE;
                                nfo->sunsetValid = FALSE;
                                nfo->sunrise = 0;
                                nfo->sunset = 0;
                                g_free (nfo->forecast);
                                nfo->forecast = NULL;
				nfo->session = NULL;
				nfo->requests_pending = 0;
				nfo->finish_cb = NULL;
				nfo->cb_data = NULL;
                                res = g_slist_append (res, nfo);
                            }
                        }
                    }

                    for (p = n->children; p; p = p->next) {
                        if (p->name && isElem (p, "temperature") && hasAttr (p, "time-layout", time_layout)) {
                            xmlNode *c;
                            GSList *at = res;
                            gboolean is_max = hasAttr (p, "type", "maximum");

                            if (!is_max && !hasAttr (p, "type", "minimum"))
                                break;

                            for (c = p->children; c && at; c = c->next) {
                                if (isElem (c, "value")) {
                                    WeatherInfo *nfo = (WeatherInfo *)at->data;
                                    xmlChar *val = xmlNodeGetContent (c);

                                    /* can pass some values as <value xsi:nil="true"/> */
                                    if (!val || !*val) {
                                        if (is_max)
                                            nfo->temp_max = nfo->temp_min;
                                        else
                                            nfo->temp_min = nfo->temp_max;
                                    } else {
                                        if (is_max)
                                            nfo->temp_max = atof ((const char *)val);
                                        else
                                            nfo->temp_min = atof ((const char *)val);
                                    }

                                    if (val)
                                        xmlFree (val);

                                    nfo->tempMinMaxValid = nfo->tempMinMaxValid || (nfo->temp_max > -999.0 && nfo->temp_min > -999.0);
                                    nfo->valid = nfo->tempMinMaxValid;

                                    at = at->next;
                                }
                            }
                        } else if (p->name && isElem (p, "weather") && hasAttr (p, "time-layout", time_layout)) {
                            xmlNode *c;
                            GSList *at = res;

                            for (c = p->children; c && at; c = c->next) {
                                if (c->name && isElem (c, "weather-conditions")) {
                                    WeatherInfo *nfo = at->data;
                                    xmlChar *val = xmlGetProp (c, XC "weather-summary");

                                    if (val && nfo) {
                                        /* Checking from top to bottom, if 'value' contains 'name', then that win,
                                           thus put longer (more precise) values to the top. */
                                        int i;
                                        struct _ph_list {
                                            const char *name;
                                            WeatherConditionPhenomenon ph;
                                        } ph_list[] = {
                                            { "Ice Crystals", PHENOMENON_ICE_CRYSTALS } ,
                                            { "Volcanic Ash", PHENOMENON_VOLCANIC_ASH } ,
                                            { "Blowing Sand", PHENOMENON_SANDSTORM } ,
                                            { "Blowing Dust", PHENOMENON_DUSTSTORM } ,
                                            { "Blowing Snow", PHENOMENON_FUNNEL_CLOUD } ,
                                            { "Drizzle", PHENOMENON_DRIZZLE } ,
                                            { "Rain", PHENOMENON_RAIN } ,
                                            { "Snow", PHENOMENON_SNOW } ,
                                            { "Fog", PHENOMENON_FOG } ,
                                            { "Smoke", PHENOMENON_SMOKE } ,
                                            { "Sand", PHENOMENON_SAND } ,
                                            { "Haze", PHENOMENON_HAZE } ,
                                            { "Dust", PHENOMENON_DUST } /*,
                                            { "", PHENOMENON_SNOW_GRAINS } ,
                                            { "", PHENOMENON_ICE_PELLETS } ,
                                            { "", PHENOMENON_HAIL } ,
                                            { "", PHENOMENON_SMALL_HAIL } ,
                                            { "", PHENOMENON_UNKNOWN_PRECIPITATION } ,
                                            { "", PHENOMENON_MIST } ,
                                            { "", PHENOMENON_SPRAY } ,
                                            { "", PHENOMENON_SQUALL } ,
                                            { "", PHENOMENON_TORNADO } ,
                                            { "", PHENOMENON_DUST_WHIRLS } */
                                        };
                                        struct _sky_list {
                                            const char *name;
                                            WeatherSky sky;
                                        } sky_list[] = {
                                            { "Mostly Sunny", SKY_BROKEN } ,
                                            { "Mostly Clear", SKY_BROKEN } ,
                                            { "Partly Cloudy", SKY_SCATTERED } ,
                                            { "Mostly Cloudy", SKY_FEW } ,
                                            { "Sunny", SKY_CLEAR } ,
                                            { "Clear", SKY_CLEAR } ,
                                            { "Cloudy", SKY_OVERCAST } ,
                                            { "Clouds", SKY_SCATTERED } ,
                                            { "Rain", SKY_SCATTERED } ,
                                            { "Snow", SKY_SCATTERED }
                                        };

                                        nfo->valid = TRUE;
                                        g_free (nfo->forecast);
                                        nfo->forecast = g_strdup ((const char *)val);

                                        for (i = 0; i < G_N_ELEMENTS (ph_list); i++) {
                                            if (strstr ((const char *)val, ph_list [i].name)) {
                                                nfo->cond.phenomenon = ph_list [i].ph;
                                                break;
                                            }
                                        }

                                        for (i = 0; i < G_N_ELEMENTS (sky_list); i++) {
                                            if (strstr ((const char *)val, sky_list [i].name)) {
                                                nfo->sky = sky_list [i].sky;
                                                break;
                                            }
                                        }
                                    }

                                    if (val)
                                        xmlFree (val);

                                    at = at->next;
                                }
                            }
                        }
                    }

                    if (res) {
                        gboolean have_any = FALSE;
                        GSList *r;

                        /* Remove invalid forecast data from the list.
                           They should be all valid or all invalid. */
                        for (r = res; r; r = r->next) {
                            WeatherInfo *nfo = r->data;

                            if (!nfo || !nfo->valid) {
                                if (r->data)
                                    weather_info_free (r->data);

                                r->data = NULL;
                            } else {
                                have_any = TRUE;

                                if (nfo->tempMinMaxValid)
                                    nfo->temp = (nfo->temp_min + nfo->temp_max) / 2.0;
                            }
                        }

                        if (!have_any) {
                            /* data members are freed already */
                            g_slist_free (res);
                            res = NULL;
                        }
                    }

                    break;
                }
            }

            g_free (time_layout);

            /* stop seeking XML */
            break;
        }
    }
    xmlFreeDoc (doc);

    #undef XC
    #undef isElem

    return res;
}

static void
iwin_finish (SoupSession *session, SoupMessage *msg, gpointer data)
{
    WeatherInfo *info = (WeatherInfo *)data;

    g_return_if_fail (info != NULL);

    if (!SOUP_STATUS_IS_SUCCESSFUL (msg->status_code)) {
        /* forecast data is not really interesting anyway ;) */
        g_warning ("Failed to get IWIN forecast data: %d %s\n",
                   msg->status_code, msg->reason_phrase);
        request_done (info, FALSE);
        return;
    }

    if (info->forecast_type == FORECAST_LIST)
        info->forecast_list = parseForecastXml (msg->response_body->data, info);
    else
        info->forecast = formatWeatherMsg (g_strdup (msg->response_body->data));

    request_done (info, TRUE);
}

/* Get forecast into newly alloc'ed string */
void
iwin_start_open (WeatherInfo *info)
{
    gchar *url, *state, *zone;
    WeatherLocation *loc;
    SoupMessage *msg;

    g_return_if_fail (info != NULL);
    loc = info->location;
    g_return_if_fail (loc != NULL);

    if (loc->zone[0] == '-' && (info->forecast_type != FORECAST_LIST || !loc->latlon_valid))
        return;

    if (info->forecast) {
        g_free (info->forecast);
        info->forecast = NULL;
    }

    free_forecast_list (info);    

    if (info->forecast_type == FORECAST_LIST) {
        /* see the description here: http://www.weather.gov/forecasts/xml/ */
        if (loc->latlon_valid) {
            struct tm tm;
            time_t now = time (NULL);

            localtime_r (&now, &tm);

            url = g_strdup_printf ("http://www.weather.gov/forecasts/xml/sample_products/browser_interface/ndfdBrowserClientByDay.php?&lat=%.02f&lon=%.02f&format=24+hourly&startDate=%04d-%02d-%02d&numDays=7",
                       RADIANS_TO_DEGREES (loc->latitude), RADIANS_TO_DEGREES (loc->longitude), 1900 + tm.tm_year, 1 + tm.tm_mon, tm.tm_mday);

            msg = soup_message_new ("GET", url);
            g_free (url);
            soup_session_queue_message (info->session, msg, iwin_finish, info);

            info->requests_pending++;
        }

        return;
    }

    if (loc->zone[0] == ':') {
        /* Met Office Region Names */
        metoffice_start_open (info);
        return;
    } else if (loc->zone[0] == '@') {
        /* Australian BOM forecasts */
        bom_start_open (info);
        return;
    }

    /* The zone for Pittsburgh (for example) is given as PAZ021 in the locations
    ** file (the PA stands for the state pennsylvania). The url used wants the state
    ** as pa, and the zone as lower case paz021.
    */
    zone = g_ascii_strdown (loc->zone, -1);
    state = g_strndup (zone, 2);

    url = g_strdup_printf ("http://weather.noaa.gov/pub/data/forecasts/zone/%s/%s.txt", state, zone);

    g_free (zone);
    g_free (state);
    
    msg = soup_message_new ("GET", url);
    g_free (url);
    soup_session_queue_message (info->session, msg, iwin_finish, info);

    info->requests_pending++;
}
