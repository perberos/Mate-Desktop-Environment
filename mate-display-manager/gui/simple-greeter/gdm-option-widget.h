/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Red Hat, Inc.
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
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
 *              William Jon McCann <mccann@jhu.edu>
 */

#ifndef __GDM_OPTION_WIDGET_H
#define __GDM_OPTION_WIDGET_H

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GDM_TYPE_OPTION_WIDGET         (gdm_option_widget_get_type ())
#define GDM_OPTION_WIDGET(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDM_TYPE_OPTION_WIDGET, GdmOptionWidget))
#define GDM_OPTION_WIDGET_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GDM_TYPE_OPTION_WIDGET, GdmOptionWidgetClass))
#define GDM_IS_OPTION_WIDGET(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDM_TYPE_OPTION_WIDGET))
#define GDM_IS_OPTION_WIDGET_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDM_TYPE_OPTION_WIDGET))
#define GDM_OPTION_WIDGET_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDM_TYPE_OPTION_WIDGET, GdmOptionWidgetClass))

typedef struct GdmOptionWidgetPrivate GdmOptionWidgetPrivate;

typedef struct
{
        GtkAlignment             parent;
        GdmOptionWidgetPrivate *priv;
} GdmOptionWidget;

typedef struct
{
        GtkAlignmentClass       parent_class;

        void (* activated)      (GdmOptionWidget *widget);
} GdmOptionWidgetClass;

typedef enum {
        GDM_OPTION_WIDGET_POSITION_TOP = 0,
        GDM_OPTION_WIDGET_POSITION_MIDDLE,
        GDM_OPTION_WIDGET_POSITION_BOTTOM,
} GdmOptionWidgetPosition;

GType                  gdm_option_widget_get_type               (void);
GtkWidget *            gdm_option_widget_new                    (const char *label_text);

void                   gdm_option_widget_add_item               (GdmOptionWidget *widget,
                                                                 const char       *id,
                                                                 const char       *name,
                                                                 const char       *comment,
                                                                 GdmOptionWidgetPosition position);

void                   gdm_option_widget_remove_item            (GdmOptionWidget *widget,
                                                                 const char       *id);

void                   gdm_option_widget_remove_all_items       (GdmOptionWidget *widget);
gboolean               gdm_option_widget_lookup_item            (GdmOptionWidget *widget,
                                                                 const char       *id,
                                                                 char            **name,
                                                                 char            **comment,
                                                                 GdmOptionWidgetPosition *position);

char *                 gdm_option_widget_get_active_item        (GdmOptionWidget *widget);
void                   gdm_option_widget_set_active_item        (GdmOptionWidget *widget,
                                                                 const char       *item);
char *                 gdm_option_widget_get_default_item       (GdmOptionWidget *widget);
void                   gdm_option_widget_set_default_item       (GdmOptionWidget *widget,
                                                                 const char       *item);
G_END_DECLS

#endif /* __GDM_OPTION_WIDGET_H */
