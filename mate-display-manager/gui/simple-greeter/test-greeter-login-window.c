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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdlib.h>
#include <libintl.h>
#include <locale.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "mdm-settings-client.h"

#include "mdm-greeter-login-window.h"

static guint cancel_idle_id = 0;

static gboolean     timed_login   = FALSE;
static GOptionEntry entries []   = {
        { "timed-login", 0, 0, G_OPTION_ARG_NONE, &timed_login, "Test timed login", NULL },
        { NULL }
};

static gboolean
do_cancel (MdmGreeterLoginWindow *login_window)
{
        mdm_greeter_login_window_reset (MDM_GREETER_LOGIN_WINDOW (login_window));
        cancel_idle_id = 0;
        return FALSE;
}

static void
on_select_user (MdmGreeterLoginWindow *login_window,
                const char            *text,
                gpointer               data)
{
        g_debug ("user selected: %s", text);
        if (cancel_idle_id != 0) {
                return;
        }
        cancel_idle_id = g_timeout_add_seconds (5, (GSourceFunc) do_cancel, login_window);
}

static void
on_cancelled (MdmGreeterLoginWindow *login_window,
              gpointer               data)
{
        g_debug ("login cancelled");
        if (cancel_idle_id != 0) {
                g_source_remove (cancel_idle_id);
                cancel_idle_id = 0;
        }
}

int
main (int argc, char *argv[])
{
        GtkWidget        *login_window;

        bindtextdomain (GETTEXT_PACKAGE, MATELOCALEDIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);

        setlocale (LC_ALL, "");

        gtk_init_with_args (&argc,
                            &argv,
                            "",
                            entries,
                            NULL,
                            NULL);

        if (! mdm_settings_client_init (DATADIR "/mdm/mdm.schemas", "/")) {
                g_critical ("Unable to initialize settings client");
                exit (1);
        }

        login_window = mdm_greeter_login_window_new (TRUE);
        g_signal_connect (login_window,
                          "user-selected",
                          G_CALLBACK (on_select_user),
                          NULL);
        g_signal_connect (login_window,
                          "cancelled",
                          G_CALLBACK (on_cancelled),
                          NULL);
        if (timed_login) {
                mdm_greeter_login_window_request_timed_login (MDM_GREETER_LOGIN_WINDOW (login_window),
                                                              g_get_user_name (),
                                                              60);
        }

        gtk_widget_show (login_window);

        gtk_main ();

        return 0;
}
