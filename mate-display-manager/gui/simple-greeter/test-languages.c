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

#include "mdm-languages.h"

static void
print_languages (void)
{
        char **language_names;
        int    i;

        language_names = mdm_get_all_language_names ();

        for (i = 0; language_names[i] != NULL; i++) {
                char *language;
                char *normalized_name;
                char *readable_language;

                normalized_name = mdm_normalize_language_name (language_names[i]);
                language = mdm_get_language_from_name (normalized_name, normalized_name);
                readable_language = mdm_get_language_from_name (normalized_name, NULL);

                g_print ("%s\t%s\t%s\t%s\n",
                         language_names[i],
                         normalized_name,
                         language,
                         readable_language);

                g_free (language);
                g_free (readable_language);
                g_free (normalized_name);
        }

        g_strfreev (language_names);
}

int
main (int argc, char *argv[])
{

        bindtextdomain (GETTEXT_PACKAGE, MATELOCALEDIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);

        setlocale (LC_ALL, "");

        gtk_init (&argc, &argv);

        print_languages ();

        return 0;
}
