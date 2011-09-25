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

#include "mdm-common.h"
#include "mdm-greeter-server.h"

#define MDM_GREETER_SERVER_DBUS_PATH      "/org/mate/DisplayManager/GreeterServer"
#define MDM_GREETER_SERVER_DBUS_INTERFACE "org.mate.DisplayManager.GreeterServer"

#define MDM_GREETER_SERVER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_GREETER_SERVER, MdmGreeterServerPrivate))

struct MdmGreeterServerPrivate
{
        char           *user_name;
        char           *group_name;
        char           *display_id;

        DBusServer     *server;
        char           *server_address;
        DBusConnection *greeter_connection;
};

enum {
        PROP_0,
        PROP_USER_NAME,
        PROP_GROUP_NAME,
        PROP_DISPLAY_ID,
};

enum {
        BEGIN_AUTO_LOGIN,
        BEGIN_VERIFICATION,
        BEGIN_VERIFICATION_FOR_USER,
        QUERY_ANSWER,
        SESSION_SELECTED,
        HOSTNAME_SELECTED,
        LANGUAGE_SELECTED,
        LAYOUT_SELECTED,
        USER_SELECTED,
        CANCELLED,
        CONNECTED,
        DISCONNECTED,
        START_SESSION_WHEN_READY,
        START_SESSION_LATER,
        LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0, };

static void     mdm_greeter_server_class_init   (MdmGreeterServerClass *klass);
static void     mdm_greeter_server_init         (MdmGreeterServer      *greeter_server);
static void     mdm_greeter_server_finalize     (GObject               *object);

G_DEFINE_TYPE (MdmGreeterServer, mdm_greeter_server, G_TYPE_OBJECT)

static gboolean
send_dbus_message (DBusConnection *connection,
                   DBusMessage    *message)
{
        gboolean is_connected;
        gboolean sent;

        g_return_val_if_fail (message != NULL, FALSE);

        if (connection == NULL) {
                g_debug ("GreeterServer: There is no valid connection");
                return FALSE;
        }

        is_connected = dbus_connection_get_is_connected (connection);
        if (! is_connected) {
                g_warning ("GreeterServer: Not connected!");
                return FALSE;
        }

        sent = dbus_connection_send (connection, message, NULL);
        dbus_connection_flush (connection);

        return sent;
}

static void
send_dbus_string_and_int_signal (MdmGreeterServer *greeter_server,
                                 const char       *name,
                                 const char       *text,
                                 int               number)
{
        DBusMessage    *message;
        DBusMessageIter iter;
        const char     *str;

        if (text != NULL) {
                str = text;
        } else {
                str = "";
        }

        g_return_if_fail (greeter_server != NULL);

        message = dbus_message_new_signal (MDM_GREETER_SERVER_DBUS_PATH,
                                           MDM_GREETER_SERVER_DBUS_INTERFACE,
                                           name);

        dbus_message_iter_init_append (message, &iter);
        dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &str);
        dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32, &number);

        g_debug ("GreeterServer: Sending %s (%s %d)", name, str, number);
        if (! send_dbus_message (greeter_server->priv->greeter_connection, message)) {
                g_debug ("GreeterServer: Could not send %s signal", name);
        }

        dbus_message_unref (message);
}

static void
send_dbus_string_signal (MdmGreeterServer *greeter_server,
                         const char       *name,
                         const char       *text)
{
        DBusMessage    *message;
        DBusMessageIter iter;
        const char     *str;

        if (text != NULL) {
                str = text;
        } else {
                str = "";
        }

        g_return_if_fail (greeter_server != NULL);

        message = dbus_message_new_signal (MDM_GREETER_SERVER_DBUS_PATH,
                                           MDM_GREETER_SERVER_DBUS_INTERFACE,
                                           name);

        dbus_message_iter_init_append (message, &iter);
        dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &str);

        g_debug ("GreeterServer: Sending %s (%s)", name, str);
        if (! send_dbus_message (greeter_server->priv->greeter_connection, message)) {
                g_debug ("GreeterServer: Could not send %s signal", name);
        }

        dbus_message_unref (message);
}

static void
send_dbus_void_signal (MdmGreeterServer *greeter_server,
                       const char       *name)
{
        DBusMessage    *message;

        g_return_if_fail (greeter_server != NULL);

        message = dbus_message_new_signal (MDM_GREETER_SERVER_DBUS_PATH,
                                           MDM_GREETER_SERVER_DBUS_INTERFACE,
                                           name);

        if (! send_dbus_message (greeter_server->priv->greeter_connection, message)) {
                g_debug ("GreeterServer: Could not send %s signal", name);
        }

        dbus_message_unref (message);
}

gboolean
mdm_greeter_server_info_query (MdmGreeterServer *greeter_server,
                               const char       *text)
{
        send_dbus_string_signal (greeter_server, "InfoQuery", text);

        return TRUE;
}

gboolean
mdm_greeter_server_secret_info_query (MdmGreeterServer *greeter_server,
                                      const char       *text)
{
        send_dbus_string_signal (greeter_server, "SecretInfoQuery", text);
        return TRUE;
}

gboolean
mdm_greeter_server_info (MdmGreeterServer *greeter_server,
                         const char       *text)
{
        send_dbus_string_signal (greeter_server, "Info", text);
        return TRUE;
}

gboolean
mdm_greeter_server_problem (MdmGreeterServer *greeter_server,
                            const char       *text)
{
        send_dbus_string_signal (greeter_server, "Problem", text);
        return TRUE;
}

gboolean
mdm_greeter_server_authentication_failed (MdmGreeterServer *greeter_server)
{
        send_dbus_void_signal (greeter_server, "AuthenticationFailed");
        return TRUE;
}

gboolean
mdm_greeter_server_reset (MdmGreeterServer *greeter_server)
{
        send_dbus_void_signal (greeter_server, "Reset");
        return TRUE;
}

gboolean
mdm_greeter_server_ready (MdmGreeterServer *greeter_server)
{
        send_dbus_void_signal (greeter_server, "Ready");
        return TRUE;
}

void
mdm_greeter_server_selected_user_changed (MdmGreeterServer *greeter_server,
                                          const char       *username)
{
        send_dbus_string_signal (greeter_server, "SelectedUserChanged", username);
}

void
mdm_greeter_server_default_language_name_changed (MdmGreeterServer *greeter_server,
                                                  const char       *language_name)
{
        send_dbus_string_signal (greeter_server, "DefaultLanguageNameChanged", language_name);
}

void
mdm_greeter_server_default_layout_name_changed (MdmGreeterServer *greeter_server,
                                                const char       *layout_name)
{
        send_dbus_string_signal (greeter_server, "DefaultLayoutNameChanged", layout_name);
}

void
mdm_greeter_server_default_session_name_changed (MdmGreeterServer *greeter_server,
                                                 const char       *session_name)
{
        send_dbus_string_signal (greeter_server, "DefaultSessionNameChanged", session_name);
}

void
mdm_greeter_server_request_timed_login (MdmGreeterServer *greeter_server,
                                        const char       *username,
                                        int               delay)
{
        send_dbus_string_and_int_signal (greeter_server, "TimedLoginRequested", username, delay);
}

void
mdm_greeter_server_user_authorized (MdmGreeterServer *greeter_server)
{
        send_dbus_void_signal (greeter_server, "UserAuthorized");
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

        path = g_strdup_printf ("unix:abstract=/tmp/mdm-greeter-%s", tmp);
#else
        path = g_strdup ("unix:tmpdir=/tmp");
#endif

        return path;
}

static DBusHandlerResult
handle_begin_verification (MdmGreeterServer *greeter_server,
                           DBusConnection   *connection,
                           DBusMessage      *message)
{
        DBusMessage *reply;

        g_debug ("GreeterServer: BeginVerification");

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        g_signal_emit (greeter_server, signals [BEGIN_VERIFICATION], 0);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_begin_auto_login (MdmGreeterServer *greeter_server,
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

        g_debug ("GreeterServer: BeginAutoLogin for '%s'", text);

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        g_signal_emit (greeter_server, signals [BEGIN_AUTO_LOGIN], 0, text);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_begin_verification_for_user (MdmGreeterServer *greeter_server,
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

        g_debug ("GreeterServer: BeginVerificationForUser for '%s'", text);

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        g_signal_emit (greeter_server, signals [BEGIN_VERIFICATION_FOR_USER], 0, text);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_answer_query (MdmGreeterServer *greeter_server,
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

        g_debug ("GreeterServer: AnswerQuery");

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        g_signal_emit (greeter_server, signals [QUERY_ANSWER], 0, text);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_select_session (MdmGreeterServer *greeter_server,
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

        g_debug ("GreeterServer: SelectSession: %s", text);

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        g_signal_emit (greeter_server, signals [SESSION_SELECTED], 0, text);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_select_hostname (MdmGreeterServer *greeter_server,
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

        g_debug ("GreeterServer: SelectHostname: %s", text);

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        g_signal_emit (greeter_server, signals [HOSTNAME_SELECTED], 0, text);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_select_language (MdmGreeterServer *greeter_server,
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

        g_debug ("GreeterServer: SelectLanguage: %s", text);

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        g_signal_emit (greeter_server, signals [LANGUAGE_SELECTED], 0, text);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_select_layout (MdmGreeterServer *greeter_server,
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

        g_debug ("GreeterServer: SelectLayout: %s", text);

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        g_signal_emit (greeter_server, signals [LAYOUT_SELECTED], 0, text);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_select_user (MdmGreeterServer *greeter_server,
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

        g_debug ("GreeterServer: SelectUser: %s", text);

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        g_signal_emit (greeter_server, signals [USER_SELECTED], 0, text);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_cancel (MdmGreeterServer *greeter_server,
               DBusConnection   *connection,
               DBusMessage      *message)
{
        DBusMessage *reply;

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        g_signal_emit (greeter_server, signals [CANCELLED], 0);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_disconnect (MdmGreeterServer *greeter_server,
                   DBusConnection   *connection,
                   DBusMessage      *message)
{
        DBusMessage *reply;

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        g_signal_emit (greeter_server, signals [DISCONNECTED], 0);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_get_display_id (MdmGreeterServer *greeter_server,
                       DBusConnection   *connection,
                       DBusMessage      *message)
{
        DBusMessage    *reply;
        DBusMessageIter iter;

        reply = dbus_message_new_method_return (message);
        dbus_message_iter_init_append (reply, &iter);
        dbus_message_iter_append_basic (&iter, DBUS_TYPE_OBJECT_PATH, &greeter_server->priv->display_id);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_start_session_when_ready (MdmGreeterServer *greeter_server,
                                 DBusConnection   *connection,
                                 DBusMessage      *message)
{
        DBusMessage *reply;
        DBusError    error;
        gboolean     should_start_session;

        dbus_error_init (&error);
        if (! dbus_message_get_args (message, &error,
                                     DBUS_TYPE_BOOLEAN, &should_start_session,
                                     DBUS_TYPE_INVALID)) {
                g_warning ("ERROR: %s", error.message);
        }

        g_debug ("GreeterServer: %sStartSessionWhenReady",
                 should_start_session? "" : "Don't ");

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        dbus_message_unref (reply);

        if (should_start_session) {
                g_signal_emit (greeter_server, signals [START_SESSION_WHEN_READY], 0);
        } else {
                g_signal_emit (greeter_server, signals [START_SESSION_LATER] ,0);
        }

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
greeter_handle_child_message (DBusConnection *connection,
                              DBusMessage    *message,
                              void           *user_data)
{
        MdmGreeterServer *greeter_server = MDM_GREETER_SERVER (user_data);

        if (dbus_message_is_method_call (message, MDM_GREETER_SERVER_DBUS_INTERFACE, "BeginVerification")) {
                return handle_begin_verification (greeter_server, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_GREETER_SERVER_DBUS_INTERFACE, "BeginVerificationForUser")) {
                return handle_begin_verification_for_user (greeter_server, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_GREETER_SERVER_DBUS_INTERFACE, "BeginAutoLogin")) {
                return handle_begin_auto_login (greeter_server, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_GREETER_SERVER_DBUS_INTERFACE, "AnswerQuery")) {
                return handle_answer_query (greeter_server, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_GREETER_SERVER_DBUS_INTERFACE, "SelectSession")) {
                return handle_select_session (greeter_server, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_GREETER_SERVER_DBUS_INTERFACE, "HostnameSession")) {
                return handle_select_hostname (greeter_server, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_GREETER_SERVER_DBUS_INTERFACE, "SelectLanguage")) {
                return handle_select_language (greeter_server, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_GREETER_SERVER_DBUS_INTERFACE, "SelectLayout")) {
                return handle_select_layout (greeter_server, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_GREETER_SERVER_DBUS_INTERFACE, "SelectUser")) {
                return handle_select_user (greeter_server, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_GREETER_SERVER_DBUS_INTERFACE, "Cancel")) {
                return handle_cancel (greeter_server, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_GREETER_SERVER_DBUS_INTERFACE, "Disconnect")) {
                return handle_disconnect (greeter_server, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_GREETER_SERVER_DBUS_INTERFACE, "GetDisplayId")) {
                return handle_get_display_id (greeter_server, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_GREETER_SERVER_DBUS_INTERFACE, "StartSessionWhenReady")) {
                return handle_start_session_when_ready (greeter_server, connection, message);
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

        g_debug ("GreeterServer: Do introspect");

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
                               "  <interface name=\"org.mate.DisplayManager.GreeterServer\">\n"
                               "    <method name=\"BeginVerification\">\n"
                               "    <method name=\"BeginTimedLogin\">\n"
                               "    </method>\n"
                               "    <method name=\"BeginVerificationForUser\">\n"
                               "      <arg name=\"username\" direction=\"in\" type=\"s\"/>\n"
                               "    </method>\n"
                               "    <method name=\"AnswerQuery\">\n"
                               "      <arg name=\"text\" direction=\"in\" type=\"s\"/>\n"
                               "    </method>\n"
                               "    <method name=\"SelectSession\">\n"
                               "      <arg name=\"text\" direction=\"in\" type=\"s\"/>\n"
                               "    </method>\n"
                               "    <method name=\"SelectLanguage\">\n"
                               "      <arg name=\"text\" direction=\"in\" type=\"s\"/>\n"
                               "    </method>\n"
                               "    <method name=\"SelectUser\">\n"
                               "      <arg name=\"text\" direction=\"in\" type=\"s\"/>\n"
                               "    </method>\n"
                               "    <method name=\"SelectHostname\">\n"
                               "      <arg name=\"text\" direction=\"in\" type=\"s\"/>\n"
                               "    </method>\n"
                               "    <method name=\"Cancel\">\n"
                               "    </method>\n"
                               "    <method name=\"Disconnect\">\n"
                               "    </method>\n"
                               "    <method name=\"GetDisplayId\">\n"
                               "      <arg name=\"id\" direction=\"out\" type=\"o\"/>\n"
                               "    </method>\n"
                               "    <method name=\"StartSessionWhenReady\">\n"
                               "      <arg name=\"should_start_session\" type=\"b\"/>\n"
                               "    </method>\n"
                               "    <signal name=\"Info\">\n"
                               "      <arg name=\"text\" type=\"s\"/>\n"
                               "    </signal>\n"
                               "    <signal name=\"Problem\">\n"
                               "      <arg name=\"text\" type=\"s\"/>\n"
                               "    </signal>\n"
                               "    <signal name=\"InfoQuery\">\n"
                               "      <arg name=\"text\" type=\"s\"/>\n"
                               "    </signal>\n"
                               "    <signal name=\"SecretInfoQuery\">\n"
                               "      <arg name=\"text\" type=\"s\"/>\n"
                               "    </signal>\n"
                               "    <signal name=\"SelectedUserChanged\">\n"
                               "      <arg name=\"username\" type=\"s\"/>\n"
                               "    </signal>\n"
                               "    <signal name=\"DefaultLanguageNameChanged\">\n"
                               "      <arg name=\"language_name\" type=\"s\"/>\n"
                               "    </signal>\n"
                               "    <signal name=\"DefaultLayoutNameChanged\">\n"
                               "      <arg name=\"layout_name\" type=\"s\"/>\n"
                               "    </signal>\n"
                               "    <signal name=\"DefaultSessionNameChanged\">\n"
                               "      <arg name=\"session_name\" type=\"s\"/>\n"
                               "    </signal>\n"
                               "    <signal name=\"TimedLoginRequested\">\n"
                               "      <arg name=\"username\" type=\"s\"/>\n"
                               "      <arg name=\"delay\" type=\"i\"/>\n"
                               "    </signal>\n"
                               "    <signal name=\"Ready\">\n"
                               "    </signal>\n"
                               "    <signal name=\"Reset\">\n"
                               "    </signal>\n"
                               "    <signal name=\"AuthenticationFailed\">\n"
                               "    </signal>\n"
                               "    <signal name=\"UserAuthorized\">\n"
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
greeter_server_message_handler (DBusConnection  *connection,
                                DBusMessage     *message,
                                void            *user_data)
{
        const char *dbus_destination = dbus_message_get_destination (message);
        const char *dbus_path        = dbus_message_get_path (message);
        const char *dbus_interface   = dbus_message_get_interface (message);
        const char *dbus_member      = dbus_message_get_member (message);

        g_debug ("greeter_server_message_handler: destination=%s obj_path=%s interface=%s method=%s",
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
                return greeter_handle_child_message (connection, message, user_data);
        }

        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static void
greeter_server_unregister_handler (DBusConnection  *connection,
                                   void            *user_data)
{
        g_debug ("greeter_server_unregister_handler");
}

static DBusHandlerResult
connection_filter_function (DBusConnection *connection,
                            DBusMessage    *message,
                            void           *user_data)
{
        MdmGreeterServer *greeter_server = MDM_GREETER_SERVER (user_data);
        const char       *dbus_path      = dbus_message_get_path (message);
        const char       *dbus_interface = dbus_message_get_interface (message);
        const char       *dbus_message   = dbus_message_get_member (message);

        g_debug ("GreeterServer: obj_path=%s interface=%s method=%s",
                 dbus_path      ? dbus_path      : "(null)",
                 dbus_interface ? dbus_interface : "(null)",
                 dbus_message   ? dbus_message   : "(null)");

        if (dbus_message_is_signal (message, DBUS_INTERFACE_LOCAL, "Disconnected")
            && strcmp (dbus_path, DBUS_PATH_LOCAL) == 0) {

                g_debug ("GreeterServer: Disconnected");

                dbus_connection_unref (connection);
                greeter_server->priv->greeter_connection = NULL;

                return DBUS_HANDLER_RESULT_HANDLED;
        }

        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static dbus_bool_t
allow_user_function (DBusConnection *connection,
                     unsigned long   uid,
                     void           *data)
{
        MdmGreeterServer *greeter_server = MDM_GREETER_SERVER (data);
        struct passwd    *pwent;

        if (greeter_server->priv->user_name == NULL) {
                return FALSE;
        }

        mdm_get_pwent_for_name (greeter_server->priv->user_name, &pwent);
        if (pwent == NULL) {
                return FALSE;
        }

        if (pwent->pw_uid == uid) {
                return TRUE;
        }

        return FALSE;
}

static void
handle_connection (DBusServer      *server,
                   DBusConnection  *new_connection,
                   void            *user_data)
{
        MdmGreeterServer *greeter_server = MDM_GREETER_SERVER (user_data);

        g_debug ("GreeterServer: Handing new connection");

        if (greeter_server->priv->greeter_connection == NULL) {
                DBusObjectPathVTable vtable = { &greeter_server_unregister_handler,
                                                &greeter_server_message_handler,
                                                NULL, NULL, NULL, NULL
                };

                greeter_server->priv->greeter_connection = new_connection;
                dbus_connection_ref (new_connection);
                dbus_connection_setup_with_g_main (new_connection, NULL);

                g_debug ("GreeterServer: greeter connection is %p", new_connection);

                dbus_connection_add_filter (new_connection,
                                            connection_filter_function,
                                            greeter_server,
                                            NULL);

                dbus_connection_set_unix_user_function (new_connection,
                                                        allow_user_function,
                                                        greeter_server,
                                                        NULL);

                dbus_connection_register_object_path (new_connection,
                                                      MDM_GREETER_SERVER_DBUS_PATH,
                                                      &vtable,
                                                      greeter_server);

                g_signal_emit (greeter_server, signals[CONNECTED], 0);

        }
}

gboolean
mdm_greeter_server_start (MdmGreeterServer *greeter_server)
{
        DBusError   error;
        gboolean    ret;
        char       *address;
        const char *auth_mechanisms[] = {"EXTERNAL", NULL};

        ret = FALSE;

        g_debug ("GreeterServer: Creating D-Bus server for greeter");

        address = generate_address ();

        dbus_error_init (&error);
        greeter_server->priv->server = dbus_server_listen (address, &error);
        g_free (address);

        if (greeter_server->priv->server == NULL) {
                g_warning ("Cannot create D-BUS server for the greeter: %s", error.message);
                /* FIXME: should probably fail if we can't create the socket */
                goto out;
        }

        dbus_server_setup_with_g_main (greeter_server->priv->server, NULL);
        dbus_server_set_auth_mechanisms (greeter_server->priv->server, auth_mechanisms);
        dbus_server_set_new_connection_function (greeter_server->priv->server,
                                                 handle_connection,
                                                 greeter_server,
                                                 NULL);
        ret = TRUE;

        g_free (greeter_server->priv->server_address);
        greeter_server->priv->server_address = dbus_server_get_address (greeter_server->priv->server);

        g_debug ("GreeterServer: D-Bus server listening on %s", greeter_server->priv->server_address);

 out:

        return ret;
}

gboolean
mdm_greeter_server_stop (MdmGreeterServer *greeter_server)
{
        gboolean ret;

        ret = FALSE;

        g_debug ("GreeterServer: Stopping greeter server...");

        return ret;
}

char *
mdm_greeter_server_get_address (MdmGreeterServer *greeter_server)
{
        return g_strdup (greeter_server->priv->server_address);
}

static void
_mdm_greeter_server_set_display_id (MdmGreeterServer *greeter_server,
                                    const char       *display_id)
{
        g_free (greeter_server->priv->display_id);
        greeter_server->priv->display_id = g_strdup (display_id);
}

static void
_mdm_greeter_server_set_user_name (MdmGreeterServer *greeter_server,
                                  const char *name)
{
        g_free (greeter_server->priv->user_name);
        greeter_server->priv->user_name = g_strdup (name);
}

static void
_mdm_greeter_server_set_group_name (MdmGreeterServer *greeter_server,
                                    const char *name)
{
        g_free (greeter_server->priv->group_name);
        greeter_server->priv->group_name = g_strdup (name);
}

static void
mdm_greeter_server_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
        MdmGreeterServer *self;

        self = MDM_GREETER_SERVER (object);

        switch (prop_id) {
        case PROP_DISPLAY_ID:
                _mdm_greeter_server_set_display_id (self, g_value_get_string (value));
                break;
        case PROP_USER_NAME:
                _mdm_greeter_server_set_user_name (self, g_value_get_string (value));
                break;
        case PROP_GROUP_NAME:
                _mdm_greeter_server_set_group_name (self, g_value_get_string (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
mdm_greeter_server_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
        MdmGreeterServer *self;

        self = MDM_GREETER_SERVER (object);

        switch (prop_id) {
        case PROP_DISPLAY_ID:
                g_value_set_string (value, self->priv->display_id);
                break;
        case PROP_USER_NAME:
                g_value_set_string (value, self->priv->user_name);
                break;
        case PROP_GROUP_NAME:
                g_value_set_string (value, self->priv->group_name);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static GObject *
mdm_greeter_server_constructor (GType                  type,
                               guint                  n_construct_properties,
                               GObjectConstructParam *construct_properties)
{
        MdmGreeterServer      *greeter_server;

        greeter_server = MDM_GREETER_SERVER (G_OBJECT_CLASS (mdm_greeter_server_parent_class)->constructor (type,
                                                                                       n_construct_properties,
                                                                                       construct_properties));

        return G_OBJECT (greeter_server);
}

static void
mdm_greeter_server_class_init (MdmGreeterServerClass *klass)
{
        GObjectClass    *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = mdm_greeter_server_get_property;
        object_class->set_property = mdm_greeter_server_set_property;
        object_class->constructor = mdm_greeter_server_constructor;
        object_class->finalize = mdm_greeter_server_finalize;

        g_type_class_add_private (klass, sizeof (MdmGreeterServerPrivate));

        g_object_class_install_property (object_class,
                                         PROP_DISPLAY_ID,
                                         g_param_spec_string ("display-id",
                                                              "display id",
                                                              "display id",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
         g_object_class_install_property (object_class,
                                         PROP_USER_NAME,
                                         g_param_spec_string ("user-name",
                                                              "user name",
                                                              "user name",
                                                              MDM_USERNAME,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
        g_object_class_install_property (object_class,
                                         PROP_GROUP_NAME,
                                         g_param_spec_string ("group-name",
                                                              "group name",
                                                              "group name",
                                                              MDM_GROUPNAME,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
        signals [BEGIN_VERIFICATION] =
                g_signal_new ("begin-verification",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmGreeterServerClass, begin_verification),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
        signals [BEGIN_AUTO_LOGIN] =
                g_signal_new ("begin-auto-login",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmGreeterServerClass, begin_auto_login),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_STRING);
        signals [BEGIN_VERIFICATION_FOR_USER] =
                g_signal_new ("begin-verification-for-user",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmGreeterServerClass, begin_verification_for_user),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_STRING);
        signals [QUERY_ANSWER] =
                g_signal_new ("query-answer",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmGreeterServerClass, query_answer),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_STRING);
        signals [SESSION_SELECTED] =
                g_signal_new ("session-selected",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmGreeterServerClass, session_selected),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_STRING);
        signals [HOSTNAME_SELECTED] =
                g_signal_new ("hostname-selected",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmGreeterServerClass, hostname_selected),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_STRING);
        signals [LANGUAGE_SELECTED] =
                g_signal_new ("language-selected",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmGreeterServerClass, language_selected),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_STRING);
        signals [LAYOUT_SELECTED] =
                g_signal_new ("layout-selected",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmGreeterServerClass, layout_selected),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_STRING);
        signals [USER_SELECTED] =
                g_signal_new ("user-selected",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmGreeterServerClass, user_selected),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_STRING);
        signals [CANCELLED] =
                g_signal_new ("cancelled",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmGreeterServerClass, cancelled),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
        signals [CONNECTED] =
                g_signal_new ("connected",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmGreeterServerClass, connected),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
        signals [DISCONNECTED] =
                g_signal_new ("disconnected",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmGreeterServerClass, disconnected),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        signals [START_SESSION_WHEN_READY] =
                g_signal_new ("start-session-when-ready",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmGreeterServerClass, start_session_when_ready),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        signals [START_SESSION_LATER] =
                g_signal_new ("start-session-later",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmGreeterServerClass, start_session_later),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
}

static void
mdm_greeter_server_init (MdmGreeterServer *greeter_server)
{

        greeter_server->priv = MDM_GREETER_SERVER_GET_PRIVATE (greeter_server);
}

static void
mdm_greeter_server_finalize (GObject *object)
{
        MdmGreeterServer *greeter_server;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_GREETER_SERVER (object));

        greeter_server = MDM_GREETER_SERVER (object);

        g_return_if_fail (greeter_server->priv != NULL);

        mdm_greeter_server_stop (greeter_server);

        G_OBJECT_CLASS (mdm_greeter_server_parent_class)->finalize (object);
}

MdmGreeterServer *
mdm_greeter_server_new (const char *display_id)
{
        GObject *object;

        object = g_object_new (MDM_TYPE_GREETER_SERVER,
                               "display-id", display_id,
                               NULL);

        return MDM_GREETER_SERVER (object);
}
