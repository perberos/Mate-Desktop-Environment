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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <mateconf/mateconf-client.h>

#include "mdm-greeter-session.h"
#include "mdm-greeter-client.h"
#include "mdm-greeter-panel.h"
#include "mdm-greeter-login-window.h"
#include "mdm-user-chooser-widget.h"

#include "mdm-profile.h"

#define MDM_GREETER_SESSION_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_GREETER_SESSION, MdmGreeterSessionPrivate))

#define MAX_LOGIN_TRIES 3

struct MdmGreeterSessionPrivate
{
        MdmGreeterClient      *client;

        GtkWidget             *login_window;
        GtkWidget             *panel;

        guint                  num_tries;
};

enum {
        PROP_0,
};

static void     mdm_greeter_session_class_init  (MdmGreeterSessionClass *klass);
static void     mdm_greeter_session_init        (MdmGreeterSession      *greeter_session);
static void     mdm_greeter_session_finalize    (GObject                *object);

G_DEFINE_TYPE (MdmGreeterSession, mdm_greeter_session, G_TYPE_OBJECT)

static gpointer session_object = NULL;

static void
on_info (MdmGreeterClient  *client,
         const char        *text,
         MdmGreeterSession *session)
{
        g_debug ("MdmGreeterSession: Info: %s", text);

        mdm_greeter_login_window_info (MDM_GREETER_LOGIN_WINDOW (session->priv->login_window), text);
}

static void
on_problem (MdmGreeterClient  *client,
            const char        *text,
            MdmGreeterSession *session)
{
        g_debug ("MdmGreeterSession: Problem: %s", text);

        mdm_greeter_login_window_problem (MDM_GREETER_LOGIN_WINDOW (session->priv->login_window), text);
}

static void
on_ready (MdmGreeterClient  *client,
          MdmGreeterSession *session)
{
        g_debug ("MdmGreeterSession: Ready");

        mdm_greeter_login_window_ready (MDM_GREETER_LOGIN_WINDOW (session->priv->login_window));
}

static void
on_reset (MdmGreeterClient  *client,
          MdmGreeterSession *session)
{
        g_debug ("MdmGreeterSession: Reset");

        session->priv->num_tries = 0;

        mdm_greeter_panel_reset (MDM_GREETER_PANEL (session->priv->panel));
        mdm_greeter_login_window_reset (MDM_GREETER_LOGIN_WINDOW (session->priv->login_window));
}

static void
on_authentication_failed (MdmGreeterClient  *client,
                          MdmGreeterSession *session)
{
        g_debug ("MdmGreeterSession: Authentication failed");

        session->priv->num_tries++;

        if (session->priv->num_tries < MAX_LOGIN_TRIES) {
                g_debug ("MdmGreeterSession: Retrying login (%d)",
                         session->priv->num_tries);

                mdm_greeter_login_window_authentication_failed (MDM_GREETER_LOGIN_WINDOW (session->priv->login_window));
        } else {
                g_debug ("MdmGreeterSession: Maximum number of login tries exceeded (%d) - resetting",
                         session->priv->num_tries - 1);
                session->priv->num_tries = 0;

                mdm_greeter_panel_reset (MDM_GREETER_PANEL (session->priv->panel));
                mdm_greeter_login_window_reset (MDM_GREETER_LOGIN_WINDOW (session->priv->login_window));
        }
}

static void
show_or_hide_user_options (MdmGreeterSession *session,
                           const char        *username)
{
    if (username != NULL && strcmp (username, MDM_USER_CHOOSER_USER_OTHER) != 0) {
            mdm_greeter_panel_show_user_options (MDM_GREETER_PANEL (session->priv->panel));
    } else {
            mdm_greeter_panel_hide_user_options (MDM_GREETER_PANEL (session->priv->panel));
    }
}

static void
on_selected_user_changed (MdmGreeterClient  *client,
                          const char        *text,
                          MdmGreeterSession *session)
{
        g_debug ("MdmGreeterSession: selected user changed: %s", text);
        show_or_hide_user_options (session, text);
}

static void
on_default_language_name_changed (MdmGreeterClient  *client,
                                  const char        *text,
                                  MdmGreeterSession *session)
{
        g_debug ("MdmGreeterSession: default language name changed: %s", text);
        mdm_greeter_panel_set_default_language_name (MDM_GREETER_PANEL (session->priv->panel),
                                                     text);
}

static void
on_default_layout_name_changed (MdmGreeterClient  *client,
                                const char        *text,
                                MdmGreeterSession *session)
{
        g_debug ("MdmGreeterSession: default layout name changed: %s", text);
        mdm_greeter_panel_set_keyboard_layout (MDM_GREETER_PANEL (session->priv->panel),
                                               text);
}

static void
on_default_session_name_changed (MdmGreeterClient  *client,
                                 const char        *text,
                                 MdmGreeterSession *session)
{
        g_debug ("MdmGreeterSession: default session name changed: %s", text);
        mdm_greeter_panel_set_default_session_name (MDM_GREETER_PANEL (session->priv->panel),
                                                    text);
}

static void
on_timed_login_requested (MdmGreeterClient  *client,
                          const char        *text,
                          int                delay,
                          MdmGreeterSession *session)
{
        g_debug ("MdmGreeterSession: timed login requested for user %s (in %d seconds)", text, delay);
        mdm_greeter_login_window_request_timed_login (MDM_GREETER_LOGIN_WINDOW (session->priv->login_window), text, delay);
}

static void
on_user_authorized (MdmGreeterClient  *client,
                    MdmGreeterSession *session)
{
        g_debug ("MdmGreeterSession: user authorized");
        mdm_greeter_login_window_user_authorized (MDM_GREETER_LOGIN_WINDOW (session->priv->login_window));
}

static void
on_info_query (MdmGreeterClient  *client,
               const char        *text,
               MdmGreeterSession *session)
{
        g_debug ("MdmGreeterSession: Info query: %s", text);

        mdm_greeter_login_window_info_query (MDM_GREETER_LOGIN_WINDOW (session->priv->login_window), text);
}

static void
on_secret_info_query (MdmGreeterClient  *client,
                      const char        *text,
                      MdmGreeterSession *session)
{
        g_debug ("MdmGreeterSession: Secret info query: %s", text);

        mdm_greeter_login_window_secret_info_query (MDM_GREETER_LOGIN_WINDOW (session->priv->login_window), text);
}

static void
on_begin_auto_login (MdmGreeterLoginWindow *login_window,
                     const char            *username,
                     MdmGreeterSession     *session)
{
        mdm_greeter_client_call_begin_auto_login (session->priv->client,
                                                  username);
}

static void
on_begin_verification (MdmGreeterLoginWindow *login_window,
                       MdmGreeterSession     *session)
{
        mdm_greeter_client_call_begin_verification (session->priv->client);
}

static void
on_begin_verification_for_user (MdmGreeterLoginWindow *login_window,
                                const char            *username,
                                MdmGreeterSession     *session)
{
        mdm_greeter_client_call_begin_verification_for_user (session->priv->client,
                                                             username);
}

static void
on_query_answer (MdmGreeterLoginWindow *login_window,
                 const char            *text,
                 MdmGreeterSession     *session)
{
        mdm_greeter_client_call_answer_query (session->priv->client,
                                              text);
}

static void
on_select_session (MdmGreeterSession     *session,
                   const char            *text)
{
        mdm_greeter_client_call_select_session (session->priv->client,
                                                text);
}

static void
on_select_language (MdmGreeterSession     *session,
                    const char            *text)
{
        mdm_greeter_client_call_select_language (session->priv->client,
                                                 text);
}

static void
on_select_layout (MdmGreeterSession      *session,
                  const char             *text)
{
        mdm_greeter_client_call_select_layout (session->priv->client,
                                               text);
}

static void
on_select_user (MdmGreeterLoginWindow *login_window,
                const char            *text,
                MdmGreeterSession     *session)
{
        show_or_hide_user_options (session, text);
        mdm_greeter_client_call_select_user (session->priv->client,
                                             text);
}

static void
on_cancelled (MdmGreeterLoginWindow *login_window,
              MdmGreeterSession     *session)
{
        mdm_greeter_panel_hide_user_options (MDM_GREETER_PANEL (session->priv->panel));
        mdm_greeter_client_call_cancel (session->priv->client);
}

static void
on_disconnected (MdmGreeterLoginWindow *login_window,
                 MdmGreeterSession     *session)
{
        mdm_greeter_client_call_disconnect (session->priv->client);
}

static void
on_start_session (MdmGreeterLoginWindow *login_window,
                  MdmGreeterSession     *session)
{
        mdm_greeter_client_call_start_session_when_ready (session->priv->client, TRUE);
}

static int
get_tallest_monitor_at_point (GdkScreen *screen,
                              int        x,
                              int        y)
{
        GdkRectangle area;
        GdkRegion *region;
        int i;
        int monitor;
        int n_monitors;
        int tallest_height;

        tallest_height = 0;
        n_monitors = gdk_screen_get_n_monitors (screen);
        monitor = -1;
        for (i = 0; i < n_monitors; i++) {
                gdk_screen_get_monitor_geometry (screen, i, &area);
                region = gdk_region_rectangle (&area);

                if (gdk_region_point_in (region, x, y)) {
                        if (area.height > tallest_height) {
                                monitor = i;
                                tallest_height = area.height;
                        }
                }
                gdk_region_destroy (region);
        }

        if (monitor == -1) {
                monitor = gdk_screen_get_monitor_at_point (screen, x, y);
        }

        return monitor;
}

static void
toggle_panel (MdmGreeterSession *session,
              gboolean           enabled)
{
        mdm_profile_start (NULL);

        if (enabled) {
                GdkDisplay *display;
                GdkScreen  *screen;
                int         monitor;
                int         x, y;
                gboolean    is_local;

                display = gdk_display_get_default ();
                gdk_display_get_pointer (display, &screen, &x, &y, NULL);

                monitor = get_tallest_monitor_at_point (screen, x, y);

                is_local = mdm_greeter_client_get_display_is_local (session->priv->client);
                session->priv->panel = mdm_greeter_panel_new (screen, monitor, is_local);

                g_signal_connect_swapped (session->priv->panel,
                                          "language-selected",
                                          G_CALLBACK (on_select_language),
                                          session);

                g_signal_connect_swapped (session->priv->panel,
                                          "layout-selected",
                                          G_CALLBACK (on_select_layout),
                                          session);

                g_signal_connect_swapped (session->priv->panel,
                                          "session-selected",
                                          G_CALLBACK (on_select_session),
                                          session);

                gtk_widget_show (session->priv->panel);
        } else {
                gtk_widget_destroy (session->priv->panel);
                session->priv->panel = NULL;
        }

        mdm_profile_end (NULL);
}

static void
toggle_login_window (MdmGreeterSession *session,
                     gboolean           enabled)
{
        mdm_profile_start (NULL);

        if (enabled) {
                gboolean is_local;

                is_local = mdm_greeter_client_get_display_is_local (session->priv->client);
                g_debug ("MdmGreeterSession: Starting a login window local:%d", is_local);
                session->priv->login_window = mdm_greeter_login_window_new (is_local);

                g_signal_connect (session->priv->login_window,
                                  "begin-auto-login",
                                  G_CALLBACK (on_begin_auto_login),
                                  session);
                g_signal_connect (session->priv->login_window,
                                  "begin-verification",
                                  G_CALLBACK (on_begin_verification),
                                  session);
                g_signal_connect (session->priv->login_window,
                                  "begin-verification-for-user",
                                  G_CALLBACK (on_begin_verification_for_user),
                                  session);
                g_signal_connect (session->priv->login_window,
                                  "query-answer",
                                  G_CALLBACK (on_query_answer),
                                  session);
                g_signal_connect (session->priv->login_window,
                                  "user-selected",
                                  G_CALLBACK (on_select_user),
                                  session);
                g_signal_connect (session->priv->login_window,
                                  "cancelled",
                                  G_CALLBACK (on_cancelled),
                                  session);
                g_signal_connect (session->priv->login_window,
                                  "disconnected",
                                  G_CALLBACK (on_disconnected),
                                  session);
                g_signal_connect (session->priv->login_window,
                                  "start-session",
                                  G_CALLBACK (on_start_session),
                                  session);
                gtk_widget_show (session->priv->login_window);
        } else {
                gtk_widget_destroy (session->priv->login_window);
                session->priv->login_window = NULL;
        }
        mdm_profile_end (NULL);
}

gboolean
mdm_greeter_session_start (MdmGreeterSession *session,
                           GError           **error)
{
        gboolean res;

        g_return_val_if_fail (MDM_IS_GREETER_SESSION (session), FALSE);

        mdm_profile_start (NULL);

        res = mdm_greeter_client_start (session->priv->client, error);

        toggle_panel (session, TRUE);
        toggle_login_window (session, TRUE);

        mdm_profile_end (NULL);

        return res;
}

void
mdm_greeter_session_stop (MdmGreeterSession *session)
{
        g_return_if_fail (MDM_IS_GREETER_SESSION (session));

        toggle_panel (session, FALSE);
        toggle_login_window (session, FALSE);
}

static void
mdm_greeter_session_set_property (GObject        *object,
                                  guint           prop_id,
                                  const GValue   *value,
                                  GParamSpec     *pspec)
{
        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
mdm_greeter_session_get_property (GObject        *object,
                                  guint           prop_id,
                                  GValue         *value,
                                  GParamSpec     *pspec)
{
        switch (prop_id) {
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static GObject *
mdm_greeter_session_constructor (GType                  type,
                                 guint                  n_construct_properties,
                                 GObjectConstructParam *construct_properties)
{
        MdmGreeterSession      *greeter_session;

        greeter_session = MDM_GREETER_SESSION (G_OBJECT_CLASS (mdm_greeter_session_parent_class)->constructor (type,
                                                                                                               n_construct_properties,
                                                                                                               construct_properties));

        return G_OBJECT (greeter_session);
}

static void
mdm_greeter_session_dispose (GObject *object)
{
        g_debug ("MdmGreeterSession: Disposing greeter_session");

        G_OBJECT_CLASS (mdm_greeter_session_parent_class)->dispose (object);
}

static void
mdm_greeter_session_class_init (MdmGreeterSessionClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = mdm_greeter_session_get_property;
        object_class->set_property = mdm_greeter_session_set_property;
        object_class->constructor = mdm_greeter_session_constructor;
        object_class->dispose = mdm_greeter_session_dispose;
        object_class->finalize = mdm_greeter_session_finalize;

        g_type_class_add_private (klass, sizeof (MdmGreeterSessionPrivate));
}

static void
mdm_greeter_session_event_handler (GdkEvent          *event,
                                   MdmGreeterSession *session)
{
        g_assert (MDM_IS_GREETER_SESSION (session));

        if (event->type == GDK_KEY_PRESS) {
                GdkEventKey *key_event;

                key_event = (GdkEventKey *) event;
                if (session->priv->panel != NULL) {
                        if (gtk_window_activate_key (GTK_WINDOW (session->priv->panel),
                                                     key_event)) {
                                gtk_window_present_with_time (GTK_WINDOW (session->priv->panel),
                                                              key_event->time);
                                return;
                        }
                }

                if (session->priv->login_window != NULL) {
                        if (gtk_window_activate_key (GTK_WINDOW (session->priv->login_window),
                                                     ((GdkEventKey *) event))) {
                                gtk_window_present_with_time (GTK_WINDOW (session->priv->login_window),
                                                              key_event->time);
                                return;
                        }
                }
        }

        gtk_main_do_event (event);
}

static void
mdm_greeter_session_init (MdmGreeterSession *session)
{
        mdm_profile_start (NULL);

        session->priv = MDM_GREETER_SESSION_GET_PRIVATE (session);

        session->priv->client = mdm_greeter_client_new ();
        g_signal_connect (session->priv->client,
                          "info-query",
                          G_CALLBACK (on_info_query),
                          session);
        g_signal_connect (session->priv->client,
                          "secret-info-query",
                          G_CALLBACK (on_secret_info_query),
                          session);
        g_signal_connect (session->priv->client,
                          "info",
                          G_CALLBACK (on_info),
                          session);
        g_signal_connect (session->priv->client,
                          "problem",
                          G_CALLBACK (on_problem),
                          session);
        g_signal_connect (session->priv->client,
                          "ready",
                          G_CALLBACK (on_ready),
                          session);
        g_signal_connect (session->priv->client,
                          "reset",
                          G_CALLBACK (on_reset),
                          session);
        g_signal_connect (session->priv->client,
                          "authentication-failed",
                          G_CALLBACK (on_authentication_failed),
                          session);
        g_signal_connect (session->priv->client,
                          "selected-user-changed",
                          G_CALLBACK (on_selected_user_changed),
                          session);
        g_signal_connect (session->priv->client,
                          "default-language-name-changed",
                          G_CALLBACK (on_default_language_name_changed),
                          session);
        g_signal_connect (session->priv->client,
                          "default-layout-name-changed",
                          G_CALLBACK (on_default_layout_name_changed),
                          session);
        g_signal_connect (session->priv->client,
                          "default-session-name-changed",
                          G_CALLBACK (on_default_session_name_changed),
                          session);
        g_signal_connect (session->priv->client,
                          "timed-login-requested",
                          G_CALLBACK (on_timed_login_requested),
                          session);
        g_signal_connect (session->priv->client,
                          "user-authorized",
                          G_CALLBACK (on_user_authorized),
                          session);

        /* We want to listen for panel mnemonics even if the
         * login window is focused, so we intercept them here.
         */
        gdk_event_handler_set ((GdkEventFunc) mdm_greeter_session_event_handler,
                               session, NULL);

        mdm_profile_end (NULL);
}

static void
mdm_greeter_session_finalize (GObject *object)
{
        MdmGreeterSession *greeter_session;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_GREETER_SESSION (object));

        greeter_session = MDM_GREETER_SESSION (object);

        g_return_if_fail (greeter_session->priv != NULL);

        G_OBJECT_CLASS (mdm_greeter_session_parent_class)->finalize (object);
}

MdmGreeterSession *
mdm_greeter_session_new (void)
{
        if (session_object != NULL) {
                g_object_ref (session_object);
        } else {
                session_object = g_object_new (MDM_TYPE_GREETER_SESSION, NULL);
                g_object_add_weak_pointer (session_object,
                                           (gpointer *) &session_object);
        }

        return MDM_GREETER_SESSION (session_object);
}
