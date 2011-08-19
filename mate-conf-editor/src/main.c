/*
 * Copyright (C) 2001, 2002 Anders Carlsson <andersca@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <config.h>

#include <stdlib.h>
#include <glib/gi18n.h>
#include <mateconf/mateconf.h>

#include "mateconf-editor-application.h"
#include "mateconf-stock-icons.h"
#include "mateconf-editor-window.h"

static char *
build_accel_filename (void)
{
	return g_build_filename (g_get_home_dir (), ".mate2", "accels", PACKAGE, NULL);
}

static void
load_accel_map (void)
{
	char *map;

	map = build_accel_filename ();
	gtk_accel_map_load (map);

	g_free (map);
}

static void
save_accel_map (void)
{
	char *map;

	map = build_accel_filename ();
	gtk_accel_map_save (map);

	g_free (map);
}

gint
main (gint argc, gchar **argv)
{
	GOptionContext *context;
	GtkWidget *window;
	GError *error = NULL;

	static gchar **remaining_args = NULL;
	gchar *initial_key = NULL;
		
	const GOptionEntry entries[] = 
	{
	  { G_OPTION_REMAINING, '\0', 0, G_OPTION_ARG_STRING_ARRAY, &remaining_args, NULL, N_("[KEY]") },
	  { NULL }
	};

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	context = g_option_context_new (N_("- Directly edit your entire configuration database"));

	g_option_context_set_translation_domain (context, GETTEXT_PACKAGE);
	g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
	g_option_context_add_group (context, gtk_get_option_group (TRUE));

	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		g_critical ("Failed to parse arguments: %s", error->message);
		g_error_free (error);
		g_option_context_free (context);
		exit (1);
	}

	g_option_context_free (context);

	/* Register our stock icons */
        gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (), ICONDIR);
	mateconf_stock_icons_register ();
	load_accel_map ();

        gtk_window_set_default_icon_name ("mateconf-editor");

	window = mateconf_editor_application_create_editor_window (MATECONF_EDITOR_WINDOW_TYPE_NORMAL);
	gtk_widget_show_now (window);

	/* get the key specified on the command line if any. Ignore the rest */
	initial_key = remaining_args != NULL ? remaining_args[0] : NULL;

	if (initial_key != NULL)
		mateconf_editor_window_go_to (MATECONF_EDITOR_WINDOW (window),initial_key);
	
	gtk_main ();

	save_accel_map ();
	g_strfreev (remaining_args);

	return 0;
}
