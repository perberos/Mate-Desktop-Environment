/* Popup menus for MATE
 *
 * Copyright (C) 1998 Mark Crichton
 * All rights reserved
 *
 * Authors: Mark Crichton <mcrichto@purdue.edu>
 *          Federico Mena <federico@nuclecu.unam.mx>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 */
/*
  @NOTATION@
*/

#ifndef MATE_POPUPMENU_H
#define MATE_POPUPMENU_H

#ifndef MATE_DISABLE_DEPRECATED

#include <gtk/gtk.h>
#include <libmateui/mate-app.h>
#include <libmateui/mate-app-helper.h>

#ifdef __cplusplus
extern "C" {
#endif

/* These routines are documented in mate-popup-menu.c */

GtkWidget *mate_popup_menu_new (MateUIInfo *uiinfo);
GtkWidget *mate_popup_menu_new_with_accelgroup (MateUIInfo *uiinfo,
						 GtkAccelGroup *accelgroup);
GtkAccelGroup *mate_popup_menu_get_accel_group(GtkMenu *menu);


void mate_popup_menu_attach (GtkWidget *popup, GtkWidget *widget,
			      gpointer user_data);

void mate_popup_menu_do_popup (GtkWidget *popup, GtkMenuPositionFunc pos_func, gpointer pos_data,
				GdkEventButton *event, gpointer user_data, GtkWidget *for_widget);

int mate_popup_menu_do_popup_modal (GtkWidget *popup, GtkMenuPositionFunc pos_func, gpointer pos_data,
				     GdkEventButton *event, gpointer user_data, GtkWidget *for_widget);

void mate_popup_menu_append (GtkWidget *popup, MateUIInfo *uiinfo);

/*** This layer, on top of the mate_popup_menu_*() routines, defines a standard way of
listing items on a widget's popup ****/
void mate_gtk_widget_add_popup_items (GtkWidget *widget, MateUIInfo *uiinfo, gpointer user_data);

#ifdef __cplusplus
}
#endif

#endif /* MATE_DISABLE_DEPRECATED */

#endif /* MATE_POPUPMENU_H */
