/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * mate-window-icon.c: convenience functions for window mini-icons
 *
 * Copyright 2000, 2001 Ximian, Inc.
 *
 * Author:  Jacob Berkman  <jacob@ximian.com>
 */

#include "config.h"

#include "libmate/mate-util.h"
#include "libmateui/mate-window-icon.h"

#include <gdk/gdk.h>
#include "libmateui/mate-client.h"

#include <stdlib.h>

#define MATE_DESKTOP_ICON "MATE_DESKTOP_ICON"

static GList *
list_from_char_array (const char **s)
{
	GList *list = NULL;
	int i;
	
	for (i = 0; s[i]; i++) {
		GdkPixbuf *pb;

		pb = gdk_pixbuf_new_from_file (s[i], NULL);
		if (pb)
			list = g_list_prepend (list, pb);
	}

	return list;
}

static void
free_list (GList *list)
{
	g_list_foreach (list, (GFunc) g_object_unref, NULL);
	g_list_free (list);
}

/**
 * mate_window_icon_set_from_default:
 * @w: the #GtkWidget to set the icon on
 *
 * Description: Makes the #GtkWindow @w use the default icon.
 */
void
mate_window_icon_set_from_default (GtkWindow *w)
{
}

/**
 * mate_window_icon_set_from_file_list:
 * @w: window to set icons on
 * @filenames: NULL terminated string array
 * 
 * Description: Convenience wrapper around gtk_window_set_icon_list(),
 * which loads the icons in @filenames.
 **/
void
mate_window_icon_set_from_file_list (GtkWindow *w, const char **filenames)
{
	GList *list;

	g_return_if_fail (w != NULL);
	g_return_if_fail (GTK_IS_WINDOW (w));
	g_return_if_fail (filenames != NULL);

	list = list_from_char_array (filenames);
	gtk_window_set_icon_list (w, list);
	free_list (list);
}

/**
 * mate_window_icon_set_from_file:
 * @w: the #GtkWidget to set the icon on
 * @filename: the name of the file to load
 *
 * Description: Makes the #GtkWindow @w use the icon in @filename.
 */
void
mate_window_icon_set_from_file (GtkWindow *w, const char *filename)
{
	const char *filenames[2] = { NULL };

	g_return_if_fail (w != NULL);
	g_return_if_fail (GTK_IS_WINDOW (w));
	g_return_if_fail (filename != NULL);

	filenames[0] = filename;
	mate_window_icon_set_from_file_list (w, filenames);
}

/**
 * mate_window_icon_set_default_from_file_list:
 * @filenames: NULL terminated string array
 * 
 * Description: Wrapper around gtk_window_set_default_icon_list(),
 * which loads the icons in @filenames.
 **/
void
mate_window_icon_set_default_from_file_list (const char **filenames)
{
	GList *list;

	g_return_if_fail (filenames != NULL);

	list = list_from_char_array (filenames);
	gtk_window_set_default_icon_list (list);
	free_list (list);
}

/**
 * mate_window_icon_set_default_from_file:
 * @filename: filename for the default window icon
 *
 * Description: Set the default icon to the image in @filename, if one 
 * of the mate_window_icon_set_default_from* functions have not already
 * been called.
 */
void
mate_window_icon_set_default_from_file (const char *filename)
{	
	const char *filenames[2] = { NULL };

	g_return_if_fail (filename != NULL);

	/* FIXME: the vector const is wrong */
	filenames[0] = filename;
	mate_window_icon_set_default_from_file_list (filenames);
}

/**
 * mate_window_icon_init:
 *
 * Description: Initialize the mate window icon by checking the
 * MATE_DESKTOP_ICON environment variable.  This function is 
 * automatically called by the mate_init process.
 */
void
mate_window_icon_init (void)
{
	MateClient *client;
	const char *filename;

	filename = g_getenv (MATE_DESKTOP_ICON);
        if (!filename || !filename[0])
		return;

	mate_window_icon_set_default_from_file (filename);

	/* remove it from our environment */
	mate_unsetenv (MATE_DESKTOP_ICON);

#ifndef G_OS_WIN32
	client = mate_master_client ();
	if (!MATE_CLIENT_CONNECTED (client))
		return;
	
	/* save it for restarts */
	mate_client_set_environment (client, MATE_DESKTOP_ICON, filename);
#endif
}
