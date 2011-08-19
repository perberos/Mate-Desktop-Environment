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

#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include <locale.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <mateconf/mateconf-client.h>

#include "mate-settings-profile.h"
#include "gsd-typing-break-manager.h"

#define GSD_TYPING_BREAK_MANAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GSD_TYPE_TYPING_BREAK_MANAGER, GsdTypingBreakManagerPrivate))

#define MATECONF_BREAK_DIR           "/desktop/mate/typing_break"

struct GsdTypingBreakManagerPrivate
{
        GPid  typing_monitor_pid;
        guint typing_monitor_idle_id;
        guint child_watch_id;
        guint setup_id;
        guint notify;
};

static void     gsd_typing_break_manager_class_init  (GsdTypingBreakManagerClass *klass);
static void     gsd_typing_break_manager_init        (GsdTypingBreakManager      *typing_break_manager);
static void     gsd_typing_break_manager_finalize    (GObject             *object);

G_DEFINE_TYPE (GsdTypingBreakManager, gsd_typing_break_manager, G_TYPE_OBJECT)

static gpointer manager_object = NULL;

static gboolean
typing_break_timeout (GsdTypingBreakManager *manager)
{
        if (manager->priv->typing_monitor_pid > 0) {
                kill (manager->priv->typing_monitor_pid, SIGKILL);
        }

        manager->priv->typing_monitor_idle_id = 0;

        return FALSE;
}

static void
child_watch (GPid                   pid,
             int                    status,
             GsdTypingBreakManager *manager)
{
        if (pid == manager->priv->typing_monitor_pid) {
                manager->priv->typing_monitor_pid = 0;
                g_spawn_close_pid (pid);
        }
}

static void
setup_typing_break (GsdTypingBreakManager *manager,
                    gboolean               enabled)
{
        mate_settings_profile_start (NULL);

        if (! enabled) {
                if (manager->priv->typing_monitor_pid != 0) {
                        manager->priv->typing_monitor_idle_id = g_timeout_add_seconds (3, (GSourceFunc) typing_break_timeout, manager);
                }
                return;
        }

        if (manager->priv->typing_monitor_idle_id != 0) {
                g_source_remove (manager->priv->typing_monitor_idle_id);
                manager->priv->typing_monitor_idle_id = 0;
        }

        if (manager->priv->typing_monitor_pid == 0) {
                GError  *error;
                char    *argv[] = { "mate-typing-monitor", "-n", NULL };
                gboolean res;

                error = NULL;
                res = g_spawn_async ("/",
                                     argv,
                                     NULL,
                                     G_SPAWN_STDOUT_TO_DEV_NULL
                                     | G_SPAWN_STDERR_TO_DEV_NULL
                                     | G_SPAWN_SEARCH_PATH
                                     | G_SPAWN_DO_NOT_REAP_CHILD,
                                     NULL,
                                     NULL,
                                     &manager->priv->typing_monitor_pid,
                                     &error);
                if (! res) {
                        /* FIXME: put up a warning */
                        g_warning ("failed: %s\n", error->message);
                        g_error_free (error);
                        manager->priv->typing_monitor_pid = 0;
                        return;
                }

                manager->priv->child_watch_id = g_child_watch_add (manager->priv->typing_monitor_pid,
                                                                   (GChildWatchFunc)child_watch,
                                                                   manager);
        }

        mate_settings_profile_end (NULL);
}

static void
typing_break_callback (MateConfClient           *client,
                       guint                  cnxn_id,
                       MateConfEntry            *entry,
                       GsdTypingBreakManager *manager)
{
        if (! strcmp (entry->key, "/desktop/mate/typing_break/enabled")) {
                if (entry->value->type == MATECONF_VALUE_BOOL) {
                        setup_typing_break (manager, mateconf_value_get_bool (entry->value));
                }
        }
}

static gboolean
really_setup_typing_break (GsdTypingBreakManager *manager)
{
        setup_typing_break (manager, TRUE);
        manager->priv->setup_id = 0;
        return FALSE;
}

gboolean
gsd_typing_break_manager_start (GsdTypingBreakManager *manager,
                                GError               **error)
{
        MateConfClient *client;
        gboolean     enabled;

        g_debug ("Starting typing_break manager");
        mate_settings_profile_start (NULL);

        client = mateconf_client_get_default ();

        mateconf_client_add_dir (client, MATECONF_BREAK_DIR, MATECONF_CLIENT_PRELOAD_ONELEVEL, NULL);
        manager->priv->notify =
                mateconf_client_notify_add (client,
                                         MATECONF_BREAK_DIR,
                                         (MateConfClientNotifyFunc) typing_break_callback, manager,
                                         NULL, NULL);

        enabled = mateconf_client_get_bool (client, MATECONF_BREAK_DIR "/enabled", NULL);
        g_object_unref (client);
        if (enabled) {
                manager->priv->setup_id =
                        g_timeout_add_seconds (3,
                                               (GSourceFunc) really_setup_typing_break,
                                               manager);
        }

        mate_settings_profile_end (NULL);

        return TRUE;
}

void
gsd_typing_break_manager_stop (GsdTypingBreakManager *manager)
{
        GsdTypingBreakManagerPrivate *p = manager->priv;

        g_debug ("Stopping typing_break manager");

        if (p->setup_id != 0) {
                g_source_remove (p->setup_id);
                p->setup_id = 0;
        }

        if (p->child_watch_id != 0) {
                g_source_remove (p->child_watch_id);
                p->child_watch_id = 0;
        }

        if (p->typing_monitor_idle_id != 0) {
                g_source_remove (p->typing_monitor_idle_id);
                p->typing_monitor_idle_id = 0;
        }

        if (p->typing_monitor_pid > 0) {
                kill (p->typing_monitor_pid, SIGKILL);
                g_spawn_close_pid (p->typing_monitor_pid);
                p->typing_monitor_pid = 0;
        }

        if (p->notify != 0) {
                MateConfClient *client = mateconf_client_get_default ();
                mateconf_client_remove_dir (client, MATECONF_BREAK_DIR, NULL);
                mateconf_client_notify_remove (client, p->notify);
                g_object_unref (client);
                p->notify = 0;
        }
}

static void
gsd_typing_break_manager_set_property (GObject        *object,
                               guint           prop_id,
                               const GValue   *value,
                               GParamSpec     *pspec)
{
        GsdTypingBreakManager *self;

        self = GSD_TYPING_BREAK_MANAGER (object);

        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gsd_typing_break_manager_get_property (GObject        *object,
                               guint           prop_id,
                               GValue         *value,
                               GParamSpec     *pspec)
{
        GsdTypingBreakManager *self;

        self = GSD_TYPING_BREAK_MANAGER (object);

        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static GObject *
gsd_typing_break_manager_constructor (GType                  type,
                              guint                  n_construct_properties,
                              GObjectConstructParam *construct_properties)
{
        GsdTypingBreakManager      *typing_break_manager;
        GsdTypingBreakManagerClass *klass;

        klass = GSD_TYPING_BREAK_MANAGER_CLASS (g_type_class_peek (GSD_TYPE_TYPING_BREAK_MANAGER));

        typing_break_manager = GSD_TYPING_BREAK_MANAGER (G_OBJECT_CLASS (gsd_typing_break_manager_parent_class)->constructor (type,
                                                                                                      n_construct_properties,
                                                                                                      construct_properties));

        return G_OBJECT (typing_break_manager);
}

static void
gsd_typing_break_manager_dispose (GObject *object)
{
        GsdTypingBreakManager *typing_break_manager;

        typing_break_manager = GSD_TYPING_BREAK_MANAGER (object);

        G_OBJECT_CLASS (gsd_typing_break_manager_parent_class)->dispose (object);
}

static void
gsd_typing_break_manager_class_init (GsdTypingBreakManagerClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = gsd_typing_break_manager_get_property;
        object_class->set_property = gsd_typing_break_manager_set_property;
        object_class->constructor = gsd_typing_break_manager_constructor;
        object_class->dispose = gsd_typing_break_manager_dispose;
        object_class->finalize = gsd_typing_break_manager_finalize;

        g_type_class_add_private (klass, sizeof (GsdTypingBreakManagerPrivate));
}

static void
gsd_typing_break_manager_init (GsdTypingBreakManager *manager)
{
        manager->priv = GSD_TYPING_BREAK_MANAGER_GET_PRIVATE (manager);

}

static void
gsd_typing_break_manager_finalize (GObject *object)
{
        GsdTypingBreakManager *typing_break_manager;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GSD_IS_TYPING_BREAK_MANAGER (object));

        typing_break_manager = GSD_TYPING_BREAK_MANAGER (object);

        g_return_if_fail (typing_break_manager->priv != NULL);

        G_OBJECT_CLASS (gsd_typing_break_manager_parent_class)->finalize (object);
}

GsdTypingBreakManager *
gsd_typing_break_manager_new (void)
{
        if (manager_object != NULL) {
                g_object_ref (manager_object);
        } else {
                manager_object = g_object_new (GSD_TYPE_TYPING_BREAK_MANAGER, NULL);
                g_object_add_weak_pointer (manager_object,
                                           (gpointer *) &manager_object);
        }

        return GSD_TYPING_BREAK_MANAGER (manager_object);
}
