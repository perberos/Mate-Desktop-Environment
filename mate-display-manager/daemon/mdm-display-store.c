/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
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

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>

#include "mdm-display-store.h"
#include "mdm-display.h"

#define MDM_DISPLAY_STORE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_DISPLAY_STORE, MdmDisplayStorePrivate))

struct MdmDisplayStorePrivate
{
        GHashTable *displays;
};

enum {
        DISPLAY_ADDED,
        DISPLAY_REMOVED,
        LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0, };

static void     mdm_display_store_class_init    (MdmDisplayStoreClass *klass);
static void     mdm_display_store_init          (MdmDisplayStore      *display_store);
static void     mdm_display_store_finalize      (GObject              *object);

G_DEFINE_TYPE (MdmDisplayStore, mdm_display_store, G_TYPE_OBJECT)

GQuark
mdm_display_store_error_quark (void)
{
        static GQuark ret = 0;
        if (ret == 0) {
                ret = g_quark_from_static_string ("mdm_display_store_error");
        }

        return ret;
}

void
mdm_display_store_clear (MdmDisplayStore    *store)
{
        g_return_if_fail (store != NULL);
        g_debug ("MdmDisplayStore: Clearing display store");
        g_hash_table_remove_all (store->priv->displays);
}

static gboolean
remove_display (char              *id,
                MdmDisplay        *display,
                MdmDisplay        *display_to_remove)
{
        if (display == display_to_remove) {
                return TRUE;
        }
        return FALSE;
}

gboolean
mdm_display_store_remove (MdmDisplayStore    *store,
                          MdmDisplay         *display)
{
        g_return_val_if_fail (store != NULL, FALSE);

        mdm_display_store_foreach_remove (store,
                                          (MdmDisplayStoreFunc)remove_display,
                                          display);
        return FALSE;
}

void
mdm_display_store_foreach (MdmDisplayStore    *store,
                           MdmDisplayStoreFunc func,
                           gpointer            user_data)
{
        g_return_if_fail (store != NULL);
        g_return_if_fail (func != NULL);

        g_hash_table_find (store->priv->displays,
                           (GHRFunc)func,
                           user_data);
}

MdmDisplay *
mdm_display_store_find (MdmDisplayStore    *store,
                        MdmDisplayStoreFunc predicate,
                        gpointer            user_data)
{
        MdmDisplay *display;

        g_return_val_if_fail (store != NULL, NULL);
        g_return_val_if_fail (predicate != NULL, NULL);

        display = g_hash_table_find (store->priv->displays,
                                     (GHRFunc)predicate,
                                     user_data);
        return display;
}

guint
mdm_display_store_foreach_remove (MdmDisplayStore    *store,
                                  MdmDisplayStoreFunc func,
                                  gpointer            user_data)
{
        guint ret;

        g_return_val_if_fail (store != NULL, 0);
        g_return_val_if_fail (func != NULL, 0);

        ret = g_hash_table_foreach_remove (store->priv->displays,
                                           (GHRFunc)func,
                                           user_data);

        return ret;
}

void
mdm_display_store_add (MdmDisplayStore *store,
                       MdmDisplay      *display)
{
        char *id;

        g_return_if_fail (store != NULL);
        g_return_if_fail (display != NULL);

        mdm_display_get_id (display, &id, NULL);

        g_debug ("MdmDisplayStore: Adding display %s to store", id);

        g_hash_table_insert (store->priv->displays,
                             id,
                             g_object_ref (display));
}

static void
mdm_display_store_class_init (MdmDisplayStoreClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = mdm_display_store_finalize;

        signals [DISPLAY_ADDED] =
                g_signal_new ("display-added",
                              G_TYPE_FROM_CLASS (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (MdmDisplayStoreClass, display_added),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1, G_TYPE_STRING);
        signals [DISPLAY_REMOVED] =
                g_signal_new ("display-removed",
                              G_TYPE_FROM_CLASS (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (MdmDisplayStoreClass, display_removed),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1, G_TYPE_STRING);

        g_type_class_add_private (klass, sizeof (MdmDisplayStorePrivate));
}

static void
display_unref (MdmDisplay *display)
{
        g_debug ("MdmDisplayStore: Unreffing display: %p", display);
        g_object_unref (display);
}

static void
mdm_display_store_init (MdmDisplayStore *store)
{

        store->priv = MDM_DISPLAY_STORE_GET_PRIVATE (store);

        store->priv->displays = g_hash_table_new_full (g_str_hash,
                                                       g_str_equal,
                                                       g_free,
                                                       (GDestroyNotify) display_unref);
}

static void
mdm_display_store_finalize (GObject *object)
{
        MdmDisplayStore *store;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_DISPLAY_STORE (object));

        store = MDM_DISPLAY_STORE (object);

        g_return_if_fail (store->priv != NULL);

        g_hash_table_destroy (store->priv->displays);

        G_OBJECT_CLASS (mdm_display_store_parent_class)->finalize (object);
}

MdmDisplayStore *
mdm_display_store_new (void)
{
        GObject *object;

        object = g_object_new (MDM_TYPE_DISPLAY_STORE,
                               NULL);

        return MDM_DISPLAY_STORE (object);
}
