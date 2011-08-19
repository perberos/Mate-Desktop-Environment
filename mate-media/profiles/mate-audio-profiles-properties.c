/* mate-audio-profiles-properties.c:
   properties capplet that shows the GapProfilesEdit dialog */

/*
 * Copyright (C) 2003 Thomas Vander Stichele
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <libintl.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "mate-media-profiles.h"
#include "audio-profile-private.h"

static void
on_dialog_destroy (GtkWidget *dialog, gpointer *user_data)
{
  /* dialog destroyed, time to bail */
  gtk_main_quit ();
}

int
main (int argc, char *argv[])
{
  GtkWidget *widget;
  MateConfClient *conf;
  GOptionContext *context;
  GError *error = NULL;

  g_thread_init (NULL);

  context = g_option_context_new (NULL);
  g_option_context_add_group (context, gtk_get_option_group (TRUE));
  if (g_option_context_parse (context, &argc, &argv, &error) == FALSE) {
	  g_print (_("%s\nRun '%s --help' to see a full list of available command line options.\n"),
		   error->message, argv[0]);
	  g_error_free (error);
	  g_option_context_free (context);
	  exit (1);
  }
  g_option_context_free (context);

  conf = mateconf_client_get_default ();
  textdomain (GETTEXT_PACKAGE);

  mate_media_profiles_init (conf);

  gtk_window_set_default_icon_name ("mate-mime-audio");

  widget = GTK_WIDGET (gm_audio_profiles_edit_new (conf, NULL));
  g_assert (GTK_IS_WIDGET (widget));

  g_signal_connect (G_OBJECT (widget), "destroy",
                    G_CALLBACK (on_dialog_destroy), NULL);

  gtk_widget_show_all (widget);
  gtk_main ();

  g_object_unref (conf);

  return 0;
}
