/*
 * mate-vfs-mime-sniff-buffer-private.h
 * Utility for implementing mate_vfs_mime_type_from_magic, and other
 * mime-type sniffing calls.
 *
 * Copyright (C) 2000 Eazel, Inc.
 * All rights reserved.
 *
 * This file is part of the Mate Library.
 *
 * The Mate Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Mate Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Mate Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef MATE_VFS_MIME_SNIFF_BUFFER_PRIVATE_H
#define MATE_VFS_MIME_SNIFF_BUFFER_PRIVATE_H

#include <libmatevfs/mate-vfs-mime-sniff-buffer.h>

struct MateVFSMimeSniffBuffer {
	guchar *buffer;
	gssize buffer_length;
        gboolean read_whole_file;
	gboolean owning;

	MateVFSSniffBufferSeekCall seek;
	MateVFSSniffBufferReadCall read;
	gpointer context;
};

const char *_mate_vfs_get_mime_type_internal         (MateVFSMimeSniffBuffer *buffer,
						       const char              *file_name,
						       gboolean                 use_suffix);
const char *_mate_vfs_mime_get_type_from_magic_table (MateVFSMimeSniffBuffer *buffer);

#endif
