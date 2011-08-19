/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2004-2006 Christian Hammond <chipx86@chipx86.com>
 * Copyright (C) 2004-2006 Mike Hearn <mike@navi.cx>
 * Copyright (C) 2010 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307, USA.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>

#include "notify.h"
#include "internal.h"
#include "notify-marshal.h"

static gboolean         _initted = FALSE;
static char            *_app_name = NULL;
static DBusGProxy      *_proxy = NULL;
static DBusGConnection *_dbus_gconn = NULL;
static GList           *_active_notifications = NULL;
static int              _spec_version_major = 0;
static int              _spec_version_minor = 0;

gboolean
_notify_check_spec_version (int major,
                            int minor)
{
       if (_spec_version_major > major)
               return TRUE;
       if (_spec_version_major < major)
               return FALSE;
       return _spec_version_minor >= minor;
}

static gboolean
_notify_update_spec_version (void)
{
       char *spec_version;

       if (!notify_get_server_info (NULL, NULL, NULL, &spec_version)) {
               return FALSE;
       }

       sscanf (spec_version,
               "%d.%d",
               &_spec_version_major,
               &_spec_version_minor);

       g_free (spec_version);

       return TRUE;
}

/**
 * notify_init:
 * @app_name: The name of the application initializing libmatenotify.
 *
 * Initialized libmatenotify. This must be called before any other functions.
 *
 * Returns: %TRUE if successful, or %FALSE on error.
 */
gboolean
notify_init (const char *app_name)
{
        g_return_val_if_fail (app_name != NULL, FALSE);
        g_return_val_if_fail (*app_name != '\0', FALSE);

        if (_initted)
                return TRUE;

        _app_name = g_strdup (app_name);

        g_type_init ();

        _initted = TRUE;

        return TRUE;
}

/**
 * notify_get_app_name:
 *
 * Gets the application name registered.
 *
 * Returns: The registered application name, passed to notify_init().
 */
const char *
notify_get_app_name (void)
{
        return _app_name;
}

/**
 * notify_uninit:
 *
 * Uninitialized libmatenotify.
 *
 * This should be called when the program no longer needs libmatenotify for
 * the rest of its lifecycle, typically just before exitting.
 */
void
notify_uninit (void)
{
        GList *l;

        if (!_initted) {
                return;
        }

        if (_app_name != NULL) {
                g_free (_app_name);
                _app_name = NULL;
        }

        for (l = _active_notifications; l != NULL; l = l->next) {
                NotifyNotification *n = NOTIFY_NOTIFICATION (l->data);

                if (_notify_notification_get_timeout (n) == 0 ||
                    _notify_notification_has_nondefault_actions (n)) {
                        notify_notification_close (n, NULL);
                }
        }

        g_object_unref (_proxy);

        _initted = FALSE;
}

/**
 * notify_is_initted:
 *
 * Gets whether or not libmatenotify is initialized.
 *
 * Returns: %TRUE if libmatenotify is initialized, or %FALSE otherwise.
 */
gboolean
notify_is_initted (void)
{
        return _initted;
}

DBusGConnection *
_notify_get_dbus_g_conn (void)
{
        return _dbus_gconn;
}

static void
on_proxy_destroy (DBusGProxy *proxy,
                  gpointer    data)
{
        _proxy = NULL;
}

DBusGProxy *
_notify_get_g_proxy (void)
{
        GError          *error;
        DBusGConnection *bus;

        if (_proxy != NULL)
                return _proxy;

        /* lazily initialize D-Bus connection */
        error = NULL;
        bus = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
        if (error != NULL) {
                g_error_free (error);
                return NULL;
        }

        _proxy = dbus_g_proxy_new_for_name (bus,
                                            NOTIFY_DBUS_NAME,
                                            NOTIFY_DBUS_CORE_OBJECT,
                                            NOTIFY_DBUS_CORE_INTERFACE);
        dbus_g_connection_unref (bus);

        g_signal_connect (_proxy,
                          "destroy",
                          G_CALLBACK (on_proxy_destroy),
                          NULL);

        dbus_g_object_register_marshaller (notify_marshal_VOID__UINT_UINT,
                                           G_TYPE_NONE,
                                           G_TYPE_UINT,
                                           G_TYPE_UINT,
                                           G_TYPE_INVALID);

        dbus_g_object_register_marshaller (notify_marshal_VOID__UINT_STRING,
                                           G_TYPE_NONE,
                                           G_TYPE_UINT,
                                           G_TYPE_STRING,
                                           G_TYPE_INVALID);

        dbus_g_proxy_add_signal (_proxy,
                                 "NotificationClosed",
                                 G_TYPE_UINT,
                                 G_TYPE_UINT,
                                 G_TYPE_INVALID);
        dbus_g_proxy_add_signal (_proxy,
                                 "ActionInvoked",
                                 G_TYPE_UINT,
                                 G_TYPE_STRING,
                                 G_TYPE_INVALID);

        if (!_notify_update_spec_version ()) {
               return NULL;
        }

        return _proxy;
}

/**
 * notify_get_server_caps:
 *
 * Queries the server for its capabilities and returns them in a #GList.
 *
 * Returns: A #GList of server capability strings.
 */
GList *
notify_get_server_caps (void)
{
        GError         *error;
        char          **caps;
        char          **cap;
        GList          *result;
        DBusGProxy     *proxy;

        caps = NULL;
        result = NULL;

        proxy = _notify_get_g_proxy ();
        if (proxy == NULL) {
                return NULL;
        }

        error = NULL;
        if (!dbus_g_proxy_call (proxy,
                                "GetCapabilities",
                                &error,
                                G_TYPE_INVALID,
                                G_TYPE_STRV,
                                &caps,
                                G_TYPE_INVALID)) {
                g_error_free (error);
                return NULL;
        }

        for (cap = caps; *cap != NULL; cap++) {
                result = g_list_append (result, g_strdup (*cap));
        }

        g_strfreev (caps);

        return result;
}

/**
 * notify_get_server_info:
 * @ret_name: The resulting server name.
 * @ret_vendor: The resulting server vendor.
 * @ret_version: The resulting server version.
 * @ret_spec_version: The resulting version of the specification the server is
 *                    compliant with.
 *
 * Queries the server for its information, specifically, the name, vendor,
 * server version, and the version of the notifications specification that it
 * is compliant with.
 *
 * Returns: %TRUE if successful, and the variables passed will be set. %FALSE
 *          on failure.
 */
gboolean
notify_get_server_info (char **ret_name,
                        char **ret_vendor,
                        char **ret_version,
                        char **ret_spec_version)
{
        GError         *error;
        DBusGProxy     *proxy;
        char           *name;
        char           *vendor;
        char           *version;
        char           *spec_version;

        proxy = _notify_get_g_proxy ();
        if (proxy == NULL) {
                return FALSE;
        }

        error = NULL;
        if (!dbus_g_proxy_call (proxy,
                                "GetServerInformation",
                                &error,
                                G_TYPE_INVALID,
                                G_TYPE_STRING, &name,
                                G_TYPE_STRING, &vendor,
                                G_TYPE_STRING, &version,
                                G_TYPE_STRING, &spec_version,
                                G_TYPE_INVALID)) {
                g_error_free (error);
                return FALSE;
        }

        if (ret_name != NULL) {
                *ret_name = name;
        } else {
                g_free (name);
        }

        if (ret_vendor != NULL) {
                *ret_vendor = vendor;
        } else {
                g_free (vendor);
        }

        if (ret_version != NULL) {
                *ret_version = version;
        } else {
                g_free (version);
        }

        if (spec_version != NULL) {
                *ret_spec_version = spec_version;
        } else {
                g_free (spec_version);
        }

        return TRUE;
}

void
_notify_cache_add_notification (NotifyNotification *n)
{
        _active_notifications = g_list_prepend (_active_notifications, n);
}

void
_notify_cache_remove_notification (NotifyNotification *n)
{
        _active_notifications = g_list_remove (_active_notifications, n);
}
