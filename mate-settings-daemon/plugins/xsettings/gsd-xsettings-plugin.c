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
#include "gsd-xsettings-plugin.h"
#include "gsd-xsettings-manager.h"

struct MateXSettingsPluginPrivate {
        MateXSettingsManager *manager;
};

#define MATE_XSETTINGS_PLUGIN_GET_PRIVATE(object) (G_TYPE_INSTANCE_GET_PRIVATE ((object), MATE_TYPE_XSETTINGS_PLUGIN, MateXSettingsPluginPrivate))

MATE_SETTINGS_PLUGIN_REGISTER (MateXSettingsPlugin, mate_xsettings_plugin)

static void
mate_xsettings_plugin_init (MateXSettingsPlugin *plugin)
{
        plugin->priv = MATE_XSETTINGS_PLUGIN_GET_PRIVATE (plugin);

        g_debug ("MateXSettingsPlugin initializing");

        plugin->priv->manager = mate_xsettings_manager_new ();
}

static void
mate_xsettings_plugin_finalize (GObject *object)
{
        MateXSettingsPlugin *plugin;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MATE_IS_XSETTINGS_PLUGIN (object));

        g_debug ("MateXSettingsPlugin finalizing");

        plugin = MATE_XSETTINGS_PLUGIN (object);

        g_return_if_fail (plugin->priv != NULL);

        if (plugin->priv->manager != NULL) {
                g_object_unref (plugin->priv->manager);
        }

        G_OBJECT_CLASS (mate_xsettings_plugin_parent_class)->finalize (object);
}

static void
impl_activate (MateSettingsPlugin *plugin)
{
        gboolean res;
        GError  *error;

        g_debug ("Activating xsettings plugin");

        error = NULL;
        res = mate_xsettings_manager_start (MATE_XSETTINGS_PLUGIN (plugin)->priv->manager, &error);
        if (! res) {
                g_warning ("Unable to start xsettings manager: %s", error->message);
                g_error_free (error);
        }
}

static void
impl_deactivate (MateSettingsPlugin *plugin)
{
        g_debug ("Deactivating xsettings plugin");
        mate_xsettings_manager_stop (MATE_XSETTINGS_PLUGIN (plugin)->priv->manager);
}

static void
mate_xsettings_plugin_class_init (MateXSettingsPluginClass *klass)
{
        GObjectClass             *object_class = G_OBJECT_CLASS (klass);
        MateSettingsPluginClass *plugin_class = MATE_SETTINGS_PLUGIN_CLASS (klass);

        object_class->finalize = mate_xsettings_plugin_finalize;

        plugin_class->activate = impl_activate;
        plugin_class->deactivate = impl_deactivate;

        g_type_class_add_private (klass, sizeof (MateXSettingsPluginPrivate));
}
