/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2005-2008  Marcel Holtmann <marcel@holtmann.org>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "bluetooth-agent.h"
#include "bluetooth-client-glue.h"

static gboolean agent_pincode(DBusGMethodInvocation *context,
					DBusGProxy *device, gpointer user_data)
{
	GHashTable *hash = NULL;
	GValue *value;
	const gchar *address, *name;

	device_get_properties(device, &hash, NULL);

	if (hash != NULL) {
		value = g_hash_table_lookup(hash, "Address");
		address = value ? g_value_get_string(value) : NULL;

		value = g_hash_table_lookup(hash, "Name");
		name = value ? g_value_get_string(value) : NULL;
	} else {
		address = NULL;
		name = NULL;
	}

	printf("address %s name %s\n", address, name);

	dbus_g_method_return(context, "1234");

	return TRUE;
}

static GMainLoop *mainloop = NULL;

static void sig_term(int sig)
{
	g_main_loop_quit(mainloop);
}

int main(int argc, char *argv[])
{
	struct sigaction sa;
	BluetoothAgent *agent;
	DBusGConnection *conn;
	DBusGProxy *proxy;
	GError *error = NULL;

	memset(&sa, 0, sizeof(sa));
	sa.sa_flags = SA_NOCLDSTOP;
	sa.sa_handler = sig_term;
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGINT,  &sa, NULL);

	g_type_init();

	mainloop = g_main_loop_new(NULL, FALSE);

	conn = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
	if (error != NULL) {
		g_printerr("Connecting to system bus failed: %s\n",
							error->message);
		g_main_loop_unref(mainloop);
		g_error_free(error);
		exit(1);
	}

	proxy = dbus_g_proxy_new_for_name(conn, "org.bluez", "/hci0",
							"org.bluez.Adapter");

	agent = bluetooth_agent_new();

	bluetooth_agent_set_pincode_func(agent, agent_pincode, NULL);

	bluetooth_agent_register(agent, proxy);

	g_main_loop_run(mainloop);

	//bluetooth_agent_unregister(agent);

	g_object_unref(agent);

	g_object_unref(proxy);

	dbus_g_connection_unref(conn);

	g_main_loop_unref(mainloop);

	return 0;
}
