/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* mate-vfs-mime-handlers.h - Mime type handlers for the MATE Virtual
   File System.

   Copyright (C) 2000 Eazel, Inc.

   The Mate Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Maciej Stachowiak <mjs@eazel.com> */

#ifndef MATE_VFS_MIME_HANDLERS_H
#define MATE_VFS_MIME_HANDLERS_H

#include <libmatevfs/mate-vfs-result.h>
#include <libmatevfs/mate-vfs-mime-utils.h>
#include <libmatevfs/mate-vfs-uri.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	MATE_VFS_MIME_APPLICATION_ARGUMENT_TYPE_URIS,
	MATE_VFS_MIME_APPLICATION_ARGUMENT_TYPE_PATHS,
	MATE_VFS_MIME_APPLICATION_ARGUMENT_TYPE_URIS_FOR_NON_FILES
} MateVFSMimeApplicationArgumentType;

typedef struct _MateVFSMimeApplicationPrivate MateVFSMimeApplicationPrivate;

/**
 * MateVFSMimeApplication:
 * @id: The desktop ID of the application.
 * @name: The user-visible name of the application.
 *
 * A struct encapsulating information about an
 * application known to the MIME database.
 *
 * Only very few fields of this information are actually
 * public, most of them must be queried using the
 * <literal>mate_vfs_mime_application_*()</literal>
 * API.
 **/
typedef struct {
	/*< public > */
	char *id;
	char *name;

	/*< private > */
#ifndef MATE_VFS_DISABLE_DEPRECATED
	char *command;
	gboolean can_open_multiple_files;
	MateVFSMimeApplicationArgumentType expects_uris;
	GList *supported_uri_schemes;
#else
	char *_command;
	gboolean _can_open_multiple_files;
	MateVFSMimeApplicationArgumentType _expects_uris;
	GList *_supported_uri_schemes;
#endif
	gboolean requires_terminal;

	/* Padded to avoid future breaks in ABI compatibility */
	void *reserved1;

	MateVFSMimeApplicationPrivate *priv;
} MateVFSMimeApplication;

/* Database */

MateVFSMimeApplication *mate_vfs_mime_get_default_application                   (const char              *mime_type);
MateVFSMimeApplication *mate_vfs_mime_get_default_application_for_uri           (const char		   *uri,
									           const char              *mime_type);
GList *                  mate_vfs_mime_get_all_applications                      (const char              *mime_type);
GList *			 mate_vfs_mime_get_all_applications_for_uri	          (const char              *uri,
									           const char              *mime_type);
/* MIME types */

const char 	        *mate_vfs_mime_get_description   		          (const char              *mime_type);
gboolean 	         mate_vfs_mime_can_be_executable   		          (const char              *mime_type);

/* Application */

MateVFSMimeApplication *mate_vfs_mime_application_new_from_desktop_id           (const char              *id);
MateVFSResult           mate_vfs_mime_application_launch                        (MateVFSMimeApplication *app,
									           GList                   *uris);
MateVFSResult           mate_vfs_mime_application_launch_with_env	          (MateVFSMimeApplication *app,
									           GList                   *uris,
									           char                   **envp);
const char		*mate_vfs_mime_application_get_desktop_id	          (MateVFSMimeApplication *app);
const char		*mate_vfs_mime_application_get_desktop_file_path         (MateVFSMimeApplication *app);
const char		*mate_vfs_mime_application_get_name                      (MateVFSMimeApplication *app);
const char              *mate_vfs_mime_application_get_generic_name              (MateVFSMimeApplication *app);
const char              *mate_vfs_mime_application_get_icon                      (MateVFSMimeApplication *app);
const char		*mate_vfs_mime_application_get_exec		          (MateVFSMimeApplication *app);
const char		*mate_vfs_mime_application_get_binary_name	          (MateVFSMimeApplication *app);
gboolean		 mate_vfs_mime_application_supports_uris	          (MateVFSMimeApplication *app);
gboolean		 mate_vfs_mime_application_requires_terminal             (MateVFSMimeApplication *app);
gboolean		 mate_vfs_mime_application_supports_startup_notification (MateVFSMimeApplication *app);
const char		*mate_vfs_mime_application_get_startup_wm_class          (MateVFSMimeApplication *app);
MateVFSMimeApplication *mate_vfs_mime_application_copy                          (MateVFSMimeApplication *application);
gboolean		 mate_vfs_mime_application_equal		          (MateVFSMimeApplication *app_a,
									           MateVFSMimeApplication *app_b);
void                     mate_vfs_mime_application_free                          (MateVFSMimeApplication *application);

/* Lists */

void                     mate_vfs_mime_application_list_free                     (GList                   *list);

#ifdef __cplusplus
}
#endif

#ifndef MATE_VFS_DISABLE_DEPRECATED
#include <libmatevfs/mate-vfs-mime-deprecated.h>
#endif

#endif /* MATE_VFS_MIME_HANDLERS_H */
