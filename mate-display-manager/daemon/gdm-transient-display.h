/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 William Jon McCann <jmccann@redhat.com>
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


#ifndef __GDM_TRANSIENT_DISPLAY_H
#define __GDM_TRANSIENT_DISPLAY_H

#include <glib-object.h>
#include <dbus/dbus-glib.h>
#include "gdm-display.h"

G_BEGIN_DECLS

#define GDM_TYPE_TRANSIENT_DISPLAY         (gdm_transient_display_get_type ())
#define GDM_TRANSIENT_DISPLAY(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDM_TYPE_TRANSIENT_DISPLAY, GdmTransientDisplay))
#define GDM_TRANSIENT_DISPLAY_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GDM_TYPE_TRANSIENT_DISPLAY, GdmTransientDisplayClass))
#define GDM_IS_TRANSIENT_DISPLAY(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDM_TYPE_TRANSIENT_DISPLAY))
#define GDM_IS_TRANSIENT_DISPLAY_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDM_TYPE_TRANSIENT_DISPLAY))
#define GDM_TRANSIENT_DISPLAY_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDM_TYPE_TRANSIENT_DISPLAY, GdmTransientDisplayClass))

typedef struct GdmTransientDisplayPrivate GdmTransientDisplayPrivate;

typedef struct
{
        GdmDisplay                  parent;
        GdmTransientDisplayPrivate *priv;
} GdmTransientDisplay;

typedef struct
{
        GdmDisplayClass   parent_class;

} GdmTransientDisplayClass;

GType               gdm_transient_display_get_type                (void);
GdmDisplay *        gdm_transient_display_new                     (int display_number);


G_END_DECLS

#endif /* __GDM_TRANSIENT_DISPLAY_H */
