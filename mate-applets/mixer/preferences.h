/* MATE Volume Applet
 * Copyright (C) 2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
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

#ifndef __GVA_PREFERENCES_H__
#define __GVA_PREFERENCES_H__

#include <glib.h>
#include <gtk/gtk.h>
#include <mate-panel-applet.h>
#include <mate-panel-applet-mateconf.h>
#include <gst/interfaces/mixer.h>

G_BEGIN_DECLS

#define MATE_VOLUME_APPLET_TYPE_PREFERENCES \
  (mate_volume_applet_preferences_get_type ())
#define MATE_VOLUME_APPLET_PREFERENCES(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_VOLUME_APPLET_TYPE_PREFERENCES, \
			       MateVolumeAppletPreferences))
#define MATE_VOLUME_APPLET_PREFERENCES_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_VOLUME_APPLET_TYPE_PREFERENCES, \
			    MateVolumeAppletPreferencesClass))
#define MATE_VOLUME_APPLET_IS_PREFERENCES(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_VOLUME_APPLET_TYPE_PREFERENCES))
#define MATE_VOLUME_APPLET_IS_PREFERENCES_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_VOLUME_APPLET_TYPE_PREFERENCES))

typedef struct _MateVolumeAppletPreferences {
  GtkDialog parent;

  /* all elements */
  GList *elements;

  /* current element that we're working on */
  GstMixer *mixer;

  /* is the track list currently locked */
  gboolean track_lock;

  /* for mateconf */
  MatePanelApplet *applet;

  /* treeview inside us */
  GtkWidget *optionmenu, *treeview;
} MateVolumeAppletPreferences;

typedef struct _MateVolumeAppletPreferencesClass {
  GtkDialogClass klass;
} MateVolumeAppletPreferencesClass;

GType	mate_volume_applet_preferences_get_type (void);
GtkWidget *mate_volume_applet_preferences_new	(MatePanelApplet *applet,
						 GList       *elements,
						 GstMixer    *mixer,
						 GList       *track);
void	mate_volume_applet_preferences_change	(MateVolumeAppletPreferences *prefs,
						 GstMixer    *mixer,
						 GList       *tracks);

G_END_DECLS

#endif /* __GVA_PREFERENCES_H__ */
