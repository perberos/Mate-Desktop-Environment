/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-linux-md-drive.c
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

#include "gdu-private.h"
#include "gdu-util.h"
#include "gdu-pool.h"
#include "gdu-device.h"
#include "gdu-linux-md-drive.h"
#include "gdu-presentable.h"

/**
 * SECTION:gdu-linux-md-drive
 * @title: GduLinuxMdDrive
 * @short_description: Linux Software RAID drives
 *
 * The #GduLinuxMdDrive class represents drives Linux Software RAID arrays.
 *
 * An #GduLinuxMdDrive drive is added to #GduPool as soon as a
 * component device that is part of the abstraction is available.  The
 * drive can be started (gdu_drive_start()) and stopped
 * (gdu_drive_stop()) and the state of the underlying components can
 * be queried through gdu_linux_md_drive_get_slave_flags()).
 *
 * See the documentation for #GduPresentable for the big picture.
 */

struct _GduLinuxMdDrivePrivate
{
        /* device may be NULL */
        GduDevice *device;

        GList *slaves;

        GduPool *pool;

        gchar *uuid;

        gchar *device_file;

        gchar *id;

        GduPresentable *enclosing_presentable;
};

static GObjectClass *parent_class = NULL;

static void gdu_linux_md_drive_presentable_iface_init (GduPresentableIface *iface);

G_DEFINE_TYPE_WITH_CODE (GduLinuxMdDrive, gdu_linux_md_drive, GDU_TYPE_DRIVE,
                         G_IMPLEMENT_INTERFACE (GDU_TYPE_PRESENTABLE,
                                                gdu_linux_md_drive_presentable_iface_init))

static void device_added (GduPool *pool, GduDevice *device, gpointer user_data);
static void device_removed (GduPool *pool, GduDevice *device, gpointer user_data);
static void device_changed (GduPool *pool, GduDevice *device, gpointer user_data);
static void device_job_changed (GduPool *pool, GduDevice *device, gpointer user_data);

static gboolean    gdu_linux_md_drive_is_active             (GduDrive               *drive);
static gboolean    gdu_linux_md_drive_is_activatable        (GduDrive               *drive);
static gboolean    gdu_linux_md_drive_can_deactivate        (GduDrive               *drive);
static gboolean    gdu_linux_md_drive_can_activate          (GduDrive               *drive,
                                                             gboolean               *out_degraded);
static void        gdu_linux_md_drive_activate              (GduDrive               *drive,
                                                             GduDriveActivateFunc    callback,
                                                             gpointer                user_data);
static void        gdu_linux_md_drive_deactivate            (GduDrive               *drive,
                                                             GduDriveDeactivateFunc  callback,
                                                             gpointer                user_data);


static void
gdu_linux_md_drive_finalize (GObject *object)
{
        GduLinuxMdDrive *drive = GDU_LINUX_MD_DRIVE (object);

        //g_debug ("##### finalized linux-md drive '%s' %p", drive->priv->id, drive);

        if (drive->priv->uuid != NULL) {
                g_signal_handlers_disconnect_by_func (drive->priv->pool, device_added, drive);
                g_signal_handlers_disconnect_by_func (drive->priv->pool, device_removed, drive);
                g_signal_handlers_disconnect_by_func (drive->priv->pool, device_changed, drive);
                g_signal_handlers_disconnect_by_func (drive->priv->pool, device_job_changed, drive);
        }

        if (drive->priv->pool != NULL)
                g_object_unref (drive->priv->pool);

        if (drive->priv->device != NULL) {
                g_object_unref (drive->priv->device);
        }

        g_list_foreach (drive->priv->slaves, (GFunc) g_object_unref, NULL);
        g_list_free (drive->priv->slaves);

        g_free (drive->priv->id);

        g_free (drive->priv->uuid);
        g_free (drive->priv->device_file);

        if (drive->priv->enclosing_presentable != NULL)
                g_object_unref (drive->priv->enclosing_presentable);

        if (G_OBJECT_CLASS (parent_class)->finalize)
                (* G_OBJECT_CLASS (parent_class)->finalize) (G_OBJECT (drive));
}

static void
gdu_linux_md_drive_class_init (GduLinuxMdDriveClass *klass)
{
        GObjectClass *gobject_class = (GObjectClass *) klass;
        GduDriveClass *drive_class = (GduDriveClass *) klass;

        parent_class = g_type_class_peek_parent (klass);

        gobject_class->finalize = gdu_linux_md_drive_finalize;

        g_type_class_add_private (klass, sizeof (GduLinuxMdDrivePrivate));

        drive_class->is_active             = gdu_linux_md_drive_is_active;
        drive_class->is_activatable        = gdu_linux_md_drive_is_activatable;
        drive_class->can_deactivate        = gdu_linux_md_drive_can_deactivate;
        drive_class->can_activate          = gdu_linux_md_drive_can_activate;
        drive_class->activate              = gdu_linux_md_drive_activate;
        drive_class->deactivate            = gdu_linux_md_drive_deactivate;
}

static void
gdu_linux_md_drive_init (GduLinuxMdDrive *drive)
{
        drive->priv = G_TYPE_INSTANCE_GET_PRIVATE (drive, GDU_TYPE_LINUX_MD_DRIVE, GduLinuxMdDrivePrivate);
}

static void
prime_devices (GduLinuxMdDrive *drive)
{
        GList *l;
        GList *devices;

        devices = gdu_pool_get_devices (drive->priv->pool);

        for (l = devices; l != NULL; l = l->next) {
                GduDevice *device = GDU_DEVICE (l->data);

                if (gdu_device_is_linux_md (device) &&
                    g_strcmp0 (gdu_device_linux_md_get_uuid (device), drive->priv->uuid) == 0) {
                        drive->priv->device = g_object_ref (device);
                }

                if (gdu_device_is_linux_md_component (device) &&
                    g_strcmp0 (gdu_device_linux_md_component_get_uuid (device), drive->priv->uuid) == 0 &&
                    !gdu_device_should_ignore (device)) {
                        drive->priv->slaves = g_list_prepend (drive->priv->slaves, g_object_ref (device));
                }

        }

        g_list_foreach (devices, (GFunc) g_object_unref, NULL);
        g_list_free (devices);
}

static void
emit_changed (GduLinuxMdDrive *drive)
{
        //g_debug ("emitting changed for uuid '%s'", drive->priv->uuid);
        g_signal_emit_by_name (drive, "changed");
        g_signal_emit_by_name (drive->priv->pool, "presentable-changed", drive);
}

static void
emit_job_changed (GduLinuxMdDrive *drive)
{
        //g_debug ("emitting job-changed for uuid '%s'", drive->priv->uuid);
        g_signal_emit_by_name (drive, "job-changed");
        g_signal_emit_by_name (drive->priv->pool, "presentable-job-changed", drive);
}

static void
device_added (GduPool *pool, GduDevice *device, gpointer user_data)
{
        GduLinuxMdDrive *drive = GDU_LINUX_MD_DRIVE (user_data);

        //g_debug ("MD: in device_added %s '%s'", gdu_device_get_object_path (device), gdu_device_linux_md_get_uuid (device));

        if (gdu_device_is_linux_md (device) &&
            g_strcmp0 (gdu_device_linux_md_get_uuid (device), drive->priv->uuid) == 0) {
                if (drive->priv->device != NULL) {
                        g_warning ("Already have md device %s", gdu_device_get_device_file (device));
                        g_object_unref (drive->priv->device);
                }
                drive->priv->device = g_object_ref (device);
                emit_changed (drive);
        }

        if (gdu_device_is_linux_md_component (device) &&
            g_strcmp0 (gdu_device_linux_md_component_get_uuid (device), drive->priv->uuid) == 0 &&
            !gdu_device_should_ignore (device)) {
                GList *l;

                for (l = drive->priv->slaves; l != NULL; l = l->next) {
                        if (g_strcmp0 (gdu_device_get_device_file (GDU_DEVICE (l->data)),
                                       gdu_device_get_device_file (device)) == 0) {
                                g_warning ("Already have md slave %s", gdu_device_get_device_file (device));
                                g_object_unref (GDU_DEVICE (l->data));
                                drive->priv->slaves = g_list_delete_link (drive->priv->slaves, l);
                        }
                }

                drive->priv->slaves = g_list_prepend (drive->priv->slaves, g_object_ref (device));
                emit_changed (drive);
        }
}

static void
device_removed (GduPool *pool, GduDevice *device, gpointer user_data)
{
        GduLinuxMdDrive *drive = GDU_LINUX_MD_DRIVE (user_data);

        //g_debug ("MD: in device_removed %s", gdu_device_get_object_path (device));

        if (device == drive->priv->device) {
                g_object_unref (device);
                drive->priv->device = NULL;
                emit_changed (drive);
        }

        if (g_list_find (drive->priv->slaves, device) != NULL) {
                g_object_unref (device);
                drive->priv->slaves = g_list_remove (drive->priv->slaves, device);
                emit_changed (drive);
        }
}

static void
device_changed (GduPool *pool, GduDevice *device, gpointer user_data)
{
        GduLinuxMdDrive *drive = GDU_LINUX_MD_DRIVE (user_data);
        gboolean has_device;
        gboolean emit_signal;

        //g_debug ("MD: in device_changed %s", gdu_device_get_object_path (device));

        emit_signal = TRUE;

        has_device = g_list_find (drive->priv->slaves, device) != NULL;

        if (has_device) {
                /* handle when device is removed from the array */
                emit_signal = TRUE;
                if (!gdu_device_is_linux_md_component (device) ||
                    g_strcmp0 (gdu_device_linux_md_component_get_uuid (device), drive->priv->uuid) != 0) {
                        drive->priv->slaves = g_list_remove (drive->priv->slaves, device);
                        g_object_unref (device);
                }
        } else {
                /* handle when device is joining the array */
                if (gdu_device_is_linux_md_component (device) &&
                    g_strcmp0 (gdu_device_linux_md_component_get_uuid (device), drive->priv->uuid) == 0 &&
                    !gdu_device_should_ignore (device)) {
                        emit_signal = TRUE;
                        drive->priv->slaves = g_list_prepend (drive->priv->slaves, g_object_ref (device));
                }
        }



        if (device == drive->priv->device || emit_signal) {
                emit_changed (drive);
        }
}

static void
device_job_changed (GduPool *pool, GduDevice *device, gpointer user_data)
{
        GduLinuxMdDrive *drive = GDU_LINUX_MD_DRIVE (user_data);
        gboolean has_device;
        gboolean emit_signal;

        //g_debug ("MD: in device_job_changed %s", gdu_device_get_object_path (device));

        emit_signal = TRUE;

        has_device = g_list_find (drive->priv->slaves, device) != NULL;

        if (has_device) {
                /* handle when device is removed from the array */
                emit_signal = TRUE;
                if (!gdu_device_is_linux_md_component (device) ||
                    g_strcmp0 (gdu_device_linux_md_component_get_uuid (device), drive->priv->uuid) != 0) {
                        drive->priv->slaves = g_list_remove (drive->priv->slaves, device);
                        g_object_unref (device);
                }
        } else {
                /* handle when device is joining the array */
                if (gdu_device_is_linux_md_component (device) &&
                    g_strcmp0 (gdu_device_linux_md_component_get_uuid (device), drive->priv->uuid) == 0 &&
                    !gdu_device_should_ignore (device)) {
                        emit_signal = TRUE;
                        drive->priv->slaves = g_list_prepend (drive->priv->slaves, g_object_ref (device));
                }
        }



        if (device == drive->priv->device || emit_signal) {
                emit_job_changed (drive);
        }
}

/**
 * _gdu_linux_md_drive_new:
 * @pool: A #GduPool.
 * @uuid: The UUID for the array.
 * @device_file: The device file for the array.
 * @enclosing_presentable: The enclosing presentable
 *
 * Creates a new #GduLinuxMdDrive. Note that only one of @uuid and
 * @device_file may be %NULL.
 */
GduLinuxMdDrive *
_gdu_linux_md_drive_new (GduPool      *pool,
                         const gchar  *uuid,
                         const gchar  *device_file,
                         GduPresentable *enclosing_presentable)
{
        GduLinuxMdDrive *drive;

        drive = GDU_LINUX_MD_DRIVE (g_object_new (GDU_TYPE_LINUX_MD_DRIVE, NULL));
        drive->priv->pool = g_object_ref (pool);
        drive->priv->uuid = g_strdup (uuid);
        drive->priv->device_file = g_strdup (device_file);

        if (uuid != NULL) {
                drive->priv->id = g_strdup_printf ("linux_md_%s", uuid);
                g_signal_connect (drive->priv->pool, "device-added", G_CALLBACK (device_added), drive);
                g_signal_connect (drive->priv->pool, "device-removed", G_CALLBACK (device_removed), drive);
                g_signal_connect (drive->priv->pool, "device-changed", G_CALLBACK (device_changed), drive);
                g_signal_connect (drive->priv->pool, "device-job-changed", G_CALLBACK (device_job_changed), drive);
                prime_devices (drive);
        } else {
                drive->priv->id = g_strdup_printf ("linux_md_%s_enclosed_by_%s",
                                                   device_file,
                                                   enclosing_presentable != NULL ? gdu_presentable_get_id (enclosing_presentable) : "(none)");
                drive->priv->device = gdu_pool_get_by_device_file (pool, device_file);
        }

        drive->priv->enclosing_presentable =
                enclosing_presentable != NULL ? g_object_ref (enclosing_presentable) : NULL;

        return drive;
}

gboolean
_gdu_linux_md_drive_has_uuid (GduLinuxMdDrive  *drive,
                              const gchar      *uuid)
{
        return g_strcmp0 (drive->priv->uuid, uuid) == 0;
}

/**
 * gdu_linux_md_drive_has_slave:
 * @drive: A #GduLinuxMdDrive.
 * @device: A #GduDevice.
 *
 * Checks if @device is a component of @drive.
 *
 * Returns: #TRUE only if @slave is a component of @drive.
 **/
gboolean
gdu_linux_md_drive_has_slave    (GduLinuxMdDrive  *drive,
                                 GduDevice            *device)
{
        return g_list_find (drive->priv->slaves, device) != NULL;
}

const gchar *
gdu_linux_md_drive_get_uuid (GduLinuxMdDrive *drive)
{
        return drive->priv->uuid;
}

/**
 * gdu_linux_md_drive_get_slaves:
 * @drive: A #GduLinuxMdDrive.
 *
 * Gets all slaves of @drive.
 *
 * Returns: A #GList of #GduDevice objects. Caller must free this list (and call g_object_unref() on all elements).
 **/
GList *
gdu_linux_md_drive_get_slaves (GduLinuxMdDrive *drive)
{
        GList *ret;
        ret = g_list_copy (drive->priv->slaves);
        g_list_foreach (ret, (GFunc) g_object_ref, NULL);
        return ret;
}

/**
 * gdu_linux_md_drive_get_slave_flags:
 * @drive: A #GduLinuxMdDrive.
 * @slave: A #GduDevice.
 *
 * Gets the state of @slave of @drive.
 *
 * Returns: A value from #GduLinuxMdDriveSlaveState for @slave.
 **/
GduLinuxMdDriveSlaveFlags
gdu_linux_md_drive_get_slave_flags (GduLinuxMdDrive  *drive,
                                    GduDevice        *slave)
{
        gchar **state;
        const gchar *holder;
        GduLinuxMdDriveSlaveFlags flags;
        guint n;

        flags = 0;

        holder = gdu_device_linux_md_component_get_holder (slave);
        if (holder == NULL || g_strcmp0 (holder, "/") == 0)
                flags |= GDU_LINUX_MD_DRIVE_SLAVE_FLAGS_NOT_ATTACHED;

        state = gdu_device_linux_md_component_get_state (slave);
        for (n = 0; state != NULL && state[n] != NULL; n++) {
                if (g_strcmp0 (state[n], "faulty") == 0)
                        flags |= GDU_LINUX_MD_DRIVE_SLAVE_FLAGS_FAULTY;
                else if (g_strcmp0 (state[n], "in_sync") == 0)
                        flags |= GDU_LINUX_MD_DRIVE_SLAVE_FLAGS_IN_SYNC;
                else if (g_strcmp0 (state[n], "writemostly") == 0)
                        flags |= GDU_LINUX_MD_DRIVE_SLAVE_FLAGS_WRITEMOSTLY;
                else if (g_strcmp0 (state[n], "blocked") == 0)
                        flags |= GDU_LINUX_MD_DRIVE_SLAVE_FLAGS_BLOCKED;
                else if (g_strcmp0 (state[n], "spare") == 0)
                        flags |= GDU_LINUX_MD_DRIVE_SLAVE_FLAGS_SPARE;
                else
                        g_debug ("unknown linux md component state %s, please add support", state[n]);
        }

        return flags;
}

/* ---------------------------------------------------------------------------------------------------- */

/* GduPresentable methods */

static const gchar *
gdu_linux_md_drive_get_id (GduPresentable *presentable)
{
        GduLinuxMdDrive *drive = GDU_LINUX_MD_DRIVE (presentable);

        return drive->priv->id;
}

static GduDevice *
gdu_linux_md_drive_get_device (GduPresentable *presentable)
{
        GduLinuxMdDrive *drive = GDU_LINUX_MD_DRIVE (presentable);
        if (drive->priv->device == NULL)
                return NULL;
        else
                return g_object_ref (drive->priv->device);
}

static GduPresentable *
gdu_linux_md_drive_get_enclosing_presentable (GduPresentable *presentable)
{
        GduLinuxMdDrive *drive = GDU_LINUX_MD_DRIVE (presentable);
        if (drive->priv->enclosing_presentable != NULL)
                return g_object_ref (drive->priv->enclosing_presentable);
        return NULL;
}

static gchar *
get_names_and_desc (GduPresentable  *presentable,
                    gchar          **out_vpd_name,
                    gchar          **out_desc)
{
        GduLinuxMdDrive *drive = GDU_LINUX_MD_DRIVE (presentable);
        GduDevice *component_device;
        gchar *ret;
        gchar *ret_desc;
        gchar *ret_vpd;
        gchar *level_str;
        guint64 component_size;
        int num_slaves;
        int num_raid_devices;
        const char *level;
        const char *name;
        gchar *strsize;

        ret = NULL;
        ret_desc = NULL;
        ret_vpd = NULL;
        strsize = NULL;
        level_str = NULL;

        /* TODO: Maybe guess size from level, num_raid_devices and component_size? */
        if (drive->priv->device != NULL) {
                guint64 size;
                size = gdu_device_get_size (drive->priv->device);
                strsize = gdu_util_get_size_for_display (size, FALSE, FALSE);
        }

        if (drive->priv->slaves != NULL) {

                component_device = GDU_DEVICE (drive->priv->slaves->data);

                level = gdu_device_linux_md_component_get_level (component_device);
                name = gdu_device_linux_md_component_get_name (component_device);
                num_raid_devices = gdu_device_linux_md_component_get_num_raid_devices (component_device);
                num_slaves = g_list_length (drive->priv->slaves);
                component_size = gdu_device_get_size (component_device);

                level_str = gdu_linux_md_get_raid_level_for_display (level, FALSE);

                if (name == NULL || strlen (name) == 0) {
                        if (strsize != NULL) {
                                /* Translators: First %s is the size, second %s is a RAID level, e.g. 'RAID-5' */
                                ret = g_strdup_printf (_("%s %s Array"),
                                                       strsize,
                                                       level_str);
                        } else {
                                /* Translators: %s is a RAID level, e.g. 'RAID-5' */
                                ret = g_strdup_printf (_("%s Array"),
                                                       level_str);
                        }
                } else {
                        ret = g_strdup (name);
                        if (strsize != NULL) {
                                ret_desc = g_strdup_printf ("%s %s",
                                                            strsize,
                                                            level_str);
                        } else {
                                ret_desc = g_strdup (level_str);
                        }
                }

                if (drive->priv->device != NULL && gdu_device_is_partition_table (drive->priv->device)) {
                        const gchar *part_table_scheme;
                        const gchar *scheme_str;

                        part_table_scheme = gdu_device_partition_table_get_scheme (drive->priv->device);

                        if (g_strcmp0 (part_table_scheme, "mbr") == 0) {
                                /* Translators: This string is used for conveying the partition table format */
                                scheme_str = _("MBR Partition Table");
                        } else if (g_strcmp0 (part_table_scheme, "gpt") == 0) {
                                /* Translators: This string is used for conveying the partition table format */
                                scheme_str = _("GUID Partition Table");
                        } else if (g_strcmp0 (part_table_scheme, "apm") == 0) {
                                /* Translators: This string is used for conveying the partition table format */
                                scheme_str = _("Apple Partition Table");
                        } else {
                                /* Translators: This string is used for conveying the partition table format when
                                 * the format is unknown
                                 */
                                scheme_str = _("Partitioned");
                        }

                        if (ret_desc != NULL) {
                                gchar *tmp;
                                tmp = ret_desc;
                                ret_desc = g_strconcat (ret_desc,
                                                        ", ",
                                                        scheme_str,
                                                        NULL);
                                g_free (tmp);
                        } else {
                                ret_desc = g_strdup (scheme_str);
                        }
                }

        } else if (drive->priv->device != NULL) {
                /* Translators: First %s is a device file such as /dev/sda4
                 * second %s is the state of the device
                 */
                ret = g_strdup_printf (_("RAID Array %s (%s)"),
                                       gdu_device_get_device_file (drive->priv->device),
                                       gdu_device_linux_md_get_state (drive->priv->device));
        } else {
                /* Translators: %s is a device file such as /dev/sda4 */
                ret = g_strdup_printf (_("RAID device %s"),
                                       drive->priv->device_file != NULL ? drive->priv->device_file : "(unknown)");
        }

        /* Fallback for level_str */
        if (level_str == NULL) {
                /* Translators: fallback for level */
                level_str = g_strdup (C_("RAID Level fallback", "RAID"));
        }

        /* Fallback for description */
        if (ret_desc == NULL) {
                ret_desc = g_strdup (_("RAID Array"));
        }

        /* Fallback for VPD name */
        if (ret_vpd == NULL) {
                if (strsize != NULL) {
                        /* Translators: first %s is the size e.g. '45 GB', second %s is the level e.g. 'RAID-5' */
                        ret_vpd = g_strdup_printf (_("%s %s Array"),
                                                   strsize,
                                                   level_str);
                } else {
                        /* Translators: %s is the level e.g. 'RAID-5' */
                        ret_vpd = g_strdup_printf (_("%s Array"), level_str);
                }
        }

        if (out_desc != NULL)
                *out_desc = ret_desc;
        else
                g_free (ret_desc);

        if (out_vpd_name != NULL)
                *out_vpd_name = ret_vpd;
        else
                g_free (ret_vpd);

        g_free (strsize);
        g_free (level_str);

        return ret;
}

static char *
gdu_linux_md_drive_get_name (GduPresentable *presentable)
{
        return get_names_and_desc (presentable, NULL, NULL);
}

static gchar *
gdu_linux_md_drive_get_description (GduPresentable *presentable)
{
        gchar *desc;
        gchar *name;

        name = get_names_and_desc (presentable, NULL, &desc);
        g_free (name);

        return desc;
}

static gchar *
gdu_linux_md_drive_get_vpd_name (GduPresentable *presentable)
{
        gchar *vpd_name;
        gchar *name;

        name = get_names_and_desc (presentable, &vpd_name, NULL);
        g_free (name);

        return vpd_name;
}

static GIcon *
gdu_linux_md_drive_get_icon (GduPresentable *presentable)
{
        GduLinuxMdDrive *drive = GDU_LINUX_MD_DRIVE (presentable);
        const gchar *emblem_name;
        const gchar *level;

        level = NULL;
        if (drive->priv->slaves != NULL) {
                level = gdu_device_linux_md_component_get_level (GDU_DEVICE (drive->priv->slaves->data));
        } else if (drive->priv->device != NULL) {
                level = gdu_device_linux_md_get_level (drive->priv->device);
        }

        emblem_name = NULL;
        if (g_strcmp0 (level, "linear") == 0) {
                emblem_name = "gdu-emblem-raid-linear";
        } else if (g_strcmp0 (level, "raid0") == 0) {
                emblem_name = "gdu-emblem-raid0";
        } else if (g_strcmp0 (level, "raid1") == 0) {
                emblem_name = "gdu-emblem-raid1";
        } else if (g_strcmp0 (level, "raid4") == 0) {
                emblem_name = "gdu-emblem-raid4";
        } else if (g_strcmp0 (level, "raid5") == 0) {
                emblem_name = "gdu-emblem-raid5";
        } else if (g_strcmp0 (level, "raid6") == 0) {
                emblem_name = "gdu-emblem-raid6";
        } else if (g_strcmp0 (level, "raid10") == 0) {
                emblem_name = "gdu-emblem-raid10";
        }

        return gdu_util_get_emblemed_icon ("gdu-multidisk-drive", emblem_name);
}

static guint64
gdu_linux_md_drive_get_offset (GduPresentable *presentable)
{
        return 0;
}

static guint64
gdu_linux_md_drive_get_size (GduPresentable *presentable)
{
        GduLinuxMdDrive *drive = GDU_LINUX_MD_DRIVE (presentable);
        GduDevice *device;
        guint64 ret;
        const char *level;
        int num_raid_devices;
        guint component_size;

        ret = 0;

        if (drive->priv->device != NULL) {
                ret = gdu_device_get_size (drive->priv->device);
                goto out;
        }

        if (drive->priv->slaves == NULL) {
                g_warning ("%s: no device and no slaves", __FUNCTION__);
                goto out;
        }

        device = GDU_DEVICE (drive->priv->slaves->data);

        level = gdu_device_linux_md_component_get_level (device);
        num_raid_devices = gdu_device_linux_md_component_get_num_raid_devices (device);
        component_size = gdu_device_get_size (device);

        if (strcmp (level, "raid0") == 0) {
                /* stripes in linux can have different sizes so we don't really know
                 * until the drive is activated
                 */
                ret = 0;
        } else if (strcmp (level, "raid1") == 0) {
                ret = component_size;
        } else if (strcmp (level, "raid4") == 0) {
                ret = component_size * (num_raid_devices - 1) / num_raid_devices;
        } else if (strcmp (level, "raid5") == 0) {
                ret = component_size * (num_raid_devices - 1) / num_raid_devices;
        } else if (strcmp (level, "raid6") == 0) {
                ret = component_size * (num_raid_devices - 2) / num_raid_devices;
        } else if (strcmp (level, "raid10") == 0) {
                /* don't really know until the drive is activated */
                ret = 0;
        } else if (strcmp (level, "linear") == 0) {
                /* don't really know until the drive is activated */
                ret = 0;
        } else {
                g_warning ("%s: unknown level '%s'", __FUNCTION__, level);
        }

out:
        return ret;
}

static GduPool *
gdu_linux_md_drive_get_pool (GduPresentable *presentable)
{
        GduLinuxMdDrive *drive = GDU_LINUX_MD_DRIVE (presentable);
        return g_object_ref (drive->priv->pool);
}

static gboolean
gdu_linux_md_drive_is_allocated (GduPresentable *presentable)
{
        return TRUE;
}

static gboolean
gdu_linux_md_drive_is_recognized (GduPresentable *presentable)
{
        /* TODO: maybe we need to return FALSE sometimes */
        return TRUE;
}

static void
gdu_linux_md_drive_presentable_iface_init (GduPresentableIface *iface)
{
        iface->get_id                    = gdu_linux_md_drive_get_id;
        iface->get_device                = gdu_linux_md_drive_get_device;
        iface->get_enclosing_presentable = gdu_linux_md_drive_get_enclosing_presentable;
        iface->get_name                  = gdu_linux_md_drive_get_name;
        iface->get_description           = gdu_linux_md_drive_get_description;
        iface->get_vpd_name              = gdu_linux_md_drive_get_vpd_name;
        iface->get_icon                  = gdu_linux_md_drive_get_icon;
        iface->get_offset                = gdu_linux_md_drive_get_offset;
        iface->get_size                  = gdu_linux_md_drive_get_size;
        iface->get_pool                  = gdu_linux_md_drive_get_pool;
        iface->is_allocated              = gdu_linux_md_drive_is_allocated;
        iface->is_recognized             = gdu_linux_md_drive_is_recognized;
}

/* ---------------------------------------------------------------------------------------------------- */

void
_gdu_linux_md_drive_rewrite_enclosing_presentable (GduLinuxMdDrive *drive)
{
        if (drive->priv->enclosing_presentable != NULL) {
                const gchar *enclosing_presentable_id;
                GduPresentable *new_enclosing_presentable;

                enclosing_presentable_id = gdu_presentable_get_id (drive->priv->enclosing_presentable);

                new_enclosing_presentable = gdu_pool_get_presentable_by_id (drive->priv->pool,
                                                                            enclosing_presentable_id);
                if (new_enclosing_presentable == NULL) {
                        g_warning ("Error rewriting enclosing_presentable for %s, no such id %s",
                                   drive->priv->id,
                                   enclosing_presentable_id);
                        goto out;
                }

                g_object_unref (drive->priv->enclosing_presentable);
                drive->priv->enclosing_presentable = new_enclosing_presentable;
        }

 out:
        ;
}

/* ---------------------------------------------------------------------------------------------------- */

/* GduDrive virtual method overrides */

static gboolean
array_is_active (const gchar *state)
{
        gboolean ret;

        ret = TRUE;

        if (g_strcmp0 (state, "clear") == 0 ||
            g_strcmp0 (state, "inactive") == 0 ||
            g_strcmp0 (state, "suspended") == 0)
                ret = FALSE;

        return ret;
}

static gboolean
gdu_linux_md_drive_is_active (GduDrive *_drive)
{
        GduLinuxMdDrive *drive = GDU_LINUX_MD_DRIVE (_drive);
        //g_debug ("is running %p", drive->priv->device);
        return drive->priv->device != NULL && array_is_active (gdu_device_linux_md_get_state (drive->priv->device));
}

static gboolean
gdu_linux_md_drive_is_activatable (GduDrive *_drive)
{
        return TRUE;
}

static gboolean
gdu_linux_md_drive_can_deactivate (GduDrive *_drive)
{
        GduLinuxMdDrive *drive = GDU_LINUX_MD_DRIVE (_drive);
        /* if we have a underlying md device, we can always deactivate */
        return drive->priv->device != NULL;
}

static gboolean
check_can_activate (GduLinuxMdDrive *drive,
                    gboolean        *out_degraded)
{
        gboolean can_activate;
        gboolean degraded;
        guint num_slaves;
        guint num_devices;
        const gchar *raid_level;
        GduDevice *first_slave_device;

        can_activate = FALSE;
        degraded = FALSE;

        num_slaves = g_list_length (drive->priv->slaves);
        if (num_slaves == 0)
                goto out;

        first_slave_device = GDU_DEVICE (drive->priv->slaves->data);

        num_devices = gdu_device_linux_md_component_get_num_raid_devices (first_slave_device);
        raid_level = gdu_device_linux_md_component_get_level (first_slave_device);

        if (strcmp (raid_level, "raid0") == 0) {
                can_activate = (num_slaves == num_devices);
                degraded = FALSE;

        } else if (strcmp (raid_level, "linear") == 0) {
                can_activate = (num_slaves == num_devices);
                degraded = FALSE;

        } else if (strcmp (raid_level, "raid1") == 0) {
                can_activate = (num_slaves >= 1);
                degraded = (num_slaves < num_devices);

        } else if (strcmp (raid_level, "raid4") == 0) {
                can_activate = (num_slaves >= num_devices - 1);
                degraded = (num_slaves < num_devices);

        } else if (strcmp (raid_level, "raid5") == 0) {
                can_activate = (num_slaves >= num_devices - 1);
                degraded = (num_slaves < num_devices);

        } else if (strcmp (raid_level, "raid6") == 0) {
                can_activate = (num_slaves >= num_devices - 2);
                degraded = (num_slaves < num_devices);

        } else if (strcmp (raid_level, "raid10") == 0) {

                /* TODO: This is not necessarily correct; whether we can activate
                 *       really depends on what slaves are available and not just
                 *       the number of slaves... To check this we'd have to look
                 *       at the metadata of each slave.
                 *
                 *       So we just claim to be able to start the array if at least
                 *       half of the devices are available.
                 *
                 */
                can_activate = (num_slaves >= num_devices / 2);
                degraded = (num_slaves < num_devices);
        }

 out:
        if (out_degraded != NULL)
                *out_degraded = degraded;

        return can_activate;
}

static gboolean
gdu_linux_md_drive_can_activate (GduDrive *_drive,
                                 gboolean *out_degraded)
{
        GduLinuxMdDrive *drive = GDU_LINUX_MD_DRIVE (_drive);
        gboolean ret;

        ret = FALSE;

        /* if already activated, we can't activate it again */
        if (gdu_linux_md_drive_is_active (_drive))
                goto out;

        /* For now, refuse to activate if state is 'inactive' since this represents a partially
         * assembled drive (may be caused by broken auto-assembly rules).
         *
         * Note that can_deactivate() will return TRUE which allows the user to stop the partially
         * assembled array and then start it again.
         */
        if (drive->priv->device != NULL &&
            g_strcmp0 (gdu_device_linux_md_get_state (drive->priv->device), "inactive") == 0)
                goto out;

        ret = check_can_activate (drive, out_degraded);
out:
        return ret;
}

typedef struct
{
        GduLinuxMdDrive *drive;
        GduDriveActivateFunc callback;
        gpointer user_data;
} ActivationData;

static ActivationData *
activation_data_new (GduLinuxMdDrive *drive,
                     GduDriveActivateFunc callback,
                     gpointer user_data)
{
        ActivationData *ad;
        ad = g_new0 (ActivationData, 1);
        ad->drive = g_object_ref (drive);
        ad->callback = callback;
        ad->user_data = user_data;
        return ad;
}

static void
activation_data_free (ActivationData *ad)
{
        g_object_unref (ad->drive);
        g_free (ad);
}

static void
activation_completed (GduPool  *pool,
                      char     *assembled_array_object_path,
                      GError   *error,
                      gpointer  user_data)
{
        ActivationData *ad = user_data;
        ad->callback (GDU_DRIVE (ad->drive), assembled_array_object_path, error, ad->user_data);
        activation_data_free (ad);
}

static void
gdu_linux_md_drive_activate (GduDrive             *_drive,
                             GduDriveActivateFunc  callback,
                             gpointer              user_data)
{
        GduLinuxMdDrive *drive = GDU_LINUX_MD_DRIVE (_drive);
        GPtrArray *components;
        GList *l;
        GList *slaves;

        g_return_if_fail (drive->priv->device == NULL);

        components = g_ptr_array_new ();
        slaves = gdu_linux_md_drive_get_slaves (drive);
        for (l = slaves; l != NULL; l = l->next) {
                GduDevice *d = l->data;
                /* no need to dup; we keep a ref on d for the lifetime of components */
                g_ptr_array_add (components, (gpointer) gdu_device_get_object_path (d));
        }

        gdu_pool_op_linux_md_start (drive->priv->pool,
                                    components,
                                    activation_completed,
                                    activation_data_new (drive, callback, user_data));

        g_ptr_array_free (components, TRUE);
        g_list_foreach (slaves, (GFunc) g_object_unref, NULL);
        g_list_free (slaves);
}

typedef struct
{
        GduLinuxMdDrive *drive;
        GduDriveDeactivateFunc callback;
        gpointer user_data;
} DeactivationData;


static DeactivationData *
deactivation_data_new (GduLinuxMdDrive *drive,
                       GduDriveDeactivateFunc callback,
                       gpointer user_data)
{
        DeactivationData *dad;
        dad = g_new0 (DeactivationData, 1);
        dad->drive = g_object_ref (drive);
        dad->callback = callback;
        dad->user_data = user_data;
        return dad;
}

static void
deactivation_data_free (DeactivationData *dad)
{
        g_object_unref (dad->drive);
        g_free (dad);
}

static void
deactivation_completed (GduDevice *device,
                        GError    *error,
                        gpointer   user_data)
{
        DeactivationData *dad = user_data;
        dad->callback (GDU_DRIVE (dad->drive), error, dad->user_data);
        deactivation_data_free (dad);
}


static void
gdu_linux_md_drive_deactivate (GduDrive               *_drive,
                               GduDriveDeactivateFunc  callback,
                               gpointer                user_data)
{
        GduLinuxMdDrive *drive = GDU_LINUX_MD_DRIVE (_drive);

        g_return_if_fail (drive->priv->device != NULL);

        gdu_device_op_linux_md_stop (drive->priv->device,
                                     deactivation_completed,
                                     deactivation_data_new (drive, callback, user_data));
}

gchar *
gdu_linux_md_drive_get_slave_state_markup (GduLinuxMdDrive  *drive,
                                           GduDevice        *slave)
{
        gchar *slave_state_str;

        slave_state_str = NULL;
        if (gdu_drive_is_active (GDU_DRIVE (drive))) {
                GduLinuxMdDriveSlaveFlags slave_flags;
                GPtrArray *slave_state;
                gchar *s;

                slave_flags = gdu_linux_md_drive_get_slave_flags (drive, slave);

                slave_state = g_ptr_array_new_with_free_func (g_free);
                if (slave_flags & GDU_LINUX_MD_DRIVE_SLAVE_FLAGS_NOT_ATTACHED)
                        g_ptr_array_add (slave_state, g_strdup (C_("Linux MD slave state", "Not Attached")));
                if (slave_flags & GDU_LINUX_MD_DRIVE_SLAVE_FLAGS_FAULTY) {
                        s = g_strconcat ("<span foreground='red'><b>",
                                         C_("Linux MD slave state", "Faulty"),
                                         "</b></span>", NULL);
                        g_ptr_array_add (slave_state, s);
                }
                if (slave_flags & GDU_LINUX_MD_DRIVE_SLAVE_FLAGS_IN_SYNC)
                        g_ptr_array_add (slave_state, g_strdup (C_("Linux MD slave state", "Fully Synchronized")));
                if (slave_flags & GDU_LINUX_MD_DRIVE_SLAVE_FLAGS_WRITEMOSTLY)
                        g_ptr_array_add (slave_state, g_strdup (C_("Linux MD slave state", "Writemostly")));
                if (slave_flags & GDU_LINUX_MD_DRIVE_SLAVE_FLAGS_BLOCKED)
                        g_ptr_array_add (slave_state, g_strdup (C_("Linux MD slave state", "Blocked")));
                if (slave_flags & GDU_LINUX_MD_DRIVE_SLAVE_FLAGS_SPARE) {
                        if (gdu_device_linux_md_component_get_position (slave) >= 0) {
                                g_ptr_array_add (slave_state, g_strdup (C_("Linux MD slave state",
                                                                           "Partially Synchronized")));
                        } else {
                                g_ptr_array_add (slave_state, g_strdup (C_("Linux MD slave state", "Spare")));
                        }
                }
                g_ptr_array_add (slave_state, NULL);
                slave_state_str = g_strjoinv (", ", (gchar **) slave_state->pdata);
                g_ptr_array_free (slave_state, TRUE);

        }

        return slave_state_str;
}
