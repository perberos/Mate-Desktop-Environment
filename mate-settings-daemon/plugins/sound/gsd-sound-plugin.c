/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Lennart Poettering <lennart@poettering.net>
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
#include "gsd-sound-plugin.h"
#include "gsd-sound-manager.h"

struct GsdSoundPluginPrivate {
        GsdSoundManager *manager;
};

#define GSD_SOUND_PLUGIN_GET_PRIVATE(object) (G_TYPE_INSTANCE_GET_PRIVATE ((object), GSD_TYPE_SOUND_PLUGIN, GsdSoundPluginPrivate))

MATE_SETTINGS_PLUGIN_REGISTER (GsdSoundPlugin, gsd_sound_plugin)

static void
gsd_sound_plugin_init (GsdSoundPlugin *plugin)
{
        plugin->priv = GSD_SOUND_PLUGIN_GET_PRIVATE (plugin);

        g_debug ("GsdSoundPlugin initializing");

        plugin->priv->manager = gsd_sound_manager_new ();
}

static void
gsd_sound_plugin_finalize (GObject *object)
{
        GsdSoundPlugin *plugin;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GSD_IS_SOUND_PLUGIN (object));

        g_debug ("GsdSoundPlugin finalizing");

        plugin = GSD_SOUND_PLUGIN (object);

        g_return_if_fail (plugin->priv != NULL);

        if (plugin->priv->manager != NULL)
                g_object_unref (plugin->priv->manager);

        G_OBJECT_CLASS (gsd_sound_plugin_parent_class)->finalize (object);
}

static void
impl_activate (MateSettingsPlugin *plugin)
{
        GError *error = NULL;

        g_debug ("Activating sound plugin");

        if (!gsd_sound_manager_start (GSD_SOUND_PLUGIN (plugin)->priv->manager, &error)) {
                g_warning ("Unable to start sound manager: %s", error->message);
                g_error_free (error);
        }
}

static void
impl_deactivate (MateSettingsPlugin *plugin)
{
        g_debug ("Deactivating sound plugin");
        gsd_sound_manager_stop (GSD_SOUND_PLUGIN (plugin)->priv->manager);
}

static void
gsd_sound_plugin_class_init (GsdSoundPluginClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        MateSettingsPluginClass *plugin_class = MATE_SETTINGS_PLUGIN_CLASS (klass);

        object_class->finalize = gsd_sound_plugin_finalize;

        plugin_class->activate = impl_activate;
        plugin_class->deactivate = impl_deactivate;

        g_type_class_add_private (klass, sizeof (GsdSoundPluginPrivate));
}
