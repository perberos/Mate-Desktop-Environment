/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Red Hat, Inc.
 * Copyright (C) 2007 Jon McCann <mccann@jhu.edu>
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
 *              Jon McCann <mccann@jhu.edu>
 */

#ifndef __MDM_CLOCK_WIDGET_H
#define __MDM_CLOCK_WIDGET_H

#include <glib-object.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MDM_TYPE_CLOCK_WIDGET         (mdm_clock_widget_get_type ())
#define MDM_CLOCK_WIDGET(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MDM_TYPE_CLOCK_WIDGET, MdmClockWidget))
#define MDM_CLOCK_WIDGET_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), MDM_TYPE_CLOCK_WIDGET, MdmClockWidgetClass))
#define MDM_IS_CLOCK_WIDGET(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MDM_TYPE_CLOCK_WIDGET))
#define MDM_IS_CLOCK_WIDGET_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MDM_TYPE_CLOCK_WIDGET))
#define MDM_CLOCK_WIDGET_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MDM_TYPE_CLOCK_WIDGET, MdmClockWidgetClass))

typedef struct MdmClockWidgetPrivate MdmClockWidgetPrivate;

typedef struct
{
        GtkAlignment           parent;
        MdmClockWidgetPrivate *priv;
} MdmClockWidget;

typedef struct
{
        GtkAlignmentClass              parent_class;
} MdmClockWidgetClass;

GType                  mdm_clock_widget_get_type               (void);
GtkWidget *            mdm_clock_widget_new                    (void);

#endif /* __MDM_CLOCK_WIDGET_H */
