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

#include "mdm-user-manager.h"
#include "mdm-settings-client.h"

static MdmUserManager *manager = NULL;
static GMainLoop      *main_loop = NULL;

static gboolean     do_monitor       = FALSE;
static gboolean     fatal_warnings   = FALSE;
static GOptionEntry entries []   = {
        { "fatal-warnings", 0, 0, G_OPTION_ARG_NONE, &fatal_warnings, "Make all warnings fatal", NULL },
        { "monitor", 0, 0, G_OPTION_ARG_NONE, &do_monitor, "Monitor changes", NULL },
        { NULL }
};

static void
on_is_loaded_changed (MdmUserManager *manager,
                      GParamSpec     *pspec,
                      gpointer        data)
{
        GSList *users;

        g_debug ("Users loaded");

        users = mdm_user_manager_list_users (manager);
        while (users != NULL) {
                g_print ("User: %s\n", mdm_user_get_user_name (users->data));
                users = g_slist_delete_link (users, users);
        }

        if (! do_monitor) {
                g_main_loop_quit (main_loop);
        }
}

static void
on_user_added (MdmUserManager *manager,
               MdmUser        *user,
               gpointer        data)
{
        g_debug ("User added: %s", mdm_user_get_user_name (user));
}

static void
on_user_removed (MdmUserManager *manager,
                 MdmUser        *user,
                 gpointer        data)
{
        g_debug ("User removed: %s", mdm_user_get_user_name (user));
}

int
main (int argc, char *argv[])
{
        GOptionContext *context;
        GError         *error;
        gboolean        res;

        bindtextdomain (GETTEXT_PACKAGE, MATELOCALEDIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);

        setlocale (LC_ALL, "");

        g_type_init ();

        g_type_init ();

        context = g_option_context_new ("MATE Display Manager");
        g_option_context_add_main_entries (context, entries, NULL);
        g_option_context_set_ignore_unknown_options (context, TRUE);

        error = NULL;
        res = g_option_context_parse (context, &argc, &argv, &error);
        g_option_context_free (context);
        if (! res) {
                g_warning ("%s", error->message);
                g_error_free (error);
                return 0;
        }

        if (fatal_warnings) {
                GLogLevelFlags fatal_mask;

                fatal_mask = g_log_set_always_fatal (G_LOG_FATAL_MASK);
                fatal_mask |= G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL;
                g_log_set_always_fatal (fatal_mask);
        }

        if (! mdm_settings_client_init (DATADIR "/mdm/mdm.schemas", "/")) {
                g_critical ("Unable to initialize settings client");
                exit (1);
        }

        manager = mdm_user_manager_ref_default ();
        g_object_set (manager, "include-all", TRUE, NULL);
        g_signal_connect (manager,
                          "notify::is-loaded",
                          G_CALLBACK (on_is_loaded_changed),
                          NULL);
        g_signal_connect (manager,
                          "user-added",
                          G_CALLBACK (on_user_added),
                          NULL);
        g_signal_connect (manager,
                          "user-removed",
                          G_CALLBACK (on_user_removed),
                          NULL);
        mdm_user_manager_queue_load (manager);

        main_loop = g_main_loop_new (NULL, FALSE);

        g_main_loop_run (main_loop);
        if (main_loop != NULL) {
                g_main_loop_unref (main_loop);
        }
        g_object_unref (manager);

        return 0;
}
