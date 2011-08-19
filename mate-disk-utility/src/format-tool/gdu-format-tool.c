/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 *  mate-disk-utility-format.c
 *
 *  Copyright (C) 2008-2009 Red Hat, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Author: Tomas Bzatek <tbzatek@redhat.com>
 *          David Zeuthen <davidz@redhat.com>
 *
 */

#include "config.h"

#include "gdu/gdu.h"
#include "gdu-gtk/gdu-gtk.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <gio/gio.h>

#include <gtk/gtk.h>

#include <stdlib.h>

#include "gdu-format-progress-dialog.h"

typedef struct
{
        GMainLoop *loop;
        GError    *error;
        gchar     *mount_point;
        gboolean   format_in_progress;
} FormatData;

static gboolean
on_hack_timeout (gpointer user_data)
{
        GMainLoop *loop = user_data;
        g_main_loop_quit (loop);
        return FALSE;
}

static void
fs_mount_cb (GduDevice *device,
             gchar     *mount_point,
             GError    *error,
             gpointer   user_data)
{
        FormatData *data = user_data;
        if (error != NULL)
                data->error = error;
        else
                data->mount_point = g_strdup (mount_point);
        g_main_loop_quit (data->loop);
}

static void
fs_create_cb (GduDevice *device,
              GError    *error,
              gpointer   user_data)
{
        FormatData *data = user_data;
        if (error != NULL)
                data->error = error;
        g_main_loop_quit (data->loop);
}

static void
show_error_dialog (GtkWindow *parent,
                   const gchar *primary,
                   const gchar *secondary)
{
        GtkWidget *dialog;

        dialog = gtk_message_dialog_new_with_markup (parent,
                                                     GTK_DIALOG_MODAL,
                                                     GTK_MESSAGE_ERROR,
                                                     GTK_BUTTONS_OK,
                                                     "<b><big><big>%s</big></big></b>\n\n%s",
                                                     primary,
                                                     secondary);
        gtk_widget_show_all (dialog);
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
}

static void
launch_palimpsest (const gchar *device_file)
{
        gchar *command_line;
        GError *error;

        command_line = g_strdup_printf ("palimpsest --show-volume \"%s\"", device_file);

        error = NULL;
        if (!g_spawn_command_line_async (command_line, &error)) {
                show_error_dialog (NULL,
                                   _("Error launching Disk Utility"),
                                   error->message);
                g_error_free (error);
        }
        g_free (command_line);
}

static void
launch_file_manager (const gchar *mount_point)
{
        gchar *command_line;

        /* Caja itself will complain on errors so no need to do error handling*/
        command_line = g_strdup_printf ("caja \"%s\"", mount_point);
        g_spawn_command_line_async (command_line, NULL);
        g_free (command_line);
}

static gchar *device_file = NULL;
static GOptionEntry entries[] = {
        { "device-file", 'd', 0, G_OPTION_ARG_FILENAME, &device_file, N_("Device to format"), N_("DEVICE") },
        { NULL }
};

typedef struct
{
        GMainLoop *loop;
        GError    *error;
} UnmountData;

static void
unmount_cb (GObject      *source_object,
            GAsyncResult *res,
            gpointer      user_data)
{
        GMount *mount = G_MOUNT (source_object);
        UnmountData *data = user_data;

        g_mount_unmount_with_operation_finish (mount, res, &data->error);

        g_main_loop_quit (data->loop);
}

static void
on_progress_dialog_response (GtkDialog *dialog,
                             gint       response_id,
                             gpointer   user_data)
{
        FormatData *data = user_data;

        data->error = g_error_new (G_IO_ERROR, G_IO_ERROR_CANCELLED, _("Operation was canceled"));
        g_main_loop_quit (data->loop);
}

static gboolean
on_grace_timeout (gpointer user_data)
{
        FormatData *data = user_data;
        g_main_loop_quit (data->loop);
        return FALSE;
}

int
main (int argc, char *argv[])
{
        int ret;
        GError *error;
        GduPool *pool;
        GduDevice *device;
        GduDevice *device_to_unmount;
        GduDevice *device_to_mount;
        GduPresentable *volume;
        gchar *volume_name;
        gchar *confirmation_secondary;
        GtkWidget *dialog;
        gint response;
        gchar *fs_type;
        gchar *fs_label;
        gboolean encrypt;
        gchar *passphrase;
        gboolean save_passphrase_in_keyring;
        gboolean save_passphrase_in_keyring_session;
        GMainLoop *loop;
        GVolumeMonitor *gvolume_monitor;
        GList *l;
        GList *gvolumes;
        GVolume *gvolume;
        GMount *gmount;
        gboolean take_ownership;
        FormatData *format_data;
        GduPresentable *toplevel;
        gchar *drive_name;
        gchar *format_desc;
        gchar *formatting_desc;
        gchar *size_str;
        gint grace_timeout_id;

        ret = 1;
        pool = NULL;
        device = NULL;
        device_to_unmount = NULL;
        device_to_mount = NULL;
        volume = NULL;
        volume_name = NULL;
        confirmation_secondary = NULL;
        dialog = NULL;
        fs_type = NULL;
        fs_label = NULL;
        passphrase = NULL;
        loop = NULL;
        volume = NULL;
        gmount = NULL;
        gvolume_monitor = NULL;
        gvolumes = NULL;
        gvolume = NULL;
        gmount = NULL;
        format_data = NULL;
        toplevel = NULL;
        drive_name = NULL;
        format_desc = NULL;
        formatting_desc = NULL;
        size_str = NULL;

        g_thread_init (NULL);

        /* Initialize gettext support */
        bindtextdomain (GETTEXT_PACKAGE, MATELOCALEDIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);

        /*  Initialize gtk  */
        error = NULL;
        if (! gtk_init_with_args (&argc, &argv, NULL, entries, GETTEXT_PACKAGE, &error)) {
                g_printerr ("Could not parse arguments: %s\n", error->message);
                g_error_free (error);
                goto out;
        }

        loop = g_main_loop_new (NULL, FALSE);

        g_set_prgname ("gdu-format-tool");
        g_set_application_name (_("Mate Disk Utility formatting tool"));

        if (device_file == NULL) {
                g_printerr ("Incorrect usage. Try --help.\n");
                goto out;
        }

        pool = gdu_pool_new ();
        if (pool == NULL) {
                g_warning ("Unable to get device pool");
                goto out;
        }

        device = gdu_pool_get_by_device_file (pool, device_file);
        if (device == NULL) {
                g_printerr ("No device for %s\n", device_file);
                goto out;
        }

        /* if the user specified a luks device, find the slave backing device */
        if (gdu_device_is_luks_cleartext (device)) {
                const gchar *slave;
                slave = gdu_device_luks_cleartext_get_slave (device);
                g_object_ref (device);
                device = gdu_pool_get_by_object_path (pool, slave);
                /* don't support LUKS in LUKS... */
                g_assert (!gdu_device_is_luks_cleartext (device));
        }

        volume = gdu_pool_get_volume_by_device (pool, device);
        if (volume == NULL) {
                g_printerr ("%s is not a volume\n", device_file);
                goto out;
        }
        volume_name = gdu_presentable_get_name (volume);

        toplevel = gdu_presentable_get_toplevel (GDU_PRESENTABLE (volume));
        if (toplevel != NULL && GDU_IS_DRIVE (toplevel)) {
                drive_name = gdu_presentable_get_name (toplevel);
        }
        size_str = gdu_util_get_size_for_display (gdu_device_get_size (device),
                                                  FALSE,
                                                  FALSE);

        if (drive_name != NULL) {
                if (gdu_device_is_partition (device)) {
                        /* Translators: First argument is the partition number, second argument is the drive name,
                         * third argument is the size (e.g. 10 GB)
                         */
                        format_desc = g_strdup_printf (_("Format partition %d of %s (%s)"),
                                                       gdu_device_partition_get_number (device),
                                                       drive_name,
                                                       size_str);
                        /* Translators: First argument is the partition number, second argument is the drive name,
                         * third argument is the size (e.g. 10 GB)
                         */
                        formatting_desc = g_strdup_printf (_("Formatting partition %d of %s (%s)"),
                                                           gdu_device_partition_get_number (device),
                                                           drive_name,
                                                           size_str);
                } else {
                        /* Translators: First argument is the drive name, second argument is the size (e.g. 10 GB) */
                        format_desc = g_strdup_printf (_("Format %s (%s)"),
                                                       drive_name,
                                                       size_str);
                        /* Translators: First argument is the drive name, second argument is the size (e.g. 10 GB) */
                        formatting_desc = g_strdup_printf (_("Formatting %s (%s)"),
                                                           drive_name,
                                                           size_str);
                }
        } else {
                /* Translators: First argument is the size (e.g. 10 GB), second is the device (e.g. /dev/md0) */
                format_desc = g_strdup_printf (_("Format %s Volume (%s)"),
                                               size_str,
                                               gdu_device_get_device_file (device));
                /* Translators: First argument is the size (e.g. 10 GB), second is the device (e.g. /dev/md0) */
                formatting_desc = g_strdup_printf (_("Formatting %s Volume (%s)"),
                                                   size_str,
                                                   gdu_device_get_device_file (device));
        }

        dialog = gdu_format_dialog_new (NULL, /* no parent window */
                                        volume,
                                        GDU_FORMAT_DIALOG_FLAGS_SIMPLE |
                                        GDU_FORMAT_DIALOG_FLAGS_DISK_UTILITY_BUTTON);
        gtk_window_set_title (GTK_WINDOW (dialog), format_desc);
        gtk_window_set_icon_name (GTK_WINDOW (dialog), "caja-gdu");
        gtk_widget_show_all (dialog);

        response = gtk_dialog_run (GTK_DIALOG (dialog));

        switch (response) {
        case GTK_RESPONSE_OK:
                break;

        case GTK_RESPONSE_ACCEPT:
                gtk_widget_destroy (dialog);
                dialog = NULL;
                launch_palimpsest (device_file);
                goto out;

        default: /* explicit fallthrough */
        case GTK_RESPONSE_CANCEL:
                goto out;
        }

        fs_type = gdu_format_dialog_get_fs_type (GDU_FORMAT_DIALOG (dialog));
        fs_label = gdu_format_dialog_get_fs_label (GDU_FORMAT_DIALOG (dialog));
        encrypt = gdu_format_dialog_get_encrypt (GDU_FORMAT_DIALOG (dialog));
        gtk_widget_destroy (dialog);
        dialog = NULL;

        /* Ask for passphrase if needed */
        passphrase = NULL;
        save_passphrase_in_keyring = FALSE;
        save_passphrase_in_keyring_session = FALSE;
        if (encrypt) {
                passphrase = gdu_util_dialog_ask_for_new_secret (NULL,
                                                                 &save_passphrase_in_keyring,
                                                                 &save_passphrase_in_keyring_session);
                if (passphrase == NULL)
                        goto out;
        }

        take_ownership = (g_strcmp0 (fs_type, "vfat") != 0);

        format_data = g_new0 (FormatData, 1);
        format_data->loop = loop;
        format_data->error = NULL;

        dialog = gdu_format_progress_dialog_new (NULL,
                                                 device,
                                                 _("Preparing..."));
        gtk_window_set_title (GTK_WINDOW (dialog), formatting_desc);
        gtk_widget_show_all (dialog);
        g_signal_connect (dialog, "response", G_CALLBACK (on_progress_dialog_response), format_data);

        /* first a small 2.5 sec window to allow the user to cancel before initiating */
        grace_timeout_id = g_timeout_add (2500, on_grace_timeout, format_data);
        g_main_loop_run (loop);
        g_source_remove (grace_timeout_id);
        if (format_data->error != NULL &&
            format_data->error->domain == G_IO_ERROR &&
            format_data->error->code == G_IO_ERROR_CANCELLED) {
                /* cancelled already, we are done */
                goto out;
        }

        /* then unmount the mount if needed; we use GIO for this to handle tearing
         * down LUKS; first determine the device to unmount
         */
        if (gdu_device_is_luks (device)) {
                const gchar *holder;
                holder = gdu_device_luks_get_holder (device);
                device_to_unmount = gdu_pool_get_by_object_path (pool, holder);
        } else {
                device_to_unmount = g_object_ref (device);
        }
        g_assert (device_to_unmount != NULL);
        gvolume_monitor = g_volume_monitor_get ();
        gvolumes = g_volume_monitor_get_volumes (gvolume_monitor);
        for (l = gvolumes; l != NULL; l = l->next) {
                GVolume *v = G_VOLUME (l->data);
                if (g_strcmp0 (g_volume_get_identifier (v, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE),
                               gdu_device_get_device_file (device_to_unmount)) == 0) {
                        gvolume = g_object_ref (v);
                        break;
                }
        }
        if (gvolume != NULL) {
                gmount = g_volume_get_mount (gvolume);
                if (gmount != NULL) {
                        GMountOperation *mount_op;

                        gdu_format_progress_dialog_set_text (GDU_FORMAT_PROGRESS_DIALOG (dialog),
                                                             _("Unmounting..."));

                        mount_op = gtk_mount_operation_new (GTK_WINDOW (dialog));
                        g_mount_unmount_with_operation (gmount,
                                                        G_MOUNT_UNMOUNT_NONE,
                                                        mount_op,
                                                        NULL,
                                                        unmount_cb,
                                                        format_data);
                        g_object_unref (mount_op);
                        g_main_loop_run (format_data->loop);

                        if (format_data->error != NULL) {

                                if (!(format_data->error->domain == G_IO_ERROR &&
                                      format_data->error->code == G_IO_ERROR_FAILED_HANDLED)) {
                                        gchar *primary;

                                        primary = g_strdup_printf (_("Unable to format '%s'"), volume_name);
                                        show_error_dialog (NULL,
                                                           primary,
                                                           format_data->error->message);

                                        g_free (primary);
                                }
                                g_error_free (format_data->error);
                                goto out;
                        }
                }
        }

        /* and now, kick off the operation */
        gdu_format_progress_dialog_set_text (GDU_FORMAT_PROGRESS_DIALOG (dialog),
                                             _("Formatting..."));
        gdu_device_op_filesystem_create (device,
                                         fs_type,
                                         fs_label,
                                         passphrase,
                                         take_ownership,
                                         fs_create_cb,
                                         format_data);
 again:
        g_main_loop_run (loop);
        if (format_data->error != NULL) {
                gtk_widget_destroy (dialog);
                dialog = NULL;

                if (format_data->error->domain == G_IO_ERROR &&
                    format_data->error->code == G_IO_ERROR_CANCELLED) {
                        gdu_format_progress_dialog_set_text (GDU_FORMAT_PROGRESS_DIALOG (dialog),
                                                             _("Cancelling..."));

                        /* cancel the job; no callback since op_filesystem_create will return an error */
                        gdu_device_op_cancel_job (device,
                                                  NULL,
                                                  NULL);
                        goto again;

                } else {
                        show_error_dialog (NULL,
                                           _("Error formatting volume"),
                                           format_data->error->message);
                        g_error_free (format_data->error);
                }
                goto out;
        }

        /* ugh, DeviceKit-disks bug - spin around in the mainloop for some time to ensure we
         * have gotten all changes
         */
        gdu_format_progress_dialog_set_text (GDU_FORMAT_PROGRESS_DIALOG (dialog),
                                             _("Mounting volume..."));
        g_timeout_add (1500, on_hack_timeout, loop);
        g_main_loop_run (loop);

        /* OK, peachy, now mount the volume and open a window */
        if (passphrase != NULL) {
                const gchar *cleartext_objpath;

                g_assert (gdu_device_is_luks (device));
                cleartext_objpath = gdu_device_luks_get_holder (device);
                device_to_mount = gdu_pool_get_by_object_path (pool, cleartext_objpath);
        } else {
                device_to_mount = g_object_ref (device);
        }

        gdu_device_op_filesystem_mount (device_to_mount,
                                        NULL,
                                        fs_mount_cb,
                                        format_data);
        g_main_loop_run (loop);
        gtk_widget_destroy (dialog);
        dialog = NULL;
        if (format_data->error != NULL) {
                show_error_dialog (NULL,
                                   _("Error mounting device"),
                                   format_data->error->message);
                g_error_free (format_data->error);
                goto out;
        }

        /* open file manager */
        launch_file_manager (format_data->mount_point);

        g_free (format_data->mount_point);

        /* save passphrase in keyring if requested */
        if (passphrase != NULL && (save_passphrase_in_keyring || save_passphrase_in_keyring_session)) {
                if (!gdu_util_save_secret (device,
                                           passphrase,
                                           save_passphrase_in_keyring_session)) {
                        show_error_dialog (NULL,
                                           _("Error storing passphrase in keyring"),
                                           "");
                        goto out;
                }
        }

        ret = 0;

 out:
        if (toplevel != NULL)
                g_object_unref (toplevel);
        g_free (drive_name);
        g_free (size_str);
        g_free (format_desc);
        g_free (formatting_desc);
        if (gmount != NULL)
                g_object_unref (gmount);
        if (gvolume != NULL)
                g_object_unref (gvolume);
        g_list_foreach (gvolumes, (GFunc) g_object_unref, NULL);
        if (gvolume_monitor != NULL)
                g_object_unref (gvolume_monitor);
        g_free (confirmation_secondary);
        g_free (volume_name);
        g_free (passphrase);
        g_free (fs_type);
        g_free (fs_label);
        g_free (device_file);
        if (loop != NULL)
                g_main_loop_unref (loop);
        if (dialog != NULL)
                gtk_widget_destroy (dialog);
        if (volume != NULL)
                g_object_unref (volume);
        if (device != NULL)
                g_object_unref (device);
        if (device_to_unmount != NULL)
                g_object_unref (device_to_unmount);
        if (device_to_mount != NULL)
                g_object_unref (device_to_mount);
        if (pool != NULL)
                g_object_unref (pool);
        if (format_data != NULL)
                g_free (format_data);
        return ret;
}
