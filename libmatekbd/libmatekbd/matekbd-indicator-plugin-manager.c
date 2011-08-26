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

#include <config.h>

#include <string.h>

#include <libxklavier/xklavier.h>

#include <matekbd-indicator-plugin-manager.h>

#define FOREACH_INITED_PLUGIN() \
{ \
  GSList *prec; \
  for( prec = manager->inited_plugin_recs; prec != NULL; prec = prec->next ) \
  { \
    const MatekbdIndicatorPlugin *plugin = \
      ( ( MatekbdIndicatorPluginManagerRecord * ) ( prec->data ) )->plugin; \
    if( plugin != NULL ) \
    {

#define NEXT_INITED_PLUGIN() \
    } \
  } \
}

static void
matekbd_indicator_plugin_manager_add_plugins_dir (MatekbdIndicatorPluginManager *
					       manager,
					       const char *dirname)
{
	GDir *dir = g_dir_open (dirname, 0, NULL);
	const gchar *filename;
	const MatekbdIndicatorPlugin *plugin;

	if (dir == NULL)
		return;

	xkl_debug (100, "Scanning [%s]...\n", dirname);
	while ((filename = g_dir_read_name (dir)) != NULL) {
		gchar *full_path =
		    g_build_filename (dirname, filename, NULL);
		xkl_debug (100, "Loading plugin module [%s]...\n",
			   full_path);
		if (full_path != NULL) {
			GModule *module = g_module_open (full_path, 0);
			if (module != NULL) {
				gpointer get_plugin_func;
				if (g_module_symbol
				    (module, "GetPlugin",
				     &get_plugin_func)) {
					plugin =
					    ((MatekbdIndicatorPluginGetPluginFunc)
					     get_plugin_func) ();
					if (plugin != NULL) {
						MatekbdIndicatorPluginManagerRecord
						    * rec =
						    g_new0
						    (MatekbdIndicatorPluginManagerRecord,
						     1);
						xkl_debug (100,
							   "Loaded plugin from [%s]: [%s]/[%s]...\n",
							   full_path,
							   plugin->name,
							   plugin->description);
						rec->full_path = full_path;
						rec->module = module;
						rec->plugin = plugin;
						g_hash_table_insert
						    (manager->all_plugin_recs,
						     full_path, rec);
						continue;
					}
				} else
					xkl_debug (0,
						   "Bad plugin: [%s]\n",
						   full_path);
				g_module_close (module);
			} else
				xkl_debug (0, "Bad module: [%s], %s\n",
					   full_path, g_module_error ());
			g_free (full_path);
		}
	}
	g_dir_close (dir);
}

static void
matekbd_indicator_plugin_manager_load_all (MatekbdIndicatorPluginManager *
					manager)
{
	if (!g_module_supported ()) {
		xkl_debug (0, "Modules are not supported - no plugins!\n");
		return;
	}
	matekbd_indicator_plugin_manager_add_plugins_dir (manager,
						       SYS_PLUGIN_DIR);
}

static void
matekbd_indicator_plugin_manager_rec_term (MatekbdIndicatorPluginManagerRecord *
					rec, void *user_data)
{
	const MatekbdIndicatorPlugin *plugin = rec->plugin;
	if (plugin != NULL) {
		xkl_debug (100, "Terminating plugin: [%s]...\n",
			   plugin->name);
		if (plugin->term_callback)
			(*plugin->term_callback) ();
	}
}

static void
matekbd_indicator_plugin_manager_rec_destroy (MatekbdIndicatorPluginManagerRecord
					   * rec)
{
	xkl_debug (100, "Unloading plugin: [%s]...\n", rec->plugin->name);

	g_module_close (rec->module);
	g_free (rec);
}

void
matekbd_indicator_plugin_manager_init (MatekbdIndicatorPluginManager * manager)
{
	manager->all_plugin_recs =
	    g_hash_table_new_full (g_str_hash, g_str_equal,
				   (GDestroyNotify) g_free,
				   (GDestroyNotify)
				   matekbd_indicator_plugin_manager_rec_destroy);
	matekbd_indicator_plugin_manager_load_all (manager);
}

void
matekbd_indicator_plugin_manager_term (MatekbdIndicatorPluginManager * manager)
{
	matekbd_indicator_plugin_manager_term_initialized_plugins (manager);
	if (manager->all_plugin_recs != NULL) {
		g_hash_table_destroy (manager->all_plugin_recs);
		manager->all_plugin_recs = NULL;
	}
}

void
 matekbd_indicator_plugin_manager_init_enabled_plugins
    (MatekbdIndicatorPluginManager * manager,
     MatekbdIndicatorPluginContainer * pc, GSList * enabled_plugins) {
	GSList *plugin_name_node = enabled_plugins;
	if (manager->all_plugin_recs == NULL)
		return;
	xkl_debug (100, "Initializing all enabled plugins...\n");
	while (plugin_name_node != NULL) {
		const char *full_path = plugin_name_node->data;
		if (full_path != NULL) {
			MatekbdIndicatorPluginManagerRecord *rec =
			    (MatekbdIndicatorPluginManagerRecord *)
			    g_hash_table_lookup (manager->all_plugin_recs,
						 full_path);

			if (rec != NULL) {
				const MatekbdIndicatorPlugin *plugin =
				    rec->plugin;
				gboolean initialized = FALSE;
				xkl_debug (100,
					   "Initializing plugin: [%s] from [%s]...\n",
					   plugin->name, full_path);
				if (plugin->init_callback != NULL)
					initialized =
					    (*plugin->init_callback) (pc);
				else
					initialized = TRUE;

				manager->inited_plugin_recs =
				    g_slist_append
				    (manager->inited_plugin_recs, rec);
				xkl_debug (100,
					   "Plugin [%s] initialized: %d\n",
					   plugin->name, initialized);
			}
		}
		plugin_name_node = g_slist_next (plugin_name_node);
	}
}

void
 matekbd_indicator_plugin_manager_term_initialized_plugins
    (MatekbdIndicatorPluginManager * manager) {

	if (manager->inited_plugin_recs == NULL)
		return;
	g_slist_foreach (manager->inited_plugin_recs,
			 (GFunc) matekbd_indicator_plugin_manager_rec_term,
			 NULL);
	g_slist_free (manager->inited_plugin_recs);
	manager->inited_plugin_recs = NULL;
}

void
matekbd_indicator_plugin_manager_toggle_plugins (MatekbdIndicatorPluginManager *
					      manager,
					      MatekbdIndicatorPluginContainer
					      * pc,
					      GSList * enabled_plugins)
{
	matekbd_indicator_plugin_manager_term_initialized_plugins (manager);
	matekbd_indicator_plugin_manager_init_enabled_plugins (manager, pc,
							    enabled_plugins);
}

void
matekbd_indicator_plugin_manager_group_changed (MatekbdIndicatorPluginManager *
					     manager, GtkWidget * notebook,
					     int new_group)
{
	FOREACH_INITED_PLUGIN ();
	if (plugin->group_changed_callback)
		(*plugin->group_changed_callback) (notebook, new_group);
	NEXT_INITED_PLUGIN ();
}

void
matekbd_indicator_plugin_manager_config_changed (MatekbdIndicatorPluginManager *
					      manager,
					      MatekbdKeyboardConfig * from,
					      MatekbdKeyboardConfig * to)
{
	FOREACH_INITED_PLUGIN ();
	if (plugin->config_changed_callback)
		(*plugin->config_changed_callback) (from, to);
	NEXT_INITED_PLUGIN ();
}

const MatekbdIndicatorPlugin *
matekbd_indicator_plugin_manager_get_plugin (MatekbdIndicatorPluginManager *
					  manager, const char *full_path)
{
	MatekbdIndicatorPluginManagerRecord *rec =
	    (MatekbdIndicatorPluginManagerRecord *)
	    g_hash_table_lookup (manager->all_plugin_recs,
				 full_path);
	if (rec == NULL)
		return NULL;
	return rec->plugin;
}

void
matekbd_indicator_plugin_manager_promote_plugin (MatekbdIndicatorPluginManager *
					      manager,
					      GSList * enabled_plugins,
					      const char *full_path)
{
	GSList *the_node = enabled_plugins;
	GSList *prev_node = NULL;

	while (the_node != NULL) {
		if (!strcmp (the_node->data, full_path)) {
			if (prev_node != NULL) {
				char *tmp = (char *) prev_node->data;
				prev_node->data = the_node->data;
				the_node->data = tmp;
			}
			break;
		}
		prev_node = the_node;
		the_node = g_slist_next (the_node);
	}
}

void
matekbd_indicator_plugin_manager_demote_plugin (MatekbdIndicatorPluginManager *
					     manager,
					     GSList * enabled_plugins,
					     const char *full_path)
{
	GSList *the_node = g_slist_find_custom (enabled_plugins, full_path,
						(GCompareFunc) strcmp);
	if (the_node != NULL) {
		GSList *next_node = g_slist_next (the_node);
		if (next_node != NULL) {
			char *tmp = (char *) next_node->data;
			next_node->data = the_node->data;
			the_node->data = tmp;
		}
	}
}

void
matekbd_indicator_plugin_manager_enable_plugin (MatekbdIndicatorPluginManager *
					     manager,
					     GSList ** enabled_plugins,
					     const char *full_path)
{
	if (NULL !=
	    matekbd_indicator_plugin_manager_get_plugin (manager,
						      full_path)) {
		*enabled_plugins =
		    g_slist_append (*enabled_plugins,
				    (gpointer) g_strdup (full_path));
	}
}

void
matekbd_indicator_plugin_manager_disable_plugin (MatekbdIndicatorPluginManager *
					      manager,
					      GSList ** enabled_plugins,
					      const char *full_path)
{
	GSList *the_node =
	    g_slist_find_custom (*enabled_plugins, full_path,
				 (GCompareFunc) strcmp);
	if (the_node != NULL) {
		g_free (the_node->data);
		*enabled_plugins =
		    g_slist_delete_link (*enabled_plugins, the_node);
	}
}

int
matekbd_indicator_plugin_manager_window_created (MatekbdIndicatorPluginManager *
					      manager, Window win,
					      Window parent)
{
	FOREACH_INITED_PLUGIN ();
	if (plugin->window_created_callback) {
		int group_to_assign =
		    (*plugin->window_created_callback) (win, parent);
		if (group_to_assign != -1) {
			xkl_debug (100,
				   "Plugin [%s] assigned group %d to new window %ld\n",
				   plugin->name, group_to_assign, win);
			return group_to_assign;
		}
	}
	NEXT_INITED_PLUGIN ();
	return -1;
}

GtkWidget *
matekbd_indicator_plugin_manager_decorate_widget (MatekbdIndicatorPluginManager *
					       manager, GtkWidget * widget,
					       const gint group,
					       const char
					       *group_description,
					       MatekbdKeyboardConfig *
					       kbd_config)
{
	FOREACH_INITED_PLUGIN ();
	if (plugin->decorate_widget_callback) {
		GtkWidget *decorated_widget =
		    (*plugin->decorate_widget_callback) (widget, group,
							 group_description,
							 kbd_config);
		if (decorated_widget != NULL) {
			xkl_debug (100,
				   "Plugin [%s] decorated widget %p to %p\n",
				   plugin->name, widget, decorated_widget);
			return decorated_widget;
		}
	}
	NEXT_INITED_PLUGIN ();
	return NULL;
}

void
matekbd_indicator_plugin_manager_configure_plugin (MatekbdIndicatorPluginManager
						* manager,
						MatekbdIndicatorPluginContainer
						* pc,
						const char *full_path,
						GtkWindow * parent)
{
	const MatekbdIndicatorPlugin *plugin =
	    matekbd_indicator_plugin_manager_get_plugin (manager, full_path);
	if (plugin->configure_properties_callback != NULL)
		plugin->configure_properties_callback (pc, parent);
}

void
matekbd_indicator_plugin_container_init (MatekbdIndicatorPluginContainer * pc,
				      MateConfClient * conf_client)
{
	pc->conf_client = conf_client;
	g_object_ref (pc->conf_client);
}

void
matekbd_indicator_plugin_container_term (MatekbdIndicatorPluginContainer * pc)
{
	g_object_unref (pc->conf_client);
}
