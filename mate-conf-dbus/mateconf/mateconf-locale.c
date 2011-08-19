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

#include "mateconf-locale.h"
#include "mateconf-internals.h"
#include <sys/time.h>
#include <time.h>
#include <string.h>

static void
mateconf_locale_cache_add (MateConfLocaleCache* cache,
                        const gchar* locale);

static MateConfLocaleList*
mateconf_locale_list_new (const gchar* locale);

typedef struct _MateConfLocaleListPrivate MateConfLocaleListPrivate;

struct _MateConfLocaleListPrivate {
  gchar** list;
  guint refcount;
};

typedef struct _Entry Entry;

struct _Entry {
  gchar* locale;
  MateConfLocaleList* list;
  GTime mod_time;
};

struct _MateConfLocaleCache {
  GHashTable* hash;
};

MateConfLocaleCache*
mateconf_locale_cache_new (void)
{
  MateConfLocaleCache* cache;

  cache = g_new(MateConfLocaleCache, 1);

  cache->hash = g_hash_table_new(g_str_hash, g_str_equal);

  return cache;
}

void
mateconf_locale_cache_free (MateConfLocaleCache* cache)
{
  mateconf_locale_cache_expire (cache, 0);

  g_assert(g_hash_table_size(cache->hash) == 0);

  g_hash_table_destroy(cache->hash);

  g_free(cache);
}

static void
mateconf_locale_cache_add (MateConfLocaleCache* cache,
                        const gchar* locale)
{
  Entry* e;

  e = g_new(Entry, 1);

  e->locale = g_strdup(locale);
  e->list = mateconf_locale_list_new(locale);
  e->mod_time = time(NULL);
  
  g_hash_table_insert (cache->hash, e->locale, e);
}

typedef struct _ExpireData ExpireData;

struct _ExpireData {
  GTime now;
  guint max_age;
};

static gboolean
expire_foreach(const gchar* key,
               Entry* e, ExpireData* ed)
{
  GTime last_access = e->mod_time;

  /* MUST be >= so max_age of 0 will remove everything */
  if ((ed->now - last_access) >= ed->max_age)
    {
      mateconf_locale_list_unref(e->list);
      g_free(e->locale);
      g_free(e);
      return TRUE;
    }
  else
    return FALSE;
}

void
mateconf_locale_cache_expire (MateConfLocaleCache* cache,
                           guint max_age_exclusive_in_seconds)
{
  ExpireData ed = { 0, 0 };
  ed.max_age = max_age_exclusive_in_seconds;

  ed.now = time(NULL);
  
  g_hash_table_foreach_remove(cache->hash, (GHRFunc)expire_foreach,
                              &ed);
}

void
mateconf_locale_list_ref (MateConfLocaleList* list)
{
  MateConfLocaleListPrivate* priv;

  priv = (MateConfLocaleListPrivate*) list;

  priv->refcount += 1;
}

void
mateconf_locale_list_unref (MateConfLocaleList* list)
{
  MateConfLocaleListPrivate* priv;

  priv = (MateConfLocaleListPrivate*) list;

  g_return_if_fail(priv->refcount > 0);

  priv->refcount -= 1;

  if (priv->refcount == 0)
    {
      g_strfreev(priv->list);
      g_free(list);
    }
}

MateConfLocaleList*
mateconf_locale_cache_get_list (MateConfLocaleCache* cache,
                             const gchar* locale)
{
  Entry* e;

  if (locale == NULL)
    locale = "C";
  
  e = g_hash_table_lookup(cache->hash, locale);

  if (e != NULL)
    {
      mateconf_locale_list_ref(e->list);
      return e->list;
    }
  else
    {
      mateconf_locale_cache_add(cache, locale);

      e = g_hash_table_lookup(cache->hash, locale);

      g_assert(e != NULL);

      mateconf_locale_list_ref(e->list);

      return e->list;
    }
}


/*
 * Big mess o' cut-and-pasted code
 */

/* --------------------------------------------------------------- */

/* Mask for components of locale spec. The ordering here is from
 * least significant to most significant
 */
enum
{
  COMPONENT_CODESET =   1 << 0,
  COMPONENT_TERRITORY = 1 << 1,
  COMPONENT_MODIFIER =  1 << 2
};

/* Break an X/Open style locale specification into components
 */
static guint
explode_locale (const gchar *locale,
		gchar **language, 
		gchar **territory, 
		gchar **codeset, 
		gchar **modifier)
{
  const gchar *uscore_pos;
  const gchar *at_pos;
  const gchar *dot_pos;

  guint mask = 0;

  uscore_pos = strchr (locale, '_');
  dot_pos = strchr (uscore_pos ? uscore_pos : locale, '.');
  at_pos = strchr (dot_pos ? dot_pos : (uscore_pos ? uscore_pos : locale), '@');

  if (at_pos)
    {
      mask |= COMPONENT_MODIFIER;
      *modifier = g_strdup (at_pos);
    }
  else
    at_pos = locale + strlen (locale);

  if (dot_pos)
    {
      mask |= COMPONENT_CODESET;
      *codeset = g_new (gchar, 1 + at_pos - dot_pos);
      strncpy (*codeset, dot_pos, at_pos - dot_pos);
      (*codeset)[at_pos - dot_pos] = '\0';
    }
  else
    dot_pos = at_pos;

  if (uscore_pos)
    {
      mask |= COMPONENT_TERRITORY;
      *territory = g_new (gchar, 1 + dot_pos - uscore_pos);
      strncpy (*territory, uscore_pos, dot_pos - uscore_pos);
      (*territory)[dot_pos - uscore_pos] = '\0';
    }
  else
    uscore_pos = dot_pos;

  *language = g_new (gchar, 1 + uscore_pos - locale);
  strncpy (*language, locale, uscore_pos - locale);
  (*language)[uscore_pos - locale] = '\0';

  return mask;
}

/*
 * Compute all interesting variants for a given locale name -
 * by stripping off different components of the value.
 *
 * For simplicity, we assume that the locale is in
 * X/Open format: language[_territory][.codeset][@modifier]
 *
 * TODO: Extend this to handle the CEN format (see the GNUlibc docs)
 *       as well. We could just copy the code from glibc wholesale
 *       but it is big, ugly, and complicated, so I'm reluctant
 *       to do so when this should handle 99% of the time...
 */
static GSList *
compute_locale_variants (const gchar *locale)
{
  GSList *retval = NULL;

  gchar *language;
  gchar *territory;
  gchar *codeset;
  gchar *modifier;

  guint mask;
  guint i;

  g_return_val_if_fail (locale != NULL, NULL);

  mask = explode_locale (locale, &language, &territory, &codeset, &modifier);

  /* Iterate through all possible combinations, from least attractive
   * to most attractive.
   */
  for (i=0; i<=mask; i++)
    if ((i & ~mask) == 0)
      {
	gchar *val = g_strconcat(language,
				 (i & COMPONENT_TERRITORY) ? territory : "",
				 (i & COMPONENT_CODESET) ? codeset : "",
				 (i & COMPONENT_MODIFIER) ? modifier : "",
				 NULL);
	retval = g_slist_prepend (retval, val);
      }

  g_free (language);
  if (mask & COMPONENT_CODESET)
    g_free (codeset);
  if (mask & COMPONENT_TERRITORY)
    g_free (territory);
  if (mask & COMPONENT_MODIFIER)
    g_free (modifier);

  return retval;
}

/* -------------------------------------------------------------- */

/*
 * End of big mess o' cut and pasted code
 */


static MateConfLocaleList*
mateconf_locale_list_new (const gchar* locale)
{
  MateConfLocaleListPrivate* priv;
  
  priv = g_new(MateConfLocaleListPrivate, 1);
  
  priv->refcount = 1;
  priv->list = mateconf_split_locale(locale);
  
  return (MateConfLocaleList*) priv;
}

gchar**
mateconf_split_locale               (const gchar* locale)
{
  gchar** vector = NULL;
  GSList* list = NULL;
  gint c_locale_defined = FALSE;
  gchar *category_memory, *orig_category_memory;
  
  if (locale == NULL)
    locale = "C";

  /* For each locale name in the colon-separated string,
     extract all its variants */
  
  orig_category_memory = category_memory = g_malloc (strlen (locale)+1);
  
  while (locale[0] != '\0')
    {
      while (locale[0] != '\0' && locale[0] == ':')
        ++locale;
      
      if (locale[0] != '\0')
        {
          char *cp = category_memory;
          
          while (locale[0] != '\0' && locale[0] != ':')
            *category_memory++= *locale++;
          
          category_memory[0]= '\0'; 
          category_memory++;
          
          if (strcmp (cp, "C") == 0)
            c_locale_defined= TRUE;
          
          list = g_slist_concat (list, compute_locale_variants (cp));
        }
    }
  
  g_free (orig_category_memory);
  
  if (!c_locale_defined)
    list = g_slist_append (list, g_strdup ("C"));

  {
    /* Convert list to a string vector */
    guint n;
    guint i;
    GSList* tmp;
    
    n = g_slist_length(list);

    g_assert(n > 0);
    
    vector = g_new0(gchar*, n + 2);

    i = 0;
    tmp = list;

    while (tmp != NULL)
      {
        vector[i] = tmp->data;

        ++i;
        tmp = g_slist_next(tmp);
      }
    
    g_slist_free(list);
  }

  return vector;
}
