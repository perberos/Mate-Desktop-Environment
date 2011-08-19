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

#include "gdm-factory-slave.h"
#include "gdm-factory-slave-glue.h"

#include "gdm-server.h"
#include "gdm-greeter-session.h"
#include "gdm-greeter-server.h"

#include "gdm-session-relay.h"

extern char **environ;

#define GDM_FACTORY_SLAVE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GDM_TYPE_FACTORY_SLAVE, GdmFactorySlavePrivate))

#define GDM_DBUS_NAME                            "org.mate.DisplayManager"
#define GDM_DBUS_LOCAL_DISPLAY_FACTORY_PATH      "/org/mate/DisplayManager/LocalDisplayFactory"
#define GDM_DBUS_LOCAL_DISPLAY_FACTORY_INTERFACE "org.mate.DisplayManager.LocalDisplayFactory"

#define MAX_CONNECT_ATTEMPTS 10

struct GdmFactorySlavePrivate
{
        char              *id;
        GPid               pid;
        guint              greeter_reset_id;

        GPid               server_pid;
        Display           *server_display;
        guint              connection_attempts;

        GdmServer         *server;
        GdmSessionRelay   *session;
        GdmGreeterServer  *greeter_server;
        GdmGreeterSession *greeter;
        DBusGProxy        *factory_proxy;
        DBusGConnection   *connection;
};

enum {
        PROP_0,
};

static void     gdm_factory_slave_class_init    (GdmFactorySlaveClass *klass);
static void     gdm_factory_slave_init          (GdmFactorySlave      *factory_slave);
static void     gdm_factory_slave_finalize      (GObject             *object);

G_DEFINE_TYPE (GdmFactorySlave, gdm_factory_slave, GDM_TYPE_SLAVE)

static gboolean
greeter_reset_timeout (GdmFactorySlave *slave)
{
        gdm_greeter_server_reset (slave->priv->greeter_server);
        slave->priv->greeter_reset_id = 0;
        return FALSE;
}

static void
queue_greeter_reset (GdmFactorySlave *slave)
{
        if (slave->priv->greeter_reset_id > 0) {
                return;
        }

        slave->priv->greeter_reset_id = g_timeout_add_seconds (2, (GSourceFunc)greeter_reset_timeout, slave);
}

static void
on_greeter_session_start (GdmGreeterSession *greeter,
                          GdmFactorySlave   *slave)
{
        g_debug ("GdmFactorySlave: Greeter started");
}

static void
on_greeter_session_stop (GdmGreeterSession *greeter,
                         GdmFactorySlave   *slave)
{
        g_debug ("GdmFactorySlave: Greeter stopped");
}

static void
on_greeter_session_exited (GdmGreeterSession    *greeter,
                           int                   code,
                           GdmFactorySlave      *slave)
{
        g_debug ("GdmSimpleSlave: Greeter exited: %d", code);
        gdm_slave_stopped (GDM_SLAVE (slave));
}

static void
on_greeter_session_died (GdmGreeterSession    *greeter,
                         int                   signal,
                         GdmFactorySlave      *slave)
{
        g_debug ("GdmSimpleSlave: Greeter died: %d", signal);
        gdm_slave_stopped (GDM_SLAVE (slave));
}


static void
on_session_info (GdmSession      *session,
                 const char      *text,
                 GdmFactorySlave *slave)
{
        g_debug ("GdmFactorySlave: Info: %s", text);
        gdm_greeter_server_info (slave->priv->greeter_server, text);
}

static void
on_session_problem (GdmSession      *session,
                    const char      *text,
                    GdmFactorySlave *slave)
{
        g_debug ("GdmFactorySlave: Problem: %s", text);
        gdm_greeter_server_problem (slave->priv->greeter_server, text);
}

static void
on_session_info_query (GdmSession      *session,
                       const char      *text,
                       GdmFactorySlave *slave)
{

        g_debug ("GdmFactorySlave: Info query: %s", text);
        gdm_greeter_server_info_query (slave->priv->greeter_server, text);
}

static void
on_session_secret_info_query (GdmSession      *session,
                              const char      *text,
                              GdmFactorySlave *slave)
{
        g_debug ("GdmFactorySlave: Secret info query: %s", text);
        gdm_greeter_server_secret_info_query (slave->priv->greeter_server, text);
}

static void
on_session_conversation_started (GdmSession      *session,
                                 GdmFactorySlave *slave)
{
        g_debug ("GdmFactorySlave: session conversation started");

        gdm_greeter_server_ready (slave->priv->greeter_server);
}

static void
on_session_setup_complete (GdmSession      *session,
                           GdmFactorySlave *slave)
{
        gdm_session_authenticate (session);
}

static void
on_session_setup_failed (GdmSession      *session,
                         const char      *message,
                         GdmFactorySlave *slave)
{
        gdm_greeter_server_problem (slave->priv->greeter_server, _("Unable to initialize login system"));

        queue_greeter_reset (slave);
}

static void
on_session_reset_complete (GdmSession      *session,
                           GdmFactorySlave *slave)
{
        g_debug ("GdmFactorySlave: PAM reset");
}

static void
on_session_reset_failed (GdmSession      *session,
                         const char      *message,
                         GdmFactorySlave *slave)
{
        g_critical ("Unable to reset PAM");
}

static void
on_session_authenticated (GdmSession      *session,
                          GdmFactorySlave *slave)
{
        gdm_session_authorize (session);
}

static void
on_session_authentication_failed (GdmSession      *session,
                                  const char      *message,
                                  GdmFactorySlave *slave)
{
        gdm_greeter_server_problem (slave->priv->greeter_server, _("Unable to authenticate user"));

        queue_greeter_reset (slave);
}

static void
on_session_authorized (GdmSession      *session,
                       GdmFactorySlave *slave)
{
        int flag;

        /* FIXME: check for migration? */
        flag = GDM_SESSION_CRED_ESTABLISH;

        gdm_session_accredit (session, flag);
}

static void
on_session_authorization_failed (GdmSession      *session,
                                 const char      *message,
                                 GdmFactorySlave *slave)
{
        gdm_greeter_server_problem (slave->priv->greeter_server, _("Unable to authorize user"));

        queue_greeter_reset (slave);
}

static void
on_session_accredited (GdmSession      *session,
                       GdmFactorySlave *slave)
{
        g_debug ("GdmFactorySlave:  session user verified");

        gdm_session_open_session (session);
}

static void
on_session_accreditation_failed (GdmSession      *session,
                                 const char      *message,
                                 GdmFactorySlave *slave)
{
        g_debug ("GdmFactorySlave: could not successfully authenticate user: %s",
                 message);

        gdm_greeter_server_problem (slave->priv->greeter_server, _("Unable to establish credentials"));

        queue_greeter_reset (slave);
}

static void
on_session_opened (GdmSession      *session,
                   GdmFactorySlave *slave)
{
        g_debug ("GdmFactorySlave: session opened");

        gdm_session_start_session (session);

        gdm_greeter_server_reset (slave->priv->greeter_server);
}

static void
on_session_open_failed (GdmSession      *session,
                        const char      *message,
                        GdmFactorySlave *slave)
{
        g_debug ("GdmFactorySlave: could not open session: %s", message);

        gdm_greeter_server_problem (slave->priv->greeter_server, _("Unable to open session"));

        queue_greeter_reset (slave);
}

static void
on_session_session_started (GdmSession      *session,
                            int              pid,
                            GdmFactorySlave *slave)
{
        g_debug ("GdmFactorySlave: Relay session started");

        gdm_greeter_server_reset (slave->priv->greeter_server);
}

static gboolean
create_product_display (GdmFactorySlave *slave)
{
        char    *parent_display_id;
        char    *server_address;
        char    *product_id;
        GError  *error;
        gboolean res;
        gboolean ret;

        ret = FALSE;

        g_debug ("GdmFactorySlave: Create product display");

        g_debug ("GdmFactorySlave: Connecting to local display factory");
        slave->priv->factory_proxy = dbus_g_proxy_new_for_name (slave->priv->connection,
                                                                GDM_DBUS_NAME,
                                                                GDM_DBUS_LOCAL_DISPLAY_FACTORY_PATH,
                                                                GDM_DBUS_LOCAL_DISPLAY_FACTORY_INTERFACE);
        if (slave->priv->factory_proxy == NULL) {
                g_warning ("Failed to create local display factory proxy");
                goto out;
        }

        server_address = gdm_session_relay_get_address (slave->priv->session);

        g_object_get (slave,
                      "display-id", &parent_display_id,
                      NULL);

        error = NULL;
        res = dbus_g_proxy_call (slave->priv->factory_proxy,
                                 "CreateProductDisplay",
                                 &error,
                                 DBUS_TYPE_G_OBJECT_PATH, parent_display_id,
                                 G_TYPE_STRING, server_address,
                                 G_TYPE_INVALID,
                                 DBUS_TYPE_G_OBJECT_PATH, &product_id,
                                 G_TYPE_INVALID);
        g_free (server_address);
        g_free (parent_display_id);

        if (! res) {
                if (error != NULL) {
                        g_warning ("Failed to create product display: %s", error->message);
                        g_error_free (error);
                } else {
                        g_warning ("Failed to create product display");
                }
                goto out;
        }

        ret = TRUE;

 out:
        return ret;
}

static void
on_session_relay_disconnected (GdmSessionRelay *session,
                               GdmFactorySlave *slave)
{
        g_debug ("GdmFactorySlave: Relay disconnected");

        /* FIXME: do some kind of loop detection */
        gdm_greeter_server_reset (slave->priv->greeter_server);
        create_product_display (slave);
}

static void
on_session_relay_connected (GdmSessionRelay *session,
                            GdmFactorySlave *slave)
{
        g_debug ("GdmFactorySlave: Relay Connected");

        gdm_session_start_conversation (GDM_SESSION (slave->priv->session));
}

static void
on_greeter_begin_verification (GdmGreeterServer *greeter_server,
                               GdmFactorySlave  *slave)
{
        g_debug ("GdmFactorySlave: begin verification");
        gdm_session_setup (GDM_SESSION (slave->priv->session),
                           "gdm");
}

static void
on_greeter_begin_verification_for_user (GdmGreeterServer *greeter_server,
                                        const char       *username,
                                        GdmFactorySlave  *slave)
{
        g_debug ("GdmFactorySlave: begin verification for user");
        gdm_session_setup_for_user (GDM_SESSION (slave->priv->session),
                                    "gdm",
                                    username);
}

static void
on_greeter_answer (GdmGreeterServer *greeter_server,
                   const char       *text,
                   GdmFactorySlave  *slave)
{
        g_debug ("GdmFactorySlave: Greeter answer");
        gdm_session_answer_query (GDM_SESSION (slave->priv->session), text);
}

static void
on_greeter_session_selected (GdmGreeterServer *greeter_server,
                             const char       *text,
                             GdmFactorySlave  *slave)
{
        gdm_session_select_session (GDM_SESSION (slave->priv->session), text);
}

static void
on_greeter_language_selected (GdmGreeterServer *greeter_server,
                              const char       *text,
                              GdmFactorySlave  *slave)
{
        gdm_session_select_language (GDM_SESSION (slave->priv->session), text);
}

static void
on_greeter_layout_selected (GdmGreeterServer *greeter_server,
                            const char       *text,
                            GdmFactorySlave  *slave)
{
        gdm_session_select_layout (GDM_SESSION (slave->priv->session), text);
}

static void
on_greeter_user_selected (GdmGreeterServer *greeter_server,
                          const char       *text,
                          GdmFactorySlave  *slave)
{
        gdm_session_select_user (GDM_SESSION (slave->priv->session), text);
}

static void
on_greeter_cancel (GdmGreeterServer *greeter_server,
                   GdmFactorySlave  *slave)
{
        gdm_session_cancel (GDM_SESSION (slave->priv->session));
}

static void
on_greeter_connected (GdmGreeterServer *greeter_server,
                      GdmFactorySlave  *slave)
{
        g_debug ("GdmFactorySlave: Greeter started");

        create_product_display (slave);
}

static void
setup_server (GdmFactorySlave *slave)
{
        /* Set the busy cursor */
        gdm_slave_set_busy_cursor (GDM_SLAVE (slave));
}

static void
run_greeter (GdmFactorySlave *slave)
{
        gboolean       display_is_local;
        char          *display_id;
        char          *display_name;
        char          *seat_id;
        char          *display_device;
        char          *display_hostname;
        char          *auth_file;
        char          *address;

        g_debug ("GdmFactorySlave: Running greeter");

        display_is_local = FALSE;
        display_id = NULL;
        display_name = NULL;
        seat_id = NULL;
        auth_file = NULL;
        display_device = NULL;
        display_hostname = NULL;

        g_object_get (slave,
                      "display-is-local", &display_is_local,
                      "display-id", &display_id,
                      "display-name", &display_name,
                      "display-seat-id", &seat_id,
                      "display-hostname", &display_hostname,
                      "display-x11-authority-file", &auth_file,
                      NULL);

        if (slave->priv->server != NULL) {
                display_device = gdm_server_get_display_device (slave->priv->server);
        }

        /* FIXME: send a signal back to the master */

        /* Run the init script. gdmslave suspends until script has terminated */
        gdm_slave_run_script (GDM_SLAVE (slave), GDMCONFDIR "/Init", GDM_USERNAME);

        slave->priv->greeter_server = gdm_greeter_server_new (display_id);
        g_signal_connect (slave->priv->greeter_server,
                          "begin-verification",
                          G_CALLBACK (on_greeter_begin_verification),
                          slave);
        g_signal_connect (slave->priv->greeter_server,
                          "begin-verification-for-user",
                          G_CALLBACK (on_greeter_begin_verification_for_user),
                          slave);
        g_signal_connect (slave->priv->greeter_server,
                          "query-answer",
                          G_CALLBACK (on_greeter_answer),
                          slave);
        g_signal_connect (slave->priv->greeter_server,
                          "session-selected",
                          G_CALLBACK (on_greeter_session_selected),
                          slave);
        g_signal_connect (slave->priv->greeter_server,
                          "language-selected",
                          G_CALLBACK (on_greeter_language_selected),
                          slave);
        g_signal_connect (slave->priv->greeter_server,
                          "layout-selected",
                          G_CALLBACK (on_greeter_layout_selected),
                          slave);
        g_signal_connect (slave->priv->greeter_server,
                          "user-selected",
                          G_CALLBACK (on_greeter_user_selected),
                          slave);
        g_signal_connect (slave->priv->greeter_server,
                          "connected",
                          G_CALLBACK (on_greeter_connected),
                          slave);
        g_signal_connect (slave->priv->greeter_server,
                          "cancelled",
                          G_CALLBACK (on_greeter_cancel),
                          slave);
        gdm_greeter_server_start (slave->priv->greeter_server);

        address = gdm_greeter_server_get_address (slave->priv->greeter_server);

        g_debug ("GdmFactorySlave: Creating greeter on %s %s", display_name, display_device);
        slave->priv->greeter = gdm_greeter_session_new (display_name,
                                                        seat_id,
                                                        display_device,
                                                        display_hostname,
                                                        display_is_local);
        g_signal_connect (slave->priv->greeter,
                          "started",
                          G_CALLBACK (on_greeter_session_start),
                          slave);
        g_signal_connect (slave->priv->greeter,
                          "stopped",
                          G_CALLBACK (on_greeter_session_stop),
                          slave);
        g_signal_connect (slave->priv->greeter,
                          "exited",
                          G_CALLBACK (on_greeter_session_exited),
                          slave);
        g_signal_connect (slave->priv->greeter,
                          "died",
                          G_CALLBACK (on_greeter_session_died),
                          slave);
        g_object_set (slave->priv->greeter,
                      "x11-authority-file", auth_file,
                      NULL);
        gdm_welcome_session_set_server_address (GDM_WELCOME_SESSION (slave->priv->greeter), address);
        gdm_welcome_session_start (GDM_WELCOME_SESSION (slave->priv->greeter));

        g_free (address);

        g_free (display_id);
        g_free (display_name);
        g_free (seat_id);
        g_free (display_device);
        g_free (display_hostname);
        g_free (auth_file);
}

static gboolean
idle_connect_to_display (GdmFactorySlave *slave)
{
        gboolean res;

        slave->priv->connection_attempts++;

        g_debug ("GdmFactorySlave: Connect to display");

        res = gdm_slave_connect_to_x11_display (GDM_SLAVE (slave));
        if (res) {
                /* FIXME: handle wait-for-go */

                setup_server (slave);
                run_greeter (slave);
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
                 GdmFactorySlave *slave)
{
        g_debug ("GdmFactorySlave: Server ready");

        g_timeout_add (500, (GSourceFunc)idle_connect_to_display, slave);
}

static void
on_server_exited (GdmServer       *server,
                  int              exit_code,
                  GdmFactorySlave *slave)
{
        g_debug ("GdmFactorySlave: server exited with code %d\n", exit_code);

        gdm_slave_stopped (GDM_SLAVE (slave));
}

static void
on_server_died (GdmServer       *server,
                int              signal_number,
                GdmFactorySlave *slave)
{
        g_debug ("GdmFactorySlave: server died with signal %d, (%s)",
                 signal_number,
                 g_strsignal (signal_number));

        gdm_slave_stopped (GDM_SLAVE (slave));
}

static gboolean
gdm_factory_slave_run (GdmFactorySlave *slave)
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

                g_debug ("GdmFactorySlave: Started X server");
        } else {
                g_timeout_add (500, (GSourceFunc)idle_connect_to_display, slave);
        }

        g_free (display_name);
        g_free (auth_file);

        return TRUE;
}

static gboolean
gdm_factory_slave_start (GdmSlave *slave)
{
        gboolean ret;

        ret = FALSE;

        g_debug ("GdmFactorySlave: Starting factory slave");

        GDM_SLAVE_CLASS (gdm_factory_slave_parent_class)->start (slave);

        GDM_FACTORY_SLAVE (slave)->priv->session = gdm_session_relay_new ();
        g_signal_connect (GDM_FACTORY_SLAVE (slave)->priv->session,
                          "conversation-started",
                          G_CALLBACK (on_session_conversation_started),
                          slave);
        g_signal_connect (GDM_FACTORY_SLAVE (slave)->priv->session,
                          "setup-complete",
                          G_CALLBACK (on_session_setup_complete),
                          slave);
        g_signal_connect (GDM_FACTORY_SLAVE (slave)->priv->session,
                          "setup-failed",
                          G_CALLBACK (on_session_setup_failed),
                          slave);
        g_signal_connect (GDM_FACTORY_SLAVE (slave)->priv->session,
                          "reset-complete",
                          G_CALLBACK (on_session_reset_complete),
                          slave);
        g_signal_connect (GDM_FACTORY_SLAVE (slave)->priv->session,
                          "reset-failed",
                          G_CALLBACK (on_session_reset_failed),
                          slave);
        g_signal_connect (GDM_FACTORY_SLAVE (slave)->priv->session,
                          "authenticated",
                          G_CALLBACK (on_session_authenticated),
                          slave);
        g_signal_connect (GDM_FACTORY_SLAVE (slave)->priv->session,
                          "authentication-failed",
                          G_CALLBACK (on_session_authentication_failed),
                          slave);
        g_signal_connect (GDM_FACTORY_SLAVE (slave)->priv->session,
                          "authorized",
                          G_CALLBACK (on_session_authorized),
                          slave);
        g_signal_connect (GDM_FACTORY_SLAVE (slave)->priv->session,
                          "authorization-failed",
                          G_CALLBACK (on_session_authorization_failed),
                          slave);
        g_signal_connect (GDM_FACTORY_SLAVE (slave)->priv->session,
                          "accredited",
                          G_CALLBACK (on_session_accredited),
                          slave);
        g_signal_connect (GDM_FACTORY_SLAVE (slave)->priv->session,
                          "accreditation-failed",
                          G_CALLBACK (on_session_accreditation_failed),
                          slave);
        g_signal_connect (GDM_FACTORY_SLAVE (slave)->priv->session,
                          "session-opened",
                          G_CALLBACK (on_session_opened),
                          slave);
        g_signal_connect (GDM_FACTORY_SLAVE (slave)->priv->session,
                          "session-open-failed",
                          G_CALLBACK (on_session_open_failed),
                          slave);
        g_signal_connect (GDM_FACTORY_SLAVE (slave)->priv->session,
                          "info",
                          G_CALLBACK (on_session_info),
                          slave);
        g_signal_connect (GDM_FACTORY_SLAVE (slave)->priv->session,
                          "problem",
                          G_CALLBACK (on_session_problem),
                          slave);
        g_signal_connect (GDM_FACTORY_SLAVE (slave)->priv->session,
                          "info-query",
                          G_CALLBACK (on_session_info_query),
                          slave);
        g_signal_connect (GDM_FACTORY_SLAVE (slave)->priv->session,
                          "secret-info-query",
                          G_CALLBACK (on_session_secret_info_query),
                          slave);
        g_signal_connect (GDM_FACTORY_SLAVE (slave)->priv->session,
                          "session-started",
                          G_CALLBACK (on_session_session_started),
                          slave);

        g_signal_connect (GDM_FACTORY_SLAVE (slave)->priv->session,
                          "connected",
                          G_CALLBACK (on_session_relay_connected),
                          slave);
        g_signal_connect (GDM_FACTORY_SLAVE (slave)->priv->session,
                          "disconnected",
                          G_CALLBACK (on_session_relay_disconnected),
                          slave);

        gdm_session_relay_start (GDM_FACTORY_SLAVE (slave)->priv->session);

        gdm_factory_slave_run (GDM_FACTORY_SLAVE (slave));

        ret = TRUE;

        return ret;
}

static gboolean
gdm_factory_slave_stop (GdmSlave *slave)
{
        g_debug ("GdmFactorySlave: Stopping factory_slave");

        GDM_SLAVE_CLASS (gdm_factory_slave_parent_class)->stop (slave);

        if (GDM_FACTORY_SLAVE (slave)->priv->session != NULL) {
                gdm_session_relay_stop (GDM_FACTORY_SLAVE (slave)->priv->session);
                g_object_unref (GDM_FACTORY_SLAVE (slave)->priv->session);
                GDM_FACTORY_SLAVE (slave)->priv->session = NULL;
        }

        if (GDM_FACTORY_SLAVE (slave)->priv->greeter_server != NULL) {
                gdm_greeter_server_stop (GDM_FACTORY_SLAVE (slave)->priv->greeter_server);
                g_object_unref (GDM_FACTORY_SLAVE (slave)->priv->greeter_server);
                GDM_FACTORY_SLAVE (slave)->priv->greeter_server = NULL;
        }

        if (GDM_FACTORY_SLAVE (slave)->priv->greeter != NULL) {
                gdm_welcome_session_stop (GDM_WELCOME_SESSION (GDM_FACTORY_SLAVE (slave)->priv->greeter));
                g_object_unref (GDM_FACTORY_SLAVE (slave)->priv->greeter);
                GDM_FACTORY_SLAVE (slave)->priv->greeter = NULL;
        }

        if (GDM_FACTORY_SLAVE (slave)->priv->server != NULL) {
                gdm_server_stop (GDM_FACTORY_SLAVE (slave)->priv->server);
                g_object_unref (GDM_FACTORY_SLAVE (slave)->priv->server);
                GDM_FACTORY_SLAVE (slave)->priv->server = NULL;
        }

        if (GDM_FACTORY_SLAVE (slave)->priv->factory_proxy != NULL) {
                g_object_unref (GDM_FACTORY_SLAVE (slave)->priv->factory_proxy);
        }

        return TRUE;
}

static void
gdm_factory_slave_set_property (GObject      *object,
                               guint          prop_id,
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
gdm_factory_slave_get_property (GObject    *object,
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
gdm_factory_slave_constructor (GType                  type,
                               guint                  n_construct_properties,
                               GObjectConstructParam *construct_properties)
{
        GdmFactorySlave      *factory_slave;

        factory_slave = GDM_FACTORY_SLAVE (G_OBJECT_CLASS (gdm_factory_slave_parent_class)->constructor (type,
                                                                                                         n_construct_properties,
                                                                                                         construct_properties));

        return G_OBJECT (factory_slave);
}

static void
gdm_factory_slave_class_init (GdmFactorySlaveClass *klass)
{
        GObjectClass  *object_class = G_OBJECT_CLASS (klass);
        GdmSlaveClass *slave_class = GDM_SLAVE_CLASS (klass);

        object_class->get_property = gdm_factory_slave_get_property;
        object_class->set_property = gdm_factory_slave_set_property;
        object_class->constructor = gdm_factory_slave_constructor;
        object_class->finalize = gdm_factory_slave_finalize;

        slave_class->start = gdm_factory_slave_start;
        slave_class->stop = gdm_factory_slave_stop;

        g_type_class_add_private (klass, sizeof (GdmFactorySlavePrivate));

        dbus_g_object_type_install_info (GDM_TYPE_FACTORY_SLAVE, &dbus_glib_gdm_factory_slave_object_info);
}

static void
gdm_factory_slave_init (GdmFactorySlave *slave)
{
        GError *error;

        slave->priv = GDM_FACTORY_SLAVE_GET_PRIVATE (slave);

        slave->priv->pid = -1;

        error = NULL;
        slave->priv->connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
        if (slave->priv->connection == NULL) {
                if (error != NULL) {
                        g_critical ("error getting system bus: %s", error->message);
                        g_error_free (error);
                }
                exit (1);
        }
}

static void
gdm_factory_slave_finalize (GObject *object)
{
        GdmFactorySlave *factory_slave;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GDM_IS_FACTORY_SLAVE (object));

        factory_slave = GDM_FACTORY_SLAVE (object);

        g_debug ("GdmFactorySlave: Finalizing slave");

        g_return_if_fail (factory_slave->priv != NULL);

        gdm_factory_slave_stop (GDM_SLAVE (factory_slave));

        if (factory_slave->priv->greeter_reset_id > 0) {
                g_source_remove (factory_slave->priv->greeter_reset_id);
                factory_slave->priv->greeter_reset_id = 0;
        }

        G_OBJECT_CLASS (gdm_factory_slave_parent_class)->finalize (object);
}

GdmSlave *
gdm_factory_slave_new (const char *id)
{
        GObject *object;

        object = g_object_new (GDM_TYPE_FACTORY_SLAVE,
                               "display-id", id,
                               NULL);

        return GDM_SLAVE (object);
}
