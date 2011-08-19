/* MATE Volume Applet
 * Copyright (C) 2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * dock.h: floating window containing volume widgets
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

#ifndef __GVA_DOCK_H__
#define __GVA_DOCK_H__

#include <glib.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

/* FIXME: This is necessary to avoid circular references in the headers.
 * This can be fixed by breaking the applet object into a model and a view. */
typedef struct _MateVolumeApplet MateVolumeApplet;

#define MATE_VOLUME_APPLET_TYPE_DOCK \
  (mate_volume_applet_dock_get_type ())
#define MATE_VOLUME_APPLET_DOCK(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_VOLUME_APPLET_TYPE_DOCK, \
			       MateVolumeAppletDock))
#define MATE_VOLUME_APPLET_DOCK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_VOLUME_APPLET_TYPE_DOCK, \
			    MateAppletAppletAppletDockClass))
#define MATE_VOLUME_APPLET_IS_DOCK(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_VOLUME_APPLET_TYPE_DOCK))
#define MATE_VOLUME_APPLET_IS_DOCK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_VOLUME_APPLET_TYPE_DOCK))

typedef struct _MateVolumeAppletDock {
  GtkWindow parent;

  /* us */
  GtkRange *scale;
  GtkButton *plus, *minus;
  GtkWidget *mute;

  /* timeout for buttons */
  guint timeout;
  gint direction;

  /* orientation */
  GtkOrientation orientation;

  MateVolumeApplet *model;
} MateVolumeAppletDock;

typedef struct _MateVolumeAppletDockClass {
  GtkWindowClass klass;
} MateVolumeAppletDockClass;

GType mate_volume_applet_dock_get_type	(void);
GtkWidget *mate_volume_applet_dock_new (GtkOrientation orientation,
					 MateVolumeApplet *parent);
void mate_volume_applet_dock_change (MateVolumeAppletDock *dock,
				      GtkAdjustment *adj);
void mate_volume_applet_dock_set_focus (MateVolumeAppletDock *dock);

G_END_DECLS

#endif /* __GVA_DOCK_H__ */
