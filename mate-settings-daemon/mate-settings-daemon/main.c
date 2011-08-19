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
#include <unistd.h>
#include <libintl.h>
#include <errno.h>
#include <locale.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>

#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "mate-settings-manager.h"
#include "mate-settings-profile.h"

#define GSD_DBUS_NAME         "org.mate.SettingsDaemon"

#define MATE_SESSION_DBUS_NAME      "org.mate.SessionManager"
#define MATE_SESSION_DBUS_OBJECT    "/org/mate/SessionManager"
#define MATE_SESSION_DBUS_INTERFACE "org.mate.SessionManager"

static char      *mateconf_prefix = NULL;
static gboolean   no_daemon    = FALSE;
static gboolean   debug        = FALSE;
static gboolean   do_timed_exit = FALSE;
static int        daemon_pipe_fds[2];
static int        term_signal_pipe_fds[2];

static GOptionEntry entries[] = {
        {"debug", 0, 0, G_OPTION_ARG_NONE, &debug, N_("Enable debugging code"), NULL },
        {"no-daemon", 0, 0, G_OPTION_ARG_NONE, &no_daemon, N_("Don't become a daemon"), NULL },
        {"mateconf-prefix", 0, 0, G_OPTION_ARG_STRING, &mateconf_prefix, N_("MateConf prefix from which to load plugin settings"), NULL},
        { "timed-exit", 0, 0, G_OPTION_ARG_NONE, &do_timed_exit, N_("Exit after a time (for debugging)"), NULL },
        {NULL}
};

static gboolean
timed_exit_cb (void)
{
        gtk_main_quit ();
        return FALSE;
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

static gboolean
acquire_name_on_proxy (DBusGProxy *bus_proxy)
{
        GError     *error;
        guint       result;
        gboolean    res;
        gboolean    ret;

        ret = FALSE;

        error = NULL;
        res = dbus_g_proxy_call (bus_proxy,
                                 "RequestName",
                                 &error,
                                 G_TYPE_STRING, GSD_DBUS_NAME,
                                 G_TYPE_UINT, 0,
                                 G_TYPE_INVALID,
                                 G_TYPE_UINT, &result,
                                 G_TYPE_INVALID);
        if (! res) {
                if (error != NULL) {
                        g_warning ("Failed to acquire %s: %s", GSD_DBUS_NAME, error->message);
                        g_error_free (error);
                } else {
                        g_warning ("Failed to acquire %s", GSD_DBUS_NAME);
                }
                goto out;
        }

        if (result != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
                if (error != NULL) {
                        g_warning ("Failed to acquire %s: %s", GSD_DBUS_NAME, error->message);
                        g_error_free (error);
                } else {
                        g_warning ("Failed to acquire %s", GSD_DBUS_NAME);
                }
                goto out;
        }

        ret = TRUE;

 out:
        return ret;
}

static DBusHandlerResult
bus_message_handler (DBusConnection *connection,
                     DBusMessage    *message,
                     void           *user_data)
{
        if (dbus_message_is_signal (message,
                                    DBUS_INTERFACE_LOCAL,
                                    "Disconnected")) {
                gtk_main_quit ();
                return DBUS_HANDLER_RESULT_HANDLED;
        }

        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static DBusGConnection *
get_session_bus (void)
{
        GError          *error;
        DBusGConnection *bus;
        DBusConnection  *connection;

        error = NULL;
        bus = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
        if (bus == NULL) {
                g_warning ("Couldn't connect to session bus: %s",
                           error->message);
                g_error_free (error);
                goto out;
        }

        connection = dbus_g_connection_get_connection (bus);
        dbus_connection_add_filter (connection,
                                    (DBusHandleMessageFunction)
                                    bus_message_handler,
                                    NULL, NULL);

        dbus_connection_set_exit_on_disconnect (connection, FALSE);

 out:
        return bus;
}

static gboolean
bus_register (DBusGConnection *bus)
{
        DBusGProxy      *bus_proxy;
        gboolean         ret;

        mate_settings_profile_start (NULL);

        ret = FALSE;

        bus_proxy = get_bus_proxy (bus);

        if (bus_proxy == NULL) {
                g_warning ("Could not construct bus_proxy object");
                goto out;
        }

        ret = acquire_name_on_proxy (bus_proxy);
        g_object_unref (bus_proxy);

        if (!ret) {
                g_warning ("Could not acquire name");
                goto out;
        }

        g_debug ("Successfully connected to D-Bus");

 out:
        mate_settings_profile_end (NULL);

        return ret;
}

static void
on_session_over (DBusGProxy *proxy, MateSettingsManager *manager)
{
        mate_settings_manager_stop (manager);
        gtk_main_quit ();
}

static void
on_term_signal (int signal)
{
        /* Wake up main loop to tell it to shutdown */
        close (term_signal_pipe_fds[1]);
        term_signal_pipe_fds[1] = -1;
}

static gboolean
on_term_signal_pipe_closed (GIOChannel *source,
                            GIOCondition condition,
                            gpointer data)
{
        MateSettingsManager *manager;

        manager = MATE_SETTINGS_MANAGER (data);

        term_signal_pipe_fds[0] = -1;

        /* Got SIGTERM, time to clean up and get out
         */
        gtk_main_quit ();

        return FALSE;
}

static void
watch_for_term_signal (MateSettingsManager *manager)
{
        GIOChannel *channel;

        if (-1 == pipe (term_signal_pipe_fds) ||
            -1 == fcntl (term_signal_pipe_fds[0], F_SETFD, FD_CLOEXEC) ||
            -1 == fcntl (term_signal_pipe_fds[1], F_SETFD, FD_CLOEXEC)) {
                g_error ("Could not create pipe: %s", g_strerror (errno));
                exit (EXIT_FAILURE);
        }

        channel = g_io_channel_unix_new (term_signal_pipe_fds[0]);
        g_io_channel_set_encoding (channel, NULL, NULL);
        g_io_channel_set_buffered (channel, FALSE);
        g_io_add_watch (channel, G_IO_HUP, on_term_signal_pipe_closed, manager);
        g_io_channel_unref (channel);

        signal (SIGTERM, on_term_signal);

}

static void
set_session_over_handler (DBusGConnection *bus, MateSettingsManager *manager)
{
        DBusGProxy *session_proxy;

        g_assert (bus != NULL);

        mate_settings_profile_start (NULL);

        session_proxy =
                 dbus_g_proxy_new_for_name (bus,
                                            MATE_SESSION_DBUS_NAME,
                                            MATE_SESSION_DBUS_OBJECT,
                                            MATE_SESSION_DBUS_INTERFACE);

        dbus_g_object_register_marshaller (
                g_cclosure_marshal_VOID__VOID,
                G_TYPE_NONE,
                G_TYPE_INVALID);

        dbus_g_proxy_add_signal (session_proxy,
                                 "SessionOver",
                                 G_TYPE_INVALID);

        dbus_g_proxy_connect_signal (session_proxy,
                                     "SessionOver",
                                     G_CALLBACK (on_session_over),
                                     manager,
                                     NULL);

        watch_for_term_signal (manager);
        mate_settings_profile_end (NULL);
}

static void
gsd_log_default_handler (const gchar   *log_domain,
                         GLogLevelFlags log_level,
                         const gchar   *message,
                         gpointer       unused_data)
{
        /* filter out DEBUG messages if debug isn't set */
        if ((log_level & G_LOG_LEVEL_MASK) == G_LOG_LEVEL_DEBUG
            && ! debug) {
                return;
        }

        g_log_default_handler (log_domain,
                               log_level,
                               message,
                               unused_data);
}


/* We want the parent process to quit after initializing all plugins,
 * but we have to do all the work in the child process.  We can't
 * initialize in parent and then fork here: that is not clean with
 * X display and DBUS where we would make the connection from one
 * process and continue using it from the other. So, we just make the
 * parent to fork early and wait. */

static void
daemon_start (void)
{
        int child_pid;
        char buf[1];

        if (no_daemon)
                return;

        mate_settings_profile_msg ("forking daemon");

        signal (SIGPIPE, SIG_IGN);
        if (-1 == pipe (daemon_pipe_fds)) {
                g_error ("Could not create pipe: %s", g_strerror (errno));
                exit (EXIT_FAILURE);
        }

        child_pid = fork ();

        switch (child_pid) {
        case -1:
                g_error ("Could not daemonize: %s", g_strerror (errno));
                exit (EXIT_FAILURE);

        case 0:
                /* child */

                close (daemon_pipe_fds[0]);

                return;

         default:
                /* parent */

                close (daemon_pipe_fds[1]);

                /* Wait for child to signal that we are good to go. */
                read (daemon_pipe_fds[0], buf, 1);

                exit (EXIT_SUCCESS);
        }
}

static void
daemon_detach (void)
{
        if (no_daemon)
                return;

        mate_settings_profile_msg ("detaching daemon");

        /* disconnect */
        setsid ();
        close (0);
        close (1);
        open ("/dev/null", O_RDONLY);
        open ("/dev/null", O_WRONLY);

        /* get outta the way */
        chdir ("/");
}

static void
daemon_terminate_parent (void)
{
        if (no_daemon)
                return;

        mate_settings_profile_msg ("terminating parent");

        write (daemon_pipe_fds[1], "1", 1);
        close (daemon_pipe_fds[1]);
}

static void
parse_args (int *argc, char ***argv)
{
        GError *error;
        GOptionContext *context;

        mate_settings_profile_start (NULL);


        context = g_option_context_new (NULL);

        g_option_context_add_main_entries (context, entries, NULL);
        g_option_context_add_group (context, gtk_get_option_group (FALSE));

        error = NULL;
        if (!g_option_context_parse (context, argc, argv, &error)) {
                if (error != NULL) {
                        g_warning ("%s", error->message);
                        g_error_free (error);
                } else {
                        g_warning ("Unable to initialize GTK+");
                }
                exit (EXIT_FAILURE);
        }

        g_option_context_free (context);

        mate_settings_profile_end (NULL);
}

int
main (int argc, char *argv[])
{
        MateSettingsManager *manager;
        DBusGConnection      *bus;
        gboolean              res;
        GError               *error;

        manager = NULL;

        if (!g_thread_supported ()) {
                g_thread_init (NULL);
        }

        mate_settings_profile_start (NULL);

        bindtextdomain (GETTEXT_PACKAGE, MATE_SETTINGS_LOCALEDIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);
        setlocale (LC_ALL, "");

        parse_args (&argc, &argv);

        daemon_start ();

        g_type_init ();

        mate_settings_profile_start ("opening gtk display");
        if (! gtk_init_check (NULL, NULL)) {
                g_warning ("Unable to initialize GTK+");
                daemon_terminate_parent ();
                exit (EXIT_FAILURE);
        }
        mate_settings_profile_end ("opening gtk display");

        daemon_detach ();

        g_log_set_default_handler (gsd_log_default_handler, NULL);

        bus = get_session_bus ();
        if (bus == NULL) {
                g_warning ("Could not get a connection to the bus");
                goto out;
        }

        if (! bus_register (bus)) {
                goto out;
        }

        mate_settings_profile_start ("mate_settings_manager_new");
        manager = mate_settings_manager_new ();
        mate_settings_profile_end ("mate_settings_manager_new");
        if (manager == NULL) {
                g_warning ("Unable to register object");
                goto out;
        }

        set_session_over_handler (bus, manager);

        /* If we aren't started by dbus then load the plugins
           automatically.  Otherwise, wait for an Awake etc. */
        if (g_getenv ("DBUS_STARTER_BUS_TYPE") == NULL) {
                error = NULL;
                if (mateconf_prefix != NULL) {
                        res = mate_settings_manager_start_with_settings_prefix (manager, mateconf_prefix, &error);
                } else {
                        res = mate_settings_manager_start (manager, &error);
                }
                if (! res) {
                        g_warning ("Unable to start: %s", error->message);
                        g_error_free (error);
                        goto out;
                }
        }

        daemon_terminate_parent ();

        if (do_timed_exit) {
                g_timeout_add (1000 * 30, (GSourceFunc) timed_exit_cb, NULL);
        }

        gtk_main ();

 out:
        g_free (mateconf_prefix);

        if (bus != NULL) {
                dbus_g_connection_unref (bus);
        }

        if (manager != NULL) {
                g_object_unref (manager);
        }

        g_debug ("SettingsDaemon finished");
        mate_settings_profile_end (NULL);

        return 0;
}
