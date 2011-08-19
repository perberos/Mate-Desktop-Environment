/* MATE Volume Control
 * Copyright (C) 2003-2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * element.h: widget representation of a single mixer element
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

#ifndef __GVC_ELEMENT_H__
#define __GVC_ELEMENT_H__

#include <glib.h>
#include <gtk/gtk.h>
#include <mateconf/mateconf-client.h>
#include <gst/interfaces/mixer.h>

G_BEGIN_DECLS

#define MATE_VOLUME_CONTROL_TYPE_ELEMENT \
  (mate_volume_control_element_get_type ())
#define MATE_VOLUME_CONTROL_ELEMENT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_VOLUME_CONTROL_TYPE_ELEMENT, \
			       MateVolumeControlElement))
#define MATE_VOLUME_CONTROL_ELEMENT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_VOLUME_CONTROL_TYPE_ELEMENT, \
			    MateVolumeControlElementClass))
#define MATE_VOLUME_CONTROL_IS_ELEMENT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_VOLUME_CONTROL_TYPE_ELEMENT))
#define MATE_VOLUME_CONTROL_IS_ELEMENT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_VOLUME_CONTROL_TYPE_ELEMENT))

typedef struct _MateVolumeControlElement {
  GtkNotebook parent;

  /* current element that we're working on */
  GstMixer *mixer;

  /* mateconf client inherited from our parent */
  MateConfClient *client;
} MateVolumeControlElement;

typedef struct _MateVolumeControlElementClass {
  GtkNotebookClass klass;
} MateVolumeControlElementClass;

GType		mate_volume_control_element_get_type	(void);
GtkWidget *	mate_volume_control_element_new	(MateConfClient  *client);
void		mate_volume_control_element_change	(MateVolumeControlElement *el,
							 GstElement  *element);
gboolean	mate_volume_control_element_whitelist	(GstMixer *mixer,
							 GstMixerTrack *track);

G_END_DECLS

#endif /* __GVC_ELEMENT_H__ */
