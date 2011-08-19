/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * mate-window.c: wrappers for setting window properties
 *
 * Copyright 2000, Chema Celorio
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 *
 * Authors:  Chema Celorio <chema@celorio.com>
 */


#include <config.h>
#include <glib.h>
#include <string.h>
#include <libmate/mate-program.h>
#include <gtk/gtk.h>

#include "mate-window.h"
#include "mate-ui-init.h"

/**
 * mate_window_toplevel_set_title:
 * @window: A pointer to the toplevel window.
 * @doc_name: The window-specific name.
 * @app_name: The application name.
 * @extension: The default extension that the application uses. %NULL if there
 * is not a default extension.
 * 
 * Set the title of a toplevel window. This is done by appending the
 * window-specific name (less the extension, if any) to the application name.
 * So it appears as "<appname> : <docname>".
 *
 * This parameters to this function are appopriate in that it identifies
 * application windows as containing certain files that are being worked on at
 * the time (for example, a word processor file, a spreadsheet or an image).
 **/
void
mate_window_toplevel_set_title (GtkWindow *window, const gchar *doc_name,
				 const gchar *app_name, const gchar *extension)
{
    gchar *full_title;
    gchar *doc_name_clean = NULL;
	
    g_return_if_fail (GTK_IS_WINDOW (window));
    g_return_if_fail (doc_name != NULL);
    g_return_if_fail (app_name != NULL);

    /* Remove the extension from the doc_name*/
    if (extension != NULL) {
	gchar * pos = strstr (doc_name, extension);
	if (pos != NULL)
	    doc_name_clean = g_strndup (doc_name, pos - doc_name);
    }

    if (!doc_name_clean)
	doc_name_clean = g_strdup (doc_name);
	
    full_title = g_strdup_printf ("%s : %s", doc_name_clean, app_name);
    gtk_window_set_title (window, full_title);

    g_free (doc_name_clean);
    g_free (full_title);
}
