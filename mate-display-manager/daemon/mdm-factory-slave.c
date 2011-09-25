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

#include "mdm-common.h"

#include "mdm-factory-slave.h"
#include "mdm-factory-slave-glue.h"

#include "mdm-server.h"
#include "mdm-greeter-session.h"
#include "mdm-greeter-server.h"

#include "mdm-session-relay.h"

extern char **environ;

#define MDM_FACTORY_SLAVE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_FACTORY_SLAVE, MdmFactorySlavePrivate))

#define MDM_DBUS_NAME                            "org.mate.DisplayManager"
#define MDM_DBUS_LOCAL_DISPLAY_FACTORY_PATH      "/org/mate/DisplayManager/LocalDisplayFactory"
#define MDM_DBUS_LOCAL_DISPLAY_FACTORY_INTERFACE "org.mate.DisplayManager.LocalDisplayFactory"

#define MAX_CONNECT_ATTEMPTS 10

struct MdmFactorySlavePrivate
{
        char              *id;
        GPid               pid;
        guint              greeter_reset_id;

        GPid               server_pid;
        Display           *server_display;
        guint              connection_attempts;

        MdmServer         *server;
        MdmSessionRelay   *session;
        MdmGreeterServer  *greeter_server;
        MdmGreeterSession *greeter;
        DBusGProxy        *factory_proxy;
        DBusGConnection   *connection;
};

enum {
        PROP_0,
};

static void     mdm_factory_slave_class_init    (MdmFactorySlaveClass *klass);
static void     mdm_factory_slave_init          (MdmFactorySlave      *factory_slave);
static void     mdm_factory_slave_finalize      (GObject             *object);

G_DEFINE_TYPE (MdmFactorySlave, mdm_factory_slave, MDM_TYPE_SLAVE)

static gboolean
greeter_reset_timeout (MdmFactorySlave *slave)
{
        mdm_greeter_server_reset (slave->priv->greeter_server);
        slave->priv->greeter_reset_id = 0;
        return FALSE;
}

static void
queue_greeter_reset (MdmFactorySlave *slave)
{
        if (slave->priv->greeter_reset_id > 0) {
                return;
        }

        slave->priv->greeter_reset_id = g_timeout_add_seconds (2, (GSourceFunc)greeter_reset_timeout, slave);
}

static void
on_greeter_session_start (MdmGreeterSession *greeter,
                          MdmFactorySlave   *slave)
{
        g_debug ("MdmFactorySlave: Greeter started");
}

static void
on_greeter_session_stop (MdmGreeterSession *greeter,
                         MdmFactorySlave   *slave)
{
        g_debug ("MdmFactorySlave: Greeter stopped");
}

static void
on_greeter_session_exited (MdmGreeterSession    *greeter,
                           int                   code,
                           MdmFactorySlave      *slave)
{
        g_debug ("MdmSimpleSlave: Greeter exited: %d", code);
        mdm_slave_stopped (MDM_SLAVE (slave));
}

static void
on_greeter_session_died (MdmGreeterSession    *greeter,
                         int                   signal,
                         MdmFactorySlave      *slave)
{
        g_debug ("MdmSimpleSlave: Greeter died: %d", signal);
        mdm_slave_stopped (MDM_SLAVE (slave));
}


static void
on_session_info (MdmSession      *session,
                 const char      *text,
                 MdmFactorySlave *slave)
{
        g_debug ("MdmFactorySlave: Info: %s", text);
        mdm_greeter_server_info (slave->priv->greeter_server, text);
}

static void
on_session_problem (MdmSession      *session,
                    const char      *text,
                    MdmFactorySlave *slave)
{
        g_debug ("MdmFactorySlave: Problem: %s", text);
        mdm_greeter_server_problem (slave->priv->greeter_server, text);
}

static void
on_session_info_query (MdmSession      *session,
                       const char      *text,
                       MdmFactorySlave *slave)
{

        g_debug ("MdmFactorySlave: Info query: %s", text);
        mdm_greeter_server_info_query (slave->priv->greeter_server, text);
}

static void
on_session_secret_info_query (MdmSession      *session,
                              const char      *text,
                              MdmFactorySlave *slave)
{
        g_debug ("MdmFactorySlave: Secret info query: %s", text);
        mdm_greeter_server_secret_info_query (slave->priv->greeter_server, text);
}

static void
on_session_conversation_started (MdmSession      *session,
                                 MdmFactorySlave *slave)
{
        g_debug ("MdmFactorySlave: session conversation started");

        mdm_greeter_server_ready (slave->priv->greeter_server);
}

static void
on_session_setup_complete (MdmSession      *session,
                           MdmFactorySlave *slave)
{
        mdm_session_authenticate (session);
}

static void
on_session_setup_failed (MdmSession      *session,
                         const char      *message,
                         MdmFactorySlave *slave)
{
        mdm_greeter_server_problem (slave->priv->greeter_server, _("Unable to initialize login system"));

        queue_greeter_reset (slave);
}

static void
on_session_reset_complete (MdmSession      *session,
                           MdmFactorySlave *slave)
{
        g_debug ("MdmFactorySlave: PAM reset");
}

static void
on_session_reset_failed (MdmSession      *session,
                         const char      *message,
                         MdmFactorySlave *slave)
{
        g_critical ("Unable to reset PAM");
}

static void
on_session_authenticated (MdmSession      *session,
                          MdmFactorySlave *slave)
{
        mdm_session_authorize (session);
}

static void
on_session_authentication_failed (MdmSession      *session,
                                  const char      *message,
                                  MdmFactorySlave *slave)
{
        mdm_greeter_server_problem (slave->priv->greeter_server, _("Unable to authenticate user"));

        queue_greeter_reset (slave);
}

static void
on_session_authorized (MdmSession      *session,
                       MdmFactorySlave *slave)
{
        int flag;

        /* FIXME: check for migration? */
        flag = MDM_SESSION_CRED_ESTABLISH;

        mdm_session_accredit (session, flag);
}

static void
on_session_authorization_failed (MdmSession      *session,
                                 const char      *message,
                                 MdmFactorySlave *slave)
{
        mdm_greeter_server_problem (slave->priv->greeter_server, _("Unable to authorize user"));

        queue_greeter_reset (slave);
}

static void
on_session_accredited (MdmSession      *session,
                       MdmFactorySlave *slave)
{
        g_debug ("MdmFactorySlave:  session user verified");

        mdm_session_open_session (session);
}

static void
on_session_accreditation_failed (MdmSession      *session,
                                 const char      *message,
                                 MdmFactorySlave *slave)
{
        g_debug ("MdmFactorySlave: could not successfully authenticate user: %s",
                 message);

        mdm_greeter_server_problem (slave->priv->greeter_server, _("Unable to establish credentials"));

        queue_greeter_reset (slave);
}

static void
on_session_opened (MdmSession      *session,
                   MdmFactorySlave *slave)
{
        g_debug ("MdmFactorySlave: session opened");

        mdm_session_start_session (session);

        mdm_greeter_server_reset (slave->priv->greeter_server);
}

static void
on_session_open_failed (MdmSession      *session,
                        const char      *message,
                        MdmFactorySlave *slave)
{
        g_debug ("MdmFactorySlave: could not open session: %s", message);

        mdm_greeter_server_problem (slave->priv->greeter_server, _("Unable to open session"));

        queue_greeter_reset (slave);
}

static void
on_session_session_started (MdmSession      *session,
                            int              pid,
                            MdmFactorySlave *slave)
{
        g_debug ("MdmFactorySlave: Relay session started");

        mdm_greeter_server_reset (slave->priv->greeter_server);
}

static gboolean
create_product_display (MdmFactorySlave *slave)
{
        char    *parent_display_id;
        char    *server_address;
        char    *product_id;
        GError  *error;
        gboolean res;
        gboolean ret;

        ret = FALSE;

        g_debug ("MdmFactorySlave: Create product display");

        g_debug ("MdmFactorySlave: Connecting to local display factory");
        slave->priv->factory_proxy = dbus_g_proxy_new_for_name (slave->priv->connection,
                                                                MDM_DBUS_NAME,
                                                                MDM_DBUS_LOCAL_DISPLAY_FACTORY_PATH,
                                                                MDM_DBUS_LOCAL_DISPLAY_FACTORY_INTERFACE);
        if (slave->priv->factory_proxy == NULL) {
                g_warning ("Failed to create local display factory proxy");
                goto out;
        }

        server_address = mdm_session_relay_get_address (slave->priv->session);

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
on_session_relay_disconnected (MdmSessionRelay *session,
                               MdmFactorySlave *slave)
{
        g_debug ("MdmFactorySlave: Relay disconnected");

        /* FIXME: do some kind of loop detection */
        mdm_greeter_server_reset (slave->priv->greeter_server);
        create_product_display (slave);
}

static void
on_session_relay_connected (MdmSessionRelay *session,
                            MdmFactorySlave *slave)
{
        g_debug ("MdmFactorySlave: Relay Connected");

        mdm_session_start_conversation (MDM_SESSION (slave->priv->session));
}

static void
on_greeter_begin_verification (MdmGreeterServer *greeter_server,
                               MdmFactorySlave  *slave)
{
        g_debug ("MdmFactorySlave: begin verification");
        mdm_session_setup (MDM_SESSION (slave->priv->session),
                           "mdm");
}

static void
on_greeter_begin_verification_for_user (MdmGreeterServer *greeter_server,
                                        const char       *username,
                                        MdmFactorySlave  *slave)
{
        g_debug ("MdmFactorySlave: begin verification for user");
        mdm_session_setup_for_user (MDM_SESSION (slave->priv->session),
                                    "mdm",
                                    username);
}

static void
on_greeter_answer (MdmGreeterServer *greeter_server,
                   const char       *text,
                   MdmFactorySlave  *slave)
{
        g_debug ("MdmFactorySlave: Greeter answer");
        mdm_session_answer_query (MDM_SESSION (slave->priv->session), text);
}

static void
on_greeter_session_selected (MdmGreeterServer *greeter_server,
                             const char       *text,
                             MdmFactorySlave  *slave)
{
        mdm_session_select_session (MDM_SESSION (slave->priv->session), text);
}

static void
on_greeter_language_selected (MdmGreeterServer *greeter_server,
                              const char       *text,
                              MdmFactorySlave  *slave)
{
        mdm_session_select_language (MDM_SESSION (slave->priv->session), text);
}

static void
on_greeter_layout_selected (MdmGreeterServer *greeter_server,
                            const char       *text,
                            MdmFactorySlave  *slave)
{
        mdm_session_select_layout (MDM_SESSION (slave->priv->session), text);
}

static void
on_greeter_user_selected (MdmGreeterServer *greeter_server,
                          const char       *text,
                          MdmFactorySlave  *slave)
{
        mdm_session_select_user (MDM_SESSION (slave->priv->session), text);
}

static void
on_greeter_cancel (MdmGreeterServer *greeter_server,
                   MdmFactorySlave  *slave)
{
        mdm_session_cancel (MDM_SESSION (slave->priv->session));
}

static void
on_greeter_connected (MdmGreeterServer *greeter_server,
                      MdmFactorySlave  *slave)
{
        g_debug ("MdmFactorySlave: Greeter started");

        create_product_display (slave);
}

static void
setup_server (MdmFactorySlave *slave)
{
        /* Set the busy cursor */
        mdm_slave_set_busy_cursor (MDM_SLAVE (slave));
}

static void
run_greeter (MdmFactorySlave *slave)
{
        gboolean       display_is_local;
        char          *display_id;
        char          *display_name;
        char          *seat_id;
        char          *display_device;
        char          *display_hostname;
        char          *auth_file;
        char          *address;

        g_debug ("MdmFactorySlave: Running greeter");

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
                display_device = mdm_server_get_display_device (slave->priv->server);
        }

        /* FIXME: send a signal back to the master */

        /* Run the init script. mdmslave suspends until script has terminated */
        mdm_slave_run_script (MDM_SLAVE (slave), MDMCONFDIR "/Init", MDM_USERNAME);

        slave->priv->greeter_server = mdm_greeter_server_new (display_id);
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
        mdm_greeter_server_start (slave->priv->greeter_server);

        address = mdm_greeter_server_get_address (slave->priv->greeter_server);

        g_debug ("MdmFactorySlave: Creating greeter on %s %s", display_name, display_device);
        slave->priv->greeter = mdm_greeter_session_new (display_name,
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
        mdm_welcome_session_set_server_address (MDM_WELCOME_SESSION (slave->priv->greeter), address);
        mdm_welcome_session_start (MDM_WELCOME_SESSION (slave->priv->greeter));

        g_free (address);

        g_free (display_id);
        g_free (display_name);
        g_free (seat_id);
        g_free (display_device);
        g_free (display_hostname);
        g_free (auth_file);
}

static gboolean
idle_connect_to_display (MdmFactorySlave *slave)
{
        gboolean res;

        slave->priv->connection_attempts++;

        g_debug ("MdmFactorySlave: Connect to display");

        res = mdm_slave_connect_to_x11_display (MDM_SLAVE (slave));
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
on_server_ready (MdmServer       *server,
                 MdmFactorySlave *slave)
{
        g_debug ("MdmFactorySlave: Server ready");

        g_timeout_add (500, (GSourceFunc)idle_connect_to_display, slave);
}

static void
on_server_exited (MdmServer       *server,
                  int              exit_code,
                  MdmFactorySlave *slave)
{
        g_debug ("MdmFactorySlave: server exited with code %d\n", exit_code);

        mdm_slave_stopped (MDM_SLAVE (slave));
}

static void
on_server_died (MdmServer       *server,
                int              signal_number,
                MdmFactorySlave *slave)
{
        g_debug ("MdmFactorySlave: server died with signal %d, (%s)",
                 signal_number,
                 g_strsignal (signal_number));

        mdm_slave_stopped (MDM_SLAVE (slave));
}

static gboolean
mdm_factory_slave_run (MdmFactorySlave *slave)
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

                slave->priv->server = mdm_server_new (display_name, auth_file);
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

                res = mdm_server_start (slave->priv->server);
                if (! res) {
                        g_warning (_("Could not start the X "
                                     "server (your graphical environment) "
                                     "due to an internal error. "
                                     "Please contact your system administrator "
                                     "or check your syslog to diagnose. "
                                     "In the meantime this display will be "
                                     "disabled.  Please restart MDM when "
                                     "the problem is corrected."));
                        exit (1);
                }

                g_debug ("MdmFactorySlave: Started X server");
        } else {
                g_timeout_add (500, (GSourceFunc)idle_connect_to_display, slave);
        }

        g_free (display_name);
        g_free (auth_file);

        return TRUE;
}

static gboolean
mdm_factory_slave_start (MdmSlave *slave)
{
        gboolean ret;

        ret = FALSE;

        g_debug ("MdmFactorySlave: Starting factory slave");

        MDM_SLAVE_CLASS (mdm_factory_slave_parent_class)->start (slave);

        MDM_FACTORY_SLAVE (slave)->priv->session = mdm_session_relay_new ();
        g_signal_connect (MDM_FACTORY_SLAVE (slave)->priv->session,
                          "conversation-started",
                          G_CALLBACK (on_session_conversation_started),
                          slave);
        g_signal_connect (MDM_FACTORY_SLAVE (slave)->priv->session,
                          "setup-complete",
                          G_CALLBACK (on_session_setup_complete),
                          slave);
        g_signal_connect (MDM_FACTORY_SLAVE (slave)->priv->session,
                          "setup-failed",
                          G_CALLBACK (on_session_setup_failed),
                          slave);
        g_signal_connect (MDM_FACTORY_SLAVE (slave)->priv->session,
                          "reset-complete",
                          G_CALLBACK (on_session_reset_complete),
                          slave);
        g_signal_connect (MDM_FACTORY_SLAVE (slave)->priv->session,
                          "reset-failed",
                          G_CALLBACK (on_session_reset_failed),
                          slave);
        g_signal_connect (MDM_FACTORY_SLAVE (slave)->priv->session,
                          "authenticated",
                          G_CALLBACK (on_session_authenticated),
                          slave);
        g_signal_connect (MDM_FACTORY_SLAVE (slave)->priv->session,
                          "authentication-failed",
                          G_CALLBACK (on_session_authentication_failed),
                          slave);
        g_signal_connect (MDM_FACTORY_SLAVE (slave)->priv->session,
                          "authorized",
                          G_CALLBACK (on_session_authorized),
                          slave);
        g_signal_connect (MDM_FACTORY_SLAVE (slave)->priv->session,
                          "authorization-failed",
                          G_CALLBACK (on_session_authorization_failed),
                          slave);
        g_signal_connect (MDM_FACTORY_SLAVE (slave)->priv->session,
                          "accredited",
                          G_CALLBACK (on_session_accredited),
                          slave);
        g_signal_connect (MDM_FACTORY_SLAVE (slave)->priv->session,
                          "accreditation-failed",
                          G_CALLBACK (on_session_accreditation_failed),
                          slave);
        g_signal_connect (MDM_FACTORY_SLAVE (slave)->priv->session,
                          "session-opened",
                          G_CALLBACK (on_session_opened),
                          slave);
        g_signal_connect (MDM_FACTORY_SLAVE (slave)->priv->session,
                          "session-open-failed",
                          G_CALLBACK (on_session_open_failed),
                          slave);
        g_signal_connect (MDM_FACTORY_SLAVE (slave)->priv->session,
                          "info",
                          G_CALLBACK (on_session_info),
                          slave);
        g_signal_connect (MDM_FACTORY_SLAVE (slave)->priv->session,
                          "problem",
                          G_CALLBACK (on_session_problem),
                          slave);
        g_signal_connect (MDM_FACTORY_SLAVE (slave)->priv->session,
                          "info-query",
                          G_CALLBACK (on_session_info_query),
                          slave);
        g_signal_connect (MDM_FACTORY_SLAVE (slave)->priv->session,
                          "secret-info-query",
                          G_CALLBACK (on_session_secret_info_query),
                          slave);
        g_signal_connect (MDM_FACTORY_SLAVE (slave)->priv->session,
                          "session-started",
                          G_CALLBACK (on_session_session_started),
                          slave);

        g_signal_connect (MDM_FACTORY_SLAVE (slave)->priv->session,
                          "connected",
                          G_CALLBACK (on_session_relay_connected),
                          slave);
        g_signal_connect (MDM_FACTORY_SLAVE (slave)->priv->session,
                          "disconnected",
                          G_CALLBACK (on_session_relay_disconnected),
                          slave);

        mdm_session_relay_start (MDM_FACTORY_SLAVE (slave)->priv->session);

        mdm_factory_slave_run (MDM_FACTORY_SLAVE (slave));

        ret = TRUE;

        return ret;
}

static gboolean
mdm_factory_slave_stop (MdmSlave *slave)
{
        g_debug ("MdmFactorySlave: Stopping factory_slave");

        MDM_SLAVE_CLASS (mdm_factory_slave_parent_class)->stop (slave);

        if (MDM_FACTORY_SLAVE (slave)->priv->session != NULL) {
                mdm_session_relay_stop (MDM_FACTORY_SLAVE (slave)->priv->session);
                g_object_unref (MDM_FACTORY_SLAVE (slave)->priv->session);
                MDM_FACTORY_SLAVE (slave)->priv->session = NULL;
        }

        if (MDM_FACTORY_SLAVE (slave)->priv->greeter_server != NULL) {
                mdm_greeter_server_stop (MDM_FACTORY_SLAVE (slave)->priv->greeter_server);
                g_object_unref (MDM_FACTORY_SLAVE (slave)->priv->greeter_server);
                MDM_FACTORY_SLAVE (slave)->priv->greeter_server = NULL;
        }

        if (MDM_FACTORY_SLAVE (slave)->priv->greeter != NULL) {
                mdm_welcome_session_stop (MDM_WELCOME_SESSION (MDM_FACTORY_SLAVE (slave)->priv->greeter));
                g_object_unref (MDM_FACTORY_SLAVE (slave)->priv->greeter);
                MDM_FACTORY_SLAVE (slave)->priv->greeter = NULL;
        }

        if (MDM_FACTORY_SLAVE (slave)->priv->server != NULL) {
                mdm_server_stop (MDM_FACTORY_SLAVE (slave)->priv->server);
                g_object_unref (MDM_FACTORY_SLAVE (slave)->priv->server);
                MDM_FACTORY_SLAVE (slave)->priv->server = NULL;
        }

        if (MDM_FACTORY_SLAVE (slave)->priv->factory_proxy != NULL) {
                g_object_unref (MDM_FACTORY_SLAVE (slave)->priv->factory_proxy);
        }

        return TRUE;
}

static void
mdm_factory_slave_set_property (GObject      *object,
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
mdm_factory_slave_get_property (GObject    *object,
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
mdm_factory_slave_constructor (GType                  type,
                               guint                  n_construct_properties,
                               GObjectConstructParam *construct_properties)
{
        MdmFactorySlave      *factory_slave;

        factory_slave = MDM_FACTORY_SLAVE (G_OBJECT_CLASS (mdm_factory_slave_parent_class)->constructor (type,
                                                                                                         n_construct_properties,
                                                                                                         construct_properties));

        return G_OBJECT (factory_slave);
}

static void
mdm_factory_slave_class_init (MdmFactorySlaveClass *klass)
{
        GObjectClass  *object_class = G_OBJECT_CLASS (klass);
        MdmSlaveClass *slave_class = MDM_SLAVE_CLASS (klass);

        object_class->get_property = mdm_factory_slave_get_property;
        object_class->set_property = mdm_factory_slave_set_property;
        object_class->constructor = mdm_factory_slave_constructor;
        object_class->finalize = mdm_factory_slave_finalize;

        slave_class->start = mdm_factory_slave_start;
        slave_class->stop = mdm_factory_slave_stop;

        g_type_class_add_private (klass, sizeof (MdmFactorySlavePrivate));

        dbus_g_object_type_install_info (MDM_TYPE_FACTORY_SLAVE, &dbus_glib_mdm_factory_slave_object_info);
}

static void
mdm_factory_slave_init (MdmFactorySlave *slave)
{
        GError *error;

        slave->priv = MDM_FACTORY_SLAVE_GET_PRIVATE (slave);

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
mdm_factory_slave_finalize (GObject *object)
{
        MdmFactorySlave *factory_slave;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_FACTORY_SLAVE (object));

        factory_slave = MDM_FACTORY_SLAVE (object);

        g_debug ("MdmFactorySlave: Finalizing slave");

        g_return_if_fail (factory_slave->priv != NULL);

        mdm_factory_slave_stop (MDM_SLAVE (factory_slave));

        if (factory_slave->priv->greeter_reset_id > 0) {
                g_source_remove (factory_slave->priv->greeter_reset_id);
                factory_slave->priv->greeter_reset_id = 0;
        }

        G_OBJECT_CLASS (mdm_factory_slave_parent_class)->finalize (object);
}

MdmSlave *
mdm_factory_slave_new (const char *id)
{
        GObject *object;

        object = g_object_new (MDM_TYPE_FACTORY_SLAVE,
                               "display-id", id,
                               NULL);

        return MDM_SLAVE (object);
}
