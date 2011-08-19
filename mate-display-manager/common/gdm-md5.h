/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * gdm-md5.h md5 implementation (based on L Peter Deutsch implementation)
 *
 * Copyright (C) 2003 Red Hat Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef GDM_MD5_H
#define GDM_MD5_H

#include <glib.h>

G_BEGIN_DECLS

typedef struct GdmMD5Context GdmMD5Context;

/**
 * A context used to store the state of the MD5 algorithm
 */
struct GdmMD5Context
{
        guint32       count[2];       /**< message length in bits, lsw first */
        guint32       abcd[4];        /**< digest buffer */
        unsigned char buf[64];        /**< accumulate block */
};

void        gdm_md5_init    (GdmMD5Context   *context);
void        gdm_md5_update  (GdmMD5Context   *context,
                             const GString   *data);
gboolean    gdm_md5_final   (GdmMD5Context   *context,
                             GString         *results);

G_END_DECLS

#endif /* GDM_MD5_H */
