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

#include "mdm-session-private.h"
#include "mdm-session-relay.h"

#define MDM_SESSION_RELAY_DBUS_PATH      "/org/mate/DisplayManager/SessionRelay"
#define MDM_SESSION_RELAY_DBUS_INTERFACE "org.mate.DisplayManager.SessionRelay"

#define MDM_SESSION_RELAY_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_SESSION_RELAY, MdmSessionRelayPrivate))

struct MdmSessionRelayPrivate
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

static void     mdm_session_relay_class_init    (MdmSessionRelayClass *klass);
static void     mdm_session_relay_init          (MdmSessionRelay      *session_relay);
static void     mdm_session_relay_finalize      (GObject              *object);
static void     mdm_session_iface_init          (MdmSessionIface      *iface);

G_DEFINE_TYPE_WITH_CODE (MdmSessionRelay,
                         mdm_session_relay,
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
                g_debug ("MdmSessionRelay: There is no valid connection");
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
send_dbus_string_signal (MdmSessionRelay *session_relay,
                         const char      *name,
                         const char      *text)
{
        DBusMessage    *message;
        DBusMessageIter iter;

        g_return_if_fail (session_relay != NULL);

        g_debug ("MdmSessionRelay: sending signal %s", name);
        message = dbus_message_new_signal (MDM_SESSION_RELAY_DBUS_PATH,
                                           MDM_SESSION_RELAY_DBUS_INTERFACE,
                                           name);

        dbus_message_iter_init_append (message, &iter);
        dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &text);

        if (! send_dbus_message (session_relay->priv->session_connection, message)) {
                g_debug ("MdmSessionRelay: Could not send %s signal", name);
        }

        dbus_message_unref (message);
}

static void
send_dbus_string_string_signal (MdmSessionRelay *session_relay,
                                const char      *name,
                                const char      *text1,
                                const char      *text2)
{
        DBusMessage    *message;
        DBusMessageIter iter;

        g_return_if_fail (session_relay != NULL);

        g_debug ("MdmSessionRelay: sending signal %s", name);
        message = dbus_message_new_signal (MDM_SESSION_RELAY_DBUS_PATH,
                                           MDM_SESSION_RELAY_DBUS_INTERFACE,
                                           name);

        dbus_message_iter_init_append (message, &iter);
        dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &text1);
        dbus_message_iter_init_append (message, &iter);
        dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &text2);

        if (! send_dbus_message (session_relay->priv->session_connection, message)) {
                g_debug ("MdmSessionRelay: Could not send %s signal", name);
        }

        dbus_message_unref (message);
}

static void
send_dbus_void_signal (MdmSessionRelay *session_relay,
                       const char      *name)
{
        DBusMessage    *message;

        g_return_if_fail (session_relay != NULL);

        g_debug ("MdmSessionRelay: sending signal %s", name);
        message = dbus_message_new_signal (MDM_SESSION_RELAY_DBUS_PATH,
                                           MDM_SESSION_RELAY_DBUS_INTERFACE,
                                           name);

        if (! send_dbus_message (session_relay->priv->session_connection, message)) {
                g_debug ("MdmSessionRelay: Could not send %s signal", name);
        }

        dbus_message_unref (message);
}

static void
mdm_session_relay_start_conversation (MdmSession *session)
{
        MdmSessionRelay *impl = MDM_SESSION_RELAY (session);
        send_dbus_void_signal (impl, "StartConversation");
}

static void
mdm_session_relay_close (MdmSession *session)
{
        MdmSessionRelay *impl = MDM_SESSION_RELAY (session);
        send_dbus_void_signal (impl, "Close");
}

static void
mdm_session_relay_setup (MdmSession *session,
                         const char *service_name)
{
        MdmSessionRelay *impl = MDM_SESSION_RELAY (session);
        send_dbus_string_signal (impl, "Setup", service_name);
}

static void
mdm_session_relay_setup_for_user (MdmSession *session,
                                  const char *service_name,
                                  const char *username)
{
        MdmSessionRelay *impl = MDM_SESSION_RELAY (session);
        send_dbus_string_string_signal (impl, "SetupForUser", service_name, username);
}

static void
mdm_session_relay_authenticate (MdmSession *session)
{
        MdmSessionRelay *impl = MDM_SESSION_RELAY (session);
        send_dbus_void_signal (impl, "Authenticate");
}

static void
mdm_session_relay_authorize (MdmSession *session)
{
        MdmSessionRelay *impl = MDM_SESSION_RELAY (session);
        send_dbus_void_signal (impl, "Authorize");
}

static void
mdm_session_relay_accredit (MdmSession *session,
                            int         cred_flag)
{
        MdmSessionRelay *impl = MDM_SESSION_RELAY (session);

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
mdm_session_relay_open_session (MdmSession *session)
{
        MdmSessionRelay *impl = MDM_SESSION_RELAY (session);
        send_dbus_void_signal (impl, "OpenSession");
}

static void
mdm_session_relay_answer_query (MdmSession *session,
                                const char *text)
{
        MdmSessionRelay *impl = MDM_SESSION_RELAY (session);
        send_dbus_string_signal (impl, "AnswerQuery", text);
}

static void
mdm_session_relay_select_session (MdmSession *session,
                                  const char *text)
{
        MdmSessionRelay *impl = MDM_SESSION_RELAY (session);
        send_dbus_string_signal (impl, "SessionSelected", text);
}

static void
mdm_session_relay_select_language (MdmSession *session,
                                   const char *text)
{
        MdmSessionRelay *impl = MDM_SESSION_RELAY (session);
        send_dbus_string_signal (impl, "LanguageSelected", text);
}

static void
mdm_session_relay_select_layout (MdmSession *session,
                                 const char *text)
{
        MdmSessionRelay *impl = MDM_SESSION_RELAY (session);
        send_dbus_string_signal (impl, "LayoutSelected", text);
}

static void
mdm_session_relay_select_user (MdmSession *session,
                               const char *text)
{
        MdmSessionRelay *impl = MDM_SESSION_RELAY (session);
        send_dbus_string_signal (impl, "UserSelected", text);
}

static void
mdm_session_relay_cancel (MdmSession *session)
{
        MdmSessionRelay *impl = MDM_SESSION_RELAY (session);

        send_dbus_void_signal (impl, "Cancelled");
}

static void
mdm_session_relay_start_session (MdmSession *session)
{
        MdmSessionRelay *impl = MDM_SESSION_RELAY (session);

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

        path = g_strdup_printf ("unix:abstract=/tmp/mdm-session-%s", tmp);
#else
        path = g_strdup ("unix:tmpdir=/tmp");
#endif

        return path;
}

static DBusHandlerResult
handle_info_query (MdmSessionRelay *session_relay,
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

        g_debug ("MdmSessionRelay: InfoQuery: %s", text);

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        _mdm_session_info_query (MDM_SESSION (session_relay), text);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_secret_info_query (MdmSessionRelay *session_relay,
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

        g_debug ("MdmSessionRelay: SecretInfoQuery: %s", text);

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        _mdm_session_secret_info_query (MDM_SESSION (session_relay), text);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_info (MdmSessionRelay *session_relay,
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

        g_debug ("MdmSessionRelay: Info: %s", text);

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        _mdm_session_info (MDM_SESSION (session_relay), text);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_problem (MdmSessionRelay *session_relay,
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

        g_debug ("MdmSessionRelay: Problem: %s", text);

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        _mdm_session_problem (MDM_SESSION (session_relay), text);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_setup_complete (MdmSessionRelay *session_relay,
                       DBusConnection  *connection,
                       DBusMessage     *message)
{
        DBusMessage *reply;
        DBusError    error;

        dbus_error_init (&error);

        g_debug ("MdmSessionRelay: SetupComplete");

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        _mdm_session_setup_complete (MDM_SESSION (session_relay));

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_setup_failed (MdmSessionRelay *session_relay,
                     DBusConnection  *connection,
                     DBusMessage     *message)
{
        DBusMessage *reply;
        DBusError    error;

        dbus_error_init (&error);

        g_debug ("MdmSessionRelay: SetupFailed");

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        _mdm_session_setup_failed (MDM_SESSION (session_relay), NULL);

        return DBUS_HANDLER_RESULT_HANDLED;
}


static DBusHandlerResult
handle_authenticated (MdmSessionRelay *session_relay,
                      DBusConnection  *connection,
                      DBusMessage     *message)
{
        DBusMessage *reply;
        DBusError    error;

        dbus_error_init (&error);

        g_debug ("MdmSessionRelay: Authenticated");

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        _mdm_session_authenticated (MDM_SESSION (session_relay));

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_authentication_failed (MdmSessionRelay *session_relay,
                              DBusConnection  *connection,
                              DBusMessage     *message)
{
        DBusMessage *reply;
        DBusError    error;

        dbus_error_init (&error);

        g_debug ("MdmSessionRelay: AuthenticationFailed");

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        _mdm_session_authentication_failed (MDM_SESSION (session_relay), NULL);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_authorized (MdmSessionRelay *session_relay,
                   DBusConnection  *connection,
                   DBusMessage     *message)
{
        DBusMessage *reply;
        DBusError    error;

        dbus_error_init (&error);

        g_debug ("MdmSessionRelay: Authorized");

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        _mdm_session_authorized (MDM_SESSION (session_relay));

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_authorization_failed (MdmSessionRelay *session_relay,
                             DBusConnection  *connection,
                             DBusMessage     *message)
{
        DBusMessage *reply;
        DBusError    error;

        dbus_error_init (&error);

        g_debug ("MdmSessionRelay: AuthorizationFailed");

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        _mdm_session_authorization_failed (MDM_SESSION (session_relay), NULL);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_accredited (MdmSessionRelay *session_relay,
                   DBusConnection  *connection,
                   DBusMessage     *message)
{
        DBusMessage *reply;
        DBusError    error;

        dbus_error_init (&error);

        g_debug ("MdmSessionRelay: Accredited");

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        _mdm_session_accredited (MDM_SESSION (session_relay));

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_accreditation_failed (MdmSessionRelay *session_relay,
                             DBusConnection  *connection,
                             DBusMessage     *message)
{
        DBusMessage *reply;
        DBusError    error;

        dbus_error_init (&error);

        g_debug ("MdmSessionRelay: AccreditationFailed");

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        _mdm_session_accreditation_failed (MDM_SESSION (session_relay), NULL);

        return DBUS_HANDLER_RESULT_HANDLED;
}
static DBusHandlerResult
handle_session_opened (MdmSessionRelay *session_relay,
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

        g_debug ("MdmSessionRelay: Session Opened");

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        _mdm_session_session_opened (MDM_SESSION (session_relay));

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_session_open_failed (MdmSessionRelay *session_relay,
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

        g_debug ("MdmSessionRelay: Session Open Failed");

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        _mdm_session_session_open_failed (MDM_SESSION (session_relay), NULL);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_session_started (MdmSessionRelay *session_relay,
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

        g_debug ("MdmSessionRelay: SessionStarted");

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        _mdm_session_session_started (MDM_SESSION (session_relay),
                                      pid);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_session_stopped (MdmSessionRelay *session_relay,
                        DBusConnection  *connection,
                        DBusMessage     *message)
{
        DBusMessage *reply;
        DBusError    error;

        dbus_error_init (&error);

        g_debug ("MdmSessionRelay: SessionStopped");

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

#if 0
        _mdm_session_session_stopped (MDM_SESSION (session_relay));
#endif

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_conversation_started (MdmSessionRelay *session_relay,
                             DBusConnection  *connection,
                             DBusMessage     *message)
{
        DBusMessage *reply;
        DBusError    error;

        dbus_error_init (&error);

        g_debug ("MdmSessionRelay: Conversation Started");

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        _mdm_session_conversation_started (MDM_SESSION (session_relay));

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
session_handle_child_message (DBusConnection *connection,
                              DBusMessage    *message,
                              void           *user_data)
{
        MdmSessionRelay *session_relay = MDM_SESSION_RELAY (user_data);

        if (dbus_message_is_method_call (message, MDM_SESSION_RELAY_DBUS_INTERFACE, "InfoQuery")) {
                return handle_info_query (session_relay, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_RELAY_DBUS_INTERFACE, "SecretInfoQuery")) {
                return handle_secret_info_query (session_relay, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_RELAY_DBUS_INTERFACE, "Info")) {
                return handle_info (session_relay, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_RELAY_DBUS_INTERFACE, "Problem")) {
                return handle_problem (session_relay, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_RELAY_DBUS_INTERFACE, "SetupComplete")) {
                return handle_setup_complete (session_relay, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_RELAY_DBUS_INTERFACE, "SetupFailed")) {
                return handle_setup_failed (session_relay, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_RELAY_DBUS_INTERFACE, "Authenticated")) {
                return handle_authenticated (session_relay, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_RELAY_DBUS_INTERFACE, "AuthenticationFailed")) {
                return handle_authentication_failed (session_relay, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_RELAY_DBUS_INTERFACE, "Authorized")) {
                return handle_authorized (session_relay, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_RELAY_DBUS_INTERFACE, "AuthorizationFailed")) {
                return handle_authorization_failed (session_relay, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_RELAY_DBUS_INTERFACE, "Accredited")) {
                return handle_accredited (session_relay, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_RELAY_DBUS_INTERFACE, "AccreditationFailed")) {
                return handle_accreditation_failed (session_relay, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_RELAY_DBUS_INTERFACE, "SessionOpened")) {
                return handle_session_opened (session_relay, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_RELAY_DBUS_INTERFACE, "SessionOpenFailed")) {
                return handle_session_open_failed (session_relay, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_RELAY_DBUS_INTERFACE, "SessionStarted")) {
                return handle_session_started (session_relay, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_RELAY_DBUS_INTERFACE, "SessionStopped")) {
                return handle_session_stopped (session_relay, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_SESSION_RELAY_DBUS_INTERFACE, "ConversationStarted")) {
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

        g_debug ("MdmSessionRelay: Do introspect");

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

        g_debug ("MdmSessionRelay: session_relay_message_handler: destination=%s obj_path=%s interface=%s method=%s",
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
        MdmSessionRelay *session_relay  = MDM_SESSION_RELAY (user_data);
        const char      *dbus_path      = dbus_message_get_path (message);
        const char      *dbus_interface = dbus_message_get_interface (message);
        const char      *dbus_message   = dbus_message_get_member (message);

        g_debug ("MdmSessionRelay: obj_path=%s interface=%s method=%s",
                 dbus_path      ? dbus_path      : "(null)",
                 dbus_interface ? dbus_interface : "(null)",
                 dbus_message   ? dbus_message   : "(null)");

        if (dbus_message_is_signal (message, DBUS_INTERFACE_LOCAL, "Disconnected")
            && strcmp (dbus_path, DBUS_PATH_LOCAL) == 0) {

                g_debug ("MdmSessionRelay: Disconnected");

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
        MdmSessionRelay *session_relay = MDM_SESSION_RELAY (user_data);

        g_debug ("MdmSessionRelay: Handling new connection");

        g_assert (session_relay->priv->session_connection == NULL);

        if (session_relay->priv->session_connection == NULL) {
                DBusObjectPathVTable vtable = { &session_relay_unregister_handler,
                                                &session_relay_message_handler,
                                                NULL, NULL, NULL, NULL
                };

                session_relay->priv->session_connection = new_connection;
                dbus_connection_ref (new_connection);
                dbus_connection_setup_with_g_main (new_connection, NULL);

                g_debug ("MdmSessionRelay: session connection is %p", new_connection);

                dbus_connection_add_filter (new_connection,
                                            connection_filter_function,
                                            session_relay,
                                            NULL);

                dbus_connection_set_unix_user_function (new_connection,
                                                        allow_user_function,
                                                        session_relay,
                                                        NULL);

                dbus_connection_register_object_path (new_connection,
                                                      MDM_SESSION_RELAY_DBUS_PATH,
                                                      &vtable,
                                                      session_relay);

                g_signal_emit (session_relay, signals[CONNECTED], 0);
        }
}

gboolean
mdm_session_relay_start (MdmSessionRelay *session_relay)
{
        DBusError   error;
        gboolean    ret;
        char       *address;
        const char *auth_mechanisms[] = {"EXTERNAL", NULL};

        ret = FALSE;

        g_debug ("MdmSessionRelay: Creating D-Bus relay for session");

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

        g_debug ("MdmSessionRelay: D-Bus relay listening on %s", session_relay->priv->server_address);

 out:

        return ret;
}

gboolean
mdm_session_relay_stop (MdmSessionRelay *session_relay)
{
        gboolean ret;

        ret = FALSE;

        g_debug ("MdmSessionRelay: Stopping session relay...");

        return ret;
}

char *
mdm_session_relay_get_address (MdmSessionRelay *session_relay)
{
        return g_strdup (session_relay->priv->server_address);
}

static void
mdm_session_relay_set_property (GObject      *object,
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
mdm_session_relay_get_property (GObject    *object,
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
mdm_session_relay_constructor (GType                  type,
                               guint                  n_construct_properties,
                               GObjectConstructParam *construct_properties)
{
        MdmSessionRelay      *session_relay;

        session_relay = MDM_SESSION_RELAY (G_OBJECT_CLASS (mdm_session_relay_parent_class)->constructor (type,
                                                                                                            n_construct_properties,
                                                                                                            construct_properties));

        return G_OBJECT (session_relay);
}

static void
mdm_session_iface_init (MdmSessionIface *iface)
{

        iface->start_conversation = mdm_session_relay_start_conversation;
        iface->setup = mdm_session_relay_setup;
        iface->setup_for_user = mdm_session_relay_setup_for_user;
        iface->authenticate = mdm_session_relay_authenticate;
        iface->authorize = mdm_session_relay_authorize;
        iface->accredit = mdm_session_relay_accredit;
        iface->open_session = mdm_session_relay_open_session;
        iface->close = mdm_session_relay_close;

        iface->cancel = mdm_session_relay_cancel;
        iface->start_session = mdm_session_relay_start_session;
        iface->answer_query = mdm_session_relay_answer_query;
        iface->select_session = mdm_session_relay_select_session;
        iface->select_language = mdm_session_relay_select_language;
        iface->select_layout = mdm_session_relay_select_layout;
        iface->select_user = mdm_session_relay_select_user;
}

static void
mdm_session_relay_class_init (MdmSessionRelayClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = mdm_session_relay_get_property;
        object_class->set_property = mdm_session_relay_set_property;
        object_class->constructor = mdm_session_relay_constructor;
        object_class->finalize = mdm_session_relay_finalize;

        g_type_class_add_private (klass, sizeof (MdmSessionRelayPrivate));

        signals [CONNECTED] =
                g_signal_new ("connected",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmSessionRelayClass, connected),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
        signals [DISCONNECTED] =
                g_signal_new ("disconnected",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmSessionRelayClass, disconnected),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
}

static void
mdm_session_relay_init (MdmSessionRelay *session_relay)
{

        session_relay->priv = MDM_SESSION_RELAY_GET_PRIVATE (session_relay);
}

static void
mdm_session_relay_finalize (GObject *object)
{
        MdmSessionRelay *session_relay;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_SESSION_RELAY (object));

        session_relay = MDM_SESSION_RELAY (object);

        g_return_if_fail (session_relay->priv != NULL);

        mdm_session_relay_stop (session_relay);

        G_OBJECT_CLASS (mdm_session_relay_parent_class)->finalize (object);
}

MdmSessionRelay *
mdm_session_relay_new (void)
{
        GObject *object;

        object = g_object_new (MDM_TYPE_SESSION_RELAY,
                               NULL);

        return MDM_SESSION_RELAY (object);
}
