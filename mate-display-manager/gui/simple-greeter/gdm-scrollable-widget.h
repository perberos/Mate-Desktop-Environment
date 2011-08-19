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
 *  Written by: Ray Strode <rstrode@redhat.com>
 */

#ifndef __GDM_SCROLLABLE_WIDGET_H
#define __GDM_SCROLLABLE_WIDGET_H

#include <glib-object.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GDM_TYPE_SCROLLABLE_WIDGET         (gdm_scrollable_widget_get_type ())
#define GDM_SCROLLABLE_WIDGET(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDM_TYPE_SCROLLABLE_WIDGET, GdmScrollableWidget))
#define GDM_SCROLLABLE_WIDGET_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GDM_TYPE_SCROLLABLE_WIDGET, GdmScrollableWidgetClass))
#define GDM_IS_SCROLLABLE_WIDGET(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDM_TYPE_SCROLLABLE_WIDGET))
#define GDM_IS_SCROLLABLE_WIDGET_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDM_TYPE_SCROLLABLE_WIDGET))
#define GDM_SCROLLABLE_WIDGET_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDM_TYPE_SCROLLABLE_WIDGET, GdmScrollableWidgetClass))

typedef struct GdmScrollableWidgetClass GdmScrollableWidgetClass;
typedef struct GdmScrollableWidget GdmScrollableWidget;
typedef struct GdmScrollableWidgetPrivate GdmScrollableWidgetPrivate;
typedef void (* GdmScrollableWidgetSlideStepFunc) (GdmScrollableWidget *scrollable_widget,
                                                   double               progress,
                                                   int                 *new_height,
                                                   gpointer            *user_data);
typedef void (* GdmScrollableWidgetSlideDoneFunc) (GdmScrollableWidget *scrollable_widget,
                                                   gpointer            *user_data);

struct GdmScrollableWidget
{
        GtkBin                      parent;
        GdmScrollableWidgetPrivate *priv;
};

struct GdmScrollableWidgetClass
{
        GtkBinClass parent_class;

        void (* scroll_child) (GdmScrollableWidget *widget, GtkScrollType type);
        void (* move_focus_out) (GdmScrollableWidget *widget, GtkDirectionType type);

};

GType                  gdm_scrollable_widget_get_type               (void);
GtkWidget *            gdm_scrollable_widget_new                    (void);
void                   gdm_scrollable_widget_stop_sliding           (GdmScrollableWidget *widget);
void                   gdm_scrollable_widget_slide_to_height        (GdmScrollableWidget *widget,
                                                                     int                  height,
                                                                     GdmScrollableWidgetSlideStepFunc step_func,
                                                                     gpointer             step_user_data,
                                                                     GdmScrollableWidgetSlideDoneFunc done_func,
                                                                     gpointer             data);
gboolean               gdm_scrollable_widget_has_queued_key_events (GdmScrollableWidget *widget);
void                   gdm_scrollable_widget_replay_queued_key_events (GdmScrollableWidget *widget);
#endif /* __GDM_SCROLLABLE_WIDGET_H */
