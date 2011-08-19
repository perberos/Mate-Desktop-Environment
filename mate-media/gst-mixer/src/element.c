/* MATE Volume Control
 * Copyright (C) 2003-2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * element.c: widget representation of a single mixer element
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
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "element.h"
#include "keys.h"
#include "preferences.h"
#include "track.h"
#include "misc.h"
#ifdef HAVE_SOUND_THEME
#include "gvc-sound-theme-chooser.h"
#endif

G_DEFINE_TYPE (MateVolumeControlElement, mate_volume_control_element, GTK_TYPE_NOTEBOOK)

static void	mate_volume_control_element_class_init	(MateVolumeControlElementClass *klass);
static void	mate_volume_control_element_init	(MateVolumeControlElement *el);
static void	mate_volume_control_element_dispose	(GObject *object);

static void	cb_mateconf			(MateConfClient     *client,
						 guint            connection_id,
						 MateConfEntry      *entry,
						 gpointer         data);


static void
mate_volume_control_element_class_init (MateVolumeControlElementClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = mate_volume_control_element_dispose;
}

static void
mate_volume_control_element_init (MateVolumeControlElement *el)
{
  el->client = NULL;
  el->mixer = NULL;
}

GtkWidget *
mate_volume_control_element_new (MateConfClient  *client)
{
  MateVolumeControlElement *el;

  /* element */
  el = g_object_new (MATE_VOLUME_CONTROL_TYPE_ELEMENT, NULL);
  el->client = g_object_ref (G_OBJECT (client));

  mateconf_client_add_dir (el->client, MATE_VOLUME_CONTROL_KEY_DIR,
			MATECONF_CLIENT_PRELOAD_RECURSIVE, NULL);
  mateconf_client_notify_add (el->client, MATE_VOLUME_CONTROL_KEY_DIR,
			   cb_mateconf, el, NULL, NULL);

  return GTK_WIDGET (el);
}

static void
mate_volume_control_element_dispose (GObject *object)
{
  MateVolumeControlElement *el = MATE_VOLUME_CONTROL_ELEMENT (object);

  if (el->client) {
    g_object_unref (G_OBJECT (el->client));
    el->client = NULL;
  }

  if (el->mixer) {
    /* remove g_timeout_add() mainloop handlers */
    mate_volume_control_element_change (el, NULL);
    gst_element_set_state (GST_ELEMENT (el->mixer), GST_STATE_NULL);
    gst_object_unref (GST_OBJECT (el->mixer));
    el->mixer = NULL;
  }

  G_OBJECT_CLASS (mate_volume_control_element_parent_class)->dispose (object);
}

/*
 * Checks if we want to show the track by default ("whitelist").
 */

gboolean
mate_volume_control_element_whitelist (GstMixer *mixer,
					GstMixerTrack *track)
{
  gint i, pos;
  gboolean found = FALSE;

  /* Yes this is a hack. */
  static struct {
    gchar *label;
    gboolean done;
  } list[] = {

/* Translator comment: the names below are a whitelist for which volume
 * controls to show by default. Make sure that those match the translations
 * of GStreamer-plugins' ALSA/OSS plugins. */
    { "cd", FALSE },
    { "line", FALSE },
    { "mic", FALSE },
    { "pcm", FALSE },
    { "headphone", FALSE },
    { "speaker", FALSE },
    { "volume", FALSE },
    { "master", FALSE },
    { "digital output", FALSE },
    { "recording", FALSE },
    { "front", FALSE },
    { NULL, FALSE }
  };
    
  /*
   * When the user changes devices, it is necessary to reset the whitelist
   * to a good default state.  This fixes bugs LP:345645, 576022
   */
  if (track == NULL)
  {
    for (i = 0; list[i].label != NULL; i++)
      list[i].done = FALSE;
    return TRUE;
  }

  /* honor the mixer supplied hints about whitelisting if available */
  if (gst_mixer_get_mixer_flags (GST_MIXER (mixer)) & GST_MIXER_FLAG_HAS_WHITELIST) {
    if (GST_MIXER_TRACK_HAS_FLAG (track, GST_MIXER_TRACK_WHITELIST)) {
      return (TRUE);
    } else {
      return (FALSE);
    }
  }

  for (i = 0; !found && list[i].label != NULL; i++) {
    gchar *label_l = NULL;

    if (list[i].done)
      continue;

    /* make case insensitive */
    if (g_object_class_find_property (G_OBJECT_GET_CLASS (track), "untranslated-label"))
      g_object_get (track, "untranslated-label", &label_l, NULL);
    if (label_l == NULL)
      g_object_get (track, "label", &label_l, NULL);
    for (pos = 0; label_l[pos] != '\0'; pos++)
      label_l[pos] = g_ascii_tolower (label_l[pos]);

    if (g_strrstr (label_l, list[i].label) != NULL) {
      found = TRUE;
      list[i].done = TRUE;
    }
    g_free (label_l);
  }

  return found;
}

/*
 * Hide/show notebook page.
 */

static void
update_tab_visibility (MateVolumeControlElement *el, gint page, gint tabnum)
{
  const GList *item;
  gboolean visible = FALSE;
  GtkWidget *t;

  for (item = gst_mixer_list_tracks (el->mixer);
       item != NULL; item = item->next) {
    GstMixerTrack *track = item->data;
    MateVolumeControlTrack *trkw =
        g_object_get_data (G_OBJECT (track), "mate-volume-control-trkw");

    if (get_page_num (el->mixer, track) == page && trkw->visible) {
      visible = TRUE;
      break;
    }
  }

  t = gtk_notebook_get_nth_page (GTK_NOTEBOOK (el), tabnum);
  if (visible)
    gtk_widget_show (t);
  else
    gtk_widget_hide (t);
}

static void
cb_notify_message (GstBus *bus, GstMessage *message, gpointer data)
{
  MateVolumeControlElement *el = data;
  GstMixerMessageType type;
  MateVolumeControlTrack *trkw;
  GstMixerTrack *track = NULL;
  GstMixerOptions *options = NULL;

  if (GST_MESSAGE_SRC (message) != GST_OBJECT (el->mixer)) {
    /* not from our mixer - can't update anything anyway */
    return;
  }

  /* This code only calls refresh if the first_track changes, because the
   * refresh code only retrieves the current value from that track anyway */
  type = gst_mixer_message_get_type (message);
  if (type == GST_MIXER_MESSAGE_MUTE_TOGGLED) {
    gst_mixer_message_parse_mute_toggled (message, &track, NULL);
  } else if (type == GST_MIXER_MESSAGE_VOLUME_CHANGED) {
    gst_mixer_message_parse_volume_changed (message, &track, NULL, NULL);
  } else if (type == GST_MIXER_MESSAGE_OPTION_CHANGED) {
    gst_mixer_message_parse_option_changed (message, &options, NULL);
    track = GST_MIXER_TRACK (options);
  } else {
    return;
  }

  trkw = g_object_get_data (G_OBJECT (track),
			    "mate-volume-control-trkw");

  mate_volume_control_track_update (trkw);
}

/*
 * Change the element. Basically recreates this object internally.
 */

void
mate_volume_control_element_change (MateVolumeControlElement *el,
				     GstElement *element)
{
  struct {
    GtkWidget *page, *old_sep, *new_sep, *flagbuttonbox;
    gboolean use;
    gint pos, height, width;
    MateVolumeControlTrack * (* get_track_widget) (GtkTable      *table,
						    gint           tab_pos,
						    GstMixer      *mixer,
						    GstMixerTrack *track,
						    GtkWidget     *left_sep,
						    GtkWidget     *right_sep,
						    GtkWidget     *flagbox);

  } content[4] = {
    { NULL, NULL, NULL, NULL, FALSE, 0, 5, 1,
      mate_volume_control_track_add_playback },
    { NULL, NULL, NULL, NULL, FALSE, 0, 5, 1,
      mate_volume_control_track_add_recording },
    { NULL, NULL, NULL, NULL, FALSE, 0, 1, 3,
      mate_volume_control_track_add_playback },
    { NULL, NULL, NULL, NULL, FALSE, 0, 1, 3,
      mate_volume_control_track_add_option }
  };
  static gboolean theme_page = FALSE;
  const GList *item;
  GstMixer *mixer;
  GstBus *bus;
  gint i;

  /* remove old pages, but not the "Sound Theme" page */
  i = 0;
  if (theme_page)
    i = 1;

  while (gtk_notebook_get_n_pages (GTK_NOTEBOOK (el)) > i) {
    gtk_notebook_remove_page (GTK_NOTEBOOK (el), 0);
  }

  /* take/put reference */
  if (el->mixer) {
    for (item = gst_mixer_list_tracks (el->mixer);
	 item != NULL; item = item->next) {
      GstMixerTrack *track = item->data;
      MateVolumeControlTrack *trkw;

      trkw = g_object_get_data (G_OBJECT (track),
				"mate-volume-control-trkw");
      g_object_set_data (G_OBJECT (track), "mate-volume-control-trkw", NULL);
      mate_volume_control_track_free (trkw);
    }
  }
  if (!element)
    return;

  g_return_if_fail (GST_IS_MIXER (element));
  mixer = GST_MIXER (element);
  gst_object_replace ((GstObject **) &el->mixer, GST_OBJECT (element));

  /* Bus for notifications */
  if (GST_ELEMENT_BUS (mixer) == NULL) {
    bus = gst_bus_new ();
    gst_bus_add_signal_watch (bus);
    g_signal_connect (G_OBJECT (bus), "message::element",
		      (GCallback) cb_notify_message, el);
    gst_element_set_bus (GST_ELEMENT (mixer), bus);
  }

  /* content pages */
  for (i = 0; i < 4; i++) {
    content[i].page = gtk_table_new (content[i].width, content[i].height, FALSE);
    gtk_container_set_border_width (GTK_CONTAINER (content[i].page), 6);
    if (i >= 2)
      gtk_table_set_row_spacings (GTK_TABLE (content[i].page), 6);
    gtk_table_set_col_spacings (GTK_TABLE (content[i].page), 6);
    content[i].flagbuttonbox = NULL;
  }

  /* show */
  mate_volume_control_element_whitelist (el->mixer, NULL);
  for (item = gst_mixer_list_tracks (el->mixer);
       item != NULL; item = item->next) {
    GstMixerTrack *track = item->data;
    MateVolumeControlTrack *trkw;
    gchar *key;
    const MateConfValue *value;
    gboolean active;

    i = get_page_num (el->mixer, track);

    /* FIXME:
     * - do not create separator if there is no more track
     *     _of this type_. We currently destroy it at the
     *     end, so it's not critical, but not nice either.
     */
    if (i == 3) {
      content[i].new_sep = gtk_hseparator_new ();
    } else if (i < 2) {
      content[i].new_sep = gtk_vseparator_new ();
    } else {
      content[i].new_sep = NULL;
    }

    /* visible? */
    active = mate_volume_control_element_whitelist (mixer, track);
    key = get_mateconf_key (el->mixer, track);
    if ((value = mateconf_client_get (el->client, key, NULL)) != NULL &&
        value->type == MATECONF_VALUE_BOOL) {
      active = mateconf_value_get_bool (value);
    }
    g_free (key);

    /* Show left separator if we're not the first track */
    if (active && content[i].use && content[i].old_sep) {

      /* Do not show separator for switches/options on Playback/Recording tab */
      if (i < 2 && track->num_channels != 0) {
        gtk_widget_show (content[i].old_sep);
      }
    }

    /* widget */
    trkw = content[i].get_track_widget (GTK_TABLE (content[i].page),
					content[i].pos++, el->mixer, track,
					content[i].old_sep, content[i].new_sep,
					content[i].flagbuttonbox);
    mate_volume_control_track_show (trkw, active);

    /* Only the first trkw on the page will return flagbuttonbox */
    if (trkw->flagbuttonbox != NULL)
       content[i].flagbuttonbox = trkw->flagbuttonbox;
    g_object_set_data (G_OBJECT (track),
		       "mate-volume-control-trkw", trkw);

    /* separator */
    if (item->next != NULL && content[i].new_sep) {
      if (i >= 2) {
        gtk_table_attach (GTK_TABLE (content[i].page), content[i].new_sep,
			  0, 3, content[i].pos, content[i].pos + 1,
			  GTK_SHRINK | GTK_FILL, 0, 0, 0);
      } else {
        gtk_table_attach (GTK_TABLE (content[i].page), content[i].new_sep,
			  content[i].pos, content[i].pos + 1, 0, 6,
			  0, GTK_SHRINK | GTK_FILL, 0, 0);
      }
      content[i].pos++;
    }

    content[i].old_sep = content[i].new_sep;

    if (active) {
      content[i].use = TRUE;
    }
  }

  /* show - need to build the tabs backwards so that deleting the "Sound Theme"
   * page can be avoided.
   */
  for (i = 3; i >= 0; i--) {
    GtkWidget *label, *view, *viewport;
    GtkAdjustment *hadjustment, *vadjustment;

    /* don't show last separator */
    if (content[i].new_sep)
      gtk_widget_destroy (content[i].new_sep);

    /* viewport for lots of tracks */
    view = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (view),
				    i >= 2 ? GTK_POLICY_NEVER :
					     GTK_POLICY_AUTOMATIC,
				    i >= 2 ? GTK_POLICY_AUTOMATIC :
					     GTK_POLICY_NEVER);

    hadjustment = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (view));
    vadjustment = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (view));
    viewport = gtk_viewport_new (hadjustment, vadjustment);
    gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport), GTK_SHADOW_NONE);

    if (content[i].flagbuttonbox != NULL) {
       GtkWidget *vbox      = NULL;
       GtkWidget *hbox      = NULL;
       GtkWidget *hbox2     = NULL;
       GtkWidget *separator = NULL;

       if (i < 2) {
          vbox      = gtk_vbox_new (FALSE, 0);
          hbox      = gtk_hbox_new (FALSE, 6);
          hbox2     = gtk_hbox_new (FALSE, 6);
          separator = gtk_hseparator_new ();
          gtk_box_pack_start (GTK_BOX (vbox), content[i].page, TRUE, TRUE, 6);
          gtk_box_pack_start (GTK_BOX (vbox), hbox2, FALSE, FALSE, 6);
          gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 6);
          gtk_box_pack_start (GTK_BOX (hbox2), separator, TRUE, TRUE, 6);
          gtk_box_pack_start (GTK_BOX (hbox), content[i].flagbuttonbox, TRUE,
                              FALSE, 6);
       } else {
          /* orientation is rotated for these ... */
          vbox      = gtk_hbox_new (FALSE, 0);
          hbox      = gtk_vbox_new (FALSE, 0);
          hbox2     = gtk_vbox_new (FALSE, 0);
          gtk_box_pack_start (GTK_BOX (vbox), content[i].page, FALSE, FALSE, 6);
          gtk_box_pack_start (GTK_BOX (vbox), hbox2, FALSE, FALSE, 6);
          gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 6);
          gtk_box_pack_start (GTK_BOX (hbox), content[i].flagbuttonbox, TRUE,
                              FALSE, 6);
       }
       gtk_widget_show_all (hbox2);
       gtk_widget_show (content[i].flagbuttonbox);
       gtk_widget_show (hbox);
       gtk_widget_show (content[i].page);
       gtk_widget_show (vbox);

       gtk_container_add (GTK_CONTAINER (viewport), vbox);
       gtk_container_add (GTK_CONTAINER (view), viewport);
    } else {
       gtk_container_add (GTK_CONTAINER (viewport), content[i].page);
       gtk_container_add (GTK_CONTAINER (view), viewport);
    }

    label = gtk_label_new (get_page_description (i));
    gtk_notebook_prepend_page (GTK_NOTEBOOK (el), view, label);
    gtk_widget_show (content[i].page);
    gtk_widget_show (viewport);
    gtk_widget_show (view);
    gtk_widget_show (label);

    update_tab_visibility (el, i, 0);
  }

  /* refresh fix */
  for (i = gtk_notebook_get_n_pages (GTK_NOTEBOOK (el)) - 1;
       i >= 0; i--) {
    gtk_notebook_set_current_page (GTK_NOTEBOOK (el), i);
  }

#ifdef HAVE_SOUND_THEME
  /* Add tab for managing themes */
  if (!theme_page) {
    theme_page = TRUE;
    GtkWidget *label, *view, *viewport, *sound_theme_chooser, *vbox;
    GtkAdjustment *hadjustment, *vadjustment;

    label = gtk_label_new (_("Sound Theme"));

    view = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (view),
				    GTK_POLICY_AUTOMATIC,
				    GTK_POLICY_AUTOMATIC);

    hadjustment = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (view));
    vadjustment = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (view));
    viewport = gtk_viewport_new (hadjustment, vadjustment);
    gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport), GTK_SHADOW_NONE);
    gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport), GTK_SHADOW_NONE);

    sound_theme_chooser = gvc_sound_theme_chooser_new ();
    vbox = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), sound_theme_chooser, TRUE, TRUE, 6);
    gtk_container_add (GTK_CONTAINER (viewport), vbox);
    gtk_container_add (GTK_CONTAINER (view), viewport);

    gtk_widget_show_all (vbox);
    gtk_widget_show (sound_theme_chooser);
    gtk_widget_show (viewport);
    gtk_widget_show (view);
    gtk_widget_show (label);

    gtk_notebook_append_page (GTK_NOTEBOOK (el), view, label);
  }
#endif
}

/*
 * MateConf callback.
 */

static void
cb_mateconf (MateConfClient *client,
	  guint        connection_id,
	  MateConfEntry  *entry,
	  gpointer     data)
{
  MateVolumeControlElement *el = MATE_VOLUME_CONTROL_ELEMENT (data);
  gchar *keybase = get_mateconf_key (el->mixer, NULL);

  if (!strncmp (mateconf_entry_get_key (entry),
		keybase, strlen (keybase))) {
    const GList *item;

    for (item = gst_mixer_list_tracks (el->mixer);
	 item != NULL; item = item->next) {
      GstMixerTrack *track = item->data;
      MateVolumeControlTrack *trkw =
	g_object_get_data (G_OBJECT (track), "mate-volume-control-trkw");
      gchar *key = get_mateconf_key (el->mixer, track);

      g_return_if_fail (mateconf_entry_get_key (entry) != NULL);
      g_return_if_fail (key != NULL);

      if (g_str_equal (mateconf_entry_get_key (entry), key)) {
        MateConfValue *value = mateconf_entry_get_value (entry);

        if (value->type == MATECONF_VALUE_BOOL) {
          gboolean active = mateconf_value_get_bool (value),
		   first[4] = { TRUE, TRUE, TRUE, TRUE };
          gint n, page = get_page_num (el->mixer, track);

          mate_volume_control_track_show (trkw, active);

          /* separators */
          for (item = gst_mixer_list_tracks (el->mixer);
	       item != NULL; item = item->next) {
            GstMixerTrack *track = item->data;
            MateVolumeControlTrack *trkw =
	      g_object_get_data (G_OBJECT (track), "mate-volume-control-trkw");

            n = get_page_num (el->mixer, track);
            if (trkw->visible && !first[n]) {
              if (trkw->left_separator) {
                if (n < 2 && track->num_channels == 0) {
                   gtk_widget_hide (trkw->left_separator);
                } else {
                   gtk_widget_show (trkw->left_separator);
                }
              }
            } else {
              if (trkw->left_separator)
                gtk_widget_hide (trkw->left_separator);
            }

            if (trkw->visible && first[n])
              first[n] = FALSE;
          }
          update_tab_visibility (el, page, page);
          break;
        }
      }

      g_free (key);
    }
  }
  g_free (keybase);
}
