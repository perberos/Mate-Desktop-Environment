/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; -*- */
/* logview-app.h - logview application singleton
 *
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
 
#ifndef __LOGVIEW_APP_H__
#define __LOGVIEW_APP_H__

#include "config.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define LOGVIEW_TYPE_APP logview_app_get_type()
#define LOGVIEW_APP(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), LOGVIEW_TYPE_APP, LogviewApp))
#define LOGVIEW_APP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), LOGVIEW_TYPE_APP, LogviewAppClass))
#define LOGVIEW_IS_APP(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LOGVIEW_TYPE_APP))
#define LOGVIEW_IS_APP_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), LOGVIEW_TYPE_APP))
#define LOGVIEW_APP_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), LOGVIEW_TYPE_APP, LogviewAppClass))

typedef struct _LogviewApp LogviewApp;
typedef struct _LogviewAppClass LogviewAppClass;
typedef struct _LogviewAppPrivate LogviewAppPrivate;

struct _LogviewApp {
  GObject parent;

  LogviewAppPrivate *priv;
};

struct _LogviewAppClass {
  GObjectClass parent_class;

  void (* app_quit) (LogviewApp *app);
};


GType logview_app_get_type (void);

/* public methods */
LogviewApp * logview_app_get (void);
void         logview_app_initialize (LogviewApp *app,
                                     char **log_files);
void         logview_app_add_error  (LogviewApp *app,
                                     const char *file_path,
                                     const char *secondary);
void         logview_app_add_errors (LogviewApp *app,
                                     GPtrArray *errors);

G_END_DECLS

#endif /* __LOGVIEW_APP_H__ */
