/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <stdlib.h>
#include <glib/gi18n.h>
#include <glib.h>
#include <glib-object.h>

#include "s-common-address.h"
#include "s-common-utils.h"

static gboolean no_fork = FALSE;
static gboolean verbose = FALSE;

static GOptionEntry entries[] = {
        {"no-fork", 0, 0, G_OPTION_ARG_NONE, &no_fork, "Don't fork individual tests", NULL},
        {"verbose", 0, 0, G_OPTION_ARG_NONE, &verbose, "Enable verbose output", NULL},
        {NULL}
};

int
main (int argc, char **argv)
{
        GOptionContext *context;
        SRunner        *r;
        int             failed;
        GError         *error;

        failed = 0;

        g_type_init ();

        context = g_option_context_new ("");
        g_option_context_add_main_entries (context, entries, NULL);
        error = NULL;
        g_option_context_parse (context, &argc, &argv, &error);
        g_option_context_free (context);

        if (error != NULL) {
                g_warning ("%s", error->message);
                g_error_free (error);
                exit (1);
        }

        r = srunner_create (suite_common_utils ());
        srunner_add_suite (r, suite_common_address ());

        if (no_fork) {
                srunner_set_fork_status (r, CK_NOFORK);
        }

        srunner_run_all (r, verbose ? CK_VERBOSE : CK_NORMAL);
        failed = srunner_ntests_failed (r);
        srunner_free (r);

        return failed != 0;
}
