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
#include <unistd.h>
#include <string.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "gdm-greeter-client.h"
#include "gdm-marshal.h"
#include "gdm-profile.h"

#define GDM_GREETER_CLIENT_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GDM_TYPE_GREETER_CLIENT, GdmGreeterClientPrivate))

#define GREETER_SERVER_DBUS_PATH      "/org/mate/DisplayManager/GreeterServer"
#define GREETER_SERVER_DBUS_INTERFACE "org.mate.DisplayManager.GreeterServer"

#define GDM_DBUS_NAME              "org.mate.DisplayManager"
#define GDM_DBUS_DISPLAY_INTERFACE "org.mate.DisplayManager.Display"

struct GdmGreeterClientPrivate
{
        DBusConnection   *connection;
        char             *address;

        char             *display_id;
        gboolean          display_is_local;
};

enum {
        PROP_0,
};

enum {
        INFO,
        PROBLEM,
        INFO_QUERY,
        SECRET_INFO_QUERY,
        READY,
        RESET,
        AUTHENTICATION_FAILED,
        SELECTED_USER_CHANGED,
        DEFAULT_LANGUAGE_NAME_CHANGED,
        DEFAULT_LAYOUT_NAME_CHANGED,
        DEFAULT_SESSION_NAME_CHANGED,
        TIMED_LOGIN_REQUESTED,
        USER_AUTHORIZED,
        LAST_SIGNAL
};

static guint gdm_greeter_client_signals [LAST_SIGNAL];

static void     gdm_greeter_client_class_init  (GdmGreeterClientClass *klass);
static void     gdm_greeter_client_init        (GdmGreeterClient      *greeter_client);
static void     gdm_greeter_client_finalize    (GObject              *object);

G_DEFINE_TYPE (GdmGreeterClient, gdm_greeter_client, G_TYPE_OBJECT)

static gpointer client_object = NULL;

GQuark
gdm_greeter_client_error_quark (void)
{
        static GQuark error_quark = 0;

        if (error_quark == 0)
                error_quark = g_quark_from_static_string ("gdm-greeter-client");

        return error_quark;
}

gboolean
gdm_greeter_client_get_display_is_local (GdmGreeterClient *client)
{
        g_return_val_if_fail (GDM_IS_GREETER_CLIENT (client), FALSE);

        return client->priv->display_is_local;
}

static void
emit_string_and_int_signal_for_message (GdmGreeterClient *client,
                                        const char       *name,
                                        DBusMessage      *message,
                                        int               signal)
{
        DBusError   error;
        const char *text;
        int         number;
        dbus_bool_t res;

        dbus_error_init (&error);
        res = dbus_message_get_args (message,
                                     &error,
                                     DBUS_TYPE_STRING, &text,
                                     DBUS_TYPE_INT32, &number,
                                     DBUS_TYPE_INVALID);
        if (res) {

                g_debug ("GdmGreeterClient: Received %s (%s %d)", name, text, number);

                g_signal_emit (client,
                               gdm_greeter_client_signals[signal],
                               0, text, number);
        } else {
                g_warning ("Unable to get arguments: %s", error.message);
                dbus_error_free (&error);
        }
}

static void
emit_string_signal_for_message (GdmGreeterClient *client,
                                const char       *name,
                                DBusMessage      *message,
                                int               signal)
{
        DBusError   error;
        const char *text;
        dbus_bool_t res;

        dbus_error_init (&error);
        res = dbus_message_get_args (message,
                                     &error,
                                     DBUS_TYPE_STRING, &text,
                                     DBUS_TYPE_INVALID);
        if (res) {

                g_debug ("GdmGreeterClient: Received %s (%s)", name, text);

                g_signal_emit (client,
                               gdm_greeter_client_signals[signal],
                               0, text);
        } else {
                g_warning ("Unable to get arguments: %s", error.message);
                dbus_error_free (&error);
        }
}

static void
on_selected_user_changed (GdmGreeterClient *client,
                          DBusMessage      *message)
{
        emit_string_signal_for_message (client, "SelectedUserChanged", message, SELECTED_USER_CHANGED);
}

static void
on_default_language_name_changed (GdmGreeterClient *client,
                                  DBusMessage      *message)
{
        emit_string_signal_for_message (client, "DefaultLanguageNameChanged", message, DEFAULT_LANGUAGE_NAME_CHANGED);
}

static void
on_default_layout_name_changed (GdmGreeterClient *client,
                                DBusMessage      *message)
{
        emit_string_signal_for_message (client, "DefaultLayoutNameChanged", message, DEFAULT_LAYOUT_NAME_CHANGED);
}

static void
on_default_session_name_changed (GdmGreeterClient *client,
                                 DBusMessage      *message)
{
        emit_string_signal_for_message (client, "DefaultSessionNameChanged", message, DEFAULT_SESSION_NAME_CHANGED);
}

static void
on_timed_login_requested (GdmGreeterClient *client,
                          DBusMessage      *message)
{
        emit_string_and_int_signal_for_message (client, "TimedLoginRequested", message, TIMED_LOGIN_REQUESTED);
}

static void
on_user_authorized (GdmGreeterClient *client,
                    DBusMessage      *message)
{
        g_signal_emit (client,
                       gdm_greeter_client_signals[USER_AUTHORIZED],
                       0);
}

static void
on_info_query (GdmGreeterClient *client,
               DBusMessage      *message)
{
        emit_string_signal_for_message (client, "InfoQuery", message, INFO_QUERY);
}

static void
on_secret_info_query (GdmGreeterClient *client,
                      DBusMessage      *message)
{
        emit_string_signal_for_message (client, "SecretInfoQuery", message, SECRET_INFO_QUERY);
}

static void
on_info (GdmGreeterClient *client,
         DBusMessage      *message)
{
        emit_string_signal_for_message (client, "Info", message, INFO);
}

static void
on_problem (GdmGreeterClient *client,
            DBusMessage      *message)
{
        emit_string_signal_for_message (client, "Problem", message, PROBLEM);
}

static void
on_ready (GdmGreeterClient *client,
          DBusMessage      *message)
{
        g_debug ("GdmGreeterClient: Ready");

        g_signal_emit (client,
                       gdm_greeter_client_signals[READY],
                       0);
}

static void
on_reset (GdmGreeterClient *client,
          DBusMessage      *message)
{
        g_debug ("GdmGreeterClient: Reset");

        g_signal_emit (client,
                       gdm_greeter_client_signals[RESET],
                       0);
}

static void
on_authentication_failed (GdmGreeterClient *client,
                          DBusMessage      *message)
{
        g_debug ("GdmGreeterClient: Authentication failed");

        g_signal_emit (client,
                       gdm_greeter_client_signals[AUTHENTICATION_FAILED],
                       0);
}

static gboolean
send_dbus_string_method (DBusConnection *connection,
                         const char     *method,
                         const char     *payload)
{
        DBusError       error;
        DBusMessage    *message;
        DBusMessage    *reply;
        DBusMessageIter iter;
        const char     *str;

        if (payload != NULL) {
                str = payload;
        } else {
                str = "";
        }

        g_debug ("GdmGreeterClient: Calling %s", method);
        message = dbus_message_new_method_call (NULL,
                                                GREETER_SERVER_DBUS_PATH,
                                                GREETER_SERVER_DBUS_INTERFACE,
                                                method);
        if (message == NULL) {
                g_warning ("Couldn't allocate the D-Bus message");
                return FALSE;
        }

        dbus_message_iter_init_append (message, &iter);
        dbus_message_iter_append_basic (&iter,
                                        DBUS_TYPE_STRING,
                                        &str);

        dbus_error_init (&error);
        reply = dbus_connection_send_with_reply_and_block (connection,
                                                           message,
                                                           -1,
                                                           &error);

        dbus_message_unref (message);

        if (dbus_error_is_set (&error)) {
                g_warning ("%s %s raised: %s\n",
                           method,
                           error.name,
                           error.message);
                return FALSE;
        }
        if (reply != NULL) {
                dbus_message_unref (reply);
        }
        dbus_connection_flush (connection);

        return TRUE;
}

static gboolean
send_dbus_bool_method (DBusConnection *connection,
                       const char     *method,
                       gboolean        payload)
{
        DBusError       error;
        DBusMessage    *message;
        DBusMessage    *reply;
        DBusMessageIter iter;

        g_debug ("GdmGreeterClient: Calling %s", method);
        message = dbus_message_new_method_call (NULL,
                                                GREETER_SERVER_DBUS_PATH,
                                                GREETER_SERVER_DBUS_INTERFACE,
                                                method);
        if (message == NULL) {
                g_warning ("Couldn't allocate the D-Bus message");
                return FALSE;
        }

        dbus_message_iter_init_append (message, &iter);
        dbus_message_iter_append_basic (&iter,
                                        DBUS_TYPE_BOOLEAN,
                                        &payload);

        dbus_error_init (&error);
        reply = dbus_connection_send_with_reply_and_block (connection,
                                                           message,
                                                           -1,
                                                           &error);

        dbus_message_unref (message);

        if (dbus_error_is_set (&error)) {
                g_warning ("%s %s raised: %s\n",
                           method,
                           error.name,
                           error.message);
                return FALSE;
        }
        if (reply != NULL) {
                dbus_message_unref (reply);
        }
        dbus_connection_flush (connection);

        return TRUE;
}

static gboolean
send_dbus_void_method (DBusConnection *connection,
                       const char     *method)
{
        DBusError       error;
        DBusMessage    *message;
        DBusMessage    *reply;

        g_debug ("GdmGreeterClient: Calling %s", method);
        message = dbus_message_new_method_call (NULL,
                                                GREETER_SERVER_DBUS_PATH,
                                                GREETER_SERVER_DBUS_INTERFACE,
                                                method);
        if (message == NULL) {
                g_warning ("Couldn't allocate the D-Bus message");
                return FALSE;
        }

        dbus_error_init (&error);
        reply = dbus_connection_send_with_reply_and_block (connection,
                                                           message,
                                                           -1,
                                                           &error);

        dbus_message_unref (message);

        if (dbus_error_is_set (&error)) {
                g_warning ("%s %s raised: %s\n",
                           method,
                           error.name,
                           error.message);
                return FALSE;
        }
        if (reply != NULL) {
                dbus_message_unref (reply);
        }
        dbus_connection_flush (connection);

        return TRUE;
}

void
gdm_greeter_client_call_begin_auto_login (GdmGreeterClient *client,
                                          const char       *username)
{
        send_dbus_string_method (client->priv->connection,
                                 "BeginAutoLogin", username);
}

void
gdm_greeter_client_call_begin_verification (GdmGreeterClient *client)
{
        send_dbus_void_method (client->priv->connection,
                               "BeginVerification");
}

void
gdm_greeter_client_call_begin_verification_for_user (GdmGreeterClient *client,
                                                     const char       *username)
{
        send_dbus_string_method (client->priv->connection,
                                 "BeginVerificationForUser",
                                 username);
}

void
gdm_greeter_client_call_answer_query (GdmGreeterClient *client,
                                      const char       *text)
{
        send_dbus_string_method (client->priv->connection,
                                 "AnswerQuery",
                                 text);
}

void
gdm_greeter_client_call_start_session_when_ready  (GdmGreeterClient *client,
                                                   gboolean          should_start_session)
{
        send_dbus_bool_method (client->priv->connection,
                               "StartSessionWhenReady",
                               should_start_session);
}

void
gdm_greeter_client_call_select_session (GdmGreeterClient *client,
                                        const char       *text)
{
        send_dbus_string_method (client->priv->connection,
                                 "SelectSession",
                                 text);
}

void
gdm_greeter_client_call_select_language (GdmGreeterClient *client,
                                         const char       *text)
{
        send_dbus_string_method (client->priv->connection,
                                 "SelectLanguage",
                                 text);
}

void
gdm_greeter_client_call_select_layout (GdmGreeterClient *client,
                                       const char       *text)
{
        send_dbus_string_method (client->priv->connection,
                                 "SelectLayout",
                                 text);
}
void
gdm_greeter_client_call_select_user (GdmGreeterClient *client,
                                     const char       *text)
{
        send_dbus_string_method (client->priv->connection,
                                 "SelectUser",
                                 text);
}

void
gdm_greeter_client_call_select_hostname (GdmGreeterClient *client,
                                         const char       *text)
{
        send_dbus_string_method (client->priv->connection,
                                 "SelectHostname",
                                 text);
}

void
gdm_greeter_client_call_cancel (GdmGreeterClient *client)
{
        send_dbus_void_method (client->priv->connection,
                               "Cancel");
}

void
gdm_greeter_client_call_disconnect (GdmGreeterClient *client)
{
        send_dbus_void_method (client->priv->connection,
                               "Disconnect");
}


static gboolean
send_get_display_id (GdmGreeterClient *client,
                     const char       *method,
                     char            **answerp)
{
        DBusError       error;
        DBusMessage    *message;
        DBusMessage    *reply;
        DBusMessageIter iter;
        gboolean        ret;
        const char     *answer;

        ret = FALSE;

        g_debug ("GdmGreeterClient: Calling %s", method);
        message = dbus_message_new_method_call (NULL,
                                                GREETER_SERVER_DBUS_PATH,
                                                GREETER_SERVER_DBUS_INTERFACE,
                                                method);
        if (message == NULL) {
                g_warning ("Couldn't allocate the D-Bus message");
                return FALSE;
        }

        dbus_error_init (&error);
        reply = dbus_connection_send_with_reply_and_block (client->priv->connection,
                                                           message,
                                                           -1,
                                                           &error);
        dbus_message_unref (message);

        if (dbus_error_is_set (&error)) {
                g_warning ("%s %s raised: %s\n",
                           method,
                           error.name,
                           error.message);
                goto out;
        }

        dbus_message_iter_init (reply, &iter);
        dbus_message_iter_get_basic (&iter, &answer);
        if (answerp != NULL) {
                *answerp = g_strdup (answer);
        }
        ret = TRUE;

        dbus_message_unref (reply);
        dbus_connection_flush (client->priv->connection);

 out:

        return ret;
}

char *
gdm_greeter_client_call_get_display_id (GdmGreeterClient *client)
{
        char *display_id;

        display_id = NULL;
        send_get_display_id (client,
                             "GetDisplayId",
                             &display_id);

        return display_id;
}

static void
cache_display_values (GdmGreeterClient *client)
{
        DBusGProxy      *display_proxy;
        DBusGConnection *connection;
        GError          *error;
        gboolean         res;

        g_free (client->priv->display_id);
        client->priv->display_id = gdm_greeter_client_call_get_display_id (client);
        if (client->priv->display_id == NULL) {
                return;
        }

        error = NULL;
        connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
        if (connection == NULL) {
                if (error != NULL) {
                        g_critical ("error getting system bus: %s", error->message);
                        g_error_free (error);
                }
                return;
        }

        g_debug ("GdmGreeterClient: Creating proxy for %s", client->priv->display_id);
        display_proxy = dbus_g_proxy_new_for_name (connection,
                                                   GDM_DBUS_NAME,
                                                   client->priv->display_id,
                                                   GDM_DBUS_DISPLAY_INTERFACE);
        /* cache some values up front */
        error = NULL;
        res = dbus_g_proxy_call (display_proxy,
                                 "IsLocal",
                                 &error,
                                 G_TYPE_INVALID,
                                 G_TYPE_BOOLEAN, &client->priv->display_is_local,
                                 G_TYPE_INVALID);
        if (! res) {
                if (error != NULL) {
                        g_warning ("Failed to get value: %s", error->message);
                        g_error_free (error);
                } else {
                        g_warning ("Failed to get value");
                }
        }
}

static DBusHandlerResult
client_dbus_handle_message (DBusConnection *connection,
                            DBusMessage    *message,
                            void           *user_data,
                            dbus_bool_t     local_interface)
{
        GdmGreeterClient *client = GDM_GREETER_CLIENT (user_data);

#if 0
        g_message ("obj_path=%s interface=%s method=%s destination=%s",
                   dbus_message_get_path (message),
                   dbus_message_get_interface (message),
                   dbus_message_get_member (message),
                   dbus_message_get_destination (message));
#endif

        g_return_val_if_fail (connection != NULL, DBUS_HANDLER_RESULT_NOT_YET_HANDLED);
        g_return_val_if_fail (message != NULL, DBUS_HANDLER_RESULT_NOT_YET_HANDLED);

        if (dbus_message_is_signal (message, GREETER_SERVER_DBUS_INTERFACE, "InfoQuery")) {
                on_info_query (client, message);
        } else if (dbus_message_is_signal (message, GREETER_SERVER_DBUS_INTERFACE, "SecretInfoQuery")) {
                on_secret_info_query (client, message);
        } else if (dbus_message_is_signal (message, GREETER_SERVER_DBUS_INTERFACE, "Info")) {
                on_info (client, message);
        } else if (dbus_message_is_signal (message, GREETER_SERVER_DBUS_INTERFACE, "Problem")) {
                on_problem (client, message);
        } else if (dbus_message_is_signal (message, GREETER_SERVER_DBUS_INTERFACE, "Ready")) {
                on_ready (client, message);
        } else if (dbus_message_is_signal (message, GREETER_SERVER_DBUS_INTERFACE, "Reset")) {
                on_reset (client, message);
        } else if (dbus_message_is_signal (message, GREETER_SERVER_DBUS_INTERFACE, "AuthenticationFailed")) {
                on_authentication_failed (client, message);
        } else if (dbus_message_is_signal (message, GREETER_SERVER_DBUS_INTERFACE, "SelectedUserChanged")) {
                on_selected_user_changed (client, message);
        } else if (dbus_message_is_signal (message, GREETER_SERVER_DBUS_INTERFACE, "DefaultLanguageNameChanged")) {
                on_default_language_name_changed (client, message);
        } else if (dbus_message_is_signal (message, GREETER_SERVER_DBUS_INTERFACE, "DefaultLayoutNameChanged")) {
                on_default_layout_name_changed (client, message);
        } else if (dbus_message_is_signal (message, GREETER_SERVER_DBUS_INTERFACE, "DefaultSessionNameChanged")) {
                on_default_session_name_changed (client, message);
        } else if (dbus_message_is_signal (message, GREETER_SERVER_DBUS_INTERFACE, "TimedLoginRequested")) {
                on_timed_login_requested (client, message);
        } else if (dbus_message_is_signal (message, GREETER_SERVER_DBUS_INTERFACE, "UserAuthorized")) {
                on_user_authorized (client, message);
        } else {
                return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
client_dbus_filter_function (DBusConnection *connection,
                             DBusMessage    *message,
                             void           *user_data)
{
        GdmGreeterClient *client = GDM_GREETER_CLIENT (user_data);
        const char       *path;

        path = dbus_message_get_path (message);

        g_debug ("GdmGreeterClient: obj_path=%s interface=%s method=%s",
                 dbus_message_get_path (message),
                 dbus_message_get_interface (message),
                 dbus_message_get_member (message));

        if (dbus_message_is_signal (message, DBUS_INTERFACE_LOCAL, "Disconnected")
            && strcmp (path, DBUS_PATH_LOCAL) == 0) {

                g_message ("Got disconnected from the session message bus");

                dbus_connection_unref (connection);
                client->priv->connection = NULL;

        } else if (dbus_message_is_signal (message,
                                           DBUS_INTERFACE_DBUS,
                                           "NameOwnerChanged")) {
                g_debug ("GdmGreeterClient: Name owner changed?");
        } else {
                return client_dbus_handle_message (connection, message, user_data, FALSE);
        }

        return DBUS_HANDLER_RESULT_HANDLED;
}

gboolean
gdm_greeter_client_start (GdmGreeterClient *client,
                          GError          **error)
{
        gboolean  ret;
        DBusError local_error;

        g_return_val_if_fail (GDM_IS_GREETER_CLIENT (client), FALSE);

        gdm_profile_start (NULL);

        ret = FALSE;

        if (client->priv->address == NULL) {
                g_warning ("GDM_GREETER_DBUS_ADDRESS not set");
                g_set_error (error,
                             GDM_GREETER_CLIENT_ERROR,
                             GDM_GREETER_CLIENT_ERROR_GENERIC,
                             "GDM_GREETER_DBUS_ADDRESS not set");
                goto out;
        }

        g_debug ("GdmGreeterClient: connecting to address: %s", client->priv->address);

        dbus_error_init (&local_error);
        client->priv->connection = dbus_connection_open (client->priv->address, &local_error);
        if (client->priv->connection == NULL) {
                if (dbus_error_is_set (&local_error)) {
                        g_warning ("error opening connection: %s", local_error.message);
                        g_set_error (error,
                                     GDM_GREETER_CLIENT_ERROR,
                                     GDM_GREETER_CLIENT_ERROR_GENERIC,
                                     "%s", local_error.message);
                        dbus_error_free (&local_error);
                } else {
                        g_warning ("Unable to open connection");
                }
                goto out;
        }

        dbus_connection_setup_with_g_main (client->priv->connection, NULL);
        dbus_connection_set_exit_on_disconnect (client->priv->connection, TRUE);

        dbus_connection_add_filter (client->priv->connection,
                                    client_dbus_filter_function,
                                    client,
                                    NULL);

        cache_display_values (client);

        ret = TRUE;

 out:
        gdm_profile_end (NULL);

        return ret;
}

void
gdm_greeter_client_stop (GdmGreeterClient *client)
{
        g_return_if_fail (GDM_IS_GREETER_CLIENT (client));

}

static void
gdm_greeter_client_set_property (GObject        *object,
                                 guint           prop_id,
                                 const GValue   *value,
                                 GParamSpec     *pspec)
{
        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gdm_greeter_client_get_property (GObject        *object,
                                 guint           prop_id,
                                 GValue         *value,
                                 GParamSpec     *pspec)
{
        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static GObject *
gdm_greeter_client_constructor (GType                  type,
                                 guint                  n_construct_properties,
                                 GObjectConstructParam *construct_properties)
{
        GdmGreeterClient      *greeter_client;

        greeter_client = GDM_GREETER_CLIENT (G_OBJECT_CLASS (gdm_greeter_client_parent_class)->constructor (type,
                                                                                                            n_construct_properties,
                                                                                                            construct_properties));

        return G_OBJECT (greeter_client);
}

static void
gdm_greeter_client_dispose (GObject *object)
{
        g_debug ("GdmGreeterClient: Disposing greeter_client");

        G_OBJECT_CLASS (gdm_greeter_client_parent_class)->dispose (object);
}

static void
gdm_greeter_client_class_init (GdmGreeterClientClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = gdm_greeter_client_get_property;
        object_class->set_property = gdm_greeter_client_set_property;
        object_class->constructor = gdm_greeter_client_constructor;
        object_class->dispose = gdm_greeter_client_dispose;
        object_class->finalize = gdm_greeter_client_finalize;

        g_type_class_add_private (klass, sizeof (GdmGreeterClientPrivate));

        gdm_greeter_client_signals[INFO_QUERY] =
                g_signal_new ("info-query",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GdmGreeterClientClass, info_query),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_STRING);

        gdm_greeter_client_signals[SECRET_INFO_QUERY] =
                g_signal_new ("secret-info-query",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GdmGreeterClientClass, secret_info_query),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_STRING);

        gdm_greeter_client_signals[INFO] =
                g_signal_new ("info",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (GdmGreeterClientClass, info),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_STRING);

        gdm_greeter_client_signals[PROBLEM] =
                g_signal_new ("problem",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (GdmGreeterClientClass, problem),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_STRING);

        gdm_greeter_client_signals[READY] =
                g_signal_new ("ready",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (GdmGreeterClientClass, ready),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        gdm_greeter_client_signals[RESET] =
                g_signal_new ("reset",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (GdmGreeterClientClass, reset),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
        gdm_greeter_client_signals[AUTHENTICATION_FAILED] =
                g_signal_new ("authentication-failed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (GdmGreeterClientClass, authentication_failed),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
        gdm_greeter_client_signals[SELECTED_USER_CHANGED] =
                g_signal_new ("selected-user-changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (GdmGreeterClientClass, selected_user_changed),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1, G_TYPE_STRING);
        gdm_greeter_client_signals[DEFAULT_LANGUAGE_NAME_CHANGED] =
                g_signal_new ("default-language-name-changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (GdmGreeterClientClass, default_language_name_changed),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1, G_TYPE_STRING);
        gdm_greeter_client_signals[DEFAULT_LAYOUT_NAME_CHANGED] =
                g_signal_new ("default-layout-name-changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (GdmGreeterClientClass, default_layout_name_changed),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1, G_TYPE_STRING);
        gdm_greeter_client_signals[DEFAULT_SESSION_NAME_CHANGED] =
                g_signal_new ("default-session-name-changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (GdmGreeterClientClass, default_session_name_changed),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1, G_TYPE_STRING);

        gdm_greeter_client_signals[TIMED_LOGIN_REQUESTED] =
                g_signal_new ("timed-login-requested",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (GdmGreeterClientClass, timed_login_requested),
                              NULL,
                              NULL,
                              gdm_marshal_VOID__STRING_INT,
                              G_TYPE_NONE,
                              2, G_TYPE_STRING, G_TYPE_INT);

        gdm_greeter_client_signals[USER_AUTHORIZED] =
                g_signal_new ("user-authorized",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (GdmGreeterClientClass, user_authorized),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE, 0);
}

static void
gdm_greeter_client_init (GdmGreeterClient *client)
{

        client->priv = GDM_GREETER_CLIENT_GET_PRIVATE (client);

        client->priv->address = g_strdup (g_getenv ("GDM_GREETER_DBUS_ADDRESS"));
}

static void
gdm_greeter_client_finalize (GObject *object)
{
        GdmGreeterClient *client;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GDM_IS_GREETER_CLIENT (object));

        client = GDM_GREETER_CLIENT (object);

        g_return_if_fail (client->priv != NULL);

        g_free (client->priv->address);

        G_OBJECT_CLASS (gdm_greeter_client_parent_class)->finalize (object);
}

GdmGreeterClient *
gdm_greeter_client_new (void)
{
        if (client_object != NULL) {
                g_object_ref (client_object);
        } else {
                client_object = g_object_new (GDM_TYPE_GREETER_CLIENT, NULL);
                g_object_add_weak_pointer (client_object,
                                           (gpointer *) &client_object);
        }

        return GDM_GREETER_CLIENT (client_object);
}
