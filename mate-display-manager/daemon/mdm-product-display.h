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


#ifndef __MDM_PRODUCT_DISPLAY_H
#define __MDM_PRODUCT_DISPLAY_H

#include <glib-object.h>
#include <dbus/dbus-glib.h>
#include "mdm-display.h"

G_BEGIN_DECLS

#define MDM_TYPE_PRODUCT_DISPLAY         (mdm_product_display_get_type ())
#define MDM_PRODUCT_DISPLAY(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_PRODUCT_DISPLAY, MdmProductDisplay))
#define MDM_PRODUCT_DISPLAY_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_PRODUCT_DISPLAY, MdmProductDisplayClass))
#define MDM_IS_PRODUCT_DISPLAY(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_PRODUCT_DISPLAY))
#define MDM_IS_PRODUCT_DISPLAY_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_PRODUCT_DISPLAY))
#define MDM_PRODUCT_DISPLAY_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_PRODUCT_DISPLAY, MdmProductDisplayClass))

typedef struct MdmProductDisplayPrivate MdmProductDisplayPrivate;

typedef struct
{
        MdmDisplay               parent;
        MdmProductDisplayPrivate *priv;
} MdmProductDisplay;

typedef struct
{
        MdmDisplayClass   parent_class;

} MdmProductDisplayClass;

GType               mdm_product_display_get_type                   (void);
MdmDisplay *        mdm_product_display_new                        (int         display_number,
                                                                    const char *server_address);

gboolean            mdm_product_display_get_relay_address          (MdmProductDisplay *display,
                                                                    char             **address,
                                                                    GError           **error);


G_END_DECLS

#endif /* __MDM_PRODUCT_DISPLAY_H */
