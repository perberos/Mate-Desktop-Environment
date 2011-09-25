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
#include "mdm-chooser-server.h"

#define MDM_CHOOSER_SERVER_DBUS_PATH      "/org/mate/DisplayManager/ChooserServer"
#define MDM_CHOOSER_SERVER_DBUS_INTERFACE "org.mate.DisplayManager.ChooserServer"

#define MDM_CHOOSER_SERVER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_CHOOSER_SERVER, MdmChooserServerPrivate))

struct MdmChooserServerPrivate
{
        char           *user_name;
        char           *group_name;
        char           *display_id;

        DBusServer     *server;
        char           *server_address;
        DBusConnection *chooser_connection;
};

enum {
        PROP_0,
        PROP_USER_NAME,
        PROP_GROUP_NAME,
        PROP_DISPLAY_ID,
};

enum {
        HOSTNAME_SELECTED,
        CONNECTED,
        DISCONNECTED,
        LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0, };

static void     mdm_chooser_server_class_init   (MdmChooserServerClass *klass);
static void     mdm_chooser_server_init         (MdmChooserServer      *chooser_server);
static void     mdm_chooser_server_finalize     (GObject               *object);

G_DEFINE_TYPE (MdmChooserServer, mdm_chooser_server, G_TYPE_OBJECT)

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

        path = g_strdup_printf ("unix:abstract=/tmp/mdm-chooser-%s", tmp);
#else
        path = g_strdup ("unix:tmpdir=/tmp");
#endif

        return path;
}

static DBusHandlerResult
handle_select_hostname (MdmChooserServer *chooser_server,
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

        g_debug ("ChooserServer: SelectHostname: %s", text);

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        if (reply != NULL) {
                dbus_message_unref (reply);
        }

        g_signal_emit (chooser_server, signals [HOSTNAME_SELECTED], 0, text);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_disconnect (MdmChooserServer *chooser_server,
                   DBusConnection   *connection,
                   DBusMessage      *message)
{
        DBusMessage *reply;

        reply = dbus_message_new_method_return (message);
        dbus_connection_send (connection, reply, NULL);
        if (reply != NULL) {
                dbus_message_unref (reply);
        }

        g_signal_emit (chooser_server, signals [DISCONNECTED], 0);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
chooser_handle_child_message (DBusConnection *connection,
                              DBusMessage    *message,
                              void           *user_data)
{
        MdmChooserServer *chooser_server = MDM_CHOOSER_SERVER (user_data);

        if (dbus_message_is_method_call (message, MDM_CHOOSER_SERVER_DBUS_INTERFACE, "SelectHostname")) {
                return handle_select_hostname (chooser_server, connection, message);
        } else if (dbus_message_is_method_call (message, MDM_CHOOSER_SERVER_DBUS_INTERFACE, "Disconnect")) {
                return handle_disconnect (chooser_server, connection, message);
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

        g_debug ("ChooserServer: Do introspect");

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
                               "  <interface name=\"org.mate.DisplayManager.ChooserServer\">\n"
                               "    <method name=\"SelectHostname\">\n"
                               "      <arg name=\"text\" direction=\"in\" type=\"s\"/>\n"
                               "    </method>\n"
                               "    <method name=\"Disconnect\">\n"
                               "    </method>\n"
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

        if (reply != NULL) {
                dbus_message_unref (reply);
        }

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
chooser_server_message_handler (DBusConnection  *connection,
                                DBusMessage     *message,
                                void            *user_data)
{
        const char *dbus_destination = dbus_message_get_destination (message);
        const char *dbus_path        = dbus_message_get_path (message);
        const char *dbus_interface   = dbus_message_get_interface (message);
        const char *dbus_member      = dbus_message_get_member (message);

        g_debug ("chooser_server_message_handler: destination=%s obj_path=%s interface=%s method=%s",
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
                return chooser_handle_child_message (connection, message, user_data);
        }

        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static void
chooser_server_unregister_handler (DBusConnection  *connection,
                                   void            *user_data)
{
        g_debug ("chooser_server_unregister_handler");
}

static DBusHandlerResult
connection_filter_function (DBusConnection *connection,
                            DBusMessage    *message,
                            void           *user_data)
{
        MdmChooserServer *chooser_server = MDM_CHOOSER_SERVER (user_data);
        const char       *dbus_path      = dbus_message_get_path (message);
        const char       *dbus_interface = dbus_message_get_interface (message);
        const char       *dbus_message   = dbus_message_get_member (message);

        g_debug ("ChooserServer: obj_path=%s interface=%s method=%s",
                 dbus_path      ? dbus_path      : "(null)",
                 dbus_interface ? dbus_interface : "(null)",
                 dbus_message   ? dbus_message   : "(null)");

        if (dbus_message_is_signal (message, DBUS_INTERFACE_LOCAL, "Disconnected")
            && strcmp (dbus_path, DBUS_PATH_LOCAL) == 0) {

                g_debug ("ChooserServer: Disconnected");

                dbus_connection_unref (connection);
                chooser_server->priv->chooser_connection = NULL;

                return DBUS_HANDLER_RESULT_HANDLED;
        }

        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static dbus_bool_t
allow_user_function (DBusConnection *connection,
                     unsigned long   uid,
                     void           *data)
{
        MdmChooserServer *chooser_server = MDM_CHOOSER_SERVER (data);
        struct passwd    *pwent;

        if (chooser_server->priv->user_name == NULL) {
                return FALSE;
        }

        mdm_get_pwent_for_name (chooser_server->priv->user_name, &pwent);
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
        MdmChooserServer *chooser_server = MDM_CHOOSER_SERVER (user_data);

        g_debug ("ChooserServer: Handing new connection");

        if (chooser_server->priv->chooser_connection == NULL) {
                DBusObjectPathVTable vtable = { &chooser_server_unregister_handler,
                                                &chooser_server_message_handler,
                                                NULL, NULL, NULL, NULL
                };

                chooser_server->priv->chooser_connection = new_connection;
                dbus_connection_ref (new_connection);
                dbus_connection_setup_with_g_main (new_connection, NULL);

                g_debug ("ChooserServer: chooser connection is %p", new_connection);

                dbus_connection_add_filter (new_connection,
                                            connection_filter_function,
                                            chooser_server,
                                            NULL);

                dbus_connection_set_unix_user_function (new_connection,
                                                        allow_user_function,
                                                        chooser_server,
                                                        NULL);

                dbus_connection_register_object_path (new_connection,
                                                      MDM_CHOOSER_SERVER_DBUS_PATH,
                                                      &vtable,
                                                      chooser_server);

                g_signal_emit (chooser_server, signals[CONNECTED], 0);

        }
}

gboolean
mdm_chooser_server_start (MdmChooserServer *chooser_server)
{
        DBusError   error;
        gboolean    ret;
        char       *address;
        const char *auth_mechanisms[] = {"EXTERNAL", NULL};

        ret = FALSE;

        g_debug ("ChooserServer: Creating D-Bus server for chooser");

        address = generate_address ();

        dbus_error_init (&error);
        chooser_server->priv->server = dbus_server_listen (address, &error);
        g_free (address);

        if (chooser_server->priv->server == NULL) {
                g_warning ("Cannot create D-BUS server for the chooser: %s", error.message);
                /* FIXME: should probably fail if we can't create the socket */
                goto out;
        }

        dbus_server_setup_with_g_main (chooser_server->priv->server, NULL);
        dbus_server_set_auth_mechanisms (chooser_server->priv->server, auth_mechanisms);
        dbus_server_set_new_connection_function (chooser_server->priv->server,
                                                 handle_connection,
                                                 chooser_server,
                                                 NULL);
        ret = TRUE;

        g_free (chooser_server->priv->server_address);
        chooser_server->priv->server_address = dbus_server_get_address (chooser_server->priv->server);

        g_debug ("ChooserServer: D-Bus server listening on %s", chooser_server->priv->server_address);

 out:

        return ret;
}

gboolean
mdm_chooser_server_stop (MdmChooserServer *chooser_server)
{
        gboolean ret;

        ret = FALSE;

        g_debug ("ChooserServer: Stopping chooser server...");

        return ret;
}

char *
mdm_chooser_server_get_address (MdmChooserServer *chooser_server)
{
        return g_strdup (chooser_server->priv->server_address);
}

static void
_mdm_chooser_server_set_display_id (MdmChooserServer *chooser_server,
                                    const char       *display_id)
{
        g_free (chooser_server->priv->display_id);
        chooser_server->priv->display_id = g_strdup (display_id);
}

static void
_mdm_chooser_server_set_user_name (MdmChooserServer *chooser_server,
                                  const char *name)
{
        g_free (chooser_server->priv->user_name);
        chooser_server->priv->user_name = g_strdup (name);
}

static void
_mdm_chooser_server_set_group_name (MdmChooserServer *chooser_server,
                                    const char *name)
{
        g_free (chooser_server->priv->group_name);
        chooser_server->priv->group_name = g_strdup (name);
}

static void
mdm_chooser_server_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
        MdmChooserServer *self;

        self = MDM_CHOOSER_SERVER (object);

        switch (prop_id) {
        case PROP_DISPLAY_ID:
                _mdm_chooser_server_set_display_id (self, g_value_get_string (value));
                break;
        case PROP_USER_NAME:
                _mdm_chooser_server_set_user_name (self, g_value_get_string (value));
                break;
        case PROP_GROUP_NAME:
                _mdm_chooser_server_set_group_name (self, g_value_get_string (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
mdm_chooser_server_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
        MdmChooserServer *self;

        self = MDM_CHOOSER_SERVER (object);

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
mdm_chooser_server_constructor (GType                  type,
                               guint                  n_construct_properties,
                               GObjectConstructParam *construct_properties)
{
        MdmChooserServer      *chooser_server;

        chooser_server = MDM_CHOOSER_SERVER (G_OBJECT_CLASS (mdm_chooser_server_parent_class)->constructor (type,
                                                                                       n_construct_properties,
                                                                                       construct_properties));

        return G_OBJECT (chooser_server);
}

static void
mdm_chooser_server_class_init (MdmChooserServerClass *klass)
{
        GObjectClass    *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = mdm_chooser_server_get_property;
        object_class->set_property = mdm_chooser_server_set_property;
        object_class->constructor = mdm_chooser_server_constructor;
        object_class->finalize = mdm_chooser_server_finalize;

        g_type_class_add_private (klass, sizeof (MdmChooserServerPrivate));

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
        signals [HOSTNAME_SELECTED] =
                g_signal_new ("hostname-selected",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmChooserServerClass, hostname_selected),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_STRING);
        signals [CONNECTED] =
                g_signal_new ("connected",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmChooserServerClass, connected),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
        signals [DISCONNECTED] =
                g_signal_new ("disconnected",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmChooserServerClass, disconnected),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
}

static void
mdm_chooser_server_init (MdmChooserServer *chooser_server)
{

        chooser_server->priv = MDM_CHOOSER_SERVER_GET_PRIVATE (chooser_server);
}

static void
mdm_chooser_server_finalize (GObject *object)
{
        MdmChooserServer *chooser_server;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_CHOOSER_SERVER (object));

        chooser_server = MDM_CHOOSER_SERVER (object);

        g_return_if_fail (chooser_server->priv != NULL);

        mdm_chooser_server_stop (chooser_server);

        G_OBJECT_CLASS (mdm_chooser_server_parent_class)->finalize (object);
}

MdmChooserServer *
mdm_chooser_server_new (const char *display_id)
{
        GObject *object;

        object = g_object_new (MDM_TYPE_CHOOSER_SERVER,
                               "display-id", display_id,
                               NULL);

        return MDM_CHOOSER_SERVER (object);
}
