/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * MATE Power Manager Inhibit Applet
 * Copyright (C) 2006 Benjamin Canou <bookeldor@gmail.com>
 * Copyright (C) 2006-2009 Richard Hughes <richard@hughsie.com>
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
#include <glib-object.h>
#include <dbus/dbus-glib.h>

#include "egg-debug.h"
#include "egg-dbus-monitor.h"
#include "gpm-common.h"

#define GPM_TYPE_INHIBIT_APPLET		(gpm_inhibit_applet_get_type ())
#define GPM_INHIBIT_APPLET(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GPM_TYPE_INHIBIT_APPLET, GpmInhibitApplet))
#define GPM_INHIBIT_APPLET_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), GPM_TYPE_INHIBIT_APPLET, GpmInhibitAppletClass))
#define GPM_IS_INHIBIT_APPLET(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GPM_TYPE_INHIBIT_APPLET))
#define GPM_IS_INHIBIT_APPLET_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GPM_TYPE_INHIBIT_APPLET))
#define GPM_INHIBIT_APPLET_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GPM_TYPE_INHIBIT_APPLET, GpmInhibitAppletClass))

typedef struct{
	MatePanelApplet parent;
	/* applet state */
	guint cookie;
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
} GpmInhibitApplet;

typedef struct{
	MatePanelAppletClass	parent_class;
} GpmInhibitAppletClass;

GType                gpm_inhibit_applet_get_type  (void);

#define GS_DBUS_SERVICE		"org.mate.SessionManager"
#define GS_DBUS_PATH		"/org/mate/SessionManager"
#define GS_DBUS_INTERFACE	"org.mate.SessionManager"

static void      gpm_inhibit_applet_class_init (GpmInhibitAppletClass *klass);
static void      gpm_inhibit_applet_init       (GpmInhibitApplet *applet);

G_DEFINE_TYPE (GpmInhibitApplet, gpm_inhibit_applet, PANEL_TYPE_APPLET)

static void	gpm_applet_get_icon		(GpmInhibitApplet *applet);
static void	gpm_applet_check_size		(GpmInhibitApplet *applet);
static gboolean	gpm_applet_draw_cb		(GpmInhibitApplet *applet);
static void	gpm_applet_update_tooltip	(GpmInhibitApplet *applet);
static gboolean	gpm_applet_click_cb		(GpmInhibitApplet *applet, GdkEventButton *event);
static void	gpm_applet_dialog_about_cb	(MateComponentUIComponent *uic, gpointer data, const gchar *verbname);
static gboolean	gpm_applet_matecomponent_cb		(MatePanelApplet *_applet, const gchar *iid, gpointer data);
static void	gpm_applet_destroy_cb		(GtkObject *object);

#define GPM_INHIBIT_APPLET_OAFID		"OAFIID:MATE_InhibitApplet"
#define GPM_INHIBIT_APPLET_FACTORY_OAFID	"OAFIID:MATE_InhibitApplet_Factory"
#define GPM_INHIBIT_APPLET_ICON_INHIBIT		"gpm-inhibit"
#define GPM_INHIBIT_APPLET_ICON_INVALID		"gpm-inhibit-invalid"
#define GPM_INHIBIT_APPLET_ICON_UNINHIBIT	"gpm-hibernate"
#define GPM_INHIBIT_APPLET_NAME			_("Power Manager Inhibit Applet")
#define GPM_INHIBIT_APPLET_DESC			_("Allows user to inhibit automatic power saving.")
#define MATE_PANEL_APPLET_VERTICAL(p)					\
	 (((p) == MATE_PANEL_APPLET_ORIENT_LEFT) || ((p) == MATE_PANEL_APPLET_ORIENT_RIGHT))


/** cookie is returned as an unsigned integer */
static gboolean
gpm_applet_inhibit (GpmInhibitApplet *applet,
		    const gchar     *appname,
		    const gchar     *reason,
		    guint           *cookie)
{
	GError  *error = NULL;
	gboolean ret;

	g_return_val_if_fail (cookie != NULL, FALSE);

	if (applet->proxy == NULL) {
		egg_warning ("not connected\n");
		return FALSE;
	}

	ret = dbus_g_proxy_call (applet->proxy, "Inhibit", &error,
				 G_TYPE_STRING, appname,
				 G_TYPE_UINT, 0, /* xid */
				 G_TYPE_STRING, reason,
				 G_TYPE_UINT, 1+2+4+8, /* logoff, switch, suspend, and idle */
				 G_TYPE_INVALID,
				 G_TYPE_UINT, cookie,
				 G_TYPE_INVALID);
	if (error) {
		g_debug ("ERROR: %s", error->message);
		g_error_free (error);
		*cookie = 0;
	}
	if (!ret) {
		/* abort as the DBUS method failed */
		g_warning ("Inhibit failed!");
	}

	return ret;
}

static gboolean
gpm_applet_uninhibit (GpmInhibitApplet *applet,
		      guint            cookie)
{
	GError *error = NULL;
	gboolean ret;

	if (applet->proxy == NULL) {
		egg_warning ("not connected");
		return FALSE;
	}

	ret = dbus_g_proxy_call (applet->proxy, "Uninhibit", &error,
				 G_TYPE_UINT, cookie,
				 G_TYPE_INVALID,
				 G_TYPE_INVALID);
	if (error) {
		g_debug ("ERROR: %s", error->message);
		g_error_free (error);
	}
	if (!ret) {
		/* abort as the DBUS method failed */
		g_warning ("Uninhibit failed!");
	}

	return ret;
}
#if 0
static gboolean
gpm_applet_has_inhibit (GpmInhibitApplet *applet,
			gboolean        *has_inhibit)
{
	GError  *error = NULL;
	gboolean ret;
	DBusGProxy *proxy;

	proxy = egg_dbus_proxy_get_proxy (applet->gproxy);
	if (proxy == NULL) {
		g_warning ("not connected");
		return FALSE;
	}

	ret = dbus_g_proxy_call (proxy, "HasInhibit", &error,
				 G_TYPE_INVALID,
				 G_TYPE_BOOLEAN, has_inhibit,
				 G_TYPE_INVALID);
	if (error) {
		g_debug ("ERROR: %s", error->message);
		g_error_free (error);
	}
	if (!ret) {
		/* abort as the DBUS method failed */
		g_warning ("HasInhibit failed!");
	}

	return ret;
}
#endif

/**
 * gpm_applet_get_icon:
 * @applet: Inhibit applet instance
 *
 * retrieve an icon from stock with a size adapted to panel
 **/
static void
gpm_applet_get_icon (GpmInhibitApplet *applet)
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
		icon = GPM_INHIBIT_APPLET_ICON_INVALID;
	} else if (applet->cookie > 0) {
		icon = GPM_INHIBIT_APPLET_ICON_INHIBIT;
	} else {
		icon = GPM_INHIBIT_APPLET_ICON_UNINHIBIT;
	}
	applet->icon = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
						 icon,
						 applet->size - 2,
						 0,
						 NULL);

	/* update size cache */
	applet->icon_height = gdk_pixbuf_get_height (applet->icon);
	applet->icon_width = gdk_pixbuf_get_width (applet->icon);
}

/**
 * gpm_applet_check_size:
 * @applet: Inhibit applet instance
 *
 * check if panel size has changed and applet adapt size
 **/
static void
gpm_applet_check_size (GpmInhibitApplet *applet)
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
		/* Adjusting incase the icon size has changed */
		if (allocation.height < applet->icon_height + 2) {
			gtk_widget_set_size_request (GTK_WIDGET(applet), applet->size, applet->icon_height + 2);
		}
	} else {
		if (applet->size != allocation.height) {
			applet->size = allocation.height;
			gpm_applet_get_icon (applet);
			gtk_widget_set_size_request (GTK_WIDGET(applet), applet->icon_width + 2, applet->size);
		}
		/* Adjusting incase the icon size has changed */
		if (allocation.width < applet->icon_width + 2) {
			gtk_widget_set_size_request (GTK_WIDGET(applet), applet->icon_width + 2, applet->size);
		}
	}
}

/**
 * gpm_applet_draw_cb:
 * @applet: Inhibit applet instance
 *
 * draws applet content (background + icon)
 **/
static gboolean
gpm_applet_draw_cb (GpmInhibitApplet *applet)
{
	gint w, h, bg_type;
	GdkColor color;
	GdkGC *gc;
	GdkPixmap *background;
	GtkAllocation allocation;

	if (gtk_widget_get_window (GTK_WIDGET(applet)) == NULL) {
		return FALSE;
	}

	/* Clear the window so we can draw on it later */
	gdk_window_clear(gtk_widget_get_window (GTK_WIDGET (applet)));

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
	if (bg_type == PANEL_PIXMAP_BACKGROUND) {
		/* fill with given background pixmap */
		gdk_draw_drawable (gtk_widget_get_window (GTK_WIDGET(applet)), gc, background, 0, 0, 0, 0, w, h);
	}
	
	/* draw color background */
	if (bg_type == PANEL_COLOR_BACKGROUND) {
		gdk_gc_set_rgb_fg_color (gc,&color);
		gdk_gc_set_fill (gc,GDK_SOLID);
		gdk_draw_rectangle (gtk_widget_get_window (GTK_WIDGET(applet)), gc, TRUE, 0, 0, w, h);
	}

	/* draw icon at center */
	gdk_draw_pixbuf (gtk_widget_get_window (GTK_WIDGET(applet)), gc, applet->icon,
			 0, 0, (w - applet->icon_width)/2, (h - applet->icon_height)/2,
			 applet->icon_width, applet->icon_height,
			 GDK_RGB_DITHER_NONE, 0, 0);

	return TRUE;
}

/**
 * gpm_applet_change_background_cb:
 *
 * Enqueues an expose event (don't know why it's not the default behaviour)
 **/
static void
gpm_applet_change_background_cb (GpmInhibitApplet *applet,
				 MatePanelAppletBackgroundType arg1,
				 GdkColor *arg2, GdkPixmap *arg3, gpointer data)
{
	gtk_widget_queue_draw (GTK_WIDGET (applet));
}

/**
 * gpm_applet_update_tooltip:
 * @applet: Inhibit applet instance
 *
 * sets tooltip's content (percentage or disabled)
 **/
static void
gpm_applet_update_tooltip (GpmInhibitApplet *applet)
{
	const gchar *buf;
	if (applet->proxy == NULL) {
		buf = _("Cannot connect to mate-power-manager");
	} else {
		if (applet->cookie > 0) {
			buf = _("Automatic sleep inhibited");
		} else {
			buf = _("Automatic sleep enabled");
		}
	}
	gtk_widget_set_tooltip_text (GTK_WIDGET(applet), buf);
}

/**
 * gpm_applet_click_cb:
 * @applet: Inhibit applet instance
 *
 * pops and unpops
 **/
static gboolean
gpm_applet_click_cb (GpmInhibitApplet *applet, GdkEventButton *event)
{
	/* react only to left mouse button */
	if (event->button != 1) {
		return FALSE;
	}

	if (applet->cookie > 0) {
		g_debug ("uninhibiting %u", applet->cookie);
		gpm_applet_uninhibit (applet, applet->cookie);
		applet->cookie = 0;
	} else {
		g_debug ("inhibiting");
		gpm_applet_inhibit (applet,
					  GPM_INHIBIT_APPLET_NAME,
					  _("Manual inhibit"),
					  &(applet->cookie));
	}
	/* update icon */
	gpm_applet_get_icon (applet);
	gpm_applet_check_size (applet);
	gpm_applet_update_tooltip (applet);
	gpm_applet_draw_cb (applet);

	return TRUE;
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
					  GPM_INHIBIT_APPLET_ICON_INHIBIT,
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
	gtk_about_dialog_set_program_name (about, GPM_INHIBIT_APPLET_NAME);
	gtk_about_dialog_set_version (about, VERSION);
	gtk_about_dialog_set_copyright (about, _("Copyright \xc2\xa9 2006-2007 Richard Hughes"));
	gtk_about_dialog_set_comments (about, GPM_INHIBIT_APPLET_DESC);
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
	gpm_help_display ("applets-inhibit");
}

/**
 * gpm_applet_destroy_cb:
 * @object: Class instance to destroy
 **/
static void
gpm_applet_destroy_cb (GtkObject *object)
{
	GpmInhibitApplet *applet = GPM_INHIBIT_APPLET(object);

	if (applet->monitor != NULL) {
		g_object_unref (applet->monitor);
	}
	if (applet->icon != NULL) {
		g_object_unref (applet->icon);
	}
}

/**
 * gpm_inhibit_applet_class_init:
 * @klass: Class instance
 **/
static void
gpm_inhibit_applet_class_init (GpmInhibitAppletClass *class)
{
	/* nothing to do here */
}


/**
 * gpm_inhibit_applet_dbus_connect:
 **/
gboolean
gpm_inhibit_applet_dbus_connect (GpmInhibitApplet *applet)
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
							 GS_DBUS_SERVICE,
							 GS_DBUS_PATH,
							 GS_DBUS_INTERFACE,
							 &error);
		if (error != NULL) {
			egg_warning ("Cannot connect, maybe the daemon is not running: %s\n", error->message);
			g_error_free (error);
			applet->proxy = NULL;
			return FALSE;
		}
	}
	return TRUE;
}

/**
 * gpm_inhibit_applet_dbus_disconnect:
 **/
gboolean
gpm_inhibit_applet_dbus_disconnect (GpmInhibitApplet *applet)
{
	if (applet->proxy != NULL) {
		egg_debug ("removing proxy\n");
		g_object_unref (applet->proxy);
		applet->proxy = NULL;
		/* we have no inhibit, these are not persistant across reboots */
		applet->cookie = 0;
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
		     GpmInhibitApplet *applet)
{
	if (status) {
		gpm_inhibit_applet_dbus_connect (applet);
		gpm_applet_update_tooltip (applet);
		gpm_applet_get_icon (applet);
		gpm_applet_draw_cb (applet);
	} else {
		gpm_inhibit_applet_dbus_disconnect (applet);
		gpm_applet_update_tooltip (applet);
		gpm_applet_get_icon (applet);
		gpm_applet_draw_cb (applet);
	}
}

/**
 * gpm_inhibit_applet_init:
 * @applet: Inhibit applet instance
 **/
static void
gpm_inhibit_applet_init (GpmInhibitApplet *applet)
{
	DBusGConnection *connection;

	/* initialize fields */
	applet->size = 0;
	applet->icon = NULL;
	applet->cookie = 0;
	applet->connection = NULL;
	applet->proxy = NULL;

	/* Add application specific icons to search path */
	gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (),
                                           GPM_DATA G_DIR_SEPARATOR_S "icons");

	applet->monitor = egg_dbus_monitor_new ();
	g_signal_connect (applet->monitor, "connection-changed",
			  G_CALLBACK (monitor_connection_cb), applet);
	connection = dbus_g_bus_get (DBUS_BUS_SESSION, NULL);
	egg_dbus_monitor_assign (applet->monitor, connection, GS_DBUS_SERVICE);
	gpm_inhibit_applet_dbus_connect (applet);

	/* prepare */
	mate_panel_applet_set_flags (MATE_PANEL_APPLET (applet), MATE_PANEL_APPLET_EXPAND_MINOR);

	/* show */
	gtk_widget_show_all (GTK_WIDGET(applet));

	/* set appropriate size and load icon accordingly */
	gpm_applet_draw_cb (applet);

	/* connect */
	g_signal_connect (G_OBJECT(applet), "button-press-event",
			  G_CALLBACK(gpm_applet_click_cb), NULL);

	g_signal_connect (G_OBJECT(applet), "expose-event",
			  G_CALLBACK(gpm_applet_draw_cb), NULL);

	/* We use g_signal_connect_after because letting the panel draw
	 * the background is the only way to have the correct
	 * background when a theme defines a background picture. */
	g_signal_connect_after (G_OBJECT(applet), "expose-event",
				G_CALLBACK(gpm_applet_draw_cb), NULL);

	g_signal_connect (G_OBJECT(applet), "change-background",
			  G_CALLBACK(gpm_applet_change_background_cb), NULL);

	g_signal_connect (G_OBJECT(applet), "change-orient",
			  G_CALLBACK(gpm_applet_draw_cb), NULL);

	g_signal_connect (G_OBJECT(applet), "destroy",
			  G_CALLBACK(gpm_applet_destroy_cb), NULL);
}

/**
 * gpm_applet_matecomponent_cb:
 * @_applet: GpmInhibitApplet instance created by the matecomponent factory
 * @iid: MateComponent id
 *
 * the function called by matecomponent factory after creation
 **/
static gboolean
gpm_applet_matecomponent_cb (MatePanelApplet *_applet, const gchar *iid, gpointer data)
{
	GpmInhibitApplet *applet = GPM_INHIBIT_APPLET(_applet);

	static MateComponentUIVerb verbs [] = {
		MATECOMPONENT_UI_VERB ("About", gpm_applet_dialog_about_cb),
		MATECOMPONENT_UI_VERB ("Help", gpm_applet_help_cb),
		MATECOMPONENT_UI_VERB_END
	};

	if (strcmp (iid, GPM_INHIBIT_APPLET_OAFID) != 0) {
		return FALSE;
	}

	mate_panel_applet_setup_menu_from_file (MATE_PANEL_APPLET (applet),
					   DATADIR,
					   "MATE_InhibitApplet.xml",
					   NULL, verbs, applet);
	gpm_applet_draw_cb (applet);
	return TRUE;
}

/**
 * this generates a main with a matecomponent factory
 **/
MATE_PANEL_APPLET_MATECOMPONENT_FACTORY
 (/* the factory iid */
 GPM_INHIBIT_APPLET_FACTORY_OAFID,
 /* generates brighness applets instead of regular mate applets  */
 GPM_TYPE_INHIBIT_APPLET,
 /* the applet name and version */
 "InhibitApplet", VERSION,
 /* our callback (with no user data) */
 gpm_applet_matecomponent_cb, NULL);
