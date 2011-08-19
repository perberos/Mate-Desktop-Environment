/*
 *
 *  Copyright (C) 2009 Bastien Nocera <hadess@hadess.net>
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <X11/extensions/XInput.h>
#include <X11/extensions/XIproto.h>

#include "bluetooth-input.h"

enum {
	KEYBOARD_APPEARED,
	KEYBOARD_DISAPPEARED,
	MOUSE_APPEARED,
	MOUSE_DISAPPEARED,
	LAST_SIGNAL
};

static int signals[LAST_SIGNAL] = { 0 };

#define BLUETOOTH_INPUT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
				BLUETOOTH_TYPE_INPUT, BluetoothInputPrivate))

typedef struct _BluetoothInputPrivate BluetoothInputPrivate;

struct _BluetoothInputPrivate {
	char *foo;
};

G_DEFINE_TYPE(BluetoothInput, bluetooth_input, G_TYPE_OBJECT)

static void bluetooth_input_finalize(GObject *input)
{
	BluetoothInputPrivate *priv = BLUETOOTH_INPUT_GET_PRIVATE(input);

	G_OBJECT_CLASS(bluetooth_input_parent_class)->finalize(input);
}

static void bluetooth_input_class_init(BluetoothInputClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;

	g_type_class_add_private(klass, sizeof(BluetoothInputPrivate));

	object_class->finalize = bluetooth_input_finalize;

	signals[KEYBOARD_APPEARED] = g_signal_new ("keyboard-appeared",
						   G_TYPE_FROM_CLASS (klass),
						   G_SIGNAL_RUN_LAST,
						   G_STRUCT_OFFSET (BluetoothInputClass, keyboard_appeared),
						   NULL, NULL,
						   g_cclosure_marshal_VOID__VOID,
						   G_TYPE_NONE, 0, G_TYPE_NONE);
	signals[KEYBOARD_DISAPPEARED] = g_signal_new ("keyboard-disappeared",
						   G_TYPE_FROM_CLASS (klass),
						   G_SIGNAL_RUN_LAST,
						   G_STRUCT_OFFSET (BluetoothInputClass, keyboard_disappeared),
						   NULL, NULL,
						   g_cclosure_marshal_VOID__VOID,
						   G_TYPE_NONE, 0, G_TYPE_NONE);
	signals[MOUSE_APPEARED] = g_signal_new ("mouse-appeared",
						G_TYPE_FROM_CLASS (klass),
						G_SIGNAL_RUN_LAST,
						G_STRUCT_OFFSET (BluetoothInputClass, mouse_appeared),
						NULL, NULL,
						g_cclosure_marshal_VOID__VOID,
						G_TYPE_NONE, 0, G_TYPE_NONE);
	signals[MOUSE_DISAPPEARED] = g_signal_new ("mouse-disappeared",
						   G_TYPE_FROM_CLASS (klass),
						   G_SIGNAL_RUN_LAST,
						   G_STRUCT_OFFSET (BluetoothInputClass, mouse_disappeared),
						   NULL, NULL,
						   g_cclosure_marshal_VOID__VOID,
						   G_TYPE_NONE, 0, G_TYPE_NONE);
}

static gboolean
supports_xinput_devices (void)
{
	gint op_code, event, error;

	return XQueryExtension (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()),
				"XInputExtension",
				&op_code,
				&event,
				&error);
}

static gboolean
bluetooth_input_device_get_type (XDeviceInfo *info,
				 gboolean *is_mouse,
				 gboolean *is_keyboard)
{
	*is_mouse = FALSE;
	*is_keyboard = FALSE;

	if (info->num_classes > 0) {
		XAnyClassPtr any;
		guint i;
		any = (XAnyClassPtr) (info->inputclassinfo);
		for (i = 0; i < info->num_classes; i++) {
			switch (any->class) {
			case KeyClass: {
#if 0
				XKeyInfoPtr k;
				k = (XKeyInfoPtr) any;
				printf("\tNum_keys is %d\n", k->num_keys);
#endif
				/* FIXME there should be a better way */
				if (g_str_has_prefix (info->name, "UVC Camera") == FALSE &&
				    g_strcmp0 (info->name, "USB Audio") != 0)
					*is_keyboard = TRUE;
				break;
			}
			case ButtonClass:
				*is_mouse = TRUE;
				break;
			default:
				;;
			}
			any = (XAnyClassPtr) ((char *) any + any->length);
		}
	}

	return (*is_mouse || *is_keyboard);
}

static gboolean
bluetooth_input_ignore_device (const char *name)
{
	guint i;
	const char const *names[] = {
		"Virtual core XTEST pointer",
		"Macintosh mouse button emulation",
		"Virtual core XTEST keyboard",
		"Power Button"
	};

	for (i = 0 ; i < G_N_ELEMENTS (names); i++) {
		if (g_strcmp0 (name, names[i]) == 0)
			return TRUE;
	}
	return FALSE;
}

void
bluetooth_input_check_for_devices (BluetoothInput *input)
{
	XDeviceInfo *device_info;
	gint n_devices;
	guint i;
	gboolean has_keyboard, has_mouse;

	has_keyboard = FALSE;
	has_mouse = FALSE;

	device_info = XListInputDevices (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), &n_devices);
	for (i = 0; i < n_devices; i++) {
		gboolean is_mouse, is_keyboard;
		if (device_info[i].use != IsXExtensionKeyboard &&
		    device_info[i].use != IsXExtensionPointer)
			continue;
		if (bluetooth_input_ignore_device (device_info[i].name) != FALSE)
			continue;
		if (bluetooth_input_device_get_type (&device_info[i], &is_mouse, &is_keyboard) == FALSE)
			continue;
		if (is_mouse != FALSE) {
			g_message ("has mouse: %s (id = %d)", device_info[i].name, device_info[i].id);
			has_mouse = TRUE;
			//break;
		}
		if (is_keyboard != FALSE) {
			g_message ("has keyboard: %s (id = %d)", device_info[i].name, device_info[i].id);
			has_keyboard = TRUE;
			//break;
		}

		//List and shit
	}

	if (device_info != NULL)
		XFreeDeviceList (device_info);
}

static GdkFilterReturn
devicepresence_filter (GdkXEvent *xevent,
		       GdkEvent  *event,
		       BluetoothInput *input)
{
	XEvent *xev = (XEvent *) xevent;
	XEventClass class_presence;
	int xi_presence;

	DevicePresence (gdk_x11_get_default_xdisplay (), xi_presence, class_presence);

	if (xev->type == xi_presence) {
		XDevicePresenceNotifyEvent *dpn = (XDevicePresenceNotifyEvent *) xev;
		if (dpn->devchange == DeviceEnabled || dpn->devchange == DeviceDisabled)
			bluetooth_input_check_for_devices (input);
	}
	return GDK_FILTER_CONTINUE;
}

static void
set_devicepresence_handler (BluetoothInput *input)
{
	Display *display;
	XEventClass class_presence;
	int xi_presence;

	display = gdk_x11_get_default_xdisplay ();

	gdk_error_trap_push ();
	DevicePresence (display, xi_presence, class_presence);
	XSelectExtensionEvent (display,
			       RootWindow (display, DefaultScreen (display)),
			       &class_presence, 1);

	gdk_flush ();
	if (!gdk_error_trap_pop ())
		gdk_window_add_filter (NULL, (GdkFilterFunc) devicepresence_filter, input);
}

static void bluetooth_input_init(BluetoothInput *input)
{
	BluetoothInputPrivate *priv = BLUETOOTH_INPUT_GET_PRIVATE(input);

	set_devicepresence_handler (input);
}

/**
 * bluetooth_input_new:
 *
 * Return value: a reference to the #BluetoothInput singleton or %NULL when XInput is not supported. Unref the object when done.
 **/
BluetoothInput *bluetooth_input_new(void)
{
	static BluetoothInput *bluetooth_input = NULL;

	if (bluetooth_input != NULL)
		return g_object_ref (bluetooth_input);

	if (supports_xinput_devices () == FALSE) {
		g_warning ("XInput not supported, input device helper disabled");
		return NULL;
	}

	bluetooth_input = BLUETOOTH_INPUT (g_object_new (BLUETOOTH_TYPE_INPUT, NULL));
	g_object_add_weak_pointer (G_OBJECT (bluetooth_input),
				   (gpointer) &bluetooth_input);

	return bluetooth_input;
}

