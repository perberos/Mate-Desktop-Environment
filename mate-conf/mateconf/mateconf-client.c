/* -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* MateConf
 * Copyright (C) 1999, 2000, 2000 Red Hat Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <config.h>
#include <stdio.h>
#include <string.h>

#include "mateconf-client.h"
#include "mateconf/mateconf-internals.h"

#include "mateconfmarshal.h"
#include "mateconfmarshal.c"

static gboolean do_trace = FALSE;

static void
trace (const char *format, ...)
{
  va_list args;
  gchar *str;

  if (!do_trace)
    return;
  
  g_return_if_fail (format != NULL);
  
  va_start (args, format);
  str = g_strdup_vprintf (format, args);
  va_end (args);

  g_message ("%s", str);
  
  g_free (str);
}

/*
 * Error handler override
 */

static MateConfClientErrorHandlerFunc global_error_handler = NULL;

/**
 * mateconf_client_set_global_default_error_handler: (skip)
 * @func: pointer to the function to be called for error handling.
 *
 * Set @func as the default error handler for the #MateConfClient. This handler would be called
 * for all #MateConfClient internal errors.
 */
void
mateconf_client_set_global_default_error_handler(MateConfClientErrorHandlerFunc func)
{
  global_error_handler = func;
}

/*
 * Dir object (for list of directories we're watching)
 */

typedef struct _Dir Dir;

struct _Dir {
  gchar* name;
  guint notify_id;
  /* number of times this dir has been added */
  guint add_count;
};

static Dir* dir_new(const gchar* name, guint notify_id);
static void dir_destroy(Dir* d);

/*
 * Listener object
 */

typedef struct _Listener Listener;

struct _Listener {
  MateConfClientNotifyFunc func;
  gpointer data;
  GFreeFunc destroy_notify;
};

static Listener* listener_new(MateConfClientNotifyFunc func,
                              GFreeFunc destroy_notify,
                              gpointer data);

static void listener_destroy(Listener* l);

/*
 * MateConfClient proper
 */

#define PUSH_USE_ENGINE(client) do { if ((client)->engine) mateconf_engine_push_owner_usage ((client)->engine, client); } while (0)
#define POP_USE_ENGINE(client) do { if ((client)->engine) mateconf_engine_pop_owner_usage ((client)->engine, client); } while (0)

enum {
  VALUE_CHANGED,
  UNRETURNED_ERROR,
  ERROR,
  LAST_SIGNAL
};

static void         register_client   (MateConfClient *client);
static MateConfClient *lookup_client     (MateConfEngine *engine);
static void         unregister_client (MateConfClient *client);

static void mateconf_client_class_init (MateConfClientClass *klass);
static void mateconf_client_init       (MateConfClient      *client);
static void mateconf_client_real_unreturned_error (MateConfClient* client, GError* error);
static void mateconf_client_real_error            (MateConfClient* client, GError* error);
static void mateconf_client_finalize              (GObject* object); 

static gboolean mateconf_client_cache          (MateConfClient *client,
                                             gboolean     take_ownership,
                                             MateConfEntry  *entry,
                                             gboolean    preserve_schema_name);

static gboolean mateconf_client_lookup         (MateConfClient *client,
                                             const char  *key,
                                             MateConfEntry **entryp);

static void mateconf_client_real_remove_dir    (MateConfClient* client,
                                             Dir* d,
                                             GError** err);

static void mateconf_client_queue_notify       (MateConfClient *client,
                                             const char  *key);
static void mateconf_client_flush_notifies     (MateConfClient *client);
static void mateconf_client_unqueue_notifies   (MateConfClient *client);
static void notify_one_entry (MateConfClient *client, MateConfEntry  *entry);

static MateConfEntry* get (MateConfClient  *client,
                        const gchar  *key,
                        gboolean      use_default,
                        GError      **error);


static gpointer parent_class = NULL;
static guint client_signals[LAST_SIGNAL] = { 0 };

GType
mateconf_client_get_type (void)
{
  static GType client_type = 0;

  if (!client_type)
    {
      static const GTypeInfo client_info =
      {
        sizeof (MateConfClientClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) mateconf_client_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (MateConfClient),
        0,              /* n_preallocs */
        (GInstanceInitFunc) mateconf_client_init
      };

      client_type = g_type_register_static (G_TYPE_OBJECT,
                                            "MateConfClient",
                                            &client_info,
                                            0);
    }

  return client_type;
}

static void
mateconf_client_class_init (MateConfClientClass *class)
{
  GObjectClass *object_class;

  object_class = (GObjectClass*) class;

  parent_class = g_type_class_peek_parent (class);

  client_signals[VALUE_CHANGED] =
    g_signal_new ("value_changed",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MateConfClientClass, value_changed),
                  NULL, NULL,
                  mateconf_marshal_VOID__STRING_POINTER,
                  G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_POINTER);

  client_signals[UNRETURNED_ERROR] =
    g_signal_new ("unreturned_error",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MateConfClientClass, unreturned_error),
                  NULL, NULL,
                  mateconf_marshal_VOID__POINTER,
                  G_TYPE_NONE, 1, G_TYPE_POINTER);

  client_signals[ERROR] =
    g_signal_new ("error",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (MateConfClientClass, error),
                  NULL, NULL,
                  mateconf_marshal_VOID__POINTER,
                  G_TYPE_NONE, 1, G_TYPE_POINTER);
  
  class->value_changed    = NULL;
  class->unreturned_error = mateconf_client_real_unreturned_error;
  class->error            = mateconf_client_real_error;

  object_class->finalize  = mateconf_client_finalize;

  if (g_getenv ("MATECONF_DEBUG_TRACE_CLIENT") != NULL)
    do_trace = TRUE;
}

static void
mateconf_client_init (MateConfClient *client)
{
  client->engine = NULL;
  client->error_mode = MATECONF_CLIENT_HANDLE_UNRETURNED;
  client->dir_hash = g_hash_table_new (g_str_hash, g_str_equal);
  client->cache_hash = g_hash_table_new (g_str_hash, g_str_equal);
  client->cache_dirs = g_hash_table_new_full (g_str_hash, g_str_equal,
					      g_free, NULL);
  /* We create the listeners only if they're actually used */
  client->listeners = NULL;
  client->notify_list = NULL;
  client->notify_handler = 0;
}

static gboolean
destroy_dir_foreach_remove(gpointer key, gpointer value, gpointer user_data)
{
  MateConfClient *client = user_data;
  Dir* d = value;
  
  /* remove notify for this dir */
  
  if (d->notify_id != 0)
    {
      trace ("REMOTED: Removing notify ID %u from engine", d->notify_id);
      PUSH_USE_ENGINE (client);
	  mateconf_engine_notify_remove (client->engine, d->notify_id);
      POP_USE_ENGINE (client);
    }
  
  d->notify_id = 0;

  dir_destroy(value);

  return TRUE;
}

static void
set_engine (MateConfClient *client,
            MateConfEngine *engine)
{
  if (engine == client->engine)
    return;
  
  if (engine)
    {
      mateconf_engine_ref (engine);

      mateconf_engine_set_owner (engine, client);
    }
  
  if (client->engine)
    {
      mateconf_engine_set_owner (client->engine, NULL);
      
      mateconf_engine_unref (client->engine);
    }
  
  client->engine = engine;  
}

static void
mateconf_client_finalize (GObject* object)
{
  MateConfClient* client = MATECONF_CLIENT(object);

  mateconf_client_unqueue_notifies (client);
  
  g_hash_table_foreach_remove (client->dir_hash,
                               destroy_dir_foreach_remove, client);
  
  mateconf_client_clear_cache (client);

  if (client->listeners != NULL)
    {
      mateconf_listeners_free (client->listeners);
      client->listeners = NULL;
    }

  g_hash_table_destroy (client->dir_hash);
  client->dir_hash = NULL;
  
  g_hash_table_destroy (client->cache_hash);
  client->cache_hash = NULL;

  g_hash_table_destroy (client->cache_dirs);
  client->cache_dirs = NULL;

  unregister_client (client);

  set_engine (client, NULL);

  if (G_OBJECT_CLASS (parent_class)->finalize)
    (* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

/*
 * Default error handlers
 */

static void
mateconf_client_real_unreturned_error (MateConfClient* client, GError* error)
{
  trace ("Unreturned error '%s'", error->message);
  if (client->error_mode == MATECONF_CLIENT_HANDLE_UNRETURNED)
    {
      if (global_error_handler != NULL)
        {
          (*global_error_handler) (client, error);
        }
      else
        {
          /* We silently ignore this, since it probably isn't
           * really an error per se
           */
          if (error->code == MATECONF_ERROR_OVERRIDDEN ||
              error->code == MATECONF_ERROR_NO_WRITABLE_DATABASE)
            return;
          
          g_printerr (_("MateConf Error: %s\n"),
                      error->message);
        }
    }
}

static void
mateconf_client_real_error            (MateConfClient* client, GError* error)
{
  trace ("Error '%s'", error->message);
  if (client->error_mode == MATECONF_CLIENT_HANDLE_ALL)
    {
      if (global_error_handler != NULL)
        {
          (*global_error_handler) (client, error);
        }
      else
        {
          g_printerr (_("MateConf Error: %s\n"),
                      error->message);
        }
    }
}

/* Emit the proper signals for the error, and fill in err */
static gboolean
handle_error(MateConfClient* client, GError* error, GError** err)
{
  if (error != NULL)
    {
      mateconf_client_error(client, error);
      
      if (err == NULL)
        {
          mateconf_client_unreturned_error(client, error);

          g_error_free(error);
        }
      else
        *err = error;

      return TRUE;
    }
  else
    return FALSE;
}

static void
notify_from_server_callback (MateConfEngine* conf, guint cnxn_id,
                             MateConfEntry *entry,
                             gpointer user_data)
{
  MateConfClient* client = user_data;
  gboolean changed;
  
  g_return_if_fail (client != NULL);
  g_return_if_fail (MATECONF_IS_CLIENT(client));
  g_return_if_fail (client->engine == conf);

  trace ("Received notify of change to '%s' from server",
         entry->key);
  
  /* First do the caching, so that state is sane for the
   * listeners or functions connected to value_changed.
   * We know this key is under a directory in our dir list.
   */
  changed = mateconf_client_cache (client, FALSE, entry, TRUE);

  if (!changed)
    return; /* don't do the notify */

  mateconf_client_queue_notify (client, entry->key);
}

/*
 * Public API
 */


/**
 * mateconf_client_get_default:
 *
 * Creates a new #MateConfClient using the default #MateConfEngine. Normally this is the
 * engine you want. If someone else is already using the default
 * #MateConfClient, this function returns the same one they're using, but
 * with the reference count incremented. So you have to unref either way.
 *
 * It's important to call g_type_init() before using this GObject, to initialize the type system.
 *
 * Return value: (transfer full): a new #MateConfClient. g_object_unref() when you're done.
 */
MateConfClient*
mateconf_client_get_default (void)
{
  MateConfClient *client;
  MateConfEngine *engine;
  
  g_return_val_if_fail(mateconf_is_initialized(), NULL);

  engine = mateconf_engine_get_default ();
  
  client = lookup_client (engine);
  if (client)
    {
      g_assert (client->engine == engine);
      g_object_ref (G_OBJECT (client));
      mateconf_engine_unref (engine);
      return client;
    }
  else
    {
      client = g_object_new (mateconf_client_get_type (), NULL);
      g_object_ref (G_OBJECT (client));      
      set_engine (client, engine);      
      register_client (client);
    }
  
  return client;
}

/**
 * mateconf_client_get_for_engine:
 * @engine: the #MateConfEngine to use.
 *
 * Creates a new #MateConfClient with a specific #MateConfEngine. Only specialized
 * configuration-related programs should need to call this function. The
 * returned #MateConfClient should be unref'd when you're done with g_object_unref().
 * Remember to avoid using the #MateConfEngine directly once you have a #MateConfClient
 * wrapper.
 *
 * Return value: (transfer full): a new #MateConfClient.
 */
MateConfClient*
mateconf_client_get_for_engine (MateConfEngine* engine)
{
  MateConfClient *client;

  g_return_val_if_fail(mateconf_is_initialized(), NULL);

  client = lookup_client (engine);
  if (client)
    {
      g_assert (client->engine == engine);
      g_object_ref (G_OBJECT (client));
      return client;
    }
  else
    {
      client = g_object_new (mateconf_client_get_type (), NULL);

      set_engine (client, engine);

      register_client (client);
    }
  
  return client;
}

typedef struct {
  MateConfClient *client;
  Dir *lower_dir;
  const char *dirname;
} OverlapData;

static void
foreach_setup_overlap(gpointer key, gpointer value, gpointer user_data)
{
  MateConfClient *client;
  Dir *dir = value;
  OverlapData * od = user_data;

  client = od->client;

  /* if we have found the first (well there is only one anyway) directory
   * that includes us that has a notify handler
   */
#ifdef MATECONF_ENABLE_DEBUG
  if (dir->notify_id != 0 &&
      mateconf_key_is_below(dir->name, od->dirname))
    {
      g_assert(od->lower_dir == NULL);
      od->lower_dir = dir;
    }
#else
  if (od->lower_dir == NULL &&
      dir->notify_id != 0 &&
      mateconf_key_is_below(dir->name, od->dirname))
      od->lower_dir = dir;
#endif
  /* if we have found a directory that we include and it has
   * a notify_id, remove the notify handler now
   * FIXME: this is a race, from now on we can miss notifies, it is
   * not an incredible amount of time so this is not a showstopper */
  else if (dir->notify_id != 0 &&
           mateconf_key_is_below (od->dirname, dir->name))
    {
      PUSH_USE_ENGINE (client);
      mateconf_engine_notify_remove (client->engine, dir->notify_id);
      POP_USE_ENGINE (client);
      dir->notify_id = 0;
    }
}

static Dir *
setup_overlaps(MateConfClient* client, const gchar* dirname)
{
  OverlapData od;

  od.client = client;
  od.lower_dir = NULL;
  od.dirname = dirname;

  g_hash_table_foreach(client->dir_hash, foreach_setup_overlap, &od);

  return od.lower_dir;
}

void
mateconf_client_add_dir     (MateConfClient* client,
                          const gchar* dirname,
                          MateConfClientPreloadType preload,
                          GError** err)
{
  Dir* d;
  guint notify_id = 0;
  GError* error = NULL;

  g_return_if_fail (mateconf_valid_key (dirname, NULL));

  trace ("Adding directory '%s'", dirname);
  
  d = g_hash_table_lookup (client->dir_hash, dirname);

  if (d == NULL)
    {
      Dir *overlap_dir;

      overlap_dir = setup_overlaps (client, dirname);

      /* only if there is no directory that includes us
       * already add a notify
       */
      if (overlap_dir == NULL)
        {
          trace ("REMOTE: Adding notify to engine at '%s'",
                 dirname);
          PUSH_USE_ENGINE (client);
          notify_id = mateconf_engine_notify_add (client->engine,
                                               dirname,
                                               notify_from_server_callback,
                                               client,
                                               &error);
          POP_USE_ENGINE (client);
          
          /* We got a notify ID or we got an error, not both */
          g_return_if_fail ( (notify_id != 0 && error == NULL) ||
                             (notify_id == 0 && error != NULL) );
      
      
          if (handle_error (client, error, err))
            return;

          g_assert (error == NULL);
        }
      else
        {
          notify_id = 0;
        }
      
      d = dir_new (dirname, notify_id);

      g_hash_table_insert (client->dir_hash, d->name, d);

      mateconf_client_preload (client, dirname, preload, &error);

      handle_error (client, error, err);
    }

  g_assert (d != NULL);

  d->add_count += 1;
}

typedef struct {
  MateConfClient *client;
  GError *error;
} AddNotifiesData;

static void
foreach_add_notifies(gpointer key, gpointer value, gpointer user_data)
{
  AddNotifiesData *ad = user_data;
  MateConfClient *client;
  Dir *dir = value;

  client = ad->client;

  if (ad->error != NULL)
    return;

  if (dir->notify_id == 0)
    {
      Dir *overlap_dir;
      overlap_dir = setup_overlaps(client, dir->name);

      /* only if there is no directory that includes us
       * already add a notify */
      if (overlap_dir == NULL)
        {
          trace ("REMOTE: Adding notify to engine at '%s'",
                 dir->name);
          PUSH_USE_ENGINE (client);
          dir->notify_id = mateconf_engine_notify_add(client->engine,
                                                   dir->name,
                                                   notify_from_server_callback,
                                                   client,
                                                   &ad->error);
          POP_USE_ENGINE (client);
          
          /* We got a notify ID or we got an error, not both */
          g_return_if_fail( (dir->notify_id != 0 && ad->error == NULL) ||
                            (dir->notify_id == 0 && ad->error != NULL) );

          /* if error is returned, then we'll just ignore
           * things until the end */
        }
    }
}

static gboolean
clear_dir_cache_foreach (char* key, MateConfEntry* entry, char *dir)
{
  if (mateconf_key_is_below (dir, key))
    {
      mateconf_entry_free (entry);
      return TRUE;
    }
  else
    return FALSE;
}

static gboolean
clear_cache_dirs_foreach (char *key, gpointer value, char *dir)
{
  if (strcmp (dir, key) == 0 ||
      mateconf_key_is_below (dir, key))
    {
      trace ("'%s' no longer fully cached", dir);
      return TRUE;
    }

  return FALSE;
}

static void
mateconf_client_real_remove_dir    (MateConfClient* client,
                                 Dir* d,
                                 GError** err)
{
  AddNotifiesData ad;

  g_return_if_fail(d != NULL);
  g_return_if_fail(d->add_count == 0);
  
  g_hash_table_remove(client->dir_hash, d->name);
  
  /* remove notify for this dir */
  
  if (d->notify_id != 0)
    {
      trace ("REMOTE: Removing notify from engine at '%s'", d->name);
      PUSH_USE_ENGINE (client);
      mateconf_engine_notify_remove (client->engine, d->notify_id);
      POP_USE_ENGINE (client);
      d->notify_id = 0;
    }
  
  g_hash_table_foreach_remove (client->cache_hash,
                               (GHRFunc)clear_dir_cache_foreach,
                               d->name);
  g_hash_table_foreach_remove (client->cache_dirs,
                               (GHRFunc)clear_cache_dirs_foreach,
                               d->name);
  dir_destroy(d);

  ad.client = client;
  ad.error = NULL;

  g_hash_table_foreach(client->dir_hash, foreach_add_notifies, &ad);

  handle_error(client, ad.error, err);
}

void
mateconf_client_remove_dir  (MateConfClient* client,
                          const gchar* dirname,
                          GError** err)
{
  Dir* found = NULL;

  trace ("Removing directory '%s'", dirname);
  
  found = g_hash_table_lookup (client->dir_hash,
                               dirname);
  
  if (found != NULL)
    {
      g_return_if_fail(found->add_count > 0);

      found->add_count -= 1;

      if (found->add_count == 0) 
        mateconf_client_real_remove_dir (client, found, err);
    }
#ifndef G_DISABLE_CHECKS
  else
    g_warning("Directory `%s' was not being monitored by MateConfClient %p",
              dirname, client);
#endif
}

/**
 * mateconf_client_notify_add:
 * @client:
 * @namespace_section
 * @func: (scope notified) (closure user_data) (destroy destroy_notify):
 * @user_data:
 * @destroy_notify:
 * @err:
 */
guint
mateconf_client_notify_add (MateConfClient* client,
                         const gchar* namespace_section,
                         MateConfClientNotifyFunc func,
                         gpointer user_data,
                         GFreeFunc destroy_notify,
                         GError** err)
{
  guint cnxn_id = 0;
  
  g_return_val_if_fail(client != NULL, 0);
  g_return_val_if_fail(MATECONF_IS_CLIENT(client), 0);

  if (client->listeners == NULL)
    client->listeners = mateconf_listeners_new();
  
  cnxn_id = mateconf_listeners_add (client->listeners,
                                 namespace_section,
                                 listener_new (func, destroy_notify, user_data),
                                 (GFreeFunc)listener_destroy);

  return cnxn_id;
}

void
mateconf_client_notify_remove  (MateConfClient* client,
                             guint cnxn)
{
  g_return_if_fail(client != NULL);
  g_return_if_fail(MATECONF_IS_CLIENT(client));
  g_return_if_fail(client->listeners != NULL);
  
  mateconf_listeners_remove(client->listeners, cnxn);
  
  if (mateconf_listeners_count(client->listeners) == 0)
    {
      mateconf_listeners_free(client->listeners);
      client->listeners = NULL;
    }
}

void
mateconf_client_notify (MateConfClient* client, const char* key)
{
  MateConfEntry *entry;

  g_return_if_fail (client != NULL);
  g_return_if_fail (MATECONF_IS_CLIENT(client));
  g_return_if_fail (key != NULL);

  entry = mateconf_client_get_entry (client, key, NULL, TRUE, NULL);
  if (entry != NULL)
    {
      notify_one_entry (client, entry);
      mateconf_entry_unref (entry);
    }
}

void
mateconf_client_set_error_handling(MateConfClient* client,
                                MateConfClientErrorHandlingMode mode)
{
  g_return_if_fail(client != NULL);
  g_return_if_fail(MATECONF_IS_CLIENT(client));

  client->error_mode = mode;
}

static gboolean
clear_cache_foreach (char* key, MateConfEntry* entry, MateConfClient* client)
{
  mateconf_entry_free (entry);

  return TRUE;
}

void
mateconf_client_clear_cache(MateConfClient* client)
{
  g_return_if_fail(client != NULL);
  g_return_if_fail(MATECONF_IS_CLIENT(client));

  trace ("Clearing cache");
  
  g_hash_table_foreach_remove (client->cache_hash, (GHRFunc)clear_cache_foreach,
                               client);

  g_hash_table_remove_all (client->cache_dirs);
}

static void
cache_pairs_in_dir(MateConfClient* client, const gchar* path);

static void 
recurse_subdir_list(MateConfClient* client, GSList* subdirs)
{
  GSList* tmp;

  tmp = subdirs;
  
  while (tmp != NULL)
    {
      gchar* s = tmp->data;
      
      cache_pairs_in_dir(client, s);

      trace ("REMOTE: All dirs at '%s'", s);
      PUSH_USE_ENGINE (client);
      recurse_subdir_list(client,
                          mateconf_engine_all_dirs (client->engine, s, NULL));
      POP_USE_ENGINE (client);

      g_free(s);
      
      tmp = g_slist_next(tmp);
    }
  
  g_slist_free(subdirs);
}

static gboolean
key_being_monitored (MateConfClient *client,
                     const char  *key)
{
  gboolean retval = FALSE;
  char* parent = g_strdup (key);
  char* end;

  end = parent + strlen (parent);
  
  while (end)
    {
      if (end == parent)
        *(end + 1) = '\0'; /* special-case "/" root dir */
      else
        *end = '\0'; /* chop '/' off of dir */
      
      if (g_hash_table_lookup (client->dir_hash, parent) != NULL)
        {
          retval = TRUE;
          break;
        }

      if (end != parent)
        end = strrchr (parent, '/');
      else
        end = NULL;
    }

  g_free (parent);

  return retval;
}

static void
cache_entry_list_destructively (MateConfClient *client,
                                GSList      *entries)
{
  GSList *tmp;
  
  tmp = entries;
  
  while (tmp != NULL)
    {
      MateConfEntry* entry = tmp->data;
      
      mateconf_client_cache (client, TRUE, entry, FALSE);
      
      tmp = g_slist_next (tmp);
    }
  
  g_slist_free (entries);
}

static void 
cache_pairs_in_dir(MateConfClient* client, const gchar* dir)
{
  GSList* pairs;
  GError* error = NULL;

  trace ("REMOTE: Caching values in '%s'", dir);
  
  PUSH_USE_ENGINE (client);
  pairs = mateconf_engine_all_entries(client->engine, dir, &error);
  POP_USE_ENGINE (client);
  
  if (error != NULL)
    {
      g_printerr (_("MateConf warning: failure listing pairs in `%s': %s"),
                  dir, error->message);
      g_error_free(error);
      error = NULL;
    }

  cache_entry_list_destructively (client, pairs);
  trace ("Mark '%s' as fully cached", dir);
  g_hash_table_insert (client->cache_dirs, g_strdup (dir), GINT_TO_POINTER (1));
}

void
mateconf_client_preload    (MateConfClient* client,
                         const gchar* dirname,
                         MateConfClientPreloadType type,
                         GError** err)
{

  g_return_if_fail(client != NULL);
  g_return_if_fail(MATECONF_IS_CLIENT(client));
  g_return_if_fail(dirname != NULL);

#ifdef MATECONF_ENABLE_DEBUG
  if (!key_being_monitored (client, dirname))
    {
      g_warning("Can only preload directories you've added with mateconf_client_add_dir() (tried to preload '%s')",
                dirname);
      return;
    }
#endif
  
  switch (type)
    {
    case MATECONF_CLIENT_PRELOAD_NONE:
      /* nothing */
      break;

    case MATECONF_CLIENT_PRELOAD_ONELEVEL:
      {
        trace ("Onelevel preload of '%s'", dirname);
        
        cache_pairs_in_dir (client, dirname);
      }
      break;

    case MATECONF_CLIENT_PRELOAD_RECURSIVE:
      {
        GSList* subdirs;

        trace ("Recursive preload of '%s'", dirname);
        
	trace ("REMOTE: All dirs at '%s'", dirname);
        PUSH_USE_ENGINE (client);
        subdirs = mateconf_engine_all_dirs(client->engine, dirname, NULL);
        POP_USE_ENGINE (client);
        
        cache_pairs_in_dir(client, dirname);
          
        recurse_subdir_list(client, subdirs);
      }
      break;

    default:
      g_assert_not_reached();
      break;
    }
}

/*
 * Basic key-manipulation facilities
 */

void
mateconf_client_set             (MateConfClient* client,
                              const gchar* key,
                              const MateConfValue* val,
                              GError** err)
{
  GError* error = NULL;

  trace ("REMOTE: Setting value of '%s'", key);
  PUSH_USE_ENGINE (client);
  mateconf_engine_set (client->engine, key, val, &error);
  POP_USE_ENGINE (client);
  
  handle_error(client, error, err);
}

gboolean
mateconf_client_unset          (MateConfClient* client,
                             const gchar* key, GError** err)
{
  GError* error = NULL;

  trace ("REMOTE: Unsetting '%s'", key);
  PUSH_USE_ENGINE (client);
  mateconf_engine_unset(client->engine, key, &error);
  POP_USE_ENGINE (client);
  
  handle_error(client, error, err);

  if (error != NULL)
    return FALSE;
  else
    return TRUE;
}

gboolean
mateconf_client_recursive_unset (MateConfClient *client,
                              const char     *key,
                              MateConfUnsetFlags flags,
                              GError        **err)
{
  GError* error = NULL;

  trace ("REMOTE: Recursive unsetting '%s'", key);
  
  PUSH_USE_ENGINE (client);
  mateconf_engine_recursive_unset(client->engine, key, flags, &error);
  POP_USE_ENGINE (client);
  
  handle_error(client, error, err);

  if (error != NULL)
    return FALSE;
  else
    return TRUE;
}

static GSList*
copy_entry_list (GSList *list)
{
  GSList *copy;
  GSList *tmp;
  
  copy = NULL;
  tmp = list;
  while (tmp != NULL)
    {
      copy = g_slist_prepend (copy,
                              mateconf_entry_copy (tmp->data));
      tmp = tmp->next;
    }

  copy = g_slist_reverse (copy);

  return copy;
}

/**
 * mateconf_client_all_entries:
 * @client: a #MateConfClient.
 * @dir: directory to list.
 * @err: the return location for an allocated #GError, or <symbol>NULL</symbol> to ignore errors.
 *
 * Lists the key-value pairs in @dir. Does not list subdirectories; for
 * that use mateconf_client_all_dirs(). The returned list contains #MateConfEntry
 * objects. A #MateConfEntry contains an <emphasis>absolute</emphasis> key
 * and a value. The list is not recursive, it contains only the immediate
 * children of @dir.  To free the returned list, mateconf_entry_free()
 * each list element, then g_slist_free() the list itself.
 * Just like mateconf_engine_all_entries (), but uses #MateConfClient caching and error-handling features.
 *
 * Return value: (element-type MateConfEntry) (transfer full): List of #MateConfEntry.
 */
GSList*
mateconf_client_all_entries    (MateConfClient* client,
                             const gchar* dir,
                             GError** err)
{
  GError *error = NULL;
  GSList *retval;
  int dirlen;

  if (g_hash_table_lookup (client->cache_dirs, dir))
    {
      GHashTableIter iter;
      gpointer key, value;

      trace ("CACHED: Getting all values in '%s'", dir);

      dirlen = strlen (dir);
      retval = NULL;
      g_hash_table_iter_init (&iter, client->cache_hash);
      while (g_hash_table_iter_next (&iter, &key, &value))
        {
          const gchar *id = key;
          MateConfEntry *entry = value;
          if (g_str_has_prefix (id, dir) &&
              id + dirlen == strrchr (id, '/'))
            retval = g_slist_prepend (retval, mateconf_entry_copy (entry));
        }

      return retval;
    }

  trace ("REMOTE: Getting all values in '%s'", dir);

  PUSH_USE_ENGINE (client);
  retval = mateconf_engine_all_entries (client->engine, dir, &error);
  POP_USE_ENGINE (client);
  
  handle_error (client, error, err);

  if (error != NULL)
    return NULL;

  if (key_being_monitored (client, dir))
    {
      cache_entry_list_destructively (client, copy_entry_list (retval));
      trace ("Mark '%s' as fully cached", dir);
      g_hash_table_insert (client->cache_dirs, g_strdup (dir), GINT_TO_POINTER (1));
    }

  return retval;
}

/**
 * mateconf_client_all_dirs:
 * @client: a #MateConfClient.
 * @dir: directory to get subdirectories from.
 * @err: the return location for an allocated #GError, or <symbol>NULL</symbol> to ignore errors.
 *
 * Lists the subdirectories in @dir. The returned list contains
 * allocated strings. Each string is the absolute path of a
 * subdirectory. You should g_free() each string in the list, then
 * g_slist_free() the list itself.  Just like mateconf_engine_all_dirs(),
 * but uses #MateConfClient caching and error-handling features.
 *
 * Return value: (element-type utf8) (transfer full): List of allocated subdirectory names.
 */
GSList*
mateconf_client_all_dirs       (MateConfClient* client,
                             const gchar* dir, GError** err)
{
  GError* error = NULL;
  GSList* retval;

  trace ("REMOTE: Getting all dirs in '%s'", dir);
  
  PUSH_USE_ENGINE (client);
  retval = mateconf_engine_all_dirs(client->engine, dir, &error);
  POP_USE_ENGINE (client);
  
  handle_error(client, error, err);

  return retval;
}

void
mateconf_client_suggest_sync   (MateConfClient* client,
                             GError** err)
{
  GError* error = NULL;

  trace ("REMOTE: Suggesting sync");
  
  PUSH_USE_ENGINE (client);
  mateconf_engine_suggest_sync(client->engine, &error);
  POP_USE_ENGINE (client);
  
  handle_error(client, error, err);
}

gboolean
mateconf_client_dir_exists(MateConfClient* client,
                        const gchar* dir, GError** err)
{
  GError* error = NULL;
  gboolean retval;

  trace ("REMOTE: Checking whether directory '%s' exists...", dir);
  
  PUSH_USE_ENGINE (client);
  retval = mateconf_engine_dir_exists (client->engine, dir, &error);
  POP_USE_ENGINE (client);
  
  handle_error (client, error, err);

  if (retval)
    trace ("'%s' exists", dir);
  else
    trace ("'%s' doesn't exist", dir);
  
  return retval;
}

gboolean
mateconf_client_key_is_writable (MateConfClient* client,
                              const gchar* key,
                              GError**     err)
{
  GError* error = NULL;
  MateConfEntry *entry = NULL;
  gboolean is_writable;

  g_return_val_if_fail (key != NULL, FALSE);
  g_return_val_if_fail (err == NULL || *err == NULL, FALSE);

  if (mateconf_client_lookup (client, key, &entry))
    {
      if (!entry)
        return FALSE;

      trace ("CACHED: Checking whether key '%s' is writable", key);
      return mateconf_entry_get_is_writable (entry);
    }
  
  trace ("REMOTE: Checking whether key '%s' is writable", key);

  entry = get (client, key, TRUE, &error);

  if (entry == NULL && error != NULL)
    handle_error (client, error, err);
  else
    g_assert (error == NULL);

  if (entry == NULL)
    is_writable = FALSE;
  else
    is_writable = mateconf_entry_get_is_writable (entry);  

  if (entry)
    mateconf_entry_free (entry);
  
  return is_writable;
}

static gboolean
check_type(const gchar* key, MateConfValue* val, MateConfValueType t, GError** err)
{
  if (val->type != t)
    {
      mateconf_set_error(err, MATECONF_ERROR_TYPE_MISMATCH,
                      _("Expected `%s' got `%s' for key %s"),
                      mateconf_value_type_to_string(t),
                      mateconf_value_type_to_string(val->type),
                      key);
      return FALSE;
    }
  else
    return TRUE;
}

static MateConfEntry*
get (MateConfClient *client,
     const gchar *key,
     gboolean     use_default,
     GError     **error)
{
  MateConfEntry *entry = NULL;
  
  g_return_val_if_fail (client != NULL, NULL);
  g_return_val_if_fail (MATECONF_IS_CLIENT(client), NULL);
  g_return_val_if_fail (error != NULL, NULL);
  g_return_val_if_fail (*error == NULL, NULL);
  
  /* Check our client-side cache */
  if (mateconf_client_lookup (client, key, &entry))

    {
      trace ("CACHED: Query for '%s'", key);
      
      if (entry == NULL)
        return NULL;
      
      if (mateconf_entry_get_is_default (entry) && !use_default)
        return NULL;
      else
        return mateconf_entry_copy (entry);
    }
      
  g_assert (entry == NULL); /* if it was in the cache we should have returned */

  /* Check the MateConfEngine */
  trace ("REMOTE: Query for '%s'", key);
  PUSH_USE_ENGINE (client);
  entry = mateconf_engine_get_entry (client->engine, key,
                                  mateconf_current_locale(),
                                  TRUE /* always use default here */,
                                  error);
  POP_USE_ENGINE (client);
  
  if (*error != NULL)
    {
      g_return_val_if_fail (entry == NULL, NULL);
      return NULL;
    }
  else
    {
      g_assert (entry != NULL); /* mateconf_engine_get_entry shouldn't return NULL ever */
      
      /* Cache this value, if it's in our directory list. */
      if (key_being_monitored (client, key))
        {
          /* cache a copy of val */
          mateconf_client_cache (client, FALSE, entry, FALSE);
        }

      /* We don't own the entry, we're returning this copy belonging
       * to the caller
       */
      if (mateconf_entry_get_is_default (entry) && !use_default)
        {
          mateconf_entry_free (entry);
          return NULL;
        }
      else
        return entry;
    }
}
     
static MateConfValue*
mateconf_client_get_full        (MateConfClient* client,
                              const gchar* key, const gchar* locale,
                              gboolean use_schema_default,
                              GError** err)
{
  GError* error = NULL;
  MateConfEntry *entry;
  MateConfValue *retval;
  
  g_return_val_if_fail (err == NULL || *err == NULL, NULL);

  if (locale != NULL)
    g_warning ("haven't implemented getting a specific locale in MateConfClient");
  
  entry = get (client, key, use_schema_default,
               &error);

  if (entry == NULL && error != NULL)
    handle_error(client, error, err);
  else
    g_assert (error == NULL);

  retval = NULL;
  
  if (entry && mateconf_entry_get_value (entry))
    retval = mateconf_value_copy (mateconf_entry_get_value (entry));

  if (entry != NULL)
    mateconf_entry_free (entry);

  return retval;
}

MateConfEntry*
mateconf_client_get_entry (MateConfClient* client,
                        const gchar* key,
                        const gchar* locale,
                        gboolean use_schema_default,
                        GError** err)
{
  GError* error = NULL;
  MateConfEntry *entry;
  
  g_return_val_if_fail(err == NULL || *err == NULL, NULL);

  if (locale != NULL)
    g_warning("haven't implemented getting a specific locale in MateConfClient");
  
  entry = get (client, key, use_schema_default,
               &error);

  if (entry == NULL && error != NULL)
    handle_error (client, error, err);
  else
    g_assert (error == NULL);
  
  return entry;
}

MateConfValue*
mateconf_client_get             (MateConfClient* client,
                              const gchar* key,
                              GError** err)
{
  g_return_val_if_fail (MATECONF_IS_CLIENT (client), NULL);
  g_return_val_if_fail (key != NULL, NULL);
  return mateconf_client_get_full (client, key, NULL, TRUE, err);
}

MateConfValue*
mateconf_client_get_without_default  (MateConfClient* client,
                                   const gchar* key,
                                   GError** err)
{
  g_return_val_if_fail (MATECONF_IS_CLIENT (client), NULL);
  g_return_val_if_fail (key != NULL, NULL);
  return mateconf_client_get_full (client, key, NULL, FALSE, err);
}

MateConfValue*
mateconf_client_get_default_from_schema (MateConfClient* client,
                                      const gchar* key,
                                      GError** err)
{
  GError* error = NULL;
  MateConfEntry *entry = NULL;
  MateConfValue *val = NULL;
  
  g_return_val_if_fail (err == NULL || *err == NULL, NULL);  
  g_return_val_if_fail (client != NULL, NULL);
  g_return_val_if_fail (MATECONF_IS_CLIENT(client), NULL);
  g_return_val_if_fail (key != NULL, NULL);
  
  /* Check our client-side cache to see if the default is the same as
   * the regular value (FIXME put a default_value field in the
   * cache and store both, lose the is_default flag)
   */
  if (mateconf_client_lookup (client, key, &entry))
    {
      if (!entry)
        return NULL;

      if (mateconf_entry_get_is_default (entry))
        {
	  trace ("CACHED: Getting schema default for '%s'", key);

          return mateconf_entry_get_value (entry) ?
            mateconf_value_copy (mateconf_entry_get_value (entry)) :
            NULL;
        }
    }

  /* Check the MateConfEngine */
  trace ("REMOTE: Getting schema default for '%s'", key);
  PUSH_USE_ENGINE (client);
  val = mateconf_engine_get_default_from_schema (client->engine, key,
                                              &error);
  POP_USE_ENGINE (client);
  
  if (error != NULL)
    {
      g_assert (val == NULL);
      handle_error (client, error, err);
      return NULL;
    }
  else
    {
      /* FIXME eventually we'll cache the value
       * by adding a field to the cache
       */
      return val;
    }
}

gdouble
mateconf_client_get_float (MateConfClient* client, const gchar* key,
                        GError** err)
{
  static const gdouble def = 0.0;
  GError* error = NULL;
  MateConfValue *val;

  g_return_val_if_fail (err == NULL || *err == NULL, 0.0);

  val = mateconf_client_get (client, key, &error);

  if (val != NULL)
    {
      gdouble retval = def;

      g_assert(error == NULL);
      
      if (check_type (key, val, MATECONF_VALUE_FLOAT, &error))
        retval = mateconf_value_get_float (val);
      else
        handle_error (client, error, err);

      mateconf_value_free (val);

      return retval;
    }
  else
    {
      if (error != NULL)
        handle_error (client, error, err);
      return def;
    }
}

gint
mateconf_client_get_int   (MateConfClient* client, const gchar* key,
                        GError** err)
{
  static const gint def = 0;
  GError* error = NULL;
  MateConfValue* val;

  g_return_val_if_fail (err == NULL || *err == NULL, 0);

  val = mateconf_client_get (client, key, &error);

  if (val != NULL)
    {
      gint retval = def;

      g_assert(error == NULL);
      
      if (check_type (key, val, MATECONF_VALUE_INT, &error))
        retval = mateconf_value_get_int(val);
      else
        handle_error (client, error, err);

      mateconf_value_free (val);

      return retval;
    }
  else
    {
      if (error != NULL)
        handle_error (client, error, err);
      return def;
    }
}

gchar*
mateconf_client_get_string(MateConfClient* client, const gchar* key,
                        GError** err)
{
  GError* error = NULL;
  MateConfValue* val;

  g_return_val_if_fail (err == NULL || *err == NULL, NULL);

  val = mateconf_client_get (client, key, &error);

  if (val != NULL)
    {
      gchar* retval = NULL;

      g_assert(error == NULL);
      
      if (check_type (key, val, MATECONF_VALUE_STRING, &error))
        retval = mateconf_value_steal_string (val);
      else
        handle_error (client, error, err);
      
      mateconf_value_free (val);

      return retval;
    }
  else
    {
      if (error != NULL)
        handle_error (client, error, err);
      return NULL;
    }
}


gboolean
mateconf_client_get_bool  (MateConfClient* client, const gchar* key,
                        GError** err)
{
  static const gboolean def = FALSE;
  GError* error = NULL;
  MateConfValue* val;

  g_return_val_if_fail (err == NULL || *err == NULL, FALSE);

  val = mateconf_client_get (client, key, &error);

  if (val != NULL)
    {
      gboolean retval = def;

      g_assert (error == NULL);
      
      if (check_type (key, val, MATECONF_VALUE_BOOL, &error))
        retval = mateconf_value_get_bool (val);
      else
        handle_error (client, error, err);

      mateconf_value_free (val);

      return retval;
    }
  else
    {
      if (error != NULL)
        handle_error (client, error, err);
      return def;
    }
}

MateConfSchema*
mateconf_client_get_schema  (MateConfClient* client,
                          const gchar* key, GError** err)
{
  GError* error = NULL;
  MateConfValue* val;

  g_return_val_if_fail (err == NULL || *err == NULL, NULL);

  val = mateconf_client_get (client, key, &error);

  if (val != NULL)
    {
      MateConfSchema* retval = NULL;

      g_assert(error == NULL);
      
      if (check_type (key, val, MATECONF_VALUE_SCHEMA, &error))
        retval = mateconf_value_steal_schema (val);
      else
        handle_error (client, error, err);
      
      mateconf_value_free (val);

      return retval;
    }
  else
    {
      if (error != NULL)
        handle_error (client, error, err);
      return NULL;
    }
}

/**
 * mateconf_client_get_list: (skip)
 * @client: a #MateConfClient.
 * @key: key you want the value of.
 * @list_type: type of each list element.
 * @err: the return location for an allocated #GError, or <symbol>NULL</symbol> to ignore errors.
 *
 * Requests the list (%MATECONF_VALUE_LIST) stored at @key.  Automatically
 * performs type-checking, so if a non-list is stored at @key, or the
 * list does not contain elements of type @list_type, an error is
 * returned. If no value is set or an error occurs, <symbol>NULL</symbol>
 * is returned. Note that <symbol>NULL</symbol> is also the empty list,
 * so if you need to distinguish the empty list from an unset value, you
 * must use mateconf_client_get () to obtain a raw #MateConfValue.
 *
 * <emphasis>Remember that MateConf lists can only store primitive types:
 * %MATECONF_VALUE_FLOAT, %MATECONF_VALUE_INT, %MATECONF_VALUE_BOOL,
 * %MATECONF_VALUE_STRING, %MATECONF_VALUE_SCHEMA.</emphasis> Also remember
 * that lists must be uniform, you may not mix types in the same list.
 *
 * The type of the list elements depends on @list_type. A #MateConfValue
 * with type %MATECONF_VALUE_LIST normally stores a list of more #MateConfValue
 * objects. mateconf_client_get_list() automatically converts to primitive C
 * types. Thus, the list-&gt;data fields in the returned list
 * contain:
 *  
 * <informaltable pgwide="1" frame="none">
 * <tgroup cols="2"><colspec colwidth="2*"/><colspec colwidth="8*"/>
 * <tbody>
 *  
 * <row>
 * <entry>%MATECONF_VALUE_INT</entry>
 * <entry>The integer itself, converted with GINT_TO_POINTER()</entry>
 * </row>
 *  
 * <row>
 * <entry>%MATECONF_VALUE_BOOL</entry>
 * <entry>The bool itself, converted with GINT_TO_POINTER()</entry>
 * </row>
 *  
 * <row>
 * <entry>%MATECONF_VALUE_FLOAT</entry>
 * <entry>A pointer to #gdouble, which should be freed with g_free()</entry>
 * </row>
 *  
 * <row>
 * <entry>%MATECONF_VALUE_STRING</entry>
 * <entry>A pointer to #gchar, which should be freed with g_free()</entry>
 * </row>
 *  
 * <row>
 * <entry>%MATECONF_VALUE_SCHEMA</entry>
 * <entry>A pointer to #MateConfSchema, which should be freed with mateconf_schema_free()</entry>
 * </row>
 *  
 * </tbody></tgroup></informaltable>
 *  
 * In the %MATECONF_VALUE_FLOAT and %MATECONF_VALUE_STRING cases, you must
 * g_free() each list element. In the %MATECONF_VALUE_SCHEMA case you must
 * mateconf_schema_free() each element. In all cases you must free the
 * list itself with g_slist_free().
 *
 * Just like mateconf_engine_get_list (), but uses #MateConfClient caching and error-handling features.
 *
* Return value: an allocated list, with elements as described above.
*/
GSList*
mateconf_client_get_list    (MateConfClient* client, const gchar* key,
                          MateConfValueType list_type, GError** err)
{
  GError* error = NULL;
  MateConfValue* val;

  g_return_val_if_fail (err == NULL || *err == NULL, NULL);

  val = mateconf_client_get (client, key, &error);

  if (val != NULL)
    {
      GSList* retval;

      g_assert (error == NULL);

      /* This function checks the type and destroys "val" */
      retval = mateconf_value_list_to_primitive_list_destructive (val, list_type, &error);

      if (error != NULL)
        {
          g_assert (retval == NULL);
          handle_error (client, error, err);
          return NULL;
        }
      else
        return retval;
    }
  else
    {
      if (error != NULL)
        handle_error (client, error, err);
      return NULL;
    }
}

gboolean
mateconf_client_get_pair    (MateConfClient* client, const gchar* key,
                          MateConfValueType car_type, MateConfValueType cdr_type,
                          gpointer car_retloc, gpointer cdr_retloc,
                          GError** err)
{
  GError* error = NULL;
  MateConfValue* val;

  g_return_val_if_fail (err == NULL || *err == NULL, FALSE);

  val = mateconf_client_get (client, key, &error);

  if (val != NULL)
    {
      g_assert(error == NULL);

      /* This function checks the type and destroys "val" */
      if (mateconf_value_pair_to_primitive_pair_destructive (val, car_type, cdr_type,
                                                          car_retloc, cdr_retloc,
                                                          &error))
        {
          g_assert (error == NULL);
          return TRUE;
        }
      else
        {
          g_assert (error != NULL);
          handle_error (client, error, err);
          return FALSE;
        }
    }
  else
    {
      if (error != NULL)
        {
          handle_error (client, error, err);
          return FALSE;
        }
      else
        return TRUE;
    }
}


/*
 * For the set functions, we just set normally, and wait for the
 * notification to come back from the server before we update
 * our cache. This may be the wrong thing; maybe we should
 * update immediately?
 * Problem with delayed update: user calls set() then get(),
 *  results in weirdness
 * Problem with with regular update: get() before the notify
 *  is out of sync with the listening parts of the application
 * 
 */

gboolean
mateconf_client_set_float   (MateConfClient* client, const gchar* key,
                          gdouble val, GError** err)
{
  GError* error = NULL;
  gboolean result;
  
  g_return_val_if_fail(client != NULL, FALSE);
  g_return_val_if_fail(MATECONF_IS_CLIENT(client), FALSE);  
  g_return_val_if_fail(key != NULL, FALSE);

  trace ("REMOTE: Setting float '%s'", key);
  PUSH_USE_ENGINE (client);
  result = mateconf_engine_set_float (client->engine, key, val, &error);
  POP_USE_ENGINE (client);

  if (result)
    return TRUE;
  else
    {
      handle_error(client, error, err);
      return FALSE;
    }
}

gboolean
mateconf_client_set_int     (MateConfClient* client, const gchar* key,
                          gint val, GError** err)
{
  GError* error = NULL;
  gboolean result;
  
  g_return_val_if_fail(client != NULL, FALSE);
  g_return_val_if_fail(MATECONF_IS_CLIENT(client), FALSE);  
  g_return_val_if_fail(key != NULL, FALSE);

  trace ("REMOTE: Setting int '%s'", key);
  PUSH_USE_ENGINE (client);
  result = mateconf_engine_set_int (client->engine, key, val, &error);
  POP_USE_ENGINE (client);
  
  if (result)
    return TRUE;
  else
    {
      handle_error(client, error, err);
      return FALSE;
    }
}

gboolean
mateconf_client_set_string  (MateConfClient* client, const gchar* key,
                          const gchar* val, GError** err)
{
  GError* error = NULL;
  gboolean result;
  
  g_return_val_if_fail(client != NULL, FALSE);
  g_return_val_if_fail(MATECONF_IS_CLIENT(client), FALSE);  
  g_return_val_if_fail(key != NULL, FALSE);
  g_return_val_if_fail(val != NULL, FALSE);

  trace ("REMOTE: Setting string '%s'", key);
  PUSH_USE_ENGINE (client);
  result = mateconf_engine_set_string(client->engine, key, val, &error);
  POP_USE_ENGINE (client);
  
  if (result)
    return TRUE;
  else
    {
      handle_error(client, error, err);
      return FALSE;
    }
}

gboolean
mateconf_client_set_bool    (MateConfClient* client, const gchar* key,
                          gboolean val, GError** err)
{
  GError* error = NULL;
  gboolean result;
  
  g_return_val_if_fail(client != NULL, FALSE);
  g_return_val_if_fail(MATECONF_IS_CLIENT(client), FALSE);  
  g_return_val_if_fail(key != NULL, FALSE);

  trace ("REMOTE: Setting bool '%s'", key);
  PUSH_USE_ENGINE (client);
  result = mateconf_engine_set_bool (client->engine, key, val, &error);
  POP_USE_ENGINE (client);

  if (result)
    return TRUE;
  else
    {
      handle_error(client, error, err);
      return FALSE;
    }
}

gboolean
mateconf_client_set_schema  (MateConfClient* client, const gchar* key,
                          const MateConfSchema* val, GError** err)
{
  GError* error = NULL;
  gboolean result;
  
  g_return_val_if_fail(client != NULL, FALSE);
  g_return_val_if_fail(MATECONF_IS_CLIENT(client), FALSE);  
  g_return_val_if_fail(key != NULL, FALSE);
  g_return_val_if_fail(val != NULL, FALSE);

  trace ("REMOTE: Setting schema '%s'", key);
  PUSH_USE_ENGINE (client);
  result = mateconf_engine_set_schema(client->engine, key, val, &error);
  POP_USE_ENGINE (client);
  
  if (result)
    return TRUE;
  else
    {
      handle_error(client, error, err);
      return FALSE;
    }
}

gboolean
mateconf_client_set_list    (MateConfClient* client, const gchar* key,
                          MateConfValueType list_type,
                          GSList* list,
                          GError** err)
{
  GError* error = NULL;
  gboolean result;
  
  g_return_val_if_fail(client != NULL, FALSE);
  g_return_val_if_fail(MATECONF_IS_CLIENT(client), FALSE);  
  g_return_val_if_fail(key != NULL, FALSE);

  trace ("REMOTE: Setting list '%s'", key);
  PUSH_USE_ENGINE (client);
  result = mateconf_engine_set_list(client->engine, key, list_type, list, &error);
  POP_USE_ENGINE (client);

  if (result)
    return TRUE;
  else
    {
      handle_error(client, error, err);
      return FALSE;
    }
}

gboolean
mateconf_client_set_pair    (MateConfClient* client, const gchar* key,
                          MateConfValueType car_type, MateConfValueType cdr_type,
                          gconstpointer address_of_car,
                          gconstpointer address_of_cdr,
                          GError** err)
{
  GError* error = NULL;
  gboolean result;
  
  g_return_val_if_fail(client != NULL, FALSE);
  g_return_val_if_fail(MATECONF_IS_CLIENT(client), FALSE);  
  g_return_val_if_fail(key != NULL, FALSE);

  trace ("REMOTE: Setting pair '%s'", key);
  PUSH_USE_ENGINE (client);
  result = mateconf_engine_set_pair (client->engine, key, car_type, cdr_type,
                                  address_of_car, address_of_cdr, &error);
  POP_USE_ENGINE (client);

  if (result)
    return TRUE;
  else
    {
      handle_error(client, error, err);
      return FALSE;
    }
}


/*
 * Functions to emit signals
 */

void
mateconf_client_error                  (MateConfClient* client, GError* error)
{
  g_return_if_fail(client != NULL);
  g_return_if_fail(MATECONF_IS_CLIENT(client));
  
  g_signal_emit (G_OBJECT(client), client_signals[ERROR], 0,
                 error);
}

void
mateconf_client_unreturned_error       (MateConfClient* client, GError* error)
{
  g_return_if_fail(client != NULL);
  g_return_if_fail(MATECONF_IS_CLIENT(client));

  g_signal_emit (G_OBJECT(client), client_signals[UNRETURNED_ERROR], 0,
                 error);
}

void
mateconf_client_value_changed          (MateConfClient* client,
                                     const gchar* key,
                                     MateConfValue* value)
{
  g_return_if_fail(client != NULL);
  g_return_if_fail(MATECONF_IS_CLIENT(client));
  g_return_if_fail(key != NULL);
  
  g_signal_emit(G_OBJECT(client), client_signals[VALUE_CHANGED], 0,
                key, value);
}

/*
 * Internal utility
 */

static gboolean
mateconf_client_cache (MateConfClient *client,
                    gboolean     take_ownership,
                    MateConfEntry  *new_entry,
                    gboolean     preserve_schema_name)
{
  gpointer oldkey = NULL, oldval = NULL;

  if (g_hash_table_lookup_extended (client->cache_hash, new_entry->key, &oldkey, &oldval))
    {
      /* Already have a value, update it */
      MateConfEntry *entry = oldval;
      gboolean changed;
      
      g_assert (entry != NULL);

      changed = ! mateconf_entry_equal (entry, new_entry);

      if (changed)
        {
          trace ("Updating value of '%s' in the cache",
                 new_entry->key);

          if (!take_ownership)
            new_entry = mateconf_entry_copy (new_entry);
          
          if (preserve_schema_name)
            mateconf_entry_set_schema_name (new_entry, 
                                         mateconf_entry_get_schema_name (entry));

          g_hash_table_replace (client->cache_hash,
                                new_entry->key,
                                new_entry);

          /* oldkey is inside entry */
          mateconf_entry_free (entry);
        }
      else
        {
          trace ("Value of '%s' hasn't actually changed, would have updated in cache if it had",
                 new_entry->key);

          if (take_ownership)
            mateconf_entry_free (new_entry);
        }

      return changed;
    }
  else
    {
      /* Create a new entry */
      if (!take_ownership)
        new_entry = mateconf_entry_copy (new_entry);
      
      g_hash_table_insert (client->cache_hash, new_entry->key, new_entry);
      trace ("Added value of '%s' to the cache",
             new_entry->key);

      return TRUE; /* changed */
    }
}

static gboolean
mateconf_client_lookup (MateConfClient *client,
                     const char  *key,
                     MateConfEntry **entryp)
{
  MateConfEntry *entry;

  g_return_val_if_fail (entryp != NULL, FALSE);
  g_return_val_if_fail (*entryp == NULL, FALSE);
  
  entry = g_hash_table_lookup (client->cache_hash, key);

  *entryp = entry;

  if (!entry)
  {
    char *dir, *last_slash;

    dir = g_strdup (key);
    last_slash = strrchr (dir, '/');
    g_assert (last_slash != NULL);
    *last_slash = 0;

    if (g_hash_table_lookup (client->cache_dirs, dir))
    {
      g_free (dir);
      trace ("Negative cache hit on %s", key);
      return TRUE;
    }

    g_free (dir);
  }

  return entry != NULL;
}

/*
 * Dir
 */

static Dir*
dir_new(const gchar* name, guint notify_id)
{
  Dir* d;

  d = g_new(Dir, 1);

  d->name = g_strdup(name);
  d->notify_id = notify_id;
  d->add_count = 0;
  
  return d;
}

static void
dir_destroy(Dir* d)
{
  g_return_if_fail(d != NULL);
  g_return_if_fail(d->notify_id == 0);
  
  g_free(d->name);
  g_free(d);
}

/*
 * Listener
 */

static Listener* 
listener_new(MateConfClientNotifyFunc func,
             GFreeFunc destroy_notify,
             gpointer data)
{
  Listener* l;

  l = g_new(Listener, 1);

  l->func = func;
  l->data = data;
  l->destroy_notify = destroy_notify;
  
  return l;
}

static void
listener_destroy(Listener* l)
{
  g_return_if_fail(l != NULL);

  if (l->destroy_notify)
    (* l->destroy_notify) (l->data);
  
  g_free(l);
}

/*
 * Change sets
 */


struct CommitData {
  MateConfClient* client;
  GError* error;
  GSList* remove_list;
  gboolean remove_committed;
};

static void
commit_foreach (MateConfChangeSet* cs,
                const gchar* key,
                MateConfValue* value,
                gpointer user_data)
{
  struct CommitData* cd = user_data;

  g_assert(cd != NULL);

  if (cd->error != NULL)
    return;
  
  if (value)
    mateconf_client_set   (cd->client, key, value, &cd->error);
  else
    mateconf_client_unset (cd->client, key, &cd->error);

  if (cd->error == NULL && cd->remove_committed)
    {
      /* Bad bad bad; we keep the key reference, knowing that it's
         valid until we modify the change set, to avoid string copies.  */
      cd->remove_list = g_slist_prepend(cd->remove_list, (gchar*)key);
    }
}

gboolean
mateconf_client_commit_change_set   (MateConfClient* client,
                                  MateConfChangeSet* cs,
                                  gboolean remove_committed,
                                  GError** err)
{
  struct CommitData cd;
  GSList* tmp;

  g_return_val_if_fail(client != NULL, FALSE);
  g_return_val_if_fail(MATECONF_IS_CLIENT(client), FALSE);
  g_return_val_if_fail(cs != NULL, FALSE);
  g_return_val_if_fail(err == NULL || *err == NULL, FALSE);
  
  cd.client = client;
  cd.error = NULL;
  cd.remove_list = NULL;
  cd.remove_committed = remove_committed;

  /* Because the commit could have lots of side
     effects, this makes it safer */
  mateconf_change_set_ref(cs);
  g_object_ref(G_OBJECT(client));
  
  mateconf_change_set_foreach(cs, commit_foreach, &cd);

  tmp = cd.remove_list;
  while (tmp != NULL)
    {
      const gchar* key = tmp->data;
      
      mateconf_change_set_remove(cs, key);

      /* key is now invalid due to our little evil trick */

      tmp = g_slist_next(tmp);
    }

  g_slist_free(cd.remove_list);
  
  mateconf_change_set_unref(cs);
  g_object_unref(G_OBJECT(client));

  if (cd.error != NULL)
    {
      if (err != NULL)
        *err = cd.error;
      else
        g_error_free(cd.error);

      return FALSE;
    }
  else
    {
      g_assert((!remove_committed) ||
               (mateconf_change_set_size(cs) == 0));
      
      return TRUE;
    }
}

struct RevertData {
  MateConfClient* client;
  GError* error;
  MateConfChangeSet* revert_set;
};

static void
revert_foreach (MateConfChangeSet* cs,
                const gchar* key,
                MateConfValue* value,
                gpointer user_data)
{
  struct RevertData* rd = user_data;
  MateConfValue* old_value;
  GError* error = NULL;
  
  g_assert(rd != NULL);

  if (rd->error != NULL)
    return;

  old_value = mateconf_client_get_without_default(rd->client, key, &error);

  if (error != NULL)
    {
      /* FIXME */
      g_warning("error creating revert set: %s", error->message);
      g_error_free(error);
      error = NULL;
    }
  
  if (old_value == NULL &&
      value == NULL)
    return; /* this commit will have no effect. */

  if (old_value == NULL)
    mateconf_change_set_unset(rd->revert_set, key);
  else
    mateconf_change_set_set_nocopy(rd->revert_set, key, old_value);
}


MateConfChangeSet*
mateconf_client_reverse_change_set  (MateConfClient* client,
                                         MateConfChangeSet* cs,
                                         GError** err)
{
  struct RevertData rd;

  rd.error = NULL;
  rd.client = client;
  rd.revert_set = mateconf_change_set_new();

  /* we're emitting signals and such, avoid
     nasty side effects with these.
  */
  g_object_ref(G_OBJECT(rd.client));
  mateconf_change_set_ref(cs);
  
  mateconf_change_set_foreach(cs, revert_foreach, &rd);

  if (rd.error != NULL)
    {
      if (err != NULL)
        *err = rd.error;
      else
        g_error_free(rd.error);
    }

  g_object_unref(G_OBJECT(rd.client));
  mateconf_change_set_unref(cs);
  
  return rd.revert_set;
}


MateConfChangeSet*
mateconf_client_change_set_from_currentv (MateConfClient* client,
                                              const gchar** keys,
                                              GError** err)
{
  MateConfValue* old_value;
  MateConfChangeSet* new_set;
  const gchar** keyp;
  
  g_return_val_if_fail(err == NULL || *err == NULL, NULL);

  new_set = mateconf_change_set_new();
  
  keyp = keys;

  while (*keyp != NULL)
    {
      GError* error = NULL;
      const gchar* key = *keyp;
      
      old_value = mateconf_client_get_without_default(client, key, &error);

      if (error != NULL)
        {
          /* FIXME */
          g_warning("error creating change set from current keys: %s", error->message);
          g_error_free(error);
          error = NULL;
        }
      
      if (old_value == NULL)
        mateconf_change_set_unset(new_set, key);
      else
        mateconf_change_set_set_nocopy(new_set, key, old_value);

      ++keyp;
    }

  return new_set;
}

MateConfChangeSet*
mateconf_client_change_set_from_current (MateConfClient* client,
                                             GError** err,
                                             const gchar* first_key,
                                             ...)
{
  GSList* keys = NULL;
  va_list args;
  const gchar* arg;
  const gchar** vec;
  MateConfChangeSet* retval;
  GSList* tmp;
  guint i;
  
  g_return_val_if_fail(err == NULL || *err == NULL, NULL);

  va_start (args, first_key);

  arg = first_key;

  while (arg != NULL)
    {
      keys = g_slist_prepend(keys, (/*non-const*/gchar*)arg);

      arg = va_arg (args, const gchar*);
    }
  
  va_end (args);

  vec = g_new0(const gchar*, g_slist_length(keys) + 1);

  i = 0;
  tmp = keys;

  while (tmp != NULL)
    {
      vec[i] = tmp->data;
      
      ++i;
      tmp = g_slist_next(tmp);
    }

  g_slist_free(keys);
  
  retval = mateconf_client_change_set_from_currentv(client, vec, err);
  
  g_free(vec);

  return retval;
}

static GHashTable * clients = NULL;

static void
register_client (MateConfClient *client)
{
  if (clients == NULL)
    clients = g_hash_table_new (NULL, NULL);

  g_hash_table_insert (clients, client->engine, client);
}

static MateConfClient *
lookup_client (MateConfEngine *engine)
{
  if (clients == NULL)
    return NULL;
  else
    return g_hash_table_lookup (clients, engine);
}

static void
unregister_client (MateConfClient *client)
{
  g_return_if_fail (clients != NULL);

  g_hash_table_remove (clients, client->engine);
}


/*
 * Notification
 */

static gboolean
notify_idle_callback (gpointer data)
{
  MateConfClient *client = data;

  client->notify_handler = 0; /* avoid g_source_remove */
  
  mateconf_client_flush_notifies (client);

  /* remove handler */
  return FALSE;
}

static void
mateconf_client_queue_notify (MateConfClient *client,
                           const char  *key)
{
  trace ("Queing notify on '%s', %d pending already", key,
         client->pending_notify_count);
  
  if (client->notify_handler == 0)
    client->notify_handler = g_idle_add (notify_idle_callback, client);
  
  client->notify_list = g_slist_prepend (client->notify_list, g_strdup (key));
  client->pending_notify_count += 1;
}

struct ClientAndEntry {
  MateConfClient* client;
  MateConfEntry* entry;
};

static void
notify_listeners_callback(MateConfListeners* listeners,
                          const gchar* key,
                          guint cnxn_id,
                          gpointer listener_data,
                          gpointer user_data)
{
  Listener* l = listener_data;
  struct ClientAndEntry* cae = user_data;
  
  g_return_if_fail (cae != NULL);
  g_return_if_fail (cae->client != NULL);
  g_return_if_fail (MATECONF_IS_CLIENT (cae->client));
  g_return_if_fail (l != NULL);
  g_return_if_fail (l->func != NULL);

  (*l->func) (cae->client, cnxn_id, cae->entry, l->data);
}
  
static void
notify_one_entry (MateConfClient *client,
                  MateConfEntry  *entry)
{
  g_object_ref (G_OBJECT (client));
  mateconf_entry_ref (entry);
  
  /* Emit the value_changed signal before notifying specific listeners;
   * I'm not sure there's a reason this matters though
   */
  mateconf_client_value_changed (client,
                              entry->key,
                              mateconf_entry_get_value (entry));

  /* Now notify our listeners, if any */
  if (client->listeners != NULL)
    {
      struct ClientAndEntry cae;

      cae.client = client;
      cae.entry = entry;
      
      mateconf_listeners_notify (client->listeners,
                              entry->key,
                              notify_listeners_callback,
                              &cae);
    }

  mateconf_entry_unref (entry);
  g_object_unref (G_OBJECT (client));
}

static void
mateconf_client_flush_notifies (MateConfClient *client)
{
  GSList *tmp;
  GSList *to_notify;
  MateConfEntry *last_entry;

  trace ("Flushing notify queue");
  
  /* Adopt notify list and clear it, to avoid reentrancy concerns.
   * Sort it to compress duplicates, and keep people from relying on
   * the notify order.
   */
  to_notify = g_slist_sort (client->notify_list, (GCompareFunc) strcmp);
  client->notify_list = NULL;
  client->pending_notify_count = 0;

  mateconf_client_unqueue_notifies (client);

  last_entry = NULL;
  tmp = to_notify;
  while (tmp != NULL)
    {
      MateConfEntry *entry = NULL;

      if (mateconf_client_lookup (client, tmp->data, &entry) && entry != NULL)
        {
          if (entry != last_entry)
            {
              trace ("Doing notification for '%s'", entry->key);
              notify_one_entry (client, entry);
              last_entry = entry;
            }
          else
            {
              trace ("Ignoring duplicate notify for '%s'", entry->key);
            }
        }
      else
        {
          trace ("Key '%s' was in notify queue but not in cache; we must have stopped monitoring it; not notifying",
                 tmp->data);
        }
      
      tmp = tmp->next;
    }
  
  g_slist_foreach (to_notify, (GFunc) g_free, NULL);
  g_slist_free (to_notify);
}

static void
mateconf_client_unqueue_notifies (MateConfClient *client)
{
  if (client->notify_handler != 0)
    {
      g_source_remove (client->notify_handler);
      client->notify_handler = 0;
    }
  
  if (client->notify_list != NULL)
    {
      g_slist_foreach (client->notify_list, (GFunc) g_free, NULL);
      g_slist_free (client->notify_list);
      client->notify_list = NULL;
      client->pending_notify_count = 0;
    }
}
