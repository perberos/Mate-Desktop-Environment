/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; -*- */
/*
 * mate-utils
 * Copyright (C) Johannes Schmid 2009 <jhs@mate.org>
 * 
 * mate-utils is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * mate-utils is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _LOGVIEW_FILTER_MANAGER_H_
#define _LOGVIEW_FILTER_MANAGER_H_

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define LOGVIEW_TYPE_FILTER_MANAGER             (logview_filter_manager_get_type ())
#define LOGVIEW_FILTER_MANAGER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), LOGVIEW_TYPE_FILTER_MANAGER, LogviewFilterManager))
#define LOGVIEW_FILTER_MANAGER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), LOGVIEW_TYPE_FILTER_MANAGER, LogviewFilterManagerClass))
#define LOGVIEW_IS_FILTER_MANAGER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LOGVIEW_TYPE_FILTER_MANAGER))
#define LOGVIEW_IS_FILTER_MANAGER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), LOGVIEW_TYPE_FILTER_MANAGER))
#define LOGVIEW_FILTER_MANAGER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), LOGVIEW_TYPE_FILTER_MANAGER, LogviewFilterManagerClass))

typedef struct _LogviewFilterManagerClass LogviewFilterManagerClass;
typedef struct _LogviewFilterManager LogviewFilterManager;
typedef struct _LogviewFilterManagerPrivate LogviewFilterManagerPrivate;

struct _LogviewFilterManagerClass {
	GtkDialogClass parent_class;
};

struct _LogviewFilterManager {
	GtkDialog parent_instance;

	LogviewFilterManagerPrivate* priv;
};

GType       logview_filter_manager_get_type (void) G_GNUC_CONST;
GtkWidget * logview_filter_manager_new (void);

G_END_DECLS

#endif /* _LOGVIEW_FILTER_MANAGER_H_ */
