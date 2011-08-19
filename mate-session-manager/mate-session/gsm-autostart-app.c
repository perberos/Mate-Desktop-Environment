/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 Novell, Inc.
 * Copyright (C) 2008 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <config.h>

#include <ctype.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>

#include <glib.h>
#include <gio/gio.h>

#include <mateconf/mateconf-client.h>

#include "gsm-autostart-app.h"
#include "gsm-util.h"

enum {
        AUTOSTART_LAUNCH_SPAWN = 0,
        AUTOSTART_LAUNCH_ACTIVATE
};

enum {
        GSM_CONDITION_NONE          = 0,
        GSM_CONDITION_IF_EXISTS     = 1,
        GSM_CONDITION_UNLESS_EXISTS = 2,
        GSM_CONDITION_MATE         = 3,
        GSM_CONDITION_UNKNOWN       = 4
};

#define GSM_SESSION_CLIENT_DBUS_INTERFACE "org.mate.SessionClient"

struct _GsmAutostartAppPrivate {
        char                 *desktop_filename;
        char                 *desktop_id;
        char                 *startup_id;

        EggDesktopFile       *desktop_file;

        /* desktop file state */
        char                 *condition_string;
        gboolean              condition;
        gboolean              autorestart;

        GFileMonitor         *condition_monitor;
        guint                 condition_notify_id;

        int                   launch_type;
        GPid                  pid;
        guint                 child_watch_id;

        DBusGProxy           *proxy;
        DBusGProxyCall       *proxy_call;
};

enum {
        CONDITION_CHANGED,
        LAST_SIGNAL
};

enum {
        PROP_0,
        PROP_DESKTOP_FILENAME
};

static guint signals[LAST_SIGNAL] = { 0 };

#define GSM_AUTOSTART_APP_GET_PRIVATE(object) (G_TYPE_INSTANCE_GET_PRIVATE ((object), GSM_TYPE_AUTOSTART_APP, GsmAutostartAppPrivate))

G_DEFINE_TYPE (GsmAutostartApp, gsm_autostart_app, GSM_TYPE_APP)

static void
gsm_autostart_app_init (GsmAutostartApp *app)
{
        app->priv = GSM_AUTOSTART_APP_GET_PRIVATE (app);

        app->priv->pid = -1;
        app->priv->condition_monitor = NULL;
        app->priv->condition = FALSE;
}

static gboolean
is_disabled (GsmApp *app)
{
        GsmAutostartAppPrivate *priv;

        priv = GSM_AUTOSTART_APP (app)->priv;

        /* GSM_AUTOSTART_APP_ENABLED_KEY key, used by old mate-session */
        if (egg_desktop_file_has_key (priv->desktop_file,
                                      GSM_AUTOSTART_APP_ENABLED_KEY, NULL) &&
            !egg_desktop_file_get_boolean (priv->desktop_file,
                                           GSM_AUTOSTART_APP_ENABLED_KEY, NULL)) {
                g_debug ("app %s is disabled by " GSM_AUTOSTART_APP_ENABLED_KEY,
                         gsm_app_peek_id (app));
                return TRUE;
        }

        /* Hidden key, used by autostart spec */
        if (egg_desktop_file_get_boolean (priv->desktop_file,
                                          EGG_DESKTOP_FILE_KEY_HIDDEN, NULL)) {
                g_debug ("app %s is disabled by Hidden",
                         gsm_app_peek_id (app));
                return TRUE;
        }

        /* Check OnlyShowIn/NotShowIn/TryExec */
        if (!egg_desktop_file_can_launch (priv->desktop_file, "MATE")) {
                g_debug ("app %s not installed or not for MATE",
                         gsm_app_peek_id (app));
                return TRUE;
        }

        /* Do not check AutostartCondition - this method is only to determine
         if the app is unconditionally disabled */

        return FALSE;
}

static gboolean
parse_condition_string (const char *condition_string,
                        guint      *condition_kindp,
                        char      **keyp)
{
        const char *space;
        const char *key;
        int         len;
        guint       kind;

        space = condition_string + strcspn (condition_string, " ");
        len = space - condition_string;
        key = space;
        while (isspace ((unsigned char)*key)) {
                key++;
        }

        if (!g_ascii_strncasecmp (condition_string, "if-exists", len) && key) {
                kind = GSM_CONDITION_IF_EXISTS;
        } else if (!g_ascii_strncasecmp (condition_string, "unless-exists", len) && key) {
                kind = GSM_CONDITION_UNLESS_EXISTS;
        } else if (!g_ascii_strncasecmp (condition_string, "MATE", len)) {
                kind = GSM_CONDITION_MATE;
        } else {
                key = NULL;
                kind = GSM_CONDITION_UNKNOWN;
        }

        if (keyp != NULL) {
                *keyp = g_strdup (key);
        }

        if (condition_kindp != NULL) {
                *condition_kindp = kind;
        }

        return (kind != GSM_CONDITION_UNKNOWN);
}

static void
if_exists_condition_cb (GFileMonitor     *monitor,
                        GFile            *file,
                        GFile            *other_file,
                        GFileMonitorEvent event,
                        GsmApp           *app)
{
        GsmAutostartAppPrivate *priv;
        gboolean                condition = FALSE;

        priv = GSM_AUTOSTART_APP (app)->priv;

        switch (event) {
        case G_FILE_MONITOR_EVENT_CREATED:
                condition = TRUE;
                break;
        case G_FILE_MONITOR_EVENT_DELETED:
                condition = FALSE;
                break;
        default:
                /* Ignore any other monitor event */
                return;
        }

        /* Emit only if the condition actually changed */
        if (condition != priv->condition) {
                priv->condition = condition;
                g_signal_emit (app, signals[CONDITION_CHANGED], 0, condition);
        }
}

static void
unless_exists_condition_cb (GFileMonitor     *monitor,
                            GFile            *file,
                            GFile            *other_file,
                            GFileMonitorEvent event,
                            GsmApp           *app)
{
        GsmAutostartAppPrivate *priv;
        gboolean                condition = FALSE;

        priv = GSM_AUTOSTART_APP (app)->priv;

        switch (event) {
        case G_FILE_MONITOR_EVENT_CREATED:
                condition = FALSE;
                break;
        case G_FILE_MONITOR_EVENT_DELETED:
                condition = TRUE;
                break;
        default:
                /* Ignore any other monitor event */
                return;
        }

        /* Emit only if the condition actually changed */
        if (condition != priv->condition) {
                priv->condition = condition;
                g_signal_emit (app, signals[CONDITION_CHANGED], 0, condition);
        }
}

static void
mateconf_condition_cb (MateConfClient *client,
                    guint        cnxn_id,
                    MateConfEntry  *entry,
                    gpointer     user_data)
{
        GsmApp                 *app;
        GsmAutostartAppPrivate *priv;
        gboolean                condition;

        g_return_if_fail (GSM_IS_APP (user_data));

        app = GSM_APP (user_data);

        priv = GSM_AUTOSTART_APP (app)->priv;

        condition = FALSE;
        if (entry->value != NULL && entry->value->type == MATECONF_VALUE_BOOL) {
                condition = mateconf_value_get_bool (entry->value);
        }

        g_debug ("GsmAutostartApp: app:%s condition changed condition:%d",
                 gsm_app_peek_id (app),
                 condition);

        /* Emit only if the condition actually changed */
        if (condition != priv->condition) {
                priv->condition = condition;
                g_signal_emit (app, signals[CONDITION_CHANGED], 0, condition);
        }
}

static void
setup_condition_monitor (GsmAutostartApp *app)
{
        guint    kind;
        char    *key;
        gboolean res;
        gboolean disabled;

        if (app->priv->condition_monitor != NULL) {
                g_file_monitor_cancel (app->priv->condition_monitor);
        }

        if (app->priv->condition_notify_id > 0) {
                MateConfClient *client;
                client = mateconf_client_get_default ();
                mateconf_client_notify_remove (client,
                                            app->priv->condition_notify_id);
                app->priv->condition_notify_id = 0;
        }

        if (app->priv->condition_string == NULL) {
                return;
        }

        /* if it is disabled outright there is no point in monitoring */
        if (is_disabled (GSM_APP (app))) {
                return;
        }

        key = NULL;
        res = parse_condition_string (app->priv->condition_string, &kind, &key);
        if (! res) {
                g_free (key);
                return;
        }

        if (key == NULL) {
                return;
        }

        if (kind == GSM_CONDITION_IF_EXISTS) {
                char  *file_path;
                GFile *file;

                file_path = g_build_filename (g_get_user_config_dir (), key, NULL);

                disabled = !g_file_test (file_path, G_FILE_TEST_EXISTS);

                file = g_file_new_for_path (file_path);
                app->priv->condition_monitor = g_file_monitor_file (file, 0, NULL, NULL);

                g_signal_connect (app->priv->condition_monitor, "changed",
                                  G_CALLBACK (if_exists_condition_cb),
                                  app);

                g_object_unref (file);
                g_free (file_path);
        } else if (kind == GSM_CONDITION_UNLESS_EXISTS) {
                char  *file_path;
                GFile *file;

                file_path = g_build_filename (g_get_user_config_dir (), key, NULL);

                disabled = g_file_test (file_path, G_FILE_TEST_EXISTS);

                file = g_file_new_for_path (file_path);
                app->priv->condition_monitor = g_file_monitor_file (file, 0, NULL, NULL);

                g_signal_connect (app->priv->condition_monitor, "changed",
                                  G_CALLBACK (unless_exists_condition_cb),
                                  app);

                g_object_unref (file);
                g_free (file_path);
        } else if (kind == GSM_CONDITION_MATE) {
                MateConfClient *client;
                char        *dir;

                client = mateconf_client_get_default ();
                g_assert (MATECONF_IS_CLIENT (client));

                disabled = !mateconf_client_get_bool (client, key, NULL);

                dir = g_path_get_dirname (key);

                mateconf_client_add_dir (client,
                                      dir,
                                      MATECONF_CLIENT_PRELOAD_NONE, NULL);
                g_free (dir);

                app->priv->condition_notify_id = mateconf_client_notify_add (client,
                                                                          key,
                                                                          mateconf_condition_cb,
                                                                          app, NULL, NULL);
                g_object_unref (client);
        } else {
                disabled = TRUE;
        }

        g_free (key);

        /* FIXME: cache the disabled value? */
}

static gboolean
load_desktop_file (GsmAutostartApp *app)
{
        char    *dbus_name;
        char    *startup_id;
        char    *phase_str;
        int      phase;
        gboolean res;

        if (app->priv->desktop_file == NULL) {
                return FALSE;
        }

        phase_str = egg_desktop_file_get_string (app->priv->desktop_file,
                                                 GSM_AUTOSTART_APP_PHASE_KEY,
                                                 NULL);
        if (phase_str != NULL) {
                if (strcmp (phase_str, "Initialization") == 0) {
                        phase = GSM_MANAGER_PHASE_INITIALIZATION;
                } else if (strcmp (phase_str, "WindowManager") == 0) {
                        phase = GSM_MANAGER_PHASE_WINDOW_MANAGER;
                } else if (strcmp (phase_str, "Panel") == 0) {
                        phase = GSM_MANAGER_PHASE_PANEL;
                } else if (strcmp (phase_str, "Desktop") == 0) {
                        phase = GSM_MANAGER_PHASE_DESKTOP;
                } else {
                        phase = GSM_MANAGER_PHASE_APPLICATION;
                }

                g_free (phase_str);
        } else {
                phase = GSM_MANAGER_PHASE_APPLICATION;
        }

        dbus_name = egg_desktop_file_get_string (app->priv->desktop_file,
                                                 GSM_AUTOSTART_APP_DBUS_NAME_KEY,
                                                 NULL);
        if (dbus_name != NULL) {
                app->priv->launch_type = AUTOSTART_LAUNCH_ACTIVATE;
        } else {
                app->priv->launch_type = AUTOSTART_LAUNCH_SPAWN;
        }

        /* this must only be done on first load */
        switch (app->priv->launch_type) {
        case AUTOSTART_LAUNCH_SPAWN:
                startup_id =
                        egg_desktop_file_get_string (app->priv->desktop_file,
                                                     GSM_AUTOSTART_APP_STARTUP_ID_KEY,
                                                     NULL);

                if (startup_id == NULL) {
                        startup_id = gsm_util_generate_startup_id ();
                }
                break;
        case AUTOSTART_LAUNCH_ACTIVATE:
                startup_id = g_strdup (dbus_name);
                break;
        default:
                g_assert_not_reached ();
        }

        res = egg_desktop_file_has_key (app->priv->desktop_file,
                                        GSM_AUTOSTART_APP_AUTORESTART_KEY,
                                        NULL);
        if (res) {
                app->priv->autorestart = egg_desktop_file_get_boolean (app->priv->desktop_file,
                                                                       GSM_AUTOSTART_APP_AUTORESTART_KEY,
                                                                       NULL);
        } else {
                app->priv->autorestart = FALSE;
        }

        g_free (app->priv->condition_string);
        app->priv->condition_string = egg_desktop_file_get_string (app->priv->desktop_file,
                                                                   "AutostartCondition",
                                                                   NULL);
        setup_condition_monitor (app);

        g_object_set (app,
                      "phase", phase,
                      "startup-id", startup_id,
                      NULL);

        g_free (startup_id);
        g_free (dbus_name);

        return TRUE;
}

static void
gsm_autostart_app_set_desktop_filename (GsmAutostartApp *app,
                                        const char      *desktop_filename)
{
        GError *error;

        if (app->priv->desktop_file != NULL) {
                egg_desktop_file_free (app->priv->desktop_file);
                app->priv->desktop_file = NULL;
                g_free (app->priv->desktop_id);
        }

        if (desktop_filename == NULL) {
                return;
        }

        app->priv->desktop_id = g_path_get_basename (desktop_filename);

        error = NULL;
        app->priv->desktop_file = egg_desktop_file_new (desktop_filename, &error);
        if (app->priv->desktop_file == NULL) {
                g_warning ("Could not parse desktop file %s: %s",
                           desktop_filename,
                           error->message);
                g_error_free (error);
                return;
        }
}

static void
gsm_autostart_app_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
        GsmAutostartApp *self;

        self = GSM_AUTOSTART_APP (object);

        switch (prop_id) {
        case PROP_DESKTOP_FILENAME:
                gsm_autostart_app_set_desktop_filename (self, g_value_get_string (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gsm_autostart_app_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
        GsmAutostartApp *self;

        self = GSM_AUTOSTART_APP (object);

        switch (prop_id) {
        case PROP_DESKTOP_FILENAME:
                if (self->priv->desktop_file != NULL) {
                        g_value_set_string (value, egg_desktop_file_get_source (self->priv->desktop_file));
                } else {
                        g_value_set_string (value, NULL);
                }
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gsm_autostart_app_dispose (GObject *object)
{
        GsmAutostartAppPrivate *priv;

        priv = GSM_AUTOSTART_APP (object)->priv;

        if (priv->startup_id) {
                g_free (priv->startup_id);
                priv->startup_id = NULL;
        }

        if (priv->condition_string) {
                g_free (priv->condition_string);
                priv->condition_string = NULL;
        }

        if (priv->desktop_file) {
                egg_desktop_file_free (priv->desktop_file);
                priv->desktop_file = NULL;
        }

        if (priv->desktop_id) {
                g_free (priv->desktop_id);
                priv->desktop_id = NULL;
        }

        if (priv->child_watch_id > 0) {
                g_source_remove (priv->child_watch_id);
                priv->child_watch_id = 0;
        }

        if (priv->proxy_call != NULL) {
                dbus_g_proxy_cancel_call (priv->proxy, priv->proxy_call);
                priv->proxy_call = NULL;
        }

        if (priv->proxy != NULL) {
                g_object_unref (priv->proxy);
                priv->proxy = NULL;
        }

        if (priv->condition_monitor) {
                g_file_monitor_cancel (priv->condition_monitor);
        }

        G_OBJECT_CLASS (gsm_autostart_app_parent_class)->dispose (object);
}

static gboolean
is_running (GsmApp *app)
{
        GsmAutostartAppPrivate *priv;
        gboolean                is;

        priv = GSM_AUTOSTART_APP (app)->priv;

        /* is running if pid is still valid or
         * or a client is connected
         */
        /* FIXME: check client */
        is = (priv->pid != -1);

        return is;
}

static gboolean
is_conditionally_disabled (GsmApp *app)
{
        GsmAutostartAppPrivate *priv;
        gboolean                res;
        gboolean                disabled;
        char                   *key;
        guint                   kind;

        priv = GSM_AUTOSTART_APP (app)->priv;

        /* Check AutostartCondition */
        if (priv->condition_string == NULL) {
                return FALSE;
        }

        key = NULL;
        res = parse_condition_string (priv->condition_string, &kind, &key);
        if (! res) {
                g_free (key);
                return TRUE;
        }

        if (key == NULL) {
                return TRUE;
        }

        if (kind == GSM_CONDITION_IF_EXISTS) {
                char *file_path;

                file_path = g_build_filename (g_get_user_config_dir (), key, NULL);
                disabled = !g_file_test (file_path, G_FILE_TEST_EXISTS);
                g_free (file_path);
        } else if (kind == GSM_CONDITION_UNLESS_EXISTS) {
                char *file_path;

                file_path = g_build_filename (g_get_user_config_dir (), key, NULL);
                disabled = g_file_test (file_path, G_FILE_TEST_EXISTS);
                g_free (file_path);
        } else if (kind == GSM_CONDITION_MATE) {
                MateConfClient *client;
                client = mateconf_client_get_default ();
                g_assert (MATECONF_IS_CLIENT (client));
                disabled = !mateconf_client_get_bool (client, key, NULL);
                g_object_unref (client);
        } else {
                disabled = TRUE;
        }

        /* Set initial condition */
        priv->condition = !disabled;

        g_free (key);

        return disabled;
}

static void
app_exited (GPid             pid,
            int              status,
            GsmAutostartApp *app)
{
        g_debug ("GsmAutostartApp: (pid:%d) done (%s:%d)",
                 (int) pid,
                 WIFEXITED (status) ? "status"
                 : WIFSIGNALED (status) ? "signal"
                 : "unknown",
                 WIFEXITED (status) ? WEXITSTATUS (status)
                 : WIFSIGNALED (status) ? WTERMSIG (status)
                 : -1);

        g_spawn_close_pid (app->priv->pid);
        app->priv->pid = -1;
        app->priv->child_watch_id = 0;

        if (WIFEXITED (status)) {
                gsm_app_exited (GSM_APP (app));
        } else if (WIFSIGNALED (status)) {
                gsm_app_died (GSM_APP (app));
        }
}

static int
_signal_pid (int pid,
             int signal)
{
        int status = -1;

        /* perhaps block sigchld */
        g_debug ("GsmAutostartApp: sending signal %d to process %d", signal, pid);
        errno = 0;
        status = kill (pid, signal);

        if (status < 0) {
                if (errno == ESRCH) {
                        g_warning ("Child process %d was already dead.",
                                   (int)pid);
                } else {
                        g_warning ("Couldn't kill child process %d: %s",
                                   pid,
                                   g_strerror (errno));
                }
        }

        /* perhaps unblock sigchld */

        return status;
}

static gboolean
autostart_app_stop_spawn (GsmAutostartApp *app,
                          GError         **error)
{
        int res;

        if (app->priv->pid < 1) {
                g_set_error (error,
                             GSM_APP_ERROR,
                             GSM_APP_ERROR_STOP,
                             "Not running");
                return FALSE;
        }

        res = _signal_pid (app->priv->pid, SIGTERM);
        if (res != 0) {
                g_set_error (error,
                             GSM_APP_ERROR,
                             GSM_APP_ERROR_STOP,
                             "Unable to stop: %s",
                             g_strerror (errno));
                return FALSE;
        }

        return TRUE;
}

static gboolean
autostart_app_stop_activate (GsmAutostartApp *app,
                             GError         **error)
{
        return TRUE;
}

static gboolean
gsm_autostart_app_stop (GsmApp  *app,
                        GError **error)
{
        GsmAutostartApp *aapp;
        gboolean         ret;

        aapp = GSM_AUTOSTART_APP (app);

        g_return_val_if_fail (aapp->priv->desktop_file != NULL, FALSE);

        switch (aapp->priv->launch_type) {
        case AUTOSTART_LAUNCH_SPAWN:
                ret = autostart_app_stop_spawn (aapp, error);
                break;
        case AUTOSTART_LAUNCH_ACTIVATE:
                ret = autostart_app_stop_activate (aapp, error);
                break;
        default:
                g_assert_not_reached ();
                break;
        }

        return ret;
}

static gboolean
autostart_app_start_spawn (GsmAutostartApp *app,
                           GError         **error)
{
        char            *env[2] = { NULL, NULL };
        gboolean         success;
        GError          *local_error;
        const char      *startup_id;
        char            *command;

        startup_id = gsm_app_peek_startup_id (GSM_APP (app));
        g_assert (startup_id != NULL);

        env[0] = g_strdup_printf ("DESKTOP_AUTOSTART_ID=%s", startup_id);

        local_error = NULL;
        command = egg_desktop_file_parse_exec (app->priv->desktop_file,
                                               NULL,
                                               &local_error);
        if (command == NULL) {
                g_warning ("Unable to parse command from  '%s': %s",
                           egg_desktop_file_get_source (app->priv->desktop_file),
                           local_error->message);
                g_error_free (local_error);
        }

        g_debug ("GsmAutostartApp: starting %s: command=%s startup-id=%s", app->priv->desktop_id, command, startup_id);
        g_free (command);

        g_free (app->priv->startup_id);
        local_error = NULL;
        success = egg_desktop_file_launch (app->priv->desktop_file,
                                           NULL,
                                           &local_error,
                                           EGG_DESKTOP_FILE_LAUNCH_PUTENV, env,
                                           EGG_DESKTOP_FILE_LAUNCH_FLAGS, G_SPAWN_DO_NOT_REAP_CHILD,
                                           EGG_DESKTOP_FILE_LAUNCH_RETURN_PID, &app->priv->pid,
                                           EGG_DESKTOP_FILE_LAUNCH_RETURN_STARTUP_ID, &app->priv->startup_id,
                                           NULL);
        g_free (env[0]);

        if (success) {
                g_debug ("GsmAutostartApp: started pid:%d", app->priv->pid);
                app->priv->child_watch_id = g_child_watch_add (app->priv->pid,
                                                               (GChildWatchFunc)app_exited,
                                                               app);
        } else {
                g_set_error (error,
                             GSM_APP_ERROR,
                             GSM_APP_ERROR_START,
                             "Unable to start application: %s", local_error->message);
                g_error_free (local_error);
        }

        return success;
}

static void
start_notify (DBusGProxy      *proxy,
              DBusGProxyCall  *call,
              GsmAutostartApp *app)
{
        gboolean res;
        GError  *error;

        error = NULL;
        res = dbus_g_proxy_end_call (proxy,
                                     call,
                                     &error,
                                     G_TYPE_INVALID);
        app->priv->proxy_call = NULL;

        if (! res) {
                g_warning ("GsmAutostartApp: Error starting application: %s", error->message);
                g_error_free (error);
        } else {
                g_debug ("GsmAutostartApp: Started application %s", app->priv->desktop_id);
        }
}

static gboolean
autostart_app_start_activate (GsmAutostartApp  *app,
                              GError          **error)
{
        const char      *name;
        char            *path;
        char            *arguments;
        DBusGConnection *bus;
        GError          *local_error;

        local_error = NULL;
        bus = dbus_g_bus_get (DBUS_BUS_SESSION, &local_error);
        if (bus == NULL) {
                if (local_error != NULL) {
                        g_warning ("error getting session bus: %s", local_error->message);
                }
                g_propagate_error (error, local_error);
                return FALSE;
        }

        name = gsm_app_peek_startup_id (GSM_APP (app));
        g_assert (name != NULL);

        path = egg_desktop_file_get_string (app->priv->desktop_file,
                                            GSM_AUTOSTART_APP_DBUS_PATH_KEY,
                                            NULL);
        if (path == NULL) {
                /* just pick one? */
                path = g_strdup ("/");
        }

        arguments = egg_desktop_file_get_string (app->priv->desktop_file,
                                                 GSM_AUTOSTART_APP_DBUS_ARGS_KEY,
                                                 NULL);

        app->priv->proxy = dbus_g_proxy_new_for_name (bus,
                                                      name,
                                                      path,
                                                      GSM_SESSION_CLIENT_DBUS_INTERFACE);
        if (app->priv->proxy == NULL) {
                g_set_error (error,
                             GSM_APP_ERROR,
                             GSM_APP_ERROR_START,
                             "Unable to start application: unable to create proxy for client");
                return FALSE;
        }

        app->priv->proxy_call = dbus_g_proxy_begin_call (app->priv->proxy,
                                                         "Start",
                                                         (DBusGProxyCallNotify)start_notify,
                                                         app,
                                                         NULL,
                                                         G_TYPE_STRING, arguments,
                                                         G_TYPE_INVALID);
        if (app->priv->proxy_call == NULL) {
                g_object_unref (app->priv->proxy);
                app->priv->proxy = NULL;
                g_set_error (error,
                             GSM_APP_ERROR,
                             GSM_APP_ERROR_START,
                             "Unable to start application: unable to call Start on client");
                return FALSE;
        }

        return TRUE;
}

static gboolean
gsm_autostart_app_start (GsmApp  *app,
                         GError **error)
{
        GsmAutostartApp *aapp;
        gboolean         ret;

        aapp = GSM_AUTOSTART_APP (app);

        g_return_val_if_fail (aapp->priv->desktop_file != NULL, FALSE);

        switch (aapp->priv->launch_type) {
        case AUTOSTART_LAUNCH_SPAWN:
                ret = autostart_app_start_spawn (aapp, error);
                break;
        case AUTOSTART_LAUNCH_ACTIVATE:
                ret = autostart_app_start_activate (aapp, error);
                break;
        default:
                g_assert_not_reached ();
                break;
        }

        return ret;
}

static gboolean
gsm_autostart_app_restart (GsmApp  *app,
                           GError **error)
{
        GError  *local_error;
        gboolean res;

        /* ignore stop errors - it is fine if it is already stopped */
        local_error = NULL;
        res = gsm_app_stop (app, &local_error);
        if (! res) {
                g_debug ("GsmAutostartApp: Couldn't stop app: %s", local_error->message);
                g_error_free (local_error);
        }

        res = gsm_app_start (app, &local_error);
        if (! res) {
                g_propagate_error (error, local_error);
                return FALSE;
        }

        return TRUE;
}

static gboolean
gsm_autostart_app_provides (GsmApp     *app,
                            const char *service)
{
        char           **provides;
        gsize            len;
        gsize            i;
        GsmAutostartApp *aapp;

        g_return_val_if_fail (GSM_IS_APP (app), FALSE);

        aapp = GSM_AUTOSTART_APP (app);

        if (aapp->priv->desktop_file == NULL) {
                return FALSE;
        }

        provides = egg_desktop_file_get_string_list (aapp->priv->desktop_file,
                                                     GSM_AUTOSTART_APP_PROVIDES_KEY,
                                                     &len, NULL);
        if (!provides) {
                return FALSE;
        }

        for (i = 0; i < len; i++) {
                if (!strcmp (provides[i], service)) {
                        g_strfreev (provides);
                        return TRUE;
                }
        }

        g_strfreev (provides);
        return FALSE;
}

static gboolean
gsm_autostart_app_has_autostart_condition (GsmApp     *app,
                                           const char *condition)
{
        GsmAutostartApp *aapp;

        g_return_val_if_fail (GSM_IS_APP (app), FALSE);
        g_return_val_if_fail (condition != NULL, FALSE);

        aapp = GSM_AUTOSTART_APP (app);

        if (aapp->priv->condition_string == NULL) {
                return FALSE;
        }

        if (strcmp (aapp->priv->condition_string, condition) == 0) {
                return TRUE;
        }

        return FALSE;
}

static gboolean
gsm_autostart_app_get_autorestart (GsmApp *app)
{
        gboolean res;
        gboolean autorestart;

        if (GSM_AUTOSTART_APP (app)->priv->desktop_file == NULL) {
                return FALSE;
        }

        autorestart = FALSE;

        res = egg_desktop_file_has_key (GSM_AUTOSTART_APP (app)->priv->desktop_file,
                                        GSM_AUTOSTART_APP_AUTORESTART_KEY,
                                        NULL);
        if (res) {
                autorestart = egg_desktop_file_get_boolean (GSM_AUTOSTART_APP (app)->priv->desktop_file,
                                                            GSM_AUTOSTART_APP_AUTORESTART_KEY,
                                                            NULL);
        }

        return autorestart;
}

static const char *
gsm_autostart_app_get_app_id (GsmApp *app)
{
        const char *location;
        const char *slash;

        if (GSM_AUTOSTART_APP (app)->priv->desktop_file == NULL) {
                return NULL;
        }

        location = egg_desktop_file_get_source (GSM_AUTOSTART_APP (app)->priv->desktop_file);

        slash = strrchr (location, '/');
        if (slash != NULL) {
                return slash + 1;
        } else {
                return location;
        }
}

static GObject *
gsm_autostart_app_constructor (GType                  type,
                               guint                  n_construct_properties,
                               GObjectConstructParam *construct_properties)
{
        GsmAutostartApp *app;

        app = GSM_AUTOSTART_APP (G_OBJECT_CLASS (gsm_autostart_app_parent_class)->constructor (type,
                                                                                               n_construct_properties,
                                                                                               construct_properties));

        if (! load_desktop_file (app)) {
                g_object_unref (app);
                app = NULL;
        }

        return G_OBJECT (app);
}

static void
gsm_autostart_app_class_init (GsmAutostartAppClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        GsmAppClass  *app_class = GSM_APP_CLASS (klass);

        object_class->set_property = gsm_autostart_app_set_property;
        object_class->get_property = gsm_autostart_app_get_property;
        object_class->dispose = gsm_autostart_app_dispose;
        object_class->constructor = gsm_autostart_app_constructor;

        app_class->impl_is_disabled = is_disabled;
        app_class->impl_is_conditionally_disabled = is_conditionally_disabled;
        app_class->impl_is_running = is_running;
        app_class->impl_start = gsm_autostart_app_start;
        app_class->impl_restart = gsm_autostart_app_restart;
        app_class->impl_stop = gsm_autostart_app_stop;
        app_class->impl_provides = gsm_autostart_app_provides;
        app_class->impl_has_autostart_condition = gsm_autostart_app_has_autostart_condition;
        app_class->impl_get_app_id = gsm_autostart_app_get_app_id;
        app_class->impl_get_autorestart = gsm_autostart_app_get_autorestart;

        g_object_class_install_property (object_class,
                                         PROP_DESKTOP_FILENAME,
                                         g_param_spec_string ("desktop-filename",
                                                              "Desktop filename",
                                                              "Freedesktop .desktop file",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
        signals[CONDITION_CHANGED] =
                g_signal_new ("condition-changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GsmAutostartAppClass, condition_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__BOOLEAN,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_BOOLEAN);

        g_type_class_add_private (object_class, sizeof (GsmAutostartAppPrivate));
}

GsmApp *
gsm_autostart_app_new (const char *desktop_file)
{
        GsmAutostartApp *app;

        app = g_object_new (GSM_TYPE_AUTOSTART_APP,
                            "desktop-filename", desktop_file,
                            NULL);

        return GSM_APP (app);
}
