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

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "libmatekbd/matekbd-keyboard-drawing.h"


static gchar *groups = NULL;
static gchar *levels = NULL;
static gchar *symbols = NULL;
static gchar *keycodes = NULL;
static gchar *geometry = NULL;

static gboolean track_config = FALSE;
static gboolean track_modifiers = FALSE;
static gboolean program_version = FALSE;

static const GOptionEntry options[] = {
	{"groups", '\0', 0, G_OPTION_ARG_STRING, &groups,
	 "Keyboard groups to display, from 1-4. Up to four groups only may be "
	 "displayed. Examples: --groups=3 or --groups=1,2,1,2",
	 "group1[,group2[,group3[,group4]]]"},
	{"levels", '\0', 0, G_OPTION_ARG_STRING, &levels,
	 "Keyboard shift levels to display, from 1-64. Up to four shift levels "
	 "only may be displayed. Examples: --levels=3 or --levels=1,2,1,2",
	 "level1[,level2[,level3[,level4]]]"},
	{"symbols", '\0', 0, G_OPTION_ARG_STRING, &symbols,
	 "Symbols component of the keyboard. If you omit this option, it is "
	 "obtained from the X server; that is, the keyboard that is currently "
	 "configured is drawn. Examples: --symbols=us or "
	 "--symbols=us(pc104)+iso9995-3+group(switch)+ctrl(nocaps)", NULL},
	{"keycodes", '\0', 0, G_OPTION_ARG_STRING, &keycodes,
	 "Keycodes component of the keyboard. If you omit this option, it is "
	 "obtained from the X server; that is, the keyboard that is currently"
	 " configured is drawn. Examples: --keycodes=xfree86+aliases(qwerty)",
	 NULL},
	{"geometry", '\0', 0, G_OPTION_ARG_STRING, &geometry,
	 "Geometry xkb component. If you omit this option, it is obtained from the"
	 " X server; that is, the keyboard that is currently configured is drawn. "
	 "Example: --geometry=kinesis", NULL},
	{"track-modifiers", '\0', 0, G_OPTION_ARG_NONE, &track_modifiers,
	 "Track the current modifiers", NULL},
	{"track-config", '\0', 0, G_OPTION_ARG_NONE, &track_config,
	 "Track the server XKB configuration", NULL},
	{"version", '\0', 0, G_OPTION_ARG_NONE, &program_version,
	 "Show current version", NULL},
	{NULL},
};

static gboolean
set_groups (gchar * groups_option,
	    MatekbdKeyboardDrawingGroupLevel * groupLevels)
{
	MatekbdKeyboardDrawingGroupLevel *pgl = groupLevels;
	gint cntr, g;

	groupLevels[0].group =
	    groupLevels[1].group =
	    groupLevels[2].group = groupLevels[3].group = -1;

	if (groups_option == NULL)
		return TRUE;

	for (cntr = 4; --cntr >= 0;) {
		if (*groups_option == '\0')
			return FALSE;

		g = *groups_option - '1';
		if (g < 0 || g >= 4)
			return FALSE;

		pgl->group = g;
		/* printf ("group %d\n", pgl->group); */

		groups_option++;
		if (*groups_option == '\0')
			return TRUE;
		if (*groups_option != ',')
			return FALSE;

		groups_option++;
		pgl++;
	}

	return TRUE;
}

static gboolean
set_levels (gchar * levels_option,
	    MatekbdKeyboardDrawingGroupLevel * groupLevels)
{
	MatekbdKeyboardDrawingGroupLevel *pgl = groupLevels;
	gint cntr, l;
	gchar *p;

	groupLevels[0].level =
	    groupLevels[1].level =
	    groupLevels[2].level = groupLevels[3].level = -1;

	if (levels_option == NULL)
		return TRUE;

	for (cntr = 4; --cntr >= 0;) {
		if (*levels_option == '\0')
			return FALSE;

		l = (gint) strtol (levels_option, &p, 10) - 1;
		if (l < 0 || l >= 64)
			return FALSE;

		pgl->level = l;
		/* printf ("level %d\n", pgl->level); */

		levels_option = p;
		if (*levels_option == '\0')
			return TRUE;
		if (*levels_option != ',')
			return FALSE;

		levels_option++;
		pgl++;
	}

	return TRUE;
}

static void
bad_keycode (MatekbdKeyboardDrawing * drawing, guint keycode)
{
	g_warning
	    ("got keycode %u, which is not on your keyboard according to your configuration",
	     keycode);
}

gint
main (gint argc, gchar ** argv)
{
	GtkWidget *window;
	GtkWidget *matekbd_keyboard_drawing;
	GdkScreen *screen;
	gint monitor;
	GdkRectangle rect;
	GOptionContext *context;

	MatekbdKeyboardDrawingGroupLevel groupLevels[4] =
	    { {0, 0}, {1, 0}, {0, 1}, {1, 1} };
	MatekbdKeyboardDrawingGroupLevel *pgroupLevels[4] =
	    { &groupLevels[0], &groupLevels[1], &groupLevels[2],
		&groupLevels[3]
	};

	context = g_option_context_new ("kbdraw");
	g_option_context_add_main_entries (context, options, NULL);
	g_option_context_parse (context, &argc, &argv, NULL);
	g_option_context_free (context);

	if (program_version) {
		g_print ("kbdraw %s\n", VERSION);
		exit (0);
	}

	gtk_init (&argc, &argv);

	if (!set_groups (groups, groupLevels)) {
		g_printerr ("--groups: invalid argument\n");
		exit (1);
	}

	if (!set_levels (levels, groupLevels)) {
		g_printerr ("--levels: invalid argument\n");
		exit (1);
	}

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	g_signal_connect (G_OBJECT (window), "destroy",
			  G_CALLBACK (gtk_main_quit), NULL);

	screen = gtk_window_get_screen (GTK_WINDOW (window));
	monitor = gdk_screen_get_monitor_at_point (screen, 0, 0);
	gdk_screen_get_monitor_geometry (screen, monitor, &rect);
	gtk_window_set_default_size (GTK_WINDOW (window),
				     rect.width * 4 / 5,
				     rect.height * 1 / 2);

	gtk_widget_show (window);

	matekbd_keyboard_drawing = matekbd_keyboard_drawing_new ();
	gtk_widget_show (matekbd_keyboard_drawing);
	gtk_container_add (GTK_CONTAINER (window), matekbd_keyboard_drawing);

	matekbd_keyboard_drawing_set_groups_levels (MATEKBD_KEYBOARD_DRAWING
						 (matekbd_keyboard_drawing),
						 pgroupLevels);

	if (track_modifiers)
		matekbd_keyboard_drawing_set_track_modifiers
		    (MATEKBD_KEYBOARD_DRAWING (matekbd_keyboard_drawing), TRUE);
	if (track_config)
		matekbd_keyboard_drawing_set_track_config
		    (MATEKBD_KEYBOARD_DRAWING (matekbd_keyboard_drawing), TRUE);
	g_signal_connect (G_OBJECT (matekbd_keyboard_drawing), "bad-keycode",
			  G_CALLBACK (bad_keycode), NULL);

	if (symbols || geometry || keycodes) {
		XkbComponentNamesRec names;
		gint success;

		memset (&names, '\0', sizeof (names));

		if (symbols)
			names.symbols = symbols;
		else
			names.symbols = (gchar *)
			    matekbd_keyboard_drawing_get_symbols
			    (MATEKBD_KEYBOARD_DRAWING
			     (matekbd_keyboard_drawing));

		if (keycodes)
			names.keycodes = keycodes;
		else
			names.keycodes = (gchar *)
			    matekbd_keyboard_drawing_get_keycodes
			    (MATEKBD_KEYBOARD_DRAWING
			     (matekbd_keyboard_drawing));

		if (geometry)
			names.geometry = geometry;
		else
			names.geometry = (gchar *)
			    matekbd_keyboard_drawing_get_geometry
			    (MATEKBD_KEYBOARD_DRAWING
			     (matekbd_keyboard_drawing));

		success =
		    matekbd_keyboard_drawing_set_keyboard
		    (MATEKBD_KEYBOARD_DRAWING (matekbd_keyboard_drawing),
		     &names);
		if (!success) {
			g_printerr
			    ("\nError loading new keyboard description with components:\n\n"
			     "  keycodes:  %s\n" "  types:     %s\n"
			     "  compat:    %s\n" "  symbols:   %s\n"
			     "  geometry:  %s\n\n", names.keycodes,
			     names.types, names.compat, names.symbols,
			     names.geometry);
			exit (1);
		}
	}

	gtk_widget_grab_focus (matekbd_keyboard_drawing);

	gtk_main ();

	return 0;
}
