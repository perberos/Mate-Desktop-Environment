/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-volume-monitor.c - Central volume handling.

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

#include "mate-vfs-private.h"
#include "mate-vfs-volume-monitor.h"
#include "mate-vfs-volume-monitor-private.h"
#ifdef USE_DAEMON
#include "mate-vfs-volume-monitor-client.h"
#endif

#ifdef G_OS_WIN32
#define _WIN32_WINNT 0x0501
#include <windows.h>
#endif

#include <glib/gstdio.h>

static void mate_vfs_volume_monitor_class_init (MateVFSVolumeMonitorClass *klass);
static void mate_vfs_volume_monitor_init       (MateVFSVolumeMonitor      *volume_monitor);
static void mate_vfs_volume_monitor_finalize   (GObject                    *object);

enum
{
	VOLUME_MOUNTED,
	VOLUME_PRE_UNMOUNT,
	VOLUME_UNMOUNTED,
	DRIVE_CONNECTED,
	DRIVE_DISCONNECTED,
	LAST_SIGNAL
};

static GObjectClass *parent_class = NULL;
static guint volume_monitor_signals [LAST_SIGNAL] = { 0 };

GType
mate_vfs_volume_monitor_get_type (void)
{
	static GType volume_monitor_type = 0;

	if (!volume_monitor_type) {
		const GTypeInfo volume_monitor_info = {
			sizeof (MateVFSVolumeMonitorClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) mate_vfs_volume_monitor_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (MateVFSVolumeMonitor),
			0,              /* n_preallocs */
			(GInstanceInitFunc) mate_vfs_volume_monitor_init
		};
		
		volume_monitor_type =
			g_type_register_static (G_TYPE_OBJECT, "MateVFSVolumeMonitor",
						&volume_monitor_info, 0);
	}
	
	return volume_monitor_type;
}

static void
mate_vfs_volume_monitor_class_init (MateVFSVolumeMonitorClass *class)
{
	GObjectClass *o_class;
	
	parent_class = g_type_class_peek_parent (class);
	
	o_class = (GObjectClass *) class;
	
	/* GObject signals */
	o_class->finalize = mate_vfs_volume_monitor_finalize;

	/**
	 * MateVFSVolumeMonitor::volume-mounted:
	 * @volume_monitor: the #MateVFSVolumeMonitor which received the signal.
	 * @volume: the #MateVFSVolume that has been mounted.
	 *
	 * This signal is emitted after the #MateVFSVolume @volume has been mounted.
	 *
	 * When the @volume is mounted, it is present in the @volume_monitor's list of mounted
	 * volumes, which can be queried using mate_vfs_volume_monitor_get_mounted_volumes().
	 *
	 * If the @volume has an associated #MateVFSDrive, it also appears in the drive's
	 * list of mounted volumes, which can be queried using mate_vfs_drive_get_mounted_volumes().
	 **/
	volume_monitor_signals[VOLUME_MOUNTED] =
		g_signal_new ("volume_mounted",
			      G_TYPE_FROM_CLASS (o_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (MateVFSVolumeMonitorClass, volume_mounted),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1,
			      MATE_VFS_TYPE_VOLUME);

	/**
	 * MateVFSVolumeMonitor::volume-pre-unmount:
	 * @volume_monitor: the #MateVFSVolumeMonitor which received the signal.
	 * @volume: the #MateVFSVolume that is about to be unmounted.
	 *
	 * This signal is emitted when the #MateVFSVolume @volume is about to be unmounted.
	 *
	 * When the @volume is unmounted, it is removed from the @volume_monitor's list of mounted
	 * volumes, which can be queried using mate_vfs_volume_monitor_get_mounted_volumes().
	 *
	 * If the @volume has an associated #MateVFSDrive, it is also removed from in the drive's
	 * list of mounted volumes, which can be queried using mate_vfs_drive_get_mounted_volumes().
	 *
	 * When a client application receives this signal, it must free all resources
	 * associated with the @volume, for instance cancel all pending file operations
	 * on the @volume, and cancel all pending file monitors using mate_vfs_monitor_cancel().
	 **/
	volume_monitor_signals[VOLUME_PRE_UNMOUNT] =
		g_signal_new ("volume_pre_unmount",
			      G_TYPE_FROM_CLASS (o_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (MateVFSVolumeMonitorClass, volume_pre_unmount),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1,
			      MATE_VFS_TYPE_VOLUME);

	/**
	 * MateVFSVolumeMonitor::volume-unmounted:
	 * @volume_monitor: the #MateVFSVolumeMonitor which received the signal.
	 * @volume: the #MateVFSVolume that has been unmounted.
	 *
	 * This signal is emitted after the #MateVFSVolume @volume has been unmounted.
	 *
	 * When the @volume is unmounted, it is removed from the @volume_monitor's list of mounted
	 * volumes, which can be queried using mate_vfs_volume_monitor_get_mounted_volumes().
	 *
	 * If the @volume has an associated #MateVFSDrive, it is also removed from in the drive's
	 * list of mounted volumes, which can be queried using mate_vfs_drive_get_mounted_volumes().
	 **/
	volume_monitor_signals[VOLUME_UNMOUNTED] =
		g_signal_new ("volume_unmounted",
			      G_TYPE_FROM_CLASS (o_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (MateVFSVolumeMonitorClass, volume_unmounted),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1,
			      MATE_VFS_TYPE_VOLUME);

	/**
	 * MateVFSVolumeMonitor::drive-connected:
	 * @volume_monitor: the #MateVFSVolumeMonitor which received the signal.
	 * @drive: the #MateVFSDrive that has been connected.
	 *
	 * This signal is emitted when the #MateVFSDrive @drive has been connected.
	 *
	 * When the @drive is connected, it is present in the @volume_monitor's list of connected
	 * drives, which can be queried using mate_vfs_volume_monitor_get_connected_drives().
	 **/
	volume_monitor_signals[DRIVE_CONNECTED] =
		g_signal_new ("drive_connected",
			      G_TYPE_FROM_CLASS (o_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (MateVFSVolumeMonitorClass, drive_connected),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1,
			      MATE_VFS_TYPE_DRIVE);

	/**
	 * MateVFSVolumeMonitor::drive-disconnected:
	 * @volume_monitor: the #MateVFSVolumeMonitor which received the signal.
	 * @drive: the #MateVFSDrive that has been disconnected.
	 *
	 * This signal is emitted after the #MateVFSDrive @drive has been disconnected.
	 *
	 * When the @drive is disconnected, it is removed from the @volume_monitor's list of connected
	 * drives, which can be queried using mate_vfs_volume_monitor_get_connected_drives().
	 **/
	volume_monitor_signals[DRIVE_DISCONNECTED] =
		g_signal_new ("drive_disconnected",
			      G_TYPE_FROM_CLASS (o_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (MateVFSVolumeMonitorClass, drive_disconnected),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1,
			      MATE_VFS_TYPE_DRIVE);
}

static void
mate_vfs_volume_monitor_init (MateVFSVolumeMonitor *volume_monitor)
{
	volume_monitor->priv = g_new0 (MateVFSVolumeMonitorPrivate, 1);

	volume_monitor->priv->mutex = g_mutex_new ();
}

G_LOCK_DEFINE_STATIC (volume_monitor_ref);

/** 
 * mate_vfs_volume_monitor_ref:
 * @volume_monitor: the #MateVFSVolumeMonitor, or %NULL.
 *
 * Increases the refcount of @volume_monitor by one, if it is not %NULL.
 *
 * You shouldn't use this function unless you know what you are doing:
 * #MateVFSVolumeMonitor is to be used as a singleton object, see
 * mate_vfs_get_volume_monitor() for more details.
 *
 * Returns: @volume_monitor with its refcount increased by one,
 * 	    or %NULL if @volume_monitor is %NULL.
 *
 * Since: 2.6
 */
MateVFSVolumeMonitor *
mate_vfs_volume_monitor_ref (MateVFSVolumeMonitor *volume_monitor)
{
	if (volume_monitor == NULL) {
		return NULL;
	}
	
	G_LOCK (volume_monitor_ref);
	g_object_ref (volume_monitor);
	G_UNLOCK (volume_monitor_ref);
	return volume_monitor;
}

/** 
 * mate_vfs_volume_monitor_unref:
 * @volume_monitor: #MateVFSVolumeMonitor, or %NULL.
 *
 * Decreases the refcount of @volume_monitor by one, if it is not %NULL.
 *
 * You shouldn't use this function unless you know what you are doing:
 * #MateVFSVolumeMonitor is to be used as a singleton object, see
 * mate_vfs_get_volume_monitor() for more details.
 *
 * Since: 2.6
 */
void
mate_vfs_volume_monitor_unref (MateVFSVolumeMonitor *volume_monitor)
{
	if (volume_monitor == NULL) {
		return;
	}
	
	G_LOCK (volume_monitor_ref);
	g_object_unref (volume_monitor);
	G_UNLOCK (volume_monitor_ref);
}


/* Remeber that this could be running on a thread other
 * than the main thread */
static void
mate_vfs_volume_monitor_finalize (GObject *object)
{
	MateVFSVolumeMonitor *volume_monitor = (MateVFSVolumeMonitor *) object;
	MateVFSVolumeMonitorPrivate *priv;

	priv = volume_monitor->priv;
	
	g_list_foreach (priv->mtab_volumes,
			(GFunc)mate_vfs_volume_unref, NULL);
	g_list_free (priv->mtab_volumes);
	g_list_foreach (priv->server_volumes,
			(GFunc)mate_vfs_volume_unref, NULL);
	g_list_free (priv->server_volumes);
	g_list_foreach (priv->vfs_volumes,
			(GFunc)mate_vfs_volume_unref, NULL);
	g_list_free (priv->vfs_volumes);

	g_list_foreach (priv->fstab_drives,
			(GFunc)mate_vfs_drive_unref, NULL);
	g_list_free (priv->fstab_drives);
	g_list_foreach (priv->vfs_drives,
			(GFunc)mate_vfs_drive_unref, NULL);
	g_list_free (priv->vfs_drives);
	
	g_mutex_free (priv->mutex);
	g_free (priv);
	volume_monitor->priv = NULL;
	
	if (G_OBJECT_CLASS (parent_class)->finalize)
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

#ifndef USE_DAEMON

#ifdef G_OS_WIN32
static void
handle_volume (MateVFSVolumeMonitor *volume_monitor,
	       wchar_t               *volume_device)
{
	wchar_t path_names[MAX_PATH*10];
	DWORD path_names_len;
	
	if (GetVolumePathNamesForVolumeNameW (volume_device,
					      path_names, G_N_ELEMENTS (path_names),
					      &path_names_len)) {
		MateVFSVolume *volume;
		MateVFSVolumePrivate *volume_priv;
		MateVFSDrive *drive;
		MateVFSDrivePrivate *drive_priv;
		
		UINT drive_type;
		wchar_t volume_name[MAX_PATH];
		wchar_t file_system_name[MAX_PATH];
		gchar *tmp;
		DWORD file_system_flags;
		
		drive_type = GetDriveTypeW (volume_device);
		
		if (drive_type == DRIVE_UNKNOWN ||
		    drive_type == DRIVE_NO_ROOT_DIR)
			return;

		drive = g_object_new (MATE_VFS_TYPE_DRIVE, NULL);
		drive_priv = drive->priv;
		
		drive_priv->device_type = MATE_VFS_DEVICE_TYPE_WINDOWS;
		
		drive_priv->device_path = g_utf16_to_utf8 (volume_device, -1, NULL, NULL, NULL);
		drive_priv->activation_uri = g_filename_to_uri (drive_priv->device_path, NULL, NULL);
		volume_name[0] = '\0';
		file_system_name[0] = '\0';
		GetVolumeInformationW (volume_device, volume_name, G_N_ELEMENTS (volume_name),
				       NULL, NULL,
				       &file_system_flags,
				       file_system_name, G_N_ELEMENTS (file_system_name));

		if (volume_name[0])
			drive_priv->display_name = g_utf16_to_utf8 (volume_name, -1, NULL, NULL, NULL);
		else if (path_names[0])
			drive_priv->display_name = g_utf16_to_utf8 (path_names, -1, NULL, NULL, NULL);
		else
			drive_priv->display_name = g_strdup (drive_priv->device_path);
		tmp = g_utf8_casefold (drive_priv->display_name, -1);
		drive_priv->display_name_key = g_utf8_collate_key (tmp, -1);
		g_free (tmp);

		/* ??? */
		drive_priv->is_user_visible = TRUE;

		drive_priv->must_eject_at_unmount = FALSE;

		_mate_vfs_volume_monitor_connected (volume_monitor, drive);
		mate_vfs_drive_unref (drive);

		volume = g_object_new (MATE_VFS_TYPE_VOLUME, NULL);
		volume_priv = volume->priv;
		
		volume_priv->volume_type = MATE_VFS_VOLUME_TYPE_MOUNTPOINT;
		volume_priv->device_type = MATE_VFS_DEVICE_TYPE_WINDOWS;

		volume_priv->drive = drive;

		mate_vfs_drive_add_mounted_volume_private (drive, volume);

		volume_priv->activation_uri = g_strdup (drive_priv->activation_uri);
		volume_priv->filesystem_type = g_utf16_to_utf8 (file_system_name, -1, NULL, NULL, NULL);
		volume_priv->display_name = g_strdup (drive_priv->display_name);
		volume_priv->display_name_key = g_strdup (drive_priv->display_name_key);

		volume_priv->is_user_visible = TRUE;
		volume_priv->is_read_only = FALSE;

		volume_priv->device_path = g_strdup (drive_priv->device_path);
		volume_priv->unix_device = 0;

		_mate_vfs_volume_monitor_mounted (volume_monitor, volume);
		mate_vfs_volume_unref (volume);
	}
}

#endif

/* Without mate-vfs-daemon, initialise the volume data just once and hope it
 * doesn't change too much while the application is running.
 */

static void
initialise_the_volume_monitor (MateVFSVolumeMonitor *volume_monitor)
{
#ifdef G_OS_WIN32

	wchar_t volume_device[MAX_PATH];
	HANDLE volume;
	BOOL more = TRUE;

	volume = FindFirstVolumeW (volume_device, G_N_ELEMENTS (volume_device));

	if (volume == INVALID_HANDLE_VALUE)
		return;

	while (more) {
		handle_volume (volume_monitor, volume_device);
		more = FindNextVolumeW (volume, volume_device, G_N_ELEMENTS (volume_device));
	}

	FindVolumeClose (volume);
#else
#error Add code here for once-only initialisation of the volume pseudo-monitor
#endif
}

#endif

G_LOCK_DEFINE_STATIC (the_volume_monitor);
static MateVFSVolumeMonitor *the_volume_monitor = NULL;
static gboolean volume_monitor_was_shutdown = FALSE;

MateVFSVolumeMonitor *
_mate_vfs_get_volume_monitor_internal (gboolean create)
{
	G_LOCK (the_volume_monitor);
	
	if (the_volume_monitor == NULL &&
	    create &&
	    !volume_monitor_was_shutdown) {
#ifdef USE_DAEMON
		if (mate_vfs_get_is_daemon ()) {
			the_volume_monitor = g_object_new (mate_vfs_get_daemon_volume_monitor_type (), NULL);
		} else {
			the_volume_monitor = g_object_new (MATE_VFS_TYPE_VOLUME_MONITOR_CLIENT, NULL);
		}
#else
		the_volume_monitor = g_object_new (MATE_VFS_TYPE_VOLUME_MONITOR, NULL);
		initialise_the_volume_monitor (the_volume_monitor);
#endif
	}
	
	G_UNLOCK (the_volume_monitor);

	return the_volume_monitor;
}

/** 
 * mate_vfs_get_volume_monitor:
 *
 * Returns a pointer to the #MateVFSVolumeMonitor singleton.
 * #MateVFSVolumeMonitor is a singleton, this means it is guaranteed to
 * exist and be valid until mate_vfs_shutdown() is called. Consequently,
 * it doesn't need to be refcounted since mate-vfs will hold a reference to 
 * it until it is shut down.
 *
 * Returns: a pointer to the #MateVFSVolumeMonitor singleton.
 *
 * Since: 2.6
 */
MateVFSVolumeMonitor *
mate_vfs_get_volume_monitor (void)
{
	return _mate_vfs_get_volume_monitor_internal (TRUE);
}

void
_mate_vfs_volume_monitor_shutdown (void)
{
	G_LOCK (the_volume_monitor);
	
	if (the_volume_monitor != NULL) {
#ifdef USE_DAEMON
		if (!mate_vfs_get_is_daemon ()) {
			mate_vfs_volume_monitor_client_shutdown_private (MATE_VFS_VOLUME_MONITOR_CLIENT (the_volume_monitor));
		}
#endif
		mate_vfs_volume_monitor_unref (the_volume_monitor);
		the_volume_monitor = NULL;
	}
	volume_monitor_was_shutdown = TRUE;
	
	G_UNLOCK (the_volume_monitor);
}



#ifdef USE_HAL
MateVFSVolume *
_mate_vfs_volume_monitor_find_volume_by_hal_udi (MateVFSVolumeMonitor *volume_monitor,
						  const char *hal_udi)
{
	GList *l;
	MateVFSVolume *vol, *ret;

	/* Doesn't need locks, only called internally on main thread and doesn't write */
	
	ret = NULL;
	for (l = volume_monitor->priv->mtab_volumes; l != NULL; l = l->next) {
		vol = l->data;
		if (vol->priv != NULL && vol->priv->hal_udi != NULL && 
		    strcmp (vol->priv->hal_udi, hal_udi) == 0) {
			ret = vol;
			break;
		}
	}

	/* burn:/// and cdda:// optical discs are by the hal backend added as VFS_MOUNT */
	if (ret == NULL) {
		for (l = volume_monitor->priv->vfs_volumes; l != NULL; l = l->next) {
			vol = l->data;
			if (vol->priv != NULL && vol->priv->hal_drive_udi != NULL && 
			    strcmp (vol->priv->hal_udi, hal_udi) == 0) {
				ret = vol;
				break;
			}
		}
	}
	
	return ret;
}

MateVFSDrive *
_mate_vfs_volume_monitor_find_drive_by_hal_udi (MateVFSVolumeMonitor *volume_monitor,
						 const char           *hal_udi)
{
	GList *l;
	MateVFSDrive *drive, *ret;

	/* Doesn't need locks, only called internally on main thread and doesn't write */
	
	ret = NULL;
	for (l = volume_monitor->priv->fstab_drives; l != NULL; l = l->next) {
		drive = l->data;
		if (drive->priv != NULL && drive->priv->hal_udi != NULL &&
		    strcmp (drive->priv->hal_udi, hal_udi) == 0) {
			ret = drive;
			break;
		}
	}

	return ret;
}

MateVFSVolume *
_mate_vfs_volume_monitor_find_volume_by_hal_drive_udi (MateVFSVolumeMonitor *volume_monitor,
							const char *hal_drive_udi)
{
	GList *l;
	MateVFSVolume *vol, *ret;

	/* Doesn't need locks, only called internally on main thread and doesn't write */
	
	ret = NULL;
	for (l = volume_monitor->priv->mtab_volumes; l != NULL; l = l->next) {
		vol = l->data;
		if (vol->priv != NULL && vol->priv->hal_drive_udi != NULL && 
		    strcmp (vol->priv->hal_drive_udi, hal_drive_udi) == 0) {
			ret = vol;
			break;
		}
	}

	/* burn:/// and cdda:// optical discs are by the hal backend added as VFS_MOUNT */
	if (ret == NULL) {
		for (l = volume_monitor->priv->vfs_volumes; l != NULL; l = l->next) {
			vol = l->data;
			if (vol->priv != NULL && vol->priv->hal_drive_udi != NULL && 
			    strcmp (vol->priv->hal_drive_udi, hal_drive_udi) == 0) {
				ret = vol;
				break;
			}
		}
	}
	
	return ret;
}

MateVFSDrive *
_mate_vfs_volume_monitor_find_drive_by_hal_drive_udi (MateVFSVolumeMonitor *volume_monitor,
						       const char           *hal_drive_udi)
{
	GList *l;
	MateVFSDrive *drive, *ret;

	/* Doesn't need locks, only called internally on main thread and doesn't write */
	
	ret = NULL;
	for (l = volume_monitor->priv->fstab_drives; l != NULL; l = l->next) {
		drive = l->data;
		if (drive->priv != NULL && drive->priv->hal_drive_udi != NULL &&
		    strcmp (drive->priv->hal_drive_udi, hal_drive_udi) == 0) {
			ret = drive;
			break;
		}
	}

	return ret;
}
#endif /* USE_HAL */

MateVFSVolume *
_mate_vfs_volume_monitor_find_volume_by_device_path (MateVFSVolumeMonitor *volume_monitor,
						      const char *device_path)
{
	GList *l;
	MateVFSVolume *vol, *ret;

	/* Doesn't need locks, only called internally on main thread and doesn't write */
	
	ret = NULL;
	for (l = volume_monitor->priv->mtab_volumes; l != NULL; l = l->next) {
		vol = l->data;
		if (vol->priv != NULL && 
		    vol->priv->device_path != NULL && /* Hmm */
		    strcmp (vol->priv->device_path, device_path) == 0) {
			ret = vol;
			break;
		}
	}
	
	return ret;
}

MateVFSDrive *
_mate_vfs_volume_monitor_find_drive_by_device_path (MateVFSVolumeMonitor *volume_monitor,
						     const char *device_path)
{
	GList *l;
	MateVFSDrive *drive, *ret;

	/* Doesn't need locks, only called internally on main thread and doesn't write */
	
	ret = NULL;
	for (l = volume_monitor->priv->fstab_drives; l != NULL; l = l->next) {
		drive = l->data;
		if (drive->priv != NULL && 
		    drive->priv->device_path != NULL && /* Hmm */
		    strcmp (drive->priv->device_path, device_path) == 0) {
			ret = drive;
			break;
		}
	}

	return ret;
}


MateVFSVolume *
_mate_vfs_volume_monitor_find_mtab_volume_by_activation_uri (MateVFSVolumeMonitor *volume_monitor,
							      const char *activation_uri)
{
	GList *l;
	MateVFSVolume *vol, *ret;

	/* Doesn't need locks, only called internally on main thread and doesn't write */
	
	ret = NULL;
	for (l = volume_monitor->priv->mtab_volumes; l != NULL; l = l->next) {
		vol = l->data;
		if (vol->priv->activation_uri != NULL &&
		    strcmp (vol->priv->activation_uri, activation_uri) == 0) {
			ret = vol;
			break;
		}
	}
	
	return ret;
}

MateVFSDrive *
_mate_vfs_volume_monitor_find_fstab_drive_by_activation_uri (MateVFSVolumeMonitor *volume_monitor,
							      const char            *activation_uri)
{
	GList *l;
	MateVFSDrive *drive, *ret;

	/* Doesn't need locks, only called internally on main thread and doesn't write */
	
	ret = NULL;
	for (l = volume_monitor->priv->fstab_drives; l != NULL; l = l->next) {
		drive = l->data;
		if (drive->priv->activation_uri != NULL &&
		    strcmp (drive->priv->activation_uri, activation_uri) == 0) {
			ret = drive;
			break;
		}
	}

	return ret;
}

MateVFSVolume *
_mate_vfs_volume_monitor_find_connected_server_by_mateconf_id (MateVFSVolumeMonitor *volume_monitor,
							     const char            *id)
{
	GList *l;
	MateVFSVolume *vol, *ret;

	/* Doesn't need locks, only called internally on main thread and doesn't write */
	
	ret = NULL;
	for (l = volume_monitor->priv->server_volumes; l != NULL; l = l->next) {
		vol = l->data;
		if (vol->priv->mateconf_id != NULL &&
		    strcmp (vol->priv->mateconf_id, id) == 0) {
			ret = vol;
			break;
		}
	}

	return ret;
}

/** 
 * mate_vfs_volume_monitor_get_volume_by_id:
 * @volume_monitor: a #MateVFSVolumeMonitor.
 * @id: #MateVFSVolume id to look for.
 *
 * Looks for a #MateVFSVolume whose id is @id. A valid @volume_monitor to pass
 * to this function can be acquired using mate_vfs_get_volume_monitor().
 *
 * Returns: the #MateVFSVolume corresponding to @id, or %NULL if no 
 * #MateVFSVolume with a matching id could be found. The caller owns a 
 * reference on the returned volume, and must call mate_vfs_volume_unref()
 * when it no longer needs it.
 *
 * Since: 2.6
 */
MateVFSVolume *
mate_vfs_volume_monitor_get_volume_by_id (MateVFSVolumeMonitor *volume_monitor,
					   gulong                 id)
{
	GList *l;
	MateVFSVolume *vol;

	g_mutex_lock (volume_monitor->priv->mutex);
	
	for (l = volume_monitor->priv->mtab_volumes; l != NULL; l = l->next) {
		vol = l->data;
		if (vol->priv->id == id) {
			mate_vfs_volume_ref (vol);
			g_mutex_unlock (volume_monitor->priv->mutex);
			return vol;
		}
	}
	for (l = volume_monitor->priv->server_volumes; l != NULL; l = l->next) {
		vol = l->data;
		if (vol->priv->id == id) {
			mate_vfs_volume_ref (vol);
			g_mutex_unlock (volume_monitor->priv->mutex);
			return vol;
		}
	}
	for (l = volume_monitor->priv->vfs_volumes; l != NULL; l = l->next) {
		vol = l->data;
		if (vol->priv->id == id) {
			mate_vfs_volume_ref (vol);
			g_mutex_unlock (volume_monitor->priv->mutex);
			return vol;
		}
	}
	
	g_mutex_unlock (volume_monitor->priv->mutex);

	return NULL;
}

/** 
 * mate_vfs_volume_monitor_get_drive_by_id:
 * @volume_monitor: a #MateVFSVolumeMonitor.
 * @id: the #MateVFSVolume id to look for.
 *
 * Looks for a #MateVFSDrive whose id is @id. A valid @volume_monitor to pass
 * to this function can be acquired using mate_vfs_get_volume_monitor()
 *
 * Returns: the #MateVFSDrive corresponding to @id, or %NULL if no 
 * #MateVFSDrive with a matching id could be found. The caller owns a 
 * reference on the returned drive, and must call mate_vfs_drive_unref()
 * when it no longer needs it.
 *
 * Since: 2.6
 */
MateVFSDrive *
mate_vfs_volume_monitor_get_drive_by_id  (MateVFSVolumeMonitor *volume_monitor,
					   gulong                 id)
{
	GList *l;
	MateVFSDrive *drive;

	g_mutex_lock (volume_monitor->priv->mutex);

	for (l = volume_monitor->priv->fstab_drives; l != NULL; l = l->next) {
		drive = l->data;
		if (drive->priv->id == id) {
			mate_vfs_drive_ref (drive);
			g_mutex_unlock (volume_monitor->priv->mutex);
			return drive;
		}
	}
	for (l = volume_monitor->priv->vfs_drives; l != NULL; l = l->next) {
		drive = l->data;
		if (drive->priv->id == id) {
			mate_vfs_drive_ref (drive);
			g_mutex_unlock (volume_monitor->priv->mutex);
			return drive;
		}
	}

	g_mutex_unlock (volume_monitor->priv->mutex);
	
	return NULL;
}

void
_mate_vfs_volume_monitor_unmount_all (MateVFSVolumeMonitor *volume_monitor)
{
	GList *l, *volumes;
	MateVFSVolume *volume;

	volumes = mate_vfs_volume_monitor_get_mounted_volumes (volume_monitor);
	
	for (l = volumes; l != NULL; l = l->next) {
		volume = l->data;
		_mate_vfs_volume_monitor_unmounted (volume_monitor, volume);
		mate_vfs_volume_unref (volume);
	}
	
	g_list_free (volumes);
}

void
_mate_vfs_volume_monitor_disconnect_all (MateVFSVolumeMonitor *volume_monitor)
{
	GList *l, *drives;
	MateVFSDrive *drive;

	drives = mate_vfs_volume_monitor_get_connected_drives (volume_monitor);
	
	for (l = drives; l != NULL; l = l->next) {
		drive = l->data;
		_mate_vfs_volume_monitor_disconnected (volume_monitor, drive);
		mate_vfs_drive_unref (drive);
	}

	g_list_free (drives);
}

/** 
 * mate_vfs_volume_monitor_emit_pre_unmount:
 * @volume_monitor: a #MateVFSVolumeMonitor.
 * @volume: a #MateVFSVolume.
 *
 * Emits the "pre-unmount" signal on @volume. 
 *
 * Since: 2.6
 */
void
mate_vfs_volume_monitor_emit_pre_unmount (MateVFSVolumeMonitor *volume_monitor,
					   MateVFSVolume        *volume)
{
	g_signal_emit (volume_monitor, volume_monitor_signals[VOLUME_PRE_UNMOUNT], 0, volume);
}

void
_mate_vfs_volume_monitor_mounted (MateVFSVolumeMonitor *volume_monitor,
				   MateVFSVolume        *volume)
{
	mate_vfs_volume_ref (volume);
	
	g_mutex_lock (volume_monitor->priv->mutex);
	switch (volume->priv->volume_type) {
	case MATE_VFS_VOLUME_TYPE_MOUNTPOINT:
		volume_monitor->priv->mtab_volumes = g_list_prepend (volume_monitor->priv->mtab_volumes,
								     volume);
		break;
	case MATE_VFS_VOLUME_TYPE_CONNECTED_SERVER:
		volume_monitor->priv->server_volumes = g_list_prepend (volume_monitor->priv->server_volumes,
								       volume);
		break;
	case MATE_VFS_VOLUME_TYPE_VFS_MOUNT:
		volume_monitor->priv->vfs_volumes = g_list_prepend (volume_monitor->priv->vfs_volumes,
								    volume);
		break;
	default:
		g_assert_not_reached ();
	}
		
	volume->priv->is_mounted = 1;
	g_mutex_unlock (volume_monitor->priv->mutex);
	
	g_signal_emit (volume_monitor, volume_monitor_signals[VOLUME_MOUNTED], 0, volume);
}

void
_mate_vfs_volume_monitor_unmounted (MateVFSVolumeMonitor *volume_monitor,
				     MateVFSVolume        *volume)
{
	MateVFSDrive *drive;
	g_mutex_lock (volume_monitor->priv->mutex);
	volume_monitor->priv->mtab_volumes = g_list_remove (volume_monitor->priv->mtab_volumes, volume);
	volume_monitor->priv->server_volumes = g_list_remove (volume_monitor->priv->server_volumes, volume);
	volume_monitor->priv->vfs_volumes = g_list_remove (volume_monitor->priv->vfs_volumes, volume);
	volume->priv->is_mounted = 0;
	g_mutex_unlock (volume_monitor->priv->mutex);

	g_signal_emit (volume_monitor, volume_monitor_signals[VOLUME_UNMOUNTED], 0, volume);
	
	drive = volume->priv->drive;
	if (drive != NULL) {
		mate_vfs_volume_unset_drive_private (volume, drive);
		mate_vfs_drive_remove_volume_private (drive, volume);
	}
	
	mate_vfs_volume_unref (volume);
}


void
_mate_vfs_volume_monitor_connected (MateVFSVolumeMonitor *volume_monitor,
				     MateVFSDrive        *drive)
{
	mate_vfs_drive_ref (drive);
	
	g_mutex_lock (volume_monitor->priv->mutex);
	volume_monitor->priv->fstab_drives = g_list_prepend (volume_monitor->priv->fstab_drives, drive);
	drive->priv->is_connected = 1;
	g_mutex_unlock (volume_monitor->priv->mutex);
	
	g_signal_emit (volume_monitor, volume_monitor_signals[DRIVE_CONNECTED], 0, drive);
	
}

void
_mate_vfs_volume_monitor_disconnected (MateVFSVolumeMonitor *volume_monitor,
					MateVFSDrive         *drive)
{
	GList *vol_list;
	GList *current_vol;
	
	g_mutex_lock (volume_monitor->priv->mutex);
	volume_monitor->priv->fstab_drives = g_list_remove (volume_monitor->priv->fstab_drives, drive);
	drive->priv->is_connected = 0;
	g_mutex_unlock (volume_monitor->priv->mutex);

	vol_list = mate_vfs_drive_get_mounted_volumes (drive);	

	for (current_vol = vol_list; current_vol != NULL; current_vol = current_vol->next) {  
		MateVFSVolume *volume;
		volume = MATE_VFS_VOLUME (vol_list->data);

		mate_vfs_volume_unset_drive_private (volume, drive);
		mate_vfs_drive_remove_volume_private (drive, volume);
	}

	g_list_free (vol_list);

	g_signal_emit (volume_monitor, volume_monitor_signals[DRIVE_DISCONNECTED], 0, drive);

	mate_vfs_drive_unref (drive);
}

/** 
 * mate_vfs_volume_monitor_get_mounted_volumes:
 * @volume_monitor: a #MateVFSVolumeMonitor.
 *
 * Gets the list of all the mounted #MateVFSVolume volumes.
 *
 * Returns: #GList of #MateVFSVolume. The #MateVFSVolume objects must be 
 * unreffed by the caller when no longer needed with mate_vfs_volume_unref()
 * and the #GList must be freed.
 *
 * Since: 2.6
 */
GList *
mate_vfs_volume_monitor_get_mounted_volumes (MateVFSVolumeMonitor *volume_monitor)
{
	GList *ret;

	g_mutex_lock (volume_monitor->priv->mutex);
	ret = g_list_copy (volume_monitor->priv->mtab_volumes);
	ret = g_list_concat (ret,
			      g_list_copy (volume_monitor->priv->server_volumes));
	ret = g_list_concat (ret,
			      g_list_copy (volume_monitor->priv->vfs_volumes));
	g_list_foreach (ret,
			(GFunc)mate_vfs_volume_ref, NULL);
	g_mutex_unlock (volume_monitor->priv->mutex);

	return ret;
}

/** 
 * mate_vfs_volume_monitor_get_connected_drives:
 * @volume_monitor: a #MateVFSVolumeMonitor.
 *
 * Returns a #GList of all drives connected to the machine.
 * The #MateVFSDrive objects must be unreffed by the caller when
 * no longer needed with mate_vfs_drive_unref() and the #GList must
 * be freed.
 *
 * Returns: a #GList of all connected drives.
 *
 * Since: 2.6
 */
GList *
mate_vfs_volume_monitor_get_connected_drives (MateVFSVolumeMonitor *volume_monitor)
{
	GList *ret;

	g_mutex_lock (volume_monitor->priv->mutex);
	ret = g_list_copy (volume_monitor->priv->fstab_drives);
	g_list_foreach (ret,
			(GFunc)mate_vfs_drive_ref, NULL);
	g_mutex_unlock (volume_monitor->priv->mutex);

	return ret;
}


static gboolean
volume_name_is_unique (MateVFSVolumeMonitor *volume_monitor,
		       const char *name)
{
	GList *l;
	MateVFSVolume *volume;

	for (l = volume_monitor->priv->mtab_volumes; l != NULL; l = l->next) {
		volume = l->data;
		if (volume->priv->is_user_visible && strcmp (volume->priv->display_name, name) == 0) {
			return FALSE;
		}
	}
	for (l = volume_monitor->priv->server_volumes; l != NULL; l = l->next) {
		volume = l->data;
		if (volume->priv->is_user_visible && strcmp (volume->priv->display_name, name) == 0) {
			return FALSE;
		}
	}
	for (l = volume_monitor->priv->vfs_volumes; l != NULL; l = l->next) {
		volume = l->data;
		if (volume->priv->is_user_visible && strcmp (volume->priv->display_name, name) == 0) {
			return FALSE;
		}
	}

	return TRUE;
}

char *
_mate_vfs_volume_monitor_uniquify_volume_name (MateVFSVolumeMonitor *volume_monitor,
						const char *name)
{
	int index;
	char *unique_name;

	index = 1;
	
	unique_name = g_strdup (name);
	while (!volume_name_is_unique (volume_monitor, unique_name)) {
		g_free (unique_name);
		index++;
		unique_name = g_strdup_printf ("%s (%d)", name, index);
	}

	return unique_name;
}

static gboolean
drive_name_is_unique (MateVFSVolumeMonitor *volume_monitor,
		       const char *name)
{
	GList *l;
	MateVFSDrive *drive;

	for (l = volume_monitor->priv->fstab_drives; l != NULL; l = l->next) {
		drive = l->data;
		if (drive->priv->is_user_visible && strcmp (drive->priv->display_name, name) == 0) {
			return FALSE;
		}
	}
	for (l = volume_monitor->priv->vfs_drives; l != NULL; l = l->next) {
		drive = l->data;
		if (drive->priv->is_user_visible && strcmp (drive->priv->display_name, name) == 0) {
			return FALSE;
		}
	}

	return TRUE;
}


char *
_mate_vfs_volume_monitor_uniquify_drive_name (MateVFSVolumeMonitor *volume_monitor,
					       const char *name)
{
	int index;
	char *unique_name;

	index = 1;
	
	unique_name = g_strdup (name);
	while (!drive_name_is_unique (volume_monitor, unique_name)) {
		g_free (unique_name);
		index++;
		unique_name = g_strdup_printf ("%s (%d)", name, index);
	}

	return unique_name;
}

/** 
 * mate_vfs_volume_monitor_get_volume_for_path:
 * @volume_monitor: a #MateVFSVolumeMonitor.
 * @path: string representing a path.
 *
 * Returns the #MateVFSVolume corresponding to @path, or %NULL.
 *
 * The volume referring to @path is found by calling %stat on @path,
 * and then iterating through the list of volumes that refer
 * to currently mounted local file systems.
 * The first volume in this list maching the @path's UNIX device
 * is returned.
 *
 * If the %stat on @path was not successful, or no volume matches @path,
 * or %NULL is returned.
 *
 * Returns: the #MateVFSVolume corresponding to the @path, or %NULL.  Volume returned 
 * must be unreffed by the caller with mate_vfs_volume_unref().
 *
 * Since: 2.6
 */
MateVFSVolume *
mate_vfs_volume_monitor_get_volume_for_path  (MateVFSVolumeMonitor *volume_monitor,
					       const char            *path)
{
	struct stat statbuf;
	dev_t device;
	GList *l;
	MateVFSVolume *volume, *res;

	if (g_stat (path, &statbuf) != 0)
		return NULL;

	device = statbuf.st_dev;

	res = NULL;
	g_mutex_lock (volume_monitor->priv->mutex);
	for (l = volume_monitor->priv->mtab_volumes; l != NULL; l = l->next) {
		volume = l->data;
		if (volume->priv->unix_device == device) {
			res = mate_vfs_volume_ref (volume);
			break;
		}
	}
	g_mutex_unlock (volume_monitor->priv->mutex);
	
	return res;
}
