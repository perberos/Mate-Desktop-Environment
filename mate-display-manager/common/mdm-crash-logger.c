/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Dan Williams <dcbw@redhat.com>
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
 * (C) Copyright 2006 Red Hat, Inc.
 */

#include "config.h"

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <syslog.h>
#include <glib.h>

int main (int argc, char ** argv)
{
        GPid            gdb_pid;
        int             out;
        char            mdm_pid[16];
        char            line[256];
        int             gdb_stat;
        int             bytes_read;
        gboolean        res;
        gboolean        done;
        GError         *error;
        int             options;
        char *  args[] = { "gdb",
                           "--batch",
                           "--quiet",
                           "--command=" DATADIR "/mdm/gdb-cmd",
                           NULL,
                           NULL };

        snprintf (mdm_pid, sizeof (mdm_pid), "--pid=%d", getppid ());
        args[4] = &mdm_pid[0];
        error = NULL;
        res = g_spawn_async_with_pipes (NULL,
                                        args,
                                        NULL,
                                        G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
                                        NULL,
                                        NULL,
                                        &gdb_pid,
                                        NULL,
                                        &out,
                                        NULL,
                                        &error);
        if (! res) {
                g_warning ("Unable to get backtrace: %s", error->message);
                g_error_free (error);
                exit (1);
        }

        options = LOG_PID | LOG_CONS;
#ifdef LOG_PERROR
        options |= LOG_PERROR;
#endif

        openlog ("mdm", options, LOG_DAEMON);
        syslog (LOG_CRIT, "******************* START **********************************");
        done = FALSE;
        while (!done) {
                bytes_read = read (out, line, sizeof (line) - 1);
                if (bytes_read > 0) {
                        char *end = &line[0];
                        char *start = &line[0];

                        /* Can't just funnel the output to syslog, have to do a separate
                         * syslog () for each line in the output.
                         */
                        line[bytes_read] = '\0';
                        while (*end != '\0') {
                                if (*end == '\n') {
                                        *end = '\0';
                                        syslog (LOG_CRIT, "%s", start);
                                        start = end + 1;
                                }
                                end++;
                        }
                } else if ((bytes_read <= 0) || ((errno != EINTR) && (errno != EAGAIN))) {
                        done = TRUE;
                }
        }
        syslog (LOG_CRIT, "******************* END **********************************");
        close (out);
        waitpid (gdb_pid, &gdb_stat, 0);
        exit (0);
}
