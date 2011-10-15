/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* mate-vfs-mime-info.h
 *
 * Copyright (C) 1998 Miguel de Icaza
 * Copyright (C) 2000 Eazel, Inc
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

#ifndef MATE_VFS_MIME_INFO_H
#define MATE_VFS_MIME_INFO_H

#include <glib.h>
#include <libmatevfs/mate-vfs-result.h>

#ifdef __cplusplus
extern "C" {
#endif

const char 	*mate_vfs_mime_get_value        		(const char *mime_type,
								 const char *key);

gboolean	 mate_vfs_mime_type_is_known			(const char *mime_type);

void             mate_vfs_mime_freeze                          (void);
void             mate_vfs_mime_thaw                            (void);

GList 	   	*mate_vfs_mime_get_extensions_list 		(const char *mime_type);
void	   	 mate_vfs_mime_extensions_list_free 		(GList      *list);

MateVFSResult	 mate_vfs_mime_set_registered_type_key 	(const char *mime_type,
							  	 const char *key,
							  	 const char *data);

	/* forces a reload of the config files */
void       	 mate_vfs_mime_info_reload   	  	 	(void);

#ifndef MATE_VFS_DISABLE_DEPRECATED

	/* functions which access to the .keys files */
MateVFSResult   mate_vfs_mime_set_value                       (const char *mime_type,
								 const char *key,
								 const char *value);
GList      	*mate_vfs_mime_get_key_list      		(const char *mime_type);
void             mate_vfs_mime_keys_list_free                  (GList *mime_type_list);

	/* functions to access the .mime files */
char 	   	*mate_vfs_mime_get_extensions_string 	 	(const char *mime_type);
char 	   	*mate_vfs_mime_get_extensions_pretty_string    (const char *mime_type);
GList 	        *mate_vfs_get_registered_mime_types 	 	(void);
void	         mate_vfs_mime_registered_mime_type_list_free 	(GList      *list);
MateVFSResult   mate_vfs_mime_set_extensions_list             (const char *mime_type,
								 const char *extensions_list);
void             mate_vfs_mime_registered_mime_type_delete     (const char *mime_type);
void             mate_vfs_mime_reset                           (void);
#endif /* MATE_VFS_DISABLE_DEPRECATED */

#ifdef __cplusplus
}
#endif

#endif /* MATE_VFS_MIME_INFO_H */














