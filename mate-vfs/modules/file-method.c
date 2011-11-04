/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* file-method.c - Local file access method for the MATE Virtual File
   System.

   Copyright (C) 1999 Free Software Foundation

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
   	Ettore Perazzoli <ettore@comm2000.it>
   	Pavel Cisler <pavel@eazel.com>
 */

#include <config.h>

#include <libmatevfs/mate-vfs-cancellation.h>
#include <libmatevfs/mate-vfs-context.h>
#include <libmatevfs/mate-vfs-method.h>
#include <libmatevfs/mate-vfs-mime.h>
#include <libmatevfs/mate-vfs-module-shared.h>
#include <libmatevfs/mate-vfs-module.h>
#include <libmatevfs/mate-vfs-utils.h>
#include <libmatevfs/mate-vfs-mime.h>
#include <libmatevfs/mate-vfs-monitor-private.h>
#include <libmatevfs/mate-vfs-private-utils.h>
#include <libmatevfs/mate-vfs-ops.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>
#ifndef G_OS_WIN32
#include <dirent.h>
#endif
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifndef G_OS_WIN32		/* We don't want the ftruncate() in mingw's unistd.h */
#include <unistd.h>
#endif
#include <utime.h>
#include <string.h>
#ifdef HAVE_FAM
#include <fam.h>
#endif

#ifdef HAVE_SELINUX
#include <selinux/selinux.h>
#endif

#if defined(HAVE_LINUX_INOTIFY_H) || defined(HAVE_SYS_INOTIFY_H)
#define USE_INOTIFY 1
#include "inotify-helper.h"
#endif

#if HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#endif
#if HAVE_SYS_VFS_H
#include <sys/vfs.h>
#elif HAVE_SYS_MOUNT_H
#if HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <sys/mount.h>
#endif

#include "file-method-acl.h"

#ifdef G_OS_WIN32
#include <windows.h>
#endif

#ifdef G_OS_WIN32
#define DIR_SEPARATORS "/\\"
#else
#define DIR_SEPARATORS "/"
#endif

typedef struct {
	MateVFSMethodMonitorCancelFunc cancel_func;  /* Must be first */
} AnyFileMonitorHandle;

#ifdef HAVE_FAM
static FAMConnection *fam_connection = NULL;
static gint fam_watch_id = 0;
G_LOCK_DEFINE_STATIC (fam_connection);

typedef struct {
	MateVFSMethodMonitorCancelFunc cancel_func;  /* Must be first */
	MateVFSURI *uri;
	FAMRequest request;
	gboolean     cancelled;
} FileMonitorHandle;
#endif

#ifdef PATH_MAX
#define	GET_PATH_MAX()	PATH_MAX
#else
static int
GET_PATH_MAX (void)
{
	static unsigned int value;

	/* This code is copied from GNU make.  It returns the maximum
	   path length by using `pathconf'.  */

	if (value == 0) {
		long int x = pathconf(G_DIR_SEPARATOR_S, _PC_PATH_MAX);

		if (x > 0)
			value = x;
		else
			return MAXPATHLEN;
	}

	return value;
}
#endif

#ifdef HAVE_OPEN64
#define OPEN open64
#else
#define OPEN g_open
#endif

#if defined(HAVE_LSEEK64) && defined(HAVE_OFF64_T)
#define LSEEK lseek64
#define OFF_T off64_t
#else
#define LSEEK lseek
#define OFF_T off_t
#endif

#ifdef G_OS_WIN32

static int
ftruncate (int     fd,
	   guint64 size)
{
	/* FIXME: not threadsafe at all! */
	LARGE_INTEGER origpos;
	int retval = -1;

	origpos.QuadPart = 0;
	origpos.u.LowPart = SetFilePointer ((HANDLE) _get_osfhandle (fd), 0,
					    &origpos.u.HighPart, FILE_CURRENT);
	if (origpos.u.LowPart != INVALID_SET_FILE_POINTER) {
		LARGE_INTEGER newpos;

		newpos.QuadPart = size;
		if (SetFilePointer ((HANDLE) _get_osfhandle (fd), newpos.u.LowPart,
				    &newpos.u.HighPart, FILE_BEGIN) != INVALID_SET_FILE_POINTER &&
		    SetEndOfFile ((HANDLE) _get_osfhandle (fd)) &&
		    SetFilePointer ((HANDLE) _get_osfhandle (fd), origpos.u.LowPart,
				    &origpos.u.HighPart, FILE_BEGIN) != INVALID_SET_FILE_POINTER)
			retval = 0;
	}

	if (retval == -1)
		errno = EIO;

	return retval;
}

#endif

static gchar *
get_path_from_uri (MateVFSURI const *uri)
{
	gchar *path;

	path = mate_vfs_unescape_string (uri->text, DIR_SEPARATORS);

	if (path == NULL) {
		return NULL;
	}

	if (!g_path_is_absolute (path)) {
		g_free (path);
		return NULL;
	}
#ifdef G_OS_WIN32
	/* Drop slash in front of drive letter */
	if (path[0] == '/' && g_ascii_isalpha (path[1]) && path[2] == ':') {
		gchar *tem = path;
		path = g_strdup (path + 1);
		g_free (tem);
	}
#endif
	return path;
}

static gchar *
get_base_from_uri (MateVFSURI const *uri)
{
	gchar *escaped_base, *base;

	escaped_base = mate_vfs_uri_extract_short_path_name (uri);
	base = mate_vfs_unescape_string (escaped_base, DIR_SEPARATORS);
	g_free (escaped_base);
	return base;
}

typedef struct {
	MateVFSURI *uri;
	gint fd;
} FileHandle;

static FileHandle *
file_handle_new (MateVFSURI *uri,
		 gint fd)
{
	FileHandle *result;
	result = g_new (FileHandle, 1);

	result->uri = mate_vfs_uri_ref (uri);
	result->fd = fd;

	return result;
}

static void
file_handle_destroy (FileHandle *handle)
{
	mate_vfs_uri_unref (handle->uri);
	g_free (handle);
}

static MateVFSResult
do_open (MateVFSMethod *method,
	 MateVFSMethodHandle **method_handle,
	 MateVFSURI *uri,
	 MateVFSOpenMode mode,
	 MateVFSContext *context)
{
	FileHandle *file_handle;
	gint fd;
	mode_t unix_mode;
	gchar *file_name;
	struct stat statbuf;

	_MATE_VFS_METHOD_PARAM_CHECK (method_handle != NULL);
	_MATE_VFS_METHOD_PARAM_CHECK (uri != NULL);

	if (mode & MATE_VFS_OPEN_READ) {
		if (mode & MATE_VFS_OPEN_WRITE)
			unix_mode = O_RDWR;
		else
			unix_mode = O_RDONLY;
	} else {
		if (mode & MATE_VFS_OPEN_WRITE)
			unix_mode = O_WRONLY;
		else
			return MATE_VFS_ERROR_INVALID_OPEN_MODE;
	}
#ifdef G_OS_WIN32
	unix_mode |= _O_BINARY;
#endif

	if ((mode & MATE_VFS_OPEN_TRUNCATE) ||
	    (!(mode & MATE_VFS_OPEN_RANDOM) && (mode & MATE_VFS_OPEN_WRITE)))
		unix_mode |= O_TRUNC;

	file_name = get_path_from_uri (uri);
	if (file_name == NULL)
		return MATE_VFS_ERROR_INVALID_URI;

	do
		fd = OPEN (file_name, unix_mode, 0);
	while (fd == -1
	       && errno == EINTR
	       && ! mate_vfs_context_check_cancellation (context));

	g_free (file_name);

	if (fd == -1)
		return mate_vfs_result_from_errno ();

#ifdef HAVE_POSIX_FADVISE
	if (! (mode & MATE_VFS_OPEN_RANDOM)) {
		posix_fadvise (fd, 0, 0, POSIX_FADV_SEQUENTIAL);
	}
#endif

	if (fstat (fd, &statbuf) != 0)
		return mate_vfs_result_from_errno ();

	if (S_ISDIR (statbuf.st_mode)) {
		close (fd);
		return MATE_VFS_ERROR_IS_DIRECTORY;
	}

	file_handle = file_handle_new (uri, fd);

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
	FileHandle *file_handle;
	gint fd;
	mode_t unix_mode;
	gchar *file_name;

	_MATE_VFS_METHOD_PARAM_CHECK (method_handle != NULL);
	_MATE_VFS_METHOD_PARAM_CHECK (uri != NULL);

	unix_mode = O_CREAT | O_TRUNC;

#ifdef G_OS_WIN32
	unix_mode |= _O_BINARY;
#endif
	if (!(mode & MATE_VFS_OPEN_WRITE))
		return MATE_VFS_ERROR_INVALID_OPEN_MODE;

	if (mode & MATE_VFS_OPEN_READ)
		unix_mode |= O_RDWR;
	else
		unix_mode |= O_WRONLY;

	if (exclusive)
		unix_mode |= O_EXCL;

	file_name = get_path_from_uri (uri);
	if (file_name == NULL)
		return MATE_VFS_ERROR_INVALID_URI;

	do
		fd = OPEN (file_name, unix_mode, perm);
	while (fd == -1
	       && errno == EINTR
	       && ! mate_vfs_context_check_cancellation (context));

	g_free (file_name);

	if (fd == -1)
		return mate_vfs_result_from_errno ();

	file_handle = file_handle_new (uri, fd);

	*method_handle = (MateVFSMethodHandle *) file_handle;

	return MATE_VFS_OK;
}

static MateVFSResult
do_close (MateVFSMethod *method,
	  MateVFSMethodHandle *method_handle,
	  MateVFSContext *context)
{
	FileHandle *file_handle;
	gint close_retval;

	g_return_val_if_fail (method_handle != NULL, MATE_VFS_ERROR_INTERNAL);

	file_handle = (FileHandle *) method_handle;

	do
		close_retval = close (file_handle->fd);
	while (close_retval != 0
	       && errno == EINTR
	       && ! mate_vfs_context_check_cancellation (context));

	/* FIXME bugzilla.eazel.com 1163: Should do this even after a failure?  */
	file_handle_destroy (file_handle);

	if (close_retval != 0) {
		return mate_vfs_result_from_errno ();
	}

	return MATE_VFS_OK;
}

static MateVFSResult
do_forget_cache	(MateVFSMethod *method,
		 MateVFSMethodHandle *method_handle,
		 MateVFSFileOffset offset,
		 MateVFSFileSize size)
{
	FileHandle *file_handle;

	g_return_val_if_fail (method_handle != NULL, MATE_VFS_ERROR_INTERNAL);

	file_handle = (FileHandle *) method_handle;

#ifdef HAVE_POSIX_FADVISE
	posix_fadvise (file_handle->fd, offset, size, POSIX_FADV_DONTNEED);
#endif

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
	gint read_val;

	g_return_val_if_fail (method_handle != NULL, MATE_VFS_ERROR_INTERNAL);

	file_handle = (FileHandle *) method_handle;

	do {
		read_val = read (file_handle->fd, buffer, num_bytes);
	} while (read_val == -1
	         && errno == EINTR
	         && ! mate_vfs_context_check_cancellation (context));

	if (read_val == -1) {
		*bytes_read = 0;
		return mate_vfs_result_from_errno ();
	} else {
		*bytes_read = read_val;

		/* Getting 0 from read() means EOF! */
		if (read_val == 0) {
			return MATE_VFS_ERROR_EOF;
		}
	}
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
	FileHandle *file_handle;
	gint write_val;

	g_return_val_if_fail (method_handle != NULL, MATE_VFS_ERROR_INTERNAL);

	file_handle = (FileHandle *) method_handle;

	do
		write_val = write (file_handle->fd, buffer, num_bytes);
	while (write_val == -1
	       && errno == EINTR
	       && ! mate_vfs_context_check_cancellation (context));

	if (write_val == -1) {
		*bytes_written = 0;
		return mate_vfs_result_from_errno ();
	} else {
		*bytes_written = write_val;
		return MATE_VFS_OK;
	}
}


static gint
seek_position_to_unix (MateVFSSeekPosition position)
{
	switch (position) {
	case MATE_VFS_SEEK_START:
		return SEEK_SET;
	case MATE_VFS_SEEK_CURRENT:
		return SEEK_CUR;
	case MATE_VFS_SEEK_END:
		return SEEK_END;
	default:
		g_warning (_("Unknown MateVFSSeekPosition %d"), position);
		return SEEK_SET; /* bogus */
	}
}

static MateVFSResult
do_seek (MateVFSMethod *method,
	 MateVFSMethodHandle *method_handle,
	 MateVFSSeekPosition whence,
	 MateVFSFileOffset offset,
	 MateVFSContext *context)
{
	FileHandle *file_handle;
	gint lseek_whence;

	file_handle = (FileHandle *) method_handle;
	lseek_whence = seek_position_to_unix (whence);

	if (LSEEK (file_handle->fd, offset, lseek_whence) == -1) {
		if (errno == ESPIPE)
			return MATE_VFS_ERROR_NOT_SUPPORTED;
		else
			return mate_vfs_result_from_errno ();
	}

	return MATE_VFS_OK;
}

static MateVFSResult
do_tell (MateVFSMethod *method,
	 MateVFSMethodHandle *method_handle,
	 MateVFSFileSize *offset_return)
{
	FileHandle *file_handle;
	OFF_T offset;

	file_handle = (FileHandle *) method_handle;

	offset = LSEEK (file_handle->fd, 0, SEEK_CUR);
	if (offset == -1) {
		if (errno == ESPIPE)
			return MATE_VFS_ERROR_NOT_SUPPORTED;
		else
			return mate_vfs_result_from_errno ();
	}

	*offset_return = offset;
	return MATE_VFS_OK;
}


static MateVFSResult
do_truncate_handle (MateVFSMethod *method,
		    MateVFSMethodHandle *method_handle,
		    MateVFSFileSize where,
		    MateVFSContext *context)
{
	FileHandle *file_handle;

	g_return_val_if_fail (method_handle != NULL, MATE_VFS_ERROR_INTERNAL);

	file_handle = (FileHandle *) method_handle;

	if (ftruncate (file_handle->fd, where) == 0) {
		return MATE_VFS_OK;
	} else {
		switch (errno) {
		case EBADF:
		case EROFS:
			return MATE_VFS_ERROR_READ_ONLY;
		case EINVAL:
			return MATE_VFS_ERROR_NOT_SUPPORTED;
		default:
			return MATE_VFS_ERROR_GENERIC;
		}
	}
}

static MateVFSResult
do_truncate (MateVFSMethod *method,
	     MateVFSURI *uri,
	     MateVFSFileSize where,
	     MateVFSContext *context)
{
#ifndef G_OS_WIN32
	gchar *path;

	path = get_path_from_uri (uri);
	if (path == NULL)
		return MATE_VFS_ERROR_INVALID_URI;

	if (truncate (path, where) == 0) {
		g_free (path);
		return MATE_VFS_OK;
	} else {
		g_free (path);
		switch (errno) {
		case EBADF:
		case EROFS:
			return MATE_VFS_ERROR_READ_ONLY;
		case EINVAL:
			return MATE_VFS_ERROR_NOT_SUPPORTED;
		default:
			return MATE_VFS_ERROR_GENERIC;
		}
	}
#else
	g_warning ("Not implemented: file::do_truncate()");
	return MATE_VFS_ERROR_NOT_SUPPORTED;
#endif
}

typedef struct {
	MateVFSURI *uri;
	MateVFSFileInfoOptions options;
#ifndef G_OS_WIN32
	DIR *dir;
	struct dirent *current_entry;
#else
	GDir *dir;
#endif
	gchar *name_buffer;
	gchar *name_ptr;
} DirectoryHandle;

static DirectoryHandle *
directory_handle_new (MateVFSURI *uri,
#ifndef G_OS_WIN32
		      DIR *dir,
#else
		      GDir *dir,
#endif
		      MateVFSFileInfoOptions options)
{
	DirectoryHandle *result;
	gchar *full_name;
	guint full_name_len;

	result = g_new (DirectoryHandle, 1);

	result->uri = mate_vfs_uri_ref (uri);
	result->dir = dir;

#ifndef G_OS_WIN32
	/* Reserve extra space for readdir_r, see man page */
	result->current_entry = g_malloc (sizeof (struct dirent) + GET_PATH_MAX() + 1);
#endif

	full_name = get_path_from_uri (uri);
	g_assert (full_name != NULL); /* already done by caller */
	full_name_len = strlen (full_name);

	result->name_buffer = g_malloc (full_name_len + GET_PATH_MAX () + 2);
	memcpy (result->name_buffer, full_name, full_name_len);

	if (full_name_len > 0 && !G_IS_DIR_SEPARATOR (full_name[full_name_len - 1]))
		result->name_buffer[full_name_len++] = G_DIR_SEPARATOR;

	result->name_ptr = result->name_buffer + full_name_len;

	g_free (full_name);

	result->options = options;

	return result;
}

static void
directory_handle_destroy (DirectoryHandle *directory_handle)
{
	mate_vfs_uri_unref (directory_handle->uri);
	g_free (directory_handle->name_buffer);
#ifndef G_OS_WIN32
	g_free (directory_handle->current_entry);
#endif
	g_free (directory_handle);
}

/* MIME detection code.  */
static void
get_mime_type (MateVFSFileInfo *info,
	       const char *full_name,
	       MateVFSFileInfoOptions options,
	       struct stat *stat_buffer)
{
	const char *mime_type;

	mime_type = NULL;
	if ((options & MATE_VFS_FILE_INFO_FOLLOW_LINKS) == 0
		&& (info->type == MATE_VFS_FILE_TYPE_SYMBOLIC_LINK)) {
		/* we are a symlink and aren't asked to follow -
		 * return the type for a symlink
		 */
		mime_type = "x-special/symlink";
	} else {
		if (options & MATE_VFS_FILE_INFO_FORCE_FAST_MIME_TYPE) {
			mime_type = mate_vfs_get_file_mime_type (full_name,
								  stat_buffer, TRUE);
		} else if (options & MATE_VFS_FILE_INFO_FORCE_SLOW_MIME_TYPE) {
			mime_type = mate_vfs_get_file_mime_type (full_name,
								  stat_buffer, FALSE);
		} else {
			mime_type = mate_vfs_get_file_mime_type_fast (full_name,
								       stat_buffer);
		}
	}

	g_assert (mime_type);
	info->mime_type = g_strdup (mime_type);
	info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE;
}

#ifndef G_OS_WIN32
static gchar *
read_link (const gchar *full_name)
{
	gchar *buffer;
	guint size;

	size = 256;
	buffer = g_malloc (size);

	while (1) {
		int read_size;

                read_size = readlink (full_name, buffer, size);
		if (read_size < 0) {
			g_free (buffer);
			return NULL;
		}
                if (read_size < size) {
			buffer[read_size] = 0;
			return buffer;
		}
                size *= 2;
		buffer = g_realloc (buffer, size);
	}
}
#endif

#ifdef HAVE_SELINUX
/* convert a SELinux scurity context string to a g_malloc() compatible string */
static char *sec_con2g_str(char *tmp)
{
        char *ret = tmp;

	if (tmp) {
	         ret = g_strdup(tmp);
		 freecon(tmp);
	}

	return ret;
}
#endif

/* Get the SELinux security context */
static int
get_selinux_context (
	MateVFSFileInfo *info,
	const char *full_name,
	MateVFSFileInfoOptions options)
{
#ifdef HAVE_SELINUX
	if (is_selinux_enabled()) {

		if ((options & MATE_VFS_FILE_INFO_FOLLOW_LINKS) == 0
			&& (info->type == MATE_VFS_FILE_TYPE_SYMBOLIC_LINK)) {

			/* we are a symlink and aren't asked to follow -
			 * return the type for a symlink */

			if (lgetfilecon_raw(full_name, &info->selinux_context) < 0)
				return mate_vfs_result_from_errno ();
		} else {

			if (getfilecon_raw(full_name, &info->selinux_context) < 0)
				return mate_vfs_result_from_errno ();
		}

		info->selinux_context = sec_con2g_str(info->selinux_context);

		info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_SELINUX_CONTEXT;
		return MATE_VFS_OK;
	}
#endif
	return MATE_VFS_OK;
}

/* Get the SELinux security context from handle */
static int
get_selinux_context_from_handle (
	MateVFSFileInfo *info,
	FileHandle *handle)
{
#ifdef HAVE_SELINUX
	if (is_selinux_enabled()) {
		if (fgetfilecon_raw(handle->fd, &info->selinux_context) >= 0)
			return mate_vfs_result_from_errno ();

		info->selinux_context = sec_con2g_str(info->selinux_context);

		info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_SELINUX_CONTEXT;
		return MATE_VFS_OK;
	}
#endif
	return MATE_VFS_OK;
}

/* Set the SELinux security context */
static int
set_selinux_context (
	const MateVFSFileInfo *info,
	const char *full_name)
{
#ifdef HAVE_SELINUX
	if (is_selinux_enabled()) {
		if (setfilecon_raw(full_name, info->selinux_context) < 0)
			return mate_vfs_result_from_errno ();

		return MATE_VFS_OK;
	}
#endif
	return MATE_VFS_OK;
}

static void
get_access_info (MateVFSFileInfo *file_info,
		 const gchar *full_name)
{
     /* FIXME: should check errno after calling access because we don't
      * want to set valid_fields if something bad happened during one
      * of the access calls
      */
#ifdef G_OS_WIN32
	if (g_access (full_name, R_OK) == 0) {
		file_info->permissions |= MATE_VFS_PERM_ACCESS_READABLE;
	}
	if (g_access (full_name, W_OK) == 0) {
		file_info->permissions |= MATE_VFS_PERM_ACCESS_WRITABLE;
	}
	if (g_file_test (full_name, G_FILE_TEST_IS_EXECUTABLE)) {
		file_info->permissions |= MATE_VFS_PERM_ACCESS_EXECUTABLE;
	}
#else
	/* Try to minimize the nr of access calls. We rely on read almost
	 * always being allowed in normal cases to keep down the number of
	 * calls needed
	 */
	if (g_access (full_name, R_OK|W_OK) == 0) {
		file_info->permissions |= MATE_VFS_PERM_ACCESS_READABLE | MATE_VFS_PERM_ACCESS_WRITABLE;
		if (g_access (full_name, X_OK) == 0) {
			file_info->permissions |= MATE_VFS_PERM_ACCESS_EXECUTABLE;
		}
	} else if (g_access (full_name, R_OK|X_OK) == 0) {
		file_info->permissions |= MATE_VFS_PERM_ACCESS_READABLE | MATE_VFS_PERM_ACCESS_EXECUTABLE;
	} else {
		if (g_access (full_name, R_OK) == 0) {
			file_info->permissions |= MATE_VFS_PERM_ACCESS_READABLE;
		} else {
			if (g_access (full_name, W_OK) == 0) {
				file_info->permissions |= MATE_VFS_PERM_ACCESS_WRITABLE;
			}
			if (g_access (full_name, X_OK) == 0) {
				file_info->permissions |= MATE_VFS_PERM_ACCESS_EXECUTABLE;
			}
		}
	}
#endif

     file_info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_ACCESS;
}

static MateVFSResult
get_stat_info (MateVFSFileInfo *file_info,
	       const gchar *full_name,
	       MateVFSFileInfoOptions options,
	       struct stat *statptr)
{
	struct stat statbuf;
#ifndef G_OS_WIN32
	gboolean is_symlink;
	char *link_file_path;
	char *symlink_name;
	char *symlink_dir;
	char *newpath;
#endif
	gboolean recursive;

	recursive = FALSE;

	MATE_VFS_FILE_INFO_SET_LOCAL (file_info, TRUE);

	if (statptr == NULL) {
		statptr = &statbuf;
	}

	if (g_lstat (full_name, statptr) != 0) {
		return mate_vfs_result_from_errno ();
	}


#ifndef G_OS_WIN32
	is_symlink = S_ISLNK (statptr->st_mode);

	if ((options & MATE_VFS_FILE_INFO_FOLLOW_LINKS) && is_symlink) {
		if (g_stat (full_name, statptr) != 0) {
			if (errno == ELOOP) {
				recursive = TRUE;
			}

			/* It's a broken symlink, revert to the lstat. This is sub-optimal but
			 * acceptable because it's not a common case.
			 */
			if (g_lstat (full_name, statptr) != 0) {
				return mate_vfs_result_from_errno ();
			}
		}
		MATE_VFS_FILE_INFO_SET_SYMLINK (file_info, TRUE);
	}
#endif
	mate_vfs_stat_to_file_info (file_info, statptr);

#ifndef G_OS_WIN32
	if (is_symlink) {
		symlink_name = NULL;
		link_file_path = g_strdup (full_name);

		/* We will either successfully read the link name or return
		 * NULL if read_link fails -- flag it as a valid field either
		 * way.
		 */
		file_info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_SYMLINK_NAME;

		while (TRUE) {
			/* Deal with multiple-level symlinks by following them as
			 * far as we can.
			 */

			g_free (symlink_name);
			symlink_name = read_link (link_file_path);
			if (symlink_name == NULL) {
				g_free (link_file_path);
				return mate_vfs_result_from_errno ();
			}
			if ((options & MATE_VFS_FILE_INFO_FOLLOW_LINKS) &&
			    symlink_name[0] != '/') {
				symlink_dir = g_path_get_dirname (link_file_path);
				newpath = g_build_filename (symlink_dir,
							    symlink_name, NULL);
				g_free (symlink_dir);
				g_free (symlink_name);
				symlink_name = mate_vfs_make_path_name_canonical (newpath);
				g_free (newpath);
			}

			if ((options & MATE_VFS_FILE_INFO_FOLLOW_LINKS) == 0
			                /* if we had an earlier ELOOP, don't get in an infinite loop here */
			        || recursive
					/* we don't care to follow links */
				|| g_lstat (symlink_name, statptr) != 0
					/* we can't make out where this points to */
				|| !S_ISLNK (statptr->st_mode)) {
					/* the next level is not a link */
				break;
			}
			g_free (link_file_path);
			link_file_path = g_strdup (symlink_name);
		}
		g_free (link_file_path);

		file_info->symlink_name = symlink_name;
	}
#endif
	return MATE_VFS_OK;
}

static MateVFSResult
get_stat_info_from_handle (MateVFSFileInfo *file_info,
			   FileHandle *handle,
			   MateVFSFileInfoOptions options,
			   struct stat *statptr)
{
	struct stat statbuf;

	if (statptr == NULL) {
		statptr = &statbuf;
	}

	if (fstat (handle->fd, statptr) != 0) {
		return mate_vfs_result_from_errno ();
	}

	mate_vfs_stat_to_file_info (file_info, statptr);
	MATE_VFS_FILE_INFO_SET_LOCAL (file_info, TRUE);

	return MATE_VFS_OK;
}


static MateVFSResult
do_open_directory (MateVFSMethod *method,
		   MateVFSMethodHandle **method_handle,
		   MateVFSURI *uri,
		   MateVFSFileInfoOptions options,
		   MateVFSContext *context)
{
	gchar *directory_name;
#ifndef G_OS_WIN32
	DIR *dir;
#else
	GDir *dir;
#endif

	directory_name = get_path_from_uri (uri);
	if (directory_name == NULL)
		return MATE_VFS_ERROR_INVALID_URI;

#ifndef G_OS_WIN32
	dir = opendir (directory_name);
#else
	dir = g_dir_open (directory_name, 0, NULL);
#endif
	g_free (directory_name);
	if (dir == NULL)
		return mate_vfs_result_from_errno ();

	*method_handle
		= (MateVFSMethodHandle *) directory_handle_new (uri, dir,
								 options);

	return MATE_VFS_OK;
}

static MateVFSResult
do_close_directory (MateVFSMethod *method,
		    MateVFSMethodHandle *method_handle,
		    MateVFSContext *context)
{
	DirectoryHandle *directory_handle;

	directory_handle = (DirectoryHandle *) method_handle;

#ifndef G_OS_WIN32
	closedir (directory_handle->dir);
#else
	g_dir_close (directory_handle->dir);
#endif

	directory_handle_destroy (directory_handle);

	return MATE_VFS_OK;
}

#ifndef HAVE_READDIR_R
G_LOCK_DEFINE_STATIC (readdir);
#endif

static MateVFSResult
do_read_directory (MateVFSMethod *method,
		   MateVFSMethodHandle *method_handle,
		   MateVFSFileInfo *file_info,
		   MateVFSContext *context)
{
#ifndef G_OS_WIN32
	struct dirent *result;
#else
	const gchar *result;
#endif
	struct stat statbuf;
	gchar *full_name;
	DirectoryHandle *handle;

	handle = (DirectoryHandle *) method_handle;

	errno = 0;
#ifdef HAVE_READDIR_R
	if (readdir_r (handle->dir, handle->current_entry, &result) != 0) {
		/* Work around a Solaris bug.
		 * readdir64_r returns -1 instead of 0 at EOF.
		 */
		if (errno == 0) {
			return MATE_VFS_ERROR_EOF;
		}
		return mate_vfs_result_from_errno ();
	}
#else
	G_LOCK (readdir);
	errno = 0;
#ifndef G_OS_WIN32
	result = readdir (handle->dir);
#else
	result = g_dir_read_name (handle->dir);
#endif

	if (result == NULL && errno != 0) {
		MateVFSResult ret = mate_vfs_result_from_errno ();
		G_UNLOCK (readdir);
		return ret;
	}
#ifndef G_OS_WIN32
	if (result != NULL) {
		memcpy (handle->current_entry, result, sizeof (struct dirent));
	}
#endif
	G_UNLOCK (readdir);
#endif

	if (result == NULL) {
		return MATE_VFS_ERROR_EOF;
	}

#ifndef G_OS_WIN32
	file_info->name = g_strdup (result->d_name);
	strcpy (handle->name_ptr, result->d_name);
#else
	file_info->name = g_strdup (result);
	strcpy (handle->name_ptr, result);
#endif
	full_name = handle->name_buffer;

	if (handle->options & MATE_VFS_FILE_INFO_NAME_ONLY) {
		return MATE_VFS_OK;
	}

	if (handle->options & MATE_VFS_FILE_INFO_GET_SELINUX_CONTEXT) {

		/* Attempt to get selinux contet, ignore error (see below) */
		get_selinux_context(file_info, full_name, handle->options);
	}

	if (get_stat_info (file_info, full_name, handle->options, &statbuf) != MATE_VFS_OK) {
		/* Return OK - this should not terminate the directory iteration
		 * and we will know from the valid_fields that we don't have the
		 * stat info.
		 */
		return MATE_VFS_OK;
	}

	if (handle->options & MATE_VFS_FILE_INFO_GET_ACCESS_RIGHTS) {
		get_access_info (file_info, full_name);
	}

	if (handle->options & MATE_VFS_FILE_INFO_GET_MIME_TYPE) {
		get_mime_type (file_info, full_name, handle->options, &statbuf);
	}

	if (handle->options & MATE_VFS_FILE_INFO_GET_ACL) {
		file_get_acl (full_name, file_info, &statbuf, context);
	}

	return MATE_VFS_OK;
}

static MateVFSResult
do_get_file_info (MateVFSMethod *method,
		  MateVFSURI *uri,
		  MateVFSFileInfo *file_info,
		  MateVFSFileInfoOptions options,
		  MateVFSContext *context)
{
	MateVFSResult result;
	gchar *full_name;
	struct stat statbuf;

	full_name = get_path_from_uri (uri);
	if (full_name == NULL)
		return MATE_VFS_ERROR_INVALID_URI;

	file_info->valid_fields = MATE_VFS_FILE_INFO_FIELDS_NONE;

	file_info->name = get_base_from_uri (uri);
	g_assert (file_info->name != NULL);

	result = get_stat_info (file_info, full_name, options, &statbuf);
	if (result != MATE_VFS_OK) {
		g_free (full_name);
		return result;
	}

	if (options & MATE_VFS_FILE_INFO_GET_SELINUX_CONTEXT) {
		get_selinux_context (file_info, full_name, options);
	}

	if (options & MATE_VFS_FILE_INFO_GET_ACCESS_RIGHTS) {
		get_access_info (file_info, full_name);
	}

	if (options & MATE_VFS_FILE_INFO_GET_MIME_TYPE) {
		get_mime_type (file_info, full_name, options, &statbuf);
	}

	if (options & MATE_VFS_FILE_INFO_GET_ACL) {
		file_get_acl (full_name, file_info, &statbuf, context);
	}

	g_free (full_name);

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
	gchar *full_name;
	struct stat statbuf;
	MateVFSResult result;

	file_handle = (FileHandle *) method_handle;

	file_info->valid_fields = MATE_VFS_FILE_INFO_FIELDS_NONE;

	full_name = get_path_from_uri (file_handle->uri);
	if (full_name == NULL) {
		return MATE_VFS_ERROR_INVALID_URI;
	}

	file_info->name = get_base_from_uri (file_handle->uri);
	g_assert (file_info->name != NULL);

	result = get_stat_info_from_handle (file_info, file_handle,
					    options, &statbuf);
	if (result != MATE_VFS_OK) {
		g_free (full_name);
		return result;
	}

	if (options & MATE_VFS_FILE_INFO_GET_SELINUX_CONTEXT) {
		result = get_selinux_context_from_handle (file_info, file_handle);
		if (result != MATE_VFS_OK) {
			g_free (full_name);
			return result;
		}
	}

	if (options & MATE_VFS_FILE_INFO_GET_MIME_TYPE) {
		get_mime_type (file_info, full_name, options, &statbuf);
	}

	if (options & MATE_VFS_FILE_INFO_GET_ACL) {
		file_get_acl (full_name, file_info, &statbuf, context);
	}

	g_free (full_name);

	return MATE_VFS_OK;
}

extern char *filesystem_type (char *path, char *relpath, struct stat *statp);

G_LOCK_DEFINE_STATIC (fstype);

static gboolean
do_is_local (MateVFSMethod *method,
	     const MateVFSURI *uri)
{
	struct stat statbuf;
	gboolean is_local;
	gchar *path;

	g_return_val_if_fail (uri != NULL, FALSE);

	path = get_path_from_uri (uri);
	if (path == NULL)
		return TRUE; /* MATE_VFS_ERROR_INVALID_URI */

	if (g_stat (path, &statbuf) == 0) {
		char *type;

		G_LOCK (fstype);
		type = filesystem_type (path, path, &statbuf);
		is_local = ((strcmp (type, "nfs") != 0) &&
			    (strcmp (type, "afs") != 0) &&
			    (strcmp (type, "autofs") != 0) &&
			    (strcmp (type, "unknown") != 0) &&
			    (strcmp (type, "novfs") != 0) &&
			    (strcmp (type, "ncpfs") != 0));
		G_UNLOCK (fstype);
	} else {
		/* Assume non-existent files are local */
		is_local = TRUE;
	}
	g_free (path);
	return is_local;
}


static MateVFSResult
do_make_directory (MateVFSMethod *method,
		   MateVFSURI *uri,
		   guint perm,
		   MateVFSContext *context)
{
	gint retval;
	gchar *full_name;

	full_name = get_path_from_uri (uri);
	if (full_name == NULL)
		return MATE_VFS_ERROR_INVALID_URI;

	retval = g_mkdir (full_name, perm);

	g_free (full_name);

	if (retval != 0) {
		return mate_vfs_result_from_errno ();
	}

	return MATE_VFS_OK;
}

static MateVFSResult
do_remove_directory (MateVFSMethod *method,
		     MateVFSURI *uri,
		     MateVFSContext *context)
{
	gchar *full_name;
	gint retval;

	full_name = get_path_from_uri (uri);
	if (full_name == NULL)
		return MATE_VFS_ERROR_INVALID_URI;

	retval = g_rmdir (full_name);

	g_free (full_name);

	if (retval != 0) {
		return mate_vfs_result_from_errno ();
	}

	return MATE_VFS_OK;
}

#undef DEBUG_FIND_DIRECTORY
/* Get rid of debugging code once we know the logic works. */

#define TRASH_DIRECTORY_NAME_BASE ".Trash"
#define MAX_TRASH_SEARCH_DEPTH 5

/* mkdir_recursive
 * Works like mkdir, except it creates all the levels of directories in @path.
 */
static int
mkdir_recursive (const char *path, int permission_bits)
{
	struct stat stat_buffer;
	const char *dir_separator_scanner;
	char *current_path;

	/* try creating a director for each level */
	for (dir_separator_scanner = path;; dir_separator_scanner++) {
		/* advance to the next directory level */
		for (;;dir_separator_scanner++) {
			if (!*dir_separator_scanner) {
				break;
			}
			if (G_IS_DIR_SEPARATOR (*dir_separator_scanner)) {
				break;
			}
		}
		if (dir_separator_scanner - path > 0) {
			current_path = g_strndup (path, dir_separator_scanner - path);
			g_mkdir (current_path, permission_bits);
			if (g_stat (current_path, &stat_buffer) != 0) {
				/* we failed to create a directory and it wasn't there already;
				 * bail
				 */
				g_free (current_path);
				return -1;
			}
			g_free (current_path);
		}
		if (!*dir_separator_scanner) {
			break;
		}
	}
	return 0;
}


static char *
append_to_path (const char *path, const char *name)
{
	return g_build_filename (path, name, NULL);
}

static char *
append_trash_path (const char *path)
{
	char *per_user_part;
	char *retval;

	per_user_part = g_strconcat (TRASH_DIRECTORY_NAME_BASE "-",
			             g_get_user_name (),
				     NULL);

	retval = g_build_filename (path, per_user_part, NULL);

	g_free (per_user_part);

	return retval;
}

static char *
find_trash_in_hierarchy (const char *start_dir, dev_t near_device_id, MateVFSContext *context)
{
	char *trash_path;
	struct stat stat_buffer;

	if (mate_vfs_context_check_cancellation (context))
		return NULL;

	/* check if there is a trash in this directory */
	trash_path = append_trash_path (start_dir);
	if (g_lstat (trash_path, &stat_buffer) == 0 && S_ISDIR (stat_buffer.st_mode)) {
		/* found it, we are done */
		g_assert (near_device_id == stat_buffer.st_dev);
		return trash_path;
	}
	g_free (trash_path);

	return NULL;
}

static GList *cached_trash_directories;
G_LOCK_DEFINE_STATIC (cached_trash_directories);

/* Element used to store chached Trash entries in the local, in-memory Trash item cache. */
typedef struct {
	char *path;
	char *device_mount_point;
	dev_t device_id;
} TrashDirectoryCachedItem;

typedef struct {
	dev_t device_id;
} FindByDeviceIDParameters;

static int
match_trash_item_by_device_id (gconstpointer item, gconstpointer data)
{
	const TrashDirectoryCachedItem *cached_item;
	FindByDeviceIDParameters *parameters;

	cached_item = (const TrashDirectoryCachedItem *)item;
	parameters = (FindByDeviceIDParameters *)data;

	return cached_item->device_id == parameters->device_id ? 0 : -1;
}

static char *
try_creating_trash_in (const char *path, guint permissions)
{
	char *trash_path;


	trash_path = append_trash_path (path);
	if (mkdir_recursive (trash_path, permissions) == 0) {
#ifdef DEBUG_FIND_DIRECTORY
		g_print ("created trash in %s\n", trash_path);
#endif
		return trash_path;
	}

#ifdef DEBUG_FIND_DIRECTORY
	g_print ("failed to create trash in %s\n", trash_path);
#endif
	g_free (trash_path);
	return NULL;
}

static char *
find_disk_top_directory (const char *item_on_disk,
			 dev_t near_device_id,
			 MateVFSContext *context)
{
	char *disk_top_directory;
	struct stat stat_buffer;

	disk_top_directory = g_strdup (item_on_disk);

	/* Walk up in the hierarchy, finding the top-most point that still
	 * matches our device ID -- the root directory of the volume.
	 */
	for (;;) {
		char *previous_search_directory;
		char *last_slash;

		previous_search_directory = g_strdup (disk_top_directory);
		last_slash = strrchr (disk_top_directory, '/');
		if (last_slash == NULL) {
			g_free (previous_search_directory);
			break;
		}

		*last_slash = '\0';
		if (g_stat (disk_top_directory, &stat_buffer) < 0
			|| stat_buffer.st_dev != near_device_id) {
			/* we ran past the root of the disk we are exploring */
			g_free (disk_top_directory);
			disk_top_directory = previous_search_directory;
			break;
		}
		/* FIXME bugzilla.eazel.com 2733: This must result in
		 * a cancelled error, but there's no way for the
		 * caller to know that. We probably have to add a
		 * MateVFSResult to this function.
		 */
		if (mate_vfs_context_check_cancellation (context)) {
			g_free (previous_search_directory);
			g_free (disk_top_directory);
			return NULL;
		}
	}
	return disk_top_directory;
}

#define TRASH_ENTRY_CACHE_PARENT ".mate/mate-vfs"
#define TRASH_ENTRY_CACHE_NAME ".trash_entry_cache"
#define NON_EXISTENT_TRASH_ENTRY "-"

/* Save the localy cached Trashed paths on disk in the user's home
 * directory.
 */
static void
save_trash_entry_cache (void)
{
	int cache_file;
	char *cache_file_parent, *cache_file_path;
	GList *p;
	char *buffer, *escaped_path, *escaped_mount_point;

	cache_file_parent = append_to_path (g_get_home_dir (), TRASH_ENTRY_CACHE_PARENT);
	cache_file_path = append_to_path (cache_file_parent, TRASH_ENTRY_CACHE_NAME);

	if (mkdir_recursive (cache_file_parent, 0777) != 0) {
		g_warning ("failed to create trash item cache file");
		return;
	}

	cache_file = g_open (cache_file_path, O_CREAT | O_TRUNC | O_RDWR, 0666);
	if (cache_file < 0) {
		g_warning ("failed to create trash item cache file");
		return;
	}

	for (p = cached_trash_directories; p != NULL; p = p->next) {
		/* Use proper escaping to not confuse paths with spaces in them */
		escaped_path = mate_vfs_escape_path_string (
			((TrashDirectoryCachedItem *)p->data)->path);
		escaped_mount_point = mate_vfs_escape_path_string(
			((TrashDirectoryCachedItem *)p->data)->device_mount_point);

		buffer = g_strdup_printf ("%s %s\n", escaped_mount_point, escaped_path);
		write (cache_file, buffer, strlen (buffer));

#ifdef DEBUG_FIND_DIRECTORY
	g_print ("saving trash item cache %s\n", buffer);
#endif

		g_free (buffer);
		g_free (escaped_mount_point);
		g_free (escaped_path);
	}
	close (cache_file);

	g_free (cache_file_path);
	g_free (cache_file_parent);
}

typedef struct {
	const char *mount_point;
	const char *trash_path;
	dev_t device_id;
	gboolean done;
} UpdateOneCachedEntryContext;

/* Updates one entry in the local Trash item cache to reflect the
 * location we just found or in which we created a new Trash.
 */
static void
update_one_cached_trash_entry (gpointer element, gpointer cast_to_context)
{
	UpdateOneCachedEntryContext *context;
	TrashDirectoryCachedItem *item;

	context = (UpdateOneCachedEntryContext *)cast_to_context;
	item = (TrashDirectoryCachedItem *)element;

	if (context->done) {
		/* We already took care of business in a previous iteration. */
		return;
	}

	if (strcmp (context->mount_point, item->device_mount_point) == 0) {
		/* This is the item we are looking for, update it. */
		g_free (item->path);
		item->path = g_strdup (context->trash_path);
		item->device_id = context->device_id;

		/* no more work */
		context->done = TRUE;
	}
}

static void
add_local_cached_trash_entry (dev_t near_device_id, const char *trash_path, const char *mount_point)
{
	TrashDirectoryCachedItem *new_cached_item;
	UpdateOneCachedEntryContext update_context;

	/* First check if we already have an entry for this mountpoint,
	 * if so, update it.
	 */

	update_context.mount_point = mount_point;
	update_context.trash_path = trash_path;
	update_context.device_id = near_device_id;
	update_context.done = FALSE;

	g_list_foreach (cached_trash_directories, update_one_cached_trash_entry, &update_context);
	if (update_context.done) {
		/* Sucessfully updated, no more work left. */
		return;
	}

	/* Save the new trash item to the local cache. */
	new_cached_item = g_new (TrashDirectoryCachedItem, 1);
	new_cached_item->path = g_strdup (trash_path);
	new_cached_item->device_mount_point = g_strdup (mount_point);
	new_cached_item->device_id = near_device_id;


	cached_trash_directories = g_list_prepend (cached_trash_directories, new_cached_item);
}

static void
add_cached_trash_entry (dev_t near_device_id, const char *trash_path, const char *mount_point)
{
	add_local_cached_trash_entry (near_device_id, trash_path, mount_point);
	/* write out the local cache */
	save_trash_entry_cache ();
}

static void
destroy_cached_trash_entry (TrashDirectoryCachedItem *entry)
{
	g_free (entry->path);
	g_free (entry->device_mount_point);
	g_free (entry);
}

/* Read the cached entries for the file cache into the local Trash item cache. */
static void
read_saved_cached_trash_entries (void)
{
	char *cache_file_path;
	FILE *cache_file;
	char buffer[2048];
	char escaped_mount_point[PATH_MAX], escaped_trash_path[PATH_MAX];
	char *mount_point, *trash_path;
	struct stat stat_buffer;
	gboolean removed_item;

	/* empty the old locally cached entries */
	g_list_foreach (cached_trash_directories,
		(GFunc)destroy_cached_trash_entry, NULL);
	g_list_free (cached_trash_directories);
	cached_trash_directories = NULL;

	/* read in the entries from disk */
	cache_file_path = g_build_filename (g_get_home_dir (),
					    TRASH_ENTRY_CACHE_PARENT,
					    TRASH_ENTRY_CACHE_NAME,
					    NULL);
	cache_file = g_fopen (cache_file_path, "r");

	if (cache_file != NULL) {
		removed_item = FALSE;
		for (;;) {
			if (fgets (buffer, sizeof (buffer), cache_file) == NULL) {
				break;
			}

			mount_point = NULL;
			trash_path = NULL;
			if (sscanf (buffer, "%s %s", escaped_mount_point, escaped_trash_path) == 2) {
				/* the paths are saved in escaped form */
				trash_path = mate_vfs_unescape_string (escaped_trash_path, DIR_SEPARATORS);
				mount_point = mate_vfs_unescape_string (escaped_mount_point, DIR_SEPARATORS);

				if (trash_path != NULL
					&& mount_point != NULL
					&& (strcmp (trash_path, NON_EXISTENT_TRASH_ENTRY) != 0 && g_lstat (trash_path, &stat_buffer) == 0)
					&& g_stat (mount_point, &stat_buffer) == 0) {
					/* We know the trash exist and we checked that it's really
					 * there - this is a good entry, copy it into the local cache.
					 * We don't want to rely on old non-existing trash entries, as they
					 * could have changed now, and they stick around filling up the cache,
					 * and slowing down startup.
					 */
					 add_local_cached_trash_entry (stat_buffer.st_dev, trash_path, mount_point);
#ifdef DEBUG_FIND_DIRECTORY
					g_print ("read trash item cache entry %s %s\n", trash_path, mount_point);
#endif
				} else {
					removed_item = TRUE;
				}
			}

			g_free (trash_path);
			g_free (mount_point);
		}
		fclose (cache_file);
		/* Save cache to get rid of stuff from on-disk cache */
		if (removed_item) {
			save_trash_entry_cache ();
		}
	}

	g_free (cache_file_path);
}

/* Create a Trash directory on the same disk as @full_name_near. */
static char *
create_trash_near (const char *full_name_near, dev_t near_device_id, const char *disk_top_directory,
	guint permissions, MateVFSContext *context)
{
	return try_creating_trash_in (disk_top_directory, permissions);
}


static gboolean
cached_trash_entry_exists (const TrashDirectoryCachedItem *entry)
{
	struct stat stat_buffer;
	return g_lstat (entry->path, &stat_buffer) == 0;
}

/* Search through the local cache looking for an entry that matches a given
 * device ID. If @check_disk specified, check if the entry we found actually exists.
 */
static char *
find_locally_cached_trash_entry_for_device_id (dev_t device_id, gboolean check_disk)
{
	GList *matching_item;
	FindByDeviceIDParameters tmp;
	const char *trash_path;

	tmp.device_id = device_id;

	matching_item = g_list_find_custom (cached_trash_directories,
		&tmp, match_trash_item_by_device_id);

	if (matching_item == NULL) {
		return NULL;
	}

	trash_path = ((TrashDirectoryCachedItem *)matching_item->data)->path;

	if (trash_path == NULL) {
		/* we already know that this disk does not contain a trash directory */
#ifdef DEBUG_FIND_DIRECTORY
		g_print ("cache indicates no trash for %s \n", trash_path);
#endif
		return g_strdup (NON_EXISTENT_TRASH_ENTRY);
	}

	if (check_disk) {
		/* We found something, make sure it still exists. */
		if (strcmp (((TrashDirectoryCachedItem *)matching_item->data)->path, NON_EXISTENT_TRASH_ENTRY) != 0
			&& !cached_trash_entry_exists ((TrashDirectoryCachedItem *)matching_item->data)) {
			/* The cached item doesn't really exist, make a new one
			 * and delete the cached entry
			 */
#ifdef DEBUG_FIND_DIRECTORY
			g_print ("entry %s doesn't exist, removing \n",
				((TrashDirectoryCachedItem *)matching_item->data)->path);
#endif
			destroy_cached_trash_entry ((TrashDirectoryCachedItem *)matching_item->data);
			cached_trash_directories = g_list_remove (cached_trash_directories,
				matching_item->data);
			return NULL;
		}
	}

#ifdef DEBUG_FIND_DIRECTORY
	g_print ("local cache found %s \n", trash_path);
#endif
	g_assert (matching_item != NULL);
	return g_strdup (trash_path);
}

/* Look for an entry in the file and local caches. */
static char *
find_cached_trash_entry_for_device (dev_t device_id, gboolean check_disk)
{
	if (cached_trash_directories == NULL) {
		if (!check_disk) {
			return NULL;
		}
		read_saved_cached_trash_entries ();
	}
	return find_locally_cached_trash_entry_for_device_id (device_id, check_disk);
}

/* Search for a Trash entry or create one. Called when there is no cached entry. */
static char *
find_or_create_trash_near (const char *full_name_near, dev_t near_device_id,
	gboolean create_if_needed, gboolean find_if_needed, guint permissions,
	MateVFSContext *context)
{
	char *result;
	char *disk_top_directory;

	result = NULL;
	/* figure out the topmost disk directory */
	disk_top_directory = find_disk_top_directory (full_name_near,
						      near_device_id, context);

	if (disk_top_directory == NULL) {
		/* Failed to find it, don't look at this disk until we
		 * are ready to try to create a Trash on it again.
		 */
#ifdef DEBUG_FIND_DIRECTORY
		g_print ("failed to find top disk directory for %s\n", full_name_near);
#endif
		add_cached_trash_entry (near_device_id, NON_EXISTENT_TRASH_ENTRY, disk_top_directory);
		return NULL;
	}

	if (find_if_needed) {
		/* figure out the topmost disk directory */
		result = find_trash_in_hierarchy (disk_top_directory, near_device_id, context);
		if (result == NULL) {
			/* We just found out there is no Trash on the disk,
			 * remember this for next time.
			 */
			result = g_strdup(NON_EXISTENT_TRASH_ENTRY);
		}
	}

	if (result == NULL && create_if_needed) {
		/* didn't find a Trash, create one */
		result = create_trash_near (full_name_near, near_device_id, disk_top_directory,
			permissions, context);
	}

	if (result != NULL) {
		/* remember whatever we found for next time */
		add_cached_trash_entry (near_device_id, result, disk_top_directory);
	}

	g_free (disk_top_directory);

	return result;
}

/* Find or create a trash directory on the same disk as @full_name_near. Check
 * the local and file cache for matching Trash entries first.
 *
 *     This is the only entry point for the trash cache code,
 * we holds the lock while operating on it only here.
 */
static char *
find_trash_directory (const char *full_name_near, dev_t near_device_id,
		      gboolean create_if_needed, gboolean find_if_needed,
		      guint permissions, MateVFSContext *context)
{
	char *result;

	G_LOCK (cached_trash_directories);

	/* look in the saved trash locations first */
	result = find_cached_trash_entry_for_device (near_device_id, find_if_needed);

	if (find_if_needed) {
		if (result != NULL && strcmp (result, NON_EXISTENT_TRASH_ENTRY) == 0 && create_if_needed) {
			/* We know there is no Trash yet because we remember
			 * from the last time we looked.
			 * If we were asked to create one, ignore the fact that
			 * we already looked for it, look again and create a
			 * new trash if we find nothing.
			 */
#ifdef DEBUG_FIND_DIRECTORY
			g_print ("cache indicates no trash for %s, force a creation \n", full_name_near);
#endif
			g_free (result);
			result = NULL;
		}

		if (result == NULL) {
			/* No luck sofar. Look for the Trash on the disk, optionally create it
			 * if we find nothing.
			 */
			result = find_or_create_trash_near (full_name_near, near_device_id,
				create_if_needed, find_if_needed, permissions, context);
		}
	} else if (create_if_needed) {
		if (result == NULL || strcmp (result, NON_EXISTENT_TRASH_ENTRY) == 0) {
			result = find_or_create_trash_near (full_name_near, near_device_id,
				create_if_needed, find_if_needed, permissions, context);
		}
	}

	if (result != NULL && strcmp(result, NON_EXISTENT_TRASH_ENTRY) == 0) {
		/* This means that we know there is no Trash */
		g_free (result);
		result = NULL;
	}

	G_UNLOCK (cached_trash_directories);

	return result;
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
	gint retval;
	char *full_name_near;
	struct stat near_item_stat;
	struct stat home_volume_stat;
	const char *home_directory;
	char *target_directory_path;
	char *target_directory_uri;


	target_directory_path = NULL;
	*result_uri = NULL;

	full_name_near = get_path_from_uri (near_uri);
	if (full_name_near == NULL)
		return MATE_VFS_ERROR_INVALID_URI;

	/* We will need the URI and the stat structure for the home directory. */
	home_directory = g_get_home_dir ();

	if (mate_vfs_context_check_cancellation (context)) {
		g_free (full_name_near);
		return MATE_VFS_ERROR_CANCELLED;
	}

	retval = g_lstat (full_name_near, &near_item_stat);
	if (retval != 0) {
		g_free (full_name_near);
		return mate_vfs_result_from_errno ();
	}

	if (mate_vfs_context_check_cancellation (context)) {
		g_free (full_name_near);
		return MATE_VFS_ERROR_CANCELLED;
	}

	retval = g_stat (home_directory, &home_volume_stat);
	if (retval != 0) {
		g_free (full_name_near);
		return mate_vfs_result_from_errno ();
	}

	if (mate_vfs_context_check_cancellation (context)) {
		g_free (full_name_near);
		return MATE_VFS_ERROR_CANCELLED;
	}

	switch (kind) {
	case MATE_VFS_DIRECTORY_KIND_TRASH:
		/* Use 0700 (S_IRWXU) for the permissions,
		 * regardless of the requested permissions, so other
		 * users can't view the trash files.
		 */
		permissions = S_IRWXU;
		if (near_item_stat.st_dev != home_volume_stat.st_dev) {
			/* This volume does not contain our home, we have to find/create the Trash
			 * elsewhere on the volume. Use a heuristic to find a good place.
			 */
			if (mate_vfs_context_check_cancellation (context))
				return MATE_VFS_ERROR_CANCELLED;

			target_directory_path = find_trash_directory (full_name_near,
				near_item_stat.st_dev, create_if_needed, find_if_needed,
				permissions, context);

			if (mate_vfs_context_check_cancellation (context)) {
				return MATE_VFS_ERROR_CANCELLED;
			}
		} else  {
			/* volume with a home directory, just create a trash in home */
			target_directory_path = append_to_path (home_directory, TRASH_DIRECTORY_NAME_BASE);
		}
		break;

	case MATE_VFS_DIRECTORY_KIND_DESKTOP:
		if (near_item_stat.st_dev != home_volume_stat.st_dev) {
			/* unsupported */
			break;
		}
		target_directory_path = append_to_path (home_directory, "Desktop");
		break;

	default:
		break;
	}

	g_free (full_name_near);

	if (target_directory_path == NULL) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

	if (create_if_needed && g_access (target_directory_path, F_OK) != 0) {
		mkdir_recursive (target_directory_path, permissions);
	}

	if (g_access (target_directory_path, F_OK) != 0) {
		g_free (target_directory_path);
		return MATE_VFS_ERROR_NOT_FOUND;
	}

	target_directory_uri = mate_vfs_get_uri_from_local_path (target_directory_path);
	g_free (target_directory_path);
	*result_uri = mate_vfs_uri_new (target_directory_uri);
	g_free (target_directory_uri);

	return MATE_VFS_OK;
}

static MateVFSResult
rename_helper (const gchar *old_full_name,
	       const gchar *new_full_name,
	       gboolean force_replace,
	       MateVFSContext *context)
{
	gboolean old_exists;
	struct stat statbuf;
	gint retval;
	gchar *temp_name;
	MateVFSHandle *temp_handle;
	MateVFSResult result;

	retval = g_stat (new_full_name, &statbuf);
	if (retval == 0) {
		/* Special case for files on case insensitive (vfat) filesystems:
		 * If the old and the new name only differ by case,
		 * try renaming via a temp file name.
		 */
		if (g_ascii_strcasecmp (old_full_name, new_full_name) == 0
		    && strcmp (old_full_name, new_full_name) != 0 && ! force_replace) {

			if (mate_vfs_context_check_cancellation (context))
				return MATE_VFS_ERROR_CANCELLED;

			result = mate_vfs_create_temp (old_full_name, &temp_name, &temp_handle);
			if (result != MATE_VFS_OK)
				return result;
			mate_vfs_close (temp_handle);
			g_unlink (temp_name);

			retval = g_rename (old_full_name, temp_name);
			if (retval == 0) {
				if (g_stat (new_full_name, &statbuf) != 0
				    && g_rename (temp_name, new_full_name) == 0) {
					/* Success */
					return MATE_VFS_OK;
				}
				/* Revert the filename back to original */
				retval = g_rename (temp_name, old_full_name);
				if (retval == 0) {
					return MATE_VFS_ERROR_FILE_EXISTS;
				}
			}
			return mate_vfs_result_from_errno_code (retval);

		} else if (! force_replace) {
			/* If we are not allowed to replace an existing file,
			 * return an error.
			 */
			return MATE_VFS_ERROR_FILE_EXISTS;
		}
		old_exists = TRUE;
	} else {
		old_exists = FALSE;
	}

	if (mate_vfs_context_check_cancellation (context))
		return MATE_VFS_ERROR_CANCELLED;

#ifdef G_OS_WIN32
	if (force_replace && old_exists)
		g_remove (new_full_name);
#endif

	retval = g_rename (old_full_name, new_full_name);

#ifndef G_OS_WIN32
	/* FIXME bugzilla.eazel.com 1186: The following assumes that,
	 * if `new_uri' and `old_uri' are on different file systems,
	 * `rename()' will always return `EXDEV' instead of `EISDIR',
	 * even if the old file is not a directory while the new one
	 * is. If this is not the case, we have to stat() both the
	 * old and new file.
	 */
	if (retval != 0 && errno == EISDIR && force_replace && old_exists) {
		/* The Unix version of `rename()' fails if the original file is
		   not a directory, while the new one is.  But we have been
		   explicitly asked to replace the destination name, so if the
		   new name points to a directory, we remove it manually.  */
		if (S_ISDIR (statbuf.st_mode)) {
			if (mate_vfs_context_check_cancellation (context))
				return MATE_VFS_ERROR_CANCELLED;
			retval = g_rmdir (new_full_name);
			if (retval != 0) {
				return mate_vfs_result_from_errno ();
			}

			if (mate_vfs_context_check_cancellation (context))
				return MATE_VFS_ERROR_CANCELLED;

			retval = g_rename (old_full_name, new_full_name);
		}
	}
#endif
	if (retval != 0) {
		return mate_vfs_result_from_errno ();
	}

	return MATE_VFS_OK;
}

static MateVFSResult
set_symlink_name_helper (const gchar *full_name,
			 const MateVFSFileInfo *info)
{
#ifndef G_OS_WIN32
	struct stat statbuf;

	if (info->symlink_name == NULL) {
		return MATE_VFS_ERROR_BAD_PARAMETERS;
	}

	if (g_lstat (full_name, &statbuf) != 0) {
		return mate_vfs_result_from_errno ();
	}

	if (!S_ISLNK (statbuf.st_mode)) {
		return MATE_VFS_ERROR_NOT_A_SYMBOLIC_LINK;
	}

	if (g_unlink (full_name) != 0) {
		return mate_vfs_result_from_errno ();
	}

	if (symlink (info->symlink_name, full_name) != 0) {
		/* TODO should we try to restore the old symlink? */
		return mate_vfs_result_from_errno ();
	}

	return MATE_VFS_OK;
#else
	return MATE_VFS_ERROR_NOT_SUPPORTED;
#endif

}

static MateVFSResult
do_move (MateVFSMethod *method,
	 MateVFSURI *old_uri,
	 MateVFSURI *new_uri,
	 gboolean force_replace,
	 MateVFSContext *context)
{
	gchar *old_full_name;
	gchar *new_full_name;
	MateVFSResult result;

	old_full_name = get_path_from_uri (old_uri);
	if (old_full_name == NULL)
		return MATE_VFS_ERROR_INVALID_URI;

	new_full_name = get_path_from_uri (new_uri);
	if (new_full_name == NULL) {
		g_free (old_full_name);
		return MATE_VFS_ERROR_INVALID_URI;
	}

	result = rename_helper (old_full_name, new_full_name,
				force_replace, context);

	g_free (old_full_name);
	g_free (new_full_name);

	return result;
}

static MateVFSResult
do_unlink (MateVFSMethod *method,
	   MateVFSURI *uri,
	   MateVFSContext *context)
{
	gchar *full_name;
	gint retval;

	full_name = get_path_from_uri (uri);
	if (full_name == NULL) {
		return MATE_VFS_ERROR_INVALID_URI;
	}

	retval = g_unlink (full_name);

	g_free (full_name);

	if (retval != 0) {
		return mate_vfs_result_from_errno ();
	}

	return MATE_VFS_OK;
}

static MateVFSResult
do_create_symbolic_link (MateVFSMethod *method,
			 MateVFSURI *uri,
			 const char *target_reference,
			 MateVFSContext *context)
{
#ifndef G_OS_WIN32
	const char *link_scheme, *target_scheme;
	char *link_full_name, *target_full_name;
	MateVFSResult result;
	MateVFSURI *target_uri;

	g_assert (target_reference != NULL);
	g_assert (uri != NULL);

	/* what we actually want is a function that takes a const char * and
	 * tells whether it is a valid URI
	 */
	target_uri = mate_vfs_uri_new (target_reference);
	if (target_uri == NULL) {
		return MATE_VFS_ERROR_INVALID_URI;
	}

	link_scheme = mate_vfs_uri_get_scheme (uri);
	g_assert (link_scheme != NULL);

	target_scheme = mate_vfs_uri_get_scheme (target_uri);
	if (target_scheme == NULL) {
		target_scheme = "file";
	}

	if ((strcmp (link_scheme, "file") == 0) && (strcmp (target_scheme, "file") == 0)) {
		/* symlink between two places on the local filesystem */
		if (strncmp (target_reference, "file", 4) != 0) {
			/* target_reference wasn't a full URI */
			target_full_name = strdup (target_reference);
		} else {
			target_full_name = get_path_from_uri (target_uri);
		}

		link_full_name = get_path_from_uri (uri);

		if (symlink (target_full_name, link_full_name) != 0) {
			result = mate_vfs_result_from_errno ();
		} else {
			result = MATE_VFS_OK;
		}

		g_free (target_full_name);
		g_free (link_full_name);
	} else {
		/* FIXME bugzilla.eazel.com 2792: do a URI link */
		result = MATE_VFS_ERROR_NOT_SUPPORTED;
	}

	mate_vfs_uri_unref (target_uri);

	return result;
#else
	return MATE_VFS_ERROR_NOT_SUPPORTED;
#endif
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
	gchar *full_name_source, *full_name_target;
	struct stat s_source, s_target;
	gint retval;

	full_name_source = get_path_from_uri (source_uri);
	retval = g_lstat (full_name_source, &s_source);
	g_free (full_name_source);

	if (retval != 0)
		return mate_vfs_result_from_errno ();

	if (mate_vfs_context_check_cancellation (context))
		return MATE_VFS_ERROR_CANCELLED;

	full_name_target = get_path_from_uri (target_uri);
	retval = g_stat (full_name_target, &s_target);
	g_free (full_name_target);

	if (retval != 0)
		return mate_vfs_result_from_errno ();

	*same_fs_return = (s_source.st_dev == s_target.st_dev);

	return MATE_VFS_OK;
}

static MateVFSResult
do_set_file_info (MateVFSMethod *method,
		  MateVFSURI *uri,
		  const MateVFSFileInfo *info,
		  MateVFSSetFileInfoMask mask,
		  MateVFSContext *context)
{
	gchar *full_name;

	full_name = get_path_from_uri (uri);
	if (full_name == NULL)
		return MATE_VFS_ERROR_INVALID_URI;

	if (mask & MATE_VFS_SET_FILE_INFO_NAME) {
		MateVFSResult result;
		gchar *dir, *encoded_dir;
		gchar *new_name;

		encoded_dir = mate_vfs_uri_extract_dirname (uri);
		dir = mate_vfs_unescape_string (encoded_dir, DIR_SEPARATORS);
		g_free (encoded_dir);
		g_assert (dir != NULL);

		new_name = g_build_filename (dir, info->name, NULL);

		result = rename_helper (full_name, new_name, FALSE, context);

		g_free (dir);
		g_free (full_name);
		full_name = new_name;

		if (result != MATE_VFS_OK) {
			g_free (full_name);
			return result;
		}
	}

	if (mask & MATE_VFS_SET_FILE_INFO_SELINUX_CONTEXT) {
		MateVFSResult result = set_selinux_context(info, full_name);
		if (result < 0) {
			g_free(full_name);
			return result;
		}
	}

	if (mate_vfs_context_check_cancellation (context)) {
		g_free (full_name);
		return MATE_VFS_ERROR_CANCELLED;
	}

	if (mask & MATE_VFS_SET_FILE_INFO_PERMISSIONS) {
		if (chmod (full_name, info->permissions) != 0) {
			g_free (full_name);
			return mate_vfs_result_from_errno ();
		}
	}

	if (mate_vfs_context_check_cancellation (context)) {
		g_free (full_name);
		return MATE_VFS_ERROR_CANCELLED;
	}

	if (mask & MATE_VFS_SET_FILE_INFO_OWNER) {
#ifndef G_OS_WIN32
		if (chown (full_name, info->uid, info->gid) != 0) {
			g_free (full_name);
			return mate_vfs_result_from_errno ();
		}
#else
		g_warning ("Not implemented: MATE_VFS_SET_FILE_INFO_OWNER");
#endif
	}

	if (mate_vfs_context_check_cancellation (context)) {
		g_free (full_name);
		return MATE_VFS_ERROR_CANCELLED;
	}

	if (mask & MATE_VFS_SET_FILE_INFO_TIME) {
		struct utimbuf utimbuf;

		utimbuf.actime = info->atime;
		utimbuf.modtime = info->mtime;

		if (utime (full_name, &utimbuf) != 0) {
			g_free (full_name);
			return mate_vfs_result_from_errno ();
		}
	}

	if (mate_vfs_context_check_cancellation (context)) {
		g_free (full_name);
		return MATE_VFS_ERROR_CANCELLED;
	}

	if (mask & MATE_VFS_SET_FILE_INFO_ACL) {
		MateVFSResult result;

		result = file_set_acl (full_name, info, context);
		if (result != MATE_VFS_OK) {
			g_free (full_name);
			return result;
		}
	}

	if (mask & MATE_VFS_SET_FILE_INFO_SYMLINK_NAME) {
		MateVFSResult result;

		result = set_symlink_name_helper (full_name, info);
		if (result != MATE_VFS_OK) {
			g_free (full_name);
			return result;
		}
	}

	g_free (full_name);

	return MATE_VFS_OK;
}

#ifdef HAVE_FAM
static gboolean
fam_do_iter_unlocked (void)
{
	while (fam_connection != NULL && FAMPending(fam_connection)) {
		FAMEvent ev;
		FileMonitorHandle *handle;
		gboolean cancelled;
		MateVFSMonitorEventType event_type;

		if (FAMNextEvent(fam_connection, &ev) != 1) {
			FAMClose(fam_connection);
			g_free(fam_connection);
			g_source_remove (fam_watch_id);
			fam_watch_id = 0;
			fam_connection = NULL;
			return FALSE;
		}

		handle = (FileMonitorHandle *)ev.userdata;
		cancelled = handle->cancelled;
		event_type = -1;

		switch (ev.code) {
			case FAMChanged:
				event_type = MATE_VFS_MONITOR_EVENT_CHANGED;
				break;
			case FAMDeleted:
				event_type = MATE_VFS_MONITOR_EVENT_DELETED;
				break;
			case FAMStartExecuting:
				event_type = MATE_VFS_MONITOR_EVENT_STARTEXECUTING;
				break;
			case FAMStopExecuting:
				event_type = MATE_VFS_MONITOR_EVENT_STOPEXECUTING;
				break;
			case FAMCreated:
				event_type = MATE_VFS_MONITOR_EVENT_CREATED;
				break;
			case FAMAcknowledge:
				if (handle->cancelled) {
					mate_vfs_uri_unref (handle->uri);
					g_free (handle);
				}
				break;
			case FAMExists:
			case FAMEndExist:
			case FAMMoved:
				/* Not supported */
				break;
		}

		if (event_type != -1 && !cancelled) {
			MateVFSURI *info_uri;
			gchar *info_str;

			/*
			 * FAM can send events with either a absolute or
			 * relative (from the monitored URI) path, so check if
			 * the filename starts with '/'.
			 */
			if (ev.filename[0] == '/') {
				info_str = mate_vfs_get_uri_from_local_path (ev.filename);
				info_uri = mate_vfs_uri_new (info_str);
				g_free (info_str);
			} else
				info_uri = mate_vfs_uri_append_file_name (handle->uri, ev.filename);

			/* This queues an idle, so there are no reentrancy issues */
			mate_vfs_monitor_callback ((MateVFSMethodHandle *)handle,
						    info_uri,
						    event_type);
			mate_vfs_uri_unref (info_uri);
		}
	}

	return TRUE;
}

static gboolean
fam_callback (GIOChannel *source,
	      GIOCondition condition,
	      gpointer data)
{
	gboolean res;
	G_LOCK (fam_connection);

	res = fam_do_iter_unlocked ();

	G_UNLOCK (fam_connection);

	return res;
}



static gboolean
monitor_setup (void)
{
	GIOChannel *ioc;

	G_LOCK (fam_connection);

	if (fam_connection == NULL) {
		fam_connection = g_malloc0(sizeof(FAMConnection));
		if (FAMOpen2(fam_connection, "mate-vfs user") != 0) {
#ifdef DEBUG_FAM
			g_print ("FAMOpen failed, FAMErrno=%d\n", FAMErrno);
#endif
			g_free(fam_connection);
			fam_connection = NULL;
			G_UNLOCK (fam_connection);
			return FALSE;
		}
		ioc = g_io_channel_unix_new (FAMCONNECTION_GETFD(fam_connection));
		fam_watch_id = g_io_add_watch (ioc,
					       G_IO_IN | G_IO_HUP | G_IO_ERR,
					       fam_callback, fam_connection);
		g_io_channel_unref (ioc);
	}

	G_UNLOCK (fam_connection);

	return TRUE;
}
#endif

#ifdef HAVE_FAM
static MateVFSResult
fam_monitor_cancel (MateVFSMethod *method,
		    MateVFSMethodHandle *method_handle)
{
	FileMonitorHandle *handle = (FileMonitorHandle *)method_handle;

	if (!monitor_setup ()) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

	if (handle->cancelled)
		return MATE_VFS_OK;

	handle->cancelled = TRUE;
	G_LOCK (fam_connection);

	/* We need to queue up incoming messages to avoid blocking on write
	   if there are many monitors being canceled */
	fam_do_iter_unlocked ();

	if (fam_connection == NULL) {
		G_UNLOCK (fam_connection);
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

	FAMCancelMonitor (fam_connection, &handle->request);
	G_UNLOCK (fam_connection);

	return MATE_VFS_OK;
}


static MateVFSResult
fam_monitor_add (MateVFSMethod *method,
		 MateVFSMethodHandle **method_handle_return,
		 MateVFSURI *uri,
		 MateVFSMonitorType monitor_type)
{
	FileMonitorHandle *handle;
	char *filename;

	if (!monitor_setup ()) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

	filename = get_path_from_uri (uri);
	if (filename == NULL) {
		return MATE_VFS_ERROR_INVALID_URI;
	}

	handle = g_new0 (FileMonitorHandle, 1);
	handle->cancel_func = fam_monitor_cancel;
	handle->uri = uri;
	handle->cancelled = FALSE;
	mate_vfs_uri_ref (uri);

	G_LOCK (fam_connection);
	/* We need to queue up incoming messages to avoid blocking on write
	   if there are many monitors being added */
	fam_do_iter_unlocked ();

	if (fam_connection == NULL) {
		G_UNLOCK (fam_connection);
		g_free (handle);
		mate_vfs_uri_unref (uri);
		g_free (filename);
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

	if (monitor_type == MATE_VFS_MONITOR_FILE) {
		FAMMonitorFile (fam_connection, filename,
			&handle->request, handle);
	} else {
		FAMMonitorDirectory (fam_connection, filename,
			&handle->request, handle);
	}

	G_UNLOCK (fam_connection);

	*method_handle_return = (MateVFSMethodHandle *)handle;

	g_free (filename);

	return MATE_VFS_OK;

}
#endif

#ifdef USE_INOTIFY

static MateVFSResult
inotify_monitor_cancel (MateVFSMethod *method,
			MateVFSMethodHandle *method_handle)
{
	ih_sub_t *sub = (ih_sub_t *)method_handle;

	if (sub->cancelled)
		return MATE_VFS_OK;

	ih_sub_cancel (sub);
	ih_sub_free (sub);
	return MATE_VFS_OK;

}

static MateVFSResult
inotify_monitor_add (MateVFSMethod *method,
		     MateVFSMethodHandle **method_handle_return,
		     MateVFSURI *uri,
		     MateVFSMonitorType monitor_type)
{
	ih_sub_t *sub;

	sub = ih_sub_new (uri, monitor_type);
	if (sub == NULL) {
		return MATE_VFS_ERROR_INVALID_URI;
	}
	sub->cancel_func = inotify_monitor_cancel;
	if (ih_sub_add (sub) == FALSE) {
		ih_sub_free (sub);
		*method_handle_return = NULL;
		return MATE_VFS_ERROR_INVALID_URI;
	}

	*method_handle_return = (MateVFSMethodHandle *)sub;
	return MATE_VFS_OK;

}
#endif

static MateVFSResult
do_monitor_add (MateVFSMethod *method,
		MateVFSMethodHandle **method_handle_return,
		MateVFSURI *uri,
		MateVFSMonitorType monitor_type)
{
#ifdef USE_INOTIFY
	/* For remote files, always prefer FAM */
	if (do_is_local (method, uri) && ih_startup ()) {
		return inotify_monitor_add (method, method_handle_return, uri, monitor_type);
	}
#endif
#ifdef HAVE_FAM
	return fam_monitor_add (method, method_handle_return, uri, monitor_type);
#endif
	return MATE_VFS_ERROR_NOT_SUPPORTED;
}

static MateVFSResult
do_monitor_cancel (MateVFSMethod *method,
		   MateVFSMethodHandle *method_handle)
{
	AnyFileMonitorHandle *handle;

	handle = (AnyFileMonitorHandle *)method_handle;
	if (handle != NULL) {
		return (handle->cancel_func) (method, method_handle);
	}
	return MATE_VFS_ERROR_NOT_SUPPORTED;
}

static MateVFSResult
do_file_control (MateVFSMethod *method,
		 MateVFSMethodHandle *method_handle,
		 const char *operation,
		 gpointer operation_data,
		 MateVFSContext *context)
{
	if (strcmp (operation, "file:test") == 0) {
		*(char **)operation_data = g_strdup ("test ok");
		return MATE_VFS_OK;
	}
	return MATE_VFS_ERROR_NOT_SUPPORTED;
}

static MateVFSResult
do_get_volume_free_space (MateVFSMethod *method,
			  const MateVFSURI *uri,
			  MateVFSFileSize *free_space)
{
#ifndef G_OS_WIN32
	MateVFSFileSize free_blocks, block_size;
       	int statfs_result;
	const char *path;
	char *unescaped_path;

#if HAVE_STATVFS
	struct statvfs statfs_buffer;
#else
	struct statfs statfs_buffer;
#endif

 	*free_space = 0;

	path = mate_vfs_uri_get_path (uri);
	if (path == NULL || *path != '/') {
		return MATE_VFS_ERROR_INVALID_URI;
	}

	unescaped_path = mate_vfs_unescape_string (path, G_DIR_SEPARATOR_S);

#if HAVE_STATVFS
	statfs_result = statvfs (unescaped_path, &statfs_buffer);
	block_size = statfs_buffer.f_frsize;
#else
#if STATFS_ARGS == 2
	statfs_result = statfs (unescaped_path, &statfs_buffer);
#elif STATFS_ARGS == 4
	statfs_result = statfs (unescaped_path, &statfs_buffer,
				sizeof (statfs_buffer), 0);
#endif
	block_size = statfs_buffer.f_bsize;
#endif

	if (statfs_result != 0) {
		g_free (unescaped_path);
		return mate_vfs_result_from_errno ();
	}


/* CF: I assume ncpfs is linux specific, if you are on a non-linux platform
 * where ncpfs is available, please file a bug about it on bugzilla.gnome.org
 * (2004-03-08)
 */
#if defined(__linux__)
	/* ncpfs does not know the amount of available and free space */
	if (statfs_buffer.f_bavail == 0 && statfs_buffer.f_bfree == 0) {
		/* statvfs does not contain an f_type field, we try again
		 * with statfs.
		 */
		struct statfs statfs_buffer2;
		statfs_result = statfs (unescaped_path, &statfs_buffer2);
		g_free (unescaped_path);

		if (statfs_result != 0) {
			return mate_vfs_result_from_errno ();
		}

		/* linux/ncp_fs.h: NCP_SUPER_MAGIC == 0x564c */
		if (statfs_buffer2.f_type == 0x564c)
		{
			return MATE_VFS_ERROR_NOT_SUPPORTED;
		}
	} else {
		/* everything is copacetic... free the unescaped path */
		g_free (unescaped_path);
	}
#else
	g_free (unescaped_path);
#endif
	free_blocks = statfs_buffer.f_bavail;

	*free_space = block_size * free_blocks;

	return MATE_VFS_OK;
#else /* G_OS_WIN32 */
	g_warning ("Not implemented for WIN32: file-method.c:do_get_volume_free_space()");
	return MATE_VFS_ERROR_NOT_SUPPORTED;
#endif
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
	do_file_control,
	do_forget_cache,
	do_get_volume_free_space
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
