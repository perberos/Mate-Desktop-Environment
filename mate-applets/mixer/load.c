/* MATE Volume Applet
 * Copyright (C) 2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * load.c: applet boilerplate code
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

#include <string.h>

#include <glib.h>
#include <gst/audio/mixerutils.h>

#include "applet.h"

typedef struct _FilterHelper {
   GList *names_list;
   gint count;
} FilterHelper;


static gboolean
_filter_func (GstMixer *mixer, gpointer data) {
   GstElementFactory *factory;
   const gchar *longname;
   FilterHelper *helper = (FilterHelper *)data;
   gchar *device = NULL;
   gchar *name, *original;

   /* fetch name */
   if (g_object_class_find_property (G_OBJECT_GET_CLASS (G_OBJECT (mixer)), "device-name")) {
      g_object_get (mixer, "device-name", &device, NULL);
      GST_DEBUG ("device-name: %s", GST_STR_NULL (device));
   } else {
      GST_DEBUG ("no 'device-name' property: device name unknown");
   }

   factory = gst_element_get_factory (GST_ELEMENT (mixer));
   longname = gst_element_factory_get_longname (factory);

   if (device) {
      GList *list;
      gint duplicates = 0;

      name = g_strdup_printf ("%s (%s)", device, longname);
      g_free (device);

      /* Devices are sorted by names, we must ensure that the names are unique */
      for (list = helper->names_list; list != NULL; list = list->next) {
         if (strcmp ((const gchar *) list->data, name) == 0)
            duplicates++;
      }

      if (duplicates > 0) {
         /*
          * There is a duplicate name, so append an index to make the name
          * unique within the list
          */
          original = name;
          name = g_strdup_printf("%s #%d", name, duplicates + 1);
      } else {
         original = g_strdup(name);
      }
   } else {
      gchar *title;

      (helper->count)++;

      title = g_strdup_printf (_("Unknown Volume Control %d"), helper->count);
      name = g_strdup_printf ("%s (%s)", title, longname);
      original = g_strdup(name);
      g_free (title);
   }

   helper->names_list = g_list_prepend (helper->names_list, name);

   GST_DEBUG ("Adding '%s' to the list of available mixers", name);

   g_object_set_data_full (G_OBJECT (mixer),
                           "mate-volume-applet-name",
                           name,
                           (GDestroyNotify) g_free);

   /* Is this even used anywhere? */
   g_object_set_data_full (G_OBJECT(mixer),
                           "mate-volume-applet-origname",
                           original,
                           (GDestroyNotify) g_free);

   name = NULL; /* no need to free, passed ownership to object data */
   original = NULL; /* no need to free, passed ownership to object data */

   gst_element_set_state (GST_ELEMENT (mixer), GST_STATE_NULL);

   return TRUE;
}

GList *
mate_volume_applet_create_mixer_collection (void)
{
   FilterHelper helper;
   GList *mixer_list;

   helper.count = 0;
   helper.names_list = NULL;

   mixer_list = gst_audio_default_registry_mixer_filter(_filter_func, FALSE, &helper);
   g_list_free (helper.names_list);

   return mixer_list;
}

static gboolean
mate_volume_applet_toplevel_configure_handler (GtkWidget *widget,
						GdkEventConfigure *event,
						gpointer data)
{
  GList *elements;
  static gboolean init = FALSE;

  g_signal_handlers_disconnect_by_func (widget,
				        mate_volume_applet_toplevel_configure_handler,
				        data);

  if (!init) {
    gst_init (NULL, NULL);
    init = TRUE;
  }

  elements = mate_volume_applet_create_mixer_collection ();
  mate_volume_applet_setup (MATE_VOLUME_APPLET (data), elements);

  return FALSE;
}

static gboolean
mate_volume_applet_factory (MatePanelApplet *applet,
			     const gchar *iid,
			     gpointer     data)
{
  /* we delay applet specific initialization until the applet 
   * is fully registered with the panel since gst_init() can block
   * for longer than the service activation timeouts
   *
   * We use configure-event as a hook because after the applet is 
   * registered with b-a-s and the panel, the panel sets size hints
   * on the applet and gives it an initial size.
   *
   * http://bugzilla.mate.org/show_bug.cgi?id=385305
   */
  g_signal_connect (gtk_widget_get_toplevel (GTK_WIDGET (applet)),
		    "configure-event",
		    G_CALLBACK (mate_volume_applet_toplevel_configure_handler),
		    applet);
  return TRUE;
}

MATE_PANEL_APPLET_OUT_PROCESS_FACTORY (
  "MixerAppletFactory",
  MATE_TYPE_VOLUME_APPLET,
  "mixer_applet2",
  mate_volume_applet_factory,
  NULL
)
