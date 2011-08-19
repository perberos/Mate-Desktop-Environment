/* MATE Volume Control
 * Copyright (C) 2003-2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * volume.c: representation of a track's volume channels
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

#define _ISOC99_SOURCE

#include <math.h>
#include <glib/gi18n.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include "volume.h"
#include "button.h"

G_DEFINE_TYPE (MateVolumeControlVolume, mate_volume_control_volume, GTK_TYPE_FIXED)


static void	mate_volume_control_volume_class_init	(MateVolumeControlVolumeClass *klass);
static void	mate_volume_control_volume_init	(MateVolumeControlVolume *el);
static void	mate_volume_control_volume_dispose	(GObject   *object);

static void	mate_volume_control_volume_size_req	(GtkWidget *widget,
							 GtkRequisition *req);
static void	mate_volume_control_volume_size_alloc	(GtkWidget *widget,
							 GtkAllocation *alloc);
static gboolean	mate_volume_control_volume_expose	(GtkWidget *widget,
							 GdkEventExpose *expose);

static void	cb_volume_changed			(GtkAdjustment *adj,
							 gpointer   data);
static void	cb_lock_toggled				(GtkToggleButton *button,
							 gpointer   data);

static gboolean	cb_check				(gpointer   data);


static void
mate_volume_control_volume_class_init (MateVolumeControlVolumeClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *gtkwidget_class = GTK_WIDGET_CLASS (klass);

  gobject_class->dispose = mate_volume_control_volume_dispose;
  gtkwidget_class->size_allocate = mate_volume_control_volume_size_alloc;
  gtkwidget_class->size_request = mate_volume_control_volume_size_req;
  gtkwidget_class->expose_event = mate_volume_control_volume_expose;
}

static void
mate_volume_control_volume_init (MateVolumeControlVolume *vol)
{
  #if GTK_CHECK_VERSION(2,18,0)
  gtk_widget_set_has_window (GTK_WIDGET (vol), TRUE);
  #else
  gtk_fixed_set_has_window (GTK_FIXED (vol), TRUE);
  #endif

  vol->mixer = NULL;
  vol->track = NULL;
  vol->padding = 6;
  vol->scales = NULL;
  vol->button = NULL;
  vol->locked = FALSE;
  vol->id = 0;
}

static GtkWidget *
get_scale (MateVolumeControlVolume *vol,
	   gint num_chan,
	   gint volume)
{
  GtkWidget *slider;
  GtkObject *adj;
  AtkObject *accessible;
  gchar *accessible_name;

  adj = gtk_adjustment_new (volume,
		vol->track->min_volume, vol->track->max_volume,
		(vol->track->max_volume - vol->track->min_volume) / 100.0,
		(vol->track->max_volume - vol->track->min_volume) / 10.0, 0.0);
  g_signal_connect (adj, "value_changed",
		    G_CALLBACK (cb_volume_changed), vol);
  slider = gtk_vscale_new (GTK_ADJUSTMENT (adj));
  gtk_scale_set_draw_value (GTK_SCALE (slider), FALSE);
  gtk_range_set_inverted (GTK_RANGE (slider), TRUE);

  /* a11y */
  accessible = gtk_widget_get_accessible (slider);
  if (GTK_IS_ACCESSIBLE (accessible)) {
    if (vol->track->num_channels == 1) {
      accessible_name = g_strdup_printf (_("Track %s"),
					 vol->track->label);
    } else {
      gchar *accessible_desc = g_strdup_printf (_("Channel %d of track %s"),
						num_chan + 1,
						vol->track->label);
      accessible_name = g_strdup_printf (_("Track %s, channel %d"),
					 vol->track->label, num_chan + 1);
      atk_object_set_description (accessible, accessible_desc); 
      g_free (accessible_desc);
    }
    atk_object_set_name (accessible, accessible_name);
    g_free (accessible_name);
  }

  return slider;
}

static void
get_button (MateVolumeControlVolume *vol,
	    gint *volumes)
{
  AtkObject *accessible;
  gchar *accessible_name, *msg;
  gint n;

  msg = g_strdup_printf (_("Lock channels for %s together"), vol->track->label);
  vol->button = mate_volume_control_button_new ("chain.png",
						 "chain-broken.png",
						 msg);
  g_free (msg);
  g_signal_connect (vol->button, "clicked",
		    G_CALLBACK (cb_lock_toggled), vol);
  for (n = 1; n < vol->track->num_channels; n++) {
    /* default, unlocked */
    if (volumes[n] != volumes[0])
      break;
  }
  mate_volume_control_button_set_active (MATE_VOLUME_CONTROL_BUTTON (vol->button),
					  n == vol->track->num_channels);

  /* a11y */
  accessible = gtk_widget_get_accessible (vol->button);
  if (GTK_IS_ACCESSIBLE (accessible)) {
    accessible_name = g_strdup_printf (_("Track %s: lock channels together"),
				       vol->track->label);
    atk_object_set_name (accessible, accessible_name);
    g_free (accessible_name);
  }
}

GtkWidget *
mate_volume_control_volume_new (GstMixer *mixer,
				 GstMixerTrack *track,
				 gint      padding)
{
  MateVolumeControlVolume *vol;
  gint *volumes, n;
  gchar *msg, *chan;
  gboolean need_timeout = TRUE;

  need_timeout = ((gst_mixer_get_mixer_flags (GST_MIXER (mixer)) &
		   GST_MIXER_FLAG_AUTO_NOTIFICATIONS) == 0);

  /* volume */
  vol = g_object_new (MATE_VOLUME_CONTROL_TYPE_VOLUME, NULL);
  gst_object_ref (GST_OBJECT (mixer));
  vol->mixer = mixer;
  vol->track = g_object_ref (G_OBJECT (track));
  if (padding >= 0)
    vol->padding = padding;

  /* sliders */
  volumes = g_new0 (gint, track->num_channels);
  gst_mixer_get_volume (mixer, track, volumes);
  for (n = 0; n < track->num_channels; n++) {
    GtkWidget *slider;

    /* we will reposition the widget once we're drawing up */
    slider = get_scale (vol, n, volumes[n]);
    gtk_fixed_put (GTK_FIXED (vol), slider, 0, 0);
    gtk_widget_show (slider);
    vol->scales = g_list_append (vol->scales, slider);

    /* somewhat dirty hack that will suffice for now. 1 chan
     * means mono, two means stereo (left/right) and > 2 means
     * alsa, where channel order is front, rear, center, lfe,
     * side. */
    if (vol->track->num_channels == 1) {
        chan = _("mono");
    } else if (vol->track->num_channels == 2) {
        chan = (n == 0) ? _("left") : _("right");
    } else {
        switch (n) {
            case 0:  chan = _("front left"); break;
            case 1:  chan = _("front right"); break;
            case 2:  chan = _("rear left"); break;
            case 3:  chan = _("rear right"); break;
            case 4:  chan = _("front center"); break;
            /* Translators: This is the name of a surround sound channel. It
             * stands for "Low-Frequency Effects". If you're not sure that
             * this has an established and different translation in your
             * language, leave it unchanged. */
            case 5:  chan = _("LFE"); break;
            case 6:  chan = _("side left"); break;
            case 7:  chan = _("side right"); break;
            default: chan = _("unknown"); break;
        }
    }

    /* Here, we can actually tell people that this
     * is a slider that will change channel X. */
    msg = g_strdup_printf (_("Volume of %s channel on %s"),
            chan, vol->track->label);
    gtk_widget_set_tooltip_text (slider, msg);
    g_free (msg);
  }

  /* chainbutton */
  get_button (vol, volumes);
  if (track->num_channels > 1) {
    gtk_fixed_put (GTK_FIXED (vol), vol->button, 0, 0);
    gtk_widget_show (vol->button);
  }

  g_free (volumes);

  /* GStreamer signals */
  if (need_timeout)
    vol->id = g_timeout_add (100, cb_check, vol);

  return GTK_WIDGET (vol);
}

static void
mate_volume_control_volume_dispose (GObject *object)
{
  MateVolumeControlVolume *vol = MATE_VOLUME_CONTROL_VOLUME (object);

  if (vol->id != 0) {
    g_source_remove (vol->id);
    vol->id = 0;
  }

  if (vol->track) {
    g_object_unref (G_OBJECT (vol->track));
    vol->track = NULL;
  }

  if (vol->mixer) {
    gst_object_unref (GST_OBJECT (vol->mixer));
    vol->mixer = NULL;
  }

  if (vol->scales) {
    g_list_free (vol->scales);
    vol->scales = NULL;
  }

  G_OBJECT_CLASS (mate_volume_control_volume_parent_class)->dispose (object);
}

/*
 * Gtk/GDK virtual functions for size negotiation.
 */

static void
mate_volume_control_volume_size_req (GtkWidget *widget,
				      GtkRequisition *req)
{
  MateVolumeControlVolume *vol = MATE_VOLUME_CONTROL_VOLUME (widget);
  GtkRequisition but_req, scale_req;

  /* request size of kids */
  GTK_WIDGET_GET_CLASS (vol->button)->size_request (vol->button, &but_req);
  GTK_WIDGET_GET_CLASS (vol->scales->data)->size_request (vol->scales->data,
							  &scale_req);
  if (scale_req.height < 100)
    scale_req.height = 100;

  /* calculate our own size from that */
  req->width = scale_req.width * vol->track->num_channels +
      vol->padding * (vol->track->num_channels - 1);
  req->height = scale_req.height + but_req.height /*+ vol->padding*/;
}

static void
mate_volume_control_volume_size_alloc (GtkWidget *widget,
					GtkAllocation *alloc)
{
  MateVolumeControlVolume *vol = MATE_VOLUME_CONTROL_VOLUME (widget);
  GtkRequisition but_req, scale_req;
  GtkAllocation but_all, scale_all;
  gint x_offset, but_deco_width, n = 0;
  GList *scales;
  GtkAllocation allocation;

  /* loop? */
  gtk_widget_get_allocation (widget, &allocation);
  if (alloc->x == allocation.x &&
      alloc->y == allocation.y &&
      alloc->width == allocation.width &&
      alloc->height == allocation.height)
    return;

  /* request size of kids */
  GTK_WIDGET_GET_CLASS (vol->button)->size_request (vol->button, &but_req);
  GTK_WIDGET_GET_CLASS (vol->scales->data)->size_request (vol->scales->data,
							  &scale_req);

  /* calculate */
  x_offset = (alloc->width - ((vol->track->num_channels * scale_req.width) +
      (vol->track->num_channels - 1) * vol->padding)) / 2;
  scale_all.width = scale_req.width;
  scale_all.height = alloc->height - but_req.height;
  scale_all.y = 0;
  but_deco_width = alloc->width - (2 * x_offset);
  but_all.width = but_req.width;
  but_all.height = but_req.height;
  but_all.x = x_offset + (but_deco_width - but_req.width) / 2;
  but_all.y = alloc->height - but_req.height;

  /* tell sliders */
  for (scales = vol->scales; scales != NULL; scales = scales->next, n++) {
    scale_all.x = x_offset + n * (scale_req.width + vol->padding);
    gtk_fixed_move (GTK_FIXED (vol), scales->data, scale_all.x, scale_all.y);
    gtk_widget_set_size_request (scales->data, scale_all.width, scale_all.height);
  }

  /* tell button */
  if (vol->track->num_channels > 1) {
    gtk_fixed_move (GTK_FIXED (vol), vol->button, but_all.x, but_all.y);
    gtk_widget_set_size_request (vol->button, but_all.width, but_all.height);
  }

  /* parent will resize window */
  GTK_WIDGET_CLASS (mate_volume_control_volume_parent_class)->size_allocate (widget, alloc);
}

static gboolean
mate_volume_control_volume_expose (GtkWidget *widget,
				    GdkEventExpose *expose)
{
  GtkAllocation allocation;
  MateVolumeControlVolume *vol = MATE_VOLUME_CONTROL_VOLUME (widget);

  /* clear background */
  gtk_widget_get_allocation (widget, &allocation);
  gdk_window_clear_area (gtk_widget_get_window (widget), 0, 0,
			 allocation.width,
			 allocation.height);

  if (vol->track->num_channels > 1) {
    gint x_offset, y_offset, height, width;
    GtkRequisition scale_req, but_req;
    GdkPoint points[3];
    GtkStyle *style;
    GtkStateType state;

    /* request size of kids */
    GTK_WIDGET_GET_CLASS (vol->button)->size_request (vol->button, &but_req);
    GTK_WIDGET_GET_CLASS (vol->scales->data)->size_request (vol->scales->data,
							    &scale_req);

    /* calculate */
    gtk_widget_get_allocation (widget, &allocation);
    x_offset = (allocation.width -
        ((vol->track->num_channels * scale_req.width) +
        (vol->track->num_channels - 1) * vol->padding)) / 2;
    y_offset = allocation.height - but_req.height;
    width = allocation.width - (2 * x_offset + but_req.width);
    height = but_req.height / 2;
    points[0].y = y_offset + 3;
    points[1].y = points[2].y = points[0].y + height - 3;

    /* draw chainbutton decorations */
    style = gtk_widget_get_style (widget);
    state = gtk_widget_get_state (widget);

    points[0].x = points[1].x = x_offset + 3;
    points[2].x = points[0].x + width - 6;
    gtk_paint_polygon (style, gtk_widget_get_window (widget),
		       state,
		       GTK_SHADOW_ETCHED_IN,
		       &expose->area, widget, "hseparator",
		       points, 3, FALSE);

    points[0].x = points[1].x = allocation.width - x_offset - 3;
    points[2].x = points[0].x - width + 6;
    gtk_paint_polygon (style, gtk_widget_get_window (widget),
		       state,
		       GTK_SHADOW_ETCHED_IN,
		       &expose->area, widget, "hseparator",
		       points, 3, FALSE);
  }

  /* take care of redrawing the kids */
  return GTK_WIDGET_CLASS (mate_volume_control_volume_parent_class)->expose_event (widget, expose);
}

/*
 * Signals handlers.
 */

static void
cb_volume_changed (GtkAdjustment *_adj,
		   gpointer       data)
{
  MateVolumeControlVolume *vol = data;
  gint *volumes, i = 0;
  GList *scales;

  if (vol->locked)
    return;
  vol->locked = TRUE;
  volumes = g_new (gint, vol->track->num_channels);

  for (scales = vol->scales; scales != NULL; scales = scales->next) {
    GtkAdjustment *adj = gtk_range_get_adjustment (scales->data);

    if (mate_volume_control_button_get_active (
            MATE_VOLUME_CONTROL_BUTTON (vol->button))) {
      gtk_adjustment_set_value (adj, gtk_adjustment_get_value (_adj));
      volumes[i++] = rint (gtk_adjustment_get_value (_adj));
    } else {
      volumes[i++] = rint (gtk_adjustment_get_value (adj));
    }
  }

  gst_mixer_set_volume (vol->mixer, vol->track, volumes);

  g_free (volumes);
  vol->locked = FALSE;
}

static void
cb_lock_toggled (GtkToggleButton *button,
		 gpointer         data)
{
  MateVolumeControlVolume *vol = data;

  if (mate_volume_control_button_get_active (
          MATE_VOLUME_CONTROL_BUTTON (vol->button))) {
    /* get the mean value, and set it on the first adjustment.
     * the cb_volume_changed () callback will take care of the
     * rest. */
    gint volume = 0, num = 0;
    GList *scales;

    for (scales = vol->scales ; scales != NULL; scales = scales->next) {
      GtkAdjustment *adj = gtk_range_get_adjustment (scales->data);

      num++;
      volume += gtk_adjustment_get_value (adj);
    }

    /* safety check */
    if (vol->scales != NULL) {
      gtk_adjustment_set_value (gtk_range_get_adjustment (vol->scales->data),
				volume / num);
    }
  }
}

/*
 * See if our volume is zero.
 */

void
mate_volume_control_volume_ask (MateVolumeControlVolume * vol,
    gboolean *real_zero, gboolean *slider_zero)
{
  GList *scales;
  gint *volumes, n, tot = 0;

  volumes = g_new (gint, vol->track->num_channels);
  gst_mixer_get_volume (vol->mixer, vol->track, volumes);
  for (n = 0; n < vol->track->num_channels; n++)
    tot += volumes[n];
  g_free (volumes);
  *real_zero = (tot == 0);

  *slider_zero = TRUE;
  for (n = 0, scales = vol->scales;
       scales != NULL; scales = scales->next, n++) {
    GtkAdjustment *adj = gtk_range_get_adjustment (scales->data);
                                                                                
    if (rint (gtk_adjustment_get_value (adj)) != 0) {
      *slider_zero = FALSE;
      break;
    }
  }
}


void
mate_volume_control_volume_update (MateVolumeControlVolume *vol)
{
  gint *volumes, n;
  GList *scales;

  /* don't do callbacks */
  if (vol->locked)
    return;

  vol->locked = TRUE;

  volumes = g_new (gint, vol->track->num_channels);
  gst_mixer_get_volume (vol->mixer, vol->track, volumes);

  /* did we change? */
  for (n = 0, scales = vol->scales;
       scales != NULL; scales = scales->next, n++) {
    GtkAdjustment *adj = gtk_range_get_adjustment (scales->data);

    if ((gint) gtk_adjustment_get_value (adj) != volumes[n]) {
      gtk_range_set_value (scales->data, volumes[n]);
    }

    /* should we release lock? */
    if (volumes[n] != volumes[0]) {
      mate_volume_control_button_set_active (
	MATE_VOLUME_CONTROL_BUTTON (vol->button), FALSE);
    }
  }

  g_free (volumes);
  vol->locked = FALSE;
}

/*
 * Timeout to check for volume changes.
 */

static gboolean
cb_check (gpointer data)
{
  mate_volume_control_volume_update (data);

  return TRUE;
}
