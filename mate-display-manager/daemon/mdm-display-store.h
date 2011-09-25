/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
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


#ifndef __MDM_DISPLAY_STORE_H
#define __MDM_DISPLAY_STORE_H

#include <glib-object.h>
#include "mdm-display.h"

G_BEGIN_DECLS

#define MDM_TYPE_DISPLAY_STORE         (mdm_display_store_get_type ())
#define MDM_DISPLAY_STORE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_DISPLAY_STORE, MdmDisplayStore))
#define MDM_DISPLAY_STORE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_DISPLAY_STORE, MdmDisplayStoreClass))
#define MDM_IS_DISPLAY_STORE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_DISPLAY_STORE))
#define MDM_IS_DISPLAY_STORE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_DISPLAY_STORE))
#define MDM_DISPLAY_STORE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_DISPLAY_STORE, MdmDisplayStoreClass))

typedef struct MdmDisplayStorePrivate MdmDisplayStorePrivate;

typedef struct
{
        GObject                 parent;
        MdmDisplayStorePrivate *priv;
} MdmDisplayStore;

typedef struct
{
        GObjectClass   parent_class;

        void          (* display_added)    (MdmDisplayStore *display_store,
                                            const char      *id);
        void          (* display_removed)  (MdmDisplayStore *display_store,
                                            const char      *id);
} MdmDisplayStoreClass;

typedef enum
{
         MDM_DISPLAY_STORE_ERROR_GENERAL
} MdmDisplayStoreError;

#define MDM_DISPLAY_STORE_ERROR mdm_display_store_error_quark ()

typedef gboolean (*MdmDisplayStoreFunc) (const char *id,
                                         MdmDisplay *display,
                                         gpointer    user_data);

GQuark              mdm_display_store_error_quark              (void);
GType               mdm_display_store_get_type                 (void);

MdmDisplayStore *   mdm_display_store_new                      (void);

void                mdm_display_store_add                      (MdmDisplayStore    *store,
                                                                MdmDisplay         *display);
void                mdm_display_store_clear                    (MdmDisplayStore    *store);
gboolean            mdm_display_store_remove                   (MdmDisplayStore    *store,
                                                                MdmDisplay         *display);
void                mdm_display_store_foreach                  (MdmDisplayStore    *store,
                                                                MdmDisplayStoreFunc func,
                                                                gpointer            user_data);
guint               mdm_display_store_foreach_remove           (MdmDisplayStore    *store,
                                                                MdmDisplayStoreFunc func,
                                                                gpointer            user_data);
MdmDisplay *        mdm_display_store_find                     (MdmDisplayStore    *store,
                                                                MdmDisplayStoreFunc predicate,
                                                                gpointer            user_data);


G_END_DECLS

#endif /* __MDM_DISPLAY_STORE_H */
