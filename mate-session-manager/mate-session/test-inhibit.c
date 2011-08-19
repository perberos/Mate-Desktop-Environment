/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <dbus/dbus-glib.h>

#define SM_DBUS_NAME      "org.mate.SessionManager"
#define SM_DBUS_PATH      "/org/mate/SessionManager"
#define SM_DBUS_INTERFACE "org.mate.SessionManager"

static DBusGConnection *bus_connection = NULL;
static DBusGProxy      *sm_proxy = NULL;
static guint            cookie = 0;

static gboolean
session_manager_connect (void)
{

        if (bus_connection == NULL) {
                GError *error;

                error = NULL;
                bus_connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
                if (bus_connection == NULL) {
                        g_message ("Failed to connect to the session bus: %s",
                                   error->message);
                        g_error_free (error);
                        exit (1);
                }
        }

        sm_proxy = dbus_g_proxy_new_for_name (bus_connection,
                                              SM_DBUS_NAME,
                                              SM_DBUS_PATH,
                                              SM_DBUS_INTERFACE);
        return (sm_proxy != NULL);
}

typedef enum {
        GSM_INHIBITOR_FLAG_LOGOUT      = 1 << 0,
        GSM_INHIBITOR_FLAG_SWITCH_USER = 1 << 1,
        GSM_INHIBITOR_FLAG_SUSPEND     = 1 << 2
} GsmInhibitFlag;

static gboolean
do_inhibit_for_window (GdkWindow *window)
{
        GError     *error;
        gboolean    res;
        const char *startup_id;
        const char *app_id;
        const char *reason;
        guint       toplevel_xid;
        guint       flags;

        startup_id = g_getenv ("DESKTOP_AUTOSTART_ID");
#if 1
        app_id = "caja-cd-burner";
        reason = "A CD burn is in progress.";
#else
        app_id = "caja";
        reason = "A file transfer is in progress.";
#endif
        toplevel_xid = GDK_DRAWABLE_XID (window);
        flags = GSM_INHIBITOR_FLAG_LOGOUT
                | GSM_INHIBITOR_FLAG_SWITCH_USER
                | GSM_INHIBITOR_FLAG_SUSPEND;

        error = NULL;
        res = dbus_g_proxy_call (sm_proxy,
                                 "Inhibit",
                                 &error,
                                 G_TYPE_STRING, app_id,
                                 G_TYPE_UINT, toplevel_xid,
                                 G_TYPE_STRING, reason,
                                 G_TYPE_UINT, flags,
                                 G_TYPE_INVALID,
                                 G_TYPE_UINT, &cookie,
                                 G_TYPE_INVALID);
        if (! res) {
                g_warning ("Failed to inhibit: %s", error->message);
                g_error_free (error);
                return FALSE;
        }

        g_debug ("Inhibiting session manager: %u", cookie);

        return TRUE;
}
static gboolean
session_manager_disconnect (void)
{
        if (sm_proxy != NULL) {
                g_object_unref (sm_proxy);
                sm_proxy = NULL;
        }

        return TRUE;
}

static gboolean
do_uninhibit (void)
{
        GError  *error;
        gboolean res;

        error = NULL;
        res = dbus_g_proxy_call (sm_proxy,
                                 "Uninhibit",
                                 &error,
                                 G_TYPE_UINT, cookie,
                                 G_TYPE_INVALID,
                                 G_TYPE_INVALID);
        if (! res) {
                g_warning ("Failed to uninhibit: %s", error->message);
                g_error_free (error);
                return FALSE;
        }

        cookie = 0;

        return TRUE;
}

static void
on_widget_show (GtkWidget *dialog,
                gpointer   data)
{
        gboolean res;

        res = do_inhibit_for_window (gtk_widget_get_window (dialog));
        if (! res) {
                g_warning ("Unable to register client with session manager");
        }
}

int
main (int   argc,
      char *argv[])
{
        gboolean   res;
        GtkWidget *dialog;

        g_log_set_always_fatal (G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING);

        gtk_init (&argc, &argv);

        res = session_manager_connect ();
        if (! res) {
                g_warning ("Unable to connect to session manager");
                exit (1);
        }

        g_timeout_add_seconds (30, (GSourceFunc)gtk_main_quit, NULL);

        dialog = gtk_message_dialog_new (NULL,
                                         0,
                                         GTK_MESSAGE_INFO,
                                         GTK_BUTTONS_CANCEL,
                                         "Inhibiting logout, switch user, and suspend.");

        g_signal_connect (dialog, "response", G_CALLBACK (gtk_main_quit), NULL);
        g_signal_connect (dialog, "show", G_CALLBACK (on_widget_show), NULL);
        gtk_widget_show (dialog);

        gtk_main ();

        do_uninhibit ();
        session_manager_disconnect ();

        return 0;
}
