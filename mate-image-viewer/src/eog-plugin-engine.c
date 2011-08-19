/* Eye Of Mate - EOG Plugin Manager
 *
 * Copyright (C) 2007 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@mate.org>
 *
 * Based on gedit code (gedit/gedit-module.c) by:
 * 	- Paolo Maggi <paolo@mate.org>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include "eog-plugin-engine.h"
#include "eog-plugin.h"
#include "eog-module.h"
#include "eog-debug.h"
#include "eog-application.h"
#include "eog-config-keys.h"
#include "eog-util.h"

#include <glib/gi18n.h>
#include <glib.h>
#include <mateconf/mateconf-client.h>

#ifdef ENABLE_PYTHON
#include "eog-python-module.h"
#endif

#define USER_EOG_PLUGINS_LOCATION "plugins/"

#define EOG_PLUGINS_ENGINE_BASE_KEY "/apps/eog/plugins"

#define PLUGIN_EXT	".eog-plugin"

typedef enum {
	EOG_PLUGIN_LOADER_C,
	EOG_PLUGIN_LOADER_PY,
} EogPluginLoader;

struct _EogPluginInfo
{
	gchar             *file;

	gchar             *location;
	EogPluginLoader    loader;
	GTypeModule       *module;

	gchar             *name;
	gchar             *desc;
	gchar             *icon_name;
	gchar            **authors;
	gchar             *copyright;
	gchar             *website;

	EogPlugin         *plugin;

	gint               active : 1;

	/* A plugin is unavailable if it is not possible to activate it
	   due to an error loading the plugin module (e.g. for Python plugins
	   when the interpreter has not been correctly initializated) */
	gint               available : 1;
};

static void	eog_plugin_engine_active_plugins_changed (MateConfClient *client,
							  guint cnxn_id,
							  MateConfEntry *entry,
							  gpointer user_data);

static GList *eog_plugins_list = NULL;

static MateConfClient *eog_plugin_engine_mateconf_client = NULL;

static GSList *active_plugins = NULL;

static void
eog_plugin_info_free (EogPluginInfo *info)
{
	if (info->plugin != NULL) {
	       	eog_debug_message (DEBUG_PLUGINS, "Unref plugin %s", info->name);

		g_object_unref (info->plugin);
	}

	g_free (info->file);
	g_free (info->location);
	g_free (info->name);
	g_free (info->desc);
	g_free (info->icon_name);
	g_free (info->website);
	g_free (info->copyright);
	g_strfreev (info->authors);

	g_free (info);
}

static EogPluginInfo *
eog_plugin_engine_load (const gchar *file)
{
	EogPluginInfo *info;
	GKeyFile *plugin_file = NULL;
	gchar *str;

	g_return_val_if_fail (file != NULL, NULL);

	eog_debug_message (DEBUG_PLUGINS, "Loading plugin: %s", file);

	info = g_new0 (EogPluginInfo, 1);
	info->file = g_strdup (file);

	plugin_file = g_key_file_new ();

	if (!g_key_file_load_from_file (plugin_file, file, G_KEY_FILE_NONE, NULL)) {
		g_warning ("Bad plugin file: %s", file);

		goto error;
	}

	if (!g_key_file_has_key (plugin_file,
			   	 "Eog Plugin",
				 "IAge",
				 NULL))	{
		eog_debug_message (DEBUG_PLUGINS,
				   "IAge key does not exist in file: %s", file);

		goto error;
	}

	/* Check IAge=2 */
	if (g_key_file_get_integer (plugin_file,
				    "Eog Plugin",
				    "IAge",
				    NULL) != 2) {
		eog_debug_message (DEBUG_PLUGINS,
				   "Wrong IAge in file: %s", file);

		goto error;
	}

	/* Get Location */
	str = g_key_file_get_string (plugin_file,
				     "Eog Plugin",
				     "Module",
				     NULL);

	if ((str != NULL) && (*str != '\0')) {
		info->location = str;
	} else {
		g_warning ("Could not find 'Module' in %s", file);

		goto error;
	}

	/* Get the loader for this plugin */
	str = g_key_file_get_string (plugin_file,
				     "Eog Plugin",
				     "Loader",
				     NULL);

	if (str && strcmp(str, "python") == 0) {
		info->loader = EOG_PLUGIN_LOADER_PY;

#ifndef ENABLE_PYTHON
		g_warning ("Cannot load Python plugin '%s' since eog was not "
			   "compiled with Python support.", file);

		goto error;
#endif

	} else {
		info->loader = EOG_PLUGIN_LOADER_C;
	}

	g_free (str);

	/* Get Name */
	str = g_key_file_get_locale_string (plugin_file,
					    "Eog Plugin",
					    "Name",
					    NULL, NULL);
	if (str) {
		info->name = str;
	} else {
		g_warning ("Could not find 'Name' in %s", file);

		goto error;
	}

	/* Get Description */
	str = g_key_file_get_locale_string (plugin_file,
					    "Eog Plugin",
					    "Description",
					    NULL, NULL);
	if (str) {
		info->desc = str;
	} else {
		eog_debug_message (DEBUG_PLUGINS, "Could not find 'Description' in %s", file);
 	}

	/* Get Icon */
	str = g_key_file_get_locale_string (plugin_file,
					    "Eog Plugin",
					    "Icon",
					    NULL, NULL);
	if (str) {
		info->icon_name = str;
	} else {
		eog_debug_message (DEBUG_PLUGINS, "Could not find 'Icon' in %s, "
						  "using 'eog-plugin'", file);
	}

	/* Get Authors */
	info->authors = g_key_file_get_string_list (plugin_file,
						    "Eog Plugin",
						    "Authors",
						    NULL,
						    NULL);

	if (info->authors == NULL)
		eog_debug_message (DEBUG_PLUGINS, "Could not find 'Authors' in %s", file);


	/* Get Copyright */
	str = g_key_file_get_string (plugin_file,
				     "Eog Plugin",
				     "Copyright",
				     NULL);
	if (str) {
		info->copyright = str;
	} else {
		eog_debug_message (DEBUG_PLUGINS, "Could not find 'Copyright' in %s", file);
	}

	/* Get Website */
	str = g_key_file_get_string (plugin_file,
				     "Eog Plugin",
				     "Website",
				     NULL);
	if (str) {
		info->website = str;
	} else {
		eog_debug_message (DEBUG_PLUGINS, "Could not find 'Website' in %s", file);
	}

	g_key_file_free (plugin_file);

	/* If we know nothing about the availability of the plugin,
	   set it as available */
	info->available = TRUE;

	return info;

error:
	g_free (info->file);
	g_free (info->location);
	g_free (info->name);
	g_free (info);
	g_key_file_free (plugin_file);

	return NULL;
}

static gint
compare_plugin_info (EogPluginInfo *info1,
		     EogPluginInfo *info2)
{
	return strcmp (info1->location, info2->location);
}

static void
eog_plugin_engine_load_dir (const gchar *dir)
{
	GError *error = NULL;
	GDir *d;
	const gchar *dirent;

	if (!g_file_test (dir, G_FILE_TEST_IS_DIR)) {
		return;
	}

	g_return_if_fail (eog_plugin_engine_mateconf_client != NULL);

	eog_debug_message (DEBUG_PLUGINS, "DIR: %s", dir);

	d = g_dir_open (dir, 0, &error);

	if (!d)	{
		g_warning ("%s", error->message);
		g_error_free (error);

		return;
	}

	while ((dirent = g_dir_read_name (d))) {
		if (g_str_has_suffix (dirent, PLUGIN_EXT)) {
			gchar *plugin_file;
			EogPluginInfo *info;

			plugin_file = g_build_filename (dir, dirent, NULL);
			info = eog_plugin_engine_load (plugin_file);
			g_free (plugin_file);

			if (info == NULL)
				continue;

			/* If a plugin with this name has already been loaded
			 * drop this one (user plugins override system plugins) */
			if (g_list_find_custom (eog_plugins_list,
						info,
						(GCompareFunc)compare_plugin_info) != NULL) {
				g_warning ("Two or more plugins named '%s'. "
					   "Only the first will be considered.\n",
					   info->location);

				eog_plugin_info_free (info);

				continue;
			}

			/* Actually, the plugin will be activated when reactivate_all
			 * will be called for the first time. */
			info->active = (g_slist_find_custom (active_plugins,
							     info->location,
							     (GCompareFunc)strcmp) != NULL);

			eog_plugins_list = g_list_prepend (eog_plugins_list, info);

			eog_debug_message (DEBUG_PLUGINS, "Plugin %s loaded", info->name);
		}
	}

	eog_plugins_list = g_list_reverse (eog_plugins_list);

	g_dir_close (d);
}

static void
eog_plugin_engine_load_all (void)
{
	gchar *pdir;

	pdir = g_build_filename (eog_util_dot_dir (),
                                 USER_EOG_PLUGINS_LOCATION, NULL);

	/* Load user's plugins */
	eog_plugin_engine_load_dir (pdir);

	g_free (pdir);

	/* Load system plugins */
	eog_plugin_engine_load_dir (EOG_PLUGIN_DIR "/");
}

gboolean
eog_plugin_engine_init (void)
{
	eog_debug (DEBUG_PLUGINS);

	g_return_val_if_fail (eog_plugins_list == NULL, FALSE);

	if (!g_module_supported ()) {
		g_warning ("eog is not able to initialize the plugins engine.");

		return FALSE;
	}

	eog_plugin_engine_mateconf_client = mateconf_client_get_default ();

	g_return_val_if_fail (eog_plugin_engine_mateconf_client != NULL, FALSE);

	mateconf_client_add_dir (eog_plugin_engine_mateconf_client,
			      EOG_PLUGINS_ENGINE_BASE_KEY,
			      MATECONF_CLIENT_PRELOAD_ONELEVEL,
			      NULL);

	mateconf_client_notify_add (eog_plugin_engine_mateconf_client,
				 EOG_CONF_PLUGINS_ACTIVE_PLUGINS,
				 eog_plugin_engine_active_plugins_changed,
				 NULL, NULL, NULL);

	active_plugins = mateconf_client_get_list (eog_plugin_engine_mateconf_client,
						EOG_CONF_PLUGINS_ACTIVE_PLUGINS,
						MATECONF_VALUE_STRING,
						NULL);

	eog_plugin_engine_load_all ();

	return TRUE;
}

void
eog_plugin_engine_garbage_collect (void)
{
#ifdef ENABLE_PYTHON
	eog_python_garbage_collect ();
#endif
}

void
eog_plugin_engine_shutdown (void)
{
	GList *pl;

	eog_debug (DEBUG_PLUGINS);

#ifdef ENABLE_PYTHON
	/* Note: that this may cause finalization of objects (typically
	 * the EogWindow) by running the garbage collector. Since some
	 * of the plugin may have installed callbacks upon object
	 * finalization (typically they need to free the WindowData)
	 * it must run before we get rid of the plugins.
	 */
	eog_python_shutdown ();
#endif

	g_return_if_fail (eog_plugin_engine_mateconf_client != NULL);

	for (pl = eog_plugins_list; pl; pl = pl->next) {
		EogPluginInfo *info = (EogPluginInfo*) pl->data;

		eog_plugin_info_free (info);
	}

	g_slist_foreach (active_plugins, (GFunc)g_free, NULL);
	g_slist_free (active_plugins);

	active_plugins = NULL;

	g_list_free (eog_plugins_list);
	eog_plugins_list = NULL;

	g_object_unref (eog_plugin_engine_mateconf_client);
	eog_plugin_engine_mateconf_client = NULL;
}

const GList *
eog_plugin_engine_get_plugins_list (void)
{
	eog_debug (DEBUG_PLUGINS);

	return eog_plugins_list;
}

static gboolean
load_plugin_module (EogPluginInfo *info)
{
	gchar *path;
	gchar *dirname;

	eog_debug (DEBUG_PLUGINS);

	g_return_val_if_fail (info != NULL, FALSE);
	g_return_val_if_fail (info->file != NULL, FALSE);
	g_return_val_if_fail (info->location != NULL, FALSE);
	g_return_val_if_fail (info->plugin == NULL, FALSE);
	g_return_val_if_fail (info->available, FALSE);

	switch (info->loader) {
	case EOG_PLUGIN_LOADER_C:
		dirname = g_path_get_dirname (info->file);
		g_return_val_if_fail (dirname != NULL, FALSE);

		path = g_module_build_path (dirname, info->location);

		g_free (dirname);

		g_return_val_if_fail (path != NULL, FALSE);

		info->module = G_TYPE_MODULE (eog_module_new (path));

		g_free (path);

		break;

#ifdef ENABLE_PYTHON
	case EOG_PLUGIN_LOADER_PY:
	{
		gchar *dir;

		if (!eog_python_init ()) {
			/* Mark plugin as unavailable and fails */
			info->available = FALSE;

			g_warning ("Cannot load Python plugin '%s' since eog "
			           "was not able to initialize the Python interpreter.",
			           info->name);

			return FALSE;
		}

		dir = g_path_get_dirname (info->file);

		g_return_val_if_fail ((info->location != NULL) &&
		                      (info->location[0] != '\0'),
		                      FALSE);

		info->module = G_TYPE_MODULE (
				eog_python_module_new (dir, info->location));

		g_free (dir);

		break;
	}
#endif
	default:
		g_return_val_if_reached (FALSE);
	}

	if (!g_type_module_use (info->module)) {
		switch (info->loader) {
		case EOG_PLUGIN_LOADER_C:
			g_warning ("Cannot load plugin '%s' since file '%s' cannot be read.",
				   info->name,
				   eog_module_get_path (EOG_MODULE (info->module)));
			break;

		case EOG_PLUGIN_LOADER_PY:
			g_warning ("Cannot load Python plugin '%s' since file '%s' cannot be read.",
				   info->name,
				   info->location);
			break;

		default:
			g_return_val_if_reached (FALSE);
		}

		g_object_unref (G_OBJECT (info->module));

		info->module = NULL;

		/* Mark plugin as unavailable and fails */
		info->available = FALSE;

		return FALSE;
	}

	switch (info->loader) {
	case EOG_PLUGIN_LOADER_C:
		info->plugin =
			EOG_PLUGIN (eog_module_new_object (EOG_MODULE (info->module)));
		break;

#ifdef ENABLE_PYTHON
	case EOG_PLUGIN_LOADER_PY:
		info->plugin =
			EOG_PLUGIN (eog_python_module_new_object (EOG_PYTHON_MODULE (info->module)));
		break;
#endif

	default:
		g_return_val_if_reached (FALSE);
	}

	g_type_module_unuse (info->module);

	eog_debug_message (DEBUG_PLUGINS, "End");

	return TRUE;
}

static gboolean
eog_plugin_engine_activate_plugin_real (EogPluginInfo *info)
{
	gboolean res = TRUE;

	/* Plugin is not available, don't try to activate/load it */
	if (!info->available) {
		return FALSE;
	}

	if (info->plugin == NULL)
		res = load_plugin_module (info);

	if (res) {
		const GList *wins = eog_application_get_windows (EOG_APP);

		while (wins != NULL) {
			eog_plugin_activate (info->plugin,
					     EOG_WINDOW (wins->data));

			wins = g_list_next (wins);
		}
	} else {
		g_warning ("Error activating plugin '%s'", info->name);
	}

	return res;
}

gboolean
eog_plugin_engine_activate_plugin (EogPluginInfo *info)
{
	eog_debug (DEBUG_PLUGINS);

	g_return_val_if_fail (info != NULL, FALSE);

	if (!info->available)
		return FALSE;

	if (info->active)
		return TRUE;

	if (eog_plugin_engine_activate_plugin_real (info)) {
		gboolean res;
		GSList *list;

		/* Update plugin state */
		info->active = TRUE;

		list = active_plugins;

		while (list != NULL) {
			if (strcmp (info->location, (gchar *)list->data) == 0) {
				g_warning ("Plugin '%s' is already active.", info->name);

				return TRUE;
			}

			list = g_slist_next (list);
		}

		active_plugins = g_slist_insert_sorted (active_plugins,
						        g_strdup (info->location),
						        (GCompareFunc)strcmp);

		res = mateconf_client_set_list (eog_plugin_engine_mateconf_client,
		    			     EOG_CONF_PLUGINS_ACTIVE_PLUGINS,
					     MATECONF_VALUE_STRING,
					     active_plugins,
					     NULL);

		if (!res)
			g_warning ("Error saving the list of active plugins.");

		return TRUE;
	}

	return FALSE;
}

static void
eog_plugin_engine_deactivate_plugin_real (EogPluginInfo *info)
{
	const GList *wins = eog_application_get_windows (EOG_APP);

	while (wins != NULL) {
		eog_plugin_deactivate (info->plugin,
				       EOG_WINDOW (wins->data));

		wins = g_list_next (wins);
	}
}

gboolean
eog_plugin_engine_deactivate_plugin (EogPluginInfo *info)
{
	gboolean res;
	GSList *list;

	eog_debug (DEBUG_PLUGINS);

	g_return_val_if_fail (info != NULL, FALSE);

	if (!info->active || !info->available)
		return TRUE;

	eog_plugin_engine_deactivate_plugin_real (info);

	/* Update plugin state */
	info->active = FALSE;

	list = active_plugins;
	res = (list == NULL);

	while (list != NULL) {
		if (strcmp (info->location, (gchar *)list->data) == 0) {
			g_free (list->data);
			active_plugins = g_slist_delete_link (active_plugins, list);
			list = NULL;
			res = TRUE;
		} else {
			list = g_slist_next (list);
		}
	}

	if (!res) {
		g_warning ("Plugin '%s' is already deactivated.", info->name);

		return TRUE;
	}

	res = mateconf_client_set_list (eog_plugin_engine_mateconf_client,
	    			     EOG_CONF_PLUGINS_ACTIVE_PLUGINS,
				     MATECONF_VALUE_STRING,
				     active_plugins,
				     NULL);

	if (!res)
		g_warning ("Error saving the list of active plugins.");

	return TRUE;
}

gboolean
eog_plugin_engine_plugin_is_active (EogPluginInfo *info)
{
	g_return_val_if_fail (info != NULL, FALSE);

	return (info->available && info->active);
}

gboolean
eog_plugin_engine_plugin_is_available (EogPluginInfo *info)
{
	g_return_val_if_fail (info != NULL, FALSE);

	return (info->available != FALSE);
}

static void
reactivate_all (EogWindow *window)
{
	GList *pl;

	eog_debug (DEBUG_PLUGINS);

	for (pl = eog_plugins_list; pl; pl = pl->next) {
		gboolean res = TRUE;

		EogPluginInfo *info = (EogPluginInfo*)pl->data;

		/* If plugin is not available, don't try to activate/load it */
		if (info->available && info->active) {
			if (info->plugin == NULL)
				res = load_plugin_module (info);

			if (res)
				eog_plugin_activate (info->plugin,
						     window);
		}
	}

	eog_debug_message (DEBUG_PLUGINS, "End");
}

void
eog_plugin_engine_update_plugins_ui (EogWindow *window,
				     gboolean   new_window)
{
	GList *pl;

	eog_debug (DEBUG_PLUGINS);

	g_return_if_fail (EOG_IS_WINDOW (window));

	if (new_window)
		reactivate_all (window);

	/* Updated ui of all the plugins that implement update_ui */
	for (pl = eog_plugins_list; pl; pl = pl->next) {
		EogPluginInfo *info = (EogPluginInfo*)pl->data;

		if (!info->available || !info->active)
			continue;

	       	eog_debug_message (DEBUG_PLUGINS, "Updating UI of %s", info->name);

		eog_plugin_update_ui (info->plugin, window);
	}
}

gboolean
eog_plugin_engine_plugin_is_configurable (EogPluginInfo *info) {
	eog_debug (DEBUG_PLUGINS);

	g_return_val_if_fail (info != NULL, FALSE);

	if ((info->plugin == NULL) || !info->active || !info->available)
		return FALSE;

	return eog_plugin_is_configurable (info->plugin);
}

void
eog_plugin_engine_configure_plugin (EogPluginInfo *info,
				    GtkWindow     *parent)
{
	GtkWidget *conf_dlg;

	GtkWindowGroup *wg;

	eog_debug (DEBUG_PLUGINS);

	g_return_if_fail (info != NULL);

	conf_dlg = eog_plugin_create_configure_dialog (info->plugin);

	g_return_if_fail (conf_dlg != NULL);

	gtk_window_set_transient_for (GTK_WINDOW (conf_dlg),
				      parent);

	// Will return a default group if no group is set
	wg = gtk_window_get_group (parent);

	// For now assign a dedicated window group if it is 
	// the default one until we know if this is really needed
	if (wg == gtk_window_get_group (NULL)) {
		wg = gtk_window_group_new ();
		gtk_window_group_add_window (wg, parent);
	}

	gtk_window_group_add_window (wg,
				     GTK_WINDOW (conf_dlg));

	gtk_window_set_modal (GTK_WINDOW (conf_dlg), TRUE);

	gtk_widget_show (conf_dlg);
}

static void
eog_plugin_engine_active_plugins_changed (MateConfClient *client,
					  guint cnxn_id,
					  MateConfEntry *entry,
					  gpointer user_data)
{
	GList *pl;
	gboolean to_activate;

	eog_debug (DEBUG_PLUGINS);

	g_return_if_fail (entry->key != NULL);
	g_return_if_fail (entry->value != NULL);

	if (!((entry->value->type == MATECONF_VALUE_LIST) &&
	      (mateconf_value_get_list_type (entry->value) == MATECONF_VALUE_STRING))) {
		g_warning ("The mateconf key '%s' may be corrupted.", EOG_CONF_PLUGINS_ACTIVE_PLUGINS);
		return;
	}

	active_plugins = mateconf_client_get_list (eog_plugin_engine_mateconf_client,
						EOG_CONF_PLUGINS_ACTIVE_PLUGINS,
						MATECONF_VALUE_STRING,
						NULL);

	for (pl = eog_plugins_list; pl; pl = pl->next) {
		EogPluginInfo *info = (EogPluginInfo*)pl->data;

		if (!info->available)
			continue;

		to_activate = (g_slist_find_custom (active_plugins,
						    info->location,
						    (GCompareFunc)strcmp) != NULL);

		if (!info->active && to_activate) {
			/* Activate plugin */
			if (eog_plugin_engine_activate_plugin_real (info))
				/* Update plugin state */
				info->active = TRUE;
		} else {
			if (info->active && !to_activate) {
				eog_plugin_engine_deactivate_plugin_real (info);

				/* Update plugin state */
				info->active = FALSE;
			}
		}
	}
}

const gchar *
eog_plugin_engine_get_plugin_name (EogPluginInfo *info)
{
	g_return_val_if_fail (info != NULL, NULL);

	return info->name;
}

const gchar *
eog_plugin_engine_get_plugin_description (EogPluginInfo *info)
{
	g_return_val_if_fail (info != NULL, NULL);

	return info->desc;
}

const gchar *
eog_plugin_engine_get_plugin_icon_name (EogPluginInfo *info)
{
	g_return_val_if_fail (info != NULL, NULL);

	/* Use the eog-plugin icon as a default if the plugin does not
	   have its own */
	if (info->icon_name != NULL &&
	    gtk_icon_theme_has_icon (gtk_icon_theme_get_default (),
	    			     info->icon_name))
		return info->icon_name;
	else
		return "eog-plugin";
}

const gchar **
eog_plugin_engine_get_plugin_authors (EogPluginInfo *info)
{
	g_return_val_if_fail (info != NULL, (const gchar **)NULL);

	return (const gchar **) info->authors;
}

const gchar *
eog_plugin_engine_get_plugin_website (EogPluginInfo *info)
{
	g_return_val_if_fail (info != NULL, NULL);

	return info->website;
}

const gchar *
eog_plugin_engine_get_plugin_copyright (EogPluginInfo *info)
{
	g_return_val_if_fail (info != NULL, NULL);

	return info->copyright;
}
