/* MATE Volume Control
 * Copyright (C) 2003-2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * main.c: intialization, window setup
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
#include "config.h"
#endif

#include <getopt.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gst/gst.h>
#include <gst/audio/mixerutils.h>

#include "keys.h"
#include "window.h"

static gchar* page = NULL;
static GOptionEntry entries[] =
{
  { "page", 'p', 0, G_OPTION_ARG_STRING, &page, N_("Startup page"), "playback|recording|switches|options" }
};

/*
 * Probe for mixer elements. Set up GList * with elements,
 * where each element has a GObject data node set of the
 * name "mate-volume-control-name" with the value being
 * the human-readable name of the element.
 *
 * All elements in the returned GList * are in state
 * GST_STATE_NULL.
 */

static gboolean
mixer_filter_func (GstMixer * mixer, gpointer user_data)
{
  GstElementFactory *factory;
  const gchar *long_name;
  gchar *devname = NULL;
  gchar *name;
  gint *p_count = (gint *) user_data;

  /* fetch name */
  if (g_object_class_find_property (G_OBJECT_GET_CLASS (G_OBJECT (mixer)),
                                    "device-name")) {
    g_object_get (mixer, "device-name", &devname, NULL);
    GST_DEBUG ("device name: %s", GST_STR_NULL (devname));
  } else {
    devname = NULL;
    GST_DEBUG ("device name unknown, no 'device-name' property");
  }
    
  factory = gst_element_get_factory (GST_ELEMENT (mixer));
  long_name = gst_element_factory_get_longname (factory);

  if (devname) {
    name = g_strdup_printf ("%s (%s)", devname, long_name);
    g_free (devname);
  } else {
    gchar *title;

    *p_count += 1;

    title = g_strdup_printf (_("Unknown Volume Control %d"),  *p_count);
    name = g_strdup_printf ("%s (%s)", title, long_name);
    g_free (title);
  }

  g_object_set_data_full (G_OBJECT (mixer),
                          "mate-volume-control-name",
                          name,
                          (GDestroyNotify) g_free);

  GST_DEBUG ("Adding '%s' to list of available mixers", name);

  gst_element_set_state (GST_ELEMENT (mixer), GST_STATE_NULL);

  return TRUE; /* add mixer to list */
}

static GList *
create_mixer_collection (void)
{
  GList *mixer_list;
  gint counter = 0;

  mixer_list = gst_audio_default_registry_mixer_filter (mixer_filter_func,
                                                        FALSE,
                                                        &counter);

  return mixer_list;
}

static void
cb_destroy (GtkWidget *widget,
	    gpointer   data)
{
  gtk_main_quit ();
}

static void
cb_check_resize (GtkContainer    *container,
      		  gpointer         user_data)
{
  MateConfClient *client;
  gint width, height;

  client = mateconf_client_get_default();
  gtk_window_get_size (GTK_WINDOW (container), &width, &height);
  mateconf_client_set_int (client, PREF_UI_WINDOW_WIDTH, width, NULL);
  mateconf_client_set_int (client, PREF_UI_WINDOW_HEIGHT, height, NULL);
}

gint
main (gint   argc,
      gchar *argv[])
{
  GOptionContext *ctx;
  GtkWidget *win;
  GList *elements;

  /* i18n */
  bindtextdomain (GETTEXT_PACKAGE, MATELOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  g_thread_init (NULL);
  ctx = g_option_context_new ("mate-volume-control");
  g_option_context_add_main_entries(ctx, entries, GETTEXT_PACKAGE);
  g_option_context_add_group (ctx, gst_init_get_option_group ());
  g_option_context_parse(ctx, &argc, &argv, NULL);
  g_option_context_free(ctx);

  gtk_init (&argc, &argv);

  gtk_window_set_default_icon_name ("multimedia-volume-control");

  if (!(elements = create_mixer_collection ())) {
    win = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_ERROR,
				  GTK_BUTTONS_CLOSE,
				  _("No volume control GStreamer plugins and/or devices found."));
    gtk_widget_show (win);
    gtk_dialog_run (GTK_DIALOG (win));
    gtk_widget_destroy (win);
    return -1;
  }

  /* window contains everything automagically */
  win = mate_volume_control_window_new (elements);
  if (page != NULL)
    mate_volume_control_window_set_page(win, page);
  g_signal_connect (win, "destroy", G_CALLBACK (cb_destroy), NULL);
  g_signal_connect (win, "check_resize", G_CALLBACK (cb_check_resize), NULL);

  gtk_widget_show (win);
  gtk_main ();

  return 0;
}
