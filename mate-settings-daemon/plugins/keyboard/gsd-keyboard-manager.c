/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright © 2001 Ximian, Inc.
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

#include <locale.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#ifdef HAVE_X11_EXTENSIONS_XF86MISC_H
#  include <X11/extensions/xf86misc.h>
#endif
#ifdef HAVE_X11_EXTENSIONS_XKB_H
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#endif

#include "mate-settings-profile.h"
#include "gsd-keyboard-manager.h"

#include "gsd-keyboard-xkb.h"
#include "gsd-xmodmap.h"

#define GSD_KEYBOARD_MANAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GSD_TYPE_KEYBOARD_MANAGER, GsdKeyboardManagerPrivate))

#ifndef HOST_NAME_MAX
#  define HOST_NAME_MAX 255
#endif

#define GSD_KEYBOARD_KEY "/desktop/mate/peripherals/keyboard"

#define KEY_REPEAT        GSD_KEYBOARD_KEY "/repeat"
#define KEY_CLICK         GSD_KEYBOARD_KEY "/click"
#define KEY_RATE          GSD_KEYBOARD_KEY "/rate"
#define KEY_DELAY         GSD_KEYBOARD_KEY "/delay"
#define KEY_CLICK_VOLUME  GSD_KEYBOARD_KEY "/click_volume"

#define KEY_BELL_VOLUME   GSD_KEYBOARD_KEY "/bell_volume"
#define KEY_BELL_PITCH    GSD_KEYBOARD_KEY "/bell_pitch"
#define KEY_BELL_DURATION GSD_KEYBOARD_KEY "/bell_duration"
#define KEY_BELL_MODE     GSD_KEYBOARD_KEY "/bell_mode"

struct GsdKeyboardManagerPrivate
{
        gboolean have_xkb;
        gint     xkb_event_base;
        guint    notify;
};

static void     gsd_keyboard_manager_class_init  (GsdKeyboardManagerClass *klass);
static void     gsd_keyboard_manager_init        (GsdKeyboardManager      *keyboard_manager);
static void     gsd_keyboard_manager_finalize    (GObject                 *object);

G_DEFINE_TYPE (GsdKeyboardManager, gsd_keyboard_manager, G_TYPE_OBJECT)

static gpointer manager_object = NULL;


#ifdef HAVE_X11_EXTENSIONS_XF86MISC_H
static gboolean
xfree86_set_keyboard_autorepeat_rate (int delay, int rate)
{
        gboolean res = FALSE;
        int      event_base_return;
        int      error_base_return;

        if (XF86MiscQueryExtension (GDK_DISPLAY (),
                                    &event_base_return,
                                    &error_base_return) == True) {
                /* load the current settings */
                XF86MiscKbdSettings kbdsettings;
                XF86MiscGetKbdSettings (GDK_DISPLAY (), &kbdsettings);

                /* assign the new values */
                kbdsettings.delay = delay;
                kbdsettings.rate = rate;
                XF86MiscSetKbdSettings (GDK_DISPLAY (), &kbdsettings);
                res = TRUE;
        }

        return res;
}
#endif /* HAVE_X11_EXTENSIONS_XF86MISC_H */

#ifdef HAVE_X11_EXTENSIONS_XKB_H
static gboolean
xkb_set_keyboard_autorepeat_rate (int delay, int rate)
{
        int interval = (rate <= 0) ? 1000000 : 1000/rate;
        if (delay <= 0)
                delay = 1;
        return XkbSetAutoRepeatRate (GDK_DISPLAY (),
                                     XkbUseCoreKbd,
                                     delay,
                                     interval);
}
#endif

static char *
gsd_keyboard_get_hostname_key (const char *subkey)
{
        char hostname[HOST_NAME_MAX + 1];

        if (gethostname (hostname, sizeof (hostname)) == 0 &&
            strcmp (hostname, "localhost") != 0 &&
            strcmp (hostname, "localhost.localdomain") != 0) {
                char *escaped;
                char *key;

                escaped = mateconf_escape_key (hostname, -1);
                key = g_strconcat (GSD_KEYBOARD_KEY
                                   "/host-",
                                   escaped,
                                   "/0/",
                                   subkey,
                                   NULL);
                g_free (escaped);
                return key;
        } else
                return NULL;
}

#ifdef HAVE_X11_EXTENSIONS_XKB_H

typedef enum {
        NUMLOCK_STATE_OFF = 0,
        NUMLOCK_STATE_ON = 1,
        NUMLOCK_STATE_UNKNOWN = 2
} NumLockState;

static void
numlock_xkb_init (GsdKeyboardManager *manager)
{
        Display *dpy = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());
        gboolean have_xkb;
        int opcode, error_base, major, minor;

        have_xkb = XkbQueryExtension (dpy,
                                      &opcode,
                                      &manager->priv->xkb_event_base,
                                      &error_base,
                                      &major,
                                      &minor)
                && XkbUseExtension (dpy, &major, &minor);

        if (have_xkb) {
                XkbSelectEventDetails (dpy,
                                       XkbUseCoreKbd,
                                       XkbStateNotifyMask,
                                       XkbModifierLockMask,
                                       XkbModifierLockMask);
        } else {
                g_warning ("XKB extension not available");
        }

        manager->priv->have_xkb = have_xkb;
}

static unsigned
numlock_NumLock_modifier_mask (void)
{
        Display *dpy = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());
        return XkbKeysymToModifiers (dpy, XK_Num_Lock);
}

static void
numlock_set_xkb_state (NumLockState new_state)
{
        unsigned int num_mask;
        Display *dpy = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());
        if (new_state != NUMLOCK_STATE_ON && new_state != NUMLOCK_STATE_OFF)
                return;
        num_mask = numlock_NumLock_modifier_mask ();
        XkbLockModifiers (dpy, XkbUseCoreKbd, num_mask, new_state ? num_mask : 0);
}

static char *
numlock_mateconf_state_key (void)
{
        char *key = gsd_keyboard_get_hostname_key ("numlock_on");
        if (!key) {
                g_message ("NumLock remembering disabled because hostname is set to \"localhost\"");
        }
        return key;
}

static NumLockState
numlock_get_mateconf_state (MateConfClient *client)
{
        int          curr_state;
        GError      *err = NULL;
        char        *key = numlock_mateconf_state_key ();

        if (!key) {
                return NUMLOCK_STATE_UNKNOWN;
        }

        curr_state = mateconf_client_get_bool (client, key, &err);
        if (err) {
                curr_state = NUMLOCK_STATE_UNKNOWN;
                g_error_free (err);
        }

        g_free (key);
        return curr_state;
}

static void
numlock_set_mateconf_state (MateConfClient *client,
                         NumLockState new_state)
{
        char *key;

        if (new_state != NUMLOCK_STATE_ON && new_state != NUMLOCK_STATE_OFF) {
                return;
        }

        key = numlock_mateconf_state_key ();
        if (key) {
                mateconf_client_set_bool (client, key, new_state, NULL);
                g_free (key);
        }
}

static GdkFilterReturn
numlock_xkb_callback (GdkXEvent *xev_,
                      GdkEvent *gdkev_,
                      gpointer xkb_event_code)
{
        XEvent *xev = (XEvent *) xev_;

        if (xev->type == GPOINTER_TO_INT (xkb_event_code)) {
                XkbEvent *xkbev = (XkbEvent *)xev;
                if (xkbev->any.xkb_type == XkbStateNotify)
                if (xkbev->state.changed & XkbModifierLockMask) {
                        unsigned num_mask = numlock_NumLock_modifier_mask ();
                        unsigned locked_mods = xkbev->state.locked_mods;
                        int numlock_state = !! (num_mask & locked_mods);
                        MateConfClient *client = mateconf_client_get_default ();
                        numlock_set_mateconf_state (client, numlock_state);
                        g_object_unref (client);
                }
        }
        return GDK_FILTER_CONTINUE;
}

static void
numlock_install_xkb_callback (GsdKeyboardManager *manager)
{
        if (!manager->priv->have_xkb)
                return;

        gdk_window_add_filter (NULL,
                               numlock_xkb_callback,
                               GINT_TO_POINTER (manager->priv->xkb_event_base));
}

#endif /* HAVE_X11_EXTENSIONS_XKB_H */

static void
apply_settings (MateConfClient        *client,
                guint               cnxn_id,
                MateConfEntry         *entry,
                GsdKeyboardManager *manager)
{
        XKeyboardControl kbdcontrol;
        gboolean         repeat;
        gboolean         click;
        int              rate;
        int              delay;
        int              click_volume;
        int              bell_volume;
        int              bell_pitch;
        int              bell_duration;
        char            *volume_string;
#ifdef HAVE_X11_EXTENSIONS_XKB_H
        gboolean         rnumlock;
#endif /* HAVE_X11_EXTENSIONS_XKB_H */

        repeat        = mateconf_client_get_bool  (client, KEY_REPEAT, NULL);
        click         = mateconf_client_get_bool  (client, KEY_CLICK, NULL);
        rate          = mateconf_client_get_int   (client, KEY_RATE, NULL);
        delay         = mateconf_client_get_int   (client, KEY_DELAY, NULL);
        click_volume  = mateconf_client_get_int   (client, KEY_CLICK_VOLUME, NULL);
#if 0
        bell_volume   = mateconf_client_get_int   (client, KEY_BELL_VOLUME, NULL);
#endif
        bell_pitch    = mateconf_client_get_int   (client, KEY_BELL_PITCH, NULL);
        bell_duration = mateconf_client_get_int   (client, KEY_BELL_DURATION, NULL);

        volume_string = mateconf_client_get_string (client, KEY_BELL_MODE, NULL);
        bell_volume   = (volume_string && !strcmp (volume_string, "on")) ? 50 : 0;
        g_free (volume_string);

#ifdef HAVE_X11_EXTENSIONS_XKB_H
        rnumlock      = mateconf_client_get_bool  (client, GSD_KEYBOARD_KEY "/remember_numlock_state", NULL);
#endif /* HAVE_X11_EXTENSIONS_XKB_H */

        gdk_error_trap_push ();
        if (repeat) {
                gboolean rate_set = FALSE;

                XAutoRepeatOn (GDK_DISPLAY ());
                /* Use XKB in preference */
#ifdef HAVE_X11_EXTENSIONS_XKB_H
                rate_set = xkb_set_keyboard_autorepeat_rate (delay, rate);
#endif
#ifdef HAVE_X11_EXTENSIONS_XF86MISC_H
                if (!rate_set)
                        rate_set = xfree86_set_keyboard_autorepeat_rate (delay, rate);
#endif
                if (!rate_set)
                        g_warning ("Neither XKeyboard not Xfree86's keyboard extensions are available,\n"
                                   "no way to support keyboard autorepeat rate settings");
        } else {
                XAutoRepeatOff (GDK_DISPLAY ());
        }

        /* as percentage from 0..100 inclusive */
        if (click_volume < 0) {
                click_volume = 0;
        } else if (click_volume > 100) {
                click_volume = 100;
        }
        kbdcontrol.key_click_percent = click ? click_volume : 0;
        kbdcontrol.bell_percent = bell_volume;
        kbdcontrol.bell_pitch = bell_pitch;
        kbdcontrol.bell_duration = bell_duration;
        XChangeKeyboardControl (GDK_DISPLAY (),
                                KBKeyClickPercent | KBBellPercent | KBBellPitch | KBBellDuration,
                                &kbdcontrol);

#ifdef HAVE_X11_EXTENSIONS_XKB_H
        if (manager->priv->have_xkb && rnumlock) {
                numlock_set_xkb_state (numlock_get_mateconf_state (client));
        }
#endif /* HAVE_X11_EXTENSIONS_XKB_H */

        XSync (GDK_DISPLAY (), FALSE);
        gdk_error_trap_pop ();
}

void
gsd_keyboard_manager_apply_settings (GsdKeyboardManager *manager)
{
        MateConfClient *client;

        client = mateconf_client_get_default ();
        apply_settings (client, 0, NULL, manager);
        g_object_unref (client);
}

static gboolean
start_keyboard_idle_cb (GsdKeyboardManager *manager)
{
        MateConfClient *client;

        mate_settings_profile_start (NULL);

        g_debug ("Starting keyboard manager");

        manager->priv->have_xkb = 0;
        client = mateconf_client_get_default ();

        mateconf_client_add_dir (client, GSD_KEYBOARD_KEY, MATECONF_CLIENT_PRELOAD_RECURSIVE, NULL);

        /* Essential - xkb initialization should happen before */
        gsd_keyboard_xkb_set_post_activation_callback ((PostActivationCallback) gsd_load_modmap_files, NULL);
        gsd_keyboard_xkb_init (client, manager);

#ifdef HAVE_X11_EXTENSIONS_XKB_H
        numlock_xkb_init (manager);
#endif /* HAVE_X11_EXTENSIONS_XKB_H */

        /* apply current settings before we install the callback */
        gsd_keyboard_manager_apply_settings (manager);

        manager->priv->notify = mateconf_client_notify_add (client, GSD_KEYBOARD_KEY,
                                                         (MateConfClientNotifyFunc) apply_settings, manager,
                                                         NULL, NULL);

        g_object_unref (client);

#ifdef HAVE_X11_EXTENSIONS_XKB_H
        numlock_install_xkb_callback (manager);
#endif /* HAVE_X11_EXTENSIONS_XKB_H */

        mate_settings_profile_end (NULL);

        return FALSE;
}

gboolean
gsd_keyboard_manager_start (GsdKeyboardManager *manager,
                            GError            **error)
{
        mate_settings_profile_start (NULL);

        g_idle_add ((GSourceFunc) start_keyboard_idle_cb, manager);

        mate_settings_profile_end (NULL);

        return TRUE;
}

void
gsd_keyboard_manager_stop (GsdKeyboardManager *manager)
{
        GsdKeyboardManagerPrivate *p = manager->priv;

        g_debug ("Stopping keyboard manager");

        if (p->notify != 0) {
                MateConfClient *client = mateconf_client_get_default ();
                mateconf_client_remove_dir (client, GSD_KEYBOARD_KEY, NULL);
                mateconf_client_notify_remove (client, p->notify);
                g_object_unref (client);
                p->notify = 0;
        }

#if HAVE_X11_EXTENSIONS_XKB_H
        if (p->have_xkb) {
                gdk_window_remove_filter (NULL,
                                          numlock_xkb_callback,
                                          GINT_TO_POINTER (p->xkb_event_base));
        }
#endif /* HAVE_X11_EXTENSIONS_XKB_H */

        gsd_keyboard_xkb_shutdown ();
}

static void
gsd_keyboard_manager_set_property (GObject        *object,
                                   guint           prop_id,
                                   const GValue   *value,
                                   GParamSpec     *pspec)
{
        GsdKeyboardManager *self;

        self = GSD_KEYBOARD_MANAGER (object);

        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gsd_keyboard_manager_get_property (GObject        *object,
                                   guint           prop_id,
                                   GValue         *value,
                                   GParamSpec     *pspec)
{
        GsdKeyboardManager *self;

        self = GSD_KEYBOARD_MANAGER (object);

        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static GObject *
gsd_keyboard_manager_constructor (GType                  type,
                                  guint                  n_construct_properties,
                                  GObjectConstructParam *construct_properties)
{
        GsdKeyboardManager      *keyboard_manager;
        GsdKeyboardManagerClass *klass;

        klass = GSD_KEYBOARD_MANAGER_CLASS (g_type_class_peek (GSD_TYPE_KEYBOARD_MANAGER));

        keyboard_manager = GSD_KEYBOARD_MANAGER (G_OBJECT_CLASS (gsd_keyboard_manager_parent_class)->constructor (type,
                                                                                                      n_construct_properties,
                                                                                                      construct_properties));

        return G_OBJECT (keyboard_manager);
}

static void
gsd_keyboard_manager_dispose (GObject *object)
{
        GsdKeyboardManager *keyboard_manager;

        keyboard_manager = GSD_KEYBOARD_MANAGER (object);

        G_OBJECT_CLASS (gsd_keyboard_manager_parent_class)->dispose (object);
}

static void
gsd_keyboard_manager_class_init (GsdKeyboardManagerClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = gsd_keyboard_manager_get_property;
        object_class->set_property = gsd_keyboard_manager_set_property;
        object_class->constructor = gsd_keyboard_manager_constructor;
        object_class->dispose = gsd_keyboard_manager_dispose;
        object_class->finalize = gsd_keyboard_manager_finalize;

        g_type_class_add_private (klass, sizeof (GsdKeyboardManagerPrivate));
}

static void
gsd_keyboard_manager_init (GsdKeyboardManager *manager)
{
        manager->priv = GSD_KEYBOARD_MANAGER_GET_PRIVATE (manager);
}

static void
gsd_keyboard_manager_finalize (GObject *object)
{
        GsdKeyboardManager *keyboard_manager;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GSD_IS_KEYBOARD_MANAGER (object));

        keyboard_manager = GSD_KEYBOARD_MANAGER (object);

        g_return_if_fail (keyboard_manager->priv != NULL);

        G_OBJECT_CLASS (gsd_keyboard_manager_parent_class)->finalize (object);
}

GsdKeyboardManager *
gsd_keyboard_manager_new (void)
{
        if (manager_object != NULL) {
                g_object_ref (manager_object);
        } else {
                manager_object = g_object_new (GSD_TYPE_KEYBOARD_MANAGER, NULL);
                g_object_add_weak_pointer (manager_object,
                                           (gpointer *) &manager_object);
        }

        return GSD_KEYBOARD_MANAGER (manager_object);
}
