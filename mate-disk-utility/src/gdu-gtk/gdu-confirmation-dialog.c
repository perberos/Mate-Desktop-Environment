/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* gdu-confirmation-dialog.c
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
#include <glib/gi18n-lib.h>

#include <atasmart.h>
#include <glib/gstdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "gdu-confirmation-dialog.h"

struct GduConfirmationDialogPrivate
{
        gchar *message;
        gchar *button_text;
};

enum
{
        PROP_0,
        PROP_MESSAGE,
        PROP_BUTTON_TEXT
};


G_DEFINE_TYPE (GduConfirmationDialog, gdu_confirmation_dialog, GDU_TYPE_DIALOG)

static void
gdu_confirmation_dialog_finalize (GObject *object)
{
        GduConfirmationDialog *dialog = GDU_CONFIRMATION_DIALOG (object);

        g_free (dialog->priv->message);

        if (G_OBJECT_CLASS (gdu_confirmation_dialog_parent_class)->finalize != NULL)
                G_OBJECT_CLASS (gdu_confirmation_dialog_parent_class)->finalize (object);
}

static void
gdu_confirmation_dialog_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
        GduConfirmationDialog *dialog = GDU_CONFIRMATION_DIALOG (object);

        switch (property_id) {
        case PROP_MESSAGE:
                g_value_set_string (value, dialog->priv->message);
                break;

        case PROP_BUTTON_TEXT:
                g_value_set_string (value, dialog->priv->button_text);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
gdu_confirmation_dialog_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
        GduConfirmationDialog *dialog = GDU_CONFIRMATION_DIALOG (object);

        switch (property_id) {
        case PROP_MESSAGE:
                dialog->priv->message = g_value_dup_string (value);
                break;

        case PROP_BUTTON_TEXT:
                dialog->priv->button_text = g_value_dup_string (value);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

/* ---------------------------------------------------------------------------------------------------- */

static void
gdu_confirmation_dialog_constructed (GObject *object)
{
        GduConfirmationDialog *dialog = GDU_CONFIRMATION_DIALOG (object);
        GtkWidget *content_area;
        GtkWidget *hbox;
        GtkWidget *vbox;
        GtkWidget *image;
        GtkWidget *label;
        gchar *s;
        GIcon *icon;
        GEmblem *emblem;
        GIcon *confirmation_icon;
        GIcon *emblemed_icon;
        gchar *name;
        gchar *vpd_name;

        icon = NULL;
        name = NULL;
        vpd_name = NULL;

        gtk_window_set_title (GTK_WINDOW (dialog), "");
        gtk_container_set_border_width (GTK_CONTAINER (dialog), 12);

        gtk_dialog_add_button (GTK_DIALOG (dialog),
                               GTK_STOCK_CANCEL,
                               GTK_RESPONSE_CANCEL);

        gtk_dialog_add_button (GTK_DIALOG (dialog),
                               dialog->priv->button_text,
                               GTK_RESPONSE_OK);

        content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

        icon = gdu_presentable_get_icon (gdu_dialog_get_presentable (GDU_DIALOG (dialog)));
        confirmation_icon = g_themed_icon_new (GTK_STOCK_DIALOG_WARNING);
        emblem = g_emblem_new (icon);
        emblemed_icon = g_emblemed_icon_new (confirmation_icon,
                                             emblem);

        hbox = gtk_hbox_new (FALSE, 12);
        gtk_box_pack_start (GTK_BOX (content_area), hbox, TRUE, TRUE, 0);

        image = gtk_image_new_from_gicon (emblemed_icon,
                                          GTK_ICON_SIZE_DIALOG);
        gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);
        gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);

        vbox = gtk_vbox_new (FALSE, 12);
        gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);

        s = g_strdup_printf ("<big><big><b>%s</b></big></big>",
                             dialog->priv->message);
        label = gtk_label_new (NULL);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
        gtk_label_set_markup (GTK_LABEL (label), s);
        g_free (s);
        gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

        name = gdu_presentable_get_name (gdu_dialog_get_presentable (GDU_DIALOG (dialog)));
        vpd_name = gdu_presentable_get_vpd_name (gdu_dialog_get_presentable (GDU_DIALOG (dialog)));
        if (GDU_IS_VOLUME (gdu_dialog_get_presentable (GDU_DIALOG (dialog)))) {
                s = g_strdup_printf (_("This operation concerns the volume \"%s\" (%s)"),
                                     name,
                                     vpd_name);
        } else if (GDU_IS_DRIVE (gdu_dialog_get_presentable (GDU_DIALOG (dialog)))) {
                s = g_strdup_printf (_("This operation concerns the drive \"%s\" (%s)"),
                                     name,
                                     vpd_name);
        } else {
                s = g_strdup_printf (_("This operation concerns \"%s\" (%s)"),
                                     name,
                                     vpd_name);
        }

        label = gtk_label_new (s);
        g_free (s);
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
        gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

        g_object_unref (icon);
        g_object_unref (emblem);
        g_object_unref (confirmation_icon);
        g_object_unref (emblemed_icon);

        g_free (name);
        g_free (vpd_name);

        if (G_OBJECT_CLASS (gdu_confirmation_dialog_parent_class)->constructed != NULL)
                G_OBJECT_CLASS (gdu_confirmation_dialog_parent_class)->constructed (object);
}

static void
gdu_confirmation_dialog_class_init (GduConfirmationDialogClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        g_type_class_add_private (klass, sizeof (GduConfirmationDialogPrivate));

        object_class->get_property = gdu_confirmation_dialog_get_property;
        object_class->set_property = gdu_confirmation_dialog_set_property;
        object_class->constructed  = gdu_confirmation_dialog_constructed;
        object_class->finalize     = gdu_confirmation_dialog_finalize;

        g_object_class_install_property (object_class,
                                         PROP_MESSAGE,
                                         g_param_spec_string ("message",
                                                              NULL,
                                                              NULL,
                                                              NULL,
                                                              G_PARAM_READABLE |
                                                              G_PARAM_WRITABLE |
                                                              G_PARAM_CONSTRUCT_ONLY));

        g_object_class_install_property (object_class,
                                         PROP_BUTTON_TEXT,
                                         g_param_spec_string ("button-text",
                                                              NULL,
                                                              NULL,
                                                              GTK_STOCK_DELETE,
                                                              G_PARAM_READABLE |
                                                              G_PARAM_WRITABLE |
                                                              G_PARAM_CONSTRUCT_ONLY));
}

static void
gdu_confirmation_dialog_init (GduConfirmationDialog *dialog)
{
        dialog->priv = G_TYPE_INSTANCE_GET_PRIVATE (dialog, GDU_TYPE_CONFIRMATION_DIALOG, GduConfirmationDialogPrivate);
}

GtkWidget *
gdu_confirmation_dialog_new (GtkWindow      *parent,
                             GduPresentable *presentable,
                             const gchar    *message,
                             const gchar    *button_text)
{
        g_return_val_if_fail (GDU_IS_PRESENTABLE (presentable), NULL);
        return GTK_WIDGET (g_object_new (GDU_TYPE_CONFIRMATION_DIALOG,
                                         "transient-for", parent,
                                         "presentable", presentable,
                                         "message", message,
                                         "button-text", button_text,
                                         NULL));
}

GtkWidget *
gdu_confirmation_dialog_new_for_drive (GtkWindow      *parent,
                                       GduDevice      *device,
                                       const gchar    *message,
                                       const gchar    *button_text)
{
        g_return_val_if_fail (GDU_IS_DEVICE (device), NULL);
        return GTK_WIDGET (g_object_new (GDU_TYPE_CONFIRMATION_DIALOG,
                                         "transient-for", parent,
                                         "drive-device", device,
                                         "message", message,
                                         "button-text", button_text,
                                         NULL));
}

GtkWidget *
gdu_confirmation_dialog_new_for_volume (GtkWindow      *parent,
                                        GduDevice      *device,
                                        const gchar    *message,
                                        const gchar    *button_text)
{
        g_return_val_if_fail (GDU_IS_DEVICE (device), NULL);
        return GTK_WIDGET (g_object_new (GDU_TYPE_CONFIRMATION_DIALOG,
                                         "transient-for", parent,
                                         "volume-device", device,
                                         "message", message,
                                         "button-text", button_text,
                                         NULL));
}

/* ---------------------------------------------------------------------------------------------------- */
