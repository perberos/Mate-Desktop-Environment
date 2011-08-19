/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Iain Holmes <iain@prettypeople.org>
 *
 * Copyright 2002, 2003, 2004, 2005 Iain Holmes
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * 4th Februrary 2005: Christian Schaller: changed license to LGPL with 
 * permission of Iain Holmes, Ronald Bultje, Leontine Binchy (SUN), Johan Dahlin *  and Joe Marcus Clarke
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <glib/gi18n.h>
#include <mateconf/mateconf-client.h>
#include <gst/gst.h>

#include "gsr-window.h"

void gsr_quit (void);
void gsr_add_recent (gchar *filename);
GtkWidget * gsr_open_window (const char *filename);

extern void mate_media_profiles_init (MateConfClient *conf);
 
static GList *windows = NULL;

/* Also used in gsr-window.c as extern MateConfClient *mateconf_client */
MateConfClient *mateconf_client = NULL;

static gboolean
delete_event_cb (GSRWindow *window,
		 gpointer data)
{
	if (!gsr_window_is_saved (window) && !gsr_discard_confirmation_dialog (window, TRUE))
		return TRUE;

	return FALSE;
}

static void
window_destroyed (GtkWidget *window,
		  gpointer data)
{
	windows = g_list_remove (windows, window);

	if (windows == NULL) {
		gtk_main_quit ();
	}
}

void
gsr_quit (void) 
{
	GList *p;

	for (p = windows; p;) {
		GSRWindow *window = p->data;

		/* p is set here instead of in the for statement,
		   because by the time we get back to the loop,
		   p will be invalid */
		p = p->next;

		if (gsr_window_is_saved (window) || gsr_discard_confirmation_dialog (window, TRUE))
			gsr_window_close (window);
	}
}

void
gsr_add_recent (gchar *filename)
{
	GtkRecentData data;
	char *groups[] = { NULL, NULL };
	char *uri;

	memset (&data, 0, sizeof (data));

	uri = g_filename_to_uri (filename, NULL, NULL);
	if (uri == NULL)
		return;

	data.mime_type = g_content_type_guess (uri, NULL, 0, NULL);
	if (data.mime_type == NULL) {
		/* No mime-type means warnings, and it breaks when adding
		 * non-GIO supported URI schemes */
		g_free (uri);
		return;
	}

	/* It's a local file */
	data.display_name = g_filename_display_basename (data.display_name);
	groups[0] = "Totem";

	data.app_name = g_strdup (g_get_application_name ());
	data.app_exec = g_strjoin (" ", g_get_prgname (), "%u", NULL);
	data.groups = groups;
	gtk_recent_manager_add_full (gtk_recent_manager_get_default (),
				     uri, &data);

	g_free (data.display_name);
	g_free (data.mime_type);
	g_free (data.app_name);
	g_free (data.app_exec);

}

/* Also referenced from gsr-window.c */
gint gsr_sample_count = 1;

GtkWidget *
gsr_open_window (const char *filename)
{
	GtkWidget *window;
	char *utf8_name;
	char *name;

	if (filename == NULL) {
		/* Translator comment: untitled here implies that
		 * there is no active sound sample. Any newly
		 * recorded samples will be saved to disk with this
		 * name as default value. */
		if (gsr_sample_count == 1) {
			utf8_name = g_strdup (_("Untitled"));
		} else {
			utf8_name = g_strdup_printf (_("Untitled-%d"), gsr_sample_count);
		}
		name = g_filename_from_utf8 (utf8_name, -1, NULL, NULL, NULL);
		g_free (utf8_name);
		++gsr_sample_count;
	} else {
		name = g_strdup (filename);
	}

	window = GTK_WIDGET (gsr_window_new (name));
	g_free (name);

	g_signal_connect (G_OBJECT (window), "delete-event",
			  G_CALLBACK (delete_event_cb), NULL);

	g_signal_connect (G_OBJECT (window), "destroy",
			  G_CALLBACK (window_destroyed), NULL);

	windows = g_list_prepend (windows, window);
	gtk_widget_show (window);

	return window;
}

int
main (int argc,
      char **argv)
{
	gchar **filenames = NULL;
	/* this is necessary because someone apparently forgot to add a
	 * convenient way to get the remaining arguments to the MateProgram
	 * API when adding the GOption stuff to it ... */
	const GOptionEntry entries[] = {
		{ G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &filenames,
		"Special option that collects any remaining arguments for us" },
		{ NULL, }
	};

	GOptionContext *ctx;
	GError *error = NULL;

	g_thread_init (NULL);

	/* Init gettext */
	bindtextdomain (GETTEXT_PACKAGE, MATELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	ctx = g_option_context_new ("mate-sound-recorder");
	/* Initializes gtk during option parsing */
	g_option_context_add_group (ctx, gtk_get_option_group (TRUE));
	g_option_context_add_group (ctx, gst_init_get_option_group ());
	g_option_context_add_main_entries (ctx, entries, GETTEXT_PACKAGE);

	if (!g_option_context_parse (ctx, &argc, &argv, &error)) {
		g_printerr ("Option parsing failed: %s\n", error->message);
		g_error_free (error);
		g_option_context_free (ctx);
		return EXIT_FAILURE;
	}

	g_option_context_free (ctx);
	gtk_window_set_default_icon_name ("mate-sound-recorder");

	/* use it like a singleton */
	mateconf_client = mateconf_client_get_default ();

	/* init mate-media-profiles */
	mate_media_profiles_init (mateconf_client);

	if (filenames != NULL && filenames[0] != NULL) {
		guint i, num;

		num = g_strv_length (filenames);
		for (i = 0; i < num; ++i) {
			gsr_open_window (filenames[i]);
		}
	} else {
			gsr_open_window (NULL);
	}

	if (filenames) {
		g_strfreev (filenames);
	}

	gtk_main ();

	return 0;
}
