/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Red Hat, Inc.
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

#ifndef __GVC_APPLET_H
#define __GVC_APPLET_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GVC_TYPE_APPLET         (gvc_applet_get_type ())
#define GVC_APPLET(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GVC_TYPE_APPLET, GvcApplet))
#define GVC_APPLET_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GVC_TYPE_APPLET, GvcAppletClass))
#define GVC_IS_APPLET(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GVC_TYPE_APPLET))
#define GVC_IS_APPLET_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GVC_TYPE_APPLET))
#define GVC_APPLET_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GVC_TYPE_APPLET, GvcAppletClass))

typedef struct GvcAppletPrivate GvcAppletPrivate;

typedef struct
{
        GObject            parent;
        GvcAppletPrivate *priv;
} GvcApplet;

typedef struct
{
        GObjectClass   parent_class;
} GvcAppletClass;

GType               gvc_applet_get_type            (void);

GvcApplet *         gvc_applet_new                 (void);
void                gvc_applet_start               (GvcApplet     *applet);

G_END_DECLS

#endif /* __GVC_APPLET_H */
