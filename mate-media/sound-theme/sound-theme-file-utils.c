/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 * Copyright (C) 2008 Bastien Nocera <hadess@hadess.net>
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
 */

#include <config.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <utime.h>
#include <strings.h>

#include "sound-theme-file-utils.h"

#define CUSTOM_THEME_NAME       "__custom"

/* This function needs to be called after each individual
 * changeset to the theme */
void
custom_theme_update_time (void)
{
        char *path;

        path = custom_theme_dir_path (NULL);
        utime (path, NULL);
        g_free (path);
}

char *
custom_theme_dir_path (const char *child)
{
        static char *dir = NULL;
        const char *data_dir;

        if (dir == NULL) {
                data_dir = g_get_user_data_dir ();
                dir = g_build_filename (data_dir, "sounds", CUSTOM_THEME_NAME, NULL);
        }
        if (child == NULL)
                return g_strdup (dir);

        return g_build_filename (dir, child, NULL);
}

static gboolean
directory_delete_recursive (GFile *directory, GError **error)
{
        GFileEnumerator *enumerator;
        GFileInfo *info;
        gboolean success = TRUE;

        enumerator = g_file_enumerate_children (directory,
                                                G_FILE_ATTRIBUTE_STANDARD_NAME ","
                                                G_FILE_ATTRIBUTE_STANDARD_TYPE,
                                                G_FILE_QUERY_INFO_NONE,
                                                NULL, error);
        if (enumerator == NULL)
                return FALSE;

        while (success &&
               (info = g_file_enumerator_next_file (enumerator, NULL, NULL))) {
                GFile *child;

                child = g_file_get_child (directory, g_file_info_get_name (info));

                if (g_file_info_get_file_type (info) == G_FILE_TYPE_DIRECTORY) {
                        success = directory_delete_recursive (child, error);
                }
                g_object_unref (info);

                if (success)
                        success = g_file_delete (child, NULL, error);
        }
        g_file_enumerator_close (enumerator, NULL, NULL);

        if (success)
                success = g_file_delete (directory, NULL, error);

        return success;
}

/**
 * capplet_file_delete_recursive :
 * @file :
 * @error  :
 *
 * A utility routine to delete files and/or directories,
 * including non-empty directories.
 **/
static gboolean
capplet_file_delete_recursive (GFile *file, GError **error)
{
        GFileInfo *info;
        GFileType type;

        g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

        info = g_file_query_info (file,
                                  G_FILE_ATTRIBUTE_STANDARD_TYPE,
                                  G_FILE_QUERY_INFO_NONE,
                                  NULL, error);
        if (info == NULL)
                return FALSE;

        type = g_file_info_get_file_type (info);
        g_object_unref (info);

        if (type == G_FILE_TYPE_DIRECTORY)
                return directory_delete_recursive (file, error);
        else
                return g_file_delete (file, NULL, error);
}

void
delete_custom_theme_dir (void)
{
        char *dir;
        GFile *file;

        dir = custom_theme_dir_path (NULL);
        file = g_file_new_for_path (dir);
        g_free (dir);
        capplet_file_delete_recursive (file, NULL);
        g_object_unref (file);

        g_debug ("deleted the custom theme dir");
}

gboolean
custom_theme_dir_is_empty (void)
{
        char            *dir;
        GFile           *file;
        gboolean         is_empty;
        GFileEnumerator *enumerator;
        GFileInfo       *info;
        GError          *error = NULL;

        dir = custom_theme_dir_path (NULL);
        file = g_file_new_for_path (dir);
        g_free (dir);

        is_empty = TRUE;

        enumerator = g_file_enumerate_children (file,
                                                G_FILE_ATTRIBUTE_STANDARD_NAME ","
                                                G_FILE_ATTRIBUTE_STANDARD_TYPE,
                                                G_FILE_QUERY_INFO_NONE,
                                                NULL, &error);
        if (enumerator == NULL) {
                g_warning ("Unable to enumerate files: %s", error->message);
                g_error_free (error);
                goto out;
        }

        while (is_empty &&
               (info = g_file_enumerator_next_file (enumerator, NULL, NULL))) {

                if (strcmp ("index.theme", g_file_info_get_name (info)) != 0) {
                        is_empty = FALSE;
                }

                g_object_unref (info);
        }
        g_file_enumerator_close (enumerator, NULL, NULL);

 out:
        g_object_unref (file);

        return is_empty;
}

static void
delete_one_file (const char *sound_name, const char *pattern)
{
        GFile *file;
        char *name, *filename;

        name = g_strdup_printf (pattern, sound_name);
        filename = custom_theme_dir_path (name);
        g_free (name);
        file = g_file_new_for_path (filename);
        g_free (filename);
        capplet_file_delete_recursive (file, NULL);
        g_object_unref (file);
}

void
delete_old_files (const char **sounds)
{
        guint i;

        for (i = 0; sounds[i] != NULL; i++) {
                delete_one_file (sounds[i], "%s.ogg");
        }
}

void
delete_disabled_files (const char **sounds)
{
        guint i;

        for (i = 0; sounds[i] != NULL; i++)
                delete_one_file (sounds[i], "%s.disabled");
}

static void
create_one_file (GFile *file)
{
        GFileOutputStream* stream;

        stream = g_file_create (file, G_FILE_CREATE_NONE, NULL, NULL);
        if (stream != NULL) {
                g_output_stream_close (G_OUTPUT_STREAM (stream), NULL, NULL);
                g_object_unref (stream);
        }
}

void
add_disabled_file (const char **sounds)
{
        guint i;

        for (i = 0; sounds[i] != NULL; i++) {
                GFile *file;
                char *name, *filename;

                name = g_strdup_printf ("%s.disabled", sounds[i]);
                filename = custom_theme_dir_path (name);
                g_free (name);
                file = g_file_new_for_path (filename);
                g_free (filename);

                create_one_file (file);
                g_object_unref (file);
        }
}

void
add_custom_file (const char **sounds, const char *filename)
{
        guint i;

        for (i = 0; sounds[i] != NULL; i++) {
                GFile *file;
                char *name, *path;

                /* We use *.ogg because it's the first type of file that
                 * libcanberra looks at */
                name = g_strdup_printf ("%s.ogg", sounds[i]);
                path = custom_theme_dir_path (name);
                g_free (name);
                /* In case there's already a link there, delete it */
                g_unlink (path);
                file = g_file_new_for_path (path);
                g_free (path);

                /* Create the link */
                g_file_make_symbolic_link (file, filename, NULL, NULL);
                g_object_unref (file);
        }
}

void
create_custom_theme (const char *parent)
{
        GKeyFile *keyfile;
        char     *data;
        char     *path;

        /* Create the custom directory */
        path = custom_theme_dir_path (NULL);
        g_mkdir_with_parents (path, 0755);
        g_free (path);

        /* Set the data for index.theme */
        keyfile = g_key_file_new ();
        g_key_file_set_string (keyfile, "Sound Theme", "Name", _("Custom"));
        g_key_file_set_string (keyfile, "Sound Theme", "Inherits", parent);
        g_key_file_set_string (keyfile, "Sound Theme", "Directories", ".");
        data = g_key_file_to_data (keyfile, NULL, NULL);
        g_key_file_free (keyfile);

        /* Save the index.theme */
        path = custom_theme_dir_path ("index.theme");
        g_file_set_contents (path, data, -1, NULL);
        g_free (path);
        g_free (data);

        custom_theme_update_time ();
}
