/*
 * Music Applet
 * Copyright (C) 2006 Paul Kuliniewicz <paul.kuliniewicz@gmail.com>
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

#ifndef MA_FANCY_BUTTON_H
#define MA_FANCY_BUTTON_H

#include <gtk/gtkbutton.h>
#include <mate-panel-applet.h>

#define MA_TYPE_FANCY_BUTTON		(ma_fancy_button_get_type ())
#define MA_FANCY_BUTTON(o) 		(G_TYPE_CHECK_INSTANCE_CAST ((o), MA_TYPE_FANCY_BUTTON, MaFancyButton))
#define MA_FANCY_BUTTON_CLASS(k) 	(G_TYPE_CHECK_CLASS_CAST ((k), MA_TYPE_FANCY_BUTTON, MaFancyButtonClass))
#define MA_IS_FANCY_BUTTON(o) 		(G_TYPE_CHECK_INSTANCE_TYPE ((o), MA_TYPE_FANCY_BUTTON))
#define MA_IS_FANCY_BUTTON_CLASS(k) 	(G_TYPE_CHECK_CLASS_TYPE ((k), MA_TYPE_FANCY_BUTTON))
#define MA_FANCY_BUTTON_GET_CLASS(o) 	(G_TYPE_INSTANCE_GET_CLASS ((o), MA_TYPE_FANCY_BUTTON, MaFancyButtonClass))

G_BEGIN_DECLS

/* h2def.py expects the names to be defined like this... */

typedef struct _MaFancyButton		MaFancyButton;
typedef struct _MaFancyButtonClass	MaFancyButtonClass;

struct _MaFancyButton
{
	GtkButton parent;
};

struct _MaFancyButtonClass
{
	GtkButtonClass parent_class;
};


GType ma_fancy_button_get_type (void);

GtkWidget *ma_fancy_button_new (void);

const gchar *ma_fancy_button_get_stock_id (MaFancyButton *fb);
void ma_fancy_button_set_stock_id (MaFancyButton *fb, const gchar *stock_id);

const gchar *ma_fancy_button_get_icon_name (MaFancyButton *fb);
void ma_fancy_button_set_icon_name (MaFancyButton *fb, const gchar *icon_name);

MatePanelAppletOrient ma_fancy_button_get_orientation (MaFancyButton *fb);
void ma_fancy_button_set_orientation (MaFancyButton *fb, MatePanelAppletOrient orient);

G_END_DECLS

#endif /* MA_FANCY_BUTTON_H */
