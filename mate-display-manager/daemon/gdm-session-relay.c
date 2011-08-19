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

#if defined (_POSIX_PRIORITY_SCHEDULING) && defined (HAVE_SCHED_YIELD)
#include <sched.h>
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "gdm-session-private.h"
#include "gdm-session-relay.h"

#define GDM_SESSION_RELAY_DBUS_PATH      "/org/mate/DisplayManager/SessionRelay"
#define GDM_SESSION_RELAY_DBUS_INTERFACE "org.mate.DisplayManager.SessionRelay"

#define GDM_SESSION_RELAY_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GDM_TYPE_SESSION_RELAY, GdmSessionRelayPrivate))

struct GdmSessionRelayPrivate
{
        DBusServer     *server;
        char           *server_address;
        DBusConnection *session_connection;
};

enum {
        PROP_0,
};

enum {
        CONNECTED,
        DISCONNECTED,
        LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0, };

static void     gdm_session_relay_class_init    (GdmSessionRelayClass *klass);
static void     gdm_session_relay_init          (GdmSessionRelay      *session_relay);
static void     gdm_session_relay_finalize      (GObject              *object);
static void     gdm_session_iface_init          (GdmSessionIface      *iface);

G_DEFINE_TYPE_WITH_CODE (GdmSessionRelay,
                         gdm_session_relay,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GDM_TYPE_SESSION,
                                                gdm_session_iface_init))

static gboolean
send_dbus_message (DBusConnection *connection,
                   DBusMessage    *message)
{
        gboolean is_connected;
        gboolean sent;

        g_return_val_if_fail (message != NULL, FALSE);

        if (connection == NULL) {
                g_debug ("GdmSessionRelay: There is no valid connection");
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
send_dbus_string_signal (GdmSessionRelay *session_relay,
                         const char      *name,
                         const char      *text)
{
        DBusMessage    *message;
        DBusMessageIter iter;

        g_return_if_fail (session_relay != NULL);

        g_debug ("GdmSessionRelay: sending signal %s", name);
        message = dbus_message_new_signal (GDM_SESSION_RELAY_DBUS_PATH,
                                           GDM_SESSION_RELAY_DBUS_INTERFACE,
                                           name);

        dbus_message_iter_init_append (message, &iter);
        dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &text);

        if (! send_dbus_message (session_relay->priv->session_connection, message)) {
                g_debug ("GdmSessionRelay: Could not send %s signal", name);
        }

        dbus_message_unref (message);
}

static void
send_dbus_string_string_signal (GdmSessionRelay *session_relay,
                                const char      *name,
                                const char      *text1,
                                const char      *text2)
{
        DBusMessage    *message;
        DBusMessageIter iter;

        g_return_if_fail (session_relay != NULL);

        g_debug ("GdmSessionRelay: sending signal %s", name);
        message = dbus_message_new_signal (GDM_SESSION_RELAY_DBUS_PATH,
                                           GDM_SESSION_RELAY_DBUS_INTERFACE,
                                           name);

        dbus_message_iter_init_append (message, &iter);
        dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &text1);
        dbus_message_iter_init_append (message, &iter);
        dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &text2);

        if (! send_dbus_message (session_relay->priv->session_connection, message)) {
                g_debug ("GdmSessionRelay: Could not send %s signal", name);
        }

        dbus_message_unref (message);
}

static void
send_dbus_void_signal (GdmSessionRelay *session_relay,
                       const char      *name)
{
        DBusMessage    *message;

        g_return_if_fail (session_relay != NULL);

        g_debug ("GdmSessionRelay: sending signal %s", name);
        message = dbus_message_new_signal (GDM_SESSION_RELAY_DBUS_PATH,
                                           GDM_SESSION_RELAY_DBUS_INTERFACE,
                                           name);

        if (! send_dbus_message (session_relay->priv->session_connection, message)) {
                g_debug ("GdmSessionRelay: Could not send %s signal", name);
        }

        dbus_message_unref (message);
}

static void
gdm_session_relay_start_conversation (GdmSession *session)
{
        GdmSessionRelay *impl = GDM_SESSION_RELAY (session);
        send_dbus_void_signal (impl, "StartConversation");
}

static void
gdm_session_relay_close (GdmSession *session)
{
        GdmSessionRelay *impl = GDM_SESSION_RELAY (session);
        send_dbus_void_signal (impl, "Close");
}

static void
gdm_session_relay_setup (GdmSession *session,
                         const char *service_name)
{
        GdmSessionRelay *impl = GDM_SESSION_RELAY (session);
        send_dbus_string_signal (impl, "Setup", service_name);
}

static void
gdm_session_relay_setup_for_user (GdmSession *session,
                                  const char *service_name,
                                  const char *username)
{
        GdmSessionRelay *impl = GDM_SESSION_RELAY (session);
        send_dbus_string_string_signal (impl, "SetupForUser", service_name, username);
}

static void
gdm_session_relay_authenticate (GdmSession *session)
{
        GdmSessionRelay *impl = GDM_SESSION_RELAY (session);
        send_dbus_void_signal (impl, "Authenticate");
}

static void
gdm_session_relay_authorize (GdmSession *session)
{
        GdmSessionRelay *impl = GDM_SESSION_RELAY (session);
        send_dbus_void_signal (impl, "Authorize");
}

static void
gdm_session_relay_accredit (GdmSession *session,
                            int         cred_flag)
{
        GdmSessionRelay *impl = GDM_SESSION_RELAY (session);

        switch (cred_flag) {
        case GDM_SESSION_CRED_ESTABLISH:
                send_dbus_void_signal (impl, "EstablishCredentials");
                break;
        case GDM_SESSION_CRED_REFRESH:
                send_dbus_void_signal (impl, "RefreshCredentials");
                break;
        default:
                g_assert_not_reached ();
        }
}

static void
gdm_session_relay_open_session (GdmSession *session)
{
        GdmSessionRelay *impl = GDM_SESSION_RELAY (session);
        send_dbus_void_signal (impl, "OpenSession");
}

static void
gdm_session_relay_answer_query (GdmSession *session,
                                const char *text)
{
        GdmSessionRelay *impl = GDM_SESSION_RELAY (session);
        send_dbus_string_signal (impl, "AnswerQuery", text);
}

static void
gdm_session_relay_select_session (GdmSession *session,
                                  const char *text)
{
        GdmSessionRelay *impl = GDM_SESSION_RELAY (session);
        send_dbus_string_signal (impl, "SessionSelected", text);
}

static void
gdm_session_relay_select_language (GdmSession *session,
                                   const char *text)
{
        GdmSessionRelay *impl = GDM_SESSION_RELAY (session);
        send_dbus_string_signal (impl, "LanguageSelected", text);
}

static void
gdm_session_relay_select_layout (GdmSession *session,
                                 const char *text)
{
        GdmSessionRelay *impl = GDM_SESSION_RELAY (session);
        send_dbus_string_signal (impl, "LayoutSelected", text);
}

static void
gdm_session_relay_select_user (GdmSession *session,
                               const char *text)
{
        GdmSessionRelay *impl = GDM_SESSION_RELAY (session);
        send_dbus_string_signal (impl, "UserSelected", text);
}

static void
gdm_session_relay_cancel (GdmSession *session)
{
        GdmSessionRelay *impl = GDM_SESSION_RELAY (session);

        send_dbus_void_signal (impl, "Cancelled");
}

static void
gdm_session_relay_start_session (GdmSession *session)
{
        GdmSessionRelay *impl = GDM_SESSION_RELAY (session);

        send_dbus_void_signal (impl, "StartSession");
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

        path = g_strdup_printf ("unix:abstract=/tmp/gdm-session-%s", tmp);
#else
        path = g_strdup ("unix:tmpdir=/tmp");
#endif

        return path;
}

static DBusHandlerResult
handle_info_query (GdmSessionRelay *session_relay,
                   DBusConnection  *connection,
                   DBusMessage     *message)
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

        g_debug ("GdmSessionRelay: InfoQuery: %s", text);

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        _gdm_session_info_query (GDM_SESSION (session_relay), text);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_secret_info_query (GdmSessionRelay *session_relay,
                          DBusConnection  *connection,
                          DBusMessage     *message)
{
        DBusMessage *reply;
        DBusError    error;
        const char  *text;

        text = NULL;

        dbus_error_init (&error);
        if (! dbus_message_get_args (message, &error,
                                     DBUS_TYPE_STRING, &text,
                                     DBUS_TYPE_INVALID)) {
                g_warning ("ERROR: %s", error.message);
        }

        g_debug ("GdmSessionRelay: SecretInfoQuery: %s", text);

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        _gdm_session_secret_info_query (GDM_SESSION (session_relay), text);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_info (GdmSessionRelay *session_relay,
             DBusConnection  *connection,
             DBusMessage     *message)
{
        DBusMessage *reply;
        DBusError    error;
        const char  *text;

        text = NULL;

        dbus_error_init (&error);
        if (! dbus_message_get_args (message, &error,
                                     DBUS_TYPE_STRING, &text,
                                     DBUS_TYPE_INVALID)) {
                g_warning ("ERROR: %s", error.message);
        }

        g_debug ("GdmSessionRelay: Info: %s", text);

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        _gdm_session_info (GDM_SESSION (session_relay), text);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_problem (GdmSessionRelay *session_relay,
             DBusConnection  *connection,
             DBusMessage     *message)
{
        DBusMessage *reply;
        DBusError    error;
        const char  *text;

        text = NULL;

        dbus_error_init (&error);
        if (! dbus_message_get_args (message, &error,
                                     DBUS_TYPE_STRING, &text,
                                     DBUS_TYPE_INVALID)) {
                g_warning ("ERROR: %s", error.message);
        }

        g_debug ("GdmSessionRelay: Problem: %s", text);

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        _gdm_session_problem (GDM_SESSION (session_relay), text);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_setup_complete (GdmSessionRelay *session_relay,
                       DBusConnection  *connection,
                       DBusMessage     *message)
{
        DBusMessage *reply;
        DBusError    error;

        dbus_error_init (&error);

        g_debug ("GdmSessionRelay: SetupComplete");

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        _gdm_session_setup_complete (GDM_SESSION (session_relay));

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_setup_failed (GdmSessionRelay *session_relay,
                     DBusConnection  *connection,
                     DBusMessage     *message)
{
        DBusMessage *reply;
        DBusError    error;

        dbus_error_init (&error);

        g_debug ("GdmSessionRelay: SetupFailed");

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        _gdm_session_setup_failed (GDM_SESSION (session_relay), NULL);

        return DBUS_HANDLER_RESULT_HANDLED;
}


static DBusHandlerResult
handle_authenticated (GdmSessionRelay *session_relay,
                      DBusConnection  *connection,
                      DBusMessage     *message)
{
        DBusMessage *reply;
        DBusError    error;

        dbus_error_init (&error);

        g_debug ("GdmSessionRelay: Authenticated");

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        _gdm_session_authenticated (GDM_SESSION (session_relay));

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_authentication_failed (GdmSessionRelay *session_relay,
                              DBusConnection  *connection,
                              DBusMessage     *message)
{
        DBusMessage *reply;
        DBusError    error;

        dbus_error_init (&error);

        g_debug ("GdmSessionRelay: AuthenticationFailed");

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        _gdm_session_authentication_failed (GDM_SESSION (session_relay), NULL);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_authorized (GdmSessionRelay *session_relay,
                   DBusConnection  *connection,
                   DBusMessage     *message)
{
        DBusMessage *reply;
        DBusError    error;

        dbus_error_init (&error);

        g_debug ("GdmSessionRelay: Authorized");

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        _gdm_session_authorized (GDM_SESSION (session_relay));

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_authorization_failed (GdmSessionRelay *session_relay,
                             DBusConnection  *connection,
                             DBusMessage     *message)
{
        DBusMessage *reply;
        DBusError    error;

        dbus_error_init (&error);

        g_debug ("GdmSessionRelay: AuthorizationFailed");

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        _gdm_session_authorization_failed (GDM_SESSION (session_relay), NULL);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_accredited (GdmSessionRelay *session_relay,
                   DBusConnection  *connection,
                   DBusMessage     *message)
{
        DBusMessage *reply;
        DBusError    error;

        dbus_error_init (&error);

        g_debug ("GdmSessionRelay: Accredited");

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        _gdm_session_accredited (GDM_SESSION (session_relay));

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_accreditation_failed (GdmSessionRelay *session_relay,
                             DBusConnection  *connection,
                             DBusMessage     *message)
{
        DBusMessage *reply;
        DBusError    error;

        dbus_error_init (&error);

        g_debug ("GdmSessionRelay: AccreditationFailed");

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        _gdm_session_accreditation_failed (GDM_SESSION (session_relay), NULL);

        return DBUS_HANDLER_RESULT_HANDLED;
}
static DBusHandlerResult
handle_session_opened (GdmSessionRelay *session_relay,
                       DBusConnection  *connection,
                       DBusMessage     *message)
{
        DBusMessage *reply;
        DBusError    error;

        dbus_error_init (&error);
        if (! dbus_message_get_args (message, &error,
                                     DBUS_TYPE_INVALID)) {
                g_warning ("ERROR: %s", error.message);
        }
        dbus_error_free (&error);

        g_debug ("GdmSessionRelay: Session Opened");

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        _gdm_session_session_opened (GDM_SESSION (session_relay));

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_session_open_failed (GdmSessionRelay *session_relay,
                            DBusConnection  *connection,
                            DBusMessage     *message)
{
        DBusMessage *reply;
        DBusError    error;

        dbus_error_init (&error);
        if (! dbus_message_get_args (message, &error,
                                     DBUS_TYPE_INVALID)) {
                g_warning ("ERROR: %s", error.message);
        }
        dbus_error_free (&error);

        g_debug ("GdmSessionRelay: Session Open Failed");

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        _gdm_session_session_open_failed (GDM_SESSION (session_relay), NULL);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_session_started (GdmSessionRelay *session_relay,
                        DBusConnection  *connection,
                        DBusMessage     *message)
{
        DBusMessage *reply;
        DBusError    error;
        int          pid;

        dbus_error_init (&error);

        pid = 0;
        if (! dbus_message_get_args (message,
                                     &error,
                                     DBUS_TYPE_INT32, &pid,
                                     DBUS_TYPE_INVALID)) {
                g_warning ("ERROR: %s", error.message);
        }

        g_debug ("GdmSessionRelay: SessionStarted");

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        _gdm_session_session_started (GDM_SESSION (session_relay),
                                      pid);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_session_stopped (GdmSessionRelay *session_relay,
                        DBusConnection  *connection,
                        DBusMessage     *message)
{
        DBusMessage *reply;
        DBusError    error;

        dbus_error_init (&error);

        g_debug ("GdmSessionRelay: SessionStopped");

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

#if 0
        _gdm_session_session_stopped (GDM_SESSION (session_relay));
#endif

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_conversation_started (GdmSessionRelay *session_relay,
                             DBusConnection  *connection,
                             DBusMessage     *message)
{
        DBusMessage *reply;
        DBusError    error;

        dbus_error_init (&error);

        g_debug ("GdmSessionRelay: Conversation Started");

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        _gdm_session_conversation_started (GDM_SESSION (session_relay));

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
session_handle_child_message (DBusConnection *connection,
                              DBusMessage    *message,
                              void           *user_data)
{
        GdmSessionRelay *session_relay = GDM_SESSION_RELAY (user_data);

        if (dbus_message_is_method_call (message, GDM_SESSION_RELAY_DBUS_INTERFACE, "InfoQuery")) {
                return handle_info_query (session_relay, connection, message);
        } else if (dbus_message_is_method_call (message, GDM_SESSION_RELAY_DBUS_INTERFACE, "SecretInfoQuery")) {
                return handle_secret_info_query (session_relay, connection, message);
        } else if (dbus_message_is_method_call (message, GDM_SESSION_RELAY_DBUS_INTERFACE, "Info")) {
                return handle_info (session_relay, connection, message);
        } else if (dbus_message_is_method_call (message, GDM_SESSION_RELAY_DBUS_INTERFACE, "Problem")) {
                return handle_problem (session_relay, connection, message);
        } else if (dbus_message_is_method_call (message, GDM_SESSION_RELAY_DBUS_INTERFACE, "SetupComplete")) {
                return handle_setup_complete (session_relay, connection, message);
        } else if (dbus_message_is_method_call (message, GDM_SESSION_RELAY_DBUS_INTERFACE, "SetupFailed")) {
                return handle_setup_failed (session_relay, connection, message);
        } else if (dbus_message_is_method_call (message, GDM_SESSION_RELAY_DBUS_INTERFACE, "Authenticated")) {
                return handle_authenticated (session_relay, connection, message);
        } else if (dbus_message_is_method_call (message, GDM_SESSION_RELAY_DBUS_INTERFACE, "AuthenticationFailed")) {
                return handle_authentication_failed (session_relay, connection, message);
        } else if (dbus_message_is_method_call (message, GDM_SESSION_RELAY_DBUS_INTERFACE, "Authorized")) {
                return handle_authorized (session_relay, connection, message);
        } else if (dbus_message_is_method_call (message, GDM_SESSION_RELAY_DBUS_INTERFACE, "AuthorizationFailed")) {
                return handle_authorization_failed (session_relay, connection, message);
        } else if (dbus_message_is_method_call (message, GDM_SESSION_RELAY_DBUS_INTERFACE, "Accredited")) {
                return handle_accredited (session_relay, connection, message);
        } else if (dbus_message_is_method_call (message, GDM_SESSION_RELAY_DBUS_INTERFACE, "AccreditationFailed")) {
                return handle_accreditation_failed (session_relay, connection, message);
        } else if (dbus_message_is_method_call (message, GDM_SESSION_RELAY_DBUS_INTERFACE, "SessionOpened")) {
                return handle_session_opened (session_relay, connection, message);
        } else if (dbus_message_is_method_call (message, GDM_SESSION_RELAY_DBUS_INTERFACE, "SessionOpenFailed")) {
                return handle_session_open_failed (session_relay, connection, message);
        } else if (dbus_message_is_method_call (message, GDM_SESSION_RELAY_DBUS_INTERFACE, "SessionStarted")) {
                return handle_session_started (session_relay, connection, message);
        } else if (dbus_message_is_method_call (message, GDM_SESSION_RELAY_DBUS_INTERFACE, "SessionStopped")) {
                return handle_session_stopped (session_relay, connection, message);
        } else if (dbus_message_is_method_call (message, GDM_SESSION_RELAY_DBUS_INTERFACE, "ConversationStarted")) {
                return handle_conversation_started (session_relay, connection, message);
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

        g_debug ("GdmSessionRelay: Do introspect");

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
                               "  <interface name=\"org.mate.DisplayManager.SessionRelay\">\n"
                               "    <method name=\"ConversationStarted\">\n"
                               "    </method>\n"
                               "    <method name=\"SetupComplete\">\n"
                               "    </method>\n"
                               "    <method name=\"SetupFailed\">\n"
                               "      <arg name=\"message\" direction=\"in\" type=\"s\"/>\n"
                               "    </method>\n"
                               "    <method name=\"ResetComplete\">\n"
                               "    </method>\n"
                               "    <method name=\"RestFailed\">\n"
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
                               "    <method name=\"InfoQuery\">\n"
                               "      <arg name=\"text\" type=\"s\"/>\n"
                               "    </method>\n"
                               "    <method name=\"SecretInfoQuery\">\n"
                               "      <arg name=\"text\" type=\"s\"/>\n"
                               "    </method>\n"
                               "    <method name=\"Info\">\n"
                               "      <arg name=\"text\" type=\"s\"/>\n"
                               "    </method>\n"
                               "    <method name=\"Problem\">\n"
                               "      <arg name=\"text\" type=\"s\"/>\n"
                               "    </method>\n"
                               "    <method name=\"SessionStarted\">\n"
                               "    </method>\n"
                               "    <method name=\"SessionStopped\">\n"
                               "    </method>\n"
                               "    <signal name=\"Reset\">\n"
                               "    </signal>\n"
                               "    <signal name=\"Setup\">\n"
                               "      <arg name=\"service_name\" type=\"s\"/>\n"
                               "    </signal>\n"
                               "    <signal name=\"SetupForUser\">\n"
                               "      <arg name=\"service_name\" type=\"s\"/>\n"
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

                               "    <signal name=\"StartConversation\">\n"
                               "    </signal>\n"
                               "    <signal name=\"Close\">\n"
                               "    </signal>\n"
                               "    <signal name=\"StartSession\">\n"
                               "    </signal>\n"
                               "    <signal name=\"AnswerQuery\">\n"
                               "      <arg name=\"text\" type=\"s\"/>\n"
                               "    </signal>\n"
                               "    <signal name=\"LanguageSelected\">\n"
                               "      <arg name=\"language\" type=\"s\"/>\n"
                               "    </signal>\n"
                               "    <signal name=\"LayoutSelected\">\n"
                               "      <arg name=\"layout\" type=\"s\"/>\n"
                               "    </signal>\n"
                               "    <signal name=\"SessionSelected\">\n"
                               "      <arg name=\"session\" type=\"s\"/>\n"
                               "    </signal>\n"
                               "    <signal name=\"UserSelected\">\n"
                               "      <arg name=\"session\" type=\"s\"/>\n"
                               "    </signal>\n"
                               "    <signal name=\"Cancelled\">\n"
                               "      <arg name=\"session\" type=\"s\"/>\n"
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
session_relay_message_handler (DBusConnection  *connection,
                                DBusMessage     *message,
                                void            *user_data)
{
        const char *dbus_destination = dbus_message_get_destination (message);
        const char *dbus_path        = dbus_message_get_path (message);
        const char *dbus_interface   = dbus_message_get_interface (message);
        const char *dbus_member      = dbus_message_get_member (message);

        g_debug ("GdmSessionRelay: session_relay_message_handler: destination=%s obj_path=%s interface=%s method=%s",
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

                /*dbus_connection_unref (connection);*/

                return DBUS_HANDLER_RESULT_HANDLED;
        } else if (dbus_message_is_method_call (message, "org.freedesktop.DBus.Introspectable", "Introspect")) {
                return do_introspect (connection, message);
        } else {
                return session_handle_child_message (connection, message, user_data);
        }

        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static void
session_relay_unregister_handler (DBusConnection  *connection,
                                   void            *user_data)
{
        g_debug ("session_relay_unregister_handler");
}

static DBusHandlerResult
connection_filter_function (DBusConnection *connection,
                            DBusMessage    *message,
                            void           *user_data)
{
        GdmSessionRelay *session_relay  = GDM_SESSION_RELAY (user_data);
        const char      *dbus_path      = dbus_message_get_path (message);
        const char      *dbus_interface = dbus_message_get_interface (message);
        const char      *dbus_message   = dbus_message_get_member (message);

        g_debug ("GdmSessionRelay: obj_path=%s interface=%s method=%s",
                 dbus_path      ? dbus_path      : "(null)",
                 dbus_interface ? dbus_interface : "(null)",
                 dbus_message   ? dbus_message   : "(null)");

        if (dbus_message_is_signal (message, DBUS_INTERFACE_LOCAL, "Disconnected")
            && strcmp (dbus_path, DBUS_PATH_LOCAL) == 0) {

                g_debug ("GdmSessionRelay: Disconnected");

                dbus_connection_unref (connection);
                session_relay->priv->session_connection = NULL;

                g_signal_emit (session_relay, signals[DISCONNECTED], 0);
        } else if (dbus_message_is_signal (message, DBUS_INTERFACE_DBUS, "NameOwnerChanged")) {


        } else {
                return session_relay_message_handler (connection, message, user_data);
        }

        return DBUS_HANDLER_RESULT_HANDLED;
}

static dbus_bool_t
allow_user_function (DBusConnection *connection,
                     unsigned long   uid,
                     void           *data)
{
        if (uid == 0) {
                return TRUE;
        }

        return FALSE;
}

static void
handle_connection (DBusServer      *server,
                   DBusConnection  *new_connection,
                   void            *user_data)
{
        GdmSessionRelay *session_relay = GDM_SESSION_RELAY (user_data);

        g_debug ("GdmSessionRelay: Handling new connection");

        g_assert (session_relay->priv->session_connection == NULL);

        if (session_relay->priv->session_connection == NULL) {
                DBusObjectPathVTable vtable = { &session_relay_unregister_handler,
                                                &session_relay_message_handler,
                                                NULL, NULL, NULL, NULL
                };

                session_relay->priv->session_connection = new_connection;
                dbus_connection_ref (new_connection);
                dbus_connection_setup_with_g_main (new_connection, NULL);

                g_debug ("GdmSessionRelay: session connection is %p", new_connection);

                dbus_connection_add_filter (new_connection,
                                            connection_filter_function,
                                            session_relay,
                                            NULL);

                dbus_connection_set_unix_user_function (new_connection,
                                                        allow_user_function,
                                                        session_relay,
                                                        NULL);

                dbus_connection_register_object_path (new_connection,
                                                      GDM_SESSION_RELAY_DBUS_PATH,
                                                      &vtable,
                                                      session_relay);

                g_signal_emit (session_relay, signals[CONNECTED], 0);
        }
}

gboolean
gdm_session_relay_start (GdmSessionRelay *session_relay)
{
        DBusError   error;
        gboolean    ret;
        char       *address;
        const char *auth_mechanisms[] = {"EXTERNAL", NULL};

        ret = FALSE;

        g_debug ("GdmSessionRelay: Creating D-Bus relay for session");

        address = generate_address ();

        dbus_error_init (&error);
        session_relay->priv->server = dbus_server_listen (address, &error);
        g_free (address);

        if (session_relay->priv->server == NULL) {
                g_warning ("Cannot create D-BUS relay for the session: %s", error.message);
                /* FIXME: should probably fail if we can't create the socket */
                goto out;
        }

        dbus_server_setup_with_g_main (session_relay->priv->server, NULL);
        dbus_server_set_auth_mechanisms (session_relay->priv->server, auth_mechanisms);
        dbus_server_set_new_connection_function (session_relay->priv->server,
                                                 handle_connection,
                                                 session_relay,
                                                 NULL);
        ret = TRUE;

        g_free (session_relay->priv->server_address);
        session_relay->priv->server_address = dbus_server_get_address (session_relay->priv->server);

        g_debug ("GdmSessionRelay: D-Bus relay listening on %s", session_relay->priv->server_address);

 out:

        return ret;
}

gboolean
gdm_session_relay_stop (GdmSessionRelay *session_relay)
{
        gboolean ret;

        ret = FALSE;

        g_debug ("GdmSessionRelay: Stopping session relay...");

        return ret;
}

char *
gdm_session_relay_get_address (GdmSessionRelay *session_relay)
{
        return g_strdup (session_relay->priv->server_address);
}

static void
gdm_session_relay_set_property (GObject      *object,
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
gdm_session_relay_get_property (GObject    *object,
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
gdm_session_relay_constructor (GType                  type,
                               guint                  n_construct_properties,
                               GObjectConstructParam *construct_properties)
{
        GdmSessionRelay      *session_relay;

        session_relay = GDM_SESSION_RELAY (G_OBJECT_CLASS (gdm_session_relay_parent_class)->constructor (type,
                                                                                                            n_construct_properties,
                                                                                                            construct_properties));

        return G_OBJECT (session_relay);
}

static void
gdm_session_iface_init (GdmSessionIface *iface)
{

        iface->start_conversation = gdm_session_relay_start_conversation;
        iface->setup = gdm_session_relay_setup;
        iface->setup_for_user = gdm_session_relay_setup_for_user;
        iface->authenticate = gdm_session_relay_authenticate;
        iface->authorize = gdm_session_relay_authorize;
        iface->accredit = gdm_session_relay_accredit;
        iface->open_session = gdm_session_relay_open_session;
        iface->close = gdm_session_relay_close;

        iface->cancel = gdm_session_relay_cancel;
        iface->start_session = gdm_session_relay_start_session;
        iface->answer_query = gdm_session_relay_answer_query;
        iface->select_session = gdm_session_relay_select_session;
        iface->select_language = gdm_session_relay_select_language;
        iface->select_layout = gdm_session_relay_select_layout;
        iface->select_user = gdm_session_relay_select_user;
}

static void
gdm_session_relay_class_init (GdmSessionRelayClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = gdm_session_relay_get_property;
        object_class->set_property = gdm_session_relay_set_property;
        object_class->constructor = gdm_session_relay_constructor;
        object_class->finalize = gdm_session_relay_finalize;

        g_type_class_add_private (klass, sizeof (GdmSessionRelayPrivate));

        signals [CONNECTED] =
                g_signal_new ("connected",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (GdmSessionRelayClass, connected),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
        signals [DISCONNECTED] =
                g_signal_new ("disconnected",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (GdmSessionRelayClass, disconnected),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
}

static void
gdm_session_relay_init (GdmSessionRelay *session_relay)
{

        session_relay->priv = GDM_SESSION_RELAY_GET_PRIVATE (session_relay);
}

static void
gdm_session_relay_finalize (GObject *object)
{
        GdmSessionRelay *session_relay;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GDM_IS_SESSION_RELAY (object));

        session_relay = GDM_SESSION_RELAY (object);

        g_return_if_fail (session_relay->priv != NULL);

        gdm_session_relay_stop (session_relay);

        G_OBJECT_CLASS (gdm_session_relay_parent_class)->finalize (object);
}

GdmSessionRelay *
gdm_session_relay_new (void)
{
        GObject *object;

        object = g_object_new (GDM_TYPE_SESSION_RELAY,
                               NULL);

        return GDM_SESSION_RELAY (object);
}
