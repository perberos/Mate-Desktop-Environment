/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
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
 */

#ifndef __MDM_USER_CHOOSER_WIDGET_H
#define __MDM_USER_CHOOSER_WIDGET_H

#include <glib-object.h>

#include "mdm-chooser-widget.h"

G_BEGIN_DECLS

#define MDM_TYPE_USER_CHOOSER_WIDGET         (mdm_user_chooser_widget_get_type ())
#define MDM_USER_CHOOSER_WIDGET(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_USER_CHOOSER_WIDGET, MdmUserChooserWidget))
#define MDM_USER_CHOOSER_WIDGET_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_USER_CHOOSER_WIDGET, MdmUserChooserWidgetClass))
#define MDM_IS_USER_CHOOSER_WIDGET(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_USER_CHOOSER_WIDGET))
#define MDM_IS_USER_CHOOSER_WIDGET_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_USER_CHOOSER_WIDGET))
#define MDM_USER_CHOOSER_WIDGET_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_USER_CHOOSER_WIDGET, MdmUserChooserWidgetClass))

typedef struct MdmUserChooserWidgetPrivate MdmUserChooserWidgetPrivate;

typedef struct
{
        MdmChooserWidget            parent;
        MdmUserChooserWidgetPrivate *priv;
} MdmUserChooserWidget;

typedef struct
{
        MdmChooserWidgetClass   parent_class;
} MdmUserChooserWidgetClass;

#define MDM_USER_CHOOSER_USER_OTHER "__other"
#define MDM_USER_CHOOSER_USER_GUEST "__guest"
#define MDM_USER_CHOOSER_USER_AUTO  "__auto"

GType                  mdm_user_chooser_widget_get_type                   (void);
GtkWidget *            mdm_user_chooser_widget_new                        (void);

char *                 mdm_user_chooser_widget_get_chosen_user_name       (MdmUserChooserWidget *widget);
void                   mdm_user_chooser_widget_set_chosen_user_name       (MdmUserChooserWidget *widget,
                                                                           const char           *user_name);
void                   mdm_user_chooser_widget_set_show_only_chosen       (MdmUserChooserWidget *widget,
                                                                           gboolean              show_only);
void                   mdm_user_chooser_widget_set_show_user_other        (MdmUserChooserWidget *widget,
                                                                           gboolean              show);
void                   mdm_user_chooser_widget_set_show_user_guest        (MdmUserChooserWidget *widget,
                                                                           gboolean              show);
void                   mdm_user_chooser_widget_set_show_user_auto         (MdmUserChooserWidget *widget,
                                                                           gboolean              show);
G_END_DECLS

#endif /* __MDM_USER_CHOOSER_WIDGET_H */
