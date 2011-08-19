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

#ifndef __GDM_RECENT_OPTION_WIDGET_H
#define __GDM_RECENT_OPTION_WIDGET_H

#include <glib-object.h>

#include "gdm-option-widget.h"

G_BEGIN_DECLS

#define GDM_TYPE_RECENT_OPTION_WIDGET         (gdm_recent_option_widget_get_type ())
#define GDM_RECENT_OPTION_WIDGET(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDM_TYPE_RECENT_OPTION_WIDGET, GdmRecentOptionWidget))
#define GDM_RECENT_OPTION_WIDGET_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GDM_TYPE_RECENT_OPTION_WIDGET, GdmRecentOptionWidgetClass))
#define GDM_IS_RECENT_OPTION_WIDGET(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDM_TYPE_RECENT_OPTION_WIDGET))
#define GDM_IS_RECENT_OPTION_WIDGET_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDM_TYPE_RECENT_OPTION_WIDGET))
#define GDM_RECENT_OPTION_WIDGET_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDM_TYPE_RECENT_OPTION_WIDGET, GdmRecentOptionWidgetClass))

typedef struct GdmRecentOptionWidgetPrivate GdmRecentOptionWidgetPrivate;

typedef struct
{
        GdmOptionWidget               parent;
        GdmRecentOptionWidgetPrivate *priv;
} GdmRecentOptionWidget;

typedef struct
{
        GdmOptionWidgetClass       parent_class;
} GdmRecentOptionWidgetClass;

typedef char * (* GdmRecentOptionLookupItemFunc) (GdmRecentOptionWidget  *widget,
                                                  const char             *key,
                                                  char                  **name,
                                                  char                  **comment);


GType                  gdm_recent_option_widget_get_type               (void);
GtkWidget *            gdm_recent_option_widget_new                    (const char *label_text,
                                                                        int         max_item_count);
gboolean               gdm_recent_option_widget_set_mateconf_key          (GdmRecentOptionWidget *widget,
                                                                        const char *mateconf_key,
                                                                        GdmRecentOptionLookupItemFunc func,
                                                                        GError     **error);
void                   gdm_recent_option_widget_add_item               (GdmRecentOptionWidget *widget,
                                                                        const char            *id);

G_END_DECLS

#endif /* __GDM_RECENT_OPTION_WIDGET_H */
