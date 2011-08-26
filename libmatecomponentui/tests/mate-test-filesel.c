/*
 * test-libfilesel.c: a small test program for libmatefilesel
 *
 * Authors:
 *    Jacob Berkman  <jacob@ximian.com>
 *
 * Copyright 2001 Ximian, Inc.
 *
 */

#include <config.h>
#include <stdlib.h>
#include <matecomponent/matecomponent-file-selector-util.h>
#include <matecomponent/matecomponent-ui-main.h>

#include <gtk/gtk.h>

#include <glib/gi18n.h>

static gint
get_files (gpointer data)
{
	char *s, **strv;
	int i;

	s = matecomponent_file_selector_open (NULL, FALSE, NULL, NULL, "/etc");
	g_print ("open test:\n\t%s\n", s);
	g_free (s);

	s = matecomponent_file_selector_save (NULL, FALSE, NULL, NULL, "/tmp", "test.txt");
	g_print ("save test:\n\t%s\n", s);
	g_free (s);

	strv = matecomponent_file_selector_open_multi (NULL, TRUE, NULL, NULL, NULL);
	g_print ("open multi test:\n");
	if (strv) {
		for (i = 0; strv[i]; i++)
			g_print ("\t%s\n", strv[i]);

		g_strfreev (strv);
	}

	matecomponent_main_quit ();

	return FALSE;
}

int
main (int argc, char *argv[])
{
	MateProgram *program;

	free (malloc (8));

	textdomain (GETTEXT_PACKAGE);

	program = mate_program_init ("mate-test-filesel", VERSION,
			    LIBMATECOMPONENTUI_MODULE,
			    argc, argv, NULL);

	matecomponent_activate ();

	g_idle_add (get_files, NULL);

	matecomponent_main ();

	g_object_unref (program);

	return 0;
}
