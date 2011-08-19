/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-volume-hole.h
 *
 * Copyright (C) 2007 David Zeuthen
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

#ifndef __GDU_VOLUME_HOLE_H
#define __GDU_VOLUME_HOLE_H

#include <gdu/gdu-types.h>

G_BEGIN_DECLS

#define GDU_TYPE_VOLUME_HOLE           (gdu_volume_hole_get_type ())
#define GDU_VOLUME_HOLE(o)             (G_TYPE_CHECK_INSTANCE_CAST ((o), GDU_TYPE_VOLUME_HOLE, GduVolumeHole))
#define GDU_VOLUME_HOLE_CLASS(k)       (G_TYPE_CHECK_CLASS_CAST ((k), GDU_VOLUME_HOLE,  GduVolumeHoleClass))
#define GDU_IS_VOLUME_HOLE(o)          (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDU_TYPE_VOLUME_HOLE))
#define GDU_IS_VOLUME_HOLE_CLASS(k)    (G_TYPE_CHECK_CLASS_TYPE ((k), GDU_TYPE_VOLUME_HOLE))
#define GDU_VOLUME_HOLE_GET_CLASS(k)   (G_TYPE_INSTANCE_GET_CLASS ((k), GDU_TYPE_VOLUME_HOLE, GduVolumeHoleClass))

typedef struct _GduVolumeHoleClass       GduVolumeHoleClass;
typedef struct _GduVolumeHolePrivate     GduVolumeHolePrivate;

struct _GduVolumeHole
{
        GObject parent;

        /* private */
        GduVolumeHolePrivate *priv;
};

struct _GduVolumeHoleClass
{
        GObjectClass parent_class;
};

GType            gdu_volume_hole_get_type  (void);

G_END_DECLS

#endif /* __GDU_VOLUME_HOLE_H */
