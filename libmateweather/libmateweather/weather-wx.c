/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* weather-wx.c - Weather server functions (WX Radar)
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

#define MATEWEATHER_I_KNOW_THIS_IS_UNSTABLE
#include "weather.h"
#include "weather-priv.h"

static void
wx_finish (SoupSession *session, SoupMessage *msg, gpointer data)
{
    WeatherInfo *info = (WeatherInfo *)data;
    GdkPixbufAnimation *animation;

    g_return_if_fail (info != NULL);

    if (!SOUP_STATUS_IS_SUCCESSFUL (msg->status_code)) {
	g_warning ("Failed to get radar map image: %d %s.\n",
		   msg->status_code, msg->reason_phrase);
	g_object_unref (info->radar_loader);
	request_done (info, FALSE);
	return;
    }

    gdk_pixbuf_loader_close (info->radar_loader, NULL);
    animation = gdk_pixbuf_loader_get_animation (info->radar_loader);
    if (animation != NULL) {
	if (info->radar)
	    g_object_unref (info->radar);
	info->radar = animation;
	g_object_ref (info->radar);
    }
    g_object_unref (info->radar_loader);

    request_done (info, TRUE);
}

static void
wx_got_chunk (SoupMessage *msg, SoupBuffer *chunk, gpointer data)
{
    WeatherInfo *info = (WeatherInfo *)data;
    GError *error = NULL;

    g_return_if_fail (info != NULL);

    gdk_pixbuf_loader_write (info->radar_loader, (guchar *)chunk->data,
			     chunk->length, &error);
    if (error) {
	g_print ("%s \n", error->message);
	g_error_free (error);
    }
}

/* Get radar map and into newly allocated pixmap */
void
wx_start_open (WeatherInfo *info)
{
    gchar *url;
    SoupMessage *msg;
    WeatherLocation *loc;

    g_return_if_fail (info != NULL);
    info->radar = NULL;
    info->radar_loader = gdk_pixbuf_loader_new ();
    loc = info->location;
    g_return_if_fail (loc != NULL);

    if (info->radar_url)
	url = g_strdup (info->radar_url);
    else {
	if (loc->radar[0] == '-')
	    return;
	url = g_strdup_printf ("http://image.weather.com/web/radar/us_%s_closeradar_medium_usen.jpg", loc->radar);
    }
 
    msg = soup_message_new ("GET", url);
    if (!msg) {
	g_warning ("Invalid radar URL: %s\n", url);
	g_free (url);
	return;
    }

    g_signal_connect (msg, "got-chunk", G_CALLBACK (wx_got_chunk), info);
    soup_message_body_set_accumulate (msg->response_body, FALSE);
    soup_session_queue_message (info->session, msg, wx_finish, info);
    g_free (url);

    info->requests_pending++;
}
