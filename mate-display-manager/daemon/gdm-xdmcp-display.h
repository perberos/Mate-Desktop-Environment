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


#ifndef __GDM_XDMCP_DISPLAY_H
#define __GDM_XDMCP_DISPLAY_H

#include <sys/types.h>
#include <sys/socket.h>
#include <glib-object.h>
#include <dbus/dbus-glib.h>

#include "gdm-display.h"
#include "gdm-address.h"

G_BEGIN_DECLS

#define GDM_TYPE_XDMCP_DISPLAY         (gdm_xdmcp_display_get_type ())
#define GDM_XDMCP_DISPLAY(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDM_TYPE_XDMCP_DISPLAY, GdmXdmcpDisplay))
#define GDM_XDMCP_DISPLAY_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GDM_TYPE_XDMCP_DISPLAY, GdmXdmcpDisplayClass))
#define GDM_IS_XDMCP_DISPLAY(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDM_TYPE_XDMCP_DISPLAY))
#define GDM_IS_XDMCP_DISPLAY_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDM_TYPE_XDMCP_DISPLAY))
#define GDM_XDMCP_DISPLAY_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDM_TYPE_XDMCP_DISPLAY, GdmXdmcpDisplayClass))

typedef struct GdmXdmcpDisplayPrivate GdmXdmcpDisplayPrivate;

typedef struct
{
        GdmDisplay              parent;
        GdmXdmcpDisplayPrivate *priv;
} GdmXdmcpDisplay;

typedef struct
{
        GdmDisplayClass   parent_class;

} GdmXdmcpDisplayClass;

GType                     gdm_xdmcp_display_get_type                 (void);

gint32                    gdm_xdmcp_display_get_session_number       (GdmXdmcpDisplay         *display);
GdmAddress              * gdm_xdmcp_display_get_remote_address       (GdmXdmcpDisplay         *display);


G_END_DECLS

#endif /* __GDM_XDMCP_DISPLAY_H */
