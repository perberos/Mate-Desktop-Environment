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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <glib/gi18n.h>
#include <glib.h>
#include <mateconf/mateconf-client.h>

#include "totem-plugin.h"

#include "totem-module.h"
#include "totem-interface.h"

#ifdef ENABLE_PYTHON
#include "totem-python-module.h"
#endif

#include "totem-plugins-engine.h"

#define PLUGIN_EXT	".totem-plugin"
#define MATECONF_PREFIX_PLUGINS MATECONF_PREFIX"/plugins"
#define MATECONF_PREFIX_PLUGIN MATECONF_PREFIX"/plugins/%s"
#define MATECONF_PLUGIN_ACTIVE MATECONF_PREFIX_PLUGINS"/%s/active"
#define MATECONF_PLUGIN_HIDDEN MATECONF_PREFIX_PLUGINS"/%s/hidden"

typedef enum
{
	TOTEM_PLUGIN_LOADER_C,
	TOTEM_PLUGIN_LOADER_PY
} TotemPluginLang;

struct _TotemPluginInfo
{
	gchar        *file;

	gchar        *location;
	TotemPluginLang lang;
	GTypeModule  *module;

	gchar        *name;
	gchar        *desc;
	gchar        **authors;
	gchar        *copyright;
	gchar        *website;

	gchar        *icon_name;
	GdkPixbuf    *icon_pixbuf;

	TotemPlugin     *plugin;

	gboolean     builtin;
	gboolean     active;
	gboolean     visible;
	guint        active_notification_id;
	guint        visible_notification_id;
};

static void totem_plugin_info_free (TotemPluginInfo *info);
static void totem_plugins_engine_plugin_active_cb (MateConfClient *client,
						guint cnxn_id,
						MateConfEntry *entry,
						TotemPluginInfo *info);
static void totem_plugins_engine_plugin_visible_cb (MateConfClient *client,
						 guint cnxn_id,
						 MateConfEntry *entry,
						 TotemPluginInfo *info);
static gboolean totem_plugins_engine_activate_plugin_real (TotemPluginInfo *info,
							TotemObject *totem,
							GError **error);
static void totem_plugins_engine_deactivate_plugin_real (TotemPluginInfo *info,
						      TotemObject *totem);

static GHashTable *totem_plugins = NULL;
guint garbage_collect_id = 0;
TotemObject *totem_plugins_object = NULL;
static MateConfClient *client = NULL;

static TotemPluginInfo *
totem_plugins_engine_load (const gchar *file)
{
	TotemPluginInfo *info;
	GKeyFile *plugin_file = NULL;
	GError *err = NULL;
	gchar *str;

	g_return_val_if_fail (file != NULL, NULL);

	info = g_new0 (TotemPluginInfo, 1);
	info->file = g_strdup (file);

	plugin_file = g_key_file_new ();
	if (!g_key_file_load_from_file (plugin_file, file, G_KEY_FILE_NONE, NULL)) {
		g_warning ("Bad plugin file: %s", file);
		goto error;
	}

	if (!g_key_file_has_key (plugin_file,
			   	 "Totem Plugin",
				 "IAge",
				 NULL))	{
		goto error;
	}

	/* Check IAge=1 */
	if (g_key_file_get_integer (plugin_file,
				    "Totem Plugin",
				    "IAge",
				    NULL) != 1)	{
		goto error;
	}

	/* Get Location */
	str = g_key_file_get_string (plugin_file,
				     "Totem Plugin",
				     "Module",
				     NULL);
	if (str) {
		info->location = str;
	} else {
		g_warning ("Could not find 'Module' in %s", file);
		goto error;
	}

	/* Get the loader for this plugin */
	str = g_key_file_get_string (plugin_file,
				     "Totem Plugin",
				     "Loader",
				     NULL);
	if (str && strcmp(str, "python") == 0) {
		info->lang = TOTEM_PLUGIN_LOADER_PY;
#ifndef ENABLE_PYTHON
		g_warning ("Cannot load Python extension '%s', Totem was not compiled with Python support", file);
		g_free (str);
		goto error;
#endif
	} else {
		info->lang = TOTEM_PLUGIN_LOADER_C;
	}
	g_free (str);

	/* Get Name */
	str = g_key_file_get_locale_string (plugin_file,
					    "Totem Plugin",
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
					    "Totem Plugin",
					    "Description",
					    NULL, NULL);
	if (str) {
		info->desc = str;
	} else {
		info->desc = g_strdup ("");
	}

	/* Get icon name */
	str = g_key_file_get_string (plugin_file,
				     "Totem Plugin",
				     "Icon",
				     NULL);
	if (str) {
		info->icon_name = str;
	} else {
		info->icon_name = g_strdup ("");
	}

	/* Get Authors */
	info->authors = g_key_file_get_string_list (plugin_file,
						    "Totem Plugin",
						    "Authors",
						    NULL, NULL);

	/* Get Copyright */
	str = g_key_file_get_string (plugin_file,
				     "Totem Plugin",
				     "Copyright",
				     NULL);
	if (str) {
		info->copyright = str;
	} else {
		info->copyright = g_strdup ("");
	}

	/* Get Copyright */
	str = g_key_file_get_string (plugin_file,
				     "Totem Plugin",
				     "Website",
				     NULL);
	if (str) {
		info->website = str;
	} else {
		info->website = g_strdup ("");
	
	}

	/* Get Builtin */
	info->builtin = g_key_file_get_boolean (plugin_file,
						"Totem Plugin",
						"Builtin",
						&err);
	if (err != NULL) {
		info->builtin = FALSE;
		g_error_free (err);
	}

	g_key_file_free (plugin_file);

	return info;

error:
	g_warning ("Failed to load plugin file: %s", file);
	g_free (info->file);
	g_free (info->location);
	g_free (info->name);
	g_free (info);
	g_key_file_free (plugin_file);

	return NULL;
}

static void
totem_plugins_engine_load_file (const char *plugin_file)
{
	TotemPluginInfo *info;
	char *key_name;
	MateConfValue *activate_value;
	gboolean activate;

	if (g_str_has_suffix (plugin_file, PLUGIN_EXT) == FALSE)
		return;

	info = totem_plugins_engine_load (plugin_file);
	if (info == NULL)
		return;

	if (g_hash_table_lookup (totem_plugins, info->location)) {
		totem_plugin_info_free (info);
		return;
	}

	g_hash_table_insert (totem_plugins, info->location, info);

	key_name = g_strdup_printf (MATECONF_PREFIX_PLUGIN, info->location);
	mateconf_client_add_dir (client, key_name, MATECONF_CLIENT_PRELOAD_ONELEVEL, NULL);
	g_free (key_name);

	key_name = g_strdup_printf (MATECONF_PLUGIN_ACTIVE, info->location);
	info->active_notification_id = mateconf_client_notify_add (client,
								key_name,
								(MateConfClientNotifyFunc)totem_plugins_engine_plugin_active_cb,
								info,
								NULL,
								NULL);
	activate_value = mateconf_client_get (client, key_name, NULL);
	g_free (key_name);

	if (activate_value == NULL) {
		/* Builtin plugins are activated by default; other plugins aren't */
		activate = info->builtin;
	} else {
		activate = mateconf_value_get_bool (activate_value);
		mateconf_value_free (activate_value);
	}

	if (info->builtin == FALSE) {
		/* Builtin plugins are *always* invisible */
		key_name = g_strdup_printf (MATECONF_PLUGIN_HIDDEN, info->location);
		info->visible_notification_id = mateconf_client_notify_add (client,
									 key_name,
									 (MateConfClientNotifyFunc)totem_plugins_engine_plugin_visible_cb,
									 info,
									 NULL,
									 NULL);
		info->visible = !mateconf_client_get_bool (client, key_name, NULL);
		g_free (key_name);
	}

	if (activate)
		totem_plugins_engine_activate_plugin (info);
}

static void
totem_plugins_engine_load_dir (const gchar *path)
{
	GDir *dir;
	const char *name;

	dir = g_dir_open (path, 0, NULL);
	if (dir == NULL)
		return;

	while ((name = g_dir_read_name (dir)) != NULL) {
		char *filename;

		filename = g_build_filename (path, name, NULL);
		if (g_file_test (filename, G_FILE_TEST_IS_DIR) != FALSE) {
			totem_plugins_engine_load_dir (filename);
		} else {
			totem_plugins_engine_load_file (filename);
		}
		g_free (filename);
	}
	g_dir_close (dir);
}

static void
totem_plugins_engine_load_all (void)
{
	GList *paths;

	paths = totem_get_plugin_paths ();
	while (paths != NULL) {
		totem_plugins_engine_load_dir (paths->data);
		g_free (paths->data);
		paths = g_list_delete_link (paths, paths);
	}
}

#if 0
#ifdef ENABLE_PYTHON
static gboolean
garbage_collect_cb (gpointer data)
{
	/* Commented out due to line 387 being commented out. More's commented out in totem-python-module.c. */
	totem_plugins_engine_garbage_collect ();
	return TRUE;
}
#endif
#endif

gboolean
totem_plugins_engine_init (TotemObject *totem)
{
	g_return_val_if_fail (totem_plugins == NULL, FALSE);

	if (!g_module_supported ())
	{
		g_warning ("Totem is not able to initialize the plugins engine.");
		return FALSE;
	}
	totem_plugins = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, (GDestroyNotify)totem_plugin_info_free);

	totem_plugins_object = totem;
	g_object_ref (G_OBJECT (totem_plugins_object));

	client = mateconf_client_get_default ();

	totem_plugins_engine_load_all ();

#if 0
#ifdef ENABLE_PYTHON
	/* Commented out because it's a no-op. A further section is commented out below, and more's commented out
	 * in totem-python-module.c. */
	garbage_collect_id = g_timeout_add_seconds_full (G_PRIORITY_LOW, 20, garbage_collect_cb, NULL, NULL);
#endif
#endif

	return TRUE;
}

void
totem_plugins_engine_garbage_collect (void)
{
#ifdef ENABLE_PYTHON
	totem_python_garbage_collect ();
#endif
}

static void
totem_plugin_info_free (TotemPluginInfo *info)
{
	if (info->active)
		totem_plugins_engine_deactivate_plugin_real (info, totem_plugins_object);

	if (info->plugin != NULL) {
		g_object_unref (info->plugin);

		/* info->module must not be unref since it is not possible to finalize
		 * a type module */
	}

	if (info->active_notification_id > 0)
		mateconf_client_notify_remove (client, info->active_notification_id);
	if (info->visible_notification_id > 0)
		mateconf_client_notify_remove (client, info->visible_notification_id);

	g_free (info->file);
	g_free (info->location);
	g_free (info->name);
	g_free (info->desc);
	g_free (info->website);
	g_free (info->copyright);
	g_free (info->icon_name);

	if (info->icon_pixbuf)
		g_object_unref (info->icon_pixbuf);
	g_strfreev (info->authors);

	g_free (info);
}

void
totem_plugins_engine_shutdown (void)
{
	if (totem_plugins != NULL)
		g_hash_table_destroy (totem_plugins);
	totem_plugins = NULL;

	if (totem_plugins_object != NULL)
		g_object_unref (totem_plugins_object);
	totem_plugins_object = NULL;

#if 0
#ifdef ENABLE_PYTHON
	if (garbage_collect_id > 0)
		g_source_remove (garbage_collect_id);
	totem_plugins_engine_garbage_collect ();
#endif
#endif

	if (client != NULL)
		g_object_unref (client);
	client = NULL;

#ifdef ENABLE_PYTHON
	totem_python_shutdown ();
#endif
}

static void
collate_values_cb (gpointer key, gpointer value, GList **list)
{
	*list = g_list_prepend (*list, value);
}

GList *
totem_plugins_engine_get_plugins_list (void)
{
	GList *list = NULL;

	if (totem_plugins == NULL)
		return NULL;

	g_hash_table_foreach (totem_plugins, (GHFunc)collate_values_cb, &list);
	list = g_list_reverse (list);

	return list;
}

static gboolean
load_plugin_module (TotemPluginInfo *info)
{
	gchar *path;
	gchar *dirname;

	g_return_val_if_fail (info != NULL, FALSE);
	g_return_val_if_fail (info->file != NULL, FALSE);
	g_return_val_if_fail (info->location != NULL, FALSE);
	g_return_val_if_fail (info->plugin == NULL, FALSE);

	switch (info->lang) {
		case TOTEM_PLUGIN_LOADER_C:
			dirname = g_path_get_dirname (info->file);
			g_return_val_if_fail (dirname != NULL, FALSE);

			path = g_module_build_path (dirname, info->location);
#ifdef TOTEM_RUN_IN_SOURCE_TREE
			if (!g_file_test (path, G_FILE_TEST_EXISTS)) {
				char *temp;

				g_free (path);
				temp = g_build_filename (dirname, ".libs", NULL);

				path = g_module_build_path (temp, info->location);
				g_free (temp);
			}
#endif

			g_free (dirname);
			g_return_val_if_fail (path != NULL, FALSE);

			info->module = G_TYPE_MODULE (totem_module_new (path, info->location));
			g_free (path);
			break;
		case TOTEM_PLUGIN_LOADER_PY:
#ifdef ENABLE_PYTHON
			info->module = G_TYPE_MODULE (totem_python_module_new (info->file, info->location));
#else
			g_warning ("Cannot load plugin %s, Python plugin support is disabled", info->location);
#endif
			break;
		default:
			g_assert_not_reached ();
	}

	if (g_type_module_use (info->module) == FALSE) {
		g_warning ("Could not load plugin %s\n", info->location);

		g_object_unref (G_OBJECT (info->module));
		info->module = NULL;

		return FALSE;
	}

	switch (info->lang) {
		case TOTEM_PLUGIN_LOADER_C:
			info->plugin = TOTEM_PLUGIN (totem_module_new_object (TOTEM_MODULE (info->module)));
			break;
		case TOTEM_PLUGIN_LOADER_PY:
#ifdef ENABLE_PYTHON
			info->plugin = TOTEM_PLUGIN (totem_python_module_new_object (TOTEM_PYTHON_MODULE (info->module)));
#endif
			break;
		default:
			g_assert_not_reached ();
	}

	return TRUE;
}

static gboolean
totem_plugins_engine_activate_plugin_real (TotemPluginInfo *info, TotemObject *totem, GError **error)
{
	gboolean res = TRUE;

	if (info->plugin == NULL)
		res = load_plugin_module (info);

	if (res)
		res = totem_plugin_activate (info->plugin, totem, error);
	else
		g_warning ("Error, impossible to activate plugin '%s'", info->name);

	return res;
}

gboolean
totem_plugins_engine_activate_plugin (TotemPluginInfo *info)
{
	char *msg;
	GError *error = NULL;
	gboolean ret;

	g_return_val_if_fail (info != NULL, FALSE);

	if (info->active)
		return TRUE;

	ret = totem_plugins_engine_activate_plugin_real (info, totem_plugins_object, &error);
	if (info->visible != FALSE || ret != FALSE) {
		char *key_name;

		key_name = g_strdup_printf (MATECONF_PLUGIN_ACTIVE, info->location);
		mateconf_client_set_bool (client, key_name, ret, NULL);
		g_free (key_name);
	}

	info->active = ret;

	if (ret != FALSE)
		return TRUE;

	if (error != NULL) {
		msg = g_strdup_printf (_("Unable to activate plugin %s.\n%s"), info->name, error->message);
		g_error_free (error);
	} else {
		msg = g_strdup_printf (_("Unable to activate plugin %s"), info->name);
	}
	totem_interface_error (_("Plugin Error"), msg, NULL);
	g_free (msg);

	return FALSE;
}

static void
totem_plugins_engine_deactivate_plugin_real (TotemPluginInfo *info, TotemObject *totem)
{
	totem_plugin_deactivate (info->plugin, totem_plugins_object);
}

gboolean
totem_plugins_engine_deactivate_plugin (TotemPluginInfo *info)
{
	char *key_name;

	g_return_val_if_fail (info != NULL, FALSE);

	if (!info->active)
		return TRUE;

	totem_plugins_engine_deactivate_plugin_real (info, totem_plugins_object);

	/* Update plugin state */
	info->active = FALSE;

	key_name = g_strdup_printf (MATECONF_PLUGIN_ACTIVE, info->location);
	mateconf_client_set_bool (client, key_name, FALSE, NULL);
	g_free (key_name);

	return TRUE;
}

gboolean
totem_plugins_engine_plugin_is_active (TotemPluginInfo *info)
{
	g_return_val_if_fail (info != NULL, FALSE);

	return info->active;
}

gboolean
totem_plugins_engine_plugin_is_visible (TotemPluginInfo *info)
{
	g_return_val_if_fail (info != NULL, FALSE);

	return info->visible;
}

gboolean
totem_plugins_engine_plugin_is_configurable (TotemPluginInfo *info)
{
	g_return_val_if_fail (info != NULL, FALSE);

	if ((info->plugin == NULL) || !info->active)
		return FALSE;

	return totem_plugin_is_configurable (info->plugin);
}

void
totem_plugins_engine_configure_plugin (TotemPluginInfo *info,
				       GtkWindow       *parent)
{
	GtkWidget *conf_dlg;

	GtkWindowGroup *wg;

	g_return_if_fail (info != NULL);

	conf_dlg = totem_plugin_create_configure_dialog (info->plugin);
	g_return_if_fail (conf_dlg != NULL);
	gtk_window_set_transient_for (GTK_WINDOW (conf_dlg),
				      parent);

	wg = gtk_window_get_group (parent);
	if (wg == NULL)
	{
		wg = gtk_window_group_new ();
		gtk_window_group_add_window (wg, parent);
	}

	gtk_window_group_add_window (wg,
				     GTK_WINDOW (conf_dlg));

	gtk_window_set_modal (GTK_WINDOW (conf_dlg), TRUE);
	gtk_widget_show (conf_dlg);
}

static void
totem_plugins_engine_plugin_active_cb (MateConfClient *mateconf_client,
				       guint cnxn_id,
				       MateConfEntry *entry,
				       TotemPluginInfo *info)
{
	if (mateconf_value_get_bool (entry->value)) {
		totem_plugins_engine_activate_plugin (info);
	} else {
		totem_plugins_engine_deactivate_plugin (info);
	}
}

static void
totem_plugins_engine_plugin_visible_cb (MateConfClient *mateconf_client,
					guint cnxn_id,
					MateConfEntry *entry,
					TotemPluginInfo *info)
{
	info->visible = !mateconf_value_get_bool (entry->value);
}

const gchar *
totem_plugins_engine_get_plugin_name (TotemPluginInfo *info)
{
	g_return_val_if_fail (info != NULL, NULL);

	return info->name;
}

const gchar *
totem_plugins_engine_get_plugin_description (TotemPluginInfo *info)
{
	g_return_val_if_fail (info != NULL, NULL);

	return info->desc;
}

const gchar **
totem_plugins_engine_get_plugin_authors (TotemPluginInfo *info)
{
	g_return_val_if_fail (info != NULL, (const gchar **)NULL);

	return (const gchar **)info->authors;
}

const gchar *
totem_plugins_engine_get_plugin_website (TotemPluginInfo *info)
{
	g_return_val_if_fail (info != NULL, NULL);

	return info->website;
}

const gchar *
totem_plugins_engine_get_plugin_copyright (TotemPluginInfo *info)
{
	g_return_val_if_fail (info != NULL, NULL);

	return info->copyright;
}

GdkPixbuf *
totem_plugins_engine_get_plugin_icon (TotemPluginInfo *info)
{
	if (info->icon_name == NULL)
		return NULL;

	if (info->icon_pixbuf == NULL) {
		char *filename = NULL;
		char *dirname;

		dirname = g_path_get_dirname (info->file);
		filename = g_build_filename (dirname, info->icon_name, NULL);
		g_free (dirname);

		info->icon_pixbuf = gdk_pixbuf_new_from_file (filename, NULL);
		g_free (filename);
	}

	return info->icon_pixbuf;
}
