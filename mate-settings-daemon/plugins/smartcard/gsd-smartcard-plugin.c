/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2010 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
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

#include <glib/gi18n-lib.h>
#include <gmodule.h>

#include <glib.h>
#include <glib-object.h>

#include <dbus/dbus-glib.h>

#include <mateconf/mateconf-client.h>

#include "mate-settings-plugin.h"
#include "gsd-smartcard-plugin.h"
#include "gsd-smartcard-manager.h"

struct GsdSmartcardPluginPrivate {
        GsdSmartcardManager *manager;
        DBusGConnection     *bus_connection;

        guint32              is_active : 1;
};

typedef enum
{
    GSD_SMARTCARD_REMOVE_ACTION_NONE,
    GSD_SMARTCARD_REMOVE_ACTION_LOCK_SCREEN,
    GSD_SMARTCARD_REMOVE_ACTION_FORCE_LOGOUT,
} GsdSmartcardRemoveAction;

#define SCREENSAVER_DBUS_NAME      "org.mate.ScreenSaver"
#define SCREENSAVER_DBUS_PATH      "/"
#define SCREENSAVER_DBUS_INTERFACE "org.mate.ScreenSaver"

#define SM_DBUS_NAME      "org.mate.SessionManager"
#define SM_DBUS_PATH      "/org/mate/SessionManager"
#define SM_DBUS_INTERFACE "org.mate.SessionManager"
#define SM_LOGOUT_MODE_FORCE 2

#define GSD_SMARTCARD_KEY "/desktop/mate/peripherals/smartcard"
#define KEY_REMOVE_ACTION GSD_SMARTCARD_KEY "/removal_action"

#define GSD_SMARTCARD_PLUGIN_GET_PRIVATE(object) (G_TYPE_INSTANCE_GET_PRIVATE ((object), GSD_TYPE_SMARTCARD_PLUGIN, GsdSmartcardPluginPrivate))

MATE_SETTINGS_PLUGIN_REGISTER (GsdSmartcardPlugin, gsd_smartcard_plugin);

static void
simulate_user_activity (GsdSmartcardPlugin *plugin)
{
        DBusGProxy *screensaver_proxy;

        g_debug ("GsdSmartcardPlugin telling screensaver about smart card insertion");
        screensaver_proxy = dbus_g_proxy_new_for_name (plugin->priv->bus_connection,
                                                       SCREENSAVER_DBUS_NAME,
                                                       SCREENSAVER_DBUS_PATH,
                                                       SCREENSAVER_DBUS_INTERFACE);

        dbus_g_proxy_call_no_reply (screensaver_proxy,
                                    "SimulateUserActivity",
                                    G_TYPE_INVALID, G_TYPE_INVALID);

        g_object_unref (screensaver_proxy);
}

static void
lock_screen (GsdSmartcardPlugin *plugin)
{
        DBusGProxy *screensaver_proxy;

        g_debug ("GsdSmartcardPlugin telling screensaver to lock screen");
        screensaver_proxy = dbus_g_proxy_new_for_name (plugin->priv->bus_connection,
                                                       SCREENSAVER_DBUS_NAME,
                                                       SCREENSAVER_DBUS_PATH,
                                                       SCREENSAVER_DBUS_INTERFACE);

        dbus_g_proxy_call_no_reply (screensaver_proxy,
                                    "Lock",
                                    G_TYPE_INVALID, G_TYPE_INVALID);

        g_object_unref (screensaver_proxy);
}

static void
force_logout (GsdSmartcardPlugin *plugin)
{
        DBusGProxy *sm_proxy;
        GError     *error;
        gboolean    res;

        g_debug ("GsdSmartcardPlugin telling session manager to force logout");
        sm_proxy = dbus_g_proxy_new_for_name (plugin->priv->bus_connection,
                                              SM_DBUS_NAME,
                                              SM_DBUS_PATH,
                                              SM_DBUS_INTERFACE);

        error = NULL;
        res = dbus_g_proxy_call (sm_proxy,
                                 "Logout",
                                 &error,
                                 G_TYPE_UINT, SM_LOGOUT_MODE_FORCE,
                                 G_TYPE_INVALID, G_TYPE_INVALID);

        if (! res) {
                g_warning ("GsdSmartcardPlugin Unable to force logout: %s", error->message);
                g_error_free (error);
        }

        g_object_unref (sm_proxy);
}

static void
gsd_smartcard_plugin_init (GsdSmartcardPlugin *plugin)
{
        plugin->priv = GSD_SMARTCARD_PLUGIN_GET_PRIVATE (plugin);

        g_debug ("GsdSmartcardPlugin initializing");

        plugin->priv->manager = gsd_smartcard_manager_new (NULL);
}

static void
gsd_smartcard_plugin_finalize (GObject *object)
{
        GsdSmartcardPlugin *plugin;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GSD_IS_SMARTCARD_PLUGIN (object));

        g_debug ("GsdSmartcardPlugin finalizing");

        plugin = GSD_SMARTCARD_PLUGIN (object);

        g_return_if_fail (plugin->priv != NULL);

        if (plugin->priv->manager != NULL) {
                g_object_unref (plugin->priv->manager);
        }

        G_OBJECT_CLASS (gsd_smartcard_plugin_parent_class)->finalize (object);
}

static void
smartcard_inserted_cb (GsdSmartcardManager *card_monitor,
                       GsdSmartcard        *card,
                       GsdSmartcardPlugin  *plugin)
{
        char *name;

        name = gsd_smartcard_get_name (card);
        g_debug ("GsdSmartcardPlugin smart card '%s' inserted", name);
        g_free (name);

        simulate_user_activity (plugin);
}

static gboolean
user_logged_in_with_smartcard (void)
{
    return g_getenv ("PKCS11_LOGIN_TOKEN_NAME") != NULL;
}

static GsdSmartcardRemoveAction
get_configured_remove_action (GsdSmartcardPlugin *plugin)
{
        MateConfClient *client;
        char *remove_action_string;
        GsdSmartcardRemoveAction remove_action;

        client = mateconf_client_get_default ();
        remove_action_string = mateconf_client_get_string (client,
                                                        KEY_REMOVE_ACTION, NULL);

        if (remove_action_string == NULL) {
                g_warning ("GsdSmartcardPlugin unable to get smartcard remove action");
                remove_action = GSD_SMARTCARD_REMOVE_ACTION_NONE;
        } else if (strcmp (remove_action_string, "none") == 0) {
                remove_action = GSD_SMARTCARD_REMOVE_ACTION_NONE;
        } else if (strcmp (remove_action_string, "lock_screen") == 0) {
                remove_action = GSD_SMARTCARD_REMOVE_ACTION_LOCK_SCREEN;
        } else if (strcmp (remove_action_string, "force_logout") == 0) {
                remove_action = GSD_SMARTCARD_REMOVE_ACTION_FORCE_LOGOUT;
        } else {
                g_warning ("GsdSmartcardPlugin unknown smartcard remove action");
                remove_action = GSD_SMARTCARD_REMOVE_ACTION_NONE;
        }

        g_object_unref (client);

        return remove_action;
}

static void
process_smartcard_removal (GsdSmartcardPlugin *plugin)
{
        GsdSmartcardRemoveAction remove_action;

        g_debug ("GsdSmartcardPlugin processing smartcard removal");
        remove_action = get_configured_remove_action (plugin);

        switch (remove_action)
        {
            case GSD_SMARTCARD_REMOVE_ACTION_NONE:
                return;
            case GSD_SMARTCARD_REMOVE_ACTION_LOCK_SCREEN:
                lock_screen (plugin);
                break;
            case GSD_SMARTCARD_REMOVE_ACTION_FORCE_LOGOUT:
                force_logout (plugin);
                break;
        }
}

static void
smartcard_removed_cb (GsdSmartcardManager *card_monitor,
                      GsdSmartcard        *card,
                      GsdSmartcardPlugin  *plugin)
{

        char *name;

        name = gsd_smartcard_get_name (card);
        g_debug ("GsdSmartcardPlugin smart card '%s' removed", name);
        g_free (name);

        if (!gsd_smartcard_is_login_card (card)) {
                g_debug ("GsdSmartcardPlugin removed smart card was not used to login");
                return;
        }

        process_smartcard_removal (plugin);
}

static void
impl_activate (MateSettingsPlugin *plugin)
{
        GError *error;
        GsdSmartcardPlugin *smartcard_plugin = GSD_SMARTCARD_PLUGIN (plugin);

        if (smartcard_plugin->priv->is_active) {
                g_debug ("GsdSmartcardPlugin Not activating smartcard plugin, because it's "
                         "already active");
                return;
        }

        if (!user_logged_in_with_smartcard ()) {
                g_debug ("GsdSmartcardPlugin Not activating smartcard plugin, because user didn't use "
                         " smartcard to log in");
                smartcard_plugin->priv->is_active = FALSE;
                return;
        }

        g_debug ("GsdSmartcardPlugin Activating smartcard plugin");

        error = NULL;
        smartcard_plugin->priv->bus_connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);

        if (smartcard_plugin->priv->bus_connection == NULL) {
                g_warning ("GsdSmartcardPlugin Unable to connect to session bus: %s", error->message);
                return;
        }

        if (!gsd_smartcard_manager_start (smartcard_plugin->priv->manager, &error)) {
                g_warning ("GsdSmartcardPlugin Unable to start smartcard manager: %s", error->message);
                g_error_free (error);
        }

        g_signal_connect (smartcard_plugin->priv->manager,
                          "smartcard-removed",
                          G_CALLBACK (smartcard_removed_cb), smartcard_plugin);

        g_signal_connect (smartcard_plugin->priv->manager,
                          "smartcard-inserted",
                          G_CALLBACK (smartcard_inserted_cb), smartcard_plugin);

        if (!gsd_smartcard_manager_login_card_is_inserted (smartcard_plugin->priv->manager)) {
                g_debug ("GsdSmartcardPlugin processing smartcard removal immediately user logged in with smartcard "
                         "and it's not inserted");
                process_smartcard_removal (smartcard_plugin);
        }

        smartcard_plugin->priv->is_active = TRUE;
}

static void
impl_deactivate (MateSettingsPlugin *plugin)
{
        GsdSmartcardPlugin *smartcard_plugin = GSD_SMARTCARD_PLUGIN (plugin);

        if (!smartcard_plugin->priv->is_active) {
                g_debug ("GsdSmartcardPlugin Not deactivating smartcard plugin, "
                         "because it's already inactive");
                return;
        }

        g_debug ("GsdSmartcardPlugin Deactivating smartcard plugin");

        gsd_smartcard_manager_stop (smartcard_plugin->priv->manager);

        g_signal_handlers_disconnect_by_func (smartcard_plugin->priv->manager,
                                              smartcard_removed_cb, smartcard_plugin);

        g_signal_handlers_disconnect_by_func (smartcard_plugin->priv->manager,
                                              smartcard_inserted_cb, smartcard_plugin);
        smartcard_plugin->priv->bus_connection = NULL;
        smartcard_plugin->priv->is_active = FALSE;
}

static void
gsd_smartcard_plugin_class_init (GsdSmartcardPluginClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        MateSettingsPluginClass *plugin_class = MATE_SETTINGS_PLUGIN_CLASS (klass);

        object_class->finalize = gsd_smartcard_plugin_finalize;

        plugin_class->activate = impl_activate;
        plugin_class->deactivate = impl_deactivate;

        g_type_class_add_private (klass, sizeof (GsdSmartcardPluginPrivate));
}
