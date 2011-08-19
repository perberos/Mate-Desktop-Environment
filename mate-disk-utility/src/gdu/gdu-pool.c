/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-pool.c
 *
 * Copyright (C) 2007 David Zeuthen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include <dbus/dbus-glib.h>
#include <string.h>
#include <stdlib.h>

#include "gdu-pool.h"
#include "gdu-presentable.h"
#include "gdu-device.h"
#include "gdu-adapter.h"
#include "gdu-expander.h"
#include "gdu-port.h"
#include "gdu-drive.h"
#include "gdu-linux-md-drive.h"
#include "gdu-volume.h"
#include "gdu-volume-hole.h"
#include "gdu-hub.h"
#include "gdu-known-filesystem.h"
#include "gdu-private.h"
#include "gdu-linux-lvm2-volume-group.h"
#include "gdu-linux-lvm2-volume.h"
#include "gdu-linux-lvm2-volume-hole.h"

#include "gdu-ssh-bridge.h"
#include "gdu-error.h"

#include "udisks-daemon-glue.h"
#include "gdu-marshal.h"

/**
 * SECTION:gdu-pool
 * @title: GduPool
 * @short_description: Enumerate and monitor storage devices
 *
 * The #GduPool object represents a connection to the udisks daemon.
 */

enum {
        DISCONNECTED,
        DEVICE_ADDED,
        DEVICE_REMOVED,
        DEVICE_CHANGED,
        DEVICE_JOB_CHANGED,
        ADAPTER_ADDED,
        ADAPTER_REMOVED,
        ADAPTER_CHANGED,
        EXPANDER_ADDED,
        EXPANDER_REMOVED,
        EXPANDER_CHANGED,
        PORT_ADDED,
        PORT_REMOVED,
        PORT_CHANGED,
        PRESENTABLE_ADDED,
        PRESENTABLE_REMOVED,
        PRESENTABLE_CHANGED,
        PRESENTABLE_JOB_CHANGED,
        LAST_SIGNAL,
};

static GObjectClass *parent_class = NULL;
static guint signals[LAST_SIGNAL] = { 0 };

static void _gdu_pool_disconnect (GduPool *pool);

struct _GduPoolPrivate
{
        gboolean is_disconnected;

        GduPresentable *machine;

        gchar *ssh_user_name;
        gchar *ssh_address;
        GPid ssh_pid;
        guint ssh_child_watch_id;

        DBusGConnection *bus;
        DBusGProxy *proxy;

        char *daemon_version;
        gboolean supports_luks_devices;
        GList *known_filesystems;

        /* the current set of presentables we know about */
        GList *presentables;

        /* the current set of devices we know about */
        GHashTable *object_path_to_device;

        /* the current set of adapters we know about */
        GHashTable *object_path_to_adapter;

        /* the current set of expanders we know about */
        GHashTable *object_path_to_expander;

        /* the current set of ports we know about */
        GHashTable *object_path_to_port;
};

G_DEFINE_TYPE (GduPool, gdu_pool, G_TYPE_OBJECT);

static void remove_all_objects_and_dbus_proxies (GduPool *pool);

static void
gdu_pool_finalize (GduPool *pool)
{
        g_print ("in gdu_pool_finalize()\n");

        remove_all_objects_and_dbus_proxies (pool);

        g_hash_table_unref (pool->priv->object_path_to_device);
        g_hash_table_unref (pool->priv->object_path_to_adapter);
        g_hash_table_unref (pool->priv->object_path_to_expander);
        g_hash_table_unref (pool->priv->object_path_to_port);

        if (pool->priv->ssh_child_watch_id > 0) {
                g_source_remove (pool->priv->ssh_child_watch_id);
                pool->priv->ssh_child_watch_id = 0;
        }
        if (pool->priv->ssh_pid > 0) {
                kill (pool->priv->ssh_pid, SIGTERM);
                pool->priv->ssh_pid = 0;
        }

        g_object_unref (pool->priv->machine);

        if (G_OBJECT_CLASS (parent_class)->finalize)
                (* G_OBJECT_CLASS (parent_class)->finalize) (G_OBJECT (pool));
}

static void
gdu_pool_class_init (GduPoolClass *klass)
{
        GObjectClass *obj_class = (GObjectClass *) klass;

        parent_class = g_type_class_peek_parent (klass);

        obj_class->finalize = (GObjectFinalizeFunc) gdu_pool_finalize;

        g_type_class_add_private (klass, sizeof (GduPoolPrivate));

        /**
         * GduPool::disconnected
         * @pool: The #GduPool emitting the signal.
         *
         * Emitted when the underlying connection has been disconnected.
         *
         * If you hold a reference to @pool, now is a good time to give it up.
         */
        signals[DISCONNECTED] =
                g_signal_new ("disconnected",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GduPoolClass, disconnected),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);

        /**
         * GduPool::device-added
         * @pool: The #GduPool emitting the signal.
         * @device: The #GduDevice that was added.
         *
         * Emitted when @device is added to @pool.
         **/
        signals[DEVICE_ADDED] =
                g_signal_new ("device-added",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GduPoolClass, device_added),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__OBJECT,
                              G_TYPE_NONE, 1,
                              GDU_TYPE_DEVICE);

        /**
         * GduPool::device-removed
         * @pool: The #GduPool emitting the signal.
         * @device: The #GduDevice that was removed.
         *
         * Emitted when @device is removed from @pool. Recipients
         * should release references to @device.
         **/
        signals[DEVICE_REMOVED] =
                g_signal_new ("device-removed",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GduPoolClass, device_removed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__OBJECT,
                              G_TYPE_NONE, 1,
                              GDU_TYPE_DEVICE);

        /**
         * GduPool::device-changed
         * @pool: The #GduPool emitting the signal.
         * @device: A #GduDevice.
         *
         * Emitted when @device is changed.
         **/
        signals[DEVICE_CHANGED] =
                g_signal_new ("device-changed",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GduPoolClass, device_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__OBJECT,
                              G_TYPE_NONE, 1,
                              GDU_TYPE_DEVICE);

        /**
         * GduPool::device-job-changed
         * @pool: The #GduPool emitting the signal.
         * @device: A #GduDevice.
         *
         * Emitted when job status on @device changes.
         **/
        signals[DEVICE_JOB_CHANGED] =
                g_signal_new ("device-job-changed",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GduPoolClass, device_job_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__OBJECT,
                              G_TYPE_NONE, 1,
                              GDU_TYPE_DEVICE);

        /**
         * GduPool::adapter-added
         * @pool: The #GduPool emitting the signal.
         * @adapter: The #GduAdapter that was added.
         *
         * Emitted when @adapter is added to @pool.
         **/
        signals[ADAPTER_ADDED] =
                g_signal_new ("adapter-added",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GduPoolClass, adapter_added),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__OBJECT,
                              G_TYPE_NONE, 1,
                              GDU_TYPE_ADAPTER);

        /**
         * GduPool::adapter-removed
         * @pool: The #GduPool emitting the signal.
         * @adapter: The #GduAdapter that was removed.
         *
         * Emitted when @adapter is removed from @pool. Recipients
         * should release references to @adapter.
         **/
        signals[ADAPTER_REMOVED] =
                g_signal_new ("adapter-removed",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GduPoolClass, adapter_removed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__OBJECT,
                              G_TYPE_NONE, 1,
                              GDU_TYPE_ADAPTER);

        /**
         * GduPool::adapter-changed
         * @pool: The #GduPool emitting the signal.
         * @adapter: A #GduAdapter.
         *
         * Emitted when @adapter is changed.
         **/
        signals[ADAPTER_CHANGED] =
                g_signal_new ("adapter-changed",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GduPoolClass, adapter_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__OBJECT,
                              G_TYPE_NONE, 1,
                              GDU_TYPE_ADAPTER);

        /**
         * GduPool::expander-added
         * @pool: The #GduPool emitting the signal.
         * @expander: The #GduExpander that was added.
         *
         * Emitted when @expander is added to @pool.
         **/
        signals[EXPANDER_ADDED] =
                g_signal_new ("expander-added",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GduPoolClass, expander_added),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__OBJECT,
                              G_TYPE_NONE, 1,
                              GDU_TYPE_EXPANDER);

        /**
         * GduPool::expander-removed
         * @pool: The #GduPool emitting the signal.
         * @expander: The #GduExpander that was removed.
         *
         * Emitted when @expander is removed from @pool. Recipients
         * should release references to @expander.
         **/
        signals[EXPANDER_REMOVED] =
                g_signal_new ("expander-removed",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GduPoolClass, expander_removed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__OBJECT,
                              G_TYPE_NONE, 1,
                              GDU_TYPE_EXPANDER);

        /**
         * GduPool::expander-changed
         * @pool: The #GduPool emitting the signal.
         * @expander: A #GduExpander.
         *
         * Emitted when @expander is changed.
         **/
        signals[EXPANDER_CHANGED] =
                g_signal_new ("expander-changed",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GduPoolClass, expander_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__OBJECT,
                              G_TYPE_NONE, 1,
                              GDU_TYPE_EXPANDER);

        /**
         * GduPool::port-added
         * @pool: The #GduPool emitting the signal.
         * @port: The #GduPort that was added.
         *
         * Emitted when @port is added to @pool.
         **/
        signals[PORT_ADDED] =
                g_signal_new ("port-added",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GduPoolClass, port_added),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__OBJECT,
                              G_TYPE_NONE, 1,
                              GDU_TYPE_PORT);

        /**
         * GduPool::port-removed
         * @pool: The #GduPool emitting the signal.
         * @port: The #GduPort that was removed.
         *
         * Emitted when @port is removed from @pool. Recipients
         * should release references to @port.
         **/
        signals[PORT_REMOVED] =
                g_signal_new ("port-removed",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GduPoolClass, port_removed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__OBJECT,
                              G_TYPE_NONE, 1,
                              GDU_TYPE_PORT);

        /**
         * GduPool::port-changed
         * @pool: The #GduPool emitting the signal.
         * @port: A #GduPort.
         *
         * Emitted when @port is changed.
         **/
        signals[PORT_CHANGED] =
                g_signal_new ("port-changed",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GduPoolClass, port_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__OBJECT,
                              G_TYPE_NONE, 1,
                              GDU_TYPE_PORT);


        /**
         * GduPool::presentable-added
         * @pool: The #GduPool emitting the signal.
         * @presentable: The #GduPresentable that was added.
         *
         * Emitted when @presentable is added to @pool.
         **/
        signals[PRESENTABLE_ADDED] =
                g_signal_new ("presentable-added",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GduPoolClass, presentable_added),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__OBJECT,
                              G_TYPE_NONE, 1,
                              GDU_TYPE_PRESENTABLE);

        /**
         * GduPool::presentable-removed
         * @pool: The #GduPool emitting the signal.
         * @presentable: The #GduPresentable that was removed.
         *
         * Emitted when @presentable is removed from @pool. Recipients
         * should release references to @presentable.
         **/
        signals[PRESENTABLE_REMOVED] =
                g_signal_new ("presentable-removed",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GduPoolClass, presentable_removed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__OBJECT,
                              G_TYPE_NONE, 1,
                              GDU_TYPE_PRESENTABLE);

        /**
         * GduPool::presentable-changed
         * @pool: The #GduPool emitting the signal.
         * @presentable: A #GduPresentable.
         *
         * Emitted when @presentable changes.
         **/
        signals[PRESENTABLE_CHANGED] =
                g_signal_new ("presentable-changed",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GduPoolClass, presentable_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__OBJECT,
                              G_TYPE_NONE, 1,
                              GDU_TYPE_PRESENTABLE);

        /**
         * GduPool::presentable-job-changed
         * @pool: The #GduPool emitting the signal.
         * @presentable: A #GduPresentable.
         *
         * Emitted when job status on @presentable changes.
         **/
        signals[PRESENTABLE_CHANGED] =
                g_signal_new ("presentable-job-changed",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GduPoolClass, presentable_job_changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__OBJECT,
                              G_TYPE_NONE, 1,
                              GDU_TYPE_PRESENTABLE);
}

static void
gdu_log_func (const gchar   *log_domain,
              GLogLevelFlags log_level,
              const gchar   *message,
              gpointer       user_data)
{
        gboolean show_debug;
        const gchar *gdu_debug_var;

        gdu_debug_var = g_getenv ("GDU_DEBUG");
        show_debug = (g_strcmp0 (gdu_debug_var, "1") == 0);

        if (G_LIKELY (!show_debug))
                goto out;

        g_print ("%s: %s\n",
                 G_LOG_DOMAIN,
                 message);
 out:
        ;
}

static void
gdu_pool_init (GduPool *pool)
{
        static gboolean log_handler_initialized = FALSE;

        if (!log_handler_initialized) {
                g_log_set_handler (G_LOG_DOMAIN,
                                   G_LOG_LEVEL_MESSAGE | G_LOG_LEVEL_INFO | G_LOG_LEVEL_DEBUG,
                                   gdu_log_func,
                                   NULL);
                log_handler_initialized = TRUE;
        }

        pool->priv = G_TYPE_INSTANCE_GET_PRIVATE (pool, GDU_TYPE_POOL, GduPoolPrivate);

        pool->priv->object_path_to_device = g_hash_table_new_full (g_str_hash,
                                                                   g_str_equal,
                                                                   NULL,
                                                                   g_object_unref);

        pool->priv->object_path_to_adapter = g_hash_table_new_full (g_str_hash,
                                                                    g_str_equal,
                                                                    NULL,
                                                                    g_object_unref);

        pool->priv->object_path_to_expander = g_hash_table_new_full (g_str_hash,
                                                                     g_str_equal,
                                                                     NULL,
                                                                     g_object_unref);

        pool->priv->object_path_to_port = g_hash_table_new_full (g_str_hash,
                                                                 g_str_equal,
                                                                 NULL,
                                                                 g_object_unref);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
diff_sorted_lists (GList         *list1,
                   GList         *list2,
                   GCompareFunc   compare,
                   GList        **added,
                   GList        **removed)
{
  int order;

  *added = *removed = NULL;

  while (list1 != NULL &&
         list2 != NULL)
    {
      order = (*compare) (list1->data, list2->data);
      if (order < 0)
        {
          *removed = g_list_prepend (*removed, list1->data);
          list1 = list1->next;
        }
      else if (order > 0)
        {
          *added = g_list_prepend (*added, list2->data);
          list2 = list2->next;
        }
      else
        { /* same item */
          list1 = list1->next;
          list2 = list2->next;
        }
    }

  while (list1 != NULL)
    {
      *removed = g_list_prepend (*removed, list1->data);
      list1 = list1->next;
    }
  while (list2 != NULL)
    {
      *added = g_list_prepend (*added, list2->data);
      list2 = list2->next;
    }
}

/* note: does not ref the result */
static GduPresentable *
find_presentable_by_object_path (GList *presentables, const gchar *object_path)
{
        GduPresentable *ret;
        GList *l;

        ret = NULL;

        for (l = presentables; l != NULL; l = l->next) {
                GduPresentable *p = GDU_PRESENTABLE (l->data);
                GduDevice *d;
                const gchar *d_object_path;

                d = gdu_presentable_get_device (p);
                if (d == NULL)
                        continue;

                d_object_path = gdu_device_get_object_path (d);
                g_object_unref (d);

                if (g_strcmp0 (object_path, d_object_path) == 0) {
                        ret = p;
                        goto out;
                }
        }

 out:
        return ret;
}

static gboolean
is_msdos_extended_partition (GduDevice *device)
{
        gboolean ret;
        gint type;

        ret = FALSE;

        if (!gdu_device_is_partition (device))
                goto out;

        if (g_strcmp0 (gdu_device_partition_get_scheme (device), "mbr") != 0)
                goto out;

        type = strtol (gdu_device_partition_get_type (device), NULL, 0);
        if (!(type == 0x05 || type == 0x0f || type == 0x85))
                goto out;

        ret = TRUE;

 out:
        return ret;
}

typedef struct {
        int number;
        guint64 offset;
        guint64 size;
} PartEntry;

static int
part_entry_compare (PartEntry *pa, PartEntry *pb, gpointer user_data)
{
        if (pa->offset > pb->offset)
                return 1;
        if (pa->offset < pb->offset)
                return -1;
        return 0;
}

static GList *
get_holes (GduPool        *pool,
           GList          *devices,
           GduDrive       *drive,
           GduDevice      *drive_device,
           GduPresentable *enclosed_in,
           gboolean        ignore_logical,
           guint64         start,
           guint64         size)
{
        GList *ret;
        gint n;
        gint num_entries;
        PartEntry *entries;
        guint64 cursor;
        guint64 gap_size;
        guint64 gap_position;
        const char *scheme;
        GList *l;

        ret = NULL;
        entries = NULL;

        /* no point if adding holes if there's no media */
        if (!gdu_device_is_media_available (drive_device))
                goto out;

        /* neither if the media isn't partitioned */
        if (!gdu_device_is_partition_table (drive_device))
                goto out;

        /*g_debug ("Adding holes for %s between %" G_GUINT64_FORMAT
                 " and %" G_GUINT64_FORMAT " (ignore_logical=%d)",
                 gdu_device_get_device_file (drive_device),
                 start,
                 start + size,
                 ignore_logical);*/

        scheme = gdu_device_partition_table_get_scheme (drive_device);

        /* find the offsets and sizes of existing partitions of the partition table */
        GArray *entries_array;
        entries_array = g_array_new (FALSE, FALSE, sizeof (PartEntry));
        num_entries = 0;
        for (l = devices; l != NULL; l = l->next) {
                GduDevice *partition_device = GDU_DEVICE (l->data);
                guint64 partition_offset;
                guint64 partition_size;
                guint partition_number;

                if (!gdu_device_is_partition (partition_device))
                        continue;
                if (g_strcmp0 (gdu_device_get_object_path (drive_device),
                               gdu_device_partition_get_slave (partition_device)) != 0)
                        continue;

                partition_offset = gdu_device_partition_get_offset (partition_device);
                partition_size = gdu_device_partition_get_size (partition_device);
                partition_number = gdu_device_partition_get_number (partition_device);

                //g_print ("  considering partition number %d at offset=%lldMB size=%lldMB\n",
                //         partition_number,
                //         partition_offset / (1000 * 1000),
                //         partition_size / (1000 * 1000));

                /* only consider partitions in the given space */
                if (partition_offset <= start)
                        continue;
                if (partition_offset >= start + size)
                        continue;

                /* ignore logical partitions if requested */
                if (ignore_logical) {
                        if (strcmp (scheme, "mbr") == 0 && partition_number > 4)
                                continue;
                }

                g_array_set_size (entries_array, num_entries + 1);

                g_array_index (entries_array, PartEntry, num_entries).number = partition_number;
                g_array_index (entries_array, PartEntry, num_entries).offset = partition_offset;
                g_array_index (entries_array, PartEntry, num_entries).size = partition_size;

                num_entries++;
        }
        entries = (PartEntry *) g_array_free (entries_array, FALSE);

        g_qsort_with_data (entries, num_entries, sizeof (PartEntry), (GCompareDataFunc) part_entry_compare, NULL);

        //g_print (" %s: start=%lldMB size=%lldMB num_entries=%d\n",
        //         gdu_device_get_device_file (drive_device),
        //         start / (1000 * 1000),
        //         size / (1000 * 1000),
        //         num_entries);
        for (n = 0, cursor = start; n <= num_entries; n++) {
                if (n < num_entries) {
                        //g_print ("  %d: %d: offset=%lldMB size=%lldMB\n",
                        //         n,
                        //         entries[n].number,
                        //         entries[n].offset / (1000 * 1000),
                        //         entries[n].size / (1000 * 1000));

                        gap_size = entries[n].offset - cursor;
                        gap_position = entries[n].offset - gap_size;
                        cursor = entries[n].offset + entries[n].size;
                } else {
                        //g_print ("  trailing: cursor=%lldMB\n",
                        //         cursor / (1000 * 1000));

                        /* trailing free space */
                        gap_size = start + size - cursor;
                        gap_position = start + size - gap_size;
                }

                /* ignore unallocated space that is less than 1% of the drive */
                if (gap_size >= gdu_device_get_size (drive_device) / 100) {
                        GduVolumeHole *hole;
                        //g_print ("  adding %lldMB gap at %lldMB\n",
                        //         gap_size / (1000 * 1000),
                        //         gap_position / (1000 * 1000));

                        hole = _gdu_volume_hole_new (pool, gap_position, gap_size, enclosed_in);
                        ret = g_list_prepend (ret, hole);
                }

        }

out:
        g_free (entries);
        return ret;
}

static GList *
get_holes_for_drive (GduPool   *pool,
                     GList     *devices,
                     GduDrive  *drive,
                     GduVolume *extended_partition)
{
        GList *ret;
        GduDevice *drive_device;

        ret = NULL;

        drive_device = gdu_presentable_get_device (GDU_PRESENTABLE (drive));

        /* drive_device is NULL for activatable drive that isn't yet activated */
        if (drive_device == NULL)
                goto out;

        /* first add holes between primary partitions */
        ret = get_holes (pool,
                         devices,
                         drive,
                         drive_device,
                         GDU_PRESENTABLE (drive),
                         TRUE,
                         0,
                         gdu_device_get_size (drive_device));

        /* then add holes in the extended partition */
        if (extended_partition != NULL) {
                GList *holes_in_extended_partition;
                GduDevice *extended_partition_device;

                extended_partition_device = gdu_presentable_get_device (GDU_PRESENTABLE (extended_partition));
                if (extended_partition_device == NULL) {
                        g_warning ("No device for extended partition %s",
                                   gdu_presentable_get_id (GDU_PRESENTABLE (extended_partition)));
                        goto out;
                }

                holes_in_extended_partition = get_holes (pool,
                                                         devices,
                                                         drive,
                                                         drive_device,
                                                         GDU_PRESENTABLE (extended_partition),
                                                         FALSE,
                                                         gdu_device_partition_get_offset (extended_partition_device),
                                                         gdu_device_partition_get_size (extended_partition_device));

                ret = g_list_concat (ret, holes_in_extended_partition);

                g_object_unref (extended_partition_device);
        }

 out:
        if (drive_device != NULL)
                g_object_unref (drive_device);
        return ret;
}

static GduPresentable *
ensure_hub (GduPool *pool,
            GduPresentable **hub,
            GList **presentables,
            const gchar *name,
            const gchar *vpd_name,
            const gchar *icon_name)
{
        GIcon *icon;
        GduPresentable *ret;

        g_assert (hub != NULL);
        g_assert (presentables != NULL);

        if (*hub != NULL)
                goto out;

        icon = g_themed_icon_new_with_default_fallbacks (icon_name);
        ret = GDU_PRESENTABLE (_gdu_hub_new (pool,
                                             GDU_HUB_USAGE_MULTI_DISK_DEVICES,
                                             NULL,                    /* adapter */
                                             NULL,                    /* expander */
                                             name,
                                             vpd_name,
                                             icon,
                                             pool->priv->machine));
        g_object_unref (icon);

        *presentables = g_list_prepend (*presentables, ret);

        *hub = ret;
 out:
        return *hub;
}

static GduPresentable *
ensure_hub_multipath (GduPool *pool,
                      GduPresentable **hub,
                      GList **presentables)
{
        return ensure_hub (pool, hub, presentables,
                           _("Multipath Devices"),
                           _("Drives with multiple I/O paths"),
                           "gdu-category-multipath");
}

static GduPresentable *
ensure_hub_raid_lvm (GduPool *pool,
                     GduPresentable **hub,
                     GList **presentables)
{
        return ensure_hub (pool, hub, presentables,
                           _("Multi-disk Devices"),
                           _("RAID, LVM and other logical drives"),
                           "gdu-category-multidisk");
}

static GduPresentable *
ensure_hub_peripheral (GduPool *pool,
                       GduPresentable **hub,
                       GList **presentables)
{
        return ensure_hub (pool, hub, presentables,
                           _("Peripheral Devices"),
                           _("USB, FireWire and other peripherals"),
                           "gdu-category-peripheral");
}

static void
recompute_presentables (GduPool *pool)
{
        GList *l;
        GList *devices;
        GList *adapters;
        GList *expanders;
        GList *new_partitioned_drives;
        GList *new_presentables;
        GList *added_presentables;
        GList *removed_presentables;
        GHashTable *hash_map_from_drive_to_extended_partition;
        GHashTable *hash_map_from_linux_md_uuid_to_drive;
        GHashTable *hash_map_from_linux_lvm2_group_uuid_to_vg;
        GHashTable *hash_map_from_adapter_objpath_to_hub;
        GHashTable *hash_map_from_expander_objpath_to_hub;
        GduPresentable *hub_raid_lvm;
        GduPresentable *hub_multipath;
        GduPresentable *hub_peripheral;

        /* The general strategy for (re-)computing presentables is rather brute force; we
         * compute the complete set of presentables every time and diff it against the
         * presentables we computed the last time. Then we send out add/remove events
         * accordingly.
         *
         * The reason for this brute-force approach is that the GduPresentable entities are
         * somewhat complicated since the whole process involves synthesizing GduVolumeHole and
         * GduLinuxMdDrive objects.
         */

        new_presentables = NULL;
        new_partitioned_drives = NULL;

        new_presentables = g_list_prepend (new_presentables, g_object_ref (pool->priv->machine));

        hash_map_from_drive_to_extended_partition = g_hash_table_new_full ((GHashFunc) gdu_presentable_hash,
                                                                           (GEqualFunc) gdu_presentable_equals,
                                                                           NULL,
                                                                           NULL);

        hash_map_from_linux_md_uuid_to_drive = g_hash_table_new_full (g_str_hash,
                                                                      g_str_equal,
                                                                      NULL,
                                                                      NULL);

        hash_map_from_linux_lvm2_group_uuid_to_vg = g_hash_table_new_full (g_str_hash,
                                                                           g_str_equal,
                                                                           NULL,
                                                                           NULL);

        hash_map_from_adapter_objpath_to_hub = g_hash_table_new_full (g_str_hash,
                                                                      g_str_equal,
                                                                      NULL,
                                                                      NULL);

        hash_map_from_expander_objpath_to_hub = g_hash_table_new_full (g_str_hash,
                                                                       g_str_equal,
                                                                       NULL,
                                                                       NULL);

        hub_raid_lvm = NULL;
        hub_multipath = NULL;
        hub_peripheral = NULL;

        /* First add all HBAs as Hub objects */
        adapters = gdu_pool_get_adapters (pool);
        for (l = adapters; l != NULL; l = l->next) {
                GduAdapter *adapter = GDU_ADAPTER (l->data);
                GduHub *hub;

                hub = _gdu_hub_new (pool,
                                    GDU_HUB_USAGE_ADAPTER,
                                    adapter,
                                    NULL,      /* expander */
                                    NULL,      /* name */
                                    NULL,      /* vpd_name */
                                    NULL,      /* icon */
                                    pool->priv->machine);  /* enclosing_presentable */

                g_hash_table_insert (hash_map_from_adapter_objpath_to_hub,
                                     (gpointer) gdu_adapter_get_object_path (adapter),
                                     hub);

                new_presentables = g_list_prepend (new_presentables, hub);
        } /* for all adapters */

        /* Then all expanders */
        expanders = gdu_pool_get_expanders (pool);
        for (l = expanders; l != NULL; l = l->next) {
                GduExpander *expander = GDU_EXPANDER (l->data);
                GduAdapter *adapter;
                GduHub *hub;
                gchar **port_object_paths;
                GduPresentable *expander_parent;

                /* we are guaranteed that upstream ports all stem from the same expander or
                 * host adapter - so just pick the first one */
                expander_parent = NULL;
                port_object_paths = gdu_expander_get_upstream_ports (expander);
                if (port_object_paths != NULL && port_object_paths[0] != NULL) {
                        GduPort *port;

                        port = gdu_pool_get_port_by_object_path (pool, port_object_paths[0]);

                        /* For now, always choose the adapter as the parent - this is *probably*
                         * the right thing (e.g. what people expect) to do _anyway_ because of
                         * the way expanders are daisy-chained
                         */
                        if (port != NULL) {
                                const gchar *adapter_object_path;
                                adapter_object_path = gdu_port_get_adapter (port);
                                adapter = gdu_pool_get_adapter_by_object_path (pool, adapter_object_path);
                                expander_parent = g_hash_table_lookup (hash_map_from_adapter_objpath_to_hub,
                                                                       adapter_object_path);
                                g_object_unref (port);
                        }
                }

                g_warn_if_fail (expander_parent != NULL);
                g_warn_if_fail (adapter != NULL);

                hub = _gdu_hub_new (pool,
                                    GDU_HUB_USAGE_EXPANDER,
                                    adapter,
                                    expander,
                                    NULL,      /* name */
                                    NULL,      /* vpd_name */
                                    NULL,      /* icon */
                                    expander_parent);
                g_object_unref (adapter);

                g_hash_table_insert (hash_map_from_expander_objpath_to_hub,
                                     (gpointer) gdu_expander_get_object_path (expander),
                                     hub);

                new_presentables = g_list_prepend (new_presentables, hub);
        } /* for all expanders */

        /* TODO: Ensure that pool->priv->devices is in topological sort order, then just loop
         *       through it and handle devices sequentially.
         *
         *       The current approach won't work for a couple of edge cases; notably stacks of devices
         *       e.g. consider a LUKS device inside a LUKS device...
         */
        devices = gdu_pool_get_devices (pool);

        /* Process all devices; the list is sorted in topologically order so we get all deps first */
        for (l = devices; l != NULL; l = l->next) {
                GduDevice *device;

                device = GDU_DEVICE (l->data);

                //g_debug ("Handling device %s", gdu_device_get_device_file (device));

                /* drives */
                if (gdu_device_is_drive (device)) {

                        GduDrive *drive;

                        if (gdu_device_is_linux_md (device)) {
                                const gchar *uuid;
                                GduPresentable *linux_md_parent;

                                uuid = gdu_device_linux_md_get_uuid (device);

                                /* 'clear' and 'inactive' devices may not have an uuid */
                                if (uuid != NULL && strlen (uuid) == 0)
                                        uuid = NULL;

                                /* TODO: Create transient GduHub object for all RAID arrays? */
                                linux_md_parent = ensure_hub_raid_lvm (pool,
                                                                       &hub_raid_lvm,
                                                                       &new_presentables);

                                if (uuid != NULL) {
                                        drive = GDU_DRIVE (_gdu_linux_md_drive_new (pool, uuid, NULL, linux_md_parent));

                                        /* Due to the topological sorting of devices, we are guaranteed that
                                         * that running Linux MD arrays come before the slaves.
                                         */
                                        g_warn_if_fail (g_hash_table_lookup (hash_map_from_linux_md_uuid_to_drive, uuid) == NULL);

                                        g_hash_table_insert (hash_map_from_linux_md_uuid_to_drive,
                                                             (gpointer) uuid,
                                                             drive);
                                } else {
                                        drive = GDU_DRIVE (_gdu_linux_md_drive_new (pool,
                                                                                    NULL,
                                                                                    gdu_device_get_device_file (device),
                                                                                    linux_md_parent));
                                }


                        } else {
                                GduPresentable *drive_parent;
                                gchar **port_object_paths;

                                drive_parent = NULL;

                                /* we are guaranteed that upstream ports all stem from the same expander or
                                 * host adapter - so just pick the first one */
                                port_object_paths = gdu_device_drive_get_ports (device);
                                if (port_object_paths != NULL && port_object_paths[0] != NULL) {
                                        GduPort *port;

                                        port = gdu_pool_get_port_by_object_path (pool, port_object_paths[0]);
                                        /* choose the expander, if available, otherwise the adapter */
                                        if (port != NULL) {
                                                const gchar *parent_object_path;
                                                const gchar *adapter_object_path;

                                                parent_object_path = gdu_port_get_parent (port);
                                                adapter_object_path = gdu_port_get_adapter (port);
                                                if (g_strcmp0 (parent_object_path, adapter_object_path) != 0) {
                                                        drive_parent = g_hash_table_lookup (hash_map_from_expander_objpath_to_hub,
                                                                                            parent_object_path);
                                                } else {
                                                        drive_parent = g_hash_table_lookup (hash_map_from_adapter_objpath_to_hub,
                                                                                            adapter_object_path);
                                                }
                                                g_object_unref (port);
                                        }
                                }

                                /* Group all Multipath devices in the virtual "Multi-path Devices" Hub */
                                if (gdu_device_is_linux_dmmp (device)) {
                                        g_warn_if_fail (drive_parent == NULL);
                                        drive_parent = ensure_hub_multipath (pool,
                                                                             &hub_multipath,
                                                                             &new_presentables);
                                }

                                /* If there's no parent it could be because the device is connected via
                                 * USB, Firewire or SDIO and udisks doesn't generate Adapter or Expander
                                 * objects for it.
                                 *
                                 * We group these devices in the virtual "Peripheral Devices" Hub
                                 */
                                if (drive_parent == NULL) {
                                        drive_parent = ensure_hub_peripheral (pool,
                                                                              &hub_peripheral,
                                                                              &new_presentables);
                                }

                                drive = _gdu_drive_new_from_device (pool, device, drive_parent);
                        }
                        new_presentables = g_list_prepend (new_presentables, drive);

                        if (gdu_device_is_partition_table (device)) {
                                new_partitioned_drives = g_list_prepend (new_partitioned_drives, drive);
                        } else {
                                /* add volume for non-partitioned (e.g. whole-disk) devices if media
                                 * is available and the drive is active
                                 */
                                if (gdu_device_is_media_available (device) && gdu_drive_is_active (drive)) {
                                        GduVolume *volume;
                                        volume = _gdu_volume_new_from_device (pool, device, GDU_PRESENTABLE (drive));
                                        new_presentables = g_list_prepend (new_presentables, volume);
                                }
                        }

                } else if (gdu_device_is_partition (device)) {

                        GduVolume *volume;
                        GduPresentable *enclosing_presentable;

                        if (is_msdos_extended_partition (device)) {
                                enclosing_presentable = find_presentable_by_object_path (new_presentables,
                                                                                         gdu_device_partition_get_slave (device));

                                if (enclosing_presentable == NULL) {
                                        g_warning ("Partition %s claims to be a partition of %s which does not exist",
                                                   gdu_device_get_object_path (device),
                                                   gdu_device_partition_get_slave (device));
                                        continue;
                                }

                                volume = _gdu_volume_new_from_device (pool, device, enclosing_presentable);

                                g_hash_table_insert (hash_map_from_drive_to_extended_partition,
                                                     enclosing_presentable,
                                                     volume);
                        } else {
                                enclosing_presentable = find_presentable_by_object_path (new_presentables,
                                                                                         gdu_device_partition_get_slave (device));
                                if (enclosing_presentable == NULL) {
                                        g_warning ("Partition %s claims to be a partition of %s which does not exist",
                                                   gdu_device_get_object_path (device),
                                                   gdu_device_partition_get_slave (device));
                                        continue;
                                }

                                /* logical partitions should be enclosed by the appropriate extended partition */
                                if (g_strcmp0 (gdu_device_partition_get_scheme (device), "mbr") == 0 &&
                                    gdu_device_partition_get_number (device) >= 5) {

                                        enclosing_presentable = g_hash_table_lookup (hash_map_from_drive_to_extended_partition,
                                                                                     enclosing_presentable);
                                        if (enclosing_presentable == NULL) {
                                                g_warning ("Partition %s is a logical partition but no extended partition exists",
                                                           gdu_device_get_object_path (device));
                                                continue;
                                        }
                                }

                                volume = _gdu_volume_new_from_device (pool, device, enclosing_presentable);

                        }

                        /*g_debug ("%s is enclosed by %s",
                          gdu_device_get_object_path (device),
                          gdu_presentable_get_id (enclosing_presentable));*/

                        new_presentables = g_list_prepend (new_presentables, volume);

                } else if (gdu_device_is_luks_cleartext (device)) {

                        const gchar *luks_cleartext_slave;
                        GduPresentable *enclosing_luks_device;
                        GduVolume *volume;


                        luks_cleartext_slave = gdu_device_luks_cleartext_get_slave (device);

                        enclosing_luks_device = find_presentable_by_object_path (new_presentables, luks_cleartext_slave);
                        if (enclosing_luks_device == NULL) {
                                g_warning ("Cannot find enclosing device %s for LUKS cleartext device %s",
                                           luks_cleartext_slave,
                                           gdu_device_get_object_path (device));
                                continue;
                        }

                        volume = _gdu_volume_new_from_device (pool, device, enclosing_luks_device);
                        new_presentables = g_list_prepend (new_presentables, volume);

                } else if (gdu_device_is_linux_lvm2_lv (device)) {

                        /* Do nothing - this is handled when creating the Lvm2VolumeGroup object below */

                } else {
                        g_warning ("Don't know how to handle device %s", gdu_device_get_device_file (device));
                }

                /* Ensure we have a GduLinuxLvm2VolumeGroup even if the volume group isn't running */
                if (gdu_device_is_linux_lvm2_pv (device) && !gdu_device_should_ignore (device)) {
                        GduLinuxLvm2VolumeGroup *vg;
                        const gchar *vg_uuid;

                        vg_uuid = gdu_device_linux_lvm2_pv_get_group_uuid (device);

                        /* First, see if we have a volume group for this UUID already */
                        vg = g_hash_table_lookup (hash_map_from_linux_lvm2_group_uuid_to_vg, vg_uuid);
                        if (vg == NULL) {
                                gchar **lvs;
                                guint n;
                                guint64 unallocated_size;

                                /* otherwise create one */
                                vg = _gdu_linux_lvm2_volume_group_new (pool,
                                                                       vg_uuid,
                                                                       ensure_hub_raid_lvm (pool,
                                                                                            &hub_raid_lvm,
                                                                                            &new_presentables));
                                g_hash_table_insert (hash_map_from_linux_lvm2_group_uuid_to_vg, (gpointer) vg_uuid, vg);
                                new_presentables = g_list_prepend (new_presentables, vg);

                                /* and create logical volume objects as well */
                                lvs = gdu_device_linux_lvm2_pv_get_group_logical_volumes (device);
                                for (n = 0; lvs != NULL && lvs[n] != NULL; n++) {
                                        const gchar *lv_desc = lvs[n];
                                        gchar **tokens;
                                        gchar *name;
                                        gchar *uuid;
                                        guint64 size;
                                        guint m;

                                        tokens = g_strsplit (lv_desc, ";", 0);
                                        for (m = 0; tokens[m] != NULL; m++) {
                                                /* TODO: we need to unescape values */
                                                if (g_str_has_prefix (tokens[m], "name="))
                                                        name = g_strdup (tokens[m] + 5);
                                                else if (g_str_has_prefix (tokens[m], "uuid="))
                                                        uuid = g_strdup (tokens[m] + 5);
                                                else if (g_str_has_prefix (tokens[m], "size="))
                                                        size = g_ascii_strtoull (tokens[m] + 5, NULL, 10);
                                        }

                                        if (name != NULL && uuid != NULL && size > 0) {
                                                GduLinuxLvm2Volume *volume;

                                                volume = _gdu_linux_lvm2_volume_new (pool,
                                                                                     vg_uuid,
                                                                                     uuid,
                                                                                     GDU_PRESENTABLE (vg));

                                                new_presentables = g_list_prepend (new_presentables, volume);

                                        } else {
                                                g_warning ("Malformed LMV2 LV in group with UUID %s: "
                                                           "pos=%d name=%s uuid=%s size=%" G_GUINT64_FORMAT,
                                                           vg_uuid,
                                                           n,
                                                           name,
                                                           uuid,
                                                           size);
                                        }

                                        g_free (name);
                                        g_free (uuid);
                                        g_strfreev (tokens);
                                } /* foreach LV in VG */

                                /* Create a GduLinuxLvm2VolumeHole for unallocated space - TODO: use 1% or
                                 * something based on extent size... instead of 1MB
                                 */
                                unallocated_size = gdu_device_linux_lvm2_pv_get_group_unallocated_size (device);
                                if (unallocated_size >= 1000 * 1000) {
                                        GduLinuxLvm2VolumeHole *volume_hole;
                                        volume_hole = _gdu_linux_lvm2_volume_hole_new (pool,
                                                                                       GDU_PRESENTABLE (vg));
                                        new_presentables = g_list_prepend (new_presentables, volume_hole);
                                }
                        }
                }

                /* Ensure we have a GduLinuxMdDrive for each non-running arrays */
                if (gdu_device_is_linux_md_component (device) && !gdu_device_should_ignore (device)) {
                        const gchar *uuid;

                        uuid = gdu_device_linux_md_component_get_uuid (device);
                        if (g_hash_table_lookup (hash_map_from_linux_md_uuid_to_drive, uuid) == NULL) {
                                GduDrive *drive;
                                GduPresentable *linux_md_parent;

                                linux_md_parent = ensure_hub_raid_lvm (pool,
                                                                       &hub_raid_lvm,
                                                                       &new_presentables);

                                drive = GDU_DRIVE (_gdu_linux_md_drive_new (pool, uuid, NULL, linux_md_parent));
                                new_presentables = g_list_prepend (new_presentables, drive);

                                g_hash_table_insert (hash_map_from_linux_md_uuid_to_drive,
                                                     (gpointer) uuid,
                                                     drive);
                        }
                }

        } /* For all devices */

        /* now add holes (representing non-partitioned space) for partitioned drives */
        for (l = new_partitioned_drives; l != NULL; l = l->next) {
                GduDrive *drive;
                GduVolume *extended_partition;
                GList *holes;

                drive = GDU_DRIVE (l->data);
                extended_partition = g_hash_table_lookup (hash_map_from_drive_to_extended_partition, drive);

                holes = get_holes_for_drive (pool, devices, drive, extended_partition);

                new_presentables = g_list_concat (new_presentables, holes);
        }

        /* clean up temporary lists / hashes */
        g_list_free (new_partitioned_drives);
        g_hash_table_unref (hash_map_from_drive_to_extended_partition);
        g_hash_table_unref (hash_map_from_linux_md_uuid_to_drive);
        g_hash_table_unref (hash_map_from_linux_lvm2_group_uuid_to_vg);
        g_hash_table_unref (hash_map_from_adapter_objpath_to_hub);
        g_hash_table_unref (hash_map_from_expander_objpath_to_hub);

        /* figure out the diff */
        new_presentables = g_list_sort (new_presentables, (GCompareFunc) gdu_presentable_compare);
        pool->priv->presentables = g_list_sort (pool->priv->presentables, (GCompareFunc) gdu_presentable_compare);
        diff_sorted_lists (pool->priv->presentables,
                           new_presentables,
                           (GCompareFunc) gdu_presentable_compare,
                           &added_presentables,
                           &removed_presentables);

        /* remove presentables in the reverse topological order */
        removed_presentables = g_list_sort (removed_presentables, (GCompareFunc) gdu_presentable_compare);
        removed_presentables = g_list_reverse (removed_presentables);
        for (l = removed_presentables; l != NULL; l = l->next) {
                GduPresentable *p = GDU_PRESENTABLE (l->data);

                g_debug ("Removed presentable %s %p", gdu_presentable_get_id (p), p);

                pool->priv->presentables = g_list_remove (pool->priv->presentables, p);
                g_signal_emit (pool, signals[PRESENTABLE_REMOVED], 0, p);
                g_signal_emit_by_name (p, "removed");
                g_object_unref (p);
        }

        /* add presentables in the right topological order */
        added_presentables = g_list_sort (added_presentables, (GCompareFunc) gdu_presentable_compare);
        for (l = added_presentables; l != NULL; l = l->next) {
                GduPresentable *p = GDU_PRESENTABLE (l->data);

                /* rewrite all enclosing_presentable references for presentables we are going to add
                 * such that they really refer to presentables _previously_ added
                 */
                if (GDU_IS_HUB (p))
                        _gdu_hub_rewrite_enclosing_presentable (GDU_HUB (p));
                else if (GDU_IS_DRIVE (p))
                        _gdu_drive_rewrite_enclosing_presentable (GDU_DRIVE (p));
                else if (GDU_IS_LINUX_MD_DRIVE (p))
                        _gdu_linux_md_drive_rewrite_enclosing_presentable (GDU_LINUX_MD_DRIVE (p));
                else if (GDU_IS_VOLUME (p))
                        _gdu_volume_rewrite_enclosing_presentable (GDU_VOLUME (p));
                else if (GDU_IS_VOLUME_HOLE (p))
                        _gdu_volume_hole_rewrite_enclosing_presentable (GDU_VOLUME_HOLE (p));
                else if (GDU_IS_LINUX_LVM2_VOLUME_GROUP (p))
                        _gdu_linux_lvm2_volume_group_rewrite_enclosing_presentable (GDU_LINUX_LVM2_VOLUME_GROUP (p));
                else if (GDU_IS_LINUX_LVM2_VOLUME (p))
                        _gdu_linux_lvm2_volume_rewrite_enclosing_presentable (GDU_LINUX_LVM2_VOLUME (p));
                else if (GDU_IS_LINUX_LVM2_VOLUME_HOLE (p))
                        _gdu_linux_lvm2_volume_hole_rewrite_enclosing_presentable (GDU_LINUX_LVM2_VOLUME_HOLE (p));

                g_debug ("Added presentable %s %p", gdu_presentable_get_id (p), p);

                pool->priv->presentables = g_list_prepend (pool->priv->presentables, g_object_ref (p));
                g_signal_emit (pool, signals[PRESENTABLE_ADDED], 0, p);
        }

        /* keep list sorted */
        pool->priv->presentables = g_list_sort (pool->priv->presentables, (GCompareFunc) gdu_presentable_compare);

        g_list_free (removed_presentables);
        g_list_free (added_presentables);

        g_list_foreach (new_presentables, (GFunc) g_object_unref, NULL);
        g_list_free (new_presentables);
        g_list_foreach (devices, (GFunc) g_object_unref, NULL);
        g_list_free (devices);
        g_list_foreach (adapters, (GFunc) g_object_unref, NULL);
        g_list_free (adapters);
        g_list_foreach (expanders, (GFunc) g_object_unref, NULL);
        g_list_free (expanders);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
device_changed_signal_handler (DBusGProxy *proxy, const char *object_path, gpointer user_data);

static void
device_added_signal_handler (DBusGProxy *proxy, const char *object_path, gpointer user_data)
{
        GduPool *pool;
        GduDevice *device;

        pool = GDU_POOL (user_data);

        device = gdu_pool_get_by_object_path (pool, object_path);
        if (device != NULL) {
                g_object_unref (device);
                g_warning ("Treating add for previously added device %s as change", object_path);
                device_changed_signal_handler (proxy, object_path, user_data);
                goto out;
        }

        device = _gdu_device_new_from_object_path (pool, object_path);
        if (device == NULL)
                goto out;

        g_hash_table_insert (pool->priv->object_path_to_device,
                             (gpointer) gdu_device_get_object_path (device),
                             device);
        g_signal_emit (pool, signals[DEVICE_ADDED], 0, device);
        //g_debug ("Added device %s", object_path);

        recompute_presentables (pool);

 out:
        ;
}

static void
device_removed_signal_handler (DBusGProxy *proxy, const char *object_path, gpointer user_data)
{
        GduPool *pool;
        GduDevice *device;

        pool = GDU_POOL (user_data);

        device = gdu_pool_get_by_object_path (pool, object_path);
        if (device == NULL) {
                /* This is not fatal - the device may have been removed when GetAll() failed
                 * when getting properties
                 */
                g_debug ("No device to remove for remove %s", object_path);
                goto out;
        }

        g_hash_table_remove (pool->priv->object_path_to_device,
                             gdu_device_get_object_path (device));
        g_signal_emit (pool, signals[DEVICE_REMOVED], 0, device);
        g_signal_emit_by_name (device, "removed");
        g_object_unref (device);
        g_debug ("Removed device %s", object_path);

        recompute_presentables (pool);

 out:
        ;
}

static void
device_changed_signal_handler (DBusGProxy *proxy, const char *object_path, gpointer user_data)
{
        GduPool *pool;
        GduDevice *device;

        pool = GDU_POOL (user_data);

        device = gdu_pool_get_by_object_path (pool, object_path);
        if (device == NULL) {
                g_warning ("Ignoring change event on non-existant device %s", object_path);
                goto out;
        }

        if (_gdu_device_changed (device)) {
                g_signal_emit (pool, signals[DEVICE_CHANGED], 0, device);
                g_signal_emit_by_name (device, "changed");
        }
        g_object_unref (device);

        recompute_presentables (pool);

 out:
        ;
}

static void
device_job_changed_signal_handler (DBusGProxy *proxy,
                                   const char *object_path,
                                   gboolean    job_in_progress,
                                   const char *job_id,
                                   guint32     job_initiated_by_uid,
                                   gboolean    job_is_cancellable,
                                   double      job_percentage,
                                   gpointer user_data)
{
        GduPool *pool = GDU_POOL (user_data);
        GduDevice *device;

        if ((device = gdu_pool_get_by_object_path (pool, object_path)) != NULL) {
                _gdu_device_job_changed (device,
                                         job_in_progress,
                                         job_id,
                                         job_initiated_by_uid,
                                         job_is_cancellable,
                                         job_percentage);
                g_signal_emit_by_name (pool, "device-job-changed", device);
                g_object_unref (device);
        } else {
                g_warning ("Unknown device %s on job-change", object_path);
        }
}

/* ---------------------------------------------------------------------------------------------------- */

static void
adapter_changed_signal_handler (DBusGProxy *proxy, const char *object_path, gpointer user_data);

static void
adapter_added_signal_handler (DBusGProxy *proxy, const char *object_path, gpointer user_data)
{
        GduPool *pool;
        GduAdapter *adapter;

        pool = GDU_POOL (user_data);

        adapter = gdu_pool_get_adapter_by_object_path (pool, object_path);
        if (adapter != NULL) {
                g_object_unref (adapter);
                g_warning ("Treating add for previously added adapter %s as change", object_path);
                adapter_changed_signal_handler (proxy, object_path, user_data);
                goto out;
        }

        adapter = _gdu_adapter_new_from_object_path (pool, object_path);
        if (adapter == NULL)
                goto out;

        g_hash_table_insert (pool->priv->object_path_to_adapter,
                             (gpointer) gdu_adapter_get_object_path (adapter),
                             adapter);
        g_signal_emit (pool, signals[ADAPTER_ADDED], 0, adapter);
        //g_debug ("Added adapter %s", object_path);

        recompute_presentables (pool);

 out:
        ;
}

static void
adapter_removed_signal_handler (DBusGProxy *proxy, const char *object_path, gpointer user_data)
{
        GduPool *pool;
        GduAdapter *adapter;

        pool = GDU_POOL (user_data);

        adapter = gdu_pool_get_adapter_by_object_path (pool, object_path);
        if (adapter == NULL) {
                g_warning ("No adapter to remove for remove %s", object_path);
                goto out;
        }

        g_hash_table_remove (pool->priv->object_path_to_adapter,
                             gdu_adapter_get_object_path (adapter));
        g_signal_emit (pool, signals[ADAPTER_REMOVED], 0, adapter);
        g_signal_emit_by_name (adapter, "removed");
        g_object_unref (adapter);
        g_debug ("Removed adapter %s", object_path);

        recompute_presentables (pool);

 out:
        ;
}

static void
adapter_changed_signal_handler (DBusGProxy *proxy, const char *object_path, gpointer user_data)
{
        GduPool *pool;
        GduAdapter *adapter;

        pool = GDU_POOL (user_data);

        adapter = gdu_pool_get_adapter_by_object_path (pool, object_path);
        if (adapter == NULL) {
                g_warning ("Ignoring change event on non-existant adapter %s", object_path);
                goto out;
        }

        if (_gdu_adapter_changed (adapter)) {
                g_signal_emit (pool, signals[ADAPTER_CHANGED], 0, adapter);
                g_signal_emit_by_name (adapter, "changed");
        }
        g_object_unref (adapter);

        recompute_presentables (pool);

 out:
        ;
}

/* ---------------------------------------------------------------------------------------------------- */

static void
expander_changed_signal_handler (DBusGProxy *proxy, const char *object_path, gpointer user_data);

static void
expander_added_signal_handler (DBusGProxy *proxy, const char *object_path, gpointer user_data)
{
        GduPool *pool;
        GduExpander *expander;

        pool = GDU_POOL (user_data);

        expander = gdu_pool_get_expander_by_object_path (pool, object_path);
        if (expander != NULL) {
                g_object_unref (expander);
                g_warning ("Treating add for previously added expander %s as change", object_path);
                expander_changed_signal_handler (proxy, object_path, user_data);
                goto out;
        }

        expander = _gdu_expander_new_from_object_path (pool, object_path);
        if (expander == NULL)
                goto out;

        g_hash_table_insert (pool->priv->object_path_to_expander,
                             (gpointer) gdu_expander_get_object_path (expander),
                             expander);
        g_signal_emit (pool, signals[EXPANDER_ADDED], 0, expander);
        //g_debug ("Added expander %s", object_path);

        recompute_presentables (pool);

 out:
        ;
}

static void
expander_removed_signal_handler (DBusGProxy *proxy, const char *object_path, gpointer user_data)
{
        GduPool *pool;
        GduExpander *expander;

        pool = GDU_POOL (user_data);

        expander = gdu_pool_get_expander_by_object_path (pool, object_path);
        if (expander == NULL) {
                g_warning ("No expander to remove for remove %s", object_path);
                goto out;
        }

        g_hash_table_remove (pool->priv->object_path_to_expander,
                             gdu_expander_get_object_path (expander));
        g_signal_emit (pool, signals[EXPANDER_REMOVED], 0, expander);
        g_signal_emit_by_name (expander, "removed");
        g_object_unref (expander);
        g_debug ("Removed expander %s", object_path);

        recompute_presentables (pool);

 out:
        ;
}

static void
expander_changed_signal_handler (DBusGProxy *proxy, const char *object_path, gpointer user_data)
{
        GduPool *pool;
        GduExpander *expander;

        pool = GDU_POOL (user_data);

        expander = gdu_pool_get_expander_by_object_path (pool, object_path);
        if (expander == NULL) {
                g_warning ("Ignoring change event on non-existant expander %s", object_path);
                goto out;
        }

        if (_gdu_expander_changed (expander)) {
                g_signal_emit (pool, signals[EXPANDER_CHANGED], 0, expander);
                g_signal_emit_by_name (expander, "changed");
        }
        g_object_unref (expander);

        recompute_presentables (pool);

 out:
        ;
}

/* ---------------------------------------------------------------------------------------------------- */

static void
port_changed_signal_handler (DBusGProxy *proxy, const char *object_path, gpointer user_data);

static void
port_added_signal_handler (DBusGProxy *proxy, const char *object_path, gpointer user_data)
{
        GduPool *pool;
        GduPort *port;

        pool = GDU_POOL (user_data);

        port = gdu_pool_get_port_by_object_path (pool, object_path);
        if (port != NULL) {
                g_object_unref (port);
                g_warning ("Treating add for previously added port %s as change", object_path);
                port_changed_signal_handler (proxy, object_path, user_data);
                goto out;
        }

        port = _gdu_port_new_from_object_path (pool, object_path);
        if (port == NULL)
                goto out;

        g_hash_table_insert (pool->priv->object_path_to_port,
                             (gpointer) gdu_port_get_object_path (port),
                             port);
        g_signal_emit (pool, signals[PORT_ADDED], 0, port);
        //g_debug ("Added port %s", object_path);

        recompute_presentables (pool);

 out:
        ;
}

static void
port_removed_signal_handler (DBusGProxy *proxy, const char *object_path, gpointer user_data)
{
        GduPool *pool;
        GduPort *port;

        pool = GDU_POOL (user_data);

        port = gdu_pool_get_port_by_object_path (pool, object_path);
        if (port == NULL) {
                g_warning ("No port to remove for remove %s", object_path);
                goto out;
        }

        g_hash_table_remove (pool->priv->object_path_to_port,
                             gdu_port_get_object_path (port));
        g_signal_emit (pool, signals[PORT_REMOVED], 0, port);
        g_signal_emit_by_name (port, "removed");
        g_object_unref (port);
        g_debug ("Removed port %s", object_path);

        recompute_presentables (pool);

 out:
        ;
}

static void
port_changed_signal_handler (DBusGProxy *proxy, const char *object_path, gpointer user_data)
{
        GduPool *pool;
        GduPort *port;

        pool = GDU_POOL (user_data);

        port = gdu_pool_get_port_by_object_path (pool, object_path);
        if (port == NULL) {
                g_warning ("Ignoring change event on non-existant port %s", object_path);
                goto out;
        }

        if (_gdu_port_changed (port)) {
                g_signal_emit (pool, signals[PORT_CHANGED], 0, port);
                g_signal_emit_by_name (port, "changed");
        }
        g_object_unref (port);

        recompute_presentables (pool);

 out:
        ;
}

/* ---------------------------------------------------------------------------------------------------- */

static gboolean
get_properties (GduPool *pool)
{
        gboolean ret;
        GError *error;
        GHashTable *hash_table;
        DBusGProxy *prop_proxy;
        GValue *value;
        GPtrArray *known_filesystems_array;
        int n;

        ret = FALSE;

	prop_proxy = dbus_g_proxy_new_for_name (pool->priv->bus,
                                                "org.freedesktop.UDisks",
                                                "/org/freedesktop/UDisks",
                                                "org.freedesktop.DBus.Properties");
        error = NULL;
        if (!dbus_g_proxy_call (prop_proxy,
                                "GetAll",
                                &error,
                                G_TYPE_STRING,
                                "org.freedesktop.UDisks",
                                G_TYPE_INVALID,
                                dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_VALUE),
                                &hash_table,
                                G_TYPE_INVALID)) {
                g_debug ("Error calling GetAll() retrieving properties for /org/freedesktop/UDisks: %s",
                         error->message);
                g_error_free (error);
                goto out;
        }

        value = g_hash_table_lookup (hash_table, "DaemonVersion");
        if (value == NULL) {
                g_warning ("No property 'DaemonVersion'");
                goto out;
        }
        pool->priv->daemon_version = g_strdup (g_value_get_string (value));

        value = g_hash_table_lookup (hash_table, "SupportsLuksDevices");
        if (value == NULL) {
                g_warning ("No property 'SupportsLuksDevices'");
                goto out;
        }
        pool->priv->supports_luks_devices = g_value_get_boolean (value);

        value = g_hash_table_lookup (hash_table, "KnownFilesystems");
        if (value == NULL) {
                g_warning ("No property 'KnownFilesystems'");
                goto out;
        }
        known_filesystems_array = g_value_get_boxed (value);
        pool->priv->known_filesystems = NULL;
        for (n = 0; n < (int) known_filesystems_array->len; n++) {
                pool->priv->known_filesystems = g_list_prepend (
                        pool->priv->known_filesystems,
                        _gdu_known_filesystem_new (known_filesystems_array->pdata[n]));
        }
        pool->priv->known_filesystems = g_list_reverse (pool->priv->known_filesystems);

        g_hash_table_unref (hash_table);

        ret = TRUE;
out:
        g_object_unref (prop_proxy);
        return ret;
}


/**
 * gdu_pool_new:
 *
 * Create a new #GduPool object.
 *
 * Returns: A #GduPool object. Caller must free this object using g_object_unref().
 */
GduPool *
gdu_pool_new (void)
{
        GduPool *pool;
        GError *error;

        error = NULL;
        pool = gdu_pool_new_for_address (NULL, NULL, &error);
        if (pool == NULL) {
                g_printerr ("======================================================================\n"
                            "Error constructing GduPool: %s\n"
                            "\n"
                            "This error suggests there's a problem with your udisks or D-Bus installation.\n"
                            "======================================================================\n",
                            error->message);
        }

        return pool;
}

DBusGConnection *
_gdu_pool_get_connection (GduPool *pool)
{
        g_assert (pool != NULL);
        return pool->priv->bus;
}

static void
on_ssh_process_terminated (GPid     pid,
                           gint     status,
                           gpointer user_data)
{
        GduPool *pool = GDU_POOL (user_data);

        g_print ("wohoo, ssh process has been terminated\n");

        /* need to take a temp ref since receivers of the ::disconnected signal
         * may unref the pool
         */
        g_object_ref (pool);

        _gdu_pool_disconnect (pool);

        g_spawn_close_pid (pid);

        g_source_remove (pool->priv->ssh_child_watch_id);
        pool->priv->ssh_child_watch_id = 0;
        pool->priv->ssh_pid = 0;

        g_object_unref (pool);
}

GduPool *
gdu_pool_new_for_address (const gchar     *ssh_user_name,
                          const gchar     *ssh_address,
                          GError         **error)
{
        int n;
        GPtrArray *devices;
        GPtrArray *adapters;
        GPtrArray *expanders;
        GPtrArray *ports;
        GduPool *pool;
        GError *local_error;

        local_error = NULL;

        pool = GDU_POOL (g_object_new (GDU_TYPE_POOL, NULL));

        if (ssh_address == NULL) {
                pool->priv->bus = dbus_g_bus_get (DBUS_BUS_SYSTEM, error);
                if (pool->priv->bus == NULL) {
                        goto error;
                }
        } else {
                pool->priv->bus = _gdu_ssh_bridge_connect (ssh_user_name,
                                                           ssh_address,
                                                           &(pool->priv->ssh_pid),
                                                           error);
                if (pool->priv->bus == NULL) {
                        goto error;
                }
                pool->priv->ssh_user_name = g_strdup (ssh_user_name);
                pool->priv->ssh_address  = g_strdup (ssh_address);

                /* Watch the ssh process */
                //g_print ("pid is %d\n", pool->priv->ssh_pid);
                pool->priv->ssh_child_watch_id = g_child_watch_add (pool->priv->ssh_pid,
                                                                    on_ssh_process_terminated,
                                                                    pool);
        }

        pool->priv->machine = GDU_PRESENTABLE (_gdu_machine_new (pool));

        dbus_g_object_register_marshaller (
                gdu_marshal_VOID__STRING_BOOLEAN_STRING_UINT_BOOLEAN_DOUBLE,
                G_TYPE_NONE,
                DBUS_TYPE_G_OBJECT_PATH,
                G_TYPE_BOOLEAN,
                G_TYPE_STRING,
                G_TYPE_UINT,
                G_TYPE_BOOLEAN,
                G_TYPE_DOUBLE,
                G_TYPE_INVALID);

	pool->priv->proxy = dbus_g_proxy_new_for_name (pool->priv->bus,
                                                       "org.freedesktop.UDisks",
                                                       "/org/freedesktop/UDisks",
                                                       "org.freedesktop.UDisks");
        dbus_g_proxy_add_signal (pool->priv->proxy, "DeviceAdded", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
        dbus_g_proxy_add_signal (pool->priv->proxy, "DeviceRemoved", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
        dbus_g_proxy_add_signal (pool->priv->proxy, "DeviceChanged", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
        dbus_g_proxy_add_signal (pool->priv->proxy,
                                 "DeviceJobChanged",
                                 DBUS_TYPE_G_OBJECT_PATH,
                                 G_TYPE_BOOLEAN,
                                 G_TYPE_STRING,
                                 G_TYPE_UINT,
                                 G_TYPE_BOOLEAN,
                                 G_TYPE_DOUBLE,
                                 G_TYPE_INVALID);

        /* get the properties on the daemon object */
        if (!get_properties (pool)) {
                g_warning ("Couldn't get daemon properties");
                goto error;
        }

        dbus_g_proxy_connect_signal (pool->priv->proxy, "DeviceAdded",
                                     G_CALLBACK (device_added_signal_handler), pool, NULL);
        dbus_g_proxy_connect_signal (pool->priv->proxy, "DeviceRemoved",
                                     G_CALLBACK (device_removed_signal_handler), pool, NULL);
        dbus_g_proxy_connect_signal (pool->priv->proxy, "DeviceChanged",
                                     G_CALLBACK (device_changed_signal_handler), pool, NULL);
        dbus_g_proxy_connect_signal (pool->priv->proxy, "DeviceJobChanged",
                                     G_CALLBACK (device_job_changed_signal_handler), pool, NULL);

        dbus_g_proxy_add_signal (pool->priv->proxy, "AdapterAdded", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
        dbus_g_proxy_add_signal (pool->priv->proxy, "AdapterRemoved", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
        dbus_g_proxy_add_signal (pool->priv->proxy, "AdapterChanged", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
        dbus_g_proxy_connect_signal (pool->priv->proxy, "AdapterAdded",
                                     G_CALLBACK (adapter_added_signal_handler), pool, NULL);
        dbus_g_proxy_connect_signal (pool->priv->proxy, "AdapterRemoved",
                                     G_CALLBACK (adapter_removed_signal_handler), pool, NULL);
        dbus_g_proxy_connect_signal (pool->priv->proxy, "AdapterChanged",
                                     G_CALLBACK (adapter_changed_signal_handler), pool, NULL);

        dbus_g_proxy_add_signal (pool->priv->proxy, "ExpanderAdded", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
        dbus_g_proxy_add_signal (pool->priv->proxy, "ExpanderRemoved", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
        dbus_g_proxy_add_signal (pool->priv->proxy, "ExpanderChanged", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
        dbus_g_proxy_connect_signal (pool->priv->proxy, "ExpanderAdded",
                                     G_CALLBACK (expander_added_signal_handler), pool, NULL);
        dbus_g_proxy_connect_signal (pool->priv->proxy, "ExpanderRemoved",
                                     G_CALLBACK (expander_removed_signal_handler), pool, NULL);
        dbus_g_proxy_connect_signal (pool->priv->proxy, "ExpanderChanged",
                                     G_CALLBACK (expander_changed_signal_handler), pool, NULL);

        dbus_g_proxy_add_signal (pool->priv->proxy, "PortAdded", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
        dbus_g_proxy_add_signal (pool->priv->proxy, "PortRemoved", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
        dbus_g_proxy_add_signal (pool->priv->proxy, "PortChanged", DBUS_TYPE_G_OBJECT_PATH, G_TYPE_INVALID);
        dbus_g_proxy_connect_signal (pool->priv->proxy, "PortAdded",
                                     G_CALLBACK (port_added_signal_handler), pool, NULL);
        dbus_g_proxy_connect_signal (pool->priv->proxy, "PortRemoved",
                                     G_CALLBACK (port_removed_signal_handler), pool, NULL);
        dbus_g_proxy_connect_signal (pool->priv->proxy, "PortChanged",
                                     G_CALLBACK (port_changed_signal_handler), pool, NULL);

        /* prime the list of devices */
        if (!org_freedesktop_UDisks_enumerate_devices (pool->priv->proxy, &devices, &local_error)) {
                g_set_error (error, GDU_ERROR, GDU_ERROR_FAILED,
                             _("Error enumerating devices: %s"),
                             local_error->message);
                g_error_free (local_error);
                goto error;
        }

        /* to check that topological sorting works, enumerate backwards by commenting out the for statement below */
        //for (n = devices->len - 1; n >= 0; n--) {
        for (n = 0; n < (int) devices->len; n++) {
                const char *object_path;
                GduDevice *device;

                object_path = devices->pdata[n];

                device = _gdu_device_new_from_object_path (pool, object_path);

                g_hash_table_insert (pool->priv->object_path_to_device,
                                     (gpointer) gdu_device_get_object_path (device),
                                     device);
        }
        g_ptr_array_foreach (devices, (GFunc) g_free, NULL);
        g_ptr_array_free (devices, TRUE);

        /* prime the list of adapters */
        if (!org_freedesktop_UDisks_enumerate_adapters (pool->priv->proxy, &adapters, &local_error)) {
                g_set_error (error, GDU_ERROR, GDU_ERROR_FAILED,
                             _("Error enumerating adapters: %s"),
                             local_error->message);
                g_error_free (local_error);
                goto error;
        }
        for (n = 0; n < (int) adapters->len; n++) {
                const char *object_path;
                GduAdapter *adapter;

                object_path = adapters->pdata[n];

                adapter = _gdu_adapter_new_from_object_path (pool, object_path);

                g_hash_table_insert (pool->priv->object_path_to_adapter,
                                     (gpointer) gdu_adapter_get_object_path (adapter),
                                     adapter);
        }
        g_ptr_array_foreach (adapters, (GFunc) g_free, NULL);
        g_ptr_array_free (adapters, TRUE);

        /* prime the list of expanders */
        if (!org_freedesktop_UDisks_enumerate_expanders (pool->priv->proxy, &expanders, &local_error)) {
                g_set_error (error, GDU_ERROR, GDU_ERROR_FAILED,
                             _("Error enumerating expanders: %s"),
                             local_error->message);
                g_error_free (local_error);
                goto error;
        }
        for (n = 0; n < (int) expanders->len; n++) {
                const char *object_path;
                GduExpander *expander;

                object_path = expanders->pdata[n];

                expander = _gdu_expander_new_from_object_path (pool, object_path);

                g_hash_table_insert (pool->priv->object_path_to_expander,
                                     (gpointer) gdu_expander_get_object_path (expander),
                                     expander);
        }
        g_ptr_array_foreach (expanders, (GFunc) g_free, NULL);
        g_ptr_array_free (expanders, TRUE);

        /* prime the list of ports */
        if (!org_freedesktop_UDisks_enumerate_ports (pool->priv->proxy, &ports, &local_error)) {
                g_set_error (error, GDU_ERROR, GDU_ERROR_FAILED,
                             _("Error enumerating ports: %s"),
                             local_error->message);
                g_error_free (local_error);
                goto error;
        }
        for (n = 0; n < (int) ports->len; n++) {
                const char *object_path;
                GduPort *port;

                object_path = ports->pdata[n];

                port = _gdu_port_new_from_object_path (pool, object_path);

                g_hash_table_insert (pool->priv->object_path_to_port,
                                     (gpointer) gdu_port_get_object_path (port),
                                     port);
        }
        g_ptr_array_foreach (ports, (GFunc) g_free, NULL);
        g_ptr_array_free (ports, TRUE);

        /* and finally compute all presentables */
        recompute_presentables (pool);

        return pool;

error:
        g_object_unref (pool);
        if (error != NULL && *error == NULL) {
                g_set_error (error,
                             GDU_ERROR,
                             GDU_ERROR_FAILED,
                             "(unspecified error)");
        }
        return NULL;
}

/**
 * gdu_pool_get_by_object_path:
 * @pool: the device pool
 * @object_path: the D-Bus object path
 *
 * Looks up #GduDevice object for @object_path.
 *
 * Returns: A #GduDevice object for @object_path, otherwise
 * #NULL. Caller must unref this object using g_object_unref().
 **/
GduDevice *
gdu_pool_get_by_object_path (GduPool *pool, const char *object_path)
{
        GduDevice *ret;

        g_assert (pool != NULL);

        ret = g_hash_table_lookup (pool->priv->object_path_to_device, object_path);
        if (ret != NULL) {
                g_object_ref (ret);
        }
        return ret;
}

/**
 * gdu_pool_get_adapter_by_object_path:
 * @pool: the pool
 * @object_path: the D-Bus object path
 *
 * Looks up #GduAdapter object for @object_path.
 *
 * Returns: A #GduAdapter object for @object_path, otherwise
 * #NULL. Caller must unref this object using g_object_unref().
 **/
GduAdapter *
gdu_pool_get_adapter_by_object_path (GduPool *pool, const char *object_path)
{
        GduAdapter *ret;

        g_assert (pool != NULL);

        ret = g_hash_table_lookup (pool->priv->object_path_to_adapter, object_path);
        if (ret != NULL) {
                g_object_ref (ret);
        }
        return ret;
}

/**
 * gdu_pool_get_expander_by_object_path:
 * @pool: the pool
 * @object_path: the D-Bus object path
 *
 * Looks up #GduExpander object for @object_path.
 *
 * Returns: A #GduExpander object for @object_path, otherwise
 * #NULL. Caller must unref this object using g_object_unref().
 **/
GduExpander *
gdu_pool_get_expander_by_object_path (GduPool *pool, const char *object_path)
{
        GduExpander *ret;

        g_assert (pool != NULL);

        ret = g_hash_table_lookup (pool->priv->object_path_to_expander, object_path);
        if (ret != NULL) {
                g_object_ref (ret);
        }
        return ret;
}

/**
 * gdu_pool_get_by_object_path:
 * @pool: the pool
 * @object_path: the D-Bus object path
 *
 * Looks up #GduPort object for @object_path.
 *
 * Returns: A #GduPort object for @object_path, otherwise
 * #NULL. Caller must unref this object using g_object_unref().
 **/
GduPort *
gdu_pool_get_port_by_object_path (GduPool *pool, const char *object_path)
{
        GduPort *ret;

        g_assert (pool != NULL);

        ret = g_hash_table_lookup (pool->priv->object_path_to_port, object_path);
        if (ret != NULL) {
                g_object_ref (ret);
        }
        return ret;
}

/**
 * gdu_pool_get_by_device_file:
 * @pool: the device pool
 * @device_file: the UNIX block special device file, e.g. /dev/sda1.
 *
 * Looks up #GduDevice object for @device_file.
 *
 * Returns: A #GduDevice object for @object_path, otherwise
 * #NULL. Caller must unref this object using g_object_unref().
 **/
GduDevice *
gdu_pool_get_by_device_file (GduPool *pool, const char *device_file)
{
        GHashTableIter iter;
        GduDevice *device;
        GduDevice *ret;

        g_assert (pool != NULL);

        ret = NULL;

        /* TODO: use lookaside hash table */

        g_hash_table_iter_init (&iter, pool->priv->object_path_to_device);
        while (g_hash_table_iter_next (&iter, NULL, (gpointer) &device)) {

                if (g_strcmp0 (gdu_device_get_device_file (device), device_file) == 0) {
                        ret = g_object_ref (device);
                        goto out;
                }
        }

 out:
        return ret;
}

static GduDevice *
find_extended_partition (GduPool *pool, const gchar *partition_table_object_path)
{
        GHashTableIter iter;
        GduDevice *device;
        GduDevice *ret;

        ret = NULL;

        g_hash_table_iter_init (&iter, pool->priv->object_path_to_device);
        while (g_hash_table_iter_next (&iter, NULL, (gpointer) &device)) {

                if (!gdu_device_is_partition (device))
                        continue;

                if (g_strcmp0 (gdu_device_partition_get_slave (device), partition_table_object_path) == 0) {
                        gint type;

                        type = strtol (gdu_device_partition_get_type (device), NULL, 0);
                        if (type == 0x05 || type == 0x0f || type == 0x85) {
                                ret = device;
                                goto out;
                        }
                }

        }

 out:
        return ret;
}

static void
device_recurse (GduPool *pool, GduDevice *device, GList **ret, guint depth)
{
        gboolean insert_after;

        /* cycle "detection" */
        g_assert (depth < 100);

        insert_after = FALSE;

        if (gdu_device_is_partition (device)) {
                const gchar *partition_table_object_path;
                GduDevice *partition_table;

                partition_table_object_path = gdu_device_partition_get_slave (device);
                partition_table = gdu_pool_get_by_object_path (pool, partition_table_object_path);

                /* we want the partition table to come before any partition */
                if (partition_table != NULL)
                        device_recurse (pool, partition_table, ret, depth + 1);

                if (g_strcmp0 (gdu_device_partition_get_scheme (device), "mbr") == 0 &&
                    gdu_device_partition_get_number (device) >= 5) {
                        GduDevice *extended_partition;

                        /* logical MSDOS partition, ensure that the extended partition comes before us */
                        extended_partition = find_extended_partition (pool, partition_table_object_path);
                        if (extended_partition != NULL) {
                                device_recurse (pool, extended_partition, ret, depth + 1);
                        }
                }

                if (partition_table != NULL)
                        g_object_unref (partition_table);
        }

        if (gdu_device_is_luks_cleartext (device)) {
                const gchar *luks_device_object_path;
                GduDevice *luks_device;

                luks_device_object_path = gdu_device_luks_cleartext_get_slave (device);
                luks_device = gdu_pool_get_by_object_path (pool, luks_device_object_path);

                /* the LUKS device must be before the cleartext device */
                if (luks_device != NULL) {
                        device_recurse (pool, luks_device, ret, depth + 1);
                        g_object_unref (luks_device);
                }
        }

        if (gdu_device_is_linux_md (device)) {
                gchar **slaves;
                guint n;

                /* Linux-MD slaves must come *after* the array itself */
                insert_after = TRUE;

                slaves = gdu_device_linux_md_get_slaves (device);
                for (n = 0; slaves != NULL && slaves[n] != NULL; n++) {
                        GduDevice *slave;

                        slave = gdu_pool_get_by_object_path (pool, slaves[n]);
                        if (slave != NULL) {
                                device_recurse (pool, slave, ret, depth + 1);
                                g_object_unref (slave);
                        }
                }

        }

        if (!g_list_find (*ret, device)) {
                if (insert_after)
                        *ret = g_list_append (*ret, device);
                else
                        *ret = g_list_prepend (*ret, device);
        }
}

/**
 * gdu_pool_get_devices:
 * @pool: A #GduPool.
 *
 * Get a list of all devices. The returned list is topologically sorted, e.g.
 * for any device A with a dependency on a device B, A is guaranteed to appear
 * after B.
 *
 * Returns: A #GList of #GduDevice objects. Caller must free this
 * (unref all objects, then use g_list_free()).
 **/
GList *
gdu_pool_get_devices (GduPool *pool)
{
        GList *list;
        GList *ret;
        GList *l;

        g_assert (pool != NULL);

        ret = NULL;

        list = g_hash_table_get_values (pool->priv->object_path_to_device);

        for (l = list; l != NULL; l = l->next) {
                GduDevice *device = GDU_DEVICE (l->data);

                device_recurse (pool, device, &ret, 0);
        }

        g_assert (g_list_length (ret) == g_list_length (list));

        g_list_free (list);

        g_list_foreach (ret, (GFunc) g_object_ref, NULL);

        ret = g_list_reverse (ret);

        return ret;
}

/**
 * gdu_pool_get_adapters:
 * @pool: A #GduPool.
 *
 * Get a list of all adapters.
 *
 * Returns: A #GList of #GduAdapter objects. Caller must free this
 * (unref all objects, then use g_list_free()).
 **/
GList *
gdu_pool_get_adapters (GduPool *pool)
{
        GList *ret;

        g_assert (pool != NULL);

        ret = NULL;

        ret = g_hash_table_get_values (pool->priv->object_path_to_adapter);
        g_list_foreach (ret, (GFunc) g_object_ref, NULL);
        return ret;
}

/**
 * gdu_pool_get_expanders:
 * @pool: A #GduPool.
 *
 * Get a list of all expanders.
 *
 * Returns: A #GList of #GduExpander objects. Caller must free this
 * (unref all objects, then use g_list_free()).
 **/
GList *
gdu_pool_get_expanders (GduPool *pool)
{
        GList *ret;

        g_assert (pool != NULL);

        ret = NULL;

        ret = g_hash_table_get_values (pool->priv->object_path_to_expander);
        g_list_foreach (ret, (GFunc) g_object_ref, NULL);
        return ret;
}

/**
 * gdu_pool_get_ports:
 * @pool: A #GduPool.
 *
 * Get a list of all ports.
 *
 * Returns: A #GList of #GduPort objects. Caller must free this
 * (unref all objects, then use g_list_free()).
 **/
GList *
gdu_pool_get_ports (GduPool *pool)
{
        GList *ret;

        g_assert (pool != NULL);

        ret = NULL;

        ret = g_hash_table_get_values (pool->priv->object_path_to_port);
        g_list_foreach (ret, (GFunc) g_object_ref, NULL);
        return ret;
}

/**
 * gdu_pool_get_presentables:
 * @pool: A #GduPool
 *
 * Get a list of all presentables.
 *
 * Returns: A #GList of objects implementing the #GduPresentable
 * interface. Caller must free this (unref all objects, then use
 * g_list_free()).
 **/
GList *
gdu_pool_get_presentables (GduPool *pool)
{
        GList *ret;
        g_assert (pool != NULL);

        ret = g_list_copy (pool->priv->presentables);
        g_list_foreach (ret, (GFunc) g_object_ref, NULL);
        return ret;
}

GList *
gdu_pool_get_enclosed_presentables (GduPool *pool, GduPresentable *presentable)
{
        GList *l;
        GList *ret;

        g_assert (pool != NULL);

        ret = NULL;
        for (l = pool->priv->presentables; l != NULL; l = l->next) {
                GduPresentable *p = l->data;
                GduPresentable *e;

                e = gdu_presentable_get_enclosing_presentable (p);
                if (e != NULL) {
                        if (gdu_presentable_equals (e, presentable))
                                ret = g_list_prepend (ret, g_object_ref (p));

                        g_object_unref (e);
                }
        }

        return ret;
}

/**
 * gdu_pool_get_volume_by_device:
 * @pool: A #GduPool.
 * @device: A #GduDevice.
 *
 * Given @device, find the #GduVolume object for it.
 *
 * Returns: A #GduVolume object or #NULL if no @device isn't a
 * volume. Caller must free this object with g_object_unref().
 **/
GduPresentable *
gdu_pool_get_volume_by_device (GduPool *pool, GduDevice *device)
{
        GduPresentable *ret;
        GList *l;

        g_assert (pool != NULL);

        /* TODO: use lookaside hash table */

        ret = NULL;

        for (l = pool->priv->presentables; l != NULL; l = l->next) {
                GduPresentable *p = GDU_PRESENTABLE (l->data);
                GduDevice *d;
                const gchar *object_path;

                if (!GDU_IS_VOLUME (p))
                        continue;

                d = gdu_presentable_get_device (p);
                if (d == NULL)
                        continue;

                object_path = gdu_device_get_object_path (d);
                g_object_unref (d);

                if (g_strcmp0 (object_path, gdu_device_get_object_path (device)) == 0) {
                        ret = g_object_ref (p);
                        goto out;
                }
        }

 out:
        return ret;
}

/**
 * gdu_pool_get_drive_by_device:
 * @pool: A #GduPool.
 * @device: A #GduDevice.
 *
 * Given @device, find the #GduDrive object for it.
 *
 * Returns: A #GduDrive object or #NULL if no @device isn't a
 * drive. Caller must free this object with g_object_unref().
 **/
GduPresentable *
gdu_pool_get_drive_by_device (GduPool *pool, GduDevice *device)
{
        GduPresentable *ret;
        GList *l;

        g_assert (pool != NULL);

        /* TODO: use lookaside hash table */

        ret = NULL;

        for (l = pool->priv->presentables; l != NULL; l = l->next) {
                GduPresentable *p = GDU_PRESENTABLE (l->data);
                GduDevice *d;
                const gchar *object_path;

                if (!GDU_IS_DRIVE (p))
                        continue;

                d = gdu_presentable_get_device (p);
                if (d == NULL)
                        continue;

                object_path = gdu_device_get_object_path (d);
                g_object_unref (d);

                if (g_strcmp0 (object_path, gdu_device_get_object_path (device)) == 0) {
                        ret = g_object_ref (p);
                        goto out;
                }
        }

 out:
        return ret;
}

GduLinuxMdDrive *
gdu_pool_get_linux_md_drive_by_uuid (GduPool *pool, const gchar *uuid)
{
        GduLinuxMdDrive *ret;
        GList *l;

        g_assert (pool != NULL);

        /* TODO: use lookaside hash table */

        ret = NULL;

        for (l = pool->priv->presentables; l != NULL; l = l->next) {
                GduPresentable *p = GDU_PRESENTABLE (l->data);

                if (! GDU_IS_LINUX_MD_DRIVE (p))
                        continue;

                if (g_strcmp0 (uuid, gdu_linux_md_drive_get_uuid (GDU_LINUX_MD_DRIVE (p))) == 0) {
                        ret = g_object_ref (p);
                        goto out;
                }
        }

 out:
        return ret;
}

GduPresentable *
gdu_pool_get_presentable_by_id (GduPool *pool, const gchar *id)
{
        GduPresentable *ret;
        GList *l;

        g_assert (pool != NULL);

        /* TODO: use lookaside hash table */

        ret = NULL;

        for (l = pool->priv->presentables; l != NULL; l = l->next) {
                GduPresentable *p = GDU_PRESENTABLE (l->data);

                if (g_strcmp0 (id, gdu_presentable_get_id (p)) == 0) {
                        ret = g_object_ref (p);
                        goto out;
                }
        }

 out:
        return ret;
}

gboolean
gdu_pool_has_presentable (GduPool *pool, GduPresentable *presentable)
{
        gboolean ret;

        g_assert (pool != NULL);

        ret = (g_list_find (pool->priv->presentables, presentable) != NULL);

        return ret;
}



GduPresentable *
gdu_pool_get_hub_by_object_path (GduPool *pool, const gchar *object_path)
{
        GduPresentable *ret;
        GList *l;

        g_assert (pool != NULL);

        /* TODO: use lookaside hash table */

        ret = NULL;
        for (l = pool->priv->presentables; l != NULL && ret == NULL; l = l->next) {
                GduPresentable *p = GDU_PRESENTABLE (l->data);
                GduAdapter *a;
                GduExpander *e;

                if (!GDU_IS_HUB (p))
                        continue;

                a = gdu_hub_get_adapter (GDU_HUB (p));
                e = gdu_hub_get_expander (GDU_HUB (p));

                if (a != NULL && g_strcmp0 (gdu_adapter_get_object_path (a), object_path) == 0) {
                        ret = g_object_ref (p);
                } else if (e != NULL && g_strcmp0 (gdu_expander_get_object_path (e), object_path) == 0) {
                        ret = g_object_ref (p);
                }

                if (a != NULL)
                        g_object_unref (a);
                if (e != NULL)
                        g_object_unref (e);
        }

        return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct {
        GduPool *pool;
        GduPoolLinuxMdStartCompletedFunc callback;
        gpointer user_data;
} LinuxMdStartData;

static void
op_linux_md_start_cb (DBusGProxy *proxy, char *assembled_array_object_path, GError *error, gpointer user_data)
{
        LinuxMdStartData *data = user_data;
        _gdu_error_fixup (error);
        if (data->callback != NULL)
                data->callback (data->pool, assembled_array_object_path, error, data->user_data);
        g_object_unref (data->pool);
        g_free (data);
}

/**
 * gdu_pool_op_linux_md_start:
 * @pool: A #GduPool.
 * @component_objpaths: A #GPtrArray of object paths.
 * @callback: Callback function.
 * @user_data: User data to pass to @callback.
 *
 * Starts a Linux md Software Array.
 **/
void
gdu_pool_op_linux_md_start (GduPool *pool,
                            GPtrArray *component_objpaths,
                            GduPoolLinuxMdStartCompletedFunc callback,
                            gpointer user_data)
{
        LinuxMdStartData *data;
        char *options[16];

        g_assert (pool != NULL);

        options[0] = NULL;

        data = g_new0 (LinuxMdStartData, 1);
        data->pool = g_object_ref (pool);
        data->callback = callback;
        data->user_data = user_data;

        org_freedesktop_UDisks_linux_md_start_async (pool->priv->proxy,
                                                     component_objpaths,
                                                     (const char **) options,
                                                     op_linux_md_start_cb,
                                                     data);
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct {
        GduPool *pool;
        GduPoolLinuxMdCreateCompletedFunc callback;
        gpointer user_data;
} LinuxMdCreateData;

static void
op_linux_md_create_cb (DBusGProxy *proxy, char *assembled_array_object_path, GError *error, gpointer user_data)
{
        LinuxMdCreateData *data = user_data;
        _gdu_error_fixup (error);
        if (data->callback != NULL)
                data->callback (data->pool, assembled_array_object_path, error, data->user_data);
        g_object_unref (data->pool);
        g_free (data);
}

/**
 * gdu_pool_op_linux_md_create:
 * @pool: A #GduPool.
 * @component_objpaths: A #GPtrArray of object paths.
 * @level: RAID level.
 * @name: Name of array.
 * @callback: Callback function.
 * @user_data: User data to pass to @callback.
 *
 * Creates a Linux md Software Array.
 **/
void
gdu_pool_op_linux_md_create (GduPool *pool,
                             GPtrArray *component_objpaths,
                             const gchar *level,
                             guint64      stripe_size,
                             const gchar *name,
                             GduPoolLinuxMdCreateCompletedFunc callback,
                             gpointer user_data)
{
        LinuxMdCreateData *data;
        char *options[16];

        g_assert (pool != NULL);

        options[0] = NULL;

        data = g_new0 (LinuxMdCreateData, 1);
        data->pool = g_object_ref (pool);
        data->callback = callback;
        data->user_data = user_data;

        org_freedesktop_UDisks_linux_md_create_async (pool->priv->proxy,
                                                      component_objpaths,
                                                      level,
                                                      stripe_size,
                                                      name,
                                                      (const char **) options,
                                                      op_linux_md_create_cb,
                                                      data);
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct {
        GduPool *pool;
        GduPoolLinuxLvm2VGStartCompletedFunc callback;
        gpointer user_data;
} LinuxLvm2VGStartData;

static void
op_linux_lvm2_vg_start_cb (DBusGProxy *proxy, GError *error, gpointer user_data)
{
        LinuxLvm2VGStartData *data = user_data;
        _gdu_error_fixup (error);
        if (data->callback != NULL)
                data->callback (data->pool, error, data->user_data);
        g_object_unref (data->pool);
        g_free (data);
}

void
gdu_pool_op_linux_lvm2_vg_start (GduPool *pool,
                                 const gchar *uuid,
                                 GduPoolLinuxLvm2VGStartCompletedFunc callback,
                                 gpointer user_data)
{
        LinuxLvm2VGStartData *data;
        char *options[16];

        g_assert (pool != NULL);

        options[0] = NULL;

        data = g_new0 (LinuxLvm2VGStartData, 1);
        data->pool = g_object_ref (pool);
        data->callback = callback;
        data->user_data = user_data;

        org_freedesktop_UDisks_linux_lvm2_vg_start_async (pool->priv->proxy,
                                                          uuid,
                                                          (const char **) options,
                                                          op_linux_lvm2_vg_start_cb,
                                                          data);
}


/* ---------------------------------------------------------------------------------------------------- */

typedef struct {
        GduPool *pool;
        GduPoolLinuxLvm2VGStopCompletedFunc callback;
        gpointer user_data;
} LinuxLvm2VGStopData;

static void
op_linux_lvm2_vg_stop_cb (DBusGProxy *proxy, GError *error, gpointer user_data)
{
        LinuxLvm2VGStopData *data = user_data;
        _gdu_error_fixup (error);
        if (data->callback != NULL)
                data->callback (data->pool, error, data->user_data);
        g_object_unref (data->pool);
        g_free (data);
}

void
gdu_pool_op_linux_lvm2_vg_stop (GduPool *pool,
                                const gchar *uuid,
                                GduPoolLinuxLvm2VGStopCompletedFunc callback,
                                gpointer user_data)
{
        LinuxLvm2VGStopData *data;
        char *options[16];

        g_assert (pool != NULL);

        options[0] = NULL;

        data = g_new0 (LinuxLvm2VGStopData, 1);
        data->pool = g_object_ref (pool);
        data->callback = callback;
        data->user_data = user_data;

        org_freedesktop_UDisks_linux_lvm2_vg_stop_async (pool->priv->proxy,
                                                          uuid,
                                                          (const char **) options,
                                                          op_linux_lvm2_vg_stop_cb,
                                                          data);
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct {
        GduPool *pool;
        GduPoolLinuxLvm2LVStartCompletedFunc callback;
        gpointer user_data;
} LinuxLvm2LVStartData;

static void
op_linux_lvm2_lv_start_cb (DBusGProxy *proxy, GError *error, gpointer user_data)
{
        LinuxLvm2LVStartData *data = user_data;
        _gdu_error_fixup (error);
        if (data->callback != NULL)
                data->callback (data->pool, error, data->user_data);
        g_object_unref (data->pool);
        g_free (data);
}

void
gdu_pool_op_linux_lvm2_lv_start (GduPool *pool,
                                 const gchar *group_uuid,
                                 const gchar *uuid,
                                 GduPoolLinuxLvm2LVStartCompletedFunc callback,
                                 gpointer user_data)
{
        LinuxLvm2LVStartData *data;
        char *options[16];

        g_assert (pool != NULL);

        options[0] = NULL;

        data = g_new0 (LinuxLvm2LVStartData, 1);
        data->pool = g_object_ref (pool);
        data->callback = callback;
        data->user_data = user_data;

        org_freedesktop_UDisks_linux_lvm2_lv_start_async (pool->priv->proxy,
                                                          group_uuid,
                                                          uuid,
                                                          (const char **) options,
                                                          op_linux_lvm2_lv_start_cb,
                                                          data);
}

/* ---------------------------------------------------------------------------------------------------- */


typedef struct {
        GduPool *pool;
        GduPoolLinuxLvm2VGSetNameCompletedFunc callback;
        gpointer user_data;
} LinuxLvm2VGSetNameData;

static void
op_linux_lvm2_vg_set_name_cb (DBusGProxy *proxy, GError *error, gpointer user_data)
{
        LinuxLvm2VGSetNameData *data = user_data;
        _gdu_error_fixup (error);
        if (data->callback != NULL)
                data->callback (data->pool, error, data->user_data);
        g_object_unref (data->pool);
        g_free (data);
}

void
gdu_pool_op_linux_lvm2_vg_set_name (GduPool *pool,
                                    const gchar *uuid,
                                    const gchar *new_name,
                                    GduPoolLinuxLvm2VGSetNameCompletedFunc callback,
                                    gpointer user_data)
{
        LinuxLvm2VGSetNameData *data;

        g_assert (pool != NULL);

        data = g_new0 (LinuxLvm2VGSetNameData, 1);
        data->pool = g_object_ref (pool);
        data->callback = callback;
        data->user_data = user_data;

        org_freedesktop_UDisks_linux_lvm2_vg_set_name_async (pool->priv->proxy,
                                                             uuid,
                                                             new_name,
                                                             op_linux_lvm2_vg_set_name_cb,
                                                             data);
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct {
        GduPool *pool;
        GduPoolLinuxLvm2LVSetNameCompletedFunc callback;
        gpointer user_data;
} LinuxLvm2LVSetNameData;

static void
op_linux_lvm2_lv_set_name_cb (DBusGProxy *proxy, GError *error, gpointer user_data)
{
        LinuxLvm2LVSetNameData *data = user_data;
        _gdu_error_fixup (error);
        if (data->callback != NULL)
                data->callback (data->pool, error, data->user_data);
        g_object_unref (data->pool);
        g_free (data);
}

void
gdu_pool_op_linux_lvm2_lv_set_name (GduPool *pool,
                                    const gchar *group_uuid,
                                    const gchar *uuid,
                                    const gchar *new_name,
                                    GduPoolLinuxLvm2LVSetNameCompletedFunc callback,
                                    gpointer user_data)
{
        LinuxLvm2LVSetNameData *data;

        g_assert (pool != NULL);

        data = g_new0 (LinuxLvm2LVSetNameData, 1);
        data->pool = g_object_ref (pool);
        data->callback = callback;
        data->user_data = user_data;

        org_freedesktop_UDisks_linux_lvm2_lv_set_name_async (pool->priv->proxy,
                                                             group_uuid,
                                                             uuid,
                                                             new_name,
                                                             op_linux_lvm2_lv_set_name_cb,
                                                             data);
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct {
        GduPool *pool;
        GduPoolLinuxLvm2LVRemoveCompletedFunc callback;
        gpointer user_data;
} LinuxLvm2LVRemoveData;

static void
op_linux_lvm2_lv_remove_cb (DBusGProxy *proxy, GError *error, gpointer user_data)
{
        LinuxLvm2LVRemoveData *data = user_data;
        _gdu_error_fixup (error);
        if (data->callback != NULL)
                data->callback (data->pool, error, data->user_data);
        g_object_unref (data->pool);
        g_free (data);
}

void
gdu_pool_op_linux_lvm2_lv_remove (GduPool *pool,
                                  const gchar *group_uuid,
                                  const gchar *uuid,
                                  GduPoolLinuxLvm2LVRemoveCompletedFunc callback,
                                  gpointer user_data)
{
        LinuxLvm2LVRemoveData *data;
        char *options[16];

        g_assert (pool != NULL);

        options[0] = NULL;

        data = g_new0 (LinuxLvm2LVRemoveData, 1);
        data->pool = g_object_ref (pool);
        data->callback = callback;
        data->user_data = user_data;

        org_freedesktop_UDisks_linux_lvm2_lv_remove_async (pool->priv->proxy,
                                                           group_uuid,
                                                           uuid,
                                                           (const char **) options,
                                                           op_linux_lvm2_lv_remove_cb,
                                                           data);
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct {
        GduPool *pool;
        GduPoolLinuxLvm2LVCreateCompletedFunc callback;
        gpointer user_data;
} LinuxLvm2LVCreateData;

static void
op_linux_lvm2_lv_create_cb (DBusGProxy *proxy,
                            char       *create_logical_volume_object_path,
                            GError *error,
                            gpointer user_data)
{
        LinuxLvm2LVCreateData *data = user_data;
        _gdu_error_fixup (error);
        if (data->callback != NULL)
                data->callback (data->pool, create_logical_volume_object_path, error, data->user_data);
        g_object_unref (data->pool);
        g_free (data);
}

void
gdu_pool_op_linux_lvm2_lv_create (GduPool *pool,
                                  const gchar *group_uuid,
                                  const gchar *name,
                                  guint64 size,
                                  guint num_stripes,
                                  guint64 stripe_size,
                                  guint num_mirrors,
                                  const char                             *fstype,
                                  const char                             *fslabel,
                                  const char                             *encrypt_passphrase,
                                  gboolean                                fs_take_ownership,
                                  GduPoolLinuxLvm2LVCreateCompletedFunc callback,
                                  gpointer user_data)
{
        LinuxLvm2LVCreateData *data;
        char *options[16];
        char *fsoptions[16];
        guint n;

        g_assert (pool != NULL);

        data = g_new0 (LinuxLvm2LVCreateData, 1);
        data->pool = g_object_ref (pool);
        data->callback = callback;
        data->user_data = user_data;

        options[0] = NULL;

        n = 0;
        if (fslabel != NULL && strlen (fslabel) > 0) {
                fsoptions[n++] = g_strdup_printf ("label=%s", fslabel);
        }
        if (encrypt_passphrase != NULL && strlen (encrypt_passphrase) > 0) {
                fsoptions[n++] = g_strdup_printf ("luks_encrypt=%s", encrypt_passphrase);
        }
        if (fs_take_ownership) {
                fsoptions[n++] = g_strdup_printf ("take_ownership_uid=%d", getuid ());
                fsoptions[n++] = g_strdup_printf ("take_ownership_gid=%d", getgid ());
        }
        fsoptions[n] = NULL;

        org_freedesktop_UDisks_linux_lvm2_lv_create_async (pool->priv->proxy,
                                                           group_uuid,
                                                           name,
                                                           size,
                                                           num_stripes,
                                                           stripe_size,
                                                           num_mirrors,
                                                           (const char **) options,
                                                           fstype,
                                                           (const char **) fsoptions,
                                                           op_linux_lvm2_lv_create_cb,
                                                           data);
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct {
        GduPool *pool;
        GduPoolLinuxLvm2VGAddPVCompletedFunc callback;
        gpointer user_data;
} LinuxLvm2VGAddPVData;

static void
op_linux_lvm2_vg_add_pv_cb (DBusGProxy *proxy, GError *error, gpointer user_data)
{
        LinuxLvm2VGAddPVData *data = user_data;
        _gdu_error_fixup (error);
        if (data->callback != NULL)
                data->callback (data->pool, error, data->user_data);
        g_object_unref (data->pool);
        g_free (data);
}

void
gdu_pool_op_linux_lvm2_vg_add_pv (GduPool *pool,
                                  const gchar *uuid,
                                  const gchar *physical_volume_object_path,
                                  GduPoolLinuxLvm2VGAddPVCompletedFunc callback,
                                  gpointer user_data)
{
        LinuxLvm2VGAddPVData *data;
        char *options[16];

        g_assert (pool != NULL);

        options[0] = NULL;

        data = g_new0 (LinuxLvm2VGAddPVData, 1);
        data->pool = g_object_ref (pool);
        data->callback = callback;
        data->user_data = user_data;

        org_freedesktop_UDisks_linux_lvm2_vg_add_pv_async (pool->priv->proxy,
                                                           uuid,
                                                           physical_volume_object_path,
                                                           (const gchar **) options,
                                                           op_linux_lvm2_vg_add_pv_cb,
                                                           data);
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct {
        GduPool *pool;
        GduPoolLinuxLvm2VGRemovePVCompletedFunc callback;
        gpointer user_data;
} LinuxLvm2VGRemovePVData;

static void
op_linux_lvm2_vg_remove_pv_cb (DBusGProxy *proxy, GError *error, gpointer user_data)
{
        LinuxLvm2VGRemovePVData *data = user_data;
        _gdu_error_fixup (error);
        if (data->callback != NULL)
                data->callback (data->pool, error, data->user_data);
        g_object_unref (data->pool);
        g_free (data);
}

void
gdu_pool_op_linux_lvm2_vg_remove_pv (GduPool *pool,
                                     const gchar *vg_uuid,
                                     const gchar *pv_uuid,
                                     GduPoolLinuxLvm2VGRemovePVCompletedFunc callback,
                                     gpointer user_data)
{
        LinuxLvm2VGRemovePVData *data;
        char *options[16];

        g_assert (pool != NULL);

        options[0] = NULL;

        data = g_new0 (LinuxLvm2VGRemovePVData, 1);
        data->pool = g_object_ref (pool);
        data->callback = callback;
        data->user_data = user_data;

        org_freedesktop_UDisks_linux_lvm2_vg_remove_pv_async (pool->priv->proxy,
                                                              vg_uuid,
                                                              pv_uuid,
                                                              (const gchar **) options,
                                                              op_linux_lvm2_vg_remove_pv_cb,
                                                              data);
}

/* ---------------------------------------------------------------------------------------------------- */

/**
 * gdu_pool_get_daemon_version:
 * @pool: A #GduPool.
 *
 * Get the version of the udisks on the system.
 *
 * Returns: The version of udisks daemon. Caller must free
 * this string using g_free().
 **/
char *
gdu_pool_get_daemon_version (GduPool *pool)
{
        return g_strdup (pool->priv->daemon_version);
}

/**
 * gdu_pool_is_daemon_inhibited:
 * @pool: A #GduPool.
 *
 * Checks if the daemon is currently inhibited.
 *
 * Returns: %TRUE if the daemon is inhibited.
 **/
gboolean
gdu_pool_is_daemon_inhibited (GduPool *pool)
{
        DBusGProxy *prop_proxy;
        gboolean ret;
        GError *error;
        GValue value = {0};

        g_assert (pool != NULL);

        /* TODO: this is a currently a synchronous call; when we port to EggDBus this will be fixed */

        ret = TRUE;

	prop_proxy = dbus_g_proxy_new_for_name (pool->priv->bus,
                                                "org.freedesktop.UDisks",
                                                "/org/freedesktop/UDisks",
                                                "org.freedesktop.DBus.Properties");
        error = NULL;
        if (!dbus_g_proxy_call (prop_proxy,
                                "Get",
                                &error,
                                G_TYPE_STRING,
                                "org.freedesktop.UDisks",
                                G_TYPE_STRING,
                                "daemon-is-inhibited",
                                G_TYPE_INVALID,
                                G_TYPE_VALUE,
                                &value,
                                G_TYPE_INVALID)) {
                g_warning ("Couldn't call Get() to determine if daemon is inhibited  for /: %s", error->message);
                g_error_free (error);
                ret = TRUE;
                goto out;
        }

        ret = g_value_get_boolean (&value);

 out:
        g_object_unref (prop_proxy);
        return ret;
}


/**
 * gdu_pool_get_known_filesystems:
 * @pool: A #GduPool.
 *
 * Get a list of file systems known to the udisks daemon.
 *
 * Returns: A #GList of #GduKnownFilesystem objects. Caller must free
 * this (unref all objects, then use g_list_free()).
 **/
GList *
gdu_pool_get_known_filesystems (GduPool *pool)
{
        GList *ret;
        g_assert (pool != NULL);

        ret = g_list_copy (pool->priv->known_filesystems);
        g_list_foreach (ret, (GFunc) g_object_ref, NULL);
        return ret;
}

/**
 * gdu_pool_get_known_filesystem_by_id:
 * @pool: A #GduPool.
 * @id: Identifier for the file system, e.g. <literal>ext3</literal> or <literal>vfat</literal>.
 *
 * Looks up a known file system by id.
 *
 * Returns: A #GduKnownFilesystem object or #NULL if file system
 * corresponding to @id exists. Caller must free this object using
 * g_object_unref().
 **/
GduKnownFilesystem *
gdu_pool_get_known_filesystem_by_id (GduPool *pool, const char *id)
{
        GList *l;
        GduKnownFilesystem *ret;
        g_assert (pool != NULL);


        ret = NULL;
        for (l = pool->priv->known_filesystems; l != NULL; l = l->next) {
                GduKnownFilesystem *kfs = GDU_KNOWN_FILESYSTEM (l->data);
                if (strcmp (gdu_known_filesystem_get_id (kfs), id) == 0) {
                        ret = g_object_ref (kfs);
                        goto out;
                }
        }
out:
        return ret;
}

/**
 * gdu_pool_supports_luks_devices:
 * @pool: A #GduPool.
 *
 * Determine if the udisks daemon supports LUKS encrypted
 * devices.
 *
 * Returns: #TRUE only if the daemon supports LUKS encrypted devices.
 **/
gboolean
gdu_pool_supports_luks_devices (GduPool *pool)
{
        g_assert (pool != NULL);
        return pool->priv->supports_luks_devices;
}

const gchar *
gdu_pool_get_ssh_user_name (GduPool *pool)
{
        g_assert (pool != NULL);
        return pool->priv->ssh_user_name;
}

const gchar *
gdu_pool_get_ssh_address (GduPool *pool)
{
        g_assert (pool != NULL);
        return pool->priv->ssh_address;
}

static void
remove_all_objects_and_dbus_proxies (GduPool *pool)
{
        g_free (pool->priv->daemon_version);
        pool->priv->daemon_version = NULL;

        g_list_foreach (pool->priv->known_filesystems, (GFunc) g_object_unref, NULL);
        g_list_free (pool->priv->known_filesystems);
        pool->priv->known_filesystems = NULL;

        g_hash_table_remove_all (pool->priv->object_path_to_device);
        g_hash_table_remove_all (pool->priv->object_path_to_adapter);
        g_hash_table_remove_all (pool->priv->object_path_to_expander);
        g_hash_table_remove_all (pool->priv->object_path_to_port);

        g_list_foreach (pool->priv->presentables, (GFunc) g_object_unref, NULL);
        g_list_free (pool->priv->presentables);
        pool->priv->presentables = NULL;

        if (pool->priv->proxy != NULL) {
                g_object_unref (pool->priv->proxy);
                pool->priv->proxy = NULL;
        }

        if (pool->priv->bus != NULL) {
                dbus_g_connection_unref (pool->priv->bus);
                pool->priv->bus = NULL;
        }
}

static void
_gdu_pool_disconnect (GduPool *pool)
{
        g_assert (pool != NULL);
        g_return_if_fail (!pool->priv->is_disconnected);

        remove_all_objects_and_dbus_proxies (pool);
        pool->priv->is_disconnected = TRUE;
        g_signal_emit (pool, signals[DISCONNECTED], 0);
}
