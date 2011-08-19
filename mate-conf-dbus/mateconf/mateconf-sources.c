
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

#include "mateconf-backend.h"
#include "mateconf-sources.h"
#include "mateconf-internals.h"
#include "mateconf-schema.h"
#include "mateconf.h"
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

/* 
 *  Sources
 */

MateConfSource* 
mateconf_resolve_address(const gchar* address, GError** err)
{
  MateConfBackend* backend;

  backend = mateconf_get_backend(address, err);

  if (backend == NULL)
    return NULL;
  else
    {
      MateConfSource* retval;

      retval = mateconf_backend_resolve_address(backend, address, err);

      if (retval == NULL)
        {
          mateconf_backend_unref(backend);
          return NULL;
        }
      else
        {
          retval->backend = backend;
          retval->address = g_strdup(address);
          
          /* Leave a ref on the backend, now held by the MateConfSource */
          
          return retval;
        }
    }
}

void         
mateconf_source_free (MateConfSource* source)
{
  MateConfBackend* backend;
  gchar* address;
  
  g_return_if_fail(source != NULL);

  backend = source->backend;
  address = source->address;

  (*source->backend->vtable.destroy_source)(source);
  
  /* Remove ref held by the source. */
  mateconf_backend_unref(backend);
  g_free(address);
}

#define SOURCE_READABLE(source, key, err)                  \
     ( ((source)->flags & MATECONF_SOURCE_ALL_READABLE) ||    \
       ((source)->backend->vtable.readable != NULL &&     \
        (*(source)->backend->vtable.readable)((source), (key), (err))) )

static gboolean
source_is_writable(MateConfSource* source, const gchar* key, GError** err)
{
  if ((source->flags & MATECONF_SOURCE_NEVER_WRITEABLE) != 0)
    return FALSE;
  else if ((source->flags & MATECONF_SOURCE_ALL_WRITEABLE) != 0)
    return TRUE;
  else if ((source->backend->vtable.writable != NULL) &&
           (*source->backend->vtable.writable)(source, key, err))
    return TRUE;
  else
    return FALSE;
}

static MateConfValue*
mateconf_source_query_value      (MateConfSource* source,
                               const gchar* key,
                               const gchar** locales,
                               gchar** schema_name,
                               GError** err)
{
  g_return_val_if_fail(source != NULL, NULL);
  g_return_val_if_fail(key != NULL, NULL);
  g_return_val_if_fail(err == NULL || *err == NULL, NULL);
  
  /* note that key validity is unchecked */

  if ( SOURCE_READABLE(source, key, err) )
    {
      g_return_val_if_fail(err == NULL || *err == NULL, NULL);
      return (*source->backend->vtable.query_value)(source, key, locales, schema_name, err);
    }
  else
    return NULL;
}

static MateConfMetaInfo*
mateconf_source_query_metainfo      (MateConfSource* source,
                                  const gchar* key,
                                  GError** err)
{
  g_return_val_if_fail(source != NULL, NULL);
  g_return_val_if_fail(key != NULL, NULL);
  g_return_val_if_fail(err == NULL || *err == NULL, NULL);
  
  /* note that key validity is unchecked */

  if ( SOURCE_READABLE(source, key, err) )
    {
      g_return_val_if_fail(err == NULL || *err == NULL, NULL);
      return (*source->backend->vtable.query_metainfo)(source, key, err);
    }
  else
    return NULL;
}


/* return value indicates whether the key was writable */
static gboolean
mateconf_source_set_value        (MateConfSource* source,
                               const gchar* key,
                               const MateConfValue* value,
                               GError** err)
{
  g_return_val_if_fail(source != NULL, FALSE);
  g_return_val_if_fail(value != NULL, FALSE);
  g_return_val_if_fail(key != NULL, FALSE);
  g_return_val_if_fail(err == NULL || *err == NULL, FALSE);
  
  /* don't check key validity */

  if ( source_is_writable(source, key, err) )
    {
      g_return_val_if_fail(err == NULL || *err == NULL, FALSE);
      (*source->backend->vtable.set_value)(source, key, value, err);
      return TRUE;
    }
  else
    return FALSE;
}

static gboolean
mateconf_source_unset_value      (MateConfSource* source,
                               const gchar* key,
                               const gchar* locale,
                               GError** err)
{
  g_return_val_if_fail (source != NULL, FALSE);
  g_return_val_if_fail (key != NULL, FALSE);
  g_return_val_if_fail (err == NULL || *err == NULL, FALSE);
  
  if ( source_is_writable(source, key, err) )
    {
      g_return_val_if_fail(err == NULL || *err == NULL, FALSE);

      (*source->backend->vtable.unset_value)(source, key, locale, err);
      return TRUE;
    }
  else
    return FALSE;
}

static GSList*      
mateconf_source_all_entries         (MateConfSource* source,
                                  const gchar* dir,
                                  const gchar** locales,
                                  GError** err)
{
  g_return_val_if_fail(source != NULL, NULL);
  g_return_val_if_fail(dir != NULL, NULL);
  g_return_val_if_fail(err == NULL || *err == NULL, NULL);
  
  if ( SOURCE_READABLE(source, dir, err) )
    {
      g_return_val_if_fail(err == NULL || *err == NULL, NULL);
      return (*source->backend->vtable.all_entries)(source, dir, locales, err);
    }
  else
    return NULL;
}

static GSList*      
mateconf_source_all_dirs          (MateConfSource* source,
                                const gchar* dir,
                                GError** err)
{
  g_return_val_if_fail(source != NULL, NULL);
  g_return_val_if_fail(dir != NULL, NULL);  
  g_return_val_if_fail(err == NULL || *err == NULL, NULL);
  
  if ( SOURCE_READABLE(source, dir, err) )
    {
      g_return_val_if_fail(err == NULL || *err == NULL, NULL);
      return (*source->backend->vtable.all_subdirs)(source, dir, err);
    }
  else
    return NULL;
}

static gboolean
mateconf_source_dir_exists        (MateConfSource* source,
                                const gchar* dir,
                                GError** err)
{
  g_return_val_if_fail(source != NULL, FALSE);
  g_return_val_if_fail(dir != NULL, FALSE);
  g_return_val_if_fail(err == NULL || *err == NULL, FALSE);
  
  if ( SOURCE_READABLE(source, dir, err) )
    {
      g_return_val_if_fail(err == NULL || *err == NULL, FALSE);
      return (*source->backend->vtable.dir_exists)(source, dir, err);
    }
  else
    return FALSE;
}

static void         
mateconf_source_remove_dir        (MateConfSource* source,
                                const gchar* dir,
                                GError** err)
{
  g_return_if_fail(source != NULL);
  g_return_if_fail(dir != NULL);
  g_return_if_fail(err == NULL || *err == NULL);
  
  if ( source_is_writable(source, dir, err) )
    {
      g_return_if_fail(err == NULL || *err == NULL);
      (*source->backend->vtable.remove_dir)(source, dir, err);
    }
}

static gboolean    
mateconf_source_set_schema        (MateConfSource* source,
                                const gchar* key,
                                const gchar* schema_key,
                                GError** err)
{
  g_return_val_if_fail (source != NULL, FALSE);
  g_return_val_if_fail (key != NULL, FALSE);
  g_return_val_if_fail (err == NULL || *err == NULL, FALSE);
  
  if ( source_is_writable(source, key, err) )
    {
      g_return_val_if_fail(err == NULL || *err == NULL, FALSE);
      (*source->backend->vtable.set_schema)(source, key, schema_key, err);
      return TRUE;
    }
  else
    return FALSE;
}

static gboolean
mateconf_source_sync_all         (MateConfSource* source, GError** err)
{
  return (*source->backend->vtable.sync_all)(source, err);
}

static void
mateconf_source_set_notify_func (MateConfSource           *source,
			      MateConfSourceNotifyFunc  notify_func,
			      gpointer               user_data)
{
  g_return_if_fail (source != NULL);

  if (source->backend->vtable.set_notify_func)
    {
      (*source->backend->vtable.set_notify_func) (source, notify_func, user_data);
    }
}

static void
mateconf_source_add_listener (MateConfSource *source,
			   guint        id,
			   const gchar *namespace_section)
{
  g_return_if_fail (source != NULL);
  g_return_if_fail (id > 0);

  if (source->backend->vtable.add_listener)
    {
      (*source->backend->vtable.add_listener) (source, id, namespace_section);
    }
}

static void
mateconf_source_remove_listener (MateConfSource *source,
			      guint        id)
{
  g_return_if_fail (source != NULL);
  g_return_if_fail (id > 0);

  if (source->backend->vtable.remove_listener)
    {
      (*source->backend->vtable.remove_listener) (source, id);
    }
}

/*
 *   Source stacks
 */

MateConfSources* 
mateconf_sources_new_from_addresses(GSList * addresses, GError** err)
{
  MateConfSources *sources;
  GList        *sources_list;

  g_return_val_if_fail( (err == NULL) || (*err == NULL), NULL);

  sources_list = NULL;
  if (addresses != NULL)
    {
      GError *last_error = NULL;

      while (addresses != NULL)
	{
	  MateConfSource* source;

	  if (last_error)
	    {
	      g_error_free (last_error);
	      last_error = NULL;
	    }
      
	  source = mateconf_resolve_address ((const gchar*)addresses->data, &last_error);

	  if (source != NULL)
	    {
	      sources_list = g_list_prepend(sources_list, source);          
	      g_return_val_if_fail(last_error == NULL, NULL);
	    }
	  else
	    {
	      g_assert(last_error != NULL);
	      mateconf_log(GCL_WARNING, _("Failed to load source \"%s\": %s"),
			(const gchar*)addresses->data, last_error->message);
	    }
          
	  addresses = g_slist_next(addresses);
	}

      if (sources_list == NULL)
	{
	  g_assert (last_error != NULL);
	  g_propagate_error (err, last_error);
	  return NULL;
	}

      if (last_error)
	{
	  g_error_free(last_error);
	  last_error = NULL;
	}
    }

  sources          = g_new0 (MateConfSources, 1);
  sources->sources = g_list_reverse (sources_list);

  {
    GList *tmp;
    int i;
    gboolean some_writable;

    some_writable = FALSE;
    i = 0;
    tmp = sources->sources;
    while (tmp != NULL)
      {
        MateConfSource *source = tmp->data;

        if (source->flags & MATECONF_SOURCE_ALL_WRITEABLE)
          {
            some_writable = TRUE;
            mateconf_log (GCL_INFO,
                       _("Resolved address \"%s\" to a writable configuration source at position %d"),
                       source->address, i);
          }
        else if (source->flags & MATECONF_SOURCE_NEVER_WRITEABLE)
          {
            mateconf_log (GCL_INFO,
                       _("Resolved address \"%s\" to a read-only configuration source at position %d"),
                       source->address, i);
          }
        else
          {
            some_writable = TRUE;
            mateconf_log (GCL_INFO,
                       _("Resolved address \"%s\" to a partially writable config source at position %d"),
                       source->address, i);
          }

        ++i;
        tmp = tmp->next;
      }

    if (!some_writable)
      mateconf_log (GCL_WARNING, _("None of the resolved addresses are writable; saving configuration settings will not be possible"));
  }
  
  return sources;
}

MateConfSources*
mateconf_sources_new_from_source       (MateConfSource* source)
{
  MateConfSources* sources;
  
  sources = g_new0(MateConfSources, 1);

  if (source)
    sources->sources = g_list_append(NULL, source);

  return sources;
}

void
mateconf_sources_free(MateConfSources* sources)
{
  GList* tmp;

  tmp = sources->sources;

  while (tmp != NULL)
    {
      mateconf_source_free(tmp->data);
      
      tmp = g_list_next(tmp);
    }

  g_list_free(sources->sources);

  g_free(sources);
}

void
mateconf_sources_clear_cache        (MateConfSources  *sources)
{
  GList* tmp;

  tmp = sources->sources;

  while (tmp != NULL)
    {
      MateConfSource* source = tmp->data;

      if (source->backend->vtable.clear_cache)
        (*source->backend->vtable.clear_cache)(source);
      
      tmp = g_list_next(tmp);
    }
}

MateConfValue*   
mateconf_sources_query_value (MateConfSources* sources, 
                           const gchar* key,
                           const gchar** locales,
                           gboolean use_schema_default,
                           gboolean* value_is_default,
                           gboolean* value_is_writable,
                           gchar   **schema_namep,
                           GError** err)
{
  GList* tmp;
  gchar* schema_name;
  GError* error;
  MateConfValue* val;
  
  g_return_val_if_fail (sources != NULL, NULL);
  g_return_val_if_fail (key != NULL, NULL);
  g_return_val_if_fail ((err == NULL) || (*err == NULL), NULL);

  /* A value is writable if it is unset and a writable source exists,
   * or if it's set and the setting is within or after a writable source.
   * So basically if we see a writable source before we get the value,
   * or get the value from a writable source, the value is writable.
   */
  
  if (!mateconf_key_check(key, err))
    return NULL;

  if (value_is_default)
    *value_is_default = FALSE;

  if (value_is_writable)
    *value_is_writable = FALSE;

  if (schema_namep)
    *schema_namep = NULL;

  val = NULL;
  schema_name = NULL;
  error = NULL;
  
  tmp = sources->sources;

  while (tmp != NULL)
    {
      MateConfSource* source;      
      gchar** schema_name_retloc;

      schema_name_retloc = &schema_name;
      if (schema_name != NULL ||                          /* already have the schema name */
          (schema_namep == NULL && !use_schema_default))  /* don't need to get the schema name */
        schema_name_retloc = NULL;
      
      source = tmp->data;

      if (val == NULL)
        {
          /* A key is writable if the source containing its value or
           * an earlier source is writable
           */          
          if (value_is_writable &&
              source_is_writable (source, key, NULL)) /* ignore errors */
            *value_is_writable = TRUE;
          
          val = mateconf_source_query_value (source, key, locales,
                                          schema_name_retloc, &error);

        }
      else if (schema_name_retloc != NULL)
        {
          MateConfMetaInfo *mi;
          
          mi = mateconf_source_query_metainfo (source, key, &error);
          
          if (mi)
            {
              *schema_name_retloc = mi->schema;
              mi->schema = NULL;
              mateconf_meta_info_free (mi);
            }
        }
          
      if (error != NULL)
        {
          if (err)
            *err = error;
          else
            g_error_free (error);

          error = NULL;

          if (val)
            mateconf_value_free (val);

          if (schema_name)
            g_free (schema_name);
          
          return NULL;
        }

      /* schema_name_retloc == NULL means we aren't still looking for schema name,
       * tmp->next == NULL means we aren't going to get a schema name
       */
      if (val != NULL && (schema_name_retloc == NULL || schema_name != NULL || tmp->next == NULL))
        {
          if (schema_namep)
            *schema_namep = schema_name;
          else
            g_free (schema_name);
          
          return val;
        }
      else
        {
          ; /* Keep looking for either val or schema name */
        }
      
      tmp = g_list_next (tmp);
    }

  g_return_val_if_fail (error == NULL, NULL);
  g_return_val_if_fail (val == NULL, NULL);
  
  /* If we got here, there was no value; we try to look up the
   * schema for this key if we have one, and use the default
   * value.
   */
  
  if (schema_name != NULL)
    {
      /* Note that if the value isn't found, then it's always the
       * default value - even if there is no default value, NULL is
       * the default.  This makes things more sane (I think) because
       * is_default basically means "was set by user" - however we
       * also want to say that if use_schema_default is FALSE then
       * value_is_default will be FALSE so we put this inside the
       * schema_name != NULL conditional
      */
      if (value_is_default)
        *value_is_default = TRUE;

      if (use_schema_default)
        {
          val = mateconf_sources_query_value (sources, schema_name, locales,
                                           FALSE, NULL, NULL, NULL, &error);
        }
      
      if (error != NULL)
        {
          if (err)
            *err = error;
          else
            g_error_free(error);

          g_free(schema_name);
          return NULL;
        }
      else if (val != NULL &&
               val->type != MATECONF_VALUE_SCHEMA)
        {
          mateconf_set_error (err, MATECONF_ERROR_FAILED,
                           _("Schema `%s' specified for `%s' stores a non-schema value"), schema_name, key);

          if (schema_namep)
            *schema_namep = schema_name;
          else
            g_free (schema_name);

          return NULL;
        }
      else if (val != NULL)
        {
          MateConfValue* retval;

          retval = mateconf_schema_steal_default_value (mateconf_value_get_schema (val));          

          mateconf_value_free (val);

          if (schema_namep)
            *schema_namep = schema_name;
          else
            g_free (schema_name);
          
          return retval;
        }
      else
        {
          if (schema_namep)
            *schema_namep = schema_name;
          else
            g_free (schema_name);
          
          return NULL;
        }
    }
  
  return NULL;
}

void
mateconf_sources_set_value   (MateConfSources* sources,
                           const gchar* key,
                           const MateConfValue* value,
			   MateConfSources **modified_sources,
                           GError** err)
{
  GList* tmp;

  g_return_if_fail(sources != NULL);
  g_return_if_fail(key != NULL);
  g_return_if_fail((err == NULL) || (*err == NULL));

  if (modified_sources)
    *modified_sources = NULL;
  
  if (!mateconf_key_check(key, err))
    return;
  
  g_assert(*key != '\0');
  
  if (key[1] == '\0')
    {
      mateconf_set_error(err, MATECONF_ERROR_IS_DIR,
                      _("The '/' name can only be a directory, not a key"));
      return;
    }
  
  tmp = sources->sources;

  while (tmp != NULL)
    {
      MateConfSource* src = tmp->data;

      mateconf_log (GCL_DEBUG, "Setting %s in %s",
                 key, src->address);
      
      /* mateconf_source_set_value return value is whether source
       * was writable at key, not whether err was set.
       */
      if (mateconf_source_set_value (src, key, value, err))
        {
          /* source was writable, err may be set */
          mateconf_log (GCL_DEBUG, "%s was writable in %s", key, src->address);
	  if (modified_sources)
	    {
	      *modified_sources = mateconf_sources_new_from_source (src);
	    }
	  return;
        }
      else
        {
          /* check whether the value is set; if it is, then
             we return an error since setting an overridden value
             would have no effect
          */
          MateConfValue* val;

          val = mateconf_source_query_value(tmp->data, key, NULL, NULL, NULL);
          
          if (val != NULL)
            {
              mateconf_log (GCL_DEBUG, "%s was already set in %s", key, src->address);
              
              mateconf_value_free(val);
              mateconf_set_error(err, MATECONF_ERROR_OVERRIDDEN,
                              _("Value for `%s' set in a read-only source at the front of your configuration path"), key);
              return;
            }
        }

      tmp = g_list_next(tmp);
    }

  /* If we arrived here, then there was nowhere to write a value */
  g_set_error (err,
               MATECONF_ERROR,
               MATECONF_ERROR_NO_WRITABLE_DATABASE,
               _("Unable to store a value at key '%s', as the configuration server has no writable databases. There are some common causes of this problem: 1) your configuration path file %s/path doesn't contain any databases or wasn't found 2) somehow we mistakenly created two mateconfd processes 3) your operating system is misconfigured so NFS file locking doesn't work in your home directory or 4) your NFS client machine crashed and didn't properly notify the server on reboot that file locks should be dropped. If you have two mateconfd processes (or had two at the time the second was launched), logging out, killing all copies of mateconfd, and logging back in may help. If you have stale locks, remove ~/.mateconf*/*lock. Perhaps the problem is that you attempted to use MateConf from two machines at once, and MateCORBA still has its default configuration that prevents remote CORBA connections - put \"ORBIIOPIPv4=1\" in /etc/matecorbarc. As always, check the user.* syslog for details on problems mateconfd encountered. There can only be one mateconfd per home directory, and it must own a lockfile in ~/.mateconfd and also lockfiles in individual storage locations such as ~/.mateconf"),           
               key, MATECONF_CONFDIR);
}

void
mateconf_sources_unset_value   (MateConfSources* sources,
                             const gchar* key,
                             const gchar* locale,
			     MateConfSources **modified_sources,
                             GError** err)
{
  /* We unset in every layer we can write to... */
  GList* tmp;
  GError* error = NULL;
  
  tmp = sources->sources;

  while (tmp != NULL)
    {
      MateConfSource* src = tmp->data;

      if (mateconf_source_unset_value(src, key, locale, &error))
        {
          /* it was writable */

          /* On error, set error and bail */
          if (error != NULL)
            {
              if (err)
                {
                  g_return_if_fail(*err == NULL);
                  *err = error;
                  return;
                }
              else
                {
                  g_error_free(error);
                  return;
                }
            }

	  if (modified_sources)
	    {
	      if (*modified_sources == NULL)
		{
		  *modified_sources = mateconf_sources_new_from_source (src);
		}
	      else
		{
		  (*modified_sources)->sources =
		    g_list_prepend ((*modified_sources)->sources, src);
		}
	    }
        }
      
      tmp = g_list_next(tmp);
    }
}

static GSList *
prepend_unset_notify (GSList       *notifies,
		      MateConfSources *modified_sources,
		      char         *key)
{
  MateConfUnsetNotify *notify;

  g_assert (modified_sources != NULL);
  g_assert (key != NULL);

  notify = g_new0 (MateConfUnsetNotify, 1);

  notify->modified_sources = modified_sources;
  notify->key              = key;

  return g_slist_append (notifies, notify);
}

static void
recursive_unset_helper (MateConfSources   *sources,
                        const char     *key,
                        const char     *locale,
                        MateConfUnsetFlags flags,
                        GSList        **notifies,
                        GError        **first_error)
{
  GError* err = NULL;
  GSList* subdirs;
  GSList* entries;
  GSList* tmp;
  const char *locales[2] = { NULL, NULL };
  MateConfSources* modified_sources;
  MateConfSources** modifiedp = NULL;

  if (notifies)
    {
      modified_sources = NULL;
      modifiedp = &modified_sources;
    }
  
  err = NULL;
  
  subdirs = mateconf_sources_all_dirs (sources, key, &err);
          
  if (subdirs != NULL)
    {
      tmp = subdirs;

      while (tmp != NULL)
        {
          char *s = tmp->data;
          char *full = mateconf_concat_dir_and_key (key, s);
          
          recursive_unset_helper (sources, full, locale, flags,
                                  notifies, first_error);
          
          g_free (s);
          g_free (full);

          tmp = g_slist_next (tmp);
        }

      g_slist_free (subdirs);
    }
  else
    {
      if (err != NULL)
        {
          mateconf_log (GCL_DEBUG, "Error listing subdirs of '%s': %s\n",
                     key, err->message);
          if (*first_error)
            g_error_free (err);
          else
            *first_error = err;
          err = NULL;
        }
    }

  locales[0] = locale;
  entries = mateconf_sources_all_entries (sources, key,
                                       locale ? locales : NULL,
                                       &err);
          
  if (err != NULL)
    {
      mateconf_log (GCL_DEBUG, "Failure listing entries in '%s': %s\n",
                 key, err->message);
      if (*first_error)
        g_error_free (err);
      else
        *first_error = err;
      err = NULL;
    }

  if (entries != NULL)
    {
      tmp = entries;

      while (tmp != NULL)
        {
          MateConfEntry* entry = tmp->data;
          char *full, *freeme;

	  full = freeme = mateconf_concat_dir_and_key (key,
						    mateconf_entry_get_key (entry));
          
          
          mateconf_sources_unset_value (sources, full, locale, modifiedp, &err);
          if (notifies)
	    {
	      *notifies = prepend_unset_notify (*notifies, modified_sources, full);
	      modified_sources = NULL;
	      freeme = NULL;
	    }

          if (err != NULL)
            {
              mateconf_log (GCL_DEBUG, "Error unsetting '%s': %s\n",
                         full, err->message);

              if (*first_error)
                g_error_free (err);
              else
                *first_error = err;
              err = NULL;
            }

          if (flags & MATECONF_UNSET_INCLUDING_SCHEMA_NAMES)
            {
              mateconf_sources_set_schema (sources,
                                        full, NULL,
                                        &err);
              if (err != NULL)
                {
                  mateconf_log (GCL_DEBUG, "Error unsetting schema on '%s': %s\n",
                             full, err->message);
                  
                  if (*first_error)
                    g_error_free (err);
                  else
                    *first_error = err;
                  err = NULL;
                }
            }
          
          mateconf_entry_free (entry);
          g_free (freeme);
          
          tmp = g_slist_next (tmp);
        }

      g_slist_free (entries);
    }

  mateconf_sources_unset_value (sources, key, locale, modifiedp, &err);
  if (notifies)
    {
      *notifies = prepend_unset_notify (*notifies,
					modified_sources,
					g_strdup (key));
      modified_sources = NULL;
    }
  
  if (err != NULL)
    {
      mateconf_log (GCL_DEBUG, "Error unsetting '%s': %s\n",
                 key, err->message);

      if (*first_error)
        g_error_free (err);
      else
        *first_error = err;
      err = NULL;
    }
}

void
mateconf_sources_recursive_unset (MateConfSources   *sources,
                               const gchar    *key,
                               const gchar    *locale,
                               MateConfUnsetFlags flags,
                               GSList        **notifies,
                               GError        **err)
{
  GError *first_error;
  
  g_return_if_fail (sources != NULL);
  g_return_if_fail (key != NULL);
  g_return_if_fail (err == NULL || *err == NULL);

  first_error = NULL;
  recursive_unset_helper (sources, key, locale, flags,
                          notifies, &first_error);

  if (first_error)
    {
      if (notifies != NULL && *notifies != NULL)
	{
	  GSList *tmp;

	  tmp = *notifies;
	  while (tmp != NULL)
	    {
	      MateConfUnsetNotify *notify = tmp->data;

	      g_free (notify->key);
	      g_free (notify);

	      tmp = tmp->next;
	    }

	  g_slist_free (*notifies);
	  *notifies = NULL;
	}

      g_propagate_error (err, first_error);
    }
}

gboolean
mateconf_sources_dir_exists (MateConfSources* sources,
                          const gchar* dir,
                          GError** err)
{
  GList *tmp;

  if (!mateconf_key_check(dir, err))
    return FALSE;
  
  tmp = sources->sources;
  
  while (tmp != NULL) 
    {
      MateConfSource* src = tmp->data;
      
      if (mateconf_source_dir_exists (src, dir, err)) 
        return TRUE;

      tmp = g_list_next(tmp);
    }
  
  return FALSE;
}
          
void          
mateconf_sources_remove_dir (MateConfSources* sources,
                          const gchar* dir,
                          GError** err)
{
  /* We remove in every layer we can write to... */
  GList* tmp;
  
  if (!mateconf_key_check(dir, err))
    return;
  
  tmp = sources->sources;

  while (tmp != NULL)
    {
      MateConfSource* src = tmp->data;
      GError* error = NULL;
      
      mateconf_source_remove_dir(src, dir, &error);

      /* On error, set error and bail */
      if (error != NULL)
        {
          if (err)
            {
              g_return_if_fail(*err == NULL);
              *err = error;
              return;
            }
          else
            {
              g_error_free(error);
              return;
            }
        }
      
      tmp = g_list_next(tmp);
    }
}

void         
mateconf_sources_set_schema (MateConfSources *sources,
                          const gchar  *key,
                          const gchar  *schema_key,
                          GError      **err)
{
  GList* tmp;

  if (!mateconf_key_check (key, err))
    return;

  if (schema_key && !mateconf_key_check (schema_key, err))
    return;
  
  tmp = sources->sources;

  while (tmp != NULL)
    {
      MateConfSource* src = tmp->data;

      /* may set error, we just leave its setting */
      /* returns TRUE if the source was writable */
      if (mateconf_source_set_schema (src, key, schema_key, err))
        return;

      tmp = g_list_next(tmp);
    }
}

/* God, this is depressingly inefficient. Maybe there's a nicer way to
   implement it... */
/* Then we have to ship it all to the app via CORBA... */
/* Anyway, we use a hash to be sure we have a single value for 
   each key in the directory, and we always take that value from
   the first source that had one set. When we're done we flatten
   the hash.
*/
static void
hash_listify_func(gpointer key, gpointer value, gpointer user_data)
{
  GSList** list_p = user_data;

  *list_p = g_slist_prepend(*list_p, value);
}

static void
hash_destroy_entries_func(gpointer key, gpointer value, gpointer user_data)
{
  MateConfEntry* entry;

  entry = value;

  mateconf_entry_free(entry);
}

static void
hash_destroy_pointers_func(gpointer key, gpointer value, gpointer user_data)
{
  g_free(value);
}

struct DefaultsLookupData {
  MateConfSources* sources;
  const gchar** locales;
};

static void
hash_lookup_defaults_func(gpointer key, gpointer value, gpointer user_data)
{
  MateConfEntry *entry = value;
  struct DefaultsLookupData* dld = user_data;
  MateConfSources *sources = dld->sources;
  const gchar** locales = dld->locales;
  
  if (mateconf_entry_get_value(entry) == NULL)
    {
      if (mateconf_entry_get_schema_name(entry) != NULL)
        {
          MateConfValue *val;


          val = mateconf_sources_query_value(sources,
                                          mateconf_entry_get_schema_name(entry),
                                          locales,
                                          TRUE,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL);

          if (val != NULL &&
              val->type == MATECONF_VALUE_SCHEMA)
            {
              MateConfValue* defval;

              defval = mateconf_schema_steal_default_value (mateconf_value_get_schema(val));

              mateconf_entry_set_value_nocopy (entry, defval);
              mateconf_entry_set_is_default (entry, TRUE);
            }

          if (val)
            mateconf_value_free(val);
        }
    }
}


static gboolean
key_is_writable (MateConfSources *sources,
                 MateConfSource  *value_in_src,
                 const gchar *key,
                 GError **err)
{
  GList *tmp;
  
  tmp = sources->sources;

  while (tmp != NULL)
    {
      MateConfSource* src;
      
      src = tmp->data;
      
      if (source_is_writable (src, key, NULL))
        return TRUE;

      if (src == value_in_src)
        return FALSE; /* didn't find a writable source before value-containing
                         source.
                      */
      
      tmp = g_list_next (tmp);
    }

  /* This shouldn't be reached actually */
  return FALSE;
}

GSList*       
mateconf_sources_all_entries   (MateConfSources* sources,
                             const gchar* dir,
                             const gchar** locales,
                             GError** err)
{
  GList* tmp;
  GHashTable* hash;
  GSList* flattened;
  gboolean first_pass = TRUE; /* as an optimization, don't bother
                                 doing hash lookups on first source
                              */
  struct DefaultsLookupData dld = { NULL, NULL };
  
  dld.sources = sources;
  dld.locales = locales;

  /* Empty MateConfSources, skip it */
  if (sources->sources == NULL)
    return NULL;

  hash = g_hash_table_new(g_str_hash, g_str_equal);

  tmp = sources->sources;

  while (tmp != NULL)
    {
      MateConfSource* src;
      GSList* pairs;
      GSList* iter;
      GError* error = NULL;
      
      src   = tmp->data;
      pairs = mateconf_source_all_entries(src, dir, locales, &error);
      iter  = pairs;
      
      /* On error, set error and bail */
      if (error != NULL)
        {
          g_hash_table_foreach(hash, hash_destroy_entries_func, NULL);
          
          g_hash_table_destroy(hash);
          
          if (err)
            {
              g_return_val_if_fail(*err == NULL, NULL);
              *err = error;
              return NULL;
            }
          else
            {
              g_error_free(error);
              return NULL;
            }
        }

      /* Iterate over the list of entries, stuffing them in the hash
         and setting their writability flag if they're new
      */
      
      while (iter != NULL)
        {
          MateConfEntry* pair = iter->data;
          MateConfEntry* previous;
          gchar *full;
          
          if (first_pass)
            previous = NULL; /* Can't possibly be there. */
          else
            previous = g_hash_table_lookup(hash, pair->key);
          
          if (previous != NULL)
            {
              if (mateconf_entry_get_value (previous) != NULL)
                /* Discard this latest one */
                ;
              else
                {
                  /* Save the new value, previously we had an entry but no value */
                  mateconf_entry_set_value_nocopy (previous,
                                                mateconf_entry_steal_value(pair));

                  /* As an efficiency hack, remember that
                   * entry->key is relative not absolute on the
                   * mateconfd side
                   */
                  full = mateconf_concat_dir_and_key (dir, previous->key);

                  mateconf_entry_set_is_writable (previous,
                                               key_is_writable (sources,
                                                                src,
                                                                full,
                                                                NULL));

                  g_free (full);
                }
              
              if (mateconf_entry_get_schema_name (previous) != NULL)
                /* Discard this latest one */
                ;
              else
                {
                  /* Save the new schema name, previously we had an entry but no schema name*/
                  if (mateconf_entry_get_schema_name (pair) != NULL)
                    {
                      mateconf_entry_set_schema_name (previous,
                                                   mateconf_entry_get_schema_name (pair));
                    }
                }

              mateconf_entry_free(pair);
            }
          else
            {
              /* Save */
              g_hash_table_insert(hash, pair->key, pair);
              
              /* As an efficiency hack, remember that
               * entry->key is relative not absolute on the
               * mateconfd side
               */
              full = mateconf_concat_dir_and_key (dir, pair->key);

              mateconf_entry_set_is_writable (pair,
                                           key_is_writable (sources,
                                                            src,
                                                            full,
                                                            NULL));
              
              g_free (full);
            }

          iter = g_slist_next(iter);
        }
      
      /* All pairs are either stored or destroyed. */
      g_slist_free(pairs);

      first_pass = FALSE;

      tmp = g_list_next(tmp);
    }

  flattened = NULL;

  g_hash_table_foreach(hash, hash_lookup_defaults_func, &dld);
  
  g_hash_table_foreach(hash, hash_listify_func, &flattened);

  g_hash_table_destroy(hash);
  
  return flattened;
}

GSList*       
mateconf_sources_all_dirs   (MateConfSources* sources,
                          const gchar* dir,
                          GError** err)
{
  GList* tmp = NULL;
  GHashTable* hash = NULL;
  GSList* flattened = NULL;
  gboolean first_pass = TRUE; /* as an optimization, don't bother
                                 doing hash lookups on first source
                              */

  g_return_val_if_fail(sources != NULL, NULL);
  g_return_val_if_fail(dir != NULL, NULL);

  /* As another optimization, skip the whole 
     hash thing if there's only zero or one sources
  */
  if (sources->sources == NULL)
    return NULL;

  if (sources->sources->next == NULL)
    {
      return mateconf_source_all_dirs (sources->sources->data, dir, err);
    }

  /* 2 or more sources */
  g_assert(g_list_length(sources->sources) > 1);

  hash = g_hash_table_new(g_str_hash, g_str_equal);

  tmp = sources->sources;

  while (tmp != NULL)
    {
      MateConfSource* src;
      GSList* subdirs;
      GSList* iter;
      GError* error = NULL;
      
      src     = tmp->data;
      subdirs = mateconf_source_all_dirs(src, dir, &error);
      iter    = subdirs;

      /* On error, set error and bail */
      if (error != NULL)
        {
          g_hash_table_foreach (hash, hash_destroy_pointers_func, NULL);
          
          g_hash_table_destroy (hash);
          
          if (err)
            {
              g_return_val_if_fail(*err == NULL, NULL);
              *err = error;
              return NULL;
            }
          else
            {
              g_error_free(error);
              return NULL;
            }
        }
      
      while (iter != NULL)
        {
          gchar* subdir = iter->data;
          gchar* previous;
          
          if (first_pass)
            previous = NULL; /* Can't possibly be there. */
          else
            previous = g_hash_table_lookup(hash, subdir);
          
          if (previous != NULL)
            {
              /* Discard */
              g_free(subdir);
            }
          else
            {
              /* Save */
              g_hash_table_insert(hash, subdir, subdir);
            }

          iter = g_slist_next(iter);
        }
      
      /* All pairs are either stored or destroyed. */
      g_slist_free(subdirs);

      first_pass = FALSE;

      tmp = g_list_next(tmp);
    }

  flattened = NULL;

  g_hash_table_foreach(hash, hash_listify_func, &flattened);

  g_hash_table_destroy(hash);

  return flattened;
}

gboolean
mateconf_sources_sync_all    (MateConfSources* sources, GError** err)
{
  GList* tmp;
  gboolean failed = FALSE;
  GError* all_errors = NULL;
  
  tmp = sources->sources;

  while (tmp != NULL)
    {
      MateConfSource* src = tmp->data;
      GError* error = NULL;
      
      if (!mateconf_source_sync_all(src, &error))
        {
          failed = TRUE;
          g_assert(error != NULL);
        }
          
      /* On error, set error and bail */
      if (error != NULL)
        {
          if (err)
            all_errors = mateconf_compose_errors(all_errors, error);

          g_error_free(error);
        }
          
      tmp = g_list_next(tmp);
    }

  if (err)
    {
      g_return_val_if_fail(*err == NULL, !failed);
      *err = all_errors;
    }
  
  return !failed;
}

MateConfMetaInfo*
mateconf_sources_query_metainfo (MateConfSources* sources,
                              const gchar* key,
                              GError** err)
{
  GList* tmp;
  MateConfMetaInfo* mi = NULL;
  
  tmp = sources->sources;

  while (tmp != NULL)
    {
      MateConfSource* src = tmp->data;
      GError* error = NULL;
      MateConfMetaInfo* this_mi;

      this_mi = mateconf_source_query_metainfo(src, key, &error);
      
      /* On error, just keep going, log the error maybe. */
      if (error != NULL)
        {
          g_assert(this_mi == NULL);
          mateconf_log(GCL_ERR, _("Error finding metainfo: %s"), error->message);
          g_error_free(error);
          error = NULL;
        }

      if (this_mi != NULL)
        {
          if (mi == NULL)
            mi = this_mi;
          else
            {
              /* Fill in any missing fields of "mi" found in "this_mi",
                 and pick the most recent mod time */
              if (mateconf_meta_info_get_schema(mi) == NULL &&
                  mateconf_meta_info_get_schema(this_mi) != NULL)
                {
                  mateconf_meta_info_set_schema(mi,
                                             mateconf_meta_info_get_schema(this_mi));
                }

              if (mateconf_meta_info_get_mod_user(mi) == NULL &&
                  mateconf_meta_info_get_mod_user(this_mi) != NULL)
                {
                  mateconf_meta_info_set_mod_user(mi,
                                               mateconf_meta_info_get_mod_user(this_mi));
                }
              
              if (mateconf_meta_info_mod_time(mi) < mateconf_meta_info_mod_time(this_mi))
                {
                  mateconf_meta_info_set_mod_time(mi,
                                               mateconf_meta_info_mod_time(this_mi));
                }

              mateconf_meta_info_free(this_mi);
            }
        }
      
      tmp = g_list_next(tmp);
    }
  
  return mi;
}

MateConfValue*
mateconf_sources_query_default_value(MateConfSources* sources,
                                  const gchar* key,
                                  const gchar** locales,
                                  gboolean* is_writable,
                                  GError** err)
{
  GError* error = NULL;
  MateConfValue* val;
  MateConfMetaInfo* mi;
  
  g_return_val_if_fail(err == NULL || *err == NULL, NULL);

  if (is_writable)
    *is_writable = key_is_writable (sources, NULL, key, NULL);
  
  mi = mateconf_sources_query_metainfo(sources, key,
                                    &error);
  if (mi == NULL)
    {
      if (error != NULL)
        {
          if (err)
            *err = error;
          else
            {
              mateconf_log(GCL_ERR, _("Error getting metainfo: %s"), error->message);
              g_error_free(error);
            }
        }
      return NULL;
    }

  if (mateconf_meta_info_get_schema(mi) == NULL)
    {
      mateconf_meta_info_free(mi);
      return NULL;
    }
      
  val = mateconf_sources_query_value(sources,
                                  mateconf_meta_info_get_schema(mi), locales,
                                  TRUE, NULL, NULL, NULL, &error);
  
  if (val != NULL)
    {
      MateConfSchema* schema;

      if (val->type != MATECONF_VALUE_SCHEMA)
        {
          mateconf_log(GCL_WARNING,
                    _("Key `%s' listed as schema for key `%s' actually stores type `%s'"),
                    mateconf_meta_info_get_schema(mi),
                    key,
                    mateconf_value_type_to_string(val->type));

          mateconf_meta_info_free(mi);
          return NULL;
        }

      mateconf_meta_info_free(mi);

      schema = mateconf_value_steal_schema (val);
      mateconf_value_free (val);
      
      if (schema != NULL)
        {
          MateConfValue* retval;

          retval = mateconf_schema_steal_default_value (schema);

          mateconf_schema_free (schema);
          
          return retval;
        }
      return NULL;
    }
  else
    {
      if (error != NULL)
        {
          if (err)
            *err = error;
          else
            {
              mateconf_log(GCL_ERR, _("Error getting value for `%s': %s"),
                        mateconf_meta_info_get_schema(mi),
                        error->message);
              g_error_free(error);
            }
        }
      
      mateconf_meta_info_free(mi);
      
      return NULL;
    }
}

void
mateconf_sources_set_notify_func (MateConfSources          *sources,
			       MateConfSourceNotifyFunc  notify_func,
			       gpointer               user_data)
{
  GList *tmp;

  tmp = sources->sources;
  while (tmp != NULL)
    {
      mateconf_source_set_notify_func (tmp->data, notify_func, user_data);

      tmp = tmp->next;
    }
}

void
mateconf_sources_add_listener (MateConfSources *sources,
			    guint         id,
			    const gchar  *namespace_section)
{
  GList *tmp;

  tmp = sources->sources;
  while (tmp != NULL)
    {
      mateconf_source_add_listener (tmp->data, id, namespace_section);

      tmp = tmp->next;
    }
}

void
mateconf_sources_remove_listener (MateConfSources *sources,
			       guint         id)
{
  GList *tmp;

  tmp = sources->sources;
  while (tmp != NULL)
    {
      mateconf_source_remove_listener (tmp->data, id);

      tmp = tmp->next;
    }
}

/* Non-allocating variant of mateconf_address_resource()
 */
static const char *
get_address_resource (const char *address)
{
  const char *start;

  g_return_val_if_fail (address != NULL, NULL);

  start = strchr (address, ':');
  if (start != NULL)
    {
      start = strchr (++start, ':');

      if (start != NULL)
        start++;
    }

  return start;
}

/* Return TRUE if
 *  1. @sources contains @modified_src and
 *  2. @key is not set in any source above @modified_src.
 */
gboolean
mateconf_sources_is_affected (MateConfSources *sources,
                           MateConfSource  *modified_src,
                           const char   *key)
{
  const char *modified_resource;
  GList      *tmp;

  modified_resource = get_address_resource (modified_src->address);

  tmp = sources->sources;
  while (tmp != NULL)
    {
      MateConfSource *source = tmp->data;

      if (source->backend == modified_src->backend &&
          strcmp (modified_resource, get_address_resource (source->address)) == 0)
        break;

      tmp = tmp->next;
    }

  if (tmp == NULL)
    return FALSE;
  
  tmp = tmp->prev;
  while (tmp != NULL)
    {
      MateConfValue *val;
      
      val = mateconf_source_query_value (tmp->data, key, NULL, NULL, NULL);
      if (val != NULL)
	{
	  mateconf_value_free (val);
	  return FALSE;
	}
      
      tmp = tmp->prev;
    }
  
  return TRUE;
}
