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

#include "mdm-xdmcp-display.h"
#include "mdm-xdmcp-greeter-display.h"
#include "mdm-xdmcp-greeter-display-glue.h"

#include "mdm-common.h"
#include "mdm-address.h"

#define MDM_XDMCP_GREETER_DISPLAY_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_XDMCP_GREETER_DISPLAY, MdmXdmcpGreeterDisplayPrivate))

struct MdmXdmcpGreeterDisplayPrivate
{
        MdmAddress             *remote_address;
        gint32                  session_number;
};

enum {
        PROP_0,
        PROP_REMOTE_ADDRESS,
        PROP_SESSION_NUMBER,
};

static void     mdm_xdmcp_greeter_display_class_init    (MdmXdmcpGreeterDisplayClass *klass);
static void     mdm_xdmcp_greeter_display_init          (MdmXdmcpGreeterDisplay      *xdmcp_greeter_display);
static void     mdm_xdmcp_greeter_display_finalize      (GObject                     *object);
static gboolean mdm_xdmcp_greeter_display_finish (MdmDisplay *display);

G_DEFINE_TYPE (MdmXdmcpGreeterDisplay, mdm_xdmcp_greeter_display, MDM_TYPE_XDMCP_DISPLAY)

static void
mdm_xdmcp_greeter_display_class_init (MdmXdmcpGreeterDisplayClass *klass)
{
        GObjectClass    *object_class = G_OBJECT_CLASS (klass);
        MdmDisplayClass *display_class = MDM_DISPLAY_CLASS (klass);

        object_class->finalize = mdm_xdmcp_greeter_display_finalize;
        display_class->finish = mdm_xdmcp_greeter_display_finish;

        g_type_class_add_private (klass, sizeof (MdmXdmcpGreeterDisplayPrivate));

        dbus_g_object_type_install_info (MDM_TYPE_XDMCP_GREETER_DISPLAY, &dbus_glib_mdm_xdmcp_greeter_display_object_info);
}

static void
mdm_xdmcp_greeter_display_init (MdmXdmcpGreeterDisplay *xdmcp_greeter_display)
{

        xdmcp_greeter_display->priv = MDM_XDMCP_GREETER_DISPLAY_GET_PRIVATE (xdmcp_greeter_display);
}

static void
mdm_xdmcp_greeter_display_finalize (GObject *object)
{
        MdmXdmcpGreeterDisplay *xdmcp_greeter_display;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_XDMCP_GREETER_DISPLAY (object));

        xdmcp_greeter_display = MDM_XDMCP_GREETER_DISPLAY (object);

        g_return_if_fail (xdmcp_greeter_display->priv != NULL);

        G_OBJECT_CLASS (mdm_xdmcp_greeter_display_parent_class)->finalize (object);
}

static gboolean
mdm_xdmcp_greeter_display_finish (MdmDisplay *display)
{
        g_return_val_if_fail (MDM_IS_DISPLAY (display), FALSE);

        MDM_DISPLAY_CLASS (mdm_xdmcp_greeter_display_parent_class)->finish (display);

        mdm_display_unmanage (display);

        return TRUE;
}

MdmDisplay *
mdm_xdmcp_greeter_display_new (const char              *hostname,
                               int                      number,
                               MdmAddress              *address,
                               gint32                   session_number)
{
        GObject *object;
        char    *x11_display;

        x11_display = g_strdup_printf ("%s:%d", hostname, number);
        object = g_object_new (MDM_TYPE_XDMCP_GREETER_DISPLAY,
                               "remote-hostname", hostname,
                               "x11-display-number", number,
                               "x11-display-name", x11_display,
                               "is-local", FALSE,
                               "remote-address", address,
                               "session-number", session_number,
                               NULL);
        g_free (x11_display);

        return MDM_DISPLAY (object);
}
