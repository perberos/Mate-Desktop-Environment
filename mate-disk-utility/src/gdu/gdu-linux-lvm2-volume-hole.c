/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2007-2010 David Zeuthen
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

#include "gdu-private.h"
#include "gdu-util.h"
#include "gdu-pool.h"
#include "gdu-device.h"
#include "gdu-linux-lvm2-volume-group.h"
#include "gdu-linux-lvm2-volume-hole.h"
#include "gdu-presentable.h"

struct _GduLinuxLvm2VolumeHolePrivate
{
        GduPool *pool;

        gchar *id;

        GduPresentable *enclosing_presentable;
};

static GObjectClass *parent_class = NULL;

static void gdu_linux_lvm2_volume_hole_presentable_iface_init (GduPresentableIface *iface);

G_DEFINE_TYPE_WITH_CODE (GduLinuxLvm2VolumeHole, gdu_linux_lvm2_volume_hole, GDU_TYPE_VOLUME_HOLE,
                         G_IMPLEMENT_INTERFACE (GDU_TYPE_PRESENTABLE,
                                                gdu_linux_lvm2_volume_hole_presentable_iface_init))

static void on_presentable_changed (GduPresentable *presentable, gpointer user_data);

static void
gdu_linux_lvm2_volume_hole_finalize (GObject *object)
{
        GduLinuxLvm2VolumeHole *volume_hole = GDU_LINUX_LVM2_VOLUME_HOLE (object);

        //g_debug ("##### finalized linux-lvm2 volume_hole '%s' %p", volume_hole->priv->id, lv);

        g_free (volume_hole->priv->id);

        if (volume_hole->priv->enclosing_presentable != NULL) {
                g_signal_handlers_disconnect_by_func (volume_hole->priv->enclosing_presentable,
                                                      on_presentable_changed,
                                                      volume_hole);
                g_object_unref (volume_hole->priv->enclosing_presentable);
        }

        if (G_OBJECT_CLASS (parent_class)->finalize)
                (* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

static void
gdu_linux_lvm2_volume_hole_class_init (GduLinuxLvm2VolumeHoleClass *klass)
{
        GObjectClass *gobject_class = (GObjectClass *) klass;

        parent_class = g_type_class_peek_parent (klass);

        gobject_class->finalize = gdu_linux_lvm2_volume_hole_finalize;

        g_type_class_add_private (klass, sizeof (GduLinuxLvm2VolumeHolePrivate));
}

static void
gdu_linux_lvm2_volume_hole_init (GduLinuxLvm2VolumeHole *volume_hole)
{
        volume_hole->priv = G_TYPE_INSTANCE_GET_PRIVATE (volume_hole, GDU_TYPE_LINUX_LVM2_VOLUME_HOLE, GduLinuxLvm2VolumeHolePrivate);
}

/**
 * _gdu_linux_lvm2_volume_hole_new:
 * @pool: A #GduPool.
 * @enclosing_presentable: The enclosing presentable.
 *
 * Creates a new #GduLinuxLvm2VolumeHole.
 */
GduLinuxLvm2VolumeHole *
_gdu_linux_lvm2_volume_hole_new (GduPool        *pool,
                                 GduPresentable *enclosing_presentable)
{
        GduLinuxLvm2VolumeHole *volume_hole;

        volume_hole = GDU_LINUX_LVM2_VOLUME_HOLE (g_object_new (GDU_TYPE_LINUX_LVM2_VOLUME_HOLE, NULL));
        volume_hole->priv->pool = g_object_ref (pool);

        volume_hole->priv->id = g_strdup_printf ("linux_lvm2_volume_hole_enclosed_by_%s",
                                                 enclosing_presentable != NULL ? gdu_presentable_get_id (enclosing_presentable) : "(none)");

        volume_hole->priv->enclosing_presentable =
                enclosing_presentable != NULL ? g_object_ref (enclosing_presentable) : NULL;

        /* Track the VG since we get the amount of free space from there */
        if (volume_hole->priv->enclosing_presentable != NULL) {
                g_signal_connect (volume_hole->priv->enclosing_presentable,
                                  "changed",
                                  G_CALLBACK (on_presentable_changed),
                                  volume_hole);
        }

        return volume_hole;
}

static void
on_presentable_changed (GduPresentable *presentable, gpointer user_data)
{
        GduLinuxLvm2VolumeHole *volume_hole = GDU_LINUX_LVM2_VOLUME_HOLE (user_data);

        g_signal_emit_by_name (volume_hole, "changed");
        g_signal_emit_by_name (volume_hole->priv->pool, "presentable-changed", volume_hole);
}

/* ---------------------------------------------------------------------------------------------------- */

/* GduPresentable methods */

static const gchar *
gdu_linux_lvm2_volume_hole_get_id (GduPresentable *presentable)
{
        GduLinuxLvm2VolumeHole *volume_hole = GDU_LINUX_LVM2_VOLUME_HOLE (presentable);

        return volume_hole->priv->id;
}

static GduDevice *
gdu_linux_lvm2_volume_hole_get_device (GduPresentable *presentable)
{
        return NULL;
}

static GduPresentable *
gdu_linux_lvm2_volume_hole_get_enclosing_presentable (GduPresentable *presentable)
{
        GduLinuxLvm2VolumeHole *volume_hole = GDU_LINUX_LVM2_VOLUME_HOLE (presentable);

        if (volume_hole->priv->enclosing_presentable != NULL)
                return g_object_ref (volume_hole->priv->enclosing_presentable);
        return NULL;
}

static guint64
gdu_linux_lvm2_volume_hole_get_size (GduPresentable *presentable)
{
        GduLinuxLvm2VolumeHole *volume_hole = GDU_LINUX_LVM2_VOLUME_HOLE (presentable);
        GduDevice *pv_device;
        guint64 ret;

        ret = 0;

        pv_device = gdu_linux_lvm2_volume_group_get_pv_device (GDU_LINUX_LVM2_VOLUME_GROUP (volume_hole->priv->enclosing_presentable));
        if (pv_device != NULL) {
                ret = gdu_device_linux_lvm2_pv_get_group_unallocated_size (pv_device);
                g_object_unref (pv_device);
        }

        return ret;
}

static gchar *
get_names_and_desc (GduPresentable  *presentable,
                    gchar          **out_vpd_name,
                    gchar          **out_desc)
{
        gchar *ret;
        gchar *ret_desc;
        gchar *ret_vpd;
        guint64 size;
        gchar *strsize;

        ret = NULL;
        ret_desc = NULL;
        ret_vpd = NULL;

        size = gdu_linux_lvm2_volume_hole_get_size (presentable);
        strsize = gdu_util_get_size_for_display (size, FALSE, FALSE);

        /* Translators: label for an unallocated space in a LVM2 volume group.
         * %s is the size, formatted like '45 GB'
         */
        ret = g_strdup_printf (_("%s Free"), strsize);
        g_free (strsize);

        /* Translators: Description */
        ret_desc = g_strdup (_("Unallocated Space"));

        /* Translators: VPD name */
        ret_vpd = g_strdup (_("LVM2 VG Unallocated Space"));

        if (out_desc != NULL)
                *out_desc = ret_desc;
        else
                g_free (ret_desc);

        if (out_vpd_name != NULL)
                *out_vpd_name = ret_vpd;
        else
                g_free (ret_vpd);

        return ret;
}

static char *
gdu_linux_lvm2_volume_hole_get_name (GduPresentable *presentable)
{
        return get_names_and_desc (presentable, NULL, NULL);
}

static gchar *
gdu_linux_lvm2_volume_hole_get_description (GduPresentable *presentable)
{
        gchar *desc;
        gchar *name;

        name = get_names_and_desc (presentable, NULL, &desc);
        g_free (name);

        return desc;
}

static gchar *
gdu_linux_lvm2_volume_hole_get_vpd_name (GduPresentable *presentable)
{
        gchar *vpd_name;
        gchar *name;

        name = get_names_and_desc (presentable, &vpd_name, NULL);
        g_free (name);

        return vpd_name;
}

static GIcon *
gdu_linux_lvm2_volume_hole_get_icon (GduPresentable *presentable)
{
        return g_themed_icon_new_with_default_fallbacks ("gdu-multidisk-drive");
}

static guint64
gdu_linux_lvm2_volume_hole_get_offset (GduPresentable *presentable)
{
        /* Halfway to G_MAXUINT64 - should guarantee that we're always at the end... */
        return G_MAXINT64;
}

static GduPool *
gdu_linux_lvm2_volume_hole_get_pool (GduPresentable *presentable)
{
        GduLinuxLvm2VolumeHole *volume_hole = GDU_LINUX_LVM2_VOLUME_HOLE (presentable);
        return g_object_ref (volume_hole->priv->pool);
}

static gboolean
gdu_linux_lvm2_volume_hole_is_allocated (GduPresentable *presentable)
{
        return FALSE;
}

static gboolean
gdu_linux_lvm2_volume_hole_is_recognized (GduPresentable *presentable)
{
        /* TODO: maybe we need to return FALSE sometimes */
        return TRUE;
}

static void
gdu_linux_lvm2_volume_hole_presentable_iface_init (GduPresentableIface *iface)
{
        iface->get_id                    = gdu_linux_lvm2_volume_hole_get_id;
        iface->get_device                = gdu_linux_lvm2_volume_hole_get_device;
        iface->get_enclosing_presentable = gdu_linux_lvm2_volume_hole_get_enclosing_presentable;
        iface->get_name                  = gdu_linux_lvm2_volume_hole_get_name;
        iface->get_description           = gdu_linux_lvm2_volume_hole_get_description;
        iface->get_vpd_name              = gdu_linux_lvm2_volume_hole_get_vpd_name;
        iface->get_icon                  = gdu_linux_lvm2_volume_hole_get_icon;
        iface->get_offset                = gdu_linux_lvm2_volume_hole_get_offset;
        iface->get_size                  = gdu_linux_lvm2_volume_hole_get_size;
        iface->get_pool                  = gdu_linux_lvm2_volume_hole_get_pool;
        iface->is_allocated              = gdu_linux_lvm2_volume_hole_is_allocated;
        iface->is_recognized             = gdu_linux_lvm2_volume_hole_is_recognized;
}

/* ---------------------------------------------------------------------------------------------------- */

void
_gdu_linux_lvm2_volume_hole_rewrite_enclosing_presentable (GduLinuxLvm2VolumeHole *volume_hole)
{
        if (volume_hole->priv->enclosing_presentable != NULL) {
                const gchar *enclosing_presentable_id;
                GduPresentable *new_enclosing_presentable;

                enclosing_presentable_id = gdu_presentable_get_id (volume_hole->priv->enclosing_presentable);

                new_enclosing_presentable = gdu_pool_get_presentable_by_id (volume_hole->priv->pool,
                                                                            enclosing_presentable_id);
                if (new_enclosing_presentable == NULL) {
                        g_warning ("Error rewriting enclosing_presentable for %s, no such id %s",
                                   volume_hole->priv->id,
                                   enclosing_presentable_id);
                        goto out;
                }

                g_object_unref (volume_hole->priv->enclosing_presentable);
                volume_hole->priv->enclosing_presentable = new_enclosing_presentable;
        }

 out:
        ;
}

/* ---------------------------------------------------------------------------------------------------- */
