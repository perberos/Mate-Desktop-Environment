/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2009  Bastien Nocera <hadess@hadess.net>
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>
#include <bluetooth-plugin.h>
#include <mateconf/mateconf-client.h>

#include "bluetooth-client.h"

#define GPS_KEY "/apps/geoclue/master/org.freedesktop.Geoclue.GPSDevice"

static char *
get_name_and_type (const char *address, BluetoothType *ret_type)
{
	BluetoothClient *client;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean cont;
	char *found_name;

	found_name = NULL;
	*ret_type = 0;
	client = bluetooth_client_new (); 
	model = bluetooth_client_get_device_model (client, NULL);
	if (model == NULL) {
		g_object_unref (client);
		return NULL;
	}

	cont = gtk_tree_model_get_iter_first(model, &iter);
	while (cont != FALSE) {
		char *bdaddr, *name;
		BluetoothType type;

		gtk_tree_model_get(model, &iter,
				   BLUETOOTH_COLUMN_ADDRESS, &bdaddr,
				   BLUETOOTH_COLUMN_ALIAS, &name,
				   BLUETOOTH_COLUMN_TYPE, &type,
				   -1);
		if (g_strcmp0 (bdaddr, address) == 0) {
			g_free (bdaddr);
			found_name = name;
			*ret_type = type;
			break;
		}
		g_free (bdaddr);
		g_free (name);

		cont = gtk_tree_model_iter_next(model, &iter);
	}

	g_object_unref (model);
	g_object_unref (client);

	return found_name;
}

static gboolean
has_config_widget (const char *bdaddr, const char **uuids)
{
	gboolean has_sp = FALSE;
	BluetoothType type;
	char *name;
	guint i;

	if (uuids == NULL)
		return FALSE;
	for (i = 0; uuids[i] != NULL; i++) {
		if (g_str_equal (uuids[i], "SerialPort")) {
			has_sp = TRUE;
			break;
		}
	}
	if (has_sp == FALSE)
		return FALSE;

	/* Just SP? Probably a GPS */
	if (g_strv_length ((char **) uuids) == 1)
		return TRUE;

	/* Type is ANY, probably a GPS device */
	name = get_name_and_type (bdaddr, &type);
	if (name != NULL && type == BLUETOOTH_TYPE_ANY) {
		g_free (name);
		return TRUE;
	}

	if (name == NULL)
		return FALSE;
	/* GPS in the name? */
	if (strstr (name, "GPS") != NULL) {
		g_free (name);
		return TRUE;
	}

	g_free (name);
	return FALSE;
}

static void
toggle_button (GtkToggleButton *button, gpointer user_data)
{
	gboolean state;
	MateConfClient *client;
	const char *bdaddr;

	client = g_object_get_data (G_OBJECT (button), "client");
	bdaddr = g_object_get_data (G_OBJECT (button), "bdaddr");

	state = gtk_toggle_button_get_active (button);
	if (state == FALSE) {
		const char *old_bdaddr;
		old_bdaddr = g_object_get_data (G_OBJECT (button), "old-bdaddr");
		mateconf_client_set_string (client, GPS_KEY, old_bdaddr ? old_bdaddr : "", NULL);
	} else {
		char *old_bdaddr;

		old_bdaddr = mateconf_client_get_string (client, GPS_KEY, NULL);
		g_object_set_data_full (G_OBJECT (button), "old-bdaddr",
					old_bdaddr, g_free);
		mateconf_client_set_string (client, GPS_KEY, bdaddr, NULL);
	}
}

static GtkWidget *
get_config_widgets (const char *bdaddr, const char **uuids)
{
	GtkWidget *button;
	MateConfClient *client;
	char *old_bdaddr;

	client = mateconf_client_get_default ();
	if (client == NULL)
		return NULL;

	button = gtk_check_button_new_with_label (_("Use this GPS device for Geolocation services"));
	g_object_set_data_full (G_OBJECT (button), "bdaddr", g_strdup (bdaddr), g_free);
	g_object_set_data_full (G_OBJECT (button), "client", client, g_object_unref);

	/* Is it already setup? */
	old_bdaddr = mateconf_client_get_string (client, GPS_KEY, NULL);
	if (g_strcmp0 (old_bdaddr, bdaddr) == 0) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
		g_object_set_data (G_OBJECT (button), "bdaddr", old_bdaddr);
	} else {
		g_free (old_bdaddr);
	}

	/* And set the signal */
	g_signal_connect (G_OBJECT (button), "toggled",
			  G_CALLBACK (toggle_button), NULL);

	return button;
}

static void
device_removed (const char *bdaddr)
{
	MateConfClient *client;
	char *str;

	client = mateconf_client_get_default ();
	if (client == NULL)
		return;

	str = mateconf_client_get_string (client, GPS_KEY, NULL);
	if (g_strcmp0 (str, bdaddr) == 0) {
		mateconf_client_set_string (client, GPS_KEY, "", NULL);
		g_message ("Device '%s' got disabled as a Geoclue GPS", bdaddr);
	}

	g_free (str);
	g_object_unref (client);
}

static GbtPluginInfo plugin_info = {
	"geoclue",
	has_config_widget,
	get_config_widgets,
	device_removed
};

GBT_INIT_PLUGIN(plugin_info)

