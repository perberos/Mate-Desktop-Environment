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

#ifndef MATECONF_MATECONF_LOCALE_H
#define MATECONF_MATECONF_LOCALE_H

#include <glib.h>

G_BEGIN_DECLS

/*
 * This thing caches the fallback list for a locale string.
 */
typedef struct _MateConfLocaleCache MateConfLocaleCache;

MateConfLocaleCache* mateconf_locale_cache_new           (void);
void              mateconf_locale_cache_free       (MateConfLocaleCache* cache);
void              mateconf_locale_cache_expire        (MateConfLocaleCache* cache,
                                                    /* >= max_age is deleted */
                                                    guint max_age_exclusive_in_seconds);


/* This API is annoying, but hey it's all MateConf internal. No users will see it.
   We need it for thread safety without the penalty of copying a string vector
   every time we get the fallback list.
*/

typedef struct _MateConfLocaleList MateConfLocaleList;

struct _MateConfLocaleList {
  const gchar** list;
};

void              mateconf_locale_list_ref            (MateConfLocaleList* list);
void              mateconf_locale_list_unref          (MateConfLocaleList* list);

/* for thread safety, this is going to increment the reference count
   on the locale list. You must unref() when done. This automatically
   adds a cache entry if necessary. locale may be NULL, in which case
   it just gets converted to "C"
*/
MateConfLocaleList*  mateconf_locale_cache_get_list      (MateConfLocaleCache* cache,
                                                    const gchar* locale);

/* Use this if you don't care about the locale cache */
gchar**           mateconf_split_locale               (const gchar* locale);

G_END_DECLS

#endif



