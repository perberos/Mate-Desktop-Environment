/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-main.c
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
#include <glib/gi18n.h>

#include <glib-object.h>
#include <string.h>
#include <stdlib.h>
#include <unique/unique.h>

#include "gdu-shell.h"

static gboolean
show_volume (GduShell   *shell,
             const char *device_file)
{
        GduPool *pool;
        GduDevice *device;
        GduPresentable *presentable;

        presentable = NULL;
        pool = gdu_shell_get_pool_for_selected_presentable (shell);
        if (pool != NULL) {
                device = gdu_pool_get_by_device_file (pool, device_file);
                if (device != NULL) {
                        presentable = gdu_pool_get_volume_by_device (pool, device);
                        g_object_unref (device);
                }
                if (presentable != NULL) {
                        gdu_shell_select_presentable (shell, presentable);
                        g_object_unref (presentable);
                        return TRUE;
                }
        }

        return FALSE;
}

static gboolean
show_drive (GduShell   *shell,
            const char *device_file)
{
        GduPool *pool;
        GduDevice *device;
        GduPresentable *presentable;

        presentable = NULL;
        pool = gdu_shell_get_pool_for_selected_presentable (shell);
        if (pool != NULL) {
                device = gdu_pool_get_by_device_file (pool, device_file);
                if (device != NULL) {
                        presentable = gdu_pool_get_drive_by_device (pool, device);
                        g_object_unref (device);
                }
                if (presentable != NULL) {
                        gdu_shell_select_presentable (shell, presentable);
                        g_object_unref (presentable);
                        return TRUE;
                }
        }

        return FALSE;
}

enum {
        CMD_PRESENT_WINDOW = 1,
        CMD_SHOW_VOLUME,
        CMD_SHOW_DRIVE
};

static UniqueResponse
message_received (UniqueApp         *app,
                  gint               command,
                  UniqueMessageData *message_data,
                  guint              timestamp,
                  GduShell          *shell)
{
        gchar *data;

        switch (command) {
        case CMD_PRESENT_WINDOW:
                gtk_window_present (GTK_WINDOW (gdu_shell_get_toplevel (shell)));
                return UNIQUE_RESPONSE_OK;
        case CMD_SHOW_VOLUME:
                gtk_window_present (GTK_WINDOW (gdu_shell_get_toplevel (shell)));
                data = unique_message_data_get_text (message_data);
                if (show_volume (shell, data))
                        return UNIQUE_RESPONSE_OK;
                else
                        return UNIQUE_RESPONSE_FAIL;
        case CMD_SHOW_DRIVE:
                gtk_window_present (GTK_WINDOW (gdu_shell_get_toplevel (shell)));
                data = unique_message_data_get_text (message_data);
                if (show_drive (shell, data))
                        return UNIQUE_RESPONSE_OK;
                else
                        return UNIQUE_RESPONSE_FAIL;
        default:
                return UNIQUE_RESPONSE_PASSTHROUGH;
        }
}

const char *volume_to_show = NULL;
const char *drive_to_show = NULL;
const char *dbus_address = NULL;

static GOptionEntry entries[] = {
        { "show-volume", 0, 0, G_OPTION_ARG_FILENAME, &volume_to_show, N_("Volume to show"), N_("DEVICE") },
        { "show-drive", 0, 0, G_OPTION_ARG_FILENAME, &drive_to_show, N_("Drive to show"), N_("DEVICE") },
        { "address", 'a', 0, G_OPTION_ARG_STRING, &dbus_address, "D-Bus address to connect to", NULL },
        { NULL }
};

int
main (int argc, char **argv)
{
        gint ret;
        GduShell *shell;
        UniqueApp *unique_app;
        UniqueMessageData *msg_data;
        UniqueResponse response;
        GError *error;

        ret = 1;
        shell = NULL;

        error = NULL;
        if (!gtk_init_with_args (&argc, &argv, "", entries, GETTEXT_PACKAGE, &error)) {
                g_printerr ("%s\n", error->message);
                g_error_free (error);
                goto out;
        }

        bindtextdomain (GETTEXT_PACKAGE, MATELOCALEDIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);

        gtk_window_set_default_icon_name ("palimpsest");

        unique_app = unique_app_new_with_commands ("org.mate.Palimpsest",
                                                   NULL,
                                                   "present_window", CMD_PRESENT_WINDOW,
                                                   "show_volume", CMD_SHOW_VOLUME,
                                                   "show_drive", CMD_SHOW_DRIVE,
                                                   NULL);

        if (unique_app_is_running (unique_app)) {
                msg_data = unique_message_data_new ();
                if (volume_to_show) {
                        unique_message_data_set_text (msg_data, volume_to_show, -1);
                        response = unique_app_send_message (unique_app, CMD_SHOW_VOLUME, msg_data);
                }
                else if (drive_to_show) {
                        unique_message_data_set_text (msg_data, drive_to_show, -1);
                        response = unique_app_send_message (unique_app, CMD_SHOW_DRIVE, msg_data);
                }
                else {
                        response = unique_app_send_message (unique_app, CMD_PRESENT_WINDOW, NULL);
                }
                unique_message_data_free (msg_data);
                if (response == UNIQUE_RESPONSE_OK)
                        return 0;
                else
                        return 1;
        }

        shell = gdu_shell_new (dbus_address);

        g_signal_connect (unique_app, "message-received",
                          G_CALLBACK (message_received), shell);

        gtk_widget_show_all (gdu_shell_get_toplevel (shell));
        gdu_shell_update (shell);

        if (volume_to_show) {
                if (!show_volume (shell, volume_to_show))
                        goto out;
        }  else if (drive_to_show) {
                if (!show_drive (shell, drive_to_show))
                        goto out;
        }

        gtk_main ();

        ret = 0;

 out:
        if (shell != NULL)
                g_object_unref (shell);
        return ret;
}
