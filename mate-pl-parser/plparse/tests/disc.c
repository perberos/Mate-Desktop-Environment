/* Small test app for disc concent detection
 * (c) 2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301  USA.
 */

#include "config.h"

#include <locale.h>
#include <glib.h>
#include <gio/gio.h>

#include "totem-disc.h"

static void
test_disc (gconstpointer data)
{
	TotemDiscMediaType type;
	GError *error = NULL;
	char *mrl = NULL;
	const char *device = (const char*) data;

	type = totem_cd_detect_type_with_url (device, &mrl, &error);

	if (type == MEDIA_TYPE_ERROR) {
		GList *or, *list;
		GVolumeMonitor *mon;

		g_message ("Error getting media type: %s", error ? error->message : "unknown reason");
		g_message ("List of connected drives:");

		mon = g_volume_monitor_get ();
		for (or = list = g_volume_monitor_get_connected_drives (mon); list != NULL; list = list->next) {
			char *device;
			device = g_drive_get_identifier ((GDrive *) list->data, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
			g_message ("\t%s", device);
			g_free (device);
			g_object_unref (list->data);
		}
		if (or == NULL)
			g_message ("\tNo connected drives!");
		else
			g_list_free (or);

		g_message ("List of volumes:");
		for (or = list = g_volume_monitor_get_volumes (mon); list != NULL; list = list->next) {
			char *device;

			device = g_volume_get_identifier ((GVolume *) list->data, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE);
			if (g_volume_get_mount ((GVolume *) list->data) == NULL)
				g_message ("\t%s", device);
			else
				g_message ("\t%s (mounted)", device);
			g_free (device);
			g_object_unref (list->data);
		}

		if (or == NULL)
			g_message ("\tNo volumes!");
		else
			g_list_free (or);

		return;
	}

	if (type != MEDIA_TYPE_DATA)
		g_message ("%s contains a %s.", device, totem_cd_get_human_readable_name (type));
	else
		g_message ("%s contains a data disc", device);
	g_assert (mrl != NULL);
	g_message ("MRL for directory is \"%s\".", mrl);

	g_free (mrl);

	return;
}

static void
log_handler (const char *log_domain, GLogLevelFlags log_level, const char *message, gpointer user_data)
{
	g_test_message ("%s", message);
}

int
main (int argc, char *argv[])
{
	GError *error = NULL;
	GOptionContext *context;
	gchar **device_paths;
	guint i = 0;
	const GOptionEntry entries[] = {
		{ G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &device_paths, "[Device paths...]", NULL },
		{ NULL }
	};

	setlocale (LC_ALL, "");

	g_type_init ();
	g_test_init (&argc, &argv, NULL);
	g_test_bug_base ("http://bugzilla.mate.org/show_bug.cgi?id=");

	/* Parse our own command-line options */
	context = g_option_context_new ("- test disc functions");
	g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);

	if (g_option_context_parse (context, &argc, &argv, &error) == FALSE) {
		g_print ("Option parsing failed: %s\n", error->message);
		return 1;
	}

	if (device_paths == NULL) {
		/* We need to handle log messages produced by g_message so they're interpreted correctly by the GTester framework */
		g_log_set_handler (NULL, G_LOG_LEVEL_MESSAGE | G_LOG_LEVEL_INFO | G_LOG_LEVEL_DEBUG, log_handler, NULL);

		/* NOTE: A disc needs to be in /dev/dvd for this test to do anything useful. Not really any way to work around this. */
		g_test_add_data_func ("/disc/exists", "/dev/dvd", test_disc);
		g_test_add_data_func ("/disc/not-exists", "/this/does/not/exist", test_disc);

		return g_test_run ();
	}

	/* If we're passed device paths, test them instead of running the GTests */
	while (device_paths[i] != NULL)
		test_disc (device_paths[i++]);

	return 0;
}
