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

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <glib-object.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <X11/Xlib.h> /* for Display */

#include "gdm-common.h"

#include "gdm-xdmcp-chooser-slave.h"
#include "gdm-xdmcp-chooser-slave-glue.h"

#include "gdm-server.h"
#include "gdm-chooser-server.h"
#include "gdm-chooser-session.h"
#include "gdm-settings-direct.h"
#include "gdm-settings-keys.h"

#define GDM_XDMCP_CHOOSER_SLAVE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GDM_TYPE_XDMCP_CHOOSER_SLAVE, GdmXdmcpChooserSlavePrivate))

#define GDM_DBUS_NAME              "org.mate.DisplayManager"
#define GDM_DBUS_DISPLAY_INTERFACE "org.mate.DisplayManager.Display"

#define MAX_CONNECT_ATTEMPTS  10
#define DEFAULT_PING_INTERVAL 15

struct GdmXdmcpChooserSlavePrivate
{
        char              *id;
        GPid               pid;

        int                ping_interval;

        guint              connection_attempts;

        GdmChooserServer  *chooser_server;
        GdmChooserSession *chooser;
};

enum {
        PROP_0,
};

enum {
        HOSTNAME_SELECTED,
        LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0, };

static void     gdm_xdmcp_chooser_slave_class_init     (GdmXdmcpChooserSlaveClass *klass);
static void     gdm_xdmcp_chooser_slave_init           (GdmXdmcpChooserSlave      *xdmcp_chooser_slave);
static void     gdm_xdmcp_chooser_slave_finalize       (GObject                   *object);

G_DEFINE_TYPE (GdmXdmcpChooserSlave, gdm_xdmcp_chooser_slave, GDM_TYPE_SLAVE)


static void
on_chooser_session_start (GdmChooserSession    *chooser,
                          GdmXdmcpChooserSlave *slave)
{
        g_debug ("GdmXdmcpChooserSlave: Chooser started");
}

static void
on_chooser_session_stop (GdmChooserSession    *chooser,
                         GdmXdmcpChooserSlave *slave)
{
        g_debug ("GdmXdmcpChooserSlave: Chooser stopped");
        gdm_slave_stopped (GDM_SLAVE (slave));
}

static void
on_chooser_session_exited (GdmChooserSession    *chooser,
                           int                   code,
                           GdmXdmcpChooserSlave *slave)
{
        g_debug ("GdmXdmcpChooserSlave: Chooser exited: %d", code);
        gdm_slave_stopped (GDM_SLAVE (slave));
}

static void
on_chooser_session_died (GdmChooserSession    *chooser,
                         int                   signal,
                         GdmXdmcpChooserSlave *slave)
{
        g_debug ("GdmXdmcpChooserSlave: Chooser died: %d", signal);
        gdm_slave_stopped (GDM_SLAVE (slave));
}

static void
on_chooser_hostname_selected (GdmChooserServer     *chooser_server,
                              const char           *name,
                              GdmXdmcpChooserSlave *slave)
{
        g_debug ("GdmXdmcpChooserSlave: emitting hostname selected: %s", name);
        g_signal_emit (slave, signals [HOSTNAME_SELECTED], 0, name);
}

static void
on_chooser_disconnected (GdmChooserServer     *chooser_server,
                         GdmXdmcpChooserSlave *slave)
{
        g_debug ("GdmXdmcpChooserSlave: Chooser disconnected");

        /* stop pinging */
        alarm (0);

        gdm_slave_stopped (GDM_SLAVE (slave));
}

static void
on_chooser_connected (GdmChooserServer     *chooser_server,
                      GdmXdmcpChooserSlave *slave)
{
        g_debug ("GdmXdmcpChooserSlave: Chooser connected");
}

static void
setup_server (GdmXdmcpChooserSlave *slave)
{
        /* Set the busy cursor */
        gdm_slave_set_busy_cursor (GDM_SLAVE (slave));
}

static void
run_chooser (GdmXdmcpChooserSlave *slave)
{
        char          *display_id;
        char          *display_name;
        char          *display_device;
        char          *display_hostname;
        char          *auth_file;
        char          *address;
        gboolean       res;

        g_debug ("GdmXdmcpChooserSlave: Running chooser");

        display_id = NULL;
        display_name = NULL;
        auth_file = NULL;
        display_device = NULL;
        display_hostname = NULL;

        g_object_get (slave,
                      "display-id", &display_id,
                      "display-name", &display_name,
                      "display-hostname", &display_hostname,
                      "display-x11-authority-file", &auth_file,
                      NULL);

        g_debug ("GdmXdmcpChooserSlave: Creating chooser for %s %s", display_name, display_hostname);

        /* FIXME: send a signal back to the master */

        /* If XDMCP setup pinging */
        slave->priv->ping_interval = DEFAULT_PING_INTERVAL;
        res = gdm_settings_direct_get_int (GDM_KEY_PING_INTERVAL,
                                           &(slave->priv->ping_interval));

        if (slave->priv->ping_interval > 0) {
                alarm (slave->priv->ping_interval);
        }

        /* Run the init script. gdmslave suspends until script has terminated */
        gdm_slave_run_script (GDM_SLAVE (slave), GDMCONFDIR "/Init", GDM_USERNAME);

        slave->priv->chooser_server = gdm_chooser_server_new (display_id);
        g_signal_connect (slave->priv->chooser_server,
                          "hostname-selected",
                          G_CALLBACK (on_chooser_hostname_selected),
                          slave);
        g_signal_connect (slave->priv->chooser_server,
                          "disconnected",
                          G_CALLBACK (on_chooser_disconnected),
                          slave);
        g_signal_connect (slave->priv->chooser_server,
                          "connected",
                          G_CALLBACK (on_chooser_connected),
                          slave);
        gdm_chooser_server_start (slave->priv->chooser_server);

        address = gdm_chooser_server_get_address (slave->priv->chooser_server);

        g_debug ("GdmXdmcpChooserSlave: Creating chooser on %s %s %s", display_name, display_device, display_hostname);
        slave->priv->chooser = gdm_chooser_session_new (display_name,
                                                        display_device,
                                                        display_hostname);
        g_signal_connect (slave->priv->chooser,
                          "started",
                          G_CALLBACK (on_chooser_session_start),
                          slave);
        g_signal_connect (slave->priv->chooser,
                          "stopped",
                          G_CALLBACK (on_chooser_session_stop),
                          slave);
        g_signal_connect (slave->priv->chooser,
                          "exited",
                          G_CALLBACK (on_chooser_session_exited),
                          slave);
        g_signal_connect (slave->priv->chooser,
                          "died",
                          G_CALLBACK (on_chooser_session_died),
                          slave);
        g_object_set (slave->priv->chooser,
                      "x11-authority-file", auth_file,
                      NULL);
        gdm_welcome_session_set_server_address (GDM_WELCOME_SESSION (slave->priv->chooser), address);
        gdm_welcome_session_start (GDM_WELCOME_SESSION (slave->priv->chooser));

        g_free (display_id);
        g_free (display_name);
        g_free (display_device);
        g_free (display_hostname);
        g_free (auth_file);
}

static gboolean
idle_connect_to_display (GdmXdmcpChooserSlave *slave)
{
        gboolean res;

        slave->priv->connection_attempts++;

        res = gdm_slave_connect_to_x11_display (GDM_SLAVE (slave));
        if (res) {
                /* FIXME: handle wait-for-go */

                setup_server (slave);
                run_chooser (slave);
        } else {
                if (slave->priv->connection_attempts >= MAX_CONNECT_ATTEMPTS) {
                        g_warning ("Unable to connect to display after %d tries - bailing out", slave->priv->connection_attempts);
                        exit (1);
                }
                return TRUE;
        }

        return FALSE;
}

static gboolean
gdm_xdmcp_chooser_slave_run (GdmXdmcpChooserSlave *slave)
{
        char    *display_name;
        char    *auth_file;

        g_object_get (slave,
                      "display-name", &display_name,
                      "display-x11-authority-file", &auth_file,
                      NULL);

        g_timeout_add (500, (GSourceFunc)idle_connect_to_display, slave);

        g_free (display_name);
        g_free (auth_file);

        return TRUE;
}

static gboolean
gdm_xdmcp_chooser_slave_start (GdmSlave *slave)
{
        GDM_SLAVE_CLASS (gdm_xdmcp_chooser_slave_parent_class)->start (slave);

        gdm_xdmcp_chooser_slave_run (GDM_XDMCP_CHOOSER_SLAVE (slave));

        return TRUE;
}

static gboolean
gdm_xdmcp_chooser_slave_stop (GdmSlave *slave)
{
        g_debug ("GdmXdmcpChooserSlave: Stopping xdmcp_chooser_slave");

        GDM_SLAVE_CLASS (gdm_xdmcp_chooser_slave_parent_class)->stop (slave);

        if (GDM_XDMCP_CHOOSER_SLAVE (slave)->priv->chooser != NULL) {
                gdm_welcome_session_stop (GDM_WELCOME_SESSION (GDM_XDMCP_CHOOSER_SLAVE (slave)->priv->chooser));
                g_object_unref (GDM_XDMCP_CHOOSER_SLAVE (slave)->priv->chooser);
                GDM_XDMCP_CHOOSER_SLAVE (slave)->priv->chooser = NULL;
        }

        return TRUE;
}

static void
gdm_xdmcp_chooser_slave_set_property (GObject      *object,
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
gdm_xdmcp_chooser_slave_get_property (GObject    *object,
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
gdm_xdmcp_chooser_slave_constructor (GType                  type,
                                     guint                  n_construct_properties,
                                     GObjectConstructParam *construct_properties)
{
        GdmXdmcpChooserSlave      *xdmcp_chooser_slave;

        xdmcp_chooser_slave = GDM_XDMCP_CHOOSER_SLAVE (G_OBJECT_CLASS (gdm_xdmcp_chooser_slave_parent_class)->constructor (type,
                                                                                                                           n_construct_properties,
                                                                                                                           construct_properties));

        return G_OBJECT (xdmcp_chooser_slave);
}

static void
gdm_xdmcp_chooser_slave_class_init (GdmXdmcpChooserSlaveClass *klass)
{
        GObjectClass  *object_class = G_OBJECT_CLASS (klass);
        GdmSlaveClass *slave_class = GDM_SLAVE_CLASS (klass);

        object_class->get_property = gdm_xdmcp_chooser_slave_get_property;
        object_class->set_property = gdm_xdmcp_chooser_slave_set_property;
        object_class->constructor = gdm_xdmcp_chooser_slave_constructor;
        object_class->finalize = gdm_xdmcp_chooser_slave_finalize;

        slave_class->start = gdm_xdmcp_chooser_slave_start;
        slave_class->stop = gdm_xdmcp_chooser_slave_stop;

        signals [HOSTNAME_SELECTED] =
                g_signal_new ("hostname-selected",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (GdmXdmcpChooserSlaveClass, hostname_selected),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_STRING);

        g_type_class_add_private (klass, sizeof (GdmXdmcpChooserSlavePrivate));

        dbus_g_object_type_install_info (GDM_TYPE_XDMCP_CHOOSER_SLAVE, &dbus_glib_gdm_xdmcp_chooser_slave_object_info);
}

static void
gdm_xdmcp_chooser_slave_init (GdmXdmcpChooserSlave *slave)
{
        slave->priv = GDM_XDMCP_CHOOSER_SLAVE_GET_PRIVATE (slave);
}

static void
gdm_xdmcp_chooser_slave_finalize (GObject *object)
{
        GdmXdmcpChooserSlave *xdmcp_chooser_slave;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GDM_IS_XDMCP_CHOOSER_SLAVE (object));

        xdmcp_chooser_slave = GDM_XDMCP_CHOOSER_SLAVE (object);

        g_return_if_fail (xdmcp_chooser_slave->priv != NULL);

        gdm_xdmcp_chooser_slave_stop (GDM_SLAVE (xdmcp_chooser_slave));

        G_OBJECT_CLASS (gdm_xdmcp_chooser_slave_parent_class)->finalize (object);
}

GdmSlave *
gdm_xdmcp_chooser_slave_new (const char *id)
{
        GObject *object;

        object = g_object_new (GDM_TYPE_XDMCP_CHOOSER_SLAVE,
                               "display-id", id,
                               NULL);

        return GDM_SLAVE (object);
}
