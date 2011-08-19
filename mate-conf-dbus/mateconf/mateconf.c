/* MateConf
 * Copyright (C) 1999, 2000 Red Hat Inc.
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <popt.h>
/*#include "MateConfX.h"*/
#include "mateconf.h"
#include "mateconf-internals.h"
#include "mateconf-sources.h"
#include "mateconf-locale.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#include <sys/time.h>
#include <unistd.h>

#ifdef HAVE_CORBA
/* Returns TRUE if there was an error, frees exception, sets err */
static gboolean mateconf_handle_corba_exception(CORBA_Environment* ev, GError** err);
/* just returns TRUE if there's an exception indicating the server is
   probably hosed; no side effects */
static gboolean mateconf_server_broken(CORBA_Environment* ev);
static void mateconf_detach_config_server(void);

/* Maximum number of times to try re-spawning the server if it's down. */
#define MAX_RETRIES 1
#endif /* HAVE_CORBA */

gboolean
mateconf_key_check(const gchar* key, GError** err)
{
  gchar* why = NULL;

  if (key == NULL)
    {
      if (err)
        *err = mateconf_error_new (MATECONF_ERROR_BAD_KEY,
                                _("Key \"%s\" is NULL"),
                                key);
      return FALSE;
    }
  else if (!mateconf_valid_key (key, &why))
    {
      if (err)
        *err = mateconf_error_new (MATECONF_ERROR_BAD_KEY, _("\"%s\": %s"),
                                key, why);
      g_free(why);
      return FALSE;
    }
  return TRUE;
}

#ifdef HAVE_CORBA
typedef struct _CnxnTable CnxnTable;

struct _MateConfEngine {
  guint refcount;

  ConfigDatabase database;

  CnxnTable* ctable;

  /* If non-NULL, this is a local engine;
     local engines don't do notification! */
  MateConfSources* local_sources;
  
  /* A list of addresses that make up this db
   * if this is not the default engine;
   * NULL if it's the default
   */
  GSList *addresses;

  /* A concatentation of the addresses above.
   */
  char *persistent_address;

  gpointer user_data;
  GDestroyNotify dnotify;

  gpointer owner;
  int owner_use_count;
  
  guint is_default : 1;

  /* If TRUE, this is a local engine (and therefore
   * has no ctable and no notifications)
   */
  guint is_local : 1;
};

typedef struct _MateConfCnxn MateConfCnxn;

struct _MateConfCnxn {
  gchar* namespace_section;
  guint client_id;
  CORBA_unsigned_long server_id; /* id returned from server */
  MateConfEngine* conf;             /* engine we're associated with */
  MateConfNotifyFunc func;
  gpointer user_data;
};

static MateConfEngine *default_engine = NULL;

static MateConfCnxn* mateconf_cnxn_new     (MateConfEngine         *conf,
                                      const gchar         *namespace_section,
                                      CORBA_unsigned_long  server_id,
                                      MateConfNotifyFunc      func,
                                      gpointer             user_data);
static void       mateconf_cnxn_destroy (MateConfCnxn           *cnxn);
static void       mateconf_cnxn_notify  (MateConfCnxn           *cnxn,
                                      MateConfEntry          *entry);


static ConfigServer   mateconf_get_config_server    (gboolean     start_if_not_found,
                                                  GError **err);

/* Forget our current server object reference, so the next call to
   mateconf_get_config_server will have to try to respawn the server */
static ConfigListener mateconf_get_config_listener  (void);

static void           mateconf_engine_detach       (MateConfEngine     *conf);
static gboolean       mateconf_engine_connect      (MateConfEngine     *conf,
                                                 gboolean         start_if_not_found,
                                                 GError         **err);
static void           mateconf_engine_set_database (MateConfEngine     *conf,
                                                 ConfigDatabase   db);
static ConfigDatabase mateconf_engine_get_database (MateConfEngine     *conf,
                                                 gboolean         start_if_not_found,
                                                 GError         **err);


#define CHECK_OWNER_USE(engine)   \
  do { if ((engine)->owner && (engine)->owner_use_count == 0) \
     g_warning ("%s: You can't use a MateConfEngine that has an active MateConfClient wrapper object. Use MateConfClient API instead.", G_GNUC_FUNCTION);  \
  } while (0)

static void         register_engine           (MateConfEngine    *conf);
static void         unregister_engine         (MateConfEngine    *conf);
static MateConfEngine *lookup_engine             (GSList         *addresses);
static MateConfEngine *lookup_engine_by_database (ConfigDatabase  db);


/* We'll use client-specific connection numbers to return to library
   users, so if mateconfd dies we can transparently re-register all our
   listener functions.  */

struct _CnxnTable {
  /* Hash from server-returned connection ID to MateConfCnxn */
  GHashTable* server_ids;
  /* Hash from our connection ID to MateConfCnxn */
  GHashTable* client_ids;
};

static CnxnTable* ctable_new                 (void);
static void       ctable_destroy             (CnxnTable           *ct);
static void       ctable_insert              (CnxnTable           *ct,
                                              MateConfCnxn           *cnxn);
static void       ctable_remove              (CnxnTable           *ct,
                                              MateConfCnxn           *cnxn);
static GSList*    ctable_remove_by_conf      (CnxnTable           *ct,
                                              MateConfEngine         *conf);
static MateConfCnxn* ctable_lookup_by_client_id (CnxnTable           *ct,
                                              guint                client_id);
static MateConfCnxn* ctable_lookup_by_server_id (CnxnTable           *ct,
                                              CORBA_unsigned_long  server_id);
static void       ctable_reinstall           (CnxnTable           *ct,
                                              MateConfCnxn           *cnxn,
                                              guint                old_server_id,
                                              guint                new_server_id);


static MateConfEngine*
mateconf_engine_blank (gboolean remote)
{
  MateConfEngine* conf;

  _mateconf_init_i18n ();
  
  conf = g_new0(MateConfEngine, 1);

  conf->refcount = 1;
  
  conf->owner = NULL;
  conf->owner_use_count = 0;
  
  if (remote)
    {
      conf->database = CORBA_OBJECT_NIL;
      conf->ctable = ctable_new();
      conf->local_sources = NULL;
      conf->is_local = FALSE;
      conf->is_default = TRUE;
    }
  else
    {
      conf->database = CORBA_OBJECT_NIL;
      conf->ctable = NULL;
      conf->local_sources = NULL;
      conf->is_local = TRUE;
      conf->is_default = FALSE;
    }
  
  return conf;
}

void
mateconf_engine_set_owner (MateConfEngine *engine,
                        gpointer     client)
{
  g_return_if_fail (engine->owner_use_count == 0);
  
  engine->owner = client;
}

void
mateconf_engine_push_owner_usage (MateConfEngine *engine,
                               gpointer     client)
{
  g_return_if_fail (engine->owner == client);

  engine->owner_use_count += 1;
}

void
mateconf_engine_pop_owner_usage  (MateConfEngine *engine,
                               gpointer     client)
{
  g_return_if_fail (engine->owner == client);
  g_return_if_fail (engine->owner_use_count > 0);

  engine->owner_use_count -= 1;
}

static GHashTable *engines_by_db = NULL;

static MateConfEngine *
lookup_engine_by_database (ConfigDatabase db)
{
  if (engines_by_db)
    return g_hash_table_lookup (engines_by_db, db);
  else
    return NULL;
}

static void
database_rec_release (gpointer rec)
{
  MateConfEngine *conf = rec;
  CORBA_Environment ev;

  CORBA_exception_init (&ev);

  CORBA_Object_release (conf->database, &ev);
  conf->database = CORBA_OBJECT_NIL;
  
  CORBA_exception_free (&ev);
}

/* This takes ownership of the ConfigDatabase */
static void
mateconf_engine_set_database (MateConfEngine *conf,
                           ConfigDatabase db)
{
  mateconf_engine_detach (conf);

  conf->database = db;

  if (engines_by_db == NULL)
    engines_by_db = g_hash_table_new_full (
	    (GHashFunc) mateconf_CORBA_Object_hash,
	    (GCompareFunc) mateconf_CORBA_Object_equal,
	    NULL,
	    database_rec_release);
  
  g_hash_table_insert (engines_by_db, conf->database, conf);  
}

static void
mateconf_engine_detach (MateConfEngine *conf)
{
  if (conf->database != CORBA_OBJECT_NIL)
    {
      g_hash_table_remove (engines_by_db, conf->database);
    }
}

static gboolean
mateconf_engine_connect (MateConfEngine *conf,
                      gboolean start_if_not_found,
                      GError **err)
{
  ConfigServer cs;
  ConfigDatabase db;
  int tries = 0;
  CORBA_Environment ev;
  
  g_return_val_if_fail (!conf->is_local, TRUE);
  
  CORBA_exception_init(&ev);

  if (!CORBA_Object_is_nil (conf->database, &ev))
    return TRUE;
  
 RETRY:
      
  cs = mateconf_get_config_server(start_if_not_found, err);
      
  if (cs == CORBA_OBJECT_NIL)
    return FALSE; /* Error should already be set */

  if (conf->is_default)
    {
      db = ConfigServer_get_default_database (cs, &ev);      
    }
  else if (conf->addresses->next == NULL) /* single element list */
    {
      db = ConfigServer_get_database (cs, conf->addresses->data, &ev);
    }
  else
    {
      ConfigServer2_AddressList *address_list;
      GSList                    *tmp;
      int                        i;

      address_list = ConfigServer2_AddressList__alloc ();
      address_list->_length  = address_list->_maximum = g_slist_length (conf->addresses);
      address_list->_buffer  = ConfigServer2_AddressList_allocbuf (address_list->_length);
      address_list->_release = CORBA_TRUE;

      i = 0;
      tmp = conf->addresses;
      while (tmp != NULL)
        {
          g_assert (i < address_list->_length);

          address_list->_buffer [i] = CORBA_string_dup (tmp->data);

          tmp = tmp->next;
          i++;
        }

      db = ConfigServer2_get_database_for_addresses ((ConfigServer2) cs, address_list, &ev);

      CORBA_free (address_list);
    }

  if (mateconf_server_broken(&ev))
    {
      if (tries < MAX_RETRIES)
        {
          ++tries;
          CORBA_exception_free(&ev);
          mateconf_detach_config_server();
          goto RETRY;
        }
    }
  
  if (mateconf_handle_corba_exception(&ev, err))
    return FALSE;

  if (CORBA_Object_is_nil (db, &ev))
    {
      if (err)
        *err = mateconf_error_new(MATECONF_ERROR_BAD_ADDRESS,
                               _("Server couldn't resolve the address `%s'"),
                               conf->persistent_address);
          
      return FALSE;
    }

  mateconf_engine_set_database (conf, db);
  
  return TRUE;
}

static ConfigDatabase
mateconf_engine_get_database (MateConfEngine *conf,
                           gboolean start_if_not_found,
                           GError **err)
{
  if (!mateconf_engine_connect (conf, start_if_not_found, err))
    return CORBA_OBJECT_NIL;
  else
    return conf->database;
}

static gboolean
mateconf_engine_is_local(MateConfEngine* conf)
{
  return conf->is_local;
}

static GHashTable *engines_by_address = NULL;

static void
register_engine (MateConfEngine *conf)
{
  g_return_if_fail (conf->addresses != NULL);

  g_assert (conf->persistent_address == NULL);

  conf->persistent_address = 
          mateconf_address_list_get_persistent_name (conf->addresses);

  if (engines_by_address == NULL)
    engines_by_address = g_hash_table_new (g_str_hash, g_str_equal);

  g_hash_table_insert (engines_by_address, conf->persistent_address, conf);
}

static void
unregister_engine (MateConfEngine *conf)
{
  g_return_if_fail (engines_by_address != NULL);

  g_assert (conf->persistent_address != NULL);
  
  g_hash_table_remove (engines_by_address, conf->persistent_address);
  g_free (conf->persistent_address);
  conf->persistent_address = NULL;

  if (g_hash_table_size (engines_by_address) == 0)
    {
      g_hash_table_destroy (engines_by_address);
      
      engines_by_address = NULL;
    }
}

static MateConfEngine *
lookup_engine (GSList *addresses)
{
  if (engines_by_address != NULL)
    {
      MateConfEngine *retval;
      char        *key;

      key = mateconf_address_list_get_persistent_name (addresses);

      retval = g_hash_table_lookup (engines_by_address, key);

      g_free (key);

      return retval;
    }

  return NULL;
}


/*
 *  Public Interface
 */

MateConfEngine*
mateconf_engine_get_local      (const gchar* address,
                             GError** err)
{
  MateConfEngine* conf;
  MateConfSource* source;

  g_return_val_if_fail(address != NULL, NULL);
  g_return_val_if_fail(err == NULL || *err == NULL, NULL);
  
  source = mateconf_resolve_address(address, err);

  if (source == NULL)
    return NULL;
  
  conf = mateconf_engine_blank(FALSE);

  conf->local_sources = mateconf_sources_new_from_source(source);

  g_assert (mateconf_engine_is_local (conf));
  
  return conf;
}

MateConfEngine *
mateconf_engine_get_local_for_addresses (GSList  *addresses,
				      GError **err)
{
  MateConfEngine *conf;

  g_return_val_if_fail (addresses != NULL, NULL);
  g_return_val_if_fail (err == NULL || *err == NULL, NULL);
  
  conf = mateconf_engine_blank (FALSE);

  conf->local_sources = mateconf_sources_new_from_addresses (addresses, err);

  g_assert (mateconf_engine_is_local (conf));
  
  return conf;
}

MateConfEngine*
mateconf_engine_get_default (void)
{
  MateConfEngine* conf = NULL;
  
  if (default_engine)
    conf = default_engine;

  if (conf == NULL)
    {
      conf = mateconf_engine_blank(TRUE);

      conf->is_default = TRUE;

      default_engine = conf;
      
      /* Ignore errors, we never return a NULL default database, and
       * since we aren't starting if it isn't found, we'll probably
       * get errors half the time anyway.
       */
      mateconf_engine_connect (conf, FALSE, NULL);
    }
  else
    conf->refcount += 1;
  
  return conf;
}

MateConfEngine*
mateconf_engine_get_for_address (const char  *address,
			      GError     **err)
{
  MateConfEngine *conf;
  GSList      *addresses;

  addresses = g_slist_append (NULL, g_strdup (address));

  conf = lookup_engine (addresses);

  if (conf == NULL)
    {
      conf = mateconf_engine_blank (TRUE);

      conf->is_default = FALSE;
      conf->addresses = addresses;

      if (!mateconf_engine_connect (conf, TRUE, err))
        {
          mateconf_engine_unref (conf);
          return NULL;
        }

      register_engine (conf);
    }
  else
    {
      g_free (addresses->data);
      g_slist_free (addresses);
      conf->refcount += 1;
    }
  
  return conf;
}

MateConfEngine*
mateconf_engine_get_for_addresses (GSList *addresses, GError** err)
{
  MateConfEngine* conf;

  conf = lookup_engine (addresses);

  if (conf == NULL)
    {
      GSList *tmp;

      conf = mateconf_engine_blank (TRUE);

      conf->is_default = FALSE;
      conf->addresses = NULL;

      tmp = addresses;
      while (tmp != NULL)
        {
          conf->addresses = g_slist_append (conf->addresses,
                                            g_strdup (tmp->data));
          tmp = tmp->next;
        }

      if (!mateconf_engine_connect (conf, TRUE, err))
        {
          mateconf_engine_unref (conf);
          return NULL;
        }

      register_engine (conf);
    }
  else
    conf->refcount += 1;
  
  return conf;
}

void
mateconf_engine_ref(MateConfEngine* conf)
{
  g_return_if_fail(conf != NULL);
  g_return_if_fail(conf->refcount > 0);

  conf->refcount += 1;
}

void         
mateconf_engine_unref(MateConfEngine* conf)
{
  g_return_if_fail(conf != NULL);
  g_return_if_fail(conf->refcount > 0);

  conf->refcount -= 1;
  
  if (conf->refcount == 0)
    {
      if (mateconf_engine_is_local(conf))
        {
          if (conf->local_sources != NULL)
            mateconf_sources_free(conf->local_sources);
        }
      else
        {
          /* Remove all connections associated with this MateConf */
          GSList* removed;
          GSList* tmp;
          CORBA_Environment ev;
      
          CORBA_exception_init(&ev);

          /* FIXME CnxnTable only has entries for this MateConfEngine now,
           * it used to be global and shared among MateConfEngine objects.
           */
          removed = ctable_remove_by_conf (conf->ctable, conf);
  
          tmp = removed;
          while (tmp != NULL)
            {
              MateConfCnxn* gcnxn = tmp->data;

              if (!CORBA_Object_is_nil (conf->database, &ev))
                {
                  GError* err = NULL;
              
                  ConfigDatabase_remove_listener(conf->database,
                                                 gcnxn->server_id,
                                                 &ev);

                  if (mateconf_handle_corba_exception(&ev, &err))
                    {
                      /* Don't set error because realistically this
                         doesn't matter to clients */
#ifdef MATECONF_ENABLE_DEBUG
                      g_warning("Failure removing listener %u from the config server: %s",
                                (guint)gcnxn->server_id,
                                err->message);
#endif
                    }
                }

              mateconf_cnxn_destroy(gcnxn);

              tmp = g_slist_next(tmp);
            }

          g_slist_free(removed);

          if (conf->dnotify)
            {
              (* conf->dnotify) (conf->user_data);
            }
          
          if (conf->addresses)
	    {
	      mateconf_address_list_free (conf->addresses);
	      conf->addresses = NULL;
	    }

	  if (conf->persistent_address)
	    {
	      unregister_engine (conf);
	    }

          /* Release the ConfigDatabase */
          mateconf_engine_detach (conf);
          
          ctable_destroy (conf->ctable);
        }

      if (conf == default_engine)
        default_engine = NULL;

      g_free(conf);
    }
}

void
mateconf_engine_set_user_data  (MateConfEngine   *engine,
                             gpointer       data,
                             GDestroyNotify dnotify)
{
  if (engine->dnotify)
    {
      (* engine->dnotify) (engine->user_data);
    }

  engine->dnotify = dnotify;
  engine->user_data = data;
}

gpointer
mateconf_engine_get_user_data  (MateConfEngine   *engine)
{
  return engine->user_data;
}

guint
mateconf_engine_notify_add(MateConfEngine* conf,
                        const gchar* namespace_section,
                        MateConfNotifyFunc func,
                        gpointer user_data,
                        GError** err)
{
  ConfigDatabase db;
  ConfigListener cl;
  gulong id;
  CORBA_Environment ev;
  MateConfCnxn* cnxn;
  gint tries = 0;
  ConfigDatabase3_PropList properties;
#define NUM_PROPERTIES 1
  ConfigStringProperty properties_buffer[1];
  
  g_return_val_if_fail(!mateconf_engine_is_local(conf), 0);

  CHECK_OWNER_USE (conf);
  
  if (mateconf_engine_is_local(conf))
    {
      if (err)
        *err = mateconf_error_new(MATECONF_ERROR_LOCAL_ENGINE,
                               _("Can't add notifications to a local configuration source"));

      return 0;
    }

  properties._buffer = properties_buffer;
  properties._length = NUM_PROPERTIES;
  properties._maximum = NUM_PROPERTIES;
  properties._release = CORBA_FALSE; /* don't free static buffer */

  properties._buffer[0].key = "name";
  properties._buffer[0].value = g_get_prgname ();
  if (properties._buffer[0].value == NULL)
    properties._buffer[0].value = "unknown";
  
  CORBA_exception_init(&ev);

 RETRY:
  
  db = mateconf_engine_get_database (conf, TRUE, err);

  if (db == CORBA_OBJECT_NIL)
    return 0;

  cl = mateconf_get_config_listener ();
  
  /* Should have aborted the program in this case probably */
  g_return_val_if_fail(cl != CORBA_OBJECT_NIL, 0);
  
  id = ConfigDatabase3_add_listener_with_properties (db,
                                                     (gchar*)namespace_section, 
                                                     cl,
                                                     &properties,
                                                     &ev);
  
  if (ev._major == CORBA_SYSTEM_EXCEPTION &&
      CORBA_exception_id (&ev) &&
      strcmp (CORBA_exception_id (&ev), "IDL:CORBA/BAD_OPERATION:1.0") == 0)
    {
      CORBA_exception_free (&ev);
      CORBA_exception_init (&ev);      
  
      id = ConfigDatabase_add_listener(db,
                                       (gchar*)namespace_section, 
                                       cl, &ev);
    }
  
  if (mateconf_server_broken(&ev))
    {
      if (tries < MAX_RETRIES)
        {
          ++tries;
          CORBA_exception_free(&ev);
          mateconf_engine_detach (conf);
          goto RETRY;
        }
    }
  
  if (mateconf_handle_corba_exception(&ev, err))
    return 0;

  cnxn = mateconf_cnxn_new(conf, namespace_section, id, func, user_data);

  ctable_insert(conf->ctable, cnxn);

  return cnxn->client_id;
}

void         
mateconf_engine_notify_remove(MateConfEngine* conf,
                           guint client_id)
{
  MateConfCnxn* gcnxn;
  CORBA_Environment ev;
  ConfigDatabase db;
  gint tries = 0;

  CHECK_OWNER_USE (conf);
  
  if (mateconf_engine_is_local(conf))
    return;
  
  CORBA_exception_init(&ev);

 RETRY:
  
  db = mateconf_engine_get_database (conf, TRUE, NULL);

  if (db == CORBA_OBJECT_NIL)
    return;

  gcnxn = ctable_lookup_by_client_id(conf->ctable, client_id);

  g_return_if_fail(gcnxn != NULL);

  ConfigDatabase_remove_listener(db,
                                 gcnxn->server_id,
                                 &ev);

  if (mateconf_server_broken(&ev))
    {
      if (tries < MAX_RETRIES)
        {
          ++tries;
          CORBA_exception_free(&ev);
          mateconf_engine_detach (conf);
          goto RETRY;
        }
    }
  
  if (mateconf_handle_corba_exception(&ev, NULL))
    {
      ; /* do nothing */
    }
  

  /* We want to do this even if the CORBA fails, so if we restart mateconfd and 
     reinstall listeners we don't reinstall this one. */
  ctable_remove(conf->ctable, gcnxn);

  mateconf_cnxn_destroy(gcnxn);
}

MateConfValue *
mateconf_engine_get_fuller (MateConfEngine *conf,
                         const gchar *key,
                         const gchar *locale,
                         gboolean use_schema_default,
                         gboolean *is_default_p,
                         gboolean *is_writable_p,
                         gchar   **schema_name_p,
                         GError **err)
{
  MateConfValue* val;
  ConfigValue* cv;
  CORBA_Environment ev;
  ConfigDatabase db;
  gint tries = 0;
  CORBA_boolean is_default = FALSE;
  CORBA_boolean is_writable = TRUE;
  CORBA_char *corba_schema_name = NULL;
  
  g_return_val_if_fail(conf != NULL, NULL);
  g_return_val_if_fail(key != NULL, NULL);
  g_return_val_if_fail(err == NULL || *err == NULL, NULL);

  CHECK_OWNER_USE (conf);
  
  if (!mateconf_key_check(key, err))
    return NULL;

  if (mateconf_engine_is_local(conf))
    {
      gchar** locale_list;
      gboolean tmp_is_default = FALSE;
      gboolean tmp_is_writable = TRUE;
      gchar *tmp_schema_name = NULL;
      
      locale_list = mateconf_split_locale(locale);
      
      val = mateconf_sources_query_value(conf->local_sources,
                                      key,
                                      (const gchar**)locale_list,
                                      use_schema_default,
                                      &tmp_is_default,
                                      &tmp_is_writable,
                                      schema_name_p ? &tmp_schema_name : NULL,
                                      err);

      if (locale_list != NULL)
        g_strfreev(locale_list);
      
      if (is_default_p)
        *is_default_p = tmp_is_default;

      if (is_writable_p)
        *is_writable_p = tmp_is_writable;

      if (schema_name_p)
        *schema_name_p = tmp_schema_name;
      else
        g_free (tmp_schema_name);
      
      return val;
    }

  g_assert(!mateconf_engine_is_local(conf));
  
  CORBA_exception_init(&ev);

 RETRY:
  
  db = mateconf_engine_get_database (conf, TRUE, err);

  if (db == CORBA_OBJECT_NIL)
    {
      g_return_val_if_fail(err == NULL || *err != NULL, NULL);

      return NULL;
    }

  if (schema_name_p)
    *schema_name_p = NULL;


  corba_schema_name = NULL;
  cv = ConfigDatabase2_lookup_with_schema_name (db,
                                                (gchar*)key, (gchar*)
                                                (locale ? locale : mateconf_current_locale()),
                                                use_schema_default,
                                                &corba_schema_name,
                                                &is_default,
                                                &is_writable,
                                                &ev);

  if (ev._major == CORBA_SYSTEM_EXCEPTION &&
      CORBA_exception_id (&ev) &&
      strcmp (CORBA_exception_id (&ev), "IDL:CORBA/BAD_OPERATION:1.0") == 0)
    {
      CORBA_exception_free (&ev);
      CORBA_exception_init (&ev);
      
      cv = ConfigDatabase_lookup_with_locale(db,
                                             (gchar*)key, (gchar*)
                                             (locale ? locale : mateconf_current_locale()),
                                             use_schema_default,
                                             &is_default,
                                             &is_writable,
                                             &ev);
    }
  
  if (mateconf_server_broken(&ev))
    {
      if (tries < MAX_RETRIES)
        {
          ++tries;
          CORBA_exception_free(&ev);
          mateconf_engine_detach (conf);
          goto RETRY;
        }
    }
  
  if (mateconf_handle_corba_exception(&ev, err))
    {
      /* NOTE: don't free cv since we got an exception! */
      return NULL;
    }
  else
    {
      val = mateconf_value_from_corba_value(cv);
      CORBA_free(cv);

      if (is_default_p)
        *is_default_p = !!is_default;
      if (is_writable_p)
        *is_writable_p = !!is_writable;

      /* we can't get a null pointer through corba
       * so the server sent us an empty string
       */
      if (corba_schema_name && corba_schema_name[0] != '/')
        {
          CORBA_free (corba_schema_name);
          corba_schema_name = NULL;
        }

      if (schema_name_p)
        *schema_name_p = g_strdup (corba_schema_name);

      if (corba_schema_name)
        CORBA_free (corba_schema_name);
      
      return val;
    }
}


MateConfValue *
mateconf_engine_get_full (MateConfEngine *conf,
                       const gchar *key,
                       const gchar *locale,
                       gboolean use_schema_default,
                       gboolean *is_default_p,
                       gboolean *is_writable_p,
                       GError **err)
{
  return mateconf_engine_get_fuller (conf, key, locale, use_schema_default,
                                  is_default_p, is_writable_p,
                                  NULL, err);
}

MateConfEntry*
mateconf_engine_get_entry(MateConfEngine* conf,
                       const gchar* key,
                       const gchar* locale,
                       gboolean use_schema_default,
                       GError** err)
{
  gboolean is_writable = TRUE;
  gboolean is_default = FALSE;
  MateConfValue *val;
  GError *error;
  MateConfEntry *entry;
  gchar *schema_name;

  CHECK_OWNER_USE (conf);
  
  schema_name = NULL;
  error = NULL;
  val = mateconf_engine_get_fuller (conf, key, locale, use_schema_default,
                                 &is_default, &is_writable,
                                 &schema_name, &error);
  if (error != NULL)
    {
      g_propagate_error (err, error);
      return NULL;
    }

  entry = mateconf_entry_new_nocopy (g_strdup (key),
                                  val);

  mateconf_entry_set_is_default (entry, is_default);
  mateconf_entry_set_is_writable (entry, is_writable);
  mateconf_entry_set_schema_name (entry, schema_name);
  g_free (schema_name);

  return entry;
}
     
MateConfValue*  
mateconf_engine_get (MateConfEngine* conf, const gchar* key, GError** err)
{
  return mateconf_engine_get_with_locale(conf, key, NULL, err);
}

MateConfValue*
mateconf_engine_get_with_locale(MateConfEngine* conf, const gchar* key,
                             const gchar* locale,
                             GError** err)
{
  return mateconf_engine_get_full(conf, key, locale, TRUE,
                               NULL, NULL, err);
}

MateConfValue*
mateconf_engine_get_without_default(MateConfEngine* conf, const gchar* key,
                                 GError** err)
{
  return mateconf_engine_get_full(conf, key, NULL, FALSE, NULL, NULL, err);
}

MateConfValue*
mateconf_engine_get_default_from_schema (MateConfEngine* conf,
                                      const gchar* key,
                                      GError** err)
{
  MateConfValue* val;
  ConfigValue* cv;
  CORBA_Environment ev;
  ConfigDatabase db;
  gint tries = 0;

  g_return_val_if_fail(conf != NULL, NULL);
  g_return_val_if_fail(key != NULL, NULL);
  g_return_val_if_fail(err == NULL || *err == NULL, NULL);

  CHECK_OWNER_USE (conf);
  
  if (!mateconf_key_check(key, err))
    return NULL;

  if (mateconf_engine_is_local(conf))
    {
      gchar** locale_list;

      locale_list = mateconf_split_locale(mateconf_current_locale());
      
      val = mateconf_sources_query_default_value(conf->local_sources,
                                              key,
                                              (const gchar**)locale_list,
                                              NULL,
                                              err);

      if (locale_list != NULL)
        g_strfreev(locale_list);
      
      return val;
    }

  g_assert(!mateconf_engine_is_local(conf));
  
  CORBA_exception_init(&ev);

 RETRY:
  
  db = mateconf_engine_get_database (conf, TRUE, err);

  if (db == CORBA_OBJECT_NIL)
    {
      g_return_val_if_fail(err == NULL || *err != NULL, NULL);

      return NULL;
    }

  cv = ConfigDatabase_lookup_default_value(db,
                                           (gchar*)key,
                                           (gchar*)mateconf_current_locale(),
                                           &ev);
  
  if (mateconf_server_broken(&ev))
    {
      if (tries < MAX_RETRIES)
        {
          ++tries;
          CORBA_exception_free(&ev);
          mateconf_engine_detach (conf);
          goto RETRY;
        }
    }
  
  if (mateconf_handle_corba_exception(&ev, err))
    {
      /* NOTE: don't free cv since we got an exception! */
      return NULL;
    }
  else
    {
      val = mateconf_value_from_corba_value(cv);
      CORBA_free(cv);

      return val;
    }
}

gboolean
mateconf_engine_set (MateConfEngine* conf, const gchar* key,
                  const MateConfValue* value, GError** err)
{
  ConfigValue* cv;
  CORBA_Environment ev;
  ConfigDatabase db;
  gint tries = 0;

  g_return_val_if_fail(conf != NULL, FALSE);
  g_return_val_if_fail(key != NULL, FALSE);
  g_return_val_if_fail(value != NULL, FALSE);
  g_return_val_if_fail(value->type != MATECONF_VALUE_INVALID, FALSE);
  g_return_val_if_fail( (value->type != MATECONF_VALUE_STRING) ||
                        (mateconf_value_get_string(value) != NULL) , FALSE );
  g_return_val_if_fail( (value->type != MATECONF_VALUE_LIST) ||
                        (mateconf_value_get_list_type(value) != MATECONF_VALUE_INVALID), FALSE);
  g_return_val_if_fail(err == NULL || *err == NULL, FALSE);

  CHECK_OWNER_USE (conf);
  
  if (!mateconf_key_check(key, err))
    return FALSE;

  if (!mateconf_value_validate (value, err))
    return FALSE;
  
  if (mateconf_engine_is_local(conf))
    {
      GError* error = NULL;
      
      mateconf_sources_set_value(conf->local_sources, key, value, NULL, &error);

      if (error != NULL)
        {
          if (err)
            *err = error;
          else
            {
              g_error_free(error);
            }
          return FALSE;
        }
      
      return TRUE;
    }

  g_assert(!mateconf_engine_is_local(conf));
  
  CORBA_exception_init(&ev);

 RETRY:
  
  db = mateconf_engine_get_database (conf, TRUE, err);

  if (db == CORBA_OBJECT_NIL)
    {
      g_return_val_if_fail(err == NULL || *err != NULL, FALSE);

      return FALSE;
    }

  cv = mateconf_corba_value_from_mateconf_value (value);

  ConfigDatabase_set(db,
                     (gchar*)key, cv,
                     &ev);

  CORBA_free(cv);

  if (mateconf_server_broken(&ev))
    {
      if (tries < MAX_RETRIES)
        {
          ++tries;
          CORBA_exception_free(&ev);
          mateconf_engine_detach (conf);
          goto RETRY;
        }
    }
  
  if (mateconf_handle_corba_exception(&ev, err))
    return FALSE;

  g_return_val_if_fail(err == NULL || *err == NULL, FALSE);
  
  return TRUE;
}

gboolean
mateconf_engine_unset (MateConfEngine* conf, const gchar* key, GError** err)
{
  CORBA_Environment ev;
  ConfigDatabase db;
  gint tries = 0;

  g_return_val_if_fail (conf != NULL, FALSE);
  g_return_val_if_fail (key != NULL, FALSE);
  g_return_val_if_fail (err == NULL || *err == NULL, FALSE);

  CHECK_OWNER_USE (conf);
  
  if (!mateconf_key_check(key, err))
    return FALSE;

  if (mateconf_engine_is_local(conf))
    {
      GError* error = NULL;
      
      mateconf_sources_unset_value(conf->local_sources, key, NULL, NULL, &error);

      if (error != NULL)
        {
          if (err)
            *err = error;
          else
            {
              g_error_free(error);
            }
          return FALSE;
        }
      
      return TRUE;
    }

  g_assert(!mateconf_engine_is_local(conf));
  
  CORBA_exception_init(&ev);

 RETRY:
  
  db = mateconf_engine_get_database (conf, TRUE, err);

  if (db == CORBA_OBJECT_NIL)
    {
      g_return_val_if_fail(err == NULL || *err != NULL, FALSE);

      return FALSE;
    }

  ConfigDatabase_unset (db,
                        (gchar*)key,
                        &ev);

  if (mateconf_server_broken (&ev))
    {
      if (tries < MAX_RETRIES)
        {
          ++tries;
          CORBA_exception_free(&ev);
          mateconf_engine_detach(conf);
          goto RETRY;
        }
    }
  
  if (mateconf_handle_corba_exception (&ev, err))
    return FALSE;

  g_return_val_if_fail (err == NULL || *err == NULL, FALSE);
  
  return TRUE;
}

/**
 * mateconf_engine_recursive_unset:
 * @engine: a #MateConfEngine
 * @key: a key or directory name
 * @flags: change how the unset is done
 * @err: return location for a #GError, or %NULL to ignore errors
 * 
 * Unsets all keys below @key, including @key itself.  If any unset
 * fails, continues on to unset as much as it can. The first
 * failure is returned in @err.
 *
 * Returns: %FALSE if error is set
 **/
gboolean
mateconf_engine_recursive_unset (MateConfEngine    *conf,
                              const char     *key,
                              MateConfUnsetFlags flags,
                              GError        **err)
{
  CORBA_Environment ev;
  ConfigDatabase3 db;
  gint tries = 0;
  ConfigDatabase3_UnsetFlags corba_flags;
  
  g_return_val_if_fail (conf != NULL, FALSE);
  g_return_val_if_fail (key != NULL, FALSE);
  g_return_val_if_fail (err == NULL || *err == NULL, FALSE);

  CHECK_OWNER_USE (conf);
  
  if (!mateconf_key_check (key, err))
    return FALSE;

  if (mateconf_engine_is_local (conf))
    {
      GError* error = NULL;
      
      mateconf_sources_recursive_unset (conf->local_sources, key, NULL,
                                     flags, NULL, &error);

      if (error != NULL)
        {
          if (err)
            *err = error;
          else
            {
              g_error_free (error);
            }
          return FALSE;
        }
      
      return TRUE;
    }

  g_assert (!mateconf_engine_is_local (conf));
  
  CORBA_exception_init(&ev);

  corba_flags = 0;
  if (flags & MATECONF_UNSET_INCLUDING_SCHEMA_NAMES)
    corba_flags |= ConfigDatabase3_UNSET_INCLUDING_SCHEMA_NAMES;
  
 RETRY:
  
  db = (ConfigDatabase3) mateconf_engine_get_database (conf, TRUE, err);

  if (db == CORBA_OBJECT_NIL)
    {
      g_return_val_if_fail (err == NULL || *err != NULL, FALSE);

      return FALSE;
    }

  ConfigDatabase3_recursive_unset (db, key, corba_flags, &ev);

  if (mateconf_server_broken (&ev))
    {
      if (tries < MAX_RETRIES)
        {
          ++tries;
          CORBA_exception_free(&ev);
          mateconf_engine_detach(conf);
          goto RETRY;
        }
    }
  
  if (mateconf_handle_corba_exception (&ev, err))
    return FALSE;

  g_return_val_if_fail (err == NULL || *err == NULL, FALSE);
  
  return TRUE;
}

gboolean
mateconf_engine_associate_schema  (MateConfEngine* conf, const gchar* key,
                                const gchar* schema_key, GError** err)
{
  CORBA_Environment ev;
  ConfigDatabase db;
  gint tries = 0;

  g_return_val_if_fail (conf != NULL, FALSE);
  g_return_val_if_fail (key != NULL, FALSE);
  g_return_val_if_fail (err == NULL || *err == NULL, FALSE);
  
  if (!mateconf_key_check (key, err))
    return FALSE;

  if (schema_key && !mateconf_key_check (schema_key, err))
    return FALSE;

  if (mateconf_engine_is_local(conf))
    {
      GError* error = NULL;
      
      mateconf_sources_set_schema (conf->local_sources, key, schema_key, &error);

      if (error != NULL)
        {
          if (err)
            *err = error;
          else
            {
              g_error_free(error);
            }
          return FALSE;
        }
      
      return TRUE;
    }

  g_assert (!mateconf_engine_is_local (conf));
  
  CORBA_exception_init (&ev);

 RETRY:
  
  db = mateconf_engine_get_database (conf, TRUE, err);

  if (db == CORBA_OBJECT_NIL)
    {
      g_return_val_if_fail (err == NULL || *err != NULL, FALSE);

      return FALSE;
    }

  ConfigDatabase_set_schema (db,
                             key,
                             /* empty string means unset */
                             schema_key ? schema_key : "",
                             &ev);

  if (mateconf_server_broken (&ev))
    {
      if (tries < MAX_RETRIES)
        {
          ++tries;
          CORBA_exception_free (&ev);
          mateconf_engine_detach (conf);
          goto RETRY;
        }
    }
  
  if (mateconf_handle_corba_exception(&ev, err))
    return FALSE;

  g_return_val_if_fail (err == NULL || *err == NULL, FALSE);
  
  return TRUE;
}


static void
qualify_entries (GSList *entries, const char *dir)
{
  GSList *tmp = entries;
  while (tmp != NULL)
    {
      MateConfEntry *entry = tmp->data;
      gchar *full;

      full = mateconf_concat_dir_and_key (dir, entry->key);

      g_free (entry->key);
      entry->key = full;

      tmp = g_slist_next (tmp);
    }
}

GSList*      
mateconf_engine_all_entries(MateConfEngine* conf, const gchar* dir, GError** err)
{
  GSList* pairs = NULL;
  ConfigDatabase_ValueList* values;
  ConfigDatabase_KeyList* keys;
  ConfigDatabase_IsDefaultList* is_defaults;
  ConfigDatabase_IsWritableList* is_writables;
  ConfigDatabase2_SchemaNameList *schema_names;
  CORBA_Environment ev;
  ConfigDatabase db;
  guint i;
  gint tries = 0;

  g_return_val_if_fail(conf != NULL, NULL);
  g_return_val_if_fail(dir != NULL, NULL);
  g_return_val_if_fail(err == NULL || *err == NULL, NULL);

  CHECK_OWNER_USE (conf);
  
  if (!mateconf_key_check(dir, err))
    return NULL;


  if (mateconf_engine_is_local(conf))
    {
      GError* error = NULL;
      gchar** locale_list;
      GSList* retval;
      
      locale_list = mateconf_split_locale(mateconf_current_locale());
      
      retval = mateconf_sources_all_entries(conf->local_sources,
                                         dir,
                                         (const gchar**)locale_list,
                                         &error);

      if (locale_list)
        g_strfreev(locale_list);
      
      if (error != NULL)
        {
          if (err)
            *err = error;
          else
            {
              g_error_free(error);
            }

          g_assert(retval == NULL);
          
          return NULL;
        }

      qualify_entries (retval, dir);
      
      return retval;
    }

  g_assert(!mateconf_engine_is_local(conf));
  
  CORBA_exception_init(&ev);
  
 RETRY:
  
  db = mateconf_engine_get_database (conf, TRUE, err);

  if (db == CORBA_OBJECT_NIL)
    {
      g_return_val_if_fail(err == NULL || *err != NULL, NULL);

      return NULL;
    }

  schema_names = NULL;
  
  ConfigDatabase2_all_entries_with_schema_name (db,
                                                (gchar*)dir,
                                                (gchar*)mateconf_current_locale(),
                                                &keys, &values, &schema_names,
                                                &is_defaults, &is_writables,
                                                &ev);
  
  if (ev._major == CORBA_SYSTEM_EXCEPTION &&
      CORBA_exception_id (&ev) &&
      strcmp (CORBA_exception_id (&ev), "IDL:CORBA/BAD_OPERATION:1.0") == 0)
    {
      CORBA_exception_free (&ev);
      CORBA_exception_init (&ev);
      
      ConfigDatabase_all_entries(db,
                                 (gchar*)dir,
                                 (gchar*)mateconf_current_locale(),
                                 &keys, &values, &is_defaults, &is_writables,
                                 &ev);
    }

  if (mateconf_server_broken(&ev))
    {
      if (tries < MAX_RETRIES)
        {
          ++tries;
          CORBA_exception_free(&ev);
          mateconf_engine_detach (conf);
          goto RETRY;
        }
    }
  
  if (mateconf_handle_corba_exception(&ev, err))
    return NULL;
  
  if (keys->_length != values->_length)
    {
      g_warning("Received unmatched key/value sequences in %s",
                G_GNUC_FUNCTION);
      return NULL;
    }

  i = 0;
  while (i < keys->_length)
    {
      MateConfEntry* pair;

      pair = 
        mateconf_entry_new_nocopy(mateconf_concat_dir_and_key (dir, keys->_buffer[i]),
                               mateconf_value_from_corba_value(&(values->_buffer[i])));

      mateconf_entry_set_is_default (pair, is_defaults->_buffer[i]);
      mateconf_entry_set_is_writable (pair, is_writables->_buffer[i]);
      if (schema_names)
        {
          /* empty string means no schema name */
          if (*(schema_names->_buffer[i]) != '\0')
            mateconf_entry_set_schema_name (pair, schema_names->_buffer[i]);
        }
      
      pairs = g_slist_prepend(pairs, pair);
      
      ++i;
    }
  
  CORBA_free(keys);
  CORBA_free(values);
  CORBA_free(is_defaults);
  CORBA_free(is_writables);
  if (schema_names)
    CORBA_free (schema_names);
  
  return pairs;
}


static void
qualify_keys (GSList *keys, const char *dir)
{
  GSList *tmp = keys;
  while (tmp != NULL)
    {
      char *key = tmp->data;
      gchar *full;

      full = mateconf_concat_dir_and_key (dir, key);

      g_free (tmp->data);
      tmp->data = full;

      tmp = g_slist_next (tmp);
    }
}

GSList*      
mateconf_engine_all_dirs(MateConfEngine* conf, const gchar* dir, GError** err)
{
  GSList* subdirs = NULL;
  ConfigDatabase_KeyList* keys;
  CORBA_Environment ev;
  ConfigDatabase db;
  guint i;
  gint tries = 0;

  g_return_val_if_fail(conf != NULL, NULL);
  g_return_val_if_fail(dir != NULL, NULL);
  g_return_val_if_fail(err == NULL || *err == NULL, NULL);

  CHECK_OWNER_USE (conf);
  
  if (!mateconf_key_check(dir, err))
    return NULL;

  if (mateconf_engine_is_local(conf))
    {
      GError* error = NULL;
      GSList* retval;
      
      retval = mateconf_sources_all_dirs(conf->local_sources,
                                      dir,
                                      &error);
      
      if (error != NULL)
        {
          if (err)
            *err = error;
          else
            {
              g_error_free(error);
            }

          g_assert(retval == NULL);
          
          return NULL;
        }

      qualify_keys (retval, dir);
      
      return retval;
    }

  g_assert(!mateconf_engine_is_local(conf));
  
  CORBA_exception_init(&ev);
  
 RETRY:
  
  db = mateconf_engine_get_database (conf, TRUE, err);

  if (db == CORBA_OBJECT_NIL)
    {
      g_return_val_if_fail(((err == NULL) || (*err && ((*err)->code == MATECONF_ERROR_NO_SERVER))), NULL);

      return NULL;
    }
  
  ConfigDatabase_all_dirs(db,
                          (gchar*)dir, 
                          &keys,
                          &ev);

  if (mateconf_server_broken(&ev))
    {
      if (tries < MAX_RETRIES)
        {
          ++tries;
          CORBA_exception_free(&ev);
          mateconf_engine_detach (conf);
          goto RETRY;
        }
    }

  if (mateconf_handle_corba_exception(&ev, err))
    return NULL;
  
  i = 0;
  while (i < keys->_length)
    {
      gchar* s;

      s = mateconf_concat_dir_and_key (dir, keys->_buffer[i]);
      
      subdirs = g_slist_prepend(subdirs, s);
      
      ++i;
    }
  
  CORBA_free(keys);

  return subdirs;
}

/* annoyingly, this is REQUIRED for local sources */
void 
mateconf_engine_suggest_sync(MateConfEngine* conf, GError** err)
{
  CORBA_Environment ev;
  ConfigDatabase db;
  gint tries = 0;

  g_return_if_fail(conf != NULL);
  g_return_if_fail(err == NULL || *err == NULL);

  CHECK_OWNER_USE (conf);
  
  if (mateconf_engine_is_local(conf))
    {
      GError* error = NULL;
      
      mateconf_sources_sync_all(conf->local_sources,
                             &error);
      
      if (error != NULL)
        {
          if (err)
            *err = error;
          else
            {
              g_error_free(error);
            }
          return;
        }
      
      return;
    }

  g_assert(!mateconf_engine_is_local(conf));
  
  CORBA_exception_init(&ev);

 RETRY:
  
  db = mateconf_engine_get_database (conf, TRUE, err);

  if (db == CORBA_OBJECT_NIL)
    {
      g_return_if_fail(err == NULL || *err != NULL);

      return;
    }

  ConfigDatabase_sync(db, &ev);

  if (mateconf_server_broken(&ev))
    {
      if (tries < MAX_RETRIES)
        {
          ++tries;
          CORBA_exception_free(&ev);
          mateconf_engine_detach (conf);
          goto RETRY;
        }
    }
  
  if (mateconf_handle_corba_exception(&ev, err))  
    ; /* nothing additional */
}

void 
mateconf_clear_cache(MateConfEngine* conf, GError** err)
{
  CORBA_Environment ev;
  ConfigDatabase db;
  gint tries = 0;

  g_return_if_fail(conf != NULL);
  g_return_if_fail(err == NULL || *err == NULL);

  /* don't disallow non-owner use here since you can't do this
   * via MateConfClient API and calling this function won't break
   * MateConfClient anyway
   */
  
  if (mateconf_engine_is_local(conf))
    {
      GError* error = NULL;
      
      mateconf_sources_clear_cache(conf->local_sources);
      
      if (error != NULL)
        {
          if (err)
            *err = error;
          else
            {
              g_error_free(error);
            }
          return;
        }
      
      return;
    }

  g_assert(!mateconf_engine_is_local(conf));
  
  CORBA_exception_init(&ev);

 RETRY:
  
  db = mateconf_engine_get_database (conf, TRUE, err);

  if (db == CORBA_OBJECT_NIL)
    {
      g_return_if_fail(err == NULL || *err != NULL);

      return;
    }

  ConfigDatabase_clear_cache(db, &ev);

  if (mateconf_server_broken(&ev))
    {
      if (tries < MAX_RETRIES)
        {
          ++tries;
          CORBA_exception_free(&ev);
          mateconf_engine_detach (conf);
          goto RETRY;
        }
    }
  
  if (mateconf_handle_corba_exception(&ev, err))  
    ; /* nothing additional */
}

void 
mateconf_synchronous_sync(MateConfEngine* conf, GError** err)
{
  CORBA_Environment ev;
  ConfigDatabase db;
  gint tries = 0;

  g_return_if_fail(conf != NULL);
  g_return_if_fail(err == NULL || *err == NULL);

  if (mateconf_engine_is_local(conf))
    {
      GError* error = NULL;
      
      mateconf_sources_sync_all(conf->local_sources, &error);
      
      if (error != NULL)
        {
          if (err)
            *err = error;
          else
            {
              g_error_free(error);
            }
          return;
        }
      
      return;
    }

  g_assert(!mateconf_engine_is_local(conf));
  
  CORBA_exception_init(&ev);

 RETRY:
  
  db = mateconf_engine_get_database (conf, TRUE, err);

  if (db == CORBA_OBJECT_NIL)
    {
      g_return_if_fail(err == NULL || *err != NULL);

      return;
    }

  ConfigDatabase_synchronous_sync(db, &ev);

  if (mateconf_server_broken(&ev))
    {
      if (tries < MAX_RETRIES)
        {
          ++tries;
          CORBA_exception_free(&ev);
          mateconf_engine_detach (conf);
          goto RETRY;
        }
    }
  
  if (mateconf_handle_corba_exception(&ev, err))  
    ; /* nothing additional */
}

gboolean
mateconf_engine_dir_exists(MateConfEngine *conf, const gchar *dir, GError** err)
{
  CORBA_Environment ev;
  ConfigDatabase db;
  CORBA_boolean server_ret;
  gint tries = 0;

  g_return_val_if_fail(conf != NULL, FALSE);
  g_return_val_if_fail(dir != NULL, FALSE);
  g_return_val_if_fail(err == NULL || *err == NULL, FALSE);

  CHECK_OWNER_USE (conf);
  
  if (!mateconf_key_check(dir, err))
    return FALSE;
  
  if (mateconf_engine_is_local(conf))
    {
      return mateconf_sources_dir_exists(conf->local_sources,
                                      dir,
                                      err);
    }

  g_assert(!mateconf_engine_is_local(conf));
  
  CORBA_exception_init(&ev);
  
 RETRY:
  
  db = mateconf_engine_get_database(conf, TRUE, err);
  
  if (db == CORBA_OBJECT_NIL)
    {
      g_return_val_if_fail(err == NULL || *err != NULL, FALSE);

      return FALSE;
    }
  
  server_ret = ConfigDatabase_dir_exists(db,
                                         (gchar*)dir,
                                         &ev);
  
  if (mateconf_server_broken(&ev))
    {
      if (tries < MAX_RETRIES)
        {
          ++tries;
          CORBA_exception_free(&ev);
          mateconf_engine_detach (conf);
          goto RETRY;
        }
    }
  
  if (mateconf_handle_corba_exception(&ev, err))  
    ; /* nothing */

  return (server_ret == CORBA_TRUE);
}

void
mateconf_engine_remove_dir (MateConfEngine* conf,
                         const gchar* dir,
                         GError** err)
{
  CORBA_Environment ev;
  ConfigDatabase db;
  gint tries = 0;

  g_return_if_fail(conf != NULL);
  g_return_if_fail(dir != NULL);
  g_return_if_fail(err == NULL || *err == NULL);

  /* FIXME we have no MateConfClient method for doing this */
  /*   CHECK_OWNER_USE (conf); */
  
  if (!mateconf_key_check(dir, err))
    return;

  if (mateconf_engine_is_local(conf))
    {
      mateconf_sources_remove_dir(conf->local_sources, dir, err);
      return;
    }

  CORBA_exception_init(&ev);
  
 RETRY:
  
  db = mateconf_engine_get_database (conf, TRUE, err);

  if (db == CORBA_OBJECT_NIL)
    {
      g_return_if_fail(err == NULL || *err != NULL);
      return;
    }
  
  ConfigDatabase_remove_dir(db, (gchar*)dir, &ev);

  if (mateconf_server_broken(&ev))
    {
      if (tries < MAX_RETRIES)
        {
          ++tries;
          CORBA_exception_free(&ev);
          mateconf_engine_detach (conf);
          goto RETRY;
        }
    }
  mateconf_handle_corba_exception(&ev, err);
  
  return;
}

gboolean
mateconf_engine_key_is_writable  (MateConfEngine *conf,
                               const gchar *key,
                               GError     **err)
{
  gboolean is_writable = TRUE;
  MateConfValue *val;

  CHECK_OWNER_USE (conf);
  
  /* FIXME implement IDL to allow getting only writability
   * (not that urgent since MateConfClient caches this crap
   * anyway)
   */
  
  val = mateconf_engine_get_full(conf, key, NULL, TRUE,
                              NULL, &is_writable, err);

  mateconf_value_free (val);
  
  return is_writable;
}

/*
 * Connection maintenance
 */

static MateConfCnxn* 
mateconf_cnxn_new(MateConfEngine* conf,
               const gchar* namespace_section,
               CORBA_unsigned_long server_id,
               MateConfNotifyFunc func,
               gpointer user_data)
{
  MateConfCnxn* cnxn;
  static guint next_id = 1;
  
  cnxn = g_new0(MateConfCnxn, 1);

  cnxn->namespace_section = g_strdup(namespace_section);
  cnxn->conf = conf;
  cnxn->server_id = server_id;
  cnxn->client_id = next_id;
  cnxn->func = func;
  cnxn->user_data = user_data;

  ++next_id;

  return cnxn;
}

static void      
mateconf_cnxn_destroy(MateConfCnxn* cnxn)
{
  g_free(cnxn->namespace_section);
  g_free(cnxn);
}

static void       
mateconf_cnxn_notify(MateConfCnxn* cnxn,
                  MateConfEntry *entry)
{
  (*cnxn->func)(cnxn->conf, cnxn->client_id,
                entry,
                cnxn->user_data);
}

/*
 *  CORBA glue
 */

static ConfigServer   server = CORBA_OBJECT_NIL;

/* errors in here should be MATECONF_ERROR_NO_SERVER */
static ConfigServer
try_to_contact_server (gboolean start_if_not_found,
                       GError **err)
{
  CORBA_Environment ev;
  
  /* Try to launch server */      
  server = mateconf_activate_server (start_if_not_found,
                                  err);
    
  /* Try to ping server, by adding ourselves as a client */
  CORBA_exception_init (&ev);   

  if (!CORBA_Object_is_nil (server, &ev))
    {
      ConfigServer_add_client (server,
                               mateconf_get_config_listener (),
                               &ev);
      
      if (ev._major != CORBA_NO_EXCEPTION)
	{
          g_set_error (err,
                       MATECONF_ERROR,
                       MATECONF_ERROR_NO_SERVER,
                       _("Adding client to server's list failed, CORBA error: %s"),
                       CORBA_exception_id (&ev));

	  CORBA_Object_release (server, &ev);
	  server = CORBA_OBJECT_NIL;
          CORBA_exception_free(&ev);
	}
    }

#ifdef MATECONF_ENABLE_DEBUG      
  if (server == CORBA_OBJECT_NIL && start_if_not_found)
    g_return_val_if_fail (err == NULL || *err != NULL, server);
#endif
  
  return server;
}

/* All errors set in here should be MATECONF_ERROR_NO_SERVER; should
   only set errors if start_if_not_found is TRUE */
static ConfigServer
mateconf_get_config_server(gboolean start_if_not_found, GError** err)
{
  g_return_val_if_fail(err == NULL || *err == NULL, server);
  
  if (server != CORBA_OBJECT_NIL)
    return server;

  server = try_to_contact_server(start_if_not_found, err);
  
  return server; /* return what we have, NIL or not */
}

ConfigListener listener = CORBA_OBJECT_NIL;

void
mateconf_detach_config_server(void)
{  
  CORBA_Environment ev;

  CORBA_exception_init(&ev);

  if (listener != CORBA_OBJECT_NIL)
    {
      CORBA_Object_release(listener, &ev);
      listener = CORBA_OBJECT_NIL;
    }

  if (server != CORBA_OBJECT_NIL)
    {
      CORBA_Object_release(server, &ev);

      if (ev._major != CORBA_NO_EXCEPTION)
        {
          g_warning("Exception releasing mateconfd server object: %s",
                    CORBA_exception_id(&ev));
        }

      server = CORBA_OBJECT_NIL;
    }

  CORBA_exception_free(&ev);

  if (engines_by_db != NULL)
    {
      g_hash_table_destroy (engines_by_db);
      engines_by_db = NULL;
    }
}

/**
 * mateconf_debug_shutdown:
 * @void: 
 * 
 * Detach from the config server and release
 * all related resources.
 *
 * Returns: 1 if an exception occurs, 0 otherwise.
 **/
int
mateconf_debug_shutdown (void)
{
  mateconf_detach_config_server ();

  return mateconf_orb_release ();
}

static void notify                  (PortableServer_Servant     servant,
                                     ConfigDatabase             db,
                                     CORBA_unsigned_long        cnxn,
                                     const CORBA_char          *key,
                                     const ConfigValue         *value,
                                     CORBA_boolean              is_default,
                                     CORBA_boolean              is_writable,
                                     CORBA_Environment         *ev);
static void ping                    (PortableServer_Servant     _servant,
                                     CORBA_Environment         *ev);
static void update_listener         (PortableServer_Servant     _servant,
                                     ConfigDatabase             db,
                                     const CORBA_char          *address,
                                     const CORBA_unsigned_long  old_cnxn,
                                     const CORBA_char          *key,
                                     const CORBA_unsigned_long  new_cnxn,
                                     CORBA_Environment         *ev);
static void invalidate_cached_values(PortableServer_Servant     _servant,
                                     ConfigDatabase             database,
                                     const ConfigListener_KeyList *keys,
                                     CORBA_Environment         *ev);
static void drop_all_caches         (PortableServer_Servant     _servant,
                                     CORBA_Environment         *ev);



static PortableServer_ServantBase__epv base_epv = {
  NULL,
  NULL,
  NULL
};

static POA_ConfigListener__epv listener_epv = {
  NULL,
  notify,
  ping,
  update_listener,
  invalidate_cached_values,
  drop_all_caches
};

static POA_ConfigListener__vepv poa_listener_vepv = { &base_epv, &listener_epv };
static POA_ConfigListener poa_listener_servant = { NULL, &poa_listener_vepv };

static void 
notify(PortableServer_Servant servant,
       ConfigDatabase db,
       CORBA_unsigned_long server_id,
       const CORBA_char* key,
       const ConfigValue* value,
       CORBA_boolean is_default,
       CORBA_boolean is_writable,
       CORBA_Environment *ev)
{
  MateConfCnxn* cnxn;
  MateConfValue* gvalue;
  MateConfEngine* conf;
  MateConfEntry* entry;
  
  conf = lookup_engine_by_database (db);

  if (conf == NULL)
    {
#ifdef MATECONF_ENABLE_DEBUG
      g_warning ("Client received notify for unknown database object");
#endif
      return;
    }
  
  cnxn = ctable_lookup_by_server_id(conf->ctable, server_id);
  
  if (cnxn == NULL)
    {
#ifdef MATECONF_ENABLE_DEBUG
      g_warning("Client received notify for unknown connection ID %u",
                (guint)server_id);
#endif
      return;
    }

  gvalue = mateconf_value_from_corba_value(value);

  entry = mateconf_entry_new_nocopy (g_strdup (key),
                                  gvalue);
  mateconf_entry_set_is_default (entry, is_default);
  mateconf_entry_set_is_writable (entry, is_writable);
  
  mateconf_cnxn_notify(cnxn, entry);

  mateconf_entry_free (entry);
}

static void
ping (PortableServer_Servant _servant, CORBA_Environment * ev)
{
  /* This one is easy :-) */
  
  return;
}

static void
update_listener (PortableServer_Servant _servant,
                 ConfigDatabase             db,
                 const CORBA_char          *address,
                 const CORBA_unsigned_long  old_cnxn_id,
                 const CORBA_char          *key,
                 const CORBA_unsigned_long  new_cnxn_id,
                 CORBA_Environment         *ev_ignored)
{
  MateConfCnxn* cnxn;
  MateConfEngine* conf;
  CORBA_Environment ev;
  
  conf = lookup_engine_by_database (db);

  /* See if we have an old engine with a now-invalid object
     reference, and update its reference. */
  if (conf == NULL)
    {
      CORBA_exception_init (&ev);
      
      if (strcmp (address, "def") == 0)
        conf = default_engine;
      else
        {
          GSList  *addresses;

          addresses = mateconf_persistent_name_get_address_list (address);
    
          conf = lookup_engine (addresses);
    
          mateconf_address_list_free (addresses);
        }

      if (conf)
        mateconf_engine_set_database (conf,
                                   CORBA_Object_duplicate (db, &ev));
    }
  
  if (conf == NULL)
    {
#ifdef MATECONF_ENABLE_DEBUG
      g_warning("Client received listener update for unknown database "
                "(this is not a big deal, this warning only appears if MateConf is compiled with debugging)");
#endif
      return;
    }
  
  cnxn = ctable_lookup_by_server_id (conf->ctable, old_cnxn_id);
  
  if (cnxn == NULL)
    {
#ifdef MATECONF_ENABLE_DEBUG
      g_warning("Client received listener update for unknown listener ID %u "
                "(this is not a big deal, this warning only appears if MateConf is compiled with debugging)",
                (guint)old_cnxn_id);
#endif
      return;
    }
  
  ctable_reinstall (conf->ctable, cnxn, old_cnxn_id, new_cnxn_id);
}

static void
invalidate_cached_values (PortableServer_Servant     _servant,
                          ConfigDatabase             database,
                          const ConfigListener_KeyList *keys,
                          CORBA_Environment         *ev)
{
#if 0
  g_warning ("FIXME process %d received request to invalidate some cached MateConf values from the server, but right now we don't know how to do that (not implemented).", (int) getpid());
#endif
}

static void
drop_all_caches (PortableServer_Servant     _servant,
                 CORBA_Environment         *ev)
{
#if 0
  g_warning ("FIXME process %d received request to invalidate all cached MateConf values from the server, but right now we don't know how to do that (not implemented).", (int) getpid());
#endif
}

static ConfigListener 
mateconf_get_config_listener(void)
{  
  if (listener == CORBA_OBJECT_NIL)
    {
      CORBA_Environment ev;
      PortableServer_POA poa;
      PortableServer_POAManager poa_mgr;

      CORBA_exception_init (&ev);
      POA_ConfigListener__init (&poa_listener_servant, &ev);
      
      g_assert (ev._major == CORBA_NO_EXCEPTION);

      poa =
        (PortableServer_POA) CORBA_ORB_resolve_initial_references (mateconf_orb_get (),
                                                                   "RootPOA", &ev);

      g_assert (ev._major == CORBA_NO_EXCEPTION);

      poa_mgr = PortableServer_POA__get_the_POAManager (poa, &ev);
      PortableServer_POAManager_activate (poa_mgr, &ev);

      g_assert (ev._major == CORBA_NO_EXCEPTION);

      listener = PortableServer_POA_servant_to_reference(poa,
                                                         &poa_listener_servant,
                                                         &ev);

      CORBA_Object_release ((CORBA_Object) poa_mgr, &ev);
      CORBA_Object_release ((CORBA_Object) poa, &ev);

      g_assert (listener != CORBA_OBJECT_NIL);
      g_assert (ev._major == CORBA_NO_EXCEPTION);
    }
  
  return listener;
}
#endif /* HAVE_CORBA */

void
mateconf_preinit (gpointer app, gpointer mod_info)
{
  /* Deprecated */
}

void
mateconf_postinit (gpointer app, gpointer mod_info)
{
  /* Deprecated */
}

/* All deprecated */
const char mateconf_version[] = VERSION;

struct poptOption mateconf_options[] = {
  {NULL}
};

/* Also deprecated */
gboolean     
mateconf_init (int argc, char **argv, GError** err)
{
  
  return TRUE;
}

gboolean
mateconf_is_initialized (void)
{
  return TRUE;
}

/* 
 * Ampersand and <> are not allowed due to the XML backend; shell
 * special characters aren't allowed; others are just in case we need
 * some magic characters someday.  hyphen, underscore, period, colon
 * are allowed as separators. % disallowed to avoid printf confusion.
 */

/* Key/dir validity is exactly the same, except that '/' must be a dir, 
   but we are sort of ignoring that for now. */

/* Also, keys can contain only ASCII */

static const gchar invalid_chars[] = " \t\r\n\"$&<>,+=#!()'|{}[]?~`;%\\";

gboolean     
mateconf_valid_key      (const gchar* key, gchar** why_invalid)
{
  const gchar* s = key;
  gboolean just_saw_slash = FALSE;

  /* Key must start with the root */
  if (*key != '/')
    {
      if (why_invalid != NULL)
        *why_invalid = g_strdup(_("Must begin with a slash (/)"));
      return FALSE;
    }
  
  /* Root key is a valid dir */
  if (*key == '/' && key[1] == '\0')
    return TRUE;

  while (*s)
    {
      if (just_saw_slash)
        {
          /* Can't have two slashes in a row, since it would mean
           * an empty spot.
           * Can't have a period right after a slash,
           * because it would be a pain for filesystem-based backends.
           */
          if (*s == '/' || *s == '.')
            {
              if (why_invalid != NULL)
                {
                  if (*s == '/')
                    *why_invalid = g_strdup(_("Can't have two slashes (/) in a row"));
                  else
                    *why_invalid = g_strdup(_("Can't have a period (.) right after a slash (/)"));
                }
              return FALSE;
            }
        }

      if (*s == '/')
        {
          just_saw_slash = TRUE;
        }
      else
        {
          const gchar* inv = invalid_chars;

          just_saw_slash = FALSE;
          
          if (((unsigned char)*s) > 127)
            {
              if (why_invalid != NULL)
                *why_invalid = g_strdup_printf (_("'%c' is not an ASCII character, so isn't allowed in key names"),
                                                *s);
              return FALSE;
            }
          
          while (*inv)
            {
              if (*inv == *s)
                {
                  if (why_invalid != NULL)
                    *why_invalid = g_strdup_printf(_("`%c' is an invalid character in key/directory names"), *s);
                  return FALSE;
                }
              ++inv;
            }
        }

      ++s;
    }

  /* Can't end with slash */
  if (just_saw_slash)
    {
      if (why_invalid != NULL)
        *why_invalid = g_strdup(_("Key/directory may not end with a slash (/)"));
      return FALSE;
    }
  else
    return TRUE;
}

/**
 * mateconf_escape_key:
 * @arbitrary_text: some text in any encoding or format
 * @len: length of @arbitrary_text in bytes, or -1 if @arbitrary_text is nul-terminated
 * 
 * Escape @arbitrary_text such that it's a valid key element (i.e. one
 * part of the key path). The escaped key won't pass mateconf_valid_key()
 * because it isn't a whole key (i.e. it doesn't have a preceding
 * slash), but prepending a slash to the escaped text should always
 * result in a valid key.
 * 
 * Return value: a nul-terminated valid MateConf key
 **/
char*
mateconf_escape_key (const char *arbitrary_text,
                  int         len)
{
  const char *p;
  const char *end;
  GString *retval;

  g_return_val_if_fail (arbitrary_text != NULL, NULL);
  
  /* Nearly all characters we would normally use for escaping aren't allowed in key
   * names, so we use @ for that.
   *
   * Invalid chars and @ itself are escaped as @xxx@ where xxx is the
   * Latin-1 value in decimal
   */

  if (len < 0)
    len = strlen (arbitrary_text);

  retval = g_string_sized_new (len);

  p = arbitrary_text;
  end = arbitrary_text + len;
  while (p != end)
    {
      if (*p == '/' || *p == '.' || *p == '@' || ((guchar) *p) > 127 ||
          strchr (invalid_chars, *p))
        {
          g_string_append_printf (retval, "@%u@", (guchar) *p);
        }
      else
        g_string_append_c (retval, *p);
      
      ++p;
    }

  return g_string_free (retval, FALSE);
}

/**
 * mateconf_unescape_key:
 * @escaped_key: a key created with mateconf_escape_key()
 * @len: length of @escaped_key in bytes, or -1 if @escaped_key is nul-terminated
 * 
 * Converts a string escaped with mateconf_escape_key() back into its original
 * form.
 * 
 * Return value: the original string that was escaped to create @escaped_key
 **/
char*
mateconf_unescape_key (const char *escaped_key,
                    int         len)
{
  const char *p;
  const char *end;
  const char *start_seq;
  GString *retval;

  g_return_val_if_fail (escaped_key != NULL, NULL);
  
  if (len < 0)
    len = strlen (escaped_key);

  retval = g_string_new (NULL);

  p = escaped_key;
  end = escaped_key + len;
  start_seq = NULL;
  while (p != end)
    {
      if (start_seq)
        {
          if (*p == '@')
            {
              /* *p is the @ that ends a seq */
              char *end_seq;
              guchar val;
              
              val = strtoul (start_seq, &end_seq, 10);
              if (start_seq != end_seq)
                g_string_append_c (retval, val);
              
              start_seq = NULL;
            }
        }
      else
        {
          if (*p == '@')
            start_seq = p + 1;
          else
            g_string_append_c (retval, *p);
        }

      ++p;
    }

  return g_string_free (retval, FALSE);
}


gboolean
mateconf_key_is_below   (const gchar* above, const gchar* below)
{
  int len;

  if (above[0] == '/' && above[1] == '\0')
    return TRUE;
  
  len = strlen (above);
  if (strncmp (below, above, len) == 0)
    {
      /* only if this is a complete key component,
       * so that /foo is not above /foofoo/bar */
      if (below[len] == '\0' || below[len] == '/')
        return TRUE;
      else
	return FALSE;
    }
  else
    return FALSE;
}

gchar*
mateconf_unique_key (void)
{
  /* This function is hardly cryptographically random but should be
     "good enough" */
  
  static guint serial = 0;
  gchar* key;
  guint t, ut, p, u, r;
  GTimeVal tv;
  
  g_get_current_time(&tv);
  
  t = tv.tv_sec;
  ut = tv.tv_usec;

  p = getpid();
  
#ifdef HAVE_GETUID
  u = getuid();
#else
  u = 0;
#endif

  /* don't bother to seed; if it's based on the time or any other
     changing info we can get, we may as well just use that changing
     info. since we don't seed we'll at least get a different number
     on every call to this function in the same executable. */
  r = rand();
  
  /* The letters may increase uniqueness by preventing "melds"
     i.e. 01t01k01 and 0101t0k1 are not the same */
  key = g_strdup_printf("%ut%uut%uu%up%ur%uk%u",
                        /* Duplicate keys must be generated
                           by two different program instances */
                        serial,
                        /* Duplicate keys must be generated
                           in the same microsecond */
                        t,
                        ut,
                        /* Duplicate keys must be generated by
                           the same user */
                        u,
                        /* Duplicate keys must be generated by
                           two programs that got the same PID */
                        p,
                        /* Duplicate keys must be generated with the
                           same random seed and the same index into
                           the series of pseudorandom values */
                        r,
                        /* Duplicate keys must result from running
                           this function at the same stack location */
                        GPOINTER_TO_UINT(&key));

  ++serial;
  
  return key;
}

#ifdef HAVE_CORBA
/*
 * Table of connections 
 */ 

static gint
corba_unsigned_long_equal (gconstpointer v1,
                           gconstpointer v2)
{
  return *((const CORBA_unsigned_long*) v1) == *((const CORBA_unsigned_long*) v2);
}

static guint
corba_unsigned_long_hash (gconstpointer v)
{
  /* for our purposes we can just assume 32 bits are significant */
  return (guint)(*(const CORBA_unsigned_long*) v);
}

static CnxnTable* 
ctable_new(void)
{
  CnxnTable* ct;

  ct = g_new(CnxnTable, 1);

  ct->server_ids = g_hash_table_new (corba_unsigned_long_hash,
                                     corba_unsigned_long_equal);  
  ct->client_ids = g_hash_table_new (g_int_hash, g_int_equal);
  
  return ct;
}

static void
ctable_destroy(CnxnTable* ct)
{
  g_hash_table_destroy (ct->server_ids);
  g_hash_table_destroy (ct->client_ids);
  g_free(ct);
}

static void       
ctable_insert(CnxnTable* ct, MateConfCnxn* cnxn)
{
  g_hash_table_insert (ct->server_ids, &cnxn->server_id, cnxn);
  g_hash_table_insert (ct->client_ids, &cnxn->client_id, cnxn);
}

static void       
ctable_remove(CnxnTable* ct, MateConfCnxn* cnxn)
{
  g_hash_table_remove (ct->server_ids, &cnxn->server_id);
  g_hash_table_remove (ct->client_ids, &cnxn->client_id);
}

struct RemoveData {
  GSList* removed;
  MateConfEngine* conf;
  gboolean save_removed;
};

static gboolean
remove_by_conf(gpointer key, gpointer value, gpointer user_data)
{
  struct RemoveData* rd = user_data;
  MateConfCnxn* cnxn = value;
  
  if (cnxn->conf == rd->conf)
    {
      if (rd->save_removed)
        rd->removed = g_slist_prepend(rd->removed, cnxn);

      return TRUE;  /* remove this one */
    }
  else 
    return FALSE; /* or not */
}

/* FIXME this no longer makes any sense, because a CnxnTable
   belongs to a MateConfEngine and all entries have the same
   MateConfEngine.
*/

/* We return a list of the removed MateConfCnxn */
static GSList*      
ctable_remove_by_conf(CnxnTable* ct, MateConfEngine* conf)
{
  guint client_ids_removed;
  guint server_ids_removed;
  struct RemoveData rd;

  rd.removed = NULL;
  rd.conf = conf;
  rd.save_removed = TRUE;
  
  client_ids_removed = g_hash_table_foreach_remove (ct->server_ids,
                                                    remove_by_conf,
                                                    &rd);

  rd.save_removed = FALSE;

  server_ids_removed = g_hash_table_foreach_remove(ct->client_ids,
                                                   remove_by_conf,
                                                   &rd);

  g_assert(client_ids_removed == server_ids_removed);
  g_assert(client_ids_removed == g_slist_length(rd.removed));

  return rd.removed;
}

static MateConfCnxn* 
ctable_lookup_by_client_id(CnxnTable* ct, guint client_id)
{
  return g_hash_table_lookup(ct->client_ids, &client_id);
}

static MateConfCnxn* 
ctable_lookup_by_server_id(CnxnTable* ct, CORBA_unsigned_long server_id)
{
  return g_hash_table_lookup (ct->server_ids, &server_id);
}

static void
ctable_reinstall (CnxnTable* ct,
                  MateConfCnxn *cnxn,
                  guint old_server_id,
                  guint new_server_id)
{
  g_return_if_fail (cnxn->server_id == old_server_id);

  g_hash_table_remove (ct->server_ids, &old_server_id);
  
  cnxn->server_id = new_server_id;

  g_hash_table_insert (ct->server_ids, &cnxn->server_id, cnxn);
}

/*
 * Daemon control
 */

void          
mateconf_shutdown_daemon (GError** err)
{
  CORBA_Environment ev;
  ConfigServer cs;

  cs = mateconf_get_config_server (FALSE, err); /* Don't want to spawn it if it's already down */

  if (err && *err && (*err)->code == MATECONF_ERROR_NO_SERVER)
    {
      /* No server is hardly an error here */
      g_error_free (*err);
      *err = NULL;
    }
  
  if (cs == CORBA_OBJECT_NIL)
    {      
      
      return;
    }

  CORBA_exception_init (&ev);

  ConfigServer_shutdown (cs, &ev);

  if (ev._major != CORBA_NO_EXCEPTION)
    {
      if (err)
        *err = mateconf_error_new (MATECONF_ERROR_FAILED, _("Failure shutting down config server: %s"),
                                CORBA_exception_id (&ev));

      CORBA_exception_free(&ev);
    }
}

gboolean
mateconf_ping_daemon(void)
{
  ConfigServer cs;
  
  cs = mateconf_get_config_server(FALSE, NULL); /* ignore error, since whole point is to see if server is reachable */

  if (cs == CORBA_OBJECT_NIL)
    return FALSE;
  else
    return TRUE;
}

gboolean
mateconf_spawn_daemon(GError** err)
{
  ConfigServer cs;

  cs = mateconf_get_config_server(TRUE, err);

  if (cs == CORBA_OBJECT_NIL)
    {
      g_return_val_if_fail(err == NULL || *err != NULL, FALSE);
      return FALSE; /* Failed to spawn, error should be set */
    }
  else
    return TRUE;
}
#endif /* HAVE_CORBA */

/*
 * Sugar functions 
 */

gdouble      
mateconf_engine_get_float (MateConfEngine* conf, const gchar* key,
                 GError** err)
{
  MateConfValue* val;
  static const gdouble deflt = 0.0;
  
  g_return_val_if_fail(conf != NULL, 0.0);
  g_return_val_if_fail(key != NULL, 0.0);
  
  val = mateconf_engine_get (conf, key, err);

  if (val == NULL)
    return deflt;
  else
    {
      gdouble retval;
      
      if (val->type != MATECONF_VALUE_FLOAT)
        {
          if (err)
            *err = mateconf_error_new(MATECONF_ERROR_TYPE_MISMATCH, _("Expected float, got %s"),
                                    mateconf_value_type_to_string(val->type));
          mateconf_value_free(val);
          return deflt;
        }

      retval = mateconf_value_get_float(val);

      mateconf_value_free(val);

      return retval;
    }
}

gint         
mateconf_engine_get_int   (MateConfEngine* conf, const gchar* key,
                 GError** err)
{
  MateConfValue* val;
  static const gint deflt = 0;
  
  g_return_val_if_fail(conf != NULL, 0);
  g_return_val_if_fail(key != NULL, 0);
  
  val = mateconf_engine_get (conf, key, err);

  if (val == NULL)
    return deflt;
  else
    {
      gint retval;

      if (val->type != MATECONF_VALUE_INT)
        {
          if (err)
            *err = mateconf_error_new(MATECONF_ERROR_TYPE_MISMATCH, _("Expected int, got %s"),
                                    mateconf_value_type_to_string(val->type));
          mateconf_value_free(val);
          return deflt;
        }

      retval = mateconf_value_get_int(val);

      mateconf_value_free(val);

      return retval;
    }
}

gchar*       
mateconf_engine_get_string(MateConfEngine* conf, const gchar* key,
                 GError** err)
{
  MateConfValue* val;
  static const gchar* deflt = NULL;
  
  g_return_val_if_fail(conf != NULL, NULL);
  g_return_val_if_fail(key != NULL, NULL);
  
  val = mateconf_engine_get (conf, key, err);

  if (val == NULL)
    return deflt ? g_strdup(deflt) : NULL;
  else
    {
      gchar* retval;

      if (val->type != MATECONF_VALUE_STRING)
        {
          if (err)
            *err = mateconf_error_new(MATECONF_ERROR_TYPE_MISMATCH, _("Expected string, got %s"),
                                    mateconf_value_type_to_string(val->type));
          mateconf_value_free(val);
          return deflt ? g_strdup(deflt) : NULL;
        }

      retval = mateconf_value_steal_string (val);
      mateconf_value_free (val);

      return retval;
    }
}

gboolean     
mateconf_engine_get_bool  (MateConfEngine* conf, const gchar* key,
                        GError** err)
{
  MateConfValue* val;
  static const gboolean deflt = FALSE;
  
  g_return_val_if_fail(conf != NULL, FALSE);
  g_return_val_if_fail(key != NULL, FALSE);
  
  val = mateconf_engine_get (conf, key, err);

  if (val == NULL)
    return deflt;
  else
    {
      gboolean retval;

      if (val->type != MATECONF_VALUE_BOOL)
        {
          if (err)
            *err = mateconf_error_new(MATECONF_ERROR_TYPE_MISMATCH, _("Expected bool, got %s"),
                                    mateconf_value_type_to_string(val->type));
          mateconf_value_free(val);
          return deflt;
        }

      retval = mateconf_value_get_bool(val);

      mateconf_value_free(val);

      return retval;
    }
}

MateConfSchema* 
mateconf_engine_get_schema  (MateConfEngine* conf, const gchar* key, GError** err)
{
  MateConfValue* val;

  g_return_val_if_fail(conf != NULL, NULL);
  g_return_val_if_fail(key != NULL, NULL);
  
  val = mateconf_engine_get_with_locale(conf, key, mateconf_current_locale(), err);

  if (val == NULL)
    return NULL;
  else
    {
      MateConfSchema* retval;

      if (val->type != MATECONF_VALUE_SCHEMA)
        {
          if (err)
            *err = mateconf_error_new(MATECONF_ERROR_TYPE_MISMATCH, _("Expected schema, got %s"),
                                    mateconf_value_type_to_string(val->type));
          mateconf_value_free(val);
          return NULL;
        }

      retval = mateconf_value_steal_schema (val);
      mateconf_value_free (val);

      return retval;
    }
}

GSList*
mateconf_engine_get_list    (MateConfEngine* conf, const gchar* key,
                          MateConfValueType list_type, GError** err)
{
  MateConfValue* val;

  g_return_val_if_fail(conf != NULL, NULL);
  g_return_val_if_fail(key != NULL, NULL);
  g_return_val_if_fail(list_type != MATECONF_VALUE_INVALID, NULL);
  g_return_val_if_fail(list_type != MATECONF_VALUE_LIST, NULL);
  g_return_val_if_fail(list_type != MATECONF_VALUE_PAIR, NULL);
  g_return_val_if_fail(err == NULL || *err == NULL, NULL);
  
  val = mateconf_engine_get_with_locale(conf, key, mateconf_current_locale(), err);

  if (val == NULL)
    return NULL;
  else
    {
      /* This type-checks the value */
      return mateconf_value_list_to_primitive_list_destructive(val, list_type, err);
    }
}

gboolean
mateconf_engine_get_pair    (MateConfEngine* conf, const gchar* key,
                   MateConfValueType car_type, MateConfValueType cdr_type,
                   gpointer car_retloc, gpointer cdr_retloc,
                   GError** err)
{
  MateConfValue* val;
  GError* error = NULL;
  
  g_return_val_if_fail(conf != NULL, FALSE);
  g_return_val_if_fail(key != NULL, FALSE);
  g_return_val_if_fail(car_type != MATECONF_VALUE_INVALID, FALSE);
  g_return_val_if_fail(car_type != MATECONF_VALUE_LIST, FALSE);
  g_return_val_if_fail(car_type != MATECONF_VALUE_PAIR, FALSE);
  g_return_val_if_fail(cdr_type != MATECONF_VALUE_INVALID, FALSE);
  g_return_val_if_fail(cdr_type != MATECONF_VALUE_LIST, FALSE);
  g_return_val_if_fail(cdr_type != MATECONF_VALUE_PAIR, FALSE);
  g_return_val_if_fail(car_retloc != NULL, FALSE);
  g_return_val_if_fail(cdr_retloc != NULL, FALSE);
  g_return_val_if_fail(err == NULL || *err == NULL, FALSE);  
  
  val = mateconf_engine_get_with_locale(conf, key, mateconf_current_locale(), &error);

  if (error != NULL)
    {
      g_assert(val == NULL);
      
      if (err)
        *err = error;
      else
        g_error_free(error);

      return FALSE;
    }
  
  if (val == NULL)
    {
      return TRUE;
    }
  else
    {
      /* Destroys val */
      return mateconf_value_pair_to_primitive_pair_destructive(val,
                                                            car_type, cdr_type,
                                                            car_retloc, cdr_retloc,
                                                            err);
    }
}

/*
 * Setters
 */

static gboolean
error_checked_set(MateConfEngine* conf, const gchar* key,
                  MateConfValue* gval, GError** err)
{
  GError* my_err = NULL;
  
  mateconf_engine_set (conf, key, gval, &my_err);

  mateconf_value_free(gval);
  
  if (my_err != NULL)
    {
      if (err)
        *err = my_err;
      else
        g_error_free(my_err);
      return FALSE;
    }
  else
    return TRUE;
}

gboolean
mateconf_engine_set_float   (MateConfEngine* conf, const gchar* key,
                          gdouble val, GError** err)
{
  MateConfValue* gval;

  g_return_val_if_fail(conf != NULL, FALSE);
  g_return_val_if_fail(key != NULL, FALSE);
  g_return_val_if_fail(err == NULL || *err == NULL, FALSE);
  
  gval = mateconf_value_new(MATECONF_VALUE_FLOAT);

  mateconf_value_set_float(gval, val);

  return error_checked_set(conf, key, gval, err);
}

gboolean
mateconf_engine_set_int     (MateConfEngine* conf, const gchar* key,
                          gint val, GError** err)
{
  MateConfValue* gval;

  g_return_val_if_fail(conf != NULL, FALSE);
  g_return_val_if_fail(key != NULL, FALSE);
  g_return_val_if_fail(err == NULL || *err == NULL, FALSE);
  
  gval = mateconf_value_new(MATECONF_VALUE_INT);

  mateconf_value_set_int(gval, val);

  return error_checked_set(conf, key, gval, err);
}

gboolean
mateconf_engine_set_string  (MateConfEngine* conf, const gchar* key,
                          const gchar* val, GError** err)
{
  MateConfValue* gval;

  g_return_val_if_fail (val != NULL, FALSE);
  g_return_val_if_fail (conf != NULL, FALSE);
  g_return_val_if_fail (key != NULL, FALSE);
  g_return_val_if_fail (err == NULL || *err == NULL, FALSE);
  
  g_return_val_if_fail (g_utf8_validate (val, -1, NULL), FALSE);
  
  gval = mateconf_value_new(MATECONF_VALUE_STRING);

  mateconf_value_set_string(gval, val);

  return error_checked_set(conf, key, gval, err);
}

gboolean
mateconf_engine_set_bool    (MateConfEngine* conf, const gchar* key,
                          gboolean val, GError** err)
{
  MateConfValue* gval;

  g_return_val_if_fail(conf != NULL, FALSE);
  g_return_val_if_fail(key != NULL, FALSE);
  g_return_val_if_fail(err == NULL || *err == NULL, FALSE);
  
  gval = mateconf_value_new(MATECONF_VALUE_BOOL);

  mateconf_value_set_bool(gval, !!val); /* canonicalize the bool */

  return error_checked_set(conf, key, gval, err);
}

gboolean
mateconf_engine_set_schema  (MateConfEngine* conf, const gchar* key,
                          const MateConfSchema* val, GError** err)
{
  MateConfValue* gval;

  g_return_val_if_fail(conf != NULL, FALSE);
  g_return_val_if_fail(key != NULL, FALSE);
  g_return_val_if_fail(val != NULL, FALSE);
  g_return_val_if_fail(err == NULL || *err == NULL, FALSE);
  
  gval = mateconf_value_new(MATECONF_VALUE_SCHEMA);

  mateconf_value_set_schema(gval, val);

  return error_checked_set(conf, key, gval, err);
}

gboolean
mateconf_engine_set_list    (MateConfEngine* conf, const gchar* key,
                          MateConfValueType list_type,
                          GSList* list,
                          GError** err)
{
  MateConfValue* value_list;
  GError *tmp_err = NULL;
  
  g_return_val_if_fail(conf != NULL, FALSE);
  g_return_val_if_fail(key != NULL, FALSE);
  g_return_val_if_fail(list_type != MATECONF_VALUE_INVALID, FALSE);
  g_return_val_if_fail(list_type != MATECONF_VALUE_LIST, FALSE);
  g_return_val_if_fail(list_type != MATECONF_VALUE_PAIR, FALSE);
  g_return_val_if_fail(err == NULL || *err == NULL, FALSE);

  value_list = mateconf_value_list_from_primitive_list(list_type, list, &tmp_err);

  if (tmp_err)
    {
      g_propagate_error (err, tmp_err);
      return FALSE;
    }
  
  /* destroys the value_list */
  
  return error_checked_set(conf, key, value_list, err);
}

gboolean
mateconf_engine_set_pair    (MateConfEngine* conf, const gchar* key,
                          MateConfValueType car_type, MateConfValueType cdr_type,
                          gconstpointer address_of_car,
                          gconstpointer address_of_cdr,
                          GError** err)
{
  MateConfValue* pair;
  GError *tmp_err = NULL;
  
  g_return_val_if_fail(conf != NULL, FALSE);
  g_return_val_if_fail(key != NULL, FALSE);
  g_return_val_if_fail(car_type != MATECONF_VALUE_INVALID, FALSE);
  g_return_val_if_fail(car_type != MATECONF_VALUE_LIST, FALSE);
  g_return_val_if_fail(car_type != MATECONF_VALUE_PAIR, FALSE);
  g_return_val_if_fail(cdr_type != MATECONF_VALUE_INVALID, FALSE);
  g_return_val_if_fail(cdr_type != MATECONF_VALUE_LIST, FALSE);
  g_return_val_if_fail(cdr_type != MATECONF_VALUE_PAIR, FALSE);
  g_return_val_if_fail(address_of_car != NULL, FALSE);
  g_return_val_if_fail(address_of_cdr != NULL, FALSE);
  g_return_val_if_fail(err == NULL || *err == NULL, FALSE);
  

  pair = mateconf_value_pair_from_primitive_pair(car_type, cdr_type,
                                              address_of_car, address_of_cdr,
                                              &tmp_err);

  if (tmp_err)
    {
      g_propagate_error (err, tmp_err);
      return FALSE;
    }  
  
  return error_checked_set(conf, key, pair, err);
}

#ifdef HAVE_CORBA
/* CORBA Util */

/* Set MateConfError from an exception, free exception, etc. */

static MateConfError
corba_errno_to_mateconf_errno(ConfigErrorType corba_err)
{
  switch (corba_err)
    {
    case ConfigFailed:
      return MATECONF_ERROR_FAILED;
    case ConfigNoPermission:
      return MATECONF_ERROR_NO_PERMISSION;
    case ConfigBadAddress:
      return MATECONF_ERROR_BAD_ADDRESS;
    case ConfigBadKey:
      return MATECONF_ERROR_BAD_KEY;
    case ConfigParseError:
      return MATECONF_ERROR_PARSE_ERROR;
    case ConfigCorrupt:
      return MATECONF_ERROR_CORRUPT;
    case ConfigTypeMismatch:
      return MATECONF_ERROR_TYPE_MISMATCH;
    case ConfigIsDir:
      return MATECONF_ERROR_IS_DIR;
    case ConfigIsKey:
      return MATECONF_ERROR_IS_KEY;
    case ConfigOverridden:
      return MATECONF_ERROR_OVERRIDDEN;
    case ConfigLockFailed:
      return MATECONF_ERROR_LOCK_FAILED;
    case ConfigNoWritableDatabase:
      return MATECONF_ERROR_NO_WRITABLE_DATABASE;
    case ConfigInShutdown:
      return MATECONF_ERROR_IN_SHUTDOWN;
    default:
      g_assert_not_reached();
      return MATECONF_ERROR_SUCCESS; /* warnings */
    }
}

static gboolean
mateconf_server_broken(CORBA_Environment* ev)
{
  switch (ev->_major)
    {
    case CORBA_SYSTEM_EXCEPTION:
      return TRUE;

    case CORBA_USER_EXCEPTION:
      {
        ConfigException* ce;

        ce = CORBA_exception_value(ev);

        return ce->err_no == ConfigInShutdown;
      }
      
    default:
      return FALSE;
    }
}

static gboolean
mateconf_handle_corba_exception(CORBA_Environment* ev, GError** err)
{
  switch (ev->_major)
    {
    case CORBA_NO_EXCEPTION:
      CORBA_exception_free (ev);
      return FALSE;
    case CORBA_SYSTEM_EXCEPTION:
      if (err)
        *err = mateconf_error_new (MATECONF_ERROR_NO_SERVER, _("CORBA error: %s"),
                                CORBA_exception_id (ev));
      CORBA_exception_free (ev);
      return TRUE;
    case CORBA_USER_EXCEPTION:
      {        
        ConfigException* ce;

        ce = CORBA_exception_value (ev);

        if (err)
          *err = mateconf_error_new (corba_errno_to_mateconf_errno (ce->err_no),
                                  "%s", ce->message);
        CORBA_exception_free (ev);
        return TRUE;
      }
    default:
      g_assert_not_reached();
      return TRUE;
    }
}
#endif

/*
 * Enumeration conversions
 */

gboolean
mateconf_string_to_enum (MateConfEnumStringPair lookup_table[],
                      const gchar* str,
                      gint* enum_value_retloc)
{
  int i = 0;
  
  while (lookup_table[i].str != NULL)
    {
      if (g_ascii_strcasecmp (lookup_table[i].str, str) == 0)
        {
          *enum_value_retloc = lookup_table[i].enum_value;
          return TRUE;
        }

      ++i;
    }

  return FALSE;
}

const gchar*
mateconf_enum_to_string (MateConfEnumStringPair lookup_table[],
                      gint enum_value)
{
  int i = 0;
  
  while (lookup_table[i].str != NULL)
    {
      if (lookup_table[i].enum_value == enum_value)
        return lookup_table[i].str;

      ++i;
    }

  return NULL;
}
