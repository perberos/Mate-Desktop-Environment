/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-slow-unmount-dialog.c
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

#include <gdu-gtk/gdu-gtk.h>

#include "gdu-slow-unmount-dialog.h"

struct GduSlowUnmountDialogPrivate
{
        GduDevice *device;
        GduPool *pool;
        GduPresentable *presentable;
        gchar *device_name;

        GtkWidget *label;
        GtkWidget *progress_bar;
        GtkWidget *details_label;

        guint pulse_timer_id;
        gboolean still_unmounting;
};

enum
{
        PROP_0,
        PROP_DEVICE,
};

static void on_device_removed (GduDevice *device,
                               gpointer   user_data);
static void on_device_changed (GduDevice *device,
                               gpointer   user_data);
static void on_device_job_changed (GduDevice *device,
                                   gpointer   user_data);

G_DEFINE_TYPE (GduSlowUnmountDialog, gdu_slow_unmount_dialog, GTK_TYPE_DIALOG)

static void
gdu_slow_unmount_dialog_finalize (GObject *object)
{
        GduSlowUnmountDialog *dialog = GDU_SLOW_UNMOUNT_DIALOG (object);

        g_signal_handlers_disconnect_by_func (dialog->priv->device, on_device_removed, dialog);
        g_signal_handlers_disconnect_by_func (dialog->priv->device, on_device_changed, dialog);
        g_signal_handlers_disconnect_by_func (dialog->priv->device, on_device_job_changed, dialog);
        g_object_unref (dialog->priv->device);
        g_object_unref (dialog->priv->pool);
        g_object_unref (dialog->priv->presentable);
        if (dialog->priv->pulse_timer_id > 0)
                g_source_remove (dialog->priv->pulse_timer_id);
        g_free (dialog->priv->device_name);

        if (G_OBJECT_CLASS (gdu_slow_unmount_dialog_parent_class)->finalize != NULL)
                G_OBJECT_CLASS (gdu_slow_unmount_dialog_parent_class)->finalize (object);
}

static void
gdu_slow_unmount_dialog_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
        GduSlowUnmountDialog *dialog = GDU_SLOW_UNMOUNT_DIALOG (object);

        switch (property_id) {
        case PROP_DEVICE:
                g_value_set_object (value, dialog->priv->device);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        }
}

static void
gdu_slow_unmount_dialog_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
        GduSlowUnmountDialog *dialog = GDU_SLOW_UNMOUNT_DIALOG (object);

        switch (property_id) {
        case PROP_DEVICE:
                dialog->priv->device = g_value_dup_object (value);
                dialog->priv->pool = gdu_device_get_pool (dialog->priv->device);
                dialog->priv->presentable = gdu_pool_get_volume_by_device (dialog->priv->pool,
                                                                           dialog->priv->device);
                dialog->priv->device_name = gdu_presentable_get_name (dialog->priv->presentable);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        }
}

static gboolean
on_pulse_timeout (gpointer user_data)
{
        GduSlowUnmountDialog *dialog = GDU_SLOW_UNMOUNT_DIALOG (user_data);

        gtk_progress_bar_pulse (GTK_PROGRESS_BAR (dialog->priv->progress_bar));

        /* keep timeout around */
        return TRUE;
}

static gboolean
nuke_dialog_cb (gpointer user_data)
{
        GduSlowUnmountDialog *dialog = GDU_SLOW_UNMOUNT_DIALOG (user_data);

        gtk_widget_destroy (GTK_WIDGET (dialog));
        g_object_unref (dialog);

        return FALSE;
}

static void
check_still_unmounting (GduSlowUnmountDialog *dialog)
{
        gchar *s;

        if (!dialog->priv->still_unmounting)
                goto out;

        if (gdu_device_job_in_progress (dialog->priv->device) &&
            g_strcmp0 (gdu_device_job_get_id (dialog->priv->device), "FilesystemUnmount") == 0)
                goto out;

        dialog->priv->still_unmounting = FALSE;

        if (dialog->priv->pulse_timer_id > 0) {
                g_source_remove (dialog->priv->pulse_timer_id);
                dialog->priv->pulse_timer_id = 0;
        }
        gtk_widget_hide (dialog->priv->progress_bar);
        gtk_widget_hide (dialog->priv->details_label);

        /* Translators: %s is the name of the device */
        s = g_strdup_printf (_("It's now safe to remove \"%s\"."),
                             dialog->priv->device_name);
        gtk_label_set_markup (GTK_LABEL (dialog->priv->label), s);

        /* nuke this dialog after four seconds */
        g_timeout_add (4 * 1000,
                       nuke_dialog_cb,
                       g_object_ref (dialog));

 out:
        ;
}

static void
on_device_removed (GduDevice *device,
                   gpointer   user_data)
{
        GduSlowUnmountDialog *dialog = GDU_SLOW_UNMOUNT_DIALOG (user_data);
        /* when the device is removed, just destroy the dialog */
        gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
on_device_changed (GduDevice *device,
                   gpointer   user_data)
{
        GduSlowUnmountDialog *dialog = GDU_SLOW_UNMOUNT_DIALOG (user_data);
        check_still_unmounting (dialog);
}

static void
on_device_job_changed (GduDevice *device,
                       gpointer   user_data)
{
        GduSlowUnmountDialog *dialog = GDU_SLOW_UNMOUNT_DIALOG (user_data);
        check_still_unmounting (dialog);
}

static void
gdu_slow_unmount_dialog_constructed (GObject *object)
{
        GduSlowUnmountDialog *dialog = GDU_SLOW_UNMOUNT_DIALOG (object);
        GtkWidget *content_area;
        GtkWidget *align;
        GdkPixbuf *pixbuf;
        GtkWidget *vbox;
        GtkWidget *label;
        GtkWidget *progress_bar;
        gchar *s;


        gtk_window_set_title (GTK_WINDOW (dialog), dialog->priv->device_name);
        pixbuf = gdu_util_get_pixbuf_for_presentable (dialog->priv->presentable, GTK_ICON_SIZE_MENU);
        gtk_window_set_icon (GTK_WINDOW (dialog), pixbuf);
        g_object_unref (pixbuf);

        content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

        align = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
        gtk_alignment_set_padding (GTK_ALIGNMENT (align), 12, 12, 12, 12);
        gtk_box_pack_start (GTK_BOX (content_area), align, TRUE, TRUE, 0);

        vbox = gtk_vbox_new (FALSE, 12);
        gtk_container_add (GTK_CONTAINER (align), vbox);

        label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
        /* Translators: %s is the name of the device */
        s = g_strdup_printf (_("Writing data to \"%s\"..."),
                             dialog->priv->device_name);
        gtk_label_set_markup (GTK_LABEL (label), s);
        g_free (s);
        gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
        dialog->priv->label = label;

        progress_bar = gtk_progress_bar_new ();
        gtk_box_pack_start (GTK_BOX (vbox), progress_bar, FALSE, FALSE, 0);
        dialog->priv->progress_bar = progress_bar;

        label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
        s = g_strconcat ("<small>",
                         _("To prevent data loss, wait until this has finished before removing "
                           "media or disconnecting the device."),
                         "</small>",
                         NULL);
        gtk_label_set_markup (GTK_LABEL (label), s);
        g_free (s);
        gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
        dialog->priv->details_label = label;

        dialog->priv->pulse_timer_id = g_timeout_add (50,
                                                      on_pulse_timeout,
                                                      dialog);

        g_signal_connect (dialog->priv->device, "removed", G_CALLBACK (on_device_removed), dialog);
        g_signal_connect (dialog->priv->device, "changed", G_CALLBACK (on_device_changed), dialog);
        g_signal_connect (dialog->priv->device, "job-changed", G_CALLBACK (on_device_job_changed), dialog);

        dialog->priv->still_unmounting = TRUE;

        if (G_OBJECT_CLASS (gdu_slow_unmount_dialog_parent_class)->constructed != NULL)
                G_OBJECT_CLASS (gdu_slow_unmount_dialog_parent_class)->constructed (object);
}

static void
gdu_slow_unmount_dialog_class_init (GduSlowUnmountDialogClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        g_type_class_add_private (klass, sizeof (GduSlowUnmountDialogPrivate));

        object_class->get_property = gdu_slow_unmount_dialog_get_property;
        object_class->set_property = gdu_slow_unmount_dialog_set_property;
        object_class->constructed  = gdu_slow_unmount_dialog_constructed;
        object_class->finalize     = gdu_slow_unmount_dialog_finalize;

        g_object_class_install_property (object_class,
                                         PROP_DEVICE,
                                         g_param_spec_object ("device",
                                                              _("Device"),
                                                              _("The device to show the dialog for"),
                                                              GDU_TYPE_DEVICE,
                                                              G_PARAM_READABLE |
                                                              G_PARAM_WRITABLE |
                                                              G_PARAM_CONSTRUCT_ONLY));
}

static void
gdu_slow_unmount_dialog_init (GduSlowUnmountDialog *dialog)
{
        dialog->priv = G_TYPE_INSTANCE_GET_PRIVATE (dialog,
                                                    GDU_TYPE_SLOW_UNMOUNT_DIALOG,
                                                    GduSlowUnmountDialogPrivate);
}

GtkWidget *
gdu_slow_unmount_dialog_new (GtkWindow *parent,
                             GduDevice *device)
{
        return GTK_WIDGET (g_object_new (GDU_TYPE_SLOW_UNMOUNT_DIALOG,
                                         "transient-for", parent,
                                         "device", device,
                                         NULL));
}

