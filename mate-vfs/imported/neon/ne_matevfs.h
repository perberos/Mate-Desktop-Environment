/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-dns-sd.c - DNS-SD functions

Copyright (C) 2004 Christian Kellner <gicmo@mate-de.org>

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
*/

#include <ne_request.h>
#include <libmatevfs/mate-vfs-result.h>

#ifndef NE_MATEVFS
#define NE_MATEVFS

#ifdef __cplusplus
extern "C" {
#endif

MateVFSResult  ne_matevfs_last_error (ne_request *req);

#ifdef __cplusplus
}
#endif

#endif
