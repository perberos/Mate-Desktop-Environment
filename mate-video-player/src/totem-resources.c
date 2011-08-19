/* 
 * Copyright (C) 2007 Bastien Nocera <hadess@hadess.net>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
 *
 * The Totem project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and Totem. This
 * permission are above and beyond the permissions granted by the GPL license
 * Totem is covered by.
 *
 * Monday 7th February 2005: Christian Schaller: Add exception clause.
 * See license_change file for details.
 *
 */

#include "config.h"

#include <glib.h>
#include <glib/gstdio.h>

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>

#include "totem-resources.h"

#define MAX_HELPER_MEMORY (256 * 1024 * 1024)	/* 256 MB */
#define MAX_HELPER_SECONDS (15)			/* 15 seconds */
#define DEFAULT_SLEEP_TIME (30 * G_USEC_PER_SEC) /* 30 seconds */

static guint sleep_time = DEFAULT_SLEEP_TIME;
static gboolean finished = TRUE;

static void
set_resource_limits (const char *input)
{
	struct rlimit limit;
	struct stat buf;
	rlim_t max;

	max = MAX_HELPER_MEMORY;

	/* Set the maximum virtual size depending on the size
	 * of the file to process, as we wouldn't be able to
	 * mmap it otherwise */
	if (input == NULL) {
		max = MAX_HELPER_MEMORY;
	} else if (g_stat (input, &buf) == 0) {
		max = MAX_HELPER_MEMORY + buf.st_size;
	} else if (g_str_has_prefix (input, "file://") != FALSE) {
		char *file;
		file = g_filename_from_uri (input, NULL, NULL);
		if (file != NULL && g_stat (file, &buf) == 0)
			max = MAX_HELPER_MEMORY + buf.st_size;
		g_free (file);
	}

	limit.rlim_cur = max;
	limit.rlim_max = max;

	setrlimit (RLIMIT_DATA, &limit);

	limit.rlim_cur = MAX_HELPER_SECONDS;
	limit.rlim_max = MAX_HELPER_SECONDS;
	setrlimit (RLIMIT_CPU, &limit);
}

G_GNUC_NORETURN static gpointer
time_monitor (gpointer data)
{
	const char *app_name;

	g_usleep (sleep_time);

	if (finished != FALSE)
		g_thread_exit (NULL);

	app_name = g_get_application_name ();
	if (app_name == NULL)
		app_name = g_get_prgname ();
	g_print ("%s couldn't process file: '%s'\n"
		 "Reason: Took too much time to process.\n",
		 app_name,
		 (const char *) data);

	exit (0);
}

void
totem_resources_monitor_start (const char *input, gint wall_clock_time)
{
	set_resource_limits (input);

	if (wall_clock_time < 0)
		return;

	if (wall_clock_time > 0)
		sleep_time = wall_clock_time;

	finished = FALSE;
	g_thread_create (time_monitor, (gpointer) input, FALSE, NULL);
}

void
totem_resources_monitor_stop (void)
{
	finished = TRUE;
}

