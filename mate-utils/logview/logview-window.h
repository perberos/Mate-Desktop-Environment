/* logview-window.h - main window of logview
 *
 * Copyright (C) 1998  Cesar Miquel  <miquel@df.uba.ar>
 * Copyright (C) 2008  Cosimo Cecchi <cosimoc@mate.org>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __LOGVIEW_WINDOW_H__
#define __LOGVIEW_WINDOW_H__

#include <gtk/gtk.h>

#define LOGVIEW_TYPE_WINDOW		  (logview_window_get_type ())
#define LOGVIEW_WINDOW(obj)		  (G_TYPE_CHECK_INSTANCE_CAST ((obj), LOGVIEW_TYPE_WINDOW, LogviewWindow))
#define LOGVIEW_WINDOW_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), LOGVIEW_TYPE_WINDOW, LogviewWindowClass))
#define LOGVIEW_IS_WINDOW(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LOGVIEW_TYPE_WINDOW))
#define LOGVIEW_IS_WINDOW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((obj), LOGVIEW_TYPE_WINDOW))
#define LOGVIEW_WINDOW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), LOGVIEW_TYPE_WINDOW, LogviewWindowClass))

typedef struct _LogviewWindow LogviewWindow;
typedef struct _LogviewWindowClass LogviewWindowClass;
typedef struct _LogviewWindowPrivate LogviewWindowPrivate;

struct _LogviewWindow {
  GtkWindow parent_instance;
  LogviewWindowPrivate *priv;
};

struct _LogviewWindowClass {
	GtkWindowClass parent_class;
};

GType logview_window_get_type (void);

/* public methods */
GtkWidget * logview_window_new        (void);
void        logview_window_add_error  (LogviewWindow *window,
                                       const char *primary,
                                       const char *secondary);
void        logview_window_add_errors (LogviewWindow *window,
                                       GPtrArray *errors);

#endif /* __LOGVIEW_WINDOW_H__ */
