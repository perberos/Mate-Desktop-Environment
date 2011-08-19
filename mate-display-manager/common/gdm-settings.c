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

#include "gdm-settings.h"
#include "gdm-settings-glue.h"

#include "gdm-settings-desktop-backend.h"

#include "gdm-marshal.h"

#define GDM_DBUS_PATH         "/org/mate/DisplayManager"
#define GDM_SETTINGS_DBUS_PATH GDM_DBUS_PATH "/Settings"
#define GDM_SETTINGS_DBUS_NAME "org.mate.DisplayManager.Settings"

#define GDM_SETTINGS_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GDM_TYPE_SETTINGS, GdmSettingsPrivate))

struct GdmSettingsPrivate
{
        DBusGConnection    *connection;
        GdmSettingsBackend *backend;
};

enum {
        VALUE_CHANGED,
        LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0, };

static void     gdm_settings_class_init (GdmSettingsClass *klass);
static void     gdm_settings_init       (GdmSettings      *settings);
static void     gdm_settings_finalize   (GObject          *object);

static gpointer settings_object = NULL;

G_DEFINE_TYPE (GdmSettings, gdm_settings, G_TYPE_OBJECT)

GQuark
gdm_settings_error_quark (void)
{
        static GQuark ret = 0;
        if (ret == 0) {
                ret = g_quark_from_static_string ("gdm_settings_error");
        }

        return ret;
}

/*
dbus-send --system --print-reply --dest=org.mate.DisplayManager /org/mate/DisplayManager/Settings org.mate.DisplayManager.Settings.GetValue string:"xdmcp/Enable"
*/

gboolean
gdm_settings_get_value (GdmSettings *settings,
                        const char  *key,
                        char       **value,
                        GError     **error)
{
        GError  *local_error;
        gboolean res;

        g_return_val_if_fail (GDM_IS_SETTINGS (settings), FALSE);
        g_return_val_if_fail (key != NULL, FALSE);

        local_error = NULL;
        res = gdm_settings_backend_get_value (settings->priv->backend,
                                              key,
                                              value,
                                              &local_error);
        if (! res) {
                g_propagate_error (error, local_error);
        }

        return res;
}

/*
dbus-send --system --print-reply --dest=org.mate.DisplayManager /org/mate/DisplayManager/Settings org.mate.DisplayManager.Settings.SetValue string:"xdmcp/Enable" string:"false"
*/

gboolean
gdm_settings_set_value (GdmSettings *settings,
                        const char  *key,
                        const char  *value,
                        GError     **error)
{
        GError  *local_error;
        gboolean res;

        g_return_val_if_fail (GDM_IS_SETTINGS (settings), FALSE);
        g_return_val_if_fail (key != NULL, FALSE);

        g_debug ("Setting value %s", key);

        local_error = NULL;
        res = gdm_settings_backend_set_value (settings->priv->backend,
                                              key,
                                              value,
                                              &local_error);
        if (! res) {
                g_propagate_error (error, local_error);
        }

        return res;
}

static gboolean
register_settings (GdmSettings *settings)
{
        GError *error = NULL;

        error = NULL;
        settings->priv->connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
        if (settings->priv->connection == NULL) {
                if (error != NULL) {
                        g_critical ("error getting system bus: %s", error->message);
                        g_error_free (error);
                }
                exit (1);
        }

        dbus_g_connection_register_g_object (settings->priv->connection, GDM_SETTINGS_DBUS_PATH, G_OBJECT (settings));

        return TRUE;
}

/*
dbus-send --system --print-reply --dest=org.mate.DisplayManager /org/mate/DisplayManager/Settings org.freedesktop.DBus.Introspectable.Introspect
*/

static void
gdm_settings_class_init (GdmSettingsClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = gdm_settings_finalize;

        signals [VALUE_CHANGED] =
                g_signal_new ("value-changed",
                              G_TYPE_FROM_CLASS (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GdmSettingsClass, value_changed),
                              NULL,
                              NULL,
                              gdm_marshal_VOID__STRING_STRING_STRING,
                              G_TYPE_NONE,
                              3,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING);

        g_type_class_add_private (klass, sizeof (GdmSettingsPrivate));

        dbus_g_object_type_install_info (GDM_TYPE_SETTINGS, &dbus_glib_gdm_settings_object_info);
}

static void
backend_value_changed (GdmSettingsBackend *backend,
                       const char         *key,
                       const char         *old_value,
                       const char         *new_value,
                       GdmSettings        *settings)
{
        g_debug ("Emitting value-changed %s %s %s", key, old_value, new_value);
        /* just proxy it */
        g_signal_emit (settings,
                       signals [VALUE_CHANGED],
                       0,
                       key,
                       old_value,
                       new_value);
}

static void
gdm_settings_init (GdmSettings *settings)
{
        settings->priv = GDM_SETTINGS_GET_PRIVATE (settings);

        settings->priv->backend = gdm_settings_desktop_backend_new ();
        g_signal_connect (settings->priv->backend,
                          "value-changed",
                          G_CALLBACK (backend_value_changed),
                          settings);
}

static void
gdm_settings_finalize (GObject *object)
{
        GdmSettings *settings;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GDM_IS_SETTINGS (object));

        settings = GDM_SETTINGS (object);

        g_return_if_fail (settings->priv != NULL);

        if (settings->priv->backend != NULL) {
                g_object_unref (settings->priv->backend);
        }

        G_OBJECT_CLASS (gdm_settings_parent_class)->finalize (object);
}

GdmSettings *
gdm_settings_new (void)
{
        if (settings_object != NULL) {
                g_object_ref (settings_object);
        } else {
                gboolean res;

                settings_object = g_object_new (GDM_TYPE_SETTINGS, NULL);
                g_object_add_weak_pointer (settings_object,
                                           (gpointer *) &settings_object);
                res = register_settings (settings_object);
                if (! res) {
                        g_warning ("Unable to register settings");
                        g_object_unref (settings_object);
                        return NULL;
                }
        }

        return GDM_SETTINGS (settings_object);
}
