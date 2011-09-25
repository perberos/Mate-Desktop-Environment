/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * (c) 2000 Eazel, Inc.
 * (c) 2001,2002 George Lebl
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

#ifndef _MDM_COMMON_UNKNOWN_H
#define _MDM_COMMON_UNKNOWN_H

#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <glib.h>

G_BEGIN_DECLS

#define        VE_IGNORE_EINTR(expr) \
        do {                         \
                errno = 0;           \
                expr;                \
        } while G_UNLIKELY (errno == EINTR);

/* like fopen with "w" but unlinks and uses O_EXCL */
FILE *         mdm_safe_fopen_w  (const char *file,
                                  mode_t      perm);

G_END_DECLS

#endif /* _MDM_COMMON_UNKNOWN_H */
