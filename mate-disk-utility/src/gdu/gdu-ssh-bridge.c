/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2009 David Zeuthen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */


#include "config.h"
#include <glib/gi18n-lib.h>

#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <gio/gunixinputstream.h>
#include <gio/gunixoutputstream.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "gdu-ssh-bridge.h"
#include "gdu-error.h"

/* Here's how it works
 *
 * - Client sets up a DBusServer on port LOCAL_PORT
 *   - we try a number of ports (in the 9000 to 10000 range) until this succeed
 * - Client creates a big random number SECRET
 * - Client creates a ssh connection to Server requesting a port-forward to Client:LOCAL_PORT
 *   - using -R 0:localhost:LOCAL_PORT
 * - Client parses REMOTE_PORT from "Allocated port <NUM> for remote forward to localhost:LOCAL_PORT"
 * - Client launches "udisks-tcp-bridge -p REMOTE" on Server and then writes SECRET and then a newline
 * - Server (effectively, ie. through the bridge) connects to Client:LOCAL_PORT
 * - Server invokes the org.freedesktop.UDisks.Client.Authorize with the SECRET string
 * - Client checks SECRET
 *   - if it doesn't check out, and error is returned and Server is disconnected
 *   - otherwise the method returns
 *
 *  Client can now use the D-Bus connection - Server is guaranteed to forward method calls and signals
 *  to and from the org.freedesktop.UDisks service running on Server
 *
 *  The reason we pass have an authorization SECRET is that otherwise
 *  malicious users on both the Client and Server may interfere.
 */

/* TODO: Need to do all this async (or in a thread) so we can have API like this
 *
 *   void gdu_pool_new_for_address            (const gchar         *ssh_user_name,
 *                                             const gchar         *ssh_address,
 *                                             GMountOperation     *connect_operation,
 *                                             GCancellable        *cancellable,
 *                                             GAsyncReadyCallback  callback,
 *                                             gpointer             user_data);
 *
 *   GduPool *gdu_pool_new_for_address_finish (GAsyncResult        *res,
 *                                             GError             **error);
 *
 * on the GduPool object. Also, we won't initially use the @connect_operation parameter for
 * now - but can do if (or once) we supply our own program to use in $SSH_ASKPASS.
 */

typedef struct {
        gchar *secret;

        GCancellable *cancellable;
        GMainLoop *loop;
        DBusConnection *server_connection;
        gboolean authorized;

        gchar *error_message;
} BridgeData;

static DBusHandlerResult connection_filter_func (DBusConnection  *connection,
                                                 DBusMessage     *message,
                                                 void            *user_data);

static void
bridge_data_free (BridgeData *data)
{
        if (data->secret != NULL)
                memset (data->secret, '\0', strlen (data->secret));
        g_free (data->secret);
        g_free (data->error_message);
        if (data->loop != NULL)
                g_main_loop_unref (data->loop);
        if (data->server_connection != NULL) {
                dbus_connection_remove_filter (data->server_connection,
                                               connection_filter_func,
                                               data);
                dbus_connection_unref (data->server_connection);
        }
        if (data->cancellable != NULL)
                g_object_unref (data->cancellable);
        g_free (data);
}

static DBusHandlerResult
connection_filter_func (DBusConnection  *connection,
                        DBusMessage     *message,
                        void            *user_data)
{
        BridgeData *data = user_data;

        //g_print ("Filter func!\n");

        if (dbus_message_is_method_call (message, "org.freedesktop.UDisks.Client", "Authorize") &&
            dbus_message_has_path (message, "/org/freedesktop/UDisks/Client")) {
                const gchar *secret;
                DBusMessage *reply;
                DBusError dbus_error;

                dbus_error_init (&dbus_error);
                if (!dbus_message_get_args (message, &dbus_error, DBUS_TYPE_STRING, &secret, DBUS_TYPE_INVALID)) {
                        reply = dbus_message_new_error (message,
                                                        "org.freedesktop.UDisks.Error.PermissionDenied",
                                                        "Error extracting authorization secret");
                        dbus_error_free (&dbus_error);
                        data->error_message = g_strdup ("No (or malformed) authorization secret included in the Authorize() method call!");
                } else if (g_strcmp0 (secret, data->secret) != 0) {
                        reply = dbus_message_new_error (message,
                                                        "org.freedesktop.UDisks.Error.PermissionDenied",
                                                        "Authorization secret does not match");
                        data->error_message = g_strdup ("Authorization secret passed from server does not match what we expected");
                } else {
                        data->authorized = TRUE;
                        reply = dbus_message_new_method_return (message);
                }
                dbus_connection_send (connection, reply, NULL);
                dbus_message_unref (reply);
        } else {
                data->error_message = g_strdup_printf ("Expected a method call Authorize() on interface "
                                                       "org.freedesktop.UDisks.Client  on the object "
                                                       "/org/freedesktop/UDisks/Client but instead got a message of "
                                                       "type=%d, path=`%s', interface=`%s' and member=`%s'.\n",
                                                       dbus_message_get_type (message),
                                                       dbus_message_get_path (message),
                                                       dbus_message_get_interface (message),
                                                       dbus_message_get_member (message));
        }

        g_main_loop_quit (data->loop);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static void
on_new_connection (DBusServer     *server,
                   DBusConnection *new_connection,
                   void           *user_data)
{
        BridgeData *data = user_data;

        if (data->server_connection != NULL) {
                g_printerr ("Someone tried to connect but we already have a connection.\n");
                goto out;
        }

        dbus_connection_set_allow_anonymous (new_connection, TRUE);
        dbus_connection_setup_with_g_main (new_connection, NULL);
        dbus_connection_ref (new_connection);
        data->server_connection = new_connection;

        //g_print ("We have a client - waiting for authorization\n");

        /* Wait for client to authorize */
        dbus_connection_add_filter (data->server_connection,
                                    connection_filter_func,
                                    data,
                                    NULL);
 out:
        ;
}

static void
child_setup (gpointer user_data)
{
        gint fd;

        /* lose controlling terminal - this forces $SSH_ASKPASS to be used for authentication */
        if ((fd = open("/dev/tty", O_RDWR)) >= 0) {
                ioctl(fd, TIOCNOTTY, 0); /* lose controlling terminal */
                close(fd);
        }
}

static void
fixup_newlines (gchar *s)
{
        gsize len;

        if (s == NULL)
                goto out;

        len = strlen (s);
        if (len < 1)
                goto out;

        if (s[len - 1] == '\r')
                s[len - 1] = '\0';

 out:
        ;
}


DBusGConnection *
_gdu_ssh_bridge_connect (const gchar      *ssh_user_name,
                         const gchar      *ssh_address,
                         GPid             *out_pid,
                         GError          **error)
{
        BridgeData *data;
        DBusGConnection *ret;
        GError *local_error;
        gchar *command_line;
        gint ssh_argc;
        gchar **ssh_argv;
        GPid ssh_pid;
        gchar *dbus_address;
        gint stdin_fd;
        gint stdout_fd;
        gint stderr_fd;
        gint local_port;
        gint remote_port;
        GOutputStream *stdin_stream;
        GInputStream *stdout_stream;
        GInputStream *stderr_stream;
        GDataOutputStream *stdin_data_stream;
        GDataInputStream *stdout_data_stream;
        GDataInputStream *stderr_data_stream;
        gchar *s;
        DBusError dbus_error;
        DBusServer *server;
        const gchar *auth_mechanisms[] = {"ANONYMOUS", NULL};
        GString *str;
        guint n;

        local_error = NULL;

        ret = NULL;

        data = NULL;
        ssh_argv = NULL;
        command_line = NULL;
        dbus_address = NULL;
        server = NULL;
        stdin_data_stream = NULL;
        stdout_data_stream = NULL;
        stderr_data_stream = NULL;
        ssh_pid = 0;

        data = g_new0 (BridgeData, 1);
        str = g_string_new (NULL);
        for (n = 0; n < 32; n++) {
                guint32 r = g_random_int ();
                g_string_append_printf (str, "%08x", r);
        }
        data->secret = g_string_free (str, FALSE);
        data->cancellable = g_cancellable_new ();
        data->loop = g_main_loop_new (NULL, FALSE);

        /* Create and start the local DBusServer */
        for (local_port = 9000; local_port < 10000; local_port++) {
                s = g_strdup_printf ("tcp:host=localhost,port=%d", local_port);
                dbus_error_init (&dbus_error);
                server = dbus_server_listen (s, &dbus_error);
                g_free (s);
                if (server == NULL) {
                        if (g_strcmp0 (dbus_error.name, "org.freedesktop.DBus.Error.AddressInUse") == 0) {
                                continue;
                        } else {
                                g_set_error (error,
                                             GDU_ERROR,
                                             GDU_ERROR_FAILED,
                                             _("Error listening to address `localhost:%d': %s: %s\n"),
                                             local_port,
                                             dbus_error.name,
                                             dbus_error.message);
                                dbus_error_free (&dbus_error);
                                goto out;
                        }
                } else {
                        break;
                }
        }
        if (server == NULL) {
                g_set_error (error,
                             GDU_ERROR,
                             GDU_ERROR_FAILED,
                             _("Error creating a local TCP server, tried binding to ports 9000-10000 on localhost"));
                dbus_error_free (&dbus_error);
                goto out;
        }

        dbus_server_setup_with_g_main (server, NULL);
        dbus_server_set_new_connection_function (server,
                                                 on_new_connection,
                                                 data,
                                                 NULL);
        /* Allow only anonymous auth */
        if (!dbus_server_set_auth_mechanisms (server, auth_mechanisms)) {
                g_set_error (error,
                             GDU_ERROR,
                             GDU_ERROR_FAILED,
                             _("Error setting auth mechanisms on local DBusServer\n"));
                goto out;
        }

        command_line = g_strdup_printf ("ssh "
                                        "-T "
                                        "-C "
                                        "-R 0:localhost:%d "
                                        "-o \"ForwardX11 no\" "
                                        "-o \"ForwardAgent no\" "
                                        "-o \"Protocol 2\" "
                                        "-o \"NoHostAuthenticationForLocalhost yes\" "
                                        "%s%c%s",
                                        local_port,
                                        ssh_user_name != NULL ? ssh_user_name : "",
                                        ssh_user_name != NULL ? '@' : ' ',
                                        ssh_address);
        //g_print ("command line: `%s'\n", command_line);
        if (!g_shell_parse_argv (command_line,
                                 &ssh_argc,
                                 &ssh_argv,
                                 &local_error)) {
                g_set_error (error,
                             GDU_ERROR,
                             GDU_ERROR_FAILED,
                             _("Unable to parse command-line `%s' (Malformed address?): %s"),
                             command_line,
                             local_error->message);
                g_error_free (local_error);
                goto out;
        }


        if (!g_spawn_async_with_pipes (NULL,
                                       ssh_argv,
                                       NULL,
                                       G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
                                       child_setup,
                                       NULL,
                                       &ssh_pid,
                                       &stdin_fd,
                                       &stdout_fd,
                                       &stderr_fd,
                                       &local_error)) {
                g_set_error (error,
                             GDU_ERROR,
                             GDU_ERROR_FAILED,
                             _("Unable to spawn ssh program: %s"),
                             local_error->message);
                g_error_free (local_error);
                goto out;
        }

        stdin_stream = g_unix_output_stream_new (stdin_fd, TRUE);
        stdout_stream = g_unix_input_stream_new (stdout_fd, TRUE);
        stderr_stream = g_unix_input_stream_new (stderr_fd, TRUE);
        stdin_data_stream = g_data_output_stream_new (stdin_stream);
        stdout_data_stream = g_data_input_stream_new (stdout_stream);
        stderr_data_stream = g_data_input_stream_new (stderr_stream);
        g_object_unref (stdin_stream);
        g_object_unref (stdout_stream);
        g_object_unref (stderr_stream);

        /* Read and parse output from ssh */
        while (TRUE) {
                //g_print (" - Reading...\n");

                s = g_data_input_stream_read_line (stderr_data_stream,
                                                   NULL, /* gsize *length */
                                                   data->cancellable,
                                                   &local_error);
                if (s == NULL) {
                        if (local_error != NULL) {
                                g_set_error (error,
                                             GDU_ERROR,
                                             GDU_ERROR_FAILED,
                                             _("Error reading stderr output: %s"),
                                             local_error->message);
                                g_error_free (local_error);
                        } else {
                                g_set_error_literal (error,
                                                     GDU_ERROR,
                                                     GDU_ERROR_FAILED,
                                                     _("Error reading stderr output: No content"));
                        }
                        goto out;
                }

                fixup_newlines (s);
                //g_print (" - Read `%s'\n", s);

                if (sscanf (s, "Allocated port %d for remote forward to", &remote_port) == 1) {
                        /* got it */
                        g_free (s);
                        break;
                } else if (strstr (s, "Permanently added") != NULL &&
                           strstr (s, "to the list of known hosts") != NULL) {
                        /* just continue */
                        g_free (s);
                } else {
                        GString *full_error_message;

                        /* otherwise assume error - Keep reading output as it may be
                         * multi-line, e.g.
                         *
                         *   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
                         *   @    WARNING: REMOTE HOST IDENTIFICATION HAS CHANGED!     @
                         *   @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
                         *   IT IS POSSIBLE THAT SOMEONE IS DOING SOMETHING NASTY!
                         *   Someone could be eavesdropping on you right now (man-in-the-middle attack)!
                         */
                        full_error_message = g_string_new (s);
                        g_free (s);

                        /* sleep for a bit to ensure we've buffered all the data from ssh's
                         * stderr stream (TODO: this is slightly racy
                         */
                        usleep (100 * 1000);

                        /* kill ssh process early so we won't block on reading additional data if there is none */
                        if (ssh_pid > 0) {
                                kill (ssh_pid, SIGTERM);
                                ssh_pid = 0;
                        }

                        while (TRUE) {
                                s = g_data_input_stream_read_line (stderr_data_stream,
                                                                   NULL, /* gsize *length */
                                                                   data->cancellable,
                                                                   NULL);
                                if (s != NULL) {
                                        fixup_newlines (s);
                                        g_string_append (full_error_message, s);
                                        g_string_append_c (full_error_message, '\n');
                                } else {
                                        break;
                                }
                        }

                        s = g_string_free (full_error_message, FALSE);
                        if (strlen (s) > 0) {
                                g_set_error_literal (error,
                                                     GDU_ERROR,
                                                     GDU_ERROR_FAILED,
                                                     s);
                        } else {
                                g_set_error_literal (error,
                                                     GDU_ERROR,
                                                     GDU_ERROR_FAILED,
                                                     _("Error logging in"));
                        }
                        g_free (s);
                        goto out;
                }
        }

        //g_print ("Yay, remote port is %d (forwarding to local port %d)\n", remote_port, local_port);

        /* Now start the bridge - the udisks-tcp-bridge program will connect to the remote port
         * which is forwarded to the local port by ssh
         */
        s = g_strdup_printf ("udisks-tcp-bridge -p %d\n", remote_port);
        if (!g_data_output_stream_put_string (stdin_data_stream,
                                              s,
                                              data->cancellable,
                                              &local_error)) {
                g_set_error (error,
                             GDU_ERROR,
                             GDU_ERROR_FAILED,
                             _("Error sending `%s': %s"),
                             s,
                             local_error->message);
                g_error_free (local_error);
                g_free (s);
                goto out;
        }
        g_free (s);

        /* Check that the udisks program is waiting for secret
         */
        s = g_data_input_stream_read_line (stderr_data_stream,
                                           NULL, /* gsize *length */
                                           data->cancellable,
                                           &local_error);
        if (s == NULL) {
                g_set_error (error,
                             GDU_ERROR,
                             GDU_ERROR_FAILED,
                             _("Error reading stderr output: %s"),
                             local_error->message);
                g_error_free (local_error);
                goto out;
        }
        if (g_strcmp0 (s, "udisks-tcp-bridge: Waiting for secret") != 0) {
                g_set_error (error,
                             GDU_ERROR,
                             GDU_ERROR_FAILED,
                             _("Unexpected stderr output - expected `udisks-tcp-bridge: Waiting for secret' but got `%s'"),
                             s);
                g_free (s);
                goto out;
        }
        g_free (s);

        /* Pass the secret
         */
        s = g_strdup_printf ("%s\n", data->secret);
        if (!g_data_output_stream_put_string (stdin_data_stream,
                                              s,
                                              data->cancellable,
                                              &local_error)) {
                g_set_error (error,
                             GDU_ERROR,
                             GDU_ERROR_FAILED,
                             _("Error passing authorization secret: %s"),
                             local_error->message);
                g_error_free (local_error);
                memset (s, '\0', strlen (s));
                g_free (s);
                goto out;
        }
        memset (s, '\0', strlen (s));
        g_free (s);

        /* Check that the udisks program really is attempting to connect (for cases
         * where the program doesn't exist on the host
         */
        s = g_data_input_stream_read_line (stderr_data_stream,
                                           NULL, /* gsize *length */
                                           data->cancellable,
                                           &local_error);
        if (s == NULL) {
                g_set_error (error,
                             GDU_ERROR,
                             GDU_ERROR_FAILED,
                             _("Error reading stderr from: %s"),
                             local_error->message);
                g_error_free (local_error);
                goto out;
        }
        if (sscanf (s, "udisks-tcp-bridge: Attempting to connect to port %d", &remote_port) != 1) {
                g_set_error (error,
                             GDU_ERROR,
                             GDU_ERROR_FAILED,
                             _("Unexpected stderr output - expected `udisks-tcp-bridge: Attempting to connect to port %d' but got `%s'"),
                             remote_port,
                             s);
                g_free (s);
                goto out;
        }
        g_free (s);

        /* Wait for D-Bus connection and authorization */
        g_main_loop_run (data->loop);

        if (data->server_connection != NULL) {
                if (data->authorized) {
                        ret = dbus_connection_get_g_connection (dbus_connection_ref (data->server_connection));
                } else {
                        dbus_connection_close (data->server_connection);
                        g_set_error (error,
                                     GDU_ERROR,
                                     GDU_ERROR_FAILED,
                                     _("The udisks-tcp-bridge program failed to prove it was authorized: %s"),
                                     data->error_message != NULL ? data->error_message : "(no detail)");
                }
        } else {
                g_set_error_literal (error,
                                     GDU_ERROR,
                                     GDU_ERROR_FAILED,
                                     _("The udisks-tcp-bridge program failed to prove it was authorized"));
        }

 out:
        if (ret == NULL) {
                /* kill ssh connection if we failed */
                if (ssh_pid > 0) {
                        kill (ssh_pid, SIGTERM);
                }
                if (out_pid != NULL)
                        *out_pid = 0;
        } else {
                if (out_pid != NULL)
                        *out_pid = ssh_pid;
        }

        if (data != NULL)
                bridge_data_free (data);
        if (server != NULL) {
                dbus_server_disconnect (server);
                dbus_server_unref (server);
        }
        if (stdin_data_stream != NULL)
                g_object_unref (stdin_data_stream);
        if (stdout_data_stream != NULL)
                g_object_unref (stdout_data_stream);
        if (stderr_data_stream != NULL)
                g_object_unref (stderr_data_stream);
        g_strfreev (ssh_argv);
        g_free (command_line);
        g_free (dbus_address);
        return ret;
}
