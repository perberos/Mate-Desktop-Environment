
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

#ifndef MATECONF_MATECONF_SOURCES_H
#define MATECONF_MATECONF_SOURCES_H

#include <glib.h>
#include "mateconf-error.h"
#include "mateconf-value.h"
#include "mateconf-listeners.h"

/* Sources are not interchangeable; different backend engines will return 
 * MateConfSource with different private elements.
 */

typedef struct _MateConfBackend MateConfBackend;

typedef struct _MateConfSource MateConfSource;

struct _MateConfSource {
  guint flags;
  gchar* address;
  MateConfBackend* backend;
};

typedef enum {
  /* These are an optimization to avoid calls to
   * the writable/readable methods in the backend
   * vtable
   */
  MATECONF_SOURCE_ALL_WRITEABLE = 1 << 0,
  MATECONF_SOURCE_ALL_READABLE = 1 << 1,
  MATECONF_SOURCE_NEVER_WRITEABLE = 1 << 2, 
  MATECONF_SOURCE_ALL_FLAGS = ((1 << 0) | (1 << 1))
} MateConfSourceFlags;

typedef void (* MateConfSourceNotifyFunc) (MateConfSource *source,
					const gchar *location,
					gpointer     user_data);

MateConfSource*  mateconf_resolve_address         (const gchar* address,
                                             GError** err);

void          mateconf_source_free          (MateConfSource* source);

/* This is the actual thing we want to talk to, the stack of sources */
typedef struct _MateConfSources MateConfSources;

struct _MateConfSources {
  GList* sources;
};

typedef struct
{
  MateConfSources *modified_sources;
  char         *key;
} MateConfUnsetNotify;


/* Even on error, this gives you an empty source list, i.e.  never
   returns NULL but may set the error if some addresses weren't
   resolved and may contain no sources.  */
MateConfSources* mateconf_sources_new_from_addresses (GSList* addresses,
                                                GError   **err);
MateConfSources* mateconf_sources_new_from_source    (MateConfSource   *source);
void          mateconf_sources_free               (MateConfSources  *sources);
void          mateconf_sources_clear_cache        (MateConfSources  *sources);
MateConfValue*   mateconf_sources_query_value        (MateConfSources  *sources,
                                                const gchar   *key,
                                                const gchar  **locales,
                                                gboolean       use_schema_default,
                                                gboolean      *value_is_default,
                                                gboolean      *value_is_writable,
                                                gchar        **schema_name,
                                                GError   **err);
void          mateconf_sources_set_value          (MateConfSources  *sources,
                                                const gchar   *key,
                                                const MateConfValue *value,
						MateConfSources **modified_sources,
                                                GError   **err);
void          mateconf_sources_unset_value        (MateConfSources  *sources,
                                                const gchar   *key,
                                                const gchar   *locale,
						MateConfSources **modified_sources,
                                                GError   **err);
void          mateconf_sources_recursive_unset    (MateConfSources  *sources,
                                                const gchar   *key,
                                                const gchar   *locale,
                                                MateConfUnsetFlags flags,
                                                GSList      **notifies,
                                                GError      **err);
GSList*       mateconf_sources_all_entries        (MateConfSources  *sources,
                                                const gchar   *dir,
                                                const gchar  **locales,
                                                GError   **err);
GSList*       mateconf_sources_all_dirs           (MateConfSources  *sources,
                                                const gchar   *dir,
                                                GError   **err);
gboolean      mateconf_sources_dir_exists         (MateConfSources  *sources,
                                                const gchar   *dir,
                                                GError   **err);
void          mateconf_sources_remove_dir         (MateConfSources  *sources,
                                                const gchar   *dir,
                                                GError   **err);
void          mateconf_sources_set_schema         (MateConfSources  *sources,
                                                const gchar   *key,
                                                const gchar   *schema_key,
                                                GError   **err);
gboolean      mateconf_sources_sync_all           (MateConfSources  *sources,
                                                GError   **err);


MateConfMetaInfo*mateconf_sources_query_metainfo     (MateConfSources* sources,
                                                const gchar* key,
                                                GError** err);

MateConfValue*   mateconf_sources_query_default_value(MateConfSources* sources,
                                                const gchar* key,
                                                const gchar** locales,
                                                gboolean* is_writable,
                                                GError** err);

void          mateconf_sources_set_notify_func    (MateConfSources          *sources,
					        MateConfSourceNotifyFunc  notify_func,
					        gpointer               user_data);
void          mateconf_sources_add_listener       (MateConfSources          *sources,
						guint                  id,
					        const gchar           *location);
void          mateconf_sources_remove_listener    (MateConfSources          *sources,
						guint                  id);

gboolean      mateconf_sources_is_affected        (MateConfSources *sources,
						MateConfSource  *modified_src,
						const char   *key);

#endif
