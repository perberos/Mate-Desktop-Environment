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

#include "mateconf-changeset.h"
#include "mateconf-internals.h"

typedef enum {
  CHANGE_INVALID,
  CHANGE_SET,
  CHANGE_UNSET
} ChangeType;

typedef struct _Change Change;

struct _Change {
  gchar* key;
  ChangeType type;
  MateConfValue* value;
};

static Change* change_new    (const gchar* key);
static void    change_set    (Change* c, MateConfValue* value);
static void    change_unset  (Change* c);
static void    change_destroy(Change* c);

struct _MateConfChangeSet {
  guint refcount;
  GHashTable* hash;
  gint in_foreach;
  gpointer user_data;
  GDestroyNotify dnotify;
};

GType
mateconf_change_set_get_type (void)
{
  static GType our_type = 0;

  if (our_type == 0)
    our_type = g_boxed_type_register_static ("MateConfChangeSet",
					     (GBoxedCopyFunc) mateconf_change_set_ref,
					     (GBoxedFreeFunc) mateconf_change_set_unref);

  return our_type;
}

MateConfChangeSet*
mateconf_change_set_new      (void)
{
  MateConfChangeSet* cs;

  cs = g_new(MateConfChangeSet, 1);

  cs->refcount = 1;
  cs->hash = g_hash_table_new(g_str_hash, g_str_equal);
  cs->in_foreach = 0;
  cs->user_data = NULL;
  cs->dnotify = NULL;
  
  return cs;
}

void
mateconf_change_set_ref      (MateConfChangeSet* cs)
{
  g_return_if_fail(cs != NULL);
  
  cs->refcount += 1;
}

void
mateconf_change_set_unref    (MateConfChangeSet* cs)
{
  g_return_if_fail(cs != NULL);
  g_return_if_fail(cs->refcount > 0);

  cs->refcount -= 1;

  if (cs->refcount == 0)
    {
      if (cs->in_foreach > 0)
        g_warning("MateConfChangeSet refcount reduced to 0 during a foreach");
      
      mateconf_change_set_clear(cs);

      g_hash_table_destroy(cs->hash);
      
      g_free(cs);
    }
}

void
mateconf_change_set_set_user_data (MateConfChangeSet *cs,
                                gpointer        data,
                                GDestroyNotify  dnotify)
{
  if (cs->dnotify)
    (* cs->dnotify) (cs->user_data);

  cs->user_data = data;
  cs->dnotify = dnotify;
}

gpointer
mateconf_change_set_get_user_data (MateConfChangeSet *cs)
{
  return cs->user_data;
}

static Change*
get_change_unconditional (MateConfChangeSet* cs,
                          const gchar* key)
{
  Change* c;

  c = g_hash_table_lookup(cs->hash, key);

  if (c == NULL)
    {
      c = change_new(key);

      g_hash_table_insert(cs->hash, c->key, c);
    }

  return c;
}

static gboolean
destroy_foreach (gpointer key, gpointer value, gpointer user_data)
{
  Change* c = value;

  g_assert(c != NULL);

  change_destroy(c);

  return TRUE; /* remove from hash */
}

void
mateconf_change_set_clear    (MateConfChangeSet* cs)
{
  g_return_if_fail(cs != NULL);

  g_hash_table_foreach_remove (cs->hash, destroy_foreach, NULL);
}

guint
mateconf_change_set_size     (MateConfChangeSet* cs)
{
  g_return_val_if_fail(cs != NULL, 0);
  
  return g_hash_table_size(cs->hash);
}

void
mateconf_change_set_remove   (MateConfChangeSet* cs,
                           const gchar* key)
{
  Change* c;
  
  g_return_if_fail(cs != NULL);
  g_return_if_fail(cs->in_foreach == 0);
  
  c = g_hash_table_lookup(cs->hash, key);

  if (c != NULL)
    {
      g_hash_table_remove(cs->hash, c->key);
      change_destroy(c);
    }
}


struct ForeachData {
  MateConfChangeSet* cs;
  MateConfChangeSetForeachFunc func;
  gpointer user_data;
};

static void
foreach(gpointer key, gpointer value, gpointer user_data)
{
  Change* c;
  struct ForeachData* fd = user_data;
  
  c = value;

  /* assumes that an UNSET change has a NULL value */
  (* fd->func) (fd->cs, c->key, c->value, fd->user_data);
}

void
mateconf_change_set_foreach  (MateConfChangeSet* cs,
                           MateConfChangeSetForeachFunc func,
                           gpointer user_data)
{
  struct ForeachData fd;
  
  g_return_if_fail(cs != NULL);
  g_return_if_fail(func != NULL);
  
  fd.cs = cs;
  fd.func = func;
  fd.user_data = user_data;

  mateconf_change_set_ref(cs);

  cs->in_foreach += 1;
  
  g_hash_table_foreach(cs->hash, foreach, &fd);

  cs->in_foreach -= 1;
  
  mateconf_change_set_unref(cs);
}

gboolean
mateconf_change_set_check_value   (MateConfChangeSet* cs, const gchar* key,
                                MateConfValue** value_retloc)
{
  Change* c;
  
  g_return_val_if_fail(cs != NULL, FALSE);

  c = g_hash_table_lookup(cs->hash, key);

  if (c == NULL)
    return FALSE;
  else
    {
      if (value_retloc != NULL)
        *value_retloc = c->value;

      return TRUE;
    }
}

void
mateconf_change_set_set_nocopy  (MateConfChangeSet* cs, const gchar* key,
                              MateConfValue* value)
{
  Change* c;
  
  g_return_if_fail(cs != NULL);
  g_return_if_fail(value != NULL);

  c = get_change_unconditional(cs, key);

  change_set(c, value);
}

void
mateconf_change_set_set (MateConfChangeSet* cs, const gchar* key,
                      MateConfValue* value)
{
  g_return_if_fail(value != NULL);
  
  mateconf_change_set_set_nocopy(cs, key, mateconf_value_copy(value));
}

void
mateconf_change_set_unset      (MateConfChangeSet* cs, const gchar* key)
{
  Change* c;
  
  g_return_if_fail(cs != NULL);

  c = get_change_unconditional(cs, key);

  change_unset(c);
}

void
mateconf_change_set_set_float   (MateConfChangeSet* cs, const gchar* key,
                              gdouble val)
{
  MateConfValue* value;
  
  g_return_if_fail(cs != NULL);

  value = mateconf_value_new(MATECONF_VALUE_FLOAT);
  mateconf_value_set_float(value, val);
  
  mateconf_change_set_set_nocopy(cs, key, value);
}

void
mateconf_change_set_set_int     (MateConfChangeSet* cs, const gchar* key,
                              gint val)
{
  MateConfValue* value;
  
  g_return_if_fail(cs != NULL);

  value = mateconf_value_new(MATECONF_VALUE_INT);
  mateconf_value_set_int(value, val);
  
  mateconf_change_set_set_nocopy(cs, key, value);
}

void
mateconf_change_set_set_string  (MateConfChangeSet* cs, const gchar* key,
                              const gchar* val)
{
  MateConfValue* value;
  
  g_return_if_fail(cs != NULL);
  g_return_if_fail(key != NULL);
  g_return_if_fail(val != NULL);
  
  value = mateconf_value_new(MATECONF_VALUE_STRING);
  mateconf_value_set_string(value, val);
  
  mateconf_change_set_set_nocopy(cs, key, value);
}

void
mateconf_change_set_set_bool    (MateConfChangeSet* cs, const gchar* key,
                              gboolean val)
{
  MateConfValue* value;
  
  g_return_if_fail(cs != NULL);

  value = mateconf_value_new(MATECONF_VALUE_BOOL);
  mateconf_value_set_bool(value, val);
  
  mateconf_change_set_set_nocopy(cs, key, value);
}

void
mateconf_change_set_set_schema  (MateConfChangeSet* cs, const gchar* key,
                              MateConfSchema* val)
{
  MateConfValue* value;
  
  g_return_if_fail(cs != NULL);

  value = mateconf_value_new(MATECONF_VALUE_SCHEMA);
  mateconf_value_set_schema(value, val);
  
  mateconf_change_set_set_nocopy(cs, key, value);
}

void
mateconf_change_set_set_list    (MateConfChangeSet* cs, const gchar* key,
                              MateConfValueType list_type,
                              GSList* list)
{
  MateConfValue* value_list;
  
  g_return_if_fail(cs != NULL);
  g_return_if_fail(key != NULL);
  g_return_if_fail(list_type != MATECONF_VALUE_INVALID);
  g_return_if_fail(list_type != MATECONF_VALUE_LIST);
  g_return_if_fail(list_type != MATECONF_VALUE_PAIR);
  
  value_list = mateconf_value_list_from_primitive_list (list_type, list, NULL);
  
  mateconf_change_set_set_nocopy(cs, key, value_list);
}


void
mateconf_change_set_set_pair    (MateConfChangeSet* cs, const gchar* key,
                              MateConfValueType car_type, MateConfValueType cdr_type,
                              gconstpointer address_of_car,
                              gconstpointer address_of_cdr)
{
  MateConfValue* pair;
  
  g_return_if_fail(cs != NULL);
  g_return_if_fail(key != NULL);
  g_return_if_fail(car_type != MATECONF_VALUE_INVALID);
  g_return_if_fail(car_type != MATECONF_VALUE_LIST);
  g_return_if_fail(car_type != MATECONF_VALUE_PAIR);
  g_return_if_fail(cdr_type != MATECONF_VALUE_INVALID);
  g_return_if_fail(cdr_type != MATECONF_VALUE_LIST);
  g_return_if_fail(cdr_type != MATECONF_VALUE_PAIR);
  g_return_if_fail(address_of_car != NULL);
  g_return_if_fail(address_of_cdr != NULL);

  pair = mateconf_value_pair_from_primitive_pair (car_type, cdr_type,
                                               address_of_car, address_of_cdr,
                                               NULL);
  
  mateconf_change_set_set_nocopy(cs, key, pair);
}


/*
 * Change
 */

Change*
change_new    (const gchar* key)
{
  Change* c;

  c = g_new(Change, 1);

  c->key  = g_strdup(key);
  c->type = CHANGE_INVALID;
  c->value = NULL;

  return c;
}

void
change_destroy(Change* c)
{
  g_return_if_fail(c != NULL);
  
  g_free(c->key);

  if (c->value)
    mateconf_value_free(c->value);

  g_free(c);
}

void
change_set    (Change* c, MateConfValue* value)
{
  g_return_if_fail(value == NULL ||
                   MATECONF_VALUE_TYPE_VALID(value->type));
  
  c->type = CHANGE_SET;

  if (value == c->value)
    return;
  
  if (c->value)
    mateconf_value_free(c->value);

  c->value = value;
}

void
change_unset  (Change* c)
{
  c->type = CHANGE_UNSET;

  if (c->value)
    mateconf_value_free(c->value);

  c->value = NULL;
}

/*
 * Actually send it upstream
 */

struct CommitData {
  MateConfEngine* conf;
  GError* error;
  GSList* remove_list;
  gboolean remove_committed;
};

static void
commit_foreach (MateConfChangeSet* cs,
                const gchar* key,
                MateConfValue* value,
                gpointer user_data)
{
  struct CommitData* cd = user_data;

  g_assert(cd != NULL);

  if (cd->error != NULL)
    return;
  
  if (value)
    mateconf_engine_set   (cd->conf, key, value, &cd->error);
  else
    mateconf_engine_unset (cd->conf, key, &cd->error);

  if (cd->error == NULL && cd->remove_committed)
    {
      /* Bad bad bad; we keep the key reference, knowing that it's
         valid until we modify the change set, to avoid string copies.  */
      cd->remove_list = g_slist_prepend(cd->remove_list, (gchar*)key);
    }
}

gboolean
mateconf_engine_commit_change_set   (MateConfEngine* conf,
                           MateConfChangeSet* cs,
                           gboolean remove_committed,
                           GError** err)
{
  struct CommitData cd;
  GSList* tmp;

  g_return_val_if_fail(conf != NULL, FALSE);
  g_return_val_if_fail(cs != NULL, FALSE);
  g_return_val_if_fail(err == NULL || *err == NULL, FALSE);
  
  cd.conf = conf;
  cd.error = NULL;
  cd.remove_list = NULL;
  cd.remove_committed = remove_committed;

  /* Because the commit could have lots of side
     effects, this makes it safer */
  mateconf_change_set_ref(cs);
  mateconf_engine_ref(conf);
  
  mateconf_change_set_foreach(cs, commit_foreach, &cd);

  tmp = cd.remove_list;
  while (tmp != NULL)
    {
      const gchar* key = tmp->data;
      
      mateconf_change_set_remove(cs, key);

      /* key is now invalid due to our little evil trick */

      tmp = g_slist_next(tmp);
    }

  g_slist_free(cd.remove_list);
  
  mateconf_change_set_unref(cs);
  mateconf_engine_unref(conf);

  if (cd.error != NULL)
    {
      if (err != NULL)
        *err = cd.error;
      else
        g_error_free(cd.error);

      return FALSE;
    }
  else
    {
      return TRUE;
    }
}

struct RevertData {
  MateConfEngine* conf;
  GError* error;
  MateConfChangeSet* revert_set;
};

static void
revert_foreach (MateConfChangeSet* cs,
                const gchar* key,
                MateConfValue* value,
                gpointer user_data)
{
  struct RevertData* rd = user_data;
  MateConfValue* old_value;
  GError* error = NULL;
  
  g_assert(rd != NULL);

  if (rd->error != NULL)
    return;

  old_value = mateconf_engine_get_without_default(rd->conf, key, &error);

  if (error != NULL)
    {
      /* FIXME */
      g_warning("error creating revert set: %s", error->message);
      g_error_free(error);
      error = NULL;
    }
  
  if (old_value == NULL &&
      value == NULL)
    return; /* this commit will have no effect. */

  if (old_value == NULL)
    mateconf_change_set_unset(rd->revert_set, key);
  else
    mateconf_change_set_set_nocopy(rd->revert_set, key, old_value);
}


MateConfChangeSet*
mateconf_engine_reverse_change_set  (MateConfEngine* conf,
                                  MateConfChangeSet* cs,
                                  GError** err)
{
  struct RevertData rd;

  g_return_val_if_fail(err == NULL || *err == NULL, NULL);
  
  rd.error = NULL;
  rd.conf = conf;
  rd.revert_set = mateconf_change_set_new();

  mateconf_change_set_foreach(cs, revert_foreach, &rd);

  if (rd.error != NULL)
    {
      if (err != NULL)
        *err = rd.error;
      else
        g_error_free(rd.error);
    }
  
  return rd.revert_set;
}

MateConfChangeSet*
mateconf_engine_change_set_from_currentv (MateConfEngine* conf,
                                       const gchar** keys,
                                       GError** err)
{
  MateConfValue* old_value;
  MateConfChangeSet* new_set;
  const gchar** keyp;
  
  g_return_val_if_fail(err == NULL || *err == NULL, NULL);

  new_set = mateconf_change_set_new();
  
  keyp = keys;

  while (*keyp != NULL)
    {
      GError* error = NULL;
      const gchar* key = *keyp;
      
      old_value = mateconf_engine_get_without_default(conf, key, &error);

      if (error != NULL)
        {
          /* FIXME */
          g_warning("error creating change set from current keys: %s", error->message);
          g_error_free(error);
          error = NULL;
        }
      
      if (old_value == NULL)
        mateconf_change_set_unset(new_set, key);
      else
        mateconf_change_set_set_nocopy(new_set, key, old_value);

      ++keyp;
    }

  return new_set;
}

MateConfChangeSet*
mateconf_engine_change_set_from_current (MateConfEngine* conf,
                                      GError** err,
                                      const gchar* first_key,
                                      ...)
{
  GSList* keys = NULL;
  va_list args;
  const gchar* arg;
  const gchar** vec;
  MateConfChangeSet* retval;
  GSList* tmp;
  guint i;
  
  g_return_val_if_fail(err == NULL || *err == NULL, NULL);

  va_start (args, first_key);

  arg = first_key;

  while (arg != NULL)
    {
      keys = g_slist_prepend(keys, (/*not-const*/gchar*)arg);

      arg = va_arg (args, const gchar*);
    }
  
  va_end (args);

  vec = g_new0(const gchar*, g_slist_length(keys) + 1);

  i = 0;
  tmp = keys;

  while (tmp != NULL)
    {
      vec[i] = tmp->data;
      
      ++i;
      tmp = g_slist_next(tmp);
    }

  g_slist_free(keys);
  
  retval = mateconf_engine_change_set_from_currentv(conf, vec, err);
  
  g_free(vec);

  return retval;
}
