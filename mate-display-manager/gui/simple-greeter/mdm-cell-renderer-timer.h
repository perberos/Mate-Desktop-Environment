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
#ifndef __MDM_CELL_RENDERER_TIMER_H
#define __MDM_CELL_RENDERER_TIMER_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MDM_TYPE_CELL_RENDERER_TIMER (mdm_cell_renderer_timer_get_type ())
#define MDM_CELL_RENDERER_TIMER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), MDM_TYPE_CELL_RENDERER_TIMER, MdmCellRendererTimer))
#define MDM_CELL_RENDERER_TIMER_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), MDM_TYPE_CELL_RENDERER_TIMER, MdmCellRendererTimerClass))
#define GTK_IS_CELL_RENDERER_TIMER(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MDM_TYPE_CELL_RENDERER_TIMER))
#define GTK_IS_CELL_RENDERER_TIMER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MDM_TYPE_CELL_RENDERER_TIMER))
#define MDM_CELL_RENDERER_TIMER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MDM_TYPE_CELL_RENDERER_TIMER, MdmCellRendererTimerClass))

typedef struct _MdmCellRendererTimer         MdmCellRendererTimer;
typedef struct _MdmCellRendererTimerClass    MdmCellRendererTimerClass;
typedef struct _MdmCellRendererTimerPrivate  MdmCellRendererTimerPrivate;

struct _MdmCellRendererTimer
{
  GtkCellRenderer              parent;

  /*< private >*/
  MdmCellRendererTimerPrivate *priv;
};

struct _MdmCellRendererTimerClass
{
  GtkCellRendererClass parent_class;
};

GType		 mdm_cell_renderer_timer_get_type (void);
GtkCellRenderer* mdm_cell_renderer_timer_new      (void);

G_END_DECLS

#endif  /* __MDM_CELL_RENDERER_TIMER_H */
