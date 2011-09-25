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
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <mateconf/mateconf-client.h>

#include "mdm-common.h"
#include "mdm-log.h"
#include "mdm-settings-client.h"
#include "mdm-settings-keys.h"

#include "mdm-chooser-host.h"
#include "mdm-host-chooser-dialog.h"

#define ACCESSIBILITY_KEY         "/desktop/mate/interface/accessibility"

static Atom AT_SPI_IOR;


static gboolean
assistive_registry_launch (void)
{
        GPid        pid;
        GError     *error;
        const char *command;
        char      **argv;
        gboolean    res;

        command = AT_SPI_REGISTRYD_DIR "/at-spi-registryd";

        argv = NULL;
        error = NULL;
        res = g_shell_parse_argv (command, NULL, &argv, &error);
        if (! res) {
                g_warning ("Unable to parse command: %s", error->message);
                return FALSE;
        }

        error = NULL;
        res = g_spawn_async (NULL,
                             argv,
                             NULL,
                             G_SPAWN_SEARCH_PATH
                             | G_SPAWN_STDOUT_TO_DEV_NULL
                             | G_SPAWN_STDERR_TO_DEV_NULL,
                             NULL,
                             NULL,
                             &pid,
                             &error);
        g_strfreev (argv);

        if (! res) {
                g_warning ("Unable to run command %s: %s", command, error->message);
                return FALSE;
        }

        if (kill (pid, 0) < 0) {
                g_warning ("at-spi-registryd not running");
                return FALSE;
        }

        return TRUE;
}

static GdkFilterReturn
filter_watch (GdkXEvent *xevent,
              GdkEvent  *event,
              gpointer   data)
{
        XEvent *xev = (XEvent *)xevent;

        if (xev->xany.type == PropertyNotify
            && xev->xproperty.atom == AT_SPI_IOR) {
                gtk_main_quit ();

                return GDK_FILTER_REMOVE;
        }

        return GDK_FILTER_CONTINUE;
}

static gboolean
filter_timeout (gpointer data)
{
        g_warning ("The accessibility registry was not found.");

        gtk_main_quit ();

        return FALSE;
}

static void
assistive_registry_start (void)
{
        GdkWindow *root;
        guint      tid;

        root = gdk_get_default_root_window ();

        if ( ! AT_SPI_IOR) {
                AT_SPI_IOR = XInternAtom (GDK_DISPLAY (), "AT_SPI_IOR", False);
        }

        gdk_window_set_events (root,  GDK_PROPERTY_CHANGE_MASK);

        if ( ! assistive_registry_launch ()) {
                g_warning ("The accessibility registry could not be started.");
                return;
        }

        gdk_window_add_filter (root, filter_watch, NULL);
        tid = g_timeout_add_seconds (5, filter_timeout, NULL);

        gtk_main ();

        gdk_window_remove_filter (root, filter_watch, NULL);
        g_source_remove (tid);
}

static void
at_set_gtk_modules (void)
{
        GSList     *modules_list;
        GSList     *l;
        const char *old;
        char      **modules;
        gboolean    found_gail;
        gboolean    found_atk_bridge;
        int         n;

        n = 0;
        modules_list = NULL;
        found_gail = FALSE;
        found_atk_bridge = FALSE;

        if ((old = g_getenv ("GTK_MODULES")) != NULL) {
                modules = g_strsplit (old, ":", -1);
                for (n = 0; modules[n]; n++) {
                        if (!strcmp (modules[n], "gail")) {
                                found_gail = TRUE;
                        } else if (!strcmp (modules[n], "atk-bridge")) {
                                found_atk_bridge = TRUE;
                        }

                        modules_list = g_slist_prepend (modules_list, modules[n]);
                        modules[n] = NULL;
                }
                g_free (modules);
        }

        if (!found_gail) {
                modules_list = g_slist_prepend (modules_list, "gail");
                ++n;
        }

        if (!found_atk_bridge) {
                modules_list = g_slist_prepend (modules_list, "atk-bridge");
                ++n;
        }

        modules = g_new (char *, n + 1);
        modules[n--] = NULL;
        for (l = modules_list; l; l = l->next) {
                modules[n--] = g_strdup (l->data);
        }

        g_setenv ("GTK_MODULES", g_strjoinv (":", modules), TRUE);
        g_strfreev (modules);
        g_slist_free (modules_list);
}

static void
load_a11y (void)
{
        const char        *env_a_t_support;
        gboolean           a_t_support;
        MateConfClient       *mateconf_client;

        mateconf_client = mateconf_client_get_default ();

        env_a_t_support = g_getenv ("MATE_ACCESSIBILITY");
        if (env_a_t_support) {
                a_t_support = atoi (env_a_t_support);
        } else {
                a_t_support = mateconf_client_get_bool (mateconf_client, ACCESSIBILITY_KEY, NULL);
        }

        if (a_t_support) {
                assistive_registry_start ();
                at_set_gtk_modules ();
        }

        g_object_unref (mateconf_client);
}

int
main (int argc, char *argv[])
{
        GtkWidget        *chooser;

        bindtextdomain (GETTEXT_PACKAGE, MATELOCALEDIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);

        setlocale (LC_ALL, "");

        mdm_set_fatal_warnings_if_unstable ();

        g_type_init ();

        mdm_log_init ();
        mdm_log_set_debug (TRUE);

        g_debug ("Chooser for display %s xauthority:%s",
                 g_getenv ("DISPLAY"),
                 g_getenv ("XAUTHORITY"));

        gdk_init (&argc, &argv);

        load_a11y ();

        gtk_init (&argc, &argv);

        chooser = mdm_host_chooser_dialog_new (MDM_CHOOSER_HOST_KIND_MASK_ALL);
        gtk_widget_set_size_request (chooser, 480, 600);

        if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_OK) {
                MdmChooserHost *host;

                host = mdm_host_chooser_dialog_get_host (MDM_HOST_CHOOSER_DIALOG (chooser));
                if (host != NULL) {
                        char *hostname;
                        /* FIXME: handle different host types here? */

                        hostname = NULL;
                        mdm_address_get_hostname (mdm_chooser_host_get_address (host), &hostname);
                        /* FIXME: fall back to numerical address? */
                        if (hostname != NULL) {
                                g_print ("hostname: %s\n", hostname);
                                g_free (hostname);
                        }
                }
        }

        gtk_widget_destroy (chooser);

        return 0;
}
