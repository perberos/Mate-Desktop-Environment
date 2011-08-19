/*
 * Copyright (C) 2008 Michael J. Chudobiak <mjc@avtechpulse.com>
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

#include <mateconf/mateconf-client.h>
#include <gio/gio.h>
#include <glib/gstdio.h>
#include <string.h>

#include "mate-settings-profile.h"
#include "gsd-housekeeping-manager.h"
#include "gsd-disk-space.h"


/* General */
#define INTERVAL_ONCE_A_DAY 24*60*60
#define INTERVAL_TWO_MINUTES 2*60


/* Thumbnail cleaner */
#define MATECONF_THUMB_AGE "/desktop/mate/thumbnail_cache/maximum_age"
#define DEFAULT_MAX_AGE_IN_DAYS 180
#define MATECONF_THUMB_SIZE "/desktop/mate/thumbnail_cache/maximum_size"
#define DEFAULT_MAX_SIZE_IN_MB 512
#define MATECONF_THUMB_BINDING_DIR "/desktop/mate/thumbnail_cache"


struct GsdHousekeepingManagerPrivate {
        guint long_term_cb;
        guint short_term_cb;
        guint mateconf_notify;
};


#define GSD_HOUSEKEEPING_MANAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GSD_TYPE_HOUSEKEEPING_MANAGER, GsdHousekeepingManagerPrivate))

static void     gsd_housekeeping_manager_class_init  (GsdHousekeepingManagerClass *klass);
static void     gsd_housekeeping_manager_init        (GsdHousekeepingManager      *housekeeping_manager);

G_DEFINE_TYPE (GsdHousekeepingManager, gsd_housekeeping_manager, G_TYPE_OBJECT)

static gpointer manager_object = NULL;


typedef struct {
        glong now;
        glong max_age;
        goffset total_size;
        goffset max_size;
} PurgeData;


typedef struct {
        time_t  mtime;
        char   *path;
        glong   size;
} ThumbData;


static void
thumb_data_free (gpointer data)
{
        ThumbData *info = data;

        if (info) {
                g_free (info->path);
                g_free (info);
        }
}


static GList *
read_dir_for_purge (const char *path, GList *files)
{
        GFile           *read_path;
        GFileEnumerator *enum_dir;

        read_path = g_file_new_for_path (path);
        enum_dir = g_file_enumerate_children (read_path,
                                              G_FILE_ATTRIBUTE_STANDARD_NAME ","
                                              G_FILE_ATTRIBUTE_TIME_MODIFIED ","
                                              G_FILE_ATTRIBUTE_STANDARD_SIZE,
                                              G_FILE_QUERY_INFO_NONE,
                                              NULL,
                                              NULL);

        if (enum_dir != NULL) {
                GFileInfo *info;
                while ((info = g_file_enumerator_next_file (enum_dir, NULL, NULL)) != NULL) {
                        const char *name;
                        name = g_file_info_get_name (info);

                        if (strlen (name) == 36 && strcmp (name + 32, ".png") == 0) {
                                ThumbData *td;
                                GFile     *entry;
                                char      *entry_path;
                                GTimeVal   mod_time;

                                entry = g_file_get_child (read_path, name);
                                entry_path = g_file_get_path (entry);
                                g_object_unref (entry);

                                g_file_info_get_modification_time (info, &mod_time);

                                td = g_new0 (ThumbData, 1);
                                td->path = entry_path;
                                td->mtime = mod_time.tv_sec;
                                td->size = g_file_info_get_size (info);

                                files = g_list_prepend (files, td);
                        }
                        g_object_unref (info);
                }
                g_object_unref (enum_dir);
        }
        g_object_unref (read_path);

        return files;
}


static void
purge_old_thumbnails (ThumbData *info, PurgeData *purge_data)
{
        if ((purge_data->now - info->mtime) > purge_data->max_age) {
                g_unlink (info->path);
                info->size = 0;
        } else {
                purge_data->total_size += info->size;
        }
}


static int
sort_file_mtime (ThumbData *file1, ThumbData *file2)
{
        return file1->mtime - file2->mtime;
}


static int
get_mateconf_int_with_default (char *key, int default_value)
{
        /* If the key is unset, we use a non-zero default value.
           A zero value corresponds to an extra-paranoid level
           of cleaning - it deletes all files. We don't want that
           as a default condition. */

        MateConfValue  *value;
        MateConfClient *client;
        int          res;

        client = mateconf_client_get_default ();
        value = mateconf_client_get (client, key, NULL);
        g_object_unref (client);

        if (value == NULL || value->type != MATECONF_VALUE_INT) {
                res = default_value;
        } else {
                res = mateconf_value_get_int (value);
                mateconf_value_free (value);
        }

        return res;
}


static void
purge_thumbnail_cache (void)
{

        char      *path;
        GList     *files;
        PurgeData  purge_data;
        GTimeVal   current_time;

        g_debug ("housekeeping: checking thumbnail cache size and freshness");

        path = g_build_filename (g_get_home_dir (),
                                 ".thumbnails",
                                 "normal",
                                 NULL);
        files = read_dir_for_purge (path, NULL);
        g_free (path);

        path = g_build_filename (g_get_home_dir (),
                                 ".thumbnails",
                                 "large",
                                 NULL);
        files = read_dir_for_purge (path, files);
        g_free (path);

        path = g_build_filename (g_get_home_dir (),
                                 ".thumbnails",
                                 "fail",
                                 "mate-thumbnail-factory",
                                 NULL);
        files = read_dir_for_purge (path, files);
        g_free (path);

        g_get_current_time (&current_time);

        purge_data.now = current_time.tv_sec;
        purge_data.max_age = get_mateconf_int_with_default (MATECONF_THUMB_AGE, DEFAULT_MAX_AGE_IN_DAYS) * 24 * 60 * 60;
        purge_data.max_size = get_mateconf_int_with_default (MATECONF_THUMB_SIZE, DEFAULT_MAX_SIZE_IN_MB) * 1024 * 1024;
        purge_data.total_size = 0;

        if (purge_data.max_age >= 0)
                g_list_foreach (files, (GFunc) purge_old_thumbnails, &purge_data);

        if ((purge_data.total_size > purge_data.max_size) && (purge_data.max_size >= 0)) {
                GList *scan;
                files = g_list_sort (files, (GCompareFunc) sort_file_mtime);
                for (scan = files; scan && (purge_data.total_size > purge_data.max_size); scan = scan->next) {
                        ThumbData *info = scan->data;
                        g_unlink (info->path);
                        purge_data.total_size -= info->size;
                }
        }

        g_list_foreach (files, (GFunc) thumb_data_free, NULL);
        g_list_free (files);
}


static gboolean
do_cleanup (GsdHousekeepingManager *manager)
{
        purge_thumbnail_cache ();
        return TRUE;
}


static gboolean
do_cleanup_once (GsdHousekeepingManager *manager)
{
        do_cleanup (manager);
        manager->priv->short_term_cb = 0;
        return FALSE;
}


static void
do_cleanup_soon (GsdHousekeepingManager *manager)
{
        if (manager->priv->short_term_cb == 0) {
                g_debug ("housekeeping: will tidy up in 2 minutes");
                manager->priv->short_term_cb = g_timeout_add_seconds (INTERVAL_TWO_MINUTES,
                                               (GSourceFunc) do_cleanup_once,
                                               manager);
        }
}


static void
bindings_callback (MateConfClient            *client,
                   guint                   cnxn_id,
                   MateConfEntry             *entry,
                   GsdHousekeepingManager *manager)
{
        do_cleanup_soon (manager);
}


static guint
register_config_callback (GsdHousekeepingManager *manager,
                          const char             *path,
                          MateConfClientNotifyFunc   func)
{
        MateConfClient *client = mateconf_client_get_default ();
        guint notify;

        mateconf_client_add_dir (client, path, MATECONF_CLIENT_PRELOAD_NONE, NULL);
        notify = mateconf_client_notify_add (client, path, func, manager, NULL, NULL);

        g_object_unref (client);

        return notify;
}


gboolean
gsd_housekeeping_manager_start (GsdHousekeepingManager *manager,
                                GError                **error)
{
        g_debug ("Starting housekeeping manager");
        mate_settings_profile_start (NULL);

        gsd_ldsm_setup (FALSE);

        manager->priv->mateconf_notify = register_config_callback (manager,
                                      MATECONF_THUMB_BINDING_DIR,
                                      (MateConfClientNotifyFunc) bindings_callback);

        /* Clean once, a few minutes after start-up */
        do_cleanup_soon (manager);

        /* Clean periodically, on a daily basis. */
        manager->priv->long_term_cb = g_timeout_add_seconds (INTERVAL_ONCE_A_DAY,
                                      (GSourceFunc) do_cleanup,
                                      manager);
        mate_settings_profile_end (NULL);

        return TRUE;
}


void
gsd_housekeeping_manager_stop (GsdHousekeepingManager *manager)
{
        GsdHousekeepingManagerPrivate *p = manager->priv;

        g_debug ("Stopping housekeeping manager");

        if (p->mateconf_notify != 0) {
                MateConfClient *client = mateconf_client_get_default ();

                mateconf_client_remove_dir (client, MATECONF_THUMB_BINDING_DIR, NULL);
                mateconf_client_notify_remove (client, p->mateconf_notify);

                g_object_unref (client);
                p->mateconf_notify = 0;
        }

        if (p->short_term_cb) {
                g_source_remove (p->short_term_cb);
                p->short_term_cb = 0;
        }

        if (p->long_term_cb) {
                g_source_remove (p->long_term_cb);
                p->long_term_cb = 0;

                /* Do a clean-up on shutdown if and only if the size or age
                   limits have been set to paranoid levels (zero) */
                if ((get_mateconf_int_with_default (MATECONF_THUMB_AGE, DEFAULT_MAX_AGE_IN_DAYS) == 0) ||
                    (get_mateconf_int_with_default (MATECONF_THUMB_SIZE, DEFAULT_MAX_SIZE_IN_MB) == 0)) {
                        do_cleanup (manager);
                }
        }

        gsd_ldsm_clean ();
}


static void
gsd_housekeeping_manager_class_init (GsdHousekeepingManagerClass *klass)
{
        g_type_class_add_private (klass, sizeof (GsdHousekeepingManagerPrivate));
}


static void
gsd_housekeeping_manager_init (GsdHousekeepingManager *manager)
{
        manager->priv = GSD_HOUSEKEEPING_MANAGER_GET_PRIVATE (manager);
}


GsdHousekeepingManager *
gsd_housekeeping_manager_new (void)
{
        if (manager_object != NULL) {
                g_object_ref (manager_object);
        } else {
                manager_object = g_object_new (GSD_TYPE_HOUSEKEEPING_MANAGER, NULL);
                g_object_add_weak_pointer (manager_object,
                                           (gpointer *) &manager_object);
        }

        return GSD_HOUSEKEEPING_MANAGER (manager_object);
}
