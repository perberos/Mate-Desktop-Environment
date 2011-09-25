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
#include <errno.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <glib-object.h>

#include "mdm-settings-utils.h"

struct _MdmSettingsEntry
{
        char   *key;
        char   *signature;
        char   *default_value;
        char   *value;
};

MdmSettingsEntry *
mdm_settings_entry_new (void)
{
        MdmSettingsEntry *entry = NULL;

        entry = g_new0 (MdmSettingsEntry, 1);
        entry->key = NULL;
        entry->signature = NULL;
        entry->value = NULL;
        entry->default_value = NULL;

        return entry;
}

const char *
mdm_settings_entry_get_key (MdmSettingsEntry *entry)
{
        return entry->key;
}

const char *
mdm_settings_entry_get_signature (MdmSettingsEntry *entry)
{
        return entry->signature;
}

const char *
mdm_settings_entry_get_default_value (MdmSettingsEntry *entry)
{
        return entry->default_value;
}

const char *
mdm_settings_entry_get_value (MdmSettingsEntry *entry)
{
        return entry->value;
}

void
mdm_settings_entry_set_value (MdmSettingsEntry *entry,
                              const char       *value)
{
        g_free (entry->value);
        entry->value = g_strdup (value);
}

void
mdm_settings_entry_free (MdmSettingsEntry *entry)
{
        g_free (entry->key);
        g_free (entry->signature);
        g_free (entry->default_value);
        g_free (entry->value);
        g_free (entry);
}

typedef struct {
        GSList                 *list;
        MdmSettingsEntry       *entry;
        gboolean                in_key;
        gboolean                in_signature;
        gboolean                in_default;
} ParserInfo;

static void
start_element_cb (GMarkupParseContext *ctx,
                  const char          *element_name,
                  const char         **attribute_names,
                  const char         **attribute_values,
                  gpointer             user_data,
                  GError             **error)
{
        ParserInfo *info;

        info = (ParserInfo *) user_data;

        /*g_debug ("parsing start: '%s'", element_name);*/

        if (strcmp (element_name, "schema") == 0) {
                info->entry = mdm_settings_entry_new ();
        } else if (strcmp (element_name, "key") == 0) {
                info->in_key = TRUE;
        } else if (strcmp (element_name, "signature") == 0) {
                info->in_signature = TRUE;
        } else if (strcmp (element_name, "default") == 0) {
                info->in_default = TRUE;
        }
}

static void
add_schema_entry (ParserInfo *info)
{
        /*g_debug ("Inserting entry %s", info->entry->key);*/

        info->list = g_slist_prepend (info->list, info->entry);
}

static void
end_element_cb (GMarkupParseContext *ctx,
                const char          *element_name,
                gpointer             user_data,
                GError             **error)
{
        ParserInfo *info;

        info = (ParserInfo *) user_data;

        /*g_debug ("parsing end: '%s'", element_name);*/

        if (strcmp (element_name, "schema") == 0) {
                add_schema_entry (info);
        } else if (strcmp (element_name, "key") == 0) {
                info->in_key = FALSE;
        } else if (strcmp (element_name, "signature") == 0) {
                info->in_signature = FALSE;
        } else if (strcmp (element_name, "default") == 0) {
                info->in_default = FALSE;
        }
}

static void
text_cb (GMarkupParseContext *ctx,
         const char          *text,
         gsize                text_len,
         gpointer             user_data,
         GError             **error)
{
        ParserInfo *info;
        char       *t;

        info = (ParserInfo *) user_data;

        t = g_strndup (text, text_len);

        if (info->in_key) {
                info->entry->key = g_strdup (t);
        } else if (info->in_signature) {
                info->entry->signature = g_strdup (t);
        } else if (info->in_default) {
                info->entry->default_value = g_strdup (t);
        }

        g_free (t);

}

static void
error_cb (GMarkupParseContext *ctx,
          GError              *error,
          gpointer             user_data)
{
}

static GMarkupParser parser = {
        start_element_cb,
        end_element_cb,
        text_cb,
        NULL,
        error_cb
};

gboolean
mdm_settings_parse_schemas (const char  *file,
                            const char  *root,
                            GSList     **schemas)
{
        GMarkupParseContext *ctx;
        ParserInfo          *info;
        char                *contents;
        gsize                len;
        GError              *error;
        gboolean             res;

        g_return_val_if_fail (file != NULL, FALSE);
        g_return_val_if_fail (root != NULL, FALSE);

        g_assert (schemas != NULL);

        contents = NULL;
        error = NULL;
        res = g_file_get_contents (file, &contents, &len, &error);
        if (! res) {
                g_warning ("Unable to read schemas file: %s", error->message);
                g_error_free (error);
                return FALSE;
        }

        info = g_new0 (ParserInfo, 1);
        ctx = g_markup_parse_context_new (&parser, 0, info, NULL);
        g_markup_parse_context_parse (ctx, contents, len, NULL);

        *schemas = info->list;

        g_markup_parse_context_free (ctx);
        g_free (info);
        g_free (contents);

        return TRUE;
}

char *
mdm_settings_parse_double_as_value (gdouble doubleval)
{
        char result[G_ASCII_DTOSTR_BUF_SIZE];

        g_ascii_dtostr (result, sizeof (result), doubleval);

        return g_strdup (result);
}

char *
mdm_settings_parse_integer_as_value (int intval)

{
        return g_strdup_printf ("%d", intval);
}

char *
mdm_settings_parse_boolean_as_value  (gboolean boolval)
{
        if (boolval) {
                return g_strdup ("true");
        } else {
                return g_strdup ("false");
        }
}


/* adapted from GKeyFile */
gboolean
mdm_settings_parse_value_as_boolean (const char *value,
                                     gboolean   *bool)
{
        if (g_ascii_strcasecmp (value, "true") == 0 || strcmp (value, "1") == 0) {
                *bool = TRUE;
                return TRUE;
        } else if (g_ascii_strcasecmp (value, "false") == 0 || strcmp (value, "0") == 0) {
                *bool = FALSE;
                return TRUE;
        } else {
                return FALSE;
        }
}

gboolean
mdm_settings_parse_value_as_integer (const char *value,
                                     int        *intval)
{
        char *end_of_valid_int;
        glong long_value;
        gint  int_value;

        errno = 0;
        long_value = strtol (value, &end_of_valid_int, 10);

        if (*value == '\0' || *end_of_valid_int != '\0') {
                return FALSE;
        }

        int_value = long_value;
        if (int_value != long_value || errno == ERANGE) {
                return FALSE;
        }

        *intval = int_value;

        return TRUE;
}

gboolean
mdm_settings_parse_value_as_double  (const char *value,
                                     gdouble    *doubleval)
{
        char   *end_of_valid_d;
        gdouble double_value = 0;

        double_value = g_ascii_strtod (value, &end_of_valid_d);

        if (*end_of_valid_d != '\0' || end_of_valid_d == value) {
                return FALSE;
        }

        *doubleval = double_value;
        return TRUE;
}
