/*
 * Copyright (C) 2006 Sergey V. Udaltsov <svu@mate.org>
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
#include <libmatekbd/gkbd-indicator-plugin.h>

typedef struct _GkbdIndicatorPluginManager {
	GHashTable *all_plugin_recs;
	GSList *inited_plugin_recs;
} GkbdIndicatorPluginManager;

typedef struct _GkbdIndicatorPluginManagerRecord {
	const char *full_path;
	GModule *module;
	const GkbdIndicatorPlugin *plugin;
} GkbdIndicatorPluginManagerRecord;

extern void
 gkbd_indicator_plugin_manager_init (GkbdIndicatorPluginManager * manager);

extern void
 gkbd_indicator_plugin_manager_term (GkbdIndicatorPluginManager * manager);

extern void
 gkbd_indicator_plugin_manager_init_enabled_plugins (GkbdIndicatorPluginManager * manager,
						     GkbdIndicatorPluginContainer
						     * pc,
						     GSList *
						     enabled_plugins);

extern void
 gkbd_indicator_plugin_manager_term_initialized_plugins (GkbdIndicatorPluginManager * manager);

extern void
 gkbd_indicator_plugin_manager_toggle_plugins (GkbdIndicatorPluginManager * manager,
					       GkbdIndicatorPluginContainer
					       * pc,
					       GSList * enabled_plugins);

extern const GkbdIndicatorPlugin
    *
gkbd_indicator_plugin_manager_get_plugin (GkbdIndicatorPluginManager *
					  manager, const char *full_path);

extern void
 gkbd_indicator_plugin_manager_promote_plugin (GkbdIndicatorPluginManager * manager,
					       GSList * enabled_plugins,
					       const char *full_path);

extern void
 gkbd_indicator_plugin_manager_demote_plugin (GkbdIndicatorPluginManager * manager,
					      GSList * enabled_plugins,
					      const char *full_path);

extern void
 gkbd_indicator_plugin_manager_enable_plugin (GkbdIndicatorPluginManager * manager,
					      GSList ** enabled_plugins,
					      const char *full_path);

extern void
 gkbd_indicator_plugin_manager_disable_plugin (GkbdIndicatorPluginManager * manager,
					       GSList ** enabled_plugins,
					       const char *full_path);

extern void
 gkbd_indicator_plugin_manager_configure_plugin (GkbdIndicatorPluginManager * manager,
						 GkbdIndicatorPluginContainer
						 * pc,
						 const char *full_path,
						 GtkWindow * parent);

/* actual calling plugin notification methods */

extern void
 gkbd_indicator_plugin_manager_group_changed (GkbdIndicatorPluginManager * manager,
					      GtkWidget * notebook,
					      int new_group);

extern void
 gkbd_indicator_plugin_manager_config_changed (GkbdIndicatorPluginManager * manager,
					       GkbdKeyboardConfig * from,
					       GkbdKeyboardConfig * to);

extern int
 gkbd_indicator_plugin_manager_window_created (GkbdIndicatorPluginManager * manager,
					       Window win, Window parent);

extern GtkWidget
    *
gkbd_indicator_plugin_manager_decorate_widget (GkbdIndicatorPluginManager *
					       manager, GtkWidget * widget,
					       const gint group, const char
					       *group_description,
					       GkbdKeyboardConfig *
					       config);

#endif
