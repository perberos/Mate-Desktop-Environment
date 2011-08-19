/* logview-loglist.h - displays a list of the opened logs
 *
 * Copyright (C) 2005 Vincent Noel <vnoel@cox.net>
 * Copyright (C) 2008 Cosimo Cecchi <cosimoc@mate.org>
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

#ifndef __LOGVIEW_LOGLIST_H__
#define __LOGVIEW_LOGLIST_H__

#define LOGVIEW_TYPE_LOGLIST logview_loglist_get_type()
#define LOGVIEW_LOGLIST(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), LOGVIEW_TYPE_LOGLIST, LogviewLoglist))
#define LOGVIEW_LOGLIST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), LOGVIEW_TYPE_LOGLIST, LogviewLogListClass))
#define LOGVIEW_IS_LOGLIST(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LOGVIEW_TYPE_LOGLIST))
#define LOGVIEW_IS_LOGLIST_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), LOGVIEW_TYPE_LOGLIST))
#define LOGVIEW_LOGLIST_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), LOGVIEW_TYPE_LOGLIST, LogviewLoglistClass))

#include <gtk/gtk.h>
#include <glib-object.h>

#include "logview-log.h"
#include "logview-utils.h"

typedef struct _LogviewLoglist LogviewLoglist;
typedef struct _LogviewLoglistClass LogviewLoglistClass;
typedef struct _LogviewLoglistPrivate LogviewLoglistPrivate;

struct _LogviewLoglist {	
  GtkTreeView parent_instance;
  LogviewLoglistPrivate *priv;
};

struct _LogviewLoglistClass {
	GtkTreeViewClass parent_class;

  void (* day_selected) (LogviewLoglist *loglist,
                         Day *day);
  void (* day_cleared) (LogviewLoglist *loglist);
};

GType logview_loglist_get_type (void);

/* public methods */
GtkWidget * logview_loglist_new                (void);
void        logview_loglist_update_lines       (LogviewLoglist *loglist,
                                                LogviewLog *log);
GDate *     logview_loglist_get_date_selection (LogviewLoglist *loglist);
void        logview_loglist_clear_date         (LogviewLoglist *loglist);

#endif /* __LOGVIEW_LOGLIST_H__ */
