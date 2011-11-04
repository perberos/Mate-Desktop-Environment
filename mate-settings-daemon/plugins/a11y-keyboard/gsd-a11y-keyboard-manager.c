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
#include <mateconf/mateconf-client.h>

#include <X11/XKBlib.h>
#include <X11/extensions/XKBstr.h>

#ifdef HAVE_X11_EXTENSIONS_XINPUT_H
#include <X11/extensions/XInput.h>
#include <X11/extensions/XIproto.h>
#endif

#ifdef HAVE_LIBMATENOTIFY
#include <libmatenotify/notify.h>
#endif /* HAVE_LIBMATENOTIFY */

#include "mate-settings-profile.h"
#include "gsd-a11y-keyboard-manager.h"
#include "gsd-a11y-preferences-dialog.h"

#define CONFIG_ROOT "/desktop/mate/accessibility/keyboard"
#define NOTIFICATION_TIMEOUT 30

#define GSD_A11Y_KEYBOARD_MANAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GSD_TYPE_A11Y_KEYBOARD_MANAGER, GsdA11yKeyboardManagerPrivate))

struct GsdA11yKeyboardManagerPrivate
{
        int        xkbEventBase;
        gboolean   stickykeys_shortcut_val;
        gboolean   slowkeys_shortcut_val;
        GtkWidget *stickykeys_alert;
        GtkWidget *slowkeys_alert;
        GtkWidget *preferences_dialog;
        GtkStatusIcon *status_icon;
        XkbDescRec *original_xkb_desc;

        guint      mateconf_notify;

#ifdef HAVE_LIBMATENOTIFY
        NotifyNotification *notification;
#endif /* HAVE_LIBMATENOTIFY */
};

static void     gsd_a11y_keyboard_manager_class_init  (GsdA11yKeyboardManagerClass *klass);
static void     gsd_a11y_keyboard_manager_init        (GsdA11yKeyboardManager      *a11y_keyboard_manager);
static void     gsd_a11y_keyboard_manager_finalize    (GObject             *object);
static void     gsd_a11y_keyboard_manager_ensure_status_icon (GsdA11yKeyboardManager *manager);
static void     set_server_from_mateconf (GsdA11yKeyboardManager *manager,
				       MateConfClient            *client);

G_DEFINE_TYPE (GsdA11yKeyboardManager, gsd_a11y_keyboard_manager, G_TYPE_OBJECT)

static gpointer manager_object = NULL;

#undef DEBUG_ACCESSIBILITY
#ifdef DEBUG_ACCESSIBILITY
#define d(str)          g_debug (str)
#else
#define d(str)          do { } while (0)
#endif

#ifdef HAVE_X11_EXTENSIONS_XINPUT_H
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
            if (dpn->devchange == DeviceEnabled) {
                MateConfClient *client;
		client = mateconf_client_get_default ();
		set_server_from_mateconf (data, client);
                g_object_unref (client);
	    }
        }
        return GDK_FILTER_CONTINUE;
}

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

static void
set_devicepresence_handler (GsdA11yKeyboardManager *manager)
{
        Display *display;
        XEventClass class_presence;
        int xi_presence;

        if (!supports_xinput_devices ())
                return;

        display = gdk_x11_get_default_xdisplay ();

        gdk_error_trap_push ();
        DevicePresence (display, xi_presence, class_presence);
        /* FIXME:
         * Note that this might overwrite other events, see:
         * https://bugzilla.gnome.org/show_bug.cgi?id=610245#c2
         **/
        XSelectExtensionEvent (display,
                               RootWindow (display, DefaultScreen (display)),
                               &class_presence, 1);

        gdk_flush ();
        if (!gdk_error_trap_pop ())
                gdk_window_add_filter (NULL, devicepresence_filter, manager);
}
#endif

static gboolean
xkb_enabled (GsdA11yKeyboardManager *manager)
{
        gboolean have_xkb;
        int opcode, errorBase, major, minor;

        have_xkb = XkbQueryExtension (GDK_DISPLAY (),
                                      &opcode,
                                      &manager->priv->xkbEventBase,
                                      &errorBase,
                                      &major,
                                      &minor)
                && XkbUseExtension (GDK_DISPLAY (), &major, &minor);

        return have_xkb;
}

static XkbDescRec *
get_xkb_desc_rec (GsdA11yKeyboardManager *manager)
{
        XkbDescRec *desc;
        Status      status = Success;

        gdk_error_trap_push ();
        desc = XkbGetMap (GDK_DISPLAY (), XkbAllMapComponentsMask, XkbUseCoreKbd);
        if (desc != NULL) {
                desc->ctrls = NULL;
                status = XkbGetControls (GDK_DISPLAY (), XkbAllControlsMask, desc);
        }
        gdk_error_trap_pop ();

        g_return_val_if_fail (desc != NULL, NULL);
        g_return_val_if_fail (desc->ctrls != NULL, NULL);
        g_return_val_if_fail (status == Success, NULL);

        return desc;
}

static int
get_int (MateConfClient *client,
         char const  *key)
{
        int res = mateconf_client_get_int  (client, key, NULL);
        if (res <= 0) {
                res = 1;
        }
        return res;
}

static gboolean
set_int (MateConfClient    *client,
         MateConfChangeSet *cs,
         char const     *key,
         int             val)
{
        mateconf_change_set_set_int (cs, key, val);
#ifdef DEBUG_ACCESSIBILITY
        if (val != mateconf_client_get_int (client, key, NULL)) {
                g_warning ("%s changed", key);
        }
#endif
        return val != mateconf_client_get_int (client, key, NULL);
}

static gboolean
set_bool (MateConfClient    *client,
          MateConfChangeSet *cs,
          char const     *key,
          int             val)
{
        gboolean bval = (val != 0);

        mateconf_change_set_set_bool (cs, key, bval ? TRUE : FALSE);
#ifdef DEBUG_ACCESSIBILITY
                if (bval != mateconf_client_get_bool (client, key, NULL)) {
                        d ("%s changed", key);
                        return TRUE;
                }
#endif
        return (bval != mateconf_client_get_bool (client, key, NULL));
}

static unsigned long
set_clear (gboolean      flag,
           unsigned long value,
           unsigned long mask)
{
        if (flag) {
                return value | mask;
        }
        return value & ~mask;
}

static gboolean
set_ctrl_from_mateconf (XkbDescRec   *desc,
                     MateConfClient  *client,
                     char const   *key,
                     unsigned long mask)
{
        gboolean result = mateconf_client_get_bool (client, key, NULL);
        desc->ctrls->enabled_ctrls = set_clear (result, desc->ctrls->enabled_ctrls, mask);
        return result;
}

static void
set_server_from_mateconf (GsdA11yKeyboardManager *manager,
                       MateConfClient            *client)
{
        XkbDescRec      *desc;
        gboolean         enable_accessX;

        mate_settings_profile_start (NULL);

        desc = get_xkb_desc_rec (manager);
        if (!desc) {
                return;
        }

        /* general */
        enable_accessX = mateconf_client_get_bool (client, CONFIG_ROOT "/enable", NULL);

        desc->ctrls->enabled_ctrls = set_clear (enable_accessX,
                                                desc->ctrls->enabled_ctrls,
                                                XkbAccessXKeysMask);

        if (set_ctrl_from_mateconf (desc, client, CONFIG_ROOT "/timeout_enable",
                                 XkbAccessXTimeoutMask)) {
                desc->ctrls->ax_timeout = get_int (client,
                                                   CONFIG_ROOT "/timeout");
                /* disable only the master flag via the server we will disable
                 * the rest on the rebound without affecting mateconf state
                 * don't change the option flags at all.
                 */
                desc->ctrls->axt_ctrls_mask = XkbAccessXKeysMask | XkbAccessXFeedbackMask;
                desc->ctrls->axt_ctrls_values = 0;
                desc->ctrls->axt_opts_mask = 0;
        }

        desc->ctrls->ax_options = set_clear (mateconf_client_get_bool (client, CONFIG_ROOT "/feature_state_change_beep", NULL),
                                             desc->ctrls->ax_options,
                                             XkbAccessXFeedbackMask | XkbAX_FeatureFBMask | XkbAX_SlowWarnFBMask);

        /* bounce keys */
        if (set_ctrl_from_mateconf (desc,
                                 client,
                                 CONFIG_ROOT "/bouncekeys_enable",
                                 XkbBounceKeysMask)) {
                desc->ctrls->debounce_delay  = get_int (client,
                                                        CONFIG_ROOT "/bouncekeys_delay");
                desc->ctrls->ax_options = set_clear (mateconf_client_get_bool (client, CONFIG_ROOT "/bouncekeys_beep_reject", NULL),
                                                     desc->ctrls->ax_options,
                                                     XkbAccessXFeedbackMask | XkbAX_BKRejectFBMask);
        }

        /* mouse keys */
        if (set_ctrl_from_mateconf (desc,
                                 client,
                                 CONFIG_ROOT "/mousekeys_enable",
                                 XkbMouseKeysMask | XkbMouseKeysAccelMask)) {
                desc->ctrls->mk_interval     = 100;     /* msec between mousekey events */
                desc->ctrls->mk_curve        = 50;

                /* We store pixels / sec, XKB wants pixels / event */
                desc->ctrls->mk_max_speed    = get_int (client,
                        CONFIG_ROOT "/mousekeys_max_speed") / (1000 / desc->ctrls->mk_interval);
                if (desc->ctrls->mk_max_speed <= 0)
                        desc->ctrls->mk_max_speed = 1;

                desc->ctrls->mk_time_to_max = get_int (client, /* events before max */
                                                       CONFIG_ROOT "/mousekeys_accel_time") / desc->ctrls->mk_interval;
                if (desc->ctrls->mk_time_to_max <= 0)
                        desc->ctrls->mk_time_to_max = 1;

                desc->ctrls->mk_delay = get_int (client, /* ms before 1st event */
                                                 CONFIG_ROOT "/mousekeys_init_delay");
        }

        /* slow keys */
        if (set_ctrl_from_mateconf (desc,
                                 client,
                                 CONFIG_ROOT "/slowkeys_enable",
                                 XkbSlowKeysMask)) {
                desc->ctrls->ax_options = set_clear (mateconf_client_get_bool (client, CONFIG_ROOT "/slowkeys_beep_press", NULL),
                                                     desc->ctrls->ax_options,
                                                     XkbAccessXFeedbackMask | XkbAX_SKPressFBMask);
                desc->ctrls->ax_options = set_clear (mateconf_client_get_bool (client, CONFIG_ROOT "/slowkeys_beep_accept", NULL),
                                                     desc->ctrls->ax_options,
                                                     XkbAccessXFeedbackMask | XkbAX_SKAcceptFBMask);
                desc->ctrls->ax_options = set_clear (mateconf_client_get_bool (client, CONFIG_ROOT "/slowkeys_beep_reject", NULL),
                                                     desc->ctrls->ax_options,
                                                     XkbAccessXFeedbackMask | XkbAX_SKRejectFBMask);
                desc->ctrls->slow_keys_delay = get_int (client,
                                                        CONFIG_ROOT "/slowkeys_delay");
                /* anything larger than 500 seems to loose all keyboard input */
                if (desc->ctrls->slow_keys_delay > 500)
                        desc->ctrls->slow_keys_delay = 500;
        }

        /* sticky keys */
        if (set_ctrl_from_mateconf (desc,
                                 client,
                                 CONFIG_ROOT "/stickykeys_enable",
                                 XkbStickyKeysMask)) {
                desc->ctrls->ax_options |= XkbAX_LatchToLockMask;
                desc->ctrls->ax_options = set_clear (mateconf_client_get_bool (client, CONFIG_ROOT "/stickykeys_two_key_off", NULL),
                                                     desc->ctrls->ax_options,
                                                     XkbAccessXFeedbackMask | XkbAX_TwoKeysMask);
                desc->ctrls->ax_options = set_clear (mateconf_client_get_bool (client, CONFIG_ROOT "/stickykeys_modifier_beep", NULL),
                                                     desc->ctrls->ax_options,
                                                     XkbAccessXFeedbackMask | XkbAX_StickyKeysFBMask);
        }

        /* toggle keys */
        desc->ctrls->ax_options = set_clear (mateconf_client_get_bool (client, CONFIG_ROOT "/togglekeys_enable", NULL),
                                             desc->ctrls->ax_options,
                                             XkbAccessXFeedbackMask | XkbAX_IndicatorFBMask);

        /*
        g_debug ("CHANGE to : 0x%x", desc->ctrls->enabled_ctrls);
        g_debug ("CHANGE to : 0x%x (2)", desc->ctrls->ax_options);
        */

        gdk_error_trap_push ();
        XkbSetControls (GDK_DISPLAY (),
                        XkbSlowKeysMask         |
                        XkbBounceKeysMask       |
                        XkbStickyKeysMask       |
                        XkbMouseKeysMask        |
                        XkbMouseKeysAccelMask   |
                        XkbAccessXKeysMask      |
                        XkbAccessXTimeoutMask   |
                        XkbAccessXFeedbackMask  |
                        XkbControlsEnabledMask,
                        desc);

        XkbFreeKeyboard (desc, XkbAllComponentsMask, True);

        XSync (GDK_DISPLAY (), FALSE);
        gdk_error_trap_pop ();

        mate_settings_profile_end (NULL);
}

static gboolean
ax_response_callback (GsdA11yKeyboardManager *manager,
                      GtkWindow              *parent,
                      gint                    response_id,
                      guint                   revert_controls_mask,
                      gboolean                enabled)
{
        MateConfClient *client;
        GdkScreen *screen;
        GError *err;

        switch (response_id) {
        case GTK_RESPONSE_DELETE_EVENT:
        case GTK_RESPONSE_REJECT:
        case GTK_RESPONSE_CANCEL:

                client = mateconf_client_get_default ();

                /* we're reverting, so we invert sense of 'enabled' flag */
                d ("cancelling AccessX request");
                if (revert_controls_mask == XkbStickyKeysMask) {
                        mateconf_client_set_bool (client,
                                               CONFIG_ROOT "/stickykeys_enable",
                                               !enabled,
                                               NULL);
                }
                else if (revert_controls_mask == XkbSlowKeysMask) {
                        mateconf_client_set_bool (client,
                                               CONFIG_ROOT "/slowkeys_enable",
                                               !enabled,
                                               NULL);
                }
                mateconf_client_suggest_sync (client, NULL);
                set_server_from_mateconf (manager, client);

                g_object_unref (client);

                break;

        case GTK_RESPONSE_HELP:
                if (!parent)
                        screen = gdk_screen_get_default ();
                else
                        screen = gtk_widget_get_screen (GTK_WIDGET (parent));

                err = NULL;
                if (!gtk_show_uri (screen,
                                   "ghelp:user-guide#goscustaccess-6",
                                   gtk_get_current_event_time(),
                                   &err)) {
                        GtkWidget *error_dialog = gtk_message_dialog_new (parent,
                                                                          0,
                                                                          GTK_MESSAGE_ERROR,
                                                                          GTK_BUTTONS_CLOSE,
                                                                          _("There was an error displaying help: %s"),
                                                                          err->message);
                        g_signal_connect (error_dialog, "response",
                                          G_CALLBACK (gtk_widget_destroy), NULL);
                        gtk_window_set_resizable (GTK_WINDOW (error_dialog), FALSE);
                        gtk_widget_show (error_dialog);
                        g_error_free (err);
                }
                return FALSE;
        default:
                break;
        }
        return TRUE;
}

static void
ax_stickykeys_response (GtkDialog              *dialog,
                        gint                    response_id,
                        GsdA11yKeyboardManager *manager)
{
        if (ax_response_callback (manager, GTK_WINDOW (dialog),
                                  response_id, XkbStickyKeysMask,
                                  manager->priv->stickykeys_shortcut_val)) {
                gtk_widget_destroy (GTK_WIDGET (dialog));
        }
}

static void
ax_slowkeys_response (GtkDialog              *dialog,
                      gint                    response_id,
                      GsdA11yKeyboardManager *manager)
{
        if (ax_response_callback (manager, GTK_WINDOW (dialog),
                                  response_id, XkbSlowKeysMask,
                                  manager->priv->slowkeys_shortcut_val)) {
                gtk_widget_destroy (GTK_WIDGET (dialog));
        }
}

static void
maybe_show_status_icon (GsdA11yKeyboardManager *manager)
{
        gboolean     show;
        MateConfClient *client;

        /* for now, show if accessx is enabled */
        client = mateconf_client_get_default ();
        show = mateconf_client_get_bool (client, CONFIG_ROOT "/enable", NULL);
        g_object_unref (client);

        if (!show && manager->priv->status_icon == NULL)
                return;

        gsd_a11y_keyboard_manager_ensure_status_icon (manager);
        gtk_status_icon_set_visible (manager->priv->status_icon, show);
}

#ifdef HAVE_LIBMATENOTIFY
static void
on_notification_closed (NotifyNotification     *notification,
                        GsdA11yKeyboardManager *manager)
{
        g_object_unref (manager->priv->notification);
        manager->priv->notification = NULL;
}

static void
on_slow_keys_action (NotifyNotification     *notification,
                     const char             *action,
                     GsdA11yKeyboardManager *manager)
{
        gboolean res;
        int      response_id;

        g_assert (action != NULL);

        if (strcmp (action, "accept") == 0) {
                response_id = GTK_RESPONSE_ACCEPT;
        } else if (strcmp (action, "reject") == 0) {
                response_id = GTK_RESPONSE_REJECT;
        } else {
                return;
        }

        res = ax_response_callback (manager, NULL,
                                    response_id, XkbSlowKeysMask,
                                    manager->priv->slowkeys_shortcut_val);
        if (res) {
                notify_notification_close (manager->priv->notification, NULL);
        }
}

static void
on_sticky_keys_action (NotifyNotification     *notification,
                       const char             *action,
                       GsdA11yKeyboardManager *manager)
{
        gboolean res;
        int      response_id;

        g_assert (action != NULL);

        if (strcmp (action, "accept") == 0) {
                response_id = GTK_RESPONSE_ACCEPT;
        } else if (strcmp (action, "reject") == 0) {
                response_id = GTK_RESPONSE_REJECT;
        } else {
                return;
        }

        res = ax_response_callback (manager, NULL,
                                    response_id, XkbStickyKeysMask,
                                    manager->priv->stickykeys_shortcut_val);
        if (res) {
                notify_notification_close (manager->priv->notification, NULL);
        }
}

#endif /* HAVE_LIBMATENOTIFY */

static gboolean
ax_slowkeys_warning_post_bubble (GsdA11yKeyboardManager *manager,
                                 gboolean                enabled)
{
#ifdef HAVE_LIBMATENOTIFY
        gboolean    res;
        const char *title;
        const char *message;
        GError     *error;

        title = enabled ?
                _("Do you want to activate Slow Keys?") :
                _("Do you want to deactivate Slow Keys?");
        message = _("You just held down the Shift key for 8 seconds.  This is the shortcut "
                    "for the Slow Keys feature, which affects the way your keyboard works.");

        if (manager->priv->status_icon == NULL || ! gtk_status_icon_is_embedded (manager->priv->status_icon)) {
                return FALSE;
        }

        if (manager->priv->slowkeys_alert != NULL) {
                gtk_widget_destroy (manager->priv->slowkeys_alert);
        }

        if (manager->priv->notification != NULL) {
                notify_notification_close (manager->priv->notification, NULL);
        }

        gsd_a11y_keyboard_manager_ensure_status_icon (manager);
        manager->priv->notification = notify_notification_new (title,
                                                               message,
                                                               "preferences-desktop-accessibility",
                                                               NULL);
        notify_notification_attach_to_status_icon (manager->priv->notification, manager->priv->status_icon);
        notify_notification_set_timeout (manager->priv->notification, NOTIFICATION_TIMEOUT * 1000);

        notify_notification_add_action (manager->priv->notification,
                                        "reject",
                                        enabled ? _("Don't activate") : _("Don't deactivate"),
                                        (NotifyActionCallback) on_slow_keys_action,
                                        manager,
                                        NULL);
        notify_notification_add_action (manager->priv->notification,
                                        "accept",
                                        enabled ? _("Activate") : _("Deactivate"),
                                        (NotifyActionCallback) on_slow_keys_action,
                                        manager,
                                        NULL);

        g_signal_connect (manager->priv->notification,
                          "closed",
                          G_CALLBACK (on_notification_closed),
                          manager);

        error = NULL;
        res = notify_notification_show (manager->priv->notification, &error);
        if (! res) {
                g_warning ("GsdA11yKeyboardManager: unable to show notification: %s", error->message);
                g_error_free (error);
                notify_notification_close (manager->priv->notification, NULL);
        }

        return res;
#else
        return FALSE;
#endif /* HAVE_LIBMATENOTIFY */
}


static void
ax_slowkeys_warning_post_dialog (GsdA11yKeyboardManager *manager,
                                 gboolean                enabled)
{
        const char *title;
        const char *message;

        title = enabled ?
                _("Do you want to activate Slow Keys?") :
                _("Do you want to deactivate Slow Keys?");
        message = _("You just held down the Shift key for 8 seconds.  This is the shortcut "
                    "for the Slow Keys feature, which affects the way your keyboard works.");

        if (manager->priv->slowkeys_alert != NULL) {
                gtk_widget_show (manager->priv->slowkeys_alert);
                return;
        }

        manager->priv->slowkeys_alert = gtk_message_dialog_new (NULL,
                                                                0,
                                                                GTK_MESSAGE_WARNING,
                                                                GTK_BUTTONS_NONE,
                                                                "%s", title);

        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (manager->priv->slowkeys_alert),
                                                  "%s", message);

        gtk_dialog_add_button (GTK_DIALOG (manager->priv->slowkeys_alert),
                               GTK_STOCK_HELP,
                               GTK_RESPONSE_HELP);
        gtk_dialog_add_button (GTK_DIALOG (manager->priv->slowkeys_alert),
                               enabled ? _("Do_n't activate") : _("Do_n't deactivate"),
                               GTK_RESPONSE_REJECT);
        gtk_dialog_add_button (GTK_DIALOG (manager->priv->slowkeys_alert),
                               enabled ? _("_Activate") : _("_Deactivate"),
                               GTK_RESPONSE_ACCEPT);

        gtk_window_set_title (GTK_WINDOW (manager->priv->slowkeys_alert),
                              _("Slow Keys Alert"));
        gtk_window_set_icon_name (GTK_WINDOW (manager->priv->slowkeys_alert),
                                  "input-keyboard");
        gtk_dialog_set_default_response (GTK_DIALOG (manager->priv->slowkeys_alert),
                                         GTK_RESPONSE_ACCEPT);

        g_signal_connect (manager->priv->slowkeys_alert,
                          "response",
                          G_CALLBACK (ax_slowkeys_response),
                          manager);
        gtk_widget_show (manager->priv->slowkeys_alert);

        g_object_add_weak_pointer (G_OBJECT (manager->priv->slowkeys_alert),
                                   (gpointer*) &manager->priv->slowkeys_alert);
}

static void
ax_slowkeys_warning_post (GsdA11yKeyboardManager *manager,
                          gboolean                enabled)
{

        manager->priv->slowkeys_shortcut_val = enabled;

        /* alway try to show something */
        if (! ax_slowkeys_warning_post_bubble (manager, enabled)) {
                ax_slowkeys_warning_post_dialog (manager, enabled);
        }
}

static gboolean
ax_stickykeys_warning_post_bubble (GsdA11yKeyboardManager *manager,
                                   gboolean                enabled)
{
#ifdef HAVE_LIBMATENOTIFY
        gboolean    res;
        const char *title;
        const char *message;
        GError     *error;

        title = enabled ?
                _("Do you want to activate Sticky Keys?") :
                _("Do you want to deactivate Sticky Keys?");
        message = enabled ?
                _("You just pressed the Shift key 5 times in a row.  This is the shortcut "
                  "for the Sticky Keys feature, which affects the way your keyboard works.") :
                _("You just pressed two keys at once, or pressed the Shift key 5 times in a row.  "
                  "This turns off the Sticky Keys feature, which affects the way your keyboard works.");

        if (manager->priv->status_icon == NULL || ! gtk_status_icon_is_embedded (manager->priv->status_icon)) {
                return FALSE;
        }

        if (manager->priv->slowkeys_alert != NULL) {
                gtk_widget_destroy (manager->priv->slowkeys_alert);
        }

        if (manager->priv->notification != NULL) {
                notify_notification_close (manager->priv->notification, NULL);
        }

        gsd_a11y_keyboard_manager_ensure_status_icon (manager);
        manager->priv->notification = notify_notification_new (title,
                                                               message,
                                                               "preferences-desktop-accessibility",
                                                               NULL);
        notify_notification_attach_to_status_icon (manager->priv->notification, manager->priv->status_icon);
        notify_notification_set_timeout (manager->priv->notification, NOTIFICATION_TIMEOUT * 1000);

        notify_notification_add_action (manager->priv->notification,
                                        "reject",
                                        enabled ? _("Don't activate") : _("Don't deactivate"),
                                        (NotifyActionCallback) on_sticky_keys_action,
                                        manager,
                                        NULL);
        notify_notification_add_action (manager->priv->notification,
                                        "accept",
                                        enabled ? _("Activate") : _("Deactivate"),
                                        (NotifyActionCallback) on_sticky_keys_action,
                                        manager,
                                        NULL);

        g_signal_connect (manager->priv->notification,
                          "closed",
                          G_CALLBACK (on_notification_closed),
                          manager);

        error = NULL;
        res = notify_notification_show (manager->priv->notification, &error);
        if (! res) {
                g_warning ("GsdA11yKeyboardManager: unable to show notification: %s", error->message);
                g_error_free (error);
                notify_notification_close (manager->priv->notification, NULL);
        }

        return res;
#else
        return FALSE;
#endif /* HAVE_LIBMATENOTIFY */
}

static void
ax_stickykeys_warning_post_dialog (GsdA11yKeyboardManager *manager,
                                   gboolean                enabled)
{
        const char *title;
        const char *message;

        title = enabled ?
                _("Do you want to activate Sticky Keys?") :
                _("Do you want to deactivate Sticky Keys?");
        message = enabled ?
                _("You just pressed the Shift key 5 times in a row.  This is the shortcut "
                  "for the Sticky Keys feature, which affects the way your keyboard works.") :
                _("You just pressed two keys at once, or pressed the Shift key 5 times in a row.  "
                  "This turns off the Sticky Keys feature, which affects the way your keyboard works.");

        if (manager->priv->stickykeys_alert != NULL) {
                gtk_widget_show (manager->priv->stickykeys_alert);
                return;
        }

        manager->priv->stickykeys_alert = gtk_message_dialog_new (NULL,
                                                                  0,
                                                                  GTK_MESSAGE_WARNING,
                                                                  GTK_BUTTONS_NONE,
                                                                  "%s", title);

        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (manager->priv->stickykeys_alert),
                                                  "%s", message);

        gtk_dialog_add_button (GTK_DIALOG (manager->priv->stickykeys_alert),
                               GTK_STOCK_HELP,
                               GTK_RESPONSE_HELP);
        gtk_dialog_add_button (GTK_DIALOG (manager->priv->stickykeys_alert),
                               enabled ? _("Do_n't activate") : _("Do_n't deactivate"),
                               GTK_RESPONSE_REJECT);
        gtk_dialog_add_button (GTK_DIALOG (manager->priv->stickykeys_alert),
                               enabled ? _("_Activate") : _("_Deactivate"),
                               GTK_RESPONSE_ACCEPT);

        gtk_window_set_title (GTK_WINDOW (manager->priv->stickykeys_alert),
                              _("Sticky Keys Alert"));
        gtk_window_set_icon_name (GTK_WINDOW (manager->priv->stickykeys_alert),
                                  "input-keyboard");
        gtk_dialog_set_default_response (GTK_DIALOG (manager->priv->stickykeys_alert),
                                         GTK_RESPONSE_ACCEPT);

        g_signal_connect (manager->priv->stickykeys_alert,
                          "response",
                          G_CALLBACK (ax_stickykeys_response),
                          manager);
        gtk_widget_show (manager->priv->stickykeys_alert);

        g_object_add_weak_pointer (G_OBJECT (manager->priv->stickykeys_alert),
                                   (gpointer*) &manager->priv->stickykeys_alert);
}

static void
ax_stickykeys_warning_post (GsdA11yKeyboardManager *manager,
                            gboolean                enabled)
{

        manager->priv->stickykeys_shortcut_val = enabled;

        /* alway try to show something */
        if (! ax_stickykeys_warning_post_bubble (manager, enabled)) {
                ax_stickykeys_warning_post_dialog (manager, enabled);
        }
}

static void
set_mateconf_from_server (GsdA11yKeyboardManager *manager)
{
        MateConfClient    *client;
        MateConfChangeSet *cs;
        XkbDescRec     *desc;
        gboolean        changed = FALSE;
        gboolean        slowkeys_changed;
        gboolean        stickykeys_changed;

        cs = mateconf_change_set_new ();
        desc = get_xkb_desc_rec (manager);
        if (! desc) {
                return;
        }

        client = mateconf_client_get_default ();

        /*
          fprintf (stderr, "changed to : 0x%x\n", desc->ctrls->enabled_ctrls);
          fprintf (stderr, "changed to : 0x%x (2)\n", desc->ctrls->ax_options);
        */

        changed |= set_bool (client,
                             cs,
                             CONFIG_ROOT "/enable",
                             desc->ctrls->enabled_ctrls & XkbAccessXKeysMask);

        changed |= set_bool (client,
                             cs,
                             CONFIG_ROOT "/feature_state_change_beep",
                             desc->ctrls->ax_options & (XkbAX_FeatureFBMask | XkbAX_SlowWarnFBMask));
        changed |= set_bool (client,
                             cs,
                             CONFIG_ROOT "/timeout_enable",
                             desc->ctrls->enabled_ctrls & XkbAccessXTimeoutMask);
        changed |= set_int (client,
                            cs,
                            CONFIG_ROOT "/timeout",
                            desc->ctrls->ax_timeout);

        changed |= set_bool (client,
                             cs,
                             CONFIG_ROOT "/bouncekeys_enable",
                             desc->ctrls->enabled_ctrls & XkbBounceKeysMask);
        changed |= set_int (client,
                            cs,
                            CONFIG_ROOT "/bouncekeys_delay",
                            desc->ctrls->debounce_delay);
        changed |= set_bool (client,
                             cs,
                             CONFIG_ROOT "/bouncekeys_beep_reject",
                             desc->ctrls->ax_options & XkbAX_BKRejectFBMask);

        changed |= set_bool (client,
                             cs,
                             CONFIG_ROOT "/mousekeys_enable",
                             desc->ctrls->enabled_ctrls & XkbMouseKeysMask);
        changed |= set_int (client,
                            cs,
                            CONFIG_ROOT "/mousekeys_max_speed",
                            desc->ctrls->mk_max_speed * (1000 / desc->ctrls->mk_interval));
        /* NOTE : mk_time_to_max is measured in events not time */
        changed |= set_int (client,
                            cs,
                            CONFIG_ROOT "/mousekeys_accel_time",
                            desc->ctrls->mk_time_to_max * desc->ctrls->mk_interval);
        changed |= set_int (client,
                            cs,
                            CONFIG_ROOT "/mousekeys_init_delay",
                            desc->ctrls->mk_delay);

        slowkeys_changed = set_bool (client,
                                     cs,
                                     CONFIG_ROOT "/slowkeys_enable",
                                     desc->ctrls->enabled_ctrls & XkbSlowKeysMask);
        changed |= set_bool (client,
                             cs,
                             CONFIG_ROOT "/slowkeys_beep_press",
                             desc->ctrls->ax_options & XkbAX_SKPressFBMask);
        changed |= set_bool (client,
                             cs,
                             CONFIG_ROOT "/slowkeys_beep_accept",
                             desc->ctrls->ax_options & XkbAX_SKAcceptFBMask);
        changed |= set_bool (client,
                             cs,
                             CONFIG_ROOT "/slowkeys_beep_reject",
                             desc->ctrls->ax_options & XkbAX_SKRejectFBMask);
        changed |= set_int (client,
                            cs,
                            CONFIG_ROOT "/slowkeys_delay",
                            desc->ctrls->slow_keys_delay);

        stickykeys_changed = set_bool (client,
                                       cs,
                                       CONFIG_ROOT "/stickykeys_enable",
                                       desc->ctrls->enabled_ctrls & XkbStickyKeysMask);
        changed |= set_bool (client,
                             cs,
                             CONFIG_ROOT "/stickykeys_two_key_off",
                             desc->ctrls->ax_options & XkbAX_TwoKeysMask);
        changed |= set_bool (client,
                             cs,
                             CONFIG_ROOT "/stickykeys_modifier_beep",
                             desc->ctrls->ax_options & XkbAX_StickyKeysFBMask);

        changed |= set_bool (client,
                             cs,
                             CONFIG_ROOT "/togglekeys_enable",
                             desc->ctrls->ax_options & XkbAX_IndicatorFBMask);

        if (!changed && stickykeys_changed ^ slowkeys_changed) {
                /*
                 * sticky or slowkeys has changed, singly, without our intervention.
                 * 99% chance this is due to a keyboard shortcut being used.
                 * we need to detect via this hack until we get
                 *  XkbAXN_AXKWarning notifications working (probable XKB bug),
                 *  at which time we can directly intercept such shortcuts instead.
                 * See cb_xkb_event_filter () below.
                 */

                /* sanity check: are keyboard shortcuts available? */
                if (desc->ctrls->enabled_ctrls & XkbAccessXKeysMask) {
                        if (slowkeys_changed) {
                                ax_slowkeys_warning_post (manager,
                                                          desc->ctrls->enabled_ctrls & XkbSlowKeysMask);
                        } else {
                                ax_stickykeys_warning_post (manager,
                                                            desc->ctrls->enabled_ctrls & XkbStickyKeysMask);
                        }
                }
        }

        XkbFreeKeyboard (desc, XkbAllComponentsMask, True);

        changed |= (stickykeys_changed | slowkeys_changed);

        if (changed) {
                mateconf_client_commit_change_set (client, cs, FALSE, NULL);
                mateconf_client_suggest_sync (client, NULL);
        }
        mateconf_change_set_unref (cs);

        g_object_unref (client);
}

static GdkFilterReturn
cb_xkb_event_filter (GdkXEvent              *xevent,
                     GdkEvent               *ignored1,
                     GsdA11yKeyboardManager *manager)
{
        XEvent   *xev   = (XEvent *) xevent;
        XkbEvent *xkbEv = (XkbEvent *) xevent;

        if (xev->xany.type == (manager->priv->xkbEventBase + XkbEventCode) &&
            xkbEv->any.xkb_type == XkbControlsNotify) {
                d ("XKB state changed");
                set_mateconf_from_server (manager);
        } else if (xev->xany.type == (manager->priv->xkbEventBase + XkbEventCode) &&
                   xkbEv->any.xkb_type == XkbAccessXNotify) {
                if (xkbEv->accessx.detail == XkbAXN_AXKWarning) {
                        d ("About to turn on an AccessX feature from the keyboard!");
                        /*
                         * TODO: when XkbAXN_AXKWarnings start working, we need to
                         * invoke ax_keys_warning_dialog_run here instead of in
                         * set_mateconf_from_server().
                         */
                }
        }

        return GDK_FILTER_CONTINUE;
}

static void
keyboard_callback (MateConfClient            *client,
                   guint                   cnxn_id,
                   MateConfEntry             *entry,
                   GsdA11yKeyboardManager *manager)
{
        set_server_from_mateconf (manager, client);
        maybe_show_status_icon (manager);
}

static void
register_config_callback (GsdA11yKeyboardManager  *manager,
                          MateConfClient             *client,
                          const char              *path,
                          MateConfClientNotifyFunc    func,
                          guint                   *notify)
{
        mateconf_client_add_dir (client, path, MATECONF_CLIENT_PRELOAD_ONELEVEL, NULL);
        *notify = mateconf_client_notify_add (client, path, func, manager, NULL, NULL);
}

static gboolean
start_a11y_keyboard_idle_cb (GsdA11yKeyboardManager *manager)
{
        guint        event_mask;
        MateConfClient *client;

        g_debug ("Starting a11y_keyboard manager");
        mate_settings_profile_start (NULL);

        if (!xkb_enabled (manager))
                goto out;

        client = mateconf_client_get_default ();

        register_config_callback (manager,
                                  client,
                                  CONFIG_ROOT,
                                  (MateConfClientNotifyFunc) keyboard_callback,
                                  &manager->priv->mateconf_notify);

#ifdef HAVE_X11_EXTENSIONS_XINPUT_H
	set_devicepresence_handler (manager);
#endif

        /* Save current xkb state so we can restore it on exit
         */
        manager->priv->original_xkb_desc = get_xkb_desc_rec (manager);

        event_mask = XkbControlsNotifyMask;
#ifdef DEBUG_ACCESSIBILITY
        event_mask |= XkbAccessXNotifyMask; /* make default when AXN_AXKWarning works */
#endif

        /* be sure to init before starting to monitor the server */
        set_server_from_mateconf (manager, client);
        g_object_unref (client);

        XkbSelectEvents (GDK_DISPLAY (),
                         XkbUseCoreKbd,
                         event_mask,
                         event_mask);

        gdk_window_add_filter (NULL,
                               (GdkFilterFunc) cb_xkb_event_filter,
                               manager);

        maybe_show_status_icon (manager);

 out:
        mate_settings_profile_end (NULL);

        return FALSE;
}


gboolean
gsd_a11y_keyboard_manager_start (GsdA11yKeyboardManager *manager,
                                 GError                **error)
{
        mate_settings_profile_start (NULL);

        g_idle_add ((GSourceFunc) start_a11y_keyboard_idle_cb, manager);

        mate_settings_profile_end (NULL);

        return TRUE;
}

static void
restore_server_xkb_config (GsdA11yKeyboardManager *manager)
{
        gdk_error_trap_push ();
        XkbSetControls (GDK_DISPLAY (),
                        XkbSlowKeysMask         |
                        XkbBounceKeysMask       |
                        XkbStickyKeysMask       |
                        XkbMouseKeysMask        |
                        XkbMouseKeysAccelMask   |
                        XkbAccessXKeysMask      |
                        XkbAccessXTimeoutMask   |
                        XkbAccessXFeedbackMask  |
                        XkbControlsEnabledMask,
                        manager->priv->original_xkb_desc);

        XkbFreeKeyboard (manager->priv->original_xkb_desc,
                         XkbAllComponentsMask, True);

        XSync (GDK_DISPLAY (), FALSE);
        gdk_error_trap_pop ();

        manager->priv->original_xkb_desc = NULL;
}

void
gsd_a11y_keyboard_manager_stop (GsdA11yKeyboardManager *manager)
{
        GsdA11yKeyboardManagerPrivate *p = manager->priv;

        g_debug ("Stopping a11y_keyboard manager");

#ifdef HAVE_X11_EXTENSIONS_XINPUT_H
        gdk_window_remove_filter (NULL, devicepresence_filter, manager);
#endif

        if (p->status_icon)
                gtk_status_icon_set_visible (p->status_icon, FALSE);

        if (p->mateconf_notify != 0) {
                MateConfClient *client = mateconf_client_get_default ();
                mateconf_client_remove_dir (client, CONFIG_ROOT, NULL);
                mateconf_client_notify_remove (client, p->mateconf_notify);
                g_object_unref (client);
                p->mateconf_notify = 0;
        }

        gdk_window_remove_filter (NULL,
                                  (GdkFilterFunc) cb_xkb_event_filter,
                                  manager);

        /* Disable all the AccessX bits
         */
        restore_server_xkb_config (manager);

        if (p->slowkeys_alert != NULL)
                gtk_widget_destroy (p->slowkeys_alert);

        if (p->stickykeys_alert != NULL)
                gtk_widget_destroy (p->stickykeys_alert);

        p->slowkeys_shortcut_val = FALSE;
        p->stickykeys_shortcut_val = FALSE;
}

static void
gsd_a11y_keyboard_manager_set_property (GObject        *object,
                                        guint           prop_id,
                                        const GValue   *value,
                                        GParamSpec     *pspec)
{
        GsdA11yKeyboardManager *self;

        self = GSD_A11Y_KEYBOARD_MANAGER (object);

        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gsd_a11y_keyboard_manager_get_property (GObject        *object,
                                        guint           prop_id,
                                        GValue         *value,
                                        GParamSpec     *pspec)
{
        GsdA11yKeyboardManager *self;

        self = GSD_A11Y_KEYBOARD_MANAGER (object);

        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static GObject *
gsd_a11y_keyboard_manager_constructor (GType                  type,
                                       guint                  n_construct_properties,
                                       GObjectConstructParam *construct_properties)
{
        GsdA11yKeyboardManager      *a11y_keyboard_manager;
        GsdA11yKeyboardManagerClass *klass;

        klass = GSD_A11Y_KEYBOARD_MANAGER_CLASS (g_type_class_peek (GSD_TYPE_A11Y_KEYBOARD_MANAGER));

        a11y_keyboard_manager = GSD_A11Y_KEYBOARD_MANAGER (G_OBJECT_CLASS (gsd_a11y_keyboard_manager_parent_class)->constructor (type,
                                                                                                      n_construct_properties,
                                                                                                      construct_properties));

        return G_OBJECT (a11y_keyboard_manager);
}

static void
gsd_a11y_keyboard_manager_dispose (GObject *object)
{
        GsdA11yKeyboardManager *a11y_keyboard_manager;

        a11y_keyboard_manager = GSD_A11Y_KEYBOARD_MANAGER (object);

        G_OBJECT_CLASS (gsd_a11y_keyboard_manager_parent_class)->dispose (object);
}

static void
gsd_a11y_keyboard_manager_class_init (GsdA11yKeyboardManagerClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = gsd_a11y_keyboard_manager_get_property;
        object_class->set_property = gsd_a11y_keyboard_manager_set_property;
        object_class->constructor = gsd_a11y_keyboard_manager_constructor;
        object_class->dispose = gsd_a11y_keyboard_manager_dispose;
        object_class->finalize = gsd_a11y_keyboard_manager_finalize;

        g_type_class_add_private (klass, sizeof (GsdA11yKeyboardManagerPrivate));
}

static void
on_preferences_dialog_response (GtkDialog              *dialog,
                                int                     response,
                                GsdA11yKeyboardManager *manager)
{
        g_signal_handlers_disconnect_by_func (dialog,
                                              on_preferences_dialog_response,
                                              manager);

        gtk_widget_destroy (GTK_WIDGET (dialog));
        manager->priv->preferences_dialog = NULL;
}

static void
on_status_icon_activate (GtkStatusIcon          *status_icon,
                         GsdA11yKeyboardManager *manager)
{
        if (manager->priv->preferences_dialog == NULL) {
                manager->priv->preferences_dialog = gsd_a11y_preferences_dialog_new ();
                g_signal_connect (manager->priv->preferences_dialog,
                                  "response",
                                  G_CALLBACK (on_preferences_dialog_response),
                                  manager);

                gtk_window_present (GTK_WINDOW (manager->priv->preferences_dialog));
        } else {
                g_signal_handlers_disconnect_by_func (manager->priv->preferences_dialog,
                                                      on_preferences_dialog_response,
                                                      manager);
                gtk_widget_destroy (GTK_WIDGET (manager->priv->preferences_dialog));
                manager->priv->preferences_dialog = NULL;
        }
}

static void
gsd_a11y_keyboard_manager_ensure_status_icon (GsdA11yKeyboardManager *manager)
{
        mate_settings_profile_start (NULL);

        if (!manager->priv->status_icon) {

                manager->priv->status_icon = gtk_status_icon_new_from_icon_name ("preferences-desktop-accessibility");
                g_signal_connect (manager->priv->status_icon,
                                  "activate",
                                  G_CALLBACK (on_status_icon_activate),
                                  manager);
        }

        mate_settings_profile_end (NULL);
}

static void
gsd_a11y_keyboard_manager_init (GsdA11yKeyboardManager *manager)
{
        manager->priv = GSD_A11Y_KEYBOARD_MANAGER_GET_PRIVATE (manager);

#ifdef HAVE_LIBMATENOTIFY
        notify_init ("mate-settings-daemon");
#endif /* HAVE_LIBMATENOTIFY */
}

static void
gsd_a11y_keyboard_manager_finalize (GObject *object)
{
        GsdA11yKeyboardManager *a11y_keyboard_manager;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GSD_IS_A11Y_KEYBOARD_MANAGER (object));

        a11y_keyboard_manager = GSD_A11Y_KEYBOARD_MANAGER (object);

        g_return_if_fail (a11y_keyboard_manager->priv != NULL);

        G_OBJECT_CLASS (gsd_a11y_keyboard_manager_parent_class)->finalize (object);
}

GsdA11yKeyboardManager *
gsd_a11y_keyboard_manager_new (void)
{
        if (manager_object != NULL) {
                g_object_ref (manager_object);
        } else {
                manager_object = g_object_new (GSD_TYPE_A11Y_KEYBOARD_MANAGER, NULL);
                g_object_add_weak_pointer (manager_object,
                                           (gpointer *) &manager_object);
        }

        return GSD_A11Y_KEYBOARD_MANAGER (manager_object);
}
