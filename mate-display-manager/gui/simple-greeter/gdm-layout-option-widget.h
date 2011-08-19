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

#ifndef __GDM_LAYOUT_OPTION_WIDGET_H
#define __GDM_LAYOUT_OPTION_WIDGET_H

#include <glib-object.h>

#include "gdm-recent-option-widget.h"

G_BEGIN_DECLS

#define GDM_TYPE_LAYOUT_OPTION_WIDGET         (gdm_layout_option_widget_get_type ())
#define GDM_LAYOUT_OPTION_WIDGET(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDM_TYPE_LAYOUT_OPTION_WIDGET, GdmLayoutOptionWidget))
#define GDM_LAYOUT_OPTION_WIDGET_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GDM_TYPE_LAYOUT_OPTION_WIDGET, GdmLayoutOptionWidgetClass))
#define GDM_IS_LAYOUT_OPTION_WIDGET(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDM_TYPE_LAYOUT_OPTION_WIDGET))
#define GDM_IS_LAYOUT_OPTION_WIDGET_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDM_TYPE_LAYOUT_OPTION_WIDGET))
#define GDM_LAYOUT_OPTION_WIDGET_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDM_TYPE_LAYOUT_OPTION_WIDGET, GdmLayoutOptionWidgetClass))

typedef struct GdmLayoutOptionWidgetPrivate GdmLayoutOptionWidgetPrivate;

typedef struct
{
        GdmRecentOptionWidget          parent;
        GdmLayoutOptionWidgetPrivate *priv;
} GdmLayoutOptionWidget;

typedef struct
{
        GdmRecentOptionWidgetClass    parent_class;

        void (* layout_activated)        (GdmLayoutOptionWidget *widget);
} GdmLayoutOptionWidgetClass;

GType                  gdm_layout_option_widget_get_type               (void);
GtkWidget *            gdm_layout_option_widget_new                    (void);

char *                 gdm_layout_option_widget_get_current_layout_name      (GdmLayoutOptionWidget *widget);
void                   gdm_layout_option_widget_set_current_layout_name      (GdmLayoutOptionWidget *widget,
                                                                                  const char              *name);



#endif /* __GDM_LAYOUT_OPTION_WIDGET_H */
