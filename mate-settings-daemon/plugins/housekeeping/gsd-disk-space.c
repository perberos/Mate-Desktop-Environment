/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 * vim: set et sw=8 ts=8:
 *
 * Copyright (c) 2008, Novell, Inc.
 *
 * Authors: Vincent Untz <vuntz@gnome.org>
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

/* gcc -DHAVE_LIBMATENOTIFY -DTEST -Wall `pkg-config --cflags --libs gobject-2.0 gio-unix-2.0 glib-2.0 gtk+-2.0 libmatenotify` -o gsd-disk-space-test gsd-disk-space.c */

#include "config.h"

#include <sys/statvfs.h>
#include <time.h>
#include <unistd.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <gio/gunixmounts.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <mateconf/mateconf-client.h>

#include "gsd-disk-space.h"
#include "gsd-ldsm-dialog.h"
#include "gsd-ldsm-trash-empty.h"


#define GIGABYTE                   1024 * 1024 * 1024

#define CHECK_EVERY_X_SECONDS      60

#define DISK_SPACE_ANALYZER        "baobab"

#define MATECONF_HOUSEKEEPING_DIR     "/apps/mate_settings_daemon/plugins/housekeeping"
#define MATECONF_FREE_PC_NOTIFY_KEY   "free_percent_notify"
#define MATECONF_FREE_PC_NOTIFY_AGAIN_KEY "free_percent_notify_again"
#define MATECONF_FREE_SIZE_NO_NOTIFY  "free_size_gb_no_notify"
#define MATECONF_MIN_NOTIFY_PERIOD    "min_notify_period"
#define MATECONF_IGNORE_PATHS         "ignore_paths"

typedef struct
{
        GUnixMountEntry *mount;
        struct statvfs buf;
        time_t notify_time;
} LdsmMountInfo;

static GHashTable        *ldsm_notified_hash = NULL;
static unsigned int       ldsm_timeout_id = 0;
static GUnixMountMonitor *ldsm_monitor = NULL;
static double             free_percent_notify = 0.05;
static double             free_percent_notify_again = 0.01;
static unsigned int       free_size_gb_no_notify = 2;
static unsigned int       min_notify_period = 10;
static GSList            *ignore_paths = NULL;
static unsigned int       mateconf_notify_id;
static MateConfClient       *client = NULL;
static GsdLdsmDialog     *dialog = NULL;
static guint64           *time_read;

static gchar*
ldsm_get_fs_id_for_path (const gchar *path)
{
        GFile *file;
        GFileInfo *fileinfo;
        gchar *attr_id_fs;

        file = g_file_new_for_path (path);
        fileinfo = g_file_query_info (file, G_FILE_ATTRIBUTE_ID_FILESYSTEM, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, NULL);
        if (fileinfo) {
                attr_id_fs = g_strdup (g_file_info_get_attribute_string (fileinfo, G_FILE_ATTRIBUTE_ID_FILESYSTEM));
                g_object_unref (fileinfo);
        } else {
                attr_id_fs = NULL;
        }

        g_object_unref (file);

        return attr_id_fs;
}

static gboolean
ldsm_mount_has_trash (LdsmMountInfo *mount)
{
        const gchar *user_data_dir;
        gchar *user_data_attr_id_fs;
        gchar *path_attr_id_fs;
        gboolean mount_uses_user_trash = FALSE;
        gchar *trash_files_dir;
        gboolean has_trash = FALSE;
        GDir *dir;
        const gchar *path;

        user_data_dir = g_get_user_data_dir ();
        user_data_attr_id_fs = ldsm_get_fs_id_for_path (user_data_dir);

        path = g_unix_mount_get_mount_path (mount->mount);
        path_attr_id_fs = ldsm_get_fs_id_for_path (path);

        if (g_strcmp0 (user_data_attr_id_fs, path_attr_id_fs) == 0) {
                /* The volume that is low on space is on the same volume as our home
                 * directory. This means the trash is at $XDG_DATA_HOME/Trash,
                 * not at the root of the volume which is full.
                 */
                mount_uses_user_trash = TRUE;
        }

        g_free (user_data_attr_id_fs);
        g_free (path_attr_id_fs);

        /* I can't think of a better way to find out if a volume has any trash. Any suggestions? */
        if (mount_uses_user_trash) {
                trash_files_dir = g_build_filename (g_get_user_data_dir (), "Trash", "files", NULL);
        } else {
                gchar *uid;

                uid = g_strdup_printf ("%d", getuid ());
                trash_files_dir = g_build_filename (path, ".Trash", uid, "files", NULL);
                if (!g_file_test (trash_files_dir, G_FILE_TEST_IS_DIR)) {
                        gchar *trash_dir;

                        g_free (trash_files_dir);
                        trash_dir = g_strdup_printf (".Trash-%s", uid);
                        trash_files_dir = g_build_filename (path, trash_dir, "files", NULL);
                        g_free (trash_dir);
                        if (!g_file_test (trash_files_dir, G_FILE_TEST_IS_DIR)) {
                                g_free (trash_files_dir);
                                g_free (uid);
                                return has_trash;
                        }
                }
                g_free (uid);
        }

        dir = g_dir_open (trash_files_dir, 0, NULL);
        if (dir) {
                if (g_dir_read_name (dir))
                        has_trash = TRUE;
                g_dir_close (dir);
        }

        g_free (trash_files_dir);

        return has_trash;
}

static void
ldsm_analyze_path (const gchar *path)
{
        const gchar *argv[] = { DISK_SPACE_ANALYZER, path, NULL };

        g_spawn_async (NULL, (gchar **) argv, NULL, G_SPAWN_SEARCH_PATH,
                        NULL, NULL, NULL, NULL);
}

static gboolean
ldsm_notify_for_mount (LdsmMountInfo *mount,
                       gboolean       multiple_volumes,
                       gboolean       other_usable_volumes)
{
        gchar  *name, *program;
        gint64 free_space;
        gint response;
        gboolean has_trash;
        gboolean has_disk_analyzer;
        gboolean retval = TRUE;
        const gchar *path;

        /* Don't show a dialog if one is already displayed */
        if (dialog)
                return retval;

        name = g_unix_mount_guess_name (mount->mount);
        free_space = (gint64) mount->buf.f_frsize * (gint64) mount->buf.f_bavail;
        has_trash = ldsm_mount_has_trash (mount);
        path = g_unix_mount_get_mount_path (mount->mount);

        program = g_find_program_in_path (DISK_SPACE_ANALYZER);
        has_disk_analyzer = (program != NULL);
        g_free (program);

        dialog = gsd_ldsm_dialog_new (other_usable_volumes,
                                      multiple_volumes,
                                      has_disk_analyzer,
                                      has_trash,
                                      free_space,
                                      name,
                                      path);

        g_free (name);

        g_object_ref (G_OBJECT (dialog));
        response = gtk_dialog_run (GTK_DIALOG (dialog));

        gtk_object_destroy (GTK_OBJECT (dialog));
        dialog = NULL;

        switch (response) {
        case GTK_RESPONSE_CANCEL:
                retval = FALSE;
                break;
        case GSD_LDSM_DIALOG_RESPONSE_ANALYZE:
                retval = FALSE;
                ldsm_analyze_path (g_unix_mount_get_mount_path (mount->mount));
                break;
        case GSD_LDSM_DIALOG_RESPONSE_EMPTY_TRASH:
                retval = TRUE;
                gsd_ldsm_trash_empty ();
                break;
        case GTK_RESPONSE_NONE:
        case GTK_RESPONSE_DELETE_EVENT:
                retval = TRUE;
                break;
        default:
                g_assert_not_reached ();
        }

        return retval;
}

static gboolean
ldsm_mount_has_space (LdsmMountInfo *mount)
{
        gdouble free_space;

        free_space = (double) mount->buf.f_bavail / (double) mount->buf.f_blocks;
        /* enough free space, nothing to do */
        if (free_space > free_percent_notify)
                return TRUE;
                
        if (((gint64) mount->buf.f_frsize * (gint64) mount->buf.f_bavail) > ((gint64) free_size_gb_no_notify * GIGABYTE))
                return TRUE;

        /* If we got here, then this volume is low on space */
        return FALSE;
}

static gboolean
ldsm_mount_is_virtual (LdsmMountInfo *mount)
{
        if (mount->buf.f_blocks == 0) {
                /* Filesystems with zero blocks are virtual */
                return TRUE;
        }

        return FALSE;
}

static gint
ldsm_ignore_path_compare (gconstpointer a,
                          gconstpointer b)
{
        return g_strcmp0 ((const gchar *)a, (const gchar *)b);
}

static gboolean
ldsm_mount_is_user_ignore (const gchar *path)
{
        if (g_slist_find_custom (ignore_paths, path, (GCompareFunc) ldsm_ignore_path_compare) != NULL)
                return TRUE;
        else
                return FALSE;
}                


static gboolean
is_in (const gchar *value, const gchar *set[])
{
        int i;
        for (i = 0; set[i] != NULL; i++)
        {
              if (strcmp (set[i], value) == 0)
                return TRUE;
        }
        return FALSE;
}

static gboolean
ldsm_mount_should_ignore (GUnixMountEntry *mount)
{
        const gchar *fs, *device, *path;
        
        path = g_unix_mount_get_mount_path (mount);
        if (ldsm_mount_is_user_ignore (path))
                return TRUE;
        
        /* This is borrowed from GLib and used as a way to determine
         * which mounts we should ignore by default. GLib doesn't
         * expose this in a way that allows it to be used for this
         * purpose
         */
                 
        const gchar *ignore_fs[] = {
                "auto",
                "autofs",
                "devfs",
                "devpts",
                "ecryptfs",
                "kernfs",
                "linprocfs",
                "proc",
                "procfs",
                "ptyfs",
                "selinuxfs",
                "linsysfs",
                "sysfs",
                "tmpfs",
                "usbfs",
                "nfsd",
                "rpc_pipefs",
                "zfs",
                NULL
        };
        const gchar *ignore_devices[] = {
                "none",
                "sunrpc",
                "devpts",
                "nfsd",
                "/dev/loop",
                "/dev/vn",
                NULL
        };
        
        fs = g_unix_mount_get_fs_type (mount);
        device = g_unix_mount_get_device_path (mount);
        
        if (is_in (fs, ignore_fs))
                return TRUE;
  
        if (is_in (device, ignore_devices))
                return TRUE;

        return FALSE;
}

static void
ldsm_free_mount_info (gpointer data)
{
        LdsmMountInfo *mount = data;

        g_return_if_fail (mount != NULL);

        g_unix_mount_free (mount->mount);
        g_free (mount);
}

static void
ldsm_maybe_warn_mounts (GList *mounts,
                        gboolean multiple_volumes,
                        gboolean other_usable_volumes)
{
        GList *l;
        gboolean done = FALSE;

        for (l = mounts; l != NULL; l = l->next) {
                LdsmMountInfo *mount_info = l->data;
                LdsmMountInfo *previous_mount_info;
                gdouble free_space;
                gdouble previous_free_space;
                time_t curr_time;
                const gchar *path;
                gboolean show_notify;

                if (done) {
                        /* Don't show any more dialogs if the user took action with the last one. The user action
                         * might free up space on multiple volumes, making the next dialog redundant.
                         */
                        ldsm_free_mount_info (mount_info);
                        continue;
                }

                path = g_unix_mount_get_mount_path (mount_info->mount);

                previous_mount_info = g_hash_table_lookup (ldsm_notified_hash, path);
                if (previous_mount_info != NULL)
                        previous_free_space = (gdouble) previous_mount_info->buf.f_bavail / (gdouble) previous_mount_info->buf.f_blocks;

                free_space = (gdouble) mount_info->buf.f_bavail / (gdouble) mount_info->buf.f_blocks;

                if (previous_mount_info == NULL) {
                        /* We haven't notified for this mount yet */
                        show_notify = TRUE;
                        mount_info->notify_time = time (NULL);
                        g_hash_table_replace (ldsm_notified_hash, g_strdup (path), mount_info);
                } else if ((previous_free_space - free_space) > free_percent_notify_again) {
                        /* We've notified for this mount before and free space has decreased sufficiently since last time to notify again */
                        curr_time = time (NULL);
                        if (difftime (curr_time, previous_mount_info->notify_time) > (gdouble)(min_notify_period * 60)) {
                                show_notify = TRUE;
                                mount_info->notify_time = curr_time;
                        } else {
                                /* It's too soon to show the dialog again. However, we still replace the LdsmMountInfo
                                 * struct in the hash table, but give it the notfiy time from the previous dialog.
                                 * This will stop the notification from reappearing unnecessarily as soon as the timeout expires.
                                 */
                                show_notify = FALSE;
                                mount_info->notify_time = previous_mount_info->notify_time;
                        }
                        g_hash_table_replace (ldsm_notified_hash, g_strdup (path), mount_info);
                } else {
                        /* We've notified for this mount before, but the free space hasn't decreased sufficiently to notify again */
                        ldsm_free_mount_info (mount_info);
                        show_notify = FALSE;
                }

                if (show_notify) {
                        if (ldsm_notify_for_mount (mount_info, multiple_volumes, other_usable_volumes))
                                done = TRUE;
                }
        }
}

static gboolean
ldsm_check_all_mounts (gpointer data)
{
        GList *mounts;
        GList *l;
        GList *check_mounts = NULL;
        GList *full_mounts = NULL;
        guint number_of_mounts;
        guint number_of_full_mounts;
        gboolean multiple_volumes = FALSE;
        gboolean other_usable_volumes = FALSE;

        /* We iterate through the static mounts in /etc/fstab first, seeing if
         * they're mounted by checking if the GUnixMountPoint has a corresponding GUnixMountEntry.
         * Iterating through the static mounts means we automatically ignore dynamically mounted media.
         */
        mounts = g_unix_mount_points_get (time_read);

        for (l = mounts; l != NULL; l = l->next) {
                GUnixMountPoint *mount_point = l->data;
                GUnixMountEntry *mount;
                LdsmMountInfo *mount_info;
                const gchar *path;

                path = g_unix_mount_point_get_mount_path (mount_point);
                mount = g_unix_mount_at (path, time_read);
                g_unix_mount_point_free (mount_point);
                if (mount == NULL) {
                        /* The GUnixMountPoint is not mounted */
                        continue;
                }

                mount_info = g_new0 (LdsmMountInfo, 1);
                mount_info->mount = mount;

                path = g_unix_mount_get_mount_path (mount);

                if (g_unix_mount_is_readonly (mount)) {
                        ldsm_free_mount_info (mount_info);
                        continue;
                }

                if (ldsm_mount_should_ignore (mount)) {
                        ldsm_free_mount_info (mount_info);
                        continue;
                }

                if (statvfs (path, &mount_info->buf) != 0) {
                        ldsm_free_mount_info (mount_info);
                        continue;
                }

                if (ldsm_mount_is_virtual (mount_info)) {
                        ldsm_free_mount_info (mount_info);
                        continue;
                }

                check_mounts = g_list_prepend (check_mounts, mount_info);
        }

        number_of_mounts = g_list_length (check_mounts);
        if (number_of_mounts > 1)
                multiple_volumes = TRUE;

        for (l = check_mounts; l != NULL; l = l->next) {
                LdsmMountInfo *mount_info = l->data;

                if (!ldsm_mount_has_space (mount_info)) {
                        full_mounts = g_list_prepend (full_mounts, mount_info);
                } else {
                        g_hash_table_remove (ldsm_notified_hash, g_unix_mount_get_mount_path (mount_info->mount));
                        ldsm_free_mount_info (mount_info);
                }
        }

        number_of_full_mounts = g_list_length (full_mounts);
        if (number_of_mounts > number_of_full_mounts)
                other_usable_volumes = TRUE;

        ldsm_maybe_warn_mounts (full_mounts, multiple_volumes,
                                other_usable_volumes);

        g_list_free (check_mounts);
        g_list_free (full_mounts);

        return TRUE;
}

static gboolean
ldsm_is_hash_item_not_in_mounts (gpointer key,
                                 gpointer value,
                                 gpointer user_data)
{
        GList *l;

        for (l = (GList *) user_data; l != NULL; l = l->next) {
                GUnixMountEntry *mount = l->data;
                const char *path;

                path = g_unix_mount_get_mount_path (mount);

                if (strcmp (path, key) == 0)
                        return FALSE;
        }

        return TRUE;
}

static void
ldsm_mounts_changed (GObject  *monitor,
                     gpointer  data)
{
        GList *mounts;

        /* remove the saved data for mounts that got removed */
        mounts = g_unix_mounts_get (time_read);
        g_hash_table_foreach_remove (ldsm_notified_hash,
                                     ldsm_is_hash_item_not_in_mounts, mounts);
        g_list_foreach (mounts, (GFunc) g_unix_mount_free, NULL);

        /* check the status now, for the new mounts */
        ldsm_check_all_mounts (NULL);

        /* and reset the timeout */
        if (ldsm_timeout_id)
                g_source_remove (ldsm_timeout_id);
        ldsm_timeout_id = g_timeout_add_seconds (CHECK_EVERY_X_SECONDS,
                                                 ldsm_check_all_mounts, NULL);
}

static gboolean
ldsm_is_hash_item_in_ignore_paths (gpointer key,
                                   gpointer value,
                                   gpointer user_data)
{
        return ldsm_mount_is_user_ignore (key);
}

static void
gsd_ldsm_get_config ()
{
        GError *error = NULL;

        free_percent_notify = mateconf_client_get_float (client,
                                                      MATECONF_HOUSEKEEPING_DIR "/" MATECONF_FREE_PC_NOTIFY_KEY,
                                                      &error);
        if (error != NULL) {
                g_warning ("Error reading configuration from MateConf: %s", error->message ? error->message : "Unknown error");
                g_clear_error (&error);
        }
        if (free_percent_notify >= 1 || free_percent_notify < 0) {
                g_warning ("Invalid configuration of free_percent_notify: %f\n" \
                           "Using sensible default", free_percent_notify);
                free_percent_notify = 0.05;
        }

        free_percent_notify_again = mateconf_client_get_float (client,
                                                            MATECONF_HOUSEKEEPING_DIR "/" MATECONF_FREE_PC_NOTIFY_AGAIN_KEY,
                                                            &error);
        if (error != NULL) {
                g_warning ("Error reading configuration from MateConf: %s", error->message ? error->message : "Unknown error");
                g_clear_error (&error);
        }
        if (free_percent_notify_again >= 1 || free_percent_notify_again < 0) {
                g_warning ("Invalid configuration of free_percent_notify_again: %f\n" \
                           "Using sensible default\n", free_percent_notify_again);
                free_percent_notify_again = 0.01;
        }

        free_size_gb_no_notify = mateconf_client_get_int (client,
                                                       MATECONF_HOUSEKEEPING_DIR "/" MATECONF_FREE_SIZE_NO_NOTIFY,
                                                       &error);
        if (error != NULL) {
                g_warning ("Error reading configuration from MateConf: %s", error->message ? error->message : "Unknown error");
                g_clear_error (&error);
        }
         min_notify_period = mateconf_client_get_int (client,
                                                   MATECONF_HOUSEKEEPING_DIR "/" MATECONF_MIN_NOTIFY_PERIOD,
                                                   &error);
         if (error != NULL) {
                 g_warning ("Error reading configuration from MateConf: %s", error->message ? error->message : "Unkown error");
                 g_clear_error (&error);
         }

         if (ignore_paths != NULL) {
                g_slist_foreach (ignore_paths, (GFunc) g_free, NULL);
                g_slist_free (ignore_paths);
         }
         ignore_paths = mateconf_client_get_list (client,
                                               MATECONF_HOUSEKEEPING_DIR "/" MATECONF_IGNORE_PATHS,
                                               MATECONF_VALUE_STRING, &error);
         if (error != NULL) {
                 g_warning ("Error reading configuration from MateConf: %s", error->message ? error->message : "Unkown error");
                 g_clear_error (&error);
         } else {
                /* Make sure we dont leave stale entries in ldsm_notified_hash */
                 g_hash_table_foreach_remove (ldsm_notified_hash,
                                              ldsm_is_hash_item_in_ignore_paths, NULL);
         }
}

static void
gsd_ldsm_update_config (MateConfClient *client,
                        guint cnxn_id,
                        MateConfEntry *entry,
                        gpointer user_data)
{
        gsd_ldsm_get_config ();
}

void
gsd_ldsm_setup (gboolean check_now)
{
        GError          *error = NULL;

        if (ldsm_notified_hash || ldsm_timeout_id || ldsm_monitor) {
                g_warning ("Low disk space monitor already initialized.");
                return;
        }

        ldsm_notified_hash = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                    g_free,
                                                    ldsm_free_mount_info);

        client = mateconf_client_get_default ();
        if (client != NULL) {
                gsd_ldsm_get_config ();
                mateconf_notify_id = mateconf_client_notify_add (client,
                                                           MATECONF_HOUSEKEEPING_DIR,
                                                           (MateConfClientNotifyFunc) gsd_ldsm_update_config,
                                                           NULL, NULL, &error);
                if (error != NULL) {
                        g_warning ("Cannot register callback for MateConf notification");
                        g_clear_error (&error);
                }
        } else {
                g_warning ("Failed to get default client");
        }

        ldsm_monitor = g_unix_mount_monitor_new ();
        g_unix_mount_monitor_set_rate_limit (ldsm_monitor, 1000);
        g_signal_connect (ldsm_monitor, "mounts-changed",
                          G_CALLBACK (ldsm_mounts_changed), NULL);

        if (check_now)
                ldsm_check_all_mounts (NULL);

        ldsm_timeout_id = g_timeout_add_seconds (CHECK_EVERY_X_SECONDS,
                                                 ldsm_check_all_mounts, NULL);

}

void
gsd_ldsm_clean (void)
{
        if (ldsm_timeout_id)
                g_source_remove (ldsm_timeout_id);
        ldsm_timeout_id = 0;

        if (ldsm_notified_hash)
                g_hash_table_destroy (ldsm_notified_hash);
        ldsm_notified_hash = NULL;

        if (ldsm_monitor)
                g_object_unref (ldsm_monitor);
        ldsm_monitor = NULL;

        if (client) {
                mateconf_client_notify_remove (client, mateconf_notify_id);
                g_object_unref (client);
        }

        if (dialog) {
                gtk_widget_destroy (GTK_WIDGET (dialog));
                dialog = NULL;
        }

        if (ignore_paths) {
                g_slist_foreach (ignore_paths, (GFunc) g_free, NULL);
                g_slist_free (ignore_paths);
        }
}

#ifdef TEST
int
main (int    argc,
      char **argv)
{
        GMainLoop *loop;

        gtk_init (&argc, &argv);

        loop = g_main_loop_new (NULL, FALSE);

        gsd_ldsm_setup (TRUE);

        g_main_loop_run (loop);

        gsd_ldsm_clean ();
        g_main_loop_unref (loop);

        return 0;
}
#endif /* TEST */
