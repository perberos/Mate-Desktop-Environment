/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 * gsm-mateconf.c
 * Copyright (C) 2007 Novell, Inc.
 *
 * FIXME: (C) on mateconf-sanity-check call, gsm_get_conf_client,
 * gsm_shutdown_mateconfd
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include "config.h"

#include <glib/gi18n.h>

#include <sys/wait.h>
#include <sys/types.h>

#include "gsm-mateconf.h"
#include "gsm-util.h"

static pid_t gsc_pid;

static void unset_display_setup (gpointer user_data);

/**
 * gsm_mateconf_init:
 *
 * Starts mateconfd asynchronously if it is not already running. This
 * must be called very soon after startup. It requires no
 * initialization beyond g_type_init().
 **/
void
gsm_mateconf_init (void)
{
        GError *error = NULL;
        char   *argv[2];

        /* Run mateconf-sanity-check. As a side effect, this will cause mateconfd
         * to be started. (We do this asynchronously so that other GSM
         * initialization can happen in parallel.)
         */

        argv[0] = MATECONF_SANITY_CHECK;
        argv[1] = NULL;

        g_spawn_async (NULL, argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD,
                       unset_display_setup, NULL, &gsc_pid, &error);
        if (error != NULL) {
                g_warning ("Failed to run mateconf-sanity-check-2: %s\n",
                           error->message);
                g_error_free (error);

                /* This probably means mateconf-sanity-check wasn't found, which
                 * really shouldn't happen, but we'll just ignore it for now as
                 * long as mateconf seems to be working later on...
                 */

                gsc_pid = 0;
        }
}

static void
unset_display_setup (gpointer user_data)
{
        /* Unset DISPLAY to make sure mateconf-sanity-check spews errors to
         * stderr instead of trying to show a dialog (since it doesn't
         * compensate for the fact that a window manager isn't running yet.)
         */
        g_unsetenv ("DISPLAY");
}

/**
 * gsm_mateconf_check:
 *
 * Verifies that gsm_mateconf_init() succeeded. (Exits with an error
 * dialog on failure.)
 **/
void
gsm_mateconf_check (void)
{
        if (gsc_pid) {
                int status;

                /* Wait for mateconf-sanity-check to finish */
                while (waitpid (gsc_pid, &status, 0) != gsc_pid) {
                        ;
                }
                gsc_pid = 0;

                if (!WIFEXITED (status) || WEXITSTATUS (status) != 0) {
                        /* FIXME: capture mateconf-sanity-check's stderr */
                        gsm_util_init_error (TRUE,
                                             _("There is a problem with the configuration server.\n"
                                               "(%s exited with status %d)"),
                                             MATECONF_SANITY_CHECK, status);
                }
        }
}

/**
 * gsm_mateconf_shutdown:
 *
 * Shuts down mateconfd before exiting.
 *
 * FIXME: does this need to be called even if mateconf-sanity-check fails?
 **/
void
gsm_mateconf_shutdown (void)
{
        GError *error;
        char   *command;
        int     status;

        command = g_strjoin (" ", MATECONFTOOL_CMD, "--shutdown", NULL);

        status = 0;
        error  = NULL;
        if (!g_spawn_command_line_sync (command, NULL, NULL, &status, &error)) {
                g_warning ("Failed to execute '%s' on logout: %s\n",
                           command, error->message);
                g_error_free (error);
        }

        if (status) {
                g_warning ("Running '%s' at logout returned an exit status of '%d'",
                           command, status);
        }

        g_free (command);
}
