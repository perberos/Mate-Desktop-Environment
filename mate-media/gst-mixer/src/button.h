/* MATE Button Control
 * Copyright (C) 2003-2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * button.h: flat toggle button with images
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

#ifndef __GVC_BUTTON_H__
#define __GVC_BUTTON_H__

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MATE_VOLUME_CONTROL_TYPE_BUTTON \
  (mate_volume_control_button_get_type ())
#define MATE_VOLUME_CONTROL_BUTTON(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_VOLUME_CONTROL_TYPE_BUTTON, \
			       MateVolumeControlButton))
#define MATE_VOLUME_CONTROL_BUTTON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_VOLUME_CONTROL_TYPE_BUTTON, \
			    MateVolumeControlButtonClass))
#define MATE_VOLUME_CONTROL_IS_BUTTON(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_VOLUME_CONTROL_TYPE_BUTTON))
#define MATE_VOLUME_CONTROL_IS_BUTTON_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_VOLUME_CONTROL_TYPE_BUTTON))

typedef struct _MateVolumeControlButton {
  GtkButton parent;

  /* stock icons */
  gchar *active_icon,
	*inactive_icon;

  /* state */
  gboolean active;

  /* image */
  GtkImage *image;
} MateVolumeControlButton;

typedef struct _MateVolumeControlButtonClass {
  GtkButtonClass klass;
} MateVolumeControlButtonClass;

GType		mate_volume_control_button_get_type	(void);
GtkWidget *	mate_volume_control_button_new		(gchar   *active_icon,
							 gchar   *inactive_icon,
							 gchar   *msg);
gboolean	mate_volume_control_button_get_active	(MateVolumeControlButton *button);
void		mate_volume_control_button_set_active	(MateVolumeControlButton *button,
							 gboolean active);

G_END_DECLS

#endif /* __GVC_BUTTON_H__ */
