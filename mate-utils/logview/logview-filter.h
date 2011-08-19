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

#ifndef _LOGVIEW_FILTER_H_
#define _LOGVIEW_FILTER_H_

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define LOGVIEW_TYPE_FILTER             (logview_filter_get_type ())
#define LOGVIEW_FILTER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), LOGVIEW_TYPE_FILTER, LogviewFilter))
#define LOGVIEW_FILTER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), LOGVIEW_TYPE_FILTER, LogviewFilterClass))
#define LOGVIEW_IS_FILTER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LOGVIEW_TYPE_FILTER))
#define LOGVIEW_IS_FILTER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), LOGVIEW_TYPE_FILTER))
#define LOGVIEW_FILTER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), LOGVIEW_TYPE_FILTER, LogviewFilterClass))

typedef struct _LogviewFilterClass LogviewFilterClass;
typedef struct _LogviewFilter LogviewFilter;
typedef struct _LogviewFilterPrivate LogviewFilterPrivate;

struct _LogviewFilterClass {
  GObjectClass parent_class;
};

struct _LogviewFilter {
  GObject parent_instance;

  LogviewFilterPrivate *priv;
};

GType           logview_filter_get_type (void) G_GNUC_CONST;
LogviewFilter * logview_filter_new (const gchar *name, 
                                    const gchar *regex);
gboolean        logview_filter_filter (LogviewFilter *filter, 
                                       const gchar *line);
GtkTextTag *    logview_filter_get_tag (LogviewFilter *filter);

G_END_DECLS

#endif /* _LOGVIEW_FILTER_H_ */
