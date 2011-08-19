/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-output-window.h
 * This file is part of gedit
 *
 * Copyright (C) 2002 Paolo Maggi 
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
 * Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA. 
 * 
 */
 
/*
 * Modified by the gedit Team, 2002. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#ifndef __GEDIT_OUTPUT_WINDOW_H__
#define __GEDIT_OUTPUT_WINDOW_H__

#include <gtk/gtk.h>

#define GEDIT_TYPE_OUTPUT_WINDOW		(gedit_output_window_get_type ())
#define GEDIT_OUTPUT_WINDOW(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_OUTPUT_WINDOW, GeditOutputWindow))
#define GEDIT_OUTPUT_WINDOW_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_OUTPUT_WINDOW, GeditOutputWindowClass))
#define GEDIT_IS_OUTPUT_WINDOW(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_OUTPUT_WINDOW))
#define GEDIT_IS_OUTPUT_WINDOW_CLASS(klass)  	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_OUTPUT_WINDOW))
#define GEDIT_OUTPUT_WINDOW_GET_CLASS(obj)  	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_OUTPUT_WINDOW, GeditOutputWindowClass))


typedef struct _GeditOutputWindow		GeditOutputWindow;
typedef struct _GeditOutputWindowClass		GeditOutputWindowClass;

typedef struct _GeditOutputWindowPrivate	GeditOutputWindowPrivate;

struct _GeditOutputWindow
{
	GtkHBox box;
	
	GeditOutputWindowPrivate *priv;
};

struct _GeditOutputWindowClass
{
	GtkHBoxClass parent_class;

	void (*close_requested) (GeditOutputWindow	*ow);
	void (*selection_changed) (GeditOutputWindow	*ow, gchar *line);
};


GType        	 gedit_output_window_get_type     	(void) G_GNUC_CONST;

GtkWidget	*gedit_output_window_new		(void);

void		 gedit_output_window_clear		(GeditOutputWindow *ow);

void 		 gedit_output_window_append_line	(GeditOutputWindow *ow,
							 const gchar *line,
							 gboolean scroll);

void 		 gedit_output_window_prepend_line	(GeditOutputWindow *ow,
							 const gchar *line,
							 gboolean scroll);

void		 gedit_output_window_set_select_multiple (GeditOutputWindow *ow,
							  const GtkSelectionMode type);

#endif /* __GEDIT_OUTPUT_WINDOW_H__ */

