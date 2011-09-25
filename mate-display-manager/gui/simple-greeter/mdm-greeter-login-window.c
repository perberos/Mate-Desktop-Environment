/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
 * Copyright (C) 2008 Red Hat, Inc.
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

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <errno.h>
#include <pwd.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <glib-object.h>

#include <gdk/gdkkeysyms.h>
#include <gdk/gdkx.h>
#include <X11/XKBlib.h>

#include <gtk/gtk.h>

#include <mateconf/mateconf-client.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "mdm-settings-client.h"
#include "mdm-settings-keys.h"
#include "mdm-profile.h"

#include "mdm-greeter-login-window.h"
#include "mdm-user-chooser-widget.h"

#ifdef HAVE_PAM
#include <security/pam_appl.h>
#define PW_ENTRY_SIZE PAM_MAX_RESP_SIZE
#else
#define PW_ENTRY_SIZE MDM_MAX_PASS
#endif

#define CK_NAME      "org.freedesktop.ConsoleKit"
#define CK_PATH      "/org/freedesktop/ConsoleKit"
#define CK_INTERFACE "org.freedesktop.ConsoleKit"

#define CK_MANAGER_PATH      "/org/freedesktop/ConsoleKit/Manager"
#define CK_MANAGER_INTERFACE "org.freedesktop.ConsoleKit.Manager"
#define CK_SEAT_INTERFACE    "org.freedesktop.ConsoleKit.Seat"
#define CK_SESSION_INTERFACE "org.freedesktop.ConsoleKit.Session"

#define UI_XML_FILE       "mdm-greeter-login-window.ui"

#define KEY_GREETER_DIR             "/apps/mdm/simple-greeter"
#define KEY_BANNER_MESSAGE_ENABLED  KEY_GREETER_DIR "/banner_message_enable"
#define KEY_BANNER_MESSAGE_TEXT     KEY_GREETER_DIR "/banner_message_text"
#define KEY_BANNER_MESSAGE_TEXT_NOCHOOSER     KEY_GREETER_DIR "/banner_message_text_nochooser"
#define KEY_LOGO                    KEY_GREETER_DIR "/logo_icon_name"
#define KEY_DISABLE_USER_LIST       "/apps/mdm/simple-greeter/disable_user_list"

#define LSB_RELEASE_COMMAND "lsb_release -d"

#define MDM_GREETER_LOGIN_WINDOW_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_GREETER_LOGIN_WINDOW, MdmGreeterLoginWindowPrivate))

enum {
        MODE_UNDEFINED = 0,
        MODE_TIMED_LOGIN,
        MODE_SELECTION,
        MODE_AUTHENTICATION,
};

enum {
        LOGIN_BUTTON_HIDDEN = 0,
        LOGIN_BUTTON_ANSWER_QUERY,
        LOGIN_BUTTON_TIMED_LOGIN
};

struct MdmGreeterLoginWindowPrivate
{
        GtkBuilder      *builder;
        GtkWidget       *user_chooser;
        GtkWidget       *auth_banner_label;
        GtkWidget       *current_button;

        guint            display_is_local : 1;
        guint            is_interactive : 1;
        guint            user_chooser_loaded : 1;
        MateConfClient     *client;

        gboolean         banner_message_enabled;
        guint            mateconf_cnxn;

        guint            last_mode;
        guint            dialog_mode;

        gboolean         user_list_disabled;
        guint            num_queries;

        gboolean         timed_login_already_enabled;
        gboolean         timed_login_enabled;
        guint            timed_login_delay;
        char            *timed_login_username;
        guint            timed_login_timeout_id;

        guint            login_button_handler_id;
        guint            start_session_handler_id;
};

enum {
        PROP_0,
        PROP_DISPLAY_IS_LOCAL,
        PROP_IS_INTERACTIVE,
};

enum {
        BEGIN_AUTO_LOGIN,
        BEGIN_VERIFICATION,
        BEGIN_VERIFICATION_FOR_USER,
        QUERY_ANSWER,
        START_SESSION,
        USER_SELECTED,
        DISCONNECTED,
        CANCELLED,
        LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0, };

static void     mdm_greeter_login_window_class_init   (MdmGreeterLoginWindowClass *klass);
static void     mdm_greeter_login_window_init         (MdmGreeterLoginWindow      *greeter_login_window);
static void     mdm_greeter_login_window_finalize     (GObject                    *object);

static void     restart_timed_login_timeout (MdmGreeterLoginWindow *login_window);
static void     on_user_unchosen            (MdmUserChooserWidget *user_chooser,
                                             MdmGreeterLoginWindow *login_window);

static void     switch_mode                 (MdmGreeterLoginWindow *login_window,
                                             int                    number);
static void     update_banner_message       (MdmGreeterLoginWindow *login_window);

G_DEFINE_TYPE (MdmGreeterLoginWindow, mdm_greeter_login_window, GTK_TYPE_WINDOW)

static void
set_busy (MdmGreeterLoginWindow *login_window)
{
        GdkCursor *cursor;

        cursor = gdk_cursor_new (GDK_WATCH);
        gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (login_window)), cursor);
        gdk_cursor_unref (cursor);
}

static void
set_ready (MdmGreeterLoginWindow *login_window)
{
        gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (login_window)), NULL);
}

static void
set_sensitive (MdmGreeterLoginWindow *login_window,
               gboolean               sensitive)
{
        GtkWidget *box;

        box = GTK_WIDGET (gtk_builder_get_object (login_window->priv->builder, "auth-input-box"));
        gtk_widget_set_sensitive (box, sensitive);

        box = GTK_WIDGET (gtk_builder_get_object (login_window->priv->builder, "buttonbox"));
        gtk_widget_set_sensitive (box, sensitive);

        gtk_widget_set_sensitive (login_window->priv->user_chooser, sensitive);
}

static void
set_focus (MdmGreeterLoginWindow *login_window)
{
        GtkWidget *entry;

        entry = GTK_WIDGET (gtk_builder_get_object (MDM_GREETER_LOGIN_WINDOW (login_window)->priv->builder, "auth-prompt-entry"));

        gdk_window_focus (gtk_widget_get_window (GTK_WIDGET (login_window)), GDK_CURRENT_TIME);

        if (gtk_widget_get_realized (entry) && ! gtk_widget_has_focus (entry)) {
                gtk_widget_grab_focus (entry);
        } else if (gtk_widget_get_realized (login_window->priv->user_chooser) && ! gtk_widget_has_focus (login_window->priv->user_chooser)) {
                gtk_widget_grab_focus (login_window->priv->user_chooser);
        }
}

static void
set_message (MdmGreeterLoginWindow *login_window,
             const char            *text)
{
        GtkWidget *label;

        label = GTK_WIDGET (gtk_builder_get_object (login_window->priv->builder, "auth-message-label"));
        gtk_label_set_text (GTK_LABEL (label), text);
}

static void
on_user_interaction (MdmGreeterLoginWindow *login_window)
{
        g_debug ("MdmGreeterLoginWindow: user is interacting with session!\n");
        restart_timed_login_timeout (login_window);
}

static GdkFilterReturn
on_xevent (XEvent                *xevent,
           GdkEvent              *event,
           MdmGreeterLoginWindow *login_window)
{
        switch (xevent->xany.type) {
                case KeyPress:
                case KeyRelease:
                case ButtonPress:
                case ButtonRelease:
                        on_user_interaction (login_window);
                        break;
                case  PropertyNotify:
                        if (xevent->xproperty.atom == gdk_x11_get_xatom_by_name ("_NET_WM_USER_TIME")) {
                                on_user_interaction (login_window);
                        }
                        break;

                default:
                        break;
        }

        return GDK_FILTER_CONTINUE;
}

static void
stop_watching_for_user_interaction (MdmGreeterLoginWindow *login_window)
{
        gdk_window_remove_filter (NULL,
                                  (GdkFilterFunc) on_xevent,
                                  login_window);
}

static void
remove_timed_login_timeout (MdmGreeterLoginWindow *login_window)
{
        if (login_window->priv->timed_login_timeout_id > 0) {
                g_debug ("MdmGreeterLoginWindow: removing timed login timer");
                g_source_remove (login_window->priv->timed_login_timeout_id);
                login_window->priv->timed_login_timeout_id = 0;
        }

        stop_watching_for_user_interaction (login_window);
}

static void
_mdm_greeter_login_window_set_interactive (MdmGreeterLoginWindow *login_window,
                                           gboolean               is_interactive)
{
        if (login_window->priv->is_interactive != is_interactive) {
                login_window->priv->is_interactive = is_interactive;
                g_object_notify (G_OBJECT (login_window), "is-interactive");
        }
}

static gboolean
timed_login_timer (MdmGreeterLoginWindow *login_window)
{
        set_sensitive (login_window, FALSE);
        set_message (login_window, _("Automatically logging in…"));

        g_debug ("MdmGreeterLoginWindow: timer expired");
        _mdm_greeter_login_window_set_interactive (login_window, TRUE);
        login_window->priv->timed_login_timeout_id = 0;

        return FALSE;
}

static void
watch_for_user_interaction (MdmGreeterLoginWindow *login_window)
{
        gdk_window_add_filter (NULL,
                               (GdkFilterFunc) on_xevent,
                               login_window);
}

static void
restart_timed_login_timeout (MdmGreeterLoginWindow *login_window)
{
        remove_timed_login_timeout (login_window);

        if (login_window->priv->timed_login_enabled) {
                g_debug ("MdmGreeterLoginWindow: adding timed login timer");
                watch_for_user_interaction (login_window);
                login_window->priv->timed_login_timeout_id = g_timeout_add_seconds (login_window->priv->timed_login_delay,
                                                                                    (GSourceFunc)timed_login_timer,
                                                                                    login_window);

                mdm_chooser_widget_set_item_timer (MDM_CHOOSER_WIDGET (login_window->priv->user_chooser),
                                                   MDM_USER_CHOOSER_USER_AUTO,
                                                   login_window->priv->timed_login_delay * 1000);
        }
}

static void
show_widget (MdmGreeterLoginWindow *login_window,
             const char            *name,
             gboolean               visible)
{
        GtkWidget *widget;

        widget = GTK_WIDGET (gtk_builder_get_object (login_window->priv->builder, name));
        if (widget != NULL) {
                if (visible) {
                        gtk_widget_show (widget);
                } else {
                        gtk_widget_hide (widget);
                }
        }
}

static void
on_login_button_clicked_answer_query (GtkButton             *button,
                                      MdmGreeterLoginWindow *login_window)
{
        GtkWidget  *entry;
        const char *text;

        set_busy (login_window);
        set_sensitive (login_window, FALSE);

        entry = GTK_WIDGET (gtk_builder_get_object (login_window->priv->builder, "auth-prompt-entry"));
        text = gtk_entry_get_text (GTK_ENTRY (entry));

        _mdm_greeter_login_window_set_interactive (login_window, TRUE);
        g_signal_emit (login_window, signals[QUERY_ANSWER], 0, text);
}

static void
on_login_button_clicked_timed_login (GtkButton             *button,
                                     MdmGreeterLoginWindow *login_window)
{
        set_busy (login_window);
        set_sensitive (login_window, FALSE);

        _mdm_greeter_login_window_set_interactive (login_window, TRUE);
}

static void
set_log_in_button_mode (MdmGreeterLoginWindow *login_window,
                        int                    mode)
{
        GtkWidget *button;
        GtkWidget *login_button;
        GtkWidget *unlock_button;
        char      *item;
        gboolean   in_use;

        in_use = FALSE;
        item = mdm_chooser_widget_get_active_item (MDM_CHOOSER_WIDGET (login_window->priv->user_chooser));
        if (item != NULL) {
                gboolean res;

                res = mdm_chooser_widget_lookup_item (MDM_CHOOSER_WIDGET (login_window->priv->user_chooser),
                                                      item,
                                                      NULL, /* image */
                                                      NULL, /* name */
                                                      NULL, /* comment */
                                                      NULL, /* priority */
                                                      &in_use,
                                                      NULL); /* is separate */
        }

        if (login_window->priv->current_button != NULL) {
                /* disconnect any signals */
                if (login_window->priv->login_button_handler_id > 0) {
                        g_signal_handler_disconnect (login_window->priv->current_button,
                                                     login_window->priv->login_button_handler_id);
                        login_window->priv->login_button_handler_id = 0;
                }
        }

        unlock_button = GTK_WIDGET (gtk_builder_get_object (login_window->priv->builder, "unlock-button"));
        login_button = GTK_WIDGET (gtk_builder_get_object (login_window->priv->builder, "log-in-button"));

        if (in_use) {
                gtk_widget_hide (login_button);
                button = unlock_button;
        } else {
                gtk_widget_hide (unlock_button);
                button = login_button;
        }
        gtk_widget_grab_default (button);

        login_window->priv->current_button = button;

        switch (mode) {
        case LOGIN_BUTTON_HIDDEN:
                gtk_widget_hide (button);
                break;
        case LOGIN_BUTTON_ANSWER_QUERY:
                login_window->priv->login_button_handler_id = g_signal_connect (button, "clicked", G_CALLBACK (on_login_button_clicked_answer_query), login_window);
                gtk_widget_show (button);
                break;
        case LOGIN_BUTTON_TIMED_LOGIN:
                login_window->priv->login_button_handler_id = g_signal_connect (button, "clicked", G_CALLBACK (on_login_button_clicked_timed_login), login_window);
                gtk_widget_show (button);
                break;
        default:
                g_assert_not_reached ();
                break;
        }
}

static gboolean
user_chooser_has_no_user (MdmGreeterLoginWindow *login_window)
{
        guint num_items;

        num_items = mdm_chooser_widget_get_number_of_items (MDM_CHOOSER_WIDGET (login_window->priv->user_chooser));
        g_debug ("MdmGreeterLoginWindow: loaded=%d num_items=%d",
                 login_window->priv->user_chooser_loaded,
                 num_items);
        return (login_window->priv->user_chooser_loaded && num_items == 0);
}

static void
maybe_show_cancel_button (MdmGreeterLoginWindow *login_window)
{
        gboolean show;

        show = FALSE;

        /* only show the cancel button if there is something to go
           back to */

        switch (login_window->priv->dialog_mode) {
        case MODE_SELECTION:
                /* should never have anything to return to from here */
                show = FALSE;
                break;
        case MODE_TIMED_LOGIN:
                /* should always have something to return to from here */
                show = TRUE;
                break;
        case MODE_AUTHENTICATION:
                if (login_window->priv->num_queries > 1) {
                        /* if we are inside a pam conversation past
                           the first step */
                        show = TRUE;
                } else {
                        if (login_window->priv->user_list_disabled || user_chooser_has_no_user (login_window)) {
                                show = FALSE;
                        } else {
                                show = TRUE;
                        }
                }
                break;
        default:
                g_assert_not_reached ();
        }

        show_widget (login_window, "cancel-button", show);
}

static void
switch_mode (MdmGreeterLoginWindow *login_window,
             int                    number)
{
        const char *default_name;
        GtkWidget  *box;

        /* Should never switch to MODE_UNDEFINED */
        g_assert (number != MODE_UNDEFINED);

        /* we want to run this even if we're supposed to
           be in the mode already so that we reset everything
           to a known state */
        if (login_window->priv->dialog_mode != number) {
                login_window->priv->last_mode = login_window->priv->dialog_mode;
                login_window->priv->dialog_mode = number;
        }

        default_name = NULL;

        switch (number) {
        case MODE_SELECTION:
                set_log_in_button_mode (login_window, LOGIN_BUTTON_HIDDEN);
                break;
        case MODE_TIMED_LOGIN:
                set_log_in_button_mode (login_window, LOGIN_BUTTON_TIMED_LOGIN);
                break;
        case MODE_AUTHENTICATION:
                set_log_in_button_mode (login_window, LOGIN_BUTTON_ANSWER_QUERY);
                break;
        default:
                g_assert_not_reached ();
        }

        show_widget (login_window, "auth-input-box", FALSE);
        maybe_show_cancel_button (login_window);

        /*
         * The rest of this function sets up the user list, so just return if
         * the user list is disabled.
         */
        if (login_window->priv->user_list_disabled && number != MODE_TIMED_LOGIN) {
                return;
        }

        box = gtk_widget_get_parent (login_window->priv->user_chooser);
        if (GTK_IS_BOX (box)) {
                guint       padding;
                GtkPackType pack_type;

                gtk_box_query_child_packing (GTK_BOX (box),
                                             login_window->priv->user_chooser,
                                             NULL,
                                             NULL,
                                             &padding,
                                             &pack_type);
                gtk_box_set_child_packing (GTK_BOX (box),
                                           login_window->priv->user_chooser,
                                           number == MODE_SELECTION,
                                           number == MODE_SELECTION,
                                           padding,
                                           pack_type);
        }
}

static void
choose_user (MdmGreeterLoginWindow *login_window,
             const char            *user_name)
{
        guint mode;

        g_assert (user_name != NULL);

        g_signal_emit (G_OBJECT (login_window), signals[USER_SELECTED],
                       0, user_name);

        mode = MODE_AUTHENTICATION;
        if (strcmp (user_name, MDM_USER_CHOOSER_USER_OTHER) == 0) {
                g_signal_emit (login_window, signals[BEGIN_VERIFICATION], 0);
        } else if (strcmp (user_name, MDM_USER_CHOOSER_USER_GUEST) == 0) {
                /* FIXME: handle guest account stuff */
        } else if (strcmp (user_name, MDM_USER_CHOOSER_USER_AUTO) == 0) {
                g_signal_emit (login_window, signals[BEGIN_AUTO_LOGIN], 0,
                               login_window->priv->timed_login_username);

                login_window->priv->timed_login_enabled = TRUE;
                restart_timed_login_timeout (login_window);

                /* just wait for the user to select language and stuff */
                mode = MODE_TIMED_LOGIN;
                set_message (login_window, _("Select language and click Log In"));
        } else {
                g_signal_emit (login_window, signals[BEGIN_VERIFICATION_FOR_USER], 0, user_name);
        }

        switch_mode (login_window, mode);
}

static void
retry_login (MdmGreeterLoginWindow *login_window)
{
        GtkWidget  *entry;
        char       *user_name;

        user_name = mdm_user_chooser_widget_get_chosen_user_name (MDM_USER_CHOOSER_WIDGET (login_window->priv->user_chooser));
        if (user_name == NULL) {
                return;
        }

        g_debug ("MdmGreeterLoginWindow: Retrying login for %s", user_name);

        entry = GTK_WIDGET (gtk_builder_get_object (MDM_GREETER_LOGIN_WINDOW (login_window)->priv->builder, "auth-prompt-entry"));
        gtk_editable_delete_text (GTK_EDITABLE (entry), 0, -1);

        choose_user (login_window, user_name);

        g_free (user_name);
}

static gboolean
can_jump_to_authenticate (MdmGreeterLoginWindow *login_window)
{
        gboolean res;

        if (!login_window->priv->user_chooser_loaded) {
                res = FALSE;
        } else if (login_window->priv->user_list_disabled) {
                res = (login_window->priv->timed_login_username == NULL);
        } else {
                res = user_chooser_has_no_user (login_window);
        }

        return res;
}

static void
reset_dialog (MdmGreeterLoginWindow *login_window,
              guint                  dialog_mode)
{
        GtkWidget  *entry;
        GtkWidget  *label;
        guint       mode;

        g_debug ("MdmGreeterLoginWindow: Resetting dialog to mode %u", dialog_mode);
        set_busy (login_window);
        set_sensitive (login_window, FALSE);

        login_window->priv->num_queries = 0;

        if (dialog_mode == MODE_SELECTION) {
                if (login_window->priv->timed_login_enabled) {
                        mdm_chooser_widget_set_item_timer (MDM_CHOOSER_WIDGET (login_window->priv->user_chooser),
                                                           MDM_USER_CHOOSER_USER_AUTO, 0);
                        remove_timed_login_timeout (login_window);
                        login_window->priv->timed_login_enabled = FALSE;
                }
                _mdm_greeter_login_window_set_interactive (login_window, FALSE);

                g_signal_handlers_block_by_func (G_OBJECT (login_window->priv->user_chooser),
                                                 G_CALLBACK (on_user_unchosen), login_window);
                mdm_user_chooser_widget_set_chosen_user_name (MDM_USER_CHOOSER_WIDGET (login_window->priv->user_chooser), NULL);
                g_signal_handlers_unblock_by_func (G_OBJECT (login_window->priv->user_chooser),
                                                   G_CALLBACK (on_user_unchosen), login_window);

                if (login_window->priv->start_session_handler_id > 0) {
                        g_signal_handler_disconnect (login_window, login_window->priv->start_session_handler_id);
                        login_window->priv->start_session_handler_id = 0;
                }

                set_message (login_window, "");
        }

        entry = GTK_WIDGET (gtk_builder_get_object (MDM_GREETER_LOGIN_WINDOW (login_window)->priv->builder, "auth-prompt-entry"));

        gtk_editable_delete_text (GTK_EDITABLE (entry), 0, -1);

        gtk_entry_set_visibility (GTK_ENTRY (entry), TRUE);

        label = GTK_WIDGET (gtk_builder_get_object (MDM_GREETER_LOGIN_WINDOW (login_window)->priv->builder, "auth-prompt-label"));
        gtk_label_set_text (GTK_LABEL (label), "");

        if (can_jump_to_authenticate (login_window)) {
                /* If we don't have a user list jump straight to authenticate */
                g_debug ("MdmGreeterLoginWindow: jumping straight to authenticate");
                switch_mode (login_window, MODE_AUTHENTICATION);

                g_debug ("Starting PAM conversation since no local users");
                g_signal_emit (G_OBJECT (login_window), signals[USER_SELECTED],
                               0, MDM_USER_CHOOSER_USER_OTHER);
                g_signal_emit (login_window, signals[BEGIN_VERIFICATION], 0);
        } else {
                switch_mode (login_window, dialog_mode);
        }

        set_sensitive (login_window, TRUE);
        set_ready (login_window);
        set_focus (MDM_GREETER_LOGIN_WINDOW (login_window));
        update_banner_message (login_window);

        if (mdm_chooser_widget_get_number_of_items (MDM_CHOOSER_WIDGET (login_window->priv->user_chooser)) >= 1) {
                mdm_chooser_widget_propagate_pending_key_events (MDM_CHOOSER_WIDGET (login_window->priv->user_chooser));
        }
}

static void
do_cancel (MdmGreeterLoginWindow *login_window)
{
        /* need to wait for response from backend */
        set_message (login_window, _("Cancelling…"));
        set_busy (login_window);
        set_sensitive (login_window, FALSE);
        g_signal_emit (login_window, signals[CANCELLED], 0);
}

gboolean
mdm_greeter_login_window_ready (MdmGreeterLoginWindow *login_window)
{
        g_return_val_if_fail (MDM_IS_GREETER_LOGIN_WINDOW (login_window), FALSE);

        set_sensitive (MDM_GREETER_LOGIN_WINDOW (login_window), TRUE);
        set_ready (MDM_GREETER_LOGIN_WINDOW (login_window));
        set_focus (MDM_GREETER_LOGIN_WINDOW (login_window));

        /* If we are retrying a previously selected user */
        if (!login_window->priv->user_list_disabled &&
            login_window->priv->dialog_mode == MODE_AUTHENTICATION) {
                retry_login (login_window);
        } else {
                /* If the user list is disabled, then start the PAM conversation */
                if (can_jump_to_authenticate (login_window)) {
                        g_debug ("Starting PAM conversation since user list disabled");
                        g_signal_emit (G_OBJECT (login_window), signals[USER_SELECTED],
                                       0, MDM_USER_CHOOSER_USER_OTHER);
                        g_signal_emit (login_window, signals[BEGIN_VERIFICATION], 0);
                }
        }

        return TRUE;
}

gboolean
mdm_greeter_login_window_authentication_failed (MdmGreeterLoginWindow *login_window)
{
        g_return_val_if_fail (MDM_IS_GREETER_LOGIN_WINDOW (login_window), FALSE);

        g_debug ("MdmGreeterLoginWindow: got authentication failed");

        /* FIXME: shake? */
        reset_dialog (login_window, MODE_AUTHENTICATION);

        return TRUE;
}

gboolean
mdm_greeter_login_window_reset (MdmGreeterLoginWindow *login_window)
{
        g_return_val_if_fail (MDM_IS_GREETER_LOGIN_WINDOW (login_window), FALSE);

        g_debug ("MdmGreeterLoginWindow: got reset");
        reset_dialog (login_window, MODE_SELECTION);

        return TRUE;
}

gboolean
mdm_greeter_login_window_info (MdmGreeterLoginWindow *login_window,
                               const char            *text)
{
        g_return_val_if_fail (MDM_IS_GREETER_LOGIN_WINDOW (login_window), FALSE);

        g_debug ("MdmGreeterLoginWindow: info: %s", text);

        set_message (MDM_GREETER_LOGIN_WINDOW (login_window), text);

        return TRUE;
}

gboolean
mdm_greeter_login_window_problem (MdmGreeterLoginWindow *login_window,
                                  const char            *text)
{
        g_return_val_if_fail (MDM_IS_GREETER_LOGIN_WINDOW (login_window), FALSE);

        g_debug ("MdmGreeterLoginWindow: problem: %s", text);

        set_message (MDM_GREETER_LOGIN_WINDOW (login_window), text);
        gdk_window_beep (gtk_widget_get_window (GTK_WIDGET (login_window)));

        return TRUE;
}

static void
request_timed_login (MdmGreeterLoginWindow *login_window)
{
        g_debug ("MdmGreeterLoginWindow: requesting timed login");

        gtk_widget_show (login_window->priv->user_chooser);

        if (login_window->priv->dialog_mode != MODE_SELECTION) {
                reset_dialog (login_window, MODE_SELECTION);
        }

        if (!login_window->priv->timed_login_already_enabled) {
                mdm_user_chooser_widget_set_chosen_user_name (MDM_USER_CHOOSER_WIDGET (login_window->priv->user_chooser),
                                                              MDM_USER_CHOOSER_USER_AUTO);
        }

        login_window->priv->timed_login_already_enabled = TRUE;
}

void
mdm_greeter_login_window_request_timed_login (MdmGreeterLoginWindow *login_window,
                                              const char            *username,
                                              int                    delay)
{
        g_return_if_fail (MDM_IS_GREETER_LOGIN_WINDOW (login_window));

        g_debug ("MdmGreeterLoginWindow: requested automatic login for user '%s' in %d seconds", username, delay);

        g_free (login_window->priv->timed_login_username);
        login_window->priv->timed_login_username = g_strdup (username);
        login_window->priv->timed_login_delay = delay;

        /* add the auto user right away so we won't trigger a mode
           switch to authenticate when the user list is disabled */
        mdm_user_chooser_widget_set_show_user_auto (MDM_USER_CHOOSER_WIDGET (login_window->priv->user_chooser), TRUE);

        /* if the users aren't loaded then we'll handle it in when they are */
        if (login_window->priv->user_chooser_loaded) {
                g_debug ("Handling timed login request since users are already loaded.");
                request_timed_login (login_window);
        } else {
                g_debug ("Waiting to handle timed login request until users are loaded.");
        }
}

static void
mdm_greeter_login_window_start_session_when_ready (MdmGreeterLoginWindow *login_window)
{
        if (login_window->priv->is_interactive) {
                g_debug ("MdmGreeterLoginWindow: starting session");
                g_signal_emit (login_window, signals[START_SESSION], 0);
        } else {
                g_debug ("MdmGreeterLoginWindow: not starting session since "
                         "user hasn't had an opportunity to pick language "
                         "and session yet.");

                /* Call back when we're ready to go
                 */
                login_window->priv->start_session_handler_id =
                    g_signal_connect (login_window, "notify::is-interactive",
                                      G_CALLBACK (mdm_greeter_login_window_start_session_when_ready),
                                      NULL);

                /* FIXME: If the user wasn't asked any questions by pam but
                 * pam still authorized them (passwd -d, or the questions got
                 * asked on an external device) then we need to let them log in.
                 * Right now we just log them in right away, but we really should
                 * set a timer up like timed login (but shorter, say ~5 seconds),
                 * so they can pick language/session.  Will need to refactor things
                 * a bit so we can share code with timed login.
                 */
                if (!login_window->priv->timed_login_enabled) {

                        g_debug ("MdmGreeterLoginWindow: Okay, we'll start the session anyway,"
                                 "because the user isn't ever going to get an opportunity to"
                                 "interact with session");
                        _mdm_greeter_login_window_set_interactive (login_window, TRUE);
                }

        }
}

gboolean
mdm_greeter_login_window_info_query (MdmGreeterLoginWindow *login_window,
                                     const char            *text)
{
        GtkWidget  *entry;
        GtkWidget  *label;

        g_return_val_if_fail (MDM_IS_GREETER_LOGIN_WINDOW (login_window), FALSE);

        login_window->priv->num_queries++;
        maybe_show_cancel_button (login_window);

        g_debug ("MdmGreeterLoginWindow: info query: %s", text);

        entry = GTK_WIDGET (gtk_builder_get_object (MDM_GREETER_LOGIN_WINDOW (login_window)->priv->builder, "auth-prompt-entry"));
        gtk_editable_delete_text (GTK_EDITABLE (entry), 0, -1);
        gtk_entry_set_visibility (GTK_ENTRY (entry), TRUE);
        set_log_in_button_mode (login_window, LOGIN_BUTTON_ANSWER_QUERY);

        label = GTK_WIDGET (gtk_builder_get_object (MDM_GREETER_LOGIN_WINDOW (login_window)->priv->builder, "auth-prompt-label"));
        gtk_label_set_text (GTK_LABEL (label), text);

        show_widget (login_window, "auth-input-box", TRUE);
        set_sensitive (MDM_GREETER_LOGIN_WINDOW (login_window), TRUE);
        set_ready (MDM_GREETER_LOGIN_WINDOW (login_window));
        set_focus (MDM_GREETER_LOGIN_WINDOW (login_window));

        mdm_chooser_widget_propagate_pending_key_events (MDM_CHOOSER_WIDGET (login_window->priv->user_chooser));

        return TRUE;
}

gboolean
mdm_greeter_login_window_secret_info_query (MdmGreeterLoginWindow *login_window,
                                            const char            *text)
{
        GtkWidget  *entry;
        GtkWidget  *label;

        g_return_val_if_fail (MDM_IS_GREETER_LOGIN_WINDOW (login_window), FALSE);

        login_window->priv->num_queries++;
        maybe_show_cancel_button (login_window);

        entry = GTK_WIDGET (gtk_builder_get_object (MDM_GREETER_LOGIN_WINDOW (login_window)->priv->builder, "auth-prompt-entry"));
        gtk_editable_delete_text (GTK_EDITABLE (entry), 0, -1);
        gtk_entry_set_visibility (GTK_ENTRY (entry), FALSE);
        set_log_in_button_mode (login_window, LOGIN_BUTTON_ANSWER_QUERY);

        label = GTK_WIDGET (gtk_builder_get_object (MDM_GREETER_LOGIN_WINDOW (login_window)->priv->builder, "auth-prompt-label"));
        gtk_label_set_text (GTK_LABEL (label), text);

        show_widget (login_window, "auth-input-box", TRUE);
        set_sensitive (MDM_GREETER_LOGIN_WINDOW (login_window), TRUE);
        set_ready (MDM_GREETER_LOGIN_WINDOW (login_window));
        set_focus (MDM_GREETER_LOGIN_WINDOW (login_window));

        mdm_chooser_widget_propagate_pending_key_events (MDM_CHOOSER_WIDGET (login_window->priv->user_chooser));

        return TRUE;
}

void
mdm_greeter_login_window_user_authorized (MdmGreeterLoginWindow *login_window)
{
        g_return_if_fail (MDM_IS_GREETER_LOGIN_WINDOW (login_window));

        g_debug ("MdmGreeterLoginWindow: user now authorized");

        mdm_greeter_login_window_start_session_when_ready (login_window);
}

static void
_mdm_greeter_login_window_set_display_is_local (MdmGreeterLoginWindow *login_window,
                                                gboolean               is)
{
        login_window->priv->display_is_local = is;
}

static void
mdm_greeter_login_window_set_property (GObject      *object,
                                       guint         prop_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
        MdmGreeterLoginWindow *self;

        self = MDM_GREETER_LOGIN_WINDOW (object);

        switch (prop_id) {
        case PROP_DISPLAY_IS_LOCAL:
                _mdm_greeter_login_window_set_display_is_local (self, g_value_get_boolean (value));
                break;
        case PROP_IS_INTERACTIVE:
                _mdm_greeter_login_window_set_interactive (self, g_value_get_boolean (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
mdm_greeter_login_window_get_property (GObject    *object,
                                       guint       prop_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
        MdmGreeterLoginWindow *self;

        self = MDM_GREETER_LOGIN_WINDOW (object);

        switch (prop_id) {
        case PROP_DISPLAY_IS_LOCAL:
                g_value_set_boolean (value, self->priv->display_is_local);
                break;
        case PROP_IS_INTERACTIVE:
                g_value_set_boolean (value, self->priv->is_interactive);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
cancel_button_clicked (GtkButton             *button,
                       MdmGreeterLoginWindow *login_window)
{
        do_cancel (login_window);
}

static void
on_user_chooser_visibility_changed (MdmGreeterLoginWindow *login_window)
{
        g_debug ("MdmGreeterLoginWindow: Chooser visibility changed");
        update_banner_message (login_window);
}

static void
on_users_loaded (MdmUserChooserWidget  *user_chooser,
                 MdmGreeterLoginWindow *login_window)
{
        g_debug ("MdmGreeterLoginWindow: users loaded");
        login_window->priv->user_chooser_loaded = TRUE;

        update_banner_message (login_window);

        if (!login_window->priv->user_list_disabled) {
                gtk_widget_show (login_window->priv->user_chooser);
        }

        if (login_window->priv->timed_login_username != NULL
            && !login_window->priv->timed_login_already_enabled) {
                request_timed_login (login_window);
        } else if (can_jump_to_authenticate (login_window)) {
                /* jump straight to authenticate */
                g_debug ("MdmGreeterLoginWindow: jumping straight to authenticate");

                switch_mode (login_window, MODE_AUTHENTICATION);

                g_debug ("Starting PAM conversation since no local users");
                g_signal_emit (G_OBJECT (login_window), signals[USER_SELECTED],
                               0, MDM_USER_CHOOSER_USER_OTHER);
                g_signal_emit (login_window, signals[BEGIN_VERIFICATION], 0);
        }
}

static void
on_user_chosen (MdmUserChooserWidget  *user_chooser,
                MdmGreeterLoginWindow *login_window)
{
        char *user_name;
        guint mode;

        user_name = mdm_user_chooser_widget_get_chosen_user_name (MDM_USER_CHOOSER_WIDGET (login_window->priv->user_chooser));
        g_debug ("MdmGreeterLoginWindow: user chosen '%s'", user_name);

        if (user_name == NULL) {
                return;
        }

        choose_user (login_window, user_name);
        g_free (user_name);
}

static void
on_user_unchosen (MdmUserChooserWidget  *user_chooser,
                  MdmGreeterLoginWindow *login_window)
{
        do_cancel (login_window);
}

static void
rotate_computer_info (MdmGreeterLoginWindow *login_window)
{
        GtkWidget *notebook;
        int        current_page;
        int        n_pages;

        /* switch page */
        notebook = GTK_WIDGET (gtk_builder_get_object (login_window->priv->builder, "computer-info-notebook"));
        current_page = gtk_notebook_get_current_page (GTK_NOTEBOOK (notebook));
        n_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (notebook));

        if (current_page + 1 < n_pages) {
                gtk_notebook_next_page (GTK_NOTEBOOK (notebook));
        } else {
                gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), 0);
        }

}

static gboolean
on_computer_info_label_button_press (GtkWidget             *widget,
                                     GdkEventButton        *event,
                                     MdmGreeterLoginWindow *login_window)
{
        rotate_computer_info (login_window);
        return FALSE;
}

static char *
file_read_one_line (const char *filename)
{
        FILE *f;
        char *line;
        char buf[4096];

        line = NULL;

        f = fopen (filename, "r");
        if (f == NULL) {
                g_warning ("Unable to open file %s: %s", filename, g_strerror (errno));
                goto out;
        }

        if (fgets (buf, sizeof (buf), f) == NULL) {
                g_warning ("Unable to read from file %s", filename);
                goto out;
        }

        line = g_strdup (buf);
        g_strchomp (line);

 out:
        fclose (f);

        return line;
}

static const char *known_etc_info_files [] = {
        "redhat-release",
        "SuSE-release",
        "gentoo-release",
        "arch-release",
        "debian_version",
        "mandriva-release",
        "slackware-version",
        "system-release",
        NULL
};


static char *
get_system_version (void)
{
        char *version;
        char *output;
        int i;

        version = NULL;

        output = NULL;
        if (g_spawn_command_line_sync (LSB_RELEASE_COMMAND, &output, NULL, NULL, NULL)) {
                if (g_str_has_prefix (output, "Description:")) {
                        version = g_strdup (output + strlen ("Description:"));
                } else {
                        version = g_strdup (output);
                }
                version = g_strstrip (version);

                /* lsb_release returns (none) if it doesn't know,
                 * so return NULL in that case */
                if (strcmp (version, "(none)") == 0) {
                        g_free (version);
                        version = NULL;
                }

                g_free (output);

                goto out;
        }

        for (i = 0; known_etc_info_files [i]; i++) {
                char *path1;
                char *path2;

                path1 = g_build_filename (SYSCONFDIR, known_etc_info_files [i], NULL);
                path2 = g_build_filename ("/etc", known_etc_info_files [i], NULL);
                if (g_access (path1, R_OK) == 0) {
                        version = file_read_one_line (path1);
                } else if (g_access (path2, R_OK) == 0) {
                        version = file_read_one_line (path2);
                }
                g_free (path2);
                g_free (path1);
                if (version != NULL) {
                        break;
                }
        }

        if (version == NULL) {
                output = NULL;
                if (g_spawn_command_line_sync ("uname -sr", &output, NULL, NULL, NULL)) {
                        version = g_strchomp (output);
                }
        }
 out:
        return version;
}

static void
create_computer_info (MdmGreeterLoginWindow *login_window)
{
        GtkWidget *label;

        mdm_profile_start (NULL);

        label = GTK_WIDGET (gtk_builder_get_object (login_window->priv->builder, "computer-info-name-label"));
        if (label != NULL) {
                char localhost[HOST_NAME_MAX + 1] = "";

                if (gethostname (localhost, HOST_NAME_MAX) == 0) {
                        gtk_label_set_text (GTK_LABEL (label), localhost);
                }

                /* If this isn't actually unique identifier for the computer, then
                 * don't bother showing it by default.
                 */
                if (strcmp (localhost, "localhost") == 0 ||
                    strcmp (localhost, "localhost.localdomain") == 0) {

                    rotate_computer_info (login_window);
                }
        }

        label = GTK_WIDGET (gtk_builder_get_object (login_window->priv->builder, "computer-info-version-label"));
        if (label != NULL) {
                char *version;
                version = get_system_version ();
                gtk_label_set_text (GTK_LABEL (label), version);
                g_free (version);
        }

        mdm_profile_end (NULL);
}

#define INVISIBLE_CHAR_DEFAULT       '*'
#define INVISIBLE_CHAR_BLACK_CIRCLE  0x25cf
#define INVISIBLE_CHAR_WHITE_BULLET  0x25e6
#define INVISIBLE_CHAR_BULLET        0x2022
#define INVISIBLE_CHAR_NONE          0


static void
load_theme (MdmGreeterLoginWindow *login_window)
{
        GtkWidget *entry;
        GtkWidget *button;
        GtkWidget *box;
        GtkWidget *image;
        GError* error = NULL;

        mdm_profile_start (NULL);

        login_window->priv->builder = gtk_builder_new ();
        if (!gtk_builder_add_from_file (login_window->priv->builder, UIDIR "/" UI_XML_FILE, &error)) {
                g_warning ("Couldn't load builder file: %s", error->message);
                g_error_free (error);
        }

        g_assert (login_window->priv->builder != NULL);

        image = GTK_WIDGET (gtk_builder_get_object (login_window->priv->builder, "logo-image"));
        if (image != NULL) {
                char        *icon_name;
                GError      *error;

                error = NULL;
                icon_name = mateconf_client_get_string (login_window->priv->client, KEY_LOGO, &error);
                if (error != NULL) {
                        g_debug ("MdmGreeterLoginWindow: unable to get logo icon name: %s", error->message);
                        g_error_free (error);
                }

                g_debug ("MdmGreeterLoginWindow: Got greeter logo '%s'",
                          icon_name ? icon_name : "(null)");
                if (icon_name != NULL) {
                        gtk_image_set_from_icon_name (GTK_IMAGE (image),
                                                      icon_name,
                                                      GTK_ICON_SIZE_DIALOG);
                        g_free (icon_name);
                }
        }

        box = GTK_WIDGET (gtk_builder_get_object (login_window->priv->builder, "window-frame"));
        gtk_container_add (GTK_CONTAINER (login_window), box);

        /* FIXME: user chooser should implement GtkBuildable and this should get dropped
         */
        login_window->priv->user_chooser = mdm_user_chooser_widget_new ();
        box = GTK_WIDGET (gtk_builder_get_object (login_window->priv->builder, "selection-box"));
        gtk_box_pack_start (GTK_BOX (box), login_window->priv->user_chooser, TRUE, TRUE, 0);
        gtk_box_reorder_child (GTK_BOX (box), login_window->priv->user_chooser, 0);

        mdm_user_chooser_widget_set_show_only_chosen (MDM_USER_CHOOSER_WIDGET (login_window->priv->user_chooser), TRUE);

        g_signal_connect (login_window->priv->user_chooser,
                          "loaded",
                          G_CALLBACK (on_users_loaded),
                          login_window);
        g_signal_connect (login_window->priv->user_chooser,
                          "activated",
                          G_CALLBACK (on_user_chosen),
                          login_window);
        g_signal_connect (login_window->priv->user_chooser,
                          "deactivated",
                          G_CALLBACK (on_user_unchosen),
                          login_window);

        g_signal_connect_swapped (login_window->priv->user_chooser,
                                 "notify::list-visible",
                                 G_CALLBACK (on_user_chooser_visibility_changed),
                                 login_window);

        login_window->priv->auth_banner_label = GTK_WIDGET (gtk_builder_get_object (login_window->priv->builder, "auth-banner-label"));
        /*make_label_small_italic (login_window->priv->auth_banner_label);*/

        button = GTK_WIDGET (gtk_builder_get_object (login_window->priv->builder, "cancel-button"));
        g_signal_connect (button, "clicked", G_CALLBACK (cancel_button_clicked), login_window);

        entry = GTK_WIDGET (gtk_builder_get_object (login_window->priv->builder, "auth-prompt-entry"));
        /* Only change the invisible character if it '*' otherwise assume it is OK */
        if ('*' == gtk_entry_get_invisible_char (GTK_ENTRY (entry))) {
                gunichar invisible_char;
                invisible_char = INVISIBLE_CHAR_BLACK_CIRCLE;
                gtk_entry_set_invisible_char (GTK_ENTRY (entry), invisible_char);
        }

        create_computer_info (login_window);

        box = GTK_WIDGET (gtk_builder_get_object (login_window->priv->builder, "computer-info-event-box"));
        g_signal_connect (box, "button-press-event", G_CALLBACK (on_computer_info_label_button_press), login_window);

        if (login_window->priv->user_list_disabled) {
                switch_mode (login_window, MODE_AUTHENTICATION);
        } else {
                switch_mode (login_window, MODE_SELECTION);
        }

        mdm_profile_end (NULL);
}

static gboolean
mdm_greeter_login_window_key_press_event (GtkWidget   *widget,
                                          GdkEventKey *event)
{
        MdmGreeterLoginWindow *login_window;

        login_window = MDM_GREETER_LOGIN_WINDOW (widget);

        if (event->keyval == GDK_Escape) {
                if (login_window->priv->dialog_mode == MODE_AUTHENTICATION
                    || login_window->priv->dialog_mode == MODE_TIMED_LOGIN) {
                        do_cancel (MDM_GREETER_LOGIN_WINDOW (widget));
                }
        }

        return GTK_WIDGET_CLASS (mdm_greeter_login_window_parent_class)->key_press_event (widget, event);
}

static void
mdm_greeter_login_window_size_request (GtkWidget      *widget,
                                       GtkRequisition *requisition)
{
        int             monitor;
        GdkScreen      *screen;
        GtkRequisition  child_requisition;
        GdkRectangle    area;

        if (GTK_WIDGET_CLASS (mdm_greeter_login_window_parent_class)->size_request) {
                GTK_WIDGET_CLASS (mdm_greeter_login_window_parent_class)->size_request (widget, requisition);
        }

        if (!gtk_widget_get_realized (widget)) {
                return;
        }

        screen = gtk_widget_get_screen (widget);
        monitor = gdk_screen_get_monitor_at_window (screen, gtk_widget_get_window (widget));
        gdk_screen_get_monitor_geometry (screen, monitor, &area);

        gtk_widget_get_child_requisition (gtk_bin_get_child (GTK_BIN (widget)), &child_requisition);
        *requisition = child_requisition;

        guint border_width = gtk_container_get_border_width (GTK_CONTAINER (widget));
        requisition->width += 2 * border_width;
        requisition->height += 2 * border_width;

        /* Make width be at least 33% screen width
         * and height be at most 80% of screen height
         */
        requisition->width = MAX (requisition->width, .33 * area.width);
        requisition->height = MIN (requisition->height, .80 * area.height);

       /* Don't ever shrink window width
        */
        GtkAllocation widget_allocation;
        gtk_widget_get_allocation (widget, &widget_allocation);

        requisition->width = MAX (requisition->width, widget_allocation.width);
}

static void
update_banner_message (MdmGreeterLoginWindow *login_window)
{
        GError      *error;
        gboolean     enabled;

        if (login_window->priv->auth_banner_label == NULL) {
                /* if the theme doesn't have a banner message */
                g_debug ("MdmGreeterLoginWindow: theme doesn't support a banner message");
                return;
        }

        error = NULL;
        enabled = mateconf_client_get_bool (login_window->priv->client, KEY_BANNER_MESSAGE_ENABLED, &error);
        if (error != NULL) {
                g_debug ("MdmGreeterLoginWindow: unable to get configuration: %s", error->message);
                g_error_free (error);
        }

        login_window->priv->banner_message_enabled = enabled;

        if (! enabled) {
                g_debug ("MdmGreeterLoginWindow: banner message disabled");
                gtk_widget_hide (login_window->priv->auth_banner_label);
        } else {
                char *message = NULL;
                error = NULL;
                if (user_chooser_has_no_user (login_window)) {
                        message = mateconf_client_get_string (login_window->priv->client, KEY_BANNER_MESSAGE_TEXT_NOCHOOSER, &error);
                        if (error != NULL) {
                                g_debug("MdmGreeterLoginWindow: unable to get nochooser banner text: %s", error->message);
                                g_error_free(error);
                        }
                }
                error = NULL;
                if (message == NULL) {
                        message = mateconf_client_get_string (login_window->priv->client, KEY_BANNER_MESSAGE_TEXT, &error);
                        if (error != NULL) {
                                g_debug("MdmGreeterLoginWindow: unable to get banner text: %s", error->message);
                                g_error_free(error);
                        }
                }
                if (message != NULL) {
                        char *markup;
                        markup = g_markup_printf_escaped ("<small><i>%s</i></small>", message);
                        gtk_label_set_markup (GTK_LABEL (login_window->priv->auth_banner_label),
                                              markup);
                        g_free (markup);
                }
                g_debug ("MdmGreeterLoginWindow: banner message: %s", message);

                gtk_widget_show (login_window->priv->auth_banner_label);
        }
}

static GObject *
mdm_greeter_login_window_constructor (GType                  type,
                                      guint                  n_construct_properties,
                                      GObjectConstructParam *construct_properties)
{
        MdmGreeterLoginWindow      *login_window;

        mdm_profile_start (NULL);

        login_window = MDM_GREETER_LOGIN_WINDOW (G_OBJECT_CLASS (mdm_greeter_login_window_parent_class)->constructor (type,
                                                                                                                      n_construct_properties,
                                                                                                                      construct_properties));


        load_theme (login_window);
        update_banner_message (login_window);

        mdm_profile_end (NULL);

        return G_OBJECT (login_window);
}

static void
mdm_greeter_login_window_class_init (MdmGreeterLoginWindowClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);
        GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

        object_class->get_property = mdm_greeter_login_window_get_property;
        object_class->set_property = mdm_greeter_login_window_set_property;
        object_class->constructor = mdm_greeter_login_window_constructor;
        object_class->finalize = mdm_greeter_login_window_finalize;

        widget_class->key_press_event = mdm_greeter_login_window_key_press_event;
        widget_class->size_request = mdm_greeter_login_window_size_request;

        signals [BEGIN_AUTO_LOGIN] =
                g_signal_new ("begin-auto-login",
                              G_TYPE_FROM_CLASS (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (MdmGreeterLoginWindowClass, begin_auto_login),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE, 1, G_TYPE_STRING);
        signals [BEGIN_VERIFICATION] =
                g_signal_new ("begin-verification",
                              G_TYPE_FROM_CLASS (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (MdmGreeterLoginWindowClass, begin_verification),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
        signals [BEGIN_VERIFICATION_FOR_USER] =
                g_signal_new ("begin-verification-for-user",
                              G_TYPE_FROM_CLASS (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (MdmGreeterLoginWindowClass, begin_verification_for_user),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1, G_TYPE_STRING);
        signals [QUERY_ANSWER] =
                g_signal_new ("query-answer",
                              G_TYPE_FROM_CLASS (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (MdmGreeterLoginWindowClass, query_answer),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1, G_TYPE_STRING);
        signals [USER_SELECTED] =
                g_signal_new ("user-selected",
                              G_TYPE_FROM_CLASS (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (MdmGreeterLoginWindowClass, user_selected),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1, G_TYPE_STRING);
        signals [CANCELLED] =
                g_signal_new ("cancelled",
                              G_TYPE_FROM_CLASS (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (MdmGreeterLoginWindowClass, cancelled),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
        signals [DISCONNECTED] =
                g_signal_new ("disconnected",
                              G_TYPE_FROM_CLASS (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (MdmGreeterLoginWindowClass, disconnected),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
        signals [START_SESSION] =
                g_signal_new ("start-session",
                              G_TYPE_FROM_CLASS (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (MdmGreeterLoginWindowClass, start_session),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        g_object_class_install_property (object_class,
                                         PROP_DISPLAY_IS_LOCAL,
                                         g_param_spec_boolean ("display-is-local",
                                                               "display is local",
                                                               "display is local",
                                                               FALSE,
                                                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
        g_object_class_install_property (object_class,
                                         PROP_IS_INTERACTIVE,
                                         g_param_spec_boolean ("is-interactive",
                                                               "Is Interactive",
                                                               "Use has had an oppurtunity to interact with window",
                                                               FALSE,
                                                               G_PARAM_READABLE));

        g_type_class_add_private (klass, sizeof (MdmGreeterLoginWindowPrivate));
}

static void
on_mateconf_key_changed (MateConfClient           *client,
                      guint                  cnxn_id,
                      MateConfEntry            *entry,
                      MdmGreeterLoginWindow *login_window)
{
        const char *key;
        MateConfValue *value;

        key = mateconf_entry_get_key (entry);
        value = mateconf_entry_get_value (entry);

        if (strcmp (key, KEY_BANNER_MESSAGE_ENABLED) == 0) {
                if (value->type == MATECONF_VALUE_BOOL) {
                        gboolean enabled;

                        enabled = mateconf_value_get_bool (value);
                        g_debug ("setting key %s = %d", key, enabled);
                        login_window->priv->banner_message_enabled = enabled;
                        update_banner_message (login_window);
                } else {
                        g_warning ("Error retrieving configuration key '%s': Invalid type",
                                   key);
                }
        } else if (strcmp (key, KEY_BANNER_MESSAGE_TEXT) == 0 || strcmp (key, KEY_BANNER_MESSAGE_TEXT_NOCHOOSER) == 0) {
                if (login_window->priv->banner_message_enabled) {
                        update_banner_message (login_window);
                }
        } else {
                g_debug ("MdmGreeterLoginWindow: Config key not handled: %s", key);
        }
}

static gboolean
on_window_state_event (GtkWidget           *widget,
                       GdkEventWindowState *event,
                       gpointer             data)
{
        if (event->changed_mask & GDK_WINDOW_STATE_ICONIFIED) {
                g_debug ("MdmGreeterLoginWindow: window iconified");
                gtk_window_deiconify (GTK_WINDOW (widget));
        }

        return FALSE;
}

static void
mdm_greeter_login_window_init (MdmGreeterLoginWindow *login_window)
{
        MateConfClient *client;
        GError      *error;
        gboolean     user_list_disable;

        mdm_profile_start (NULL);

        login_window->priv = MDM_GREETER_LOGIN_WINDOW_GET_PRIVATE (login_window);
        login_window->priv->timed_login_enabled = FALSE;
        login_window->priv->dialog_mode = MODE_UNDEFINED;

        client = mateconf_client_get_default ();
        error = NULL;

        /* The user list is not shown only if the user list is disabled and
         * timed login is also not being used.
         */
        user_list_disable = mateconf_client_get_bool (client,
                                                   KEY_DISABLE_USER_LIST,
                                                   &error);
        if (error != NULL) {
                g_debug ("MdmUserChooserWidget: unable to get disable-user-list configuration: %s", error->message);
                g_error_free (error);
        }

        login_window->priv->user_list_disabled = user_list_disable;

        gtk_window_set_title (GTK_WINDOW (login_window), _("Login Window"));
        /*gtk_window_set_opacity (GTK_WINDOW (login_window), 0.85);*/
        gtk_window_set_position (GTK_WINDOW (login_window), GTK_WIN_POS_CENTER_ALWAYS);
        gtk_window_set_deletable (GTK_WINDOW (login_window), FALSE);
        gtk_window_set_decorated (GTK_WINDOW (login_window), FALSE);
        gtk_window_set_keep_below (GTK_WINDOW (login_window), TRUE);
        gtk_window_set_skip_taskbar_hint (GTK_WINDOW (login_window), TRUE);
        gtk_window_set_skip_pager_hint (GTK_WINDOW (login_window), TRUE);
        gtk_window_stick (GTK_WINDOW (login_window));
        gtk_container_set_border_width (GTK_CONTAINER (login_window), 0);

        g_signal_connect (login_window,
                          "window-state-event",
                          G_CALLBACK (on_window_state_event),
                          NULL);

        login_window->priv->client = mateconf_client_get_default ();
        mateconf_client_add_dir (login_window->priv->client,
                              KEY_GREETER_DIR,
                              MATECONF_CLIENT_PRELOAD_ONELEVEL,
                              NULL);
        login_window->priv->mateconf_cnxn = mateconf_client_notify_add (login_window->priv->client,
                                                                  KEY_GREETER_DIR,
                                                                  (MateConfClientNotifyFunc)on_mateconf_key_changed,
                                                                  login_window,
                                                                  NULL,
                                                                  NULL);
        mdm_profile_end (NULL);
}

static void
mdm_greeter_login_window_finalize (GObject *object)
{
        MdmGreeterLoginWindow *login_window;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_GREETER_LOGIN_WINDOW (object));

        login_window = MDM_GREETER_LOGIN_WINDOW (object);

        g_return_if_fail (login_window->priv != NULL);

        if (login_window->priv->client != NULL) {
                g_object_unref (login_window->priv->client);
        }

        G_OBJECT_CLASS (mdm_greeter_login_window_parent_class)->finalize (object);
}

GtkWidget *
mdm_greeter_login_window_new (gboolean is_local)
{
        GObject *object;

        object = g_object_new (MDM_TYPE_GREETER_LOGIN_WINDOW,
                               "display-is-local", is_local,
                               "resizable", FALSE,
                               NULL);

        return GTK_WIDGET (object);
}
