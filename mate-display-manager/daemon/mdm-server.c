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
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <ctype.h>
#include <pwd.h>
#include <grp.h>
#include <signal.h>
#include <sys/resource.h>

#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <glib-object.h>

#include <X11/Xlib.h> /* for Display */

#include "mdm-common.h"
#include "mdm-signal-handler.h"

#include "mdm-server.h"

extern char **environ;

#define MDM_SERVER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_SERVER, MdmServerPrivate))

/* These are the servstat values, also used as server
 * process exit codes */
#define SERVER_TIMEOUT 2        /* Server didn't start */
#define SERVER_DEAD 250         /* Server stopped */
#define SERVER_PENDING 251      /* Server started but not ready for connections yet */
#define SERVER_RUNNING 252      /* Server running and ready for connections */
#define SERVER_ABORT 253        /* Server failed badly. Suspending display. */

#define MAX_LOGS 5

struct MdmServerPrivate
{
        char    *command;
        GPid     pid;

        gboolean disable_tcp;
        int      priority;
        char    *user_name;
        char    *session_args;

        char    *log_dir;
        char    *display_name;
        char    *display_device;
        char    *auth_file;

        gboolean is_parented;
        char    *parent_display_name;
        char    *parent_auth_file;
        char    *chosen_hostname;

        guint    child_watch_id;
};

enum {
        PROP_0,
        PROP_DISPLAY_NAME,
        PROP_DISPLAY_DEVICE,
        PROP_AUTH_FILE,
        PROP_IS_PARENTED,
        PROP_PARENT_DISPLAY_NAME,
        PROP_PARENT_AUTH_FILE,
        PROP_CHOSEN_HOSTNAME,
        PROP_COMMAND,
        PROP_PRIORITY,
        PROP_USER_NAME,
        PROP_SESSION_ARGS,
        PROP_LOG_DIR,
        PROP_DISABLE_TCP,
};

enum {
        READY,
        EXITED,
        DIED,
        LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0, };

static void     mdm_server_class_init   (MdmServerClass *klass);
static void     mdm_server_init         (MdmServer      *server);
static void     mdm_server_finalize     (GObject        *object);

G_DEFINE_TYPE (MdmServer, mdm_server, G_TYPE_OBJECT)

static char *
_mdm_server_query_ck_for_display_device (MdmServer *server)
{
        char    *out;
        char    *command;
        int      status;
        gboolean res;
        GError  *error;

        g_return_val_if_fail (MDM_IS_SERVER (server), NULL);

        error = NULL;
        command = g_strdup_printf (LIBEXECDIR "/ck-get-x11-display-device --display %s",
                                   server->priv->display_name);

        g_debug ("MdmServer: Running helper %s", command);
        out = NULL;
        res = g_spawn_command_line_sync (command,
                                         &out,
                                         NULL,
                                         &status,
                                         &error);
        if (! res) {
                g_warning ("Could not run helper: %s", error->message);
                g_error_free (error);
        } else {
                out = g_strstrip (out);
                g_debug ("MdmServer: Got tty: '%s'", out);
        }

        g_free (command);

        return out;
}

char *
mdm_server_get_display_device (MdmServer *server)
{
        if (server->priv->display_device == NULL) {
                server->priv->display_device =
                    _mdm_server_query_ck_for_display_device (server);

                g_object_notify (G_OBJECT (server), "display-device");
        }

        return g_strdup (server->priv->display_device);
}

static gboolean
emit_ready_idle (MdmServer *server)
{
        g_debug ("MdmServer: Got USR1 from X server - emitting READY");

        g_signal_emit (server, signals[READY], 0);
        return FALSE;
}


static gboolean
signal_cb (int        signo,
           MdmServer *server)

{
        g_idle_add ((GSourceFunc)emit_ready_idle, server);

        return TRUE;
}

static void
add_ready_handler (MdmServer *server)
{
        MdmSignalHandler *signal_handler;

        signal_handler = mdm_signal_handler_new ();
        mdm_signal_handler_add (signal_handler,
                                SIGUSR1,
                                (MdmSignalHandlerFunc)signal_cb,
                                server);
        g_object_unref (signal_handler);
}

static void
remove_ready_handler (MdmServer *server)
{
        MdmSignalHandler *signal_handler;

        signal_handler = mdm_signal_handler_new ();
        mdm_signal_handler_remove_func (signal_handler,
                                        SIGUSR1,
                                        (MdmSignalHandlerFunc)signal_cb,
                                        server);
        g_object_unref (signal_handler);
}

/* We keep a connection (parent_dsp) open with the parent X server
 * before running a proxy on it to prevent the X server resetting
 * as we open and close other connections.
 * Note that XDMCP servers, by default, reset when the seed X
 * connection closes whereas usually the X server only quits when
 * all X connections have closed.
 */
#if 0
static gboolean
connect_to_parent (MdmServer *server)
{
        int maxtries;
        int openretries;

        g_debug ("MdmServer: Connecting to parent display \'%s\'",
                   d->parent_disp);

        d->parent_dsp = NULL;

        maxtries = SERVER_IS_XDMCP (d) ? 10 : 2;

        openretries = 0;
        while (openretries < maxtries &&
               d->parent_dsp == NULL) {
                d->parent_dsp = XOpenDisplay (d->parent_disp);

                if G_UNLIKELY (d->parent_dsp == NULL) {
                        g_debug ("MdmServer: Sleeping %d on a retry", 1+openretries*2);
                        mdm_sleep_no_signal (1+openretries*2);
                        openretries++;
                }
        }

        if (d->parent_dsp == NULL)
                mdm_error (_("%s: failed to connect to parent display \'%s\'"),
                           "mdm_server_start", d->parent_disp);

        return d->parent_dsp != NULL;
}
#endif

static gboolean
mdm_server_resolve_command_line (MdmServer  *server,
                                 const char *vtarg,
                                 int        *argcp,
                                 char     ***argvp)
{
        int      argc;
        char   **argv;
        int      len;
        int      i;
        gboolean gotvtarg = FALSE;
        gboolean query_in_arglist = FALSE;

        g_shell_parse_argv (server->priv->command, &argc, &argv, NULL);

        for (len = 0; argv != NULL && argv[len] != NULL; len++) {
                char *arg = argv[len];

                /* HACK! Not to add vt argument to servers that already force
                 * allocation.  Mostly for backwards compat only */
                if (strncmp (arg, "vt", 2) == 0 &&
                    isdigit (arg[2]) &&
                    (arg[3] == '\0' ||
                     (isdigit (arg[3]) && arg[4] == '\0')))
                        gotvtarg = TRUE;
                if (strcmp (arg, "-query") == 0 ||
                    strcmp (arg, "-indirect") == 0)
                        query_in_arglist = TRUE;
        }

        argv = g_renew (char *, argv, len + 10);
        /* shift args down one */
        for (i = len - 1; i >= 1; i--) {
                argv[i+1] = argv[i];
        }

        /* server number is the FIRST argument, before any others */
        argv[1] = g_strdup (server->priv->display_name);
        len++;

        if (server->priv->auth_file != NULL) {
                argv[len++] = g_strdup ("-auth");
                argv[len++] = g_strdup (server->priv->auth_file);
        }

        if (server->priv->chosen_hostname) {
                /* run just one session */
                argv[len++] = g_strdup ("-terminate");
                argv[len++] = g_strdup ("-query");
                argv[len++] = g_strdup (server->priv->chosen_hostname);
                query_in_arglist = TRUE;
        }

        if (server->priv->disable_tcp && ! query_in_arglist) {
                argv[len++] = g_strdup ("-nolisten");
                argv[len++] = g_strdup ("tcp");
        }

        if (vtarg != NULL && ! gotvtarg) {
                argv[len++] = g_strdup (vtarg);
        }

        argv[len++] = NULL;

        *argvp = argv;
        *argcp = len;

        return TRUE;
}

static void
rotate_logs (const char *path,
             guint       n_copies)
{
        int i;

        for (i = n_copies - 1; i > 0; i--) {
                char *name_n;
                char *name_n1;

                name_n = g_strdup_printf ("%s.%d", path, i);
                if (i > 1) {
                        name_n1 = g_strdup_printf ("%s.%d", path, i - 1);
                } else {
                        name_n1 = g_strdup (path);
                }

                VE_IGNORE_EINTR (g_unlink (name_n));
                VE_IGNORE_EINTR (g_rename (name_n1, name_n));

                g_free (name_n1);
                g_free (name_n);
        }

        VE_IGNORE_EINTR (g_unlink (path));
}

static void
change_user (MdmServer *server)
{
        struct passwd *pwent;

        if (server->priv->user_name == NULL) {
                return;
        }

        mdm_get_pwent_for_name (server->priv->user_name, &pwent);
        if (pwent == NULL) {
                g_warning (_("Server was to be spawned by user %s but that user doesn't exist"),
                           server->priv->user_name);
                _exit (1);
        }

        g_debug ("MdmServer: Changing (uid:gid) for child process to (%d:%d)",
                 pwent->pw_uid,
                 pwent->pw_gid);

        if (pwent->pw_uid != 0) {
                if (setgid (pwent->pw_gid) < 0)  {
                        g_warning (_("Couldn't set groupid to %d"),
                                   pwent->pw_gid);
                        _exit (1);
                }

                if (initgroups (pwent->pw_name, pwent->pw_gid) < 0) {
                        g_warning (_("initgroups () failed for %s"),
                                   pwent->pw_name);
                        _exit (1);
                }

                if (setuid (pwent->pw_uid) < 0)  {
                        g_warning (_("Couldn't set userid to %d"),
                                   (int)pwent->pw_uid);
                        _exit (1);
                }
        } else {
                gid_t groups[1] = { 0 };

                if (setgid (0) < 0)  {
                        g_warning (_("Couldn't set groupid to %d"), 0);
                        /* Don't error out, it's not fatal, if it fails we'll
                         * just still be */
                }

                /* this will get rid of any suplementary groups etc... */
                setgroups (1, groups);
        }
}

static void
server_child_setup (MdmServer *server)
{
        int              logfd;
        struct sigaction ign_signal;
        sigset_t         mask;
        char            *log_file;
        char            *log_path;

        log_file = g_strdup_printf ("%s.log", server->priv->display_name);
        log_path = g_build_filename (server->priv->log_dir, log_file, NULL);
        g_free (log_file);

        /* Rotate the X server logs */
        rotate_logs (log_path, MAX_LOGS);

        /* Log all output from spawned programs to a file */
        g_debug ("MdmServer: Opening logfile for server %s", log_path);

        VE_IGNORE_EINTR (g_unlink (log_path));
        VE_IGNORE_EINTR (logfd = open (log_path, O_CREAT|O_APPEND|O_TRUNC|O_WRONLY|O_EXCL, 0644));

        g_free (log_path);

        if (logfd != -1) {
                VE_IGNORE_EINTR (dup2 (logfd, 1));
                VE_IGNORE_EINTR (dup2 (logfd, 2));
                close (logfd);
        } else {
                g_warning (_("%s: Could not open log file for display %s!"),
                           "mdm_server_spawn",
                           server->priv->display_name);
        }

        /* The X server expects USR1/TTIN/TTOU to be SIG_IGN */
        ign_signal.sa_handler = SIG_IGN;
        ign_signal.sa_flags = SA_RESTART;
        sigemptyset (&ign_signal.sa_mask);

        if (sigaction (SIGUSR1, &ign_signal, NULL) < 0) {
                g_warning (_("%s: Error setting %s to %s"),
                           "mdm_server_spawn", "USR1", "SIG_IGN");
                _exit (SERVER_ABORT);
        }

        if (sigaction (SIGTTIN, &ign_signal, NULL) < 0) {
                g_warning (_("%s: Error setting %s to %s"),
                           "mdm_server_spawn", "TTIN", "SIG_IGN");
                _exit (SERVER_ABORT);
        }

        if (sigaction (SIGTTOU, &ign_signal, NULL) < 0) {
                g_warning (_("%s: Error setting %s to %s"),
                           "mdm_server_spawn", "TTOU", "SIG_IGN");
                _exit (SERVER_ABORT);
        }

        /* And HUP and TERM are at SIG_DFL from mdm_unset_signals,
           we also have an empty mask and all that fun stuff */

        /* unblock signals (especially HUP/TERM/USR1) so that we
         * can control the X server */
        sigemptyset (&mask);
        sigprocmask (SIG_SETMASK, &mask, NULL);

        /* Terminate the process when the parent dies */
#ifdef HAVE_SYS_PRCTL_H
        prctl (PR_SET_PDEATHSIG, SIGTERM);
#endif

        if (server->priv->priority != 0) {
                if (setpriority (PRIO_PROCESS, 0, server->priv->priority)) {
                        g_warning (_("%s: Server priority couldn't be set to %d: %s"),
                                   "mdm_server_spawn",
                                   server->priv->priority,
                                   g_strerror (errno));
                }
        }

        setpgid (0, 0);

        change_user (server);
}

static void
listify_hash (const char *key,
              const char *value,
              GPtrArray  *env)
{
        char *str;
        str = g_strdup_printf ("%s=%s", key, value);
        g_ptr_array_add (env, str);
}

static GPtrArray *
get_server_environment (MdmServer *server)
{
        GPtrArray  *env;
        char      **l;
        GHashTable *hash;

        env = g_ptr_array_new ();

        /* create a hash table of current environment, then update keys has necessary */
        hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
        for (l = environ; *l != NULL; l++) {
                char **str;
                str = g_strsplit (*l, "=", 2);
                g_hash_table_insert (hash, str[0], str[1]);
                g_free (str);
        }

        /* modify environment here */
        if (server->priv->is_parented) {
                if (server->priv->parent_auth_file != NULL) {
                        g_hash_table_insert (hash, g_strdup ("XAUTHORITY"), g_strdup (server->priv->parent_auth_file));
                }

                if (server->priv->parent_display_name != NULL) {
                        g_hash_table_insert (hash, g_strdup ("DISPLAY"), g_strdup (server->priv->parent_display_name));
                }
        } else {
                g_hash_table_insert (hash, g_strdup ("DISPLAY="), g_strdup (server->priv->display_name));
        }

        if (server->priv->user_name != NULL) {
                struct passwd *pwent;

                mdm_get_pwent_for_name (server->priv->user_name, &pwent);

                if (pwent->pw_dir != NULL
                    && g_file_test (pwent->pw_dir, G_FILE_TEST_EXISTS)) {
                        g_hash_table_insert (hash, g_strdup ("HOME"), g_strdup (pwent->pw_dir));
                } else {
                        /* Hack */
                        g_hash_table_insert (hash, g_strdup ("HOME"), g_strdup ("/"));
                }
                g_hash_table_insert (hash, g_strdup ("SHELL"), g_strdup (pwent->pw_shell));
                g_hash_table_remove (hash, "MAIL");
        }

        g_hash_table_foreach (hash, (GHFunc)listify_hash, env);
        g_hash_table_destroy (hash);

        g_ptr_array_add (env, NULL);

        return env;
}

static void
server_add_xserver_args (MdmServer *server,
                         int       *argc,
                         char    ***argv)
{
        int    count;
        char **args;
        int    len;
        int    i;

        len = *argc;
        g_shell_parse_argv (server->priv->session_args, &count, &args, NULL);
        *argv = g_renew (char *, *argv, len + count + 1);

        for (i=0; i < count;i++) {
                *argv[len++] = g_strdup (args[i]);
        }

        *argc += count;

        argv[len] = NULL;
        g_strfreev (args);
}

static void
server_child_watch (GPid       pid,
                    int        status,
                    MdmServer *server)
{
        g_debug ("MdmServer: child (pid:%d) done (%s:%d)",
                 (int) pid,
                 WIFEXITED (status) ? "status"
                 : WIFSIGNALED (status) ? "signal"
                 : "unknown",
                 WIFEXITED (status) ? WEXITSTATUS (status)
                 : WIFSIGNALED (status) ? WTERMSIG (status)
                 : -1);

        if (WIFEXITED (status)) {
                int code = WEXITSTATUS (status);
                g_signal_emit (server, signals [EXITED], 0, code);
        } else if (WIFSIGNALED (status)) {
                int num = WTERMSIG (status);
                g_signal_emit (server, signals [DIED], 0, num);
        }

        g_spawn_close_pid (server->priv->pid);
        server->priv->pid = -1;
}

static gboolean
mdm_server_spawn (MdmServer  *server,
                  const char *vtarg)
{
        int              argc;
        gchar          **argv = NULL;
        GError          *error;
        GPtrArray       *env;
        gboolean         ret;
        char            *freeme;

        ret = FALSE;

        /* Figure out the server command */
        argv = NULL;
        argc = 0;
        mdm_server_resolve_command_line (server,
                                         vtarg,
                                         &argc,
                                         &argv);

        if (server->priv->session_args) {
                server_add_xserver_args (server, &argc, &argv);
        }

        if (argv[0] == NULL) {
                g_warning (_("%s: Empty server command for display %s"),
                           "mdm_server_spawn",
                           server->priv->display_name);
                _exit (SERVER_ABORT);
        }

        env = get_server_environment (server);

        freeme = g_strjoinv (" ", argv);
        g_debug ("MdmServer: Starting X server process: %s", freeme);
        g_free (freeme);

        error = NULL;
        ret = g_spawn_async_with_pipes (NULL,
                                        argv,
                                        (char **)env->pdata,
                                        G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
                                        (GSpawnChildSetupFunc)server_child_setup,
                                        server,
                                        &server->priv->pid,
                                        NULL,
                                        NULL,
                                        NULL,
                                        &error);

        if (! ret) {
                g_warning ("Could not start command '%s': %s",
                           server->priv->command,
                           error->message);
                g_error_free (error);
        }

        g_strfreev (argv);
        g_ptr_array_foreach (env, (GFunc)g_free, NULL);
        g_ptr_array_free (env, TRUE);

        g_debug ("MdmServer: Started X server process %d - waiting for READY", (int)server->priv->pid);

        server->priv->child_watch_id = g_child_watch_add (server->priv->pid,
                                                          (GChildWatchFunc)server_child_watch,
                                                          server);

        return ret;
}

/**
 * mdm_server_start:
 * @disp: Pointer to a MdmDisplay structure
 *
 * Starts a local X server. Handles retries and fatal errors properly.
 */

gboolean
mdm_server_start (MdmServer *server)
{
        gboolean res;

        /* fork X server process */
        res = mdm_server_spawn (server, NULL);

        return res;
}

static void
server_died (MdmServer *server)
{
        int exit_status;

        g_debug ("MdmServer: Waiting on process %d", server->priv->pid);
        exit_status = mdm_wait_on_pid (server->priv->pid);

        if (WIFEXITED (exit_status) && (WEXITSTATUS (exit_status) != 0)) {
                g_debug ("MdmServer: Wait on child process failed");
        } else {
                /* exited normally */
        }

        g_spawn_close_pid (server->priv->pid);
        server->priv->pid = -1;

        if (server->priv->display_device != NULL) {
                g_free (server->priv->display_device);
                server->priv->display_device = NULL;
                g_object_notify (G_OBJECT (server), "display-device");
        }

        g_debug ("MdmServer: Server died");
}

gboolean
mdm_server_stop (MdmServer *server)
{
        int res;

        if (server->priv->pid <= 1) {
                return TRUE;
        }

        /* remove watch source before we can wait on child */
        if (server->priv->child_watch_id > 0) {
                g_source_remove (server->priv->child_watch_id);
                server->priv->child_watch_id = 0;
        }

        g_debug ("MdmServer: Stopping server");

        res = mdm_signal_pid (server->priv->pid, SIGTERM);
        if (res < 0) {
        } else {
                server_died (server);
        }

        return TRUE;
}


static void
_mdm_server_set_display_name (MdmServer  *server,
                              const char *name)
{
        g_free (server->priv->display_name);
        server->priv->display_name = g_strdup (name);
}

static void
_mdm_server_set_auth_file (MdmServer  *server,
                           const char *auth_file)
{
        g_free (server->priv->auth_file);
        server->priv->auth_file = g_strdup (auth_file);
}

static void
_mdm_server_set_user_name (MdmServer  *server,
                           const char *name)
{
        g_free (server->priv->user_name);
        server->priv->user_name = g_strdup (name);
}

static void
_mdm_server_set_disable_tcp (MdmServer  *server,
                             gboolean    disabled)
{
        server->priv->disable_tcp = disabled;
}

static void
mdm_server_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
        MdmServer *self;

        self = MDM_SERVER (object);

        switch (prop_id) {
        case PROP_DISPLAY_NAME:
                _mdm_server_set_display_name (self, g_value_get_string (value));
                break;
        case PROP_AUTH_FILE:
                _mdm_server_set_auth_file (self, g_value_get_string (value));
                break;
        case PROP_USER_NAME:
                _mdm_server_set_user_name (self, g_value_get_string (value));
                break;
        case PROP_DISABLE_TCP:
                _mdm_server_set_disable_tcp (self, g_value_get_boolean (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
mdm_server_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
        MdmServer *self;

        self = MDM_SERVER (object);

        switch (prop_id) {
        case PROP_DISPLAY_NAME:
                g_value_set_string (value, self->priv->display_name);
                break;
        case PROP_DISPLAY_DEVICE:
                g_value_take_string (value,
                                     mdm_server_get_display_device (self));
                break;
        case PROP_AUTH_FILE:
                g_value_set_string (value, self->priv->auth_file);
                break;
        case PROP_USER_NAME:
                g_value_set_string (value, self->priv->user_name);
                break;
        case PROP_DISABLE_TCP:
                g_value_set_boolean (value, self->priv->disable_tcp);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static GObject *
mdm_server_constructor (GType                  type,
                        guint                  n_construct_properties,
                        GObjectConstructParam *construct_properties)
{
        MdmServer      *server;

        server = MDM_SERVER (G_OBJECT_CLASS (mdm_server_parent_class)->constructor (type,
                                                                                    n_construct_properties,
                                                                                    construct_properties));
        return G_OBJECT (server);
}

static void
mdm_server_class_init (MdmServerClass *klass)
{
        GObjectClass    *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = mdm_server_get_property;
        object_class->set_property = mdm_server_set_property;
        object_class->constructor = mdm_server_constructor;
        object_class->finalize = mdm_server_finalize;

        g_type_class_add_private (klass, sizeof (MdmServerPrivate));

        signals [READY] =
                g_signal_new ("ready",
                              G_TYPE_FROM_CLASS (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (MdmServerClass, ready),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
        signals [EXITED] =
                g_signal_new ("exited",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmServerClass, exited),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__INT,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_INT);
        signals [DIED] =
                g_signal_new ("died",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmServerClass, died),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__INT,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_INT);

        g_object_class_install_property (object_class,
                                         PROP_DISPLAY_NAME,
                                         g_param_spec_string ("display-name",
                                                              "name",
                                                              "name",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
        g_object_class_install_property (object_class,
                                         PROP_DISPLAY_DEVICE,
                                         g_param_spec_string ("display-device",
                                                              "Display Device",
                                                              "Path to terminal display is running on",
                                                              NULL,
                                                              G_PARAM_READABLE));
        g_object_class_install_property (object_class,
                                         PROP_AUTH_FILE,
                                         g_param_spec_string ("auth-file",
                                                              "Authorization File",
                                                              "Path to X authorization file",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

        g_object_class_install_property (object_class,
                                         PROP_USER_NAME,
                                         g_param_spec_string ("user-name",
                                                              "user name",
                                                              "user name",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
        g_object_class_install_property (object_class,
                                         PROP_DISABLE_TCP,
                                         g_param_spec_boolean ("disable-tcp",
                                                               NULL,
                                                               NULL,
                                                               TRUE,
                                                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

}

static void
mdm_server_init (MdmServer *server)
{

        server->priv = MDM_SERVER_GET_PRIVATE (server);

        server->priv->pid = -1;
        server->priv->command = g_strdup (X_SERVER " -br -verbose");
        server->priv->log_dir = g_strdup (LOGDIR);

        add_ready_handler (server);
}

static void
mdm_server_finalize (GObject *object)
{
        MdmServer *server;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_SERVER (object));

        server = MDM_SERVER (object);

        g_return_if_fail (server->priv != NULL);

        remove_ready_handler (server);

        mdm_server_stop (server);

        g_free (server->priv->command);
        g_free (server->priv->user_name);
        g_free (server->priv->session_args);
        g_free (server->priv->log_dir);
        g_free (server->priv->display_name);
        g_free (server->priv->display_device);
        g_free (server->priv->auth_file);
        g_free (server->priv->parent_display_name);
        g_free (server->priv->parent_auth_file);
        g_free (server->priv->chosen_hostname);

        G_OBJECT_CLASS (mdm_server_parent_class)->finalize (object);
}

MdmServer *
mdm_server_new (const char *display_name,
                const char *auth_file)
{
        GObject *object;

        object = g_object_new (MDM_TYPE_SERVER,
                               "display-name", display_name,
                               "auth-file", auth_file,
                               NULL);

        return MDM_SERVER (object);
}
