/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-section-linux-md-drive.h
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

#include <gtk/gtk.h>
#include "gdu-section.h"

#ifndef GDU_SECTION_LINUX_LVM2_VOLUME_GROUP_H
#define GDU_SECTION_LINUX_LVM2_VOLUME_GROUP_H

#define GDU_TYPE_SECTION_LINUX_LVM2_VOLUME_GROUP             (gdu_section_linux_lvm2_volume_group_get_type ())
#define GDU_SECTION_LINUX_LVM2_VOLUME_GROUP(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GDU_TYPE_SECTION_LINUX_LVM2_VOLUME_GROUP, GduSectionLinuxLvm2VolumeGroup))
#define GDU_SECTION_LINUX_LVM2_VOLUME_GROUP_CLASS(obj)       (G_TYPE_CHECK_CLASS_CAST ((obj), GDU_SECTION_LINUX_LVM2_VOLUME_GROUP,  GduSectionLinuxLvm2VolumeGroupClass))
#define GDU_IS_SECTION_LINUX_LVM2_VOLUME_GROUP(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDU_TYPE_SECTION_LINUX_LVM2_VOLUME_GROUP))
#define GDU_IS_SECTION_LINUX_LVM2_VOLUME_GROUP_CLASS(obj)    (G_TYPE_CHECK_CLASS_TYPE ((obj), GDU_TYPE_SECTION_LINUX_LVM2_VOLUME_GROUP))
#define GDU_SECTION_LINUX_LVM2_VOLUME_GROUP_GET_CLASS        (G_TYPE_INSTANCE_GET_CLASS ((obj), GDU_TYPE_SECTION_LINUX_LVM2_VOLUME_GROUP, GduSectionLinuxLvm2VolumeGroupClass))

typedef struct _GduSectionLinuxLvm2VolumeGroupClass       GduSectionLinuxLvm2VolumeGroupClass;
typedef struct _GduSectionLinuxLvm2VolumeGroup            GduSectionLinuxLvm2VolumeGroup;

struct _GduSectionLinuxLvm2VolumeGroupPrivate;
typedef struct _GduSectionLinuxLvm2VolumeGroupPrivate     GduSectionLinuxLvm2VolumeGroupPrivate;

struct _GduSectionLinuxLvm2VolumeGroup
{
        GduSection parent;

        /* private */
        GduSectionLinuxLvm2VolumeGroupPrivate *priv;
};

struct _GduSectionLinuxLvm2VolumeGroupClass
{
        GduSectionClass parent_class;
};

GType            gdu_section_linux_lvm2_volume_group_get_type (void);
GtkWidget       *gdu_section_linux_lvm2_volume_group_new      (GduShell       *shell,
                                                               GduPresentable *presentable);

#endif /* GDU_SECTION_LINUX_LVM2_VOLUME_GROUP_H */
