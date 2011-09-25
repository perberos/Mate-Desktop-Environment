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

#ifndef __MDM_LANGUAGE_OPTION_WIDGET_H
#define __MDM_LANGUAGE_OPTION_WIDGET_H

#include <glib-object.h>

#include "mdm-recent-option-widget.h"

G_BEGIN_DECLS

#define MDM_TYPE_LANGUAGE_OPTION_WIDGET         (mdm_language_option_widget_get_type ())
#define MDM_LANGUAGE_OPTION_WIDGET(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_LANGUAGE_OPTION_WIDGET, MdmLanguageOptionWidget))
#define MDM_LANGUAGE_OPTION_WIDGET_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_LANGUAGE_OPTION_WIDGET, MdmLanguageOptionWidgetClass))
#define MDM_IS_LANGUAGE_OPTION_WIDGET(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_LANGUAGE_OPTION_WIDGET))
#define MDM_IS_LANGUAGE_OPTION_WIDGET_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_LANGUAGE_OPTION_WIDGET))
#define MDM_LANGUAGE_OPTION_WIDGET_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_LANGUAGE_OPTION_WIDGET, MdmLanguageOptionWidgetClass))

typedef struct MdmLanguageOptionWidgetPrivate MdmLanguageOptionWidgetPrivate;

typedef struct
{
        MdmRecentOptionWidget          parent;
        MdmLanguageOptionWidgetPrivate *priv;
} MdmLanguageOptionWidget;

typedef struct
{
        MdmRecentOptionWidgetClass    parent_class;

        void (* language_activated)        (MdmLanguageOptionWidget *widget);
} MdmLanguageOptionWidgetClass;

GType                  mdm_language_option_widget_get_type               (void);
GtkWidget *            mdm_language_option_widget_new                    (void);

char *                 mdm_language_option_widget_get_current_language_name      (MdmLanguageOptionWidget *widget);
void                   mdm_language_option_widget_set_current_language_name      (MdmLanguageOptionWidget *widget,
                                                                                  const char              *lang_name);



#endif /* __MDM_LANGUAGE_OPTION_WIDGET_H */
