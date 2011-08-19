/* MateConf
 * Copyright (C) 1999, 2000 Red Hat Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef MATECONF_MATECONF_ERROR_H
#define MATECONF_MATECONF_ERROR_H

#include <glib.h>

G_BEGIN_DECLS

#define MATECONF_ERROR mateconf_error_quark ()

/* Error Numbers */

/* Sync with ConfigErrorType in MateConf.idl, and some switch statements in the code */
typedef enum {
  MATECONF_ERROR_SUCCESS = 0,
  MATECONF_ERROR_FAILED = 1,        /* Something didn't work, don't know why, probably unrecoverable
                                    so there's no point having a more specific errno */

  MATECONF_ERROR_NO_SERVER = 2,     /* Server can't be launched/contacted */
  MATECONF_ERROR_NO_PERMISSION = 3, /* don't have permission for that */
  MATECONF_ERROR_BAD_ADDRESS = 4,   /* Address couldn't be resolved */
  MATECONF_ERROR_BAD_KEY = 5,       /* directory or key isn't valid (contains bad
                                    characters, or malformed slash arrangement) */
  MATECONF_ERROR_PARSE_ERROR = 6,   /* Syntax error when parsing */
  MATECONF_ERROR_CORRUPT = 7,       /* Fatal error parsing/loading information inside the backend */
  MATECONF_ERROR_TYPE_MISMATCH = 8, /* Type requested doesn't match type found */
  MATECONF_ERROR_IS_DIR = 9,        /* Requested key operation on a dir */
  MATECONF_ERROR_IS_KEY = 10,       /* Requested dir operation on a key */
  MATECONF_ERROR_OVERRIDDEN = 11,   /* Read-only source at front of path has set the value */
  MATECONF_ERROR_OAF_ERROR = 12,    /* liboaf error */
  MATECONF_ERROR_LOCAL_ENGINE = 13, /* Tried to use remote operations on a local engine */
  MATECONF_ERROR_LOCK_FAILED = 14,  /* Failed to get a lockfile */
  MATECONF_ERROR_NO_WRITABLE_DATABASE = 15, /* nowhere to write a value */
  MATECONF_ERROR_IN_SHUTDOWN = 16   /* server is shutting down */
} MateConfError;

GQuark mateconf_error_quark (void);

G_END_DECLS

#endif



