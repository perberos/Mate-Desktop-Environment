/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2006-2007 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU General Public License Version 2
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __GPM_GRAPH_WIDGET_H__
#define __GPM_GRAPH_WIDGET_H__

#include <gtk/gtk.h>
#include "gpm-point-obj.h"

G_BEGIN_DECLS

#define GPM_TYPE_GRAPH_WIDGET		(gpm_graph_widget_get_type ())
#define GPM_GRAPH_WIDGET(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GPM_TYPE_GRAPH_WIDGET, GpmGraphWidget))
#define GPM_GRAPH_WIDGET_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), GPM_GRAPH_WIDGET, GpmGraphWidgetClass))
#define GPM_IS_GRAPH_WIDGET(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GPM_TYPE_GRAPH_WIDGET))
#define GPM_IS_GRAPH_WIDGET_CLASS(obj)	(G_TYPE_CHECK_CLASS_TYPE ((obj), EFF_TYPE_GRAPH_WIDGET))
#define GPM_GRAPH_WIDGET_GET_CLASS	(G_TYPE_INSTANCE_GET_CLASS ((obj), GPM_TYPE_GRAPH_WIDGET, GpmGraphWidgetClass))

#define GPM_GRAPH_WIDGET_LEGEND_SPACING		17

typedef struct GpmGraphWidget		GpmGraphWidget;
typedef struct GpmGraphWidgetClass	GpmGraphWidgetClass;
typedef struct GpmGraphWidgetPrivate	GpmGraphWidgetPrivate;

typedef enum {
	GPM_GRAPH_WIDGET_TYPE_INVALID,
	GPM_GRAPH_WIDGET_TYPE_PERCENTAGE,
	GPM_GRAPH_WIDGET_TYPE_FACTOR,
	GPM_GRAPH_WIDGET_TYPE_TIME,
	GPM_GRAPH_WIDGET_TYPE_POWER,
	GPM_GRAPH_WIDGET_TYPE_VOLTAGE,
	GPM_GRAPH_WIDGET_TYPE_UNKNOWN
} GpmGraphWidgetType;

typedef enum {
	GPM_GRAPH_WIDGET_PLOT_LINE,
	GPM_GRAPH_WIDGET_PLOT_POINTS,
	GPM_GRAPH_WIDGET_PLOT_BOTH
} GpmGraphWidgetPlot;

/* the different kinds of lines in the key */
typedef struct {
	guint32			 color;
	gchar			*desc;
} GpmGraphWidgetKeyData;

struct GpmGraphWidget
{
	GtkDrawingArea		 parent;
	GpmGraphWidgetPrivate	*priv;
};

struct GpmGraphWidgetClass
{
	GtkDrawingAreaClass parent_class;
};

GType		 gpm_graph_widget_get_type		(void);
GtkWidget	*gpm_graph_widget_new			(void);

gboolean	 gpm_graph_widget_data_clear		(GpmGraphWidget		*graph);
gboolean	 gpm_graph_widget_data_assign		(GpmGraphWidget		*graph,
							 GpmGraphWidgetPlot	 plot,
							 GPtrArray		*array);
gboolean	 gpm_graph_widget_key_data_add		(GpmGraphWidget		*graph,
							 guint32		 color,
							 const gchar		*desc);

G_END_DECLS

#endif
