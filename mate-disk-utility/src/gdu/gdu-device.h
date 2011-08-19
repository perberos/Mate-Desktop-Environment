/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-device.h
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

#if !defined (__GDU_INSIDE_GDU_H) && !defined (GDU_COMPILATION)
#error "Only <gdu/gdu.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef __GDU_DEVICE_H
#define __GDU_DEVICE_H

#include <unistd.h>
#include <sys/types.h>

#include <gdu/gdu-types.h>
#include <gdu/gdu-callbacks.h>

G_BEGIN_DECLS

#define GDU_TYPE_DEVICE           (gdu_device_get_type ())
#define GDU_DEVICE(o)             (G_TYPE_CHECK_INSTANCE_CAST ((o), GDU_TYPE_DEVICE, GduDevice))
#define GDU_DEVICE_CLASS(k)       (G_TYPE_CHECK_CLASS_CAST ((k), GDU_DEVICE,  GduDeviceClass))
#define GDU_IS_DEVICE(o)          (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDU_TYPE_DEVICE))
#define GDU_IS_DEVICE_CLASS(k)    (G_TYPE_CHECK_CLASS_TYPE ((k), GDU_TYPE_DEVICE))
#define GDU_DEVICE_GET_CLASS(k)   (G_TYPE_INSTANCE_GET_CLASS ((k), GDU_TYPE_DEVICE, GduDeviceClass))

typedef struct _GduDeviceClass    GduDeviceClass;
typedef struct _GduDevicePrivate  GduDevicePrivate;

struct _GduDevice
{
        GObject parent;

        /* private */
        GduDevicePrivate *priv;
};

struct _GduDeviceClass
{
        GObjectClass parent_class;

        /* signals */
        void (*changed)     (GduDevice *device);
        void (*job_changed) (GduDevice *device);
        void (*removed)     (GduDevice *device);
};

GType       gdu_device_get_type              (void);
const char *gdu_device_get_object_path       (GduDevice   *device);
GduDevice  *gdu_device_find_parent           (GduDevice   *device);
GduPool    *gdu_device_get_pool              (GduDevice   *device);

dev_t gdu_device_get_dev (GduDevice *device);
guint64 gdu_device_get_detection_time (GduDevice *device);
guint64 gdu_device_get_media_detection_time (GduDevice *device);
const char *gdu_device_get_device_file (GduDevice *device);
const char *gdu_device_get_device_file_presentation (GduDevice *device);
guint64 gdu_device_get_size (GduDevice *device);
guint64 gdu_device_get_block_size (GduDevice *device);
gboolean gdu_device_is_removable (GduDevice *device);
gboolean gdu_device_is_media_available (GduDevice *device);
gboolean gdu_device_is_media_change_detected (GduDevice *device);
gboolean gdu_device_is_media_change_detection_polling (GduDevice *device);
gboolean gdu_device_is_media_change_detection_inhibitable (GduDevice *device);
gboolean gdu_device_is_media_change_detection_inhibited (GduDevice *device);
gboolean gdu_device_is_read_only (GduDevice *device);
gboolean gdu_device_is_system_internal (GduDevice *device);
gboolean gdu_device_is_partition (GduDevice *device);
gboolean gdu_device_is_partition_table (GduDevice *device);
gboolean gdu_device_is_drive (GduDevice *device);
gboolean gdu_device_is_optical_disc (GduDevice *device);
gboolean gdu_device_is_luks (GduDevice *device);
gboolean gdu_device_is_luks_cleartext (GduDevice *device);
gboolean gdu_device_is_linux_md_component (GduDevice *device);
gboolean gdu_device_is_linux_md (GduDevice *device);
gboolean gdu_device_is_linux_lvm2_lv (GduDevice *device);
gboolean gdu_device_is_linux_lvm2_pv (GduDevice *device);
gboolean gdu_device_is_linux_dmmp (GduDevice *device);
gboolean gdu_device_is_linux_dmmp_component (GduDevice *device);
gboolean gdu_device_is_linux_loop (GduDevice *device);
gboolean gdu_device_is_mounted (GduDevice *device);
const char *gdu_device_get_mount_path (GduDevice *device);
char **gdu_device_get_mount_paths (GduDevice *device);
uid_t gdu_device_get_mounted_by_uid (GduDevice *device);
gboolean    gdu_device_get_presentation_hide (GduDevice *device);
gboolean    gdu_device_get_presentation_nopolicy (GduDevice *device);
const char *gdu_device_get_presentation_name (GduDevice *device);
const char *gdu_device_get_presentation_icon_name (GduDevice *device);

gboolean    gdu_device_job_in_progress (GduDevice *device);
const char *gdu_device_job_get_id (GduDevice *device);
uid_t       gdu_device_job_get_initiated_by_uid (GduDevice *device);
gboolean    gdu_device_job_is_cancellable (GduDevice *device);
double      gdu_device_job_get_percentage (GduDevice *device);

const char *gdu_device_id_get_usage (GduDevice *device);
const char *gdu_device_id_get_type (GduDevice *device);
const char *gdu_device_id_get_version (GduDevice *device);
const char *gdu_device_id_get_label (GduDevice *device);
const char *gdu_device_id_get_uuid (GduDevice *device);

const char *gdu_device_partition_get_slave (GduDevice *device);
const char *gdu_device_partition_get_scheme (GduDevice *device);
const char *gdu_device_partition_get_type (GduDevice *device);
const char *gdu_device_partition_get_label (GduDevice *device);
const char *gdu_device_partition_get_uuid (GduDevice *device);
char **gdu_device_partition_get_flags (GduDevice *device);
int gdu_device_partition_get_number (GduDevice *device);
guint64 gdu_device_partition_get_offset (GduDevice *device);
guint64 gdu_device_partition_get_size (GduDevice *device);
guint64 gdu_device_partition_get_alignment_offset (GduDevice *device);

const char *gdu_device_partition_table_get_scheme (GduDevice *device);
int         gdu_device_partition_table_get_count (GduDevice *device);

const char *gdu_device_luks_get_holder (GduDevice *device);

const char *gdu_device_luks_cleartext_get_slave (GduDevice *device);
uid_t gdu_device_luks_cleartext_unlocked_by_uid (GduDevice *device);

const char *gdu_device_drive_get_vendor (GduDevice *device);
const char *gdu_device_drive_get_model (GduDevice *device);
const char *gdu_device_drive_get_revision (GduDevice *device);
const char *gdu_device_drive_get_serial (GduDevice *device);
const char *gdu_device_drive_get_wwn (GduDevice *device);
const char *gdu_device_drive_get_connection_interface (GduDevice *device);
guint64 gdu_device_drive_get_connection_speed (GduDevice *device);
char **gdu_device_drive_get_media_compatibility (GduDevice *device);
const char *gdu_device_drive_get_media (GduDevice *device);
gboolean gdu_device_drive_get_is_media_ejectable (GduDevice *device);
gboolean gdu_device_drive_get_requires_eject (GduDevice *device);
gboolean gdu_device_drive_get_can_detach (GduDevice *device);
gboolean gdu_device_drive_get_can_spindown (GduDevice *device);
gboolean gdu_device_drive_get_is_rotational (GduDevice *device);
guint    gdu_device_drive_get_rotation_rate (GduDevice *device);
const char *gdu_device_drive_get_write_cache (GduDevice *device);
const char *gdu_device_drive_get_adapter (GduDevice *device);
char **gdu_device_drive_get_ports (GduDevice *device);
char **gdu_device_drive_get_similar_devices (GduDevice *device);

gboolean gdu_device_optical_disc_get_is_blank (GduDevice *device);
gboolean gdu_device_optical_disc_get_is_appendable (GduDevice *device);
gboolean gdu_device_optical_disc_get_is_closed (GduDevice *device);
guint gdu_device_optical_disc_get_num_tracks (GduDevice *device);
guint gdu_device_optical_disc_get_num_audio_tracks (GduDevice *device);
guint gdu_device_optical_disc_get_num_sessions (GduDevice *device);

const char *gdu_device_linux_md_component_get_level (GduDevice *device);
int         gdu_device_linux_md_component_get_position (GduDevice *device);
int         gdu_device_linux_md_component_get_num_raid_devices (GduDevice *device);
const char *gdu_device_linux_md_component_get_uuid (GduDevice *device);
const char *gdu_device_linux_md_component_get_home_host (GduDevice *device);
const char *gdu_device_linux_md_component_get_name (GduDevice *device);
const char *gdu_device_linux_md_component_get_version (GduDevice *device);
const char *gdu_device_linux_md_component_get_holder (GduDevice *device);
char       **gdu_device_linux_md_component_get_state (GduDevice *device);

const char *gdu_device_linux_md_get_state (GduDevice *device);
const char *gdu_device_linux_md_get_level (GduDevice *device);
int         gdu_device_linux_md_get_num_raid_devices (GduDevice *device);
const char *gdu_device_linux_md_get_uuid (GduDevice *device);
const char *gdu_device_linux_md_get_home_host (GduDevice *device);
const char *gdu_device_linux_md_get_name (GduDevice *device);
const char *gdu_device_linux_md_get_version (GduDevice *device);
char      **gdu_device_linux_md_get_slaves (GduDevice *device);
gboolean    gdu_device_linux_md_is_degraded (GduDevice *device);
const char *gdu_device_linux_md_get_sync_action (GduDevice *device);
double      gdu_device_linux_md_get_sync_percentage (GduDevice *device);
guint64     gdu_device_linux_md_get_sync_speed (GduDevice *device);

const char *gdu_device_linux_lvm2_lv_get_name (GduDevice *device);
const char *gdu_device_linux_lvm2_lv_get_uuid (GduDevice *device);
const char *gdu_device_linux_lvm2_lv_get_group_name (GduDevice *device);
const char *gdu_device_linux_lvm2_lv_get_group_uuid (GduDevice *device);

const char *gdu_device_linux_lvm2_pv_get_uuid (GduDevice *device);
guint       gdu_device_linux_lvm2_pv_get_num_metadata_areas (GduDevice *device);
const char *gdu_device_linux_lvm2_pv_get_group_name (GduDevice *device);
const char *gdu_device_linux_lvm2_pv_get_group_uuid (GduDevice *device);
guint64     gdu_device_linux_lvm2_pv_get_group_size (GduDevice *device);
guint64     gdu_device_linux_lvm2_pv_get_group_unallocated_size (GduDevice *device);
guint64     gdu_device_linux_lvm2_pv_get_group_extent_size (GduDevice *device);
guint64     gdu_device_linux_lvm2_pv_get_group_sequence_number (GduDevice *device);
gchar     **gdu_device_linux_lvm2_pv_get_group_physical_volumes (GduDevice *device);
gchar     **gdu_device_linux_lvm2_pv_get_group_logical_volumes (GduDevice *device);

const char *gdu_device_linux_dmmp_component_get_holder (GduDevice *device);
const char *gdu_device_linux_dmmp_get_name (GduDevice *device);
char **gdu_device_linux_dmmp_get_slaves (GduDevice *device);
const char *gdu_device_linux_dmmp_get_parameters (GduDevice *device);

const char *gdu_device_linux_loop_get_filename (GduDevice *device);

gboolean      gdu_device_drive_ata_smart_get_is_available (GduDevice *device);
guint64       gdu_device_drive_ata_smart_get_time_collected (GduDevice *device);
const gchar  *gdu_device_drive_ata_smart_get_status (GduDevice *device);
gconstpointer gdu_device_drive_ata_smart_get_blob (GduDevice *device, gsize *out_size);

/* ---------------------------------------------------------------------------------------------------- */

gboolean gdu_device_should_ignore (GduDevice *device);

/* ---------------------------------------------------------------------------------------------------- */

void gdu_device_op_filesystem_mount                   (GduDevice                             *device,
                                                       gchar                                **options,
                                                       GduDeviceFilesystemMountCompletedFunc  callback,
                                                       gpointer                               user_data);

/* ---------------------------------------------------------------------------------------------------- */

void gdu_device_op_filesystem_unmount                 (GduDevice                               *device,
                                                       GduDeviceFilesystemUnmountCompletedFunc  callback,
                                                       gpointer                                 user_data);

/* ---------------------------------------------------------------------------------------------------- */

void gdu_device_op_filesystem_check                 (GduDevice                             *device,
                                                     GduDeviceFilesystemCheckCompletedFunc  callback,
                                                     gpointer                               user_data);

/* ---------------------------------------------------------------------------------------------------- */

void gdu_device_op_partition_delete        (GduDevice                             *device,
                                            GduDevicePartitionDeleteCompletedFunc  callback,
                                            gpointer                               user_data);

/* ---------------------------------------------------------------------------------------------------- */

void gdu_device_op_partition_modify        (GduDevice                             *device,
                                            const char                            *type,
                                            const char                            *label,
                                            char                                 **flags,
                                            GduDevicePartitionModifyCompletedFunc  callback,
                                            gpointer                               user_data);

/* ---------------------------------------------------------------------------------------------------- */

void gdu_device_op_partition_table_create  (GduDevice                                  *device,
                                            const char                                 *scheme,
                                            GduDevicePartitionTableCreateCompletedFunc  callback,
                                            gpointer                                    user_data);

/* ---------------------------------------------------------------------------------------------------- */

void gdu_device_op_luks_unlock       (GduDevice   *device,
                                      const char *secret,
                                      GduDeviceLuksUnlockCompletedFunc callback,
                                      gpointer user_data);

/* ---------------------------------------------------------------------------------------------------- */

void gdu_device_op_luks_lock          (GduDevice                           *device,
                                       GduDeviceLuksLockCompletedFunc  callback,
                                       gpointer                             user_data);

/* ---------------------------------------------------------------------------------------------------- */

void gdu_device_op_luks_change_passphrase (GduDevice   *device,
                                           const char  *old_secret,
                                           const char  *new_secret,
                                           GduDeviceLuksChangePassphraseCompletedFunc callback,
                                           gpointer user_data);

/* ---------------------------------------------------------------------------------------------------- */

void gdu_device_op_filesystem_set_label (GduDevice                                *device,
                                         const char                               *new_label,
                                         GduDeviceFilesystemSetLabelCompletedFunc  callback,
                                         gpointer                                  user_data);

/* ---------------------------------------------------------------------------------------------------- */

void gdu_device_op_drive_ata_smart_initiate_selftest      (GduDevice                                        *device,
                                                           const char                                       *test,
                                                           GduDeviceDriveAtaSmartInitiateSelftestCompletedFunc  callback,
                                                           gpointer                                          user_data);

/* ---------------------------------------------------------------------------------------------------- */

void  gdu_device_drive_ata_smart_refresh_data (GduDevice                                  *device,
                                               GduDeviceDriveAtaSmartRefreshDataCompletedFunc callback,
                                               gpointer                                    user_data);

/* ---------------------------------------------------------------------------------------------------- */

void gdu_device_op_linux_md_stop     (GduDevice                         *device,
                                      GduDeviceLinuxMdStopCompletedFunc  callback,
                                      gpointer                           user_data);

/* ---------------------------------------------------------------------------------------------------- */

void gdu_device_op_linux_md_check    (GduDevice                           *device,
                                      gchar                              **options,
                                      GduDeviceLinuxMdCheckCompletedFunc   callback,
                                      gpointer                             user_data);

/* ---------------------------------------------------------------------------------------------------- */

void gdu_device_op_linux_md_add_spare (GduDevice                             *device,
                                       const char                            *component_objpath,
                                       GduDeviceLinuxMdAddSpareCompletedFunc  callback,
                                       gpointer                               user_data);

/* ---------------------------------------------------------------------------------------------------- */

void gdu_device_op_linux_md_expand (GduDevice                           *device,
                                    GPtrArray                           *component_objpaths,
                                    GduDeviceLinuxMdExpandCompletedFunc  callback,
                                    gpointer                             user_data);

/* ---------------------------------------------------------------------------------------------------- */

void gdu_device_op_linux_md_remove_component (GduDevice                                    *device,
                                              const char                                   *component_objpath,
                                              GduDeviceLinuxMdRemoveComponentCompletedFunc  callback,
                                              gpointer                                      user_data);

/* ---------------------------------------------------------------------------------------------------- */

void gdu_device_op_filesystem_create (GduDevice                              *device,
                                      const char                             *fstype,
                                      const char                             *fslabel,
                                      const char                             *encrypt_passphrase,
                                      gboolean                                fs_take_ownership,
                                      GduDeviceFilesystemCreateCompletedFunc  callback,
                                      gpointer                                user_data);

/* ---------------------------------------------------------------------------------------------------- */

void gdu_device_op_partition_create       (GduDevice   *device,
                                           guint64      offset,
                                           guint64      size,
                                           const char  *type,
                                           const char  *label,
                                           char       **flags,
                                           const char  *fstype,
                                           const char  *fslabel,
                                           const char  *encrypt_passphrase,
                                           gboolean     fs_take_ownership,
                                           GduDevicePartitionCreateCompletedFunc callback,
                                           gpointer user_data);

/* ---------------------------------------------------------------------------------------------------- */

void gdu_device_op_cancel_job (GduDevice *device,
                               GduDeviceCancelJobCompletedFunc callback,
                               gpointer user_data);

/* ---------------------------------------------------------------------------------------------------- */

void gdu_device_filesystem_list_open_files (GduDevice                                     *device,
                                            GduDeviceFilesystemListOpenFilesCompletedFunc  callback,
                                            gpointer                                       user_data);

GList *gdu_device_filesystem_list_open_files_sync (GduDevice  *device,
                                                   GError    **error);

/* ---------------------------------------------------------------------------------------------------- */

void gdu_device_op_drive_eject                 (GduDevice                        *device,
                                                GduDeviceDriveEjectCompletedFunc  callback,
                                                gpointer                          user_data);

/* ---------------------------------------------------------------------------------------------------- */

void gdu_device_op_drive_detach                (GduDevice                        *device,
                                                GduDeviceDriveDetachCompletedFunc callback,
                                                gpointer                          user_data);

/* ---------------------------------------------------------------------------------------------------- */

void gdu_device_op_drive_poll_media                 (GduDevice                        *device,
                                                     GduDeviceDrivePollMediaCompletedFunc   callback,
                                                     gpointer                          user_data);

/* ---------------------------------------------------------------------------------------------------- */

void gdu_device_op_drive_benchmark (GduDevice                             *device,
                                    gboolean                               do_write_benchmark,
                                    const gchar* const *                   options,
                                    GduDeviceDriveBenchmarkCompletedFunc   callback,
                                    gpointer                               user_data);

/* ---------------------------------------------------------------------------------------------------- */

void gdu_device_op_linux_lvm2_lv_stop     (GduDevice                             *device,
                                           GduDeviceLinuxLvm2LVStopCompletedFunc  callback,
                                           gpointer                               user_data);

G_END_DECLS

#endif /* __GDU_DEVICE_H */
