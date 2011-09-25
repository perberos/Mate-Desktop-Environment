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

#include "mdm-settings-desktop-backend.h"

#include "mdm-marshal.h"

#define MDM_SETTINGS_DESKTOP_BACKEND_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), MDM_TYPE_SETTINGS_DESKTOP_BACKEND, MdmSettingsDesktopBackendPrivate))

struct MdmSettingsDesktopBackendPrivate
{
        char       *filename;
        GKeyFile   *key_file;
        gboolean    dirty;
        guint       save_id;
};

static void     mdm_settings_desktop_backend_class_init (MdmSettingsDesktopBackendClass *klass);
static void     mdm_settings_desktop_backend_init       (MdmSettingsDesktopBackend      *settings_desktop_backend);
static void     mdm_settings_desktop_backend_finalize   (GObject                        *object);

G_DEFINE_TYPE (MdmSettingsDesktopBackend, mdm_settings_desktop_backend, MDM_TYPE_SETTINGS_BACKEND)

static gboolean
parse_key_string (const char *keystring,
                  char      **group,
                  char      **key,
                  char      **locale,
                  char      **value)
{
        char   **split1;
        char   **split2;
        char    *g;
        char    *k;
        char    *l;
        char    *v;
        char    *tmp1;
        char    *tmp2;
        gboolean ret;

        g_return_val_if_fail (keystring != NULL, FALSE);

        ret = FALSE;
        g = k = v = l = NULL;
        split1 = split2 = NULL;

        if (group != NULL) {
                *group = g;
        }
        if (key != NULL) {
                *key = k;
        }
        if (locale != NULL) {
                *locale = l;
        }
        if (value != NULL) {
                *value = v;
        }

        /*g_debug ("Attempting to parse key string: %s", keystring);*/

        split1 = g_strsplit (keystring, "/", 2);
        if (split1 == NULL
            || split1 [0] == NULL
            || split1 [1] == NULL
            || split1 [0][0] == '\0'
            || split1 [1][0] == '\0') {
                g_warning ("MdmSettingsDesktopBackend: invalid key: %s", keystring);
                goto out;
        }

        g = split1 [0];

        split2 = g_strsplit (split1 [1], "=", 2);
        if (split2 == NULL) {
                k = split1 [1];
        } else {
                k = split2 [0];
                v = split2 [1];
        }

        /* trim off the locale */
        tmp1 = strchr (k, '[');
        tmp2 = strchr (k, ']');
        if (tmp1 != NULL && tmp2 != NULL && tmp2 > tmp1) {
                l = g_strndup (tmp1 + 1, tmp2 - tmp1 - 1);
                *tmp1 = '\0';
        }

        ret = TRUE;

        if (group != NULL) {
                *group = g_strdup (g);
        }
        if (key != NULL) {
                *key = g_strdup (k);
        }
        if (locale != NULL) {
                *locale = g_strdup (l);
        }
        if (value != NULL) {
                *value = g_strdup (v);
        }
 out:

        g_strfreev (split1);
        g_strfreev (split2);

        return ret;
}

static gboolean
mdm_settings_desktop_backend_get_value (MdmSettingsBackend *backend,
                                        const char         *key,
                                        char              **value,
                                        GError            **error)
{
        GError  *local_error;
        char    *val;
        char    *g;
        char    *k;
        char    *l;
        gboolean ret;

        g_return_val_if_fail (MDM_IS_SETTINGS_BACKEND (backend), FALSE);
        g_return_val_if_fail (key != NULL, FALSE);

        ret = FALSE;

        if (value != NULL) {
                *value = NULL;
        }

        val = g = k = l = NULL;
        /*MDM_SETTINGS_BACKEND_CLASS (mdm_settings_desktop_backend_parent_class)->get_value (display);*/
        if (! parse_key_string (key, &g, &k, &l, NULL)) {
                g_set_error (error, MDM_SETTINGS_BACKEND_ERROR, MDM_SETTINGS_BACKEND_ERROR_KEY_NOT_FOUND, "Key not found");
                goto out;
        }

        /*g_debug ("Getting key: %s %s %s", g, k, l);*/
        local_error = NULL;
        val = g_key_file_get_value (MDM_SETTINGS_DESKTOP_BACKEND (backend)->priv->key_file,
                                    g,
                                    k,
                                    &local_error);
        if (local_error != NULL) {
                g_error_free (local_error);
                g_set_error (error, MDM_SETTINGS_BACKEND_ERROR, MDM_SETTINGS_BACKEND_ERROR_KEY_NOT_FOUND, "Key not found");
                goto out;
        }

        if (value != NULL) {
                *value = g_strdup (val);
        }
        ret = TRUE;
 out:
        g_free (val);
        g_free (g);
        g_free (k);
        g_free (l);

        return ret;
}

static void
save_settings (MdmSettingsDesktopBackend *backend)
{
        GError   *local_error;
        char     *contents;
        gsize     length;

        if (! backend->priv->dirty) {
                return;
        }

        g_debug ("Saving settings to %s", backend->priv->filename);

        local_error = NULL;
        contents = g_key_file_to_data (backend->priv->key_file, &length, &local_error);
        if (local_error != NULL) {
                g_warning ("Unable to save settings: %s", local_error->message);
                g_error_free (local_error);
                return;
        }

        local_error = NULL;
        g_file_set_contents (backend->priv->filename,
                             contents,
                             length,
                             &local_error);
        if (local_error != NULL) {
                g_warning ("Unable to save settings: %s", local_error->message);
                g_error_free (local_error);
                g_free (contents);
                return;
        }

        g_free (contents);

        backend->priv->dirty = FALSE;
}

static gboolean
save_settings_timer (MdmSettingsDesktopBackend *backend)
{
        save_settings (backend);
        backend->priv->save_id = 0;
        return FALSE;
}

static void
queue_save (MdmSettingsDesktopBackend *backend)
{
        if (! backend->priv->dirty) {
                return;
        }

        if (backend->priv->save_id != 0) {
                /* already pending */
                return;
        }

        backend->priv->save_id = g_timeout_add_seconds (5, (GSourceFunc)save_settings_timer, backend);
}

static gboolean
mdm_settings_desktop_backend_set_value (MdmSettingsBackend *backend,
                                        const char         *key,
                                        const char         *value,
                                        GError            **error)
{
        GError *local_error;
        char   *old_val;
        char   *g;
        char   *k;
        char   *l;

        g_return_val_if_fail (MDM_IS_SETTINGS_BACKEND (backend), FALSE);
        g_return_val_if_fail (key != NULL, FALSE);

        /*MDM_SETTINGS_BACKEND_CLASS (mdm_settings_desktop_backend_parent_class)->get_value (display);*/
        if (! parse_key_string (key, &g, &k, &l, NULL)) {
                g_set_error (error, MDM_SETTINGS_BACKEND_ERROR, MDM_SETTINGS_BACKEND_ERROR_KEY_NOT_FOUND, "Key not found");
                return FALSE;
        }

        local_error = NULL;
        old_val = g_key_file_get_value (MDM_SETTINGS_DESKTOP_BACKEND (backend)->priv->key_file,
                                        g,
                                        k,
                                        &local_error);
        if (local_error != NULL) {
                g_error_free (local_error);
        }

        /*g_debug ("Setting key: %s %s %s", g, k, l);*/
        local_error = NULL;
        g_key_file_set_value (MDM_SETTINGS_DESKTOP_BACKEND (backend)->priv->key_file,
                              g,
                              k,
                              value);

        MDM_SETTINGS_DESKTOP_BACKEND (backend)->priv->dirty = TRUE;
        queue_save (MDM_SETTINGS_DESKTOP_BACKEND (backend));

        mdm_settings_backend_value_changed (backend, key, old_val, value);

        g_free (old_val);

        return TRUE;
}

static void
mdm_settings_desktop_backend_class_init (MdmSettingsDesktopBackendClass *klass)
{
        GObjectClass            *object_class = G_OBJECT_CLASS (klass);
        MdmSettingsBackendClass *backend_class = MDM_SETTINGS_BACKEND_CLASS (klass);

        object_class->finalize = mdm_settings_desktop_backend_finalize;

        backend_class->get_value = mdm_settings_desktop_backend_get_value;
        backend_class->set_value = mdm_settings_desktop_backend_set_value;

        g_type_class_add_private (klass, sizeof (MdmSettingsDesktopBackendPrivate));
}

static void
mdm_settings_desktop_backend_init (MdmSettingsDesktopBackend *backend)
{
        gboolean res;
        GError  *error;

        backend->priv = MDM_SETTINGS_DESKTOP_BACKEND_GET_PRIVATE (backend);

        backend->priv->key_file = g_key_file_new ();
        backend->priv->filename = g_strdup (MDM_CUSTOM_CONF);

        error = NULL;
        res = g_key_file_load_from_file (backend->priv->key_file,
                                         backend->priv->filename,
                                         G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS,
                                         &error);
        if (! res) {
                g_warning ("Unable to load file '%s': %s", backend->priv->filename, error->message);
        }
}

static void
mdm_settings_desktop_backend_finalize (GObject *object)
{
        MdmSettingsDesktopBackend *backend;

        g_return_if_fail (object != NULL);
        g_return_if_fail (MDM_IS_SETTINGS_DESKTOP_BACKEND (object));

        backend = MDM_SETTINGS_DESKTOP_BACKEND (object);

        g_return_if_fail (backend->priv != NULL);

        save_settings (backend);
        g_key_file_free (backend->priv->key_file);
        g_free (backend->priv->filename);

        G_OBJECT_CLASS (mdm_settings_desktop_backend_parent_class)->finalize (object);
}

MdmSettingsBackend *
mdm_settings_desktop_backend_new (void)
{
        GObject *object;

        object = g_object_new (MDM_TYPE_SETTINGS_DESKTOP_BACKEND, NULL);

        return MDM_SETTINGS_BACKEND (object);
}
