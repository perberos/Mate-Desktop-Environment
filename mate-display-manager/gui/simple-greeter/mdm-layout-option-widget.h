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

#ifndef __MDM_LAYOUT_OPTION_WIDGET_H
#define __MDM_LAYOUT_OPTION_WIDGET_H

#include <glib-object.h>

#include "mdm-recent-option-widget.h"

G_BEGIN_DECLS

#define MDM_TYPE_LAYOUT_OPTION_WIDGET         (mdm_layout_option_widget_get_type ())
#define MDM_LAYOUT_OPTION_WIDGET(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_LAYOUT_OPTION_WIDGET, MdmLayoutOptionWidget))
#define MDM_LAYOUT_OPTION_WIDGET_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_LAYOUT_OPTION_WIDGET, MdmLayoutOptionWidgetClass))
#define MDM_IS_LAYOUT_OPTION_WIDGET(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_LAYOUT_OPTION_WIDGET))
#define MDM_IS_LAYOUT_OPTION_WIDGET_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_LAYOUT_OPTION_WIDGET))
#define MDM_LAYOUT_OPTION_WIDGET_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_LAYOUT_OPTION_WIDGET, MdmLayoutOptionWidgetClass))

typedef struct MdmLayoutOptionWidgetPrivate MdmLayoutOptionWidgetPrivate;

typedef struct
{
        MdmRecentOptionWidget          parent;
        MdmLayoutOptionWidgetPrivate *priv;
} MdmLayoutOptionWidget;

typedef struct
{
        MdmRecentOptionWidgetClass    parent_class;

        void (* layout_activated)        (MdmLayoutOptionWidget *widget);
} MdmLayoutOptionWidgetClass;

GType                  mdm_layout_option_widget_get_type               (void);
GtkWidget *            mdm_layout_option_widget_new                    (void);

char *                 mdm_layout_option_widget_get_current_layout_name      (MdmLayoutOptionWidget *widget);
void                   mdm_layout_option_widget_set_current_layout_name      (MdmLayoutOptionWidget *widget,
                                                                                  const char              *name);



#endif /* __MDM_LAYOUT_OPTION_WIDGET_H */
