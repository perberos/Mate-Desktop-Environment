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

#include <stickynotes_applet.h>

#ifndef __STICKYNOTES_APPLET_CALLBACKS_H__
#define __STICKYNOTES_APPLET_CALLBACKS_H__

/* Callbacks for the sticky notes applet */
gboolean applet_button_cb(GtkWidget *widget, GdkEventButton *event, StickyNotesApplet *applet);
gboolean applet_key_cb(GtkWidget *widget, GdkEventKey *event, StickyNotesApplet *applet);
gboolean applet_cross_cb(GtkWidget *widget, GdkEventCrossing *event, StickyNotesApplet *applet);
gboolean applet_focus_cb(GtkWidget *widget, GdkEventFocus *event, StickyNotesApplet *applet);
void install_check_click_on_desktop (void);
void applet_change_orient_cb(MatePanelApplet *mate_panel_applet, MatePanelAppletOrient orient, StickyNotesApplet *applet);
void applet_size_allocate_cb(GtkWidget *widget, GtkAllocation *allocation, StickyNotesApplet *applet);
void applet_change_bg_cb (MatePanelApplet *mate_panel_applet,
				 MatePanelAppletBackgroundType type,
				 GdkColor *color,
                     		 GdkPixmap *pixmap,
                     		 StickyNotesApplet *applet);
void applet_destroy_cb (MatePanelApplet *mate_panel_applet, StickyNotesApplet *applet);
/* Callbacks for sticky notes applet menu */
void menu_create_cb(GtkAction *action, StickyNotesApplet *applet);
void menu_new_note_cb(GtkAction *action, StickyNotesApplet *applet);
void menu_hide_notes_cb(GtkAction *action, StickyNotesApplet *applet);
void menu_destroy_all_cb(GtkAction *action, StickyNotesApplet *applet);
void menu_toggle_lock_cb(GtkAction *action, StickyNotesApplet *applet);
void menu_preferences_cb(GtkAction *action, StickyNotesApplet *applet);
void menu_help_cb(GtkAction *action, StickyNotesApplet *applet);
void menu_about_cb(GtkAction *action, StickyNotesApplet *applet);

/* Callbacks for sticky notes preferences dialog */
void preferences_save_cb(gpointer data);
void preferences_color_cb (GtkWidget *button, gpointer data);
void preferences_font_cb (GtkWidget *button, gpointer data);
void preferences_apply_cb(MateConfClient *client, guint cnxn_id, MateConfEntry *entry, gpointer data);
void preferences_response_cb(GtkWidget *dialog, gint response, gpointer data);
gboolean preferences_delete_cb(GtkWidget *widget, GdkEvent *event, gpointer data);

#endif /* __STICKYNOTES_APPLET_CALLBACKS_H__ */
