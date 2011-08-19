/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Red Hat, Inc.
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
#include <string.h>
#include <unistd.h>

#include <glib.h>
#include <dbus/dbus-glib.h>

#define SM_DBUS_NAME      "org.mate.SessionManager"
#define SM_DBUS_PATH      "/org/mate/SessionManager"
#define SM_DBUS_INTERFACE "org.mate.SessionManager"

#define SM_CLIENT_DBUS_INTERFACE "org.mate.SessionManager.ClientPrivate"

static DBusGConnection *bus_connection = NULL;
static DBusGProxy      *sm_proxy = NULL;
static char            *client_id = NULL;
static DBusGProxy      *client_proxy = NULL;
static GMainLoop       *main_loop = NULL;

static gboolean
session_manager_connect (void)
{

        if (bus_connection == NULL) {
                GError *error;

                error = NULL;
                bus_connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
                if (bus_connection == NULL) {
                        g_message ("Failed to connect to the session bus: %s",
                                   error->message);
                        g_error_free (error);
                        exit (1);
                }
        }

        sm_proxy = dbus_g_proxy_new_for_name (bus_connection,
                                              SM_DBUS_NAME,
                                              SM_DBUS_PATH,
                                              SM_DBUS_INTERFACE);
        return (sm_proxy != NULL);
}

static void
on_client_query_end_session (DBusGProxy     *proxy,
                             guint           flags,
                             gpointer        data)
{
        GError     *error;
        gboolean    is_ok;
        gboolean    res;
        const char *reason;

        is_ok = FALSE;
        reason = "Unsaved files";

        g_debug ("Got query end session signal flags=%u", flags);

        error = NULL;
        res = dbus_g_proxy_call (proxy,
                                 "EndSessionResponse",
                                 &error,
                                 G_TYPE_BOOLEAN, is_ok,
                                 G_TYPE_STRING, reason,
                                 G_TYPE_INVALID,
                                 G_TYPE_INVALID);
}

static void
on_client_end_session (DBusGProxy     *proxy,
                       guint           flags,
                       gpointer        data)
{
        g_debug ("Got end session signal flags=%u", flags);
}

static void
on_client_cancel_end_session (DBusGProxy     *proxy,
                              gpointer        data)
{
        g_debug ("Got end session cancelled signal");
}

static void
on_client_stop (DBusGProxy     *proxy,
                gpointer        data)
{
        g_debug ("Got client stop signal");
        g_main_loop_quit (main_loop);
}

static gboolean
register_client (void)
{
        GError     *error;
        gboolean    res;
        const char *startup_id;
        const char *app_id;

        startup_id = g_getenv ("DESKTOP_AUTOSTART_ID");
        app_id = "gedit";

        error = NULL;
        res = dbus_g_proxy_call (sm_proxy,
                                 "RegisterClient",
                                 &error,
                                 G_TYPE_STRING, app_id,
                                 G_TYPE_STRING, startup_id,
                                 G_TYPE_INVALID,
                                 DBUS_TYPE_G_OBJECT_PATH, &client_id,
                                 G_TYPE_INVALID);
        if (! res) {
                g_warning ("Failed to register client: %s", error->message);
                g_error_free (error);
                return FALSE;
        }

        g_debug ("Client registered with session manager: %s", client_id);
        client_proxy = dbus_g_proxy_new_for_name (bus_connection,
                                                  SM_DBUS_NAME,
                                                  client_id,
                                                  SM_CLIENT_DBUS_INTERFACE);
        dbus_g_proxy_add_signal (client_proxy,
                                 "QueryEndSession",
                                 G_TYPE_UINT,
                                 G_TYPE_INVALID);
        dbus_g_proxy_add_signal (client_proxy,
                                 "EndSession",
                                 G_TYPE_UINT,
                                 G_TYPE_INVALID);
        dbus_g_proxy_add_signal (client_proxy,
                                 "CancelEndSession",
                                 G_TYPE_UINT,
                                 G_TYPE_INVALID);
        dbus_g_proxy_add_signal (client_proxy,
                                 "Stop",
                                 G_TYPE_INVALID);
        dbus_g_proxy_connect_signal (client_proxy,
                                     "QueryEndSession",
                                     G_CALLBACK (on_client_query_end_session),
                                     NULL,
                                     NULL);
        dbus_g_proxy_connect_signal (client_proxy,
                                     "EndSession",
                                     G_CALLBACK (on_client_end_session),
                                     NULL,
                                     NULL);
        dbus_g_proxy_connect_signal (client_proxy,
                                     "CancelEndSession",
                                     G_CALLBACK (on_client_cancel_end_session),
                                     NULL,
                                     NULL);
        dbus_g_proxy_connect_signal (client_proxy,
                                     "Stop",
                                     G_CALLBACK (on_client_stop),
                                     NULL,
                                     NULL);

        return TRUE;
}

static gboolean
session_manager_disconnect (void)
{
        if (sm_proxy != NULL) {
                g_object_unref (sm_proxy);
                sm_proxy = NULL;
        }

        return TRUE;
}

static gboolean
unregister_client (void)
{
        GError  *error;
        gboolean res;

        error = NULL;
        res = dbus_g_proxy_call (sm_proxy,
                                 "UnregisterClient",
                                 &error,
                                 DBUS_TYPE_G_OBJECT_PATH, client_id,
                                 G_TYPE_INVALID,
                                 G_TYPE_INVALID);
        if (! res) {
                g_warning ("Failed to unregister client: %s", error->message);
                g_error_free (error);
                return FALSE;
        }

        g_free (client_id);
        client_id = NULL;

        return TRUE;
}

static gboolean
quit_test (gpointer data)
{
        g_main_loop_quit (main_loop);
        return FALSE;
}

int
main (int   argc,
      char *argv[])
{
        gboolean res;

        g_log_set_always_fatal (G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING);

        g_type_init ();

        res = session_manager_connect ();
        if (! res) {
                g_warning ("Unable to connect to session manager");
                exit (1);
        }

        res = register_client ();
        if (! res) {
                g_warning ("Unable to register client with session manager");
        }

        main_loop = g_main_loop_new (NULL, FALSE);

        g_timeout_add_seconds (30, quit_test, NULL);

        g_main_loop_run (main_loop);
        g_main_loop_unref (main_loop);

        unregister_client ();
        session_manager_disconnect ();

        return 0;
}
