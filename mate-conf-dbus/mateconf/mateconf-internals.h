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

#ifndef MATECONF_MATECONF_INTERNALS_H
#define MATECONF_MATECONF_INTERNALS_H

#ifndef MATECONF_ENABLE_INTERNALS
#error "you are trying to use MateConf internal functions outside of MateConf. This is a Bad Idea, the ABI for these internals is not fixed"
#endif

#ifdef MATECONF_ENABLE_INTERNALS

#include <config.h>
#include <libintl.h>

#define _(String) dgettext (GETTEXT_PACKAGE, String)
#define N_(String) (String)

#include <glib.h>
#include <glib/gstdio.h>
#include "mateconf-error.h"
#include "mateconf-value.h"
#include "mateconf-engine.h"
#include "mateconf-sources.h"
/*#include "MateConfX.h"*/

#ifdef G_OS_WIN32

#define DEV_NULL "NUL:"

#include <sys/stat.h>

#ifndef S_IRWXU
#define S_IRWXU (_S_IREAD|_S_IWRITE|_S_IEXEC)
#endif
#ifndef S_IRWXG
#define S_IRWXG (S_IRWXU >> 3)
#endif
#ifndef S_IRWXO
#define S_IRWXO (S_IRWXU >> 6)
#endif

#undef MATECONF_LOCALE_DIR
const char *_mateconf_win32_get_locale_dir (void) G_GNUC_CONST;
#define MATECONF_LOCALE_DIR _mateconf_win32_get_locale_dir ()

#undef MATECONF_CONFDIR
const char *_mateconf_win32_get_confdir (void) G_GNUC_CONST;
#define MATECONF_CONFDIR _mateconf_win32_get_confdir ()

#undef MATECONF_ETCDIR
const char *_mateconf_win32_get_etcdir (void) G_GNUC_CONST;
#define MATECONF_ETCDIR _mateconf_win32_get_etcdir ()

#undef MATECONF_SERVERDIR
const char *_mateconf_win32_get_serverdir (void) G_GNUC_CONST;
#define MATECONF_SERVERDIR _mateconf_win32_get_serverdir ()

#undef MATECONF_BACKEND_DIR
const char *_mateconf_win32_get_backend_dir (void) G_GNUC_CONST;
#define MATECONF_BACKEND_DIR _mateconf_win32_get_backend_dir ()

char *_mateconf_win32_replace_prefix (const char *configure_time_path);
const char *_mateconf_win32_get_home_dir (void);

#else

#define DEV_NULL "/dev/null"

#endif

#define MATECONF_DATABASE_LIST_DELIM ';'

gchar*       mateconf_key_directory  (const gchar* key);
const gchar* mateconf_key_key        (const gchar* key);

/* These file tests are in libmate, I cut-and-pasted them */
enum {
  MATECONF_FILE_EXISTS=(1<<0)|(1<<1)|(1<<2), /*any type of file*/
  MATECONF_FILE_ISFILE=1<<0,
  MATECONF_FILE_ISLINK=1<<1,
  MATECONF_FILE_ISDIR=1<<2
};

gboolean mateconf_file_test   (const gchar* filename, int test);
gboolean mateconf_file_exists (const gchar* filename);

#ifdef HAVE_CORBA
MateConfValue*  mateconf_value_from_corba_value            (const ConfigValue *value);
ConfigValue* mateconf_corba_value_from_mateconf_value      (const MateConfValue  *value);
void         mateconf_fill_corba_value_from_mateconf_value (const MateConfValue  *value,
                                                      ConfigValue       *dest);
ConfigValue* mateconf_invalid_corba_value               (void);

void          mateconf_fill_corba_schema_from_mateconf_schema (const MateConfSchema  *sc,
                                                         ConfigSchema       *dest);
ConfigSchema* mateconf_corba_schema_from_mateconf_schema      (const MateConfSchema  *sc);
MateConfSchema*  mateconf_schema_from_corba_schema            (const ConfigSchema *cs);

gchar* mateconf_object_to_string (CORBA_Object obj,
                               GError **err);
#endif

char   *mateconf_address_list_get_persistent_name (GSList     *addresses);
GSList *mateconf_persistent_name_get_address_list (const char *persistent_name);
void    mateconf_address_list_free                (GSList     *addresses);

const gchar*   mateconf_value_type_to_string   (MateConfValueType  type);
MateConfValueType mateconf_value_type_from_string (const gchar    *str);


GSList*       mateconf_load_source_path (const gchar* filename, GError** err);

/* shouldn't be used in applications (although implemented in mateconf.c) */

void     mateconf_shutdown_daemon (GError **err);
gboolean mateconf_ping_daemon     (void);
gboolean mateconf_spawn_daemon    (GError **err);
#ifdef HAVE_CORBA
int      mateconf_orb_release     (void);
#endif

/* Returns 0 on failure (or if the string is "0" of course) */
gulong       mateconf_string_to_gulong (const gchar *str);
gboolean     mateconf_string_to_double (const gchar *str,
                                     gdouble     *val);
gchar*       mateconf_double_to_string (gdouble      val);
const gchar* mateconf_current_locale   (void);


/* Log wrapper; we might want to not use syslog someday */
typedef enum {
  GCL_EMERG,
  GCL_ALERT,
  GCL_CRIT,
  GCL_ERR,
  GCL_WARNING,
  GCL_NOTICE,
  GCL_INFO,
  GCL_DEBUG
} MateConfLogPriority;

void          mateconf_log      (MateConfLogPriority pri, const gchar* format, ...) G_GNUC_PRINTF (2, 3);

extern gboolean mateconf_log_debug_messages;

/* return FALSE and set error if the key is bad */
gboolean      mateconf_key_check(const gchar* key, GError** err);

/*
 * If these were public they'd be in mateconf-value.h
 */

/* for the complicated types */
MateConfValue* mateconf_value_new_list_from_string (MateConfValueType list_type,
                                              const gchar* str,
					      GError** err);
MateConfValue* mateconf_value_new_pair_from_string (MateConfValueType car_type,
                                              MateConfValueType cdr_type,
                                              const gchar* str,
					      GError** err);

GSList*      mateconf_value_steal_list   (MateConfValue *value);
MateConfSchema* mateconf_value_steal_schema (MateConfValue *value);
char*        mateconf_value_steal_string (MateConfValue *value);

/* These are a hack to encode values into strings and ship them over CORBA,
 * necessary for obscure reasons (MateCORBA doesn't like recursive datatypes yet)
 */

/* string quoting is only public for the benefit of the test suite */

gchar* mateconf_quote_string           (const gchar  *str);
gchar* mateconf_unquote_string         (const gchar  *str,
                                     const gchar **end,
                                     GError      **err);
void   mateconf_unquote_string_inplace (gchar        *str,
                                     gchar       **end,
                                     GError      **err);

MateConfValue* mateconf_value_decode (const gchar *encoded);
gchar*      mateconf_value_encode (MateConfValue  *val);

/*
 * List/pair conversion stuff
 */

MateConfValue* mateconf_value_list_from_primitive_list (MateConfValueType  list_type,
                                                  GSList         *list,
                                                  GError        **err);
MateConfValue* mateconf_value_pair_from_primitive_pair (MateConfValueType  car_type,
                                                  MateConfValueType  cdr_type,
                                                  gconstpointer   address_of_car,
                                                  gconstpointer   address_of_cdr,
                                                  GError        **err);

GSList*  mateconf_value_list_to_primitive_list_destructive (MateConfValue      *val,
                                                         MateConfValueType   list_type,
                                                         GError         **err);
gboolean mateconf_value_pair_to_primitive_pair_destructive (MateConfValue      *val,
                                                         MateConfValueType   car_type,
                                                         MateConfValueType   cdr_type,
                                                         gpointer         car_retloc,
                                                         gpointer         cdr_retloc,
                                                         GError         **err);


void         mateconf_set_daemon_mode (gboolean     setting);
gboolean     mateconf_in_daemon_mode  (void);
void         mateconf_set_daemon_ior  (const gchar *ior);
const gchar* mateconf_get_daemon_ior  (void);

/* Returns TRUE if there was an error, frees exception, sets err */
#ifdef HAVE_CORBA
gboolean mateconf_handle_oaf_exception (CORBA_Environment* ev, GError** err);
#endif

void mateconf_nanosleep (gulong useconds);

typedef struct _MateConfLock MateConfLock;

MateConfLock* mateconf_get_lock     (const gchar  *lock_directory,
                               GError      **err);
gboolean   mateconf_release_lock (MateConfLock    *lock,
                               GError      **err);
#ifdef HAVE_CORBA
MateConfLock* mateconf_get_lock_or_current_holder (const gchar  *lock_directory,
                                             ConfigServer *current_server,
                                             GError      **err);
ConfigServer mateconf_get_current_lock_holder  (const gchar *lock_directory,
                                             GString     *failure_log);

void mateconf_daemon_blow_away_locks (void);
#endif

GError*  mateconf_error_new  (MateConfError en,
                           const gchar* format, ...) G_GNUC_PRINTF (2, 3);

void     mateconf_set_error  (GError** err,
                           MateConfError en,
                           const gchar* format, ...) G_GNUC_PRINTF (3, 4);

/* merge two errors into a single message */
GError*  mateconf_compose_errors (GError* err1, GError* err2);

#ifdef HAVE_CORBA
CORBA_ORB mateconf_orb_get (void);

ConfigServer mateconf_activate_server (gboolean  start_if_not_found,
				    GError  **error);

char*     mateconf_get_lock_dir (void);
char*     mateconf_get_daemon_dir (void);
#endif

gboolean mateconf_schema_validate (const MateConfSchema  *sc,
                                GError            **err);
gboolean mateconf_value_validate  (const MateConfValue   *value,
                                GError            **err);


void mateconf_engine_set_owner        (MateConfEngine *engine,
                                    gpointer     client);
void mateconf_engine_push_owner_usage (MateConfEngine *engine,
                                    gpointer     client);
void mateconf_engine_pop_owner_usage  (MateConfEngine *engine,
                                    gpointer     client);

gboolean mateconf_engine_recursive_unset (MateConfEngine      *engine,
                                       const char       *key,
                                       MateConfUnsetFlags   flags,
                                       GError          **err);

#ifdef HAVE_CORBA
gboolean mateconf_CORBA_Object_equal (gconstpointer a,
                                   gconstpointer b);
guint    mateconf_CORBA_Object_hash  (gconstpointer key);
#endif

MateConfValue* mateconf_schema_steal_default_value (MateConfSchema *schema);

void mateconf_value_set_string_nocopy (MateConfValue *value,
                                    char       *str);

void _mateconf_init_i18n (void);

gboolean mateconf_use_local_locks (void);

#endif /* MATECONF_ENABLE_INTERNALS */

#endif /* MATECONF_MATECONF_INTERNALS_H */
