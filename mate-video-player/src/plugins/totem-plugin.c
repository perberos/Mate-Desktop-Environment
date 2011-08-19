/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * heavily based on code from Rhythmbox and Gedit
 *
 * Copyright (C) 2002-2005 Paolo Maggi
 * Copyright (C) 2007 Bastien Nocera <hadess@hadess.net>
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

/**
 * SECTION:totem-plugin
 * @short_description: base plugin class and loading/unloading functions
 * @stability: Unstable
 * @include: totem-plugin.h
 *
 * #TotemPlugin is a general-purpose architecture for adding plugins to Totem, with
 * derived support for different programming languages.
 **/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <mateconf/mateconf-client.h>

#include "totem-plugin.h"
#include "totem-uri.h"
#include "totem-interface.h"

G_DEFINE_TYPE (TotemPlugin, totem_plugin, G_TYPE_OBJECT)

#define TOTEM_PLUGIN_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TOTEM_TYPE_PLUGIN, TotemPluginPrivate))

static void totem_plugin_finalise (GObject *o);
static void totem_plugin_set_property (GObject *object,
				       guint prop_id,
				       const GValue *value,
				       GParamSpec *pspec);
static void totem_plugin_get_property (GObject *object,
				       guint prop_id,
				       GValue *value,
				       GParamSpec *pspec);

struct TotemPluginPrivate {
	char *name;
};

enum
{
	PROP_0,
	PROP_NAME
};

GQuark
totem_plugin_error_quark (void)
{
	static GQuark quark;

	if (!quark)
		quark = g_quark_from_static_string ("totem_plugin_error");

	return quark;
}

static gboolean
is_configurable (TotemPlugin *plugin)
{
	return (TOTEM_PLUGIN_GET_CLASS (plugin)->create_configure_dialog != NULL);
}

static void
totem_plugin_class_init (TotemPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = totem_plugin_finalise;
	object_class->get_property = totem_plugin_get_property;
	object_class->set_property = totem_plugin_set_property;

	klass->activate = NULL;
	klass->deactivate = NULL;
	klass->create_configure_dialog = NULL;
	klass->is_configurable = is_configurable;

	/* FIXME: this should be a construction property, but due to the python plugin hack can't be */
	/**
	 * TotemPlugin:name:
	 *
	 * The plugin's name. It should be a construction property, but due to the Python plugin hack, it
	 * can't be: do not change the name after construction. Should be the same as used for naming plugin-
	 * specific resources.
	 **/
	g_object_class_install_property (object_class,
					 PROP_NAME,
					 g_param_spec_string ("name",
							      "name",
							      "name",
							      NULL,
							      G_PARAM_READWRITE /*| G_PARAM_CONSTRUCT_ONLY*/));

	g_type_class_add_private (klass, sizeof (TotemPluginPrivate));
}

static void
totem_plugin_init (TotemPlugin *plugin)
{
	/* Empty */
}

static void
totem_plugin_finalise (GObject *object)
{
	TotemPluginPrivate *priv = TOTEM_PLUGIN_GET_PRIVATE (object);

	g_free (priv->name);

	G_OBJECT_CLASS (totem_plugin_parent_class)->finalize (object);
}

static void
totem_plugin_set_property (GObject *object,
			   guint prop_id,
			   const GValue *value,
			   GParamSpec *pspec)
{
	TotemPluginPrivate *priv = TOTEM_PLUGIN_GET_PRIVATE (object);

	switch (prop_id)
	{
	case PROP_NAME:
		g_free (priv->name);
		priv->name = g_value_dup_string (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
totem_plugin_get_property (GObject *object,
			   guint prop_id,
			   GValue *value,
			   GParamSpec *pspec)
{
	TotemPluginPrivate *priv = TOTEM_PLUGIN_GET_PRIVATE (object);

	switch (prop_id)
	{
	case PROP_NAME:
		g_value_set_string (value, priv->name);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

/**
 * totem_plugin_activate:
 * @plugin: a #TotemPlugin
 * @totem: a #TotemObject
 * @error: return location for a #GError, or %NULL
 *
 * Activates the passed @plugin by calling its activate method.
 *
 * Return value: %TRUE on success
 **/
gboolean
totem_plugin_activate (TotemPlugin *plugin,
		       TotemObject *totem,
		       GError **error)
{
	g_return_val_if_fail (TOTEM_IS_PLUGIN (plugin), FALSE);
	g_return_val_if_fail (TOTEM_IS_OBJECT (totem), FALSE);

	if (TOTEM_PLUGIN_GET_CLASS (plugin)->activate)
		return TOTEM_PLUGIN_GET_CLASS (plugin)->activate (plugin, totem, error);

	return TRUE;
}

/**
 * totem_plugin_deactivate:
 * @plugin: a #TotemPlugin
 * @totem: a #TotemObject
 *
 * Deactivates @plugin by calling its deactivate method.
 **/
void
totem_plugin_deactivate	(TotemPlugin *plugin,
			 TotemObject *totem)
{
	g_return_if_fail (TOTEM_IS_PLUGIN (plugin));
	g_return_if_fail (TOTEM_IS_OBJECT (totem));

	if (TOTEM_PLUGIN_GET_CLASS (plugin)->deactivate)
		TOTEM_PLUGIN_GET_CLASS (plugin)->deactivate (plugin, totem);
}

/**
 * totem_plugin_is_configurable:
 * @plugin: a #TotemPlugin
 *
 * Returns %TRUE if the plugin is configurable and has a
 * configuration dialog. It calls the plugin's
 * is_configurable method.
 *
 * Return value: %TRUE if the plugin is configurable
 **/
gboolean
totem_plugin_is_configurable (TotemPlugin *plugin)
{
	g_return_val_if_fail (TOTEM_IS_PLUGIN (plugin), FALSE);

	return TOTEM_PLUGIN_GET_CLASS (plugin)->is_configurable (plugin);
}

/**
 * totem_plugin_create_configure_dialog:
 * @plugin: a #TotemPlugin
 *
 * Returns the plugin's configuration dialog, as created by
 * the plugin's create_configure_dialog method.
 *
 * Return value: the configuration dialog, or %NULL
 **/
GtkWidget *
totem_plugin_create_configure_dialog (TotemPlugin *plugin)
{
	g_return_val_if_fail (TOTEM_IS_PLUGIN (plugin), NULL);

	if (TOTEM_PLUGIN_GET_CLASS (plugin)->create_configure_dialog)
		return TOTEM_PLUGIN_GET_CLASS (plugin)->create_configure_dialog (plugin);
	else
		return NULL;
}

#define UNINSTALLED_PLUGINS_LOCATION "plugins"

GList *
totem_get_plugin_paths (void)
{
	GList *paths;
	char  *path;
	MateConfClient *client;

	paths = NULL;

	client = mateconf_client_get_default ();
	if (mateconf_client_get_bool (client, MATECONF_PREFIX"/disable_user_plugins", NULL) == FALSE) {
		path = g_build_filename (totem_data_dot_dir (), "plugins", NULL);
		paths = g_list_prepend (paths, path);
	}

#ifdef TOTEM_RUN_IN_SOURCE_TREE
	path = g_build_filename (UNINSTALLED_PLUGINS_LOCATION, NULL);
	paths = g_list_prepend (paths, path);
#endif

	path = g_strdup (TOTEM_PLUGIN_DIR);
	paths = g_list_prepend (paths, path);

	paths = g_list_reverse (paths);

	return paths;
}

/**
 * totem_plugin_find_file:
 * @plugin: a #TotemPlugin
 * @file: the file to find
 *
 * Finds the specified @file by looking in the plugin paths
 * listed by totem_get_plugin_paths() and then in the system
 * Totem data directory.
 *
 * This should be used by plugins to find plugin-specific
 * resource files.
 *
 * Return value: a newly-allocated absolute path for the file, or %NULL
 **/
char *
totem_plugin_find_file (TotemPlugin *plugin,
			const char *file)
{
	TotemPluginPrivate *priv = TOTEM_PLUGIN_GET_PRIVATE (plugin);
	GList *paths;
	GList *l;
	char *ret = NULL;

	paths = totem_get_plugin_paths ();

	for (l = paths; l != NULL; l = l->next) {
		if (ret == NULL && priv->name) {
			char *tmp;

			tmp = g_build_filename (l->data, priv->name, file, NULL);

			if (g_file_test (tmp, G_FILE_TEST_EXISTS)) {
				ret = tmp;
				break;
			}
			g_free (tmp);
		}
	}

	g_list_foreach (paths, (GFunc)g_free, NULL);
	g_list_free (paths);

	/* global data files */
	if (ret == NULL)
		ret = totem_interface_get_full_path (file);

	/* ensure it's an absolute path, so doesn't confuse rb_glade_new et al */
	if (ret && ret[0] != '/') {
		char *pwd = g_get_current_dir ();
		char *path = g_strconcat (pwd, G_DIR_SEPARATOR_S, ret, NULL);
		g_free (ret);
		g_free (pwd);
		ret = path;
	}
	return ret;
}

/**
 * totem_plugin_load_interface:
 * @plugin: a #TotemPlugin
 * @name: interface filename
 * @fatal: %TRUE if it's a fatal error if the interface can't be loaded
 * @parent: the interface's parent #GtkWindow
 * @user_data: a pointer to be passed to each signal handler in the interface when they're called
 *
 * Loads an interface file (GtkBuilder UI file) for a plugin, given its filename and
 * assuming it's installed in the plugin's data directory.
 *
 * This should be used instead of attempting to load interfaces manually in plugins.
 *
 * Return value: the #GtkBuilder instance for the interface
 **/
GtkBuilder *
totem_plugin_load_interface (TotemPlugin *plugin, const char *name,
			     gboolean fatal, GtkWindow *parent,
			     gpointer user_data)
{
	GtkBuilder *builder = NULL;
	char *filename;

	filename = totem_plugin_find_file (plugin, name);
	builder = totem_interface_load_with_full_path (filename, fatal, parent,
						       user_data);
	g_free (filename);

	return builder;
}
