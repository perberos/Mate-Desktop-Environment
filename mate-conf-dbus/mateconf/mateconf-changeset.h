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

#ifndef MATECONF_MATECONF_CHANGESET_H
#define MATECONF_MATECONF_CHANGESET_H

#include <mateconf/mateconf.h>

G_BEGIN_DECLS

/*
 * A MateConfChangeSet is basically a hash from keys to "changes in value,"
 * where a change in a value is either a new value or "unset this value."
 *
 * You can use this to collect changes then "commit" them as a group to
 * the MateConf database.
 */

#define MATECONF_TYPE_CHANGE_SET                  (mateconf_change_set_get_type ())

typedef struct _MateConfChangeSet MateConfChangeSet;

typedef void (* MateConfChangeSetForeachFunc) (MateConfChangeSet* cs,
                                            const gchar* key,
                                            MateConfValue* value,
                                            gpointer user_data);

gboolean        mateconf_engine_commit_change_set   (MateConfEngine* conf,
                                                  MateConfChangeSet* cs,
                                                  /* remove all
                                                     successfully
                                                     committed changes
                                                     from the set */
                                                  gboolean remove_committed,
                                                  GError** err);

/* Create a change set that would revert the given change set
   for the given MateConfEngine */
MateConfChangeSet* mateconf_engine_reverse_change_set  (MateConfEngine* conf,
                                                  MateConfChangeSet* cs,
                                                  GError** err);

/* Create a change set that would restore the current state of all the keys
   in the NULL-terminated array "keys" */
MateConfChangeSet* mateconf_engine_change_set_from_currentv (MateConfEngine* conf,
                                                       const gchar** keys,
                                                       GError** err);

MateConfChangeSet* mateconf_engine_change_set_from_current (MateConfEngine* conf,
                                                      GError** err,
                                                      const gchar* first_key,
                                                      ...) G_GNUC_NULL_TERMINATED;


GType           mateconf_change_set_get_type (void);
MateConfChangeSet* mateconf_change_set_new      (void);
void            mateconf_change_set_ref      (MateConfChangeSet* cs);

void            mateconf_change_set_unref    (MateConfChangeSet* cs);

void            mateconf_change_set_clear    (MateConfChangeSet* cs);

guint           mateconf_change_set_size     (MateConfChangeSet* cs);

void            mateconf_change_set_remove   (MateConfChangeSet* cs,
                                           const gchar* key);

void            mateconf_change_set_foreach  (MateConfChangeSet* cs,
                                           MateConfChangeSetForeachFunc func,
                                           gpointer user_data);

/* Returns TRUE if the change set contains the given key; if the key
   is in the set, either NULL (for unset) or a MateConfValue is placed in
   *value_retloc; the value is not a copy and should not be
   freed. value_retloc can be NULL if you just want to check for a value,
   and don't care what it is. */
gboolean     mateconf_change_set_check_value   (MateConfChangeSet* cs, const gchar* key,
                                             MateConfValue** value_retloc);

void         mateconf_change_set_set         (MateConfChangeSet* cs, const gchar* key,
                                           MateConfValue* value);

void         mateconf_change_set_set_nocopy  (MateConfChangeSet* cs, const gchar* key,
                                           MateConfValue* value);

void         mateconf_change_set_unset      (MateConfChangeSet* cs, const gchar* key);

void         mateconf_change_set_set_float   (MateConfChangeSet* cs, const gchar* key,
                                           gdouble val);

void         mateconf_change_set_set_int     (MateConfChangeSet* cs, const gchar* key,
                                           gint val);

void         mateconf_change_set_set_string  (MateConfChangeSet* cs, const gchar* key,
                                           const gchar* val);

void         mateconf_change_set_set_bool    (MateConfChangeSet* cs, const gchar* key,
                                           gboolean val);

void         mateconf_change_set_set_schema  (MateConfChangeSet* cs, const gchar* key,
                                           MateConfSchema* val);

void         mateconf_change_set_set_list    (MateConfChangeSet* cs, const gchar* key,
                                           MateConfValueType list_type,
                                           GSList* list);

void         mateconf_change_set_set_pair    (MateConfChangeSet* cs, const gchar* key,
                                           MateConfValueType car_type, MateConfValueType cdr_type,
                                           gconstpointer address_of_car,
                                           gconstpointer address_of_cdr);


/* For use by language bindings only */
void     mateconf_change_set_set_user_data (MateConfChangeSet *cs,
                                         gpointer        data,
                                         GDestroyNotify  dnotify);
gpointer mateconf_change_set_get_user_data (MateConfChangeSet *cs);



G_END_DECLS

#endif



