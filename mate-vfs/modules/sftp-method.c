/* -*- Mode: C; tab-width: 8; indent-tabs-mode: 8; c-basic-offset: 8 -*- */

/* sftp-method.c - Mate VFS module for SFTP
 *
 * Copyright (C) 2002 Bradford Hovinen
 * Portions copyright (C) 2001, 2002 Damien Miller
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * Author: Bradford Hovinen <bghovinen@math.uwaterloo.ca>
 */

/* Portions of this file are derived from OpenSSH.
 *
 * Copyright notice for derived sources follows:
 *
 * Copyright (c) 2001,2002 Damien Miller.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <config.h>

#include <libmatevfs/mate-vfs-context.h>
#include <libmatevfs/mate-vfs-method.h>
#include <libmatevfs/mate-vfs-module.h>
#include <libmatevfs/mate-vfs-module-shared.h>
#include <libmatevfs/mate-vfs-utils.h>
#include <libmatevfs/mate-vfs-mime.h>
#include <libmatevfs/mate-vfs-mime-utils.h>
#include <libmatevfs/mate-vfs-module-callback-module-api.h>
#include <libmatevfs/mate-vfs-ops.h>
#include <libmatevfs/mate-vfs-private-utils.h>
#include <libmatevfs/mate-vfs-standard-callbacks.h>

#include <string.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/time.h>

#include <glib/gi18n-lib.h>

#include "sftp.h"
#include <libmatevfs/mate-vfs-pty.h>

static size_t default_req_len = 32*1024;
static guint max_req = 8;

#ifdef HAVE_GRANTPT
/* We only use this on systems with unix98 ptys */
#define USE_PTY 1
#endif

typedef struct {
	gchar         *hash_name;
	gint           in_fd;
	gint           out_fd;
	gint           tty_fd;
	GIOChannel    *error_channel;
	pid_t          ssh_pid;

	guint          msg_id;
	guint          version;

	guint          ref_count;
	guint          close_timeout_id;

	GMutex        *mutex;

	guint          event_id;
	MateVFSResult status;
} SftpConnection;

typedef struct 
{
	MateVFSMethodHandle  method_handle;
	gchar                *sftp_handle;
	gint                  sftp_handle_len;
	SftpConnection       *connection;
	guint64               offset;
	MateVFSFileInfo     *info;
	guint                 info_alloc;
	guint                 info_read_ptr;
	guint                 info_write_ptr;
	char                 *path;
	MateVFSFileInfoOptions dir_options; 
} SftpOpenHandle;

static GHashTable *sftp_connection_table = NULL;

G_LOCK_DEFINE_STATIC (sftp_connection_table);

#define SFTP_CONNECTION(p) ((SftpConnection *) (p))
#define SFTP_OPEN_HANDLE(p) ((SftpOpenHandle *) (p))

#define SFTP_CLOSE_TIMEOUT (10 * 60 * 1000)      /* Ten minutes */
#define INIT_DIR_INFO_ALLOC 16
#define INIT_BUFFER_ALLOC   128

#ifdef FULL_TRACE
#  ifndef PARTIAL_TRACE
#    define PARTIAL_TRACE
#  endif
#  ifndef MINIMAL_TRACE
#    define MINIMAL_TRACE
#  endif
#  define DEBUG4(x) x
#else
#  define DEBUG4(x)
#endif

#ifdef PARTIAL_TRACE
#  ifndef MINIMAL_TRACE
#    define MINIMAL_TRACE
#  endif
#  define DEBUG2(x) x
#else
#  define DEBUG2(x)
#endif

#ifdef MINIMAL_TRACE
#  define DEBUG(x) x
#else
#  define DEBUG(x)
#endif

#define URI_TO_PATH(uri,path) \
	path = mate_vfs_unescape_string (mate_vfs_uri_get_path (uri), NULL); \
	if (path == NULL || !strcmp (path, "")) { \
		g_free (path); \
		path = g_strdup ("/"); \
	}

static MateVFSResult do_check_same_fs (MateVFSMethod  *method,
					MateVFSURI     *a,
					MateVFSURI     *b,
					gboolean        *same_fs_return,
					MateVFSContext *context);
static MateVFSResult do_get_file_info_from_handle (MateVFSMethod          *method,
						    MateVFSMethodHandle    *method_handle,
						    MateVFSFileInfo        *file_info,
						    MateVFSFileInfoOptions  options,
						    MateVFSContext         *context);

static void sftp_connection_ref (SftpConnection *connection);

static gboolean sftp_connection_process_errors (GIOChannel *channel,
						GIOCondition cond,
						MateVFSResult *status);

typedef struct
{
	char  *base;
	char  *read_ptr;
	char  *write_ptr;
	gint     alloc;
} Buffer;

/* Inspired by atomicio() from OpenSSH */

typedef ssize_t (*read_write_fn) (int, void *, size_t);

static gssize
atomic_io (read_write_fn f, gint fd, gpointer buffer_in, gsize size) 
{
	gssize pos = 0, res;
	guchar *buffer;
        long int __result;

	buffer = buffer_in;

	while (pos < size) {
	  	do __result = (long int) (f (fd, buffer, size - pos));
		while (__result == -1L && errno == EINTR);
		res = __result;

		if (res < 0)
			return -1;
		else if (res == 0)
			return pos;

		buffer += res;
		pos += res;
	}

	return pos;
}

static void
buffer_init (Buffer *buf) 
{
	g_return_if_fail (buf != NULL);

	buf->base = g_new0 (gchar, INIT_BUFFER_ALLOC);
	buf->read_ptr = buf->base + sizeof (guint);
	buf->write_ptr = buf->base + sizeof (guint);
	buf->alloc = INIT_BUFFER_ALLOC;
}

static void
buffer_free (Buffer *buf) 
{
	g_return_if_fail (buf != NULL);

	if (buf->base == NULL) {
		g_critical ("No initialized buffers present. Something is being double-freed");
		return;
	}

	g_free (buf->base);
	buf->base = buf->read_ptr = buf->write_ptr = NULL;
	buf->alloc = 0;
}

static void
buffer_check_alloc (Buffer *buf, guint32 size)
{
	guint32 r_len, w_len;

	g_return_if_fail (buf != NULL);
	g_return_if_fail (buf->base != NULL);

	DEBUG4 (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Allocating %p to %d", buf, size));

	while (buf->write_ptr - buf->base + size > buf->alloc) {
		buf->alloc *= 2;
		r_len = buf->read_ptr - buf->base;
		w_len = buf->write_ptr - buf->base;
		buf->base = g_realloc (buf->base, buf->alloc);
		buf->read_ptr = buf->base + r_len;
		buf->write_ptr = buf->base + w_len;
	}
}

static void
buffer_clear (Buffer *buf) 
{
	g_return_if_fail (buf != NULL);
	g_return_if_fail (buf->base != NULL);

	DEBUG4 (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Clearing %p", buf));

	buf->read_ptr = buf->write_ptr = buf->base + sizeof (guint);
}

static void
buffer_read (Buffer *buf, gpointer data, guint32 size) 
{
	guint32 len;

	g_return_if_fail (buf != NULL);
	g_return_if_fail (buf->base != NULL);

	DEBUG4 (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Reading %d from %p to %p", size, buf, data));

	if (buf->write_ptr - buf->read_ptr < size)
		g_critical ("Could not read %d bytes", size);

	len = MIN (size, buf->write_ptr - buf->read_ptr);
	memcpy (data, buf->read_ptr, len);
	buf->read_ptr += len;
}

static void
buffer_write (Buffer *buf, gconstpointer data, guint32 size) 
{
	g_return_if_fail (buf != NULL);
	g_return_if_fail (buf->base != NULL);

	DEBUG4 (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Writing %d from %p to %p", size, data, buf));

	buffer_check_alloc (buf, size);
	memcpy (buf->write_ptr, data, size);
	buf->write_ptr += size;
}

static MateVFSResult
buffer_send (Buffer *buf, int fd) 
{
	guint bytes_written = 0;
	guint32 len = buf->write_ptr - buf->read_ptr;
	guint32 w_len = GINT32_TO_BE (len);
	MateVFSResult res = MATE_VFS_OK;

	g_return_val_if_fail (buf != NULL, MATE_VFS_ERROR_INTERNAL);
	g_return_val_if_fail (buf->base != NULL, MATE_VFS_ERROR_INTERNAL);

	DEBUG2 (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Sending message of length %d from %p to %d",
		       G_STRFUNC, len, buf, fd));

	buf->read_ptr -= sizeof (guint32);

	*((guint32 *) buf->read_ptr) = w_len;

	if ((bytes_written = atomic_io ((read_write_fn) write, fd, buf->read_ptr,
					buf->write_ptr - buf->read_ptr)) < 0)
	{
		g_critical ("Could not write entire buffer: %s", g_strerror (errno));
		res = MATE_VFS_ERROR_IO;
	} else {
		if (bytes_written == buf->write_ptr - buf->read_ptr)
			buf->read_ptr = buf->write_ptr = buf->base + sizeof (guint32);
		else
			buf->read_ptr += bytes_written;

		res = MATE_VFS_OK;
	}

	DEBUG2 (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: %d bytes written",
		       G_STRFUNC, bytes_written));

	return res;
}

static MateVFSResult
buffer_recv (Buffer *buf, int fd) 
{
	guint32 r_len, len, bytes_read;

	g_return_val_if_fail (buf != NULL, MATE_VFS_ERROR_INTERNAL);
	g_return_val_if_fail (buf->base != NULL, MATE_VFS_ERROR_INTERNAL);

	DEBUG2 (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Receiving message from %d to %p",
		       G_STRFUNC, fd, buf));

	bytes_read = atomic_io (read, fd, &r_len, sizeof (guint32));

	if (bytes_read == -1) {
		DEBUG2 (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Could not read length: %s",
			       G_STRFUNC, g_strerror (errno)));
		return MATE_VFS_ERROR_IO;
	}
	else if (bytes_read == 0) {
		DEBUG2 (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Connection closed: %d",
			       G_STRFUNC, fd));
		return MATE_VFS_ERROR_IO;
	}

	len = GINT32_TO_BE (r_len);

	DEBUG2 (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Message is of length %d", G_STRFUNC, len));

	/* 256K was the max allowed in OpenSSH */
	if (len > 256 * 1024) {
		g_critical ("Message length too long: %d", len);
		return MATE_VFS_ERROR_IO;
	}

	buffer_check_alloc (buf, len);

	if ((bytes_read = atomic_io (read, fd, buf->write_ptr, len)) == -1) {
		g_critical ("Could not read data: %s", g_strerror (errno));
		return MATE_VFS_ERROR_IO;
	}

	DEBUG2 (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: %d bytes read",
		       G_STRFUNC, bytes_read));

	buf->write_ptr += bytes_read;

	return MATE_VFS_OK;
}

static gchar
buffer_read_gchar (Buffer *buf) 
{
	gchar data;

	g_return_val_if_fail (buf != NULL, 0);
	g_return_val_if_fail (buf->base != NULL, 0);

	buffer_read (buf, &data, sizeof (gchar));

	DEBUG4 (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Read byte %d from %p", data, buf));

	return data;
}

static gint32
buffer_read_gint32 (Buffer *buf) 
{
	gint32 data;

	g_return_val_if_fail (buf != NULL, 0);
	g_return_val_if_fail (buf->base != NULL, 0);

	buffer_read (buf, &data, sizeof (gint32));

	DEBUG4 (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Read word %d from %p",
		       GINT32_TO_BE (data), buf));

	return GINT32_TO_BE (data);
}

static gint64
buffer_read_gint64 (Buffer *buf) 
{
	gint64 data;

	g_return_val_if_fail (buf != NULL, 0);
	g_return_val_if_fail (buf->base != NULL, 0);

	buffer_read (buf, &data, sizeof (gint64));

	DEBUG4 (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Read doubleword %lld from %p",
		       GINT64_TO_BE (data), buf));

	return GINT64_TO_BE (data);
}

static gpointer
buffer_read_block (Buffer *buf, gint32 *p_len) 
{
	gint32 len;
	gpointer data;

	g_return_val_if_fail (buf != NULL, NULL);
	g_return_val_if_fail (buf->base != NULL, NULL);

	if (p_len == NULL)
		p_len = &len;

	*p_len = buffer_read_gint32 (buf);
	data = g_new (gchar, *p_len);
	buffer_read (buf, data, *p_len);

	DEBUG4 (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Read block of length %d from %p", *p_len, buf));

	return data;
}

static gchar *
buffer_read_string (Buffer *buf, gint32 *p_len) 
{
	gint32 len;
	gchar *data;

	g_return_val_if_fail (buf != NULL, NULL);
	g_return_val_if_fail (buf->base != NULL, NULL);

	if (p_len == NULL)
		p_len = &len;

	*p_len = buffer_read_gint32 (buf);
	data = g_new (gchar, *p_len + 1);
	buffer_read (buf, data, *p_len);

	data[*p_len] = '\0';

	DEBUG4 (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Read string of length %d from %p: %s",
		       *p_len, buf, data));

	return data;
}

/* Derived from OpenSSH, sftp-client.c:decode_stat */

static void
buffer_read_file_info (Buffer *buf, MateVFSFileInfo *info) 
{
	gint32 flags;

	g_return_if_fail (buf != NULL);
	g_return_if_fail (buf->base != NULL);

	flags = buffer_read_gint32 (buf);

	info->valid_fields = MATE_VFS_FILE_INFO_FIELDS_NONE;

	if (flags & SSH2_FILEXFER_ATTR_SIZE) {
		info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_SIZE;
		info->size = buffer_read_gint64 (buf);
	}
	
	if (flags & SSH2_FILEXFER_ATTR_UIDGID) {
		/* Deprecated in recent SFTP drafts, it isn't really useful
		 * since it doesn't relate to the local UID/GID info.
		 *
		 * We skip the buffer, but ignore the contents. */
		buffer_read_gint32 (buf); /* UID */
		buffer_read_gint32 (buf); /* GID */
	}

	/* TODO SSH_FILEXFER_ATTR_OWNERGROUP */

	if (flags & SSH2_FILEXFER_ATTR_PERMISSIONS) {
		info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_PERMISSIONS;
		info->permissions = buffer_read_gint32 (buf);

		DEBUG4 (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Permissions is %x", G_STRFUNC,
			       info->permissions));

		info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_TYPE;
		
		if (S_ISREG (info->permissions)) info->type = MATE_VFS_FILE_TYPE_REGULAR;		
		else if (S_ISDIR (info->permissions)) info->type = MATE_VFS_FILE_TYPE_DIRECTORY;
		else if (S_ISFIFO (info->permissions)) info->type = MATE_VFS_FILE_TYPE_FIFO;
		else if (S_ISSOCK (info->permissions)) info->type = MATE_VFS_FILE_TYPE_SOCKET;
		else if (S_ISCHR (info->permissions)) info->type = MATE_VFS_FILE_TYPE_CHARACTER_DEVICE;
		else if (S_ISBLK (info->permissions)) info->type = MATE_VFS_FILE_TYPE_BLOCK_DEVICE;
		else if (S_ISLNK (info->permissions)) info->type = MATE_VFS_FILE_TYPE_SYMBOLIC_LINK;
		else info->type = MATE_VFS_FILE_TYPE_UNKNOWN;

		info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_FLAGS;
		info->flags = (info->type == MATE_VFS_FILE_TYPE_SYMBOLIC_LINK) ?
			MATE_VFS_FILE_FLAGS_SYMLINK : MATE_VFS_FILE_FLAGS_NONE;
	}
	if (flags & SSH2_FILEXFER_ATTR_ACMODTIME) {
		info->valid_fields |=
			MATE_VFS_FILE_INFO_FIELDS_ATIME | MATE_VFS_FILE_INFO_FIELDS_MTIME;
		info->atime = buffer_read_gint32 (buf);
		info->mtime = buffer_read_gint32 (buf);
	}

	info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_IO_BLOCK_SIZE;
	info->io_block_size = default_req_len * max_req;

	DEBUG4 (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Read file info from %p", buf));
}

static void
buffer_write_gchar (Buffer *buf, gchar data) 
{
	g_return_if_fail (buf != NULL);
	g_return_if_fail (buf->base != NULL);

	DEBUG4 (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Writing byte %d to %p", data, buf));

	buffer_write (buf, &data, sizeof (gchar));
}

static void
buffer_write_gint32 (Buffer *buf, gint32 data) 
{
	gint32 w_data;

	g_return_if_fail (buf != NULL);
	g_return_if_fail (buf->base != NULL);

	DEBUG4 (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Writing word %d to %p", data, buf));

	w_data = GINT32_TO_BE (data);
	buffer_write (buf, &w_data, sizeof (gint32));
}

static void
buffer_write_gint64 (Buffer *buf, gint64 data) 
{
	gint64 w_data;

	g_return_if_fail (buf != NULL);
	g_return_if_fail (buf->base != NULL);

	DEBUG4 (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Writing doubleword %lld to %p", data, buf));

	w_data = GINT64_TO_BE (data);
	buffer_write (buf, &w_data, sizeof (gint64));
}

static void
buffer_write_block (Buffer *buf, gconstpointer ptr, gint32 len) 
{
	g_return_if_fail (buf != NULL);
	g_return_if_fail (buf->base != NULL);

	DEBUG4 (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Writing block of length %d to %p", len, buf));

	buffer_write_gint32 (buf, len);
	buffer_write (buf, ptr, len);
}

static void
buffer_write_string (Buffer *buf, const gchar *data) 
{
	gint32 len;

	g_return_if_fail (buf != NULL);
	g_return_if_fail (buf->base != NULL);

	len = strlen (data);

	DEBUG4 (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Writing string of length %d to %p: %s",
		       len, buf, data));

	buffer_write_block (buf, data, len);
}

/* Derived from OpenSSH, sftp-client.c:encode_stat */

static void
buffer_write_file_info (Buffer *buf, const MateVFSFileInfo *info, MateVFSSetFileInfoMask mask)
{
	guint flags = 0;

	g_return_if_fail (buf != NULL);
	g_return_if_fail (buf->base != NULL);

	DEBUG4 (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Writing file info to %p", buf));

	if (mask & MATE_VFS_SET_FILE_INFO_PERMISSIONS)
		flags |= SSH2_FILEXFER_ATTR_PERMISSIONS;
	if (mask & MATE_VFS_SET_FILE_INFO_TIME)
		flags |= SSH2_FILEXFER_ATTR_ACMODTIME;

	buffer_write_gint32 (buf, flags);

	if (mask & MATE_VFS_SET_FILE_INFO_PERMISSIONS)
		buffer_write_gint32 (buf, info->permissions & 0777);
	if (mask & MATE_VFS_SET_FILE_INFO_TIME) {
		buffer_write_gint32 (buf, info->atime);
		buffer_write_gint32 (buf, info->mtime);
	}
}

static MateVFSResult 
sftp_status_to_vfs_result (guint status) 
{
	switch (status) {
	    case SSH2_FX_OK:
		return MATE_VFS_OK;
	    case SSH2_FX_EOF:
		return MATE_VFS_ERROR_EOF;
	    case SSH2_FX_NO_SUCH_FILE:
		return MATE_VFS_ERROR_NOT_FOUND;
	    case SSH2_FX_PERMISSION_DENIED:
		return MATE_VFS_ERROR_NOT_PERMITTED;
	    case SSH2_FX_NO_CONNECTION:
		return MATE_VFS_ERROR_LOGIN_FAILED;
	    case SSH2_FX_FAILURE:
		return MATE_VFS_ERROR_GENERIC;
	    case SSH2_FX_BAD_MESSAGE:
		return MATE_VFS_ERROR_INTERNAL;
	    case SSH2_FX_CONNECTION_LOST:
		return MATE_VFS_ERROR_IO;
	    case SSH2_FX_OP_UNSUPPORTED:
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	    default:
		return MATE_VFS_ERROR_GENERIC;
	}
}


/* Derived from OpenSSH, sftp-client.c:get_status */

static MateVFSResult
iobuf_read_result (int fd, guint expected_id)
{
	Buffer msg;
	guint type, id, status;

	buffer_init (&msg);
	buffer_recv (&msg, fd);
	type = buffer_read_gchar (&msg);
	id = buffer_read_gint32 (&msg);

	if (id != expected_id)
		g_critical ("ID mismatch (%u != %u)", id, expected_id);
	if (type != SSH2_FXP_STATUS)
		g_critical ("Expected SSH2_FXP_STATUS(%u) packet, got %u", SSH2_FXP_STATUS, type);

	status = buffer_read_gint32 (&msg);
	buffer_free (&msg);

	DEBUG2 (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Result is %d", status));

	return sftp_status_to_vfs_result (status);
}

/* Derived from OpenSSH, sftp-client.c:get_handle */

static MateVFSResult
iobuf_read_handle (int fd, gchar **handle, guint expected_id, guint32 *len)
{
	Buffer msg;
	gchar type;
	guint id, status;

	buffer_init (&msg);
	buffer_recv (&msg, fd);

	type = buffer_read_gchar (&msg);
	id = buffer_read_gint32 (&msg);

	if (id != expected_id)
		g_critical ("ID mismatch (%u != %u)", id, expected_id);
	if (type == SSH2_FXP_STATUS) {
		*handle = NULL;
		status = buffer_read_gint32 (&msg);
		buffer_free (&msg);
		return sftp_status_to_vfs_result (status);
	} else if (type != SSH2_FXP_HANDLE)
		g_critical ("Expected SSH2_FXP_HANDLE(%u) packet, got %u",
			    SSH2_FXP_HANDLE, type);

	*handle = buffer_read_block (&msg, (gint32 *)len);

	buffer_free (&msg);

	DEBUG2 (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Handle is %s", *handle));

	return MATE_VFS_OK;
}

/* Derived from OpenSSH, sftp-client.c:get_decode_stat */

/* this neither includes the name nor the MIME type,
 * which are set in update_mime_type_and_name_from_path */
static MateVFSResult
iobuf_read_file_info (int fd, MateVFSFileInfo *info, guint expected_id)
{
	Buffer msg;
	gchar type;
	guint id, status;
	MateVFSResult res;

	buffer_init (&msg);
	buffer_recv (&msg, fd);

	type = buffer_read_gchar (&msg);
	id = buffer_read_gint32 (&msg);

	if (id != expected_id) {
		g_warning ("ID mismatch (%u != %u)", id, expected_id);
		res = MATE_VFS_ERROR_PROTOCOL_ERROR;
	} else if (type == SSH2_FXP_STATUS) {
		status = buffer_read_gint32 (&msg);
		DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
			      "%s: Reading file info failed with SSH2_FXP_STATUS(%u), status %u",
			      G_STRFUNC, type, status));
		res = sftp_status_to_vfs_result (status);
	} else if (type == SSH2_FXP_ATTRS) {
		buffer_read_file_info (&msg, info);
		res = MATE_VFS_OK;
	} else {
		g_warning ("Expected SSH2_FXP_STATUS(%u) or SSH2_FXP_ATTRS(%u) packet, got %u",
			   SSH2_FXP_STATUS, SSH2_FXP_ATTRS, type);
		res = MATE_VFS_ERROR_PROTOCOL_ERROR;
	}

	buffer_free (&msg);

	return res;
}

/* Derived from OpenSSH, sftp-client.c:send_read_request */

static MateVFSResult
iobuf_send_read_request (int            fd,
			 guint          id,
			 guint64        offset,
			 guint          len,
			 const char    *handle,
			 guint          handle_len)
{
	Buffer msg;
	MateVFSResult res;

	buffer_init (&msg);

	buffer_write_gchar (&msg, SSH2_FXP_READ);
	buffer_write_gint32 (&msg, id);
	buffer_write_block (&msg, handle, handle_len);
	buffer_write_gint64 (&msg, offset);
	buffer_write_gint32 (&msg, len);
	res = buffer_send (&msg, fd);

	buffer_free (&msg);

	return res;
}

/* Derived from OpenSSH, sftp-client.c:send_string_request */

static void
iobuf_send_string_request (int            fd,
			   guint          id,
			   guint          code,
			   const char    *s,
			   guint          len)
{
	Buffer msg;

	buffer_init (&msg);

	buffer_write_gchar (&msg, code);
	buffer_write_gint32 (&msg, id);
	buffer_write_block (&msg, s, len);
	buffer_send (&msg, fd);

	buffer_free (&msg);
}

/* Derived from OpenSSH, sftp-client.c:send_string_attrs_request */

static void
iobuf_send_string_request_with_file_info (int                      fd,
					  guint                    id,
					  guint                    code,
					  const char              *s,
					  guint                    len,
					  const MateVFSFileInfo  *info,
					  MateVFSSetFileInfoMask  mask)
{
	Buffer msg;

	buffer_init (&msg);

	buffer_write_gchar (&msg, code);
	buffer_write_gint32 (&msg, id);
	buffer_write_block (&msg, s, len);
	buffer_write_file_info (&msg, info, mask);
	buffer_send (&msg, fd);

	buffer_free (&msg);
}

static char*
get_user_from_string_or_password_line (const char *user_string,
				       const char *password_line)
{
	char *chr, *user = NULL;

	if (!g_str_has_prefix (password_line, "Enter passphrase for key")) {
		chr = strchr (password_line, '@');

		if (chr != NULL) {
			user = g_strndup (password_line, chr - password_line);
		}
	}
	if (user == NULL) {
		user = g_strdup (user_string);
	}
	return user;
}

static char*
get_object_from_password_line (const char *password_line)
{
	char *chr, *ptr, *object = NULL;

	if (g_str_has_prefix (password_line, "Enter passphrase for key")) {
		ptr = strchr (password_line, '\'');
		if (ptr != NULL) {
			ptr += 1;
			chr = strchr (ptr, '\'');
			if (chr != NULL) {
				object = g_strndup (ptr, chr-ptr);
			} else {
				object = g_strdup (ptr);
			}
		}
	}
	return object;
}

static char*
get_server_from_uri_or_password_line (const MateVFSURI *uri,
				      const char *password_line)
{
	if (!g_str_has_prefix (password_line, "Enter passphrase for key")) {
		return g_strdup ( (char *)mate_vfs_uri_get_host_name (uri));
	}
	return NULL;
}

static char*
get_authtype_from_password_line (const char *password_line)
{
	if (g_str_has_prefix (password_line, "Enter passphrase for key")) {
		return g_strdup ("publickey");
	} 
	return g_strdup ("password");
}

static gboolean
invoke_fill_auth (const MateVFSURI *uri,
		  const char *password_line,
		  char **password_out)
{
	gboolean invoked;
	MateVFSModuleCallbackFillAuthenticationIn in_args;
	MateVFSModuleCallbackFillAuthenticationOut out_args;


	memset (&in_args, 0, sizeof (in_args));
	in_args.protocol = "sftp";
	
	in_args.uri = mate_vfs_uri_to_string (uri, 0);
	in_args.object = get_object_from_password_line (password_line);
	in_args.authtype = get_authtype_from_password_line (password_line);
	in_args.domain = NULL;
	in_args.port = mate_vfs_uri_get_host_port (uri);
	in_args.server = get_server_from_uri_or_password_line (uri, password_line);
	in_args.username = get_user_from_string_or_password_line (mate_vfs_uri_get_user_name (uri), password_line);
	memset (&out_args, 0, sizeof (out_args));

	invoked = mate_vfs_module_callback_invoke
		  (MATE_VFS_MODULE_CALLBACK_FILL_AUTHENTICATION,
		  &in_args, sizeof (in_args),
		  &out_args, sizeof (out_args));
	if (invoked && out_args.valid) {
		*password_out = g_strdup (out_args.password);
		g_free (out_args.username);
		g_free (out_args.domain);
		g_free (out_args.password);
	} else {
		*password_out = NULL;
	}

	g_free (in_args.uri);
	g_free (in_args.username);
	g_free (in_args.object);
	g_free (in_args.server);
	g_free (in_args.authtype);

	return invoked && out_args.valid;
}

static gboolean
invoke_full_auth (const MateVFSURI *uri,
		  gboolean done_auth,
		  const char *password_line,
		  const char *user_name,
		  char **password_out,
		  char **keyring_out,
		  char **user_out,
		  char **object_out,
		  char **authtype_out,
		  gboolean *save_password_out,
		  gboolean *abort_auth_out)
{
	MateVFSModuleCallbackFullAuthenticationIn in_args;
	MateVFSModuleCallbackFullAuthenticationOut out_args;
	gboolean invoked;

	memset (&in_args, 0, sizeof (in_args));
	in_args.uri = mate_vfs_uri_to_string (uri, 0);
	in_args.flags = MATE_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_NEED_PASSWORD |
			MATE_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_SAVING_SUPPORTED;
	in_args.default_user = get_user_from_string_or_password_line (user_name, password_line);
	if (mate_vfs_uri_get_user_name (uri) == NULL) {
		in_args.flags |= MATE_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_NEED_USERNAME;
	} else {
		in_args.username = g_strdup (in_args.default_user);
	}
	if (done_auth) {
		in_args.flags |= MATE_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION_PREVIOUS_ATTEMPT_FAILED;
	}
	in_args.protocol = "sftp";
	in_args.object = get_object_from_password_line (password_line);
	in_args.authtype = get_authtype_from_password_line (password_line);
	in_args.domain = NULL;
	in_args.port = mate_vfs_uri_get_host_port (uri);
	in_args.server = get_server_from_uri_or_password_line (uri, password_line);

	memset (&out_args, 0, sizeof (out_args));

	invoked = mate_vfs_module_callback_invoke
		(MATE_VFS_MODULE_CALLBACK_FULL_AUTHENTICATION,
		 &in_args, sizeof (in_args),
		 &out_args, sizeof (out_args));
	if (invoked && !out_args.abort_auth) {
		if (out_args.save_password) {
			*keyring_out = g_strdup (out_args.keyring);
			*object_out = get_object_from_password_line (password_line);
			*authtype_out = get_authtype_from_password_line (password_line);
		}
		*user_out = g_strdup (out_args.username);
		*password_out = g_strdup (out_args.password);
		*save_password_out = out_args.save_password;
		g_free (out_args.username);
		g_free (out_args.domain);
		g_free (out_args.password);
		g_free (out_args.keyring);
	} else {
		*user_out = NULL;
		*password_out = NULL;
	}

	*abort_auth_out = out_args.abort_auth;

	g_free (in_args.uri);
	g_free (in_args.username);
	g_free (in_args.default_user);
	g_free (in_args.object);
	g_free (in_args.server);
	g_free (in_args.authtype);

	return invoked && !out_args.abort_auth;
}

static void
invoke_save_auth (const MateVFSURI *uri,
	   char *keyring,
	   char *user,
	   char *object,
	   char *authtype,
	   char *password)
{
	MateVFSModuleCallbackSaveAuthenticationIn save_in_args;
	MateVFSModuleCallbackSaveAuthenticationOut save_out_args;

	memset (&save_in_args, 0, sizeof (save_in_args));
	save_in_args.uri = mate_vfs_uri_to_string (uri, 0);
	save_in_args.server = (char *)mate_vfs_uri_get_host_name (uri);
	save_in_args.port = mate_vfs_uri_get_host_port (uri);
	save_in_args.protocol = "sftp";
	save_in_args.keyring = keyring;
	save_in_args.username = user;
	save_in_args.object = object;
	save_in_args.authtype = authtype;
	save_in_args.password = password;

	memset (&save_out_args, 0, sizeof (save_out_args));
	mate_vfs_module_callback_invoke (MATE_VFS_MODULE_CALLBACK_SAVE_AUTHENTICATION,
					  &save_in_args, sizeof (save_in_args),
					  &save_out_args, sizeof (save_out_args));
	g_free (save_in_args.uri);
}
	

typedef enum {
	SFTP_VENDOR_INVALID = 0,
	SFTP_VENDOR_OPENSSH,
	SFTP_VENDOR_SSH
} SFTPClientVendor;

static SFTPClientVendor
get_sftp_client_vendor (void)
{
	char *ssh_stderr;
	char *args[3];
	gint ssh_exitcode;
	SFTPClientVendor res = SFTP_VENDOR_INVALID;
	
	args[0] = g_strdup (SSH_PROGRAM);
	args[1] = g_strdup ("-V");
	args[2] = NULL;
	if (g_spawn_sync (NULL, args, NULL,
			  G_SPAWN_SEARCH_PATH | G_SPAWN_STDOUT_TO_DEV_NULL,
			  NULL, NULL,
			  NULL, &ssh_stderr,
			  &ssh_exitcode, NULL)) {
		if (ssh_stderr == NULL)
			res = SFTP_VENDOR_INVALID;
		else if ((strstr (ssh_stderr, "OpenSSH") != NULL) || (strstr (ssh_stderr, "Sun_SSH") != NULL))
			res = SFTP_VENDOR_OPENSSH;
		else if (strstr (ssh_stderr, "SSH Secure Shell") != NULL)
			res = SFTP_VENDOR_SSH;
		else
			res = SFTP_VENDOR_INVALID;
	}
	g_free (ssh_stderr);
	g_free (args[0]);
	g_free (args[1]);

	return res;
}

/* Derived from OpenSSH, sftp.c:main */

static MateVFSResult
sftp_connect (SftpConnection **connection, const MateVFSURI *uri) 
{
	MateVFSResult  res;
	GIOChannel     *error_channel = NULL;
	GIOChannel     *tty_channel = NULL;
	int             in_fd, out_fd, err_fd, tty_fd;
	pid_t           ssh_pid;
	gint            port;
	guint           last_arg, i;
	gboolean        full_auth;
	gboolean        done_auth;
	gboolean	save_password = FALSE;
	Buffer          msg;
	gchar           type;
	char *user_name = NULL;
	char *password = NULL;
	char *keyring  = NULL;
	char *object   = NULL;
	char *authtype = NULL;

	GError         *error = NULL;

	gchar          *args[20]; /* Enough for now, extend if you add more args */
	gboolean invoked; 		  
	MateVFSModuleCallbackQuestionIn in_args; 
	MateVFSModuleCallbackQuestionOut out_args;

	SFTPClientVendor client_vendor;
	
	DEBUG (gchar *tmp);

	client_vendor = get_sftp_client_vendor ();

	done_auth = FALSE;
	full_auth = FALSE;

	user_name = g_strdup (mate_vfs_uri_get_user_name (uri));
	if (user_name == NULL) {
		user_name = g_strdup (g_get_user_name ());
	}

	password = g_strdup (mate_vfs_uri_get_password (uri));
	port = mate_vfs_uri_get_host_port (uri);

	/* required if the user decides to login with another user name.
	 * In this case, we'll have to re-spawn the SSH client */
tty_retry:

	/* Fill in the first few args */
	last_arg = 0;
	args[last_arg++] = g_strdup (SSH_PROGRAM);

	if (client_vendor == SFTP_VENDOR_OPENSSH) {
		args[last_arg++] = g_strdup ("-oForwardX11 no");
		args[last_arg++] = g_strdup ("-oForwardAgent no");
		args[last_arg++] = g_strdup ("-oClearAllForwardings yes");
		args[last_arg++] = g_strdup ("-oProtocol 2");
		args[last_arg++] = g_strdup ("-oNoHostAuthenticationForLocalhost yes");
#ifndef USE_PTY
		args[last_arg++] = g_strdup ("-oBatchMode yes");
#endif

	} else if (client_vendor == SFTP_VENDOR_SSH) {
		args[last_arg++] = g_strdup ("-x");
	} else {
		for (i = 0; i < last_arg; i++) {
			g_free (args[i]);
		}
		return MATE_VFS_ERROR_INTERNAL;
	}

	/* Make sure the last few arguments are clear */
	for (i = last_arg; i < sizeof (args) / sizeof (const gchar *); ++i)
		args[i] = NULL;

	if (port != 0) {
		args[last_arg++] = g_strdup ("-p");
		args[last_arg++] = g_strdup_printf ("%d", port);
	}

	if (user_name != NULL) {
		args[last_arg++] = g_strdup ("-l");
		args[last_arg++] = g_strdup (user_name);
	}

	args[last_arg++] = g_strdup ("-s");

	if (client_vendor == SFTP_VENDOR_SSH) {
		args[last_arg++] = g_strdup ("sftp");
		args[last_arg++] = g_strdup (mate_vfs_uri_get_host_name (uri));
	} else {
		args[last_arg++] = g_strdup (mate_vfs_uri_get_host_name (uri));
		args[last_arg++] = g_strdup ("sftp");
	}

	args[last_arg++] = NULL;

	DEBUG (tmp = g_strjoinv (" ", args));
	DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Command line is %s", G_STRFUNC, tmp));
	DEBUG (g_free (tmp));

	tty_fd = -1;
#ifdef USE_PTY
	tty_fd = mate_vfs_pty_open(&ssh_pid, MATE_VFS_PTY_REAP_CHILD, NULL,
				    args[0], args+1, NULL,
				    300, 300, 
				    &out_fd, &in_fd, &err_fd);
	if (tty_fd == -1) {
		*connection = NULL;
		for (i = 0; i < last_arg; i++) {
			g_free (args[i]);
		}
		g_free (user_name);
		return MATE_VFS_ERROR_INTERNAL;
	}
#else
	if (!g_spawn_async_with_pipes (NULL, args, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL,
				       &ssh_pid, &out_fd, &in_fd, &err_fd, &error))
	{
		g_critical ("Could not launch ssh: %s", error->message);
		*connection = NULL;
		for (i = 0; i < last_arg; i++) {
			g_free (args[i]);
		}
		g_free (user_name);
		return MATE_VFS_ERROR_INTERNAL;
	}
#endif

	error_channel = g_io_channel_unix_new (err_fd);
	g_io_channel_set_flags (error_channel,
				g_io_channel_get_flags (error_channel) | G_IO_FLAG_NONBLOCK, NULL);

	for (i = 0; i < last_arg; i++) {
		g_free (args[i]);
	}

	buffer_init (&msg);

	buffer_write_gchar (&msg, SSH2_FXP_INIT);
	buffer_write_gint32 (&msg, SSH2_FILEXFER_VERSION);
	buffer_send (&msg, out_fd);

	tty_channel = NULL;
	if (tty_fd != -1) {
		tty_channel = g_io_channel_unix_new (tty_fd);
		g_io_channel_set_encoding (tty_channel, NULL, NULL);
		g_io_channel_set_flags (tty_channel,
					g_io_channel_get_flags (tty_channel) | G_IO_FLAG_NONBLOCK, NULL);
	}
	while (tty_fd != -1) {
		fd_set ifds;
		struct timeval tv;
		int ret;
		int prompt_fd;
		GIOChannel *prompt_channel;
		GIOStatus io_status;
		char buffer[1024];
		gsize len;
		char *choices[3];
		char *pos;
		char *startpos;
		char *endpos;
		char *hostname = NULL;
		char *fingerprint = NULL;
		gboolean aborted = FALSE;

		if (client_vendor == SFTP_VENDOR_SSH) {
			prompt_fd = err_fd;
			prompt_channel = error_channel;
		} else {
			prompt_fd = tty_fd;
			prompt_channel = tty_channel;
		}
		
		FD_ZERO (&ifds);
		FD_SET (in_fd, &ifds);
		FD_SET (prompt_fd, &ifds);

		tv.tv_sec = 20;
		tv.tv_usec = 0;

		ret = select (MAX (in_fd, prompt_fd)+1, &ifds, NULL, NULL, &tv);

		if (ret <= 0) {
			/* Timeout */
			res = MATE_VFS_ERROR_TIMEOUT;
			goto bail;
		}

		if (FD_ISSET (in_fd, &ifds)) {
			break;
		}

		g_assert (FD_ISSET (prompt_fd, &ifds));

		error = NULL;
		io_status = g_io_channel_read_chars (prompt_channel, buffer, sizeof(buffer)-1, &len, &error);
		if (io_status == G_IO_STATUS_NORMAL) {
			char *new_user_name = NULL;
			char *new_password = NULL;

			/*
			 * If the input URI contains a username
			 *     if the input URI contains a password, we attempt one login and return MATE_VFS_ERROR_ACCESS_DENIED on failure.
			 *     if the input URI contains no password, we query the user until he provides a correct one, or he cancels.
			 *
			 * If the input URI contains no username
			 *     (a) the user is queried for a user name and a password, with the default login being his
			 *     local login name.
			 *
			 *     (b) if the user decides to change his remote login name, we go to tty_retry because we need a
			 *     new SSH session, attempting one login with his provided credentials, and if that fails proceed
			 *     with (a), but use his desired remote login name as default.
			 *
			 * The "password" variable is only used for the very first login attempt,
			 * or for the first re-login attempt when the user decided to change his name.
			 * Otherwise, we "new_password" and "new_user_name" is used, as output variable
			 * for user and keyring input.
			 */
			buffer[len] = 0;
			if (g_str_has_suffix (buffer, "password: ") ||
			    g_str_has_suffix (buffer, "Password: ") ||
			    g_str_has_suffix (buffer, "Password:")  ||
			    g_str_has_prefix (buffer, "Enter passphrase for key")) {
				if (!done_auth && password != NULL) {
					g_io_channel_write_chars (tty_channel, password, -1, &len, NULL);
					g_io_channel_write_chars (tty_channel, "\n", 1, &len, NULL);
					g_io_channel_flush (tty_channel, NULL);
				} else if (!done_auth && mate_vfs_uri_get_password (uri) == NULL &&
					   invoke_fill_auth (uri, buffer, &new_password) && new_password != NULL) {
					g_io_channel_write_chars (tty_channel, new_password, -1, &len, NULL);
					g_io_channel_write_chars (tty_channel, "\n", 1, &len, NULL);
					g_io_channel_flush (tty_channel, NULL);
					g_free (password);
					password = new_password;
				} else if (mate_vfs_uri_get_password (uri) == NULL &&
					   invoke_full_auth (uri, done_auth, buffer, user_name, &new_password, &keyring,
						   	     &new_user_name, &object, &authtype, &save_password, &aborted) && new_password != NULL) {
					if (new_user_name == NULL) {
						new_user_name = g_strdup (g_get_user_name ());
					}

					g_assert (user_name != NULL);
					g_assert (new_user_name != NULL);

					/* if the user changed the login name, we have to re-invoke SSH */
					if (strcmp (user_name, new_user_name) != 0) {
						buffer_free (&msg);

						if (error_channel != NULL) {
							g_io_channel_unref (error_channel);
						}
						if (tty_channel != NULL) {
							g_io_channel_unref (tty_channel);
						}

						g_free (user_name);
						user_name = new_user_name;

						g_free (password);
						password = new_password;

						done_auth = FALSE;

						goto tty_retry;
					}

					full_auth = TRUE;
					g_io_channel_write_chars (tty_channel, new_password, -1, &len, NULL);
					g_io_channel_write_chars (tty_channel, "\n", 1, &len, NULL);
					g_io_channel_flush (tty_channel, NULL);

					g_free (new_user_name);
					g_free (password);
					password = new_password;
				} else if (aborted) {
					res = MATE_VFS_ERROR_CANCELLED;
					goto bail;
				} else {
					res = MATE_VFS_ERROR_ACCESS_DENIED;
					goto bail;
				}
				done_auth = TRUE;
			} else if (g_str_has_prefix (buffer, "The authenticity of host '") ||
				   strstr (buffer, "Key fingerprint:") != NULL) {

				if (g_str_has_prefix (buffer, "The authenticity of host '")) {
					/* OpenSSH */
					pos = strchr (&buffer[26], '\'');
					if (pos == NULL) {
						res = MATE_VFS_ERROR_GENERIC;
						goto bail;
					}


					hostname = g_strndup (&buffer[26], pos - (&buffer[26]));

					startpos = strstr (pos, " key fingerprint is ");
					if (startpos == NULL) {
						res = MATE_VFS_ERROR_GENERIC;
						g_free (hostname);
						goto bail;
					}

					startpos = startpos + 20;
					endpos = strchr (startpos, '.');
					if (endpos == NULL) {
						res = MATE_VFS_ERROR_GENERIC;
						g_free (hostname);
						goto bail;
					}
					fingerprint = g_strndup (startpos, endpos - startpos);
				} else if (strstr (buffer, "Key fingerprint:") != NULL) {
					/* SSH.com*/
					hostname = g_strdup (mate_vfs_uri_get_host_name (uri));
					startpos = strstr (buffer, "Key fingerprint:");
					if (startpos == NULL) {
						g_free (hostname);
						res = MATE_VFS_ERROR_GENERIC;
						goto bail;
					}
					startpos = startpos + 18;
					endpos = strchr (startpos, '\r');
					fingerprint = g_strndup (startpos, endpos - startpos);
				} else {
					res = MATE_VFS_ERROR_GENERIC;
					goto bail;	
				}
				
				in_args.primary_message = g_strdup_printf (_("The identity of the remote computer (%s) is unknown."), hostname);
				in_args.secondary_message = g_strdup_printf (_("This happens when you log in to a computer the first time.\n\n"
									       "The identity sent by the remote computer is %s. "
									       "If you want to be absolutely sure it is safe to continue, contact the system administrator."), fingerprint);

				g_free (hostname);
				g_free (fingerprint);

				in_args.choices = choices;
				in_args.choices[0] = (char *) _("Log In Anyway");
				in_args.choices[1] = (char *) _("Cancel Login");
				in_args.choices[2] = NULL;
								
				invoked = mate_vfs_module_callback_invoke
					(MATE_VFS_MODULE_CALLBACK_QUESTION,
					 &in_args, sizeof (in_args),
					 &out_args, sizeof (out_args));
				
				if (invoked) {
					if (out_args.answer == 0) {
						g_io_channel_write_chars (tty_channel, "yes\n", -1, &len, NULL);
					} else {
						g_io_channel_write_chars (tty_channel, "no\n", -1, &len, NULL);
						g_free (in_args.primary_message);
						g_free (in_args.secondary_message);
						res = MATE_VFS_ERROR_ACCESS_DENIED;
						goto bail;
					}
					g_io_channel_flush (tty_channel, NULL);
					buffer[0]='\0';
				} else {
					g_io_channel_write_chars (tty_channel, "no\n", -1, &len, NULL);
					g_io_channel_flush (tty_channel, NULL);
					buffer[0]='\0';
					g_free (in_args.primary_message);
					g_free (in_args.secondary_message);
					res = MATE_VFS_ERROR_ACCESS_DENIED;
					goto bail;
				}
				g_free (in_args.primary_message);
				g_free (in_args.secondary_message);
			}
			
		}
	}

	if ((res = buffer_recv (&msg, in_fd)) != MATE_VFS_OK) {
		/* Could not read the response; check the error stream for ssh errors */

		sftp_connection_process_errors (error_channel, G_IO_IN, &res);

		/* If no error came over the pipe, then this looks like a generic I/O error */
		if (res == MATE_VFS_OK)
			res = MATE_VFS_ERROR_IO;
	}
	else if ((type = buffer_read_gchar (&msg)) != SSH2_FXP_VERSION) {
		/* Response given was not correct. Give up with a protocol error */

		DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
			      "%s: Protocol error: message type is %d", G_STRFUNC, type));
		res = MATE_VFS_ERROR_PROTOCOL_ERROR;
	} else {
		/* Everything's A-OK. Set up the connection and go */
		if (full_auth == TRUE && save_password == TRUE) {
			invoke_save_auth (uri, keyring, user_name, object, authtype, password);
		}

		if (!g_thread_supported ()) g_thread_init (NULL);

		*connection = g_new0 (SftpConnection, 1);
		(*connection)->ref_count = 1;
		(*connection)->in_fd = in_fd;
		(*connection)->out_fd = out_fd;
		(*connection)->tty_fd = tty_fd;
		(*connection)->error_channel = error_channel;
		g_io_channel_ref (error_channel);
		(*connection)->ssh_pid = ssh_pid;
		(*connection)->version = buffer_read_gint32 (&msg);
		(*connection)->mutex = g_mutex_new ();
		(*connection)->msg_id = 1;
		(*connection)->status = MATE_VFS_OK;
		(*connection)->event_id = g_io_add_watch ((*connection)->error_channel, G_IO_IN,
							  (GIOFunc)
							  sftp_connection_process_errors,
							  &(*connection)->status);

		DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Version is %d",
			      (*connection)->version));
	}

 bail:
	buffer_free (&msg);

	g_free (password);
	g_free (keyring);
	g_free (user_name);
	g_free (object);
	g_free (authtype);
	if (error_channel != NULL) {
		g_io_channel_unref (error_channel);
	}
	if (tty_channel != NULL) {
		g_io_channel_unref (tty_channel);
	}
	if (res != MATE_VFS_OK) {
		close (in_fd);
		close (out_fd);
		close (err_fd);
		if (tty_fd != -1)
			close (tty_fd);
		*connection = NULL;

		/* TODO: Do we leak error_channel and connection? */
	}

	return res;
}

static MateVFSResult
sftp_get_connection (SftpConnection **connection, const MateVFSURI *uri) 
{
	gchar *hash_name;
	MateVFSResult res;
	const gchar *user_name;
	const gchar *host_name;

	g_return_val_if_fail (connection != NULL, MATE_VFS_ERROR_INTERNAL);
	g_return_val_if_fail (uri != NULL, MATE_VFS_ERROR_INTERNAL);

	G_LOCK (sftp_connection_table);

	if (sftp_connection_table == NULL)
		sftp_connection_table = g_hash_table_new
			(g_str_hash, g_str_equal);

	user_name = mate_vfs_uri_get_user_name (uri);
	host_name = mate_vfs_uri_get_host_name (uri);

	if (host_name == NULL)
	{
		res = MATE_VFS_ERROR_HOST_NOT_FOUND;
		goto bail;
	}

	if (user_name != NULL)
		hash_name = g_strconcat (user_name, "@", host_name, NULL);
	else
		hash_name = g_strdup (host_name);

	DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
		      "%s: Getting connection to %s", G_STRFUNC, hash_name));

	*connection = g_hash_table_lookup (sftp_connection_table, hash_name);

	if (*connection == NULL) {
		DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
			      "%s: Connection not found. Opening new one.", G_STRFUNC));

		res = sftp_connect (connection, uri);

		if (res == MATE_VFS_OK) {
			if (*connection == NULL)
			{
				res = MATE_VFS_ERROR_INTERNAL;
				g_free (hash_name);
				goto bail;
			}
			g_mutex_lock ((*connection)->mutex);
			(*connection)->hash_name = hash_name;
			g_hash_table_insert (sftp_connection_table, hash_name, *connection);
		} else {
			g_free (hash_name);
		}
	}
#if 0
	else if (!g_mutex_trylock ((*connection)->mutex)) {
		DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
			      "%s: Connection found but locked. Opening new one.", G_STRFUNC));

		res = sftp_connect (connection, uri);
		g_mutex_lock ((*connection)->mutex);

		// Don't insert this one in the table; it'll go away
		// when done
	}
#endif
	else {
		DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Connection found. Locking",
			      G_STRFUNC));

		g_mutex_lock ((*connection)->mutex);
		sftp_connection_ref ((*connection));

		DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Lock acquired",
			      G_STRFUNC));

		g_free (hash_name);
		res = MATE_VFS_OK;
	}

 bail:
	G_UNLOCK (sftp_connection_table);

	return res;
}

static gboolean
sftp_connection_close (SftpConnection *conn) 
{
	DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Closing connection %p.", conn));

	close (conn->in_fd);
	close (conn->out_fd);
	if (conn->tty_fd != -1)
		close (conn->tty_fd);
	g_source_remove (conn->event_id);
	g_io_channel_shutdown (conn->error_channel, FALSE, NULL);
	g_io_channel_unref (conn->error_channel);

	g_free (conn->hash_name);
	g_free (conn);

	return TRUE;
}

static gboolean
sftp_connection_process_errors (GIOChannel *channel, GIOCondition cond, MateVFSResult *status) 
{
	gchar *str, *str1;
	GError *error = NULL;
	GIOStatus io_status;

	g_return_val_if_fail (status != NULL, FALSE);

	if (cond != G_IO_IN) return TRUE;

	DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Enter", G_STRFUNC));

	io_status = g_io_channel_read_line (channel, &str, NULL, NULL, &error);

	switch (io_status) {
	    case G_IO_STATUS_ERROR:
		DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
			      "%s: Could not read error message: %s", G_STRFUNC, error->message));
		*status = MATE_VFS_ERROR_IO;
		break;

	    case G_IO_STATUS_EOF:
		DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
			      "%s: Subprocess closed the connection", G_STRFUNC));
		*status = MATE_VFS_ERROR_EOF;
		return FALSE;

	    case G_IO_STATUS_AGAIN:
		DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: No error", G_STRFUNC));
		*status = MATE_VFS_OK;
		break;

	    case G_IO_STATUS_NORMAL:
		/* Keep reading error messages until no more are available */
		while (io_status == G_IO_STATUS_NORMAL) {
			io_status = g_io_channel_read_line (channel, &str1, NULL, NULL, &error);

			if (io_status == G_IO_STATUS_NORMAL) {
				g_free (str);
				str = str1;

				DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
					      "%s: Error message was %s", G_STRFUNC, str));
			}
		}

		if (strstr (str, "Permission denied") != NULL)
			*status = MATE_VFS_ERROR_LOGIN_FAILED;
		else if (strstr (str, "Name or service not known") != NULL)
			*status = MATE_VFS_ERROR_HOST_NOT_FOUND;
		else if (strstr (str, "Connection refused") != NULL)
			*status = MATE_VFS_ERROR_ACCESS_DENIED;
		else if (strstr (str, "No route to host") != NULL)
			*status = MATE_VFS_ERROR_HOST_NOT_FOUND;
		else if (strstr (str, "Host key verification failed") != NULL) {
			*status = MATE_VFS_ERROR_SERVICE_NOT_AVAILABLE;
		}
		break;
	}

	g_free (str);

	return TRUE;
}

static gint
sftp_connection_get_id (SftpConnection *conn) 
{
	gint id;

	g_return_val_if_fail (conn != NULL, 0);

	id = conn->msg_id++;
	DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Message id %d", G_STRFUNC, id));

	return id;
}

static void
sftp_connection_lock (SftpConnection *conn) 
{
	g_mutex_lock (conn->mutex);
}

static void
sftp_connection_unlock (SftpConnection *conn) 
{
	g_mutex_unlock (conn->mutex);
}

/* caller must have a lock on the connection */
static void
sftp_connection_ref (SftpConnection *conn) 
{
	++conn->ref_count;

	if (conn->close_timeout_id > 0) {
		g_source_remove (conn->close_timeout_id);
		conn->close_timeout_id = 0;
	}
}

static gboolean
close_and_remove_connection (SftpConnection *conn)
{
	sftp_connection_lock (conn);

	conn->close_timeout_id = 0;

	if (conn->ref_count != 0) {
		sftp_connection_unlock (conn);
		return FALSE;
	}

	G_LOCK (sftp_connection_table);
	g_hash_table_remove (sftp_connection_table, conn->hash_name);	
	G_UNLOCK (sftp_connection_table);

	sftp_connection_unlock (conn);

	sftp_connection_close (conn);

	return FALSE;
}

/* caller must have a lock on the connection */
static void
sftp_connection_unref (SftpConnection *conn) 
{
	if (--conn->ref_count == 0 && conn->close_timeout_id == 0) {
		conn->close_timeout_id
			= g_timeout_add (SFTP_CLOSE_TIMEOUT, (GSourceFunc) close_and_remove_connection, conn);
	}
}

/* Portions of the below functions inspired by functions in OpenSSH sftp-client.c */

#if 0

static MateVFSResult
get_real_path (SftpConnection *conn, const gchar *path, gchar **realpath)
{
	Buffer msg;
	guint type, recv_id, count, id;
	guint status;
	MateVFSResult res;

	id = sftp_connection_get_id (conn);

	iobuf_send_string_request (conn->out_fd, id, SSH2_FXP_REALPATH, path, strlen (path));

	buffer_init (&msg);

	res = buffer_recv (&msg, conn->in_fd);

	if (res != MATE_VFS_OK) {
		g_critical ("Error receiving message: %d", res);
		buffer_free (&msg);
		return res;
	}

	type = buffer_read_gchar (&msg);
	recv_id = buffer_read_gint32 (&msg);

	if (type == SSH2_FXP_STATUS) {
		status = buffer_read_gint32 (&msg);
		buffer_free (&msg);
		*realpath = NULL;
		DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Bad status (%d).",
			      G_STRFUNC, status));
		return sftp_status_to_vfs_result (status);
	}
	else if (recv_id != id || type != SSH2_FXP_NAME) {
		DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Bad message id or type (%d, %d).",
			      G_STRFUNC, recv_id, type));
		buffer_free (&msg);
		return MATE_VFS_ERROR_PROTOCOL_ERROR;
	}

	count = buffer_read_gint32 (&msg);

	if (count == 0) {
		DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: File not found: %s",
			      G_STRFUNC, path));
		buffer_free (&msg);
		return MATE_VFS_ERROR_NOT_FOUND;
	}
	else if (count != 1) {
		DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Bad count (%d).",
			      G_STRFUNC, count));
		buffer_free (&msg);
		return MATE_VFS_ERROR_PROTOCOL_ERROR;
	}

	*realpath = buffer_read_string (&msg, NULL);

	buffer_free (&msg);

	DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Real path is %s", *realpath));

	return MATE_VFS_OK;
}

#endif /* 0 */

static MateVFSResult 
do_open (MateVFSMethod        *method,
	 MateVFSMethodHandle **method_handle,
	 MateVFSURI           *uri,
	 MateVFSOpenMode       mode,
	 MateVFSContext       *context) 
{
	SftpConnection *conn;
	SftpOpenHandle *handle;
	MateVFSResult res;
	MateVFSFileInfo info;

	Buffer msg;
	guint id;
	gint32 sftp_mode;
	gchar *sftp_handle;
	gint sftp_handle_len;
	gchar *path;

	DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Enter", G_STRFUNC));

	res = sftp_get_connection (&conn, uri);
	if (res != MATE_VFS_OK) return res;

	URI_TO_PATH (uri, path);

	id = sftp_connection_get_id (conn);

	buffer_init (&msg);
	buffer_write_gchar (&msg, SSH2_FXP_OPEN);
	buffer_write_gint32 (&msg, id);
	buffer_write_string (&msg, path);

	sftp_mode = 0;
	if (mode & MATE_VFS_OPEN_READ) sftp_mode |= SSH2_FXF_READ;
	if (mode & MATE_VFS_OPEN_WRITE) sftp_mode |= SSH2_FXF_WRITE;
	if ((mode & MATE_VFS_OPEN_TRUNCATE) ||
	    ((mode & MATE_VFS_OPEN_WRITE) && !(mode & MATE_VFS_OPEN_RANDOM)))
		sftp_mode |= SSH2_FXF_TRUNC;

	buffer_write_gint32 (&msg, sftp_mode);

	memset (&info, 0, sizeof (MateVFSFileInfo));
	buffer_write_file_info (&msg, &info, MATE_VFS_SET_FILE_INFO_NONE);

	buffer_send (&msg, conn->out_fd);
	buffer_free (&msg);
	
	res = iobuf_read_handle (conn->in_fd, &sftp_handle, id, (guint32 *)&sftp_handle_len);

	if (res == MATE_VFS_OK) {
		handle = g_new0 (SftpOpenHandle, 1);
		handle->sftp_handle = sftp_handle;
		handle->sftp_handle_len = sftp_handle_len;
		handle->path = path;
		handle->connection = conn;
		*method_handle = (MateVFSMethodHandle *) handle;

		sftp_connection_unlock (conn);

		DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Exit", G_STRFUNC));
		return MATE_VFS_OK;
	} else {
		*method_handle = NULL;

		g_free (path);

		sftp_connection_unref (conn);
		sftp_connection_unlock (conn);

		DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Exit", G_STRFUNC));
		return res;
	}
}

static MateVFSResult
do_create (MateVFSMethod        *method,
	   MateVFSMethodHandle **method_handle,
	   MateVFSURI           *uri,
	   MateVFSOpenMode       mode,
	   gboolean               exclusive,
	   guint                  perm,
	   MateVFSContext       *context)
{
	SftpConnection *conn;
	SftpOpenHandle *handle;
	MateVFSResult res;
	MateVFSFileInfo info;

	Buffer msg;
	int id;
	int ssh_mode;

	gchar *sftp_handle;
	guint sftp_handle_len;
	gchar *path;

	DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Enter", G_STRFUNC));

	res = sftp_get_connection (&conn, uri);
	if (res != MATE_VFS_OK) return res;

	URI_TO_PATH (uri, path);

	id = sftp_connection_get_id (conn);

	buffer_init (&msg);
	buffer_write_gchar (&msg, SSH2_FXP_OPEN);
	buffer_write_gint32 (&msg, id);
	buffer_write_string (&msg, path);

	ssh_mode = SSH2_FXF_CREAT;
	if (mode & MATE_VFS_OPEN_READ) ssh_mode |= SSH2_FXF_READ;
	if (mode & MATE_VFS_OPEN_WRITE) ssh_mode |= SSH2_FXF_WRITE;
	if (exclusive) {
		ssh_mode |= SSH2_FXF_EXCL;
	} else {
		/* It might be ok to unconditionnally add this truncation flag,
		 * but I'm not 100% sure that SSH2_FXF_EXCL takes precedence
		 * over SSH2_FXF_TRUNC, so better be safe than sorry ;)
		 */
		ssh_mode |= SSH2_FXF_TRUNC;
	}

	buffer_write_gint32 (&msg, ssh_mode);

	memset (&info, 0,  sizeof (MateVFSFileInfo));
	info.permissions = perm;
	buffer_write_file_info (&msg, &info, MATE_VFS_SET_FILE_INFO_PERMISSIONS);

	buffer_send (&msg, conn->out_fd);
	buffer_free (&msg);

	res = iobuf_read_handle (conn->in_fd, &sftp_handle, id, &sftp_handle_len);

	if (res == MATE_VFS_OK) {
		handle = g_new0 (SftpOpenHandle, 1);
		handle->sftp_handle = sftp_handle;
		handle->sftp_handle_len = sftp_handle_len;
		handle->path = path;
		handle->connection = conn;
		*method_handle = (MateVFSMethodHandle *) handle;

		sftp_connection_unlock (conn);

		DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Exit", G_STRFUNC));
		return MATE_VFS_OK;
	} else {
		*method_handle = NULL;

		g_free (path);

		sftp_connection_unref (conn);
		sftp_connection_unlock (conn);
		
		DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Exit", G_STRFUNC));
		return res;
	}
}

static MateVFSResult 
do_close (MateVFSMethod       *method,
	  MateVFSMethodHandle *method_handle,
	  MateVFSContext      *context) 
{
	SftpOpenHandle *handle;

	guint id, status;
	Buffer msg;

	guint i;

	DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Enter", G_STRFUNC));

	handle = SFTP_OPEN_HANDLE (method_handle);

	buffer_init (&msg);

	sftp_connection_lock (handle->connection);

	id = sftp_connection_get_id (handle->connection);
	buffer_write_gchar (&msg, SSH2_FXP_CLOSE);
	buffer_write_gint32 (&msg, id);
	buffer_write_block (&msg, handle->sftp_handle, handle->sftp_handle_len);
	buffer_send (&msg, handle->connection->out_fd);

	status = iobuf_read_result (handle->connection->in_fd, id);

	buffer_free (&msg);
	sftp_connection_unref (handle->connection);
	sftp_connection_unlock (handle->connection);

	for (i = handle->info_read_ptr; i < handle->info_write_ptr; ++i)
		g_free (handle->info[i].name);

	g_free (handle->info);
	g_free (handle->sftp_handle);
	g_free (handle->path);
	g_free (handle);

	DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Exit", G_STRFUNC));

	return status;
}

static MateVFSResult 
do_read (MateVFSMethod       *method, 
	 MateVFSMethodHandle *method_handle, 
	 gpointer              buffer_in,
	 MateVFSFileSize      num_bytes, 
	 MateVFSFileSize     *bytes_read, 
	 MateVFSContext      *context) 
{
	SftpOpenHandle *handle;
	Buffer msg;
	char type;
	int recv_id, status;
	guint req_ptr, req_svc_ptr, req_svc;
	guint len;
	guchar *buffer;
	guchar *curr_ptr;
	MateVFSResult result;
	gboolean got_eof;
	guint outstanding;
	int queue_len;

	struct ReadRequest 
	{
		gint     id;
		guint    req_len;
		guchar  *ptr;
	};

	struct ReadRequest *read_req;

	DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Enter", G_STRFUNC));

	got_eof = FALSE;
	
	handle = SFTP_OPEN_HANDLE (method_handle);
	*bytes_read = 0;
	req_ptr = 0; /* Queue head */
	req_svc_ptr = 0; /* Queue tail */
	curr_ptr = buffer_in;
	buffer = buffer_in;
	outstanding = 0;

	/* This must be at least one larger than max_req so we don't wrap around the compares.
	 * This means we always leave one unused element between the head and tail. */
	queue_len = max_req + 1;
	read_req = g_new0 (struct ReadRequest, queue_len);

	buffer_init (&msg);

	sftp_connection_lock (handle->connection);

	while ((*bytes_read < num_bytes) ||
	       (outstanding > 0)) {
		/* Request as many blocks as we can without overfilling the queue (i.e. max_req) */
		while (curr_ptr < buffer + num_bytes &&
		       (req_ptr + 1) % queue_len != req_svc_ptr) {
			read_req[req_ptr].id = sftp_connection_get_id (handle->connection);
			read_req[req_ptr].req_len =
				MIN ((buffer + num_bytes) - curr_ptr, default_req_len);
			read_req[req_ptr].ptr = curr_ptr;

			DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
				      "%s: (%d) Sending read request %d, length %d, pointer %p",
				      G_STRFUNC, req_ptr, read_req[req_ptr].id,
				      read_req[req_ptr].req_len, read_req[req_ptr].ptr));

			outstanding++;
			iobuf_send_read_request (handle->connection->out_fd,
						 read_req[req_ptr].id,
						 handle->offset + (read_req[req_ptr].ptr - buffer),
						 read_req[req_ptr].req_len,
						 handle->sftp_handle, handle->sftp_handle_len);

			curr_ptr += read_req[req_ptr].req_len;

			req_ptr = (req_ptr + 1) % queue_len;
		}

		buffer_clear (&msg);
		result = buffer_recv (&msg, handle->connection->in_fd);
		outstanding--;

		if (result != MATE_VFS_OK) {
			buffer_free (&msg);
			sftp_connection_unlock (handle->connection);
			return result;
		}

		type = buffer_read_gchar (&msg);
		recv_id = buffer_read_gint32 (&msg);

		/* Look for the id received among sent ids */
		for (req_svc = req_svc_ptr;
		     req_svc != req_ptr;
		     req_svc = (req_svc + 1) % queue_len) {
			if (read_req[req_svc].id == recv_id) {
				break;
			}
		}

		if (req_svc == req_ptr) { /* Didn't find the id -- unexpected reply */
			DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Unexpected id %d",
				      G_STRFUNC, recv_id));
			buffer_free (&msg);
			g_free (read_req);
			sftp_connection_unlock (handle->connection);
			return MATE_VFS_ERROR_PROTOCOL_ERROR;
		}

		switch (type) {
		    case SSH2_FXP_STATUS:
			status = buffer_read_gint32 (&msg);

			DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Got status message %d",
				      G_STRFUNC, status));

			DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: req_svc = %d, req_ptr = %d",
				      G_STRFUNC, req_svc, req_ptr));

			if (status != SSH2_FX_EOF) {
				DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: status return %d",
					      G_STRFUNC, sftp_status_to_vfs_result (status)));
				buffer_free (&msg);
				g_free (read_req);
				sftp_connection_unlock (handle->connection);
				return sftp_status_to_vfs_result (status);
			}

			if (read_req[req_svc].ptr == buffer)
				got_eof = TRUE;
			
			/* We hit an EOF, so make sure we don't try any more reads after this */
			num_bytes = MIN (num_bytes, read_req[req_svc].ptr - buffer);

			/* Kill this request */
			read_req[req_svc].id = 0;

			break;

		    case SSH2_FXP_DATA:
			len = buffer_read_gint32 (&msg);
			buffer_read (&msg, read_req[req_svc].ptr, len);

			*bytes_read += len;

			if (len < read_req[req_svc].req_len) {
				/* Missing data. Request that part */
				read_req[req_svc].id = sftp_connection_get_id (handle->connection);
				read_req[req_svc].req_len -= len;
				read_req[req_svc].ptr += len;

				outstanding++;
				iobuf_send_read_request
					(handle->connection->out_fd,
					 read_req[req_svc].id,
					 handle->offset + (read_req[req_svc].ptr - buffer),
					 read_req[req_svc].req_len,
					 handle->sftp_handle, handle->sftp_handle_len);
			} else
				read_req[req_svc].id = 0;

			break;

		    default:
			buffer_free (&msg);
			g_free (read_req);
			sftp_connection_unlock (handle->connection);
			return MATE_VFS_ERROR_PROTOCOL_ERROR;
		}

		/* Pop finished requests from the tail */
		while (req_svc_ptr != req_ptr && read_req[req_svc_ptr].id == 0) {
			req_svc_ptr = (req_svc_ptr + 1) % queue_len;
		}
	}

	handle->offset += *bytes_read;

	buffer_free (&msg);
	g_free (read_req);

	DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Exit", G_STRFUNC));

	sftp_connection_unlock (handle->connection);

	if (got_eof)
		return MATE_VFS_ERROR_EOF;
	
	return MATE_VFS_OK;
}

static MateVFSResult 
do_write (MateVFSMethod       *method, 
	  MateVFSMethodHandle *method_handle, 
	  gconstpointer         buffer_in, 
	  MateVFSFileSize      num_bytes, 
	  MateVFSFileSize     *bytes_written,
	  MateVFSContext      *context) 
{
	SftpOpenHandle *handle;
	Buffer msg;
	char type;
	int recv_id, status;
	guint req_ptr, req_svc, req_svc_ptr;
	guint curr_offset;
	const guchar *buffer;
	int queue_len;

	struct WriteRequest 
	{
		gint   id;
		guint  req_len;
		guint  offset;
	};

	struct WriteRequest *write_req;

	DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Enter", G_STRFUNC));

	handle = SFTP_OPEN_HANDLE (method_handle);

	/* This must be at least one larger than max_req so we don't wrap around the compares.
	 * This means we always leave one unused element between the head and tail. */
	queue_len = max_req + 1;
	write_req = g_new0 (struct WriteRequest, queue_len);
	
	buffer_init (&msg);
	*bytes_written = 0;
	curr_offset = 0;
	buffer = buffer_in;
	req_ptr = 0;
	req_svc_ptr = 0;

	sftp_connection_lock (handle->connection);

	while (*bytes_written < num_bytes) {
		while (curr_offset < num_bytes &&
		       (req_ptr + 1) % queue_len != req_svc_ptr) {
			write_req[req_ptr].id = sftp_connection_get_id (handle->connection);
			write_req[req_ptr].req_len = MIN (num_bytes - curr_offset, default_req_len);
			write_req[req_ptr].offset = curr_offset;

			curr_offset += write_req[req_ptr].req_len;

			DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
				      "%s: (%d) Sending write request %d, length %d, offset %d",
				      G_STRFUNC, req_ptr, write_req[req_ptr].id,
				      write_req[req_ptr].req_len, write_req[req_ptr].offset));

			buffer_clear (&msg);
			buffer_write_gchar (&msg, SSH2_FXP_WRITE);
			buffer_write_gint32 (&msg, write_req[req_ptr].id);
			buffer_write_block (&msg, handle->sftp_handle, handle->sftp_handle_len);
			buffer_write_gint64 (&msg, handle->offset + write_req[req_ptr].offset);
			buffer_write_block (&msg, buffer + write_req[req_ptr].offset,
					    write_req[req_ptr].req_len);
			
			buffer_send (&msg, handle->connection->out_fd);

			req_ptr = (req_ptr + 1) % queue_len;
		}

		buffer_clear (&msg);
		buffer_recv (&msg, handle->connection->in_fd);
		type = buffer_read_gchar (&msg);
		recv_id = buffer_read_gint32 (&msg);

		if (type != SSH2_FXP_STATUS) {
			buffer_free (&msg);
			g_free (write_req);
			sftp_connection_unlock (handle->connection);
			return MATE_VFS_ERROR_PROTOCOL_ERROR;
		}


		status = buffer_read_gint32 (&msg);
		if (status != SSH2_FX_OK) {
			buffer_free (&msg);
			g_free (write_req);
			sftp_connection_unlock (handle->connection);
			return sftp_status_to_vfs_result (status);
		}

		/* Look for the id received among sent ids */
		for (req_svc = req_svc_ptr;
		     req_svc != req_ptr;
		     req_svc = (req_svc + 1) % queue_len) {
			if (write_req[req_svc].id == recv_id) {
				break;
			}
		}
		
		if (req_svc == req_ptr) { /* Didn't find the id -- unexpected reply */
			DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Unexpected id %d",
				      G_STRFUNC, recv_id));
			buffer_free (&msg);
			g_free (write_req);
			sftp_connection_unlock (handle->connection);
			return MATE_VFS_ERROR_PROTOCOL_ERROR;
		}

		write_req[req_svc].id = 0;
		*bytes_written += write_req[req_svc].req_len;

		/* Pop finished requests from the tail */
		while (req_svc_ptr != req_ptr && write_req[req_svc_ptr].id == 0) {
			req_svc_ptr = (req_svc_ptr + 1) % queue_len;
		}
	}

	handle->offset += *bytes_written;

	buffer_free (&msg);
	g_free (write_req);

	DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Exit", G_STRFUNC));

	sftp_connection_unlock (handle->connection);

	return MATE_VFS_OK;
}

static MateVFSResult
do_seek (MateVFSMethod       *method,
	 MateVFSMethodHandle *method_handle,
	 MateVFSSeekPosition  whence,
	 MateVFSFileOffset    offset,
	 MateVFSContext      *context)
{
	SftpOpenHandle *handle;
	MateVFSFileInfo file_info = { 0, };
	MateVFSResult res;

	DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Enter", G_STRFUNC));

	handle = SFTP_OPEN_HANDLE (method_handle);

	switch (whence) {
	    case MATE_VFS_SEEK_START:
		handle->offset = offset;
		break;

	    case MATE_VFS_SEEK_CURRENT:
		handle->offset += offset;
		break;

	    case MATE_VFS_SEEK_END:
		res = do_get_file_info_from_handle (method, method_handle, &file_info,
						    MATE_VFS_FILE_INFO_DEFAULT, context);

		if (res != MATE_VFS_OK)
			return res;

		handle->offset = file_info.size + offset;
		break;
	}

	DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Exit", G_STRFUNC));

	return MATE_VFS_OK;
}

static MateVFSResult
do_tell (MateVFSMethod       *method,
	 MateVFSMethodHandle *method_handle,
	 MateVFSFileSize   *offset_return)
{
	SftpOpenHandle *handle;

	DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Enter", G_STRFUNC));

	handle = SFTP_OPEN_HANDLE (method_handle);
	*offset_return = handle->offset;

	DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Exit", G_STRFUNC));

	return MATE_VFS_OK;
}

static char *
sftp_readlink (SftpConnection *connection,
	       const char *path)
{
	Buffer msg;
	guint recv_id;
	char type;
	unsigned int id;
	char *ret;

	DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Enter", G_STRFUNC));

	id = sftp_connection_get_id (connection);

	DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Requesting symlink target for %s",
		      G_STRFUNC, path));

	buffer_init (&msg);
	buffer_write_gchar (&msg, SSH2_FXP_READLINK);
	buffer_write_gint32 (&msg, id);
	buffer_write_string (&msg, path);
	buffer_send (&msg, connection->out_fd);

	buffer_clear (&msg);

	buffer_recv (&msg, connection->in_fd);

	type = buffer_read_gchar (&msg);
	recv_id = buffer_read_gint32 (&msg);

	ret = NULL;

	if (recv_id != id)
		g_critical ("%s: ID mismatch (%u != %u)", G_STRFUNC, recv_id, id);
	else if (type == SSH2_FXP_NAME) {
		int count;

		count = buffer_read_gint32 (&msg);
		if (count == 1) {
			ret = buffer_read_string (&msg, NULL);
			DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
				      "%s: Symlink resolved to %s",
				      G_STRFUNC, ret));
		} else {
			DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
				      "%s: Readlink failed, got unexpected filename count (%d, 1 expected)",
				      G_STRFUNC, count));
		}
	} else if (type == SSH2_FXP_STATUS) {
		DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
			      "%s: Readlink failed, SSH2_FXP_STATUS response",
			      G_STRFUNC));
	} else {
		DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
			      "%s: Readlink failed, bad response (%d)",
			      G_STRFUNC, type));
	}

	buffer_free (&msg);

	return ret;
}

static void
update_mime_type_and_name_from_path (MateVFSFileInfo *file_info,
				     const char *path,
				     MateVFSFileInfoOptions options)
{
	if (file_info->name != NULL)
		g_free (file_info->name);

	if (file_info->valid_fields & MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE)
		g_free (file_info->mime_type);

	/* always use the original path as filename, even if the stat info
	 * refers to the actual target */
	if (!strcmp (path, "/"))
		file_info->name = g_strdup (path);
	else
		file_info->name = g_path_get_basename (path);

	file_info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_MIME_TYPE;

	if (file_info->valid_fields & MATE_VFS_FILE_INFO_FIELDS_TYPE &&
	    file_info->type == MATE_VFS_FILE_TYPE_SYMBOLIC_LINK)
		/* only relevant for broken symlinks, or if symlinks are not followed */
		file_info->mime_type = g_strdup ("x-special/symlink");
	else if (file_info->valid_fields & MATE_VFS_FILE_INFO_FIELDS_SYMLINK_NAME &&
		 file_info->symlink_name != NULL &&
		 (options & MATE_VFS_FILE_INFO_FOLLOW_LINKS) &&
		 file_info->type == MATE_VFS_FILE_TYPE_REGULAR)
		file_info->mime_type = g_strdup (mate_vfs_mime_type_from_name_or_default
						 (file_info->symlink_name, MATE_VFS_MIME_TYPE_UNKNOWN));
	else if (file_info->valid_fields & MATE_VFS_FILE_INFO_FIELDS_TYPE &&
		 file_info->type == MATE_VFS_FILE_TYPE_REGULAR)
		file_info->mime_type = g_strdup (mate_vfs_mime_type_from_name_or_default
						 (file_info->name, MATE_VFS_MIME_TYPE_UNKNOWN));
	else
		file_info->mime_type = g_strdup (mate_vfs_mime_type_from_mode_or_default (file_info->permissions, MATE_VFS_MIME_TYPE_UNKNOWN));
}

/* from mate-vfs-utils.c */
#define MAX_SYMLINKS_FOLLOWED 32

static MateVFSResult
get_file_info_for_path (SftpConnection          *conn,
			const char              *path,
			MateVFSFileInfo        *file_info,
			MateVFSFileInfoOptions  options)
{
	MateVFSResult res;
	unsigned int id;

	DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Enter", G_STRFUNC));

	if (conn->version == 0) { /* SSH2_FXP_STAT_VERSION_0 doesn't allow our symlink semantics */
		DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Server with protocol version 0 detected, LSTAT unsupported.",
			      G_STRFUNC));
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

	id = sftp_connection_get_id (conn);

	iobuf_send_string_request (conn->out_fd, id,
				   SSH2_FXP_LSTAT,
				   path, strlen (path));

	res = iobuf_read_file_info (conn->in_fd, file_info, id);
	if (res != MATE_VFS_OK) return res;

	if (options & MATE_VFS_FILE_INFO_FOLLOW_LINKS &&
	    file_info->type == MATE_VFS_FILE_TYPE_SYMBOLIC_LINK) {
		char *target_path;
		MateVFSFileInfo *target_info, *last_valid_target_info;
		unsigned int followed_symlinks;

		DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
			      "%s: Got MATE_VFS_FILE_INFO_FOLLOW_LINKS, encountered symlink, resolving.",
			      G_STRFUNC));

		followed_symlinks = 0;

		target_info = mate_vfs_file_info_new ();
		last_valid_target_info = NULL;

		target_path = NULL;

		/* resolve either to last existant symlink (s_1->...->s_n)->NULL
		 * or to first file which is not a symlink (s_1->...->s_n->t) */
		while (1) {
			char *next_target_reference;
			char *tmp;

			if (++followed_symlinks > MAX_SYMLINKS_FOLLOWED) {
				DEBUG2 (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
					       "%s: Too many symlinks while resolving %s.",
					       G_STRFUNC, target_path));
				res = MATE_VFS_ERROR_TOO_MANY_LINKS;
				/* we signal failure but still fill the file_info as good as we can.
				 * Is this allowed? */
				break;
			}

			next_target_reference = sftp_readlink (conn, target_path != NULL ? target_path : path);

			DEBUG2 (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
				       "%s: Resolved %s to %s",
				       G_STRFUNC, target_path != NULL ? target_path : path, next_target_reference));

			if (next_target_reference == NULL) {
				DEBUG2 (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
					       "%s: Resultion of %s to %s failed, target probably nonexistant.",
					       G_STRFUNC, target_path, next_target_reference));
				break;
			}

			tmp = target_path;
			target_path = mate_vfs_resolve_symlink (target_path != NULL ? target_path : path, next_target_reference);
			g_free (tmp);

			id = sftp_connection_get_id (conn);
			iobuf_send_string_request (conn->out_fd, id,
						   SSH2_FXP_LSTAT,
						   target_path, strlen (target_path));

			res = iobuf_read_file_info (conn->in_fd, target_info, id);
			if (res != MATE_VFS_OK ||
			    (target_info->valid_fields & MATE_VFS_FILE_INFO_FIELDS_TYPE) == 0) {
				DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
					      "%s: Resultion of %s to %s failed, could not get file info.",
					      G_STRFUNC, target_path, next_target_reference));
				res = MATE_VFS_OK;
				break;
			}

			if (last_valid_target_info == NULL)
				last_valid_target_info = mate_vfs_file_info_new ();
			else
				mate_vfs_file_info_clear (last_valid_target_info);

			mate_vfs_file_info_copy (last_valid_target_info, target_info);

			if (target_info->type != MATE_VFS_FILE_TYPE_SYMBOLIC_LINK) {
				DEBUG2 (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
					       "%s: Aborting symlink resolution, %s is no symlink.",
					       G_STRFUNC, target_path));
				break;
			}

			mate_vfs_file_info_clear (target_info);
		}


		DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
			      "%s: Resolved %s -> %s", 
			      G_STRFUNC, path, target_path));

		if (last_valid_target_info != NULL) {
			mate_vfs_file_info_clear (file_info);
			mate_vfs_file_info_copy (file_info, last_valid_target_info);
			mate_vfs_file_info_unref (last_valid_target_info);
		}

		mate_vfs_file_info_unref (target_info);

		MATE_VFS_FILE_INFO_SET_SYMLINK (file_info, TRUE);
		file_info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_SYMLINK_NAME;
		file_info->symlink_name = target_path;
	} else if (file_info->type == MATE_VFS_FILE_TYPE_SYMBOLIC_LINK) {
		file_info->valid_fields |= MATE_VFS_FILE_INFO_FIELDS_SYMLINK_NAME;
		file_info->symlink_name = sftp_readlink (conn, path);
		MATE_VFS_FILE_INFO_SET_SYMLINK (file_info, TRUE);
	}

	update_mime_type_and_name_from_path (file_info, path, options);

	DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Exit", G_STRFUNC));

	return res;
}

static MateVFSResult
do_get_file_info (MateVFSMethod          *method,
		  MateVFSURI             *uri,
		  MateVFSFileInfo        *file_info,
		  MateVFSFileInfoOptions  options,
		  MateVFSContext         *context) 
{
	SftpConnection *conn;
	MateVFSResult res;
	char *path;

	DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Enter (no exit notify)", G_STRFUNC));

	res = sftp_get_connection (&conn, uri);
	if (res != MATE_VFS_OK) return res;

	URI_TO_PATH (uri, path);
	res = get_file_info_for_path (conn, path, file_info, options);
	g_free (path);

	sftp_connection_unref (conn);
	sftp_connection_unlock (conn);

	return res;
}

static MateVFSResult
do_get_file_info_from_handle (MateVFSMethod          *method,
			      MateVFSMethodHandle    *method_handle,
			      MateVFSFileInfo        *file_info,
			      MateVFSFileInfoOptions  options,
			      MateVFSContext         *context)
{
	SftpOpenHandle *handle;

	DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Enter (no exit notify)", G_STRFUNC));

	handle = SFTP_OPEN_HANDLE (method_handle);

	/* we can't use FSTAT, because it always follows the symlink, but doesn't return the target file name.
	 * So we fall back to our home-brewn LSTAT/readlink code. */
	return get_file_info_for_path (handle->connection,
				       handle->path,
				       file_info, options);
}

static gboolean 
do_is_local (MateVFSMethod    *method, 
	     const MateVFSURI *uri)
{
	return FALSE;
}

static MateVFSResult
do_open_directory (MateVFSMethod          *method,
		   MateVFSMethodHandle   **method_handle,
		   MateVFSURI             *uri,
		   MateVFSFileInfoOptions  options,
		   MateVFSContext         *context)
{
	SftpConnection *conn;
	SftpOpenHandle *handle;
	MateVFSResult res;

	gchar *sftp_handle;
	Buffer msg;
	guint id, sftp_handle_len;
	gchar *path;

	DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Enter", G_STRFUNC));

	res = sftp_get_connection (&conn, uri);
	if (res != MATE_VFS_OK) return res;

	id = sftp_connection_get_id (conn);

	URI_TO_PATH (uri, path);

	DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Opening %s", G_STRFUNC, path));

	buffer_init (&msg);
	buffer_write_gchar (&msg, SSH2_FXP_OPENDIR);
	buffer_write_gint32 (&msg, id);
	buffer_write_string (&msg, path);
	buffer_send (&msg, conn->out_fd);

	buffer_free (&msg);

	res = iobuf_read_handle (conn->in_fd, &sftp_handle, id, &sftp_handle_len);

	DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Result is %d", G_STRFUNC, res));

	if (res == MATE_VFS_OK) {
		handle = g_new0 (SftpOpenHandle, 1);
		handle->sftp_handle = sftp_handle;
		handle->sftp_handle_len = sftp_handle_len;
		handle->connection = conn;
		handle->info = g_new0 (MateVFSFileInfo, INIT_DIR_INFO_ALLOC);
		handle->info_alloc = INIT_DIR_INFO_ALLOC;
		handle->info_read_ptr = 0;
		handle->info_write_ptr = 0;
		handle->path = path;
		handle->dir_options = options;
		*method_handle = (MateVFSMethodHandle *) handle;
		
		sftp_connection_unlock (conn);

		DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Opened directory %p",
			      G_STRFUNC, handle));
		DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Exit", G_STRFUNC));
		return MATE_VFS_OK;
	} else {
		/* For some reason, some servers report EOF when the directory doesn't exist. *shrug* */
		if (res == MATE_VFS_ERROR_EOF)
			res = MATE_VFS_ERROR_NOT_FOUND;

		g_free (path);

		sftp_connection_unref (conn);
		sftp_connection_unlock (conn);

		*method_handle = NULL;
		DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Exit", G_STRFUNC));
		return res;
	}
}

static MateVFSResult
do_close_directory (MateVFSMethod       *method,
		    MateVFSMethodHandle *method_handle,
		    MateVFSContext      *context) 
{
	return do_close (method, method_handle, context);
}

static MateVFSResult
do_read_directory (MateVFSMethod       *method,
		   MateVFSMethodHandle *method_handle,
		   MateVFSFileInfo     *file_info,
		   MateVFSContext      *context)
{
	SftpOpenHandle *handle;
	guint id, recv_id;
	gint status, count, i;
	Buffer msg;
	gchar type;

	DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Enter", G_STRFUNC));

	handle = SFTP_OPEN_HANDLE (method_handle);

	DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Reading directory %p",
		      G_STRFUNC, handle));

	if (handle->info_read_ptr < handle->info_write_ptr) {
		DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Found directory entry %s in cache",
			      G_STRFUNC, handle->info[handle->info_read_ptr].name));

		mate_vfs_file_info_copy (file_info, &(handle->info[handle->info_read_ptr++]));
		DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Exit", G_STRFUNC));
		return MATE_VFS_OK;
	}

	sftp_connection_lock (handle->connection);

	DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: No entries in cache", G_STRFUNC));

	id = sftp_connection_get_id (handle->connection);

	buffer_init (&msg);
	buffer_write_gchar (&msg, SSH2_FXP_READDIR);
	buffer_write_gint32 (&msg, id);
	buffer_write_block (&msg, handle->sftp_handle, handle->sftp_handle_len);
	buffer_send (&msg, handle->connection->out_fd);

	buffer_clear (&msg);

	buffer_recv (&msg, handle->connection->in_fd);

	type = buffer_read_gchar (&msg);
	recv_id = buffer_read_gint32 (&msg);

	if (recv_id != id) {
		buffer_free (&msg);
		sftp_connection_unlock (handle->connection);
		return MATE_VFS_ERROR_PROTOCOL_ERROR;
	}

	if (type == SSH2_FXP_STATUS) {
		status = buffer_read_gint32 (&msg);
		buffer_free (&msg);
		
		if (status == SSH2_FX_EOF || SSH2_FX_OK) {
			DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
				      "%s: End of directory reached (EOF status)",
				      G_STRFUNC));
			sftp_connection_unlock (handle->connection);
			return MATE_VFS_ERROR_EOF;
		} else {
			DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Error status %d",
				      G_STRFUNC, status));
			do_close (method, method_handle, context);
			sftp_connection_unlock (handle->connection);
			return sftp_status_to_vfs_result (status);
		}
	}
	else if (type == SSH2_FXP_NAME) {
		count = buffer_read_gint32 (&msg);
		if (count == 0) {
			DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
				      "%s: End of directory reached (count 0)",
				      G_STRFUNC));
			buffer_free (&msg);
			sftp_connection_unlock (handle->connection);
			return MATE_VFS_ERROR_EOF;
		}

		DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Count is %d", G_STRFUNC, count));

		if (handle->info_write_ptr + count > handle->info_alloc) {
			if (handle->info_read_ptr > 0) {
				memmove (&(handle->info[handle->info_read_ptr]), handle->info,
					 sizeof (MateVFSFileInfo) *
					 (handle->info_write_ptr - handle->info_read_ptr));

				handle->info_write_ptr -= handle->info_read_ptr;
				handle->info_read_ptr = 0;
			}

			while (handle->info_write_ptr + count > handle->info_alloc) {
				handle->info_alloc *= 2;
				handle->info = g_renew (MateVFSFileInfo, handle->info, handle->info_alloc);
				memset (&(handle->info[handle->info_write_ptr]), 0,
					sizeof (MateVFSFileInfo) *
					(handle->info_alloc - handle->info_write_ptr));
			}
		}

		for (i = 0; i < count; i++) {
			MateVFSFileInfo *info;
			char *filename, *longname;
			char *path;

			info = &(handle->info[handle->info_write_ptr]);

			filename = buffer_read_string (&msg, NULL);
			longname = buffer_read_string (&msg, NULL);

			buffer_read_file_info (&msg, info);

			DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: %d, filename is %s",
				      G_STRFUNC, i, filename));

			if (info->valid_fields & MATE_VFS_FILE_INFO_FIELDS_TYPE &&
			    info->type == MATE_VFS_FILE_TYPE_SYMBOLIC_LINK) {
				path = g_build_filename (handle->path, filename, NULL);
				get_file_info_for_path (handle->connection, path,
							info, handle->dir_options);
				g_free (path);
			} else
				update_mime_type_and_name_from_path (info, filename, handle->dir_options);

			DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: %d, MIME type is %s",
				      G_STRFUNC, i, info->mime_type));

			g_free (filename);
			g_free (longname);

			handle->info_write_ptr++;
		}
	} else {
		DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Got wrong packet type (%d)",
			      G_STRFUNC, type));
		buffer_free (&msg);
		sftp_connection_unlock (handle->connection);
		return MATE_VFS_ERROR_PROTOCOL_ERROR;
	}

	buffer_free (&msg);
	
	if (handle->info_read_ptr < handle->info_write_ptr) {
		mate_vfs_file_info_copy (file_info, &(handle->info[handle->info_read_ptr]));
		g_free (handle->info[handle->info_read_ptr].name);
		handle->info[handle->info_read_ptr].name = NULL;
		g_free (handle->info[handle->info_read_ptr].mime_type);
		handle->info[handle->info_read_ptr].mime_type = NULL;
		handle->info_read_ptr++;
		DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Exit", G_STRFUNC));
		sftp_connection_unlock (handle->connection);
		return MATE_VFS_OK;
	} else {
		DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Exit", G_STRFUNC));
		sftp_connection_unlock (handle->connection);
		return MATE_VFS_ERROR_EOF;
	}
}

static MateVFSResult
do_make_directory (MateVFSMethod  *method,
		   MateVFSURI     *uri,
		   guint            perm,
		   MateVFSContext *context)
{
	SftpConnection *conn;
	MateVFSResult res;
	gchar *path;
	guint id;
	MateVFSFileInfo info;

	res = sftp_get_connection (&conn, uri);
	if (res != MATE_VFS_OK) return res;

	id = sftp_connection_get_id (conn);

	URI_TO_PATH (uri, path);

	memset (&info, 0, sizeof (MateVFSFileInfo));
	iobuf_send_string_request_with_file_info (conn->out_fd, id, SSH2_FXP_MKDIR,
						  path, strlen (path), &info,
						  MATE_VFS_SET_FILE_INFO_NONE);

	g_free (path);

	res = iobuf_read_result (conn->in_fd, id);

	sftp_connection_unref (conn);
	sftp_connection_unlock (conn);

	if (res == MATE_VFS_ERROR_GENERIC &&
	    mate_vfs_uri_exists (uri)) {
		res = MATE_VFS_ERROR_FILE_EXISTS;
	}

	return res;
}

static MateVFSResult
do_remove_directory (MateVFSMethod  *method,
		     MateVFSURI     *uri,
		     MateVFSContext *context)
{
	SftpConnection *conn;
	MateVFSResult res;
	gchar *path;
	guint id;

	res = sftp_get_connection (&conn, uri);
	if (res != MATE_VFS_OK) return res;

	id = sftp_connection_get_id (conn);

	URI_TO_PATH (uri, path);
	iobuf_send_string_request (conn->out_fd, id, SSH2_FXP_RMDIR, path, strlen (path));
	g_free (path);

	res = iobuf_read_result (conn->in_fd, id);

	sftp_connection_unref (conn);
	sftp_connection_unlock (conn);

	return res;
}

static MateVFSResult
do_move (MateVFSMethod  *method,
	 MateVFSURI     *old_uri,
	 MateVFSURI     *new_uri,
	 gboolean         force_replace,
	 MateVFSContext *context)
{
	SftpConnection *conn;
	MateVFSResult res;
	gboolean same_fs;

	Buffer msg;
	guint id;

	gchar *old_path, *new_path;

	same_fs = FALSE;
	do_check_same_fs (method, old_uri, new_uri, &same_fs, NULL);
	if (!same_fs) {
		return MATE_VFS_ERROR_NOT_SAME_FILE_SYSTEM;
	}

	res = sftp_get_connection (&conn, old_uri);
	if (res != MATE_VFS_OK) return res;

	URI_TO_PATH (old_uri, old_path);
	URI_TO_PATH (new_uri, new_path);

	id = sftp_connection_get_id (conn);

	/* if force_replace is specified, try to remove new_uri */
	if (force_replace) {
		iobuf_send_string_request (conn->out_fd,
					   id,
					   SSH2_FXP_REMOVE,
					   new_path,
					   strlen (new_path));
		res = iobuf_read_result (conn->in_fd, id);
		if (res != MATE_VFS_OK && res != MATE_VFS_ERROR_NOT_FOUND)
			goto bail;
	}

	buffer_init (&msg);
	buffer_write_gchar (&msg, SSH2_FXP_RENAME);
	buffer_write_gint32 (&msg, id);
	buffer_write_string (&msg, old_path);
	buffer_write_string (&msg, new_path);
	buffer_send (&msg, conn->out_fd);
	buffer_free (&msg);

	res = iobuf_read_result (conn->in_fd, id);

 bail:
	g_free (old_path);
	g_free (new_path);

	sftp_connection_unref (conn);
	sftp_connection_unlock (conn);

	return res;
}

static MateVFSResult
do_rename (MateVFSMethod  *method,
	   MateVFSURI     *old_uri,
	   const gchar     *new_name,
	   MateVFSContext *context)
{
	SftpConnection *conn;
	MateVFSResult res;
	char *old_dirname;

	Buffer msg;
	guint id;

	gchar *old_path, *new_path;

	res = sftp_get_connection (&conn, old_uri);
	if (res != MATE_VFS_OK) return res;

	URI_TO_PATH (old_uri, old_path);

	old_dirname = g_path_get_dirname (old_path);

	new_path = g_build_filename (old_dirname, new_name, NULL);
	if (new_path == NULL) {
		g_free (old_path);
		sftp_connection_unref (conn);
		sftp_connection_unlock (conn);
		return MATE_VFS_ERROR_INVALID_URI;
	}

	g_free (old_dirname);

	id = sftp_connection_get_id (conn);

	buffer_init (&msg);
	buffer_write_gchar (&msg, SSH2_FXP_RENAME);
	buffer_write_gint32 (&msg, id);
	buffer_write_string (&msg, old_path);
	buffer_write_string (&msg, new_path);
	buffer_send (&msg, conn->out_fd);
	buffer_free (&msg);

	g_free (old_path);
	g_free (new_path);

	res = iobuf_read_result (conn->in_fd, id);

	sftp_connection_unref (conn);
	sftp_connection_unlock (conn);

	return res;
}

static MateVFSResult
do_unlink (MateVFSMethod  *method,
	   MateVFSURI     *uri,
	   MateVFSContext *context)
{
	SftpConnection *conn;
	MateVFSResult res;
	gchar *path;
	guint id;

	res = sftp_get_connection (&conn, uri);
	if (res != MATE_VFS_OK) return res;

	id = sftp_connection_get_id (conn);

	URI_TO_PATH (uri, path);
	iobuf_send_string_request (conn->out_fd, id, SSH2_FXP_REMOVE, path, strlen (path));
	g_free (path);

	res = iobuf_read_result (conn->in_fd, id);

	sftp_connection_unref (conn);
	sftp_connection_unlock (conn);

	return res;
}

static MateVFSResult
do_check_same_fs (MateVFSMethod  *method,
		  MateVFSURI     *a,
		  MateVFSURI     *b,
		  gboolean        *same_fs_return,
		  MateVFSContext *context)
{
	const gchar *a_host_name, *b_host_name;
	const gchar *a_user_name, *b_user_name;

	g_return_val_if_fail (a != NULL, MATE_VFS_ERROR_INTERNAL);
	g_return_val_if_fail (b != NULL, MATE_VFS_ERROR_INTERNAL);

	a_host_name = mate_vfs_uri_get_host_name (a);
	b_host_name = mate_vfs_uri_get_host_name (b);
	a_user_name = mate_vfs_uri_get_user_name (a);
	b_user_name = mate_vfs_uri_get_user_name (b);

	g_return_val_if_fail (a_host_name != NULL, MATE_VFS_ERROR_INVALID_URI);
	g_return_val_if_fail (b_host_name != NULL, MATE_VFS_ERROR_INVALID_URI);

	/* TODO this is wrong because we can't
	 * tell what login name is used for NULL
	 * user names, but at least this check is
	 * I/O-less.
	 * We could also look up the connections
	 * for both URIs and check whether their
	 * actual login name (i.e. the user name
	 * passed with -l to the SSH client)
	 * matches.
	 */
	if (a_user_name == NULL)
		a_user_name = "";
	if (b_user_name == NULL)
		b_user_name = "";

	if (same_fs_return != NULL)
		*same_fs_return =
			((!strcmp (a_host_name, b_host_name)) &&
			 (!strcmp (a_user_name, b_user_name)));

	return MATE_VFS_OK;
}

static MateVFSResult
do_set_file_info (MateVFSMethod          *method,
		  MateVFSURI             *uri,
		  const MateVFSFileInfo  *info,
		  MateVFSSetFileInfoMask  mask,
		  MateVFSContext         *context)
{
	SftpConnection *conn;
	MateVFSResult res = MATE_VFS_OK;
	guint id;
	gchar *path;

	DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Enter", G_STRFUNC));

	if ((mask & ~(MATE_VFS_SET_FILE_INFO_NAME |
		      MATE_VFS_SET_FILE_INFO_PERMISSIONS |
		      MATE_VFS_SET_FILE_INFO_OWNER |
		      MATE_VFS_SET_FILE_INFO_TIME)) != 0) {
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

	if (mask & (MATE_VFS_SET_FILE_INFO_PERMISSIONS |
		    MATE_VFS_SET_FILE_INFO_OWNER |
		    MATE_VFS_SET_FILE_INFO_TIME))
	{
		res = sftp_get_connection (&conn, uri);
		if (res != MATE_VFS_OK) return res;

		id = sftp_connection_get_id (conn);

		URI_TO_PATH (uri, path);
		iobuf_send_string_request_with_file_info (conn->out_fd, id, SSH2_FXP_SETSTAT,
							  path, strlen (path), info, mask);
		g_free (path);

		res = iobuf_read_result (conn->in_fd, id);

		sftp_connection_unref (conn);
		sftp_connection_unlock (conn);
	}

	if (res == MATE_VFS_OK && (mask & MATE_VFS_SET_FILE_INFO_NAME))
		res = do_rename (method, uri, info->name, context);

	DEBUG (g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "%s: Exit: %d", G_STRFUNC, res));

	return res;
}

static MateVFSResult
do_create_symlink (MateVFSMethod   *method,
		   MateVFSURI      *uri,
		   const gchar      *target,
		   MateVFSContext  *context)
{
	SftpConnection *conn;
	MateVFSResult res;
	MateVFSURI *target_uri;
	Buffer msg;
	guint id;
	char *path;
	char *real_target;
	gboolean same_fs;

	res = sftp_get_connection (&conn, uri);
	if (res != MATE_VFS_OK) return res;

	if (conn->version < 3) {
		sftp_connection_unref (conn);
		sftp_connection_unlock (conn);
		return MATE_VFS_ERROR_NOT_SUPPORTED;
	}

	URI_TO_PATH (uri, path);

	real_target = NULL;

	target_uri = mate_vfs_uri_new (target);
	if (target_uri != NULL) {
		same_fs = FALSE;
		do_check_same_fs (method, uri, target_uri, &same_fs, NULL);
		if (!same_fs) {
			g_free (path);
			mate_vfs_uri_unref (target_uri);
			sftp_connection_unref (conn);
			sftp_connection_unlock (conn);
			return MATE_VFS_ERROR_NOT_SAME_FILE_SYSTEM;
		}

		URI_TO_PATH (target_uri, real_target);
		mate_vfs_uri_unref (target_uri);
	}

	if (real_target == NULL)
		real_target = g_strdup (target);

	id = sftp_connection_get_id (conn);
	
	buffer_init (&msg);
	buffer_write_gchar (&msg, SSH2_FXP_SYMLINK);
	buffer_write_gint32 (&msg, id);
	buffer_write_string (&msg, real_target);
	buffer_write_string (&msg, path);
	buffer_send (&msg, conn->out_fd);
	buffer_free (&msg);

	res = iobuf_read_result (conn->in_fd, id);

	sftp_connection_unref (conn);
	sftp_connection_unlock (conn);

	if (res == MATE_VFS_ERROR_GENERIC &&
	    mate_vfs_uri_exists (uri)) {
		res = MATE_VFS_ERROR_FILE_EXISTS;
	}

	g_free (path);
	g_free (real_target);

	return res;
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
	NULL, /* truncate_handle */
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
	NULL, /* truncate */
	NULL, /* find_directory */
	do_create_symlink,
	NULL, /* monitor_add */
	NULL /* monitor_cancel */
};

MateVFSMethod *
vfs_module_init (const char *method_name, 
		 const char *args)
{
	return &method;
}

static gboolean
close_thunk (gpointer key, gpointer value, gpointer user_data) 
{
	sftp_connection_close (SFTP_CONNECTION (value));
	return TRUE;
}

void
vfs_module_shutdown (MateVFSMethod *method)
{
	G_LOCK (sftp_connection_table);

	if (sftp_connection_table != NULL) {
		g_hash_table_foreach_remove (sftp_connection_table, (GHRFunc) close_thunk, NULL);
	}

	G_UNLOCK (sftp_connection_table);
}
