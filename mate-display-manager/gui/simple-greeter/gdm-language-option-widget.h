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

#ifndef __GDM_LANGUAGE_OPTION_WIDGET_H
#define __GDM_LANGUAGE_OPTION_WIDGET_H

#include <glib-object.h>

#include "gdm-recent-option-widget.h"

G_BEGIN_DECLS

#define GDM_TYPE_LANGUAGE_OPTION_WIDGET         (gdm_language_option_widget_get_type ())
#define GDM_LANGUAGE_OPTION_WIDGET(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDM_TYPE_LANGUAGE_OPTION_WIDGET, GdmLanguageOptionWidget))
#define GDM_LANGUAGE_OPTION_WIDGET_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GDM_TYPE_LANGUAGE_OPTION_WIDGET, GdmLanguageOptionWidgetClass))
#define GDM_IS_LANGUAGE_OPTION_WIDGET(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDM_TYPE_LANGUAGE_OPTION_WIDGET))
#define GDM_IS_LANGUAGE_OPTION_WIDGET_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDM_TYPE_LANGUAGE_OPTION_WIDGET))
#define GDM_LANGUAGE_OPTION_WIDGET_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDM_TYPE_LANGUAGE_OPTION_WIDGET, GdmLanguageOptionWidgetClass))

typedef struct GdmLanguageOptionWidgetPrivate GdmLanguageOptionWidgetPrivate;

typedef struct
{
        GdmRecentOptionWidget          parent;
        GdmLanguageOptionWidgetPrivate *priv;
} GdmLanguageOptionWidget;

typedef struct
{
        GdmRecentOptionWidgetClass    parent_class;

        void (* language_activated)        (GdmLanguageOptionWidget *widget);
} GdmLanguageOptionWidgetClass;

GType                  gdm_language_option_widget_get_type               (void);
GtkWidget *            gdm_language_option_widget_new                    (void);

char *                 gdm_language_option_widget_get_current_language_name      (GdmLanguageOptionWidget *widget);
void                   gdm_language_option_widget_set_current_language_name      (GdmLanguageOptionWidget *widget,
                                                                                  const char              *lang_name);



#endif /* __GDM_LANGUAGE_OPTION_WIDGET_H */
