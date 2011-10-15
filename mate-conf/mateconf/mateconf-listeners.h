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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef MATECONF_MATECONF_LISTENERS_H
#define MATECONF_MATECONF_LISTENERS_H

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Skipped from introspection because it's not registered as boxed */
/**
 * MateConfListeners: (skip)
 *
 * The #MateConfListeners structure contains nothing other than a dummy pointer. Internally
 * the data about listeners is maintained through a listener table structure,
 * LTable which contains data like the namespace, an array to hold the listeners, count of
 * active listeners,value to be given to the next connection and the list of connection indices
 * to be recycled. There is also a Listener structure maintaining data pertaining to listeners.
 */

/*
 * The listeners object is used to store listeners who want notification
 * of changes in a namespace section.
 *
 * It's shared between mateconfd and the GObject convenience wrapper.
 * It's also a public API.
 */

typedef struct _MateConfListeners MateConfListeners;

typedef void (*MateConfListenersCallback)(MateConfListeners* listeners,
                                       const gchar*    all_above_key,
                                       guint           cnxn_id,
                                       gpointer        listener_data,
                                       gpointer        user_data);

typedef void (*MateConfListenersForeach) (const gchar* location,
                                       guint        cnxn_id,
                                       gpointer     listener_data,
                                       gpointer     user_data);

typedef gboolean (*MateConfListenersPredicate) (const gchar* location,
                                             guint        cnxn_id,
                                             gpointer     listener_data,
                                             gpointer     user_data);

MateConfListeners*     mateconf_listeners_new     (void);

void                mateconf_listeners_free    (MateConfListeners* listeners);

guint               mateconf_listeners_add     (MateConfListeners* listeners,
                                             const gchar* listen_point,
                                             gpointer listener_data,
                                             /* can be NULL */
                                             GFreeFunc destroy_notify);

/* Safe on nonexistent listeners, for robustness against broken
 * clients
 */

void     mateconf_listeners_remove   (MateConfListeners          *listeners,
                                   guint                    cnxn_id);
void     mateconf_listeners_notify   (MateConfListeners          *listeners,
                                   const gchar             *all_above,
                                   MateConfListenersCallback   callback,
                                   gpointer                 user_data);
guint    mateconf_listeners_count    (MateConfListeners          *listeners);
void     mateconf_listeners_foreach  (MateConfListeners          *listeners,
                                   MateConfListenersForeach    callback,
                                   gpointer                 user_data);
gboolean mateconf_listeners_get_data (MateConfListeners          *listeners,
                                   guint                    cnxn_id,
                                   gpointer                *listener_data_p,
                                   const gchar            **location_p);

void     mateconf_listeners_remove_if (MateConfListeners         *listeners,
                                    MateConfListenersPredicate predicate,
                                    gpointer                user_data);

#ifdef __cplusplus
}
#endif

#endif



