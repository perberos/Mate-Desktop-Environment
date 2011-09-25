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
#include <ctype.h>
#include <pwd.h>
#include <grp.h>
#include <signal.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>

#include "mdm-welcome-session.h"
#include "mdm-greeter-session.h"

#define MDM_GREETER_SERVER_DBUS_PATH      "/org/mate/DisplayManager/GreeterServer"
#define MDM_GREETER_SERVER_DBUS_INTERFACE "org.mate.DisplayManager.GreeterServer"

#define MDM_GREETER_SESSION_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_GREETER_SESSION, MdmGreeterSessionPrivate))

struct MdmGreeterSessionPrivate
{
        gpointer dummy;
};

enum {
        PROP_0,
};

static void     mdm_greeter_session_class_init    (MdmGreeterSessionClass *klass);
static void     mdm_greeter_session_init  (MdmGreeterSession      *greeter_session);
static void     mdm_greeter_session_finalize      (GObject         *object);

G_DEFINE_TYPE (MdmGreeterSession, mdm_greeter_session, MDM_TYPE_WELCOME_SESSION)

static void
mdm_greeter_session_set_property (GObject      *object,
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
mdm_greeter_session_get_property (GObject    *object,
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
mdm_greeter_session_constructor (GType                  type,
                                 guint                  n_construct_properties,
                                 GObjectConstructParam *construct_properties)
{
        MdmGreeterSession      *greeter_session;
        MdmGreeterSessionClass *klass;

        klass = MDM_GREETER_SESSION_CLASS (g_type_class_peek (MDM_TYPE_GREETER_SESSION));

        greeter_session = MDM_GREETER_SESSION (G_OBJECT_CLASS (mdm_greeter_session_parent_class)->constructor (type,
                                                                                                               n_construct_properties,
                                                                                                               construct_properties));

        return G_OBJECT (greeter_session);
}

static void
mdm_greeter_session_class_init (MdmGreeterSessionClass *klass)
{
        GObjectClass    *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = mdm_greeter_session_get_property;
        object_class->set_property = mdm_greeter_session_set_property;
        object_class->constructor = mdm_greeter_session_constructor;
        object_class->finalize = mdm_greeter_session_finalize;

        g_type_class_add_private (klass, sizeof (MdmGreeterSessionPrivate));
}

static void
mdm_greeter_session_init (MdmGreeterSession *greeter_session)
{

        greeter_session->priv = MDM_GREETER_SESSION_GET_PRIVATE (greeter_session);
}

static void
mdm_greeter_session_finalize (GObject *object)
{
        MdmGreeterSession *greeter_session;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_GREETER_SESSION (object));

        greeter_session = MDM_GREETER_SESSION (object);

        g_return_if_fail (greeter_session->priv != NULL);

        G_OBJECT_CLASS (mdm_greeter_session_parent_class)->finalize (object);
}

MdmGreeterSession *
mdm_greeter_session_new (const char *display_name,
                         const char *seat_id,
                         const char *display_device,
                         const char *display_hostname,
                         gboolean    display_is_local)
{
        GObject *object;

        object = g_object_new (MDM_TYPE_GREETER_SESSION,
                               "command", BINDIR "/mate-session --autostart="DATADIR"/mdm/autostart/LoginWindow/",
                               "server-dbus-path", MDM_GREETER_SERVER_DBUS_PATH,
                               "server-dbus-interface", MDM_GREETER_SERVER_DBUS_INTERFACE,
                               "server-env-var-name", "MDM_GREETER_DBUS_ADDRESS",
                               "register-ck-session", TRUE,
                               "x11-display-name", display_name,
                               "x11-display-seat-id", seat_id,
                               "x11-display-device", display_device,
                               "x11-display-hostname", display_hostname,
                               "x11-display-is-local", display_is_local,
                               "runtime-dir", MDM_SCREENSHOT_DIR,
                               NULL);

        return MDM_GREETER_SESSION (object);
}
