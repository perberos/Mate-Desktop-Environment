/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-drive.c - Handling of drives for the MATE Virtual File System.

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

#include "mate-vfs-drive.h"
#include "mate-vfs-volume-monitor-private.h"

static void     mate_vfs_drive_class_init           (MateVFSDriveClass *klass);
static void     mate_vfs_drive_init                 (MateVFSDrive      *drive);
static void     mate_vfs_drive_finalize             (GObject          *object);

enum
{
	VOLUME_MOUNTED,
	VOLUME_PRE_UNMOUNT,
	VOLUME_UNMOUNTED,
	LAST_SIGNAL
};

static GObjectClass *parent_class = NULL;
static guint drive_signals [LAST_SIGNAL] = { 0 };

G_LOCK_DEFINE_STATIC (drives);

GType
mate_vfs_drive_get_type (void)
{
	static GType drive_type = 0;

	if (!drive_type) {
		const GTypeInfo drive_info = {
			sizeof (MateVFSDriveClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) mate_vfs_drive_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (MateVFSDrive),
			0,              /* n_preallocs */
			(GInstanceInitFunc) mate_vfs_drive_init
		};
		
		drive_type =
			g_type_register_static (G_TYPE_OBJECT, "MateVFSDrive",
						&drive_info, 0);
	}
	
	return drive_type;
}

static void
mate_vfs_drive_class_init (MateVFSDriveClass *class)
{
	GObjectClass *o_class;
	
	parent_class = g_type_class_peek_parent (class);
	
	o_class = (GObjectClass *) class;
	
	/* GObject signals */
	o_class->finalize = mate_vfs_drive_finalize;

	/**
	 * MateVFSDrive::volume-mounted:
	 * @drive: the #MateVFSDrive which received the signal.
	 * @volume: the #MateVFSVolume that has been mounted.
	 *
	 * This signal is emitted after the #MateVFSVolume @volume has been mounted.
	 *
	 * When the @volume is mounted, it is added to the @drive's list of mounted
	 * volumes, which can be queried using mate_vfs_drive_get_mounted_volumes().
	 *
	 * It is also added to the list of the #MateVFSVolumeMonitor's list of mounted
	 * volumes, which can be queried using mate_vfs_volume_monitor_get_mounted_volumes().
	 **/
	drive_signals[VOLUME_MOUNTED] =
		g_signal_new ("volume_mounted",
			      G_TYPE_FROM_CLASS (o_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (MateVFSDriveClass, volume_mounted),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1,
			      MATE_VFS_TYPE_VOLUME);

	/**
	 * MateVFSDrive::volume-pre-unmount:
	 * @drive: the #MateVFSDrive which received the signal.
	 * @volume: the #MateVFSVolume that is about to be unmounted.
	 *
	 * This signal is emitted when the #MateVFSVolume @volume, which has been present in
	 * the #MateVFSDrive @drive, is about to be unmounted.
	 *
	 * When the @volume is unmounted, it is removed from the @drive's list of mounted
	 * volumes, which can be queried using mate_vfs_drive_get_mounted_volumes().
	 *
	 * It is also removed from the #MateVFSVolumeMonitor's list of mounted volumes,
	 * which can be queried using mate_vfs_volume_monitor_get_mounted_volumes().
	 *
	 * When a client application receives this signal, it must free all resources
	 * associated with the @volume, for instance cancel all pending file operations
	 * on the @volume, and cancel all pending file monitors using mate_vfs_monitor_cancel().
	 **/
	drive_signals[VOLUME_PRE_UNMOUNT] =
		g_signal_new ("volume_pre_unmount",
			      G_TYPE_FROM_CLASS (o_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (MateVFSDriveClass, volume_pre_unmount),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1,
			      MATE_VFS_TYPE_VOLUME);

	/**
	 * MateVFSDrive::volume-unmounted:
	 * @drive: the #MateVFSDrive which received the signal.
	 * @volume: the #MateVFSVolume that has been unmounted.
	 *
	 * This signal is emitted after the #MateVFSVolume @volume, which had been present in
	 * the #MateVFSDrive @drive, has been unmounted.
	 *
	 * When the @volume is unmounted, it is removed from the @drive's list of mounted
	 * volumes, which can be queried using mate_vfs_drive_get_mounted_volumes().
	 *
	 * It is also removed from the #MateVFSVolumeMonitor's list of mounted volumes,
	 * which can be queried using mate_vfs_volume_monitor_get_mounted_volumes().
	 **/
	drive_signals[VOLUME_UNMOUNTED] =
		g_signal_new ("volume_unmounted",
			      G_TYPE_FROM_CLASS (o_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (MateVFSDriveClass, volume_unmounted),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1,
			      MATE_VFS_TYPE_VOLUME);
}

static void
mate_vfs_drive_init (MateVFSDrive *drive)
{
	static int drive_id_counter = 1;
	
	drive->priv = g_new0 (MateVFSDrivePrivate, 1);

	G_LOCK (drives);
	drive->priv->id = drive_id_counter++;
	G_UNLOCK (drives);
}

/** 
 * mate_vfs_drive_ref:
 * @drive: a #MateVFSDrive, or %NULL.
 *
 * Increases the refcount of the @drive by 1, if it is not %NULL.
 *
 * Returns: the @drive with its refcount increased by one,
 * 	    or %NULL if @drive is %NULL.
 *
 * Since: 2.6
 */
MateVFSDrive *
mate_vfs_drive_ref (MateVFSDrive *drive)
{
	if (drive == NULL) {
		return NULL;
	}

	G_LOCK (drives);
	g_object_ref (drive);
	G_UNLOCK (drives);
	return drive;
}

/** 
 * mate_vfs_drive_unref:
 * @drive: a #MateVFSDrive, or %NULL.
 *
 * Decreases the refcount of the @drive by 1, if it is not %NULL.
 *
 * Since: 2.6
 */
void
mate_vfs_drive_unref (MateVFSDrive *drive)
{
	if (drive == NULL) {
		return;
	}
	
	G_LOCK (drives);
	g_object_unref (drive);
	G_UNLOCK (drives);
}


/* Remeber that this could be running on a thread other
 * than the main thread */
static void
mate_vfs_drive_finalize (GObject *object)
{
	MateVFSDrive *drive = (MateVFSDrive *) object;
	MateVFSDrivePrivate *priv;
	GList *current_vol;

	priv = drive->priv;

	for (current_vol = priv->volumes; current_vol != NULL; current_vol = current_vol->next) {
		MateVFSVolume *vol;
		vol = MATE_VFS_VOLUME (current_vol->data);

		mate_vfs_volume_unset_drive_private (vol, drive);
		mate_vfs_volume_unref (vol);
	}

	g_list_free (priv->volumes);
	g_free (priv->device_path);
	g_free (priv->activation_uri);
	g_free (priv->display_name);
	g_free (priv->display_name_key);
	g_free (priv->icon);
	g_free (priv->hal_udi);
	g_free (priv->hal_drive_udi);
	g_free (priv->hal_backing_crypto_volume_udi);
	g_free (priv);
	drive->priv = NULL;
	
	if (G_OBJECT_CLASS (parent_class)->finalize)
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

/** 
 * mate_vfs_drive_get_id:
 * @drive: a #MateVFSDrive.
 *
 * Returns: drive id, a #gulong value.
 *
 * Since: 2.6
 */
gulong 
mate_vfs_drive_get_id (MateVFSDrive *drive)
{
	g_return_val_if_fail (drive != NULL, 0);

	return drive->priv->id;
}

/** 
 * mate_vfs_drive_get_device_type:
 * @drive: a #MateVFSDrive.
 *
 * Returns: device type, a #MateVFSDeviceType value.
 *
 * Since: 2.6
 */
MateVFSDeviceType
mate_vfs_drive_get_device_type (MateVFSDrive *drive)
{
	g_return_val_if_fail (drive != NULL, MATE_VFS_DEVICE_TYPE_UNKNOWN);

	return drive->priv->device_type;
}

/** 
 * mate_vfs_drive_get_mounted_volume:
 * @drive: a #MateVFSDrive.
 *
 * Returns the first mounted volume for the @drive.
 *
 * Returns: a #MateVFSVolume.
 *
 * Since: 2.6
 * Deprecated: Use mate_vfs_drive_get_mounted_volumes() instead.
 */
MateVFSVolume *
mate_vfs_drive_get_mounted_volume (MateVFSDrive *drive)
{
	MateVFSVolume *vol;
	GList *first_vol;

	g_return_val_if_fail (drive != NULL, NULL);

	G_LOCK (drives);
	first_vol = g_list_first (drive->priv->volumes);

	if (first_vol != NULL) {
		vol = mate_vfs_volume_ref (MATE_VFS_VOLUME (first_vol->data));
	} else {
		vol = NULL;
	}

	G_UNLOCK (drives);

	return vol;
}

/** 
 * mate_vfs_drive_volume_list_free:
 * @volumes: list of #MateVFSVolumes to be freed, or %NULL.
 *
 * Frees the list @volumes, if it is not %NULL.
 *
 * Since: 2.8
 */
void
mate_vfs_drive_volume_list_free (GList *volumes)
{
	if (volumes != NULL) {
		g_list_foreach (volumes, 
				(GFunc)mate_vfs_volume_unref,
				NULL);

		g_list_free (volumes);
	}
}

/** 
 * mate_vfs_drive_get_mounted_volumes:
 * @drive: a #MateVFSDrive.
 *
 * Returns: list of mounted volumes for the @drive.
 *
 * Since: 2.8
 */
GList *
mate_vfs_drive_get_mounted_volumes (MateVFSDrive *drive)
{
	GList *return_list;

	g_return_val_if_fail (drive != NULL, NULL);

	G_LOCK (drives);
	return_list = g_list_copy (drive->priv->volumes);
	g_list_foreach (return_list, 
			(GFunc)mate_vfs_volume_ref,
			NULL);

	G_UNLOCK (drives);

	return return_list;
}

/** 
 * mate_vfs_drive_is_mounted:
 * @drive: a #MateVFSDrive.
 *
 * Returns: %TRUE if the @drive is mounted, %FALSE otherwise.
 *
 * Since: 2.6
 */
gboolean
mate_vfs_drive_is_mounted (MateVFSDrive *drive)
{
	gboolean res;

	g_return_val_if_fail (drive != NULL, FALSE);

	G_LOCK (drives);
	res = drive->priv->volumes != NULL;
	G_UNLOCK (drives);
	
	return res;
}

/** 
 * mate_vfs_drive_needs_eject:
 * @drive: a #MateVFSDrive.
 *
 * Returns: %TRUE if the @drive needs to be ejected, %FALSE otherwise.
 *
 * Since: 2.16
 */
gboolean
mate_vfs_drive_needs_eject (MateVFSDrive *drive)
{
	gboolean res;

	g_return_val_if_fail (drive != NULL, FALSE);

	G_LOCK (drives);
	res = drive->priv->must_eject_at_unmount;
	G_UNLOCK (drives);
	
	return res;
}

void
mate_vfs_drive_remove_volume_private (MateVFSDrive      *drive,
				       MateVFSVolume     *volume)
{
	G_LOCK (drives);
	g_assert ((g_list_find (drive->priv->volumes,
				 volume)) != NULL);

	drive->priv->volumes = g_list_remove (drive->priv->volumes,
                                              volume);
	G_UNLOCK (drives);
	mate_vfs_volume_unref (volume);
}

void
mate_vfs_drive_add_mounted_volume_private (MateVFSDrive      *drive,
					    MateVFSVolume     *volume)
{
	G_LOCK (drives);

	g_assert ((g_list_find (drive->priv->volumes,
				 volume)) == NULL);

	drive->priv->volumes = g_list_append (drive->priv->volumes, 
					      mate_vfs_volume_ref (volume));

	G_UNLOCK (drives);
}

/** 
 * mate_vfs_drive_get_device_path:
 * @drive: a #MateVFSDrive.
 *
 * Returns the device path of a #MateVFSDrive.
 *
 * For HAL drives, this returns the value of the
 * drives's "block.device" key. For UNIX mounts,
 * it returns the %mntent's %mnt_fsname entry.
 *
 * Otherwise, it returns %NULL.
 *
 * Returns: a newly allocated string for the device path of the #drive.
 *
 * Since: 2.6
 */
char *
mate_vfs_drive_get_device_path (MateVFSDrive *drive)
{
	g_return_val_if_fail (drive != NULL, NULL);

	return g_strdup (drive->priv->device_path);
}

/** 
 * mate_vfs_drive_get_activation_uri:
 * @drive: a #MateVFSDrive.
 *
 * Returns the activation URI of a #MateVFSDrive.
 *
 * The returned URI usually refers to a valid location. You can check the
 * validity of the location by calling mate_vfs_uri_new() with the URI,
 * and checking whether the return value is not %NULL.
 *
 * Returns: a newly allocated string for the activation uri of the #drive.
 *
 * Since: 2.6
 */
char *
mate_vfs_drive_get_activation_uri (MateVFSDrive *drive)
{
	g_return_val_if_fail (drive != NULL, NULL);

	return g_strdup (drive->priv->activation_uri);
}

/** 
 * mate_vfs_drive_get_hal_udi:
 * @drive: a #MateVFSDrive.
 *
 * Returns the HAL UDI of a #MateVFSDrive.
 *
 * For HAL drives, this matches the value of the "info.udi" key,
 * for other drives it is %NULL.
 *
 * Returns: a newly allocated string for the unique device id of the @drive, or %NULL.
 *
 * Since: 2.6
 */
char *
mate_vfs_drive_get_hal_udi (MateVFSDrive *drive)
{
	g_return_val_if_fail (drive != NULL, NULL);

	return g_strdup (drive->priv->hal_udi);
}

/** 
 * mate_vfs_drive_get_display_name:
 * @drive: a #MateVFSDrive.
 *
 * Returns: a newly allocated string for the display name of the @drive.
 *
 * Since: 2.6
 */
char *
mate_vfs_drive_get_display_name (MateVFSDrive *drive)
{
	g_return_val_if_fail (drive != NULL, NULL);

	return g_strdup (drive->priv->display_name);
}

/** 
 * mate_vfs_drive_get_icon:
 * @drive: a #MateVFSDrive.
 *
 * Returns: a newly allocated string for the icon filename of the @drive.
 *
 * Since: 2.6
 */
char *
mate_vfs_drive_get_icon (MateVFSDrive *drive)
{
	g_return_val_if_fail (drive != NULL, NULL);

	return g_strdup (drive->priv->icon);
}

/** 
 * mate_vfs_drive_is_user_visible:
 * @drive: a #MateVFSDrive.
 *
 * Returns whether the @drive is visible to the user. This
 * should be used by applications to determine whether it
 * is included in user interfaces listing available drives.
 *
 * Returns: %TRUE if the @drive is visible to the user, %FALSE otherwise.
 *
 * Since: 2.6
 */
gboolean
mate_vfs_drive_is_user_visible (MateVFSDrive *drive)
{
	g_return_val_if_fail (drive != NULL, FALSE);

	return drive->priv->is_user_visible;
}

/** 
 * mate_vfs_drive_is_connected:
 * @drive: a #MateVFSDrive.
 *
 * Returns: %TRUE if the @drive is connected, %FALSE otherwise.
 *
 * Since: 2.6
 */
gboolean
mate_vfs_drive_is_connected (MateVFSDrive *drive)
{
	g_return_val_if_fail (drive != NULL, FALSE);

	return drive->priv->is_connected;
}

/** 
 * mate_vfs_drive_compare:
 * @a: a #MateVFSDrive.
 * @b: a #MateVFSDrive.
 *
 * Compares two #MateVFSDrive objects @a and @b. Two
 * #MateVFSDrive objects referring to different drives
 * are guaranteed to not return 0 when comparing them,
 * if they refer to the same drive 0 is returned.
 *
 * The resulting #gint should be used to determine the
 * order in which @a and @b are displayed in graphical
 * user interfces.
 *
 * The comparison algorithm first of all peeks the device
 * type of @a and @b, they will be sorted in the following
 * order:
 * <itemizedlist>
 * <listitem><para>Magnetic and opto-magnetic drives (ZIP, floppy)</para></listitem>
 * <listitem><para>Optical drives (CD, DVD)</para></listitem>
 * <listitem><para>External drives (USB sticks, music players)</para></listitem>
 * <listitem><para>Mounted hard disks<</para></listitem>
 * <listitem><para>Other drives<</para></listitem>
 * </itemizedlist>
 *
 * Afterwards, the display name of @a and @b is compared
 * using a locale-sensitive sorting algorithm, which
 * involves g_utf8_collate_key().
 *
 * If two drives have the same display name, their
 * unique ID is compared which can be queried using
 * mate_vfs_drive_get_id().
 *
 * Returns: 0 if the drives refer to the same @MateVFSDrive,
 * a negative value if @a should be displayed before @b,
 * or a positive value if @a should be displayed after @b.
 *
 * Since: 2.6
 */
gint
mate_vfs_drive_compare (MateVFSDrive *a,
			 MateVFSDrive *b)
{
	MateVFSDrivePrivate *priva, *privb;
	gint res;

	if (a == b) {
		return 0;
	}

	priva = a->priv;
	privb = b->priv;

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
mate_vfs_drive_to_dbus (DBusMessageIter *iter,
			 MateVFSDrive   *drive)
{
	MateVFSDrivePrivate *priv;
	DBusMessageIter       struct_iter;
        DBusMessageIter       array_iter;
	MateVFSVolume       *volume;
	gint32                i;
	GList *l;
		
	g_return_val_if_fail (iter != NULL, FALSE);
	g_return_val_if_fail (drive != NULL, FALSE);

	priv = drive->priv;

	if (!dbus_message_iter_open_container (iter,
					       DBUS_TYPE_STRUCT,
					       NULL, /* for struct */
					       &struct_iter)) {
		return FALSE;
	}

	i = priv->id;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	i = priv->device_type;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);


        if (!dbus_message_iter_open_container (&struct_iter,
                                               DBUS_TYPE_ARRAY,
                                               DBUS_TYPE_INT32_AS_STRING,
                                               &array_iter)) {
                return FALSE;
        }
	for (l = drive->priv->volumes; l != NULL; l = l->next) {
		volume = MATE_VFS_VOLUME (l->data);
		i = volume->priv->id;
		dbus_message_iter_append_basic (&array_iter, DBUS_TYPE_INT32, &i);
	}
        if (!dbus_message_iter_close_container (&struct_iter, &array_iter)) {
                return FALSE;
        }

	utils_append_string_or_null (&struct_iter, priv->device_path);
	utils_append_string_or_null (&struct_iter, priv->activation_uri);
	utils_append_string_or_null (&struct_iter, priv->display_name);
	utils_append_string_or_null (&struct_iter, priv->icon);
	utils_append_string_or_null (&struct_iter, priv->hal_udi);

	dbus_message_iter_append_basic (&struct_iter,
					DBUS_TYPE_BOOLEAN,
					&priv->is_user_visible);
	dbus_message_iter_append_basic (&struct_iter,
					DBUS_TYPE_BOOLEAN,
					&priv->is_connected);
	dbus_message_iter_append_basic (&struct_iter,
					DBUS_TYPE_BOOLEAN,
					&priv->must_eject_at_unmount);

	if (!dbus_message_iter_close_container (iter, &struct_iter)) {
		return FALSE;
	}
	    
	return TRUE;
}

MateVFSDrive *
_mate_vfs_drive_from_dbus (DBusMessageIter       *iter,
			    MateVFSVolumeMonitor *volume_monitor)
{
	DBusMessageIter       struct_iter;
	DBusMessageIter       array_iter;
	MateVFSDrive        *drive;
	MateVFSDrivePrivate *priv;
	gint32                i;

	g_return_val_if_fail (iter != NULL, NULL);
	g_return_val_if_fail (volume_monitor != NULL, NULL);
	
	g_assert (dbus_message_iter_get_arg_type (iter) == DBUS_TYPE_STRUCT);
	
	/* Note: the drives lock is locked in _init. */
	drive = g_object_new (MATE_VFS_TYPE_DRIVE, NULL);
	priv = drive->priv;

	dbus_message_iter_recurse (iter, &struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i);
	priv->id = i;

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i);
	priv->device_type = i;


	dbus_message_iter_next (&struct_iter);
	if (dbus_message_iter_get_arg_type (&struct_iter) == DBUS_TYPE_ARRAY) {
		dbus_message_iter_recurse (&struct_iter, &array_iter);
		while (dbus_message_iter_get_arg_type (&array_iter) == DBUS_TYPE_INT32) {
			MateVFSVolume *volume;
			dbus_message_iter_get_basic (&array_iter, &i);

			volume = mate_vfs_volume_monitor_get_volume_by_id (volume_monitor,
									    i);
			if (volume != NULL) {
				mate_vfs_drive_add_mounted_volume_private (drive, volume);
				mate_vfs_volume_set_drive_private (volume, drive);
			}
			
			if (!dbus_message_iter_has_next (&array_iter)) {
				break;
			}
			dbus_message_iter_next (&array_iter);
		}
	}
	
	dbus_message_iter_next (&struct_iter);
	priv->device_path = utils_get_string_or_null (&struct_iter, TRUE);

	dbus_message_iter_next (&struct_iter);
	priv->activation_uri = utils_get_string_or_null (&struct_iter, TRUE);

	dbus_message_iter_next (&struct_iter);
	priv->display_name = utils_get_string_or_null (&struct_iter, TRUE);
	if (drive->priv->display_name != NULL) {
		char *tmp = g_utf8_casefold (drive->priv->display_name, -1);
		drive->priv->display_name_key = g_utf8_collate_key (tmp, -1);
		g_free (tmp);
	} else {
		drive->priv->display_name_key = NULL;
	}
	
	dbus_message_iter_next (&struct_iter);
	priv->icon = utils_get_string_or_null (&struct_iter, TRUE);
	
	dbus_message_iter_next (&struct_iter);
	priv->hal_udi = utils_get_string_or_null (&struct_iter, TRUE);

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &priv->is_user_visible);

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &priv->is_connected);

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &priv->must_eject_at_unmount);

	return drive;
}

#endif	/* USE_DAEMON */
