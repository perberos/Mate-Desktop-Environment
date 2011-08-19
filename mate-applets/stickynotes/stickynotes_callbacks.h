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

#ifndef __STICKYNOTES_CALLBACKS_H__
#define __STICKYNOTES_CALLBACKS_H__

#include <stickynotes.h>

/* Callbacks for the sticky notes windows */
gboolean stickynote_toggle_lock_cb(GtkWidget *widget, StickyNote *note);
gboolean stickynote_close_cb(GtkWidget *widget, StickyNote *note);
gboolean stickynote_resize_cb(GtkWidget *widget, GdkEventButton *event, StickyNote *note);
gboolean stickynote_move_cb(GtkWidget *widget, GdkEventButton *event, StickyNote *note);
gboolean stickynote_expose_cb(GtkWidget *widget, GdkEventExpose *event, StickyNote *note);
gboolean stickynote_configure_cb(GtkWidget *widget, GdkEventConfigure *event, StickyNote *note);
gboolean stickynote_delete_cb(GtkWidget *widget, GdkEvent *event, StickyNote *note);
gboolean stickynote_show_popup_menu(GtkWidget *widget, GdkEventButton *event, GtkWidget *popup_menu);

/* Callbacks for the sticky notes popup menu */
void popup_create_cb(GtkWidget *widget, StickyNote *note);
void popup_destroy_cb(GtkWidget *widget, StickyNote *note);
void popup_toggle_lock_cb(GtkToggleAction *action, StickyNote *note);
void popup_properties_cb(GtkWidget *widget, StickyNote *note);

/* Callbacks for sticky notes properties dialog */
void properties_apply_title_cb(StickyNote *note);
void properties_apply_color_cb(StickyNote *note);
void properties_apply_font_cb(StickyNote *note);
void properties_color_cb(GtkWidget *button, StickyNote *note);
void properties_font_cb(GtkWidget *button, StickyNote *note);
void properties_activate_cb(GtkWidget *widget, StickyNote *note);

#endif /* __STICKYNOTES_CALLBACKS_H__ */
