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

#ifndef __MATE_XSETTINGS_PLUGIN_H__
#define __MATE_XSETTINGS_PLUGIN_H__

#include <glib.h>
#include <glib-object.h>
#include <gmodule.h>

#include "mate-settings-plugin.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MATE_TYPE_XSETTINGS_PLUGIN                (mate_xsettings_plugin_get_type ())
#define MATE_XSETTINGS_PLUGIN(o)                  (G_TYPE_CHECK_INSTANCE_CAST ((o), MATE_TYPE_XSETTINGS_PLUGIN, MateXSettingsPlugin))
#define MATE_XSETTINGS_PLUGIN_CLASS(k)            (G_TYPE_CHECK_CLASS_CAST((k), MATE_TYPE_XSETTINGS_PLUGIN, MateXSettingsPluginClass))
#define MATE_IS_XSETTINGS_PLUGIN(o)               (G_TYPE_CHECK_INSTANCE_TYPE ((o), MATE_TYPE_XSETTINGS_PLUGIN))
#define MATE_IS_XSETTINGS_PLUGIN_CLASS(k)         (G_TYPE_CHECK_CLASS_TYPE ((k), MATE_TYPE_XSETTINGS_PLUGIN))
#define MATE_XSETTINGS_PLUGIN_GET_CLASS(o)        (G_TYPE_INSTANCE_GET_CLASS ((o), MATE_TYPE_XSETTINGS_PLUGIN, MateXSettingsPluginClass))

typedef struct MateXSettingsPluginPrivate MateXSettingsPluginPrivate;

typedef struct
{
        MateSettingsPlugin          parent;
        MateXSettingsPluginPrivate *priv;
} MateXSettingsPlugin;

typedef struct
{
        MateSettingsPluginClass parent_class;
} MateXSettingsPluginClass;

GType   mate_xsettings_plugin_get_type            (void) G_GNUC_CONST;

/* All the plugins must implement this function */
G_MODULE_EXPORT GType register_mate_settings_plugin (GTypeModule *module);

#ifdef __cplusplus
}
#endif

#endif /* __MATE_XSETTINGS_PLUGIN_H__ */
