/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2002-2005 Paolo Maggi
 * Copyright (C) 2007      William Jon McCann <mccann@jhu.edu>
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
 * Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include "mate-settings-plugin.h"

G_DEFINE_TYPE (MateSettingsPlugin, mate_settings_plugin, G_TYPE_OBJECT)

static void
dummy (MateSettingsPlugin *plugin)
{
        /* Empty */
}

static void
mate_settings_plugin_class_init (MateSettingsPluginClass *klass)
{
        klass->activate = dummy;
        klass->deactivate = dummy;
}

static void
mate_settings_plugin_init (MateSettingsPlugin *plugin)
{
        /* Empty */
}

void
mate_settings_plugin_activate (MateSettingsPlugin *plugin)
{
        g_return_if_fail (MATE_IS_SETTINGS_PLUGIN (plugin));

        MATE_SETTINGS_PLUGIN_GET_CLASS (plugin)->activate (plugin);
}

void
mate_settings_plugin_deactivate  (MateSettingsPlugin *plugin)
{
        g_return_if_fail (MATE_IS_SETTINGS_PLUGIN (plugin));

        MATE_SETTINGS_PLUGIN_GET_CLASS (plugin)->deactivate (plugin);
}
