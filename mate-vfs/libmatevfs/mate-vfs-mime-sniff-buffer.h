/*
 * mate-vfs-mime-sniff-buffer.h
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

#ifndef MATE_VFS_MIME_SNIFF_BUFFER_H
#define MATE_VFS_MIME_SNIFF_BUFFER_H

#include <libmatevfs/mate-vfs-handle.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef MateVFSResult (* MateVFSSniffBufferSeekCall)(gpointer context,
		MateVFSSeekPosition whence, MateVFSFileOffset offset);
typedef MateVFSResult (* MateVFSSniffBufferReadCall)(gpointer context,
		gpointer buffer, MateVFSFileSize bytes, MateVFSFileSize *bytes_read);

typedef struct MateVFSMimeSniffBuffer MateVFSMimeSniffBuffer;

MateVFSMimeSniffBuffer	*_mate_vfs_mime_sniff_buffer_new_from_handle
					(MateVFSHandle 		*file);
MateVFSMimeSniffBuffer	*_mate_vfs_mime_sniff_buffer_new_from_memory
					(const guchar 			*buffer,
					 gssize 			buffer_size);
MateVFSMimeSniffBuffer	*mate_vfs_mime_sniff_buffer_new_from_existing_data
					(const guchar 			*buffer,
					 gssize 			buffer_size);
MateVFSMimeSniffBuffer	*_mate_vfs_mime_sniff_buffer_new_generic
					(MateVFSSniffBufferSeekCall	seek_callback,
					 MateVFSSniffBufferReadCall	read_callback,
					 gpointer			context);


void			 mate_vfs_mime_sniff_buffer_free
					(MateVFSMimeSniffBuffer	*buffer);

MateVFSResult		 _mate_vfs_mime_sniff_buffer_get
					(MateVFSMimeSniffBuffer	*buffer,
					 gssize				size);

const char  		*mate_vfs_get_mime_type_for_buffer
					 (MateVFSMimeSniffBuffer	*buffer);
gboolean		 _mate_vfs_sniff_buffer_looks_like_text
					 (MateVFSMimeSniffBuffer	*buffer);
gboolean		 _mate_vfs_sniff_buffer_looks_like_mp3
					 (MateVFSMimeSniffBuffer	*buffer);
#ifdef __cplusplus
}
#endif

#endif
