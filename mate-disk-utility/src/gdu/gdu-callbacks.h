/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-callbacks.h
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

#ifndef __GDU_CALLBACKS_H
#define __GDU_CALLBACKS_H

#include <gdu/gdu-types.h>

G_BEGIN_DECLS

/**
 * SECTION:gdu-callbacks
 * @title: Callbacks
 * @short_description: Callback function types
 *
 * Various callback function signatures.
 **/

/* ---------------------------------------------------------------------------------------------------- */
/* GduDevice */

typedef void (*GduDeviceFilesystemMountCompletedFunc) (GduDevice    *device,
                                                       char         *mount_point,
                                                       GError       *error,
                                                       gpointer      user_data);

typedef void (*GduDeviceFilesystemUnmountCompletedFunc) (GduDevice    *device,
                                                         GError       *error,
                                                         gpointer      user_data);

typedef void (*GduDeviceFilesystemCheckCompletedFunc) (GduDevice    *device,
                                                       gboolean      is_clean,
                                                       GError       *error,
                                                       gpointer      user_data);

typedef void (*GduDevicePartitionDeleteCompletedFunc) (GduDevice    *device,
                                                       GError       *error,
                                                       gpointer      user_data);

typedef void (*GduDevicePartitionModifyCompletedFunc) (GduDevice    *device,
                                                       GError       *error,
                                                       gpointer      user_data);

typedef void (*GduDevicePartitionTableCreateCompletedFunc) (GduDevice    *device,
                                                            GError       *error,
                                                            gpointer      user_data);

typedef void (*GduDeviceLuksUnlockCompletedFunc) (GduDevice  *device,
                                                  char       *object_path_of_cleartext_device,
                                                  GError     *error,
                                                  gpointer    user_data);

typedef void (*GduDeviceLuksLockCompletedFunc) (GduDevice    *device,
                                                GError       *error,
                                                gpointer      user_data);

typedef void (*GduDeviceLuksChangePassphraseCompletedFunc) (GduDevice  *device,
                                                            GError     *error,
                                                            gpointer    user_data);

typedef void (*GduDeviceFilesystemSetLabelCompletedFunc) (GduDevice    *device,
                                                          GError       *error,
                                                          gpointer      user_data);

typedef void (*GduDeviceDriveAtaSmartInitiateSelftestCompletedFunc) (GduDevice    *device,
                                                                     GError       *error,
                                                                     gpointer      user_data);

typedef void (*GduDeviceDriveAtaSmartRefreshDataCompletedFunc) (GduDevice  *device,
                                                                GError     *error,
                                                                gpointer    user_data);

typedef void (*GduDeviceLinuxMdStopCompletedFunc) (GduDevice    *device,
                                                   GError       *error,
                                                   gpointer      user_data);

typedef void (*GduDeviceLinuxMdCheckCompletedFunc) (GduDevice    *device,
                                                    guint         num_errors,
                                                    GError       *error,
                                                    gpointer      user_data);

typedef void (*GduDeviceLinuxMdAddSpareCompletedFunc) (GduDevice    *device,
                                                       GError       *error,
                                                       gpointer      user_data);

typedef void (*GduDeviceLinuxMdExpandCompletedFunc) (GduDevice    *device,
                                                     GError       *error,
                                                     gpointer      user_data);

typedef void (*GduDeviceLinuxMdRemoveComponentCompletedFunc) (GduDevice    *device,
                                                              GError       *error,
                                                              gpointer      user_data);

typedef void (*GduDeviceFilesystemCreateCompletedFunc) (GduDevice  *device,
                                                        GError     *error,
                                                        gpointer    user_data);

typedef void (*GduDevicePartitionCreateCompletedFunc) (GduDevice  *device,
                                                       char       *created_device_object_path,
                                                       GError     *error,
                                                       gpointer    user_data);

typedef void (*GduDeviceCancelJobCompletedFunc) (GduDevice  *device,
                                                 GError     *error,
                                                 gpointer    user_data);

typedef void (*GduDeviceFilesystemListOpenFilesCompletedFunc) (GduDevice    *device,
                                                               GList        *processes,
                                                               GError       *error,
                                                               gpointer      user_data);


typedef void (*GduDeviceDriveEjectCompletedFunc) (GduDevice    *device,
                                                  GError       *error,
                                                  gpointer      user_data);

typedef void (*GduDeviceDriveDetachCompletedFunc) (GduDevice    *device,
                                                   GError       *error,
                                                   gpointer      user_data);

typedef void (*GduDeviceDrivePollMediaCompletedFunc) (GduDevice    *device,
                                                      GError       *error,
                                                      gpointer      user_data);

typedef void (*GduDeviceDriveBenchmarkCompletedFunc) (GduDevice    *device,
                                                      GPtrArray    *read_transfer_rate_results,
                                                      GPtrArray    *write_transfer_rate_results,
                                                      GPtrArray    *access_time_results,
                                                      GError       *error,
                                                      gpointer      user_data);

typedef void (*GduDeviceLinuxLvm2LVStopCompletedFunc) (GduDevice  *device,
                                                       GError     *error,
                                                       gpointer    user_data);

/* ---------------------------------------------------------------------------------------------------- */
/* GduPool */

typedef void (*GduPoolLinuxMdStartCompletedFunc) (GduPool    *pool,
                                                  char       *assembled_array_object_path,
                                                  GError     *error,
                                                  gpointer    user_data);

typedef void (*GduPoolLinuxMdCreateCompletedFunc) (GduPool    *pool,
                                                   char       *array_object_path,
                                                   GError     *error,
                                                   gpointer    user_data);

typedef void (*GduPoolLinuxLvm2VGStartCompletedFunc) (GduPool    *pool,
                                                      GError     *error,
                                                      gpointer    user_data);

typedef void (*GduPoolLinuxLvm2VGStopCompletedFunc) (GduPool    *pool,
                                                     GError     *error,
                                                     gpointer    user_data);

typedef void (*GduPoolLinuxLvm2LVStartCompletedFunc) (GduPool    *pool,
                                                      GError     *error,
                                                      gpointer    user_data);

typedef void (*GduPoolLinuxLvm2VGSetNameCompletedFunc) (GduPool    *pool,
                                                        GError     *error,
                                                        gpointer    user_data);

typedef void (*GduPoolLinuxLvm2LVSetNameCompletedFunc) (GduPool    *pool,
                                                        GError     *error,
                                                        gpointer    user_data);

typedef void (*GduPoolLinuxLvm2LVRemoveCompletedFunc) (GduPool    *pool,
                                                       GError     *error,
                                                       gpointer    user_data);

typedef void (*GduPoolLinuxLvm2LVCreateCompletedFunc) (GduPool    *pool,
                                                       char       *create_logical_volume_object_path,
                                                       GError     *error,
                                                       gpointer    user_data);

typedef void (*GduPoolLinuxLvm2VGAddPVCompletedFunc) (GduPool    *pool,
                                                      GError     *error,
                                                      gpointer    user_data);

typedef void (*GduPoolLinuxLvm2VGRemovePVCompletedFunc) (GduPool    *pool,
                                                         GError     *error,
                                                         gpointer    user_data);

/* ---------------------------------------------------------------------------------------------------- */
/* GduDrive */

typedef void (*GduDriveActivateFunc) (GduDrive  *drive,
                                      char      *assembled_drive_object_path,
                                      GError    *error,
                                      gpointer   user_data);

typedef void (*GduDriveDeactivateFunc) (GduDrive  *drive,
                                        GError    *error,
                                        gpointer   user_data);


/* ---------------------------------------------------------------------------------------------------- */

G_END_DECLS

#endif /* __GDU_CALLBACKS_H */
