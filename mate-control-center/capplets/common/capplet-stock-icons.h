/*
 * capplet-stock-icons.h
 *
 * Copyright (C) 2002 Sun Microsystems, Inc.
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Authors:
 * 	Rajkumar Sivasamy <rajkumar.siva@wipro.com>
 * 	Taken bits of code from panel-stock-icons.h, Thanks Mark <mark@skynet.ie>
 */

#ifndef __CAPPLET_STOCK_ICONS_H__
#define __CAPPLET_STOCK_ICONS_H__

#include <glib.h>
#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KEYBOARD_CAPPLET_DEFAULT_ICON_SIZE	48
#define MOUSE_CAPPLET_DEFAULT_WIDTH		120
#define MOUSE_CAPPLET_DEFAULT_HEIGHT		100
#define MOUSE_CAPPLET_DBLCLCK_ICON_SIZE		100

/* stock icons */
#define KEYBOARD_REPEAT			"keyboard-repeat"
#define KEYBOARD_CURSOR			"keyboard-cursor"
#define KEYBOARD_VOLUME			"keyboard-volume"
#define KEYBOARD_BELL			"keyboard-bell"
#define ACCESSX_KEYBOARD_BOUNCE 	"accessibility-keyboard-bouncekey"
#define ACCESSX_KEYBOARD_SLOW 		"accessibility-keyboard-slowkey"
#define ACCESSX_KEYBOARD_MOUSE 		"accessibility-keyboard-mousekey"
#define ACCESSX_KEYBOARD_STICK 		"accessibility-keyboard-stickykey"
#define ACCESSX_KEYBOARD_TOGGLE 	"accessibility-keyboard-togglekey"
#define MOUSE_DBLCLCK_MAYBE		"mouse-dblclck-maybe"
#define MOUSE_DBLCLCK_ON		"mouse-dblclck-on"
#define MOUSE_DBLCLCK_OFF		"mouse-dblclck-off"
#define MOUSE_RIGHT_HANDED		"mouse-right-handed"
#define MOUSE_LEFT_HANDED		"mouse-left-handed"

void        capplet_init_stock_icons			(void);
GtkIconSize keyboard_capplet_icon_get_size		(void);
GtkIconSize mouse_capplet_icon_get_size			(void);
GtkIconSize mouse_capplet_dblclck_icon_get_size		(void);

#ifdef __cplusplus
}
#endif

#endif /* __CAPPLET_STOCK_ICONS_H__ */
