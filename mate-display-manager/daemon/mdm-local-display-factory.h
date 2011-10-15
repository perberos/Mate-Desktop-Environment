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


#ifndef __MDM_LOCAL_DISPLAY_FACTORY_H
#define __MDM_LOCAL_DISPLAY_FACTORY_H

#include <glib-object.h>

#include "mdm-display-factory.h"
#include "mdm-display-store.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MDM_TYPE_LOCAL_DISPLAY_FACTORY         (mdm_local_display_factory_get_type ())
#define MDM_LOCAL_DISPLAY_FACTORY(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_LOCAL_DISPLAY_FACTORY, MdmLocalDisplayFactory))
#define MDM_LOCAL_DISPLAY_FACTORY_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_LOCAL_DISPLAY_FACTORY, MdmLocalDisplayFactoryClass))
#define MDM_IS_LOCAL_DISPLAY_FACTORY(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_LOCAL_DISPLAY_FACTORY))
#define MDM_IS_LOCAL_DISPLAY_FACTORY_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_LOCAL_DISPLAY_FACTORY))
#define MDM_LOCAL_DISPLAY_FACTORY_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_LOCAL_DISPLAY_FACTORY, MdmLocalDisplayFactoryClass))

typedef struct MdmLocalDisplayFactoryPrivate MdmLocalDisplayFactoryPrivate;

typedef struct
{
        MdmDisplayFactory              parent;
        MdmLocalDisplayFactoryPrivate *priv;
} MdmLocalDisplayFactory;

typedef struct
{
        MdmDisplayFactoryClass   parent_class;
} MdmLocalDisplayFactoryClass;

typedef enum
{
         MDM_LOCAL_DISPLAY_FACTORY_ERROR_GENERAL
} MdmLocalDisplayFactoryError;

#define MDM_LOCAL_DISPLAY_FACTORY_ERROR mdm_local_display_factory_error_quark ()

GQuark                     mdm_local_display_factory_error_quark              (void);
GType                      mdm_local_display_factory_get_type                 (void);

MdmLocalDisplayFactory *   mdm_local_display_factory_new                      (MdmDisplayStore        *display_store);

gboolean                   mdm_local_display_factory_create_transient_display (MdmLocalDisplayFactory *factory,
                                                                               char                  **id,
                                                                               GError                **error);

gboolean                   mdm_local_display_factory_create_product_display   (MdmLocalDisplayFactory *factory,
                                                                               const char             *parent_display_id,
                                                                               const char             *relay_address,
                                                                               char                  **id,
                                                                               GError                **error);

#ifdef __cplusplus
}
#endif

#endif /* __MDM_LOCAL_DISPLAY_FACTORY_H */
