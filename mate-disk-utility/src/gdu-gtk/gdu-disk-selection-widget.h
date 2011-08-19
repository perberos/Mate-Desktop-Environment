/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2009 Red Hat, Inc.
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
 *
 * Author: David Zeuthen <davidz@redhat.com>
 */

#ifndef __GDU_DISK_SELECTION_WIDGET_H
#define __GDU_DISK_SELECTION_WIDGET_H

#include <gdu-gtk/gdu-gtk.h>

G_BEGIN_DECLS

#define GDU_TYPE_DISK_SELECTION_WIDGET         (gdu_disk_selection_widget_get_type())
#define GDU_DISK_SELECTION_WIDGET(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDU_TYPE_DISK_SELECTION_WIDGET, GduDiskSelectionWidget))
#define GDU_DISK_SELECTION_WIDGET_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GDU_TYPE_DISK_SELECTION_WIDGET, GduDiskSelectionWidgetClass))
#define GDU_IS_DISK_SELECTION_WIDGET(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDU_TYPE_DISK_SELECTION_WIDGET))
#define GDU_IS_DISK_SELECTION_WIDGET_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDU_TYPE_DISK_SELECTION_WIDGET))
#define GDU_DISK_SELECTION_WIDGET_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDU_TYPE_DISK_SELECTION_WIDGET, GduDiskSelectionWidgetClass))

typedef struct GduDiskSelectionWidgetClass   GduDiskSelectionWidgetClass;
typedef struct GduDiskSelectionWidgetPrivate GduDiskSelectionWidgetPrivate;

struct GduDiskSelectionWidget
{
        GtkVBox parent;

        /*< private >*/
        GduDiskSelectionWidgetPrivate *priv;
};

struct GduDiskSelectionWidgetClass
{
        GtkVBoxClass parent_class;

        void (*changed) (GduDiskSelectionWidget *widget);

        gchar *(*is_drive_ignored) (GduDiskSelectionWidget *widget,
                                    GduDrive               *drive);
};

GType       gdu_disk_selection_widget_get_type                         (void) G_GNUC_CONST;
GtkWidget  *gdu_disk_selection_widget_new                              (GduPool                     *pool,
                                                                        GduDiskSelectionWidgetFlags  flags);
GPtrArray  *gdu_disk_selection_widget_get_selected_drives              (GduDiskSelectionWidget      *widget);
guint64     gdu_disk_selection_widget_get_component_size               (GduDiskSelectionWidget      *widget);
void        gdu_disk_selection_widget_set_component_size               (GduDiskSelectionWidget      *widget,
                                                                        guint64                      component_size);
guint       gdu_disk_selection_widget_get_num_available_disks          (GduDiskSelectionWidget      *widget);
guint64     gdu_disk_selection_widget_get_largest_segment_for_all      (GduDiskSelectionWidget      *widget);
guint64     gdu_disk_selection_widget_get_largest_segment_for_selected (GduDiskSelectionWidget      *widget);

G_END_DECLS

#endif  /* __GDU_DISK_SELECTION_WIDGET_H */

