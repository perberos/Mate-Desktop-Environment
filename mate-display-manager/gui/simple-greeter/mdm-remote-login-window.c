/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 William Jon McCann <jmccann@redhat.com>
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
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <glib-object.h>

#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include "mdm-remote-login-window.h"
#include "mdm-common.h"

#define MDM_REMOTE_LOGIN_WINDOW_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_REMOTE_LOGIN_WINDOW, MdmRemoteLoginWindowPrivate))

struct MdmRemoteLoginWindowPrivate
{
        gboolean connected;
        char    *hostname;
        char    *display;
        GPid     xserver_pid;
        guint    xserver_watch_id;
};

enum {
        PROP_0,
};

enum {
        DISCONNECTED,
        LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0, };

static void     mdm_remote_login_window_class_init   (MdmRemoteLoginWindowClass *klass);
static void     mdm_remote_login_window_init         (MdmRemoteLoginWindow      *remote_login_window);
static void     mdm_remote_login_window_finalize     (GObject                    *object);

G_DEFINE_TYPE (MdmRemoteLoginWindow, mdm_remote_login_window, GTK_TYPE_WINDOW)

static void
xserver_child_watch (GPid                  pid,
                     int                   status,
                     MdmRemoteLoginWindow *login_window)
{
        g_debug ("MdmRemoteLoginWindow: **** xserver (pid:%d) done (%s:%d)",
                 (int) pid,
                 WIFEXITED (status) ? "status"
                 : WIFSIGNALED (status) ? "signal"
                 : "unknown",
                 WIFEXITED (status) ? WEXITSTATUS (status)
                 : WIFSIGNALED (status) ? WTERMSIG (status)
                 : -1);

        g_spawn_close_pid (login_window->priv->xserver_pid);

        login_window->priv->xserver_pid = -1;
        login_window->priv->xserver_watch_id = 0;

        gtk_widget_destroy (GTK_WIDGET (login_window));
}

static gboolean
start_xephyr (MdmRemoteLoginWindow *login_window)
{
        GError     *local_error;
        char      **argv;
        gboolean    res;
        gboolean    ret;
        int         flags;
        char       *command;

        command = g_strdup_printf ("Xephyr -query %s -parent 0x%x -br -once %s",
                                   login_window->priv->hostname,
                                   (unsigned int)GDK_WINDOW_XID (gtk_widget_get_window (GTK_WIDGET (login_window))),
                                   login_window->priv->display);
        g_debug ("MdmRemoteLoginWindow: Running: %s", command);

        ret = FALSE;

        argv = NULL;
        local_error = NULL;
        res = g_shell_parse_argv (command, NULL, &argv, &local_error);
        if (! res) {
                g_warning ("MdmRemoteLoginWindow: Unable to parse command: %s", local_error->message);
                g_error_free (local_error);
                goto out;
        }

        flags = G_SPAWN_SEARCH_PATH
                | G_SPAWN_DO_NOT_REAP_CHILD;

        local_error = NULL;
        res = g_spawn_async (NULL,
                             argv,
                             NULL,
                             flags,
                             NULL,
                             NULL,
                             &login_window->priv->xserver_pid,
                             &local_error);
        g_strfreev (argv);

        if (! res) {
                g_warning ("MdmRemoteLoginWindow: Unable to run command %s: %s",
                           command,
                           local_error->message);
                g_error_free (local_error);
                goto out;
        }

        g_debug ("MdmRemoteLoginWindow: Started: pid=%d command='%s'",
                 login_window->priv->xserver_pid,
                 command);

        login_window->priv->xserver_watch_id = g_child_watch_add (login_window->priv->xserver_pid,
                                                                  (GChildWatchFunc)xserver_child_watch,
                                                                  login_window);
        ret = TRUE;

 out:
        g_free (command);

        return ret;
}

static gboolean
start_xdmx (MdmRemoteLoginWindow *login_window)
{
        char    *cmd;
        gboolean res;
        GError  *error;

        cmd = g_strdup_printf ("Xdmx -query %s -br -once %s",
                               login_window->priv->hostname,
                               login_window->priv->display);
        g_debug ("Running: %s", cmd);

        error = NULL;
        res = g_spawn_command_line_async (cmd, &error);

        g_free (cmd);

        if (! res) {
                g_warning ("Could not start Xdmx X server: %s", error->message);
                g_error_free (error);
                return FALSE;
        }

        return TRUE;
}

gboolean
mdm_remote_login_window_connect (MdmRemoteLoginWindow *login_window,
                                 const char           *hostname)
{
        gboolean res;
        char    *title;

        title = g_strdup_printf (_("Remote Login (Connecting to %s…)"), hostname);

        gtk_window_set_title (GTK_WINDOW (login_window), title);

        login_window->priv->hostname = g_strdup (hostname);
        login_window->priv->display = g_strdup (":300");

        if (0) {
                res = start_xdmx (login_window);
        } else {
                res = start_xephyr (login_window);
        }

        if (res) {
                title = g_strdup_printf (_("Remote Login (Connected to %s)"), hostname);
                gtk_window_set_title (GTK_WINDOW (login_window), title);
                g_free (title);
        }

        return res;
}

static void
mdm_remote_login_window_set_property (GObject      *object,
                                      guint         prop_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
mdm_remote_login_window_get_property (GObject    *object,
                                      guint       prop_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static GObject *
mdm_remote_login_window_constructor (GType                  type,
                                     guint                  n_construct_properties,
                                     GObjectConstructParam *construct_properties)
{
        MdmRemoteLoginWindow      *login_window;

        login_window = MDM_REMOTE_LOGIN_WINDOW (G_OBJECT_CLASS (mdm_remote_login_window_parent_class)->constructor (type,
                                                                                                                      n_construct_properties,
                                                                                                                      construct_properties));


        return G_OBJECT (login_window);
}

static void
mdm_remote_login_window_class_init (MdmRemoteLoginWindowClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = mdm_remote_login_window_get_property;
        object_class->set_property = mdm_remote_login_window_set_property;
        object_class->constructor = mdm_remote_login_window_constructor;
        object_class->finalize = mdm_remote_login_window_finalize;

        signals [DISCONNECTED] =
                g_signal_new ("disconnected",
                              G_TYPE_FROM_CLASS (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (MdmRemoteLoginWindowClass, disconnected),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        g_type_class_add_private (klass, sizeof (MdmRemoteLoginWindowPrivate));
}

static void
mdm_remote_login_window_init (MdmRemoteLoginWindow *login_window)
{
        login_window->priv = MDM_REMOTE_LOGIN_WINDOW_GET_PRIVATE (login_window);

        gtk_window_set_position (GTK_WINDOW (login_window), GTK_WIN_POS_CENTER_ALWAYS);
        gtk_window_set_title (GTK_WINDOW (login_window), _("Remote Login"));
        gtk_window_set_decorated (GTK_WINDOW (login_window), FALSE);
        gtk_window_set_skip_taskbar_hint (GTK_WINDOW (login_window), TRUE);
        gtk_window_set_skip_pager_hint (GTK_WINDOW (login_window), TRUE);
        gtk_window_stick (GTK_WINDOW (login_window));
        gtk_window_maximize (GTK_WINDOW (login_window));
        gtk_window_set_icon_name (GTK_WINDOW (login_window), "computer");
}

static void
mdm_remote_login_window_finalize (GObject *object)
{
        MdmRemoteLoginWindow *login_window;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_REMOTE_LOGIN_WINDOW (object));

        login_window = MDM_REMOTE_LOGIN_WINDOW (object);

        g_return_if_fail (login_window->priv != NULL);

        G_OBJECT_CLASS (mdm_remote_login_window_parent_class)->finalize (object);
}

GtkWidget *
mdm_remote_login_window_new (gboolean is_local)
{
        GObject *object;

        object = g_object_new (MDM_TYPE_REMOTE_LOGIN_WINDOW,
                               NULL);

        return GTK_WIDGET (object);
}
