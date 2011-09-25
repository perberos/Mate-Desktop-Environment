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

#ifndef __MDM_SCROLLABLE_WIDGET_H
#define __MDM_SCROLLABLE_WIDGET_H

#include <glib-object.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MDM_TYPE_SCROLLABLE_WIDGET         (mdm_scrollable_widget_get_type ())
#define MDM_SCROLLABLE_WIDGET(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_SCROLLABLE_WIDGET, MdmScrollableWidget))
#define MDM_SCROLLABLE_WIDGET_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_SCROLLABLE_WIDGET, MdmScrollableWidgetClass))
#define MDM_IS_SCROLLABLE_WIDGET(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_SCROLLABLE_WIDGET))
#define MDM_IS_SCROLLABLE_WIDGET_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_SCROLLABLE_WIDGET))
#define MDM_SCROLLABLE_WIDGET_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_SCROLLABLE_WIDGET, MdmScrollableWidgetClass))

typedef struct MdmScrollableWidgetClass MdmScrollableWidgetClass;
typedef struct MdmScrollableWidget MdmScrollableWidget;
typedef struct MdmScrollableWidgetPrivate MdmScrollableWidgetPrivate;
typedef void (* MdmScrollableWidgetSlideStepFunc) (MdmScrollableWidget *scrollable_widget,
                                                   double               progress,
                                                   int                 *new_height,
                                                   gpointer            *user_data);
typedef void (* MdmScrollableWidgetSlideDoneFunc) (MdmScrollableWidget *scrollable_widget,
                                                   gpointer            *user_data);

struct MdmScrollableWidget
{
        GtkBin                      parent;
        MdmScrollableWidgetPrivate *priv;
};

struct MdmScrollableWidgetClass
{
        GtkBinClass parent_class;

        void (* scroll_child) (MdmScrollableWidget *widget, GtkScrollType type);
        void (* move_focus_out) (MdmScrollableWidget *widget, GtkDirectionType type);

};

GType                  mdm_scrollable_widget_get_type               (void);
GtkWidget *            mdm_scrollable_widget_new                    (void);
void                   mdm_scrollable_widget_stop_sliding           (MdmScrollableWidget *widget);
void                   mdm_scrollable_widget_slide_to_height        (MdmScrollableWidget *widget,
                                                                     int                  height,
                                                                     MdmScrollableWidgetSlideStepFunc step_func,
                                                                     gpointer             step_user_data,
                                                                     MdmScrollableWidgetSlideDoneFunc done_func,
                                                                     gpointer             data);
gboolean               mdm_scrollable_widget_has_queued_key_events (MdmScrollableWidget *widget);
void                   mdm_scrollable_widget_replay_queued_key_events (MdmScrollableWidget *widget);
#endif /* __MDM_SCROLLABLE_WIDGET_H */
