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

#ifndef __GVC_CHANNEL_BAR_H
#define __GVC_CHANNEL_BAR_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GVC_TYPE_CHANNEL_BAR         (gvc_channel_bar_get_type ())
#define GVC_CHANNEL_BAR(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GVC_TYPE_CHANNEL_BAR, GvcChannelBar))
#define GVC_CHANNEL_BAR_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GVC_TYPE_CHANNEL_BAR, GvcChannelBarClass))
#define GVC_IS_CHANNEL_BAR(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GVC_TYPE_CHANNEL_BAR))
#define GVC_IS_CHANNEL_BAR_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GVC_TYPE_CHANNEL_BAR))
#define GVC_CHANNEL_BAR_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GVC_TYPE_CHANNEL_BAR, GvcChannelBarClass))

typedef struct GvcChannelBarPrivate GvcChannelBarPrivate;

typedef struct
{
        GtkHBox               parent;
        GvcChannelBarPrivate *priv;
} GvcChannelBar;

typedef struct
{
        GtkHBoxClass          parent_class;
} GvcChannelBarClass;

GType               gvc_channel_bar_get_type            (void);

GtkWidget *         gvc_channel_bar_new                 (void);

void                gvc_channel_bar_set_name            (GvcChannelBar *bar,
                                                         const char    *name);
void                gvc_channel_bar_set_icon_name       (GvcChannelBar *bar,
                                                         const char    *icon_name);
void                gvc_channel_bar_set_low_icon_name   (GvcChannelBar *bar,
                                                         const char    *icon_name);
void                gvc_channel_bar_set_high_icon_name  (GvcChannelBar *bar,
                                                         const char    *icon_name);

void                gvc_channel_bar_set_orientation     (GvcChannelBar *bar,
                                                         GtkOrientation orientation);
GtkOrientation      gvc_channel_bar_get_orientation     (GvcChannelBar *bar);

GtkAdjustment *     gvc_channel_bar_get_adjustment      (GvcChannelBar *bar);

gboolean            gvc_channel_bar_get_is_muted        (GvcChannelBar *bar);
void                gvc_channel_bar_set_is_muted        (GvcChannelBar *bar,
                                                         gboolean       is_muted);
gboolean            gvc_channel_bar_get_show_mute       (GvcChannelBar *bar);
void                gvc_channel_bar_set_show_mute       (GvcChannelBar *bar,
                                                         gboolean       show_mute);
void                gvc_channel_bar_set_size_group      (GvcChannelBar *bar,
                                                         GtkSizeGroup  *group,
                                                         gboolean       symmetric);
void                gvc_channel_bar_set_is_amplified    (GvcChannelBar *bar,
                                                         gboolean amplified);
void                gvc_channel_bar_set_base_volume     (GvcChannelBar *bar,
                                                         guint32        base_volume);

gboolean            gvc_channel_bar_scroll              (GvcChannelBar *bar,
                                                         GdkScrollDirection direction);

G_END_DECLS

#endif /* __GVC_CHANNEL_BAR_H */
