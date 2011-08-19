/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-volume.c
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
#include "gdu-volume.h"
#include "gdu-presentable.h"
#include "gdu-linux-md-drive.h"

/**
 * SECTION:gdu-volume
 * @title: GduVolume
 * @short_description: Volumes
 *
 * The #GduVolume class is used to represent regions of a drive;
 * typically it represents partitions (for partitioned devices) or
 * the whole file system (for e.g. optical discs and floppy disks).
 *
 * See the documentation for #GduPresentable for the big picture.
 */

struct _GduVolumePrivate
{
        GduDevice *device;
        GduPool *pool;
        GduPresentable *enclosing_presentable;
        gchar *id;
};

static GObjectClass *parent_class = NULL;

static void gdu_volume_presentable_iface_init (GduPresentableIface *iface);
G_DEFINE_TYPE_WITH_CODE (GduVolume, gdu_volume, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GDU_TYPE_PRESENTABLE,
                                                gdu_volume_presentable_iface_init))

static void device_job_changed (GduDevice *device, gpointer user_data);
static void device_changed (GduDevice *device, gpointer user_data);

static gboolean        gdu_volume_is_allocated_real  (GduVolume *volume);
static gboolean        gdu_volume_is_recognized_real (GduVolume *volume);
static GduVolumeFlags  gdu_volume_get_flags_real     (GduVolume *volume);

static const struct
{
        const char *disc_type;
        const char *icon_name;
        const char *ui_name;
        const char *ui_name_blank;
} disc_data[] = {
  /* Translator: The word "blank" is used as an adjective, e.g. we are decsribing discs that are already blank */
  {"optical_cd",             "media-optical-cd-rom",        N_("CD-ROM Disc"),     N_("Blank CD-ROM Disc")},
  {"optical_cd_r",           "media-optical-cd-r",          N_("CD-R Disc"),       N_("Blank CD-R Disc")},
  {"optical_cd_rw",          "media-optical-cd-rw",         N_("CD-RW Disc"),      N_("Blank CD-RW Disc")},
  {"optical_dvd",            "media-optical-dvd-rom",       N_("DVD-ROM Disc"),    N_("Blank DVD-ROM Disc")},
  {"optical_dvd_r",          "media-optical-dvd-r",         N_("DVD-ROM Disc"),    N_("Blank DVD-ROM Disc")},
  {"optical_dvd_rw",         "media-optical-dvd-rw",        N_("DVD-RW Disc"),     N_("Blank DVD-RW Disc")},
  {"optical_dvd_ram",        "media-optical-dvd-ram",       N_("DVD-RAM Disc"),    N_("Blank DVD-RAM Disc")},
  {"optical_dvd_plus_r",     "media-optical-dvd-r-plus",    N_("DVD+R Disc"),      N_("Blank DVD+R Disc")},
  {"optical_dvd_plus_rw",    "media-optical-dvd-rw-plus",   N_("DVD+RW Disc"),     N_("Blank DVD+RW Disc")},
  {"optical_dvd_plus_r_dl",  "media-optical-dvd-dl-r-plus", N_("DVD+R DL Disc"),   N_("Blank DVD+R DL Disc")},
  {"optical_dvd_plus_rw_dl", "media-optical-dvd-dl-r-plus", N_("DVD+RW DL Disc"),  N_("Blank DVD+RW DL Disc")},
  {"optical_bd",             "media-optical-bd-rom",        N_("Blu-Ray Disc"),    N_("Blank Blu-Ray Disc")},
  {"optical_bd_r",           "media-optical-bd-r",          N_("Blu-Ray R Disc"),  N_("Blank Blu-Ray R Disc")},
  {"optical_bd_re",          "media-optical-bd-re",         N_("Blu-Ray RW Disc"), N_("Blank Blu-Ray RW Disc")},
  {"optical_hddvd",          "media-optical-hddvd-rom",     N_("HD DVD Disc"),     N_("Blank HD DVD Disc")},
  {"optical_hddvd_r",        "media-optical-hddvd-r",       N_("HD DVD-R Disc"),   N_("Blank HD DVD-R Disc")},
  {"optical_hddvd_rw",       "media-optical-hddvd-rw",      N_("HD DVD-RW Disc"),  N_("Blank HD DVD-RW Disc")},
  {"optical_mo",             "media-optical-mo",            N_("MO Disc"),         N_("Blank MO Disc")},
  {"optical_mrw",            "media-optical-mrw",           N_("MRW Disc"),        N_("Blank MRW Disc")},
  {"optical_mrw_w",          "media-optical-mrw-w",         N_("MRW/W Disc"),      N_("Blank MRW/W Disc")},
  {NULL, NULL, NULL, NULL}
};

static void
gdu_volume_finalize (GObject *object)
{
        GduVolume *volume = GDU_VOLUME (object);

        //g_debug ("##### finalized volume '%s' %p", volume->priv->id, volume);

        if (volume->priv->device != NULL) {
                g_signal_handlers_disconnect_by_func (volume->priv->device, device_changed, volume);
                g_signal_handlers_disconnect_by_func (volume->priv->device, device_job_changed, volume);
                g_object_unref (volume->priv->device);
        }

        if (volume->priv->pool != NULL)
                g_object_unref (volume->priv->pool);

        if (volume->priv->enclosing_presentable != NULL)
                g_object_unref (volume->priv->enclosing_presentable);

        g_free (volume->priv->id);

        if (G_OBJECT_CLASS (parent_class)->finalize != NULL)
                (* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

static void
gdu_volume_class_init (GduVolumeClass *klass)
{
        GObjectClass *obj_class = (GObjectClass *) klass;

        parent_class = g_type_class_peek_parent (klass);

        obj_class->finalize = gdu_volume_finalize;

        klass->is_allocated   = gdu_volume_is_allocated_real;
        klass->is_recognized  = gdu_volume_is_recognized_real;
        klass->get_flags      = gdu_volume_get_flags_real;

        g_type_class_add_private (klass, sizeof (GduVolumePrivate));
}

static void
gdu_volume_init (GduVolume *volume)
{
        volume->priv = G_TYPE_INSTANCE_GET_PRIVATE (volume, GDU_TYPE_VOLUME, GduVolumePrivate);
}

static void
device_changed (GduDevice *device, gpointer user_data)
{
        GduVolume *volume = GDU_VOLUME (user_data);
        g_signal_emit_by_name (volume, "changed");
        g_signal_emit_by_name (volume->priv->pool, "presentable-changed", volume);
}

static void
device_job_changed (GduDevice *device, gpointer user_data)
{
        GduVolume *volume = GDU_VOLUME (user_data);
        g_signal_emit_by_name (volume, "job-changed");
        g_signal_emit_by_name (volume->priv->pool, "presentable-job-changed", volume);
}

GduVolume *
_gdu_volume_new_from_device (GduPool *pool, GduDevice *device, GduPresentable *enclosing_presentable)
{
        GduVolume *volume;

        volume = GDU_VOLUME (g_object_new (GDU_TYPE_VOLUME, NULL));
        volume->priv->device = g_object_ref (device);
        volume->priv->pool = g_object_ref (pool);
        volume->priv->enclosing_presentable =
                enclosing_presentable != NULL ? g_object_ref (enclosing_presentable) : NULL;
        volume->priv->id = g_strdup_printf ("volume_%s_enclosed_by_%s",
                                            gdu_device_get_device_file (volume->priv->device),
                                            enclosing_presentable != NULL ? gdu_presentable_get_id (enclosing_presentable) : "(none)");

        g_signal_connect (device, "changed", (GCallback) device_changed, volume);
        g_signal_connect (device, "job-changed", (GCallback) device_job_changed, volume);
        return volume;
}

static const gchar *
gdu_volume_get_id (GduPresentable *presentable)
{
        GduVolume *volume = GDU_VOLUME (presentable);
        return volume->priv->id;
}

static GduDevice *
gdu_volume_get_device (GduPresentable *presentable)
{
        GduVolume *volume = GDU_VOLUME (presentable);
        return g_object_ref (volume->priv->device);
}

static GduPresentable *
gdu_volume_get_enclosing_presentable (GduPresentable *presentable)
{
        GduVolume *volume = GDU_VOLUME (presentable);
        if (volume->priv->enclosing_presentable != NULL)
                return g_object_ref (volume->priv->enclosing_presentable);
        return NULL;
}

/* This function can be used for any GduPresentable */
gchar *
_gdu_volume_get_names_and_desc (GduPresentable  *presentable,
                                gchar          **out_vpd_name,
                                gchar          **out_desc)
{
        GduPresentable *drive_presentable;
        GduDevice *device;
        GduDevice *drive_device;
        const char *label;
        const char *usage;
        const char *type;
        const char *version;
        const char *drive_media;
        const char *presentation_name;
        char *result;
        gboolean is_extended_partition;
        char *strsize;
        guint64 size;
        guint n;
        gchar *result_desc;
        gchar *result_vpd;

        result = NULL;
        result_desc = NULL;
        result_vpd = NULL;

        device = NULL;
        drive_presentable = NULL;
        drive_device = NULL;
        drive_media = NULL;
        strsize = NULL;

        device = gdu_presentable_get_device (presentable);
        if (device == NULL) {
                g_warning ("GduDevice is NULL for GduPresentable with id %s of type %s",
                           gdu_presentable_get_id (presentable),
                           G_OBJECT_TYPE_NAME (presentable));
                goto out;
        }

        drive_presentable = gdu_presentable_get_enclosing_presentable (presentable);
        if (drive_presentable != NULL) {
                drive_device = gdu_presentable_get_device (drive_presentable);
                if (drive_device != NULL)
                  drive_media = gdu_device_drive_get_media (drive_device);
        }

        label = gdu_device_id_get_label (device);
        if (gdu_device_is_partition (device))
                size = gdu_device_partition_get_size (device);
        else
                size = gdu_device_get_size (device);
        strsize = gdu_util_get_size_for_display (size, FALSE, FALSE);

        presentation_name = gdu_device_get_presentation_name (device);
        if (presentation_name != NULL && strlen (presentation_name) > 0) {
                result = g_strdup (presentation_name);
                goto out;
        }

        /* see comment in gdu_pool_add_device_by_object_path() for how to avoid hardcoding 0x05 etc. types */
        is_extended_partition = FALSE;
        if (gdu_device_is_partition (device) &&
            strcmp (gdu_device_partition_get_scheme (device), "mbr") == 0) {
                int part_type;
                part_type = strtol (gdu_device_partition_get_type (device), NULL, 0);
                if (part_type == 0x05 || part_type == 0x0f || part_type == 0x85)
                        is_extended_partition = TRUE;
        }

        usage = gdu_device_id_get_usage (device);
        type = gdu_device_id_get_type (device);
        version = gdu_device_id_get_version (device);

        /* handle optical discs */
        if (gdu_device_is_optical_disc (device) &&
            gdu_device_optical_disc_get_is_blank (device)) {
                for (n = 0; disc_data[n].disc_type != NULL; n++) {
                        if (g_strcmp0 (disc_data[n].disc_type, drive_media) == 0) {
                                result = g_strdup (gettext (disc_data[n].ui_name_blank));
                                break;
                        }
                }

                if (result == NULL) {
                        g_warning ("Unknown drive-media value '%s'", drive_media);
                        result = g_strdup (_("Blank Optical Disc"));
                }

                goto out;
        }

        if (is_extended_partition) {
                /* Translators: Label for an extended partition
                 * %s is the size, formatted like '45 GB'
                 */
                result = g_strdup_printf (_("%s Extended"), strsize);
                result_desc = g_strdup (_("Contains logical partitions"));
        } else if ((usage != NULL && strcmp (usage, "filesystem") == 0) &&
                   (label != NULL && strlen (label) > 0)) {
                gchar *fsdesc;
                result = g_strdup (label);
                fsdesc = gdu_util_get_fstype_for_display (type, version, TRUE);
                result_desc = g_strdup_printf ("%s %s",
                                               strsize,
                                               fsdesc);
                g_free (fsdesc);
        } else if (usage != NULL) {
                if (strcmp (usage, "crypto") == 0) {
                        /* Translators: Label for an extended partition
                         * %s is the size, formatted like '45 GB'
                         */
                        result = g_strdup_printf (_("%s Encrypted"), strsize);
                } else if (gdu_device_is_optical_disc (device)) {
                        for (n = 0; disc_data[n].disc_type != NULL; n++) {
                                if (g_strcmp0 (disc_data[n].disc_type, drive_media) == 0) {
                                        result = g_strdup (gettext (disc_data[n].ui_name));
                                        break;
                                }
                        }
                        if (result == NULL) {
                                g_warning ("Unknown drive-media value '%s'", drive_media);
                                result = g_strdup (_("Optical Disc"));
                        }
                } else if (strcmp (usage, "filesystem") == 0) {
                        /* Translators: Label for a partition with a filesystem
                         * %s is the size, formatted like '45 GB'
                         */
                        result = g_strdup_printf (_("%s Filesystem"), strsize);
                        result_desc = gdu_util_get_fstype_for_display (type, version, TRUE);
                } else if (strcmp (usage, "partitiontable") == 0) {
                        /* Translators: Label for a partition table
                         * %s is the size, formatted like '45 GB'
                         */
                        result = g_strdup_printf (_("%s Partition Table"), strsize);
                } else if (strcmp (usage, "raid") == 0) {
                        if (strcmp (type, "LVM2_member") == 0) {
                                /* Translators: Label for a LVM volume
                                * %s is the size, formatted like '45 GB'
                                */
                                result = g_strdup_printf (_("%s LVM2 Physical Volume"), strsize);
                        } else {
                                const gchar *array_name;
                                const gchar *level;
                                gchar *level_str;

                                array_name = gdu_device_linux_md_component_get_name (device);
                                level = gdu_device_linux_md_component_get_level (device);

                                if (level != NULL && strlen (level) > 0)
                                        level_str = gdu_linux_md_get_raid_level_for_display (level, FALSE);
                                else
                                        /* Translators: Used if no specific RAID level could be determined */
                                        level_str = g_strdup (C_("RAID level", "RAID"));

                                if (array_name != NULL && strlen (array_name) > 0) {
                                        /* Translators: label for a RAID component
                                         * First %s is the size, formatted like '45 GB'
                                         * Second %s is the RAID level string, e.g 'RAID-5'
                                         */
                                        result = g_strdup_printf (_("%s %s Component"), strsize, level_str);
                                        /* Translators: description for a RAID component
                                         * First %s is the array name, e.g. 'My Photos RAID',
                                         */
                                        result_desc = g_strdup_printf (_("Part of \"%s\" array"),
                                                                       array_name);
                                } else {
                                        /* Translators: label for a RAID component
                                         * First %s is the size, formatted like '45 GB'
                                         * Second %s is the RAID level string, e.g 'RAID-5'
                                         */
                                        result = g_strdup_printf (_("%s %s Component"), strsize, level_str);
                                        result_desc = g_strdup (level_str);
                                }

                                g_free (level_str);
                        }
                } else if (strcmp (usage, "other") == 0) {
                        if (strcmp (type, "swap") == 0) {
                                /* Translators: label for a swap partition
                                 * %s is the size, formatted like '45 GB'
                                 */
                                result = g_strdup_printf (_("%s Swap Space"), strsize);
                        } else {
                                /* Translators: label for a data partition
                                 * %s is the size, formatted like '45 GB'
                                 */
                                result = g_strdup_printf (_("%s Data"), strsize);
                        }
                } else if (strcmp (usage, "") == 0) {
                        /* Translators: label for a volume of unrecognized use
                         * %s is the size, formatted like '45 GB'
                         */
                        result = g_strdup_printf (_("%s Unrecognized"), strsize);
                        /* Translators: description for a volume of unrecognized use */
                        result_desc = g_strdup (_("Unknown or Unused"));
                }
        } else {
                if (gdu_device_is_partition (device)) {
                        /* Translators: label for a partition
                         * %s is the size, formatted like '45 GB'
                         */
                        result = g_strdup_printf (_("%s Partition"), strsize);
                } else {
                        result = g_strdup_printf (_("%s Partition"), strsize);
                }
        }

        if (result == NULL)
                result = g_strdup_printf (_("%s Unrecognized"), strsize);

 out:
        if (device != NULL)
                g_object_unref (device);
        if (drive_device != NULL)
                g_object_unref (drive_device);
        if (drive_presentable != NULL)
                g_object_unref (drive_presentable);

        /* Fallback if description isn't explicitly set */
        if (result_desc == NULL) {
                result_desc = g_strdup (strsize);
        }

        /* Fallback if VPD name isn't explicitly set */
        if (result_vpd == NULL) {
                gchar *drive_vpd_name;

                drive_vpd_name = NULL;

                if (drive_presentable != NULL) {
                        drive_vpd_name = gdu_presentable_get_vpd_name (drive_presentable);
                }

                if (gdu_device_is_partition (device)) {
                        if (drive_vpd_name != NULL) {
                                /* Translators: The VPD name for a volume. The %d is the partition number
                                 * and the %s is the VPD name for the drive.
                                 */
                                result_vpd = g_strdup_printf (_("Partition %d of %s"),
                                                              gdu_device_partition_get_number (device),
                                                              drive_vpd_name);
                        } else {
                                /* Translators: The VPD name for a volume. The %d is the partition number.
                                 */
                                result_vpd = g_strdup_printf (_("Partition %d"),
                                                              gdu_device_partition_get_number (device));
                        }
                } else {
                        if (drive_vpd_name != NULL) {
                                /* Translators: The VPD name for a whole-disk volume.
                                 * The %s is the VPD name for the drive.
                                 */
                                result_vpd = g_strdup_printf (_("Whole-disk volume on %s"),
                                                              drive_vpd_name);
                        } else {
                                /* Translators: The VPD name for a whole-disk volume.
                                 */
                                result_vpd = g_strdup (_("Whole-disk volume"));
                        }
                }
                g_free (drive_vpd_name);
        }

        if (out_desc != NULL)
                *out_desc = result_desc;
        else
                g_free (result_desc);

        if (out_vpd_name != NULL)
                *out_vpd_name = result_vpd;
        else
                g_free (result_vpd);

        g_free (strsize);

        return result;
}

static char *
gdu_volume_get_name (GduPresentable *presentable)
{
        return _gdu_volume_get_names_and_desc (presentable, NULL, NULL);
}

static gchar *
gdu_volume_get_description (GduPresentable *presentable)
{
        gchar *desc;
        gchar *name;

        name = _gdu_volume_get_names_and_desc (presentable, NULL, &desc);
        g_free (name);

        return desc;
}

static gchar *
gdu_volume_get_vpd_name (GduPresentable *presentable)
{
        gchar *vpd_name;
        gchar *name;

        name = _gdu_volume_get_names_and_desc (presentable, &vpd_name, NULL);
        g_free (name);

        return vpd_name;
}

static GIcon *
gdu_volume_get_icon (GduPresentable *presentable)
{
        GduVolume *volume = GDU_VOLUME (presentable);
        GduPresentable *p;
        GduDevice *d;
        const char *usage;
        const char *connection_interface;
        const char *name;
        const char *drive_media;
        const char *presentation_icon_name;
        gboolean is_removable;
        GIcon *icon;
        guint n;

        p = NULL;
        d = NULL;
        name = NULL;
        is_removable = FALSE;
        usage = NULL;
        icon = NULL;

        if (gdu_device_is_luks_cleartext (volume->priv->device)) {
                GduPresentable *enclosing_cryptotext_volume;
                GIcon *enclosing_cryptotext_icon;
                GEmblem *emblem;
                GIcon *padlock;

                enclosing_cryptotext_volume = gdu_presentable_get_enclosing_presentable (presentable);
                g_warn_if_fail (GDU_IS_VOLUME (enclosing_cryptotext_volume));
                enclosing_cryptotext_icon = gdu_presentable_get_icon (enclosing_cryptotext_volume);
                g_object_unref (enclosing_cryptotext_volume);

                padlock = g_themed_icon_new ("gdu-encrypted-unlock");
                emblem = g_emblem_new_with_origin (padlock, G_EMBLEM_ORIGIN_DEVICE);
                icon = g_emblemed_icon_new (enclosing_cryptotext_icon, emblem);

                g_object_unref (padlock);
                g_object_unref (emblem);
                g_object_unref (enclosing_cryptotext_icon);
                goto out;
        }

        usage = gdu_device_id_get_usage (volume->priv->device);

        presentation_icon_name = gdu_device_get_presentation_icon_name (volume->priv->device);
        if (presentation_icon_name != NULL && strlen (presentation_icon_name) > 0) {
                name = presentation_icon_name;
                goto out;
        }

        p = gdu_presentable_get_enclosing_presentable (GDU_PRESENTABLE (presentable));
        /* handle logical partitions enclosed by an extented partition */
        if (GDU_IS_VOLUME (p)) {
                GduPresentable *temp;
                temp = p;
                p = gdu_presentable_get_enclosing_presentable (p);
                g_object_unref (temp);
                if (!GDU_IS_DRIVE (p)) {
                        g_object_unref (p);
                        p = NULL;
                }
        }
        if (p == NULL)
                goto out;

        if (GDU_IS_LINUX_MD_DRIVE (p)) {
                name = "gdu-multidisk-drive";
                goto out;
        }

#if 0
        /* unless it's a file system or LUKS encrypted volume, use the gdu-unmountable icon */
        if (g_strcmp0 (usage, "filesystem") != 0 && g_strcmp0 (usage, "crypto") != 0) {
                name = "gdu-unmountable";
                goto out;
        }
#endif

        d = gdu_presentable_get_device (p);
        if (d == NULL)
                goto out;

        if (!gdu_device_is_drive (d))
                goto out;


        if (gdu_device_is_linux_loop (d)) {
                name = "drive-removable-media-file";
                goto out;
        }

        connection_interface = gdu_device_drive_get_connection_interface (d);
        if (connection_interface == NULL)
                goto out;

        is_removable = gdu_device_is_removable (d);

        drive_media = gdu_device_drive_get_media (d);

        /* first try the media */
        if (drive_media != NULL) {
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
                } else if (strcmp (drive_media, "floppy") == 0) {
                        name = "media-floppy";
                } else if (strcmp (drive_media, "floppy_zip") == 0) {
                        name = "media-floppy-zip";
                } else if (strcmp (drive_media, "floppy_jaz") == 0) {
                        name = "media-floppy-jaz";
                } else if (g_str_has_prefix (drive_media, "flash")) {
                        name = "media-flash";
                } else if (g_str_has_prefix (drive_media, "optical")) {
                        for (n = 0; disc_data[n].disc_type != NULL; n++) {
                                if (strcmp (disc_data[n].disc_type, drive_media) == 0) {
                                        name = disc_data[n].icon_name;
                                        break;
                                }
                        }
                        if (name == NULL)
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

        if (name != NULL) {
                icon = g_themed_icon_new_with_default_fallbacks (name);
        }

out:
#if 0
        if (usage != NULL && strcmp (usage, "crypto") == 0) {
                GEmblem *emblem;
                GIcon *padlock;
                GIcon *emblemed_icon;

                padlock = g_themed_icon_new ("gdu-encrypted-lock");
                emblem = g_emblem_new_with_origin (padlock, G_EMBLEM_ORIGIN_DEVICE);

                emblemed_icon = g_emblemed_icon_new (icon, emblem);
                g_object_unref (icon);
                icon = emblemed_icon;

                g_object_unref (padlock);
                g_object_unref (emblem);
        }
#endif

        /* ultimate fallback */
        if (icon == NULL) {
                if (name == NULL) {
                        if (is_removable)
                                name = "drive-removable-media";
                        else
                                name = "drive-harddisk";
                }
                icon = g_themed_icon_new_with_default_fallbacks (name);
        }

        /* Attach a MP emblem if it's a multipathed device or a path for a multipathed device */
        if (d != NULL && gdu_device_is_linux_dmmp (d)) {
                GEmblem *emblem;
                GIcon *padlock;
                GIcon *emblemed_icon;

                padlock = g_themed_icon_new ("gdu-emblem-mp");
                emblem = g_emblem_new_with_origin (padlock, G_EMBLEM_ORIGIN_DEVICE);

                emblemed_icon = g_emblemed_icon_new (icon, emblem);
                g_object_unref (icon);
                icon = emblemed_icon;

                g_object_unref (padlock);
                g_object_unref (emblem);
        } else if (d != NULL && gdu_device_is_linux_dmmp_component (d)) {
                GEmblem *emblem;
                GIcon *padlock;
                GIcon *emblemed_icon;

                padlock = g_themed_icon_new ("gdu-emblem-mp-component");
                emblem = g_emblem_new_with_origin (padlock, G_EMBLEM_ORIGIN_DEVICE);

                emblemed_icon = g_emblemed_icon_new (icon, emblem);
                g_object_unref (icon);
                icon = emblemed_icon;

                g_object_unref (padlock);
                g_object_unref (emblem);
        }

        if (p != NULL)
                g_object_unref (p);
        if (d != NULL)
                g_object_unref (d);

        return icon;
}

static guint64
gdu_volume_get_offset (GduPresentable *presentable)
{
        GduVolume *volume = GDU_VOLUME (presentable);
        if (gdu_device_is_partition (volume->priv->device))
                return gdu_device_partition_get_offset (volume->priv->device);
        return 0;
}

static guint64
gdu_volume_get_size (GduPresentable *presentable)
{
        GduVolume *volume = GDU_VOLUME (presentable);
        if (gdu_device_is_partition (volume->priv->device))
                return gdu_device_partition_get_size (volume->priv->device);
        return gdu_device_get_size (volume->priv->device);
}

static GduPool *
gdu_volume_get_pool (GduPresentable *presentable)
{
        GduVolume *volume = GDU_VOLUME (presentable);
        return gdu_device_get_pool (volume->priv->device);
}


static gboolean
gdu_volume_presentable_is_allocated (GduPresentable *presentable)
{
        return TRUE;
}

static gboolean
gdu_volume_presentable_is_recognized (GduPresentable *presentable)
{
        GduVolume *volume = GDU_VOLUME (presentable);
        gboolean is_extended_partition;

        is_extended_partition = FALSE;
        if (gdu_device_is_partition (volume->priv->device) &&
            strcmp (gdu_device_partition_get_scheme (volume->priv->device), "mbr") == 0) {
                int type;
                type = strtol (gdu_device_partition_get_type (volume->priv->device), NULL, 0);
                if (type == 0x05 || type == 0x0f || type == 0x85)
                        is_extended_partition = TRUE;
        }

        if (is_extended_partition)
                return TRUE;
        else
                return strlen (gdu_device_id_get_usage (volume->priv->device)) > 0;
}

static void
gdu_volume_presentable_iface_init (GduPresentableIface *iface)
{
        iface->get_id                    = gdu_volume_get_id;
        iface->get_device                = gdu_volume_get_device;
        iface->get_enclosing_presentable = gdu_volume_get_enclosing_presentable;
        iface->get_name                  = gdu_volume_get_name;
        iface->get_description           = gdu_volume_get_description;
        iface->get_vpd_name              = gdu_volume_get_vpd_name;
        iface->get_icon                  = gdu_volume_get_icon;
        iface->get_offset                = gdu_volume_get_offset;
        iface->get_size                  = gdu_volume_get_size;
        iface->get_pool                  = gdu_volume_get_pool;
        iface->is_allocated              = gdu_volume_presentable_is_allocated;
        iface->is_recognized             = gdu_volume_presentable_is_recognized;
}

void
_gdu_volume_rewrite_enclosing_presentable (GduVolume *volume)
{
        if (volume->priv->enclosing_presentable != NULL) {
                const gchar *enclosing_presentable_id;
                GduPresentable *new_enclosing_presentable;

                enclosing_presentable_id = gdu_presentable_get_id (volume->priv->enclosing_presentable);

                new_enclosing_presentable = gdu_pool_get_presentable_by_id (volume->priv->pool,
                                                                            enclosing_presentable_id);
                if (new_enclosing_presentable == NULL) {
                        g_warning ("Error rewriting enclosing_presentable for %s, no such id %s",
                                   volume->priv->id,
                                   enclosing_presentable_id);
                        goto out;
                }

                g_object_unref (volume->priv->enclosing_presentable);
                volume->priv->enclosing_presentable = new_enclosing_presentable;
        }

 out:
        ;
}

/* ---------------------------------------------------------------------------------------------------- */

gboolean
gdu_volume_is_allocated (GduVolume *volume)
{
        GduVolumeClass *klass = GDU_VOLUME_GET_CLASS (volume);
        return klass->is_allocated (volume);
}

gboolean
gdu_volume_is_recognized (GduVolume *volume)
{
        GduVolumeClass *klass = GDU_VOLUME_GET_CLASS (volume);
        return klass->is_recognized (volume);
}

GduVolumeFlags
gdu_volume_get_flags (GduVolume *volume)
{
        GduVolumeClass *klass = GDU_VOLUME_GET_CLASS (volume);
        return klass->get_flags (volume);
}

/* ---------------------------------------------------------------------------------------------------- */

static gboolean
gdu_volume_is_allocated_real (GduVolume *volume)
{
        GduDevice *d;
        gboolean ret;

        ret = FALSE;
        d = gdu_presentable_get_device (GDU_PRESENTABLE (volume));
        if (d != NULL) {
                ret = TRUE;
                g_object_unref (d);
        }

        return ret;
}

static gboolean
gdu_volume_is_recognized_real (GduVolume *volume)
{
        GduDevice *d;
        gboolean ret;

        ret = FALSE;
        d = gdu_presentable_get_device (GDU_PRESENTABLE (volume));
        if (d != NULL) {
                gboolean is_extended_partition;
                is_extended_partition = FALSE;
                if (gdu_device_is_partition (d) && g_strcmp0 (gdu_device_partition_get_scheme (d), "mbr") == 0) {
                        gint type;
                        type = strtol (gdu_device_partition_get_type (d), NULL, 0);
                        if (type == 0x05 || type == 0x0f || type == 0x85) {
                                is_extended_partition = TRUE;
                        }
                }

                if (is_extended_partition)
                        ret = TRUE;
                else if (strlen (gdu_device_id_get_usage (volume->priv->device)) > 0)
                        ret = TRUE;

                g_object_unref (d);
        }

        return ret;
}

static GduVolumeFlags
gdu_volume_get_flags_real (GduVolume *volume)
{
        GduVolumeFlags ret;
        GduDevice *d;

        ret = GDU_VOLUME_FLAGS_NONE;

        d = gdu_presentable_get_device (GDU_PRESENTABLE (volume));
        if (d != NULL) {
                if (gdu_device_is_partition (d)) {
                        ret |= GDU_VOLUME_FLAGS_PARTITION;
                }

                if (g_strcmp0 (gdu_device_partition_get_scheme (d), "mbr") == 0) {
                        gint type;

                        type = strtol (gdu_device_partition_get_type (volume->priv->device), NULL, 0);
                        if (type == 0x05 || type == 0x0f || type == 0x85) {
                                ret |= GDU_VOLUME_FLAGS_PARTITION_MBR_EXTENDED;
                        }

                        if (gdu_device_partition_get_number (d) >= 5)
                                ret |= GDU_VOLUME_FLAGS_PARTITION_MBR_LOGICAL;
                }
                g_object_unref (d);
        }
        return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

GduDrive *
gdu_volume_get_drive (GduVolume *volume)
{
        GduDrive *ret;
        GduPresentable *p;

        ret = NULL;

        p = GDU_PRESENTABLE (volume);
        do {
                GduPresentable *enclosing;

                enclosing = gdu_presentable_get_enclosing_presentable (p);
                if (enclosing == NULL)
                        goto out;

                if (GDU_IS_DRIVE (enclosing)) {
                        ret = GDU_DRIVE (enclosing);
                        goto out;
                }

                g_object_unref (enclosing);
                p = enclosing;

        } while (TRUE);

 out:
        return ret;
}

/* ---------------------------------------------------------------------------------------------------- */
