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

#ifndef __MDM_OPTION_WIDGET_H
#define __MDM_OPTION_WIDGET_H

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MDM_TYPE_OPTION_WIDGET         (mdm_option_widget_get_type ())
#define MDM_OPTION_WIDGET(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_OPTION_WIDGET, MdmOptionWidget))
#define MDM_OPTION_WIDGET_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_OPTION_WIDGET, MdmOptionWidgetClass))
#define MDM_IS_OPTION_WIDGET(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_OPTION_WIDGET))
#define MDM_IS_OPTION_WIDGET_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_OPTION_WIDGET))
#define MDM_OPTION_WIDGET_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_OPTION_WIDGET, MdmOptionWidgetClass))

typedef struct MdmOptionWidgetPrivate MdmOptionWidgetPrivate;

typedef struct
{
        GtkAlignment             parent;
        MdmOptionWidgetPrivate *priv;
} MdmOptionWidget;

typedef struct
{
        GtkAlignmentClass       parent_class;

        void (* activated)      (MdmOptionWidget *widget);
} MdmOptionWidgetClass;

typedef enum {
        MDM_OPTION_WIDGET_POSITION_TOP = 0,
        MDM_OPTION_WIDGET_POSITION_MIDDLE,
        MDM_OPTION_WIDGET_POSITION_BOTTOM,
} MdmOptionWidgetPosition;

GType                  mdm_option_widget_get_type               (void);
GtkWidget *            mdm_option_widget_new                    (const char *label_text);

void                   mdm_option_widget_add_item               (MdmOptionWidget *widget,
                                                                 const char       *id,
                                                                 const char       *name,
                                                                 const char       *comment,
                                                                 MdmOptionWidgetPosition position);

void                   mdm_option_widget_remove_item            (MdmOptionWidget *widget,
                                                                 const char       *id);

void                   mdm_option_widget_remove_all_items       (MdmOptionWidget *widget);
gboolean               mdm_option_widget_lookup_item            (MdmOptionWidget *widget,
                                                                 const char       *id,
                                                                 char            **name,
                                                                 char            **comment,
                                                                 MdmOptionWidgetPosition *position);

char *                 mdm_option_widget_get_active_item        (MdmOptionWidget *widget);
void                   mdm_option_widget_set_active_item        (MdmOptionWidget *widget,
                                                                 const char       *item);
char *                 mdm_option_widget_get_default_item       (MdmOptionWidget *widget);
void                   mdm_option_widget_set_default_item       (MdmOptionWidget *widget,
                                                                 const char       *item);
G_END_DECLS

#endif /* __MDM_OPTION_WIDGET_H */
