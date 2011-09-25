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

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>

#include "mdm-display-factory.h"
#include "mdm-display-store.h"

#define MDM_DISPLAY_FACTORY_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_DISPLAY_FACTORY, MdmDisplayFactoryPrivate))

struct MdmDisplayFactoryPrivate
{
        MdmDisplayStore *display_store;
};

enum {
        PROP_0,
        PROP_DISPLAY_STORE,
};

static void     mdm_display_factory_class_init  (MdmDisplayFactoryClass *klass);
static void     mdm_display_factory_init        (MdmDisplayFactory      *factory);
static void     mdm_display_factory_finalize    (GObject                *object);

G_DEFINE_ABSTRACT_TYPE (MdmDisplayFactory, mdm_display_factory, G_TYPE_OBJECT)

GQuark
mdm_display_factory_error_quark (void)
{
        static GQuark ret = 0;
        if (ret == 0) {
                ret = g_quark_from_static_string ("mdm_display_factory_error");
        }

        return ret;
}

MdmDisplayStore *
mdm_display_factory_get_display_store (MdmDisplayFactory *factory)
{
        g_return_val_if_fail (MDM_IS_DISPLAY_FACTORY (factory), NULL);

        return factory->priv->display_store;
}

gboolean
mdm_display_factory_start (MdmDisplayFactory *factory)
{
        gboolean ret;

        g_return_val_if_fail (MDM_IS_DISPLAY_FACTORY (factory), FALSE);

        g_object_ref (factory);
        ret = MDM_DISPLAY_FACTORY_GET_CLASS (factory)->start (factory);
        g_object_unref (factory);

        return ret;
}

gboolean
mdm_display_factory_stop (MdmDisplayFactory *factory)
{
        gboolean ret;

        g_return_val_if_fail (MDM_IS_DISPLAY_FACTORY (factory), FALSE);

        g_object_ref (factory);
        ret = MDM_DISPLAY_FACTORY_GET_CLASS (factory)->stop (factory);
        g_object_unref (factory);

        return ret;
}

static void
mdm_display_factory_set_display_store (MdmDisplayFactory *factory,
                                       MdmDisplayStore   *display_store)
{
        if (factory->priv->display_store != NULL) {
                g_object_unref (factory->priv->display_store);
                factory->priv->display_store = NULL;
        }

        if (display_store != NULL) {
                factory->priv->display_store = g_object_ref (display_store);
        }
}

static void
mdm_display_factory_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
        MdmDisplayFactory *self;

        self = MDM_DISPLAY_FACTORY (object);

        switch (prop_id) {
        case PROP_DISPLAY_STORE:
                mdm_display_factory_set_display_store (self, g_value_get_object (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
mdm_display_factory_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
        MdmDisplayFactory *self;

        self = MDM_DISPLAY_FACTORY (object);

        switch (prop_id) {
        case PROP_DISPLAY_STORE:
                g_value_set_object (value, self->priv->display_store);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
mdm_display_factory_class_init (MdmDisplayFactoryClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = mdm_display_factory_get_property;
        object_class->set_property = mdm_display_factory_set_property;
        object_class->finalize = mdm_display_factory_finalize;

        g_object_class_install_property (object_class,
                                         PROP_DISPLAY_STORE,
                                         g_param_spec_object ("display-store",
                                                              "display store",
                                                              "display store",
                                                              MDM_TYPE_DISPLAY_STORE,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

        g_type_class_add_private (klass, sizeof (MdmDisplayFactoryPrivate));
}

static void
mdm_display_factory_init (MdmDisplayFactory *factory)
{
        factory->priv = MDM_DISPLAY_FACTORY_GET_PRIVATE (factory);
}

static void
mdm_display_factory_finalize (GObject *object)
{
        MdmDisplayFactory *factory;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_DISPLAY_FACTORY (object));

        factory = MDM_DISPLAY_FACTORY (object);

        g_return_if_fail (factory->priv != NULL);

        G_OBJECT_CLASS (mdm_display_factory_parent_class)->finalize (object);
}
