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

#if !defined (__GDU_INSIDE_GDU_H) && !defined (GDU_COMPILATION)
#error "Only <gdu/gdu.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef __GDU_LINUX_LVM2_VOLUME_GROUP_H
#define __GDU_LINUX_LVM2_VOLUME_GROUP_H

#include <gdu/gdu-types.h>
#include <gdu/gdu-callbacks.h>
#include <gdu/gdu-drive.h>

G_BEGIN_DECLS

#define GDU_TYPE_LINUX_LVM2_VOLUME_GROUP         (gdu_linux_lvm2_volume_group_get_type ())
#define GDU_LINUX_LVM2_VOLUME_GROUP(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDU_TYPE_LINUX_LVM2_VOLUME_GROUP, GduLinuxLvm2VolumeGroup))
#define GDU_LINUX_LVM2_VOLUME_GROUP_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GDU_LINUX_LVM2_VOLUME_GROUP,  GduLinuxLvm2VolumeGroupClass))
#define GDU_IS_LINUX_LVM2_VOLUME_GROUP(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDU_TYPE_LINUX_LVM2_VOLUME_GROUP))
#define GDU_IS_LINUX_LVM2_VOLUME_GROUP_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDU_TYPE_LINUX_LVM2_VOLUME_GROUP))
#define GDU_LINUX_LVM2_VOLUME_GROUP_GET_CLASS(k) (G_TYPE_INSTANCE_GET_CLASS ((k), GDU_TYPE_LINUX_LVM2_VOLUME_GROUP, GduLinuxLvm2VolumeGroupClass))

typedef struct _GduLinuxLvm2VolumeGroupClass    GduLinuxLvm2VolumeGroupClass;
typedef struct _GduLinuxLvm2VolumeGroupPrivate  GduLinuxLvm2VolumeGroupPrivate;

struct _GduLinuxLvm2VolumeGroup
{
        GduDrive parent;

        /*< private >*/
        GduLinuxLvm2VolumeGroupPrivate *priv;
};

struct _GduLinuxLvm2VolumeGroupClass
{
        GduDriveClass parent_class;
};

typedef enum {
        GDU_LINUX_LVM2_VOLUME_GROUP_STATE_NOT_RUNNING,
        GDU_LINUX_LVM2_VOLUME_GROUP_STATE_PARTIALLY_RUNNING,
        GDU_LINUX_LVM2_VOLUME_GROUP_STATE_RUNNING,
} GduLinuxLvm2VolumeGroupState;

GType                         gdu_linux_lvm2_volume_group_get_type      (void);
const gchar                  *gdu_linux_lvm2_volume_group_get_uuid      (GduLinuxLvm2VolumeGroup  *vg);
GduLinuxLvm2VolumeGroupState  gdu_linux_lvm2_volume_group_get_state     (GduLinuxLvm2VolumeGroup  *vg);
GduDevice                    *gdu_linux_lvm2_volume_group_get_pv_device (GduLinuxLvm2VolumeGroup  *vg);
guint                         gdu_linux_lvm2_volume_group_get_num_lvs   (GduLinuxLvm2VolumeGroup  *vg);
gboolean                      gdu_linux_lvm2_volume_group_get_lv_info   (GduLinuxLvm2VolumeGroup  *vg,
                                                                         const gchar              *lv_uuid,
                                                                         guint                    *out_position,
                                                                         gchar                   **out_name,
                                                                         guint64                  *out_size);
gboolean                      gdu_linux_lvm2_volume_group_get_pv_info   (GduLinuxLvm2VolumeGroup  *vg,
                                                                         const gchar              *pv_uuid,
                                                                         guint                    *out_position,
                                                                         guint64                  *out_size,
                                                                         guint64                  *out_allocated_size);
gchar                        *gdu_linux_lvm2_volume_group_get_compute_new_lv_name (GduLinuxLvm2VolumeGroup  *vg);

G_END_DECLS

#endif /* __GDU_LINUX_LVM2_VOLUME_GROUP_H */
