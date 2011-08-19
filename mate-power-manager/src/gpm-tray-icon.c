/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2005 William Jon McCann <mccann@jhu.edu>
 * Copyright (C) 2005-2009 Richard Hughes <richard@hughsie.com>
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

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>

#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <mateconf/mateconf-client.h>
#include <libupower-glib/upower.h>

#include "egg-debug.h"

#include "gpm-upower.h"
#include "gpm-engine.h"
#include "gpm-common.h"
#include "gpm-stock-icons.h"
#include "gpm-tray-icon.h"

static void     gpm_tray_icon_finalize   (GObject	   *object);

#define GPM_TRAY_ICON_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GPM_TYPE_TRAY_ICON, GpmTrayIconPrivate))

struct GpmTrayIconPrivate
{
	MateConfClient		*conf;
	GpmEngine		*engine;
	GtkStatusIcon		*status_icon;
	gboolean		 show_actions;
};

G_DEFINE_TYPE (GpmTrayIcon, gpm_tray_icon, G_TYPE_OBJECT)

/**
 * gpm_tray_icon_enable_actions:
 **/
static void
gpm_tray_icon_enable_actions (GpmTrayIcon *icon, gboolean enabled)
{
	g_return_if_fail (GPM_IS_TRAY_ICON (icon));
	icon->priv->show_actions = enabled;
}

/**
 * gpm_tray_icon_show:
 * @enabled: If we should show the tray
 **/
static void
gpm_tray_icon_show (GpmTrayIcon *icon, gboolean enabled)
{
	g_return_if_fail (GPM_IS_TRAY_ICON (icon));
	gtk_status_icon_set_visible (icon->priv->status_icon, enabled);
}

/**
 * gpm_tray_icon_set_tooltip:
 * @tooltip: The tooltip text, e.g. "Batteries charged"
 **/
gboolean
gpm_tray_icon_set_tooltip (GpmTrayIcon *icon, const gchar *tooltip)
{
	g_return_val_if_fail (icon != NULL, FALSE);
	g_return_val_if_fail (GPM_IS_TRAY_ICON (icon), FALSE);
	g_return_val_if_fail (tooltip != NULL, FALSE);

#if GTK_CHECK_VERSION(2,15,0)
	gtk_status_icon_set_tooltip_text (icon->priv->status_icon, tooltip);
#else
	gtk_status_icon_set_tooltip (icon->priv->status_icon, tooltip);
#endif
	return TRUE;
}

/**
 * gpm_tray_icon_get_status_icon:
 **/
GtkStatusIcon *
gpm_tray_icon_get_status_icon (GpmTrayIcon *icon)
{
	g_return_val_if_fail (GPM_IS_TRAY_ICON (icon), NULL);
	return g_object_ref (icon->priv->status_icon);
}

/**
 * gpm_tray_icon_set_image_from_stock:
 * @filename: The icon name, e.g. GPM_STOCK_APP_ICON, or NULL to remove.
 *
 * Loads a pixmap from disk, and sets as the tooltip icon
 **/
gboolean
gpm_tray_icon_set_icon (GpmTrayIcon *icon, const gchar *filename)
{
	g_return_val_if_fail (icon != NULL, FALSE);
	g_return_val_if_fail (GPM_IS_TRAY_ICON (icon), FALSE);

	if (filename != NULL) {
		egg_debug ("Setting icon to %s", filename);
		gtk_status_icon_set_from_icon_name (icon->priv->status_icon, filename);

		/* make sure that we are visible */
		gpm_tray_icon_show (icon, TRUE);
	} else {
		/* remove icon */
		egg_debug ("no icon will be displayed");

		/* make sure that we are hidden */
		gpm_tray_icon_show (icon, FALSE);
	}
	return TRUE;
}

/**
 * gpm_tray_icon_show_info_cb:
 **/
static void
gpm_tray_icon_show_info_cb (GtkMenuItem *item, gpointer data)
{
	gchar *path;
	const gchar *object_path;

	object_path = g_object_get_data (G_OBJECT (item), "object-path");
	path = g_strdup_printf ("%s/mate-power-statistics --device %s", BINDIR, object_path);
	if (!g_spawn_command_line_async (path, NULL))
		egg_warning ("Couldn't execute command: %s", path);
	g_free (path);
}

/**
 * gpm_tray_icon_show_preferences_cb:
 * @action: A valid GtkAction
 **/
static void
gpm_tray_icon_show_preferences_cb (GtkMenuItem *item, gpointer data)
{
	const gchar *command = "mate-power-preferences";

	if (g_spawn_command_line_async (command, NULL) == FALSE)
		egg_warning ("Couldn't execute command: %s", command);
}

/**
 * gpm_tray_icon_popup_cleared_cd:
 * @widget: The popup Gtkwidget
 *
 * We have to re-enable the tooltip when the popup is removed
 **/
static void
gpm_tray_icon_popup_cleared_cd (GtkWidget *widget, GpmTrayIcon *icon)
{
	g_return_if_fail (GPM_IS_TRAY_ICON (icon));
	egg_debug ("clear tray");
	g_object_ref_sink (widget);
	g_object_unref (widget);
}

/**
 * gpm_tray_icon_class_init:
 **/
static void
gpm_tray_icon_class_init (GpmTrayIconClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gpm_tray_icon_finalize;
	g_type_class_add_private (klass, sizeof (GpmTrayIconPrivate));
}

/**
 * gpm_tray_icon_add_device:
 **/
static guint
gpm_tray_icon_add_device (GpmTrayIcon *icon, GtkMenu *menu, const GPtrArray *array, UpDeviceKind kind)
{
	guint i;
	guint added = 0;
	gchar *icon_name;
	gchar *label;
	GtkWidget *item;
	GtkWidget *image;
	const gchar *object_path;
	const gchar *desc;
	UpDevice *device;
	UpDeviceKind kind_tmp;
	gdouble percentage;

	/* find type */
	for (i=0;i<array->len;i++) {
		device = g_ptr_array_index (array, i);

		/* get device properties */
		g_object_get (device,
			      "kind", &kind_tmp,
			      "percentage", &percentage,
			      NULL);

		if (kind != kind_tmp)
			continue;

		object_path = up_device_get_object_path (device);
		egg_debug ("adding device %s", object_path);
		added++;

		/* generate the label */
		desc = gpm_device_kind_to_localised_text (kind, 1);
		label = g_strdup_printf ("%s (%.1f%%)", desc, percentage);
		item = gtk_image_menu_item_new_with_label (label);

		/* generate the image */
		icon_name = gpm_upower_get_device_icon (device);
		image = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_MENU);
		gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
		gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (item), TRUE);

		/* callback and add the the menu */
		g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (gpm_tray_icon_show_info_cb), icon);
		g_object_set_data (G_OBJECT (item), "object-path", (gpointer) object_path);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

		g_free (icon_name);
		g_free (label);
	}
	return added;
}

/**
 * gpm_tray_icon_create_menu:
 *
 * Display the popup menu.
 **/
static void
gpm_tray_icon_create_menu (GpmTrayIcon *icon, guint32 timestamp)
{
	GtkMenu *menu = (GtkMenu*) gtk_menu_new ();
	GtkWidget *item;
	GtkWidget *image;
	guint dev_cnt = 0;
	GPtrArray *array;

	/* add all device types to the drop down menu */
	array = gpm_engine_get_devices (icon->priv->engine);
	dev_cnt += gpm_tray_icon_add_device (icon, menu, array, UP_DEVICE_KIND_BATTERY);
	dev_cnt += gpm_tray_icon_add_device (icon, menu, array, UP_DEVICE_KIND_UPS);
	dev_cnt += gpm_tray_icon_add_device (icon, menu, array, UP_DEVICE_KIND_MOUSE);
	dev_cnt += gpm_tray_icon_add_device (icon, menu, array, UP_DEVICE_KIND_KEYBOARD);
	dev_cnt += gpm_tray_icon_add_device (icon, menu, array, UP_DEVICE_KIND_PDA);
	dev_cnt += gpm_tray_icon_add_device (icon, menu, array, UP_DEVICE_KIND_PHONE);
#if UP_CHECK_VERSION(0,9,5)
	dev_cnt += gpm_tray_icon_add_device (icon, menu, array, UP_DEVICE_KIND_MEDIA_PLAYER);
	dev_cnt += gpm_tray_icon_add_device (icon, menu, array, UP_DEVICE_KIND_TABLET);
	dev_cnt += gpm_tray_icon_add_device (icon, menu, array, UP_DEVICE_KIND_COMPUTER);
#endif
	g_ptr_array_unref (array);

	/* skip for things like live-cd's and GDM */
	if (!icon->priv->show_actions)
		goto skip_prefs;

	/* only do the seporator if we have at least one device */
	if (dev_cnt != 0) {
		item = gtk_separator_menu_item_new ();
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	}

	/* preferences */
	item = gtk_image_menu_item_new_with_mnemonic (_("_Preferences"));
	image = gtk_image_new_from_icon_name (GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
	g_signal_connect (G_OBJECT (item), "activate",
			  G_CALLBACK (gpm_tray_icon_show_preferences_cb), icon);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

skip_prefs:
	/* show the menu */
	gtk_widget_show_all (GTK_WIDGET (menu));
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL,
			gtk_status_icon_position_menu, icon->priv->status_icon,
			1, timestamp);

	g_signal_connect (GTK_WIDGET (menu), "hide",
			  G_CALLBACK (gpm_tray_icon_popup_cleared_cd), icon);
}

/**
 * gpm_tray_icon_popup_menu_cb:
 *
 * Display the popup menu.
 **/
static void
gpm_tray_icon_popup_menu_cb (GtkStatusIcon *status_icon, guint button, guint32 timestamp, GpmTrayIcon *icon)
{
	egg_debug ("icon right clicked");
	gpm_tray_icon_create_menu (icon, timestamp);
}


/**
 * gpm_tray_icon_activate_cb:
 * @button: Which buttons are pressed
 *
 * Callback when the icon is clicked
 **/
static void
gpm_tray_icon_activate_cb (GtkStatusIcon *status_icon, GpmTrayIcon *icon)
{
	egg_debug ("icon left clicked");
	gpm_tray_icon_create_menu (icon, gtk_get_current_event_time());
}

/**
 * gpm_conf_mateconf_key_changed_cb:
 *
 * We might have to do things when the mateconf keys change; do them here.
 **/
static void
gpm_conf_mateconf_key_changed_cb (MateConfClient *client, guint cnxn_id, MateConfEntry *entry, GpmTrayIcon *icon)
{
	MateConfValue *value;
	gboolean allowed_in_menu;

	value = mateconf_entry_get_value (entry);
	if (value == NULL)
		return;

	if (strcmp (entry->key, GPM_CONF_UI_SHOW_ACTIONS) == 0) {
		allowed_in_menu = mateconf_value_get_bool (value);
		gpm_tray_icon_enable_actions (icon, allowed_in_menu);
	}
}

/**
 * gpm_tray_icon_init:
 *
 * Initialise the tray object
 **/
static void
gpm_tray_icon_init (GpmTrayIcon *icon)
{
	gboolean allowed_in_menu;

	icon->priv = GPM_TRAY_ICON_GET_PRIVATE (icon);

	icon->priv->engine = gpm_engine_new ();

	icon->priv->conf = mateconf_client_get_default ();
	/* watch mate-power-manager keys */
	mateconf_client_add_dir (icon->priv->conf, GPM_CONF_DIR,
			      MATECONF_CLIENT_PRELOAD_RECURSIVE, NULL);
	mateconf_client_notify_add (icon->priv->conf, GPM_CONF_DIR,
				 (MateConfClientNotifyFunc) gpm_conf_mateconf_key_changed_cb,
				 icon, NULL, NULL);

	icon->priv->status_icon = gtk_status_icon_new ();
	g_signal_connect_object (G_OBJECT (icon->priv->status_icon),
				 "popup_menu",
				 G_CALLBACK (gpm_tray_icon_popup_menu_cb),
				 icon, 0);
	g_signal_connect_object (G_OBJECT (icon->priv->status_icon),
				 "activate",
				 G_CALLBACK (gpm_tray_icon_activate_cb),
				 icon, 0);

	allowed_in_menu = mateconf_client_get_bool (icon->priv->conf, GPM_CONF_UI_SHOW_ACTIONS, NULL);
	gpm_tray_icon_enable_actions (icon, allowed_in_menu);
}

/**
 * gpm_tray_icon_finalize:
 * @object: This TrayIcon class instance
 **/
static void
gpm_tray_icon_finalize (GObject *object)
{
	GpmTrayIcon *tray_icon;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GPM_IS_TRAY_ICON (object));

	tray_icon = GPM_TRAY_ICON (object);

	g_object_unref (tray_icon->priv->status_icon);
	g_object_unref (tray_icon->priv->engine);
	g_return_if_fail (tray_icon->priv != NULL);

	G_OBJECT_CLASS (gpm_tray_icon_parent_class)->finalize (object);
}

/**
 * gpm_tray_icon_new:
 * Return value: A new TrayIcon object.
 **/
GpmTrayIcon *
gpm_tray_icon_new (void)
{
	GpmTrayIcon *tray_icon;
	tray_icon = g_object_new (GPM_TYPE_TRAY_ICON, NULL);
	return GPM_TRAY_ICON (tray_icon);
}

