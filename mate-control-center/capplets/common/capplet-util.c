/* -*- mode: c; style: linux -*- */

/* capplet-util.c
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Written by Bradford Hovinen <hovinen@ximian.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <ctype.h>

/* For stat */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <glib/gi18n.h>
#include <stdlib.h>

#include "capplet-util.h"

static void
capplet_error_dialog (GtkWindow *parent, char const *msg, GError *err)
{
	if (err != NULL) {
		GtkWidget *dialog;

		dialog = gtk_message_dialog_new (GTK_WINDOW (parent),
			GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_ERROR,
			GTK_BUTTONS_CLOSE,
			msg, err->message);

		g_signal_connect (G_OBJECT (dialog),
			"response",
			G_CALLBACK (gtk_widget_destroy), NULL);
		gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
		gtk_widget_show (dialog);
		g_error_free (err);
	}
}

/**
 * capplet_help :
 * @parent :
 * @helpfile :
 * @section  :
 *
 * A quick utility routine to display help for capplets, and handle errors in a
 * Havoc happy way.
 **/
void
capplet_help (GtkWindow *parent, char const *section)
{
	GError *error = NULL;
	char *uri;
	GdkScreen *screen;

	g_return_if_fail (section != NULL);

	if (!parent)
		screen = gdk_screen_get_default ();
	else
		screen = gtk_widget_get_screen (GTK_WIDGET (parent));

	uri = g_strdup_printf ("ghelp:user-guide#%s", section);

	if (!gtk_show_uri (screen, uri, gtk_get_current_event_time (), &error)) {
		capplet_error_dialog (
			parent,
			_("There was an error displaying help: %s"),
			error);
	}

	g_free (uri);
}

/**
 * capplet_set_icon :
 * @window :
 * @file_name  :
 *
 * A quick utility routine to avoid the cut-n-paste of bogus code
 * that caused several bugs.
 **/
void
capplet_set_icon (GtkWidget *window, char const *icon_file_name)
{
	/* Make sure that every window gets an icon */
	gtk_window_set_default_icon_name (icon_file_name);
	gtk_window_set_icon_name (GTK_WINDOW (window), icon_file_name);
}

static gboolean
directory_delete_recursive (GFile *directory, GError **error)
{
	GFileEnumerator *enumerator;
	GFileInfo *info;
	gboolean success = TRUE;

	enumerator = g_file_enumerate_children (directory,
						G_FILE_ATTRIBUTE_STANDARD_NAME ","
						G_FILE_ATTRIBUTE_STANDARD_TYPE,
						G_FILE_QUERY_INFO_NONE,
						NULL, error);
	if (enumerator == NULL)
		return FALSE;

	while (success &&
	       (info = g_file_enumerator_next_file (enumerator, NULL, NULL))) {
		GFile *child;

		child = g_file_get_child (directory, g_file_info_get_name (info));

		if (g_file_info_get_file_type (info) == G_FILE_TYPE_DIRECTORY) {
			success = directory_delete_recursive (child, error);
		} else {
			success = g_file_delete (child, NULL, error);
		}
		g_object_unref (info);
	}
	g_file_enumerator_close (enumerator, NULL, NULL);

	if (success)
		success = g_file_delete (directory, NULL, error);

	return success;
}

/**
 * capplet_file_delete_recursive :
 * @file :
 * @error  :
 *
 * A utility routine to delete files and/or directories,
 * including non-empty directories.
 **/
gboolean
capplet_file_delete_recursive (GFile *file, GError **error)
{
	GFileInfo *info;
	GFileType type;

	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	info = g_file_query_info (file,
				  G_FILE_ATTRIBUTE_STANDARD_TYPE,
				  G_FILE_QUERY_INFO_NONE,
				  NULL, error);
	if (info == NULL)
		return FALSE;

	type = g_file_info_get_file_type (info);
	g_object_unref (info);

	if (type == G_FILE_TYPE_DIRECTORY)
		return directory_delete_recursive (file, error);
	else
		return g_file_delete (file, NULL, error);
}

void
capplet_init (GOptionContext *context,
	      int *argc,
	      char ***argv)
{
	GError *err = NULL;

#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, MATELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	if (context) {
#if GLIB_CHECK_VERSION (2, 12, 0)
		g_option_context_set_translation_domain (context, GETTEXT_PACKAGE);
#endif
		g_option_context_add_group (context, gtk_get_option_group (TRUE));

		if (!g_option_context_parse (context, argc, argv, &err)) {
			g_printerr ("%s\n", err->message);
			exit (1);
		}
	}

	gtk_init (argc, argv);
}
