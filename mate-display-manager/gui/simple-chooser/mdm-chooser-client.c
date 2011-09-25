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

#include "mdm-chooser-client.h"

#define MDM_CHOOSER_CLIENT_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_CHOOSER_CLIENT, MdmChooserClientPrivate))

#define CHOOSER_SERVER_DBUS_PATH      "/org/mate/DisplayManager/ChooserServer"
#define CHOOSER_SERVER_DBUS_INTERFACE "org.mate.DisplayManager.ChooserServer"

#define MDM_DBUS_NAME              "org.mate.DisplayManager"
#define MDM_DBUS_DISPLAY_INTERFACE "org.mate.DisplayManager.Display"

struct MdmChooserClientPrivate
{
        DBusConnection   *connection;
        char             *address;
};

enum {
        PROP_0,
};

static void     mdm_chooser_client_class_init  (MdmChooserClientClass *klass);
static void     mdm_chooser_client_init        (MdmChooserClient      *chooser_client);
static void     mdm_chooser_client_finalize    (GObject               *object);

G_DEFINE_TYPE (MdmChooserClient, mdm_chooser_client, G_TYPE_OBJECT)

static gpointer client_object = NULL;

GQuark
mdm_chooser_client_error_quark (void)
{
        static GQuark error_quark = 0;

        if (error_quark == 0)
                error_quark = g_quark_from_static_string ("mdm-chooser-client");

        return error_quark;
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

        g_debug ("MdmChooserClient: Calling %s", method);
        message = dbus_message_new_method_call (NULL,
                                                CHOOSER_SERVER_DBUS_PATH,
                                                CHOOSER_SERVER_DBUS_INTERFACE,
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
send_dbus_void_method (DBusConnection *connection,
                       const char     *method)
{
        DBusError       error;
        DBusMessage    *message;
        DBusMessage    *reply;

        g_debug ("MdmChooserClient: Calling %s", method);
        message = dbus_message_new_method_call (NULL,
                                                CHOOSER_SERVER_DBUS_PATH,
                                                CHOOSER_SERVER_DBUS_INTERFACE,
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
mdm_chooser_client_call_select_hostname (MdmChooserClient *client,
                                         const char       *text)
{
        send_dbus_string_method (client->priv->connection,
                                 "SelectHostname",
                                 text);
}

void
mdm_chooser_client_call_disconnect (MdmChooserClient *client)
{
        send_dbus_void_method (client->priv->connection,
                               "Disconnect");
}

static DBusHandlerResult
client_dbus_handle_message (DBusConnection *connection,
                            DBusMessage    *message,
                            void           *user_data,
                            dbus_bool_t     local_interface)
{

#if 0
        g_message ("obj_path=%s interface=%s method=%s destination=%s",
                   dbus_message_get_path (message),
                   dbus_message_get_interface (message),
                   dbus_message_get_member (message),
                   dbus_message_get_destination (message));
#endif

        g_return_val_if_fail (connection != NULL, DBUS_HANDLER_RESULT_NOT_YET_HANDLED);
        g_return_val_if_fail (message != NULL, DBUS_HANDLER_RESULT_NOT_YET_HANDLED);


        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
client_dbus_filter_function (DBusConnection *connection,
                             DBusMessage    *message,
                             void           *user_data)
{
        MdmChooserClient *client = MDM_CHOOSER_CLIENT (user_data);
        const char       *path;

        path = dbus_message_get_path (message);

        g_debug ("MdmChooserClient: obj_path=%s interface=%s method=%s",
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
                g_debug ("MdmChooserClient: Name owner changed?");
        } else {
                return client_dbus_handle_message (connection, message, user_data, FALSE);
        }

        return DBUS_HANDLER_RESULT_HANDLED;
}

gboolean
mdm_chooser_client_start (MdmChooserClient *client,
                          GError          **error)
{
        gboolean  ret;
        DBusError local_error;

        g_return_val_if_fail (MDM_IS_CHOOSER_CLIENT (client), FALSE);

        ret = FALSE;

        if (client->priv->address == NULL) {
                g_warning ("MDM_CHOOSER_DBUS_ADDRESS not set");
                g_set_error (error,
                             MDM_CHOOSER_CLIENT_ERROR,
                             MDM_CHOOSER_CLIENT_ERROR_GENERIC,
                             "MDM_CHOOSER_DBUS_ADDRESS not set");
                goto out;
        }

        g_debug ("MdmChooserClient: connecting to address: %s", client->priv->address);

        dbus_error_init (&local_error);
        client->priv->connection = dbus_connection_open (client->priv->address, &local_error);
        if (client->priv->connection == NULL) {
                if (dbus_error_is_set (&local_error)) {
                        g_warning ("error opening connection: %s", local_error.message);
                        g_set_error (error,
                                     MDM_CHOOSER_CLIENT_ERROR,
                                     MDM_CHOOSER_CLIENT_ERROR_GENERIC,
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

        ret = TRUE;

 out:
        return ret;
}

void
mdm_chooser_client_stop (MdmChooserClient *client)
{
        g_return_if_fail (MDM_IS_CHOOSER_CLIENT (client));

}

static void
mdm_chooser_client_set_property (GObject        *object,
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
mdm_chooser_client_get_property (GObject        *object,
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
mdm_chooser_client_constructor (GType                  type,
                                 guint                  n_construct_properties,
                                 GObjectConstructParam *construct_properties)
{
        MdmChooserClient      *chooser_client;

        chooser_client = MDM_CHOOSER_CLIENT (G_OBJECT_CLASS (mdm_chooser_client_parent_class)->constructor (type,
                                                                                                            n_construct_properties,
                                                                                                            construct_properties));

        return G_OBJECT (chooser_client);
}

static void
mdm_chooser_client_dispose (GObject *object)
{
        g_debug ("MdmChooserClient: Disposing chooser_client");

        G_OBJECT_CLASS (mdm_chooser_client_parent_class)->dispose (object);
}

static void
mdm_chooser_client_class_init (MdmChooserClientClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = mdm_chooser_client_get_property;
        object_class->set_property = mdm_chooser_client_set_property;
        object_class->constructor = mdm_chooser_client_constructor;
        object_class->dispose = mdm_chooser_client_dispose;
        object_class->finalize = mdm_chooser_client_finalize;

        g_type_class_add_private (klass, sizeof (MdmChooserClientPrivate));
}

static void
mdm_chooser_client_init (MdmChooserClient *client)
{

        client->priv = MDM_CHOOSER_CLIENT_GET_PRIVATE (client);

        client->priv->address = g_strdup (g_getenv ("MDM_CHOOSER_DBUS_ADDRESS"));
}

static void
mdm_chooser_client_finalize (GObject *object)
{
        MdmChooserClient *client;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_CHOOSER_CLIENT (object));

        client = MDM_CHOOSER_CLIENT (object);

        g_return_if_fail (client->priv != NULL);

        g_free (client->priv->address);

        G_OBJECT_CLASS (mdm_chooser_client_parent_class)->finalize (object);
}

MdmChooserClient *
mdm_chooser_client_new (void)
{
        if (client_object != NULL) {
                g_object_ref (client_object);
        } else {
                client_object = g_object_new (MDM_TYPE_CHOOSER_CLIENT, NULL);
                g_object_add_weak_pointer (client_object,
                                           (gpointer *) &client_object);
        }

        return MDM_CHOOSER_CLIENT (client_object);
}
