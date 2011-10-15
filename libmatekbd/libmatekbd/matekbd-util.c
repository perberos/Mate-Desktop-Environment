/*
 * Copyright (C) 2006 Sergey V. Udaltsov <svu@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include <matekbd-util.h>

#include <time.h>

#include <glib/gi18n.h>

#include <libxklavier/xklavier.h>

#include <mateconf/mateconf-client.h>

#include <matekbd-config-private.h>

static void
matekbd_log_appender (const char file[], const char function[],
		   int level, const char format[], va_list args)
{
	time_t now = time (NULL);
	g_log (NULL, G_LOG_LEVEL_DEBUG, "[%08ld,%03d,%s:%s/] \t",
	       (long) now, level, file, function);
	g_logv (NULL, G_LOG_LEVEL_DEBUG, format, args);
}

void
matekbd_install_glib_log_appender (void)
{
	xkl_set_log_appender (matekbd_log_appender);
}

#define MATEKBD_PREVIEW_CONFIG_KEY_PREFIX  MATEKBD_CONFIG_KEY_PREFIX "/preview"

const gchar MATEKBD_PREVIEW_CONFIG_DIR[] = MATEKBD_PREVIEW_CONFIG_KEY_PREFIX;
const gchar MATEKBD_PREVIEW_CONFIG_KEY_X[] =
    MATEKBD_PREVIEW_CONFIG_KEY_PREFIX "/x";
const gchar MATEKBD_PREVIEW_CONFIG_KEY_Y[] =
    MATEKBD_PREVIEW_CONFIG_KEY_PREFIX "/y";
const gchar MATEKBD_PREVIEW_CONFIG_KEY_WIDTH[] =
    MATEKBD_PREVIEW_CONFIG_KEY_PREFIX "/width";
const gchar MATEKBD_PREVIEW_CONFIG_KEY_HEIGHT[] =
    MATEKBD_PREVIEW_CONFIG_KEY_PREFIX "/height";

GdkRectangle *
matekbd_preview_load_position (void)
{
	GError *gerror = NULL;
	GdkRectangle *rv = NULL;
	gint x, y, w, h;
	MateConfClient *conf_client = mateconf_client_get_default ();

	if (conf_client == NULL)
		return NULL;

	x = mateconf_client_get_int (conf_client,
				  MATEKBD_PREVIEW_CONFIG_KEY_X, &gerror);
	if (gerror != NULL) {
		xkl_debug (0, "Error getting the preview x: %s\n",
			   gerror->message);
		g_error_free (gerror);
		g_object_unref (G_OBJECT (conf_client));
		return NULL;
	}

	y = mateconf_client_get_int (conf_client,
				  MATEKBD_PREVIEW_CONFIG_KEY_Y, &gerror);
	if (gerror != NULL) {
		xkl_debug (0, "Error getting the preview y: %s\n",
			   gerror->message);
		g_error_free (gerror);
		g_object_unref (G_OBJECT (conf_client));
		return NULL;
	}

	w = mateconf_client_get_int (conf_client,
				  MATEKBD_PREVIEW_CONFIG_KEY_WIDTH, &gerror);
	if (gerror != NULL) {
		xkl_debug (0, "Error getting the preview width: %s\n",
			   gerror->message);
		g_error_free (gerror);
		g_object_unref (G_OBJECT (conf_client));
		return NULL;
	}

	h = mateconf_client_get_int (conf_client,
				  MATEKBD_PREVIEW_CONFIG_KEY_HEIGHT, &gerror);
	if (gerror != NULL) {
		xkl_debug (0, "Error getting the preview height: %s\n",
			   gerror->message);
		g_error_free (gerror);
		g_object_unref (G_OBJECT (conf_client));
		return NULL;
	}

	g_object_unref (G_OBJECT (conf_client));

	rv = g_new (GdkRectangle, 1);
	if (x == -1 || y == -1 || w == -1 || h == -1) {
		/* default values should be treated as
		 * "0.75 of the screen size" */
		GdkScreen *scr = gdk_screen_get_default ();
		gint w = gdk_screen_get_width (scr);
		gint h = gdk_screen_get_height (scr);
		rv->x = w >> 3;
		rv->y = h >> 3;
		rv->width = w - (w >> 2);
		rv->height = h - (h >> 2);
	} else {
		rv->x = x;
		rv->y = y;
		rv->width = w;
		rv->height = h;
	}
	return rv;
}

void
matekbd_preview_save_position (GdkRectangle * rect)
{
	MateConfClient *conf_client = mateconf_client_get_default ();
	MateConfChangeSet *cs;
	GError *gerror = NULL;

	cs = mateconf_change_set_new ();

	mateconf_change_set_set_int (cs, MATEKBD_PREVIEW_CONFIG_KEY_X, rect->x);
	mateconf_change_set_set_int (cs, MATEKBD_PREVIEW_CONFIG_KEY_Y, rect->y);
	mateconf_change_set_set_int (cs, MATEKBD_PREVIEW_CONFIG_KEY_WIDTH,
				  rect->width);
	mateconf_change_set_set_int (cs, MATEKBD_PREVIEW_CONFIG_KEY_HEIGHT,
				  rect->height);

	mateconf_client_commit_change_set (conf_client, cs, TRUE, &gerror);
	if (gerror != NULL) {
		g_warning ("Error saving preview configuration: %s\n",
			   gerror->message);
		g_error_free (gerror);
	}
	mateconf_change_set_unref (cs);
	g_object_unref (G_OBJECT (conf_client));
}
