/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-shell.c
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
#include <glib/gi18n.h>

#include <string.h>
#include <dbus/dbus-glib.h>

#include <config.h>
#include <glib-object.h>
#include <string.h>
#include <glib/gi18n.h>

#include <gdu/gdu.h>
#include <gdu-gtk/gdu-gtk.h>

#include "gdu-shell.h"

#include "gdu-section-linux-md-drive.h"
#include "gdu-section-linux-lvm2-volume-group.h"
#include "gdu-section-drive.h"
#include "gdu-section-volumes.h"
#include "gdu-section-hub.h"

static gboolean add_pool (GduShell     *shell,
                          const gchar  *ssh_user_name,
                          const gchar  *ssh_address,
                          GError      **error);

struct _GduShellPrivate
{
        gchar *ssh_address;

        GtkWidget *app_window;
        GPtrArray *pools;

        GduPoolTreeModel *model;
        GtkWidget *tree_view;

        /* -------------------------------------------------------------------------------- */

        GtkWidget *sections_vbox;

        /* -------------------------------------------------------------------------------- */

        GduPresentable *presentable_now_showing;

        GtkActionGroup *action_group;
        GtkUIManager *ui_manager;
};

static GObjectClass *parent_class = NULL;

static GduSection *get_section_by_type (GduShell *shell, GType section_type);

G_DEFINE_TYPE (GduShell, gdu_shell, G_TYPE_OBJECT);

static void
gdu_shell_finalize (GduShell *shell)
{
        g_free (shell->priv->ssh_address);
        if (G_OBJECT_CLASS (parent_class)->finalize)
                (* G_OBJECT_CLASS (parent_class)->finalize) (G_OBJECT (shell));
}

static void
gdu_shell_class_init (GduShellClass *klass)
{
        GObjectClass *obj_class = (GObjectClass *) klass;

        parent_class = g_type_class_peek_parent (klass);

        obj_class->finalize = (GObjectFinalizeFunc) gdu_shell_finalize;

        g_type_class_add_private (klass, sizeof (GduShellPrivate));
}

static void create_window (GduShell        *shell);

static void
gdu_shell_init (GduShell *shell)
{
        shell->priv = G_TYPE_INSTANCE_GET_PRIVATE (shell, GDU_TYPE_SHELL, GduShellPrivate);

        shell->priv->pools = g_ptr_array_new ();
}

GduShell *
gdu_shell_new (const gchar *ssh_address)
{
        GduShell *shell;
        shell = GDU_SHELL (g_object_new (GDU_TYPE_SHELL, NULL));
        shell->priv->ssh_address = g_strdup (ssh_address);
        create_window (shell);
        return shell;
}

GtkWidget *
gdu_shell_get_toplevel (GduShell *shell)
{
        return shell->priv->app_window;
}

GduPool *
gdu_shell_get_pool_for_selected_presentable (GduShell *shell)
{
        GduPool *pool;

        if (shell->priv->presentable_now_showing != NULL) {
                pool = gdu_presentable_get_pool (shell->priv->presentable_now_showing);
                g_object_unref (pool);
        } else {
                pool = NULL;
        }

        return pool;
}


GduPresentable *
gdu_shell_get_selected_presentable (GduShell *shell)
{
        return shell->priv->presentable_now_showing;
}

void
gdu_shell_select_presentable (GduShell *shell, GduPresentable *presentable)
{
        gboolean selected;
        GduPool *pool;

        pool = gdu_presentable_get_pool (presentable);

        if (GDU_IS_VOLUME (presentable)) {
                GduDevice *device;
                GduPresentable *p_to_select;

                p_to_select = NULL;
                device = gdu_presentable_get_device (presentable);
                if (device != NULL) {
                        if (gdu_device_is_partition (device)) {
                                GduDevice *drive_device;
                                drive_device = gdu_pool_get_by_object_path (pool,
                                                                            gdu_device_partition_get_slave (device));
                                if (drive_device != NULL) {
                                        p_to_select = gdu_pool_get_drive_by_device (pool, drive_device);
                                        g_object_unref (drive_device);
                                }
                        } else {
                                p_to_select = gdu_pool_get_drive_by_device (pool, device);
                        }
                        g_object_unref (device);
                }

                if (p_to_select != NULL) {
                        GduSection *volumes_section;

                        gdu_pool_tree_view_select_presentable (GDU_POOL_TREE_VIEW (shell->priv->tree_view),
                                                               p_to_select);
                        gtk_widget_grab_focus (shell->priv->tree_view);

                        volumes_section = get_section_by_type (shell, GDU_TYPE_SECTION_VOLUMES);
                        g_warn_if_fail (volumes_section != NULL);
                        if (volumes_section != NULL) {
                                g_warn_if_fail (gdu_section_volumes_select_volume (GDU_SECTION_VOLUMES (volumes_section),
                                                                                   presentable));
                        }

                        selected = TRUE;
                        g_object_unref (p_to_select);
                }
        } else {
                gdu_pool_tree_view_select_presentable (GDU_POOL_TREE_VIEW (shell->priv->tree_view), presentable);
                gtk_widget_grab_focus (shell->priv->tree_view);
                selected = TRUE;
        }

        if (!selected) {
                g_warning ("%s: %s: Unhandled presentable %s",
                           G_STRLOC, G_STRFUNC,
                           gdu_presentable_get_id (presentable));
        }

        g_object_unref (pool);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
remove_section (GtkWidget *widget, gpointer callback_data)
{
        gtk_container_remove (GTK_CONTAINER (callback_data), widget);
}

static void
update_section (GtkWidget *section, gpointer callback_data)
{
        gdu_section_update (GDU_SECTION (section));
}

static GList *
compute_sections_to_show (GduShell *shell)
{
        GList *sections_to_show;

        sections_to_show = NULL;
        if (shell->priv->presentable_now_showing != NULL) {
                if (GDU_IS_HUB (shell->priv->presentable_now_showing)) {

                        sections_to_show = g_list_append (sections_to_show, (gpointer) GDU_TYPE_SECTION_HUB);

                } else if (GDU_IS_LINUX_MD_DRIVE (shell->priv->presentable_now_showing)) {

                        sections_to_show = g_list_append (sections_to_show, (gpointer) GDU_TYPE_SECTION_LINUX_MD_DRIVE);
                        sections_to_show = g_list_append (sections_to_show, (gpointer) GDU_TYPE_SECTION_VOLUMES);

                } else if (GDU_IS_LINUX_LVM2_VOLUME_GROUP (shell->priv->presentable_now_showing)) {

                        sections_to_show = g_list_append (sections_to_show, (gpointer) GDU_TYPE_SECTION_LINUX_LVM2_VOLUME_GROUP);
                        sections_to_show = g_list_append (sections_to_show, (gpointer) GDU_TYPE_SECTION_VOLUMES);

                } else if (GDU_IS_DRIVE (shell->priv->presentable_now_showing)) {

                        sections_to_show = g_list_append (sections_to_show, (gpointer) GDU_TYPE_SECTION_DRIVE);
                        sections_to_show = g_list_append (sections_to_show, (gpointer) GDU_TYPE_SECTION_VOLUMES);
                }
        }

        return sections_to_show;
}


static void
update_title (GduShell *shell)
{
        GduPresentable *presentable;
        gchar *name;
        gchar *vpd_name;
        gchar *s;
        GduPool *pool;
        const gchar *ssh_address;
        GduDevice *device;
        gchar *device_file;

        presentable = shell->priv->presentable_now_showing;

        if (presentable == NULL) {
                /* Translators: Window title when no item is selected.
                 */
                s = g_strdup (_("Disk Utility"));
                goto out;
        }

        pool = gdu_presentable_get_pool (presentable);
        name = gdu_presentable_get_name (presentable);
        vpd_name = gdu_presentable_get_vpd_name (presentable);
        device = gdu_presentable_get_device (presentable);
        device_file = NULL;
        if (device != NULL) {
                device_file = g_strdup (gdu_device_get_device_file (device));
                g_object_unref (device);
        }

        ssh_address = gdu_pool_get_ssh_address (pool);

        if (ssh_address != NULL) {
                if (GDU_IS_MACHINE (presentable)) {
                        /* Translators: Window title when the item representing a remote machine is selected.
                         * First %s is the hostname of the remote host (e.g. 'widget.mate.org')
                         */
                        s = g_strdup_printf (_("%s — Disk Utility"),
                                             ssh_address);
                } else {
                        if (device_file != NULL) {
                                /* Translators: Window title when an item on a remote machine is selected and there is a device file.
                                 * First %s is the name of the item (e.g. '80 GB Solid-State Disk' or 'Saturn').
                                 * Second %s is the VPD name of the item (e.g. 'ATA INTEL SSDSA2MH080G1GC' or '6 TB RAID-6 Array').
                                 * Third is the device file (e.g. '/dev/sda')
                                 * Fourth %s is the hostname of the remote host (e.g. 'widget.mate.org')
                                 */
                                s = g_strdup_printf (_("%s (%s) [%s] @ %s — Disk Utility"),
                                                     name,
                                                     vpd_name,
                                                     device_file,
                                                     ssh_address);
                        } else {
                                /* Translators: Window title when an item on a remote machine is selected.
                                 * First %s is the name of the item (e.g. '80 GB Solid-State Disk' or 'Saturn').
                                 * Second %s is the VPD name of the item (e.g. 'ATA INTEL SSDSA2MH080G1GC' or '6 TB RAID-6 Array').
                                 * Third %s is the hostname of the remote host (e.g. 'widget.mate.org')
                                 */
                                s = g_strdup_printf (_("%s (%s) @ %s — Disk Utility"),
                                                     name,
                                                     vpd_name,
                                                     ssh_address);
                        }
                }
        } else {
                if (GDU_IS_MACHINE (presentable)) {
                        /* Translators: Window title when the item representing the local machine is selected.
                         */
                        s = g_strdup (_("Disk Utility"));
                } else {
                        if (device_file != NULL) {
                                /* Translators: Window title when an item on the local machine is selected and there is a device file.
                                 * First %s is the name of the item (e.g. '80 GB Solid-State Disk' or 'Saturn').
                                 * Second %s is the VPD name of the item (e.g. 'ATA INTEL SSDSA2MH080G1GC' or '6 TB RAID-6 Array').
                                 * Third is the device file (e.g. '/dev/sda')
                                 */
                                s = g_strdup_printf (_("%s (%s) [%s] — Disk Utility"),
                                                     name,
                                                     vpd_name,
                                                     device_file);
                        } else {
                                /* Translators: Window title when an item on the local machine is selected.
                                 * First %s is the name of the item (e.g. '80 GB Solid-State Disk' or 'Saturn').
                                 * Second %s is the VPD name of the item (e.g. 'ATA INTEL SSDSA2MH080G1GC' or '6 TB RAID-6 Array').
                                 */
                                s = g_strdup_printf (_("%s (%s) — Disk Utility"),
                                                     name,
                                                     vpd_name);
                        }
                }
        }
        g_free (vpd_name);
        g_free (name);
        g_object_unref (pool);
        g_free (device_file);

 out:
        gtk_window_set_title (GTK_WINDOW (shell->priv->app_window), s);
        g_free (s);
}

/* called when a new presentable is selected
 *  - or a presentable changes
 *  - or the job state of a presentable changes
 *
 * and to update visibility of embedded widgets
 */
void
gdu_shell_update (GduShell *shell)
{
        static GduPresentable *last_presentable = NULL;
        gboolean reset_sections;
        GList *sections_to_show;

        update_title (shell);

        reset_sections = FALSE;
        if (shell->priv->presentable_now_showing == NULL) {
                reset_sections = TRUE;
        } else if (last_presentable == NULL) {
                reset_sections = TRUE;
        } else if (g_strcmp0 (gdu_presentable_get_id (shell->priv->presentable_now_showing),
                              gdu_presentable_get_id (last_presentable)) != 0) {
                reset_sections = TRUE;
        }
        last_presentable = shell->priv->presentable_now_showing;

        sections_to_show = compute_sections_to_show (shell);

        /* if this differs from what we currently show, prompt a reset */
        if (!reset_sections) {
                GList *children;
                GList *l;
                GList *ll;

                children = gtk_container_get_children (GTK_CONTAINER (shell->priv->sections_vbox));
                if (g_list_length (children) != g_list_length (sections_to_show)) {
                        reset_sections = TRUE;
                } else {
                        for (l = sections_to_show, ll = children; l != NULL; l = l->next, ll = ll->next) {
                                if (G_OBJECT_TYPE (ll->data) != (GType) l->data) {
                                        reset_sections = TRUE;
                                        break;
                                }
                        }
                }
                g_list_free (children);
        }

        if (reset_sections) {
                GList *l;

                /* out with the old... */
                gtk_container_foreach (GTK_CONTAINER (shell->priv->sections_vbox),
                                       remove_section,
                                       shell->priv->sections_vbox);

                /* ... and in with the new */
                for (l = sections_to_show; l != NULL; l = l->next) {
                        GType type = (GType) l->data;
                        GtkWidget *section;

                        section = g_object_new (type,
                                                "shell", shell,
                                                "presentable", shell->priv->presentable_now_showing,
                                                NULL);

                        gtk_widget_show_all (section);

                        gtk_box_pack_start (GTK_BOX (shell->priv->sections_vbox),
                                            section,
                                            FALSE, FALSE, 0);
                }

        }
        g_list_free (sections_to_show);

        /* update all sections */
        gtk_container_foreach (GTK_CONTAINER (shell->priv->sections_vbox),
                               update_section,
                               shell);
}

static GduSection *
get_section_by_type (GduShell *shell, GType section_type)
{
        GList *children;
        GList *l;
        GduSection *ret;

        ret = NULL;
        children = gtk_container_get_children (GTK_CONTAINER (shell->priv->sections_vbox));
        for (l = children; l != NULL; l = l->next) {
                GduSection *section = GDU_SECTION (l->data);

                if (g_type_is_a (G_OBJECT_TYPE (section), section_type)) {
                        ret = section;
                        break;
                }
        }
        g_list_free (children);

        return ret;
}

static void
presentable_changed (GduPresentable *presentable, gpointer user_data)
{
        GduShell *shell = GDU_SHELL (user_data);
        gdu_shell_update (shell);
}

static void
presentable_job_changed (GduPresentable *presentable, gpointer user_data)
{
        GduShell *shell = GDU_SHELL (user_data);
        gdu_shell_update (shell);
}

static void
device_tree_changed (GtkTreeSelection *selection, gpointer user_data)
{
        GduShell *shell = GDU_SHELL (user_data);
        GduPresentable *presentable;

        presentable = gdu_pool_tree_view_get_selected_presentable (GDU_POOL_TREE_VIEW (shell->priv->tree_view));

        //g_debug ("selected=%p - now_showing=%p", presentable, shell->priv->presentable_now_showing);

        if (shell->priv->presentable_now_showing != NULL) {
                g_signal_handlers_disconnect_by_func (shell->priv->presentable_now_showing,
                                                      (GCallback) presentable_changed,
                                                      shell);
                g_signal_handlers_disconnect_by_func (shell->priv->presentable_now_showing,
                                                      (GCallback) presentable_job_changed,
                                                      shell);
                g_object_unref (shell->priv->presentable_now_showing);
                shell->priv->presentable_now_showing = NULL;
        }

        if (presentable != NULL) {
                /* steal the reference */
                shell->priv->presentable_now_showing = presentable;

                g_signal_connect (shell->priv->presentable_now_showing, "changed",
                                  (GCallback) presentable_changed, shell);
                g_signal_connect (shell->priv->presentable_now_showing, "job-changed",
                                  (GCallback) presentable_job_changed, shell);

                gdu_shell_update (shell);
        } else {
                g_warning ("No presentable currently selected");
        }
}

static void
presentable_added (GduPool *pool, GduPresentable *presentable, gpointer user_data)
{
        GduShell *shell = GDU_SHELL (user_data);
        gdu_shell_update (shell);
}

static void
presentable_removed (GduPool *pool, GduPresentable *presentable, gpointer user_data)
{
        GduShell *shell = GDU_SHELL (user_data);
        GduPresentable *enclosing_presentable;

        if (shell->priv->presentable_now_showing != NULL &&
            g_strcmp0 (gdu_presentable_get_id (presentable),
                       gdu_presentable_get_id (shell->priv->presentable_now_showing)) == 0) {

                /* Try going to the enclosing presentable if that one
                 * is available. Otherwise go to the first one.
                 */

                enclosing_presentable = gdu_presentable_get_enclosing_presentable (presentable);
                if (enclosing_presentable != NULL) {
                        gdu_shell_select_presentable (shell, enclosing_presentable);
                        g_object_unref (enclosing_presentable);
                } else {
                        gdu_pool_tree_view_select_first_presentable (GDU_POOL_TREE_VIEW (shell->priv->tree_view));
                        gtk_widget_grab_focus (shell->priv->tree_view);
                }
        }
        gdu_shell_update (shell);
}

static void
help_contents_action_callback (GtkAction *action, gpointer user_data)
{
        /* TODO */
        //mate_help_display ("mate-disk-utility.xml", NULL, NULL);
        g_warning ("TODO: launch help");
}
/* ---------------------------------------------------------------------------------------------------- */

typedef struct {
        GduShell *shell;
        GduPool *pool;

        /* data obtained from GduCreateLinuxMdDialog */
        gchar *level;
        gchar *name;
        guint64 size;
        guint64 component_size;
        guint64 stripe_size;
        GPtrArray *drives;

        /* List of created components - GduDevice objects */
        GPtrArray *components;

} CreateLinuxMdData;

static void
create_linux_md_data_free (CreateLinuxMdData *data)
{
        g_object_unref (data->shell);
        g_object_unref (data->pool);
        g_free (data->level);
        g_free (data->name);
        g_ptr_array_unref (data->drives);
        g_ptr_array_unref (data->components);
        g_free (data);
}

static void create_linux_md_do (CreateLinuxMdData *data);

static void
md_raid_create_volume_cb (GduDrive     *drive,
                          GAsyncResult *res,
                          gpointer      user_data)
{
        CreateLinuxMdData *data = user_data;
        GduVolume *volume;
        GError *error;

        error = NULL;
        volume = gdu_drive_create_volume_finish (drive,
                                                 res,
                                                 &error);
        if (volume == NULL) {
                gdu_shell_raise_error (data->shell,
                                       NULL,
                                       error,
                                       _("Error creating component for RAID array"));
                g_error_free (error);
        } else {
                GduDevice *d;

                d = gdu_presentable_get_device (GDU_PRESENTABLE (volume));
                g_ptr_array_add (data->components, d);
                g_object_unref (volume);

                /* now that we have a component... carry on... */
                create_linux_md_do (data);
        }

}

static void
new_linux_md_create_array_cb (GduPool    *pool,
                              char       *array_object_path,
                              GError     *error,
                              gpointer    user_data)
{

        CreateLinuxMdData *data = user_data;

        if (error != NULL) {
                gdu_shell_raise_error (data->shell,
                                       NULL,
                                       error,
                                       _("Error creating RAID array"));
                g_error_free (error);

                //g_debug ("Error creating array");
        } else {
                GduDevice *d;
                GduPresentable *p;

                /* YAY - array has been created - switch the shell to it */
                d = gdu_pool_get_by_object_path (pool, array_object_path);
                p = gdu_pool_get_drive_by_device (pool, d);
                gdu_shell_select_presentable (data->shell, p);
                g_object_unref (p);
                g_object_unref (d);

                //g_debug ("Done creating array");
        }

        create_linux_md_data_free (data);
}

static void
create_linux_md_do (CreateLinuxMdData *data)
{
        if (data->components->len == data->drives->len) {
                GPtrArray *objpaths;
                guint n;

                /* Create array */
                //g_debug ("Yay, now creating array");

                objpaths = g_ptr_array_new ();
                for (n = 0; n < data->components->len; n++) {
                        GduDevice *d = GDU_DEVICE (data->components->pdata[n]);
                        g_ptr_array_add (objpaths, (gpointer) gdu_device_get_object_path (d));
                }

                gdu_pool_op_linux_md_create (data->pool,
                                             objpaths,
                                             data->level,
                                             data->stripe_size,
                                             data->name,
                                             new_linux_md_create_array_cb,
                                             data);
                g_ptr_array_free (objpaths, TRUE);

        } else {
                GduDrive *drive;
                guint num_component;

                num_component = data->components->len;
                drive = GDU_DRIVE (data->drives->pdata[num_component]);

                gdu_drive_create_volume (drive,
                                         data->component_size,
                                         data->name,
                                         GDU_CREATE_VOLUME_FLAGS_LINUX_MD,
                                         (GAsyncReadyCallback) md_raid_create_volume_cb,
                                         data);
        }
}

static void
new_linux_md_array_callback (GtkAction *action, gpointer user_data)
{
        GduShell *shell = GDU_SHELL (user_data);
        GtkWidget *dialog;
        gint response;
        GduPool *pool;

        //g_debug ("New Linux MD Array!");

        pool = gdu_shell_get_pool_for_selected_presentable (shell);

        dialog = gdu_create_linux_md_dialog_new (GTK_WINDOW (shell->priv->app_window), pool);

        gtk_widget_show_all (dialog);
        response = gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_hide (dialog);

        if (response == GTK_RESPONSE_OK) {
                CreateLinuxMdData *data;

                data = g_new0 (CreateLinuxMdData, 1);
                data->pool           = g_object_ref (pool);
                data->shell          = g_object_ref (shell);
                data->level          = gdu_create_linux_md_dialog_get_level (GDU_CREATE_LINUX_MD_DIALOG (dialog));
                data->name           = gdu_create_linux_md_dialog_get_name (GDU_CREATE_LINUX_MD_DIALOG (dialog));
                data->size           = gdu_create_linux_md_dialog_get_size (GDU_CREATE_LINUX_MD_DIALOG (dialog));
                data->component_size = gdu_create_linux_md_dialog_get_component_size (GDU_CREATE_LINUX_MD_DIALOG (dialog));
                data->stripe_size    = gdu_create_linux_md_dialog_get_stripe_size (GDU_CREATE_LINUX_MD_DIALOG (dialog));
                data->drives         = gdu_create_linux_md_dialog_get_drives (GDU_CREATE_LINUX_MD_DIALOG (dialog));

                data->components  = g_ptr_array_new_with_free_func (g_object_unref);

                create_linux_md_do (data);
        }
        gtk_widget_destroy (dialog);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
on_file_connect_action (GtkAction *action,
                        gpointer   user_data)
{
        GduShell *shell = GDU_SHELL (user_data);
        GtkWidget *dialog;
        gint response;

        dialog = gdu_connect_to_server_dialog_new (GTK_WINDOW (shell->priv->app_window));
        gtk_widget_show_all (dialog);
        response = gtk_dialog_run (GTK_DIALOG (dialog));

        if (response == GTK_RESPONSE_OK) {
                const gchar *user_name;
                const gchar *address;
                GError *error;

                user_name = gdu_connect_to_server_dialog_get_user_name (GDU_CONNECT_TO_SERVER_DIALOG (dialog));
                address = gdu_connect_to_server_dialog_get_address (GDU_CONNECT_TO_SERVER_DIALOG (dialog));

                gtk_widget_destroy (dialog);

                error = NULL;
                if (!add_pool (shell, user_name, address, &error)) {
                        GtkWidget *dialog;
                        gchar *s;

                        s = g_strdup_printf (_("Error connecting to “%s”"), address);

                        dialog = gdu_error_dialog_new (GTK_WINDOW (gdu_shell_get_toplevel (shell)),
                                                       NULL,
                                                       s,
                                                       error);
                        g_free (s);
                        gtk_widget_show_all (dialog);
                        gtk_window_present (GTK_WINDOW (dialog));
                        gtk_dialog_run (GTK_DIALOG (dialog));
                        gtk_widget_destroy (dialog);

                        g_error_free (error);
                }
        } else {
                gtk_widget_destroy (dialog);
        }
}

/* ---------------------------------------------------------------------------------------------------- */

static void
quit_action_callback (GtkAction *action, gpointer user_data)
{
        gtk_main_quit ();
}

static void
about_action_callback (GtkAction *action, gpointer user_data)
{
        GduShell *shell = GDU_SHELL (user_data);
        GdkPixbuf *logo;
        const char *artists[] = {
                "Mike Langlie <mlanglie@redhat.com>",
                NULL
        };
        const char *authors[] = {
                "David Zeuthen <davidz@redhat.com>",
                NULL
        };

        logo = gtk_icon_theme_load_icon (gtk_icon_theme_get_default (),
                                         "palimpsest",
                                         96,
                                         GTK_ICON_LOOKUP_GENERIC_FALLBACK,
                                         NULL);

        gtk_show_about_dialog (GTK_WINDOW (shell->priv->app_window),
                               "program-name", _("Disk Utility"),
                               "version", VERSION,
                               "copyright", "\xc2\xa9 2007-2009 Red Hat, Inc.",
                               "authors", authors,
                               "artists", artists,
                               "translator-credits", _("translator-credits"),
                               "logo", logo,
                               NULL);
        if (logo != NULL)
                g_object_unref (logo);
}

static const gchar *ui =
        "<ui>"
        "  <menubar>"
        "    <menu action='file'>"
#if HAVE_REMOTE_ACCESS
        "      <menuitem action='file-connect'/>"
#endif
        "      <menu action='file-create'>"
        "        <menuitem action='file-create-linux-md-array'/>"
        "      </menu>"
        "      <menuitem action='quit'/>"
        "    </menu>"
        "    <menu action='help'>"
#if 0
        "      <menuitem action='contents'/>"
#endif
        "      <menuitem action='about'/>"
        "    </menu>"
        "  </menubar>"
        "</ui>";

static GtkActionEntry entries[] = {
        {"file", NULL, N_("_File"), NULL, NULL, NULL },
        {"file-connect", "gtk-connect", N_("Connect to _Server..."), NULL, N_("Manage storage devices on another machine"), G_CALLBACK (on_file_connect_action)},
        {"file-create", NULL, N_("_Create"), NULL, NULL, NULL },
        {"file-create-linux-md-array", "gdu-raid-array", N_("_RAID Array..."), NULL, N_("Create a RAID array"), G_CALLBACK (new_linux_md_array_callback)},
        {"edit", NULL, N_("_Edit"), NULL, NULL, NULL },
        {"help", NULL, N_("_Help"), NULL, NULL, NULL },

        {"quit", GTK_STOCK_QUIT, N_("_Quit"), "<Ctrl>Q", N_("Quit"), G_CALLBACK (quit_action_callback)},
        {"contents", GTK_STOCK_HELP, N_("_Help"), "F1", N_("Get Help on Disk Utility"), G_CALLBACK (help_contents_action_callback)},
        {"about", GTK_STOCK_ABOUT, N_("_About"), NULL, NULL, G_CALLBACK (about_action_callback)}
};

static GtkUIManager *
create_ui_manager (GduShell *shell)
{
        GtkUIManager *ui_manager;
        GError *error;

        shell->priv->action_group = gtk_action_group_new ("MateDiskUtilityActions");
        gtk_action_group_set_translation_domain (shell->priv->action_group, NULL);
        gtk_action_group_add_actions (shell->priv->action_group, entries, G_N_ELEMENTS (entries), shell);

        /* -------------------------------------------------------------------------------- */

        ui_manager = gtk_ui_manager_new ();
        gtk_ui_manager_insert_action_group (ui_manager, shell->priv->action_group, 0);

        error = NULL;
        if (!gtk_ui_manager_add_ui_from_string
            (ui_manager, ui, -1, &error)) {
                g_message ("Building menus failed: %s", error->message);
                g_error_free (error);
                gtk_main_quit ();
        }

        return ui_manager;
}

static void
fix_focus_cb (GtkDialog *dialog, gpointer data)
{
        GtkWidget *button;

        button = gtk_window_get_default_widget (GTK_WINDOW (dialog));
        gtk_widget_grab_focus (button);
}

static void
expander_cb (GtkExpander *expander, GParamSpec *pspec, GtkWindow *dialog)
{
        gtk_window_set_resizable (dialog, gtk_expander_get_expanded (expander));
}

/**
 * gdu_shell_raise_error:
 * @shell: An object implementing the #GduShell interface
 * @presentable: The #GduPresentable for which the error was raised or %NULL.
 * @error: The #GError obtained from the operation
 * @primary_markup_format: Format string for the primary markup text of the dialog
 * @...: Arguments for markup string
 *
 * Show the user (through a dialog or other means (e.g. cluebar)) that an error occured.
 **/
void
gdu_shell_raise_error (GduShell       *shell,
                       GduPresentable *presentable,
                       GError         *error,
                       const char     *primary_markup_format,
                       ...)
{
        GtkWidget *dialog;
        char *error_text;
        char *window_title;
        GIcon *window_icon;
        va_list args;
        char *error_msg;
        GtkWidget *box, *hbox, *expander, *sw, *tv;
        GList *children;
        GtkTextBuffer *buffer;

        g_return_if_fail (shell != NULL);
        g_return_if_fail (error != NULL);

        window_icon = NULL;
        if (presentable != NULL) {
                window_title = gdu_presentable_get_name (presentable);
                window_icon = gdu_presentable_get_icon (presentable);
        } else {
                window_title = g_strdup (_("An error occured"));
        }

        va_start (args, primary_markup_format);
        error_text = g_strdup_vprintf (primary_markup_format, args);
        va_end (args);

        switch (error->code) {
        case GDU_ERROR_FAILED:
                error_msg = _("The operation failed.");
                break;
        case GDU_ERROR_BUSY:
                error_msg = _("The device is busy.");
                break;
        case GDU_ERROR_CANCELLED:
                error_msg = _("The operation was canceled.");
                break;
        case GDU_ERROR_INHIBITED:
                error_msg = _("The daemon is being inhibited.");
                break;
        case GDU_ERROR_INVALID_OPTION:
                error_msg = _("An invalid option was passed.");
                break;
        case GDU_ERROR_NOT_SUPPORTED:
                error_msg = _("The operation is not supported.");
                break;
        case GDU_ERROR_ATA_SMART_WOULD_WAKEUP:
                error_msg = _("Getting ATA SMART data would wake up the device.");
                break;
        case GDU_ERROR_PERMISSION_DENIED:
                error_msg = _("Permission denied.");
                break;
        default:
                error_msg = _("Unknown error");
                break;
        }

        dialog = gtk_message_dialog_new_with_markup (
                GTK_WINDOW (shell->priv->app_window),
                GTK_DIALOG_MODAL|GTK_DIALOG_DESTROY_WITH_PARENT,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_CLOSE,
                "<big><b>%s</b></big>",
                error_text);
        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s", error_msg);

	/* Here we cheat a little by poking in the messagedialog internals
         * to add the details expander to the inner vbox and arrange things
         * so that resizing the dialog works as expected.
         */
	box = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
	children = gtk_container_get_children (GTK_CONTAINER (box));
	hbox = GTK_WIDGET (children->data);
	gtk_container_child_set (GTK_CONTAINER (box), hbox,
                                 "expand", TRUE,
                                 "fill", TRUE,
                                 NULL);
	g_list_free (children);
	children = gtk_container_get_children (GTK_CONTAINER (hbox));
	box = GTK_WIDGET (children->next->data);
	g_list_free (children);
	children = gtk_container_get_children (GTK_CONTAINER (box));
	gtk_container_child_set (GTK_CONTAINER (box), GTK_WIDGET (children->next->data),
                                 "expand", FALSE,
                                 "fill", FALSE,
                                 NULL);
	g_list_free (children);

	expander = g_object_new (GTK_TYPE_EXPANDER,
                                 "label", _("_Details:"),
                                 "use-underline", TRUE,
                                 "use-markup", TRUE,
                                 NULL);
        sw = g_object_new (GTK_TYPE_SCROLLED_WINDOW,
                           "hscrollbar-policy", GTK_POLICY_AUTOMATIC,
                           "vscrollbar-policy", GTK_POLICY_AUTOMATIC,
                           "shadow-type", GTK_SHADOW_IN,
                           NULL);
        buffer = gtk_text_buffer_new (NULL);
        gtk_text_buffer_set_text (buffer, error->message, -1);
	tv = gtk_text_view_new_with_buffer (buffer);
        gtk_text_view_set_editable (GTK_TEXT_VIEW (tv), FALSE);

        gtk_container_add (GTK_CONTAINER (sw), tv);
        gtk_container_add (GTK_CONTAINER (expander), sw);
	gtk_box_pack_end (GTK_BOX (box), expander, TRUE, TRUE, 0);
        gtk_widget_show_all (expander);

        /* Make the window resizable when the details are visible
         */
	g_signal_connect (expander, "notify::expanded", G_CALLBACK (expander_cb), dialog);

        /* We don't want the initial focus to end up on the expander,
         * so grab it to the close button on map.
         */
        gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
        g_signal_connect (dialog, "map", G_CALLBACK (fix_focus_cb), NULL);


        // TODO: no support for GIcon in GtkWindow
        //gtk_window_set_icon_name (GTK_WINDOW (dialog), window_icon_name);

        g_signal_connect_swapped (dialog,
                                  "response",
                                  G_CALLBACK (gtk_widget_destroy),
                                  dialog);
        gtk_window_present (GTK_WINDOW (dialog));

        g_free (window_title);
        if (window_icon != NULL)
                g_object_unref (window_icon);
        g_free (error_text);
}

static void
pool_disconnected (GduPool  *pool,
                   gpointer user_data)
{
        GduShell *shell = GDU_SHELL (user_data);

        g_warn_if_fail (g_ptr_array_remove (shell->priv->pools, pool));
        g_object_unref (pool);

        gdu_pool_tree_model_set_pools (shell->priv->model, shell->priv->pools);
}

static gboolean
add_pool (GduShell     *shell,
          const gchar  *ssh_user_name,
          const gchar  *ssh_address,
          GError      **error)
{
        GduPool *pool;
        gboolean ret;

        ret = FALSE;

        pool = gdu_pool_new_for_address (ssh_user_name, ssh_address, error);
        if (pool == NULL)
                goto out;

        g_signal_connect (pool, "presentable-added", (GCallback) presentable_added, shell);
        g_signal_connect (pool, "presentable-removed", (GCallback) presentable_removed, shell);
        g_signal_connect (pool, "disconnected", (GCallback) pool_disconnected, shell);

        g_ptr_array_add (shell->priv->pools, pool);

        if (shell->priv->model != NULL) {
                GduPresentable *selected_presentable;

                selected_presentable = gdu_shell_get_selected_presentable (shell);

                gdu_pool_tree_model_set_pools (shell->priv->model, shell->priv->pools);

                if (selected_presentable != NULL)
                        gdu_shell_select_presentable (shell, selected_presentable);
        }

        ret = TRUE;

 out:
        return ret;
}

static void
create_window (GduShell *shell)
{
        GtkWidget *vbox;
        GtkWidget *vbox1;
        GtkWidget *vbox2;
        GtkWidget *menubar;
        GtkAccelGroup *accel_group;
        GtkWidget *hpane;
        GtkWidget *tree_view_scrolled_window;
        GtkTreeSelection *select;
        GtkWidget *label;
        GtkTreeViewColumn *column;
        GError *error;

        error = NULL;
        if (!add_pool (shell, NULL, NULL, &error)) {
                g_printerr ("Error creating pool: `%s'\n", error->message);
                g_error_free (error);
                g_critical ("Bailing out");
        }

        shell->priv->app_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
        gtk_window_set_resizable (GTK_WINDOW (shell->priv->app_window), TRUE);
        gtk_window_set_default_size (GTK_WINDOW (shell->priv->app_window), 800, 600);
        gtk_window_set_title (GTK_WINDOW (shell->priv->app_window), _("Disk Utility"));

        vbox = gtk_vbox_new (FALSE, 0);
        gtk_container_add (GTK_CONTAINER (shell->priv->app_window), vbox);

        shell->priv->ui_manager = create_ui_manager (shell);
        accel_group = gtk_ui_manager_get_accel_group (shell->priv->ui_manager);
        gtk_window_add_accel_group (GTK_WINDOW (shell->priv->app_window), accel_group);

        menubar = gtk_ui_manager_get_widget (shell->priv->ui_manager, "/menubar");
        gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, FALSE, 0);

        /* tree view */
        tree_view_scrolled_window = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (tree_view_scrolled_window),
                                        GTK_POLICY_NEVER,
                                        GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (tree_view_scrolled_window),
                                             GTK_SHADOW_IN);
        shell->priv->model = gdu_pool_tree_model_new (shell->priv->pools,
                                                      NULL,
                                                      GDU_POOL_TREE_MODEL_FLAGS_NO_VOLUMES);
        shell->priv->tree_view = gdu_pool_tree_view_new (shell->priv->model,
                                                         GDU_POOL_TREE_VIEW_FLAGS_NONE);
        g_object_unref (shell->priv->model);
        gtk_container_add (GTK_CONTAINER (tree_view_scrolled_window), shell->priv->tree_view);


        /* --- */

        vbox1 = gtk_vbox_new (FALSE, 0);

        /* --- */

        vbox2 = gtk_vbox_new (FALSE, 0);
        //gtk_container_set_border_width (GTK_CONTAINER (vbox2), 12);
        gtk_box_pack_start (GTK_BOX (vbox1), vbox2, TRUE, TRUE, 0);

        /* --- */

        shell->priv->sections_vbox = gtk_vbox_new (FALSE, 12);
        gtk_container_set_border_width (GTK_CONTAINER (shell->priv->sections_vbox), 6);
        gtk_box_pack_start (GTK_BOX (vbox2), shell->priv->sections_vbox, TRUE, TRUE, 0);

        /* setup and add horizontal pane */
        hpane = gtk_hpaned_new ();

        label = gtk_label_new (NULL);
        gtk_label_set_markup_with_mnemonic (GTK_LABEL (label),
                                            _("_Storage Devices"));
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_label_set_mnemonic_widget (GTK_LABEL (label), shell->priv->tree_view);

        column = gtk_tree_view_get_column (GTK_TREE_VIEW (shell->priv->tree_view), 0);
        gtk_tree_view_column_set_widget (column, label);
        gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (shell->priv->tree_view), TRUE);
        gtk_widget_show (label);

        gtk_paned_add1 (GTK_PANED (hpane), tree_view_scrolled_window);
        gtk_paned_add2 (GTK_PANED (hpane), vbox1);
        gtk_widget_set_size_request (shell->priv->tree_view, 260, -1),
        //gtk_paned_set_position (GTK_PANED (hpane), 260);

        gtk_box_pack_start (GTK_BOX (vbox), hpane, TRUE, TRUE, 0);

        select = gtk_tree_view_get_selection (GTK_TREE_VIEW (shell->priv->tree_view));
        gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);
        g_signal_connect (select, "changed", (GCallback) device_tree_changed, shell);

        /* when starting up, set focus on tree view */
        gtk_widget_grab_focus (shell->priv->tree_view);

        g_signal_connect (shell->priv->app_window, "delete-event", gtk_main_quit, NULL);

        gtk_widget_show_all (vbox);

        gdu_pool_tree_view_select_first_presentable (GDU_POOL_TREE_VIEW (shell->priv->tree_view));
}

