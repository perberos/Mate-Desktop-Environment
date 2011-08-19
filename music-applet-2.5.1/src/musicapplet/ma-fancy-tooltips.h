/*
 * Music Applet
 * Copyright (C) 2006 Paul Kuliniewicz <paul@kuliniewicz.org>
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

#ifndef MA_FANCY_TOOLTIPS_H
#define MA_FANCY_TOOLTIPS_H

#include <gtk/gtktooltips.h>

#define MA_TYPE_FANCY_TOOLTIPS		(ma_fancy_tooltips_get_type ())
#define MA_FANCY_TOOLTIPS(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), MA_TYPE_FANCY_TOOLTIPS, MaFancyTooltips))
#define MA_FANCY_TOOLTIPS_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST ((k), MA_TYPE_FANCY_TOOLTIPS, MaFancyTooltipsClass))
#define MA_IS_FANCY_TOOLTIPS(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), MA_TYPE_FANCY_TOOLTIPS))
#define MA_IS_FANCY_TOOLTIPS_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), MA_TYPE_FANCY_TOOLTIPS))
#define MA_FANCY_TOOLTIPS_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), MA_TYPE_FANCY_TOOLTIPS, MaFancyTooltipsClass))

G_BEGIN_DECLS

/* h2def.py expects the names to be defined like this... */

typedef struct _MaFancyTooltips		MaFancyTooltips;
typedef struct _MaFancyTooltipsClass	MaFancyTooltipsClass;

struct _MaFancyTooltips
{
	GtkTooltips parent;
};

struct _MaFancyTooltipsClass
{
	GtkTooltipsClass parent_class;
};


GType ma_fancy_tooltips_get_type (void);

GtkTooltips *ma_fancy_tooltips_new (void);

GtkWidget *ma_fancy_tooltips_get_content (MaFancyTooltips *tips);
void ma_fancy_tooltips_set_content (MaFancyTooltips *tips, GtkWidget *content);

G_END_DECLS

#endif /* MA_FANCY_TOOLTIPS_H */
