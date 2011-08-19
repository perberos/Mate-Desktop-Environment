/* bacon-resize.c
 * Copyright (C) 2003-2004, Bastien Nocera <hadess@hadess.net>
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02139, USA.
 */

#include "config.h"
#include "bacon-resize.h"
#include <glib.h>

#ifdef HAVE_XVIDMODE
#include <gdk/gdkx.h>
#include <gdk/gdk.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>

#include <X11/extensions/xf86vmode.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/Xrender.h>
#endif

static void bacon_resize_set_property	(GObject *object,
					 guint property_id,
					 const GValue *value,
					 GParamSpec *pspec);
static void bacon_resize_get_property	(GObject *object,
					 guint property_id,
					 GValue *value,
					 GParamSpec *pspec);
#ifdef HAVE_XVIDMODE
static void bacon_resize_finalize	(GObject *object);
#endif /* HAVE_XVIDMODE */

static void set_video_widget		(BaconResize *resize,
					 GtkWidget *video_widget);
#ifdef HAVE_XVIDMODE
static void screen_changed_cb		(GtkWidget *video_widget,
					 GdkScreen *previous_screen,
					 BaconResize *resize);
#endif /* HAVE_XVIDMODE */

struct BaconResizePrivate {
	gboolean have_xvidmode;
	gboolean resized;
	GtkWidget *video_widget;
#ifdef HAVE_XVIDMODE
	/* XRandR */
	XRRScreenConfiguration *xr_screen_conf;
	XRRScreenSize *xr_sizes;
	Rotation xr_current_rotation;
	SizeID xr_original_size;
#endif
};

enum {
	PROP_HAVE_XVIDMODE = 1,
	PROP_VIDEO_WIDGET
};

G_DEFINE_TYPE (BaconResize, bacon_resize, G_TYPE_OBJECT)

static void
bacon_resize_class_init (BaconResizeClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (BaconResizePrivate));

	object_class->set_property = bacon_resize_set_property;
	object_class->get_property = bacon_resize_get_property;
#ifdef HAVE_XVIDMODE
	object_class->finalize = bacon_resize_finalize;
#endif /* HAVE_XVIDMODE */

	g_object_class_install_property (object_class, PROP_HAVE_XVIDMODE,
					 g_param_spec_boolean ("have-xvidmode", NULL, NULL,
							       FALSE, G_PARAM_READABLE |
                                                               G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class, PROP_VIDEO_WIDGET,
					 g_param_spec_object ("video-widget", "video-widget",
						 	      "The related video widget",
							      GTK_TYPE_WIDGET,
							      G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY |
                                                              G_PARAM_STATIC_STRINGS));
}

static void
bacon_resize_init (BaconResize *resize)
{
	resize->priv = G_TYPE_INSTANCE_GET_PRIVATE (resize, BACON_TYPE_RESIZE, BaconResizePrivate);

	resize->priv->have_xvidmode = FALSE;
	resize->priv->resized = FALSE;
}

BaconResize *
bacon_resize_new (GtkWidget *video_widget)
{
	return BACON_RESIZE (g_object_new (BACON_TYPE_RESIZE, "video-widget", video_widget, NULL));
}

#ifdef HAVE_XVIDMODE
static void
bacon_resize_finalize (GObject *object)
{
	BaconResize *self = BACON_RESIZE (object);

	g_signal_handlers_disconnect_by_func (self->priv->video_widget, screen_changed_cb, self);

	G_OBJECT_CLASS (bacon_resize_parent_class)->finalize (object);
}
#endif /* HAVE_XVIDMODE */

static void
bacon_resize_set_property (GObject *object,
			   guint property_id,
			   const GValue *value,
			   GParamSpec *pspec)
{
	switch (property_id)
	{
	case PROP_VIDEO_WIDGET:
		set_video_widget (BACON_RESIZE (object), GTK_WIDGET (g_value_get_object (value)));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
bacon_resize_get_property (GObject *object,
			   guint property_id,
			   GValue *value,
			   GParamSpec *pspec)
{
	switch (property_id)
	{
	case PROP_HAVE_XVIDMODE:
		g_value_set_boolean (value, BACON_RESIZE (object)->priv->have_xvidmode);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
set_video_widget (BaconResize *resize, GtkWidget *video_widget)
{
#ifdef HAVE_XVIDMODE
	GdkDisplay *display;
	GdkScreen *screen;
	int event_basep, error_basep;
	XRRScreenConfiguration *xr_screen_conf;
#endif
	g_return_if_fail (gtk_widget_get_realized (video_widget));

	resize->priv->video_widget = video_widget;

#ifdef HAVE_XVIDMODE
	display = gtk_widget_get_display (video_widget);
	screen = gtk_widget_get_screen (video_widget);

	g_signal_connect (G_OBJECT (video_widget),
			  "screen-changed", G_CALLBACK (screen_changed_cb), resize);

	XLockDisplay (GDK_DISPLAY_XDISPLAY (display));

	if (!XF86VidModeQueryExtension (GDK_DISPLAY_XDISPLAY (display), &event_basep, &error_basep))
		goto bail;

	if (!XRRQueryExtension (GDK_DISPLAY_XDISPLAY (display), &event_basep, &error_basep))
		goto bail;

	/* We don't use the output here, checking whether XRRGetScreenInfo works */
	xr_screen_conf = XRRGetScreenInfo (GDK_DISPLAY_XDISPLAY (display), GDK_WINDOW_XWINDOW (gdk_screen_get_root_window (screen)));
	if (xr_screen_conf == NULL)
		goto bail;
 
	XRRFreeScreenConfigInfo (xr_screen_conf);
	XUnlockDisplay (GDK_DISPLAY_XDISPLAY (display));
	resize->priv->have_xvidmode = TRUE;
	return;

bail:
	XUnlockDisplay (GDK_DISPLAY_XDISPLAY (display));
	resize->priv->have_xvidmode = FALSE;
#endif /* HAVE_XVIDMODE */
}

#ifdef HAVE_XVIDMODE
static void screen_changed_cb (GtkWidget *video_widget, GdkScreen *previous_screen, BaconResize *resize)
{
	if (resize->priv->resized == TRUE)
		bacon_resize_resize (resize);
	else
		bacon_resize_restore (resize);
}
#endif /* HAVE_XVIDMODE */

void
bacon_resize_resize (BaconResize *resize)
{
#ifdef HAVE_XVIDMODE
	int width, height, i, xr_nsize, res, dotclock;
	XF86VidModeModeLine modeline;
	XRRScreenSize *xr_sizes;
	gboolean found = FALSE;
	GdkWindow *root;
	GdkScreen *screen;
	Display *xdisplay;

	g_return_if_fail (GTK_IS_WIDGET (resize->priv->video_widget));
	g_return_if_fail (gtk_widget_get_realized (resize->priv->video_widget));

	xdisplay = GDK_DRAWABLE_XDISPLAY (gtk_widget_get_window (GTK_WIDGET (resize->priv->video_widget)));
	if (xdisplay == NULL)
		return;

	XLockDisplay (xdisplay);

	screen = gtk_widget_get_screen (resize->priv->video_widget);
	root = gdk_screen_get_root_window (screen);

	/* XF86VidModeGetModeLine just doesn't work nicely with multiple monitors */
	if (gdk_screen_get_n_monitors (screen) > 1)
		goto bail;

	res = XF86VidModeGetModeLine (xdisplay, GDK_SCREEN_XNUMBER (screen), &dotclock, &modeline);
	if (!res)
		goto bail;

	/* Check if there's a viewport */
	width = gdk_screen_get_width (screen);
	height = gdk_screen_get_height (screen);

	if (width <= modeline.hdisplay && height <= modeline.vdisplay)
		goto bail;

	gdk_error_trap_push ();

	/* Find the XRandR mode that corresponds to the real size */
	resize->priv->xr_screen_conf = XRRGetScreenInfo (xdisplay, GDK_WINDOW_XWINDOW (root));
	xr_sizes = XRRConfigSizes (resize->priv->xr_screen_conf, &xr_nsize);
	resize->priv->xr_original_size = XRRConfigCurrentConfiguration (resize->priv->xr_screen_conf, &(resize->priv->xr_current_rotation));
	if (gdk_error_trap_pop ()) {
		g_warning ("XRRConfigSizes or XRRConfigCurrentConfiguration failed");
		goto bail;
	}

	for (i = 0; i < xr_nsize; i++) {
		if (modeline.hdisplay == xr_sizes[i].width && modeline.vdisplay == xr_sizes[i].height) {
			found = TRUE;
			break;
		}
	}

	if (!found)
		goto bail;

	gdk_error_trap_push ();
	XRRSetScreenConfig (xdisplay,
			resize->priv->xr_screen_conf,
			GDK_WINDOW_XWINDOW (root),
			(SizeID) i,
			resize->priv->xr_current_rotation,
			CurrentTime);
	gdk_flush ();
	if (gdk_error_trap_pop ())
		g_warning ("XRRSetScreenConfig failed");
	else
		resize->priv->resized = TRUE;

bail:
	XUnlockDisplay (xdisplay);
#endif /* HAVE_XVIDMODE */
}

void
bacon_resize_restore (BaconResize *resize)
{
#ifdef HAVE_XVIDMODE
	int width, height, res, dotclock;
	XF86VidModeModeLine modeline;
	GdkWindow *root;
	GdkScreen *screen;
	Display *xdisplay;

	g_return_if_fail (GTK_IS_WIDGET (resize->priv->video_widget));
	g_return_if_fail (gtk_widget_get_realized (resize->priv->video_widget));

	/* We haven't called bacon_resize_resize before, or it exited
	 * as we didn't need a resize. */
	if (resize->priv->xr_screen_conf == NULL)
		return;

	xdisplay = GDK_DRAWABLE_XDISPLAY (gtk_widget_get_window (GTK_WIDGET (resize->priv->video_widget)));
	if (xdisplay == NULL)
		return;

	XLockDisplay (xdisplay);

	screen = gtk_widget_get_screen (resize->priv->video_widget);
	root = gdk_screen_get_root_window (screen);
	res = XF86VidModeGetModeLine (xdisplay, GDK_SCREEN_XNUMBER (screen), &dotclock, &modeline);
	if (!res)
		goto bail;

	/* Check if there's a viewport */
	width = gdk_screen_get_width (screen);
	height = gdk_screen_get_height (screen);

	if (width > modeline.hdisplay && height > modeline.vdisplay)
		goto bail;

	gdk_error_trap_push ();
	XRRSetScreenConfig (xdisplay,
			resize->priv->xr_screen_conf,
			GDK_WINDOW_XWINDOW (root),
			resize->priv->xr_original_size,
			resize->priv->xr_current_rotation,
			CurrentTime);
	gdk_flush ();
	if (gdk_error_trap_pop ())
		g_warning ("XRRSetScreenConfig failed");
	else
		resize->priv->resized = FALSE;

	XRRFreeScreenConfigInfo (resize->priv->xr_screen_conf);
	resize->priv->xr_screen_conf = NULL;

bail:
	XUnlockDisplay (xdisplay);
#endif
}

