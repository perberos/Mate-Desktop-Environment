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

#include "gdm-common.h"
#include "gdm-display.h"
#include "gdm-transient-display.h"
#include "gdm-transient-display-glue.h"

#define GDM_TRANSIENT_DISPLAY_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GDM_TYPE_TRANSIENT_DISPLAY, GdmTransientDisplayPrivate))

struct GdmTransientDisplayPrivate
{
        gpointer dummy;
};

enum {
        PROP_0,
};

static void     gdm_transient_display_class_init   (GdmTransientDisplayClass *klass);
static void     gdm_transient_display_init         (GdmTransientDisplay      *display);
static void     gdm_transient_display_finalize     (GObject                  *object);

G_DEFINE_TYPE (GdmTransientDisplay, gdm_transient_display, GDM_TYPE_DISPLAY)

static gboolean
gdm_transient_display_create_authority (GdmDisplay *display)
{
        g_return_val_if_fail (GDM_IS_DISPLAY (display), FALSE);

        GDM_DISPLAY_CLASS (gdm_transient_display_parent_class)->create_authority (display);

        return TRUE;
}

static gboolean
gdm_transient_display_add_user_authorization (GdmDisplay *display,
                                              const char *username,
                                              char      **filename,
                                              GError    **error)
{
        return GDM_DISPLAY_CLASS (gdm_transient_display_parent_class)->add_user_authorization (display, username, filename, error);
}

static gboolean
gdm_transient_display_remove_user_authorization (GdmDisplay *display,
                                                 const char *username,
                                                 GError    **error)
{
        return GDM_DISPLAY_CLASS (gdm_transient_display_parent_class)->remove_user_authorization (display, username, error);
}

static gboolean
gdm_transient_display_manage (GdmDisplay *display)
{
        g_return_val_if_fail (GDM_IS_DISPLAY (display), FALSE);

        GDM_DISPLAY_CLASS (gdm_transient_display_parent_class)->manage (display);

        return TRUE;
}

static gboolean
gdm_transient_display_finish (GdmDisplay *display)
{
        g_return_val_if_fail (GDM_IS_DISPLAY (display), FALSE);

        GDM_DISPLAY_CLASS (gdm_transient_display_parent_class)->finish (display);

        /* we don't restart/remanage transient displays */
        gdm_display_unmanage (display);

        return TRUE;
}

static gboolean
gdm_transient_display_unmanage (GdmDisplay *display)
{
        g_return_val_if_fail (GDM_IS_DISPLAY (display), FALSE);

        GDM_DISPLAY_CLASS (gdm_transient_display_parent_class)->unmanage (display);

        return TRUE;
}

static void
gdm_transient_display_set_property (GObject      *object,
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
gdm_transient_display_get_property (GObject    *object,
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
gdm_transient_display_get_timed_login_details (GdmDisplay *display,
                                               gboolean   *enabledp,
                                               char      **usernamep,
                                               int        *delayp)
{
        *enabledp = FALSE;
        *usernamep = g_strdup ("");
        *delayp = 0;
}

static void
gdm_transient_display_class_init (GdmTransientDisplayClass *klass)
{
        GObjectClass    *object_class = G_OBJECT_CLASS (klass);
        GdmDisplayClass *display_class = GDM_DISPLAY_CLASS (klass);

        object_class->get_property = gdm_transient_display_get_property;
        object_class->set_property = gdm_transient_display_set_property;
        object_class->finalize = gdm_transient_display_finalize;

        display_class->create_authority = gdm_transient_display_create_authority;
        display_class->add_user_authorization = gdm_transient_display_add_user_authorization;
        display_class->remove_user_authorization = gdm_transient_display_remove_user_authorization;
        display_class->manage = gdm_transient_display_manage;
        display_class->finish = gdm_transient_display_finish;
        display_class->unmanage = gdm_transient_display_unmanage;
        display_class->get_timed_login_details = gdm_transient_display_get_timed_login_details;

        g_type_class_add_private (klass, sizeof (GdmTransientDisplayPrivate));

        dbus_g_object_type_install_info (GDM_TYPE_TRANSIENT_DISPLAY, &dbus_glib_gdm_transient_display_object_info);
}

static void
gdm_transient_display_init (GdmTransientDisplay *display)
{

        display->priv = GDM_TRANSIENT_DISPLAY_GET_PRIVATE (display);
}

static void
gdm_transient_display_finalize (GObject *object)
{
        GdmTransientDisplay *display;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GDM_IS_TRANSIENT_DISPLAY (object));

        display = GDM_TRANSIENT_DISPLAY (object);

        g_return_if_fail (display->priv != NULL);

        G_OBJECT_CLASS (gdm_transient_display_parent_class)->finalize (object);
}

GdmDisplay *
gdm_transient_display_new (int display_number)
{
        GObject *object;
        char    *x11_display;

        x11_display = g_strdup_printf (":%d", display_number);
        object = g_object_new (GDM_TYPE_TRANSIENT_DISPLAY,
                               "x11-display-number", display_number,
                               "x11-display-name", x11_display,
                               NULL);
        g_free (x11_display);

        return GDM_DISPLAY (object);
}
