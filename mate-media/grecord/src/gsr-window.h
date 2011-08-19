/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Iain Holmes <iain@prettypeople.org>
 *
 *  Copyright 2002 Iain Holmes
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * 4th Februrary 2005: Christian Schaller: changed license to LGPL with
 * permission of Iain Holmes, Ronald Bultje, Leontine Binchy (SUN), Johan Dahlin
 *  and Joe Marcus Clarke
 *
 */

#ifndef __GSR_WINDOW_H__
#define __GSR_WINDOW_H__

#include <gtk/gtk.h>

#define GSR_TYPE_WINDOW (gsr_window_get_type ())
#define GSR_WINDOW(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSR_TYPE_WINDOW, GSRWindow))
#define GSR_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GSR_TYPE_WINDOW, GSRWindowClass))
#define GSR_IS_WINDOW(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSR_TYPE_WINDOW))
#define GSR_IS_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GSR_TYPE_WINDOW))

typedef struct _GSRWindow GSRWindow;
typedef struct _GSRWindowClass GSRWindowClass;
typedef struct _GSRWindowPrivate GSRWindowPrivate;

struct _GSRWindow {
	GtkWindow parent;

	GSRWindowPrivate *priv;
};

struct _GSRWindowClass {
	GtkWindowClass parent_class;
};


GType		gsr_window_get_type		(void);

GtkWidget*	gsr_window_new			(const char *filename);
void		gsr_window_close		(GSRWindow *window);
gboolean	gsr_window_is_saved		(GSRWindow *window);
gboolean	gsr_discard_confirmation_dialog	(GSRWindow *window, gboolean closing);

#endif
