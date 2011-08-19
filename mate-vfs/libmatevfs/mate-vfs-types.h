/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-types.h - Types used by the MATE Virtual File System.

   Copyright (C) 1999, 2001 Free Software Foundation

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

   Author: Ettore Perazzoli <ettore@gnu.org>
           Seth Nickell <snickell@stanford.edu>
*/

#ifndef MATE_VFS_TYPES_H
#define MATE_VFS_TYPES_H

#ifndef MATE_VFS_DISABLE_DEPRECATED

/* see mate-vfs-file-size.h for MATE_VFS_SIZE_IS_<type> */
/* see mate-vfs-file-size.h for MATE_VFS_OFFSET_IS_<type> */
/* see mate-vfs-file-size.h for MATE_VFS_SIZE_FORMAT_STR */
/* see mate-vfs-file-size.h for MATE_VFS_OSFFSET_FORMAT_STR */
/* see mate-vfs-file-size.h for MateVFSFileSize */
/* see mate-vfs-file-size.h for MateVFSFileOffset */
/* see mate-vfs-result.h for MateVFSResult */
/* see mate-vfs-method.h for MateVFSOpenMode */
/* see mate-vfs-method.h for MateVFSSeekPosition */
/* see mate-vfs-file-info.h for MateVFSFileType */
/* see mate-vfs-file-info.h for MateVFSFilePermissions */
/* see mate-vfs-handle.h for MateVFSHandle */
/* see mate-vfs-uri.h for MateVFSURI */
/* see mate-vfs-uri.h for MateVFSToplevelURI */
/* see mate-vfs-uri.h for MateVFSURIHideOptions */
/* see mate-vfs-file-info.h for MateVFSFileFlags */
/* see mate-vfs-file-info.h for MateVFSFileInfoFields */
/* see mate-vfs-file-info.h for MateVFSFileInfo */
/* see mate-vfs-file-info.h for MateVFSFileInfoOptions */
/* see mate-vfs-file-info.h for MateVFSFileInfoMask */
/* see mate-vfs-find-directory.h for MateVFSFindDirectoryKind */
/* see mate-vfs-xfer.h for MateVFSXferOptions */
/* see mate-vfs-xfer.h for MateVFSXferProgressStatus */
/* see mate-vfs-xfer.h for MateVFSXferOverwriteMode */
/* see mate-vfs-xfer.h for MateVFSXferOverwriteAction */
/* see mate-vfs-xfer.h for MateVFSXferErrorMode */
/* see mate-vfs-xfer.h for MateVFSXferErrorAction */
/* see mate-vfs-xfer.h for MateVFSXferPhase */
/* see mate-vfs-xfer.h for MateVFSXferProgressInfo */
/* see mate-vfs-xfer.h for MateVFSXferProgressCallback */
/* see mate-vfs-async-ops.h for MateVFSAsyncHandle */
/* see mate-vfs-async-ops.h for MateVFSAsyncCallback */
/* see mate-vfs-async-ops.h for MateVFSAsyncOpenCallback */
/* see mate-vfs-async-ops.h for MateVFSAsyncCreateCallback */
/* see mate-vfs-async-ops.h for MateVFSAsyncOpenAsChannelCallback */
/* see mate-vfs-async-ops.h for MateVFSAsyncCloseCallback */
/* see mate-vfs-async-ops.h for MateVFSAsyncReadCallback */
/* see mate-vfs-async-ops.h for MateVFSAsyncWriteCallback */
/* see mate-vfs-file-info.h for MateVFSFileInfoResult */
/* see mate-vfs-async-ops.h for MateVFSAsyncGetFileInfoCallback */
/* see mate-vfs-async-ops.h for MateVFSAsyncSetFileInfoCallback */
/* see mate-vfs-async-ops.h for MateVFSAsyncDirectoryLoadCallback */
/* see mate-vfs-async-ops.h for MateVFSAsyncXferProgressCallback */
/* see mate-vfs-async-ops.h for MateVFSFindDirectoryResult */
/* see mate-vfs-async-ops.h for MateVFSAsyncFindDirectoryCallback */
/* see mate-vfs-module-callback.h for MateVFSModuleCallback */

/* Includes to provide compatibility with programs that
   still include mate-vfs-types.h directly */
#include <libmatevfs/mate-vfs-async-ops.h>
#include <libmatevfs/mate-vfs-module-callback.h>
#include <libmatevfs/mate-vfs-file-info.h>
#include <libmatevfs/mate-vfs-file-size.h>
#include <libmatevfs/mate-vfs-find-directory.h>
#include <libmatevfs/mate-vfs-result.h>
#include <libmatevfs/mate-vfs-uri.h>
#include <libmatevfs/mate-vfs-xfer.h>

#endif /* MATE_VFS_DISABLE_DEPRECATED */

#endif /* MATE_VFS_TYPES_H */
