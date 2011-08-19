
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

#ifndef MATECONF_MATECONFBACKEND_H
#define MATECONF_MATECONFBACKEND_H

#include <mateconf/mateconf-internals.h>
#include <gmodule.h>
#include <mateconf/mateconf-sources.h>

/*
 * This vtable is more complicated than strictly necessary, hoping
 * that backends can be smart and optimize some calls
 */

typedef struct _MateConfBackendVTable MateConfBackendVTable;

struct _MateConfBackendVTable {
  /* Set to sizeof (MateConfBackendVTable) - used for future proofing */
  gsize                  vtable_size;

  void                (* shutdown)        (GError** err);

  MateConfSource*        (* resolve_address) (const gchar* address,
                                           GError** err);

  /* Thread locks. If the backend is thread-safe, then these
   * can be NULL. If per-source locks are needed, then these
   * calls should lock a mutex stored in the MateConfSource.
   * If a per-backend lock is needed, then the calls can ignore
   * their source argument and lock the whole backend.
   */
  void                (* lock)            (MateConfSource* source,
                                           GError** err);

  void                (* unlock)          (MateConfSource* source,
                                           GError** err);

  /* Report whether a given key (and its subkeys) can be read/written.
   * Sources may not permit reading/writing from/to /foo but forbid
   * writing to /foo/bar; if a key can be read or written then its
   * subkeys may also be read/written.
   *
   * This field allows backends to be configured so that they only
   * store certain kinds of data in certain sections of the MateConf
   * namespace.
   *
   * If these functions return an error, they MUST return FALSE as
   * well.
   */

  gboolean           (* readable)         (MateConfSource* source,
                                           const gchar* key,
                                           GError** err);

  gboolean           (* writable)        (MateConfSource* source,
                                           const gchar* key,
                                           GError** err);
  
  /* schema_name filled if NULL or MATECONF_VALUE_IGNORE_SUBSEQUENT returned.
     if schema_name is NULL, it isn't filled */
  MateConfValue*         (* query_value)     (MateConfSource* source, 
                                           const gchar* key,
                                           const gchar** locales,
                                           gchar** schema_name,
                                           GError** err);
  
  MateConfMetaInfo*      (* query_metainfo)  (MateConfSource* source,
                                           const gchar* key,
                                           GError** err);
  
  void                (* set_value)       (MateConfSource* source, 
                                           const gchar* key, 
                                           const MateConfValue* value,
                                           GError** err);

  /* Returns list of MateConfEntry with key set to a relative
   * pathname. In the public client-side API the key
   * is always absolute though.
   */
  GSList*             (* all_entries)     (MateConfSource* source,
                                           const gchar* dir,
                                           const gchar** locales,
                                           GError** err);

  /* Returns list of allocated strings, relative names */
  GSList*             (* all_subdirs)     (MateConfSource* source,
                                           const gchar* dir,
                                           GError** err);

  void                (* unset_value)     (MateConfSource* source,
                                           const gchar* key,
                                           const gchar* locale,
                                           GError** err);

  gboolean            (* dir_exists)      (MateConfSource* source,
                                           const gchar* dir,
                                           GError** err);
        
  void                (* remove_dir)      (MateConfSource* source,
                                           const gchar* dir,
                                           GError** err);
  
  void                (* set_schema)      (MateConfSource* source,
                                           const gchar* key,
                                           const gchar* schema_key,
                                           GError** err);

  gboolean            (* sync_all)        (MateConfSource* source,
                                           GError** err);

  void                (* destroy_source)  (MateConfSource* source);

  /* This is basically used by the test suite */
  void                (* clear_cache)     (MateConfSource* source);

  /* used by mateconf-sanity-check */
  void                (* blow_away_locks) (const char *address);

  void                (* set_notify_func) (MateConfSource           *source,
					   MateConfSourceNotifyFunc  notify_func,
					   gpointer               user_data);

  void                (* add_listener)    (MateConfSource           *source,
					   guint                  id,
					   const gchar           *namespace_section);

  void                (* remove_listener) (MateConfSource           *source,
					   guint                  id);
};

struct _MateConfBackend {
  const gchar* name;
  guint refcount;
  MateConfBackendVTable vtable;
  GModule* module;
};

/* Address utility stuff */

/* Get the backend name */
gchar*        mateconf_address_backend(const gchar* address);
/* Get the resource name understood only by the backend */
gchar*        mateconf_address_resource(const gchar* address);
/* Get the backend flags */
gchar**       mateconf_address_flags(const gchar* address);

gchar*        mateconf_backend_file(const gchar* address);

/* Obtain the MateConfBackend for this address, based on the first part of the 
 * address. The refcount is always incremented, and you must unref() later.
 * The same backend may be returned by multiple calls to this routine,
 * but you can assume they are different if you do the refcounting
 * right. Returns NULL if it fails.
 */
MateConfBackend* mateconf_get_backend(const gchar* address, GError** err);

void          mateconf_backend_ref(MateConfBackend* backend);
void          mateconf_backend_unref(MateConfBackend* backend);

MateConfSource*  mateconf_backend_resolve_address (MateConfBackend* backend, 
                                             const gchar* address,
                                             GError** err);

void          mateconf_blow_away_locks       (const gchar* address);

#endif
