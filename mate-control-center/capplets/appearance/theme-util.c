/*
 * Copyright (C) 2007 The MATE Foundation
 * Written by Thomas Wood <thos@gnome.org>
 *            Jens Granseuer <jensgr@gmx.net>
 * All Rights Reserved
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "appearance.h"

#include <string.h>
#include <glib/gi18n.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include "capplet-util.h"
#include "theme-util.h"

gboolean
theme_is_writable (const gpointer theme)
{
  MateThemeCommonInfo *info = theme;
  GFile *file;
  GFileInfo *file_info;
  gboolean writable;

  if (info == NULL || info->path == NULL)
    return FALSE;

  file = g_file_new_for_path (info->path);
  file_info = g_file_query_info (file,
                                 G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE,
                                 G_FILE_QUERY_INFO_NONE,
                                 NULL, NULL);
  g_object_unref (file);

  if (file_info == NULL)
    return FALSE;

  writable = g_file_info_get_attribute_boolean (file_info,
                                                G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE);
  g_object_unref (file_info);

  return writable;
}

gboolean
theme_delete (const gchar *name, ThemeType type)
{
  gboolean rc;
  GtkDialog *dialog;
  gchar *theme_dir;
  gint response;
  MateThemeCommonInfo *theme;
  GFile *dir;
  gboolean del_empty_parent;

  dialog = (GtkDialog *) gtk_message_dialog_new (NULL,
						 GTK_DIALOG_MODAL,
						 GTK_MESSAGE_QUESTION,
						 GTK_BUTTONS_CANCEL,
						 _("Would you like to delete this theme?"));
  gtk_dialog_add_button (dialog, GTK_STOCK_DELETE, GTK_RESPONSE_ACCEPT);
  response = gtk_dialog_run (dialog);
  gtk_widget_destroy (GTK_WIDGET (dialog));
  if (response != GTK_RESPONSE_ACCEPT)
    return FALSE;

  /* Most theme types are put into separate subdirectories. For those
     we want to delete those directories as well. */
  del_empty_parent = TRUE;

  switch (type) {
    case THEME_TYPE_GTK:
      theme = (MateThemeCommonInfo *) mate_theme_info_find (name);
      theme_dir = g_build_filename (theme->path, "gtk-2.0", NULL);
      break;

    case THEME_TYPE_ICON:
      theme = (MateThemeCommonInfo *) mate_theme_icon_info_find (name);
      theme_dir = g_path_get_dirname (theme->path);
      del_empty_parent = FALSE;
      break;

    case THEME_TYPE_WINDOW:
      theme = (MateThemeCommonInfo *) mate_theme_info_find (name);
      theme_dir = g_build_filename (theme->path, "marco-1", NULL);
      break;

    case THEME_TYPE_META:
      theme = (MateThemeCommonInfo *) mate_theme_meta_info_find (name);
      theme_dir = g_strdup (theme->path);
      break;

    case THEME_TYPE_CURSOR:
      theme = (MateThemeCommonInfo *) mate_theme_cursor_info_find (name);
      theme_dir = g_build_filename (theme->path, "cursors", NULL);
      break;

    default:
      return FALSE;
  }

  dir = g_file_new_for_path (theme_dir);
  g_free (theme_dir);

  if (!capplet_file_delete_recursive (dir, NULL)) {
    GtkWidget *info_dialog = gtk_message_dialog_new (NULL,
						     GTK_DIALOG_MODAL,
						     GTK_MESSAGE_ERROR,
						     GTK_BUTTONS_OK,
						     _("Theme cannot be deleted"));
    gtk_dialog_run (GTK_DIALOG (info_dialog));
    gtk_widget_destroy (info_dialog);
    rc = FALSE;
  } else {
    if (del_empty_parent) {
      /* also delete empty parent directories */
      GFile *parent = g_file_get_parent (dir);
      g_file_delete (parent, NULL, NULL);
      g_object_unref (parent);
    }
    rc = TRUE;
  }

  g_object_unref (dir);
  return rc;
}

gboolean
theme_model_iter_last (GtkTreeModel *model, GtkTreeIter *iter)
{
  GtkTreeIter walk, prev;
  gboolean valid;

  valid = gtk_tree_model_get_iter_first (model, &walk);

  if (valid) {
    do {
      prev = walk;
      valid = gtk_tree_model_iter_next (model, &walk);
    } while (valid);

    *iter = prev;
    return TRUE;
  }
  return FALSE;
}

gboolean
theme_find_in_model (GtkTreeModel *model, const gchar *name, GtkTreeIter *iter)
{
  GtkTreeIter walk;
  gboolean valid;
  gchar *test;

  if (!name)
    return FALSE;

  for (valid = gtk_tree_model_get_iter_first (model, &walk); valid;
       valid = gtk_tree_model_iter_next (model, &walk))
  {
    gtk_tree_model_get (model, &walk, COL_NAME, &test, -1);

    if (test) {
      gint cmp = strcmp (test, name);
      g_free (test);

      if (!cmp) {
      	if (iter)
          *iter = walk;
        return TRUE;
      }
    }
  }

  return FALSE;
}

gboolean
packagekit_available (void)
{
  DBusGConnection *connection;
  DBusGProxy *proxy;
  gboolean available = FALSE;

  connection = dbus_g_bus_get (DBUS_BUS_SESSION, NULL);
  if (connection == NULL) {
    return FALSE;
  }

  proxy = dbus_g_proxy_new_for_name (connection,
                                     DBUS_SERVICE_DBUS,
                                     DBUS_PATH_DBUS,
                                     DBUS_INTERFACE_DBUS);

  org_freedesktop_DBus_name_has_owner (proxy,
                                       "org.freedesktop.PackageKit",
                                       &available,
                                       NULL);

  g_object_unref (proxy);
  dbus_g_connection_unref (connection);

  return available;
}

void theme_install_file(GtkWindow* parent, const gchar* path)
{
	DBusGConnection* connection;
	DBusGProxy* proxy;
	GError* error = NULL;
	gboolean ret;

	connection = dbus_g_bus_get(DBUS_BUS_SESSION, NULL);

	if (connection == NULL)
	{
		g_warning("Could not get session bus");
		return;
	}

	proxy = dbus_g_proxy_new_for_name(connection,
		"org.freedesktop.PackageKit",
		"/org/freedesktop/PackageKit",
		"org.freedesktop.PackageKit");


	ret = dbus_g_proxy_call(proxy, "InstallProvideFile", &error,
		G_TYPE_STRING, path,
		G_TYPE_INVALID, G_TYPE_INVALID);

	g_object_unref(proxy);

	if (!ret)
	{
		GtkWidget* dialog = gtk_message_dialog_new(NULL,
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_ERROR,
			GTK_BUTTONS_OK,
			_("Could not install theme engine"));
		gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG (dialog), "%s", error->message);

		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		g_error_free(error);
	}

	dbus_g_connection_unref(connection);
}
