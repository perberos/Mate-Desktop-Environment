/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* computer-method.c - The 

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

   Authors:
         Alexander Larsson <alexl@redhat.com>
 */

#include <config.h>

#include <stdlib.h>
#include <string.h>

#include <libmatevfs/mate-vfs-module-shared.h>
#include <libmatevfs/mate-vfs-module.h>
#include <libmatevfs/mate-vfs-volume-monitor.h>
#include <libmatevfs/mate-vfs-utils.h>
#include <libmatevfs/mate-vfs-monitor-private.h>

#include <glib/gi18n-lib.h>

typedef enum {
	COMPUTER_HOME_LINK,
	COMPUTER_ROOT_LINK,
	COMPUTER_DRIVE,
	COMPUTER_VOLUME,
	COMPUTER_NETWORK_LINK
} ComputerFileType;

typedef struct {
	char *file_name; /* Not encoded */
	ComputerFileType type;

	MateVFSVolume *volume;
	MateVFSDrive *drive;
	
	GList *file_monitors;
} ComputerFile;

typedef struct {
	GList *files;
	GList *dir_monitors;
} ComputerDir;

typedef struct {
	MateVFSMonitorType type;
	ComputerFile *file;
} ComputerMonitor;

static ComputerDir *root_dir = NULL;

G_LOCK_DEFINE_STATIC (root_dir);

static ComputerFile *
computer_file_new (ComputerFileType type)
{
	ComputerFile *file;

	file = g_new0 (ComputerFile, 1);
	file->type = type;
	
	return file;
}

static void
computer_file_free (ComputerFile *file)
{
	GList *l;
	ComputerMonitor *monitor;
	
	if (file->type == COMPUTER_VOLUME) {
		mate_vfs_volume_unref (file->volume);
	}
	if (file->type == COMPUTER_DRIVE) {
		mate_vfs_drive_unref (file->drive);
	}
	
	for (l = file->file_monitors; l != NULL; l = l->next) {
		monitor = l->data;
		monitor->file = NULL;
	}
	g_list_free (file->file_monitors);
	
	g_free (file);
}

static MateVFSURI *
computer_file_get_uri (ComputerFile *file) {
	MateVFSURI *uri;
	MateVFSURI *tmp;

	uri = mate_vfs_uri_new ("computer:///");
	if (file != NULL) {
		tmp = uri;
		uri = mate_vfs_uri_append_file_name (uri, file->file_name);
		mate_vfs_uri_unref (tmp);
	}
	return uri;
}


static void
computer_file_add (ComputerDir *dir, ComputerFile *file)
{
	ComputerMonitor *monitor;
	GList *l;
	MateVFSURI *uri;

	dir->files = g_list_prepend (dir->files, file);

	uri = computer_file_get_uri (file);
	for (l = dir->dir_monitors; l != NULL; l = l->next) {
		monitor = l->data;
		mate_vfs_monitor_callback ((MateVFSMethodHandle *)monitor,
					    uri,
					    MATE_VFS_MONITOR_EVENT_CREATED);
	}
	mate_vfs_uri_unref (uri);
}

static void
computer_file_remove (ComputerDir *dir, ComputerFile *file)
{
	ComputerMonitor *monitor;
	GList *l;
	MateVFSURI *uri;
	
	dir->files = g_list_remove (dir->files, file);

	uri = computer_file_get_uri (file);
	for (l = dir->dir_monitors; l != NULL; l = l->next) {
		monitor = l->data;
		mate_vfs_monitor_callback ((MateVFSMethodHandle *)monitor,
					    uri,
					    MATE_VFS_MONITOR_EVENT_DELETED);
	}
	for (l = file->file_monitors; l != NULL; l = l->next) {
		monitor = l->data;
		mate_vfs_monitor_callback ((MateVFSMethodHandle *)monitor,
					    uri,
					    MATE_VFS_MONITOR_EVENT_DELETED);
	}
	mate_vfs_uri_unref (uri);
	
	computer_file_free (file);
}

static void
computer_file_changed (ComputerDir *dir, ComputerFile *file)
{
	ComputerMonitor *monitor;
	GList *l;
	MateVFSURI *uri;

	uri = computer_file_get_uri (file);
	for (l = dir->dir_monitors; l != NULL; l = l->next) {
		monitor = l->data;
		mate_vfs_monitor_callback ((MateVFSMethodHandle *)monitor,
					    uri,
					    MATE_VFS_MONITOR_EVENT_CHANGED);
	}
	for (l = file->file_monitors; l != NULL; l = l->next) {
		monitor = l->data;
		mate_vfs_monitor_callback ((MateVFSMethodHandle *)monitor,
					    uri,
					    MATE_VFS_MONITOR_EVENT_CHANGED);
	}
	mate_vfs_uri_unref (uri);
}

static ComputerFile *
get_volume_file (ComputerDir *dir, MateVFSVolume *volume)
{
	GList *l;
	ComputerFile *file;

	for (l = dir->files; l != NULL; l = l->next) {
		file = l->data;
		if (file->type == COMPUTER_VOLUME &&
		    file->volume == volume) {
			return file;
		}
	}
	return NULL;
}

static ComputerFile *
get_drive_file (ComputerDir *dir, MateVFSDrive *drive)
{
	GList *l;
	ComputerFile *file;

	for (l = dir->files; l != NULL; l = l->next) {
		file = l->data;
		if (file->type == COMPUTER_DRIVE &&
		    file->drive == drive) {
			return file;
		}
	}
	return NULL;
}

static ComputerFile *
get_file (ComputerDir *dir, char *name)
{
	GList *l;
	ComputerFile *file;

	if (!name) {
		return NULL;
	}

	for (l = dir->files; l != NULL; l = l->next) {
		file = l->data;
		if (strcmp (file->file_name, name) == 0) {
			return file;
		}
	}
	return NULL;
}

static char *
build_file_name (char *name, char *extension)
{
	char *escaped;
	char *ret;
	
	escaped = mate_vfs_escape_string (name);
	ret = g_strconcat (escaped, extension, NULL);
	g_free (escaped);
	
	return ret;
}

static void
volume_mounted (MateVFSVolumeMonitor *volume_monitor,
		MateVFSVolume	       *volume,
		ComputerDir            *dir)
{
	ComputerFile *file;
	MateVFSDrive *drive;
	char *name;
	
	G_LOCK (root_dir);
	if (mate_vfs_volume_is_user_visible (volume)) {
		drive = mate_vfs_volume_get_drive (volume);
		if (drive == NULL) {
			file = computer_file_new (COMPUTER_VOLUME);
			name = mate_vfs_volume_get_display_name (volume);
			file->file_name = build_file_name (name, ".volume");
			g_free (name);
			file->volume = mate_vfs_volume_ref (volume);
			computer_file_add (dir, file);
		} else {
			file = get_drive_file (dir, drive);
			if (file != NULL) {
				computer_file_changed (dir, file);
			}
		}
		mate_vfs_drive_unref (drive);
	}
	G_UNLOCK (root_dir);
}

static void
volume_unmounted (MateVFSVolumeMonitor *volume_monitor,
		  MateVFSVolume	*volume,
		  ComputerDir           *dir)
{
	ComputerFile *file;
	MateVFSDrive *drive;
	
	G_LOCK (root_dir);
	drive = mate_vfs_volume_get_drive (volume);
	if (drive != NULL) {
		file = get_drive_file (dir, drive);
		if (file != NULL) {
			computer_file_changed (dir, file);
		}
		mate_vfs_drive_unref (drive);
	}
	
	file = get_volume_file (dir, volume);
	if (file != NULL) {
		computer_file_remove (dir, file);
	}
	G_UNLOCK (root_dir);
}

static void
drive_connected (MateVFSVolumeMonitor *volume_monitor,
		 MateVFSDrive	       *drive,
		 ComputerDir           *dir)
{
	ComputerFile *file;
	char *name;
	
	G_LOCK (root_dir);
	file = computer_file_new (COMPUTER_DRIVE);
	name = mate_vfs_drive_get_display_name (drive);
	file->file_name = build_file_name (name, ".drive");
	g_free (name);
	file->drive = mate_vfs_drive_ref (drive);
	computer_file_add (dir, file);
	G_UNLOCK (root_dir);
}

static void
drive_disconnected (MateVFSVolumeMonitor *volume_monitor,
		    MateVFSDrive	  *drive,
		    ComputerDir           *dir)
{
	ComputerFile *file;
	
	G_LOCK (root_dir);
	file = get_drive_file (dir, drive);
	if (file != NULL) {
		computer_file_remove (dir, file);
	}
	G_UNLOCK (root_dir);
}

static void
fill_root (ComputerDir *dir)
{
	MateVFSVolumeMonitor *monitor;
	MateVFSVolume *volume;
	MateVFSDrive *drive;
	GList *volumes, *drives, *l;
	ComputerFile *file;
	char *name;
	
	monitor = mate_vfs_get_volume_monitor ();

#if 0
	/* Don't want home in computer:// */
	file = computer_file_new (COMPUTER_HOME_LINK);
	file->file_name = g_strdup ("Home.desktop");
	computer_file_add (dir, file);
#endif
	
	file = computer_file_new (COMPUTER_ROOT_LINK);
	file->file_name = g_strdup ("Filesystem.desktop");
	computer_file_add (dir, file);
	
	file = computer_file_new (COMPUTER_NETWORK_LINK);
	file->file_name = g_strdup ("Network.desktop");
	computer_file_add (dir, file);
	
	volumes = mate_vfs_volume_monitor_get_mounted_volumes (monitor);
	drives = mate_vfs_volume_monitor_get_connected_drives (monitor);
	
	for (l = drives; l != NULL; l = l->next) {
		drive = l->data;
		file = computer_file_new (COMPUTER_DRIVE);
		name = mate_vfs_drive_get_display_name (drive);
		file->file_name = build_file_name (name, ".drive");
		g_free (name);
		file->drive = mate_vfs_drive_ref (drive);
		computer_file_add (dir, file);
	}
	
	for (l = volumes; l != NULL; l = l->next) {
		volume = l->data;
		if (mate_vfs_volume_is_user_visible (volume)) {
			drive = mate_vfs_volume_get_drive (volume);
			if (drive == NULL) {
				file = computer_file_new (COMPUTER_VOLUME);
				name = mate_vfs_volume_get_display_name (volume);
				file->file_name = build_file_name (name, ".volume");
				g_free (name);
				file->volume = mate_vfs_volume_ref (volume);
				computer_file_add (dir, file);
			}
			mate_vfs_drive_unref (drive);
		}
	}

	g_list_foreach (drives, (GFunc) mate_vfs_drive_unref, NULL);
	g_list_foreach (volumes, (GFunc) mate_vfs_volume_unref, NULL);
	g_list_free (drives);
	g_list_free (volumes);

	g_signal_connect (monitor, "volume_mounted",
			  G_CALLBACK (volume_mounted), dir);
	g_signal_connect (monitor, "volume_unmounted",
			  G_CALLBACK (volume_unmounted), dir);
	g_signal_connect (monitor, "drive_connected",
			  G_CALLBACK (drive_connected), dir);
	g_signal_connect (monitor, "drive_disconnected",
			  G_CALLBACK (drive_disconnected), dir);
	
}

static ComputerDir *
get_root (void)
{
	G_LOCK (root_dir);
	if (root_dir == NULL) {
		root_dir = g_new0 (ComputerDir, 1);
		fill_root (root_dir);
	}
	
	G_UNLOCK (root_dir);

	return root_dir;
}

typedef struct {
	char *data;
	int len;
	int pos;
} FileHandle;

static FileHandle *
file_handle_new (char *data)
{
	FileHandle *result;
	result = g_new (FileHandle, 1);

	result->data = data;
	result->len = strlen (data);
	result->pos = 0;

	return result;
}

static void
file_handle_destroy (FileHandle *handle)
{
	g_free (handle->data);
	g_free (handle);
}

static char *
get_data_for_volume (MateVFSVolume *volume)
{
	char *uri;
	char *name;
	char *icon;
	char *data;

	uri = mate_vfs_volume_get_activation_uri (volume);
	name = mate_vfs_volume_get_display_name (volume);
	icon = mate_vfs_volume_get_icon (volume);
	data = g_strdup_printf ("[Desktop Entry]\n"
				"Encoding=UTF-8\n"
				"Name=%s\n"
				"Type=FSDevice\n"
				"Icon=%s\n"
				"URL=%s\n"
				"X-Mate-Volume=%ld\n",
				name,
				icon,
				uri,
				mate_vfs_volume_get_id (volume));

	g_free (uri);
	g_free (name);
	g_free (icon);
	
	return data;
}

static char *
get_data_for_drive (MateVFSDrive *drive)
{
	char *uri;
	char *name;
	char *icon;
	char *data;
	char *tmp1, *tmp2;
	GList *volume_list;

	volume_list = mate_vfs_drive_get_mounted_volumes (drive);
	if (volume_list != NULL) {
		MateVFSVolume *volume;
		volume = MATE_VFS_VOLUME (volume_list->data);

		uri = mate_vfs_volume_get_activation_uri (volume);
		tmp1 = mate_vfs_drive_get_display_name (drive);
		tmp2 = mate_vfs_volume_get_display_name (volume);
		if (strcmp (tmp1, tmp2) != 0) {
			name = g_strconcat (tmp1, ": ", tmp2, NULL);
		} else {
			name = g_strdup (tmp1);
		}
		g_free (tmp1);
		g_free (tmp2);
		icon = mate_vfs_volume_get_icon (volume);
		mate_vfs_volume_unref (volume);
	} else {
		uri = mate_vfs_drive_get_activation_uri (drive);
		name = mate_vfs_drive_get_display_name (drive);
		icon = mate_vfs_drive_get_icon (drive);
	}
	
	data = g_strdup_printf ("[Desktop Entry]\n"
				"Encoding=UTF-8\n"
				"Name=%s\n"
				"Type=FSDevice\n"
				"Icon=%s\n"
				"URL=%s\n"
				"X-Mate-Drive=%ld\n",
				name,
				icon,
				(uri != NULL) ? uri : "",
				mate_vfs_drive_get_id (drive));
	g_free (uri);
	g_free (name);
	g_free (icon);
	
	return data;
}

static char *
get_data_for_network (void)
{
	char *data;

	data = g_strdup_printf ("[Desktop Entry]\n"
				"Encoding=UTF-8\n"
				"Name=%s\n"
				"Type=Link\n"
				"Icon=mate-fs-network\n"
				"URL=network://\n",
				_("Network"));

	return data;
}

static char *
get_data_for_home (void)
{
	char *data;
	char *uri;

	uri = mate_vfs_get_uri_from_local_path (g_get_home_dir ());
	data = g_strdup_printf ("[Desktop Entry]\n"
				"Encoding=UTF-8\n"
				"Name=%s\n"
				"Type=Link\n"
				"Icon=mate-fs-home\n"
				"URL=%s\n",
				_("Home"),
				uri);
	g_free (uri);

	return data;
}

static char *
get_data_for_root (void)
{
	char *data;

	data = g_strdup_printf ("[Desktop Entry]\n"
				"Encoding=UTF-8\n"
				"Name=%s\n"
				"Type=Link\n"
				"Icon=mate-dev-harddisk\n"
				"URL=file:///\n",
				_("Filesystem"));

	return data;
}

static MateVFSResult
do_open (MateVFSMethod *method,
	 MateVFSMethodHandle **method_handle,
	 MateVFSURI *uri,
	 MateVFSOpenMode mode,
	 MateVFSContext *context)
{
	FileHandle *file_handle;
	ComputerFile *file;
	ComputerDir *dir;
	char *data;
	char *name;
	
	_MATE_VFS_METHOD_PARAM_CHECK (method_handle != NULL);
	_MATE_VFS_METHOD_PARAM_CHECK (uri != NULL);

	if (mode & MATE_VFS_OPEN_WRITE) {
		return MATE_VFS_ERROR_NOT_PERMITTED;
	}

	if (strcmp (uri->text, "/") == 0) {
		return MATE_VFS_ERROR_NOT_PERMITTED;
	}

	dir = get_root ();

	G_LOCK (root_dir);
	
	name = mate_vfs_uri_extract_short_name (uri);
	file = get_file (dir, name);
	g_free (name);
	
	if (file == NULL) {
		G_UNLOCK (root_dir);
		return MATE_VFS_ERROR_NOT_FOUND;
	}

	data = NULL;
	switch (file->type) {
	case COMPUTER_HOME_LINK:
		data = get_data_for_home ();
		break;
	case COMPUTER_ROOT_LINK:
		data = get_data_for_root ();
		break;
	case COMPUTER_NETWORK_LINK:
		data = get_data_for_network ();
		break;
	case COMPUTER_DRIVE:
		data = get_data_for_drive (file->drive);
		break;
	case COMPUTER_VOLUME:
		data = get_data_for_volume (file->volume);
		break;
	}

	G_UNLOCK (root_dir);
		
	file_handle = file_handle_new (data);

	*method_handle = (MateVFSMethodHandle *) file_handle;

	return MATE_VFS_OK;
}

static MateVFSResult
do_create (MateVFSMethod *method,
	   MateVFSMethodHandle **method_handle,
	   MateVFSURI *uri,
	   MateVFSOpenMode mode,
	   gboolean exclusive,
	   guint perm,
	   MateVFSContext *context)
{
	return MATE_VFS_ERROR_NOT_PERMITTED;
}

static MateVFSResult
do_close (MateVFSMethod *method,
	  MateVFSMethodHandle *method_handle,
	  MateVFSContext *context)
{
	FileHandle *file_handle;

	g_return_val_if_fail (method_handle != NULL, MATE_VFS_ERROR_INTERNAL);

	file_handle = (FileHandle *) method_handle;

	file_handle_destroy (file_handle);

	return MATE_VFS_OK;
}

static MateVFSResult
do_read (MateVFSMethod *method,
	 MateVFSMethodHandle *method_handle,
	 gpointer buffer,
	 MateVFSFileSize num_bytes,
	 MateVFSFileSize *bytes_read,
	 MateVFSContext *context)
{
	FileHandle *file_handle;
	int read_len;

	g_return_val_if_fail (method_handle != NULL, MATE_VFS_ERROR_INTERNAL);

	file_handle = (FileHandle *) method_handle;
	*bytes_read = 0;
	
	if (file_handle->pos >= file_handle->len) {
		return MATE_VFS_ERROR_EOF;
	}

	read_len = MIN (num_bytes, file_handle->len - file_handle->pos);

	memcpy (buffer, file_handle->data + file_handle->pos, read_len);
	*bytes_read = read_len;
	file_handle->pos += read_len;

	
	return MATE_VFS_OK;
}

static MateVFSResult
do_write (MateVFSMethod *method,
	  MateVFSMethodHandle *method_handle,
	  gconstpointer buffer,
	  MateVFSFileSize num_bytes,
	  MateVFSFileSize *bytes_written,
	  MateVFSContext *context)
{
	return MATE_VFS_ERROR_NOT_PERMITTED;
}


static MateVFSResult
do_seek (MateVFSMethod *method,
	 MateVFSMethodHandle *method_handle,
	 MateVFSSeekPosition whence,
	 MateVFSFileOffset offset,
	 MateVFSContext *context)
{
	FileHandle *file_handle;

	file_handle = (FileHandle *) method_handle;

	switch (whence) {
	case MATE_VFS_SEEK_START:
		file_handle->pos = offset;
		break;
	case MATE_VFS_SEEK_CURRENT:
		file_handle->pos += offset;
		break;
	case MATE_VFS_SEEK_END:
		file_handle->pos = file_handle->len + offset;
		break;
	}

	if (file_handle->pos < 0) {
		file_handle->pos = 0;
	}
	
	if (file_handle->pos > file_handle->len) {
		file_handle->pos = file_handle->len;
	}
	
	return MATE_VFS_OK;
}

static MateVFSResult
do_tell (MateVFSMethod *method,
	 MateVFSMethodHandle *method_handle,
	 MateVFSFileSize *offset_return)
{
	FileHandle *file_handle;

	file_handle = (FileHandle *) method_handle;

	*offset_return = file_handle->pos;
	return MATE_VFS_OK;
}


static MateVFSResult
do_truncate_handle (MateVFSMethod *method,
		    MateVFSMethodHandle *method_handle,
		    MateVFSFileSize where,
		    MateVFSContext *context)
{
	return MATE_VFS_ERROR_NOT_PERMITTED;
}

static MateVFSResult
do_truncate (MateVFSMethod *method,
	     MateVFSURI *uri,
	     MateVFSFileSize where,
	     MateVFSContext *context)
{
	return MATE_VFS_ERROR_NOT_PERMITTED;
}

typedef struct {
	MateVFSFileInfoOptions options;
	GList *entries;
} DirectoryHandle;

static DirectoryHandle *
directory_handle_new (MateVFSFileInfoOptions options)
{
	DirectoryHandle *result;

	result = g_new (DirectoryHandle, 1);
	result->options = options;
	result->entries = NULL;

	return result;
}

static void
directory_handle_destroy (DirectoryHandle *dir_handle)
{
	g_list_foreach (dir_handle->entries, (GFunc)g_free, NULL);
	g_list_free (dir_handle->entries);
	g_free (dir_handle);
}

static MateVFSResult
do_open_directory (MateVFSMethod *method,
		   MateVFSMethodHandle **method_handle,
		   MateVFSURI *uri,
		   MateVFSFileInfoOptions options,
		   MateVFSContext *context)
{
	DirectoryHandle *dir_handle;
	GList *l;
	ComputerFile *file;
	ComputerDir *dir;

	dir_handle = directory_handle_new (options);

	dir = get_root ();

	G_LOCK (root_dir);
	for (l = dir->files; l != NULL; l = l->next) {
		file = l->data;
		dir_handle->entries = g_list_prepend (dir_handle->entries,
						      g_strdup (file->file_name));
	}
	G_UNLOCK (root_dir);

	*method_handle = (MateVFSMethodHandle *) dir_handle;
	
	return MATE_VFS_OK;
}

static MateVFSResult
do_close_directory (MateVFSMethod *method,
		    MateVFSMethodHandle *method_handle,
		    MateVFSContext *context)
{
	DirectoryHandle *dir_handle;

	dir_handle = (DirectoryHandle *) method_handle;

	directory_handle_destroy (dir_handle);

	return MATE_VFS_OK;
}

static MateVFSResult
do_read_directory (MateVFSMethod *method,
		   MateVFSMethodHandle *method_handle,
		   MateVFSFileInfo *file_info,
		   MateVFSContext *context)
{
	DirectoryHandle *handle;
	GList *entry;

	handle = (DirectoryHandle *) method_handle;

	if (handle->entries == NULL) {
		return MATE_VFS_ERROR_EOF;
	}

	entry = handle->entries;
	handle->entries = g_list_remove_link (handle->entries, entry);
	
	file_info->valid_fields = MATE_VFS_FILE_INFO_FIELDS_NONE;
	file_info->name = g_strdup (entry->data);
	g_free (entry->data);
	g_list_free_1 (entry);

	file_info->mime_type = g_strdup ("application/x-desktop");
	file_info->type = MATE_VFS_FILE_TYPE_REGULAR;
	file_info->valid_fields |=
		MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE |
		MATE_VFS_FILE_INFO_FIELDS_TYPE;

	file_info->permissions =
		MATE_VFS_PERM_USER_READ |
		MATE_VFS_PERM_OTHER_READ |
		MATE_VFS_PERM_GROUP_READ;
	file_info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_PERMISSIONS;
	
	return MATE_VFS_OK;
}

static MateVFSResult
do_get_file_info (MateVFSMethod *method,
		  MateVFSURI *uri,
		  MateVFSFileInfo *file_info,
		  MateVFSFileInfoOptions options,
		  MateVFSContext *context)
{
	file_info->valid_fields = MATE_VFS_FILE_INFO_FIELDS_NONE;

	if (strcmp (uri->text, "/") == 0) {
		file_info->name = g_strdup ("/");
		
		file_info->mime_type = g_strdup ("x-directory/normal");
		file_info->type = MATE_VFS_FILE_TYPE_DIRECTORY;
		file_info->valid_fields |=
			MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE |
			MATE_VFS_FILE_INFO_FIELDS_TYPE;
	} else {
		file_info->name = mate_vfs_uri_extract_short_name (uri);
		
		file_info->mime_type = g_strdup ("application/x-desktop");
		file_info->type = MATE_VFS_FILE_TYPE_REGULAR;
		file_info->valid_fields |=
			MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE |
			MATE_VFS_FILE_INFO_FIELDS_TYPE;
	}
	file_info->permissions =
		MATE_VFS_PERM_USER_READ |
		MATE_VFS_PERM_OTHER_READ |
		MATE_VFS_PERM_GROUP_READ;
	file_info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_PERMISSIONS;
	
	return MATE_VFS_OK;
}

static MateVFSResult
do_get_file_info_from_handle (MateVFSMethod *method,
			      MateVFSMethodHandle *method_handle,
			      MateVFSFileInfo *file_info,
			      MateVFSFileInfoOptions options,
			      MateVFSContext *context)
{
	FileHandle *file_handle;

	file_handle = (FileHandle *) method_handle;

	file_info->valid_fields = MATE_VFS_FILE_INFO_FIELDS_NONE;
	
	file_info->mime_type = g_strdup ("application/x-desktop");
	file_info->size = file_handle->len;
	file_info->type = MATE_VFS_FILE_TYPE_REGULAR;
	file_info->valid_fields |=
		MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE |
		MATE_VFS_FILE_INFO_FIELDS_SIZE |
		MATE_VFS_FILE_INFO_FIELDS_TYPE;

	return MATE_VFS_OK;
}

static gboolean
do_is_local (MateVFSMethod *method,
	     const MateVFSURI *uri)
{
	return TRUE;
}


static MateVFSResult
do_make_directory (MateVFSMethod *method,
		   MateVFSURI *uri,
		   guint perm,
		   MateVFSContext *context)
{
	return MATE_VFS_ERROR_NOT_PERMITTED;
}

static MateVFSResult
do_remove_directory (MateVFSMethod *method,
		     MateVFSURI *uri,
		     MateVFSContext *context)
{
	return MATE_VFS_ERROR_NOT_PERMITTED;
}


static MateVFSResult
do_find_directory (MateVFSMethod *method,
		   MateVFSURI *near_uri,
		   MateVFSFindDirectoryKind kind,
		   MateVFSURI **result_uri,
		   gboolean create_if_needed,
		   gboolean find_if_needed,
		   guint permissions,
		   MateVFSContext *context)
{
	return MATE_VFS_ERROR_NOT_SUPPORTED;
}

static MateVFSResult
do_move (MateVFSMethod *method,
	 MateVFSURI *old_uri,
	 MateVFSURI *new_uri,
	 gboolean force_replace,
	 MateVFSContext *context)
{
	return MATE_VFS_ERROR_NOT_PERMITTED;
}

static MateVFSResult
do_unlink (MateVFSMethod *method,
	   MateVFSURI *uri,
	   MateVFSContext *context)
{
	return MATE_VFS_ERROR_NOT_PERMITTED;
}

static MateVFSResult
do_create_symbolic_link (MateVFSMethod *method,
			 MateVFSURI *uri,
			 const char *target_reference,
			 MateVFSContext *context)
{
	return MATE_VFS_ERROR_NOT_PERMITTED;
}

/* When checking whether two locations are on the same file system, we are
   doing this to determine whether we can recursively move or do other
   sorts of transfers.  When a symbolic link is the "source", its
   location is the location of the link file, because we want to
   know about transferring the link, whereas for symbolic links that
   are "targets", we use the location of the object being pointed to,
   because that is where we will be moving/copying to. */
static MateVFSResult
do_check_same_fs (MateVFSMethod *method,
		  MateVFSURI *source_uri,
		  MateVFSURI *target_uri,
		  gboolean *same_fs_return,
		  MateVFSContext *context)
{
	return TRUE;
}

static MateVFSResult
do_set_file_info (MateVFSMethod *method,
		  MateVFSURI *uri,
		  const MateVFSFileInfo *info,
		  MateVFSSetFileInfoMask mask,
		  MateVFSContext *context)
{
	return MATE_VFS_ERROR_NOT_PERMITTED;
}

static MateVFSResult
do_monitor_add (MateVFSMethod *method,
		MateVFSMethodHandle **method_handle_return,
		MateVFSURI *uri,
		MateVFSMonitorType monitor_type)
{
	ComputerDir *dir;
	ComputerMonitor *monitor;
	char *name;

	if (strcmp (uri->text, "/") == 0) {
		dir = get_root ();

		monitor = g_new0 (ComputerMonitor, 1);
		monitor->type = MATE_VFS_MONITOR_DIRECTORY;
		
		G_LOCK (root_dir);
		dir->dir_monitors = g_list_prepend (dir->dir_monitors, monitor);
		G_UNLOCK (root_dir);
	} else {
		if (monitor_type != MATE_VFS_MONITOR_FILE) {
			return MATE_VFS_ERROR_NOT_SUPPORTED;
		}
		
		dir = get_root ();

		monitor = g_new0 (ComputerMonitor, 1);
		monitor->type = MATE_VFS_MONITOR_FILE;
		
		G_LOCK (root_dir);
		name = mate_vfs_uri_extract_short_name (uri);
		monitor->file = get_file (dir, name);
		g_free (name);

		if (monitor->file != NULL) {
			monitor->file->file_monitors = g_list_prepend (monitor->file->file_monitors,
								       monitor);
		}
		G_UNLOCK (root_dir);
		
	}
	
	*method_handle_return = (MateVFSMethodHandle *)monitor;
	
	return MATE_VFS_OK;
}

static MateVFSResult
do_monitor_cancel (MateVFSMethod *method,
		   MateVFSMethodHandle *method_handle)
{
	ComputerDir *dir;
	ComputerMonitor *monitor;
	ComputerFile *file;

	dir = get_root ();

	G_LOCK (root_dir);
	monitor = (ComputerMonitor *) method_handle;
	if (monitor->type == MATE_VFS_MONITOR_DIRECTORY) {
		dir->dir_monitors = g_list_remove (dir->dir_monitors, monitor);
	} else {
		file = monitor->file;
		if (file != NULL) {
			file->file_monitors = g_list_remove (file->file_monitors,
							     monitor);
		}
	}
	G_UNLOCK (root_dir);

	g_free (monitor);
	
	return MATE_VFS_OK;
}

static MateVFSResult
do_file_control (MateVFSMethod *method,
		 MateVFSMethodHandle *method_handle,
		 const char *operation,
		 gpointer operation_data,
		 MateVFSContext *context)
{
	return MATE_VFS_ERROR_NOT_SUPPORTED;
}

static MateVFSMethod method = {
	sizeof (MateVFSMethod),
	do_open,
	do_create,
	do_close,
	do_read,
	do_write,
	do_seek,
	do_tell,
	do_truncate_handle,
	do_open_directory,
	do_close_directory,
	do_read_directory,
	do_get_file_info,
	do_get_file_info_from_handle,
	do_is_local,
	do_make_directory,
	do_remove_directory,
	do_move,
	do_unlink,
	do_check_same_fs,
	do_set_file_info,
	do_truncate,
	do_find_directory,
	do_create_symbolic_link,
	do_monitor_add,
	do_monitor_cancel,
	do_file_control
};

MateVFSMethod *
vfs_module_init (const char *method_name, const char *args)
{
	return &method;
}

void
vfs_module_shutdown (MateVFSMethod *method)
{
}
