/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
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

#include "mate-settings-plugin.h"
#include "gsd-a11y-keyboard-plugin.h"
#include "gsd-a11y-keyboard-manager.h"

struct GsdA11yKeyboardPluginPrivate {
        GsdA11yKeyboardManager *manager;
};

#define GSD_A11Y_KEYBOARD_PLUGIN_GET_PRIVATE(object) (G_TYPE_INSTANCE_GET_PRIVATE ((object), GSD_TYPE_A11Y_KEYBOARD_PLUGIN, GsdA11yKeyboardPluginPrivate))

MATE_SETTINGS_PLUGIN_REGISTER (GsdA11yKeyboardPlugin, gsd_a11y_keyboard_plugin)

static void
gsd_a11y_keyboard_plugin_init (GsdA11yKeyboardPlugin *plugin)
{
        plugin->priv = GSD_A11Y_KEYBOARD_PLUGIN_GET_PRIVATE (plugin);

        g_debug ("GsdA11yKeyboardPlugin initializing");

        plugin->priv->manager = gsd_a11y_keyboard_manager_new ();
}

static void
gsd_a11y_keyboard_plugin_finalize (GObject *object)
{
        GsdA11yKeyboardPlugin *plugin;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GSD_IS_A11Y_KEYBOARD_PLUGIN (object));

        g_debug ("GsdA11yKeyboardPlugin finalizing");

        plugin = GSD_A11Y_KEYBOARD_PLUGIN (object);

        g_return_if_fail (plugin->priv != NULL);

        if (plugin->priv->manager != NULL) {
                g_object_unref (plugin->priv->manager);
        }

        G_OBJECT_CLASS (gsd_a11y_keyboard_plugin_parent_class)->finalize (object);
}

static void
impl_activate (MateSettingsPlugin *plugin)
{
        gboolean res;
        GError  *error;

        g_debug ("Activating a11y_keyboard plugin");

        error = NULL;
        res = gsd_a11y_keyboard_manager_start (GSD_A11Y_KEYBOARD_PLUGIN (plugin)->priv->manager, &error);
        if (! res) {
                g_warning ("Unable to start a11y_keyboard manager: %s", error->message);
                g_error_free (error);
        }
}

static void
impl_deactivate (MateSettingsPlugin *plugin)
{
        g_debug ("Deactivating a11y_keyboard plugin");
        gsd_a11y_keyboard_manager_stop (GSD_A11Y_KEYBOARD_PLUGIN (plugin)->priv->manager);
}

static void
gsd_a11y_keyboard_plugin_class_init (GsdA11yKeyboardPluginClass *klass)
{
        GObjectClass           *object_class = G_OBJECT_CLASS (klass);
        MateSettingsPluginClass *plugin_class = MATE_SETTINGS_PLUGIN_CLASS (klass);

        object_class->finalize = gsd_a11y_keyboard_plugin_finalize;

        plugin_class->activate = impl_activate;
        plugin_class->deactivate = impl_deactivate;

        g_type_class_add_private (klass, sizeof (GsdA11yKeyboardPluginPrivate));
}
