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

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <pwd.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>

#include "mdm-common.h"
#include "mdm-display.h"
#include "mdm-transient-display.h"
#include "mdm-transient-display-glue.h"

#define MDM_TRANSIENT_DISPLAY_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_TRANSIENT_DISPLAY, MdmTransientDisplayPrivate))

struct MdmTransientDisplayPrivate
{
        gpointer dummy;
};

enum {
        PROP_0,
};

static void     mdm_transient_display_class_init   (MdmTransientDisplayClass *klass);
static void     mdm_transient_display_init         (MdmTransientDisplay      *display);
static void     mdm_transient_display_finalize     (GObject                  *object);

G_DEFINE_TYPE (MdmTransientDisplay, mdm_transient_display, MDM_TYPE_DISPLAY)

static gboolean
mdm_transient_display_create_authority (MdmDisplay *display)
{
        g_return_val_if_fail (MDM_IS_DISPLAY (display), FALSE);

        MDM_DISPLAY_CLASS (mdm_transient_display_parent_class)->create_authority (display);

        return TRUE;
}

static gboolean
mdm_transient_display_add_user_authorization (MdmDisplay *display,
                                              const char *username,
                                              char      **filename,
                                              GError    **error)
{
        return MDM_DISPLAY_CLASS (mdm_transient_display_parent_class)->add_user_authorization (display, username, filename, error);
}

static gboolean
mdm_transient_display_remove_user_authorization (MdmDisplay *display,
                                                 const char *username,
                                                 GError    **error)
{
        return MDM_DISPLAY_CLASS (mdm_transient_display_parent_class)->remove_user_authorization (display, username, error);
}

static gboolean
mdm_transient_display_manage (MdmDisplay *display)
{
        g_return_val_if_fail (MDM_IS_DISPLAY (display), FALSE);

        MDM_DISPLAY_CLASS (mdm_transient_display_parent_class)->manage (display);

        return TRUE;
}

static gboolean
mdm_transient_display_finish (MdmDisplay *display)
{
        g_return_val_if_fail (MDM_IS_DISPLAY (display), FALSE);

        MDM_DISPLAY_CLASS (mdm_transient_display_parent_class)->finish (display);

        /* we don't restart/remanage transient displays */
        mdm_display_unmanage (display);

        return TRUE;
}

static gboolean
mdm_transient_display_unmanage (MdmDisplay *display)
{
        g_return_val_if_fail (MDM_IS_DISPLAY (display), FALSE);

        MDM_DISPLAY_CLASS (mdm_transient_display_parent_class)->unmanage (display);

        return TRUE;
}

static void
mdm_transient_display_set_property (GObject      *object,
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
mdm_transient_display_get_property (GObject    *object,
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

static void
mdm_transient_display_get_timed_login_details (MdmDisplay *display,
                                               gboolean   *enabledp,
                                               char      **usernamep,
                                               int        *delayp)
{
        *enabledp = FALSE;
        *usernamep = g_strdup ("");
        *delayp = 0;
}

static void
mdm_transient_display_class_init (MdmTransientDisplayClass *klass)
{
        GObjectClass    *object_class = G_OBJECT_CLASS (klass);
        MdmDisplayClass *display_class = MDM_DISPLAY_CLASS (klass);

        object_class->get_property = mdm_transient_display_get_property;
        object_class->set_property = mdm_transient_display_set_property;
        object_class->finalize = mdm_transient_display_finalize;

        display_class->create_authority = mdm_transient_display_create_authority;
        display_class->add_user_authorization = mdm_transient_display_add_user_authorization;
        display_class->remove_user_authorization = mdm_transient_display_remove_user_authorization;
        display_class->manage = mdm_transient_display_manage;
        display_class->finish = mdm_transient_display_finish;
        display_class->unmanage = mdm_transient_display_unmanage;
        display_class->get_timed_login_details = mdm_transient_display_get_timed_login_details;

        g_type_class_add_private (klass, sizeof (MdmTransientDisplayPrivate));

        dbus_g_object_type_install_info (MDM_TYPE_TRANSIENT_DISPLAY, &dbus_glib_mdm_transient_display_object_info);
}

static void
mdm_transient_display_init (MdmTransientDisplay *display)
{

        display->priv = MDM_TRANSIENT_DISPLAY_GET_PRIVATE (display);
}

static void
mdm_transient_display_finalize (GObject *object)
{
        MdmTransientDisplay *display;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_TRANSIENT_DISPLAY (object));

        display = MDM_TRANSIENT_DISPLAY (object);

        g_return_if_fail (display->priv != NULL);

        G_OBJECT_CLASS (mdm_transient_display_parent_class)->finalize (object);
}

MdmDisplay *
mdm_transient_display_new (int display_number)
{
        GObject *object;
        char    *x11_display;

        x11_display = g_strdup_printf (":%d", display_number);
        object = g_object_new (MDM_TYPE_TRANSIENT_DISPLAY,
                               "x11-display-number", display_number,
                               "x11-display-name", x11_display,
                               NULL);
        g_free (x11_display);

        return MDM_DISPLAY (object);
}
