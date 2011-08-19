/* logview-findbar.h - find toolbar for logview
 *
 * Copyright (C) 2004 Vincent Noel <vnoel@cox.net>
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

#ifndef __LOGVIEW_FINDBAR_H__
#define __LOGVIEW_FINDBAR_H__

#include <gtk/gtk.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define LOGVIEW_TYPE_FINDBAR \
  (logview_findbar_get_type ())
#define LOGVIEW_FINDBAR(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), LOGVIEW_TYPE_FINDBAR, LogviewFindbar))
#define LOGVIEW_FINDBAR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), LOGVIEW_TYPE_FINDBAR, LogviewFindbarClass))
#define LOGVIEW_IS_FINDBAR(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LOGVIEW_TYPE_FINDBAR))
#define LOGVIEW_IS_FINDBAR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((obj), LOGVIEW_TYPE_FINDBAR))
#define LOGVIEW_FINDBAR_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), LOGVIEW_TYPE_FINDBAR, LogviewFindbarClass))

typedef struct _LogviewFindbar LogviewFindbar;
typedef struct _LogviewFindbarClass LogviewFindbarClass;
typedef struct _LogviewFindbarPrivate LogviewFindbarPrivate;

struct _LogviewFindbar {
  GtkToolbar parent_instance;
  LogviewFindbarPrivate *priv;
};

struct _LogviewFindbarClass {
  GtkToolbarClass parent_class;

  /* signals */
  void (* previous)     (LogviewFindbar *findbar);
  void (* next)         (LogviewFindbar *findbar);
  void (* close)        (LogviewFindbar *findbar);
  void (* text_changed) (LogviewFindbar *findbar);
};

GType logview_findbar_get_type (void);

/* public methods */
GtkWidget *  logview_findbar_new         (void);
void         logview_findbar_open        (LogviewFindbar *findbar);
const char * logview_findbar_get_text    (LogviewFindbar *findbar);
void         logview_findbar_set_message (LogviewFindbar *findbar,
                                          const char *message);

G_END_DECLS

#endif /* __LOGVIEW_FINDBAR_H__ */
