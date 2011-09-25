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

#include "mdm-common.h"

#include "mdm-xdmcp-chooser-slave.h"
#include "mdm-xdmcp-chooser-slave-glue.h"

#include "mdm-server.h"
#include "mdm-chooser-server.h"
#include "mdm-chooser-session.h"
#include "mdm-settings-direct.h"
#include "mdm-settings-keys.h"

#define MDM_XDMCP_CHOOSER_SLAVE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_XDMCP_CHOOSER_SLAVE, MdmXdmcpChooserSlavePrivate))

#define MDM_DBUS_NAME              "org.mate.DisplayManager"
#define MDM_DBUS_DISPLAY_INTERFACE "org.mate.DisplayManager.Display"

#define MAX_CONNECT_ATTEMPTS  10
#define DEFAULT_PING_INTERVAL 15

struct MdmXdmcpChooserSlavePrivate
{
        char              *id;
        GPid               pid;

        int                ping_interval;

        guint              connection_attempts;

        MdmChooserServer  *chooser_server;
        MdmChooserSession *chooser;
};

enum {
        PROP_0,
};

enum {
        HOSTNAME_SELECTED,
        LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0, };

static void     mdm_xdmcp_chooser_slave_class_init     (MdmXdmcpChooserSlaveClass *klass);
static void     mdm_xdmcp_chooser_slave_init           (MdmXdmcpChooserSlave      *xdmcp_chooser_slave);
static void     mdm_xdmcp_chooser_slave_finalize       (GObject                   *object);

G_DEFINE_TYPE (MdmXdmcpChooserSlave, mdm_xdmcp_chooser_slave, MDM_TYPE_SLAVE)


static void
on_chooser_session_start (MdmChooserSession    *chooser,
                          MdmXdmcpChooserSlave *slave)
{
        g_debug ("MdmXdmcpChooserSlave: Chooser started");
}

static void
on_chooser_session_stop (MdmChooserSession    *chooser,
                         MdmXdmcpChooserSlave *slave)
{
        g_debug ("MdmXdmcpChooserSlave: Chooser stopped");
        mdm_slave_stopped (MDM_SLAVE (slave));
}

static void
on_chooser_session_exited (MdmChooserSession    *chooser,
                           int                   code,
                           MdmXdmcpChooserSlave *slave)
{
        g_debug ("MdmXdmcpChooserSlave: Chooser exited: %d", code);
        mdm_slave_stopped (MDM_SLAVE (slave));
}

static void
on_chooser_session_died (MdmChooserSession    *chooser,
                         int                   signal,
                         MdmXdmcpChooserSlave *slave)
{
        g_debug ("MdmXdmcpChooserSlave: Chooser died: %d", signal);
        mdm_slave_stopped (MDM_SLAVE (slave));
}

static void
on_chooser_hostname_selected (MdmChooserServer     *chooser_server,
                              const char           *name,
                              MdmXdmcpChooserSlave *slave)
{
        g_debug ("MdmXdmcpChooserSlave: emitting hostname selected: %s", name);
        g_signal_emit (slave, signals [HOSTNAME_SELECTED], 0, name);
}

static void
on_chooser_disconnected (MdmChooserServer     *chooser_server,
                         MdmXdmcpChooserSlave *slave)
{
        g_debug ("MdmXdmcpChooserSlave: Chooser disconnected");

        /* stop pinging */
        alarm (0);

        mdm_slave_stopped (MDM_SLAVE (slave));
}

static void
on_chooser_connected (MdmChooserServer     *chooser_server,
                      MdmXdmcpChooserSlave *slave)
{
        g_debug ("MdmXdmcpChooserSlave: Chooser connected");
}

static void
setup_server (MdmXdmcpChooserSlave *slave)
{
        /* Set the busy cursor */
        mdm_slave_set_busy_cursor (MDM_SLAVE (slave));
}

static void
run_chooser (MdmXdmcpChooserSlave *slave)
{
        char          *display_id;
        char          *display_name;
        char          *display_device;
        char          *display_hostname;
        char          *auth_file;
        char          *address;
        gboolean       res;

        g_debug ("MdmXdmcpChooserSlave: Running chooser");

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

        g_debug ("MdmXdmcpChooserSlave: Creating chooser for %s %s", display_name, display_hostname);

        /* FIXME: send a signal back to the master */

        /* If XDMCP setup pinging */
        slave->priv->ping_interval = DEFAULT_PING_INTERVAL;
        res = mdm_settings_direct_get_int (MDM_KEY_PING_INTERVAL,
                                           &(slave->priv->ping_interval));

        if (slave->priv->ping_interval > 0) {
                alarm (slave->priv->ping_interval);
        }

        /* Run the init script. mdmslave suspends until script has terminated */
        mdm_slave_run_script (MDM_SLAVE (slave), MDMCONFDIR "/Init", MDM_USERNAME);

        slave->priv->chooser_server = mdm_chooser_server_new (display_id);
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
        mdm_chooser_server_start (slave->priv->chooser_server);

        address = mdm_chooser_server_get_address (slave->priv->chooser_server);

        g_debug ("MdmXdmcpChooserSlave: Creating chooser on %s %s %s", display_name, display_device, display_hostname);
        slave->priv->chooser = mdm_chooser_session_new (display_name,
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
        mdm_welcome_session_set_server_address (MDM_WELCOME_SESSION (slave->priv->chooser), address);
        mdm_welcome_session_start (MDM_WELCOME_SESSION (slave->priv->chooser));

        g_free (display_id);
        g_free (display_name);
        g_free (display_device);
        g_free (display_hostname);
        g_free (auth_file);
}

static gboolean
idle_connect_to_display (MdmXdmcpChooserSlave *slave)
{
        gboolean res;

        slave->priv->connection_attempts++;

        res = mdm_slave_connect_to_x11_display (MDM_SLAVE (slave));
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
mdm_xdmcp_chooser_slave_run (MdmXdmcpChooserSlave *slave)
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
mdm_xdmcp_chooser_slave_start (MdmSlave *slave)
{
        MDM_SLAVE_CLASS (mdm_xdmcp_chooser_slave_parent_class)->start (slave);

        mdm_xdmcp_chooser_slave_run (MDM_XDMCP_CHOOSER_SLAVE (slave));

        return TRUE;
}

static gboolean
mdm_xdmcp_chooser_slave_stop (MdmSlave *slave)
{
        g_debug ("MdmXdmcpChooserSlave: Stopping xdmcp_chooser_slave");

        MDM_SLAVE_CLASS (mdm_xdmcp_chooser_slave_parent_class)->stop (slave);

        if (MDM_XDMCP_CHOOSER_SLAVE (slave)->priv->chooser != NULL) {
                mdm_welcome_session_stop (MDM_WELCOME_SESSION (MDM_XDMCP_CHOOSER_SLAVE (slave)->priv->chooser));
                g_object_unref (MDM_XDMCP_CHOOSER_SLAVE (slave)->priv->chooser);
                MDM_XDMCP_CHOOSER_SLAVE (slave)->priv->chooser = NULL;
        }

        return TRUE;
}

static void
mdm_xdmcp_chooser_slave_set_property (GObject      *object,
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
mdm_xdmcp_chooser_slave_get_property (GObject    *object,
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
mdm_xdmcp_chooser_slave_constructor (GType                  type,
                                     guint                  n_construct_properties,
                                     GObjectConstructParam *construct_properties)
{
        MdmXdmcpChooserSlave      *xdmcp_chooser_slave;

        xdmcp_chooser_slave = MDM_XDMCP_CHOOSER_SLAVE (G_OBJECT_CLASS (mdm_xdmcp_chooser_slave_parent_class)->constructor (type,
                                                                                                                           n_construct_properties,
                                                                                                                           construct_properties));

        return G_OBJECT (xdmcp_chooser_slave);
}

static void
mdm_xdmcp_chooser_slave_class_init (MdmXdmcpChooserSlaveClass *klass)
{
        GObjectClass  *object_class = G_OBJECT_CLASS (klass);
        MdmSlaveClass *slave_class = MDM_SLAVE_CLASS (klass);

        object_class->get_property = mdm_xdmcp_chooser_slave_get_property;
        object_class->set_property = mdm_xdmcp_chooser_slave_set_property;
        object_class->constructor = mdm_xdmcp_chooser_slave_constructor;
        object_class->finalize = mdm_xdmcp_chooser_slave_finalize;

        slave_class->start = mdm_xdmcp_chooser_slave_start;
        slave_class->stop = mdm_xdmcp_chooser_slave_stop;

        signals [HOSTNAME_SELECTED] =
                g_signal_new ("hostname-selected",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (MdmXdmcpChooserSlaveClass, hostname_selected),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1,
                              G_TYPE_STRING);

        g_type_class_add_private (klass, sizeof (MdmXdmcpChooserSlavePrivate));

        dbus_g_object_type_install_info (MDM_TYPE_XDMCP_CHOOSER_SLAVE, &dbus_glib_mdm_xdmcp_chooser_slave_object_info);
}

static void
mdm_xdmcp_chooser_slave_init (MdmXdmcpChooserSlave *slave)
{
        slave->priv = MDM_XDMCP_CHOOSER_SLAVE_GET_PRIVATE (slave);
}

static void
mdm_xdmcp_chooser_slave_finalize (GObject *object)
{
        MdmXdmcpChooserSlave *xdmcp_chooser_slave;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_XDMCP_CHOOSER_SLAVE (object));

        xdmcp_chooser_slave = MDM_XDMCP_CHOOSER_SLAVE (object);

        g_return_if_fail (xdmcp_chooser_slave->priv != NULL);

        mdm_xdmcp_chooser_slave_stop (MDM_SLAVE (xdmcp_chooser_slave));

        G_OBJECT_CLASS (mdm_xdmcp_chooser_slave_parent_class)->finalize (object);
}

MdmSlave *
mdm_xdmcp_chooser_slave_new (const char *id)
{
        GObject *object;

        object = g_object_new (MDM_TYPE_XDMCP_CHOOSER_SLAVE,
                               "display-id", id,
                               NULL);

        return MDM_SLAVE (object);
}
