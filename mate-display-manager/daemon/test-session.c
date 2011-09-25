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

#include "mdm-session-direct.h"

static GMainLoop *loop;

static void
on_conversation_started (MdmSession *session,
                         const char *username)
{
        g_debug ("Got conversation started: calling setup...");

        mdm_session_setup (session, "mdm");
}

static void
on_session_setup_complete (MdmSession *session,
                           gpointer    data)
{
        g_debug ("Session setup complete");
        mdm_session_authenticate (session);
}

static void
on_session_setup_failed (MdmSession *session,
                         const char *message,
                         gpointer    data)
{
        g_print ("Unable to initialize PAM: %s\n", message);

        exit (1);
}

static void
on_session_reset_complete (MdmSession *session,
                           gpointer    data)
{
        g_debug ("Session reset complete");
}

static void
on_session_reset_failed (MdmSession *session,
                         const char *message,
                         gpointer    data)
{
        g_print ("Unable to reset PAM: %s\n", message);

        exit (1);
}

static void
on_session_authenticated (MdmSession *session,
                          gpointer    data)
{
        g_debug ("Session authenticated");
        mdm_session_authorize (session);
}

static void
on_session_authentication_failed (MdmSession *session,
                                  const char *message,
                                  gpointer    data)
{
        g_print ("Unable to authenticate user: %s\n", message);

        exit (1);
}

static void
on_session_authorized (MdmSession *session,
                       gpointer    data)
{
        g_debug ("Session authorized");
        mdm_session_accredit (session, MDM_SESSION_CRED_ESTABLISH);
}

static void
on_session_authorization_failed (MdmSession *session,
                                 const char *message,
                                 gpointer    data)
{
        g_print ("Unable to authorize user: %s\n", message);

        exit (1);
}

static void
on_session_accredited (MdmSession *session,
                       gpointer    data)
{
        char *username;

        username = mdm_session_direct_get_username (MDM_SESSION_DIRECT (session));

        g_print ("%s%ssuccessfully accredited\n",
                 username ? username : "", username ? " " : "");
        g_free (username);

        mdm_session_start_session (session);

}

static void
on_session_accreditation_failed (MdmSession *session,
                                 const char *message,
                                 gpointer    data)
{
        g_print ("Unable to accredit user: %s\n", message);

        exit (1);
}

static void
on_session_started (MdmSession *session)
{
        g_print ("session started\n");
}

static void
on_session_exited (MdmSession *session,
                   int         exit_code)
{
        g_print ("session exited with code %d\n", exit_code);
        exit (0);
}

static void
on_session_died (MdmSession *session,
                 int         signal_number)
{
        g_print ("session died with signal %d, (%s)\n",
                 signal_number,
                 g_strsignal (signal_number));
        exit (1);
}

static void
on_info_query (MdmSession *session,
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
                mdm_session_close (session);
                g_main_loop_quit (loop);
        } else {
                mdm_session_answer_query (session, answer);
        }
}

static void
on_info (MdmSession *session,
         const char *info)
{
        g_print ("\n** NOTE: %s\n", info);
}

static void
on_problem (MdmSession *session,
            const char *problem)
{
        g_print ("\n** WARNING: %s\n", problem);
}

static void
on_secret_info_query (MdmSession *session,
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

        mdm_session_answer_query (session, answer);
}

static void
import_environment (MdmSessionDirect *session)
{
}

int
main (int   argc,
      char *argv[])
{
        MdmSessionDirect *session;
        char             *username;

        g_log_set_always_fatal (G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING);

        g_type_init ();

        do {
                g_debug ("creating instance of MdmSessionDirect object...");
                session = mdm_session_direct_new ("/org/mate/DisplayManager/Display1",
                                                  ":0",
                                                  g_get_host_name (),
                                                  ttyname (STDIN_FILENO),
                                                  getenv("XAUTHORITY"),
                                                  TRUE);
                g_debug ("MdmSessionDirect object created successfully");

                if (argc <= 1) {
                        username = NULL;
                } else {
                        username = argv[1];
                }

                mdm_session_start_conversation (MDM_SESSION (session));

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

                g_message ("destroying previously created MdmSessionDirect object...");
                g_object_unref (session);
                g_message ("MdmSessionDirect object destroyed successfully");
        } while (1);
}
