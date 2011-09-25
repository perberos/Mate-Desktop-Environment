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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <locale.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "mdm-xerrors.h"
#include "mdm-signal-handler.h"
#include "mdm-log.h"
#include "mdm-common.h"
#include "mdm-simple-slave.h"
#include "mdm-settings.h"
#include "mdm-settings-direct.h"
#include "mdm-settings-keys.h"
#include "mdm-settings-client.h"

static MdmSettings     *settings        = NULL;
static int              mdm_return_code = 0;

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

static gboolean
signal_cb (int      signo,
           gpointer data)
{
        int ret;

        g_debug ("Got callback for signal %d", signo);

        ret = TRUE;

        switch (signo) {
        case SIGSEGV:
        case SIGBUS:
        case SIGILL:
        case SIGABRT:
                g_debug ("Caught signal %d.", signo);

                ret = FALSE;
                break;

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
                /* we get this from xorg - can't use for anything else */
                ret = TRUE;

                break;

        case SIGUSR2:
                g_debug ("Got USR2 signal");
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

static void
on_slave_stopped (MdmSlave   *slave,
                  GMainLoop  *main_loop)
{
        g_debug ("slave finished");
        mdm_return_code = 0;
        g_main_loop_quit (main_loop);
}

static gboolean
is_debug_set (void)
{
        gboolean debug = FALSE;

        /* enable debugging for unstable builds */
        if (mdm_is_version_unstable ()) {
                return TRUE;
        }

        mdm_settings_direct_get_boolean (MDM_KEY_DEBUG, &debug);
        return debug;
}

int
main (int    argc,
      char **argv)
{
        GMainLoop        *main_loop;
        GOptionContext   *context;
        DBusGConnection  *connection;
        MdmSlave         *slave;
        static char      *display_id = NULL;
        MdmSignalHandler *signal_handler;
        static GOptionEntry entries []   = {
                { "display-id", 0, 0, G_OPTION_ARG_STRING, &display_id, N_("Display ID"), N_("ID") },
                { NULL }
        };

        bindtextdomain (GETTEXT_PACKAGE, MATELOCALEDIR);
        textdomain (GETTEXT_PACKAGE);
        setlocale (LC_ALL, "");

        mdm_set_fatal_warnings_if_unstable ();

        g_type_init ();

        context = g_option_context_new (_("MATE Display Manager Slave"));
        g_option_context_add_main_entries (context, entries, NULL);

        g_option_context_parse (context, &argc, &argv, NULL);
        g_option_context_free (context);

        /* For debugging */
        /*sleep (10);*/

        connection = get_system_bus ();
        if (connection == NULL) {
                goto out;
        }

        mdm_xerrors_init ();
        mdm_log_init ();

        settings = mdm_settings_new ();
        if (settings == NULL) {
                g_warning ("Unable to initialize settings");
                goto out;
        }

        if (! mdm_settings_direct_init (settings, DATADIR "/mdm/mdm.schemas", "/")) {
                g_warning ("Unable to initialize settings");
                goto out;
        }

        mdm_log_set_debug (is_debug_set ());

        if (display_id == NULL) {
                g_critical ("No display ID set");
                exit (1);
        }

        if (! mdm_settings_client_init (DATADIR "/mdm/mdm.schemas", "/")) {
                g_critical ("Unable to initialize settings client");
                exit (1);
        }

        main_loop = g_main_loop_new (NULL, FALSE);

        signal_handler = mdm_signal_handler_new ();
        mdm_signal_handler_set_fatal_func (signal_handler,
                                           (GDestroyNotify)g_main_loop_quit,
                                           main_loop);
        mdm_signal_handler_add (signal_handler, SIGTERM, signal_cb, NULL);
        mdm_signal_handler_add (signal_handler, SIGINT, signal_cb, NULL);
        mdm_signal_handler_add (signal_handler, SIGILL, signal_cb, NULL);
        mdm_signal_handler_add (signal_handler, SIGBUS, signal_cb, NULL);
        mdm_signal_handler_add (signal_handler, SIGFPE, signal_cb, NULL);
        mdm_signal_handler_add (signal_handler, SIGHUP, signal_cb, NULL);
        mdm_signal_handler_add (signal_handler, SIGSEGV, signal_cb, NULL);
        mdm_signal_handler_add (signal_handler, SIGABRT, signal_cb, NULL);
        mdm_signal_handler_add (signal_handler, SIGUSR1, signal_cb, NULL);
        mdm_signal_handler_add (signal_handler, SIGUSR2, signal_cb, NULL);

        slave = mdm_simple_slave_new (display_id);
        if (slave == NULL) {
                goto out;
        }
        g_signal_connect (slave,
                          "stopped",
                          G_CALLBACK (on_slave_stopped),
                          main_loop);
        mdm_slave_start (slave);

        g_main_loop_run (main_loop);

        if (slave != NULL) {
                g_object_unref (slave);
        }

        if (signal_handler != NULL) {
                g_object_unref (signal_handler);
        }

        g_main_loop_unref (main_loop);

 out:

        g_debug ("Slave finished");

        return mdm_return_code;
}
