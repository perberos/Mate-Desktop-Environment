/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-section-drive.c
 *
 * Copyright (C) 2009 David Zeuthen
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
#include <gio/gdesktopappinfo.h>

#include <gdu-gtk/gdu-gtk.h>
#include "gdu-section-drive.h"

struct _GduSectionDrivePrivate
{
        GtkWidget *multipath_component_warning_info_bar;
        GtkWidget *multipath_component_info_bar;

        GtkWidget *drive_label;
        GtkWidget *main_vbox;

        GduDetailsElement *model_element;
        GduDetailsElement *firmware_element;
        GduDetailsElement *serial_element;
        GduDetailsElement *wwn_element;
        GduDetailsElement *location_element;
        GduDetailsElement *device_element;
        GduDetailsElement *write_cache_element;
        GduDetailsElement *rotation_rate_element;
        GduDetailsElement *capacity_element;
        GduDetailsElement *connection_element;
        GduDetailsElement *partitioning_element;
        GduDetailsElement *smart_element;

        GduButtonElement *cddvd_button;
        GduButtonElement *format_button;
        GduButtonElement *eject_button;
        GduButtonElement *detach_button;
        GduButtonElement *smart_button;
        GduButtonElement *benchmark_button;
};

G_DEFINE_TYPE (GduSectionDrive, gdu_section_drive, GDU_TYPE_SECTION)

/* ---------------------------------------------------------------------------------------------------- */

static gboolean
has_strv0 (gchar **strv, const gchar *str)
{
        gboolean ret;
        guint n;

        ret = FALSE;

        for (n = 0; strv != NULL && strv[n] != NULL; n++) {
                if (g_strcmp0 (strv[n], str) == 0) {
                        ret = TRUE;
                        goto out;
                }
        }

 out:
        return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

static void
gdu_section_drive_finalize (GObject *object)
{
        //GduSectionDrive *section = GDU_SECTION_DRIVE (object);

        if (G_OBJECT_CLASS (gdu_section_drive_parent_class)->finalize != NULL)
                G_OBJECT_CLASS (gdu_section_drive_parent_class)->finalize (object);
}

static void
gdu_section_drive_update (GduSection *_section)
{
        GduSectionDrive *section = GDU_SECTION_DRIVE (_section);
        GduPresentable *p;
        GduDevice *d;
        gchar *s;
        gchar *s2;
        const gchar *vendor;
        const gchar *model;
        const gchar *firmware;
        const gchar *serial;
        const gchar *wwn;
        const gchar *write_cache;
        guint rotation_rate;
        gboolean is_rotational;
        GIcon *icon;
        gchar **port_object_paths;
        GduPort *port;
        gboolean show_cddvd_button;
        gboolean show_format_button;
        gboolean show_eject_button;
        gboolean show_detach_button;
        gboolean show_smart_button;
        gboolean show_benchmark_button;
        const gchar *device_file;
        gchar **similar_devices;
        gboolean show_multipath_component_warning_info_bar;
        gboolean show_multipath_component_info_bar;
        gboolean make_insensitive;

        show_cddvd_button = FALSE;
        show_format_button = FALSE;
        show_eject_button = FALSE;
        show_detach_button = FALSE;
        show_smart_button = FALSE;
        show_benchmark_button = FALSE;
        show_multipath_component_warning_info_bar = FALSE;
        show_multipath_component_info_bar = FALSE;
        make_insensitive = FALSE;

        d = NULL;
        port = NULL;
        wwn = NULL;
        p = gdu_section_get_presentable (_section);

        d = gdu_presentable_get_device (p);
        if (d == NULL)
                goto out;

        if (gdu_device_is_linux_dmmp (d)) {
                s = g_strconcat ("<b>", _("Multipath Drive"), "</b>", NULL);
        } else {
                s = g_strconcat ("<b>", _("Drive"), "</b>", NULL);
        }
        gtk_label_set_markup (GTK_LABEL (section->priv->drive_label), s);
        g_free (s);

        /* TODO: handle multiple ports */
        port_object_paths = gdu_device_drive_get_ports (d);
        if (port_object_paths != NULL && port_object_paths[0] != NULL) {
                GduPool *pool;
                pool = gdu_device_get_pool (d);
                port = gdu_pool_get_port_by_object_path (pool, port_object_paths[0]);
                g_object_unref (pool);
        }
        if (port != NULL) {
                GduPool *pool;
                GduPresentable *hub;

                s = NULL;

                pool = gdu_device_get_pool (d);
                hub = gdu_pool_get_hub_by_object_path (pool, gdu_port_get_parent (port));
                if (hub != NULL) {
                        gchar *hub_name;
                        GduAdapter *adapter;
                        gint port_number;
                        const gchar *fabric;

                        adapter = gdu_hub_get_adapter (GDU_HUB (hub));
                        fabric = gdu_adapter_get_fabric (adapter);
                        hub_name = gdu_presentable_get_name (hub);
                        port_number = gdu_port_get_number (port);

                        if (port_number >= 0) {
                                if (g_strcmp0 (fabric, "scsi_sas") == 0) {
                                        /* Translators: This is used in the "Location" element for a disk
                                         * connected to a SAS Adapter or Expander - The %d is the
                                         * PHY number (starts at 1) and the %s is the Adapter/Expander
                                         * name (e.g. "SAS Host Adapter").
                                         */
                                        s = g_strdup_printf (_("PHY %d of %s"), port_number + 1, hub_name);
                                } else {
                                        /* Translators: This is used in the "Location" element for a disk
                                         * connected to a Adapter or Expander - The %d is the
                                         * PHY number (starts at 1) and the %s is the Adapter/Expander
                                         * name (e.g. "SATA Host Adapter").
                                         */
                                        s = g_strdup_printf (_("Port %d of %s"), port_number + 1, hub_name);
                                }
                        }

                        g_free (hub_name);
                        g_object_unref (adapter);
                        g_object_unref (hub);
                }
                if (s == NULL)
                        s = g_strdup ("–");
                gdu_details_element_set_text (section->priv->location_element, s);
                g_free (s);

                g_object_unref (pool);
        } else {
                gdu_details_element_set_text (section->priv->location_element, "–");
        }
        device_file = gdu_device_get_device_file_presentation (d);
        if (device_file == NULL || strlen (device_file) == 0)
                device_file = gdu_device_get_device_file (d);
        gdu_details_element_set_text (section->priv->device_element, device_file);

        model = gdu_device_drive_get_model (d);
        vendor = gdu_device_drive_get_vendor (d);
        if (vendor != NULL && strlen (vendor) == 0)
                vendor = NULL;
        if (model != NULL && strlen (model) == 0)
                model = NULL;
        s = g_strdup_printf ("%s%s%s",
                             vendor != NULL ? vendor : "",
                             vendor != NULL ? " " : "",
                             model != NULL ? model : "");
        if (strlen (s) == 0) {
                gdu_details_element_set_text (section->priv->model_element, "–");
        } else {
                gdu_details_element_set_text (section->priv->model_element, s);
        }
        g_free (s);

        firmware = gdu_device_drive_get_revision (d);
        if (firmware == NULL || strlen (firmware) == 0)
                firmware = "–";
        gdu_details_element_set_text (section->priv->firmware_element, firmware);

        serial = gdu_device_drive_get_serial (d);
        if (serial == NULL || strlen (serial) == 0)
                serial = "–";
        gdu_details_element_set_text (section->priv->serial_element, serial);

        wwn = gdu_device_drive_get_wwn (d);
        if (wwn == NULL || strlen (wwn) == 0) {
                gdu_details_element_set_text (section->priv->wwn_element, "–");
        } else {
                s = g_strdup_printf ("0x%s", wwn);
                gdu_details_element_set_text (section->priv->wwn_element, s);
                g_free (s);
        }

        write_cache = gdu_device_drive_get_write_cache (d);
        is_rotational = gdu_device_drive_get_is_rotational (d);
        rotation_rate = gdu_device_drive_get_rotation_rate (d);

        if (write_cache == NULL || strlen (write_cache) == 0) {
                gdu_details_element_set_text (section->priv->write_cache_element, "–");
        } else if (g_strcmp0 (write_cache, "enabled") == 0) {
                gdu_details_element_set_text (section->priv->write_cache_element, C_("Write Cache", "Enabled"));
        } else if (g_strcmp0 (write_cache, "disabled") == 0) {
                gdu_details_element_set_text (section->priv->write_cache_element, C_("Write Cache", "Disabled"));
        } else {
                gdu_details_element_set_text (section->priv->write_cache_element, write_cache);
        }

        if (is_rotational) {
                if (rotation_rate > 0) {
                        /* Translators: This is for the rotation rate of a hard
                         * disk - RPM means rounds per minute
                         */
                        s = g_strdup_printf (C_("Rotation Rate", "%d RPM"), rotation_rate);
                        gdu_details_element_set_text (section->priv->rotation_rate_element, s);
                        g_free (s);
                } else {
                        gdu_details_element_set_text (section->priv->rotation_rate_element, "–");
                }
        } else {
                gdu_details_element_set_text (section->priv->rotation_rate_element,
                                              C_("Rotation Rate", "Solid-State Disk"));
        }

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
                if (gdu_device_is_media_available (d)) {
                        gdu_details_element_set_text (section->priv->partitioning_element, _("Not Partitioned"));
                } else {
                        gdu_details_element_set_text (section->priv->partitioning_element, "–");
                }
        }

        if (gdu_device_drive_ata_smart_get_is_available (d) &&
            gdu_device_drive_ata_smart_get_time_collected (d) > 0) {
                gboolean highlight;

                s = gdu_util_ata_smart_status_to_desc (gdu_device_drive_ata_smart_get_status (d),
                                                       &highlight,
                                                       NULL,
                                                       &icon);
                if (highlight) {
                        s2 = g_strdup_printf ("<span fgcolor=\"red\"><b>%s</b></span>", s);
                        g_free (s);
                        s = s2;
                }

                gdu_details_element_set_text (section->priv->smart_element, s);
                gdu_details_element_set_icon (section->priv->smart_element, icon);

                g_free (s);
                g_object_unref (icon);
        } else {
                /* Translators: This string is used when SMART is not supported */
                gdu_details_element_set_text (section->priv->smart_element, _("Not Supported"));
                icon = g_themed_icon_new ("gdu-smart-unknown");
                gdu_details_element_set_icon (section->priv->smart_element, icon);
                g_object_unref (icon);
        }

        if (gdu_device_is_media_available (d)) {
                s = gdu_util_get_size_for_display (gdu_device_get_size (d),
                                                   FALSE,
                                                   TRUE);
                gdu_details_element_set_text (section->priv->capacity_element, s);
                g_free (s);
                show_benchmark_button = TRUE;
        } else {
                gdu_details_element_set_text (section->priv->capacity_element, _("No Media Detected"));
        }

        s = gdu_util_get_connection_for_display (gdu_device_drive_get_connection_interface (d),
                                                 gdu_device_drive_get_connection_speed (d));
        gdu_details_element_set_text (section->priv->connection_element, s);
        g_free (s);

        /* buttons */
        if (d != NULL &&
            gdu_device_drive_ata_smart_get_is_available (d) &&
            gdu_device_drive_ata_smart_get_time_collected (d) > 0) {
                show_smart_button = TRUE;
        }

        if (d != NULL && gdu_device_drive_get_is_media_ejectable (d)) {
                show_eject_button =  TRUE;
        }

        if (d != NULL && gdu_device_drive_get_can_detach (d)) {
                show_detach_button = TRUE;
        }

        if (d != NULL && has_strv0 (gdu_device_drive_get_media_compatibility (d), "optical_cd")) {
                show_cddvd_button = TRUE;
        } else {
                show_format_button = TRUE;
        }

        /* Show a warning cluebar if multipath isn't configured */
        similar_devices = gdu_device_drive_get_similar_devices (d);
        if (similar_devices != NULL && g_strv_length (similar_devices) > 0) {

                if (!gdu_device_is_linux_dmmp (d) && !gdu_device_is_linux_dmmp_component (d)) {
                        show_multipath_component_warning_info_bar = TRUE;
                }
        }

        /* Show an informational cluebar if multipath is configured and the selected object
         * is just a single path. Also make the whole section insensitive.
         */
        if (gdu_device_is_linux_dmmp_component (d)) {
                show_multipath_component_info_bar = TRUE;
                make_insensitive = TRUE;
        }

 out:
        gdu_button_element_set_visible (section->priv->cddvd_button, show_cddvd_button);
        gdu_button_element_set_visible (section->priv->format_button, show_format_button);
        gdu_button_element_set_visible (section->priv->eject_button, show_eject_button);
        gdu_button_element_set_visible (section->priv->detach_button, show_detach_button);
        gdu_button_element_set_visible (section->priv->smart_button, show_smart_button);
        gdu_button_element_set_visible (section->priv->benchmark_button, show_benchmark_button);

        if (show_multipath_component_warning_info_bar) {
                gtk_widget_show_all (section->priv->multipath_component_warning_info_bar);
        } else {
                gtk_widget_hide_all (section->priv->multipath_component_warning_info_bar);
        }

        if (show_multipath_component_info_bar) {
                gtk_widget_show_all (section->priv->multipath_component_info_bar);
        } else {
                gtk_widget_hide_all (section->priv->multipath_component_info_bar);
        }

        gtk_widget_set_sensitive (section->priv->drive_label, !make_insensitive);
        gtk_widget_set_sensitive (section->priv->main_vbox, !make_insensitive);

        if (d != NULL)
                g_object_unref (d);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
on_cddvd_button_clicked (GduButtonElement *button_element,
                         gpointer          user_data)
{
        GduSectionDrive *section = GDU_SECTION_DRIVE (user_data);
        GAppLaunchContext *launch_context;
        GAppInfo *app_info;
        GtkWidget *dialog;
        GError *error;

        app_info = NULL;
        launch_context = NULL;

        app_info = G_APP_INFO (g_desktop_app_info_new ("brasero.desktop"));
        if (app_info == NULL) {
                /* TODO: Use PackageKit to install Brasero */
                dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (gdu_shell_get_toplevel (gdu_section_get_shell (GDU_SECTION (section)))),
                                                             GTK_DIALOG_MODAL,
                                                             GTK_MESSAGE_ERROR,
                                                             GTK_BUTTONS_OK,
                                                             "<b><big><big>%s</big></big></b>\n\n%s",
                                                             _("Error launching Brasero"),
                                                             _("The application is not installed"));
                gtk_widget_show_all (dialog);
                gtk_dialog_run (GTK_DIALOG (dialog));
                gtk_widget_destroy (dialog);
                goto out;
        }

        launch_context = G_APP_LAUNCH_CONTEXT (gdk_app_launch_context_new ());

        error = NULL;
        if (!g_app_info_launch (app_info,
                                NULL, /* no files */
                                launch_context,
                                &error)) {
                dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (gdu_shell_get_toplevel (gdu_section_get_shell (GDU_SECTION (section)))),
                                                             GTK_DIALOG_MODAL,
                                                             GTK_MESSAGE_ERROR,
                                                             GTK_BUTTONS_OK,
                                                             "<b><big><big>%s</big></big></b>\n\n%s",
                                                             _("Error launching Brasero"),
                                                             error->message);
                g_error_free (error);
                gtk_widget_show_all (dialog);
                gtk_dialog_run (GTK_DIALOG (dialog));
                gtk_widget_destroy (dialog);
        }

 out:
        if (app_info != NULL)
                g_object_unref (app_info);
        if (launch_context != NULL)
                g_object_unref (launch_context);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
on_smart_button_clicked (GduButtonElement *button_element,
                         gpointer          user_data)
{
        GduSectionDrive *section = GDU_SECTION_DRIVE (user_data);
        GtkWindow *toplevel;
        GtkWidget *dialog;

        toplevel = GTK_WINDOW (gdu_shell_get_toplevel (gdu_section_get_shell (GDU_SECTION (section))));
        dialog = gdu_ata_smart_dialog_new (toplevel,
                                           GDU_DRIVE (gdu_section_get_presentable (GDU_SECTION (section))));
        gtk_widget_show_all (dialog);
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
}

void
gdu_section_drive_on_benchmark_button_clicked (GduButtonElement *button_element,
                                               gpointer          user_data)
{
        GduSection *section = GDU_SECTION (user_data);
        GtkWindow *toplevel;
        GtkWidget *dialog;

        toplevel = GTK_WINDOW (gdu_shell_get_toplevel (gdu_section_get_shell (GDU_SECTION (section))));
        dialog = gdu_drive_benchmark_dialog_new (toplevel,
                                                 GDU_DRIVE (gdu_section_get_presentable (GDU_SECTION (section))));
        gtk_widget_show_all (dialog);
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
eject_op_callback (GduDevice *device,
                   GError    *error,
                   gpointer   user_data)
{
        GduShell *shell = GDU_SHELL (user_data);

        if (error != NULL) {
                GtkWidget *dialog;
                dialog = gdu_error_dialog_new_for_drive (GTK_WINDOW (gdu_shell_get_toplevel (shell)),
                                                         device,
                                                         _("Error ejecting medium"),
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
on_eject_button_clicked (GduButtonElement *button_element,
                         gpointer          user_data)
{
        GduSectionDrive *section = GDU_SECTION_DRIVE (user_data);
        GduDevice *d;

        d = gdu_presentable_get_device (gdu_section_get_presentable (GDU_SECTION (section)));
        if (d == NULL)
                goto out;

        gdu_device_op_drive_eject (d,
                                   eject_op_callback,
                                   g_object_ref (gdu_section_get_shell (GDU_SECTION (section))));

        g_object_unref (d);
 out:
        ;
}

/* ---------------------------------------------------------------------------------------------------- */

static void
detach_op_callback (GduDevice *device,
                    GError    *error,
                    gpointer   user_data)
{
        GduShell *shell = GDU_SHELL (user_data);

        if (error != NULL) {
                GtkWidget *dialog;
                dialog = gdu_error_dialog_new_for_drive (GTK_WINDOW (gdu_shell_get_toplevel (shell)),
                                                         device,
                                                         _("Error detaching drive"),
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
on_detach_button_clicked (GduButtonElement *button_element,
                          gpointer          user_data)
{
        GduSectionDrive *section = GDU_SECTION_DRIVE (user_data);
        GduDevice *d;

        d = gdu_presentable_get_device (gdu_section_get_presentable (GDU_SECTION (section)));
        if (d == NULL)
                goto out;

        gdu_device_op_drive_detach (d,
                                    detach_op_callback,
                                    g_object_ref (gdu_section_get_shell (GDU_SECTION (section))));

        g_object_unref (d);
 out:
        ;
}

/* ---------------------------------------------------------------------------------------------------- */

static void
partition_table_create_op_callback (GduDevice  *device,
                                    GError     *error,
                                    gpointer    user_data)
{
        GduShell *shell = GDU_SHELL (user_data);

        if (error != NULL) {
                GtkWidget *dialog;

                dialog = gdu_error_dialog_new_for_drive (GTK_WINDOW (gdu_shell_get_toplevel (shell)),
                                                         device,
                                                         _("Error formatting drive"),
                                                         error);
                gtk_widget_show_all (dialog);
                gtk_window_present (GTK_WINDOW (dialog));
                gtk_dialog_run (GTK_DIALOG (dialog));
                gtk_widget_destroy (dialog);
                g_error_free (error);
        }
        g_object_unref (shell);
}

/* exported, can only assume user_data is a GduSection */
void
gdu_section_drive_on_format_button_clicked (GduButtonElement *button_element,
                                            gpointer          user_data)
{
        GduSection *section = GDU_SECTION (user_data);
        GduDevice *d;
        GtkWindow *toplevel;
        GtkWidget *dialog;
        GtkWidget *confirmation_dialog;
        gint response;

        dialog = NULL;
        confirmation_dialog = NULL;

        d = gdu_presentable_get_device (gdu_section_get_presentable (section));
        if (d == NULL)
                goto out;

        toplevel = GTK_WINDOW (gdu_shell_get_toplevel (gdu_section_get_shell (section)));
        dialog = gdu_partition_dialog_new_for_drive (toplevel, d);
        gtk_widget_show_all (dialog);
        response = gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_hide (dialog);
        if (response == GTK_RESPONSE_OK) {
                confirmation_dialog = gdu_confirmation_dialog_new_for_drive (toplevel,
                                                                             d,
                                                                             _("Are you sure you want to format the drive?"),
                                                                             _("_Format"));
                gtk_widget_show_all (confirmation_dialog);
                response = gtk_dialog_run (GTK_DIALOG (confirmation_dialog));
                gtk_widget_hide (confirmation_dialog);
                if (response == GTK_RESPONSE_OK) {
                        gdu_device_op_partition_table_create (d,
                                                    gdu_partition_dialog_get_scheme (GDU_PARTITION_DIALOG (dialog)),
                                                    partition_table_create_op_callback,
                                                    g_object_ref (gdu_section_get_shell (section)));
                }
        }
 out:
        if (dialog != NULL)
                gtk_widget_destroy (dialog);
        if (confirmation_dialog != NULL)
                gtk_widget_destroy (confirmation_dialog);
        if (d != NULL)
                g_object_unref (d);
}

/* ---------------------------------------------------------------------------------------------------- */

#if 0
static gint
pstrcmp0 (gconstpointer a, gconstpointer b)
{
        return g_strcmp0 (*((gchar **) a), *((gchar **) b));
}

static void
on_multipath_warning_info_bar_next_path (GtkInfoBar *info_bar,
                                         gint        response_id,
                                         gpointer    user_data)
{
        GduSectionDrive *section = GDU_SECTION_DRIVE (user_data);
        GPtrArray *p;
        guint n;
        gchar **similar_devices;
        GduDevice *d;
        const gchar *object_path;
        const gchar *next_object_path;
        GduPool *pool;
        GduDevice *device_for_next_object_path;
        GduPresentable *drive_for_next_object_path;

        p = NULL;
        pool = NULL;
        device_for_next_object_path = NULL;
        drive_for_next_object_path = NULL;

        d = gdu_presentable_get_device (gdu_section_get_presentable (GDU_SECTION (section)));
        if (d == NULL) {
                goto out;
        }

        object_path = gdu_device_get_object_path (d);

        similar_devices = gdu_device_drive_get_similar_devices (d);
        p = g_ptr_array_new ();
        for (n = 0; similar_devices != NULL && similar_devices[n] != NULL; n++) {
                g_ptr_array_add (p, similar_devices[n]);
        }
        g_ptr_array_add (p, (gpointer) object_path);
        g_ptr_array_sort (p, pstrcmp0);

        for (n = 0; n < p->len; n++) {
                if (g_strcmp0 ((const gchar *) p->pdata[n], object_path) == 0) {
                        break;
                }
        }
        g_assert (n < p->len);

        next_object_path = (const gchar *) p->pdata[(n + 1) % p->len];

        pool = gdu_device_get_pool (d);

        device_for_next_object_path = gdu_pool_get_by_object_path (pool, next_object_path);
        if (device_for_next_object_path == NULL) {
                g_warning ("No device for object path %s", next_object_path);
                goto out;
        }

        drive_for_next_object_path = gdu_pool_get_drive_by_device (pool, device_for_next_object_path);
        if (drive_for_next_object_path == NULL) {
                g_warning ("No drive for object path %s", next_object_path);
                goto out;
        }

        gdu_shell_select_presentable (gdu_section_get_shell (GDU_SECTION (section)),
                                      drive_for_next_object_path);

 out:
        if (device_for_next_object_path != NULL)
                g_object_unref (device_for_next_object_path);
        if (drive_for_next_object_path != NULL)
                g_object_unref (drive_for_next_object_path);
        if (pool != NULL)
                g_object_unref (pool);
        if (p != NULL)
                g_ptr_array_free (p, TRUE);
}
#endif

/* ---------------------------------------------------------------------------------------------------- */

static void
on_multipath_component_info_bar_go_to_multipath_device (GtkInfoBar *info_bar,
                                                        gint        response_id,
                                                        gpointer    user_data)
{
        GduSectionDrive *section = GDU_SECTION_DRIVE (user_data);
        GduDevice *d;
        const gchar *multipath_object_path;
        GduPool *pool;
        GduDevice *device_for_multipath;
        GduPresentable *drive_for_multipath;

        pool = NULL;
        device_for_multipath = NULL;
        drive_for_multipath = NULL;

        d = gdu_presentable_get_device (gdu_section_get_presentable (GDU_SECTION (section)));
        if (d == NULL) {
                goto out;
        }

        multipath_object_path = gdu_device_linux_dmmp_component_get_holder (d);

        pool = gdu_device_get_pool (d);
        device_for_multipath = gdu_pool_get_by_object_path (pool, multipath_object_path);
        if (device_for_multipath == NULL) {
                g_warning ("No device for object path %s", multipath_object_path);
                goto out;
        }

        drive_for_multipath = gdu_pool_get_drive_by_device (pool, device_for_multipath);
        if (drive_for_multipath == NULL) {
                g_warning ("No drive for object path %s", multipath_object_path);
                goto out;
        }

        gdu_shell_select_presentable (gdu_section_get_shell (GDU_SECTION (section)),
                                      drive_for_multipath);

 out:
        if (device_for_multipath != NULL)
                g_object_unref (device_for_multipath);
        if (drive_for_multipath != NULL)
                g_object_unref (drive_for_multipath);
        if (pool != NULL)
                g_object_unref (pool);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
gdu_section_drive_constructed (GObject *object)
{
        GduSectionDrive *section = GDU_SECTION_DRIVE (object);
        GtkWidget *align;
        GtkWidget *label;
        GtkWidget *table;
        GtkWidget *vbox;
        GtkWidget *hbox;
        GtkWidget *image;
        GduPresentable *p;
        GduDevice *d;
        GPtrArray *elements;
        GduDetailsElement *element;
        GduButtonElement *button_element;
        GtkWidget *info_bar;

        p = gdu_section_get_presentable (GDU_SECTION (section));
        d = gdu_presentable_get_device (p);

        gtk_box_set_spacing (GTK_BOX (section), 12);

        /* -------------------------------------------------------------------------------- */

        info_bar = gtk_info_bar_new ();
        gtk_info_bar_set_message_type (GTK_INFO_BAR (info_bar), GTK_MESSAGE_WARNING);
        hbox = gtk_hbox_new (FALSE, 6);
        image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);
        gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
        label = gtk_label_new (NULL);
        gtk_label_set_markup (GTK_LABEL (label),
                              _("<b>WARNING:</b> Several paths to this drive has been detected but no "
                                "corresponding multipath device was found. Your OS may be misconfigured."));
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
        gtk_label_set_width_chars (GTK_LABEL (label), 70);
        gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
        gtk_container_add (GTK_CONTAINER (gtk_info_bar_get_content_area (GTK_INFO_BAR (info_bar))), hbox);
        gtk_box_pack_start (GTK_BOX (section), info_bar, FALSE, FALSE, 0);
        section->priv->multipath_component_warning_info_bar = info_bar;

        /*------------------------------------- */

        info_bar = gtk_info_bar_new ();
        gtk_info_bar_set_message_type (GTK_INFO_BAR (info_bar), GTK_MESSAGE_INFO);
        gtk_info_bar_add_button (GTK_INFO_BAR (info_bar),
                                 _("Go to Multipath Device"),
                                 GTK_RESPONSE_OK);
        g_signal_connect (info_bar,
                          "response",
                          G_CALLBACK (on_multipath_component_info_bar_go_to_multipath_device),
                          section);
        hbox = gtk_hbox_new (FALSE, 6);
        image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_DIALOG);
        gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
        label = gtk_label_new (NULL);
        gtk_label_set_markup (GTK_LABEL (label),
                              _("<b>NOTE:</b> This object represents one of several paths to the drive. "
                                "To perform operations on the drive, use the corresponding multipath object."));
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
        gtk_label_set_width_chars (GTK_LABEL (label), 70);
        gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
        gtk_container_add (GTK_CONTAINER (gtk_info_bar_get_content_area (GTK_INFO_BAR (info_bar))), hbox);
        gtk_box_pack_start (GTK_BOX (section), info_bar, FALSE, FALSE, 0);
        section->priv->multipath_component_info_bar = info_bar;

        /*------------------------------------- */

        label = gtk_label_new (NULL);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_box_pack_start (GTK_BOX (section), label, FALSE, FALSE, 0);
        section->priv->drive_label = label;

        align = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
        gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 0, 12, 0);
        gtk_box_pack_start (GTK_BOX (section), align, FALSE, FALSE, 0);

        vbox = gtk_vbox_new (FALSE, 6);
        gtk_container_add (GTK_CONTAINER (align), vbox);
        section->priv->main_vbox = vbox;

        /* -------------------------------------------------------------------------------- */

        elements = g_ptr_array_new_with_free_func (g_object_unref);

        element = gdu_details_element_new (_("Model:"), NULL, NULL);
        g_ptr_array_add (elements, element);
        section->priv->model_element = element;

        element = gdu_details_element_new (_("Serial Number:"), NULL, NULL);
        g_ptr_array_add (elements, element);
        section->priv->serial_element = element;

        element = gdu_details_element_new (_("Firmware Version:"), NULL, NULL);
        g_ptr_array_add (elements, element);
        section->priv->firmware_element = element;

        /* Translators: if you translate "World Wide Name", please include the abbreviation "WWN" */
        element = gdu_details_element_new (_("World Wide Name:"), NULL, NULL);
        g_ptr_array_add (elements, element);
        section->priv->wwn_element = element;

        element = gdu_details_element_new (_("Location:"), NULL, NULL);
        g_ptr_array_add (elements, element);
        section->priv->location_element = element;

        element = gdu_details_element_new (_("Device:"), NULL, NULL);
        g_ptr_array_add (elements, element);
        section->priv->device_element = element;

        element = gdu_details_element_new (_("Write Cache:"), NULL, NULL);
        g_ptr_array_add (elements, element);
        section->priv->write_cache_element = element;

        element = gdu_details_element_new (_("Rotation Rate:"), NULL, NULL);
        g_ptr_array_add (elements, element);
        section->priv->rotation_rate_element = element;

        element = gdu_details_element_new (_("Capacity:"), NULL, NULL);
        g_ptr_array_add (elements, element);
        section->priv->capacity_element = element;

        element = gdu_details_element_new (_("Connection:"), NULL, NULL);
        g_ptr_array_add (elements, element);
        section->priv->connection_element = element;

        element = gdu_details_element_new (_("Partitioning:"), NULL, NULL);
        g_ptr_array_add (elements, element);
        section->priv->partitioning_element = element;

        element = gdu_details_element_new (_("SMART Status:"), NULL, NULL);
        g_ptr_array_add (elements, element);
        section->priv->smart_element = element;

        table = gdu_details_table_new (2, elements);
        g_ptr_array_unref (elements);
        gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

        /* -------------------------------------------------------------------------------- */

        align = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
        gtk_alignment_set_padding (GTK_ALIGNMENT (align), 0, 0, 12, 0);
        gtk_box_pack_start (GTK_BOX (vbox), align, FALSE, FALSE, 0);

        /* -------------------------------------------------------------------------------- */

        elements = g_ptr_array_new_with_free_func (g_object_unref);

        button_element = gdu_button_element_new ("brasero",
                                                 _("Open CD/_DVD Application"),
                                                 _("Create and copy CDs and DVDs"));
        g_signal_connect (button_element,
                          "clicked",
                          G_CALLBACK (on_cddvd_button_clicked),
                          section);
        g_ptr_array_add (elements, button_element);
        section->priv->cddvd_button = button_element;

        button_element = gdu_button_element_new ("caja-gdu",
                                                 _("Format _Drive"),
                                                 _("Erase or partition the drive"));
        g_signal_connect (button_element,
                          "clicked",
                          G_CALLBACK (gdu_section_drive_on_format_button_clicked),
                          section);
        g_ptr_array_add (elements, button_element);
        section->priv->format_button = button_element;

        button_element = gdu_button_element_new ("gdu-check-disk",
                                                 _("SM_ART Data"),
                                                 _("View SMART data and run self-tests"));
        g_signal_connect (button_element,
                          "clicked",
                          G_CALLBACK (on_smart_button_clicked),
                          section);
        g_ptr_array_add (elements, button_element);
        section->priv->smart_button = button_element;

        button_element = gdu_button_element_new ("gdu-eject",
                                                 _("_Eject"),
                                                 _("Eject medium from the drive"));
        g_signal_connect (button_element,
                          "clicked",
                          G_CALLBACK (on_eject_button_clicked),
                          section);
        g_ptr_array_add (elements, button_element);
        section->priv->eject_button = button_element;

        button_element = gdu_button_element_new ("gdu-detach",
                                                 _("Safe Rem_oval"),
                                                 _("Power down the drive so it can be removed"));
        g_signal_connect (button_element,
                          "clicked",
                          G_CALLBACK (on_detach_button_clicked),
                          section);
        g_ptr_array_add (elements, button_element);
        section->priv->detach_button = button_element;

        button_element = gdu_button_element_new ("gtk-execute", /* TODO: better icon */
                                                 _("_Benchmark"),
                                                 _("Measure drive performance"));
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

        if (G_OBJECT_CLASS (gdu_section_drive_parent_class)->constructed != NULL)
                G_OBJECT_CLASS (gdu_section_drive_parent_class)->constructed (object);
}

static void
gdu_section_drive_class_init (GduSectionDriveClass *klass)
{
        GObjectClass *gobject_class;
        GduSectionClass *section_class;

        gobject_class = G_OBJECT_CLASS (klass);
        section_class = GDU_SECTION_CLASS (klass);

        gobject_class->finalize    = gdu_section_drive_finalize;
        gobject_class->constructed = gdu_section_drive_constructed;
        section_class->update      = gdu_section_drive_update;

        g_type_class_add_private (klass, sizeof (GduSectionDrivePrivate));
}

static void
gdu_section_drive_init (GduSectionDrive *section)
{
        section->priv = G_TYPE_INSTANCE_GET_PRIVATE (section, GDU_TYPE_SECTION_DRIVE, GduSectionDrivePrivate);
}

GtkWidget *
gdu_section_drive_new (GduShell       *shell,
                       GduPresentable *presentable)
{
        return GTK_WIDGET (g_object_new (GDU_TYPE_SECTION_DRIVE,
                                         "shell", shell,
                                         "presentable", presentable,
                                         NULL));
}
