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

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>

#include "mdm-display-factory.h"
#include "mdm-local-display-factory.h"
#include "mdm-local-display-factory-glue.h"

#include "mdm-display-store.h"
#include "mdm-static-display.h"
#include "mdm-transient-display.h"
#include "mdm-static-factory-display.h"
#include "mdm-product-display.h"

#define MDM_LOCAL_DISPLAY_FACTORY_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_LOCAL_DISPLAY_FACTORY, MdmLocalDisplayFactoryPrivate))

#define CK_SEAT1_PATH                       "/org/freedesktop/ConsoleKit/Seat1"

#define MDM_DBUS_PATH                       "/org/mate/DisplayManager"
#define MDM_LOCAL_DISPLAY_FACTORY_DBUS_PATH MDM_DBUS_PATH "/LocalDisplayFactory"
#define MDM_MANAGER_DBUS_NAME               "org.mate.DisplayManager.LocalDisplayFactory"

#define MAX_DISPLAY_FAILURES 5

struct MdmLocalDisplayFactoryPrivate
{
        DBusGConnection *connection;
        DBusGProxy      *proxy;
        GHashTable      *displays;

        /* FIXME: this needs to be per seat? */
        guint            num_failures;
};

enum {
        PROP_0,
};

static void     mdm_local_display_factory_class_init    (MdmLocalDisplayFactoryClass *klass);
static void     mdm_local_display_factory_init          (MdmLocalDisplayFactory      *factory);
static void     mdm_local_display_factory_finalize      (GObject                     *object);

static MdmDisplay *create_display                       (MdmLocalDisplayFactory      *factory);

static gpointer local_display_factory_object = NULL;

G_DEFINE_TYPE (MdmLocalDisplayFactory, mdm_local_display_factory, MDM_TYPE_DISPLAY_FACTORY)

GQuark
mdm_local_display_factory_error_quark (void)
{
        static GQuark ret = 0;
        if (ret == 0) {
                ret = g_quark_from_static_string ("mdm_local_display_factory_error");
        }

        return ret;
}

static void
listify_hash (gpointer    key,
              MdmDisplay *display,
              GList     **list)
{
        *list = g_list_prepend (*list, key);
}

static int
sort_nums (gpointer a,
           gpointer b)
{
        guint32 num_a;
        guint32 num_b;

        num_a = GPOINTER_TO_UINT (a);
        num_b = GPOINTER_TO_UINT (b);

        if (num_a > num_b) {
                return 1;
        } else if (num_a < num_b) {
                return -1;
        } else {
                return 0;
        }
}

static guint32
take_next_display_number (MdmLocalDisplayFactory *factory)
{
        GList  *list;
        GList  *l;
        guint32 ret;

        ret = 0;
        list = NULL;

        g_hash_table_foreach (factory->priv->displays, (GHFunc)listify_hash, &list);
        if (list == NULL) {
                goto out;
        }

        /* sort low to high */
        list = g_list_sort (list, (GCompareFunc)sort_nums);

        g_debug ("MdmLocalDisplayFactory: Found the following X displays:");
        for (l = list; l != NULL; l = l->next) {
                g_debug ("MdmLocalDisplayFactory: %u", GPOINTER_TO_UINT (l->data));
        }

        for (l = list; l != NULL; l = l->next) {
                guint32 num;
                num = GPOINTER_TO_UINT (l->data);

                /* always fill zero */
                if (l->prev == NULL && num != 0) {
                        ret = 0;
                        break;
                }
                /* now find the first hole */
                if (l->next == NULL || GPOINTER_TO_UINT (l->next->data) != (num + 1)) {
                        ret = num + 1;
                        break;
                }
        }
 out:

        /* now reserve this number */
        g_debug ("MdmLocalDisplayFactory: Reserving X display: %u", ret);
        g_hash_table_insert (factory->priv->displays, GUINT_TO_POINTER (ret), NULL);

        return ret;
}

static void
on_display_disposed (MdmLocalDisplayFactory *factory,
                     MdmDisplay             *display)
{
        g_debug ("MdmLocalDisplayFactory: Display %p disposed", display);
}

static void
store_display (MdmLocalDisplayFactory *factory,
               guint32                 num,
               MdmDisplay             *display)
{
        MdmDisplayStore *store;

        g_object_weak_ref (G_OBJECT (display), (GWeakNotify)on_display_disposed, factory);

        store = mdm_display_factory_get_display_store (MDM_DISPLAY_FACTORY (factory));
        mdm_display_store_add (store, display);

        /* now fill our reserved spot */
        g_hash_table_insert (factory->priv->displays, GUINT_TO_POINTER (num), NULL);
}

/*
  Example:
  dbus-send --system --dest=org.mate.DisplayManager \
  --type=method_call --print-reply --reply-timeout=2000 \
  /org/mate/DisplayManager/Manager \
  org.mate.DisplayManager.Manager.GetDisplays
*/
gboolean
mdm_local_display_factory_create_transient_display (MdmLocalDisplayFactory *factory,
                                                    char                  **id,
                                                    GError                **error)
{
        gboolean         ret;
        MdmDisplay      *display;
        guint32          num;

        g_return_val_if_fail (MDM_IS_LOCAL_DISPLAY_FACTORY (factory), FALSE);

        ret = FALSE;

        num = take_next_display_number (factory);

        g_debug ("MdmLocalDisplayFactory: Creating transient display %d", num);

        display = mdm_transient_display_new (num);

        /* FIXME: don't hardcode seat1? */
        g_object_set (display, "seat-id", CK_SEAT1_PATH, NULL);

        store_display (factory, num, display);

        if (! mdm_display_manage (display)) {
                display = NULL;
                goto out;
        }

        if (! mdm_display_get_id (display, id, NULL)) {
                display = NULL;
                goto out;
        }

        ret = TRUE;
 out:
        /* ref either held by store or not at all */
        g_object_unref (display);

        return ret;
}

gboolean
mdm_local_display_factory_create_product_display (MdmLocalDisplayFactory *factory,
                                                  const char             *parent_display_id,
                                                  const char             *relay_address,
                                                  char                  **id,
                                                  GError                **error)
{
        gboolean    ret;
        MdmDisplay *display;
        guint32     num;

        g_return_val_if_fail (MDM_IS_LOCAL_DISPLAY_FACTORY (factory), FALSE);

        ret = FALSE;

        g_debug ("MdmLocalDisplayFactory: Creating product display parent %s address:%s",
                 parent_display_id, relay_address);

        num = take_next_display_number (factory);

        g_debug ("MdmLocalDisplayFactory: got display num %u", num);

        display = mdm_product_display_new (num, relay_address);

        /* FIXME: don't hardcode seat1? */
        g_object_set (display, "seat-id", CK_SEAT1_PATH, NULL);

        store_display (factory, num, display);

        if (! mdm_display_manage (display)) {
                display = NULL;
                goto out;
        }

        if (! mdm_display_get_id (display, id, NULL)) {
                display = NULL;
                goto out;
        }

        ret = TRUE;
 out:
        /* ref either held by store or not at all */
        g_object_unref (display);

        return ret;
}

static void
on_static_display_status_changed (MdmDisplay             *display,
                                  GParamSpec             *arg1,
                                  MdmLocalDisplayFactory *factory)
{
        int              status;
        MdmDisplayStore *store;
        int              num;

        num = -1;
        mdm_display_get_x11_display_number (display, &num, NULL);
        g_assert (num != -1);

        store = mdm_display_factory_get_display_store (MDM_DISPLAY_FACTORY (factory));

        status = mdm_display_get_status (display);

        g_debug ("MdmLocalDisplayFactory: static display status changed: %d", status);
        switch (status) {
        case MDM_DISPLAY_FINISHED:
                /* remove the display number from factory->priv->displays
                   so that it may be reused */
                g_hash_table_remove (factory->priv->displays, GUINT_TO_POINTER (num));
                mdm_display_store_remove (store, display);
                /* reset num failures */
                factory->priv->num_failures = 0;
                create_display (factory);
                break;
        case MDM_DISPLAY_FAILED:
                /* leave the display number in factory->priv->displays
                   so that it doesn't get reused */
                mdm_display_store_remove (store, display);
                factory->priv->num_failures++;
                if (factory->priv->num_failures > MAX_DISPLAY_FAILURES) {
                        /* oh shit */
                        g_warning ("MdmLocalDisplayFactory: maximum number of X display failures reached: check X server log for errors");
                        /* FIXME: should monitor hardware changes to
                           try again when seats change */
                } else {
                        create_display (factory);
                }
                break;
        case MDM_DISPLAY_UNMANAGED:
                break;
        case MDM_DISPLAY_PREPARED:
                break;
        case MDM_DISPLAY_MANAGED:
                break;
        default:
                g_assert_not_reached ();
                break;
        }
}

static MdmDisplay *
create_display (MdmLocalDisplayFactory *factory)
{
        MdmDisplay *display;
        guint32     num;

        num = take_next_display_number (factory);

#if 0
        display = mdm_static_factory_display_new (num);
#else
        display = mdm_static_display_new (num);
#endif
        if (display == NULL) {
                g_warning ("Unable to create display: %d", num);
                return NULL;
        }

        /* FIXME: don't hardcode seat1? */
        g_object_set (display, "seat-id", CK_SEAT1_PATH, NULL);

        g_signal_connect (display,
                          "notify::status",
                          G_CALLBACK (on_static_display_status_changed),
                          factory);

        store_display (factory, num, display);

        /* let store own the ref */
        g_object_unref (display);

        if (! mdm_display_manage (display)) {
                mdm_display_unmanage (display);
        }

        return display;
}

static gboolean
mdm_local_display_factory_start (MdmDisplayFactory *base_factory)
{
        gboolean                ret;
        MdmLocalDisplayFactory *factory = MDM_LOCAL_DISPLAY_FACTORY (base_factory);
        MdmDisplay             *display;

        g_return_val_if_fail (MDM_IS_LOCAL_DISPLAY_FACTORY (factory), FALSE);

        ret = TRUE;

        /* FIXME: use seat configuration */
        display = create_display (factory);
        if (display == NULL) {
                ret = FALSE;
        }

        return ret;
}

static gboolean
mdm_local_display_factory_stop (MdmDisplayFactory *base_factory)
{
        MdmLocalDisplayFactory *factory = MDM_LOCAL_DISPLAY_FACTORY (base_factory);

        g_return_val_if_fail (MDM_IS_LOCAL_DISPLAY_FACTORY (factory), FALSE);

        return TRUE;
}

static void
mdm_local_display_factory_set_property (GObject       *object,
                                        guint          prop_id,
                                        const GValue  *value,
                                        GParamSpec    *pspec)
{
        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
mdm_local_display_factory_get_property (GObject    *object,
                                        guint       prop_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static gboolean
register_factory (MdmLocalDisplayFactory *factory)
{
        GError *error = NULL;

        error = NULL;
        factory->priv->connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
        if (factory->priv->connection == NULL) {
                if (error != NULL) {
                        g_critical ("error getting system bus: %s", error->message);
                        g_error_free (error);
                }
                exit (1);
        }

        dbus_g_connection_register_g_object (factory->priv->connection, MDM_LOCAL_DISPLAY_FACTORY_DBUS_PATH, G_OBJECT (factory));

        return TRUE;
}

static GObject *
mdm_local_display_factory_constructor (GType                  type,
                                       guint                  n_construct_properties,
                                       GObjectConstructParam *construct_properties)
{
        MdmLocalDisplayFactory      *factory;
        gboolean                     res;

        factory = MDM_LOCAL_DISPLAY_FACTORY (G_OBJECT_CLASS (mdm_local_display_factory_parent_class)->constructor (type,
                                                                                                                   n_construct_properties,
                                                                                                                   construct_properties));

        res = register_factory (factory);
        if (! res) {
                g_warning ("Unable to register local display factory with system bus");
        }

        return G_OBJECT (factory);
}

static void
mdm_local_display_factory_class_init (MdmLocalDisplayFactoryClass *klass)
{
        GObjectClass           *object_class = G_OBJECT_CLASS (klass);
        MdmDisplayFactoryClass *factory_class = MDM_DISPLAY_FACTORY_CLASS (klass);

        object_class->get_property = mdm_local_display_factory_get_property;
        object_class->set_property = mdm_local_display_factory_set_property;
        object_class->finalize = mdm_local_display_factory_finalize;
        object_class->constructor = mdm_local_display_factory_constructor;

        factory_class->start = mdm_local_display_factory_start;
        factory_class->stop = mdm_local_display_factory_stop;

        g_type_class_add_private (klass, sizeof (MdmLocalDisplayFactoryPrivate));

        dbus_g_object_type_install_info (MDM_TYPE_LOCAL_DISPLAY_FACTORY, &dbus_glib_mdm_local_display_factory_object_info);
}

static void
mdm_local_display_factory_init (MdmLocalDisplayFactory *factory)
{
        factory->priv = MDM_LOCAL_DISPLAY_FACTORY_GET_PRIVATE (factory);

        factory->priv->displays = g_hash_table_new (NULL, NULL);
}

static void
mdm_local_display_factory_finalize (GObject *object)
{
        MdmLocalDisplayFactory *factory;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_LOCAL_DISPLAY_FACTORY (object));

        factory = MDM_LOCAL_DISPLAY_FACTORY (object);

        g_return_if_fail (factory->priv != NULL);

        g_hash_table_destroy (factory->priv->displays);

        G_OBJECT_CLASS (mdm_local_display_factory_parent_class)->finalize (object);
}

MdmLocalDisplayFactory *
mdm_local_display_factory_new (MdmDisplayStore *store)
{
        if (local_display_factory_object != NULL) {
                g_object_ref (local_display_factory_object);
        } else {
                local_display_factory_object = g_object_new (MDM_TYPE_LOCAL_DISPLAY_FACTORY,
                                                             "display-store", store,
                                                             NULL);
                g_object_add_weak_pointer (local_display_factory_object,
                                           (gpointer *) &local_display_factory_object);
        }

        return MDM_LOCAL_DISPLAY_FACTORY (local_display_factory_object);
}
