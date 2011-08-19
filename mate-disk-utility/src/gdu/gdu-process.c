/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-process.c
 *
 * Copyright (C) 2007 David Zeuthen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include <stdlib.h>
#include <string.h>
#include <dbus/dbus-glib.h>
#include <time.h>
#include <gio/gdesktopappinfo.h>

#include "gdu-private.h"
#include "gdu-process.h"

struct _GduProcessPrivate {
        pid_t pid;
        uid_t uid;
        char *command_line;
};

static GObjectClass *parent_class = NULL;

G_DEFINE_TYPE (GduProcess, gdu_process, G_TYPE_OBJECT);

static void
gdu_process_finalize (GduProcess *process)
{
        g_free (process->priv->command_line);
        if (G_OBJECT_CLASS (parent_class)->finalize)
                (* G_OBJECT_CLASS (parent_class)->finalize) (G_OBJECT (process));
}

static void
gdu_process_class_init (GduProcessClass *klass)
{
        GObjectClass *obj_class = (GObjectClass *) klass;

        parent_class = g_type_class_peek_parent (klass);

        obj_class->finalize = (GObjectFinalizeFunc) gdu_process_finalize;

        g_type_class_add_private (klass, sizeof (GduProcessPrivate));
}

static void
gdu_process_init (GduProcess *process)
{
        process->priv = G_TYPE_INSTANCE_GET_PRIVATE (process, GDU_TYPE_PROCESS, GduProcessPrivate);
}

pid_t
gdu_process_get_id (GduProcess *process)
{
        return process->priv->pid;
}

uid_t
gdu_process_get_owner (GduProcess *process)
{
        return process->priv->uid;
}

const char *
gdu_process_get_command_line (GduProcess *process)
{
        return process->priv->command_line;
}

GAppInfo *
gdu_process_get_app_info (GduProcess *process)
{
        GDesktopAppInfo *app_info;
        char *filename;
        char *basename;
        const char *exe;
        const char *exe_basename;
        GDir *dir;
        int n;
        char *s;

        app_info = NULL;
        for (n = 0; process->priv->command_line[n] != '\0'; n++) {
                if (process->priv->command_line[n] == ' ')
                        break;
        }
        s = g_strndup (process->priv->command_line, n);
        basename = g_path_get_basename (s);
        g_free (s);

        /* First try the name */
        filename = g_strdup_printf ("/usr/share/applications/%s.desktop", basename);
        app_info = g_desktop_app_info_new_from_filename (filename);
        g_free (filename);
        if (app_info != NULL) {
                exe = g_app_info_get_executable (G_APP_INFO (app_info));
                exe_basename = exe == NULL ? NULL : strrchr (exe, '/');
                if (exe_basename == NULL)
                        exe_basename = exe;
                if (exe_basename != NULL && strcmp (exe_basename, basename) == 0)
                        goto found;
                g_object_unref (app_info);
                app_info = NULL;
        }

        /* Some vendors (like Fedora) tend to prefix with 'mate-'. Stupid. */
        filename = g_strdup_printf ("/usr/share/applications/mate-%s.desktop", basename);
        app_info = g_desktop_app_info_new_from_filename (filename);
        g_free (filename);
        if (app_info != NULL) {
                exe = g_app_info_get_executable (G_APP_INFO (app_info));
                exe_basename = exe == NULL ? NULL : strrchr (exe, '/');
                if (exe_basename == NULL)
                        exe_basename = exe;
                if (exe_basename != NULL && strcmp (exe_basename, basename) == 0)
                        goto found;
                g_object_unref (app_info);
                app_info = NULL;
        }

        dir = g_dir_open ("/usr/share/applications", 0, NULL);
        if (dir != NULL) {
                const char *name;
                while ((name = g_dir_read_name (dir)) != NULL && app_info == NULL) {

                        filename = g_strdup_printf ("/usr/share/applications/%s", name);
                        app_info = g_desktop_app_info_new_from_filename (filename);
                        g_free (filename);

                        if (app_info != NULL) {
                                exe = g_app_info_get_executable (G_APP_INFO (app_info));
                                exe_basename = exe == NULL ? NULL : strrchr (exe, '/');
                                if (exe_basename == NULL)
                                        exe_basename = exe;
                                if (exe_basename != NULL && strcmp (exe_basename, basename) == 0) {
                                        /* gotcha! */
                                } else {
                                        g_object_unref (app_info);
                                        app_info = NULL;
                                }
                        }
                }
                g_dir_close (dir);
        }

found:
        g_free (basename);

        if (app_info != NULL)
                return G_APP_INFO (app_info);
        else
                return NULL;
}

GduProcess *
_gdu_process_new (gpointer data)
{
        GduProcess *process;
        GValue elem0 = {0};

        process = GDU_PROCESS (g_object_new (GDU_TYPE_PROCESS, NULL));

        g_value_init (&elem0, PROCESS_STRUCT_TYPE);
        g_value_set_static_boxed (&elem0, data);
        dbus_g_type_struct_get (&elem0,
                                0, &(process->priv->pid),
                                1, &(process->priv->uid),
                                2, &(process->priv->command_line),
                                G_MAXUINT);
        return process;
}
