/* mate-audio-profiles-test.c: */

/*
 * Copyright (C) 2003 Thomas Vander Stichele
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <gtk/gtk.h>
#include <mateconf/mateconf-client.h>
#include <gst/gst.h>

#include <profiles/mate-media-profiles.h>

static void
edit_clicked_cb (GtkButton *button, GtkWindow *window)
{
  GtkWidget *edit_dialog = NULL;
  edit_dialog = gm_audio_profiles_edit_new (mateconf_client_get_default (), window);
  g_assert (edit_dialog != NULL);
  gtk_widget_show_all (GTK_WIDGET (edit_dialog));
}

static void
test_clicked_cb (GtkButton *button, GtkWidget *combo)
{
  GstStateChangeReturn ret;
  gchar *partialpipe = NULL;
  gchar *extension = NULL;
  gchar *pipeline_desc;
  GError *error = NULL;
  GMAudioProfile *profile;
  GstElement *pipeline = NULL;
  GstMessage *msg = NULL;
  GstBus *bus = NULL;

  profile = gm_audio_profile_choose_get_active (combo);
  g_return_if_fail (profile != NULL);

  gtk_widget_set_sensitive (GTK_WIDGET (button), FALSE);

  extension = g_strdup (gm_audio_profile_get_extension (profile));
  partialpipe = g_strdup (gm_audio_profile_get_pipeline (profile));

  g_print ("You chose profile with name %s and pipeline %s\n",
           gm_audio_profile_get_name (profile),
           gm_audio_profile_get_pipeline (profile));

  pipeline_desc = g_strdup_printf ("audiotestsrc wave=sine num-buffers=4096 "
                                   " ! audioconvert "
                                   " ! %s "
                                   " ! filesink location=test.%s",
                                   partialpipe, extension);

  g_print ("Going to run pipeline %s\n", pipeline_desc);

  pipeline = gst_parse_launch (pipeline_desc, &error);
  if (error)
  {
    g_warning ("Error parsing pipeline: %s", error->message);
    goto done;
  }

  bus = gst_element_get_bus (pipeline);

  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  /* wait for state change to complete or to have failed */
  ret = gst_element_get_state (pipeline, NULL, NULL, -1);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    /* check if an error was posted on the bus */
    if ((msg = gst_bus_poll (bus, GST_MESSAGE_ERROR, 0))) {
      gst_message_parse_error (msg, &error, NULL);
    }

    g_warning ("Error starting pipeline: %s",
        (error) ? error->message : "UNKNOWN ERROR");

    goto done;
  }

  g_print ("Writing test sound to test.%s ...\n", extension);

  /* wait for it finish (error or EOS), but no more than 30 secs */
  msg = gst_bus_poll (bus, GST_MESSAGE_ERROR | GST_MESSAGE_EOS, 30*GST_SECOND);

  if (msg) {
    switch (GST_MESSAGE_TYPE (msg)) {
      case GST_MESSAGE_EOS:
        g_print ("Test finished successfully.\n");
        break;
      case GST_MESSAGE_ERROR:
        gst_message_parse_error (msg, &error, NULL);
        g_warning ("Error starting pipeline: %s",
            (error) ? error->message : "UNKNOWN ERROR");
        break;
      default:
        g_assert_not_reached ();
    }
  } else {
    g_warning ("Test did not finish within 30 seconds!\n");
  }

done:

  g_print ("==============================================================\n");

  if (error)
    g_error_free (error);

  if (pipeline) {
    gst_element_set_state (pipeline, GST_STATE_NULL);
    gst_object_unref (pipeline);
  }

  if (msg)
    gst_message_unref (msg);

  if (bus)
    gst_object_unref (bus);

  g_free (pipeline_desc);
  g_free (partialpipe);
  g_free (extension);

  gtk_widget_set_sensitive (GTK_WIDGET (button), TRUE);
}

int
main (int argc, char **argv)
{
  GtkWidget *window, *hbox, *combo, *edit, *test;
  MateConfClient *mateconf;
  GOptionContext *context;
  GError *error = NULL;

  g_thread_init (NULL);
  context = g_option_context_new (NULL);
  g_option_context_add_group (context, gst_init_get_option_group ());
  g_option_context_add_group (context, gtk_get_option_group (TRUE));
  if (g_option_context_parse (context, &argc, &argv, &error) == FALSE) {
	  g_print ("%s\nRun '%s --help' to see a full list of available command line options.\n",
		   error->message, argv[0]);
	  g_error_free (error);
	  g_option_context_free (context);
	  exit (1);
  }
  g_option_context_free (context);

  mateconf = mateconf_client_get_default ();
  mate_media_profiles_init (mateconf);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  combo = gm_audio_profile_choose_new ();

  edit = gtk_button_new_with_mnemonic ("_Edit Profiles");
  test = gtk_button_new_with_mnemonic ("_Test");
  g_signal_connect (edit, "clicked", (GCallback) edit_clicked_cb, window);
  g_signal_connect (test, "clicked", (GCallback) test_clicked_cb, combo);
  g_signal_connect (edit, "destroy", (GCallback) gtk_main_quit, NULL);

  hbox = gtk_hbox_new (FALSE, 7);
  gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), test, FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), edit, FALSE, TRUE, 0);
  gtk_container_add (GTK_CONTAINER (window), hbox);
  gtk_widget_show_all (window);
  gtk_main ();

  return 0;
}
