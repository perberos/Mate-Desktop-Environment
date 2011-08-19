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

#ifndef __GDM_CLOCK_WIDGET_H
#define __GDM_CLOCK_WIDGET_H

#include <glib-object.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GDM_TYPE_CLOCK_WIDGET         (gdm_clock_widget_get_type ())
#define GDM_CLOCK_WIDGET(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDM_TYPE_CLOCK_WIDGET, GdmClockWidget))
#define GDM_CLOCK_WIDGET_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GDM_TYPE_CLOCK_WIDGET, GdmClockWidgetClass))
#define GDM_IS_CLOCK_WIDGET(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDM_TYPE_CLOCK_WIDGET))
#define GDM_IS_CLOCK_WIDGET_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDM_TYPE_CLOCK_WIDGET))
#define GDM_CLOCK_WIDGET_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GDM_TYPE_CLOCK_WIDGET, GdmClockWidgetClass))

typedef struct GdmClockWidgetPrivate GdmClockWidgetPrivate;

typedef struct
{
        GtkAlignment           parent;
        GdmClockWidgetPrivate *priv;
} GdmClockWidget;

typedef struct
{
        GtkAlignmentClass              parent_class;
} GdmClockWidgetClass;

GType                  gdm_clock_widget_get_type               (void);
GtkWidget *            gdm_clock_widget_new                    (void);

#endif /* __GDM_CLOCK_WIDGET_H */
