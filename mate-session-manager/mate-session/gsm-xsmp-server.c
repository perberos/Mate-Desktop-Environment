/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 Novell, Inc.
 * Copyright (C) 2008 Red Hat, Inc.
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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>

#include <X11/ICE/ICElib.h>
#include <X11/ICE/ICEutil.h>
#include <X11/ICE/ICEconn.h>
#include <X11/SM/SMlib.h>

#ifdef HAVE_X11_XTRANS_XTRANS_H
/* Get the proto for _IceTransNoListen */
#define ICE_t
#define TRANS_SERVER
#include <X11/Xtrans/Xtrans.h>
#undef  ICE_t
#undef TRANS_SERVER
#endif /* HAVE_X11_XTRANS_XTRANS_H */

#include "gsm-xsmp-server.h"
#include "gsm-xsmp-client.h"
#include "gsm-util.h"

/* ICEauthority stuff */
/* Various magic numbers stolen from iceauth.c */
#define GSM_ICE_AUTH_RETRIES      10
#define GSM_ICE_AUTH_INTERVAL     2   /* 2 seconds */
#define GSM_ICE_AUTH_LOCK_TIMEOUT 600 /* 10 minutes */

#define GSM_ICE_MAGIC_COOKIE_AUTH_NAME "MIT-MAGIC-COOKIE-1"
#define GSM_ICE_MAGIC_COOKIE_LEN       16

#define GSM_XSMP_SERVER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GSM_TYPE_XSMP_SERVER, GsmXsmpServerPrivate))

struct GsmXsmpServerPrivate
{
        GsmStore       *client_store;

        IceListenObj   *xsmp_sockets;
        int             num_xsmp_sockets;
        int             num_local_xsmp_sockets;

};

enum {
        PROP_0,
        PROP_CLIENT_STORE
};

static void     gsm_xsmp_server_class_init  (GsmXsmpServerClass *klass);
static void     gsm_xsmp_server_init        (GsmXsmpServer      *xsmp_server);
static void     gsm_xsmp_server_finalize    (GObject         *object);

static gpointer xsmp_server_object = NULL;

G_DEFINE_TYPE (GsmXsmpServer, gsm_xsmp_server, G_TYPE_OBJECT)

typedef struct {
        GsmXsmpServer *server;
        IceListenObj   listener;
} GsmIceConnectionData;

typedef struct {
        guint watch_id;
        guint protocol_timeout;
} GsmIceConnectionWatch;

static void
disconnect_ice_connection (IceConn ice_conn)
{
        IceSetShutdownNegotiation (ice_conn, FALSE);
        IceCloseConnection (ice_conn);
}

static void
free_ice_connection_watch (GsmIceConnectionWatch *data)
{
        if (data->watch_id) {
                g_source_remove (data->watch_id);
                data->watch_id = 0;
        }

        if (data->protocol_timeout) {
                g_source_remove (data->protocol_timeout);
                data->protocol_timeout = 0;
        }

        g_free (data);
}

static gboolean
ice_protocol_timeout (IceConn ice_conn)
{
        GsmIceConnectionWatch *data;

        g_debug ("GsmXsmpServer: ice_protocol_timeout for IceConn %p with status %d",
                 ice_conn, IceConnectionStatus (ice_conn));

        data = ice_conn->context;

        free_ice_connection_watch (data);
        disconnect_ice_connection (ice_conn);

        return FALSE;
}

static gboolean
auth_iochannel_watch (GIOChannel   *source,
                      GIOCondition  condition,
                      IceConn       ice_conn)
{

        GsmIceConnectionWatch *data;
        gboolean               keep_going;

        data = ice_conn->context;

        switch (IceProcessMessages (ice_conn, NULL, NULL)) {
        case IceProcessMessagesSuccess:
                keep_going = TRUE;
                break;
        case IceProcessMessagesIOError:
                g_debug ("GsmXsmpServer: IceProcessMessages returned IceProcessMessagesIOError");
                free_ice_connection_watch (data);
                disconnect_ice_connection (ice_conn);
                keep_going = FALSE;
                break;
        case IceProcessMessagesConnectionClosed:
                g_debug ("GsmXsmpServer: IceProcessMessages returned IceProcessMessagesConnectionClosed");
                free_ice_connection_watch (data);
                keep_going = FALSE;
                break;
        default:
                g_assert_not_reached ();
        }

        return keep_going;
}

/* IceAcceptConnection returns a new ICE connection that is in a "pending" state,
 * this is because authentification may be necessary.
 * So we've to authenticate it, before accept_xsmp_connection() is called.
 * Then each GsmXSMPClient will have its own IceConn watcher
 */
static void
auth_ice_connection (IceConn ice_conn)
{
        GIOChannel            *channel;
        GsmIceConnectionWatch *data;
        int                    fd;

        g_debug ("GsmXsmpServer: auth_ice_connection()");

        fd = IceConnectionNumber (ice_conn);
        fcntl (fd, F_SETFD, fcntl (fd, F_GETFD, 0) | FD_CLOEXEC);
        channel = g_io_channel_unix_new (fd);

        data = g_new0 (GsmIceConnectionWatch, 1);
        ice_conn->context = data;

        data->protocol_timeout = g_timeout_add_seconds (5,
                                                        (GSourceFunc)ice_protocol_timeout,
                                                        ice_conn);
        data->watch_id = g_io_add_watch (channel,
                                         G_IO_IN | G_IO_ERR,
                                         (GIOFunc)auth_iochannel_watch,
                                         ice_conn);
        g_io_channel_unref (channel);
}

/* This is called (by glib via xsmp->ice_connection_watch) when a
 * connection is first received on the ICE listening socket.
 */
static gboolean
accept_ice_connection (GIOChannel           *source,
                       GIOCondition          condition,
                       GsmIceConnectionData *data)
{
        IceConn         ice_conn;
        IceAcceptStatus status;

        g_debug ("GsmXsmpServer: accept_ice_connection()");

        ice_conn = IceAcceptConnection (data->listener, &status);
        if (status != IceAcceptSuccess) {
                g_debug ("GsmXsmpServer: IceAcceptConnection returned %d", status);
                return TRUE;
        }

        auth_ice_connection (ice_conn);

        return TRUE;
}

void
gsm_xsmp_server_start (GsmXsmpServer *server)
{
        GIOChannel *channel;
        int         i;

        for (i = 0; i < server->priv->num_local_xsmp_sockets; i++) {
                GsmIceConnectionData *data;

                data = g_new0 (GsmIceConnectionData, 1);
                data->server = server;
                data->listener = server->priv->xsmp_sockets[i];

                channel = g_io_channel_unix_new (IceGetListenConnectionNumber (server->priv->xsmp_sockets[i]));
                g_io_add_watch_full (channel,
                                     G_PRIORITY_DEFAULT,
                                     G_IO_IN | G_IO_HUP | G_IO_ERR,
                                     (GIOFunc)accept_ice_connection,
                                     data,
                                     (GDestroyNotify)g_free);
                g_io_channel_unref (channel);
        }
}

static void
gsm_xsmp_server_set_client_store (GsmXsmpServer *xsmp_server,
                                  GsmStore      *store)
{
        g_return_if_fail (GSM_IS_XSMP_SERVER (xsmp_server));

        if (store != NULL) {
                g_object_ref (store);
        }

        if (xsmp_server->priv->client_store != NULL) {
                g_object_unref (xsmp_server->priv->client_store);
        }

        xsmp_server->priv->client_store = store;
}

static void
gsm_xsmp_server_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
        GsmXsmpServer *self;

        self = GSM_XSMP_SERVER (object);

        switch (prop_id) {
        case PROP_CLIENT_STORE:
                gsm_xsmp_server_set_client_store (self, g_value_get_object (value));
                break;
         default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gsm_xsmp_server_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
        GsmXsmpServer *self;

        self = GSM_XSMP_SERVER (object);

        switch (prop_id) {
        case PROP_CLIENT_STORE:
                g_value_set_object (value, self->priv->client_store);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

/* This is called (by libSM) when XSMP is initiated on an ICE
 * connection that was already accepted by accept_ice_connection.
 */
static Status
accept_xsmp_connection (SmsConn        sms_conn,
                        GsmXsmpServer *server,
                        unsigned long *mask_ret,
                        SmsCallbacks  *callbacks_ret,
                        char         **failure_reason_ret)
{
        IceConn                ice_conn;
        GsmClient             *client;
        GsmIceConnectionWatch *data;

        /* FIXME: what about during shutdown but before gsm_xsmp_shutdown? */
        if (server->priv->xsmp_sockets == NULL) {
                g_debug ("GsmXsmpServer: In shutdown, rejecting new client");

                *failure_reason_ret = strdup (_("Refusing new client connection because the session is currently being shut down\n"));
                return FALSE;
        }

        ice_conn = SmsGetIceConnection (sms_conn);
        data = ice_conn->context;

        /* Each GsmXSMPClient has its own IceConn watcher */
        free_ice_connection_watch (data);

        client = gsm_xsmp_client_new (ice_conn);

        gsm_store_add (server->priv->client_store, gsm_client_peek_id (client), G_OBJECT (client));
        /* the store will own the ref */
        g_object_unref (client);

        gsm_xsmp_client_connect (GSM_XSMP_CLIENT (client), sms_conn, mask_ret, callbacks_ret);

        return TRUE;
}

static void
ice_error_handler (IceConn       conn,
                   Bool          swap,
                   int           offending_minor_opcode,
                   unsigned long offending_sequence,
                   int           error_class,
                   int           severity,
                   IcePointer    values)
{
        g_debug ("GsmXsmpServer: ice_error_handler (%p, %s, %d, %lx, %d, %d)",
                 conn, swap ? "TRUE" : "FALSE", offending_minor_opcode,
                 offending_sequence, error_class, severity);

        if (severity == IceCanContinue) {
                return;
        }

        /* FIXME: the ICElib docs are completely vague about what we're
         * supposed to do in this case. Need to verify that calling
         * IceCloseConnection() here is guaranteed to cause neither
         * free-memory-reads nor leaks.
         */
        IceCloseConnection (conn);
}

static void
ice_io_error_handler (IceConn conn)
{
        g_debug ("GsmXsmpServer: ice_io_error_handler (%p)", conn);

        /* We don't need to do anything here; the next call to
         * IceProcessMessages() for this connection will receive
         * IceProcessMessagesIOError and we can handle the error there.
         */
}

static void
sms_error_handler (SmsConn       conn,
                   Bool          swap,
                   int           offending_minor_opcode,
                   unsigned long offending_sequence_num,
                   int           error_class,
                   int           severity,
                   IcePointer    values)
{
        g_debug ("GsmXsmpServer: sms_error_handler (%p, %s, %d, %lx, %d, %d)",
                 conn, swap ? "TRUE" : "FALSE", offending_minor_opcode,
                 offending_sequence_num, error_class, severity);

        /* We don't need to do anything here; if the connection needs to be
         * closed, libSM will do that itself.
         */
}

static IceAuthFileEntry *
auth_entry_new (const char *protocol,
                const char *network_id)
{
        IceAuthFileEntry *file_entry;
        IceAuthDataEntry  data_entry;

        file_entry = malloc (sizeof (IceAuthFileEntry));

        file_entry->protocol_name = strdup (protocol);
        file_entry->protocol_data = NULL;
        file_entry->protocol_data_length = 0;
        file_entry->network_id = strdup (network_id);
        file_entry->auth_name = strdup (GSM_ICE_MAGIC_COOKIE_AUTH_NAME);
        file_entry->auth_data = IceGenerateMagicCookie (GSM_ICE_MAGIC_COOKIE_LEN);
        file_entry->auth_data_length = GSM_ICE_MAGIC_COOKIE_LEN;

        /* Also create an in-memory copy, which is what the server will
         * actually use for checking client auth.
         */
        data_entry.protocol_name = file_entry->protocol_name;
        data_entry.network_id = file_entry->network_id;
        data_entry.auth_name = file_entry->auth_name;
        data_entry.auth_data = file_entry->auth_data;
        data_entry.auth_data_length = file_entry->auth_data_length;
        IceSetPaAuthData (1, &data_entry);

        return file_entry;
}

static gboolean
update_iceauthority (GsmXsmpServer *server,
                     gboolean       adding)
{
        char             *filename;
        char            **our_network_ids;
        FILE             *fp;
        IceAuthFileEntry *auth_entry;
        GSList           *entries;
        GSList           *e;
        int               i;
        gboolean          ok = FALSE;

        filename = IceAuthFileName ();
        if (IceLockAuthFile (filename,
                             GSM_ICE_AUTH_RETRIES,
                             GSM_ICE_AUTH_INTERVAL,
                             GSM_ICE_AUTH_LOCK_TIMEOUT) != IceAuthLockSuccess) {
                return FALSE;
        }

        our_network_ids = g_malloc (server->priv->num_local_xsmp_sockets * sizeof (char *));
        for (i = 0; i < server->priv->num_local_xsmp_sockets; i++) {
                our_network_ids[i] = IceGetListenConnectionString (server->priv->xsmp_sockets[i]);
        }

        entries = NULL;

        fp = fopen (filename, "r+");
        if (fp != NULL) {
                while ((auth_entry = IceReadAuthFileEntry (fp)) != NULL) {
                        /* Skip/delete entries with no network ID (invalid), or with
                         * our network ID; if we're starting up, an entry with our
                         * ID must be a stale entry left behind by an old process,
                         * and if we're shutting down, it won't be valid in the
                         * future, so either way we want to remove it from the list.
                         */
                        if (!auth_entry->network_id) {
                                IceFreeAuthFileEntry (auth_entry);
                                continue;
                        }

                        for (i = 0; i < server->priv->num_local_xsmp_sockets; i++) {
                                if (!strcmp (auth_entry->network_id, our_network_ids[i])) {
                                        IceFreeAuthFileEntry (auth_entry);
                                        break;
                                }
                        }
                        if (i != server->priv->num_local_xsmp_sockets) {
                                continue;
                        }

                        entries = g_slist_prepend (entries, auth_entry);
                }

                rewind (fp);
        } else {
                int fd;

                if (g_file_test (filename, G_FILE_TEST_EXISTS)) {
                        g_warning ("Unable to read ICE authority file: %s", filename);
                        goto cleanup;
                }

                fd = open (filename, O_CREAT | O_WRONLY, 0600);
                fp = fdopen (fd, "w");
                if (!fp) {
                        g_warning ("Unable to write to ICE authority file: %s", filename);
                        if (fd != -1) {
                                close (fd);
                        }
                        goto cleanup;
                }
        }

        if (adding) {
                for (i = 0; i < server->priv->num_local_xsmp_sockets; i++) {
                        entries = g_slist_append (entries,
                                                  auth_entry_new ("ICE", our_network_ids[i]));
                        entries = g_slist_prepend (entries,
                                                   auth_entry_new ("XSMP", our_network_ids[i]));
                }
        }

        for (e = entries; e; e = e->next) {
                IceAuthFileEntry *auth_entry = e->data;
                IceWriteAuthFileEntry (fp, auth_entry);
                IceFreeAuthFileEntry (auth_entry);
        }
        g_slist_free (entries);

        fclose (fp);
        ok = TRUE;

 cleanup:
        IceUnlockAuthFile (filename);
        for (i = 0; i < server->priv->num_local_xsmp_sockets; i++) {
                free (our_network_ids[i]);
        }
        g_free (our_network_ids);

        return ok;
}


static void
setup_listener (GsmXsmpServer *server)
{
        char   error[256];
        mode_t saved_umask;
        char  *network_id_list;
        int    i;
        int    res;

        /* Set up sane error handlers */
        IceSetErrorHandler (ice_error_handler);
        IceSetIOErrorHandler (ice_io_error_handler);
        SmsSetErrorHandler (sms_error_handler);

        /* Initialize libSM; we pass NULL for hostBasedAuthProc to disable
         * host-based authentication.
         */
        res = SmsInitialize (PACKAGE,
                             VERSION,
                             (SmsNewClientProc)accept_xsmp_connection,
                             server,
                             NULL,
                             sizeof (error),
                             error);
        if (! res) {
                gsm_util_init_error (TRUE, "Could not initialize libSM: %s", error);
        }

#ifdef HAVE_X11_XTRANS_XTRANS_H
        /* By default, IceListenForConnections will open one socket for each
         * transport type known to X. We don't want connections from remote
         * hosts, so for security reasons it would be best if ICE didn't
         * even open any non-local sockets. So we use an internal ICElib
         * method to disable them here. Unfortunately, there is no way to
         * ask X what transport types it knows about, so we're forced to
         * guess.
         */
        _IceTransNoListen ("tcp");
#endif

        /* Create the XSMP socket. Older versions of IceListenForConnections
         * have a bug which causes the umask to be set to 0 on certain types
         * of failures. Probably not an issue on any modern systems, but
         * we'll play it safe.
         */
        saved_umask = umask (0);
        umask (saved_umask);
        res = IceListenForConnections (&server->priv->num_xsmp_sockets,
                                       &server->priv->xsmp_sockets,
                                       sizeof (error),
                                       error);
        if (! res) {
                gsm_util_init_error (TRUE, _("Could not create ICE listening socket: %s"), error);
        }

        umask (saved_umask);

        /* Find the local sockets in the returned socket list and move them
         * to the start of the list.
         */
        for (i = server->priv->num_local_xsmp_sockets = 0; i < server->priv->num_xsmp_sockets; i++) {
                char *id = IceGetListenConnectionString (server->priv->xsmp_sockets[i]);

                if (!strncmp (id, "local/", sizeof ("local/") - 1) ||
                    !strncmp (id, "unix/", sizeof ("unix/") - 1)) {
                        if (i > server->priv->num_local_xsmp_sockets) {
                                IceListenObj tmp;
                                tmp = server->priv->xsmp_sockets[i];
                                server->priv->xsmp_sockets[i] = server->priv->xsmp_sockets[server->priv->num_local_xsmp_sockets];
                                server->priv->xsmp_sockets[server->priv->num_local_xsmp_sockets] = tmp;
                        }
                        server->priv->num_local_xsmp_sockets++;
                }
                free (id);
        }

        if (server->priv->num_local_xsmp_sockets == 0) {
                gsm_util_init_error (TRUE, "IceListenForConnections did not return a local listener!");
        }

#ifdef HAVE_X11_XTRANS_XTRANS_H
        if (server->priv->num_local_xsmp_sockets != server->priv->num_xsmp_sockets) {
                /* Xtrans was apparently compiled with support for some
                 * non-local transport besides TCP (which we disabled above); we
                 * won't create IO watches on those extra sockets, so
                 * connections to them will never be noticed, but they're still
                 * there, which is inelegant.
                 *
                 * If the g_warning below is triggering for you and you want to
                 * stop it, the fix is to add additional _IceTransNoListen()
                 * calls above.
                 */
                network_id_list = IceComposeNetworkIdList (server->priv->num_xsmp_sockets - server->priv->num_local_xsmp_sockets,
                                                           server->priv->xsmp_sockets + server->priv->num_local_xsmp_sockets);
                g_warning ("IceListenForConnections returned %d non-local listeners: %s",
                           server->priv->num_xsmp_sockets - server->priv->num_local_xsmp_sockets,
                           network_id_list);
                free (network_id_list);
        }
#endif

        /* Update .ICEauthority with new auth entries for our socket */
        if (!update_iceauthority (server, TRUE)) {
                /* FIXME: is this really fatal? Hm... */
                gsm_util_init_error (TRUE,
                                     "Could not update ICEauthority file %s",
                                     IceAuthFileName ());
        }

        network_id_list = IceComposeNetworkIdList (server->priv->num_local_xsmp_sockets,
                                                   server->priv->xsmp_sockets);

        gsm_util_setenv ("SESSION_MANAGER", network_id_list);
        g_debug ("GsmXsmpServer: SESSION_MANAGER=%s\n", network_id_list);
        free (network_id_list);
}

static GObject *
gsm_xsmp_server_constructor (GType                  type,
                             guint                  n_construct_properties,
                             GObjectConstructParam *construct_properties)
{
        GsmXsmpServer *xsmp_server;

        xsmp_server = GSM_XSMP_SERVER (G_OBJECT_CLASS (gsm_xsmp_server_parent_class)->constructor (type,
                                                                                       n_construct_properties,
                                                                                       construct_properties));
        setup_listener (xsmp_server);

        return G_OBJECT (xsmp_server);
}

static void
gsm_xsmp_server_class_init (GsmXsmpServerClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = gsm_xsmp_server_get_property;
        object_class->set_property = gsm_xsmp_server_set_property;
        object_class->constructor = gsm_xsmp_server_constructor;
        object_class->finalize = gsm_xsmp_server_finalize;

        g_object_class_install_property (object_class,
                                         PROP_CLIENT_STORE,
                                         g_param_spec_object ("client-store",
                                                              NULL,
                                                              NULL,
                                                              GSM_TYPE_STORE,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

        g_type_class_add_private (klass, sizeof (GsmXsmpServerPrivate));
}

static void
gsm_xsmp_server_init (GsmXsmpServer *xsmp_server)
{
        xsmp_server->priv = GSM_XSMP_SERVER_GET_PRIVATE (xsmp_server);

}

static void
gsm_xsmp_server_finalize (GObject *object)
{
        GsmXsmpServer *xsmp_server;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GSM_IS_XSMP_SERVER (object));

        xsmp_server = GSM_XSMP_SERVER (object);

        g_return_if_fail (xsmp_server->priv != NULL);

        IceFreeListenObjs (xsmp_server->priv->num_xsmp_sockets, 
                           xsmp_server->priv->xsmp_sockets);

        if (xsmp_server->priv->client_store != NULL) {
                g_object_unref (xsmp_server->priv->client_store);
        }

        G_OBJECT_CLASS (gsm_xsmp_server_parent_class)->finalize (object);
}

GsmXsmpServer *
gsm_xsmp_server_new (GsmStore *client_store)
{
        if (xsmp_server_object != NULL) {
                g_object_ref (xsmp_server_object);
        } else {
                xsmp_server_object = g_object_new (GSM_TYPE_XSMP_SERVER,
                                                   "client-store", client_store,
                                                   NULL);

                g_object_add_weak_pointer (xsmp_server_object,
                                           (gpointer *) &xsmp_server_object);
        }

        return GSM_XSMP_SERVER (xsmp_server_object);
}
