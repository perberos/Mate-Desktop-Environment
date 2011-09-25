/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright 2008 Red Hat, Inc,
 *           2007 William Jon McCann <mccann@jhu.edu>
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
 * Written by : William Jon McCann <mccann@jhu.edu>
 *              Ray Strode <rstrode@redhat.com>
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>

#include "mdm-sessions.h"

typedef struct _MdmSessionFile {
        char    *id;
        char    *path;
        char    *translated_name;
        char    *translated_comment;
} MdmSessionFile;

static GHashTable *mdm_available_sessions_map;

static gboolean mdm_sessions_map_is_initialized = FALSE;

/* adapted from mate-menus desktop-entries.c */
static gboolean
key_file_is_relevant (GKeyFile     *key_file)
{
        GError    *error;
        gboolean   no_display;
        gboolean   hidden;
        gboolean   tryexec_failed;
        char      *tryexec;

        error = NULL;
        no_display = g_key_file_get_boolean (key_file,
                                             G_KEY_FILE_DESKTOP_GROUP,
                                             "NoDisplay",
                                             &error);
        if (error) {
                no_display = FALSE;
                g_error_free (error);
        }

        error = NULL;
        hidden = g_key_file_get_boolean (key_file,
                                         G_KEY_FILE_DESKTOP_GROUP,
                                         "Hidden",
                                         &error);
        if (error) {
                hidden = FALSE;
                g_error_free (error);
        }

        tryexec_failed = FALSE;
        tryexec = g_key_file_get_string (key_file,
                                         G_KEY_FILE_DESKTOP_GROUP,
                                         "TryExec",
                                         NULL);
        if (tryexec) {
                char *path;

                path = g_find_program_in_path (g_strstrip (tryexec));

                tryexec_failed = (path == NULL);

                g_free (path);
                g_free (tryexec);
        }

        if (no_display || hidden || tryexec_failed) {
                return FALSE;
        }

        return TRUE;
}

static void
load_session_file (const char              *id,
                   const char              *path)
{
        GKeyFile          *key_file;
        GError            *error;
        gboolean           res;
        MdmSessionFile    *session;

        key_file = g_key_file_new ();

        error = NULL;
        res = g_key_file_load_from_file (key_file, path, 0, &error);

        if (!res) {
                g_debug ("Failed to load \"%s\": %s\n", path, error->message);
                g_error_free (error);
                goto out;
        }

        if (! g_key_file_has_group (key_file, G_KEY_FILE_DESKTOP_GROUP)) {
                goto out;
        }

        res = g_key_file_has_key (key_file, G_KEY_FILE_DESKTOP_GROUP, "Name", NULL);
        if (! res) {
                g_debug ("\"%s\" contains no \"Name\" key\n", path);
                goto out;
        }

        if (!key_file_is_relevant (key_file)) {
                g_debug ("\"%s\" is hidden or contains non-executable TryExec program\n", path);
                goto out;
        }

        session = g_new0 (MdmSessionFile, 1);

        session->id = g_strdup (id);
        session->path = g_strdup (path);

        session->translated_name = g_key_file_get_locale_string (key_file, G_KEY_FILE_DESKTOP_GROUP, "Name", NULL, NULL);
        session->translated_comment = g_key_file_get_locale_string (key_file, G_KEY_FILE_DESKTOP_GROUP, "Comment", NULL, NULL);

        g_hash_table_insert (mdm_available_sessions_map,
                             g_strdup (id),
                             session);
 out:
        g_key_file_free (key_file);
}

static void
collect_sessions_from_directory (const char *dirname)
{
        GDir       *dir;
        const char *filename;

        /* FIXME: add file monitor to directory */

        dir = g_dir_open (dirname, 0, NULL);
        if (dir == NULL) {
                return;
        }

        while ((filename = g_dir_read_name (dir))) {
                char *id;
                char *full_path;

                if (! g_str_has_suffix (filename, ".desktop")) {
                        continue;
                }
                id = g_strndup (filename, strlen (filename) - strlen (".desktop"));

                full_path = g_build_filename (dirname, filename, NULL);

                load_session_file (id, full_path);

                g_free (id);
                g_free (full_path);
        }

        g_dir_close (dir);
}

static void
collect_sessions (void)
{
        int         i;
        const char *search_dirs[] = {
                "/etc/X11/sessions/",
                DMCONFDIR "/Sessions/",
                DATADIR "/mdm/BuiltInSessions/",
                DATADIR "/xsessions/",
                NULL
        };

        if (mdm_available_sessions_map == NULL) {
                mdm_available_sessions_map = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                                    g_free, g_free);
        }

        for (i = 0; search_dirs [i] != NULL; i++) {
                collect_sessions_from_directory (search_dirs [i]);
        }
}

char **
mdm_get_all_sessions (void)
{
        GHashTableIter iter;
        gpointer key, value;
        GPtrArray *array;

        if (!mdm_sessions_map_is_initialized) {
                collect_sessions ();

                mdm_sessions_map_is_initialized = TRUE;
        }

        array = g_ptr_array_new ();
        g_hash_table_iter_init (&iter, mdm_available_sessions_map);
        while (g_hash_table_iter_next (&iter, &key, &value)) {
                MdmSessionFile *session;

                session = (MdmSessionFile *) value;

                g_ptr_array_add (array, g_strdup (session->id));
        }
        g_ptr_array_add (array, NULL);

        return (char **) g_ptr_array_free (array, FALSE);
}

gboolean
mdm_get_details_for_session (const char  *id,
                             char       **name,
                             char       **comment)
{
        MdmSessionFile *session;

        if (!mdm_sessions_map_is_initialized) {
                collect_sessions ();

                mdm_sessions_map_is_initialized = TRUE;
        }

        session = (MdmSessionFile *) g_hash_table_lookup (mdm_available_sessions_map,
                                                          id);

        if (session == NULL) {
                return FALSE;
        }

        if (name != NULL) {
                *name = g_strdup (session->translated_name);
        }

        if (comment != NULL) {
                *comment = g_strdup (session->translated_comment);
        }

        return TRUE;
}
