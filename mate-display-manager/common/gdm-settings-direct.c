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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <glib-object.h>

#include "gdm-settings.h"
#include "gdm-settings-utils.h"
#include "gdm-settings-direct.h"

static GHashTable      *schemas;
static GdmSettings     *settings_object;

static GdmSettingsEntry *
get_entry_for_key (const char *key)
{
        GdmSettingsEntry *entry;

        entry = g_hash_table_lookup (schemas, key);

        return entry;
}

static void
assert_signature (GdmSettingsEntry *entry,
                  const char       *signature)
{
        const char *sig;

        sig = gdm_settings_entry_get_signature (entry);

        g_assert (sig != NULL);
        g_assert (signature != NULL);
        g_assert (strcmp (signature, sig) == 0);
}

static gboolean
get_value (const char *key,
           char      **value)
{
        GError  *error;
        char    *str;
        gboolean res;

        error = NULL;
        res = gdm_settings_get_value (settings_object, key, &str, &error);
        if (! res) {
                if (error != NULL) {
                        g_error_free (error);
                } else {
                }

                return FALSE;
        }

        if (value != NULL) {
                *value = g_strdup (str);
        }

        g_free (str);

        return TRUE;
}

gboolean
gdm_settings_direct_get_int (const char        *key,
                             int               *value)
{
        GdmSettingsEntry *entry;
        gboolean          ret;
        gboolean          res;
        char             *str;

        g_return_val_if_fail (key != NULL, FALSE);

        entry = get_entry_for_key (key);
        g_assert (entry != NULL);

        assert_signature (entry, "i");

        ret = FALSE;

        res = get_value (key, &str);

        if (! res) {
                /* use the default */
                str = g_strdup (gdm_settings_entry_get_default_value (entry));
        }

        ret = gdm_settings_parse_value_as_integer (str, value);

        g_free (str);

        return ret;
}

gboolean
gdm_settings_direct_get_uint (const char        *key,
                              uint              *value)
{
        gboolean          ret;
        int               intvalue;

        ret = FALSE;
        ret = gdm_settings_direct_get_int (key, &intvalue);
   
        if (intvalue >= 0)
           *value = intvalue;
        else
           ret = FALSE;

        return ret;
}

gboolean
gdm_settings_direct_get_boolean (const char        *key,
                                 gboolean          *value)
{
        GdmSettingsEntry *entry;
        gboolean          ret;
        gboolean          res;
        char             *str;

        g_return_val_if_fail (key != NULL, FALSE);

        entry = get_entry_for_key (key);
        g_assert (entry != NULL);

        assert_signature (entry, "b");

        ret = FALSE;

        res = get_value (key, &str);

        if (! res) {
                /* use the default */
                str = g_strdup (gdm_settings_entry_get_default_value (entry));
        }

        ret = gdm_settings_parse_value_as_boolean  (str, value);

        g_free (str);

        return ret;
}

gboolean
gdm_settings_direct_get_string (const char        *key,
                                char             **value)
{
        GdmSettingsEntry *entry;
        gboolean          ret;
        gboolean          res;
        char             *str;

        g_return_val_if_fail (key != NULL, FALSE);

        entry = get_entry_for_key (key);
        g_assert (entry != NULL);

        assert_signature (entry, "s");

        ret = FALSE;

        res = get_value (key, &str);

        if (! res) {
                /* use the default */
                str = g_strdup (gdm_settings_entry_get_default_value (entry));
        }

        if (value != NULL) {
                *value = g_strdup (str);
        }

        g_free (str);

        return ret;
}

gboolean
gdm_settings_direct_set (const char        *key,
                         GValue            *value)
{
        return TRUE;
}

static void
hashify_list (GdmSettingsEntry *entry,
              gpointer          data)
{
        g_hash_table_insert (schemas, g_strdup (gdm_settings_entry_get_key (entry)), entry);
}

gboolean
gdm_settings_direct_init (GdmSettings *settings,
                          const char  *file,
                          const char  *root)
{
        GSList  *list;

        g_return_val_if_fail (file != NULL, FALSE);
        g_return_val_if_fail (root != NULL, FALSE);

        g_assert (schemas == NULL);

        if (! gdm_settings_parse_schemas (file, root, &list)) {
                g_warning ("Unable to parse schemas");
                return FALSE;
        }

        schemas = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify)gdm_settings_entry_free);
        g_slist_foreach (list, (GFunc)hashify_list, NULL);

        settings_object = settings;

        return TRUE;
}

void
gdm_settings_direct_shutdown (void)
{

}
