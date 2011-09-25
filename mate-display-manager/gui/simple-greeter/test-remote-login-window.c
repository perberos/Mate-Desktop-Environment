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

#include "mdm-remote-login-window.h"

int
main (int argc, char *argv[])
{
        GtkWidget        *login_window;
        char             *std_out;
        char             *hostname;
        GRegex           *re;
        GMatchInfo       *match_info = NULL;
        gboolean          res;
        GError           *error;

        bindtextdomain (GETTEXT_PACKAGE, MATELOCALEDIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);

        std_out = NULL;
        g_spawn_command_line_sync (LIBEXECDIR "/mdm-host-chooser",
                                   &std_out,
                                   NULL, NULL, NULL);
        if (std_out == NULL) {
                exit (1);
        }

        error = NULL;
        re = g_regex_new ("hostname: (?P<hostname>[a-zA-Z0-9.-]+)", 0, 0, &error);
        if (re == NULL) {
                g_warning ("%s", error->message);
                goto out;
        }

        g_regex_match (re, std_out, 0, &match_info);

        res = g_match_info_matches (match_info);
        if (! res) {
                g_warning ("Unable to parse output: %s", std_out);
                goto out;
        }

        hostname = g_match_info_fetch_named (match_info, "hostname");

        g_debug ("Got %s", hostname);

        setlocale (LC_ALL, "");

        gtk_init (&argc, &argv);

        login_window = mdm_remote_login_window_new (TRUE);
        g_signal_connect (login_window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
        gtk_widget_show (login_window);

        mdm_remote_login_window_connect (MDM_REMOTE_LOGIN_WINDOW (login_window), hostname);

        gtk_main ();
 out:

        if (match_info != NULL) {
                g_match_info_free (match_info);
        }
        if (re != NULL) {
                g_regex_unref (re);
        }

        g_free (std_out);

        return 0;
}
