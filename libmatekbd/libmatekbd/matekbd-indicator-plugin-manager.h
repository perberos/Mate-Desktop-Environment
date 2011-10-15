/*
 * Copyright (C) 2006 Sergey V. Udaltsov <svu@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GSWITCHIT_PLUGIN_MANAGER_H__
#define __GSWITCHIT_PLUGIN_MANAGER_H__

#include <gmodule.h>
#include <libmatekbd/matekbd-indicator-plugin.h>

typedef struct _MatekbdIndicatorPluginManager {
	GHashTable *all_plugin_recs;
	GSList *inited_plugin_recs;
} MatekbdIndicatorPluginManager;

typedef struct _MatekbdIndicatorPluginManagerRecord {
	const char *full_path;
	GModule *module;
	const MatekbdIndicatorPlugin *plugin;
} MatekbdIndicatorPluginManagerRecord;

extern void
 matekbd_indicator_plugin_manager_init (MatekbdIndicatorPluginManager * manager);

extern void
 matekbd_indicator_plugin_manager_term (MatekbdIndicatorPluginManager * manager);

extern void
 matekbd_indicator_plugin_manager_init_enabled_plugins (MatekbdIndicatorPluginManager * manager,
						     MatekbdIndicatorPluginContainer
						     * pc,
						     GSList *
						     enabled_plugins);

extern void
 matekbd_indicator_plugin_manager_term_initialized_plugins (MatekbdIndicatorPluginManager * manager);

extern void
 matekbd_indicator_plugin_manager_toggle_plugins (MatekbdIndicatorPluginManager * manager,
					       MatekbdIndicatorPluginContainer
					       * pc,
					       GSList * enabled_plugins);

extern const MatekbdIndicatorPlugin
    *
matekbd_indicator_plugin_manager_get_plugin (MatekbdIndicatorPluginManager *
					  manager, const char *full_path);

extern void
 matekbd_indicator_plugin_manager_promote_plugin (MatekbdIndicatorPluginManager * manager,
					       GSList * enabled_plugins,
					       const char *full_path);

extern void
 matekbd_indicator_plugin_manager_demote_plugin (MatekbdIndicatorPluginManager * manager,
					      GSList * enabled_plugins,
					      const char *full_path);

extern void
 matekbd_indicator_plugin_manager_enable_plugin (MatekbdIndicatorPluginManager * manager,
					      GSList ** enabled_plugins,
					      const char *full_path);

extern void
 matekbd_indicator_plugin_manager_disable_plugin (MatekbdIndicatorPluginManager * manager,
					       GSList ** enabled_plugins,
					       const char *full_path);

extern void
 matekbd_indicator_plugin_manager_configure_plugin (MatekbdIndicatorPluginManager * manager,
						 MatekbdIndicatorPluginContainer
						 * pc,
						 const char *full_path,
						 GtkWindow * parent);

/* actual calling plugin notification methods */

extern void
 matekbd_indicator_plugin_manager_group_changed (MatekbdIndicatorPluginManager * manager,
					      GtkWidget * notebook,
					      int new_group);

extern void
 matekbd_indicator_plugin_manager_config_changed (MatekbdIndicatorPluginManager * manager,
					       MatekbdKeyboardConfig * from,
					       MatekbdKeyboardConfig * to);

extern int
 matekbd_indicator_plugin_manager_window_created (MatekbdIndicatorPluginManager * manager,
					       Window win, Window parent);

extern GtkWidget
    *
matekbd_indicator_plugin_manager_decorate_widget (MatekbdIndicatorPluginManager *
					       manager, GtkWidget * widget,
					       const gint group, const char
					       *group_description,
					       MatekbdKeyboardConfig *
					       config);

#endif
