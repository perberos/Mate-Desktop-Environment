/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/*
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

#ifndef MATE_VFS_DISABLE_DEPRECATED

#ifndef MATE_VFS_MIME_DEPRECATED_H
#define MATE_VFS_MIME_DEPRECATED_H

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------
 * From mate-vfs-mime-handlers.h
 * ------------------------------
 */

/**
 * MateVFSMimeActionType:
 * @MATE_VFS_MIME_ACTION_TYPE_NONE: neither an application nor a component.
 * @MATE_VFS_MIME_ACTION_TYPE_APPLICATION: an application.
 * @MATE_VFS_MIME_ACTION_TYPE_COMPONENT: a component.
 *
 * This is used to specify the %type of a #MateVFSMimeAction.
 **/
typedef enum {
	MATE_VFS_MIME_ACTION_TYPE_NONE,
	MATE_VFS_MIME_ACTION_TYPE_APPLICATION,
	MATE_VFS_MIME_ACTION_TYPE_COMPONENT
} MateVFSMimeActionType;

/**
 * MateVFSMimeAction:
 * @action_type: The #MateVFSMimeActionType describing the type of this action.
 *
 * This data structure describes an action that can be done
 * on a file.
 **/
typedef struct _MateVFSMimeAction MateVFSMimeAction;

struct _MateVFSMimeAction {
	/* <public >*/
	MateVFSMimeActionType action_type;
	union {
		void *component;
		void *dummy_component;
		MateVFSMimeApplication *application;
	} action;

	/*< private >*/
	/* Padded to avoid future breaks in ABI compatibility */
	void *reserved1;
};

MateVFSMimeActionType   mate_vfs_mime_get_default_action_type            (const char              *mime_type);
MateVFSMimeAction *     mate_vfs_mime_get_default_action                 (const char              *mime_type);
MateVFSMimeApplication *mate_vfs_mime_application_new_from_id            (const char              *id);
void                     mate_vfs_mime_action_free                        (MateVFSMimeAction      *action);

MateVFSResult           mate_vfs_mime_action_launch                      (MateVFSMimeAction      *action,
									    GList                   *uris);
MateVFSResult           mate_vfs_mime_action_launch_with_env             (MateVFSMimeAction      *action,
									    GList                   *uris,
									    char                   **envp);

const char  		*mate_vfs_mime_get_icon 			   (const char 		    *mime_type);

/* List manipulation helper functions */
gboolean                 mate_vfs_mime_id_in_application_list             (const char              *id,
									    GList                   *applications);
GList *                  mate_vfs_mime_remove_application_from_list       (GList                   *applications,
									    const char              *application_id,
									    gboolean                *did_remove);
GList *                  mate_vfs_mime_id_list_from_application_list      (GList                   *applications);

/* Stored as delta to current user level - API function computes delta and stores in prefs */
MateVFSResult           mate_vfs_mime_add_extension                      (const char              *mime_type,
									    const char              *extension);
MateVFSResult           mate_vfs_mime_remove_extension                   (const char              *mime_type,
									    const char              *extension);
MateVFSResult           mate_vfs_mime_set_default_action_type            (const char              *mime_type,
									    MateVFSMimeActionType   action_type);
MateVFSResult           mate_vfs_mime_set_default_application            (const char              *mime_type,
									    const char              *application_id);
MateVFSResult           mate_vfs_mime_set_default_component              (const char              *mime_type,
									    const char              *component_iid);
MateVFSResult  	 mate_vfs_mime_set_icon 			   (const char 		    *mime_type,
									    const char		    *filename);
MateVFSResult		 mate_vfs_mime_set_description			   (const char		    *mime_type,
									    const char		    *description);

MateVFSResult	 	 mate_vfs_mime_set_can_be_executable   	   (const char              *mime_type,
									    gboolean		     new_value);


/* No way to override system list; can only add. */
MateVFSResult           mate_vfs_mime_extend_all_applications            (const char              *mime_type,
									    GList                   *application_ids);
/* Only "user" entries may be removed. */
MateVFSResult           mate_vfs_mime_remove_from_all_applications       (const char              *mime_type,
									    GList                   *application_ids);


GList *                  mate_vfs_mime_get_short_list_applications        (const char              *mime_type);
MateVFSResult           mate_vfs_mime_set_short_list_applications        (const char              *mime_type,
									    GList                   *application_ids);
MateVFSResult           mate_vfs_mime_set_short_list_components          (const char              *mime_type,
									    GList                   *component_iids);
MateVFSResult           mate_vfs_mime_add_application_to_short_list      (const char              *mime_type,
									    const char              *application_id);
MateVFSResult           mate_vfs_mime_remove_application_from_short_list (const char              *mime_type,
									    const char              *application_id);
MateVFSResult           mate_vfs_mime_add_component_to_short_list        (const char              *mime_type,
									    const char              *iid);
MateVFSResult           mate_vfs_mime_remove_component_from_short_list   (const char              *mime_type,
									    const char              *iid);


/* There are actually in matecomponent-activation, but defined here */
#if defined(MATE_VFS_INCLUDE_MATECOMPONENT) || defined(_MateComponent_ServerInfo_defined)
#include <matecomponent-activation/matecomponent-activation-server-info.h>
MateComponent_ServerInfo *mate_vfs_mime_get_default_component (const char *mime_type);
#else
void *mate_vfs_mime_get_default_component (const char *mime_type);
#endif

GList *  mate_vfs_mime_get_all_components          (const char *mime_type);
void     mate_vfs_mime_component_list_free         (GList      *list);
GList *  mate_vfs_mime_remove_component_from_list  (GList      *components,
						     const char *iid,
						     gboolean   *did_remove);
GList *  mate_vfs_mime_id_list_from_component_list (GList      *components);
gboolean mate_vfs_mime_id_in_component_list        (const char *iid,
						     GList      *components);
GList *  mate_vfs_mime_get_short_list_components   (const char *mime_type);

#ifdef __cplusplus
}
#endif

#endif /* MATE_VFS_MIME_DEPRECATED_H */

#endif /* MATE_VFS_DISABLE_DEPRECATED */
