/* -*- mode: C; c-file-style: "gnu" -*- */

/* MateConf
 * Copyright (C) 1999, 2000 Red Hat Inc.
 * Copyright (C) 2003 Imendio AB
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

#include <config.h>

#include <popt.h>
#include "mateconf.h"
#include "mateconf-dbus-utils.h"
#include "mateconf-internals.h"
#include "mateconf-sources.h"
#include "mateconf-locale.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>

#define d(x) 

#define DAEMON_NAME_OWNER_CHANGED_RULE \
    "type='signal',member='NameOwnerChanged',arg0='org.mate.MateConf'"
#define NOTIFY_RULE \
    "type='method_call',interface='org.mate.MateConf.Database',member='Notify'"
#define DAEMON_DISCONNECTED_RULE \
    "type='signal',member='Disconnected'"

struct _MateConfEngine {
  guint refcount;

  gchar *database;

  GHashTable *notify_dirs;
  GHashTable *notify_ids;

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

typedef struct {
  gchar* namespace_section;
  guint client_id;

  MateConfEngine* conf;             /* Engine we're associated with */
  MateConfNotifyFunc func;
  gpointer user_data;
} MateConfCnxn;

typedef struct {
  GList *cnxns;     /* List of connections to be notified below the dir */
} CnxnsData;


static DBusConnection *global_conn = NULL;
static gboolean        service_running = FALSE;
static gboolean        needs_reconnect = FALSE;
static MateConfEngine    *default_engine = NULL;
static GHashTable     *engines_by_db = NULL;
static GHashTable     *engines_by_address = NULL;
static gboolean        dbus_disconnected = FALSE;

static gboolean     ensure_dbus_connection      (void);
static gboolean     ensure_service              (gboolean          start_if_not_found,
						 GError          **err);
static gboolean     ensure_database             (MateConfEngine      *conf,
						 gboolean          start_if_not_found,
						 GError          **err);

static void         mateconf_engine_detach         (MateConfEngine      *conf);
static void         mateconf_engine_set_database   (MateConfEngine      *conf,
						 const gchar      *db);
static const gchar *mateconf_engine_get_database   (MateConfEngine      *conf,
						 gboolean          start_if_not_found,
						 GError          **err);

static void         register_engine             (MateConfEngine      *conf);
static void         unregister_engine           (MateConfEngine      *conf);
static MateConfCnxn *  mateconf_cnxn_new              (MateConfEngine      *conf,
						 const gchar      *namespace_section,
						 MateConfNotifyFunc   func,
						 gpointer          user_data);
static void         mateconf_cnxn_destroy          (MateConfCnxn        *cnxn);
static void         mateconf_cnxn_notify           (MateConfCnxn        *cnxn,
						 MateConfEntry       *entry);
static MateConfCnxn *  mateconf_cnxn_lookup_id        (MateConfEngine      *conf,
						 guint             client_id);
static GList *      mateconf_cnxn_lookup_dir       (MateConfEngine      *conf,
						 const gchar      *dir);
static void         mateconf_cnxn_insert           (MateConfEngine      *conf,
						 const gchar      *dir,
						 guint              client_id,
						 MateConfCnxn        *cnxn);
static void         mateconf_cnxn_remove           (MateConfEngine      *conf,
						 MateConfCnxn        *cnxn);
static gboolean     send_notify_add             (MateConfEngine      *conf,
						 MateConfCnxn        *cnxn,
						 GError          **err);
static void         reinitialize_databases      (void);
static DBusHandlerResult
                    mateconf_dbus_message_filter   (DBusConnection   *dbus_conn,
						 DBusMessage      *message,
						 gpointer          user_data);
static MateConfEngine *lookup_engine_by_addresses  (GSList           *addresses);
static MateConfEngine *lookup_engine_by_database   (const gchar      *db);
static gboolean     mateconf_handle_dbus_exception (DBusMessage      *message,
						 DBusError        *derr,
						 GError          **gerr);
static void         mateconf_detach_config_server  (void);
static DBusHandlerResult
                    handle_notify               (DBusConnection   *connection,
						 DBusMessage      *message,
						 MateConfEngine      *conf);


#define CHECK_OWNER_USE(engine) \
  do { if ((engine)->owner && (engine)->owner_use_count == 0) \
      g_warning ("%s: You can't use a MateConfEngine that has an active " \
		 "MateConfClient wrapper object. Use MateConfClient API instead.", \
		 G_GNUC_FUNCTION); \
  } while (0)


static MateConfError
dbus_error_name_to_mateconf_errno (const char *name)
{
  int i;
  struct
  {
    const char *name;
    MateConfError error;
  } errors [] = {
    { MATECONF_DBUS_ERROR_FAILED, MATECONF_ERROR_FAILED },
    { MATECONF_DBUS_ERROR_NO_PERMISSION, MATECONF_ERROR_NO_PERMISSION },
    { MATECONF_DBUS_ERROR_BAD_ADDRESS, MATECONF_ERROR_BAD_ADDRESS },
    { MATECONF_DBUS_ERROR_BAD_KEY, MATECONF_ERROR_BAD_KEY },
    { MATECONF_DBUS_ERROR_PARSE_ERROR, MATECONF_ERROR_PARSE_ERROR },
    { MATECONF_DBUS_ERROR_CORRUPT, MATECONF_ERROR_CORRUPT },
    { MATECONF_DBUS_ERROR_TYPE_MISMATCH, MATECONF_ERROR_TYPE_MISMATCH },
    { MATECONF_DBUS_ERROR_IS_DIR, MATECONF_ERROR_IS_DIR },
    { MATECONF_DBUS_ERROR_IS_KEY, MATECONF_ERROR_IS_KEY },
    { MATECONF_DBUS_ERROR_OVERRIDDEN, MATECONF_ERROR_OVERRIDDEN },
    { MATECONF_DBUS_ERROR_LOCK_FAILED, MATECONF_ERROR_LOCK_FAILED },
    { MATECONF_DBUS_ERROR_NO_WRITABLE_DATABASE, MATECONF_ERROR_NO_WRITABLE_DATABASE },
    { MATECONF_DBUS_ERROR_IN_SHUTDOWN, MATECONF_ERROR_IN_SHUTDOWN },
  };

  for (i = 0; i < G_N_ELEMENTS (errors); i++)
    {
      if (strcmp (name, errors[i].name) == 0)
	return errors[i].error;
    }

  g_assert_not_reached ();
  
  return MATECONF_ERROR_SUCCESS;
}

/* If no error is detected, return FALSE with no side-effects. If an error is
 * detected, return TRUE, set gerr, unref message and free derr.
 */
static gboolean
mateconf_handle_dbus_exception (DBusMessage *message, DBusError *derr, GError **gerr)
{
  char *error_string;
  const char *name;

  if (message == NULL)
    {
      if (derr && dbus_error_is_set (derr))
	{
	  if (gerr)
	    {
	      *gerr = mateconf_error_new (MATECONF_ERROR_NO_SERVER, _("D-BUS error: %s"),
				       derr->message);
	    }
	}
      else 
	{
	  if (gerr)
	    *gerr = mateconf_error_new (MATECONF_ERROR_FAILED, _("Unknown error"));
	}

      if (derr)
	dbus_error_free (derr);

      return TRUE;
    }
    
  if (dbus_message_get_type (message) != DBUS_MESSAGE_TYPE_ERROR)
    return FALSE;

  if (derr)
    dbus_error_free (derr);

  name = dbus_message_get_member (message);

  dbus_message_get_args (message, NULL,
			 DBUS_TYPE_STRING, &error_string,
			 DBUS_TYPE_INVALID);
  
  if (g_str_has_prefix (name, "org.freedesktop.DBus.Error"))
    {
      if (gerr)
	*gerr = mateconf_error_new (MATECONF_ERROR_NO_SERVER, _("D-BUS error: %s"),
				error_string);
    }
  else if (g_str_has_prefix (name, "org.mate.MateConf.Error"))
    {
      if (gerr)
	{
	  MateConfError en;
	  
	  en = dbus_error_name_to_mateconf_errno (name);
	  *gerr = mateconf_error_new (en, error_string);
	}
    }
  else
    {
      if (gerr)
	*gerr = mateconf_error_new (MATECONF_ERROR_FAILED, _("Unknown error %s: %s"),
				 name, error_string);
    }

  dbus_message_unref (message);
  
  return TRUE;
}

static MateConfEngine*
mateconf_engine_blank (gboolean remote)
{
  MateConfEngine* conf;

  _mateconf_init_i18n ();
  
  conf = g_new0 (MateConfEngine, 1);

  conf->refcount = 1;
  
  conf->owner = NULL;
  conf->owner_use_count = 0;
  
  if (remote)
    {
      conf->database = NULL;

      conf->notify_dirs = g_hash_table_new_full (g_str_hash, g_str_equal,
						 g_free, NULL);

      conf->notify_ids = g_hash_table_new (NULL, NULL);
      
      conf->local_sources = NULL;
      conf->is_local = FALSE;
      conf->is_default = TRUE;
    }
  else
    {
      conf->database = NULL;
      conf->notify_ids = NULL;
      conf->notify_dirs = NULL;
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

static MateConfEngine *
lookup_engine_by_database (const gchar *db)
{
  if (engines_by_db)
    return g_hash_table_lookup (engines_by_db, db);
  else
    return NULL;
}

static void
database_hash_value_destroy (gpointer value)
{
  MateConfEngine *conf = value;

  g_free (conf->database);
  conf->database = NULL;
}

static void
mateconf_engine_set_database (MateConfEngine *conf,
                           const gchar *db)
{
  mateconf_engine_detach (conf);

  conf->database = g_strdup (db);

  if (engines_by_db == NULL)
    engines_by_db = g_hash_table_new_full (g_str_hash,
					   g_str_equal,
					   NULL,
					   database_hash_value_destroy);
  
  g_hash_table_insert (engines_by_db, conf->database, conf);  
}

static void
mateconf_engine_detach (MateConfEngine *conf)
{
  if (conf->database != NULL)
    {
      g_hash_table_remove (engines_by_db, conf->database);
    }
}

static gboolean
ensure_dbus_connection (void)
{
  DBusError error;

  if (global_conn != NULL)
    return TRUE;

  if (dbus_disconnected)
    {
      g_warning ("The connection to DBus was broken. Can't reinitialize it.");
      return FALSE;
    }

  dbus_error_init (&error);

#ifdef USE_SYSTEM_BUS
  global_conn = dbus_bus_get_private (DBUS_BUS_SYSTEM, &error);
#else
  global_conn = dbus_bus_get_private (DBUS_BUS_SESSION, &error);
#endif
  
  if (!global_conn) 
    {
      g_warning ("Client failed to connect to the D-BUS daemon:\n%s", error.message);
      
      dbus_error_free (&error);
      return FALSE;
    }
	
  dbus_connection_setup_with_g_main (global_conn, NULL);

  dbus_connection_set_exit_on_disconnect (global_conn, FALSE);

  dbus_bus_add_match (global_conn, DAEMON_NAME_OWNER_CHANGED_RULE, NULL);
  dbus_bus_add_match (global_conn, NOTIFY_RULE, NULL);
  dbus_bus_add_match (global_conn, DAEMON_DISCONNECTED_RULE, NULL);

  dbus_connection_add_filter (global_conn, mateconf_dbus_message_filter,
			      NULL, NULL);
  
  return TRUE;
}

static gboolean
ensure_service (gboolean  start_if_not_found,
		GError   **err)
{
  DBusError error;

  if (global_conn == NULL)
    {
      if (!ensure_dbus_connection ())
	{
	  g_set_error (err, MATECONF_ERROR,
		       MATECONF_ERROR_NO_SERVER,
		       _("No D-BUS daemon running\n"));
	  return FALSE;
	}

      g_assert (global_conn != NULL);
    }

  if (service_running)
    return TRUE;
  
  if (start_if_not_found)
    {
      d(g_print ("* activate_service, activating\n"));

      dbus_error_init (&error);
      
      if (!dbus_bus_start_service_by_name (global_conn,
					   MATECONF_DBUS_SERVICE,
					   0,
					   NULL,
					   &error))
	{
	  const gchar *msg;
	  
	  if (dbus_error_is_set (&error))
	    msg = error.message;
	  else
	    msg = _("Unknown error");
	  
	  g_set_error (err, MATECONF_ERROR,
		       MATECONF_ERROR_NO_SERVER,
		       _("Failed to activate configuration server: %s\n"),
		       msg);
	  
	  if (dbus_error_is_set (&error))
	    dbus_error_free (&error);
	  
	  return FALSE;
	}
      
      service_running = TRUE;
      
      return TRUE;
    }
  
  return FALSE;
}

static gboolean
ensure_database (MateConfEngine  *conf,
		 gboolean      start_if_not_found,
		 GError      **err)
{
  DBusMessage *message, *reply;
  DBusError error;
  gchar *db;

  g_return_val_if_fail (!conf->is_local, TRUE);

  if (!ensure_service (start_if_not_found, err))
    return FALSE;

  if (needs_reconnect)
    {
      /* Re-connect notifications and re-get database names from the previous
       * (if any) instance of the MateConf service.
       */
      needs_reconnect = FALSE;
      reinitialize_databases ();
    }
  
  if (conf->database != NULL)
    return TRUE;
  
  if (conf->is_default)
    {
      message = dbus_message_new_method_call (MATECONF_DBUS_SERVICE,
					      MATECONF_DBUS_SERVER_OBJECT,
					      MATECONF_DBUS_SERVER_INTERFACE,
					      MATECONF_DBUS_SERVER_GET_DEFAULT_DB);
    }
  else
    {
      gchar *addresses;

      addresses = mateconf_address_list_get_persistent_name (conf->addresses);
      
      message = dbus_message_new_method_call (MATECONF_DBUS_SERVICE,
					      MATECONF_DBUS_SERVER_OBJECT,
					      MATECONF_DBUS_SERVER_INTERFACE,
					      MATECONF_DBUS_SERVER_GET_DB);
      dbus_message_append_args (message,
				DBUS_TYPE_STRING, &addresses,
				DBUS_TYPE_INVALID);
      
      g_free (addresses);
    }

  dbus_error_init (&error);
  reply = dbus_connection_send_with_reply_and_block (global_conn,
						     message, -1, &error);
  
  dbus_message_unref (message);
  
  if (mateconf_handle_dbus_exception (reply, &error, err))
    return FALSE;

  dbus_message_get_args (reply,
			 NULL,
			 DBUS_TYPE_STRING, &db,
			 DBUS_TYPE_INVALID);

  dbus_message_unref (reply);

  if (db == NULL)
    {
      if (err)
        *err = mateconf_error_new (MATECONF_ERROR_BAD_ADDRESS,
				_("Server couldn't resolve the address `%s'"),
				conf->persistent_address);
      
      return FALSE;
    }

  mateconf_engine_set_database (conf, db);

  return TRUE;
}

static const gchar *
mateconf_engine_get_database (MateConfEngine *conf,
                           gboolean start_if_not_found,
                           GError **err)
{
  if (!ensure_database (conf, start_if_not_found, err))
    return NULL;
  else
    return conf->database;
}

static gboolean
mateconf_engine_is_local (MateConfEngine* conf)
{
  return conf->is_local;
}

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
lookup_engine_by_addresses (GSList *addresses)
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
 * Connection maintenance
 */

static MateConfCnxn *
mateconf_cnxn_new (MateConfEngine *conf,
		const gchar *namespace_section,
		MateConfNotifyFunc func,
		gpointer user_data)
{
  MateConfCnxn *cnxn;
  static guint next_id = 1;
  
  cnxn = g_new0 (MateConfCnxn, 1);

  cnxn->namespace_section = g_strdup (namespace_section);
  cnxn->conf = conf;
  cnxn->client_id = next_id;
  cnxn->func = func;
  cnxn->user_data = user_data;

  ++next_id;

  return cnxn;
}

static void      
mateconf_cnxn_destroy (MateConfCnxn* cnxn)
{
  g_free (cnxn->namespace_section);
  g_free (cnxn);
}

static void       
mateconf_cnxn_notify (MateConfCnxn* cnxn,
		   MateConfEntry *entry)
{
  (*cnxn->func) (cnxn->conf, cnxn->client_id,
		 entry,
		 cnxn->user_data);
}

static GList *
mateconf_cnxn_lookup_dir (MateConfEngine *conf, const gchar *dir)
{
  CnxnsData *data;

  data = g_hash_table_lookup (conf->notify_dirs, dir);

  if (data == NULL)
    return NULL;

  return data->cnxns;
}

static MateConfCnxn *
mateconf_cnxn_lookup_id (MateConfEngine *conf, guint client_id)
{
  gint id = client_id;

  return g_hash_table_lookup (conf->notify_ids, GINT_TO_POINTER (id));
}

static void
mateconf_cnxn_insert (MateConfEngine *conf, const gchar *dir, guint client_id, MateConfCnxn *cnxn)
{
  CnxnsData *data;
  gint id = client_id;

  data = g_hash_table_lookup (conf->notify_dirs, dir);

  if (data == NULL)
    {
      data = g_new (CnxnsData, 1);
      data->cnxns = NULL;
      g_hash_table_insert (conf->notify_dirs, g_strdup (dir), data);
    }
  
  data->cnxns = g_list_prepend (data->cnxns, cnxn);

  g_hash_table_insert (conf->notify_ids, GINT_TO_POINTER (id), cnxn);
}

static void
mateconf_cnxn_remove (MateConfEngine *conf, MateConfCnxn *cnxn)
{
  CnxnsData *data;
  gint id = cnxn->client_id;

  g_hash_table_remove (conf->notify_ids, GINT_TO_POINTER (id));

  data = g_hash_table_lookup (conf->notify_dirs, cnxn->namespace_section);
  if (data)
    {
      data->cnxns = g_list_remove (data->cnxns, cnxn);

      if (data->cnxns == NULL)
	{
	  g_hash_table_remove (conf->notify_dirs, cnxn->namespace_section);
	  g_free (data);

	  mateconf_cnxn_destroy (cnxn);
	}
    }
}
  

/*
 *  Public Interface
 */

MateConfEngine*
mateconf_engine_get_local      (const gchar* address,
                             GError** err)
{
  MateConfEngine *conf;
  MateConfSource *source;

  g_return_val_if_fail (address != NULL, NULL);
  g_return_val_if_fail (err == NULL || *err == NULL, NULL);
  
  source = mateconf_resolve_address (address, err);

  if (source == NULL)
    return NULL;
  
  conf = mateconf_engine_blank (FALSE);

  conf->local_sources = mateconf_sources_new_from_source (source);

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
      conf = mateconf_engine_blank (TRUE);

      conf->is_default = TRUE;

      default_engine = conf;
    }
  else
    conf->refcount += 1;
  
  return conf;
}

MateConfEngine*
mateconf_engine_get_for_address (const gchar* address, GError** err)
{
  MateConfEngine *conf;
  GSList      *addresses;
  
  addresses = g_slist_append (NULL, g_strdup (address));

  conf = lookup_engine_by_addresses (addresses);

  if (conf == NULL)
    {
      conf = mateconf_engine_blank (TRUE);

      conf->is_default = FALSE;
      conf->addresses = addresses;

      if (!ensure_database (conf, TRUE, err))
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

  conf = lookup_engine_by_addresses (addresses);

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

      if (!ensure_database (conf, TRUE, err))
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
mateconf_engine_ref (MateConfEngine* conf)
{
  g_return_if_fail (conf != NULL);
  g_return_if_fail (conf->refcount > 0);

  conf->refcount += 1;
}

void         
mateconf_engine_unref (MateConfEngine* conf)
{
  g_return_if_fail (conf != NULL);
  g_return_if_fail (conf->refcount > 0);

  conf->refcount -= 1;
  
  if (conf->refcount == 0)
    {
      if (mateconf_engine_is_local (conf))
        {
          if (conf->local_sources != NULL)
            mateconf_sources_free (conf->local_sources);
        }
      else
        {
          /* Remove all connections associated with this MateConf */

	  /* FIXME: remove notify_ids from hash when we have
	     add/remove_notify. */

          if (conf->dnotify)
            {
              (* conf->dnotify) (conf->user_data);
            }
          
          /* do this after removing the notifications,
             to avoid funky race conditions */
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

          if (conf->notify_ids)
	    g_hash_table_destroy (conf->notify_ids);
          if (conf->notify_dirs)
	    g_hash_table_destroy (conf->notify_dirs);
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
mateconf_engine_get_user_data (MateConfEngine   *engine)
{
  return engine->user_data;
}

static gboolean
send_notify_add (MateConfEngine *conf,
		 MateConfCnxn *cnxn,
		 GError **err)
{
  const gchar *db;
  DBusMessage *message, *reply;
  DBusError error;
    
  db = mateconf_engine_get_database (conf, TRUE, err);
  
  if (db == NULL)
    {
      g_return_val_if_fail(err == NULL || *err != NULL, 0);
      
      return FALSE;
    }

  message = dbus_message_new_method_call (MATECONF_DBUS_SERVICE,
					  db,
					  MATECONF_DBUS_DATABASE_INTERFACE,
					  MATECONF_DBUS_DATABASE_ADD_NOTIFY);
  
  dbus_message_append_args (message,
			    DBUS_TYPE_STRING, &cnxn->namespace_section,
			    DBUS_TYPE_INVALID);

  dbus_error_init (&error);
  reply = dbus_connection_send_with_reply_and_block (global_conn,
						     message, -1, &error);
  dbus_message_unref (message);
  
  if (mateconf_handle_dbus_exception (reply, &error, err))
    return FALSE;
  
  dbus_message_unref (reply);

  return TRUE;
}

guint
mateconf_engine_notify_add (MateConfEngine* conf,
			 const gchar* namespace_section,
			 MateConfNotifyFunc func,
			 gpointer user_data,
			 GError** err)
{
  MateConfCnxn *cnxn;
  
  g_return_val_if_fail (!mateconf_engine_is_local (conf), 0);

  CHECK_OWNER_USE (conf);
  
  if (mateconf_engine_is_local (conf))
    {
      if (err)
        *err = mateconf_error_new (MATECONF_ERROR_LOCAL_ENGINE,
				_("Can't add notifications to a local configuration source"));

      return 0;
    }

  cnxn = mateconf_cnxn_new (conf, namespace_section, func, user_data);
  mateconf_cnxn_insert (conf, namespace_section, cnxn->client_id, cnxn);
  
  if (!send_notify_add (conf, cnxn, err))
    {
      mateconf_cnxn_remove (conf, cnxn);
      return 0;
    }

  return cnxn->client_id;
}

void         
mateconf_engine_notify_remove (MateConfEngine* conf,
			    guint client_id)
{
  MateConfCnxn *cnxn;
  const gchar *db;
  gchar *namespace_section = NULL;
  DBusMessage *message, *reply;
  DBusError error;
  
  CHECK_OWNER_USE (conf);
  
  if (mateconf_engine_is_local(conf))
    return;

  cnxn = mateconf_cnxn_lookup_id (conf, client_id);
  if (cnxn != NULL)
    {
      namespace_section = g_strdup (cnxn->namespace_section);
      mateconf_cnxn_remove (conf, cnxn);
    }
  
  g_return_if_fail (cnxn != NULL); 
  
  db = mateconf_engine_get_database (conf, TRUE, NULL);
  
  if (db == NULL)
    return;

  d(g_print ("notify_remove, id = %d\n", client_id));
  
  message = dbus_message_new_method_call (MATECONF_DBUS_SERVICE,
					  db,
					  MATECONF_DBUS_DATABASE_INTERFACE,
					  MATECONF_DBUS_DATABASE_REMOVE_NOTIFY);
  
  dbus_message_append_args (message,
			    DBUS_TYPE_STRING, &namespace_section,
			    DBUS_TYPE_INVALID);

  dbus_error_init (&error);
  reply = dbus_connection_send_with_reply_and_block (global_conn, message, -1, &error);
  dbus_message_unref (message);
  
  if (dbus_error_is_set (&error))
    dbus_error_free (&error);
  
  g_free (namespace_section);
  
  dbus_message_unref (reply);
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
  const gchar *db;
  gboolean is_default = FALSE;
  gboolean is_writable = TRUE;
  gchar *schema_name = NULL;
  DBusMessage *message, *reply;
  DBusError error;
  DBusMessageIter iter;
  gboolean success;
  
  g_return_val_if_fail(conf != NULL, NULL);
  g_return_val_if_fail(key != NULL, NULL);
  g_return_val_if_fail(err == NULL || *err == NULL, NULL);

  CHECK_OWNER_USE (conf);
  
  if (!mateconf_key_check (key, err))
    return NULL;

  if (mateconf_engine_is_local (conf))
    {
      gchar **locale_list;
      
      locale_list = mateconf_split_locale (locale);
      
      val = mateconf_sources_query_value (conf->local_sources,
				       key,
				       (const gchar**)locale_list,
				       use_schema_default,
				       &is_default,
				       &is_writable,
				       schema_name_p ? &schema_name : NULL,
				       err);

      if (locale_list != NULL)
        g_strfreev(locale_list);
      
      if (is_default_p)
        *is_default_p = is_default;

      if (is_writable_p)
        *is_writable_p = is_writable;

      if (schema_name_p)
        *schema_name_p = schema_name;
      else
        g_free (schema_name);
      
      return val;
    }

  g_assert (!mateconf_engine_is_local (conf));

  db = mateconf_engine_get_database (conf, TRUE, err);

  if (db == NULL)
    {
      g_return_val_if_fail(err == NULL || *err != NULL, NULL);
      return NULL;
    }

  if (schema_name_p)
    *schema_name_p = NULL;

  message = dbus_message_new_method_call (MATECONF_DBUS_SERVICE,
					  db,
					  MATECONF_DBUS_DATABASE_INTERFACE,
					  MATECONF_DBUS_DATABASE_LOOKUP_EXTENDED);

  locale = locale ? locale : mateconf_current_locale();
  
  dbus_message_append_args (message,
			    DBUS_TYPE_STRING, &key,
			    DBUS_TYPE_STRING, &locale,
			    DBUS_TYPE_BOOLEAN, &use_schema_default,
			    DBUS_TYPE_INVALID);

  dbus_error_init (&error);
  reply = dbus_connection_send_with_reply_and_block (global_conn, message, -1, &error);
  dbus_message_unref (message);

  if (mateconf_handle_dbus_exception (reply, &error, err))
    return NULL;

  dbus_message_iter_init (reply, &iter);

  /* If there is no struct (entry) here, there is no value. */
  if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_STRUCT)
    {
      dbus_message_unref (reply);
      return NULL;
    }
  
  success = mateconf_dbus_utils_get_entry_values (&iter,
					       NULL,
					       &val,
					       &is_default,
					       &is_writable,
					       &schema_name);
  
  dbus_message_unref (reply);
  
  if (!success)
    {
      if (err)
	g_set_error (err, MATECONF_ERROR,
		     MATECONF_ERROR_FAILED,
		     _("Couldn't get value"));
      
      return NULL;
    }
  
  if (is_default_p)
    *is_default_p = !!is_default;
  
  if (is_writable_p)
    *is_writable_p = !!is_writable;
  
  if (schema_name && schema_name[0] != '/')
    {
      schema_name = NULL;
    }
  
  if (schema_name_p)
    *schema_name_p = g_strdup (schema_name);
  
  return val;
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
mateconf_engine_get_entry (MateConfEngine* conf,
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

  entry = mateconf_entry_new_nocopy (g_strdup (key), val);

  mateconf_entry_set_is_default (entry, is_default);
  mateconf_entry_set_is_writable (entry, is_writable);
  mateconf_entry_set_schema_name (entry, schema_name);

  g_free (schema_name);

  return entry;
}
     
MateConfValue*  
mateconf_engine_get (MateConfEngine* conf, const gchar* key, GError** err)
{
  return mateconf_engine_get_with_locale (conf, key, NULL, err);
}

MateConfValue*
mateconf_engine_get_with_locale (MateConfEngine* conf, const gchar* key,
			      const gchar* locale,
			      GError** err)
{
  return mateconf_engine_get_full (conf, key, locale, TRUE,
				NULL, NULL, err);
}

MateConfValue*
mateconf_engine_get_without_default (MateConfEngine* conf, const gchar* key,
				  GError** err)
{
  return mateconf_engine_get_full (conf, key, NULL, FALSE, NULL, NULL, err);
}

MateConfValue*
mateconf_engine_get_default_from_schema (MateConfEngine* conf,
                                      const gchar* key,
                                      GError** err)
{
  MateConfValue* val;
  const gchar *db;
  const gchar *locale;
  DBusMessage *message, *reply;
  DBusError error;
  DBusMessageIter iter;
  
  g_return_val_if_fail(conf != NULL, NULL);
  g_return_val_if_fail(key != NULL, NULL);
  g_return_val_if_fail(err == NULL || *err == NULL, NULL);

  CHECK_OWNER_USE (conf);
  
  if (!mateconf_key_check (key, err))
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

  g_assert (!mateconf_engine_is_local (conf));

  db = mateconf_engine_get_database (conf, TRUE, err);

  if (db == NULL)
    {
      g_return_val_if_fail(err == NULL || *err != NULL, NULL);
      return NULL;
    }

  message = dbus_message_new_method_call (MATECONF_DBUS_SERVICE,
					  db,
					  MATECONF_DBUS_DATABASE_INTERFACE,
					  MATECONF_DBUS_DATABASE_LOOKUP_DEFAULT);

  locale = mateconf_current_locale();
  
  dbus_message_append_args (message,
			    DBUS_TYPE_STRING, &key,
			    DBUS_TYPE_STRING, &locale,
			    DBUS_TYPE_INVALID);

  dbus_error_init (&error);
  reply = dbus_connection_send_with_reply_and_block (global_conn, message, -1, &error);
  dbus_message_unref (message);

  if (mateconf_handle_dbus_exception (reply, &error, err))
    return NULL;

  dbus_message_iter_init (reply, &iter);

  /* If there is no struct (entry) here, there is no value. */
  if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_STRUCT)
    {
      dbus_message_unref (reply);
      return NULL;
    }
  
  val = mateconf_dbus_utils_get_value (&iter);
  
  dbus_message_unref (reply);
  
  if (!val)
    {
      if (err)
	g_set_error (err, MATECONF_ERROR,
		     MATECONF_ERROR_FAILED,
		     _("Couldn't get value"));
      
      return NULL;
    }
  
  return val;
}

gboolean
mateconf_engine_set (MateConfEngine* conf, const gchar* key,
                  const MateConfValue* value, GError** err)
{
  const gchar *db;
  DBusMessage *message, *reply;
  DBusError error;
  DBusMessageIter iter;

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

  if (!mateconf_key_check (key, err))
    return FALSE;

  if (!mateconf_value_validate (value, err))
    return FALSE;
  
  if (mateconf_engine_is_local (conf))
    {
      GError* error = NULL;
      
      mateconf_sources_set_value (conf->local_sources, key, value, NULL, &error);

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
  
  db = mateconf_engine_get_database (conf, TRUE, err);

  if (db == NULL)
    {
      g_return_val_if_fail(err == NULL || *err != NULL, FALSE);
      
      return FALSE;
    }

  message = dbus_message_new_method_call (MATECONF_DBUS_SERVICE,
					  db,
					  MATECONF_DBUS_DATABASE_INTERFACE,
					  MATECONF_DBUS_DATABASE_SET);
  
  dbus_message_append_args (message,
			    DBUS_TYPE_STRING, &key,
			    DBUS_TYPE_INVALID);

  dbus_message_iter_init_append (message, &iter);
  mateconf_dbus_utils_append_value (&iter, value);

  dbus_error_init (&error);
  reply = dbus_connection_send_with_reply_and_block (global_conn, message, -1, &error);
  dbus_message_unref (message);

  if (mateconf_handle_dbus_exception (reply, &error, err))
    return FALSE;

  g_return_val_if_fail (err == NULL || *err == NULL, FALSE);

  dbus_message_unref (reply);

  return TRUE;
}

gboolean
mateconf_engine_unset (MateConfEngine* conf, const gchar* key, GError** err)
{
  const gchar *db;
  DBusMessage *message, *reply;
  DBusError error;
  const gchar *empty;

  g_return_val_if_fail (conf != NULL, FALSE);
  g_return_val_if_fail (key != NULL, FALSE);
  g_return_val_if_fail (err == NULL || *err == NULL, FALSE);

  CHECK_OWNER_USE (conf);
  
  if (!mateconf_key_check (key, err))
    return FALSE;

  if (mateconf_engine_is_local (conf))
    {
      GError* error = NULL;
      
      mateconf_sources_unset_value (conf->local_sources, key, NULL, NULL, &error);

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

  g_assert(!mateconf_engine_is_local(conf));
  
  db = mateconf_engine_get_database (conf, TRUE, err);

  if (db == NULL)
    {
      g_return_val_if_fail(err == NULL || *err != NULL, FALSE);

      return FALSE;
    }

  message = dbus_message_new_method_call (MATECONF_DBUS_SERVICE,
					  db,
					  MATECONF_DBUS_DATABASE_INTERFACE,
					  MATECONF_DBUS_DATABASE_UNSET);

  empty = "";
  dbus_message_append_args (message,
			    DBUS_TYPE_STRING, &key,
			    DBUS_TYPE_STRING, &empty,
			    DBUS_TYPE_INVALID);

  dbus_error_init (&error);
  reply = dbus_connection_send_with_reply_and_block (global_conn, message, -1, &error);
  dbus_message_unref (message);
  
  if (mateconf_handle_dbus_exception (reply, &error, err))
    return FALSE;

  g_return_val_if_fail (err == NULL || *err == NULL, FALSE);

  dbus_message_unref (reply);

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
  const gchar *db;
  DBusMessage *message, *reply;
  DBusError error;
  guint dbus_flags;
  const gchar *empty;
  
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
  
  dbus_flags = 0;
  if (flags & MATECONF_UNSET_INCLUDING_SCHEMA_NAMES)
    dbus_flags |= MATECONF_DBUS_UNSET_INCLUDING_SCHEMA_NAMES;

  db = mateconf_engine_get_database (conf, TRUE, err);

  if (db == NULL)
    {
      g_return_val_if_fail (err == NULL || *err != NULL, FALSE);

      return FALSE;
    }

  message = dbus_message_new_method_call (MATECONF_DBUS_SERVICE,
					  db,
					  MATECONF_DBUS_DATABASE_INTERFACE,
					  MATECONF_DBUS_DATABASE_RECURSIVE_UNSET);

  empty = "";
  dbus_message_append_args (message,
			    DBUS_TYPE_STRING, &key,
			    DBUS_TYPE_STRING, &empty,
			    DBUS_TYPE_UINT32, &dbus_flags,
			    DBUS_TYPE_INVALID);

  dbus_error_init (&error);
  reply = dbus_connection_send_with_reply_and_block (global_conn, message, -1, &error);
  dbus_message_unref (message);
  
  if (mateconf_handle_dbus_exception (reply, &error, err))
    return FALSE;

  g_return_val_if_fail (err == NULL || *err == NULL, FALSE);

  dbus_message_unref (reply);

  return TRUE;
}

gboolean
mateconf_engine_associate_schema  (MateConfEngine* conf, const gchar* key,
                                const gchar* schema_key, GError** err)
{
  const gchar *db;
  DBusMessage *message, *reply;
  DBusError error;
  
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
  
  db = mateconf_engine_get_database (conf, TRUE, err);

  if (db == NULL)
    {
      g_return_val_if_fail (err == NULL || *err != NULL, FALSE);

      return FALSE;
    }

  message = dbus_message_new_method_call (MATECONF_DBUS_SERVICE,
					  db,
					  MATECONF_DBUS_DATABASE_INTERFACE,
					  MATECONF_DBUS_DATABASE_SET_SCHEMA);

   /* Empty schema string means unset. */
  schema_key = schema_key ? schema_key : "";
  
  dbus_message_append_args (message,
			    DBUS_TYPE_STRING, &key,
			    DBUS_TYPE_STRING, &schema_key,
			    DBUS_TYPE_INVALID);

  dbus_error_init (&error);
  reply = dbus_connection_send_with_reply_and_block (global_conn, message, -1, &error);
  dbus_message_unref (message);
  
  if (mateconf_handle_dbus_exception (reply, &error, err))
    return FALSE;
  
  g_return_val_if_fail (err == NULL || *err == NULL, FALSE);

  dbus_message_unref (reply);

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
mateconf_engine_all_entries (MateConfEngine* conf, const gchar* dir, GError** err)
{
  GSList* entries = NULL;
  const gchar *db;
  DBusMessage *message, *reply;
  DBusError error;
  DBusMessageIter iter;
  const gchar *locale;

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
  
  db = mateconf_engine_get_database (conf, TRUE, err);

  if (db == NULL)
    {
      g_return_val_if_fail(err == NULL || *err != NULL, NULL);

      return NULL;
    }

  message = dbus_message_new_method_call (MATECONF_DBUS_SERVICE,
					  db,
					  MATECONF_DBUS_DATABASE_INTERFACE,
					  MATECONF_DBUS_DATABASE_GET_ALL_ENTRIES);

  locale = mateconf_current_locale ();
  dbus_message_append_args (message,
			    DBUS_TYPE_STRING, &dir,
			    DBUS_TYPE_STRING, &locale,
			    DBUS_TYPE_INVALID);
  
  dbus_error_init (&error);
  reply = dbus_connection_send_with_reply_and_block (global_conn, message, -1, &error);
  dbus_message_unref (message);
  
  if (mateconf_handle_dbus_exception (reply, &error, err))
    return NULL;
  
  g_return_val_if_fail (err == NULL || *err == NULL, NULL);

  dbus_message_iter_init (reply, &iter);
  
  entries = mateconf_dbus_utils_get_entries (&iter, dir);
  
  dbus_message_unref (reply);

  return entries;
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
  const gchar *db;
  DBusMessage *message, *reply;
  DBusError error;
  DBusMessageIter iter;
  DBusMessageIter array_iter;
  
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

  db = mateconf_engine_get_database (conf, TRUE, err);

  if (db == NULL)
    {
      g_return_val_if_fail (err == NULL || *err != NULL, NULL);
      
      return NULL;
    }
  
  message = dbus_message_new_method_call (MATECONF_DBUS_SERVICE,
					  db,
					  MATECONF_DBUS_DATABASE_INTERFACE,
					  MATECONF_DBUS_DATABASE_GET_ALL_DIRS);

  dbus_message_append_args (message,
			    DBUS_TYPE_STRING, &dir,
			    DBUS_TYPE_INVALID);
  
  dbus_error_init (&error);
  reply = dbus_connection_send_with_reply_and_block (global_conn, message, -1, &error);
  dbus_message_unref (message);
  
  if (mateconf_handle_dbus_exception (reply, &error, err))
    return NULL;

  g_return_val_if_fail (err == NULL || *err == NULL, NULL);

  dbus_message_iter_init (reply, &iter);

  dbus_message_iter_recurse (&iter, &array_iter);
  while (dbus_message_iter_get_arg_type (&array_iter) == DBUS_TYPE_STRING)
    {
      const gchar *key;
      gchar       *s;
      
      dbus_message_iter_get_basic (&array_iter, &key);
      
      s = mateconf_concat_dir_and_key (dir, key);
      subdirs = g_slist_prepend (subdirs, s);
      
      if (!dbus_message_iter_next (&array_iter))
	break;
    }
  
  dbus_message_unref (reply);

  return subdirs;
}

/* annoyingly, this is REQUIRED for local sources */
void 
mateconf_engine_suggest_sync(MateConfEngine* conf, GError** err)
{
  const gchar *db;
  DBusMessage *message;
  DBusMessage *reply;
  DBusError error;
  
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

  db = mateconf_engine_get_database (conf, TRUE, err);

  if (db == NULL)
    {
      g_return_if_fail (err == NULL || *err != NULL);
    }
  
  message = dbus_message_new_method_call (MATECONF_DBUS_SERVICE,
					  db,
					  MATECONF_DBUS_DATABASE_INTERFACE,
					  MATECONF_DBUS_DATABASE_SUGGEST_SYNC);

  dbus_error_init (&error);
  reply = dbus_connection_send_with_reply_and_block (global_conn, message, -1, &error);
  dbus_message_unref (message);

  if (!mateconf_handle_dbus_exception (reply, &error, err))
    dbus_message_unref (reply);
}

void 
mateconf_clear_cache(MateConfEngine* conf, GError** err)
{
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

  /* Do nothing for non-local case. */
}

void 
mateconf_synchronous_sync(MateConfEngine* conf, GError** err)
{
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

  /* Do nothing for non-local case. */
}

gboolean
mateconf_engine_dir_exists (MateConfEngine *conf, const gchar *dir, GError** err)
{
  const gchar *db;
  dbus_bool_t exists;
  DBusMessage *message, *reply;
  DBusError error;
  
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
  
  db = mateconf_engine_get_database(conf, TRUE, err);
  
  if (db == NULL)
    {
      g_return_val_if_fail(err == NULL || *err != NULL, FALSE);

      return FALSE;
    }

  message = dbus_message_new_method_call (MATECONF_DBUS_SERVICE,
					  db,
					  MATECONF_DBUS_DATABASE_INTERFACE,
					  MATECONF_DBUS_DATABASE_DIR_EXISTS);
  
  dbus_message_append_args (message,
			    DBUS_TYPE_STRING, &dir,
			    DBUS_TYPE_INVALID);
  
  dbus_error_init (&error);
  reply = dbus_connection_send_with_reply_and_block (global_conn, message, -1, &error);
  dbus_message_unref (message);
 
  if (mateconf_handle_dbus_exception (reply, &error, err))
    return FALSE;

  g_return_val_if_fail (err == NULL || *err == NULL, FALSE);

  exists = FALSE;
  dbus_message_get_args (reply,
			 NULL,
			 DBUS_TYPE_BOOLEAN, &exists,
			 DBUS_TYPE_INVALID);

  dbus_message_unref (reply);

  return !!exists;
}

void
mateconf_engine_remove_dir (MateConfEngine* conf,
                         const gchar* dir,
                         GError** err)
{
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
}

static void
cnxn_get_all_func (gpointer key,
		   gpointer value,
		   gpointer user_data)
{
  GList **list = user_data;
  
  *list = g_list_prepend (*list, value);
}

static void
engines_by_db_get_all_func (gpointer key,
			    gpointer value,
			    gpointer user_data)
{
  GList **list = user_data;

  *list = g_list_prepend (*list, value);
}

static void
reinitialize_databases (void)
{
  GList *engines = NULL, *engine;
  GList *cnxns, *l;
  MateConfEngine *conf;
  
  if (engines_by_db)
    g_hash_table_foreach (engines_by_db,
			  engines_by_db_get_all_func,
			  &engines);

  /* Reset databases. */
  for (engine = engines; engine; engine = engine->next)
    {
      conf = engine->data;

      g_hash_table_remove (engines_by_db, conf->database);  
      ensure_database (conf, FALSE, NULL);
    }
  
  /* Re-add notifications. */
  for (engine = engines; engine; engine = engine->next)
    {
      conf = engine->data;
      
      cnxns = NULL;
      g_hash_table_foreach (conf->notify_ids,
			    cnxn_get_all_func,
			    &cnxns);
      
      for (l = cnxns; l; l = l->next)
	{
	  MateConfCnxn *cnxn = l->data;
	  
	  send_notify_add (conf, cnxn, NULL);
	}
      
      g_list_free (cnxns);
    }
  
  g_list_free (engines);
}

static DBusHandlerResult
mateconf_dbus_message_filter (DBusConnection    *dbus_conn,
			   DBusMessage       *message,
			   gpointer           user_data)
{
  if (dbus_message_is_method_call (message,
				   MATECONF_DBUS_CLIENT_INTERFACE,
				   "Notify"))
    {
      return handle_notify (dbus_conn, message, NULL);
    }
  else if (dbus_message_is_signal (message,
				   DBUS_INTERFACE_LOCAL,
				   "Disconnected"))
    {
      dbus_connection_unref (global_conn);
      global_conn = NULL;
      service_running = FALSE;
      dbus_disconnected = TRUE;

      g_warning ("Got Disconnected from DBus.\n");

      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
  else if (dbus_message_is_signal (message,
				   DBUS_INTERFACE_DBUS,
				   "NameOwnerChanged"))
    {
      char *service;
      char *old_owner;
      char *new_owner;

      dbus_message_get_args (message,
			     NULL,
			     DBUS_TYPE_STRING, &service,
			     DBUS_TYPE_STRING, &old_owner,
			     DBUS_TYPE_STRING, &new_owner,
			     DBUS_TYPE_INVALID);
      
      if (strcmp (service, MATECONF_DBUS_SERVICE) != 0)
	{
	  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}
      
      if (strcmp (old_owner, "") == 0) 
	{
	  /* MateConfd is back. */
	  service_running = TRUE;
	  
	  if (needs_reconnect)
	    {
	      needs_reconnect = FALSE;
	      reinitialize_databases ();
	    }
	  
	  d(g_print ("*** Gconf Service created\n"));
	}

      if (strcmp (new_owner, "") == 0) 
	{
	  /* MateConfd is gone, set the state so we can detect that we're down. */
	  service_running = FALSE;
	  needs_reconnect = TRUE;
  
	  d(g_print ("*** MateConf Service deleted\n"));
	}
      
      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;      
    }

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
} 

/* FIXME: What should this do in the D-BUS case? */
static void
mateconf_detach_config_server(void)
{  
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
 * all related resources
 **/
int
mateconf_debug_shutdown (void)
{
  mateconf_detach_config_server ();

  return 0;
}

static DBusHandlerResult
handle_notify (DBusConnection *connection,
	       DBusMessage *message,
	       MateConfEngine *conf2)
{
  MateConfEngine *conf;
  gchar *key, *schema_name;
  gboolean is_default, is_writable;
  DBusMessageIter iter;
  MateConfValue *value;
  MateConfEntry* entry;
  GList *list, *l;
  gboolean match = FALSE;
  gchar *namespace_section, *db;

  dbus_message_iter_init (message, &iter);

  dbus_message_iter_get_basic (&iter, &db);

  if (!dbus_message_iter_next (&iter))
    {
      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
  
  dbus_message_iter_get_basic (&iter, &namespace_section);

  if (!dbus_message_iter_next (&iter))
    {
      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }

  conf = lookup_engine_by_database (db);

  g_return_val_if_fail (conf != NULL, DBUS_HANDLER_RESULT_NOT_YET_HANDLED);
  if (conf == NULL)
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  
  if (!mateconf_dbus_utils_get_entry_values (&iter,
					  &key,
					  &value,
					  &is_default,
					  &is_writable,
					  &schema_name))
    {
      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
  
  d(g_print ("Got notify on %s (%s)\n", key, namespace_section));

  list = mateconf_cnxn_lookup_dir (conf, namespace_section);
  for (l = list; l; l = l->next)
    {
      MateConfCnxn *cnxn = l->data;

      d(g_print ("match? %s\n", cnxn->namespace_section));
      
      if (strcmp (cnxn->namespace_section, namespace_section) == 0)
	{
	  d(g_print ("yes: %s\n", key));
	  
	  entry = mateconf_entry_new (key, value);
	  mateconf_cnxn_notify (cnxn, entry);
	  mateconf_entry_free (entry);
	  
	  match = TRUE;
	}
    }

  if (value)
    mateconf_value_free (value);

  if (!match)
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  
  return DBUS_HANDLER_RESULT_HANDLED;
}


/*
 * Daemon control
 */

void          
mateconf_shutdown_daemon (GError** err)
{
  DBusMessage *message;

  /* Don't want to spawn it if it's already down */
  if (global_conn == NULL || !service_running)
    return;
  
  message = dbus_message_new_method_call (MATECONF_DBUS_SERVICE,
					  MATECONF_DBUS_SERVER_OBJECT,
					  MATECONF_DBUS_SERVER_INTERFACE,
					  MATECONF_DBUS_SERVER_SHUTDOWN);

  dbus_connection_send (global_conn, message, 0);
  dbus_connection_flush (global_conn);

  dbus_message_unref (message);
}

gboolean
mateconf_ping_daemon (void)
{
  return service_running;
}

gboolean
mateconf_spawn_daemon (GError **err)
{
  return ensure_service (TRUE, err);
}


