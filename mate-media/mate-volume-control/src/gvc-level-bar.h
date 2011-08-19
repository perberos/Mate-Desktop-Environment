/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 William Jon McCann <william.jon.mccann@gmail.com>
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

#ifndef __GVC_LEVEL_BAR_H
#define __GVC_LEVEL_BAR_H

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GVC_TYPE_LEVEL_BAR         (gvc_level_bar_get_type ())
#define GVC_LEVEL_BAR(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GVC_TYPE_LEVEL_BAR, GvcLevelBar))
#define GVC_LEVEL_BAR_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GVC_TYPE_LEVEL_BAR, GvcLevelBarClass))
#define GVC_IS_LEVEL_BAR(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GVC_TYPE_LEVEL_BAR))
#define GVC_IS_LEVEL_BAR_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GVC_TYPE_LEVEL_BAR))
#define GVC_LEVEL_BAR_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GVC_TYPE_LEVEL_BAR, GvcLevelBarClass))

typedef struct GvcLevelBarPrivate GvcLevelBarPrivate;

typedef struct
{
        GtkHBox             parent;
        GvcLevelBarPrivate *priv;
} GvcLevelBar;

typedef struct
{
        GtkHBoxClass        parent_class;
} GvcLevelBarClass;

typedef enum
{
    GVC_LEVEL_SCALE_LINEAR,
    GVC_LEVEL_SCALE_LOG,
    GVC_LEVEL_SCALE_LAST
} GvcLevelScale;

GType               gvc_level_bar_get_type            (void);

GtkWidget *         gvc_level_bar_new                 (void);
void                gvc_level_bar_set_orientation     (GvcLevelBar   *bar,
                                                       GtkOrientation orientation);
GtkOrientation      gvc_level_bar_get_orientation     (GvcLevelBar   *bar);

void                gvc_level_bar_set_peak_adjustment (GvcLevelBar   *bar,
                                                       GtkAdjustment *adjustment);
GtkAdjustment *     gvc_level_bar_get_peak_adjustment (GvcLevelBar   *bar);
void                gvc_level_bar_set_rms_adjustment  (GvcLevelBar   *bar,
                                                       GtkAdjustment *adjustment);
GtkAdjustment *     gvc_level_bar_get_rms_adjustment  (GvcLevelBar   *bar);
void                gvc_level_bar_set_scale           (GvcLevelBar   *bar,
                                                       GvcLevelScale  scale);


G_END_DECLS

#endif /* __GVC_LEVEL_BAR_H */
