/* MATE Volume Control
 * Copyright (C) 2003-2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * preferences.h: preferences screen
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

#ifndef __GVC_PREFERENCES_H__
#define __GVC_PREFERENCES_H__

#include <glib.h>
#include <gtk/gtk.h>
#include <mateconf/mateconf-client.h>
#include <gst/interfaces/mixer.h>

G_BEGIN_DECLS

#define MATE_VOLUME_CONTROL_TYPE_PREFERENCES \
  (mate_volume_control_preferences_get_type ())
#define MATE_VOLUME_CONTROL_PREFERENCES(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_VOLUME_CONTROL_TYPE_PREFERENCES, \
			       MateVolumeControlPreferences))
#define MATE_VOLUME_CONTROL_PREFERENCES_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_VOLUME_CONTROL_TYPE_PREFERENCES, \
			    MateVolumeControlPreferencesClass))
#define MATE_VOLUME_CONTROL_IS_PREFERENCES(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_VOLUME_CONTROL_TYPE_PREFERENCES))
#define MATE_VOLUME_CONTROL_IS_PREFERENCES_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_VOLUME_CONTROL_TYPE_PREFERENCES))

typedef struct _MateVolumeControlPreferences {
  GtkDialog parent;

  /* current element that we're working on */
  GstMixer *mixer;

  /* mateconf client inherited from our parent */
  MateConfClient *client;
  guint client_cnxn;

  /* treeview inside us */
  GtkWidget *treeview;
} MateVolumeControlPreferences;

typedef struct _MateVolumeControlPreferencesClass {
  GtkDialogClass klass;
} MateVolumeControlPreferencesClass;

GType	mate_volume_control_preferences_get_type (void);
GtkWidget *mate_volume_control_preferences_new	(GstElement  *element,
						 MateConfClient *client);
void	mate_volume_control_preferences_change	(MateVolumeControlPreferences *prefs,
						 GstElement  *element);

/*
 * MateConf thingy. Escapes spaces and such.
 */
gchar *	get_mateconf_key	(GstMixer *mixer, GstMixerTrack *track);


G_END_DECLS

#endif /* __GVC_PREFERENCES_H__ */
