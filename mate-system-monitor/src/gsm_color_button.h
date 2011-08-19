/*
 * Mate system monitor colour pickers
 * Copyright (C) 2007 Karl Lattimer <karl@qdh.org.uk>
 * All rights reserved.
 *
 * This Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with the software; see the file COPYING. If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GSM_COLOR_BUTTON_H__
#define __GSM_COLOR_BUTTON_H__

#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <cairo.h>
#include <librsvg/rsvg.h>
#include <librsvg/rsvg-cairo.h>

G_BEGIN_DECLS
/* The GtkColorSelectionButton widget is a simple color picker in a button.	
 * The button displays a sample of the currently selected color.	When 
 * the user clicks on the button, a color selection dialog pops up.	
 * The color picker emits the "color_set" signal when the color is set.
 */
#define GSM_TYPE_COLOR_BUTTON            (gsm_color_button_get_type ())
#define GSM_COLOR_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSM_TYPE_COLOR_BUTTON, GSMColorButton))
#define GSM_COLOR_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GSM_TYPE_COLOR_BUTTON, GSMColorButtonClass))
#define GSM_IS_COLOR_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSM_TYPE_COLOR_BUTTON))
#define GSM_IS_COLOR_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GSM_TYPE_COLOR_BUTTON))
#define GSM_COLOR_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GSM_TYPE_COLOR_BUTTON, GSMColorButtonClass))
typedef struct _GSMColorButton           GSMColorButton;
typedef struct _GSMColorButtonClass      GSMColorButtonClass;
typedef struct _GSMColorButtonPrivate    GSMColorButtonPrivate;

struct _GSMColorButton
{
  GtkDrawingArea widget;

  /*< private > */

  GSMColorButtonPrivate *priv;
};

/* Widget types */
enum
{
  GSMCP_TYPE_CPU,
  GSMCP_TYPE_PIE,
  GSMCP_TYPE_NETWORK_IN,
  GSMCP_TYPE_NETWORK_OUT,
  GSMCP_TYPES
};

struct _GSMColorButtonClass
{
  GtkWidgetClass parent_class;

  void (*color_set) (GSMColorButton * cp);

  /* Padding for future expansion */
  void (*_gtk_reserved1) (void);
  void (*_gtk_reserved2) (void);
  void (*_gtk_reserved3) (void);
  void (*_gtk_reserved4) (void);
};

GType gsm_color_button_get_type (void) G_GNUC_CONST;
GtkWidget *gsm_color_button_new (const GdkColor * color, guint type);
void gsm_color_button_set_color (GSMColorButton * color_button, const GdkColor * color);
void gsm_color_button_set_fraction (GSMColorButton * color_button, const gdouble fraction);
void gsm_color_button_set_cbtype (GSMColorButton * color_button, guint type);
void gsm_color_button_get_color (GSMColorButton * color_button, GdkColor * color);
gdouble gsm_color_button_get_fraction (GSMColorButton * color_button);
guint gsm_color_button_get_cbtype (GSMColorButton * color_button);
void gsm_color_button_set_title (GSMColorButton * color_button, const gchar * title);
G_CONST_RETURN gchar *gsm_color_button_get_title (GSMColorButton * color_button);

G_END_DECLS
#endif /* __GSM_COLOR_BUTTON_H__ */
