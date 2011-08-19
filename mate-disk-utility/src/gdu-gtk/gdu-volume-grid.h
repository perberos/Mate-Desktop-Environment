/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-grid-element.h
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

#if !defined (__GDU_GTK_INSIDE_GDU_GTK_H) && !defined (GDU_GTK_COMPILATION)
#error "Only <gdu-gtk/gdu-gtk.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef __GDU_VOLUME_GRID_H
#define __GDU_VOLUME_GRID_H

#include <gdu-gtk/gdu-gtk-types.h>

G_BEGIN_DECLS

#define GDU_TYPE_VOLUME_GRID         gdu_volume_grid_get_type()
#define GDU_VOLUME_GRID(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDU_TYPE_VOLUME_GRID, GduVolumeGrid))
#define GDU_VOLUME_GRID_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GDU_TYPE_VOLUME_GRID, GduVolumeGridClass))
#define GDU_IS_VOLUME_GRID(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDU_TYPE_VOLUME_GRID))
#define GDU_IS_VOLUME_GRID_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDU_TYPE_VOLUME_GRID))
#define GDU_VOLUME_GRID_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDU_TYPE_VOLUME_GRID, GduVolumeGridClass))

typedef struct GduVolumeGridClass   GduVolumeGridClass;
typedef struct GduVolumeGridPrivate GduVolumeGridPrivate;

struct GduVolumeGrid
{
        GtkDrawingArea parent;

        /*< private >*/
        GduVolumeGridPrivate *priv;
};

struct GduVolumeGridClass
{
        GtkDrawingAreaClass parent_class;

        /* signals */
        void (*changed) (GduVolumeGrid *grid);
};

GType           gdu_volume_grid_get_type         (void) G_GNUC_CONST;
GtkWidget*      gdu_volume_grid_new              (GduDrive            *drive);
GduPresentable *gdu_volume_grid_get_selected     (GduVolumeGrid       *grid);
gboolean        gdu_volume_grid_select           (GduVolumeGrid       *grid,
                                                  GduPresentable      *volume);

G_END_DECLS



#endif /* __GDU_VOLUME_GRID_H */
