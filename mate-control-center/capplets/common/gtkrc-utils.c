/*
 * Copyright (C) 2007 The MATE Foundation
 * Written by Thomas Wood <thos@gnome.org>
 *            Jens Granseuer <jensgr@gmx.net>
 * All Rights Reserved
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <unistd.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <fcntl.h>
#include <gtk/gtk.h>

#define INCLUDE_SYMBOL ((gpointer) 1)
#define ENGINE_SYMBOL ((gpointer) 2)
#define COLOR_SCHEME_SYMBOL ((gpointer) 3)

gchar* gtkrc_find_named(const gchar* name)
{
	/* find the gtkrc of the named theme
	 * taken from gtkrc.c (gtk_rc_parse_named)
	 */
	gchar* path = NULL;
	const gchar* home_dir;
	const gchar* subpath = "gtk-2.0" G_DIR_SEPARATOR_S "gtkrc";

	/* First look in the users home directory
	*/
	home_dir = g_get_home_dir();
	
	if (home_dir)
	{
		path = g_build_filename(home_dir, ".themes", name, subpath, NULL);
		
		if (!g_file_test (path, G_FILE_TEST_EXISTS))
		{
			g_free (path);
			path = NULL;
		}
	}

	if (!path)
	{
		gchar* theme_dir = gtk_rc_get_theme_dir();
		path = g_build_filename(theme_dir, name, subpath, NULL);
		g_free(theme_dir);

		if (!g_file_test(path, G_FILE_TEST_EXISTS))
		{
			g_free (path);
			path = NULL;
		}
	}

	return path;

}

void gtkrc_get_details(gchar* filename, GSList** engines, GSList** symbolic_colors)
{
	gint file = -1;
	GSList* files = NULL;
	GSList* read_files = NULL;
	GTokenType token;
	GScanner *scanner = g_scanner_new (NULL);

	g_scanner_scope_add_symbol (scanner, 0, "include", INCLUDE_SYMBOL);

	if (engines != NULL)
	{
		g_scanner_scope_add_symbol (scanner, 0, "engine", ENGINE_SYMBOL);
	}

	files = g_slist_prepend (files, g_strdup (filename));
	
	while (files != NULL)
	{
		filename = files->data;
		files = g_slist_delete_link (files, files);

		if (filename == NULL)
			continue;

		if (g_slist_find_custom (read_files, filename, (GCompareFunc) strcmp))
		{
			g_warning ("Recursion in the gtkrc detected!");
			g_free (filename);
			continue; /* skip this file since we've done it before... */
		}

		read_files = g_slist_prepend (read_files, filename);

		file = g_open (filename, O_RDONLY);
		if (file == -1)
		{
			g_warning ("Could not open file \"%s\"", filename);
		}
		else
		{
			g_scanner_input_file (scanner, file);
			while ((token = g_scanner_get_next_token (scanner)) != G_TOKEN_EOF)
			{
				GTokenType string_token;
				if (token == '@')
				{
					if (symbolic_colors == NULL)
						continue;
					token = g_scanner_get_next_token (scanner);
					if (token != G_TOKEN_IDENTIFIER)
						continue;
					if (!g_slist_find_custom (*symbolic_colors, scanner->value.v_identifier, (GCompareFunc) strcmp))
						*symbolic_colors = g_slist_append (*symbolic_colors, g_strdup (scanner->value.v_identifier));
					continue;
				}

				if (token != G_TOKEN_SYMBOL)
					continue;

				if (scanner->value.v_symbol == INCLUDE_SYMBOL)
				{
					string_token = g_scanner_get_next_token (scanner);
					if (string_token != G_TOKEN_STRING)
						continue;
					if (g_path_is_absolute (scanner->value.v_string))
					{
						files = g_slist_prepend (files, g_strdup (scanner->value.v_string));
					}
					else
					{
						gchar *basedir = g_path_get_dirname (filename);
						files = g_slist_prepend (files, g_build_path (G_DIR_SEPARATOR_S, basedir, scanner->value.v_string, NULL));
						g_free (basedir);
					}
				}
				else if (scanner->value.v_symbol == ENGINE_SYMBOL)
				{
					string_token = g_scanner_get_next_token (scanner);
					if (string_token != G_TOKEN_STRING || scanner->value.v_string[0] == '\0')
						continue;
					if (!g_slist_find_custom (*engines, scanner->value.v_string, (GCompareFunc) strcmp))
						*engines = g_slist_append (*engines, g_strdup (scanner->value.v_string));
				}

			}
			close (file);
		}
	}

	g_slist_foreach (read_files, (GFunc) g_free, NULL);
	g_slist_free (read_files);

	g_scanner_destroy (scanner);
}


gchar *
gtkrc_get_color_scheme (const gchar *gtkrc_file)
{
	gint file = -1;
	gchar *result = NULL;
	GSList *files = NULL;
	GSList *read_files = NULL;
	GTokenType token;
	GScanner *scanner = gtk_rc_scanner_new ();

	g_scanner_scope_add_symbol (scanner, 0, "include", INCLUDE_SYMBOL);
	g_scanner_scope_add_symbol (scanner, 0, "gtk_color_scheme", COLOR_SCHEME_SYMBOL);
	g_scanner_scope_add_symbol (scanner, 0, "gtk-color-scheme", COLOR_SCHEME_SYMBOL);

	files = g_slist_prepend (files, g_strdup (gtkrc_file));
	while (files != NULL)
	{
		gchar *filename = files->data;
		files = g_slist_delete_link (files, files);

		if (filename == NULL)
			continue;

		if (g_slist_find_custom (read_files, filename, (GCompareFunc) strcmp))
		{
			g_warning ("Recursion in the gtkrc detected!");
			g_free (filename);
			continue; /* skip this file since we've done it before... */
		}

		read_files = g_slist_prepend (read_files, filename);

		file = g_open (filename, O_RDONLY);
		if (file == -1)
		{
			g_warning ("Could not open file \"%s\"", filename);
		}
		else
		{
			g_scanner_input_file (scanner, file);
			while ((token = g_scanner_get_next_token (scanner)) != G_TOKEN_EOF)
			{
				if (GINT_TO_POINTER (token) == COLOR_SCHEME_SYMBOL)
				{
					if (g_scanner_get_next_token (scanner) == '=')
					{
						token = g_scanner_get_next_token (scanner);
						if (token == G_TOKEN_STRING)
						{
							g_free (result);
							result = g_strdup (scanner->value.v_string);
						}
					}
				}
			}
			close (file);
		}
	}

	g_slist_foreach (read_files, (GFunc) g_free, NULL);
	g_slist_free (read_files);

	g_scanner_destroy (scanner);
	return result;
}

gchar* gtkrc_get_color_scheme_for_theme(const gchar* theme_name)
{
	/* try to find the color scheme from the gtkrc */
	gchar* gtkrc_file;
	gchar* scheme = NULL;

	gtkrc_file = gtkrc_find_named(theme_name);

	if (gtkrc_file)
	{
		scheme = gtkrc_get_color_scheme(gtkrc_file);
		g_free(gtkrc_file);
	}

	return scheme;
}
