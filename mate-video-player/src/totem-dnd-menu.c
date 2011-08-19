/* 
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
 *
 * The Totem project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and Totem. This
 * permission are above and beyond the permissions granted by the GPL license
 * Totem is covered by.
 */

#include "config.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <gdk/gdkkeysyms.h>

#include "totem-dnd-menu.h"

typedef struct
{
	GMainLoop *loop;
	GdkDragAction ch;
} DragData;

static void
drag_menu_deactivate_callback (GtkWidget *menu,
			       DragData *dt)
{
	dt->ch = GDK_ACTION_DEFAULT;
	if (g_main_loop_is_running (dt->loop))
		g_main_loop_quit (dt->loop);
}

static void
drag_drop_action_activated_callback (GtkWidget  *menu_item,
				     DragData       *dt)
{
	dt->ch = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (menu_item), "action"));
	if (g_main_loop_is_running (dt->loop))
		g_main_loop_quit (dt->loop);
}

static void
drag_append_drop_action_menu_item (GtkWidget          *menu,
				   const char    *text,
				   const char    *icon,
				   GdkDragAction action,
				   DragData       *dt)
{
	GtkWidget *menu_item;

	menu_item = gtk_image_menu_item_new_with_mnemonic (text);
	gtk_widget_set_sensitive (menu_item, TRUE);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

	if (icon != NULL) {
		GtkWidget *image;

		image = gtk_image_new_from_stock (icon, GTK_ICON_SIZE_MENU);
		gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), image);
	}

	g_object_set_data (G_OBJECT (menu_item), "action", GINT_TO_POINTER (action));

	g_signal_connect (menu_item, "activate",
			  G_CALLBACK (drag_drop_action_activated_callback), dt);

	gtk_widget_show (menu_item);
}

GdkDragAction
totem_drag_ask (gboolean show_add_to)
{
	GtkWidget *menu;
	GtkWidget *menu_item;
	DragData dt;

	dt.ch = 0;

	menu = gtk_menu_new ();

	drag_append_drop_action_menu_item (menu, _("_Play Now"), "gtk-media-play", GDK_ACTION_MOVE, &dt);

	if (show_add_to != FALSE)
		drag_append_drop_action_menu_item (menu, _("_Add to Playlist"), "gtk-add", GDK_ACTION_COPY, &dt);

	menu_item = gtk_separator_menu_item_new ();
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);

	drag_append_drop_action_menu_item (menu, _("Cancel"), NULL, GDK_ACTION_DEFAULT, &dt);

	g_signal_connect (menu, "deactivate",
			  G_CALLBACK (drag_menu_deactivate_callback), &dt);

	gtk_grab_add (menu);

	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
			3, gtk_get_current_event_time ());

	dt.loop = g_main_loop_new (NULL, FALSE);
	g_main_loop_run (dt.loop);
	gtk_grab_remove (menu);
	g_main_loop_unref (dt.loop);

	gtk_widget_destroy (menu);

	return dt.ch;
}
