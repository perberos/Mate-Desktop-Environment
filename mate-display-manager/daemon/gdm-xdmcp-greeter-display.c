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
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>

#include "gdm-xdmcp-display.h"
#include "gdm-xdmcp-greeter-display.h"
#include "gdm-xdmcp-greeter-display-glue.h"

#include "gdm-common.h"
#include "gdm-address.h"

#define GDM_XDMCP_GREETER_DISPLAY_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GDM_TYPE_XDMCP_GREETER_DISPLAY, GdmXdmcpGreeterDisplayPrivate))

struct GdmXdmcpGreeterDisplayPrivate
{
        GdmAddress             *remote_address;
        gint32                  session_number;
};

enum {
        PROP_0,
        PROP_REMOTE_ADDRESS,
        PROP_SESSION_NUMBER,
};

static void     gdm_xdmcp_greeter_display_class_init    (GdmXdmcpGreeterDisplayClass *klass);
static void     gdm_xdmcp_greeter_display_init          (GdmXdmcpGreeterDisplay      *xdmcp_greeter_display);
static void     gdm_xdmcp_greeter_display_finalize      (GObject                     *object);
static gboolean gdm_xdmcp_greeter_display_finish (GdmDisplay *display);

G_DEFINE_TYPE (GdmXdmcpGreeterDisplay, gdm_xdmcp_greeter_display, GDM_TYPE_XDMCP_DISPLAY)

static void
gdm_xdmcp_greeter_display_class_init (GdmXdmcpGreeterDisplayClass *klass)
{
        GObjectClass    *object_class = G_OBJECT_CLASS (klass);
        GdmDisplayClass *display_class = GDM_DISPLAY_CLASS (klass);

        object_class->finalize = gdm_xdmcp_greeter_display_finalize;
        display_class->finish = gdm_xdmcp_greeter_display_finish;

        g_type_class_add_private (klass, sizeof (GdmXdmcpGreeterDisplayPrivate));

        dbus_g_object_type_install_info (GDM_TYPE_XDMCP_GREETER_DISPLAY, &dbus_glib_gdm_xdmcp_greeter_display_object_info);
}

static void
gdm_xdmcp_greeter_display_init (GdmXdmcpGreeterDisplay *xdmcp_greeter_display)
{

        xdmcp_greeter_display->priv = GDM_XDMCP_GREETER_DISPLAY_GET_PRIVATE (xdmcp_greeter_display);
}

static void
gdm_xdmcp_greeter_display_finalize (GObject *object)
{
        GdmXdmcpGreeterDisplay *xdmcp_greeter_display;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GDM_IS_XDMCP_GREETER_DISPLAY (object));

        xdmcp_greeter_display = GDM_XDMCP_GREETER_DISPLAY (object);

        g_return_if_fail (xdmcp_greeter_display->priv != NULL);

        G_OBJECT_CLASS (gdm_xdmcp_greeter_display_parent_class)->finalize (object);
}

static gboolean
gdm_xdmcp_greeter_display_finish (GdmDisplay *display)
{
        g_return_val_if_fail (GDM_IS_DISPLAY (display), FALSE);

        GDM_DISPLAY_CLASS (gdm_xdmcp_greeter_display_parent_class)->finish (display);

        gdm_display_unmanage (display);

        return TRUE;
}

GdmDisplay *
gdm_xdmcp_greeter_display_new (const char              *hostname,
                               int                      number,
                               GdmAddress              *address,
                               gint32                   session_number)
{
        GObject *object;
        char    *x11_display;

        x11_display = g_strdup_printf ("%s:%d", hostname, number);
        object = g_object_new (GDM_TYPE_XDMCP_GREETER_DISPLAY,
                               "remote-hostname", hostname,
                               "x11-display-number", number,
                               "x11-display-name", x11_display,
                               "is-local", FALSE,
                               "remote-address", address,
                               "session-number", session_number,
                               NULL);
        g_free (x11_display);

        return GDM_DISPLAY (object);
}
