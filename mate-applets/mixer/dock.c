/* MATE Volume Applet
 * Copyright (C) 2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * dock.c: floating window containing volume widgets
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

#include <glib-object.h>
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "dock.h"

static void	mate_volume_applet_dock_class_init	(MateVolumeAppletDockClass *klass);
static void	mate_volume_applet_dock_init		(MateVolumeAppletDock *applet);
static void	mate_volume_applet_dock_dispose	(GObject *object);

static gboolean	cb_button_press				(GtkWidget *widget,
							 GdkEventButton *button,
							 gpointer   data);
static gboolean	cb_button_release			(GtkWidget *widget,
							 GdkEventButton *button,
							 gpointer   data);
static gboolean	cb_key_press			(GtkWidget *widget,
						 GdkEventKey *event,
						 gpointer   data);

static GtkWindowClass *parent_class = NULL;

G_DEFINE_TYPE (MateVolumeAppletDock, mate_volume_applet_dock, GTK_TYPE_WINDOW)

static void
mate_volume_applet_dock_class_init (MateVolumeAppletDockClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_ref (GTK_TYPE_WINDOW);

  gobject_class->dispose = mate_volume_applet_dock_dispose;
}

static void
mate_volume_applet_dock_init (MateVolumeAppletDock *dock)
{
  dock->orientation = -1;
  dock->timeout = 0;

#if 1
  /* We can't use a simple GDK_WINDOW_TYPE_HINT_DOCK here since
   * the dock windows don't accept input by default. Instead we use
   * the popup menu type. In the end we set everything by hand anyway
   * since what happens depends very heavily on the window manager. */
//  gtk_window_set_type_hint (GTK_WINDOW (dock),
//      			    GDK_WINDOW_TYPE_HINT_POPUP_MENU);
  gtk_window_set_keep_above (GTK_WINDOW (dock), TRUE);
  gtk_window_set_decorated (GTK_WINDOW (dock), FALSE);
  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (dock), TRUE);
  gtk_window_set_skip_pager_hint (GTK_WINDOW (dock), TRUE);
  gtk_window_set_resizable (GTK_WINDOW (dock), FALSE);
  gtk_window_stick (GTK_WINDOW (dock));
#else
  /* This works well, except that keyboard focus is impossible. */
  gtk_window_set_type_hint (GTK_WINDOW (dock),
      			    GDK_WINDOW_TYPE_HINT_DOCK);
  gtk_window_set_decorated (GTK_WINDOW (dock), FALSE);
  gtk_window_set_resizable (GTK_WINDOW (dock), FALSE);
  gtk_window_stick (GTK_WINDOW (dock));
  GTK_WIDGET_SET_FLAGS (dock, GTK_CAN_FOCUS);
#endif
}

static void mute_cb (GtkToggleButton *mute_widget, MateVolumeAppletDock *dock)
{
  /* Only toggle the mute if we are actually going to change the
   * mute. This stops loops where the toggle_mute code calls us
   * back to make sure our display is in sync with other mute buttons. */
  if (mixer_is_muted (dock->model) !=
      gtk_toggle_button_get_active (mute_widget))
    mate_volume_applet_toggle_mute (dock->model);
}

static void launch_mixer_cb (GtkButton *button, MateVolumeAppletDock *dock)
{
  mate_volume_applet_run_mixer (dock->model);
}

/*
 * This is evil.
 *
 * Because we can't get a horizontal slider to behave sanely
 * with respect to up/down keys, we capture those keypress
 * and send them to the main applet - which can handle them sanely.
 * To emphasise that this is exceptional behaviour, the declarations
 * of the appropriate functions are made here rather than in a header.
 *
 */
gboolean mate_volume_applet_key (GtkWidget   *widget,
				  GdkEventKey *event);
gboolean mate_volume_applet_scroll (GtkWidget      *widget,
				     GdkEventScroll *event);

static gboolean proxy_key_event (GtkWidget *self, GdkEventKey *event,
				 GtkWidget *applet)
{
  mate_volume_applet_key (applet, event);

  return TRUE;
}

static gboolean proxy_scroll_event (GtkWidget *self, GdkEventScroll *event,
				    GtkWidget *applet)
{
  mate_volume_applet_scroll (applet, event);

  return TRUE;
}

GtkWidget *
mate_volume_applet_dock_new (GtkOrientation orientation,
			      MateVolumeApplet *parent)
{
  /* FIXME: Remove the orientation argument, or fix it for vertical
     boxes (a "horizontal" orientation - the meaning is reversed for
     historical reasons. */

  GtkWidget *button, *scale, *mute, *more, *label;
  GtkWidget *container, *outerline, *innerline, *frame;
  MateVolumeAppletDock *dock;
  gint i;
  static struct {
    GtkWidget * (* sfunc) (GtkAdjustment *adj);
    GtkWidget * (* container) (gboolean, gint);
    GtkWidget * (* subcontainer) (gboolean, gint);
    gint sw, sh;
    gboolean inverted;
  } magic[2] = {
    { gtk_vscale_new, gtk_hbox_new, gtk_vbox_new, -1, 200, TRUE},
    { gtk_hscale_new, gtk_vbox_new, gtk_hbox_new, 200, -1, FALSE}
  };

  dock = g_object_new (MATE_VOLUME_APPLET_TYPE_DOCK,
		       NULL);
  gtk_window_set_screen (GTK_WINDOW (dock),
                         gtk_widget_get_screen(GTK_WIDGET (parent)));
  dock->orientation = orientation;
  dock->model = parent;
  g_signal_connect (dock, "key_press_event", G_CALLBACK (cb_key_press),
		    NULL);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (dock), frame);

  container = magic[orientation].container (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (container), 6);
  gtk_container_add (GTK_CONTAINER (frame), container);
  outerline = magic[orientation].subcontainer (FALSE, 0);
  innerline = magic[orientation].subcontainer (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (container), outerline, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (container), innerline, FALSE, FALSE, 0);

  dock->minus = GTK_BUTTON (gtk_button_new ());
  gtk_box_pack_start (GTK_BOX (outerline), GTK_WIDGET (dock->minus),
		      FALSE, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (dock->minus),
		     gtk_image_new_from_stock (GTK_STOCK_REMOVE,
					       GTK_ICON_SIZE_BUTTON));
  dock->plus = GTK_BUTTON (gtk_button_new ());
  gtk_box_pack_end (GTK_BOX (outerline), GTK_WIDGET (dock->plus),
		    FALSE, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (dock->plus),
		     gtk_image_new_from_stock (GTK_STOCK_ADD,
					       GTK_ICON_SIZE_BUTTON));

  button = GTK_WIDGET (dock->plus);
  for (i = 0; i<2; i++) { /* For button in (dock->plus, dock->minus): */
    gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
    g_signal_connect (button, "button-press-event",
		      G_CALLBACK (cb_button_press), dock);
    g_signal_connect (button, "button-release-event",
		      G_CALLBACK (cb_button_release), dock);
    button = GTK_WIDGET (dock->minus);
  }

  scale = magic[orientation].sfunc (NULL);
  g_signal_connect (scale, "key-press-event", G_CALLBACK (proxy_key_event),
		    parent);
  g_signal_connect (scale, "scroll-event", G_CALLBACK (proxy_scroll_event),
		    parent);
  dock->scale = GTK_RANGE (scale);
  gtk_widget_set_size_request (scale,
			       magic[orientation].sw,
			       magic[orientation].sh);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
  gtk_range_set_inverted (dock->scale, magic[orientation].inverted);
  gtk_box_pack_start (GTK_BOX (outerline), GTK_WIDGET (dock->scale),
		      TRUE, TRUE, 0);

  dock->mute = gtk_check_button_new_with_label (_("Mute"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dock->mute),
				mixer_is_muted (dock->model));
  g_signal_connect (dock->mute, "toggled", G_CALLBACK (mute_cb), dock);
  gtk_box_pack_start (GTK_BOX (innerline), dock->mute, TRUE, TRUE, 0);

  more = gtk_button_new_with_label (_("Volume Control..."));
  g_signal_connect (more, "clicked", G_CALLBACK (launch_mixer_cb), dock);
  gtk_box_pack_end (GTK_BOX (innerline), more, TRUE, TRUE, 0);

  gtk_container_add (GTK_CONTAINER (dock), frame);

  return GTK_WIDGET (dock);
}

static void
destroy_source (MateVolumeAppletDock *dock)
{
  if (dock->timeout) {
    g_source_remove (dock->timeout);
    dock->timeout = 0;
  }
}

static void
mate_volume_applet_dock_dispose (GObject *object)
{
  MateVolumeAppletDock *dock = MATE_VOLUME_APPLET_DOCK (object);

  destroy_source (dock);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

/*
 * Change the value of the slider. This is called both from a direct
 * call from the +/- button callbacks and via a timer so holding down the
 * buttons changes the volume.
 */

static gboolean
cb_timeout (gpointer data)
{
  MateVolumeAppletDock *dock = data;
  GtkAdjustment *adj;
  gfloat volume;
  gboolean res = TRUE;

  if (!dock->timeout)
    return FALSE;

  adj = gtk_range_get_adjustment (dock->scale);
  volume = gtk_range_get_value (dock->scale);
  volume += dock->direction * gtk_adjustment_get_step_increment (adj);

  if (volume <= gtk_adjustment_get_lower (adj)) {
    volume = gtk_adjustment_get_lower (adj);
    res = FALSE;
  } else if (volume >= gtk_adjustment_get_upper (adj)) {
    volume = gtk_adjustment_get_upper (adj);
    res = FALSE;
  }

  gtk_range_set_value (dock->scale, volume);

  if (!res)
    dock->timeout = 0;

  return res;
}

/*
 * React if user presses +/- buttons.
 */

static gboolean
cb_button_press (GtkWidget *widget,
		 GdkEventButton *button,
		 gpointer   data)
{
  MateVolumeAppletDock *dock = data;

  dock->direction = (GTK_BUTTON (widget) == dock->plus) ? 1 : -1;
  destroy_source (dock);
  dock->timeout = g_timeout_add (100, cb_timeout, data);
  cb_timeout (data);

  return TRUE;
}

static gboolean
cb_button_release (GtkWidget *widget,
		   GdkEventButton *button,
		   gpointer   data)
{
  MateVolumeAppletDock *dock = data;

  destroy_source (dock);

  return TRUE;
}

static gboolean
cb_key_press (GtkWidget *widget,
	      GdkEventKey *event,
	      gpointer data)
{

  /* Trap the escape key to popdown the dock. */
  if (event->keyval == GDK_Escape) {
    /* This is trickier than it looks. The main applet is watching for
     * this widget to loose focus. Hiding the widget causes a
     * focus-loss, thus the applet gets the focus-out signal and all
     * the book-keeping gets done (like setting the applet button
     * hilight) without an explicit callback. */
    gtk_widget_hide (widget);
  }

  return FALSE;
}

/*
 * Set the adjustment for the slider.
 */

void
mate_volume_applet_dock_change (MateVolumeAppletDock *dock,
				 GtkAdjustment *adj)
{
  gtk_range_set_adjustment (dock->scale, adj);
}

void
mate_volume_applet_dock_set_focus (MateVolumeAppletDock *dock)
{
  gtk_widget_grab_focus (GTK_WIDGET (dock->scale));
}
