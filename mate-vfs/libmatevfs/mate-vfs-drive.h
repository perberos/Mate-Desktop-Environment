/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-drive.h - Handling of drives for the MATE Virtual File System.

   Copyright (C) 2003 Red Hat, Inc

   The Mate Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Alexander Larsson <alexl@redhat.com>
*/

#ifndef MATE_VFS_DRIVE_H
#define MATE_VFS_DRIVE_H

#include <glib-object.h>
#include <libmatevfs/mate-vfs-volume.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATE_VFS_TYPE_DRIVE        (mate_vfs_drive_get_type ())
#define MATE_VFS_DRIVE(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), MATE_VFS_TYPE_DRIVE, MateVFSDrive))
#define MATE_VFS_DRIVE_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), MATE_VFS_TYPE_DRIVE, MateVFSDriveClass))
#define MATE_IS_VFS_DRIVE(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), MATE_VFS_TYPE_DRIVE))
#define MATE_IS_VFS_DRIVE_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), MATE_VFS_TYPE_DRIVE))

typedef struct _MateVFSDrivePrivate MateVFSDrivePrivate;

struct _MateVFSDrive {
	GObject parent;

	/*< private >*/
        MateVFSDrivePrivate *priv;
};

typedef struct _MateVFSDriveClass MateVFSDriveClass;

struct _MateVFSDriveClass {
	GObjectClass parent_class;

	void (* volume_mounted)	  	(MateVFSDrive *drive,
				   	 MateVFSVolume	*volume);
	void (* volume_pre_unmount)	(MateVFSDrive *drive,
				   	 MateVFSVolume	*volume);
	void (* volume_unmounted)	(MateVFSDrive *drive,
				   	 MateVFSVolume	*volume);
};

GType mate_vfs_drive_get_type (void) G_GNUC_CONST;

MateVFSDrive *mate_vfs_drive_ref   (MateVFSDrive *drive);
void           mate_vfs_drive_unref (MateVFSDrive *drive);
void           mate_vfs_drive_volume_list_free (GList *volumes);


gulong             mate_vfs_drive_get_id              (MateVFSDrive *drive);
MateVFSDeviceType mate_vfs_drive_get_device_type     (MateVFSDrive *drive);

#ifndef MATE_VFS_DISABLE_DEPRECATED
MateVFSVolume *   mate_vfs_drive_get_mounted_volume  (MateVFSDrive *drive);
#endif /*MATE_VFS_DISABLE_DEPRECATED*/

GList *            mate_vfs_drive_get_mounted_volumes (MateVFSDrive *drive);
char *             mate_vfs_drive_get_device_path     (MateVFSDrive *drive);
char *             mate_vfs_drive_get_activation_uri  (MateVFSDrive *drive);
char *             mate_vfs_drive_get_display_name    (MateVFSDrive *drive);
char *             mate_vfs_drive_get_icon            (MateVFSDrive *drive);
char *             mate_vfs_drive_get_hal_udi         (MateVFSDrive *drive);
gboolean           mate_vfs_drive_is_user_visible     (MateVFSDrive *drive);
gboolean           mate_vfs_drive_is_connected        (MateVFSDrive *drive);
gboolean           mate_vfs_drive_is_mounted          (MateVFSDrive *drive);
gboolean           mate_vfs_drive_needs_eject         (MateVFSDrive *drive);

gint               mate_vfs_drive_compare             (MateVFSDrive *a,
							MateVFSDrive *b);

void mate_vfs_drive_mount   (MateVFSDrive             *drive,
			      MateVFSVolumeOpCallback   callback,
			      gpointer                   user_data);
void mate_vfs_drive_unmount (MateVFSDrive             *drive,
			      MateVFSVolumeOpCallback   callback,
			      gpointer                   user_data);
void mate_vfs_drive_eject   (MateVFSDrive             *drive,
			      MateVFSVolumeOpCallback   callback,
			      gpointer                   user_data);

#ifdef __cplusplus
}
#endif
#endif /* MATE_VFS_DRIVE_H */

