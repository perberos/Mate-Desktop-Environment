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

#ifdef  HAVE_LOGINDEVPERM
#include <libdevinfo.h>
#endif  /* HAVE_LOGINDEVPERM */

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <glib-object.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <X11/Xlib.h> /* for Display */

#include "mdm-common.h"

#include "mdm-settings-client.h"
#include "mdm-settings-keys.h"

#include "mdm-simple-slave.h"
#include "mdm-simple-slave-glue.h"

#include "mdm-server.h"
#include "mdm-session.h"
#include "mdm-session-direct.h"
#include "mdm-greeter-server.h"
#include "mdm-greeter-session.h"
#include "mdm-settings-direct.h"
#include "mdm-settings-keys.h"

#define MDM_SIMPLE_SLAVE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_SIMPLE_SLAVE, MdmSimpleSlavePrivate))

#define MDM_DBUS_NAME              "org.mate.DisplayManager"
#define MDM_DBUS_DISPLAY_INTERFACE "org.mate.DisplayManager.Display"

#define MAX_CONNECT_ATTEMPTS  10
#define DEFAULT_PING_INTERVAL 15

struct MdmSimpleSlavePrivate
{
        GPid               pid;

        guint              greeter_reset_id;
        guint              start_session_id;

        int                ping_interval;

        GPid               server_pid;
        guint              connection_attempts;

        MdmServer         *server;
        MdmSessionDirect  *session;

        MdmGreeterServer  *greeter_server;
        MdmGreeterSession *greeter;

        guint              start_session_when_ready : 1;
        guint              waiting_to_start_session : 1;
#ifdef  HAVE_LOGINDEVPERM
        gboolean           use_logindevperm;
#endif
};

enum {
        PROP_0,
};

static void     mdm_simple_slave_class_init     (MdmSimpleSlaveClass *klass);
static void     mdm_simple_slave_init           (MdmSimpleSlave      *simple_slave);
static void     mdm_simple_slave_finalize       (GObject             *object);

G_DEFINE_TYPE (MdmSimpleSlave, mdm_simple_slave, MDM_TYPE_SLAVE)

static void create_new_session (MdmSimpleSlave *slave);
static void start_greeter      (MdmSimpleSlave *slave);

static void
on_session_started (MdmSession       *session,
                    int               pid,
                    MdmSimpleSlave   *slave)
{
        char *username;

        g_debug ("MdmSimpleSlave: session started %d", pid);

        /* Run the PreSession script. mdmslave suspends until script has terminated */
        username = mdm_session_direct_get_username (slave->priv->session);
        if (username != NULL) {
                mdm_slave_run_script (MDM_SLAVE (slave), MDMCONFDIR "/PreSession", username);
        }
        g_free (username);

        /* FIXME: should we do something here?
         * Note that error return status from PreSession script should
         * be ignored in the case of a X-MDM-BypassXsession session, which can
         * be checked by calling:
         * mdm_session_direct_bypasses_xsession (session)
         */
}

#ifdef  HAVE_LOGINDEVPERM
static void
mdm_simple_slave_grant_console_permissions (MdmSimpleSlave *slave)
{
        char *username;
        char *display_device;
        struct passwd *passwd_entry;

        username = mdm_session_direct_get_username (slave->priv->session);
        display_device = mdm_session_direct_get_display_device (slave->priv->session);

        if (username != NULL) {
                mdm_get_pwent_for_name (username, &passwd_entry);

                /*
                 * Only do logindevperm processing if /dev/console or
                 * a device associated with a VT
                 */
                if (display_device != NULL &&
                   (strncmp (display_device, "/dev/vt/", strlen ("/dev/vt/")) == 0 ||
                    strcmp  (display_device, "/dev/console") == 0)) {
                        g_debug ("Logindevperm login for user %s, device %s",
                                 username, display_device);
                        (void) di_devperm_login (display_device,
                                                 passwd_entry->pw_uid,
                                                 passwd_entry->pw_gid,
                                                 NULL);
                        slave->priv->use_logindevperm = TRUE;
                }
        }

        if (!slave->priv->use_logindevperm) {
                g_debug ("Not calling di_devperm_login login for user %s, device %s",
                         username, display_device);
        }
}

static void
mdm_simple_slave_revoke_console_permissions (MdmSimpleSlave *slave)
{
        char *username;
        char *display_device;

        username = mdm_session_direct_get_username (slave->priv->session);
        display_device = mdm_session_direct_get_display_device (slave->priv->session);

        /*
         * Only do logindevperm processing if /dev/console or a device
         * associated with a VT.  Do this after processing the PostSession
         * script so that permissions for devices are not returned to root
         * before running the script.
         */
        if (slave->priv->use_logindevperm == TRUE &&
            display_device != NULL &&
           (strncmp (display_device, "/dev/vt/", strlen ("/dev/vt/")) == 0 ||
            strcmp  (display_device, "/dev/console") == 0)) {
                g_debug ("di_devperm_logout for user %s, device %s",
                         username, display_device);
                (void) di_devperm_logout (display_device);
                slave->priv->use_logindevperm = FALSE;
        } else {
                g_debug ("Not calling di_devperm_logout logout for user %s, device %s",
                         username, display_device);
        }

        g_free (username);
        g_free (display_device);
}
#endif  /* HAVE_LOGINDEVPERM */

static void
on_session_exited (MdmSession     *session,
                   int             exit_code,
                   MdmSimpleSlave *slave)
{
        g_debug ("MdmSimpleSlave: session exited with code %d\n", exit_code);
        mdm_slave_stopped (MDM_SLAVE (slave));
}

static void
on_session_died (MdmSession     *session,
                 int             signal_number,
                 MdmSimpleSlave *slave)
{
        g_debug ("MdmSimpleSlave: session died with signal %d, (%s)",
                 signal_number,
                 g_strsignal (signal_number));

        mdm_slave_stopped (MDM_SLAVE (slave));
}

static gboolean
add_user_authorization (MdmSimpleSlave *slave,
                        char          **filename)
{
        char    *username;
        gboolean ret;

        username = mdm_session_direct_get_username (slave->priv->session);
        ret = mdm_slave_add_user_authorization (MDM_SLAVE (slave),
                                                username,
                                                filename);
        g_free (username);

        return ret;
}

static void
destroy_session (MdmSimpleSlave *slave)
{
        if (slave->priv->session != NULL) {
                mdm_session_close (MDM_SESSION (slave->priv->session));
                g_object_unref (slave->priv->session);
                slave->priv->session = NULL;
        }
}

static void
reset_session (MdmSimpleSlave *slave)
{
        destroy_session (slave);
        create_new_session (slave);
        mdm_session_start_conversation (MDM_SESSION (slave->priv->session));
}

static gboolean
greeter_reset_timeout (MdmSimpleSlave *slave)
{
        g_debug ("MdmSimpleSlave: resetting greeter");

        if (slave->priv->greeter_server != NULL) {
                mdm_greeter_server_reset (slave->priv->greeter_server);
                reset_session (slave);
        } else {
                start_greeter (slave);
                create_new_session (slave);
        }
        slave->priv->greeter_reset_id = 0;
        return FALSE;
}

static gboolean
auth_failed_reset_timeout (MdmSimpleSlave *slave)
{
        g_debug ("MdmSimpleSlave: auth failed resetting slave");

        if (slave->priv->greeter_server != NULL) {
                mdm_greeter_server_authentication_failed (slave->priv->greeter_server);
                reset_session (slave);
        } else {
                start_greeter (slave);
                create_new_session (slave);
        }
        slave->priv->greeter_reset_id = 0;
        return FALSE;
}

static void
queue_greeter_reset (MdmSimpleSlave *slave)
{
        if (slave->priv->greeter_reset_id > 0) {
                return;
        }

        slave->priv->greeter_reset_id = g_idle_add ((GSourceFunc)greeter_reset_timeout, slave);
}

static void
queue_auth_failed_reset (MdmSimpleSlave *slave)
{
        /* use the greeter reset idle id so we don't do both at once */
        if (slave->priv->greeter_reset_id > 0) {
                return;
        }

        slave->priv->greeter_reset_id = g_idle_add ((GSourceFunc)auth_failed_reset_timeout, slave);
}

static void
on_session_setup_complete (MdmSession     *session,
                           MdmSimpleSlave *slave)
{
        mdm_session_authenticate (session);
}

static void
on_session_setup_failed (MdmSession     *session,
                         const char     *message,
                         MdmSimpleSlave *slave)
{
        if (slave->priv->greeter_server != NULL) {
                mdm_greeter_server_problem (slave->priv->greeter_server,
                                           message != NULL ? message:  _("Unable to initialize login system"));
        }

        destroy_session (slave);
        queue_greeter_reset (slave);
}

static void
on_session_reset_complete (MdmSession     *session,
                           MdmSimpleSlave *slave)
{
        g_debug ("MdmSimpleSlave: PAM reset");
}

static void
on_session_reset_failed (MdmSession     *session,
                         const char     *message,
                         MdmSimpleSlave *slave)
{
        g_critical ("Unable to reset PAM");
}

static void
on_session_authenticated (MdmSession     *session,
                          MdmSimpleSlave *slave)
{
        mdm_session_authorize (session);
}

static void
on_session_authentication_failed (MdmSession     *session,
                                  const char     *message,
                                  MdmSimpleSlave *slave)
{
        if (slave->priv->greeter_server != NULL) {
                mdm_greeter_server_problem (slave->priv->greeter_server,
                                            message != NULL ? message : _("Unable to authenticate user"));
        }

        destroy_session (slave);

        g_debug ("MdmSimpleSlave: Authentication failed - may retry");
        queue_auth_failed_reset (slave);
}

static void
mdm_simple_slave_accredit_when_ready (MdmSimpleSlave *slave)
{
        if (slave->priv->start_session_when_ready) {
                char *ssid;
                char *username;
                int   cred_flag;

                slave->priv->waiting_to_start_session = FALSE;

                username = mdm_session_direct_get_username (slave->priv->session);

                ssid = mdm_slave_get_primary_session_id_for_user (MDM_SLAVE (slave), username);
                if (ssid != NULL && ssid [0] != '\0') {
                        /* FIXME: we don't yet support refresh */
                        cred_flag = MDM_SESSION_CRED_ESTABLISH;
                } else {
                        cred_flag = MDM_SESSION_CRED_ESTABLISH;
                }
                g_free (ssid);
                g_free (username);

                mdm_session_accredit (MDM_SESSION (slave->priv->session), cred_flag);
        } else {
                slave->priv->waiting_to_start_session = TRUE;
        }
}

static void
on_session_authorized (MdmSession     *session,
                       MdmSimpleSlave *slave)
{
        if (slave->priv->greeter_server != NULL) {
                mdm_greeter_server_user_authorized (slave->priv->greeter_server);
                mdm_simple_slave_accredit_when_ready (slave);
        } else {
                slave->priv->start_session_when_ready = TRUE;
                mdm_simple_slave_accredit_when_ready (slave);
        }
}

static void
on_session_authorization_failed (MdmSession     *session,
                                 const char     *message,
                                 MdmSimpleSlave *slave)
{
        if (slave->priv->greeter_server != NULL) {
                mdm_greeter_server_problem (slave->priv->greeter_server,
                                           message != NULL ? message :  _("Unable to authorize user"));
        }

        destroy_session (slave);
        queue_greeter_reset (slave);
}

static gboolean
try_migrate_session (MdmSimpleSlave *slave)
{
        char    *username;
        gboolean res;

        g_debug ("MdmSimpleSlave: trying to migrate session");

        username = mdm_session_direct_get_username (slave->priv->session);

        /* try to switch to an existing session */
        res = mdm_slave_switch_to_user_session (MDM_SLAVE (slave), username);
        g_free (username);

        return res;
}

static void
stop_greeter (MdmSimpleSlave *slave)
{
        char *username;

        g_debug ("MdmSimpleSlave: Stopping greeter");

        if (slave->priv->greeter == NULL) {
                g_debug ("MdmSimpleSlave: No greeter running");
                return;
        }

        /* Run the PostLogin script. mdmslave suspends until script has terminated */
        username = mdm_session_direct_get_username (slave->priv->session);

        if (username != NULL) {
                mdm_slave_run_script (MDM_SLAVE (slave), MDMCONFDIR "/PostLogin", username);
        }
        g_free (username);

        mdm_welcome_session_stop (MDM_WELCOME_SESSION (slave->priv->greeter));
        mdm_greeter_server_stop (slave->priv->greeter_server);

        g_object_unref (slave->priv->greeter);
        slave->priv->greeter = NULL;
}

static gboolean
start_session_timeout (MdmSimpleSlave *slave)
{

        char    *auth_file;
        gboolean migrated;

        g_debug ("MdmSimpleSlave: accredited");

        migrated = try_migrate_session (slave);
        g_debug ("MdmSimpleSlave: migrated: %d", migrated);
        if (migrated) {
                destroy_session (slave);

                /* We don't stop the slave here because
                   when Xorg exits it switches to the VT it was
                   started from.  That interferes with fast
                   user switching. */
                queue_greeter_reset (slave);

                goto out;
        }

        stop_greeter (slave);

        auth_file = NULL;
        add_user_authorization (slave, &auth_file);

        g_assert (auth_file != NULL);

        g_object_set (slave->priv->session,
                      "user-x11-authority-file", auth_file,
                      NULL);

        g_free (auth_file);

        mdm_session_start_session (MDM_SESSION (slave->priv->session));
 out:
        slave->priv->start_session_id = 0;
        return FALSE;
}

static void
queue_start_session (MdmSimpleSlave *slave)
{
        if (slave->priv->start_session_id > 0) {
                return;
        }

        slave->priv->start_session_id = g_idle_add ((GSourceFunc)start_session_timeout, slave);
}

static void
on_session_accredited (MdmSession     *session,
                       MdmSimpleSlave *slave)
{
        mdm_session_open_session (session);
}

static void
on_session_accreditation_failed (MdmSession     *session,
                                 const char     *message,
                                 MdmSimpleSlave *slave)
{
        gboolean migrated;

        g_debug ("MdmSimpleSlave: accreditation failed");

        migrated = try_migrate_session (slave);

        /* If we switched to another session we don't care if
           accreditation fails */
        if (! migrated) {
                if (slave->priv->greeter_server != NULL) {
                        const char *problem;
                        if (message) {
                                problem = message;
                        } else {
                                problem = _("Unable to establish credentials");
                        }
                        mdm_greeter_server_problem (slave->priv->greeter_server,
                                                    problem);
                }
        }

        /* We don't stop the slave here after migrating because
           when Xorg exits it switches to the VT it was
           started from.  That interferes with fast
           user switching. */
        destroy_session (slave);

        queue_greeter_reset (slave);
}

static void
on_session_opened (MdmSession     *session,
                   MdmSimpleSlave *slave)
{
#ifdef  HAVE_LOGINDEVPERM
        mdm_simple_slave_grant_console_permissions (slave);
#endif  /* HAVE_LOGINDEVPERM */

        queue_start_session (slave);
}

static void
on_session_open_failed (MdmSession     *session,
                        const char     *message,
                        MdmSimpleSlave *slave)
{
        if (slave->priv->greeter_server != NULL) {
                mdm_greeter_server_problem (slave->priv->greeter_server,
                                            _("Unable to open session"));
        }

        destroy_session (slave);
        queue_greeter_reset (slave);
}

static void
on_session_info (MdmSession     *session,
                 const char     *text,
                 MdmSimpleSlave *slave)
{
        g_debug ("MdmSimpleSlave: Info: %s", text);
        if (slave->priv->greeter_server != NULL) {
                mdm_greeter_server_info (slave->priv->greeter_server, text);
        }
}

static void
on_session_problem (MdmSession     *session,
                    const char     *text,
                    MdmSimpleSlave *slave)
{
        g_debug ("MdmSimpleSlave: Problem: %s", text);
        mdm_greeter_server_problem (slave->priv->greeter_server, text);
}

static void
on_session_info_query (MdmSession     *session,
                       const char     *text,
                       MdmSimpleSlave *slave)
{

        g_debug ("MdmSimpleSlave: Info query: %s", text);
        mdm_greeter_server_info_query (slave->priv->greeter_server, text);
}

static void
on_session_secret_info_query (MdmSession     *session,
                              const char     *text,
                              MdmSimpleSlave *slave)
{
        g_debug ("MdmSimpleSlave: Secret info query: %s", text);
        mdm_greeter_server_secret_info_query (slave->priv->greeter_server, text);
}

static void
on_session_conversation_started (MdmSession     *session,
                                 MdmSimpleSlave *slave)
{
        gboolean res;
        gboolean enabled;
        char    *username;
        int      delay;

        g_debug ("MdmSimpleSlave: session conversation started");
        if (slave->priv->greeter_server != NULL) {
                res = mdm_greeter_server_ready (slave->priv->greeter_server);
                if (! res) {
                        g_warning ("Unable to send ready");
                }
        }

        enabled = FALSE;
        mdm_slave_get_timed_login_details (MDM_SLAVE (slave), &enabled, &username, &delay);
        if (! enabled) {
                return;
        }

        if (slave->priv->greeter_server != NULL) {
                mdm_greeter_server_request_timed_login (slave->priv->greeter_server, username, delay);
        } else {
                g_debug ("MdmSimpleSlave: begin auto login for user '%s'", username);
                mdm_session_setup_for_user (MDM_SESSION (slave->priv->session),
                                            "mdm-autologin",
                                            username);
        }

        g_free (username);
}

static void
on_session_selected_user_changed (MdmSession     *session,
                                  const char     *text,
                                  MdmSimpleSlave *slave)
{
        g_debug ("MdmSimpleSlave: Selected user changed: %s", text);

        if (slave->priv->greeter_server != NULL) {
                mdm_greeter_server_selected_user_changed (slave->priv->greeter_server, text);
        }
}

static void
on_default_language_name_changed (MdmSession     *session,
                                  const char     *text,
                                  MdmSimpleSlave *slave)
{
        g_debug ("MdmSimpleSlave: Default language name changed: %s", text);

        if (slave->priv->greeter_server != NULL) {
                mdm_greeter_server_default_language_name_changed (slave->priv->greeter_server, text);
        }
}

static void
on_default_layout_name_changed (MdmSession     *session,
                                const char     *text,
                                MdmSimpleSlave *slave)
{
        g_debug ("MdmSimpleSlave: Default layout name changed: %s", text);

        if (slave->priv->greeter_server != NULL) {
                mdm_greeter_server_default_layout_name_changed (slave->priv->greeter_server, text);
        }
}

static void
on_default_session_name_changed (MdmSession     *session,
                                 const char     *text,
                                 MdmSimpleSlave *slave)
{
        g_debug ("MdmSimpleSlave: Default session name changed: %s", text);

        if (slave->priv->greeter_server != NULL) {
                mdm_greeter_server_default_session_name_changed (slave->priv->greeter_server, text);
        }
}

static void
create_new_session (MdmSimpleSlave *slave)
{
        gboolean       display_is_local;
        char          *display_id;
        char          *display_name;
        char          *display_hostname;
        char          *display_device;
        char          *display_x11_authority_file;

        g_debug ("MdmSimpleSlave: Creating new session");

        g_object_get (slave,
                      "display-id", &display_id,
                      "display-name", &display_name,
                      "display-hostname", &display_hostname,
                      "display-is-local", &display_is_local,
                      "display-x11-authority-file", &display_x11_authority_file,
                      NULL);

        display_device = NULL;
        if (slave->priv->server != NULL) {
                display_device = mdm_server_get_display_device (slave->priv->server);
        }

        slave->priv->session = mdm_session_direct_new (display_id,
                                                       display_name,
                                                       display_hostname,
                                                       display_device,
                                                       display_x11_authority_file,
                                                       display_is_local);
        g_free (display_id);
        g_free (display_name);
        g_free (display_device);
        g_free (display_hostname);

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
#if 0
        g_signal_connect (slave->priv->session,
                          "closed",
                          G_CALLBACK (on_session_closed),
                          slave);
#endif
        g_signal_connect (slave->priv->session,
                          "selected-user-changed",
                          G_CALLBACK (on_session_selected_user_changed),
                          slave);

        g_signal_connect (slave->priv->session,
                          "default-language-name-changed",
                          G_CALLBACK (on_default_language_name_changed),
                          slave);

        g_signal_connect (slave->priv->session,
                          "default-layout-name-changed",
                          G_CALLBACK (on_default_layout_name_changed),
                          slave);

        g_signal_connect (slave->priv->session,
                          "default-session-name-changed",
                          G_CALLBACK (on_default_session_name_changed),
                          slave);
}

static void
on_greeter_session_start (MdmGreeterSession *greeter,
                          MdmSimpleSlave    *slave)
{
        g_debug ("MdmSimpleSlave: Greeter started");
}

static void
on_greeter_session_stop (MdmGreeterSession *greeter,
                         MdmSimpleSlave    *slave)
{
        g_debug ("MdmSimpleSlave: Greeter stopped");
        mdm_slave_stopped (MDM_SLAVE (slave));
}

static void
on_greeter_session_exited (MdmGreeterSession    *greeter,
                           int                   code,
                           MdmSimpleSlave       *slave)
{
        g_debug ("MdmSimpleSlave: Greeter exited: %d", code);
        mdm_slave_stopped (MDM_SLAVE (slave));
}

static void
on_greeter_session_died (MdmGreeterSession    *greeter,
                         int                   signal,
                         MdmSimpleSlave       *slave)
{
        g_debug ("MdmSimpleSlave: Greeter died: %d", signal);
        mdm_slave_stopped (MDM_SLAVE (slave));
}

static void
on_greeter_begin_verification (MdmGreeterServer *greeter_server,
                               MdmSimpleSlave   *slave)
{
        g_debug ("MdmSimpleSlave: begin verification");
        mdm_session_setup (MDM_SESSION (slave->priv->session),
                           "mdm");
}

static void
on_greeter_begin_auto_login (MdmGreeterServer *greeter_server,
                             const char       *username,
                             MdmSimpleSlave   *slave)
{
        g_debug ("MdmSimpleSlave: begin auto login for user '%s'", username);
        mdm_session_setup_for_user (MDM_SESSION (slave->priv->session),
                                    "mdm-autologin",
                                    username);
}

static void
on_greeter_begin_verification_for_user (MdmGreeterServer *greeter_server,
                                        const char       *username,
                                        MdmSimpleSlave   *slave)
{
        g_debug ("MdmSimpleSlave: begin verification");
        mdm_session_setup_for_user (MDM_SESSION (slave->priv->session),
                                    "mdm",
                                    username);
}

static void
on_greeter_answer (MdmGreeterServer *greeter_server,
                   const char       *text,
                   MdmSimpleSlave   *slave)
{
        mdm_session_answer_query (MDM_SESSION (slave->priv->session), text);
}

static void
on_greeter_session_selected (MdmGreeterServer *greeter_server,
                             const char       *text,
                             MdmSimpleSlave   *slave)
{
        mdm_session_select_session (MDM_SESSION (slave->priv->session), text);
}

static void
on_greeter_language_selected (MdmGreeterServer *greeter_server,
                              const char       *text,
                              MdmSimpleSlave   *slave)
{
        mdm_session_select_language (MDM_SESSION (slave->priv->session), text);
}

static void
on_greeter_layout_selected (MdmGreeterServer *greeter_server,
                            const char       *text,
                            MdmSimpleSlave   *slave)
{
        mdm_session_select_layout (MDM_SESSION (slave->priv->session), text);
}

static void
on_greeter_user_selected (MdmGreeterServer *greeter_server,
                          const char       *text,
                          MdmSimpleSlave   *slave)
{
        g_debug ("MdmSimpleSlave: Greeter user selected");
}

static void
on_greeter_cancel (MdmGreeterServer *greeter_server,
                   MdmSimpleSlave   *slave)
{
        g_debug ("MdmSimpleSlave: Greeter cancelled");
        queue_greeter_reset (slave);
}

static void
on_greeter_connected (MdmGreeterServer *greeter_server,
                      MdmSimpleSlave   *slave)
{
        gboolean display_is_local;

        g_debug ("MdmSimpleSlave: Greeter connected");

        mdm_session_start_conversation (MDM_SESSION (slave->priv->session));

        g_object_get (slave,
                      "display-is-local", &display_is_local,
                      NULL);

        /* If XDMCP stop pinging */
        if ( ! display_is_local) {
                alarm (0);
        }
}

static void
on_start_session_when_ready (MdmGreeterServer *session,
                             MdmSimpleSlave   *slave)
{
        g_debug ("MdmSimpleSlave: Will start session when ready");
        slave->priv->start_session_when_ready = TRUE;

        if (slave->priv->waiting_to_start_session) {
                mdm_simple_slave_accredit_when_ready (slave);
        }
}

static void
on_start_session_later (MdmGreeterServer *session,
                        MdmSimpleSlave   *slave)
{
        g_debug ("MdmSimpleSlave: Will start session when ready and told");
        slave->priv->start_session_when_ready = FALSE;
}

static void
setup_server (MdmSimpleSlave *slave)
{
        /* Set the busy cursor */
        mdm_slave_set_busy_cursor (MDM_SLAVE (slave));
}

static void
start_greeter (MdmSimpleSlave *slave)
{
        gboolean       display_is_local;
        char          *display_id;
        char          *display_name;
        char          *seat_id;
        char          *display_device;
        char          *display_hostname;
        char          *auth_file;
        char          *address;
        gboolean       res;

        g_debug ("MdmSimpleSlave: Running greeter");

        display_is_local = FALSE;
        display_id = NULL;
        display_name = NULL;
        seat_id = NULL;
        auth_file = NULL;
        display_device = NULL;
        display_hostname = NULL;

        g_object_get (slave,
                      "display-id", &display_id,
                      "display-is-local", &display_is_local,
                      "display-name", &display_name,
                      "display-seat-id", &seat_id,
                      "display-hostname", &display_hostname,
                      "display-x11-authority-file", &auth_file,
                      NULL);

        g_debug ("MdmSimpleSlave: Creating greeter for %s %s", display_name, display_hostname);

        if (slave->priv->server != NULL) {
                display_device = mdm_server_get_display_device (slave->priv->server);
        }

        /* FIXME: send a signal back to the master */

        /* If XDMCP setup pinging */
        slave->priv->ping_interval = DEFAULT_PING_INTERVAL;
        res = mdm_settings_direct_get_int (MDM_KEY_PING_INTERVAL,
                                           &(slave->priv->ping_interval));

        if ( ! display_is_local && slave->priv->ping_interval > 0) {
                alarm (slave->priv->ping_interval);
        }

        /* Run the init script. mdmslave suspends until script has terminated */
        mdm_slave_run_script (MDM_SLAVE (slave), MDMCONFDIR "/Init", MDM_USERNAME);

        slave->priv->greeter_server = mdm_greeter_server_new (display_id);
        g_signal_connect (slave->priv->greeter_server,
                          "begin-auto-login",
                          G_CALLBACK (on_greeter_begin_auto_login),
                          slave);
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
        g_signal_connect (slave->priv->greeter_server,
                          "start-session-when-ready",
                          G_CALLBACK (on_start_session_when_ready),
                          slave);

        g_signal_connect (slave->priv->greeter_server,
                          "start-session-later",
                          G_CALLBACK (on_start_session_later),
                          slave);

        mdm_greeter_server_start (slave->priv->greeter_server);

        g_debug ("MdmSimpleSlave: Creating greeter on %s %s %s", display_name, display_device, display_hostname);
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

        address = mdm_greeter_server_get_address (slave->priv->greeter_server);
        mdm_welcome_session_set_server_address (MDM_WELCOME_SESSION (slave->priv->greeter), address);
        g_free (address);
        mdm_welcome_session_start (MDM_WELCOME_SESSION (slave->priv->greeter));

        g_free (display_id);
        g_free (display_name);
        g_free (seat_id);
        g_free (display_device);
        g_free (display_hostname);
        g_free (auth_file);
}

static gboolean
idle_connect_to_display (MdmSimpleSlave *slave)
{
        gboolean res;

        slave->priv->connection_attempts++;

        res = mdm_slave_connect_to_x11_display (MDM_SLAVE (slave));
        if (res) {
                gboolean enabled;
                int      delay;

                /* FIXME: handle wait-for-go */

                setup_server (slave);

                delay = 0;
                enabled = FALSE;
                mdm_slave_get_timed_login_details (MDM_SLAVE (slave), &enabled, NULL, &delay);
                if (! enabled || delay > 0) {
                        start_greeter (slave);
                        create_new_session (slave);
                } else {
                        /* Run the init script. mdmslave suspends until script has terminated */
                        mdm_slave_run_script (MDM_SLAVE (slave), MDMCONFDIR "/Init", MDM_USERNAME);
                        reset_session (slave);
                }
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
on_server_ready (MdmServer      *server,
                 MdmSimpleSlave *slave)
{
        g_idle_add ((GSourceFunc)idle_connect_to_display, slave);
}

static void
on_server_exited (MdmServer      *server,
                  int             exit_code,
                  MdmSimpleSlave *slave)
{
        g_debug ("MdmSimpleSlave: server exited with code %d\n", exit_code);

        mdm_slave_stopped (MDM_SLAVE (slave));
}

static void
on_server_died (MdmServer      *server,
                int             signal_number,
                MdmSimpleSlave *slave)
{
        g_debug ("MdmSimpleSlave: server died with signal %d, (%s)",
                 signal_number,
                 g_strsignal (signal_number));

        mdm_slave_stopped (MDM_SLAVE (slave));
}

static gboolean
mdm_simple_slave_run (MdmSimpleSlave *slave)
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
                gboolean disable_tcp;

                slave->priv->server = mdm_server_new (display_name, auth_file);

                disable_tcp = TRUE;
                if (mdm_settings_client_get_boolean (MDM_KEY_DISALLOW_TCP,
                                                     &disable_tcp)) {
                        g_object_set (slave->priv->server,
                                      "disable-tcp", disable_tcp,
                                      NULL);
                }

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

                g_debug ("MdmSimpleSlave: Started X server");
        } else {
                g_timeout_add (500, (GSourceFunc)idle_connect_to_display, slave);
        }

        g_free (display_name);
        g_free (auth_file);

        return TRUE;
}

static gboolean
mdm_simple_slave_start (MdmSlave *slave)
{
        MDM_SLAVE_CLASS (mdm_simple_slave_parent_class)->start (slave);

        mdm_simple_slave_run (MDM_SIMPLE_SLAVE (slave));

        return TRUE;
}

static gboolean
mdm_simple_slave_stop (MdmSlave *slave)
{
        g_debug ("MdmSimpleSlave: Stopping simple_slave");

        MDM_SLAVE_CLASS (mdm_simple_slave_parent_class)->stop (slave);

        if (MDM_SIMPLE_SLAVE (slave)->priv->greeter != NULL) {
                stop_greeter (MDM_SIMPLE_SLAVE (slave));
        }

        if (MDM_SIMPLE_SLAVE (slave)->priv->session != NULL) {
                char *username;

                /* Run the PostSession script. mdmslave suspends until script
                 * has terminated
                 */
                username = mdm_session_direct_get_username (MDM_SIMPLE_SLAVE (slave)->priv->session);
                if (username != NULL) {
                        mdm_slave_run_script (MDM_SLAVE (slave), MDMCONFDIR "/PostSession", username);
                }
                g_free (username);

#ifdef  HAVE_LOGINDEVPERM
                mdm_simple_slave_revoke_console_permissions (MDM_SIMPLE_SLAVE (slave));
#endif

                mdm_session_close (MDM_SESSION (MDM_SIMPLE_SLAVE (slave)->priv->session));
                g_object_unref (MDM_SIMPLE_SLAVE (slave)->priv->session);
                MDM_SIMPLE_SLAVE (slave)->priv->session = NULL;
        }

        if (MDM_SIMPLE_SLAVE (slave)->priv->server != NULL) {
                mdm_server_stop (MDM_SIMPLE_SLAVE (slave)->priv->server);
                g_object_unref (MDM_SIMPLE_SLAVE (slave)->priv->server);
                MDM_SIMPLE_SLAVE (slave)->priv->server = NULL;
        }

        return TRUE;
}

static void
mdm_simple_slave_set_property (GObject      *object,
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
mdm_simple_slave_get_property (GObject    *object,
                               guint       prop_id,
                               GValue      *value,
                               GParamSpec *pspec)
{
        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static GObject *
mdm_simple_slave_constructor (GType                  type,
                              guint                  n_construct_properties,
                              GObjectConstructParam *construct_properties)
{
        MdmSimpleSlave      *simple_slave;

        simple_slave = MDM_SIMPLE_SLAVE (G_OBJECT_CLASS (mdm_simple_slave_parent_class)->constructor (type,
                                                                                 n_construct_properties,
                                                                                 construct_properties));

        return G_OBJECT (simple_slave);
}

static void
mdm_simple_slave_class_init (MdmSimpleSlaveClass *klass)
{
        GObjectClass  *object_class = G_OBJECT_CLASS (klass);
        MdmSlaveClass *slave_class = MDM_SLAVE_CLASS (klass);

        object_class->get_property = mdm_simple_slave_get_property;
        object_class->set_property = mdm_simple_slave_set_property;
        object_class->constructor = mdm_simple_slave_constructor;
        object_class->finalize = mdm_simple_slave_finalize;

        slave_class->start = mdm_simple_slave_start;
        slave_class->stop = mdm_simple_slave_stop;

        g_type_class_add_private (klass, sizeof (MdmSimpleSlavePrivate));

        dbus_g_object_type_install_info (MDM_TYPE_SIMPLE_SLAVE, &dbus_glib_mdm_simple_slave_object_info);
}

static void
mdm_simple_slave_init (MdmSimpleSlave *slave)
{
        slave->priv = MDM_SIMPLE_SLAVE_GET_PRIVATE (slave);
#ifdef  HAVE_LOGINDEVPERM
        slave->priv->use_logindevperm = FALSE;
#endif
}

static void
mdm_simple_slave_finalize (GObject *object)
{
        MdmSimpleSlave *simple_slave;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_SIMPLE_SLAVE (object));

        simple_slave = MDM_SIMPLE_SLAVE (object);

        g_return_if_fail (simple_slave->priv != NULL);

        mdm_simple_slave_stop (MDM_SLAVE (simple_slave));

        if (simple_slave->priv->greeter_reset_id > 0) {
                g_source_remove (simple_slave->priv->greeter_reset_id);
                simple_slave->priv->greeter_reset_id = 0;
        }

        G_OBJECT_CLASS (mdm_simple_slave_parent_class)->finalize (object);
}

MdmSlave *
mdm_simple_slave_new (const char *id)
{
        GObject *object;

        object = g_object_new (MDM_TYPE_SIMPLE_SLAVE,
                               "display-id", id,
                               NULL);

        return MDM_SLAVE (object);
}
