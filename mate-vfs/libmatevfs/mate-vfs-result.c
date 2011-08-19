/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-error.c - Error handling for the MATE Virtual File System.

   Copyright (C) 1999 Free Software Foundation
   Copyright (C) 2007 Thadeu Lima de Souza Cascardo

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

   Modified on 2007-01-29 by Thadeu Lima de Souza Cascardo to handle
   ENOTSUP.

*/

#include <config.h>
#include "mate-vfs-result.h"

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <errno.h>
#ifndef G_OS_WIN32
#include <netdb.h>
#endif

#ifndef G_OS_WIN32
/* AIX #defines h_errno */
#ifndef h_errno
extern int h_errno;
#endif
#endif

static char *status_strings[] = {
	/* MATE_VFS_OK */				N_("No error"),
	/* MATE_VFS_ERROR_NOT_FOUND */			N_("File not found"),
	/* MATE_VFS_ERROR_GENERIC */			N_("Generic error"),
	/* MATE_VFS_ERROR_INTERNAL */			N_("Internal error"),
	/* MATE_VFS_ERROR_BAD_PARAMETERS */		N_("Invalid parameters"),
	/* MATE_VFS_ERROR_NOT_SUPPORTED */		N_("Unsupported operation"),
	/* MATE_VFS_ERROR_IO */			N_("I/O error"),
	/* MATE_VFS_ERROR_CORRUPTED_DATA */		N_("Data corrupted"),
	/* MATE_VFS_ERROR_WRONG_FORMAT */		N_("Format not valid"),
	/* MATE_VFS_ERROR_BAD_FILE */			N_("Bad file handle"),
	/* MATE_VFS_ERROR_TOO_BIG */			N_("File too big"),
	/* MATE_VFS_ERROR_NO_SPACE */			N_("No space left on device"),
	/* MATE_VFS_ERROR_READ_ONLY */			N_("Read-only file system"),
	/* MATE_VFS_ERROR_INVALID_URI */		N_("Invalid URI"),
	/* MATE_VFS_ERROR_NOT_OPEN */			N_("File not open"),
	/* MATE_VFS_ERROR_INVALID_OPEN_MODE */		N_("Open mode not valid"),
	/* MATE_VFS_ERROR_ACCESS_DENIED */		N_("Access denied"),
	/* MATE_VFS_ERROR_TOO_MANY_OPEN_FILES */	N_("Too many open files"),
	/* MATE_VFS_ERROR_EOF */			N_("End of file"),
	/* MATE_VFS_ERROR_NOT_A_DIRECTORY */		N_("Not a directory"),
	/* MATE_VFS_ERROR_IN_PROGRESS */		N_("Operation in progress"),
	/* MATE_VFS_ERROR_INTERRUPTED */		N_("Operation interrupted"),
	/* MATE_VFS_ERROR_FILE_EXISTS */		N_("File exists"),
	/* MATE_VFS_ERROR_LOOP */			N_("Looping links encountered"),
	/* MATE_VFS_ERROR_NOT_PERMITTED */		N_("Operation not permitted"),
	/* MATE_VFS_ERROR_IS_DIRECTORY */       	N_("Is a directory"),
        /* MATE_VFS_ERROR_NO_MEMORY */             	N_("Not enough memory"),
	/* MATE_VFS_ERROR_HOST_NOT_FOUND */		N_("Host not found"),
	/* MATE_VFS_ERROR_INVALID_HOST_NAME */		N_("Host name not valid"),
	/* MATE_VFS_ERROR_HOST_HAS_NO_ADDRESS */  	N_("Host has no address"),
	/* MATE_VFS_ERROR_LOGIN_FAILED */		N_("Login failed"),
	/* MATE_VFS_ERROR_CANCELLED */			N_("Operation cancelled"),
	/* MATE_VFS_ERROR_DIRECTORY_BUSY */     	N_("Directory busy"),
	/* MATE_VFS_ERROR_DIRECTORY_NOT_EMPTY */ 	N_("Directory not empty"),
	/* MATE_VFS_ERROR_TOO_MANY_LINKS */		N_("Too many links"),
	/* MATE_VFS_ERROR_READ_ONLY_FILE_SYSTEM */	N_("Read only file system"),
	/* MATE_VFS_ERROR_NOT_SAME_FILE_SYSTEM */	N_("Not on the same file system"),
	/* MATE_VFS_ERROR_NAME_TOO_LONG */		N_("Name too long"),
	/* MATE_VFS_ERROR_SERVICE_NOT_AVAILABLE */     N_("Service not available"),
	/* MATE_VFS_ERROR_SERVICE_OBSOLETE */          N_("Request obsoletes service's data"),
	/* MATE_VFS_ERROR_PROTOCOL_ERROR */		N_("Protocol error"),
	/* MATE_VFS_ERROR_NO_MASTER_BROWSER */		N_("Could not find master browser"),
	/* MATE_VFS_ERROR_NO_DEFAULT */		N_("No default action associated"),
	/* MATE_VFS_ERROR_NO_HANDLER */		N_("No handler for URL scheme"),
	/* MATE_VFS_ERROR_PARSE */			N_("Error parsing command line"),
	/* MATE_VFS_ERROR_LAUNCH */			N_("Error launching command"),
	/* MATE_VFS_ERROR_TIMEOUT */			N_("Timeout reached"),
	/* MATE_VFS_ERROR_NAMESERVER */                N_("Nameserver error"),
 	/* MATE_VFS_ERROR_LOCKED */			N_("The resource is locked"),
	/* MATE_VFS_ERROR_DEPRECATED_FUNCTION */       N_("Function call deprecated"),
	/* MATE_VFS_ERROR_INVALID_FILENAME */		N_("Invalid filename"),
	/* MATE_VFS_ERROR_NOT_A_SYMBOLIC_LINK */	N_("Not a symbolic link")
};

/**
 * mate_vfs_result_from_errno_code:
 * @errno_code: integer of the same type as the system "errno".
 *
 * Converts a system errno value to a #MateVFSResult.
 *
 * Return value: a #MateVFSResult equivalent to @errno_code.
 */
MateVFSResult
mate_vfs_result_from_errno_code (int errno_code)
{
	/* Please keep these in alphabetical order.  */
	switch (errno_code) {
	case E2BIG:        return MATE_VFS_ERROR_TOO_BIG;
	case EACCES:	   return MATE_VFS_ERROR_ACCESS_DENIED;
	case EBUSY:	   return MATE_VFS_ERROR_DIRECTORY_BUSY;
	case EBADF:	   return MATE_VFS_ERROR_BAD_FILE;
#ifdef ECONNREFUSED
	case ECONNREFUSED: return MATE_VFS_ERROR_SERVICE_NOT_AVAILABLE;
#endif
	case EEXIST:	   return MATE_VFS_ERROR_FILE_EXISTS;
	case EFAULT:	   return MATE_VFS_ERROR_INTERNAL;
	case EFBIG:	   return MATE_VFS_ERROR_TOO_BIG;
	case EINTR:	   return MATE_VFS_ERROR_INTERRUPTED;
	case EINVAL:	   return MATE_VFS_ERROR_BAD_PARAMETERS;
	case EIO:	   return MATE_VFS_ERROR_IO;
	case EISDIR:	   return MATE_VFS_ERROR_IS_DIRECTORY;
#ifdef ELOOP
	case ELOOP:	   return MATE_VFS_ERROR_LOOP;
#endif
	case EMFILE:	   return MATE_VFS_ERROR_TOO_MANY_OPEN_FILES;
	case EMLINK:	   return MATE_VFS_ERROR_TOO_MANY_LINKS;
#ifdef ENETUNREACH
	case ENETUNREACH:  return MATE_VFS_ERROR_SERVICE_NOT_AVAILABLE;
#endif
	case ENFILE:	   return MATE_VFS_ERROR_TOO_MANY_OPEN_FILES;
#if ENOTEMPTY != EEXIST
	case ENOTEMPTY:    return MATE_VFS_ERROR_DIRECTORY_NOT_EMPTY;
#endif
	case ENOENT:	   return MATE_VFS_ERROR_NOT_FOUND;
	case ENOMEM:	   return MATE_VFS_ERROR_NO_MEMORY;
	case ENOSPC:	   return MATE_VFS_ERROR_NO_SPACE;
	case ENOTDIR:	   return MATE_VFS_ERROR_NOT_A_DIRECTORY;
	case EPERM:	   return MATE_VFS_ERROR_NOT_PERMITTED;
	case EROFS:	   return MATE_VFS_ERROR_READ_ONLY_FILE_SYSTEM;
#ifdef ETIMEDOUT
	case ETIMEDOUT:    return MATE_VFS_ERROR_TIMEOUT;
#endif
	case EXDEV:	   return MATE_VFS_ERROR_NOT_SAME_FILE_SYSTEM;
	case ENAMETOOLONG: return MATE_VFS_ERROR_NAME_TOO_LONG;
#ifdef ENOTSUP
	case ENOTSUP:    return MATE_VFS_ERROR_NOT_SUPPORTED;
#endif
	
		/* FIXME bugzilla.eazel.com 1191: To be completed.  */
	default:	return MATE_VFS_ERROR_GENERIC;
	}
}

/**
 * mate_vfs_result_from_errno:
 * 
 * Converts the system errno to a #MateVFSResult.
 *
 * Return value: a #MateVFSResult equivalent to the current system errno.
 */
MateVFSResult
mate_vfs_result_from_errno (void)
{
       return mate_vfs_result_from_errno_code(errno);
}
 
#ifndef G_OS_WIN32
/**
 * mate_vfs_result_from_h_errno:
 * 
 * Converts the system "h_errno" to a #MateVFSResult (h_errno represents errors
 * accessing and finding internet hosts)
 *
 * Return value: a #MateVFSResult equivalent to the current system "h_errno".
 */
MateVFSResult
mate_vfs_result_from_h_errno (void)
{
	return mate_vfs_result_from_h_errno_val (h_errno);
}

/**
 * mate_vfs_result_from_h_errno_val:
 * @h_errno_code: an integer representing the same error code
 * as the system h_errno.
 * 
 * Converts the error code @h_errno_code into a #MateVFSResult.
 * 
 * Return Value: The #MateVFSResult equivalent to the @h_errno_code.
 */
MateVFSResult
mate_vfs_result_from_h_errno_val (int h_errno_code)
{
	switch (h_errno_code) {
	case HOST_NOT_FOUND:	return MATE_VFS_ERROR_HOST_NOT_FOUND;
	case NO_ADDRESS:	return MATE_VFS_ERROR_HOST_HAS_NO_ADDRESS;
	case TRY_AGAIN:		return MATE_VFS_ERROR_NAMESERVER;
	case NO_RECOVERY:	return MATE_VFS_ERROR_NAMESERVER;
	default:
		return MATE_VFS_ERROR_GENERIC;
	}
}
#endif

/**
 * mate_vfs_result_to_string:
 * @result: a #MateVFSResult to convert to a string.
 *
 * Returns a string representing @result, useful for debugging
 * purposes, but probably not appropriate for passing to the user.
 *
 * Return value: a string representing @result.
 */
const char *
mate_vfs_result_to_string (MateVFSResult result)
{
	if ((guint) result >= (guint) (sizeof (status_strings)
				      / sizeof (*status_strings)))
		return _("Unknown error");

	return _(status_strings[(guint) result]);
}
