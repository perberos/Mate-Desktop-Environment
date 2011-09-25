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

#ifndef __MDM_RECENT_OPTION_WIDGET_H
#define __MDM_RECENT_OPTION_WIDGET_H

#include <glib-object.h>

#include "mdm-option-widget.h"

G_BEGIN_DECLS

#define MDM_TYPE_RECENT_OPTION_WIDGET         (mdm_recent_option_widget_get_type ())
#define MDM_RECENT_OPTION_WIDGET(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_RECENT_OPTION_WIDGET, MdmRecentOptionWidget))
#define MDM_RECENT_OPTION_WIDGET_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_RECENT_OPTION_WIDGET, MdmRecentOptionWidgetClass))
#define MDM_IS_RECENT_OPTION_WIDGET(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_RECENT_OPTION_WIDGET))
#define MDM_IS_RECENT_OPTION_WIDGET_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_RECENT_OPTION_WIDGET))
#define MDM_RECENT_OPTION_WIDGET_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_RECENT_OPTION_WIDGET, MdmRecentOptionWidgetClass))

typedef struct MdmRecentOptionWidgetPrivate MdmRecentOptionWidgetPrivate;

typedef struct
{
        MdmOptionWidget               parent;
        MdmRecentOptionWidgetPrivate *priv;
} MdmRecentOptionWidget;

typedef struct
{
        MdmOptionWidgetClass       parent_class;
} MdmRecentOptionWidgetClass;

typedef char * (* MdmRecentOptionLookupItemFunc) (MdmRecentOptionWidget  *widget,
                                                  const char             *key,
                                                  char                  **name,
                                                  char                  **comment);


GType                  mdm_recent_option_widget_get_type               (void);
GtkWidget *            mdm_recent_option_widget_new                    (const char *label_text,
                                                                        int         max_item_count);
gboolean               mdm_recent_option_widget_set_mateconf_key          (MdmRecentOptionWidget *widget,
                                                                        const char *mateconf_key,
                                                                        MdmRecentOptionLookupItemFunc func,
                                                                        GError     **error);
void                   mdm_recent_option_widget_add_item               (MdmRecentOptionWidget *widget,
                                                                        const char            *id);

G_END_DECLS

#endif /* __MDM_RECENT_OPTION_WIDGET_H */
