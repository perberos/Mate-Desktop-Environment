/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-hub.h
 *
 * Copyright (C) 2009 David Zeuthen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#if !defined (__GDU_INSIDE_GDU_H) && !defined (GDU_COMPILATION)
#error "Only <gdu/gdu.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef __GDU_HUB_H
#define __GDU_HUB_H

#include <gdu/gdu-types.h>

G_BEGIN_DECLS

#define GDU_TYPE_HUB           (gdu_hub_get_type ())
#define GDU_HUB(o)             (G_TYPE_CHECK_INSTANCE_CAST ((o), GDU_TYPE_HUB, GduHub))
#define GDU_HUB_CLASS(k)       (G_TYPE_CHECK_CLASS_CAST ((k), GDU_HUB,  GduHubClass))
#define GDU_IS_HUB(o)          (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDU_TYPE_HUB))
#define GDU_IS_HUB_CLASS(k)    (G_TYPE_CHECK_CLASS_TYPE ((k), GDU_TYPE_HUB))
#define GDU_HUB_GET_CLASS(k)   (G_TYPE_INSTANCE_GET_CLASS ((k), GDU_TYPE_HUB, GduHubClass))

typedef struct _GduHubClass       GduHubClass;
typedef struct _GduHubPrivate     GduHubPrivate;

struct _GduHub
{
        GObject parent;

        /* private */
        GduHubPrivate *priv;
};

struct _GduHubClass
{
        GObjectClass parent_class;
};

GType        gdu_hub_get_type     (void);
GduAdapter  *gdu_hub_get_adapter  (GduHub *hub);
GduExpander *gdu_hub_get_expander (GduHub *hub);
GduHubUsage  gdu_hub_get_usage    (GduHub *hub);

G_END_DECLS

#endif /* __GDU_HUB_H */
