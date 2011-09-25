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
#include <string.h>
#include <locale.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#define MDM_DBUS_NAME                            "org.mate.DisplayManager"
#define MDM_DBUS_LOCAL_DISPLAY_FACTORY_PATH      "/org/mate/DisplayManager/LocalDisplayFactory"
#define MDM_DBUS_LOCAL_DISPLAY_FACTORY_INTERFACE "org.mate.DisplayManager.LocalDisplayFactory"

#define CK_NAME      "org.freedesktop.ConsoleKit"
#define CK_PATH      "/org/freedesktop/ConsoleKit"
#define CK_INTERFACE "org.freedesktop.ConsoleKit"

#define CK_MANAGER_PATH      "/org/freedesktop/ConsoleKit/Manager"
#define CK_MANAGER_INTERFACE "org.freedesktop.ConsoleKit.Manager"
#define CK_SEAT_INTERFACE    "org.freedesktop.ConsoleKit.Seat"
#define CK_SESSION_INTERFACE "org.freedesktop.ConsoleKit.Session"

static const char *send_command     = NULL;
static gboolean    use_xnest        = FALSE;
static gboolean    no_lock          = FALSE;
static gboolean    debug_in         = FALSE;
static gboolean    authenticate     = FALSE;
static gboolean    startnew         = FALSE;
static gboolean    monte_carlo_pi   = FALSE;
static gboolean    show_version     = FALSE;
static char      **args_remaining   = NULL;

/* Keep all config options for compatibility even if they are noops */
GOptionEntry options [] = {
        { "command", 'c', 0, G_OPTION_ARG_STRING, &send_command, N_("Only the VERSION command is supported"), N_("COMMAND") },
        { "xnest", 'n', 0, G_OPTION_ARG_NONE, &use_xnest, N_("Ignored — retained for compatibility"), NULL },
        { "no-lock", 'l', 0, G_OPTION_ARG_NONE, &no_lock, N_("Ignored — retained for compatibility"), NULL },
        { "debug", 'd', 0, G_OPTION_ARG_NONE, &debug_in, N_("Debugging output"), NULL },
        { "authenticate", 'a', 0, G_OPTION_ARG_NONE, &authenticate, N_("Ignored — retained for compatibility"), NULL },
        { "startnew", 's', 0, G_OPTION_ARG_NONE, &startnew, N_("Ignored — retained for compatibility"), NULL },
        { "monte-carlo-pi", 0, 0, G_OPTION_ARG_NONE, &monte_carlo_pi, NULL, NULL },
        { "version", 0, 0, G_OPTION_ARG_NONE, &show_version, N_("Version of this application"), NULL },
        { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, &args_remaining, NULL, NULL },
        { NULL }
};

#define MDM_FLEXISERVER_ERROR mdm_flexiserver_error_quark ()
static GQuark
mdm_flexiserver_error_quark (void)
{
        static GQuark ret = 0;
        if (ret == 0) {
                ret = g_quark_from_static_string ("mdm_flexiserver_error");
        }

        return ret;
}

static gboolean
is_program_in_path (const char *program)
{
        char *tmp = g_find_program_in_path (program);
        if (tmp != NULL) {
                g_free (tmp);
                return TRUE;
        } else {
                return FALSE;
        }
}

static void
maybe_lock_screen (void)
{
        gboolean   use_gscreensaver = FALSE;
        GError    *error            = NULL;
        char      *command;
        GdkScreen *screen;

        if (is_program_in_path ("mate-screensaver-command")) {
                use_gscreensaver = TRUE;
        } else if (! is_program_in_path ("xscreensaver-command")) {
                return;
        }

        if (use_gscreensaver) {
                command = g_strdup ("mate-screensaver-command --lock");
        } else {
                command = g_strdup ("xscreensaver-command -lock");
        }

        screen = gdk_screen_get_default ();

        if (! gdk_spawn_command_line_on_screen (screen, command, &error)) {
                g_warning ("Cannot lock screen: %s", error->message);
                g_error_free (error);
        }

        g_free (command);

        if (! use_gscreensaver) {
                command = g_strdup ("xscreensaver-command -throttle");
                if (! gdk_spawn_command_line_on_screen (screen, command, &error)) {
                        g_warning ("Cannot disable screensaver engines: %s", error->message);
                        g_error_free (error);
                }

                g_free (command);
        }
}

static void
calc_pi (void)
{
        unsigned long n = 0, h = 0;
        double x, y;
        printf ("\n");
        for (;;) {
                x = g_random_double ();
                y = g_random_double ();
                if (x*x + y*y <= 1)
                        h++;
                n++;
                if ( ! (n & 0xfff))
                        printf ("pi ~~ %1.10f\t(%lu/%lu * 4) iteration: %lu \r",
                                ((double)h)/(double)n * 4.0, h, n, n);
        }
}

static gboolean
create_transient_display (DBusConnection *connection,
                          GError        **error)
{
        DBusError       local_error;
        DBusMessage    *message;
        DBusMessage    *reply;
        gboolean        ret;
        DBusMessageIter iter;
        const char     *value;

        ret = FALSE;
        reply = NULL;

        dbus_error_init (&local_error);
        message = dbus_message_new_method_call (MDM_DBUS_NAME,
                                                MDM_DBUS_LOCAL_DISPLAY_FACTORY_PATH,
                                                MDM_DBUS_LOCAL_DISPLAY_FACTORY_INTERFACE,
                                                "CreateTransientDisplay");
        if (message == NULL) {
                g_set_error (error, MDM_FLEXISERVER_ERROR, 0, "Out of memory.");
                goto out;
        }

        dbus_error_init (&local_error);
        reply = dbus_connection_send_with_reply_and_block (connection,
                                                           message,
                                                           -1,
                                                           &local_error);
        if (reply == NULL) {
                if (dbus_error_is_set (&local_error)) {
                        g_warning ("Unable to create transient display: %s", local_error.message);
                        g_set_error (error, MDM_FLEXISERVER_ERROR, 0, "%s", local_error.message);
                        dbus_error_free (&local_error);
                        goto out;
                }
        }

        dbus_message_iter_init (reply, &iter);
        dbus_message_iter_get_basic (&iter, &value);
        g_debug ("Started %s", value);

        ret = TRUE;
 out:
        if (message != NULL) {
                dbus_message_unref (message);
        }
        if (reply != NULL) {
                dbus_message_unref (reply);
        }

        return ret;
}

static gboolean
get_current_session_id (DBusConnection *connection,
                        char          **session_id)
{
        DBusError       local_error;
        DBusMessage    *message;
        DBusMessage    *reply;
        gboolean        ret;
        DBusMessageIter iter;
        const char     *value;

        ret = FALSE;
        reply = NULL;

        dbus_error_init (&local_error);
        message = dbus_message_new_method_call (CK_NAME,
                                                CK_MANAGER_PATH,
                                                CK_MANAGER_INTERFACE,
                                                "GetCurrentSession");
        if (message == NULL) {
                goto out;
        }

        dbus_error_init (&local_error);
        reply = dbus_connection_send_with_reply_and_block (connection,
                                                           message,
                                                           -1,
                                                           &local_error);
        if (reply == NULL) {
                if (dbus_error_is_set (&local_error)) {
                        g_warning ("Unable to determine session: %s", local_error.message);
                        dbus_error_free (&local_error);
                        goto out;
                }
        }

        dbus_message_iter_init (reply, &iter);
        dbus_message_iter_get_basic (&iter, &value);
        if (session_id != NULL) {
                *session_id = g_strdup (value);
        }

        ret = TRUE;
 out:
        if (message != NULL) {
                dbus_message_unref (message);
        }
        if (reply != NULL) {
                dbus_message_unref (reply);
        }

        return ret;
}

static gboolean
get_seat_id_for_session (DBusConnection *connection,
                         const char     *session_id,
                         char          **seat_id)
{
        DBusError       local_error;
        DBusMessage    *message;
        DBusMessage    *reply;
        gboolean        ret;
        DBusMessageIter iter;
        const char     *value;

        ret = FALSE;
        reply = NULL;

        dbus_error_init (&local_error);
        message = dbus_message_new_method_call (CK_NAME,
                                                session_id,
                                                CK_SESSION_INTERFACE,
                                                "GetSeatId");
        if (message == NULL) {
                goto out;
        }

        dbus_error_init (&local_error);
        reply = dbus_connection_send_with_reply_and_block (connection,
                                                           message,
                                                           -1,
                                                           &local_error);
        if (reply == NULL) {
                if (dbus_error_is_set (&local_error)) {
                        g_warning ("Unable to determine seat: %s", local_error.message);
                        dbus_error_free (&local_error);
                        goto out;
                }
        }

        dbus_message_iter_init (reply, &iter);
        dbus_message_iter_get_basic (&iter, &value);
        if (seat_id != NULL) {
                *seat_id = g_strdup (value);
        }

        ret = TRUE;
 out:
        if (message != NULL) {
                dbus_message_unref (message);
        }
        if (reply != NULL) {
                dbus_message_unref (reply);
        }

        return ret;
}

static char *
get_current_seat_id (DBusConnection *connection)
{
        gboolean res;
        char    *session_id;
        char    *seat_id;

        session_id = NULL;
        seat_id = NULL;

        res = get_current_session_id (connection, &session_id);
        if (res) {
                res = get_seat_id_for_session (connection, session_id, &seat_id);
        }
        g_free (session_id);

        return seat_id;
}

static gboolean
activate_session_id (DBusConnection *connection,
                     const char     *seat_id,
                     const char     *session_id)
{
        DBusError    local_error;
        DBusMessage *message;
        DBusMessage *reply;
        gboolean     ret;

        ret = FALSE;
        reply = NULL;

        g_debug ("Switching to session %s", session_id);

        dbus_error_init (&local_error);
        message = dbus_message_new_method_call (CK_NAME,
                                                seat_id,
                                                CK_SEAT_INTERFACE,
                                                "ActivateSession");
        if (message == NULL) {
                goto out;
        }

        if (! dbus_message_append_args (message,
                                        DBUS_TYPE_OBJECT_PATH, &session_id,
                                        DBUS_TYPE_INVALID)) {
                goto out;
        }


        dbus_error_init (&local_error);
        reply = dbus_connection_send_with_reply_and_block (connection,
                                                           message,
                                                           -1,
                                                           &local_error);
        if (reply == NULL) {
                if (dbus_error_is_set (&local_error)) {
                        g_warning ("Unable to activate session: %s", local_error.message);
                        dbus_error_free (&local_error);
                        goto out;
                }
        }

        ret = TRUE;
 out:
        if (message != NULL) {
                dbus_message_unref (message);
        }
        if (reply != NULL) {
                dbus_message_unref (reply);
        }

        return ret;
}

static gboolean
session_is_login_window (DBusConnection *connection,
                         const char     *session_id)
{
        DBusError       local_error;
        DBusMessage    *message;
        DBusMessage    *reply;
        gboolean        ret;
        DBusMessageIter iter;
        const char     *value;

        ret = FALSE;
        reply = NULL;

        dbus_error_init (&local_error);
        message = dbus_message_new_method_call (CK_NAME,
                                                session_id,
                                                CK_SESSION_INTERFACE,
                                                "GetSessionType");
        if (message == NULL) {
                goto out;
        }

        dbus_error_init (&local_error);
        reply = dbus_connection_send_with_reply_and_block (connection,
                                                           message,
                                                           -1,
                                                           &local_error);
        if (reply == NULL) {
                if (dbus_error_is_set (&local_error)) {
                        g_warning ("Unable to determine seat: %s", local_error.message);
                        dbus_error_free (&local_error);
                        goto out;
                }
        }

        dbus_message_iter_init (reply, &iter);
        dbus_message_iter_get_basic (&iter, &value);

        if (value == NULL || value[0] == '\0' || strcmp (value, "LoginWindow") != 0) {
                goto out;
        }

        ret = TRUE;
 out:
        if (message != NULL) {
                dbus_message_unref (message);
        }
        if (reply != NULL) {
                dbus_message_unref (reply);
        }

        return ret;
}

static gboolean
seat_can_activate_sessions (DBusConnection *connection,
                            const char     *seat_id)
{
        DBusError       local_error;
        DBusMessage    *message;
        DBusMessage    *reply;
        DBusMessageIter iter;
        gboolean        can_activate;

        can_activate = FALSE;
        reply = NULL;

        dbus_error_init (&local_error);
        message = dbus_message_new_method_call (CK_NAME,
                                                seat_id,
                                                CK_SEAT_INTERFACE,
                                                "CanActivateSessions");
        if (message == NULL) {
                goto out;
        }

        dbus_error_init (&local_error);
        reply = dbus_connection_send_with_reply_and_block (connection,
                                                           message,
                                                           -1,
                                                           &local_error);
        if (reply == NULL) {
                if (dbus_error_is_set (&local_error)) {
                        g_warning ("Unable to activate session: %s", local_error.message);
                        dbus_error_free (&local_error);
                        goto out;
                }
        }

        dbus_message_iter_init (reply, &iter);
        dbus_message_iter_get_basic (&iter, &can_activate);

 out:
        if (message != NULL) {
                dbus_message_unref (message);
        }
        if (reply != NULL) {
                dbus_message_unref (reply);
        }

        return can_activate;
}

/* from libhal */
static char **
get_path_array_from_iter (DBusMessageIter *iter,
                          int             *num_elements)
{
        int count;
        char **buffer;

        count = 0;
        buffer = (char **)malloc (sizeof (char *) * 8);

        if (buffer == NULL)
                goto oom;

        buffer[0] = NULL;
        while (dbus_message_iter_get_arg_type (iter) == DBUS_TYPE_OBJECT_PATH) {
                const char *value;
                char *str;

                if ((count % 8) == 0 && count != 0) {
                        buffer = realloc (buffer, sizeof (char *) * (count + 8));
                        if (buffer == NULL)
                                goto oom;
                }

                dbus_message_iter_get_basic (iter, &value);
                str = strdup (value);
                if (str == NULL)
                        goto oom;

                buffer[count] = str;

                dbus_message_iter_next (iter);
                count++;
        }

        if ((count % 8) == 0) {
                buffer = realloc (buffer, sizeof (char *) * (count + 1));
                if (buffer == NULL)
                        goto oom;
        }

        buffer[count] = NULL;
        if (num_elements != NULL)
                *num_elements = count;
        return buffer;

oom:
        g_debug ("%s %d : error allocating memory\n", __FILE__, __LINE__);
        return NULL;

}

static char **
seat_get_sessions (DBusConnection *connection,
                   const char     *seat_id)
{
        DBusError       error;
        DBusMessage    *message;
        DBusMessage    *reply;
        DBusMessageIter iter_reply;
        DBusMessageIter iter_array;
        char           **sessions;

        sessions = NULL;
        message = NULL;
        reply = NULL;

        dbus_error_init (&error);
        message = dbus_message_new_method_call (CK_NAME,
                                                seat_id,
                                                CK_SEAT_INTERFACE,
                                                "GetSessions");
        if (message == NULL) {
                g_debug ("Couldn't allocate the D-Bus message");
                goto out;
        }

        dbus_error_init (&error);
        reply = dbus_connection_send_with_reply_and_block (connection,
                                                           message,
                                                           -1, &error);
        dbus_connection_flush (connection);

        if (dbus_error_is_set (&error)) {
                g_debug ("ConsoleKit %s raised:\n %s\n\n", error.name, error.message);
                goto out;
        }

        if (reply == NULL) {
                g_debug ("ConsoleKit: No reply for GetSessionsForUser");
                goto out;
        }

        dbus_message_iter_init (reply, &iter_reply);
        if (dbus_message_iter_get_arg_type (&iter_reply) != DBUS_TYPE_ARRAY) {
                g_debug ("ConsoleKit Wrong reply for GetSessionsForUser - expecting an array.");
                goto out;
        }

        dbus_message_iter_recurse (&iter_reply, &iter_array);
        sessions = get_path_array_from_iter (&iter_array, NULL);

 out:
        if (message != NULL) {
                dbus_message_unref (message);
        }
        if (reply != NULL) {
                dbus_message_unref (reply);
        }

        return sessions;
}

static char *
get_login_window_session_id (DBusConnection *connection,
                             const char     *seat_id)
{
        gboolean    can_activate_sessions;
        char      **sessions;
        char       *session_id;
        int         i;

        session_id = NULL;
        sessions = NULL;

        g_debug ("checking if seat can activate sessions");

        can_activate_sessions = seat_can_activate_sessions (connection, seat_id);
        if (! can_activate_sessions) {
                g_debug ("seat is unable to activate sessions");
                goto out;
        }

        sessions = seat_get_sessions (connection, seat_id);
        for (i = 0; sessions [i] != NULL; i++) {
                char *ssid;

                ssid = sessions [i];

                if (session_is_login_window (connection, ssid)) {
                        session_id = g_strdup (ssid);
                        break;
                }
        }
        g_strfreev (sessions);

 out:
        return session_id;
}

static gboolean
goto_login_session (GError **error)
{
        gboolean        ret;
        gboolean        res;
        char           *session_id;
        char           *seat_id;
        DBusError       local_error;
        DBusConnection *connection;

        ret = FALSE;

        dbus_error_init (&local_error);
        connection = dbus_bus_get (DBUS_BUS_SYSTEM, &local_error);
        if (connection == NULL) {
                g_debug ("Failed to connect to the D-Bus daemon: %s", local_error.message);
                g_set_error (error, MDM_FLEXISERVER_ERROR, 0, "%s", local_error.message);
                dbus_error_free (&local_error);
                return FALSE;
        }

        /* First look for any existing LoginWindow sessions on the seat.
           If none are found, create a new one. */

        seat_id = get_current_seat_id (connection);
        if (seat_id == NULL || seat_id[0] == '\0') {
                g_debug ("seat id is not set; can't switch sessions");
                g_set_error (error, MDM_FLEXISERVER_ERROR, 0, _("Could not identify the current session."));

                return FALSE;
        }

        session_id = get_login_window_session_id (connection, seat_id);
        if (session_id != NULL) {
                res = activate_session_id (connection, seat_id, session_id);
                if (res) {
                        ret = TRUE;
                }
        }

        if (! ret) {
                res = create_transient_display (connection, error);
                if (res) {
                        ret = TRUE;
                }
        }

        return ret;
}

int
main (int argc, char *argv[])
{
        GOptionContext *ctx;
        gboolean        res;
        GError         *error;

        bindtextdomain (GETTEXT_PACKAGE, MATELOCALEDIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);
        setlocale (LC_ALL, "");

        /* Option parsing */
        ctx = g_option_context_new (_("- New MDM login"));
        g_option_context_set_translation_domain (ctx, GETTEXT_PACKAGE);
        g_option_context_add_main_entries (ctx, options, NULL);
        g_option_context_parse (ctx, &argc, &argv, NULL);
        g_option_context_free (ctx);


        if (show_version) {
                g_print ("%s %s\n", argv [0], VERSION);
                exit (1);
        }

        /* don't support commands other than VERSION */
        if (send_command != NULL) {
                if (strcmp (send_command, "VERSION") == 0) {
                        g_print ("MDM  %s \n", VERSION);
                        return 0;
                } else {
                        g_warning ("No longer supported");
                }
                return 1;
        }

        gtk_init (&argc, &argv);

        if (monte_carlo_pi) {
                calc_pi ();
                return 0;
        }

        if (args_remaining != NULL && args_remaining[0] != NULL) {

        }

        if (use_xnest) {
                g_warning ("Not yet implemented");
                return 1;
        }

        error = NULL;
        res = goto_login_session (&error);
        if (! res) {
                GtkWidget *dialog;
                char      *message;

                if (error != NULL) {
                        message = g_strdup_printf ("%s", error->message);
                        g_error_free (error);
                } else {
                        message = g_strdup ("");
                }

                dialog = gtk_message_dialog_new (NULL,
                                                 GTK_DIALOG_DESTROY_WITH_PARENT,
                                                 GTK_MESSAGE_ERROR,
                                                 GTK_BUTTONS_CLOSE,
                                                 "%s", _("Unable to start new display"));

                gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                                          "%s", message);
                g_free (message);

                gtk_window_set_title (GTK_WINDOW (dialog), "");
                gtk_window_set_icon_name (GTK_WINDOW (dialog), "session-properties");
                gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
                gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), 14);

                gtk_dialog_run (GTK_DIALOG (dialog));
                gtk_widget_destroy (dialog);
        } else {
                maybe_lock_screen ();
        }

        return 1;
}
