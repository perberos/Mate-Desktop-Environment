/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* weather-bom.c - Australian Bureau of Meteorology forecast source
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

#include <string.h>

#define MATEWEATHER_I_KNOW_THIS_IS_UNSTABLE
#include "weather.h"
#include "weather-priv.h"

static gchar *
bom_parse (const gchar *meto)
{
    gchar *p, *rp;

    g_return_val_if_fail (meto != NULL, NULL);

    p = strstr (meto, "<pre>");
    g_return_val_if_fail (p != NULL, NULL);

    rp = strstr (p, "</pre>");
    g_return_val_if_fail (rp !=NULL, NULL);

    p += 5; /* skip the <pre> */

    return g_strndup (p, rp-p);
}

static void
bom_finish (SoupSession *session, SoupMessage *msg, gpointer data)
{
    WeatherInfo *info = (WeatherInfo *)data;

    g_return_if_fail (info != NULL);

    if (!SOUP_STATUS_IS_SUCCESSFUL (msg->status_code)) {
        g_warning ("Failed to get BOM forecast data: %d %s.\n",
		   msg->status_code, msg->reason_phrase);
        request_done (info, FALSE);
	return;
    }

    info->forecast = bom_parse (msg->response_body->data);
    request_done (info, TRUE);
}

void
bom_start_open (WeatherInfo *info)
{
    gchar *url;
    SoupMessage *msg;
    WeatherLocation *loc;

    loc = info->location;

    url = g_strdup_printf ("http://www.bom.gov.au/cgi-bin/wrap_fwo.pl?%s.txt",
			   loc->zone + 1);

    msg = soup_message_new ("GET", url);
    soup_session_queue_message (info->session, msg, bom_finish, info);
    g_free (url);

    info->requests_pending++;
}
