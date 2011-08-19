/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-drive.h
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

#ifndef __GDU_DRIVE_H
#define __GDU_DRIVE_H

#include <gdu/gdu-types.h>

G_BEGIN_DECLS

#define GDU_TYPE_DRIVE         (gdu_drive_get_type ())
#define GDU_DRIVE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDU_TYPE_DRIVE, GduDrive))
#define GDU_DRIVE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GDU_DRIVE,  GduDriveClass))
#define GDU_IS_DRIVE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDU_TYPE_DRIVE))
#define GDU_IS_DRIVE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDU_TYPE_DRIVE))
#define GDU_DRIVE_GET_CLASS(k) (G_TYPE_INSTANCE_GET_CLASS ((k), GDU_TYPE_DRIVE, GduDriveClass))

typedef struct _GduDriveClass    GduDriveClass;
typedef struct _GduDrivePrivate  GduDrivePrivate;

/* TODO: move to gdu-enums.h */
/**
 * GduCreateVolumeFlags:
 * @GDU_CREATE_VOLUME_FLAGS_NONE: No flags are set.
 * @GDU_CREATE_VOLUME_FLAGS_LINUX_MD: The volume is to be used for Linux MD RAID.
 * @GDU_CREATE_VOLUME_FLAGS_LINUX_LVM2: The volume is to be used for Linux LVM2.
 *
 * Flags used in gdu_drive_create_volume().
 */
typedef enum {
        GDU_CREATE_VOLUME_FLAGS_NONE = 0x00,
        GDU_CREATE_VOLUME_FLAGS_LINUX_MD = (1<<0),
        GDU_CREATE_VOLUME_FLAGS_LINUX_LVM2 = (1<<1)
} GduCreateVolumeFlags;

struct _GduDrive
{
        GObject parent;

        /*< private >*/
        GduDrivePrivate *priv;
};

struct _GduDriveClass
{
        GObjectClass parent_class;

        /*< public >*/
        /* VTable */
        gboolean    (*is_active)             (GduDrive              *drive);
        gboolean    (*is_activatable)        (GduDrive              *drive);
        gboolean    (*can_deactivate)        (GduDrive              *drive);
        gboolean    (*can_activate)          (GduDrive              *drive,
                                              gboolean              *out_degraded);
        void        (*activate)              (GduDrive              *drive,
                                              GduDriveActivateFunc   callback,
                                              gpointer               user_data);
        void        (*deactivate)            (GduDrive              *drive,
                                              GduDriveDeactivateFunc callback,
                                              gpointer               user_data);

        gboolean   (*can_create_volume)     (GduDrive        *drive,
                                             gboolean        *out_is_uninitialized,
                                             guint64         *out_largest_contiguous_free_segment,
                                             guint64         *out_total_free,
                                             GduPresentable **out_presentable);

        void       (*create_volume)         (GduDrive              *drive,
                                             guint64                size,
                                             const gchar           *name,
                                             GduCreateVolumeFlags   flags,
                                             GAsyncReadyCallback    callback,
                                             gpointer               user_data);

        GduVolume *(*create_volume_finish) (GduDrive              *drive,
                                            GAsyncResult          *res,
                                            GError               **error);
};

GType       gdu_drive_get_type           (void);

gboolean    gdu_drive_is_active             (GduDrive              *drive);
gboolean    gdu_drive_is_activatable        (GduDrive              *drive);
gboolean    gdu_drive_can_deactivate        (GduDrive              *drive);
gboolean    gdu_drive_can_activate          (GduDrive              *drive,
                                             gboolean              *out_degraded);
void        gdu_drive_activate              (GduDrive              *drive,
                                             GduDriveActivateFunc   callback,
                                             gpointer               user_data);
void        gdu_drive_deactivate            (GduDrive              *drive,
                                             GduDriveDeactivateFunc callback,
                                             gpointer               user_data);


gboolean    gdu_drive_can_create_volume     (GduDrive        *drive,
                                             gboolean        *out_is_uninitialized,
                                             guint64         *out_largest_contiguous_free_segment,
                                             guint64         *out_total_free,
                                             GduPresentable **out_presentable);

void        gdu_drive_create_volume         (GduDrive              *drive,
                                             guint64                size,
                                             const gchar           *name,
                                             GduCreateVolumeFlags   flags,
                                             GAsyncReadyCallback    callback,
                                             gpointer               user_data);

GduVolume  *gdu_drive_create_volume_finish  (GduDrive              *drive,
                                             GAsyncResult          *res,
                                             GError               **error);

gboolean    gdu_drive_count_mbr_partitions  (GduDrive        *drive,
                                             guint           *out_num_primary_partitions,
                                             gboolean        *out_has_extended_partition);

GList      *gdu_drive_get_volumes           (GduDrive  *drive);

G_END_DECLS

#endif /* __GDU_DRIVE_H */
