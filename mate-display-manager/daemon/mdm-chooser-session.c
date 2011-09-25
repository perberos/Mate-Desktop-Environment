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
#include <ctype.h>
#include <pwd.h>
#include <grp.h>
#include <signal.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>

#include "mdm-common.h"

#include "mdm-welcome-session.h"
#include "mdm-chooser-session.h"

#define MDM_CHOOSER_SERVER_DBUS_PATH      "/org/mate/DisplayManager/ChooserServer"
#define MDM_CHOOSER_SERVER_DBUS_INTERFACE "org.mate.DisplayManager.ChooserServer"

#define MDM_CHOOSER_SESSION_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_CHOOSER_SESSION, MdmChooserSessionPrivate))

struct MdmChooserSessionPrivate
{
        gpointer dummy;
};

enum {
        PROP_0,
};

static void     mdm_chooser_session_class_init    (MdmChooserSessionClass *klass);
static void     mdm_chooser_session_init          (MdmChooserSession      *chooser_session);
static void     mdm_chooser_session_finalize      (GObject                *object);

G_DEFINE_TYPE (MdmChooserSession, mdm_chooser_session, MDM_TYPE_WELCOME_SESSION)

static void
mdm_chooser_session_set_property (GObject      *object,
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
mdm_chooser_session_get_property (GObject    *object,
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
mdm_chooser_session_constructor (GType                  type,
                                 guint                  n_construct_properties,
                                 GObjectConstructParam *construct_properties)
{
        MdmChooserSession      *chooser_session;

        chooser_session = MDM_CHOOSER_SESSION (G_OBJECT_CLASS (mdm_chooser_session_parent_class)->constructor (type,
                                                                                                               n_construct_properties,
                                                                                                               construct_properties));

        return G_OBJECT (chooser_session);
}

static void
mdm_chooser_session_class_init (MdmChooserSessionClass *klass)
{
        GObjectClass    *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = mdm_chooser_session_get_property;
        object_class->set_property = mdm_chooser_session_set_property;
        object_class->constructor = mdm_chooser_session_constructor;
        object_class->finalize = mdm_chooser_session_finalize;

        g_type_class_add_private (klass, sizeof (MdmChooserSessionPrivate));
}

static void
mdm_chooser_session_init (MdmChooserSession *chooser_session)
{

        chooser_session->priv = MDM_CHOOSER_SESSION_GET_PRIVATE (chooser_session);
}

static void
mdm_chooser_session_finalize (GObject *object)
{
        MdmChooserSession *chooser_session;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_CHOOSER_SESSION (object));

        chooser_session = MDM_CHOOSER_SESSION (object);

        g_return_if_fail (chooser_session->priv != NULL);

        G_OBJECT_CLASS (mdm_chooser_session_parent_class)->finalize (object);
}

MdmChooserSession *
mdm_chooser_session_new (const char *display_name,
                         const char *display_device,
                         const char *display_hostname)
{
        GObject *object;

        object = g_object_new (MDM_TYPE_CHOOSER_SESSION,
                               "command", LIBEXECDIR "/mdm-simple-chooser",
                               "server-dbus-path", MDM_CHOOSER_SERVER_DBUS_PATH,
                               "server-dbus-interface", MDM_CHOOSER_SERVER_DBUS_INTERFACE,
                               "server-env-var-name", "MDM_CHOOSER_DBUS_ADDRESS",
                               "register-ck-session", FALSE,
                               "x11-display-name", display_name,
                               "x11-display-device", display_device,
                               "x11-display-hostname", display_hostname,
                               NULL);

        return MDM_CHOOSER_SESSION (object);
}
