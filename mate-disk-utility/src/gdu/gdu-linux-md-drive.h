/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-linux-md-drive.h
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

#ifndef __GDU_LINUX_MD_DRIVE_H
#define __GDU_LINUX_MD_DRIVE_H

#include <gdu/gdu-types.h>
#include <gdu/gdu-callbacks.h>
#include <gdu/gdu-drive.h>

G_BEGIN_DECLS

#define GDU_TYPE_LINUX_MD_DRIVE         (gdu_linux_md_drive_get_type ())
#define GDU_LINUX_MD_DRIVE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDU_TYPE_LINUX_MD_DRIVE, GduLinuxMdDrive))
#define GDU_LINUX_MD_DRIVE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GDU_LINUX_MD_DRIVE,  GduLinuxMdDriveClass))
#define GDU_IS_LINUX_MD_DRIVE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDU_TYPE_LINUX_MD_DRIVE))
#define GDU_IS_LINUX_MD_DRIVE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDU_TYPE_LINUX_MD_DRIVE))
#define GDU_LINUX_MD_DRIVE_GET_CLASS(k) (G_TYPE_INSTANCE_GET_CLASS ((k), GDU_TYPE_LINUX_MD_DRIVE, GduLinuxMdDriveClass))

typedef struct _GduLinuxMdDriveClass    GduLinuxMdDriveClass;
typedef struct _GduLinuxMdDrivePrivate  GduLinuxMdDrivePrivate;

struct _GduLinuxMdDrive
{
        GduDrive parent;

        /*< private >*/
        GduLinuxMdDrivePrivate *priv;
};

struct _GduLinuxMdDriveClass
{
        GduDriveClass parent_class;
};

/**
 * GduLinuxMdDriveSlaveFlags:
 * @GDU_LINUX_MD_DRIVE_SLAVE_FLAGS_NONE: No flags are set.
 * @GDU_LINUX_MD_DRIVE_SLAVE_FLAGS_NOT_ATTACHED: If set, the slave is
 * not part of the array but appears as a child only because the UUID
 * on the device matches that of the array. Is also set if the array
 * does not exist.
 * @GDU_LINUX_MD_DRIVE_SLAVE_FLAGS_FAULTY: Device has been kick from
 * active use due to a detected fault.
 * @GDU_LINUX_MD_DRIVE_SLAVE_FLAGS_IN_SYNC: Device is a fully in-sync
 * member of the array.
 * @GDU_LINUX_MD_DRIVE_SLAVE_FLAGS_WRITEMOSTLY: Device will only be
 * subject to read requests if there are no other options. This
 * applies only to RAID1 arrays.
 * @GDU_LINUX_MD_DRIVE_SLAVE_FLAGS_BLOCKED: Device has failed,
 * metadata is "external", and the failure hasn't been acknowledged
 * yet. Writes that would write to this device if it were not faulty
 * are blocked.
 * @GDU_LINUX_MD_DRIVE_SLAVE_FLAGS_SPARE: Device is working, but not a
 * full member. This includes spares that in the process of being
 * recovered to.
 *
 * State for slaves of an Linux MD software raid drive. Everything but @GDU_LINUX_MD_DRIVE_SLAVE_FLAGS_NONE
 * and @GDU_LINUX_MD_DRIVE_SLAVE_FLAGS_NOT_ATTACHED corresponds to the comma-separated strings in
 * <literal>/sys/block/mdXXX/md/dev-YYY/state</literal> in sysfs. See Documentation/md.txt in the Linux
 * kernel for more information.
 **/
typedef enum {
        GDU_LINUX_MD_DRIVE_SLAVE_FLAGS_NONE          = 0,
        GDU_LINUX_MD_DRIVE_SLAVE_FLAGS_NOT_ATTACHED  = (1<<0),
        GDU_LINUX_MD_DRIVE_SLAVE_FLAGS_FAULTY        = (1<<1),
        GDU_LINUX_MD_DRIVE_SLAVE_FLAGS_IN_SYNC       = (1<<2),
        GDU_LINUX_MD_DRIVE_SLAVE_FLAGS_WRITEMOSTLY   = (1<<3),
        GDU_LINUX_MD_DRIVE_SLAVE_FLAGS_BLOCKED       = (1<<4),
        GDU_LINUX_MD_DRIVE_SLAVE_FLAGS_SPARE         = (1<<5),
} GduLinuxMdDriveSlaveFlags;

GType                      gdu_linux_md_drive_get_type               (void);
const gchar               *gdu_linux_md_drive_get_uuid               (GduLinuxMdDrive  *drive);
gboolean                   gdu_linux_md_drive_has_slave              (GduLinuxMdDrive  *drive,
                                                                      GduDevice        *device);
GList                     *gdu_linux_md_drive_get_slaves             (GduLinuxMdDrive  *drive);
GduLinuxMdDriveSlaveFlags  gdu_linux_md_drive_get_slave_flags        (GduLinuxMdDrive  *drive,
                                                                      GduDevice        *slave);
gchar                     *gdu_linux_md_drive_get_slave_state_markup (GduLinuxMdDrive  *drive,
                                                                      GduDevice        *slave);

G_END_DECLS

#endif /* __GDU_LINUX_MD_DRIVE_H */
