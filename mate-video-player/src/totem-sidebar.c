/* totem-sidebar.c

   Copyright (C) 2004-2005 Bastien Nocera

   The Mate Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301  USA.

   Author: Bastien Nocera <hadess@hadess.net>
 */

#include "config.h"

#include <glib/gi18n.h>

#include "totem.h"
#include "totem-sidebar.h"
#include "totem-private.h"
#include "ev-sidebar.h"

static void
cb_resize (Totem * totem)
{
	GValue gvalue_size = { 0, };
	gint handle_size;
	GtkWidget *pane;
	GtkAllocation allocation;
	int w, h;

	gtk_widget_get_allocation (totem->win, &allocation);
	w = allocation.width;
	h = allocation.height;

	g_value_init (&gvalue_size, G_TYPE_INT);
	pane = GTK_WIDGET (gtk_builder_get_object (totem->xml, "tmw_main_pane"));
	gtk_widget_style_get_property (pane, "handle-size", &gvalue_size);
	handle_size = g_value_get_int (&gvalue_size);
	g_value_unset (&gvalue_size);
	
	gtk_widget_get_allocation (totem->sidebar, &allocation);
	if (totem->sidebar_shown) {
		w += allocation.width + handle_size;
	} else {
		w -= allocation.width + handle_size;
	}

	if (w > 0 && h > 0)
		gtk_window_resize (GTK_WINDOW (totem->win), w, h);
}

void
totem_sidebar_toggle (Totem *totem, gboolean state)
{
	GtkAction *action;
	GtkWidget *box, *arrow;

	if (gtk_widget_get_visible (GTK_WIDGET (totem->sidebar)) == state)
		return;

	if (state != FALSE)
		gtk_widget_show (GTK_WIDGET (totem->sidebar));
	else
		gtk_widget_hide (GTK_WIDGET (totem->sidebar));

	action = gtk_action_group_get_action (totem->main_action_group, "sidebar");
	totem_signal_block_by_data (G_OBJECT (action), totem);
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), state);
	totem_signal_unblock_by_data (G_OBJECT (action), totem);

	box = GTK_WIDGET (gtk_builder_get_object (totem->xml, "tmw_sidebar_button_hbox"));
	arrow = g_object_get_data (G_OBJECT (box), "arrow");
	gtk_arrow_set (GTK_ARROW (arrow), state ? GTK_ARROW_LEFT : GTK_ARROW_RIGHT, GTK_SHADOW_NONE);

	totem->sidebar_shown = state;
	cb_resize(totem);
}

static void
toggle_sidebar_from_sidebar (GtkWidget *playlist, Totem *totem)
{
	totem_sidebar_toggle (totem, FALSE);
}

gboolean
totem_sidebar_is_visible (Totem *totem)
{
	return totem->sidebar_shown;
}

static gboolean
has_popup (void)
{
	GList *list, *l;
	gboolean retval = FALSE;

	list = gtk_window_list_toplevels ();
	for (l = list; l != NULL; l = l->next) {
		GtkWindow *window = GTK_WINDOW (l->data);
		if (gtk_widget_get_visible (GTK_WIDGET (window)) && gtk_window_get_window_type (window) == GTK_WINDOW_POPUP) {
			retval = TRUE;
			break;
		}
	}
	g_list_free (list);
	return retval;
}

gboolean
totem_sidebar_is_focused (Totem *totem, gboolean *handles_kbd)
{
	GtkWidget *focused;

	if (handles_kbd != NULL)
		*handles_kbd = has_popup ();

	focused = gtk_window_get_focus (GTK_WINDOW (totem->win));
	if (focused != NULL && gtk_widget_is_ancestor
			(focused, GTK_WIDGET (totem->sidebar)) != FALSE) {
		return TRUE;
	}

	return FALSE;
}

void
totem_sidebar_setup (Totem *totem, gboolean visible, const char *page_id)
{
	GtkPaned *item;
	GtkAction *action;

	item = GTK_PANED (gtk_builder_get_object (totem->xml, "tmw_main_pane"));
	totem->sidebar = ev_sidebar_new ();
	ev_sidebar_add_page (EV_SIDEBAR (totem->sidebar),
			"playlist", _("Playlist"),
			GTK_WIDGET (totem->playlist));
	if (page_id != NULL) {
		ev_sidebar_set_current_page (EV_SIDEBAR (totem->sidebar),
				page_id);
	} else {
		ev_sidebar_set_current_page (EV_SIDEBAR (totem->sidebar),
				"playlist");
	}
	gtk_paned_pack2 (item, totem->sidebar, FALSE, FALSE);

	totem->sidebar_shown = visible;

	action = gtk_action_group_get_action (totem->main_action_group,
			"sidebar");

	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), visible);

	/* Signals */
	g_signal_connect (G_OBJECT (totem->sidebar), "closed",
			G_CALLBACK (toggle_sidebar_from_sidebar), totem);

	gtk_widget_show_all (totem->sidebar);
	gtk_widget_realize (totem->sidebar);

	if (!visible) {
		gtk_widget_hide (totem->sidebar);
	} else {
		GtkWidget *box, *arrow;

		box = GTK_WIDGET (gtk_builder_get_object (totem->xml, "tmw_sidebar_button_hbox"));
		arrow = g_object_get_data (G_OBJECT (box), "arrow");
		gtk_arrow_set (GTK_ARROW (arrow), GTK_ARROW_LEFT, GTK_SHADOW_NONE);
	}
}

char *
totem_sidebar_get_current_page (Totem *totem)
{
	if (totem->sidebar == NULL)
		return NULL;
	return ev_sidebar_get_current_page (EV_SIDEBAR (totem->sidebar));
}

void
totem_sidebar_set_current_page (Totem *totem,
				const char *name,
				gboolean force_visible)
{
	if (name == NULL)
		return;

	ev_sidebar_set_current_page (EV_SIDEBAR (totem->sidebar), name);
	if (force_visible != FALSE)
		totem_sidebar_toggle (totem, TRUE);
}

