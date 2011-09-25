/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Red Hat, Inc.
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

#include "mdm-sessions.h"

static void
print_sessions (void)
{
        char **session_names;
        int    i;

        session_names = mdm_get_all_sessions ();

        for (i = 0; session_names[i] != NULL; i++) {
                gboolean res;
                char    *name;
                char    *comment;

                res = mdm_get_details_for_session (session_names[i],
                                                   &name,
                                                   &comment);
                if (! res) {
                        continue;
                }
                g_print ("%s\t%s\t%s\n",
                         session_names[i],
                         name,
                         comment);

                g_free (name);
                g_free (comment);
        }

        g_strfreev (session_names);
}

int
main (int argc, char *argv[])
{

        bindtextdomain (GETTEXT_PACKAGE, MATELOCALEDIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);

        setlocale (LC_ALL, "");

        gtk_init (&argc, &argv);

        print_sessions ();

        return 0;
}
