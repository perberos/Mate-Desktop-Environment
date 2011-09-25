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
#include <signal.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <glib-object.h>

#include "mdm-common.h"

#include "mdm-slave-proxy.h"

#define MDM_SLAVE_PROXY_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_SLAVE_PROXY, MdmSlaveProxyPrivate))

#define MAX_LOGS 5

struct MdmSlaveProxyPrivate
{
        char    *command;
        char    *log_path;
        GPid     pid;
        guint    child_watch_id;
};

enum {
        PROP_0,
        PROP_COMMAND,
        PROP_LOG_PATH,
};

enum {
        EXITED,
        DIED,
        LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0, };

static void     mdm_slave_proxy_class_init      (MdmSlaveProxyClass *klass);
static void     mdm_slave_proxy_init            (MdmSlaveProxy      *slave);
static void     mdm_slave_proxy_finalize        (GObject            *object);

G_DEFINE_TYPE (MdmSlaveProxy, mdm_slave_proxy, G_TYPE_OBJECT)

static void
child_watch (GPid           pid,
             int            status,
             MdmSlaveProxy *slave)
{
        g_debug ("MdmSlaveProxy: slave (pid:%d) done (%s:%d)",
                 (int) pid,
                 WIFEXITED (status) ? "status"
                 : WIFSIGNALED (status) ? "signal"
                 : "unknown",
                 WIFEXITED (status) ? WEXITSTATUS (status)
                 : WIFSIGNALED (status) ? WTERMSIG (status)
                 : -1);

        g_spawn_close_pid (slave->priv->pid);
        slave->priv->pid = -1;
        slave->priv->child_watch_id = 0;

        if (WIFEXITED (status)) {
                int code = WEXITSTATUS (status);
                g_signal_emit (slave, signals [EXITED], 0, code);
        } else if (WIFSIGNALED (status)) {
                int num = WTERMSIG (status);
                g_signal_emit (slave, signals [DIED], 0, num);
        }
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

typedef struct {
        const char *log_file;
} SpawnChildData;

static void
spawn_child_setup (SpawnChildData *data)
{

        if (data->log_file != NULL) {
                int logfd;

                rotate_logs (data->log_file, MAX_LOGS);

                VE_IGNORE_EINTR (g_unlink (data->log_file));
                VE_IGNORE_EINTR (logfd = open (data->log_file, O_CREAT|O_APPEND|O_TRUNC|O_WRONLY|O_EXCL, 0644));

                if (logfd != -1) {
                        VE_IGNORE_EINTR (dup2 (logfd, 1));
                        VE_IGNORE_EINTR (dup2 (logfd, 2));
                        close (logfd);
                }
        }
}

static gboolean
spawn_command_line_async (const char *command_line,
                          const char *log_file,
                          char      **env,
                          GPid       *child_pid,
                          GError    **error)
{
        char           **argv;
        GError          *local_error;
        gboolean         ret;
        gboolean         res;
        SpawnChildData   data;

        ret = FALSE;

        argv = NULL;
        local_error = NULL;
        if (! g_shell_parse_argv (command_line, NULL, &argv, &local_error)) {
                g_warning ("Could not parse command: %s", local_error->message);
                g_propagate_error (error, local_error);
                goto out;
        }

        data.log_file = log_file;

        local_error = NULL;
        res = g_spawn_async (NULL,
                             argv,
                             env,
                             G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
                             (GSpawnChildSetupFunc)spawn_child_setup,
                             &data,
                             child_pid,
                             &local_error);

        if (! res) {
                g_warning ("Could not spawn command: %s", local_error->message);
                g_propagate_error (error, local_error);
                goto out;
        }

        ret = TRUE;
 out:
        g_strfreev (argv);

        return ret;
}

static gboolean
spawn_slave (MdmSlaveProxy *slave)
{
        gboolean    result;
        GError     *error;

        g_debug ("MdmSlaveProxy: Running command: %s", slave->priv->command);

        error = NULL;
        result = spawn_command_line_async (slave->priv->command,
                                           slave->priv->log_path,
                                           NULL,
                                           &slave->priv->pid,
                                           &error);
        if (! result) {
                g_warning ("Could not start command '%s': %s", slave->priv->command, error->message);
                g_error_free (error);
                goto out;
        }

        g_debug ("MdmSlaveProxy: Started slave with pid %d", slave->priv->pid);

        slave->priv->child_watch_id = g_child_watch_add (slave->priv->pid,
                                                         (GChildWatchFunc)child_watch,
                                                         slave);

        result = TRUE;

 out:

        return result;
}

static void
kill_slave (MdmSlaveProxy *slave)
{
        int exit_status;
        int res;

        if (slave->priv->pid <= 1) {
                return;
        }

        res = mdm_signal_pid (slave->priv->pid, SIGTERM);
        if (res < 0) {
                g_warning ("Unable to kill slave process");
        } else {
                exit_status = mdm_wait_on_pid (slave->priv->pid);
                g_spawn_close_pid (slave->priv->pid);
                slave->priv->pid = 0;
        }
}

gboolean
mdm_slave_proxy_start (MdmSlaveProxy *slave)
{
        spawn_slave (slave);

        return TRUE;
}

gboolean
mdm_slave_proxy_stop (MdmSlaveProxy *slave)
{
        g_debug ("MdmSlaveProxy: Killing slave");

        if (slave->priv->child_watch_id > 0) {
                g_source_remove (slave->priv->child_watch_id);
                slave->priv->child_watch_id = 0;
        }

        kill_slave (slave);

        return TRUE;
}

void
mdm_slave_proxy_set_command (MdmSlaveProxy *slave,
                             const char    *command)
{
        g_free (slave->priv->command);
        slave->priv->command = g_strdup (command);
}

void
mdm_slave_proxy_set_log_path (MdmSlaveProxy *slave,
                              const char    *path)
{
        g_free (slave->priv->log_path);
        slave->priv->log_path = g_strdup (path);
}

static void
mdm_slave_proxy_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
        MdmSlaveProxy *self;

        self = MDM_SLAVE_PROXY (object);

        switch (prop_id) {
        case PROP_COMMAND:
                mdm_slave_proxy_set_command (self, g_value_get_string (value));
                break;
        case PROP_LOG_PATH:
                mdm_slave_proxy_set_log_path (self, g_value_get_string (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
mdm_slave_proxy_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
        MdmSlaveProxy *self;

        self = MDM_SLAVE_PROXY (object);

        switch (prop_id) {
        case PROP_COMMAND:
                g_value_set_string (value, self->priv->command);
                break;
        case PROP_LOG_PATH:
                g_value_set_string (value, self->priv->log_path);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
mdm_slave_proxy_dispose (GObject *object)
{
        MdmSlaveProxy *slave;

        slave = MDM_SLAVE_PROXY (object);

        g_debug ("MdmSlaveProxy: Disposing slave proxy");
        mdm_slave_proxy_stop (slave);

        G_OBJECT_CLASS (mdm_slave_proxy_parent_class)->dispose (object);
}

static void
mdm_slave_proxy_class_init (MdmSlaveProxyClass *klass)
{
        GObjectClass    *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = mdm_slave_proxy_get_property;
        object_class->set_property = mdm_slave_proxy_set_property;
        object_class->dispose = mdm_slave_proxy_dispose;
        object_class->finalize = mdm_slave_proxy_finalize;

        g_type_class_add_private (klass, sizeof (MdmSlaveProxyPrivate));

        g_object_class_install_property (object_class,
                                         PROP_COMMAND,
                                         g_param_spec_string ("command",
                                                              "command",
                                                              "command",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
        g_object_class_install_property (object_class,
                                         PROP_LOG_PATH,
                                         g_param_spec_string ("log-path",
                                                              "log path",
                                                              "log path",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

        signals [EXITED] =
                g_signal_new ("exited",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmSlaveProxyClass, exited),
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
                              G_STRUCT_OFFSET (MdmSlaveProxyClass, died),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__INT,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_INT);
}

static void
mdm_slave_proxy_init (MdmSlaveProxy *slave)
{

        slave->priv = MDM_SLAVE_PROXY_GET_PRIVATE (slave);

        slave->priv->pid = -1;
}

static void
mdm_slave_proxy_finalize (GObject *object)
{
        MdmSlaveProxy *slave;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_SLAVE_PROXY (object));

        slave = MDM_SLAVE_PROXY (object);

        g_return_if_fail (slave->priv != NULL);

        g_free (slave->priv->command);
        g_free (slave->priv->log_path);

        G_OBJECT_CLASS (mdm_slave_proxy_parent_class)->finalize (object);
}

MdmSlaveProxy *
mdm_slave_proxy_new (void)
{
        GObject *object;

        object = g_object_new (MDM_TYPE_SLAVE_PROXY,
                               NULL);

        return MDM_SLAVE_PROXY (object);
}
