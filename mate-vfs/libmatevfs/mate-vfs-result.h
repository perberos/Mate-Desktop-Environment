/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* mate-vfs-result.h - Result handling for the MATE Virtual File System.

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

   Author: Ettore Perazzoli <ettore@comm2000.it>
           Seth Nickell <snickell@stanford.edu>
*/

#ifndef MATE_VFS_RESULT_H
#define MATE_VFS_RESULT_H

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* IMPORTANT NOTICE: If you add error types here, please also add the
   corresponding descriptions in `mate-vfs-result.c'.  Moreover, *always* add
   new values at the end of the list, and *never* remove values.  */

/**
 * MateVFSResult:
 * @MATE_VFS_OK: No error.
 * @MATE_VFS_ERROR_NOT_FOUND: File not found.
 * @MATE_VFS_ERROR_GENERIC: Generic error.
 * @MATE_VFS_ERROR_INTERNAL: Internal error.
 * @MATE_VFS_ERROR_BAD_PARAMETERS: Invalid parameters.
 * @MATE_VFS_ERROR_NOT_SUPPORTED: Unsupported operation.
 * @MATE_VFS_ERROR_IO: I/O error.
 * @MATE_VFS_ERROR_CORRUPTED_DATA: Data corrupted.
 * @MATE_VFS_ERROR_WRONG_FORMAT: Format not valid.
 * @MATE_VFS_ERROR_BAD_FILE: Bad file handle.
 * @MATE_VFS_ERROR_TOO_BIG: File too big.
 * @MATE_VFS_ERROR_NO_SPACE: No space left on device.
 * @MATE_VFS_ERROR_READ_ONLY: Read-only file system.
 * @MATE_VFS_ERROR_INVALID_URI: Invalid URI.
 * @MATE_VFS_ERROR_NOT_OPEN: File not open.
 * @MATE_VFS_ERROR_INVALID_OPEN_MODE: Open mode not valid.
 * @MATE_VFS_ERROR_ACCESS_DENIED: Access denied.
 * @MATE_VFS_ERROR_TOO_MANY_OPEN_FILES: Too many open files.
 * @MATE_VFS_ERROR_EOF: End of file.
 * @MATE_VFS_ERROR_NOT_A_DIRECTORY: Not a directory.
 * @MATE_VFS_ERROR_IN_PROGRESS: Operation in progress.
 * @MATE_VFS_ERROR_INTERRUPTED: Operation interrupted.
 * @MATE_VFS_ERROR_FILE_EXISTS: File exists.
 * @MATE_VFS_ERROR_LOOP: Looping links encountered.
 * @MATE_VFS_ERROR_NOT_PERMITTED: Operation not permitted.
 * @MATE_VFS_ERROR_IS_DIRECTORY: Is a directory.
 * @MATE_VFS_ERROR_NO_MEMORY: Not enough memory.
 * @MATE_VFS_ERROR_HOST_NOT_FOUND: Host not found.
 * @MATE_VFS_ERROR_INVALID_HOST_NAME: Host name not valid.
 * @MATE_VFS_ERROR_HOST_HAS_NO_ADDRESS: Host has no address.
 * @MATE_VFS_ERROR_LOGIN_FAILED: Login failed.
 * @MATE_VFS_ERROR_CANCELLED: Operation cancelled.
 * @MATE_VFS_ERROR_DIRECTORY_BUSY: Directory busy.
 * @MATE_VFS_ERROR_DIRECTORY_NOT_EMPTY: Directory not empty.
 * @MATE_VFS_ERROR_TOO_MANY_LINKS: Too many links.
 * @MATE_VFS_ERROR_READ_ONLY_FILE_SYSTEM: Read only file system.
 * @MATE_VFS_ERROR_NOT_SAME_FILE_SYSTEM: Not on the same file system.
 * @MATE_VFS_ERROR_NAME_TOO_LONG: Name too long.
 * @MATE_VFS_ERROR_SERVICE_NOT_AVAILABLE: Service not available.
 * @MATE_VFS_ERROR_SERVICE_OBSOLETE: Request obsoletes service's data.
 * @MATE_VFS_ERROR_PROTOCOL_ERROR: Protocol error.
 * @MATE_VFS_ERROR_NO_MASTER_BROWSER: Could not find master browser.
 * @MATE_VFS_ERROR_NO_DEFAULT: No default action associated.
 * @MATE_VFS_ERROR_NO_HANDLER: No handler for URL scheme.
 * @MATE_VFS_ERROR_PARSE: Error parsing command line.
 * @MATE_VFS_ERROR_LAUNCH: Error launching command.
 * @MATE_VFS_ERROR_TIMEOUT: Timeout reached.
 * @MATE_VFS_ERROR_NAMESERVER: Nameserver error.
 * @MATE_VFS_ERROR_LOCKED: The resource is locked.
 * @MATE_VFS_ERROR_DEPRECATED_FUNCTION: Function call deprecated.
 * @MATE_VFS_ERROR_INVALID_FILENAME: The specified filename is invalid.
 * @MATE_VFS_ERROR_NOT_A_SYMBOLIC_LINK: Not a symbolic link.
 *
 * A #MateVFSResult informs library clients about the result of a file operation.
 * Unless it is #MATE_VFS_OK, it denotes that a problem occurred and the operation
 * could not be executed successfully.
 *
 * mate_vfs_result_to_string() provides a textual representation of #MateVFSResults.
 **/
typedef enum {
	MATE_VFS_OK,
	MATE_VFS_ERROR_NOT_FOUND,
	MATE_VFS_ERROR_GENERIC,
	MATE_VFS_ERROR_INTERNAL,
	MATE_VFS_ERROR_BAD_PARAMETERS,
	MATE_VFS_ERROR_NOT_SUPPORTED,
	MATE_VFS_ERROR_IO,
	MATE_VFS_ERROR_CORRUPTED_DATA,
	MATE_VFS_ERROR_WRONG_FORMAT,
	MATE_VFS_ERROR_BAD_FILE,
	MATE_VFS_ERROR_TOO_BIG,
	MATE_VFS_ERROR_NO_SPACE,
	MATE_VFS_ERROR_READ_ONLY,
	MATE_VFS_ERROR_INVALID_URI,
	MATE_VFS_ERROR_NOT_OPEN,
	MATE_VFS_ERROR_INVALID_OPEN_MODE,
	MATE_VFS_ERROR_ACCESS_DENIED,
	MATE_VFS_ERROR_TOO_MANY_OPEN_FILES,
	MATE_VFS_ERROR_EOF,
	MATE_VFS_ERROR_NOT_A_DIRECTORY,
	MATE_VFS_ERROR_IN_PROGRESS,
	MATE_VFS_ERROR_INTERRUPTED,
	MATE_VFS_ERROR_FILE_EXISTS,
	MATE_VFS_ERROR_LOOP,
	MATE_VFS_ERROR_NOT_PERMITTED,
	MATE_VFS_ERROR_IS_DIRECTORY,
	MATE_VFS_ERROR_NO_MEMORY,
	MATE_VFS_ERROR_HOST_NOT_FOUND,
	MATE_VFS_ERROR_INVALID_HOST_NAME,
	MATE_VFS_ERROR_HOST_HAS_NO_ADDRESS,
	MATE_VFS_ERROR_LOGIN_FAILED,
	MATE_VFS_ERROR_CANCELLED,
	MATE_VFS_ERROR_DIRECTORY_BUSY,
	MATE_VFS_ERROR_DIRECTORY_NOT_EMPTY,
	MATE_VFS_ERROR_TOO_MANY_LINKS,
	MATE_VFS_ERROR_READ_ONLY_FILE_SYSTEM,
	MATE_VFS_ERROR_NOT_SAME_FILE_SYSTEM,
	MATE_VFS_ERROR_NAME_TOO_LONG,
	MATE_VFS_ERROR_SERVICE_NOT_AVAILABLE,
	MATE_VFS_ERROR_SERVICE_OBSOLETE,
	MATE_VFS_ERROR_PROTOCOL_ERROR,
	MATE_VFS_ERROR_NO_MASTER_BROWSER,
	MATE_VFS_ERROR_NO_DEFAULT,
	MATE_VFS_ERROR_NO_HANDLER,
	MATE_VFS_ERROR_PARSE,
	MATE_VFS_ERROR_LAUNCH,
	MATE_VFS_ERROR_TIMEOUT,
	MATE_VFS_ERROR_NAMESERVER,
	MATE_VFS_ERROR_LOCKED,
	MATE_VFS_ERROR_DEPRECATED_FUNCTION,
	MATE_VFS_ERROR_INVALID_FILENAME,
	MATE_VFS_ERROR_NOT_A_SYMBOLIC_LINK,
	MATE_VFS_NUM_ERRORS
} MateVFSResult;

const char	*mate_vfs_result_to_string	    (MateVFSResult result);
MateVFSResult   mate_vfs_result_from_errno_code   (int errno_code);
MateVFSResult	 mate_vfs_result_from_errno	    (void);
MateVFSResult   mate_vfs_result_from_h_errno_val  (int h_errno_code);
MateVFSResult   mate_vfs_result_from_h_errno      (void);

#ifdef __cplusplus
}
#endif

#endif /* MATE_VFS_RESULT_H */
