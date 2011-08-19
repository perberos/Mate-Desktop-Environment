/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
 *
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#include <locale.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>

#ifdef HAVE_X11_EXTENSIONS_XINPUT_H
#include <X11/extensions/XInput.h>
#include <X11/extensions/XIproto.h>
#endif
#include <mateconf/mateconf.h>
#include <mateconf/mateconf-client.h>

#include "mate-settings-profile.h"
#include "gsd-mouse-manager.h"

#define GSD_MOUSE_MANAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GSD_TYPE_MOUSE_MANAGER, GsdMouseManagerPrivate))

#define MATECONF_MOUSE_DIR         "/desktop/mate/peripherals/mouse"
#define MATECONF_MOUSE_A11Y_DIR    "/desktop/mate/accessibility/mouse"
#define MATECONF_TOUCHPAD_DIR      "/desktop/mate/peripherals/touchpad"

#define KEY_LEFT_HANDED         MATECONF_MOUSE_DIR "/left_handed"
#define KEY_MOTION_ACCELERATION MATECONF_MOUSE_DIR "/motion_acceleration"
#define KEY_MOTION_THRESHOLD    MATECONF_MOUSE_DIR "/motion_threshold"
#define KEY_LOCATE_POINTER      MATECONF_MOUSE_DIR "/locate_pointer"
#define KEY_DWELL_ENABLE        MATECONF_MOUSE_A11Y_DIR "/dwell_enable"
#define KEY_DELAY_ENABLE        MATECONF_MOUSE_A11Y_DIR "/delay_enable"
#define KEY_TOUCHPAD_DISABLE_W_TYPING    MATECONF_TOUCHPAD_DIR "/disable_while_typing"
#ifdef HAVE_X11_EXTENSIONS_XINPUT_H
#define KEY_TAP_TO_CLICK        MATECONF_TOUCHPAD_DIR "/tap_to_click"
#define KEY_SCROLL_METHOD       MATECONF_TOUCHPAD_DIR "/scroll_method"
#define KEY_PAD_HORIZ_SCROLL    MATECONF_TOUCHPAD_DIR "/horiz_scroll_enabled"
#define KEY_TOUCHPAD_ENABLED    MATECONF_TOUCHPAD_DIR "/touchpad_enabled"
#endif

struct GsdMouseManagerPrivate
{
        guint notify;
        guint notify_a11y;
        guint notify_touchpad;

        gboolean mousetweaks_daemon_running;
        gboolean syndaemon_spawned;
        GPid syndaemon_pid;
	gboolean locate_pointer_spawned;
	GPid locate_pointer_pid;
};

static void     gsd_mouse_manager_class_init  (GsdMouseManagerClass *klass);
static void     gsd_mouse_manager_init        (GsdMouseManager      *mouse_manager);
static void     gsd_mouse_manager_finalize    (GObject             *object);
static void     set_mouse_settings            (GsdMouseManager      *manager);
#ifdef HAVE_X11_EXTENSIONS_XINPUT_H
static int      set_tap_to_click              (gboolean state, gboolean left_handed);
static XDevice* device_is_touchpad            (XDeviceInfo deviceinfo);
#endif

G_DEFINE_TYPE (GsdMouseManager, gsd_mouse_manager, G_TYPE_OBJECT)

static gpointer manager_object = NULL;

static void
gsd_mouse_manager_set_property (GObject        *object,
                               guint           prop_id,
                               const GValue   *value,
                               GParamSpec     *pspec)
{
        GsdMouseManager *self;

        self = GSD_MOUSE_MANAGER (object);

        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gsd_mouse_manager_get_property (GObject        *object,
                               guint           prop_id,
                               GValue         *value,
                               GParamSpec     *pspec)
{
        GsdMouseManager *self;

        self = GSD_MOUSE_MANAGER (object);

        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static GObject *
gsd_mouse_manager_constructor (GType                  type,
                              guint                  n_construct_properties,
                              GObjectConstructParam *construct_properties)
{
        GsdMouseManager      *mouse_manager;
        GsdMouseManagerClass *klass;

        klass = GSD_MOUSE_MANAGER_CLASS (g_type_class_peek (GSD_TYPE_MOUSE_MANAGER));

        mouse_manager = GSD_MOUSE_MANAGER (G_OBJECT_CLASS (gsd_mouse_manager_parent_class)->constructor (type,
                                                                                                      n_construct_properties,
                                                                                                      construct_properties));

        return G_OBJECT (mouse_manager);
}

static void
gsd_mouse_manager_dispose (GObject *object)
{
        GsdMouseManager *mouse_manager;

        mouse_manager = GSD_MOUSE_MANAGER (object);

        G_OBJECT_CLASS (gsd_mouse_manager_parent_class)->dispose (object);
}

static void
gsd_mouse_manager_class_init (GsdMouseManagerClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = gsd_mouse_manager_get_property;
        object_class->set_property = gsd_mouse_manager_set_property;
        object_class->constructor = gsd_mouse_manager_constructor;
        object_class->dispose = gsd_mouse_manager_dispose;
        object_class->finalize = gsd_mouse_manager_finalize;

        g_type_class_add_private (klass, sizeof (GsdMouseManagerPrivate));
}


#ifdef HAVE_X11_EXTENSIONS_XINPUT_H
static gboolean
supports_xinput_devices (void)
{
        gint op_code, event, error;

        return XQueryExtension (GDK_DISPLAY (),
                                "XInputExtension",
                                &op_code,
                                &event,
                                &error);
}
#endif

static void
configure_button_layout (guchar   *buttons,
                         gint      n_buttons,
                         gboolean  left_handed)
{
        const gint left_button = 1;
        gint right_button;
        gint i;

        /* if the button is higher than 2 (3rd button) then it's
         * probably one direction of a scroll wheel or something else
         * uninteresting
         */
        right_button = MIN (n_buttons, 3);

        /* If we change things we need to make sure we only swap buttons.
         * If we end up with multiple physical buttons assigned to the same
         * logical button the server will complain. This code assumes physical
         * button 0 is the physical left mouse button, and that the physical
         * button other than 0 currently assigned left_button or right_button
         * is the physical right mouse button.
         */

        /* check if the current mapping satisfies the above assumptions */
        if (buttons[left_button - 1] != left_button &&
            buttons[left_button - 1] != right_button)
                /* The current mapping is weird. Swapping buttons is probably not a
                 * good idea.
                 */
                return;

        /* check if we are left_handed and currently not swapped */
        if (left_handed && buttons[left_button - 1] == left_button) {
                /* find the right button */
                for (i = 0; i < n_buttons; i++) {
                        if (buttons[i] == right_button) {
                                buttons[i] = left_button;
                                break;
                        }
                }
                /* swap the buttons */
                buttons[left_button - 1] = right_button;
        }
        /* check if we are not left_handed but are swapped */
        else if (!left_handed && buttons[left_button - 1] == right_button) {
                /* find the right button */
                for (i = 0; i < n_buttons; i++) {
                        if (buttons[i] == left_button) {
                                buttons[i] = right_button;
                                break;
                        }
                }
                /* swap the buttons */
                buttons[left_button - 1] = left_button;
        }
}

#ifdef HAVE_X11_EXTENSIONS_XINPUT_H
static gboolean
xinput_device_has_buttons (XDeviceInfo *device_info)
{
        int i;
        XAnyClassInfo *class_info;

        class_info = device_info->inputclassinfo;
        for (i = 0; i < device_info->num_classes; i++) {
                if (class_info->class == ButtonClass) {
                        XButtonInfo *button_info;

                        button_info = (XButtonInfo *) class_info;
                        if (button_info->num_buttons > 0)
                                return TRUE;
                }

                class_info = (XAnyClassInfo *) (((guchar *) class_info) +
                                                class_info->length);
        }
        return FALSE;
}

static gboolean
touchpad_has_single_button (XDevice *device)
{
        Atom type, prop;
        int format;
        unsigned long nitems, bytes_after;
        unsigned char *data;
        gboolean is_single_button = FALSE;
        int rc;

        prop = XInternAtom (GDK_DISPLAY (), "Synaptics Capabilities", False);
        if (!prop)
                return FALSE;

        gdk_error_trap_push ();
        rc = XGetDeviceProperty (GDK_DISPLAY (), device, prop, 0, 1, False,
                                XA_INTEGER, &type, &format, &nitems,
                                &bytes_after, &data);
        if (rc == Success && type == XA_INTEGER && format == 8 && nitems >= 3)
                is_single_button = (data[0] == 1 && data[1] == 0 && data[2] == 0);

        if (rc == Success)
                XFree (data);

        gdk_error_trap_pop ();

        return is_single_button;
}


static void
set_xinput_devices_left_handed (gboolean left_handed)
{
        XDeviceInfo *device_info;
        gint n_devices;
        guchar *buttons;
        gsize buttons_capacity = 16;
        gint n_buttons;
        gint i;

        device_info = XListInputDevices (GDK_DISPLAY (), &n_devices);

        if (n_devices > 0)
                buttons = g_new (guchar, buttons_capacity);
        else
                buttons = NULL;

        for (i = 0; i < n_devices; i++) {
                XDevice *device = NULL;

                if ((device_info[i].use == IsXPointer) ||
                    (device_info[i].use == IsXKeyboard) ||
                    (!xinput_device_has_buttons (&device_info[i])))
                        continue;

                /* If the device is a touchpad, swap tap buttons
                 * around too, otherwise a tap would be a right-click */
                device = device_is_touchpad (device_info[i]);
                if (device != NULL) {
                        MateConfClient *client = mateconf_client_get_default ();
                        gboolean tap = mateconf_client_get_bool (client, KEY_TAP_TO_CLICK, NULL);
                        gboolean single_button = touchpad_has_single_button (device);

                        if (tap && !single_button)
                                set_tap_to_click (tap, left_handed);
                        XCloseDevice (GDK_DISPLAY (), device);
                        g_object_unref (client);

                        if (single_button)
                            continue;
                }

                gdk_error_trap_push ();

                device = XOpenDevice (GDK_DISPLAY (), device_info[i].id);

                if ((gdk_error_trap_pop () != 0) ||
                    (device == NULL))
                        continue;

                n_buttons = XGetDeviceButtonMapping (GDK_DISPLAY (), device,
                                                     buttons,
                                                     buttons_capacity);

                while (n_buttons > buttons_capacity) {
                        buttons_capacity = n_buttons;
                        buttons = (guchar *) g_realloc (buttons,
                                                        buttons_capacity * sizeof (guchar));

                        n_buttons = XGetDeviceButtonMapping (GDK_DISPLAY (), device,
                                                             buttons,
                                                             buttons_capacity);
                }

                configure_button_layout (buttons, n_buttons, left_handed);

                XSetDeviceButtonMapping (GDK_DISPLAY (), device, buttons, n_buttons);
                XCloseDevice (GDK_DISPLAY (), device);
        }
        g_free (buttons);

        if (device_info != NULL)
                XFreeDeviceList (device_info);
}

static GdkFilterReturn
devicepresence_filter (GdkXEvent *xevent,
                       GdkEvent  *event,
                       gpointer   data)
{
        XEvent *xev = (XEvent *) xevent;
        XEventClass class_presence;
        int xi_presence;

        DevicePresence (gdk_x11_get_default_xdisplay (), xi_presence, class_presence);

        if (xev->type == xi_presence)
        {
            XDevicePresenceNotifyEvent *dpn = (XDevicePresenceNotifyEvent *) xev;
            if (dpn->devchange == DeviceEnabled)
                set_mouse_settings ((GsdMouseManager *) data);
        }
        return GDK_FILTER_CONTINUE;
}

static void
set_devicepresence_handler (GsdMouseManager *manager)
{
        Display *display;
        XEventClass class_presence;
        int xi_presence;

        if (!supports_xinput_devices ())
                return;

        display = gdk_x11_get_default_xdisplay ();

        gdk_error_trap_push ();
        DevicePresence (display, xi_presence, class_presence);
        XSelectExtensionEvent (display,
                               RootWindow (display, DefaultScreen (display)),
                               &class_presence, 1);

        gdk_flush ();
        if (!gdk_error_trap_pop ())
                gdk_window_add_filter (NULL, devicepresence_filter, manager);
}
#endif

static void
set_left_handed (GsdMouseManager *manager,
                 gboolean         left_handed)
{
        guchar *buttons ;
        gsize buttons_capacity = 16;
        gint n_buttons, i;

#ifdef HAVE_X11_EXTENSIONS_XINPUT_H
        if (supports_xinput_devices ()) {
                /* When XInput support is available, never set the
                 * button ordering on the core pointer as that would
                 * revert the changes we make on the devices themselves */
                set_xinput_devices_left_handed (left_handed);
                return;
        }
#endif

        buttons = g_new (guchar, buttons_capacity);
        n_buttons = XGetPointerMapping (GDK_DISPLAY (),
                                        buttons,
                                        (gint) buttons_capacity);
        while (n_buttons > buttons_capacity) {
                buttons_capacity = n_buttons;
                buttons = (guchar *) g_realloc (buttons,
                                                buttons_capacity * sizeof (guchar));

                n_buttons = XGetPointerMapping (GDK_DISPLAY (),
                                                buttons,
                                                (gint) buttons_capacity);
        }

        configure_button_layout (buttons, n_buttons, left_handed);

        /* X refuses to change the mapping while buttons are engaged,
         * so if this is the case we'll retry a few times
         */
        for (i = 0;
             i < 20 && XSetPointerMapping (GDK_DISPLAY (), buttons, n_buttons) == MappingBusy;
             ++i) {
                g_usleep (300);
        }

        g_free (buttons);
}

static void
set_motion_acceleration (GsdMouseManager *manager,
                         gfloat           motion_acceleration)
{
        gint numerator, denominator;

        if (motion_acceleration >= 1.0) {
                /* we want to get the acceleration, with a resolution of 0.5
                 */
                if ((motion_acceleration - floor (motion_acceleration)) < 0.25) {
                        numerator = floor (motion_acceleration);
                        denominator = 1;
                } else if ((motion_acceleration - floor (motion_acceleration)) < 0.5) {
                        numerator = ceil (2.0 * motion_acceleration);
                        denominator = 2;
                } else if ((motion_acceleration - floor (motion_acceleration)) < 0.75) {
                        numerator = floor (2.0 *motion_acceleration);
                        denominator = 2;
                } else {
                        numerator = ceil (motion_acceleration);
                        denominator = 1;
                }
        } else if (motion_acceleration < 1.0 && motion_acceleration > 0) {
                /* This we do to 1/10ths */
                numerator = floor (motion_acceleration * 10) + 1;
                denominator= 10;
        } else {
                numerator = -1;
                denominator = -1;
        }

        XChangePointerControl (GDK_DISPLAY (), True, False,
                               numerator, denominator,
                               0);
}

static void
set_motion_threshold (GsdMouseManager *manager,
                      int              motion_threshold)
{
        XChangePointerControl (GDK_DISPLAY (), False, True,
                               0, 0, motion_threshold);
}

#ifdef HAVE_X11_EXTENSIONS_XINPUT_H
static XDevice*
device_is_touchpad (XDeviceInfo deviceinfo)
{
        XDevice *device;
        Atom realtype, prop;
        int realformat;
        unsigned long nitems, bytes_after;
        unsigned char *data;

        if (deviceinfo.type != XInternAtom (GDK_DISPLAY (), XI_TOUCHPAD, False))
                return NULL;

        prop = XInternAtom (GDK_DISPLAY (), "Synaptics Off", False);
        if (!prop)
                return NULL;

        gdk_error_trap_push ();
        device = XOpenDevice (GDK_DISPLAY (), deviceinfo.id);
        if (gdk_error_trap_pop () || (device == NULL))
                return NULL;

        gdk_error_trap_push ();
        if ((XGetDeviceProperty (GDK_DISPLAY (), device, prop, 0, 1, False,
                                XA_INTEGER, &realtype, &realformat, &nitems,
                                &bytes_after, &data) == Success) && (realtype != None)) {
                gdk_error_trap_pop ();
                XFree (data);
                return device;
        }
        gdk_error_trap_pop ();

        XCloseDevice (GDK_DISPLAY (), device);
        return NULL;
}
#endif

static int
set_disable_w_typing (GsdMouseManager *manager, gboolean state)
{

        if (state) {
                GError *error = NULL;
                char *args[5];

                if (manager->priv->syndaemon_spawned)
                        return 0;

                args[0] = "syndaemon";
                args[1] = "-i";
                args[2] = "0.5";
                args[3] = "-k";
                args[4] = NULL;

                if (!g_find_program_in_path (args[0]))
                        return 0;

                g_spawn_async (g_get_home_dir (), args, NULL,
                                G_SPAWN_SEARCH_PATH, NULL, NULL,
                                &manager->priv->syndaemon_pid, &error);

                manager->priv->syndaemon_spawned = (error == NULL);

                if (error) {
                        MateConfClient *client;
                        client = mateconf_client_get_default ();
                        mateconf_client_set_bool (client, KEY_TOUCHPAD_DISABLE_W_TYPING, FALSE, NULL);
                        g_object_unref (client);
                        g_error_free (error);
                }

        } else if (manager->priv->syndaemon_spawned)
        {
                kill (manager->priv->syndaemon_pid, SIGHUP);
                g_spawn_close_pid (manager->priv->syndaemon_pid);
                manager->priv->syndaemon_spawned = FALSE;
        }

        return 0;
}

#ifdef HAVE_X11_EXTENSIONS_XINPUT_H
static int
set_tap_to_click (gboolean state, gboolean left_handed)
{
        int numdevices, i, format, rc;
        unsigned long nitems, bytes_after;
        XDeviceInfo *devicelist = XListInputDevices (GDK_DISPLAY (), &numdevices);
        XDevice * device;
        unsigned char* data;
        Atom prop, type;

        if (devicelist == NULL)
                return 0;

        prop = XInternAtom (GDK_DISPLAY (), "Synaptics Tap Action", False);

        if (!prop)
                return 0;

        for (i = 0; i < numdevices; i++) {
                if ((device = device_is_touchpad (devicelist[i]))) {
                        gdk_error_trap_push ();
                        rc = XGetDeviceProperty (GDK_DISPLAY (), device, prop, 0, 2,
                                                False, XA_INTEGER, &type, &format, &nitems,
                                                &bytes_after, &data);

                        if (rc == Success && type == XA_INTEGER && format == 8 && nitems >= 7)
                        {
                                /* Set RLM mapping for 1/2/3 fingers*/
                                data[4] = (state) ? ((left_handed) ? 3 : 1) : 0;
                                data[5] = (state) ? ((left_handed) ? 1 : 3) : 0;
                                data[6] = (state) ? 2 : 0;
                                XChangeDeviceProperty (GDK_DISPLAY (), device, prop, XA_INTEGER, 8,
                                                        PropModeReplace, data, nitems);
                        }

                        if (rc == Success)
                                XFree (data);
                        XCloseDevice (GDK_DISPLAY (), device);
                        if (gdk_error_trap_pop ()) {
                                g_warning ("Error in setting tap to click on \"%s\"", devicelist[i].name);
                                continue;
                        }
                }
        }

        XFreeDeviceList (devicelist);
        return 0;
}

static int
set_horiz_scroll (gboolean state)
{
        int numdevices, i, rc;
        XDeviceInfo *devicelist = XListInputDevices (GDK_DISPLAY (), &numdevices);
        XDevice *device;
        Atom act_type, prop_edge, prop_twofinger;
        int act_format;
        unsigned long nitems, bytes_after;
        unsigned char *data;

        if (devicelist == NULL)
                return 0;

        prop_edge = XInternAtom (GDK_DISPLAY (), "Synaptics Edge Scrolling", False);
        prop_twofinger = XInternAtom (GDK_DISPLAY (), "Synaptics Two-Finger Scrolling", False);

        if (!prop_edge || !prop_twofinger)
                return 0;

        for (i = 0; i < numdevices; i++) {
                if ((device = device_is_touchpad (devicelist[i]))) {
                        gdk_error_trap_push ();
                        rc = XGetDeviceProperty (GDK_DISPLAY (), device,
                                                prop_edge, 0, 1, False,
                                                XA_INTEGER, &act_type, &act_format, &nitems,
                                                &bytes_after, &data);
                        if (rc == Success && act_type == XA_INTEGER &&
                                act_format == 8 && nitems >= 2) {
                                data[1] = (state && data[0]);
                                XChangeDeviceProperty (GDK_DISPLAY (), device,
                                                        prop_edge, XA_INTEGER, 8,
                                                        PropModeReplace, data, nitems);
                        }

                        XFree (data);

                        rc = XGetDeviceProperty (GDK_DISPLAY (), device,
                                                prop_twofinger, 0, 1, False,
                                                XA_INTEGER, &act_type, &act_format, &nitems,
                                                &bytes_after, &data);
                        if (rc == Success && act_type == XA_INTEGER &&
                                act_format == 8 && nitems >= 2) {
                                data[1] = (state && data[0]);
                                XChangeDeviceProperty (GDK_DISPLAY (), device,
                                                        prop_twofinger, XA_INTEGER, 8,
                                                        PropModeReplace, data, nitems);
                        }

                        XFree (data);
                        XCloseDevice (GDK_DISPLAY (), device);
                        if (gdk_error_trap_pop ()) {
                                g_warning ("Error in setting horiz scroll on \"%s\"", devicelist[i].name);
                                continue;
                        }
                }
        }

        XFreeDeviceList (devicelist);
        return 0;
}


/**
 * Scroll methods are: 0 - disabled, 1 - edge scrolling, 2 - twofinger
 * scrolling
 */
static int
set_edge_scroll (int method)
{
        int numdevices, i, rc;
        XDeviceInfo *devicelist = XListInputDevices (GDK_DISPLAY (), &numdevices);
        XDevice *device;
        Atom act_type, prop_edge, prop_twofinger;
        int act_format;
        unsigned long nitems, bytes_after;
        unsigned char *data;

        if (devicelist == NULL)
                return 0;

        prop_edge = XInternAtom (GDK_DISPLAY (), "Synaptics Edge Scrolling", False);
        prop_twofinger = XInternAtom (GDK_DISPLAY (), "Synaptics Two-Finger Scrolling", False);

        if (!prop_edge || !prop_twofinger)
                return 0;

        for (i = 0; i < numdevices; i++) {
                if ((device = device_is_touchpad (devicelist[i]))) {
                        gdk_error_trap_push ();
                        rc = XGetDeviceProperty (GDK_DISPLAY (), device,
                                                prop_edge, 0, 1, False,
                                                XA_INTEGER, &act_type, &act_format, &nitems,
                                                &bytes_after, &data);
                        if (rc == Success && act_type == XA_INTEGER &&
                                act_format == 8 && nitems >= 2) {
                                data[0] = (method == 1) ? 1 : 0;
                                XChangeDeviceProperty (GDK_DISPLAY (), device,
                                                        prop_edge, XA_INTEGER, 8,
                                                        PropModeReplace, data, nitems);
                        }

                        XFree (data);

                        rc = XGetDeviceProperty (GDK_DISPLAY (), device,
                                                prop_twofinger, 0, 1, False,
                                                XA_INTEGER, &act_type, &act_format, &nitems,
                                                &bytes_after, &data);
                        if (rc == Success && act_type == XA_INTEGER &&
                                act_format == 8 && nitems >= 2) {
                                data[0] = (method == 2) ? 1 : 0;
                                XChangeDeviceProperty (GDK_DISPLAY (), device,
                                                        prop_twofinger, XA_INTEGER, 8,
                                                        PropModeReplace, data, nitems);
                        }

                        XFree (data);
                        XCloseDevice (GDK_DISPLAY (), device);
                        if (gdk_error_trap_pop ()) {
                                g_warning ("Error in setting edge scroll on \"%s\"", devicelist[i].name);
                                continue;
                        }
                }
        }

        XFreeDeviceList (devicelist);
        return 0;
}

static int
set_touchpad_enabled (gboolean state)
{
        int numdevices, i;
        XDeviceInfo *devicelist = XListInputDevices (GDK_DISPLAY (), &numdevices);
        XDevice *device;
        Atom prop_enabled;

        if (devicelist == NULL)
                return 0;

        prop_enabled = XInternAtom (GDK_DISPLAY (), "Device Enabled", False);

        if (!prop_enabled)
            return 0;

        for (i = 0; i < numdevices; i++) {
                if ((device = device_is_touchpad (devicelist[i]))) {
                        unsigned char data = state;
                        gdk_error_trap_push ();
                        XChangeDeviceProperty (GDK_DISPLAY (), device,
                                               prop_enabled, XA_INTEGER, 8,
                                               PropModeReplace, &data, 1);
                        XCloseDevice (GDK_DISPLAY (), device);
                        gdk_flush ();
                        if (gdk_error_trap_pop ()) {
                                g_warning ("Error %s device \"%s\"",
                                           (state) ? "enabling" : "disabling",
                                           devicelist[i].name);
                                continue;
                        }
                }
        }

        XFreeDeviceList (devicelist);
        return 0;
}
#endif

static void
set_locate_pointer (GsdMouseManager *manager,
                    gboolean         state)
{
        if (state) {
                GError *error = NULL;
                char *args[2];

                if (manager->priv->locate_pointer_spawned)
                        return;

                args[0] = LIBEXECDIR "/gsd-locate-pointer";
                args[1] = NULL;

                g_spawn_async (NULL, args, NULL,
                               0, NULL, NULL,
                               &manager->priv->locate_pointer_pid, &error);

                manager->priv->locate_pointer_spawned = (error == NULL);

                if (error) {
                        MateConfClient *client;
                        client = mateconf_client_get_default ();
                        mateconf_client_set_bool (client, KEY_LOCATE_POINTER, FALSE, NULL);
                        g_object_unref (client);
                        g_error_free (error);
                }

        }
	else if (manager->priv->locate_pointer_spawned) {
                kill (manager->priv->locate_pointer_pid, SIGHUP);
                g_spawn_close_pid (manager->priv->locate_pointer_pid);
                manager->priv->locate_pointer_spawned = FALSE;
        }
}

static void
set_mousetweaks_daemon (GsdMouseManager *manager,
                        gboolean         dwell_enable,
                        gboolean         delay_enable)
{
        GError *error = NULL;
        gchar *comm;
        gboolean run_daemon = dwell_enable || delay_enable;

        if (run_daemon || manager->priv->mousetweaks_daemon_running)
                comm = g_strdup_printf ("mousetweaks %s",
                                        run_daemon ? "" : "-s");
        else
                return;

        if (run_daemon)
                manager->priv->mousetweaks_daemon_running = TRUE;


        if (! g_spawn_command_line_async (comm, &error)) {
                if (error->code == G_SPAWN_ERROR_NOENT &&
                    (dwell_enable || delay_enable)) {
                        GtkWidget *dialog;
                        MateConfClient *client;

                        client = mateconf_client_get_default ();
                        if (dwell_enable)
                                mateconf_client_set_bool (client,
                                                       KEY_DWELL_ENABLE,
                                                       FALSE, NULL);
                        else if (delay_enable)
                                mateconf_client_set_bool (client,
                                                       KEY_DELAY_ENABLE,
                                                       FALSE, NULL);
                        g_object_unref (client);

                        dialog = gtk_message_dialog_new (NULL, 0,
                                                         GTK_MESSAGE_WARNING,
                                                         GTK_BUTTONS_OK,
                                                         _("Could not enable mouse accessibility features"));
                        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                                                  _("Mouse accessibility requires Mousetweaks "
                                                                    "to be installed on your system."));
                        gtk_window_set_title (GTK_WINDOW (dialog),
                                              _("Mouse Preferences"));
                        gtk_window_set_icon_name (GTK_WINDOW (dialog),
                                                  "input-mouse");
                        gtk_dialog_run (GTK_DIALOG (dialog));
                        gtk_widget_destroy (dialog);
                }
                g_error_free (error);
        }
        g_free (comm);
}

static void
set_mouse_settings (GsdMouseManager *manager)
{
        MateConfClient *client = mateconf_client_get_default ();
        gboolean left_handed = mateconf_client_get_bool (client, KEY_LEFT_HANDED, NULL);

        set_left_handed (manager, left_handed);
        set_motion_acceleration (manager, mateconf_client_get_float (client, KEY_MOTION_ACCELERATION , NULL));
        set_motion_threshold (manager, mateconf_client_get_int (client, KEY_MOTION_THRESHOLD, NULL));

        set_disable_w_typing (manager, mateconf_client_get_bool (client, KEY_TOUCHPAD_DISABLE_W_TYPING, NULL));
#ifdef HAVE_X11_EXTENSIONS_XINPUT_H
        set_tap_to_click (mateconf_client_get_bool (client, KEY_TAP_TO_CLICK, NULL), left_handed);
        set_edge_scroll (mateconf_client_get_int (client, KEY_SCROLL_METHOD, NULL));
        set_horiz_scroll (mateconf_client_get_bool (client, KEY_PAD_HORIZ_SCROLL, NULL));
        set_touchpad_enabled (mateconf_client_get_bool (client, KEY_TOUCHPAD_ENABLED, NULL));
#endif

        g_object_unref (client);
}

static void
mouse_callback (MateConfClient        *client,
                guint               cnxn_id,
                MateConfEntry         *entry,
                GsdMouseManager    *manager)
{
        if (! strcmp (entry->key, KEY_LEFT_HANDED)) {
                if (entry->value->type == MATECONF_VALUE_BOOL) {
                        set_left_handed (manager, mateconf_value_get_bool (entry->value));
                }
        } else if (! strcmp (entry->key, KEY_MOTION_ACCELERATION)) {
                if (entry->value->type == MATECONF_VALUE_FLOAT) {
                        set_motion_acceleration (manager, mateconf_value_get_float (entry->value));
                }
        } else if (! strcmp (entry->key, KEY_MOTION_THRESHOLD)) {
                if (entry->value->type == MATECONF_VALUE_INT) {
                        set_motion_threshold (manager, mateconf_value_get_int (entry->value));
                }
        } else if (! strcmp (entry->key, KEY_TOUCHPAD_DISABLE_W_TYPING)) {
                if (entry->value->type == MATECONF_VALUE_BOOL)
                        set_disable_w_typing (manager, mateconf_value_get_bool (entry->value));
#ifdef HAVE_X11_EXTENSIONS_XINPUT_H
        } else if (! strcmp (entry->key, KEY_TAP_TO_CLICK)) {
                if (entry->value->type == MATECONF_VALUE_BOOL) {
                        set_tap_to_click (mateconf_value_get_bool (entry->value),
                                          mateconf_client_get_bool (client, KEY_LEFT_HANDED, NULL));
                }
        } else if (! strcmp (entry->key, KEY_SCROLL_METHOD)) {
                if (entry->value->type == MATECONF_VALUE_INT) {
                        set_edge_scroll (mateconf_value_get_int (entry->value));
                        set_horiz_scroll (mateconf_client_get_bool (client, KEY_PAD_HORIZ_SCROLL, NULL));
                }
        } else if (! strcmp (entry->key, KEY_PAD_HORIZ_SCROLL)) {
                if (entry->value->type == MATECONF_VALUE_BOOL)
                        set_horiz_scroll (mateconf_value_get_bool (entry->value));
#endif
        } else if (! strcmp (entry->key, KEY_LOCATE_POINTER)) {
                if (entry->value->type == MATECONF_VALUE_BOOL) {
                        set_locate_pointer (manager, mateconf_value_get_bool (entry->value));
                }
#ifdef HAVE_X11_EXTENSIONS_XINPUT_H
        } else if (! strcmp (entry->key, KEY_TOUCHPAD_ENABLED)) {
                if (entry->value->type == MATECONF_VALUE_BOOL) {
                    set_touchpad_enabled (mateconf_value_get_bool (entry->value));
                }
#endif
        } else if (! strcmp (entry->key, KEY_DWELL_ENABLE)) {
                if (entry->value->type == MATECONF_VALUE_BOOL) {
                        set_mousetweaks_daemon (manager,
                                                mateconf_value_get_bool (entry->value),
                                                mateconf_client_get_bool (client, KEY_DELAY_ENABLE, NULL));
                }
        } else if (! strcmp (entry->key, KEY_DELAY_ENABLE)) {
                if (entry->value->type == MATECONF_VALUE_BOOL) {
                        set_mousetweaks_daemon (manager,
                                                mateconf_client_get_bool (client, KEY_DWELL_ENABLE, NULL),
                                                mateconf_value_get_bool (entry->value));
                }
        }
}

static guint
register_config_callback (GsdMouseManager         *manager,
                          MateConfClient             *client,
                          const char              *path,
                          MateConfClientNotifyFunc    func)
{
        mateconf_client_add_dir (client, path, MATECONF_CLIENT_PRELOAD_ONELEVEL, NULL);
        return mateconf_client_notify_add (client, path, func, manager, NULL, NULL);
}

static void
gsd_mouse_manager_init (GsdMouseManager *manager)
{
        manager->priv = GSD_MOUSE_MANAGER_GET_PRIVATE (manager);
}

static gboolean
gsd_mouse_manager_idle_cb (GsdMouseManager *manager)
{
        MateConfClient *client;

        mate_settings_profile_start (NULL);

        client = mateconf_client_get_default ();

        manager->priv->notify =
                register_config_callback (manager,
                                          client,
                                          MATECONF_MOUSE_DIR,
                                          (MateConfClientNotifyFunc) mouse_callback);
        manager->priv->notify_a11y =
                register_config_callback (manager,
                                          client,
                                          MATECONF_MOUSE_A11Y_DIR,
                                          (MateConfClientNotifyFunc) mouse_callback);
        manager->priv->notify_touchpad =
                register_config_callback (manager,
                                          client,
                                          MATECONF_TOUCHPAD_DIR,
                                          (MateConfClientNotifyFunc) mouse_callback);
        manager->priv->syndaemon_spawned = FALSE;

#ifdef HAVE_X11_EXTENSIONS_XINPUT_H
        set_devicepresence_handler (manager);
#endif
        set_mouse_settings (manager);
        set_locate_pointer (manager, mateconf_client_get_bool (client, KEY_LOCATE_POINTER, NULL));
        set_mousetweaks_daemon (manager,
                                mateconf_client_get_bool (client, KEY_DWELL_ENABLE, NULL),
                                mateconf_client_get_bool (client, KEY_DELAY_ENABLE, NULL));

        set_disable_w_typing (manager, mateconf_client_get_bool (client, KEY_TOUCHPAD_DISABLE_W_TYPING, NULL));
#ifdef HAVE_X11_EXTENSIONS_XINPUT_H
        set_tap_to_click (mateconf_client_get_bool (client, KEY_TAP_TO_CLICK, NULL),
                          mateconf_client_get_bool (client, KEY_LEFT_HANDED, NULL));
        set_edge_scroll (mateconf_client_get_int (client, KEY_SCROLL_METHOD, NULL));
        set_horiz_scroll (mateconf_client_get_bool (client, KEY_PAD_HORIZ_SCROLL, NULL));
        set_touchpad_enabled (mateconf_client_get_bool (client, KEY_TOUCHPAD_ENABLED, NULL));
#endif

        g_object_unref (client);

        mate_settings_profile_end (NULL);

        return FALSE;
}

gboolean
gsd_mouse_manager_start (GsdMouseManager *manager,
                         GError         **error)
{
        mate_settings_profile_start (NULL);

        g_idle_add ((GSourceFunc) gsd_mouse_manager_idle_cb, manager);

        mate_settings_profile_end (NULL);

        return TRUE;
}

void
gsd_mouse_manager_stop (GsdMouseManager *manager)
{
        GsdMouseManagerPrivate *p = manager->priv;
        MateConfClient *client;

        g_debug ("Stopping mouse manager");

        client = mateconf_client_get_default ();

        if (p->notify != 0) {
                mateconf_client_remove_dir (client, MATECONF_MOUSE_DIR, NULL);
                mateconf_client_notify_remove (client, p->notify);
                p->notify = 0;
        }

        if (p->notify_a11y != 0) {
                mateconf_client_remove_dir (client, MATECONF_MOUSE_A11Y_DIR, NULL);
                mateconf_client_notify_remove (client, p->notify_a11y);
                p->notify_a11y = 0;
        }

        if (p->notify_touchpad != 0) {
                mateconf_client_remove_dir (client, MATECONF_TOUCHPAD_DIR, NULL);
                mateconf_client_notify_remove (client, p->notify_touchpad);
                p->notify_touchpad = 0;
        }

        g_object_unref (client);

        set_locate_pointer (manager, FALSE);

#ifdef HAVE_X11_EXTENSIONS_XINPUT_H
        gdk_window_remove_filter (NULL, devicepresence_filter, manager);
#endif
}

static void
gsd_mouse_manager_finalize (GObject *object)
{
        GsdMouseManager *mouse_manager;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GSD_IS_MOUSE_MANAGER (object));

        mouse_manager = GSD_MOUSE_MANAGER (object);

        g_return_if_fail (mouse_manager->priv != NULL);

        G_OBJECT_CLASS (gsd_mouse_manager_parent_class)->finalize (object);
}

GsdMouseManager *
gsd_mouse_manager_new (void)
{
        if (manager_object != NULL) {
                g_object_ref (manager_object);
        } else {
                manager_object = g_object_new (GSD_TYPE_MOUSE_MANAGER, NULL);
                g_object_add_weak_pointer (manager_object,
                                           (gpointer *) &manager_object);
        }

        return GSD_MOUSE_MANAGER (manager_object);
}
