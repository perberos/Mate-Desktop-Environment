/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2006 Ray Strode <rstrode@redhat.com>
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include "config.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>

#include <locale.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <glib-object.h>

#ifdef HAVE_LIBXKLAVIER
#include <libxklavier/xklavier.h>
#include <X11/Xlib.h> /* for Display */
#endif

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "mdm-session-direct.h"
#include "mdm-session.h"
#include "mdm-session-private.h"
#include "mdm-session-direct-glue.h"

#include "mdm-session-record.h"
#include "mdm-session-worker-job.h"
#include "mdm-common.h"

#define MDM_SESSION_DBUS_PATH         "/org/mate/DisplayManager/Session"
#define MDM_SESSION_DBUS_INTERFACE    "org.mate.DisplayManager.Session"
#define MDM_SESSION_DBUS_ERROR_CANCEL "org.mate.DisplayManager.Session.Error.Cancel"

#ifndef MDM_SESSION_DEFAULT_PATH
#define MDM_SESSION_DEFAULT_PATH "/usr/local/bin:/usr/bin:/bin"
#endif

struct _MdmSessionDirectPrivate
{
        /* per open scope */
        char                *selected_session;
        char                *saved_session;
        char                *selected_language;
        char                *saved_language;
        char                *selected_layout;
        char                *saved_layout;
        char                *selected_user;
        char                *user_x11_authority_file;

        DBusMessage         *message_pending_reply;
        DBusConnection      *worker_connection;

        MdmSessionWorkerJob *job;
        GPid                 session_pid;
        guint32              is_running : 1;

        /* object lifetime scope */
        char                *id;
        char                *display_id;
        char                *display_name;
        char                *display_hostname;
        char                *display_device;
        char                *display_x11_authority_file;
        gboolean             display_is_local;

        char                *fallback_session_name;

        DBusServer          *server;
        char                *server_address;
        GHashTable          *environment;
        DBusGConnection     *connection;
};

enum {
        PROP_0,
        PROP_DISPLAY_ID,
        PROP_DISPLAY_NAME,
        PROP_DISPLAY_HOSTNAME,
        PROP_DISPLAY_IS_LOCAL,
        PROP_DISPLAY_DEVICE,
        PROP_DISPLAY_X11_AUTHORITY_FILE,
        PROP_USER_X11_AUTHORITY_FILE,
};

static void     mdm_session_iface_init          (MdmSessionIface      *iface);

G_DEFINE_TYPE_WITH_CODE (MdmSessionDirect,
                         mdm_session_direct,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (MDM_TYPE_SESSION,
                                                mdm_session_iface_init))

static gboolean
send_dbus_message (DBusConnection *connection,
                   DBusMessage    *message)
{
        gboolean is_connected;
        gboolean sent;

        g_return_val_if_fail (message != NULL, FALSE);

        if (connection == NULL) {
                g_warning ("There is no valid connection");
                return FALSE;
        }

        is_connected = dbus_connection_get_is_connected (connection);
        if (! is_connected) {
                g_warning ("Not connected!");
                return FALSE;
        }

        sent = dbus_connection_send (connection, message, NULL);

        return sent;
}

static void
send_dbus_string_signal (MdmSessionDirect *session,
                         const char *name,
                         const char *text)
{
        DBusMessage    *message;
        DBusMessageIter iter;

        g_return_if_fail (session != NULL);

        message = dbus_message_new_signal (MDM_SESSION_DBUS_PATH,
                                           MDM_SESSION_DBUS_INTERFACE,
                                           name);

        dbus_message_iter_init_append (message, &iter);
        dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &text);

        if (! send_dbus_message (session->priv->worker_connection, message)) {
                g_debug ("MdmSessionDirect: Could not send %s signal",
                         name ? name : "(null)");
        }

        dbus_message_unref (message);
}

static void
send_dbus_void_signal (MdmSessionDirect *session,
                       const char       *name)
{
        DBusMessage *message;

        g_return_if_fail (session != NULL);

        message = dbus_message_new_signal (MDM_SESSION_DBUS_PATH,
                                           MDM_SESSION_DBUS_INTERFACE,
                                           name);

        if (! send_dbus_message (session->priv->worker_connection, message)) {
                g_debug ("MdmSessionDirect: Could not send %s signal", name);
        }

        dbus_message_unref (message);
}

static void
on_authentication_failed (MdmSession *session,
                          const char *message)
{
        MdmSessionDirect *impl = MDM_SESSION_DIRECT (session);
        mdm_session_record_failed (impl->priv->session_pid,
                                   impl->priv->selected_user,
                                   impl->priv->display_hostname,
                                   impl->priv->display_name,
                                   impl->priv->display_device);
}

static void
on_session_started (MdmSession *session)
{
        MdmSessionDirect *impl = MDM_SESSION_DIRECT (session);
        mdm_session_record_login (impl->priv->session_pid,
                                  impl->priv->selected_user,
                                  impl->priv->display_hostname,
                                  impl->priv->display_name,
                                  impl->priv->display_device);
}

static void
on_session_start_failed (MdmSession *session,
                         const char *message)
{
        MdmSessionDirect *impl = MDM_SESSION_DIRECT (session);
        mdm_session_record_login (impl->priv->session_pid,
                                  impl->priv->selected_user,
                                  impl->priv->display_hostname,
                                  impl->priv->display_name,
                                  impl->priv->display_device);
}

static void
on_session_exited (MdmSession *session,
                   int        exit_code)
{
        MdmSessionDirect *impl = MDM_SESSION_DIRECT (session);
        mdm_session_record_logout (impl->priv->session_pid,
                                   impl->priv->selected_user,
                                   impl->priv->display_hostname,
                                   impl->priv->display_name,
                                   impl->priv->display_device);
}

static DBusHandlerResult
mdm_session_direct_handle_setup_complete (MdmSessionDirect *session,
                                          DBusConnection   *connection,
                                          DBusMessage      *message)
{
        DBusMessage *reply;

        g_debug ("MdmSessionDirect: Emitting 'setup-complete' signal");

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        _mdm_session_setup_complete (MDM_SESSION (session));

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
mdm_session_direct_handle_setup_failed (MdmSessionDirect *session,
                                        DBusConnection   *connection,
                                        DBusMessage      *message)
{
        DBusMessage *reply;
        DBusError    error;
        const char  *text;

        dbus_error_init (&error);
        if (! dbus_message_get_args (message, &error,
                                     DBUS_TYPE_STRING, &text,
                                     DBUS_TYPE_INVALID)) {
                g_warning ("ERROR: %s", error.message);
        }

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        g_debug ("MdmSessionDirect: Emitting 'setup-failed' signal");

        _mdm_session_setup_failed (MDM_SESSION (session), text);

        return DBUS_HANDLER_RESULT_HANDLED;
}


static DBusHandlerResult
mdm_session_direct_handle_reset_complete (MdmSessionDirect *session,
                                          DBusConnection   *connection,
                                          DBusMessage      *message)
{
        DBusMessage *reply;

        g_debug ("MdmSessionDirect: Emitting 'reset-complete' signal");

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        _mdm_session_reset_complete (MDM_SESSION (session));

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
mdm_session_direct_handle_reset_failed (MdmSessionDirect *session,
                                        DBusConnection   *connection,
                                        DBusMessage      *message)
{
        DBusMessage *reply;
        DBusError    error;
        const char  *text;

        dbus_error_init (&error);
        if (! dbus_message_get_args (message, &error,
                                     DBUS_TYPE_STRING, &text,
                                     DBUS_TYPE_INVALID)) {
                g_warning ("ERROR: %s", error.message);
        }

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        g_debug ("MdmSessionDirect: Emitting 'reset-failed' signal");

        _mdm_session_reset_failed (MDM_SESSION (session), text);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
mdm_session_direct_handle_authenticated (MdmSessionDirect *session,
                                         DBusConnection   *connection,
                                         DBusMessage      *message)
{
        DBusMessage *reply;

        g_debug ("MdmSessionDirect: Emitting 'authenticated' signal");

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        _mdm_session_authenticated (MDM_SESSION (session));

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
mdm_session_direct_handle_authentication_failed (MdmSessionDirect *session,
                                                 DBusConnection   *connection,
                                                 DBusMessage      *message)
{
        DBusMessage *reply;
        DBusError    error;
        const char  *text;

        dbus_error_init (&error);
        if (! dbus_message_get_args (message, &error,
                                     DBUS_TYPE_STRING, &text,
                                     DBUS_TYPE_INVALID)) {
                g_warning ("ERROR: %s", error.message);
        }

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        g_debug ("MdmSessionDirect: Emitting 'authentication-failed' signal");

        _mdm_session_authentication_failed (MDM_SESSION (session), text);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
mdm_session_direct_handle_authorized (MdmSessionDirect *session,
                                      DBusConnection   *connection,
                                      DBusMessage      *message)
{
        DBusMessage *reply;

        g_debug ("MdmSessionDirect: Emitting 'authorized' signal");

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        _mdm_session_authorized (MDM_SESSION (session));

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
mdm_session_direct_handle_authorization_failed (MdmSessionDirect *session,
                                                DBusConnection   *connection,
                                                DBusMessage      *message)
{
        DBusMessage *reply;
        DBusError    error;
        const char  *text;

        dbus_error_init (&error);
        if (! dbus_message_get_args (message, &error,
                                     DBUS_TYPE_STRING, &text,
                                     DBUS_TYPE_INVALID)) {
                g_warning ("ERROR: %s", error.message);
        }

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        g_debug ("MdmSessionDirect: Emitting 'authorization-failed' signal");

        _mdm_session_authorization_failed (MDM_SESSION (session), text);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
mdm_session_direct_handle_accredited (MdmSessionDirect *session,
                                      DBusConnection   *connection,
                                      DBusMessage      *message)
{
        DBusMessage *reply;

        g_debug ("MdmSessionDirect: Emitting 'accredited' signal");

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        _mdm_session_accredited (MDM_SESSION (session));

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
mdm_session_direct_handle_accreditation_failed (MdmSessionDirect *session,
                                                DBusConnection   *connection,
                                                DBusMessage      *message)
{
        DBusMessage *reply;
        DBusError    error;
        const char  *text;

        dbus_error_init (&error);
        if (! dbus_message_get_args (message, &error,
                                     DBUS_TYPE_STRING, &text,
                                     DBUS_TYPE_INVALID)) {
                g_warning ("ERROR: %s", error.message);
        }

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        g_debug ("MdmSessionDirect: Emitting 'accreditation-failed' signal");

        _mdm_session_accreditation_failed (MDM_SESSION (session), text);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static const char **
get_system_session_dirs (void)
{
        static const char *search_dirs[] = {
                "/etc/X11/sessions/",
                DMCONFDIR "/Sessions/",
                DATADIR "/xsessions/",
                DATADIR "/mdm/BuiltInSessions/",
                NULL
        };

        return search_dirs;
}

static gboolean
is_prog_in_path (const char *prog)
{
        char    *f;
        gboolean ret;

        f = g_find_program_in_path (prog);
        ret = (f != NULL);
        g_free (f);
        return ret;
}

static gboolean
get_session_command_for_file (const char *file,
                              char      **command)
{
        GKeyFile   *key_file;
        GError     *error;
        char       *exec;
        gboolean    ret;
        gboolean    res;

        exec = NULL;
        ret = FALSE;
        if (command != NULL) {
                *command = NULL;
        }

        key_file = g_key_file_new ();

        g_debug ("MdmSessionDirect: looking for session file '%s'", file);

        error = NULL;
        res = g_key_file_load_from_dirs (key_file,
                                         file,
                                         get_system_session_dirs (),
                                         NULL,
                                         G_KEY_FILE_NONE,
                                         &error);
        if (! res) {
                g_debug ("MdmSessionDirect: File '%s' not found: %s", file, error->message);
                g_error_free (error);
                if (command != NULL) {
                        *command = NULL;
                }
                goto out;
        }

        error = NULL;
        res = g_key_file_get_boolean (key_file,
                                      G_KEY_FILE_DESKTOP_GROUP,
                                      G_KEY_FILE_DESKTOP_KEY_HIDDEN,
                                      &error);
        if (error == NULL && res) {
                g_debug ("MdmSessionDirect: Session %s is marked as hidden", file);
                goto out;
        }

        exec = g_key_file_get_string (key_file,
                                      G_KEY_FILE_DESKTOP_GROUP,
                                      G_KEY_FILE_DESKTOP_KEY_TRY_EXEC,
                                      NULL);
        if (exec != NULL) {
                res = is_prog_in_path (exec);
                g_free (exec);
                exec = NULL;

                if (! res) {
                        g_debug ("MdmSessionDirect: Command not found: %s",
                                 G_KEY_FILE_DESKTOP_KEY_TRY_EXEC);
                        goto out;
                }
        }

        error = NULL;
        exec = g_key_file_get_string (key_file,
                                      G_KEY_FILE_DESKTOP_GROUP,
                                      G_KEY_FILE_DESKTOP_KEY_EXEC,
                                      &error);
        if (error != NULL) {
                g_debug ("MdmSessionDirect: %s key not found: %s",
                         G_KEY_FILE_DESKTOP_KEY_EXEC,
                         error->message);
                g_error_free (error);
                goto out;
        }

        if (command != NULL) {
                *command = g_strdup (exec);
        }
        ret = TRUE;

out:
        g_free (exec);

        return ret;
}

static gboolean
get_session_command_for_name (const char *name,
                              char      **command)
{
        gboolean res;
        char    *filename;

        filename = g_strdup_printf ("%s.desktop", name);
        res = get_session_command_for_file (filename, command);
        g_free (filename);

        /*
         * The MDM Xsession script honors "custom" as a valid session.  If the
         * session is one of these, no file is needed, then just run the
         * command as "custom".
         */
        if (!res && strcmp (name, MDM_CUSTOM_SESSION) == 0) {
                g_debug ("No custom desktop file, but accepting it anyway.");
                if (command != NULL) {
                        *command = g_strdup (MDM_CUSTOM_SESSION);
                }
                res = TRUE;
        }

        return res;
}

static const char *
get_default_language_name (MdmSessionDirect *session)
{
    if (session->priv->saved_language != NULL) {
                return session->priv->saved_language;
    }

    return setlocale (LC_MESSAGES, NULL);
}

static char *
get_system_default_layout (MdmSessionDirect *session)
{
    char *result = NULL;
#ifdef HAVE_LIBXKLAVIER
    static XklEngine *engine = NULL;

    if (engine == NULL) {
            Display *display = XOpenDisplay (session->priv->display_name);
            if (display != NULL) {
                    engine = xkl_engine_get_instance (display);
            }
            /* do NOT call XCloseDisplay (display) here;
             * xkl_engine_get_instance() is a singleton which saves the display */
    }

    if (engine != NULL) {
            XklConfigRec *config = xkl_config_rec_new ();
            if (xkl_config_rec_get_from_server (config, engine) && config->layouts && config->layouts[0]) {
                    if (config->variants && config->variants[0] && config->variants[0][0])
                            result = g_strdup_printf("%s\t%s", config->layouts[0], config->variants[0]);
                    else
                            result = g_strdup (config->layouts[0]);
            }
            g_object_unref (config);
    }
#endif

    if (!result)
        result = g_strdup ("us");
    return result;
}

static const char *
get_default_layout_name (MdmSessionDirect *session)
{
    if (!session->priv->saved_layout) {
        session->priv->saved_layout = get_system_default_layout (session);
    }

    return session->priv->saved_layout;
}

static const char *
get_fallback_session_name (MdmSessionDirect *session_direct)
{
        const char  **search_dirs;
        int           i;
        char         *name;
        GSequence    *sessions;
        GSequenceIter *session;

        if (session_direct->priv->fallback_session_name != NULL) {
                /* verify that the cached version still exists */
                if (get_session_command_for_name (session_direct->priv->fallback_session_name, NULL)) {
                        goto out;
                }
        }

        name = g_strdup ("mate");
        if (get_session_command_for_name (name, NULL)) {
                g_free (session_direct->priv->fallback_session_name);
                session_direct->priv->fallback_session_name = name;
                goto out;
        }
        g_free (name);

        sessions = g_sequence_new (g_free);

        search_dirs = get_system_session_dirs ();
        for (i = 0; search_dirs[i] != NULL; i++) {
                GDir       *dir;
                const char *base_name;

                dir = g_dir_open (search_dirs[i], 0, NULL);

                if (dir == NULL) {
                        continue;
                }

                do {
                        base_name = g_dir_read_name (dir);

                        if (base_name == NULL) {
                                break;
                        }

                        if (!g_str_has_suffix (base_name, ".desktop")) {
                                continue;
                        }

                        if (get_session_command_for_file (base_name, NULL)) {

                                g_sequence_insert_sorted (sessions, g_strdup (base_name), (GCompareDataFunc) g_strcmp0, NULL);
                        }
                } while (base_name != NULL);

                g_dir_close (dir);
        }

        name = NULL;
        session = g_sequence_get_begin_iter (sessions);
        do {
               if (g_sequence_get (session)) {
                       char *base_name;

                       g_free (name);
                       base_name = g_sequence_get (session);
                       name = g_strndup (base_name,
                                         strlen (base_name) -
                                         strlen (".desktop"));

                       break;
               }
               session = g_sequence_iter_next (session);
        } while (!g_sequence_iter_is_end (session));

        g_free (session_direct->priv->fallback_session_name);
        session_direct->priv->fallback_session_name = name;

        g_sequence_free (sessions);

 out:
        return session_direct->priv->fallback_session_name;
}

static const char *
get_default_session_name (MdmSessionDirect *session)
{
        if (session->priv->saved_session != NULL) {
                return session->priv->saved_session;
        }

        return get_fallback_session_name (session);
}

static void
mdm_session_direct_defaults_changed (MdmSessionDirect *session)
{
        _mdm_session_default_language_name_changed (MDM_SESSION (session),
                                                    get_default_language_name (session));
        _mdm_session_default_layout_name_changed (MDM_SESSION (session),
                                                  get_default_layout_name (session));
        _mdm_session_default_session_name_changed (MDM_SESSION (session),
                                                   get_default_session_name (session));
}

static void
mdm_session_direct_select_user (MdmSession *session,
                                const char *text)
{
        MdmSessionDirect *impl = MDM_SESSION_DIRECT (session);

        g_debug ("MdmSessionDirect: Setting user: '%s'", text);

        g_free (impl->priv->selected_user);
        impl->priv->selected_user = g_strdup (text);

        g_free (impl->priv->saved_session);
        impl->priv->saved_session = NULL;

        g_free (impl->priv->saved_language);
        impl->priv->saved_language = NULL;

        g_free (impl->priv->saved_layout);
        impl->priv->saved_layout = NULL;
}

static DBusHandlerResult
mdm_session_direct_handle_username_changed (MdmSessionDirect *session,
                                            DBusConnection   *connection,
                                            DBusMessage      *message)
{
        DBusMessage *reply;
        DBusError    error;
        const char  *text;

        dbus_error_init (&error);
        if (! dbus_message_get_args (message, &error,
                                     DBUS_TYPE_STRING, &text,
                                     DBUS_TYPE_INVALID)) {
                g_warning ("ERROR: %s", error.message);
        }

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        g_debug ("MdmSessionDirect: changing username from '%s' to '%s'",
                 session->priv->selected_user != NULL ? session->priv->selected_user : "<unset>",
                 (strlen (text)) ? text : "<unset>");

        mdm_session_direct_select_user (MDM_SESSION (session), (strlen (text) > 0) ? g_strdup (text) : NULL);

        _mdm_session_selected_user_changed (MDM_SESSION (session), session->priv->selected_user);

        mdm_session_direct_defaults_changed (session);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static void
cancel_pending_query (MdmSessionDirect *session)
{
        DBusMessage *reply;

        if (session->priv->message_pending_reply == NULL) {
                return;
        }

        g_debug ("MdmSessionDirect: Cancelling pending query");

        reply = dbus_message_new_error (session->priv->message_pending_reply,
                                        MDM_SESSION_DBUS_ERROR_CANCEL,
                                        "Operation cancelled");
        dbus_connection_send (session->priv->worker_connection, reply, NULL);
        dbus_connection_flush (session->priv->worker_connection);

        dbus_message_unref (reply);
        dbus_message_unref (session->priv->message_pending_reply);
        session->priv->message_pending_reply = NULL;
}

static void
answer_pending_query (MdmSessionDirect *session,
                      const char       *answer)
{
        DBusMessage    *reply;
        DBusMessageIter iter;

        g_assert (session->priv->message_pending_reply != NULL);

        reply = dbus_message_new_method_return (session->priv->message_pending_reply);
        dbus_message_iter_init_append (reply, &iter);
        dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &answer);

        dbus_connection_send (session->priv->worker_connection, reply, NULL);
        dbus_message_unref (reply);

        dbus_message_unref (session->priv->message_pending_reply);
        session->priv->message_pending_reply = NULL;
}

static void
set_pending_query (MdmSessionDirect *session,
                   DBusMessage      *message)
{
        g_assert (session->priv->message_pending_reply == NULL);

        session->priv->message_pending_reply = dbus_message_ref (message);
}

static DBusHandlerResult
mdm_session_direct_handle_info_query (MdmSessionDirect *session,
                                      DBusConnection   *connection,
                                      DBusMessage      *message)
{
        DBusError    error;
        const char  *text;

        dbus_error_init (&error);
        if (! dbus_message_get_args (message, &error,
                                     DBUS_TYPE_STRING, &text,
                                     DBUS_TYPE_INVALID)) {
                g_warning ("ERROR: %s", error.message);
        }

        set_pending_query (session, message);

        g_debug ("MdmSessionDirect: Emitting 'info-query' signal");
        _mdm_session_info_query (MDM_SESSION (session), text);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
mdm_session_direct_handle_secret_info_query (MdmSessionDirect *session,
                                             DBusConnection   *connection,
                                             DBusMessage      *message)
{
        DBusError    error;
        const char  *text;

        dbus_error_init (&error);
        if (! dbus_message_get_args (message, &error,
                                     DBUS_TYPE_STRING, &text,
                                     DBUS_TYPE_INVALID)) {
                g_warning ("ERROR: %s", error.message);
        }

        set_pending_query (session, message);

        g_debug ("MdmSessionDirect: Emitting 'secret-info-query' signal");
        _mdm_session_secret_info_query (MDM_SESSION (session), text);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
mdm_session_direct_handle_info (MdmSessionDirect *session,
                                DBusConnection   *connection,
                                DBusMessage      *message)
{
        DBusMessage *reply;
        DBusError    error;
        const char  *text;

        dbus_error_init (&error);
        if (! dbus_message_get_args (message, &error,
                                     DBUS_TYPE_STRING, &text,
                                     DBUS_TYPE_INVALID)) {
                g_warning ("ERROR: %s", error.message);
        }

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        g_debug ("MdmSessionDirect: Emitting 'info' signal");
        _mdm_session_info (MDM_SESSION (session), text);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
mdm_session_direct_handle_cancel_pending_query (MdmSessionDirect *session,
                                                DBusConnection   *connection,
                                                DBusMessage      *message)
{
        DBusMessage *reply;

        g_debug ("MdmSessionDirect: worker cancelling pending query");
        cancel_pending_query (session);

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
mdm_session_direct_handle_problem (MdmSessionDirect *session,
                                   DBusConnection   *connection,
                                   DBusMessage      *message)
{
        DBusMessage *reply;
        DBusError    error;
        const char  *text;

        dbus_error_init (&error);
        if (! dbus_message_get_args (message, &error,
                                     DBUS_TYPE_STRING, &text,
                                     DBUS_TYPE_INVALID)) {
                g_warning ("ERROR: %s", error.message);
        }

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        g_debug ("MdmSessionDirect: Emitting 'problem' signal");
        _mdm_session_problem (MDM_SESSION (session), text);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
mdm_session_direct_handle_session_opened (MdmSessionDirect *session,
                                          DBusConnection   *connection,
                                          DBusMessage      *message)
{
        DBusMessage *reply;
        DBusError    error;

        g_debug ("MdmSessionDirect: Handling SessionOpened");

        dbus_error_init (&error);
        if (! dbus_message_get_args (message, &error, DBUS_TYPE_INVALID)) {
                g_warning ("ERROR: %s", error.message);
        }

        g_debug ("MdmSessionDirect: Emitting 'session-opened' signal");

        _mdm_session_session_opened (MDM_SESSION (session));

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
mdm_session_direct_handle_open_failed (MdmSessionDirect *session,
                                       DBusConnection   *connection,
                                       DBusMessage      *message)
{
        DBusMessage *reply;
        DBusError    error;
        const char  *text;

        dbus_error_init (&error);
        if (! dbus_message_get_args (message, &error,
                                     DBUS_TYPE_STRING, &text,
                                     DBUS_TYPE_INVALID)) {
                g_warning ("ERROR: %s", error.message);
        }

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        g_debug ("MdmSessionDirect: Emitting 'session-open-failed' signal");
        _mdm_session_session_open_failed (MDM_SESSION (session), text);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
mdm_session_direct_handle_session_started (MdmSessionDirect *session,
                                           DBusConnection   *connection,
                                           DBusMessage      *message)
{
        DBusMessage *reply;
        DBusError    error;
        int          pid;

        pid = 0;

        g_debug ("MdmSessionDirect: Handling SessionStarted");

        dbus_error_init (&error);
        if (! dbus_message_get_args (message, &error,
                                     DBUS_TYPE_INT32, &pid,
                                     DBUS_TYPE_INVALID)) {
                g_warning ("ERROR: %s", error.message);
        }

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        g_debug ("MdmSessionDirect: Emitting 'session-started' signal with pid '%d'",
                 pid);

        session->priv->session_pid = pid;
        session->priv->is_running = TRUE;

        _mdm_session_session_started (MDM_SESSION (session), pid);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
mdm_session_direct_handle_start_failed (MdmSessionDirect *session,
                                        DBusConnection   *connection,
                                        DBusMessage      *message)
{
        DBusMessage *reply;
        DBusError    error;
        const char  *text;

        dbus_error_init (&error);
        if (! dbus_message_get_args (message, &error,
                                     DBUS_TYPE_STRING, &text,
                                     DBUS_TYPE_INVALID)) {
                g_warning ("ERROR: %s", error.message);
        }

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        g_debug ("MdmSessionDirect: Emitting 'session-start-failed' signal");
        _mdm_session_session_start_failed (MDM_SESSION (session), text);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
mdm_session_direct_handle_session_exited (MdmSessionDirect *session,
                                          DBusConnection   *connection,
                                          DBusMessage      *message)
{
        DBusMessage *reply;
        DBusError    error;
        int          code;

        dbus_error_init (&error);
        if (! dbus_message_get_args (message, &error,
                                     DBUS_TYPE_INT32, &code,
                                     DBUS_TYPE_INVALID)) {
                g_warning ("ERROR: %s", error.message);
        }

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        g_debug ("MdmSessionDirect: Emitting 'session-exited' signal with exit code '%d'",
                 code);

        session->priv->is_running = FALSE;
        _mdm_session_session_exited (MDM_SESSION (session), code);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
mdm_session_direct_handle_session_died (MdmSessionDirect *session,
                                        DBusConnection   *connection,
                                        DBusMessage      *message)
{
        DBusMessage *reply;
        DBusError    error;
        int          code;

        dbus_error_init (&error);
        if (! dbus_message_get_args (message, &error,
                                     DBUS_TYPE_INT32, &code,
                                     DBUS_TYPE_INVALID)) {
                g_warning ("ERROR: %s", error.message);
        }

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        g_debug ("MdmSessionDirect: Emitting 'session-died' signal with signal number '%d'",
                 code);

        session->priv->is_running = FALSE;
        _mdm_session_session_died (MDM_SESSION (session), code);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
mdm_session_direct_handle_saved_language_name_read (MdmSessionDirect *session,
                                                    DBusConnection   *connection,
                                                    DBusMessage      *message)
{
        DBusMessage *reply;
        DBusError    error;
        const char  *language_name;

        dbus_error_init (&error);
        if (! dbus_message_get_args (message, &error,
                                     DBUS_TYPE_STRING, &language_name,
                                     DBUS_TYPE_INVALID)) {
                g_warning ("ERROR: %s", error.message);
        }

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        if (strcmp (language_name,
                    get_default_language_name (session)) != 0) {
                g_free (session->priv->saved_language);
                session->priv->saved_language = g_strdup (language_name);

                _mdm_session_default_language_name_changed (MDM_SESSION (session),
                                                            language_name);
        }


        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
mdm_session_direct_handle_saved_layout_name_read (MdmSessionDirect *session,
                                                  DBusConnection   *connection,
                                                  DBusMessage      *message)
{
        DBusMessage *reply;
        DBusError    error;
        const char  *layout_name;

        dbus_error_init (&error);
        if (! dbus_message_get_args (message, &error,
                                     DBUS_TYPE_STRING, &layout_name,
                                     DBUS_TYPE_INVALID)) {
                g_warning ("ERROR: %s", error.message);
        }

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        if (strcmp (layout_name,
                    get_default_layout_name (session)) != 0) {
                g_free (session->priv->saved_layout);
                session->priv->saved_layout = g_strdup (layout_name);

                _mdm_session_default_layout_name_changed (MDM_SESSION (session),
                                                          layout_name);
        }


        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
mdm_session_direct_handle_saved_session_name_read (MdmSessionDirect *session,
                                                   DBusConnection   *connection,
                                                   DBusMessage      *message)
{
        DBusMessage *reply;
        DBusError    error;
        const char  *session_name;

        dbus_error_init (&error);
        if (! dbus_message_get_args (message, &error,
                                     DBUS_TYPE_STRING, &session_name,
                                     DBUS_TYPE_INVALID)) {
                g_warning ("ERROR: %s", error.message);
        }

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        if (! get_session_command_for_name (session_name, NULL)) {
                /* ignore sessions that don't exist */
                g_debug ("MdmSessionDirect: not using invalid .dmrc session: %s", session_name);
                g_free (session->priv->saved_session);
                session->priv->saved_session = NULL;
                goto out;
        }

        if (strcmp (session_name,
                    get_default_session_name (session)) != 0) {
                g_free (session->priv->saved_session);
                session->priv->saved_session = g_strdup (session_name);

                _mdm_session_default_session_name_changed (MDM_SESSION (session),
                                                           session_name);
        }
 out:
        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
session_worker_message (DBusConnection *connection,
                        DBusMessage    *message,
                        void           *user_data)
{
        MdmSessionDirect *session = MDM_SESSION_DIRECT (user_data);

        if (dbus_message_is_method_call (message, MDM_SESSION_DBUS_INTERFACE, "InfoQuery")) {
                return mdm_session_direct_handle_info_query (session, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_DBUS_INTERFACE, "SecretInfoQuery")) {
                return mdm_session_direct_handle_secret_info_query (session, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_DBUS_INTERFACE, "Info")) {
                return mdm_session_direct_handle_info (session, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_DBUS_INTERFACE, "Problem")) {
                return mdm_session_direct_handle_problem (session, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_DBUS_INTERFACE, "CancelPendingQuery")) {
                return mdm_session_direct_handle_cancel_pending_query (session, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_DBUS_INTERFACE, "SetupComplete")) {
                return mdm_session_direct_handle_setup_complete (session, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_DBUS_INTERFACE, "SetupFailed")) {
                return mdm_session_direct_handle_setup_failed (session, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_DBUS_INTERFACE, "ResetComplete")) {
                return mdm_session_direct_handle_reset_complete (session, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_DBUS_INTERFACE, "ResetFailed")) {
                return mdm_session_direct_handle_reset_failed (session, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_DBUS_INTERFACE, "Authenticated")) {
                return mdm_session_direct_handle_authenticated (session, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_DBUS_INTERFACE, "AuthenticationFailed")) {
                return mdm_session_direct_handle_authentication_failed (session, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_DBUS_INTERFACE, "Authorized")) {
                return mdm_session_direct_handle_authorized (session, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_DBUS_INTERFACE, "AuthorizationFailed")) {
                return mdm_session_direct_handle_authorization_failed (session, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_DBUS_INTERFACE, "Accredited")) {
                return mdm_session_direct_handle_accredited (session, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_DBUS_INTERFACE, "AccreditationFailed")) {
                return mdm_session_direct_handle_accreditation_failed (session, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_DBUS_INTERFACE, "UsernameChanged")) {
                return mdm_session_direct_handle_username_changed (session, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_DBUS_INTERFACE, "SessionOpened")) {
                return mdm_session_direct_handle_session_opened (session, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_DBUS_INTERFACE, "OpenFailed")) {
                return mdm_session_direct_handle_open_failed (session, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_DBUS_INTERFACE, "SessionStarted")) {
                return mdm_session_direct_handle_session_started (session, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_DBUS_INTERFACE, "StartFailed")) {
                return mdm_session_direct_handle_start_failed (session, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_DBUS_INTERFACE, "SessionExited")) {
                return mdm_session_direct_handle_session_exited (session, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_DBUS_INTERFACE, "SessionDied")) {
                return mdm_session_direct_handle_session_died (session, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_DBUS_INTERFACE, "SavedLanguageNameRead")) {
                return mdm_session_direct_handle_saved_language_name_read (session, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_DBUS_INTERFACE, "SavedLayoutNameRead")) {
                return mdm_session_direct_handle_saved_layout_name_read (session, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_DBUS_INTERFACE, "SavedSessionNameRead")) {
                return mdm_session_direct_handle_saved_session_name_read (session, connection, message);
        }

        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static DBusHandlerResult
do_introspect (DBusConnection *connection,
               DBusMessage    *message)
{
        DBusMessage *reply;
        GString     *xml;
        char        *xml_string;

        g_debug ("MdmSessionDirect: Do introspect");

        /* standard header */
        xml = g_string_new ("<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n"
                            "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
                            "<node>\n"
                            "  <interface name=\"org.freedesktop.DBus.Introspectable\">\n"
                            "    <method name=\"Introspect\">\n"
                            "      <arg name=\"data\" direction=\"out\" type=\"s\"/>\n"
                            "    </method>\n"
                            "  </interface>\n");

        /* interface */
        xml = g_string_append (xml,
                               "  <interface name=\"org.mate.DisplayManager.Session\">\n"
                               "    <method name=\"SetupComplete\">\n"
                               "    </method>\n"
                               "    <method name=\"SetupFailed\">\n"
                               "      <arg name=\"message\" direction=\"in\" type=\"s\"/>\n"
                               "    </method>\n"
                               "    <method name=\"ResetComplete\">\n"
                               "    </method>\n"
                               "    <method name=\"ResetFailed\">\n"
                               "      <arg name=\"message\" direction=\"in\" type=\"s\"/>\n"
                               "    </method>\n"
                               "    <method name=\"Authenticated\">\n"
                               "    </method>\n"
                               "    <method name=\"AuthenticationFailed\">\n"
                               "      <arg name=\"message\" direction=\"in\" type=\"s\"/>\n"
                               "    </method>\n"
                               "    <method name=\"Authorized\">\n"
                               "    </method>\n"
                               "    <method name=\"AuthorizationFailed\">\n"
                               "      <arg name=\"message\" direction=\"in\" type=\"s\"/>\n"
                               "    </method>\n"
                               "    <method name=\"Accredited\">\n"
                               "    </method>\n"
                               "    <method name=\"AccreditationFailed\">\n"
                               "      <arg name=\"message\" direction=\"in\" type=\"s\"/>\n"
                               "    </method>\n"
                               "    <method name=\"CancelPendingQuery\">\n"
                               "    </method>\n"
                               "    <method name=\"InfoQuery\">\n"
                               "      <arg name=\"query\" direction=\"in\" type=\"s\"/>\n"
                               "      <arg name=\"answer\" direction=\"out\" type=\"s\"/>\n"
                               "    </method>\n"
                               "    <method name=\"SecretInfoQuery\">\n"
                               "      <arg name=\"query\" direction=\"in\" type=\"s\"/>\n"
                               "      <arg name=\"answer\" direction=\"out\" type=\"s\"/>\n"
                               "    </method>\n"
                               "    <method name=\"Info\">\n"
                               "      <arg name=\"text\" direction=\"in\" type=\"s\"/>\n"
                               "    </method>\n"
                               "    <method name=\"Problem\">\n"
                               "      <arg name=\"text\" direction=\"in\" type=\"s\"/>\n"
                               "    </method>\n"
                               "    <method name=\"UsernameChanged\">\n"
                               "      <arg name=\"text\" direction=\"in\" type=\"s\"/>\n"
                               "    </method>\n"
                               "    <method name=\"StartFailed\">\n"
                               "      <arg name=\"message\" direction=\"in\" type=\"s\"/>\n"
                               "    </method>\n"
                               "    <method name=\"SessionStarted\">\n"
                               "      <arg name=\"pid\" direction=\"in\" type=\"i\"/>\n"
                               "      <arg name=\"environment\" direction=\"in\" type=\"as\"/>\n"
                               "    </method>\n"
                               "    <method name=\"SessionExited\">\n"
                               "      <arg name=\"code\" direction=\"in\" type=\"i\"/>\n"
                               "    </method>\n"
                               "    <method name=\"SessionDied\">\n"
                               "      <arg name=\"signal\" direction=\"in\" type=\"i\"/>\n"
                               "    </method>\n"
                               "    <signal name=\"Reset\">\n"
                               "    </signal>\n"
                               "    <signal name=\"Setup\">\n"
                               "      <arg name=\"service_name\" type=\"s\"/>\n"
                               "      <arg name=\"x11_display_name\" type=\"s\"/>\n"
                               "      <arg name=\"display_device\" type=\"s\"/>\n"
                               "      <arg name=\"hostname\" type=\"s\"/>\n"
                               "      <arg name=\"x11_authority_file\" type=\"s\"/>\n"
                               "    </signal>\n"
                               "    <signal name=\"SetupForUser\">\n"
                               "      <arg name=\"service_name\" type=\"s\"/>\n"
                               "      <arg name=\"x11_display_name\" type=\"s\"/>\n"
                               "      <arg name=\"display_device\" type=\"s\"/>\n"
                               "      <arg name=\"hostname\" type=\"s\"/>\n"
                               "      <arg name=\"x11_authority_file\" type=\"s\"/>\n"
                               "      <arg name=\"username\" type=\"s\"/>\n"
                               "    </signal>\n"
                               "    <signal name=\"Authenticate\">\n"
                               "    </signal>\n"
                               "    <signal name=\"Authorize\">\n"
                               "    </signal>\n"
                               "    <signal name=\"EstablishCredentials\">\n"
                               "    </signal>\n"
                               "    <signal name=\"RefreshCredentials\">\n"
                               "    </signal>\n"
                               "    <signal name=\"SetEnvironmentVariable\">\n"
                               "      <arg name=\"name\" type=\"s\"/>\n"
                               "      <arg name=\"value\" type=\"s\"/>\n"
                               "    </signal>\n"
                               "    <signal name=\"SetLanguageName\">\n"
                               "      <arg name=\"language_name\" type=\"s\"/>\n"
                               "    </signal>\n"
                               "    <signal name=\"SetSessionName\">\n"
                               "      <arg name=\"session_name\" type=\"s\"/>\n"
                               "    </signal>\n"
                               "    <signal name=\"StartProgram\">\n"
                               "      <arg name=\"command\" type=\"s\"/>\n"
                               "    </signal>\n"
                               "  </interface>\n");

        reply = dbus_message_new_method_return (message);

        xml = g_string_append (xml, "</node>\n");
        xml_string = g_string_free (xml, FALSE);

        dbus_message_append_args (reply,
                                  DBUS_TYPE_STRING, &xml_string,
                                  DBUS_TYPE_INVALID);

        g_free (xml_string);

        if (reply == NULL) {
                g_error ("No memory");
        }

        if (! dbus_connection_send (connection, reply, NULL)) {
                g_error ("No memory");
        }

        dbus_message_unref (reply);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
session_message_handler (DBusConnection  *connection,
                         DBusMessage     *message,
                         void            *user_data)
{
        const char *dbus_destination = dbus_message_get_destination (message);
        const char *dbus_path        = dbus_message_get_path (message);
        const char *dbus_interface   = dbus_message_get_interface (message);
        const char *dbus_member      = dbus_message_get_member (message);

        g_debug ("session_message_handler: destination=%s obj_path=%s interface=%s method=%s",
                 dbus_destination ? dbus_destination : "(null)",
                 dbus_path        ? dbus_path        : "(null)",
                 dbus_interface   ? dbus_interface   : "(null)",
                 dbus_member      ? dbus_member      : "(null)");

        if (dbus_message_is_method_call (message, "org.freedesktop.DBus", "AddMatch")) {
                DBusMessage *reply;

                reply = dbus_message_new_method_return (message);

                if (reply == NULL) {
                        g_error ("No memory");
                }

                if (! dbus_connection_send (connection, reply, NULL)) {
                        g_error ("No memory");
                }

                dbus_message_unref (reply);

                return DBUS_HANDLER_RESULT_HANDLED;
        } else if (dbus_message_is_signal (message, DBUS_INTERFACE_LOCAL, "Disconnected") &&
                   strcmp (dbus_message_get_path (message), DBUS_PATH_LOCAL) == 0) {

                g_debug ("MdmSessionDirect: Disconnected");

                /*dbus_connection_unref (connection);*/

                return DBUS_HANDLER_RESULT_HANDLED;
        } else if (dbus_message_is_method_call (message, "org.freedesktop.DBus.Introspectable", "Introspect")) {
                return do_introspect (connection, message);
        } else {
                return session_worker_message (connection, message, user_data);
        }

        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

/* Note: Use abstract sockets like dbus does by default on Linux. Abstract
 * sockets are only available on Linux.
 */
static char *
generate_address (void)
{
        char *path;
#if defined (__linux__)
        int   i;
        char  tmp[9];

        for (i = 0; i < 8; i++) {
                if (g_random_int_range (0, 2) == 0) {
                        tmp[i] = g_random_int_range ('a', 'z' + 1);
                } else {
                        tmp[i] = g_random_int_range ('A', 'Z' + 1);
                }
        }
        tmp[8] = '\0';

        path = g_strdup_printf ("unix:abstract=/tmp/mdm-session-%s", tmp);
#else
        path = g_strdup ("unix:tmpdir=/tmp");
#endif

        return path;
}

static void
session_unregister_handler (DBusConnection  *connection,
                            void            *user_data)
{
        g_debug ("session_unregister_handler");
}

static dbus_bool_t
allow_user_function (DBusConnection *connection,
                     unsigned long   uid,
                     void           *data)
{
        if (0 == uid) {
                return TRUE;
        }

        g_debug ("MdmSessionDirect: User not allowed");

        return FALSE;
}

static void
handle_connection (DBusServer      *server,
                   DBusConnection  *new_connection,
                   void            *user_data)
{
        MdmSessionDirect *session = MDM_SESSION_DIRECT (user_data);

        g_debug ("MdmSessionDirect: Handing new connection");

        if (session->priv->worker_connection == NULL) {
                DBusObjectPathVTable vtable = { &session_unregister_handler,
                                                &session_message_handler,
                                                NULL, NULL, NULL, NULL
                };

                session->priv->worker_connection = new_connection;
                dbus_connection_ref (new_connection);
                dbus_connection_setup_with_g_main (new_connection, NULL);

                g_debug ("MdmSessionDirect: worker connection is %p", new_connection);
                dbus_connection_set_exit_on_disconnect (new_connection, FALSE);

                dbus_connection_set_unix_user_function (new_connection,
                                                        allow_user_function,
                                                        session,
                                                        NULL);

                dbus_connection_register_object_path (new_connection,
                                                      MDM_SESSION_DBUS_PATH,
                                                      &vtable,
                                                      session);

                g_debug ("MdmSessionDirect: Emitting conversation-started signal");
                _mdm_session_conversation_started (MDM_SESSION (session));
        }
}

static gboolean
setup_server (MdmSessionDirect *session)
{
        DBusError   error;
        gboolean    ret;
        char       *address;
        const char *auth_mechanisms[] = {"EXTERNAL", NULL};

        ret = FALSE;

        g_debug ("MdmSessionDirect: Creating D-Bus server for session");

        address = generate_address ();

        dbus_error_init (&error);
        session->priv->server = dbus_server_listen (address, &error);
        g_free (address);

        if (session->priv->server == NULL) {
                g_warning ("Cannot create D-BUS server for the session: %s", error.message);
                /* FIXME: should probably fail if we can't create the socket */
                goto out;
        }

        dbus_server_setup_with_g_main (session->priv->server, NULL);
        dbus_server_set_auth_mechanisms (session->priv->server, auth_mechanisms);
        dbus_server_set_new_connection_function (session->priv->server,
                                                 handle_connection,
                                                 session,
                                                 NULL);
        ret = TRUE;

        g_free (session->priv->server_address);
        session->priv->server_address = dbus_server_get_address (session->priv->server);

        g_debug ("MdmSessionDirect: D-Bus server listening on %s", session->priv->server_address);

 out:

        return ret;
}

static void
mdm_session_direct_init (MdmSessionDirect *session)
{
        session->priv = G_TYPE_INSTANCE_GET_PRIVATE (session,
                                                     MDM_TYPE_SESSION_DIRECT,
                                                     MdmSessionDirectPrivate);

        g_signal_connect (session,
                          "authentication-failed",
                          G_CALLBACK (on_authentication_failed),
                          NULL);
        g_signal_connect (session,
                          "session-started",
                          G_CALLBACK (on_session_started),
                          NULL);
        g_signal_connect (session,
                          "session-start-failed",
                          G_CALLBACK (on_session_start_failed),
                          NULL);
        g_signal_connect (session,
                          "session-exited",
                          G_CALLBACK (on_session_exited),
                          NULL);

        session->priv->session_pid = -1;

        session->priv->environment = g_hash_table_new_full (g_str_hash,
                                                            g_str_equal,
                                                            (GDestroyNotify) g_free,
                                                            (GDestroyNotify) g_free);

        setup_server (session);

}

static void
worker_started (MdmSessionWorkerJob *job,
                MdmSessionDirect    *session)
{
        g_debug ("MdmSessionDirect: Worker job started");
}

static void
worker_exited (MdmSessionWorkerJob *job,
               int                  code,
               MdmSessionDirect    *session)
{
        g_debug ("MdmSessionDirect: Worker job exited: %d", code);

        if (session->priv->is_running) {
                _mdm_session_session_exited (MDM_SESSION (session), code);
        }
}

static void
worker_died (MdmSessionWorkerJob *job,
             int                  signum,
             MdmSessionDirect    *session)
{
        g_debug ("MdmSessionDirect: Worker job died: %d", signum);

        if (session->priv->is_running) {
                _mdm_session_session_died (MDM_SESSION (session), signum);
        }
}

static gboolean
start_worker (MdmSessionDirect *session)
{
        gboolean res;

        session->priv->job = mdm_session_worker_job_new ();
        mdm_session_worker_job_set_server_address (session->priv->job, session->priv->server_address);
        g_signal_connect (session->priv->job,
                          "started",
                          G_CALLBACK (worker_started),
                          session);
        g_signal_connect (session->priv->job,
                          "exited",
                          G_CALLBACK (worker_exited),
                          session);
        g_signal_connect (session->priv->job,
                          "died",
                          G_CALLBACK (worker_died),
                          session);

        res = mdm_session_worker_job_start (session->priv->job);

        return res;
}

static void
stop_worker (MdmSessionDirect *session)
{
        g_signal_handlers_disconnect_by_func (session->priv->job,
                                              G_CALLBACK (worker_started),
                                              session);
        g_signal_handlers_disconnect_by_func (session->priv->job,
                                              G_CALLBACK (worker_exited),
                                              session);
        g_signal_handlers_disconnect_by_func (session->priv->job,
                                              G_CALLBACK (worker_died),
                                              session);

        cancel_pending_query (session);

        if (session->priv->worker_connection != NULL) {
                dbus_connection_close (session->priv->worker_connection);
                session->priv->worker_connection = NULL;
        }

        mdm_session_worker_job_stop (session->priv->job);
        g_object_unref (session->priv->job);
        session->priv->job = NULL;
}

static void
mdm_session_direct_start_conversation (MdmSession *session)
{
        MdmSessionDirect *impl = MDM_SESSION_DIRECT (session);

        g_return_if_fail (session != NULL);

        g_debug ("MdmSessionDirect: Starting conversation");

        start_worker (impl);
}

static void
send_setup (MdmSessionDirect *session,
            const char       *service_name)
{
        DBusMessage    *message;
        DBusMessageIter iter;
        const char     *display_name;
        const char     *display_device;
        const char     *display_hostname;
        const char     *display_x11_authority_file;

        g_assert (service_name != NULL);

        if (session->priv->display_name != NULL) {
                display_name = session->priv->display_name;
        } else {
                display_name = "";
        }
        if (session->priv->display_hostname != NULL) {
                display_hostname = session->priv->display_hostname;
        } else {
                display_hostname = "";
        }
        if (session->priv->display_device != NULL) {
                display_device = session->priv->display_device;
        } else {
                display_device = "";
        }
        if (session->priv->display_x11_authority_file != NULL) {
                display_x11_authority_file = session->priv->display_x11_authority_file;
        } else {
                display_x11_authority_file = "";
        }

        g_debug ("MdmSessionDirect: Beginning setup");

        message = dbus_message_new_signal (MDM_SESSION_DBUS_PATH,
                                           MDM_SESSION_DBUS_INTERFACE,
                                           "Setup");

        dbus_message_iter_init_append (message, &iter);
        dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &service_name);
        dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &display_name);
        dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &display_device);
        dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &display_hostname);
        dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &display_x11_authority_file);

        if (! send_dbus_message (session->priv->worker_connection, message)) {
                g_debug ("MdmSessionDirect: Could not send %s signal", "Setup");
        }

        dbus_message_unref (message);
}

static void
send_setup_for_user (MdmSessionDirect *session,
                     const char       *service_name)
{
        DBusMessage    *message;
        DBusMessageIter iter;
        const char     *display_name;
        const char     *display_device;
        const char     *display_hostname;
        const char     *display_x11_authority_file;
        const char     *selected_user;

        g_assert (service_name != NULL);

        if (session->priv->display_name != NULL) {
                display_name = session->priv->display_name;
        } else {
                display_name = "";
        }
        if (session->priv->display_hostname != NULL) {
                display_hostname = session->priv->display_hostname;
        } else {
                display_hostname = "";
        }
        if (session->priv->display_device != NULL) {
                display_device = session->priv->display_device;
        } else {
                display_device = "";
        }
        if (session->priv->display_x11_authority_file != NULL) {
                display_x11_authority_file = session->priv->display_x11_authority_file;
        } else {
                display_x11_authority_file = "";
        }
        if (session->priv->selected_user != NULL) {
                selected_user = session->priv->selected_user;
        } else {
                selected_user = "";
        }

        g_debug ("MdmSessionDirect: Beginning setup for user %s", session->priv->selected_user);

        message = dbus_message_new_signal (MDM_SESSION_DBUS_PATH,
                                           MDM_SESSION_DBUS_INTERFACE,
                                           "SetupForUser");

        dbus_message_iter_init_append (message, &iter);
        dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &service_name);
        dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &display_name);
        dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &display_device);
        dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &display_hostname);
        dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &display_x11_authority_file);
        dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &selected_user);

        if (! send_dbus_message (session->priv->worker_connection, message)) {
                g_debug ("MdmSessionDirect: Could not send %s signal", "SetupForUser");
        }

        dbus_message_unref (message);
}

static void
mdm_session_direct_setup (MdmSession *session,
                          const char *service_name)
{
        MdmSessionDirect *impl = MDM_SESSION_DIRECT (session);

        g_return_if_fail (session != NULL);
        g_return_if_fail (dbus_connection_get_is_connected (impl->priv->worker_connection));

        send_setup (impl, service_name);
        mdm_session_direct_defaults_changed (impl);
}

static void
mdm_session_direct_setup_for_user (MdmSession *session,
                                   const char *service_name,
                                   const char *username)
{
        MdmSessionDirect *impl = MDM_SESSION_DIRECT (session);

        g_return_if_fail (session != NULL);
        g_return_if_fail (dbus_connection_get_is_connected (impl->priv->worker_connection));
        g_return_if_fail (username != NULL);

        mdm_session_direct_select_user (session, username);

        send_setup_for_user (impl, service_name);
        mdm_session_direct_defaults_changed (impl);
}

static void
mdm_session_direct_authenticate (MdmSession *session)
{
        MdmSessionDirect *impl = MDM_SESSION_DIRECT (session);

        g_return_if_fail (session != NULL);
        g_return_if_fail (dbus_connection_get_is_connected (impl->priv->worker_connection));

        send_dbus_void_signal (impl, "Authenticate");
}

static void
mdm_session_direct_authorize (MdmSession *session)
{
        MdmSessionDirect *impl = MDM_SESSION_DIRECT (session);

        g_return_if_fail (session != NULL);
        g_return_if_fail (dbus_connection_get_is_connected (impl->priv->worker_connection));

        send_dbus_void_signal (impl, "Authorize");
}

static void
mdm_session_direct_accredit (MdmSession *session,
                             int         cred_flag)
{
        MdmSessionDirect *impl = MDM_SESSION_DIRECT (session);

        g_return_if_fail (session != NULL);
        g_return_if_fail (dbus_connection_get_is_connected (impl->priv->worker_connection));

        switch (cred_flag) {
        case MDM_SESSION_CRED_ESTABLISH:
                send_dbus_void_signal (impl, "EstablishCredentials");
                break;
        case MDM_SESSION_CRED_REFRESH:
                send_dbus_void_signal (impl, "RefreshCredentials");
                break;
        default:
                g_assert_not_reached ();
        }
}

static void
send_environment_variable (const char       *key,
                           const char       *value,
                           MdmSessionDirect *session)
{
        DBusMessage    *message;
        DBusMessageIter iter;

        message = dbus_message_new_signal (MDM_SESSION_DBUS_PATH,
                                           MDM_SESSION_DBUS_INTERFACE,
                                           "SetEnvironmentVariable");

        dbus_message_iter_init_append (message, &iter);
        dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &key);
        dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &value);

        if (! send_dbus_message (session->priv->worker_connection, message)) {
                g_debug ("MdmSessionDirect: Could not send %s signal", "SetEnvironmentVariable");
        }

        dbus_message_unref (message);
}

static void
send_environment (MdmSessionDirect *session)
{

        g_hash_table_foreach (session->priv->environment,
                              (GHFunc) send_environment_variable,
                              session);
}

static const char *
get_language_name (MdmSessionDirect *session)
{
        if (session->priv->selected_language != NULL) {
                return session->priv->selected_language;
        }

        return get_default_language_name (session);
}

static const char *
get_layout_name (MdmSessionDirect *session)
{
        if (session->priv->selected_layout != NULL) {
                return session->priv->selected_layout;
        }

        return get_default_layout_name (session);
}

static const char *
get_session_name (MdmSessionDirect *session)
{
        /* FIXME: test the session names before we use them? */

        if (session->priv->selected_session != NULL) {
                return session->priv->selected_session;
        }

        return get_default_session_name (session);
}

static char *
get_session_command (MdmSessionDirect *session)
{
        gboolean    res;
        char       *command;
        const char *session_name;

        session_name = get_session_name (session);

        command = NULL;
        res = get_session_command_for_name (session_name, &command);
        if (! res) {
                g_critical ("Cannot find a command for specified session: %s", session_name);
                exit (1);
        }

        return command;
}

static void
mdm_session_direct_set_environment_variable (MdmSessionDirect *session,
                                             const char       *key,
                                             const char       *value)
{
        g_return_if_fail (session != NULL);
        g_return_if_fail (session != NULL);
        g_return_if_fail (key != NULL);
        g_return_if_fail (value != NULL);

        g_hash_table_replace (session->priv->environment,
                              g_strdup (key),
                              g_strdup (value));
}

static void
setup_session_environment (MdmSessionDirect *session)
{
        mdm_session_direct_set_environment_variable (session,
                                                     "MDMSESSION",
                                                     get_session_name (session));
        mdm_session_direct_set_environment_variable (session,
                                                     "DESKTOP_SESSION",
                                                     get_session_name (session));

        mdm_session_direct_set_environment_variable (session,
                                                     "LANG",
                                                     get_language_name (session));
        mdm_session_direct_set_environment_variable (session,
                                                     "MDM_LANG",
                                                     get_language_name (session));

        if (strcmp (get_layout_name (session),
                    get_default_layout_name (session)) == 0) {
                mdm_session_direct_set_environment_variable (session,
                                                             "MDM_KEYBOARD_LAYOUT",
                                                             get_layout_name (session));
        }

        mdm_session_direct_set_environment_variable (session,
                                                     "DISPLAY",
                                                     session->priv->display_name);

        if (session->priv->user_x11_authority_file != NULL) {
                mdm_session_direct_set_environment_variable (session,
                                                             "XAUTHORITY",
                                                             session->priv->user_x11_authority_file);
        }

        if (g_getenv ("WINDOWPATH") != NULL) {
                mdm_session_direct_set_environment_variable (session,
                                                             "WINDOWPATH",
                                                             g_getenv ("WINDOWPATH"));
        }


        /* FIXME: We do this here and in the session worker.  We should consolidate
         * somehow.
         */
        mdm_session_direct_set_environment_variable (session,
                                                     "PATH",
                                                     strcmp (BINDIR, "/usr/bin") == 0?
                                                     MDM_SESSION_DEFAULT_PATH :
                                                     BINDIR ":" MDM_SESSION_DEFAULT_PATH);

}

static void
mdm_session_direct_open_session (MdmSession *session)
{
        MdmSessionDirect *impl = MDM_SESSION_DIRECT (session);

        g_return_if_fail (session != NULL);

        send_dbus_void_signal (impl, "OpenSession");
}

static void
mdm_session_direct_start_session (MdmSession *session)
{
        MdmSessionDirect *impl = MDM_SESSION_DIRECT (session);
        char             *command;
        char             *program;

        g_return_if_fail (session != NULL);
        g_return_if_fail (impl->priv->is_running == FALSE);

        command = get_session_command (impl);

        if (mdm_session_direct_bypasses_xsession (impl)) {
                program = g_strdup (command);
        } else {
                program = g_strdup_printf (MDMCONFDIR "/Xsession \"%s\"", command);
        }

        g_free (command);

        setup_session_environment (impl);
        send_environment (impl);

        send_dbus_string_signal (impl, "StartProgram", program);
        g_free (program);
}

static void
mdm_session_direct_close (MdmSession *session)
{
        MdmSessionDirect *impl = MDM_SESSION_DIRECT (session);

        g_return_if_fail (session != NULL);

        g_debug ("MdmSessionDirect: Closing session");

        if (impl->priv->job != NULL) {
                if (impl->priv->is_running) {
                        mdm_session_record_logout (impl->priv->session_pid,
                                                   impl->priv->selected_user,
                                                   impl->priv->display_hostname,
                                                   impl->priv->display_name,
                                                   impl->priv->display_device);
                }

                stop_worker (impl);
        }

        g_free (impl->priv->selected_user);
        impl->priv->selected_user = NULL;

        g_free (impl->priv->selected_session);
        impl->priv->selected_session = NULL;

        g_free (impl->priv->saved_session);
        impl->priv->saved_session = NULL;

        g_free (impl->priv->selected_language);
        impl->priv->selected_language = NULL;

        g_free (impl->priv->saved_language);
        impl->priv->saved_language = NULL;

        g_free (impl->priv->selected_layout);
        impl->priv->selected_layout = NULL;

        g_free (impl->priv->saved_layout);
        impl->priv->saved_layout = NULL;

        g_free (impl->priv->user_x11_authority_file);
        impl->priv->user_x11_authority_file = NULL;

        g_hash_table_remove_all (impl->priv->environment);

        impl->priv->session_pid = -1;
        impl->priv->is_running = FALSE;
}

static void
mdm_session_direct_answer_query  (MdmSession *session,
                                  const char *text)
{
        MdmSessionDirect *impl = MDM_SESSION_DIRECT (session);

        g_return_if_fail (session != NULL);

        answer_pending_query (impl, text);
}

static void
mdm_session_direct_cancel  (MdmSession *session)
{
        MdmSessionDirect *impl = MDM_SESSION_DIRECT (session);

        g_return_if_fail (session != NULL);

        cancel_pending_query (impl);
}

char *
mdm_session_direct_get_username (MdmSessionDirect *session)
{
        g_return_val_if_fail (session != NULL, NULL);

        return g_strdup (session->priv->selected_user);
}

char *
mdm_session_direct_get_display_device (MdmSessionDirect *session)
{
        g_return_val_if_fail (session != NULL, NULL);

        return g_strdup (session->priv->display_device);
}

gboolean
mdm_session_direct_bypasses_xsession (MdmSessionDirect *session_direct)
{
        GError     *error;
        GKeyFile   *key_file;
        gboolean    res;
        gboolean    bypasses_xsession = FALSE;
        char       *filename;

        g_return_val_if_fail (session_direct != NULL, FALSE);
        g_return_val_if_fail (MDM_IS_SESSION_DIRECT (session_direct), FALSE);

        filename = g_strdup_printf ("%s.desktop", get_session_name (session_direct));

        key_file = g_key_file_new ();
        error = NULL;
        res = g_key_file_load_from_dirs (key_file,
                                         filename,
                                         get_system_session_dirs (),
                                         NULL,
                                         G_KEY_FILE_NONE,
                                         &error);
        if (! res) {
                g_debug ("MdmSessionDirect: File '%s' not found: %s", filename, error->message);
                goto out;
        }

        error = NULL;
        res = g_key_file_has_key (key_file, G_KEY_FILE_DESKTOP_GROUP, "X-MDM-BypassXsession", NULL);
        if (!res) {
                goto out;
        } else {
                bypasses_xsession = g_key_file_get_boolean (key_file, G_KEY_FILE_DESKTOP_GROUP, "X-MDM-BypassXsession", &error);
                if (error) {
                        bypasses_xsession = FALSE;
                        g_error_free (error);
                        goto out;
                }
                if (bypasses_xsession) {
                        g_debug ("MdmSessionDirect: Session %s bypasses Xsession wrapper script", filename);
                }
        }

out:
        g_free (filename);
        return bypasses_xsession;
}

static void
mdm_session_direct_select_session (MdmSession *session,
                                   const char *text)
{
        MdmSessionDirect *impl = MDM_SESSION_DIRECT (session);

        g_free (impl->priv->selected_session);

        if (strcmp (text, "__previous") == 0) {
                impl->priv->selected_session = NULL;
        } else {
                impl->priv->selected_session = g_strdup (text);
        }

        send_dbus_string_signal (impl, "SetSessionName",
                                 get_session_name (impl));
}

static void
mdm_session_direct_select_language (MdmSession *session,
                                    const char *text)
{
        MdmSessionDirect *impl = MDM_SESSION_DIRECT (session);

        g_free (impl->priv->selected_language);

        if (strcmp (text, "__previous") == 0) {
                impl->priv->selected_language = NULL;
        } else {
                impl->priv->selected_language = g_strdup (text);
        }

        send_dbus_string_signal (impl, "SetLanguageName",
                                 get_language_name (impl));
}

static void
mdm_session_direct_select_layout (MdmSession *session,
                                  const char *text)
{
        MdmSessionDirect *impl = MDM_SESSION_DIRECT (session);

        g_free (impl->priv->selected_layout);

        if (strcmp (text, "__previous") == 0) {
                impl->priv->selected_layout = NULL;
        } else {
                impl->priv->selected_layout = g_strdup (text);
        }

        send_dbus_string_signal (impl, "SetLayoutName",
                                 get_layout_name (impl));
}

static void
_mdm_session_direct_set_display_id (MdmSessionDirect *session,
                                    const char       *id)
{
        g_free (session->priv->display_id);
        session->priv->display_id = g_strdup (id);
}

/* At some point we may want to read these right from
 * the slave but for now I don't want the dependency */
static void
_mdm_session_direct_set_display_name (MdmSessionDirect *session,
                                      const char       *name)
{
        g_free (session->priv->display_name);
        session->priv->display_name = g_strdup (name);
}

static void
_mdm_session_direct_set_display_hostname (MdmSessionDirect *session,
                                          const char       *name)
{
        g_free (session->priv->display_hostname);
        session->priv->display_hostname = g_strdup (name);
}

static void
_mdm_session_direct_set_display_device (MdmSessionDirect *session,
                                        const char       *name)
{
        g_debug ("MdmSessionDirect: Setting display device: %s", name);
        g_free (session->priv->display_device);
        session->priv->display_device = g_strdup (name);
}

static void
_mdm_session_direct_set_user_x11_authority_file (MdmSessionDirect *session,
                                                 const char       *name)
{
        g_free (session->priv->user_x11_authority_file);
        session->priv->user_x11_authority_file = g_strdup (name);
}

static void
_mdm_session_direct_set_display_x11_authority_file (MdmSessionDirect *session,
                                                    const char       *name)
{
        g_free (session->priv->display_x11_authority_file);
        session->priv->display_x11_authority_file = g_strdup (name);
}

static void
_mdm_session_direct_set_display_is_local (MdmSessionDirect *session,
                                          gboolean          is)
{
        session->priv->display_is_local = is;
}

static void
mdm_session_direct_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
        MdmSessionDirect *self;

        self = MDM_SESSION_DIRECT (object);

        switch (prop_id) {
        case PROP_DISPLAY_ID:
                _mdm_session_direct_set_display_id (self, g_value_get_string (value));
                break;
        case PROP_DISPLAY_NAME:
                _mdm_session_direct_set_display_name (self, g_value_get_string (value));
                break;
        case PROP_DISPLAY_HOSTNAME:
                _mdm_session_direct_set_display_hostname (self, g_value_get_string (value));
                break;
        case PROP_DISPLAY_DEVICE:
                _mdm_session_direct_set_display_device (self, g_value_get_string (value));
                break;
        case PROP_USER_X11_AUTHORITY_FILE:
                _mdm_session_direct_set_user_x11_authority_file (self, g_value_get_string (value));
                break;
        case PROP_DISPLAY_X11_AUTHORITY_FILE:
                _mdm_session_direct_set_display_x11_authority_file (self, g_value_get_string (value));
                break;
        case PROP_DISPLAY_IS_LOCAL:
                _mdm_session_direct_set_display_is_local (self, g_value_get_boolean (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
mdm_session_direct_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
        MdmSessionDirect *self;

        self = MDM_SESSION_DIRECT (object);

        switch (prop_id) {
        case PROP_DISPLAY_ID:
                g_value_set_string (value, self->priv->display_id);
                break;
        case PROP_DISPLAY_NAME:
                g_value_set_string (value, self->priv->display_name);
                break;
        case PROP_DISPLAY_HOSTNAME:
                g_value_set_string (value, self->priv->display_hostname);
                break;
        case PROP_DISPLAY_DEVICE:
                g_value_set_string (value, self->priv->display_device);
                break;
        case PROP_USER_X11_AUTHORITY_FILE:
                g_value_set_string (value, self->priv->user_x11_authority_file);
                break;
        case PROP_DISPLAY_X11_AUTHORITY_FILE:
                g_value_set_string (value, self->priv->display_x11_authority_file);
                break;
        case PROP_DISPLAY_IS_LOCAL:
                g_value_set_boolean (value, self->priv->display_is_local);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
mdm_session_direct_dispose (GObject *object)
{
        MdmSessionDirect *session;

        session = MDM_SESSION_DIRECT (object);

        g_debug ("MdmSessionDirect: Disposing session");

        mdm_session_direct_close (MDM_SESSION (session));

        g_free (session->priv->display_id);
        session->priv->display_id = NULL;

        g_free (session->priv->display_name);
        session->priv->display_name = NULL;

        g_free (session->priv->display_hostname);
        session->priv->display_hostname = NULL;

        g_free (session->priv->display_device);
        session->priv->display_device = NULL;

        g_free (session->priv->display_x11_authority_file);
        session->priv->display_x11_authority_file = NULL;

        g_free (session->priv->server_address);
        session->priv->server_address = NULL;

        if (session->priv->server != NULL) {
                dbus_server_disconnect (session->priv->server);
                dbus_server_unref (session->priv->server);
                session->priv->server = NULL;
        }

        if (session->priv->environment != NULL) {
                g_hash_table_destroy (session->priv->environment);
                session->priv->environment = NULL;
        }

        G_OBJECT_CLASS (mdm_session_direct_parent_class)->dispose (object);
}

static void
mdm_session_direct_finalize (GObject *object)
{
        MdmSessionDirect *session;
        GObjectClass     *parent_class;

        session = MDM_SESSION_DIRECT (object);

        g_free (session->priv->id);

        g_free (session->priv->selected_user);
        g_free (session->priv->selected_session);
        g_free (session->priv->saved_session);
        g_free (session->priv->selected_language);
        g_free (session->priv->saved_language);
        g_free (session->priv->selected_layout);
        g_free (session->priv->saved_layout);

        g_free (session->priv->fallback_session_name);

        parent_class = G_OBJECT_CLASS (mdm_session_direct_parent_class);

        if (parent_class->finalize != NULL)
                parent_class->finalize (object);
}

static gboolean
register_session (MdmSessionDirect *session)
{
        GError *error;

        error = NULL;
        session->priv->connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
        if (session->priv->connection == NULL) {
                if (error != NULL) {
                        g_critical ("error getting system bus: %s", error->message);
                        g_error_free (error);
                }
                exit (1);
        }

        dbus_g_connection_register_g_object (session->priv->connection, session->priv->id, G_OBJECT (session));

        return TRUE;
}

static GObject *
mdm_session_direct_constructor (GType                  type,
                                guint                  n_construct_properties,
                                GObjectConstructParam *construct_properties)
{
        MdmSessionDirect *session;
        gboolean          res;
        const char       *id;

        session = MDM_SESSION_DIRECT (G_OBJECT_CLASS (mdm_session_direct_parent_class)->constructor (type,
                                                                                          n_construct_properties,
                                                                                          construct_properties));

        /* Always match the session id with the master */
        id = NULL;
        if (g_str_has_prefix (session->priv->display_id, "/org/mate/DisplayManager/Display")) {
                id = session->priv->display_id + strlen ("/org/mate/DisplayManager/Display");
        }

        g_assert (id != NULL);

        session->priv->id = g_strdup_printf ("/org/mate/DisplayManager/Session%s", id);
        g_debug ("MdmSessionDirect: Registering %s", session->priv->id);

        res = register_session (session);
        if (! res) {
                g_warning ("Unable to register session with system bus");
        }

        return G_OBJECT (session);
}

static void
mdm_session_iface_init (MdmSessionIface *iface)
{
        iface->start_conversation = mdm_session_direct_start_conversation;
        iface->setup = mdm_session_direct_setup;
        iface->setup_for_user = mdm_session_direct_setup_for_user;
        iface->authenticate = mdm_session_direct_authenticate;
        iface->authorize = mdm_session_direct_authorize;
        iface->accredit = mdm_session_direct_accredit;
        iface->open_session = mdm_session_direct_open_session;
        iface->close = mdm_session_direct_close;

        iface->cancel = mdm_session_direct_cancel;
        iface->start_session = mdm_session_direct_start_session;
        iface->answer_query = mdm_session_direct_answer_query;
        iface->select_session = mdm_session_direct_select_session;
        iface->select_language = mdm_session_direct_select_language;
        iface->select_layout = mdm_session_direct_select_layout;
        iface->select_user = mdm_session_direct_select_user;
}

static void
mdm_session_direct_class_init (MdmSessionDirectClass *session_class)
{
        GObjectClass *object_class;

        object_class = G_OBJECT_CLASS (session_class);

        object_class->get_property = mdm_session_direct_get_property;
        object_class->set_property = mdm_session_direct_set_property;
        object_class->constructor = mdm_session_direct_constructor;
        object_class->dispose = mdm_session_direct_dispose;
        object_class->finalize = mdm_session_direct_finalize;

        g_type_class_add_private (session_class, sizeof (MdmSessionDirectPrivate));

        g_object_class_install_property (object_class,
                                         PROP_DISPLAY_ID,
                                         g_param_spec_string ("display-id",
                                                              "display id",
                                                              "display id",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
        g_object_class_install_property (object_class,
                                         PROP_DISPLAY_NAME,
                                         g_param_spec_string ("display-name",
                                                              "display name",
                                                              "display name",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
        g_object_class_install_property (object_class,
                                         PROP_DISPLAY_HOSTNAME,
                                         g_param_spec_string ("display-hostname",
                                                              "display hostname",
                                                              "display hostname",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
        g_object_class_install_property (object_class,
                                         PROP_DISPLAY_IS_LOCAL,
                                         g_param_spec_boolean ("display-is-local",
                                                               "display is local",
                                                               "display is local",
                                                               TRUE,
                                                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
        g_object_class_install_property (object_class,
                                         PROP_DISPLAY_X11_AUTHORITY_FILE,
                                         g_param_spec_string ("display-x11-authority-file",
                                                              "display x11 authority file",
                                                              "display x11 authority file",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
        /* not construct only */
        g_object_class_install_property (object_class,
                                         PROP_USER_X11_AUTHORITY_FILE,
                                         g_param_spec_string ("user-x11-authority-file",
                                                              "",
                                                              "",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
        g_object_class_install_property (object_class,
                                         PROP_DISPLAY_DEVICE,
                                         g_param_spec_string ("display-device",
                                                              "display device",
                                                              "display device",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT));


        dbus_g_object_type_install_info (MDM_TYPE_SESSION_DIRECT, &dbus_glib_mdm_session_direct_object_info);
}

MdmSessionDirect *
mdm_session_direct_new (const char *display_id,
                        const char *display_name,
                        const char *display_hostname,
                        const char *display_device,
                        const char *display_x11_authority_file,
                        gboolean    display_is_local)
{
        MdmSessionDirect *session;

        session = g_object_new (MDM_TYPE_SESSION_DIRECT,
                                "display-id", display_id,
                                "display-name", display_name,
                                "display-hostname", display_hostname,
                                "display-device", display_device,
                                "display-x11-authority-file", display_x11_authority_file,
                                "display-is-local", display_is_local,
                                NULL);

        return session;
}

gboolean
mdm_session_direct_restart (MdmSessionDirect *session,
                            GError          **error)
{
        gboolean ret;

        ret = TRUE;
        g_debug ("MdmSessionDirect: Request to restart session");

        return ret;
}

gboolean
mdm_session_direct_stop (MdmSessionDirect *session,
                         GError          **error)
{
        gboolean ret;

        ret = TRUE;

        g_debug ("MdmSessionDirect: Request to stop session");

        return ret;
}

gboolean
mdm_session_direct_detach (MdmSessionDirect *session,
                           GError          **error)
{
        gboolean ret;

        ret = TRUE;

        g_debug ("MdmSessionDirect: Request to detach session");

        return ret;
}
