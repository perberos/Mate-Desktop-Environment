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
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <glib-object.h>
#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "mdm-common.h"

#include "mdm-manager.h"
#include "mdm-manager-glue.h"
#include "mdm-display-store.h"
#include "mdm-display-factory.h"
#include "mdm-local-display-factory.h"
#include "mdm-xdmcp-display-factory.h"

#define MDM_MANAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_MANAGER, MdmManagerPrivate))

#define MDM_DBUS_PATH         "/org/mate/DisplayManager"
#define MDM_MANAGER_DBUS_PATH MDM_DBUS_PATH "/Manager"
#define MDM_MANAGER_DBUS_NAME "org.mate.DisplayManager.Manager"

struct MdmManagerPrivate
{
        MdmDisplayStore        *display_store;
        MdmLocalDisplayFactory *local_factory;
#ifdef HAVE_LIBXDMCP
        MdmXdmcpDisplayFactory *xdmcp_factory;
#endif
        gboolean                xdmcp_enabled;

        gboolean                started;
        gboolean                wait_for_go;
        gboolean                no_console;

        DBusGProxy             *bus_proxy;
        DBusGConnection        *connection;
};

enum {
        PROP_0,
        PROP_XDMCP_ENABLED
};

enum {
        DISPLAY_ADDED,
        DISPLAY_REMOVED,
        LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0, };

static void     mdm_manager_class_init  (MdmManagerClass *klass);
static void     mdm_manager_init        (MdmManager      *manager);
static void     mdm_manager_finalize    (GObject         *object);

static gpointer manager_object = NULL;

G_DEFINE_TYPE (MdmManager, mdm_manager, G_TYPE_OBJECT)

GQuark
mdm_manager_error_quark (void)
{
        static GQuark ret = 0;
        if (ret == 0) {
                ret = g_quark_from_static_string ("mdm_manager_error");
        }

        return ret;
}

static gboolean
listify_display_ids (const char *id,
                     MdmDisplay *display,
                     GPtrArray **array)
{
        g_ptr_array_add (*array, g_strdup (id));

        /* return FALSE to continue */
        return FALSE;
}

/*
  Example:
  dbus-send --system --dest=org.mate.DisplayManager \
  --type=method_call --print-reply --reply-timeout=2000 \
  /org/mate/DisplayManager/Manager \
  org.mate.DisplayManager.Manager.GetDisplays
*/
gboolean
mdm_manager_get_displays (MdmManager *manager,
                          GPtrArray **displays,
                          GError    **error)
{
        g_return_val_if_fail (MDM_IS_MANAGER (manager), FALSE);

        if (displays == NULL) {
                return FALSE;
        }

        *displays = g_ptr_array_new ();
        mdm_display_store_foreach (manager->priv->display_store,
                                   (MdmDisplayStoreFunc)listify_display_ids,
                                   displays);

        return TRUE;
}

void
mdm_manager_stop (MdmManager *manager)
{
        g_debug ("MdmManager: MDM stopping");

        if (manager->priv->local_factory != NULL) {
                mdm_display_factory_stop (MDM_DISPLAY_FACTORY (manager->priv->local_factory));
        }

#ifdef HAVE_LIBXDMCP
        if (manager->priv->xdmcp_factory != NULL) {
                mdm_display_factory_stop (MDM_DISPLAY_FACTORY (manager->priv->xdmcp_factory));
        }
#endif

        manager->priv->started = FALSE;
}

void
mdm_manager_start (MdmManager *manager)
{
        g_debug ("MdmManager: MDM starting to manage displays");

        if (! manager->priv->wait_for_go) {
                mdm_display_factory_start (MDM_DISPLAY_FACTORY (manager->priv->local_factory));
        }

#ifdef HAVE_LIBXDMCP
        /* Accept remote connections */
        if (manager->priv->xdmcp_enabled && ! manager->priv->wait_for_go) {
                if (manager->priv->xdmcp_factory != NULL) {
                        g_debug ("MdmManager: Accepting XDMCP connections...");
                        mdm_display_factory_start (MDM_DISPLAY_FACTORY (manager->priv->xdmcp_factory));
                }
        }
#endif

        manager->priv->started = TRUE;
}

void
mdm_manager_set_wait_for_go (MdmManager *manager,
                             gboolean    wait_for_go)
{
        if (manager->priv->wait_for_go != wait_for_go) {
                manager->priv->wait_for_go = wait_for_go;

                if (! wait_for_go) {
                        /* we got a go */
                        mdm_display_factory_start (MDM_DISPLAY_FACTORY (manager->priv->local_factory));

#ifdef HAVE_LIBXDMCP
                        if (manager->priv->xdmcp_enabled && manager->priv->xdmcp_factory != NULL) {
                                g_debug ("MdmManager: Accepting XDMCP connections...");
                                mdm_display_factory_start (MDM_DISPLAY_FACTORY (manager->priv->xdmcp_factory));
                        }
#endif
                }
        }
}

typedef struct {
        const char *service_name;
        MdmManager *manager;
} RemoveDisplayData;

static gboolean
remove_display_for_connection (char              *id,
                               MdmDisplay        *display,
                               RemoveDisplayData *data)
{
        g_assert (display != NULL);
        g_assert (data->service_name != NULL);

        /* FIXME: compare service name to that of display */
#if 0
        if (strcmp (info->service_name, data->service_name) == 0) {
                remove_session_for_cookie (data->manager, cookie, NULL);
                leader_info_cancel (info);
                return TRUE;
        }
#endif

        return FALSE;
}

static void
remove_displays_for_connection (MdmManager *manager,
                                const char *service_name)
{
        RemoveDisplayData data;

        data.service_name = service_name;
        data.manager = manager;

        mdm_display_store_foreach_remove (manager->priv->display_store,
                                          (MdmDisplayStoreFunc)remove_display_for_connection,
                                          &data);
}

static void
bus_name_owner_changed (DBusGProxy  *bus_proxy,
                        const char  *service_name,
                        const char  *old_service_name,
                        const char  *new_service_name,
                        MdmManager  *manager)
{
        if (strlen (new_service_name) == 0) {
                remove_displays_for_connection (manager, old_service_name);
        }
}

static gboolean
register_manager (MdmManager *manager)
{
        GError *error = NULL;

        error = NULL;
        manager->priv->connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
        if (manager->priv->connection == NULL) {
                if (error != NULL) {
                        g_critical ("error getting system bus: %s", error->message);
                        g_error_free (error);
                }
                exit (1);
        }

        manager->priv->bus_proxy = dbus_g_proxy_new_for_name (manager->priv->connection,
                                                              DBUS_SERVICE_DBUS,
                                                              DBUS_PATH_DBUS,
                                                              DBUS_INTERFACE_DBUS);
        dbus_g_proxy_add_signal (manager->priv->bus_proxy,
                                 "NameOwnerChanged",
                                 G_TYPE_STRING,
                                 G_TYPE_STRING,
                                 G_TYPE_STRING,
                                 G_TYPE_INVALID);
        dbus_g_proxy_connect_signal (manager->priv->bus_proxy,
                                     "NameOwnerChanged",
                                     G_CALLBACK (bus_name_owner_changed),
                                     manager,
                                     NULL);

        dbus_g_connection_register_g_object (manager->priv->connection, MDM_MANAGER_DBUS_PATH, G_OBJECT (manager));

        return TRUE;
}

void
mdm_manager_set_xdmcp_enabled (MdmManager *manager,
                               gboolean    enabled)
{
        g_return_if_fail (MDM_IS_MANAGER (manager));

        if (manager->priv->xdmcp_enabled != enabled) {
                manager->priv->xdmcp_enabled = enabled;
#ifdef HAVE_LIBXDMCP
                if (manager->priv->xdmcp_enabled) {
                        manager->priv->xdmcp_factory = mdm_xdmcp_display_factory_new (manager->priv->display_store);
                        if (manager->priv->started) {
                                mdm_display_factory_start (MDM_DISPLAY_FACTORY (manager->priv->xdmcp_factory));
                        }
                } else {
                        if (manager->priv->started) {
                                mdm_display_factory_stop (MDM_DISPLAY_FACTORY (manager->priv->xdmcp_factory));
                        }

                        g_object_unref (manager->priv->xdmcp_factory);
                        manager->priv->xdmcp_factory = NULL;
                }
#endif
        }

}

static void
mdm_manager_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue  *value,
                          GParamSpec    *pspec)
{
        MdmManager *self;

        self = MDM_MANAGER (object);

        switch (prop_id) {
        case PROP_XDMCP_ENABLED:
                mdm_manager_set_xdmcp_enabled (self, g_value_get_boolean (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
mdm_manager_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
        MdmManager *self;

        self = MDM_MANAGER (object);

        switch (prop_id) {
        case PROP_XDMCP_ENABLED:
                g_value_set_boolean (value, self->priv->xdmcp_enabled);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static GObject *
mdm_manager_constructor (GType                  type,
                         guint                  n_construct_properties,
                         GObjectConstructParam *construct_properties)
{
        MdmManager      *manager;

        manager = MDM_MANAGER (G_OBJECT_CLASS (mdm_manager_parent_class)->constructor (type,
                                                                                       n_construct_properties,
                                                                                       construct_properties));

        manager->priv->local_factory = mdm_local_display_factory_new (manager->priv->display_store);

#ifdef HAVE_LIBXDMCP
        if (manager->priv->xdmcp_enabled) {
                manager->priv->xdmcp_factory = mdm_xdmcp_display_factory_new (manager->priv->display_store);
        }
#endif

        return G_OBJECT (manager);
}

static void
mdm_manager_class_init (MdmManagerClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = mdm_manager_get_property;
        object_class->set_property = mdm_manager_set_property;
        object_class->constructor = mdm_manager_constructor;
        object_class->finalize = mdm_manager_finalize;

        signals [DISPLAY_ADDED] =
                g_signal_new ("display-added",
                              G_TYPE_FROM_CLASS (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (MdmManagerClass, display_added),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1, G_TYPE_STRING);
        signals [DISPLAY_REMOVED] =
                g_signal_new ("display-removed",
                              G_TYPE_FROM_CLASS (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (MdmManagerClass, display_removed),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1, G_TYPE_STRING);

        g_object_class_install_property (object_class,
                                         PROP_XDMCP_ENABLED,
                                         g_param_spec_boolean ("xdmcp-enabled",
                                                               NULL,
                                                               NULL,
                                                               FALSE,
                                                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

        g_type_class_add_private (klass, sizeof (MdmManagerPrivate));

        dbus_g_object_type_install_info (MDM_TYPE_MANAGER, &dbus_glib_mdm_manager_object_info);
}

static void
mdm_manager_init (MdmManager *manager)
{

        manager->priv = MDM_MANAGER_GET_PRIVATE (manager);

        manager->priv->display_store = mdm_display_store_new ();
}

static void
mdm_manager_finalize (GObject *object)
{
        MdmManager *manager;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_MANAGER (object));

        manager = MDM_MANAGER (object);

        g_return_if_fail (manager->priv != NULL);

#ifdef HAVE_LIBXDMCP
        if (manager->priv->xdmcp_factory != NULL) {
                g_object_unref (manager->priv->xdmcp_factory);
        }
#endif

        mdm_display_store_clear (manager->priv->display_store);
        g_object_unref (manager->priv->display_store);

        G_OBJECT_CLASS (mdm_manager_parent_class)->finalize (object);
}

MdmManager *
mdm_manager_new (void)
{
        if (manager_object != NULL) {
                g_object_ref (manager_object);
        } else {
                gboolean res;

                manager_object = g_object_new (MDM_TYPE_MANAGER, NULL);
                g_object_add_weak_pointer (manager_object,
                                           (gpointer *) &manager_object);
                res = register_manager (manager_object);
                if (! res) {
                        g_object_unref (manager_object);
                        return NULL;
                }
        }

        return MDM_MANAGER (manager_object);
}
