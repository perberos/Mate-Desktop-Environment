/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2006 Ray Strode <rstrode@redhat.com>
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if defined(HAVE_UTMPX_H)
#include <utmpx.h>
#endif

#if defined(HAVE_UTMP_H)
#include <utmp.h>
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>

#include "mdm-session-record.h"

#ifndef MDM_BAD_SESSION_RECORDS_FILE
#define MDM_BAD_SESSION_RECORDS_FILE "/var/log/btmp"
#endif

#if !defined(MDM_NEW_SESSION_RECORDS_FILE)
#    if defined(WTMPX_FILE)
#        define MDM_NEW_SESSION_RECORDS_FILE WTMPX_FILE
#    elif defined(_PATH_WTMPX)
#        define MDM_NEW_SESSION_RECORDS_FILE _PATH_WTMPX
#    elif defined(WTMPX_FILENAME)
#        define MDM_NEW_SESSION_RECORDS_FILE WTMPX_FILENAME
#    elif defined(WTMP_FILE)
#        define MDM_NEW_SESSION_RECORDS_FILE WTMP_FILE
#    elif defined(_PATH_WTMP) /* BSD systems */
#        define MDM_NEW_SESSION_RECORDS_FILE _PATH_WTMP
#    else
#        define MDM_NEW_SESSION_RECORDS_FILE "/var/log/wtmp"
#    endif
#endif

static void
record_set_username (UTMP       *u,
                     const char *user_name)
{
        const char *username;

        /*
         * It is possible that PAM failed before it mapped the user
         * input into a valid username, so we fallback to try using
         * "(unknown)"
         */
        if (user_name != NULL) {
                username = user_name;
        } else {
                username = "(unknown)";
        }

#if defined(HAVE_UT_UT_USER)
        strncpy (u->ut_user,
                 username,
                 sizeof (u->ut_user));
        g_debug ("using ut_user %.*s",
                 (int) sizeof (u->ut_user),
                 u->ut_user);
#elif defined(HAVE_UT_UT_NAME)
        strncpy (u->ut_name,
                 username,
                 sizeof (u->ut_name));
        g_debug ("using ut_name %.*s",
                 (int) sizeof (u->ut_name),
                 u->ut_name);
#endif
}

static void
record_set_timestamp (UTMP *u)
{
#if defined(HAVE_UT_UT_TV)
        GTimeVal    now = { 0 };

        /* Set time in TV format */
        g_get_current_time (&now);
        u->ut_tv.tv_sec  = now.tv_sec;
        u->ut_tv.tv_usec = now.tv_usec;
        g_debug ("using ut_tv time %ld",
                 (glong) u->ut_tv.tv_sec);
#elif defined(HAVE_UT_UT_TIME)
        /* Set time in time format */
        time (&u->ut_time);
        g_debug ("using ut_time %ld",
                 (glong) u->ut_time);
#endif
}

static void
record_set_pid (UTMP *u,
                GPid  pid)
{
#if defined(HAVE_UT_UT_PID)
        /* Set pid */
        if (pid != 0) {
                u->ut_pid = pid;
        }
        g_debug ("using ut_pid %d", (int) u->ut_pid);
#endif
}

static void
record_set_id (UTMP       *u,
               const char *id)
{
#if defined(HAVE_UT_UT_ID)
        strncpy (u->ut_id, id, sizeof (u->ut_id));
        g_debug ("using ut_id %.*s", (int) sizeof (u->ut_id), u->ut_id);
#endif
}

static void
record_set_host (UTMP       *u,
                 const char *x11_display_name,
                 const char *host_name)
{
        char *hostname;

#if defined(HAVE_UT_UT_HOST)
        hostname = NULL;

        /*
         * Set ut_host to hostname:$DISPLAY if remote, otherwise set
         * to $DISPLAY
         */
        if (host_name != NULL
            && x11_display_name != NULL
            && g_str_has_prefix (x11_display_name, ":")) {
                hostname = g_strdup_printf ("%s%s", host_name, x11_display_name);
        } else {
                hostname = g_strdup (x11_display_name);
        }

        if (hostname != NULL) {
                strncpy (u->ut_host, hostname, sizeof (u->ut_host));
                g_debug ("using ut_host %.*s", (int) sizeof (u->ut_host), u->ut_host);
                g_free (hostname);

#ifdef HAVE_UT_UT_SYSLEN
                u->ut_syslen = MIN (strlen (hostname), sizeof (u->ut_host));
#endif
        }
#endif
}

static void
record_set_line (UTMP       *u,
                 const char *display_device,
                 const char *x11_display_name)
{
        /*
         * Set ut_line to the device name associated with this display
         * but remove the "/dev/" prefix.  If no device, then use the
         * $DISPLAY value.
         */
        if (display_device != NULL
            && g_str_has_prefix (display_device, "/dev/")) {
                strncpy (u->ut_line,
                         display_device + strlen ("/dev/"),
                         sizeof (u->ut_line));
        } else if (x11_display_name != NULL
                   && g_str_has_prefix (x11_display_name, ":")) {
                strncpy (u->ut_line,
                         x11_display_name,
                         sizeof (u->ut_line));
        }

        g_debug ("using ut_line %.*s", (int) sizeof (u->ut_line), u->ut_line);
}

void
mdm_session_record_login (GPid                  session_pid,
                          const char           *user_name,
                          const char           *host_name,
                          const char           *x11_display_name,
                          const char           *display_device)
{
        UTMP        session_record = { 0 };
        UTMP       *u;

        record_set_username (&session_record, user_name);

        g_debug ("Writing login record");

#if defined(HAVE_UT_UT_TYPE)
        session_record.ut_type = USER_PROCESS;
        g_debug ("using ut_type USER_PROCESS");
#endif

        record_set_timestamp (&session_record);
        record_set_pid (&session_record, session_pid);

        /* Set ut_id to the $DISPLAY value */
        record_set_id (&session_record, x11_display_name);
        record_set_host (&session_record, x11_display_name, host_name);
        record_set_line (&session_record, display_device, x11_display_name);

        /* Handle wtmp */
        g_debug ("Writing wtmp session record to " MDM_NEW_SESSION_RECORDS_FILE);
#if defined(HAVE_UPDWTMPX)
        updwtmpx (MDM_NEW_SESSION_RECORDS_FILE, &session_record);
#elif defined(HAVE_UPDWTMP)
        updwtmp (MDM_NEW_SESSION_RECORDS_FILE, &session_record);
#elif defined(HAVE_LOGWTMP) && defined(HAVE_UT_UT_HOST)
#if defined(HAVE_UT_UT_USER)
        logwtmp (session_record.ut_line, session_record.ut_user, session_record.ut_host);
#elif defined(HAVE_UT_UT_NAME)
        logwtmp (session_record.ut_line, session_record.ut_name, session_record.ut_host);
#endif
#endif

        /*
         * Handle utmp
         * Update if entry already exists
         */
#if defined(HAVE_GETUTXENT)
        setutxent ();

        while ((u = getutxent ()) != NULL) {
                if (u->ut_type == USER_PROCESS &&
                    (session_record.ut_line != NULL &&
                     (strncmp (u->ut_line, session_record.ut_line,
                               sizeof (u->ut_line)) == 0 ||
                      u->ut_pid == session_record.ut_pid))) {
                        g_debug ("Updating existing utmp record");
                        pututxline (&session_record);
                        break;
                }
        }
        endutxent ();

        /* Add new entry if update did not work */
        if (u == NULL) {
                g_debug ("Adding new utmp record");
                pututxline (&session_record);
        }
#elif defined(HAVE_LOGIN)
	login (&session_record);
#endif
}

void
mdm_session_record_logout (GPid                  session_pid,
                           const char           *user_name,
                           const char           *host_name,
                           const char           *x11_display_name,
                           const char           *display_device)
{
        UTMP        session_record = { 0 };
        UTMP       *u;

        g_debug ("Writing logout record");

#if defined(HAVE_UT_UT_TYPE)
        session_record.ut_type = DEAD_PROCESS;
        g_debug ("using ut_type DEAD_PROCESS");
#endif

        record_set_timestamp (&session_record);
        record_set_pid (&session_record, session_pid);
        /* Set ut_id to the $DISPLAY value */
        record_set_id (&session_record, x11_display_name);
        record_set_host (&session_record, x11_display_name, host_name);
        record_set_line (&session_record, display_device, x11_display_name);


        /* Handle wtmp */
        g_debug ("Writing wtmp logout record to " MDM_NEW_SESSION_RECORDS_FILE);
#if defined(HAVE_UPDWTMPX)
        updwtmpx (MDM_NEW_SESSION_RECORDS_FILE, &session_record);
#elif defined (HAVE_UPDWTMP)
        updwtmp (MDM_NEW_SESSION_RECORDS_FILE, &session_record);
#elif defined(HAVE_LOGWTMP)
        logwtmp (session_record.ut_line, "", "");
#endif

        /* Handle utmp */
#if defined(HAVE_GETUTXENT)
        setutxent ();

        while ((u = getutxent ()) != NULL &&
               (u = getutxid (&session_record)) != NULL) {

                g_debug ("Removing utmp record");
                if (u->ut_pid == session_pid &&
                    u->ut_type == DEAD_PROCESS) {
                        /* Already done */
                        break;
                }

                u->ut_type = DEAD_PROCESS;
#if defined(HAVE_UT_UT_TV)
                u->ut_tv.tv_sec  = session_record.ut_tv.tv_sec;
                u->ut_tv.tv_usec = session_record.ut_tv.tv_usec;
#elif defined(HAVE_UT_UT_TIME)
                u->ut_time = session_record.ut_time;
#endif
#ifdef HAVE_UT_UT_EXIT_E_TERMINATION
                u->ut_exit.e_termination = 0;
                u->ut_exit.e_exit = 0;
#endif

                pututxline (u);

                break;
        }

        endutxent ();
#elif defined(HAVE_LOGOUT)
        logout (session_record.ut_line);
#endif
}

void
mdm_session_record_failed (GPid                  session_pid,
                           const char           *user_name,
                           const char           *host_name,
                           const char           *x11_display_name,
                           const char           *display_device)
{
        UTMP        session_record = { 0 };

        record_set_username (&session_record, user_name);

        g_debug ("Writing failed session attempt record");

#if defined(HAVE_UT_UT_TYPE)
        session_record.ut_type = USER_PROCESS;
        g_debug ("using ut_type USER_PROCESS");
#endif

        record_set_timestamp (&session_record);
        record_set_pid (&session_record, session_pid);
        /* Set ut_id to the $DISPLAY value */
        record_set_id (&session_record, x11_display_name);
        record_set_host (&session_record, x11_display_name, host_name);
        record_set_line (&session_record, display_device, x11_display_name);

        /* Handle btmp */
        g_debug ("Writing btmp failed session attempt record to "
                 MDM_BAD_SESSION_RECORDS_FILE);

#if defined(HAVE_UPDWTMPX)
        updwtmpx (MDM_BAD_SESSION_RECORDS_FILE, &session_record);
#elif defined(HAVE_UPDWTMP)
        updwtmp(MDM_BAD_SESSION_RECORDS_FILE, &session_record);
#endif

}
