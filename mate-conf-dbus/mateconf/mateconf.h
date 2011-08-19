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

#ifndef MATECONF_MATECONF_H
#define MATECONF_MATECONF_H

#include <glib.h>

G_BEGIN_DECLS

#include <mateconf/mateconf-schema.h>
#include <mateconf/mateconf-engine.h>
#include <mateconf/mateconf-error.h>
#include <mateconf/mateconf-enum-types.h>

typedef void (*MateConfNotifyFunc) (MateConfEngine* conf,
                                 guint cnxn_id,
                                 MateConfEntry *entry,
                                 gpointer user_data);
  
/* Returns ID of the notification */
/* returns 0 on error, 0 is an invalid ID */
guint mateconf_engine_notify_add    (MateConfEngine      *conf,
                                  /* dir or key to listen to */
                                  const gchar      *namespace_section,
                                  MateConfNotifyFunc   func,
                                  gpointer          user_data,
                                  GError      **err);

void  mateconf_engine_notify_remove (MateConfEngine      *conf,
                                  guint             cnxn);



/* Low-level interfaces */
MateConfValue* mateconf_engine_get                     (MateConfEngine  *conf,
                                                  const gchar  *key,
                                                  GError  **err);

MateConfValue* mateconf_engine_get_without_default     (MateConfEngine  *conf,
                                                  const gchar  *key,
                                                  GError  **err);

MateConfEntry* mateconf_engine_get_entry                (MateConfEngine  *conf,
                                                   const gchar  *key,
                                                   const gchar  *locale,
                                                   gboolean      use_schema_default,
                                                   GError  **err);


/* Locale only matters if you are expecting to get a schema, or if you
   don't know what you are expecting and it might be a schema. Note
   that mateconf_engine_get () automatically uses the current locale, which is
   normally what you want. */
MateConfValue* mateconf_engine_get_with_locale         (MateConfEngine  *conf,
                                                  const gchar  *key,
                                                  const gchar  *locale,
                                                  GError  **err);


/* Get the default value stored in the schema associated with this key */
MateConfValue* mateconf_engine_get_default_from_schema (MateConfEngine  *conf,
                                                  const gchar  *key,
                                                  GError  **err);
gboolean    mateconf_engine_set                     (MateConfEngine  *conf,
                                                  const gchar  *key,
                                                  const MateConfValue *value,
                                                  GError  **err);
gboolean    mateconf_engine_unset                   (MateConfEngine  *conf,
                                                  const gchar  *key,
                                                  GError  **err);


/*
 * schema_key should have a schema (if key stores a value) or a dir
 * full of schemas (if key stores a directory name)
 */

gboolean mateconf_engine_associate_schema (MateConfEngine  *conf,
                                        const gchar  *key,
                                        const gchar  *schema_key,
                                        GError  **err);
GSList*  mateconf_engine_all_entries      (MateConfEngine  *conf,
                                        const gchar  *dir,
                                        GError  **err);
GSList*  mateconf_engine_all_dirs         (MateConfEngine  *conf,
                                        const gchar  *dir,
                                        GError  **err);
void     mateconf_engine_suggest_sync     (MateConfEngine  *conf,
                                        GError  **err);
gboolean mateconf_engine_dir_exists       (MateConfEngine  *conf,
                                        const gchar  *dir,
                                        GError  **err);
void     mateconf_engine_remove_dir       (MateConfEngine* conf,
                                        const gchar* dir,
                                        GError** err);

gboolean mateconf_engine_key_is_writable  (MateConfEngine *conf,
                                        const gchar *key,
                                        GError     **err);

/* if you pass non-NULL for why_invalid, it gives a user-readable
   explanation of the problem in g_malloc()'d memory
*/
gboolean mateconf_valid_key          (const gchar  *key,
                                   gchar       **why_invalid);


/* return TRUE if the path "below" would be somewhere below the directory "above" */
gboolean mateconf_key_is_below       (const gchar  *above,
                                   const gchar  *below);


/* Returns allocated concatenation of these two */
gchar*   mateconf_concat_dir_and_key (const gchar  *dir,
                                   const gchar  *key);


/* Returns a different string every time (at least, the chances of
   getting a duplicate are like one in a zillion). The key is a
   legal mateconf key name (a single element of one) */
gchar*   mateconf_unique_key         (void);

/* Escape/unescape a string to create a valid key */
char*    mateconf_escape_key         (const char *arbitrary_text,
                                   int         len);
char*    mateconf_unescape_key       (const char *escaped_key,
                                   int         len);


/* 
 * Higher-level stuff 
 */


gdouble      mateconf_engine_get_float  (MateConfEngine     *conf,
                                      const gchar     *key,
                                      GError     **err);
gint         mateconf_engine_get_int    (MateConfEngine     *conf,
                                      const gchar     *key,
                                      GError     **err);


/* free the retval, retval can be NULL for "unset" */
gchar*       mateconf_engine_get_string (MateConfEngine     *conf,
                                      const gchar     *key,
                                      GError     **err);
gboolean     mateconf_engine_get_bool   (MateConfEngine     *conf,
                                      const gchar     *key,
                                      GError     **err);


/* this one has no default since it would be expensive and make little
   sense; it returns NULL as a default, to indicate unset or error */
/* free the retval */
/* Note that this returns the schema stored at key, NOT
   the schema associated with the key. */
MateConfSchema* mateconf_engine_get_schema (MateConfEngine     *conf,
                                      const gchar     *key,
                                      GError     **err);


/*
  This automatically converts the list to the given list type;
  a list of int or bool stores values in the list->data field
  using GPOINTER_TO_INT(), a list of strings stores the gchar*
  in list->data, a list of float contains pointers to allocated
  gdouble (gotta love C!).
*/
GSList*      mateconf_engine_get_list   (MateConfEngine     *conf,
                                      const gchar     *key,
                                      MateConfValueType   list_type,
                                      GError     **err);

/*
  The car_retloc and cdr_retloc args should be the address of the appropriate
  type:
  bool    gboolean*
  int     gint*
  string  gchar**
  float   gdouble*
  schema  MateConfSchema**
*/
gboolean     mateconf_engine_get_pair   (MateConfEngine     *conf,
                                      const gchar     *key,
                                      MateConfValueType   car_type,
                                      MateConfValueType   cdr_type,
                                      gpointer         car_retloc,
                                      gpointer         cdr_retloc,
                                      GError     **err);


/* setters return TRUE on success; note that you still should suggest a sync */
gboolean mateconf_engine_set_float  (MateConfEngine     *conf,
                                  const gchar     *key,
                                  gdouble          val,
                                  GError     **err);
gboolean mateconf_engine_set_int    (MateConfEngine     *conf,
                                  const gchar     *key,
                                  gint             val,
                                  GError     **err);
gboolean mateconf_engine_set_string (MateConfEngine     *conf,
                                  const gchar     *key,
                                  const gchar     *val,
                                  GError     **err);
gboolean mateconf_engine_set_bool   (MateConfEngine     *conf,
                                  const gchar     *key,
                                  gboolean         val,
                                  GError     **err);
gboolean mateconf_engine_set_schema (MateConfEngine     *conf,
                                  const gchar     *key,
                                  const MateConfSchema     *val,
                                  GError     **err);


/* List should be the same as the one mateconf_engine_get_list() would return */
gboolean mateconf_engine_set_list   (MateConfEngine     *conf,
                                  const gchar     *key,
                                  MateConfValueType   list_type,
                                  GSList          *list,
                                  GError     **err);
gboolean mateconf_engine_set_pair   (MateConfEngine     *conf,
                                  const gchar     *key,
                                  MateConfValueType   car_type,
                                  MateConfValueType   cdr_type,
                                  gconstpointer    address_of_car,
                                  gconstpointer    address_of_cdr,
                                  GError     **err);


/* Utility function converts enumerations to and from strings */
typedef struct _MateConfEnumStringPair MateConfEnumStringPair;

struct _MateConfEnumStringPair {
  gint enum_value;
  const gchar* str;
};

gboolean     mateconf_string_to_enum (MateConfEnumStringPair  lookup_table[],
                                   const gchar         *str,
                                   gint                *enum_value_retloc);
const gchar* mateconf_enum_to_string (MateConfEnumStringPair  lookup_table[],
                                   gint                 enum_value);

int mateconf_debug_shutdown (void);

#ifndef MATECONF_DISABLE_DEPRECATED
gboolean     mateconf_init           (int argc, char **argv, GError** err);
gboolean     mateconf_is_initialized (void);
#endif /* MATECONF_DISABLE_DEPRECATED */

/* No, you can't use this stuff. Bad application developer. Bad. */
#ifdef MATECONF_ENABLE_INTERNALS

/* This stuff is only useful in MATE 2.0, so isn't in this MateConf
 * release.
 */

#ifndef MATECONF_DISABLE_DEPRECATED
/* For use by the Mate module system */
void mateconf_preinit(gpointer app, gpointer mod_info);
void mateconf_postinit(gpointer app, gpointer mod_info);

extern const char mateconf_version[];

#ifdef HAVE_POPT_H
#include <popt.h>
#endif

#ifdef POPT_AUTOHELP
/* If people are using popt, then make the table available to them */
extern struct poptOption mateconf_options[];
#endif
#endif /* MATECONF_DISABLE_DEPRECATED */

void mateconf_clear_cache(MateConfEngine* conf, GError** err);
void mateconf_synchronous_sync(MateConfEngine* conf, GError** err);

MateConfValue * mateconf_engine_get_full (MateConfEngine *conf,
                                    const gchar *key,
                                    const gchar *locale,
                                    gboolean use_schema_default,
                                    gboolean *is_default_p,
                                    gboolean *is_writable_p,
                                    GError **err);

#endif /* MATECONF_ENABLE_INTERNALS */

G_END_DECLS

#endif
