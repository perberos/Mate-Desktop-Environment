/* MATE Volume Control
 * Copyright (C) 2003-2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * button.c: flat toggle button with icons
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
#include <gtk/gtk.h>

#include "button.h"

G_DEFINE_TYPE (MateVolumeControlButton, mate_volume_control_button, GTK_TYPE_BUTTON)


static void	mate_volume_control_button_class_init	(MateVolumeControlButtonClass *klass);
static void	mate_volume_control_button_init	(MateVolumeControlButton *button);
static void	mate_volume_control_button_dispose	(GObject   *object);

static void	mate_volume_control_button_clicked	(GtkButton *button);

static void
mate_volume_control_button_class_init (MateVolumeControlButtonClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkButtonClass *gtkbutton_class = GTK_BUTTON_CLASS (klass);

  gobject_class->dispose = mate_volume_control_button_dispose;
  gtkbutton_class->clicked = mate_volume_control_button_clicked;
}

static void
mate_volume_control_button_init (MateVolumeControlButton *button)
{
  button->active_icon = NULL;
  button->inactive_icon = NULL;

  button->active = FALSE;
}

static void
mate_volume_control_button_dispose (GObject *object)
{
  G_OBJECT_CLASS (mate_volume_control_button_parent_class)->dispose (object);
}

GtkWidget *
mate_volume_control_button_new (gchar *active_icon,
				 gchar *inactive_icon,
				 gchar *msg)
{
  MateVolumeControlButton *button;
  GtkWidget *image;

  button = g_object_new (MATE_VOLUME_CONTROL_TYPE_BUTTON, NULL);
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  button->active_icon = active_icon;
  button->inactive_icon = inactive_icon;

  image = gtk_image_new ();
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);
  button->image = GTK_IMAGE (image);
  gtk_button_clicked (GTK_BUTTON (button));

  gtk_widget_set_tooltip_text (GTK_WIDGET (button), g_strdup (msg));

  return GTK_WIDGET (button);
}

gboolean
mate_volume_control_button_get_active (MateVolumeControlButton *button)
{
  return button->active;
}

void
mate_volume_control_button_set_active (MateVolumeControlButton *button,
					gboolean active)
{
  if (button->active != active)
    gtk_button_clicked (GTK_BUTTON (button));
}

static void
mate_volume_control_button_clicked (GtkButton *_button)
{
  MateVolumeControlButton *button = MATE_VOLUME_CONTROL_BUTTON (_button);

  button->active = !button->active;

  if (strstr (button->active_icon, ".png")) {
    gchar *filename;
    GdkPixbuf *pixbuf;

    if (button->active)
      filename = g_build_filename (PIX_DIR, button->active_icon, NULL);
    else
      filename = g_build_filename (PIX_DIR, button->inactive_icon, NULL);
   
    pixbuf = gdk_pixbuf_new_from_file (filename, NULL);
    gtk_image_set_from_pixbuf (button->image, pixbuf);
    g_object_unref (pixbuf);
    g_free (filename);
  } else {
    if (button->active) {
      gtk_image_set_from_icon_name (button->image, button->active_icon,
				GTK_ICON_SIZE_MENU);
    } else {
      gtk_image_set_from_icon_name (button->image, button->inactive_icon,
				GTK_ICON_SIZE_MENU);
    }
  }
}
