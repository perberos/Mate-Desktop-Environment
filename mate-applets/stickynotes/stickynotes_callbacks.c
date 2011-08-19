/* Sticky Notes
 * Copyright (C) 2002-2003 Loban A Rahman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <config.h>
#include <stickynotes_callbacks.h>

/* Sticky Window Callback : Lock/Unlock the window */
gboolean
stickynote_toggle_lock_cb (GtkWidget *widget, StickyNote *note)
{
	stickynote_set_locked (note, !note->locked);

	return TRUE;
}

/* Sticky Window Callback : Close the window. */
gboolean
stickynote_close_cb (GtkWidget *widget, StickyNote *note)
{

	stickynotes_remove (note);
	
	return TRUE;
}

/* Sticky Window Callback : Resize the window. */
gboolean stickynote_resize_cb(GtkWidget *widget, GdkEventButton *event, StickyNote *note)
{
	if (event->type == GDK_BUTTON_PRESS && event->button == 1) {
		if (widget == note->w_resize_se)
			gtk_window_begin_resize_drag(GTK_WINDOW(note->w_window), GDK_WINDOW_EDGE_SOUTH_EAST,
						     event->button, event->x_root, event->y_root, event->time);
		else /* if (widget == note->w_resize_sw) */
			gtk_window_begin_resize_drag(GTK_WINDOW(note->w_window), GDK_WINDOW_EDGE_SOUTH_WEST,
						     event->button, event->x_root, event->y_root, event->time);
	}
	else
		return FALSE;
			
	return TRUE;
}

/* Sticky Window Callback : Move the window or edit the title. */
gboolean stickynote_move_cb(GtkWidget *widget, GdkEventButton *event, StickyNote *note)
{
	if (event->type == GDK_BUTTON_PRESS && event->button == 1)
		gtk_window_begin_move_drag(GTK_WINDOW(note->w_window), event->button, event->x_root, event->y_root, event->time);
	else if (event->type == GDK_2BUTTON_PRESS && event->button == 1)
		stickynote_change_properties(note);
	else
		return FALSE;

	return TRUE;
}

/* Sticky Window Callback : Skip taskbar and pager when exposing the widow */
gboolean stickynote_expose_cb(GtkWidget *widget, GdkEventExpose *event, StickyNote *note)
{
	gtk_window_set_skip_taskbar_hint(GTK_WINDOW(note->w_window), TRUE);
	gtk_window_set_skip_pager_hint(GTK_WINDOW(note->w_window), TRUE);
	
	return FALSE;
}

/* Sticky Window Callback : Store settings when resizing/moving the window */
gboolean stickynote_configure_cb(GtkWidget *widget, GdkEventConfigure *event, StickyNote *note)
{
	note->x = event->x;
	note->y = event->y;
	note->w = event->width;
	note->h = event->height;

	stickynotes_save();

	return FALSE;
}

/* Sticky Window Callback : Get confirmation when deleting the window. */
gboolean stickynote_delete_cb(GtkWidget *widget, GdkEvent *event, StickyNote *note)
{
	stickynotes_remove(note);

	return TRUE;
}

/* Sticky Window Callback : Popup the right click menu. */
gboolean
stickynote_show_popup_menu (GtkWidget *widget, GdkEventButton *event, GtkWidget *popup_menu)
{
  

	if (event->type == GDK_BUTTON_PRESS && event->button == 3)
	{
		gtk_menu_popup (GTK_MENU (popup_menu),
				NULL, NULL,
				NULL, NULL,
				event->button, event->time);
	}

	return FALSE;
}


/* Popup Menu Callback : Create a new sticky note */
void popup_create_cb(GtkWidget *widget, StickyNote *note)
{
	stickynotes_add(gtk_widget_get_screen(note->w_window));
}

/* Popup Menu Callback : Destroy selected sticky note */
void popup_destroy_cb(GtkWidget *widget, StickyNote *note)
{
	stickynotes_remove(note);
}

/* Popup Menu Callback : Lock/Unlock selected sticky note */
void popup_toggle_lock_cb(GtkToggleAction *action, StickyNote *note)
{
	stickynote_set_locked(note, gtk_toggle_action_get_active(action));
}

/* Popup Menu Callback : Change sticky note properties */
void popup_properties_cb(GtkWidget *widget, StickyNote *note)
{
	stickynote_change_properties(note);
}

/* Properties Dialog Callback : Apply title */
void properties_apply_title_cb(StickyNote *note)
{
	stickynote_set_title(note, gtk_entry_get_text(GTK_ENTRY(note->w_entry)));
}

/* Properties Dialog Callback : Apply color */
void properties_apply_color_cb(StickyNote *note)
{
	char *color_str = NULL;
	char *font_color_str = NULL;
	
	if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(note->w_def_color)))
	{
		GdkColor color;
		GdkColor font_color;
		
		gtk_color_button_get_color (GTK_COLOR_BUTTON (note->w_color),
				&color);
		gtk_color_button_get_color (
				GTK_COLOR_BUTTON (note->w_font_color),
				&font_color);
		color_str = g_strdup_printf ("#%.2x%.2x%.2x",
				color.red / 256,
				color.green / 256,
				color.blue / 256);
		font_color_str = g_strdup_printf ("#%.2x%.2x%.2x",
				font_color.red / 256,
				font_color.green / 256,
				font_color.blue / 256);
	}
	
	stickynote_set_color (note, color_str, font_color_str, TRUE);

	g_free (color_str);
	g_free (font_color_str);
}

/* Properties Dialog Callback : Apply font */
void properties_apply_font_cb(StickyNote *note)
{
	const gchar *font_str = NULL;
	
	if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(note->w_def_font)))
	{
		font_str = gtk_font_button_get_font_name (
				GTK_FONT_BUTTON (note->w_font));
	}

	stickynote_set_font(note, font_str, TRUE);
}

/* Properties Dialog Callback : Color */
void
properties_color_cb (GtkWidget *button, StickyNote *note)
{
	char *color_str = NULL;
	char *font_color_str = NULL;
	
	if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(note->w_def_color)))
	{
			GdkColor color;
			GdkColor font_color;
			
			gtk_color_button_get_color (
					GTK_COLOR_BUTTON (note->w_color),
					&color);
			gtk_color_button_get_color (
					GTK_COLOR_BUTTON (note->w_font_color),
					&font_color);
			color_str = g_strdup_printf ("#%.2x%.2x%.2x",
					color.red / 256,
					color.green / 256,
					color.blue / 256);
			font_color_str = g_strdup_printf ("#%.2x%.2x%.2x",
					font_color.red / 256,
					font_color.green / 256,
					font_color.blue / 256);
		}
	
	stickynote_set_color (note, color_str, font_color_str, TRUE);

	g_free (color_str);
	g_free (font_color_str);
}

/* Properties Dialog Callback : Font */
void properties_font_cb (GtkWidget *button, StickyNote *note)
{
        const char *font_str;

        font_str = gtk_font_button_get_font_name (GTK_FONT_BUTTON (button));

	stickynote_set_font(note, font_str, TRUE);
}

/* Properties Dialog Callback : Activate */
void properties_activate_cb(GtkWidget *widget, StickyNote *note)
{
	gtk_dialog_response(GTK_DIALOG(note->w_properties), GTK_RESPONSE_CLOSE);
}
