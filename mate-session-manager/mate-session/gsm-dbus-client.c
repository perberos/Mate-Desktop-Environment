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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include "config.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "gsm-dbus-client.h"
#include "gsm-marshal.h"

#include "gsm-manager.h"

#define GSM_DBUS_CLIENT_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GSM_TYPE_DBUS_CLIENT, GsmDBusClientPrivate))

#define IS_STRING_EMPTY(x) ((x)==NULL||(x)[0]=='\0')


#define SM_DBUS_NAME                     "org.mate.SessionManager"
#define SM_DBUS_CLIENT_PRIVATE_INTERFACE "org.mate.SessionManager.ClientPrivate"

struct GsmDBusClientPrivate
{
        char                 *bus_name;
        GPid                  caller_pid;
        GsmClientRestartStyle restart_style_hint;
        DBusConnection       *connection;
};

enum {
        PROP_0,
        PROP_BUS_NAME
};

G_DEFINE_TYPE (GsmDBusClient, gsm_dbus_client, GSM_TYPE_CLIENT)

GQuark
gsm_dbus_client_error_quark (void)
{
        static GQuark ret = 0;
        if (ret == 0) {
                ret = g_quark_from_static_string ("gsm_dbus_client_error");
        }

        return ret;
}

#define ENUM_ENTRY(NAME, DESC) { NAME, "" #NAME "", DESC }

GType
gsm_dbus_client_error_get_type (void)
{
        static GType etype = 0;

        if (etype == 0) {
                static const GEnumValue values[] = {
                        ENUM_ENTRY (GSM_DBUS_CLIENT_ERROR_GENERAL, "GeneralError"),
                        ENUM_ENTRY (GSM_DBUS_CLIENT_ERROR_NOT_CLIENT, "NotClient"),
                        { 0, 0, 0 }
                };

                g_assert (GSM_DBUS_CLIENT_NUM_ERRORS == G_N_ELEMENTS (values) - 1);

                etype = g_enum_register_static ("GsmDbusClientError", values);
        }

        return etype;
}

static gboolean
setup_connection (GsmDBusClient *client)
{
        DBusError error;

        dbus_error_init (&error);

        if (client->priv->connection == NULL) {
                client->priv->connection = dbus_bus_get (DBUS_BUS_SESSION, &error);
                if (client->priv->connection == NULL) {
                        if (dbus_error_is_set (&error)) {
                                g_debug ("GsmDbusClient: Couldn't connect to session bus: %s",
                                         error.message);
                                dbus_error_free (&error);
                        }
                        return FALSE;
                }

                dbus_connection_setup_with_g_main (client->priv->connection, NULL);
                dbus_connection_set_exit_on_disconnect (client->priv->connection, FALSE);
        }

        return TRUE;
}

static void
raise_error (DBusConnection *connection,
             DBusMessage    *in_reply_to,
             const char     *error_name,
             char           *format, ...)
{
        char         buf[512];
        DBusMessage *reply;

        va_list args;
        va_start (args, format);
        vsnprintf (buf, sizeof (buf), format, args);
        va_end (args);

        reply = dbus_message_new_error (in_reply_to, error_name, buf);
        if (reply == NULL) {
                g_error ("No memory");
        }
        if (! dbus_connection_send (connection, reply, NULL)) {
                g_error ("No memory");
        }

        dbus_message_unref (reply);
}

static void
handle_end_session_response (GsmDBusClient *client,
                             DBusMessage   *message)
{
        const char     *sender;
        DBusMessage    *reply;
        DBusError       error;
        dbus_bool_t     is_ok;
        const char     *reason;

        dbus_error_init (&error);
        if (! dbus_message_get_args (message, &error,
                                     DBUS_TYPE_BOOLEAN, &is_ok,
                                     DBUS_TYPE_STRING, &reason,
                                     DBUS_TYPE_INVALID)) {
                if (dbus_error_is_set (&error)) {
                        g_warning ("Invalid method call: %s", error.message);
                        dbus_error_free (&error);
                }
                raise_error (client->priv->connection,
                             message,
                             DBUS_ERROR_FAILED,
                             "There is a syntax error in the invocation of the method EndSessionResponse");
                return;
        }

        g_debug ("GsmDBusClient: got EndSessionResponse is-ok:%d reason=%s", is_ok, reason);

        /* make sure it is from our client */
        sender = dbus_message_get_sender (message);
        if (sender == NULL
            || IS_STRING_EMPTY (client->priv->bus_name)
            || strcmp (sender, client->priv->bus_name) != 0) {

                raise_error (client->priv->connection,
                             message,
                             DBUS_ERROR_FAILED,
                             "Caller not recognized as the client");
                return;
        }

        reply = dbus_message_new_method_return (message);
        if (reply == NULL) {
                g_error ("No memory");
        }

        gsm_client_end_session_response (GSM_CLIENT (client),
                                         is_ok, FALSE, FALSE, reason);


        if (! dbus_connection_send (client->priv->connection, reply, NULL)) {
                g_error ("No memory");
        }

        dbus_message_unref (reply);
}

static DBusHandlerResult
client_dbus_filter_function (DBusConnection *connection,
                             DBusMessage    *message,
                             void           *user_data)
{
        GsmDBusClient *client = GSM_DBUS_CLIENT (user_data);
        const char    *path;

        g_return_val_if_fail (connection != NULL, DBUS_HANDLER_RESULT_NOT_YET_HANDLED);
        g_return_val_if_fail (message != NULL, DBUS_HANDLER_RESULT_NOT_YET_HANDLED);

        path = dbus_message_get_path (message);

        g_debug ("GsmDBusClient: obj_path=%s interface=%s method=%s",
                 dbus_message_get_path (message),
                 dbus_message_get_interface (message),
                 dbus_message_get_member (message));

        if (dbus_message_is_method_call (message, SM_DBUS_CLIENT_PRIVATE_INTERFACE, "EndSessionResponse")) {
                g_assert (gsm_client_peek_id (GSM_CLIENT (client)) != NULL);

                if (path != NULL && strcmp (path, gsm_client_peek_id (GSM_CLIENT (client))) != 0) {
                        /* Different object path */
                        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
                }
                handle_end_session_response (client, message);
                return DBUS_HANDLER_RESULT_HANDLED;
        }

        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static GObject *
gsm_dbus_client_constructor (GType                  type,
                             guint                  n_construct_properties,
                             GObjectConstructParam *construct_properties)
{
        GsmDBusClient *client;

        client = GSM_DBUS_CLIENT (G_OBJECT_CLASS (gsm_dbus_client_parent_class)->constructor (type,
                                                                                              n_construct_properties,
                                                                                              construct_properties));

        if (! setup_connection (client)) {
                g_object_unref (client);
                return NULL;
        }

        /* Object path is already registered by base class */
        dbus_connection_add_filter (client->priv->connection, client_dbus_filter_function, client, NULL);

        return G_OBJECT (client);
}

static void
gsm_dbus_client_init (GsmDBusClient *client)
{
        client->priv = GSM_DBUS_CLIENT_GET_PRIVATE (client);
}

/* adapted from PolicyKit */
static gboolean
get_caller_info (GsmDBusClient *client,
                 const char    *sender,
                 uid_t         *calling_uid,
                 pid_t         *calling_pid)
{
        gboolean         res;
        GError          *error;
        DBusGConnection *connection;
        DBusGProxy      *bus_proxy;

        res = FALSE;
        bus_proxy = NULL;

        if (sender == NULL) {
                goto out;
        }

        error = NULL;
        connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
        if (connection == NULL) {
                if (error != NULL) {
                        g_warning ("error getting session bus: %s", error->message);
                        g_error_free (error);
                }
                goto out;
        }

        bus_proxy = dbus_g_proxy_new_for_name (connection,
                                               DBUS_SERVICE_DBUS,
                                               DBUS_PATH_DBUS,
                                               DBUS_INTERFACE_DBUS);

        error = NULL;
        if (! dbus_g_proxy_call (bus_proxy, "GetConnectionUnixUser", &error,
                                 G_TYPE_STRING, sender,
                                 G_TYPE_INVALID,
                                 G_TYPE_UINT, calling_uid,
                                 G_TYPE_INVALID)) {
                g_debug ("GetConnectionUnixUser() failed: %s", error->message);
                g_error_free (error);
                goto out;
        }

        error = NULL;
        if (! dbus_g_proxy_call (bus_proxy, "GetConnectionUnixProcessID", &error,
                                 G_TYPE_STRING, sender,
                                 G_TYPE_INVALID,
                                 G_TYPE_UINT, calling_pid,
                                 G_TYPE_INVALID)) {
                g_debug ("GetConnectionUnixProcessID() failed: %s", error->message);
                g_error_free (error);
                goto out;
        }

        res = TRUE;

        g_debug ("uid = %d", *calling_uid);
        g_debug ("pid = %d", *calling_pid);

out:
        if (bus_proxy != NULL) {
                g_object_unref (bus_proxy);
        }
        return res;
}

static void
gsm_dbus_client_set_bus_name (GsmDBusClient  *client,
                              const char     *bus_name)
{
        uid_t    uid;
        pid_t    pid;

        g_return_if_fail (GSM_IS_DBUS_CLIENT (client));

        g_free (client->priv->bus_name);

        client->priv->bus_name = g_strdup (bus_name);
        g_object_notify (G_OBJECT (client), "bus-name");

        if (client->priv->bus_name != NULL) {
                gboolean res;

                res = get_caller_info (client, bus_name, &uid, &pid);
                if (! res) {
                        pid = 0;
                }
        } else {
                pid = 0;
        }
        client->priv->caller_pid = pid;
}

const char *
gsm_dbus_client_get_bus_name (GsmDBusClient  *client)
{
        g_return_val_if_fail (GSM_IS_DBUS_CLIENT (client), NULL);

        return client->priv->bus_name;
}

static void
gsm_dbus_client_set_property (GObject       *object,
                              guint          prop_id,
                              const GValue  *value,
                              GParamSpec    *pspec)
{
        GsmDBusClient *self;

        self = GSM_DBUS_CLIENT (object);

        switch (prop_id) {
        case PROP_BUS_NAME:
                gsm_dbus_client_set_bus_name (self, g_value_get_string (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gsm_dbus_client_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
        GsmDBusClient *self;

        self = GSM_DBUS_CLIENT (object);

        switch (prop_id) {
        case PROP_BUS_NAME:
                g_value_set_string (value, self->priv->bus_name);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gsm_dbus_client_finalize (GObject *object)
{
        GsmDBusClient *client = (GsmDBusClient *) object;

        g_free (client->priv->bus_name);

        G_OBJECT_CLASS (gsm_dbus_client_parent_class)->finalize (object);
}

static GKeyFile *
dbus_client_save (GsmClient *client,
                  GError   **error)
{
        g_debug ("GsmDBusClient: saving client with id %s",
                 gsm_client_peek_id (client));

        /* FIXME: We still don't support client saving for D-Bus
         * session clients */

        return NULL;
}

static gboolean
dbus_client_stop (GsmClient *client,
                  GError   **error)
{
        GsmDBusClient  *dbus_client = (GsmDBusClient *) client;
        DBusMessage    *message;
        gboolean        ret;

        ret = FALSE;

        /* unicast the signal to only the registered bus name */
        message = dbus_message_new_signal (gsm_client_peek_id (client),
                                           SM_DBUS_CLIENT_PRIVATE_INTERFACE,
                                           "Stop");
        if (message == NULL) {
                goto out;
        }
        if (!dbus_message_set_destination (message, dbus_client->priv->bus_name)) {
                goto out;
        }

        if (!dbus_connection_send (dbus_client->priv->connection, message, NULL)) {
                goto out;
        }

        ret = TRUE;

 out:
        if (message != NULL) {
                dbus_message_unref (message);
        }

        return ret;
}

static char *
dbus_client_get_app_name (GsmClient *client)
{
        /* Always use app-id instead */
        return NULL;
}

static GsmClientRestartStyle
dbus_client_get_restart_style_hint (GsmClient *client)
{
        return (GSM_DBUS_CLIENT (client)->priv->restart_style_hint);
}

static guint
dbus_client_get_unix_process_id (GsmClient *client)
{
        return (GSM_DBUS_CLIENT (client)->priv->caller_pid);
}

static gboolean
dbus_client_query_end_session (GsmClient *client,
                               guint      flags,
                               GError   **error)
{
        GsmDBusClient  *dbus_client = (GsmDBusClient *) client;
        DBusMessage    *message;
        DBusMessageIter iter;
        gboolean        ret;

        ret = FALSE;

        if (dbus_client->priv->bus_name == NULL) {
                g_set_error (error,
                             GSM_CLIENT_ERROR,
                             GSM_CLIENT_ERROR_NOT_REGISTERED,
                             "Client is not registered");
                return FALSE;
        }

        g_debug ("GsmDBusClient: sending QueryEndSession signal to %s", dbus_client->priv->bus_name);

        /* unicast the signal to only the registered bus name */
        message = dbus_message_new_signal (gsm_client_peek_id (client),
                                           SM_DBUS_CLIENT_PRIVATE_INTERFACE,
                                           "QueryEndSession");
        if (message == NULL) {
                g_set_error (error,
                             GSM_CLIENT_ERROR,
                             GSM_CLIENT_ERROR_NOT_REGISTERED,
                             "Unable to send QueryEndSession message");
                goto out;
        }
        if (!dbus_message_set_destination (message, dbus_client->priv->bus_name)) {
                g_set_error (error,
                             GSM_CLIENT_ERROR,
                             GSM_CLIENT_ERROR_NOT_REGISTERED,
                             "Unable to send QueryEndSession message");
                goto out;
        }

        dbus_message_iter_init_append (message, &iter);
        dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32, &flags);

        if (!dbus_connection_send (dbus_client->priv->connection, message, NULL)) {
                g_set_error (error,
                             GSM_CLIENT_ERROR,
                             GSM_CLIENT_ERROR_NOT_REGISTERED,
                             "Unable to send QueryEndSession message");
                goto out;
        }

        ret = TRUE;

 out:
        if (message != NULL) {
                dbus_message_unref (message);
        }

        return ret;
}

static gboolean
dbus_client_end_session (GsmClient *client,
                         guint      flags,
                         GError   **error)
{
        GsmDBusClient  *dbus_client = (GsmDBusClient *) client;
        DBusMessage    *message;
        DBusMessageIter iter;
        gboolean        ret;

        ret = FALSE;

        /* unicast the signal to only the registered bus name */
        message = dbus_message_new_signal (gsm_client_peek_id (client),
                                           SM_DBUS_CLIENT_PRIVATE_INTERFACE,
                                           "EndSession");
        if (message == NULL) {
                g_set_error (error,
                             GSM_CLIENT_ERROR,
                             GSM_CLIENT_ERROR_NOT_REGISTERED,
                             "Unable to send EndSession message");
                goto out;
        }
        if (!dbus_message_set_destination (message, dbus_client->priv->bus_name)) {
                g_set_error (error,
                             GSM_CLIENT_ERROR,
                             GSM_CLIENT_ERROR_NOT_REGISTERED,
                             "Unable to send EndSession message");
                goto out;
        }

        dbus_message_iter_init_append (message, &iter);
        dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32, &flags);

        if (!dbus_connection_send (dbus_client->priv->connection, message, NULL)) {
                g_set_error (error,
                             GSM_CLIENT_ERROR,
                             GSM_CLIENT_ERROR_NOT_REGISTERED,
                             "Unable to send EndSession message");
                goto out;
        }

        ret = TRUE;

 out:
        if (message != NULL) {
                dbus_message_unref (message);
        }
        return ret;
}

static gboolean
dbus_client_cancel_end_session (GsmClient *client,
                                GError   **error)
{
        GsmDBusClient  *dbus_client = (GsmDBusClient *) client;
        DBusMessage    *message;
        gboolean        ret;

        /* unicast the signal to only the registered bus name */
        message = dbus_message_new_signal (gsm_client_peek_id (client),
                                           SM_DBUS_CLIENT_PRIVATE_INTERFACE,
                                           "CancelEndSession");
        if (message == NULL) {
                g_set_error (error,
                             GSM_CLIENT_ERROR,
                             GSM_CLIENT_ERROR_NOT_REGISTERED,
                             "Unable to send CancelEndSession message");
                goto out;
        }
        if (!dbus_message_set_destination (message, dbus_client->priv->bus_name)) {
                g_set_error (error,
                             GSM_CLIENT_ERROR,
                             GSM_CLIENT_ERROR_NOT_REGISTERED,
                             "Unable to send CancelEndSession message");
                goto out;
        }

        if (!dbus_connection_send (dbus_client->priv->connection, message, NULL)) {
                g_set_error (error,
                             GSM_CLIENT_ERROR,
                             GSM_CLIENT_ERROR_NOT_REGISTERED,
                             "Unable to send CancelEndSession message");
                goto out;
        }

        ret = TRUE;

 out:
        if (message != NULL) {
                dbus_message_unref (message);
        }

        return ret;
}

static void
gsm_dbus_client_dispose (GObject *object)
{
        GsmDBusClient *client;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GSM_IS_DBUS_CLIENT (object));

        client = GSM_DBUS_CLIENT (object);

        dbus_connection_remove_filter (client->priv->connection, client_dbus_filter_function, client);

        G_OBJECT_CLASS (gsm_dbus_client_parent_class)->dispose (object);
}

static void
gsm_dbus_client_class_init (GsmDBusClientClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);
        GsmClientClass *client_class = GSM_CLIENT_CLASS (klass);

        object_class->finalize             = gsm_dbus_client_finalize;
        object_class->constructor          = gsm_dbus_client_constructor;
        object_class->get_property         = gsm_dbus_client_get_property;
        object_class->set_property         = gsm_dbus_client_set_property;
        object_class->dispose              = gsm_dbus_client_dispose;

        client_class->impl_save                   = dbus_client_save;
        client_class->impl_stop                   = dbus_client_stop;
        client_class->impl_query_end_session      = dbus_client_query_end_session;
        client_class->impl_end_session            = dbus_client_end_session;
        client_class->impl_cancel_end_session     = dbus_client_cancel_end_session;
        client_class->impl_get_app_name           = dbus_client_get_app_name;
        client_class->impl_get_restart_style_hint = dbus_client_get_restart_style_hint;
        client_class->impl_get_unix_process_id    = dbus_client_get_unix_process_id;

        g_object_class_install_property (object_class,
                                         PROP_BUS_NAME,
                                         g_param_spec_string ("bus-name",
                                                              "bus-name",
                                                              "bus-name",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

        g_type_class_add_private (klass, sizeof (GsmDBusClientPrivate));
}

GsmClient *
gsm_dbus_client_new (const char *startup_id,
                     const char *bus_name)
{
        GsmDBusClient *client;

        client = g_object_new (GSM_TYPE_DBUS_CLIENT,
                               "startup-id", startup_id,
                               "bus-name", bus_name,
                               NULL);

        return GSM_CLIENT (client);
}
