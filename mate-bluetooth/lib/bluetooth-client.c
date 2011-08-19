/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2005-2008  Marcel Holtmann <marcel@holtmann.org>
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

/**
 * SECTION:bluetooth-client
 * @short_description: Bluetooth client object
 * @stability: Stable
 * @include: bluetooth-client.h
 *
 * The #BluetoothClient object is used to query the state of Bluetooth
 * devices and adapters.
 **/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include <dbus/dbus-glib.h>

#include "bluetooth-client.h"
#include "bluetooth-client-private.h"
#include "bluetooth-client-glue.h"
#include "mate-bluetooth-enum-types.h"

#include "marshal.h"

#ifdef DEBUG
#define DBG(fmt, arg...) printf("%s:%s() " fmt "\n", __FILE__, __FUNCTION__ , ## arg)
#else
#define DBG(fmt...)
#endif

#ifndef DBUS_TYPE_G_OBJECT_PATH_ARRAY
#define DBUS_TYPE_G_OBJECT_PATH_ARRAY \
	(dbus_g_type_get_collection("GPtrArray", DBUS_TYPE_G_OBJECT_PATH))
#endif

#ifndef DBUS_TYPE_G_DICTIONARY
#define DBUS_TYPE_G_DICTIONARY \
	(dbus_g_type_get_map("GHashTable", G_TYPE_STRING, G_TYPE_VALUE))
#endif

#define BLUEZ_SERVICE	"org.bluez"

#define BLUEZ_MANAGER_PATH	"/"
#define BLUEZ_MANAGER_INTERFACE	"org.bluez.Manager"
#define BLUEZ_ADAPTER_INTERFACE	"org.bluez.Adapter"
#define BLUEZ_DEVICE_INTERFACE	"org.bluez.Device"

static char * detectable_interfaces[] = {
	"org.bluez.Headset",
	"org.bluez.AudioSink",
	"org.bluez.Audio",
	"org.bluez.Input"
};

static char * connectable_interfaces[] = {
	"org.bluez.Audio",
	"org.bluez.Input"
};

/* Keep in sync with above */
#define BLUEZ_INPUT_INTERFACE	(connectable_interfaces[1])
#define BLUEZ_AUDIO_INTERFACE (connectable_interfaces[0])
#define BLUEZ_HEADSET_INTERFACE (detectable_interfaces[0])
#define BLUEZ_AUDIOSINK_INTERFACE (detectable_interfaces[1])

static DBusGConnection *connection = NULL;

#define BLUETOOTH_CLIENT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
				BLUETOOTH_TYPE_CLIENT, BluetoothClientPrivate))

typedef struct _BluetoothClientPrivate BluetoothClientPrivate;

struct _BluetoothClientPrivate {
	DBusGProxy *dbus;
	DBusGProxy *manager;
	GtkTreeStore *store;
	char *default_adapter;
	gboolean default_adapter_powered;
};

enum {
	PROP_0,
	PROP_DEFAULT_ADAPTER,
	PROP_DEFAULT_ADAPTER_POWERED,
};

G_DEFINE_TYPE(BluetoothClient, bluetooth_client, G_TYPE_OBJECT)

/**
 * bluetooth_type_to_string:
 * @type: a #BluetoothType
 *
 * Returns the string representation of the @type passed. Do not free the return value.
 *
 * Return value: a string.
 **/
const gchar *bluetooth_type_to_string(BluetoothType type)
{
	switch (type) {
	case BLUETOOTH_TYPE_ANY:
		return _("All types");
	case BLUETOOTH_TYPE_PHONE:
		return _("Phone");
	case BLUETOOTH_TYPE_MODEM:
		return _("Modem");
	case BLUETOOTH_TYPE_COMPUTER:
		return _("Computer");
	case BLUETOOTH_TYPE_NETWORK:
		return _("Network");
	case BLUETOOTH_TYPE_HEADSET:
		/* translators: a hands-free headset, a combination of a single speaker with a microphone */
		return _("Headset");
	case BLUETOOTH_TYPE_HEADPHONES:
		return _("Headphones");
	case BLUETOOTH_TYPE_OTHER_AUDIO:
		return _("Audio device");
	case BLUETOOTH_TYPE_KEYBOARD:
		return _("Keyboard");
	case BLUETOOTH_TYPE_MOUSE:
		return _("Mouse");
	case BLUETOOTH_TYPE_CAMERA:
		return _("Camera");
	case BLUETOOTH_TYPE_PRINTER:
		return _("Printer");
	case BLUETOOTH_TYPE_JOYPAD:
		return _("Joypad");
	case BLUETOOTH_TYPE_TABLET:
		return _("Tablet");
	case BLUETOOTH_TYPE_VIDEO:
		return _("Video device");
	default:
		return _("Unknown");
	}
}

/**
 * bluetooth_verify_address:
 * @bdaddr: a string representing a Bluetooth address
 *
 * Returns whether the string is a valid Bluetooth address. This does not contact the device in any way.
 *
 * Return value: %TRUE if the address is valid, %FALSE if not.
 **/
gboolean
bluetooth_verify_address (const char *bdaddr)
{
	gboolean retval = TRUE;
	char **elems;
	guint i;

	g_return_val_if_fail (bdaddr != NULL, FALSE);

	if (strlen (bdaddr) != 17)
		return FALSE;

	elems = g_strsplit (bdaddr, ":", -1);
	if (elems == NULL)
		return FALSE;
	if (g_strv_length (elems) != 6) {
		g_strfreev (elems);
		return FALSE;
	}
	for (i = 0; i < 6; i++) {
		if (strlen (elems[i]) != 2 ||
		    g_ascii_isxdigit (elems[i][0]) == FALSE ||
		    g_ascii_isxdigit (elems[i][1]) == FALSE) {
			retval = FALSE;
			break;
		}
	}

	g_strfreev (elems);
	return retval;
}

guint
bluetooth_class_to_type (guint32 class)
{
	switch ((class & 0x1f00) >> 8) {
	case 0x01:
		return BLUETOOTH_TYPE_COMPUTER;
	case 0x02:
		switch ((class & 0xfc) >> 2) {
		case 0x01:
		case 0x02:
		case 0x03:
		case 0x05:
			return BLUETOOTH_TYPE_PHONE;
		case 0x04:
			return BLUETOOTH_TYPE_MODEM;
		}
		break;
	case 0x03:
		return BLUETOOTH_TYPE_NETWORK;
	case 0x04:
		switch ((class & 0xfc) >> 2) {
		case 0x01:
		case 0x02:
			return BLUETOOTH_TYPE_HEADSET;
		case 0x06:
			return BLUETOOTH_TYPE_HEADPHONES;
		case 0x0b: /* VCR */
		case 0x0c: /* Video Camera */
		case 0x0d: /* Camcorder */
			return BLUETOOTH_TYPE_VIDEO;
		default:
			return BLUETOOTH_TYPE_OTHER_AUDIO;
		}
		break;
	case 0x05:
		switch ((class & 0xc0) >> 6) {
		case 0x00:
			switch ((class & 0x1e) >> 2) {
			case 0x01:
			case 0x02:
				return BLUETOOTH_TYPE_JOYPAD;
			}
			break;
		case 0x01:
			return BLUETOOTH_TYPE_KEYBOARD;
		case 0x02:
			switch ((class & 0x1e) >> 2) {
			case 0x05:
				return BLUETOOTH_TYPE_TABLET;
			default:
				return BLUETOOTH_TYPE_MOUSE;
			}
		}
		break;
	case 0x06:
		if (class & 0x80)
			return BLUETOOTH_TYPE_PRINTER;
		if (class & 0x20)
			return BLUETOOTH_TYPE_CAMERA;
		break;
	}

	return 0;
}

typedef gboolean (*IterSearchFunc) (GtkTreeStore *store,
				GtkTreeIter *iter, gpointer user_data);

static gboolean iter_search(GtkTreeStore *store,
				GtkTreeIter *iter, GtkTreeIter *parent,
				IterSearchFunc func, gpointer user_data)
{
	gboolean cont, found = FALSE;

	if (parent == NULL)
		cont = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store),
									iter);
	else
		cont = gtk_tree_model_iter_children(GTK_TREE_MODEL(store),
								iter, parent);

	while (cont == TRUE) {
		GtkTreeIter child;

		found = func(store, iter, user_data);
		if (found == TRUE)
			break;

		found = iter_search(store, &child, iter, func, user_data);
		if (found == TRUE) {
			*iter = child;
			break;
		}

		cont = gtk_tree_model_iter_next(GTK_TREE_MODEL(store), iter);
	}

	return found;
}

static gboolean compare_path(GtkTreeStore *store,
					GtkTreeIter *iter, gpointer user_data)
{
	const gchar *path = user_data;
	DBusGProxy *object;
	gboolean found = FALSE;

	gtk_tree_model_get(GTK_TREE_MODEL(store), iter,
					BLUETOOTH_COLUMN_PROXY, &object, -1);

	if (object != NULL) {
		found = g_str_equal(path, dbus_g_proxy_get_path(object));
		g_object_unref(object);
	}

	return found;
}

static gboolean
compare_address (GtkTreeStore *store,
		 GtkTreeIter *iter,
		 gpointer user_data)
{
	const char *address = user_data;
	char *tmp_address;
	gboolean found = FALSE;

	gtk_tree_model_get (GTK_TREE_MODEL(store), iter,
			    BLUETOOTH_COLUMN_ADDRESS, &tmp_address, -1);
	found = g_str_equal (address, tmp_address);
	g_free (tmp_address);

	return found;
}

static gboolean
get_iter_from_path (GtkTreeStore *store,
		    GtkTreeIter *iter,
		    const char *path)
{
	return iter_search(store, iter, NULL, compare_path, (gpointer) path);
}

static gboolean
get_iter_from_proxy(GtkTreeStore *store,
		    GtkTreeIter *iter,
		    DBusGProxy *proxy)
{
	return iter_search(store, iter, NULL, compare_path,
			   (gpointer) dbus_g_proxy_get_path (proxy));
}

static gboolean
get_iter_from_address (GtkTreeStore *store,
		       GtkTreeIter  *iter,
		       const char   *address,
		       DBusGProxy   *adapter)
{
	GtkTreeIter parent_iter;

	if (get_iter_from_proxy (store, &parent_iter, adapter) == FALSE)
		return FALSE;

	return iter_search (store, iter, &parent_iter, compare_address, (gpointer) address);
}

static void
device_services_changed (DBusGProxy *iface, const char *property,
			 GValue *value, gpointer user_data)
{
	BluetoothClient *client = user_data;
	BluetoothClientPrivate *priv = BLUETOOTH_CLIENT_GET_PRIVATE(client);
	GtkTreeIter iter;
	GtkTreePath *tree_path;
	GHashTable *table;
	const char *path;
	BluetoothStatus status;

	if (g_str_equal (property, "Connected") != FALSE) {
		status = g_value_get_boolean (value) ?
			BLUETOOTH_STATUS_CONNECTED :
			BLUETOOTH_STATUS_DISCONNECTED;
	} else if (g_str_equal (property, "State") != FALSE) {
		GEnumClass *eclass;
		GEnumValue *ev;
		eclass = g_type_class_ref (BLUETOOTH_TYPE_STATUS);
		ev = g_enum_get_value_by_nick (eclass, g_value_get_string (value));
		if (ev == NULL) {
			g_warning ("Unknown status '%s'", g_value_get_string (value));
			status = BLUETOOTH_STATUS_DISCONNECTED;
		} else {
			status = ev->value;
		}
		g_type_class_unref (eclass);
	} else
		return;

	path = dbus_g_proxy_get_path (iface);
	if (get_iter_from_path (priv->store, &iter, path) == FALSE)
		return;

	gtk_tree_model_get(GTK_TREE_MODEL (priv->store), &iter,
			   BLUETOOTH_COLUMN_SERVICES, &table,
			   -1);

	g_hash_table_insert (table,
			     (gpointer) dbus_g_proxy_get_interface (iface),
			     GINT_TO_POINTER (status));

	tree_path = gtk_tree_model_get_path (GTK_TREE_MODEL (priv->store), &iter);
	gtk_tree_model_row_changed (GTK_TREE_MODEL (priv->store), tree_path, &iter);
	gtk_tree_path_free (tree_path);
}

static GHashTable *
device_list_nodes (DBusGProxy *device, BluetoothClient *client, gboolean connect_signal)
{
	GHashTable *table;
	guint i;

	if (device == NULL)
		return NULL;

	table = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, NULL);

	for (i = 0; i < G_N_ELEMENTS (detectable_interfaces); i++) {
		DBusGProxy *iface;
		GHashTable *props;

		/* Don't add the input interface for devices that already have
		 * audio stuff */
		if (g_str_equal (detectable_interfaces[i], BLUEZ_INPUT_INTERFACE)
		    && g_hash_table_size (table) > 0)
			continue;

		/* Don't add the audio interface if there's no Headset or AudioSink,
		 * that means that it could only receive audio */
		if (g_str_equal (detectable_interfaces[i], BLUEZ_AUDIO_INTERFACE)) {
			if (g_hash_table_lookup (table, BLUEZ_HEADSET_INTERFACE) == NULL &&
			    g_hash_table_lookup (table, BLUEZ_AUDIOSINK_INTERFACE) == NULL) {
				continue;
			}
		}

		/* And skip interface if it's already in the hash table */
		if (g_hash_table_lookup (table, detectable_interfaces[i]) != NULL)
			continue;

		iface = dbus_g_proxy_new_from_proxy (device, detectable_interfaces[i], NULL);
		if (dbus_g_proxy_call (iface, "GetProperties", NULL,
				       G_TYPE_INVALID, dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_VALUE), &props,
				       G_TYPE_INVALID) != FALSE) {
			GValue *value;
			BluetoothStatus status;

			value = g_hash_table_lookup(props, "Connected");
			if (value != NULL) {
				status = g_value_get_boolean(value) ?
					BLUETOOTH_STATUS_CONNECTED :
					BLUETOOTH_STATUS_DISCONNECTED;
			} else {
				GEnumClass *eclass;
				GEnumValue *ev;

				eclass = g_type_class_ref (BLUETOOTH_TYPE_STATUS);
				value = g_hash_table_lookup(props, "State");
				ev = g_enum_get_value_by_nick (eclass, g_value_get_string (value));
				if (ev == NULL) {
					g_warning ("Unknown status '%s'", g_value_get_string (value));
					status = BLUETOOTH_STATUS_DISCONNECTED;
				} else {
					status = ev->value;
				}
				g_type_class_unref (eclass);
			}

			g_hash_table_insert (table, (gpointer) detectable_interfaces[i], GINT_TO_POINTER (status));

			if (connect_signal != FALSE) {
				dbus_g_proxy_add_signal(iface, "PropertyChanged",
							G_TYPE_STRING, G_TYPE_VALUE, G_TYPE_INVALID);
				dbus_g_proxy_connect_signal(iface, "PropertyChanged",
							    G_CALLBACK(device_services_changed), client, NULL);
			}
		}
	}

	if (g_hash_table_size (table) == 0) {
		g_hash_table_destroy (table);
		return NULL;
	}

	return table;
}

static const char *
uuid16_custom_to_string (guint uuid16, const char *uuid)
{
	switch (uuid16) {
	case 0x2:
		return "SyncMLClient";
	case 0x5601:
		return "Nokia SyncML Server";
	default:
		g_debug ("Unhandled custom UUID %s (0x%x)", uuid, uuid16);
		return NULL;
	}
}

/* Short names from Table 2 at:
 * https://www.bluetooth.org/Technical/AssignedNumbers/service_discovery.htm */
static const char *
uuid16_to_string (guint uuid16, const char *uuid)
{
	switch (uuid16) {
	case 0x1101:
		return "SerialPort";
	case 0x1103:
		return "DialupNetworking";
	case 0x1104:
		return "IrMCSync";
	case 0x1105:
		return "OBEXObjectPush";
	case 0x1106:
		return "OBEXFileTransfer";
	case 0x1108:
		return "HSP";
	case 0x110A:
		return "AudioSource";
	case 0x110B:
		return "AudioSink";
	case 0x110c:
		return "A/V_RemoteControlTarget";
	case 0x110e:
		return "A/V_RemoteControl";
	case 0x1112:
		return "Headset_-_AG";
	case 0x1115:
		return "PANU";
	case 0x1116:
		return "NAP";
	case 0x1117:
		return "GN";
	case 0x111e:
		return "Handsfree";
	case 0x111F:
		return "HandsfreeAudioGateway";
	case 0x1124:
		return "HumanInterfaceDeviceService";
	case 0x112d:
		return "SIM_Access";
	case 0x112F:
		return "Phonebook_Access_-_PSE";
	case 0x1203:
		return "GenericAudio";
	case 0x1000: /* ServiceDiscoveryServerServiceClassID */
	case 0x1200: /* PnPInformation */
		/* Those are ignored */
		return NULL;
	case 0x1201:
		return "GenericNetworking";
	case 0x1303:
		return "VideoSource";
	case 0x8e771303:
	case 0x8e771301:
		return "SEMC HLA";
	case 0x8e771401:
		return "SEMC Watch Phone";
	default:
		g_debug ("Unhandled UUID %s (0x%x)", uuid, uuid16);
		return NULL;
	}
}

/**
 * bluetooth_uuid_to_string:
 * @uuid: a string representing a Bluetooth UUID
 *
 * Returns a string representing a human-readable (but not usable for display to users) version of the @uuid. Do not free the return value.
 *
 * Return value: a string.
 **/
const char *
bluetooth_uuid_to_string (const char *uuid)
{
	char **parts;
	guint uuid16;
	gboolean is_custom = FALSE;

	if (g_str_has_suffix (uuid, "-0000-1000-8000-0002ee000002") != FALSE)
		is_custom = TRUE;

	parts = g_strsplit (uuid, "-", -1);
	if (parts == NULL || parts[0] == NULL) {
		g_strfreev (parts);
		return NULL;
	}

	uuid16 = g_ascii_strtoull (parts[0], NULL, 16);
	g_strfreev (parts);
	if (uuid16 == 0)
		return NULL;

	if (is_custom == FALSE)
		return uuid16_to_string (uuid16, uuid);
	return uuid16_custom_to_string (uuid16, uuid);
}

static char **
device_list_uuids (GValue *value)
{
	GPtrArray *ret;
	char **uuids;
	guint i;

	if (value == NULL)
		return NULL;

	uuids = g_value_get_boxed (value);
	if (uuids == NULL)
		return NULL;

	ret = g_ptr_array_new ();

	for (i = 0; uuids[i] != NULL; i++) {
		const char *uuid;

		uuid = bluetooth_uuid_to_string (uuids[i]);
		if (uuid == NULL)
			continue;
		g_ptr_array_add (ret, g_strdup (uuid));
	}
	if (ret->len == 0) {
		g_ptr_array_free (ret, TRUE);
		return NULL;
	}

	g_ptr_array_add (ret, NULL);

	return (char **) g_ptr_array_free (ret, FALSE);
}

static void device_changed(DBusGProxy *device, const char *property,
					GValue *value, gpointer user_data)
{
	BluetoothClient *client = BLUETOOTH_CLIENT(user_data);
	BluetoothClientPrivate *priv = BLUETOOTH_CLIENT_GET_PRIVATE(client);
	GtkTreeIter iter;

	DBG("client %p property %s", client, property);

	if (get_iter_from_proxy(priv->store, &iter, device) == FALSE)
		return;

	if (g_str_equal(property, "Name") == TRUE) {
		const gchar *name = g_value_get_string(value);

		gtk_tree_store_set(priv->store, &iter,
					BLUETOOTH_COLUMN_NAME, name, -1);
	} else if (g_str_equal(property, "Alias") == TRUE) {
		const gchar *alias = g_value_get_string(value);

		gtk_tree_store_set(priv->store, &iter,
					BLUETOOTH_COLUMN_ALIAS, alias, -1);
	} else if (g_str_equal(property, "Icon") == TRUE) {
		const gchar *icon = g_value_get_string(value);

		gtk_tree_store_set(priv->store, &iter,
					BLUETOOTH_COLUMN_ICON, icon, -1);
	} else if (g_str_equal(property, "Paired") == TRUE) {
		gboolean paired = g_value_get_boolean(value);

		gtk_tree_store_set(priv->store, &iter,
				BLUETOOTH_COLUMN_PAIRED, paired, -1);
	} else if (g_str_equal(property, "Trusted") == TRUE) {
		gboolean trusted = g_value_get_boolean(value);

		gtk_tree_store_set(priv->store, &iter,
				BLUETOOTH_COLUMN_TRUSTED, trusted, -1);
	} else if (g_str_equal(property, "Connected") == TRUE) {
		gboolean connected = g_value_get_boolean(value);

		gtk_tree_store_set(priv->store, &iter,
				BLUETOOTH_COLUMN_CONNECTED, connected, -1);
	} else if (g_str_equal (property, "UUIDs") == TRUE) {
		GHashTable *services;
		char **uuids;

		services = device_list_nodes (device, client, TRUE);
		uuids = device_list_uuids (value);
		gtk_tree_store_set(priv->store, &iter,
				   BLUETOOTH_COLUMN_SERVICES, services,
				   BLUETOOTH_COLUMN_UUIDS, uuids, -1);
		if (services != NULL)
			g_hash_table_unref (services);
		g_strfreev (uuids);
	}
}

static void add_device(DBusGProxy *adapter, GtkTreeIter *parent,
					BluetoothClient *client,
					const char *path, GHashTable *hash)
{
	BluetoothClientPrivate *priv = BLUETOOTH_CLIENT_GET_PRIVATE(client);
	DBusGProxy *device;
	GValue *value;
	const gchar *address, *alias, *name, *icon;
	char **uuids;
	GHashTable *services;
	gboolean paired, trusted, connected;
	int legacypairing;
	guint type;
	GtkTreeIter iter;
	gboolean cont;

	services = NULL;

	if (hash == NULL) {
		device = dbus_g_proxy_new_from_proxy(adapter,
						BLUEZ_DEVICE_INTERFACE, path);

		if (device != NULL)
			device_get_properties(device, &hash, NULL);
	} else
		device = NULL;

	if (hash != NULL) {
		value = g_hash_table_lookup(hash, "Address");
		address = value ? g_value_get_string(value) : NULL;

		value = g_hash_table_lookup(hash, "Alias");
		alias = value ? g_value_get_string(value) : NULL;

		value = g_hash_table_lookup(hash, "Name");
		name = value ? g_value_get_string(value) : NULL;

		value = g_hash_table_lookup(hash, "Class");
		type = value ? bluetooth_class_to_type(g_value_get_uint(value)) : BLUETOOTH_TYPE_ANY;

		value = g_hash_table_lookup(hash, "Icon");
		icon = value ? g_value_get_string(value) : "bluetooth";

		value = g_hash_table_lookup(hash, "Paired");
		paired = value ? g_value_get_boolean(value) : FALSE;

		value = g_hash_table_lookup(hash, "Trusted");
		trusted = value ? g_value_get_boolean(value) : FALSE;

		value = g_hash_table_lookup(hash, "Connected");
		connected = value ? g_value_get_boolean(value) : FALSE;

		value = g_hash_table_lookup(hash, "UUIDs");
		uuids = device_list_uuids (value);

		value = g_hash_table_lookup(hash, "LegacyPairing");
		legacypairing = value ? g_value_get_boolean(value) : -1;
	} else {
		if (device)
			g_object_unref (device);
		return;
	}

	cont = gtk_tree_model_iter_children(GTK_TREE_MODEL(priv->store),
								&iter, parent);

	while (cont == TRUE) {
		gchar *value;

		gtk_tree_model_get(GTK_TREE_MODEL(priv->store), &iter,
					BLUETOOTH_COLUMN_ADDRESS, &value, -1);

		if (g_ascii_strcasecmp(address, value) == 0) {
			gtk_tree_store_set(priv->store, &iter,
					BLUETOOTH_COLUMN_ADDRESS, address,
					BLUETOOTH_COLUMN_ALIAS, alias,
					BLUETOOTH_COLUMN_NAME, name,
					BLUETOOTH_COLUMN_TYPE, type,
					BLUETOOTH_COLUMN_ICON, icon,
					BLUETOOTH_COLUMN_LEGACYPAIRING, legacypairing,
					-1);
			if (uuids != NULL) {
				gtk_tree_store_set(priv->store, &iter,
						   BLUETOOTH_COLUMN_UUIDS, uuids,
						   -1);
			}

			if (device != NULL) {
				services = device_list_nodes (device, client, FALSE);

				gtk_tree_store_set(priv->store, &iter,
					BLUETOOTH_COLUMN_PROXY, device,
					BLUETOOTH_COLUMN_CONNECTED, connected,
					BLUETOOTH_COLUMN_TRUSTED, trusted,
					BLUETOOTH_COLUMN_PAIRED, paired,
					BLUETOOTH_COLUMN_SERVICES, services,
					-1);
			}

			goto done;
		}
		g_free (value);

		cont = gtk_tree_model_iter_next(GTK_TREE_MODEL(priv->store), &iter);
	}

	services = device_list_nodes (device, client, TRUE);

	gtk_tree_store_insert_with_values(priv->store, &iter, parent, -1,
				BLUETOOTH_COLUMN_PROXY, device,
				BLUETOOTH_COLUMN_ADDRESS, address,
				BLUETOOTH_COLUMN_ALIAS, alias,
				BLUETOOTH_COLUMN_NAME, name,
				BLUETOOTH_COLUMN_TYPE, type,
				BLUETOOTH_COLUMN_ICON, icon,
				BLUETOOTH_COLUMN_PAIRED, paired,
				BLUETOOTH_COLUMN_TRUSTED, trusted,
				BLUETOOTH_COLUMN_CONNECTED, connected,
				BLUETOOTH_COLUMN_SERVICES, services,
				BLUETOOTH_COLUMN_UUIDS, uuids,
				BLUETOOTH_COLUMN_LEGACYPAIRING, legacypairing,
				-1);

done:
	if (device != NULL) {
		dbus_g_proxy_add_signal(device, "PropertyChanged",
				G_TYPE_STRING, G_TYPE_VALUE, G_TYPE_INVALID);
		dbus_g_proxy_connect_signal(device, "PropertyChanged",
				G_CALLBACK(device_changed), client, NULL);
		g_object_unref(device);
	}
	g_strfreev (uuids);
	if (services)
		g_hash_table_unref (services);
}

static void device_found(DBusGProxy *adapter, const char *address,
					GHashTable *hash, gpointer user_data)
{
	BluetoothClient *client = user_data;
	BluetoothClientPrivate *priv = BLUETOOTH_CLIENT_GET_PRIVATE(client);
	GtkTreeIter iter;

	DBG("client %p address %s", client, address);

	if (get_iter_from_proxy(priv->store, &iter, adapter) == TRUE)
		add_device(adapter, &iter, client, NULL, hash);
}

static void device_created(DBusGProxy *adapter,
				const char *path, gpointer user_data)
{
	BluetoothClient *client = user_data;
	BluetoothClientPrivate *priv = BLUETOOTH_CLIENT_GET_PRIVATE(client);
	GtkTreeIter iter;

	DBG("client %p path %s", client, path);

	if (get_iter_from_proxy(priv->store, &iter, adapter) == TRUE)
		add_device(adapter, &iter, client, path, NULL);
}

static void device_removed(DBusGProxy *adapter,
				const char *path, gpointer user_data)
{
	BluetoothClient *client = user_data;
	BluetoothClientPrivate *priv = BLUETOOTH_CLIENT_GET_PRIVATE(client);
	GtkTreeIter iter;

	DBG("client %p path %s", client, path);

	if (get_iter_from_path(priv->store, &iter, path) == TRUE)
		gtk_tree_store_remove(priv->store, &iter);
}

static void adapter_changed(DBusGProxy *adapter, const char *property,
					GValue *value, gpointer user_data)
{
	BluetoothClient *client = BLUETOOTH_CLIENT(user_data);
	BluetoothClientPrivate *priv = BLUETOOTH_CLIENT_GET_PRIVATE(client);
	GtkTreeIter iter;
	gboolean notify = FALSE;

	DBG("client %p property %s", client, property);

	if (get_iter_from_proxy(priv->store, &iter, adapter) == FALSE)
		return;

	if (g_str_equal(property, "Name") == TRUE) {
		const gchar *name = g_value_get_string(value);

		gtk_tree_store_set(priv->store, &iter,
					BLUETOOTH_COLUMN_NAME, name, -1);
	} else if (g_str_equal(property, "Discovering") == TRUE) {
		gboolean discovering = g_value_get_boolean(value);

		gtk_tree_store_set(priv->store, &iter,
				BLUETOOTH_COLUMN_DISCOVERING, discovering, -1);
		notify = TRUE;
	} else if (g_str_equal(property, "Powered") == TRUE) {
		gboolean powered = g_value_get_boolean(value);
		gboolean is_default;

		gtk_tree_store_set(priv->store, &iter,
				   BLUETOOTH_COLUMN_POWERED, powered, -1);
		gtk_tree_model_get(GTK_TREE_MODEL(priv->store), &iter,
				   BLUETOOTH_COLUMN_DEFAULT, &is_default, -1);
		if (is_default != FALSE && powered != priv->default_adapter_powered) {
			priv->default_adapter_powered = powered;
			g_object_notify (G_OBJECT (client), "default-adapter-powered");
		}
	} else if (g_str_equal(property, "Discoverable") == TRUE) {
		gboolean discoverable = g_value_get_boolean(value);

		gtk_tree_store_set(priv->store, &iter,
				BLUETOOTH_COLUMN_DISCOVERABLE, discoverable, -1);
		notify = TRUE;
	}

	if (notify != FALSE) {
		GtkTreePath *path;

		/* Tell the world */
		path = gtk_tree_model_get_path (GTK_TREE_MODEL (priv->store), &iter);
		gtk_tree_model_row_changed (GTK_TREE_MODEL (priv->store), path, &iter);
		gtk_tree_path_free (path);
	}
}

static void adapter_added(DBusGProxy *manager,
				const char *path, gpointer user_data)
{
	BluetoothClient *client = user_data;
	BluetoothClientPrivate *priv = BLUETOOTH_CLIENT_GET_PRIVATE(client);
	GtkTreeIter iter;
	DBusGProxy *adapter;
	GPtrArray *devices;
	GHashTable *hash = NULL;
	GValue *value;
	const gchar *address, *name;
	gboolean discovering, discoverable, powered;

	DBG("client %p path %s", client, path);

	adapter = dbus_g_proxy_new_from_proxy(manager,
					BLUEZ_ADAPTER_INTERFACE, path);

	adapter_get_properties(adapter, &hash, NULL);
	if (hash != NULL) {
		value = g_hash_table_lookup(hash, "Address");
		address = value ? g_value_get_string(value) : NULL;

		value = g_hash_table_lookup(hash, "Name");
		name = value ? g_value_get_string(value) : NULL;

		value = g_hash_table_lookup(hash, "Discovering");
		discovering = value ? g_value_get_boolean(value) : FALSE;

		value = g_hash_table_lookup(hash, "Powered");
		powered = value ? g_value_get_boolean(value) : FALSE;

		value = g_hash_table_lookup(hash, "Devices");
		devices = value ? g_value_get_boxed (value) : NULL;

		value = g_hash_table_lookup (hash, "Discoverable");
		discoverable = value ? g_value_get_boolean (value) : FALSE;
	} else {
		address = NULL;
		name = NULL;
		discovering = FALSE;
		discoverable = FALSE;
		powered = FALSE;
		devices = NULL;
	}

	gtk_tree_store_insert_with_values(priv->store, &iter, NULL, -1,
					  BLUETOOTH_COLUMN_PROXY, adapter,
					  BLUETOOTH_COLUMN_ADDRESS, address,
					  BLUETOOTH_COLUMN_NAME, name,
					  BLUETOOTH_COLUMN_DISCOVERING, discovering,
					  BLUETOOTH_COLUMN_DISCOVERABLE, discoverable,
					  BLUETOOTH_COLUMN_POWERED, powered,
					  -1);

	dbus_g_proxy_add_signal(adapter, "PropertyChanged",
				G_TYPE_STRING, G_TYPE_VALUE, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(adapter, "PropertyChanged",
				G_CALLBACK(adapter_changed), client, NULL);

	dbus_g_proxy_add_signal(adapter, "DeviceCreated",
				DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(adapter, "DeviceCreated",
				G_CALLBACK(device_created), client, NULL);

	dbus_g_proxy_add_signal(adapter, "DeviceRemoved",
				DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(adapter, "DeviceRemoved",
				G_CALLBACK(device_removed), client, NULL);

	dbus_g_proxy_add_signal(adapter, "DeviceFound",
			G_TYPE_STRING, DBUS_TYPE_G_DICTIONARY, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(adapter, "DeviceFound",
				G_CALLBACK(device_found), client, NULL);

	if (devices != NULL) {
		int i;

		for (i = 0; i < devices->len; i++) {
			gchar *path = g_ptr_array_index(devices, i);
			device_created(adapter, path, client);
			g_free(path);
		}
	}

	g_object_unref(adapter);
}

static void adapter_removed(DBusGProxy *manager,
				const char *path, gpointer user_data)
{
	BluetoothClient *client = user_data;
	BluetoothClientPrivate *priv = BLUETOOTH_CLIENT_GET_PRIVATE(client);
	GtkTreeIter iter;
	gboolean cont;

	DBG("client %p path %s", client, path);

	cont = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(priv->store),
									&iter);

	while (cont == TRUE) {
		DBusGProxy *adapter;
		const char *adapter_path;
		gboolean found;

		gtk_tree_model_get(GTK_TREE_MODEL(priv->store), &iter,
					BLUETOOTH_COLUMN_PROXY, &adapter, -1);

		adapter_path = dbus_g_proxy_get_path(adapter);

		found = g_str_equal(path, adapter_path);
		if (found == TRUE) {
			g_signal_handlers_disconnect_by_func(adapter,
						adapter_changed, client);
			g_signal_handlers_disconnect_by_func(adapter,
						device_created, client);
			g_signal_handlers_disconnect_by_func(adapter,
						device_removed, client);
			g_signal_handlers_disconnect_by_func(adapter,
						device_found, client);
		}

		g_object_unref(adapter);

		if (found == TRUE) {
			cont = gtk_tree_store_remove(priv->store, &iter);
			continue;
		}

		cont = gtk_tree_model_iter_next(GTK_TREE_MODEL(priv->store),
									&iter);
	}

	/* No adapters left in the tree? */
	if (gtk_tree_model_iter_n_children (GTK_TREE_MODEL(priv->store), NULL) == 0) {
		g_free(priv->default_adapter);
		priv->default_adapter = NULL;
		priv->default_adapter_powered = FALSE;
		g_object_notify (G_OBJECT (client), "default-adapter");
		g_object_notify (G_OBJECT (client), "default-adapter-powered");
	}
}

static void default_adapter_changed(DBusGProxy *manager,
				const char *path, gpointer user_data)
{
	BluetoothClient *client = user_data;
	BluetoothClientPrivate *priv = BLUETOOTH_CLIENT_GET_PRIVATE(client);
	GtkTreeIter iter;
	gboolean cont;

	DBG("client %p path %s", client, path);

	cont = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(priv->store),
									&iter);

	while (cont == TRUE) {
		DBusGProxy *adapter;
		const char *adapter_path;
		gboolean found, powered;

		gtk_tree_model_get(GTK_TREE_MODEL(priv->store), &iter,
				   BLUETOOTH_COLUMN_PROXY, &adapter,
				   BLUETOOTH_COLUMN_POWERED, &powered, -1);

		adapter_path = dbus_g_proxy_get_path(adapter);

		found = g_str_equal(path, adapter_path);

		g_object_unref(adapter);

		if (found != FALSE)
			priv->default_adapter_powered = powered;

		gtk_tree_store_set(priv->store, &iter,
					BLUETOOTH_COLUMN_DEFAULT, found, -1);

		cont = gtk_tree_model_iter_next(GTK_TREE_MODEL(priv->store),
									&iter);
	}

	/* Record the new default adapter */
	g_free(priv->default_adapter);
	priv->default_adapter = g_strdup(path);
	g_object_notify (G_OBJECT (client), "default-adapter");
	g_object_notify (G_OBJECT (client), "default-adapter-powered");
}

static void name_owner_changed(DBusGProxy *dbus, const char *name,
			const char *prev, const char *new, gpointer user_data)
{
	BluetoothClient *client = user_data;
	BluetoothClientPrivate *priv = BLUETOOTH_CLIENT_GET_PRIVATE(client);
	GtkTreeIter iter;
	gboolean cont;

	if (g_str_equal(name, BLUEZ_SERVICE) == FALSE || *new != '\0')
		return;

	DBG("client %p name %s", client, name);

	cont = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(priv->store),
									&iter);

	while (cont == TRUE)
		cont = gtk_tree_store_remove(priv->store, &iter);
}

static void bluetooth_client_init(BluetoothClient *client)
{
	BluetoothClientPrivate *priv = BLUETOOTH_CLIENT_GET_PRIVATE(client);
	GPtrArray *array = NULL;
	gchar *default_path = NULL;

	DBG("client %p", client);

	priv->store = gtk_tree_store_new(_BLUETOOTH_NUM_COLUMNS, G_TYPE_OBJECT,
					 G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
					 G_TYPE_UINT, G_TYPE_STRING,
					 G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN,
					 G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_INT,
					 G_TYPE_BOOLEAN, G_TYPE_HASH_TABLE, G_TYPE_STRV);

	priv->dbus = dbus_g_proxy_new_for_name(connection, DBUS_SERVICE_DBUS,
				DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS);

	dbus_g_proxy_add_signal(priv->dbus, "NameOwnerChanged",
					G_TYPE_STRING, G_TYPE_STRING,
					G_TYPE_STRING, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(priv->dbus, "NameOwnerChanged",
				G_CALLBACK(name_owner_changed), client, NULL);

	priv->manager = dbus_g_proxy_new_for_name(connection, BLUEZ_SERVICE,
				BLUEZ_MANAGER_PATH, BLUEZ_MANAGER_INTERFACE);

	dbus_g_proxy_add_signal(priv->manager, "AdapterAdded",
				DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(priv->manager, "AdapterAdded",
				G_CALLBACK(adapter_added), client, NULL);

	dbus_g_proxy_add_signal(priv->manager, "AdapterRemoved",
				DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(priv->manager, "AdapterRemoved",
				G_CALLBACK(adapter_removed), client, NULL);

	dbus_g_proxy_add_signal(priv->manager, "DefaultAdapterChanged",
				DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(priv->manager, "DefaultAdapterChanged",
			G_CALLBACK(default_adapter_changed), client, NULL);

	manager_list_adapters(priv->manager, &array, NULL);
	if (array != NULL) {
		int i;

		for (i = 0; i < array->len; i++) {
			gchar *path = g_ptr_array_index(array, i);
			adapter_added(priv->manager, path, client);
			g_free(path);
		}
	}

	manager_default_adapter(priv->manager, &default_path, NULL);
	if (default_path != NULL) {
		default_adapter_changed(priv->manager, default_path, client);
		g_free(default_path);
	}
}

static void
bluetooth_client_get_property (GObject        *object,
			       guint           property_id,
			       GValue         *value,
			       GParamSpec     *pspec)
{
	BluetoothClientPrivate *priv = BLUETOOTH_CLIENT_GET_PRIVATE(object);

	switch (property_id) {
	case PROP_DEFAULT_ADAPTER:
		g_value_set_string (value, priv->default_adapter);
		break;
	case PROP_DEFAULT_ADAPTER_POWERED:
		g_value_set_boolean (value, priv->default_adapter_powered);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void bluetooth_client_finalize(GObject *client)
{
	BluetoothClientPrivate *priv = BLUETOOTH_CLIENT_GET_PRIVATE(client);

	DBG("client %p", client);

	g_signal_handlers_disconnect_by_func(priv->dbus,
					name_owner_changed, client);
	g_object_unref(priv->dbus);

	g_signal_handlers_disconnect_by_func(priv->manager,
						adapter_added, client);
	g_signal_handlers_disconnect_by_func(priv->manager,
						adapter_removed, client);
	g_signal_handlers_disconnect_by_func(priv->manager,
					default_adapter_changed, client);
	g_object_unref(priv->manager);

	g_object_unref(priv->store);

	g_free (priv->default_adapter);
	priv->default_adapter = NULL;

	G_OBJECT_CLASS(bluetooth_client_parent_class)->finalize(client);
}

static void bluetooth_client_class_init(BluetoothClientClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	GError *error = NULL;

	DBG("class %p", klass);

	g_type_class_add_private(klass, sizeof(BluetoothClientPrivate));

	object_class->finalize = bluetooth_client_finalize;
	object_class->get_property = bluetooth_client_get_property;


	g_object_class_install_property (object_class, PROP_DEFAULT_ADAPTER,
					 g_param_spec_string ("default-adapter", NULL, NULL,
							      NULL, G_PARAM_READABLE));
	g_object_class_install_property (object_class, PROP_DEFAULT_ADAPTER_POWERED,
					 g_param_spec_boolean ("default-adapter-powered", NULL, NULL,
					 		       FALSE, G_PARAM_READABLE));

	dbus_g_object_register_marshaller(marshal_VOID__STRING_BOXED,
						G_TYPE_NONE, G_TYPE_STRING,
						G_TYPE_VALUE, G_TYPE_INVALID);

	connection = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);

	if (error != NULL) {
		g_printerr("Connecting to system bus failed: %s\n",
							error->message);
		g_error_free(error);
	}
}

/**
 * bluetooth_client_new:
 *
 * Returns a reference to the #BluetoothClient singleton. Use #g_object_unref() the object when done.
 *
 * Return value: a #BluetoothClient object.
 **/
BluetoothClient *bluetooth_client_new(void)
{
	static BluetoothClient *bluetooth_client = NULL;

	if (bluetooth_client != NULL)
		return g_object_ref(bluetooth_client);

	bluetooth_client = BLUETOOTH_CLIENT (g_object_new (BLUETOOTH_TYPE_CLIENT, NULL));
	g_object_add_weak_pointer (G_OBJECT (bluetooth_client),
				   (gpointer) &bluetooth_client);

	DBG("client %p", bluetooth_client);

	return bluetooth_client;
}

/**
 * bluetooth_client_get_model:
 * @client: a #BluetoothClient object
 *
 * Returns an unfiltered #GtkTreeModel representing the adapter and devices available on the system.
 *
 * Return value: a #GtkTreeModel object.
 **/
GtkTreeModel *bluetooth_client_get_model (BluetoothClient *client)
{
	BluetoothClientPrivate *priv;
	GtkTreeModel *model;

	g_return_val_if_fail (BLUETOOTH_IS_CLIENT (client), NULL);

	DBG("client %p", client);

	priv = BLUETOOTH_CLIENT_GET_PRIVATE(client);
	model = g_object_ref(priv->store);

	return model;
}

/**
 * bluetooth_client_get_filter_model:
 * @client: a #BluetoothClient object
 * @func: a #GtkTreeModelFilterVisibleFunc
 * @data: user data to pass to gtk_tree_model_filter_set_visible_func()
 * @destroy: a destroy function for gtk_tree_model_filter_set_visible_func()
 *
 * Returns a #GtkTreeModel of devices filtered using the @func, @data and @destroy arguments to pass to gtk_tree_model_filter_set_visible_func().
 *
 * Return value: a #GtkTreeModel object.
 **/
GtkTreeModel *bluetooth_client_get_filter_model (BluetoothClient *client,
						 GtkTreeModelFilterVisibleFunc func,
						 gpointer data, GDestroyNotify destroy)
{
	BluetoothClientPrivate *priv;
	GtkTreeModel *model;

	g_return_val_if_fail (BLUETOOTH_IS_CLIENT (client), NULL);

	DBG("client %p", client);

	priv = BLUETOOTH_CLIENT_GET_PRIVATE(client);
	model = gtk_tree_model_filter_new(GTK_TREE_MODEL(priv->store), NULL);

	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(model),
							func, data, destroy);

	return model;
}

static gboolean adapter_filter(GtkTreeModel *model,
					GtkTreeIter *iter, gpointer user_data)
{
	DBusGProxy *proxy;
	gboolean active;

	gtk_tree_model_get(model, iter, BLUETOOTH_COLUMN_PROXY, &proxy, -1);

	if (proxy == NULL)
		return FALSE;

	active = g_str_equal(BLUEZ_ADAPTER_INTERFACE,
					dbus_g_proxy_get_interface(proxy));

	g_object_unref(proxy);

	return active;
}

/**
 * bluetooth_client_get_adapter_model:
 * @client: a #BluetoothClient object
 *
 * Returns a filtered #GtkTreeModel with only adapters present.
 *
 * Return value: a #GtkTreeModel object.
 **/
GtkTreeModel *bluetooth_client_get_adapter_model (BluetoothClient *client)
{
	DBG("client %p", client);

	return bluetooth_client_get_filter_model (client, adapter_filter,
						  NULL, NULL);
}

/**
 * bluetooth_client_get_device_model:
 * @client: a #BluetoothClient object
 * @adapter: a #DBusGProxy of the adapter object, or %NULL to get the default adapter.
 *
 * Returns a filtered #GtkTreeModel with only devices belonging to the selected adapter listed. Note that the model will follow a specific adapter, and will not follow the default-adapter when %NULL is passed.
 *
 * Return value: a #GtkTreeModel object.
 **/
GtkTreeModel *bluetooth_client_get_device_model (BluetoothClient *client,
						 DBusGProxy *adapter)
{
	BluetoothClientPrivate *priv;
	GtkTreeModel *model;
	GtkTreePath *path;
	GtkTreeIter iter;
	gboolean cont, found = FALSE;

	g_return_val_if_fail (BLUETOOTH_IS_CLIENT (client), NULL);

	DBG("client %p", client);

	priv = BLUETOOTH_CLIENT_GET_PRIVATE(client);
	cont = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(priv->store),
									&iter);

	while (cont == TRUE) {
		DBusGProxy *proxy;
		gboolean is_default;

		gtk_tree_model_get(GTK_TREE_MODEL(priv->store), &iter,
				BLUETOOTH_COLUMN_PROXY, &proxy,
				BLUETOOTH_COLUMN_DEFAULT, &is_default, -1);

		if (adapter == NULL && is_default == TRUE)
			found = TRUE;

		if (proxy == adapter)
			found = TRUE;

		g_object_unref(proxy);

		if (found == TRUE)
			break;

		cont = gtk_tree_model_iter_next(GTK_TREE_MODEL(priv->store),
									&iter);
	}

	if (found == TRUE) {
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(priv->store),
									&iter);
		model = gtk_tree_model_filter_new(GTK_TREE_MODEL(priv->store),
									path);
		gtk_tree_path_free(path);
	} else
		model = NULL;

	return model;
}

/**
 * bluetooth_client_get_device_filter_model:
 * @client: a #BluetoothClient object
 * @adapter: a #DBusGProxy representing a particular adapter, or %NULL for the default adapter.
 * @func: a #GtkTreeModelFilterVisibleFunc
 * @data: user data to pass to gtk_tree_model_filter_set_visible_func()
 * @destroy: a destroy function for gtk_tree_model_filter_set_visible_func()
 *
 * Returns a #GtkTreeModel of adapters filtered using the @func, @data and
 * @destroy arguments to pass to gtk_tree_model_filter_set_visible_func().
 *
 * Return value: a #GtkTreeModel object.
 **/
GtkTreeModel *bluetooth_client_get_device_filter_model(BluetoothClient *client,
		DBusGProxy *adapter, GtkTreeModelFilterVisibleFunc func,
				gpointer data, GDestroyNotify destroy)
{
	GtkTreeModel *model;

	g_return_val_if_fail (BLUETOOTH_IS_CLIENT (client), NULL);

	DBG("client %p", client);

	model = bluetooth_client_get_device_model(client, adapter);

	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(model),
							func, data, destroy);

	return model;
}

/**
 * bluetooth_client_get_default_adapter:
 * @client: a #BluetoothClient object
 *
 * Returns a #DBusGProxy object representing the default adapter, or %NULL if no adapters are present.
 *
 * Return value: a #DBusGProxy object.
 **/
DBusGProxy *bluetooth_client_get_default_adapter(BluetoothClient *client)
{
	BluetoothClientPrivate *priv = BLUETOOTH_CLIENT_GET_PRIVATE(client);
	GtkTreeIter iter;
	gboolean cont;

	g_return_val_if_fail (BLUETOOTH_IS_CLIENT (client), NULL);

	DBG("client %p", client);

	cont = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(priv->store),
									&iter);

	while (cont == TRUE) {
		DBusGProxy *adapter;
		gboolean is_default;

		gtk_tree_model_get(GTK_TREE_MODEL(priv->store), &iter,
				BLUETOOTH_COLUMN_PROXY, &adapter,
				BLUETOOTH_COLUMN_DEFAULT, &is_default, -1);

		if (is_default == TRUE)
			return adapter;

		g_object_unref(adapter);

		cont = gtk_tree_model_iter_next(GTK_TREE_MODEL(priv->store),
									&iter);
	}

	return NULL;
}

/**
 * bluetooth_client_start_discovery:
 * @client: a #BluetoothClient object
 *
 * Start a discovery on the default adapter.
 *
 * Return value: Whether a discovery was successfully started on the default adapter.
 **/
gboolean bluetooth_client_start_discovery(BluetoothClient *client)
{
	DBusGProxy *adapter;

	g_return_val_if_fail (BLUETOOTH_IS_CLIENT (client), FALSE);

	DBG("client %p", client);

	adapter = bluetooth_client_get_default_adapter(client);
	if (adapter == NULL)
		return FALSE;

	adapter_start_discovery(adapter, NULL);

	g_object_unref(adapter);

	return TRUE;
}

/**
 * bluetooth_client_stop_discovery:
 * @client: a #BluetoothClient object
 *
 * Stop a discovery started on the default adapter.
 *
 * Return value: Whether a discovery was successfully stopped on the default adapter.
 **/
gboolean bluetooth_client_stop_discovery(BluetoothClient *client)
{
	DBusGProxy *adapter;

	g_return_val_if_fail (BLUETOOTH_IS_CLIENT (client), FALSE);

	DBG("client %p", client);

	adapter = bluetooth_client_get_default_adapter(client);
	if (adapter == NULL)
		return FALSE;

	adapter_stop_discovery(adapter, NULL);

	g_object_unref(adapter);

	return TRUE;
}

/**
 * bluetooth_client_set_discoverable:
 * @client: a #BluetoothClient object
 * @discoverable: whether the device should be discoverable
 *
 * Sets the default adapter's discoverable status.
 *
 * Return value: Whether setting the state on the default adapter was successful.
 **/
gboolean
bluetooth_client_set_discoverable (BluetoothClient *client,
				   gboolean discoverable)
{
	DBusGProxy *adapter;
	GValue disco = { 0 };
	GValue timeout = { 0 };
	gboolean ret;

	g_return_val_if_fail (BLUETOOTH_IS_CLIENT (client), FALSE);

	DBG("client %p", client);

	adapter = bluetooth_client_get_default_adapter (client);
	if (adapter == NULL)
		return FALSE;

	g_value_init (&disco, G_TYPE_BOOLEAN);
	g_value_init (&timeout, G_TYPE_UINT);

	g_value_set_boolean (&disco, discoverable);
	g_value_set_uint (&timeout, 0);

	ret = dbus_g_proxy_call (adapter, "SetProperty", NULL,
				 G_TYPE_STRING, "Discoverable",
				 G_TYPE_VALUE, &disco,
				 G_TYPE_INVALID, G_TYPE_INVALID);
	if (ret == FALSE)
		goto bail;

	ret = dbus_g_proxy_call (adapter, "SetProperty", NULL,
				 G_TYPE_STRING, "DiscoverableTimeout",
				 G_TYPE_VALUE, &timeout,
				 G_TYPE_INVALID, G_TYPE_INVALID);

bail:
	g_value_unset (&disco);
	g_value_unset (&timeout);

	g_object_unref(adapter);

	return ret;
}

typedef struct {
	BluetoothClientCreateDeviceFunc func;
	gpointer data;
	BluetoothClient *client;
} CreateDeviceData;

static void create_device_callback(DBusGProxy *proxy,
					DBusGProxyCall *call, void *user_data)
{
	CreateDeviceData *devdata = user_data;
	GError *error = NULL;
	char *path = NULL;

	dbus_g_proxy_end_call(proxy, call, &error,
			DBUS_TYPE_G_OBJECT_PATH, &path, G_TYPE_INVALID);

	if (error != NULL)
		path = NULL;

	if (devdata->func)
		devdata->func(devdata->client, path, error, devdata->data);

	if (error != NULL)
		g_error_free (error);

	g_object_unref (devdata->client);
	g_object_unref(proxy);
}

gboolean bluetooth_client_create_device (BluetoothClient *client,
					 const char *address,
					 const char *agent,
					 BluetoothClientCreateDeviceFunc func,
					 gpointer data)
{
	BluetoothClientPrivate *priv = BLUETOOTH_CLIENT_GET_PRIVATE(client);
	CreateDeviceData *devdata;
	DBusGProxy *adapter;
	GtkTreeIter iter;

	g_return_val_if_fail (BLUETOOTH_IS_CLIENT (client), FALSE);
	g_return_val_if_fail (address != NULL, FALSE);

	DBG("client %p", client);

	adapter = bluetooth_client_get_default_adapter(client);
	if (adapter == NULL)
		return FALSE;

	/* Remove the pairing if it already exists, but only for pairings */
	if (agent != NULL &&
	    get_iter_from_address(priv->store, &iter, address, adapter) == TRUE) {
		DBusGProxy *device;
		gboolean paired;
		GError *err = NULL;

		gtk_tree_model_get (GTK_TREE_MODEL(priv->store), &iter,
				    BLUETOOTH_COLUMN_PROXY, &device,
				    BLUETOOTH_COLUMN_PAIRED, &paired, -1);
		if (paired != FALSE &&
		    dbus_g_proxy_call (adapter, "RemoveDevice", &err,
				       DBUS_TYPE_G_OBJECT_PATH, dbus_g_proxy_get_path (device),
				       G_TYPE_INVALID, G_TYPE_INVALID) == FALSE) {
			g_warning ("Failed to remove device '%s': %s", address,
				   err->message);
			g_error_free (err);
		}
		if (device != NULL)
			g_object_unref (device);
	}

	devdata = g_try_new0(CreateDeviceData, 1);
	if (devdata == NULL)
		return FALSE;

	devdata->func = func;
	devdata->data = data;
	devdata->client = g_object_ref (client);

	if (agent != NULL)
		dbus_g_proxy_begin_call_with_timeout(adapter,
						     "CreatePairedDevice", create_device_callback,
						     devdata, g_free, 90 * 1000,
						     G_TYPE_STRING, address,
						     DBUS_TYPE_G_OBJECT_PATH, agent,
						     G_TYPE_STRING, "DisplayYesNo", G_TYPE_INVALID);
	else
		dbus_g_proxy_begin_call_with_timeout(adapter,
						     "CreateDevice", create_device_callback,
						     devdata, g_free, 60 * 1000,
						     G_TYPE_STRING, address, G_TYPE_INVALID);

	return TRUE;
}

gboolean bluetooth_client_set_trusted(BluetoothClient *client,
					const char *device, gboolean trusted)
{
	BluetoothClientPrivate *priv = BLUETOOTH_CLIENT_GET_PRIVATE(client);
	DBusGProxy *proxy;
	GValue value = { 0 };

	g_return_val_if_fail (BLUETOOTH_IS_CLIENT (client), FALSE);
	g_return_val_if_fail (device != NULL, FALSE);

	DBG("client %p", client);

	proxy = dbus_g_proxy_new_from_proxy(priv->manager,
					BLUEZ_DEVICE_INTERFACE, device);
	if (proxy == NULL)
		return FALSE;

	g_value_init(&value, G_TYPE_BOOLEAN);
	g_value_set_boolean(&value, trusted);

	dbus_g_proxy_call(proxy, "SetProperty", NULL,
					G_TYPE_STRING, "Trusted",
					G_TYPE_VALUE, &value,
					G_TYPE_INVALID, G_TYPE_INVALID);

	g_value_unset(&value);

	g_object_unref(proxy);

	return TRUE;
}

typedef struct {
	BluetoothClientConnectFunc func;
	gpointer data;
	BluetoothClient *client;
	/* used for disconnect */
	GList *services;
} ConnectData;

static void connect_callback(DBusGProxy *proxy,
			     DBusGProxyCall *call, void *user_data)
{
	ConnectData *conndata = user_data;
	GError *error = NULL;
	gboolean retval = TRUE;

	dbus_g_proxy_end_call(proxy, call, &error, G_TYPE_INVALID);

	if (error != NULL) {
		retval = FALSE;
		g_error_free(error);
	}

	if (conndata->func)
		conndata->func(conndata->client, retval, conndata->data);

	g_object_unref (conndata->client);
	g_object_unref(proxy);
}

gboolean bluetooth_client_connect_service(BluetoothClient *client,
					  const char *device,
					  BluetoothClientConnectFunc func,
					  gpointer data)
{
	BluetoothClientPrivate *priv = BLUETOOTH_CLIENT_GET_PRIVATE(client);
	ConnectData *conndata;
	DBusGProxy *proxy;
	GHashTable *table;
	GtkTreeIter iter;
	const char *iface_name;
	guint i;

	g_return_val_if_fail (BLUETOOTH_IS_CLIENT (client), FALSE);
	g_return_val_if_fail (device != NULL, FALSE);

	DBG("client %p", client);

	if (get_iter_from_path (priv->store, &iter, device) == FALSE)
		return FALSE;

	gtk_tree_model_get(GTK_TREE_MODEL (priv->store), &iter,
			   BLUETOOTH_COLUMN_SERVICES, &table,
			   -1);
	if (table == NULL)
		return FALSE;

	conndata = g_new0 (ConnectData, 1);

	iface_name = NULL;
	for (i = 0; i < G_N_ELEMENTS (connectable_interfaces); i++) {
		if (g_hash_table_lookup_extended (table, connectable_interfaces[i], NULL, NULL) != FALSE) {
			iface_name = connectable_interfaces[i];
			break;
		}
	}
	g_hash_table_unref (table);

	if (iface_name == NULL) {
		g_printerr("No supported services on the '%s' device\n", device);
		g_free (conndata);
		return FALSE;
	}

	proxy = dbus_g_proxy_new_from_proxy(priv->manager,
						iface_name, device);
	if (proxy == NULL) {
		g_free (conndata);
		return FALSE;
	}

	conndata->func = func;
	conndata->data = data;
	conndata->client = g_object_ref (client);

	dbus_g_proxy_begin_call(proxy, "Connect",
				connect_callback, conndata, g_free,
				G_TYPE_INVALID);

	return TRUE;
}

static void disconnect_callback(DBusGProxy *proxy,
				DBusGProxyCall *call,
				void *user_data)
{
	ConnectData *conndata = user_data;

	dbus_g_proxy_end_call(proxy, call, NULL, G_TYPE_INVALID);

	if (conndata->services != NULL) {
		DBusGProxy *service;

		service = dbus_g_proxy_new_from_proxy (proxy,
						       conndata->services->data, NULL);

		conndata->services = g_list_remove (conndata->services, conndata->services->data);

		dbus_g_proxy_begin_call(service, "Disconnect",
					disconnect_callback, conndata, NULL,
					G_TYPE_INVALID);

		g_object_unref (proxy);

		return;
	}

	if (conndata->func)
		conndata->func(conndata->client, TRUE, conndata->data);

	g_object_unref (conndata->client);
	g_object_unref(proxy);
	g_free (conndata);
}

static int
service_to_index (const char *service)
{
	guint i;

	g_return_val_if_fail (service != NULL, -1);

	for (i = 0; i < G_N_ELEMENTS (connectable_interfaces); i++) {
		if (g_str_equal (connectable_interfaces[i], service) != FALSE)
			return i;
	}
	for (i = 0; i < G_N_ELEMENTS (detectable_interfaces); i++) {
		if (g_str_equal (detectable_interfaces[i], service) != FALSE)
			return i + G_N_ELEMENTS (connectable_interfaces);
	}

	g_assert_not_reached ();

	return -1;
}

static int
rev_sort_services (const char *servicea, const char *serviceb)
{
	int a, b;

	a = service_to_index (servicea);
	b = service_to_index (serviceb);

	if (a < b)
		return 1;
	if (a > b)
		return -1;
	return 0;
}

gboolean bluetooth_client_disconnect_service (BluetoothClient *client,
					      const char *device,
					      BluetoothClientConnectFunc func,
					      gpointer data)
{
	BluetoothClientPrivate *priv = BLUETOOTH_CLIENT_GET_PRIVATE(client);
	ConnectData *conndata;
	DBusGProxy *proxy;
	GHashTable *table;
	GtkTreeIter iter;

	g_return_val_if_fail (BLUETOOTH_IS_CLIENT (client), FALSE);
	g_return_val_if_fail (device != NULL, FALSE);

	DBG("client %p", client);

	if (get_iter_from_path (priv->store, &iter, device) == FALSE)
		return FALSE;

	gtk_tree_model_get(GTK_TREE_MODEL (priv->store), &iter,
			   BLUETOOTH_COLUMN_PROXY, &proxy,
			   BLUETOOTH_COLUMN_SERVICES, &table,
			   -1);
	if (proxy == NULL) {
		if (table != NULL)
			g_hash_table_unref (table);
		return FALSE;
	}

	conndata = g_new0 (ConnectData, 1);

	conndata->func = func;
	conndata->data = data;
	conndata->client = g_object_ref (client);

	if (table == NULL) {
		dbus_g_proxy_begin_call(proxy, "Disconnect",
					disconnect_callback, conndata, NULL,
					G_TYPE_INVALID);
	} else {
		DBusGProxy *service;

		conndata->services = g_hash_table_get_keys (table);
		g_hash_table_unref (table);
		conndata->services = g_list_sort (conndata->services, (GCompareFunc) rev_sort_services);

		service = dbus_g_proxy_new_from_proxy (priv->manager,
						       conndata->services->data, device);

		conndata->services = g_list_remove (conndata->services, conndata->services->data);

		dbus_g_proxy_begin_call(service, "Disconnect",
					disconnect_callback, conndata, NULL,
					G_TYPE_INVALID);
	}

	return TRUE;
}

#define BOOL_STR(x) (x ? "True" : "False")

static void
services_foreach (const char *service, gpointer _value, GString *str)
{
	GEnumClass *eclass;
	GEnumValue *ev;
	BluetoothStatus status = GPOINTER_TO_INT (_value);

	eclass = g_type_class_ref (BLUETOOTH_TYPE_STATUS);
	ev = g_enum_get_value (eclass, status);
	if (ev == NULL)
		g_warning ("Unknown status value %d", status);

	g_string_append_printf (str, "%s (%s) ", service, ev ? ev->value_nick : "unknown");
	g_type_class_unref (eclass);
}

void
bluetooth_client_dump_device (GtkTreeModel *model,
			      GtkTreeIter *iter,
			      gboolean recurse)
{
	DBusGProxy *proxy;
	char *address, *alias, *name, *icon, **uuids;
	gboolean is_default, paired, trusted, connected, discoverable, discovering, powered, is_adapter;
	GHashTable *services;
	GtkTreeIter parent;
	guint type;

	gtk_tree_model_get (model, iter,
			    BLUETOOTH_COLUMN_ADDRESS, &address,
			    BLUETOOTH_COLUMN_ALIAS, &alias,
			    BLUETOOTH_COLUMN_NAME, &name,
			    BLUETOOTH_COLUMN_TYPE, &type,
			    BLUETOOTH_COLUMN_ICON, &icon,
			    BLUETOOTH_COLUMN_DEFAULT, &is_default,
			    BLUETOOTH_COLUMN_PAIRED, &paired,
			    BLUETOOTH_COLUMN_TRUSTED, &trusted,
			    BLUETOOTH_COLUMN_CONNECTED, &connected,
			    BLUETOOTH_COLUMN_DISCOVERABLE, &discoverable,
			    BLUETOOTH_COLUMN_DISCOVERING, &discovering,
			    BLUETOOTH_COLUMN_POWERED, &powered,
			    BLUETOOTH_COLUMN_SERVICES, &services,
			    BLUETOOTH_COLUMN_UUIDS, &uuids,
			    BLUETOOTH_COLUMN_PROXY, &proxy,
			    -1);
	is_adapter = !gtk_tree_model_iter_parent (model, &parent, iter);

	if (is_adapter != FALSE) {
		/* Adapter */
		g_print ("Adapter: %s (%s)\n", name, address);
		if (is_default)
			g_print ("\tDefault adapter\n");
		g_print ("\tDiscoverable: %s\n", BOOL_STR (discoverable));
		if (discovering)
			g_print ("\tDiscovery in progress\n");
		g_print ("\t%s\n", powered ? "Is powered" : "Is not powered");
	} else {
		/* Device */
		g_print ("Device: %s (%s)\n", alias, address);
		g_print ("\tD-Bus Path: %s\n", proxy ? dbus_g_proxy_get_path (proxy) : "(none)");
		g_print ("\tType: %s Icon: %s\n", bluetooth_type_to_string (type), icon);
		g_print ("\tPaired: %s Trusted: %s Connected: %s\n", BOOL_STR(paired), BOOL_STR(trusted), BOOL_STR(connected));
		if (services != NULL) {
			GString *str;

			str = g_string_new (NULL);
			g_hash_table_foreach (services, (GHFunc) services_foreach, str);
			g_print ("\tServices: %s\n", str->str);
			g_string_free (str, TRUE);
		}
		if (uuids != NULL) {
			guint i;
			g_print ("\tUUIDs: ");
			for (i = 0; uuids[i] != NULL; i++)
				g_print ("%s ", uuids[i]);
			g_print ("\n");
		}
	}
	g_print ("\n");

	g_free (alias);
	g_free (address);
	g_free (icon);
	if (proxy != NULL)
		g_object_unref (proxy);
	if (services != NULL)
		g_hash_table_unref (services);
	g_strfreev (uuids);

	if (recurse == FALSE)
		return;

	if (is_adapter != FALSE) {
		GtkTreeIter child;

		if (gtk_tree_model_iter_children (model, &child, iter) == FALSE)
			return;
		bluetooth_client_dump_device (model, &child, FALSE);
		while (gtk_tree_model_iter_next (model, &child))
			bluetooth_client_dump_device (model, &child, FALSE);
	}

}

