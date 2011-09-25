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
#include <pwd.h>
#include <grp.h>
#include <sys/wait.h>
#include <locale.h>
#include <signal.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <glib-object.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "mdm-manager.h"
#include "mdm-log.h"
#include "mdm-common.h"
#include "mdm-signal-handler.h"

#include "mdm-settings.h"
#include "mdm-settings-direct.h"
#include "mdm-settings-keys.h"

#define MDM_DBUS_NAME "org.mate.DisplayManager"

static void bus_proxy_destroyed_cb (DBusGProxy  *bus_proxy,
                                    MdmManager **managerp);

extern char **environ;

static MdmManager      *manager       = NULL;
static MdmSettings     *settings      = NULL;
static uid_t            mdm_uid       = -1;
static gid_t            mdm_gid       = -1;

static gboolean
timed_exit_cb (GMainLoop *loop)
{
        g_main_loop_quit (loop);
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
bus_reconnect (MdmManager *manager)
{
        DBusGConnection *bus;
        DBusGProxy      *bus_proxy;
        gboolean         ret;

        ret = TRUE;

        bus = get_system_bus ();
        if (bus == NULL) {
                goto out;
        }

        bus_proxy = get_bus_proxy (bus);
        if (bus_proxy == NULL) {
                g_warning ("Could not construct bus_proxy object; will retry");
                goto out;
        }

        if (! acquire_name_on_proxy (bus_proxy) ) {
                g_warning ("Could not acquire name; will retry");
                goto out;
        }

        manager = mdm_manager_new ();
        if (manager == NULL) {
                g_warning ("Could not construct manager object");
                exit (1);
        }

        g_signal_connect (bus_proxy,
                          "destroy",
                          G_CALLBACK (bus_proxy_destroyed_cb),
                          &manager);

        g_debug ("Successfully reconnected to D-Bus");

        mdm_manager_start (manager);

        ret = FALSE;

 out:
        return ret;
}

static void
bus_proxy_destroyed_cb (DBusGProxy  *bus_proxy,
                        MdmManager **managerp)
{
        g_debug ("Disconnected from D-Bus");

        if (managerp == NULL || *managerp == NULL) {
                /* probably shutting down or something */
                return;
        }

        g_object_unref (*managerp);
        *managerp = NULL;

        g_timeout_add_seconds (3, (GSourceFunc)bus_reconnect, managerp);
}

static void
delete_pid (void)
{
        g_unlink (MDM_PID_FILE);
}

static void
write_pid (void)
{
        int     pf;
        ssize_t written;
        char    pid[9];

        errno = 0;
        pf = open (MDM_PID_FILE, O_WRONLY|O_CREAT|O_TRUNC|O_EXCL, 0644);
        if (pf < 0) {
                g_warning (_("Cannot write PID file %s: possibly out of disk space: %s"),
                           MDM_PID_FILE,
                           g_strerror (errno));

                return;
        }

        snprintf (pid, sizeof (pid), "%lu\n", (long unsigned) getpid ());
        errno = 0;
        written = write (pf, pid, strlen (pid));
        close (pf);

        if (written < 0) {
                g_warning (_("Cannot write PID file %s: possibly out of disk space: %s"),
                           MDM_PID_FILE,
                           g_strerror (errno));
                return;
        }

        g_atexit (delete_pid);
}

static void
check_logdir (void)
{
        struct stat     statbuf;
        int             r;
        const char     *log_path;

        log_path = LOGDIR;

        r = g_stat (log_path, &statbuf);
        if (r < 0 || ! S_ISDIR (statbuf.st_mode))  {
                if (g_mkdir (log_path, 0755) < 0) {
                        mdm_fail (_("Logdir %s does not exist or isn't a directory."),
                                  log_path);
                }
                g_chmod (log_path, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
        }
}

static void
check_servauthdir (const char  *auth_path,
                   struct stat *statbuf)
{
        int r;

        /* Enter paranoia mode */
        r = g_stat (auth_path, statbuf);
        if (r < 0) {
                mdm_fail (_("Authdir %s does not exist. Aborting."), auth_path);
        }

        if (! S_ISDIR (statbuf->st_mode)) {
                mdm_fail (_("Authdir %s is not a directory. Aborting."), auth_path);
        }
}

static void
set_effective_user (uid_t uid)
{
        int res;

        res = 0;

        if (geteuid () != uid) {
                res = seteuid (uid);
        }

        if (res != 0) {
                g_error ("Cannot set uid to %d: %s",
                         (int)uid,
                         g_strerror (errno));
        }
}

static void
set_effective_group (gid_t gid)
{
        int res;

        res = 0;
        if (getegid () != gid) {
                res = setegid (gid);
        }

        if (res != 0) {
                g_error ("Cannot set gid to %d: %s",
                         (int)gid,
                         g_strerror (errno));
        }
}

static void
set_effective_user_group (uid_t uid,
                          gid_t gid)
{
        set_effective_user (0);
        set_effective_group (gid);
        if (uid != 0) {
                set_effective_user (0);
        }
}

static void
mdm_daemon_check_permissions (uid_t uid,
                              gid_t gid)
{
        struct stat statbuf;
        const char *auth_path;

        auth_path = LOGDIR;

        /* Enter paranoia mode */
        check_servauthdir (auth_path, &statbuf);

        set_effective_user_group (0, 0);

        /* Now set things up for us as  */
        chown (auth_path, 0, gid);
        g_chmod (auth_path, (S_IRWXU|S_IRWXG|S_ISVTX));

        set_effective_user_group (uid, gid);

        /* Again paranoid */
        check_servauthdir (auth_path, &statbuf);

        if G_UNLIKELY (statbuf.st_uid != 0 || statbuf.st_gid != gid)  {
                mdm_fail (_("Authdir %s is not owned by user %d, group %d. Aborting."),
                          auth_path,
                          (int)uid,
                          (int)gid);
        }

        if G_UNLIKELY (statbuf.st_mode != (S_IFDIR|S_IRWXU|S_IRWXG|S_ISVTX))  {
                mdm_fail (_("Authdir %s has wrong permissions %o. Should be %o. Aborting."),
                          auth_path,
                          statbuf.st_mode,
                          (S_IRWXU|S_IRWXG|S_ISVTX));
        }
}

static void
mdm_daemon_change_user (uid_t *uidp,
                        gid_t *gidp)
{
        char          *username;
        char          *groupname;
        uid_t          uid;
        gid_t          gid;
        struct passwd *pwent;
        struct group  *grent;

        username = NULL;
        groupname = NULL;
        uid = 0;
        gid = 0;

        mdm_settings_direct_get_string (MDM_KEY_USER, &username);
        mdm_settings_direct_get_string (MDM_KEY_GROUP, &groupname);

        if (username == NULL || groupname == NULL) {
                return;
        }

        g_debug ("Changing user:group to %s:%s", username, groupname);

        /* Lookup user and groupid for the MDM user */
        mdm_get_pwent_for_name (username, &pwent);

        /* Set uid and gid */
        if G_UNLIKELY (pwent == NULL) {
                mdm_fail (_("Can't find the MDM user '%s'. Aborting!"), username);
        } else {
                uid = pwent->pw_uid;
        }

        if G_UNLIKELY (uid == 0) {
                mdm_fail (_("The MDM user should not be root. Aborting!"));
        }

        grent = getgrnam (groupname);

        if G_UNLIKELY (grent == NULL) {
                mdm_fail (_("Can't find the MDM group '%s'. Aborting!"), groupname);
        } else  {
                gid = grent->gr_gid;
        }

        if G_UNLIKELY (gid == 0) {
                mdm_fail (_("The MDM group should not be root. Aborting!"));
        }

        /* gid remains 'mdm' */
        set_effective_user_group (uid, gid);

        if (uidp != NULL) {
                *uidp = uid;
        }

        if (gidp != NULL) {
                *gidp = gid;
        }

        g_free (username);
        g_free (groupname);
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
        GMainLoop          *main_loop;
        GOptionContext     *context;
        DBusGProxy         *bus_proxy;
        DBusGConnection    *connection;
        GError             *error;
        int                 ret;
        gboolean            res;
        gboolean            xdmcp_enabled;
        MdmSignalHandler   *signal_handler;
        static gboolean     do_timed_exit    = FALSE;
        static gboolean     print_version    = FALSE;
        static gboolean     fatal_warnings   = FALSE;
        static GOptionEntry entries []   = {
                { "fatal-warnings", 0, 0, G_OPTION_ARG_NONE, &fatal_warnings, N_("Make all warnings fatal"), NULL },
                { "timed-exit", 0, 0, G_OPTION_ARG_NONE, &do_timed_exit, N_("Exit after a time (for debugging)"), NULL },
                { "version", 0, 0, G_OPTION_ARG_NONE, &print_version, N_("Print MDM version"), NULL },

                { NULL }
        };

        bindtextdomain (GETTEXT_PACKAGE, MATELOCALEDIR);
        textdomain (GETTEXT_PACKAGE);
        setlocale (LC_ALL, "");

        ret = 1;

        mdm_set_fatal_warnings_if_unstable ();

        g_type_init ();

        context = g_option_context_new (_("MATE Display Manager"));
        g_option_context_add_main_entries (context, entries, NULL);
        g_option_context_set_ignore_unknown_options (context, TRUE);

        error = NULL;
        res = g_option_context_parse (context, &argc, &argv, &error);
        g_option_context_free (context);
        if (! res) {
                g_warning ("%s", error->message);
                g_error_free (error);
                goto out;
        }

        if (print_version) {
                g_print ("MDM %s\n", VERSION);
                exit (1);
        }

        if (fatal_warnings) {
                GLogLevelFlags fatal_mask;

                fatal_mask = g_log_set_always_fatal (G_LOG_FATAL_MASK);
                fatal_mask |= G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL;
                g_log_set_always_fatal (fatal_mask);
        }

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

        mdm_daemon_change_user (&mdm_uid, &mdm_gid);
        mdm_daemon_check_permissions (mdm_uid, mdm_gid);

        set_effective_user_group (0, 0);
        check_logdir ();

        /* XDM compliant error message */
        if (getuid () != 0) {
                /* make sure the pid file doesn't get wiped */
                g_warning (_("Only the root user can run MDM"));
                exit (-1);
        }

        /* pid file */
        delete_pid ();
        write_pid ();

        g_chdir (AUTHDIR);

        manager = mdm_manager_new ();

        if (manager == NULL) {
                goto out;
        }

        xdmcp_enabled = FALSE;
        mdm_settings_direct_get_boolean (MDM_KEY_XDMCP_ENABLE, &xdmcp_enabled);
        mdm_manager_set_xdmcp_enabled (manager, xdmcp_enabled);

        g_signal_connect (bus_proxy,
                          "destroy",
                          G_CALLBACK (bus_proxy_destroyed_cb),
                          &manager);

        main_loop = g_main_loop_new (NULL, FALSE);

        signal_handler = mdm_signal_handler_new ();
        mdm_signal_handler_set_fatal_func (signal_handler,
                                           (GDestroyNotify)g_main_loop_quit,
                                           main_loop);
        mdm_signal_handler_add_fatal (signal_handler);
        mdm_signal_handler_add (signal_handler, SIGTERM, signal_cb, NULL);
        mdm_signal_handler_add (signal_handler, SIGINT, signal_cb, NULL);
        mdm_signal_handler_add (signal_handler, SIGFPE, signal_cb, NULL);
        mdm_signal_handler_add (signal_handler, SIGHUP, signal_cb, NULL);
        mdm_signal_handler_add (signal_handler, SIGUSR1, signal_cb, NULL);

        if (do_timed_exit) {
                g_timeout_add_seconds (30, (GSourceFunc) timed_exit_cb, main_loop);
        }

        mdm_manager_start (manager);

        g_main_loop_run (main_loop);

        g_debug ("MDM finished, cleaning up...");

        if (manager != NULL) {
                g_object_unref (manager);
        }

        if (settings != NULL) {
                g_object_unref (settings);
        }

        if (signal_handler != NULL) {
                g_object_unref (signal_handler);
        }

        mdm_settings_direct_shutdown ();
        mdm_log_shutdown ();

        g_main_loop_unref (main_loop);

        ret = 0;

 out:

        return ret;
}
