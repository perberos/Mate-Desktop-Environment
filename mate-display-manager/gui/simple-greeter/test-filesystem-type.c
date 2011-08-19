/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 William Jon McCann <mccann@jhu.edu>
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
#include <gio/gio.h>

static char *
get_filesystem_type (const char *path)
{
        GFile      *file;
        GFileInfo  *file_info;
        GError     *error;
        char       *filesystem_type;

        file = g_file_new_for_path (path);
        error = NULL;
        file_info = g_file_query_filesystem_info (file,
                                                  G_FILE_ATTRIBUTE_FILESYSTEM_TYPE,
                                                  NULL,
                                                  &error);
        if (file_info == NULL) {
                g_warning ("Unable to query filesystem type: %s", error->message);
                g_error_free (error);
                g_object_unref (file);
                return NULL;
        }

        filesystem_type = g_strdup (g_file_info_get_attribute_string (file_info,
                                                                      G_FILE_ATTRIBUTE_FILESYSTEM_TYPE));

        g_object_unref (file);
        g_object_unref (file_info);

        return filesystem_type;
}

static void
print_fstype (char **paths)
{
        int i;

        i = 0;
        while (paths[i] != NULL) {
                char *fstype;
                fstype = get_filesystem_type (paths[i]);
                g_print ("%s is %s\n", paths[i], fstype);
                g_free (fstype);
                i++;
        }
}

int
main (int argc, char *argv[])
{
        GOptionContext *ctx;
        char          **paths = NULL;
        GOptionEntry    options[] = {
                {G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &paths, NULL},
                {NULL}
        };

        bindtextdomain (GETTEXT_PACKAGE, MATELOCALEDIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);

        setlocale (LC_ALL, "");

        g_type_init ();

        /* Option parsing */
        ctx = g_option_context_new ("- Test filesystem type");
        g_option_context_add_main_entries (ctx, options, GETTEXT_PACKAGE);
        g_option_context_parse (ctx, &argc, &argv, NULL);
        g_option_context_free (ctx);

        if (paths != NULL) {
                print_fstype (paths);

                g_strfreev (paths);
        }

        return 0;
}
