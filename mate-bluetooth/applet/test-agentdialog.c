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

#include "agent.c"
#include "notify.h"

static void activate_callback(GObject *widget, gpointer user_data)
{
	show_agents();
}

int main(int argc, char *argv[])
{
	GtkStatusIcon *statusicon;
	DBusGConnection *conn;
	DBusGProxy *adapter, *device;
	GError *error = NULL;

	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

	gtk_init(&argc, &argv);

	gtk_window_set_default_icon_name("bluetooth");

	conn = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
	if (error != NULL) {
		g_printerr("Connecting to system bus failed: %s\n",
							error->message);
		g_error_free(error);
		return 1;
	}

	set_icon (TRUE);
	statusicon = init_notification();

	g_signal_connect(statusicon, "activate",
				G_CALLBACK(activate_callback), NULL);

	setup_agents();

	adapter = dbus_g_proxy_new_for_name(conn, "org.bluez",
						"/hci0", "org.bluez.Adapter");

	device = dbus_g_proxy_new_from_proxy(adapter,
			"/hci0/dev_11_22_33_44_55_66", "org.bluez.Device");

	//display_dialog(adapter, device, "Test (00:11:22:33:44:55)", "123456", 0, NULL);
	pin_dialog(adapter, device, "Test", "'Test' (00:11:22:33:44:55)", FALSE, NULL);
	confirm_dialog(adapter, device, "Test", "'Test' (00:11:22:33:44:55)", "123456", NULL);
	auth_dialog(adapter, device, "Test", "'Test' (00:11:22:33:44:55)", "UUID", NULL);

	gtk_main();

	g_object_unref(device);
	g_object_unref(adapter);

	cleanup_notification();

	cleanup_agents();

	dbus_g_connection_unref(conn);

	return 0;
}
