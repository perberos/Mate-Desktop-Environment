/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2001, 2002 Anders Carlsson <andersca@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "mateconf-bookmarks.h"

#include "mateconf-stock-icons.h"
#include "mateconf-tree-model.h"
#include <mateconf/mateconf-client.h>
#include <gtk/gtk.h>
#include <string.h>


static void
mateconf_bookmarks_bookmark_activated (GtkWidget *menuitem, MateConfEditorWindow *window)
{
	mateconf_editor_window_go_to (window,
				   g_object_get_data (G_OBJECT (menuitem), "mateconf-key"));
}

static void
mateconf_bookmarks_set_item_has_icon (GtkWidget *item,
				   gboolean   have_icons)
{
	GtkWidget *image;

	image = gtk_image_menu_item_get_image (GTK_IMAGE_MENU_ITEM (item));
	if (image && !g_object_get_data (G_OBJECT (item), "mateconf-editor-icon"))
		g_object_set_data_full (G_OBJECT (item), "mateconf-editor-icon",
					g_object_ref (image), g_object_unref);

	if (!image)
		image = g_object_get_data (G_OBJECT (item), "mateconf-editor-icon");

	if (!image)
		return;

	if (have_icons)
		gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
	else
		gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), NULL);
}

static void
mateconf_bookmarks_set_have_icons (GtkWidget *menu, gboolean have_icons)
{
        GList *items, *n;

        items = gtk_container_get_children (GTK_CONTAINER (menu));

        for (n = items; n != NULL; n = n->next) 
                if (GTK_IS_IMAGE_MENU_ITEM (n->data))
                        mateconf_bookmarks_set_item_has_icon (GTK_WIDGET (n->data), have_icons);
}

static void
mateconf_bookmarks_have_icons_notify (MateConfClient       *client,
				   guint              cnxn_id,
				   MateConfEntry        *entry,
				   gpointer           data)
{
        GtkWidget *menu;
	gboolean have_icons;

        menu = GTK_WIDGET (data);

	if (entry->value->type != MATECONF_VALUE_BOOL)
		return;

	have_icons = mateconf_value_get_bool (entry->value);

	mateconf_bookmarks_set_have_icons (menu, have_icons);
}

static void
mateconf_bookmarks_update_menu (GtkWidget *menu)
{
	GSList *list, *tmp;
	GtkWidget *menuitem, *window;
	MateConfClient *client;

	window = g_object_get_data (G_OBJECT (menu), "editor-window");
	client = mateconf_client_get_default ();
	
	/* Get the old list and then set it */
	list = mateconf_client_get_list (client,
				     "/apps/mateconf-editor/bookmarks", MATECONF_VALUE_STRING, NULL);

	if (list != NULL) {
		menuitem = gtk_separator_menu_item_new ();
		gtk_widget_show (menuitem);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
	}

	for (tmp = list; tmp; tmp = tmp->next) {
		menuitem = gtk_image_menu_item_new_with_label (tmp->data);
		g_signal_connect (menuitem, "activate",
				  G_CALLBACK (mateconf_bookmarks_bookmark_activated), window);
		g_object_set_data_full (G_OBJECT (menuitem), "mateconf-key", g_strdup (tmp->data), g_free);
		gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menuitem), gtk_image_new_from_stock (STOCK_BOOKMARK,
													 GTK_ICON_SIZE_MENU));
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
		gtk_widget_show_all (menuitem);

		g_free (tmp->data);
	}

	g_object_unref (client);
	g_slist_free (list);
}

static void
mateconf_bookmarks_key_changed (MateConfClient *client, guint cnxn_id, MateConfEntry *entry, gpointer user_data)
{
	GList *child_list, *tmp;
	GtkWidget *menu_item;
	
	child_list = gtk_container_get_children (GTK_CONTAINER (user_data));

	for (tmp = child_list; tmp; tmp = tmp->next) {
		menu_item = tmp->data;

		if (g_object_get_data (G_OBJECT (menu_item), "mateconf-key") != NULL ||
			GTK_IS_SEPARATOR_MENU_ITEM (menu_item)) {
			gtk_widget_destroy (menu_item);
		}
	}

	mateconf_bookmarks_update_menu (GTK_WIDGET (user_data));
	
	g_list_free (child_list);
}

void
mateconf_bookmarks_add_bookmark (const char *path)
{
	GSList *list, *tmp;
	MateConfClient *client;

	client = mateconf_client_get_default ();

	/* Get the old list and then set it */
	list = mateconf_client_get_list (client,
				     "/apps/mateconf-editor/bookmarks", MATECONF_VALUE_STRING, NULL);

	/* FIXME: We need error handling here, also this function leaks memory */

	/* Check that the bookmark hasn't been added already */
	for (tmp = list; tmp; tmp = tmp->next) {
		if (strcmp (tmp->data, path) == 0) {
			g_slist_free (list);
			return;
		}
	}

	/* Append the new bookmark */
	list = g_slist_append (list, g_strdup (path));
	
	mateconf_client_set_list (client,
			       "/apps/mateconf-editor/bookmarks", MATECONF_VALUE_STRING, list, NULL);
	g_slist_free (list);
	g_object_unref (client);
}

static void
remove_notify_id (gpointer data)
{
	MateConfClient *client;

	client = mateconf_client_get_default ();
	mateconf_client_notify_remove (client, GPOINTER_TO_INT (data));

	g_object_unref (client);
}

void
mateconf_bookmarks_hook_up_menu (MateConfEditorWindow *window,
			      GtkWidget *menu,
			      GtkWidget *add_bookmark,
			      GtkWidget *edit_bookmarks)
{
	MateConfClient *client;
	guint notify_id;

	g_object_set_data (G_OBJECT (menu), "editor-window", window);

	client = mateconf_client_get_default ();
	
	/* Add a notify function */
	mateconf_client_add_dir (client, "/apps/mateconf-editor/bookmarks",
			      MATECONF_CLIENT_PRELOAD_ONELEVEL, NULL);
	notify_id = mateconf_client_notify_add (client, "/apps/mateconf-editor/bookmarks",
					     mateconf_bookmarks_key_changed, menu, NULL, NULL);
	g_object_set_data_full (G_OBJECT (menu), "notify-id", GINT_TO_POINTER (notify_id),
				remove_notify_id);


	notify_id = mateconf_client_notify_add (client, "/desktop/mate/interface/menus_have_icons",
					     mateconf_bookmarks_have_icons_notify, menu, NULL, NULL); 
	g_object_set_data_full (G_OBJECT (menu), "notify-id-x", GINT_TO_POINTER (notify_id),
				remove_notify_id);

	mateconf_bookmarks_update_menu (menu);

        {
                gboolean have_icons;
                MateConfValue *value;
                GError *err;

                err = NULL;
                value = mateconf_client_get (client, "/desktop/mate/interface/menus_have_icons", &err);

                if (err != NULL || value == NULL || value->type != MATECONF_VALUE_BOOL)
                        return;

                have_icons = mateconf_value_get_bool (value);
                mateconf_bookmarks_set_have_icons (menu, have_icons);

                mateconf_value_free (value);
        }

	if ( ! mateconf_client_key_is_writable (client, "/apps/mateconf-editor/bookmarks", NULL)) {
		gtk_widget_set_sensitive (add_bookmark, FALSE);
		gtk_widget_set_sensitive (edit_bookmarks, FALSE);
	}

	g_object_unref (client);
}
