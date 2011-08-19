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
#include "gsd-xrdb-plugin.h"
#include "gsd-xrdb-manager.h"

struct GsdXrdbPluginPrivate {
        GsdXrdbManager *manager;
};

#define GSD_XRDB_PLUGIN_GET_PRIVATE(object) (G_TYPE_INSTANCE_GET_PRIVATE ((object), GSD_TYPE_XRDB_PLUGIN, GsdXrdbPluginPrivate))

MATE_SETTINGS_PLUGIN_REGISTER (GsdXrdbPlugin, gsd_xrdb_plugin)

static void
gsd_xrdb_plugin_init (GsdXrdbPlugin *plugin)
{
        plugin->priv = GSD_XRDB_PLUGIN_GET_PRIVATE (plugin);

        g_debug ("GsdXrdbPlugin initializing");

        plugin->priv->manager = gsd_xrdb_manager_new ();
}

static void
gsd_xrdb_plugin_finalize (GObject *object)
{
        GsdXrdbPlugin *plugin;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GSD_IS_XRDB_PLUGIN (object));

        g_debug ("GsdXrdbPlugin finalizing");

        plugin = GSD_XRDB_PLUGIN (object);

        g_return_if_fail (plugin->priv != NULL);

        if (plugin->priv->manager != NULL) {
                g_object_unref (plugin->priv->manager);
        }

        G_OBJECT_CLASS (gsd_xrdb_plugin_parent_class)->finalize (object);
}

static void
impl_activate (MateSettingsPlugin *plugin)
{
        gboolean res;
        GError  *error;

        g_debug ("Activating xrdb plugin");

        error = NULL;
        res = gsd_xrdb_manager_start (GSD_XRDB_PLUGIN (plugin)->priv->manager, &error);
        if (! res) {
                g_warning ("Unable to start xrdb manager: %s", error->message);
                g_error_free (error);
        }
}

static void
impl_deactivate (MateSettingsPlugin *plugin)
{
        g_debug ("Deactivating xrdb plugin");
        gsd_xrdb_manager_stop (GSD_XRDB_PLUGIN (plugin)->priv->manager);
}

static void
gsd_xrdb_plugin_class_init (GsdXrdbPluginClass *klass)
{
        GObjectClass             *object_class = G_OBJECT_CLASS (klass);
        MateSettingsPluginClass *plugin_class = MATE_SETTINGS_PLUGIN_CLASS (klass);

        object_class->finalize = gsd_xrdb_plugin_finalize;

        plugin_class->activate = impl_activate;
        plugin_class->deactivate = impl_deactivate;

        g_type_class_add_private (klass, sizeof (GsdXrdbPluginPrivate));
}
