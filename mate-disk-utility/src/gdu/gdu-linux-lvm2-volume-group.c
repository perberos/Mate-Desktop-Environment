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
#include "gdu-linux-lvm2-volume.h"
#include "gdu-presentable.h"
#include "gdu-volume-hole.h"

struct _GduLinuxLvm2VolumeGroupPrivate
{
        GList *slaves;

        GduPool *pool;

        gchar *uuid;

        /* list of PV GduDevice objects that makes up this VG */
        GList *pv_devices;
        /* the PV for which we get VG data from (highest seqnum) */
        GduDevice *pv;

        gchar *id;

        GduPresentable *enclosing_presentable;
};

static GObjectClass *parent_class = NULL;

static void gdu_linux_lvm2_volume_group_presentable_iface_init (GduPresentableIface *iface);

static gboolean gdu_linux_lvm2_volume_group_is_active (GduDrive *drive);
static gboolean gdu_linux_lvm2_volume_group_is_activatable (GduDrive *drive);
static gboolean gdu_linux_lvm2_volume_group_can_deactivate (GduDrive *drive);
static gboolean gdu_linux_lvm2_volume_group_can_activate (GduDrive *drive, gboolean *out_degraded);

static void gdu_linux_lvm2_volume_group_activate (GduDrive              *drive,
                                                  GduDriveActivateFunc   callback,
                                                  gpointer               user_data);

static void gdu_linux_lvm2_volume_group_deactivate (GduDrive                *drive,
                                                    GduDriveDeactivateFunc   callback,
                                                    gpointer                 user_data);

static gboolean gdu_linux_lvm2_volume_group_can_create_volume (GduDrive        *drive,
                                                               gboolean        *out_is_uninitialized,
                                                               guint64         *out_largest_contiguous_free_segment,
                                                               guint64         *out_total_free,
                                                               GduPresentable **out_presentable);

static void gdu_linux_lvm2_volume_group_create_volume (GduDrive              *drive,
                                                       guint64                size,
                                                       const gchar           *name,
                                                       GduCreateVolumeFlags   flags,
                                                       GAsyncReadyCallback    callback,
                                                       gpointer               user_data);

static GduVolume *gdu_linux_lvm2_volume_group_create_volume_finish (GduDrive              *drive,
                                                                    GAsyncResult          *res,
                                                                    GError               **error);

static void on_device_added (GduPool *pool, GduDevice *device, gpointer user_data);
static void on_device_removed (GduPool *pool, GduDevice *device, gpointer user_data);
static void on_device_changed (GduPool *pool, GduDevice *device, gpointer user_data);

G_DEFINE_TYPE_WITH_CODE (GduLinuxLvm2VolumeGroup, gdu_linux_lvm2_volume_group, GDU_TYPE_DRIVE,
                         G_IMPLEMENT_INTERFACE (GDU_TYPE_PRESENTABLE,
                                                gdu_linux_lvm2_volume_group_presentable_iface_init))

static void
gdu_linux_lvm2_volume_group_finalize (GObject *object)
{
        GduLinuxLvm2VolumeGroup *vg = GDU_LINUX_LVM2_VOLUME_GROUP (object);

        //g_debug ("##### finalized linux-lvm2 volume group '%s' %p", vg->priv->id, vg);

        if (vg->priv->pool != NULL) {
                g_signal_handlers_disconnect_by_func (vg->priv->pool, on_device_added, vg);
                g_signal_handlers_disconnect_by_func (vg->priv->pool, on_device_removed, vg);
                g_signal_handlers_disconnect_by_func (vg->priv->pool, on_device_changed, vg);
                g_object_unref (vg->priv->pool);
        }

        g_free (vg->priv->id);
        g_free (vg->priv->uuid);

        if (vg->priv->enclosing_presentable != NULL)
                g_object_unref (vg->priv->enclosing_presentable);

        g_list_foreach (vg->priv->pv_devices, (GFunc) g_object_unref, NULL);
        g_list_free (vg->priv->pv_devices);
        if (vg->priv->pv != NULL)
                g_object_unref (vg->priv->pv);

        if (G_OBJECT_CLASS (parent_class)->finalize)
                (* G_OBJECT_CLASS (parent_class)->finalize) (G_OBJECT (vg));
}

static void
gdu_linux_lvm2_volume_group_class_init (GduLinuxLvm2VolumeGroupClass *klass)
{
        GObjectClass *gobject_class = (GObjectClass *) klass;
        GduDriveClass *drive_class = (GduDriveClass *) klass;

        parent_class = g_type_class_peek_parent (klass);

        gobject_class->finalize = gdu_linux_lvm2_volume_group_finalize;

        g_type_class_add_private (klass, sizeof (GduLinuxLvm2VolumeGroupPrivate));

        drive_class->is_active             = gdu_linux_lvm2_volume_group_is_active;
        drive_class->is_activatable        = gdu_linux_lvm2_volume_group_is_activatable;
        drive_class->can_deactivate        = gdu_linux_lvm2_volume_group_can_deactivate;
        drive_class->can_activate          = gdu_linux_lvm2_volume_group_can_activate;
        drive_class->can_create_volume     = gdu_linux_lvm2_volume_group_can_create_volume;
        drive_class->create_volume         = gdu_linux_lvm2_volume_group_create_volume;
        drive_class->create_volume_finish  = gdu_linux_lvm2_volume_group_create_volume_finish;
        drive_class->activate              = gdu_linux_lvm2_volume_group_activate;
        drive_class->deactivate            = gdu_linux_lvm2_volume_group_deactivate;
}

static void
gdu_linux_lvm2_volume_group_init (GduLinuxLvm2VolumeGroup *vg)
{
        vg->priv = G_TYPE_INSTANCE_GET_PRIVATE (vg, GDU_TYPE_LINUX_LVM2_VOLUME_GROUP, GduLinuxLvm2VolumeGroupPrivate);
}

static gboolean
emit_changed_idle_cb (gpointer user_data)
{
        GduLinuxLvm2VolumeGroup *vg = GDU_LINUX_LVM2_VOLUME_GROUP (user_data);
        //g_debug ("emitting changed for uuid '%s'", vg->priv->uuid);
        g_signal_emit_by_name (vg, "changed");
        g_signal_emit_by_name (vg->priv->pool, "presentable-changed", vg);
        g_object_unref (vg);
        return FALSE; /* remove idle source */
}

static void
emit_changed (GduLinuxLvm2VolumeGroup *vg)
{
        /* emit changed in idle so GduPool has a chance to build GduPresentable objects - this is
         * needed for e.g. GduEditLinuxLvm2 where it wants a GduVolume for each PV
         */
        g_idle_add (emit_changed_idle_cb, g_object_ref (vg));
}

static gboolean
find_pvs (GduLinuxLvm2VolumeGroup *vg)
{
        GList *devices;
        GList *l;
        guint64 seq_num;
        GduDevice *pv_to_use;
        gboolean emitted_changed;
        gchar *old_pvs;
        gchar *new_pvs;

        emitted_changed = FALSE;

        /* TODO: do incremental list management instead of recomputing on
         * each add/remove/change event
         */

        /* out with the old.. */
        g_list_foreach (vg->priv->pv_devices, (GFunc) g_object_unref, NULL);
        g_list_free (vg->priv->pv_devices);
        vg->priv->pv_devices = NULL;

        /* find all GduDevice objects for LVs that are part of this VG */
        devices = gdu_pool_get_devices (vg->priv->pool);
        for (l = devices; l != NULL; l = l->next) {
                GduDevice *d = GDU_DEVICE (l->data);
                if (gdu_device_is_linux_lvm2_pv (d) &&
                    g_strcmp0 (gdu_device_linux_lvm2_pv_get_group_uuid (d), vg->priv->uuid) == 0) {
                        vg->priv->pv_devices = g_list_prepend (vg->priv->pv_devices, g_object_ref (d));
                }
        }
        g_list_foreach (devices, (GFunc) g_object_unref, NULL);
        g_list_free (devices);

        /* Find the PV with the highest sequence number and use that */
        seq_num = 0;
        pv_to_use = NULL;
        for (l = vg->priv->pv_devices; l != NULL; l = l->next) {
                GduDevice *d = GDU_DEVICE (l->data);
                if (pv_to_use == NULL || gdu_device_linux_lvm2_pv_get_group_sequence_number (d) > seq_num) {
                        pv_to_use = d;
                }
        }

        old_pvs = NULL;
        if (vg->priv->pv != NULL) {
                old_pvs = g_strjoinv (",", gdu_device_linux_lvm2_pv_get_group_physical_volumes (vg->priv->pv));
        }

        if (pv_to_use == NULL) {
                /* ok, switch to the new LV */
                if (vg->priv->pv != NULL)
                        g_object_unref (vg->priv->pv);
                vg->priv->pv = NULL;

                /* emit changed since data might have changed */
                emit_changed (vg);
                emitted_changed = TRUE;

        } else if (vg->priv->pv == NULL ||
            (gdu_device_linux_lvm2_pv_get_group_sequence_number (pv_to_use) >
             gdu_device_linux_lvm2_pv_get_group_sequence_number (vg->priv->pv))) {
                /* ok, switch to the new PV */
                if (vg->priv->pv != NULL)
                        g_object_unref (vg->priv->pv);
                vg->priv->pv = g_object_ref (pv_to_use);

                /* emit changed since data might have changed */
                emit_changed (vg);
                emitted_changed = TRUE;
        }

        new_pvs = NULL;
        if (vg->priv->pv != NULL) {
                new_pvs = g_strjoinv (",", gdu_device_linux_lvm2_pv_get_group_physical_volumes (vg->priv->pv));
        }

        g_free (old_pvs);
        g_free (new_pvs);

        /* If *anything* on the PVs changed - also emit ::changed ourselves */
        if (g_strcmp0 (old_pvs, new_pvs) != 0) {
                emit_changed (vg);
                emitted_changed = TRUE;
        }

        return emitted_changed;
}

static void
on_device_added (GduPool *pool, GduDevice *device, gpointer user_data)
{
        GduLinuxLvm2VolumeGroup *vg = GDU_LINUX_LVM2_VOLUME_GROUP (user_data);
        find_pvs (vg);
}

static void
on_device_removed (GduPool *pool, GduDevice *device, gpointer user_data)
{
        GduLinuxLvm2VolumeGroup *vg = GDU_LINUX_LVM2_VOLUME_GROUP (user_data);
        find_pvs (vg);
}

static void
on_device_changed (GduPool *pool, GduDevice *device, gpointer user_data)
{
        GduLinuxLvm2VolumeGroup *vg = GDU_LINUX_LVM2_VOLUME_GROUP (user_data);
        gboolean emitted_changed;

        emitted_changed = find_pvs (vg);

        /* propagate change events from LVs and PVs as change events on the VG */
        if (!emitted_changed) {
                if ((gdu_device_is_linux_lvm2_pv (device) &&
                     g_strcmp0 (gdu_device_linux_lvm2_pv_get_group_uuid (device), vg->priv->uuid) == 0) ||
                    (gdu_device_is_linux_lvm2_lv (device) &&
                     g_strcmp0 (gdu_device_linux_lvm2_lv_get_group_uuid (device), vg->priv->uuid) == 0)) {

                        emit_changed (vg);
                }
        }
}

/**
 * _gdu_linux_lvm2_volume_group_new:
 * @pool: A #GduPool.
 * @uuid: The UUID for the volume group.
 * @enclosing_presentable: The enclosing presentable
 *
 * Creates a new #GduLinuxLvm2VolumeGroup. Note that only one of @uuid and
 * @device_file may be %NULL.
 */
GduLinuxLvm2VolumeGroup *
_gdu_linux_lvm2_volume_group_new (GduPool        *pool,
                                  const gchar    *uuid,
                                  GduPresentable *enclosing_presentable)
{
        GduLinuxLvm2VolumeGroup *vg;

        vg = GDU_LINUX_LVM2_VOLUME_GROUP (g_object_new (GDU_TYPE_LINUX_LVM2_VOLUME_GROUP, NULL));
        vg->priv->pool = g_object_ref (pool);
        vg->priv->uuid = g_strdup (uuid);

        vg->priv->id = g_strdup_printf ("linux_lvm2_volume_group_%s_enclosed_by_%s",
                                        uuid,
                                        enclosing_presentable != NULL ? gdu_presentable_get_id (enclosing_presentable) : "(none)");

        vg->priv->enclosing_presentable =
                enclosing_presentable != NULL ? g_object_ref (enclosing_presentable) : NULL;

        g_signal_connect (vg->priv->pool, "device-added", G_CALLBACK (on_device_added), vg);
        g_signal_connect (vg->priv->pool, "device-removed", G_CALLBACK (on_device_removed), vg);
        g_signal_connect (vg->priv->pool, "device-changed", G_CALLBACK (on_device_changed), vg);
        find_pvs (vg);

        return vg;
}

/* ---------------------------------------------------------------------------------------------------- */

/* GduPresentable methods */

static const gchar *
gdu_linux_lvm2_volume_group_get_id (GduPresentable *presentable)
{
        GduLinuxLvm2VolumeGroup *vg = GDU_LINUX_LVM2_VOLUME_GROUP (presentable);

        return vg->priv->id;
}

static GduDevice *
gdu_linux_lvm2_volume_group_get_device (GduPresentable *presentable)
{
        return NULL;
}

static GduPresentable *
gdu_linux_lvm2_volume_group_get_enclosing_presentable (GduPresentable *presentable)
{
        GduLinuxLvm2VolumeGroup *vg = GDU_LINUX_LVM2_VOLUME_GROUP (presentable);
        if (vg->priv->enclosing_presentable != NULL)
                return g_object_ref (vg->priv->enclosing_presentable);
        return NULL;
}

static gchar *
get_names_and_desc (GduPresentable  *presentable,
                    gchar          **out_vpd_name,
                    gchar          **out_desc)
{
        GduLinuxLvm2VolumeGroup *vg = GDU_LINUX_LVM2_VOLUME_GROUP (presentable);
        gchar *ret;
        gchar *ret_desc;
        gchar *ret_vpd;
        guint64 size;
        gchar *size_str;

        ret = NULL;
        ret_desc = NULL;
        ret_vpd = NULL;
        size = 0;
        size_str = NULL;

        if (vg->priv->pv != NULL) {
                ret = g_strdup (gdu_device_linux_lvm2_pv_get_group_name (vg->priv->pv));
                size = gdu_device_linux_lvm2_pv_get_group_size (vg->priv->pv);
                size_str = gdu_util_get_size_for_display (size, FALSE, FALSE);
        } else {
                ret = g_strdup (_("Volume Group"));
        }

        /* Translators: Description */
        ret_desc = g_strdup (_("Volume Group"));

        if (size_str != NULL) {
                /* Translators: VPD name - first %s is the size e.g. '45 GB' */
                ret_vpd = g_strdup_printf (_("%s LVM2 Volume Group"), size_str);
        } else {
                /* Translators: VPD name when size is not known */
                ret_vpd = g_strdup (_("LVM2 Volume Group"));
        }

        if (out_desc != NULL)
                *out_desc = ret_desc;
        else
                g_free (ret_desc);

        if (out_vpd_name != NULL)
                *out_vpd_name = ret_vpd;
        else
                g_free (ret_vpd);

        g_free (size_str);

        return ret;
}

static char *
gdu_linux_lvm2_volume_group_get_name (GduPresentable *presentable)
{
        return get_names_and_desc (presentable, NULL, NULL);
}

static gchar *
gdu_linux_lvm2_volume_group_get_description (GduPresentable *presentable)
{
        gchar *desc;
        gchar *name;

        name = get_names_and_desc (presentable, NULL, &desc);
        g_free (name);

        return desc;
}

static gchar *
gdu_linux_lvm2_volume_group_get_vpd_name (GduPresentable *presentable)
{
        gchar *vpd_name;
        gchar *name;

        name = get_names_and_desc (presentable, &vpd_name, NULL);
        g_free (name);

        return vpd_name;
}

static GIcon *
gdu_linux_lvm2_volume_group_get_icon (GduPresentable *presentable)
{
        return gdu_util_get_emblemed_icon ("gdu-multidisk-drive", "gdu-emblem-lvm-vg");
}

static guint64
gdu_linux_lvm2_volume_group_get_offset (GduPresentable *presentable)
{
        return 0;
}

static guint64
gdu_linux_lvm2_volume_group_get_size (GduPresentable *presentable)
{
        GduLinuxLvm2VolumeGroup *vg = GDU_LINUX_LVM2_VOLUME_GROUP (presentable);
        guint64 ret;

        if (vg->priv->pv != NULL) {
                ret = gdu_device_linux_lvm2_pv_get_group_size (vg->priv->pv);
        } else {
                ret = 0;
        }

        return ret;
}

static GduPool *
gdu_linux_lvm2_volume_group_get_pool (GduPresentable *presentable)
{
        GduLinuxLvm2VolumeGroup *vg = GDU_LINUX_LVM2_VOLUME_GROUP (presentable);
        return g_object_ref (vg->priv->pool);
}

static gboolean
gdu_linux_lvm2_volume_group_is_allocated (GduPresentable *presentable)
{
        return TRUE;
}

static gboolean
gdu_linux_lvm2_volume_group_is_recognized (GduPresentable *presentable)
{
        /* TODO: maybe we need to return FALSE sometimes */
        return TRUE;
}

static void
gdu_linux_lvm2_volume_group_presentable_iface_init (GduPresentableIface *iface)
{
        iface->get_id                    = gdu_linux_lvm2_volume_group_get_id;
        iface->get_device                = gdu_linux_lvm2_volume_group_get_device;
        iface->get_enclosing_presentable = gdu_linux_lvm2_volume_group_get_enclosing_presentable;
        iface->get_name                  = gdu_linux_lvm2_volume_group_get_name;
        iface->get_description           = gdu_linux_lvm2_volume_group_get_description;
        iface->get_vpd_name              = gdu_linux_lvm2_volume_group_get_vpd_name;
        iface->get_icon                  = gdu_linux_lvm2_volume_group_get_icon;
        iface->get_offset                = gdu_linux_lvm2_volume_group_get_offset;
        iface->get_size                  = gdu_linux_lvm2_volume_group_get_size;
        iface->get_pool                  = gdu_linux_lvm2_volume_group_get_pool;
        iface->is_allocated              = gdu_linux_lvm2_volume_group_is_allocated;
        iface->is_recognized             = gdu_linux_lvm2_volume_group_is_recognized;
}

/* ---------------------------------------------------------------------------------------------------- */

void
_gdu_linux_lvm2_volume_group_rewrite_enclosing_presentable (GduLinuxLvm2VolumeGroup *vg)
{
        if (vg->priv->enclosing_presentable != NULL) {
                const gchar *enclosing_presentable_id;
                GduPresentable *new_enclosing_presentable;

                enclosing_presentable_id = gdu_presentable_get_id (vg->priv->enclosing_presentable);

                new_enclosing_presentable = gdu_pool_get_presentable_by_id (vg->priv->pool,
                                                                            enclosing_presentable_id);
                if (new_enclosing_presentable == NULL) {
                        g_warning ("Error rewriting enclosing_presentable for %s, no such id %s",
                                   vg->priv->id,
                                   enclosing_presentable_id);
                        goto out;
                }

                g_object_unref (vg->priv->enclosing_presentable);
                vg->priv->enclosing_presentable = new_enclosing_presentable;
        }

 out:
        ;
}

/* ---------------------------------------------------------------------------------------------------- */
/* GduDrive virtual method overrides */

static gboolean
gdu_linux_lvm2_volume_group_is_active (GduDrive *drive)
{
        GduLinuxLvm2VolumeGroup *vg = GDU_LINUX_LVM2_VOLUME_GROUP (drive);
        return gdu_linux_lvm2_volume_group_get_state (vg) != GDU_LINUX_LVM2_VOLUME_GROUP_STATE_NOT_RUNNING;
}

static gboolean
gdu_linux_lvm2_volume_group_is_activatable (GduDrive *drive)
{
        return TRUE;
}

static gboolean
gdu_linux_lvm2_volume_group_can_deactivate (GduDrive *drive)
{
        GduLinuxLvm2VolumeGroup *vg = GDU_LINUX_LVM2_VOLUME_GROUP (drive);
        return gdu_linux_lvm2_volume_group_get_state (vg) != GDU_LINUX_LVM2_VOLUME_GROUP_STATE_NOT_RUNNING;
}

static gboolean
gdu_linux_lvm2_volume_group_can_activate (GduDrive *drive,
                                          gboolean *out_degraded)
{
        GduLinuxLvm2VolumeGroup *vg = GDU_LINUX_LVM2_VOLUME_GROUP (drive);
        /* TODO: actually compute out_degraded and also return FALSE in case we can't even start in degraded mode */
        if (out_degraded != NULL)
                *out_degraded = FALSE;
        return gdu_linux_lvm2_volume_group_get_state (vg) != GDU_LINUX_LVM2_VOLUME_GROUP_STATE_RUNNING;
}

static gboolean
gdu_linux_lvm2_volume_group_can_create_volume (GduDrive        *drive,
                                               gboolean        *out_is_uninitialized,
                                               guint64         *out_largest_contiguous_free_segment,
                                               guint64         *out_total_free,
                                               GduPresentable **out_presentable)
{
        GList *enclosed_presentables;
        GList *l;
        guint64 largest_contiguous_free_segment;
        guint64 total_free;
        GduPresentable *pres;
        gboolean ret;
        GduPool *pool;

        largest_contiguous_free_segment = 0;
        total_free = 0;
        pres = NULL;
        ret = FALSE;

        pool = gdu_presentable_get_pool (GDU_PRESENTABLE (drive));

        enclosed_presentables = gdu_pool_get_enclosed_presentables (pool, GDU_PRESENTABLE (drive));
        for (l = enclosed_presentables; l != NULL; l = l->next) {
                GduPresentable *ep = GDU_PRESENTABLE (l->data);

                if (GDU_IS_VOLUME_HOLE (ep)) {
                        guint64 size;

                        size = gdu_presentable_get_size (ep);

                        if (size > largest_contiguous_free_segment) {
                                largest_contiguous_free_segment = size;
                                pres = ep;
                        }

                        total_free += size;

                }
        }
        g_list_foreach (enclosed_presentables, (GFunc) g_object_unref, NULL);
        g_list_free (enclosed_presentables);

        if (out_largest_contiguous_free_segment != NULL)
                *out_largest_contiguous_free_segment = largest_contiguous_free_segment;

        if (out_total_free != NULL)
                *out_total_free = total_free;

        if (out_is_uninitialized != NULL)
                *out_is_uninitialized = FALSE;

        if (out_presentable != NULL) {
                *out_presentable = (pres != NULL ? g_object_ref (pres) : NULL);
        }

        ret = (largest_contiguous_free_segment > 0);

        g_object_unref (pool);

        return ret;
}

static void
lvm2_lv_create_op_callback (GduPool    *pool,
                            gchar      *created_device_object_path,
                            GError     *error,
                            gpointer    user_data)
{
        GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (user_data);

        if (error != NULL) {
                g_simple_async_result_set_from_error (simple, error);
                g_error_free (error);
        } else {
                GduDevice *d;
                GduPresentable *volume;

                d = gdu_pool_get_by_object_path (pool, created_device_object_path);
                g_assert (d != NULL);

                volume = gdu_pool_get_volume_by_device (pool, d);
                g_assert (volume != NULL);

                g_simple_async_result_set_op_res_gpointer (simple, volume, g_object_unref);

                g_object_unref (d);
        }
        g_simple_async_result_complete_in_idle (simple);
}

static void
gdu_linux_lvm2_volume_group_create_volume (GduDrive              *drive,
                                           guint64                size,
                                           const gchar           *name,
                                           GduCreateVolumeFlags   flags,
                                           GAsyncReadyCallback    callback,
                                           gpointer               user_data)
{
        GduLinuxLvm2VolumeGroup *vg = GDU_LINUX_LVM2_VOLUME_GROUP (drive);
        GSimpleAsyncResult *simple;
        gchar *volume_name;

        g_return_if_fail (GDU_IS_DRIVE (drive));

        simple = g_simple_async_result_new (G_OBJECT (drive),
                                            callback,
                                            user_data,
                                            gdu_linux_lvm2_volume_group_create_volume);

        /* For now, just use a generic LV name - TODO: include RAID/LVM etc */
        volume_name = gdu_linux_lvm2_volume_group_get_compute_new_lv_name (vg);

        gdu_pool_op_linux_lvm2_lv_create (vg->priv->pool,
                                          vg->priv->uuid,
                                          volume_name,
                                          size,
                                          0, /* num_stripes */
                                          0, /* stripe_size */
                                          0, /* num_mirrors */
                                          "", /* fs_type */
                                          "", /* fs_label */
                                          "", /* encrypt_passphrase */
                                          FALSE, /* fs_take_ownership */
                                          lvm2_lv_create_op_callback,
                                          simple);
}

static GduVolume *
gdu_linux_lvm2_volume_group_create_volume_finish (GduDrive              *drive,
                                                  GAsyncResult          *res,
                                                  GError               **error)
{
        GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (res);
        GduVolume *ret;

        g_return_val_if_fail (GDU_IS_DRIVE (drive), NULL);
        g_return_val_if_fail (res != NULL, NULL);

        g_warn_if_fail (g_simple_async_result_get_source_tag (simple) == gdu_linux_lvm2_volume_group_create_volume);

        ret = NULL;
        if (g_simple_async_result_propagate_error (simple, error))
                goto out;

        ret = GDU_VOLUME (g_simple_async_result_get_op_res_gpointer (simple));

 out:
        return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * gdu_linux_lvm2_volume_group_get_lv_device:
 * @vg: A #GduLinuxLvm2VolumeGroup.
 *
 * Returns a #GduDevice for a PV in @vg with the most up to date volume group info.
 *
 * Returns: A #GduDevice (free with g_object_unref()) or %NULL if not found.
 */
GduDevice *
gdu_linux_lvm2_volume_group_get_pv_device (GduLinuxLvm2VolumeGroup *vg)
{
        if (vg->priv->pv != NULL) {
                return g_object_ref (vg->priv->pv);
        } else {
                return NULL;
        }
}

GduLinuxLvm2VolumeGroupState
gdu_linux_lvm2_volume_group_get_state (GduLinuxLvm2VolumeGroup *vg)
{
        GList *lvs;
        GList *l;
        guint num_lvs;
        guint num_running_lvs;
        GduLinuxLvm2VolumeGroupState ret;

        lvs = gdu_pool_get_enclosed_presentables (vg->priv->pool, GDU_PRESENTABLE (vg));
        num_lvs = 0;
        num_running_lvs = 0;
        for (l = lvs; l != NULL; l = l->next) {
                GduPresentable *p = GDU_PRESENTABLE (l->data);

                if (GDU_IS_LINUX_LVM2_VOLUME (p)) {
                        GduDevice *d;

                        d = gdu_presentable_get_device (p);
                        if (d != NULL) {
                                num_running_lvs++;
                                g_object_unref (d);
                        }
                        num_lvs++;
                }
        }
        g_list_foreach (lvs, (GFunc) g_object_unref, NULL);
        g_list_free (lvs);

        if (num_running_lvs == 0)
                ret = GDU_LINUX_LVM2_VOLUME_GROUP_STATE_NOT_RUNNING;
        else if (num_running_lvs == num_lvs)
                ret = GDU_LINUX_LVM2_VOLUME_GROUP_STATE_RUNNING;
        else
                ret = GDU_LINUX_LVM2_VOLUME_GROUP_STATE_PARTIALLY_RUNNING;

        return ret;
}

const gchar *
gdu_linux_lvm2_volume_group_get_uuid (GduLinuxLvm2VolumeGroup *vg)
{
        return vg->priv->uuid;
}

guint
gdu_linux_lvm2_volume_group_get_num_lvs (GduLinuxLvm2VolumeGroup *vg)
{
        guint ret;
        gchar **lvs;

        ret = 0;

        if (vg->priv->pv == NULL)
                goto out;

        lvs = gdu_device_linux_lvm2_pv_get_group_logical_volumes (vg->priv->pv);

        if (lvs != NULL)
                ret = g_strv_length (lvs);

 out:
        return ret;
}

gboolean
gdu_linux_lvm2_volume_group_get_lv_info (GduLinuxLvm2VolumeGroup  *vg,
                                         const gchar              *lv_uuid,
                                         guint                    *out_position,
                                         gchar                   **out_name,
                                         guint64                  *out_size)
{
        gchar **lvs;
        gboolean ret;
        guint position;
        gchar *name;
        guint64 size;
        guint n;

        position = G_MAXUINT;
        name = NULL;
        size = G_MAXUINT64;
        ret = FALSE;

        if (vg->priv->pv == NULL)
                goto out;

        lvs = gdu_device_linux_lvm2_pv_get_group_logical_volumes (vg->priv->pv);

        for (n = 0; lvs != NULL && lvs[n] != NULL; n++) {
                gchar **tokens;
                guint m;

                tokens = g_strsplit (lvs[n], ";", 0);

                for (m = 0; tokens[m] != NULL; m++) {
                        /* TODO: we need to unescape values */
                        if (g_str_has_prefix (tokens[m], "uuid=") && g_strcmp0 (tokens[m] + 5, lv_uuid) == 0) {
                                guint p;

                                for (p = 0; tokens[p] != NULL; p++) {
                                        /* TODO: we need to unescape values */
                                        if (g_str_has_prefix (tokens[p], "name="))
                                                name = g_strdup (tokens[p] + 5);
                                        else if (g_str_has_prefix (tokens[p], "size="))
                                                size = g_ascii_strtoull (tokens[p] + 5, NULL, 10);
                                }
                                position = n;

                                g_strfreev (tokens);
                                ret = TRUE;
                                goto out;
                        }
                }
                g_strfreev (tokens);
        }

 out:
        if (ret) {
                if (out_position != NULL)
                        *out_position = position;
                if (out_name != NULL)
                        *out_name = name;
                else
                        g_free (name);
                if (out_size != NULL)
                        *out_size = size;
        }
        return ret;
}

gboolean
gdu_linux_lvm2_volume_group_get_pv_info (GduLinuxLvm2VolumeGroup  *vg,
                                         const gchar              *pv_uuid,
                                         guint                    *out_position,
                                         guint64                  *out_size,
                                         guint64                  *out_allocated_size)
{
        gchar **pvs;
        gboolean ret;
        guint position;
        guint64 size;
        guint64 allocated_size;
        guint n;

        position = G_MAXUINT;
        size = G_MAXUINT64;
        allocated_size = G_MAXUINT64;
        ret = FALSE;

        if (vg->priv->pv == NULL)
                goto out;

        pvs = gdu_device_linux_lvm2_pv_get_group_physical_volumes (vg->priv->pv);

        for (n = 0; pvs != NULL && pvs[n] != NULL; n++) {
                gchar **tokens;
                guint m;

                tokens = g_strsplit (pvs[n], ";", 0);

                for (m = 0; tokens[m] != NULL; m++) {

                        /* TODO: we need to unescape values */
                        if (g_str_has_prefix (tokens[m], "uuid=") && g_strcmp0 (tokens[m] + 5, pv_uuid) == 0) {
                                guint p;

                                for (p = 0; tokens[p] != NULL; p++) {
                                        /* TODO: we need to unescape values */
                                        if (g_str_has_prefix (tokens[p], "size="))
                                                size = g_ascii_strtoull (tokens[p] + 5, NULL, 10);
                                        else if (g_str_has_prefix (tokens[p], "allocated_size="))
                                                allocated_size = g_ascii_strtoull (tokens[p] + 15, NULL, 10);
                                }
                                position = n;

                                g_strfreev (tokens);
                                ret = TRUE;
                                goto out;
                        }
                }
                g_strfreev (tokens);
        }

 out:
        if (ret) {
                if (out_position != NULL)
                        *out_position = position;
                if (out_size != NULL)
                        *out_size = size;
                if (out_allocated_size != NULL)
                        *out_allocated_size = allocated_size;
        }
        return ret;
}

static gboolean
lv_name_exists (GduLinuxLvm2VolumeGroup *vg, const gchar *name)
{
        gboolean ret;
        gchar **lvs;
        guint n;

        ret = FALSE;

        if (vg->priv->pv == NULL)
                goto out;

        lvs = gdu_device_linux_lvm2_pv_get_group_logical_volumes (vg->priv->pv);
        for (n = 0; lvs != NULL && lvs[n] != NULL; n++) {
                gchar **tokens;
                guint m;

                tokens = g_strsplit (lvs[n], ";", 0);

                for (m = 0; tokens[m] != NULL; m++) {
                        /* TODO: we need to unescape values */
                        if (g_str_has_prefix (tokens[m], "name=") && g_strcmp0 (tokens[m] + 5, name) == 0) {
                                g_strfreev (tokens);
                                ret = TRUE;
                                goto out;
                        }
                }
                g_strfreev (tokens);
        }

 out:
        return ret;
}

/* Computes the next available Logical Volume name */
gchar *
gdu_linux_lvm2_volume_group_get_compute_new_lv_name (GduLinuxLvm2VolumeGroup *vg)
{
        GString *s;

        s = g_string_new (NULL);
        g_string_append_printf (s, "Logical_Volume_%02d", gdu_linux_lvm2_volume_group_get_num_lvs (vg));

        while (lv_name_exists (vg, s->str))
                g_string_append_c (s, '_');

        return g_string_free (s, FALSE);
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct
{
        GduDrive *drive;
        GduDriveActivateFunc callback;
        gpointer user_data;
} VGStartData;

static void
vg_start_cb (GduPool    *pool,
             GError     *error,
             gpointer    user_data)
{
        VGStartData *data = user_data;

        data->callback (data->drive, NULL, error, data->user_data);
        g_object_unref (data->drive);
        g_free (data);
}

static void
gdu_linux_lvm2_volume_group_activate (GduDrive              *drive,
                                      GduDriveActivateFunc   callback,
                                      gpointer               user_data)
{
        GduLinuxLvm2VolumeGroup *vg = GDU_LINUX_LVM2_VOLUME_GROUP (drive);
        VGStartData *data;

        data = g_new0 (VGStartData, 1);
        data->drive = g_object_ref (drive);
        data->callback = callback;
        data->user_data = user_data;

        gdu_pool_op_linux_lvm2_vg_start (vg->priv->pool,
                                         vg->priv->uuid,
                                         vg_start_cb,
                                         data);
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct
{
        GduDrive *drive;
        GduDriveDeactivateFunc callback;
        gpointer user_data;
} VGStopData;

static void
vg_stop_cb (GduPool    *pool,
            GError     *error,
            gpointer    user_data)
{
        VGStopData *data = user_data;

        data->callback (data->drive, error, data->user_data);
        g_object_unref (data->drive);
        g_free (data);
}

static void
gdu_linux_lvm2_volume_group_deactivate (GduDrive                *drive,
                                        GduDriveDeactivateFunc   callback,
                                        gpointer                 user_data)
{
        GduLinuxLvm2VolumeGroup *vg = GDU_LINUX_LVM2_VOLUME_GROUP (drive);
        VGStopData *data;

        data = g_new0 (VGStopData, 1);
        data->drive = g_object_ref (drive);
        data->callback = callback;
        data->user_data = user_data;

        gdu_pool_op_linux_lvm2_vg_stop (vg->priv->pool,
                                        vg->priv->uuid,
                                        vg_stop_cb,
                                        data);
}

/* ---------------------------------------------------------------------------------------------------- */
