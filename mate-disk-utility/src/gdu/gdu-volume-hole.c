/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-volume-hole.c
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
#include "gdu-device.h"
#include "gdu-volume-hole.h"
#include "gdu-presentable.h"
#include "gdu-linux-md-drive.h"

/**
 * SECTION:gdu-volume-hole
 * @title: GduVolumeHole
 * @short_description: Unallocated regions of partitioned devices
 *
 * Instances of the #GduVolumeHole class is used to represent the
 * unallocated bits on a partitioned device.
 *
 * See the documentation for #GduPresentable for the big picture.
 */

struct _GduVolumeHolePrivate
{
        guint64 offset;
        guint64 size;
        GduPresentable *enclosing_presentable;
        GduPool *pool;
        gchar *id;
};

static GObjectClass *parent_class = NULL;

static void gdu_volume_hole_presentable_iface_init (GduPresentableIface *iface);
G_DEFINE_TYPE_WITH_CODE (GduVolumeHole, gdu_volume_hole, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GDU_TYPE_PRESENTABLE,
                                                gdu_volume_hole_presentable_iface_init))

static void
gdu_volume_hole_finalize (GduVolumeHole *volume_hole)
{
        //g_debug ("finalized volume_hole '%s' %p", volume_hole->priv->id, volume_hole);

        if (volume_hole->priv->pool != NULL)
                g_object_unref (volume_hole->priv->pool);

        if (volume_hole->priv->enclosing_presentable != NULL)
                g_object_unref (volume_hole->priv->enclosing_presentable);

        g_free (volume_hole->priv->id);

        if (G_OBJECT_CLASS (parent_class)->finalize)
                (* G_OBJECT_CLASS (parent_class)->finalize) (G_OBJECT (volume_hole));
}

static void
gdu_volume_hole_class_init (GduVolumeHoleClass *klass)
{
        GObjectClass *obj_class = (GObjectClass *) klass;

        parent_class = g_type_class_peek_parent (klass);

        obj_class->finalize = (GObjectFinalizeFunc) gdu_volume_hole_finalize;

        g_type_class_add_private (klass, sizeof (GduVolumeHolePrivate));
}

static void
gdu_volume_hole_init (GduVolumeHole *volume_hole)
{
        volume_hole->priv = G_TYPE_INSTANCE_GET_PRIVATE (volume_hole, GDU_TYPE_VOLUME_HOLE, GduVolumeHolePrivate);
}

GduVolumeHole *
_gdu_volume_hole_new (GduPool *pool, guint64 offset, guint64 size, GduPresentable *enclosing_presentable)
{
        GduVolumeHole *volume_hole;

        volume_hole = GDU_VOLUME_HOLE (g_object_new (GDU_TYPE_VOLUME_HOLE, NULL));
        volume_hole->priv->pool = g_object_ref (pool);
        volume_hole->priv->offset = offset;
        volume_hole->priv->size = size;
        volume_hole->priv->enclosing_presentable =
                enclosing_presentable != NULL ? g_object_ref (enclosing_presentable) : NULL;

        volume_hole->priv->id = g_strdup_printf ("volume_hole_%s_%" G_GUINT64_FORMAT "_%" G_GUINT64_FORMAT,
                                                 enclosing_presentable != NULL ? gdu_presentable_get_id (enclosing_presentable) : "(none)",
                                                 offset,
                                                 size);

        return volume_hole;
}

static const gchar *
gdu_volume_hole_get_id (GduPresentable *presentable)
{
        GduVolumeHole *volume_hole = GDU_VOLUME_HOLE (presentable);
        return volume_hole->priv->id;
}

static GduDevice *
gdu_volume_hole_get_device (GduPresentable *presentable)
{
        return NULL;
}

static GduPresentable *
gdu_volume_hole_get_enclosing_presentable (GduPresentable *presentable)
{
        GduVolumeHole *volume_hole = GDU_VOLUME_HOLE (presentable);
        if (volume_hole->priv->enclosing_presentable != NULL)
                return g_object_ref (volume_hole->priv->enclosing_presentable);
        return NULL;
}

static char *
gdu_volume_hole_get_name (GduPresentable *presentable)
{
        GduVolumeHole *volume_hole = GDU_VOLUME_HOLE (presentable);
        char *result;
        char *strsize;

        strsize = gdu_util_get_size_for_display (volume_hole->priv->size, FALSE, FALSE);
        /* Translators: label for an unallocated space on a disk
         * %s is the size, formatted like '45 GB'
         */
        result = g_strdup_printf (_("%s Free"), strsize);
        g_free (strsize);

        return result;
}

static gchar *
gdu_volume_hole_get_description (GduPresentable *presentable)
{
        return g_strdup (_("Unallocated Space"));
}

static char *
gdu_volume_hole_get_vpd_name (GduPresentable *presentable)
{
        /* TODO: we might want to include more information in the future - such as
         *
         *       - Offset at where the hole is (at offset '45 GB')
         *       - What partitions are adjacent (between partitions 3 and 4)
         *
         */
        return g_strdup ("");
}

static GIcon *
gdu_volume_hole_get_icon (GduPresentable *presentable)
{
        GduPresentable *p;
        GduDevice *d;
        const char *connection_interface;
        const char *name;
        const char *drive_media;
        gboolean is_removable;

        p = NULL;
        d = NULL;
        name = NULL;
        is_removable = FALSE;

        p = gdu_presentable_get_enclosing_presentable (presentable);
        if (p == NULL)
                goto out;

        d = gdu_presentable_get_device (p);
        if (d == NULL)
                goto out;

        if (!gdu_device_is_drive (d))
                goto out;

        connection_interface = gdu_device_drive_get_connection_interface (d);
        if (connection_interface == NULL)
                goto out;

        is_removable = gdu_device_is_removable (d);

        drive_media = gdu_device_drive_get_media (d);

        /* Linux MD devices can be partitioned */
        if (GDU_IS_LINUX_MD_DRIVE (p)) {
                name = "gdu-multidisk-drive";
        }

        /* first try the media */
        if (name == NULL && drive_media != NULL) {
                if (strcmp (drive_media, "flash_cf") == 0) {
                        name = "media-flash-cf";
                } else if (strcmp (drive_media, "flash_ms") == 0) {
                        name = "media-flash-ms";
                } else if (strcmp (drive_media, "flash_sm") == 0) {
                        name = "media-flash-sm";
                } else if (strcmp (drive_media, "flash_sd") == 0) {
                        name = "media-flash-sd";
                } else if (strcmp (drive_media, "flash_sdhc") == 0) {
                        /* TODO: get icon name for sdhc */
                        name = "media-flash-sd";
                } else if (strcmp (drive_media, "flash_mmc") == 0) {
                        /* TODO: get icon for mmc */
                        name = "media-flash-sd";
                } else if (g_str_has_prefix (drive_media, "flash")) {
                        name = "media-flash";
                } else if (g_str_has_prefix (drive_media, "optical")) {
                        /* TODO: handle rest of optical-* */
                        name = "media-optical";
                }
        }

        /* else fall back to connection interface */
        if (name == NULL && connection_interface != NULL) {
                if (g_str_has_prefix (connection_interface, "ata")) {
                        if (is_removable)
                                name = "drive-removable-media-ata";
                        else
                                name = "drive-harddisk-ata";
                } else if (g_str_has_prefix (connection_interface, "scsi")) {
                        if (is_removable)
                                name = "drive-removable-media-scsi";
                        else
                                name = "drive-harddisk-scsi";
                } else if (strcmp (connection_interface, "usb") == 0) {
                        if (is_removable)
                                name = "drive-removable-media-usb";
                        else
                                name = "drive-harddisk-usb";
                } else if (strcmp (connection_interface, "firewire") == 0) {
                        if (is_removable)
                                name = "drive-removable-media-ieee1394";
                        else
                                name = "drive-harddisk-ieee1394";
                }
        }

out:
        if (p != NULL)
                g_object_unref (p);
        if (d != NULL)
                g_object_unref (d);

        /* ultimate fallback */
        if (name == NULL) {
                if (is_removable)
                        name = "drive-removable-media";
                else
                        name = "drive-harddisk";
        }

        return g_themed_icon_new_with_default_fallbacks (name);
}

static guint64
gdu_volume_hole_get_offset (GduPresentable *presentable)
{
        GduVolumeHole *volume_hole = GDU_VOLUME_HOLE (presentable);
        return volume_hole->priv->offset;
}

static guint64
gdu_volume_hole_get_size (GduPresentable *presentable)
{
        GduVolumeHole *volume_hole = GDU_VOLUME_HOLE (presentable);
        return volume_hole->priv->size;
}

static GduPool *
gdu_volume_hole_get_pool (GduPresentable *presentable)
{
        GduVolumeHole *volume_hole = GDU_VOLUME_HOLE (presentable);
        return g_object_ref (volume_hole->priv->pool);
}

static gboolean
gdu_volume_hole_is_allocated (GduPresentable *presentable)
{
        return FALSE;
}

static gboolean
gdu_volume_hole_is_recognized (GduPresentable *presentable)
{
        return FALSE;
}

static void
gdu_volume_hole_presentable_iface_init (GduPresentableIface *iface)
{
        iface->get_id                    = gdu_volume_hole_get_id;
        iface->get_device                = gdu_volume_hole_get_device;
        iface->get_enclosing_presentable = gdu_volume_hole_get_enclosing_presentable;
        iface->get_name                  = gdu_volume_hole_get_name;
        iface->get_description           = gdu_volume_hole_get_description;
        iface->get_vpd_name              = gdu_volume_hole_get_vpd_name;
        iface->get_icon                  = gdu_volume_hole_get_icon;
        iface->get_offset                = gdu_volume_hole_get_offset;
        iface->get_size                  = gdu_volume_hole_get_size;
        iface->get_pool                  = gdu_volume_hole_get_pool;
        iface->is_allocated              = gdu_volume_hole_is_allocated;
        iface->is_recognized             = gdu_volume_hole_is_recognized;
}

void
_gdu_volume_hole_rewrite_enclosing_presentable (GduVolumeHole *volume_hole)
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
