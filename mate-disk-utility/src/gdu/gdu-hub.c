/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-hub.c
 *
 * Copyright (C) 2007 David Zeuthen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include <string.h>
#include <dbus/dbus-glib.h>
#include <stdlib.h>

#include "gdu-private.h"
#include "gdu-util.h"
#include "gdu-pool.h"
#include "gdu-adapter.h"
#include "gdu-expander.h"
#include "gdu-hub.h"
#include "gdu-presentable.h"
#include "gdu-linux-md-drive.h"

/**
 * SECTION:gdu-hub
 * @title: GduHub
 * @short_description: HUBs
 *
 * #GduHub objects are used to represent Host Adapters and Expanders
 * (e.g. SAS Expanders and SATA Port Multipliers).
 *
 * See the documentation for #GduPresentable for the big picture.
 */

struct _GduHubPrivate
{
        GduHubUsage usage;
        GduAdapter *adapter;
        GduExpander *expander;
        GduPool *pool;
        GduPresentable *enclosing_presentable;
        gchar *id;

        gchar *given_name;
        gchar *given_vpd_name;
        GIcon *given_icon;
};

static GObjectClass *parent_class = NULL;

static void gdu_hub_presentable_iface_init (GduPresentableIface *iface);
G_DEFINE_TYPE_WITH_CODE (GduHub, gdu_hub, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GDU_TYPE_PRESENTABLE,
                                                gdu_hub_presentable_iface_init))

static void adapter_changed (GduAdapter *adapter, gpointer user_data);
static void expander_changed (GduExpander *expander, gpointer user_data);


static void
gdu_hub_finalize (GObject *object)
{
        GduHub *hub = GDU_HUB (object);

        //g_debug ("##### finalized hub '%s' %p", hub->priv->id, hub);

        if (hub->priv->expander != NULL) {
                g_signal_handlers_disconnect_by_func (hub->priv->expander, expander_changed, hub);
                g_object_unref (hub->priv->expander);
        }

        if (hub->priv->adapter != NULL) {
                g_signal_handlers_disconnect_by_func (hub->priv->adapter, adapter_changed, hub);
                g_object_unref (hub->priv->adapter);
        }

        if (hub->priv->pool != NULL)
                g_object_unref (hub->priv->pool);

        if (hub->priv->enclosing_presentable != NULL)
                g_object_unref (hub->priv->enclosing_presentable);

        g_free (hub->priv->id);

        g_free (hub->priv->given_name);
        g_free (hub->priv->given_vpd_name);
        if (hub->priv->given_icon != NULL)
                g_object_unref (hub->priv->given_icon);

        if (G_OBJECT_CLASS (parent_class)->finalize != NULL)
                (* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

static void
gdu_hub_class_init (GduHubClass *klass)
{
        GObjectClass *obj_class = (GObjectClass *) klass;

        parent_class = g_type_class_peek_parent (klass);

        obj_class->finalize = gdu_hub_finalize;

        g_type_class_add_private (klass, sizeof (GduHubPrivate));
}

static void
gdu_hub_init (GduHub *hub)
{
        hub->priv = G_TYPE_INSTANCE_GET_PRIVATE (hub, GDU_TYPE_HUB, GduHubPrivate);
}

static void
adapter_changed (GduAdapter *adapter, gpointer user_data)
{
        GduHub *hub = GDU_HUB (user_data);
        g_signal_emit_by_name (hub, "changed");
        g_signal_emit_by_name (hub->priv->pool, "presentable-changed", hub);
}

static void
expander_changed (GduExpander *expander, gpointer user_data)
{
        GduHub *hub = GDU_HUB (user_data);
        g_signal_emit_by_name (hub, "changed");
        g_signal_emit_by_name (hub->priv->pool, "presentable-changed", hub);
}

/**
 *
 * If @usage is %GDU_HUB_USAGE_ADAPTER the @adapter must not be %NULL and @expander, @name, @vpd_name and @icon must all be %NULL.
 * If @usage is %GDU_HUB_USAGE_EXPANDER the @adapter and @expander must not be %NULL and @name, @vpd_name and @icon must all be %NULL.
 * Otherwise @adapter and @expander must be %NULL and @name, @vpd_name and @icon must not not %NULL.
 */
GduHub *
_gdu_hub_new (GduPool        *pool,
              GduHubUsage     usage,
              GduAdapter     *adapter,
              GduExpander    *expander,
              const gchar    *name,
              const gchar    *vpd_name,
              GIcon          *icon,
              GduPresentable *enclosing_presentable)
{
        GduHub *hub;

        hub = GDU_HUB (g_object_new (GDU_TYPE_HUB, NULL));
        hub->priv->adapter = adapter != NULL ? g_object_ref (adapter) : NULL;
        hub->priv->expander = expander != NULL ? g_object_ref (expander) : NULL;
        hub->priv->pool = g_object_ref (pool);
        hub->priv->enclosing_presentable =
                enclosing_presentable != NULL ? g_object_ref (enclosing_presentable) : NULL;
        if (expander != NULL) {
                hub->priv->id = g_strdup_printf ("%s__enclosed_by_%s",
                                                 gdu_expander_get_native_path (hub->priv->expander),
                                                 enclosing_presentable != NULL ? gdu_presentable_get_id (enclosing_presentable) : "(none)");
        } else if (adapter != NULL) {
                hub->priv->id = g_strdup_printf ("%s__enclosed_by_%s",
                                                 gdu_adapter_get_native_path (hub->priv->adapter),
                                                 enclosing_presentable != NULL ? gdu_presentable_get_id (enclosing_presentable) : "(none)");
        } else {
                hub->priv->id = g_strdup_printf ("%s__enclosed_by_%s",
                                                 name,
                                                 enclosing_presentable != NULL ? gdu_presentable_get_id (enclosing_presentable) : "(none)");
        }
        if (adapter != NULL)
                g_signal_connect (adapter, "changed", (GCallback) adapter_changed, hub);
        if (expander != NULL)
                g_signal_connect (expander, "changed", (GCallback) expander_changed, hub);

        hub->priv->usage = usage;
        hub->priv->given_name = g_strdup (name);
        hub->priv->given_vpd_name = g_strdup (vpd_name);
        hub->priv->given_icon = icon != NULL ? g_object_ref (icon) : NULL;

        return hub;
}

static const gchar *
gdu_hub_get_id (GduPresentable *presentable)
{
        GduHub *hub = GDU_HUB (presentable);
        return hub->priv->id;
}

static GduDevice *
gdu_hub_get_device (GduPresentable *presentable)
{
        return NULL;
}

static GduPresentable *
gdu_hub_get_enclosing_presentable (GduPresentable *presentable)
{
        GduHub *hub = GDU_HUB (presentable);
        if (hub->priv->enclosing_presentable != NULL)
                return g_object_ref (hub->priv->enclosing_presentable);
        return NULL;
}

static char *
gdu_hub_get_name (GduPresentable *presentable)
{
        GduHub *hub = GDU_HUB (presentable);
        gchar *ret;

        if (hub->priv->expander != NULL) {
                /* TODO: include type e.g. SATA Port Multiplier, SAS Expander etc */
                ret = g_strdup (_("SAS Expander"));

        } else if (hub->priv->adapter != NULL) {
                const gchar *fabric;

                fabric = gdu_adapter_get_fabric (hub->priv->adapter);

                if (g_str_has_prefix (fabric, "ata_pata")) {
                        ret = g_strdup (_("PATA Host Adapter"));
                } else if (g_str_has_prefix (fabric, "ata_sata")) {
                        ret = g_strdup (_("SATA Host Adapter"));
                } else if (g_str_has_prefix (fabric, "ata")) {
                        ret = g_strdup (_("ATA Host Adapter"));
                } else if (g_str_has_prefix (fabric, "scsi_sas")) {
                        ret = g_strdup (_("SAS Host Adapter"));
                } else if (g_str_has_prefix (fabric, "scsi")) {
                        ret = g_strdup (_("SCSI Host Adapter"));
                } else {
                        ret = g_strdup (_("Host Adapter"));
                }

        } else {
                ret = g_strdup (hub->priv->given_name);
        }

        return ret;
}

static gchar *
gdu_hub_get_vpd_name (GduPresentable *presentable)
{
        GduHub *hub = GDU_HUB (presentable);
        gchar *s;
        const gchar *vendor;
        const gchar *model;

        if (hub->priv->expander != NULL) {
                vendor = gdu_expander_get_vendor (hub->priv->expander);
                model = gdu_expander_get_model (hub->priv->expander);
                s = g_strdup_printf ("%s %s", vendor, model);
        } else if (hub->priv->adapter != NULL) {
                vendor = gdu_adapter_get_vendor (hub->priv->adapter);
                model = gdu_adapter_get_model (hub->priv->adapter);
                //s = g_strdup_printf ("%s %s", vendor, model);
                s = g_strdup (model);
        } else {
                s = g_strdup (hub->priv->given_vpd_name);
        }

        return s;
}

static gchar *
gdu_hub_get_description (GduPresentable *presentable)
{
        /* TODO: include number of ports, speed, receptable type etc. */
        return gdu_hub_get_vpd_name (presentable);
}

static GIcon *
gdu_hub_get_icon (GduPresentable *presentable)
{
        GIcon *icon;
        GduHub *hub = GDU_HUB (presentable);

        if (hub->priv->given_icon != NULL) {
                icon = g_object_ref (hub->priv->given_icon);
        } else {
                if (hub->priv->usage == GDU_HUB_USAGE_EXPANDER)
                        icon = g_themed_icon_new_with_default_fallbacks ("gdu-expander");
                else
                        icon = g_themed_icon_new_with_default_fallbacks ("gdu-hba");
        }

        return icon;
}

static guint64
gdu_hub_get_offset (GduPresentable *presentable)
{
        return 0;
}

static guint64
gdu_hub_get_size (GduPresentable *presentable)
{
        return 0;
}

static GduPool *
gdu_hub_get_pool (GduPresentable *presentable)
{
        GduHub *hub = GDU_HUB (presentable);
        return g_object_ref (hub->priv->pool);
}

static gboolean
gdu_hub_is_allocated (GduPresentable *presentable)
{
        return FALSE;
}

static gboolean
gdu_hub_is_recognized (GduPresentable *presentable)
{
        return FALSE;
}

GduAdapter *
gdu_hub_get_adapter (GduHub *hub)
{
        return hub->priv->adapter != NULL ? g_object_ref (hub->priv->adapter) : NULL;
}

GduExpander *
gdu_hub_get_expander (GduHub *hub)
{
        return hub->priv->expander != NULL ? g_object_ref (hub->priv->expander) : NULL;
}

static void
gdu_hub_presentable_iface_init (GduPresentableIface *iface)
{
        iface->get_id                    = gdu_hub_get_id;
        iface->get_device                = gdu_hub_get_device;
        iface->get_enclosing_presentable = gdu_hub_get_enclosing_presentable;
        iface->get_name                  = gdu_hub_get_name;
        iface->get_description           = gdu_hub_get_description;
        iface->get_vpd_name              = gdu_hub_get_vpd_name;
        iface->get_icon                  = gdu_hub_get_icon;
        iface->get_offset                = gdu_hub_get_offset;
        iface->get_size                  = gdu_hub_get_size;
        iface->get_pool                  = gdu_hub_get_pool;
        iface->is_allocated              = gdu_hub_is_allocated;
        iface->is_recognized             = gdu_hub_is_recognized;
}

void
_gdu_hub_rewrite_enclosing_presentable (GduHub *hub)
{
        if (hub->priv->enclosing_presentable != NULL) {
                const gchar *enclosing_presentable_id;
                GduPresentable *new_enclosing_presentable;

                enclosing_presentable_id = gdu_presentable_get_id (hub->priv->enclosing_presentable);

                new_enclosing_presentable = gdu_pool_get_presentable_by_id (hub->priv->pool,
                                                                            enclosing_presentable_id);
                if (new_enclosing_presentable == NULL) {
                        g_warning ("Error rewriting enclosing_presentable for %s, no such id %s",
                                   hub->priv->id,
                                   enclosing_presentable_id);
                        goto out;
                }

                g_object_unref (hub->priv->enclosing_presentable);
                hub->priv->enclosing_presentable = new_enclosing_presentable;
        }

 out:
        ;
}
