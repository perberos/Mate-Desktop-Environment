/*
 * Music Applet
 * Copyright (C) 2007 Paul Kuliniewicz <paul@kuliniewicz.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02111-1301, USA.
 */

#ifndef MA_SCROLLER_H
#define MA_SCROLLER_H

#include <gtk/gtk.h>
#include <mate-panel-applet.h>

#define MA_TYPE_SCROLLER		(ma_scroller_get_type ())
#define MA_SCROLLER(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), MA_TYPE_SCROLLER, MaScroller))
#define MA_SCROLLER_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), MA_TYPE_SCROLLER, MaScrollerClass))
#define MA_IS_SCROLLER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MA_TYPE_SCROLLER))
#define MA_IS_SCROLLER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), MA_TYPE_SCROLLER))
#define MA_SCROLLER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), MA_TYPE_SCROLLER, MaScrollerClass))

G_BEGIN_DECLS

typedef struct _MaScroller		MaScroller;
typedef struct _MaScrollerClass		MaScrollerClass;

struct _MaScroller
{
	GtkEventBox parent;
};

struct _MaScrollerClass
{
	GtkEventBoxClass parent_class;
};


GType ma_scroller_get_type (void);

GtkWidget *ma_scroller_new (void);

GObject *ma_scroller_get_plugin (MaScroller *scroller);
void ma_scroller_set_plugin (MaScroller *scroller, GObject *plugin);

MatePanelAppletOrient ma_scroller_get_orientation (MaScroller *scroller);
void ma_scroller_set_orientation (MaScroller *scroller, MatePanelAppletOrient orient);

G_END_DECLS

#endif /* MA_SCROLLER_H */
