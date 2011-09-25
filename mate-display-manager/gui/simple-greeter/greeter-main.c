/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdlib.h>
#include <locale.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <mateconf/mateconf-client.h>

#include <dbus/dbus-glib.h>

#include "mdm-log.h"
#include "mdm-common.h"
#include "mdm-signal-handler.h"
#include "mdm-settings-client.h"
#include "mdm-settings-keys.h"
#include "mdm-profile.h"

#include "mdm-greeter-session.h"

#define SM_DBUS_NAME      "org.mate.SessionManager"
#define SM_DBUS_PATH      "/org/mate/SessionManager"
#define SM_DBUS_INTERFACE "org.mate.SessionManager"

#define SM_CLIENT_DBUS_INTERFACE "org.mate.SessionManager.ClientPrivate"

static DBusGConnection *bus_connection = NULL;
static DBusGProxy      *sm_proxy = NULL;
static char            *client_id = NULL;
static DBusGProxy      *client_proxy = NULL;

static gboolean
is_debug_set (void)
{
        gboolean debug = FALSE;

        /* enable debugging for unstable builds */
        if (mdm_is_version_unstable ()) {
                return TRUE;
        }

        mdm_settings_client_get_boolean (MDM_KEY_DEBUG, &debug);
        return debug;
}


static gboolean
signal_cb (int      signo,
           gpointer data)
{
        int ret;

        g_debug ("Got callback for signal %d", signo);

        ret = TRUE;

        switch (signo) {
        case SIGFPE:
        case SIGPIPE:
                /* let the fatal signals interrupt us */
                g_debug ("Caught signal %d, shutting down abnormally.", signo);
                ret = FALSE;

                break;

        case SIGINT:
        case SIGTERM:
                /* let the fatal signals interrupt us */
                g_debug ("Caught signal %d, shutting down normally.", signo);
                ret = FALSE;

                break;

        case SIGHUP:
                g_debug ("Got HUP signal");
                /* FIXME:
                 * Reread config stuff like system config files, VPN service files, etc
                 */
                ret = TRUE;

                break;

        case SIGUSR1:
                g_debug ("Got USR1 signal");
                /* FIXME:
                 * Play with log levels or something
                 */
                ret = TRUE;

                mdm_log_toggle_debug ();

                break;

        default:
                g_debug ("Caught unhandled signal %d", signo);
                ret = TRUE;

                break;
        }

        return ret;
}

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
stop_cb (gpointer data)
{
        gtk_main_quit ();
}

static gboolean
end_session_response (gboolean is_okay, const gchar *reason)
{
        gboolean ret;
        GError *error = NULL;

        ret = dbus_g_proxy_call (client_proxy, "EndSessionResponse",
                                 &error,
                                 G_TYPE_BOOLEAN, is_okay,
                                 G_TYPE_STRING, reason,
                                 G_TYPE_INVALID,
                                 G_TYPE_INVALID);

        if (!ret) {
                g_warning ("Failed to send session response %s", error->message);
                g_error_free (error);
        }

        return ret;
}

static void
query_end_session_cb (guint flags, gpointer data)
{
        end_session_response (TRUE, NULL);
}

static void
end_session_cb (guint flags, gpointer data)
{
        end_session_response (TRUE, NULL);
        gtk_main_quit ();
}

static gboolean
register_client (void)
{
        GError     *error;
        gboolean    res;
        const char *startup_id;
        const char *app_id;

        startup_id = g_getenv ("DESKTOP_AUTOSTART_ID");
        app_id = "mdm-simple-greeter.desktop";

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

        dbus_g_proxy_add_signal (client_proxy, "Stop", G_TYPE_INVALID);
        dbus_g_proxy_connect_signal (client_proxy, "Stop",
                                     G_CALLBACK (stop_cb), NULL, NULL);

        dbus_g_proxy_add_signal (client_proxy, "QueryEndSession", G_TYPE_UINT, G_TYPE_INVALID);
        dbus_g_proxy_connect_signal (client_proxy, "QueryEndSession",
                                     G_CALLBACK (query_end_session_cb), NULL, NULL);

        dbus_g_proxy_add_signal (client_proxy, "EndSession", G_TYPE_UINT, G_TYPE_INVALID);
        dbus_g_proxy_connect_signal (client_proxy, "EndSession",
                                     G_CALLBACK (end_session_cb), NULL, NULL);

        g_unsetenv ("DESKTOP_AUTOSTART_ID");

        return TRUE;
}

int
main (int argc, char *argv[])
{
        GError            *error;
        MdmGreeterSession *session;
        gboolean           res;
        MdmSignalHandler  *signal_handler;

        bindtextdomain (GETTEXT_PACKAGE, MATELOCALEDIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);

        setlocale (LC_ALL, "");

        mdm_set_fatal_warnings_if_unstable ();

        g_type_init ();

        mdm_profile_start ("Initializing settings client");
        if (! mdm_settings_client_init (DATADIR "/mdm/mdm.schemas", "/")) {
                g_critical ("Unable to initialize settings client");
                exit (1);
        }
        mdm_profile_end ("Initializing settings client");

        g_debug ("Greeter session pid=%d display=%s xauthority=%s",
                 (int)getpid (),
                 g_getenv ("DISPLAY"),
                 g_getenv ("XAUTHORITY"));

        /* FIXME: For testing to make it easier to attach gdb */
        /*sleep (15);*/

        mdm_log_init ();
        mdm_log_set_debug (is_debug_set ());

        gtk_init (&argc, &argv);

        signal_handler = mdm_signal_handler_new ();
        mdm_signal_handler_add_fatal (signal_handler);
        mdm_signal_handler_add (signal_handler, SIGTERM, signal_cb, NULL);
        mdm_signal_handler_add (signal_handler, SIGINT, signal_cb, NULL);
        mdm_signal_handler_add (signal_handler, SIGFPE, signal_cb, NULL);
        mdm_signal_handler_add (signal_handler, SIGHUP, signal_cb, NULL);
        mdm_signal_handler_add (signal_handler, SIGUSR1, signal_cb, NULL);

        mdm_profile_start ("Creating new greeter session");
        session = mdm_greeter_session_new ();
        if (session == NULL) {
                g_critical ("Unable to create greeter session");
                exit (1);
        }
        mdm_profile_end ("Creating new greeter session");

        error = NULL;
        res = mdm_greeter_session_start (session, &error);
        if (! res) {
                g_warning ("Unable to start greeter session: %s", error->message);
                g_error_free (error);
                exit (1);
        }

        res = session_manager_connect ();
        if (! res) {
                g_warning ("Unable to connect to session manager");
                exit (1);
        }

        res = register_client ();
        if (! res) {
                g_warning ("Unable to register client with session manager");
        }

        gtk_main ();

        if (session != NULL) {
                g_object_unref (session);
        }

        return 0;
}
