/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-volume.c - Handling of volumes for the MATE Virtual File System.

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

#include <config.h>
#include <string.h>
#include <glib.h>

#include "mate-vfs-volume.h"
#include "mate-vfs-volume-monitor-private.h"
#include "mate-vfs-filesystem-type.h"

static void     mate_vfs_volume_class_init           (MateVFSVolumeClass *klass);
static void     mate_vfs_volume_init                 (MateVFSVolume      *volume);
static void     mate_vfs_volume_finalize             (GObject          *object);


static GObjectClass *parent_class = NULL;

G_LOCK_DEFINE_STATIC (volumes);

GType
mate_vfs_volume_get_type (void)
{
	static GType volume_type = 0;

	if (!volume_type) {
		const GTypeInfo volume_info = {
			sizeof (MateVFSVolumeClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) mate_vfs_volume_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (MateVFSVolume),
			0,              /* n_preallocs */
			(GInstanceInitFunc) mate_vfs_volume_init
		};
		
		volume_type =
			g_type_register_static (G_TYPE_OBJECT, "MateVFSVolume",
						&volume_info, 0);
	}
	
	return volume_type;
}

static void
mate_vfs_volume_class_init (MateVFSVolumeClass *class)
{
	GObjectClass *o_class;
	
	parent_class = g_type_class_peek_parent (class);
	
	o_class = (GObjectClass *) class;
	
	/* GObject signals */
	o_class->finalize = mate_vfs_volume_finalize;
}

static void
mate_vfs_volume_init (MateVFSVolume *volume)
{
	static int volume_id_counter = 1;
	
	volume->priv = g_new0 (MateVFSVolumePrivate, 1);

	G_LOCK (volumes);
	volume->priv->id = volume_id_counter++;
	G_UNLOCK (volumes);
	
}

/** 
 * mate_vfs_volume_ref:
 * @volume: a #MateVFSVolume, or %NULL.
 *
 * Increases the refcount of the @volume by 1, if it is not %NULL.
 *
 * Returns: the @volume with its refcount increased by one,
 * 	    or %NULL if @volume is NULL.
 *
 * Since: 2.6
 */
MateVFSVolume *
mate_vfs_volume_ref (MateVFSVolume *volume)
{
	if (volume == NULL) {
		return NULL;
	}
		
	G_LOCK (volumes);
	g_object_ref (volume);
	G_UNLOCK (volumes);
	return volume;
}

/** 
 * mate_vfs_volume_unref:
 * @volume: a #MateVFSVolume, or %NULL.
 *
 * Decreases the refcount of the @volume by 1, if it is not %NULL.
 *
 * Since: 2.6
 */
void
mate_vfs_volume_unref (MateVFSVolume *volume)
{
	if (volume == NULL) {
		return;
	}
	
	G_LOCK (volumes);
	g_object_unref (volume);
	G_UNLOCK (volumes);
}


/* Remeber that this could be running on a thread other
 * than the main thread */
static void
mate_vfs_volume_finalize (GObject *object)
{
	MateVFSVolume *volume = (MateVFSVolume *) object;
	MateVFSVolumePrivate *priv;
	
	priv = volume->priv;

	/* The volume can't be finalized if there is a
	   drive that owns it */
	g_assert (priv->drive == NULL);
	
	g_free (priv->device_path);
	g_free (priv->activation_uri);
	g_free (priv->filesystem_type);
	g_free (priv->display_name);
	g_free (priv->display_name_key);
	g_free (priv->icon);
	g_free (priv->mateconf_id);
	g_free (priv->hal_udi);
	g_free (priv->hal_drive_udi);
	g_free (priv);
	volume->priv = NULL;
	
	if (G_OBJECT_CLASS (parent_class)->finalize)
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}


/** 
 * mate_vfs_volume_get_volume_type:
 * @volume: a #MateVFSVolume.
 *
 * Returns the #MateVFSVolumeType of the @volume.
 *
 * Returns: the volume type for @volume.
 *
 * Since: 2.6
 */
MateVFSVolumeType
mate_vfs_volume_get_volume_type (MateVFSVolume *volume)
{
	g_return_val_if_fail (volume != NULL, MATE_VFS_VOLUME_TYPE_MOUNTPOINT);

	return volume->priv->volume_type;
}


/** 
 * mate_vfs_volume_get_device_type:
 * @volume: a #MateVFSVolume.
 *
 * Returns the #MateVFSDeviceType of the @volume.
 *
 * Returns: the device type for @volume.
 *
 * Since: 2.6
 */
MateVFSDeviceType
mate_vfs_volume_get_device_type (MateVFSVolume *volume)
{
	g_return_val_if_fail (volume != NULL, MATE_VFS_DEVICE_TYPE_UNKNOWN);

	return volume->priv->device_type;
}

/** 
 * mate_vfs_volume_get_id:
 * @volume: a #MateVFSVolume.
 *
 * Returns the ID of the @volume,
 *
 * Two #MateVFSVolumes are guaranteed to refer
 * to the same volume if they have the same ID.
 *
 * Returns: the id for the @volume.
 *
 * Since: 2.6
 */
gulong
mate_vfs_volume_get_id (MateVFSVolume *volume)
{
	g_return_val_if_fail (volume != NULL, 0);

	return volume->priv->id;
}

/** 
 * mate_vfs_volume_get_drive:
 * @volume: a #MateVFSVolume.
 *
 * Returns: the drive for the @volume.
 *
 * Since: 2.6
 */
MateVFSDrive *
mate_vfs_volume_get_drive (MateVFSVolume *volume)
{
	MateVFSDrive *drive;

	g_return_val_if_fail (volume != NULL, NULL);

	G_LOCK (volumes);
	drive = mate_vfs_drive_ref (volume->priv->drive);
	G_UNLOCK (volumes);
	
	return drive;
}

void
mate_vfs_volume_unset_drive_private (MateVFSVolume     *volume,
				      MateVFSDrive      *drive)
{
	G_LOCK (volumes);
	g_assert (volume->priv->drive == drive);
	volume->priv->drive = NULL;
	G_UNLOCK (volumes);
}

void
mate_vfs_volume_set_drive_private (MateVFSVolume     *volume,
				    MateVFSDrive      *drive)
{
	G_LOCK (volumes);
	g_assert (volume->priv->drive == NULL);
	volume->priv->drive = drive;
	G_UNLOCK (volumes);
}

/** 
 * mate_vfs_volume_get_device_path:
 * @volume: a #MateVFSVolume.
 *
 * Returns the device path of a #MateVFSVolume.
 *
 * For HAL volumes, this returns the value of the
 * volume's "block.device" key. For UNIX mounts,
 * it returns the %mntent's %mnt_fsname entry.
 *
 * Otherwise, it returns %NULL.
 *
 * Returns: a newly allocated string for device path of @volume.
 *
 * Since: 2.6
 */
char *
mate_vfs_volume_get_device_path (MateVFSVolume *volume)
{
	g_return_val_if_fail (volume != NULL, NULL);

	return g_strdup (volume->priv->device_path);
}

/** 
 * mate_vfs_volume_get_activation_uri:
 * @volume: a #MateVFSVolume.
 *
 * Returns the activation URI of a #MateVFSVolume.
 *
 * The returned URI usually refers to a valid location. You can check the
 * validity of the location by calling mate_vfs_uri_new() with the URI,
 * and checking whether the return value is not %NULL.
 *
 * Returns: a newly allocated string for activation uri of @volume.
 *
 * Since: 2.6
 */
char *
mate_vfs_volume_get_activation_uri (MateVFSVolume *volume)
{
	g_return_val_if_fail (volume != NULL, NULL);

	return g_strdup (volume->priv->activation_uri);
}

/** 
 * mate_vfs_volume_get_hal_udi:
 * @volume: a #MateVFSVolume.
 *
 * Returns the HAL UDI of a #MateVFSVolume.
 *
 * For HAL volumes, this matches the value of the "info.udi" key,
 * for other volumes it is %NULL.
 *
 * Returns: a newly allocated string for unique device id of @volume, or %NULL.
 *
 * Since: 2.6
 */
char *
mate_vfs_volume_get_hal_udi (MateVFSVolume *volume)
{
	g_return_val_if_fail (volume != NULL, NULL);

	return g_strdup (volume->priv->hal_udi);
}

/** 
 * mate_vfs_volume_get_filesystem_type:
 * @volume: a #MateVFSVolume.
 *
 * Returns a string describing the file system on @volume,
 * or %NULL if no information on the underlying file system is
 * available.
 *
 * The file system may be used to provide special functionality
 * that depends on the file system type, for instance to determine
 * whether trashing is supported (cf. mate_vfs_volume_handles_trash()).
 *
 * For HAL mounts, this returns the value of the "volume.fstype"
 * key, for traditional UNIX mounts it is set to the %mntent's 
 * %mnt_type key, for connected servers, %NULL is returned.
 *
 * As of MateVFS 2.15.4, the following file systems are recognized
 * by MateVFS:
 *
 * <table frame="none">
 *  <title>Recognized File Systems</title>
 *  <tgroup cols="3" align="left">
 *   <?dbhtml cellpadding="10" ?>
 *   <colspec colwidth="1*"/>
 *   <colspec colwidth="1*"/>
 *   <colspec colwidth="1*"/>
 *   <thead>
 *    <row>
 *     <entry>File System</entry>
 *     <entry>Display Name</entry>
 *     <entry>Supports Trash</entry>
 *    </row>
 *   </thead>
 *   <tbody>
 *    <row><entry>affs</entry><entry>AFFS Volume</entry><entry>No</entry></row>
 *    <row><entry>afs</entry><entry>AFS Network Volume</entry><entry>No</entry></row>
 *    <row><entry>auto</entry><entry>Auto-detected Volume</entry><entry>No</entry></row>
 *    <row><entry>cd9660</entry><entry>CD-ROM Drive</entry><entry>No</entry></row>
 *    <row><entry>cdda</entry><entry>CD Digital Audio</entry><entry>No</entry></row>
 *    <row><entry>cdrom</entry><entry>CD-ROM Drive</entry><entry>No</entry></row>
 *    <row><entry>devfs</entry><entry>Hardware Device Volume</entry><entry>No</entry></row>
 *    <row><entry>encfs</entry><entry>EncFS Volume</entry><entry>Yes</entry></row>
 *    <row><entry>ext2</entry><entry>Ext2 Linux Volume</entry><entry>Yes</entry></row>
 *    <row><entry>ext2fs</entry><entry>Ext2 Linux Volume</entry><entry>Yes</entry></row>
 *    <row><entry>ext3</entry><entry>Ext3 Linux Volume</entry><entry>Yes</entry></row>
 *    <row><entry>fat</entry><entry>MSDOS Volume</entry><entry>Yes</entry></row>
 *    <row><entry>ffs</entry><entry>BSD Volume</entry><entry>Yes</entry></row>
 *    <row><entry>fuse</entry><entry>FUSE Volume</entry><entry>Yes</entry></row>
 *    <row><entry>hfs</entry><entry>MacOS Volume</entry><entry>Yes</entry></row>
 *    <row><entry>hfsplus</entry><entry>MacOS Volume</entry><entry>No</entry></row>
 *    <row><entry>iso9660</entry><entry>CDROM Volume</entry><entry>No</entry></row>
 *    <row><entry>hsfs</entry><entry>Hsfs CDROM Volume</entry><entry>No</entry></row>
 *    <row><entry>jfs</entry><entry>JFS Volume</entry><entry>Yes</entry></row>
 *    <row><entry>hpfs</entry><entry>Windows NT Volume</entry><entry>No</entry></row>
 *    <row><entry>kernfs</entry><entry>System Volume</entry><entry>No</entry></row>
 *    <row><entry>lfs</entry><entry>BSD Volume</entry><entry>Yes</entry></row>
 *    <row><entry>linprocfs</entry><entry>System Volume</entry><entry>No</entry></row>
 *    <row><entry>mfs</entry><entry>Memory Volume</entry><entry>Yes</entry></row>
 *    <row><entry>minix</entry><entry>Minix Volume</entry><entry>No</entry></row>
 *    <row><entry>msdos</entry><entry>MSDOS Volume</entry><entry>No</entry></row>
 *    <row><entry>msdosfs</entry><entry>MSDOS Volume</entry><entry>No</entry></row>
 *    <row><entry>nfs</entry><entry>NFS Network Volume</entry><entry>Yes</entry></row>
 *    <row><entry>ntfs</entry><entry>Windows NT Volume</entry><entry>No</entry></row>
 *    <row><entry>nwfs</entry><entry>Netware Volume</entry><entry>No</entry></row>
 *    <row><entry>proc</entry><entry>System Volume</entry><entry>No</entry></row>
 *    <row><entry>procfs</entry><entry>System Volume</entry><entry>No</entry></row>
 *    <row><entry>ptyfs</entry><entry>System Volume</entry><entry>No</entry></row>
 *    <row><entry>reiser4</entry><entry>Reiser4 Linux Volume</entry><entry>Yes</entry></row>
 *    <row><entry>reiserfs</entry><entry>ReiserFS Linux Volume</entry><entry>Yes</entry></row>
 *    <row><entry>smbfs</entry><entry>Windows Shared Volume</entry><entry>Yes</entry></row>
 *    <row><entry>supermount</entry><entry>SuperMount Volume</entry><entry>No</entry></row>
 *    <row><entry>udf</entry><entry>DVD Volume</entry><entry>No</entry></row>
 *    <row><entry>ufs</entry><entry>Solaris/BSD Volume</entry><entry>Yes</entry></row>
 *    <row><entry>udfs</entry><entry>Udfs Solaris Volume</entry><entry>Yes</entry></row>
 *    <row><entry>pcfs</entry><entry>Pcfs Solaris Volume</entry><entry>Yes</entry></row>
 *    <row><entry>samfs</entry><entry>Sun SAM-QFS Volume</entry><entry>Yes</entry></row>
 *    <row><entry>tmpfs</entry><entry>Temporary Volume</entry><entry>Yes</entry></row>
 *    <row><entry>umsdos</entry><entry>Enhanced DOS Volume</entry><entry>No</entry></row>
 *    <row><entry>vfat</entry><entry>Windows VFAT Volume</entry><entry>Yes</entry></row>
 *    <row><entry>xenix</entry><entry>Xenix Volume</entry><entry>No</entry></row>
 *    <row><entry>xfs</entry><entry>XFS Linux Volume</entry><entry>Yes</entry></row>
 *    <row><entry>xiafs</entry><entry>XIAFS Volume</entry><entry>No</entry></row>
 *    <row><entry>cifs</entry><entry>CIFS Volume</entry><entry>Yes</entry></row>
 *   </tbody>
 *  </tgroup>
 * </table>
 *
 * Returns: a newly allocated string for filesystem type of @volume.
 *
 * Since: 2.6
 */
char *
mate_vfs_volume_get_filesystem_type (MateVFSVolume *volume)
{
	g_return_val_if_fail (volume != NULL, NULL);

	return g_strdup (volume->priv->filesystem_type);
}

/** 
 * mate_vfs_volume_get_display_name:
 * @volume: a #MateVFSVolume.
 *
 * Returns the display name of the @volume.
 *
 * Returns: a newly allocated string for display name of @volume.
 *
 * Since: 2.6
 */
char *
mate_vfs_volume_get_display_name (MateVFSVolume *volume)
{
	g_return_val_if_fail (volume != NULL, NULL);

	return g_strdup (volume->priv->display_name);
}

/** 
 * mate_vfs_volume_get_icon:
 * @volume: a #MateVFSVolume.
 *
 * Returns: a newly allocated string for the icon filename of @volume.
 *
 * Since: 2.6
 */
char *
mate_vfs_volume_get_icon (MateVFSVolume *volume)
{
	g_return_val_if_fail (volume != NULL, NULL);

	return g_strdup (volume->priv->icon);
}

/** 
 * mate_vfs_volume_is_user_visible:
 * @volume: a #MateVFSVolume.
 *
 * Returns whether the @volume is visible to the user. This
 * should be used by applications to determine whether it
 * is included in user interfaces listing available volumes.
 *
 * Returns: %TRUE if @volume is visible to the user, %FALSE otherwise.
 *
 * Since: 2.6
 */
gboolean
mate_vfs_volume_is_user_visible (MateVFSVolume *volume)
{
	g_return_val_if_fail (volume != NULL, FALSE);

	return volume->priv->is_user_visible;
}

/** 
 * mate_vfs_volume_is_read_only:
 * @volume: a #MateVFSVolume.
 *
 * Returns whether the file system on a @volume is read-only.
 *
 * For HAL volumes, the "volume.is_mounted_read_only" key
 * is authoritative, for traditional UNIX mounts it returns
 * %TRUE if the mount was done with the "ro" option.
 * For servers, %FALSE is returned.
 *
 * Returns: %TRUE if the @volume is read-only to the user, %FALSE otherwise.
 *
 * Since: 2.6
 */
gboolean
mate_vfs_volume_is_read_only (MateVFSVolume *volume)
{
	g_return_val_if_fail (volume != NULL, FALSE);

	return volume->priv->is_read_only;
}

/** 
 * mate_vfs_volume_is_mounted:
 * @volume: a #MateVFSVolume.
 *
 * Returns whether the file system on a @volume is currently mounted.
 *
 * For HAL volumes, this reflects the value of the "volume.is_mounted"
 * key, for traditional UNIX mounts and connected servers, %TRUE
 * is returned, because their existence implies that they are
 * mounted.
 *
 * Returns: %TRUE if the @volume is mounted, %FALSE otherwise.
 *
 * Since: 2.6
 */
gboolean
mate_vfs_volume_is_mounted (MateVFSVolume *volume)
{
	g_return_val_if_fail (volume != NULL, FALSE);

	return volume->priv->is_mounted;
}

/** 
 * mate_vfs_volume_handles_trash:
 * @volume: a #MateVFSVolume.
 *
 * Returns whether the file system on a @volume supports trashing of files.
 *
 * If the @volume has an AutoFS file system (i.e. mate_vfs_volume_get_device_type()
 * returns #MATE_VFS_DEVICE_TYPE_AUTOFS), or if the @volume is mounted read-only
 * (mate_vfs_volume_is_read_only() returns %TRUE), it is assumed
 * to not support trashing of files.
 *
 * Otherwise, if the @volume provides file system information, it is
 * determined whether the file system supports trashing of files.
 * See mate_vfs_volume_get_filesystem_type() for details which
 * volumes provide file system information, and which file systems
 * currently support a trash.
 *
 * Returns: %TRUE if @volume handles trash, %FALSE otherwise.
 *
 * Since: 2.6
 */
gboolean
mate_vfs_volume_handles_trash (MateVFSVolume *volume)
{
	g_return_val_if_fail (volume != NULL, FALSE);

	if (volume->priv->device_type == MATE_VFS_DEVICE_TYPE_AUTOFS) {
		return FALSE;
	}
	if (volume->priv->is_read_only) {
		return FALSE;
	}
	if (volume->priv->filesystem_type != NULL) {
		return _mate_vfs_filesystem_use_trash (volume->priv->filesystem_type);
	}
	return FALSE;
}

int
_mate_vfs_device_type_get_sort_group (MateVFSDeviceType type)
{
	switch (type) {
	case MATE_VFS_DEVICE_TYPE_FLOPPY:
	case MATE_VFS_DEVICE_TYPE_ZIP:
	case MATE_VFS_DEVICE_TYPE_JAZ:
		return 0;
	case MATE_VFS_DEVICE_TYPE_CDROM:
	case MATE_VFS_DEVICE_TYPE_AUDIO_CD:
	case MATE_VFS_DEVICE_TYPE_VIDEO_DVD:
		return 1;
	case MATE_VFS_DEVICE_TYPE_CAMERA:
	case MATE_VFS_DEVICE_TYPE_MEMORY_STICK:
	case MATE_VFS_DEVICE_TYPE_MUSIC_PLAYER:
		return 2;
	case MATE_VFS_DEVICE_TYPE_HARDDRIVE:
	case MATE_VFS_DEVICE_TYPE_WINDOWS:
	case MATE_VFS_DEVICE_TYPE_APPLE:
		return 3;
	case MATE_VFS_DEVICE_TYPE_NFS:
	case MATE_VFS_DEVICE_TYPE_SMB:
	case MATE_VFS_DEVICE_TYPE_NETWORK:
		return 4;
	default:
		return 5;
	}
}

/** 
 * mate_vfs_volume_compare:
 * @a: a #MateVFSVolume.
 * @b: a #MateVFSVolume.
 *
 * Compares two #MateVFSVolume objects @a and @b. Two
 * #MateVFSVolume objects referring to different volumes
 * are guaranteed to not return 0 when comparing them,
 * if they refer to the same volume 0 is returned.
 *
 * The resulting #gint should be used to determine the
 * order in which @a and @b are displayed in graphical
 * user interfces.
 *
 * The comparison algorithm first of all peeks the device
 * type of @a and @b, they will be sorted in the following
 * order:
 * <itemizedlist>
 * <listitem><para>Magnetic and opto-magnetic volumes (ZIP, floppy)</para></listitem>
 * <listitem><para>Optical volumes (CD, DVD)</para></listitem>
 * <listitem><para>External volumes (USB sticks, music players)</para></listitem>
 * <listitem><para>Mounted hard disks</para></listitem>
 * <listitem><para>Network mounts</para></listitem>
 * <listitem><para>Other volumes</para></listitem>
 * </itemizedlist>
 *
 * Afterwards, the display name of @a and @b is compared
 * using a locale-sensitive sorting algorithm, which
 * involves g_utf8_collate_key().
 *
 * If two volumes have the same display name, their
 * unique ID is compared which can be queried using
 * mate_vfs_volume_get_id().
 *
 * Returns: 0 if the volumes refer to the same @MateVFSVolume,
 * a negative value if @a should be displayed before @b,
 * or a positive value if @a should be displayed after @b.
 *
 * Since: 2.6
 */
gint
mate_vfs_volume_compare (MateVFSVolume *a,
			  MateVFSVolume *b)
{
	MateVFSVolumePrivate *priva, *privb;
	gint res;

	if (a == b) {
		return 0;
	}

	priva = a->priv;
	privb = b->priv;
	res = privb->volume_type - priva->volume_type;
	if (res != 0) {
		return res;
	}

	res = _mate_vfs_device_type_get_sort_group (priva->device_type) - _mate_vfs_device_type_get_sort_group (privb->device_type);
	if (res != 0) {
		return res;
	}

	res = strcmp (priva->display_name_key, privb->display_name_key);
	if (res != 0) {
		return res;
	}
	
	return privb->id - priva->id;
}


#ifdef USE_DAEMON

static void
utils_append_string_or_null (DBusMessageIter *iter,
			     const gchar     *str)
{
	if (str == NULL)
		str = "";
	
	dbus_message_iter_append_basic (iter, DBUS_TYPE_STRING, &str);
}

static gchar *
utils_get_string_or_null (DBusMessageIter *iter, gboolean empty_is_null)
{
	const gchar *str;

	dbus_message_iter_get_basic (iter, &str);

	if (empty_is_null && *str == 0) {
		return NULL;
	} else {
		return g_strdup (str);
	}
}

gboolean
mate_vfs_volume_to_dbus (DBusMessageIter *iter,
			  MateVFSVolume  *volume)
{
	MateVFSVolumePrivate *priv;
	DBusMessageIter        struct_iter;
	MateVFSDrive         *drive;
	gint32                 i;

	g_return_val_if_fail (iter != NULL, FALSE);
	g_return_val_if_fail (volume != NULL, FALSE);

	priv = volume->priv;

	if (!dbus_message_iter_open_container (iter,
					       DBUS_TYPE_STRUCT,
					       NULL, /* for struct */
					       &struct_iter)) {
		return FALSE;
	}

	i = priv->id;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	i = priv->volume_type;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	i = priv->device_type;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	drive = mate_vfs_volume_get_drive (volume);
	if (drive != NULL) {
		i = drive->priv->id;
		mate_vfs_drive_unref (drive);
	} else {
		i = 0;
	}
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	utils_append_string_or_null (&struct_iter, priv->activation_uri);
	utils_append_string_or_null (&struct_iter, priv->filesystem_type);
	utils_append_string_or_null (&struct_iter, priv->display_name);
	utils_append_string_or_null (&struct_iter, priv->icon);

	dbus_message_iter_append_basic (&struct_iter,
					DBUS_TYPE_BOOLEAN,
					&priv->is_user_visible);
	dbus_message_iter_append_basic (&struct_iter,
					DBUS_TYPE_BOOLEAN,
					&priv->is_read_only);
	dbus_message_iter_append_basic (&struct_iter,
					DBUS_TYPE_BOOLEAN,
					&priv->is_mounted);
	
	utils_append_string_or_null (&struct_iter, priv->device_path);

	i = priv->unix_device;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	utils_append_string_or_null (&struct_iter, priv->hal_udi);
	utils_append_string_or_null (&struct_iter, priv->mateconf_id);

	dbus_message_iter_close_container (iter, &struct_iter);
	
	return TRUE;
}

MateVFSVolume *
_mate_vfs_volume_from_dbus (DBusMessageIter       *iter,
			     MateVFSVolumeMonitor *volume_monitor)
{
	MateVFSVolumePrivate *priv;
	DBusMessageIter        struct_iter;
	MateVFSVolume        *volume;
	gint32                 i;

	g_return_val_if_fail (iter != NULL, NULL);
	g_return_val_if_fail (volume_monitor != NULL, NULL);
	
	g_assert (dbus_message_iter_get_arg_type (iter) == DBUS_TYPE_STRUCT);

	volume = g_object_new (MATE_VFS_TYPE_VOLUME, NULL);

	priv = volume->priv;

	dbus_message_iter_recurse (iter, &struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i);
	priv->id = i;

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i);
	priv->volume_type = i;

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i);
	priv->device_type = i;

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i);
	if (i != 0) {
		priv->drive = mate_vfs_volume_monitor_get_drive_by_id (
			volume_monitor, i);
		
		if (priv->drive != NULL) {
			mate_vfs_drive_add_mounted_volume_private (priv->drive, volume);
			
			/* The drive reference is weak */
			mate_vfs_drive_unref (priv->drive);
		}
	}

	dbus_message_iter_next (&struct_iter);
	priv->activation_uri = utils_get_string_or_null (&struct_iter, TRUE);

	dbus_message_iter_next (&struct_iter);
	priv->filesystem_type = utils_get_string_or_null (&struct_iter, TRUE);

	dbus_message_iter_next (&struct_iter);
	priv->display_name = utils_get_string_or_null (&struct_iter, TRUE);

	if (volume->priv->display_name != NULL) {
		char *tmp = g_utf8_casefold (volume->priv->display_name, -1);
		volume->priv->display_name_key = g_utf8_collate_key (tmp, -1);
		g_free (tmp);
	} else {
		volume->priv->display_name_key = NULL;
	}
	
	dbus_message_iter_next (&struct_iter);
	priv->icon = utils_get_string_or_null (&struct_iter, TRUE);

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &priv->is_user_visible);

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &priv->is_read_only);

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &priv->is_mounted);

	dbus_message_iter_next (&struct_iter);
	priv->device_path = utils_get_string_or_null (&struct_iter, TRUE);

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &priv->unix_device);

	dbus_message_iter_next (&struct_iter);
	priv->hal_udi = utils_get_string_or_null (&struct_iter, TRUE);
	
	dbus_message_iter_next (&struct_iter);
	priv->mateconf_id = utils_get_string_or_null (&struct_iter, TRUE);
	
	return volume;
}

#endif
