/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Michael J. Chudobiak <mjc@avtechpulse.com>
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

#ifndef __GSD_HOUSEKEEPING_PLUGIN_H__
#define __GSD_HOUSEKEEPING_PLUGIN_H__

#include <glib.h>
#include <glib-object.h>
#include <gmodule.h>

#include "mate-settings-plugin.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GSD_TYPE_HOUSEKEEPING_PLUGIN                (gsd_housekeeping_plugin_get_type ())
#define GSD_HOUSEKEEPING_PLUGIN(o)                  (G_TYPE_CHECK_INSTANCE_CAST ((o), GSD_TYPE_HOUSEKEEPING_PLUGIN, GsdHousekeepingPlugin))
#define GSD_HOUSEKEEPING_PLUGIN_CLASS(k)            (G_TYPE_CHECK_CLASS_CAST((k), GSD_TYPE_HOUSEKEEPING_PLUGIN, GsdHousekeepingPluginClass))
#define GSD_IS_HOUSEKEEPING_PLUGIN(o)               (G_TYPE_CHECK_INSTANCE_TYPE ((o), GSD_TYPE_HOUSEKEEPING_PLUGIN))
#define GSD_IS_HOUSEKEEPING_PLUGIN_CLASS(k)         (G_TYPE_CHECK_CLASS_TYPE ((k), GSD_TYPE_HOUSEKEEPING_PLUGIN))
#define GSD_HOUSEKEEPING_PLUGIN_GET_CLASS(o)        (G_TYPE_INSTANCE_GET_CLASS ((o), GSD_TYPE_HOUSEKEEPING_PLUGIN, GsdHousekeepingPluginClass))

typedef struct GsdHousekeepingPluginPrivate GsdHousekeepingPluginPrivate;

typedef struct {
        MateSettingsPlugin		 parent;
        GsdHousekeepingPluginPrivate	*priv;
} GsdHousekeepingPlugin;

typedef struct {
        MateSettingsPluginClass parent_class;
} GsdHousekeepingPluginClass;

GType   gsd_housekeeping_plugin_get_type		(void) G_GNUC_CONST;

/* All the plugins must implement this function */
G_MODULE_EXPORT GType register_mate_settings_plugin	(GTypeModule *module);

#ifdef __cplusplus
}
#endif

#endif /* __GSD_HOUSEKEEPING_PLUGIN_H__ */
