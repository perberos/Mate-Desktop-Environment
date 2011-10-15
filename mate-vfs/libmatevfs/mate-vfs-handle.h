/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-handle.h - Handle object for MATE VFS files.

   Copyright (C) 1999 Free Software Foundation

   The Mate Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Ettore Perazzoli <ettore@comm2000.it>
*/

#ifndef MATE_VFS_HANDLE_H
#define MATE_VFS_HANDLE_H

#include <libmatevfs/mate-vfs-context.h>
#include <libmatevfs/mate-vfs-file-size.h>
#include <libmatevfs/mate-vfs-file-info.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * MateVFSMethodHandle:
 *
 * Generic handle (pointer) that encapsulates the method internals
 * associated with a specific resource.
 *
 * Methods are supposed to store all information and open resources
 * regarding a handle in a dynamically allocated structure, which
 * is casted into a #MateVFSMethodHandle (in fact a void pointer)
 * when it is passed around in generic MateVFS functions.
 * During the lifetime of a handle, MateVFS stores it as a
 * generic pointer. After closing a handle, it will not be
 * known to MateVFS anymore.
 *
 * There are file handles and file monitor handles.
 *
 * File handles are opened in the #MateVFSMethod's
 * #MateVFSMethodOpenFunc, #MateVFSMethodCreateFunc
 * and #MateVFSMethodOpenDirectoryFunc
 * and closed in its #MateVFSMethodCloseFunc and
 * #MateVFSMethodCloseDirectoryFunc.
 *
 * A number of #MateVFSMethod operations rely on file handles:
 *
 * #MateVFSMethodReadFunc
 * #MateVFSMethodWriteFunc
 * #MateVFSMethodSeekFunc
 * #MateVFSMethodTellFunc
 * #MateVFSMethodGetFileInfoFromHandleFunc
 * #MateVFSMethodTruncateHandleFunc
 * #MateVFSMethodFileControlFunc
 *
 * File monitor handles however are created using
 * #MateVFSMethodMonitorAddFunc
 * and cancelled using
 * #MateVFSMethodMonitorCancelFunc
 **/
typedef gpointer MateVFSMethodHandle;

/**
 * MateVFSHandle:
 *
 * Handle to a file.
 *
 * A handle is obtained using mate_vfs_open() and mate_vfs_create()
 * family of functions on the file. A handle represents a file stream, mate_vfs_close(),
 * mate_vfs_write(), mate_vfs_read() and all the other I/O operations take a
 * MateVFSHandle * that identifies the file where the operation is going to be
 * performed.
 **/
typedef struct MateVFSHandle MateVFSHandle;

/**
 * MateVFSOpenMode:
 * @MATE_VFS_OPEN_NONE: No access.
 * @MATE_VFS_OPEN_READ: Read access.
 * @MATE_VFS_OPEN_WRITE: Write access.
 * @MATE_VFS_OPEN_RANDOM: Random access.
 * @MATE_VFS_OPEN_TRUNCATE: Truncate file before accessing it, i.e. delete its contents.
 *
 * Mode in which files are opened. If MATE_VFS_OPEN_RANDOM is not used, the
 * file will be have to be accessed sequentially.
 **/
typedef enum {
        MATE_VFS_OPEN_NONE = 0,
        MATE_VFS_OPEN_READ = 1 << 0,
        MATE_VFS_OPEN_WRITE = 1 << 1,
        MATE_VFS_OPEN_RANDOM = 1 << 2,
	MATE_VFS_OPEN_TRUNCATE = 1 << 3
} MateVFSOpenMode;

/**
 * MateVFSSeekPosition:
 * @MATE_VFS_SEEK_START: Start of the file.
 * @MATE_VFS_SEEK_CURRENT: Current position.
 * @MATE_VFS_SEEK_END: End of the file.
 *
 * This is used to specify the start position for seek operations.
 **/
typedef enum {
        MATE_VFS_SEEK_START,
        MATE_VFS_SEEK_CURRENT,
        MATE_VFS_SEEK_END
} MateVFSSeekPosition;


#ifdef __cplusplus
}
#endif

#endif /* MATE_VFS_HANDLE_H */
