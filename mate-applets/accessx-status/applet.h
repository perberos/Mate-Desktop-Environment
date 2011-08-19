/* Keyboard Accessibility Status Applet
 * Copyright 2003 Sun Microsystems Inc.
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
 */

#ifndef __ACCESSX_APPLET_H__
#define __ACCESSX_APPLET_H__

#include <gtk/gtk.h>

#include <mate-panel-applet.h>

#define ACCESSX_APPLET          "ax-applet"
#define ACCESSX_BASE_ICON       "ax-base"
#define ACCESSX_ACCEPT_BASE     "ax-accept"
#define ACCESSX_REJECT_BASE     "ax-reject"
#define MOUSEKEYS_BASE_ICON     "ax-mouse-base"
#define MOUSEKEYS_BUTTON_LEFT   "ax-button-left"
#define MOUSEKEYS_BUTTON_MIDDLE "ax-button-middle"
#define MOUSEKEYS_BUTTON_RIGHT  "ax-button-right"
#define MOUSEKEYS_DOT_LEFT      "ax-dot-left"
#define MOUSEKEYS_DOT_MIDDLE    "ax-dot-middle"
#define MOUSEKEYS_DOT_RIGHT     "ax-dot-right"
#define SHIFT_KEY_ICON      "ax-shift-key"
#define CONTROL_KEY_ICON    "ax-control-key"
#define ALT_KEY_ICON        "ax-alt-key"
#define META_KEY_ICON       "ax-meta-key"
#define SUPER_KEY_ICON      "ax-super-key"
#define HYPER_KEY_ICON      "ax-hyper-key"
#define ALTGRAPH_KEY_ICON      "ax-altgraph-key"
#define SLOWKEYS_IDLE_ICON  "ax-sk-idle"
#define SLOWKEYS_PENDING_ICON "ax-sk-pending"
#define SLOWKEYS_ACCEPT_ICON  "ax-sk-accept"
#define SLOWKEYS_REJECT_ICON  "ax-sk-reject"
#define BOUNCEKEYS_ICON     "ax-bouncekeys"

#define STATUS_APPLET_ICON_SIZE GTK_ICON_SIZE_LARGE_TOOLBAR
typedef enum {
	ACCESSX_STATUS_ERROR_NONE = 0,
	ACCESSX_STATUS_ERROR_XKB_DISABLED,
	ACCESSX_STATUS_ERROR_UNKNOWN
}AccessxStatusErrorType;

typedef struct 
{
	MatePanelApplet      *applet;
	GtkWidget        *box;
        GtkWidget        *idlefoo;
	GtkWidget        *mousefoo;
	GtkWidget        *stickyfoo;
	GtkWidget        *slowfoo;
	GtkWidget        *bouncefoo;
	GtkWidget        *shift_indicator;
	GtkWidget        *ctrl_indicator;
	GtkWidget        *alt_indicator;
	GtkWidget        *meta_indicator;
	GtkWidget        *hyper_indicator;
	GtkWidget        *super_indicator;
	GtkWidget        *alt_graph_indicator;
	MatePanelAppletOrient orient;
	GtkIconFactory   *icon_factory;
	gboolean          initialized; 
	XkbDescRec       *xkb;
	Display          *xkb_display;
	AccessxStatusErrorType error_type;
} AccessxStatusApplet;

typedef enum
{
	ACCESSX_STATUS_MODIFIERS = 1 << 0,
	ACCESSX_STATUS_SLOWKEYS = 1 << 1,
	ACCESSX_STATUS_BOUNCEKEYS = 1 << 2,
	ACCESSX_STATUS_MOUSEKEYS = 1 << 3,
	ACCESSX_STATUS_ENABLED = 1 << 4,
	ACCESSX_STATUS_ALL = 0xFFFF
} AccessxStatusNotifyType;

#endif
