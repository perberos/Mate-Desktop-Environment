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


#ifndef __MDM_DISPLAY_FACTORY_H
#define __MDM_DISPLAY_FACTORY_H

#include <glib-object.h>

#include "mdm-display-store.h"

G_BEGIN_DECLS

#define MDM_TYPE_DISPLAY_FACTORY         (mdm_display_factory_get_type ())
#define MDM_DISPLAY_FACTORY(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_DISPLAY_FACTORY, MdmDisplayFactory))
#define MDM_DISPLAY_FACTORY_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_DISPLAY_FACTORY, MdmDisplayFactoryClass))
#define MDM_IS_DISPLAY_FACTORY(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_DISPLAY_FACTORY))
#define MDM_IS_DISPLAY_FACTORY_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_DISPLAY_FACTORY))
#define MDM_DISPLAY_FACTORY_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_DISPLAY_FACTORY, MdmDisplayFactoryClass))

typedef struct MdmDisplayFactoryPrivate MdmDisplayFactoryPrivate;

typedef struct
{
        GObject                   parent;
        MdmDisplayFactoryPrivate *priv;
} MdmDisplayFactory;

typedef struct
{
        GObjectClass   parent_class;

        gboolean (*start)                  (MdmDisplayFactory *factory);
        gboolean (*stop)                   (MdmDisplayFactory *factory);
} MdmDisplayFactoryClass;

typedef enum
{
         MDM_DISPLAY_FACTORY_ERROR_GENERAL
} MdmDisplayFactoryError;

#define MDM_DISPLAY_FACTORY_ERROR mdm_display_factory_error_quark ()

GQuark                     mdm_display_factory_error_quark             (void);
GType                      mdm_display_factory_get_type                (void);

gboolean                   mdm_display_factory_start                   (MdmDisplayFactory *manager);
gboolean                   mdm_display_factory_stop                    (MdmDisplayFactory *manager);
MdmDisplayStore *          mdm_display_factory_get_display_store       (MdmDisplayFactory *manager);

G_END_DECLS

#endif /* __MDM_DISPLAY_FACTORY_H */
