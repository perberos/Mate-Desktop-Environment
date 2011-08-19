/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-pool.h
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

#if !defined (__GDU_INSIDE_GDU_H) && !defined (GDU_COMPILATION)
#error "Only <gdu/gdu.h> can be included directly, this file may disappear or change contents."
#endif

#ifndef __GDU_POOL_H
#define __GDU_POOL_H

#include <gdu/gdu-types.h>
#include <gdu/gdu-callbacks.h>

G_BEGIN_DECLS

#define GDU_TYPE_POOL         (gdu_pool_get_type ())
#define GDU_POOL(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GDU_TYPE_POOL, GduPool))
#define GDU_POOL_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GDU_POOL,  GduPoolClass))
#define GDU_IS_POOL(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GDU_TYPE_POOL))
#define GDU_IS_POOL_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GDU_TYPE_POOL))
#define GDU_POOL_GET_CLASS(k) (G_TYPE_INSTANCE_GET_CLASS ((k), GDU_TYPE_POOL, GduPoolClass))

typedef struct _GduPoolClass       GduPoolClass;
typedef struct _GduPoolPrivate     GduPoolPrivate;

struct _GduPool
{
        GObject parent;

        /* private */
        GduPoolPrivate *priv;
};

struct _GduPoolClass
{
        GObjectClass parent_class;

        /* signals */
        void (*disconnected) (GduPool *pool);

        void (*device_added) (GduPool *pool, GduDevice *device);
        void (*device_removed) (GduPool *pool, GduDevice *device);
        void (*device_changed) (GduPool *pool, GduDevice *device);
        void (*device_job_changed) (GduPool *pool, GduDevice *device);

        void (*adapter_added) (GduPool *pool, GduAdapter *adapter);
        void (*adapter_removed) (GduPool *pool, GduAdapter *adapter);
        void (*adapter_changed) (GduPool *pool, GduAdapter *adapter);

        void (*expander_added) (GduPool *pool, GduExpander *expander);
        void (*expander_removed) (GduPool *pool, GduExpander *expander);
        void (*expander_changed) (GduPool *pool, GduExpander *expander);

        void (*port_added) (GduPool *pool, GduPort *port);
        void (*port_removed) (GduPool *pool, GduPort *port);
        void (*port_changed) (GduPool *pool, GduPort *port);

        void (*presentable_added) (GduPool *pool, GduPresentable *presentable);
        void (*presentable_removed) (GduPool *pool, GduPresentable *presentable);
        void (*presentable_changed) (GduPool *pool, GduPresentable *presentable);
        void (*presentable_job_changed) (GduPool *pool, GduPresentable *presentable);
};

GType       gdu_pool_get_type           (void);
GduPool    *gdu_pool_new                (void);
GduPool    *gdu_pool_new_for_address    (const gchar  *ssh_user_name,
                                         const gchar  *ssh_address,
                                         GError      **error);

const gchar *gdu_pool_get_ssh_user_name (GduPool *pool);
const gchar *gdu_pool_get_ssh_address   (GduPool *pool);

char       *gdu_pool_get_daemon_version (GduPool *pool);
gboolean    gdu_pool_is_daemon_inhibited (GduPool *pool);
gboolean    gdu_pool_supports_luks_devices (GduPool *pool);
GList      *gdu_pool_get_known_filesystems (GduPool *pool);
GduKnownFilesystem *gdu_pool_get_known_filesystem_by_id (GduPool *pool, const char *id);

GduDevice  *gdu_pool_get_by_object_path (GduPool *pool, const char *object_path);
GduDevice  *gdu_pool_get_by_device_file (GduPool *pool, const char *device_file);
GduPresentable *gdu_pool_get_volume_by_device      (GduPool *pool, GduDevice *device);
GduPresentable *gdu_pool_get_drive_by_device       (GduPool *pool, GduDevice *device);
GduLinuxMdDrive *gdu_pool_get_linux_md_drive_by_uuid (GduPool *pool, const gchar *uuid);

GduPresentable *gdu_pool_get_presentable_by_id     (GduPool *pool, const gchar *id);

gboolean    gdu_pool_has_presentable (GduPool *pool, GduPresentable *presentable);


GList      *gdu_pool_get_devices               (GduPool *pool);
GList      *gdu_pool_get_presentables          (GduPool *pool);
GList      *gdu_pool_get_enclosed_presentables (GduPool *pool, GduPresentable *presentable);

GduAdapter *gdu_pool_get_adapter_by_object_path (GduPool *pool, const char *object_path);
GList      *gdu_pool_get_adapters               (GduPool *pool);

GduExpander *gdu_pool_get_expander_by_object_path (GduPool *pool, const char *object_path);
GList      *gdu_pool_get_expanders               (GduPool *pool);

GduPort    *gdu_pool_get_port_by_object_path (GduPool *pool, const char *object_path);
GList      *gdu_pool_get_ports               (GduPool *pool);

GduPresentable *gdu_pool_get_hub_by_object_path (GduPool *pool, const gchar *object_path);

/* ---------------------------------------------------------------------------------------------------- */

void gdu_pool_op_linux_md_start (GduPool *pool,
                                 GPtrArray *component_objpaths,
                                 GduPoolLinuxMdStartCompletedFunc callback,
                                 gpointer user_data);

void gdu_pool_op_linux_md_create (GduPool     *pool,
                                  GPtrArray   *component_objpaths,
                                  const gchar *level,
                                  guint64      stripe_size,
                                  const gchar *name,
                                  GduPoolLinuxMdCreateCompletedFunc callback,
                                  gpointer user_data);

void gdu_pool_op_linux_lvm2_vg_start (GduPool *pool,
                                      const gchar *uuid,
                                      GduPoolLinuxLvm2VGStartCompletedFunc callback,
                                      gpointer user_data);

void gdu_pool_op_linux_lvm2_vg_stop (GduPool *pool,
                                     const gchar *uuid,
                                     GduPoolLinuxLvm2VGStopCompletedFunc callback,
                                     gpointer user_data);

void gdu_pool_op_linux_lvm2_lv_start (GduPool *pool,
                                      const gchar *group_uuid,
                                      const gchar *uuid,
                                      GduPoolLinuxLvm2VGStartCompletedFunc callback,
                                      gpointer user_data);

void gdu_pool_op_linux_lvm2_vg_set_name (GduPool *pool,
                                         const gchar *uuid,
                                         const gchar *new_name,
                                         GduPoolLinuxLvm2VGSetNameCompletedFunc callback,
                                         gpointer user_data);

void gdu_pool_op_linux_lvm2_lv_set_name (GduPool *pool,
                                         const gchar *group_uuid,
                                         const gchar *uuid,
                                         const gchar *new_name,
                                         GduPoolLinuxLvm2LVSetNameCompletedFunc callback,
                                         gpointer user_data);

void gdu_pool_op_linux_lvm2_lv_remove (GduPool *pool,
                                       const gchar *group_uuid,
                                       const gchar *uuid,
                                       GduPoolLinuxLvm2LVRemoveCompletedFunc callback,
                                       gpointer user_data);

void gdu_pool_op_linux_lvm2_lv_create (GduPool *pool,
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
                                       gpointer user_data);

void gdu_pool_op_linux_lvm2_vg_add_pv (GduPool *pool,
                                       const gchar *uuid,
                                       const gchar *physical_volume_object_path,
                                       GduPoolLinuxLvm2VGAddPVCompletedFunc callback,
                                       gpointer user_data);

void gdu_pool_op_linux_lvm2_vg_remove_pv (GduPool *pool,
                                          const gchar *vg_uuid,
                                          const gchar *pv_uuid,
                                          GduPoolLinuxLvm2VGRemovePVCompletedFunc callback,
                                          gpointer user_data);

G_END_DECLS

#endif /* __GDU_POOL_H */
