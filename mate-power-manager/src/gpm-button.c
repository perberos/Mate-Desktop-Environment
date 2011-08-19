/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2006-2007 Richard Hughes <richard@hughsie.com>
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

#include "config.h"

#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>

#include <X11/X.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <X11/XF86keysym.h>
#include <libupower-glib/upower.h>

#include "gpm-common.h"
#include "gpm-button.h"

#include "egg-debug.h"

static void     gpm_button_finalize   (GObject	      *object);

#define GPM_BUTTON_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GPM_TYPE_BUTTON, GpmButtonPrivate))

struct GpmButtonPrivate
{
	GdkScreen		*screen;
	GdkWindow		*window;
	GHashTable		*keysym_to_name_hash;
	gchar			*last_button;
	GTimer			*timer;
	gboolean		 lid_is_closed;
	UpClient		*client;
};

enum {
	BUTTON_PRESSED,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };
static gpointer gpm_button_object = NULL;

G_DEFINE_TYPE (GpmButton, gpm_button, G_TYPE_OBJECT)

#define GPM_BUTTON_DUPLICATE_TIMEOUT	0.125f

/**
 * gpm_button_emit_type:
 **/
static gboolean
gpm_button_emit_type (GpmButton *button, const gchar *type)
{
	g_return_val_if_fail (GPM_IS_BUTTON (button), FALSE);

	/* did we just have this button before the timeout? */
	if (g_strcmp0 (type, button->priv->last_button) == 0 &&
	    g_timer_elapsed (button->priv->timer, NULL) < GPM_BUTTON_DUPLICATE_TIMEOUT) {
		egg_debug ("ignoring duplicate button %s", type);
		return FALSE;
	}

	egg_debug ("emitting button-pressed : %s", type);
	g_signal_emit (button, signals [BUTTON_PRESSED], 0, type);

	/* save type and last size */
	g_free (button->priv->last_button);
	button->priv->last_button = g_strdup (type);
	g_timer_reset (button->priv->timer);

	return TRUE;
}

/**
 * gpm_button_filter_x_events:
 **/
static GdkFilterReturn
gpm_button_filter_x_events (GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
	GpmButton *button = (GpmButton *) data;
	XEvent *xev = (XEvent *) xevent;
	guint keycode;
	const gchar *key;
	gchar *keycode_str;

	if (xev->type != KeyPress)
		return GDK_FILTER_CONTINUE;

	keycode = xev->xkey.keycode;

	/* is the key string already in our DB? */
	keycode_str = g_strdup_printf ("0x%x", keycode);
	key = g_hash_table_lookup (button->priv->keysym_to_name_hash, (gpointer) keycode_str);
	g_free (keycode_str);

	/* found anything? */
	if (key == NULL) {
		egg_debug ("Key %i not found in hash", keycode);
		/* pass normal keypresses on, which might help with accessibility access */
		return GDK_FILTER_CONTINUE;
	}

	egg_debug ("Key %i mapped to key %s", keycode, key);
	gpm_button_emit_type (button, key);

	return GDK_FILTER_REMOVE;
}

/**
 * gpm_button_grab_keystring:
 * @button: This button class instance
 * @keystr: The key string, e.g. "<Control><Alt>F11"
 * @hashkey: A unique key made up from the modmask and keycode suitable for
 *           referencing in a hashtable.
 *           You must free this string, or specify NULL to ignore.
 *
 * Grab the key specified in the key string.
 *
 * Return value: TRUE if we parsed and grabbed okay
 **/
static gboolean
gpm_button_grab_keystring (GpmButton *button, guint64 keycode)
{
	guint modmask = AnyModifier;
	Display *display;
	gint ret;

	/* get the current X Display */
	display = GDK_DISPLAY_XDISPLAY (gdk_display_get_default());

	/* don't abort on error */
	gdk_error_trap_push ();

	/* grab the key if possible */
	ret = XGrabKey (display, keycode, modmask,
			GDK_WINDOW_XID (button->priv->window), True,
			GrabModeAsync, GrabModeAsync);
	if (ret == BadAccess) {
		egg_warning ("Failed to grab modmask=%u, keycode=%li",
			     modmask, (long int) keycode);
		return FALSE;
	}

	/* grab the lock key if possible */
	ret = XGrabKey (display, keycode, LockMask | modmask,
			GDK_WINDOW_XID (button->priv->window), True,
			GrabModeAsync, GrabModeAsync);
	if (ret == BadAccess) {
		egg_warning ("Failed to grab modmask=%u, keycode=%li",
			     LockMask | modmask, (long int) keycode);
		return FALSE;
	}

	/* we are not processing the error */
	gdk_flush ();
	gdk_error_trap_pop ();

	egg_debug ("Grabbed modmask=%x, keycode=%li", modmask, (long int) keycode);
	return TRUE;
}

/**
 * gpm_button_grab_keystring:
 * @button: This button class instance
 * @keystr: The key string, e.g. "<Control><Alt>F11"
 * @hashkey: A unique key made up from the modmask and keycode suitable for
 *           referencing in a hashtable.
 *           You must free this string, or specify NULL to ignore.
 *
 * Grab the key specified in the key string.
 *
 * Return value: TRUE if we parsed and grabbed okay
 **/
static gboolean
gpm_button_xevent_key (GpmButton *button, guint keysym, const gchar *key_name)
{
	gchar *key = NULL;
	gboolean ret;
	gchar *keycode_str;
	guint keycode;

	/* convert from keysym to keycode */
	keycode = XKeysymToKeycode (GDK_DISPLAY_XDISPLAY (gdk_display_get_default()), keysym);
	if (keycode == 0) {
		egg_warning ("could not map keysym %x to keycode", keysym);
		return FALSE;
	}

	/* is the key string already in our DB? */
	keycode_str = g_strdup_printf ("0x%x", keycode);
	key = g_hash_table_lookup (button->priv->keysym_to_name_hash, (gpointer) keycode_str);
	if (key != NULL) {
		egg_warning ("found in hash %i", keycode);
		g_free (keycode_str);
		return FALSE;
	}

	/* try to register X event */
	ret = gpm_button_grab_keystring (button, keycode);
	if (!ret) {
		egg_warning ("Failed to grab %i", keycode);
		g_free (keycode_str);
		return FALSE;
	}

	/* add to hash table */
	g_hash_table_insert (button->priv->keysym_to_name_hash, (gpointer) keycode_str, (gpointer) g_strdup (key_name));

	/* the key is freed in the hash function unref */
	return TRUE;
}

/**
 * gpm_button_class_init:
 * @button: This class instance
 **/
static void
gpm_button_class_init (GpmButtonClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = gpm_button_finalize;
	g_type_class_add_private (klass, sizeof (GpmButtonPrivate));

	signals [BUTTON_PRESSED] =
		g_signal_new ("button-pressed",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GpmButtonClass, button_pressed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE, 1, G_TYPE_STRING);
}

/**
 * gpm_button_is_lid_closed:
 **/
gboolean
gpm_button_is_lid_closed (GpmButton *button)
{
	g_return_val_if_fail (GPM_IS_BUTTON (button), FALSE);
	return 	up_client_get_lid_is_closed (button->priv->client);
}

/**
 * gpm_button_reset_time:
 *
 * We have to refresh the event time on resume to handle duplicate buttons
 * properly when the time is significant when we suspend.
 **/
gboolean
gpm_button_reset_time (GpmButton *button)
{
	g_return_val_if_fail (GPM_IS_BUTTON (button), FALSE);
	g_timer_reset (button->priv->timer);
	return TRUE;
}

/**
 * gpm_button_client_changed_cb
 **/
static void
gpm_button_client_changed_cb (UpClient *client, GpmButton *button)
{
	gboolean lid_is_closed;

	/* get new state */
	lid_is_closed = up_client_get_lid_is_closed (button->priv->client);

	/* same state */
	if (button->priv->lid_is_closed == lid_is_closed)
		return;

	/* save state */
	button->priv->lid_is_closed = lid_is_closed;

	/* sent correct event */
	if (lid_is_closed)
		gpm_button_emit_type (button, GPM_BUTTON_LID_CLOSED);
	else
		gpm_button_emit_type (button, GPM_BUTTON_LID_OPEN);
}

/**
 * gpm_button_init:
 * @button: This class instance
 **/
static void
gpm_button_init (GpmButton *button)
{
	button->priv = GPM_BUTTON_GET_PRIVATE (button);

	button->priv->screen = gdk_screen_get_default ();
	button->priv->window = gdk_screen_get_root_window (button->priv->screen);

	button->priv->keysym_to_name_hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
	button->priv->last_button = NULL;
	button->priv->timer = g_timer_new ();

	button->priv->client = up_client_new ();
	button->priv->lid_is_closed = up_client_get_lid_is_closed (button->priv->client);
	g_signal_connect (button->priv->client, "changed",
			  G_CALLBACK (gpm_button_client_changed_cb), button);

	/* register the brightness keys */
	gpm_button_xevent_key (button, XF86XK_PowerOff, GPM_BUTTON_POWER);
#ifdef HAVE_XF86XK_SUSPEND
	/* The kernel messes up suspend/hibernate in some places. One of
	 * them is the key names. Unfortunately, they refuse to see the
	 * errors of their way in the name of 'compatibility'. Meh
	 */
	gpm_button_xevent_key (button, XF86XK_Suspend, GPM_BUTTON_HIBERNATE);
#endif
	gpm_button_xevent_key (button, XF86XK_Sleep, GPM_BUTTON_SUSPEND); /* should be configurable */
#ifdef HAVE_XF86XK_HIBERNATE
	gpm_button_xevent_key (button, XF86XK_Hibernate, GPM_BUTTON_HIBERNATE);
#endif
	gpm_button_xevent_key (button, XF86XK_MonBrightnessUp, GPM_BUTTON_BRIGHT_UP);
	gpm_button_xevent_key (button, XF86XK_MonBrightnessDown, GPM_BUTTON_BRIGHT_DOWN);
	gpm_button_xevent_key (button, XF86XK_ScreenSaver, GPM_BUTTON_LOCK);
#ifdef HAVE_XF86XK_BATTERY
	gpm_button_xevent_key (button, XF86XK_Battery, GPM_BUTTON_BATTERY);
#endif
	gpm_button_xevent_key (button, XF86XK_KbdBrightnessUp, GPM_BUTTON_KBD_BRIGHT_UP);
	gpm_button_xevent_key (button, XF86XK_KbdBrightnessDown, GPM_BUTTON_KBD_BRIGHT_DOWN);
	gpm_button_xevent_key (button, XF86XK_KbdLightOnOff, GPM_BUTTON_KBD_BRIGHT_TOGGLE);

	/* use global filter */
	gdk_window_add_filter (button->priv->window,
			       gpm_button_filter_x_events, (gpointer) button);
}

/**
 * gpm_button_finalize:
 * @object: This class instance
 **/
static void
gpm_button_finalize (GObject *object)
{
	GpmButton *button;
	g_return_if_fail (object != NULL);
	g_return_if_fail (GPM_IS_BUTTON (object));

	button = GPM_BUTTON (object);
	button->priv = GPM_BUTTON_GET_PRIVATE (button);

	g_object_unref (button->priv->client);
	g_free (button->priv->last_button);
	g_timer_destroy (button->priv->timer);

	g_hash_table_unref (button->priv->keysym_to_name_hash);

	G_OBJECT_CLASS (gpm_button_parent_class)->finalize (object);
}

/**
 * gpm_button_new:
 * Return value: new class instance.
 **/
GpmButton *
gpm_button_new (void)
{
	if (gpm_button_object != NULL) {
		g_object_ref (gpm_button_object);
	} else {
		gpm_button_object = g_object_new (GPM_TYPE_BUTTON, NULL);
		g_object_add_weak_pointer (gpm_button_object, &gpm_button_object);
	}
	return GPM_BUTTON (gpm_button_object);
}
