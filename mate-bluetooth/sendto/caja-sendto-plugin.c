/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 * Copyright (C) 2004 Roberto Majadas
 * Copyright (C) 2005, 2009 Bastien Nocera
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more av.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301  USA.
 *
 * Author:  Bastien Nocera <hadess@hadess.net>
 */

#include "config.h"

#include <bluetooth-chooser-combo.h>
#include <bluetooth-chooser.h>
#include <glib/gi18n-lib.h>

#include "caja-sendto-plugin.h"

#define SCHEMA_NAME "org.mate.Bluetooth.nst"
#define PREF_LAST_USED "last-used"

static GtkWidget *combo;
static char *cmd = NULL;

static gboolean
init (NstPlugin *plugin)
{
	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

	/* Check whether bluetooth-sendto or mate-obex-send are available */
	cmd = g_find_program_in_path ("bluetooth-sendto");
	if (cmd == NULL)
		return FALSE;

	return TRUE;
}

static void
set_last_used_device (void)
{
	char *bdaddr;
	GSettings *settings;

	settings = g_settings_new (SCHEMA_NAME);
	bdaddr = g_settings_get_string (settings, PREF_LAST_USED);
	g_object_unref (settings);

	if (bdaddr != NULL && *bdaddr != '\0') {
		g_object_set (G_OBJECT (combo), "device", bdaddr, NULL);
	} else {
		g_object_set (G_OBJECT (combo), "device", BLUETOOTH_CHOOSER_COMBO_FIRST_DEVICE, NULL);
	}
	g_free (bdaddr);
}

static GtkWidget*
get_contacts_widget (NstPlugin *plugin)
{
	GtkWidget *chooser;
	const char *filter = "OBEXObjectPush";

	combo = bluetooth_chooser_combo_new ();
	g_object_get (G_OBJECT (combo), "chooser", &chooser, NULL);
	g_object_set (chooser,
		      "show-searching", TRUE,
		      "show-device-type", FALSE,
		      "show-device-category", FALSE,
		      "show-pairing", TRUE,
		      "show-connected", FALSE,
		      "device-service-filter", filter,
		      NULL);
	set_last_used_device ();
	bluetooth_chooser_start_discovery (BLUETOOTH_CHOOSER (chooser));
	gtk_container_set_border_width (GTK_CONTAINER (combo), 0);
	gtk_widget_show (combo);

	return combo;
}

static void
save_last_used_obex_device (const char *bdaddr)
{
	GSettings *settings;

	settings = g_settings_new (SCHEMA_NAME);
	g_settings_set (settings, PREF_LAST_USED, bdaddr);
	g_object_unref (settings);
}

static gboolean
send_files (NstPlugin *plugin,
	    GtkWidget *contact_widget,
	    GList *file_list)
{
	char *bdaddr = NULL;
	GPtrArray *argv;
	GList *list;
	gboolean ret;
	GError *err = NULL;

	g_object_get (G_OBJECT (combo), "device", &bdaddr, NULL);
	if (bdaddr == NULL)
		return FALSE;

	argv = g_ptr_array_new ();
	g_ptr_array_add (argv, cmd);
	g_ptr_array_add (argv, "--dest");
	g_ptr_array_add (argv, bdaddr);

	for (list = file_list; list != NULL; list = list->next) {
		g_ptr_array_add (argv, (gchar *) list->data);
	}
	g_ptr_array_add (argv, NULL);

#if 0
	g_print ("launching command: ");
	for (i = 0; i < argv->len - 1; i++) {
		g_print ("%s ", (gchar *) g_ptr_array_index (argv, i));
	}
	g_print ("\n");
#endif
	ret = g_spawn_async (NULL, (gchar **) argv->pdata,
			NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &err);
	g_ptr_array_free (argv, TRUE);

	if (ret == FALSE) {
		g_warning ("Couldn't send files via bluetooth: %s", err->message);
		g_error_free (err);
	} else {
		save_last_used_obex_device (bdaddr);
	}
	return ret;
}

static gboolean
validate_destination (NstPlugin *plugin,
		      GtkWidget *contact_widget,
		      char **error)
{
#if 0
	GError *e = NULL;
	char *bdaddr, *device_path;
	DBusGProxy *device;
	GHashTable *props;
	GValue *value;
	gboolean found = FALSE;
	char **array;
	gboolean first_time = TRUE;

	g_return_val_if_fail (error != NULL, FALSE);

	if (get_select_device (NULL, &bdaddr) == FALSE) {
		*error = g_strdup (_("Programming error, could not find the device in the list"));
		return FALSE;
	}

	if (dbus_g_proxy_call (object, "FindDevice", NULL,
			       G_TYPE_STRING, bdaddr, G_TYPE_INVALID,
			       DBUS_TYPE_G_OBJECT_PATH, &device_path, G_TYPE_INVALID) == FALSE) {
		g_free (bdaddr);
		return TRUE;
	}

	device = dbus_g_proxy_new_for_name (conn, "org.bluez",
					    device_path, "org.bluez.Device");

again:
	if (dbus_g_proxy_call (device, "GetProperties", NULL,
			       G_TYPE_INVALID, dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_VALUE),
			       &props, G_TYPE_INVALID) == FALSE) {
		goto bail;
	}

	value = g_hash_table_lookup (props, "UUIDs");
	array = g_value_get_boxed (value);
	if (array != NULL) {
		char *uuid;
		guint i;

		for (i = 0; array[i] != NULL; i++) {
			if (g_str_has_suffix (array[i], "-0000-1000-8000-00805f9b34fb") != FALSE) {
				if (g_str_has_prefix (array[i], "0000") != FALSE) {
					char *tmp;
					tmp = g_strndup (array[i] + 4, 4);
					uuid = g_strdup_printf ("0x%s", tmp);
					g_free (tmp);
				} else {
					char *tmp;
					tmp = g_strndup (array[i], 8);
					uuid = g_strdup_printf ("0x%s", tmp);
				}
			} else {
				uuid = g_strdup (array[i]);
			}

			if (strcmp (uuid, OBEX_FILETRANS_SVCLASS_ID_STR) == 0 ||
			    strcmp (uuid, OBEX_PUSH_SVCLASS_ID_STR) == 0      ){
				found = TRUE;
				g_free (uuid);
				break;
			}

			g_free (uuid);
		}
	} else {
		/* No array, can't really check now, can we */
		found = TRUE;
	}

	g_hash_table_destroy (props);
	if (found == TRUE || first_time == FALSE)
		goto bail;

	first_time = FALSE;

	/* If no valid service found the first time around, then request services refresh */
	if (! dbus_g_proxy_call (device, "DiscoverServices", &e, G_TYPE_STRING, NULL,
				 G_TYPE_INVALID, dbus_g_type_get_map("GHashTable", G_TYPE_UINT, G_TYPE_STRING),
				 &props, G_TYPE_INVALID)) {
		goto bail;
	}
	goto again;

bail:
	g_object_unref (device);

	if (found == FALSE)
		*error = g_strdup_printf (_("Obex Push file transfer unsupported"));

	return found;
#endif
	return TRUE;
}

static gboolean
destroy (NstPlugin *plugin)
{
	gtk_widget_destroy (combo);
	g_free (cmd);
	return TRUE;
}

static
NstPluginInfo plugin_info = {
	"bluetooth",
	"bluetooth",
	N_("Bluetooth (OBEX Push)"),
	GETTEXT_PACKAGE,
	CAJA_CAPS_NONE,
	init,
	get_contacts_widget,
	validate_destination,
	send_files,
	destroy
};

NST_INIT_PLUGIN (plugin_info)

