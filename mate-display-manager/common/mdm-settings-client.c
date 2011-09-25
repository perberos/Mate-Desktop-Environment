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

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <glib-object.h>

#include "mdm-settings-client.h"
#include "mdm-settings-utils.h"

#define SETTINGS_DBUS_NAME      "org.mate.DisplayManager"
#define SETTINGS_DBUS_PATH      "/org/mate/DisplayManager/Settings"
#define SETTINGS_DBUS_INTERFACE "org.mate.DisplayManager.Settings"

static GHashTable      *notifiers      = NULL;
static GHashTable      *schemas        = NULL;
static DBusGProxy      *settings_proxy = NULL;
static DBusGConnection *connection     = NULL;
static guint32          id_serial      = 0;

typedef struct {
        guint                       id;
        char                       *root;
        MdmSettingsClientNotifyFunc func;
        gpointer                    user_data;
        GFreeFunc                   destroy_notify;
} MdmSettingsClientNotify;

static void
mdm_settings_client_notify_free (MdmSettingsClientNotify *notify)
{
        g_free (notify->root);

        if (notify->destroy_notify != NULL) {
                notify->destroy_notify (notify->user_data);
        }

        g_free (notify);
}

static MdmSettingsEntry *
get_entry_for_key (const char *key)
{
        MdmSettingsEntry *entry;

        entry = g_hash_table_lookup (schemas, key);

        if (entry == NULL) {
                g_warning ("MdmSettingsClient: unable to find schema for key: %s", key);
        }

        return entry;
}

static gboolean
set_value (const char *key,
           const char *value)
{
        GError  *error;
        gboolean res;

        /* FIXME: check cache */

        g_debug ("Setting %s=%s", key, value);
        error = NULL;
        res = dbus_g_proxy_call (settings_proxy,
                                 "SetValue",
                                 &error,
                                 G_TYPE_STRING, key,
                                 G_TYPE_STRING, value,
                                 G_TYPE_INVALID,
                                 G_TYPE_INVALID);
        if (! res) {
                if (error != NULL) {
                        /*g_debug ("Failed to get value for %s: %s", key, error->message);*/
                        g_error_free (error);
                } else {
                        /*g_debug ("Failed to get value for %s", key);*/
                }

                return FALSE;
        }

        return TRUE;
}

static gboolean
get_value (const char *key,
           char      **value)
{
        GError  *error;
        char    *str;
        gboolean res;

        /* FIXME: check cache */

        error = NULL;
        res = dbus_g_proxy_call (settings_proxy,
                                 "GetValue",
                                 &error,
                                 G_TYPE_STRING, key,
                                 G_TYPE_INVALID,
                                 G_TYPE_STRING, &str,
                                 G_TYPE_INVALID);
        if (! res) {
                if (error != NULL) {
                        /*g_debug ("Failed to get value for %s: %s", key, error->message);*/
                        g_error_free (error);
                } else {
                        /*g_debug ("Failed to get value for %s", key);*/
                }

                return FALSE;
        }

        if (value != NULL) {
                *value = g_strdup (str);
        }

        g_free (str);

        return TRUE;
}

static void
assert_signature (MdmSettingsEntry *entry,
                  const char       *signature)
{
        const char *sig;

        sig = mdm_settings_entry_get_signature (entry);

        g_assert (sig != NULL);
        g_assert (signature != NULL);
        g_assert (strcmp (signature, sig) == 0);
}

static guint32
get_next_serial (void)
{
        guint32 serial;

        serial = id_serial++;

        if ((gint32)id_serial < 0) {
                id_serial = 1;
        }

        return serial;
}

guint
mdm_settings_client_notify_add (const char                 *root,
                                MdmSettingsClientNotifyFunc func,
                                gpointer                    user_data,
                                GFreeFunc                   destroy_notify)
{
        guint32                  id;
        MdmSettingsClientNotify *notify;

        id = get_next_serial ();

        notify = g_new0 (MdmSettingsClientNotify, 1);
        notify->id = id;
        notify->root = g_strdup (root);
        notify->func = func;
        notify->user_data = user_data;
        notify->destroy_notify = destroy_notify;

        g_hash_table_insert (notifiers, GINT_TO_POINTER (id), notify);

        return id;
}

void
mdm_settings_client_notify_remove (guint id)
{
        g_hash_table_remove (notifiers, GINT_TO_POINTER (id));
}

gboolean
mdm_settings_client_get_string (const char  *key,
                                char       **value)
{
        MdmSettingsEntry *entry;
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
                str = g_strdup (mdm_settings_entry_get_default_value (entry));
        }

        if (value != NULL) {
                *value = g_strdup (str);
        }

        ret = TRUE;

        g_free (str);

        return ret;
}

gboolean
mdm_settings_client_get_locale_string (const char  *key,
                                       const char  *locale,
                                       char       **value)
{
        char    *candidate_key;
        char    *translated_value;
        char   **languages;
        gboolean free_languages = FALSE;
        int      i;
        gboolean ret;

        g_return_val_if_fail (key != NULL, FALSE);

        candidate_key = NULL;
        translated_value = NULL;

        if (locale != NULL) {
                languages = g_new (char *, 2);
                languages[0] = (char *)locale;
                languages[1] = NULL;

                free_languages = TRUE;
        } else {
                languages = (char **) g_get_language_names ();
                free_languages = FALSE;
        }

        for (i = 0; languages[i]; i++) {
                gboolean res;

                candidate_key = g_strdup_printf ("%s[%s]", key, languages[i]);

                res = get_value (candidate_key, &translated_value);
                g_free (candidate_key);

                if (res) {
                        break;
                }

                g_free (translated_value);
                translated_value = NULL;
        }

        /* Fallback to untranslated key
         */
        if (translated_value == NULL) {
                get_value (key, &translated_value);
        }

        if (free_languages) {
                g_strfreev (languages);
        }

        if (translated_value != NULL) {
                ret = TRUE;
                if (value != NULL) {
                        *value = g_strdup (translated_value);
                }
        } else {
                ret = FALSE;
        }

        g_free (translated_value);

        return ret;
}

gboolean
mdm_settings_client_get_boolean (const char *key,
                                 gboolean   *value)
{
        MdmSettingsEntry *entry;
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
                str = g_strdup (mdm_settings_entry_get_default_value (entry));
        }

        ret = mdm_settings_parse_value_as_boolean  (str, value);

        g_free (str);

        return ret;
}

gboolean
mdm_settings_client_get_int (const char *key,
                             int        *value)
{
        MdmSettingsEntry *entry;
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
                str = g_strdup (mdm_settings_entry_get_default_value (entry));
        }

        ret = mdm_settings_parse_value_as_integer (str, value);

        g_free (str);

        return ret;
}

gboolean
mdm_settings_client_set_int (const char *key,
                             int         value)
{
        MdmSettingsEntry *entry;
        gboolean          res;
        char             *str;

        g_return_val_if_fail (key != NULL, FALSE);

        entry = get_entry_for_key (key);
        g_assert (entry != NULL);

        assert_signature (entry, "i");

        str = mdm_settings_parse_integer_as_value (value);

        res = set_value (key, str);

        g_free (str);

        return res;
}

gboolean
mdm_settings_client_set_string (const char *key,
                                const char *value)
{
        MdmSettingsEntry *entry;
        gboolean          res;

        g_return_val_if_fail (key != NULL, FALSE);

        entry = get_entry_for_key (key);
        g_assert (entry != NULL);

        assert_signature (entry, "s");

        res = set_value (key, value);

        return res;
}

gboolean
mdm_settings_client_set_boolean (const char *key,
                                 gboolean    value)
{
        MdmSettingsEntry *entry;
        gboolean          res;
        char             *str;

        g_return_val_if_fail (key != NULL, FALSE);

        entry = get_entry_for_key (key);
        g_assert (entry != NULL);

        assert_signature (entry, "b");

        str = mdm_settings_parse_boolean_as_value (value);

        res = set_value (key, str);

        g_free (str);

        return res;
}

static void
hashify_list (MdmSettingsEntry *entry,
              gpointer          data)
{
        g_hash_table_insert (schemas, g_strdup (mdm_settings_entry_get_key (entry)), entry);
}

static void
send_notification (gpointer                 key,
                   MdmSettingsClientNotify *notify,
                   MdmSettingsEntry        *entry)
{
        /* get out if the key is not in the region of interest */
        if (! g_str_has_prefix (mdm_settings_entry_get_key (entry), notify->root)) {
                return;
        }

        notify->func (notify->id, entry, notify->user_data);
}

static void
on_value_changed (DBusGProxy *proxy,
                  const char *key,
                  const char *old_value,
                  const char *new_value,
                  gpointer    data)
{
        MdmSettingsEntry *entry;

        g_debug ("Value Changed key=%s old=%s new=%s", key, old_value, new_value);

        /* lookup entry */
        entry = get_entry_for_key (key);

        if (entry == NULL) {
                return;
        }

        mdm_settings_entry_set_value (entry, new_value);

        g_hash_table_foreach (notifiers, (GHFunc)send_notification, entry);
}

gboolean
mdm_settings_client_init (const char *file,
                          const char *root)
{
        GError  *error;
        GSList  *list;

        g_return_val_if_fail (file != NULL, FALSE);
        g_return_val_if_fail (root != NULL, FALSE);

        g_assert (schemas == NULL);

        error = NULL;
        connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
        if (connection == NULL) {
                if (error != NULL) {
                        g_warning ("error getting system bus: %s", error->message);
                        g_error_free (error);
                }
                return FALSE;
        }

        settings_proxy = dbus_g_proxy_new_for_name (connection,
                                                    SETTINGS_DBUS_NAME,
                                                    SETTINGS_DBUS_PATH,
                                                    SETTINGS_DBUS_INTERFACE);
        if (settings_proxy == NULL) {
                g_warning ("Unable to connect to settings server");
                return FALSE;
        }

        list = NULL;
        if (! mdm_settings_parse_schemas (file, root, &list)) {
                g_warning ("Unable to parse schemas");
                return FALSE;
        }

        notifiers = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, (GDestroyNotify)mdm_settings_client_notify_free);

        schemas = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, (GDestroyNotify)mdm_settings_entry_free);
        g_slist_foreach (list, (GFunc)hashify_list, NULL);

        dbus_g_proxy_add_signal (settings_proxy, "ValueChanged", G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INVALID);
        dbus_g_proxy_connect_signal (settings_proxy,
                                     "ValueChanged",
                                     G_CALLBACK (on_value_changed),
                                     NULL,
                                     NULL);


        return TRUE;
}

void
mdm_settings_client_shutdown (void)
{

}
