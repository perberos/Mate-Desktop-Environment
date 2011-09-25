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

#include "config.h"

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <locale.h>
#include <fcntl.h>
#include <sys/types.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>

#include "mdm-common.h"

/* Like fopen with "w" */
FILE *
mdm_safe_fopen_w (const char *file,
                  mode_t      perm)
{
        int fd;
        FILE *ret;
        VE_IGNORE_EINTR (g_unlink (file));
        do {
                int flags;

                errno = 0;
                flags = O_EXCL | O_CREAT | O_TRUNC | O_WRONLY;
#ifdef O_NOCTTY
                flags |= O_NOCTTY;
#endif
#ifdef O_NOFOLLOW
                flags |= O_NOFOLLOW;
#endif

                fd = g_open (file, flags, perm);
        } while (errno == EINTR);

        if (fd < 0) {
                return NULL;
        }

        ret = fdopen (fd, "w");
        return ret;
}
