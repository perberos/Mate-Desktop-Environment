/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 David Zeuthen <david@fubar.dk>
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <glib-object.h>
#include <gdk/gdkx.h>
#include <mateconf/mateconf-client.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <polkit/polkit.h>
#include <polkit-dbus/polkit-dbus.h>
#include <polkit-grant/polkit-grant.h>

#include "polkit-mate-manager.h"
#include "polkit-mate-manager-glue.h"

#include "polkit-mate-auth-dialog.h"

#define KEY_AUTH_DIALOG_GRAB_KEYBOARD "/desktop/mate/policykit/auth_dialog_grab_keyboard"

static void do_cancel_auth (void);


static guint kill_timer_id = 0;

static gboolean kill_timer_no_exit = FALSE;

static gboolean
kill_timer_exit (gpointer user_data)
{
        g_debug ("Exiting because of kill timer.");
        exit (0);
        return FALSE;
}

static void
kill_timer_stop (void)
{

        if (kill_timer_id > 0) {
                g_debug ("Removing kill timer.");
                g_source_remove (kill_timer_id);
                kill_timer_id = 0;
        }
}

static void
kill_timer_start (void)
{
        if (kill_timer_no_exit)
                return;

        kill_timer_stop ();
        g_debug ("Setting kill timer to 30 seconds.");
        kill_timer_id = g_timeout_add (30 * 1000, kill_timer_exit, NULL);
}

static gboolean
do_polkit_auth (PolKitContext *pk_context,
                DBusConnection *system_bus_connection,
                const char *caller_exe_name,
                const char *action_id, 
                guint32     xid,
                pid_t       caller_pid);

struct PolKitMateManagerPrivate
{
        DBusGConnection *session_bus_connection;
        DBusGProxy      *session_bus_proxy;

        DBusGConnection *system_bus_connection;

        PolKitContext   *pk_context;

        gboolean         auth_in_progress;
        char            *session_bus_unique_name_of_client;
};

static void     polkit_mate_manager_class_init  (PolKitMateManagerClass *klass);
static void     polkit_mate_manager_init        (PolKitMateManager      *seat);
static void     polkit_mate_manager_finalize    (GObject     *object);

G_DEFINE_TYPE (PolKitMateManager, polkit_mate_manager, G_TYPE_OBJECT)

#define POLKIT_MATE_MANAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), POLKIT_MATE_TYPE_MANAGER, PolKitMateManagerPrivate))

GQuark
polkit_mate_manager_error_quark (void)
{
        static GQuark ret = 0;

#if 0
	if (etype == 0) {
		static const GEnumValue values[] = {
			ENUM_ENTRY (POLKIT_MANAGER_ERROR_NO_SUCH_USER, "NoSuchUser"),
			ENUM_ENTRY (POLKIT_MANAGER_ERROR_NO_SUCH_PRIVILEGE, "NoSuchPrivilege"),
			ENUM_ENTRY (POLKIT_MANAGER_ERROR_NOT_PRIVILEGED, "NotPrivileged"),
			ENUM_ENTRY (POLKIT_MANAGER_ERROR_CANNOT_OBTAIN_PRIVILEGE, "CannotObtainPrivilege"),
			ENUM_ENTRY (POLKIT_MANAGER_ERROR_ERROR, "Error"),
			{ 0, 0, 0 }
		};
		
		g_assert (POLKIT_MANAGER_NUM_ERRORS == G_N_ELEMENTS (values) - 1);
		
		etype = g_enum_register_static ("PolkitManagerError", values);
	}
#endif

        if (ret == 0) {
                ret = g_quark_from_static_string ("polkit_mate_manager_error");
        }

        return ret;
}

static GObject *
polkit_mate_manager_constructor (GType                  type,
                                  guint                  n_construct_properties,
                                  GObjectConstructParam *construct_properties)
{
        PolKitMateManager      *manager;
        PolKitMateManagerClass *klass;

        klass = POLKIT_MATE_MANAGER_CLASS (g_type_class_peek (POLKIT_MATE_TYPE_MANAGER));

        manager = POLKIT_MATE_MANAGER (G_OBJECT_CLASS (polkit_mate_manager_parent_class)->constructor (
                                                type,
                                                n_construct_properties,
                                                construct_properties));

        return G_OBJECT (manager);
}

static void
polkit_mate_manager_class_init (PolKitMateManagerClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->constructor = polkit_mate_manager_constructor;
        object_class->finalize = polkit_mate_manager_finalize;

        g_type_class_add_private (klass, sizeof (PolKitMateManagerPrivate));

        dbus_g_object_type_install_info (POLKIT_MATE_TYPE_MANAGER, &dbus_glib_polkit_mate_manager_object_info);
}

static void
polkit_mate_manager_init (PolKitMateManager *manager)
{
        manager->priv = POLKIT_MATE_MANAGER_GET_PRIVATE (manager);

}

static void
polkit_mate_manager_finalize (GObject *object)
{
        PolKitMateManager *manager;

        g_return_if_fail (object != NULL);
        g_return_if_fail (POLKIT_MATE_IS_MANAGER (object));

        manager = POLKIT_MATE_MANAGER (object);

        g_return_if_fail (manager->priv != NULL);

        g_object_unref (manager->priv->session_bus_proxy);

        G_OBJECT_CLASS (polkit_mate_manager_parent_class)->finalize (object);
}


static void
_polkit_config_changed_cb (PolKitContext *pk_context, gpointer user_data)
{
        g_debug ("PolicyKit reports that the config have changed");
}


static void
session_bus_name_owner_changed (DBusGProxy  *bus_proxy,
                                const char  *service_name,
                                const char  *old_service_name,
                                const char  *new_service_name,
                                gpointer     user_data)
{
        PolKitMateManager *manager = user_data;

        if (strlen (new_service_name) == 0 &&
            manager->priv->auth_in_progress &&
            manager->priv->session_bus_unique_name_of_client != NULL &&
            strcmp (manager->priv->session_bus_unique_name_of_client, old_service_name) == 0) {
                do_cancel_auth ();
        }

}


static gboolean
pk_io_watch_have_data (GIOChannel *channel, GIOCondition condition, gpointer user_data)
{
        int fd;
        PolKitContext *pk_context = user_data;
        fd = g_io_channel_unix_get_fd (channel);
        polkit_context_io_func (pk_context, fd);
        return TRUE;
}

static int 
pk_io_add_watch (PolKitContext *pk_context, int fd)
{
        guint id = 0;
        GIOChannel *channel;
        channel = g_io_channel_unix_new (fd);
        if (channel == NULL)
                goto out;
        id = g_io_add_watch (channel, G_IO_IN, pk_io_watch_have_data, pk_context);
        if (id == 0) {
                g_io_channel_unref (channel);
                goto out;
        }
        g_io_channel_unref (channel);
out:
        return id;
}

static void 
pk_io_remove_watch (PolKitContext *pk_context, int watch_id)
{
        g_source_remove (watch_id);
}

static gboolean
register_manager (PolKitMateManager *manager)
{
        GError *error = NULL;
        PolKitError *pk_error;

        error = NULL;
        manager->priv->session_bus_connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
        if (manager->priv->session_bus_connection == NULL) {
                if (error != NULL) {
                        g_critical ("error getting session bus: %s", error->message);
                        g_error_free (error);
                }
                goto error;
        }

        manager->priv->system_bus_connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
        if (manager->priv->system_bus_connection == NULL) {
                if (error != NULL) {
                        g_critical ("error getting system bus: %s", error->message);
                        g_error_free (error);
                }
                goto error;
        }

        dbus_g_connection_register_g_object (manager->priv->session_bus_connection, 
                                             "/org/mate/PolicyKit/Manager", 
                                             G_OBJECT (manager));

        dbus_g_connection_register_g_object (manager->priv->session_bus_connection, 
                                             "/", 
                                             G_OBJECT (manager));

        manager->priv->session_bus_proxy = dbus_g_proxy_new_for_name (manager->priv->session_bus_connection,
                                                                      DBUS_SERVICE_DBUS,
                                                                      DBUS_PATH_DBUS,
                                                                      DBUS_INTERFACE_DBUS);

        dbus_g_proxy_add_signal (manager->priv->session_bus_proxy,
                                 "NameOwnerChanged",
                                 G_TYPE_STRING,
                                 G_TYPE_STRING,
                                 G_TYPE_STRING,
                                 G_TYPE_INVALID);
        dbus_g_proxy_connect_signal (manager->priv->session_bus_proxy,
                                     "NameOwnerChanged",
                                     G_CALLBACK (session_bus_name_owner_changed),
                                     manager,
                                     NULL);

        manager->priv->pk_context = polkit_context_new ();
        if (manager->priv->pk_context == NULL) {
                g_critical ("error creating PK context");
                goto error;
        }
        polkit_context_set_load_descriptions (manager->priv->pk_context);
        polkit_context_set_config_changed (manager->priv->pk_context,
                                           _polkit_config_changed_cb,
                                           NULL);
        polkit_context_set_io_watch_functions (manager->priv->pk_context, pk_io_add_watch, pk_io_remove_watch);
        pk_error = NULL;
        if (!polkit_context_init (manager->priv->pk_context, &pk_error)) {
                g_critical ("error creating PK context: %s", polkit_error_get_error_message (pk_error));
                polkit_error_free (pk_error);
                goto error;
        }

        return TRUE;

error:
        return FALSE;
}


PolKitMateManager *
polkit_mate_manager_new (gboolean no_exit)
{
        GObject *object;
        gboolean res;

        object = g_object_new (POLKIT_MATE_TYPE_MANAGER, NULL);

        res = register_manager (POLKIT_MATE_MANAGER (object));
        if (! res) {
                g_object_unref (object);
                return NULL;
        }

        kill_timer_no_exit = no_exit;
        kill_timer_start ();

        return POLKIT_MATE_MANAGER (object);
}

/* TODO: this is right now Linux specific */

static char *
get_exe_for_pid (pid_t pid)
{
        char *result;
        char buf[PATH_MAX];

        if (polkit_sysdeps_get_exe_for_pid_with_helper (pid, buf, sizeof buf) < 0) {
                result = g_strdup (_("(unknown"));
        } else {
                result =  g_strdup (buf);
        }

        return result;
}

typedef struct
{
        PolKitMateManager    *manager;
        char                  *action_id;
        guint32                xid;
        pid_t                  pid;
        DBusGMethodInvocation *context;
} WorkClosure;

static gboolean
do_auth (gpointer user_data)
{
        WorkClosure *wc = user_data;
        const char *sender;
        pid_t caller_pid;
        char *caller_exe;
        gboolean gained_privilege;
        GError *error;

        kill_timer_stop ();

        if (wc->manager->priv->auth_in_progress) {
                error = g_error_new (POLKIT_MATE_MANAGER_ERROR,
                                     POLKIT_MATE_MANAGER_ERROR_AUTH_IN_PROGRESS,
                                     "Another client is already authenticating. Please try again later.");
                dbus_g_method_return_error (wc->context, error);
                g_error_free (error);
                goto out;
        }

        g_debug ("in show_dialog action_id='%s'", wc->action_id);

        sender = dbus_g_method_get_sender (wc->context);

        if (wc->pid == -1) {
                error = NULL;
                if (! dbus_g_proxy_call (wc->manager->priv->session_bus_proxy, "GetConnectionUnixProcessID", &error,
                                         G_TYPE_STRING, sender,
                                         G_TYPE_INVALID,
                                         G_TYPE_UINT, &caller_pid,
                                         G_TYPE_INVALID)) {
                        g_debug ("GetConnectionUnixProcessID() failed: %s", error->message);
                        g_error_free (error);
                        
                        error = g_error_new (POLKIT_MATE_MANAGER_ERROR,
                                             POLKIT_MATE_MANAGER_ERROR_GENERAL,
                                             "Unable to lookup process ID for caller");
                        dbus_g_method_return_error (wc->context, error);
                        g_error_free (error);
                        goto out;
                }
        } else {
                caller_pid = wc->pid;
        }

        g_debug ("process id = %d", caller_pid);

        caller_exe = get_exe_for_pid (caller_pid);
        if (caller_exe == NULL) {
                caller_exe = g_strdup ("(unknown)");
#if 0
                error = g_error_new (POLKIT_MATE_MANAGER_ERROR,
                                     POLKIT_MATE_MANAGER_ERROR_GENERAL,
                                     "Unable to lookup exe for caller");
                dbus_g_method_return_error (wc->context, error);
                g_error_free (error);
                goto out;
#endif
        }

        g_debug ("exe = '%s'", caller_exe);

        wc->manager->priv->auth_in_progress = TRUE;
        wc->manager->priv->session_bus_unique_name_of_client = g_strdup (sender);

        gained_privilege = do_polkit_auth (wc->manager->priv->pk_context,
                                           dbus_g_connection_get_connection (wc->manager->priv->system_bus_connection),
                                           caller_exe,
                                           wc->action_id,
                                           wc->xid,
                                           caller_pid);

        g_free (caller_exe);

        dbus_g_method_return (wc->context, gained_privilege);

        wc->manager->priv->auth_in_progress = FALSE;
        g_free (wc->manager->priv->session_bus_unique_name_of_client);
        wc->manager->priv->session_bus_unique_name_of_client = NULL;
out:
        kill_timer_start ();
        g_free (wc->action_id);
        g_free (wc);
        return FALSE;
}

/* exported methods */


gboolean
polkit_mate_manager_obtain_authorization (PolKitMateManager    *manager,
                                           const char            *action_id,
                                           guint32                xid,
                                           guint32                pid,
                                           DBusGMethodInvocation *context)
{
        WorkClosure *wc;

        wc = g_new0 (WorkClosure, 1);
        wc->manager = manager;
        wc->action_id = g_strdup (action_id);
        wc->xid = xid;
        wc->context = context;
        wc->pid = pid;

        g_idle_add (do_auth, wc);
        return TRUE;
}

gboolean
polkit_mate_manager_show_dialog (PolKitMateManager    *manager,
                                  const char            *action_id,
                                  guint32                xid,
                                  DBusGMethodInvocation *context)
{
        return polkit_mate_manager_obtain_authorization (manager, action_id, xid, -1, context);
}


/*----------------------------------------------------------------------------------------------------*/

typedef struct {
        GMainLoop *loop;
        GtkWidget *dialog;
        PolKitGrant *polkit_grant;

        gboolean gained_privilege;
        gboolean was_cancelled;
        gboolean was_bogus;
        gboolean new_user_selected;

        gboolean remember_session;
        gboolean remember_always;
        gboolean requires_admin;
        char **admin_users;

        char *admin_user_selected;

        gboolean got_keyboard_grab;
} UserData;

static void
conversation_type (PolKitGrant *polkit_grant, PolKitResult auth_type, void *user_data)
{
        UserData *ud = user_data;

        ud->remember_session = FALSE;
        ud->remember_always = FALSE;
        ud->requires_admin = FALSE;

        switch (auth_type) {
        case POLKIT_RESULT_ONLY_VIA_ADMIN_AUTH_ONE_SHOT:
        case POLKIT_RESULT_ONLY_VIA_ADMIN_AUTH:
                ud->requires_admin = TRUE;
                break;

        case POLKIT_RESULT_ONLY_VIA_ADMIN_AUTH_KEEP_SESSION:
                ud->requires_admin = TRUE;
                ud->remember_session = TRUE;
                break;

        case POLKIT_RESULT_ONLY_VIA_ADMIN_AUTH_KEEP_ALWAYS:
                ud->requires_admin = TRUE;
                ud->remember_always = TRUE;
                break;

        case POLKIT_RESULT_ONLY_VIA_SELF_AUTH_ONE_SHOT:
        case POLKIT_RESULT_ONLY_VIA_SELF_AUTH:
                break;

        case POLKIT_RESULT_ONLY_VIA_SELF_AUTH_KEEP_SESSION:
                ud->remember_session = TRUE;
                break;

        case POLKIT_RESULT_ONLY_VIA_SELF_AUTH_KEEP_ALWAYS:
                ud->remember_always = TRUE;
                break;
        default:
                g_assert_not_reached ();
                break;
        }

        polkit_mate_auth_dialog_set_options (POLKIT_MATE_AUTH_DIALOG (ud->dialog), 
                                              ud->remember_session,
                                              ud->remember_always,
                                              ud->requires_admin,
                                              ud->admin_users);
}

/*--------------------------------------------------------------------------------------------------------------*/

static void
_do_grab (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
        UserData *ud = user_data;
        MateConfClient *client;

        ud->got_keyboard_grab = FALSE;

        client = mateconf_client_get_default ();
        if (mateconf_client_get_bool (client, KEY_AUTH_DIALOG_GRAB_KEYBOARD, NULL)) { /* NULL-GError */
                GdkGrabStatus ret;

                ret = gdk_keyboard_grab (widget->window, 
                                         FALSE,
                                         GDK_CURRENT_TIME); /* FIXME: ideally we need a real timestamp */
                if (ret != GDK_GRAB_SUCCESS) {
                        g_warning ("Couldn't grab the keyboard; ret = %d", ret);
                } else {
                        g_debug ("Grabbed keyboard");
                        ud->got_keyboard_grab = TRUE;
                }
        }
}

/*--------------------------------------------------------------------------------------------------------------*/

static void 
dialog_response (GtkDialog *dialog, gint response_id, gpointer user_data)
{
        UserData *ud = user_data;

        g_debug ("dialog response %d", response_id);
        if (ud->admin_user_selected == NULL)
                ud->admin_user_selected = g_strdup ("");
}

static void
user_selected (PolkitMateAuthDialog *auth_dialog, const char *user_name, gpointer user_data)
{
        UserData *ud = user_data;

        g_debug ("In user_selected, user_name=%s", user_name);

        if (ud->admin_user_selected == NULL) {
                /* happens when we're invoked from conversation_select_admin_user() */
                ud->admin_user_selected = g_strdup (user_name);
        } else {
                g_debug ("Restart auth as new user...");
                g_free (ud->admin_user_selected);
                ud->admin_user_selected = g_strdup (user_name);
                ud->new_user_selected = TRUE;
                polkit_grant_cancel_auth (ud->polkit_grant);
                /* break out of gtk_dialog_run() ... */
                gtk_dialog_response (GTK_DIALOG (ud->dialog), GTK_RESPONSE_OK);
        }
}

static void
do_show_dialog (UserData *ud)
{
        guint32 server_time;
        gtk_widget_realize (ud->dialog);
        server_time = gdk_x11_get_server_time (ud->dialog->window);
        gtk_window_present_with_time (GTK_WINDOW (ud->dialog), server_time);
        gtk_widget_show_all (ud->dialog);
}

static char *
conversation_select_admin_user (PolKitGrant *polkit_grant, char **admin_users, void *user_data)
{
        UserData *ud = user_data;
        gulong response_id;
        char *result;
        int n;
        const char *user_name;

        /* if we've already selected the admin user.. then reuse the same one (this
         * is mainly when the user entered the wrong password)
         */
        if (ud->admin_user_selected != NULL) {
                result = strdup (ud->admin_user_selected);
                goto out;
        }

        ud->admin_users = g_strdupv (admin_users);

        /* show the auth dialog with the given admin users */
        polkit_mate_auth_dialog_set_options (POLKIT_MATE_AUTH_DIALOG (ud->dialog), 
                                              ud->remember_session,
                                              ud->remember_always,
                                              ud->requires_admin,
                                              ud->admin_users);

        /* if we are running as one of the users in admin_users then preselect that user... */
        user_name = g_get_user_name ();
        if (user_name != NULL) {
                for (n = 0; admin_users[n] != NULL; n++) {
                        if (strcmp (user_name, admin_users[n]) == 0) {
                                ud->admin_user_selected = g_strdup (user_name);
                                result = strdup (ud->admin_user_selected);

                                polkit_mate_auth_dialog_select_admin_user (POLKIT_MATE_AUTH_DIALOG (ud->dialog), 
                                                                            ud->admin_user_selected);

                                g_debug ("Preselecting ourselves as admin_user");
                                goto out;
                        }
                }
        }

        polkit_mate_auth_dialog_set_prompt (POLKIT_MATE_AUTH_DIALOG (ud->dialog), _("_Password:"), FALSE);
	response_id = g_signal_connect (GTK_WIDGET (ud->dialog), "response", G_CALLBACK (dialog_response), ud);

        do_show_dialog (ud);

        /* run the mainloop waiting for the user to be selected */
        while (ud->admin_user_selected == NULL)
                g_main_context_iteration (g_main_loop_get_context (ud->loop), TRUE);

        g_signal_handler_disconnect (GTK_WIDGET (ud->dialog), response_id);

        /* if admin_user_selected is the empty string.. it means the dialog was
         * cancelled (see dialog_response() above) 
         */
        if (strcmp (ud->admin_user_selected, "") == 0) {
                ud->was_cancelled = TRUE;
                polkit_grant_cancel_auth (polkit_grant);
                result = NULL;
        } else {
                result = strdup (ud->admin_user_selected);
        }
out:
        return result;
}

/*--------------------------------------------------------------------------------------------------------------*/

static char *
conversation_pam_prompt (PolKitGrant *polkit_grant, const char *request, void *user_data, gboolean echo_on)
{
        UserData *ud = user_data;
        char *password;
        char *request2;
        int response;

        password = NULL;

        g_debug ("in conversation_pam_prompt, request='%s', echo_on=%d", request, echo_on);

        /* Fix up, and localize, password prompt if it's password auth */
        printf ("request: '%s'\n", request);
        if (g_ascii_strncasecmp (request, "password:", 9) == 0) {
                if (ud->requires_admin) {
                        if (ud->admin_user_selected != NULL) {
                                request2 = g_strdup_printf (_("_Password for %s:"), ud->admin_user_selected);
                        } else {
                                request2 = g_strdup (_("_Password for root:"));
                        }
                } else {
                        request2 = g_strdup (_("_Password:"));
                }
        } else if (g_ascii_strncasecmp (request, "password or swipe finger:", 25) == 0) {
                if (ud->requires_admin) {
                        if (ud->admin_user_selected != NULL) {
                                request2 = g_strdup_printf (_("_Password or swipe finger for %s:"), 
                                                            ud->admin_user_selected);
                        } else {
                                request2 = g_strdup (_("_Password or swipe finger for root:"));
                        }
                } else {
                        request2 = g_strdup (_("_Password or swipe finger:"));
                }
        } else {
                request2 = g_strdup (request);
        }

        polkit_mate_auth_dialog_set_prompt (POLKIT_MATE_AUTH_DIALOG (ud->dialog), request2, echo_on);
        g_free (request2);

        do_show_dialog (ud);

        response = gtk_dialog_run (GTK_DIALOG (ud->dialog));

        /* cancel auth unless user clicked "Authenticate" */
        if (response != GTK_RESPONSE_OK) {
                ud->was_cancelled = TRUE;
                polkit_grant_cancel_auth (polkit_grant);
                goto out;
        }

        password = strdup (polkit_mate_auth_dialog_get_password (POLKIT_MATE_AUTH_DIALOG (ud->dialog)));
out:
        return password;
}

static char *
conversation_pam_prompt_echo_off (PolKitGrant *polkit_grant, const char *request, void *user_data)
{
        return conversation_pam_prompt (polkit_grant, request, user_data, FALSE);
}

static char *
conversation_pam_prompt_echo_on (PolKitGrant *polkit_grant, const char *request, void *user_data)
{
        return conversation_pam_prompt (polkit_grant, request, user_data, TRUE);
}

static void
conversation_pam_error_msg (PolKitGrant *polkit_grant, const char *msg, void *user_data)
{
        g_debug ("error_msg='%s'", msg);
}

static void
conversation_pam_text_info (PolKitGrant *polkit_grant, const char *msg, void *user_data)
{
        g_debug ("text_info='%s'", msg);
}

static PolKitResult
conversation_override_grant_type (PolKitGrant *polkit_grant, PolKitResult auth_type, void *user_data)
{
        polkit_bool_t keep_session = FALSE;
        polkit_bool_t keep_always = FALSE;
        PolKitResult overridden_auth_type;
        UserData *ud = user_data;

        switch (auth_type) {
        case POLKIT_RESULT_ONLY_VIA_ADMIN_AUTH_ONE_SHOT:
        case POLKIT_RESULT_ONLY_VIA_SELF_AUTH_ONE_SHOT:
                break;
        case POLKIT_RESULT_ONLY_VIA_ADMIN_AUTH:
        case POLKIT_RESULT_ONLY_VIA_SELF_AUTH:
                break;
        case POLKIT_RESULT_ONLY_VIA_ADMIN_AUTH_KEEP_SESSION:
        case POLKIT_RESULT_ONLY_VIA_SELF_AUTH_KEEP_SESSION:
                keep_session = 
                        polkit_mate_auth_dialog_get_remember_session (POLKIT_MATE_AUTH_DIALOG (ud->dialog));
                break;
        case POLKIT_RESULT_ONLY_VIA_ADMIN_AUTH_KEEP_ALWAYS:
        case POLKIT_RESULT_ONLY_VIA_SELF_AUTH_KEEP_ALWAYS:
                keep_always = 
                        polkit_mate_auth_dialog_get_remember_always (POLKIT_MATE_AUTH_DIALOG (ud->dialog));
                keep_session = 
                        polkit_mate_auth_dialog_get_remember_session (POLKIT_MATE_AUTH_DIALOG (ud->dialog));
                break;
        default:
                g_assert_not_reached ();
        }

        g_debug ("keep_always = %d", keep_always);
        g_debug ("keep_session = %d", keep_session);

        switch (auth_type) {
        case POLKIT_RESULT_ONLY_VIA_ADMIN_AUTH_ONE_SHOT:
                overridden_auth_type = POLKIT_RESULT_ONLY_VIA_ADMIN_AUTH_ONE_SHOT;
                break;
        case POLKIT_RESULT_ONLY_VIA_ADMIN_AUTH:
        case POLKIT_RESULT_ONLY_VIA_ADMIN_AUTH_KEEP_SESSION:
        case POLKIT_RESULT_ONLY_VIA_ADMIN_AUTH_KEEP_ALWAYS:
                overridden_auth_type = POLKIT_RESULT_ONLY_VIA_ADMIN_AUTH;
                if (keep_session)
                        overridden_auth_type = POLKIT_RESULT_ONLY_VIA_ADMIN_AUTH_KEEP_SESSION;
                else if (keep_always)
                        overridden_auth_type = POLKIT_RESULT_ONLY_VIA_ADMIN_AUTH_KEEP_ALWAYS;
                break;

        case POLKIT_RESULT_ONLY_VIA_SELF_AUTH_ONE_SHOT:
                overridden_auth_type = POLKIT_RESULT_ONLY_VIA_SELF_AUTH_ONE_SHOT;
                break;
        case POLKIT_RESULT_ONLY_VIA_SELF_AUTH:
        case POLKIT_RESULT_ONLY_VIA_SELF_AUTH_KEEP_SESSION:
        case POLKIT_RESULT_ONLY_VIA_SELF_AUTH_KEEP_ALWAYS:
                overridden_auth_type = POLKIT_RESULT_ONLY_VIA_SELF_AUTH;
                if (keep_session)
                        overridden_auth_type = POLKIT_RESULT_ONLY_VIA_SELF_AUTH_KEEP_SESSION;
                else if (keep_always)
                        overridden_auth_type = POLKIT_RESULT_ONLY_VIA_SELF_AUTH_KEEP_ALWAYS;
                break;

        default:
                g_assert_not_reached ();
        }

        return overridden_auth_type;
}


static void 
conversation_done (PolKitGrant *polkit_grant, 
                   polkit_bool_t gained_privilege, 
                   polkit_bool_t input_was_bogus, 
                   void *user_data)
{
        UserData *ud = user_data;
        ud->gained_privilege = gained_privilege;
        ud->was_bogus = input_was_bogus;

        g_debug ("in conversation_done gained=%d, bogus=%d", gained_privilege, input_was_bogus);

        if ((ud->was_bogus || ud->was_cancelled) && ud->dialog != NULL) {
                gtk_widget_destroy (ud->dialog);
                ud->dialog = NULL;
        }

        g_main_loop_quit (ud->loop);
}

static void
child_watch_func (GPid pid,
                  gint status,
                  gpointer user_data)
{
        PolKitGrant *polkit_grant = user_data;
        polkit_grant_child_func (polkit_grant, pid, WEXITSTATUS (status));
}

static int
add_child_watch (PolKitGrant *polkit_grant, pid_t pid)
{
        return g_child_watch_add (pid, child_watch_func, polkit_grant);
}

static gboolean
io_watch_have_data (GIOChannel *channel, GIOCondition condition, gpointer user_data)
{
        int fd;
        PolKitGrant *polkit_grant = user_data;
        fd = g_io_channel_unix_get_fd (channel);
        polkit_grant_io_func (polkit_grant, fd);
        return TRUE;
}

static int
add_io_watch (PolKitGrant *polkit_grant, int fd)
{
        guint id = 0;
        GIOChannel *channel;
        channel = g_io_channel_unix_new (fd);
        if (channel == NULL)
                goto out;
        id = g_io_add_watch (channel, G_IO_IN, io_watch_have_data, polkit_grant);
        if (id == 0) {
                g_io_channel_unref (channel);
                goto out;
        }
        g_io_channel_unref (channel);
out:
        return id;
}

static void 
remove_watch (PolKitGrant *polkit_auth, int watch_id)
{
        g_source_remove (watch_id);
}

static PolKitGrant *grant = NULL;
static UserData *ud = NULL;

static void
add_to_blacklist (UserData *ud, const char *action_id)
{
        GSList *action_list, *l;
        MateConfClient *client;
        GError *error;
        gboolean never_retain_authorization;

        client = mateconf_client_get_default ();
	if (client == NULL) {
		g_warning ("Error getting MateConfClient");
		goto out;
	}

	error = NULL;
	never_retain_authorization = !mateconf_client_get_bool (client, KEY_AUTH_DIALOG_RETAIN_AUTHORIZATION, &error);
	if (error != NULL) {
		g_warning ("Error getting key %s: %s",
			   KEY_AUTH_DIALOG_RETAIN_AUTHORIZATION,
			   error->message);
		g_error_free (error);
		goto out;
	}

        if (never_retain_authorization)
                goto out;

        error = NULL;
        action_list = mateconf_client_get_list (client,
                                             KEY_AUTH_DIALOG_RETAIN_AUTHORIZATION_BLACKLIST,
                                             MATECONF_VALUE_STRING,
                                             &error);
        if (error != NULL) {
                g_warning ("Error getting key %s: %s",
                           KEY_AUTH_DIALOG_RETAIN_AUTHORIZATION_BLACKLIST,
                           error->message);
                g_error_free (error);
                goto out;
        }

        for (l = action_list; l != NULL; l = l->next) {
                const char *str = l->data;
                if (strcmp (str, action_id) == 0) {
                        /* already there */
                        goto done;
                }
        }

        action_list = g_slist_append (action_list, g_strdup (action_id));

        if (!mateconf_client_set_list (client,
                                    KEY_AUTH_DIALOG_RETAIN_AUTHORIZATION_BLACKLIST,
                                    MATECONF_VALUE_STRING,
                                    action_list,
                                    &error)) {
                g_warning ("Error setting key %s: %s",
                           KEY_AUTH_DIALOG_RETAIN_AUTHORIZATION_BLACKLIST,
                           error->message);
                g_error_free (error);
                error = NULL;
        }

done:
        g_slist_foreach (action_list, (GFunc) g_free, NULL);
        g_slist_free (action_list);

out:
        ;
}

static gboolean
do_polkit_auth (PolKitContext  *pk_context,
                DBusConnection *system_bus_connection,
                const char     *caller_exe_name,
                const char     *action_id, 
                guint32         xid,
                pid_t           caller_pid)
{
        PolKitAction *action = NULL;
        PolKitCaller *caller = NULL;
        DBusError error;
        gboolean ret;
        int num_tries;
        PolKitPolicyCache *pk_cache;
        PolKitPolicyFileEntry *pfe;
        const char *message_markup;
        const char *vendor;
        const char *vendor_url;
        const char *icon_name;

        ret = FALSE;

        ud = g_new0 (UserData, 1);

        ud->requires_admin = FALSE;

        action = polkit_action_new ();
        polkit_action_set_action_id (action, action_id);

        dbus_error_init (&error);
        caller = polkit_caller_new_from_pid (system_bus_connection, 
                                             caller_pid,
                                             &error);
        if (caller == NULL) {
                if (dbus_error_is_set (&error)) {
                        fprintf (stderr, "error: polkit_caller_new_from_pid(): %s: %s\n", 
                                 error.name, error.message);
                        goto error;
                }
        }

        ud->dialog = NULL;
        ud->loop = g_main_loop_new (NULL, TRUE);
        ud->gained_privilege = FALSE;

        pk_cache = polkit_context_get_policy_cache (pk_context);
        if (pk_cache == NULL) {
                g_warning ("Cannot load policy files");
                goto error;
        }
        pfe = polkit_policy_cache_get_entry (pk_cache, action);
        if (pfe == NULL) {
                g_warning ("No policy file entry for given action");
                goto error;
        }

        message_markup = polkit_policy_file_entry_get_action_message (pfe);
        if (message_markup == NULL) {
                g_warning ("No message markup for given action");
                goto error;
        }

        vendor = polkit_policy_file_entry_get_action_vendor (pfe);
        vendor_url = polkit_policy_file_entry_get_action_vendor_url (pfe);
        icon_name = polkit_policy_file_entry_get_action_icon_name (pfe);
        
        ud->dialog = polkit_mate_auth_dialog_new (
                caller_exe_name,
                action_id,
                vendor,
                vendor_url,
                icon_name,
                message_markup);

        /* XID zero is used to mean "universal null resource or null atom" ... so if the
         * caller is not an X11 client he can pass in 0 and we won't even attempt to
         * find an X11 window that we're transient for.
         */
        if (xid != 0) {
                GdkWindow *foreign_window;
                foreign_window = gdk_window_foreign_new (xid);
                if (foreign_window != NULL) {
                        gtk_widget_realize (ud->dialog);
                        gdk_window_set_transient_for (ud->dialog->window, foreign_window);
                }
        }

	g_signal_connect (GTK_WIDGET (ud->dialog), "user-selected", G_CALLBACK (user_selected), ud);

        /* grab the pointer and keyboard as soon as we are mapped */
        g_signal_connect(ud->dialog, "map_event", G_CALLBACK(_do_grab), ud);

        num_tries = 0;
try_again:
        grant = polkit_grant_new ();
        ud->polkit_grant = grant;
        polkit_grant_set_functions (grant,
                                    add_io_watch,
                                    add_child_watch,
                                    remove_watch,
                                    conversation_type,
                                    conversation_select_admin_user,
                                    conversation_pam_prompt_echo_off,
                                    conversation_pam_prompt_echo_on,
                                    conversation_pam_error_msg,
                                    conversation_pam_text_info,
                                    conversation_override_grant_type,
                                    conversation_done,
                                    ud);
        ud->was_cancelled = FALSE;
        ud->was_bogus = FALSE;
        ud->new_user_selected = FALSE;
        
        if (!polkit_grant_initiate_auth (grant,
                                         action,
                                         caller)) {
                g_warning ("Failed to initiate privilege grant.");
                ret = 1;
                goto error;
        }
        g_main_loop_run (ud->loop);

        if (ud->new_user_selected) {
                g_debug ("new user selected so restarting auth..");
                polkit_grant_unref (grant);
                grant = NULL;
                goto try_again;
        }

        num_tries++;

        g_debug ("gained_privilege=%d was_cancelled=%d was_bogus=%d.", 
                 ud->gained_privilege, ud->was_cancelled, ud->was_bogus);

        if (!ud->gained_privilege && !ud->was_cancelled && !ud->was_bogus) {
                if (ud->dialog != NULL) {

                        /* shake the dialog to indicate error */
                        polkit_mate_auth_dialog_indicate_auth_error (POLKIT_MATE_AUTH_DIALOG (ud->dialog));

                        if (num_tries < 3) {
                                polkit_grant_unref (grant);
                                grant = NULL;
                                goto try_again;
                        }
                }
        }

        if (ud->gained_privilege) {
                /* add to blacklist if the user unchecked the "remember authorization" check box */
                if ((ud->remember_always &&
                     !polkit_mate_auth_dialog_get_remember_always (POLKIT_MATE_AUTH_DIALOG (ud->dialog))) ||
                    (ud->remember_session &&
                     !polkit_mate_auth_dialog_get_remember_session (POLKIT_MATE_AUTH_DIALOG (ud->dialog)))) {
                        add_to_blacklist (ud, action_id);
                }
        }

        ret = ud->gained_privilege;

error:

        /* Ungrab keyboard and pointer */
        if (ud->got_keyboard_grab) {
                gdk_keyboard_ungrab (GDK_CURRENT_TIME);
                g_debug ("Ungrabbed keyboard");
        }

        if (ud->dialog != NULL) {
                gtk_widget_destroy (ud->dialog);
                ud->dialog = NULL;
        }
        if (ud->loop != NULL)
                g_main_loop_unref (ud->loop);
        if (ud->admin_users != NULL)
                g_strfreev (ud->admin_users);
        if (ud->admin_user_selected != NULL)
                g_free (ud->admin_user_selected);

        if (grant != NULL)
                polkit_grant_unref (grant);
        if (action != NULL)
                polkit_action_unref (action);
        if (caller != NULL)
                polkit_caller_unref (caller);

        ud = NULL;
        grant = NULL;

        return ret;
}

static void
do_cancel_auth (void)
{
        g_return_if_fail (ud != NULL);
        g_return_if_fail (grant != NULL);

        ud->was_cancelled = TRUE;
        polkit_grant_cancel_auth (grant);
}
