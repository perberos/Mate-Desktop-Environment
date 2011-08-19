/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Plugin engine for Totem, heavily based on the code from Rhythmbox,
 * which is based heavily on the code from gedit.
 *
 * Copyright (C) 2002-2005 Paolo Maggi
 *               2006 James Livingston  <jrl@ids.org.au>
 *               2007 Bastien Nocera <hadess@hadess.net>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301  USA.
 *
 * Sunday 13th May 2007: Bastien Nocera: Add exception clause.
 * See license_change file for details.
 *
 */

#ifndef __TOTEM_PLUGINS_ENGINE_H__
#define __TOTEM_PLUGINS_ENGINE_H__

#include <glib.h>
#include <totem.h>

typedef struct _TotemPluginInfo TotemPluginInfo;

gboolean	 totem_plugins_engine_init 		(TotemObject *totem);
void		 totem_plugins_engine_shutdown 		(void);

void		 totem_plugins_engine_garbage_collect	(void);

GList*		totem_plugins_engine_get_plugins_list 	(void);

gboolean 	 totem_plugins_engine_activate_plugin 	(TotemPluginInfo *info);
gboolean 	 totem_plugins_engine_deactivate_plugin	(TotemPluginInfo *info);
gboolean 	 totem_plugins_engine_plugin_is_active 	(TotemPluginInfo *info);
gboolean 	 totem_plugins_engine_plugin_is_visible (TotemPluginInfo *info);

gboolean	 totem_plugins_engine_plugin_is_configurable
							(TotemPluginInfo *info);
void	 	 totem_plugins_engine_configure_plugin	(TotemPluginInfo *info,
							 GtkWindow *parent);

const gchar*	totem_plugins_engine_get_plugin_name	(TotemPluginInfo *info);
const gchar*	totem_plugins_engine_get_plugin_description
							(TotemPluginInfo *info);

const gchar**	totem_plugins_engine_get_plugin_authors	(TotemPluginInfo *info);
const gchar*	totem_plugins_engine_get_plugin_website	(TotemPluginInfo *info);
const gchar*	totem_plugins_engine_get_plugin_copyright
							(TotemPluginInfo *info);
GdkPixbuf *	totem_plugins_engine_get_plugin_icon	(TotemPluginInfo *info);

#endif  /* __TOTEM_PLUGINS_ENGINE_H__ */
