/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007-2008 William Jon McCann <mccann@jhu.edu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */


#ifndef __GSM_STORE_H
#define __GSM_STORE_H

#include <glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GSM_TYPE_STORE         (gsm_store_get_type ())
#define GSM_STORE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GSM_TYPE_STORE, GsmStore))
#define GSM_STORE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GSM_TYPE_STORE, GsmStoreClass))
#define GSM_IS_STORE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GSM_TYPE_STORE))
#define GSM_IS_STORE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GSM_TYPE_STORE))
#define GSM_STORE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GSM_TYPE_STORE, GsmStoreClass))

typedef struct GsmStorePrivate GsmStorePrivate;

typedef struct
{
        GObject          parent;
        GsmStorePrivate *priv;
} GsmStore;

typedef struct
{
        GObjectClass   parent_class;

        void          (* added)    (GsmStore   *store,
                                    const char *id);
        void          (* removed)  (GsmStore   *store,
                                    const char *id);
} GsmStoreClass;

typedef enum
{
         GSM_STORE_ERROR_GENERAL
} GsmStoreError;

#define GSM_STORE_ERROR gsm_store_error_quark ()

typedef gboolean (*GsmStoreFunc) (const char *id,
                                  GObject    *object,
                                  gpointer    user_data);

GQuark              gsm_store_error_quark              (void);
GType               gsm_store_get_type                 (void);

GsmStore *          gsm_store_new                      (void);

gboolean            gsm_store_get_locked               (GsmStore    *store);
void                gsm_store_set_locked               (GsmStore    *store,
                                                        gboolean     locked);

guint               gsm_store_size                     (GsmStore    *store);
gboolean            gsm_store_add                      (GsmStore    *store,
                                                        const char  *id,
                                                        GObject     *object);
void                gsm_store_clear                    (GsmStore    *store);
gboolean            gsm_store_remove                   (GsmStore    *store,
                                                        const char  *id);

void                gsm_store_foreach                  (GsmStore    *store,
                                                        GsmStoreFunc func,
                                                        gpointer     user_data);
guint               gsm_store_foreach_remove           (GsmStore    *store,
                                                        GsmStoreFunc func,
                                                        gpointer     user_data);
GObject *           gsm_store_find                     (GsmStore    *store,
                                                        GsmStoreFunc predicate,
                                                        gpointer     user_data);
GObject *           gsm_store_lookup                   (GsmStore    *store,
                                                        const char  *id);


#ifdef __cplusplus
}
#endif

#endif /* __GSM_STORE_H */
