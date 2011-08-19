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
#include <pwd.h>
#include <grp.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <glib-object.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <X11/Xlib.h> /* for Display */

#include "gdm-common.h"

#include "gdm-product-slave.h"
#include "gdm-product-slave-glue.h"

#include "gdm-server.h"
#include "gdm-session-direct.h"

extern char **environ;

#define GDM_PRODUCT_SLAVE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GDM_TYPE_PRODUCT_SLAVE, GdmProductSlavePrivate))

#define GDM_DBUS_NAME                      "org.mate.DisplayManager"
#define GDM_DBUS_PRODUCT_DISPLAY_INTERFACE "org.mate.DisplayManager.ProductDisplay"

#define RELAY_SERVER_DBUS_PATH      "/org/mate/DisplayManager/SessionRelay"
#define RELAY_SERVER_DBUS_INTERFACE "org.mate.DisplayManager.SessionRelay"

#define MAX_CONNECT_ATTEMPTS 10

struct GdmProductSlavePrivate
{
        char             *id;
        GPid              pid;

        char             *relay_address;

        GPid              server_pid;
        Display          *server_display;
        guint             connection_attempts;

        GdmServer        *server;
        GdmSessionDirect *session;
        DBusConnection   *session_relay_connection;

        DBusGProxy       *product_display_proxy;
        DBusGConnection  *connection;
};

enum {
        PROP_0,
        PROP_DISPLAY_ID,
};

static void     gdm_product_slave_class_init    (GdmProductSlaveClass *klass);
static void     gdm_product_slave_init          (GdmProductSlave      *product_slave);
static void     gdm_product_slave_finalize      (GObject             *object);

G_DEFINE_TYPE (GdmProductSlave, gdm_product_slave, GDM_TYPE_SLAVE)

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

        g_debug ("GdmProductSlave: Calling %s", method);
        message = dbus_message_new_method_call (NULL,
                                                RELAY_SERVER_DBUS_PATH,
                                                RELAY_SERVER_DBUS_INTERFACE,
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

        g_debug ("GdmProductSlave: Calling %s", method);
        message = dbus_message_new_method_call (NULL,
                                                RELAY_SERVER_DBUS_PATH,
                                                RELAY_SERVER_DBUS_INTERFACE,
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


static gboolean
send_dbus_int_method (DBusConnection *connection,
                      const char     *method,
                      int             payload)
{
        DBusError       error;
        DBusMessage    *message;
        DBusMessage    *reply;
        DBusMessageIter iter;

        g_debug ("GdmSessionWorker: Calling %s", method);
        message = dbus_message_new_method_call (NULL,
                                                RELAY_SERVER_DBUS_PATH,
                                                RELAY_SERVER_DBUS_INTERFACE,
                                                method);
        if (message == NULL) {
                g_warning ("Couldn't allocate the D-Bus message");
                return FALSE;
        }

        dbus_message_iter_init_append (message, &iter);
        dbus_message_iter_append_basic (&iter,
                                        DBUS_TYPE_INT32,
                                        &payload);

        dbus_error_init (&error);
        reply = dbus_connection_send_with_reply_and_block (connection,
                                                           message,
                                                           -1,
                                                           &error);
        dbus_message_unref (message);
        if (reply != NULL) {
                dbus_message_unref (reply);
        }
        dbus_connection_flush (connection);

        if (dbus_error_is_set (&error)) {
                g_debug ("%s %s raised: %s\n",
                         method,
                         error.name,
                         error.message);
                return FALSE;
        }

        return TRUE;
}

static void
relay_session_started (GdmProductSlave *slave,
                       int              pid)
{
        send_dbus_int_method (slave->priv->session_relay_connection,
                              "SessionStarted",
                              pid);
}

static void
relay_session_conversation_started (GdmProductSlave *slave)
{
        send_dbus_void_method (slave->priv->session_relay_connection,
                               "ConversationStarted");
}

static void
on_session_conversation_started (GdmSession      *session,
                                 GdmProductSlave *slave)
{
        g_debug ("GdmProductSlave: session conversation started");

        relay_session_conversation_started (slave);
}

static void
disconnect_relay (GdmProductSlave *slave)
{
        /* drop the connection */

        dbus_connection_close (slave->priv->session_relay_connection);
        slave->priv->session_relay_connection = NULL;
}

static void
on_session_started (GdmSession      *session,
                    int              pid,
                    GdmProductSlave *slave)
{
        g_debug ("GdmProductSlave: session started");

        relay_session_started (slave, pid);

        disconnect_relay (slave);
}

static void
on_session_exited (GdmSession      *session,
                   int              exit_code,
                   GdmProductSlave *slave)
{
        g_debug ("GdmProductSlave: session exited with code %d", exit_code);

        gdm_slave_stopped (GDM_SLAVE (slave));
}

static void
on_session_died (GdmSession      *session,
                 int              signal_number,
                 GdmProductSlave *slave)
{
        g_debug ("GdmProductSlave: session died with signal %d, (%s)",
                 signal_number,
                 g_strsignal (signal_number));

        gdm_slave_stopped (GDM_SLAVE (slave));
}

static void
setup_server (GdmProductSlave *slave)
{
        /* Set the busy cursor */
        gdm_slave_set_busy_cursor (GDM_SLAVE (slave));
}

static gboolean
add_user_authorization (GdmProductSlave *slave,
                        char           **filename)
{
        char    *username;
        gboolean ret;

        username = gdm_session_direct_get_username (slave->priv->session);

        ret = gdm_slave_add_user_authorization (GDM_SLAVE (slave),
                                                username,
                                                filename);
        g_debug ("GdmProductSlave: Adding user authorization for %s: %s", username, *filename);

        g_free (username);

        return ret;
}

static gboolean
setup_session (GdmProductSlave *slave)
{
        char *auth_file;
        char *display_device;

        auth_file = NULL;
        add_user_authorization (slave, &auth_file);

        g_assert (auth_file != NULL);

        display_device = NULL;
        if (slave->priv->server != NULL) {
                display_device = gdm_server_get_display_device (slave->priv->server);
        }

        g_object_set (slave->priv->session,
                      "display-device", display_device,
                      "user-x11-authority-file", auth_file,
                      NULL);

        g_free (display_device);
        g_free (auth_file);

        gdm_session_start_session (GDM_SESSION (slave->priv->session));

        return TRUE;
}

static gboolean
idle_connect_to_display (GdmProductSlave *slave)
{
        gboolean res;

        slave->priv->connection_attempts++;

        res = gdm_slave_connect_to_x11_display (GDM_SLAVE (slave));
        if (res) {
                /* FIXME: handle wait-for-go */

                setup_server (slave);
                setup_session (slave);
        } else {
                if (slave->priv->connection_attempts >= MAX_CONNECT_ATTEMPTS) {
                        g_warning ("Unable to connect to display after %d tries - bailing out", slave->priv->connection_attempts);
                        exit (1);
                }
                return TRUE;
        }

        return FALSE;
}

static void
on_server_ready (GdmServer       *server,
                 GdmProductSlave *slave)
{
        g_timeout_add (500, (GSourceFunc)idle_connect_to_display, slave);
}

static void
on_server_exited (GdmServer       *server,
                  int              exit_code,
                  GdmProductSlave *slave)
{
        g_debug ("GdmProductSlave: server exited with code %d\n", exit_code);

        gdm_slave_stopped (GDM_SLAVE (slave));
}

static void
on_server_died (GdmServer       *server,
                int              signal_number,
                GdmProductSlave *slave)
{
        g_debug ("GdmProductSlave: server died with signal %d, (%s)",
                 signal_number,
                 g_strsignal (signal_number));

        gdm_slave_stopped (GDM_SLAVE (slave));
}

static gboolean
gdm_product_slave_create_server (GdmProductSlave *slave)
{
        char    *display_name;
        char    *auth_file;
        gboolean display_is_local;

        g_object_get (slave,
                      "display-is-local", &display_is_local,
                      "display-name", &display_name,
                      "display-x11-authority-file", &auth_file,
                      NULL);

        /* if this is local display start a server if one doesn't
         * exist */
        if (display_is_local) {
                gboolean res;

                slave->priv->server = gdm_server_new (display_name, auth_file);
                g_signal_connect (slave->priv->server,
                                  "exited",
                                  G_CALLBACK (on_server_exited),
                                  slave);
                g_signal_connect (slave->priv->server,
                                  "died",
                                  G_CALLBACK (on_server_died),
                                  slave);
                g_signal_connect (slave->priv->server,
                                  "ready",
                                  G_CALLBACK (on_server_ready),
                                  slave);

                res = gdm_server_start (slave->priv->server);
                if (! res) {
                        g_warning (_("Could not start the X "
                                     "server (your graphical environment) "
                                     "due to an internal error. "
                                     "Please contact your system administrator "
                                     "or check your syslog to diagnose. "
                                     "In the meantime this display will be "
                                     "disabled.  Please restart GDM when "
                                     "the problem is corrected."));
                        exit (1);
                }

                g_debug ("GdmProductSlave: Started X server");
        } else {
                g_timeout_add (500, (GSourceFunc)idle_connect_to_display, slave);
        }

        g_free (display_name);
        g_free (auth_file);

        return TRUE;
}

static void
on_session_setup_complete (GdmSession      *session,
                           GdmProductSlave *slave)
{
        send_dbus_void_method (slave->priv->session_relay_connection,
                               "SetupComplete");
}

static void
on_session_setup_failed (GdmSession      *session,
                         const char      *message,
                         GdmProductSlave *slave)
{
        send_dbus_string_method (slave->priv->session_relay_connection,
                                 "SetupFailed",
                                 message);
}

static void
on_session_reset_complete (GdmSession      *session,
                           GdmProductSlave *slave)
{
        send_dbus_void_method (slave->priv->session_relay_connection,
                               "ResetComplete");
}

static void
on_session_reset_failed (GdmSession      *session,
                         const char      *message,
                         GdmProductSlave *slave)
{
        send_dbus_string_method (slave->priv->session_relay_connection,
                                 "ResetFailed",
                                 message);
}

static void
on_session_authenticated (GdmSession      *session,
                          GdmProductSlave *slave)
{
        send_dbus_void_method (slave->priv->session_relay_connection,
                               "Authenticated");
}

static void
on_session_authentication_failed (GdmSession      *session,
                                  const char      *message,
                                  GdmProductSlave *slave)
{
        send_dbus_string_method (slave->priv->session_relay_connection,
                                 "AuthenticationFailed",
                                 message);
}

static void
on_session_authorized (GdmSession      *session,
                       GdmProductSlave *slave)
{
        send_dbus_void_method (slave->priv->session_relay_connection,
                               "Authorized");
}

static void
on_session_authorization_failed (GdmSession      *session,
                                 const char      *message,
                                 GdmProductSlave *slave)
{
        send_dbus_string_method (slave->priv->session_relay_connection,
                                 "AuthorizationFailed",
                                 message);
}

static void
on_session_accredited (GdmSession      *session,
                       GdmProductSlave *slave)
{
        send_dbus_void_method (slave->priv->session_relay_connection,
                               "Accredited");
}

static void
on_session_accreditation_failed (GdmSession      *session,
                                 const char      *message,
                                 GdmProductSlave *slave)
{
        send_dbus_string_method (slave->priv->session_relay_connection,
                                 "AccreditationFailed",
                                 message);
}

static void
on_session_opened (GdmSession      *session,
                   GdmProductSlave *slave)
{
        send_dbus_void_method (slave->priv->session_relay_connection,
                               "SessionOpened");
}

static void
on_session_open_failed (GdmSession      *session,
                        const char      *message,
                        GdmProductSlave *slave)
{
        send_dbus_string_method (slave->priv->session_relay_connection,
                                 "SessionOpenFailed",
                                 message);
}

static void
on_session_info (GdmSession      *session,
                 const char      *text,
                 GdmProductSlave *slave)
{
        send_dbus_string_method (slave->priv->session_relay_connection,
                                 "Info",
                                 text);
}

static void
on_session_problem (GdmSession      *session,
                    const char      *text,
                    GdmProductSlave *slave)
{
        send_dbus_string_method (slave->priv->session_relay_connection,
                                 "Problem",
                                 text);
}

static void
on_session_info_query (GdmSession      *session,
                       const char      *text,
                       GdmProductSlave *slave)
{
        send_dbus_string_method (slave->priv->session_relay_connection,
                                 "InfoQuery",
                                 text);
}

static void
on_session_secret_info_query (GdmSession      *session,
                              const char      *text,
                              GdmProductSlave *slave)
{
        send_dbus_string_method (slave->priv->session_relay_connection,
                                 "SecretInfoQuery",
                                 text);
}

static void
on_relay_setup (GdmProductSlave *slave,
                DBusMessage     *message)
{
        DBusError   error;
        const char *service_name;
        dbus_bool_t res;

        service_name = NULL;

        dbus_error_init (&error);
        res = dbus_message_get_args (message,
                                     &error,
                                     DBUS_TYPE_STRING, &service_name,
                                     DBUS_TYPE_INVALID);
        if (res) {
                g_debug ("GdmProductSlave: Relay Setup");
                gdm_session_setup (GDM_SESSION (slave->priv->session),
                                   service_name);
        } else {
                g_warning ("Unable to get arguments: %s", error.message);
                dbus_error_free (&error);
        }
}

static void
on_relay_setup_for_user (GdmProductSlave *slave,
                         DBusMessage     *message)
{
        DBusError   error;
        const char *service_name;
        const char *username;
        dbus_bool_t res;

        username = NULL;
        service_name = NULL;

        dbus_error_init (&error);
        res = dbus_message_get_args (message,
                                     &error,
                                     DBUS_TYPE_STRING, &service_name,
                                     DBUS_TYPE_STRING, &username,
                                     DBUS_TYPE_INVALID);
        if (res) {
                g_debug ("GdmProductSlave: Relay SetupForUser");
                gdm_session_setup_for_user (GDM_SESSION (slave->priv->session),
                                            service_name,
                                            username);
        } else {
                g_warning ("Unable to get arguments: %s", error.message);
                dbus_error_free (&error);
        }
}

static void
on_relay_authenticate (GdmProductSlave *slave,
                       DBusMessage     *message)
{
        g_debug ("GdmProductSlave: Relay Authenticate");

        gdm_session_authenticate (GDM_SESSION (slave->priv->session));
}

static void
on_relay_authorize (GdmProductSlave *slave,
                    DBusMessage     *message)
{
        g_debug ("GdmProductSlave: Relay Authorize");

        gdm_session_authorize (GDM_SESSION (slave->priv->session));
}

static void
on_relay_establish_credentials (GdmProductSlave *slave,
                                DBusMessage     *message)
{
        g_debug ("GdmProductSlave: Relay EstablishCredentials");

        gdm_session_accredit (GDM_SESSION (slave->priv->session), GDM_SESSION_CRED_ESTABLISH);
}

static void
on_relay_refresh_credentials (GdmProductSlave *slave,
                              DBusMessage     *message)
{
        g_debug ("GdmProductSlave: Relay RefreshCredentials");

        gdm_session_accredit (GDM_SESSION (slave->priv->session), GDM_SESSION_CRED_REFRESH);
}

static void
on_relay_answer_query (GdmProductSlave *slave,
                       DBusMessage     *message)
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
                g_debug ("GdmProductSlave: Relay AnswerQuery");
                gdm_session_answer_query (GDM_SESSION (slave->priv->session), text);
        } else {
                g_warning ("Unable to get arguments: %s", error.message);
                dbus_error_free (&error);
        }
}

static void
on_relay_session_selected (GdmProductSlave *slave,
                           DBusMessage     *message)
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
                g_debug ("GdmProductSlave: Session selected %s", text);
                gdm_session_select_session (GDM_SESSION (slave->priv->session), text);
        } else {
                g_warning ("Unable to get arguments: %s", error.message);
                dbus_error_free (&error);
        }
}

static void
on_relay_language_selected (GdmProductSlave *slave,
                            DBusMessage     *message)
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
                g_debug ("GdmProductSlave: Language selected %s", text);
                gdm_session_select_language (GDM_SESSION (slave->priv->session), text);
        } else {
                g_warning ("Unable to get arguments: %s", error.message);
                dbus_error_free (&error);
        }
}

static void
on_relay_layout_selected (GdmProductSlave *slave,
                          DBusMessage     *message)
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
                g_debug ("GdmProductSlave: Layout selected %s", text);
                gdm_session_select_layout (GDM_SESSION (slave->priv->session), text);
        } else {
                g_warning ("Unable to get arguments: %s", error.message);
                dbus_error_free (&error);
        }
}

static void
on_relay_user_selected (GdmProductSlave *slave,
                        DBusMessage     *message)
{
        g_debug ("GdmProductSlave: Greeter user selected");
}

static void
on_relay_start_conversation (GdmProductSlave *slave,
                             DBusMessage     *message)
{
        gdm_session_start_conversation (GDM_SESSION (slave->priv->session));
}

static void
on_relay_open_session (GdmProductSlave *slave,
                        DBusMessage     *message)
{
        gdm_session_open_session (GDM_SESSION (slave->priv->session));
}

static void
on_relay_start_session (GdmProductSlave *slave,
                        DBusMessage     *message)
{
        gdm_product_slave_create_server (slave);
}

static void
create_new_session (GdmProductSlave *slave)
{
        gboolean       display_is_local;
        char          *display_id;
        char          *display_name;
        char          *display_hostname;
        char          *display_device;
        char          *display_x11_authority_file;

        g_debug ("GdmProductSlave: Creating new session");

        g_object_get (slave,
                      "display-id", &display_id,
                      "display-name", &display_name,
                      "display-hostname", &display_hostname,
                      "display-is-local", &display_is_local,
                      "display-x11-authority-file", &display_x11_authority_file,
                      NULL);

        /* FIXME: we don't yet have a display device! */
        display_device = g_strdup ("");

        slave->priv->session = gdm_session_direct_new (display_id,
                                                       display_name,
                                                       display_hostname,
                                                       display_device,
                                                       display_x11_authority_file,
                                                       display_is_local);
        g_free (display_id);
        g_free (display_name);
        g_free (display_hostname);
        g_free (display_x11_authority_file);
        g_free (display_device);

        g_signal_connect (slave->priv->session,
                          "conversation-started",
                          G_CALLBACK (on_session_conversation_started),
                          slave);
        g_signal_connect (slave->priv->session,
                          "setup-complete",
                          G_CALLBACK (on_session_setup_complete),
                          slave);
        g_signal_connect (slave->priv->session,
                          "setup-failed",
                          G_CALLBACK (on_session_setup_failed),
                          slave);
        g_signal_connect (slave->priv->session,
                          "reset-complete",
                          G_CALLBACK (on_session_reset_complete),
                          slave);
        g_signal_connect (slave->priv->session,
                          "reset-failed",
                          G_CALLBACK (on_session_reset_failed),
                          slave);
        g_signal_connect (slave->priv->session,
                          "authenticated",
                          G_CALLBACK (on_session_authenticated),
                          slave);
        g_signal_connect (slave->priv->session,
                          "authentication-failed",
                          G_CALLBACK (on_session_authentication_failed),
                          slave);
        g_signal_connect (slave->priv->session,
                          "authorized",
                          G_CALLBACK (on_session_authorized),
                          slave);
        g_signal_connect (slave->priv->session,
                          "authorization-failed",
                          G_CALLBACK (on_session_authorization_failed),
                          slave);
        g_signal_connect (slave->priv->session,
                          "accredited",
                          G_CALLBACK (on_session_accredited),
                          slave);
        g_signal_connect (slave->priv->session,
                          "accreditation-failed",
                          G_CALLBACK (on_session_accreditation_failed),
                          slave);
        g_signal_connect (slave->priv->session,
                          "session-opened",
                          G_CALLBACK (on_session_opened),
                          slave);
        g_signal_connect (slave->priv->session,
                          "session-open-failed",
                          G_CALLBACK (on_session_open_failed),
                          slave);
        g_signal_connect (slave->priv->session,
                          "info",
                          G_CALLBACK (on_session_info),
                          slave);
        g_signal_connect (slave->priv->session,
                          "problem",
                          G_CALLBACK (on_session_problem),
                          slave);
        g_signal_connect (slave->priv->session,
                          "info-query",
                          G_CALLBACK (on_session_info_query),
                          slave);
        g_signal_connect (slave->priv->session,
                          "secret-info-query",
                          G_CALLBACK (on_session_secret_info_query),
                          slave);

        g_signal_connect (slave->priv->session,
                          "session-started",
                          G_CALLBACK (on_session_started),
                          slave);
        g_signal_connect (slave->priv->session,
                          "session-exited",
                          G_CALLBACK (on_session_exited),
                          slave);
        g_signal_connect (slave->priv->session,
                          "session-died",
                          G_CALLBACK (on_session_died),
                          slave);
}

static void
on_relay_cancelled (GdmProductSlave *slave,
                    DBusMessage     *message)
{
        g_debug ("GdmProductSlave: Relay cancelled");

        if (slave->priv->session != NULL) {
                gdm_session_close (GDM_SESSION (slave->priv->session));
                g_object_unref (slave->priv->session);
        }

        create_new_session (slave);
}

static void
get_relay_address (GdmProductSlave *slave)
{
        GError  *error;
        char    *text;
        gboolean res;

        text = NULL;
        error = NULL;
        res = dbus_g_proxy_call (slave->priv->product_display_proxy,
                                 "GetRelayAddress",
                                 &error,
                                 G_TYPE_INVALID,
                                 G_TYPE_STRING, &text,
                                 G_TYPE_INVALID);
        if (! res) {
                g_warning ("Unable to get relay address: %s", error->message);
                g_error_free (error);
        } else {
                g_free (slave->priv->relay_address);
                slave->priv->relay_address = g_strdup (text);
                g_debug ("GdmProductSlave: Got relay address: %s", slave->priv->relay_address);
        }

        g_free (text);
}

static DBusHandlerResult
relay_dbus_handle_message (DBusConnection *connection,
                           DBusMessage    *message,
                           void           *user_data,
                           dbus_bool_t     local_interface)
{
        GdmProductSlave *slave = GDM_PRODUCT_SLAVE (user_data);

#if 0
        g_message ("obj_path=%s interface=%s method=%s destination=%s",
                   dbus_message_get_path (message),
                   dbus_message_get_interface (message),
                   dbus_message_get_member (message),
                   dbus_message_get_destination (message));
#endif

        g_return_val_if_fail (connection != NULL, DBUS_HANDLER_RESULT_NOT_YET_HANDLED);
        g_return_val_if_fail (message != NULL, DBUS_HANDLER_RESULT_NOT_YET_HANDLED);

        if (dbus_message_is_signal (message, RELAY_SERVER_DBUS_INTERFACE, "Setup")) {
                on_relay_setup (slave, message);
        } else if (dbus_message_is_signal (message, RELAY_SERVER_DBUS_INTERFACE, "SetupForUser")) {
                on_relay_setup_for_user (slave, message);
        } else if (dbus_message_is_signal (message, RELAY_SERVER_DBUS_INTERFACE, "Authenticate")) {
                on_relay_authenticate (slave, message);
        } else if (dbus_message_is_signal (message, RELAY_SERVER_DBUS_INTERFACE, "Authorize")) {
                on_relay_authorize (slave, message);
        } else if (dbus_message_is_signal (message, RELAY_SERVER_DBUS_INTERFACE, "EstablishCredentials")) {
                on_relay_establish_credentials (slave, message);
        } else if (dbus_message_is_signal (message, RELAY_SERVER_DBUS_INTERFACE, "RefreshCredentials")) {
                on_relay_refresh_credentials (slave, message);
        } else if (dbus_message_is_signal (message, RELAY_SERVER_DBUS_INTERFACE, "AnswerQuery")) {
                on_relay_answer_query (slave, message);
        } else if (dbus_message_is_signal (message, RELAY_SERVER_DBUS_INTERFACE, "SessionSelected")) {
                on_relay_session_selected (slave, message);
        } else if (dbus_message_is_signal (message, RELAY_SERVER_DBUS_INTERFACE, "LanguageSelected")) {
                on_relay_language_selected (slave, message);
        } else if (dbus_message_is_signal (message, RELAY_SERVER_DBUS_INTERFACE, "LayoutSelected")) {
                on_relay_layout_selected (slave, message);
        } else if (dbus_message_is_signal (message, RELAY_SERVER_DBUS_INTERFACE, "UserSelected")) {
                on_relay_user_selected (slave, message);
        } else if (dbus_message_is_signal (message, RELAY_SERVER_DBUS_INTERFACE, "OpenSession")) {
                on_relay_open_session (slave, message);
        } else if (dbus_message_is_signal (message, RELAY_SERVER_DBUS_INTERFACE, "StartSession")) {
                on_relay_start_session (slave, message);
        } else if (dbus_message_is_signal (message, RELAY_SERVER_DBUS_INTERFACE, "StartConversation")) {
                on_relay_start_conversation (slave, message);
        } else if (dbus_message_is_signal (message, RELAY_SERVER_DBUS_INTERFACE, "Cancelled")) {
                on_relay_cancelled (slave, message);
        } else {
                return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
relay_dbus_filter_function (DBusConnection *connection,
                            DBusMessage    *message,
                            void           *user_data)
{
        GdmProductSlave *slave = GDM_PRODUCT_SLAVE (user_data);
        const char      *path;

        path = dbus_message_get_path (message);

        g_debug ("GdmProductSlave: obj_path=%s interface=%s method=%s",
                 dbus_message_get_path (message),
                 dbus_message_get_interface (message),
                 dbus_message_get_member (message));

        if (dbus_message_is_signal (message, DBUS_INTERFACE_LOCAL, "Disconnected")
            && strcmp (path, DBUS_PATH_LOCAL) == 0) {

                g_debug ("GdmProductSlave: Got disconnected from the server");

                dbus_connection_unref (connection);
                slave->priv->connection = NULL;

        } else if (dbus_message_is_signal (message,
                                           DBUS_INTERFACE_DBUS,
                                           "NameOwnerChanged")) {
                g_debug ("GdmProductSlave: Name owner changed?");
        } else {
                return relay_dbus_handle_message (connection, message, user_data, FALSE);
        }

        return DBUS_HANDLER_RESULT_HANDLED;
}

static gboolean
connect_to_session_relay (GdmProductSlave *slave)
{
        DBusError       error;

        get_relay_address (slave);

        g_debug ("GdmProductSlave: connecting to session relay address: %s", slave->priv->relay_address);

        dbus_error_init (&error);
        slave->priv->session_relay_connection = dbus_connection_open_private (slave->priv->relay_address, &error);
        if (slave->priv->session_relay_connection == NULL) {
                if (dbus_error_is_set (&error)) {
                        g_warning ("error opening connection: %s", error.message);
                        dbus_error_free (&error);
                } else {
                        g_warning ("Unable to open connection");
                }
                exit (1);
        }

        dbus_connection_setup_with_g_main (slave->priv->session_relay_connection, NULL);
        dbus_connection_set_exit_on_disconnect (slave->priv->session_relay_connection, FALSE);

        dbus_connection_add_filter (slave->priv->session_relay_connection,
                                    relay_dbus_filter_function,
                                    slave,
                                    NULL);

        return TRUE;
}

static gboolean
gdm_product_slave_start (GdmSlave *slave)
{
        gboolean ret;
        GError  *error;
        char    *display_id;

        ret = FALSE;

        GDM_SLAVE_CLASS (gdm_product_slave_parent_class)->start (slave);

        g_object_get (slave,
                      "display-id", &display_id,
                      NULL);

        error = NULL;
        GDM_PRODUCT_SLAVE (slave)->priv->connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
        if (GDM_PRODUCT_SLAVE (slave)->priv->connection == NULL) {
                if (error != NULL) {
                        g_critical ("error getting system bus: %s", error->message);
                        g_error_free (error);
                }
                exit (1);
        }

        error = NULL;
        GDM_PRODUCT_SLAVE (slave)->priv->product_display_proxy = dbus_g_proxy_new_for_name_owner (GDM_PRODUCT_SLAVE (slave)->priv->connection,
                                                                                                  GDM_DBUS_NAME,
                                                                                                  display_id,
                                                                                                  GDM_DBUS_PRODUCT_DISPLAY_INTERFACE,
                                                                                                  &error);
        if (GDM_PRODUCT_SLAVE (slave)->priv->product_display_proxy == NULL) {
                if (error != NULL) {
                        g_warning ("Failed to create display proxy %s: %s", display_id, error->message);
                        g_error_free (error);
                } else {
                        g_warning ("Unable to create display proxy");
                }
                goto out;
        }

        create_new_session (GDM_PRODUCT_SLAVE (slave));

        connect_to_session_relay (GDM_PRODUCT_SLAVE (slave));

        ret = TRUE;

 out:
        g_free (display_id);

        return ret;
}

static gboolean
gdm_product_slave_stop (GdmSlave *slave)
{
        g_debug ("GdmProductSlave: Stopping product_slave");

        GDM_SLAVE_CLASS (gdm_product_slave_parent_class)->stop (slave);

        if (GDM_PRODUCT_SLAVE (slave)->priv->session != NULL) {
                gdm_session_close (GDM_SESSION (GDM_PRODUCT_SLAVE (slave)->priv->session));
                g_object_unref (GDM_PRODUCT_SLAVE (slave)->priv->session);
                GDM_PRODUCT_SLAVE (slave)->priv->session = NULL;
        }

        if (GDM_PRODUCT_SLAVE (slave)->priv->server != NULL) {
                gdm_server_stop (GDM_PRODUCT_SLAVE (slave)->priv->server);
                g_object_unref (GDM_PRODUCT_SLAVE (slave)->priv->server);
                GDM_PRODUCT_SLAVE (slave)->priv->server = NULL;
        }

        if (GDM_PRODUCT_SLAVE (slave)->priv->product_display_proxy != NULL) {
                g_object_unref (GDM_PRODUCT_SLAVE (slave)->priv->product_display_proxy);
        }

        return TRUE;
}

static void
gdm_product_slave_set_property (GObject      *object,
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
gdm_product_slave_get_property (GObject    *object,
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
gdm_product_slave_constructor (GType                  type,
                               guint                  n_construct_properties,
                               GObjectConstructParam *construct_properties)
{
        GdmProductSlave      *product_slave;

        product_slave = GDM_PRODUCT_SLAVE (G_OBJECT_CLASS (gdm_product_slave_parent_class)->constructor (type,
                                                                                                         n_construct_properties,
                                                                                                         construct_properties));
        return G_OBJECT (product_slave);
}

static void
gdm_product_slave_class_init (GdmProductSlaveClass *klass)
{
        GObjectClass  *object_class = G_OBJECT_CLASS (klass);
        GdmSlaveClass *slave_class = GDM_SLAVE_CLASS (klass);

        object_class->get_property = gdm_product_slave_get_property;
        object_class->set_property = gdm_product_slave_set_property;
        object_class->constructor = gdm_product_slave_constructor;
        object_class->finalize = gdm_product_slave_finalize;

        slave_class->start = gdm_product_slave_start;
        slave_class->stop = gdm_product_slave_stop;

        g_type_class_add_private (klass, sizeof (GdmProductSlavePrivate));

        dbus_g_object_type_install_info (GDM_TYPE_PRODUCT_SLAVE, &dbus_glib_gdm_product_slave_object_info);
}

static void
gdm_product_slave_init (GdmProductSlave *slave)
{
        slave->priv = GDM_PRODUCT_SLAVE_GET_PRIVATE (slave);
}

static void
gdm_product_slave_finalize (GObject *object)
{
        GdmProductSlave *slave;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GDM_IS_PRODUCT_SLAVE (object));

        slave = GDM_PRODUCT_SLAVE (object);

        g_return_if_fail (slave->priv != NULL);

        gdm_product_slave_stop (GDM_SLAVE (slave));

        g_free (slave->priv->relay_address);

        G_OBJECT_CLASS (gdm_product_slave_parent_class)->finalize (object);
}

GdmSlave *
gdm_product_slave_new (const char *id)
{
        GObject *object;

        object = g_object_new (GDM_TYPE_PRODUCT_SLAVE,
                               "display-id", id,
                               NULL);

        return GDM_SLAVE (object);
}
