/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 1998, 1999, 2000 Martin K, Petersen <mkp@mkp.net>
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
#include <errno.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif

#include <X11/Xmd.h>
#include <X11/Xdmcp.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "mdm-address.h"
#include "mdm-chooser-host.h"
#include "mdm-host-chooser-widget.h"

#define MDM_HOST_CHOOSER_WIDGET_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_HOST_CHOOSER_WIDGET, MdmHostChooserWidgetPrivate))

struct MdmHostChooserWidgetPrivate
{
        GtkWidget      *treeview;

        int             kind_mask;

        char          **hosts;

        XdmcpBuffer     broadcast_buf;
        XdmcpBuffer     query_buf;
        gboolean        have_ipv6;
        int             socket_fd;
        guint           io_watch_id;
        guint           scan_time_id;
        guint           ping_try_id;

        int             ping_tries;

        GSList         *broadcast_addresses;
        GSList         *query_addresses;
        GSList         *chooser_hosts;

        MdmChooserHost *current_host;
};

enum {
        PROP_0,
        PROP_KIND_MASK,
};

enum {
        HOST_ACTIVATED,
        LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0, };

static void     mdm_host_chooser_widget_class_init  (MdmHostChooserWidgetClass *klass);
static void     mdm_host_chooser_widget_init        (MdmHostChooserWidget      *host_chooser_widget);
static void     mdm_host_chooser_widget_finalize    (GObject                   *object);

G_DEFINE_TYPE (MdmHostChooserWidget, mdm_host_chooser_widget, GTK_TYPE_VBOX)

#define MDM_XDMCP_PROTOCOL_VERSION 1001
#define SCAN_TIMEOUT 30
#define PING_TIMEOUT 2
#define PING_TRIES 3

enum {
        CHOOSER_LIST_ICON_COLUMN = 0,
        CHOOSER_LIST_LABEL_COLUMN,
        CHOOSER_LIST_HOST_COLUMN
};

static void
chooser_host_add (MdmHostChooserWidget *widget,
                  MdmChooserHost       *host)
{
        widget->priv->chooser_hosts = g_slist_prepend (widget->priv->chooser_hosts, host);
}

#if 0
static void
chooser_host_remove (MdmHostChooserWidget *widget,
                     MdmChooserHost       *host)
{
        widget->priv->chooser_hosts = g_slist_remove (widget->priv->chooser_hosts, host);
}
#endif

static MdmChooserHost *
find_known_host (MdmHostChooserWidget *widget,
                 MdmAddress           *address)
{
        GSList         *li;
        MdmChooserHost *host;

        for (li = widget->priv->chooser_hosts; li != NULL; li = li->next) {
                host = li->data;
                if (mdm_address_equal (mdm_chooser_host_get_address (host), address)) {
                        goto out;
                }
        }

        host = NULL;
 out:

        return host;
}

static void
browser_add_host (MdmHostChooserWidget *widget,
                  MdmChooserHost       *host)
{
        char         *hostname;
        char         *name;
        char         *desc;
        char         *label;
        GtkTreeModel *model;
        GtkTreeIter   iter;
        gboolean      res;

        g_assert (host != NULL);

        if (! mdm_chooser_host_get_willing (host)) {
                gtk_widget_set_sensitive (GTK_WIDGET (widget), TRUE);
                return;
        }

        res = mdm_address_get_hostname (mdm_chooser_host_get_address (host), &hostname);
        if (! res) {
                mdm_address_get_numeric_info (mdm_chooser_host_get_address (host), &hostname, NULL);
        }

        name = g_markup_escape_text (hostname, -1);
        desc = g_markup_escape_text (mdm_chooser_host_get_description (host), -1);
        label = g_strdup_printf ("<b>%s</b>\n%s", name, desc);
        g_free (name);
        g_free (desc);

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (widget->priv->treeview));

        gtk_list_store_append (GTK_LIST_STORE (model), &iter);
        gtk_list_store_set (GTK_LIST_STORE (model),
                            &iter,
                            CHOOSER_LIST_ICON_COLUMN, NULL,
                            CHOOSER_LIST_LABEL_COLUMN, label,
                            CHOOSER_LIST_HOST_COLUMN, host,
                            -1);
        g_free (label);

}

static gboolean
decode_packet (GIOChannel           *source,
               GIOCondition          condition,
               MdmHostChooserWidget *widget)
{
        struct sockaddr_storage clnt_ss;
        MdmAddress             *address;
        int                     ss_len;
        XdmcpHeader             header;
        int                     res;
        static XdmcpBuffer      buf;
        ARRAY8                  auth = {0};
        ARRAY8                  host = {0};
        ARRAY8                  stat = {0};
        char                   *status;
        MdmChooserHost         *chooser_host;

        status = NULL;
        address = NULL;

        g_debug ("decode_packet: GIOCondition %d", (int)condition);

        if ( ! (condition & G_IO_IN)) {
                return TRUE;
        }

        ss_len = (int) sizeof (clnt_ss);

        res = XdmcpFill (widget->priv->socket_fd, &buf, (XdmcpNetaddr)&clnt_ss, &ss_len);
        if G_UNLIKELY (! res) {
                g_debug (_("XDMCP: Could not create XDMCP buffer!"));
                return TRUE;
        }

        res = XdmcpReadHeader (&buf, &header);
        if G_UNLIKELY (! res) {
                g_warning (_("XDMCP: Could not read XDMCP header!"));
                return TRUE;
        }

        if G_UNLIKELY (header.version != XDM_PROTOCOL_VERSION &&
                       header.version != MDM_XDMCP_PROTOCOL_VERSION) {
                g_warning (_("XMDCP: Incorrect XDMCP version!"));
                return TRUE;
        }

        address = mdm_address_new_from_sockaddr ((struct sockaddr *) &clnt_ss, ss_len);
        if (address == NULL) {
                g_warning (_("XMDCP: Unable to parse address"));
                return TRUE;
        }

        mdm_address_debug (address);

        if (header.opcode == WILLING) {
                if (! XdmcpReadARRAY8 (&buf, &auth)) {
                        goto done;
                }

                if (! XdmcpReadARRAY8 (&buf, &host)) {
                        goto done;
                }

                if (! XdmcpReadARRAY8 (&buf, &stat)) {
                        goto done;
                }

                status = g_strndup ((char *) stat.data, MIN (stat.length, 256));
        } else if (header.opcode == UNWILLING) {
                /* immaterial, will not be shown */
                status = NULL;
        } else {
                goto done;
        }

        g_debug ("STATUS: %s", status);

        chooser_host = find_known_host (widget, address);
        if (chooser_host == NULL) {
                chooser_host = g_object_new (MDM_TYPE_CHOOSER_HOST,
                                             "address", address,
                                             "description", status,
                                             "willing", (header.opcode == WILLING),
                                             "kind", MDM_CHOOSER_HOST_KIND_XDMCP,
                                             NULL);
                chooser_host_add (widget, chooser_host);
                browser_add_host (widget, chooser_host);
        } else {
                /* server changed it's mind */
                if (header.opcode == WILLING
                    && ! mdm_chooser_host_get_willing (chooser_host)) {
                        g_object_set (chooser_host, "willing", TRUE, NULL);
                        browser_add_host (widget, chooser_host);
                }
                /* FIXME: handle unwilling? */
        }

 done:
        if (header.opcode == WILLING) {
                XdmcpDisposeARRAY8 (&auth);
                XdmcpDisposeARRAY8 (&host);
                XdmcpDisposeARRAY8 (&stat);
        }

        g_free (status);
        mdm_address_free (address);

        return TRUE;
}

static void
do_ping (MdmHostChooserWidget *widget,
         gboolean              full)
{
        GSList *l;

        g_debug ("do ping full:%d", full);

        for (l = widget->priv->broadcast_addresses; l != NULL; l = l->next) {
                MdmAddress              *address;
                int                      res;

                address = l->data;

                mdm_address_debug (address);
                errno = 0;
                g_debug ("fd:%d", widget->priv->socket_fd);
                res = XdmcpFlush (widget->priv->socket_fd,
                                  &widget->priv->broadcast_buf,
                                  (XdmcpNetaddr)mdm_address_peek_sockaddr_storage (address),
                                  (int)mdm_sockaddr_len (mdm_address_peek_sockaddr_storage (address)));

                if (! res) {
                        g_warning ("Unable to flush the XDMCP broadcast packet: %s", g_strerror (errno));
                }
        }

        if (full) {
                for (l = widget->priv->query_addresses; l != NULL; l = l->next) {
                        MdmAddress *address;
                        int         res;

                        address = l->data;

                        mdm_address_debug (address);
                        res = XdmcpFlush (widget->priv->socket_fd,
                                          &widget->priv->query_buf,
                                          (XdmcpNetaddr)mdm_address_peek_sockaddr_storage (address),
                                          (int)mdm_sockaddr_len (mdm_address_peek_sockaddr_storage (address)));
                        if (! res) {
                                g_warning ("Unable to flush the XDMCP query packet");
                        }
                }
        }
}

static gboolean
ping_try (MdmHostChooserWidget *widget)
{
        do_ping (widget, FALSE);

        widget->priv->ping_tries --;

        if (widget->priv->ping_tries <= 0) {
                widget->priv->ping_try_id = 0;
                return FALSE;
        } else {
                return TRUE;
        }
}

static void
xdmcp_discover (MdmHostChooserWidget *widget)
{
#if 0
        gtk_widget_set_sensitive (GTK_WIDGET (manage), FALSE);
        gtk_widget_set_sensitive (GTK_WIDGET (rescan), FALSE);
        gtk_list_store_clear (GTK_LIST_STORE (browser_model));
        gtk_widget_set_sensitive (GTK_WIDGET (browser), FALSE);
        gtk_label_set_label (GTK_LABEL (status_label),
                             _(scanning_message));

        while (hl) {
                mdm_chooser_host_dispose ((MdmChooserHost *) hl->data);
                hl = hl->next;
        }

        g_list_free (chooser_hosts);
        chooser_hosts = NULL;
#endif

        do_ping (widget, TRUE);

#if 0
        if (widget->priv->scan_time_id > 0) {
                g_source_remove (widget->priv->scan_time_id);
        }

        widget->priv->scan_time_id = g_timeout_add_seconds (SCAN_TIMEOUT,
                                                            chooser_scan_time_update,
                                                            widget);
#endif
        /* Note we already used up one try */
        widget->priv->ping_tries = PING_TRIES - 1;
        if (widget->priv->ping_try_id > 0) {
                g_source_remove (widget->priv->ping_try_id);
        }

        widget->priv->ping_try_id = g_timeout_add_seconds (PING_TIMEOUT,
                                                           (GSourceFunc)ping_try,
                                                           widget);
}

/* Find broadcast address for all active, non pointopoint interfaces */
static void
find_broadcast_addresses (MdmHostChooserWidget *widget)
{
        int           i;
        int           num;
        int           sock;
        struct ifconf ifc;
        char         *buf;
        struct ifreq *ifr;

        g_debug ("Finding broadcast addresses");

        sock = socket (AF_INET, SOCK_DGRAM, 0);
#ifdef SIOCGIFNUM
        if (ioctl (sock, SIOCGIFNUM, &num) < 0) {
                num = 64;
        }
#else
        num = 64;
#endif

        ifc.ifc_len = sizeof (struct ifreq) * num;
        ifc.ifc_buf = buf = g_malloc0 (ifc.ifc_len);
        if (ioctl (sock, SIOCGIFCONF, &ifc) < 0) {
                g_warning ("Could not get local addresses!");
                goto out;
        }

        ifr = ifc.ifc_req;
        num = ifc.ifc_len / sizeof (struct ifreq);
        for (i = 0 ; i < num ; i++) {
                const char *name;

                name = ifr[i].ifr_name;
                g_debug ("Checking if %s", name);
                if (name != NULL && name[0] != '\0') {
                        struct ifreq            ifreq;
                        MdmAddress             *address;
                        struct sockaddr_in      sin;

                        memset (&ifreq, 0, sizeof (ifreq));

                        strncpy (ifreq.ifr_name,
                                 ifr[i].ifr_name,
                                 sizeof (ifreq.ifr_name));
                        /* paranoia */
                        ifreq.ifr_name[sizeof (ifreq.ifr_name) - 1] = '\0';

                        if ((ioctl (sock, SIOCGIFFLAGS, &ifreq) < 0) && (errno != ENXIO)) {
                                g_warning ("Could not get SIOCGIFFLAGS for %s", ifr[i].ifr_name);
                        }

                        if ((ifreq.ifr_flags & IFF_UP) == 0 ||
                            (ifreq.ifr_flags & IFF_BROADCAST) == 0 ||
                            ioctl (sock, SIOCGIFBRDADDR, &ifreq) < 0) {
                                g_debug ("Skipping if %s", name);
                                continue;
                        }

                        g_memmove (&sin, &ifreq.ifr_broadaddr, sizeof (struct sockaddr_in));
                        sin.sin_port = htons (XDM_UDP_PORT);
                        address = mdm_address_new_from_sockaddr ((struct sockaddr *) &sin, sizeof (sin));
                        if (address != NULL) {
                                g_debug ("Adding if %s", name);
                                mdm_address_debug (address);

                                widget->priv->broadcast_addresses = g_slist_append (widget->priv->broadcast_addresses, address);
                        }
                }
        }
 out:
        g_free (buf);
        close (sock);
}

static void
add_hosts (MdmHostChooserWidget *widget)
{
        int i;

        for (i = 0; widget->priv->hosts != NULL && widget->priv->hosts[i] != NULL; i++) {
                struct addrinfo  hints;
                struct addrinfo *result;
                struct addrinfo *ai;
                int              gaierr;
                const char      *name;
                char             serv_buf [NI_MAXSERV];
                const char      *serv;

                name = widget->priv->hosts[i];

                if (strcmp (name, "BROADCAST") == 0) {
                        find_broadcast_addresses (widget);
                        continue;
                }

                if (strcmp (name, "MULTICAST") == 0) {
                        /*mdm_chooser_find_mcaddr ();*/
                        continue;
                }

                result = NULL;
                memset (&hints, 0, sizeof (hints));
                hints.ai_socktype = SOCK_STREAM;

                snprintf (serv_buf, sizeof (serv_buf), "%u", XDM_UDP_PORT);
                serv = serv_buf;

                gaierr = getaddrinfo (name, serv, &hints, &result);
                if (gaierr != 0) {
                        g_warning ("Unable to get address info for name %s: %s", name, gai_strerror (gaierr));
                        continue;
                }

                for (ai = result; ai != NULL; ai = ai->ai_next) {
                        MdmAddress *address;

                        address = mdm_address_new_from_sockaddr (ai->ai_addr, ai->ai_addrlen);
                        if (address != NULL) {
                                widget->priv->query_addresses = g_slist_append (widget->priv->query_addresses, address);
                        }
                }

                freeaddrinfo (result);
        }

        if (widget->priv->broadcast_addresses == NULL && widget->priv->query_addresses == NULL) {
                find_broadcast_addresses (widget);
        }
}

static void
xdmcp_init (MdmHostChooserWidget *widget)
{
        XdmcpHeader   header;
        int           sockopts;
        int           res;
        GIOChannel   *ioc;
        ARRAYofARRAY8 aanames;

        sockopts = 1;

        widget->priv->socket_fd = -1;

        /* Open socket for communication */
#ifdef ENABLE_IPV6
        widget->priv->socket_fd = socket (AF_INET6, SOCK_DGRAM, 0);
        if (widget->priv->socket_fd != -1) {
                widget->priv->have_ipv6 = TRUE;
#ifdef IPV6_V6ONLY
		{
			int zero = 0;
			if (setsockopt(widget->priv->socket_fd, IPPROTO_IPV6, IPV6_V6ONLY, &zero, sizeof(zero)) < 0)
				g_warning("setsockopt(IPV6_V6ONLY): %s", g_strerror(errno));
		}
#endif
        }
#endif
        if (! widget->priv->have_ipv6) {
                widget->priv->socket_fd = socket (AF_INET, SOCK_DGRAM, 0);
                if (widget->priv->socket_fd == -1) {
                        g_critical ("Could not create socket!");
                }
        }

        res = setsockopt (widget->priv->socket_fd,
                          SOL_SOCKET,
                          SO_BROADCAST,
                          (char *) &sockopts,
                          sizeof (sockopts));
        if (res < 0) {
                g_critical ("Could not set socket options!");
        }

        /* Assemble XDMCP BROADCAST_QUERY packet in static buffer */
        memset (&header, 0, sizeof (XdmcpHeader));
        header.opcode  = (CARD16) BROADCAST_QUERY;
        header.length  = 1;
        header.version = XDM_PROTOCOL_VERSION;
        aanames.length = 0;
        XdmcpWriteHeader (&widget->priv->broadcast_buf, &header);
        XdmcpWriteARRAYofARRAY8 (&widget->priv->broadcast_buf, &aanames);

        /* Assemble XDMCP QUERY packet in static buffer */
        memset (&header, 0, sizeof (XdmcpHeader));
        header.opcode  = (CARD16) QUERY;
        header.length  = 1;
        header.version = XDM_PROTOCOL_VERSION;
        memset (&widget->priv->query_buf, 0, sizeof (XdmcpBuffer));
        XdmcpWriteHeader (&widget->priv->query_buf, &header);
        XdmcpWriteARRAYofARRAY8 (&widget->priv->query_buf, &aanames);

        add_hosts (widget);

        ioc = g_io_channel_unix_new (widget->priv->socket_fd);
        g_io_channel_set_encoding (ioc, NULL, NULL);
        g_io_channel_set_buffered (ioc, FALSE);
        widget->priv->io_watch_id = g_io_add_watch (ioc,
                                                    G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP | G_IO_NVAL,
                                                    (GIOFunc)decode_packet,
                                                    widget);
        g_io_channel_unref (ioc);
}

void
mdm_host_chooser_widget_refresh (MdmHostChooserWidget *widget)
{
        g_return_if_fail (MDM_IS_HOST_CHOOSER_WIDGET (widget));

        xdmcp_discover (widget);
}

MdmChooserHost *
mdm_host_chooser_widget_get_host (MdmHostChooserWidget *widget)
{
        MdmChooserHost *host;

        g_return_val_if_fail (MDM_IS_HOST_CHOOSER_WIDGET (widget), NULL);

        host = NULL;
        if (widget->priv->current_host != NULL) {
                host = g_object_ref (widget->priv->current_host);
        }

        return host;
}

static void
_mdm_host_chooser_widget_set_kind_mask (MdmHostChooserWidget *widget,
                                        int                   kind_mask)
{
        if (widget->priv->kind_mask != kind_mask) {
                widget->priv->kind_mask = kind_mask;
        }
}

static void
mdm_host_chooser_widget_set_property (GObject        *object,
                                      guint           prop_id,
                                      const GValue   *value,
                                      GParamSpec     *pspec)
{
        MdmHostChooserWidget *self;

        self = MDM_HOST_CHOOSER_WIDGET (object);

        switch (prop_id) {
        case PROP_KIND_MASK:
                _mdm_host_chooser_widget_set_kind_mask (self, g_value_get_int (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
mdm_host_chooser_widget_get_property (GObject        *object,
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
mdm_host_chooser_widget_constructor (GType                  type,
                                     guint                  n_construct_properties,
                                     GObjectConstructParam *construct_properties)
{
        MdmHostChooserWidget      *widget;

        widget = MDM_HOST_CHOOSER_WIDGET (G_OBJECT_CLASS (mdm_host_chooser_widget_parent_class)->constructor (type,
                                                                                                                           n_construct_properties,
                                                                                                                           construct_properties));

        xdmcp_init (widget);
        xdmcp_discover (widget);

        return G_OBJECT (widget);
}

static void
mdm_host_chooser_widget_dispose (GObject *object)
{
        MdmHostChooserWidget *widget;

        widget = MDM_HOST_CHOOSER_WIDGET (object);

        g_debug ("Disposing host_chooser_widget");

        if (widget->priv->broadcast_addresses != NULL) {
                g_slist_foreach (widget->priv->broadcast_addresses,
                                 (GFunc)mdm_address_free,
                                 NULL);
                g_slist_free (widget->priv->broadcast_addresses);
                widget->priv->broadcast_addresses = NULL;
        }
        if (widget->priv->query_addresses != NULL) {
                g_slist_foreach (widget->priv->query_addresses,
                                 (GFunc)mdm_address_free,
                                 NULL);
                g_slist_free (widget->priv->query_addresses);
                widget->priv->query_addresses = NULL;
        }
        if (widget->priv->chooser_hosts != NULL) {
                g_slist_foreach (widget->priv->chooser_hosts,
                                 (GFunc)g_object_unref,
                                 NULL);
                g_slist_free (widget->priv->chooser_hosts);
                widget->priv->chooser_hosts = NULL;
        }

        widget->priv->current_host = NULL;

        G_OBJECT_CLASS (mdm_host_chooser_widget_parent_class)->dispose (object);
}

static void
mdm_host_chooser_widget_class_init (MdmHostChooserWidgetClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = mdm_host_chooser_widget_get_property;
        object_class->set_property = mdm_host_chooser_widget_set_property;
        object_class->constructor = mdm_host_chooser_widget_constructor;
        object_class->dispose = mdm_host_chooser_widget_dispose;
        object_class->finalize = mdm_host_chooser_widget_finalize;

        g_object_class_install_property (object_class,
                                         PROP_KIND_MASK,
                                         g_param_spec_int ("kind-mask",
                                                           "kind mask",
                                                           "kind mask",
                                                           0,
                                                           G_MAXINT,
                                                           0,
                                                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

        signals [HOST_ACTIVATED] = g_signal_new ("host-activated",
                                                 G_TYPE_FROM_CLASS (object_class),
                                                 G_SIGNAL_RUN_LAST,
                                                 G_STRUCT_OFFSET (MdmHostChooserWidgetClass, host_activated),
                                                 NULL,
                                                 NULL,
                                                 g_cclosure_marshal_VOID__VOID,
                                                 G_TYPE_NONE,
                                                 0);

        g_type_class_add_private (klass, sizeof (MdmHostChooserWidgetPrivate));
}

static void
on_host_selected (GtkTreeSelection     *selection,
                  MdmHostChooserWidget *widget)
{
        GtkTreeModel   *model = NULL;
        GtkTreeIter     iter = {0};
        MdmChooserHost *curhost;

        curhost = NULL;

        if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
                gtk_tree_model_get (model, &iter, CHOOSER_LIST_HOST_COLUMN, &curhost, -1);
        }

        widget->priv->current_host = curhost;
}

static void
on_row_activated (GtkTreeView          *tree_view,
                  GtkTreePath          *tree_path,
                  GtkTreeViewColumn    *tree_column,
                  MdmHostChooserWidget *widget)
{
        g_signal_emit (widget, signals[HOST_ACTIVATED], 0);
}

static void
mdm_host_chooser_widget_init (MdmHostChooserWidget *widget)
{
        GtkWidget         *scrolled;
        GtkTreeSelection  *selection;
        GtkTreeViewColumn *column;
        GtkTreeModel      *model;

        widget->priv = MDM_HOST_CHOOSER_WIDGET_GET_PRIVATE (widget);

        scrolled = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
                                             GTK_SHADOW_IN);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                        GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
        gtk_box_pack_start (GTK_BOX (widget), scrolled, TRUE, TRUE, 0);

        widget->priv->treeview = gtk_tree_view_new ();
        gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (widget->priv->treeview), FALSE);
        g_signal_connect (widget->priv->treeview,
                          "row-activated",
                          G_CALLBACK (on_row_activated),
                          widget);
        gtk_container_add (GTK_CONTAINER (scrolled), widget->priv->treeview);

        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget->priv->treeview));
        gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
        g_signal_connect (selection, "changed",
                          G_CALLBACK (on_host_selected),
                          widget);

        model = (GtkTreeModel *)gtk_list_store_new (3,
                                                    GDK_TYPE_PIXBUF,
                                                    G_TYPE_STRING,
                                                    G_TYPE_POINTER);
        gtk_tree_view_set_model (GTK_TREE_VIEW (widget->priv->treeview), model);

        column = gtk_tree_view_column_new_with_attributes ("Icon",
                                                           gtk_cell_renderer_pixbuf_new (),
                                                           "pixbuf", CHOOSER_LIST_ICON_COLUMN,
                                                           NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (widget->priv->treeview), column);

        column = gtk_tree_view_column_new_with_attributes ("Hostname",
                                                           gtk_cell_renderer_text_new (),
                                                           "markup", CHOOSER_LIST_LABEL_COLUMN,
                                                           NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (widget->priv->treeview), column);

        gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (model),
                                              CHOOSER_LIST_LABEL_COLUMN,
                                              GTK_SORT_ASCENDING);
}

static void
mdm_host_chooser_widget_finalize (GObject *object)
{
        MdmHostChooserWidget *host_chooser_widget;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_HOST_CHOOSER_WIDGET (object));

        host_chooser_widget = MDM_HOST_CHOOSER_WIDGET (object);

        g_return_if_fail (host_chooser_widget->priv != NULL);

        G_OBJECT_CLASS (mdm_host_chooser_widget_parent_class)->finalize (object);
}

GtkWidget *
mdm_host_chooser_widget_new (int kind_mask)
{
        GObject *object;

        object = g_object_new (MDM_TYPE_HOST_CHOOSER_WIDGET,
                               "kind-mask", kind_mask,
                               NULL);

        return GTK_WIDGET (object);
}
