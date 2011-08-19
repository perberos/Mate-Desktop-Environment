/* Eye Of Mate - Debugging
 *
 * Copyright (C) 2007 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@mate.org>
 *
 * Based on gedit code (gedit/gedit-debug.h) by:
 * 	- Alex Roberts <bse@error.fsnet.co.uk>
 *	- Evan Lawrence <evan@worldpath.net>
 *	- Chema Celorio <chema@celorio.com>
 *	- Paolo Maggi <paolo@mate.org>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include "eog-debug.h"

#define ENABLE_PROFILING

#ifdef ENABLE_PROFILING
static GTimer *timer = NULL;
static gdouble last = 0.0;
#endif

static EogDebugSection debug = EOG_NO_DEBUG;

void
eog_debug_init (void)
{
	if (g_getenv ("EOG_DEBUG") != NULL)
	{
		/* Enable all debugging */
		debug = ~EOG_NO_DEBUG;
		goto out;
	}

	if (g_getenv ("EOG_DEBUG_WINDOW") != NULL)
		debug = debug | EOG_DEBUG_WINDOW;

	if (g_getenv ("EOG_DEBUG_VIEW") != NULL)
		debug = debug | EOG_DEBUG_VIEW;

	if (g_getenv ("EOG_DEBUG_JOBS") != NULL)
		debug = debug | EOG_DEBUG_JOBS;

	if (g_getenv ("EOG_DEBUG_THUMBNAIL") != NULL)
		debug = debug | EOG_DEBUG_THUMBNAIL;

	if (g_getenv ("EOG_DEBUG_IMAGE_DATA") != NULL)
		debug = debug | EOG_DEBUG_IMAGE_DATA;

	if (g_getenv ("EOG_DEBUG_IMAGE_LOAD") != NULL)
		debug = debug | EOG_DEBUG_IMAGE_LOAD;

	if (g_getenv ("EOG_DEBUG_IMAGE_SAVE") != NULL)
		debug = debug | EOG_DEBUG_IMAGE_SAVE;

	if (g_getenv ("EOG_DEBUG_LIST_STORE") != NULL)
		debug = debug | EOG_DEBUG_LIST_STORE;

	if (g_getenv ("EOG_DEBUG_PREFERENCES") != NULL)
		debug = debug | EOG_DEBUG_PREFERENCES;

	if (g_getenv ("EOG_DEBUG_PRINTING") != NULL)
		debug = debug | EOG_DEBUG_PRINTING;

	if (g_getenv ("EOG_DEBUG_LCMS") != NULL)
		debug = debug | EOG_DEBUG_LCMS;

	if (g_getenv ("EOG_DEBUG_PLUGINS") != NULL)
		debug = debug | EOG_DEBUG_PLUGINS;

out:

#ifdef ENABLE_PROFILING
	if (debug != EOG_NO_DEBUG)
		timer = g_timer_new ();
#endif
	return;
}

void
eog_debug_message (EogDebugSection   section,
		   const gchar      *file,
		   gint              line,
		   const gchar      *function,
		   const gchar      *format, ...)
{
	if (G_UNLIKELY (debug & section))
	{
#ifdef ENABLE_PROFILING
		gdouble seconds;
#endif

		va_list args;
		gchar *msg;

		g_return_if_fail (format != NULL);

		va_start (args, format);
		msg = g_strdup_vprintf (format, args);
		va_end (args);

#ifdef ENABLE_PROFILING
		g_return_if_fail (timer != NULL);

		seconds = g_timer_elapsed (timer, NULL);
		g_print ("[%f (%f)] %s:%d (%s) %s\n",
			 seconds, seconds - last,  file, line, function, msg);
		last = seconds;
#else
		g_print ("%s:%d (%s) %s\n", file, line, function, msg);
#endif

		fflush (stdout);

		g_free (msg);
	}
}

void eog_debug (EogDebugSection  section,
		  const gchar       *file,
		  gint               line,
		  const gchar       *function)
{
	if (G_UNLIKELY (debug & section))
	{
#ifdef ENABLE_PROFILING
		gdouble seconds;

		g_return_if_fail (timer != NULL);

		seconds = g_timer_elapsed (timer, NULL);
		g_print ("[%f (%f)] %s:%d (%s)\n",
			 seconds, seconds - last, file, line, function);
		last = seconds;
#else
		g_print ("%s:%d (%s)\n", file, line, function);
#endif
		fflush (stdout);
	}
}
