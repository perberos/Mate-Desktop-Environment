/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 2 -*- */
/* MATE Volume Applet
 * Copyright (C) 2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * applet.h: the main applet
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

#ifndef __GVA_APPLET_H__
#define __GVA_APPLET_H__

#include <glib.h>
#include <glib/gi18n.h>

#include <gtk/gtk.h>
#include <mateconf/mateconf-client.h>
#include <mate-panel-applet-mateconf.h>
#include <gst/gst.h>
#include <gst/interfaces/mixer.h>

#include "dock.h"

G_BEGIN_DECLS

#define MATE_TYPE_VOLUME_APPLET \
  (mate_volume_applet_get_type ())
#define MATE_VOLUME_APPLET(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_VOLUME_APPLET, \
			       MateVolumeApplet))
#define MATE_VOLUME_APPLET_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_VOLUME_APPLET, \
			    MateAppletAppletAppletClass))
#define MATE_IS_VOLUME_APPLET(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_VOLUME_APPLET))
#define MATE_IS_VOLUME_APPLET_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_VOLUME_APPLET))

struct _MateVolumeApplet {
  MatePanelApplet parent;

  /* menu actions */
  GtkActionGroup *action_group;

  /* our main icon, which is our panel user interface */
  GtkImage *image;

  /* the docked window containing the volume widget */
  MateVolumeAppletDock *dock;
  gboolean pop;
  /* the adjustment connected to the dock slider */
  GtkAdjustment *adjustment;

  /* list of volume control elements */
  GList *elements;

  /* mateconf */
  MateConfClient *client;

  /* element */
  GstMixer *mixer;
  GstBus *bus;
  gboolean lock;
  gint state;
  GList *tracks;

  /* timeout ID, used to decouple on disposal */
  guint timeout;

  /* preferences */
  GtkWidget *prefs;

  /* icon theme */
  GdkPixbuf *pix[5];
  gint panel_size;

  /* use same object for setting tooltop */
  gboolean force_next_update;
};

typedef struct _MateVolumeAppletClass {
  MatePanelAppletClass klass;
} MateVolumeAppletClass;

void     mate_volume_applet_adjust_volume (GstMixer      *mixer,
					    GstMixerTrack *track,
					    gdouble        volume);
void     mate_volume_applet_toggle_mute (MateVolumeApplet *applet);
void     mate_volume_applet_run_mixer (MateVolumeApplet *applet);
GType    mate_volume_applet_get_type (void);
gboolean mate_volume_applet_setup    (MateVolumeApplet *applet,
				       GList             *elements);
gboolean mixer_is_muted (MateVolumeApplet *applet);

G_END_DECLS

#endif /* __GVA_APPLET_H__ */
