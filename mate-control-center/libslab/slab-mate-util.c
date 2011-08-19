/*
 * This file is part of libslab.
 *
 * Copyright (c) 2006 Novell, Inc.
 *
 * Libslab is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * Libslab is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libslab; if not, write to the Free Software Foundation, Inc., 51
 * Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "slab-mate-util.h"
#include "libslab-utils.h"

#include <mateconf/mateconf-client.h>
#include <gio/gio.h>
#include <string.h>

gboolean
get_slab_mateconf_bool (const gchar * key)
{
	MateConfClient *mateconf_client;
	GError *error;

	gboolean value;

	mateconf_client = mateconf_client_get_default ();
	error = NULL;

	value = mateconf_client_get_bool (mateconf_client, key, &error);

	g_object_unref (mateconf_client);

	if (error)
	{
		g_warning ("error accessing %s [%s]\n", key, error->message);
		g_error_free (error);
	}

	return value;
}

gint
get_slab_mateconf_int (const gchar * key)
{
	MateConfClient *mateconf_client;
	GError *error;

	gint value;

	mateconf_client = mateconf_client_get_default ();
	error = NULL;

	value = mateconf_client_get_int (mateconf_client, key, &error);

	g_object_unref (mateconf_client);
	if (error)
	{
		g_warning ("error accessing %s [%s]\n", key, error->message);
		g_error_free (error);
	}

	return value;
}

gchar *
get_slab_mateconf_string (const gchar * key)
{
	MateConfClient *mateconf_client;
	GError *error;

	gchar *value;

	mateconf_client = mateconf_client_get_default ();
	error = NULL;

	value = mateconf_client_get_string (mateconf_client, key, &error);

	g_object_unref (mateconf_client);
	if (error)
	{
		g_warning ("error accessing %s [%s]\n", key, error->message);
		g_error_free (error);
	}

	return value;
}

void
free_list_of_strings (GList * string_list)
{
	g_assert (string_list != NULL);
	g_list_foreach (string_list, (GFunc) g_free, NULL);
	g_list_free (string_list);
}

void
free_slab_mateconf_slist_of_strings (GSList * string_list)
{
	g_assert (string_list != NULL);
	g_slist_foreach (string_list, (GFunc) g_free, NULL);
	g_slist_free (string_list);
}

GSList *
get_slab_mateconf_slist (const gchar * key)
{
	MateConfClient *mateconf_client;
	GError *error;

	GSList *value;

	mateconf_client = mateconf_client_get_default ();
	error = NULL;

	value = mateconf_client_get_list (mateconf_client, key, MATECONF_VALUE_STRING, &error);

	g_object_unref (mateconf_client);
	if (error)
	{
		g_warning ("error accessing %s [%s]\n", key, error->message);

		g_error_free (error);
	}

	return value;
}

MateDesktopItem *
load_desktop_item_from_mateconf_key (const gchar * key)
{
	MateDesktopItem *item;
	gchar *id = get_slab_mateconf_string (key);

	if (!id)
		return NULL;

	item = load_desktop_item_from_unknown (id);
	g_free (id);
	return item;
}

MateDesktopItem *
load_desktop_item_from_unknown (const gchar *id)
{
	MateDesktopItem *item;
	gchar            *basename;

	GError *error = NULL;


	item = mate_desktop_item_new_from_uri (id, 0, &error);

	if (! error)
		return item;
	else {
		g_error_free (error);
		error = NULL;
	}

	item = mate_desktop_item_new_from_file (id, 0, &error);

	if (! error)
		return item;
	else {
		g_error_free (error);
		error = NULL;
	}

	item = mate_desktop_item_new_from_basename (id, 0, &error);

	if (! error)
		return item;
	else {
		g_error_free (error);
		error = NULL;
	}

	basename = g_strrstr (id, "/");

	if (basename) {
		basename++;

		item = mate_desktop_item_new_from_basename (basename, 0, &error);

		if (! error)
			return item;
		else {
			g_error_free (error);
			error = NULL;
		}
	}

	return NULL;
}

gchar *
get_package_name_from_desktop_item (MateDesktopItem * desktop_item)
{
	gchar *argv[6];
	gchar *package_name;
	gint retval;
	GError *error;

	argv[0] = "rpm";
	argv[1] = "-qf";
	argv[2] = "--qf";
	argv[3] = "%{NAME}";
	argv[4] = g_filename_from_uri (mate_desktop_item_get_location (desktop_item), NULL, NULL);
	argv[5] = NULL;

	error = NULL;

	if (!g_spawn_sync (NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &package_name, NULL,
			&retval, &error))
	{
		g_warning ("error: [%s]\n", error->message);
		g_error_free (error);
		retval = -1;
	}

	g_free (argv[4]);

	if (!retval)
		return package_name;
	else
		return NULL;
}

gboolean
open_desktop_item_exec (MateDesktopItem * desktop_item)
{
	GError *error = NULL;

	if (!desktop_item)
		return FALSE;

	mate_desktop_item_launch (desktop_item, NULL, MATE_DESKTOP_ITEM_LAUNCH_ONLY_ONE, &error);

	if (error)
	{
		g_warning ("error launching %s [%s]\n",
			mate_desktop_item_get_location (desktop_item), error->message);

		g_error_free (error);
		return FALSE;
	}

	return TRUE;
}

gboolean
open_desktop_item_help (MateDesktopItem * desktop_item)
{
	const gchar *doc_path;
	gchar *help_uri;

	GError *error;

	if (!desktop_item)
		return FALSE;

	doc_path = mate_desktop_item_get_string (desktop_item, "DocPath");

	if (doc_path)
	{
		help_uri = g_strdup_printf ("ghelp:%s", doc_path);

		error = NULL;
		if (!gtk_show_uri (libslab_get_current_screen (), help_uri, gtk_get_current_event_time (), &error))
		{
			g_warning ("error opening %s [%s]\n", help_uri, error->message);

			g_free (help_uri);
			g_error_free (error);
			return FALSE;
		}

		g_free (help_uri);
	}
	else
		return FALSE;

	return TRUE;
}

gboolean
desktop_item_is_in_main_menu (MateDesktopItem * desktop_item)
{
	return desktop_uri_is_in_main_menu (mate_desktop_item_get_location (desktop_item));
}

gboolean
desktop_uri_is_in_main_menu (const gchar * uri)
{
	GSList *app_list;

	GSList *node;
	gint offset;
	gint uri_len;
	gboolean found = FALSE;

	app_list = get_slab_mateconf_slist (SLAB_USER_SPECIFIED_APPS_KEY);

	if (!app_list)
		return FALSE;

	uri_len = strlen (uri);

	for (node = app_list; node; node = node->next)
	{
		offset = uri_len - strlen ((gchar *) node->data);

		if (offset < 0)
			offset = 0;

		if (!strcmp (&uri[offset], (gchar *) node->data))
		{
			found = TRUE;
			break;
		}
	}

	free_slab_mateconf_slist_of_strings (app_list);
	return found;
}

gint
desktop_item_location_compare (gconstpointer a_obj, gconstpointer b_obj)
{
	const gchar *a;
	const gchar *b;

	gint offset;

	a = (const gchar *) a_obj;
	b = (const gchar *) b_obj;

	offset = strlen (a) - strlen (b);

	if (offset > 0)
		return strcmp (&a[offset], b);
	else if (offset < 0)
		return strcmp (a, &b[-offset]);
	else
		return strcmp (a, b);
}

gboolean
slab_load_image (GtkImage * image, GtkIconSize size, const gchar * image_id)
{
	GdkPixbuf *pixbuf;
	gint width;
	gint height;

	gchar *id;

	if (!image_id)
		return FALSE;

	id = g_strdup (image_id);

	gtk_icon_size_lookup (size, &width, &height);

	if (g_path_is_absolute (id))
		pixbuf = gdk_pixbuf_new_from_file_at_size (id, width, height, NULL);
	else
	{
		if (	/* file extensions are not copesetic with loading by "name" */
			g_str_has_suffix (id, ".png") ||
			g_str_has_suffix (id, ".svg") ||
			g_str_has_suffix (id, ".xpm")
		   )

			id[strlen (id) - 4] = '\0';

		pixbuf = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (), id, width, 0,
			NULL);
	}

	if (pixbuf)
	{
		gtk_image_set_from_pixbuf (image, pixbuf);

		g_object_unref (pixbuf);

		g_free (id);

		return TRUE;
	}
	else
	{			/* This will make it show the "broken image" icon */
		gtk_image_set_from_file (image, id);

		g_free (id);

		return FALSE;
	}
}

gchar *
string_replace_once (const gchar * str_template, const gchar * key, const gchar * value)
{
	GString *str_built;
	gint pivot;

	pivot = strstr (str_template, key) - str_template;

	str_built = g_string_new_len (str_template, pivot);
	g_string_append (str_built, value);
	g_string_append (str_built, &str_template[pivot + strlen (key)]);

	return g_string_free (str_built, FALSE);
}

void
spawn_process (const gchar *command)
{
	gchar **argv;
	GError *error = NULL;

	if (!command || strlen (command) < 1)
		return;

	argv = g_strsplit (command, " ", -1);

	g_spawn_async (NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error);

	if (error)
	{
		g_warning ("error spawning [%s]: [%s]\n", command, error->message);

		g_error_free (error);
	}

	g_strfreev (argv);
}

void
copy_file (const gchar * src_uri, const gchar * dst_uri)
{
	GFile *src;
	GFile *dst;
	GError *error = NULL;
	gboolean res;

	src = g_file_new_for_uri (src_uri);
	dst = g_file_new_for_uri (dst_uri);

	res = g_file_copy (src, dst,
			   G_FILE_COPY_NONE,
			   NULL, NULL, NULL, &error);

	if (!res)
	{
		g_warning ("error copying [%s] to [%s]: %s.", src_uri, dst_uri, error->message);
		g_error_free (error);
	}

	g_object_unref (src);
	g_object_unref (dst);
}
