
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

#ifndef MATECONF_MATECONF_VALUE_H
#define MATECONF_MATECONF_VALUE_H

#include <glib.h>
#include "mateconf-error.h"

G_BEGIN_DECLS

/* 
 * A MateConfValue is used to pass configuration values around
 */

typedef enum {
  MATECONF_VALUE_INVALID,
  MATECONF_VALUE_STRING,
  MATECONF_VALUE_INT,
  MATECONF_VALUE_FLOAT,
  MATECONF_VALUE_BOOL,
  MATECONF_VALUE_SCHEMA,

  /* unfortunately these aren't really types; we want list_of_string,
     list_of_int, etc.  but it's just too complicated to implement.
     instead we'll complain in various places if you do something
     moronic like mix types in a list or treat pair<string,int> and
     pair<float,bool> as the same type. */
  MATECONF_VALUE_LIST,
  MATECONF_VALUE_PAIR
  
} MateConfValueType;

#define MATECONF_VALUE_TYPE_VALID(x) (((x) > MATECONF_VALUE_INVALID) && ((x) <= MATECONF_VALUE_PAIR))

/* Forward declaration */
typedef struct _MateConfSchema MateConfSchema;

typedef struct _MateConfValue MateConfValue;

struct _MateConfValue {
  MateConfValueType type;
};

const char*    mateconf_value_get_string    (const MateConfValue *value);
int            mateconf_value_get_int       (const MateConfValue *value);
double         mateconf_value_get_float     (const MateConfValue *value);
MateConfValueType mateconf_value_get_list_type (const MateConfValue *value);
GSList*        mateconf_value_get_list      (const MateConfValue *value);
MateConfValue*    mateconf_value_get_car       (const MateConfValue *value);
MateConfValue*    mateconf_value_get_cdr       (const MateConfValue *value);
gboolean       mateconf_value_get_bool      (const MateConfValue *value);
MateConfSchema*   mateconf_value_get_schema    (const MateConfValue *value);

MateConfValue* mateconf_value_new                  (MateConfValueType type);

/* doesn't work on complicated types (only string, int, bool, float) */
MateConfValue* mateconf_value_new_from_string      (MateConfValueType type,
                                              const gchar* str,
                                              GError** err);

MateConfValue* mateconf_value_copy                 (const MateConfValue* src);
void        mateconf_value_free                 (MateConfValue* value);

void        mateconf_value_set_int              (MateConfValue* value,
                                              gint the_int);
void        mateconf_value_set_string           (MateConfValue* value,
                                              const gchar* the_str);
void        mateconf_value_set_float            (MateConfValue* value,
                                              gdouble the_float);
void        mateconf_value_set_bool             (MateConfValue* value,
                                              gboolean the_bool);
void        mateconf_value_set_schema           (MateConfValue* value,
                                              const MateConfSchema* sc);
void        mateconf_value_set_schema_nocopy    (MateConfValue* value,
                                              MateConfSchema* sc);
void        mateconf_value_set_car              (MateConfValue* value,
                                              const MateConfValue* car);
void        mateconf_value_set_car_nocopy       (MateConfValue* value,
                                              MateConfValue* car);
void        mateconf_value_set_cdr              (MateConfValue* value,
                                              const MateConfValue* cdr);
void        mateconf_value_set_cdr_nocopy       (MateConfValue* value,
                                              MateConfValue* cdr);
/* Set a list of MateConfValue, NOT lists or pairs */
void        mateconf_value_set_list_type        (MateConfValue* value,
                                              MateConfValueType type);
void        mateconf_value_set_list_nocopy      (MateConfValue* value,
                                              GSList* list);
void        mateconf_value_set_list             (MateConfValue* value,
                                              GSList* list);

gchar*      mateconf_value_to_string            (const MateConfValue* value);

int         mateconf_value_compare              (const MateConfValue* value_a,
                                              const MateConfValue* value_b);

/* Meta-information about a key. Not the same as a schema; this is
 * information stored on the key, the schema is a specification
 * that may apply to this key.
 */

/* FIXME MateConfMetaInfo is basically deprecated in favor of stuffing this
 * info into MateConfEntry, though the transition isn't complete.
 */

typedef struct _MateConfMetaInfo MateConfMetaInfo;

struct _MateConfMetaInfo {
  gchar* schema;
  gchar* mod_user; /* user owning the daemon that made the last modification */
  GTime  mod_time; /* time of the modification */
};

const char* mateconf_meta_info_get_schema   (MateConfMetaInfo *gcmi);
const char* mateconf_meta_info_get_mod_user (MateConfMetaInfo *gcmi);
GTime       mateconf_meta_info_mod_time     (MateConfMetaInfo *gcmi);

MateConfMetaInfo* mateconf_meta_info_new          (void);
void           mateconf_meta_info_free         (MateConfMetaInfo *gcmi);
void           mateconf_meta_info_set_schema   (MateConfMetaInfo *gcmi,
                                             const gchar   *schema_name);
void           mateconf_meta_info_set_mod_user (MateConfMetaInfo *gcmi,
                                             const gchar   *mod_user);
void           mateconf_meta_info_set_mod_time (MateConfMetaInfo *gcmi,
                                             GTime          mod_time);



/* Key-value pairs; used to list the contents of
 *  a directory
 */  

typedef enum
{
  MATECONF_UNSET_INCLUDING_SCHEMA_NAMES = 1 << 0
} MateConfUnsetFlags;

typedef struct _MateConfEntry MateConfEntry;

struct _MateConfEntry {
  char *key;
  MateConfValue *value;
};

const char* mateconf_entry_get_key         (const MateConfEntry *entry);
MateConfValue* mateconf_entry_get_value       (const MateConfEntry *entry);
const char* mateconf_entry_get_schema_name (const MateConfEntry *entry);
gboolean    mateconf_entry_get_is_default  (const MateConfEntry *entry);
gboolean    mateconf_entry_get_is_writable (const MateConfEntry *entry);

MateConfEntry* mateconf_entry_new              (const gchar *key,
                                          const MateConfValue  *val);
MateConfEntry* mateconf_entry_new_nocopy       (gchar       *key,
                                          MateConfValue  *val);

MateConfEntry* mateconf_entry_copy             (const MateConfEntry *src);
#ifndef MATECONF_DISABLE_DEPRECATED
void        mateconf_entry_free             (MateConfEntry  *entry);
#endif
void        mateconf_entry_ref   (MateConfEntry *entry);
void        mateconf_entry_unref (MateConfEntry *entry);

/* Transfer ownership of value to the caller. */
MateConfValue* mateconf_entry_steal_value      (MateConfEntry  *entry);
void        mateconf_entry_set_value        (MateConfEntry  *entry,
                                          const MateConfValue  *val);
void        mateconf_entry_set_value_nocopy (MateConfEntry  *entry,
                                          MateConfValue  *val);
void        mateconf_entry_set_schema_name  (MateConfEntry  *entry,
                                          const gchar *name);
void        mateconf_entry_set_is_default   (MateConfEntry  *entry,
                                          gboolean     is_default);
void        mateconf_entry_set_is_writable  (MateConfEntry  *entry,
                                          gboolean     is_writable);

gboolean    mateconf_entry_equal            (const MateConfEntry *a,
                                          const MateConfEntry *b);

G_END_DECLS

#endif


