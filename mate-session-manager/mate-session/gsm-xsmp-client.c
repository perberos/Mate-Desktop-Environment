/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 Novell, Inc.
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
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#include <gio/gio.h>
#include <glib/gi18n.h>

#include "gsm-xsmp-client.h"
#include "gsm-marshal.h"

#include "gsm-util.h"
#include "gsm-autostart-app.h"
#include "gsm-manager.h"

#define GsmDesktopFile "_GSM_DesktopFile"

#define IS_STRING_EMPTY(x) ((x)==NULL||(x)[0]=='\0')

#define GSM_XSMP_CLIENT_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GSM_TYPE_XSMP_CLIENT, GsmXSMPClientPrivate))

struct GsmXSMPClientPrivate
{

        SmsConn    conn;
        IceConn    ice_connection;

        guint      watch_id;

        char      *description;
        GPtrArray *props;

        /* SaveYourself state */
        int        current_save_yourself;
        int        next_save_yourself;
        guint      next_save_yourself_allow_interact : 1;
};

enum {
        PROP_0,
        PROP_ICE_CONNECTION
};

enum {
        REGISTER_REQUEST,
        LOGOUT_REQUEST,
        LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (GsmXSMPClient, gsm_xsmp_client, GSM_TYPE_CLIENT)

static gboolean
client_iochannel_watch (GIOChannel    *channel,
                        GIOCondition   condition,
                        GsmXSMPClient *client)
{
        gboolean keep_going;

        g_object_ref (client);
        switch (IceProcessMessages (client->priv->ice_connection, NULL, NULL)) {
        case IceProcessMessagesSuccess:
                keep_going = TRUE;
                break;

        case IceProcessMessagesIOError:
                g_debug ("GsmXSMPClient: IceProcessMessagesIOError on '%s'", client->priv->description);
                gsm_client_set_status (GSM_CLIENT (client), GSM_CLIENT_FAILED);
                /* Emitting "disconnected" will eventually cause
                 * IceCloseConnection() to be called.
                 */
                gsm_client_disconnected (GSM_CLIENT (client));
                keep_going = FALSE;
                break;

        case IceProcessMessagesConnectionClosed:
                g_debug ("GsmXSMPClient: IceProcessMessagesConnectionClosed on '%s'",
                         client->priv->description);
                client->priv->ice_connection = NULL;
                keep_going = FALSE;
                break;

        default:
                g_assert_not_reached ();
        }
        g_object_unref (client);

        return keep_going;
}

static SmProp *
find_property (GsmXSMPClient *client,
               const char    *name,
               int           *index)
{
        SmProp *prop;
        int i;

        for (i = 0; i < client->priv->props->len; i++) {
                prop = client->priv->props->pdata[i];

                if (!strcmp (prop->name, name)) {
                        if (index) {
                                *index = i;
                        }
                        return prop;
                }
        }

        return NULL;
}

static void
set_description (GsmXSMPClient *client)
{
        SmProp     *prop;
        const char *id;

        prop = find_property (client, SmProgram, NULL);
        id = gsm_client_peek_startup_id (GSM_CLIENT (client));

        g_free (client->priv->description);
        if (prop) {
                client->priv->description = g_strdup_printf ("%p [%.*s %s]",
                                                             client,
                                                             prop->vals[0].length,
                                                             (char *)prop->vals[0].value,
                                                             id);
        } else if (id != NULL) {
                client->priv->description = g_strdup_printf ("%p [%s]", client, id);
        } else {
                client->priv->description = g_strdup_printf ("%p", client);
        }
}

static void
setup_connection (GsmXSMPClient *client)
{
        GIOChannel    *channel;
        int            fd;

        g_debug ("GsmXSMPClient: Setting up new connection");

        fd = IceConnectionNumber (client->priv->ice_connection);
        fcntl (fd, F_SETFD, fcntl (fd, F_GETFD, 0) | FD_CLOEXEC);
        channel = g_io_channel_unix_new (fd);
        client->priv->watch_id = g_io_add_watch (channel,
                                                 G_IO_IN | G_IO_ERR,
                                                 (GIOFunc)client_iochannel_watch,
                                                 client);
        g_io_channel_unref (channel);

        set_description (client);

        g_debug ("GsmXSMPClient: New client '%s'", client->priv->description);
}

static GObject *
gsm_xsmp_client_constructor (GType                  type,
                             guint                  n_construct_properties,
                             GObjectConstructParam *construct_properties)
{
        GsmXSMPClient *client;

        client = GSM_XSMP_CLIENT (G_OBJECT_CLASS (gsm_xsmp_client_parent_class)->constructor (type,
                                                                                              n_construct_properties,
                                                                                              construct_properties));
        setup_connection (client);

        return G_OBJECT (client);
}

static void
gsm_xsmp_client_init (GsmXSMPClient *client)
{
        client->priv = GSM_XSMP_CLIENT_GET_PRIVATE (client);

        client->priv->props = g_ptr_array_new ();
        client->priv->current_save_yourself = -1;
        client->priv->next_save_yourself = -1;
        client->priv->next_save_yourself_allow_interact = FALSE;
}


static void
delete_property (GsmXSMPClient *client,
                 const char    *name)
{
        int     index;
        SmProp *prop;

        prop = find_property (client, name, &index);
        if (!prop) {
                return;
        }

#if 0
        /* This is wrong anyway; we can't unconditionally run the current
         * discard command; if this client corresponds to a GsmAppResumed,
         * and the current discard command is identical to the app's
         * discard_command, then we don't run the discard command now,
         * because that would delete a saved state we may want to resume
         * again later.
         */
        if (!strcmp (name, SmDiscardCommand)) {
                gsm_client_run_discard (GSM_CLIENT (client));
        }
#endif

        g_ptr_array_remove_index_fast (client->priv->props, index);
        SmFreeProperty (prop);
}


static void
debug_print_property (SmProp *prop)
{
        GString *tmp;
        int      i;

        switch (prop->type[0]) {
        case 'C': /* CARD8 */
                g_debug ("GsmXSMPClient:   %s = %d", prop->name, *(unsigned char *)prop->vals[0].value);
                break;

        case 'A': /* ARRAY8 */
                g_debug ("GsmXSMPClient:   %s = '%s'", prop->name, (char *)prop->vals[0].value);
                break;

        case 'L': /* LISTofARRAY8 */
                tmp = g_string_new (NULL);
                for (i = 0; i < prop->num_vals; i++) {
                        g_string_append_printf (tmp, "'%.*s' ", prop->vals[i].length,
                                                (char *)prop->vals[i].value);
                }
                g_debug ("GsmXSMPClient:   %s = %s", prop->name, tmp->str);
                g_string_free (tmp, TRUE);
                break;

        default:
                g_debug ("GsmXSMPClient:   %s = ??? (%s)", prop->name, prop->type);
                break;
        }
}


static void
set_properties_callback (SmsConn     conn,
                         SmPointer   manager_data,
                         int         num_props,
                         SmProp    **props)
{
        GsmXSMPClient *client = manager_data;
        int            i;

        g_debug ("GsmXSMPClient: Set properties from client '%s'", client->priv->description);

        for (i = 0; i < num_props; i++) {
                delete_property (client, props[i]->name);
                g_ptr_array_add (client->priv->props, props[i]);

                debug_print_property (props[i]);

                if (!strcmp (props[i]->name, SmProgram))
                        set_description (client);
        }

        free (props);

}

static void
delete_properties_callback (SmsConn     conn,
                            SmPointer   manager_data,
                            int         num_props,
                            char      **prop_names)
{
        GsmXSMPClient *client = manager_data;
        int i;

        g_debug ("GsmXSMPClient: Delete properties from '%s'", client->priv->description);

        for (i = 0; i < num_props; i++) {
                delete_property (client, prop_names[i]);

                g_debug ("  %s", prop_names[i]);
        }

        free (prop_names);
}

static void
get_properties_callback (SmsConn   conn,
                         SmPointer manager_data)
{
        GsmXSMPClient *client = manager_data;

        g_debug ("GsmXSMPClient: Get properties request from '%s'", client->priv->description);

        SmsReturnProperties (conn,
                             client->priv->props->len,
                             (SmProp **)client->priv->props->pdata);
}

static char *
prop_to_command (SmProp *prop)
{
        GString *str;
        int i, j;
        gboolean need_quotes;

        str = g_string_new (NULL);
        for (i = 0; i < prop->num_vals; i++) {
                char *val = prop->vals[i].value;

                need_quotes = FALSE;
                for (j = 0; j < prop->vals[i].length; j++) {
                        if (!g_ascii_isalnum (val[j]) && !strchr ("-_=:./", val[j])) {
                                need_quotes = TRUE;
                                break;
                        }
                }

                if (i > 0) {
                        g_string_append_c (str, ' ');
                }

                if (!need_quotes) {
                        g_string_append_printf (str,
                                                "%.*s",
                                                prop->vals[i].length,
                                                (char *)prop->vals[i].value);
                } else {
                        g_string_append_c (str, '\'');
                        while (val < (char *)prop->vals[i].value + prop->vals[i].length) {
                                if (*val == '\'') {
                                        g_string_append (str, "'\''");
                                } else {
                                        g_string_append_c (str, *val);
                                }
                                val++;
                        }
                        g_string_append_c (str, '\'');
                }
        }

        return g_string_free (str, FALSE);
}

static char *
xsmp_get_restart_command (GsmClient *client)
{
        SmProp *prop;

        prop = find_property (GSM_XSMP_CLIENT (client), SmRestartCommand, NULL);

        if (!prop || strcmp (prop->type, SmLISTofARRAY8) != 0) {
                return NULL;
        }

        return prop_to_command (prop);
}

static char *
xsmp_get_discard_command (GsmClient *client)
{
        SmProp *prop;

        prop = find_property (GSM_XSMP_CLIENT (client), SmDiscardCommand, NULL);

        if (!prop || strcmp (prop->type, SmLISTofARRAY8) != 0) {
                return NULL;
        }

        return prop_to_command (prop);
}

static void
do_save_yourself (GsmXSMPClient *client,
                  int            save_type,
                  gboolean       allow_interact)
{
        g_assert (client->priv->conn != NULL);

        if (client->priv->next_save_yourself != -1) {
                /* Either we're currently doing a shutdown and there's a checkpoint
                 * queued after it, or vice versa. Either way, the new SaveYourself
                 * is redundant.
                 */
                g_debug ("GsmXSMPClient:   skipping redundant SaveYourself for '%s'",
                         client->priv->description);
        } else if (client->priv->current_save_yourself != -1) {
                g_debug ("GsmXSMPClient:   queuing new SaveYourself for '%s'",
                         client->priv->description);
                client->priv->next_save_yourself = save_type;
                client->priv->next_save_yourself_allow_interact = allow_interact;
        } else {
                client->priv->current_save_yourself = save_type;
                /* make sure we don't have anything queued */
                client->priv->next_save_yourself = -1;
                client->priv->next_save_yourself_allow_interact = FALSE;

                switch (save_type) {
                case SmSaveLocal:
                        /* Save state */
                        SmsSaveYourself (client->priv->conn,
                                         SmSaveLocal,
                                         FALSE,
                                         SmInteractStyleNone,
                                         FALSE);
                        break;

                default:
                        /* Logout */
                        if (!allow_interact) {
                                SmsSaveYourself (client->priv->conn,
                                                 save_type, /* save type */
                                                 TRUE, /* shutdown */
                                                 SmInteractStyleNone, /* interact style */
                                                 TRUE); /* fast */
                        } else {
                                SmsSaveYourself (client->priv->conn,
                                                 save_type, /* save type */
                                                 TRUE, /* shutdown */
                                                 SmInteractStyleAny, /* interact style */
                                                 FALSE /* fast */);
                        }
                        break;
                }
        }
}

static void
xsmp_save_yourself_phase2 (GsmClient *client)
{
        GsmXSMPClient *xsmp = (GsmXSMPClient *) client;

        g_debug ("GsmXSMPClient: xsmp_save_yourself_phase2 ('%s')", xsmp->priv->description);

        SmsSaveYourselfPhase2 (xsmp->priv->conn);
}

static void
xsmp_interact (GsmClient *client)
{
        GsmXSMPClient *xsmp = (GsmXSMPClient *) client;

        g_debug ("GsmXSMPClient: xsmp_interact ('%s')", xsmp->priv->description);

        SmsInteract (xsmp->priv->conn);
}

static gboolean
xsmp_cancel_end_session (GsmClient *client,
                         GError   **error)
{
        GsmXSMPClient *xsmp = (GsmXSMPClient *) client;

        g_debug ("GsmXSMPClient: xsmp_cancel_end_session ('%s')", xsmp->priv->description);

        if (xsmp->priv->conn == NULL) {
                g_set_error (error,
                             GSM_CLIENT_ERROR,
                             GSM_CLIENT_ERROR_NOT_REGISTERED,
                             "Client is not registered");
                return FALSE;
        }

        SmsShutdownCancelled (xsmp->priv->conn);

        /* reset the state */
        xsmp->priv->current_save_yourself = -1;
        xsmp->priv->next_save_yourself = -1;
        xsmp->priv->next_save_yourself_allow_interact = FALSE;

        return TRUE;
}

static char *
get_desktop_file_path (GsmXSMPClient *client)
{
        SmProp     *prop;
        char       *desktop_file_path = NULL;
        char      **dirs;
        const char *program_name;

        /* XSMP clients using eggsmclient defines a special property
         * pointing to their respective desktop entry file */
        prop = find_property (client, GsmDesktopFile, NULL);

        if (prop) {
                GFile *file = g_file_new_for_uri (prop->vals[0].value);
                desktop_file_path = g_file_get_path (file);
                g_object_unref (file);
                goto out;
        }

        /* If we can't get desktop file from GsmDesktopFile then we
         * try to find the desktop file from its program name */
        prop = find_property (client, SmProgram, NULL);
        program_name = prop->vals[0].value;

        dirs = gsm_util_get_autostart_dirs ();

        desktop_file_path =
                gsm_util_find_desktop_file_for_app_name (program_name,
                                                         dirs);

        g_strfreev (dirs);

out:
        g_debug ("GsmXSMPClient: desktop file for client %s is %s",
                 gsm_client_peek_id (GSM_CLIENT (client)),
                 desktop_file_path ? desktop_file_path : "(null)");

        return desktop_file_path;
}

static void
set_desktop_file_keys_from_client (GsmClient *client,
                                   GKeyFile  *keyfile)
{
        SmProp *prop;
        char   *name;
        char   *comment;

        prop = find_property (GSM_XSMP_CLIENT (client), SmProgram, NULL);
        name = g_strdup (prop->vals[0].value);

        comment = g_strdup_printf ("Client %s which was automatically saved",
                                   gsm_client_peek_startup_id (client));

        g_key_file_set_string (keyfile,
                               G_KEY_FILE_DESKTOP_GROUP,
                               G_KEY_FILE_DESKTOP_KEY_NAME,
                               name);

        g_key_file_set_string (keyfile,
                               G_KEY_FILE_DESKTOP_GROUP,
                               G_KEY_FILE_DESKTOP_KEY_COMMENT,
                               comment);

        g_key_file_set_string (keyfile,
                               G_KEY_FILE_DESKTOP_GROUP,
                               G_KEY_FILE_DESKTOP_KEY_ICON,
                               "system-run");

        g_key_file_set_string (keyfile,
                               G_KEY_FILE_DESKTOP_GROUP,
                               G_KEY_FILE_DESKTOP_KEY_TYPE,
                               "Application");

        g_key_file_set_boolean (keyfile,
                                G_KEY_FILE_DESKTOP_GROUP,
                                G_KEY_FILE_DESKTOP_KEY_STARTUP_NOTIFY,
                                TRUE);

        g_free (name);
        g_free (comment);
}

static GKeyFile *
create_client_key_file (GsmClient   *client,
                        const char  *desktop_file_path,
                        GError     **error) {
        GKeyFile *keyfile;

        keyfile = g_key_file_new ();

        if (desktop_file_path != NULL) {
                g_key_file_load_from_file (keyfile,
                                           desktop_file_path,
                                           G_KEY_FILE_KEEP_COMMENTS |
                                           G_KEY_FILE_KEEP_TRANSLATIONS,
                                           error);
        } else {
                set_desktop_file_keys_from_client (client, keyfile);
        }

        return keyfile;
}

static GsmClientRestartStyle
xsmp_get_restart_style_hint (GsmClient *client);

static GKeyFile *
xsmp_save (GsmClient *client,
           GError   **error)
{
        GsmClientRestartStyle restart_style;

        GKeyFile *keyfile = NULL;
        char     *desktop_file_path = NULL;
        char     *exec_program = NULL;
        char     *exec_discard = NULL;
        char     *startup_id = NULL;
        GError   *local_error;

        g_debug ("GsmXSMPClient: saving client with id %s",
                 gsm_client_peek_id (client));

        local_error = NULL;

        restart_style = xsmp_get_restart_style_hint (client);
        if (restart_style == GSM_CLIENT_RESTART_NEVER) {
                goto out;
        }

        exec_program = xsmp_get_restart_command (client);
        if (!exec_program) {
                goto out;
        }

        desktop_file_path = get_desktop_file_path (GSM_XSMP_CLIENT (client));

        keyfile = create_client_key_file (client,
                                          desktop_file_path,
                                          &local_error);

        if (local_error) {
                goto out;
        }

        g_object_get (client,
                      "startup-id", &startup_id,
                      NULL);

        g_key_file_set_string (keyfile,
                               G_KEY_FILE_DESKTOP_GROUP,
                               GSM_AUTOSTART_APP_STARTUP_ID_KEY,
                               startup_id);

        g_key_file_set_string (keyfile,
                               G_KEY_FILE_DESKTOP_GROUP,
                               G_KEY_FILE_DESKTOP_KEY_EXEC,
                               exec_program);

        exec_discard = xsmp_get_discard_command (client);
        if (exec_discard)
                g_key_file_set_string (keyfile,
                                       G_KEY_FILE_DESKTOP_GROUP,
                                       GSM_AUTOSTART_APP_DISCARD_KEY,
                                       exec_discard);

out:
        g_free (desktop_file_path);
        g_free (exec_program);
        g_free (exec_discard);
        g_free (startup_id);

        if (local_error != NULL) {
                g_propagate_error (error, local_error);
                g_key_file_free (keyfile);

                return NULL;
        }

        return keyfile;
}

static gboolean
xsmp_stop (GsmClient *client,
           GError   **error)
{
        GsmXSMPClient *xsmp = (GsmXSMPClient *) client;

        g_debug ("GsmXSMPClient: xsmp_stop ('%s')", xsmp->priv->description);

        if (xsmp->priv->conn == NULL) {
                g_set_error (error,
                             GSM_CLIENT_ERROR,
                             GSM_CLIENT_ERROR_NOT_REGISTERED,
                             "Client is not registered");
                return FALSE;
        }

        SmsDie (xsmp->priv->conn);

        return TRUE;
}

static gboolean
xsmp_query_end_session (GsmClient *client,
                        guint      flags,
                        GError   **error)
{
        gboolean allow_interact;
        int      save_type;

        if (GSM_XSMP_CLIENT (client)->priv->conn == NULL) {
                g_set_error (error,
                             GSM_CLIENT_ERROR,
                             GSM_CLIENT_ERROR_NOT_REGISTERED,
                             "Client is not registered");
                return FALSE;
        }

        allow_interact = !(flags & GSM_CLIENT_END_SESSION_FLAG_FORCEFUL);

        /* we don't want to save the session state, but we just want to know if
         * there's user data the client has to save and we want to give the
         * client a chance to tell the user about it. This is consistent with
         * the manager not setting GSM_CLIENT_END_SESSION_FLAG_SAVE for this
         * phase. */
        save_type = SmSaveGlobal;

        do_save_yourself (GSM_XSMP_CLIENT (client), save_type, allow_interact);
        return TRUE;
}

static gboolean
xsmp_end_session (GsmClient *client,
                  guint      flags,
                  GError   **error)
{
        gboolean phase2;

        if (GSM_XSMP_CLIENT (client)->priv->conn == NULL) {
                g_set_error (error,
                             GSM_CLIENT_ERROR,
                             GSM_CLIENT_ERROR_NOT_REGISTERED,
                             "Client is not registered");
                return FALSE;
        }

        phase2 = (flags & GSM_CLIENT_END_SESSION_FLAG_LAST);

        if (phase2) {
                xsmp_save_yourself_phase2 (client);
        } else {
                gboolean allow_interact;
                int      save_type;

                /* we gave a chance to interact to the app during
                 * xsmp_query_end_session(), now it's too late to interact */
                allow_interact = FALSE;

                if (flags & GSM_CLIENT_END_SESSION_FLAG_SAVE) {
                        save_type = SmSaveBoth;
                } else {
                        save_type = SmSaveGlobal;
                }

                do_save_yourself (GSM_XSMP_CLIENT (client),
                                  save_type, allow_interact);
        }

        return TRUE;
}

static char *
xsmp_get_app_name (GsmClient *client)
{
        SmProp *prop;
        char   *name;

        prop = find_property (GSM_XSMP_CLIENT (client), SmProgram, NULL);
        name = prop_to_command (prop);

        return name;
}

static void
gsm_client_set_ice_connection (GsmXSMPClient *client,
                               gpointer       conn)
{
        client->priv->ice_connection = conn;
}

static void
gsm_xsmp_client_set_property (GObject       *object,
                              guint          prop_id,
                              const GValue  *value,
                              GParamSpec    *pspec)
{
        GsmXSMPClient *self;

        self = GSM_XSMP_CLIENT (object);

        switch (prop_id) {
        case PROP_ICE_CONNECTION:
                gsm_client_set_ice_connection (self, g_value_get_pointer (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gsm_xsmp_client_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
        GsmXSMPClient *self;

        self = GSM_XSMP_CLIENT (object);

        switch (prop_id) {
        case PROP_ICE_CONNECTION:
                g_value_set_pointer (value, self->priv->ice_connection);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gsm_xsmp_client_disconnect (GsmXSMPClient *client)
{
        if (client->priv->watch_id > 0) {
                g_source_remove (client->priv->watch_id);
        }

        if (client->priv->conn != NULL) {
                SmsCleanUp (client->priv->conn);
        }

        if (client->priv->ice_connection != NULL) {
                IceSetShutdownNegotiation (client->priv->ice_connection, FALSE);
                IceCloseConnection (client->priv->ice_connection);
        }
}

static void
gsm_xsmp_client_finalize (GObject *object)
{
        GsmXSMPClient *client = (GsmXSMPClient *) object;

        g_debug ("GsmXSMPClient: xsmp_finalize (%s)", client->priv->description);
        gsm_xsmp_client_disconnect (client);

        g_free (client->priv->description);
        g_ptr_array_foreach (client->priv->props, (GFunc)SmFreeProperty, NULL);
        g_ptr_array_free (client->priv->props, TRUE);

        G_OBJECT_CLASS (gsm_xsmp_client_parent_class)->finalize (object);
}

static gboolean
_boolean_handled_accumulator (GSignalInvocationHint *ihint,
                              GValue                *return_accu,
                              const GValue          *handler_return,
                              gpointer               dummy)
{
        gboolean    continue_emission;
        gboolean    signal_handled;

        signal_handled = g_value_get_boolean (handler_return);
        g_value_set_boolean (return_accu, signal_handled);
        continue_emission = !signal_handled;

        return continue_emission;
}

static GsmClientRestartStyle
xsmp_get_restart_style_hint (GsmClient *client)
{
        SmProp               *prop;
        GsmClientRestartStyle hint;

        g_debug ("GsmXSMPClient: getting restart style");
        hint = GSM_CLIENT_RESTART_IF_RUNNING;

        prop = find_property (GSM_XSMP_CLIENT (client), SmRestartStyleHint, NULL);

        if (!prop || strcmp (prop->type, SmCARD8) != 0) {
                return GSM_CLIENT_RESTART_IF_RUNNING;
        }

        switch (((unsigned char *)prop->vals[0].value)[0]) {
        case SmRestartIfRunning:
                hint = GSM_CLIENT_RESTART_IF_RUNNING;
                break;
        case SmRestartAnyway:
                hint = GSM_CLIENT_RESTART_ANYWAY;
                break;
        case SmRestartImmediately:
                hint = GSM_CLIENT_RESTART_IMMEDIATELY;
                break;
        case SmRestartNever:
                hint = GSM_CLIENT_RESTART_NEVER;
                break;
        default:
                break;
        }

        return hint;
}

static gboolean
_parse_value_as_uint (const char *value,
                      guint      *uintval)
{
        char  *end_of_valid_uint;
        gulong ulong_value;
        guint  uint_value;

        errno = 0;
        ulong_value = strtoul (value, &end_of_valid_uint, 10);

        if (*value == '\0' || *end_of_valid_uint != '\0') {
                return FALSE;
        }

        uint_value = ulong_value;
        if (uint_value != ulong_value || errno == ERANGE) {
                return FALSE;
        }

        *uintval = uint_value;

        return TRUE;
}

static guint
xsmp_get_unix_process_id (GsmClient *client)
{
        SmProp  *prop;
        guint    pid;
        gboolean res;

        g_debug ("GsmXSMPClient: getting pid");

        prop = find_property (GSM_XSMP_CLIENT (client), SmProcessID, NULL);

        if (!prop || strcmp (prop->type, SmARRAY8) != 0) {
                return 0;
        }

        pid = 0;
        res = _parse_value_as_uint ((char *)prop->vals[0].value, &pid);
        if (! res) {
                pid = 0;
        }

        return pid;
}

static void
gsm_xsmp_client_class_init (GsmXSMPClientClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);
        GsmClientClass *client_class = GSM_CLIENT_CLASS (klass);

        object_class->finalize             = gsm_xsmp_client_finalize;
        object_class->constructor          = gsm_xsmp_client_constructor;
        object_class->get_property         = gsm_xsmp_client_get_property;
        object_class->set_property         = gsm_xsmp_client_set_property;

        client_class->impl_save                   = xsmp_save;
        client_class->impl_stop                   = xsmp_stop;
        client_class->impl_query_end_session      = xsmp_query_end_session;
        client_class->impl_end_session            = xsmp_end_session;
        client_class->impl_cancel_end_session     = xsmp_cancel_end_session;
        client_class->impl_get_app_name           = xsmp_get_app_name;
        client_class->impl_get_restart_style_hint = xsmp_get_restart_style_hint;
        client_class->impl_get_unix_process_id    = xsmp_get_unix_process_id;

        signals[REGISTER_REQUEST] =
                g_signal_new ("register-request",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GsmXSMPClientClass, register_request),
                              _boolean_handled_accumulator,
                              NULL,
                              gsm_marshal_BOOLEAN__POINTER,
                              G_TYPE_BOOLEAN,
                              1, G_TYPE_POINTER);
        signals[LOGOUT_REQUEST] =
                g_signal_new ("logout-request",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GsmXSMPClientClass, logout_request),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__BOOLEAN,
                              G_TYPE_NONE,
                              1, G_TYPE_BOOLEAN);

        g_object_class_install_property (object_class,
                                         PROP_ICE_CONNECTION,
                                         g_param_spec_pointer ("ice-connection",
                                                               "ice-connection",
                                                               "ice-connection",
                                                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

        g_type_class_add_private (klass, sizeof (GsmXSMPClientPrivate));
}

GsmClient *
gsm_xsmp_client_new (IceConn ice_conn)
{
        GsmXSMPClient *xsmp;

        xsmp = g_object_new (GSM_TYPE_XSMP_CLIENT,
                             "ice-connection", ice_conn,
                             NULL);

        return GSM_CLIENT (xsmp);
}

static Status
register_client_callback (SmsConn    conn,
                          SmPointer  manager_data,
                          char      *previous_id)
{
        GsmXSMPClient *client = manager_data;
        gboolean       handled;
        char          *id;

        g_debug ("GsmXSMPClient: Client '%s' received RegisterClient(%s)",
                 client->priv->description,
                 previous_id ? previous_id : "NULL");


        /* There are three cases:
         * 1. id is NULL - we'll use a new one
         * 2. id is known - we'll use known one
         * 3. id is unknown - this is an error
         */
        id = g_strdup (previous_id);

        handled = FALSE;
        g_signal_emit (client, signals[REGISTER_REQUEST], 0, &id, &handled);
        if (! handled) {
                g_debug ("GsmXSMPClient:  RegisterClient not handled!");
                g_free (id);
                free (previous_id);
                g_assert_not_reached ();
                return FALSE;
        }

        if (IS_STRING_EMPTY (id)) {
                g_debug ("GsmXSMPClient:   rejected: invalid previous_id");
                free (previous_id);
                return FALSE;
        }

        g_object_set (client, "startup-id", id, NULL);

        set_description (client);

        g_debug ("GsmXSMPClient: Sending RegisterClientReply to '%s'", client->priv->description);

        SmsRegisterClientReply (conn, id);

        if (IS_STRING_EMPTY (previous_id)) {
                /* Send the initial SaveYourself. */
                g_debug ("GsmXSMPClient: Sending initial SaveYourself");
                SmsSaveYourself (conn, SmSaveLocal, False, SmInteractStyleNone, False);
                client->priv->current_save_yourself = SmSaveLocal;
        }

        gsm_client_set_status (GSM_CLIENT (client), GSM_CLIENT_REGISTERED);

        g_free (id);
        free (previous_id);

        return TRUE;
}


static void
save_yourself_request_callback (SmsConn   conn,
                                SmPointer manager_data,
                                int       save_type,
                                Bool      shutdown,
                                int       interact_style,
                                Bool      fast,
                                Bool      global)
{
        GsmXSMPClient *client = manager_data;

        g_debug ("GsmXSMPClient: Client '%s' received SaveYourselfRequest(%s, %s, %s, %s, %s)",
                 client->priv->description,
                 save_type == SmSaveLocal ? "SmSaveLocal" :
                 save_type == SmSaveGlobal ? "SmSaveGlobal" : "SmSaveBoth",
                 shutdown ? "Shutdown" : "!Shutdown",
                 interact_style == SmInteractStyleAny ? "SmInteractStyleAny" :
                 interact_style == SmInteractStyleErrors ? "SmInteractStyleErrors" :
                 "SmInteractStyleNone", fast ? "Fast" : "!Fast",
                 global ? "Global" : "!Global");

        /* Examining the g_debug above, you can see that there are a total
         * of 72 different combinations of options that this could have been
         * called with. However, most of them are stupid.
         *
         * If @shutdown and @global are both TRUE, that means the caller is
         * requesting that a logout message be sent to all clients, so we do
         * that. We use @fast to decide whether or not to show a
         * confirmation dialog. (This isn't really what @fast is for, but
         * the old mate-session and ksmserver both interpret it that way,
         * so we do too.) We ignore @save_type because we pick the correct
         * save_type ourselves later based on user prefs, dialog choices,
         * etc, and we ignore @interact_style, because clients have not used
         * it correctly consistently enough to make it worth honoring.
         *
         * If @shutdown is TRUE and @global is FALSE, the caller is
         * confused, so we ignore the request.
         *
         * If @shutdown is FALSE and @save_type is SmSaveGlobal or
         * SmSaveBoth, then the client wants us to ask some or all open
         * applications to save open files to disk, but NOT quit. This is
         * silly and so we ignore the request.
         *
         * If @shutdown is FALSE and @save_type is SmSaveLocal, then the
         * client wants us to ask some or all open applications to update
         * their current saved state, but not log out. At the moment, the
         * code only supports this for the !global case (ie, a client
         * requesting that it be allowed to update *its own* saved state,
         * but not having everyone else update their saved state).
         */

        if (shutdown && global) {
                g_debug ("GsmXSMPClient:   initiating shutdown");
                g_signal_emit (client, signals[LOGOUT_REQUEST], 0, !fast);
        } else if (!shutdown && !global) {
                g_debug ("GsmXSMPClient:   initiating checkpoint");
                do_save_yourself (client, SmSaveLocal, TRUE);
        } else {
                g_debug ("GsmXSMPClient:   ignoring");
        }
}

static void
save_yourself_phase2_request_callback (SmsConn   conn,
                                       SmPointer manager_data)
{
        GsmXSMPClient *client = manager_data;

        g_debug ("GsmXSMPClient: Client '%s' received SaveYourselfPhase2Request",
                 client->priv->description);

        client->priv->current_save_yourself = -1;

        /* this is a valid response to SaveYourself and therefore
           may be a response to a QES or ES */
        gsm_client_end_session_response (GSM_CLIENT (client),
                                         TRUE, TRUE, FALSE,
                                         NULL);
}

static void
interact_request_callback (SmsConn   conn,
                           SmPointer manager_data,
                           int       dialog_type)
{
        GsmXSMPClient *client = manager_data;
#if 0
        gboolean       res;
        GError        *error;
#endif

        g_debug ("GsmXSMPClient: Client '%s' received InteractRequest(%s)",
                 client->priv->description,
                 dialog_type == SmDialogNormal ? "Dialog" : "Errors");

        gsm_client_end_session_response (GSM_CLIENT (client),
                                         FALSE, FALSE, FALSE,
                                         _("This program is blocking logout."));

#if 0
        /* Can't just call back with Interact because session client
           grabs the keyboard!  So, we try to get it to release
           grabs by telling it we've cancelled the shutdown.
           This grabbing is clearly bullshit and is not supported by
           the client spec or protocol spec.
        */
        res = xsmp_cancel_end_session (GSM_CLIENT (client), &error);
        if (! res) {
                g_warning ("Unable to cancel end session: %s", error->message);
                g_error_free (error);
        }
#endif
        xsmp_interact (GSM_CLIENT (client));
}

static void
interact_done_callback (SmsConn   conn,
                        SmPointer manager_data,
                        Bool      cancel_shutdown)
{
        GsmXSMPClient *client = manager_data;

        g_debug ("GsmXSMPClient: Client '%s' received InteractDone(cancel_shutdown = %s)",
                 client->priv->description,
                 cancel_shutdown ? "True" : "False");

        gsm_client_end_session_response (GSM_CLIENT (client),
                                         TRUE, FALSE, cancel_shutdown,
                                         NULL);
}

static void
save_yourself_done_callback (SmsConn   conn,
                             SmPointer manager_data,
                             Bool      success)
{
        GsmXSMPClient *client = manager_data;

        g_debug ("GsmXSMPClient: Client '%s' received SaveYourselfDone(success = %s)",
                 client->priv->description,
                 success ? "True" : "False");

	if (client->priv->current_save_yourself != -1) {
		SmsSaveComplete (client->priv->conn);
		client->priv->current_save_yourself = -1;
	}

        /* If success is false then the application couldn't save data. Nothing
         * the session manager can do about, though. FIXME: we could display a
         * dialog about this, I guess. */
        gsm_client_end_session_response (GSM_CLIENT (client),
                                         TRUE, FALSE, FALSE,
                                         NULL);

        if (client->priv->next_save_yourself) {
                int      save_type = client->priv->next_save_yourself;
                gboolean allow_interact = client->priv->next_save_yourself_allow_interact;

                client->priv->next_save_yourself = -1;
                client->priv->next_save_yourself_allow_interact = -1;
                do_save_yourself (client, save_type, allow_interact);
        }
}

static void
close_connection_callback (SmsConn     conn,
                           SmPointer   manager_data,
                           int         count,
                           char      **reason_msgs)
{
        GsmXSMPClient *client = manager_data;
        int            i;

        g_debug ("GsmXSMPClient: Client '%s' received CloseConnection", client->priv->description);
        for (i = 0; i < count; i++) {
                g_debug ("GsmXSMPClient:  close reason: '%s'", reason_msgs[i]);
        }
        SmFreeReasons (count, reason_msgs);

        gsm_client_set_status (GSM_CLIENT (client), GSM_CLIENT_FINISHED);
        gsm_client_disconnected (GSM_CLIENT (client));
}

void
gsm_xsmp_client_connect (GsmXSMPClient *client,
                         SmsConn        conn,
                         unsigned long *mask_ret,
                         SmsCallbacks  *callbacks_ret)
{
        client->priv->conn = conn;

        g_debug ("GsmXSMPClient: Initializing client %s", client->priv->description);

        *mask_ret = 0;

        *mask_ret |= SmsRegisterClientProcMask;
        callbacks_ret->register_client.callback = register_client_callback;
        callbacks_ret->register_client.manager_data  = client;

        *mask_ret |= SmsInteractRequestProcMask;
        callbacks_ret->interact_request.callback = interact_request_callback;
        callbacks_ret->interact_request.manager_data = client;

        *mask_ret |= SmsInteractDoneProcMask;
        callbacks_ret->interact_done.callback = interact_done_callback;
        callbacks_ret->interact_done.manager_data = client;

        *mask_ret |= SmsSaveYourselfRequestProcMask;
        callbacks_ret->save_yourself_request.callback = save_yourself_request_callback;
        callbacks_ret->save_yourself_request.manager_data = client;

        *mask_ret |= SmsSaveYourselfP2RequestProcMask;
        callbacks_ret->save_yourself_phase2_request.callback = save_yourself_phase2_request_callback;
        callbacks_ret->save_yourself_phase2_request.manager_data = client;

        *mask_ret |= SmsSaveYourselfDoneProcMask;
        callbacks_ret->save_yourself_done.callback = save_yourself_done_callback;
        callbacks_ret->save_yourself_done.manager_data = client;

        *mask_ret |= SmsCloseConnectionProcMask;
        callbacks_ret->close_connection.callback = close_connection_callback;
        callbacks_ret->close_connection.manager_data  = client;

        *mask_ret |= SmsSetPropertiesProcMask;
        callbacks_ret->set_properties.callback = set_properties_callback;
        callbacks_ret->set_properties.manager_data = client;

        *mask_ret |= SmsDeletePropertiesProcMask;
        callbacks_ret->delete_properties.callback = delete_properties_callback;
        callbacks_ret->delete_properties.manager_data = client;

        *mask_ret |= SmsGetPropertiesProcMask;
        callbacks_ret->get_properties.callback = get_properties_callback;
        callbacks_ret->get_properties.manager_data = client;
}

void
gsm_xsmp_client_save_state (GsmXSMPClient *client)
{
        g_return_if_fail (GSM_IS_XSMP_CLIENT (client));
}
