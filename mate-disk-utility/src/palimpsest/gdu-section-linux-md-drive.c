/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-section-linux-md-drive.c
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
#include <stdlib.h>
#include <math.h>

#include <gdu/gdu.h>
#include <gdu-gtk/gdu-gtk.h>

#include "gdu-section-drive.h"
#include "gdu-section-linux-md-drive.h"

struct _GduSectionLinuxMdDrivePrivate
{
        GduDetailsElement *level_element;
        GduDetailsElement *metadata_version_element;
        GduDetailsElement *name_element;
        GduDetailsElement *partitioning_element;
        GduDetailsElement *state_element;
        GduDetailsElement *capacity_element;
        GduDetailsElement *action_element;
        GduDetailsElement *components_element;

        GduButtonElement *md_start_button;
        GduButtonElement *md_stop_button;
        GduButtonElement *format_button;
        GduButtonElement *edit_components_button;
        GduButtonElement *check_button;
        GduButtonElement *benchmark_button;
};

G_DEFINE_TYPE (GduSectionLinuxMdDrive, gdu_section_linux_md_drive, GDU_TYPE_SECTION)

/* ---------------------------------------------------------------------------------------------------- */

static void
gdu_section_linux_md_drive_finalize (GObject *object)
{
        //GduSectionLinuxMdDrive *section = GDU_SECTION_LINUX_MD_DRIVE (object);

        if (G_OBJECT_CLASS (gdu_section_linux_md_drive_parent_class)->finalize != NULL)
                G_OBJECT_CLASS (gdu_section_linux_md_drive_parent_class)->finalize (object);
}

static void
update_state_and_action_elements (GduSectionLinuxMdDrive *section)
{
        GduLinuxMdDrive *drive;
        GduDevice *d;
        gchar *state_str;
        gchar *action_str;
        gdouble action_progress;

        drive = GDU_LINUX_MD_DRIVE (gdu_section_get_presentable (GDU_SECTION (section)));
        d = gdu_presentable_get_device (GDU_PRESENTABLE (drive));

        action_str = NULL;
        action_progress = -1.0;
        if (!gdu_drive_is_active (GDU_DRIVE (drive))) {
                if (d != NULL) {
                        state_str = g_strdup (C_("RAID status", "Not running, partially assembled"));
                } else {
                        gboolean can_activate;
                        gboolean degraded;

                        can_activate = gdu_drive_can_activate (GDU_DRIVE (drive), &degraded);

                        if (can_activate && !degraded) {
                                state_str = g_strdup (C_("RAID status", "Not running"));
                        } else if (can_activate && degraded) {
                                state_str = g_strdup (C_("RAID status", "Not running, can only start degraded"));
                        } else {
                                state_str = g_strdup (C_("RAID status", "Not running, not enough components to start"));
                        }
                }

                action_str = g_strdup ("–");
        } else {
                gboolean is_degraded;
                const gchar *sync_action;
                gdouble sync_percentage;
                guint64 sync_speed;

                is_degraded = gdu_device_linux_md_is_degraded (d);
                sync_action = gdu_device_linux_md_get_sync_action (d);
                sync_percentage = gdu_device_linux_md_get_sync_percentage (d);
                sync_speed = gdu_device_linux_md_get_sync_speed (d);

                if (is_degraded) {
                        state_str  = g_strdup_printf ("<span foreground='red'><b>%s</b></span>",
                                                      C_("RAID status", "DEGRADED"));
                } else {
                        state_str = g_strdup (C_("RAID status", "Running"));
                }

                if (strcmp (sync_action, "idle") != 0) {

                        /* TODO: include speed somewhere? */

                        if (strcmp (sync_action, "reshape") == 0) {
                                action_str = g_strdup_printf (C_("RAID action", "Reshaping"));
                        } else if (strcmp (sync_action, "resync") == 0) {
                                action_str = g_strdup_printf (C_("RAID action", "Resyncing"));
                        } else if (strcmp (sync_action, "repair") == 0) {
                                action_str = g_strdup_printf (C_("RAID action", "Repairing"));
                        } else if (strcmp (sync_action, "recover") == 0) {
                                action_str = g_strdup_printf (C_("RAID action", "Recovering"));
                        } else if (strcmp (sync_action, "check") == 0) {
                                action_str = g_strdup_printf (C_("RAID action", "Checking"));
                        }

                        action_progress = sync_percentage / 100.0;
                } else {
                        action_str = g_strdup (C_("RAID action", "Idle"));
                }
        }

        gdu_details_element_set_text (section->priv->state_element, state_str);
        gdu_details_element_set_text (section->priv->action_element, action_str);
        gdu_details_element_set_progress (section->priv->action_element, action_progress);

        g_free (state_str);
        g_free (action_str);

        if (d != NULL)
                g_object_unref (d);
}

#if 0
static gboolean
on_component_label_activate_link (GtkLabel    *label,
                                  const gchar *uri,
                                  gpointer     user_data)
{
        GduSectionLinuxMdDrive *section = GDU_SECTION_LINUX_MD_DRIVE (user_data);
        GduPool *pool;
        GduDevice *device;
        GduPresentable *volume;

        pool = NULL;
        device = NULL;
        volume = NULL;

        pool = gdu_presentable_get_pool (gdu_section_get_presentable (GDU_SECTION (section)));

        device = gdu_pool_get_by_object_path (pool, uri);
        if (device == NULL)
                goto out;

        volume = gdu_pool_get_volume_by_device (pool, device);
        if (volume == NULL)
                goto out;

        gdu_shell_select_presentable (gdu_section_get_shell (GDU_SECTION (section)), GDU_PRESENTABLE (volume));

 out:
        if (pool != NULL)
                g_object_unref (pool);
        if (volume != NULL)
                g_object_unref (volume);
        if (device != NULL)
                g_object_unref (device);
        return TRUE;
}
#endif

static void
gdu_section_linux_md_drive_update (GduSection *_section)
{
        GduSectionLinuxMdDrive *section = GDU_SECTION_LINUX_MD_DRIVE (_section);
        GduPresentable *p;
        GduDevice *d;
        gchar *s;
        gboolean show_md_start_button;
        gboolean show_md_stop_button;
        gboolean show_format_button;
        gboolean show_edit_components_button;
        gboolean show_check_button;
        gboolean show_benchmark_button;
        GList *slaves;
        GduDevice *slave;
        const gchar *level;
        const gchar *metadata_version;
        const gchar *name;
        const gchar *home_host;
        guint num_raid_devices;

        show_md_start_button = FALSE;
        show_md_stop_button = FALSE;
        show_format_button = FALSE;
        show_edit_components_button = FALSE;
        show_check_button = FALSE;
        show_benchmark_button = FALSE;

        p = gdu_section_get_presentable (_section);
        d = gdu_presentable_get_device (p);
        slaves = gdu_linux_md_drive_get_slaves (GDU_LINUX_MD_DRIVE (p));
        if (slaves == NULL)
                goto out;

        slave = GDU_DEVICE (slaves->data);

        level = gdu_device_linux_md_component_get_level (slave);
        metadata_version = gdu_device_linux_md_component_get_version (slave);
        name = gdu_device_linux_md_component_get_name (slave);
        home_host = gdu_device_linux_md_component_get_home_host (slave);
        num_raid_devices = (guint) gdu_device_linux_md_component_get_num_raid_devices (slave);

        s = gdu_linux_md_get_raid_level_for_display (level, TRUE);
        gdu_details_element_set_text (section->priv->level_element, s);
        g_free (s);
        gdu_details_element_set_text (section->priv->metadata_version_element, metadata_version);

        if (name != NULL && strlen (name) > 0)
                gdu_details_element_set_text (section->priv->name_element, name);
        else
                gdu_details_element_set_text (section->priv->name_element, "–");

        s = g_strdup_printf ("%d", num_raid_devices);
        gdu_details_element_set_text (section->priv->components_element, s);
        g_free (s);

        if (d != NULL) {
                if (gdu_device_is_partition_table (d)) {
                        const gchar *scheme;

                        scheme = gdu_device_partition_table_get_scheme (d);
                        if (g_strcmp0 (scheme, "apm") == 0) {
                                s = g_strdup (_("Apple Partition Map"));
                        } else if (g_strcmp0 (scheme, "mbr") == 0) {
                                s = g_strdup (_("Master Boot Record"));
                        } else if (g_strcmp0 (scheme, "gpt") == 0) {
                                s = g_strdup (_("GUID Partition Table"));
                        } else {
                                /* Translators: 'scheme' refers to a partition table format here, like 'mbr' or 'gpt' */
                                s = g_strdup_printf (_("Unknown Scheme: %s"), scheme);
                        }
                        gdu_details_element_set_text (section->priv->partitioning_element, s);
                        g_free (s);
                } else {
                        gdu_details_element_set_text (section->priv->partitioning_element,
                                                      _("Not Partitioned"));
                }

                s = gdu_util_get_size_for_display (gdu_device_get_size (d),
                                                   FALSE,
                                                   TRUE);
                gdu_details_element_set_text (section->priv->capacity_element, s);
                g_free (s);

                show_format_button = TRUE;
                show_check_button = TRUE;
                show_benchmark_button = TRUE;
                show_edit_components_button = TRUE;
                show_md_stop_button = TRUE;
        } else {
                gdu_details_element_set_text (section->priv->partitioning_element, "–");
                /* TODO: maybe try and compute the size when not running */
                gdu_details_element_set_text (section->priv->capacity_element, "–");
                show_md_start_button = TRUE;
        }

 out:
        update_state_and_action_elements (section);

        gdu_button_element_set_visible (section->priv->md_start_button, show_md_start_button);
        gdu_button_element_set_visible (section->priv->md_stop_button, show_md_stop_button);
        gdu_button_element_set_visible (section->priv->format_button, show_format_button);
        gdu_button_element_set_visible (section->priv->edit_components_button, show_edit_components_button);
        gdu_button_element_set_visible (section->priv->check_button, show_check_button);
        gdu_button_element_set_visible (section->priv->benchmark_button, show_benchmark_button);

        if (d != NULL)
                g_object_unref (d);
        g_list_foreach (slaves, (GFunc) g_object_unref, NULL);
        g_list_free (slaves);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
md_stop_op_callback (GduDevice *device,
                     GError    *error,
                     gpointer   user_data)
{
        GduShell *shell = GDU_SHELL (user_data);

        if (error != NULL) {
                GtkWidget *dialog;
                dialog = gdu_error_dialog_new_for_drive (GTK_WINDOW (gdu_shell_get_toplevel (shell)),
                                                         device,
                                                         _("Error stopping RAID Array"),
                                                         error);
                gtk_widget_show_all (dialog);
                gtk_window_present (GTK_WINDOW (dialog));
                gtk_dialog_run (GTK_DIALOG (dialog));
                gtk_widget_destroy (dialog);
                g_error_free (error);
        }
        g_object_unref (shell);
}

static void
on_md_stop_button_clicked (GduButtonElement *button_element,
                           gpointer          user_data)
{
        GduSectionLinuxMdDrive *section = GDU_SECTION_LINUX_MD_DRIVE (user_data);
        GduDevice *d;

        d = gdu_presentable_get_device (gdu_section_get_presentable (GDU_SECTION (section)));
        if (d == NULL)
                goto out;

        gdu_device_op_linux_md_stop (d,
                                     md_stop_op_callback,
                                     g_object_ref (gdu_section_get_shell (GDU_SECTION (section))));

        g_object_unref (d);
 out:
        ;
}

/* ---------------------------------------------------------------------------------------------------- */

static void
md_start_op_callback (GduDrive   *drive,
                      gchar      *assembled_drive_object_path,
                      GError     *error,
                      gpointer    user_data)
{
        GduShell *shell = GDU_SHELL (user_data);

        if (error != NULL) {
                GtkWidget *dialog;
                dialog = gdu_error_dialog_new (GTK_WINDOW (gdu_shell_get_toplevel (shell)),
                                               GDU_PRESENTABLE (drive),
                                               _("Error starting RAID Array"),
                                               error);
                gtk_widget_show_all (dialog);
                gtk_window_present (GTK_WINDOW (dialog));
                gtk_dialog_run (GTK_DIALOG (dialog));
                gtk_widget_destroy (dialog);
                g_error_free (error);
        } else {
                g_free (assembled_drive_object_path);
        }
        g_object_unref (shell);
}

static void
on_md_start_button_clicked (GduButtonElement *button_element,
                            gpointer          user_data)
{
        GduSectionLinuxMdDrive *section = GDU_SECTION_LINUX_MD_DRIVE (user_data);
        GduDrive *drive;
        gboolean degraded;
        GtkWindow *toplevel;

        drive = GDU_DRIVE (gdu_section_get_presentable (GDU_SECTION (section)));
        toplevel = GTK_WINDOW (gdu_shell_get_toplevel (gdu_section_get_shell (GDU_SECTION (section))));

        if (!gdu_drive_can_activate (drive, &degraded)) {
                GtkWidget *dialog;
                GError *error;
                error = g_error_new (GDU_ERROR,
                                     GDU_ERROR_FAILED,
                                     _("Not enough components available to start the RAID Array"));
                dialog = gdu_error_dialog_new (toplevel,
                                               GDU_PRESENTABLE (drive),
                                               _("Not enough components available to start the RAID Array"),
                                               error);
                gtk_widget_show_all (dialog);
                gtk_window_present (GTK_WINDOW (dialog));
                gtk_dialog_run (GTK_DIALOG (dialog));
                gtk_widget_destroy (dialog);
                g_error_free (error);
                goto out;
        }

        if (degraded) {
                GtkWidget *dialog;
                gint response;

                dialog = gdu_confirmation_dialog_new (toplevel,
                                                      GDU_PRESENTABLE (drive),
                                                      _("Are you sure you want the RAID Array degraded?"),
                                                      _("_Start"));
                gtk_widget_show_all (dialog);
                response = gtk_dialog_run (GTK_DIALOG (dialog));
                gtk_widget_hide (dialog);
                gtk_widget_destroy (dialog);
                if (response != GTK_RESPONSE_OK)
                        goto out;
        }

        gdu_drive_activate (drive,
                            md_start_op_callback,
                            g_object_ref (gdu_section_get_shell (GDU_SECTION (section))));

 out:
        ;
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct {
        GduShell *shell;
        GduLinuxMdDrive *array;
        GduDevice *slave;
} RemoveComponentData;

static void
remove_component_data_free (RemoveComponentData *data)
{
        g_object_unref (data->shell);
        g_object_unref (data->array);
        g_object_unref (data->slave);
        g_free (data);
}

static void
remove_component_delete_partition_op_callback (GduDevice  *device,
                                               GError     *error,
                                               gpointer    user_data)
{
        RemoveComponentData *data = user_data;

        if (error != NULL) {
                GtkWidget *dialog;
                dialog = gdu_error_dialog_new_for_drive (GTK_WINDOW (gdu_shell_get_toplevel (data->shell)),
                                                         device,
                                                         _("Error deleting partition for component in RAID Array"),
                                                         error);
                gtk_widget_show_all (dialog);
                gtk_window_present (GTK_WINDOW (dialog));
                gtk_dialog_run (GTK_DIALOG (dialog));
                gtk_widget_destroy (dialog);
                g_error_free (error);
        }

        remove_component_data_free (data);
}

static void
remove_component_op_callback (GduDevice  *device,
                              GError     *error,
                              gpointer    user_data)
{
        RemoveComponentData *data = user_data;

        if (error != NULL) {
                GtkWidget *dialog;
                dialog = gdu_error_dialog_new_for_drive (GTK_WINDOW (gdu_shell_get_toplevel (data->shell)),
                                                         device,
                                                         _("Error removing component from RAID Array"),
                                                         error);
                gtk_widget_show_all (dialog);
                gtk_window_present (GTK_WINDOW (dialog));
                gtk_dialog_run (GTK_DIALOG (dialog));
                gtk_widget_destroy (dialog);
                g_error_free (error);

                remove_component_data_free (data);
        } else {
                /* if the device is a partition, also remove the partition */
                if (gdu_device_is_partition (data->slave)) {
                        gdu_device_op_partition_delete (data->slave,
                                                        remove_component_delete_partition_op_callback,
                                                        data);
                } else {
                        remove_component_data_free (data);
                }
        }
}

static void
on_components_dialog_remove_button_clicked (GduEditLinuxMdDialog *_dialog,
                                            GduDevice            *slave_device,
                                            gpointer              user_data)
{
        GduSectionLinuxMdDrive *section = GDU_SECTION_LINUX_MD_DRIVE (user_data);
        GduLinuxMdDrive *linux_md_drive;
        GduDevice *device;
        GduLinuxMdDriveSlaveFlags slave_flags;
        GtkWindow *toplevel;
        GtkWidget *dialog;
        gint response;
        RemoveComponentData *data;

        device = NULL;

        toplevel = GTK_WINDOW (gdu_shell_get_toplevel (gdu_section_get_shell (GDU_SECTION (section))));

        linux_md_drive = GDU_LINUX_MD_DRIVE (gdu_section_get_presentable (GDU_SECTION (section)));
        device = gdu_presentable_get_device (GDU_PRESENTABLE (linux_md_drive));
        if (device == NULL)
                goto out;

        slave_flags = gdu_linux_md_drive_get_slave_flags (linux_md_drive, slave_device);
        if (slave_flags & GDU_LINUX_MD_DRIVE_SLAVE_FLAGS_NOT_ATTACHED)
                goto out;

        /* TODO: more details in this dialog - e.g. "The RAID array may degrade" etc etc */
        dialog = gdu_confirmation_dialog_new_for_volume (toplevel,
                                                         slave_device,
                                                         _("Are you sure you want the remove the component?"),
                                                         _("_Remove"));
        gtk_widget_show_all (dialog);
        response = gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_hide (dialog);
        gtk_widget_destroy (dialog);
        if (response != GTK_RESPONSE_OK)
                goto out;

        data = g_new0 (RemoveComponentData, 1);
        data->shell = g_object_ref (gdu_section_get_shell (GDU_SECTION (section)));
        data->array = g_object_ref (linux_md_drive);
        data->slave = g_object_ref (slave_device);

        gdu_device_op_linux_md_remove_component (device,
                                                 gdu_device_get_object_path (slave_device),
                                                 remove_component_op_callback,
                                                 data);

 out:
        if (device != NULL)
                g_object_unref (device);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
attach_component_op_callback (GduDevice  *device,
                              GError     *error,
                              gpointer    user_data)
{
        GduShell *shell = GDU_SHELL (user_data);

        if (error != NULL) {
                GtkWidget *dialog;
                dialog = gdu_error_dialog_new_for_drive (GTK_WINDOW (gdu_shell_get_toplevel (shell)),
                                                         device,
                                                         _("Error adding component to RAID Array"),
                                                         error);
                gtk_widget_show_all (dialog);
                gtk_window_present (GTK_WINDOW (dialog));
                gtk_dialog_run (GTK_DIALOG (dialog));
                gtk_widget_destroy (dialog);
                g_error_free (error);
        }
        g_object_unref (shell);
}

static void
on_components_dialog_attach_button_clicked (GduEditLinuxMdDialog *_dialog,
                                            GduDevice            *slave_device,
                                            gpointer              user_data)
{
        GduSectionLinuxMdDrive *section = GDU_SECTION_LINUX_MD_DRIVE (user_data);
        GduLinuxMdDrive *linux_md_drive;
        GduDevice *device;
        GduLinuxMdDriveSlaveFlags slave_flags;

        device = NULL;

        linux_md_drive = GDU_LINUX_MD_DRIVE (gdu_section_get_presentable (GDU_SECTION (section)));
        device = gdu_presentable_get_device (GDU_PRESENTABLE (linux_md_drive));
        if (device == NULL)
                goto out;

        slave_flags = gdu_linux_md_drive_get_slave_flags (linux_md_drive, slave_device);
        if (slave_flags & GDU_LINUX_MD_DRIVE_SLAVE_FLAGS_NOT_ATTACHED) {
                gdu_device_op_linux_md_add_spare (device,
                                                  gdu_device_get_object_path (slave_device),
                                                  attach_component_op_callback,
                                                  g_object_ref (gdu_section_get_shell (GDU_SECTION (section))));
        }

 out:
        if (device != NULL)
                g_object_unref (device);
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct {
        GduShell *shell;
        GduLinuxMdDrive *linux_md_drive;
        GduDevice *linux_md_drive_device;
        GPtrArray *drives_to_add_to;
        GPtrArray *created_components_object_paths;
        guint next_volume_number;
        gboolean failed;
        guint64 size;
        gboolean is_expansion;
} AddComponentData;

static void
add_component_data_free (AddComponentData *data)
{
        if (data->shell != NULL)
                g_object_unref (data->shell);
        if (data->linux_md_drive != NULL)
                g_object_unref (data->linux_md_drive);
        if (data->linux_md_drive_device != NULL)
                g_object_unref (data->linux_md_drive_device);
        if (data->drives_to_add_to != NULL)
                g_ptr_array_unref (data->drives_to_add_to);
        if (data->created_components_object_paths != NULL)
                g_ptr_array_unref (data->created_components_object_paths);
        g_free (data);
}

static void do_create_volumes (AddComponentData *data);

static void
add_component_cb (GduDevice  *device,
                  GError     *error,
                  gpointer    user_data)
{
        AddComponentData *data = user_data;

        if (error != NULL) {
                GtkWidget *dialog;
                dialog = gdu_error_dialog_new_for_drive (GTK_WINDOW (gdu_shell_get_toplevel (data->shell)),
                                                         device,
                                                         _("Error adding component to RAID Array"),
                                                         error);
                gtk_widget_show_all (dialog);
                gtk_window_present (GTK_WINDOW (dialog));
                gtk_dialog_run (GTK_DIALOG (dialog));
                gtk_widget_destroy (dialog);
                g_error_free (error);
                data->failed = TRUE;
        } else {
                /* Onwards to the next one */
                data->next_volume_number++;
        }

        do_create_volumes (data);
}

static void
new_component_create_volume_cb (GduDrive     *drive,
                                GAsyncResult *res,
                                gpointer      user_data)
{
        AddComponentData *data = user_data;
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
                data->failed = TRUE;
                do_create_volumes (data);
        } else {
                GduDevice *component_device;

                component_device = gdu_presentable_get_device (GDU_PRESENTABLE (volume));

                if (data->is_expansion) {
                        /* If expanding, just queue up object paths ... */
                        g_ptr_array_add (data->created_components_object_paths,
                                         g_strdup (gdu_device_get_object_path (component_device)));
                        /* ... and continue onwards to the next volume */
                        data->next_volume_number++;
                        do_create_volumes (data);
                } else {
                        gdu_device_op_linux_md_add_spare (data->linux_md_drive_device,
                                                          gdu_device_get_object_path (component_device),
                                                          add_component_cb,
                                                          data);
                }

                g_object_unref (component_device);
                g_object_unref (volume);
        }
}

static void
expand_md_cb (GduDevice  *device,
              GError     *error,
              gpointer    user_data)
{
        AddComponentData *data = user_data;

        if (error != NULL) {
                GtkWidget *dialog;
                dialog = gdu_error_dialog_new_for_drive (GTK_WINDOW (gdu_shell_get_toplevel (data->shell)),
                                                         device,
                                                         _("Error expanding RAID Array"),
                                                         error);
                gtk_widget_show_all (dialog);
                gtk_window_present (GTK_WINDOW (dialog));
                gtk_dialog_run (GTK_DIALOG (dialog));
                gtk_widget_destroy (dialog);
                g_error_free (error);
                data->failed = TRUE;
        }

        add_component_data_free (data);
}

static void
do_create_volumes (AddComponentData *data)
{
        if (data->failed) {
                /* Failed - already shown dialogs */
                add_component_data_free (data);
        } else if (data->next_volume_number == data->drives_to_add_to->len) {
                /* Done! */

                if (data->is_expansion) {
                        gdu_device_op_linux_md_expand (data->linux_md_drive_device,
                                                       data->created_components_object_paths,
                                                       expand_md_cb,
                                                       data);
                } else {
                        add_component_data_free (data);
                }
        } else {
                GduDrive *drive;
                drive = data->drives_to_add_to->pdata[data->next_volume_number];
                gdu_drive_create_volume (drive,
                                         data->size,
                                         gdu_device_linux_md_get_name (data->linux_md_drive_device),
                                         GDU_CREATE_VOLUME_FLAGS_LINUX_MD,
                                         (GAsyncReadyCallback) new_component_create_volume_cb,
                                         data);
        }
}

static void
generic_add_component (GduSectionLinuxMdDrive *section,
                       gboolean is_expansion)
{
        GduLinuxMdDrive *linux_md_drive;
        GduDevice *device;
        GtkWidget *dialog;
        gint response;
        GtkWindow *toplevel;
        AddComponentData *data;

        device = NULL;
        dialog = NULL;

        toplevel = GTK_WINDOW (gdu_shell_get_toplevel (gdu_section_get_shell (GDU_SECTION (section))));

        linux_md_drive = GDU_LINUX_MD_DRIVE (gdu_section_get_presentable (GDU_SECTION (section)));
        device = gdu_presentable_get_device (GDU_PRESENTABLE (linux_md_drive));
        if (device == NULL)
                goto out;

        dialog = gdu_add_component_linux_md_dialog_new (toplevel,
                                                        is_expansion ?
                                                          GDU_ADD_COMPONENT_LINUX_MD_FLAGS_EXPANSION :
                                                          GDU_ADD_COMPONENT_LINUX_MD_FLAGS_SPARE,
                                                        linux_md_drive);
        gtk_widget_show_all (dialog);
        response = gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_hide (dialog);
        if (response != GTK_RESPONSE_APPLY)
                goto out;

        data = g_new0 (AddComponentData, 1);
        data->shell = g_object_ref (gdu_section_get_shell (GDU_SECTION (section)));
        data->linux_md_drive = g_object_ref (linux_md_drive);
        data->linux_md_drive_device = g_object_ref (device);
        data->drives_to_add_to = gdu_add_component_linux_md_dialog_get_drives (GDU_ADD_COMPONENT_LINUX_MD_DIALOG (dialog));
        data->size = gdu_add_component_linux_md_dialog_get_size (GDU_ADD_COMPONENT_LINUX_MD_DIALOG (dialog));
        data->is_expansion = is_expansion;
        data->created_components_object_paths = g_ptr_array_new_with_free_func (g_free);

        do_create_volumes (data);

 out:
        if (dialog != NULL)
                gtk_widget_destroy (dialog);
        if (device != NULL)
                g_object_unref (device);
}

static void
on_components_dialog_expand_button_clicked (GduEditLinuxMdDialog *dialog,
                                            gpointer              user_data)
{
        GduSectionLinuxMdDrive *section = GDU_SECTION_LINUX_MD_DRIVE (user_data);
        generic_add_component (section, TRUE);
}

static void
on_components_dialog_add_spare_button_clicked (GduEditLinuxMdDialog *dialog,
                                               gpointer              user_data)
{
        GduSectionLinuxMdDrive *section = GDU_SECTION_LINUX_MD_DRIVE (user_data);
        generic_add_component (section, FALSE);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
on_edit_components_button_clicked (GduButtonElement *button_element,
                                   gpointer          user_data)
{
        GduSectionLinuxMdDrive *section = GDU_SECTION_LINUX_MD_DRIVE (user_data);
        GduPresentable *p;
        GtkWindow *toplevel;
        GtkWidget *dialog;

        p = gdu_section_get_presentable (GDU_SECTION (section));
        toplevel = GTK_WINDOW (gdu_shell_get_toplevel (gdu_section_get_shell (GDU_SECTION (section))));

        dialog = gdu_edit_linux_md_dialog_new (toplevel, GDU_LINUX_MD_DRIVE (p));

        g_signal_connect (dialog,
                          "add-spare-button-clicked",
                          G_CALLBACK (on_components_dialog_add_spare_button_clicked),
                          section);
        g_signal_connect (dialog,
                          "expand-button-clicked",
                          G_CALLBACK (on_components_dialog_expand_button_clicked),
                          section);
        g_signal_connect (dialog,
                          "attach-button-clicked",
                          G_CALLBACK (on_components_dialog_attach_button_clicked),
                          section);
        g_signal_connect (dialog,
                          "remove-button-clicked",
                          G_CALLBACK (on_components_dialog_remove_button_clicked),
                          section);

        gtk_widget_show_all (dialog);
        gtk_window_present (GTK_WINDOW (dialog));
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
md_check_op_callback (GduDevice *device,
                      guint      num_errors,
                      GError    *error,
                      gpointer   user_data)
{
        GduShell *shell = GDU_SHELL (user_data);

        if (error != NULL) {
                GtkWidget *dialog;
                dialog = gdu_error_dialog_new_for_drive (GTK_WINDOW (gdu_shell_get_toplevel (shell)),
                                                         device,
                                                         _("Error checking RAID Array"),
                                                         error);
                gtk_widget_show_all (dialog);
                gtk_window_present (GTK_WINDOW (dialog));
                gtk_dialog_run (GTK_DIALOG (dialog));
                gtk_widget_destroy (dialog);
                g_error_free (error);
        } else {
                /* TODO: report result back? */
        }
        g_object_unref (shell);
}

static void
on_check_button_clicked (GduButtonElement *button_element,
                         gpointer          user_data)
{
        GduSectionLinuxMdDrive *section = GDU_SECTION_LINUX_MD_DRIVE (user_data);
        const gchar *options[] = {"repair", NULL};
        GduDevice *d;

        d = gdu_presentable_get_device (gdu_section_get_presentable (GDU_SECTION (section)));
        if (d == NULL)
                goto out;

        gdu_device_op_linux_md_check (d,
                                      (gchar **) options,
                                      md_check_op_callback,
                                      g_object_ref (gdu_section_get_shell (GDU_SECTION (section))));

        g_object_unref (d);
 out:
        ;
}

/* ---------------------------------------------------------------------------------------------------- */

static void
gdu_section_linux_md_drive_constructed (GObject *object)
{
        GduSectionLinuxMdDrive *section = GDU_SECTION_LINUX_MD_DRIVE (object);
        GtkWidget *align;
        GtkWidget *label;
        GtkWidget *table;
        GtkWidget *vbox;
        gchar *s;
        GduPresentable *p;
        GduDevice *d;
        GPtrArray *elements;
        GduDetailsElement *element;
        GduButtonElement *button_element;

        p = gdu_section_get_presentable (GDU_SECTION (section));
        d = gdu_presentable_get_device (p);

        gtk_box_set_spacing (GTK_BOX (section), 12);

        /*------------------------------------- */

        label = gtk_label_new (NULL);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        s = g_strconcat ("<b>", _("RAID Array"), "</b>", NULL);
        gtk_label_set_markup (GTK_LABEL (label), s);
        g_free (s);
        gtk_box_pack_start (GTK_BOX (section), label, FALSE, FALSE, 0);

        align = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
        gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 0, 12, 0);
        gtk_box_pack_start (GTK_BOX (section), align, FALSE, FALSE, 0);

        vbox = gtk_vbox_new (FALSE, 6);
        gtk_container_add (GTK_CONTAINER (align), vbox);

        elements = g_ptr_array_new_with_free_func (g_object_unref);

        element = gdu_details_element_new (_("Level:"), NULL, NULL);
        g_ptr_array_add (elements, element);
        section->priv->level_element = element;

        element = gdu_details_element_new (_("Metadata Version:"), NULL, NULL);
        g_ptr_array_add (elements, element);
        section->priv->metadata_version_element = element;

        element = gdu_details_element_new (_("Name:"), NULL, NULL);
        g_ptr_array_add (elements, element);
        section->priv->name_element = element;

        element = gdu_details_element_new (_("Partitioning:"), NULL, NULL);
        g_ptr_array_add (elements, element);
        section->priv->partitioning_element = element;

        element = gdu_details_element_new (_("State:"), NULL, NULL);
        g_ptr_array_add (elements, element);
        section->priv->state_element = element;

        element = gdu_details_element_new (_("Capacity:"), NULL, NULL);
        g_ptr_array_add (elements, element);
        section->priv->capacity_element = element;

        element = gdu_details_element_new (_("Action:"), NULL, NULL);
        g_ptr_array_add (elements, element);
        section->priv->action_element = element;

        element = gdu_details_element_new (_("Components:"), NULL, NULL);
        g_ptr_array_add (elements, element);
        section->priv->components_element = element;

        table = gdu_details_table_new (2, elements);
        g_ptr_array_unref (elements);
        gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

        /* -------------------------------------------------------------------------------- */

        align = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
        gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 0, 12, 0);
        gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 0);

        elements = g_ptr_array_new_with_free_func (g_object_unref);

        button_element = gdu_button_element_new ("gdu-raid-array-start",
                                                 _("St_art RAID Array"),
                                                 _("Bring up the RAID Array"));
        g_signal_connect (button_element,
                          "clicked",
                          G_CALLBACK (on_md_start_button_clicked),
                          section);
        g_ptr_array_add (elements, button_element);
        section->priv->md_start_button = button_element;

        button_element = gdu_button_element_new ("gdu-raid-array-stop",
                                                 _("St_op RAID Array"),
                                                 _("Tear down the RAID Array"));
        g_signal_connect (button_element,
                          "clicked",
                          G_CALLBACK (on_md_stop_button_clicked),
                          section);
        g_ptr_array_add (elements, button_element);
        section->priv->md_stop_button = button_element;

        button_element = gdu_button_element_new ("caja-gdu",
                                                 _("Format/Erase RAI_D Array"),
                                                 _("Erase or partition the array"));
        g_signal_connect (button_element,
                          "clicked",
                          G_CALLBACK (gdu_section_drive_on_format_button_clicked),
                          section);
        g_ptr_array_add (elements, button_element);
        section->priv->format_button = button_element;

        button_element = gdu_button_element_new ("gdu-check-disk",
                                                 _("Chec_k Array"),
                                                 _("Check and repair the array"));
        g_signal_connect (button_element,
                          "clicked",
                          G_CALLBACK (on_check_button_clicked),
                          section);
        g_ptr_array_add (elements, button_element);
        section->priv->check_button = button_element;

        button_element = gdu_button_element_new (GTK_STOCK_EDIT,
                                                 _("Edit Com_ponents"),
                                                 _("Create and remove components"));
        g_signal_connect (button_element,
                          "clicked",
                          G_CALLBACK (on_edit_components_button_clicked),
                          section);
        g_ptr_array_add (elements, button_element);
        section->priv->edit_components_button = button_element;

        button_element = gdu_button_element_new ("gtk-execute", /* TODO: better icon */
                                                 _("_Benchmark"),
                                                 _("Measure RAID array performance"));
        g_signal_connect (button_element,
                          "clicked",
                          G_CALLBACK (gdu_section_drive_on_benchmark_button_clicked),
                          section);
        g_ptr_array_add (elements, button_element);
        section->priv->benchmark_button = button_element;

        table = gdu_button_table_new (2, elements);
        g_ptr_array_unref (elements);
        gtk_container_add (GTK_CONTAINER (align), table);

        /* -------------------------------------------------------------------------------- */

        gtk_widget_show_all (GTK_WIDGET (section));

        if (d != NULL)
                g_object_unref (d);

        if (G_OBJECT_CLASS (gdu_section_linux_md_drive_parent_class)->constructed != NULL)
                G_OBJECT_CLASS (gdu_section_linux_md_drive_parent_class)->constructed (object);
}

static void
gdu_section_linux_md_drive_class_init (GduSectionLinuxMdDriveClass *klass)
{
        GObjectClass *gobject_class;
        GduSectionClass *section_class;

        gobject_class = G_OBJECT_CLASS (klass);
        section_class = GDU_SECTION_CLASS (klass);

        gobject_class->finalize    = gdu_section_linux_md_drive_finalize;
        gobject_class->constructed = gdu_section_linux_md_drive_constructed;
        section_class->update      = gdu_section_linux_md_drive_update;

        g_type_class_add_private (klass, sizeof (GduSectionLinuxMdDrivePrivate));
}

static void
gdu_section_linux_md_drive_init (GduSectionLinuxMdDrive *section)
{
        section->priv = G_TYPE_INSTANCE_GET_PRIVATE (section, GDU_TYPE_SECTION_LINUX_MD_DRIVE, GduSectionLinuxMdDrivePrivate);
}

GtkWidget *
gdu_section_linux_md_drive_new (GduShell       *shell,
                       GduPresentable *presentable)
{
        return GTK_WIDGET (g_object_new (GDU_TYPE_SECTION_LINUX_MD_DRIVE,
                                         "shell", shell,
                                         "presentable", presentable,
                                         NULL));
}
