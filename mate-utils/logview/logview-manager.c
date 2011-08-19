/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; -*- */
/* logview-manager.c - manager for the opened log objects
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 551 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

/* logview-manager.c */

#include "logview-manager.h"

#include <glib/gi18n.h>

#include "logview-prefs.h"
#include "logview-marshal.h"
#include "logview-app.h"

enum {
  LOG_ADDED,
  LOG_CLOSED,
  ACTIVE_CHANGED,
  LAST_SIGNAL
};

typedef struct {
  LogviewManager *manager;
  gboolean set_active;
  gboolean is_multiple;
  GFile *file;
} CreateCBData;

typedef struct {
  int total;
  int current;
  GPtrArray *errors;
} MultipleCreation;

struct _LogviewManagerPrivate {
  GHashTable *logs;
  LogviewLog *active_log;
};

static LogviewManager *singleton = NULL;
static MultipleCreation *op = NULL;
static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (LogviewManager, logview_manager, G_TYPE_OBJECT);

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), LOGVIEW_TYPE_MANAGER, LogviewManagerPrivate))

static void
logview_manager_finalize (GObject *object)
{
  LogviewManager *manager;

  manager = LOGVIEW_MANAGER (object);

  if (manager->priv->active_log) {
   g_object_unref (manager->priv->active_log);
  }

  g_hash_table_destroy (manager->priv->logs);

  G_OBJECT_CLASS (logview_manager_parent_class)->finalize (object);
}

static void
logview_manager_class_init (LogviewManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = logview_manager_finalize;

  signals[LOG_ADDED] = g_signal_new ("log-added",
                                     G_OBJECT_CLASS_TYPE (object_class),
                                     G_SIGNAL_RUN_LAST,
                                     G_STRUCT_OFFSET (LogviewManagerClass, log_added),
                                     NULL, NULL,
                                     g_cclosure_marshal_VOID__OBJECT,
                                     G_TYPE_NONE, 1,
                                     LOGVIEW_TYPE_LOG);

  signals[LOG_CLOSED] = g_signal_new ("log-closed",
                                      G_OBJECT_CLASS_TYPE (object_class),
                                      G_SIGNAL_RUN_LAST,
                                      G_STRUCT_OFFSET (LogviewManagerClass, log_closed),
                                      NULL, NULL,
                                      g_cclosure_marshal_VOID__OBJECT,
                                      G_TYPE_NONE, 1,
                                      LOGVIEW_TYPE_LOG);

  signals[ACTIVE_CHANGED] = g_signal_new ("active-changed",
                                          G_OBJECT_CLASS_TYPE (object_class),
                                          G_SIGNAL_RUN_LAST,
                                          G_STRUCT_OFFSET (LogviewManagerClass, active_changed),
                                          NULL, NULL,
                                          logview_marshal_VOID__OBJECT_OBJECT,
                                          G_TYPE_NONE, 2,
                                          LOGVIEW_TYPE_LOG,
                                          LOGVIEW_TYPE_LOG);

  g_type_class_add_private (klass, sizeof (LogviewManagerPrivate));
}

static void
logview_manager_init (LogviewManager *self)
{
  LogviewManagerPrivate *priv = self->priv = GET_PRIVATE (self);

  priv->active_log = NULL;
  priv->logs = g_hash_table_new_full (g_str_hash, g_str_equal, 
                                      (GDestroyNotify) g_free, (GDestroyNotify) g_object_unref);
}

static MultipleCreation *
multiple_creation_op_new (int total)
{
  MultipleCreation *retval;

  retval = g_slice_new0 (MultipleCreation);
  retval->total = total;
  retval->current = 0;
  retval->errors = g_ptr_array_new ();

  return retval;
}

static void
multiple_creation_op_free (MultipleCreation *mc)
{
  g_ptr_array_foreach (mc->errors, (GFunc) g_strfreev, NULL);
  g_ptr_array_free (mc->errors, TRUE);

  g_slice_free (MultipleCreation, mc);
}

static void
create_log_cb (LogviewLog *log,
               GError *error,
               gpointer user_data)
{
  CreateCBData *data = user_data;

  if (log) {
    char *log_uri;
    LogviewPrefs *prefs;
    GFile *file;

    log_uri = logview_log_get_uri (log);

    /* creation went well, store the log and notify */
    g_hash_table_insert (data->manager->priv->logs,
                         log_uri, log);

    prefs = logview_prefs_get ();
    file = logview_log_get_gfile (log);
    logview_prefs_store_log (prefs, file);

    g_object_unref (file);

    g_signal_emit (data->manager, signals[LOG_ADDED], 0, log, NULL);

    if (data->set_active) {
      logview_manager_set_active_log (data->manager, log);
    }
  } else {
    char *path;

    /* notify the error */
    path = g_file_get_path (data->file);

    if (!data->is_multiple) {
      logview_app_add_error (logview_app_get (),
                             path, error->message);
    } else {
      char **error_arr = g_new0 (char *, 3);

      error_arr[0] = g_strdup (path);
      error_arr[1] = g_strdup (error->message);
      error_arr[2] = NULL;

      g_ptr_array_add (op->errors, error_arr);
    }

    g_free (path);
  }
  
  if (data->is_multiple) {
    op->current++;

    if (op->total == op->current) {
      logview_app_add_errors (logview_app_get (), op->errors);
      multiple_creation_op_free (op);
      op = NULL;
    }
  }

  g_object_unref (data->file);
  g_slice_free (CreateCBData, data);
}

static void
add_log_from_gfile_internal (LogviewManager *manager,
                             GFile *file,
                             gboolean set_active,
                             gboolean is_multiple)
{
  char *file_uri;
  LogviewLog *log;
  CreateCBData *data;

  file_uri = g_file_get_uri (file);

  if (set_active == FALSE) {
    /* if it's the first log being added, set it as active anyway */
    set_active = (manager->priv->logs == NULL);
  }

  if ((log = g_hash_table_lookup (manager->priv->logs, file_uri)) != NULL) {
    /* log already exists, don't load it */
    if (set_active) {
      logview_manager_set_active_log (manager, log);
    }
  } else {
    data = g_slice_new0 (CreateCBData);
    data->manager = manager;
    data->set_active = set_active;
    data->is_multiple = is_multiple;
    data->file = g_object_ref (file);

    logview_log_create_from_gfile (file, create_log_cb, data);
  }

  g_free (file_uri);
}

static void
logview_manager_add_log_from_name (LogviewManager *manager,
                                   const char *filename, gboolean set_active,
                                   gboolean is_multiple)
{
  GFile *file;

  file = g_file_new_for_path (filename);

  add_log_from_gfile_internal (manager, file, set_active, is_multiple);

  g_object_unref (file);
}

/* public methods */

LogviewManager*
logview_manager_get (void)
{
  if (!singleton) {
    singleton = g_object_new (LOGVIEW_TYPE_MANAGER, NULL);
  }

  return singleton;
}

void
logview_manager_set_active_log (LogviewManager *manager,
                                LogviewLog *log)
{
  LogviewLog *old_log = NULL;

  g_assert (LOGVIEW_IS_MANAGER (manager));

  if (manager->priv->active_log) {
    old_log = manager->priv->active_log;
  }

  manager->priv->active_log = g_object_ref (log);

  g_signal_emit (manager, signals[ACTIVE_CHANGED], 0, log, old_log, NULL);

  if (old_log) {
    g_object_unref (old_log);
  }
}

LogviewLog *
logview_manager_get_active_log (LogviewManager *manager)
{
  g_assert (LOGVIEW_IS_MANAGER (manager));

  return (manager->priv->active_log != NULL) ?
    g_object_ref (manager->priv->active_log) :
    NULL;
}

void
logview_manager_add_log_from_gfile (LogviewManager *manager,
                                    GFile *file,
                                    gboolean set_active)
{
  g_assert (LOGVIEW_IS_MANAGER (manager));

  add_log_from_gfile_internal (manager, file, set_active, FALSE);
}

void
logview_manager_add_logs_from_name_list (LogviewManager *manager,
                                         GSList *names,
                                         const char *active)
{
  GSList *l;

  g_assert (LOGVIEW_IS_MANAGER (manager));
  g_assert (op == NULL);

  op = multiple_creation_op_new (g_slist_length (names));

  for (l = names; l; l = l->next) {
    logview_manager_add_log_from_name (manager, l->data,
                                       (g_ascii_strcasecmp (active, l->data) == 0),
                                       TRUE);
  }
}

void
logview_manager_add_logs_from_names (LogviewManager *manager,
                                     char ** names)
{
  int i;

  g_assert (LOGVIEW_IS_MANAGER (manager));
  g_assert (op == NULL);

  op = multiple_creation_op_new (G_N_ELEMENTS (names));

  for (i = 0; names[i]; i++) {
    logview_manager_add_log_from_name (manager, names[i], FALSE, TRUE);
  }
}

int
logview_manager_get_log_count (LogviewManager *manager)
{
  g_assert (LOGVIEW_IS_MANAGER (manager));

  return g_hash_table_size (manager->priv->logs);
}

LogviewLog *
logview_manager_get_if_loaded (LogviewManager *manager, char *uri)
{
  LogviewLog *log;

  g_assert (LOGVIEW_IS_MANAGER (manager));

  log = g_hash_table_lookup (manager->priv->logs, uri);

  if (log != NULL) {
    return g_object_ref (log);
  }

  return NULL;
}

void
logview_manager_close_active_log (LogviewManager *manager)
{
  LogviewLog *active_log;
  char *log_uri;
  GFile *file;

  g_assert (LOGVIEW_IS_MANAGER (manager));

  active_log = manager->priv->active_log;
  log_uri = logview_log_get_uri (active_log);
  file = logview_log_get_gfile (active_log);

  g_signal_emit (manager, signals[LOG_CLOSED], 0, active_log, NULL);

  logview_prefs_remove_stored_log (logview_prefs_get (), file);

  g_object_unref (file);

  /* we own two refs to the active log; one is inside the hash table */
  g_object_unref (active_log);
  g_hash_table_remove (manager->priv->logs, log_uri);

  g_free (log_uri);

  /* someone else will take care of setting the next active log to us */
}

gboolean
logview_manager_log_is_active (LogviewManager *manager,
                               LogviewLog *log)
{
  g_assert (LOGVIEW_IS_MANAGER (manager));

  return (manager->priv->active_log == log);
}
