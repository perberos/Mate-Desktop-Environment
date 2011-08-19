/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-error.h
 *
 * Copyright (C) 2007 David Zeuthen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#if !defined (__GDU_INSIDE_GDU_H) && !defined (GDU_COMPILATION)
#error "Only <gdu/gdu.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef __GDU_ERROR_H
#define __GDU_ERROR_H

#include <gdu/gdu-types.h>

G_BEGIN_DECLS

/**
 * GduError:
 * @GDU_ERROR_FAILED: The operation failed.
 * @GDU_ERROR_BUSY: The device is busy
 * @GDU_ERROR_CANCELLED: The operation was cancelled
 * @GDU_ERROR_INHIBITED: The daemon is being inhibited.
 * @GDU_ERROR_INVALID_OPTION: An invalid option was passed
 * @GDU_ERROR_NOT_SUPPORTED: Operation not supported.
 * @GDU_ERROR_ATA_SMART_WOULD_WAKEUP: Getting S.M.A.R.T. data for the device would require to spin it up.
 * @GDU_ERROR_PERMISSION_DENIED: Permission denied.
 * @GDU_ERROR_FILESYSTEM_DRIVER_MISSING: The filesystem driver for a filesystem is not installed.
 * @GDU_ERROR_FILESYSTEM_TOOLS_MISSING: User-space tools to carry out the request action on a filesystem is not installed.
 *
 * Error codes in the #GDU_ERROR domain.
 */
typedef enum
{
        GDU_ERROR_FAILED,
        GDU_ERROR_BUSY,
        GDU_ERROR_CANCELLED,
        GDU_ERROR_INHIBITED,
        GDU_ERROR_INVALID_OPTION,
        GDU_ERROR_NOT_SUPPORTED,
        GDU_ERROR_ATA_SMART_WOULD_WAKEUP,
        GDU_ERROR_PERMISSION_DENIED,
        GDU_ERROR_FILESYSTEM_DRIVER_MISSING,
        GDU_ERROR_FILESYSTEM_TOOLS_MISSING
} GduError;

/**
 * GDU_ERROR:
 *
 * Error domain used for errors reported from udisks daemon
 * via D-Bus. Note that not all remote errors are mapped to this
 * domain. Errors in this domain will come from the #GduError
 * enumeration. See #GError for more information on error domains.
 */
#define GDU_ERROR gdu_error_quark ()

GQuark      gdu_error_quark           (void);

G_END_DECLS

#endif /* __GDU_ERROR_H */
