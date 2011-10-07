/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Red Hat, Inc.
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

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>

#include "mdm-address.h"
#include "mdm-chooser-host.h"

#define MDM_CHOOSER_HOST_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_CHOOSER_HOST, MdmChooserHostPrivate))

struct MdmChooserHostPrivate
{
        MdmAddress        *address;
        char              *description;
        MdmChooserHostKind kind;
        gboolean           willing;
};

enum {
        PROP_0,
        PROP_ADDRESS,
        PROP_DESCRIPTION,
        PROP_KIND,
        PROP_WILLING,
};

static void     mdm_chooser_host_class_init  (MdmChooserHostClass *klass);
static void     mdm_chooser_host_init        (MdmChooserHost      *chooser_host);
static void     mdm_chooser_host_finalize    (GObject             *object);

G_DEFINE_TYPE (MdmChooserHost, mdm_chooser_host, G_TYPE_OBJECT)

MdmAddress *
mdm_chooser_host_get_address (MdmChooserHost *host)
{
        g_return_val_if_fail (MDM_IS_CHOOSER_HOST (host), NULL);

        return host->priv->address;
}

const char* mdm_chooser_host_get_description(MdmChooserHost* host)
{
	g_return_val_if_fail(MDM_IS_CHOOSER_HOST(host), NULL);

	return host->priv->description;
}

MdmChooserHostKind
mdm_chooser_host_get_kind (MdmChooserHost *host)
{
        g_return_val_if_fail (MDM_IS_CHOOSER_HOST (host), 0);

        return host->priv->kind;
}

gboolean
mdm_chooser_host_get_willing (MdmChooserHost *host)
{
        g_return_val_if_fail (MDM_IS_CHOOSER_HOST (host), FALSE);

        return host->priv->willing;
}

static void
_mdm_chooser_host_set_address (MdmChooserHost *host,
                               MdmAddress     *address)
{
        if (host->priv->address != NULL) {
                mdm_address_free (host->priv->address);
        }

        g_assert (address != NULL);

        mdm_address_debug (address);
        host->priv->address = mdm_address_copy (address);
}

static void
_mdm_chooser_host_set_description (MdmChooserHost *host,
                                   const char     *description)
{
        g_free (host->priv->description);
        host->priv->description = g_strdup (description);
}

static void
_mdm_chooser_host_set_kind (MdmChooserHost *host,
                            int             kind)
{
        if (host->priv->kind != kind) {
                host->priv->kind = kind;
        }
}

static void
_mdm_chooser_host_set_willing (MdmChooserHost *host,
                               gboolean        willing)
{
        if (host->priv->willing != willing) {
                host->priv->willing = willing;
        }
}

static void
mdm_chooser_host_set_property (GObject      *object,
                               guint         param_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
        MdmChooserHost *host;

        host = MDM_CHOOSER_HOST (object);

        switch (param_id) {
        case PROP_ADDRESS:
                _mdm_chooser_host_set_address (host, g_value_get_boxed (value));
                break;
        case PROP_DESCRIPTION:
                _mdm_chooser_host_set_description (host, g_value_get_string (value));
                break;
        case PROP_KIND:
                _mdm_chooser_host_set_kind (host, g_value_get_int (value));
                break;
        case PROP_WILLING:
                _mdm_chooser_host_set_willing (host, g_value_get_boolean (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
                break;
        }
}

static void
mdm_chooser_host_get_property (GObject    *object,
                               guint       param_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
        MdmChooserHost *host;

        host = MDM_CHOOSER_HOST (object);

        switch (param_id) {
        case PROP_ADDRESS:
                g_value_set_boxed (value, host->priv->address);
                break;
        case PROP_DESCRIPTION:
                g_value_set_string (value, host->priv->description);
                break;
        case PROP_KIND:
                g_value_set_int (value, host->priv->kind);
                break;
        case PROP_WILLING:
                g_value_set_boolean (value, host->priv->willing);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
                break;
        }
}

static void
mdm_chooser_host_class_init (MdmChooserHostClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->set_property = mdm_chooser_host_set_property;
        object_class->get_property = mdm_chooser_host_get_property;
        object_class->finalize = mdm_chooser_host_finalize;


        g_object_class_install_property (object_class,
                                         PROP_ADDRESS,
                                         g_param_spec_boxed ("address",
                                                             "address",
                                                             "address",
                                                             MDM_TYPE_ADDRESS,
                                                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
        g_object_class_install_property (object_class,
                                         PROP_DESCRIPTION,
                                         g_param_spec_string ("description",
                                                              "description",
                                                              "description",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

        g_object_class_install_property (object_class,
                                         PROP_KIND,
                                         g_param_spec_int ("kind",
                                                           "kind",
                                                           "kind",
                                                           0,
                                                           G_MAXINT,
                                                           0,
                                                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
        g_object_class_install_property (object_class,
                                         PROP_WILLING,
                                         g_param_spec_boolean ("willing",
                                                               "willing",
                                                               "willing",
                                                               FALSE,
                                                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT));


        g_type_class_add_private (klass, sizeof (MdmChooserHostPrivate));
}

static void
mdm_chooser_host_init (MdmChooserHost *widget)
{
        widget->priv = MDM_CHOOSER_HOST_GET_PRIVATE (widget);
}

static void
mdm_chooser_host_finalize (GObject *object)
{
        MdmChooserHost *host;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_CHOOSER_HOST (object));

        host = MDM_CHOOSER_HOST (object);

        g_return_if_fail (host->priv != NULL);

        g_free (host->priv->description);
        mdm_address_free (host->priv->address);

        G_OBJECT_CLASS (mdm_chooser_host_parent_class)->finalize (object);
}
