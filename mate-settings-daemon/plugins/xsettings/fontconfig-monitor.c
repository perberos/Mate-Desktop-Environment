/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Red Hat, Inc.
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
 * Author:  Behdad Esfahbod, Red Hat, Inc.
 */

#include "fontconfig-monitor.h"

#include <gio/gio.h>
#include <fontconfig/fontconfig.h>

#define TIMEOUT_SECONDS 2

static void
stuff_changed (GFileMonitor *monitor,
               GFile *file,
               GFile *other_file,
               GFileMonitorEvent event_type,
               gpointer handle);

void
fontconfig_cache_init (void)
{
        FcInit ();
}

gboolean
fontconfig_cache_update (void)
{
        return !FcConfigUptoDate (NULL) && FcInitReinitialize ();
}

static void
monitor_files (GPtrArray *monitors,
               FcStrList *list,
               gpointer   data)
{
        const char *str;

        while ((str = (const char *) FcStrListNext (list))) {
                GFile *file;
                GFileMonitor *monitor;

                file = g_file_new_for_path (str);

                monitor = g_file_monitor (file, G_FILE_MONITOR_NONE, NULL, NULL);

                g_object_unref (file);

                if (!monitor)
                        continue;

                g_signal_connect (monitor, "changed", G_CALLBACK (stuff_changed), data);

                g_ptr_array_add (monitors, monitor);
        }

        FcStrListDone (list);
}


struct _fontconfig_monitor_handle {
        GPtrArray *monitors;

        guint timeout;

        GFunc    notify_callback;
        gpointer notify_data;
};

static GPtrArray *
monitors_create (gpointer data)
{
        GPtrArray *monitors = g_ptr_array_new ();

        monitor_files (monitors, FcConfigGetConfigFiles (NULL), data);
        monitor_files (monitors, FcConfigGetFontDirs (NULL)   , data);

        return monitors;
}

static void
monitors_free (GPtrArray *monitors)
{
        if (!monitors)
                return;

        g_ptr_array_foreach (monitors, (GFunc) g_object_unref, NULL);
        g_ptr_array_free (monitors, TRUE);
}

static gboolean
update (gpointer data)
{
        fontconfig_monitor_handle_t *handle = data;
        gboolean notify = FALSE;

        handle->timeout = 0;

        if (fontconfig_cache_update ()) {
                notify = TRUE;
                monitors_free (handle->monitors);
                handle->monitors = monitors_create (data);
        }

        /* we finish modifying handle before calling the notify callback,
         * allowing the callback to free the monitor if it decides to. */

        if (notify && handle->notify_callback)
                handle->notify_callback (data, handle->notify_data);

        return FALSE;
}

static void
stuff_changed (GFileMonitor *monitor G_GNUC_UNUSED,
               GFile *file G_GNUC_UNUSED,
               GFile *other_file G_GNUC_UNUSED,
               GFileMonitorEvent event_type G_GNUC_UNUSED,
               gpointer data)
{
        fontconfig_monitor_handle_t *handle = data;

        /* wait for quiescence */
        if (handle->timeout)
                g_source_remove (handle->timeout);

        handle->timeout = g_timeout_add_seconds (TIMEOUT_SECONDS, update, data);
}


fontconfig_monitor_handle_t *
fontconfig_monitor_start (GFunc    notify_callback,
                          gpointer notify_data)
{
        fontconfig_monitor_handle_t *handle = g_slice_new0 (fontconfig_monitor_handle_t);

        handle->notify_callback = notify_callback;
        handle->notify_data = notify_data;
        handle->monitors = monitors_create (handle);

        return handle;
}

void
fontconfig_monitor_stop  (fontconfig_monitor_handle_t *handle)
{
        if (handle->timeout)
          g_source_remove (handle->timeout);
        handle->timeout = 0;

        monitors_free (handle->monitors);
        handle->monitors = NULL;
}

#ifdef FONTCONFIG_MONITOR_TEST
static void
yay (void)
{
        g_message ("yay");
}

int
main (void)
{
        GMainLoop *loop;

        g_type_init ();

        fontconfig_monitor_start ((GFunc) yay, NULL);

        loop = g_main_loop_new (NULL, TRUE);
        g_main_loop_run (loop);

        return 0;
}
#endif
