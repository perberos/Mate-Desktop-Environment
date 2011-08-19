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

#include "gdu-format-progress-dialog.h"

struct GduFormatProgressDialogPrivate
{
        GduDevice *device;
        GduPool *pool;
        GduPresentable *presentable;
        gchar *device_name;

        gchar *text;

        GtkWidget *label;
        GtkWidget *progress_bar;
        GtkWidget *details_label;

        guint pulse_timer_id;
};

enum
{
        PROP_0,
        PROP_DEVICE,
        PROP_TEXT,
};

G_DEFINE_TYPE (GduFormatProgressDialog, gdu_format_progress_dialog, GTK_TYPE_DIALOG)

static void
gdu_format_progress_dialog_finalize (GObject *object)
{
        GduFormatProgressDialog *dialog = GDU_FORMAT_PROGRESS_DIALOG (object);

        g_object_unref (dialog->priv->device);
        g_object_unref (dialog->priv->pool);
        g_object_unref (dialog->priv->presentable);
        if (dialog->priv->pulse_timer_id > 0)
                g_source_remove (dialog->priv->pulse_timer_id);
        g_free (dialog->priv->device_name);

        if (G_OBJECT_CLASS (gdu_format_progress_dialog_parent_class)->finalize != NULL)
                G_OBJECT_CLASS (gdu_format_progress_dialog_parent_class)->finalize (object);
}

static void
gdu_format_progress_dialog_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
        GduFormatProgressDialog *dialog = GDU_FORMAT_PROGRESS_DIALOG (object);

        switch (property_id) {
        case PROP_DEVICE:
                g_value_set_object (value, dialog->priv->device);
                break;

        case PROP_TEXT:
                g_value_set_string (value, dialog->priv->text);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
gdu_format_progress_dialog_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
        GduFormatProgressDialog *dialog = GDU_FORMAT_PROGRESS_DIALOG (object);

        switch (property_id) {
        case PROP_DEVICE:
                dialog->priv->device = g_value_dup_object (value);
                dialog->priv->pool = gdu_device_get_pool (dialog->priv->device);
                dialog->priv->presentable = gdu_pool_get_volume_by_device (dialog->priv->pool,
                                                                           dialog->priv->device);
                dialog->priv->device_name = gdu_presentable_get_name (dialog->priv->presentable);
                break;

        case PROP_TEXT:
                g_free (dialog->priv->text);
                dialog->priv->text = g_value_dup_string (value);
                if (dialog->priv->label != NULL) {
                        gtk_label_set_markup (GTK_LABEL (dialog->priv->label),
                                              dialog->priv->text);
                }
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static gboolean
on_pulse_timeout (gpointer user_data)
{
        GduFormatProgressDialog *dialog = GDU_FORMAT_PROGRESS_DIALOG (user_data);

        /* TODO: would be nice to show a progress bar instead of pulse once
         *       DeviceKit-disks is able to report progress on FilesystemCreate()
         */

        gtk_progress_bar_pulse (GTK_PROGRESS_BAR (dialog->priv->progress_bar));

        /* keep timeout around */
        return TRUE;
}

static void
gdu_format_progress_dialog_constructed (GObject *object)
{
        GduFormatProgressDialog *dialog = GDU_FORMAT_PROGRESS_DIALOG (object);
        GtkWidget *content_area;
        GtkWidget *align;
        GtkWidget *vbox;
        GtkWidget *label;
        GtkWidget *progress_bar;
        gchar *s;


        gtk_dialog_add_button (GTK_DIALOG (dialog),
                               GTK_STOCK_CANCEL,
                               GTK_RESPONSE_CANCEL);

        gtk_window_set_title (GTK_WINDOW (dialog), "");
        gtk_window_set_icon_name (GTK_WINDOW (dialog), "caja-gdu");

        content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

        align = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
        gtk_alignment_set_padding (GTK_ALIGNMENT (align), 12, 12, 12, 12);
        gtk_box_pack_start (GTK_BOX (content_area), align, TRUE, TRUE, 0);

        vbox = gtk_vbox_new (FALSE, 12);
        gtk_container_add (GTK_CONTAINER (align), vbox);

        label = gtk_label_new (NULL);
        gtk_label_set_markup (GTK_LABEL (label), dialog->priv->text);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
        gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
        dialog->priv->label = label;

        progress_bar = gtk_progress_bar_new ();
        gtk_box_pack_start (GTK_BOX (vbox), progress_bar, FALSE, FALSE, 0);
        dialog->priv->progress_bar = progress_bar;

        label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
        s = g_strconcat ("<small>",
                         _("To prevent data corruption, wait until this has finished before removing "
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

        if (G_OBJECT_CLASS (gdu_format_progress_dialog_parent_class)->constructed != NULL)
                G_OBJECT_CLASS (gdu_format_progress_dialog_parent_class)->constructed (object);
}

static void
gdu_format_progress_dialog_class_init (GduFormatProgressDialogClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        g_type_class_add_private (klass, sizeof (GduFormatProgressDialogPrivate));

        object_class->get_property = gdu_format_progress_dialog_get_property;
        object_class->set_property = gdu_format_progress_dialog_set_property;
        object_class->constructed  = gdu_format_progress_dialog_constructed;
        object_class->finalize     = gdu_format_progress_dialog_finalize;

        g_object_class_install_property (object_class,
                                         PROP_DEVICE,
                                         g_param_spec_object ("device",
                                                              _("Device"),
                                                              _("The device to show the dialog for"),
                                                              GDU_TYPE_DEVICE,
                                                              G_PARAM_READABLE |
                                                              G_PARAM_WRITABLE |
                                                              G_PARAM_CONSTRUCT_ONLY |
                                                              G_PARAM_STATIC_NAME |
                                                              G_PARAM_STATIC_NICK |
                                                              G_PARAM_STATIC_BLURB));

        g_object_class_install_property (object_class,
                                         PROP_TEXT,
                                         g_param_spec_string ("text",
                                                              _("text"),
                                                              _("Text to show"),
                                                              NULL,
                                                              G_PARAM_READABLE |
                                                              G_PARAM_WRITABLE |
                                                              G_PARAM_CONSTRUCT |
                                                              G_PARAM_STATIC_NAME |
                                                              G_PARAM_STATIC_NICK |
                                                              G_PARAM_STATIC_BLURB));
}

static void
gdu_format_progress_dialog_init (GduFormatProgressDialog *dialog)
{
        dialog->priv = G_TYPE_INSTANCE_GET_PRIVATE (dialog,
                                                    GDU_TYPE_FORMAT_PROGRESS_DIALOG,
                                                    GduFormatProgressDialogPrivate);
}

GtkWidget *
gdu_format_progress_dialog_new (GtkWindow   *parent,
                                GduDevice   *device,
                                const gchar *text)
{
        return GTK_WIDGET (g_object_new (GDU_TYPE_FORMAT_PROGRESS_DIALOG,
                                         "transient-for", parent,
                                         "device", device,
                                         "text", text,
                                         NULL));
}

void
gdu_format_progress_dialog_set_text (GduFormatProgressDialog *dialog,
                                     const gchar             *text)
{
        g_return_if_fail (GDU_IS_FORMAT_PROGRESS_DIALOG (dialog));
        g_object_set (dialog, "text", text, NULL);
}
