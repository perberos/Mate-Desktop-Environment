/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-volume.h - Handling of volumes for the MATE Virtual File System.

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

#ifndef MATE_VFS_VOLUME_H
#define MATE_VFS_VOLUME_H

#include <sys/types.h>
#include <sys/stat.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATE_VFS_TYPE_VOLUME        (mate_vfs_volume_get_type ())
#define MATE_VFS_VOLUME(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), MATE_VFS_TYPE_VOLUME, MateVFSVolume))
#define MATE_VFS_VOLUME_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), MATE_VFS_TYPE_VOLUME, MateVFSVolumeClass))
#define MATE_IS_VFS_VOLUME(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), MATE_VFS_TYPE_VOLUME))
#define MATE_IS_VFS_VOLUME_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), MATE_VFS_TYPE_VOLUME))

typedef struct _MateVFSVolumePrivate MateVFSVolumePrivate;

typedef struct _MateVFSVolume MateVFSVolume;

struct _MateVFSVolume {
	GObject parent;

	/*< private >*/
        MateVFSVolumePrivate *priv;
};

typedef struct _MateVFSVolumeClass MateVFSVolumeClass;

struct _MateVFSVolumeClass {
	GObjectClass parent_class;
};

/**
 * MateVFSDeviceType:
 * @MATE_VFS_DEVICE_TYPE_UNKNOWN: the type of this #MateVFSVolume or #MateVFSDrive is not known.
 * @MATE_VFS_DEVICE_TYPE_AUDIO_CD: only used for #MateVFSVolume objects. Denotes that this
 * volume is an audio CD.
 * @MATE_VFS_DEVICE_TYPE_VIDEO_DVD: only used for #MateVFSVolume objects. Denotes that this
 * volume is a video DVD.
 * @MATE_VFS_DEVICE_TYPE_HARDDRIVE: this is a mount point refering to a harddisk partition that
 * neither has a Microsoft file system (FAT, VFAT, NTFS) nor an Apple file system (HFS, HFS+).
 * @MATE_VFS_DEVICE_TYPE_CDROM: this may either be a mount point or a HAL drive/volume. Either way,
 * it refers to a CD-ROM device respectively volume.
 * @MATE_VFS_DEVICE_TYPE_FLOPPY: the volume or drive referenced by this #MateVFSVolume or
 * #MateVFSDrive is a floppy disc respectively a floppy drive.
 * @MATE_VFS_DEVICE_TYPE_ZIP: the volume or drive referenced by this #MateVFSVolume or
 * #MateVFSDrive is a ZIP disc respectively a ZIP drive.
 * @MATE_VFS_DEVICE_TYPE_JAZ: the volume or drive referenced by this #MateVFSVolume or
 * #MateVFSDrive is a JAZ disc respectively a JAZ drive.
 * @MATE_VFS_DEVICE_TYPE_NFS: this is a mount point having an NFS file system.
 * @MATE_VFS_DEVICE_TYPE_AUTOFS: this is a mount point having an AutoFS file system.
 * @MATE_VFS_DEVICE_TYPE_CAMERA: only used for #MateVFSVolume objects. Denotes that this volume is a camera.
 * @MATE_VFS_DEVICE_TYPE_MEMORY_STICK: only used for #MateVFSVolume objects. Denotes that this volume is a memory stick.
 * @MATE_VFS_DEVICE_TYPE_SMB: this is a mount point having a Samba file system.
 * @MATE_VFS_DEVICE_TYPE_APPLE: this is a mount point refering to a harddisk partition, that has an
 * Apple file system (HFS, HFS+).
 * @MATE_VFS_DEVICE_TYPE_MUSIC_PLAYER: only used for #MateVFSVolume objects. Denotes that this
 * volume is a music player.
 * @MATE_VFS_DEVICE_TYPE_WINDOWS: this is a mount point refering to a harddisk partition, that has a
 * Microsoft file system (FAT, VFAT, NTFS).
 * @MATE_VFS_DEVICE_TYPE_LOOPBACK: this is a mount point refering to a loopback device.
 * @MATE_VFS_DEVICE_TYPE_NETWORK: only used for #MateVFSVolume objects, denoting that this volume
 * refers to a network mount that is not managed by the kernel VFS but exclusively known to MateVFS.
 *
 * Identifies the device type of a #MateVFSVolume or a #MateVFSDrive.
 **/
typedef enum {
	MATE_VFS_DEVICE_TYPE_UNKNOWN,
	MATE_VFS_DEVICE_TYPE_AUDIO_CD,
	MATE_VFS_DEVICE_TYPE_VIDEO_DVD,
	MATE_VFS_DEVICE_TYPE_HARDDRIVE,
	MATE_VFS_DEVICE_TYPE_CDROM,
	MATE_VFS_DEVICE_TYPE_FLOPPY,
	MATE_VFS_DEVICE_TYPE_ZIP,
	MATE_VFS_DEVICE_TYPE_JAZ,
	MATE_VFS_DEVICE_TYPE_NFS,
	MATE_VFS_DEVICE_TYPE_AUTOFS,
	MATE_VFS_DEVICE_TYPE_CAMERA,
	MATE_VFS_DEVICE_TYPE_MEMORY_STICK,
	MATE_VFS_DEVICE_TYPE_SMB,
	MATE_VFS_DEVICE_TYPE_APPLE,
	MATE_VFS_DEVICE_TYPE_MUSIC_PLAYER,
	MATE_VFS_DEVICE_TYPE_WINDOWS,
	MATE_VFS_DEVICE_TYPE_LOOPBACK,
	MATE_VFS_DEVICE_TYPE_NETWORK
} MateVFSDeviceType;

/**
 * @MATE_VFS_VOLUME_TYPE_MOUNTPOINT: this is a mount point managed by the kernel.
 * @MATE_VFS_VOLUME_TYPE_VFS_MOUNT: this is a special volume only known to MateVFS,
 * for instance a blank disk or an audio CD.
 * @MATE_VFS_VOLUME_TYPE_CONNECTED_SERVER: this is a special volume only known
 * MateVFS, referring to a MateVFSURI network location, for instance a location
 * on an http, an ftp or an sftp server.
 *
 * Identifies the volume type of a #MateVFSVolume.
 **/
typedef enum {
	MATE_VFS_VOLUME_TYPE_MOUNTPOINT,
	MATE_VFS_VOLUME_TYPE_VFS_MOUNT,
	MATE_VFS_VOLUME_TYPE_CONNECTED_SERVER
} MateVFSVolumeType;

/**
 * MateVFSVolumeOpCallback:
 * @succeeded: whether the volume operation succeeded
 * @error: a string identifying the error that occurred, if
 * @succeeded is %FALSE. Otherwise %NULL.
 * @detailed_error: a string more specifically identifying
 * the error that occurred, if @succeeded is %FALSE.
 * Otherwise %NULL.
 * @user_data: the user data that was passed when registering
 * the callback.
 *
 * Note that if succeeded is FALSE and error, detailed_error are both
 * empty strings the client is not supposed to display a dialog as an
 * external mount/umount/eject helper will have done so.
 *
 * Since: 2.6
 **/
typedef void (*MateVFSVolumeOpCallback) (gboolean succeeded,
					  char *error,
					  char *detailed_error,
					  gpointer user_data);


/* Need to declare this here due to cross use in mate-vfs-drive.h */
typedef struct _MateVFSDrive MateVFSDrive;

#include <libmatevfs/mate-vfs-drive.h>

GType mate_vfs_volume_get_type (void) G_GNUC_CONST;

MateVFSVolume *mate_vfs_volume_ref   (MateVFSVolume *volume);
void            mate_vfs_volume_unref (MateVFSVolume *volume);

gulong             mate_vfs_volume_get_id              (MateVFSVolume *volume);
MateVFSVolumeType mate_vfs_volume_get_volume_type     (MateVFSVolume *volume);
MateVFSDeviceType mate_vfs_volume_get_device_type     (MateVFSVolume *volume);
MateVFSDrive *    mate_vfs_volume_get_drive           (MateVFSVolume *volume);
char *             mate_vfs_volume_get_device_path     (MateVFSVolume *volume);
char *             mate_vfs_volume_get_activation_uri  (MateVFSVolume *volume);
char *             mate_vfs_volume_get_filesystem_type (MateVFSVolume *volume);
char *             mate_vfs_volume_get_display_name    (MateVFSVolume *volume);
char *             mate_vfs_volume_get_icon            (MateVFSVolume *volume);
char *             mate_vfs_volume_get_hal_udi         (MateVFSVolume *volume);
gboolean           mate_vfs_volume_is_user_visible     (MateVFSVolume *volume);
gboolean           mate_vfs_volume_is_read_only        (MateVFSVolume *volume);
gboolean           mate_vfs_volume_is_mounted          (MateVFSVolume *volume);
gboolean           mate_vfs_volume_handles_trash       (MateVFSVolume *volume);

gint               mate_vfs_volume_compare             (MateVFSVolume *a,
							 MateVFSVolume *b);


void mate_vfs_volume_unmount    (MateVFSVolume           *volume,
				  MateVFSVolumeOpCallback  callback,
				  gpointer                  user_data);
void mate_vfs_volume_eject      (MateVFSVolume           *volume,
				  MateVFSVolumeOpCallback  callback,
				  gpointer                  user_data);
void mate_vfs_connect_to_server (const char               *uri,
				  const char               *display_name,
				  const char               *icon);

#ifdef __cplusplus
}
#endif

#endif /* MATE_VFS_VOLUME_H */
