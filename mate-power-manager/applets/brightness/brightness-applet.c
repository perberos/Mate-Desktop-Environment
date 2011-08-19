/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * MATE Power Manager Brightness Applet
 * Copyright (C) 2006 Benjamin Canou <bookeldor@gmail.com>
 * Copyright (C) 2007 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

/* FIXME: gdk_gc_* needs porting to cairo */
#undef GDK_DISABLE_DEPRECATED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mate-panel-applet.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib-object.h>
#include <dbus/dbus-glib.h>

#include "egg-debug.h"
#include "egg-dbus-monitor.h"
#include "gpm-common.h"

#define GPM_TYPE_BRIGHTNESS_APPLET		(gpm_brightness_applet_get_type ())
#define GPM_BRIGHTNESS_APPLET(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GPM_TYPE_BRIGHTNESS_APPLET, GpmBrightnessApplet))
#define GPM_BRIGHTNESS_APPLET_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GPM_TYPE_BRIGHTNESS_APPLET, GpmBrightnessAppletClass))
#define GPM_IS_BRIGHTNESS_APPLET(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GPM_TYPE_BRIGHTNESS_APPLET))
#define GPM_IS_BRIGHTNESS_APPLET_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GPM_TYPE_BRIGHTNESS_APPLET))
#define GPM_BRIGHTNESS_APPLET_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GPM_TYPE_BRIGHTNESS_APPLET, GpmBrightnessAppletClass))

typedef struct{
	MatePanelApplet parent;
	/* applet state */
	gboolean call_worked; /* g-p-m refusing action */
	gboolean popped; /* the popup is shown */
	/* the popup and its widgets */
	GtkWidget *popup, *slider, *btn_plus, *btn_minus;
	/* the icon and a cache for size*/
	GdkPixbuf *icon;
	gint icon_width, icon_height;
	/* connection to g-p-m */
	DBusGProxy *proxy;
	DBusGConnection *connection;
	EggDbusMonitor *monitor;
	guint level;
	/* a cache for panel size */
	gint size;
} GpmBrightnessApplet;

typedef struct{
	MatePanelAppletClass	parent_class;
} GpmBrightnessAppletClass;

GType                gpm_brightness_applet_get_type  (void);


static void      gpm_brightness_applet_class_init (GpmBrightnessAppletClass *klass);
static void      gpm_brightness_applet_init       (GpmBrightnessApplet *applet);

G_DEFINE_TYPE (GpmBrightnessApplet, gpm_brightness_applet, PANEL_TYPE_APPLET)

static void      gpm_applet_get_icon              (GpmBrightnessApplet *applet);
static void      gpm_applet_check_size            (GpmBrightnessApplet *applet);
static gboolean  gpm_applet_draw_cb               (GpmBrightnessApplet *applet);
static void      gpm_applet_change_background_cb  (GpmBrightnessApplet *applet,
						   MatePanelAppletBackgroundType arg1,
						   GdkColor *arg2, GdkPixmap *arg3, gpointer data);
static void      gpm_applet_theme_change_cb (GtkIconTheme *icon_theme, gpointer data);
static void      gpm_applet_stop_scroll_events_cb (GtkWidget *widget, GdkEvent  *event);
static gboolean  gpm_applet_destroy_popup_cb      (GpmBrightnessApplet *applet);
static void      gpm_applet_update_tooltip        (GpmBrightnessApplet *applet);
static void      gpm_applet_update_popup_level    (GpmBrightnessApplet *applet);
static gboolean  gpm_applet_plus_cb               (GtkWidget *w, GpmBrightnessApplet *applet);
static gboolean  gpm_applet_minus_cb              (GtkWidget *w, GpmBrightnessApplet *applet);
static gboolean  gpm_applet_key_press_cb          (GpmBrightnessApplet *applet, GdkEventKey *event);
static gboolean  gpm_applet_scroll_cb             (GpmBrightnessApplet *applet, GdkEventScroll *event);
static gboolean  gpm_applet_slide_cb              (GtkWidget *w, GpmBrightnessApplet *applet);
static void      gpm_applet_create_popup          (GpmBrightnessApplet *applet);
static gboolean  gpm_applet_popup_cb              (GpmBrightnessApplet *applet, GdkEventButton *event);
static void      gpm_applet_dialog_about_cb       (MateComponentUIComponent *uic, gpointer data, const gchar *verbname);
static gboolean  gpm_applet_matecomponent_cb             (MatePanelApplet *_applet, const gchar *iid, gpointer data);
static void      gpm_applet_destroy_cb            (GtkObject *object);

#define GPM_BRIGHTNESS_APPLET_OAFID		"OAFIID:MATE_BrightnessApplet"
#define GPM_BRIGHTNESS_APPLET_FACTORY_OAFID	"OAFIID:MATE_BrightnessApplet_Factory"
#define GPM_BRIGHTNESS_APPLET_ICON		"mate-brightness-applet"
#define GPM_BRIGHTNESS_APPLET_ICON_DISABLED	"gpm-brightness-lcd-disabled"
#define GPM_BRIGHTNESS_APPLET_ICON_INVALID	"gpm-brightness-lcd-invalid"
#define GPM_BRIGHTNESS_APPLET_NAME		_("Power Manager Brightness Applet")
#define GPM_BRIGHTNESS_APPLET_DESC		_("Adjusts laptop panel brightness.")
#define MATE_PANEL_APPLET_VERTICAL(p)					\
	 (((p) == MATE_PANEL_APPLET_ORIENT_LEFT) || ((p) == MATE_PANEL_APPLET_ORIENT_RIGHT))

/**
 * gpm_applet_get_brightness:
 * Return value: Success value, or zero for failure
 **/
static gboolean
gpm_applet_get_brightness (GpmBrightnessApplet *applet)
{
	GError  *error = NULL;
	gboolean ret;
	guint policy_brightness;

	if (applet->proxy == NULL) {
		egg_warning ("not connected\n");
		return FALSE;
	}

	ret = dbus_g_proxy_call (applet->proxy, "GetBrightness", &error,
				 G_TYPE_INVALID,
				 G_TYPE_UINT, &policy_brightness,
				 G_TYPE_INVALID);
	if (error) {
		egg_debug ("ERROR: %s\n", error->message);
		g_error_free (error);
	}
	if (ret) {
		applet->level = policy_brightness;
	} else {
		/* abort as the DBUS method failed */
		egg_warning ("GetBrightness failed!\n");
	}

	return ret;
}

/**
 * gpm_applet_set_brightness:
 * Return value: Success value, or zero for failure
 **/
static gboolean
gpm_applet_set_brightness (GpmBrightnessApplet *applet)
{
	GError  *error = NULL;
	gboolean ret;

	if (applet->proxy == NULL) {
		egg_warning ("not connected");
		return FALSE;
	}

	ret = dbus_g_proxy_call (applet->proxy, "SetBrightness", &error,
				 G_TYPE_UINT, applet->level,
				 G_TYPE_INVALID,
				 G_TYPE_INVALID);
	if (error) {
		egg_debug ("ERROR: %s", error->message);
		g_error_free (error);
	}
	if (!ret) {
		/* abort as the DBUS method failed */
		egg_warning ("SetBrightness failed!");
	}

	return ret;
}

/**
 * gpm_applet_get_icon:
 * @applet: Brightness applet instance
 *
 * retrieve an icon from stock with a size adapted to panel
 **/
static void
gpm_applet_get_icon (GpmBrightnessApplet *applet)
{
	const gchar *icon;

	/* free */
	if (applet->icon != NULL) {
		g_object_unref (applet->icon);
		applet->icon = NULL;
	}

	if (applet->size <= 2) {
		return;
	}

	/* get icon */
	if (applet->proxy == NULL) {
		icon = GPM_BRIGHTNESS_APPLET_ICON_INVALID;
	} else if (applet->call_worked == FALSE) {
		icon = GPM_BRIGHTNESS_APPLET_ICON_DISABLED;
	} else {
		icon = GPM_BRIGHTNESS_APPLET_ICON;
	}

	applet->icon = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
						 icon, applet->size - 2, 0, NULL);

	if (applet->icon == NULL) {
		egg_warning ("Cannot find %s!\n", icon);
	} else {
		egg_debug ("got icon %s!\n", icon);
		/* update size cache */
		applet->icon_height = gdk_pixbuf_get_height (applet->icon);
		applet->icon_width = gdk_pixbuf_get_width (applet->icon);
	}
}

/**
 * gpm_applet_check_size:
 * @applet: Brightness applet instance
 *
 * check if panel size has changed and applet adapt size
 **/
static void
gpm_applet_check_size (GpmBrightnessApplet *applet)
{
	GtkAllocation allocation;

	/* we don't use the size function here, but the yet allocated size because the
	   size value is false (kind of rounded) */
	gtk_widget_get_allocation (GTK_WIDGET (applet), &allocation);
	if (MATE_PANEL_APPLET_VERTICAL(mate_panel_applet_get_orient (MATE_PANEL_APPLET (applet)))) {
		if (applet->size != allocation.width) {
			applet->size = allocation.width;
			gpm_applet_get_icon (applet);
			gtk_widget_set_size_request (GTK_WIDGET(applet), applet->size, applet->icon_height + 2);
		}
	} else {
		if (applet->size != allocation.height) {
			applet->size = allocation.height;
			gpm_applet_get_icon (applet);
			gtk_widget_set_size_request (GTK_WIDGET(applet), applet->icon_width + 2, applet->size);
		}
	}
}

/**
 * gpm_applet_draw_cb:
 * @applet: Brightness applet instance
 *
 * draws applet content (background + icon)
 **/
static gboolean
gpm_applet_draw_cb (GpmBrightnessApplet *applet)
{
	gint w, h, bg_type;
	GdkColor color;
	GdkGC *gc;
	GdkPixmap *background;
	GtkAllocation allocation;

	if (gtk_widget_get_window (GTK_WIDGET(applet)) == NULL) {
		return FALSE;
	}

	/* retrieve applet size */
	gpm_applet_get_icon (applet);
	gpm_applet_check_size (applet);
	if (applet->size <= 2) {
		return FALSE;
	}

	/* if no icon, then don't try to display */
	if (applet->icon == NULL) {
		return FALSE;
	}

	gtk_widget_get_allocation (GTK_WIDGET (applet), &allocation);
	w = allocation.width;
	h = allocation.height;

	gc = gdk_gc_new (gtk_widget_get_window (GTK_WIDGET(applet)));

	/* draw pixmap background */
	bg_type = mate_panel_applet_get_background (MATE_PANEL_APPLET (applet), &color, &background);
	if (bg_type == PANEL_PIXMAP_BACKGROUND && !applet->popped) {
		/* fill with given background pixmap */
		gdk_draw_drawable (gtk_widget_get_window (GTK_WIDGET(applet)), gc, background, 0, 0, 0, 0, w, h);
	}
	
	/* draw color background */
	if (bg_type == PANEL_COLOR_BACKGROUND && !applet->popped) {
		gdk_gc_set_rgb_fg_color (gc,&color);
		gdk_gc_set_fill (gc,GDK_SOLID);
		gdk_draw_rectangle (gtk_widget_get_window (GTK_WIDGET(applet)), gc, TRUE, 0, 0, w, h);
	}

	/* fill with selected color if popped */
	if (applet->popped) {
		color = gtk_rc_get_style (GTK_WIDGET(applet))->bg[GTK_STATE_SELECTED];
		gdk_gc_set_rgb_fg_color (gc,&color);
		gdk_gc_set_fill (gc,GDK_SOLID);
		gdk_draw_rectangle (gtk_widget_get_window (GTK_WIDGET(applet)), gc, TRUE, 0, 0, w, h);
	}

	/* draw icon at center */
	gdk_draw_pixbuf (gtk_widget_get_window (GTK_WIDGET(applet)), gc, applet->icon,
			 0, 0, (w - applet->icon_width)/2, (h - applet->icon_height)/2,
			 applet->icon_width, applet->icon_height,
			 GDK_RGB_DITHER_NONE, 0, 0);

	g_object_unref (gc);

	return TRUE;
}

/**
 * gpm_applet_change_background_cb:
 *
 * Enqueues an expose event (don't know why it's not the default behaviour)
 **/
static void
gpm_applet_change_background_cb (GpmBrightnessApplet *applet,
				 MatePanelAppletBackgroundType arg1,
				 GdkColor *arg2, GdkPixmap *arg3, gpointer data)
{
	gtk_widget_queue_draw (GTK_WIDGET (applet));
}

/**
 * gpm_applet_destroy_popup_cb:
 * @applet: Brightness applet instance
 *
 * destroys the popup (called if orientation has changed)
 **/
static gboolean
gpm_applet_destroy_popup_cb (GpmBrightnessApplet *applet)
{
	if (applet->popup != NULL) {
		gtk_widget_set_parent (applet->popup, NULL);
		gtk_widget_destroy (applet->popup);
		applet->popup = NULL;
	}
	return TRUE;
}

/**
 * gpm_applet_update_tooltip:
 * @applet: Brightness applet instance
 *
 * sets tooltip's content (percentage or disabled)
 **/
static void
gpm_applet_update_tooltip (GpmBrightnessApplet *applet)
{
	gchar *buf = NULL;
	if (applet->popped == FALSE) {
		if (applet->proxy == NULL) {
			buf = g_strdup (_("Cannot connect to mate-power-manager"));
		} else if (applet->call_worked == FALSE) {
			buf = g_strdup (_("Cannot get laptop panel brightness"));
		} else {
			buf = g_strdup_printf (_("LCD brightness : %d%%"), applet->level);
		}
		gtk_widget_set_tooltip_text (GTK_WIDGET(applet), buf);
	} else {
		gtk_widget_set_tooltip_text (GTK_WIDGET(applet), NULL);
	}
	g_free (buf);
}

/**
 * gpm_applet_update_popup_level:
 * @applet: Brightness applet instance
 * @get_hw: set UI value from HW value
 * @set_hw: set HW value from UI value
 *
 * updates popup and hardware level of brightness
 * FALSE FAlSE -> set UI from cached value
 * TRUE  FAlSE -> set UI from HW value
 * TRUE  FALSE -> set HW from UI value, then set UI from HW value
 * FALSE TRUE  -> set HW from UI value
 **/
static void
gpm_applet_update_popup_level (GpmBrightnessApplet *applet)
{
	if (applet->popup != NULL) {
		gtk_widget_set_sensitive (applet->btn_plus, applet->level < 100);
		gtk_widget_set_sensitive (applet->btn_minus, applet->level > 0);
		gtk_range_set_value (GTK_RANGE(applet->slider), (guint) applet->level);
	}
	gpm_applet_update_tooltip (applet);
}

/**
 * gpm_applet_plus_cb:
 * @widget: The sending widget (plus button)
 * @applet: Brightness applet instance
 *
 * callback for the plus button
 **/
static gboolean
gpm_applet_plus_cb (GtkWidget *w, GpmBrightnessApplet *applet)
{
	if (applet->level < 100) {
		applet->level++;
	}
	applet->call_worked = gpm_applet_set_brightness (applet);
	gpm_applet_update_popup_level (applet);
	return TRUE;
}

/**
 * gpm_applet_minus_cb:
 * @widget: The sending widget (minus button)
 * @applet: Brightness applet instance
 *
 * callback for the minus button
 **/
static gboolean
gpm_applet_minus_cb (GtkWidget *w, GpmBrightnessApplet *applet)
{
	if (applet->level > 0) {
		applet->level--;
	}
	applet->call_worked = gpm_applet_set_brightness (applet);
	gpm_applet_update_popup_level (applet);
	return TRUE;
}

/**
 * gpm_applet_slide_cb:
 * @widget: The sending widget (slider)
 * @applet: Brightness applet instance
 *
 * callback for the slider
 **/
static gboolean
gpm_applet_slide_cb (GtkWidget *w, GpmBrightnessApplet *applet)
{
	applet->level = gtk_range_get_value (GTK_RANGE(applet->slider));
	applet->call_worked = gpm_applet_set_brightness (applet);
	gpm_applet_update_popup_level (applet);
	return TRUE;
}

/**
 * gpm_applet_slide_cb:
 * @applet: Brightness applet instance
 * @event: The key press event
 *
 * callback handling keyboard
 * mainly escape to unpop and arrows to change brightness
 **/
static gboolean
gpm_applet_key_press_cb (GpmBrightnessApplet *applet, GdkEventKey *event)
{
	int i;
	
	switch (event->keyval) {
	case GDK_KP_Enter:
	case GDK_ISO_Enter:
	case GDK_3270_Enter:
	case GDK_Return:
	case GDK_space:
	case GDK_KP_Space:
	case GDK_Escape:
		/* if yet popped, release focus and hide then redraw applet unselected */
		if (applet->popped) {
			gdk_keyboard_ungrab (GDK_CURRENT_TIME);
			gdk_pointer_ungrab (GDK_CURRENT_TIME);
			gtk_grab_remove (GTK_WIDGET(applet));
			gtk_widget_set_state (GTK_WIDGET(applet), GTK_STATE_NORMAL);
			gtk_widget_hide (applet->popup);
			applet->popped = FALSE;
			gpm_applet_draw_cb (applet);
			gpm_applet_update_tooltip (applet);
			return TRUE;
		} else {
			return FALSE;
		}
		break;
	case GDK_Page_Up:
		for (i = 0;i < 10;i++) {
			gpm_applet_plus_cb (NULL, applet);
		}
		return TRUE;
		break;
	case GDK_Left:
	case GDK_Up:
		gpm_applet_plus_cb (NULL, applet);
		return TRUE;
		break;
	case GDK_Page_Down:
		for (i = 0;i < 10;i++) {
			gpm_applet_minus_cb (NULL, applet);
		}
		return TRUE;
		break;
	case GDK_Right:
	case GDK_Down:
		gpm_applet_minus_cb (NULL, applet);
		return TRUE;
		break;
	default:
		return FALSE;
		break;
	}

	return FALSE;
}

/**
 * gpm_applet_scroll_cb:
 * @applet: Brightness applet instance
 * @event: The scroll event
 *
 * callback handling mouse scrolls, either when the applet
 * is not popped and the mouse is over the applet, or when
 * the applet is popped and no matter where the mouse is.
 **/
static gboolean
gpm_applet_scroll_cb (GpmBrightnessApplet *applet, GdkEventScroll *event)
{
	int i;

	if (event->type == GDK_SCROLL) {
		if (event->direction == GDK_SCROLL_UP) {
			for (i = 0;i < 5;i++) {
				gpm_applet_plus_cb (NULL, applet);
			}
			
		} else {
			for (i = 0;i < 5;i++) {
				gpm_applet_minus_cb (NULL, applet);
			}
		}
		return TRUE;
	}

	return FALSE;
}

/**
 * gpm_applet_create_popup:
 * @applet: Brightness applet instance
 *
 * cretes a new popup according to orientation of panel
 **/
static void
gpm_applet_create_popup (GpmBrightnessApplet *applet)
{
	static GtkWidget *box, *frame;
	gint orientation = mate_panel_applet_get_orient (MATE_PANEL_APPLET (MATE_PANEL_APPLET (applet)));

	gpm_applet_destroy_popup_cb (applet);

	/* slider */
	if (MATE_PANEL_APPLET_VERTICAL(orientation)) {
		applet->slider = gtk_hscale_new_with_range (0, 100, 1);
		gtk_widget_set_size_request (applet->slider, 100, -1);
	} else {
		applet->slider = gtk_vscale_new_with_range (0, 100, 1);
		gtk_widget_set_size_request (applet->slider, -1, 100);
	}
	gtk_range_set_inverted (GTK_RANGE(applet->slider), TRUE);
	gtk_scale_set_draw_value (GTK_SCALE(applet->slider), FALSE);
	gtk_range_set_value (GTK_RANGE(applet->slider), applet->level);
	g_signal_connect (G_OBJECT(applet->slider), "value-changed", G_CALLBACK(gpm_applet_slide_cb), applet);

	/* minus button */
	applet->btn_minus = gtk_button_new_with_label ("\342\210\222"); /* U+2212 MINUS SIGN */
	gtk_button_set_relief (GTK_BUTTON(applet->btn_minus), GTK_RELIEF_NONE);
	g_signal_connect (G_OBJECT(applet->btn_minus), "pressed", G_CALLBACK(gpm_applet_minus_cb), applet);

	/* plus button */
	applet->btn_plus = gtk_button_new_with_label ("+");
	gtk_button_set_relief (GTK_BUTTON(applet->btn_plus), GTK_RELIEF_NONE);
	g_signal_connect (G_OBJECT(applet->btn_plus), "pressed", G_CALLBACK(gpm_applet_plus_cb), applet);

	/* box */
	if (MATE_PANEL_APPLET_VERTICAL(orientation)) {
		box = gtk_hbox_new (FALSE, 1);
	} else {
		box = gtk_vbox_new (FALSE, 1);
	}
	gtk_box_pack_start (GTK_BOX(box), applet->btn_plus, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX(box), applet->slider, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(box), applet->btn_minus, FALSE, FALSE, 0);

	/* frame */
	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME(frame), GTK_SHADOW_OUT);
	gtk_container_add (GTK_CONTAINER(frame), box);

	/* window */
	applet->popup = gtk_window_new (GTK_WINDOW_POPUP);
	gtk_window_set_type_hint (GTK_WINDOW(applet->popup), GDK_WINDOW_TYPE_HINT_UTILITY);
	gtk_widget_set_parent (applet->popup, GTK_WIDGET(applet));
	gtk_container_add (GTK_CONTAINER(applet->popup), frame);
}

/**
 * gpm_applet_popup_cb:
 * @applet: Brightness applet instance
 *
 * pops and unpops
 **/
static gboolean
gpm_applet_popup_cb (GpmBrightnessApplet *applet, GdkEventButton *event)
{
	GtkAllocation allocation, popup_allocation;
	gint orientation, x, y;

	/* react only to left mouse button */
	if (event->button != 1) {
		return FALSE;
	}

	/* if yet popped, release focus and hide then redraw applet unselected */
	if (applet->popped) {
		gdk_keyboard_ungrab (GDK_CURRENT_TIME);
		gdk_pointer_ungrab (GDK_CURRENT_TIME);
		gtk_grab_remove (GTK_WIDGET(applet));
		gtk_widget_set_state (GTK_WIDGET(applet), GTK_STATE_NORMAL);
		gtk_widget_hide (applet->popup);
		applet->popped = FALSE;
		gpm_applet_draw_cb (applet);
		gpm_applet_update_tooltip (applet);
		return TRUE;
	}

	/* update UI for current brightness */
	gpm_applet_update_popup_level (applet);

	/* otherwise pop */
	applet->popped = TRUE;
	gpm_applet_draw_cb (applet);

	/* create a new popup (initial or if panel parameters changed) */
	if (applet->popup == NULL) {
		gpm_applet_create_popup (applet);
	}

	/* update UI for current brightness */
	gpm_applet_update_popup_level (applet);

	gtk_widget_show_all (applet->popup);

	/* retrieve geometry parameters and move window appropriately */
	orientation = mate_panel_applet_get_orient (MATE_PANEL_APPLET (MATE_PANEL_APPLET (applet)));
	gdk_window_get_origin (gtk_widget_get_window (GTK_WIDGET(applet)), &x, &y);

	gtk_widget_get_allocation (GTK_WIDGET (applet), &allocation);
	gtk_widget_get_allocation (GTK_WIDGET (applet->popup), &popup_allocation);
	switch (orientation) {
	case MATE_PANEL_APPLET_ORIENT_DOWN:
		x += allocation.x + allocation.width/2;
		y += allocation.y + allocation.height;
		x -= popup_allocation.width/2;
		break;
	case MATE_PANEL_APPLET_ORIENT_UP:
		x += allocation.x + allocation.width/2;
		y += allocation.y;
		x -= popup_allocation.width/2;
		y -= popup_allocation.height;
		break;
	case MATE_PANEL_APPLET_ORIENT_RIGHT:
		y += allocation.y + allocation.height/2;
		x += allocation.x + allocation.width;
		y -= popup_allocation.height/2;
		break;
	case MATE_PANEL_APPLET_ORIENT_LEFT:
		y += allocation.y + allocation.height/2;
		x += allocation.x;
		x -= popup_allocation.width;
		y -= popup_allocation.height/2;
		break;
	default:
		g_assert_not_reached ();
	}

	gtk_window_move (GTK_WINDOW (applet->popup), x, y);

	/* grab input */
	gtk_widget_grab_focus (GTK_WIDGET(applet));
	gtk_grab_add (GTK_WIDGET(applet));
	gdk_pointer_grab (gtk_widget_get_window (GTK_WIDGET(applet)), TRUE,
			  GDK_BUTTON_PRESS_MASK |
			  GDK_BUTTON_RELEASE_MASK |
			  GDK_POINTER_MOTION_MASK,
			  NULL, NULL, GDK_CURRENT_TIME);
	gdk_keyboard_grab (gtk_widget_get_window (GTK_WIDGET(applet)),
			   TRUE, GDK_CURRENT_TIME);
	gtk_widget_set_state (GTK_WIDGET(applet), GTK_STATE_SELECTED);

	return TRUE;
}

/**
 * gpm_applet_theme_change_cb:
 *
 * Updtes icon when theme changes
 **/
static void
gpm_applet_theme_change_cb (GtkIconTheme *icon_theme, gpointer data)
{
	GpmBrightnessApplet *applet = GPM_BRIGHTNESS_APPLET (data);
	gpm_applet_get_icon (applet);
}

/**
 * gpm_applet_stop_scroll_events_cb:
 *
 * Prevents scroll events from reaching the tooltip
 **/
static void
gpm_applet_stop_scroll_events_cb (GtkWidget *widget, GdkEvent  *event)
{
	if (event->type == GDK_SCROLL)
		g_signal_stop_emission_by_name (widget, "event-after");
}

/**
 * gpm_applet_dialog_about_cb:
 *
 * displays about dialog
 **/
static void
gpm_applet_dialog_about_cb (MateComponentUIComponent *uic, gpointer data, const gchar *verbname)
{
	GtkAboutDialog *about;

	GdkPixbuf *logo =
		gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
					  GPM_BRIGHTNESS_APPLET_ICON,
					  128, 0, NULL);

	static const gchar *authors[] = {
		"Benjamin Canou <bookeldor@gmail.com>",
		"Richard Hughes <richard@hughsie.com>",
		NULL
	};
	const char *documenters [] = {
		NULL
	};
	const char *license[] = {
		 N_("Licensed under the GNU General Public License Version 2"),
		 N_("Power Manager is free software; you can redistribute it and/or\n"
		   "modify it under the terms of the GNU General Public License\n"
		   "as published by the Free Software Foundation; either version 2\n"
		   "of the License, or (at your option) any later version."),
		 N_("Power Manager is distributed in the hope that it will be useful,\n"
		   "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		   "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
		   "GNU General Public License for more details."),
		 N_("You should have received a copy of the GNU General Public License\n"
		   "along with this program; if not, write to the Free Software\n"
		   "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA\n"
		   "02110-1301, USA.")
	};
	const char *translator_credits = NULL;
	char	   *license_trans;

	license_trans = g_strconcat (_(license[0]), "\n\n", _(license[1]), "\n\n",
				     _(license[2]), "\n\n", _(license[3]), "\n", NULL);

	about = (GtkAboutDialog*) gtk_about_dialog_new ();
	gtk_about_dialog_set_program_name (about, GPM_BRIGHTNESS_APPLET_NAME);
	gtk_about_dialog_set_version (about, VERSION);
	gtk_about_dialog_set_copyright (about, _("Copyright \xc2\xa9 2006 Benjamin Canou"));
	gtk_about_dialog_set_comments (about, GPM_BRIGHTNESS_APPLET_DESC);
	gtk_about_dialog_set_authors (about, authors);
	gtk_about_dialog_set_documenters (about, documenters);
	gtk_about_dialog_set_translator_credits (about, translator_credits);
	gtk_about_dialog_set_logo (about, logo);
	gtk_about_dialog_set_license (about, license_trans);
	gtk_about_dialog_set_website (about, GPM_HOMEPAGE_URL);

	g_signal_connect (G_OBJECT(about), "response",
			  G_CALLBACK(gtk_widget_destroy), NULL);

	gtk_widget_show (GTK_WIDGET(about));

	g_free (license_trans);
	g_object_unref (logo);
}

/**
 * gpm_applet_help_cb:
 *
 * open gpm help
 **/
static void
gpm_applet_help_cb (MateComponentUIComponent *uic, gpointer data, const gchar *verbname)
{
	gpm_help_display ("applets-brightness");
}

/**
 * gpm_applet_destroy_cb:
 * @object: Class instance to destroy
 **/
static void
gpm_applet_destroy_cb (GtkObject *object)
{
	GpmBrightnessApplet *applet = GPM_BRIGHTNESS_APPLET(object);

	if (applet->monitor != NULL) {
		g_object_unref (applet->monitor);
	}
	if (applet->icon != NULL) {
		g_object_unref (applet->icon);
	}
}

/**
 * gpm_brightness_applet_class_init:
 * @klass: Class instance
 **/
static void
gpm_brightness_applet_class_init (GpmBrightnessAppletClass *class)
{
	/* nothing to do here */
}

static void
brightness_changed_cb (DBusGProxy          *proxy,
		       guint	            brightness,
		       GpmBrightnessApplet *applet)
{
	egg_debug ("BrightnessChanged detected: %i\n", brightness);
	applet->level = brightness;
}

/**
 * gpm_brightness_applet_dbus_connect:
 **/
gboolean
gpm_brightness_applet_dbus_connect (GpmBrightnessApplet *applet)
{
	GError *error = NULL;

	if (applet->connection == NULL) {
		egg_debug ("get connection\n");
		g_clear_error (&error);
		applet->connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
		if (error != NULL) {
			egg_warning ("Could not connect to DBUS daemon: %s", error->message);
			g_error_free (error);
			applet->connection = NULL;
			return FALSE;
		}
	}
	if (applet->proxy == NULL) {
		egg_debug ("get proxy\n");
		g_clear_error (&error);
		applet->proxy = dbus_g_proxy_new_for_name_owner (applet->connection,
							 GPM_DBUS_SERVICE,
							 GPM_DBUS_PATH_BACKLIGHT,
							 GPM_DBUS_INTERFACE_BACKLIGHT,
							 &error);
		if (error != NULL) {
			egg_warning ("Cannot connect, maybe the daemon is not running: %s\n", error->message);
			g_error_free (error);
			applet->proxy = NULL;
			return FALSE;
		}
		dbus_g_proxy_add_signal (applet->proxy, "BrightnessChanged",
					 G_TYPE_UINT, G_TYPE_INVALID);
		dbus_g_proxy_connect_signal (applet->proxy, "BrightnessChanged",
					     G_CALLBACK (brightness_changed_cb),
					     applet, NULL);
		/* reset, we might be starting race */
		applet->call_worked = gpm_applet_get_brightness (applet);
	}
	return TRUE;
}

/**
 * gpm_brightness_applet_dbus_disconnect:
 **/
gboolean
gpm_brightness_applet_dbus_disconnect (GpmBrightnessApplet *applet)
{
	if (applet->proxy != NULL) {
		egg_debug ("removing proxy\n");
		g_object_unref (applet->proxy);
		applet->proxy = NULL;
	}
	return TRUE;
}

/**
 * monitor_connection_cb:
 * @proxy: The dbus raw proxy
 * @status: The status of the service, where TRUE is connected
 * @screensaver: This class instance
 **/
static void
monitor_connection_cb (EggDbusMonitor           *monitor,
		     gboolean	          status,
		     GpmBrightnessApplet *applet)
{
	if (status) {
		gpm_brightness_applet_dbus_connect (applet);
		gpm_applet_update_tooltip (applet);
		gpm_applet_draw_cb (applet);
	} else {
		gpm_brightness_applet_dbus_disconnect (applet);
		gpm_applet_update_tooltip (applet);
		gpm_applet_draw_cb (applet);
	}
}

/**
 * gpm_brightness_applet_init:
 * @applet: Brightness applet instance
 **/
static void
gpm_brightness_applet_init (GpmBrightnessApplet *applet)
{
	DBusGConnection *connection;

	/* initialize fields */
	applet->size = 0;
	applet->call_worked = TRUE;
	applet->popped = FALSE;
	applet->popup = NULL;
	applet->icon = NULL;
	applet->connection = NULL;
	applet->proxy = NULL;

	/* Add application specific icons to search path */
	gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (),
                                           GPM_DATA G_DIR_SEPARATOR_S "icons");


	applet->monitor = egg_dbus_monitor_new ();
	g_signal_connect (applet->monitor, "connection-changed",
			  G_CALLBACK (monitor_connection_cb), applet);
	connection = dbus_g_bus_get (DBUS_BUS_SESSION, NULL);
	egg_dbus_monitor_assign (applet->monitor, connection, GPM_DBUS_SERVICE);
	gpm_brightness_applet_dbus_connect (applet);

	/* coldplug */
	applet->call_worked = gpm_applet_get_brightness (applet);
	gpm_applet_update_popup_level (applet);

	/* prepare */
	mate_panel_applet_set_flags (MATE_PANEL_APPLET (applet), MATE_PANEL_APPLET_EXPAND_MINOR);

	/* show */
	gtk_widget_show_all (GTK_WIDGET(applet));

	/* set appropriate size and load icon accordingly */
	gpm_applet_draw_cb (applet);

	/* connect */
	g_signal_connect (G_OBJECT(applet), "button-press-event",
			  G_CALLBACK(gpm_applet_popup_cb), NULL);

	g_signal_connect (G_OBJECT(applet), "scroll-event",
			  G_CALLBACK(gpm_applet_scroll_cb), NULL);

	g_signal_connect (G_OBJECT(applet), "key-press-event",
			  G_CALLBACK(gpm_applet_key_press_cb), NULL);

	/* We use g_signal_connect_after because letting the panel draw
	 * the background is the only way to have the correct
	 * background when a theme defines a background picture. */
	g_signal_connect_after (G_OBJECT(applet), "expose-event",
				G_CALLBACK(gpm_applet_draw_cb), NULL);

	g_signal_connect (G_OBJECT(applet), "change-background",
			  G_CALLBACK(gpm_applet_change_background_cb), NULL);

	g_signal_connect (G_OBJECT(applet), "change-orient",
			  G_CALLBACK(gpm_applet_draw_cb), NULL);

	g_signal_connect (G_OBJECT(applet), "change-orient",
			  G_CALLBACK(gpm_applet_destroy_popup_cb), NULL);

	g_signal_connect (G_OBJECT(applet), "destroy",
			  G_CALLBACK(gpm_applet_destroy_cb), NULL);

	/* prevent scroll events from reaching the tooltip */
	g_signal_connect (G_OBJECT (applet), "event-after", G_CALLBACK (gpm_applet_stop_scroll_events_cb), NULL);

	g_signal_connect (gtk_icon_theme_get_default (), "changed", G_CALLBACK (gpm_applet_theme_change_cb), applet);
}

/**
 * gpm_applet_matecomponent_cb:
 * @_applet: GpmBrightnessApplet instance created by the matecomponent factory
 * @iid: MateComponent id
 *
 * the function called by matecomponent factory after creation
 **/
static gboolean
gpm_applet_matecomponent_cb (MatePanelApplet *_applet, const gchar *iid, gpointer data)
{
	GpmBrightnessApplet *applet = GPM_BRIGHTNESS_APPLET(_applet);

	static MateComponentUIVerb verbs [] = {
		MATECOMPONENT_UI_VERB ("About", gpm_applet_dialog_about_cb),
		MATECOMPONENT_UI_VERB ("Help", gpm_applet_help_cb),
		MATECOMPONENT_UI_VERB_END
	};

	if (strcmp (iid, GPM_BRIGHTNESS_APPLET_OAFID) != 0) {
		return FALSE;
	}

	mate_panel_applet_setup_menu_from_file (MATE_PANEL_APPLET (applet),
					   DATADIR,
					   "MATE_BrightnessApplet.xml",
					   NULL, verbs, applet);
	gpm_applet_draw_cb (applet);
	return TRUE;
}

/**
 * this generates a main with a matecomponent factory
 **/
MATE_PANEL_APPLET_MATECOMPONENT_FACTORY
 (/* the factory iid */
 GPM_BRIGHTNESS_APPLET_FACTORY_OAFID,
 /* generates brighness applets instead of regular mate applets  */
 GPM_TYPE_BRIGHTNESS_APPLET,
 /* the applet name and version */
 "BrightnessApplet", VERSION,
 /* our callback (with no user data) */
 gpm_applet_matecomponent_cb, NULL);
