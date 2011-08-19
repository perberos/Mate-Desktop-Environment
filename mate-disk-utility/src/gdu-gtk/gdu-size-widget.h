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

#ifndef __GDU_SIZE_WIDGET_H
#define __GDU_SIZE_WIDGET_H

#include <gdu-gtk/gdu-gtk.h>

G_BEGIN_DECLS

#define GDU_TYPE_SIZE_WIDGET         (gdu_size_widget_get_type())
#define GDU_SIZE_WIDGET(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDU_TYPE_SIZE_WIDGET, GduSizeWidget))
#define GDU_SIZE_WIDGET_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GDU_TYPE_SIZE_WIDGET, GduSizeWidgetClass))
#define GDU_IS_SIZE_WIDGET(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDU_TYPE_SIZE_WIDGET))
#define GDU_IS_SIZE_WIDGET_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDU_TYPE_SIZE_WIDGET))
#define GDU_SIZE_WIDGET_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDU_TYPE_SIZE_WIDGET, GduSizeWidgetClass))

typedef struct GduSizeWidgetClass   GduSizeWidgetClass;
typedef struct GduSizeWidgetPrivate GduSizeWidgetPrivate;

struct GduSizeWidget
{
        GtkHBox parent;

        /*< private >*/
        GduSizeWidgetPrivate *priv;
};

struct GduSizeWidgetClass
{
        GtkHBoxClass parent_class;

        void (*changed) (GduSizeWidget *widget);
};

GType       gdu_size_widget_get_type     (void) G_GNUC_CONST;
GtkWidget*  gdu_size_widget_new          (guint64 size,
                                          guint64 min_size,
                                          guint64 max_size);
void        gdu_size_widget_set_size     (GduSizeWidget *widget,
                                          guint64        size);
void        gdu_size_widget_set_min_size (GduSizeWidget *widget,
                                          guint64        min_size);
void        gdu_size_widget_set_max_size (GduSizeWidget *widget,
                                          guint64        max_size);
guint64     gdu_size_widget_get_size     (GduSizeWidget *widget);
guint64     gdu_size_widget_get_min_size (GduSizeWidget *widget);
guint64     gdu_size_widget_get_max_size (GduSizeWidget *widget);

G_END_DECLS

#endif  /* __GDU_SIZE_WIDGET_H */

