/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>

#include <locale.h>

#include <glib.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "mdm-settings.h"

#define MDM_DBUS_NAME "org.mate.DisplayManager"

static MdmSettings     *settings      = NULL;

static gboolean
acquire_name_on_proxy (DBusGProxy *bus_proxy)
{
        GError     *error;
        guint       result;
        gboolean    res;
        gboolean    ret;

        ret = FALSE;

        if (bus_proxy == NULL) {
                goto out;
        }

        error = NULL;
        res = dbus_g_proxy_call (bus_proxy,
                                 "RequestName",
                                 &error,
                                 G_TYPE_STRING, MDM_DBUS_NAME,
                                 G_TYPE_UINT, 0,
                                 G_TYPE_INVALID,
                                 G_TYPE_UINT, &result,
                                 G_TYPE_INVALID);
        if (! res) {
                if (error != NULL) {
                        g_warning ("Failed to acquire %s: %s", MDM_DBUS_NAME, error->message);
                        g_error_free (error);
                } else {
                        g_warning ("Failed to acquire %s", MDM_DBUS_NAME);
                }
                goto out;
        }

        if (result != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
                if (error != NULL) {
                        g_warning ("Failed to acquire %s: %s", MDM_DBUS_NAME, error->message);
                        g_error_free (error);
                } else {
                        g_warning ("Failed to acquire %s", MDM_DBUS_NAME);
                }
                goto out;
        }

        ret = TRUE;

 out:
        return ret;
}

static DBusGProxy *
get_bus_proxy (DBusGConnection *connection)
{
        DBusGProxy *bus_proxy;

        bus_proxy = dbus_g_proxy_new_for_name (connection,
                                               DBUS_SERVICE_DBUS,
                                               DBUS_PATH_DBUS,
                                               DBUS_INTERFACE_DBUS);
        return bus_proxy;
}

static DBusGConnection *
get_system_bus (void)
{
        GError          *error;
        DBusGConnection *bus;
        DBusConnection  *connection;

        error = NULL;
        bus = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
        if (bus == NULL) {
                g_warning ("Couldn't connect to system bus: %s",
                           error->message);
                g_error_free (error);
                goto out;
        }

        connection = dbus_g_connection_get_connection (bus);
        dbus_connection_set_exit_on_disconnect (connection, FALSE);

 out:
        return bus;
}

int
main (int argc, char **argv)
{
        GMainLoop          *main_loop;
        DBusGConnection    *connection;
        DBusGProxy         *bus_proxy;

        g_type_init ();

        connection = get_system_bus ();
        if (connection == NULL) {
                goto out;
        }

        bus_proxy = get_bus_proxy (connection);
        if (bus_proxy == NULL) {
                g_warning ("Could not construct bus_proxy object; bailing out");
                goto out;
        }

        if (! acquire_name_on_proxy (bus_proxy) ) {
                g_warning ("Could not acquire name; bailing out");
                goto out;
        }

        settings = mdm_settings_new ();
        if (settings == NULL) {
                g_warning ("Unable to initialize settings");
                exit (1);
        }

        main_loop = g_main_loop_new (NULL, FALSE);
        g_main_loop_run (main_loop);

        g_main_loop_unref (main_loop);

        g_object_unref (settings);

 out:
        return 0;
}
