#ifndef _MATE_BLUETOOTH_PLUGIN_H_
#define _MATE_BLUETOOTH_PLUGIN_H_

#include <gmodule.h>
#include <gtk/gtk.h>

/**
 * SECTION:bluetooth-plugin
 * @short_description: Bluetooth plug-in
 * @stability: Stable
 * @include: bluetooth-plugin.h
 *
 * Plug-ins can be used to extend set up wizards and preferences.
 **/

typedef struct _GbtPluginInfo GbtPluginInfo;
typedef struct _GbtPlugin GbtPlugin;

/**
 * GbtPluginInfo:
 * @id: A unique ID representing the plugin
 * @has_config_widget: Whether the passed Bluetooth address and UUIDs would have a configuration widget
 * @get_config_widgets: Returns the configuration widget for the passed Bluetooth address and UUIDs
 * @device_removed: Remove any configuration relating to the Bluetooth address passed
 *
 * A structure representing a mate-bluetooth wizard and properties plugin. You should also call GBT_INIT_PLUGIN() on the plugin structure to export it.
 **/
struct _GbtPluginInfo 
{
	const char                         *id;
	gboolean (* has_config_widget)     (const char *bdaddr, const char **uuids);
	GtkWidget * (* get_config_widgets) (const char *bdaddr, const char **uuids);
	void (* device_removed)            (const char *bdaddr);
};

/**
 * GbtPlugin:
 * @module: the #GModule for the opened shared library
 * @info: a #GbtPluginInfo structure
 *
 * A structure as used in mate-bluetooth.
 **/
struct _GbtPlugin
{
	GModule *module;
	GbtPluginInfo *info;
};

/**
* GBT_INIT_PLUGIN:
* @plugininfo: a #GbtPluginInfo structure representing the plugin
*
* Call this on an #GbtPluginInfo structure to make it available to mate-bluetooth.
**/
#define GBT_INIT_PLUGIN(plugininfo)					\
	gboolean gbt_init_plugin (GbtPlugin *plugin);			\
	G_MODULE_EXPORT gboolean					\
	gbt_init_plugin (GbtPlugin *plugin) {				\
		plugin->info = &(plugininfo);				\
		return TRUE;						\
	}

#endif /* _MATE_BLUETOOTH_PLUGIN_H_ */

