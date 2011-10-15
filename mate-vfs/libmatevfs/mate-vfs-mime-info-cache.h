/* mate-vfs-mime-info-cache.h
 *
 * Copyright (C) 2004 Red Hat, Inc
 *
 * The Mate Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Mate Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Mate Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef MATE_VFS_DISABLE_DEPRECATED

#ifndef MATE_VFS_MIME_INFO_CACHE_H
#define MATE_VFS_MIME_INFO_CACHE_H

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * DESKTOP_ENTRY_GROUP:
 *
 * The #GKeyFile group used for desktop entries.
 **/
#define DESKTOP_ENTRY_GROUP "Desktop Entry"

GList              *mate_vfs_mime_get_all_desktop_entries (const char *mime_type);
gchar              *mate_vfs_mime_get_default_desktop_entry (const char *mime_type);

#ifdef __cplusplus
}
#endif

#endif /* MATE_VFS_MIME_INFO_CACHE_H */

#endif /* MATE_VFS_DISABLE_DEPRECATED */
