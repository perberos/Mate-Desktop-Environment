/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2001, 2002 Anders Carlsson <andersca@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __MATECONF_CELL_RENDERER_H__
#define __MATECONF_CELL_RENDERER_H__

#include <gtk/gtk.h>
#include <mateconf/mateconf-value.h>

#define MATECONF_TYPE_CELL_RENDERER	    (mateconf_cell_renderer_get_type ())
#define MATECONF_CELL_RENDERER(obj)	    (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATECONF_TYPE_CELL_RENDERER, MateConfCellRenderer))
#define MATECONF_CELL_RENDERER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATECONF_TYPE_CELL_RENDERER, MateConfCellRendererClass))
#define MATECONF_IS_CELL_RENDERER(obj)	    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATECONF_TYPE_CELL_RENDERER))
#define MATECONF_IS_CELL_RENDERER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), MATECONF_TYPE_CELL_RENDERER))
#define MATECONF_CELL_RENDERER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATECONF_TYPE_CELL_RENDERER, MateConfCellRendererClass))

typedef struct _MateConfCellRenderer MateConfCellRenderer;
typedef struct _MateConfCellRendererClass MateConfCellRendererClass;

enum {
	MATECONF_CELL_RENDERER_ICON_COLUMN,
	MATECONF_CELL_RENDERER_KEY_NAME_COLUMN,
	MATECONF_CELL_RENDERER_VALUE_COLUMN,
	MATECONF_CELL_RENDERER_VALUE_TYPE_COLUMN,
	MATECONF_CELL_RENDERER_NUM_COLUMNS
};

struct _MateConfCellRenderer {
	GtkCellRenderer parent_instance;

	MateConfValue *value;

	GtkCellRenderer *text_renderer;
	GtkCellRenderer *toggle_renderer;
};

struct _MateConfCellRendererClass {
	GtkCellRendererClass parent_class;

	void (*changed) (MateConfCellRenderer *renderer, char *path, MateConfValue *value);
	gboolean (*check_writable) (MateConfCellRenderer *renderer, char *path);
};

GType mateconf_cell_renderer_get_type (void);
GtkCellRenderer *mateconf_cell_renderer_new (void);

#endif /* __MATECONF_CELL_RENDERER_H__ */
