/*
 * gsp-keyfile.c: GKeyFile extensions
 *
 * Copyright (C) 2008, 2009 Novell, Inc.
 *
 * Based on code from panel-keyfile.c (from mate-panel)
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
 * Authors:
 *        Vincent Untz <vuntz@gnome.org>
 */

#include <string.h>

#include <glib.h>

#include "gsp-keyfile.h"

void
gsp_key_file_populate (GKeyFile *keyfile)
{
        gsp_key_file_set_string (keyfile,
                                 G_KEY_FILE_DESKTOP_KEY_TYPE,
                                 "Application");

        gsp_key_file_set_string (keyfile,
                                 G_KEY_FILE_DESKTOP_KEY_EXEC,
                                 "/bin/false");
}

//FIXME: kill this when bug #309224 is fixed
gboolean
gsp_key_file_to_file (GKeyFile     *keyfile,
                      const gchar  *path,
                      GError      **error)
{
        GError  *write_error;
        gchar   *data;
        gsize    length;
        gboolean res;

        g_return_val_if_fail (keyfile != NULL, FALSE);
        g_return_val_if_fail (path != NULL, FALSE);

        write_error = NULL;
        data = g_key_file_to_data (keyfile, &length, &write_error);

        if (write_error) {
                g_propagate_error (error, write_error);
                return FALSE;
        }

        res = g_file_set_contents (path, data, length, &write_error);
        g_free (data);

        if (write_error) {
                g_propagate_error (error, write_error);
                return FALSE;
        }

        return res;
}

gboolean
gsp_key_file_get_boolean (GKeyFile    *keyfile,
                          const gchar *key,
                          gboolean     default_value)
{
        GError   *error;
        gboolean  retval;

        error = NULL;
        retval = g_key_file_get_boolean (keyfile, G_KEY_FILE_DESKTOP_GROUP,
                                         key, &error);
        if (error != NULL) {
                retval = default_value;
                g_error_free (error);
        }

        return retval;
}

void
gsp_key_file_set_locale_string (GKeyFile    *keyfile,
                                const gchar *key,
                                const gchar *value)
{
        const char         *locale;
        const char * const *langs_pointer;
        int                 i;

        if (value == NULL) {
                value = "";
        }

        locale = NULL;
        langs_pointer = g_get_language_names ();
        for (i = 0; langs_pointer[i] != NULL; i++) {
                /* find first without encoding  */
                if (strchr (langs_pointer[i], '.') == NULL) {
                        locale = langs_pointer[i]; 
                        break;
                }
        }

        if (locale != NULL) {
                g_key_file_set_locale_string (keyfile, G_KEY_FILE_DESKTOP_GROUP,
                                              key, locale, value);
        } else {
                g_key_file_set_string (keyfile, G_KEY_FILE_DESKTOP_GROUP,
                                       key, value);
        }
}

void
gsp_key_file_ensure_C_key (GKeyFile   *keyfile,
                           const char *key)
{
        char *C_value;
        char *buffer;

        /* Make sure we set the "C" locale strings to the terms we set here.
         * This is so that if the user logs into another locale they get their
         * own description there rather then empty. It is not the C locale
         * however, but the user created this entry herself so it's OK */
        C_value = gsp_key_file_get_string (keyfile, key);
        if (C_value == NULL || C_value [0] == '\0') {
                buffer = gsp_key_file_get_locale_string (keyfile, key);
                if (buffer) {
                        gsp_key_file_set_string (keyfile, key, buffer);
                        g_free (buffer);
                }
        }
        g_free (C_value);
}
