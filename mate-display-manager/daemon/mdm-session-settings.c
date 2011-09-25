/* mdm-session-settings.c - Loads session and language from ~/.dmrc
 *
 * Copyright (C) 2008 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * Written by: Ray Strode <rstrode@redhat.com>
 */
#include "config.h"
#include "mdm-session-settings.h"

#include <errno.h>
#include <pwd.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>

struct _MdmSessionSettingsPrivate
{
        char *session_name;
        char *language_name;
        char *layout_name;
};

static void mdm_session_settings_finalize (GObject *object);
static void mdm_session_settings_class_install_properties (MdmSessionSettingsClass *
                                              settings_class);

static void mdm_session_settings_set_property (GObject      *object,
                                              guint         prop_id,
                                              const GValue *value,
                                              GParamSpec   *pspec);
static void mdm_session_settings_get_property (GObject      *object,
                                              guint         prop_id,
                                              GValue       *value,
                                              GParamSpec   *pspec);

enum {
        PROP_0 = 0,
        PROP_SESSION_NAME,
        PROP_LANGUAGE_NAME,
        PROP_LAYOUT_NAME,
};

G_DEFINE_TYPE (MdmSessionSettings, mdm_session_settings, G_TYPE_OBJECT)

static void
mdm_session_settings_class_init (MdmSessionSettingsClass *settings_class)
{
        GObjectClass *object_class;

        object_class = G_OBJECT_CLASS (settings_class);

        object_class->finalize = mdm_session_settings_finalize;

        mdm_session_settings_class_install_properties (settings_class);

        g_type_class_add_private (settings_class, sizeof (MdmSessionSettingsPrivate));
}

static void
mdm_session_settings_class_install_properties (MdmSessionSettingsClass *settings_class)
{
        GObjectClass *object_class;
        GParamSpec   *param_spec;

        object_class = G_OBJECT_CLASS (settings_class);
        object_class->set_property = mdm_session_settings_set_property;
        object_class->get_property = mdm_session_settings_get_property;

        param_spec = g_param_spec_string ("session-name", "Session Name",
                                        "The name of the session",
                                        NULL, G_PARAM_READWRITE);
        g_object_class_install_property (object_class, PROP_SESSION_NAME, param_spec);

        param_spec = g_param_spec_string ("language-name", "Language Name",
                                        "The name of the language",
                                        NULL,
                                       G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
        g_object_class_install_property (object_class, PROP_LANGUAGE_NAME, param_spec);
        param_spec = g_param_spec_string ("layout-name", "Keyboard Layout Name",
                                        "The name of the keyboard layout",
                                        NULL,
                                       G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
        g_object_class_install_property (object_class, PROP_LAYOUT_NAME, param_spec);
}

static void
mdm_session_settings_init (MdmSessionSettings *settings)
{
        settings->priv = G_TYPE_INSTANCE_GET_PRIVATE (settings,
                                                     MDM_TYPE_SESSION_SETTINGS,
                                                     MdmSessionSettingsPrivate);

}

static void
mdm_session_settings_finalize (GObject *object)
{
        MdmSessionSettings *settings;
        GObjectClass *parent_class;

        settings = MDM_SESSION_SETTINGS (object);

        g_free (settings->priv->session_name);
        g_free (settings->priv->language_name);
        g_free (settings->priv->layout_name);

        parent_class = G_OBJECT_CLASS (mdm_session_settings_parent_class);

        if (parent_class->finalize != NULL) {
                parent_class->finalize (object);
        }
}

void
mdm_session_settings_set_language_name (MdmSessionSettings *settings,
                                        const char         *language_name)
{
        g_return_if_fail (MDM_IS_SESSION_SETTINGS (settings));

        if (settings->priv->language_name == NULL ||
            strcmp (settings->priv->language_name, language_name) != 0) {
                settings->priv->language_name = g_strdup (language_name);
                g_object_notify (G_OBJECT (settings), "language-name");
        }
}

void
mdm_session_settings_set_layout_name (MdmSessionSettings *settings,
                                      const char         *layout_name)
{
        g_return_if_fail (MDM_IS_SESSION_SETTINGS (settings));

        if (settings->priv->layout_name == NULL ||
            strcmp (settings->priv->layout_name, layout_name) != 0) {
                settings->priv->layout_name = g_strdup (layout_name);
                g_object_notify (G_OBJECT (settings), "layout-name");
        }
}

void
mdm_session_settings_set_session_name (MdmSessionSettings *settings,
                                       const char         *session_name)
{
        g_return_if_fail (MDM_IS_SESSION_SETTINGS (settings));

        if (settings->priv->session_name == NULL ||
            strcmp (settings->priv->session_name, session_name) != 0) {
                settings->priv->session_name = g_strdup (session_name);
                g_object_notify (G_OBJECT (settings), "session-name");
        }
}

char *
mdm_session_settings_get_language_name (MdmSessionSettings *settings)
{
        g_return_val_if_fail (MDM_IS_SESSION_SETTINGS (settings), NULL);
        return g_strdup (settings->priv->language_name);
}

char *
mdm_session_settings_get_layout_name (MdmSessionSettings *settings)
{
        g_return_val_if_fail (MDM_IS_SESSION_SETTINGS (settings), NULL);
        return g_strdup (settings->priv->layout_name);
}

char *
mdm_session_settings_get_session_name (MdmSessionSettings *settings)
{
        g_return_val_if_fail (MDM_IS_SESSION_SETTINGS (settings), NULL);
        return g_strdup (settings->priv->session_name);
}

static void
mdm_session_settings_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
        MdmSessionSettings *settings;

        settings = MDM_SESSION_SETTINGS (object);

        switch (prop_id) {
                case PROP_LANGUAGE_NAME:
                        mdm_session_settings_set_language_name (settings, g_value_get_string (value));
                break;

                case PROP_LAYOUT_NAME:
                        mdm_session_settings_set_layout_name (settings, g_value_get_string (value));
                break;

                case PROP_SESSION_NAME:
                        mdm_session_settings_set_session_name (settings, g_value_get_string (value));
                break;

                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}

static void
mdm_session_settings_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
        MdmSessionSettings *settings;

        settings = MDM_SESSION_SETTINGS (object);

        switch (prop_id) {
                case PROP_SESSION_NAME:
                        g_value_set_string (value, settings->priv->session_name);
                break;

                case PROP_LANGUAGE_NAME:
                        g_value_set_string (value, settings->priv->language_name);
                break;

                case PROP_LAYOUT_NAME:
                        g_value_set_string (value, settings->priv->layout_name);
                break;

                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

MdmSessionSettings *
mdm_session_settings_new (void)
{
        MdmSessionSettings *settings;

        settings = g_object_new (MDM_TYPE_SESSION_SETTINGS,
                                 NULL);

        return settings;
}

gboolean
mdm_session_settings_is_loaded (MdmSessionSettings  *settings)
{
        return settings->priv->session_name != NULL ||
               settings->priv->language_name != NULL ||
               settings->priv->layout_name != NULL;
}

gboolean
mdm_session_settings_load (MdmSessionSettings  *settings,
                           const char          *username,
                           GError             **error)
{
        GKeyFile *key_file;
        GError   *load_error;
        gboolean  is_loaded;
        char     *session_name;
        char     *language_name;
        char     *layout_name;
        char     *filename;

        g_return_val_if_fail (settings != NULL, FALSE);
        g_return_val_if_fail (username != NULL, FALSE);
        g_return_val_if_fail (!mdm_session_settings_is_loaded (settings), FALSE);

        filename = g_build_filename (MDM_CACHE_DIR, username, "dmrc", NULL);

        is_loaded = FALSE;
        key_file = g_key_file_new ();

        load_error = NULL;
        if (!g_key_file_load_from_file (key_file, filename,
                                        G_KEY_FILE_NONE, &load_error)) {
                g_propagate_error (error, load_error);
                goto out;
        }

        session_name = g_key_file_get_string (key_file, "Desktop", "Session",
                                              &load_error);

        if (session_name != NULL) {
                mdm_session_settings_set_session_name (settings, session_name);
                g_free (session_name);
        } else if (g_error_matches (load_error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_KEY_NOT_FOUND)) {
                g_error_free (load_error);
                load_error = NULL;
        } else {
                g_propagate_error (error, load_error);
                goto out;
        }

        language_name = g_key_file_get_string (key_file, "Desktop", "Language",
                                               &load_error);

        if (language_name != NULL) {
                mdm_session_settings_set_language_name (settings, language_name);
                g_free (language_name);
        } else if (g_error_matches (load_error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_KEY_NOT_FOUND)) {
                g_error_free (load_error);
                load_error = NULL;
        } else {
                g_propagate_error (error, load_error);
                goto out;
        }

        layout_name = g_key_file_get_string (key_file, "Desktop", "Layout",
                                             &load_error);

        if (layout_name != NULL) {
                mdm_session_settings_set_layout_name (settings, layout_name);
                g_free (layout_name);
        } else if (g_error_matches (load_error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_KEY_NOT_FOUND)) {
                g_error_free (load_error);
                load_error = NULL;
        } else {
                g_propagate_error (error, load_error);
                goto out;
        }

        is_loaded = TRUE;
out:
        g_key_file_free (key_file);
        g_free (filename);

        return is_loaded;
}

gboolean
mdm_session_settings_save (MdmSessionSettings  *settings,
                           const char          *home_directory,
                           GError             **error)
{
        GKeyFile *key_file;
        GError   *file_error;
        gboolean  is_saved;
        char     *filename;
        gsize     length;
        gchar    *contents;

        g_return_val_if_fail (MDM_IS_SESSION_SETTINGS (settings), FALSE);
        g_return_val_if_fail (home_directory != NULL, FALSE);
        g_return_val_if_fail (mdm_session_settings_is_loaded (settings), FALSE);
        filename = g_build_filename (home_directory, ".dmrc", NULL);

        is_saved = FALSE;
        key_file = g_key_file_new ();

        file_error = NULL;
        g_key_file_load_from_file (key_file, filename,
                                   G_KEY_FILE_KEEP_COMMENTS |
                                   G_KEY_FILE_KEEP_TRANSLATIONS,
                                   NULL);

        if (settings->priv->session_name != NULL) {
                g_key_file_set_string (key_file, "Desktop", "Session",
                                       settings->priv->session_name);
        }

        if (settings->priv->language_name != NULL) {
                g_key_file_set_string (key_file, "Desktop", "Language",
                                       settings->priv->language_name);
        }

        if (settings->priv->layout_name != NULL) {
                g_key_file_set_string (key_file, "Desktop", "Layout",
                                       settings->priv->layout_name);
        }

        contents = g_key_file_to_data (key_file, &length, &file_error);

        if (contents == NULL) {
                g_propagate_error (error, file_error);
                goto out;
        }

        if (!g_file_set_contents (filename, contents, length, &file_error)) {
                g_free (contents);
                g_propagate_error (error, file_error);
                goto out;
        }
        g_free (contents);

        is_saved = TRUE;
out:
        g_key_file_free (key_file);
        g_free (filename);

        return is_saved;
}
