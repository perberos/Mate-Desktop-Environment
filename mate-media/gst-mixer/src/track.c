/* MATE Volume Control
 * Copyright (C) 2003-2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * track.c: layout of a single mixer track
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

#include <unistd.h>
#include <glib/gi18n.h>
#include <string.h>

#include "button.h"
#include "track.h"
#include "volume.h"

static const struct {
  gchar *label,
	*pixmap;
} pix[] = {
  { "cd",         "media-optical"         	},
  { "line",       "gvc-line-in"                 },
  { "aux",        "gvc-line-in"                 },
  { "mic",        "audio-input-microphone"      },
  { "cap",        "gvc-line-in"			},
  { "mix",        "multimedia-volume-control"   },
  { "pcm",        "gvc-tone"       		},
  { "headphone",  "gvc-headphones" 		},
  { "phone",      "phone"      			},
  { "speaker",    "audio-volume-high"           },
  { "front",      "audio-volume-high"           },
  { "surround",   "audio-volume-high"           },
  { "side",       "audio-volume-high"           },
  { "center",     "audio-volume-high"           },
  { "lfe",        "audio-volume-high"           },
  { "video",      "video-display"      		},
  { "volume",     "gvc-tone"       		},
  { "master",     "gvc-tone"       		},
  { "3d",         "gvc-3d-sound"		},
  { "beep",       "keyboard"                    },
  { "record",     "audio-input-microphone"      },
  { NULL, NULL }
};

/*
 * UI responses.
 */

static void
cb_mute_toggled (MateVolumeControlButton *button,
		 gpointer         data)
{
  MateVolumeControlTrack *ctrl = data;

  gst_mixer_set_mute (ctrl->mixer, ctrl->track,
		      !mate_volume_control_button_get_active (button));
}

static void
cb_record_toggled (MateVolumeControlButton *button,
		   gpointer         data)
{
  MateVolumeControlTrack *ctrl = data;

  gst_mixer_set_record (ctrl->mixer, ctrl->track,
		        mate_volume_control_button_get_active (button));
}

/* Tells us whether toggling a switch should change the corresponding
 * GstMixerTrack's MUTE or RECORD flag.
 */
static gboolean
should_toggle_record_switch (const GstMixerTrack *track)
{
  return (GST_MIXER_TRACK_HAS_FLAG (track, GST_MIXER_TRACK_INPUT) &&
    !GST_MIXER_TRACK_HAS_FLAG (track, GST_MIXER_TRACK_NO_RECORD));
}


static void
cb_toggle_changed (GtkToggleButton *button,
		   gpointer         data)
{
  MateVolumeControlTrack *ctrl = data;

  if (should_toggle_record_switch (ctrl->track)) {
    gst_mixer_set_record (ctrl->mixer, ctrl->track,
      gtk_toggle_button_get_active (button));
  } else {
    gst_mixer_set_mute (ctrl->mixer, ctrl->track,
      !gtk_toggle_button_get_active (button));
  }
}

static void
cb_option_changed (GtkComboBox *box,
		   gpointer     data)
{
  MateVolumeControlTrack *ctrl = data;
  gchar *opt;

  opt = gtk_combo_box_get_active_text (box);
  if (opt)
    gst_mixer_set_option (ctrl->mixer, GST_MIXER_OPTIONS (ctrl->track), opt);
  g_free (opt);
}

/*
 * Timeout to check for changes in mixer outside ourselves.
 */

void
mate_volume_control_track_update (MateVolumeControlTrack *trkw)
{
  gboolean mute, record;
  gboolean vol_is_zero = FALSE, slider_is_zero = FALSE;
  GstMixer *mixer;
  GstMixerTrack *track;
  gint i;
  gint *dummy;

  g_return_if_fail (trkw != NULL);

  track = trkw->track;
  mixer = trkw->mixer;

  /* trigger an update of the mixer state */
  if (GST_IS_MIXER_OPTIONS (track)) {
    const GList *opt;
    GstMixerOptions *options = GST_MIXER_OPTIONS (track);
    const char *active_opt;
    active_opt = gst_mixer_get_option (mixer, options);

    for (i = 0, opt = gst_mixer_options_get_values (options);
        opt != NULL;
        opt = opt->next, i++) {
      if (g_str_equal (active_opt, opt->data)) {
       gtk_combo_box_set_active (GTK_COMBO_BOX (trkw->options), i);
      }
    }

    return;
  }

  dummy = g_new (gint, MAX (track->num_channels, 1));
  gst_mixer_get_volume (mixer, track, dummy);
  g_free (dummy);

  mute = GST_MIXER_TRACK_HAS_FLAG (track, GST_MIXER_TRACK_MUTE) ?
    TRUE : FALSE;
  record = GST_MIXER_TRACK_HAS_FLAG (track, GST_MIXER_TRACK_RECORD) ?
    TRUE : FALSE;

  if (trkw->sliderbox) {
    mate_volume_control_volume_update (MATE_VOLUME_CONTROL_VOLUME (trkw->sliderbox));
    mate_volume_control_volume_ask (
      MATE_VOLUME_CONTROL_VOLUME (trkw->sliderbox),
      &vol_is_zero, &slider_is_zero);

    if (trkw->mute && !slider_is_zero && vol_is_zero)
        mute = TRUE;
  }

  if (trkw->mute) {
    if (mate_volume_control_button_get_active (trkw->mute) == mute) {
      mate_volume_control_button_set_active (trkw->mute, !mute);
    }
  }

  if (trkw->record) {
    if (mate_volume_control_button_get_active (trkw->record) != record) {
      mate_volume_control_button_set_active (trkw->record, record);
    }
  }

  if (trkw->toggle) {
    if (should_toggle_record_switch (trkw->track)) {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (trkw->toggle),
        GST_MIXER_TRACK_HAS_FLAG (trkw->track, GST_MIXER_TRACK_RECORD));
    } else {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (trkw->toggle),
        !GST_MIXER_TRACK_HAS_FLAG (trkw->track, GST_MIXER_TRACK_MUTE));
    }
  }

  /* FIXME:
   * - options.
   */
}


static gboolean
cb_check (gpointer data)
{
  mate_volume_control_track_update (data);

  return TRUE;
}

/*
 * Actual UI code.
 */

static MateVolumeControlTrack *
mate_volume_control_track_add_title (GtkTable *table,
				      gint      tab_pos,
				      GtkOrientation or,
				      GstMixer *mixer,
				      GstMixerTrack *track,
				      GtkWidget *l_sep,
				      GtkWidget *r_sep)
{
  MateVolumeControlTrack *ctrl;
  gchar *ulabel = NULL;
  gchar *str = NULL;
  gint i;
  gboolean need_timeout = TRUE;

  need_timeout = ((gst_mixer_get_mixer_flags (GST_MIXER (mixer)) &
		   GST_MIXER_FLAG_AUTO_NOTIFICATIONS) == 0);

  /* start */
  ctrl = g_new0 (MateVolumeControlTrack, 1);
  ctrl->mixer = mixer;
  g_object_ref (G_OBJECT (track));
  ctrl->track = track;
  ctrl->left_separator = l_sep;
  ctrl->right_separator = r_sep;
  ctrl->visible = TRUE;
  ctrl->table = table;
  ctrl->pos = tab_pos;
  if (need_timeout)
    ctrl->id = g_timeout_add (200, cb_check, ctrl);
  ctrl->flagbuttonbox = NULL;

  /* find image from label string (optional) */
  if (g_object_class_find_property (G_OBJECT_GET_CLASS (track), "untranslated-label"))
    g_object_get (track, "untranslated-label", &ulabel, NULL);

  if (ulabel == NULL)
    g_object_get (track, "label", &ulabel, NULL);

  if (ulabel) {
    gint pos;

    /* make case insensitive */
    for (pos = 0; ulabel[pos] != '\0'; pos++)
      ulabel[pos] = g_ascii_tolower (ulabel[pos]);

    for (i = 0; pix[i].label != NULL; i++) {
      if (g_strrstr (ulabel, pix[i].label) != NULL) {
        str = pix[i].pixmap;
        break;
      }
    }

    g_free (ulabel);
  }

  if ((str != NULL) && (track->num_channels != 0)) {
    if ((ctrl->image = gtk_image_new_from_icon_name (str, GTK_ICON_SIZE_MENU)) != NULL) {
      gtk_misc_set_alignment (GTK_MISC (ctrl->image), 0.5, 0.5);
      if (or == GTK_ORIENTATION_VERTICAL) {
        gtk_table_attach (GTK_TABLE (table), ctrl->image,
			  tab_pos, tab_pos + 1, 0, 1,
			  GTK_EXPAND, 0, 0, 0);
      } else {
        gtk_table_attach (GTK_TABLE (table), ctrl->image,
			  0, 1, tab_pos, tab_pos + 1,
			  0, GTK_EXPAND, 0, 0);
      }
      gtk_widget_show (ctrl->image);
    }
  }

  /* text label */
  if (or == GTK_ORIENTATION_HORIZONTAL)
    str = g_strdup_printf (_("%s:"), track->label);
  else
    str = g_strdup (track->label);
  ctrl->label = gtk_label_new (str);
  if (or == GTK_ORIENTATION_HORIZONTAL) {
    g_free (str);
    gtk_misc_set_alignment (GTK_MISC (ctrl->label), 0.0, 0.5);
  }
  if (or == GTK_ORIENTATION_VERTICAL) {
    gtk_table_attach (table, ctrl->label,
		      tab_pos, tab_pos + 1, 1, 2,
		      GTK_EXPAND, 0, 0, 0);
  } else {
    gtk_table_attach (table, ctrl->label,
		      1, 2, tab_pos, tab_pos + 1,
		      GTK_FILL, GTK_EXPAND, 0, 0);
  }
  gtk_widget_show (ctrl->label);

  return ctrl;
}

static void
mate_volume_control_track_put_switch (GtkTable *table,
				       gint      tab_pos,
				       MateVolumeControlTrack *ctrl)
{
  GtkWidget *button;
  AtkObject *accessible;
  gchar *accessible_name, *msg;

  /* container box */
  ctrl->buttonbox = gtk_hbox_new (FALSE, 0);
  gtk_table_attach (GTK_TABLE (table), ctrl->buttonbox,
		    tab_pos, tab_pos + 1,
		    3, 4, GTK_EXPAND, 0, 0, 0);
  gtk_widget_show (ctrl->buttonbox);

  /* if we weren't supposed to show the mute button, then don't create it */
  if (GST_MIXER_TRACK_HAS_FLAG (ctrl->track, GST_MIXER_TRACK_NO_MUTE)) {
    return;
  }

  /* mute button */
  msg = g_strdup_printf (_("Mute/Unmute %s"), ctrl->track->label);
  button = mate_volume_control_button_new ("audio-volume-high",
					    "audio-volume-muted",
					     msg);
  ctrl->mute = MATE_VOLUME_CONTROL_BUTTON (button);
  g_free (msg);

  mate_volume_control_button_set_active (
    MATE_VOLUME_CONTROL_BUTTON (button),
    !GST_MIXER_TRACK_HAS_FLAG (ctrl->track, GST_MIXER_TRACK_MUTE));

  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (cb_mute_toggled), ctrl);

  /* a11y */
  accessible = gtk_widget_get_accessible (button);
  if (GTK_IS_ACCESSIBLE (accessible)) {
    accessible_name = g_strdup_printf (_("Track %s: mute"),
				       ctrl->track->label);
    atk_object_set_name (accessible, accessible_name);
    g_free (accessible_name);
  }

  /* show */
  gtk_box_pack_start (GTK_BOX (ctrl->buttonbox), button,
		      FALSE, FALSE, 0);
  gtk_widget_show (button);
}

MateVolumeControlTrack *
mate_volume_control_track_add_playback	(GtkTable *table,
					 gint      tab_pos,
					 GstMixer *mixer,
					 GstMixerTrack *track,
					 GtkWidget *l_sep,
					 GtkWidget *r_sep,
                                         GtkWidget *fbox)
{
  MateVolumeControlTrack *ctrl;

  /* switch and options exception (no sliders) */
  if (track->num_channels == 0) {
    if (GST_IS_MIXER_OPTIONS (track)) {
      return (mate_volume_control_track_add_option (table, tab_pos, mixer, track,
                                                     l_sep, r_sep, fbox));
    }
    return (mate_volume_control_track_add_switch (table, tab_pos, mixer, track,
                                                   l_sep, r_sep, fbox));
  }

  /* image, title */
  ctrl = mate_volume_control_track_add_title (table, tab_pos,
					       GTK_ORIENTATION_VERTICAL,
					       mixer, track, l_sep, r_sep);

  ctrl->sliderbox = mate_volume_control_volume_new (ctrl->mixer,
						     ctrl->track, 6);
  gtk_table_attach (GTK_TABLE (table), ctrl->sliderbox,
		    tab_pos, tab_pos + 1, 2, 3,
		    GTK_EXPAND, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (ctrl->sliderbox);

  /* mute button */
  mate_volume_control_track_put_switch (table, tab_pos, ctrl);

  return ctrl;
}

MateVolumeControlTrack *
mate_volume_control_track_add_recording (GtkTable *table,
					  gint      tab_pos,
					  GstMixer *mixer,
					  GstMixerTrack *track,
					  GtkWidget *l_sep,
					  GtkWidget *r_sep,
					  GtkWidget *fbox)
{
  MateVolumeControlTrack *ctrl;
  GtkWidget *button;
  AtkObject *accessible;
  gchar *accessible_name, *msg;

  ctrl = mate_volume_control_track_add_playback (table, tab_pos, mixer,
						  track, l_sep, r_sep, fbox);
  if (track->num_channels == 0) {
    return ctrl;
  }

  /* FIXME:
   * - there's something fishy about this button, it
   *     is always FALSE.
   */
  if (!GST_MIXER_TRACK_HAS_FLAG (track, GST_MIXER_TRACK_NO_RECORD)) {
    /* only the record button here */
    msg = g_strdup_printf (_("Toggle audio recording from %s"),
                          ctrl->track->label);
    button = mate_volume_control_button_new ("audio-input-microphone",
                                              "audio-input-microphone-muted",
                                              msg);
    ctrl->record = MATE_VOLUME_CONTROL_BUTTON (button);
    g_free (msg);
    mate_volume_control_button_set_active (MATE_VOLUME_CONTROL_BUTTON (button),
                                            GST_MIXER_TRACK_HAS_FLAG (track,
					      GST_MIXER_TRACK_RECORD));
    g_signal_connect (G_OBJECT (button), "clicked",
		      G_CALLBACK (cb_record_toggled), ctrl);

    /* a11y */
    accessible = gtk_widget_get_accessible (button);
    if (GTK_IS_ACCESSIBLE (accessible)) {
      accessible_name = g_strdup_printf (_("Track %s: audio recording"),
                                        track->label);
      atk_object_set_name (accessible, accessible_name);
      g_free (accessible_name);
    }

    /* attach, show */
    gtk_box_pack_start (GTK_BOX (ctrl->buttonbox), button,
		        FALSE, FALSE, 0);
    gtk_widget_show (button);
  }

  return ctrl;
}

MateVolumeControlTrack *
mate_volume_control_track_add_switch (GtkTable *table,
				       gint      tab_pos,
				       GstMixer *mixer,
				       GstMixerTrack *track,
				       GtkWidget *l_sep,
				       GtkWidget *r_sep,
				       GtkWidget *fbox)
{
  MateVolumeControlTrack *ctrl;
  GtkWidget *toggle;
  gint volume;

  /* image, title */
  toggle = gtk_check_button_new ();

  /* this is a hack - we query volume to initialize switch state */
  gst_mixer_get_volume (mixer, track, &volume);

  if (should_toggle_record_switch (track)) {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
      GST_MIXER_TRACK_HAS_FLAG (track, GST_MIXER_TRACK_RECORD));
  } else {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
      !GST_MIXER_TRACK_HAS_FLAG (track, GST_MIXER_TRACK_MUTE));
  }

  if (fbox == NULL) {
    fbox = gtk_table_new(0, 3, FALSE);
    gtk_table_set_col_spacings (GTK_TABLE (fbox), 6);
  }
  table = GTK_TABLE (fbox);

  ctrl = mate_volume_control_track_add_title (table, tab_pos,
                                              GTK_ORIENTATION_HORIZONTAL,
                                              mixer, track, l_sep, r_sep);
  ctrl->toggle = toggle;
  ctrl->flagbuttonbox = fbox;

  /* attach'n'show */
  gtk_table_attach (table, toggle,
                   2, 3, tab_pos, tab_pos + 1,
                   GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 0);

  g_signal_connect (toggle, "toggled",
		    G_CALLBACK (cb_toggle_changed), ctrl);
  gtk_widget_show (toggle);

  return ctrl;
}

MateVolumeControlTrack *
mate_volume_control_track_add_option (GtkTable *table,
				       gint      tab_pos,
				       GstMixer *mixer,
				       GstMixerTrack *track,
				       GtkWidget *l_sep,
				       GtkWidget *r_sep,
				       GtkWidget *fbox)
{
  MateVolumeControlTrack *ctrl;
  GstMixerOptions *options = GST_MIXER_OPTIONS (track);
  const GList *opt, *opts;
  AtkObject *accessible;
  gchar *accessible_name;
  gint i = 0;
  const gchar *active_opt;

  if (fbox == NULL) {
    fbox = gtk_table_new(0, 3, FALSE);
    gtk_table_set_col_spacings (GTK_TABLE (fbox), 6);
  }
  table = GTK_TABLE (fbox);

  ctrl = mate_volume_control_track_add_title (table, tab_pos,
					       GTK_ORIENTATION_HORIZONTAL,
					       mixer, track, l_sep, r_sep);

  /* optionmenu */
  active_opt = gst_mixer_get_option (mixer, options);
  if (active_opt != NULL) {
    ctrl->options = gtk_combo_box_new_text ();
    opts = gst_mixer_options_get_values (options);
    for (opt = opts; opt != NULL; opt = opt->next, i++) {
      if (opt->data == NULL)
	continue;

      gtk_combo_box_append_text (GTK_COMBO_BOX (ctrl->options), opt->data);

      if (g_str_equal (active_opt, opt->data)) {
	gtk_combo_box_set_active (GTK_COMBO_BOX (ctrl->options), i);
      }
    }
  }

  /* a11y */
  accessible = gtk_widget_get_accessible (ctrl->options);
  if (GTK_IS_ACCESSIBLE (accessible)) {
    accessible_name = g_strdup_printf (_("%s Option Selection"),
				       ctrl->track->label);
    atk_object_set_name (accessible, accessible_name);
    g_free (accessible_name);
  }
  gtk_widget_show (ctrl->options);
  g_signal_connect (ctrl->options, "changed",
		    G_CALLBACK (cb_option_changed), ctrl);

  ctrl->flagbuttonbox = fbox;

  /* attach'n'show */
  gtk_table_attach (table, ctrl->options,
		    2, 3, tab_pos, tab_pos + 1,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 0);
  gtk_widget_show (ctrl->options);

  return ctrl;
}

void
mate_volume_control_track_free (MateVolumeControlTrack *track)
{
  if (track->id != 0) {
    g_source_remove (track->id);
    track->id = 0;
  }

  g_object_unref (G_OBJECT (track->track));

  g_free (track);
}

void
mate_volume_control_track_show (MateVolumeControlTrack *track,
				 gboolean visible)
{
#define func(w) \
  if (w != NULL) { \
    if (visible) { \
      gtk_widget_show (w); \
    } else { \
      gtk_widget_hide (w); \
    } \
  }

  func (track->label);
  func (track->image);
  func (track->sliderbox);
  func (track->buttonbox);
  func (track->toggle);
  func (track->options);

  track->visible = visible;

  /* get rid of spacing between hidden tracks */
  if (visible) {
    if (track->options) {
      gtk_table_set_row_spacing (track->table,
				 track->pos, 6);
      if (track->pos > 0)
        gtk_table_set_row_spacing (track->table,
				   track->pos - 1, 6);
    } else if (!track->toggle) {
      gtk_table_set_col_spacing (track->table,
				 track->pos, 6);
      if (track->pos > 0)
        gtk_table_set_col_spacing (track->table,
				   track->pos - 1, 6);
    }
  } else {
    if (track->options) {
      gtk_table_set_row_spacing (track->table,
				 track->pos, 0);
      if (track->pos > 0)
        gtk_table_set_row_spacing (track->table,
				   track->pos - 1, 0);
    } else if (!track->toggle) {
      gtk_table_set_col_spacing (track->table,
				 track->pos, 0);
      if (track->pos > 0)
        gtk_table_set_col_spacing (track->table,
				   track->pos - 1, 0);
    }
  }
}
