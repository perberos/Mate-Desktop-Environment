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
#ifndef __GDM_CELL_RENDERER_TIMER_H
#define __GDM_CELL_RENDERER_TIMER_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GDM_TYPE_CELL_RENDERER_TIMER (gdm_cell_renderer_timer_get_type ())
#define GDM_CELL_RENDERER_TIMER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GDM_TYPE_CELL_RENDERER_TIMER, GdmCellRendererTimer))
#define GDM_CELL_RENDERER_TIMER_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), GDM_TYPE_CELL_RENDERER_TIMER, GdmCellRendererTimerClass))
#define GTK_IS_CELL_RENDERER_TIMER(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDM_TYPE_CELL_RENDERER_TIMER))
#define GTK_IS_CELL_RENDERER_TIMER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GDM_TYPE_CELL_RENDERER_TIMER))
#define GDM_CELL_RENDERER_TIMER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GDM_TYPE_CELL_RENDERER_TIMER, GdmCellRendererTimerClass))

typedef struct _GdmCellRendererTimer         GdmCellRendererTimer;
typedef struct _GdmCellRendererTimerClass    GdmCellRendererTimerClass;
typedef struct _GdmCellRendererTimerPrivate  GdmCellRendererTimerPrivate;

struct _GdmCellRendererTimer
{
  GtkCellRenderer              parent;

  /*< private >*/
  GdmCellRendererTimerPrivate *priv;
};

struct _GdmCellRendererTimerClass
{
  GtkCellRendererClass parent_class;
};

GType		 gdm_cell_renderer_timer_get_type (void);
GtkCellRenderer* gdm_cell_renderer_timer_new      (void);

G_END_DECLS

#endif  /* __GDM_CELL_RENDERER_TIMER_H */
