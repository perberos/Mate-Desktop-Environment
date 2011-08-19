/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; -*- */
/* logview-prefs.c - logview user preferences handling
 *
 * Copyright (C) 1998  Cesar Miquel  <miquel@df.uba.ar>
 * Copyright (C) 2004  Vincent Noel
 * Copyright (C) 2006  Emmanuele Bassi
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/stat.h>
#include <string.h>
#include <gtk/gtk.h>
#include <mateconf/mateconf-client.h>

#include "logview-prefs.h"

#define LOGVIEW_DEFAULT_HEIGHT 400
#define LOGVIEW_DEFAULT_WIDTH 600

/* logview settings */
#define MATECONF_DIR 		"/apps/mate-system-log"
#define MATECONF_WIDTH_KEY 	MATECONF_DIR "/width"
#define MATECONF_HEIGHT_KEY 	MATECONF_DIR "/height"
#define MATECONF_LOGFILE 		MATECONF_DIR "/logfile"
#define MATECONF_LOGFILES 		MATECONF_DIR "/logfiles"
#define MATECONF_FONTSIZE_KEY 	MATECONF_DIR "/fontsize"
#define MATECONF_FILTERS     MATECONF_DIR "/filters"

/* desktop-wide settings */
#define MATECONF_MONOSPACE_FONT_NAME "/desktop/mate/interface/monospace_font_name"
#define MATECONF_MENUS_HAVE_TEAROFF  "/desktop/mate/interface/menus_have_tearoff"

static LogviewPrefs *singleton = NULL;

enum {
  SYSTEM_FONT_CHANGED,
  HAVE_TEAROFF_CHANGED,
  LAST_SIGNAL
};

enum {
  FILTER_NAME,
  FILTER_INVISIBLE,
  FILTER_FOREGROUND,
  FILTER_BACKGROUND,
  FILTER_REGEX,
  MAX_TOKENS
};

static guint signals[LAST_SIGNAL] = { 0 };

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), LOGVIEW_TYPE_PREFS, LogviewPrefsPrivate))

struct _LogviewPrefsPrivate {
  MateConfClient *client;

  guint size_store_timeout;

  GHashTable *filters;
};

typedef struct {
  int width;
  int height;
} WindowSize;

G_DEFINE_TYPE (LogviewPrefs, logview_prefs, G_TYPE_OBJECT);

static void
do_finalize (GObject *obj)
{
  LogviewPrefs *prefs = LOGVIEW_PREFS (obj);

  g_hash_table_destroy (prefs->priv->filters);

  g_object_unref (prefs->priv->client);

  G_OBJECT_CLASS (logview_prefs_parent_class)->finalize (obj);
}

static void
logview_prefs_class_init (LogviewPrefsClass *klass)
{
  GObjectClass *oclass = G_OBJECT_CLASS (klass);

  oclass->finalize = do_finalize;

  signals[SYSTEM_FONT_CHANGED] = g_signal_new ("system-font-changed",
                                               G_OBJECT_CLASS_TYPE (oclass),
                                               G_SIGNAL_RUN_LAST,
                                               G_STRUCT_OFFSET (LogviewPrefsClass, system_font_changed),
                                               NULL, NULL,
                                               g_cclosure_marshal_VOID__STRING,
                                               G_TYPE_NONE, 1,
                                               G_TYPE_STRING);
  signals[HAVE_TEAROFF_CHANGED] = g_signal_new ("have-tearoff-changed",
                                                G_OBJECT_CLASS_TYPE (oclass),
                                                G_SIGNAL_RUN_LAST,
                                                G_STRUCT_OFFSET (LogviewPrefsClass, have_tearoff_changed),
                                                NULL, NULL,
                                                g_cclosure_marshal_VOID__BOOLEAN,
                                                G_TYPE_NONE, 1,
                                                G_TYPE_BOOLEAN);

  g_type_class_add_private (klass, sizeof (LogviewPrefsPrivate));
}

static void
have_tearoff_changed_cb (MateConfClient *client,
                         guint id,
                         MateConfEntry *entry,
                         gpointer data)
{
  LogviewPrefs *prefs = data;

  if (entry->value && (entry->value->type == MATECONF_VALUE_BOOL)) {
    gboolean add_tearoffs;

    add_tearoffs = mateconf_value_get_bool (entry->value);
    g_signal_emit (prefs, signals[HAVE_TEAROFF_CHANGED], 0, add_tearoffs, NULL);
  }
}

static void
monospace_font_changed_cb (MateConfClient *client,
                           guint id,
                           MateConfEntry *entry,
                           gpointer data)
{
  LogviewPrefs *prefs = data;

  if (entry->value && (entry->value->type == MATECONF_VALUE_STRING)) {
    const gchar *monospace_font_name;

    monospace_font_name = mateconf_value_get_string (entry->value);
    g_signal_emit (prefs, signals[SYSTEM_FONT_CHANGED], 0, monospace_font_name, NULL);
  }
}

static gboolean
size_store_timeout_cb (gpointer data)
{
  WindowSize *size = data;
  LogviewPrefs *prefs = logview_prefs_get ();

  if (size->width > 0 && size->height > 0) {
    if (mateconf_client_key_is_writable (prefs->priv->client, MATECONF_WIDTH_KEY, NULL))
      mateconf_client_set_int (prefs->priv->client,
                            MATECONF_WIDTH_KEY,
                            size->width,
                            NULL);

    if (mateconf_client_key_is_writable (prefs->priv->client, MATECONF_HEIGHT_KEY, NULL))
      mateconf_client_set_int (prefs->priv->client,
                            MATECONF_HEIGHT_KEY,
                            size->height,
                            NULL);
  }

  /* reset the source id */
  prefs->priv->size_store_timeout = 0;

  g_free (size);

  return FALSE;
}

#define DELIMITER ":"

static void
load_filters (LogviewPrefs *prefs)
{
  GSList *node; 
  GSList *filters;
  gchar **tokens;
  LogviewFilter *filter;
  GtkTextTag *tag;
  GdkColor color;

  filters = mateconf_client_get_list (prefs->priv->client,
                                   MATECONF_FILTERS,
                                   MATECONF_VALUE_STRING,
                                   NULL);

  prefs->priv->filters = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                (GDestroyNotify) g_free,
                                                (GDestroyNotify) g_object_unref);

  for (node = filters; node != NULL; node = g_slist_next (node)) {
    tokens = g_strsplit (node->data, DELIMITER, MAX_TOKENS);
    filter = logview_filter_new (tokens[FILTER_NAME], tokens[FILTER_REGEX]);
    tag = gtk_text_tag_new (tokens[FILTER_NAME]);

    g_object_set (tag, "invisible",
                  g_str_equal (tokens[FILTER_INVISIBLE], "1"), NULL);

    if (strlen (tokens[FILTER_FOREGROUND])) {
      gdk_color_parse (tokens[FILTER_FOREGROUND], &color);
      g_object_set (tag, "foreground-gdk", &color, 
                    "foreground-set", TRUE, NULL);
    }

    if (strlen (tokens[FILTER_BACKGROUND])) {
      gdk_color_parse (tokens[FILTER_BACKGROUND], &color);
      g_object_set (tag, "paragraph-background-gdk", &color, 
                    "paragraph-background-set", TRUE, NULL);
    }

    g_object_set (filter, "texttag", tag, NULL);
    g_hash_table_insert (prefs->priv->filters, 
                         g_strdup(tokens[FILTER_NAME]), 
                         filter);

    g_object_ref (filter);
    g_object_unref (tag);
    g_strfreev (tokens);
  }

  g_slist_foreach (filters, (GFunc) g_free, NULL);
  g_slist_free (filters);
}

static void
save_filter_foreach_func (gpointer key, gpointer value, gpointer user_data)
{
  GSList **filters;
  const gchar *name;
  LogviewFilter *filter;
  GdkColor *foreground;
  gboolean foreground_set;
  GdkColor *background;
  gboolean background_set;
  gchar *regex, *color;
  gboolean invisible;
  GtkTextTag *tag;
  GString *prefs_string;

  filters = user_data;
  filter = LOGVIEW_FILTER (value);
  name = key;
  color = NULL;

  prefs_string = g_string_new (name);
  g_string_append (prefs_string, DELIMITER);

  g_object_get (filter,
                "regex", &regex,
                "texttag", &tag,
                NULL);
  g_object_get (tag,
                "foreground-gdk", &foreground,
                "paragraph-background-gdk", &background,
                "foreground-set", &foreground_set,
                "paragraph-background-set", &background_set,
                "invisible", &invisible, NULL);

  if (invisible) {
    g_string_append (prefs_string, "1" DELIMITER);
  } else {
    g_string_append (prefs_string, "0" DELIMITER);
  }

  if (foreground_set) {
    color = gdk_color_to_string (foreground);
    g_string_append (prefs_string, color);
    g_free (color);
  }

  if (foreground) {
    gdk_color_free (foreground);
  }

  g_string_append (prefs_string, DELIMITER);

  if (background_set) {
    color = gdk_color_to_string (background);
    g_string_append (prefs_string, color);
    g_free (color);
  }

  if (background) {
    gdk_color_free (background);
  }

  g_string_append (prefs_string, DELIMITER);
  g_string_append (prefs_string, regex);

  g_free (regex);
  g_object_unref (tag);
  
  *filters = g_slist_prepend (*filters, g_string_free (prefs_string, FALSE));
} 

static void
save_filters (LogviewPrefs *prefs)
{
  GSList *filters;

  filters = NULL;

  g_hash_table_foreach (prefs->priv->filters,
                        save_filter_foreach_func,
                        &filters);
  mateconf_client_set_list (prefs->priv->client,
                         MATECONF_FILTERS,
                         MATECONF_VALUE_STRING,
                         filters, NULL);

  g_slist_foreach (filters, (GFunc) g_free, NULL);
  g_slist_free (filters);
}

static void
get_filters_foreach (gpointer key, gpointer value, gpointer user_data)
{
  GList **list;
  list = user_data;
  *list = g_list_append (*list, value);
}

static void
logview_prefs_init (LogviewPrefs *self)
{
  LogviewPrefsPrivate *priv;

  priv = self->priv = GET_PRIVATE (self);

  priv->client = mateconf_client_get_default ();
  priv->size_store_timeout = 0;

  mateconf_client_notify_add (priv->client,
                           MATECONF_MONOSPACE_FONT_NAME,
                           (MateConfClientNotifyFunc) monospace_font_changed_cb,
                           self, NULL, NULL);
  mateconf_client_notify_add (priv->client,
                           MATECONF_MENUS_HAVE_TEAROFF,
                           (MateConfClientNotifyFunc) have_tearoff_changed_cb,
                           self, NULL, NULL);

  load_filters (self);
}

/* public methods */

LogviewPrefs *
logview_prefs_get ()
{
  if (!singleton)
    singleton = g_object_new (LOGVIEW_TYPE_PREFS, NULL);

  return singleton;
}

void
logview_prefs_store_window_size (LogviewPrefs *prefs,
                                 int width, int height)
{
  /* we want to be smart here: since we will get a lot of configure events
   * while resizing, we schedule the real MateConf storage in a timeout.
   */
  WindowSize *size;

  g_assert (LOGVIEW_IS_PREFS (prefs));

  size = g_new0 (WindowSize, 1);
  size->width = width;
  size->height = height;

  if (prefs->priv->size_store_timeout != 0) {
    /* reschedule the timeout */
    g_source_remove (prefs->priv->size_store_timeout);
    prefs->priv->size_store_timeout = 0;
  }

  prefs->priv->size_store_timeout = g_timeout_add (200,
                                                   size_store_timeout_cb,
                                                   size);
}

void
logview_prefs_get_stored_window_size (LogviewPrefs *prefs,
                                      int *width, int *height)
{
  g_assert (LOGVIEW_IS_PREFS (prefs));

  *width = mateconf_client_get_int (prefs->priv->client,
                                 MATECONF_WIDTH_KEY,
                                 NULL);

  *height = mateconf_client_get_int (prefs->priv->client,
                                  MATECONF_HEIGHT_KEY,
                                  NULL);

  if ((*width == 0) ^ (*height == 0)) {
    /* if one of the two failed, return default for both */
    *width = LOGVIEW_DEFAULT_WIDTH;
    *height = LOGVIEW_DEFAULT_HEIGHT;
  }
}

char *
logview_prefs_get_monospace_font_name (LogviewPrefs *prefs)
{
  g_assert (LOGVIEW_IS_PREFS (prefs));

  return (mateconf_client_get_string (prefs->priv->client, MATECONF_MONOSPACE_FONT_NAME, NULL));
}

gboolean
logview_prefs_get_have_tearoff (LogviewPrefs *prefs)
{
  g_assert (LOGVIEW_IS_PREFS (prefs));

	return (mateconf_client_get_bool (prefs->priv->client, MATECONF_MENUS_HAVE_TEAROFF, NULL));
}

/* the elements should be freed with g_free () */

GSList *
logview_prefs_get_stored_logfiles (LogviewPrefs *prefs)
{
  GSList *retval;

  g_assert (LOGVIEW_IS_PREFS (prefs));

  retval = mateconf_client_get_list (prefs->priv->client,
                                  MATECONF_LOGFILES,
                                  MATECONF_VALUE_STRING,
                                  NULL);
  return retval;
}

void
logview_prefs_store_log (LogviewPrefs *prefs, GFile *file)
{
  GSList *stored_logs, *l;
  GFile *stored;
  gboolean found = FALSE;

  g_assert (LOGVIEW_IS_PREFS (prefs));
  g_assert (G_IS_FILE (file));

  stored_logs = logview_prefs_get_stored_logfiles (prefs);

  for (l = stored_logs; l; l = l->next) {
    stored = g_file_parse_name (l->data);
    if (g_file_equal (file, stored)) {
      found = TRUE;
    }

    g_object_unref (stored);

    if (found) {
      break;
    }
  }

  if (!found) {
    stored_logs = g_slist_prepend (stored_logs, g_file_get_parse_name (file));
    mateconf_client_set_list (prefs->priv->client,
                           MATECONF_LOGFILES,
                           MATECONF_VALUE_STRING,
                           stored_logs,
                           NULL);
  }

  /* the string list is copied */
  g_slist_foreach (stored_logs, (GFunc) g_free, NULL);
  g_slist_free (stored_logs);
}

void
logview_prefs_remove_stored_log (LogviewPrefs *prefs, GFile *target)
{
  GSList *stored_logs, *l, *removed = NULL;
  GFile *stored;

  g_assert (LOGVIEW_IS_PREFS (prefs));
  g_assert (G_IS_FILE (target));

  stored_logs = logview_prefs_get_stored_logfiles (prefs);

  for (l = stored_logs; l; l = l->next) {
    stored = g_file_parse_name (l->data);
    if (g_file_equal (stored, target)) {
      removed = l;
      stored_logs = g_slist_remove_link (stored_logs, l);
    }

    g_object_unref (stored);

    if (removed) {
      break;
    }
  }

  if (removed) {
    mateconf_client_set_list (prefs->priv->client,
                           MATECONF_LOGFILES,
                           MATECONF_VALUE_STRING,
                           stored_logs,
                           NULL);
  }

  /* the string list is copied */
  g_slist_foreach (stored_logs, (GFunc) g_free, NULL);
  g_slist_free (stored_logs);

  if (removed) {
    g_free (removed->data);
    g_slist_free (removed);
  }
}

void
logview_prefs_store_fontsize (LogviewPrefs *prefs, int fontsize)
{
  g_assert (LOGVIEW_IS_PREFS (prefs));
  g_assert (fontsize > 0);

  if (mateconf_client_key_is_writable (prefs->priv->client, MATECONF_FONTSIZE_KEY, NULL)) {
    mateconf_client_set_int (prefs->priv->client,
                          MATECONF_FONTSIZE_KEY,
                          fontsize, NULL);
  }
}

int
logview_prefs_get_stored_fontsize (LogviewPrefs *prefs)
{
  g_assert (LOGVIEW_IS_PREFS (prefs));

	return mateconf_client_get_int (prefs->priv->client, MATECONF_FONTSIZE_KEY, NULL);
}

void
logview_prefs_store_active_logfile (LogviewPrefs *prefs,
                                    const char *filename)
{
  g_assert (LOGVIEW_IS_PREFS (prefs));

  if (mateconf_client_key_is_writable (prefs->priv->client, MATECONF_LOGFILE, NULL)) {
    mateconf_client_set_string (prefs->priv->client,
                             MATECONF_LOGFILE,
                             filename,
                             NULL);
  }
}

char *
logview_prefs_get_active_logfile (LogviewPrefs *prefs)
{
  char *filename;

  g_assert (LOGVIEW_IS_PREFS (prefs));

  filename = mateconf_client_get_string (prefs->priv->client, MATECONF_LOGFILE, NULL);

  return filename;
}

GList *
logview_prefs_get_filters (LogviewPrefs *prefs)
{
  GList *filters = NULL;

  g_assert (LOGVIEW_IS_PREFS (prefs));

  g_hash_table_foreach (prefs->priv->filters,
                        get_filters_foreach,
                        &filters);

  return filters;
}

void
logview_prefs_remove_filter (LogviewPrefs *prefs,
                             const gchar *name)
{
  g_assert (LOGVIEW_IS_PREFS (prefs));

  g_hash_table_remove (prefs->priv->filters,
                       name);

  save_filters (prefs);
}

void
logview_prefs_add_filter (LogviewPrefs *prefs,
                          LogviewFilter *filter)
{
  gchar* name;

  g_assert (LOGVIEW_IS_PREFS (prefs));
  g_assert (LOGVIEW_IS_FILTER (filter));

  g_object_get (filter, "name", &name, NULL);
  g_hash_table_insert (prefs->priv->filters, name, g_object_ref (filter));

  save_filters (prefs);
}

LogviewFilter *
logview_prefs_get_filter (LogviewPrefs *prefs,
                          const gchar *name)
{
  g_assert (LOGVIEW_IS_PREFS (prefs));

  return g_hash_table_lookup (prefs->priv->filters, name);
}

