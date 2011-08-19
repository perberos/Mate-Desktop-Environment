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


#ifndef __GDM_LOCAL_DISPLAY_FACTORY_H
#define __GDM_LOCAL_DISPLAY_FACTORY_H

#include <glib-object.h>

#include "gdm-display-factory.h"
#include "gdm-display-store.h"

G_BEGIN_DECLS

#define GDM_TYPE_LOCAL_DISPLAY_FACTORY         (gdm_local_display_factory_get_type ())
#define GDM_LOCAL_DISPLAY_FACTORY(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDM_TYPE_LOCAL_DISPLAY_FACTORY, GdmLocalDisplayFactory))
#define GDM_LOCAL_DISPLAY_FACTORY_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GDM_TYPE_LOCAL_DISPLAY_FACTORY, GdmLocalDisplayFactoryClass))
#define GDM_IS_LOCAL_DISPLAY_FACTORY(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDM_TYPE_LOCAL_DISPLAY_FACTORY))
#define GDM_IS_LOCAL_DISPLAY_FACTORY_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDM_TYPE_LOCAL_DISPLAY_FACTORY))
#define GDM_LOCAL_DISPLAY_FACTORY_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDM_TYPE_LOCAL_DISPLAY_FACTORY, GdmLocalDisplayFactoryClass))

typedef struct GdmLocalDisplayFactoryPrivate GdmLocalDisplayFactoryPrivate;

typedef struct
{
        GdmDisplayFactory              parent;
        GdmLocalDisplayFactoryPrivate *priv;
} GdmLocalDisplayFactory;

typedef struct
{
        GdmDisplayFactoryClass   parent_class;
} GdmLocalDisplayFactoryClass;

typedef enum
{
         GDM_LOCAL_DISPLAY_FACTORY_ERROR_GENERAL
} GdmLocalDisplayFactoryError;

#define GDM_LOCAL_DISPLAY_FACTORY_ERROR gdm_local_display_factory_error_quark ()

GQuark                     gdm_local_display_factory_error_quark              (void);
GType                      gdm_local_display_factory_get_type                 (void);

GdmLocalDisplayFactory *   gdm_local_display_factory_new                      (GdmDisplayStore        *display_store);

gboolean                   gdm_local_display_factory_create_transient_display (GdmLocalDisplayFactory *factory,
                                                                               char                  **id,
                                                                               GError                **error);

gboolean                   gdm_local_display_factory_create_product_display   (GdmLocalDisplayFactory *factory,
                                                                               const char             *parent_display_id,
                                                                               const char             *relay_address,
                                                                               char                  **id,
                                                                               GError                **error);

G_END_DECLS

#endif /* __GDM_LOCAL_DISPLAY_FACTORY_H */
