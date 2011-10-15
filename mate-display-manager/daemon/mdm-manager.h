/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2006 William Jon McCann <mccann@jhu.edu>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */


#ifndef __MDM_MANAGER_H
#define __MDM_MANAGER_H

#include <glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MDM_TYPE_MANAGER         (mdm_manager_get_type ())
#define MDM_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_MANAGER, MdmManager))
#define MDM_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_MANAGER, MdmManagerClass))
#define MDM_IS_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_MANAGER))
#define MDM_IS_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_MANAGER))
#define MDM_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_MANAGER, MdmManagerClass))

typedef struct MdmManagerPrivate MdmManagerPrivate;

typedef struct
{
        GObject           parent;
        MdmManagerPrivate *priv;
} MdmManager;

typedef struct
{
        GObjectClass   parent_class;

        void          (* display_added)    (MdmManager      *manager,
                                            const char      *id);
        void          (* display_removed)  (MdmManager      *manager,
                                            const char      *id);
} MdmManagerClass;

typedef enum
{
         MDM_MANAGER_ERROR_GENERAL
} MdmManagerError;

#define MDM_MANAGER_ERROR mdm_manager_error_quark ()

GQuark              mdm_manager_error_quark                    (void);
GType               mdm_manager_get_type                       (void);

MdmManager *        mdm_manager_new                            (void);
void                mdm_manager_start                          (MdmManager *manager);
void                mdm_manager_stop                           (MdmManager *manager);
void                mdm_manager_set_wait_for_go                (MdmManager *manager,
                                                                gboolean    wait_for_go);

void                mdm_manager_set_xdmcp_enabled              (MdmManager *manager,
                                                                gboolean    enabled);
gboolean            mdm_manager_get_displays                   (MdmManager *manager,
                                                                GPtrArray **displays,
                                                                GError    **error);


#ifdef __cplusplus
}
#endif

#endif /* __MDM_MANAGER_H */
