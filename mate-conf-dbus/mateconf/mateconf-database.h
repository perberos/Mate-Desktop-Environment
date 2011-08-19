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

#ifndef MATECONF_MATECONF_DATABASE_H
#define MATECONF_MATECONF_DATABASE_H

#include <glib.h>

G_BEGIN_DECLS

#include "mateconf-error.h"
/*#include "MateConfX.h"*/
#include "mateconf-listeners.h"
#include "mateconf-sources.h"
#include "mateconf-internals.h"
#include "mateconf-locale.h"

#include <dbus/dbus.h>

typedef struct _MateConfDatabase MateConfDatabase;

struct _MateConfDatabase
{
#ifdef HAVE_MATECORBA
  /* "inherit" from the servant,
     must be first in struct */
  POA_ConfigDatabase3 servant;
  ConfigDatabase objref;
#endif

  /* D-Bus specific parts: */
  char           *object_path;
  
  /* Information about clients that want notification. */
  GHashTable     *notifications;
  GHashTable     *listening_clients;
  /* End of D-Bus stuff. */
	
  MateConfListeners* listeners;
  MateConfSources* sources;

  GTime last_access;
  guint sync_idle;
  guint sync_timeout;

  gchar *persistent_name;
};

MateConfDatabase* mateconf_database_new  (MateConfSources  *sources);
void           mateconf_database_free (MateConfDatabase *db);

void                mateconf_database_drop_dead_listeners (MateConfDatabase *db);

#ifdef HAVE_MATECORBA
CORBA_unsigned_long mateconf_database_add_listener     (MateConfDatabase       *db,
                                                     ConfigListener       who,
                                                     const char          *name,
                                                     const gchar         *where);
void                mateconf_database_remove_listener  (MateConfDatabase       *db,
                                                     CORBA_unsigned_long  cnxn);

CORBA_unsigned_long mateconf_database_readd_listener   (MateConfDatabase       *db,
                                                     ConfigListener       who,
                                                     const char          *name,
                                                     const gchar         *where);
void                mateconf_database_notify_listeners (MateConfDatabase       *db,
                                                     MateConfSources        *modified_sources,
                                                     const gchar         *key,
                                                     const ConfigValue   *value,
                                                     gboolean             is_default,
                                                     gboolean             is_writable,
                                                     gboolean             notify_others);
#endif

MateConfValue* mateconf_database_query_value         (MateConfDatabase  *db,
                                                const gchar    *key,
                                                const gchar   **locales,
                                                gboolean        use_schema_default,
                                                gchar         **schema_name,
                                                gboolean       *value_is_default,
                                                gboolean       *value_is_writable,
                                                GError    **err);
MateConfValue* mateconf_database_query_default_value (MateConfDatabase  *db,
                                                const gchar    *key,
                                                const gchar   **locales,
                                                gboolean       *is_writable,
                                                GError    **err);



void mateconf_database_set   (MateConfDatabase      *db,
                           const gchar        *key,
                           MateConfValue         *value,
                           GError        **err);
void mateconf_database_unset (MateConfDatabase      *db,
                           const gchar        *key,
                           const gchar        *locale,
                           GError        **err);

void mateconf_database_recursive_unset (MateConfDatabase      *db,
                                     const gchar        *key,
                                     const gchar        *locale,
                                     MateConfUnsetFlags     flags,
                                     GError            **err);


gboolean mateconf_database_dir_exists  (MateConfDatabase  *db,
                                     const gchar    *dir,
                                     GError    **err);
void     mateconf_database_remove_dir  (MateConfDatabase  *db,
                                     const gchar    *dir,
                                     GError    **err);
GSList*  mateconf_database_all_entries (MateConfDatabase  *db,
                                     const gchar    *dir,
                                     const gchar   **locales,
                                     GError    **err);
GSList*  mateconf_database_all_dirs    (MateConfDatabase  *db,
                                     const gchar    *dir,
                                     GError    **err);
void     mateconf_database_set_schema  (MateConfDatabase  *db,
                                     const gchar    *key,
                                     const gchar    *schema_key,
                                     GError    **err);


void     mateconf_database_sync             (MateConfDatabase  *db,
                                          GError    **err);
gboolean mateconf_database_synchronous_sync (MateConfDatabase  *db,
                                          GError    **err);
void     mateconf_database_clear_cache      (MateConfDatabase  *db,
                                          GError    **err);

MateConfLocaleList* mateconfd_locale_cache_lookup(const gchar* locale);
void mateconfd_locale_cache_expire (void);
void mateconfd_locale_cache_drop  (void);

const gchar* mateconf_database_get_persistent_name (MateConfDatabase *db);

#ifdef HAVE_CORBA
void mateconf_database_log_listeners_to_string (MateConfDatabase *db,
                                             gboolean is_default,
                                             GString *str);
#endif

G_END_DECLS

#endif



