/* gdict-aligned-window.c - Popup window aligned to a widget
 *
 * Copyright (c) 2005-2006  Emmanuele Bassi <ebassi@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,  
 * but WITHOUT ANY WARRANTY; without even the implied warranty of  
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the  
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License  
 * along with this program; if not, write to the Free Software  
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Ported from Seth Nickell's Python class:
 *   Copyright (c) 2003 Seth Nickell
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include "gdict-aligned-window.h"

#define GDICT_ALIGNED_WINDOW_GET_PRIVATE(obj)	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), GDICT_TYPE_ALIGNED_WINDOW, GdictAlignedWindowPrivate))

struct _GdictAlignedWindowPrivate
{
  GtkWidget *align_widget;
  
  guint motion_id;
};

enum
{
  PROP_0,
  
  PROP_ALIGN_WIDGET
};

static void gdict_aligned_window_finalize     (GObject      *object);
static void gdict_aligned_window_get_property (GObject      *object,
					       guint         prop_id,
					       GValue       *value,
					       GParamSpec   *pspec);
static void gdict_aligned_window_set_property (GObject      *object,
					       guint         prop_id,
					       const GValue *value,
					       GParamSpec   *pspec);

static void     gdict_aligned_window_realize          (GtkWidget        *widget);
static void     gdict_aligned_window_show             (GtkWidget        *widget);

static gboolean gdict_aligned_window_motion_notify_cb (GtkWidget        *widget,
						       GdkEventMotion   *event,
						       GdictAlignedWindow *aligned_window);


G_DEFINE_TYPE (GdictAlignedWindow, gdict_aligned_window, GTK_TYPE_WINDOW);



static void
gdict_aligned_window_class_init (GdictAlignedWindowClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  
  gobject_class->set_property = gdict_aligned_window_set_property;
  gobject_class->get_property = gdict_aligned_window_get_property;
  gobject_class->finalize = gdict_aligned_window_finalize;
  
  widget_class->realize = gdict_aligned_window_realize;
  widget_class->show = gdict_aligned_window_show;
  
  g_object_class_install_property (gobject_class, PROP_ALIGN_WIDGET,
  				   g_param_spec_object ("align-widget",
  				   			"Align Widget",
  				   			"The widget the window should align to",
  				   			GTK_TYPE_WIDGET,
  				   			G_PARAM_READWRITE));
  
  g_type_class_add_private (klass, sizeof (GdictAlignedWindowPrivate));
}

static void
gdict_aligned_window_init (GdictAlignedWindow *aligned_window)
{
  GdictAlignedWindowPrivate *priv = GDICT_ALIGNED_WINDOW_GET_PRIVATE (aligned_window);
  GtkWindow *window = GTK_WINDOW (aligned_window);
  
  aligned_window->priv = priv;
  
  priv->align_widget = NULL;
  priv->motion_id = 0;
  
  /* set window properties */
#if 0
  gtk_window_set_modal (window, TRUE);
#endif
  gtk_window_set_decorated (window, FALSE);
  gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_DOCK);
}

static void
gdict_aligned_window_get_property (GObject    *object,
				   guint       prop_id,
				   GValue     *value,
				   GParamSpec *pspec)
{
  GdictAlignedWindow *aligned_window = GDICT_ALIGNED_WINDOW (object);
  
  switch (prop_id)
    {
    case PROP_ALIGN_WIDGET:
      g_value_set_object (value, aligned_window->priv->align_widget);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gdict_aligned_window_set_property (GObject      *object,
				   guint         prop_id,
				   const GValue *value,
				   GParamSpec   *pspec)
{
  GdictAlignedWindow *aligned_window = GDICT_ALIGNED_WINDOW (object);
  
  switch (prop_id)
    {
    case PROP_ALIGN_WIDGET:
      gdict_aligned_window_set_widget (aligned_window,
      				       g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gdict_aligned_window_position (GdictAlignedWindow *window)
{
  GdictAlignedWindowPrivate *priv;
  GtkWidget *align_widget;
  gint our_width, our_height;
  gint entry_x, entry_y, entry_width, entry_height;
  gint x, y;
  GdkGravity gravity = GDK_GRAVITY_NORTH_WEST;
  GdkWindow *gdk_window;

  g_assert (GDICT_IS_ALIGNED_WINDOW (window));
  priv = window->priv;

  if (!priv->align_widget)
    return;

  align_widget = priv->align_widget;
  gdk_window = gtk_widget_get_window (align_widget);

  gdk_flush ();
  
  gdk_window_get_geometry (gtk_widget_get_window (GTK_WIDGET (window)),
  			   NULL,
  			   NULL,
  			   &our_width,
  			   &our_height,
  			   NULL);
  
  /* stick, skip taskbar and pager */
  gtk_window_stick (GTK_WINDOW (window));
  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (window), TRUE);
  gtk_window_set_skip_pager_hint (GTK_WINDOW (window), TRUE);

  /* make sure the align_widget is realized before we do anything */
  gtk_widget_realize (align_widget);
  
  /* get the positional and dimensional attributes of the align widget */
  gdk_window_get_origin (gdk_window,
  			 &entry_x,
  			 &entry_y);
  gdk_window_get_geometry (gdk_window,
  			   NULL,
  			   NULL,
  			   &entry_width,
  			   &entry_height,
  			   NULL);
  
  if (entry_x + our_width < gdk_screen_width ())
    x = entry_x + 1;
  else
    {
      x = entry_x + entry_width - our_width - 1;
      
      gravity = GDK_GRAVITY_NORTH_EAST;
    }
  
  if (entry_y + entry_height + our_height < gdk_screen_height ())
    y = entry_y + entry_height - 1;
  else
    {
      y = entry_y - our_height + 1;
      
      if (gravity == GDK_GRAVITY_NORTH_EAST)
	gravity = GDK_GRAVITY_SOUTH_EAST;
      else
	gravity = GDK_GRAVITY_SOUTH_WEST;
    }
  
  gtk_window_set_gravity (GTK_WINDOW (window), gravity);
  gtk_window_move (GTK_WINDOW (window), x, y);
}

static void
gdict_aligned_window_realize (GtkWidget *widget)
{
  GTK_WIDGET_CLASS (gdict_aligned_window_parent_class)->realize (widget);

  gdict_aligned_window_position (GDICT_ALIGNED_WINDOW (widget));
}

static void
gdict_aligned_window_show (GtkWidget *widget)
{
  gdict_aligned_window_position (GDICT_ALIGNED_WINDOW (widget));
  
  GTK_WIDGET_CLASS (gdict_aligned_window_parent_class)->show (widget);
}

static void
gdict_aligned_window_finalize (GObject *object)
{
  G_OBJECT_CLASS (gdict_aligned_window_parent_class)->finalize (object);
}

static gboolean
gdict_aligned_window_motion_notify_cb (GtkWidget        *widget,
				       GdkEventMotion   *event,
				       GdictAlignedWindow *aligned_window)
{
  GtkAllocation alloc;
  GdkRectangle rect;

  gtk_widget_get_allocation (GTK_WIDGET (aligned_window), &alloc);
  
  rect.x = 0;
  rect.y = 0;
  rect.width = alloc.width;
  rect.height = alloc.height;

  gdk_window_invalidate_rect (gtk_widget_get_window (GTK_WIDGET (aligned_window)),
		  	      &rect,
			      FALSE);
  
  return FALSE;
}


/**
 * gdict_aligned_window_new:
 * @align_widget: a #GtkWidget to which the window should align
 *
 * Creates a new window, aligned to a previously created widget.
 *
 * Return value: a new #GdictAlignedWindow
 */
GtkWidget *
gdict_aligned_window_new (GtkWidget *align_widget)
{
  return g_object_new (GDICT_TYPE_ALIGNED_WINDOW,
  		       "align-widget", align_widget,
  		       NULL);
}

/**
 * gdict_aligned_window_set_widget:
 * @aligned_window: a #GdictAlignedWindow
 * @align_widget: the #GtkWidget @aligned_window should align to
 *
 * Sets @align_widget as the #GtkWidget to which @aligned_window should
 * align.
 *
 * Note that @align_widget must have a #GdkWindow in order to
 * #GdictAlignedWindow to work.
 */
void
gdict_aligned_window_set_widget (GdictAlignedWindow *aligned_window,
			         GtkWidget          *align_widget)
{
  GdictAlignedWindowPrivate *priv;
  
  g_return_if_fail (GDICT_IS_ALIGNED_WINDOW (aligned_window));
  g_return_if_fail (GTK_IS_WIDGET (align_widget));

#if 0  
  if (GTK_WIDGET_NO_WINDOW (align_widget))
    {
      g_warning ("Attempting to set a widget of class '%s' as the "
                 "align widget, but widgets of this class does not "
                 "have a GdkWindow.",
                 g_type_name (G_OBJECT_TYPE (align_widget)));
      
      return;
    }
#endif

  priv = GDICT_ALIGNED_WINDOW_GET_PRIVATE (aligned_window);
  
  if (priv->align_widget)
    {
      g_signal_handler_disconnect (priv->align_widget, priv->motion_id);
      priv->align_widget = NULL;
    }

  priv->align_widget = align_widget;
  priv->motion_id = g_signal_connect (priv->align_widget, "motion-notify-event",
				      G_CALLBACK (gdict_aligned_window_motion_notify_cb),
				      aligned_window);
}

/**
 * gdict_aligned_window_get_widget:
 * @aligned_window: a #GdictAlignedWindow
 *
 * Retrieves the #GtkWidget to which @aligned_window is aligned to.
 *
 * Return value: the align widget.
 */
GtkWidget *
gdict_aligned_window_get_widget (GdictAlignedWindow *aligned_window)
{
  g_return_val_if_fail (GDICT_IS_ALIGNED_WINDOW (aligned_window), NULL);
  
  return aligned_window->priv->align_widget;
}
