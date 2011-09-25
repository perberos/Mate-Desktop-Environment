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

#ifndef __MDM_HOST_CHOOSER_WIDGET_H
#define __MDM_HOST_CHOOSER_WIDGET_H

#include <glib-object.h>
#include <gtk/gtk.h>
#include "mdm-chooser-host.h"

G_BEGIN_DECLS

#define MDM_TYPE_HOST_CHOOSER_WIDGET         (mdm_host_chooser_widget_get_type ())
#define MDM_HOST_CHOOSER_WIDGET(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_HOST_CHOOSER_WIDGET, MdmHostChooserWidget))
#define MDM_HOST_CHOOSER_WIDGET_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_HOST_CHOOSER_WIDGET, MdmHostChooserWidgetClass))
#define MDM_IS_HOST_CHOOSER_WIDGET(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_HOST_CHOOSER_WIDGET))
#define MDM_IS_HOST_CHOOSER_WIDGET_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_HOST_CHOOSER_WIDGET))
#define MDM_HOST_CHOOSER_WIDGET_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_HOST_CHOOSER_WIDGET, MdmHostChooserWidgetClass))

typedef struct MdmHostChooserWidgetPrivate MdmHostChooserWidgetPrivate;

typedef struct
{
        GtkVBox                      parent;
        MdmHostChooserWidgetPrivate *priv;
} MdmHostChooserWidget;

typedef struct
{
        GtkVBoxClass   parent_class;

        /* signals */
        void (* host_activated)        (MdmHostChooserWidget *widget);
} MdmHostChooserWidgetClass;

GType                  mdm_host_chooser_widget_get_type           (void);
GtkWidget *            mdm_host_chooser_widget_new                (int                   kind_mask);

void                   mdm_host_chooser_widget_set_kind_mask      (MdmHostChooserWidget *widget,
                                                                   int                   kind_mask);

void                   mdm_host_chooser_widget_refresh            (MdmHostChooserWidget *widget);

MdmChooserHost *       mdm_host_chooser_widget_get_host           (MdmHostChooserWidget *widget);

G_END_DECLS

#endif /* __MDM_HOST_CHOOSER_WIDGET_H */
