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


#ifndef __MDM_STATIC_FACTORY_DISPLAY_H
#define __MDM_STATIC_FACTORY_DISPLAY_H

#include <glib-object.h>
#include <dbus/dbus-glib.h>

#include "mdm-display.h"
#include "mdm-display-store.h"

G_BEGIN_DECLS

#define MDM_TYPE_STATIC_FACTORY_DISPLAY         (mdm_static_factory_display_get_type ())
#define MDM_STATIC_FACTORY_DISPLAY(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_STATIC_FACTORY_DISPLAY, MdmStaticFactoryDisplay))
#define MDM_STATIC_FACTORY_DISPLAY_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_STATIC_FACTORY_DISPLAY, MdmStaticFactoryDisplayClass))
#define MDM_IS_STATIC_FACTORY_DISPLAY(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_STATIC_FACTORY_DISPLAY))
#define MDM_IS_STATIC_FACTORY_DISPLAY_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_STATIC_FACTORY_DISPLAY))
#define MDM_STATIC_FACTORY_DISPLAY_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_STATIC_FACTORY_DISPLAY, MdmStaticFactoryDisplayClass))

typedef struct MdmStaticFactoryDisplayPrivate MdmStaticFactoryDisplayPrivate;

typedef struct
{
        MdmDisplay               parent;
        MdmStaticFactoryDisplayPrivate *priv;
} MdmStaticFactoryDisplay;

typedef struct
{
        MdmDisplayClass   parent_class;

} MdmStaticFactoryDisplayClass;

GType               mdm_static_factory_display_get_type                (void);
MdmDisplay *        mdm_static_factory_display_new                     (int                      display_number);

gboolean            mdm_static_factory_display_create_product_display  (MdmStaticFactoryDisplay *display,
                                                                        const char              *server_address,
                                                                        char                   **id,
                                                                        GError                 **error);

G_END_DECLS

#endif /* __MDM_STATIC_FACTORY_DISPLAY_H */
