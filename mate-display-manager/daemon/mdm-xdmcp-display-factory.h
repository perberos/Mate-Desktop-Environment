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


#ifndef __MDM_XDMCP_DISPLAY_FACTORY_H
#define __MDM_XDMCP_DISPLAY_FACTORY_H

#include <glib-object.h>

#include "mdm-display-factory.h"
#include "mdm-display-store.h"

G_BEGIN_DECLS

#define MDM_TYPE_XDMCP_DISPLAY_FACTORY         (mdm_xdmcp_display_factory_get_type ())
#define MDM_XDMCP_DISPLAY_FACTORY(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_XDMCP_DISPLAY_FACTORY, MdmXdmcpDisplayFactory))
#define MDM_XDMCP_DISPLAY_FACTORY_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_XDMCP_DISPLAY_FACTORY, MdmXdmcpDisplayFactoryClass))
#define MDM_IS_XDMCP_DISPLAY_FACTORY(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_XDMCP_DISPLAY_FACTORY))
#define MDM_IS_XDMCP_DISPLAY_FACTORY_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_XDMCP_DISPLAY_FACTORY))
#define MDM_XDMCP_DISPLAY_FACTORY_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_XDMCP_DISPLAY_FACTORY, MdmXdmcpDisplayFactoryClass))

typedef struct MdmXdmcpDisplayFactoryPrivate MdmXdmcpDisplayFactoryPrivate;

typedef struct
{
        MdmDisplayFactory              parent;
        MdmXdmcpDisplayFactoryPrivate *priv;
} MdmXdmcpDisplayFactory;

typedef struct
{
        MdmDisplayFactoryClass   parent_class;
} MdmXdmcpDisplayFactoryClass;

typedef enum
{
         MDM_XDMCP_DISPLAY_FACTORY_ERROR_GENERAL
} MdmXdmcpDisplayFactoryError;

#define MDM_XDMCP_DISPLAY_FACTORY_ERROR mdm_xdmcp_display_factory_error_quark ()

GQuark                     mdm_xdmcp_display_factory_error_quark      (void);
GType                      mdm_xdmcp_display_factory_get_type         (void);

MdmXdmcpDisplayFactory *   mdm_xdmcp_display_factory_new              (MdmDisplayStore        *display_store);

void                       mdm_xdmcp_display_factory_set_port         (MdmXdmcpDisplayFactory *manager,
                                                                       guint                   port);

G_END_DECLS

#endif /* __MDM_XDMCP_DISPLAY_FACTORY_H */
