/* logview-prefs.h - logview user preferences handling
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

#ifndef __LOGVIEW_PREFS_H__
#define __LOGVIEW_PREFS_H__

#include "logview-filter.h"

#define LOGVIEW_TYPE_PREFS logview_prefs_get_type()
#define LOGVIEW_PREFS(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), LOGVIEW_TYPE_PREFS, LogviewPrefs))
#define LOGVIEW_PREFS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), LOGVIEW_TYPE_PREFS, LogviewPrefsClass))
#define LOGVIEW_IS_PREFS(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LOGVIEW_TYPE_PREFS))
#define LOGVIEW_IS_PREFS_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), LOGVIEW_TYPE_PREFS))
#define LOGVIEW_PREFS_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), LOGVIEW_TYPE_PREFS, LogviewPrefsClass))

typedef struct _LogviewPrefs LogviewPrefs;
typedef struct _LogviewPrefsClass LogviewPrefsClass;
typedef struct _LogviewPrefsPrivate LogviewPrefsPrivate;

struct _LogviewPrefs {
  GObject parent;
  LogviewPrefsPrivate *priv;
};

struct _LogviewPrefsClass {
  GObjectClass parent_class;

  /* signals */
  void (* system_font_changed)  (LogviewPrefs *prefs,
                                 const char *font_name);
  void (* have_tearoff_changed) (LogviewPrefs *prefs,
                                 gboolean have_tearoff);
  void (* filters_changed)      (LogviewPrefs *prefs);
};

GType          logview_prefs_get_type (void);

/* public methods */

LogviewPrefs *  logview_prefs_get (void);
void            logview_prefs_store_window_size       (LogviewPrefs *prefs,
                                                       int width, int height);
void            logview_prefs_get_stored_window_size  (LogviewPrefs *prefs,
                                                       int *width, int *height);
char *          logview_prefs_get_monospace_font_name (LogviewPrefs *prefs);
gboolean        logview_prefs_get_have_tearoff        (LogviewPrefs *prefs);
void            logview_prefs_store_log               (LogviewPrefs *prefs,
                                                       GFile *file);
void            logview_prefs_remove_stored_log       (LogviewPrefs *prefs,
                                                       GFile *target);
GSList *        logview_prefs_get_stored_logfiles     (LogviewPrefs *prefs);
void            logview_prefs_store_fontsize          (LogviewPrefs *prefs,
                                                       int fontsize);
int             logview_prefs_get_stored_fontsize     (LogviewPrefs *prefs);
void            logview_prefs_store_active_logfile    (LogviewPrefs *prefs,
                                                       const char *filename);
char *          logview_prefs_get_active_logfile      (LogviewPrefs *prefs);

GList *         logview_prefs_get_filters             (LogviewPrefs *prefs);
void            logview_prefs_remove_filter           (LogviewPrefs *prefs,
                                                       const gchar* name);
void            logview_prefs_add_filter              (LogviewPrefs *prefs,
                                                       LogviewFilter *filter);
LogviewFilter * logview_prefs_get_filter              (LogviewPrefs *prefs,
                                                       const gchar *name);

#endif /* __LOG_PREFS_H__ */
