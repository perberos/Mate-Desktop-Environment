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

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>

#include "mdm-display.h"
#include "mdm-product-display.h"
#include "mdm-product-display-glue.h"

#define MDM_PRODUCT_DISPLAY_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_PRODUCT_DISPLAY, MdmProductDisplayPrivate))

#define DEFAULT_SLAVE_COMMAND LIBEXECDIR"/mdm-product-slave"

struct MdmProductDisplayPrivate
{
        char *relay_address;
};

enum {
        PROP_0,
        PROP_RELAY_ADDRESS,
};

static void     mdm_product_display_class_init  (MdmProductDisplayClass *klass);
static void     mdm_product_display_init        (MdmProductDisplay      *product_display);
static void     mdm_product_display_finalize    (GObject                *object);

G_DEFINE_TYPE (MdmProductDisplay, mdm_product_display, MDM_TYPE_DISPLAY)

static gboolean
mdm_product_display_create_authority (MdmDisplay *display)
{
        g_return_val_if_fail (MDM_IS_DISPLAY (display), FALSE);

        MDM_DISPLAY_CLASS (mdm_product_display_parent_class)->create_authority (display);

        return TRUE;
}

static gboolean
mdm_product_display_add_user_authorization (MdmDisplay *display,
                                            const char *username,
                                            char      **filename,
                                            GError    **error)
{
        return MDM_DISPLAY_CLASS (mdm_product_display_parent_class)->add_user_authorization (display, username, filename, error);
}

static gboolean
mdm_product_display_remove_user_authorization (MdmDisplay *display,
                                               const char *username,
                                               GError    **error)
{
        return MDM_DISPLAY_CLASS (mdm_product_display_parent_class)->remove_user_authorization (display, username, error);
}

static gboolean
mdm_product_display_manage (MdmDisplay *display)
{
        g_return_val_if_fail (MDM_IS_DISPLAY (display), FALSE);

        MDM_DISPLAY_CLASS (mdm_product_display_parent_class)->manage (display);

        return TRUE;
}

static gboolean
mdm_product_display_finish (MdmDisplay *display)
{
        g_return_val_if_fail (MDM_IS_DISPLAY (display), FALSE);

        MDM_DISPLAY_CLASS (mdm_product_display_parent_class)->finish (display);

        /* when a product display is done it is done */
        mdm_display_unmanage (display);

        return TRUE;
}

static gboolean
mdm_product_display_unmanage (MdmDisplay *display)
{
        g_return_val_if_fail (MDM_IS_DISPLAY (display), FALSE);

        MDM_DISPLAY_CLASS (mdm_product_display_parent_class)->unmanage (display);

        return TRUE;
}

gboolean
mdm_product_display_get_relay_address (MdmProductDisplay *display,
                                       char             **address,
                                       GError           **error)
{
        g_return_val_if_fail (MDM_IS_DISPLAY (display), FALSE);

        if (address != NULL) {
                *address = g_strdup (display->priv->relay_address);
                g_debug ("MdmProductDisplay: Returning address: %s", display->priv->relay_address);
        }

        return TRUE;
}

static void
mdm_product_display_set_relay_address (MdmProductDisplay *display,
                                       const char        *address)
{
        g_free (display->priv->relay_address);
        display->priv->relay_address = g_strdup (address);
}


static void
mdm_product_display_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
        MdmProductDisplay *self;

        self = MDM_PRODUCT_DISPLAY (object);

        switch (prop_id) {
        case PROP_RELAY_ADDRESS:
                mdm_product_display_set_relay_address (self, g_value_get_string (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
mdm_product_display_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
        MdmProductDisplay *self;

        self = MDM_PRODUCT_DISPLAY (object);

        switch (prop_id) {
        case PROP_RELAY_ADDRESS:
                g_value_set_string (value, self->priv->relay_address);
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static GObject *
mdm_product_display_constructor (GType                  type,
                                 guint                  n_construct_properties,
                                 GObjectConstructParam *construct_properties)
{
        MdmProductDisplay      *display;

        display = MDM_PRODUCT_DISPLAY (G_OBJECT_CLASS (mdm_product_display_parent_class)->constructor (type,
                                                                                                       n_construct_properties,
                                                                                                       construct_properties));
        return G_OBJECT (display);
}

static void
mdm_product_display_class_init (MdmProductDisplayClass *klass)
{
        GObjectClass    *object_class = G_OBJECT_CLASS (klass);
        MdmDisplayClass *display_class = MDM_DISPLAY_CLASS (klass);

        object_class->get_property = mdm_product_display_get_property;
        object_class->set_property = mdm_product_display_set_property;
        object_class->constructor = mdm_product_display_constructor;
        object_class->finalize = mdm_product_display_finalize;

        display_class->create_authority = mdm_product_display_create_authority;
        display_class->add_user_authorization = mdm_product_display_add_user_authorization;
        display_class->remove_user_authorization = mdm_product_display_remove_user_authorization;
        display_class->manage = mdm_product_display_manage;
        display_class->finish = mdm_product_display_finish;
        display_class->unmanage = mdm_product_display_unmanage;

        g_type_class_add_private (klass, sizeof (MdmProductDisplayPrivate));

        g_object_class_install_property (object_class,
                                         PROP_RELAY_ADDRESS,
                                         g_param_spec_string ("relay-address",
                                                              "relay address",
                                                              "relay address",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

        dbus_g_object_type_install_info (MDM_TYPE_PRODUCT_DISPLAY, &dbus_glib_mdm_product_display_object_info);
}

static void
mdm_product_display_init (MdmProductDisplay *product_display)
{

        product_display->priv = MDM_PRODUCT_DISPLAY_GET_PRIVATE (product_display);
}

static void
mdm_product_display_finalize (GObject *object)
{
        MdmProductDisplay *product_display;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_PRODUCT_DISPLAY (object));

        product_display = MDM_PRODUCT_DISPLAY (object);

        g_return_if_fail (product_display->priv != NULL);

        g_free (product_display->priv->relay_address);

        G_OBJECT_CLASS (mdm_product_display_parent_class)->finalize (object);
}

MdmDisplay *
mdm_product_display_new (int         display_number,
                         const char *relay_address)
{
        GObject *object;
        char    *x11_display;

        x11_display = g_strdup_printf (":%d", display_number);
        object = g_object_new (MDM_TYPE_PRODUCT_DISPLAY,
                               "slave-command", DEFAULT_SLAVE_COMMAND,
                               "x11-display-number", display_number,
                               "x11-display-name", x11_display,
                               "relay-address", relay_address,
                               NULL);
        g_free (x11_display);

        return MDM_DISPLAY (object);
}
