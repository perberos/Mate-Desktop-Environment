/* 
 * Copyright (C) 2006 Bastien Nocera <hadess@hadess.net>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>

#include <bacon-video-widget.h>
#include <glib.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <glib/gi18n.h>

#include "totem-resources.h"
#include "totem-mime-types.h"

static gboolean show_mimetype = FALSE;
static gboolean g_fatal_warnings = FALSE;
static char **filenames = NULL;

static void
print_mimetypes (void)
{
	guint i;

	for (i =0; i < G_N_ELEMENTS (mime_types); i++) {
		g_print ("%s\n", mime_types[i]);
	}
}

static const GOptionEntry entries[] = {
	{"mimetype", 'm', 0, G_OPTION_ARG_NONE, &show_mimetype, "List the supported mime-types", NULL},
	{"g-fatal-warnings", 0, 0, G_OPTION_ARG_NONE, &g_fatal_warnings, "Make all warnings fatal", NULL},
	{G_OPTION_REMAINING, '\0', 0, G_OPTION_ARG_FILENAME_ARRAY, &filenames, "Audio files to play back", NULL},
	{NULL}
};

int main (int argc, char **argv)
{
	GOptionGroup *options;
	GOptionContext *context;
	GtkWidget *widget;
	BaconVideoWidget *bvw;
	GError *error = NULL;
	const char *path;

	bindtextdomain (GETTEXT_PACKAGE, MATELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	g_thread_init (NULL);
	gdk_threads_init ();

	g_set_application_name (_("Audio Preview"));
	gtk_window_set_default_icon_name ("totem");
	g_setenv("PULSE_PROP_media.role", "music", TRUE);

	context = g_option_context_new ("Plays audio passed on the standard input or the filename passed on the command-line");
	options = bacon_video_widget_get_option_group ();
	g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
	g_option_context_add_group (context, options);
	g_type_init ();

	if (g_option_context_parse (context, &argc, &argv, &error) == FALSE) {
		g_print ("couldn't parse command-line options: %s\n", error->message);
		g_error_free (error);
		return 1;
	}

	if (g_fatal_warnings) {
		GLogLevelFlags fatal_mask;

		fatal_mask = g_log_set_always_fatal (G_LOG_FATAL_MASK);
		fatal_mask |= G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL;
		g_log_set_always_fatal (fatal_mask);
	}

	if (show_mimetype == TRUE) {
		print_mimetypes ();
		return 0;
	}

	if (filenames != NULL && g_strv_length (filenames) != 1) {
		char *help;
		help = g_option_context_get_help (context, FALSE, NULL);
		g_print ("%s", help);
		g_free (help);
		return 1;
	}
	path = filenames ? filenames[0] : "fd://0";

	widget = bacon_video_widget_new (-1, -1, BVW_USE_TYPE_AUDIO, &error);
	if (widget == NULL) {
		g_print ("error creating the video widget: %s\n", error->message);
		g_error_free (error);
		return 1;
	}
	bvw = BACON_VIDEO_WIDGET (widget);

	totem_resources_monitor_start (NULL, -1);
	if (bacon_video_widget_open (bvw, path, NULL, &error) == FALSE) {
		g_print ("Can't open %s: %s\n", path, error->message);
		return 1;
	}
	if (bacon_video_widget_play (bvw, &error) == FALSE) {
		g_print ("Can't play %s: %s\n", path, error->message);
		return 1;
	}

	gtk_main ();

	return 0;
}

