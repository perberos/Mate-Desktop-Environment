/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-open-fd.c - convert a file descriptor to a handle

   Copyright (C) 2002 Giovanni Corriga

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

   Author: Giovanni Corriga <valkadesh@libero.it>
*/

#include <config.h>
#include <errno.h>

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#include "mate-vfs-uri.h"
#include "mate-vfs-method.h"
#include "mate-vfs-handle.h"
#include "mate-vfs-module-shared.h"
#include "mate-vfs-mime.h"
#include "mate-vfs-handle-private.h"
#include "mate-vfs-utils.h"

#ifdef G_OS_WIN32
#include <errno.h>
#define ftruncate(fd,length) ((length < G_MAXLONG) ? _chsize (fd, length) : (errno = EINVAL, -1))
#endif

static MateVFSURI*
create_anonymous_uri (MateVFSMethod* method)
{
	MateVFSToplevelURI* tl_uri;
	MateVFSURI* uri;
	
	tl_uri = g_new0 (MateVFSToplevelURI, 1);
	
	uri = (MateVFSURI *) tl_uri;
	
	uri->ref_count = 1;
	uri->method = method;
	
	return uri;
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
do_close (MateVFSMethod *method,
	  MateVFSMethodHandle *method_handle,
	  MateVFSContext *context)
{
	FileHandle *file_handle;
	gint close_retval;

	g_return_val_if_fail (method_handle != NULL, MATE_VFS_ERROR_INTERNAL);

	file_handle = (FileHandle *) method_handle;

	do {
		close_retval = close (file_handle->fd);
	} while (close_retval != 0
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

	do {
		write_val = write (file_handle->fd, buffer, num_bytes);
	} while (write_val == -1
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

	if (lseek (file_handle->fd, offset, lseek_whence) == -1) {
		if (errno == ESPIPE) {
			return MATE_VFS_ERROR_NOT_SUPPORTED;
		} else {
			return mate_vfs_result_from_errno ();
		}
	}

	return MATE_VFS_OK;
}

static MateVFSResult
do_tell (MateVFSMethod *method,
	 MateVFSMethodHandle *method_handle,
	 MateVFSFileSize *offset_return)
{
	FileHandle *file_handle;
	off_t offset;

	file_handle = (FileHandle *) method_handle;

	offset = lseek (file_handle->fd, 0, SEEK_CUR);
	if (offset == -1) {
		if (errno == ESPIPE) {
			return MATE_VFS_ERROR_NOT_SUPPORTED;
		} else {
			return mate_vfs_result_from_errno ();
		}
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

/* MIME detection code.  */
static void
get_mime_type (MateVFSFileInfo *info,
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
		mime_type = mate_vfs_get_file_mime_type (NULL, stat_buffer,
				(options & MATE_VFS_FILE_INFO_FORCE_FAST_MIME_TYPE) != 0);
	}

	g_assert (mime_type);
	info->mime_type = g_strdup (mime_type);
	info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE;
}

static MateVFSResult
do_get_file_info_from_handle (MateVFSMethod *method,
			      MateVFSMethodHandle *method_handle,
			      MateVFSFileInfo *file_info,
			      MateVFSFileInfoOptions options,
			      MateVFSContext *context)
{
	FileHandle *file_handle;
	struct stat statbuf;
	MateVFSResult result;

	file_handle = (FileHandle *) method_handle;

	file_info->valid_fields = MATE_VFS_FILE_INFO_FIELDS_NONE;

	result = get_stat_info_from_handle (file_info, file_handle,
					    options, &statbuf);
	if (result != MATE_VFS_OK) {
		return result;
	}

	if (options & MATE_VFS_FILE_INFO_GET_MIME_TYPE) {
		get_mime_type (file_info, options, &statbuf);
	}

	return MATE_VFS_OK;
}

static gboolean
do_is_local (MateVFSMethod *method,
	     const MateVFSURI *uri)
{
	return TRUE;
}

static MateVFSMethod method = {
	sizeof (MateVFSMethod),
	NULL, /* do_open */
	NULL, /* do_create */
	do_close,
	do_read,
	do_write,
	do_seek,
	do_tell,
	do_truncate_handle,
	NULL, /* do_open_directory */
	NULL, /* do_close_directory */
	NULL, /* do_read_directory */
	NULL, /* do_get_file_info */
	do_get_file_info_from_handle,
	do_is_local,
	NULL, /* do_make_directory */
	NULL, /* do_remove_directory */
	NULL, /* do_move */
	NULL, /* do_unlink */
	NULL, /* do_check_same_fs */
	NULL, /* do_set_file_info */
	NULL, /* do_truncate */
	NULL, /* do_find_directory */
	NULL, /* do_create_symbolic_link */
	NULL, /* do_monitor_add */
	NULL, /* do_monitor_cancel */
};

static MateVFSOpenMode
get_open_mode (gint filedes)
{
#ifndef G_OS_WIN32
	int flags;

	flags = fcntl(filedes, F_GETFL);
	if (flags & O_RDONLY) {
		return MATE_VFS_OPEN_READ;
	} else if (flags & O_WRONLY) {
		return MATE_VFS_OPEN_WRITE;
	} else if (flags & O_RDWR) {
		return (MATE_VFS_OPEN_READ | MATE_VFS_OPEN_WRITE);
	}
#endif

	return MATE_VFS_OPEN_READ; /* bogus */
}

/**
 * mate_vfs_open_fd:
 * @handle: A pointer to a pointer to a MateVFSHandle object
 * @filedes: a UNIX file descriptor
 * 
 * Converts an open unix file descript into a MateVFSHandle that 
 * can be used with the normal MateVFS file operations. When the
 * handle is closed the file descriptor will also be closed.
 *
 * Return Value: %MATE_VFS_OK if the open was ok, a suitable error otherwise.
 *
 * Since 2.2
 **/

MateVFSResult
mate_vfs_open_fd (MateVFSHandle **handle, int filedes)
{
	MateVFSURI* uri;
	FileHandle* file_handle;
	MateVFSOpenMode open_mode;
	
	g_return_val_if_fail(handle != NULL, MATE_VFS_ERROR_BAD_PARAMETERS);
	
	uri = create_anonymous_uri (&method);

	open_mode = get_open_mode (filedes);
	
	file_handle = file_handle_new (uri, filedes);
	
	*handle = _mate_vfs_handle_new (uri, (MateVFSMethodHandle*)file_handle, open_mode);
	if (!handle) {
		return MATE_VFS_ERROR_INTERNAL;
	}
	
	return MATE_VFS_OK;
}

