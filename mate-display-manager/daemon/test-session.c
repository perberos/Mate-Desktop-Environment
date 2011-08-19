/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>

#include <glib.h>

#include "gdm-session-direct.h"

static GMainLoop *loop;

static void
on_conversation_started (GdmSession *session,
                         const char *username)
{
        g_debug ("Got conversation started: calling setup...");

        gdm_session_setup (session, "gdm");
}

static void
on_session_setup_complete (GdmSession *session,
                           gpointer    data)
{
        g_debug ("Session setup complete");
        gdm_session_authenticate (session);
}

static void
on_session_setup_failed (GdmSession *session,
                         const char *message,
                         gpointer    data)
{
        g_print ("Unable to initialize PAM: %s\n", message);

        exit (1);
}

static void
on_session_reset_complete (GdmSession *session,
                           gpointer    data)
{
        g_debug ("Session reset complete");
}

static void
on_session_reset_failed (GdmSession *session,
                         const char *message,
                         gpointer    data)
{
        g_print ("Unable to reset PAM: %s\n", message);

        exit (1);
}

static void
on_session_authenticated (GdmSession *session,
                          gpointer    data)
{
        g_debug ("Session authenticated");
        gdm_session_authorize (session);
}

static void
on_session_authentication_failed (GdmSession *session,
                                  const char *message,
                                  gpointer    data)
{
        g_print ("Unable to authenticate user: %s\n", message);

        exit (1);
}

static void
on_session_authorized (GdmSession *session,
                       gpointer    data)
{
        g_debug ("Session authorized");
        gdm_session_accredit (session, GDM_SESSION_CRED_ESTABLISH);
}

static void
on_session_authorization_failed (GdmSession *session,
                                 const char *message,
                                 gpointer    data)
{
        g_print ("Unable to authorize user: %s\n", message);

        exit (1);
}

static void
on_session_accredited (GdmSession *session,
                       gpointer    data)
{
        char *username;

        username = gdm_session_direct_get_username (GDM_SESSION_DIRECT (session));

        g_print ("%s%ssuccessfully accredited\n",
                 username ? username : "", username ? " " : "");
        g_free (username);

        gdm_session_start_session (session);

}

static void
on_session_accreditation_failed (GdmSession *session,
                                 const char *message,
                                 gpointer    data)
{
        g_print ("Unable to accredit user: %s\n", message);

        exit (1);
}

static void
on_session_started (GdmSession *session)
{
        g_print ("session started\n");
}

static void
on_session_exited (GdmSession *session,
                   int         exit_code)
{
        g_print ("session exited with code %d\n", exit_code);
        exit (0);
}

static void
on_session_died (GdmSession *session,
                 int         signal_number)
{
        g_print ("session died with signal %d, (%s)\n",
                 signal_number,
                 g_strsignal (signal_number));
        exit (1);
}

static void
on_info_query (GdmSession *session,
               const char *query_text)
{
        char  answer[1024];
        char *res;

        g_print ("%s ", query_text);

        answer[0] = '\0';
        res = fgets (answer, sizeof (answer), stdin);
        if (res == NULL) {
                g_warning ("Couldn't get an answer");
        }

        answer[strlen (answer) - 1] = '\0';

        if (answer[0] == '\0') {
                gdm_session_close (session);
                g_main_loop_quit (loop);
        } else {
                gdm_session_answer_query (session, answer);
        }
}

static void
on_info (GdmSession *session,
         const char *info)
{
        g_print ("\n** NOTE: %s\n", info);
}

static void
on_problem (GdmSession *session,
            const char *problem)
{
        g_print ("\n** WARNING: %s\n", problem);
}

static void
on_secret_info_query (GdmSession *session,
                      const char *query_text)
{
        char           answer[1024];
        char          *res;
        struct termios ts0;
        struct termios ts1;

        tcgetattr (fileno (stdin), &ts0);
        ts1 = ts0;
        ts1.c_lflag &= ~ECHO;

        g_print ("%s", query_text);

        if (tcsetattr (fileno (stdin), TCSAFLUSH, &ts1) != 0) {
                fprintf (stderr, "Could not set terminal attributes\n");
                exit (1);
        }

        answer[0] = '\0';
        res = fgets (answer, sizeof (answer), stdin);
        answer[strlen (answer) - 1] = '\0';
        if (res == NULL) {
                g_warning ("Couldn't get an answer");
        }

        tcsetattr (fileno (stdin), TCSANOW, &ts0);

        g_print ("\n");

        gdm_session_answer_query (session, answer);
}

static void
import_environment (GdmSessionDirect *session)
{
}

int
main (int   argc,
      char *argv[])
{
        GdmSessionDirect *session;
        char             *username;

        g_log_set_always_fatal (G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING);

        g_type_init ();

        do {
                g_debug ("creating instance of GdmSessionDirect object...");
                session = gdm_session_direct_new ("/org/mate/DisplayManager/Display1",
                                                  ":0",
                                                  g_get_host_name (),
                                                  ttyname (STDIN_FILENO),
                                                  getenv("XAUTHORITY"),
                                                  TRUE);
                g_debug ("GdmSessionDirect object created successfully");

                if (argc <= 1) {
                        username = NULL;
                } else {
                        username = argv[1];
                }

                gdm_session_start_conversation (GDM_SESSION (session));

                g_signal_connect (session,
                                  "conversation-started",
                                  G_CALLBACK (on_conversation_started),
                                  username);
                g_signal_connect (session,
                                  "setup-complete",
                                  G_CALLBACK (on_session_setup_complete),
                                  NULL);
                g_signal_connect (session,
                                  "setup-failed",
                                  G_CALLBACK (on_session_setup_failed),
                                  NULL);
                g_signal_connect (session,
                                  "reset-complete",
                                  G_CALLBACK (on_session_reset_complete),
                                  NULL);
                g_signal_connect (session,
                                  "reset-failed",
                                  G_CALLBACK (on_session_reset_failed),
                                  NULL);
                g_signal_connect (session,
                                  "authenticated",
                                  G_CALLBACK (on_session_authenticated),
                                  NULL);
                g_signal_connect (session,
                                  "authentication-failed",
                                  G_CALLBACK (on_session_authentication_failed),
                                  NULL);
                g_signal_connect (session,
                                  "authorized",
                                  G_CALLBACK (on_session_authorized),
                                  NULL);
                g_signal_connect (session,
                                  "authorization-failed",
                                  G_CALLBACK (on_session_authorization_failed),
                                  NULL);
                g_signal_connect (session,
                                  "accredited",
                                  G_CALLBACK (on_session_accredited),
                                  NULL);
                g_signal_connect (session,
                                  "accreditation-failed",
                                  G_CALLBACK (on_session_accreditation_failed),
                                  NULL);

                g_signal_connect (session,
                                  "info",
                                  G_CALLBACK (on_info),
                                  NULL);
                g_signal_connect (session,
                                  "problem",
                                  G_CALLBACK (on_problem),
                                  NULL);
                g_signal_connect (session,
                                  "info-query",
                                  G_CALLBACK (on_info_query),
                                  NULL);
                g_signal_connect (session,
                                  "secret-info-query",
                                  G_CALLBACK (on_secret_info_query),
                                  NULL);

                g_signal_connect (session,
                                  "session-started",
                                  G_CALLBACK (on_session_started),
                                  NULL);
                g_signal_connect (session,
                                  "session-exited",
                                  G_CALLBACK (on_session_exited),
                                  NULL);
                g_signal_connect (session,
                                  "session-died",
                                  G_CALLBACK (on_session_died),
                                  NULL);

                import_environment (session);

                loop = g_main_loop_new (NULL, FALSE);
                g_main_loop_run (loop);
                g_main_loop_unref (loop);

                g_message ("destroying previously created GdmSessionDirect object...");
                g_object_unref (session);
                g_message ("GdmSessionDirect object destroyed successfully");
        } while (1);
}
