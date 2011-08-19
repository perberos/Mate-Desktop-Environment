/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-private.h
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

#if defined (__GDU_INSIDE_GDU_H)
#error "Can't include a private header in the public header file."
#endif

#ifndef __GDU_PRIVATE_H
#define __GDU_PRIVATE_H

#include "gdu-types.h"

#define ATA_SMART_ATTRIBUTE_STRUCT_TYPE (dbus_g_type_get_struct ("GValueArray", \
                                                                 G_TYPE_UINT, \
                                                                 G_TYPE_STRING, \
                                                                 G_TYPE_UINT, \
                                                                 G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, \
                                                                 G_TYPE_UCHAR, G_TYPE_BOOLEAN, \
                                                                 G_TYPE_UCHAR, G_TYPE_BOOLEAN, \
                                                                 G_TYPE_UCHAR, G_TYPE_BOOLEAN, \
                                                                 G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, \
                                                                 G_TYPE_UINT, G_TYPE_UINT64, \
                                                                 dbus_g_type_get_collection ("GArray", G_TYPE_UCHAR), \
                                                                 G_TYPE_INVALID))

#define ATA_SMART_HISTORICAL_DATA_STRUCT_TYPE (dbus_g_type_get_struct ("GValueArray",   \
                                                                       G_TYPE_UINT64, \
                                                                       G_TYPE_BOOLEAN, \
                                                                       G_TYPE_BOOLEAN, \
                                                                       G_TYPE_BOOLEAN, \
                                                                       G_TYPE_BOOLEAN, \
                                                                       G_TYPE_DOUBLE, \
                                                                       G_TYPE_UINT64, \
                                                                       dbus_g_type_get_collection ("GPtrArray", ATA_SMART_ATTRIBUTE_STRUCT_TYPE), \
                                                                       G_TYPE_INVALID))

#define KNOWN_FILESYSTEMS_STRUCT_TYPE (dbus_g_type_get_struct ("GValueArray",   \
                                                               G_TYPE_STRING, \
                                                               G_TYPE_STRING, \
                                                               G_TYPE_BOOLEAN, \
                                                               G_TYPE_BOOLEAN, \
                                                               G_TYPE_BOOLEAN, \
                                                               G_TYPE_UINT, \
                                                               G_TYPE_BOOLEAN, \
                                                               G_TYPE_BOOLEAN, \
                                                               G_TYPE_BOOLEAN, \
                                                               G_TYPE_BOOLEAN, \
                                                               G_TYPE_BOOLEAN, \
                                                               G_TYPE_BOOLEAN, \
                                                               G_TYPE_BOOLEAN, \
                                                               G_TYPE_BOOLEAN, \
                                                               G_TYPE_INVALID))

#define PROCESS_STRUCT_TYPE (dbus_g_type_get_struct ("GValueArray",   \
                                                     G_TYPE_UINT,     \
                                                     G_TYPE_UINT,     \
                                                     G_TYPE_STRING,   \
                                                     G_TYPE_INVALID))

DBusGConnection *_gdu_pool_get_connection (GduPool *pool);

GduKnownFilesystem    *_gdu_known_filesystem_new       (gpointer data);

GduProcess            * _gdu_process_new               (gpointer data);

void _gdu_error_fixup (GError *error);

GduDevice  *_gdu_device_new_from_object_path  (GduPool     *pool, const char  *object_path);

GduVolume   *_gdu_volume_new_from_device      (GduPool *pool, GduDevice *volume, GduPresentable *enclosing_presentable);
GduDrive    *_gdu_drive_new_from_device       (GduPool *pool, GduDevice *drive, GduPresentable *enclosing_presentable);
GduVolumeHole   *_gdu_volume_hole_new       (GduPool *pool, guint64 offset, guint64 size, GduPresentable *enclosing_presentable);


GduLinuxMdDrive   *_gdu_linux_md_drive_new             (GduPool              *pool,
                                                        const gchar          *uuid,
                                                        const gchar          *device_file,
                                                        GduPresentable *enclosing_presentable);

gboolean _gdu_linux_md_drive_has_uuid (GduLinuxMdDrive  *drive,
                                       const gchar      *uuid);


gboolean    _gdu_device_changed               (GduDevice   *device);
void        _gdu_device_job_changed           (GduDevice   *device,
                                               gboolean     job_in_progress,
                                               const char  *job_id,
                                               uid_t        job_initiated_by_uid,
                                               gboolean     job_is_cancellable,
                                               double       job_percentage);

GduAdapter *_gdu_adapter_new_from_object_path (GduPool *pool, const char *object_path);
gboolean    _gdu_adapter_changed              (GduAdapter   *adapter);

GduExpander *_gdu_expander_new_from_object_path (GduPool *pool, const char *object_path);
gboolean    _gdu_expander_changed               (GduExpander   *expander);

GduHub     *_gdu_hub_new                        (GduPool        *pool,
                                                 GduHubUsage     usage,
                                                 GduAdapter     *adapter,
                                                 GduExpander    *expander,
                                                 const gchar    *name,
                                                 const gchar    *vpd_name,
                                                 GIcon          *icon,
                                                 GduPresentable *enclosing_presentable);

GduPort    *_gdu_port_new_from_object_path (GduPool *pool, const char *object_path);
gboolean    _gdu_port_changed               (GduPort   *port);

GduMachine *_gdu_machine_new (GduPool *pool);

GduLinuxLvm2VolumeGroup *
_gdu_linux_lvm2_volume_group_new (GduPool        *pool,
                                  const gchar    *vg_uuid,
                                  GduPresentable *enclosing_presentable);

void _gdu_linux_lvm2_volume_group_rewrite_enclosing_presentable (GduLinuxLvm2VolumeGroup *vg);

GduLinuxLvm2Volume *_gdu_linux_lvm2_volume_new (GduPool        *pool,
                                                const gchar    *group_uuid,
                                                const gchar    *uuid,
                                                GduPresentable *enclosing_presentable);

void _gdu_linux_lvm2_volume_rewrite_enclosing_presentable (GduLinuxLvm2Volume *volume);

GduLinuxLvm2VolumeHole *_gdu_linux_lvm2_volume_hole_new (GduPool        *pool,
                                                         GduPresentable *enclosing_presentable);

void _gdu_linux_lvm2_volume_hole_rewrite_enclosing_presentable (GduLinuxLvm2VolumeHole *volume_hole);


void _gdu_hub_rewrite_enclosing_presentable (GduHub *hub);
void _gdu_drive_rewrite_enclosing_presentable (GduDrive *drive);
void _gdu_linux_md_drive_rewrite_enclosing_presentable (GduLinuxMdDrive *drive);
void _gdu_volume_rewrite_enclosing_presentable (GduVolume *volume);
void _gdu_volume_hole_rewrite_enclosing_presentable (GduVolumeHole *volume_hole);

gchar *_gdu_volume_get_names_and_desc (GduPresentable  *presentable,
                                       gchar          **out_vpd_name,
                                       gchar          **out_desc);

#endif /* __GDU_PRIVATE_H */
