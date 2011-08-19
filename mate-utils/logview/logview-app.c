/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; -*- */
/* logview-app.c - logview application singleton
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

/* logview-app.c */

#include "logview-app.h"

#include "logview-manager.h"
#include "logview-window.h"
#include "logview-prefs.h"

#include <glib/gi18n.h>

struct _LogviewAppPrivate {
  LogviewPrefs *prefs;
  LogviewManager *manager;
  LogviewWindow *window;
};

enum {
  APP_QUIT,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static LogviewApp *app_singleton = NULL;

G_DEFINE_TYPE (LogviewApp, logview_app, G_TYPE_OBJECT);

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), LOGVIEW_TYPE_APP, LogviewAppPrivate))

static gboolean
main_window_delete_cb (GtkWidget *widget,
                       GdkEvent *event,
                       gpointer user_data)
{
  LogviewApp *app = user_data;

  g_signal_emit (app, signals[APP_QUIT], 0, NULL);

  return FALSE;
}

static gboolean
logview_app_set_window (LogviewApp *app)
{
  LogviewWindow *window;
  gboolean retval = FALSE;

  window = LOGVIEW_WINDOW (logview_window_new ());

  if (window) {
    app->priv->window = window;
    g_signal_connect (window, "delete-event",
                      G_CALLBACK (main_window_delete_cb), app);
    retval = TRUE;
  }

  gtk_window_set_default_icon_name ("logviewer");

  return retval;
}

typedef struct {
  LogviewApp *app;
  GSList *logs;
} EnumerateJob;

/* TODO: ideally we should parse configuration files in /etc/logrotate.conf
 * and all the files in /etc/logrotate.d/ and group all the logs referring
 * to the same entry under a category. Right now, we just do some
 * parsing instead, and fill with quasi-sensible defaults.
 */

/* adapted from sysklogd sources */
static GSList*
parse_syslog ()
{
  char cbuf[BUFSIZ];
  char *cline, *p;
  FILE *cf;
  GSList *logfiles = NULL;

  if ((cf = fopen ("/etc/syslog.conf", "r")) == NULL) {
    return NULL;
  }

  cline = cbuf;
  while (fgets (cline, sizeof (cbuf) - (cline - cbuf), cf) != NULL) {
    gchar **list;
    gint i;

    for (p = cline; g_ascii_isspace (*p); ++p);
    if (*p == '\0' || *p == '#' || *p == '\n')
      continue;

    list = g_strsplit_set (p, ", -\t()\n", 0);

    for (i = 0; list[i]; ++i) {
      if (*list[i] == '/' &&
          g_slist_find_custom (logfiles, list[i],
                               (GCompareFunc) g_ascii_strcasecmp) == NULL)
      {
        logfiles = g_slist_insert (logfiles,
                                   g_strdup (list[i]), 0);
      }
    }

    g_strfreev (list);
  }

  fclose (cf);

  return logfiles;
}

static void
enumerate_job_finish (EnumerateJob *job)
{
  GSList *files = job->logs;
  LogviewApp *app = job->app;

  logview_manager_add_logs_from_name_list (app->priv->manager, files, files->data);

  g_slist_foreach (files, (GFunc) g_free, NULL);
  g_slist_free (files);

  g_object_unref (job->app);
  g_slice_free (EnumerateJob, job);
}

static void
enumerate_next_files_async_cb (GObject *source,
                               GAsyncResult *res,
                               gpointer user_data)
{
  EnumerateJob *job = user_data;
  GList *enumerated_files, *l;
  GFileInfo *info;
  GSList *logs;
  const char *content_type, *name;
  char *parse_string, *container_path;
  GFileType type;
  GFile *container;

  enumerated_files = g_file_enumerator_next_files_finish (G_FILE_ENUMERATOR (source),
                                                          res, NULL);
  if (!enumerated_files) {
    enumerate_job_finish (job);
    return;
  }

  logs = job->logs;
  container = g_file_enumerator_get_container (G_FILE_ENUMERATOR (source));
  container_path = g_file_get_path (container);

  /* TODO: we don't support grouping rotated logs yet, skip gzipped files
   * and those which name contains a formatted date.
   */
  for (l = enumerated_files; l; l = l->next) {
    info = l->data;
    type = g_file_info_get_file_type (info);
    content_type = g_file_info_get_content_type (info);
    name = g_file_info_get_name (info);
    
    if (!g_file_info_get_attribute_boolean (info, "access::can-read")) {
      g_object_unref (info);
      continue;
    }

    if (type != (G_FILE_TYPE_REGULAR || G_FILE_TYPE_SYMBOLIC_LINK) ||
        !g_content_type_is_a (content_type, "text/plain"))
    {
      g_object_unref (info);
      continue;
    }

    if (g_content_type_is_a (content_type, "application/x-gzip")) {
      g_object_unref (info);
      continue;
    }

    if (g_regex_match_simple ("\\d{8}$", name, 0, 0)) {
      g_object_unref (info);
      continue;
    }

    parse_string = g_build_filename (container_path, name, NULL);

    if (g_slist_find_custom (logs, parse_string, (GCompareFunc) g_ascii_strcasecmp) == NULL) {
      logs = g_slist_append (logs, parse_string);
    } else {
      g_free (parse_string);
    }

    g_object_unref (info);
    parse_string = NULL;
  }

  g_list_free (enumerated_files);
  g_object_unref (container);
  g_free (container_path);

  job->logs = logs;

  enumerate_job_finish (job);
}

static void
enumerate_children_async_cb (GObject *source,
                             GAsyncResult *res,
                             gpointer user_data)
{
  EnumerateJob *job = user_data;
  GFileEnumerator *enumerator;

  enumerator = g_file_enumerate_children_finish (G_FILE (source),
                                                 res, NULL);
  if (!enumerator) {
    enumerate_job_finish (job);
    return;
  }

  g_file_enumerator_next_files_async (enumerator, G_MAXINT,
                                      G_PRIORITY_DEFAULT,
                                      NULL, enumerate_next_files_async_cb, job);  
}

static void
logview_app_first_time_initialize (LogviewApp *app)
{
  GSList *logs;
  GFile *log_dir;
  EnumerateJob *job;

  /* let's add all accessible files in /var/log and those mentioned
   * in /etc/syslog.conf.
   */

  logs = parse_syslog ();

  job = g_slice_new0 (EnumerateJob);
  job->app = g_object_ref (app);
  job->logs = logs;

  log_dir = g_file_new_for_path ("/var/log/");
  g_file_enumerate_children_async (log_dir,
                                   "standard::*,access::can-read", 0,
                                   G_PRIORITY_DEFAULT, NULL,
                                   enumerate_children_async_cb, job);

  g_object_unref (log_dir);
}

static void
do_finalize (GObject *obj)
{
  LogviewApp *app = LOGVIEW_APP (obj);

  g_object_unref (app->priv->manager);
  g_object_unref (app->priv->prefs);

  G_OBJECT_CLASS (logview_app_parent_class)->finalize (obj);
}

static void
logview_app_class_init (LogviewAppClass *klass)
{
  GObjectClass *oclass = G_OBJECT_CLASS (klass);

  oclass->finalize = do_finalize;

  signals[APP_QUIT] =
    g_signal_new ("app-quit",
                  G_OBJECT_CLASS_TYPE (oclass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LogviewAppClass, app_quit),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  g_type_class_add_private (klass, sizeof (LogviewAppPrivate));
}

static void
logview_app_init (LogviewApp *self)
{
  LogviewAppPrivate *priv = self->priv = GET_PRIVATE (self);

  priv->prefs = logview_prefs_get ();
  priv->manager = logview_manager_get ();
}

LogviewApp*
logview_app_get (void)
{
  if (!app_singleton) {
    app_singleton = g_object_new (LOGVIEW_TYPE_APP, NULL);

    if (!logview_app_set_window (app_singleton)) {
      g_object_unref (app_singleton);
      app_singleton = NULL;
    }
  }

  return app_singleton;
}

void
logview_app_initialize (LogviewApp *app, char **log_files)
{
  LogviewAppPrivate *priv;

  g_assert (LOGVIEW_IS_APP (app));

  priv = app->priv;

  /* open regular logs and add each log passed as a parameter */

  if (log_files == NULL) {
    char *active_log;
    GSList *logs;

    active_log = logview_prefs_get_active_logfile (priv->prefs);
    logs = logview_prefs_get_stored_logfiles (priv->prefs);

    if (!logs) {
      logview_app_first_time_initialize (app);
    } else {
      logview_manager_add_logs_from_name_list (priv->manager,
                                               logs, active_log);

      g_free (active_log);
      g_slist_foreach (logs, (GFunc) g_free, NULL);
      g_slist_free (logs);
    }
  } else {
    logview_manager_add_logs_from_names (priv->manager, log_files);
  }

  gtk_widget_show (GTK_WIDGET (priv->window));
}

void
logview_app_add_error (LogviewApp *app,
                       const char *file_path,
                       const char *secondary)
{
  LogviewWindow *window;
  char *primary;

  g_assert (LOGVIEW_IS_APP (app));

  window = app->priv->window;
  primary = g_strdup_printf (_("Impossible to open the file %s"), file_path);

  logview_window_add_error (window, primary, secondary);

  g_free (primary);
}

void
logview_app_add_errors (LogviewApp *app,
                        GPtrArray *errors)
{
  LogviewWindow *window;

  g_assert (LOGVIEW_IS_APP (app));

  window = app->priv->window;

  if (errors->len == 0) {
    return;
  } else if (errors->len == 1) {
    char **err;

    err = g_ptr_array_index (errors, 0);
    logview_window_add_error (window, err[0], err[1]);
  } else {
    logview_window_add_errors (window, errors);
  }
}
