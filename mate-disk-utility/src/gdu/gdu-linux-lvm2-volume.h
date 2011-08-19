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

#ifndef __GDU_LINUX_LVM2_VOLUME_H
#define __GDU_LINUX_LVM2_VOLUME_H

#include <gdu/gdu-types.h>
#include <gdu/gdu-callbacks.h>
#include <gdu/gdu-volume.h>

G_BEGIN_DECLS

#define GDU_TYPE_LINUX_LVM2_VOLUME         (gdu_linux_lvm2_volume_get_type ())
#define GDU_LINUX_LVM2_VOLUME(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDU_TYPE_LINUX_LVM2_VOLUME, GduLinuxLvm2Volume))
#define GDU_LINUX_LVM2_VOLUME_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GDU_LINUX_LVM2_VOLUME,  GduLinuxLvm2VolumeClass))
#define GDU_IS_LINUX_LVM2_VOLUME(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDU_TYPE_LINUX_LVM2_VOLUME))
#define GDU_IS_LINUX_LVM2_VOLUME_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDU_TYPE_LINUX_LVM2_VOLUME))
#define GDU_LINUX_LVM2_VOLUME_GET_CLASS(k) (G_TYPE_INSTANCE_GET_CLASS ((k), GDU_TYPE_LINUX_LVM2_VOLUME, GduLinuxLvm2VolumeClass))

typedef struct _GduLinuxLvm2VolumeClass    GduLinuxLvm2VolumeClass;
typedef struct _GduLinuxLvm2VolumePrivate  GduLinuxLvm2VolumePrivate;

struct _GduLinuxLvm2Volume
{
        GduVolume parent;

        /*< private >*/
        GduLinuxLvm2VolumePrivate *priv;
};

struct _GduLinuxLvm2VolumeClass
{
        GduVolumeClass parent_class;
};

GType        gdu_linux_lvm2_volume_get_type       (void);
const gchar *gdu_linux_lvm2_volume_get_name       (GduLinuxLvm2Volume *volume);
const gchar *gdu_linux_lvm2_volume_get_uuid       (GduLinuxLvm2Volume *volume);
const gchar *gdu_linux_lvm2_volume_get_group_uuid (GduLinuxLvm2Volume *volume);

G_END_DECLS

#endif /* __GDU_LINUX_LVM2_VOLUME_H */
